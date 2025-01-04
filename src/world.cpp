#include "world.hpp"
#include <filesystem>
#include "parser.hpp"

static constexpr auto items_directory = "items";
static constexpr auto rooms_directory = "rooms";

using DirectoryIterator = std::filesystem::recursive_directory_iterator;

[[nodiscard]] static auto read_item_blueprints() {
    auto blueprints = std::unordered_map<c2k::Utf8String, ItemBlueprint>{};
    for (auto const& directory_entry : DirectoryIterator{ items_directory }) {
        if (directory_entry.path().extension() != ".item") {
            std::cout << "Ignoring file \"" << directory_entry.path().string() << "\" due to extension mismatch.\n";
            continue;
        }
        std::cout << "Reading item file \"" << directory_entry.path().string() << "\"...\n";
        auto const tree = File{ directory_entry.path() }.tree();
        auto item = ItemBlueprint{
            tree.fetch<String>("name"),
            tree.fetch<String>("description"),
            tree.fetch<IdentifierList>("classes"),
        };
        blueprints.emplace(directory_entry.path().stem().string(), std::move(item));
    }

    if (not blueprints.contains("default")) {
        std::cerr << "Warning: No default item found. Please add a file called \"default.item\".\n";
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
    std::cout << "Instantiating item \"" << key.view() << "\".\n";
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
            std::cout << "Ignoring file \"" << directory_entry.path().string() << "\" due to extension mismatch.\n";
            continue;
        }
        std::cout << "Reading room file \"" << directory_entry.path().string() << "\"...\n";
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

void World::process_command(Command const& command) {
    if (not command.has_nouns()) {
        if (try_handle_single_verb(command.verb)) {
            return;
        }
    } else if (command.nouns.size() == 1) {
        if (try_handle_verb_and_single_noun(command.verb, command.nouns.front().noun)) {
            return;
        }
    }
    std::cout << "Ich verstehe nicht, was ich tun soll.\n";
}

[[nodiscard]] WordList World::known_objects() const {
    auto objects = WordList{};
    for (auto const& exit : m_current_room->exits()) {
        objects.push_back(m_rooms.at(exit.target_room).name());
    }
    for (auto const& item : m_current_room->inventory()) {
        objects.push_back(item->blueprint().name());
    }
    return objects;
}

[[nodiscard]] bool World::try_handle_single_verb(c2k::Utf8StringView const verb) {
    if (verb == "inventar") {
        if (m_inventory.is_empty()) {
            std::cout << "Ich habe nichts bei mir.\n";
            return true;
        }
        std::cout << "Ich habe folgendes bei mir:\n";
        for (auto const& item : m_inventory) {
            std::cout << item->blueprint().name() << '\n';
        }
        return true;
    }
    if (verb == "schaue") {
        std::cout << m_current_room->description() << '\n';
        return true;
    }
    if (verb == "hilf" or verb == "hilfe") {
        std::cout << "Du schaust dich um und siehst die folgenden Dinge, mit denen du interagieren könntest:\n";
        for (auto const& object : known_objects()) {
            std::cout << object << '\n';
        }
        return true;
    }
    return false;
}

[[nodiscard]] bool World::try_handle_verb_and_single_noun(c2k::Utf8StringView const verb, c2k::Utf8StringView const noun) {
    if (verb == "nimm" or verb == "nehme") {
        if (auto item = find_item(noun)) {
            if (not item.value()->blueprint().is_collectible()) {
                std::cout << "Das kann ich nicht mitnehmen.\n";
                return true;
            }
            std::cout << "<" << item.value()->blueprint().name() << " eingesammelt>\n";
            m_inventory.insert(std::move(item.value()));
            m_current_room->inventory().clean_up();
            return true;
        }
        return false;
    }
    if (verb == "schaue") {
        if (auto const item = find_item(noun)) {
            std::cout << item.value()->blueprint().description() << '\n';
            return true;
        }
        if (auto const exit = find_exit(noun)) {
            std::cout << exit.value().description << '\n';
            return true;
        }
        return false;
    }
    if (verb == "betritt" or verb == "betrete") {
        if (auto exit = find_exit(noun)) {
            for (auto const required_item : exit.value().required_items) {
                if (not m_inventory.contains(required_item)) {
                    std::cout << exit->on_locked.value() << '\n';
                    return true;
                }
            }
            auto& target_room = find_room_by_reference(exit.value().target_room);
            std::cout << m_current_room->on_exit() << '\n';
            m_current_room = &target_room;
            std::cout << m_current_room->on_entry() << '\n';
            return true;
        }
        return false;
    }
    if (verb == "öffne" or verb == "öffnen") {
        if (auto item = find_item(noun)) {
            if (not item.value()->blueprint().has_inventory()) {
                std::cout << "Das kann ich nicht öffnen.\n";
                return true;
            }
            auto& inventory = item.value()->inventory();
            if (inventory.is_empty()) {
                std::cout << "Es ist leer.\n";
                return true;
            }
            std::cout << "Du findest die folgenden Gegenstände:\n";
            for (auto& item_to_take : inventory) {
                std::cout << item_to_take->blueprint().name() << '\n';
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
