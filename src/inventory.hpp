#pragma once

#include <lib2k/types.hpp>
#include <lib2k/utf8/string.hpp>
#include <memory>
#include <vector>

#include "utils.hpp"

class Item;
class ItemBlueprint;

class Inventory final {
private:
    std::vector<std::unique_ptr<Item>> m_contents;

public:
    Inventory() = default;

    template<std::same_as<std::unique_ptr<Item>>... Items>
    explicit Inventory(Items&&... items)
        : m_contents{ std::forward<Items>(items)... } {}

    [[nodiscard]] bool is_empty() const {
        return m_contents.empty();
    }

    [[nodiscard]] bool is_not_empty() const {
        return not is_empty();
    }

    [[nodiscard]] bool contains(ItemBlueprint const* blueprint) const;

    void clear() {
        m_contents.clear();
    }

    void clean_up() {
        erase_if(m_contents, [](auto const& pointer) { return pointer == nullptr; });
    }

    void insert(std::unique_ptr<Item> item);

    bool remove(Item* item);

    [[nodiscard]] c2k::Utf8String pretty_print(usize base_indentation, usize indentation_step = 2) const;

    [[nodiscard]] auto begin() {
        return m_contents.begin();
    }

    [[nodiscard]] auto end() {
        return m_contents.end();
    }

    [[nodiscard]] auto begin() const {
        return m_contents.begin();
    }

    [[nodiscard]] auto end() const {
        return m_contents.end();
    }

    [[nodiscard]] auto cbegin() const {
        return m_contents.cbegin();
    }

    [[nodiscard]] auto cend() const {
        return m_contents.cend();
    }

    friend std::ostream& operator<<(std::ostream& ostream, Inventory const& inventory) {
        return ostream << inventory.pretty_print(2, 2).view();
    }
};
