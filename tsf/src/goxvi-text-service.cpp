#include "goxvi-text-service.h"

#include <optional>

#include <imm.h>
#include <string.h>  // _wcsicmp

#include "debug-logging.h"
#include "dll-module.h"
#include "goxvi-guids.h"
#include "goxvi/shortcut-expander.h"
#include "mouse-click-commit-hook.h"

using Microsoft::WRL::ComPtr;

namespace {

// Browser address bars (Chrome omnibox, Edge, ...) paint their OWN autocomplete
// popup and, on a mouse-down, hit-test that popup against the frame the user is
// looking at, then terminate the composition themselves. They are TSF-native,
// so OnCompositionTerminated keeps/replaces the word. If the mouse hook
// pre-commits the composing word first, the popup reflows (autocomplete re-runs
// on the committed text) a frame before the reinjected click is hit-tested, so
// the click lands on a shifted row — the classic "type a, click a suggestion,
// wrong one opens". Detecting the host by its exe (the TIP is in-process, so
// GetModuleFileName(NULL) is the host) lets us leave those clicks untouched.
bool hostHandlesClickTermination() {
  wchar_t path[MAX_PATH] = {};
  const DWORD n = GetModuleFileNameW(nullptr, path, MAX_PATH);
  if (n == 0 || n >= MAX_PATH) return false;
  const wchar_t* exe = wcsrchr(path, L'\\');
  exe = exe ? exe + 1 : path;
  static const wchar_t* const kBrowsers[] = {
      L"chrome.exe",   L"msedge.exe", L"brave.exe",   L"vivaldi.exe",
      L"opera.exe",    L"chromium.exe", L"arc.exe",   L"firefox.exe",
  };
  for (const wchar_t* b : kBrowsers) {
    if (_wcsicmp(exe, b) == 0) return true;
  }
  return false;
}

}  // namespace

GoxviTextService::GoxviTextService() { DllAddRef(); }

GoxviTextService::~GoxviTextService() { DllRelease(); }

// ---- IUnknown --------------------------------------------------------------

STDMETHODIMP GoxviTextService::QueryInterface(REFIID riid, void** ppv) {
  if (!ppv) return E_INVALIDARG;
  *ppv = nullptr;
  if (riid == IID_IUnknown || riid == IID_ITfTextInputProcessor) {
    *ppv = static_cast<ITfTextInputProcessor*>(this);
  } else if (riid == IID_ITfTextInputProcessorEx) {
    *ppv = static_cast<ITfTextInputProcessorEx*>(this);
  } else if (riid == IID_ITfKeyEventSink) {
    *ppv = static_cast<ITfKeyEventSink*>(this);
  } else if (riid == IID_ITfCompositionSink) {
    *ppv = static_cast<ITfCompositionSink*>(this);
  } else if (riid == IID_ITfDisplayAttributeProvider) {
    *ppv = static_cast<ITfDisplayAttributeProvider*>(this);
  } else if (riid == IID_ITfThreadMgrEventSink) {
    *ppv = static_cast<ITfThreadMgrEventSink*>(this);
  } else if (riid == IID_ITfTextEditSink) {
    *ppv = static_cast<ITfTextEditSink*>(this);
  } else {
    return E_NOINTERFACE;
  }
  AddRef();
  return S_OK;
}

STDMETHODIMP_(ULONG) GoxviTextService::AddRef() {
  return InterlockedIncrement(&refCount_);
}

STDMETHODIMP_(ULONG) GoxviTextService::Release() {
  const LONG count = InterlockedDecrement(&refCount_);
  if (count == 0) delete this;
  return count;
}

// ---- ITfTextInputProcessor(Ex) ----------------------------------------------

STDMETHODIMP GoxviTextService::Activate(ITfThreadMgr* threadMgr,
                                        TfClientId clientId) {
  return ActivateEx(threadMgr, clientId, 0);
}

STDMETHODIMP GoxviTextService::ActivateEx(ITfThreadMgr* threadMgr,
                                          TfClientId clientId,
                                          [[maybe_unused]] DWORD flags) {
  if (!threadMgr) return E_INVALIDARG;
  GOXVI_LOG(L"ActivateEx flags=0x%08X", flags);
  threadMgr_ = threadMgr;
  clientId_ = clientId;
  hostHandlesClickTermination_ = hostHandlesClickTermination();
  GOXVI_LOG(L"hostHandlesClickTermination=%d", hostHandlesClickTermination_ ? 1 : 0);

  registerDisplayAttributeGuid();
  composition_.initialize(static_cast<ITfCompositionSink*>(this),
                          displayAttributeAtom_);

  if (FAILED(adviseKeyEventSink())) {
    Deactivate();
    return E_FAIL;
  }
  adviseThreadMgrSink();  // best-effort: focus-loss commit
  ComPtr<ITfDocumentMgr> focusDocMgr;  // best-effort: caret-move commit
  if (SUCCEEDED(threadMgr_->GetFocus(&focusDocMgr))) {
    adviseTextEditSink(focusDocMgr.Get());
  }
  // Commit BEFORE the app sees a click: IMM/CUAS hosts (Telegram/Qt, Sublime,
  // edit controls) cancel the uncommitted composition string on mouse down —
  // TSF gives the TIP no notification it could act on early enough.
  goxvi_mouse::installClickCommitHook(
      [this] { return commitOnPointerDown(); });
  return S_OK;
}

