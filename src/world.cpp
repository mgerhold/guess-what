#include "world.hpp"
#include <filesystem>
#include <functional>
#include "action.hpp"
#include "context.hpp"
#include "dialog_database.hpp"
#include "parser.hpp"
#include "synonyms_dict.hpp"

static constexpr auto items_directory = "items";
static constexpr auto rooms_directory = "rooms";

using DirectoryIterator = std::filesystem::recursive_directory_iterator;

[[nodiscard]] static std::vector<std::unique_ptr<Action>> action_list(Tree const& tree) {
    auto actions = std::vector<std::unique_ptr<Action>>{};
    for (auto const& [action_type, arguments] : tree.entries()) {
        if (action_type == "print") {
            if (not arguments->is_string()) {
                throw std::runtime_error{ "Print action must have a string argument." };
            }
            actions.push_back(std::make_unique<Print>(arguments->as_string().value()));
            continue;
        }
        if (action_type == "with") {
            if (not arguments->is_identifier_list()) {
                throw std::runtime_error{ "Use action must have an identifier list as argument." };
            }
            actions.push_back(std::make_unique<Use>(arguments->as_identifier_list().values()));
            continue;
        }
        if (action_type == "consume") {
            if (arguments->is_reference()) {
                actions.push_back(std::make_unique<Consume>());
                continue;
            }
            if (not arguments->is_identifier_list()) {
                throw std::runtime_error{ "Consume action must have a reference or an identifier list as argument." };
            }
            actions.push_back(std::make_unique<Consume>(arguments->as_identifier_list().values()));
            continue;
        }
        if (action_type == "spawn") {
            if (not arguments->is_identifier_list()) {
                throw std::runtime_error{ "Spawn action must have an identifier list as argument." };
            }
            actions.push_back(std::make_unique<Spawn>(arguments->as_identifier_list().values()));
            continue;
        }
        if (action_type == "take") {
            if (not arguments->is_identifier_list()) {
                throw std::runtime_error{ "Take action must have an identifier list as argument." };
            }
            actions.push_back(std::make_unique<Take>(arguments->as_identifier_list().values()));
            continue;
        }
        if (action_type == "define") {
            if (not arguments->is_identifier_list()) {
                throw std::runtime_error{ "Define action must have an identifier list as argument." };
            }
            actions.push_back(std::make_unique<Define>(arguments->as_identifier_list().values()));
            continue;
        }
        if (action_type == "undefine") {
            if (not arguments->is_identifier_list()) {
                throw std::runtime_error{ "Undefine action must have an identifier list as argument." };
            }
            actions.push_back(std::make_unique<Undefine>(arguments->as_identifier_list().values()));
            continue;
        }
        if (action_type == "if") {
            if (not arguments->is_identifier_list()) {
                throw std::runtime_error{ "If action must have an identifier list as argument." };
            }
            actions.push_back(std::make_unique<If>(arguments->as_identifier_list().values()));
            continue;
        }
        if (action_type == "if_not") {
            if (not arguments->is_identifier_list()) {
                throw std::runtime_error{ "IfNot action must have an identifier list as argument." };
            }
            actions.push_back(std::make_unique<IfNot>(arguments->as_identifier_list().values()));
            continue;
        }
        if (action_type == "goto") {
            if (not arguments->is_identifier_list() or arguments->as_identifier_list().values().size() != 1) {
                throw std::runtime_error{ "IfNot action must have a single identifier as argument." };
            }
            actions.push_back(std::make_unique<Goto>(arguments->as_identifier_list().values().front()));
            continue;
        }
        if (action_type == "dialog") {
            if (not arguments->is_identifier_list() or arguments->as_identifier_list().values().size() != 1) {
                throw std::runtime_error{ "Dialog action must have a single identifier as argument." };
            }
            actions.push_back(std::make_unique<DialogAction>(arguments->as_identifier_list().values().front()));
            continue;
        }
        if (action_type == "win") {
            if (not arguments->is_reference()) {
                throw std::runtime_error{ "Win action must have a reference as argument." };
            }
            actions.push_back(std::make_unique<Win>());
            continue;
        }
        throw std::runtime_error{ std::string{ action_type.view() } + " is not a valid action." };
    }
    return actions;
}

