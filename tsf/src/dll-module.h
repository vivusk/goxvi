#pragma once

#include <windows.h>

// Module-wide state shared by the COM plumbing.
extern HINSTANCE g_hInstance;

// DLL object/lock count for DllCanUnloadNow.
void DllAddRef();
void DllRelease();
LONG DllRefCount();
