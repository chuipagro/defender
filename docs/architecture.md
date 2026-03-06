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
        +spawnEnemy(type: EnemyType, path: span~Vector2f~) Enemy&
        +updateAll(dt: float)
        +renderAll(target: RenderTarget&)
        +removeDeadEntities()
        -towers_ : vector~unique_ptr~Tower~~
        -enemies_ : vector~unique_ptr~Enemy~~
        -projectiles_ : vector~unique_ptr~Projectile~~
    }
```

---

## WaveManager

```mermaid
classDiagram
    class WaveManager {
        +update(dt: float)
        +startNextWave()
        +isWaveComplete() bool
        +getCurrentWave() int
        -waves_ : vector~WaveData~
        -spawnTimer_ : float
        -entityManager_ : EntityManager&
    }

    class WaveData {
        +enemies : vector~SpawnEntry~
        +spawnInterval : float
    }

    class SpawnEntry {
        +type : EnemyType
        +count : int
        +delay : float
    }

    WaveManager --> WaveData
    WaveData --> SpawnEntry
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

```
src/
├── core/
│   ├── Game.hpp / Game.cpp
│   ├── StateMachine.hpp          ← std::variant<MenuState, PlayState, ...>
│   └── EventBus.hpp              ← template header-only
├── states/
│   ├── MenuState.hpp / .cpp
│   ├── PlayState.hpp / .cpp
│   ├── PauseState.hpp / .cpp
│   └── GameOverState.hpp / .cpp
├── entities/
│   ├── Entity.hpp                ← abstract base
│   ├── Tower.hpp / .cpp
│   ├── Enemy.hpp / .cpp
│   └── Projectile.hpp / .cpp
├── managers/
│   ├── EntityManager.hpp / .cpp
│   ├── WaveManager.hpp / .cpp
│   └── ResourceManager.hpp / .cpp
├── map/
│   ├── Map.hpp / .cpp
│   └── Tile.hpp
└── defender_lib.cpp              ← CMake stub
```