[[nodiscard]] static auto read_item_blueprints() {
    auto blueprints = std::unordered_map<c2k::Utf8String, ItemBlueprint>{};
    for (auto const& directory_entry : DirectoryIterator{ items_directory }) {
        if (directory_entry.path().extension() != ".item") {
            continue;
        }
        auto const tree = File{ directory_entry.path() }.tree();

        auto actions = ItemBlueprint::Actions{};

        if (auto const actions_tree = tree.try_fetch<Tree>("actions")) {
            for (auto const& [key, value] : actions_tree.value()) {
                if (not value->is_tree()) {
                    throw std::runtime_error{ "Actions must be defined as tree." };
                }
                actions.emplace_back(key, action_list(value->as_tree()));
            }
        }

        auto reference = c2k::Utf8String{ directory_entry.path().stem().string() };
        auto item = ItemBlueprint{
            reference,
            tree.fetch<String>("name"),
            tree.fetch<String>("description"),
            tree.fetch<IdentifierList>("classes"),
            std::move(actions),
        };
        blueprints.emplace(std::move(reference), std::move(item));
    }
    return blueprints;
}

[[nodiscard]] static std::unique_ptr<Item> instantiate_item(
    World::ItemBlueprints const& blueprints,
    c2k::Utf8StringView const key,
    Entry const& value
) {
    auto const find_iterator = blueprints.find(key);
    if (find_iterator == blueprints.cend()) {
        throw std::runtime_error{ "Item \"" + std::string{ key.view() } + "\" requested, but no blueprint found." };
    }
    auto const& blueprint = find_iterator->second;
    auto inventory = Inventory{};
    if (value.is_tree()) {
        // TODO: Apply other properties if present.
        auto const contents = value.as_tree().try_fetch<Tree>("contents");
        if (contents.has_value()) {
            for (auto const& [sub_key, sub_value] : contents.value()) {
                inventory.insert(instantiate_item(blueprints, sub_key, *sub_value));
            }
        }
    } else if (not value.is_reference()) {
        throw std::runtime_error{ "Room contents can only be specified as reference or sub-tree (found "
                                  + std::string{ value.type_name() } + " "
                                  + std::string{ value.pretty_print(0, 0).view() } + " instead)." };
    }
    return std::make_unique<Item>(blueprint, std::move(inventory));
}

[[nodiscard]] static std::vector<Exit> extract_exits(World::ItemBlueprints const& item_blueprints, Tree const& tree) {
    auto const exits_tree = tree.try_fetch<Tree>("exits");
    if (not exits_tree.has_value()) {
        return {};
    }
    auto exits = std::vector<Exit>{};
    for (auto const& [key, value] : exits_tree.value()) {
        if (not value->is_tree()) {
            throw std::runtime_error{ "Room exits must be defined as tree (got " + std::string{ value->type_name() }
                                      + " " + std::string{ value->pretty_print(0, 0).view() } + ")." };
        }
        auto const& sub_tree = value->as_tree();

        auto description = sub_tree.fetch<String>("description");

        auto required_items = std::vector<ItemBlueprint const*>{};
        auto on_locked = std::optional<c2k::Utf8String>{};
        if (auto const required_items_list = sub_tree.try_fetch<IdentifierList>("required_items")) {
            for (auto const& required_item : required_items_list.value()) {
                auto const find_iterator = item_blueprints.find(required_item);
                if (find_iterator == item_blueprints.cend()) {
                    throw std::runtime_error{ "Room exit requires item blueprint \""
                                              + std::string{ required_item.view() } + "\" which could not be found." };
                }
                required_items.push_back(&find_iterator->second);
            }
            on_locked = sub_tree.fetch<String>("on_locked");
        }

        exits.emplace_back(key, std::move(description), std::move(required_items), std::move(on_locked));
    }
    return exits;
}

