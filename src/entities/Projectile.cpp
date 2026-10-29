#include "defender/entities/Projectile.hpp"

#include "defender/core/Concepts.hpp"

#include <utility>

namespace defender
{

static_assert(Updatable<Projectile>);
static_assert(Renderable<Projectile>);

Projectile::Projectile(EntityId id, sf::Vector2f position, std::weak_ptr<Enemy> target,
                       float damage, float speed)
    : Entity(id), position_(position), target_(std::move(target)), damage_(damage), speed_(speed)
{
}

void Projectile::update(float /*dt*/)
{
    // TODO: move toward the target at speed_; on hit, call takeDamage(damage_)
    //       and die. Die too if the target expired.
}

void Projectile::render(sf::RenderTarget& /*target*/) const
{
    // TODO: draw the projectile at position_.
}

}  // namespace defender
