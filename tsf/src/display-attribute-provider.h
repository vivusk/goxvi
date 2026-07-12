#pragma once

#include <msctf.h>

// "Hidden composition" display attribute: every field default — no underline,
// no colors. Apps render the composition exactly like committed text.
class GoxviDisplayAttributeInfo : public ITfDisplayAttributeInfo {
 public:
  // IUnknown
  STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
  STDMETHODIMP_(ULONG) AddRef() override;
  STDMETHODIMP_(ULONG) Release() override;

  // ITfDisplayAttributeInfo
  STDMETHODIMP GetGUID(GUID* guid) override;
  STDMETHODIMP GetDescription(BSTR* description) override;
  STDMETHODIMP GetAttributeInfo(TF_DISPLAYATTRIBUTE* attribute) override;
  STDMETHODIMP SetAttributeInfo(const TF_DISPLAYATTRIBUTE* attribute) override;
  STDMETHODIMP Reset() override;

 private:
  LONG refCount_ = 1;
};

class GoxviEnumDisplayAttributeInfo : public IEnumTfDisplayAttributeInfo {
 public:
  // IUnknown
  STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
  STDMETHODIMP_(ULONG) AddRef() override;
  STDMETHODIMP_(ULONG) Release() override;

  // IEnumTfDisplayAttributeInfo
  STDMETHODIMP Clone(IEnumTfDisplayAttributeInfo** enumInfo) override;
  STDMETHODIMP Next(ULONG count, ITfDisplayAttributeInfo** info,
                    ULONG* fetched) override;
  STDMETHODIMP Reset() override;
  STDMETHODIMP Skip(ULONG count) override;

 private:
  LONG refCount_ = 1;
  ULONG index_ = 0;  // single element
};
