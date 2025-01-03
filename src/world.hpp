#pragma once

#include <unordered_map>
#include "item.hpp"
#include "room.hpp"

class World final {
private:
    std::unordered_map<c2k::Utf8String, ItemBlueprint> m_items;
    std::unordered_map<c2k::Utf8String, Room> m_rooms;

public:
    World();
};
