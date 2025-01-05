#pragma once

#include <filesystem>
#include <lib2k/utf8/string.hpp>
#include <vector>
#include "label.hpp"
#include "terminal.hpp"

class Dialog final {
private:
    c2k::Utf8String m_speaker;
    std::unordered_map<c2k::Utf8String, Label> m_labels;

public:
    explicit Dialog(std::filesystem::path const& path);

    [[nodiscard]] usize read_choice(Terminal& terminal, usize size) const;
    void run(
        Terminal& terminal,
        std::function<void(c2k::Utf8StringView)> const& define,
        std::function<bool(c2k::Utf8StringView)> const& has_item
    ) const;
};
