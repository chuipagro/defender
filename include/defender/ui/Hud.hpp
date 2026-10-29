#pragma once

#include "defender/core/Player.hpp"
#include "defender/managers/LevelManager.hpp"

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>

#include <array>
#include <optional>

namespace defender
{

enum class HudButton
{
    NextWave,
    Speed,
    SkillTree,
};

/// What the HUD shows, gathered by PlayState each frame.
struct HudInfo
{
    const Player& player;
    int wave = 0;
    int totalWaves = LEVEL_WAVES;
    int remainingEnemies = 0;
    LevelManager::Phase phase = LevelManager::Phase::Break;
    float breakRemaining = 0.f;
    bool fastForward = false;
    sf::Vector2f mouse;
};

/// Top bar: health, shield, gold, skill points, wave, enemies left, and
/// buttons (next wave, speed, skill tree).
class Hud
{
public:
    explicit Hud(float width) : width_(width) {}

    void render(sf::RenderTarget& target, const HudInfo& info);
    /// Button under `point` (screen coordinates), if any.
    [[nodiscard]] std::optional<HudButton> buttonAt(sf::Vector2f point) const;

    static constexpr float HEIGHT = 40.f;

private:
    float width_;
    std::array<sf::FloatRect, 3> buttons_{};  ///< Indexed by HudButton
    std::array<bool, 3> visible_{};
};

}  // namespace defender
