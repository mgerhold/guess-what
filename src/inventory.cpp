#include "inventory.hpp"
#include "item.hpp"

void Inventory::insert(std::unique_ptr<Item> item) {
    m_contents.push_back(std::move(item));
}
