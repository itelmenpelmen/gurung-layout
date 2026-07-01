#pragma once

#include "../core/GurungTransliterator.h"

#include <Windows.h>
#include <msctf.h>

#include <optional>
#include <string>
#include <vector>

class GurungTextService final : public ITfTextInputProcessorEx, public ITfKeyEventSink {
public:
    GurungTextService();
    ~GurungTextService();

    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    STDMETHODIMP Activate(ITfThreadMgr* thread_mgr, TfClientId client_id) override;
    STDMETHODIMP ActivateEx(ITfThreadMgr* thread_mgr, TfClientId client_id, DWORD flags) override;
    STDMETHODIMP Deactivate() override;

    STDMETHODIMP OnSetFocus(BOOL foreground) override;
    STDMETHODIMP OnTestKeyDown(ITfContext* context, WPARAM wParam, LPARAM lParam, BOOL* eaten) override;
    STDMETHODIMP OnTestKeyUp(ITfContext* context, WPARAM wParam, LPARAM lParam, BOOL* eaten) override;
    STDMETHODIMP OnKeyDown(ITfContext* context, WPARAM wParam, LPARAM lParam, BOOL* eaten) override;
    STDMETHODIMP OnKeyUp(ITfContext* context, WPARAM wParam, LPARAM lParam, BOOL* eaten) override;
    STDMETHODIMP OnPreservedKey(ITfContext* context, REFGUID guid, BOOL* eaten) override;

private:
    bool IsCtrlDown() const;
    bool IsShiftDown() const;
    bool IsAltDown() const;
    bool IsHandledKey(WPARAM wParam) const;
    std::optional<char> VirtualKeyToAscii(WPARAM wParam) const;
    HRESULT ReplaceComposingText(ITfContext* context, const std::wstring& text, LONG old_units_override = -1);
    HRESULT RenderBuffer(ITfContext* context);
    HRESULT CommitBuffer(ITfContext* context, std::optional<size_t> candidate_index);
    HRESULT InsertText(ITfContext* context, const std::wstring& text);
    void ClearBuffer();

    LONG ref_count_ = 1;
    ITfThreadMgr* thread_mgr_ = nullptr;
    TfClientId client_id_ = 0;
    gurung::Transliterator transliterator_;
    gurung::Script script_ = gurung::Script::Devanagari;
    gurung::DeadKey pending_dead_key_ = gurung::DeadKey::None;
    bool pass_through_ = false;
    std::string buffer_;
    LONG rendered_units_ = 0;
    std::vector<gurung::Candidate> last_candidates_;
};
