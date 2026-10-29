#pragma once

#include "defender/entities/Enemy.hpp"
#include "defender/entities/Entity.hpp"
#include "defender/entities/TowerDefinitions.hpp"

#include <SFML/System/Vector2.hpp>

#include <memory>
#include <string>

namespace defender
{

/// Flies toward its target (following it). If the target dies on the way,
/// it ends its flight where the target was.
///
/// When it arrives, hasImpact() becomes true: EntityManager then applies the
/// shot (damage, splash, tower effect) and removes it.
class Projectile : public Entity
{
public:
    Projectile(EntityId id, sf::Vector2f position, std::weak_ptr<Enemy> target, const Shot& shot,
               float speed);

    void update(float dt) override;
    void render(sf::RenderTarget& target) const override;

    [[nodiscard]] bool hasImpact() const { return impact_; }
    /// Marks the impact as handled (the projectile disappears).
    void consume() { alive_ = false; }

    [[nodiscard]] sf::Vector2f getPosition() const { return position_; }
    [[nodiscard]] const Shot& shot() const { return shot_; }
    /// The target, or null if it died or vanished.
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
    float angle_ = 0.f;  ///< Degrees, for rotating sprites
    float animationTime_ = 0.f;
    bool impact_ = false;
    std::string textureId_;
};

}  // namespace defender
