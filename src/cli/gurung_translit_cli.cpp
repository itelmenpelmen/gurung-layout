#include "../core/GurungTransliterator.h"

#include <fcntl.h>
#include <io.h>

#include <iostream>
#include <string>

int main(int argc, char** argv) {
    _setmode(_fileno(stdout), _O_U16TEXT);

    if (argc < 2) {
        std::wcout << L"Usage: gurung-translit.exe [--latin] WORD...\n";
        return 0;
    }

    gurung::Script script = gurung::Script::Devanagari;
    int first_word = 1;
    if (std::string(argv[1]) == "--latin") {
        script = gurung::Script::Latin;
        first_word = 2;
    }

    gurung::Transliterator transliterator;
    for (int i = first_word; i < argc; ++i) {
        const auto candidates = transliterator.GetCandidates(argv[i], script);
        if (!candidates.empty()) {
            std::wcout << candidates.front().text;
        } else {
            std::wcout << transliterator.Transliterate(argv[i], script);
        }
        if (i + 1 < argc) {
            std::wcout << L" ";
        }
    }
    std::wcout << L"\n";
    return 0;
}
