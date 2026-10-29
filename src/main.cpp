#include "defender/core/Game.hpp"

#include <exception>
#include <iostream>

int main()
{
    try
    {
        defender::Game game;
        game.run();
    }
    catch (const std::exception& error)
    {
        std::cerr << "defender: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
