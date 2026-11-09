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

class EnemySpawner
{
public:
    virtual ~EnemySpawner() = default;
    [[nodiscard]] virtual EnemySpec rollSpec() = 0;
    virtual void spawnChild(const Enemy& parent, EnemySpec spec) = 0;
};


struct EnemyHooks
{
    std::function<void(Enemy&)> onSpawn{};
    std::function<void(Enemy&, float dt)> onUpdate{};
    std::function<float(Enemy&, float damage)> onHit{};
    std::function<void(Enemy&, EnemySpawner&)> onDeath{};
    std::function<void(Enemy&)> onReachBase{};
};

struct SpeciesDefinition
{
    std::string_view description{};
    EnemyStats base{};
    EnemyHooks hooks{};
};

struct ModifierDefinition
{
    std::string_view description{};
    EnemyStats mult{};
    EnemyStats add{};
    EnemyHooks hooks{};
};

[[nodiscard]] const SpeciesDefinition& speciesDefinition(EnemyType type);
[[nodiscard]] const ModifierDefinition& elementDefinition(Element element);
[[nodiscard]] const ModifierDefinition& rankDefinition(Rank rank);


[[nodiscard]] EnemyStats computeStats(EnemySpec spec);

// ── Helpers to write definitions ──────────────────────────────────────────────

[[nodiscard]] EnemyStats baseStats(float health, float speed, float reward, float damage,
                                   float scale = 1.f);
[[nodiscard]] EnemyStats times(std::initializer_list<std::pair<Stat, float>> changes);
[[nodiscard]] EnemyStats plus(std::initializer_list<std::pair<Stat, float>> changes);

// ── Ready-made hooks, reusable in any layer ───────────────────────────────────

[[nodiscard]] std::function<float(Enemy&, float)> armor(float armor);
[[nodiscard]] std::function<void(Enemy&, float)> regeneration(float perSecond);
[[nodiscard]] std::function<void(Enemy&, EnemySpawner&)> splitInto(int count, Rank rank);

// ── Loot ──────────────────────────────────────────────────────────────────────

[[nodiscard]] int materialDrop(EnemySpec spec, float roll);

inline constexpr int MAX_SPAWN_GENERATION = 3;

}  // namespace defender
