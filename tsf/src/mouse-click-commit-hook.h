#pragma once

#include <windows.h>

#include <functional>

// Thread-scoped WH_MOUSE hook: fires BEFORE the app's window proc sees a
// mouse-button-down, i.e. before IMM/CUAS hosts (Telegram/Qt, Sublime, EDIT
// controls) cancel the uncommitted composition string on click. The callback
// commits the composing word right there; the hook then eats the click and
// re-sends it through the input queue so the committed text (posted
// messages) reaches the app before the click does. TSF-native hosts
// (Notepad) never needed this — terminated composition text stays there.
namespace goxvi_mouse {

// Install on the current (input) thread. `onPointerDown` runs on this same
// thread for every client-area button down while installed; it returns true
// when a composing word was committed (the hook then eats + re-injects the
// click). Re-install replaces.
void installClickCommitHook(std::function<bool()> onPointerDown);

// Remove the hook for the current thread (TIP Deactivate). Safe when absent.
void removeClickCommitHook();

}  // namespace goxvi_mouse
