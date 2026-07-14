// ITfKeyEventSink for GoxviTextService: classify keys, drive the engine,
// keep OnTestKeyDown/OnKeyDown eaten decisions identical.

#include "debug-logging.h"
#include "goxvi-text-service.h"
#include "key-event-handler.h"

using goxvi_keys::KeyAction;
using goxvi_keys::KeyDecision;

HRESULT GoxviTextService::handleKeyDown(ITfContext* context, WPARAM wparam,
                                        BOOL* eaten, bool testOnly) {
  if (!eaten) return E_INVALIDARG;
  *eaten = FALSE;
  if (!context) return S_OK;

  // Our own re-injected key (console ordering fix below): pass it through
  // untouched or it would loop forever.
  if (GetMessageExtraInfo() ==
      static_cast<LPARAM>(goxvi_keys::kGoxviInjectedKeyTag)) {
    return S_OK;
  }

  try {
    // Word start = no composition AND empty engine (direct-mode words never
    // compose — the engine is their only state). Poll config before
    // classifying: VNI's digit keys must be recognized as ProcessLetter even
    // as the very first key of a word (runs in Test too, idempotent, so the
    // eaten decision matches KeyDown).
    const bool wordStart =
        !composition_.isComposing() && engine_.currentDisplay().empty();
    if (wordStart) pollConfigAtWordStart();
    const KeyDecision decision =
        goxvi_keys::classifyKey(wparam, config_.inputMethod);
    switch (decision.action) {
      case KeyAction::Ignore:
        return S_OK;

      case KeyAction::CommitAndPass:
        // Terminators while composing are eaten, the word committed, then
        // the key re-posted — so it reaches the app strictly AFTER the
        // committed text. Committed text travels a slower lane than raw keys
        // in terminal hosts (PageUp history search over SSH) and CUAS edit
        // controls (Explorer F2 rename: Enter accepted the OLD name, '-'
        // landed before the word). Same mechanism as built-in Windows Telex.
        if (composition_.isComposing()) {
          *eaten = TRUE;
          if (!testOnly) {
            commitCurrentWord();
            // Injection blocked (BlockInput, desktop switch)? Un-eat so the
            // host delivers the original key itself — ordering degrades but
            // the user's Enter/space is never swallowed.
            if (!goxvi_keys::reinjectKey(wparam)) *eaten = FALSE;
          }
        } else if (!engine_.currentDisplay().empty()) {
          // Direct-mode word (URL bar): the text on screen is already real
          // input — nothing to commit, the terminator passes natively. That
          // IS the fix: Enter/End reach the omnibox with its default match
          // (inline suggestion) intact. Gõ tắt intentionally does not expand
          // here — an injected expansion would land AFTER the un-eaten
          // terminator.
          engine_.reset();
        }
        return S_OK;

      case KeyAction::CommitAndPassChord:
        // Ctrl/Alt/Win chord: commit, never eat — the app must see the real
        // chord with live modifier state. Runs in Test too (idempotent):
        // some apps only call TestKeyDown, and the word must be final before
        // the shortcut reaches the app.
        commitCurrentWord();
        return S_OK;

      case KeyAction::ProcessBackspace:
        if (composition_.isComposing()) {
          *eaten = TRUE;
          if (!testOnly) processBackspace(context);
        } else if (directWordMode_ && !engine_.currentDisplay().empty()) {
          *eaten = TRUE;
          if (!testOnly) directBackspace(context);
        }
        return S_OK;  // idle → pass through

      case KeyAction::CancelToRaw:
        if (composition_.isComposing()) {
          *eaten = TRUE;  // mid-word Esc only cancels the transform
          if (!testOnly) cancelWordToRaw(context);
        } else if (directWordMode_ && !engine_.currentDisplay().empty()) {
          *eaten = TRUE;
          if (!testOnly) directCancelToRaw(context);
        }
        return S_OK;  // idle Esc → app

      case KeyAction::ProcessLetter:
        if (wordStart) {
          // New word: respect disabled contexts and password fields (global
          // on/off is Windows' job — switch keyboard via Win+Space) and
          // classify the field. A browser URL/search bar (IS_URL/IS_SEARCH)
          // types through the direct injector: a composition commit there
          // resets the omnibox default match, so Enter/End land on the bare
          // typed text instead of the inline suggestion — built-in Telex has
          // the same bug. Verdicts from OnTestKeyDown are reused for the
          // OnKeyDown of the same key — the probe costs a sync edit session,
          // no need to pay for it twice per word start.
          const ULONGLONG now = GetTickCount64();
          bool blocked;
          bool urlField;
          if (context == blockCheck_.context && wparam == blockCheck_.vk &&
              now - blockCheck_.tick <= 100) {
            blocked = blockCheck_.blocked;
            urlField = blockCheck_.urlField;
          } else if (goxvi_keys::isContextDisabled(context)) {
            blocked = true;
            urlField = false;
          } else {
            const goxvi_keys::FieldScopes scopes =
                goxvi_keys::readFieldScopes(context, clientId_);
            blocked = scopes.password;
            urlField = scopes.urlField;
          }
          blockCheck_ = {context, wparam, now, blocked, urlField};
          if (blocked) return S_OK;
          directWordMode_ = hostHandlesClickTermination_ && urlField;
        }
        *eaten = TRUE;
        if (!testOnly) {
          if (directWordMode_ && !composition_.isComposing()) {
            directLetter(context, decision.ch);
          } else {
            // Typing over selected text in a CUAS edit control (Explorer F2
            // rename): the control deletes its selection synchronously via
            // WM_CLEAR before the composition opens — TSF-side deletes stay
            // invisible until commit there (see clearFocusedEditControlSelection).
            // KeyDown only: OnTestKeyDown must stay side-effect free.
            if (!composition_.isComposing()) {
              goxvi_keys::clearFocusedEditControlSelection();
            }
            processLetter(context, decision.ch);
          }
        }
        return S_OK;
    }
  } catch (...) {
    // Never crash the host app: drop the composition state and pass the key.
    composition_.onExternallyTerminated();
    engine_.reset();
    *eaten = FALSE;
  }
  return S_OK;
}

