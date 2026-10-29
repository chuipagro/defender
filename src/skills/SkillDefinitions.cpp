#include "defender/skills/SkillTree.hpp"

namespace defender
{

// ═════════════════════════════════════════════════════════════════════════════
//  Skill tree — edit this file to add nodes.
//
//  A node costs skill points, needs its prerequisites, and runs its effect
//  once when bought. (column, row) place it in the menu (columns 0-9, rows 0-3).
//  Texts are shown in game: lowercase accents are fine, uppercase ones are not
//  in the pixel font.
// ═════════════════════════════════════════════════════════════════════════════

namespace
{

/// Unlocks a tower type.
std::function<void(Player&, TowerUpgrades&)> unlock(TowerType type)
{
    return [type](Player& /*player*/, TowerUpgrades& towers)
    { towers.unlocked[static_cast<std::size_t>(type)] = true; };
}

/// +`amount` (e.g. 0.25 = +25%) on one stat of a tower type.
std::function<void(Player&, TowerUpgrades&)> boost(TowerType type, float TowerBonus::*stat,
                                                   float amount)
{
    return [type, stat, amount](Player& /*player*/, TowerUpgrades& towers)
    { towers.of(type).*stat += amount; };
}

/// The 3 nodes of an element: tower, power, fire rate.
void addElementBranch(std::vector<SkillNode>& nodes, TowerType type, std::string_view towerId,
                      std::string_view powerId, std::string_view rateId, std::string_view name,
                      std::string_view description, int column)
{
    nodes.push_back({.id = towerId,
                     .name = name,
                     .description = description,
                     .cost = 1,
                     .prerequisites = {"tower.normal"},
                     .column = column,
                     .row = 1,
                     .tower = type,
                     .effect = unlock(type)});
    nodes.push_back({.id = powerId,
                     .name = "Puissance",
                     .description = "+25% de dégâts pour cette tour.",
                     .cost = 1,
                     .prerequisites = {towerId},
                     .column = column,
                     .row = 2,
                     .icon = Icon::Sword,
                     .effect = boost(type, &TowerBonus::damage, 0.25f)});
    nodes.push_back({.id = rateId,
                     .name = "Cadence",
                     .description = "+20% de vitesse de tir pour cette tour.",
                     .cost = 2,
                     .prerequisites = {powerId},
                     .column = column,
                     .row = 3,
                     .icon = Icon::Bolt,
                     .effect = boost(type, &TowerBonus::fireRate, 0.2f)});
}

std::vector<SkillNode> buildTree()
{
    std::vector<SkillNode> nodes;

    // ── Towers ────────────────────────────────────────────────────────────────
    nodes.push_back({.id = "tower.normal",
                     .name = "Tour normale",
                     .description = "Une arbalète sur une tour de pierre. Disponible dès le départ.",
                     .cost = 0,
                     .column = 3,
                     .row = 0,
                     .tower = Element::Normal,
                     .effect = unlock(Element::Normal)});
    nodes.push_back({.id = "normal.power",
                     .name = "Carreaux affûtés",
                     .description = "+25% de dégâts pour la tour normale.",
                     .cost = 1,
                     .prerequisites = {"tower.normal"},
                     .column = 3,
                     .row = 1,
                     .icon = Icon::Sword,
                     .effect = boost(Element::Normal, &TowerBonus::damage, 0.25f)});
    nodes.push_back({.id = "normal.range",
                     .name = "Arbalète lourde",
                     .description = "+20% de portée pour la tour normale.",
                     .cost = 1,
                     .prerequisites = {"normal.power"},
                     .column = 3,
                     .row = 2,
                     .icon = Icon::Target,
                     .effect = boost(Element::Normal, &TowerBonus::range, 0.2f)});

    addElementBranch(nodes, Element::Water, "tower.water", "water.power", "water.rate",
                     "Tour d'eau", "Tire des bulles qui ralentissent les ennemis.", 0);
    addElementBranch(nodes, Element::Fire, "tower.fire", "fire.power", "fire.rate",
                     "Tour de feu", "Enflamme les ennemis: dégâts sur la durée.", 2);
    addElementBranch(nodes, Element::Earth, "tower.earth", "earth.power", "earth.rate",
                     "Tour de terre", "Lance des rochers: dégâts de zone.", 4);
    addElementBranch(nodes, Element::Wind, "tower.wind", "wind.power", "wind.rate",
                     "Tour de vent", "Bourrasques qui repoussent les ennemis.", 6);

    // ── Base (player) ─────────────────────────────────────────────────────────
    nodes.push_back({.id = "base.walls",
                     .name = "Remparts",
                     .description = "+15 points de vie maximum.",
                     .cost = 1,
                     .column = 8,
                     .row = 0,
                     .icon = Icon::Heart,
                     .effect = [](Player& player, TowerUpgrades&) { player.addMaxHealth(15.f); }});
    nodes.push_back({.id = "base.shield",
                     .name = "Bouclier",
                     .description = "Bouclier de 10, absorbe les dégâts avant la vie. Se recharge "
                                    "à chaque fin de vague.",
                     .cost = 1,
                     .prerequisites = {"base.walls"},
                     .column = 8,
                     .row = 1,
                     .icon = Icon::Shield,
                     .effect = [](Player& player, TowerUpgrades&) { player.addMaxShield(10.f); }});
    nodes.push_back({.id = "base.shield2",
                     .name = "Bouclier renforcé",
                     .description = "+15 de bouclier maximum.",
                     .cost = 2,
                     .prerequisites = {"base.shield"},
                     .column = 8,
                     .row = 2,
                     .icon = Icon::Shield,
                     .effect = [](Player& player, TowerUpgrades&) { player.addMaxShield(15.f); }});
    nodes.push_back({.id = "base.treasurer",
                     .name = "Trésorier",
                     .description = "+25% d'or à chaque fin de vague.",
                     .cost = 2,
                     .prerequisites = {"base.walls"},
                     .column = 9,
                     .row = 1,
                     .icon = Icon::Coin,
                     .effect = [](Player& player, TowerUpgrades&)
                     { player.addWaveGoldBonus(0.25f); }});
    return nodes;
}

}  // namespace

std::span<const SkillNode> skillNodes()
{
    static const std::vector<SkillNode> nodes = buildTree();
    return nodes;
}

}  // namespace defender
