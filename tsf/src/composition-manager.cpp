#include "composition-manager.h"

#include "debug-logging.h"
#include "edit-session-callbacks.h"

using Microsoft::WRL::ComPtr;

void CompositionManager::initialize(ITfCompositionSink* sink,
                                    TfGuidAtom displayAttributeAtom) {
  sink_ = sink;
  attributeAtom_ = displayAttributeAtom;
}

HRESULT CompositionManager::ensureStarted(TfEditCookie ec, ITfContext* context) {
  if (composition_) return S_OK;

  ComPtr<ITfContextComposition> contextComposition;
  HRESULT hr = context->QueryInterface(IID_PPV_ARGS(&contextComposition));
  if (FAILED(hr)) return hr;

  // Compose at the current selection, not a QUERYONLY insert point: with
  // text selected (Explorer F2 rename selects the old name) the first key
  // must REPLACE it like plain typing. Some stores (Explorer's CUAS edit)
  // answer QUERYONLY with a collapsed range beside the selection, leaving the
  // old text in place.
  ComPtr<ITfRange> range;
  TF_SELECTION selection = {};
  ULONG fetched = 0;
  if (SUCCEEDED(context->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &selection,
                                      &fetched)) &&
      fetched == 1 && selection.range) {
    range.Attach(selection.range);  // take over the GetSelection reference
    // Safety net for a non-empty selection: delete it before composing so
    // the first SetText behaves like plain typing. The MAIN path never gets
    // here with a selection — the key sink injects a real VK_DELETE first
    // (see injectSelectionReplace): CUAS edit controls batch ALL TIP edits
    // (even ended scratch compositions) until the last composition ends, so
    // only a real key removes the old text from the screen immediately.
    BOOL selectionEmpty = TRUE;
    if (SUCCEEDED(range->IsEmpty(ec, &selectionEmpty)) && !selectionEmpty) {
      range->SetText(ec, 0, L"", 0);
    }
  } else {  // no selection object (rare) — fall back to the insert point
    ComPtr<ITfInsertAtSelection> insertAtSelection;
    HRESULT hrFallback = context->QueryInterface(IID_PPV_ARGS(&insertAtSelection));
    if (FAILED(hrFallback)) return hrFallback;
    hrFallback = insertAtSelection->InsertTextAtSelection(
        ec, TF_IAS_QUERYONLY, nullptr, 0, &range);
    if (FAILED(hrFallback) || !range) return FAILED(hrFallback) ? hrFallback : E_FAIL;
  }

  hr = contextComposition->StartComposition(ec, range.Get(), sink_, &composition_);
  if (SUCCEEDED(hr) && !composition_) hr = E_FAIL;  // app refused composition
  if (SUCCEEDED(hr)) context_ = context;
  return hr;
}

HRESULT CompositionManager::updateText(ITfContext* context, TfClientId clientId,
                                       const std::wstring& text) {
  // Capture by value: the session normally runs synchronously (key event),
  // but runEditSession falls back to async when sync is denied.
  ComPtr<ITfContext> ctx(context);
  return runEditSession(
      context, clientId, TF_ES_SYNC | TF_ES_READWRITE,
      [this, ctx, text](TfEditCookie ec) {
        ITfContext* context = ctx.Get();
        HRESULT hr = ensureStarted(ec, context);
        if (FAILED(hr)) return hr;

        ComPtr<ITfRange> range;
        hr = composition_->GetRange(&range);
        if (FAILED(hr)) return hr;
        hr = range->SetText(ec, 0, text.c_str(), static_cast<LONG>(text.size()));
        if (FAILED(hr)) return hr;

        applyDisplayAttribute(ec, context, range.Get());

        ComPtr<ITfRange> caret;
        if (SUCCEEDED(range->Clone(&caret))) {  // caret to end of the word
          caret->Collapse(ec, TF_ANCHOR_END);
          TF_SELECTION selection;
          selection.range = caret.Get();
          selection.style.ase = TF_AE_END;
          selection.style.fInterimChar = FALSE;
          context->SetSelection(ec, 1, &selection);
        }
        return S_OK;
      });
}

HRESULT CompositionManager::commit(TfClientId clientId) {
  if (!composition_) return S_OK;
  ComPtr<ITfComposition> composition = composition_;
  ComPtr<ITfContext> context = context_;
  composition_.Reset();
  context_.Reset();
  return runEditSession(
      context.Get(), clientId, TF_ES_SYNC | TF_ES_READWRITE,
      [this, composition, context](TfEditCookie ec) {
        ComPtr<ITfRange> range;
        if (SUCCEEDED(composition->GetRange(&range))) {
          clearDisplayAttribute(ec, context.Get(), range.Get());
        }
        return composition->EndComposition(ec);
      });
}

