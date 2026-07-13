#include "tip-registration.h"

#include <msctf.h>
#include <wrl/client.h>

#include <string>

#include "dll-module.h"
#include "goxvi-guids.h"

using Microsoft::WRL::ComPtr;

namespace goxvi_registration {
namespace {

// Decision (L5 REVISED): SECUREMODE and COMLESS are both registered.
// - SECUREMODE: apps can activate TSF with TF_TMAE_SECUREMODE and msctf then
//   activates ONLY TIPs in this category (probe-verified: ActivateEx(0x2)
//   skips loading a TIP without it). Games/anticheat hosts (LoL) do exactly
//   that, which left the TIP dead until the user re-toggled the input method
//   in-game. All builtin MS TIPs (Vietnamese Telex, JPN IME, Pinyin) carry it.
//   Secure-mode hygiene on our side: no key logging in Release (GOXVI_LOG
//   compile-out), no UI, password contexts respected, and the mouse
//   click-commit hook is skipped under TF_TMAE_SECUREMODE (see ActivateEx).
// - COMLESS: activation on threads that never called CoInitialize; activation
//   paths degrade gracefully (registerDisplayAttributeGuid is best-effort,
//   applyDisplayAttribute skips TF_INVALID_GUIDATOM).
const GUID* const kCategories[] = {
    &GUID_TFCAT_TIP_KEYBOARD,
    &GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT,
    &GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT,
    &GUID_TFCAT_TIPCAP_UIELEMENTENABLED,
    &GUID_TFCAT_TIPCAP_SECUREMODE,
    &GUID_TFCAT_TIPCAP_COMLESS,
    &GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER,
};

// Machine-wide CLSID path under HKLM (not HKCR). Elevated regsvr32 then writes
// to HKLM\SOFTWARE\Classes for all users; a 32-bit regsvr32 auto-redirects to
// ...\Wow6432Node\Classes, so 64-bit apps get the x64 DLL and 32-bit apps (Zalo)
// get the x86 DLL. Writing HKCR instead landed x86 in per-user HKCU (installer
// needs machine-wide).
std::wstring clsidKeyPath() {
  wchar_t clsid[64];
  StringFromGUID2(CLSID_GoxviTextService, clsid, 64);
  return std::wstring(L"SOFTWARE\\Classes\\CLSID\\") + clsid;
}

HRESULT createProfileMgr(ComPtr<ITfInputProcessorProfileMgr>& mgr) {
  return CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr,
                          CLSCTX_INPROC_SERVER,
                          IID_ITfInputProcessorProfileMgr, &mgr);
}

HRESULT createCategoryMgr(ComPtr<ITfCategoryMgr>& mgr) {
  return CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER,
                          IID_ITfCategoryMgr, &mgr);
}

}  // namespace

HRESULT registerComServer() {
  wchar_t modulePath[MAX_PATH];
  if (GetModuleFileNameW(g_hInstance, modulePath, MAX_PATH) == 0) return E_FAIL;

  const std::wstring keyPath = clsidKeyPath();
  HKEY key = nullptr;
  LSTATUS st = RegCreateKeyExW(HKEY_LOCAL_MACHINE, keyPath.c_str(), 0, nullptr,
                               REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr,
                               &key, nullptr);
  if (st != ERROR_SUCCESS) return HRESULT_FROM_WIN32(st);
  RegSetValueExW(key, nullptr, 0, REG_SZ,
                 reinterpret_cast<const BYTE*>(kGoxviDescription),
                 static_cast<DWORD>(sizeof(kGoxviDescription)));

  HKEY inproc = nullptr;
  st = RegCreateKeyExW(key, L"InprocServer32", 0, nullptr,
                       REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &inproc,
                       nullptr);
  RegCloseKey(key);
  if (st != ERROR_SUCCESS) return HRESULT_FROM_WIN32(st);

  RegSetValueExW(inproc, nullptr, 0, REG_SZ,
                 reinterpret_cast<const BYTE*>(modulePath),
                 static_cast<DWORD>((wcslen(modulePath) + 1) * sizeof(wchar_t)));
  const wchar_t threading[] = L"Apartment";
  RegSetValueExW(inproc, L"ThreadingModel", 0, REG_SZ,
                 reinterpret_cast<const BYTE*>(threading), sizeof(threading));
  RegCloseKey(inproc);
  return S_OK;
}

void unregisterComServer() {
  RegDeleteTreeW(HKEY_LOCAL_MACHINE, clsidKeyPath().c_str());
}

HRESULT registerProfile() {
  ComPtr<ITfInputProcessorProfileMgr> mgr;
  HRESULT hr = createProfileMgr(mgr);
  if (FAILED(hr)) return hr;
  return mgr->RegisterProfile(
      CLSID_GoxviTextService, kGoxviLangId, GUID_GoxviProfile,
      kGoxviDescription,
      static_cast<ULONG>(wcslen(kGoxviDescription)), nullptr, 0, 0, nullptr, 0,
      TRUE /*enabled by default*/, 0);
}

void unregisterProfile() {
  ComPtr<ITfInputProcessorProfileMgr> mgr;
  if (SUCCEEDED(createProfileMgr(mgr))) {
    mgr->UnregisterProfile(CLSID_GoxviTextService, kGoxviLangId,
                           GUID_GoxviProfile, 0);
  }
}

HRESULT registerCategories() {
  ComPtr<ITfCategoryMgr> mgr;
  HRESULT hr = createCategoryMgr(mgr);
  if (FAILED(hr)) return hr;
  for (const GUID* category : kCategories) {
    hr = mgr->RegisterCategory(CLSID_GoxviTextService, *category,
                               CLSID_GoxviTextService);
    if (FAILED(hr)) return hr;
  }
  return S_OK;
}

void unregisterCategories() {
  ComPtr<ITfCategoryMgr> mgr;
  if (FAILED(createCategoryMgr(mgr))) return;
  for (const GUID* category : kCategories) {
    mgr->UnregisterCategory(CLSID_GoxviTextService, *category,
                            CLSID_GoxviTextService);
  }
}

}  // namespace goxvi_registration
