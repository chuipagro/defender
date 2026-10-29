#include "defender/entities/Tower.hpp"

#include "defender/core/Concepts.hpp"
#include "defender/managers/ResourceManager.hpp"
#include "defender/map/MapRenderer.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include <algorithm>
#include <cmath>

namespace defender
{

static_assert(Updatable<Tower>);
static_assert(Renderable<Tower>);

namespace
{

std::string towerTexture(TowerType type)
{
    return "sprites/towers/" + std::string(elementName(type)) + ".png";
}

/// Sprite with its feet (bottom row) at the bottom of the tile centered at `position`.
sf::Sprite towerSprite(TowerType type, int frame, sf::Vector2f position, bool flip)
{
    sf::Sprite sprite(ResourceManager::instance().getTexture(towerTexture(type)),
                      sf::IntRect(frame * Tower::FRAME_W, 0, Tower::FRAME_W, Tower::FRAME_H));
    sprite.setOrigin(Tower::FRAME_W / 2.f, Tower::FRAME_H);
    sprite.setScale(flip ? -1.f : 1.f, 1.f);
    sprite.setPosition(std::round(position.x), std::round(position.y + (TILE_SIZE / 2.f)));
    return sprite;
}

}  // namespace

Tower::Tower(EntityId id, TowerType type, GridPos tile, sf::Vector2f position,
             const TowerUpgrades& upgrades)
    : Entity(id),
      type_(type),
      tile_(tile),
      position_(position),
      stats_(towerStats(type, 1, upgrades)),
      textureId_(towerTexture(type))
{
}

void Tower::update(float dt)
{
    cooldown_ = std::max(cooldown_ - dt, 0.f);
    sinceShot_ += dt;
    animationTime_ += dt;
}

std::shared_ptr<Enemy> Tower::pickTarget(std::span<const std::shared_ptr<Enemy>> enemies) const
{
    std::shared_ptr<Enemy> best;
    const float rangeSquared = stats_.range * stats_.range;
    for (const auto& enemy : enemies)
    {
        if (!enemy->isAlive())
        {
            continue;
        }
        const sf::Vector2f d = enemy->getPosition() - position_;
        if ((d.x * d.x) + (d.y * d.y) <= rangeSquared &&
            (!best || enemy->progress() > best->progress()))
        {
            best = enemy;
        }
    }
    return best;
}

Shot Tower::fire(const Enemy& target)
{
    cooldown_ = stats_.fireRate > 0.f ? 1.f / stats_.fireRate : 1.f;
    sinceShot_ = 0.f;
    if (target.getPosition().x != position_.x)
    {
        facingLeft_ = target.getPosition().x < position_.x;
    }
    return {.type = type_, .damage = stats_.damage, .splash = stats_.splash, .strength = stats_.strength};
}

bool Tower::upgrade(const TowerUpgrades& upgrades)
{
    if (level_ >= TOWER_MAX_LEVEL)
    {
        return false;
    }
    ++level_;
    refreshStats(upgrades);
    return true;
}

void Tower::refreshStats(const TowerUpgrades& upgrades)
{
    stats_ = towerStats(type_, level_, upgrades);
}

void Tower::render(sf::RenderTarget& target) const
{
    int frame = 0;
    if (type_ == TowerType::Normal)
    {
        // Crossbow: frame 3 right after a shot, then the string is pulled back.
        const float reload = stats_.fireRate > 0.f ? 1.f / stats_.fireRate : 1.f;
        frame = sinceShot_ < 0.12f ? 3 : std::min(2, static_cast<int>(sinceShot_ / reload * 3.f));
    }
    else
    {
        frame = static_cast<int>(animationTime_ / FRAME_TIME) % FRAME_COUNT;
    }
    target.draw(towerSprite(type_, frame, position_, facingLeft_));

    // Level pips on the foot of the tower
    for (int i = 0; i < level_; ++i)
    {
        sf::RectangleShape pip({4.f, 4.f});
        pip.setPosition(std::round(position_.x) - 9.f + (static_cast<float>(i) * 7.f),
                        std::round(position_.y) + 11.f);
        pip.setFillColor(sf::Color(245, 197, 66));
        pip.setOutlineColor(sf::Color(40, 24, 10));
        pip.setOutlineThickness(1.f);
        target.draw(pip);
    }
}

void Tower::drawGhost(sf::RenderTarget& target, TowerType type, sf::Vector2f position,
                      sf::Color tint)
{
    sf::Sprite sprite = towerSprite(type, 0, position, false);
    sprite.setColor(tint);
    target.draw(sprite);
}

}  // namespace defender
