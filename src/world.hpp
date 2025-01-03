#pragma once

#include "item.hpp"
#include <unordered_map>

class World final {
private:
    std::unordered_map<c2k::Utf8String, Item> m_items;

public:
    World();
};
