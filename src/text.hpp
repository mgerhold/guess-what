#pragma once

#include <lib2k/utf8/string.hpp>
#include <filesystem>
#include "terminal.hpp"

class Text final {
private:
    std::vector<c2k::Utf8String> m_lines;

public:
    explicit Text(std::filesystem::path const& path);
    void print(Terminal& terminal) const;
};
