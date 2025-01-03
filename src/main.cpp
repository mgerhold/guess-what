#include <iostream>
#include <string>
#include "parser.hpp"

int main() {
    using namespace c2k::Utf8Literals;
    while (true) {
        std::cout << "> ";
        auto input = std::string{};
        if (not std::getline(std::cin, input)) {
            std::cerr << "Fehler beim Lesen der Eingabe.\n";
            return EXIT_FAILURE;
        }
        auto parser = Parser{ input };
        auto const command = parser.parse({ "suppe"_utf8, "haus"_utf8 });
        if (not command.has_value()) {
            std::cout << command.error() << '\n';
        } else {
            std::cout << command.value() << '\n';
        }
    }
}
