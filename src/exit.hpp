#pragma once

#include <lib2k/utf8/string.hpp>
#include <vector>
#include "item.hpp"

struct Exit final {
    c2k::Utf8String target_room;
    std::vector<ItemBlueprint const*> required_items;
};
