#pragma once

#include <unknwn.h>
#include <windows.h>

// Minimal IClassFactory for GoxviTextService. A single static instance is
// handed out by DllGetClassObject; module lifetime is tracked via DllAddRef.
class GoxviClassFactory : public IClassFactory {
 public:
  // IUnknown
  STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
  STDMETHODIMP_(ULONG) AddRef() override;
  STDMETHODIMP_(ULONG) Release() override;

  // IClassFactory
  STDMETHODIMP CreateInstance(IUnknown* outer, REFIID riid, void** ppv) override;
  STDMETHODIMP LockServer(BOOL lock) override;
};

extern GoxviClassFactory g_classFactory;
