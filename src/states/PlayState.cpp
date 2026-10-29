#include "defender/states/PlayState.hpp"

#include "defender/core/Events.hpp"
#include "defender/managers/ResourceManager.hpp"

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

}  // namespace

PlayState::PlayState(const MapGeneratorConfig& config)
    : map_(generateMap(config)),
      path_(pathInPixels(map_)),
      enemyFactory_(config.biome, config.seed),
      waveManager_(entityManager_, enemyFactory_, path_),
      reachedBaseHandler_(EventBus::instance().subscribe<EnemyReachedBase>(
          [this](const EnemyReachedBase& /*event*/) { ++reachedBase_; })),
      killedHandler_(EventBus::instance().subscribe<EnemyKilled>(
          [this](const EnemyKilled& event)
          {
              gold_ += event.reward;
              ++kills_;
          }))
{
    mapRenderer_.build(map_, ResourceManager::instance().getTexture(tilesetId(config.biome)));
    waveManager_.startNextWave();
}

PlayState::~PlayState()
{
    EventBus::instance().unsubscribe(reachedBaseHandler_);
    EventBus::instance().unsubscribe(killedHandler_);
}

void PlayState::handleEvent(const sf::Event& event)
{
    // TODO: tower placement on buildable tiles, pause key.
    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Space)
    {
        timeScale_ = timeScale_ == 1.f ? FAST_FORWARD : 1.f;
    }
    // Debug until towers shoot: left click hurts the enemy under the cursor.
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
    {
        const sf::Vector2f point(static_cast<float>(event.mouseButton.x),
                                 static_cast<float>(event.mouseButton.y));
        // The front-most one is drawn last: the lowest on screen.
        Enemy* target = nullptr;
        for (const auto& enemy : entityManager_.enemies())
        {
            if (enemy->isAlive() && enemy->contains(point) &&
                (target == nullptr || enemy->getPosition().y > target->getPosition().y))
            {
                target = enemy.get();
            }
        }
        if (target != nullptr)
        {
            target->takeDamage(CLICK_DAMAGE);
        }
    }
}

void PlayState::update(float dt)
{
    const float scaled = dt * timeScale_;
    waveManager_.update(scaled);
    entityManager_.updateAll(scaled);
    entityManager_.removeDeadEntities();
}

void PlayState::render(sf::RenderTarget& target)
{
    target.draw(mapRenderer_);
    entityManager_.renderAll(target);
}

std::string PlayState::status() const
{
    std::string text = "Defender - " + std::string(biomeName(map_.biome())) + " - wave " +
                       std::to_string(waveManager_.getCurrentWave());
    if (waveManager_.isWaveComplete())
    {
        text += " complete";
    }
    else
    {
        text += ": " + std::to_string(waveManager_.spawnedCount()) + "/" +
                std::to_string(waveManager_.waveSize()) + " spawned, " +
                std::to_string(entityManager_.enemies().size()) + " on the map";
    }
    text += " - " + std::to_string(gold_) + " gold, " + std::to_string(kills_) + " kills, " +
            std::to_string(reachedBase_) + " reached the base";
    text += timeScale_ == 1.f ? "   [Space] x3" : "   x3 [Space]";
    text += "  [Click] hit  [R] new map  [1-4] biome";
    return text;
}

}  // namespace defender
