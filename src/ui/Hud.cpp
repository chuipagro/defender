#include "defender/ui/Hud.hpp"

#include "defender/ui/Ui.hpp"

#include <cmath>
#include <string>

namespace defender
{

namespace
{

constexpr float ICON_SCALE = 2.f;
constexpr float TEXT_SCALE = 2.f;
constexpr float ICON_PX = ICON_SIZE * ICON_SCALE;
constexpr float GAP = 14.f;

std::string whole(float value)
{
    return std::to_string(static_cast<int>(std::ceil(value)));
}

/// Draws icon + text from `x`, returns the x after them.
float item(sf::RenderTarget& target, float x, Icon icon, const std::string& text, sf::Color color,
           sf::Color tint = sf::Color::White)
{
    const float iconY = (Hud::HEIGHT - ICON_PX) / 2.f;
    ui::drawIcon(target, icon, {x, iconY}, ICON_SCALE, tint);
    x += ICON_PX + 4.f;
    const sf::Vector2f size = ui::measureText(text, TEXT_SCALE);
    ui::drawText(target, text, {x, (Hud::HEIGHT - size.y) / 2.f}, TEXT_SCALE, color);
    return x + size.x + GAP;
}

}  // namespace

void Hud::render(sf::RenderTarget& target, const HudInfo& info)
{
    ui::drawPanel(target, {0.f, 0.f, width_, HEIGHT}, ui::PANEL);
    ui::drawPanel(target, {0.f, HEIGHT - 2.f, width_, 2.f}, ui::BORDER);

    const Player& player = info.player;
    float x = 8.f;
    const bool lowHealth = player.health() <= player.maxHealth() * 0.25f;
    x = item(target, x, Icon::Heart, whole(player.health()) + "/" + whole(player.maxHealth()),
             lowHealth ? ui::BAD : sf::Color::White);
    const bool hasShield = player.maxShield() > 0.f;
    x = item(target, x, Icon::Shield, whole(player.shield()) + "/" + whole(player.maxShield()),
             hasShield ? sf::Color::White : ui::TEXT_DIM,
             hasShield ? sf::Color::White : sf::Color(110, 110, 120));
    x = item(target, x, Icon::Coin, std::to_string(player.gold()), ui::GOLD);
    x = item(target, x, Icon::Star, std::to_string(player.skillPoints()),
             player.skillPoints() > 0 ? ui::GOLD : ui::TEXT_DIM);
    x = item(target, x, Icon::Flag,
             std::to_string(info.wave) + "/" + std::to_string(info.totalWaves), sf::Color::White);
    item(target, x, Icon::Paw, std::to_string(info.remainingEnemies), sf::Color::White);

    // Buttons, from the right edge.
    const float buttonH = HEIGHT - 8.f;
    float right = width_ - 6.f;
    auto button = [&](HudButton id, float width, bool active)
    {
        right -= width;
        const sf::FloatRect box(right, 4.f, width, buttonH);
        right -= 6.f;
        const bool hover = box.contains(info.mouse);
        ui::drawPanel(target, box, hover ? ui::PANEL_LIGHT : sf::Color(34, 31, 45),
                      active ? ui::GOLD : ui::BORDER);
        buttons_[static_cast<std::size_t>(id)] = box;
        visible_[static_cast<std::size_t>(id)] = true;
        return box;
    };
    auto centeredIcon = [&](Icon icon, const sf::FloatRect& box, sf::Color tint = sf::Color::White)
    {
        ui::drawIcon(target, icon,
                     {box.left + ((box.width - ICON_PX) / 2.f), box.top + ((box.height - ICON_PX) / 2.f)},
                     ICON_SCALE, tint);
    };

    centeredIcon(Icon::Tree, button(HudButton::SkillTree, 40.f, player.skillPoints() > 0));
    centeredIcon(Icon::FastForward, button(HudButton::Speed, 40.f, info.fastForward),
                 info.fastForward ? ui::GOLD : sf::Color::White);

    visible_[static_cast<std::size_t>(HudButton::NextWave)] = false;
    if (info.phase == LevelManager::Phase::Break)
    {
        // "Next wave in Ns": click to start it now.
        const std::string seconds = whole(info.breakRemaining) + "s";
        const float width = ICON_PX + 10.f + ui::measureText(seconds, TEXT_SCALE).x + 8.f;
        const sf::FloatRect box = button(HudButton::NextWave, width, true);
        ui::drawIcon(target, Icon::Play, {box.left + 4.f, box.top + ((box.height - ICON_PX) / 2.f)},
                     ICON_SCALE);
        ui::drawText(target, seconds,
                     {box.left + ICON_PX + 8.f,
                      box.top + ((box.height - ui::measureText(seconds, TEXT_SCALE).y) / 2.f)},
                     TEXT_SCALE);
    }
}

std::optional<HudButton> Hud::buttonAt(sf::Vector2f point) const
{
    for (std::size_t i = 0; i < buttons_.size(); ++i)
    {
        if (visible_[i] && buttons_[i].contains(point))
        {
            return static_cast<HudButton>(i);
        }
    }
    return std::nullopt;
}

}  // namespace defender
