#pragma once

#include "defender/core/EventBus.hpp"
#include "defender/entities/EnemyDefinitions.hpp"
#include "defender/entities/EnemyFactory.hpp"
#include "defender/managers/EntityManager.hpp"

#include <SFML/System/Vector2.hpp>

#include <cstddef>
#include <span>
#include <vector>

namespace defender
{

struct SpawnEntry
{
    EnemySpec spec;
    int count = 1;
    float delay = 0.f;  ///< Seconds before this entry starts
};

struct WaveData
{
    std::vector<SpawnEntry> enemies;
    float spawnInterval = 1.f;  ///< Seconds between two enemies
};

/// Builds waves with the EnemyFactory and spawns them over time.
/// Also runs the onDeath hooks of killed enemies, and spawns their children.
///
/// Not copyable nor movable: it is subscribed to the EventBus with `this`.
class WaveManager : public EnemySpawner
{
public:
    WaveManager(EntityManager& entityManager, EnemyFactory& factory,
                std::span<const sf::Vector2f> path, const WaveRules& rules = WAVE_RULES);
    ~WaveManager() override;

    WaveManager(const WaveManager&) = delete;
    WaveManager& operator=(const WaveManager&) = delete;
    WaveManager(WaveManager&&) = delete;
    WaveManager& operator=(WaveManager&&) = delete;

    /// Spawns the enemies of the current wave as their time comes, and the
    /// children of enemies killed since the last update.
    void update(float dt);
    /// Builds the next wave with the factory and starts spawning it.
    void startNextWave();
    /// True once every enemy of the current wave spawned (children included)
    /// and none is alive.
    [[nodiscard]] bool isWaveComplete() const;
    [[nodiscard]] int getCurrentWave() const { return currentWave_; }

    /// Enemies of the current wave already spawned / in total (children not counted).
    [[nodiscard]] int spawnedCount() const { return spawned_; }
    [[nodiscard]] int waveSize() const { return total_; }
    /// The current wave's plan.
    [[nodiscard]] const WaveData* currentWaveData() const;

    /// Number of enemies in a given wave (wave 1 = 30).
    [[nodiscard]] int enemiesInWave(int wave) const;
    [[nodiscard]] const WaveRules& rules() const { return rules_; }

    // EnemySpawner (used by onDeath hooks)
    [[nodiscard]] EnemySpec rollSpec() override;
    void spawnChild(const Enemy& parent, EnemySpec spec) override;

    /// Distance in pixels between two children of the same parent.
    static constexpr float CHILD_SPACING = 14.f;

private:
    struct PendingChild
    {
        EnemySpec spec;
        std::size_t pathIndex;
        sf::Vector2f position;
        int generation;
        float backOffset;
    };

    /// Rolls the factory to fill a wave of `count` enemies.
    [[nodiscard]] WaveData buildWave(int count);
    [[nodiscard]] bool allSpawned() const;
    void spawnPendingChildren();

    WaveRules rules_;
    std::vector<WaveData> waves_;
    std::vector<PendingChild> pendingChildren_;
    float spawnTimer_ = 0.f;
    int currentWave_ = 0;
    std::size_t nextEntry_ = 0;  ///< Entry of the current wave being spawned
    int spawnedInEntry_ = 0;
    int spawned_ = 0;
    int total_ = 0;
    EntityManager& entityManager_;
    EnemyFactory& factory_;
    std::span<const sf::Vector2f> path_;
    EventBus::HandlerId killedHandler_;
    /// Children already queued for the enemy being handled (to spread them out).
    int childrenOfCurrentParent_ = 0;
};

}  // namespace defender
