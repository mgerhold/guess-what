#include "file_parser.hpp"
#include <lib2k/defer.hpp>
#include "utils.hpp"

using namespace c2k::Utf8Literals;

[[nodiscard]] static usize get_indentation(c2k::Utf8StringView const view) {
    auto indentation = usize{ 0 };
    for (auto const c : view) {
        if (not is_whitespace(c)) {
            return indentation;
        }
        ++indentation;
    }
    return indentation;
}

[[nodiscard]] Tree const& Entry::as_tree() const {
    throw std::bad_cast{};
}

[[nodiscard]] String const& Entry::as_string() const {
    throw std::bad_cast{};
}

[[nodiscard]] IdentifierList const& Entry::as_identifier_list() const {
    throw std::bad_cast{};
}

[[nodiscard]] Reference const& Entry::as_reference() const {
    throw std::bad_cast{};
}

[[nodiscard]] c2k::Utf8String Tree::pretty_print(usize const base_indentation, usize const indentation_step) const {
    auto result = c2k::Utf8String{};
    for (auto const& [key, value] : m_entries) {
        result += indent(key, base_indentation);
        if (value->is_reference()) {
            result += '\n';
            continue;
        }
        result += ": ";
        if (value->is_tree()) {
            result += '\n';
            result += value->pretty_print(base_indentation + indentation_step, indentation_step);
        } else {
            result += value->pretty_print(0, 0);
            result += '\n';
        }
    }
    return result;
}

[[nodiscard]] c2k::Utf8String String::pretty_print(usize const base_indentation, usize) const {
    return indent('"'_utf8 + m_value + '"'_utf8, base_indentation);
}

[[nodiscard]] c2k::Utf8String IdentifierList::pretty_print(usize const base_indentation, usize) const {
    return indent(", "_utf8.join(m_values), base_indentation);
}

[[nodiscard]] c2k::Utf8String Reference::pretty_print(usize base_indentation, usize indentation_step) const {
    return "";
}
