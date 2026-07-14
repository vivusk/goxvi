#pragma once

#include <string>

namespace goxvi::settings {

// Chạy powershell.exe (System32\WindowsPowerShell\v1.0 — PS 5.1 có sẵn mọi
// Windows) ẩn cửa sổ, chờ tối đa timeoutMs, trả true khi exit code 0. Dùng cho
// các bước cần cmdlet chính chủ không có Win32 API tương đương
// (Set-WinUserLanguageList, Set-WinDefaultInputMethodOverride). Kết quả + exit
// code ghi vào activate-tip.log dưới nhãn logTag. psCommand chỉ được dùng nháy
// ĐƠN bên trong (chuỗi bọc bằng nháy kép khi truyền qua -Command).
// Timeout thì KHÔNG terminate (tránh chém powershell giữa lúc ghi language
// list) — chỉ log rồi trả false.
bool runHiddenPowerShell(const std::wstring& psCommand, unsigned timeoutMs,
                         const wchar_t* logTag);

}  // namespace goxvi::settings
