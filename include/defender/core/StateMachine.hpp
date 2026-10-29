#pragma once

#include "defender/states/GameOverState.hpp"
#include "defender/states/MenuState.hpp"
#include "defender/states/PauseState.hpp"
#include "defender/states/PlayState.hpp"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Event.hpp>

#include <utility>
#include <variant>

namespace defender
{

using State = std::variant<MenuState, PlayState, PauseState, GameOverState>;

/// Holds the current state and forwards the game loop to it with std::visit.
class StateMachine
{
public:
    /// Replaces the current state, built in place.
    template <typename S, typename... Args>
    void change(Args&&... args)
    {
        state_.template emplace<S>(std::forward<Args>(args)...);
    }

    void handleEvent(const sf::Event& event)
    {
        std::visit([&event](auto& state) { state.handleEvent(event); }, state_);
    }

    void update(float dt)
    {
        std::visit([dt](auto& state) { state.update(dt); }, state_);
    }

    void render(sf::RenderTarget& target)
    {
        std::visit([&target](auto& state) { state.render(target); }, state_);
    }

    template <typename S>
    [[nodiscard]] bool is() const
    {
        return std::holds_alternative<S>(state_);
    }

    /// The current state if it is an S, nullptr otherwise.
    template <typename S>
    [[nodiscard]] S* get()
    {
        return std::get_if<S>(&state_);
    }

private:
    State state_;  ///< Starts on MenuState
};

}  // namespace defender
