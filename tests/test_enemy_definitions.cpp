#include "defender/core/EventBus.hpp"
#include "defender/core/Events.hpp"
#include "defender/entities/EnemyDefinitions.hpp"
#include "defender/entities/EnemyFactory.hpp"
#include "defender/managers/EntityManager.hpp"
#include "defender/managers/WaveManager.hpp"

#include <gtest/gtest.h>

#include <vector>

using defender::Biome;
using defender::computeStats;
using defender::Element;
using defender::EnemyFactory;
using defender::EnemyType;
using defender::Rank;
using defender::Stat;

namespace
{

const std::vector<sf::Vector2f> LONG_PATH{{0.f, 0.f}, {1000.f, 0.f}, {1000.f, 1000.f}};

}  // namespace

// ── Stats composition ─────────────────────────────────────────────────────────

TEST(EnemyStats, NormalHasTheSpeciesStats)
{
    const auto base = defender::speciesDefinition(EnemyType::Dog).base;
    const auto dog = computeStats({EnemyType::Dog, Element::Normal});
    EXPECT_EQ(dog.values, base.values);
}

TEST(EnemyStats, WaterIsOneAndAHalfTimesStrongerPlusFiveGold)
{
    const auto base = defender::speciesDefinition(EnemyType::Dog).base;
    const auto dog = computeStats({EnemyType::Dog, Element::Water});
    EXPECT_FLOAT_EQ(dog.health(), base.health() * 1.5f);
    EXPECT_FLOAT_EQ(dog.damage(), base.damage() * 1.5f);
    EXPECT_FLOAT_EQ(dog.get(Stat::Reward), (base.get(Stat::Reward) * 1.5f) + 5.f);
    EXPECT_FLOAT_EQ(dog.speed(), base.speed());
}

TEST(EnemyStats, EveryElementIsStrongerThanNormal)
{
    for (const EnemyType type : {EnemyType::Octopus, EnemyType::Orangutan, EnemyType::Cat,
                                 EnemyType::Dog, EnemyType::Duck, EnemyType::Frog})
    {
        const auto normal = computeStats({type, Element::Normal});
        for (const Element element : {Element::Water, Element::Fire, Element::Earth, Element::Wind})
        {
            const auto stats = computeStats({type, element});
            EXPECT_GT(stats.health(), normal.health()) << defender::elementName(element);
            EXPECT_GT(stats.reward(), normal.reward()) << defender::elementName(element);
        }
    }
}

TEST(EnemyStats, SpeciesAndElementCombine)
{
    // Wind = x1.3 global, x1.35 speed on top of the cat's stats.
    const auto base = defender::speciesDefinition(EnemyType::Cat).base;
    const auto cat = computeStats({EnemyType::Cat, Element::Wind});
    EXPECT_FLOAT_EQ(cat.speed(), base.speed() * 1.35f);
    EXPECT_FLOAT_EQ(cat.health(), base.health() * 1.3f);
}

TEST(EnemyStats, HigherRanksAreBiggerAndTougher)
{
    float lastScale = 0.f;
    float lastHealth = 0.f;
    int lastReward = 0;
    for (const Rank rank : {Rank::Normal, Rank::MiniBoss, Rank::Boss, Rank::SuperBoss})
    {
        const auto stats = computeStats({EnemyType::Duck, Element::Earth, rank});
        EXPECT_GT(stats.scale(), lastScale) << defender::rankName(rank);
        EXPECT_GT(stats.health(), lastHealth) << defender::rankName(rank);
        EXPECT_GT(stats.reward(), lastReward) << defender::rankName(rank);
        lastScale = stats.scale();
        lastHealth = stats.health();
        lastReward = stats.reward();
    }
    EXPECT_FLOAT_EQ(computeStats({EnemyType::Duck, Element::Normal}).scale(), 1.f);
}

TEST(EnemyStats, HelpersFillNeutralValues)
{
    const auto mult = defender::times({{Stat::Health, 4.f}});
    const auto add = defender::plus({{Stat::Reward, 5.f}});
    EXPECT_FLOAT_EQ(mult.get(Stat::Health), 4.f);
    EXPECT_FLOAT_EQ(mult.get(Stat::Speed), 1.f);
    EXPECT_FLOAT_EQ(add.get(Stat::Reward), 5.f);
    EXPECT_FLOAT_EQ(add.get(Stat::Health), 0.f);
}

