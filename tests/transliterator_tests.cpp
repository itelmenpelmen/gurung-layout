#include "../src/core/GurungTransliterator.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct Case {
    const char* input;
    const wchar_t* expected;
};

bool CheckEqual(const std::wstring& actual, const std::wstring& expected, const char* input) {
    if (actual == expected) {
        return true;
    }
    std::wcerr << L"FAIL input=" << gurung::ToWideAscii(input)
               << L" expected=[" << expected << L"] actual=[" << actual << L"]\n";
    return false;
}

}  // namespace

int main() {
    gurung::Transliterator transliterator;
    int failures = 0;

    const std::vector<Case> devanagari_cases = {
        {"kri", L"क्रि"},
        {"tamu", L"तमु"},
        {"phodzo", L"फोजो"},
        {"tsa", L"च"},
        {"tsah", L"च्ह"},
        {"mih", L"म्हि"},
        {"kli", L"क्लि"},
        {"sya", L"स्या"},
        {"neme", L"नेमे"},
        {"nwara", L"न्वार"},
        {"k/ri", L"क्रि"},
        {"k~", L"कँ"},
        {"ka~", L"काँ"},
        {"mlo~gu", L"म्लोँगु"},
        {"q", L"ट"},
        {"Q", L"ठ"},
        {"x", L"ड"},
        {"X", L"ढ"},
    };

    for (const auto& test : devanagari_cases) {
        const auto candidates = transliterator.GetCandidates(test.input, gurung::Script::Devanagari);
        const std::wstring actual = candidates.empty()
            ? transliterator.Transliterate(test.input, gurung::Script::Devanagari)
            : candidates.front().text;
        if (!CheckEqual(actual, test.expected, test.input)) {
            ++failures;
        }
    }

    const std::vector<Case> latin_cases = {
        {"a.", L"ạ"},
        {"t.", L"ṭ"},
        {"d.", L"ḍ"},
        {"a~", L"ã"},
        {"i~", L"ĩ"},
        {"u~", L"ũ"},
    };

    for (const auto& test : latin_cases) {
        const auto actual = transliterator.Transliterate(test.input, gurung::Script::Latin);
        if (!CheckEqual(actual, test.expected, test.input)) {
            ++failures;
        }
    }

    if (transliterator.ApplyDeadKey(gurung::DeadKey::Nasal, L'a', gurung::Script::Latin) != L"ã") {
        std::wcerr << L"FAIL Ctrl+Shift+N + a Latin dead key\n";
        ++failures;
    }
    if (transliterator.ApplyDeadKey(gurung::DeadKey::Underdot, L't', gurung::Script::Latin) != L"ṭ") {
        std::wcerr << L"FAIL Ctrl+Shift+D + t Latin dead key\n";
        ++failures;
    }
    if (transliterator.ApplyDeadKey(gurung::DeadKey::Nasal, L'a', gurung::Script::Devanagari) != L"आँ") {
        std::wcerr << L"FAIL Ctrl+Shift+N + a Devanagari dead key\n";
        ++failures;
    }
    if (transliterator.ApplyDeadKey(gurung::DeadKey::Underdot, L't', gurung::Script::Devanagari) != L"ट") {
        std::wcerr << L"FAIL Ctrl+Shift+D + t Devanagari dead key\n";
        ++failures;
    }

    const auto ambiguous = transliterator.GetCandidates("khi", gurung::Script::Devanagari);
    bool saw_khi = false;
    bool saw_breathy = false;
    for (const auto& candidate : ambiguous) {
        saw_khi = saw_khi || candidate.text == L"खि";
        saw_breathy = saw_breathy || candidate.text == L"क्हि";
    }
    if (!saw_khi || !saw_breathy) {
        std::wcerr << L"FAIL ambiguous khi should include खि and क्हि\n";
        ++failures;
    }

    if (failures != 0) {
        std::wcerr << failures << L" test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::wcout << L"All Gurung transliterator tests passed\n";
    return EXIT_SUCCESS;
}
