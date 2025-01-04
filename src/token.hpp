#pragma once

#include <iostream>
#include <lib2k/utf8/string.hpp>
#include <variant>
#include "utils.hpp"

namespace token {
    struct Colon final {};

    struct Comma final {};

    struct Linebreak final {};

    struct Indent final {};

    struct Dedent final {};

    struct EndOfInput final {};

    struct Identifier final {
        c2k::Utf8String lexeme;
    };

    struct String final {
        c2k::Utf8String lexeme;
    };

}  // namespace token

using Token = std::
    variant<token::Colon, token::Comma, token::Linebreak, token::Indent, token::Dedent, token::Identifier, token::String, token::EndOfInput>;

inline std::ostream& operator<<(std::ostream& ostream, Token const& token) {
    std::visit(
        Overloaded{
            [&](token::Colon const&) { ostream << "COLON"; },
            [&](token::Comma const&) { ostream << "COMMA"; },
            [&](token::Linebreak const&) { ostream << "LINEBREAK"; },
            [&](token::Indent const&) { ostream << "INDENT"; },
            [&](token::Dedent const&) { ostream << "DEDENT"; },
            [&](token::EndOfInput const&) { ostream << "END_OF_INPUT"; },
            [&](token::Identifier const& identifier) { ostream << "IDENTIFIER(" << identifier.lexeme.view() << ')'; },
            [&](token::String const& identifier) { ostream << "STRING(" << identifier.lexeme.view() << ')'; },
        },
        token
    );
    return ostream;
}
