#include "defender/entities/Projectile.hpp"

#include "defender/core/Concepts.hpp"
#include "defender/managers/ResourceManager.hpp"

#include <SFML/Graphics/Sprite.hpp>

#include <cmath>
#include <numbers>
#include <utility>

namespace defender
{

static_assert(Updatable<Projectile>);
static_assert(Renderable<Projectile>);

namespace
{

/// Projectiles aim at the body, not at the feet.
constexpr float BODY_HEIGHT = 10.f;

sf::Vector2f aimPoint(const Enemy& enemy)
{
    return enemy.getPosition() - sf::Vector2f(0.f, BODY_HEIGHT * enemy.scale());
}

}  // namespace

Projectile::Projectile(EntityId id, sf::Vector2f position, std::weak_ptr<Enemy> target,
                       const Shot& shot, float speed)
    : Entity(id),
      position_(position),
      target_(std::move(target)),
      lastTargetPosition_(position),
      shot_(shot),
      speed_(speed),
      textureId_("sprites/projectiles/" + std::string(towerDefinition(shot.type).projectile) +
                 ".png")
{
    if (const auto enemy = target_.lock())
    {
        lastTargetPosition_ = aimPoint(*enemy);
    }
}

std::shared_ptr<Enemy> Projectile::target() const
{
    auto enemy = target_.lock();
    return enemy && enemy->isAlive() ? enemy : nullptr;
}

void Projectile::update(float dt)
{
    if (!alive_ || impact_)
    {
        return;
    }
    animationTime_ += dt;
    if (const auto enemy = target())
    {
        lastTargetPosition_ = aimPoint(*enemy);
    }
    const sf::Vector2f delta = lastTargetPosition_ - position_;
    const float distance = std::hypot(delta.x, delta.y);
    const float step = speed_ * dt;
    if (distance > 0.f)
    {
        angle_ = std::atan2(delta.y, delta.x) * 180.f / std::numbers::pi_v<float>;
    }
    if (distance <= step)
    {
        position_ = lastTargetPosition_;
        impact_ = true;
    }
    else
    {
        position_ += delta * (step / distance);
    }
}

void Projectile::render(sf::RenderTarget& target) const
{
    const auto frame = static_cast<int>(animationTime_ / FRAME_TIME) % FRAME_COUNT;
    sf::Sprite sprite(ResourceManager::instance().getTexture(textureId_),
                      sf::IntRect(frame * FRAME_SIZE, 0, FRAME_SIZE, FRAME_SIZE));
    sprite.setOrigin(FRAME_SIZE / 2.f, FRAME_SIZE / 2.f);
    if (towerDefinition(shot_.type).rotateProjectile)
    {
        sprite.setRotation(angle_);
    }
    sprite.setPosition(std::round(position_.x), std::round(position_.y));
    target.draw(sprite);
}

}  // namespace defender
