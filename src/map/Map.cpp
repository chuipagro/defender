#include "defender/map/Map.hpp"

#include <stdexcept>
#include <utility>

namespace defender
{

Map::Map(int width, int height, Biome biome)
    : width_(width),
      height_(height),
      biome_(biome),
      tiles_(static_cast<std::size_t>(width) * static_cast<std::size_t>(height),
             TileType::Obstacle)
{
    if (width <= 0 || height <= 0)
    {
        throw std::invalid_argument("Map: width and height must be positive");
    }
}

bool Map::contains(GridPos pos) const
{
    return pos.x >= 0 && pos.y >= 0 && pos.x < width_ && pos.y < height_;
}

TileType Map::at(GridPos pos) const
{
    return contains(pos) ? tiles_[index(pos)] : TileType::Obstacle;
}

void Map::set(GridPos pos, TileType type)
{
    if (!contains(pos))
    {
        throw std::out_of_range("Map::set: position outside the map");
    }
    tiles_[index(pos)] = type;
}

void Map::setPath(std::vector<GridPos> path)
{
    for (const GridPos& pos : path)
    {
        set(pos, TileType::Path);
    }
    path_ = std::move(path);
}

std::size_t Map::index(GridPos pos) const
{
    return static_cast<std::size_t>(pos.y) * static_cast<std::size_t>(width_) +
           static_cast<std::size_t>(pos.x);
}

}  // namespace defender
