#pragma once

#include "defender/entities/Enemy.hpp"
#include "defender/entities/Entity.hpp"
#include "defender/entities/TowerDefinitions.hpp"

#include <SFML/System/Vector2.hpp>

#include <memory>
#include <string>

namespace defender
{
class Projectile : public Entity
{
public:
    Projectile(EntityId id, sf::Vector2f position, std::weak_ptr<Enemy> target, const Shot& shot,
               float speed);

    void update(float dt) override;
    void render(sf::RenderTarget& target) const override;

    [[nodiscard]] bool hasImpact() const { return impact_; }
    void consume() { alive_ = false; }

    [[nodiscard]] sf::Vector2f getPosition() const { return position_; }
    [[nodiscard]] const Shot& shot() const { return shot_; }
    [[nodiscard]] std::shared_ptr<Enemy> target() const;

    static constexpr int FRAME_SIZE = 16;
    static constexpr int FRAME_COUNT = 4;
    static constexpr float FRAME_TIME = 0.08f;

private:
    sf::Vector2f position_;
    std::weak_ptr<Enemy> target_;
    sf::Vector2f lastTargetPosition_;
    Shot shot_;
    float speed_;
    float angle_ = 0.f;
    float animationTime_ = 0.f;
    bool impact_ = false;
    std::string textureId_;
};

}  // namespace defender
