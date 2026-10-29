#include "defender/core/EventBus.hpp"
#include "defender/core/Events.hpp"
#include "defender/entities/EnemyFactory.hpp"
#include "defender/managers/EntityManager.hpp"
#include "defender/managers/WaveManager.hpp"

#include <gtest/gtest.h>

#include <vector>

using defender::Biome;
using defender::Element;
using defender::EnemyFactory;
using defender::EnemyType;

namespace
{

/// An L-shaped path: 3 tiles right, then 2 tiles down (32 px tiles).
const std::vector<sf::Vector2f> L_PATH{{16.f, 16.f}, {112.f, 16.f}, {112.f, 80.f}};

}  // namespace

// ── Enemy movement ───────────────────────────────────────────────────────────

TEST(EnemyMovement, WalksThePathToTheEnd)
{
    auto enemy = EnemyFactory::create(1, {EnemyType::Cat, Element::Fire}, L_PATH);
    const float speed = enemy->speed();
    const float length = 96.f + 64.f;

    // Halfway through the first segment.
    enemy->update(48.f / speed);
    EXPECT_NEAR(enemy->getPosition().x, 64.f, 0.01f);
    EXPECT_NEAR(enemy->getPosition().y, 16.f, 0.01f);
    EXPECT_TRUE(enemy->isAlive());

    // Past the corner: 16 px down the second segment.
    enemy->update(64.f / speed);
    EXPECT_NEAR(enemy->getPosition().x, 112.f, 0.01f);
    EXPECT_NEAR(enemy->getPosition().y, 32.f, 0.01f);

    // To the end: it stops on the last point and leaves the game.
    enemy->update(length / speed);
    EXPECT_EQ(enemy->getPosition(), L_PATH.back());
    EXPECT_TRUE(enemy->hasReachedEnd());
    EXPECT_FALSE(enemy->isAlive());
}

TEST(EnemyMovement, BigTimeStepCrossesSeveralPoints)
{
    auto enemy = EnemyFactory::create(1, {EnemyType::Dog, Element::Water}, L_PATH);
    enemy->update(1000.f);
    EXPECT_EQ(enemy->getPosition(), L_PATH.back());
    EXPECT_TRUE(enemy->hasReachedEnd());
}

TEST(EnemyMovement, ReachingTheEndEmitsAnEvent)
{
    int reached = 0;
    auto& bus = defender::EventBus::instance();
    const auto id = bus.subscribe<defender::EnemyReachedBase>(
        [&reached](const defender::EnemyReachedBase& /*event*/) { ++reached; });
    auto enemy = EnemyFactory::create(1, {EnemyType::Duck, Element::Wind}, L_PATH);
    enemy->update(1000.f);
    enemy->update(1.f);  // already gone: no second event
    bus.unsubscribe(id);
    EXPECT_EQ(reached, 1);
}

TEST(EnemyMovement, KilledEnemyDidNotReachTheEnd)
{
    auto enemy = EnemyFactory::create(1, {EnemyType::Frog, Element::Earth}, L_PATH);
    enemy->takeDamage(1000.f);
    enemy->update(1000.f);
    EXPECT_FALSE(enemy->isAlive());
    EXPECT_FALSE(enemy->hasReachedEnd());
}

// ── Waves ────────────────────────────────────────────────────────────────────

namespace
{

defender::WaveData wavePlan(int count, float interval)
{
    defender::WaveData data;
    data.spawnInterval = interval;
    for (int i = 0; i < count; ++i)
    {
        data.enemies.push_back({{EnemyType::Cat, Element::Normal}, 1, 0.f});
    }
    return data;
}

}  // namespace

TEST(Waves, WaveSpawnsEveryoneThenCompletes)
{
    defender::EntityManager entities;
    EnemyFactory factory(Biome::Ice, 5);
    defender::WaveManager waves(entities, factory, L_PATH);
    waves.startWave(1, wavePlan(12, 0.5f));
    EXPECT_EQ(waves.getCurrentWave(), 1);
    EXPECT_EQ(waves.waveSize(), 12);
    EXPECT_EQ(waves.remainingEnemies(), 12);
    EXPECT_FALSE(waves.isWaveComplete());

    // Simulate the game loop at 60 fps until the wave is over.
    const float dt = 1.f / 60.f;
    int reached = 0;
    int maxOnMap = 0;
    for (int frame = 0; frame < 60 * 120 && !waves.isWaveComplete(); ++frame)
    {
        waves.update(dt);
        entities.updateAll(dt);
        for (const auto& enemy : entities.enemies())
        {
            reached += enemy->hasReachedEnd() ? 1 : 0;
        }
        maxOnMap = std::max(maxOnMap, static_cast<int>(entities.enemies().size()));
        entities.removeDeadEntities();
    }

    EXPECT_TRUE(waves.isWaveComplete());
    EXPECT_EQ(waves.spawnedCount(), 12);
    EXPECT_EQ(waves.remainingEnemies(), 0);
    EXPECT_EQ(reached, 12) << "every enemy must walk to the end";
    EXPECT_LT(maxOnMap, 12) << "enemies spawn one after the other, not all at once";
}

TEST(Waves, SpawnsOneEnemyPerInterval)
{
    const std::vector<sf::Vector2f> longPath{{0.f, 0.f}, {100000.f, 0.f}};
    defender::EntityManager entities;
    EnemyFactory factory(Biome::Desert, 1);
    defender::WaveManager waves(entities, factory, longPath);
    waves.startWave(1, wavePlan(10, 0.7f));
    waves.update(0.f);
    EXPECT_EQ(waves.spawnedCount(), 1);
    waves.update((0.7f * 3.f) + 0.001f);  // float margin
    EXPECT_EQ(waves.spawnedCount(), 4);
}
