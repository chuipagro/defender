#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace defender
{

/// What a map cell allows.
enum class TileType : std::uint8_t
{
    Obstacle,   ///< Nothing can be placed here
    Buildable,  ///< A tower can be placed here
    Path,       ///< Enemies walk here
};

/// Visual theme of a map. Only changes the tileset, not the layout.
enum class Biome : std::uint8_t
{
    Desert,
    Ice,
    Volcano,
    Forest,
};

inline constexpr std::size_t BIOME_COUNT = 4;

/// Name used for asset files (assets/sprites/tiles/<name>.png).
constexpr std::string_view biomeName(Biome biome)
{
    switch (biome)
    {
        case Biome::Desert:
            return "desert";
        case Biome::Ice:
            return "ice";
        case Biome::Volcano:
            return "volcano";
        case Biome::Forest:
            return "forest";
    }
    return "forest";
}

/// Cell coordinates on the map grid.
struct GridPos
{
    int x = 0;
    int y = 0;

    bool operator==(const GridPos&) const = default;
};

}  // namespace defender
