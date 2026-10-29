#include "defender/skills/SkillTree.hpp"

#include <algorithm>

namespace defender
{

SkillTree::SkillTree(Player& player, std::span<const SkillNode> nodes)
    : nodes_(nodes), owned_(nodes.size(), false)
{
    for (std::size_t i = 0; i < nodes_.size(); ++i)
    {
        if (nodes_[i].cost == 0 && nodes_[i].prerequisites.empty())
        {
            buy(i, player);
        }
    }
}

std::optional<std::size_t> SkillTree::find(std::string_view id) const
{
    const auto it = std::ranges::find(nodes_, id, &SkillNode::id);
    if (it == nodes_.end())
    {
        return std::nullopt;
    }
    return static_cast<std::size_t>(it - nodes_.begin());
}

bool SkillTree::owns(std::string_view id) const
{
    const auto index = find(id);
    return index && owned_[*index];
}

SkillTree::State SkillTree::state(std::size_t index) const
{
    if (owned_.at(index))
    {
        return State::Owned;
    }
    const auto& prerequisites = nodes_[index].prerequisites;
    const bool ready = std::ranges::all_of(prerequisites,
                                           [this](std::string_view id) { return owns(id); });
    return ready ? State::Available : State::Locked;
}

bool SkillTree::canBuy(std::size_t index, const Player& player) const
{
    return state(index) == State::Available && player.skillPoints() >= nodes_[index].cost;
}

bool SkillTree::buy(std::size_t index, Player& player)
{
    if (!canBuy(index, player) || !player.spendSkillPoints(nodes_[index].cost))
    {
        return false;
    }
    owned_[index] = true;
    if (nodes_[index].effect)
    {
        nodes_[index].effect(player, towers_);
    }
    return true;
}

}  // namespace defender
