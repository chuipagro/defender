#include "defender/map/Maze.hpp"

#include <array>
#include <stdexcept>

namespace defender
{

namespace
{

constexpr std::array<GridPos, 4> DIRECTIONS{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};

GridPos cellToTile(GridPos cell)
{
    return {(2 * cell.x) + 1, (2 * cell.y) + 1};
}

}  // namespace

Maze::Maze(int cellsX, int cellsY)
    : width_((2 * cellsX) + 1),
      height_((2 * cellsY) + 1),
      open_(static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_), false)
{
}

Maze Maze::generate(int cellsX, int cellsY, std::mt19937& rng)
{
    if (cellsX < 1 || cellsY < 1)
    {
        throw std::invalid_argument("Maze: needs at least one cell in each direction");
    }

    Maze maze(cellsX, cellsY);
    std::vector<bool> visited(static_cast<std::size_t>(cellsX) * static_cast<std::size_t>(cellsY),
                              false);
    auto cellIndex = [cellsX](GridPos cell)
    { return (static_cast<std::size_t>(cell.y) * static_cast<std::size_t>(cellsX)) +
             static_cast<std::size_t>(cell.x); };

    // Iterative recursive backtracker: walk to a random unvisited
    // neighbor, carving the wall in between; backtrack at dead ends.
    std::vector<GridPos> stack{{0, 0}};
    visited[0] = true;
    maze.open(cellToTile({0, 0}));

    while (!stack.empty())
    {
        const GridPos current = stack.back();
        std::array<GridPos, 4> candidates{};
        std::size_t count = 0;
        for (const GridPos& dir : DIRECTIONS)
        {
            const GridPos next{current.x + dir.x, current.y + dir.y};
            if (next.x >= 0 && next.y >= 0 && next.x < cellsX && next.y < cellsY &&
                !visited[cellIndex(next)])
            {
                candidates[count++] = next;
            }
        }
        if (count == 0)
        {
            stack.pop_back();
            continue;
        }

        std::uniform_int_distribution<std::size_t> pick(0, count - 1);
        const GridPos next = candidates[pick(rng)];
        maze.open({current.x + next.x + 1, current.y + next.y + 1});  // wall in between
        maze.open(cellToTile(next));
        visited[cellIndex(next)] = true;
        stack.push_back(next);
    }
    return maze;
}

bool Maze::isOpen(GridPos pos) const
{
    if (pos.x < 0 || pos.y < 0 || pos.x >= width_ || pos.y >= height_)
    {
        return false;
    }
    return open_[(static_cast<std::size_t>(pos.y) * static_cast<std::size_t>(width_)) +
                 static_cast<std::size_t>(pos.x)];
}

void Maze::open(GridPos pos)
{
    open_[(static_cast<std::size_t>(pos.y) * static_cast<std::size_t>(width_)) +
          static_cast<std::size_t>(pos.x)] = true;
}

}  // namespace defender
