#pragma once

#include "defender/entities/Enemy.hpp"
#include "defender/entities/Entity.hpp"
#include "defender/entities/TowerDefinitions.hpp"
#include "defender/map/Tile.hpp"

#include <SFML/System/Vector2.hpp>

#include <memory>
#include <span>
#include <string>

namespace defender
{

/// A tower on a buildable tile. Shoots the most advanced enemy in range.
/// Stats come from TowerDefinitions.cpp (type x level x skill tree).
class Tower : public Entity
{
public:
    /// `position` is the center of `tile` in pixels.
    Tower(EntityId id, TowerType type, GridPos tile, sf::Vector2f position,
          const TowerUpgrades& upgrades = {});

    void update(float dt) override;
    void render(sf::RenderTarget& target) const override;

    /// The enemy to shoot: alive, in range, furthest along the path. Null if none.
    [[nodiscard]] std::shared_ptr<Enemy> pickTarget(
        std::span<const std::shared_ptr<Enemy>> enemies) const;
    /// True when the cooldown is over.
    [[nodiscard]] bool isReady() const { return cooldown_ <= 0.f; }
    /// Starts the cooldown, turns toward `target` and returns what to shoot.
    Shot fire(const Enemy& target);

    /// Raises the level by one (the caller pays). False at max level.
    bool upgrade(const TowerUpgrades& upgrades);
    /// Recomputes the stats (after a skill tree purchase).
    void refreshStats(const TowerUpgrades& upgrades);
    /// Removes the tower (sold).
    void remove() { alive_ = false; }

    [[nodiscard]] TowerType type() const { return type_; }
    [[nodiscard]] GridPos tile() const { return tile_; }
    [[nodiscard]] int level() const { return level_; }
    [[nodiscard]] const TowerStats& stats() const { return stats_; }
    [[nodiscard]] sf::Vector2f getPosition() const { return position_; }
    /// Where projectiles start (top of the tower).
    [[nodiscard]] sf::Vector2f muzzle() const { return position_ - sf::Vector2f(0.f, 26.f); }

    /// Draws a tower sprite (first frame) with its feet on the tile centered
    /// at `position`. Used for the placement preview.
    static void drawGhost(sf::RenderTarget& target, TowerType type, sf::Vector2f position,
                          sf::Color tint);

    /// Sprite sheet: 4 frames of 32x48.
    static constexpr int FRAME_W = 32;
    static constexpr int FRAME_H = 48;
    static constexpr int FRAME_COUNT = 4;
    static constexpr float FRAME_TIME = 0.15f;

private:
    TowerType type_;
    GridPos tile_;
    sf::Vector2f position_;
    int level_ = 1;
    TowerStats stats_;
    float cooldown_ = 0.f;
    float sinceShot_ = 10.f;  ///< Seconds since the last shot (animation)
    float animationTime_ = 0.f;
    bool facingLeft_ = false;
    std::string textureId_;
};

}  // namespace defender
