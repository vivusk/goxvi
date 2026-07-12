#pragma once

#include <windows.h>

// Fixed GUIDs, generated once (phase 3). Never change: registry entries and
// user profiles reference them.
extern const CLSID CLSID_GoxviTextService;
extern const GUID GUID_GoxviProfile;
extern const GUID GUID_GoxviDisplayAttribute;

inline constexpr LANGID kGoxviLangId = 0x042A;  // Vietnamese
inline constexpr const wchar_t kGoxviDescription[] = L"Goxvi";  // one TIP, Telex+VNI in settings
