#include "defender/core/Player.hpp"
#include "defender/skills/SkillTree.hpp"

#include <gtest/gtest.h>

#include <set>
#include <string_view>
#include <utility>

using defender::Player;
using defender::SkillTree;

// ── Player ────────────────────────────────────────────────────────────────────

TEST(Player, StartsWithGoldAndHealth)
{
    const Player player;
    EXPECT_EQ(player.gold(), Player::STARTING_GOLD);
    EXPECT_FLOAT_EQ(player.health(), Player::STARTING_HEALTH);
    EXPECT_FLOAT_EQ(player.maxShield(), 0.f);
    EXPECT_FALSE(player.isDead());
}

TEST(Player, SpendsOnlyWhatItHas)
{
    Player player;
    player.earn(50);
    EXPECT_EQ(player.gold(), Player::STARTING_GOLD + 50);
    EXPECT_FALSE(player.spend(10000));
    EXPECT_EQ(player.gold(), Player::STARTING_GOLD + 50);
    EXPECT_TRUE(player.spend(30));
    EXPECT_EQ(player.gold(), Player::STARTING_GOLD + 20);
}

TEST(Player, ShieldAbsorbsFirst)
{
    Player player;
    player.addMaxShield(10.f);
    player.takeDamage(4.f);
    EXPECT_FLOAT_EQ(player.shield(), 6.f);
    EXPECT_FLOAT_EQ(player.health(), Player::STARTING_HEALTH);
    player.takeDamage(10.f);
    EXPECT_FLOAT_EQ(player.shield(), 0.f);
    EXPECT_FLOAT_EQ(player.health(), Player::STARTING_HEALTH - 4.f);
}

TEST(Player, DiesAtZeroHealth)
{
    Player player;
    player.takeDamage(1000.f);
    EXPECT_TRUE(player.isDead());
    EXPECT_FLOAT_EQ(player.health(), 0.f);
    player.heal(10.f);
    EXPECT_TRUE(player.isDead());
}

TEST(Player, WaveEndGivesGoldPointsAndShield)
{
    Player player;
    player.addMaxShield(10.f);
    player.takeDamage(10.f);
    player.addWaveGoldBonus(0.5f);
    const int points = player.skillPoints();
    EXPECT_EQ(player.completeWave(100, 3), 150);
    EXPECT_EQ(player.gold(), Player::STARTING_GOLD + 150);
    EXPECT_EQ(player.skillPoints(), points + 3);
    EXPECT_FLOAT_EQ(player.shield(), 10.f);
}

// ── Skill tree ────────────────────────────────────────────────────────────────

TEST(SkillTree, DefinitionsAreConsistent)
{
    std::set<std::string_view> ids;
    std::set<std::pair<int, int>> cells;
    for (const auto& node : defender::skillNodes())
    {
        EXPECT_TRUE(ids.insert(node.id).second) << "duplicate id " << node.id;
        EXPECT_TRUE(cells.insert({node.column, node.row}).second) << node.id << " overlaps";
        EXPECT_GE(node.cost, 0);
        EXPECT_TRUE(node.effect) << node.id;
    }
    const Player player;
    Player copy = player;
    const SkillTree tree(copy);
    for (const auto& node : defender::skillNodes())
    {
        for (const auto id : node.prerequisites)
        {
            EXPECT_TRUE(tree.find(id).has_value()) << node.id << " needs unknown " << id;
        }
    }
}

TEST(SkillTree, NormalTowerIsOwnedFromTheStart)
{
    Player player;
    const SkillTree tree(player);
    EXPECT_TRUE(tree.owns("tower.normal"));
    EXPECT_TRUE(tree.towers().isUnlocked(defender::Element::Normal));
    EXPECT_FALSE(tree.towers().isUnlocked(defender::Element::Fire));
    EXPECT_EQ(player.skillPoints(), Player::STARTING_SKILL_POINTS);
}

TEST(SkillTree, BuyingNeedsPrerequisitesAndPoints)
{
    Player player;
    SkillTree tree(player);
    const auto fire = *tree.find("tower.fire");
    const auto firePower = *tree.find("fire.power");
    EXPECT_EQ(tree.state(firePower), SkillTree::State::Locked);
    EXPECT_FALSE(tree.buy(firePower, player));

    EXPECT_EQ(tree.state(fire), SkillTree::State::Available);
    EXPECT_TRUE(tree.buy(fire, player));
    EXPECT_TRUE(tree.towers().isUnlocked(defender::Element::Fire));
    EXPECT_EQ(tree.state(fire), SkillTree::State::Owned);
    EXPECT_FALSE(tree.buy(fire, player)) << "already owned";

    EXPECT_EQ(tree.state(firePower), SkillTree::State::Available);
    EXPECT_FALSE(tree.buy(firePower, player)) << "no points left";
    player.addSkillPoints(1);
    EXPECT_TRUE(tree.buy(firePower, player));
    EXPECT_FLOAT_EQ(tree.towers().of(defender::Element::Fire).damage, 1.25f);
}

TEST(SkillTree, BaseBranchUpgradesThePlayer)
{
    Player player;
    player.addSkillPoints(10);
    SkillTree tree(player);
    EXPECT_TRUE(tree.buy(*tree.find("base.walls"), player));
    EXPECT_FLOAT_EQ(player.maxHealth(), Player::STARTING_HEALTH + 15.f);
    EXPECT_TRUE(tree.buy(*tree.find("base.shield"), player));
    EXPECT_FLOAT_EQ(player.maxShield(), 10.f);
    EXPECT_FLOAT_EQ(player.shield(), 10.f);
    EXPECT_TRUE(tree.buy(*tree.find("base.treasurer"), player));
    EXPECT_FLOAT_EQ(player.waveGoldMultiplier(), 1.25f);
}
