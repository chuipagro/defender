#pragma once

#include "defender/core/EventBus.hpp"
#include "defender/entities/EnemyFactory.hpp"
#include "defender/managers/EntityManager.hpp"
#include "defender/managers/WaveManager.hpp"
#include "defender/map/MapGenerator.hpp"
#include "defender/map/MapRenderer.hpp"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>

#include <string>
#include <vector>

namespace defender
{

/// A running game on one map: draws it and plays the waves.
///
/// Not copyable nor movable: WaveManager keeps references to members.
class PlayState
{
public:
    explicit PlayState(const MapGeneratorConfig& config);

    PlayState(const PlayState&) = delete;
    PlayState& operator=(const PlayState&) = delete;
    PlayState(PlayState&&) = delete;
    PlayState& operator=(PlayState&&) = delete;
    ~PlayState();

    void handleEvent(const sf::Event& event);
    void update(float dt);
    void render(sf::RenderTarget& target);

    /// One-line summary (wave, enemies, speed) for the window title.
    [[nodiscard]] std::string status() const;
    [[nodiscard]] const Map& map() const { return map_; }

    [[nodiscard]] int gold() const { return gold_; }

    /// Speed-up applied with the Space key.
    static constexpr float FAST_FORWARD = 3.f;
    static constexpr int STARTING_GOLD = 100;
    /// Damage of a left click on an enemy (debug, until towers shoot).
    static constexpr float CLICK_DAMAGE = 25.f;

private:
    Map map_;
    std::vector<sf::Vector2f> path_;  ///< Map path in pixels, from off-screen to off-screen
    EntityManager entityManager_;
    EnemyFactory enemyFactory_;
    WaveManager waveManager_;
    MapRenderer mapRenderer_;
    EventBus::HandlerId reachedBaseHandler_;
    EventBus::HandlerId killedHandler_;
    int reachedBase_ = 0;
    int gold_ = STARTING_GOLD;
    int kills_ = 0;
    float timeScale_ = 1.f;
};

}  // namespace defender
