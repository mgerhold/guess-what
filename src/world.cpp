#include "world.hpp"
#include <filesystem>

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
        std::cout << "Exit: " << key.view() << '\n';
        if (value->is_reference()) {
            exits.emplace_back(key, std::vector<ItemBlueprint const*>{});
            continue;
        }
        if (not value->is_identifier_list()) {
            throw std::runtime_error{ "Room exits must be defined as either reference or identifier list (got "
                                      + std::string{ value->type_name() } + " "
                                      + std::string{ value->pretty_print(0, 0).view() } + ")." };
        }
        auto required_items = std::vector<ItemBlueprint const*>{};
        for (auto const& identifier : value->as_identifier_list().values()) {
            auto const find_iterator = item_blueprints.find(identifier);
            if (find_iterator == item_blueprints.cend()) {
                throw std::runtime_error{ "Room exit requires item blueprint \"" + std::string{ identifier.view() }
                                          + "\" which could not be found." };
            }
            required_items.push_back(&find_iterator->second);
        }
        exits.emplace_back(key, std::move(required_items));
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

    if (not rooms.contains("start")) {
        std::cerr << "Warning: No starting room found. Please add a file called \"start.room\".\n";
    }

    /*for (auto const& [_, room] : rooms) {
        std::cout << room;
    }*/

    return rooms;
}

World::World()
    : m_item_blueprints{ read_item_blueprints() }, m_rooms{ read_rooms(m_item_blueprints) } {}
