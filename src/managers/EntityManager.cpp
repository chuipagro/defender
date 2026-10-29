#include "defender/managers/EntityManager.hpp"

#include <algorithm>
#include <utility>

namespace defender
{

Tower& EntityManager::addTower(sf::Vector2f position, TowerType type)
{
    // TODO: stats per tower type, gold check.
    towers_.push_back(std::make_unique<Tower>(nextId(), type, position, TowerStats{}));
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

void EntityManager::updateAll(float dt)
{
    for (const auto& tower : towers_)
    {
        tower->update(dt);
        tower->shoot(enemies_);
    }
    for (const auto& enemy : enemies_)
    {
        enemy->update(dt);
    }
    for (const auto& projectile : projectiles_)
    {
        projectile->update(dt);
    }
}

void EntityManager::renderAll(sf::RenderTarget& target) const
{
    // Draw enemies from top to bottom so the lower ones overlap the upper ones.
    std::vector<const Enemy*> sorted;
    sorted.reserve(enemies_.size());
    for (const auto& enemy : enemies_)
    {
        sorted.push_back(enemy.get());
    }
    std::ranges::sort(sorted, {}, [](const Enemy* e) { return e->getPosition().y; });
    for (const Enemy* enemy : sorted)
    {
        enemy->render(target);
    }
    for (const auto& tower : towers_)
    {
        tower->render(target);
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
