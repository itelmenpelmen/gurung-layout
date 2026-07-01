#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace gurung {

enum class Script {
    Devanagari,
    Latin
};

enum class DeadKey {
    None,
    Nasal,
    Underdot
};

struct Candidate {
    std::wstring text;
    std::wstring latin;
    std::wstring gloss;
    int priority = 0;
    bool ambiguous = false;
};

struct TransformResult {
    std::wstring committed;
    std::wstring composing;
    std::vector<Candidate> candidates;
    Script script = Script::Devanagari;
    DeadKey pending_dead_key = DeadKey::None;
    bool pass_through = false;
};

class Transliterator {
public:
    std::wstring Transliterate(std::string_view input, Script script) const;
    std::vector<Candidate> GetCandidates(std::string_view input, Script script) const;
    std::wstring ApplyDeadKey(DeadKey dead_key, wchar_t ch, Script script) const;

private:
    std::wstring TransliterateDevanagari(std::string_view input) const;
    std::wstring TransliterateLatin(std::string_view input) const;
};

std::string NormalizeLatinKey(std::string_view input);
std::wstring ToWideAscii(std::string_view input);

}  // namespace gurung
