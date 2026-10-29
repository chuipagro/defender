#include "defender/core/EventBus.hpp"
#include "defender/core/Events.hpp"
#include "defender/entities/EnemyFactory.hpp"
#include "defender/managers/EntityManager.hpp"
#include "defender/managers/WaveManager.hpp"

#include <gtest/gtest.h>

#include <array>
#include <stdexcept>
#include <vector>

using defender::Biome;
using defender::Element;
using defender::EnemyFactory;
using defender::EnemyType;

namespace
{

constexpr std::array<Biome, 4> BIOMES{Biome::Desert, Biome::Ice, Biome::Volcano, Biome::Forest};
constexpr std::array<Element, 5> ELEMENTS{Element::Normal, Element::Water, Element::Fire,
                                         Element::Earth, Element::Wind};
constexpr std::array<EnemyType, 6> TYPES{EnemyType::Octopus, EnemyType::Orangutan, EnemyType::Cat,
                                        EnemyType::Dog,     EnemyType::Duck,      EnemyType::Frog};

}  // namespace

// ── Probability matrices ─────────────────────────────────────────────────────

TEST(EnemyFactory, NothingIsEverAtZeroPercent)
{
    for (const Biome biome : BIOMES)
    {
        const EnemyFactory factory(biome, 0);
        for (const Element e : ELEMENTS)
        {
            EXPECT_GT(factory.probability(e), 0.0);
        }
        for (const EnemyType t : TYPES)
        {
            EXPECT_GT(factory.probability(t), 0.0);
        }
    }
}

TEST(EnemyFactory, ProbabilitiesSumToOne)
{
    for (const Biome biome : BIOMES)
    {
        const EnemyFactory factory(biome, 0);
        double elements = 0.0;
        double joint = 0.0;
        for (const Element e : ELEMENTS)
        {
            elements += factory.probability(e);
            for (const EnemyType t : TYPES)
            {
                joint += factory.probability(defender::EnemySpec{t, e});
            }
        }
        EXPECT_NEAR(elements, 1.0, 1e-9);
        EXPECT_NEAR(joint, 1.0, 1e-9);
    }
}

TEST(EnemyFactory, BiomesFavorTheirElements)
{
    const EnemyFactory ice(Biome::Ice, 0);
    EXPECT_GT(ice.probability(Element::Water), ice.probability(Element::Fire));
    EXPECT_GT(ice.probability(Element::Wind), ice.probability(Element::Fire));

    const EnemyFactory volcano(Biome::Volcano, 0);
    EXPECT_GT(volcano.probability(Element::Fire), volcano.probability(Element::Water));

    const EnemyFactory forest(Biome::Forest, 0);
    EXPECT_GT(forest.probability(Element::Earth), forest.probability(Element::Fire));
}

TEST(EnemyFactory, RollsFollowTheMatrix)
{
    // 20000 rolls: observed frequencies should be within 1.5 points of the table.
    for (const Biome biome : BIOMES)
    {
        EnemyFactory factory(biome, 42);
        std::array<int, 5> elementCounts{};
        std::array<int, 6> typeCounts{};
        const int rolls = 20000;
        for (int i = 0; i < rolls; ++i)
        {
            const auto spec = factory.roll();
            ++elementCounts[static_cast<std::size_t>(spec.element)];
            ++typeCounts[static_cast<std::size_t>(spec.type)];
        }
        for (const Element e : ELEMENTS)
        {
            EXPECT_NEAR(elementCounts[static_cast<std::size_t>(e)] / double(rolls),
                        factory.probability(e), 0.015);
        }
        for (const EnemyType t : TYPES)
        {
            EXPECT_NEAR(typeCounts[static_cast<std::size_t>(t)] / double(rolls),
                        factory.probability(t), 0.015);
        }
    }
}

TEST(EnemyFactory, RareElementsStillSpawn)
{
    // Fire is the rarest element on ice maps, but it must show up.
    EnemyFactory factory(Biome::Ice, 7);
    int fire = 0;
    for (int i = 0; i < 2000; ++i)
    {
        fire += factory.roll().element == Element::Fire ? 1 : 0;
    }
    EXPECT_GT(fire, 0);
}

TEST(EnemyFactory, ElementalFactorChangesTheShare)
{
    auto elementalShare = [](double factor)
    {
        EnemyFactory factory(Biome::Forest, 11);
        int elemental = 0;
        for (int i = 0; i < 4000; ++i)
        {
            elemental += factory.roll(defender::Rank::Normal, factor).element != Element::Normal;
        }
        return elemental / 4000.0;
    };
    EXPECT_NEAR(elementalShare(1.0), 0.5, 0.03);
    EXPECT_NEAR(elementalShare(0.25), 0.2, 0.03);  // 25 / (100 + 25)
    EXPECT_NEAR(elementalShare(3.0), 0.75, 0.03);
}

