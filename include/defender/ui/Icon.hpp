#pragma once

#include "defender/entities/Types.hpp"

#include <cstdint>

namespace defender
{

/// Icons of assets/sprites/ui/icons.png (16x16 each), in the same order as
/// ICONS in tools/sprites/ui.py.
enum class Icon : std::uint8_t
{
    Coin,
    Heart,
    Shield,
    Star,
    Flag,
    Paw,
    Hourglass,
    FastForward,
    Play,
    Tree,
    Lock,
    Sword,
    Target,
    Bolt,
    Close,
    Check,
    WaterDrop,  ///< Water material
    Ember,      ///< Fire material
    Stone,      ///< Earth material
    Feather,    ///< Wind material
    Upgrade,
};

/// Icon of an element's material (Coin for Normal: normal towers use gold only).
constexpr Icon materialIcon(Element element)
{
    switch (element)
    {
        case Element::Water:
            return Icon::WaterDrop;
        case Element::Fire:
            return Icon::Ember;
        case Element::Earth:
            return Icon::Stone;
        case Element::Wind:
            return Icon::Feather;
        case Element::Normal:
            return Icon::Coin;
    }
    return Icon::Coin;
}

inline constexpr int ICON_SIZE = 16;

}  // namespace defender
