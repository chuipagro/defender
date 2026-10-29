#pragma once

#include <SFML/System/Vector2.hpp>

namespace defender
{

class Enemy;

struct EnemyReachedBase
{
    float damage = 0.f;
};

/// Emitted when an enemy dies from damage. `enemy` is only valid while the
/// event is being handled (it is removed from the game afterwards).
struct EnemyKilled
{
    Enemy* enemy = nullptr;
    int reward = 0;
};

struct TowerPlaced
{
    sf::Vector2f pos;
};

struct WaveStarted
{
    int waveNumber = 0;
};

}  // namespace defender
