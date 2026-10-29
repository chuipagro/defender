#include "defender/core/Player.hpp"

#include <algorithm>
#include <cmath>

namespace defender
{

void Player::earn(int amount)
{
    gold_ += std::max(amount, 0);
}

bool Player::spend(int amount)
{
    if (amount < 0 || amount > gold_)
    {
        return false;
    }
    gold_ -= amount;
    return true;
}

void Player::addMaterials(Element element, int amount)
{
    if (element != Element::Normal)
    {
        materials_[static_cast<std::size_t>(element)] += std::max(amount, 0);
    }
}

bool Player::canAfford(const Cost& cost) const
{
    const bool materialsOk = cost.material == Element::Normal || cost.materials <= 0 ||
                             materials(cost.material) >= cost.materials;
    return gold_ >= cost.gold && materialsOk;
}

bool Player::pay(const Cost& cost)
{
    if (!canAfford(cost))
    {
        return false;
    }
    gold_ -= cost.gold;
    if (cost.material != Element::Normal && cost.materials > 0)
    {
        materials_[static_cast<std::size_t>(cost.material)] -= cost.materials;
    }
    return true;
}

void Player::takeDamage(float amount)
{
    if (amount <= 0.f)
    {
        return;
    }
    const float absorbed = std::min(shield_, amount);
    shield_ -= absorbed;
    health_ = std::max(health_ - (amount - absorbed), 0.f);
}

void Player::heal(float amount)
{
    if (!isDead())
    {
        health_ = std::min(health_ + std::max(amount, 0.f), maxHealth_);
    }
}

void Player::addMaxHealth(float amount)
{
    maxHealth_ += amount;
    heal(amount);
}

void Player::addMaxShield(float amount)
{
    maxShield_ += amount;
    shield_ = std::min(shield_ + amount, maxShield_);
}

bool Player::spendSkillPoints(int amount)
{
    if (amount < 0 || amount > skillPoints_)
    {
        return false;
    }
    skillPoints_ -= amount;
    return true;
}

int Player::completeWave(int bonusGold, int skillPoints)
{
    const auto gold = static_cast<int>(std::lround(static_cast<float>(bonusGold) *
                                                   waveGoldMultiplier_));
    earn(gold);
    addSkillPoints(skillPoints);
    refillShield();
    return gold;
}

}  // namespace defender
