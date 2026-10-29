#include "defender/core/EventBus.hpp"
#include "defender/core/Events.hpp"
#include "defender/managers/EntityManager.hpp"
#include "defender/managers/LevelManager.hpp"
#include "defender/managers/WaveManager.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <vector>

using defender::Biome;
using defender::EnemyFactory;
using defender::LevelManager;
using defender::planForWave;
using defender::Rank;

namespace
{

const std::vector<sf::Vector2f> SHORT_PATH{{0.f, 0.f}, {64.f, 0.f}};

float share(int wave, Rank rank)
{
    return planForWave(wave).rankShare[static_cast<std::size_t>(rank)];
}

/// Rank counts of a built wave.
std::array<int, defender::RANK_COUNT> rankCounts(const defender::WaveData& wave)
{
    std::array<int, defender::RANK_COUNT> counts{};
    for (const auto& entry : wave.enemies)
    {
        counts[static_cast<std::size_t>(entry.spec.rank)] += entry.count;
    }
    return counts;
}

/// A level manager on an empty game, to build waves.
struct Level
{
    defender::EntityManager entities;
    EnemyFactory factory{Biome::Forest, 21};
    defender::WaveManager waves{entities, factory, SHORT_PATH};
    LevelManager level{waves, factory, 21};
};

}  // namespace

// ── Curves ────────────────────────────────────────────────────────────────────

TEST(Difficulty, InterpolatesBetweenPoints)
{
    const std::array<defender::CurvePoint, 2> curve{{{10, 0.f}, {20, 100.f}}};
    EXPECT_FLOAT_EQ(defender::interpolate(curve, 5), 0.f);
    EXPECT_FLOAT_EQ(defender::interpolate(curve, 15), 50.f);
    EXPECT_FLOAT_EQ(defender::interpolate(curve, 30), 100.f);
}

TEST(Difficulty, EnemyCountGoesFrom5To200)
{
    EXPECT_EQ(planForWave(1).count, 5);
    EXPECT_EQ(planForWave(100).count, 200);
    for (int wave = 2; wave <= defender::LEVEL_WAVES; ++wave)
    {
        EXPECT_GE(planForWave(wave).count, planForWave(wave - 1).count) << "wave " << wave;
    }
    for (int wave = 100; wave <= 150; ++wave)
    {
        EXPECT_EQ(planForWave(wave).count, 200) << "wave " << wave;
    }
}

TEST(Difficulty, RanksUnlockAtTheirWave)
{
    EXPECT_EQ(share(9, Rank::MiniBoss), 0.f);
    EXPECT_GT(share(10, Rank::MiniBoss), 0.f);
    EXPECT_EQ(share(24, Rank::Boss), 0.f);
    EXPECT_GT(share(25, Rank::Boss), 0.f);
    EXPECT_EQ(share(49, Rank::SuperBoss), 0.f);
    EXPECT_GT(share(50, Rank::SuperBoss), 0.f);
}

TEST(Difficulty, HardEnemiesGetMoreFrequent)
{
    for (const Rank rank : {Rank::MiniBoss, Rank::Boss, Rank::SuperBoss})
    {
        for (int wave = defender::unlockWave(rank) + 1; wave <= defender::LEVEL_WAVES; ++wave)
        {
            EXPECT_GT(share(wave, rank), share(wave - 1, rank)) << "wave " << wave;
        }
    }
    // Waves 100-150: same count, harder content.
    EXPECT_GT(planForWave(150).elementalFactor, planForWave(100).elementalFactor);
    EXPECT_GT(share(150, Rank::SuperBoss), share(100, Rank::SuperBoss));
}

// ── Built waves ───────────────────────────────────────────────────────────────

TEST(LevelWaves, EarlyWavesHaveNoBosses)
{
    Level game;
    for (int wave = 1; wave < 10; ++wave)
    {
        const auto counts = rankCounts(game.level.buildWave(planForWave(wave)));
        EXPECT_EQ(counts[static_cast<std::size_t>(Rank::Normal)], planForWave(wave).count);
    }
}

