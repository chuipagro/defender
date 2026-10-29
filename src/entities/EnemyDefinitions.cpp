#include "defender/entities/EnemyDefinitions.hpp"

#include "defender/entities/Enemy.hpp"

#include <algorithm>
#include <array>

namespace defender
{

// ═════════════════════════════════════════════════════════════════════════════
//  Enemy definitions — edit this file to balance or add behaviors.
//
//  An enemy = species + element + rank. Each layer gives stats and hooks
//  (functions run on spawn, every frame, on hit, on death, at the base).
//  A hook receives the Enemy: it can read/change its stats, heal it, use its
//  memory slots (species: MEMORY_SPECIES, element: MEMORY_ELEMENT,
//  rank: MEMORY_RANK, so that layers never overwrite each other), etc.
// ═════════════════════════════════════════════════════════════════════════════

namespace
{

// ── Species ───────────────────────────────────────────────────────────────────

const std::array<SpeciesDefinition, ENEMY_TYPE_COUNT>& speciesTable()
{
    //                              health  speed  reward  damage
    static const std::array<SpeciesDefinition, ENEMY_TYPE_COUNT> table = {{
        {.description = "Octopus: hops, average.",
         .base = baseStats(60.f, 110.f, 8.f, 1.f)},

        {.description = "Orangutan: slow tank with a thick hide (armor 2).",
         .base = baseStats(150.f, 70.f, 15.f, 2.f),
         .hooks = {.onHit = armor(2.f)}},

        {.description = "Cat: fast. Nine lives: survives its first deadly hit with 1 hp.",
         .base = baseStats(50.f, 150.f, 6.f, 1.f),
         .hooks = {.onHit =
                       [](Enemy& cat, float damage)
                   {
                       float& usedLife = cat.memory(Enemy::MEMORY_SPECIES);
                       if (usedLife == 0.f && damage >= cat.health())
                       {
                           usedLife = 1.f;
                           return cat.health() - 1.f;
                       }
                       return damage;
                   }}},

        {.description = "Dog: balanced. Runs 30% faster once below half health.",
         .base = baseStats(80.f, 125.f, 8.f, 1.f),
         .hooks = {.onUpdate =
                       [](Enemy& dog, float /*dt*/)
                   {
                       float& enraged = dog.memory(Enemy::MEMORY_SPECIES);
                       if (enraged == 0.f && dog.health() < dog.maxHealth() * 0.5f)
                       {
                           enraged = 1.f;
                           dog.stats()[Stat::Speed] *= 1.3f;
                       }
                   }}},

        {.description = "Duck: fragile and quick.",
         .base = baseStats(40.f, 135.f, 5.f, 1.f)},

        {.description = "Frog: hops.",
         .base = baseStats(55.f, 120.f, 7.f, 1.f)},
    }};
    return table;
}

// ── Elements ──────────────────────────────────────────────────────────────────
//
// Elemental enemies are stronger than normal ones ("global" = health, gold and
// damage) and each has a bonus effect.

/// Health, gold and damage multiplied by `factor`, speed by `speed`.
EnemyStats global(float factor, float speed = 1.f)
{
    return times({{Stat::Health, factor},
                  {Stat::Reward, factor},
                  {Stat::Damage, factor},
                  {Stat::Speed, speed}});
}

const std::array<ModifierDefinition, ELEMENT_COUNT>& elementTable()
{
    static const std::array<ModifierDefinition, ELEMENT_COUNT> table = {{
        {.description = "Normal: natural colors, base stats.",
         .mult = times({}),
         .add = plus({})},

        {.description = "Water: x1.5 stats, +5 gold. Leaves a normal enemy behind when killed.",
         .mult = global(1.5f),
         .add = plus({{Stat::Reward, 5.f}}),
         .hooks = {.onDeath =
                       [](Enemy& enemy, EnemySpawner& spawner)
                   {
                       spawner.spawnChild(enemy, {enemy.type(), Element::Normal, Rank::Normal});
                   }}},

        {.description = "Fire: x1.4 stats, +20% speed. Rises from its ashes once (40% health).",
         .mult = global(1.4f, 1.2f),
         .add = plus({}),
         .hooks = {.onHit =
                       [](Enemy& enemy, float damage)
                   {
                       float& reborn = enemy.memory(Enemy::MEMORY_ELEMENT);
                       if (reborn == 0.f && damage >= enemy.health())
                       {
                           reborn = 1.f;
                           enemy.setHealth(enemy.maxHealth() * 0.4f);
                           return 0.f;
                       }
                       return damage;
                   }}},

        {.description = "Earth: x1.5 stats, -15% speed. Stone skin: armor 3.",
         .mult = global(1.5f, 0.85f),
         .add = plus({}),
         .hooks = {.onHit = armor(3.f)}},

        {.description = "Wind: x1.3 stats, +35% speed. Dodges one hit out of 5.",
         .mult = global(1.3f, 1.35f),
         .add = plus({}),
         .hooks = {.onHit =
                       [](Enemy& enemy, float damage)
                   {
                       float& hits = enemy.memory(Enemy::MEMORY_ELEMENT);
                       hits += 1.f;
                       return static_cast<int>(hits) % 5 == 0 ? 0.f : damage;
                   }}},
    }};
    return table;
}

// ── Ranks ─────────────────────────────────────────────────────────────────────

const std::array<ModifierDefinition, RANK_COUNT>& rankTable()
{
    static const std::array<ModifierDefinition, RANK_COUNT> table = {{
        {.description = "Normal.", .mult = times({}), .add = plus({})},

        {.description = "Mini-boss: x4 health, x3 gold and damage, a bit slower, bigger.",
         .mult = times({{Stat::Health, 4.f},
                        {Stat::Reward, 3.f},
                        {Stat::Damage, 3.f},
                        {Stat::Speed, 0.9f},
                        {Stat::Scale, 1.4f}}),
         .add = plus({})},

        {.description = "Boss: x12 health, x10 gold, x5 damage, armor 3, huge.",
         .mult = times({{Stat::Health, 12.f},
                        {Stat::Reward, 10.f},
                        {Stat::Damage, 5.f},
                        {Stat::Speed, 0.75f},
                        {Stat::Scale, 1.9f}}),
         .add = plus({}),
         .hooks = {.onHit = armor(3.f)}},

        {.description = "Super boss: x25 health, x20 gold, armor 5. "
                        "Splits into 3 mini-bosses of its element when killed.",
         .mult = times({{Stat::Health, 25.f},
                        {Stat::Reward, 20.f},
                        {Stat::Damage, 10.f},
                        {Stat::Speed, 0.6f},
                        {Stat::Scale, 2.4f}}),
         .add = plus({}),
         .hooks = {.onHit = armor(5.f), .onDeath = splitInto(3, Rank::MiniBoss)}},
    }};
    return table;
}

EnemyStats filled(float value, std::initializer_list<std::pair<Stat, float>> changes)
{
    EnemyStats stats;
    stats.values.fill(value);
    for (const auto& [stat, amount] : changes)
    {
        stats.set(stat, amount);
    }
    return stats;
}

}  // namespace

// ── Lookups ───────────────────────────────────────────────────────────────────

const SpeciesDefinition& speciesDefinition(EnemyType type)
{
    return speciesTable()[static_cast<std::size_t>(type)];
}

const ModifierDefinition& elementDefinition(Element element)
{
    return elementTable()[static_cast<std::size_t>(element)];
}

const ModifierDefinition& rankDefinition(Rank rank)
{
    return rankTable()[static_cast<std::size_t>(rank)];
}

EnemyStats computeStats(EnemySpec spec)
{
    const SpeciesDefinition& species = speciesDefinition(spec.type);
    const ModifierDefinition& element = elementDefinition(spec.element);
    const ModifierDefinition& rank = rankDefinition(spec.rank);

    EnemyStats stats;
    for (std::size_t i = 0; i < STAT_COUNT; ++i)
    {
        stats.values[i] = (species.base.values[i] * element.mult.values[i] * rank.mult.values[i]) +
                          element.add.values[i] + rank.add.values[i];
        stats.values[i] = std::max(stats.values[i], 0.f);
    }
    stats[Stat::Health] = std::max(stats.health(), 1.f);
    stats[Stat::Scale] = std::clamp(stats.scale(), 0.25f, 4.f);
    return stats;
}

// ── Helpers ───────────────────────────────────────────────────────────────────

EnemyStats baseStats(float health, float speed, float reward, float damage, float scale)
{
    return filled(0.f, {{Stat::Health, health},
                        {Stat::Speed, speed},
                        {Stat::Reward, reward},
                        {Stat::Damage, damage},
                        {Stat::Scale, scale}});
}

EnemyStats times(std::initializer_list<std::pair<Stat, float>> changes)
{
    return filled(1.f, changes);
}

EnemyStats plus(std::initializer_list<std::pair<Stat, float>> changes)
{
    return filled(0.f, changes);
}

std::function<float(Enemy&, float)> armor(float armor)
{
    return [armor](Enemy& /*enemy*/, float damage)
    { return std::max(damage - armor, damage * 0.2f); };
}

std::function<void(Enemy&, float)> regeneration(float perSecond)
{
    return [perSecond](Enemy& enemy, float dt) { enemy.heal(perSecond * dt); };
}

std::function<void(Enemy&, EnemySpawner&)> splitInto(int count, Rank rank)
{
    return [count, rank](Enemy& enemy, EnemySpawner& spawner)
    {
        if (enemy.generation() >= MAX_SPAWN_GENERATION)
        {
            return;
        }
        for (int i = 0; i < count; ++i)
        {
            EnemySpec child = spawner.rollSpec();
            child.element = enemy.element();
            child.rank = rank;
            spawner.spawnChild(enemy, child);
        }
    };
}

int materialDrop(EnemySpec spec, float roll)
{
    if (spec.element == Element::Normal)
    {
        return 0;
    }
    switch (spec.rank)
    {
        case Rank::Normal:
            return roll < 0.35f ? 1 : 0;  // 35% chance
        case Rank::MiniBoss:
            return 2;
        case Rank::Boss:
            return 5;
        case Rank::SuperBoss:
            return 10;
    }
    return 0;
}

}  // namespace defender
