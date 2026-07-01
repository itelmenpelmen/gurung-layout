#include "TextService.h"

#include "ComSupport.h"
#include "EditSession.h"

#include <algorithm>

GurungTextService::GurungTextService() {
    DllAddRef();
}

GurungTextService::~GurungTextService() {
    Deactivate();
    DllRelease();
}

STDMETHODIMP GurungTextService::QueryInterface(REFIID riid, void** ppvObject) {
    if (ppvObject == nullptr) {
        return E_INVALIDARG;
    }
    *ppvObject = nullptr;

    if (riid == IID_IUnknown || riid == IID_ITfTextInputProcessor || riid == IID_ITfTextInputProcessorEx) {
        *ppvObject = static_cast<ITfTextInputProcessorEx*>(this);
    } else if (riid == IID_ITfKeyEventSink) {
        *ppvObject = static_cast<ITfKeyEventSink*>(this);
    } else {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) GurungTextService::AddRef() {
    return static_cast<ULONG>(InterlockedIncrement(&ref_count_));
}

STDMETHODIMP_(ULONG) GurungTextService::Release() {
    const LONG count = InterlockedDecrement(&ref_count_);
    if (count == 0) {
        delete this;
    }
    return static_cast<ULONG>(count);
}

STDMETHODIMP GurungTextService::Activate(ITfThreadMgr* thread_mgr, TfClientId client_id) {
    return ActivateEx(thread_mgr, client_id, 0);
}

STDMETHODIMP GurungTextService::ActivateEx(ITfThreadMgr* thread_mgr, TfClientId client_id, DWORD /*flags*/) {
    if (thread_mgr == nullptr) {
        return E_INVALIDARG;
    }

    thread_mgr_ = thread_mgr;
    thread_mgr_->AddRef();
    client_id_ = client_id;

    ITfKeystrokeMgr* keystroke_mgr = nullptr;
    HRESULT hr = thread_mgr_->QueryInterface(IID_ITfKeystrokeMgr, reinterpret_cast<void**>(&keystroke_mgr));
    if (SUCCEEDED(hr) && keystroke_mgr != nullptr) {
        hr = keystroke_mgr->AdviseKeyEventSink(client_id_, static_cast<ITfKeyEventSink*>(this), TRUE);
        keystroke_mgr->Release();
    }

    return hr;
}

STDMETHODIMP GurungTextService::Deactivate() {
    if (thread_mgr_ != nullptr) {
        ITfKeystrokeMgr* keystroke_mgr = nullptr;
        if (SUCCEEDED(thread_mgr_->QueryInterface(IID_ITfKeystrokeMgr, reinterpret_cast<void**>(&keystroke_mgr))) &&
            keystroke_mgr != nullptr) {
            keystroke_mgr->UnadviseKeyEventSink(client_id_);
            keystroke_mgr->Release();
        }
        thread_mgr_->Release();
        thread_mgr_ = nullptr;
    }

    client_id_ = 0;
    ClearBuffer();
    return S_OK;
}

STDMETHODIMP GurungTextService::OnSetFocus(BOOL /*foreground*/) {
    return S_OK;
}

STDMETHODIMP GurungTextService::OnTestKeyDown(ITfContext* /*context*/, WPARAM wParam, LPARAM /*lParam*/, BOOL* eaten) {
    if (eaten == nullptr) {
        return E_INVALIDARG;
    }
    *eaten = IsHandledKey(wParam) ? TRUE : FALSE;
    return S_OK;
}

STDMETHODIMP GurungTextService::OnTestKeyUp(ITfContext* /*context*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL* eaten) {
    if (eaten != nullptr) {
        *eaten = FALSE;
    }
    return S_OK;
}

STDMETHODIMP GurungTextService::OnKeyDown(ITfContext* context, WPARAM wParam, LPARAM /*lParam*/, BOOL* eaten) {
    if (eaten == nullptr) {
        return E_INVALIDARG;
    }
    *eaten = FALSE;

    if (IsCtrlDown() && !IsAltDown() && wParam == VK_SPACE) {
        if (IsShiftDown()) {
            script_ = script_ == gurung::Script::Devanagari ? gurung::Script::Latin : gurung::Script::Devanagari;
            ClearBuffer();
        } else {
            pass_through_ = !pass_through_;
            ClearBuffer();
        }
        *eaten = TRUE;
        return S_OK;
    }

    if (IsCtrlDown() && IsShiftDown() && !IsAltDown() && wParam == 'N') {
        pending_dead_key_ = gurung::DeadKey::Nasal;
        *eaten = TRUE;
        return S_OK;
    }

    if (IsCtrlDown() && IsShiftDown() && !IsAltDown() && wParam == 'D') {
        pending_dead_key_ = gurung::DeadKey::Underdot;
        *eaten = TRUE;
        return S_OK;
    }

    if (pass_through_) {
        return S_OK;
    }

    if (script_ == gurung::Script::Latin) {
        const auto ch = VirtualKeyToAscii(wParam);
        if (pending_dead_key_ != gurung::DeadKey::None && ch.has_value()) {
            const std::wstring text = transliterator_.ApplyDeadKey(pending_dead_key_, static_cast<wchar_t>(*ch), script_);
            pending_dead_key_ = gurung::DeadKey::None;
            *eaten = TRUE;
            return InsertText(context, text);
        }
        return S_OK;
    }

    if (wParam == VK_BACK) {
        if (buffer_.empty()) {
            return S_OK;
        }
        buffer_.pop_back();
        *eaten = TRUE;
        return RenderBuffer(context);
    }

    if (wParam == VK_ESCAPE) {
        ClearBuffer();
        *eaten = FALSE;
        return S_OK;
    }

    if (wParam == VK_SPACE) {
        if (buffer_.empty()) {
            return S_OK;
        }
        *eaten = TRUE;
        return CommitBuffer(context, std::nullopt);
    }

    if (wParam >= '1' && wParam <= '9' && last_candidates_.size() > 1 && !buffer_.empty()) {
        const size_t index = static_cast<size_t>(wParam - '1');
        if (index < last_candidates_.size()) {
            *eaten = TRUE;
            return CommitBuffer(context, index);
        }
    }

    const auto ch = VirtualKeyToAscii(wParam);
    if (!ch.has_value()) {
        return S_OK;
    }

    if (pending_dead_key_ != gurung::DeadKey::None) {
        if (pending_dead_key_ == gurung::DeadKey::Nasal && !buffer_.empty() &&
            (*ch == 'a' || *ch == 'e' || *ch == 'i' || *ch == 'o' || *ch == 'u')) {
            buffer_.push_back(*ch);
            buffer_.push_back('~');
            pending_dead_key_ = gurung::DeadKey::None;
            *eaten = TRUE;
            return RenderBuffer(context);
        }

        const std::wstring text = transliterator_.ApplyDeadKey(pending_dead_key_, static_cast<wchar_t>(*ch), script_);
        pending_dead_key_ = gurung::DeadKey::None;
        *eaten = TRUE;
        return InsertText(context, text);
    }

    buffer_.push_back(*ch);
    *eaten = TRUE;
    return RenderBuffer(context);
}

STDMETHODIMP GurungTextService::OnKeyUp(ITfContext* /*context*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL* eaten) {
    if (eaten != nullptr) {
        *eaten = FALSE;
    }
    return S_OK;
}

STDMETHODIMP GurungTextService::OnPreservedKey(ITfContext* /*context*/, REFGUID /*guid*/, BOOL* eaten) {
    if (eaten != nullptr) {
        *eaten = FALSE;
    }
    return S_OK;
}

bool GurungTextService::IsCtrlDown() const {
    return (GetKeyState(VK_CONTROL) & 0x8000) != 0;
}

bool GurungTextService::IsShiftDown() const {
    return (GetKeyState(VK_SHIFT) & 0x8000) != 0;
}

bool GurungTextService::IsAltDown() const {
    return (GetKeyState(VK_MENU) & 0x8000) != 0;
}

bool GurungTextService::IsHandledKey(WPARAM wParam) const {
    if (IsCtrlDown() && !IsAltDown() && wParam == VK_SPACE) {
        return true;
    }
    if (IsCtrlDown() && IsShiftDown() && !IsAltDown() && (wParam == 'N' || wParam == 'D')) {
        return true;
    }
    if (pass_through_) {
        return false;
    }
    if (script_ == gurung::Script::Latin) {
        return pending_dead_key_ != gurung::DeadKey::None && VirtualKeyToAscii(wParam).has_value();
    }
    return wParam == VK_BACK || wParam == VK_SPACE || VirtualKeyToAscii(wParam).has_value();
}

std::optional<char> GurungTextService::VirtualKeyToAscii(WPARAM wParam) const {
    const bool shift = IsShiftDown();

    if (wParam >= 'A' && wParam <= 'Z') {
        const char lower = static_cast<char>('a' + (wParam - 'A'));
        return shift ? static_cast<char>(wParam) : lower;
    }

    if (wParam >= '0' && wParam <= '9') {
        return static_cast<char>(wParam);
    }

    switch (wParam) {
        case VK_OEM_2: return shift ? '?' : '/';
        case VK_OEM_PERIOD: return shift ? '>' : '.';
        case VK_OEM_COMMA: return shift ? '<' : ',';
        case VK_OEM_3: return shift ? '~' : '`';
        case VK_OEM_1: return shift ? ':' : ';';
        case VK_OEM_7: return shift ? '"' : '\'';
        case VK_OEM_MINUS: return shift ? '_' : '-';
        case VK_OEM_PLUS: return shift ? '+' : '=';
        default: return std::nullopt;
    }
}

HRESULT GurungTextService::ReplaceComposingText(ITfContext* context, const std::wstring& text, LONG old_units_override) {
    if (context == nullptr) {
        return E_INVALIDARG;
    }
    const LONG old_units = old_units_override >= 0 ? old_units_override : rendered_units_;
    auto* session = new ReplaceTextEditSession(context, old_units, text);
    HRESULT edit_hr = E_FAIL;
    const HRESULT hr = context->RequestEditSession(client_id_, session, TF_ES_SYNC | TF_ES_READWRITE, &edit_hr);
    session->Release();
    if (SUCCEEDED(hr) && SUCCEEDED(edit_hr)) {
        rendered_units_ = static_cast<LONG>(text.size());
        return S_OK;
    }
    return FAILED(hr) ? hr : edit_hr;
}

HRESULT GurungTextService::RenderBuffer(ITfContext* context) {
    if (buffer_.empty()) {
        last_candidates_.clear();
        const HRESULT hr = ReplaceComposingText(context, L"");
        rendered_units_ = 0;
        return hr;
    }

    last_candidates_ = transliterator_.GetCandidates(buffer_, gurung::Script::Devanagari);
    const std::wstring text = last_candidates_.empty()
        ? transliterator_.Transliterate(buffer_, gurung::Script::Devanagari)
        : last_candidates_.front().text;
    return ReplaceComposingText(context, text);
}

HRESULT GurungTextService::CommitBuffer(ITfContext* context, std::optional<size_t> candidate_index) {
    if (buffer_.empty()) {
        return S_OK;
    }

    if (last_candidates_.empty()) {
        last_candidates_ = transliterator_.GetCandidates(buffer_, gurung::Script::Devanagari);
    }

    std::wstring text;
    if (candidate_index.has_value() && *candidate_index < last_candidates_.size()) {
        text = last_candidates_[*candidate_index].text;
    } else if (!last_candidates_.empty()) {
        text = last_candidates_.front().text;
    } else {
        text = transliterator_.Transliterate(buffer_, gurung::Script::Devanagari);
    }
    text += L" ";

    const LONG old_units = rendered_units_;
    ClearBuffer();
    const HRESULT hr = ReplaceComposingText(context, text, old_units);
    rendered_units_ = 0;
    return hr;
}

HRESULT GurungTextService::InsertText(ITfContext* context, const std::wstring& text) {
    const HRESULT hr = ReplaceComposingText(context, text, 0);
    rendered_units_ = 0;
    return hr;
}

void GurungTextService::ClearBuffer() {
    buffer_.clear();
    last_candidates_.clear();
    pending_dead_key_ = gurung::DeadKey::None;
    rendered_units_ = 0;
}
