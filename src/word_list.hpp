#pragma once

#include <lib2k/utf8/string.hpp>
#include <vector>
#include "utils.hpp"

using WordList = std::vector<c2k::Utf8String>;

[[nodiscard]] inline WordList read_word_list(std::filesystem::path const& path) {
    auto const contents = c2k::Utf8String{ read_file(path) };
    auto words = contents.split("\n");
    for (auto& word : words) {
        word = trim(word);
    }
    return words;
}
