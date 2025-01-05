#pragma once

#include <filesystem>
#include <lib2k/utf8/string.hpp>
#include <unordered_map>
#include "dialog.hpp"

class DialogDatabase {
private:
    static constexpr auto dialogs_directory = "dialogs";

    std::unordered_map<c2k::Utf8String, Dialog> m_dialogs;

public:
    DialogDatabase();
    void run_dialog(
        c2k::Utf8StringView name,
        Terminal& terminal,
        std::function<void(c2k::Utf8StringView)> const& define,
        std::function<bool(c2k::Utf8StringView)> const& has_item
    ) const;
};
