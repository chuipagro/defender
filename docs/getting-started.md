# Getting Started

## Prerequisites

| Tool | Version |
|---|---|
| CMake | ≥ 3.21 |
| GCC / Clang | C++20 support (GCC ≥ 12, Clang ≥ 14) |
| SFML deps | Auto-fetched via FetchContent |
| OpenAL | `libopenal-dev` (for audio) |

**Install system dependencies (Ubuntu/Debian):**

```bash
sudo apt-get install -y \
  cmake build-essential \
  libgl1-mesa-dev libudev-dev \
  libfreetype6-dev \
  libopenal-dev libvorbis-dev libflac-dev
```

---

## Build

```bash
# Configure
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Compile (parallel build)
cmake --build build --parallel

# Run
./build/src/defender
```

---

## Tests

Tests are **headless** — no display required. They target `defender_lib` (game logic only, no SFML window).

```bash
# Configure in Debug mode
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build --parallel

# Run tests
ctest --test-dir build --output-on-failure
```

!!! info "No display needed"
    The test suite runs on `defender_lib` which excludes SFML window code, making it suitable for CI environments without a display server.

---

## Release Builds

The CI pipeline produces pre-built binaries for each tagged release. You can download them directly from the [Releases page](https://github.com/chuipagro/defender/releases) — no build required.
