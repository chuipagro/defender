#pragma once

#include "defender/entities/Enemy.hpp"
#include "defender/entities/Entity.hpp"

#include <SFML/System/Vector2.hpp>

#include <memory>
#include <span>

namespace defender
{

struct TowerStats
{
    float range = 96.f;    ///< Pixels
    float fireRate = 1.f;  ///< Shots per second
    float damage = 10.f;
    int cost = 50;
};

class Tower : public Entity
{
public:
    Tower(EntityId id, TowerType type, sf::Vector2f position, const TowerStats& stats);

    void update(float dt) override;
    void render(sf::RenderTarget& target) const override;
    /// Picks a target among the enemies and fires if the cooldown allows it.
    void shoot(std::span<const std::shared_ptr<Enemy>> enemies);

    [[nodiscard]] TowerType type() const { return type_; }
    [[nodiscard]] sf::Vector2f getPosition() const { return position_; }

private:
    TowerType type_;
    sf::Vector2f position_;
    float range_;
    float fireRate_;
    float cooldown_ = 0.f;
    TowerStats stats_;
};

}  // namespace defender
