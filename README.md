<div align="center">

# 🏰 Defender

**A tower defense game built with C++20 and SFML**

[![CI](https://github.com/chuipagro/defender/actions/workflows/ci.yml/badge.svg)](https://github.com/chuipagro/defender/actions/workflows/ci.yml)
[![Release](https://github.com/chuipagro/defender/actions/workflows/release.yml/badge.svg)](https://github.com/chuipagro/defender/releases)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

</div>

---

## 📖 About

**Defender** is a tower defense game where you place towers to stop waves of enemies from reaching your base. Built from scratch in **C++20** with **SFML** for rendering and audio.

- 🎮 Classic tower defense gameplay
- 🏗️ Clean C++20 architecture (concepts, spans, RAII, variants)
- 🧪 Tested with GoogleTest + CTest
- 🚀 CI/CD via GitHub Actions

## 📐 Architecture

See [ARCHITECTURE.md](ARCHITECTURE.md) for the full design: class diagrams, state machine, entity hierarchy, event bus, and game loop.

---

## 🛠️ Build

### Prerequisites

| Tool | Version |
|---|---|
| CMake | ≥ 3.21 |
| GCC / Clang | C++20 support (GCC ≥ 12, Clang ≥ 14) |
| SFML deps | Auto-fetched via FetchContent |
| OpenAL | `libopenal-dev` (for audio) |

**Install system deps (Ubuntu/Debian):**
```bash
sudo apt-get install -y \
  cmake build-essential \
  libgl1-mesa-dev libudev-dev \
  libfreetype6-dev \
  libopenal-dev libvorbis-dev libflac-dev
```

### Build

```bash
# Configure
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Compile
cmake --build build --parallel

# Run
./build/src/defender
```

---

## 🧪 Tests

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Tests are **headless** — no display required. They target `defender_lib` (game logic only, no SFML window).

---

## 🚀 CI/CD

| Workflow | Trigger | Action |
|---|---|---|
| `ci.yml` | Push / PR on `main`, `develop` | Build + test |
| `release-drafter.yml` | PR merged to `main` | Update release draft notes |
| `release.yml` | `git push tag v*.*.*` | Build, package, publish release |

### Release a new version

```bash
git tag v0.2.0
git push origin v0.2.0
```

GitHub Actions will build the binary, package it as `defender-v0.2.0-linux-x86_64.tar.gz`, and publish it to the [Releases](https://github.com/chuipagro/defender/releases) page.

### Conventional Commits → Auto-labels

PR titles follow [Conventional Commits](https://www.conventionalcommits.org/) and are **auto-labeled**:

| Prefix | Label | Release section |
|---|---|---|
| `feat:` | `feature` | 🚀 New Features |
| `fix:` | `bug` | 🐛 Bug Fixes |
| `perf:` | `performance` | ⚡ Performance |
| `refactor:` | `enhancement` | ✨ Enhancements |
| `docs:` | `docs` | 📖 Documentation |
| `chore:` / `ci:` / `test:` | `chore` | 🔧 Chores |
| `breaking:` | `breaking` | 💥 Breaking Changes |

---

## 🗂️ Project Structure

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

## 🔧 Code Style

Formatted with **clang-format** (Allman, 4 spaces, 100 cols) and linted with **clang-tidy**:

```bash
# Format all sources
clang-format -i src/**/*.cpp include/**/*.hpp

# Lint (requires build/compile_commands.json)
clang-tidy -p build src/**/*.cpp
```

---

## 📄 License

MIT © chuipagro
