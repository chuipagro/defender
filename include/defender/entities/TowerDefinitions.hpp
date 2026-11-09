#pragma once

#include "defender/entities/Types.hpp"

#include <array>
#include <cstddef>
#include <functional>
#include <string_view>

namespace defender
{

class Enemy;

struct Cost
{
    int gold = 0;
    Element material = Element::Normal;  ///< Normal = no material needed
    int materials = 0;
};

struct TowerBonus
{
    float damage = 1.f;
    float range = 1.f;
    float fireRate = 1.f;
};

struct TowerUpgrades
{
    std::array<bool, ELEMENT_COUNT> unlocked{};     ///< Indexed by TowerType
    std::array<TowerBonus, ELEMENT_COUNT> bonus{};  ///< Indexed by TowerType

    [[nodiscard]] bool isUnlocked(TowerType type) const
    {
        return unlocked[static_cast<std::size_t>(type)];
    }
    [[nodiscard]] TowerBonus& of(TowerType type) { return bonus[static_cast<std::size_t>(type)]; }
    [[nodiscard]] const TowerBonus& of(TowerType type) const
    {
        return bonus[static_cast<std::size_t>(type)];
    }
};

struct Shot
{
    TowerType type = TowerType::Normal;
    float damage = 0.f;
    float splash = 0.f;    ///< Radius in pixels (0 = only the target)
    float strength = 1.f;  ///< Effect power (grows with the tower level)
};

struct TowerStats
{
    float damage = 0.f;
    float range = 0.f;            ///< Pixels
    float fireRate = 0.f;         ///< Shots per second
    float projectileSpeed = 0.f;  ///< Pixels per second
    float splash = 0.f;           ///< Pixels
    float strength = 1.f;
};

inline constexpr int TOWER_MAX_LEVEL = 5;

/// One tower type. Defined in TowerDefinitions.cpp.
struct TowerDefinition
{
    std::string_view name{};
    std::string_view description{};
    std::string_view projectile{};      ///< assets/sprites/projectiles/<name>.png
    bool rotateProjectile = true;       ///< Turn the sprite toward the target
    int cost = 50;                      ///< Gold to place it
    float damage = 10.f;
    float range = 96.f;                 ///< Pixels (a tile is 32)
    float fireRate = 1.f;               ///< Shots per second
    float projectileSpeed = 300.f;
    float splash = 0.f;
    /// Gold for each upgrade: [0] = level 1 -> 2, [1] = level 2 -> 3...
    std::array<int, TOWER_MAX_LEVEL - 1> upgrades{};
    /// Effect on each enemy hit (after the damage). Empty = damage only.
    std::function<void(Enemy&, const Shot&)> onHit{};
};

[[nodiscard]] const TowerDefinition& towerDefinition(TowerType type);

[[nodiscard]] TowerStats towerStats(TowerType type, int level, const TowerUpgrades& upgrades = {});

[[nodiscard]] Cost placeCost(TowerType type);
[[nodiscard]] Cost upgradeCost(TowerType type, int level);
[[nodiscard]] int sellValue(TowerType type, int level);

}  // namespace defender
