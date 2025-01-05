#pragma once

#include <lib2k/utf8/string.hpp>
#include <vector>

#include "choice.hpp"

struct Label final {
    c2k::Utf8String text;
    std::vector<Choice> choices;

    explicit Label(c2k::Utf8String text, std::vector<Choice> choices)
        : text{ std::move(text) }, choices{ std::move(choices) } {}
};
