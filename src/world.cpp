#include "world.hpp"
#include <filesystem>
#include "parser.hpp"
#include "synonyms_dict.hpp"

static constexpr auto items_directory = "items";
static constexpr auto rooms_directory = "rooms";

using DirectoryIterator = std::filesystem::recursive_directory_iterator;

[[nodiscard]] static auto read_item_blueprints() {
    auto blueprints = std::unordered_map<c2k::Utf8String, ItemBlueprint>{};
    for (auto const& directory_entry : DirectoryIterator{ items_directory }) {
        if (directory_entry.path().extension() != ".item") {
            continue;
        }
        auto const tree = File{ directory_entry.path() }.tree();
        auto item = ItemBlueprint{
            tree.fetch<String>("name"),
            tree.fetch<String>("description"),
            tree.fetch<IdentifierList>("classes"),
        };
        blueprints.emplace(directory_entry.path().stem().string(), std::move(item));
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

void World::process_command(
    Command const& command,
    Terminal& terminal,
    SynonymsDict const& synonyms,
    TextDatabase const& text_database
) {
    if (not command.has_nouns()) {
        if (try_handle_single_verb(command.verb, terminal, synonyms, text_database)) {
            return;
        }
    } else if (command.nouns.size() == 1) {
        if (try_handle_verb_and_single_noun(command.verb, command.nouns.front().noun, terminal, synonyms, text_database)) {
            return;
        }
    }
    terminal.println("Ich verstehe nicht, was ich tun soll.");
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
    return objects;
}

[[nodiscard]] bool World::try_handle_single_verb(
    c2k::Utf8StringView const verb,
    Terminal& terminal,
    SynonymsDict const& synonyms,
    TextDatabase const& text_database
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
    TextDatabase const& text_database
) {
    if (synonyms.is_synonym_of(verb, "take")) {
        if (auto item = find_item(noun)) {
            if (not item.value()->blueprint().is_collectible()) {
                terminal.println("Das kann ich nicht mitnehmen.");
                return true;
            }
            terminal.print("<");
            terminal.set_text_color(TextColor::Green);
            terminal.print(item.value()->blueprint().name());
            terminal.reset_colors();
            terminal.println(" eingesammelt>");
            m_inventory.insert(std::move(item.value()));
            m_current_room->inventory().clean_up();
            return true;
        }
        return false;
    }
    if (synonyms.is_synonym_of(verb, "look")) {
        if (noun == m_current_room->name().to_lowercase()) {
            terminal.println(m_current_room->description());
            return true;
        }
        if (auto const item = find_item(noun)) {
            terminal.println(item.value()->blueprint().description());
            return true;
        }
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

[[nodiscard]] tl::optional<std::unique_ptr<Item>&> World::find_item(c2k::Utf8StringView const name) {
    auto& inventory = m_current_room->inventory();
    auto const find_iterator = std::find_if(inventory.begin(), inventory.end(), [&](auto& item) {
        return item->blueprint().name().to_lowercase() == c2k::Utf8String{ name }.to_lowercase();
    });
    if (find_iterator == inventory.end()) {
        return tl::nullopt;
    }
    return *find_iterator;
}
