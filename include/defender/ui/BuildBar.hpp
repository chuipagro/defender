#pragma once

#include "defender/core/Player.hpp"
#include "defender/entities/TowerDefinitions.hpp"

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>

#include <array>
#include <optional>

namespace defender
{

/// Bottom bar: one button per tower type (keys 1-5) with its price, and the
/// player's elemental materials.
class BuildBar
{
public:
    /// The bar spans `width` pixels from y = `top`.
    BuildBar(float top, float width);

    void render(sf::RenderTarget& target, const Player& player, const TowerUpgrades& upgrades,
                std::optional<TowerType> selected, sf::Vector2f mouse) const;
    /// Tower button under `point`, if any.
    [[nodiscard]] std::optional<TowerType> towerAt(sf::Vector2f point) const;
    [[nodiscard]] bool contains(sf::Vector2f point) const { return bar_.contains(point); }
    /// Center of a material counter (where dropped materials fly to).
    [[nodiscard]] sf::Vector2f materialSlot(Element element) const;

    static constexpr float HEIGHT = 52.f;

private:
    void renderTooltip(sf::RenderTarget& target, TowerType type, const TowerUpgrades& upgrades) const;

    sf::FloatRect bar_;
    std::array<sf::FloatRect, ELEMENT_COUNT> buttons_{};    ///< Indexed by TowerType
    std::array<sf::Vector2f, ELEMENT_COUNT> materialSlots_{};  ///< Indexed by Element
};

}  // namespace defender
