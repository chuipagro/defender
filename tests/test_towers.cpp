#include "defender/core/Player.hpp"
#include "defender/entities/EnemyDefinitions.hpp"
#include "defender/entities/EnemyFactory.hpp"
#include "defender/entities/TowerDefinitions.hpp"
#include "defender/managers/EntityManager.hpp"

#include <gtest/gtest.h>

#include <vector>

using defender::Element;
using defender::EnemyFactory;
using defender::EnemyType;
using defender::Rank;
using defender::TowerType;

namespace
{

/// A straight horizontal path along y = 16, 40 tiles long.
const std::vector<sf::Vector2f> ROAD{{0.f, 16.f}, {1280.f, 16.f}};

/// Tower on the tile just below the road, at x tile `column`.
defender::Tower& towerNextToRoad(defender::EntityManager& entities, TowerType type, int column)
{
    const sf::Vector2f center((static_cast<float>(column) + 0.5f) * 32.f, 48.f);
    return entities.addTower(type, {column, 1}, center);
}

defender::Enemy& enemyAt(defender::EntityManager& entities, defender::EnemySpec spec, float x)
{
    auto& enemy = entities.spawnEnemy(EnemyFactory::create(entities.nextId(), spec, ROAD));
    enemy.placeOnPath(0, {x, 16.f});
    return enemy;
}

/// Runs the entity manager for `seconds` with a fixed step.
void run(defender::EntityManager& entities, float seconds, float dt = 1.f / 60.f)
{
    for (float t = 0.f; t < seconds; t += dt)
    {
        entities.updateAll(dt);
        entities.removeDeadEntities();
    }
}

}  // namespace

// ── Definitions ───────────────────────────────────────────────────────────────

TEST(TowerDefinitions, LevelsMakeTowersStronger)
{
    for (const TowerType type : {TowerType::Normal, TowerType::Water, TowerType::Fire,
                                 TowerType::Earth, TowerType::Wind})
    {
        for (int level = 2; level <= defender::TOWER_MAX_LEVEL; ++level)
        {
            const auto before = defender::towerStats(type, level - 1);
            const auto after = defender::towerStats(type, level);
            EXPECT_GT(after.damage, before.damage);
            EXPECT_GT(after.range, before.range);
            EXPECT_GT(after.fireRate, before.fireRate);
        }
    }
}

TEST(TowerDefinitions, SkillBonusesApply)
{
    defender::TowerUpgrades upgrades;
    upgrades.of(TowerType::Fire).damage = 1.25f;
    EXPECT_FLOAT_EQ(defender::towerStats(TowerType::Fire, 1, upgrades).damage,
                    defender::towerDefinition(TowerType::Fire).damage * 1.25f);
}

TEST(TowerDefinitions, PlacingCostsGoldOnly)
{
    for (const TowerType type : {TowerType::Normal, TowerType::Water, TowerType::Fire,
                                 TowerType::Earth, TowerType::Wind})
    {
        const auto cost = defender::placeCost(type);
        EXPECT_GT(cost.gold, 0);
        EXPECT_EQ(cost.materials, 0);
    }
}

TEST(TowerDefinitions, ElementalUpgradesNeedTheirMaterial)
{
    for (int level = 1; level < defender::TOWER_MAX_LEVEL; ++level)
    {
        const auto normal = defender::upgradeCost(TowerType::Normal, level);
        EXPECT_GT(normal.gold, 0);
        EXPECT_EQ(normal.materials, 0) << "normal towers upgrade with gold only";
        for (const TowerType type : {TowerType::Water, TowerType::Fire, TowerType::Earth,
                                     TowerType::Wind})
        {
            const auto cost = defender::upgradeCost(type, level);
            EXPECT_GT(cost.gold, 0);
            EXPECT_EQ(cost.material, type);
            EXPECT_GT(cost.materials, 0);
        }
    }
    EXPECT_EQ(defender::upgradeCost(TowerType::Fire, defender::TOWER_MAX_LEVEL).gold, 0);
}

TEST(TowerDefinitions, SellingGivesBackPartOfTheGold)
{
    const int placed = defender::placeCost(TowerType::Normal).gold;
    EXPECT_EQ(defender::sellValue(TowerType::Normal, 1), static_cast<int>(placed * 0.6f + 0.5f));
    EXPECT_GT(defender::sellValue(TowerType::Normal, 3), defender::sellValue(TowerType::Normal, 1));
}

// ── Materials ─────────────────────────────────────────────────────────────────

TEST(Materials, OnlyElementalEnemiesDropThem)
{
    EXPECT_EQ(defender::materialDrop({EnemyType::Cat, Element::Normal, Rank::SuperBoss}, 0.f), 0);
    EXPECT_EQ(defender::materialDrop({EnemyType::Cat, Element::Fire}, 0.1f), 1);
    EXPECT_EQ(defender::materialDrop({EnemyType::Cat, Element::Fire}, 0.9f), 0);
    EXPECT_GT(defender::materialDrop({EnemyType::Cat, Element::Water, Rank::Boss}, 0.9f),
              defender::materialDrop({EnemyType::Cat, Element::Water, Rank::MiniBoss}, 0.9f));
}

TEST(Materials, UpgradeCostsAreChecked)
{
    defender::Player player;
    player.earn(1000);
    const auto cost = defender::upgradeCost(TowerType::Water, 1);
    EXPECT_FALSE(player.canAfford(cost));
    EXPECT_FALSE(player.pay(cost));
    player.addMaterials(Element::Water, cost.materials);
    EXPECT_TRUE(player.pay(cost));
    EXPECT_EQ(player.materials(Element::Water), 0);
    EXPECT_EQ(player.gold(), defender::Player::STARTING_GOLD + 1000 - cost.gold);
}