// ── Hooks ─────────────────────────────────────────────────────────────────────

TEST(EnemyHooks, EarthArmorReducesHits)
{
    auto earth = EnemyFactory::create(1, {EnemyType::Duck, Element::Earth}, LONG_PATH);
    auto normal = EnemyFactory::create(2, {EnemyType::Duck, Element::Normal}, LONG_PATH);
    EXPECT_FLOAT_EQ(earth->takeDamage(10.f), 7.f);  // armor 3
    EXPECT_FLOAT_EQ(normal->takeDamage(10.f), 10.f);
    // Armor never blocks more than 80%.
    EXPECT_FLOAT_EQ(earth->takeDamage(1.f), 0.2f);
}

TEST(EnemyHooks, ArmorLayersStack)
{
    // Orangutan (2) + earth (3) + boss (3) = 8.
    auto tank = EnemyFactory::create(1, {EnemyType::Orangutan, Element::Earth, Rank::Boss},
                                     LONG_PATH);
    EXPECT_FLOAT_EQ(tank->takeDamage(20.f), 12.f);
}

TEST(EnemyHooks, RegenerationHealsUpToMaxHealth)
{
    auto enemy = EnemyFactory::create(1, {EnemyType::Frog, Element::Normal}, LONG_PATH);
    const auto regen = defender::regeneration(2.f);
    enemy->takeDamage(10.f);
    const float hurt = enemy->health();
    regen(*enemy, 1.f);
    EXPECT_FLOAT_EQ(enemy->health(), hurt + 2.f);
    regen(*enemy, 100.f);
    EXPECT_FLOAT_EQ(enemy->health(), enemy->maxHealth());
}

TEST(EnemyHooks, FireRisesFromItsAshesOnce)
{
    auto enemy = EnemyFactory::create(1, {EnemyType::Duck, Element::Fire}, LONG_PATH);
    enemy->takeDamage(1e6f);
    EXPECT_TRUE(enemy->isAlive());
    EXPECT_FLOAT_EQ(enemy->health(), enemy->maxHealth() * 0.4f);
    enemy->takeDamage(1e6f);
    EXPECT_FALSE(enemy->isAlive());
}

TEST(EnemyHooks, WindDodgesOneHitOutOfFive)
{
    auto enemy = EnemyFactory::create(1, {EnemyType::Orangutan, Element::Wind}, LONG_PATH);
    int dodged = 0;
    for (int i = 0; i < 10; ++i)
    {
        dodged += enemy->takeDamage(1.f) == 0.f ? 1 : 0;
    }
    EXPECT_EQ(dodged, 2);
}

TEST(EnemyHooks, WaterLeavesANormalEnemyBehind)
{
    defender::EntityManager entities;
    EnemyFactory factory(Biome::Ice, 3);
    defender::WaveManager waves(entities, factory, LONG_PATH);
    auto& frog = entities.spawnEnemy(
        EnemyFactory::create(entities.nextId(), {EnemyType::Frog, Element::Water}, LONG_PATH));
    frog.update(1.f);
    const sf::Vector2f where = frog.getPosition();
    frog.takeDamage(1e6f);
    entities.removeDeadEntities();
    waves.update(0.f);
    ASSERT_EQ(entities.enemies().size(), 1u);
    const auto& child = entities.enemies().front();
    EXPECT_EQ(child->spec(), (defender::EnemySpec{EnemyType::Frog, Element::Normal, Rank::Normal}));
    EXPECT_EQ(child->getPosition(), where);
}

TEST(EnemyHooks, CatSurvivesItsFirstDeadlyHit)
{
    auto cat = EnemyFactory::create(1, {EnemyType::Cat, Element::Normal}, LONG_PATH);
    cat->takeDamage(1000.f);
    EXPECT_TRUE(cat->isAlive());
    EXPECT_FLOAT_EQ(cat->health(), 1.f);
    cat->takeDamage(1000.f);
    EXPECT_FALSE(cat->isAlive());
}

