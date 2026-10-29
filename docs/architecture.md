# Architecture

## C++20 Principles Applied

- **Concepts** — template constraints (`Updatable`, `Renderable`, `Damageable`)
- **`std::span`** — pass ranges without copies
- **RAII** everywhere — no raw `new`/`delete`, only `unique_ptr` / `shared_ptr`
- **Composition > inheritance** — `Entity` is a lightweight base; behaviors live in components
- **`std::variant` + `std::visit`** — State Machine without pure virtuals
- **Logic / render separation** — `defender_lib` has no display dependency

---

## Overview

```mermaid
graph TD
    main["main.cpp\n(entry point)"]
    Game["Game\n(orchestrateur)"]
    SM["StateMachine\n(std::variant)"]
    MenuState["MenuState"]
    PlayState["PlayState"]
    PauseState["PauseState"]
    GameOverState["GameOverState"]

    main --> Game
    Game --> SM
    SM --> MenuState
    SM --> PlayState
    SM --> PauseState
    SM --> GameOverState
```

---

## Game State — PlayState

```mermaid
graph LR
    PS["PlayState"]
    WM["WaveManager"]
    Map["Map"]
    EM["EntityManager"]
    RM["ResourceManager\n(singleton RAII)"]
    EB["EventBus"]

    PS --> WM
    PS --> Map
    PS --> EM
    PS --> EB
    EM --> RM
    WM --> EM
```

---

## Entity Hierarchy

```mermaid
classDiagram
    class Entity {
        <<abstract>>
        +update(dt: float) = 0
        +render(RenderTarget&) = 0
        +isAlive() bool
        #alive_ : bool
        #id_ : EntityId
    }

    class Tower {
        +update(dt: float)
        +render(RenderTarget&)
        +shoot(enemies: span~Enemy~)
        -range_ : float
        -fireRate_ : float
        -cooldown_ : float
        -stats_ : TowerStats
    }

    class Enemy {
        +update(dt: float)
        +render(RenderTarget&)
        +takeDamage(amount: float)
        +getPosition() Vector2f
        -type_ : EnemyType
        -element_ : Element
        -path_ : span~Vector2f~
        -pathIndex_ : size_t
        -health_ : float
        -speed_ : float
    }

    class Projectile {
        +update(dt: float)
        +render(RenderTarget&)
        -target_ : weak_ptr~Enemy~
        -damage_ : float
        -speed_ : float
    }

    Entity <|-- Tower
    Entity <|-- Enemy
    Entity <|-- Projectile
```

---

## C++20 Concepts

```mermaid
classDiagram
    class Updatable {
        <<concept>>
        +update(dt: float)
    }
    class Renderable {
        <<concept>>
        +render(target: RenderTarget&)
    }
    class Damageable {
        <<concept>>
        +takeDamage(float)
        +isAlive() bool
    }

    Updatable <.. Tower : satisfies
    Updatable <.. Enemy : satisfies
    Renderable <.. Tower : satisfies
    Renderable <.. Enemy : satisfies
    Damageable <.. Enemy : satisfies
```

---

## EntityManager

```mermaid
classDiagram
    class EntityManager {
        +addTower(pos: Vector2f, type: TowerType) Tower&
        +spawnEnemy(enemy: unique_ptr~Enemy~) Enemy&
        +nextId() EntityId
        +updateAll(dt: float)
        +renderAll(target: RenderTarget&)
        +removeDeadEntities()
        -towers_ : vector~unique_ptr~Tower~~
        -enemies_ : vector~shared_ptr~Enemy~~
        -projectiles_ : vector~unique_ptr~Projectile~~
    }
```

Enemies are `shared_ptr` so that `Projectile` can follow its target with a
`weak_ptr` and notice when it died.

---

## EnemyFactory

Creates enemies at random, following **probability matrices that depend on the
map's biome**. The species and the element are drawn independently:

`P(type, element | biome) = P(type | biome) × P(element | biome)`

