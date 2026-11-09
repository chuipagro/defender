#pragma once

#include "defender/entities/TowerDefinitions.hpp"
#include "defender/entities/Types.hpp"

#include <array>
#include <cstddef>

namespace defender
{

class Player
{
public:
    Player() = default;

    // Gold
    [[nodiscard]] int gold() const { return gold_; }
    void earn(int amount);
    /// Pays `amount` if possible; returns false (and pays nothing) otherwise.
    bool spend(int amount);

    // Elemental materials (dropped by elemental enemies)
    [[nodiscard]] int materials(Element element) const
    {
        return materials_[static_cast<std::size_t>(element)];
    }
    void addMaterials(Element element, int amount);

    // Costs (gold + materials)
    [[nodiscard]] bool canAfford(const Cost& cost) const;
    /// Pays `cost` if possible; returns false (and pays nothing) otherwise.
    bool pay(const Cost& cost);

    // Health and shield
    [[nodiscard]] float health() const { return health_; }
    [[nodiscard]] float maxHealth() const { return maxHealth_; }
    [[nodiscard]] float shield() const { return shield_; }
    [[nodiscard]] float maxShield() const { return maxShield_; }
    [[nodiscard]] bool isDead() const { return health_ <= 0.f; }
    /// Shield absorbs first, the rest goes to health.
    void takeDamage(float amount);
    void heal(float amount);
    /// Raises max health and heals by the same amount.
    void addMaxHealth(float amount);
    /// Raises max shield and fills it by the same amount.
    void addMaxShield(float amount);
    void refillShield() { shield_ = maxShield_; }

    // Gold bonuses (from the skill tree)
    [[nodiscard]] float waveGoldMultiplier() const { return waveGoldMultiplier_; }
    void addWaveGoldBonus(float fraction) { waveGoldMultiplier_ += fraction; }
    [[nodiscard]] float killGoldMultiplier() const { return killGoldMultiplier_; }
    void addKillGoldBonus(float fraction) { killGoldMultiplier_ += fraction; }

    /// Earns a kill reward (with the multiplier). Returns the gold earned.
    int earnKill(int reward);
    /// Earns the wave bonus (with the multiplier) and refills the shield.
    /// Returns the gold earned.
    int completeWave(int bonusGold);

    static constexpr int STARTING_GOLD = 100;
    static constexpr float STARTING_HEALTH = 50.f;

private:
    int gold_ = STARTING_GOLD;
    std::array<int, ELEMENT_COUNT> materials_{};
    float health_ = STARTING_HEALTH;
    float maxHealth_ = STARTING_HEALTH;
    float shield_ = 0.f;
    float maxShield_ = 0.f;
    float waveGoldMultiplier_ = 1.f;
    float killGoldMultiplier_ = 1.f;
};

}  // namespace defender
