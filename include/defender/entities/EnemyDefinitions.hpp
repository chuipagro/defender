#pragma once

#include "defender/entities/Stats.hpp"
#include "defender/entities/Types.hpp"

#include <functional>
#include <initializer_list>
#include <string_view>
#include <utility>

namespace defender
{

class Enemy;

/// What an onDeath hook can do with the game: draw a random spec with the
/// biome's matrices, and spawn children where the dead enemy was.
/// WaveManager implements it.
class EnemySpawner
{
public:
    virtual ~EnemySpawner() = default;

    /// A random spec, following the current biome's probability matrices.
    [[nodiscard]] virtual EnemySpec rollSpec() = 0;
    /// Spawns `spec` on the path at the parent's place (spread a bit behind it).
    /// The child gets generation = parent.generation() + 1.
    virtual void spawnChild(const Enemy& parent, EnemySpec spec) = 0;
};

/// Functions called during an enemy's life. Each layer (species, element,
/// rank) has its own; all of them run, in that order. Leave one empty to skip it.
struct EnemyHooks
{
    /// Once, when the enemy is created.
    std::function<void(Enemy&)> onSpawn{};
    /// Every frame while alive (dt in seconds).
    std::function<void(Enemy&, float dt)> onUpdate{};
    /// Before damage is applied: returns the damage actually taken.
    /// Chained: each layer receives what the previous one returned.
    std::function<float(Enemy&, float damage)> onHit{};
    /// When killed by damage (not when reaching the base).
    std::function<void(Enemy&, EnemySpawner&)> onDeath{};
    /// When it reaches the base, right before the lives are lost.
    std::function<void(Enemy&)> onReachBase{};
};

/// A species gives the base stats (e.g. Dog: 80 hp, 125 px/s).
struct SpeciesDefinition
{
    std::string_view description{};
    EnemyStats base{};
    EnemyHooks hooks{};
};

/// An element or a rank changes the stats on top of the species:
///   final = base * mult(element) * mult(rank) + add(element) + add(rank)
struct ModifierDefinition
{
    std::string_view description{};
    EnemyStats mult{};  ///< 1 = unchanged
    EnemyStats add{};   ///< 0 = unchanged
    EnemyHooks hooks{};
};

/// Definitions of the game, written in EnemyDefinitions.cpp.
[[nodiscard]] const SpeciesDefinition& speciesDefinition(EnemyType type);
[[nodiscard]] const ModifierDefinition& elementDefinition(Element element);
[[nodiscard]] const ModifierDefinition& rankDefinition(Rank rank);

/// Final stats of a spec (species, element and rank combined, clamped:
/// health >= 1, other stats >= 0, scale in [0.25, 4]).
[[nodiscard]] EnemyStats computeStats(EnemySpec spec);

// ── Helpers to write definitions ──────────────────────────────────────────────

/// Base stats of a species.
[[nodiscard]] EnemyStats baseStats(float health, float speed, float reward, float damage,
                                   float scale = 1.f);
/// Multipliers: times({{Stat::Health, 4.f}}) = x4 health, everything else x1.
[[nodiscard]] EnemyStats times(std::initializer_list<std::pair<Stat, float>> changes);
/// Bonuses: plus({{Stat::Reward, 5.f}}) = +5 gold, everything else +0.
[[nodiscard]] EnemyStats plus(std::initializer_list<std::pair<Stat, float>> changes);

// ── Ready-made hooks, reusable in any layer ───────────────────────────────────

/// onHit: removes `armor` from each hit, but always lets 20% through.
[[nodiscard]] std::function<float(Enemy&, float)> armor(float armor);
/// onUpdate: heals `perSecond` hp per second, up to max health.
[[nodiscard]] std::function<void(Enemy&, float)> regeneration(float perSecond);
/// onDeath: spawns `count` children of rank `rank`, same element, random species
/// (biome matrices), unless the enemy is already at MAX_SPAWN_GENERATION.
[[nodiscard]] std::function<void(Enemy&, EnemySpawner&)> splitInto(int count, Rank rank);

// ── Loot ──────────────────────────────────────────────────────────────────────

/// Materials of its element dropped by a killed enemy. `roll` is a random
/// number in [0, 1). Normal enemies drop nothing.
[[nodiscard]] int materialDrop(EnemySpec spec, float roll);

/// Children of children... stop splitting at this depth.
inline constexpr int MAX_SPAWN_GENERATION = 3;

}  // namespace defender
