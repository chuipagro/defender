#include "defender/core/Game.hpp"


#include <SFML/Window/Event.hpp>

#include <algorithm>

namespace defender
{

namespace
{

/// Longest frame step: avoids huge jumps after a freeze or a window drag.
constexpr float MAX_DT = 0.1f;

}  // namespace

Game::Game()
    : window_(sf::VideoMode(PlayState::screenSize(config_).x, PlayState::screenSize(config_).y),
              "Defender", sf::Style::Titlebar | sf::Style::Close)
{
    window_.setFramerateLimit(60);
    config_.seed = randomDevice_();
    config_.biome = static_cast<Biome>(randomDevice_() % BIOME_COUNT);
    // TODO: start on MenuState once the menu exists.
    newGame();
}

void Game::newGame()
{
    stateMachine_.change<PlayState>(config_);
    clock_.restart();
}

void Game::run()
{
    // Loop from docs/architecture.md (Game Loop).
    while (window_.isOpen())
    {
        pollEvents();
        stateMachine_.update(std::min(clock_.restart().asSeconds(), MAX_DT));
        window_.clear();
        stateMachine_.render(window_);
        window_.display();

        if (titleClock_.getElapsedTime().asSeconds() > 0.25f)
        {
            titleClock_.restart();
            if (const PlayState* play = stateMachine_.get<PlayState>())
            {
                window_.setTitle(play->status());
            }
        }
    }
}

void Game::pollEvents()
{
    sf::Event event{};
    while (window_.pollEvent(event))
    {
        // While the skill tree is open, keys belong to it (Escape closes it).
        const PlayState* play = stateMachine_.get<PlayState>();
        if (play != nullptr && play->isMenuOpen())
        {
            if (event.type == sf::Event::Closed)
            {
                window_.close();
                return;
            }
            stateMachine_.handleEvent(event);
            continue;
        }
        const bool escape =
            event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape;
        if (escape && play != nullptr && play->capturesEscape())
        {
            stateMachine_.handleEvent(event);  // cancels the tower placement / selection
            continue;
        }
        if (event.type == sf::Event::Closed || escape)
        {
            window_.close();
            return;
        }
        if (event.type == sf::Event::KeyPressed)
        {
            // Handled here, not in the state: they replace the current state.
            if (event.key.code == sf::Keyboard::R)
            {
                config_.seed = randomDevice_();
                newGame();
                continue;
            }
            if (event.key.code >= sf::Keyboard::F1 && event.key.code <= sf::Keyboard::F4)
            {
                config_.biome = static_cast<Biome>(event.key.code - sf::Keyboard::F1);
                newGame();
                continue;
            }
        }
        stateMachine_.handleEvent(event);
    }
}

}  // namespace defender
