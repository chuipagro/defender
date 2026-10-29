#pragma once

#include "defender/entities/EnemyFactory.hpp"
#include "defender/entities/Types.hpp"
#include "defender/managers/WaveManager.hpp"

#include <array>
#include <cstdint>
#include <random>
#include <span>

namespace defender
{

/// One point of a difficulty curve: the value at a given wave.
/// Between two points the value is linearly interpolated.
struct CurvePoint
{
    int wave = 1;
    float value = 0.f;
};

/// Value of `curve` at `wave` (flat before the first and after the last point).
[[nodiscard]] float interpolate(std::span<const CurvePoint> curve, int wave);

/// Everything that makes a wave harder than the previous one.
struct WavePlan
{
    int wave = 1;
    int count = 5;               ///< Enemies spawned by the wave (children not counted)
    float spawnInterval = 1.f;   ///< Seconds between two enemies
    std::array<float, RANK_COUNT> rankShare{};  ///< Share of the wave per rank (Normal unused)
    float elementalFactor = 1.f; ///< See EnemyFactory::roll
    int bonusGold = 0;           ///< Given when the wave is cleared
    int skillPoints = 0;         ///< Given when the wave is cleared
};

/// Difficulty of a wave (curves in LevelManager.cpp).
[[nodiscard]] WavePlan planForWave(int wave);

/// First wave where each rank can appear.
[[nodiscard]] int unlockWave(Rank rank);

inline constexpr int LEVEL_WAVES = 150;

/// Plays the waves of a level one after the other: a break, the wave, a
/// break, the next (harder) wave... until wave LEVEL_WAVES is cleared.
///
/// Emits WaveCompleted (bonus gold and skill points) at the end of each wave.
class LevelManager
{
public:
    enum class Phase
    {
        Break,    ///< Waiting before the next wave
        Running,  ///< A wave is being played
        Victory,  ///< The last wave was cleared
    };

    LevelManager(WaveManager& waveManager, EnemyFactory& factory, std::uint32_t seed);

    void update(float dt);
    /// Ends the break and starts the next wave right away.
    void startNextWaveNow();

    /// Rolls the enemies of a wave following its plan: the rank counts come
    /// from the shares, the strongest enemy comes last.
    [[nodiscard]] WaveData buildWave(const WavePlan& plan);

    [[nodiscard]] Phase phase() const { return phase_; }
    /// Current wave number (0 before the first one).
    [[nodiscard]] int wave() const { return wave_; }
    /// Seconds left before the next wave (during a break).
    [[nodiscard]] float breakRemaining() const { return breakTimer_; }
    [[nodiscard]] const WavePlan& currentPlan() const { return plan_; }

    static constexpr float FIRST_BREAK = 3.f;  ///< Seconds before wave 1
    static constexpr float BREAK_TIME = 8.f;   ///< Seconds between two waves

private:
    void startWave();

    WaveManager& waveManager_;
    EnemyFactory& factory_;
    std::mt19937 rng_;
    Phase phase_ = Phase::Break;
    int wave_ = 0;
    float breakTimer_ = FIRST_BREAK;
    WavePlan plan_;
};

}  // namespace defender
