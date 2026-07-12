#include "edit-session-callbacks.h"

#include <new>

namespace {

class LambdaEditSession : public ITfEditSession {
 public:
  explicit LambdaEditSession(const std::function<HRESULT(TfEditCookie)>& fn)
      : fn_(fn) {}

  STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
    if (!ppv) return E_INVALIDARG;
    if (riid == IID_IUnknown || riid == IID_ITfEditSession) {
      *ppv = static_cast<ITfEditSession*>(this);
      AddRef();
      return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
  }

  STDMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&refCount_); }

  STDMETHODIMP_(ULONG) Release() override {
    const LONG count = InterlockedDecrement(&refCount_);
    if (count == 0) delete this;
    return count;
  }

  STDMETHODIMP DoEditSession(TfEditCookie ec) override {
    try {
      return fn_(ec);
    } catch (...) {
      return E_FAIL;  // never leak exceptions across the COM boundary
    }
  }

 private:
  ~LambdaEditSession() = default;

  LONG refCount_ = 1;
  std::function<HRESULT(TfEditCookie)> fn_;
};

}  // namespace

HRESULT runEditSession(ITfContext* context, TfClientId clientId, DWORD flags,
                       const std::function<HRESULT(TfEditCookie)>& fn,
                       bool allowAsyncFallback) {
  if (!context) return E_INVALIDARG;
  LambdaEditSession* session = new (std::nothrow) LambdaEditSession(fn);
  if (!session) return E_OUTOFMEMORY;
  HRESULT sessionResult = S_OK;
  HRESULT hr =
      context->RequestEditSession(clientId, session, flags, &sessionResult);
  if (allowAsyncFallback && hr == TF_E_SYNCHRONOUS && (flags & TF_ES_SYNC)) {
    // Sync sessions are only granted inside key-event processing. Commits
    // from focus loss / Deactivate land here — retry async so the word still
    // gets finalized (callbacks capture state by value, so late execution is
    // safe). DONTCARE: the session's own HRESULT is unavailable/ignored.
    sessionResult = S_OK;
    hr = context->RequestEditSession(clientId, session,
                                     (flags & ~TF_ES_SYNC) | TF_ES_ASYNCDONTCARE,
                                     &sessionResult);
  }
  session->Release();
  return FAILED(hr) ? hr : sessionResult;
}
