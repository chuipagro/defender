#pragma once

#include "defender/entities/Enemy.hpp"
#include "defender/entities/Projectile.hpp"
#include "defender/entities/Tower.hpp"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>

#include <memory>
#include <span>
#include <vector>

namespace defender
{

/// Owns every entity of a game and runs them.
///
/// Enemies are shared_ptr so projectiles can track them with weak_ptr.
class EntityManager
{
public:
    Tower& addTower(sf::Vector2f position, TowerType type);
    /// Takes an enemy built by EnemyFactory (use nextId() for its id).
    Enemy& spawnEnemy(std::unique_ptr<Enemy> enemy);
    Projectile& addProjectile(std::unique_ptr<Projectile> projectile);

    void updateAll(float dt);
    void renderAll(sf::RenderTarget& target) const;
    void removeDeadEntities();

    /// Unique id for the next entity.
    [[nodiscard]] EntityId nextId() { return nextId_++; }

    [[nodiscard]] std::span<const std::shared_ptr<Enemy>> enemies() const { return enemies_; }

private:
    std::vector<std::unique_ptr<Tower>> towers_;
    std::vector<std::shared_ptr<Enemy>> enemies_;
    std::vector<std::unique_ptr<Projectile>> projectiles_;
    EntityId nextId_ = 1;
};

}  // namespace defender
