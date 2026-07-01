#include "GurungTransliterator.h"

#include <algorithm>
#include <cwctype>
#include <optional>

namespace gurung {
namespace {

constexpr wchar_t kVirama[] = L"्";
constexpr wchar_t kChandrabindu[] = L"ँ";
constexpr wchar_t kCombiningDotBelow[] = L"\u0323";
constexpr wchar_t kCombiningTilde[] = L"\u0303";

struct ConsonantRule {
    const char* latin;
    const wchar_t* devanagari;
    const char* aliases[5];
};

struct VowelRule {
    const char* latin;
    const wchar_t* independent;
    const wchar_t* matra;
    const char* aliases[5];
};

struct LexiconEntry {
    const char* latin_key;
    const wchar_t* devanagari;
    const wchar_t* canonical_latin;
    const wchar_t* gloss;
    int priority;
};

constexpr ConsonantRule kConsonants[] = {
    {"tsh", L"छ", {"tsh", "ch", "C", nullptr, nullptr}},
    {"dzh", L"झ", {"dzh", "jh", "J", nullptr, nullptr}},
    {"t.h", L"ठ", {"t.h", "qh", "Q", nullptr, nullptr}},
    {"d.h", L"ढ", {"d.h", "xh", "X", nullptr, nullptr}},
    {"kh", L"ख", {"kh", "K", nullptr, nullptr, nullptr}},
    {"gh", L"घ", {"gh", "G", nullptr, nullptr, nullptr}},
    {"ng", L"ङ", {"ng", "<", nullptr, nullptr, nullptr}},
    {"ts", L"च", {"ts", "c", nullptr, nullptr, nullptr}},
    {"dz", L"ज", {"dz", "j", nullptr, nullptr, nullptr}},
    {"ṭh", L"ठ", {"ṭh", nullptr, nullptr, nullptr, nullptr}},
    {"ḍh", L"ढ", {"ḍh", nullptr, nullptr, nullptr, nullptr}},
    {"th", L"थ", {"th", "T", nullptr, nullptr, nullptr}},
    {"dh", L"ध", {"dh", "D", nullptr, nullptr, nullptr}},
    {"ph", L"फ", {"ph", "P", "f", nullptr, nullptr}},
    {"bh", L"भ", {"bh", "B", nullptr, nullptr, nullptr}},
    {"ṭ", L"ट", {"ṭ", "t.", "q", nullptr, nullptr}},
    {"ḍ", L"ड", {"ḍ", "d.", "x", nullptr, nullptr}},
    {"k", L"क", {"k", nullptr, nullptr, nullptr, nullptr}},
    {"g", L"ग", {"g", nullptr, nullptr, nullptr, nullptr}},
    {"t", L"त", {"t", nullptr, nullptr, nullptr, nullptr}},
    {"d", L"द", {"d", nullptr, nullptr, nullptr, nullptr}},
    {"n", L"न", {"n", nullptr, nullptr, nullptr, nullptr}},
    {"p", L"प", {"p", nullptr, nullptr, nullptr, nullptr}},
    {"b", L"ब", {"b", nullptr, nullptr, nullptr, nullptr}},
    {"m", L"म", {"m", nullptr, nullptr, nullptr, nullptr}},
    {"y", L"य", {"y", nullptr, nullptr, nullptr, nullptr}},
    {"r", L"र", {"r", nullptr, nullptr, nullptr, nullptr}},
    {"l", L"ल", {"l", nullptr, nullptr, nullptr, nullptr}},
    {"w", L"व", {"w", "v", nullptr, nullptr, nullptr}},
    {"s", L"स", {"s", nullptr, nullptr, nullptr, nullptr}},
    {"h", L"ह", {"h", nullptr, nullptr, nullptr, nullptr}},
};

constexpr VowelRule kVowels[] = {
    {"ạ", L"अ", L"", {"a.", "H", nullptr, nullptr, nullptr}},
    {"ao", L"औ", L"ौ", {"ao", "au", "W", nullptr, nullptr}},
    {"ai", L"ऐ", L"ै", {"ai", "E", nullptr, nullptr, nullptr}},
    {"ii", L"ई", L"ी", {"ii", "I", nullptr, nullptr, nullptr}},
    {"uu", L"ऊ", L"ू", {"uu", "U", nullptr, nullptr, nullptr}},
    {"a", L"आ", L"ा", {"a", "A", nullptr, nullptr, nullptr}},
    {"i", L"इ", L"ि", {"i", nullptr, nullptr, nullptr, nullptr}},
    {"u", L"उ", L"ु", {"u", nullptr, nullptr, nullptr, nullptr}},
    {"e", L"ए", L"े", {"e", nullptr, nullptr, nullptr, nullptr}},
    {"o", L"ओ", L"ो", {"o", "O", nullptr, nullptr, nullptr}},
};

constexpr LexiconEntry kLexicon[] = {
    {"kri", L"क्रि", L"kri", L"one", 100},
    {"ngyaulo", L"ङ्याउंलो", L"ngyaũlo", L"two", 90},
    {"ngyaulo", L"ङ्यउंलो", L"ngyạũlo", L"two; ạ variant", 80},
    {"saulo", L"सउंलो", L"sạũlo", L"three", 90},
    {"tamu", L"तमु", L"tạmu", L"Tamu/Gurung", 100},
    {"phodzo", L"फोजो", L"phodzo", L"Bhujung", 100},
    {"tsa", L"च", L"tsạ", L"that", 100},
    {"tsah", L"च्ह", L"tsạh", L"son", 100},
    {"tshah", L"छ्ह", L"tshạh", L"son", 95},
    {"mi", L"मि", L"mi", L"eye", 90},
    {"mih", L"म्हि", L"mih", L"person", 100},
    {"khaba", L"खाब", L"khabạ", L"fill", 100},
    {"khaba", L"खाँब", L"khãbạ", L"can", 90},
    {"paba", L"पांब", L"pãba", L"distribute", 95},
    {"pahba", L"प्हांब", L"pãhba", L"wet", 95},
    {"kli", L"क्लि", L"kli", L"stool", 100},
    {"kli", L"क्लिं", L"klĩ", L"snow", 85},
    {"liba", L"लिंब", L"lĩbạ", L"delicious", 95},
    {"lihba", L"ल्हिब", L"lihibạ", L"heavy", 95},
    {"koba", L"कोब", L"kobạ", L"feed", 100},
    {"khoba", L"खोब", L"khobạ", L"feed/like", 95},
    {"kohba", L"क्होब", L"kohbạ", L"understand", 100},
    {"kih", L"क्हि", L"kih", L"you", 95},
    {"kyho", L"क्य्होँ", L"kyhõ", L"you", 90},
    {"sya", L"स्या", L"sya", L"meat", 100},
    {"syo", L"स्यो", L"syo", L"river", 95},
    {"nwara", L"न्वार", L"nwarạ", L"cat", 95},
    {"nayu", L"नयु", L"nạyu", L"dog", 95},
    {"neme", L"नेमे", L"neme", L"bird", 95},
    {"mleba", L"म्लेब", L"mlebạ", L"sample", 90},
    {"mlogu", L"म्लोँगु", L"mlõgu", L"sample", 90},
    {"mlogya", L"म्लोग्या", L"mlogya", L"black", 95},
    {"mlogya", L"म्लोग्याँ", L"mlogyã", L"black", 90},
    {"mru", L"म्रुँ", L"mrũ", L"king", 95},
    {"mrusyo", L"म्रुँस्याे", L"mrũsyo", L"queen", 90},
};

template <typename Rule>
bool AliasMatches(const Rule& rule, std::string_view input, size_t pos, size_t* length) {
    for (const char* alias : rule.aliases) {
        if (alias == nullptr) {
            continue;
        }
        const std::string_view token(alias);
        if (input.substr(pos, token.size()) == token) {
            *length = token.size();
            return true;
        }
    }
    return false;
}

const ConsonantRule* MatchConsonant(std::string_view input, size_t pos, size_t* length) {
    const ConsonantRule* best = nullptr;
    size_t best_length = 0;
    for (const auto& rule : kConsonants) {
        size_t candidate_length = 0;
        if (AliasMatches(rule, input, pos, &candidate_length) && candidate_length > best_length) {
            best = &rule;
            best_length = candidate_length;
        }
    }
    if (best != nullptr) {
        *length = best_length;
    }
    return best;
}

const VowelRule* MatchVowel(std::string_view input, size_t pos, size_t* length) {
    const VowelRule* best = nullptr;
    size_t best_length = 0;
    for (const auto& rule : kVowels) {
        size_t candidate_length = 0;
        if (AliasMatches(rule, input, pos, &candidate_length) && candidate_length > best_length) {
            best = &rule;
            best_length = candidate_length;
        }
    }
    if (best != nullptr) {
        *length = best_length;
    }
    return best;
}

bool IsNasalMarker(std::string_view input, size_t pos, size_t* length) {
    if (pos >= input.size()) {
        return false;
    }
    if (input[pos] == '~' || input[pos] == 'M') {
        *length = 1;
        return true;
    }
    return false;
}

bool IsClusterSecond(const ConsonantRule& rule) {
    const std::string_view latin(rule.latin);
    return latin == "l" || latin == "r" || latin == "y" || latin == "w" ||
           latin == "h" || latin == "dz";
}

std::wstring WithNasalIfPresent(std::wstring base, std::string_view input, size_t* pos) {
    size_t nasal_length = 0;
    if (IsNasalMarker(input, *pos, &nasal_length)) {
        base += kChandrabindu;
        *pos += nasal_length;
    }
    return base;
}

std::wstring RenderBreathyCandidate(const ConsonantRule& c, const VowelRule& v) {
    std::wstring output = c.devanagari;
    output += kVirama;
    output += L"ह";
    output += v.matra;
    return output;
}

void AddUniqueCandidate(std::vector<Candidate>* candidates, Candidate candidate) {
    const auto duplicate = std::find_if(candidates->begin(), candidates->end(), [&](const Candidate& existing) {
        return existing.text == candidate.text && existing.latin == candidate.latin;
    });
    if (duplicate == candidates->end()) {
        candidates->push_back(std::move(candidate));
    }
}

std::wstring ApplyLatinNasal(wchar_t ch) {
    switch (std::towlower(ch)) {
        case L'a': return L"ã";
        case L'e': return L"ẽ";
        case L'i': return L"ĩ";
        case L'o': return L"õ";
        case L'u': return L"ũ";
        default: {
            std::wstring out;
            out.push_back(ch);
            out += kCombiningTilde;
            return out;
        }
    }
}

std::wstring ApplyLatinUnderdot(wchar_t ch) {
    switch (std::towlower(ch)) {
        case L'a': return L"ạ";
        case L't': return L"ṭ";
        case L'd': return L"ḍ";
        case L'n': return L"ṇ";
        case L's': return L"ṣ";
        case L'l': return L"ḷ";
        case L'r': return L"ṛ";
        case L'h': return L"ḥ";
        default: {
            std::wstring out;
            out.push_back(ch);
            out += kCombiningDotBelow;
            return out;
        }
    }
}

}  // namespace

std::string NormalizeLatinKey(std::string_view input) {
    std::string normalized;
    normalized.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        const unsigned char ch = static_cast<unsigned char>(input[i]);
        if (ch == 0xCC && i + 1 < input.size()) {
            const unsigned char next = static_cast<unsigned char>(input[i + 1]);
            if (next == 0x83) {
                normalized.push_back('~');
                ++i;
                continue;
            }
            if (next == 0xA3) {
                normalized.push_back('.');
                ++i;
                continue;
            }
        }
        switch (ch) {
            case 0xC3:
                if (i + 1 < input.size()) {
                    const unsigned char next = static_cast<unsigned char>(input[i + 1]);
                    if (next == 0xA3) { normalized.push_back('a'); normalized.push_back('~'); ++i; continue; }
                    if (next == 0xB5) { normalized.push_back('o'); normalized.push_back('~'); ++i; continue; }
                    if (next == 0xA9) { normalized.push_back('e'); ++i; continue; }
                }
                break;
            case 0xC4:
                if (i + 1 < input.size()) {
                    const unsigned char next = static_cast<unsigned char>(input[i + 1]);
                    if (next == 0xA9) { normalized.push_back('i'); normalized.push_back('~'); ++i; continue; }
                    if (next == 0xA8) { normalized.push_back('I'); normalized.push_back('~'); ++i; continue; }
                }
                break;
            case 0xC5:
                if (i + 1 < input.size()) {
                    const unsigned char next = static_cast<unsigned char>(input[i + 1]);
                    if (next == 0xA9) { normalized.push_back('u'); normalized.push_back('~'); ++i; continue; }
                }
                break;
            default:
                break;
        }
        normalized.push_back(static_cast<char>(ch));
    }
    return normalized;
}

