#include "activate-tip.h"

#include <msctf.h>
#include <objbase.h>
#include <wrl/client.h>

#include <cstdio>
#include <string>

#include "goxvi-guids.h"

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
  if (!dll) return false;
  auto fn = reinterpret_cast<LayoutOrTipFn>(GetProcAddress(dll, fnName));
  const bool ok = fn && fn(psz, flags);
  FreeLibrary(dll);
  return ok;
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
    if (SUCCEEDED(CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr,
                                   CLSCTX_INPROC_SERVER,
                                   IID_ITfInputProcessorProfileMgr, &mgr))) {
      const HRESULT hr = mgr->ActivateProfile(
          TF_PROFILETYPE_INPUTPROCESSOR, kGoxviLangId, CLSID_GoxviTextService,
          GUID_GoxviProfile, nullptr,
          TF_IPPMF_FORSESSION | TF_IPPMF_ENABLEPROFILE);
      activated = SUCCEEDED(hr);
    }
  }  // ComPtr nhả trước CoUninitialize
  if (SUCCEEDED(hrInit)) CoUninitialize();
  return activated;
}

}  // namespace

bool activateGoxviTip() {
  const std::wstring tip = goxviTipString();
  // Bền vào language list trước — Settings/override-dropdown mới thấy Goxvi.
  const bool installed = callInputDll("InstallLayoutOrTip", tip.c_str(), 0);
  // Mặc định cho process mới (best-effort — không chặn kết quả chung).
  callInputDll("SetDefaultLayoutOrTip", tip.c_str(), 0);
  const bool activated = activateForSession();
  return installed && activated;
}

bool deactivateGoxviTip() {
  return callInputDll("InstallLayoutOrTip", goxviTipString().c_str(),
                      kIlotUninstall);
}

}  // namespace goxvi::settings
