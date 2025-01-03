#include "file_parser.hpp"
#include <lib2k/defer.hpp>

using namespace c2k::Utf8Literals;

[[nodiscard]] static c2k::Utf8String indent(c2k::Utf8StringView const view, usize const indentation) {
    auto result = c2k::Utf8String{};
    for (auto i = usize{ 0 }; i < indentation; ++i) {
        result += ' ';
    }
    result += view;
    return result;
}

[[nodiscard]] static constexpr auto is_whitespace(c2k::Utf8Char const c) {
    return c == ' ' or c == '\f' or c == '\n' or c == '\r' or c == '\t' or c == '\v';
}

[[nodiscard]] static c2k::Utf8StringView left_trim(c2k::Utf8StringView const view) {
    for (auto it = view.cbegin(); it != view.cend(); ++it) {
        if (not is_whitespace(*it)) {
            return view.substring(it, view.cend());
        }
    }
    return view;
}

[[nodiscard]] static c2k::Utf8StringView right_trim(c2k::Utf8StringView const view) {
    for (auto it = view.crbegin(); it != view.crend(); ++it) {
        if (not is_whitespace(*it)) {
            return view.substring(view.cbegin(), it.base());
        }
    }
    return view;
}

[[nodiscard]] static c2k::Utf8StringView trim(c2k::Utf8StringView const view) {
    return left_trim(right_trim(view));
}

[[nodiscard]] static usize get_indentation(c2k::Utf8StringView const view) {
    auto indentation = usize{ 0 };
    for (auto it = view.cbegin(); it != view.cend(); ++it) {
        if (not is_whitespace(*it)) {
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

[[nodiscard]] Tree File::parse(c2k::Utf8StringView const view) {
    if (view.is_empty() or view.back() != '\n') {
        throw std::runtime_error{ "Input must end with a linebreak." };
    }
    static constexpr auto is_comment = [](c2k::Utf8StringView const line) {
        auto const trimmed = trim(line);
        return trimmed.calculate_char_count() >= 2 and trimmed.front() == '/' and *(trimmed.cbegin() + 1) == '/';
    };

    auto dict = Tree::Dict{};
    auto iterator = view.find('\n');
    auto line = right_trim(view.substring(view.cbegin(), iterator));
    if (iterator == view.cend() - 1) {
        auto const trimmed = trim(line);
        if (not line.is_empty()) {
            dict[trimmed] = std::make_unique<Reference>();
        }
        return Tree{ std::move(dict) };
    }

    // Use the indentation of the first line as a base for the rest.
    auto const indentation = get_indentation(line);
    while (true) {
        auto const advance_line = [&] {
            if (iterator == view.cend()) {
                return;
            }
            auto const new_iterator = view.find('\n', iterator + 1);
            line = view.substring(iterator + 1, new_iterator);
            iterator = new_iterator;
        };
        // Automatically advance to the next line when leaving this scope
        // (e.g. when using continue).
        auto const line_advancer = c2k::Defer{ advance_line };

        if (iterator == view.cend()) {
            break;
        }

        if (is_comment(line) or trim(line).is_empty()) {
            continue;
        }

        auto contains_colon = line.find(":") != line.cend();
        auto parts = line.split(":");
        for (auto& part : parts) {
            part = trim(part);
        }
        std::erase_if(parts, [](c2k::Utf8StringView const part) { return part.is_empty(); });
        switch (parts.size()) {
            case 1: {
                auto const key = parts.front();
                if (not contains_colon) {
                    // This is a reference.
                    dict[key] = std::make_unique<Reference>();
                    break;
                }
                // This is a nested tree.
                // Find the indented block for the recursive call (may extend until
                // the end of the file).
                auto const block_start = iterator + 1;
                auto previous_iterator = iterator;
                auto previous_line = line;
                while (true) {
                    advance_line();
                    if (iterator == view.cend()) {
                        // End of file.
                        break;
                    }
                    if (get_indentation(line) <= indentation) {
                        // This line has the same indentation as or less than the base line.
                        break;
                    }
                    previous_iterator = iterator;
                    previous_line = line;
                }
                auto const block = view.substring(block_start, previous_iterator + 1);
                dict[key] = std::make_unique<Tree>(parse(block));
                // Undo the last advancement, because the line will be advanced
                // automatically at the scope's end.
                iterator = previous_iterator;
                line = previous_line;
                break;
            }
            case 2: {
                auto const key = parts.front();
                auto const value = parts.back();
                if (value.front() == '"') {
                    dict[key] = string(value);
                } else {
                    dict[key] = identifier_list(value);
                }
                break;
            }
            default:
                throw std::runtime_error{ "Unable to parse file in line: " + std::string{ line.view() } };
        }
    }

    return Tree{ std::move(dict) };
}

[[nodiscard]] std::unique_ptr<String> File::string(c2k::Utf8StringView const view) {
    if (view.back() != '"') {
        throw std::runtime_error{ "Unterminated string literal: " + std::string{ view.view() } };
    }
    return std::make_unique<String>(view.substring(view.cbegin() + 1, view.cend() - 1));
}

[[nodiscard]] std::unique_ptr<IdentifierList> File::identifier_list(c2k::Utf8StringView view) {
    auto const identifiers_views = view.split(",");
    auto identifiers = std::vector<c2k::Utf8String>{};
    identifiers.reserve(identifiers_views.size());
    for (auto& identifier_view : identifiers_views) {
        auto const trimmed = trim(identifier_view);
        if (trimmed.is_empty()) {
            continue;
        }
        identifiers.emplace_back(trimmed);
    }
    if (identifiers.empty()) {
        throw std::runtime_error{ "Empty identifier list." };
    }
    return std::make_unique<IdentifierList>(std::move(identifiers));
}
