#pragma once

#include <filesystem>
#include <fstream>
#include <lib2k/types.hpp>
#include <lib2k/utf8/string.hpp>
#include <lib2k/utf8/string_view.hpp>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

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

    [[nodiscard]] virtual c2k::Utf8String pretty_print(usize base_indentation, usize indentation_step) const = 0;
};

class Tree final : public Entry {
public:
    using Dict = std::unordered_map<c2k::Utf8String, std::unique_ptr<Entry>>;

private:
    Dict m_entries;

public:
    explicit Tree(std::unordered_map<c2k::Utf8String, std::unique_ptr<Entry>> entries)
        : m_entries{ std::move(entries) } {}

    [[nodiscard]] bool is_tree() const override {
        return true;
    }

    [[nodiscard]] c2k::Utf8String pretty_print(usize base_indentation, usize indentation_step) const override;
};

class String final : public Entry {
private:
    c2k::Utf8String m_value;

public:
    explicit String(c2k::Utf8String value)
        : m_value{ std::move(value) } {}

    [[nodiscard]] bool is_string() const override {
        return true;
    }

    [[nodiscard]] c2k::Utf8String pretty_print(usize base_indentation, usize indentation_step) const override;
};

class IdentifierList final : public Entry {
private:
    std::vector<c2k::Utf8String> m_values;

public:
    explicit IdentifierList(std::vector<c2k::Utf8String> values)
        : m_values{ std::move(values) } {}

    [[nodiscard]] bool is_identifier_list() const override {
        return true;
    }

    [[nodiscard]] c2k::Utf8String pretty_print(usize base_indentation, usize indentation_step) const override;
};

class Reference final : public Entry {
public:
    [[nodiscard]] bool is_reference() const override {
        return true;
    }

    [[nodiscard]] c2k::Utf8String pretty_print(usize base_indentation, usize indentation_step) const override;
};

class File final {
private:
    std::filesystem::path m_filepath;
    std::string m_filename;
    Tree m_contents;

public:
    explicit File(std::filesystem::path const& filepath)
        : m_filepath{ canonical(filepath) },
          m_filename{ m_filepath.filename().string() },
          m_contents{ parse(read_file(m_filepath)) } {}

    friend std::ostream& operator<<(std::ostream& ostream, File const& file) {
        return ostream << file.m_filename << '\n' << file.m_contents.pretty_print(0, 4).view();
    }

private:
    [[nodiscard]] static c2k::Utf8String read_file(std::filesystem::path const& path) {
        auto file = std::ifstream{ path };
        if (not file) {
            throw std::runtime_error{ "Unable to open file: " + path.string() };
        }
        auto stream = std::ostringstream{};
        stream << file.rdbuf();
        if (not file) {
            throw std::runtime_error{ "Failed to read file: " + path.string() };
        }
        return c2k::Utf8String{ std::move(stream).str() };
    }

    [[nodiscard]] static Tree parse(c2k::Utf8StringView view);
    [[nodiscard]] static std::unique_ptr<String> string(c2k::Utf8StringView view);
    [[nodiscard]] static std::unique_ptr<IdentifierList> identifier_list(c2k::Utf8StringView view);
};
