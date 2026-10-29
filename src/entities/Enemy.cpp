#include "defender/entities/Enemy.hpp"

#include "defender/core/Concepts.hpp"
#include "defender/core/EventBus.hpp"
#include "defender/core/Events.hpp"
#include "defender/managers/ResourceManager.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include <algorithm>
#include <cmath>

namespace defender
{

static_assert(Updatable<Enemy>);
static_assert(Renderable<Enemy>);
static_assert(Damageable<Enemy>);

namespace
{

float length(sf::Vector2f v)
{
    return std::hypot(v.x, v.y);
}

/// Green for normal enemies, then orange, red and purple for bosses.
sf::Color healthBarColor(Rank rank)
{
    switch (rank)
    {
        case Rank::Normal:
            return {80, 220, 80};
        case Rank::MiniBoss:
            return {240, 170, 40};
        case Rank::Boss:
            return {230, 50, 50};
        case Rank::SuperBoss:
            return {190, 60, 230};
    }
    return {80, 220, 80};
}

}  // namespace

Enemy::Enemy(EntityId id, EnemySpec spec, std::span<const sf::Vector2f> path, int generation)
    : Entity(id),
      spec_(spec),
      generation_(generation),
      stats_(computeStats(spec)),
      path_(path),
      position_(path.empty() ? sf::Vector2f{} : path.front()),
      health_(stats_.health()),
      textureId_("sprites/enemies/" + std::string(elementName(spec.element)) + "/" +
                 std::string(enemyTypeName(spec.type)) + ".png")
{
    for (const EnemyHooks* hooks : layers())
    {
        if (hooks->onSpawn)
        {
            hooks->onSpawn(*this);
        }
    }
}

std::array<const EnemyHooks*, 3> Enemy::layers() const
{
    return {&speciesDefinition(spec_.type).hooks, &elementDefinition(spec_.element).hooks,
            &rankDefinition(spec_.rank).hooks};
}

void Enemy::update(float dt)
{
    if (!alive_)
    {
        return;
    }
    animationTime_ += dt;
    for (const EnemyHooks* hooks : layers())
    {
        if (hooks->onUpdate)
        {
            hooks->onUpdate(*this, dt);
        }
    }

    // Walk `speed * dt` pixels along the path, possibly past several points.
    float remaining = speed() * dt;
    while (remaining > 0.f && pathIndex_ + 1 < path_.size())
    {
        const sf::Vector2f delta = path_[pathIndex_ + 1] - position_;
        const float distance = length(delta);
        if (delta.x != 0.f)
        {
            facingLeft_ = delta.x < 0.f;
        }
        if (distance <= remaining)
        {
            position_ = path_[pathIndex_ + 1];
            ++pathIndex_;
            remaining -= distance;
        }
        else
        {
            position_ += delta * (remaining / distance);
            remaining = 0.f;
        }
    }

    if (pathIndex_ + 1 >= path_.size())
    {
        reachedEnd_ = true;
        alive_ = false;
        for (const EnemyHooks* hooks : layers())
        {
            if (hooks->onReachBase)
            {
                hooks->onReachBase(*this);
            }
        }
        EventBus::instance().emit(EnemyReachedBase{damage()});
    }
}

float Enemy::takeDamage(float amount)
{
    if (!alive_ || amount <= 0.f)
    {
        return 0.f;
    }
    for (const EnemyHooks* hooks : layers())
    {
        if (hooks->onHit)
        {
            amount = std::max(hooks->onHit(*this, amount), 0.f);
        }
    }
    health_ -= amount;
    if (health_ <= 0.f)
    {
        health_ = 0.f;
        alive_ = false;
        EventBus::instance().emit(EnemyKilled{this, reward()});
    }
    return amount;
}

void Enemy::heal(float amount)
{
    if (alive_)
    {
        health_ = std::min(health_ + amount, maxHealth());
    }
}

void Enemy::runDeathHooks(EnemySpawner& spawner)
{
    for (const EnemyHooks* hooks : layers())
    {
        if (hooks->onDeath)
        {
            hooks->onDeath(*this, spawner);
        }
    }
}

void Enemy::placeOnPath(std::size_t index, sf::Vector2f position)
{
    if (path_.empty())
    {
        return;
    }
    pathIndex_ = std::min(index, path_.size() - 1);
    position_ = position;
}

void Enemy::moveBack(float distance)
{
    while (distance > 0.f && !path_.empty())
    {
        const sf::Vector2f target = path_[pathIndex_];
        const sf::Vector2f delta = target - position_;
        const float toPoint = length(delta);
        if (toPoint >= distance)
        {
            position_ += delta * (distance / toPoint);
            return;
        }
        position_ = target;
        distance -= toPoint;
        if (pathIndex_ == 0)
        {
            return;
        }
        --pathIndex_;
    }
}

bool Enemy::contains(sf::Vector2f point) const
{
    // The body is roughly a circle above the feet.
    const float radius = 12.f * scale();
    const sf::Vector2f center = position_ - sf::Vector2f(0.f, 8.f * scale());
    return length(point - center) <= radius;
}

void Enemy::render(sf::RenderTarget& target) const
{
    const auto frame = static_cast<int>(animationTime_ / FRAME_TIME) % FRAME_COUNT;
    sf::Sprite sprite(ResourceManager::instance().getTexture(textureId_),
                      sf::IntRect(frame * FRAME_SIZE, 0, FRAME_SIZE, FRAME_SIZE));
    // Feet (row ~29 of the frame) slightly below the path center line.
    sprite.setOrigin(FRAME_SIZE / 2.f, 24.f);
    sprite.setScale(facingLeft_ ? -scale() : scale(), scale());
    // Whole pixels keep the pixel art crisp.
    sprite.setPosition(std::round(position_.x), std::round(position_.y));
    target.draw(sprite);

    if (health_ < maxHealth() || spec_.rank != Rank::Normal)
    {
        renderHealthBar(target);
    }
}

void Enemy::renderHealthBar(sf::RenderTarget& target) const
{
    const float width = std::round(20.f * scale());
    const sf::Vector2f topLeft(std::round(position_.x - (width / 2.f)),
                               std::round(position_.y - (26.f * scale())));

    sf::RectangleShape back({width + 2.f, 4.f});
    back.setPosition(topLeft - sf::Vector2f(1.f, 1.f));
    back.setFillColor(sf::Color(20, 20, 20, 200));
    target.draw(back);

    const float ratio = std::clamp(health_ / maxHealth(), 0.f, 1.f);
    sf::RectangleShape bar({std::round(width * ratio), 2.f});
    bar.setPosition(topLeft);
    bar.setFillColor(healthBarColor(spec_.rank));
    target.draw(bar);
}

}  // namespace defender