TEST(LevelWaves, EachRankShowsUpOnItsUnlockWave)
{
    Level game;
    for (const Rank rank : {Rank::MiniBoss, Rank::Boss, Rank::SuperBoss})
    {
        const int wave = defender::unlockWave(rank);
        const auto counts = rankCounts(game.level.buildWave(planForWave(wave)));
        EXPECT_GE(counts[static_cast<std::size_t>(rank)], 1) << defender::rankName(rank);
        // Nothing stronger than the rank just unlocked.
        for (auto r = static_cast<std::size_t>(rank) + 1; r < defender::RANK_COUNT; ++r)
        {
            EXPECT_EQ(counts[r], 0);
        }
    }
}

TEST(LevelWaves, CountsMatchThePlanAndStrongestComesLast)
{
    Level game;
    for (const int wave : {1, 10, 37, 64, 100, 150})
    {
        const auto plan = planForWave(wave);
        const auto data = game.level.buildWave(plan);
        ASSERT_EQ(static_cast<int>(data.enemies.size()), plan.count);
        const auto strongest = std::ranges::max(data.enemies, {}, [](const auto& e)
                                                { return e.spec.rank; }).spec.rank;
        EXPECT_EQ(data.enemies.back().spec.rank, strongest) << "wave " << wave;
        EXPECT_FLOAT_EQ(data.spawnInterval, plan.spawnInterval);
    }
}

TEST(LevelWaves, LateWavesHaveManyBosses)
{
    Level game;
    const auto counts = rankCounts(game.level.buildWave(planForWave(150)));
    EXPECT_NEAR(counts[static_cast<std::size_t>(Rank::MiniBoss)], 50, 2);   // 25% of 200
    EXPECT_NEAR(counts[static_cast<std::size_t>(Rank::Boss)], 20, 2);       // 10%
    EXPECT_NEAR(counts[static_cast<std::size_t>(Rank::SuperBoss)], 10, 2);  // 5%
}

// ── Level flow ────────────────────────────────────────────────────────────────

TEST(LevelFlow, PlaysWavesOneAfterTheOther)
{
    Level game;
    std::vector<defender::WaveCompleted> completed;
    auto& bus = defender::EventBus::instance();
    const auto id = bus.subscribe<defender::WaveCompleted>(
        [&completed](const defender::WaveCompleted& event) { completed.push_back(event); });

    EXPECT_EQ(game.level.phase(), LevelManager::Phase::Break);
    EXPECT_EQ(game.level.wave(), 0);

    // Run the game loop until two waves are cleared.
    const float dt = 1.f / 30.f;
    for (int frame = 0; frame < 30 * 300 && completed.size() < 2; ++frame)
    {
        game.waves.update(dt);
        game.entities.updateAll(dt);
        game.entities.removeDeadEntities();
        game.level.update(dt);
    }
    bus.unsubscribe(id);

    ASSERT_EQ(completed.size(), 2u);
    EXPECT_EQ(completed[0].waveNumber, 1);
    EXPECT_EQ(completed[1].waveNumber, 2);
    EXPECT_EQ(completed[0].bonusGold, planForWave(1).bonusGold);
    EXPECT_EQ(completed[0].skillPoints, 1);
    EXPECT_EQ(game.level.phase(), LevelManager::Phase::Break);
    EXPECT_NEAR(game.level.breakRemaining(), LevelManager::BREAK_TIME, 0.1f);
}

TEST(LevelFlow, NextWaveCanStartEarly)
{
    Level game;
    game.level.startNextWaveNow();
    EXPECT_EQ(game.level.phase(), LevelManager::Phase::Running);
    EXPECT_EQ(game.level.wave(), 1);
    EXPECT_EQ(game.waves.waveSize(), 5);
    game.level.startNextWaveNow();  // ignored while a wave runs
    EXPECT_EQ(game.level.wave(), 1);
}

TEST(LevelFlow, MilestoneWavesGiveMorePoints)
{
    EXPECT_EQ(planForWave(9).skillPoints, 1);
    EXPECT_EQ(planForWave(10).skillPoints, 3);
    EXPECT_GT(planForWave(50).bonusGold, planForWave(1).bonusGold);
}
