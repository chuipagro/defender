#include "defender/ui/BuildBar.hpp"

#include "defender/ui/Ui.hpp"

#include <algorithm>
#include <string>

namespace defender
{

namespace
{

constexpr float BUTTON_W = 96.f;
constexpr float BUTTON_H = 44.f;
constexpr float SPACING = 6.f;
constexpr float MATERIAL_W = 64.f;

}  // namespace

BuildBar::BuildBar(float top, float width) : bar_(0.f, top, width, HEIGHT)
{
    for (std::size_t i = 0; i < ELEMENT_COUNT; ++i)
    {
        buttons_[i] = {6.f + (static_cast<float>(i) * (BUTTON_W + SPACING)),
                       top + ((HEIGHT - BUTTON_H) / 2.f), BUTTON_W, BUTTON_H};
    }
    // Materials on the right, one slot per element (Normal has none).
    float x = width - 8.f - ((ELEMENT_COUNT - 1) * MATERIAL_W);
    for (std::size_t i = 1; i < ELEMENT_COUNT; ++i)
    {
        materialSlots_[i] = {x + 16.f, top + (HEIGHT / 2.f)};
        x += MATERIAL_W;
    }
}

std::optional<TowerType> BuildBar::towerAt(sf::Vector2f point) const
{
    for (std::size_t i = 0; i < buttons_.size(); ++i)
    {
        if (buttons_[i].contains(point))
        {
            return static_cast<TowerType>(i);
        }
    }
    return std::nullopt;
}

sf::Vector2f BuildBar::materialSlot(Element element) const
{
    return materialSlots_[static_cast<std::size_t>(element)];
}

void BuildBar::render(sf::RenderTarget& target, const Player& player,
                      const TowerUpgrades& upgrades, std::optional<TowerType> selected,
                      sf::Vector2f mouse) const
{
    ui::drawPanel(target, bar_, ui::PANEL);
    ui::drawPanel(target, {bar_.left, bar_.top, bar_.width, 2.f}, ui::BORDER);

    for (std::size_t i = 0; i < buttons_.size(); ++i)
    {
        const auto type = static_cast<TowerType>(i);
        const sf::FloatRect& box = buttons_[i];
        const bool unlocked = upgrades.isUnlocked(type);
        const Cost cost = placeCost(type);
        const bool affordable = player.canAfford(cost);
        const bool hover = box.contains(mouse);

        const sf::Color border = selected == type ? ui::GOLD
                                 : unlocked && affordable ? ui::BORDER
                                                          : sf::Color(64, 60, 76);
        ui::drawPanel(target, box, hover ? ui::PANEL_LIGHT : sf::Color(34, 31, 45), border,
                      selected == type ? 3.f : 2.f);
        const sf::Color tint = unlocked ? sf::Color::White : sf::Color(85, 82, 96);
        ui::drawTowerIcon(target, type, {box.left + 4.f, box.top + 2.f, 32.f, box.height - 4.f}, tint);
        ui::drawText(target, std::to_string(i + 1), {box.left + 4.f, box.top + 3.f}, 1.f,
                     ui::TEXT_DIM);

        const float textX = box.left + 40.f;
        if (!unlocked)
        {
            ui::drawIcon(target, Icon::Lock, {textX + 12.f, box.top + 6.f}, 2.f);
            continue;
        }
        ui::drawIcon(target, Icon::Coin, {textX, box.top + 14.f}, 1.f);
        ui::drawText(target, std::to_string(cost.gold), {textX + 18.f, box.top + 15.f}, 2.f,
                     affordable ? ui::GOLD : ui::BAD);
    }

    for (std::size_t i = 1; i < ELEMENT_COUNT; ++i)
    {
        const auto element = static_cast<Element>(i);
        const sf::Vector2f slot = materialSlots_[i];
        ui::drawIcon(target, materialIcon(element), {slot.x - 16.f, slot.y - 16.f}, 2.f);
        const int count = player.materials(element);
        ui::drawText(target, std::to_string(count), {slot.x + 20.f, slot.y - 7.f}, 2.f,
                     count > 0 ? sf::Color::White : ui::TEXT_DIM);
    }

    if (const auto hovered = towerAt(mouse))
    {
        renderTooltip(target, *hovered, upgrades);
    }
}

void BuildBar::renderTooltip(sf::RenderTarget& target, TowerType type,
                             const TowerUpgrades& upgrades) const
{
    const TowerDefinition& def = towerDefinition(type);
    std::string hint = upgrades.isUnlocked(type) ? "Clic ou touche " +
                                                       std::to_string(static_cast<int>(type) + 1) +
                                                       ", puis clic sur une dalle"
                                                 : "A débloquer dans l'arbre de compétences [T]";
    const float width = std::max({ui::measureText(def.name, 2.f).x,
                                  ui::measureText(def.description, 2.f).x,
                                  ui::measureText(hint, 2.f).x}) + 24.f;
    const sf::FloatRect& button = buttons_[static_cast<std::size_t>(type)];
    const sf::FloatRect box(std::min(button.left, bar_.width - width - 4.f), bar_.top - 82.f, width,
                            76.f);
    ui::drawPanel(target, box, ui::PANEL, ui::BORDER);
    ui::drawText(target, def.name, {box.left + 12.f, box.top + 8.f}, 2.f, ui::GOLD);
    ui::drawText(target, def.description, {box.left + 12.f, box.top + 30.f}, 2.f);
    ui::drawText(target, hint, {box.left + 12.f, box.top + 52.f}, 2.f,
                 upgrades.isUnlocked(type) ? ui::TEXT_DIM : ui::BAD);
}

}  // namespace defender
