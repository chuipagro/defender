#include "defender/managers/EntityManager.hpp"

#include <algorithm>
#include <utility>

namespace defender
{

Tower& EntityManager::addTower(TowerType type, GridPos tile, sf::Vector2f position,
                               const TowerUpgrades& upgrades)
{
    towers_.push_back(std::make_unique<Tower>(nextId(), type, tile, position, upgrades));
    return *towers_.back();
}

Enemy& EntityManager::spawnEnemy(std::unique_ptr<Enemy> enemy)
{
    enemies_.push_back(std::move(enemy));
    return *enemies_.back();
}

Projectile& EntityManager::addProjectile(std::unique_ptr<Projectile> projectile)
{
    projectiles_.push_back(std::move(projectile));
    return *projectiles_.back();
}

Tower* EntityManager::towerAt(GridPos tile) const
{
    const auto it = std::ranges::find_if(towers_, [tile](const auto& tower)
                                         { return tower->isAlive() && tower->tile() == tile; });
    return it == towers_.end() ? nullptr : it->get();
}

void EntityManager::refreshTowers(const TowerUpgrades& upgrades)
{
    for (const auto& tower : towers_)
    {
        tower->refreshStats(upgrades);
    }
}

void EntityManager::updateAll(float dt)
{
    // Enemies move first, so towers aim at where they are now.
    for (const auto& enemy : enemies_)
    {
        enemy->update(dt);
    }
    for (const auto& tower : towers_)
    {
        tower->update(dt);
        if (!tower->isAlive() || !tower->isReady())
        {
            continue;
        }
        if (const auto target = tower->pickTarget(enemies_))
        {
            const Shot shot = tower->fire(*target);
            projectiles_.push_back(std::make_unique<Projectile>(
                nextId(), tower->muzzle(), target, shot, tower->stats().projectileSpeed));
        }
    }
    for (const auto& projectile : projectiles_)
    {
        projectile->update(dt);
        if (projectile->isAlive() && projectile->hasImpact())
        {
            resolveImpact(*projectile);
            projectile->consume();
        }
    }
}

void EntityManager::resolveImpact(const Projectile& projectile)
{
    const Shot& shot = projectile.shot();
    const auto& onHit = towerDefinition(shot.type).onHit;
    auto hit = [&](Enemy& enemy)
    {
        enemy.takeDamage(shot.damage);
        if (onHit && enemy.isAlive())
        {
            onHit(enemy, shot);
        }
    };

    if (shot.splash > 0.f)
    {
        const float radiusSquared = shot.splash * shot.splash;
        for (const auto& enemy : enemies_)
        {
            const sf::Vector2f d = enemy->getPosition() - projectile.getPosition();
            if (enemy->isAlive() && (d.x * d.x) + (d.y * d.y) <= radiusSquared)
            {
                hit(*enemy);
            }
        }
        return;
    }
    if (const auto target = projectile.target())
    {
        hit(*target);
    }
}

void EntityManager::renderAll(sf::RenderTarget& target) const
{
    // One depth-sorted list: something lower on screen is drawn in front.
    struct Item
    {
        float depth;
        const Entity* entity;
    };
    std::vector<Item> items;
    items.reserve(enemies_.size() + towers_.size());
    for (const auto& enemy : enemies_)
    {
        items.push_back({enemy->getPosition().y, enemy.get()});
    }
    for (const auto& tower : towers_)
    {
        items.push_back({tower->getPosition().y + 8.f, tower.get()});
    }
    std::ranges::stable_sort(items, {}, &Item::depth);
    for (const Item& item : items)
    {
        item.entity->render(target);
    }
    for (const auto& projectile : projectiles_)
    {
        projectile->render(target);
    }
}

void EntityManager::removeDeadEntities()
{
    auto dead = [](const auto& entity) { return !entity->isAlive(); };
    std::erase_if(towers_, dead);
    std::erase_if(enemies_, dead);
    std::erase_if(projectiles_, dead);
}

}  // namespace defender
