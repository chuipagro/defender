#include "defender/states/PlayState.hpp"

#include "defender/core/Events.hpp"
#include "defender/managers/ResourceManager.hpp"
#include "defender/ui/Ui.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/View.hpp>

#include <algorithm>
#include <cmath>
#include <utility>

namespace defender
{

namespace
{

/// Tile centers of the map path, plus one point a tile outside the map at
/// each end so enemies walk in and out of the screen.
std::vector<sf::Vector2f> pathInPixels(const Map& map)
{
    const auto tile = static_cast<float>(TILE_SIZE);
    auto center = [tile](GridPos p)
    {
        return sf::Vector2f((static_cast<float>(p.x) + 0.5f) * tile,
                            (static_cast<float>(p.y) + 0.5f) * tile);
    };
    std::vector<sf::Vector2f> pixels;
    const auto path = map.path();
    if (path.empty())
    {
        return pixels;
    }
    pixels.reserve(path.size() + 2);
    pixels.push_back(center(path.front()) - sf::Vector2f(tile, 0.f));
    for (const GridPos& p : path)
    {
        pixels.push_back(center(p));
    }
    pixels.push_back(center(path.back()) + sf::Vector2f(tile, 0.f));
    return pixels;
}

std::string tilesetId(Biome biome)
{
    return "sprites/tiles/" + std::string(biomeName(biome)) + ".png";
}

/// Message shown when a wave brings something new.
std::string waveBanner(int wave)
{
    std::string text = "Vague " + std::to_string(wave);
    if (wave == unlockWave(Rank::MiniBoss))
    {
        text += "\nLes mini-boss arrivent !";
    }
    else if (wave == unlockWave(Rank::Boss))
    {
        text += "\nLes boss arrivent !";
    }
    else if (wave == unlockWave(Rank::SuperBoss))
    {
        text += "\nLes super boss arrivent !";
    }
    else if (wave == LEVEL_WAVES)
    {
        text += "\nDernière vague !";
    }
    return text;
}

}  // namespace

sf::Vector2u PlayState::screenSize(const MapGeneratorConfig& config)
{
    return {static_cast<unsigned>(config.width * TILE_SIZE),
            static_cast<unsigned>((config.height * TILE_SIZE) + static_cast<int>(Hud::HEIGHT) +
                                  static_cast<int>(BuildBar::HEIGHT))};
}

PlayState::PlayState(const MapGeneratorConfig& config)
    : map_(generateMap(config)),
      path_(pathInPixels(map_)),
      screen_(sf::Vector2f(screenSize(config))),
      enemyFactory_(config.biome, config.seed),
      waveManager_(entityManager_, enemyFactory_, path_),
      levelManager_(waveManager_, enemyFactory_, config.seed ^ 0x9E3779B9u),
      skillTree_(player_),
      hud_(screen_.x),
      buildBar_(screen_.y - BuildBar::HEIGHT, screen_.x),
      towerPanel_({0.f, Hud::HEIGHT, screen_.x, screen_.y - Hud::HEIGHT - BuildBar::HEIGHT}),
      skillMenu_(screen_),
      lootRng_(config.seed ^ 0x51ED270Bu)
{
    mapRenderer_.build(map_, ResourceManager::instance().getTexture(tilesetId(config.biome)));

    auto& bus = EventBus::instance();
    handlers_.push_back(bus.subscribe<EnemyReachedBase>(
        [this](const EnemyReachedBase& event) { player_.takeDamage(event.damage); }));
    handlers_.push_back(bus.subscribe<EnemyKilled>(
        [this](const EnemyKilled& event)
        {
            if (event.enemy != nullptr && event.enemy->path().data() == path_.data())
            {
                onEnemyKilled(*event.enemy, event.reward);
            }
        }));
    handlers_.push_back(bus.subscribe<WaveStarted>(
        [this](const WaveStarted& event) { showBanner(waveBanner(event.waveNumber)); }));
    handlers_.push_back(bus.subscribe<WaveCompleted>(
        [this](const WaveCompleted& event)
        {
            const int gold = player_.completeWave(event.bonusGold, event.skillPoints);
            showBanner("Vague " + std::to_string(event.waveNumber) + " terminée\n+" +
                       std::to_string(gold) + " or  +" + std::to_string(event.skillPoints) +
                       (event.skillPoints > 1 ? " points" : " point"));
        }));
}

PlayState::~PlayState()
{
    for (const auto id : handlers_)
    {
        EventBus::instance().unsubscribe(id);
    }
}

void PlayState::onEnemyKilled(const Enemy& enemy, int reward)
{
    player_.earn(reward);
    ++kills_;
    std::uniform_real_distribution<float> unit(0.f, 1.f);
    const int materials = materialDrop(enemy.spec(), unit(lootRng_));
    if (materials <= 0)
    {
        return;
    }
    player_.addMaterials(enemy.element(), materials);
    // A few visible drops, spread around the enemy.
    const sf::Vector2f from = worldToScreen(enemy.getPosition() - sf::Vector2f(0.f, 10.f));
    for (int i = 0; i < std::min(materials, 5); ++i)
    {
        const float spread = (static_cast<float>(i) - (static_cast<float>(std::min(materials, 5)) - 1.f) / 2.f) * 10.f;
        drops_.push_back({enemy.element(), from + sf::Vector2f(spread, 0.f), -0.05f * static_cast<float>(i)});
    }
}

bool PlayState::isOver() const
{
    return player_.isDead() || levelManager_.phase() == LevelManager::Phase::Victory;
}

bool PlayState::capturesEscape() const
{
    return menuOpen_ || build_.has_value() || selected_.has_value();
}

// ── Coordinates ───────────────────────────────────────────────────────────────

sf::Vector2f PlayState::worldToScreen(sf::Vector2f world) const
{
    return world + sf::Vector2f(0.f, Hud::HEIGHT);
}

std::optional<GridPos> PlayState::tileAt(sf::Vector2f world) const
{
    if (world.x < 0.f || world.y < 0.f)
    {
        return std::nullopt;
    }
    const GridPos tile{static_cast<int>(world.x) / TILE_SIZE, static_cast<int>(world.y) / TILE_SIZE};
    return map_.contains(tile) ? std::optional(tile) : std::nullopt;
}

// ── Towers ────────────────────────────────────────────────────────────────────

void PlayState::selectBuild(std::optional<TowerType> type)
{
    build_ = type;
    if (type)
    {
        selected_.reset();
    }
}

bool PlayState::canPlace(TowerType type, GridPos tile) const
{
    return map_.contains(tile) && map_.isBuildable(tile) && entityManager_.towerAt(tile) == nullptr &&
           skillTree_.towers().isUnlocked(type) && player_.canAfford(placeCost(type));
}

bool PlayState::placeTower(GridPos tile)
{
    if (!build_ || !canPlace(*build_, tile) || !player_.pay(placeCost(*build_)))
    {
        return false;
    }
    const sf::Vector2f center((static_cast<float>(tile.x) + 0.5f) * TILE_SIZE,
                              (static_cast<float>(tile.y) + 0.5f) * TILE_SIZE);
    entityManager_.addTower(*build_, tile, center, skillTree_.towers());
    return true;
}

bool PlayState::upgradeTower(GridPos tile)
{
    Tower* tower = entityManager_.towerAt(tile);
    if (tower == nullptr || tower->level() >= TOWER_MAX_LEVEL ||
        !player_.pay(upgradeCost(tower->type(), tower->level())))
    {
        return false;
    }
    return tower->upgrade(skillTree_.towers());
}

bool PlayState::sellTower(GridPos tile)
{
    Tower* tower = entityManager_.towerAt(tile);
    if (tower == nullptr)
    {
        return false;
    }
    player_.earn(sellValue(tower->type(), tower->level()));
    tower->remove();
    if (selected_ == tile)
    {
        selected_.reset();
    }
    return true;
}

void PlayState::select(std::optional<GridPos> tile)
{
    selected_ = tile;
    if (tile)
    {
        towerPanel_.placeNear(worldToScreen({(static_cast<float>(tile->x) + 0.5f) * TILE_SIZE,
                                             (static_cast<float>(tile->y) + 0.5f) * TILE_SIZE}));
    }
}

// ── Input ─────────────────────────────────────────────────────────────────────

void PlayState::handleEvent(const sf::Event& event)
{
    if (event.type == sf::Event::MouseMoved)
    {
        mouse_ = {static_cast<float>(event.mouseMove.x), static_cast<float>(event.mouseMove.y)};
        return;
    }
    if (isOver())
    {
        return;
    }
    const bool pressed = event.type == sf::Event::MouseButtonPressed;
    const sf::Vector2f click = pressed ? sf::Vector2f(static_cast<float>(event.mouseButton.x),
                                                      static_cast<float>(event.mouseButton.y))
                                       : sf::Vector2f();
    const bool leftClick = pressed && event.mouseButton.button == sf::Mouse::Left;
    const bool rightClick = pressed && event.mouseButton.button == sf::Mouse::Right;
    const bool key = event.type == sf::Event::KeyPressed;

    if (key && event.key.code == sf::Keyboard::T)
    {
        menuOpen_ = !menuOpen_;
        return;
    }
    if (menuOpen_)
    {
        if (key && event.key.code == sf::Keyboard::Escape)
        {
            menuOpen_ = false;
        }
        else if (leftClick)
        {
            menuOpen_ = !skillMenu_.click(click, skillTree_, player_);
            entityManager_.refreshTowers(skillTree_.towers());
        }
        return;
    }

    if (key)
    {
        switch (event.key.code)
        {
            case sf::Keyboard::Space:
                timeScale_ = timeScale_ == 1.f ? FAST_FORWARD : 1.f;
                break;
            case sf::Keyboard::N:
                levelManager_.startNextWaveNow();
                break;
            case sf::Keyboard::Escape:
                selectBuild(std::nullopt);
                select(std::nullopt);
                break;
            case sf::Keyboard::U:
                if (selected_)
                {
                    upgradeTower(*selected_);
                }
                break;
            default:
                if (event.key.code >= sf::Keyboard::Num1 &&
                    event.key.code < sf::Keyboard::Num1 + static_cast<int>(ELEMENT_COUNT))
                {
                    const auto type = static_cast<TowerType>(event.key.code - sf::Keyboard::Num1);
                    selectBuild(build_ == type ? std::nullopt : std::optional(type));
                }
                break;
        }
        return;
    }
    if (rightClick)
    {
        selectBuild(std::nullopt);
        select(std::nullopt);
        return;
    }
    if (!leftClick)
    {
        return;
    }

    if (click.y < Hud::HEIGHT)
    {
        if (const auto button = hud_.buttonAt(click))
        {
            switch (*button)
            {
                case HudButton::NextWave:
                    levelManager_.startNextWaveNow();
                    break;
                case HudButton::Speed:
                    timeScale_ = timeScale_ == 1.f ? FAST_FORWARD : 1.f;
                    break;
                case HudButton::SkillTree:
                    menuOpen_ = true;
                    break;
            }
        }
        return;
    }
    if (buildBar_.contains(click))
    {
        if (const auto type = buildBar_.towerAt(click))
        {
            selectBuild(build_ == *type ? std::nullopt : std::optional(*type));
        }
        return;
    }
    if (selected_ && towerPanel_.contains(click))
    {
        if (const auto action = towerPanel_.actionAt(click))
        {
            if (*action == TowerAction::Upgrade)
            {
                upgradeTower(*selected_);
            }
            else
            {
                sellTower(*selected_);
            }
        }
        return;
    }
    clickWorld(click - sf::Vector2f(0.f, Hud::HEIGHT));
}

void PlayState::clickWorld(sf::Vector2f world)
{
    const auto tile = tileAt(world);
    if (!tile)
    {
        return;
    }
    if (build_)
    {
        placeTower(*tile);  // stays in build mode to place several towers
        return;
    }
    select(entityManager_.towerAt(*tile) != nullptr ? tile : std::nullopt);
}

// ── Update ────────────────────────────────────────────────────────────────────

void PlayState::update(float dt)
{
    bannerTimer_ = std::max(bannerTimer_ - dt, 0.f);
    for (Drop& drop : drops_)
    {
        drop.time += dt;
    }
    std::erase_if(drops_, [](const Drop& drop) { return drop.time >= DROP_TIME; });
    if (menuOpen_ || isOver())
    {
        return;  // paused
    }
    const float scaled = dt * timeScale_;
    waveManager_.update(scaled);
    entityManager_.updateAll(scaled);
    entityManager_.removeDeadEntities();
    levelManager_.update(scaled);
}

void PlayState::showBanner(std::string text)
{
    banner_ = std::move(text);
    bannerTimer_ = BANNER_TIME;
}

// ── Render ────────────────────────────────────────────────────────────────────

void PlayState::render(sf::RenderTarget& target)
{
    // World, between the HUD and the build bar
    const sf::View defaultView = target.getDefaultView();
    const float mapHeight = screen_.y - Hud::HEIGHT - BuildBar::HEIGHT;
    sf::View world(sf::FloatRect(0.f, 0.f, screen_.x, mapHeight));
    world.setViewport({0.f, Hud::HEIGHT / screen_.y, 1.f, mapHeight / screen_.y});
    target.setView(world);
    target.draw(mapRenderer_);
    renderWorldOverlay(target);
    entityManager_.renderAll(target);
    if (build_)
    {
        // Ghost tower on the hovered tile
        if (const auto tile = tileAt(mouse_ - sf::Vector2f(0.f, Hud::HEIGHT)))
        {
            const sf::Vector2f center((static_cast<float>(tile->x) + 0.5f) * TILE_SIZE,
                                      (static_cast<float>(tile->y) + 0.5f) * TILE_SIZE);
            Tower::drawGhost(target, *build_, center,
                             canPlace(*build_, *tile) ? sf::Color(255, 255, 255, 170)
                                                      : sf::Color(255, 90, 90, 150));
        }
    }
    target.setView(defaultView);

    hud_.render(target, HudInfo{.player = player_,
                                .wave = levelManager_.wave(),
                                .totalWaves = LEVEL_WAVES,
                                .remainingEnemies = waveManager_.remainingEnemies(),
                                .phase = levelManager_.phase(),
                                .breakRemaining = levelManager_.breakRemaining(),
                                .fastForward = timeScale_ != 1.f,
                                .mouse = mouse_});
    if (selected_)
    {
        if (const Tower* tower = entityManager_.towerAt(*selected_))
        {
            towerPanel_.render(target, *tower, player_, mouse_);
        }
    }
    renderDrops(target);
    buildBar_.render(target, player_, skillTree_.towers(), build_, mouse_);
    if (!isOver())
    {
        renderBanner(target);
    }
    if (menuOpen_)
    {
        skillMenu_.render(target, skillTree_, player_, mouse_);
    }
    if (isOver())
    {
        renderEndScreen(target);
    }
}

void PlayState::renderWorldOverlay(sf::RenderTarget& target) const
{
    // Range circle of the selected tower, or of the tower being placed.
    auto rangeCircle = [&target](sf::Vector2f center, float radius, sf::Color color)
    {
        sf::CircleShape circle(radius, 48);
        circle.setOrigin(radius, radius);
        circle.setPosition(center);
        circle.setFillColor(sf::Color(color.r, color.g, color.b, 40));
        circle.setOutlineColor(sf::Color(color.r, color.g, color.b, 150));
        circle.setOutlineThickness(1.f);
        target.draw(circle);
    };
    if (selected_)
    {
        if (const Tower* tower = entityManager_.towerAt(*selected_))
        {
            rangeCircle(tower->getPosition(), tower->stats().range, sf::Color::White);
        }
    }
    if (build_)
    {
        if (const auto tile = tileAt(mouse_ - sf::Vector2f(0.f, Hud::HEIGHT)))
        {
            const sf::Vector2f center((static_cast<float>(tile->x) + 0.5f) * TILE_SIZE,
                                      (static_cast<float>(tile->y) + 0.5f) * TILE_SIZE);
            rangeCircle(center, towerStats(*build_, 1, skillTree_.towers()).range,
                        canPlace(*build_, *tile) ? sf::Color(120, 230, 110) : sf::Color(240, 80, 80));
        }
    }
}

void PlayState::renderDrops(sf::RenderTarget& target) const
{
    constexpr float POP = 0.35f;  // seconds spent popping up before flying
    for (const Drop& drop : drops_)
    {
        if (drop.time < 0.f)
        {
            continue;  // not out yet
        }
        sf::Vector2f position;
        if (drop.time < POP)
        {
            const float t = drop.time / POP;
            position = drop.from - sf::Vector2f(0.f, 18.f * std::sin(t * 3.14159f / 2.f));
        }
        else
        {
            const float t = std::min((drop.time - POP) / (DROP_TIME - POP), 1.f);
            const float ease = t * t;
            const sf::Vector2f start = drop.from - sf::Vector2f(0.f, 18.f);
            position = start + ((buildBar_.materialSlot(drop.element) - start) * ease);
        }
        ui::drawIcon(target, materialIcon(drop.element), position - sf::Vector2f(8.f, 8.f), 1.f);
    }
}

void PlayState::renderBanner(sf::RenderTarget& target) const
{
    if (bannerTimer_ <= 0.f || banner_.empty())
    {
        return;
    }
    // Fades out during the last second.
    const auto alpha = static_cast<sf::Uint8>(255.f * std::min(bannerTimer_, 1.f));
    const float scale = 3.f;
    const sf::Vector2f size = ui::measureText(banner_, scale);
    const sf::FloatRect box((screen_.x - size.x) / 2.f - 16.f, Hud::HEIGHT + 24.f, size.x + 32.f,
                            size.y + 24.f);
    ui::drawPanel(target, box, sf::Color(24, 22, 32, static_cast<sf::Uint8>(alpha * 0.8f)),
                  sf::Color(110, 100, 140, alpha));
    ui::drawText(target, banner_, {box.left + 16.f, box.top + 12.f}, scale,
                 sf::Color(245, 197, 66, alpha));
}

void PlayState::renderEndScreen(sf::RenderTarget& target) const
{
    ui::drawPanel(target, {0.f, 0.f, screen_.x, screen_.y}, sf::Color(0, 0, 0, 160));
    const bool victory = !player_.isDead();
    const std::string title = victory ? "Victoire !" : "Partie perdue";
    const std::string details = "Vague " + std::to_string(levelManager_.wave()) + "/" +
                                std::to_string(LEVEL_WAVES) + "   " + std::to_string(kills_) +
                                " ennemis vaincus";
    const std::string hint = "[R] nouvelle partie   [Echap] quitter";

    const sf::FloatRect box((screen_.x - 520.f) / 2.f, (screen_.y - 180.f) / 2.f, 520.f, 180.f);
    ui::drawPanel(target, box, ui::PANEL, victory ? ui::GOLD : ui::BAD, 3.f);
    auto centered = [&](const std::string& text, float y, float scale, sf::Color color)
    {
        const float width = ui::measureText(text, scale).x;
        ui::drawText(target, text, {box.left + ((box.width - width) / 2.f), y}, scale, color);
    };
    centered(title, box.top + 24.f, 5.f, victory ? ui::GOLD : ui::BAD);
    centered(details, box.top + 96.f, 2.f, sf::Color::White);
    centered(hint, box.top + 136.f, 2.f, ui::TEXT_DIM);
}

std::string PlayState::status() const
{
    return "Defender - " + std::string(biomeName(map_.biome())) +
           "   [1-5] tours  [U] améliorer  [T] compétences  [N] vague suivante  [Espace] x3  "
           "[R] nouvelle carte  [F1-F4] biome";
}

}  // namespace defender
