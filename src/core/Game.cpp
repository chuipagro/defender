#include "defender/core/Game.hpp"

#include "defender/managers/LevelManager.hpp"

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/View.hpp>

#include <algorithm>
#include <stdexcept>

namespace defender
{

namespace
{

constexpr float MAX_DT = 0.1f;

/// Scale and top-left corner of the canvas inside the window (keeps the aspect ratio).
struct Letterbox
{
    float scale = 1.f;
    sf::Vector2f offset;
};

Letterbox letterbox(sf::Vector2u canvas, sf::Vector2u window)
{
    const float scale = std::min(static_cast<float>(window.x) / static_cast<float>(canvas.x),
                                 static_cast<float>(window.y) / static_cast<float>(canvas.y));
    return {scale, {(static_cast<float>(window.x) - (static_cast<float>(canvas.x) * scale)) / 2.f,
                    (static_cast<float>(window.y) - (static_cast<float>(canvas.y) * scale)) / 2.f}};
}

}  // namespace

Game::Game()
    : canvasSize_(PlayState::screenSize(config_)),
      window_(sf::VideoMode(canvasSize_.x, canvasSize_.y), "Defender", sf::Style::Default)
{
    if (!canvas_.create(canvasSize_.x, canvasSize_.y))
    {
        throw std::runtime_error("cannot create the render canvas");
    }
    window_.setFramerateLimit(60);
    config_.seed = randomDevice_();
    config_.biome = static_cast<Biome>(randomDevice_() % BIOME_COUNT);
    newGame();
}

void Game::newGame()
{
    stateMachine_.change<PlayState>(config_, level_, skills_);
    clock_.restart();
}

void Game::nextLevel()
{
    level_ = std::min(level_ + 1, LEVEL_COUNT);
    config_.seed = randomDevice_();
    config_.biome = static_cast<Biome>(randomDevice_() % BIOME_COUNT);
    newGame();
}

void Game::run()
{
    while (window_.isOpen())
    {
        pollEvents();
        stateMachine_.update(std::min(clock_.restart().asSeconds(), MAX_DT));
        canvas_.clear();
        stateMachine_.render(canvas_);
        canvas_.display();
        present();

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

void Game::present()
{
    const Letterbox box = letterbox(canvasSize_, window_.getSize());
    sf::Sprite sprite(canvas_.getTexture());
    sprite.setScale(box.scale, box.scale);
    sprite.setPosition(box.offset);
    window_.clear();
    window_.draw(sprite);
    window_.display();
}

void Game::toCanvas(sf::Event& event) const
{
    const Letterbox box = letterbox(canvasSize_, window_.getSize());
    if (box.scale <= 0.f)
    {
        return;
    }
    auto convert = [&box](int& x, int& y)
    {
        x = static_cast<int>((static_cast<float>(x) - box.offset.x) / box.scale);
        y = static_cast<int>((static_cast<float>(y) - box.offset.y) / box.scale);
    };
    switch (event.type)
    {
        case sf::Event::MouseMoved:
            convert(event.mouseMove.x, event.mouseMove.y);
            break;
        case sf::Event::MouseButtonPressed:
        case sf::Event::MouseButtonReleased:
            convert(event.mouseButton.x, event.mouseButton.y);
            break;
        case sf::Event::MouseWheelScrolled:
            convert(event.mouseWheelScroll.x, event.mouseWheelScroll.y);
            break;
        default:
            break;
    }
}

void Game::pollEvents()
{
    sf::Event event{};
    while (window_.pollEvent(event))
    {
        if (event.type == sf::Event::Closed)
        {
            window_.close();
            return;
        }
        if (event.type == sf::Event::Resized)
        {
            window_.setView(sf::View(sf::FloatRect(0.f, 0.f, static_cast<float>(event.size.width),
                                                   static_cast<float>(event.size.height))));
            continue;
        }
        toCanvas(event);

        const PlayState* play = stateMachine_.get<PlayState>();
        if (play != nullptr && play->isMenuOpen())
        {
            stateMachine_.handleEvent(event);
            continue;
        }
        const bool key = event.type == sf::Event::KeyPressed;
        const bool escape = key && event.key.code == sf::Keyboard::Escape;
        if (escape && play != nullptr && play->capturesEscape())
        {
            stateMachine_.handleEvent(event);
            continue;
        }
        if (escape)
        {
            window_.close();
            return;
        }
        if (key)
        {
            if (event.key.code == sf::Keyboard::Enter && play != nullptr && play->isVictory())
            {
                nextLevel();
                continue;
            }
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
