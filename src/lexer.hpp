#pragma once

#include <lib2k/types.hpp>
#include <lib2k/utf8/string.hpp>
#include <lib2k/utf8/string_view.hpp>
#include <optional>
#include "token.hpp"

#include "utils.hpp"

class Lexer final {
private:
    c2k::Utf8StringView m_source;
    c2k::Utf8StringView::ConstIterator m_iterator;
    std::vector<Token> m_tokens;

public:
    explicit Lexer(c2k::Utf8StringView const source)
        : m_source{ source }, m_iterator{ m_source.cbegin() } {}

    [[nodiscard]] std::vector<Token> tokenize() {
        m_iterator = m_source.cbegin();
        m_tokens.clear();

        if (current() == '\n') {
            throw std::runtime_error{ "First line must not be empty." };
        }

        if (is_whitespace(current())) {
            throw std::runtime_error{ "First line must not start indented." };
        }

        auto indentation_step = std::optional<usize>{};
        auto indentation = usize{ 0 };  // Measured in multiples of the step size.
        while (not is_at_end()) {
            // std::cerr << "'" << current().as_string_view() << "'\n";
            if (current() == ',') {
                m_tokens.push_back(token::Comma{});
                advance();
                continue;
            }

            if (current() == ':') {
                m_tokens.push_back(token::Colon{});
                advance();
                continue;
            }

            if (current() == '\n') {
                m_tokens.push_back(token::Linebreak{});
                advance();

                auto next_newline = m_source.find('\n', m_iterator);
                while (next_newline != m_source.cend()) {
                    auto const line = trim(m_source.substring(m_iterator, next_newline));
                    if (line.is_empty()
                        or (line.calculate_char_count() >= 2 and line.front() == '/' and *(line.cbegin() + 1) == '/')) {
                        m_iterator = next_newline;
                        advance();
                        next_newline = m_source.find('\n', m_iterator);
                    } else {
                        break;
                    }
                }

                if (is_whitespace(current()) and current() != ' ' and current() != '\n') {
                    throw std::runtime_error{ "Lines must not start with whitespace except for regular spaces." };
                }
                if (not indentation_step.has_value() and current() == ' ') {
                    auto num_leading_spaces = usize{ 0 };
                    while (current() == ' ') {
                        advance();
                        ++num_leading_spaces;
                    }
                    indentation_step = num_leading_spaces;
                    indentation = 1;
                    m_tokens.emplace_back(token::Indent{});
                } else {
                    auto num_leading_spaces = usize{ 0 };
                    while (current() == ' ') {
                        advance();
                        ++num_leading_spaces;
                    }
                    if (num_leading_spaces > 0 or indentation_step.has_value()) {
                        if (num_leading_spaces % indentation_step.value() != 0) {
                            throw std::runtime_error{ "Unexpected indentation " + std::to_string(num_leading_spaces)
                                                      + " (must be multiple of "
                                                      + std::to_string(indentation_step.value()) + ")." };
                        }
                        auto const current_indentation = num_leading_spaces / indentation_step.value();
                        while (indentation < current_indentation) {
                            ++indentation;
                            m_tokens.push_back(token::Indent{});
                        }
                        while (indentation > current_indentation) {
                            --indentation;
                            m_tokens.push_back(token::Dedent{});
                        }
                    }
                }
                continue;
            }

            if (current() == '/') {
                advance();
                if (current() != '/') {
                    throw std::runtime_error{ "Illegal token '/' (did you mean '//' to start a comment?)" };
                }
                while (not is_at_end() and current() != '\n') {
                    advance();
                }
                continue;
            }

            if (current() == '"') {
                advance();
                auto const string_start = m_iterator;
                auto last_quotes = std::optional<c2k::Utf8StringView::ConstIterator>{};
                while (not is_at_end() and current() != '\n') {
                    if (current() == '"') {
                        last_quotes = m_iterator;
                    }
                    advance();
                }
                if (not last_quotes.has_value()) {
                    throw std::runtime_error{ "Unterminated string literal." };
                }
                for (auto it = last_quotes.value() + 1; it != m_iterator; ++it) {
                    if (not is_whitespace(*it)) {
                        throw std::runtime_error{ "String literal must be the last token of line." };
                    }
                }
                m_tokens.push_back(token::String{
                    c2k::Utf8String{ string_start, last_quotes.value() }
                });
                continue;
            }

            if (is_whitespace(current())) {
                advance();
                continue;
            }

            // Identifier.
            if (not is_valid_identifier_char(current())) {
                throw std::runtime_error{ "Expected identifier, got " + std::string{ current().as_string_view() }
                                          + "." };
            }
            auto const identifier_start = m_iterator;
            while (is_valid_identifier_char(current())) {
                advance();
            }
            m_tokens.push_back(token::Identifier{
                c2k::Utf8String{ identifier_start, m_iterator }
            });
        }

        while (indentation > 0) {
            m_tokens.push_back(token::Dedent{});
            --indentation;
        }

        m_tokens.emplace_back(token::EndOfInput{});
        return std::move(m_tokens);
    }

private:
    [[nodiscard]] bool is_at_end() const {
        return m_iterator == m_source.cend();
    }

    [[nodiscard]] c2k::Utf8Char current() const {
        if (is_at_end()) {
            return '\0';
        }
        return *m_iterator;
    }

    void advance() {
        if (is_at_end()) {
            return;
        }
        ++m_iterator;
    }

    [[nodiscard]] static bool is_valid_identifier_char(c2k::Utf8Char const c) {
        auto const codepoint = c.codepoint();
        return (codepoint >= 'a' and codepoint <= 'z') or (codepoint >= 'A' and codepoint <= 'Z')
               or (codepoint >= '0' and codepoint <= '9') or codepoint == '_';
    }
};
