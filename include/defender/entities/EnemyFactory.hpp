#pragma once

#include "defender/entities/Enemy.hpp"
#include "defender/map/Tile.hpp"

#include <SFML/System/Vector2.hpp>

#include <array>
#include <cstdint>
#include <memory>
#include <random>
#include <span>

namespace defender
{

/// Relative spawn weights for one biome. Weights do not need to sum to
/// anything, but every one of them must be > 0: no enemy is ever at 0%.
struct SpawnWeights
{
    std::array<double, ELEMENT_COUNT> elements{};    ///< Indexed by Element
    std::array<double, ENEMY_TYPE_COUNT> types{};    ///< Indexed by EnemyType
};

/// One row of weights per biome, indexed by Biome.
using SpawnTable = std::array<SpawnWeights, BIOME_COUNT>;

/// The game's probability matrices (see EnemyFactory.cpp).
const SpawnTable& defaultSpawnTable();

/// Creates enemies at random, following the biome's probability matrices.
///
/// The element and the species are drawn independently, so
/// P(type, element) = P(type | biome) * P(element | biome).
class EnemyFactory
{
public:
    /// Throws std::invalid_argument if a weight of the table is <= 0.
    explicit EnemyFactory(Biome biome, std::uint32_t seed = std::random_device{}(),
                          const SpawnTable& table = defaultSpawnTable());

    /// Draws what the next enemy will be (species and element; rank as given).
    /// `elementalFactor` multiplies the weight of every element but Normal:
    /// 2 = elemental enemies twice as likely as in the table.
    [[nodiscard]] EnemySpec roll(Rank rank = Rank::Normal, double elementalFactor = 1.0);

    /// Draws a spec and builds the enemy.
    [[nodiscard]] std::unique_ptr<Enemy> create(EntityId id, std::span<const sf::Vector2f> path);

    /// Builds an enemy of a given spec (no randomness). Its stats and hooks
    /// come from EnemyDefinitions.cpp.
    [[nodiscard]] static std::unique_ptr<Enemy> create(EntityId id, EnemySpec spec,
                                                       std::span<const sf::Vector2f> path,
                                                       int generation = 0);

    /// Probabilities (0..1) for this factory's biome.
    [[nodiscard]] double probability(Element element) const;
    [[nodiscard]] double probability(EnemyType type) const;
    [[nodiscard]] double probability(EnemySpec spec) const;

    [[nodiscard]] Biome biome() const { return biome_; }

private:
    Biome biome_;
    SpawnWeights weights_;
    std::mt19937 rng_;
    std::discrete_distribution<std::size_t> elementDistribution_;
    std::discrete_distribution<std::size_t> typeDistribution_;
};

}  // namespace defender