[[nodiscard]] static auto read_rooms(World::ItemBlueprints const& item_blueprints) {
    auto rooms = std::unordered_map<c2k::Utf8String, Room>{};
    for (auto const& directory_entry : DirectoryIterator{ rooms_directory }) {
        if (directory_entry.path().extension() != ".room") {
            continue;
        }
        auto const tree = File{ directory_entry.path() }.tree();
        auto room = Room{ tree.fetch<String>("name"),
                          tree.fetch<String>("description"),
                          tree.fetch<String>("on_entry"),
                          tree.fetch<String>("on_exit"),
                          extract_exits(item_blueprints, tree) };
        auto [inserted, _] = rooms.emplace(directory_entry.path().stem().string(), std::move(room));

        // If the room has any contents, insert all items into the room's inventory.
        auto const contents = tree.try_fetch<Tree>("contents");
        if (not contents.has_value()) {
            continue;
        }
        for (auto const& [key, value] : contents.value()) {
            inserted->second.insert(instantiate_item(item_blueprints, key, *value));
        }
    }

    return rooms;
}

World::World()
    : m_item_blueprints{ read_item_blueprints() }, m_rooms{ read_rooms(m_item_blueprints) } {
    if (auto const start_room = m_rooms.find("start"); start_room != m_rooms.cend()) {
        m_current_room = &start_room->second;
    } else {
        std::cerr << "Warning: No starting room found. Please add a file called \"start.room\".\n";
    }
}

[[nodiscard]] bool World::process_command(
    Command const& command,
    Terminal& terminal,
    SynonymsDict const& synonyms,
    TextDatabase const& text_database,
    DialogDatabase const& dialog_database
) {
    if (not command.has_nouns()) {
        if (try_handle_single_verb(command.verb, terminal, synonyms, text_database, dialog_database)) {
            return m_running;
        }
    } else if (command.nouns.size() == 1) {
        if (try_handle_verb_and_single_noun(
                command.verb,
                command.nouns.front().noun,
                terminal,
                synonyms,
                text_database,
                dialog_database
            )) {
            return m_running;
        }
    } else {
        // Check if there's an item that provides a custom action for the
        // given nouns.
        auto const context = build_context(terminal, text_database, dialog_database);
        auto const category = synonyms.reverse_lookup(command.verb);

        if (auto item = find_item(command.nouns.at(0).noun, true)) {
            // "item" now is the item that the player wants to interact with.
            if (auto target = find_item(command.nouns.at(1).noun, true)) {
                // "target" now is the target item that the player wants to interact with.
                if (item.value()->try_execute_action(category, { target.value().get() }, context)) {
                    return m_running;
                }

                // If this didn't work, we try to swap the items.
                if (target.value()->try_execute_action(category, { item.value().get() }, context)) {
                    return m_running;
                }
            }
        }
    }
    terminal.println("Ich verstehe nicht, was ich tun soll.");
    return m_running;
}

[[nodiscard]] WordList World::known_objects() const {
    auto objects = WordList{};
    objects.push_back(m_current_room->name());
    for (auto const& exit : m_current_room->exits()) {
        objects.push_back(m_rooms.at(exit.target_room).name());
    }
    for (auto const& item : m_current_room->inventory()) {
        objects.push_back(item->blueprint().name());
    }
    for (auto const& item : m_inventory) {
        objects.push_back(item->blueprint().name());
    }
    std::sort(objects.begin(), objects.end(), [](auto const& a, auto const& b) { return a.view() < b.view(); });
    return objects;
}

[[nodiscard]] bool World::try_handle_single_verb(
    c2k::Utf8StringView const verb,
    Terminal& terminal,
    SynonymsDict const& synonyms,
    TextDatabase const& text_database,
    DialogDatabase const& dialog_database
) {
    if (synonyms.is_synonym_of(verb, "user_manual")) {
        text_database.get("user_manual").print(terminal);
        return true;
    }
    if (synonyms.is_synonym_of(verb, "inventory")) {
        if (m_inventory.is_empty()) {
            terminal.println("Ich habe nichts bei mir.");
            return true;
        }
        terminal.println("Ich habe folgendes bei mir:");
        for (auto const& item : m_inventory) {
            terminal.println(item->blueprint().name());
        }
        return true;
    }
    if (synonyms.is_synonym_of(verb, "look")) {
        terminal.println(m_current_room->description());
        return true;
    }
    if (synonyms.is_synonym_of(verb, "help")) {
        terminal.println("Du schaust dich um und siehst die folgenden Dinge, mit denen du interagieren könntest:");
        for (auto const& object : known_objects()) {
            terminal.println(object);
        }
        return true;
    }
    return false;
}

