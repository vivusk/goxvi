#include "tabs.h"

#include <windows.h>

#include <uxtheme.h>

#include "goxvi/engine-types.h"

namespace goxvi::settings::ui {

namespace {

constexpr wchar_t kClass[] = L"GoxviOverviewTab";

enum : int {
  kTelex = 1001,
  kVni = 1002,
  kToneOld = 1003,
  kToneNew = 1004,
  kShortcuts = 1005,
};

// Per-instance state; lifetime tied to the page window (freed on WM_NCDESTROY).
struct State {
  PageContext ctx;
};

State* stateOf(HWND h) {
  return reinterpret_cast<State*>(GetWindowLongPtrW(h, GWLP_USERDATA));
}

// A group box with its visual style stripped (SetWindowTheme "",""): the themed
// group box otherwise paints a faint #F9F9F9 interior fill that makes the white
// radios inside look boxed. Non-themed, its interior is transparent so radios,
// labels and the group all sit on the same white page. Frame = light etched line.
void groupBox(HWND page, const wchar_t* title, int x, int y, int w, int h,
              const PageContext& c) {
  HWND gb = child(page, L"BUTTON", title, BS_GROUPBOX, x, y, w, h, -1, c);
  SetWindowTheme(gb, L"", L"");
}

void buildControls(HWND page, State* s) {
  const PageContext& c = s->ctx;
  const goxvi::EngineConfig& cfg = c.store->config();

  // Narrow portrait form: options as radio pairs stacked VERTICALLY (one choice
  // per line) inside each group box. Bề rộng co theo page (form hẹp lại cho tỉ lệ
  // dáng đứng): group box fill ngang (lề 8px), radio/checkbox inset trong đó.
  RECT prc;
  GetClientRect(page, &prc);
  const int pageW = MulDiv(prc.right, 96, c.dpi);
  const int gbW = pageW - 16;   // group box: lề 8px hai bên
  const int rbW = pageW - 44;   // radio/checkbox: inset trong group box

  // --- Kiểu gõ (Telex | VNI) ---
  groupBox(page, L"Kiểu gõ", 8, 8, gbW, 76, c);
  child(page, L"BUTTON", L"Telex", BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP,
        22, 28, rbW, 22, kTelex, c);
  child(page, L"BUTTON", L"VNI", BS_AUTORADIOBUTTON, 22, 54, rbW, 22, kVni, c);

  // --- Kiểu bỏ dấu (cũ | mới) ---
  groupBox(page, L"Kiểu bỏ dấu", 8, 92, gbW, 76, c);
  child(page, L"BUTTON", L"Kiểu cũ  (hòa, thúy)",
        BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP, 22, 112, rbW, 22, kToneOld,
        c);
  child(page, L"BUTTON", L"Kiểu mới  (hoà, thuý)", BS_AUTORADIOBUTTON, 22, 138,
        rbW, 22, kToneNew, c);

  // --- Gõ tắt (master switch; bảng gõ tắt ở tab kế) ---
  groupBox(page, L"Gõ tắt", 8, 176, gbW, 48, c);
  child(page, L"BUTTON", L"Bật gõ tắt",
        BS_AUTOCHECKBOX | WS_GROUP | WS_TABSTOP, 22, 196, rbW, 22, kShortcuts,
        c);

  // Initial states from the loaded config.
  CheckDlgButton(page, kVni,
                 cfg.inputMethod == goxvi::InputMethod::Vni ? BST_CHECKED
                                                            : BST_UNCHECKED);
  CheckDlgButton(page, kTelex,
                 cfg.inputMethod == goxvi::InputMethod::Vni ? BST_UNCHECKED
                                                            : BST_CHECKED);
  CheckDlgButton(page, kToneNew,
                 cfg.toneStyle == goxvi::ToneStyle::New ? BST_CHECKED
                                                        : BST_UNCHECKED);
  CheckDlgButton(page, kToneOld,
                 cfg.toneStyle == goxvi::ToneStyle::New ? BST_UNCHECKED
                                                        : BST_CHECKED);
  CheckDlgButton(page, kShortcuts,
                 cfg.shortcutsEnabled ? BST_CHECKED : BST_UNCHECKED);
}

void onCommand(HWND page, State* s, int id, int code) {
  if (code != BN_CLICKED) return;
  goxvi::EngineConfig& cfg = s->ctx.store->config();
  switch (id) {
    case kTelex:
      cfg.inputMethod = goxvi::InputMethod::Telex;
      break;
    case kVni:
      cfg.inputMethod = goxvi::InputMethod::Vni;
      break;
    case kToneOld:
      cfg.toneStyle = goxvi::ToneStyle::Old;
      break;
    case kToneNew:
      cfg.toneStyle = goxvi::ToneStyle::New;
      break;
    case kShortcuts:
      cfg.shortcutsEnabled = IsDlgButtonChecked(page, kShortcuts) == BST_CHECKED;
      break;
    default:
      return;
  }
  s->ctx.store->save();
  notifySaved(s->ctx);
}

LRESULT CALLBACK wndProc(HWND h, UINT msg, WPARAM w, LPARAM l) {
  switch (msg) {
    case WM_CREATE: {
      auto* cs = reinterpret_cast<CREATESTRUCTW*>(l);
      auto* s = new State{*static_cast<PageContext*>(cs->lpCreateParams)};
      SetWindowLongPtrW(h, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(s));
      buildControls(h, s);
      return 0;
    }
    case WM_COMMAND:
      if (State* s = stateOf(h))
        onCommand(h, s, LOWORD(w), HIWORD(w));
      return 0;
    case WM_CTLCOLORSTATIC:
      // Windows default dialog colour (COLOR_3DFACE, #F0F0F0) everywhere - no
      // white. Radios/checkboxes paint this colour natively; statics + the
      // (de-themed) group boxes match it, so the whole page is one flat colour.
      SetBkMode(reinterpret_cast<HDC>(w), TRANSPARENT);
      return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_3DFACE));
    case WM_NCDESTROY:
      delete stateOf(h);
      SetWindowLongPtrW(h, GWLP_USERDATA, 0);
      return 0;
  }
  return DefWindowProcW(h, msg, w, l);
}

void ensureClass() {
  static bool registered = false;
  if (registered) return;
  WNDCLASSW wc = {};
  wc.lpfnWndProc = wndProc;
  wc.hInstance = GetModuleHandleW(nullptr);
  wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1);
  wc.lpszClassName = kClass;
  RegisterClassW(&wc);
  registered = true;
}

}  // namespace

HWND createOverviewTab(HWND parent, const PageContext& ctx, RECT area) {
  ensureClass();
  PageContext local = ctx;
  // No WS_CLIPCHILDREN: the page must paint its COLOR_3DFACE background UNDER the
  // (de-themed, transparent-interior) group boxes too, otherwise their interior
  // reveals the lighter tab-control body (#F9F9F9) instead of the page colour.
  return CreateWindowExW(0, kClass, nullptr, WS_CHILD,
                         area.left, area.top, area.right - area.left,
                         area.bottom - area.top, parent, nullptr,
                         GetModuleHandleW(nullptr), &local);
}

}  // namespace goxvi::settings::ui
