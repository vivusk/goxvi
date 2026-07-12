#include "key-event-handler.h"

#include <inputscope.h>
#include <wrl/client.h>

#include "edit-session-callbacks.h"

using Microsoft::WRL::ComPtr;

namespace goxvi_keys {

KeyDecision classifyKey(WPARAM vk, goxvi::InputMethod inputMethod) {
  switch (vk) {  // bare modifiers that must not disturb the composition
    case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT:
    case VK_CAPITAL: case VK_NUMLOCK: case VK_SCROLL:
      return {KeyAction::Ignore, 0};
    default:
      break;
  }

  // Ctrl/Alt/Win (bare or chorded): commit immediately, keep app shortcuts.
  if ((GetKeyState(VK_CONTROL) & 0x8000) || (GetKeyState(VK_MENU) & 0x8000) ||
      (GetKeyState(VK_LWIN) & 0x8000) || (GetKeyState(VK_RWIN) & 0x8000)) {
    return {KeyAction::CommitAndPassChord, 0};
  }

  if (vk >= 'A' && vk <= 'Z') {
    const bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    const bool caps = (GetKeyState(VK_CAPITAL) & 1) != 0;
    const bool upper = shift != caps;
    const wchar_t ch = static_cast<wchar_t>(vk);
    return {KeyAction::ProcessLetter,
            upper ? ch : static_cast<wchar_t>(ch + 32)};
  }

  if (inputMethod == goxvi::InputMethod::Vni && vk >= '0' && vk <= '9') {
    return {KeyAction::ProcessLetter, static_cast<wchar_t>(vk)};
  }

  if (vk == VK_BACK) return {KeyAction::ProcessBackspace, 0};

  if (vk == VK_ESCAPE) return {KeyAction::CancelToRaw, 0};

  // Everything else — space, punctuation, digits, Enter, Tab, arrows,
  // Delete, function keys — terminates the word.
  return {KeyAction::CommitAndPass, 0};
}

namespace {

bool compartmentIsSet(ITfCompartmentMgr* mgr, REFGUID guid) {
  ComPtr<ITfCompartment> compartment;
  if (FAILED(mgr->GetCompartment(guid, &compartment))) return false;
  VARIANT var;
  VariantInit(&var);
  bool set = false;
  if (SUCCEEDED(compartment->GetValue(&var))) {
    set = var.vt == VT_I4 && var.lVal != 0;
    VariantClear(&var);
  }
  return set;
}

}  // namespace

bool isContextDisabled(ITfContext* context) {
  ComPtr<ITfCompartmentMgr> mgr;
  if (FAILED(context->QueryInterface(IID_PPV_ARGS(&mgr)))) return false;
  return compartmentIsSet(mgr.Get(), GUID_COMPARTMENT_KEYBOARD_DISABLED) ||
         compartmentIsSet(mgr.Get(), GUID_COMPARTMENT_EMPTYCONTEXT);
}

bool isNavigationKey(WPARAM vk) {
  switch (vk) {
    case VK_PRIOR: case VK_NEXT: case VK_UP: case VK_DOWN:
    case VK_LEFT: case VK_RIGHT: case VK_HOME: case VK_END:
    case VK_INSERT: case VK_DELETE:
      return true;
    default:
      return false;
  }
}

void clearFocusedEditControlSelection() {
  HWND focus = GetFocus();
  if (!focus) return;
  wchar_t cls[64];
  if (!GetClassNameW(focus, cls, ARRAYSIZE(cls))) return;
  _wcslwr_s(cls);  // "Edit", "RichEdit50W", "WindowsForms10.EDIT.app...", "TEdit"
  if (!wcsstr(cls, L"edit")) return;
  DWORD start = 0, end = 0;
  SendMessageW(focus, EM_GETSEL, reinterpret_cast<WPARAM>(&start),
               reinterpret_cast<LPARAM>(&end));
  if (end > start) SendMessageW(focus, WM_CLEAR, 0, 0);
}

bool reinjectKey(WPARAM vk) {
  const bool extended = isNavigationKey(vk);  // nav keys are E0-extended
  INPUT inputs[2] = {};
  for (INPUT& input : inputs) {
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = static_cast<WORD>(vk);
    input.ki.wScan = static_cast<WORD>(
        MapVirtualKeyW(static_cast<UINT>(vk), MAPVK_VK_TO_VSC));
    if (extended) input.ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
    input.ki.dwExtraInfo = kGoxviInjectedKeyTag;
  }
  inputs[1].ki.dwFlags |= KEYEVENTF_KEYUP;
  // Partial injection (down without up) is as good as none — treat < 2 as
  // failure so the caller lets the original key through instead.
  return SendInput(2, inputs, sizeof(INPUT)) == 2;
}

bool isPasswordContext(ITfContext* context, TfClientId clientId) {
  bool password = false;
  bool inspected = false;
  // No async fallback: the lambda captures locals by reference and the result
  // is read right below — a late async run would write to a dead stack frame.
  runEditSession(context, clientId, TF_ES_SYNC | TF_ES_READ,
                 [&](TfEditCookie ec) {
                   inspected = true;
                   ComPtr<ITfReadOnlyProperty> property;
                   if (FAILED(context->GetAppProperty(GUID_PROP_INPUTSCOPE,
                                                      &property))) {
                     return S_OK;
                   }
                   TF_SELECTION selection = {};
                   ULONG fetched = 0;
                   if (FAILED(context->GetSelection(ec, TF_DEFAULT_SELECTION, 1,
                                                    &selection, &fetched)) ||
                       fetched == 0) {
                     return S_OK;
                   }
                   VARIANT var;
                   VariantInit(&var);
                   if (SUCCEEDED(property->GetValue(ec, selection.range, &var))) {
                     if (var.vt == VT_UNKNOWN && var.punkVal) {
                       ComPtr<ITfInputScope> scope;
                       if (SUCCEEDED(var.punkVal->QueryInterface(
                               IID_PPV_ARGS(&scope)))) {
                         InputScope* scopes = nullptr;
                         UINT count = 0;
                         if (SUCCEEDED(scope->GetInputScopes(&scopes, &count))) {
                           for (UINT i = 0; i < count; ++i) {
                             if (scopes[i] == IS_PASSWORD) password = true;
                           }
                           CoTaskMemFree(scopes);
                         }
                       }
                     }
                     VariantClear(&var);
                   }
                   selection.range->Release();
                   return S_OK;
                 },
                 /*allowAsyncFallback=*/false);
  // Sync read denied → we could not rule out a password field. Fail safe:
  // treat it as one (letters pass through raw; typing still works).
  return password || !inspected;
}

}  // namespace goxvi_keys