[[nodiscard]] bool World::try_handle_verb_and_single_noun(
    c2k::Utf8StringView const verb,
    c2k::Utf8StringView const noun,
    Terminal& terminal,
    SynonymsDict const& synonyms,
    TextDatabase const& text_database,
    DialogDatabase const& dialog_database
) {
    // First, we check if there's an item that provides a custom action for the
    // given noun. If so, we execute the action and return early.
    auto const context = build_context(terminal, text_database, dialog_database);
    auto const category = synonyms.reverse_lookup(verb);
    if (auto item = find_item(noun)) {
        if (item.value()->try_execute_action(category, {}, context)) {
            return true;
        }
    }

    if (synonyms.is_synonym_of(verb, "take")) {
        if (auto item = find_item(noun)) {
            if (not item.value()->blueprint().is_collectible()) {
                terminal.println("Das kann ich nicht mitnehmen.");
                return true;
            }
            terminal.print_raw("<");
            terminal.set_text_color(TextColor::Green);
            terminal.print_raw(item.value()->blueprint().name());
            terminal.reset_colors();
            terminal.print_raw(" eingesammelt>\n");
            m_inventory.insert(std::move(item.value()));
            m_current_room->inventory().clean_up();
            return true;
        }
        return false;
    }
    if (synonyms.is_synonym_of(verb, "look")) {
        // Check if the noun is the name of the current room.
        if (noun == m_current_room->name().to_lowercase()) {
            terminal.println(m_current_room->description());
            return true;
        }

        // Check if the noun is the name of an item in the current room.
        if (auto const item = find_item(noun)) {
            terminal.println(item.value()->blueprint().description());
            return true;
        }

        // Check if the noun is in the player's inventory.
        if (auto const item = std::find_if(
                m_inventory.begin(),
                m_inventory.end(),
                [&](auto const& item) {
                    return item->blueprint().name().to_lowercase() == c2k::Utf8String{ noun }.to_lowercase();
                }
            );
            item != m_inventory.end()) {
            terminal.println((*item)->blueprint().description());
            return true;
        }

        // Check if the noun is the name of an exit.
        if (auto const exit = find_exit(noun)) {
            terminal.println(exit.value().description);
            return true;
        }
        return false;
    }
    if (synonyms.is_synonym_of(verb, "enter")) {
        if (auto exit = find_exit(noun)) {
            for (auto const required_item : exit.value().required_items) {
                if (not m_inventory.contains(required_item)) {
                    terminal.println(exit->on_locked.value());
                    return true;
                }
            }
            auto& target_room = find_room_by_reference(exit.value().target_room);
            terminal.clear();
            terminal.println(m_current_room->on_exit());
            m_current_room = &target_room;
            terminal.println(m_current_room->on_entry());
            return true;
        }
        return false;
    }
    if (synonyms.is_synonym_of(verb, "open")) {
        if (auto item = find_item(noun)) {
            if (not item.value()->blueprint().has_inventory()) {
                terminal.println("Das kann ich nicht öffnen.");
                return true;
            }
            auto& inventory = item.value()->inventory();
            if (inventory.is_empty()) {
                terminal.println("Es ist leer.");
                return true;
            }
            terminal.println("Du findest die folgenden Gegenstände:");
            for (auto& item_to_take : inventory) {
                terminal.println(item_to_take->blueprint().name());
                m_current_room->insert(std::move(item_to_take));
            }
            inventory.clear();
            return true;
        }
        return false;
    }
    return false;
}