```mermaid
classDiagram
    class EnemyFactory {
        +EnemyFactory(biome: Biome, seed: uint32, table: SpawnTable)
        +roll(rank: Rank) EnemySpec
        +create(id: EntityId, path: span~Vector2f~) unique_ptr~Enemy~
        +create(id: EntityId, spec: EnemySpec, path: span~Vector2f~, generation: int)$ unique_ptr~Enemy~
        +probability(element: Element) double
        +probability(type: EnemyType) double
        -weights_ : SpawnWeights
        -rng_ : mt19937
    }

    class EnemySpec {
        +type : EnemyType
        +element : Element
        +rank : Rank
    }

    class SpawnWeights {
        +elements : array~double, ELEMENT_COUNT~
        +types : array~double, ENEMY_TYPE_COUNT~
    }

    EnemyFactory --> SpawnWeights : one row per biome
    EnemyFactory ..> EnemySpec : rolls
    WaveManager --> EnemyFactory : builds waves
```

Default matrices (`src/entities/EnemyFactory.cpp`, in %):

| Biome   | Water | Fire | Earth | Wind |
|---------|------:|-----:|------:|-----:|
| Desert  |    10 |   35 |    35 |   20 |
| Ice     |    45 |    5 |    15 |   35 |
| Volcano |     5 |   50 |    35 |   10 |
| Forest  |    25 |   10 |    40 |   25 |

| Biome   | Octopus | Orangutan | Cat | Dog | Duck | Frog |
|---------|--------:|----------:|----:|----:|-----:|-----:|
| Desert  |       5 |        20 |  30 |  30 |    5 |   10 |
| Ice     |      20 |        10 |  15 |  20 |   25 |   10 |
| Volcano |      10 |        25 |  25 |  20 |   10 |   10 |
| Forest  |      10 |        30 |  15 |  15 |   15 |   15 |

!!! note "Never 0%"
    Every weight must be > 0. A `static_assert` rejects the default table at
    compile time otherwise, and the constructor throws `std::invalid_argument`
    for a custom table. Adding an element (ice, magma, lightning...) leaves a
    0 in every row until its column is filled, so it cannot be forgotten.

---

## Enemy definitions (stats + hooks)

An enemy is **species + element + rank**. All three are defined in C++, in
`src/entities/EnemyDefinitions.cpp`, and each one brings stats **and functions**.

```
final stat = base(species) × mult(element) × mult(rank) + add(element) + add(rank)
```

| Layer   | Struct               | Gives                                     |
|---------|----------------------|-------------------------------------------|
| Species | `SpeciesDefinition`  | base stats (`baseStats(hp, speed, gold, damage)`) + hooks |
| Element | `ModifierDefinition` | multipliers (`times`) + bonuses (`plus`) + hooks |
| Rank    | `ModifierDefinition` | multipliers (`times`) + bonuses (`plus`) + hooks |

Stats (`Stats.hpp`): `Health`, `Speed`, `Reward`, `Damage`, `Scale`.

### Hooks

`EnemyHooks` holds optional `std::function`s. The hooks of the three layers all
run, species first, then element, then rank:

| Hook          | Signature                              | When                          |
|---------------|----------------------------------------|-------------------------------|
| `onSpawn`     | `void(Enemy&)`                         | once, at creation             |
| `onUpdate`    | `void(Enemy&, float dt)`               | every frame while alive       |
| `onHit`       | `float(Enemy&, float damage)`          | before damage, returns the damage taken (chained) |
| `onDeath`     | `void(Enemy&, EnemySpawner&)`          | killed by damage              |
| `onReachBase` | `void(Enemy&)`                         | reaches the base              |

A hook can change the enemy (`enemy.stats()[Stat::Speed] *= 1.3f`, `enemy.heal(5)`),
and remember things in `enemy.memory(0..3)`. Ready-made hooks: `armor(n)`,
`regeneration(hpPerSecond)`, `splitInto(count, rank)`.

