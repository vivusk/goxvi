#include "activate-tip.h"

#include <msctf.h>
#include <objbase.h>
#include <wrl/client.h>

#include <cstdio>
#include <string>

#include "activate-tip-log.h"
#include "default-input-override.h"
#include "goxvi-guids.h"
#include "run-hidden-powershell.h"

using Microsoft::WRL::ComPtr;

namespace goxvi::settings {
namespace {

// Chuỗi "042A:{CLSID}{ProfileGUID}" theo định dạng layout-or-TIP của input.dll.
// Dựng từ đúng GUID hằng trong goxvi-guids.cpp (một nguồn, không chép literal).
std::wstring goxviTipString() {
  wchar_t clsid[64];
  wchar_t profile[64];
  StringFromGUID2(CLSID_GoxviTextService, clsid, 64);
  StringFromGUID2(GUID_GoxviProfile, profile, 64);
  wchar_t buf[160];
  swprintf_s(buf, L"%04X:%s%s", kGoxviLangId, clsid, profile);
  return buf;
}

// InstallLayoutOrTip / SetDefaultLayoutOrTip (input.dll) — API chuẩn cho
// installer bộ gõ, không có import lib trong SDK nên load động. Cùng chữ ký.
using LayoutOrTipFn = BOOL(WINAPI*)(LPCWSTR psz, DWORD dwFlags);
constexpr DWORD kIlotUninstall = 0x00000001;  // ILOT_UNINSTALL

bool callInputDll(const char* fnName, LPCWSTR psz, DWORD flags) {
  HMODULE dll =
      LoadLibraryExW(L"input.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
  if (!dll) {
    activateTipLog(L"%hs: LoadLibrary input.dll FAIL (err=%lu)", fnName,
                   GetLastError());
    return false;
  }
  auto fn = reinterpret_cast<LayoutOrTipFn>(GetProcAddress(dll, fnName));
  const bool ok = fn && fn(psz, flags);
  const DWORD err = ok ? 0 : GetLastError();
  FreeLibrary(dll);
  activateTipLog(L"%hs(flags=%lu) -> %ls (err=%lu)", fnName, flags,
                 ok ? L"OK" : L"FAIL", err);
  return ok;
}

// Xác minh BỀN VỮNG thay vì tin BOOL của InstallLayoutOrTip (field report
// 0.0.3: máy cài xong vẫn thiếu Goxvi trong Settings/override-dropdown).
// InstallLayoutOrTip ghi tip thành value name dưới
// HKCU\Control Panel\International\User Profile\<bcp47-tag>\ — chính nguồn mà
// Settings đọc để dựng language list. Đọc-only. Quét mọi <tag> con để không
// phụ thuộc Windows đặt 'vi' hay 'vi-VN'; value name registry vốn
// case-insensitive nên so thẳng bằng RegQueryValueEx.
bool tipPresentInUserProfile(const std::wstring& tip) {
  HKEY root = nullptr;
  if (RegOpenKeyExW(HKEY_CURRENT_USER,
                    L"Control Panel\\International\\User Profile", 0, KEY_READ,
                    &root) != ERROR_SUCCESS) {
    return false;
  }
  bool found = false;
  for (DWORD i = 0; !found; ++i) {
    wchar_t sub[128];
    DWORD len = 128;
    if (RegEnumKeyExW(root, i, sub, &len, nullptr, nullptr, nullptr,
                      nullptr) != ERROR_SUCCESS) {
      break;  // hết subkey
    }
    HKEY lang = nullptr;
    if (RegOpenKeyExW(root, sub, 0, KEY_READ, &lang) != ERROR_SUCCESS) {
      continue;
    }
    found = RegQueryValueExW(lang, tip.c_str(), nullptr, nullptr, nullptr,
                             nullptr) == ERROR_SUCCESS;
    RegCloseKey(lang);
  }
  RegCloseKey(root);
  return found;
}

// Đường lui khi InstallLayoutOrTip không ghi bền được: Set-WinUserLanguageList
// là cmdlet chính chủ (Win8+), tự thêm ngôn ngữ Việt nếu máy chưa có rồi chèn
// tip vào InputMethodTips. Chỉ chạy khi verify lần 1 fail nên chi phí spawn
// powershell.exe không nằm trên đường thành công thường gặp.
// -like 'vi*' khớp cả 'vi' lẫn 'vi-VN'; exit 2 = không thêm được ngôn ngữ.
bool runLanguageListFallback(const std::wstring& tip) {
  std::wstring ps = L"$tip='" + tip + L"'; ";
  ps +=
      L"$list = Get-WinUserLanguageList; "
      L"$vi = $list | Where-Object { $_.LanguageTag -like 'vi*' } "
      L"| Select-Object -First 1; "
      L"if (-not $vi) { $list.Add('vi-VN'); "
      L"$vi = $list | Where-Object { $_.LanguageTag -like 'vi*' } "
      L"| Select-Object -First 1 }; "
      L"if (-not $vi) { exit 2 }; "
      L"if ($vi.InputMethodTips -notcontains $tip) "
      L"{ $vi.InputMethodTips.Add($tip) }; "
      L"Set-WinUserLanguageList -LanguageList $list -Force; exit 0";
  // 60 s: Set-WinUserLanguageList có thể chậm khi thêm ngôn ngữ mới.
  return runHiddenPowerShell(ps, 60000, L"fallback Set-WinUserLanguageList");
}

// Kích hoạt profile cho PHIÊN hiện tại (hiệu lực ngay, khỏi Win+Space). Chỉ
// dùng API TSF (KHÔNG chọc registry User Profile bẩn/fragile). COM-only qua
// CoCreateInstance + GUID initguid (goxvi-guids.cpp) — không cần msctf.lib
// (SDK 26100 đã bỏ), giống cách TIP DLL đăng ký profile.
bool activateForSession() {
  const HRESULT hrInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  bool activated = false;
  {
    ComPtr<ITfInputProcessorProfileMgr> mgr;
    const HRESULT hrCreate = CoCreateInstance(
        CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
        IID_ITfInputProcessorProfileMgr, &mgr);
    if (SUCCEEDED(hrCreate)) {
      const HRESULT hr = mgr->ActivateProfile(
          TF_PROFILETYPE_INPUTPROCESSOR, kGoxviLangId, CLSID_GoxviTextService,
          GUID_GoxviProfile, nullptr,
          TF_IPPMF_FORSESSION | TF_IPPMF_ENABLEPROFILE);
      activated = SUCCEEDED(hr);
      activateTipLog(L"ActivateProfile FORSESSION -> hr=0x%08X",
                     static_cast<unsigned>(hr));
    } else {
      activateTipLog(L"CoCreate TF_InputProcessorProfiles -> hr=0x%08X",
                     static_cast<unsigned>(hrCreate));
    }
  }  // ComPtr nhả trước CoUninitialize
  if (SUCCEEDED(hrInit)) CoUninitialize();
  return activated;
}

}  // namespace

bool activateGoxviTip() {
  activateTipLogBegin(L"--activate-tip");
  const std::wstring tip = goxviTipString();
  activateTipLog(L"tip=%ls", tip.c_str());

  // Bước 1 — BỀN vào language list: Settings/override-dropdown CHỈ liệt kê
  // kiểu gõ trong User Profile của user; đăng ký HKLM (regsvr32) là chưa đủ.
  callInputDll("InstallLayoutOrTip", tip.c_str(), 0);
  bool persistent = tipPresentInUserProfile(tip);
  activateTipLog(L"verify User Profile -> %ls",
                 persistent ? L"PRESENT" : L"MISSING");
  if (!persistent) {
    runLanguageListFallback(tip);
    persistent = tipPresentInUserProfile(tip);
    activateTipLog(L"verify User Profile (post-fallback) -> %ls",
                   persistent ? L"PRESENT" : L"MISSING");
  }

  // Bước 2 — MẶC ĐỊNH cho process mới. SetDefaultLayoutOrTip giữ lại cho các
  // state khác (preload/logon) nhưng đã kiểm chứng nó KHÔNG ghi
  // InputMethodOverride (field report 0.0.4) -> ensure... mới là bước quyết
  // định, có verify.
  callInputDll("SetDefaultLayoutOrTip", tip.c_str(), 0);
  const bool defaulted = ensureDefaultInputMethodOverride(tip);

  // Bước 3 — hiệu lực ngay cho phiên (best-effort, đã log: fail thì user
  // Win+Space chọn được vì list + default đã đúng).
  activateForSession();

  activateTipLog(L"result: persistent=%ls default=%ls",
                 persistent ? L"OK" : L"FAIL", defaulted ? L"OK" : L"FAIL");
  return persistent && defaulted;
}

bool deactivateGoxviTip() {
  activateTipLogBegin(L"--deactivate-tip");
  const std::wstring tip = goxviTipString();
  // Dọn override TRƯỚC khi gỡ khỏi list — tránh cửa sổ override trỏ TIP ma.
  clearDefaultInputMethodOverrideIfGoxvi(tip);
  return callInputDll("InstallLayoutOrTip", tip.c_str(), kIlotUninstall);
}

}  // namespace goxvi::settings
