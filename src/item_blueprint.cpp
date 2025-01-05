#include "item_blueprint.hpp"
#include <lib2k/utf8/string.hpp>
#include <lib2k/utf8/string_view.hpp>
#include "item.hpp"

[[nodiscard]] bool ItemBlueprint::has_class(c2k::Utf8StringView const name) const {
    return std::find(m_classes.cbegin(), m_classes.cend(), name) != m_classes.cend();
}

[[nodiscard]] std::unique_ptr<Item> ItemBlueprint::instantiate() const {
    return std::make_unique<Item>(*this, Inventory{});
}
