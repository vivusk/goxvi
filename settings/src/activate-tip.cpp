#include "activate-tip.h"

#include <msctf.h>
#include <objbase.h>
#include <wrl/client.h>

#include "goxvi-guids.h"

using Microsoft::WRL::ComPtr;

namespace goxvi::settings {

// Người dùng VN chưa quen Win+Space -> installer cho tick chọn "đặt Goxvi làm bàn
// phím VIE ngay". Chỉ dùng API TSF (KHÔNG chọc HKCU\...\User Profile bẩn/fragile):
// ActivateProfile với TF_IPPMF_FORSESSION|TF_IPPMF_ENABLEPROFILE bật + kích hoạt
// profile cho cả phiên user; Windows sau đó tự nhớ lựa chọn gần nhất per-user.
// PHẢI chạy trong ngữ cảnh user (runasoriginaluser) thì mới tác động đúng phiên.
// COM-only qua CoCreateInstance + GUID initguid (goxvi-guids.cpp) — không cần
// msctf.lib (SDK 26100 đã bỏ), giống cách TIP DLL đăng ký profile.
bool activateGoxviTip() {
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

}  // namespace goxvi::settings
