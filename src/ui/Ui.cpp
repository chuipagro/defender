#include "defender/ui/Ui.hpp"

#include "defender/managers/ResourceManager.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <algorithm>
#include <cmath>
#include <optional>

namespace defender::ui
{

namespace
{

constexpr int FONT_COLUMNS = 16;
constexpr char32_t FIRST_ASCII = 32;
constexpr char32_t LAST_ASCII = 126;
/// Extra glyphs after ASCII, same order as FONT_EXTRA in tools/sprites/ui.py.
constexpr std::u32string_view FONT_EXTRA = U"éèêàâçùûôî";

const sf::Color SHADOW{0, 0, 0, 170};

/// Decodes the next UTF-8 code point and advances `i`.
char32_t nextCodePoint(std::string_view text, std::size_t& i)
{
    const auto byte = static_cast<unsigned char>(text[i++]);
    if (byte < 0x80)
    {
        return byte;
    }
    int extra = 0;
    char32_t code = 0;
    if ((byte & 0xE0) == 0xC0)
    {
        extra = 1;
        code = byte & 0x1F;
    }
    else if ((byte & 0xF0) == 0xE0)
    {
        extra = 2;
        code = byte & 0x0F;
    }
    else
    {
        extra = 3;
        code = byte & 0x07;
    }
    for (int k = 0; k < extra && i < text.size(); ++k)
    {
        code = (code << 6) | (static_cast<unsigned char>(text[i++]) & 0x3F);
    }
    return code;
}

/// Index of a glyph in font.png, or nullopt for a space/unknown character.
std::optional<int> glyphIndex(char32_t code)
{
    if (code > FIRST_ASCII && code <= LAST_ASCII)
    {
        return static_cast<int>(code - FIRST_ASCII);
    }
    const auto extra = FONT_EXTRA.find(code);
    if (extra != std::u32string_view::npos)
    {
        return static_cast<int>(LAST_ASCII - FIRST_ASCII + 1 + extra);
    }
    // Uppercase accents are drawn without their accent.
    switch (code)
    {
        case U'É':
        case U'È':
        case U'Ê':
            return static_cast<int>('E' - FIRST_ASCII);
        case U'À':
        case U'Â':
            return static_cast<int>('A' - FIRST_ASCII);
        default:
            return std::nullopt;
    }
}

void appendText(sf::VertexArray& quads, std::string_view text, sf::Vector2f origin, float scale,
                sf::Color color)
{
    sf::Vector2f pen = origin;
    for (std::size_t i = 0; i < text.size();)
    {
        const char32_t code = nextCodePoint(text, i);
        if (code == U'\n')
        {
            pen = {origin.x, pen.y + ((GLYPH_H + 2) * scale)};
            continue;
        }
        if (const auto glyph = glyphIndex(code))
        {
            const auto tx = static_cast<float>((*glyph % FONT_COLUMNS) * GLYPH_W);
            const auto ty = static_cast<float>((*glyph / FONT_COLUMNS) * GLYPH_H);
            const float w = GLYPH_W * scale;
            const float h = GLYPH_H * scale;
            const sf::Vector2f corners[4] = {pen, pen + sf::Vector2f(w, 0.f),
                                             pen + sf::Vector2f(w, h), pen + sf::Vector2f(0.f, h)};
            const sf::Vector2f tex[4] = {{tx, ty},
                                         {tx + GLYPH_W, ty},
                                         {tx + GLYPH_W, ty + GLYPH_H},
                                         {tx, ty + GLYPH_H}};
            for (int k = 0; k < 4; ++k)
            {
                quads.append(sf::Vertex(corners[k], color, tex[k]));
            }
        }
        pen.x += GLYPH_W * scale;
    }
}

sf::Vector2f snap(sf::Vector2f p)
{
    return {std::round(p.x), std::round(p.y)};
}

}  // namespace

sf::Vector2f measureText(std::string_view text, float scale)
{
    int columns = 0;
    int widest = 0;
    int lines = text.empty() ? 0 : 1;
    for (std::size_t i = 0; i < text.size();)
    {
        if (nextCodePoint(text, i) == U'\n')
        {
            ++lines;
            columns = 0;
            continue;
        }
        widest = std::max(widest, ++columns);
    }
    // The last column of a glyph cell is spacing: not counted at the end.
    const float width = widest == 0 ? 0.f : ((widest * GLYPH_W) - 1) * scale;
    const float height = lines == 0 ? 0.f : ((lines * (GLYPH_H + 2)) - 3) * scale;
    return {width, height};
}

void drawText(sf::RenderTarget& target, std::string_view text, sf::Vector2f position, float scale,
              sf::Color color, bool shadow)
{
    sf::VertexArray quads(sf::Quads);
    position = snap(position);
    if (shadow)
    {
        appendText(quads, text, position + sf::Vector2f(scale, scale), scale, SHADOW);
    }
    appendText(quads, text, position, scale, color);
    target.draw(quads, &ResourceManager::instance().getTexture("sprites/ui/font.png"));
}

std::vector<std::string> wrapText(std::string_view text, float maxWidth, float scale)
{
    std::vector<std::string> lines;
    std::string line;
    std::size_t start = 0;
    while (start <= text.size())
    {
        std::size_t end = text.find(' ', start);
        if (end == std::string_view::npos)
        {
            end = text.size();
        }
        const std::string_view word = text.substr(start, end - start);
        const std::string candidate = line.empty() ? std::string(word) : line + " " + std::string(word);
        if (!line.empty() && measureText(candidate, scale).x > maxWidth)
        {
            lines.push_back(line);
            line = std::string(word);
        }
        else
        {
            line = candidate;
        }
        start = end + 1;
    }
    if (!line.empty())
    {
        lines.push_back(line);
    }
    return lines;
}

void drawIcon(sf::RenderTarget& target, Icon icon, sf::Vector2f position, float scale,
              sf::Color tint)
{
    sf::Sprite sprite(ResourceManager::instance().getTexture("sprites/ui/icons.png"),
                      sf::IntRect(static_cast<int>(icon) * ICON_SIZE, 0, ICON_SIZE, ICON_SIZE));
    sprite.setPosition(snap(position));
    sprite.setScale(scale, scale);
    sprite.setColor(tint);
    target.draw(sprite);
}

void drawTowerIcon(sf::RenderTarget& target, TowerType type, const sf::FloatRect& box,
                   sf::Color tint)
{
    const auto& texture = ResourceManager::instance().getTexture(
        "sprites/towers/" + std::string(elementName(type)) + ".png");
    constexpr int TOWER_W = 32;
    constexpr int TOWER_H = 48;
    // Whole scales only (square pixels). If the box is too short, the foot of
    // the pillar is cut off rather than shrinking the sprite.
    const float scale = std::max(1.f, std::floor(std::min(box.width / TOWER_W, box.height / TOWER_H)));
    const int visible = std::min(TOWER_H, static_cast<int>(box.height / scale));
    sf::Sprite sprite(texture, sf::IntRect(0, 0, TOWER_W, visible));
    sprite.setScale(scale, scale);
    sprite.setPosition(snap({box.left + ((box.width - (TOWER_W * scale)) / 2.f),
                             box.top + ((box.height - (static_cast<float>(visible) * scale)) / 2.f)}));
    sprite.setColor(tint);
    target.draw(sprite);
}

void drawPanel(sf::RenderTarget& target, const sf::FloatRect& box, sf::Color fill,
               sf::Color border, float borderWidth)
{
    sf::RectangleShape rect({box.width, box.height});
    rect.setPosition(box.left, box.top);
    rect.setFillColor(fill);
    if (border.a > 0)
    {
        rect.setOutlineColor(border);
        rect.setOutlineThickness(-borderWidth);
    }
    target.draw(rect);
}

void drawLine(sf::RenderTarget& target, sf::Vector2f from, sf::Vector2f to, float width,
              sf::Color color)
{
    const sf::Vector2f delta = to - from;
    const float length = std::hypot(delta.x, delta.y);
    if (length <= 0.f)
    {
        return;
    }
    const sf::Vector2f normal(-delta.y / length * width / 2.f, delta.x / length * width / 2.f);
    sf::VertexArray quad(sf::Quads, 4);
    quad[0] = sf::Vertex(from + normal, color);
    quad[1] = sf::Vertex(to + normal, color);
    quad[2] = sf::Vertex(to - normal, color);
    quad[3] = sf::Vertex(from - normal, color);
    target.draw(quad);
}

}  // namespace defender::ui