TEST(EnemyFactory, SameSeedSameEnemies)
{
    EnemyFactory a(Biome::Forest, 123);
    EnemyFactory b(Biome::Forest, 123);
    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(a.roll(), b.roll());
    }
}

TEST(EnemyFactory, RejectsAZeroWeight)
{
    defender::SpawnTable table = defender::defaultSpawnTable();
    table[static_cast<std::size_t>(Biome::Ice)].elements[static_cast<std::size_t>(Element::Fire)] =
        0.0;
    EXPECT_THROW(EnemyFactory(Biome::Desert, 0, table), std::invalid_argument);
}

TEST(EnemyFactory, AcceptsACustomTable)
{
    defender::SpawnTable table = defender::defaultSpawnTable();
    auto& desert = table[static_cast<std::size_t>(Biome::Desert)];
    desert.elements = {1, 1, 1, 1, 96};
    const EnemyFactory factory(Biome::Desert, 0, table);
    EXPECT_NEAR(factory.probability(Element::Wind), 0.96, 1e-9);
}

// ── Enemy creation ───────────────────────────────────────────────────────────

TEST(EnemyFactory, CreatesEnemyWithSpecAndStats)
{
    const std::vector<sf::Vector2f> path{{16.f, 48.f}, {48.f, 48.f}};
    const auto enemy = EnemyFactory::create(7, {EnemyType::Orangutan, Element::Fire}, path);
    ASSERT_NE(enemy, nullptr);
    EXPECT_EQ(enemy->id(), 7u);
    EXPECT_EQ(enemy->type(), EnemyType::Orangutan);
    EXPECT_EQ(enemy->element(), Element::Fire);
    EXPECT_FLOAT_EQ(enemy->health(),
                    defender::computeStats({EnemyType::Orangutan, Element::Fire}).health());
    EXPECT_EQ(enemy->getPosition(), path.front());
    EXPECT_TRUE(enemy->isAlive());
}

TEST(EnemyFactory, RandomCreateUsesTheRoll)
{
    const std::vector<sf::Vector2f> path{{0.f, 0.f}};
    EnemyFactory rolling(Biome::Volcano, 99);
    EnemyFactory creating(Biome::Volcano, 99);
    for (int i = 0; i < 20; ++i)
    {
        const auto spec = rolling.roll();
        const auto enemy = creating.create(static_cast<defender::EntityId>(i), path);
        EXPECT_EQ(enemy->type(), spec.type);
        EXPECT_EQ(enemy->element(), spec.element);
    }
}

TEST(Enemy, DiesWhenHealthReachesZero)
{
    const std::vector<sf::Vector2f> path{{0.f, 0.f}};
    auto enemy = EnemyFactory::create(1, {EnemyType::Duck, Element::Water}, path);
    enemy->takeDamage(enemy->health() - 1.f);
    EXPECT_TRUE(enemy->isAlive());
    enemy->takeDamage(1.f);
    EXPECT_FALSE(enemy->isAlive());
}

// ── Skeleton wiring ──────────────────────────────────────────────────────────

TEST(Skeleton, EntityManagerTakesFactoryEnemies)
{
    const std::vector<sf::Vector2f> path{{0.f, 0.f}};
    defender::EntityManager manager;
    EnemyFactory factory(Biome::Ice, 1);
    auto& enemy = manager.spawnEnemy(factory.create(manager.nextId(), path));
    EXPECT_EQ(manager.enemies().size(), 1u);
    EXPECT_EQ(manager.enemies().front().get(), &enemy);
}

TEST(Skeleton, WaveManagerStartsWaves)
{
    const std::vector<sf::Vector2f> path{{0.f, 0.f}};
    defender::EntityManager manager;
    EnemyFactory factory(Biome::Forest, 1);
    defender::WaveManager waves(manager, factory, path);
    waves.startWave(1, defender::WaveData{});
    EXPECT_EQ(waves.getCurrentWave(), 1);
}

TEST(Skeleton, EventBusCompiles)
{
    auto& bus = defender::EventBus::instance();
    const auto id =
        bus.subscribe<defender::EnemyKilled>([](const defender::EnemyKilled& /*event*/) {});
    bus.emit(defender::EnemyKilled{nullptr, 10});
    bus.unsubscribe(id);
    SUCCEED();
}
