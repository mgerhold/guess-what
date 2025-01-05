#include "inventory.hpp"
#include "item.hpp"

[[nodiscard]] bool Inventory::contains(ItemBlueprint const* const blueprint) const {
    for (auto const& item : m_contents) {
        if (&item->blueprint() == blueprint) {
            return true;
        }
    }
    return false;
}

void Inventory::insert(std::unique_ptr<Item> item) {
    m_contents.push_back(std::move(item));
}

bool Inventory::remove(Item* item) {
    auto const find_iterator = std::find_if(m_contents.begin(), m_contents.end(), [&](auto const& stored_item) {
        return stored_item.get() == item;
    });
    if (find_iterator == m_contents.end()) {
        return false;
    }
    m_contents.erase(find_iterator);
    return true;
}

[[nodiscard]] c2k::Utf8String Inventory::pretty_print(usize const base_indentation, usize const indentation_step) const {
    auto result = c2k::Utf8String{};
    for (auto const& item : m_contents) {
        result += indent(item->blueprint().name(), base_indentation);
        result += "\n";
        if (item->inventory().is_not_empty()) {
            result += item->inventory().pretty_print(base_indentation + indentation_step, indentation_step);
        }
    }
    return result;
}
