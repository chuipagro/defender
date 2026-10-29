#pragma once

#include "defender/core/EventBus.hpp"
#include "defender/core/Player.hpp"
#include "defender/entities/EnemyFactory.hpp"
#include "defender/managers/EntityManager.hpp"
#include "defender/managers/LevelManager.hpp"
#include "defender/managers/WaveManager.hpp"
#include "defender/map/MapGenerator.hpp"
#include "defender/map/MapRenderer.hpp"
#include "defender/skills/SkillTree.hpp"
#include "defender/ui/BuildBar.hpp"
#include "defender/ui/Hud.hpp"
#include "defender/ui/SkillTreeMenu.hpp"
#include "defender/ui/TowerPanel.hpp"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>

#include <optional>
#include <random>
#include <string>
#include <vector>

namespace defender
{

/// A running game on one map: the level's waves, the player, towers, the HUD,
/// the build bar and the skill tree menu.
///
/// Screen layout, top to bottom: HUD (Hud::HEIGHT), map, build bar (BuildBar::HEIGHT).
/// Not copyable nor movable: managers keep references to members.
class PlayState
{
public:
    explicit PlayState(const MapGeneratorConfig& config);

    PlayState(const PlayState&) = delete;
    PlayState& operator=(const PlayState&) = delete;
    PlayState(PlayState&&) = delete;
    PlayState& operator=(PlayState&&) = delete;
    ~PlayState();

    void handleEvent(const sf::Event& event);
    void update(float dt);
    void render(sf::RenderTarget& target);

    /// One-line summary for the window title.
    [[nodiscard]] std::string status() const;
    [[nodiscard]] const Map& map() const { return map_; }
    [[nodiscard]] const Player& player() const { return player_; }
    [[nodiscard]] bool isMenuOpen() const { return menuOpen_; }
    [[nodiscard]] bool isOver() const;
    /// True when Escape has something to cancel (menu, tower to place, selection).
    [[nodiscard]] bool capturesEscape() const;

    // ── Actions (also used by tests and tools) ──
    /// Chooses the tower type to place (nullopt = cancel).
    void selectBuild(std::optional<TowerType> type);
    /// Places the selected build type on `tile`. False if not possible.
    bool placeTower(GridPos tile);
    /// True if a tower of `type` can go on `tile` right now (tile, unlock, gold).
    [[nodiscard]] bool canPlace(TowerType type, GridPos tile) const;
    bool upgradeTower(GridPos tile);
    bool sellTower(GridPos tile);
    [[nodiscard]] SkillTree& skillTree() { return skillTree_; }
    [[nodiscard]] Player& player() { return player_; }

    /// Window size needed for a map of this config (HUD + map + build bar).
    [[nodiscard]] static sf::Vector2u screenSize(const MapGeneratorConfig& config);

    /// Speed-up applied with the Space key.
    static constexpr float FAST_FORWARD = 3.f;
    static constexpr float BANNER_TIME = 3.f;
    static constexpr float DROP_TIME = 1.f;  ///< Flight of a dropped material

private:
    /// A dropped material flying from an enemy to its counter (visual only:
    /// the player already has it).
    struct Drop
    {
        Element element;
        sf::Vector2f from;  ///< Screen coordinates
        float time = 0.f;
    };

    void clickWorld(sf::Vector2f world);
    void select(std::optional<GridPos> tile);
    void onEnemyKilled(const Enemy& enemy, int reward);
    [[nodiscard]] std::optional<GridPos> tileAt(sf::Vector2f world) const;
    [[nodiscard]] sf::Vector2f worldToScreen(sf::Vector2f world) const;
    void showBanner(std::string text);
    void renderWorldOverlay(sf::RenderTarget& target) const;
    void renderDrops(sf::RenderTarget& target) const;
    void renderBanner(sf::RenderTarget& target) const;
    void renderEndScreen(sf::RenderTarget& target) const;

    Map map_;
    std::vector<sf::Vector2f> path_;  ///< Map path in pixels, from off-screen to off-screen
    sf::Vector2f screen_;
    EntityManager entityManager_;
    EnemyFactory enemyFactory_;
    WaveManager waveManager_;
    LevelManager levelManager_;
    Player player_;
    SkillTree skillTree_;
    MapRenderer mapRenderer_;
    Hud hud_;
    BuildBar buildBar_;
    TowerPanel towerPanel_;
    SkillTreeMenu skillMenu_;
    std::vector<EventBus::HandlerId> handlers_;
    std::mt19937 lootRng_;
    std::vector<Drop> drops_;
    std::optional<TowerType> build_;      ///< Tower type being placed
    std::optional<GridPos> selected_;     ///< Selected tower
    int kills_ = 0;
    float timeScale_ = 1.f;
    bool menuOpen_ = false;
    sf::Vector2f mouse_;
    std::string banner_;
    float bannerTimer_ = 0.f;
};

}  // namespace defender
