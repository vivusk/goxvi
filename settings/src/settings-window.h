#pragma once

#include <windows.h>

#include "config-store.h"

// The single top-level window: a native tab control hosting the three tab pages
// (Tổng quan / Bảng gõ tắt / Giới thiệu). Owns the ConfigStore that every page
// edits.
namespace goxvi::settings {

// Window class of the top-level window — shared with main.cpp so the
// single-instance check finds OUR window (a title-only FindWindow could match
// any stray window that happens to be named "Goxvi").
inline constexpr wchar_t kMainWindowClass[] = L"GoxviSettingsMain";

// Create + show the window. Returns its HWND (null on failure). The window runs
// the standard message loop in main.cpp.
HWND createSettingsWindow();

}  // namespace goxvi::settings
