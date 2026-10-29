#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace defender
{

/// Every numeric enemy stat. To add one (e.g. "Shield"):
///   1. add it here and in statName() below,
///   2. give it a value in EnemyDefinitions.cpp (baseStats, times, plus),
///   3. read it where it matters (enemy.stats().get(Stat::Shield)).
/// Special behaviors (armor, regen, splitting...) are hooks instead,
/// see EnemyDefinitions.hpp.
enum class Stat : std::uint8_t
{
    Health,  ///< Hit points
    Speed,   ///< Pixels per second (a tile is 32 px)
    Reward,  ///< Gold earned when killed
    Damage,  ///< Lives lost when it reaches the base
    Scale,   ///< Sprite size (1 = 32 px)
};

inline constexpr std::size_t STAT_COUNT = 5;

constexpr std::string_view statName(Stat stat)
{
    switch (stat)
    {
        case Stat::Health:
            return "health";
        case Stat::Speed:
            return "speed";
        case Stat::Reward:
            return "reward";
        case Stat::Damage:
            return "damage";
        case Stat::Scale:
            return "scale";
    }
    return "health";
}

/// One value per Stat.
struct EnemyStats
{
    std::array<float, STAT_COUNT> values{};

    [[nodiscard]] float get(Stat stat) const { return values[static_cast<std::size_t>(stat)]; }
    void set(Stat stat, float value) { values[static_cast<std::size_t>(stat)] = value; }
    float& operator[](Stat stat) { return values[static_cast<std::size_t>(stat)]; }

    [[nodiscard]] float health() const { return get(Stat::Health); }
    [[nodiscard]] float speed() const { return get(Stat::Speed); }
    [[nodiscard]] int reward() const { return static_cast<int>(get(Stat::Reward) + 0.5f); }
    [[nodiscard]] float damage() const { return get(Stat::Damage); }
    [[nodiscard]] float scale() const { return get(Stat::Scale); }
};

}  // namespace defender
