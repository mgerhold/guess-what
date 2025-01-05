#include "terminal.hpp"
#include <chrono>
#include <iostream>
#include <lib2k/types.hpp>
#include <locale>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>
#ifdef _WIN32
#include "windows.hpp"
#else
inline void setup_terminal() {}
#endif

Terminal::Terminal() {
    auto expected = false;
    if (not s_initialized.compare_exchange_strong(expected, true)) {
        throw std::runtime_error{ "Terminal may only be initialized once." };
    }
    setup_terminal();
    enter_alternate_screen_buffer();
}

Terminal::~Terminal() noexcept {
    show_cursor();
    exit_alternate_screen_buffer();
    s_initialized = false;
}

void Terminal::clear(bool delayed) {
    using namespace std::chrono_literals;
    if (delayed) {
        for (auto i = 0; i < 3; ++i) {
            print_raw(".");
            std::cout << std::flush;
            std::this_thread::sleep_for(200ms);
        }
    }
    std::cout << "\x1b[2J\x1b[H";
}

void Terminal::set_position(int const x, int const y) {
    if (not is_valid_position(x, y)) {
        throw std::runtime_error{ "Invalid position for terminal cursor." };
    }
    std::cout << "\x1b[" << (y + 1) << ";" << (x + 1) << "H";
}

void Terminal::print_raw(c2k::Utf8StringView const text) {
    std::cout << text.view();
}

void Terminal::print(c2k::Utf8StringView const text) {
    print_wrapped(text);
}

void Terminal::println() {
    std::cout << '\n';
}

void Terminal::println(c2k::Utf8StringView const text) {
    print(text);
    println();
}

[[nodiscard]] c2k::Utf8String Terminal::read_line() {
    auto input = std::string{};
    if (not std::getline(std::cin, input)) {
        throw std::runtime_error{ "Failed to read line." };
    }
    return input;
}

void Terminal::hide_cursor() {
    std::cout << "\033[?25l";
}

void Terminal::show_cursor() {
    std::cout << "\033[?25h";
}

void Terminal::set_text_color(TextColor const color) {
    std::cout << "\x1b[" << static_cast<int>(color) << "m";
}

void Terminal::set_background_color(BackgroundColor const color) {
    std::cout << "\x1b[" << static_cast<int>(color) << "m";
}

void Terminal::reset_colors() {
    std::cout << "\x1b[0m";
}

void Terminal::enter_alternate_screen_buffer() {
    std::cout << "\033[?1049h";
}

void Terminal::exit_alternate_screen_buffer() {
    std::cout << "\033[?1049l";
}

[[nodiscard]] bool Terminal::is_valid_position(int const x, int const y) const {
    return x >= 0 && x < width && y >= 0 && y < height;
}

void Terminal::print_wrapped(c2k::Utf8StringView text) {
    using namespace std::chrono_literals;
    auto x = 0;
    auto const words = text.split(" ");
    for (auto const& word : words) {
        auto word_length = static_cast<int>(word.calculate_char_width());
        auto to_print = word;
        auto const is_headline = word_length >= 1 and word.front() == '#';
        if (is_headline) {
            set_text_color(TextColor::BrightWhite);
            --word_length;
            to_print = word.substring(word.cbegin() + 1);
        }

        auto const remaining = width - x;
        if (word_length > remaining) {
            x = 0;
            std::cout << '\n';
        }
        if (is_headline) {
            std::cout << to_print.view();
        } else {
            auto num_asterisks = usize{ 0 };
            for (auto const c : to_print) {
                if (c == '*') {
                    ++num_asterisks;
                }
            }

            auto const should_be_highlighted = num_asterisks == 2;

            auto highlighted = false;
            for (auto const c : to_print) {
                if (should_be_highlighted and c == '*' and not highlighted) {
                    set_text_color(TextColor::BrightYellow);
                    highlighted = true;
                } else if (should_be_highlighted and c == '*' and highlighted) {
                    reset_colors();
                    highlighted = false;
                } else {
                    std::cout << c;
                    std::cout << std::flush;
                    std::this_thread::sleep_for(20ms);
                }
            }
        }
        x += word_length + 1;
        if (x < width) {
            std::cout << ' ';
        }
    }
    reset_colors();
}
