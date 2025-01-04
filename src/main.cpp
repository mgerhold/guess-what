#include <iostream>
#include <string>
#include "file_parser.hpp"
#include "item.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "world.hpp"

[[nodiscard]] static Command get_next_command(WordList const& ignore_list, World const& world) {
    while (true) {
        std::cout << "> ";
        auto input = std::string{};
        if (not std::getline(std::cin, input)) {
            throw std::runtime_error{ "Error reading input. " };
        }

        auto const command = parse_command(input, world.known_objects(), ignore_list);
        if (command.has_value()) {
            return command.value();
        }
        std::cout << command.error() << '\n';
    }
}

int main() {
    using namespace c2k::Utf8Literals;

    auto const ignore_list = read_word_list("lists/ignore.list");

    try {
        auto world = World{};
        while (true) {
            world.process_command(get_next_command(ignore_list, world));
        }
    } catch (std::exception const& exception) {
        std::cerr << "Error: " << exception.what() << '\n';
    }
}
