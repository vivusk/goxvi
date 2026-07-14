#include <windows.h>

#include <commctrl.h>

#include <cwchar>

#include "activate-tip.h"
#include "language-switch-hotkey.h"
#include "settings-window.h"

// Entry point for Goxvi.exe — the native (no .NET) settings app. Single-instance
// (a second launch focuses the existing window), then a plain Win32 message loop
// with IsDialogMessage so Tab/arrow keyboard navigation works across the tabs.

namespace {

constexpr wchar_t kMutexName[] = L"GoxviSettings.SingleInstance";

void focusExistingWindow() {
  // Match by our window class, not the title — any window could be named
  // "Goxvi".
  HWND existing = FindWindowW(goxvi::settings::kMainWindowClass, nullptr);
  if (existing) {
    if (IsIconic(existing)) ShowWindow(existing, SW_RESTORE);
    SetForegroundWindow(existing);
  }
}

}  // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR pCmdLine, int) {
  // Chế độ helper không cửa sổ: installer gọi `Goxvi.exe --activate-tip` (vô
  // điều kiện ở ssPostInstall, ngữ cảnh user, có chờ) để thêm Goxvi vào
  // language list + đặt mặc định + kích hoạt ngay sau khi cài; uninstaller gọi
  // `--deactivate-tip` để gỡ khỏi list. Xử lý TRƯỚC mutex/cửa sổ rồi thoát
  // luôn. Exit 0 = đã ghi BỀN vào language list (installer đọc code này để
  // cảnh báo); chi tiết từng bước ở %APPDATA%\Goxvi\activate-tip.log.
  if (pCmdLine && wcsstr(pCmdLine, L"--activate-tip")) {
    const bool activated = goxvi::settings::activateGoxviTip();
    // Kèm hotkey Ctrl+Shift chuyển ENG<->VIE (thói quen UniKey) — best-effort,
    // không tính vào kết quả cài kiểu gõ.
    goxvi::settings::setCtrlShiftLanguageSwitchHotkey();
    return activated ? 0 : 1;
  }
  if (pCmdLine && wcsstr(pCmdLine, L"--deactivate-tip")) {
    return goxvi::settings::deactivateGoxviTip() ? 0 : 1;
  }

  // Single instance: a second launch just surfaces the running window.
  HANDLE mutex = CreateMutexW(nullptr, TRUE, kMutexName);
  if (mutex && GetLastError() == ERROR_ALREADY_EXISTS) {
    focusExistingWindow();
    return 0;
  }

  INITCOMMONCONTROLSEX icc = {sizeof(icc),
                              ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES |
                                  ICC_STANDARD_CLASSES};
  InitCommonControlsEx(&icc);

  HWND window = goxvi::settings::createSettingsWindow();
  if (!window) return 1;

  MSG msg;
  while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
    if (!IsDialogMessageW(window, &msg)) {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
  }

  if (mutex) CloseHandle(mutex);
  return static_cast<int>(msg.wParam);
}
