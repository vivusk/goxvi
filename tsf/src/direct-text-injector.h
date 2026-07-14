#pragma once

#include <windows.h>

#include <string>

// Direct input mode for browser URL/search bars (Chromium omnibox): a TSF
// composition commit resets the omnibox's default match, so Enter/End land on
// the bare typed text instead of the inline suggestion (built-in Telex has
// the same bug — it is the composition model itself that loses). Here every
// transform is realized as REAL keyboard input instead: backspaces to erase
// the changed tail + KEYEVENTF_UNICODE chars to retype it. The omnibox sees
// plain typing, so suggestions, Enter and End behave exactly as with English.
// (Revived from the terminal-host direct mode dropped in c22195e.)
namespace goxvi_direct {

// Erase `eraseCount` chars (synthetic VK_BACK) then type `insertText`
// (VK_PACKET unicode), all tagged with kGoxviInjectedKeyTag so the key sink
// passes them through untouched. leadingForwardDelete prepends one VK_DELETE:
// an omnibox parks its inline-autocomplete tail as a SELECTION after the
// caret, and the first VK_BACK would consume that instead of a word char —
// the forward delete clears the tail (a no-op at a plain insertion point,
// since the word being edited always has its caret at the end).
void sendTextReplacement(int eraseCount, const std::wstring& insertText,
                         bool leadingForwardDelete);

}  // namespace goxvi_direct
