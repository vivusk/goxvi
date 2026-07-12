// The four COM exports (see goxvi-tsf.def).

#include <windows.h>

#include "class-factory.h"
#include "dll-module.h"
#include "goxvi-guids.h"
#include "tip-registration.h"

namespace {

// regsvr32 initializes COM, but be defensive when called elsewhere.
struct ScopedCoInit {
  ScopedCoInit() {
    const HRESULT hr = CoInitialize(nullptr);
    needUninit = SUCCEEDED(hr);  // RPC_E_CHANGED_MODE → already initialized
  }
  ~ScopedCoInit() {
    if (needUninit) CoUninitialize();
  }
  bool needUninit = false;
};

}  // namespace

STDAPI DllUnregisterServer();  // used by DllRegisterServer's rollback

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv) {
  if (!ppv) return E_INVALIDARG;
  *ppv = nullptr;
  if (rclsid != CLSID_GoxviTextService) return CLASS_E_CLASSNOTAVAILABLE;
  return g_classFactory.QueryInterface(riid, ppv);
}

STDAPI DllCanUnloadNow() { return DllRefCount() <= 0 ? S_OK : S_FALSE; }

STDAPI DllRegisterServer() {
  ScopedCoInit coInit;
  HRESULT hr = goxvi_registration::registerComServer();
  if (SUCCEEDED(hr)) hr = goxvi_registration::registerProfile();
  if (SUCCEEDED(hr)) hr = goxvi_registration::registerCategories();
  if (FAILED(hr)) DllUnregisterServer();
  return hr;
}

STDAPI DllUnregisterServer() {
  ScopedCoInit coInit;
  goxvi_registration::unregisterCategories();
  goxvi_registration::unregisterProfile();
  goxvi_registration::unregisterComServer();
  return S_OK;
}
