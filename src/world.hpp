#pragma once

#include <unordered_map>
#include "item.hpp"
#include "room.hpp"

class World final {
public:
    using ItemBlueprints = std::unordered_map<c2k::Utf8String, ItemBlueprint>;
private:
    ItemBlueprints m_item_blueprints;
    std::unordered_map<c2k::Utf8String, Room> m_rooms;

public:
    World();
};
