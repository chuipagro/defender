<div align="center">

# 🏰 Defender

**A tower defense game built with C++20 and SFML**

[![CI](https://github.com/chuipagro/defender/actions/workflows/ci.yml/badge.svg)](https://github.com/chuipagro/defender/actions/workflows/ci.yml)
[![Release](https://github.com/chuipagro/defender/actions/workflows/release.yml/badge.svg)](https://github.com/chuipagro/defender/releases)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://github.com/chuipagro/defender/blob/main/LICENSE)

</div>

---

## About

**Defender** is a tower defense game where you place towers to stop waves of enemies from reaching your base. Built from scratch in **C++20** with **SFML** for rendering and audio.

- 🎮 Classic tower defense gameplay
- 🏗️ Clean C++20 architecture (concepts, spans, RAII, variants)
- 🧪 Tested with GoogleTest + CTest
- 🚀 CI/CD via GitHub Actions

---

## Quick Start

=== "Build"

    ```bash
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel
    ./build/src/defender
    ```

=== "Test"

    ```bash
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure
    ```

---

## Project Structure

```
defender/
├── src/
│   ├── core/          # Game, StateMachine, EventBus
│   ├── states/        # MenuState, PlayState, PauseState, GameOverState
│   ├── entities/      # Tower, Enemy, Projectile
│   ├── managers/      # EntityManager, WaveManager, ResourceManager
│   └── map/           # Map, Tile
├── include/defender/  # Public headers
├── tests/             # GoogleTest unit tests
├── assets/            # Textures, fonts, sounds
└── .github/workflows/ # CI/CD pipelines
```

---

## Documentation

| Section | Description |
|---|---|
| [Getting Started](getting-started.md) | Build, run, and test the project |
| [Architecture](architecture.md) | Class diagrams, state machine, entity hierarchy |
| [CI/CD](ci-cd.md) | Pipelines, releases, and versioning |
| [Code Style](code-style.md) | Formatting and linting conventions |
