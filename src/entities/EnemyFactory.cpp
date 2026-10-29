#include "defender/entities/EnemyFactory.hpp"

#include <cmath>
#include <numeric>
#include <stdexcept>
#include <string>

namespace defender
{

namespace
{

// ── Probability matrices ──────────────────────────────────────────────────────
//
// Relative weights per biome (a row sums to 100 to read as percentages).
// Every weight must stay > 0: the static_assert below enforces it.

constexpr SpawnTable DEFAULT_SPAWN_TABLE = {{
    //            Water  Fire  Earth  Wind        Octopus Orangutan Cat Dog Duck Frog
    /* Desert  */ {{10, 35, 35, 20},             {5, 20, 30, 30, 5, 10}},
    /* Ice     */ {{45, 5, 15, 35},              {20, 10, 15, 20, 25, 10}},
    /* Volcano */ {{5, 50, 35, 10},              {10, 25, 25, 20, 10, 10}},
    /* Forest  */ {{25, 10, 40, 25},             {10, 30, 15, 15, 15, 15}},
}};

constexpr bool allPositive(const SpawnTable& table)
{
    for (const SpawnWeights& row : table)
    {
        for (const double w : row.elements)
        {
            if (!(w > 0.0))
            {
                return false;
            }
        }
        for (const double w : row.types)
        {
            if (!(w > 0.0))
            {
                return false;
            }
        }
    }
    return true;
}

static_assert(allPositive(DEFAULT_SPAWN_TABLE),
              "Spawn weights must all be > 0: no enemy may have a 0% chance");

void validate(const SpawnTable& table)
{
    for (std::size_t biome = 0; biome < table.size(); ++biome)
    {
        auto check = [biome](double w)
        {
            if (!std::isfinite(w) || w <= 0.0)
            {
                throw std::invalid_argument("EnemyFactory: biome " + std::to_string(biome) +
                                            " has a weight <= 0 (no enemy may be at 0%)");
            }
        };
        for (const double w : table[biome].elements)
        {
            check(w);
        }
        for (const double w : table[biome].types)
        {
            check(w);
        }
    }
}

template <std::size_t N>
double share(const std::array<double, N>& weights, std::size_t index)
{
    return weights[index] / std::accumulate(weights.begin(), weights.end(), 0.0);
}

}  // namespace

const SpawnTable& defaultSpawnTable()
{
    return DEFAULT_SPAWN_TABLE;
}

EnemyFactory::EnemyFactory(Biome biome, std::uint32_t seed, const SpawnTable& table)
    : biome_(biome), weights_((validate(table), table[static_cast<std::size_t>(biome)])), rng_(seed)
{
    elementDistribution_ = {weights_.elements.begin(), weights_.elements.end()};
    typeDistribution_ = {weights_.types.begin(), weights_.types.end()};
}

EnemySpec EnemyFactory::roll(Rank rank)
{
    const auto type = static_cast<EnemyType>(typeDistribution_(rng_));
    const auto element = static_cast<Element>(elementDistribution_(rng_));
    return {type, element, rank};
}

std::unique_ptr<Enemy> EnemyFactory::create(EntityId id, std::span<const sf::Vector2f> path)
{
    return create(id, roll(), path);
}

std::unique_ptr<Enemy> EnemyFactory::create(EntityId id, EnemySpec spec,
                                            std::span<const sf::Vector2f> path, int generation)
{
    return std::make_unique<Enemy>(id, spec, path, generation);
}

double EnemyFactory::probability(Element element) const
{
    return share(weights_.elements, static_cast<std::size_t>(element));
}

double EnemyFactory::probability(EnemyType type) const
{
    return share(weights_.types, static_cast<std::size_t>(type));
}

double EnemyFactory::probability(EnemySpec spec) const
{
    return probability(spec.type) * probability(spec.element);
}

}  // namespace defender
