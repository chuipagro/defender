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

/// Owns every entity of a game and runs them: towers shoot, projectiles fly
/// and hit, enemies walk.
///
/// Enemies are shared_ptr so projectiles can track them with weak_ptr.
class EntityManager
{
public:
    /// Places a tower on `tile` (centered at `position`). The caller checks
    /// that the tile is free and buildable, and makes the player pay.
    Tower& addTower(TowerType type, GridPos tile, sf::Vector2f position,
                    const TowerUpgrades& upgrades = {});
    /// Takes an enemy built by EnemyFactory (use nextId() for its id).
    Enemy& spawnEnemy(std::unique_ptr<Enemy> enemy);
    Projectile& addProjectile(std::unique_ptr<Projectile> projectile);

    void updateAll(float dt);
    /// Towers and enemies sorted by depth (lower on screen = in front), then projectiles.
    void renderAll(sf::RenderTarget& target) const;
    void removeDeadEntities();

    /// The tower standing on `tile`, or null.
    [[nodiscard]] Tower* towerAt(GridPos tile) const;
    /// Recomputes every tower's stats (after a skill tree purchase).
    void refreshTowers(const TowerUpgrades& upgrades);

    /// Unique id for the next entity.
    [[nodiscard]] EntityId nextId() { return nextId_++; }

    [[nodiscard]] std::span<const std::shared_ptr<Enemy>> enemies() const { return enemies_; }
    [[nodiscard]] std::span<const std::unique_ptr<Tower>> towers() const { return towers_; }
    [[nodiscard]] std::span<const std::unique_ptr<Projectile>> projectiles() const
    {
        return projectiles_;
    }

private:
    /// Applies a projectile that reached its target: damage (to the target or
    /// to everyone in the splash radius) and the tower's effect.
    void resolveImpact(const Projectile& projectile);

    std::vector<std::unique_ptr<Tower>> towers_;
    std::vector<std::shared_ptr<Enemy>> enemies_;
    std::vector<std::unique_ptr<Projectile>> projectiles_;
    EntityId nextId_ = 1;
};

}  // namespace defender
