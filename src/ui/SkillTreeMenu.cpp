#include "defender/ui/SkillTreeMenu.hpp"

#include "defender/ui/Ui.hpp"

#include <string>

namespace defender
{

namespace
{

constexpr int GRID_COLUMNS = 10;
constexpr float MARGIN = 16.f;
const sf::Color LOCKED_FILL{30, 28, 38};
const sf::Color LOCKED_BORDER{64, 60, 76};
const sf::Color LOCKED_TINT{85, 82, 96};

sf::Vector2f center(const sf::FloatRect& box)
{
    return {box.left + (box.width / 2.f), box.top + (box.height / 2.f)};
}

}  // namespace

SkillTreeMenu::SkillTreeMenu(sf::Vector2f screenSize)
    : panel_(MARGIN, MARGIN, screenSize.x - (2 * MARGIN), screenSize.y - (2 * MARGIN))
{
    const float gridWidth = ((GRID_COLUMNS - 1) * columnSpacing_) + NODE_SIZE;
    gridOrigin_ = {panel_.left + ((panel_.width - gridWidth) / 2.f), panel_.top + 80.f};
    closeButton_ = {panel_.left + panel_.width - 44.f, panel_.top + 8.f, 36.f, 36.f};
}

sf::FloatRect SkillTreeMenu::nodeBox(const SkillNode& node) const
{
    return {gridOrigin_.x + (static_cast<float>(node.column) * columnSpacing_),
            gridOrigin_.y + (static_cast<float>(node.row) * rowSpacing_), NODE_SIZE, NODE_SIZE};
}

std::optional<std::size_t> SkillTreeMenu::nodeAt(const SkillTree& tree, sf::Vector2f point) const
{
    const auto nodes = tree.nodes();
    for (std::size_t i = 0; i < nodes.size(); ++i)
    {
        if (nodeBox(nodes[i]).contains(point))
        {
            return i;
        }
    }
    return std::nullopt;
}

bool SkillTreeMenu::click(sf::Vector2f point, SkillTree& tree, Player& player)
{
    if (closeButton_.contains(point) || !panel_.contains(point))
    {
        return true;
    }
    if (const auto index = nodeAt(tree, point))
    {
        tree.buy(*index, player);
    }
    return false;
}

void SkillTreeMenu::render(sf::RenderTarget& target, const SkillTree& tree, const Player& player,
                           sf::Vector2f mouse) const
{
    const auto size = target.getView().getSize();
    ui::drawPanel(target, {0.f, 0.f, size.x, size.y}, sf::Color(0, 0, 0, 150));
    ui::drawPanel(target, panel_, ui::PANEL, ui::BORDER, 3.f);

    // Title, points, close button
    ui::drawText(target, "Arbre de compétences", {panel_.left + 16.f, panel_.top + 16.f}, 3.f,
                 ui::GOLD);
    const std::string points = std::to_string(player.skillPoints());
    const float pointsX = closeButton_.left - 16.f - ui::measureText(points, 3.f).x;
    ui::drawText(target, points, {pointsX, panel_.top + 16.f}, 3.f, ui::GOLD);
    ui::drawIcon(target, Icon::Star, {pointsX - 38.f, panel_.top + 12.f}, 2.f);
    ui::drawPanel(target, closeButton_,
                  closeButton_.contains(mouse) ? ui::PANEL_LIGHT : sf::Color(34, 31, 45), ui::BORDER);
    ui::drawIcon(target, Icon::Close, {closeButton_.left + 2.f, closeButton_.top + 2.f}, 2.f);

    // Section labels
    auto label = [&](std::string_view text, int firstColumn, int lastColumn)
    {
        const float left = gridOrigin_.x + (static_cast<float>(firstColumn) * columnSpacing_);
        const float right = gridOrigin_.x + (static_cast<float>(lastColumn) * columnSpacing_) + NODE_SIZE;
        const float width = ui::measureText(text, 2.f).x;
        ui::drawText(target, text, {(left + right - width) / 2.f, gridOrigin_.y - 26.f}, 2.f,
                     ui::TEXT_DIM);
    };
    label("Tours", 0, 6);
    label("Base", 8, 9);

    const auto nodes = tree.nodes();

    // Links to prerequisites, under the nodes
    for (std::size_t i = 0; i < nodes.size(); ++i)
    {
        for (const std::string_view id : nodes[i].prerequisites)
        {
            const auto parent = tree.find(id);
            if (!parent)
            {
                continue;
            }
            const bool parentOwned = tree.state(*parent) == SkillTree::State::Owned;
            const bool owned = tree.state(i) == SkillTree::State::Owned;
            const sf::Color color = owned ? ui::GOLD : parentOwned ? ui::BORDER : LOCKED_BORDER;
            ui::drawLine(target, center(nodeBox(nodes[*parent])), center(nodeBox(nodes[i])), 4.f,
                         color);
        }
    }

    // Nodes
    const auto hovered = nodeAt(tree, mouse);
    for (std::size_t i = 0; i < nodes.size(); ++i)
    {
        const SkillNode& node = nodes[i];
        const sf::FloatRect box = nodeBox(node);
        const auto state = tree.state(i);
        const bool affordable = tree.canBuy(i, player);
        const bool hover = hovered == i;

        sf::Color fill = state == SkillTree::State::Locked ? LOCKED_FILL : sf::Color(40, 36, 54);
        if (hover)
        {
            fill = ui::PANEL_LIGHT;
        }
        const sf::Color border = state == SkillTree::State::Owned ? ui::GOLD
                                 : affordable                     ? ui::GOOD
                                 : state == SkillTree::State::Locked ? LOCKED_BORDER
                                                                     : ui::BORDER;
        ui::drawPanel(target, box, fill, border, 3.f);

        const sf::Color tint =
            state == SkillTree::State::Locked ? LOCKED_TINT : sf::Color::White;
        if (node.tower)
        {
            ui::drawTowerIcon(target, *node.tower,
                              {box.left + 3.f, box.top + 3.f, box.width - 6.f, box.height - 6.f}, tint);
        }
        else
        {
            ui::drawIcon(target, node.icon, {box.left + 8.f, box.top + 8.f}, 2.f, tint);
        }

        if (state == SkillTree::State::Owned)
        {
            ui::drawIcon(target, Icon::Check, {box.left + box.width - 12.f, box.top - 6.f}, 1.f);
        }
        else
        {
            if (state == SkillTree::State::Locked)
            {
                ui::drawIcon(target, Icon::Lock, {box.left + box.width - 12.f, box.top + box.height - 12.f},
                             1.f);
            }
            // Cost under the box
            const std::string cost = std::to_string(node.cost);
            const float width = 16.f + 2.f + ui::measureText(cost, 2.f).x;
            const float x = box.left + ((box.width - width) / 2.f);
            ui::drawIcon(target, Icon::Star, {x, box.top + box.height + 4.f}, 1.f);
            ui::drawText(target, cost, {x + 18.f, box.top + box.height + 5.f}, 2.f,
                         player.skillPoints() >= node.cost ? ui::GOLD : ui::TEXT_DIM);
        }
    }

    // Details of the hovered node
    const float infoTop = gridOrigin_.y + (3 * rowSpacing_) + NODE_SIZE + 34.f;
    const sf::FloatRect info(panel_.left + 12.f, infoTop, panel_.width - 24.f,
                             panel_.top + panel_.height - 12.f - infoTop);
    ui::drawPanel(target, info, sf::Color(34, 31, 45), ui::BORDER);
    const float textX = info.left + 12.f;
    if (!hovered)
    {
        ui::drawText(target, "Survolez une compétence pour voir ses détails.", {textX, info.top + 12.f},
                     2.f, ui::TEXT_DIM);
        ui::drawText(target, "Clic: acheter      [T] ou [Echap]: fermer", {textX, info.top + 40.f}, 2.f,
                     ui::TEXT_DIM);
        return;
    }

    const SkillNode& node = nodes[*hovered];
    ui::drawText(target, node.name, {textX, info.top + 10.f}, 2.f, ui::GOLD);
    float y = info.top + 34.f;
    for (const std::string& line : ui::wrapText(node.description, info.width - 24.f, 2.f))
    {
        ui::drawText(target, line, {textX, y}, 2.f);
        y += 20.f;
    }

    std::string status;
    sf::Color statusColor = ui::TEXT_DIM;
    switch (tree.state(*hovered))
    {
        case SkillTree::State::Owned:
            status = "Acquis";
            statusColor = ui::GOLD;
            break;
        case SkillTree::State::Available:
            if (tree.canBuy(*hovered, player))
            {
                status = "Cliquer pour acheter";
                statusColor = ui::GOOD;
            }
            else
            {
                status = "Pas assez de points";
                statusColor = ui::BAD;
            }
            break;
        case SkillTree::State::Locked:
        {
            status = "Requiert:";
            for (const std::string_view id : node.prerequisites)
            {
                if (const auto parent = tree.find(id); parent && !tree.owns(id))
                {
                    status += " " + std::string(nodes[*parent].name);
                }
            }
            statusColor = ui::BAD;
            break;
        }
    }
    ui::drawText(target, status, {textX, info.top + info.height - 26.f}, 2.f, statusColor);
}

}  // namespace defender
