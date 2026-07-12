#include "tabs.h"

#include <windows.h>

#include "settings-version.h"

namespace goxvi::settings::ui {

namespace {

constexpr wchar_t kClass[] = L"GoxviAboutTab";

struct State {
  PageContext ctx;
  HFONT titleFont = nullptr;
  HICON logo = nullptr;
};

State* stateOf(HWND h) {
  return reinterpret_cast<State*>(GetWindowLongPtrW(h, GWLP_USERDATA));
}

// Full about copy in one read-only, scrollable text block (keeps the tab a
// single control instead of a dozen fiddly statics).
constexpr wchar_t kBody[] =
    L"Goxvi là bộ gõ tiếng Việt cho Windows 11, xây dựng theo chuẩn Text "
    L"Services Framework (TSF). Bộ gõ chạy trực tiếp bên trong tiến trình của "
    L"ứng dụng đang nhập liệu, nên hoạt động với cả ứng dụng cũ (Win32), ứng "
    L"dụng nền web (Electron) và ứng dụng Windows Store (UWP).\r\n\r\n"
    L"Chuyển giữa tiếng Việt và tiếng Anh bằng phím Windows + Space, giống như "
    L"đổi bàn phím của Windows. Bộ gõ không xử lý phím trong ô mật khẩu.\r\n\r\n"
    L"TÍNH NĂNG\r\n"
    L"• Kiểu gõ Telex và VNI. Telex dùng chữ cái để bỏ dấu (aa → â, as → á); "
    L"VNI dùng phím số (a6 → â, a1 → á). Telex ở đây là Telex đơn giản: w đứng "
    L"một mình không tự thành ư, chỉ aw, ow, uw, uow mới tạo ă/ơ/ư.\r\n"
    L"• Hai kiểu đặt dấu thanh: kiểu cũ đặt ở nguyên âm đầu (òa, úy), kiểu mới "
    L"đặt theo âm chính (oà, uý).\r\n"
    L"• Gõ tắt: gõ một chuỗi viết tắt rồi kết thúc từ, bộ gõ thay bằng nội dung "
    L"đầy đủ đã khai báo (vd: trc → trước). Viết tắt chỉ nhận chữ thường a–z.\r\n"
    L"• Huỷ dấu bằng phím Esc: đang gõ dở một từ, nhấn Esc để trả về đúng chuỗi "
    L"phím đã gõ, không bỏ dấu.\r\n\r\n"
    L"CẤU HÌNH VÀ DỮ LIỆU\r\n"
    L"Toàn bộ cấu hình lưu trong tệp %APPDATA%\\Goxvi\\config.json. Thay đổi có "
    L"hiệu lực trong khoảng 2 giây, áp dụng từ từ tiếp theo bạn gõ — không cần "
    L"khởi động lại bộ gõ hay ứng dụng.\r\n\r\n"
    L"Bộ gõ hoạt động hoàn toàn ngoại tuyến. Không có nội dung gõ hay cấu hình "
    L"nào được gửi ra ngoài máy.";

void buildControls(HWND page, State* s) {
  const PageContext& c = s->ctx;

  // Bề rộng/chiều cao co theo page (form hẹp + thấp): tiêu đề/phiên bản/ô nội dung
  // fill ngang, ô nội dung fill tới đáy.
  RECT prc;
  GetClientRect(page, &prc);
  const int pageW = MulDiv(prc.right, 96, c.dpi);
  const int pageH = MulDiv(prc.bottom, 96, c.dpi);

  // Header: a bigger app logo (embedded icon, resource id 1) beside the name
  // "Goxvi" (large) over a smaller version line.
  const int logoPx = scale(56, c.dpi);
  s->logo = static_cast<HICON>(LoadImageW(GetModuleHandleW(nullptr),
                                          MAKEINTRESOURCEW(1), IMAGE_ICON,
                                          logoPx, logoPx, LR_DEFAULTCOLOR));
  HWND icon = child(page, L"STATIC", L"", SS_ICON, 14, 10, 56, 56, -1, c);
  SendMessageW(icon, STM_SETICON, reinterpret_cast<WPARAM>(s->logo), 0);

  s->titleFont = CreateFontW(-MulDiv(18, c.dpi, 72), 0, 0, 0, FW_SEMIBOLD, FALSE,
                             FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                             DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

  HWND name = child(page, L"STATIC", L"Goxvi", SS_LEFT, 82, 12, pageW - 90, 30,
                    -1, c);
  SendMessageW(name, WM_SETFONT, reinterpret_cast<WPARAM>(s->titleFont), TRUE);

  // Version line uses the default (smaller) UI font applied by child().
  child(page, L"STATIC", L"Phiên bản " L"" GOXVI_VERSION_STR, SS_LEFT, 84, 44,
        pageW - 92, 18, -1, c);

  const int bodyTop = 80, pad = 8;
  child(page, L"EDIT", kBody,
        ES_MULTILINE | ES_READONLY | WS_VSCROLL | ES_AUTOVSCROLL, 14, bodyTop,
        pageW - 22, pageH - bodyTop - pad, -1, c);
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
    case WM_CTLCOLORSTATIC:
      // Windows default dialog colour (COLOR_3DFACE) - no white. The read-only
      // body edit blends as flowing text (no textbox look).
      SetBkMode(reinterpret_cast<HDC>(w), TRANSPARENT);
      return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_3DFACE));
    case WM_NCDESTROY: {
      if (State* s = stateOf(h)) {
        if (s->titleFont) DeleteObject(s->titleFont);
        if (s->logo) DestroyIcon(s->logo);
        delete s;
      }
      SetWindowLongPtrW(h, GWLP_USERDATA, 0);
      return 0;
    }
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

HWND createAboutTab(HWND parent, const PageContext& ctx, RECT area) {
  ensureClass();
  PageContext local = ctx;
  return CreateWindowExW(0, kClass, nullptr, WS_CHILD | WS_CLIPCHILDREN,
                         area.left, area.top, area.right - area.left,
                         area.bottom - area.top, parent, nullptr,
                         GetModuleHandleW(nullptr), &local);
}

}  // namespace goxvi::settings::ui
