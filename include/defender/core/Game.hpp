#pragma once

#include "defender/core/StateMachine.hpp"
#include "defender/map/MapGenerator.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>

#include <random>

namespace defender
{

/// Owns the window and runs the main loop (60 fps).
class Game
{
public:
    Game();

    void run();

private:
    void pollEvents();
    /// Starts a new game on a map built from config_.
    void newGame();

    MapGeneratorConfig config_;
    std::random_device randomDevice_;
    sf::RenderWindow window_;
    StateMachine stateMachine_;
    sf::Clock clock_;
    sf::Clock titleClock_;
};

}  // namespace defender
