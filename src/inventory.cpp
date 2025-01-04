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
