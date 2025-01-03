#include "parser.hpp"
#include <algorithm>
#include <array>
#include <lib2k/string_utils.hpp>
#include <optional>

using namespace c2k::Utf8Literals;

static auto const punctuation = ".!,:-/\\()[]{}#+&_<>\"'"_utf8view;

// clang-format off
static auto const ignore_list = std::array{
    "auf"_utf8view,
    "an"_utf8view,
    "der"_utf8view,
    "die"_utf8view,
    "das"_utf8view,
    "des"_utf8view,
    "den"_utf8view,
    "dem"_utf8view,
    "dessen"_utf8view,
    "ein"_utf8view,
    "eine"_utf8view,
    "im"_utf8view,
    "in"_utf8view,
    "von"_utf8view,
    "vom"_utf8view,
    "zu"_utf8view,
    "zum"_utf8view,
    "zur"_utf8view,
    "über"_utf8view,
    "unter"_utf8view,
    "mir"_utf8view,
    "mit"_utf8view,
};
// clang-format on

static std::optional<c2k::Utf8String> sanitized(c2k::Utf8StringView const view) {
    auto result = c2k::Utf8String{};
    for (auto c : view) {
        if (c.is_uppercase()) {
            result += c.to_lowercase();
            continue;
        }
        if (punctuation.find(c) != punctuation.cend()) {
            // This is a punctuation character => ignore.
            continue;
        }
        result += c;
    }
    if (result.is_empty()) {
        return std::nullopt;
    }
    return result;
}

Parser::Parser(c2k::Utf8StringView const input)
    : m_tokens{ tokenize(input) } {}

[[nodiscard]] std::expected<Command, ParserError> Parser::parse(std::vector<c2k::Utf8String> const& objects) const {
    auto const is_object = [&objects](c2k::Utf8String const& object) {
        return std::find(objects.cbegin(), objects.cend(), object) != objects.cend();
    };

    switch (m_tokens.size()) {
        case 1:
            // Verb.
            return Command{ m_tokens.at(0), std::nullopt, std::nullopt };
        case 2:
            // Verb Subjective.
            if (not is_object(m_tokens.at(1))) {
                return std::unexpected{ UnknownObject{ m_tokens.at(1) } };
            }
            return Command{
                m_tokens.at(0),
                Noun{ std::nullopt, m_tokens.at(1) },
                std::nullopt
            };
        case 3:
            // Verb Adjective Subjective or Verb Subjective Objective.
            if (not is_object(m_tokens.at(2))) {
                return std::unexpected{ UnknownObject{ m_tokens.at(2) } };
            }
            if (is_object(m_tokens.at(1))) {
                // Verb Subjective Objective.
                return Command{
                    m_tokens.at(0),
                    Noun{ std::nullopt, m_tokens.at(1) },
                    Noun{ std::nullopt, m_tokens.at(2) },
                };
            }
            // Verb Adjective Subjective.
            return Command{
                m_tokens.at(0),
                Noun{ m_tokens.at(1), m_tokens.at(2) },
            };
        case 4:
            // Verb Adjective Subjective Objective or Verb Subjective Adjective Objective.
            if (not is_object(m_tokens.at(3))) {
                return std::unexpected{ UnknownObject{ m_tokens.at(3) } };
            }
            if (is_object(m_tokens.at(1))) {
                // Verb Subjective Adjective Objective.
                return Command{
                    m_tokens.at(0),
                    Noun{   std::nullopt, m_tokens.at(1) },
                    Noun{ m_tokens.at(2), m_tokens.at(3) },
                };
            }
            if (not is_object(m_tokens.at(2))) {
                return std::unexpected{ UnknownObject{ m_tokens.at(2) } };
            }
            // Verb Adjective Subjective Objective.
            return Command{
                m_tokens.at(0),
                Noun{ m_tokens.at(1), m_tokens.at(2) },
                Noun{   std::nullopt, m_tokens.at(3) },
            };
        case 5:
            // Verb Adjective Subjective Adjective Objective.
            if (not is_object(m_tokens.at(2))) {
                return std::unexpected{ UnknownObject{ m_tokens.at(2) } };
            }
            if (not is_object(m_tokens.at(4))) {
                return std::unexpected{ UnknownObject{ m_tokens.at(4) } };
            }
            return Command{
                m_tokens.at(0),
                Noun{ m_tokens.at(1), m_tokens.at(2) },
                Noun{ m_tokens.at(3), m_tokens.at(4) },
            };
        case 0:
        default:
            return std::unexpected{ SyntaxError{} };
    }
}

[[nodiscard]] std::vector<c2k::Utf8String> Parser::tokenize(c2k::Utf8StringView input) {
    auto const views = input.split(" "_utf8view);
    auto tokens = std::vector<c2k::Utf8String>{};
    tokens.reserve(views.size());
    for (auto const& view : views) {
        if (view.is_empty()) {
            continue;
        }
        auto token = sanitized(view);
        if (not token.has_value()) {
            continue;
        }
        if (std::find(ignore_list.begin(), ignore_list.end(), c2k::Utf8StringView{ token.value() })
            != ignore_list.cend()) {
            continue;
        }
        tokens.push_back(std::move(token).value());
    }
    return tokens;
}
