#pragma once

#include <msctf.h>
#include <windows.h>

#include "goxvi/engine-types.h"

// Key classification + context checks for the key event sink.
namespace goxvi_keys {

enum class KeyAction {
  Ignore,            // Shift/CapsLock/... — no commit, not eaten
  ProcessLetter,     // A-Z → engine (ch carries the cased character)
  ProcessBackspace,  // engine if composing, else pass through
  CancelToRaw,        // Esc: revert word to raw typed keys + commit, eaten
                      // (UniKey convention); pass through when not composing
  CommitAndPass,      // word terminator (space, Enter, punctuation, nav...):
                      // while composing → eat + commit + reinject the key
  CommitAndPassChord, // Ctrl/Alt/Win chord: commit, never eaten — the app
                      // must see the real chord with live modifier state
};

struct KeyDecision {
  KeyAction action = KeyAction::CommitAndPass;
  wchar_t ch = 0;
};

// VK A-Z is mapped manually with Shift/CapsLock — NOT ToUnicodeEx, whose side
// effects destroy the layout's dead-key state (M4). Non-QWERTY layouts are
// knowingly unsupported for now. VK '0'-'9' become ProcessLetter only under
// VNI (its tone/diacritic keys); under Telex they still terminate the word.
KeyDecision classifyKey(WPARAM vk, goxvi::InputMethod inputMethod);

// Compartment checks (keyboard disabled / empty context) — no edit session.
bool isContextDisabled(ITfContext* context);

// Field classification from the GUID_PROP_INPUTSCOPE of the current selection,
// read in one sync read-only edit session. Fail-safe defaults apply when the
// sync session is denied or the scope is unreadable: password=true (letters
// pass through raw rather than composing in a possible password field),
// urlField=false (normal composition path).
struct FieldScopes {
  bool password = true;
  bool urlField = false;  // IS_URL / IS_SEARCH — browser omnibox and friends
};
FieldScopes readFieldScopes(ITfContext* context, TfClientId clientId);

// True when the app currently holds a NON-EMPTY selection (sync read-only
// session; false on failure). Used by direct mode: an omnibox parks its
// inline-autocomplete tail as a selection right after the caret, and injected
// backspaces would consume THAT first — the injector must know to clear it.
bool selectionIsNonEmpty(ITfContext* context, TfClientId clientId);

// Typing over a selected text in a CUAS edit control (Explorer F2 rename —
// class "Edit" and friends): the control must delete its selection BEFORE the
// composition starts, or the old text stays visible next to the new word
// until commit — CUAS batches TSF document edits while a composition is
// active AND hides the real selection from GetSelection (returns collapsed).
// The TIP runs in-process, so at word start this asks the focused edit
// control directly (EM_GETSEL) and deletes a non-empty selection with a
// synchronous WM_CLEAR — fully applied before the composition opens. Key
// injection (VK_DELETE + re-posted letter) was tried first and failed:
// Explorer's rename edit terminates the young composition on the text-change
// notification, splitting the word ("ar" instead of "ả"). No-op for TSF-aware
// apps (class check fails) — their selection is handled in ensureStarted.
void clearFocusedEditControlSelection();

// Navigation keys (E0-extended scan codes) — used to set KEYEVENTF_EXTENDEDKEY
// when re-posting.
bool isNavigationKey(WPARAM vk);

// Re-post `vk` via SendInput, tagged with kGoxviInjectedKeyTag so our own
// key sink recognizes it (GetMessageExtraInfo) and passes it through.
// Terminator keys while composing are eaten, the word committed, then the key
// re-posted — so it reaches the app strictly AFTER the committed text. The
// committed text travels a slower lane than raw keys in terminal hosts (PageUp
// history over SSH) and CUAS edit controls (Explorer F2 rename: Enter used to
// accept the OLD name, '-' landed before the word). Mirrors built-in Telex.
// Returns false when SendInput injected nothing (BlockInput, desktop switch):
// the caller must fall back to NOT eating the original key or it is lost.
bool reinjectKey(WPARAM vk);

inline constexpr ULONG_PTR kGoxviInjectedKeyTag = 0x476F7856;  // 'GoxV'

}  // namespace goxvi_keys
