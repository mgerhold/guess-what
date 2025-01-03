#include "item.hpp"
#include <array>
#include <stdexcept>

using namespace c2k::Utf8Literals;
using c2k::Utf8String;
using c2k::Utf8StringView;

[[nodiscard]] bool ItemBlueprint::has_class(Utf8StringView const name) const {
    return std::find(m_classes.cbegin(), m_classes.cend(), name) != m_classes.cend();
}
