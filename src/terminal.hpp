#pragma once

#include <atomic>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <lib2k/utf8/string_view.hpp>

enum class TextColor {
    Black = 30,
    Red = 31,
    Green = 32,
    Yellow = 33,
    Blue = 34,
    Magenta = 35,
    Cyan = 36,
    White = 37,
    BrightBlack = 90,
    BrightRed = 91,
    BrightGreen = 92,
    BrightYellow = 93,
    BrightBlue = 94,
    BrightMagenta = 95,
    BrightCyan = 96,
    BrightWhite = 97,
};

enum class BackgroundColor {
    Black = 40,
    Red = 41,
    Green = 42,
    Yellow = 43,
    Blue = 44,
    Magenta = 45,
    Cyan = 46,
    White = 47,
    BrightBlack = 100,
    BrightRed = 101,
    BrightGreen = 102,
    BrightYellow = 103,
    BrightBlue = 104,
    BrightMagenta = 105,
    BrightCyan = 106,
    BrightWhite = 107,
};

class Terminal final {
    static inline std::atomic_bool s_initialized = false;
    int m_width = 0;
    int m_height = 0;
    std::vector<std::string> m_buffer;

public:
    Terminal();

    Terminal(Terminal const& other) = delete;
    Terminal(Terminal&& other) noexcept = delete;
    Terminal& operator=(Terminal const& other) = delete;
    Terminal& operator=(Terminal&& other) noexcept = delete;

    ~Terminal() noexcept;

    [[nodiscard]] int width() const {
        return m_width;
    }

    [[nodiscard]] int height() const {
        return m_height;
    }

    void clear();
    void set_position(int x, int y);
    void print(c2k::Utf8StringView text);
    void println();
    void println(c2k::Utf8StringView text);
    [[nodiscard]] c2k::Utf8String read_line();
    void hide_cursor();
    void show_cursor();
    void set_text_color(TextColor color);
    void set_background_color(BackgroundColor color);
    void reset_colors();
    void fill_background(BackgroundColor color);

private:
    void enter_alternate_screen_buffer();
    void exit_alternate_screen_buffer();
    [[nodiscard]] bool is_valid_position(int x, int y) const;
};
