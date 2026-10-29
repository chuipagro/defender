##
## ─────────────────────────────────────────────────────────────
##   Project  :  defender
##   File     :  Makefile
##   Author   :  chuipagro
##   Date     :  29/10/2026
## ─────────────────────────────────────────────────────────────
##
##   Thin wrapper around CMake. Every build target first runs
##   update_cmakelists.sh, so new .cpp files in src/ or tests/
##   are picked up automatically.
##

# ══════════════════════════════════════════════════════════════
#  Binary & build settings
# ══════════════════════════════════════════════════════════════

NAME        := defender
TESTS_NAME  := defender_tests

BUILD_DIR   := build
BUILD_TYPE  ?= Release
JOBS        ?= $(shell nproc 2>/dev/null || echo 4)

CMAKE       := cmake
UPDATE      := ./update_cmakelists.sh
BANNER      := banner.txt
STATUS      := $(BUILD_DIR)/.last_status

VERSION     := $(shell grep -m1 -oE 'defender VERSION [0-9.]+' CMakeLists.txt | cut -d' ' -f3)
CXX_VERSION := $(shell c++ -dumpfullversion 2>/dev/null || echo unknown)
START       := $(shell date +%s%N)

# ══════════════════════════════════════════════════════════════
#  Colors
# ══════════════════════════════════════════════════════════════

