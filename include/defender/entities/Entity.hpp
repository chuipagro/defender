#pragma once

#include "defender/entities/Types.hpp"

#include <SFML/Graphics/RenderTarget.hpp>

namespace defender
{

/// Lightweight base of every game object.
class Entity
{
public:
    explicit Entity(EntityId id) : id_(id) {}
    virtual ~Entity() = default;

    Entity(const Entity&) = delete;
    Entity& operator=(const Entity&) = delete;
    Entity(Entity&&) = default;
    Entity& operator=(Entity&&) = default;

    virtual void update(float dt) = 0;
    virtual void render(sf::RenderTarget& target) const = 0;

    [[nodiscard]] bool isAlive() const { return alive_; }
    [[nodiscard]] EntityId id() const { return id_; }

protected:
    bool alive_ = true;
    EntityId id_;
};

}  // namespace defender
