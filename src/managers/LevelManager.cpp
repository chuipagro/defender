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

constexpr int MINIBOSS_WAVE = 10;
constexpr int BOSS_WAVE = 25;
constexpr int SUPERBOSS_WAVE = 50;

/// Gold when a wave is cleared: 20 + 5 per wave.
int bonusGoldFor(int wave)
{
    return 20 + (5 * wave);
}

/// 1 skill point per wave, 3 every 10th wave.
int skillPointsFor(int wave)
{
    return wave % 10 == 0 ? 3 : 1;
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

WavePlan planForWave(int wave)
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
    plan.bonusGold = bonusGoldFor(wave);
    plan.skillPoints = skillPointsFor(wave);
    return plan;
}

// ── LevelManager ──────────────────────────────────────────────────────────────

LevelManager::LevelManager(WaveManager& waveManager, EnemyFactory& factory, std::uint32_t seed)
    : waveManager_(waveManager), factory_(factory), rng_(seed)
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
                if (wave_ >= LEVEL_WAVES)
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
    plan_ = planForWave(wave_);
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

    WaveData wave;
    wave.spawnInterval = plan.spawnInterval;
    for (const Rank rank : ranks)
    {
        wave.enemies.push_back({factory_.roll(rank, plan.elementalFactor), 1, 0.f});
    }
    return wave;
}

}  // namespace defender
