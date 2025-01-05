#pragma once

#include <lib2k/utf8/string.hpp>
#include <filesystem>
#include "terminal.hpp"

class Text final {
private:
    enum class Formatting {
        Headline,
        Paragraph,
        Regular,
    };

    struct Line final {
        c2k::Utf8String text;
        Formatting formatting;

        Line(c2k::Utf8String text, Formatting const formatting)
            : text{ std::move(text) }, formatting{ formatting } {}
    };

    std::vector<Line> m_lines;

public:
    explicit Text(std::filesystem::path const& path);
    void print(Terminal& terminal) const;

private:
    static void set_color(Terminal& terminal, Formatting formatting);
};
