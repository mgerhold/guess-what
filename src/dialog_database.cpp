#include "dialog_database.hpp"

DialogDatabase::DialogDatabase() {
    using DirectoryIterator = std::filesystem::recursive_directory_iterator;
    for (auto const& entry : DirectoryIterator{ dialogs_directory }) {
        if (entry.path().extension() != ".dialog") {
            continue;
        }
        m_dialogs.emplace(entry.path().stem().string(), Dialog{ entry.path() });
    }
}

void DialogDatabase::run_dialog(
    c2k::Utf8StringView const name,
    Terminal& terminal,
    std::function<void(c2k::Utf8StringView)> const& define,
    std::function<bool(c2k::Utf8StringView)> const& has_item
) const {
    m_dialogs.at(name).run(terminal, define, has_item);
}
