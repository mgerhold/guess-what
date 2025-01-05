#include "text.hpp"
#include <lib2k/utf8/string_view.hpp>
#include "utils.hpp"

Text::Text(std::filesystem::path const& path) {
    auto const text = read_file(path);
    m_lines = text.split("\n");
}

void Text::print(Terminal& terminal) const {
    terminal.clear();
    for (auto const& line : m_lines) {
        terminal.println(line);
    }
    terminal.println();
}
