#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────
#   Project  :  defender
#   File     :  update_cmakelists.sh
#   Author   :  chuipagro
#   Date     :  29/10/2026
# ─────────────────────────────────────────────────────────────
#
#   Usage: ./update_cmakelists.sh [--dry-run]
#
#   Scans src/ and tests/ for .cpp files (recursively) and
#   rewrites the source lists between the markers:
#       src/CMakeLists.txt    # <SOURCES> ... # </SOURCES>
#       tests/CMakeLists.txt  # <TESTS>   ... # </TESTS>
#
#   src/main.cpp is excluded: it belongs to the executable,
#   everything else goes into defender_lib.
#   A file is only rewritten when its content changes, so CMake
#   does not reconfigure for nothing.
# ─────────────────────────────────────────────────────────────

set -euo pipefail

# ── Config ────────────────────────────────────────────────────

ROOT="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="$ROOT/src"
TESTS_DIR="$ROOT/tests"
SRC_CMAKE="$SRC_DIR/CMakeLists.txt"
TESTS_CMAKE="$TESTS_DIR/CMakeLists.txt"
INDENT="        "
DRY_RUN=false

# ── Colors ────────────────────────────────────────────────────

RESET="\033[0m"
BOLD="\033[1m"
GREEN="\033[32m"
YELLOW="\033[33m"
CYAN="\033[36m"
RED="\033[31m"
DIM="\033[2m"

# ── Helpers ───────────────────────────────────────────────────

info()    { printf "${BOLD}${CYAN}  ▶  ${RESET}${BOLD}%s${RESET}\n" "$*"; }
success() { printf "${BOLD}${GREEN}  ✔  ${RESET}%s\n" "$*"; }
warn()    { printf "${BOLD}${YELLOW}  ⚠  ${RESET}%s\n" "$*"; }
error()   { printf "${BOLD}${RED}  ✖  ${RESET}%s\n" "$*" >&2; }
detail()  { printf "${DIM}       %s${RESET}\n" "$*"; }

# ── Args ──────────────────────────────────────────────────────

for arg in "$@"; do
    case "$arg" in
        --dry-run) DRY_RUN=true ;;
        *) error "Unknown argument: $arg"; exit 1 ;;
    esac
done

# ── Scan ──────────────────────────────────────────────────────

# list_sources <dir> [excluded relative path]
# Prints every .cpp under <dir>, relative to <dir>, sorted.
list_sources() {
    local dir="$1" exclude="${2:-}"
    find "$dir" -type f -name "*.cpp" -printf "%P\n" \
        | LC_ALL=C sort \
        | { if [[ -n "$exclude" ]]; then grep -vxF "$exclude" || true; else cat; fi; }
}

# build_block <files...>
# One indented path per line, as expected inside the CMake call.
build_block() {
    local f
    for f in "$@"; do
        printf "%s%s\n" "$INDENT" "$f"
    done
}

# ── Replace ───────────────────────────────────────────────────

# replace_block <file> <tag> <block>
replace_block() {
    local file="$1" tag="$2" block="$3"
    local tmp

    if [[ ! -f "$file" ]]; then
        error "File not found: $file"
        exit 1
    fi
    if ! grep -q "# <${tag}>" "$file" || ! grep -q "# </${tag}>" "$file"; then
        error "Markers '# <${tag}>' / '# </${tag}>' not found in ${file#"$ROOT"/}"
        exit 1
    fi

    tmp="$(mktemp)"
    BLOCK="$block" awk -v start="# <${tag}>" -v stop="# </${tag}>" '
        index($0, start) { print; printf "%s", ENVIRON["BLOCK"]; skip = 1; next }
        index($0, stop)  { skip = 0 }
        !skip            { print }
    ' "$file" > "$tmp"

    if cmp -s "$tmp" "$file"; then
        detail "${file#"$ROOT"/}: up to date"
        rm -f "$tmp"
    elif $DRY_RUN; then
        warn "${file#"$ROOT"/}: would change (dry run)"
        diff -u "$file" "$tmp" || true
        rm -f "$tmp"
    else
        cat "$tmp" > "$file"
        rm -f "$tmp"
        success "${file#"$ROOT"/}: updated"
    fi
}

# ── Main ──────────────────────────────────────────────────────

info "Scanning sources..."

mapfile -t SRC_FILES   < <(list_sources "$SRC_DIR" "main.cpp")
mapfile -t TEST_FILES  < <(list_sources "$TESTS_DIR")

detail "src/   : ${#SRC_FILES[@]} file(s) ${SRC_FILES[*]:-}"
detail "tests/ : ${#TEST_FILES[@]} file(s) ${TEST_FILES[*]:-}"

if [[ ${#SRC_FILES[@]} -eq 0 ]]; then
    error "No .cpp file in src/ besides main.cpp: defender_lib would be empty"
    exit 1
fi
if [[ ${#TEST_FILES[@]} -eq 0 ]]; then
    error "No .cpp file in tests/: defender_tests would be empty"
    exit 1
fi

replace_block "$SRC_CMAKE"   "SOURCES" "$(build_block "${SRC_FILES[@]}")"$'\n'
replace_block "$TESTS_CMAKE" "TESTS"   "$(build_block "${TEST_FILES[@]}")"$'\n'