STDMETHODIMP GoxviTextService::Deactivate() {
  GOXVI_LOG(L"Deactivate");
  goxvi_mouse::removeClickCommitHook();
  commitCurrentWord();
  unadviseTextEditSink();
  unadviseThreadMgrSink();
  unadviseKeyEventSink();
  threadMgr_.Reset();
  clientId_ = TF_CLIENTID_NULL;
  return S_OK;
}

// ---- sinks advise -----------------------------------------------------------

HRESULT GoxviTextService::adviseKeyEventSink() {
  ComPtr<ITfKeystrokeMgr> keystrokeMgr;
  HRESULT hr = threadMgr_.As(&keystrokeMgr);
  if (FAILED(hr)) return hr;
  hr = keystrokeMgr->AdviseKeyEventSink(clientId_,
                                        static_cast<ITfKeyEventSink*>(this),
                                        TRUE /*foreground*/);
  keySinkAdvised_ = SUCCEEDED(hr);
  return hr;
}

void GoxviTextService::unadviseKeyEventSink() {
  if (!keySinkAdvised_ || !threadMgr_) return;
  ComPtr<ITfKeystrokeMgr> keystrokeMgr;
  if (SUCCEEDED(threadMgr_.As(&keystrokeMgr))) {
    keystrokeMgr->UnadviseKeyEventSink(clientId_);
  }
  keySinkAdvised_ = false;
}

HRESULT GoxviTextService::adviseThreadMgrSink() {
  ComPtr<ITfSource> source;
  HRESULT hr = threadMgr_.As(&source);
  if (FAILED(hr)) return hr;
  return source->AdviseSink(IID_ITfThreadMgrEventSink,
                            static_cast<ITfThreadMgrEventSink*>(this),
                            &threadMgrSinkCookie_);
}

void GoxviTextService::unadviseThreadMgrSink() {
  if (threadMgrSinkCookie_ == TF_INVALID_COOKIE || !threadMgr_) return;
  ComPtr<ITfSource> source;
  if (SUCCEEDED(threadMgr_.As(&source))) {
    source->UnadviseSink(threadMgrSinkCookie_);
  }
  threadMgrSinkCookie_ = TF_INVALID_COOKIE;
}

void GoxviTextService::adviseTextEditSink(ITfDocumentMgr* docMgr) {
  unadviseTextEditSink();
  if (!docMgr) return;
  ComPtr<ITfContext> context;
  if (FAILED(docMgr->GetTop(&context)) || !context) return;
  ComPtr<ITfSource> source;
  if (FAILED(context.As(&source))) return;
  if (SUCCEEDED(source->AdviseSink(IID_ITfTextEditSink,
                                   static_cast<ITfTextEditSink*>(this),
                                   &textEditSinkCookie_))) {
    editSinkContext_ = context;
  } else {
    textEditSinkCookie_ = TF_INVALID_COOKIE;
  }
}

void GoxviTextService::unadviseTextEditSink() {
  if (textEditSinkCookie_ != TF_INVALID_COOKIE && editSinkContext_) {
    ComPtr<ITfSource> source;
    if (SUCCEEDED(editSinkContext_.As(&source))) {
      source->UnadviseSink(textEditSinkCookie_);
    }
  }
  textEditSinkCookie_ = TF_INVALID_COOKIE;
  editSinkContext_.Reset();
}

void GoxviTextService::registerDisplayAttributeGuid() {
  ComPtr<ITfCategoryMgr> categoryMgr;
  if (SUCCEEDED(CoCreateInstance(CLSID_TF_CategoryMgr, nullptr,
                                 CLSCTX_INPROC_SERVER, IID_ITfCategoryMgr,
                                 &categoryMgr))) {
    categoryMgr->RegisterGUID(GUID_GoxviDisplayAttribute,
                              &displayAttributeAtom_);
  }
}

// ---- composition/engine glue --------------------------------------------------

void GoxviTextService::pollConfigAtWordStart() {
  goxvi::EngineConfig config;
  if (configWatcher_.pollConfig(config)) {
    config_ = config;
    engine_.setConfig(config);
  }
}

