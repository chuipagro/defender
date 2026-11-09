#pragma once

#include "defender/core/StateMachine.hpp"
#include "defender/map/MapGenerator.hpp"
#include "defender/skills/SkillTree.hpp"

#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>

#include <random>

namespace defender
{

/// Owns the window and runs the main loop (60 fps).
///
/// The game is drawn on a fixed-size canvas, then scaled to fit the window
/// (letterboxed). Mouse events are converted back to canvas coordinates.
class Game
{
public:
    Game();

    void run();

private:
    void pollEvents();
    void newGame();
    void nextLevel();
    void present();
    void toCanvas(sf::Event& event) const;

    MapGeneratorConfig config_;
    std::random_device randomDevice_;
    sf::Vector2u canvasSize_;
    sf::RenderWindow window_;
    sf::RenderTexture canvas_;
    SkillTree skills_;
    int level_ = 1;
    StateMachine stateMachine_;
    sf::Clock clock_;
    sf::Clock titleClock_;
};

}  // namespace defender
