#include "defender/map/MapGenerator.hpp"

#include "defender/map/Maze.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <random>
#include <stdexcept>
#include <vector>

namespace defender
{

namespace
{

constexpr std::array<GridPos, 4> DIRECTIONS{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};

void validate(const MapGeneratorConfig& config)
{
    if (config.width < 5 || config.height < 5 || config.width % 2 == 0 ||
        config.height % 2 == 0)
    {
        throw std::invalid_argument("generateMap: width and height must be odd and >= 5");
    }
}

/// Breadth-first search over the open maze tiles from start.
/// Returns, for every tile, the index of the previous tile (-1 if unreached)
/// and fills distance (-1 if unreached).
std::vector<int> explore(const Maze& maze, GridPos start, std::vector<int>& distance)
{
    const int width = maze.width();
    const auto size = static_cast<std::size_t>(width) * static_cast<std::size_t>(maze.height());
    std::vector<int> previous(size, -1);
    distance.assign(size, -1);

    auto index = [width](GridPos p) { return (p.y * width) + p.x; };
    std::vector<GridPos> queue{start};
    distance[static_cast<std::size_t>(index(start))] = 0;
    for (std::size_t head = 0; head < queue.size(); ++head)
    {
        const GridPos current = queue[head];
        for (const GridPos& dir : DIRECTIONS)
        {
            const GridPos next{current.x + dir.x, current.y + dir.y};
            if (!maze.isOpen(next) || distance[static_cast<std::size_t>(index(next))] >= 0)
            {
                continue;
            }
            distance[static_cast<std::size_t>(index(next))] =
                distance[static_cast<std::size_t>(index(current))] + 1;
            previous[static_cast<std::size_t>(index(next))] = index(current);
            queue.push_back(next);
        }
    }
    return previous;
}

}  // namespace

Map generateMap(const MapGeneratorConfig& config)
{
    validate(config);
    std::mt19937 rng(config.seed);
    const int width = config.width;
    const int height = config.height;
    const int cellsY = (height - 1) / 2;

    const Maze maze = Maze::generate((width - 1) / 2, cellsY, rng);
    Map map(width, height, config.biome);

    // Entry (left column) and exit (right column): the pair of cells that
    // are the farthest apart in the maze, so the path winds as much as
    // possible. The maze itself is what makes every map different.
    GridPos start{1, 1};
    GridPos end{width - 2, 1};
    std::vector<int> previous;
    int best = -1;
    for (int startY = 1; startY < height; startY += 2)
    {
        std::vector<int> distance;
        std::vector<int> candidatePrevious = explore(maze, {1, startY}, distance);
        for (int endY = 1; endY < height; endY += 2)
        {
            const int length = distance[static_cast<std::size_t>((endY * width) + width - 2)];
            if (length > best)
            {
                best = length;
                start = {1, startY};
                end = {width - 2, endY};
                previous = candidatePrevious;
            }
        }
    }

    // The maze is perfect, so following `previous` back gives the only route.
    std::vector<GridPos> path{{width - 1, end.y}};
    for (int i = (end.y * width) + end.x; i >= 0; i = previous[static_cast<std::size_t>(i)])
    {
        path.push_back({i % width, i / width});
    }
    path.push_back({0, start.y});
    std::reverse(path.begin(), path.end());
    map.setPath(path);

    // Chebyshev distance to the path, capped: marks tiles near the path.
    std::vector<int> nearPath(static_cast<std::size_t>(width) * static_cast<std::size_t>(height),
                              config.buildReach + 1);
    for (const GridPos& p : path)
    {
        for (int dy = -config.buildReach; dy <= config.buildReach; ++dy)
        {
            for (int dx = -config.buildReach; dx <= config.buildReach; ++dx)
            {
                const GridPos q{p.x + dx, p.y + dy};
                if (map.contains(q))
                {
                    int& d = nearPath[static_cast<std::size_t>((q.y * width) + q.x)];
                    d = std::min(d, std::max(std::abs(dx), std::abs(dy)));
                }
            }
        }
    }

    std::bernoulli_distribution wallBuild(config.wallBuildChance);
    for (int y = 1; y < height - 1; ++y)
    {
        for (int x = 1; x < width - 1; ++x)
        {
            const GridPos pos{x, y};
            if (map.at(pos) == TileType::Path)
            {
                continue;
            }
            const int d = nearPath[static_cast<std::size_t>((y * width) + x)];
            if (maze.isOpen(pos) ? d <= config.buildReach : d <= 1 && wallBuild(rng))
            {
                map.set(pos, TileType::Buildable);
            }
        }
    }
    return map;
}

}  // namespace defender
