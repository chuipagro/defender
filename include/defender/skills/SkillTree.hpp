#pragma once

#include "defender/core/Player.hpp"
#include "defender/entities/TowerDefinitions.hpp"
#include "defender/entities/Types.hpp"
#include "defender/ui/Icon.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace defender
{

/// One node of the tree. Defined in SkillDefinitions.cpp.
struct SkillNode
{
    std::string_view id{};         ///< Unique, used by `prerequisites`
    std::string_view name{};       ///< Shown in the menu
    std::string_view description{};  ///< Shown in the menu
    int cost = 4;                  ///< Skill points
    std::vector<std::string_view> prerequisites{};  ///< Ids of the nodes to own first
    int column = 0;                ///< Position in the menu grid
    int row = 0;
    std::optional<TowerType> tower{};  ///< Drawn with this tower's sprite...
    Icon icon = Icon::Star;            ///< ...or with this icon
    std::function<void(Player&, TowerUpgrades&)> effect{};  ///< Applied once, when bought
};

/// The game's skill tree (SkillDefinitions.cpp).
[[nodiscard]] std::span<const SkillNode> skillNodes();

/// Owned nodes and skill points. Kept from one level to the next: at the
/// start of each level, `applyTo` replays every owned node on the new player.
class SkillTree
{
public:
    enum class State
    {
        Locked,     ///< A required node is missing
        Available,  ///< Can be bought (if there are enough points)
        Owned,
    };

    /// Free nodes without requirements (cost 0) are owned from the start.
    explicit SkillTree(std::span<const SkillNode> nodes = skillNodes());

    [[nodiscard]] State state(std::size_t index) const;
    [[nodiscard]] bool canBuy(std::size_t index) const;
    /// Pays the points and applies the effect to `player`. False if not possible.
    bool buy(std::size_t index, Player& player);
    /// Resets the tower bonuses and applies every owned node to `player`.
    void applyTo(Player& player);

    [[nodiscard]] int points() const { return points_; }
    void addPoints(int amount) { points_ += std::max(amount, 0); }

    [[nodiscard]] std::span<const SkillNode> nodes() const { return nodes_; }
    [[nodiscard]] std::optional<std::size_t> find(std::string_view id) const;
    [[nodiscard]] bool owns(std::string_view id) const;
    [[nodiscard]] const TowerUpgrades& towers() const { return towers_; }

    static constexpr int STARTING_POINTS = 2;

private:
    std::span<const SkillNode> nodes_;
    std::vector<bool> owned_;
    TowerUpgrades towers_;
    int points_ = STARTING_POINTS;
};

}  // namespace defender
