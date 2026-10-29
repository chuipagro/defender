#include "defender/map/MapRenderer.hpp"

#include <array>

namespace defender
{

namespace
{

constexpr int VARIANTS = 4;
constexpr int BUILDABLE_COLUMN = 0;
constexpr int OBSTACLE_COLUMN = 4;
constexpr int GROUND_COLUMN = 8;
/// Share of obstacle tiles drawn with an object; the rest is plain,
/// non-buildable ground, which keeps the map readable.
constexpr std::uint32_t OBSTACLE_OBJECT_PERCENT = 45;

sf::Vector2i tileInSheet(const Map& map, GridPos pos)
{
    const auto variant = static_cast<int>(tileHash(pos) % VARIANTS);
    switch (map.at(pos))
    {
        case TileType::Path:
            return {pathMask(map, pos), 0};
        case TileType::Buildable:
            return {BUILDABLE_COLUMN + variant, 1};
        case TileType::Obstacle:
            break;
    }
    const bool object = tileHash(pos, 1) % 100 < OBSTACLE_OBJECT_PERCENT;
    return {(object ? OBSTACLE_COLUMN : GROUND_COLUMN) + variant, 1};
}

}  // namespace

std::uint8_t pathMask(const Map& map, GridPos pos)
{
    auto connects = [&](int dx, int dy)
    {
        const GridPos next{pos.x + dx, pos.y + dy};
        if (!map.contains(next))
        {
            return dy == 0;  // entry / exit lead off the left and right edges
        }
        return map.at(next) == TileType::Path;
    };
    std::uint8_t mask = 0;
    mask |= connects(0, -1) ? 1 : 0;
    mask |= connects(1, 0) ? 2 : 0;
    mask |= connects(0, 1) ? 4 : 0;
    mask |= connects(-1, 0) ? 8 : 0;
    return mask;
}

std::uint32_t tileHash(GridPos pos, std::uint32_t salt)
{
    std::uint32_t h = (static_cast<std::uint32_t>(pos.x) * 0x27D4EB2Du) ^
                      (static_cast<std::uint32_t>(pos.y) * 0x165667B1u) ^ (salt * 0x9E3779B9u);
    h ^= h >> 15;
    h *= 0x2C1B3C6Du;
    h ^= h >> 12;
    h *= 0x297A2D39u;
    h ^= h >> 15;
    return h;
}

void MapRenderer::build(const Map& map, const sf::Texture& tileset)
{
    tileset_ = &tileset;
    vertices_.clear();
    vertices_.resize(static_cast<std::size_t>(map.width()) *
                     static_cast<std::size_t>(map.height()) * 6);

    const auto size = static_cast<float>(TILE_SIZE);
    std::size_t v = 0;
    for (int y = 0; y < map.height(); ++y)
    {
        for (int x = 0; x < map.width(); ++x)
        {
            const sf::Vector2i sheet = tileInSheet(map, {x, y});
            const sf::Vector2f pos(static_cast<float>(x) * size, static_cast<float>(y) * size);
            const sf::Vector2f tex(static_cast<float>(sheet.x) * size,
                                   static_cast<float>(sheet.y) * size);
            const std::array<sf::Vector2f, 6> corners{{{0, 0}, {size, 0}, {0, size},
                                                       {0, size}, {size, 0}, {size, size}}};
            for (const sf::Vector2f& corner : corners)
            {
                vertices_[v].position = pos + corner;
                vertices_[v].texCoords = tex + corner;
                ++v;
            }
        }
    }
}

void MapRenderer::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    states.texture = tileset_;
    target.draw(vertices_, states);
}

}  // namespace defender