[[nodiscard]] Room& World::find_room_by_reference(c2k::Utf8StringView const name) {
    auto const find_iterator = m_rooms.find(name);
    if (find_iterator == m_rooms.end()) {
        throw std::runtime_error{ "Room '" + std::string{ name.view() } + "' not found." };
    }
    return find_iterator->second;
}

[[nodiscard]] tl::optional<Exit const&> World::find_exit(c2k::Utf8StringView const name) {
    for (auto const& exit : m_current_room->exits()) {
        auto const& room = find_room_by_reference(exit.target_room);
        if (room.name().to_lowercase() == c2k::Utf8String{ name }.to_lowercase()) {
            return exit;
        }
    }
    return tl::nullopt;
}

[[nodiscard]] tl::optional<std::unique_ptr<Item>&> World::find_item(
    c2k::Utf8StringView const name,
    bool include_player_inventory
) {
    auto& inventory = m_current_room->inventory();
    auto find_iterator = std::find_if(inventory.begin(), inventory.end(), [&](auto& item) {
        return item->blueprint().name().to_lowercase() == c2k::Utf8String{ name }.to_lowercase();
    });
    if (find_iterator != inventory.end() or not include_player_inventory) {
        return *find_iterator;
    }

    find_iterator = std::find_if(m_inventory.begin(), m_inventory.end(), [&](auto& item) {
        return item->blueprint().name().to_lowercase() == c2k::Utf8String{ name }.to_lowercase();
    });
    if (find_iterator != m_inventory.end()) {
        return *find_iterator;
    }
    return tl::nullopt;
}

[[nodiscard]] Context World::build_context(
    Terminal& terminal,
    TextDatabase const& text_database,
    DialogDatabase const& dialog_database
) {
    auto available_items = std::vector<Item*>{};
    for (auto& item : m_current_room->inventory()) {
        available_items.push_back(item.get());
    }
    for (auto& item : m_inventory) {
        available_items.push_back(item.get());
    }

    auto const define = [this](c2k::Utf8StringView const identifier) {
        m_defines.insert(identifier);
    };

    auto const has_item = [this](c2k::Utf8StringView const reference) -> bool {
        return std::find_if(
                   m_inventory.cbegin(),
                   m_inventory.cend(),
                   [&](auto const& item) { return item->blueprint().reference() == reference; }
               )
               != m_inventory.cend();
    };

    return Context{
        terminal,
        std::move(available_items),
        [this](Item* item) { remove_item(item); },
        [this](c2k::Utf8StringView const reference, SpawnLocation const location) { spawn_item(reference, location); },
        define,
        [this](c2k::Utf8StringView const identifier) { m_defines.erase(identifier); },
        [this](c2k::Utf8StringView const identifier) { return m_defines.contains(identifier); },
        [this, &terminal](c2k::Utf8StringView const room_reference) {
            auto& room = find_room_by_reference(room_reference);
            terminal.clear();
            terminal.println(m_current_room->on_exit());
            m_current_room = &room;
            terminal.println(m_current_room->on_entry());
        },
        [this, &terminal, &dialog_database, define, has_item](c2k::Utf8StringView const dialog_reference) {
            dialog_database.run_dialog(dialog_reference, terminal, define, has_item);
        },
        [this, &terminal, &text_database] {
            terminal.clear();
            text_database.get("win").print(terminal);
            m_running = false;
        },
    };
}

void World::remove_item(Item* item) {
    if (m_inventory.remove(item)) {
        return;
    }
    if (m_current_room->inventory().remove(item)) {
        return;
    }
    throw std::runtime_error{ "Item to remove could not be found." };
}

void World::spawn_item(c2k::Utf8StringView const reference, SpawnLocation const location) {
    auto const item_blueprint = m_item_blueprints.find(reference);
    if (item_blueprint == m_item_blueprints.cend()) {
        throw std::runtime_error{ "Item blueprint \"" + std::string{ reference.view() } + "\" not found." };
    }
    switch (location) {
        case SpawnLocation::Inventory:
            m_inventory.insert(item_blueprint->second.instantiate());
            break;
        case SpawnLocation::Room:
            m_current_room->insert(item_blueprint->second.instantiate());
            break;
    }
}
