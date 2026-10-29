#pragma once

#include "defender/map/Map.hpp"

#include <cstdint>

namespace defender
{

struct MapGeneratorConfig
{
    int width = 25;           ///< In tiles, odd and >= 5
    int height = 17;          ///< In tiles, odd and >= 5
    std::uint32_t seed = 0;   ///< Same seed + same config = same map
    Biome biome = Biome::Forest;
    /// Maze dead ends within this distance of the path become buildable.
    int buildReach = 2;
    /// Chance for a wall touching the path to become buildable.
    double wallBuildChance = 0.25;
};

/// Builds a tower-defense map from a perfect maze:
///  - the enemy path is the unique maze route from an entry on the left
///    edge to the farthest cell of the right edge;
///  - maze corridors near the path and some walls along it are buildable;
///  - everything else is an obstacle.
/// Throws std::invalid_argument on an even or too small size.
Map generateMap(const MapGeneratorConfig& config);

}  // namespace defender
