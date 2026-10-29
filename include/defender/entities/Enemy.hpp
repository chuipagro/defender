#pragma once

#include "defender/entities/EnemyDefinitions.hpp"
#include "defender/entities/Entity.hpp"
#include "defender/entities/Stats.hpp"

#include <SFML/System/Vector2.hpp>

#include <array>
#include <cstddef>
#include <span>
#include <string>

namespace defender
{

/// Walks along the map path. Its look is the <type> sprite in <element>
/// colors, drawn bigger for higher ranks.
///
/// Stats and behaviors come from EnemyDefinitions.cpp: species + element + rank.
class Enemy : public Entity
{
public:
    /// `generation` = how many splits led to this enemy (0 for a wave enemy).
    Enemy(EntityId id, EnemySpec spec, std::span<const sf::Vector2f> path, int generation = 0);

    void update(float dt) override;
    void render(sf::RenderTarget& target) const override;

    /// Applies damage after the onHit hooks (armor...). Returns the damage taken.
    /// Emits EnemyKilled when it dies.
    float takeDamage(float amount);
    /// Restores health, up to max health.
    void heal(float amount);
    /// Runs the onDeath hooks of every layer (called by WaveManager on EnemyKilled).
    void runDeathHooks(EnemySpawner& spawner);

    /// Puts the enemy on the path segment starting at `index`, at `position`.
    void placeOnPath(std::size_t index, sf::Vector2f position);
    /// Walks `distance` pixels back along the path (stops at the start).
    void moveBack(float distance);

    /// True if `point` is on the enemy's body (for clicks).
    [[nodiscard]] bool contains(sf::Vector2f point) const;

    [[nodiscard]] sf::Vector2f getPosition() const { return position_; }
    [[nodiscard]] EnemySpec spec() const { return spec_; }
    [[nodiscard]] EnemyType type() const { return spec_.type; }
    [[nodiscard]] Element element() const { return spec_.element; }
    [[nodiscard]] Rank rank() const { return spec_.rank; }
    [[nodiscard]] int generation() const { return generation_; }
    [[nodiscard]] std::size_t pathIndex() const { return pathIndex_; }
    [[nodiscard]] std::span<const sf::Vector2f> path() const { return path_; }

    /// Current stats. Hooks may change them (e.g. a speed boost).
    [[nodiscard]] EnemyStats& stats() { return stats_; }
    [[nodiscard]] const EnemyStats& stats() const { return stats_; }
    [[nodiscard]] float health() const { return health_; }
    [[nodiscard]] float maxHealth() const { return stats_.health(); }
    [[nodiscard]] float speed() const { return stats_.speed(); }
    [[nodiscard]] int reward() const { return stats_.reward(); }
    [[nodiscard]] float damage() const { return stats_.damage(); }
    [[nodiscard]] float scale() const { return stats_.scale(); }

    /// Free slots for hooks to remember things (all start at 0).
    [[nodiscard]] float& memory(std::size_t slot) { return memory_.at(slot); }

    /// True once it walked to the last point of the path.
    [[nodiscard]] bool hasReachedEnd() const { return reachedEnd_; }

    /// Sprite sheet: 4 frames of 32x32, facing right.
    static constexpr int FRAME_SIZE = 32;
    static constexpr int FRAME_COUNT = 4;
    static constexpr float FRAME_TIME = 0.12f;
    static constexpr std::size_t MEMORY_SLOTS = 4;

private:
    /// Species, element and rank hooks, in that order.
    [[nodiscard]] std::array<const EnemyHooks*, 3> layers() const;
    void renderHealthBar(sf::RenderTarget& target) const;

    EnemySpec spec_;
    int generation_;
    EnemyStats stats_;
    std::span<const sf::Vector2f> path_;
    std::size_t pathIndex_ = 0;
    sf::Vector2f position_;
    float health_;
    bool reachedEnd_ = false;
    bool facingLeft_ = false;
    float animationTime_ = 0.f;
    std::array<float, MEMORY_SLOTS> memory_{};
    std::string textureId_;  ///< "sprites/enemies/<element>/<type>.png"
};

}  // namespace defender
