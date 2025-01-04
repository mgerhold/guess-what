#pragma once

#include <lib2k/utf8/string.hpp>
#include <vector>
#include "exit.hpp"
#include "file_parser.hpp"
#include "inventory.hpp"

class Room final {
private:
    c2k::Utf8String m_name;
    c2k::Utf8String m_description;
    c2k::Utf8String m_on_entry;
    c2k::Utf8String m_on_exit;
    Inventory m_inventory;
    std::vector<Exit> m_exits;

public:
    explicit Room(
        c2k::Utf8String name,
        c2k::Utf8String description,
        c2k::Utf8String on_entry,
        c2k::Utf8String on_exit,
        std::vector<Exit> exits
    )
        : m_name{ std::move(name) },
          m_description{ std::move(description) },
          m_on_entry{ std::move(on_entry) },
          m_on_exit{ std::move(on_exit) },
          m_exits{ std::move(exits) } {}

    void insert(std::unique_ptr<Item> item);

    [[nodiscard]] c2k::Utf8String const& name() const {
        return m_name;
    }

    [[nodiscard]] c2k::Utf8String const& description() const {
        return m_description;
    }

    [[nodiscard]] c2k::Utf8String const& on_entry() const {
        return m_on_entry;
    }

    [[nodiscard]] c2k::Utf8String const& on_exit() const {
        return m_on_exit;
    }

    [[nodiscard]] Inventory& inventory() {
        return m_inventory;
    }

    [[nodiscard]] Inventory const& inventory() const {
        return m_inventory;
    }

    [[nodiscard]] std::vector<Exit> const& exits() const {
        return m_exits;
    }

    friend std::ostream& operator<<(std::ostream& ostream, Room const& room) {
        ostream << room.m_name.view() << '\n';
        ostream << room.m_inventory << '\n';
        return ostream;
    }
};
