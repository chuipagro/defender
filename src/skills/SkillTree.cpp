#include "defender/skills/SkillTree.hpp"

#include <algorithm>

namespace defender
{

SkillTree::SkillTree(std::span<const SkillNode> nodes) : nodes_(nodes), owned_(nodes.size(), false)
{
    for (std::size_t i = 0; i < nodes_.size(); ++i)
    {
        owned_[i] = nodes_[i].cost == 0 && nodes_[i].prerequisites.empty();
    }
    Player scratch;
    applyTo(scratch);
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
    const bool ready = std::ranges::all_of(nodes_[index].prerequisites,
                                           [this](std::string_view id) { return owns(id); });
    return ready ? State::Available : State::Locked;
}

bool SkillTree::canBuy(std::size_t index) const
{
    return state(index) == State::Available && points_ >= nodes_[index].cost;
}

bool SkillTree::buy(std::size_t index, Player& player)
{
    if (!canBuy(index))
    {
        return false;
    }
    points_ -= nodes_[index].cost;
    owned_[index] = true;
    if (nodes_[index].effect)
    {
        nodes_[index].effect(player, towers_);
    }
    return true;
}

void SkillTree::applyTo(Player& player)
{
    towers_ = {};
    for (std::size_t i = 0; i < nodes_.size(); ++i)
    {
        if (owned_[i] && nodes_[i].effect)
        {
            nodes_[i].effect(player, towers_);
        }
    }
}

}  // namespace defender
