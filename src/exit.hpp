#pragma once

#include <lib2k/utf8/string.hpp>
#include <vector>
#include "item.hpp"

struct Exit final {
    c2k::Utf8String target_room;
    c2k::Utf8String description;
    std::vector<ItemBlueprint const*> required_items;
    std::optional<c2k::Utf8String> on_locked;

    explicit Exit(
        c2k::Utf8String target_room,
        c2k::Utf8String description,
        std::vector<ItemBlueprint const*> required_items,
        std::optional<c2k::Utf8String> on_locked
    )
        : target_room{ std::move(target_room) },
          description{ std::move(description) },
          required_items{ std::move(required_items) },
          on_locked{ std::move(on_locked) } {
        if (not this->required_items.empty() and not this->on_locked.has_value()) {
            throw std::runtime_error{ "A on_locked message must be provided if required items are defined." };
        }
    }
};
