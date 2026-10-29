#pragma once

#include <SFML/Graphics/RenderTarget.hpp>

#include <concepts>

namespace defender
{

template <typename T>
concept Updatable = requires(T& object, float dt) {
    { object.update(dt) } -> std::same_as<void>;
};

template <typename T>
concept Renderable = requires(const T& object, sf::RenderTarget& target) {
    { object.render(target) } -> std::same_as<void>;
};

template <typename T>
concept Damageable = requires(T& object, const T& constObject, float amount) {
    object.takeDamage(amount);  // may return the damage actually taken
    { constObject.isAlive() } -> std::convertible_to<bool>;
};

}  // namespace defender
