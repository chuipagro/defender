#pragma once

#include "defender/entities/Enemy.hpp"
#include "defender/entities/Projectile.hpp"
#include "defender/entities/Tower.hpp"
#include "defender/map/Tile.hpp"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>

#include <memory>
#include <span>
#include <vector>

namespace defender
{

class EntityManager
{
public:
    Tower& addTower(TowerType type, GridPos tile, sf::Vector2f position,
                    const TowerUpgrades& upgrades = {});
    Enemy& spawnEnemy(std::unique_ptr<Enemy> enemy);
    Projectile& addProjectile(std::unique_ptr<Projectile> projectile);

    void updateAll(float dt);
    void renderAll(sf::RenderTarget& target) const;
    void removeDeadEntities();

    [[nodiscard]] Tower* towerAt(GridPos tile) const;
    void refreshTowers(const TowerUpgrades& upgrades);

    [[nodiscard]] EntityId nextId() { return nextId_++; }

    [[nodiscard]] std::span<const std::shared_ptr<Enemy>> enemies() const { return enemies_; }
    [[nodiscard]] std::span<const std::unique_ptr<Tower>> towers() const { return towers_; }
    [[nodiscard]] std::span<const std::unique_ptr<Projectile>> projectiles() const
    {
        return projectiles_;
    }

private:
    void resolveImpact(const Projectile& projectile);

    std::vector<std::unique_ptr<Tower>> towers_;
    std::vector<std::shared_ptr<Enemy>> enemies_;
    std::vector<std::unique_ptr<Projectile>> projectiles_;
    EntityId nextId_ = 1;
};

}  // namespace defender
