#pragma once

#include <lib2k/utf8/string.hpp>
#include <vector>
#include "file_parser.hpp"

class Item final {
private:
    c2k::Utf8String m_name;
    c2k::Utf8String m_description;
    std::vector<c2k::Utf8String> m_classes;
    // TODO: Actions.

public:
    explicit Item(Tree const& tree);

    [[nodiscard]] c2k::Utf8String const& name() const {
        return m_name;
    }

    friend std::ostream& operator<<(std::ostream& ostream, Item const& item) {
        return ostream << "Item(" << item.m_name.view() << ")";
    }
};
