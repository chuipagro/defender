#include <SFML/Graphics.hpp>
#include <iostream>

int main()
{
    sf::RenderWindow window(
        sf::VideoMode(800, 600),
        "Defender — Tower Defense",
        sf::Style::Default
    );
    window.setFramerateLimit(60);

    // Placeholder: a green square representing a tower
    sf::RectangleShape tower(sf::Vector2f(40.f, 40.f));
    tower.setFillColor(sf::Color(50, 200, 50));
    tower.setPosition(380.f, 280.f);

    while (window.isOpen()) {
        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (event.type == sf::Event::KeyPressed &&
                event.key.code == sf::Keyboard::Escape) {
                window.close();
            }
        }

        window.clear(sf::Color(20, 20, 30));
        window.draw(tower);
        window.display();
    }

    return 0;
}
