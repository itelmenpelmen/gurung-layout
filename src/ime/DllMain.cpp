#include "ComSupport.h"
#include "Guids.h"
#include "TextService.h"

#include <Windows.h>
#include <msctf.h>

#include <strsafe.h>

const CLSID CLSID_GurungTextService =
    {0xfaec539d, 0x4867, 0x4342, {0x8e, 0x56, 0x99, 0x53, 0xa9, 0xef, 0x17, 0x01}};
const GUID GUID_GurungProfile =
    {0x8e110837, 0xe540, 0x44af, {0xa8, 0xb4, 0x48, 0xed, 0x7d, 0x80, 0x8b, 0x65}};
const GUID GUID_GurungDisplayAttribute =
    {0x1cd12331, 0x52a0, 0x4a21, {0xa0, 0xda, 0xcc, 0x7c, 0x2a, 0xc6, 0xd4, 0x34}};
const GUID GUID_GurungModePreservedKey =
    {0xcc9dfec3, 0x9c40, 0x4394, {0xa6, 0xa2, 0xb2, 0x03, 0xe6, 0xfd, 0x66, 0x4f}};

namespace {

HMODULE g_module = nullptr;
LONG g_object_count = 0;
LONG g_lock_count = 0;

class ClassFactory final : public IClassFactory {
public:
    ClassFactory() {
        DllAddRef();
    }

    ~ClassFactory() {
        DllRelease();
    }

    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) override {
        if (ppvObject == nullptr) {
            return E_INVALIDARG;
        }
        *ppvObject = nullptr;
        if (riid == IID_IUnknown || riid == IID_IClassFactory) {
            *ppvObject = static_cast<IClassFactory*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef() override {
        return static_cast<ULONG>(InterlockedIncrement(&ref_count_));
    }

    STDMETHODIMP_(ULONG) Release() override {
        const LONG count = InterlockedDecrement(&ref_count_);
        if (count == 0) {
            delete this;
        }
        return static_cast<ULONG>(count);
    }

    STDMETHODIMP CreateInstance(IUnknown* outer, REFIID riid, void** ppvObject) override {
        if (ppvObject == nullptr) {
            return E_INVALIDARG;
        }
        *ppvObject = nullptr;
        if (outer != nullptr) {
            return CLASS_E_NOAGGREGATION;
        }

        auto* service = new GurungTextService();
        const HRESULT hr = service->QueryInterface(riid, ppvObject);
        service->Release();
        return hr;
    }

    STDMETHODIMP LockServer(BOOL lock) override {
        if (lock) {
            InterlockedIncrement(&g_lock_count);
        } else {
            InterlockedDecrement(&g_lock_count);
        }
        return S_OK;
    }

private:
    LONG ref_count_ = 1;
};

HRESULT GuidToString(REFGUID guid, wchar_t* buffer, size_t buffer_count) {
    return StringFromGUID2(guid, buffer, static_cast<int>(buffer_count)) > 0 ? S_OK : E_FAIL;
}

HRESULT SetRegString(HKEY key, const wchar_t* value_name, const wchar_t* value) {
    const DWORD bytes = static_cast<DWORD>((wcslen(value) + 1) * sizeof(wchar_t));
    return RegSetValueExW(key, value_name, 0, REG_SZ, reinterpret_cast<const BYTE*>(value), bytes) == ERROR_SUCCESS
        ? S_OK
        : E_FAIL;
}

HRESULT RegisterComServer() {
    wchar_t module_path[MAX_PATH] = {};
    if (GetModuleFileNameW(g_module, module_path, ARRAYSIZE(module_path)) == 0) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    wchar_t clsid_string[64] = {};
    HRESULT hr = GuidToString(CLSID_GurungTextService, clsid_string, ARRAYSIZE(clsid_string));
    if (FAILED(hr)) {
        return hr;
    }

    wchar_t key_path[128] = {};
    hr = StringCchPrintfW(key_path, ARRAYSIZE(key_path), L"CLSID\\%s", clsid_string);
    if (FAILED(hr)) {
        return hr;
    }

    HKEY clsid_key = nullptr;
    LONG reg = RegCreateKeyExW(HKEY_CLASSES_ROOT, key_path, 0, nullptr, 0, KEY_WRITE, nullptr, &clsid_key, nullptr);
    if (reg != ERROR_SUCCESS) {
        return HRESULT_FROM_WIN32(reg);
    }
    SetRegString(clsid_key, nullptr, kTextServiceDescription);

    HKEY inproc_key = nullptr;
    reg = RegCreateKeyExW(clsid_key, L"InprocServer32", 0, nullptr, 0, KEY_WRITE, nullptr, &inproc_key, nullptr);
    if (reg == ERROR_SUCCESS) {
        SetRegString(inproc_key, nullptr, module_path);
        SetRegString(inproc_key, L"ThreadingModel", kTextServiceModel);
        RegCloseKey(inproc_key);
    }
    RegCloseKey(clsid_key);
    return reg == ERROR_SUCCESS ? S_OK : HRESULT_FROM_WIN32(reg);
}

HRESULT UnregisterComServer() {
    wchar_t clsid_string[64] = {};
    HRESULT hr = GuidToString(CLSID_GurungTextService, clsid_string, ARRAYSIZE(clsid_string));
    if (FAILED(hr)) {
        return hr;
    }
    wchar_t key_path[128] = {};
    hr = StringCchPrintfW(key_path, ARRAYSIZE(key_path), L"CLSID\\%s", clsid_string);
    if (FAILED(hr)) {
        return hr;
    }
    RegDeleteTreeW(HKEY_CLASSES_ROOT, key_path);
    return S_OK;
}

HRESULT RegisterTextServiceProfile() {
    ITfInputProcessorProfileMgr* profile_mgr = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_ITfInputProcessorProfileMgr, reinterpret_cast<void**>(&profile_mgr));
    if (SUCCEEDED(hr) && profile_mgr != nullptr) {
        profile_mgr->UnregisterProfile(CLSID_GurungTextService, 0, GUID_GurungProfile, TF_URP_ALLPROFILES);
    }

    ITfCategoryMgr* category_mgr = nullptr;
    hr = CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER,
                          IID_ITfCategoryMgr, reinterpret_cast<void**>(&category_mgr));
    if (SUCCEEDED(hr) && category_mgr != nullptr) {
        category_mgr->RegisterCategory(CLSID_GurungTextService, GUID_TFCAT_TIP_KEYBOARD, CLSID_GurungTextService);
        category_mgr->Release();
    }

