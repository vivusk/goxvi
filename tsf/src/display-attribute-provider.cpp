// GoxviDisplayAttributeInfo / enumerator + the text service's
// ITfDisplayAttributeProvider methods.

#include "display-attribute-provider.h"

#include <new>

#include "goxvi-guids.h"
#include "goxvi-text-service.h"

// ---- GoxviDisplayAttributeInfo ----------------------------------------------

STDMETHODIMP GoxviDisplayAttributeInfo::QueryInterface(REFIID riid, void** ppv) {
  if (!ppv) return E_INVALIDARG;
  if (riid == IID_IUnknown || riid == IID_ITfDisplayAttributeInfo) {
    *ppv = static_cast<ITfDisplayAttributeInfo*>(this);
    AddRef();
    return S_OK;
  }
  *ppv = nullptr;
  return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) GoxviDisplayAttributeInfo::AddRef() {
  return InterlockedIncrement(&refCount_);
}

STDMETHODIMP_(ULONG) GoxviDisplayAttributeInfo::Release() {
  const LONG count = InterlockedDecrement(&refCount_);
  if (count == 0) delete this;
  return count;
}

STDMETHODIMP GoxviDisplayAttributeInfo::GetGUID(GUID* guid) {
  if (!guid) return E_INVALIDARG;
  *guid = GUID_GoxviDisplayAttribute;
  return S_OK;
}

STDMETHODIMP GoxviDisplayAttributeInfo::GetDescription(BSTR* description) {
  if (!description) return E_INVALIDARG;
  *description = SysAllocString(L"Goxvi hidden composition");
  return *description ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP GoxviDisplayAttributeInfo::GetAttributeInfo(
    TF_DISPLAYATTRIBUTE* attribute) {
  if (!attribute) return E_INVALIDARG;
  *attribute = {};  // all defaults: TF_CT_NONE colors, TF_LS_NONE — no underline
  attribute->crText.type = TF_CT_NONE;
  attribute->crBk.type = TF_CT_NONE;
  attribute->crLine.type = TF_CT_NONE;
  attribute->lsStyle = TF_LS_NONE;
  attribute->fBoldLine = FALSE;
  attribute->bAttr = TF_ATTR_INPUT;
  return S_OK;
}

STDMETHODIMP GoxviDisplayAttributeInfo::SetAttributeInfo(
    const TF_DISPLAYATTRIBUTE* /*attribute*/) {
  return E_NOTIMPL;
}

STDMETHODIMP GoxviDisplayAttributeInfo::Reset() { return S_OK; }

// ---- GoxviEnumDisplayAttributeInfo -------------------------------------------

STDMETHODIMP GoxviEnumDisplayAttributeInfo::QueryInterface(REFIID riid,
                                                           void** ppv) {
  if (!ppv) return E_INVALIDARG;
  if (riid == IID_IUnknown || riid == IID_IEnumTfDisplayAttributeInfo) {
    *ppv = static_cast<IEnumTfDisplayAttributeInfo*>(this);
    AddRef();
    return S_OK;
  }
  *ppv = nullptr;
  return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) GoxviEnumDisplayAttributeInfo::AddRef() {
  return InterlockedIncrement(&refCount_);
}

STDMETHODIMP_(ULONG) GoxviEnumDisplayAttributeInfo::Release() {
  const LONG count = InterlockedDecrement(&refCount_);
  if (count == 0) delete this;
  return count;
}

STDMETHODIMP GoxviEnumDisplayAttributeInfo::Clone(
    IEnumTfDisplayAttributeInfo** enumInfo) {
  if (!enumInfo) return E_INVALIDARG;
  auto* clone = new (std::nothrow) GoxviEnumDisplayAttributeInfo();
  if (!clone) return E_OUTOFMEMORY;
  clone->index_ = index_;
  *enumInfo = clone;
  return S_OK;
}

STDMETHODIMP GoxviEnumDisplayAttributeInfo::Next(ULONG count,
                                                 ITfDisplayAttributeInfo** info,
                                                 ULONG* fetched) {
  if (fetched) *fetched = 0;
  if (!info) return E_INVALIDARG;
  ULONG got = 0;
  if (count > 0 && index_ == 0) {
    auto* item = new (std::nothrow) GoxviDisplayAttributeInfo();
    if (!item) return E_OUTOFMEMORY;
    info[0] = item;
    index_ = 1;
    got = 1;
  }
  if (fetched) *fetched = got;
  return got == count ? S_OK : S_FALSE;
}

STDMETHODIMP GoxviEnumDisplayAttributeInfo::Reset() {
  index_ = 0;
  return S_OK;
}

STDMETHODIMP GoxviEnumDisplayAttributeInfo::Skip(ULONG count) {
  index_ += count;
  return index_ <= 1 ? S_OK : S_FALSE;
}

// ---- GoxviTextService: ITfDisplayAttributeProvider ----------------------------

STDMETHODIMP GoxviTextService::EnumDisplayAttributeInfo(
    IEnumTfDisplayAttributeInfo** enumInfo) {
  if (!enumInfo) return E_INVALIDARG;
  auto* enumerator = new (std::nothrow) GoxviEnumDisplayAttributeInfo();
  if (!enumerator) return E_OUTOFMEMORY;
  *enumInfo = enumerator;
  return S_OK;
}

STDMETHODIMP GoxviTextService::GetDisplayAttributeInfo(
    REFGUID guid, ITfDisplayAttributeInfo** info) {
  if (!info) return E_INVALIDARG;
  *info = nullptr;
  if (guid != GUID_GoxviDisplayAttribute) return E_INVALIDARG;
  auto* item = new (std::nothrow) GoxviDisplayAttributeInfo();
  if (!item) return E_OUTOFMEMORY;
  *info = item;
  return S_OK;
}
