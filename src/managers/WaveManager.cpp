#include "defender/managers/WaveManager.hpp"

#include "defender/core/Events.hpp"

#include <algorithm>
#include <utility>

namespace defender
{

WaveManager::WaveManager(EntityManager& entityManager, EnemyFactory& factory,
                         std::span<const sf::Vector2f> path)
    : entityManager_(entityManager),
      factory_(factory),
      path_(path),
      killedHandler_(EventBus::instance().subscribe<EnemyKilled>(
          [this](const EnemyKilled& event)
          {
              // Only our enemies (another game may share the global bus).
              if (event.enemy != nullptr && event.enemy->path().data() == path_.data())
              {
                  childrenOfCurrentParent_ = 0;
                  event.enemy->runDeathHooks(*this);
              }
          }))
{
}

WaveManager::~WaveManager()
{
    EventBus::instance().unsubscribe(killedHandler_);
}

void WaveManager::startWave(int number, WaveData data)
{
    currentWave_ = number;
    total_ = 0;
    for (const SpawnEntry& entry : data.enemies)
    {
        total_ += entry.count;
    }
    spawned_ = 0;
    nextEntry_ = 0;
    spawnedInEntry_ = 0;
    spawnTimer_ = data.enemies.empty() ? 0.f : data.enemies.front().delay;
    waves_.clear();
    waves_.push_back(std::move(data));
    EventBus::instance().emit(WaveStarted{currentWave_});
}

void WaveManager::update(float dt)
{
    spawnPendingChildren();
    if (waves_.empty())
    {
        return;
    }
    const WaveData& wave = waves_.back();
    spawnTimer_ -= dt;
    while (spawnTimer_ <= 0.f && nextEntry_ < wave.enemies.size())
    {
        const SpawnEntry& entry = wave.enemies[nextEntry_];
        entityManager_.spawnEnemy(EnemyFactory::create(entityManager_.nextId(), entry.spec, path_));
        ++spawned_;
        spawnTimer_ += wave.spawnInterval;
        if (++spawnedInEntry_ >= entry.count)
        {
            ++nextEntry_;
            spawnedInEntry_ = 0;
            if (nextEntry_ < wave.enemies.size())
            {
                spawnTimer_ += wave.enemies[nextEntry_].delay;
            }
        }
    }
}

EnemySpec WaveManager::rollSpec()
{
    return factory_.roll();
}

void WaveManager::spawnChild(const Enemy& parent, EnemySpec spec)
{
    // Queued: we may be inside EntityManager::updateAll, which iterates the enemies.
    pendingChildren_.push_back({spec, parent.pathIndex(), parent.getPosition(),
                                parent.generation() + 1,
                                static_cast<float>(childrenOfCurrentParent_) * CHILD_SPACING});
    ++childrenOfCurrentParent_;
}

void WaveManager::spawnPendingChildren()
{
    for (const PendingChild& child : pendingChildren_)
    {
        auto enemy =
            EnemyFactory::create(entityManager_.nextId(), child.spec, path_, child.generation);
        enemy->placeOnPath(child.pathIndex, child.position);
        enemy->moveBack(child.backOffset);
        entityManager_.spawnEnemy(std::move(enemy));
    }
    pendingChildren_.clear();
}

const WaveData* WaveManager::currentWaveData() const
{
    return waves_.empty() ? nullptr : &waves_.back();
}

bool WaveManager::allSpawned() const
{
    return !waves_.empty() && nextEntry_ >= waves_.back().enemies.size() &&
           pendingChildren_.empty();
}

bool WaveManager::isWaveComplete() const
{
    const auto enemies = entityManager_.enemies();
    return allSpawned() &&
           std::none_of(enemies.begin(), enemies.end(), [](const auto& e) { return e->isAlive(); });
}

int WaveManager::remainingEnemies() const
{
    const auto enemies = entityManager_.enemies();
    const auto alive = std::count_if(enemies.begin(), enemies.end(),
                                     [](const auto& e) { return e->isAlive(); });
    return (total_ - spawned_) + static_cast<int>(alive) +
           static_cast<int>(pendingChildren_.size());
}

}  // namespace defender
