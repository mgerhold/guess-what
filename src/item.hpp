#pragma once

#include <lib2k/utf8/string.hpp>
#include <vector>
#include "file_parser.hpp"
#include "inventory.hpp"
#include "item_blueprint.hpp"

class Item final {
private:
    ItemBlueprint const* m_blueprint;
    Inventory m_inventory;

public:
    explicit Item(ItemBlueprint const& blueprint, Inventory inventory)
        : m_blueprint{ &blueprint }, m_inventory{ std::move(inventory) } {}

    [[nodiscard]] ItemBlueprint const& blueprint() const {
        return *m_blueprint;
    }

    [[nodiscard]] Inventory& inventory() {
        return m_inventory;
    }

    [[nodiscard]] Inventory const& inventory() const {
        return m_inventory;
    }

    [[nodiscard]] bool try_execute_action(
        c2k::Utf8StringView const category,
        std::vector<Item*> const& targets,
        ActionContext const& context
    ) {
        for (auto const& [action_category, actions] : m_blueprint->actions()) {
            if (action_category != category) {
                continue;
            }
            auto actions_completed = true;
            for (auto const& action : actions) {
                if (not action->try_execute(*this, targets, context)) {
                    actions_completed = false;
                    break;
                }
            }
            if (actions_completed) {
                return true;
            }
        }
        return false;
    }

    friend std::ostream& operator<<(std::ostream& ostream, Item const& item) {
        ostream << item.blueprint().name().view() << '\n';
        ostream << item.inventory() << '\n';
        return ostream;
    }
};
