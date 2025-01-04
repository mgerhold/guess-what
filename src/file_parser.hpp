#pragma once

#include <concepts>
#include <filesystem>
#include <fstream>
#include <lib2k/types.hpp>
#include <lib2k/utf8/string.hpp>
#include <lib2k/utf8/string_view.hpp>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <tl/optional.hpp>
#include <unordered_map>
#include "entry.hpp"
#include "lexer.hpp"
#include "token.hpp"
#include "utils.hpp"

class FileParser final {
private:
    std::vector<Token> m_tokens;
    usize m_index = 0;

public:
    explicit FileParser(std::vector<Token> tokens)
        : m_tokens{ std::move(tokens) } {}

    [[nodiscard]] Tree parse() {
        return tree();
    }

private:
    [[nodiscard]] std::vector<c2k::Utf8String> identifier_list() {
        auto identifiers = std::vector{ expect<token::Identifier>("Expected identifier").lexeme };
        while (match<token::Comma>()) {
            identifiers.push_back(expect<token::Identifier>("Expected identifier").lexeme);
        }
        // Allow trailing comma.
        std::ignore = match<token::Comma>();
        expect<token::Linebreak>("Expected linebreak");
        return identifiers;
    }

    [[nodiscard]] Tree tree() {
        auto dict = Tree::Dict{};

        // Ignore leading linebreaks (if the file is empty, this will ignore the
        // complete file, which is okay).
        while (match<token::Linebreak>()) {}

        while (not is_at_end() and not current_is<token::Dedent>()) {
            auto const& key = expect<token::Identifier>("Expected identifier");
            if (match<token::Linebreak>()) {
                dict.emplace_back(key.lexeme, std::make_unique<Reference>());
                continue;
            }
            expect<token::Colon>("Expected ':'");
            if (auto const string = match<token::String>()) {
                dict.emplace_back(key.lexeme, std::make_unique<String>(string.value().lexeme));
                expect<token::Linebreak>("Expected linebreak after string");
                continue;
            }

            if (current_is<token::Identifier>()) {
                dict.emplace_back(key.lexeme, std::make_unique<IdentifierList>(identifier_list()));
                continue;
            }
            expect<token::Linebreak>("Expected string, identifier list, or linebreak");
            expect<token::Indent>("Subtree must be indented");
            dict.emplace_back(key.lexeme, std::make_unique<Tree>(tree()));
            expect<token::Dedent>("Subtree must end with dedent");
        }

        return Tree{ std::move(dict) };
    }

    template<typename T>
    T const& expect(char const* const error_message) {
        if (auto const result = match<T>()) {
            return result.value();
        }
        auto stream = std::ostringstream{};
        stream << error_message << " (got " << current() << " instead)";
        throw std::runtime_error{ std::move(stream).str() };
    }

    template<typename T>
    [[nodiscard]] tl::optional<T const&> match() {
        if (not current_is<T>()) {
            return tl::nullopt;
        }
        auto const& result = std::get<T>(current());
        advance();
        return result;
    }

    template<typename T>
    [[nodiscard]] bool current_is() const {
        return is<T>(current());
    }

    [[nodiscard]] bool is_at_end() const {
        return m_index >= m_tokens.size() or is<token::EndOfInput>(m_tokens.at(m_index));
    }

    [[nodiscard]] Token const& current() const {
        if (is_at_end()) {
            return m_tokens.back();
        }
        return m_tokens.at(m_index);
    }

    void advance() {
        if (is_at_end()) {
            return;
        }
        ++m_index;
    }

    template<typename T>
    [[nodiscard]] static bool is(Token const& token) {
        return std::holds_alternative<T>(token);
    }
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
          m_contents{ FileParser{ Lexer{ read_file(m_filepath) }.tokenize() }.parse() } {}

    [[nodiscard]] Tree const& tree() const& {
        return m_contents;
    }

    [[nodiscard]] Tree tree() && {
        return std::move(m_contents);
    }

    friend std::ostream& operator<<(std::ostream& ostream, File const& file) {
        return ostream << file.m_filename << '\n' << file.m_contents.pretty_print(0, 4).view();
    }
};
