#include "defender/map/MapGenerator.hpp"
#include "defender/map/MapRenderer.hpp"
#include "defender/map/Maze.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <random>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

using defender::GridPos;
using defender::TileType;

namespace
{

defender::MapGeneratorConfig configWithSeed(std::uint32_t seed)
{
    defender::MapGeneratorConfig config;
    config.seed = seed;
    return config;
}

bool adjacent(GridPos a, GridPos b)
{
    return std::abs(a.x - b.x) + std::abs(a.y - b.y) == 1;
}

}  // namespace

// ── Maze ─────────────────────────────────────────────────────────────────────

TEST(Maze, IsPerfect)
{
    // A perfect maze is a spanning tree of its cells: every cell is
    // reachable and there are exactly cells - 1 passages (no loop).
    for (std::uint32_t seed = 0; seed < 20; ++seed)
    {
        std::mt19937 rng(seed);
        const int cellsX = 12;
        const int cellsY = 8;
        const auto maze = defender::Maze::generate(cellsX, cellsY, rng);
        ASSERT_EQ(maze.width(), 25);
        ASSERT_EQ(maze.height(), 17);

        int open = 0;
        for (int y = 0; y < maze.height(); ++y)
        {
            for (int x = 0; x < maze.width(); ++x)
            {
                open += maze.isOpen({x, y}) ? 1 : 0;
            }
        }
        const int cells = cellsX * cellsY;
        EXPECT_EQ(open, cells + (cells - 1));

        // Flood fill from the first cell reaches every open tile.
        std::set<std::pair<int, int>> seen{{1, 1}};
        std::vector<GridPos> stack{{1, 1}};
        while (!stack.empty())
        {
            const GridPos p = stack.back();
            stack.pop_back();
            for (const GridPos d : {GridPos{1, 0}, GridPos{-1, 0}, GridPos{0, 1}, GridPos{0, -1}})
            {
                const GridPos q{p.x + d.x, p.y + d.y};
                if (maze.isOpen(q) && seen.insert({q.x, q.y}).second)
                {
                    stack.push_back(q);
                }
            }
        }
        EXPECT_EQ(static_cast<int>(seen.size()), open);
    }
}

TEST(Maze, BorderIsClosed)
{
    std::mt19937 rng(7);
    const auto maze = defender::Maze::generate(5, 4, rng);
    for (int x = 0; x < maze.width(); ++x)
    {
        EXPECT_FALSE(maze.isOpen({x, 0}));
        EXPECT_FALSE(maze.isOpen({x, maze.height() - 1}));
    }
    EXPECT_FALSE(maze.isOpen({-1, 1}));
}

TEST(Maze, RejectsEmptySize)
{
    std::mt19937 rng(0);
    EXPECT_THROW(defender::Maze::generate(0, 3, rng), std::invalid_argument);
}

// ── Map generator ────────────────────────────────────────────────────────────

TEST(MapGenerator, PathGoesFromLeftEdgeToRightEdge)
{
    for (std::uint32_t seed = 0; seed < 50; ++seed)
    {
        const auto map = defender::generateMap(configWithSeed(seed));
        const auto path = map.path();
        ASSERT_GE(path.size(), 2u);
        EXPECT_EQ(path.front().x, 0);
        EXPECT_EQ(path.back().x, map.width() - 1);

        std::set<std::pair<int, int>> visited;
        for (std::size_t i = 0; i < path.size(); ++i)
        {
            EXPECT_EQ(map.at(path[i]), TileType::Path);
            EXPECT_TRUE(visited.insert({path[i].x, path[i].y}).second) << "path loops";
            if (i > 0)
            {
                EXPECT_TRUE(adjacent(path[i - 1], path[i])) << "gap at step " << i;
            }
        }
    }
}

TEST(MapGenerator, OnlyPathTilesArePath)
{
    const auto map = defender::generateMap(configWithSeed(3));
    std::size_t pathTiles = 0;
    for (int y = 0; y < map.height(); ++y)
    {
        for (int x = 0; x < map.width(); ++x)
        {
            pathTiles += map.at({x, y}) == TileType::Path ? 1 : 0;
        }
    }
    EXPECT_EQ(pathTiles, map.path().size());
}

