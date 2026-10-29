#pragma once

#include "defender/map/Tile.hpp"

#include <random>
#include <vector>

namespace defender
{

/// Perfect maze laid out on a tile grid.
///
/// Cells sit at odd coordinates (2x+1, 2y+1) and the tiles between two
/// cells are walls or passages. "Perfect" means there is exactly one
/// route between any two cells: no loops, no unreachable area.
class Maze
{
public:
    /// Carves a maze of cellsX * cellsY cells with an iterative
    /// recursive backtracker (long winding corridors).
    /// Throws std::invalid_argument if a dimension is < 1.
    static Maze generate(int cellsX, int cellsY, std::mt19937& rng);

    /// Size in tiles: 2 * cells + 1.
    [[nodiscard]] int width() const { return width_; }
    [[nodiscard]] int height() const { return height_; }

    /// True for carved tiles (cells and passages); false for walls
    /// and for positions outside the maze.
    [[nodiscard]] bool isOpen(GridPos pos) const;

private:
    Maze(int cellsX, int cellsY);
    void open(GridPos pos);

    int width_;
    int height_;
    std::vector<bool> open_;
};

}  // namespace defender
