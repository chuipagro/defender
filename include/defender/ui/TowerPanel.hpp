#pragma once

#include "defender/core/Player.hpp"
#include "defender/entities/Tower.hpp"

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>

#include <optional>

namespace defender
{

enum class TowerAction
{
    Upgrade,
    Sell,
};

/// Panel next to the selected tower: level, stats, upgrade (gold + materials)
/// and sell buttons. Positions are in screen coordinates.
class TowerPanel
{
public:
    /// The panel stays inside `area` (the map on screen).
    explicit TowerPanel(sf::FloatRect area) : area_(area) {}

    /// Places the panel next to a tower drawn at `towerOnScreen`.
    void placeNear(sf::Vector2f towerOnScreen);
    void render(sf::RenderTarget& target, const Tower& tower, const Player& player,
                sf::Vector2f mouse) const;
    [[nodiscard]] std::optional<TowerAction> actionAt(sf::Vector2f point) const;
    [[nodiscard]] bool contains(sf::Vector2f point) const { return box_.contains(point); }

    static constexpr float WIDTH = 276.f;
    static constexpr float HEIGHT = 128.f;

private:
    sf::FloatRect area_;
    sf::FloatRect box_;
    sf::FloatRect upgradeButton_;
    sf::FloatRect sellButton_;
};

}  // namespace defender
