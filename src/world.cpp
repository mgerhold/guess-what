#include "world.hpp"

#include <filesystem>

static constexpr auto items_directory = "items";
static constexpr auto rooms_directory = "rooms";

World::World() {
    using DirectoryIterator = std::filesystem::recursive_directory_iterator;
    for (auto const& directory_entry : DirectoryIterator{ items_directory }) {
        if (directory_entry.path().extension() != ".item") {
            std::cout << "Ignoring file \"" << directory_entry.path().string() << "\" due to extension mismatch.\n";
            continue;
        }
        std::cout << "Reading item file \"" << directory_entry.path().string() << "\"...\n";
        auto item = Item{ File{ directory_entry.path() }.tree() };
        m_items.emplace(directory_entry.path().stem().string(), std::move(item));
    }

    if (not m_items.contains("default")) {
        std::cerr << "Warning: No default item found. Please add a file called \"default.item\".\n";
    }
}