bool GoxviTextService::commitOnPointerDown() {
  if (!composition_.isComposing()) return false;
  // Browser address bars hit-test their autocomplete popup themselves, then
  // terminate the composition natively. Pre-committing here would reflow the
  // popup before the reinjected click lands → wrong suggestion. Let the real
  // click through; OnCompositionTerminated keeps the word (Decision 6b).
  if (hostHandlesClickTermination_) {
    GOXVI_LOG(L"pointer down while composing -> host drives click, pass through");
    return false;
  }
  GOXVI_LOG(L"pointer down while composing -> complete");
  // IMM/CUAS host: completing the IMM composition here (still before the
  // app's own click handling) delivers the word as the result string, so the
  // app's follow-up CPS_CANCEL finds nothing to discard.
  HWND focus = GetFocus();
  if (focus) {
    HIMC imc = ImmGetContext(focus);
    if (imc) {
      ImmNotifyIME(imc, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
      ImmReleaseContext(focus, imc);
    }
  }
  // TSF-native host (no IMM bridge / notify was a no-op): end it ourselves.
  // No shortcut expansion — a click is not a terminator (Decision 8).
  if (composition_.isComposing()) commitCurrentWord(/*applyShortcut=*/false);
  return true;
}

void GoxviTextService::commitCurrentWord(bool applyShortcut) {
  if (composition_.isComposing()) {
    // gõ tắt: only when enabled, not Esc, and the raw buffer is still an exact
    // mirror of what was typed (Literal-backspace drift → skip; M1/Decision 9).
    std::optional<std::wstring> expansion;
    if (applyShortcut && config_.shortcutsEnabled && engine_.rawKeysExact()) {
      expansion = goxvi::matchShortcut(engine_.rawTypedKeys(), config_.shortcuts);
    }
    if (expansion) {
      composition_.commitWithReplacement(clientId_, *expansion);
    } else {
      composition_.commit(clientId_);
    }
  }
  engine_.reset();
}

void GoxviTextService::processLetter(ITfContext* context, wchar_t ch) {
  goxvi::KeyResult result = engine_.processKey(ch);
  if (result.overflow) {  // R2-M1: commit as-is, restart with this key
    commitCurrentWord();
    result = engine_.processKey(ch);
  }
  if (!result.consumed) return;
  composition_.updateText(context, clientId_, result.display);
}

void GoxviTextService::cancelWordToRaw(ITfContext* context) {
  const std::wstring raw = engine_.rawTypedKeys();
  if (!raw.empty()) composition_.updateText(context, clientId_, raw);
  commitCurrentWord(/*applyShortcut=*/false);  // Esc: raw wins, never expand (C1)
}

void GoxviTextService::processBackspace(ITfContext* context) {
  std::wstring display;
  if (!engine_.processBackspace(display)) return;
  if (display.empty()) {
    composition_.clearAndEnd(clientId_);
  } else {
    composition_.updateText(context, clientId_, display);
  }
}

// ---- ITfCompositionSink -------------------------------------------------------

STDMETHODIMP GoxviTextService::OnCompositionTerminated(
    TfEditCookie ecWrite, ITfComposition* composition) {
  // Caret moved / click / app decision. Hosts that delete the shadow text get
  // it re-inserted via the write cookie; keep-style hosts (RichEdit...) are
  // already final. IMM/CUAS hosts that cancel on click never reach this with
  // an uncommitted word — the mouse hook completed it beforehand.
  composition_.finalizeExternalTermination(ecWrite, composition,
                                           engine_.currentDisplay());
  engine_.reset();
  return S_OK;
}

// ---- ITfThreadMgrEventSink ----------------------------------------------------

STDMETHODIMP GoxviTextService::OnInitDocumentMgr(ITfDocumentMgr*) { return S_OK; }

STDMETHODIMP GoxviTextService::OnUninitDocumentMgr(ITfDocumentMgr*) { return S_OK; }

STDMETHODIMP GoxviTextService::OnSetFocus(ITfDocumentMgr* focus,
                                          ITfDocumentMgr* /*prevFocus*/) {
  commitCurrentWord();  // finalize the word left behind in the old view
  adviseTextEditSink(focus);
  return S_OK;
}

STDMETHODIMP GoxviTextService::OnPushContext(ITfContext*) { return S_OK; }

STDMETHODIMP GoxviTextService::OnPopContext(ITfContext*) { return S_OK; }

// ---- ITfTextEditSink ------------------------------------------------------------

STDMETHODIMP GoxviTextService::OnEndEdit(ITfContext* context,
                                         TfEditCookie ecReadOnly,
                                         ITfEditRecord* editRecord) {
  if (!composition_.isComposing() || !editRecord || !context) return S_OK;
  BOOL selectionChanged = FALSE;
  if (FAILED(editRecord->GetSelectionStatus(&selectionChanged)) ||
      !selectionChanged) {
    return S_OK;
  }
  // Our own edit sessions always end with the caret at the end of the word →
  // still covered → skipped. A click/caret move OUTSIDE the word means hosts
  // that never fire OnCompositionTerminated (CUAS edit controls) are about to
  // silently drop it: finalize as displayed. No shortcut expansion — the user
  // did not type a terminator (mirrors OnCompositionTerminated semantics).
  if (composition_.selectionCoveredByComposition(ecReadOnly, context)) {
    return S_OK;
  }
  GOXVI_LOG(L"OnEndEdit: caret left the composition -> commit");
  commitCurrentWord(/*applyShortcut=*/false);
  return S_OK;
}
