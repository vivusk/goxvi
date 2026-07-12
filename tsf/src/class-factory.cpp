#include "class-factory.h"

#include <new>

#include "dll-module.h"
#include "goxvi-text-service.h"

GoxviClassFactory g_classFactory;

STDMETHODIMP GoxviClassFactory::QueryInterface(REFIID riid, void** ppv) {
  if (!ppv) return E_INVALIDARG;
  if (riid == IID_IUnknown || riid == IID_IClassFactory) {
    *ppv = static_cast<IClassFactory*>(this);
    AddRef();
    return S_OK;
  }
  *ppv = nullptr;
  return E_NOINTERFACE;
}

// Static lifetime object — ref counting is a no-op; the module lock keeps the
// DLL loaded instead.
STDMETHODIMP_(ULONG) GoxviClassFactory::AddRef() { return 2; }

STDMETHODIMP_(ULONG) GoxviClassFactory::Release() { return 1; }

STDMETHODIMP GoxviClassFactory::CreateInstance(IUnknown* outer, REFIID riid,
                                               void** ppv) {
  if (!ppv) return E_INVALIDARG;
  *ppv = nullptr;
  if (outer) return CLASS_E_NOAGGREGATION;

  GoxviTextService* service = new (std::nothrow) GoxviTextService();
  if (!service) return E_OUTOFMEMORY;

  const HRESULT hr = service->QueryInterface(riid, ppv);
  service->Release();  // QI holds the caller's reference on success
  return hr;
}

STDMETHODIMP GoxviClassFactory::LockServer(BOOL lock) {
  if (lock) {
    DllAddRef();
  } else {
    DllRelease();
  }
  return S_OK;
}
