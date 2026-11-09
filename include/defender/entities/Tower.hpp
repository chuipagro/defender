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
class Tower : public Entity
{
public:
    Tower(EntityId id, TowerType type, GridPos tile, sf::Vector2f position,
          const TowerUpgrades& upgrades = {});

    void update(float dt) override;
    void render(sf::RenderTarget& target) const override;

    [[nodiscard]] std::shared_ptr<Enemy> pickTarget(
        std::span<const std::shared_ptr<Enemy>> enemies) const;
    [[nodiscard]] bool isReady() const { return cooldown_ <= 0.f; }
    Shot fire(const Enemy& target);

    bool upgrade(const TowerUpgrades& upgrades);
    void refreshStats(const TowerUpgrades& upgrades);
    void remove() { alive_ = false; }

    [[nodiscard]] TowerType type() const { return type_; }
    [[nodiscard]] GridPos tile() const { return tile_; }
    [[nodiscard]] int level() const { return level_; }
    [[nodiscard]] const TowerStats& stats() const { return stats_; }
    [[nodiscard]] sf::Vector2f getPosition() const { return position_; }
    [[nodiscard]] sf::Vector2f muzzle() const { return position_ - sf::Vector2f(0.f, 26.f); }

    static void drawGhost(sf::RenderTarget& target, TowerType type, sf::Vector2f position,
                          sf::Color tint);

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
    float sinceShot_ = 10.f;
    float animationTime_ = 0.f;
    bool facingLeft_ = false;
    std::string textureId_;
};

}  // namespace defender
