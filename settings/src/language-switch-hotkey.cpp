#include "language-switch-hotkey.h"

#include <windows.h>

namespace goxvi::settings {
namespace {

// HKCU\Keyboard Layout\Toggle — bộ ba giá trị REG_SZ mà hộp thoại "Advanced Key
// Settings" của Windows ghi khi user chọn Ctrl+Shift cho "Switch Input Language":
//   Language Hotkey = 2  (2 = Ctrl+Shift, 1 = Alt+Shift, 3 = none, 4 = grave `)
//   Hotkey          = 2  (bản legacy, Windows giữ đồng bộ với Language Hotkey)
//   Layout Hotkey   = 3  (hotkey đổi layout TRONG một ngôn ngữ: tắt để không
//                         tranh Ctrl+Shift với hotkey đổi ngôn ngữ ở trên)
bool writeToggleValue(HKEY key, const wchar_t* name, const wchar_t* value) {
  return RegSetValueExW(key, name, 0, REG_SZ,
                        reinterpret_cast<const BYTE*>(value),
                        static_cast<DWORD>((wcslen(value) + 1) *
                                           sizeof(wchar_t))) == ERROR_SUCCESS;
}

}  // namespace

bool setCtrlShiftLanguageSwitchHotkey() {
  HKEY key = nullptr;
  if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Keyboard Layout\\Toggle", 0, nullptr,
                      REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, nullptr, &key,
                      nullptr) != ERROR_SUCCESS) {
    return false;
  }
  const bool ok = writeToggleValue(key, L"Language Hotkey", L"2") &&
                  writeToggleValue(key, L"Hotkey", L"2") &&
                  writeToggleValue(key, L"Layout Hotkey", L"3");
  RegCloseKey(key);
  // Bắt hệ thống đọc lại registry hotkey ngay — không cần sign out/in.
  if (ok) SystemParametersInfoW(SPI_SETLANGTOGGLE, 0, nullptr, 0);
  return ok;
}

}  // namespace goxvi::settings
