"""Element palettes.

Every sprite is drawn with *keys*, not colors. A palette maps the
element-dependent keys to real colors, so one drawing gives every
element variant.

Element keys (change with the element):
    O  outline (darkest)
    1  shadow
    2  base color
    3  light
    4  highlight
    a  accent dark   (beak, feet, crystals, trims...)
    b  accent light

Fixed keys (same for every element) are in FIXED below.

To add an element (magma, ice, lightning...), add an entry to
ELEMENTS and run `make sprites`.
"""

ELEMENTS = {
    "water": {
        "O": "#14234f",
        "1": "#24479a",
        "2": "#3a72d8",
        "3": "#6aa6f2",
        "4": "#bfe0ff",
        "a": "#e0a42a",
        "b": "#ffd66b",
    },
    "fire": {
        "O": "#3d0d0d",
        "1": "#9a1f1a",
        "2": "#d93a28",
        "3": "#f2703a",
        "4": "#ffb36b",
        "a": "#f2c230",
        "b": "#fff08a",
    },
    "earth": {
        "O": "#241c17",
        "1": "#54463b",
        "2": "#7a6a5c",
        "3": "#9e8f80",
        "4": "#c2b6a8",
        "a": "#5f7d34",
        "b": "#93b856",
    },
    "wind": {
        "O": "#3f4c5c",
        "1": "#a3b2c2",
        "2": "#d3dde7",
        "3": "#edf2f7",
        "4": "#ffffff",
        "a": "#5cc7b0",
        "b": "#b5f2e4",
    },
}

FIXED = {
    ".": None,          # transparent
    "w": "#ffffff",     # eye white
    "k": "#16141a",     # pupil / nose
    "r": "#f08ca0",     # pink (tongue, inner ear)
    "f": "#e8b48a",     # skin (orangutan face)
    "F": "#b07a56",     # skin shadow
    "S": "#2a2a33",     # stone outline
    "s": "#5d5f6b",     # stone dark
    "t": "#878a96",     # stone mid
    "u": "#b3b6c0",     # stone light
    "x": (0, 0, 0, 70), # ground shadow (semi-transparent)
}


def hex_to_rgba(value):
    if value is None:
        return (0, 0, 0, 0)
    if isinstance(value, tuple):
        return value
    value = value.lstrip("#")
    return (int(value[0:2], 16), int(value[2:4], 16), int(value[4:6], 16), 255)


def resolve(element):
    """Full key -> RGBA table for one element."""
    table = {k: hex_to_rgba(v) for k, v in FIXED.items()}
    table.update({k: hex_to_rgba(v) for k, v in ELEMENTS[element].items()})
    return table


# ── Map biomes ────────────────────────────────────────────────────────────────
#
# Tile keys (see tiles.py):
#   A B C      ground dark / mid / light       D E   ground details
#   P Q R S    path edge / mid / light / detail
#   T U V W X  build slab outline / dark / mid / light / rune glow
#   K          obstacle outline
#   1 2 3 4    obstacle main ramp (cactus, crystal, lava, foliage...)
#   j l m n    obstacle second ramp (rock, trunk...)
#   a b        obstacle accent (flowers, smoke, berries...)

BIOMES = {
    "desert": {
        "A": "#c4925a", "B": "#d8ad6c", "C": "#e8c587", "D": "#b07d47", "E": "#f4dcaa",
        "P": "#8a5a34", "Q": "#a8744a", "R": "#bb895c", "S": "#7a4c2a",
        "T": "#5e4430", "U": "#a07a52", "V": "#c49a68", "W": "#e0bb86", "X": "#4fd0e0",
        "K": "#2a2016",
        "1": "#2f6b34", "2": "#43913f", "3": "#6fbf55", "4": "#a8e07a",
        "j": "#6e4f36", "l": "#8f6c4a", "m": "#b08a60", "n": "#cfae80",
        "a": "#e85a8a", "b": "#ffb0c8",
    },
    "ice": {
        "A": "#a9c4d8", "B": "#cfe2ee", "C": "#e6f2f9", "D": "#93b3ca", "E": "#ffffff",
        "P": "#5a86a8", "Q": "#7fa8c8", "R": "#9cc0dc", "S": "#4a6f8e",
        "T": "#2e4258", "U": "#5d7891", "V": "#8aa3b8", "W": "#b8cad8", "X": "#7fe0ff",
        "K": "#1a2c40",
        "1": "#3f7fb8", "2": "#6fb0e0", "3": "#a8dcf5", "4": "#ffffff",
        "j": "#4f5f70", "l": "#6f8194", "m": "#93a5b6", "n": "#c4d2de",
        "a": "#2f5e4a", "b": "#4f8a6a",
    },
    "volcano": {
        "A": "#2c2628", "B": "#3a3234", "C": "#4c4244", "D": "#1e1a1b", "E": "#665856",
        "P": "#3a1610", "Q": "#5e2618", "R": "#7a3420", "S": "#ff7a2a",
        "T": "#120e14", "U": "#2a2230", "V": "#3d3346", "W": "#5a4d66", "X": "#ff9a3c",
        "K": "#0e0a0b",
        "1": "#b3261e", "2": "#f05a28", "3": "#ffa040", "4": "#ffe080",
        "j": "#1e1a1b", "l": "#332b2d", "m": "#4a3f42", "n": "#66595c",
        "a": "#6b6466", "b": "#9a9294",
    },
    "forest": {
        "A": "#3f7a3a", "B": "#4f9444", "C": "#62a852", "D": "#2f5e2f", "E": "#8cc96b",
        "P": "#6b4a2e", "Q": "#8a6440", "R": "#a37a52", "S": "#57391f",
        "T": "#2a2f2a", "U": "#5d6660", "V": "#7f8a82", "W": "#a3ada5", "X": "#8fe07a",
        "K": "#142414",
        "1": "#23502a", "2": "#2f6e35", "3": "#43914a", "4": "#6fbf6a",
        "j": "#3e2a1a", "l": "#5a3e26", "m": "#7a5636", "n": "#9a7450",
        "a": "#e04a4a", "b": "#ffd04a",
    },
}


def resolve_biome(biome):
    """Full key -> RGBA table for one biome's tiles."""
    table = {k: hex_to_rgba(v) for k, v in FIXED.items()}
    table.update({k: hex_to_rgba(v) for k, v in BIOMES[biome].items()})
    return table
