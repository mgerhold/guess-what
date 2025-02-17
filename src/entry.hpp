#pragma once

#include <lib2k/types.hpp>
#include <lib2k/utf8/string.hpp>
#include <lib2k/utf8/string_view.hpp>
#include <tl/optional.hpp>
#include <unordered_map>
#include <vector>

class Tree;
class String;
class IdentifierList;
class Reference;

class Entry {
public:
    Entry() = default;
    Entry(Entry const& other) = default;
    Entry(Entry&& other) noexcept = default;
    Entry& operator=(Entry const& other) = default;
    Entry& operator=(Entry&& other) noexcept = default;
    virtual ~Entry() = default;

    [[nodiscard]] virtual bool is_tree() const {
        return false;
    }

    [[nodiscard]] virtual bool is_string() const {
        return false;
    }

    [[nodiscard]] virtual bool is_identifier_list() const {
        return false;
    }

    [[nodiscard]] virtual bool is_reference() const {
        return false;
    }

    [[nodiscard]] virtual Tree const& as_tree() const;
    [[nodiscard]] virtual String const& as_string() const;
    [[nodiscard]] virtual IdentifierList const& as_identifier_list() const;
    [[nodiscard]] virtual Reference const& as_reference() const;
    [[nodiscard]] virtual char const* type_name() const = 0;

    [[nodiscard]] virtual c2k::Utf8String pretty_print(usize base_indentation, usize indentation_step) const = 0;
};

class Tree final : public Entry {
public:
    using Dict = std::vector<std::pair<c2k::Utf8String, std::unique_ptr<Entry>>>;
    using ValueType = Dict;

private:
    Dict m_entries;

public:
    explicit Tree(Dict entries)
        : m_entries{ std::move(entries) } {}

    [[nodiscard]] bool is_tree() const override {
        return true;
    }

    [[nodiscard]] c2k::Utf8String pretty_print(usize base_indentation, usize indentation_step) const override;

    template<std::derived_from<Entry> T>
    [[nodiscard]] auto try_fetch(c2k::Utf8StringView key) const -> tl::optional<typename T::ValueType const&>;

    template<std::derived_from<Entry> T>
    [[nodiscard]] auto fetch(c2k::Utf8StringView const key) const -> typename T::ValueType const& {
        auto result = try_fetch<T>(key);
        if (not result.has_value()) {
            throw std::runtime_error{ "Required key \"" + std::string{ key.view() } + "\" not found or type mismatch." };
        }
        return std::move(result).value();
    }

    template<std::derived_from<Entry> T>
    [[nodiscard]] auto fetch_all(c2k::Utf8StringView const key) const -> std::vector<typename T::ValueType const*>;

    [[nodiscard]] Tree const& as_tree() const override {
        return *this;
    }

    [[nodiscard]] char const* type_name() const override {
        return "Tree";
    }

    [[nodiscard]] Dict const& entries() const {
        return m_entries;
    }

    [[nodiscard]] auto cbegin() const {
        return m_entries.cbegin();
    }

    [[nodiscard]] auto cend() const {
        return m_entries.cend();
    }
};

class String final : public Entry {
public:
    using ValueType = c2k::Utf8String;

private:
    c2k::Utf8String m_value;

public:
    explicit String(c2k::Utf8String value)
        : m_value{ std::move(value) } {}

    [[nodiscard]] bool is_string() const override {
        return true;
    }

    [[nodiscard]] c2k::Utf8String pretty_print(usize base_indentation, usize indentation_step) const override;

    [[nodiscard]] String const& as_string() const override {
        return *this;
    }

    [[nodiscard]] char const* type_name() const override {
        return "String";
    }

    [[nodiscard]] c2k::Utf8String const& value() const {
        return m_value;
    }
};

class IdentifierList final : public Entry {
public:
    using ValueType = std::vector<c2k::Utf8String>;

private:
    std::vector<c2k::Utf8String> m_values;

public:
    explicit IdentifierList(std::vector<c2k::Utf8String> values)
        : m_values{ std::move(values) } {}

    [[nodiscard]] bool is_identifier_list() const override {
        return true;
    }

    [[nodiscard]] c2k::Utf8String pretty_print(usize base_indentation, usize indentation_step) const override;

    [[nodiscard]] IdentifierList const& as_identifier_list() const override {
        return *this;
    }

    [[nodiscard]] char const* type_name() const override {
        return "IdentifierList";
    }

    [[nodiscard]] std::vector<c2k::Utf8String> const& values() const {
        return m_values;
    }
};

class Reference final : public Entry {
public:
    using ValueType = Reference;

    [[nodiscard]] bool is_reference() const override {
        return true;
    }

    [[nodiscard]] c2k::Utf8String pretty_print(usize base_indentation, usize indentation_step) const override;

    [[nodiscard]] Reference const& as_reference() const override {
        return *this;
    }

    [[nodiscard]] char const* type_name() const override {
        return "Reference";
    }
};

template<std::derived_from<Entry> T>
auto Tree::try_fetch(c2k::Utf8StringView const key) const -> tl::optional<typename T::ValueType const&> {
    auto const iterator =
        std::find_if(m_entries.cbegin(), m_entries.cend(), [&](auto const& pair) { return pair.first == key; });
    if (iterator == m_entries.cend()) {
        // Not found.
        return tl::nullopt;
    }
    if constexpr (std::same_as<T, String>) {
        if (not iterator->second->is_string()) {
            return tl::nullopt;
        }
        return iterator->second->as_string().value();
    } else if constexpr (std::same_as<T, Tree>) {
        if (not iterator->second->is_tree()) {
            return tl::nullopt;
        }
        return iterator->second->as_tree().entries();
    } else if constexpr (std::same_as<T, IdentifierList>) {
        if (not iterator->second->is_identifier_list()) {
            return tl::nullopt;
        }
        return iterator->second->as_identifier_list().values();
    } else if constexpr (std::same_as<T, Reference>) {
        if (not iterator->second->is_reference()) {
            return tl::nullopt;
        }
        return iterator->second->as_reference();
    } else {
        throw;
    }
}

template<std::derived_from<Entry> T>
auto Tree::fetch_all(c2k::Utf8StringView const key) const -> std::vector<typename T::ValueType const*> {
    auto results = std::vector<typename T::ValueType const*>{};
    for (auto const& [current_key, value] : m_entries) {
        if (key != current_key) {
            continue;
        }
        if constexpr (std::same_as<T, String>) {
            if (not value->is_string()) {
                continue;
            }
            return &value->as_string().value();
        } else if constexpr (std::same_as<T, Tree>) {
            if (not value->is_tree()) {
                continue;
            }
            return &value->as_tree().entries();
        } else if constexpr (std::same_as<T, IdentifierList>) {
            if (not value->is_identifier_list()) {
                continue;
            }
            return &value->as_identifier_list().values();
        } else if constexpr (std::same_as<T, Reference>) {
            if (not value->is_reference()) {
                continue;
            }
            return &value->as_reference();
        } else {
            throw;
        }
    }
    return results;
}