HRESULT CompositionManager::commitWithReplacement(TfClientId clientId,
                                                  const std::wstring& text) {
  if (!composition_) return S_OK;
  ComPtr<ITfComposition> composition = composition_;
  ComPtr<ITfContext> context = context_;
  composition_.Reset();
  context_.Reset();
  return runEditSession(
      context.Get(), clientId, TF_ES_SYNC | TF_ES_READWRITE,
      [this, composition, context, text](TfEditCookie ec) {
        ComPtr<ITfRange> range;
        HRESULT hr = composition->GetRange(&range);
        if (FAILED(hr)) return hr;
        hr = range->SetText(ec, 0, text.c_str(), static_cast<LONG>(text.size()));
        if (FAILED(hr)) return hr;

        // Caret to end of the replacement (H2): the terminator key is NOT
        // eaten, so without this it would land at the old caret position and
        // split the expansion.
        ComPtr<ITfRange> caret;
        if (SUCCEEDED(range->Clone(&caret))) {
          caret->Collapse(ec, TF_ANCHOR_END);
          TF_SELECTION selection;
          selection.range = caret.Get();
          selection.style.ase = TF_AE_END;
          selection.style.fInterimChar = FALSE;
          context->SetSelection(ec, 1, &selection);
        }
        clearDisplayAttribute(ec, context.Get(), range.Get());
        return composition->EndComposition(ec);
      });
}

HRESULT CompositionManager::clearAndEnd(TfClientId clientId) {
  if (!composition_) return S_OK;
  ComPtr<ITfComposition> composition = composition_;
  ComPtr<ITfContext> context = context_;
  composition_.Reset();
  context_.Reset();
  return runEditSession(
      context.Get(), clientId, TF_ES_SYNC | TF_ES_READWRITE,
      [this, composition, context](TfEditCookie ec) {
        ComPtr<ITfRange> range;
        if (SUCCEEDED(composition->GetRange(&range))) {
          range->SetText(ec, 0, L"", 0);
          clearDisplayAttribute(ec, context.Get(), range.Get());
        }
        return composition->EndComposition(ec);
      });
}

bool CompositionManager::selectionCoveredByComposition(TfEditCookie ec,
                                                       ITfContext* context) {
  if (!composition_) return true;  // nothing to protect
  if (context_ && context != context_.Get()) return false;  // other context

  ComPtr<ITfRange> compRange;
  if (FAILED(composition_->GetRange(&compRange)) || !compRange) return false;

  TF_SELECTION selection = {};
  ULONG fetched = 0;
  if (FAILED(context->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &selection,
                                   &fetched)) ||
      fetched != 1 || !selection.range) {
    return false;
  }
  ComPtr<ITfRange> selRange;
  selRange.Attach(selection.range);

  LONG startCmp = 0;
  LONG endCmp = 0;
  if (FAILED(selRange->CompareStart(ec, compRange.Get(), TF_ANCHOR_START,
                                    &startCmp)) ||
      FAILED(selRange->CompareEnd(ec, compRange.Get(), TF_ANCHOR_END,
                                  &endCmp))) {
    return false;
  }
  return startCmp >= 0 && endCmp <= 0;
}

void CompositionManager::onExternallyTerminated() {
  GOXVI_LOG(L"composition terminated by app/TSF");
  composition_.Reset();
  context_.Reset();
}

void CompositionManager::finalizeExternalTermination(
    TfEditCookie ecWrite, ITfComposition* composition,
    const std::wstring& expectedText) {
  GOXVI_LOG(L"OnCompositionTerminated (composing=%d)",
            composition_ ? 1 : 0);
  ComPtr<ITfContext> context = context_;
  composition_.Reset();
  context_.Reset();
  if (!composition || !context) return;

  ComPtr<ITfRange> range;
  if (FAILED(composition->GetRange(&range)) || !range) return;

  // Keep-style hosts (RichEdit...) leave the terminated text in the document —
  // nothing to do. Cancel-style hosts (UWP CoreText, CUAS console...) treat an
  // uncommitted composition as provisional and delete it before this callback:
  // the word the user saw would vanish on a click/caret move, so re-insert it
  // with the write cookie TSF hands us for exactly this purpose.
  if (!expectedText.empty()) {
    wchar_t buf[128] = {};
    ULONG got = 0;
    ComPtr<ITfRange> probe;
    if (SUCCEEDED(range->Clone(&probe))) {
      probe->GetText(ecWrite, 0, buf, 127, &got);
    }
    if (std::wstring(buf, got) != expectedText) {
      GOXVI_LOG(L"terminated composition lost its text -> re-insert");
      range->SetText(ecWrite, 0, expectedText.c_str(),
                     static_cast<LONG>(expectedText.size()));
    }
  }
  clearDisplayAttribute(ecWrite, context.Get(), range.Get());
}

void CompositionManager::applyDisplayAttribute(TfEditCookie ec,
                                               ITfContext* context,
                                               ITfRange* range) {
  if (attributeAtom_ == TF_INVALID_GUIDATOM) return;
  ComPtr<ITfProperty> property;
  if (FAILED(context->GetProperty(GUID_PROP_ATTRIBUTE, &property))) return;
  VARIANT var;
  VariantInit(&var);
  var.vt = VT_I4;
  var.lVal = static_cast<LONG>(attributeAtom_);
  property->SetValue(ec, range, &var);
}

void CompositionManager::clearDisplayAttribute(TfEditCookie ec,
                                               ITfContext* context,
                                               ITfRange* range) {
  ComPtr<ITfProperty> property;
  if (FAILED(context->GetProperty(GUID_PROP_ATTRIBUTE, &property))) return;
  property->Clear(ec, range);
}
