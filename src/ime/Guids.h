#pragma once

#include <Windows.h>
#include <guiddef.h>

extern const CLSID CLSID_GurungTextService;
extern const GUID GUID_GurungProfile;
extern const GUID GUID_GurungDisplayAttribute;
extern const GUID GUID_GurungModePreservedKey;

constexpr LANGID kGurungLangId = 0x2400;  // Windows custom host language: und-Latn.
constexpr wchar_t kTextServiceDescription[] = L"Gurung Scientific IME";
constexpr wchar_t kTextServiceModel[] = L"Apartment";
constexpr wchar_t kInstallSubdir[] = L"Berkeley Computer\\GurungScientificIME";
