#include "EditSession.h"

#include "ComSupport.h"

#include <Windows.h>

ReplaceTextEditSession::ReplaceTextEditSession(ITfContext* context, LONG old_text_units, std::wstring replacement)
    : context_(context), old_text_units_(old_text_units), replacement_(std::move(replacement)) {
    DllAddRef();
    if (context_ != nullptr) {
        context_->AddRef();
    }
}

ReplaceTextEditSession::~ReplaceTextEditSession() {
    if (context_ != nullptr) {
        context_->Release();
    }
    DllRelease();
}

STDMETHODIMP ReplaceTextEditSession::QueryInterface(REFIID riid, void** ppvObject) {
    if (ppvObject == nullptr) {
        return E_INVALIDARG;
    }
    *ppvObject = nullptr;

    if (riid == IID_IUnknown || riid == IID_ITfEditSession) {
        *ppvObject = static_cast<ITfEditSession*>(this);
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) ReplaceTextEditSession::AddRef() {
    return static_cast<ULONG>(InterlockedIncrement(&ref_count_));
}

STDMETHODIMP_(ULONG) ReplaceTextEditSession::Release() {
    const LONG count = InterlockedDecrement(&ref_count_);
    if (count == 0) {
        delete this;
    }
    return static_cast<ULONG>(count);
}

STDMETHODIMP ReplaceTextEditSession::DoEditSession(TfEditCookie ec) {
    if (context_ == nullptr) {
        return E_FAIL;
    }

    TF_SELECTION selection = {};
    ULONG fetched = 0;
    HRESULT hr = context_->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &selection, &fetched);
    if (FAILED(hr) || fetched == 0 || selection.range == nullptr) {
        return FAILED(hr) ? hr : E_FAIL;
    }

    ITfRange* range = selection.range;
    if (old_text_units_ > 0) {
        LONG shifted = 0;
        range->ShiftStart(ec, -old_text_units_, &shifted, nullptr);
    }

    hr = range->SetText(ec, 0, replacement_.c_str(), static_cast<LONG>(replacement_.size()));
    if (SUCCEEDED(hr)) {
        range->Collapse(ec, TF_ANCHOR_END);
        context_->SetSelection(ec, 1, &selection);
    }

    range->Release();
    return hr;
}
