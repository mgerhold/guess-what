#include "room.hpp"
#include "item.hpp"

void Room::insert(std::unique_ptr<Item> item) {
    m_inventory.insert(std::move(item));
}
