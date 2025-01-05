#pragma once

#include <filesystem>
#include <unordered_map>
#include "text.hpp"

class TextDatabase final {
private:
    static constexpr auto texts_directory = "texts";
    std::unordered_map<c2k::Utf8String, Text> m_texts;

public:
    TextDatabase() {
        using DirectoryIterator = std::filesystem::recursive_directory_iterator;
        for (auto const& entry : DirectoryIterator{ texts_directory }) {
            if (entry.path().extension() != ".txt") {
                continue;
            }
            m_texts.emplace(entry.path().stem().string(), Text{ entry.path() });
        }
    }

    [[nodiscard]] bool contains(c2k::Utf8StringView const key) const {
        return m_texts.contains(key);
    }

    [[nodiscard]] Text const& get(c2k::Utf8StringView const key) const {
        return m_texts.at(key);
    }
};
