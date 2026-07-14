#include "run-hidden-powershell.h"

#include <windows.h>

#include "activate-tip-log.h"

namespace goxvi::settings {

bool runHiddenPowerShell(const std::wstring& psCommand, unsigned timeoutMs,
                         const wchar_t* logTag) {
  wchar_t sysDir[MAX_PATH];
  if (GetSystemDirectoryW(sysDir, MAX_PATH) == 0) return false;
  std::wstring cmd = L"\"";
  cmd += sysDir;
  cmd +=
      L"\\WindowsPowerShell\\v1.0\\powershell.exe\" -NoProfile "
      L"-NonInteractive -ExecutionPolicy Bypass -Command \"";
  cmd += psCommand;
  cmd += L"\"";

  STARTUPINFOW si = {};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi = {};
  if (!CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE,
                      CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
    activateTipLog(L"%ls: CreateProcess powershell FAIL (err=%lu)", logTag,
                   GetLastError());
    return false;
  }
  const DWORD wait = WaitForSingleObject(pi.hProcess, timeoutMs);
  DWORD exitCode = 1;
  if (wait == WAIT_OBJECT_0) GetExitCodeProcess(pi.hProcess, &exitCode);
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  activateTipLog(L"%ls -> wait=%lu exit=%lu", logTag, wait, exitCode);
  return wait == WAIT_OBJECT_0 && exitCode == 0;
}

}  // namespace goxvi::settings
