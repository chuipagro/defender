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
class Enemy : public Entity
{
public:
    Enemy(EntityId id, EnemySpec spec, std::span<const sf::Vector2f> path, int generation = 0,
          EnemyBoost boost = {});

    void update(float dt) override;
    void render(sf::RenderTarget& target) const override;
    float takeDamage(float amount, bool ignoreHooks = false);
    void applySlow(float factor, float seconds);
    void applyBurn(float damagePerSecond, float seconds);
    void heal(float amount);
    void setHealth(float health);
    void runDeathHooks(EnemySpawner& spawner);

    void placeOnPath(std::size_t index, sf::Vector2f position);
    void moveBack(float distance);
    /// Sideways offset in pixels from the path center (drawing only).
    void setLane(float offset) { lane_ = offset; }

    [[nodiscard]] bool contains(sf::Vector2f point) const;

    [[nodiscard]] sf::Vector2f getPosition() const { return position_; }
    [[nodiscard]] EnemySpec spec() const { return spec_; }
    [[nodiscard]] EnemyType type() const { return spec_.type; }
    [[nodiscard]] Element element() const { return spec_.element; }
    [[nodiscard]] Rank rank() const { return spec_.rank; }
    [[nodiscard]] int generation() const { return generation_; }
    [[nodiscard]] EnemyBoost boost() const { return boost_; }
    [[nodiscard]] std::size_t pathIndex() const { return pathIndex_; }
    [[nodiscard]] std::span<const sf::Vector2f> path() const { return path_; }

    /// Current stats. Hooks may change them (e.g. a speed boost).
    [[nodiscard]] EnemyStats& stats() { return stats_; }
    [[nodiscard]] const EnemyStats& stats() const { return stats_; }
    [[nodiscard]] float health() const { return health_; }
    [[nodiscard]] float maxHealth() const { return stats_.health(); }
    [[nodiscard]] float speed() const { return stats_.speed(); }
    [[nodiscard]] float currentSpeed() const;
    [[nodiscard]] bool isSlowed() const { return slowTimer_ > 0.f; }
    [[nodiscard]] bool isBurning() const { return burnTimer_ > 0.f; }
    [[nodiscard]] float progress() const { return progress_; }
    [[nodiscard]] int reward() const { return stats_.reward(); }
    [[nodiscard]] float damage() const { return stats_.damage(); }
    [[nodiscard]] float scale() const { return stats_.scale(); }

    [[nodiscard]] float& memory(std::size_t slot) { return memory_.at(slot); }

    [[nodiscard]] bool hasReachedEnd() const { return reachedEnd_; }

    static constexpr int FRAME_SIZE = 32;
    static constexpr int FRAME_COUNT = 4;
    static constexpr float FRAME_TIME = 0.12f;
    static constexpr std::size_t MEMORY_SLOTS = 4;
    static constexpr std::size_t MEMORY_SPECIES = 0;
    static constexpr std::size_t MEMORY_ELEMENT = 1;
    static constexpr std::size_t MEMORY_RANK = 2;
    static constexpr std::size_t MEMORY_EXTRA = 3;

private:
    [[nodiscard]] std::array<const EnemyHooks*, 3> layers() const;
    void renderHealthBar(sf::RenderTarget& target) const;
    [[nodiscard]] sf::Vector2f drawPosition() const;

    EnemySpec spec_;
    int generation_;
    EnemyBoost boost_;
    EnemyStats stats_;
    std::span<const sf::Vector2f> path_;
    std::size_t pathIndex_ = 0;
    sf::Vector2f position_;
    float health_;
    bool reachedEnd_ = false;
    bool facingLeft_ = false;
    float animationTime_ = 0.f;
    std::array<float, MEMORY_SLOTS> memory_{};
    float progress_ = 0.f;
    float slowFactor_ = 1.f;
    float slowTimer_ = 0.f;
    float burnDps_ = 0.f;
    float burnTimer_ = 0.f;
    float lane_ = 0.f;
    std::string textureId_;
};

}  // namespace defender
