#include "terminal.hpp"
#include <lib2k/types.hpp>
#include <locale>
#include <sstream>
#include <utility>
#ifdef _WIN32
#include <windows.h>

[[nodiscard]] static std::pair<int, int> get_terminal_size() {
    auto info = CONSOLE_SCREEN_BUFFER_INFO{};
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
    return {
        info.srWindow.Right - info.srWindow.Left + 1,
        info.srWindow.Bottom - info.srWindow.Top + 1,
    };
}
#else
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstdio>

[[nodiscard]] static std::pair<int, int> get_terminal_size() {
    auto info = winsize{};
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &info);
    return {
        info.ws_col,
        info.ws_row,
    };
}
#endif

static void enable_ansi() {
#ifdef _WIN32
    auto const stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (stdout_handle == INVALID_HANDLE_VALUE) {
        throw std::runtime_error{ "Unable to get stdout handle." };
    }
    auto dwMode = DWORD{ 0 };
    GetConsoleMode(stdout_handle, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(stdout_handle, dwMode);
#endif
}

Terminal::Terminal() {
    auto expected = false;
    if (not s_initialized.compare_exchange_strong(expected, true)) {
        throw std::runtime_error{ "Terminal may only be initialized once." };
    }
    std::setlocale(LC_ALL, ".65001");
    enable_ansi();
    std::tie(m_width, m_height) = get_terminal_size();

    m_buffer.reserve(height());
    for (auto row = 0; row < height(); ++row) {
        m_buffer.emplace_back(width(), ' ');
    }

    enter_alternate_screen_buffer();
}

Terminal::~Terminal() noexcept {
    show_cursor();
    exit_alternate_screen_buffer();
    s_initialized = false;
}

void Terminal::clear() {
    std::cout << "\x1b[2J\x1b[H";
}

void Terminal::set_position(int const x, int const y) {
    if (not is_valid_position(x, y)) {
        throw std::runtime_error{ "Invalid position for terminal cursor." };
    }
    std::cout << "\x1b[" << (y + 1) << ";" << (x + 1) << "H";
}

void Terminal::print(c2k::Utf8StringView const text) {
    std::cout << text.view();
}

void Terminal::println() {
    std::cout << '\n';
}

void Terminal::println(c2k::Utf8StringView const text) {
    std::cout << text.view() << '\n';
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

void Terminal::fill_background(BackgroundColor const color) {
    set_background_color(color);
    for (auto row = 0; row < height(); ++row) {
        set_position(0, row);
        print(m_buffer[row]);
    }
    set_position(0, 0);
}

void Terminal::enter_alternate_screen_buffer() {
    std::cout << "\033[?1049h";
}

void Terminal::exit_alternate_screen_buffer() {
    std::cout << "\033[?1049l";
}

[[nodiscard]] bool Terminal::is_valid_position(int const x, int const y) const {
    return x >= 0 && x < m_width && y >= 0 && y < m_height;
}
