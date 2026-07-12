#pragma once

#include <msctf.h>

#include <functional>

// Runs `fn` inside an ITfEditSession requested on `context`. All text
// manipulation must go through here (TSF async-aware contract). Key events
// use TF_ES_SYNC | TF_ES_READWRITE (SampleIME pattern).
//
// allowAsyncFallback: when a TF_ES_SYNC request is denied, retry async so the
// edit still happens later (commits from focus loss / Deactivate). Callers
// whose lambda captures locals BY REFERENCE, or who read a result right after
// returning (isPasswordContext), MUST pass false — an async retry would run
// the lambda after those locals are gone.
HRESULT runEditSession(ITfContext* context, TfClientId clientId, DWORD flags,
                       const std::function<HRESULT(TfEditCookie)>& fn,
                       bool allowAsyncFallback = true);
