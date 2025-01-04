#pragma once

#include <unordered_map>

#include "command.hpp"
#include "item.hpp"
#include "room.hpp"
#include "word_list.hpp"

class World final {
public:
    using ItemBlueprints = std::unordered_map<c2k::Utf8String, ItemBlueprint>;

private:
    ItemBlueprints m_item_blueprints;
    std::unordered_map<c2k::Utf8String, Room> m_rooms;
    Room* m_current_room = nullptr;
    Inventory m_inventory;

public:
    World();
    void process_command(Command const& command);
    [[nodiscard]] WordList known_objects() const;

private:
    [[nodiscard]] bool try_handle_single_verb(c2k::Utf8StringView verb);
    [[nodiscard]] bool try_handle_verb_and_single_noun(c2k::Utf8StringView verb, c2k::Utf8StringView noun);
    [[nodiscard]] Room& find_room_by_reference(c2k::Utf8StringView name);
    [[nodiscard]] tl::optional<Exit const&> find_exit(c2k::Utf8StringView name);
    [[nodiscard]] tl::optional<std::unique_ptr<Item>&> find_item(c2k::Utf8StringView name);
};
