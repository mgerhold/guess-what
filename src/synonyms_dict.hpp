#pragma once

#include <string>
#include <unordered_map>
#include "word_list.hpp"

class SynonymsDict final {
private:
    static constexpr auto synonyms_directory = "synonyms";

    std::unordered_map<c2k::Utf8String, WordList> m_word_lists;

public:
    SynonymsDict() {
        using DirectoryIterator = std::filesystem::recursive_directory_iterator;
        for (auto const& entry : DirectoryIterator{ synonyms_directory }) {
            if (entry.path().extension() != ".list") {
                continue;
            }
            auto lines = c2k::Utf8String{ read_file(entry.path()) }.split("\n");
            for (auto& line : lines) {
                line = trim(line);
            }
            erase_if(lines, [](auto const& line) { return line.is_empty(); });
            m_word_lists.emplace(entry.path().stem().string(), std::move(lines));
        }
    }

    [[nodiscard]] bool is_synonym_of(c2k::Utf8StringView const word, c2k::Utf8StringView const category) const {
        auto const find_iterator = m_word_lists.find(category);
        if (find_iterator == m_word_lists.cend()) {
            throw std::runtime_error{ "No word list named '" + std::string{ word.view() } + "' found." };
        }
        for (auto const& synonym : find_iterator->second) {
            if (word == synonym) {
                return true;
            }
        }
        return false;
    }
};
