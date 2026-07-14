#include "activate-tip-log.h"

#include <windows.h>

#include <shlobj.h>

#include <cstdarg>
#include <cstdio>
#include <string>

namespace goxvi::settings {
namespace {

// Path rỗng = logging tắt (không xác định được %APPDATA% hoặc mở file fail).
std::wstring& logPath() {
  static std::wstring path;
  return path;
}

// Mở/đóng theo từng dòng (log chỉ ~15 dòng/lần chạy): không giữ handle sống
// suốt vòng đời helper, không cần lo thứ tự đóng khi exit sớm. _SH_DENYNO để
// user mở xem song song khi cần.
FILE* openLog(const wchar_t* mode) {
  if (logPath().empty()) return nullptr;
  return _wfsopen(logPath().c_str(), mode, _SH_DENYNO);
}

}  // namespace

void activateTipLogBegin(const wchar_t* mode) {
  PWSTR appData = nullptr;
  if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr,
                                  &appData))) {
    return;
  }
  std::wstring dir(appData);
  CoTaskMemFree(appData);
  dir += L"\\Goxvi";
  CreateDirectoryW(dir.c_str(), nullptr);  // no-op nếu đã tồn tại
  logPath() = dir + L"\\activate-tip.log";

  FILE* f = openLog(L"w, ccs=UTF-8");  // tạo mới mỗi lần chạy
  if (!f) {
    logPath().clear();
    return;
  }
  SYSTEMTIME st;
  GetLocalTime(&st);
  fwprintf(f, L"[%04u-%02u-%02u %02u:%02u:%02u] Goxvi helper %ls\n", st.wYear,
           st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, mode);
  fclose(f);
}

void activateTipLog(const wchar_t* format, ...) {
  FILE* f = openLog(L"a, ccs=UTF-8");
  if (!f) return;
  va_list args;
  va_start(args, format);
  vfwprintf(f, format, args);
  va_end(args);
  fputwc(L'\n', f);
  fclose(f);
}

}  // namespace goxvi::settings
