#pragma once

#include <lib2k/utf8/string.hpp>
#include <vector>
#include "file_parser.hpp"
#include "inventory.hpp"

class ItemBlueprint final {
private:
    c2k::Utf8String m_name;
    c2k::Utf8String m_description;
    std::vector<c2k::Utf8String> m_classes;

public:
    explicit ItemBlueprint(c2k::Utf8String name, c2k::Utf8String description, std::vector<c2k::Utf8String> classes)
        : m_name{ std::move(name) }, m_description{ std::move(description) }, m_classes{ std::move(classes) } {}

    [[nodiscard]] bool has_inventory() const {
        return has_class("inventory");
    }

    [[nodiscard]] bool is_collectible() const {
        return has_class("collectible");
    }

    [[nodiscard]] bool has_class(c2k::Utf8StringView name) const;

    [[nodiscard]] c2k::Utf8String const& name() const {
        return m_name;
    }

    [[nodiscard]] c2k::Utf8String const& description() const {
        return m_description;
    }
};

class Item final {
private:
    ItemBlueprint const* m_blueprint;
    Inventory m_inventory;

public:
    explicit Item(ItemBlueprint const& blueprint, Inventory inventory)
        : m_blueprint{ &blueprint }, m_inventory{ std::move(inventory) } {}

    [[nodiscard]] ItemBlueprint const& blueprint() const {
        return *m_blueprint;
    }

    [[nodiscard]] Inventory& inventory() {
        return m_inventory;
    }

    [[nodiscard]] Inventory const& inventory() const {
        return m_inventory;
    }

    friend std::ostream& operator<<(std::ostream& ostream, Item const& item) {
        ostream << item.blueprint().name().view() << '\n';
        ostream << item.inventory() << '\n';
        return ostream;
    }
};
