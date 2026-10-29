#include "defender/ui/TowerPanel.hpp"

#include "defender/map/MapRenderer.hpp"
#include "defender/ui/Ui.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace defender
{

namespace
{

std::string oneDecimal(float value)
{
    char text[16];
    std::snprintf(text, sizeof(text), "%.1f", static_cast<double>(value));
    return text;
}

}  // namespace

void TowerPanel::placeNear(sf::Vector2f towerOnScreen)
{
    // To the right of the tower, or to the left if there is no room.
    float left = towerOnScreen.x + 28.f;
    if (left + WIDTH > area_.left + area_.width)
    {
        left = towerOnScreen.x - 28.f - WIDTH;
    }
    left = std::clamp(left, area_.left + 4.f, area_.left + area_.width - WIDTH - 4.f);
    const float top = std::clamp(towerOnScreen.y - (HEIGHT / 2.f), area_.top + 4.f,
                                 area_.top + area_.height - HEIGHT - 4.f);
    box_ = {left, top, WIDTH, HEIGHT};
    upgradeButton_ = {left + 8.f, top + 56.f, WIDTH - 16.f, 28.f};
    sellButton_ = {left + 8.f, top + 92.f, WIDTH - 16.f, 28.f};
}

std::optional<TowerAction> TowerPanel::actionAt(sf::Vector2f point) const
{
    if (upgradeButton_.contains(point))
    {
        return TowerAction::Upgrade;
    }
    if (sellButton_.contains(point))
    {
        return TowerAction::Sell;
    }
    return std::nullopt;
}

void TowerPanel::render(sf::RenderTarget& target, const Tower& tower, const Player& player,
                        sf::Vector2f mouse) const
{
    ui::drawPanel(target, box_, ui::PANEL, ui::BORDER);
    const TowerDefinition& def = towerDefinition(tower.type());
    const float x = box_.left + 10.f;

    ui::drawText(target, def.name, {x, box_.top + 8.f}, 2.f, ui::GOLD);
    const std::string level =
        "Niv." + std::to_string(tower.level()) + "/" + std::to_string(TOWER_MAX_LEVEL);
    ui::drawText(target, level,
                 {box_.left + box_.width - 10.f - ui::measureText(level, 2.f).x, box_.top + 8.f}, 2.f);

    // Stats: damage, range (in tiles), shots per second
    const TowerStats& stats = tower.stats();
    float sx = x;
    auto stat = [&](Icon icon, const std::string& text)
    {
        ui::drawIcon(target, icon, {sx, box_.top + 30.f}, 1.f);
        ui::drawText(target, text, {sx + 18.f, box_.top + 31.f}, 2.f);
        sx += 18.f + ui::measureText(text, 2.f).x + 14.f;
    };
    stat(Icon::Sword, std::to_string(static_cast<int>(std::lround(stats.damage))));
    stat(Icon::Target, oneDecimal(stats.range / TILE_SIZE));
    stat(Icon::Bolt, oneDecimal(stats.fireRate) + "/s");

    // Upgrade: gold (+ materials for elemental towers)
    auto button = [&](const sf::FloatRect& box, bool enabled)
    {
        const sf::Color fill = !enabled ? sf::Color(30, 28, 38)
                               : box.contains(mouse) ? ui::PANEL_LIGHT
                                                     : sf::Color(40, 36, 54);
        ui::drawPanel(target, box, fill, enabled ? ui::BORDER : sf::Color(64, 60, 76));
    };
    if (tower.level() >= TOWER_MAX_LEVEL)
    {
        button(upgradeButton_, false);
        ui::drawText(target, "Niveau max", {upgradeButton_.left + 8.f, upgradeButton_.top + 7.f},
                     2.f, ui::TEXT_DIM);
    }
    else
    {
        const Cost cost = upgradeCost(tower.type(), tower.level());
        button(upgradeButton_, player.canAfford(cost));
        float bx = upgradeButton_.left + 6.f;
        const float iy = upgradeButton_.top + 6.f;
        ui::drawIcon(target, Icon::Upgrade, {bx, iy}, 1.f);
        bx += 24.f;
        ui::drawIcon(target, Icon::Coin, {bx, iy}, 1.f);
        const std::string gold = std::to_string(cost.gold);
        ui::drawText(target, gold, {bx + 18.f, iy + 1.f}, 2.f,
                     player.gold() >= cost.gold ? ui::GOLD : ui::BAD);
        bx += 18.f + ui::measureText(gold, 2.f).x + 14.f;
        if (cost.material != Element::Normal && cost.materials > 0)
        {
            ui::drawIcon(target, materialIcon(cost.material), {bx, iy}, 1.f);
            const int owned = player.materials(cost.material);
            const std::string count = std::to_string(owned) + "/" + std::to_string(cost.materials);
            ui::drawText(target, count, {bx + 18.f, iy + 1.f}, 2.f,
                         owned >= cost.materials ? sf::Color::White : ui::BAD);
        }
    }

    // Sell
    button(sellButton_, true);
    ui::drawIcon(target, Icon::Coin, {sellButton_.left + 6.f, sellButton_.top + 6.f}, 1.f);
    ui::drawText(target, "Vendre +" + std::to_string(sellValue(tower.type(), tower.level())),
                 {sellButton_.left + 30.f, sellButton_.top + 7.f}, 2.f);
}

}  // namespace defender
