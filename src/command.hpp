#pragma once

#include <array>
#include <iostream>
#include <lib2k/static_vector.hpp>
#include <lib2k/utf8/string.hpp>
#include <lib2k/utf8/string_view.hpp>
#include <optional>

using namespace c2k::Utf8Literals;

struct Noun {
private:
    static inline auto const adjective_suffixes = std::array{
        "er"_utf8view, "es"_utf8view, "en"_utf8view, "em"_utf8view, "e"_utf8view,
    };

    [[nodiscard]] static bool ends_with(c2k::Utf8StringView const view, c2k::Utf8StringView const suffix) {
        if (view.calculate_char_count() < suffix.calculate_char_count()) {
            return false;
        }
        return view.substring(view.cend() - suffix.calculate_char_count()) == suffix;
    }

    [[nodiscard]] static c2k::Utf8String normalize_adjective(c2k::Utf8StringView const adjective) {
        for (auto const suffix : adjective_suffixes) {
            if (ends_with(adjective, suffix)) {
                return adjective.substring(adjective.cbegin(), adjective.cend() - suffix.calculate_char_count());
            }
        }
        return adjective;
    }

public:
    std::optional<c2k::Utf8String> adjective;
    c2k::Utf8String noun;

    Noun() = default;

    Noun(std::optional<c2k::Utf8String> adjective, c2k::Utf8String object)
        : noun{ std::move(object) } {
        if (adjective.has_value()) {
            this->adjective = normalize_adjective(std::move(adjective).value());
        }
    }

    friend std::ostream& operator<<(std::ostream& ostream, Noun const& object) {
        ostream << "[";
        if (object.adjective.has_value()) {
            ostream << "ADJECTIVE[" << object.adjective.value() << "] ";
        }
        return ostream << "NOUN[" << object.noun << "]]";
    }
};

struct Command {
    c2k::Utf8String verb;
    c2k::StaticVector<Noun, 2> nouns;

    explicit Command(
        c2k::Utf8String verb,
        std::optional<Noun> subject = std::nullopt,
        std::optional<Noun> object = std::nullopt
    )
        : verb{ std::move(verb) } {
        if (subject.has_value()) {
            nouns.push_back(std::move(subject.value()));
        }
        if (object.has_value()) {
            nouns.push_back(std::move(object.value()));
        }
    }

    friend std::ostream& operator<<(std::ostream& ostream, Command const& command) {
        ostream << "VERB[" << command.verb << "]";
        for (auto const& noun : command.nouns) {
            ostream << " " << noun;
        }
        return ostream;
    }
};