```cpp
{.description = "Dog: runs 30% faster once below half health.",
 .base = baseStats(80.f, 125.f, 8.f, 1.f),
 .hooks = {.onUpdate = [](Enemy& dog, float) {
     float& enraged = dog.memory(0);
     if (enraged == 0.f && dog.health() < dog.maxHealth() * 0.5f) {
         enraged = 1.f;
         dog.stats()[Stat::Speed] *= 1.3f;
     }
 }}},
```

### Current definitions

| Layer      | Effect |
|------------|--------|
| Orangutan  | slow tank, armor 2 |
| Cat        | fast, survives its first deadly hit with 1 hp |
| Dog        | +30% speed below half health |
| Normal     | natural colors, base stats |
| Water      | ×1.5 health/gold/damage, +5 gold, leaves a normal enemy behind when killed |
| Fire       | ×1.4 health/gold/damage, ×1.2 speed, rises from its ashes once (40% health) |
| Earth      | ×1.5 health/gold/damage, ×0.85 speed, armor 3 |
| Wind       | ×1.3 health/gold/damage, ×1.35 speed, dodges one hit out of 5 |
| Mini-boss  | ×4 health, ×3 gold/damage, ×0.9 speed, size ×1.4 |
| Boss       | ×12 health, ×10 gold, ×5 damage, ×0.75 speed, armor 3, size ×1.9 |
| Super boss | ×25 health, ×20 gold, ×10 damage, ×0.6 speed, armor 5, size ×2.4, splits into 3 mini-bosses |

Armor removes `n` from each hit but always lets 20% through; armor hooks stack.
Each layer has its own memory slot (`Enemy::MEMORY_SPECIES`, `MEMORY_ELEMENT`,
`MEMORY_RANK`) so their hooks never overwrite each other.

### Super boss split

```mermaid
sequenceDiagram
    participant E as Enemy (super boss)
    participant B as EventBus
    participant W as WaveManager (EnemySpawner)
    participant M as EntityManager
    E->>B: EnemyKilled{enemy, reward}
    B->>W: handler
    W->>E: runDeathHooks(*this)
    E->>W: splitInto → rollSpec() ×3, spawnChild(parent, spec)
    Note over W: queued (we may be inside updateAll)
    W->>M: next update(): spawnEnemy(child), placed behind the parent
```

Children keep the element, roll their species with the biome matrices, and get
`generation + 1`; splitting stops at `MAX_SPAWN_GENERATION` (3). A wave is not
complete while children are waiting to spawn.

---

## LevelManager (difficulty)

A level is 150 waves played one after the other:
`Break (8 s, N / play button to skip) → Running → WaveCompleted → Break → ...`
until wave 150 is cleared (`Phase::Victory`).

Each wave gets a `WavePlan` from `planForWave(wave)`, built from curves of
`{wave, value}` points (linear in between) at the top of `LevelManager.cpp`:

| Wave | Enemies | Mini-boss | Boss | Super boss | Elemental factor |
|-----:|--------:|----------:|-----:|-----------:|-----------------:|
|    1 |       5 |         – |    – |          – |             0.2 |
|   10 |      12 |        3% |    – |          – |             0.3 |
|   25 |      30 |      4.9% |   1% |          – |            0.45 |
|   50 |      75 |        8% |   2% |       0.5% |             0.8 |
|  100 |     200 |       15% |   5% |         2% |             1.5 |
|  150 |     200 |       25% |  10% |         5% |               3 |

- A rank always shows up at least once on its unlock wave (10, 25, 50).
- Counts per rank are `count × share`, rounded at random; the strongest enemy closes the wave.
- The elemental factor multiplies the weight of every element but Normal in the
  biome matrix (1 = half of the enemies are elemental).
- Cleared wave: `20 + 5 × wave` gold, 1 skill point (3 every 10th wave).

---

## Towers

Defined in C++ in `src/entities/TowerDefinitions.cpp` (`TowerDefinition`: price,
damage, range, fire rate, projectile sprite, splash, upgrade costs, `onHit` effect).

