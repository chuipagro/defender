#pragma once

#include "defender/entities/Enemy.hpp"
#include "defender/entities/Entity.hpp"

#include <SFML/System/Vector2.hpp>

#include <memory>

namespace defender
{

/// Flies toward its target; does nothing if the target died meanwhile.
class Projectile : public Entity
{
public:
    Projectile(EntityId id, sf::Vector2f position, std::weak_ptr<Enemy> target, float damage,
               float speed);

    void update(float dt) override;
    void render(sf::RenderTarget& target) const override;

    [[nodiscard]] sf::Vector2f getPosition() const { return position_; }

private:
    sf::Vector2f position_;
    std::weak_ptr<Enemy> target_;
    float damage_;
    float speed_;
};

}  // namespace defender
