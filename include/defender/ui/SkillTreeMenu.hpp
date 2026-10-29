#pragma once

#include "defender/core/Player.hpp"
#include "defender/skills/SkillTree.hpp"

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>

#include <cstddef>
#include <optional>

namespace defender
{

/// Full-screen skill tree: nodes linked to their prerequisites, details of
/// the hovered node at the bottom, click to buy.
class SkillTreeMenu
{
public:
    explicit SkillTreeMenu(sf::Vector2f screenSize);

    void render(sf::RenderTarget& target, const SkillTree& tree, const Player& player,
                sf::Vector2f mouse) const;
    /// Handles a click at `point`. Returns true if the menu must close.
    bool click(sf::Vector2f point, SkillTree& tree, Player& player);

    /// Node box of a node, in screen coordinates.
    [[nodiscard]] sf::FloatRect nodeBox(const SkillNode& node) const;
    [[nodiscard]] std::optional<std::size_t> nodeAt(const SkillTree& tree, sf::Vector2f point) const;

    static constexpr float NODE_SIZE = 48.f;

private:
    sf::FloatRect panel_;
    sf::FloatRect closeButton_;
    sf::Vector2f gridOrigin_;
    float columnSpacing_ = 72.f;
    float rowSpacing_ = 86.f;
};

}  // namespace defender