// ── Targeting and shooting ────────────────────────────────────────────────────

TEST(TowerShooting, TargetsTheMostAdvancedEnemyInRange)
{
    defender::EntityManager entities;
    auto& tower = towerNextToRoad(entities, TowerType::Normal, 10);
    enemyAt(entities, {EnemyType::Duck, Element::Normal}, 300.f);       // in range
    auto& ahead = enemyAt(entities, {EnemyType::Duck, Element::Normal}, 380.f);  // in range, ahead
    enemyAt(entities, {EnemyType::Duck, Element::Normal}, 900.f);       // out of range
    EXPECT_EQ(tower.pickTarget(entities.enemies()).get(), &ahead);
}

TEST(TowerShooting, ShootsOncePerCooldown)
{
    defender::EntityManager entities;
    auto& tower = towerNextToRoad(entities, TowerType::Normal, 10);
    auto& enemy = enemyAt(entities, {EnemyType::Orangutan, Element::Normal}, 336.f);
    enemy.stats()[defender::Stat::Speed] = 0.f;  // stands still
    entities.updateAll(0.01f);
    EXPECT_EQ(entities.projectiles().size(), 1u);
    entities.updateAll(0.01f);
    EXPECT_EQ(entities.projectiles().size(), 1u) << "still reloading";
    EXPECT_FALSE(tower.isReady());
}

TEST(TowerShooting, ArrowsKillAPassingEnemy)
{
    defender::EntityManager entities;
    towerNextToRoad(entities, TowerType::Normal, 6);
    towerNextToRoad(entities, TowerType::Normal, 8);
    auto& duck = enemyAt(entities, {EnemyType::Duck, Element::Normal}, 0.f);
    const auto id = duck.id();
    run(entities, 12.f);
    for (const auto& enemy : entities.enemies())
    {
        EXPECT_NE(enemy->id(), id) << "the duck should be dead";
    }
}

TEST(TowerShooting, WaterSlowsFireBurnsWindPushes)
{
    defender::EntityManager entities;
    towerNextToRoad(entities, TowerType::Water, 10);
    auto& slowed = enemyAt(entities, {EnemyType::Orangutan, Element::Normal}, 336.f);
    run(entities, 1.f);
    EXPECT_TRUE(slowed.isSlowed());
    EXPECT_LT(slowed.currentSpeed(), slowed.speed());

    defender::EntityManager fire;
    towerNextToRoad(fire, TowerType::Fire, 10);
    auto& burning = enemyAt(fire, {EnemyType::Orangutan, Element::Normal}, 336.f);
    burning.stats()[defender::Stat::Speed] = 0.f;
    run(fire, 0.5f);
    EXPECT_TRUE(burning.isBurning());

    defender::EntityManager wind;
    towerNextToRoad(wind, TowerType::Wind, 10);
    auto& pushed = enemyAt(wind, {EnemyType::Orangutan, Element::Normal}, 336.f);
    pushed.stats()[defender::Stat::Speed] = 0.f;
    const float before = pushed.progress();
    run(wind, 0.5f);
    EXPECT_LT(pushed.progress(), before);
}

TEST(TowerShooting, EarthHitsEveryoneInTheSplash)
{
    defender::EntityManager entities;
    towerNextToRoad(entities, TowerType::Earth, 10);
    auto& a = enemyAt(entities, {EnemyType::Orangutan, Element::Normal}, 336.f);
    auto& b = enemyAt(entities, {EnemyType::Orangutan, Element::Normal}, 346.f);
    a.stats()[defender::Stat::Speed] = 0.f;
    b.stats()[defender::Stat::Speed] = 0.f;
    run(entities, 0.6f);
    EXPECT_LT(a.health(), a.maxHealth());
    EXPECT_LT(b.health(), b.maxHealth());
}

TEST(TowerShooting, BurnIgnoresArmor)
{
    auto enemy = EnemyFactory::create(1, {EnemyType::Orangutan, Element::Earth}, ROAD);
    enemy->stats()[defender::Stat::Speed] = 0.f;
    enemy->applyBurn(10.f, 1.f);
    const float before = enemy->health();
    enemy->update(1.f);
    EXPECT_NEAR(enemy->health(), before - 10.f, 0.01f);
}

// ── Placement ─────────────────────────────────────────────────────────────────

TEST(TowerPlacement, OneTowerPerTileAndSelling)
{
    defender::EntityManager entities;
    auto& tower = towerNextToRoad(entities, TowerType::Normal, 4);
    EXPECT_EQ(entities.towerAt({4, 1}), &tower);
    EXPECT_EQ(entities.towerAt({5, 1}), nullptr);
    tower.remove();
    EXPECT_EQ(entities.towerAt({4, 1}), nullptr);
    entities.removeDeadEntities();
    EXPECT_TRUE(entities.towers().empty());
}

TEST(TowerPlacement, UpgradeStopsAtMaxLevel)
{
    defender::EntityManager entities;
    auto& tower = towerNextToRoad(entities, TowerType::Fire, 4);
    EXPECT_TRUE(tower.upgrade({}));
    EXPECT_TRUE(tower.upgrade({}));
    EXPECT_EQ(tower.level(), defender::TOWER_MAX_LEVEL);
    EXPECT_FALSE(tower.upgrade({}));
}
