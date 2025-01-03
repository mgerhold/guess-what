#include "item.hpp"
#include <array>
#include <stdexcept>

using namespace c2k::Utf8Literals;
using c2k::Utf8String;
using c2k::Utf8StringView;

Item::Item(Tree const& tree)
    : m_name{ tree.fetch<String>("name") },
      m_description{ tree.fetch<String>("description") },
      m_classes{ tree.fetch<IdentifierList>("class") } {}
