#include "default-input-override.h"

#include <windows.h>

#include "activate-tip-log.h"
#include "run-hidden-powershell.h"

namespace goxvi::settings {
namespace {

constexpr wchar_t kUserProfileKey[] =
    L"Control Panel\\International\\User Profile";
constexpr wchar_t kOverrideValue[] = L"InputMethodOverride";

}  // namespace

std::wstring readDefaultInputMethodOverride() {
  wchar_t buf[160];
  DWORD size = sizeof(buf);
  if (RegGetValueW(HKEY_CURRENT_USER, kUserProfileKey, kOverrideValue,
                   RRF_RT_REG_SZ, nullptr, buf, &size) != ERROR_SUCCESS) {
    return {};
  }
  return buf;
}

bool ensureDefaultInputMethodOverride(const std::wstring& tip) {
  if (_wcsicmp(readDefaultInputMethodOverride().c_str(), tip.c_str()) == 0) {
    activateTipLog(L"InputMethodOverride da la Goxvi (skip set)");
    return true;
  }
  // try/catch -ErrorAction Stop: cmdlet lỗi (policy chặn, tip không hợp lệ...)
  // thì exit 1 thay vì exit 0 giả — verify registry bên dưới vẫn là chốt chặn
  // cuối nên exit code ở đây chỉ phục vụ log.
  runHiddenPowerShell(
      L"try { Set-WinDefaultInputMethodOverride -InputTip '" + tip +
          L"' -ErrorAction Stop; exit 0 } catch { exit 1 }",
      30000, L"Set-WinDefaultInputMethodOverride");
  const bool ok =
      _wcsicmp(readDefaultInputMethodOverride().c_str(), tip.c_str()) == 0;
  activateTipLog(L"verify InputMethodOverride -> %ls",
                 ok ? L"GOXVI" : L"KHAC/RONG");
  return ok;
}

void clearDefaultInputMethodOverrideIfGoxvi(const std::wstring& tip) {
  if (_wcsicmp(readDefaultInputMethodOverride().c_str(), tip.c_str()) != 0) {
    return;  // override của người khác (hoặc không có) — không đụng
  }
  HKEY key = nullptr;
  if (RegOpenKeyExW(HKEY_CURRENT_USER, kUserProfileKey, 0, KEY_SET_VALUE,
                    &key) != ERROR_SUCCESS) {
    return;
  }
  const LSTATUS st = RegDeleteValueW(key, kOverrideValue);
  RegCloseKey(key);
  activateTipLog(L"clear InputMethodOverride (uninstall) -> st=%ld",
                 static_cast<long>(st));
}

}  // namespace goxvi::settings
