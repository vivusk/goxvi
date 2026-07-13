#include "settings-window.h"

#include <windows.h>

#include <commctrl.h>

#include "tabs.h"
#include "ui-common.h"

namespace goxvi::settings {

namespace {

using ui::PageContext;

constexpr const wchar_t* kClass = kMainWindowClass;  // shared with main.cpp
constexpr int kClientW = 228;  // logical (96-dpi); dáng đứng ~0.8 (W:H) cho thuận
                               // mắt (trước 290 gần vuông). Control các tab co ngang
                               // theo page width nên không vỡ khi hẹp lại.
constexpr int kClientH = 312;  // narrow portrait form; height khít nội dung tab
                               // Tổng quan (bottom ~252 sau khi group "Tính năng
                               // khác" thêm checkbox thứ 2). Đã bỏ nút Đóng + status
                               // line -> tab lấp gần hết cửa sổ (chỉ chừa lề 8px).
                               // Tab Bảng gõ tắt / Giới thiệu co theo (fill đáy).

enum : int { kTabCtrl = 100 };

struct State {
  ConfigStore store;
  HFONT font = nullptr;
  int dpi = 96;
  HWND tab = nullptr;
  HWND pages[3] = {};  // 0 overview, 1 shortcuts, 2 about
  int current = 0;
};

State* stateOf(HWND h) {
  return reinterpret_cast<State*>(GetWindowLongPtrW(h, GWLP_USERDATA));
}

// Display rect (client coords) the tab pages sit in, derived from the tab
// control's own rect via TabCtrl_AdjustRect.
RECT pageArea(State* s) {
  RECT rc = {ui::scale(8, s->dpi), ui::scale(8, s->dpi),
             ui::scale(8 + (kClientW - 16), s->dpi),
             ui::scale(8 + (kClientH - 16), s->dpi)};
  TabCtrl_AdjustRect(s->tab, FALSE, &rc);
  return rc;
}

void addTab(HWND tab, int index, const wchar_t* text) {
  TCITEMW it = {};
  it.mask = TCIF_TEXT;
  it.pszText = const_cast<LPWSTR>(text);
  TabCtrl_InsertItem(tab, index, &it);
}

void selectPage(State* s, int index) {
  if (index < 0 || index > 2) return;
  ShowWindow(s->pages[s->current], SW_HIDE);
  s->current = index;
  ShowWindow(s->pages[index], SW_SHOW);
}

void build(HWND h, State* s) {
  s->dpi = GetDpiForWindow(h);
  s->font = ui::createUiFont(s->dpi);

  // Resize the (fixed) window to the DPI-correct client size now that dpi known.
  RECT want = {0, 0, ui::scale(kClientW, s->dpi), ui::scale(kClientH, s->dpi)};
  AdjustWindowRectExForDpi(&want, static_cast<DWORD>(GetWindowLongW(h, GWL_STYLE)),
                           FALSE, 0, static_cast<UINT>(s->dpi));
  SetWindowPos(h, nullptr, 0, 0, want.right - want.left, want.bottom - want.top,
               SWP_NOMOVE | SWP_NOZORDER);

  // Đã bỏ nút Đóng (đóng bằng nút X caption) + status line: tab lấp gần hết cửa
  // sổ, chỉ chừa lề 8px mọi phía.
  s->tab = ui::child(h, WC_TABCONTROLW, L"", WS_CLIPSIBLINGS | WS_TABSTOP, 8, 8,
                     kClientW - 16, kClientH - 16, kTabCtrl,
                     PageContext{&s->store, s->font, s->dpi, nullptr});
  addTab(s->tab, 0, L"Tổng quan");
  addTab(s->tab, 1, L"Bảng gõ tắt");
  addTab(s->tab, 2, L"Giới thiệu");

  // Tab pages share config + font; status=nullptr (không còn status line —
  // notifySaved/commit đã guard null nên bỏ qua an toàn).
  const PageContext ctx{&s->store, s->font, s->dpi, nullptr};
  const RECT area = pageArea(s);
  s->pages[0] = ui::createOverviewTab(h, ctx, area);
  s->pages[1] = ui::createShortcutsTab(h, ctx, area);
  s->pages[2] = ui::createAboutTab(h, ctx, area);
  ShowWindow(s->pages[0], SW_SHOW);

  // Cảnh báo lúc nạp config (parse lỗi...) hiếm — không còn status line nên báo
  // qua MessageBox để KHÔNG nuốt lỗi.
  if (!s->store.startupWarning().empty())
    MessageBoxW(h, s->store.startupWarning().c_str(), L"Goxvi",
                MB_OK | MB_ICONWARNING);
}

LRESULT CALLBACK wndProc(HWND h, UINT msg, WPARAM w, LPARAM l) {
  State* s = stateOf(h);
  switch (msg) {
    case WM_CREATE: {
      s = new State{};
      SetWindowLongPtrW(h, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(s));
      build(h, s);
      return 0;
    }
    case WM_NOTIFY: {
      auto* nm = reinterpret_cast<NMHDR*>(l);
      if (s && nm->idFrom == kTabCtrl && nm->code == TCN_SELCHANGE)
        selectPage(s, TabCtrl_GetCurSel(s->tab));
      return 0;
    }
    case WM_DESTROY:
      if (s) { if (s->font) DeleteObject(s->font); delete s; }
      SetWindowLongPtrW(h, GWLP_USERDATA, 0);
      PostQuitMessage(0);
      return 0;
  }
  return DefWindowProcW(h, msg, w, l);
}

void ensureClass() {
  WNDCLASSW wc = {};
  wc.lpfnWndProc = wndProc;
  wc.hInstance = GetModuleHandleW(nullptr);
  wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1);
  wc.lpszClassName = kClass;
  wc.hIcon = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(1));
  RegisterClassW(&wc);
}

}  // namespace

HWND createSettingsWindow() {
  ensureClass();
  // Fixed-size: caption + sysmenu + minimize, but NO thick frame / maximize.
  const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
  HWND h = CreateWindowExW(0, kClass, L"Goxvi", style, CW_USEDEFAULT,
                           CW_USEDEFAULT, 100, 100, nullptr, nullptr,
                           GetModuleHandleW(nullptr), nullptr);
  if (!h) return nullptr;

  // Center on the work area of the monitor the window landed on.
  RECT wr = {};
  GetWindowRect(h, &wr);
  HMONITOR mon = MonitorFromWindow(h, MONITOR_DEFAULTTONEAREST);
  MONITORINFO mi = {sizeof(mi)};
  GetMonitorInfoW(mon, &mi);
  const int ww = wr.right - wr.left, wh = wr.bottom - wr.top;
  const int x = mi.rcWork.left + (mi.rcWork.right - mi.rcWork.left - ww) / 2;
  const int y = mi.rcWork.top + (mi.rcWork.bottom - mi.rcWork.top - wh) / 2;
  SetWindowPos(h, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

  ShowWindow(h, SW_SHOW);
  UpdateWindow(h);
  return h;
}

}  // namespace goxvi::settings
