#include "world.hpp"
#include <filesystem>

static constexpr auto items_directory = "items";
static constexpr auto rooms_directory = "rooms";

using DirectoryIterator = std::filesystem::recursive_directory_iterator;

[[nodiscard]] static auto read_items() {
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
    std::unordered_map<c2k::Utf8String, ItemBlueprint> const& blueprints,
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

[[nodiscard]] static auto read_rooms(std::unordered_map<c2k::Utf8String, ItemBlueprint> const& item_blueprints) {
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
                          tree.fetch<String>("on_exit") };
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

    for (auto const& [_, room] : rooms) {
        std::cout << room;
    }

    return rooms;
}

World::World()
    : m_items{ read_items() }, m_rooms{ read_rooms(m_items) } {}
