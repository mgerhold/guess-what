#pragma once

#include <unordered_map>
#include "action.hpp"

class ItemBlueprint final {
public:
    using Actions = std::vector<std::pair<c2k::Utf8String, std::vector<std::unique_ptr<Action>>>>;

private:
    c2k::Utf8String m_reference;
    c2k::Utf8String m_name;
    c2k::Utf8String m_description;
    std::vector<c2k::Utf8String> m_classes;
    Actions m_actions;

public:
    explicit ItemBlueprint(
        c2k::Utf8String reference,
        c2k::Utf8String name,
        c2k::Utf8String description,
        std::vector<c2k::Utf8String> classes,
        Actions actions
    )
        : m_reference{ std::move(reference) },
          m_name{ std::move(name) },
          m_description{ std::move(description) },
          m_classes{ std::move(classes) },
          m_actions{ std::move(actions) } {}

    [[nodiscard]] bool has_inventory() const {
        return has_class("inventory");
    }

    [[nodiscard]] bool is_collectible() const {
        return has_class("collectible");
    }

    [[nodiscard]] bool has_class(c2k::Utf8StringView name) const;

    [[nodiscard]] c2k::Utf8String const& reference() const {
        return m_reference;
    }

    [[nodiscard]] c2k::Utf8String const& name() const {
        return m_name;
    }

    [[nodiscard]] c2k::Utf8String const& description() const {
        return m_description;
    }

    [[nodiscard]] Actions const& actions() const {
        return m_actions;
    }

    [[nodiscard]] std::unique_ptr<Item> instantiate() const;
};
