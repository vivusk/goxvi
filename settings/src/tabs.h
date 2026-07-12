#pragma once

#include <windows.h>

#include "ui-common.h"

// Each tab is a WS_CHILD container window (its own class + WndProc, state in
// GWLP_USERDATA) placed inside the tab control's display rect and shown one at
// a time. All three edit the SAME ConfigStore via the PageContext and save on
// change. `area` is the tab display rect in pixels (already DPI-correct).
namespace goxvi::settings::ui {

HWND createOverviewTab(HWND parent, const PageContext& ctx, RECT area);
HWND createShortcutsTab(HWND parent, const PageContext& ctx, RECT area);
HWND createAboutTab(HWND parent, const PageContext& ctx, RECT area);

}  // namespace goxvi::settings::ui
