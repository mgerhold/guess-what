#pragma once

#include <lib2k/utf8/string.hpp>
#include <variant>
#include <vector>
#include "terminal.hpp"

class Item;

enum class SpawnLocation {
    Inventory,
    Room,
};

class ActionContext {
public:
    ActionContext() = default;
    ActionContext(ActionContext const& other) = default;
    ActionContext(ActionContext&& other) noexcept = default;
    ActionContext& operator=(ActionContext const& other) = default;
    ActionContext& operator=(ActionContext&& other) noexcept = default;
    virtual ~ActionContext() = default;

    [[nodiscard]] virtual Terminal& terminal() const = 0;
    [[nodiscard]] virtual std::vector<Item*> const& available_items() const = 0;
    virtual void remove_item(Item* item) const = 0;
    [[nodiscard]] virtual Item* find_item(c2k::Utf8StringView reference) const = 0;
    virtual void spawn_item(c2k::Utf8StringView reference, SpawnLocation location) const = 0;
    virtual void define(c2k::Utf8StringView identifier) const = 0;
    virtual void undefine(c2k::Utf8StringView identifier) const = 0;
    [[nodiscard]] virtual bool is_defined(c2k::Utf8StringView identifier) const = 0;
    virtual void goto_room(c2k::Utf8StringView room_reference) const = 0;
    virtual void start_dialog(c2k::Utf8String const& dialog_reference) const = 0;
    virtual void win() const = 0;
};

class Action {
public:
    Action() = default;
    Action(Action const& other) = default;
    Action(Action&& other) noexcept = default;
    Action& operator=(Action const& other) = default;
    Action& operator=(Action&& other) noexcept = default;
    virtual ~Action() = default;

    [[nodiscard]] virtual bool try_execute(Item& item, std::vector<Item*> const& targets, ActionContext const& context) = 0;
};

class Print final : public Action {
private:
    c2k::Utf8String m_text;

public:
    explicit Print(c2k::Utf8String text)
        : m_text{ std::move(text) } {}

    [[nodiscard]] bool try_execute(Item&, std::vector<Item*> const&, ActionContext const& context) override {
        context.terminal().println(m_text);
        return true;
    }
};

class Use final : public Action {
private:
    std::vector<c2k::Utf8String> m_identifiers;

public:
    explicit Use(std::vector<c2k::Utf8String> identifiers)
        : m_identifiers{ std::move(identifiers) } {}

    [[nodiscard]] bool try_execute(Item&, std::vector<Item*> const& targets, ActionContext const& context) override {
        // Check if all items are available.
        for (auto const& identifier : m_identifiers) {
            if (std::find_if(
                    targets.cbegin(),
                    targets.cend(),
                    [&](auto const& item) { return item->blueprint().reference() == identifier; }
                )
                == targets.cend()) {
                return false;
            }
        }
        return true;
    }
};

class Consume final : public Action {
private:
    std::vector<c2k::Utf8String> m_identifiers;

public:
    Consume() = default;

    explicit Consume(std::vector<c2k::Utf8String> identifiers)
        : m_identifiers{ std::move(identifiers) } {}

    [[nodiscard]] bool try_execute(Item& item, std::vector<Item*> const& targets, ActionContext const& context) override {
        if (m_identifiers.empty()) {
            context.remove_item(&item);
            return true;
        }
        for (auto const& identifier : m_identifiers) {
            context.remove_item(context.find_item(identifier));
        }
        return true;
    }
};

class Spawn final : public Action {
private:
    std::vector<c2k::Utf8String> m_identifiers;

public:
    explicit Spawn(std::vector<c2k::Utf8String> identifiers)
        : m_identifiers{ std::move(identifiers) } {}

    [[nodiscard]] bool try_execute(Item&, std::vector<Item*> const&, ActionContext const& context) override {
        for (auto const& identifier : m_identifiers) {
            context.spawn_item(identifier, SpawnLocation::Room);
        }
        return true;
    }
};

class Take final : public Action {
private:
    std::vector<c2k::Utf8String> m_identifiers;

public:
    explicit Take(std::vector<c2k::Utf8String> identifiers)
        : m_identifiers{ std::move(identifiers) } {}

    [[nodiscard]] bool try_execute(Item&, std::vector<Item*> const&, ActionContext const& context) override {
        for (auto const& identifier : m_identifiers) {
            context.spawn_item(identifier, SpawnLocation::Inventory);
        }
        return true;
    }
};

class Define final : public Action {
private:
    std::vector<c2k::Utf8String> m_identifiers;

public:
    explicit Define(std::vector<c2k::Utf8String> identifiers)
        : m_identifiers{ std::move(identifiers) } {}

    [[nodiscard]] bool try_execute(Item& item, std::vector<Item*> const& targets, ActionContext const& context) override {
        for (auto const& identifier : m_identifiers) {
            context.define(identifier);
        }
        return true;
    }
};

class Undefine final : public Action {
private:
    std::vector<c2k::Utf8String> m_identifiers;

public:
    explicit Undefine(std::vector<c2k::Utf8String> identifiers)
        : m_identifiers{ std::move(identifiers) } {}

    [[nodiscard]] bool try_execute(Item& item, std::vector<Item*> const& targets, ActionContext const& context) override {
        for (auto const& identifier : m_identifiers) {
            context.undefine(identifier);
        }
        return true;
    }
};

class If final : public Action {
private:
    std::vector<c2k::Utf8String> m_identifiers;

public:
    explicit If(std::vector<c2k::Utf8String> identifiers)
        : m_identifiers{ std::move(identifiers) } {}

    [[nodiscard]] bool try_execute(Item& item, std::vector<Item*> const& targets, ActionContext const& context) override {
        for (auto const& identifier : m_identifiers) {
            if (not context.is_defined(identifier)) {
                return false;
            }
        }
        return true;
    }
};

class IfNot final : public Action {
private:
    std::vector<c2k::Utf8String> m_identifiers;

public:
    explicit IfNot(std::vector<c2k::Utf8String> identifiers)
        : m_identifiers{ std::move(identifiers) } {}

    [[nodiscard]] bool try_execute(Item& item, std::vector<Item*> const& targets, ActionContext const& context) override {
        for (auto const& identifier : m_identifiers) {
            if (context.is_defined(identifier)) {
                return false;
            }
        }
        return true;
    }
};

class Goto final : public Action {
private:
    c2k::Utf8String m_target;

public:
    explicit Goto(c2k::Utf8String target)
        : m_target{ std::move(target) } {}

    [[nodiscard]] bool try_execute(Item& item, std::vector<Item*> const& targets, ActionContext const& context) override {
        context.goto_room(m_target);
        return true;
    }
};

class DialogAction final : public Action {
private:
    c2k::Utf8String m_dialog_reference;

public:
    explicit DialogAction(c2k::Utf8String dialog_reference)
        : m_dialog_reference{ std::move(dialog_reference) } {}

    [[nodiscard]] bool try_execute(Item&, std::vector<Item*> const&, ActionContext const& context) override {
        context.start_dialog(m_dialog_reference);
        return true;
    }
};

class Win final : public Action {
public:
    Win() = default;

    [[nodiscard]] bool try_execute(Item&, std::vector<Item*> const&, ActionContext const& context) override {
        context.win();
        return true;
    }
};
