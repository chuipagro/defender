#include "defender/entities/TowerDefinitions.hpp"

#include "defender/entities/Enemy.hpp"

#include <algorithm>
#include <cmath>

namespace defender
{

// ═════════════════════════════════════════════════════════════════════════════
//  Tower definitions — edit this file to balance towers.
//
//  Placing costs gold only. Upgrading a normal tower costs gold; upgrading an
//  elemental tower costs gold + materials of its element (dropped by enemies
//  of that element).
// ═════════════════════════════════════════════════════════════════════════════

namespace
{

/// Per level: damage +50%, range +10%, fire rate +15%, effects +25%.
constexpr float DAMAGE_PER_LEVEL = 0.5f;
constexpr float RANGE_PER_LEVEL = 0.1f;
constexpr float RATE_PER_LEVEL = 0.15f;
constexpr float STRENGTH_PER_LEVEL = 0.25f;
constexpr float SELL_REFUND = 0.6f;

const std::array<TowerDefinition, ELEMENT_COUNT>& towerTable()
{
    static const std::array<TowerDefinition, ELEMENT_COUNT> table = {{
        {.name = "Tour normale",
         .description = "Tire des flèches rapides.",
         .projectile = "arrow",
         .cost = 50,
         .damage = 12.f,
         .range = 96.f,
         .fireRate = 1.2f,
         .projectileSpeed = 360.f,
         .upgrades = {{{.gold = 60}, {.gold = 120}}}},

        {.name = "Tour d'eau",
         .description = "Bulles qui ralentissent de 40%.",
         .projectile = "bubble",
         .rotateProjectile = false,
         .cost = 80,
         .damage = 8.f,
         .range = 96.f,
         .fireRate = 1.f,
         .projectileSpeed = 240.f,
         .upgrades = {{{.gold = 90, .material = Element::Water, .materials = 3},
                       {.gold = 180, .material = Element::Water, .materials = 8}}},
         .onHit = [](Enemy& enemy, const Shot& shot)
         { enemy.applySlow(0.6f, 1.5f * shot.strength); }},

        {.name = "Tour de feu",
         .description = "Boules de feu qui brûlent (dégâts sur 3 s).",
         .projectile = "fireball",
         .cost = 90,
         .damage = 10.f,
         .range = 96.f,
         .fireRate = 0.9f,
         .projectileSpeed = 260.f,
         .upgrades = {{{.gold = 100, .material = Element::Fire, .materials = 3},
                       {.gold = 200, .material = Element::Fire, .materials = 8}}},
         .onHit = [](Enemy& enemy, const Shot& shot)
         { enemy.applyBurn(6.f * shot.strength, 3.f); }},

        {.name = "Tour de terre",
         .description = "Rochers lents, dégâts de zone.",
         .projectile = "rock",
         .rotateProjectile = false,
         .cost = 100,
         .damage = 20.f,
         .range = 80.f,
         .fireRate = 0.6f,
         .projectileSpeed = 200.f,
         .splash = 40.f,
         .upgrades = {{{.gold = 110, .material = Element::Earth, .materials = 3},
                       {.gold = 220, .material = Element::Earth, .materials = 8}}}},

        {.name = "Tour de vent",
         .description = "Bourrasques qui repoussent les ennemis.",
         .projectile = "gust",
         .cost = 85,
         .damage = 6.f,
         .range = 112.f,
         .fireRate = 1.4f,
         .projectileSpeed = 320.f,
         .upgrades = {{{.gold = 95, .material = Element::Wind, .materials = 3},
                       {.gold = 190, .material = Element::Wind, .materials = 8}}},
         .onHit = [](Enemy& enemy, const Shot& shot)
         {
             // Big enemies are harder to push.
             enemy.moveBack(20.f * shot.strength / enemy.scale());
         }},
    }};
    return table;
}

}  // namespace

const TowerDefinition& towerDefinition(TowerType type)
{
    return towerTable()[static_cast<std::size_t>(type)];
}

TowerStats towerStats(TowerType type, int level, const TowerUpgrades& upgrades)
{
    const TowerDefinition& def = towerDefinition(type);
    const TowerBonus& bonus = upgrades.of(type);
    const auto extra = static_cast<float>(std::clamp(level, 1, TOWER_MAX_LEVEL) - 1);
    return {.damage = def.damage * (1.f + (DAMAGE_PER_LEVEL * extra)) * bonus.damage,
            .range = def.range * (1.f + (RANGE_PER_LEVEL * extra)) * bonus.range,
            .fireRate = def.fireRate * (1.f + (RATE_PER_LEVEL * extra)) * bonus.fireRate,
            .projectileSpeed = def.projectileSpeed,
            .splash = def.splash * (1.f + (STRENGTH_PER_LEVEL * extra)),
            .strength = 1.f + (STRENGTH_PER_LEVEL * extra)};
}

Cost placeCost(TowerType type)
{
    return {.gold = towerDefinition(type).cost};
}

Cost upgradeCost(TowerType type, int level)
{
    if (level < 1 || level >= TOWER_MAX_LEVEL)
    {
        return {};
    }
    return towerDefinition(type).upgrades[static_cast<std::size_t>(level - 1)];
}

int sellValue(TowerType type, int level)
{
    int spent = placeCost(type).gold;
    for (int l = 1; l < std::clamp(level, 1, TOWER_MAX_LEVEL); ++l)
    {
        spent += upgradeCost(type, l).gold;
    }
    return static_cast<int>(std::lround(static_cast<float>(spent) * SELL_REFUND));
}

}  // namespace defender