| Tower  | Gold | Projectile | Effect | Upgrades (level 2 / 3) |
|--------|-----:|------------|--------|------------------------|
| Normal |   50 | arrow      | fast shots | 60 / 120 gold |
| Water  |   80 | bubble     | slows to 60% speed for 1.5 s | 90 g + 3 / 180 g + 8 water drops |
| Fire   |   90 | fireball   | burns 6 hp/s for 3 s (ignores armor) | 100 g + 3 / 200 g + 8 embers |
| Earth  |  100 | rock       | splash damage (40 px) | 110 g + 3 / 220 g + 8 stones |
| Wind   |   85 | gust       | pushes back 20 px (less for big enemies) | 95 g + 3 / 190 g + 8 feathers |

Each level: damage +50%, range +10%, fire rate +15%, effects +25%. Skill tree
bonuses (`TowerUpgrades`) multiply on top. Selling gives back 60% of the gold spent.

Shooting (`EntityManager::updateAll`): enemies move, each ready tower picks the
alive enemy in range with the highest `progress()` (furthest along the path) and
fires a `Projectile` carrying a `Shot`. Projectiles home in on their target; on
arrival `resolveImpact` applies the damage (target or splash) and the tower's `onHit`.

**Materials**: elemental enemies drop materials of their element when killed
(`materialDrop`: 35% chance of 1 for a normal rank, 2 / 5 / 10 for mini-boss /
boss / super boss). They fly to their counter in the build bar.

---

## Player

Gold (enemy rewards + end-of-wave bonus), elemental materials, health (50), shield (0 until unlocked,
absorbs damage first, refilled at the end of each wave), skill points.

## Skill tree

Nodes are defined in C++ in `src/skills/SkillDefinitions.cpp`: cost in skill
points, prerequisites, position in the menu, icon or tower sprite, and an
`effect(Player&, TowerUpgrades&)` run once when bought.

- **Towers**: normal tower (owned from the start) → water, fire, earth, wind towers,
  each with *Puissance* (+25% damage) and *Cadence* (+20% fire rate).
- **Base**: max health, shield, reinforced shield, +25% wave gold.

`TowerUpgrades` (unlocked towers + damage/range/fire-rate multipliers) is what
the tower system will read.

## UI

- `ui/Ui.hpp`: pixel font (`sprites/ui/font.png`, ASCII + é è ê à â ç ù û ô î),
  icons (`sprites/ui/icons.png`, `enum class Icon`), panels, lines.
- `Hud`: top bar (health, shield, gold, points, wave, enemies left, next-wave /
  speed / skill-tree buttons).
- `SkillTreeMenu`: full-screen tree (T or the tree button), pauses the game.
- `BuildBar`: bottom bar, tower buttons (keys 1-5) and material counters.
- `TowerPanel`: next to the selected tower: level, stats, upgrade (U) and sell.

Controls: 1-5 / build bar = choose a tower, left click on a slab = place (stays in
build mode), right click / Escape = cancel, click a tower = select it.

---

## WaveManager

```mermaid
classDiagram
    class WaveManager {
        +update(dt: float)
        +isWaveComplete() bool
        +startWave(number: int, data: WaveData)
        +remainingEnemies() int
        +getCurrentWave() int
        +rollSpec() EnemySpec
        +spawnChild(parent: Enemy, spec: EnemySpec)
        -waves_ : vector~WaveData~
        -pendingChildren_ : vector~PendingChild~
        -spawnTimer_ : float
        -entityManager_ : EntityManager&
    }

    class WaveData {
        +enemies : vector~SpawnEntry~
        +spawnInterval : float
    }

    class SpawnEntry {
        +spec : EnemySpec
        +count : int
        +delay : float
    }

    WaveManager --> WaveData
    WaveData --> SpawnEntry
    EnemySpawner <|-- WaveManager
```

---

## EventBus (Decoupling)

```mermaid
classDiagram
    class EventBus {
        <<singleton>>
        +subscribe~T~(handler: function) HandlerId
        +unsubscribe(HandlerId)
        +emit~T~(event: T)
        -handlers_ : unordered_map
    }

    class EnemyReachedBase { +damage: float }
    class EnemyKilled { +reward: int }
    class TowerPlaced { +pos: Vector2f }
    class WaveStarted { +waveNumber: int }

    EventBus ..> EnemyReachedBase : emits
    EventBus ..> EnemyKilled : emits
    EventBus ..> TowerPlaced : emits
    EventBus ..> WaveStarted : emits
```

