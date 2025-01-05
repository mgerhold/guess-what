#pragma once

#include <vector>
#include "item.hpp"
#include "terminal.hpp"

class Context final : public ActionContext {
private:
    Terminal* m_terminal;
    std::vector<Item*> m_available_items;
    std::function<void(Item*)> m_remove_item;
    std::function<void(c2k::Utf8StringView, SpawnLocation)> m_spawn_item;
    std::function<void(c2k::Utf8StringView)> m_define;
    std::function<void(c2k::Utf8StringView)> m_undefine;
    std::function<bool(c2k::Utf8StringView)> m_is_defined;
    std::function<void(c2k::Utf8StringView)> m_goto_room;
    std::function<void(c2k::Utf8StringView)> m_start_dialog;
    std::function<void(void)> m_win;

public:
    Context(
        Terminal& terminal,
        std::vector<Item*> available_items,
        std::function<void(Item*)> remove_item,
        std::function<void(c2k::Utf8StringView, SpawnLocation)> spawn_item,
        std::function<void(c2k::Utf8StringView)> define,
        std::function<void(c2k::Utf8StringView)> undefine,
        std::function<bool(c2k::Utf8StringView)> is_defined,
        std::function<void(c2k::Utf8StringView)> goto_room,
        std::function<void(c2k::Utf8StringView)> start_dialog,
        std::function<void(void)> win
    )
        : m_terminal{ &terminal },
          m_available_items{ std::move(available_items) },
          m_remove_item{ std::move(remove_item) },
          m_spawn_item{ std::move(spawn_item) },
          m_define{ std::move(define) },
          m_undefine{ std::move(undefine) },
          m_is_defined{ std::move(is_defined) },
          m_goto_room{ std::move(goto_room) },
          m_start_dialog{ std::move(start_dialog) },
          m_win{ std::move(win) } {}

    [[nodiscard]] Terminal& terminal() const override {
        return *m_terminal;
    }

    [[nodiscard]] std::vector<Item*> const& available_items() const override {
        return m_available_items;
    }

    void remove_item(Item* item) const override {
        m_remove_item(item);
    }

    [[nodiscard]] Item* find_item(c2k::Utf8StringView reference) const override {
        return *std::find_if(m_available_items.cbegin(), m_available_items.cend(), [&](auto const& item) {
            return item->blueprint().reference() == reference;
        });
    }

    void spawn_item(c2k::Utf8StringView const reference, SpawnLocation const location) const override {
        m_spawn_item(reference, location);
    }

    void define(c2k::Utf8StringView const identifier) const override {
        m_define(identifier);
    }

    void undefine(c2k::Utf8StringView const identifier) const override {
        m_undefine(identifier);
    }

    [[nodiscard]] bool is_defined(c2k::Utf8StringView const identifier) const override {
        return m_is_defined(identifier);
    }

    void goto_room(c2k::Utf8StringView const room_reference) const override {
        m_goto_room(room_reference);
    }

    void start_dialog(c2k::Utf8String const& dialog_reference) const override {
        m_start_dialog(dialog_reference);
    }

    void win() const override {
        m_win();
    }
};
