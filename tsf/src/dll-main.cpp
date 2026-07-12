#include "dll-module.h"

HINSTANCE g_hInstance = nullptr;

namespace {
volatile LONG g_dllRefCount = 0;
}

void DllAddRef() { InterlockedIncrement(&g_dllRefCount); }
void DllRelease() { InterlockedDecrement(&g_dllRefCount); }
LONG DllRefCount() { return g_dllRefCount; }

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD reason, LPVOID /*reserved*/) {
  switch (reason) {
    case DLL_PROCESS_ATTACH:
      g_hInstance = hInstance;
      DisableThreadLibraryCalls(hInstance);
      break;
    case DLL_PROCESS_DETACH:
      break;
  }
  return TRUE;
}