---

## ResourceManager

```mermaid
classDiagram
    class ResourceManager {
        <<RAII singleton>>
        +getTexture(id: TextureId) Texture&
        +getFont(id: FontId) Font&
        +getSoundBuffer(id: SoundId) SoundBuffer&
        -textures_ : unordered_map~TextureId, Texture~
        -fonts_ : unordered_map~FontId, Font~
        -sounds_ : unordered_map~SoundId, SoundBuffer~
        -ResourceManager()
    }
```

---

## Game Loop

```mermaid
sequenceDiagram
    participant Main
    participant Game
    participant StateMachine
    participant PlayState
    participant EntityManager

    Main->>Game: run()
    loop 60 fps
        Game->>Game: pollEvents()
        Game->>StateMachine: handleEvent(event)
        Game->>StateMachine: update(dt)
        StateMachine->>PlayState: update(dt)
        PlayState->>EntityManager: updateAll(dt)
        PlayState->>EntityManager: removeDeadEntities()
        Game->>StateMachine: render(window)
        StateMachine->>PlayState: render(window)
        PlayState->>EntityManager: renderAll(window)
    end
```

---

## File Structure

Headers live in `include/defender/<folder>/`, sources in `src/<folder>/`.

```
src/
├── core/
│   ├── Game.hpp / Game.cpp
│   ├── StateMachine.hpp          ← std::variant<MenuState, PlayState, ...>
│   ├── Player.hpp / .cpp         ← gold, health, shield, skill points
│   ├── EventBus.hpp              ← template header-only
│   ├── Events.hpp                ← EnemyKilled, WaveStarted...
│   └── Concepts.hpp              ← Updatable, Renderable, Damageable
├── states/
│   ├── MenuState.hpp / .cpp
│   ├── PlayState.hpp / .cpp
│   ├── PauseState.hpp / .cpp
│   └── GameOverState.hpp / .cpp
├── entities/
│   ├── Types.hpp                 ← Element, EnemyType, Rank, EnemySpec
│   ├── Stats.hpp                 ← Stat, EnemyStats
│   ├── EnemyDefinitions.hpp / .cpp ← species/element/rank stats + hooks, loot
│   ├── TowerDefinitions.hpp / .cpp ← tower stats, levels, costs, effects
│   ├── Entity.hpp                ← abstract base
│   ├── Tower.hpp / .cpp
│   ├── Enemy.hpp / .cpp
│   ├── EnemyFactory.hpp / .cpp   ← biome probability matrices
│   └── Projectile.hpp / .cpp
├── managers/
│   ├── EntityManager.hpp / .cpp
│   ├── LevelManager.hpp / .cpp   ← 150 waves, difficulty curves
│   ├── WaveManager.hpp / .cpp
│   └── ResourceManager.hpp / .cpp
├── skills/
│   ├── SkillTree.hpp / .cpp      ← TowerUpgrades, SkillNode, SkillTree
│   └── SkillDefinitions.cpp      ← the nodes of the tree
├── ui/
│   ├── Icon.hpp                  ← enum of icons.png
│   ├── Ui.hpp / .cpp             ← pixel font, icons, panels
│   ├── Hud.hpp / .cpp
│   ├── BuildBar.hpp / .cpp
│   ├── TowerPanel.hpp / .cpp
│   └── SkillTreeMenu.hpp / .cpp
├── map/
│   ├── Tile.hpp                  ← TileType, Biome, GridPos
│   ├── Map.hpp / .cpp
│   ├── Maze.hpp / .cpp           ← perfect maze (recursive backtracker)
│   ├── MapGenerator.hpp / .cpp
│   └── MapRenderer.hpp / .cpp
└── defender_lib.cpp              ← CMake stub
```