    if (profile_mgr != nullptr) {
        hr = profile_mgr->RegisterProfile(CLSID_GurungTextService, kGurungLangId, GUID_GurungProfile,
                                          kTextServiceDescription,
                                          static_cast<ULONG>(wcslen(kTextServiceDescription)),
                                          nullptr, 0, 0, nullptr, 0, TRUE, 0);
        profile_mgr->Release();
        return hr;
    }

    ITfInputProcessorProfiles* profiles = nullptr;
    hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                          IID_ITfInputProcessorProfiles, reinterpret_cast<void**>(&profiles));
    if (FAILED(hr) || profiles == nullptr) {
        return hr;
    }

    profiles->Unregister(CLSID_GurungTextService);

    hr = profiles->Register(CLSID_GurungTextService);
    if (SUCCEEDED(hr)) {
        hr = profiles->AddLanguageProfile(CLSID_GurungTextService, kGurungLangId, GUID_GurungProfile,
                                          kTextServiceDescription,
                                          static_cast<ULONG>(wcslen(kTextServiceDescription)),
                                          nullptr, 0, 0);
    }

    profiles->Release();
    return hr;
}

HRESULT UnregisterTextServiceProfile() {
    ITfCategoryMgr* category_mgr = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_ITfCategoryMgr, reinterpret_cast<void**>(&category_mgr));
    if (SUCCEEDED(hr) && category_mgr != nullptr) {
        category_mgr->UnregisterCategory(CLSID_GurungTextService, GUID_TFCAT_TIP_KEYBOARD, CLSID_GurungTextService);
        category_mgr->Release();
    }

    ITfInputProcessorProfileMgr* profile_mgr = nullptr;
    hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                          IID_ITfInputProcessorProfileMgr, reinterpret_cast<void**>(&profile_mgr));
    if (SUCCEEDED(hr) && profile_mgr != nullptr) {
        profile_mgr->UnregisterProfile(CLSID_GurungTextService, 0, GUID_GurungProfile, TF_URP_ALLPROFILES);
        profile_mgr->Release();
    }

    ITfInputProcessorProfiles* profiles = nullptr;
    hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                          IID_ITfInputProcessorProfiles, reinterpret_cast<void**>(&profiles));
    if (FAILED(hr) || profiles == nullptr) {
        return hr;
    }
    profiles->Unregister(CLSID_GurungTextService);
    profiles->Release();
    return S_OK;
}

}  // namespace

void DllAddRef() {
    InterlockedIncrement(&g_object_count);
}

void DllRelease() {
    InterlockedDecrement(&g_object_count);
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID /*reserved*/) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_module = module;
        DisableThreadLibraryCalls(module);
    }
    return TRUE;
}

STDAPI DllCanUnloadNow() {
    return (g_object_count == 0 && g_lock_count == 0) ? S_OK : S_FALSE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppvObject) {
    if (rclsid != CLSID_GurungTextService) {
        return CLASS_E_CLASSNOTAVAILABLE;
    }
    auto* factory = new ClassFactory();
    const HRESULT hr = factory->QueryInterface(riid, ppvObject);
    factory->Release();
    return hr;
}

STDAPI DllRegisterServer() {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool co_initialized = SUCCEEDED(hr);
    if (hr == RPC_E_CHANGED_MODE) {
        co_initialized = false;
        hr = S_OK;
    }
    if (FAILED(hr)) {
        return hr;
    }

    hr = RegisterComServer();
    if (SUCCEEDED(hr)) {
        hr = RegisterTextServiceProfile();
    }

    if (co_initialized) {
        CoUninitialize();
    }
    return hr;
}

STDAPI DllUnregisterServer() {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool co_initialized = SUCCEEDED(hr);
    if (hr == RPC_E_CHANGED_MODE) {
        co_initialized = false;
        hr = S_OK;
    }

    if (SUCCEEDED(hr)) {
        UnregisterTextServiceProfile();
        UnregisterComServer();
    }

    if (co_initialized) {
        CoUninitialize();
    }
    return S_OK;
}
