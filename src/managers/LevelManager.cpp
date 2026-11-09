#include "defender/managers/LevelManager.hpp"

#include "defender/core/EventBus.hpp"
#include "defender/core/Events.hpp"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace defender
{

namespace
{

// ═════════════════════════════════════════════════════════════════════════════
//  Difficulty curves — edit these to balance the level.
//  { wave, value }, linearly interpolated in between.
// ═════════════════════════════════════════════════════════════════════════════

/// Enemies per wave: 5 at wave 1, growing faster and faster up to 200 at
/// wave 100, then flat (waves 100-150 get harder through the ranks instead).
constexpr std::array<CurvePoint, 7> COUNT = {{
    {1, 5.f}, {10, 12.f}, {25, 30.f}, {50, 75.f}, {75, 135.f}, {100, 200.f}, {150, 200.f},
}};

/// Seconds between two enemies.
constexpr std::array<CurvePoint, 5> SPAWN_INTERVAL = {{
    {1, 1.f}, {25, 0.75f}, {50, 0.55f}, {100, 0.35f}, {150, 0.3f},
}};

/// Share of the wave for each rank, from its unlock wave on.
constexpr std::array<CurvePoint, 4> MINIBOSS_SHARE = {{
    {10, 0.03f}, {50, 0.08f}, {100, 0.15f}, {150, 0.25f},
}};
constexpr std::array<CurvePoint, 4> BOSS_SHARE = {{
    {25, 0.01f}, {50, 0.02f}, {100, 0.05f}, {150, 0.10f},
}};
constexpr std::array<CurvePoint, 3> SUPERBOSS_SHARE = {{
    {50, 0.005f}, {100, 0.02f}, {150, 0.05f},
}};

/// Elemental enemies: 1 = as likely as normal ones (see EnemyFactory).
constexpr std::array<CurvePoint, 5> ELEMENTAL_FACTOR = {{
    {1, 0.2f}, {25, 0.45f}, {50, 0.8f}, {100, 1.5f}, {150, 3.f},
}};

/// Health multiplier of every enemy (on top of its rank and element).
constexpr std::array<CurvePoint, 6> HEALTH = {{
    {1, 1.f}, {10, 1.5f}, {25, 2.5f}, {50, 5.f}, {100, 12.f}, {150, 25.f},
}};
/// Extra health per level after the first (0.1 = +10% per level).
constexpr float LEVEL_HEALTH_BONUS = 0.1f;

/// Chance for an enemy to be fast, and its speed multiplier (random in the range).
constexpr std::array<CurvePoint, 4> FAST_CHANCE = {{
    {1, 0.05f}, {20, 0.12f}, {50, 0.2f}, {150, 0.3f},
}};
constexpr float FAST_SPEED_MIN = 1.2f;
constexpr float FAST_SPEED_MAX = 1.5f;

/// From GROUP_WAVE on, enemies can spawn at the same time (side by side).
constexpr int GROUP_WAVE = 10;
constexpr std::array<CurvePoint, 3> GROUP_CHANCE = {{
    {10, 0.2f}, {50, 0.4f}, {150, 0.6f},
}};
constexpr std::array<CurvePoint, 3> MAX_GROUP = {{
    {10, 2.f}, {40, 3.f}, {100, 4.f},
}};
constexpr float LANE_SPACING = 8.f;

constexpr int MINIBOSS_WAVE = 10;
constexpr int BOSS_WAVE = 25;
constexpr int SUPERBOSS_WAVE = 50;

/// Level 1: 50 waves, then +10 per level (150 max).
constexpr int FIRST_LEVEL_WAVES = 50;
constexpr int WAVES_PER_LEVEL = 10;
constexpr int MAX_LEVEL_WAVES = 150;

int bonusGoldFor(int wave)
{
    return 20 + (5 * wave);
}

/// 1 point every 10 waves, +3 for clearing the level.
constexpr int LEVEL_CLEAR_POINTS = 3;
int skillPointsFor(int wave, int level)
{
    return (wave % 10 == 0 ? 1 : 0) + (wave == levelWaves(level) ? LEVEL_CLEAR_POINTS : 0);
}

}  // namespace

float interpolate(std::span<const CurvePoint> curve, int wave)
{
    if (curve.empty())
    {
        return 0.f;
    }
    if (wave <= curve.front().wave)
    {
        return curve.front().value;
    }
    for (std::size_t i = 1; i < curve.size(); ++i)
    {
        if (wave <= curve[i].wave)
        {
            const CurvePoint& a = curve[i - 1];
            const CurvePoint& b = curve[i];
            const float t = static_cast<float>(wave - a.wave) / static_cast<float>(b.wave - a.wave);
            return a.value + ((b.value - a.value) * t);
        }
    }
    return curve.back().value;
}

int unlockWave(Rank rank)
{
    switch (rank)
    {
        case Rank::Normal:
            return 1;
        case Rank::MiniBoss:
            return MINIBOSS_WAVE;
        case Rank::Boss:
            return BOSS_WAVE;
        case Rank::SuperBoss:
            return SUPERBOSS_WAVE;
    }
    return 1;
}

int levelWaves(int level)
{
    return std::min(FIRST_LEVEL_WAVES + (WAVES_PER_LEVEL * (std::max(level, 1) - 1)), MAX_LEVEL_WAVES);
}

WavePlan planForWave(int wave, int level)
{
    WavePlan plan;
    plan.wave = wave;
    plan.count = static_cast<int>(std::lround(interpolate(COUNT, wave)));
    plan.spawnInterval = interpolate(SPAWN_INTERVAL, wave);
    auto share = [wave](Rank rank, std::span<const CurvePoint> curve)
    { return wave < unlockWave(rank) ? 0.f : interpolate(curve, wave); };
    plan.rankShare[static_cast<std::size_t>(Rank::MiniBoss)] = share(Rank::MiniBoss, MINIBOSS_SHARE);
    plan.rankShare[static_cast<std::size_t>(Rank::Boss)] = share(Rank::Boss, BOSS_SHARE);
    plan.rankShare[static_cast<std::size_t>(Rank::SuperBoss)] =
        share(Rank::SuperBoss, SUPERBOSS_SHARE);
    plan.elementalFactor = interpolate(ELEMENTAL_FACTOR, wave);
    plan.health = interpolate(HEALTH, wave) *
                  (1.f + (LEVEL_HEALTH_BONUS * static_cast<float>(std::max(level, 1) - 1)));
    plan.fastChance = interpolate(FAST_CHANCE, wave);
    if (wave >= GROUP_WAVE)
    {
        plan.groupChance = interpolate(GROUP_CHANCE, wave);
        plan.maxGroup = static_cast<int>(interpolate(MAX_GROUP, wave));
    }
    plan.bonusGold = bonusGoldFor(wave);
    plan.skillPoints = skillPointsFor(wave, level);
    return plan;
}

// ── LevelManager ──────────────────────────────────────────────────────────────

LevelManager::LevelManager(WaveManager& waveManager, EnemyFactory& factory, std::uint32_t seed,
                           int level)
    : waveManager_(waveManager), factory_(factory), rng_(seed), level_(std::clamp(level, 1, LEVEL_COUNT))
{
}

void LevelManager::update(float dt)
{
    switch (phase_)
    {
        case Phase::Break:
            breakTimer_ -= dt;
            if (breakTimer_ <= 0.f)
            {
                startWave();
            }
            break;
        case Phase::Running:
            if (waveManager_.isWaveComplete())
            {
                EventBus::instance().emit(WaveCompleted{wave_, plan_.bonusGold, plan_.skillPoints});
                if (wave_ >= totalWaves())
                {
                    phase_ = Phase::Victory;
                }
                else
                {
                    phase_ = Phase::Break;
                    breakTimer_ = BREAK_TIME;
                }
            }
            break;
        case Phase::Victory:
            break;
    }
}

void LevelManager::startNextWaveNow()
{
    if (phase_ == Phase::Break)
    {
        startWave();
    }
}

void LevelManager::startWave()
{
    ++wave_;
    plan_ = planForWave(wave_, level_);
    phase_ = Phase::Running;
    breakTimer_ = 0.f;
    waveManager_.startWave(wave_, buildWave(plan_));
}

WaveData LevelManager::buildWave(const WavePlan& plan)
{
    // How many of each rank: expected count, rounded at random
    // (e.g. 2.3 -> 2 or 3, 3 with a 30% chance), at least 1 on its unlock wave.
    std::uniform_real_distribution<float> unit(0.f, 1.f);
    std::vector<Rank> ranks;
    ranks.reserve(static_cast<std::size_t>(plan.count));
    for (const Rank rank : {Rank::SuperBoss, Rank::Boss, Rank::MiniBoss})
    {
        const float expected =
            static_cast<float>(plan.count) * plan.rankShare[static_cast<std::size_t>(rank)];
        auto n = static_cast<int>(expected);
        if (unit(rng_) < expected - static_cast<float>(n))
        {
            ++n;
        }
        if (plan.wave == unlockWave(rank))
        {
            n = std::max(n, 1);
        }
        for (int i = 0; i < n && std::cmp_less(ranks.size(), plan.count); ++i)
        {
            ranks.push_back(rank);
        }
    }
    while (std::cmp_less(ranks.size(), plan.count))
    {
        ranks.push_back(Rank::Normal);
    }

    // Mixed up, but the strongest one closes the wave.
    std::shuffle(ranks.begin(), ranks.end(), rng_);
    if (!ranks.empty())
    {
        std::iter_swap(std::max_element(ranks.begin(), ranks.end()), ranks.end() - 1);
    }

    // Groups: an enemy may come with the previous one, spread on lanes.
    std::uniform_real_distribution<float> fastSpeed(FAST_SPEED_MIN, FAST_SPEED_MAX);
    WaveData wave;
    std::size_t groupStart = 0;
    auto spreadGroup = [&wave](std::size_t first)
    {
        const auto size = static_cast<float>(wave.enemies.size() - first);
        for (std::size_t i = first; i < wave.enemies.size(); ++i)
        {
            wave.enemies[i].lane = (static_cast<float>(i - first) - ((size - 1.f) / 2.f)) * LANE_SPACING;
        }
    };
    for (std::size_t i = 0; i < ranks.size(); ++i)
    {
        SpawnEntry entry{.spec = factory_.roll(ranks[i], plan.elementalFactor),
                         .boost = {.health = plan.health},
                         .delay = i == 0 ? 0.f : plan.spawnInterval};
        if (unit(rng_) < plan.fastChance)
        {
            entry.boost.speed = fastSpeed(rng_);
        }
        const bool joins = i > 0 && std::cmp_less(i - groupStart, plan.maxGroup) &&
                           unit(rng_) < plan.groupChance;
        if (joins)
        {
            entry.delay = 0.f;
        }
        else
        {
            spreadGroup(groupStart);
            groupStart = i;
        }
        wave.enemies.push_back(entry);
    }
    spreadGroup(groupStart);
    return wave;
}

}  // namespace defender
