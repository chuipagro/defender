#pragma once

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Event.hpp>

namespace defender
{

class MenuState
{
public:
    void handleEvent(const sf::Event& event);
    void update(float dt);
    void render(sf::RenderTarget& target);
};

}  // namespace defender
