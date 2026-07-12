#pragma once

#include <windows.h>

// Registry/TSF registration used by DllRegisterServer/DllUnregisterServer.
// Requires admin (writes HKLM); run once via scripts/register-tip.ps1.
namespace goxvi_registration {

HRESULT registerComServer();     // CLSID\{...}\InprocServer32 (Apartment)
void unregisterComServer();

HRESULT registerProfile();       // ITfInputProcessorProfileMgr, langid 0x042A
void unregisterProfile();

HRESULT registerCategories();    // keyboard TIP + capability categories
void unregisterCategories();

}  // namespace goxvi_registration
