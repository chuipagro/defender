#pragma once

#include "defender/map/Tile.hpp"

#include <span>
#include <vector>

namespace defender
{

/// Grid of tiles plus the ordered path enemies follow.
class Map
{
public:
    /// Creates a map filled with obstacles and no path.
    Map(int width, int height, Biome biome);

    [[nodiscard]] int width() const { return width_; }
    [[nodiscard]] int height() const { return height_; }
    [[nodiscard]] Biome biome() const { return biome_; }

    [[nodiscard]] bool contains(GridPos pos) const;
    /// Tile at pos; positions outside the map count as obstacles.
    [[nodiscard]] TileType at(GridPos pos) const;
    [[nodiscard]] bool isBuildable(GridPos pos) const { return at(pos) == TileType::Buildable; }
    void set(GridPos pos, TileType type);

    /// Path from the entry (first) to the exit (last), one tile per step.
    [[nodiscard]] std::span<const GridPos> path() const { return path_; }
    /// Stores the path and marks its tiles as TileType::Path.
    void setPath(std::vector<GridPos> path);

private:
    [[nodiscard]] std::size_t index(GridPos pos) const;

    int width_;
    int height_;
    Biome biome_;
    std::vector<TileType> tiles_;
    std::vector<GridPos> path_;
};

}  // namespace defender
