#pragma once

#include <lib2k/utf8/string.hpp>
#include <optional>
#include <tl/optional.hpp>
#include <vector>

struct Choice final {
    c2k::Utf8String prompt;
    c2k::Utf8String text;
    std::vector<c2k::Utf8String> required_items;
    std::vector<c2k::Utf8String> defines;
    tl::optional<c2k::Utf8String> goto_target_reference;

    explicit Choice(
        c2k::Utf8String prompt,
        c2k::Utf8String text,
        std::vector<c2k::Utf8String> required_items,
        std::vector<c2k::Utf8String> defines,
        tl::optional<c2k::Utf8String> goto_target_reference
    )
        : prompt{ std::move(prompt) },
          text{ std::move(text) },
          required_items{ std::move(required_items) },
          defines{ std::move(defines) },
          goto_target_reference{ std::move(goto_target_reference) } {}
};
