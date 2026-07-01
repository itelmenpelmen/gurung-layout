#pragma once

#include <msctf.h>
#include <string>

class ReplaceTextEditSession final : public ITfEditSession {
public:
    ReplaceTextEditSession(ITfContext* context, LONG old_text_units, std::wstring replacement);
    ~ReplaceTextEditSession();

    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;
    STDMETHODIMP DoEditSession(TfEditCookie ec) override;

private:
    LONG ref_count_ = 1;
    ITfContext* context_ = nullptr;
    LONG old_text_units_ = 0;
    std::wstring replacement_;
};