std::wstring ToWideAscii(std::string_view input) {
    std::wstring widened;
    widened.reserve(input.size());
    for (char ch : input) {
        widened.push_back(static_cast<unsigned char>(ch));
    }
    return widened;
}

std::wstring Transliterator::Transliterate(std::string_view input, Script script) const {
    if (script == Script::Latin) {
        return TransliterateLatin(input);
    }
    return TransliterateDevanagari(input);
}

std::vector<Candidate> Transliterator::GetCandidates(std::string_view input, Script script) const {
    const std::string key = NormalizeLatinKey(input);
    std::vector<Candidate> candidates;

    if (script == Script::Devanagari) {
        for (const auto& entry : kLexicon) {
            if (key == entry.latin_key) {
                AddUniqueCandidate(&candidates, Candidate{entry.devanagari, entry.canonical_latin, entry.gloss, entry.priority, false});
            }
        }
    }

    const std::wstring direct = Transliterate(key, script);
    AddUniqueCandidate(&candidates, Candidate{direct, TransliterateLatin(key), L"", 50, false});

    if (script == Script::Devanagari) {
        // Breathy ambiguity: Gurung examples contain both C+h+V and C+V+h
        // input habits for outputs such as क्हि. Expose them as candidates.
        for (const auto& base_consonant : kConsonants) {
            size_t c_len = 0;
            if (!AliasMatches(base_consonant, key, 0, &c_len)) {
                continue;
            }

            if (key.size() > c_len && key[c_len] == 'h') {
                size_t v_len = 0;
                const VowelRule* v = MatchVowel(key, c_len + 1, &v_len);
                if (v != nullptr && c_len + 1 + v_len == key.size()) {
                    AddUniqueCandidate(&candidates, Candidate{RenderBreathyCandidate(base_consonant, *v), L"", L"breathy consonant candidate", 45, true});
                }
            }

            size_t v_len = 0;
            const VowelRule* v = MatchVowel(key, c_len, &v_len);
            if (v != nullptr && c_len + v_len < key.size() && key[c_len + v_len] == 'h' &&
                c_len + v_len + 1 == key.size()) {
                AddUniqueCandidate(&candidates, Candidate{RenderBreathyCandidate(base_consonant, *v), L"", L"breathy consonant candidate", 45, true});
            }
        }

        std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
            return a.priority > b.priority;
        });
        for (auto& candidate : candidates) {
            candidate.ambiguous = candidates.size() > 1;
        }
    }

    return candidates;
}

