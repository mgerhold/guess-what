#include "parser.hpp"
#include <algorithm>
#include <array>
#include <lib2k/string_utils.hpp>
#include <optional>

using namespace c2k::Utf8Literals;

static auto const punctuation = ".!,:-/\\()[]{}#+&_<>\"'"_utf8view;

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

[[nodiscard]] static WordList tokenize(c2k::Utf8StringView input, WordList const& ignore_list) {
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

std::expected<Command, ParserError> parse_command(
    c2k::Utf8StringView const input,
    WordList const& objects,
    WordList const& ignore_list
) {
    auto const tokens = tokenize(input, ignore_list);

    auto const is_object = [&objects](c2k::Utf8String const& object) {
        return std::find_if(
                   objects.cbegin(),
                   objects.cend(),
                   [&](auto const& current) { return current.to_lowercase() == object.to_lowercase(); }
               )
               != objects.cend();
    };

    switch (tokens.size()) {
        case 1:
            // Verb.
            return Command{ tokens.at(0), std::nullopt, std::nullopt };
        case 2:
            // Verb Subjective.
            if (not is_object(tokens.at(1))) {
                return std::unexpected{ UnknownObject{ tokens.at(1) } };
            }
            return Command{
                tokens.at(0),
                Noun{ std::nullopt, tokens.at(1) },
                std::nullopt
            };
        case 3:
            // Verb Adjective Subjective or Verb Subjective Objective.
            if (not is_object(tokens.at(2))) {
                return std::unexpected{ UnknownObject{ tokens.at(2) } };
            }
            if (is_object(tokens.at(1))) {
                // Verb Subjective Objective.
                return Command{
                    tokens.at(0),
                    Noun{ std::nullopt, tokens.at(1) },
                    Noun{ std::nullopt, tokens.at(2) },
                };
            }
            // Verb Adjective Subjective.
            return Command{
                tokens.at(0),
                Noun{ tokens.at(1), tokens.at(2) },
            };
        case 4:
            // Verb Adjective Subjective Objective or Verb Subjective Adjective Objective.
            if (not is_object(tokens.at(3))) {
                return std::unexpected{ UnknownObject{ tokens.at(3) } };
            }
            if (is_object(tokens.at(1))) {
                // Verb Subjective Adjective Objective.
                return Command{
                    tokens.at(0),
                    Noun{ std::nullopt, tokens.at(1) },
                    Noun{ tokens.at(2), tokens.at(3) },
                };
            }
            if (not is_object(tokens.at(2))) {
                return std::unexpected{ UnknownObject{ tokens.at(2) } };
            }
            // Verb Adjective Subjective Objective.
            return Command{
                tokens.at(0),
                Noun{ tokens.at(1), tokens.at(2) },
                Noun{ std::nullopt, tokens.at(3) },
            };
        case 5:
            // Verb Adjective Subjective Adjective Objective.
            if (not is_object(tokens.at(2))) {
                return std::unexpected{ UnknownObject{ tokens.at(2) } };
            }
            if (not is_object(tokens.at(4))) {
                return std::unexpected{ UnknownObject{ tokens.at(4) } };
            }
            return Command{
                tokens.at(0),
                Noun{ tokens.at(1), tokens.at(2) },
                Noun{ tokens.at(3), tokens.at(4) },
            };
        case 0:
        default:
            return std::unexpected{ SyntaxError{} };
    }
}
