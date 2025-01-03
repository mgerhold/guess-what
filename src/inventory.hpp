#pragma once

#include <memory>
#include <vector>

class Item;

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

    void insert(std::unique_ptr<Item> item);
};
