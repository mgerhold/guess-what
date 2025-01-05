#include <iostream>
#include <string>
#include "dialog_database.hpp"
#include "file_parser.hpp"
#include "item.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "synonyms_dict.hpp"
#include "terminal.hpp"
#include "text_database.hpp"
#include "world.hpp"

[[nodiscard]] static Command get_next_command(Terminal& terminal, WordList const& ignore_list, World const& world) {
    while (true) {
        terminal.set_text_color(TextColor::BrightWhite);
        terminal.print_raw("> ");
        terminal.reset_colors();
        auto const input = terminal.read_line();
        auto const command = parse_command(input, world.known_objects(), ignore_list);
        if (command.has_value()) {
            return command.value();
        }
        terminal.println(to_string(command.error()));
    }
}

int main() {
    using namespace c2k::Utf8Literals;

    auto const synonyms_dict = SynonymsDict{};
    auto const ignore_list = read_word_list("lists/ignore.list");
    auto const text_database = TextDatabase{};
    auto const dialog_database = DialogDatabase{};
    if (not text_database.contains("intro")) {
        throw std::runtime_error{ "Missing intro text." };
    }

    auto terminal = Terminal{};
    text_database.get("intro").print(terminal);

    try {
        auto world = World{};
        while (true) {
            auto const command = get_next_command(terminal, ignore_list, world);
            if (not world.process_command(command, terminal, synonyms_dict, text_database, dialog_database)) {
                break;
            }
        }
    } catch (std::exception const& exception) {
        std::cerr << "Error: " << exception.what() << '\n';
    }
}
