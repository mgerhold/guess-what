#pragma once

#include <expected>
#include <lib2k/utf8/string.hpp>
#include <lib2k/utf8/string_view.hpp>
#include <variant>
#include <vector>
#include "command.hpp"
#include "utils.hpp"
#include "word_list.hpp"

struct SyntaxError {};

struct UnknownObject {
    c2k::Utf8String name;
};

using ParserError = std::variant<SyntaxError, UnknownObject>;

inline std::ostream& operator<<(std::ostream& ostream, ParserError const& error) {
    return std::visit(
        Overloaded{
            [&](SyntaxError const&) -> std::ostream& { return ostream << "Ich habe dich nicht verstanden."; },
            [&](UnknownObject const& object) -> std::ostream& {
                return ostream << "Ich kenne kein Objekt namens '" << object.name << "'.";
            },
        },
        error
    );
}

[[nodiscard]] std::expected<Command, ParserError> parse_command(c2k::Utf8StringView input, WordList const& objects, WordList const& ignore_list);
