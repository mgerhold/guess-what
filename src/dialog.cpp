#include "dialog.hpp"

#include <lib2k/string_utils.hpp>
#include "file_parser.hpp"
#include "utils.hpp"

Dialog::Dialog(std::filesystem::path const& path) {
    auto const file = read_file(path);
    auto const tree = FileParser{ Lexer{ file }.tokenize() }.parse();

    auto start_found = false;
    m_speaker = tree.fetch<String>("speaker");
    for (auto const& [label_name, sub_tree] : tree.fetch<Tree>("labels")) {
        if (not sub_tree->is_tree()) {
            throw std::runtime_error{ "Label must be defined as tree." };
        }
        auto const& label_tree = sub_tree->as_tree();
        auto text = label_tree.fetch<String>("text");
        auto choices = std::vector<Choice>{};
        for (auto const& [choice_key, choice_sub_tree] : label_tree.entries()) {
            if (choice_key != "choice") {
                continue;
            }
            if (not choice_sub_tree->is_tree()) {
                throw std::runtime_error{ "Choice must be defined as tree." };
            }
            auto const& choice_tree = choice_sub_tree->as_tree();
            auto prompt = choice_tree.fetch<String>("prompt");
            auto choice_text = choice_tree.fetch<String>("text");
            auto required_items = choice_tree.try_fetch<IdentifierList>("required_items");
            auto defines = choice_tree.try_fetch<IdentifierList>("define");
            auto goto_target_reference = choice_tree.try_fetch<IdentifierList>("goto");
            if (not goto_target_reference.has_value() and not choice_tree.try_fetch<Reference>("exit").has_value()) {
                throw std::runtime_error{ "Choice must have either a \"goto\" or an \"exit\" key." };
            }
            if (goto_target_reference.has_value() and goto_target_reference->size() != 1) {
                throw std::runtime_error{ "Goto target must be a single identifier." };
            }
            auto goto_target = goto_target_reference.map([](auto const& identifiers) { return identifiers.front(); });
            choices.emplace_back(
                std::move(prompt),
                std::move(choice_text),
                required_items.value_or(std::vector<c2k::Utf8String>{}),
                defines.value_or(std::vector<c2k::Utf8String>{}),
                std::move(goto_target)
            );
        }

        if (label_name == "start") {
            start_found = true;
        }
        m_labels.emplace(label_name, Label{ std::move(text), std::move(choices) });
    }
    if (not start_found) {
        throw std::runtime_error{ "Dialog must have a \"start\" label." };
    }
}

usize Dialog::read_choice(Terminal& terminal, usize const size) const {
    while (true) {
        terminal.print_raw("> ");
        auto const input = terminal.read_line();
        auto const number = c2k::parse<usize>(input.view());
        if (number.has_value() and number.value() > 0 and number.value() <= size) {
            return number.value() - 1;
        }
        terminal.println("Ungültige Eingabe. Bitte wähle eine der angegebenen Optionen.");
    }
}

void Dialog::run(
    Terminal& terminal,
    std::function<void(c2k::Utf8StringView)> const& define,
    std::function<bool(c2k::Utf8StringView)> const& has_item
) const {
    auto current_label = c2k::Utf8String{ "start" };
    while (true) {
        auto const& label = m_labels.at(current_label);
        terminal.println(m_speaker.operator+(": ").operator+(label.text));

        auto possible_choices = std::vector<Choice const*>{};

        for (auto const& choice : label.choices) {
            auto all_required_items_available = true;
            for (auto const& required_item : choice.required_items) {
                if (not has_item(required_item)) {
                    all_required_items_available = false;
                    break;
                }
            }
            if (all_required_items_available) {
                possible_choices.push_back(&choice);
            }
        }

        for (auto i = usize{ 0 }; i < possible_choices.size(); ++i) {
            terminal.println(std::to_string(i + 1) + ". " + possible_choices.at(i)->prompt);
        }

        auto const choice_index = read_choice(terminal, possible_choices.size());
        auto const& choice = *possible_choices.at(choice_index);
        terminal.println("*Ich*: " + choice.text);
        for (auto const& define_identifier : choice.defines) {
            define(define_identifier);
        }
        if (choice.goto_target_reference.has_value()) {
            current_label = choice.goto_target_reference.value();
        } else {
            break;
        }
    }
}
