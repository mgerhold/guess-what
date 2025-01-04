#pragma once

#include <filesystem>
#include <fstream>
#include <lib2k/types.hpp>
#include <lib2k/utf8/string.hpp>
#include <lib2k/utf8/string_view.hpp>
#include <sstream>

template<typename... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};

[[nodiscard]] inline c2k::Utf8String read_file(std::filesystem::path const& path) {
    auto file = std::ifstream{ path };
    if (not file) {
        throw std::runtime_error{ "Unable to open file: " + path.string() };
    }
    auto stream = std::ostringstream{};
    stream << file.rdbuf();
    if (not file) {
        throw std::runtime_error{ "Failed to read file: " + path.string() };
    }
    return c2k::Utf8String{ std::move(stream).str() };
}

[[nodiscard]] inline c2k::Utf8String indent(c2k::Utf8StringView const view, usize const indentation) {
    auto result = c2k::Utf8String{};
    for (auto i = usize{ 0 }; i < indentation; ++i) {
        result += ' ';
    }
    result += view;
    return result;
}

[[nodiscard]] constexpr auto is_whitespace(c2k::Utf8Char const c) {
    return c == ' ' or c == '\f' or c == '\n' or c == '\r' or c == '\t' or c == '\v';
}

[[nodiscard]] inline c2k::Utf8StringView left_trim(c2k::Utf8StringView const view) {
    for (auto it = view.cbegin(); it != view.cend(); ++it) {
        if (not is_whitespace(*it)) {
            return view.substring(it, view.cend());
        }
    }
    return view.substring(view.cbegin(), view.cbegin());
}

[[nodiscard]] inline c2k::Utf8StringView right_trim(c2k::Utf8StringView const view) {
    for (auto it = view.crbegin(); it != view.crend(); ++it) {
        if (not is_whitespace(*it)) {
            return view.substring(view.cbegin(), it.base());
        }
    }
    return view.substring(view.cbegin(), view.cbegin());
}

[[nodiscard]] inline c2k::Utf8StringView trim(c2k::Utf8StringView const view) {
    return left_trim(right_trim(view));
}
