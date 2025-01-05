#include "text.hpp"
#include <lib2k/utf8/string_view.hpp>
#include "utils.hpp"

Text::Text(std::filesystem::path const& path) {
    auto const text = read_file(path);
    auto const lines = text.split("\n");
    for (auto const& line : lines) {
        auto const trimmed = trim(c2k::Utf8StringView{ line });
        if (not trimmed.is_empty()) {
            if (trimmed.front() == '#') {
                m_lines.emplace_back(trim(trimmed.substring(1)), Formatting::Headline);
                continue;
            }
            if (trimmed.front() == '>') {
                m_lines.emplace_back(trim(trimmed.substring(1)), Formatting::Paragraph);
                continue;
            }
        }
        m_lines.emplace_back(line, Formatting::Regular);
    }
}

void Text::print(Terminal& terminal) const {
    terminal.clear();
    auto y = -1;
    auto const width = terminal.width();
    auto const height = terminal.height();
    for (auto const& [text, formatting] : m_lines) {
        auto x = 0;
        ++y;
        set_color(terminal, formatting);
        if (formatting == Formatting::Paragraph) {
            x += 2;
        }
        auto const words = text.split(" ");
        for (auto const& word : words) {
            auto const word_length = static_cast<int>(word.calculate_char_width());
            auto const remaining = width - x;
            if (word_length > remaining) {
                x = 0;
                ++y;
            }
            if (y >= height) {
                break;
            }
            terminal.set_position(x, y);
            terminal.print(word);
            x += word_length + 1;
        }
    }
    terminal.reset_colors();
    terminal.println();
}

void Text::set_color(Terminal& terminal, Formatting const formatting) {
    switch (formatting) {
        case Formatting::Headline:
            terminal.set_text_color(TextColor::BrightWhite);
            terminal.set_background_color(BackgroundColor::Black);
            break;
        case Formatting::Paragraph:
            terminal.set_text_color(TextColor::White);
            terminal.set_background_color(BackgroundColor::Black);
            break;
        case Formatting::Regular:
            terminal.set_text_color(TextColor::White);
            terminal.set_background_color(BackgroundColor::Black);
            break;
    }
}