TEST(EnemyHooks, DogRunsFasterWhenHurt)
{
    auto dog = EnemyFactory::create(1, {EnemyType::Dog, Element::Normal}, LONG_PATH);
    const float speed = dog->speed();
    dog->takeDamage(dog->maxHealth() * 0.6f);
    dog->update(0.f);
    EXPECT_FLOAT_EQ(dog->speed(), speed * 1.3f);
    dog->update(0.f);  // only once
    EXPECT_FLOAT_EQ(dog->speed(), speed * 1.3f);
}

TEST(EnemyHooks, KillEmitsReward)
{
    int gold = 0;
    auto& bus = defender::EventBus::instance();
    const auto id = bus.subscribe<defender::EnemyKilled>(
        [&gold](const defender::EnemyKilled& event) { gold += event.reward; });
    auto enemy = EnemyFactory::create(1, {EnemyType::Duck, Element::Water}, LONG_PATH);
    const int reward = enemy->reward();
    enemy->takeDamage(1000.f);
    enemy->takeDamage(1000.f);  // already dead: no second reward
    bus.unsubscribe(id);
    EXPECT_EQ(gold, reward);
}

// ── Super boss ────────────────────────────────────────────────────────────────

TEST(SuperBoss, SplitsIntoThreeMiniBossesOfItsElement)
{
    defender::EntityManager entities;
    EnemyFactory factory(Biome::Volcano, 3);
    defender::WaveManager waves(entities, factory, LONG_PATH);

    auto& boss = entities.spawnEnemy(EnemyFactory::create(
        entities.nextId(), {EnemyType::Orangutan, Element::Wind, Rank::SuperBoss}, LONG_PATH));
    boss.update(500.f / boss.speed());  // walk a bit
    const sf::Vector2f where = boss.getPosition();
    boss.takeDamage(1e9f);
    entities.removeDeadEntities();
    EXPECT_FALSE(waves.isWaveComplete()) << "children are still waiting to spawn";

    waves.update(0.f);
    ASSERT_EQ(entities.enemies().size(), 3u);
    for (const auto& child : entities.enemies())
    {
        EXPECT_EQ(child->rank(), Rank::MiniBoss);
        EXPECT_EQ(child->element(), Element::Wind);
        EXPECT_EQ(child->generation(), 1);
        EXPECT_LE(child->getPosition().x, where.x);  // on the path, at or behind the boss
        EXPECT_NEAR(child->getPosition().x, where.x, 3 * defender::WaveManager::CHILD_SPACING);
        EXPECT_EQ(child->getPosition().y, 0.f);
    }
}

TEST(SuperBoss, StopsSplittingAtMaxGeneration)
{
    defender::EntityManager entities;
    EnemyFactory factory(Biome::Ice, 3);
    defender::WaveManager waves(entities, factory, LONG_PATH);
    auto& boss = entities.spawnEnemy(EnemyFactory::create(
        entities.nextId(), {EnemyType::Duck, Element::Earth, Rank::SuperBoss}, LONG_PATH,
        defender::MAX_SPAWN_GENERATION));
    boss.takeDamage(1e9f);
    entities.removeDeadEntities();
    waves.update(0.f);
    EXPECT_TRUE(entities.enemies().empty());
}

TEST(SuperBoss, MoveBackFollowsThePath)
{
    auto enemy = EnemyFactory::create(1, {EnemyType::Cat, Element::Fire}, LONG_PATH);
    enemy->placeOnPath(1, {1000.f, 10.f});  // just after the corner
    enemy->moveBack(30.f);
    EXPECT_NEAR(enemy->getPosition().x, 980.f, 0.01f);
    EXPECT_NEAR(enemy->getPosition().y, 0.f, 0.01f);
    enemy->moveBack(5000.f);
    EXPECT_EQ(enemy->getPosition(), LONG_PATH.front());
}

TEST(EnemyClick, ContainsGrowsWithScale)
{
    auto small = EnemyFactory::create(1, {EnemyType::Cat, Element::Fire}, LONG_PATH);
    auto big = EnemyFactory::create(2, {EnemyType::Cat, Element::Fire, Rank::SuperBoss},
                                    LONG_PATH);
    const sf::Vector2f farPoint(0.f, -30.f);
    EXPECT_TRUE(small->contains({0.f, -8.f}));
    EXPECT_FALSE(small->contains(farPoint));
    EXPECT_TRUE(big->contains(farPoint));
}
