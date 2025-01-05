#pragma once

#include <unordered_map>
#include "command.hpp"
#include "item.hpp"
#include "room.hpp"
#include "synonyms_dict.hpp"
#include "terminal.hpp"
#include "text_database.hpp"
#include "word_list.hpp"
#include <unordered_set>

class Context;

class World final {
public:
    using ItemBlueprints = std::unordered_map<c2k::Utf8String, ItemBlueprint>;

private:
    ItemBlueprints m_item_blueprints;
    std::unordered_map<c2k::Utf8String, Room> m_rooms;
    Room* m_current_room = nullptr;
    Inventory m_inventory;
    std::unordered_set<c2k::Utf8String> m_defines;

public:
    World();
    void process_command(
        Command const& command,
        Terminal& terminal,
        SynonymsDict const& synonyms,
        TextDatabase const& text_database
    );
    [[nodiscard]] WordList known_objects() const;

private:
    [[nodiscard]] bool try_handle_single_verb(
        c2k::Utf8StringView verb,
        Terminal& terminal,
        SynonymsDict const& synonyms,
        TextDatabase const& text_database
    );
    [[nodiscard]] bool try_handle_verb_and_single_noun(
        c2k::Utf8StringView verb,
        c2k::Utf8StringView noun,
        Terminal& terminal,
        SynonymsDict const& synonyms,
        TextDatabase const& text_database
    );
    [[nodiscard]] Room& find_room_by_reference(c2k::Utf8StringView name);
    [[nodiscard]] tl::optional<Exit const&> find_exit(c2k::Utf8StringView name);
    [[nodiscard]] tl::optional<std::unique_ptr<Item>&> find_item(c2k::Utf8StringView name);
    [[nodiscard]] Context build_context(Terminal& terminal);
    void remove_item(Item* item);
    void spawn_item(c2k::Utf8StringView reference, SpawnLocation location);
};