std::wstring Transliterator::ApplyDeadKey(DeadKey dead_key, wchar_t ch, Script script) const {
    if (dead_key == DeadKey::None) {
        return std::wstring(1, ch);
    }
    if (script == Script::Latin) {
        return dead_key == DeadKey::Nasal ? ApplyLatinNasal(ch) : ApplyLatinUnderdot(ch);
    }

    if (dead_key == DeadKey::Nasal) {
        if (ch == L'a') return L"आँ";
        if (ch == L'e') return L"एँ";
        if (ch == L'i') return L"इँ";
        if (ch == L'o') return L"ओँ";
        if (ch == L'u') return L"उँ";
        std::wstring out(1, ch);
        out += kChandrabindu;
        return out;
    }

    if (ch == L'a') return L"अ";
    if (ch == L't') return L"ट";
    if (ch == L'd') return L"ड";
    std::wstring out(1, ch);
    return out;
}

std::wstring Transliterator::TransliterateDevanagari(std::string_view raw_input) const {
    const std::string input = NormalizeLatinKey(raw_input);
    std::wstring output;

    for (size_t pos = 0; pos < input.size();) {
        if (input[pos] == '/') {
            output += kVirama;
            ++pos;
            continue;
        }

        size_t consonant_length = 0;
        const ConsonantRule* consonant = MatchConsonant(input, pos, &consonant_length);
        if (consonant != nullptr) {
            const size_t after_consonant = pos + consonant_length;

            if (after_consonant < input.size() && input[after_consonant] == '/') {
                output += consonant->devanagari;
                output += kVirama;
                pos = after_consonant + 1;
                continue;
            }

            size_t vowel_length = 0;
            const VowelRule* vowel = MatchVowel(input, after_consonant, &vowel_length);
            if (vowel != nullptr) {
                output += consonant->devanagari;
                output += vowel->matra;
                pos = after_consonant + vowel_length;
                output = WithNasalIfPresent(std::move(output), input, &pos);
                continue;
            }

            size_t next_consonant_length = 0;
            const ConsonantRule* next_consonant = MatchConsonant(input, after_consonant, &next_consonant_length);
            if (next_consonant != nullptr && IsClusterSecond(*next_consonant)) {
                output += consonant->devanagari;
                output += kVirama;
                pos = after_consonant;
                continue;
            }

            output += consonant->devanagari;
            pos = after_consonant;
            output = WithNasalIfPresent(std::move(output), input, &pos);
            continue;
        }

        size_t vowel_length = 0;
        const VowelRule* vowel = MatchVowel(input, pos, &vowel_length);
        if (vowel != nullptr) {
            output += vowel->independent;
            pos += vowel_length;
            output = WithNasalIfPresent(std::move(output), input, &pos);
            continue;
        }

        size_t nasal_length = 0;
        if (IsNasalMarker(input, pos, &nasal_length)) {
            output += kChandrabindu;
            pos += nasal_length;
            continue;
        }

        output.push_back(static_cast<unsigned char>(input[pos]));
        ++pos;
    }

    return output;
}

std::wstring Transliterator::TransliterateLatin(std::string_view raw_input) const {
    const std::string input = NormalizeLatinKey(raw_input);
    std::wstring output;
    output.reserve(input.size());

    for (size_t pos = 0; pos < input.size();) {
        if (pos + 1 < input.size() && input[pos + 1] == '~') {
            output += ApplyLatinNasal(static_cast<unsigned char>(input[pos]));
            pos += 2;
            continue;
        }
        if (pos + 1 < input.size() && input[pos + 1] == '.') {
            output += ApplyLatinUnderdot(static_cast<unsigned char>(input[pos]));
            pos += 2;
            continue;
        }
        output.push_back(static_cast<unsigned char>(input[pos]));
        ++pos;
    }

    return output;
}

}  // namespace gurung