TEST(MapGenerator, PathIsLong)
{
    // The exit is the farthest cell: the path should cross a good part
    // of the map, not just go straight across.
    for (std::uint32_t seed = 0; seed < 50; ++seed)
    {
        const auto map = defender::generateMap(configWithSeed(seed));
        EXPECT_GT(map.path().size(), static_cast<std::size_t>(map.width()) * 2);
    }
}

TEST(MapGenerator, HasBuildableTilesAlongThePath)
{
    for (std::uint32_t seed = 0; seed < 50; ++seed)
    {
        const auto map = defender::generateMap(configWithSeed(seed));
        int nextToPath = 0;
        for (const GridPos& p : map.path())
        {
            for (const GridPos d : {GridPos{1, 0}, GridPos{-1, 0}, GridPos{0, 1}, GridPos{0, -1}})
            {
                nextToPath += map.isBuildable({p.x + d.x, p.y + d.y}) ? 1 : 0;
            }
        }
        EXPECT_GE(nextToPath, 10) << "seed " << seed;
    }
}

TEST(MapGenerator, BorderIsNeverBuildable)
{
    const auto map = defender::generateMap(configWithSeed(11));
    for (int x = 0; x < map.width(); ++x)
    {
        EXPECT_NE(map.at({x, 0}), TileType::Buildable);
        EXPECT_NE(map.at({x, map.height() - 1}), TileType::Buildable);
    }
}

TEST(MapGenerator, SameSeedSameMap)
{
    const auto a = defender::generateMap(configWithSeed(1234));
    const auto b = defender::generateMap(configWithSeed(1234));
    for (int y = 0; y < a.height(); ++y)
    {
        for (int x = 0; x < a.width(); ++x)
        {
            ASSERT_EQ(a.at({x, y}), b.at({x, y}));
        }
    }
}

TEST(MapGenerator, DifferentSeedsDifferentMaps)
{
    const auto a = defender::generateMap(configWithSeed(1));
    const auto b = defender::generateMap(configWithSeed(2));
    int differences = 0;
    for (int y = 0; y < a.height(); ++y)
    {
        for (int x = 0; x < a.width(); ++x)
        {
            differences += a.at({x, y}) != b.at({x, y}) ? 1 : 0;
        }
    }
    EXPECT_GT(differences, 0);
}

TEST(MapGenerator, BiomeDoesNotChangeLayout)
{
    auto config = configWithSeed(99);
    const auto forest = defender::generateMap(config);
    config.biome = defender::Biome::Volcano;
    const auto volcano = defender::generateMap(config);
    EXPECT_EQ(volcano.biome(), defender::Biome::Volcano);
    for (int y = 0; y < forest.height(); ++y)
    {
        for (int x = 0; x < forest.width(); ++x)
        {
            ASSERT_EQ(forest.at({x, y}), volcano.at({x, y}));
        }
    }
}

TEST(MapGenerator, RejectsEvenOrTinySizes)
{
    auto config = configWithSeed(0);
    config.width = 24;
    EXPECT_THROW(defender::generateMap(config), std::invalid_argument);
    config.width = 3;
    EXPECT_THROW(defender::generateMap(config), std::invalid_argument);
}

TEST(MapGenerator, OtherSizesWork)
{
    auto config = configWithSeed(5);
    config.width = 41;
    config.height = 9;
    const auto map = defender::generateMap(config);
    EXPECT_EQ(map.path().front().x, 0);
    EXPECT_EQ(map.path().back().x, 40);
}

// ── Rendering helpers ────────────────────────────────────────────────────────

TEST(MapRenderer, PathMasksMatchNeighbors)
{
    const auto map = defender::generateMap(configWithSeed(8));
    const auto path = map.path();
    // Entry tile on the left edge connects west (off-map) and to the next tile.
    EXPECT_TRUE(defender::pathMask(map, path.front()) & 8);
    EXPECT_TRUE(defender::pathMask(map, path.back()) & 2);
    for (std::size_t i = 1; i + 1 < path.size(); ++i)
    {
        // A path tile in the middle has exactly two connections.
        const auto mask = defender::pathMask(map, path[i]);
        int bits = 0;
        for (int b = 0; b < 4; ++b)
        {
            bits += (mask >> b) & 1;
        }
        EXPECT_EQ(bits, 2) << "tile " << i;
    }
}
