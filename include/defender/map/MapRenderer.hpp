#pragma once

#include "defender/map/Map.hpp"

#include <SFML/Graphics.hpp>

#include <cstdint>

namespace defender
{

constexpr int TILE_SIZE = 32;

/// Draws a Map with a biome tileset (assets/sprites/tiles/<biome>.png).
///
/// Tileset layout (see tools/sprites/tiles.py):
///   row 0, column m      path tile for neighbor mask m (N=1, E=2, S=4, W=8)
///   row 1, columns 0-3   buildable variants
///   row 1, columns 4-7   obstacle variants
///   row 1, columns 8-11  plain ground variants (also used for obstacles)
class MapRenderer : public sf::Drawable
{
public:
    /// Rebuilds the vertex array; call it after the map or tileset changes.
    void build(const Map& map, const sf::Texture& tileset);

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    sf::VertexArray vertices_{sf::Triangles};
    const sf::Texture* tileset_ = nullptr;
};

/// Path tile neighbor mask (N=1, E=2, S=4, W=8). Entry and exit tiles on
/// the left / right edges also connect outward.
std::uint8_t pathMask(const Map& map, GridPos pos);

/// Stable, well-mixed hash of a tile position, used to pick variants.
std::uint32_t tileHash(GridPos pos, std::uint32_t salt = 0);

}  // namespace defender
