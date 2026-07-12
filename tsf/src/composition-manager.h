#pragma once

#include <msctf.h>
#include <wrl/client.h>

#include <string>

// Owns the active ITfComposition. Mutations run in sync read/write edit
// sessions (async fallback when sync is denied outside key events — focus
// loss commits). The composition carries an "empty" display attribute (no
// underline) so the text looks like plain typing — hidden composition.
class CompositionManager {
 public:
  // Sink is the text service (ITfCompositionSink), weak — it owns this object.
  void initialize(ITfCompositionSink* sink, TfGuidAtom displayAttributeAtom);

  bool isComposing() const { return composition_ != nullptr; }

  // Start (if needed) at the current selection and replace the composition
  // text; caret goes to the end of the word.
  HRESULT updateText(ITfContext* context, TfClientId clientId,
                     const std::wstring& text);

  // Finalize: text stays exactly as displayed, composition ends.
  HRESULT commit(TfClientId clientId);

  // Finalize with a replacement (gõ tắt): replace the composition text with
  // `text`, move the caret to the end of it, then end the composition — all in
  // one edit session so the following terminator key lands after `text`.
  HRESULT commitWithReplacement(TfClientId clientId, const std::wstring& text);

  // Backspaced to empty: remove the composition text, then end it.
  HRESULT clearAndEnd(TfClientId clientId);

  // True when the current selection still lies within the composition range
  // (end-inclusive — our own edits park the caret at the end of the word).
  // False → the caret escaped (mouse click): the word must be committed.
  bool selectionCoveredByComposition(TfEditCookie ec, ITfContext* context);

  // App/TSF terminated the composition (caret move, click): text is already
  // final — just drop our references. Exception path only (no edit cookie);
  // the normal terminate path is finalizeExternalTermination below.
  void onExternallyTerminated();

  // ITfCompositionSink::OnCompositionTerminated glue: keep-style hosts leave
  // the text in place; cancel-style hosts delete the provisional text — then
  // re-insert `expectedText` (the display the user saw) via the write cookie
  // so a click/caret move never loses the word. Drops our references.
  void finalizeExternalTermination(TfEditCookie ecWrite,
                                   ITfComposition* composition,
                                   const std::wstring& expectedText);

 private:
  HRESULT ensureStarted(TfEditCookie ec, ITfContext* context);
  void applyDisplayAttribute(TfEditCookie ec, ITfContext* context, ITfRange* range);
  void clearDisplayAttribute(TfEditCookie ec, ITfContext* context, ITfRange* range);

  Microsoft::WRL::ComPtr<ITfComposition> composition_;
  Microsoft::WRL::ComPtr<ITfContext> context_;
  ITfCompositionSink* sink_ = nullptr;
  TfGuidAtom attributeAtom_ = TF_INVALID_GUIDATOM;
};
