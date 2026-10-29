#include "defender/entities/Tower.hpp"

#include "defender/core/Concepts.hpp"

namespace defender
{

static_assert(Updatable<Tower>);
static_assert(Renderable<Tower>);

Tower::Tower(EntityId id, TowerType type, sf::Vector2f position, const TowerStats& stats)
    : Entity(id),
      type_(type),
      position_(position),
      range_(stats.range),
      fireRate_(stats.fireRate),
      stats_(stats)
{
}

void Tower::update(float /*dt*/)
{
    // TODO: decrease cooldown_.
}

void Tower::render(sf::RenderTarget& /*target*/) const
{
    // TODO: draw the animated tower sprite of type_ at position_.
}

void Tower::shoot(std::span<const std::shared_ptr<Enemy>> /*enemies*/)
{
    // TODO: pick the first enemy within range_, spawn a Projectile,
    //       reset cooldown_ to 1 / fireRate_.
}

}  // namespace defender