RESET       := \033[0m
BOLD        := \033[1m
DIM         := \033[2m
RED         := \033[31m
GREEN       := \033[32m
YELLOW      := \033[33m
BLUE        := \033[34m
MAGENTA     := \033[35m
CYAN        := \033[36m

# Hide "Entering directory" noise from the make calls CMake runs.
MAKEFLAGS   += --no-print-directory

# Steps run in order; parallelism happens inside CMake (-j).
.NOTPARALLEL:

# ══════════════════════════════════════════════════════════════
#  Display helpers
# ══════════════════════════════════════════════════════════════

# Octopus + signature from banner.txt, with a pink → cyan gradient.
define PRINT_BANNER
	@if [ -f $(BANNER) ]; then \
		printf "\n"; \
		awk 'NR == FNR { n++; next } \
			{ c = split("201 200 165 164 129 128 93 99 63 69 75 81 45 51 50 49", p, " "); \
			  i = int((FNR - 1) * c / n) + 1; \
			  printf "\033[1;38;5;%sm%s\033[0m\n", p[i], $$0 }' $(BANNER) $(BANNER); \
		printf "\n"; \
	fi
endef

# $(call STEP,<n/total>,<title>)
define STEP
	@printf "\n$(BOLD)$(BLUE)  ❯ [$(1)]$(RESET)$(BOLD) $(2)$(RESET)\n"
	@printf "$(DIM)  ──────────────────────────────────────────────────$(RESET)\n"
endef

# $(call BOX,<color>,<symbol>,<text>)  — shell command, 50 cols inside
BOX = printf "\n$(BOLD)$(1)  ╭──────────────────────────────────────────────────╮\n  │  $(2)  %-45s│\n  ╰──────────────────────────────────────────────────╯$(RESET)\n\n" "$(3)"

# Sets $$t to the time elapsed since make started (e.g. 1.42s).
ELAPSED = ms=$$(( ($$(date +%s%N) - $(START)) / 1000000 )); \
	t=$$(printf '%d.%02ds' $$((ms / 1000)) $$((ms % 1000 / 10)))

# Rewrites CMake's progress lines into a compact, colored log.
# Compiler errors and anything else go through untouched.
PRETTY := sed -u -E \
	-e 's|^(\[ *[0-9]+%\]) Building CXX object .*\.dir/(.*)\.o$$|  \x1b[2m\1\x1b[0m  \x1b[1;36mCXX \x1b[0m \2|' \
	-e 's|^(\[ *[0-9]+%\]) Linking CXX [a-z ]+ (.*)$$|  \x1b[2m\1\x1b[0m  \x1b[1;35mLINK\x1b[0m \2|' \
	-e 's|^(\[ *[0-9]+%\]) Built target (.*)$$|  \x1b[2m\1\x1b[0m  \x1b[1;32mDONE\x1b[0m \x1b[2m\2\x1b[0m|' \
	-e 's|^-- Configuring done.*|  \x1b[35m⚙\x1b[0m  \x1b[2mCMakeLists changed: reconfigured\x1b[0m|' \
	-e '/^-- /d' \
	-e '/^g?make\[[0-9]+\]: \*\*\*/d'

# $(call BUILD_TARGET,<target>) — builds through PRETTY, keeps the exit code.
define BUILD_TARGET
	@mkdir -p $(BUILD_DIR)
	@{ $(CMAKE) --build $(BUILD_DIR) --target $(1) -j $(JOBS) 2>&1; echo $$? > $(STATUS); } | $(PRETTY); \
	if [ "$$(cat $(STATUS))" != 0 ]; then \
		$(ELAPSED); $(call BOX,$(RED),✖,Build of $(1) failed after $$t); exit 1; \
	fi
endef

define INFO_BOX
	@printf "$(BOLD)$(BLUE)  ╭──────────────────────────────────────────────────╮$(RESET)\n"
	@printf "$(BOLD)$(BLUE)  │$(RESET)  $(CYAN)◆$(RESET) $(BOLD)%-9s$(RESET) %-36s$(BOLD)$(BLUE)│$(RESET)\n" "Project" "$(NAME) v$(VERSION)"
	@printf "$(BOLD)$(BLUE)  │$(RESET)  $(CYAN)◆$(RESET) $(BOLD)%-9s$(RESET) %-36s$(BOLD)$(BLUE)│$(RESET)\n" "Build" "$(BUILD_TYPE)"
	@printf "$(BOLD)$(BLUE)  │$(RESET)  $(CYAN)◆$(RESET) $(BOLD)%-9s$(RESET) %-36s$(BOLD)$(BLUE)│$(RESET)\n" "Compiler" "c++ $(CXX_VERSION)"
	@printf "$(BOLD)$(BLUE)  │$(RESET)  $(CYAN)◆$(RESET) $(BOLD)%-9s$(RESET) %-36s$(BOLD)$(BLUE)│$(RESET)\n" "Jobs" "$(JOBS)"
	@printf "$(BOLD)$(BLUE)  ╰──────────────────────────────────────────────────╯$(RESET)\n"
endef

# ══════════════════════════════════════════════════════════════
#  Shared steps
# ══════════════════════════════════════════════════════════════

define DO_UPDATE
	@$(UPDATE)
endef

# Configure only when the build tree was never generated (build/Makefile
# only exists after a successful configure). Afterwards,
# `cmake --build` reconfigures by itself if a CMakeLists changed.
define DO_CONFIGURE
	@if [ -f $(BUILD_DIR)/Makefile ]; then \
		printf "  $(GREEN)✔$(RESET)  Already configured $(DIM)(CMake reconfigures itself when needed)$(RESET)\n"; \
	else \
		mkdir -p $(BUILD_DIR); \
		printf "  $(MAGENTA)⚙$(RESET)  Running CMake ($(BUILD_TYPE))...\n"; \
		if $(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
				-DCMAKE_COLOR_DIAGNOSTICS=ON > $(BUILD_DIR)/configure.log 2>&1; then \
			grep -h -E 'SFML' $(BUILD_DIR)/configure.log | sed 's/^-- /  \x1b[32m✔\x1b[0m  /'; \
			printf "  $(GREEN)✔$(RESET)  Configured $(DIM)(log: $(BUILD_DIR)/configure.log)$(RESET)\n"; \
		else \
			cat $(BUILD_DIR)/configure.log; \
			$(ELAPSED); $(call BOX,$(RED),✖,CMake configuration failed after $$t); exit 1; \
		fi; \
	fi
endef

# ══════════════════════════════════════════════════════════════
#  Default target
# ══════════════════════════════════════════════════════════════

.DEFAULT_GOAL := all

all:
	$(PRINT_BANNER)
	$(INFO_BOX)
	$(call STEP,1/3,Updating CMakeLists)
	$(DO_UPDATE)
	$(call STEP,2/3,Configuring)
	$(DO_CONFIGURE)
	$(call STEP,3/3,Compiling $(NAME))
	$(call BUILD_TARGET,$(NAME))
	@cp $(BUILD_DIR)/src/$(NAME) ./$(NAME)
	@printf "\n  $(GREEN)✔$(RESET)  Binary $(BOLD)./$(NAME)$(RESET) ready $(DIM)(%s)$(RESET)\n" "$$(du -h ./$(NAME) | cut -f1)"
	@$(ELAPSED); $(call BOX,$(GREEN),✔,Build successful in $$t)

# ══════════════════════════════════════════════════════════════
#  Standalone steps
# ══════════════════════════════════════════════════════════════

update:
	$(call STEP,update,Updating CMakeLists)
	$(DO_UPDATE)

configure:
	$(call STEP,configure,Configuring)
	$(DO_CONFIGURE)

# ══════════════════════════════════════════════════════════════
#  Tests
# ══════════════════════════════════════════════════════════════

tests_run:
	$(PRINT_BANNER)
	$(INFO_BOX)
	$(call STEP,1/4,Updating CMakeLists)
	$(DO_UPDATE)
	$(call STEP,2/4,Configuring)
	$(DO_CONFIGURE)
	$(call STEP,3/4,Compiling $(TESTS_NAME))
	$(call BUILD_TARGET,$(TESTS_NAME))
	$(call STEP,4/4,Running tests)
	@if ctest --test-dir $(BUILD_DIR) --output-on-failure -j $(JOBS); then \
		$(ELAPSED); $(call BOX,$(GREEN),✔,All tests passed in $$t); \
	else \
		$(ELAPSED); $(call BOX,$(RED),✖,Some tests failed after $$t); exit 1; \
	fi

tests: tests_run

# ══════════════════════════════════════════════════════════════
#  Run
# ══════════════════════════════════════════════════════════════

run: all
	@printf "  $(MAGENTA)▶$(RESET)  Launching $(BOLD)./$(NAME)$(RESET)...\n\n"
	@./$(NAME)

# ══════════════════════════════════════════════════════════════
#  Clean
# ══════════════════════════════════════════════════════════════

# Removes compiled objects but keeps the CMake config and the
# downloaded dependencies (SFML, GoogleTest).
clean:
	$(call STEP,clean,Removing compiled objects)
	@if [ -f $(BUILD_DIR)/Makefile ]; then \
		$(CMAKE) --build $(BUILD_DIR) --target clean; \
		printf "  $(GREEN)✔$(RESET)  Objects removed $(DIM)(CMake config kept)$(RESET)\n"; \
	else \
		printf "  $(DIM)•  Nothing to clean$(RESET)\n"; \
	fi

# Removes the whole build tree and the binary.
fclean:
	$(call STEP,fclean,Removing build tree and binary)
	@if [ -d $(BUILD_DIR) ]; then \
		size=$$(du -sh $(BUILD_DIR) | cut -f1); rm -rf $(BUILD_DIR); \
		printf "  $(GREEN)✔$(RESET)  Removed $(BOLD)$(BUILD_DIR)/$(RESET) $(DIM)(%s)$(RESET)\n" "$$size"; \
	else \
		printf "  $(DIM)•  No $(BUILD_DIR)/ to remove$(RESET)\n"; \
	fi
	@if [ -f $(NAME) ]; then \
		rm -f $(NAME); \
		printf "  $(GREEN)✔$(RESET)  Removed $(BOLD)./$(NAME)$(RESET)\n"; \
	else \
		printf "  $(DIM)•  No ./$(NAME) to remove$(RESET)\n"; \
	fi

re: fclean all

# ══════════════════════════════════════════════════════════════
#  Sprites
# ══════════════════════════════════════════════════════════════

# Regenerates assets/sprites/ from tools/sprites/ (key grids + palettes)
# and writes zoomed previews into build/sprites-preview/.
sprites:
	$(call STEP,sprites,Generating pixel-art sprites)
	@python3 tools/sprites/generate.py --preview $(BUILD_DIR)/sprites-preview
	@printf "  $(GREEN)✔$(RESET)  Sprites ready $(DIM)(preview: $(BUILD_DIR)/sprites-preview/)$(RESET)\n"

# ══════════════════════════════════════════════════════════════
#  Help
# ══════════════════════════════════════════════════════════════

help:
	$(PRINT_BANNER)
	@printf "$(BOLD)  Targets$(RESET)\n"
	@printf "  $(CYAN)%-16s$(RESET) %s\n" \
		"make"           "update CMakeLists and build ./$(NAME)" \
		"make tests_run" "build and run the unit tests (alias: tests)" \
		"make run"       "build then launch the game" \
		"make clean"     "remove compiled objects (keeps CMake config)" \
		"make fclean"    "remove $(BUILD_DIR)/ and ./$(NAME)" \
		"make re"        "fclean + all" \
		"make update"    "only regenerate the CMakeLists source lists" \
		"make configure" "only run the CMake configuration" \
		"make sprites"   "regenerate assets/sprites/ (+ previews)"
	@printf "\n$(BOLD)  Variables$(RESET)\n"
	@printf "  $(CYAN)%-16s$(RESET) %s\n" \
		"BUILD_TYPE"     "Release (default) or Debug" \
		"JOBS"           "parallel jobs (default: $(JOBS))"
	@printf "\n"

# ══════════════════════════════════════════════════════════════
#  Phony
# ══════════════════════════════════════════════════════════════

.PHONY: all update configure tests_run tests run clean fclean re sprites help