STDMETHODIMP GoxviTextService::OnSetFocus(BOOL foreground) {
  GOXVI_LOG(L"OnSetFocus foreground=%d", foreground);
  if (!foreground) commitCurrentWord();
  return S_OK;
}

STDMETHODIMP GoxviTextService::OnTestKeyDown(ITfContext* context, WPARAM wparam,
                                             LPARAM /*lparam*/, BOOL* eaten) {
  return handleKeyDown(context, wparam, eaten, /*testOnly=*/true);
}

STDMETHODIMP GoxviTextService::OnTestKeyUp(ITfContext* /*context*/,
                                           WPARAM /*wparam*/, LPARAM /*lparam*/,
                                           BOOL* eaten) {
  if (!eaten) return E_INVALIDARG;
  *eaten = FALSE;
  return S_OK;
}

STDMETHODIMP GoxviTextService::OnKeyDown(ITfContext* context, WPARAM wparam,
                                         LPARAM /*lparam*/, BOOL* eaten) {
  GOXVI_LOG(L"OnKeyDown vk=0x%02llX", static_cast<unsigned long long>(wparam));
  return handleKeyDown(context, wparam, eaten, /*testOnly=*/false);
}

STDMETHODIMP GoxviTextService::OnKeyUp(ITfContext* /*context*/, WPARAM /*wparam*/,
                                       LPARAM /*lparam*/, BOOL* eaten) {
  if (!eaten) return E_INVALIDARG;
  *eaten = FALSE;
  return S_OK;
}

STDMETHODIMP GoxviTextService::OnPreservedKey(ITfContext* /*context*/,
                                              REFGUID /*guid*/, BOOL* eaten) {
  if (!eaten) return E_INVALIDARG;
  *eaten = FALSE;
  return S_OK;
}
