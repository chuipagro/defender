#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace defender
{

using EntityId = std::uint32_t;

/// Elements shared by enemies and towers. Normal = no element (natural
/// colors, base stats); the others are stronger and have bonus effects.
/// Adding one (Ice, Magma, Lightning...) means adding a column to every
/// row of the spawn table in EnemyFactory.cpp — it will not compile otherwise.
enum class Element : std::uint8_t
{
    Normal,
    Water,
    Fire,
    Earth,
    Wind,
};

inline constexpr std::size_t ELEMENT_COUNT = 5;

enum class EnemyType : std::uint8_t
{
    Octopus,
    Orangutan,
    Cat,
    Dog,
    Duck,
    Frog,
};

inline constexpr std::size_t ENEMY_TYPE_COUNT = 6;

/// Status of an enemy: changes its stats and its size.
enum class Rank : std::uint8_t
{
    Normal,
    MiniBoss,
    Boss,
    SuperBoss,
};

inline constexpr std::size_t RANK_COUNT = 4;

/// Towers are defined by their element for now.
using TowerType = Element;

/// What to spawn: an animal species, in an element's colors, with a rank.
struct EnemySpec
{
    EnemyType type = EnemyType::Cat;
    Element element = Element::Normal;
    Rank rank = Rank::Normal;

    bool operator==(const EnemySpec&) const = default;
};

/// Name used for asset files (assets/sprites/.../<name>.png).
constexpr std::string_view elementName(Element element)
{
    switch (element)
    {
        case Element::Normal:
            return "normal";
        case Element::Water:
            return "water";
        case Element::Fire:
            return "fire";
        case Element::Earth:
            return "earth";
        case Element::Wind:
            return "wind";
    }
    return "normal";
}

constexpr std::string_view rankName(Rank rank)
{
    switch (rank)
    {
        case Rank::Normal:
            return "normal";
        case Rank::MiniBoss:
            return "miniboss";
        case Rank::Boss:
            return "boss";
        case Rank::SuperBoss:
            return "superboss";
    }
    return "normal";
}

constexpr std::string_view enemyTypeName(EnemyType type)
{
    switch (type)
    {
        case EnemyType::Octopus:
            return "octopus";
        case EnemyType::Orangutan:
            return "orangutan";
        case EnemyType::Cat:
            return "cat";
        case EnemyType::Dog:
            return "dog";
        case EnemyType::Duck:
            return "duck";
        case EnemyType::Frog:
            return "frog";
    }
    return "cat";
}

}  // namespace defender
