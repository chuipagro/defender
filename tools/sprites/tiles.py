"""Map tiles: 32x32, one sheet per biome (see BIOMES in palettes.py).

Sheet layout (16 columns x 2 rows of 32x32 tiles):
    row 0, col m       path tile for neighbor mask m (N=1, E=2, S=4, W=8)
    row 1, cols 0-3    buildable slab variants
    row 1, cols 4-7    obstacle variants (biome specific)
    row 1, cols 8-11   plain ground variants

Every tile is drawn on the biome ground, so tiles can be laid out
without any extra background layer.
"""

import math
import random

from canvas import Canvas

T = 32
PATH_MIN, PATH_MAX = 6, 26     # path band inside a tile (20 px wide)
CORNER = 5                     # rounding of the outer path corners
N, E, S, W = 1, 2, 4, 8

LAYOUT = {
    "path": (0, 0),            # (row, first column)
    "buildable": (1, 0),
    "obstacle": (1, 4),
    "ground": (1, 8),
}
VARIANTS = 4


# ── Ground ────────────────────────────────────────────────────────────────────

def ground(c, biome, rng):
    c.rect(0, 0, T, T, "B")
    for _ in range(70):                                   # soft grain
        c.set(rng.randrange(T), rng.randrange(T), rng.choice("AC"))
    if biome == "desert":
        for _ in range(4):                                # wind ripples
            x, y = rng.randrange(2, 26), rng.randrange(3, 30)
            c.rect(x, y, rng.randrange(3, 6), 1, "C")
            c.rect(x + 1, y + 1, 3, 1, "A")
        for _ in range(3):
            c.set(rng.randrange(2, 30), rng.randrange(2, 30), "D")
    elif biome == "ice":
        for _ in range(5):                                # sparkles
            c.set(rng.randrange(1, 31), rng.randrange(1, 31), "E")
        for _ in range(2):                                # thin cracks
            x, y = rng.randrange(4, 28), rng.randrange(4, 28)
            for _ in range(5):
                c.set(x, y, "D")
                x, y = x + rng.choice((-1, 1)), y + rng.choice((0, 1))
    elif biome == "volcano":
        for _ in range(3):                                # dark cracks
            x, y = rng.randrange(3, 29), rng.randrange(3, 29)
            for _ in range(6):
                c.set(x, y, "D")
                x, y = x + rng.choice((-1, 0, 1)), y + 1
        for _ in range(4):
            c.set(rng.randrange(1, 31), rng.randrange(1, 31), "E")
    elif biome == "forest":
        for _ in range(6):                                # grass tufts
            x, y = rng.randrange(2, 30), rng.randrange(3, 30)
            c.set(x, y, "D")
            c.set(x - 1, y - 1, "D")
            c.set(x + 1, y - 1, "D")
            c.set(x, y - 2, "E")
        if rng.random() < 0.5:                            # tiny flower
            x, y = rng.randrange(4, 28), rng.randrange(4, 28)
            c.set(x, y, "b")
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                c.set(x + dx, y + dy, "w")


# ── Path (autotile) ──────────────────────────────────────────────────────────

def path_inside(x, y, mask):
    """Is (x, y) on the path? Coordinates may be outside the tile."""
    def band(v):
        return PATH_MIN <= v < PATH_MAX

    if x < 0:
        return bool(mask & W) and band(y)
    if x >= T:
        return bool(mask & E) and band(y)
    if y < 0:
        return bool(mask & N) and band(x)
    if y >= T:
        return bool(mask & S) and band(x)
    if band(x) and band(y):
        # Round the outer corners where both sides are closed.
        for closed, cx, cy in (
            (not mask & (N | W), PATH_MIN + CORNER, PATH_MIN + CORNER),
            (not mask & (N | E), PATH_MAX - CORNER, PATH_MIN + CORNER),
            (not mask & (S | W), PATH_MIN + CORNER, PATH_MAX - CORNER),
            (not mask & (S | E), PATH_MAX - CORNER, PATH_MAX - CORNER),
        ):
            inx = x < cx if cx == PATH_MIN + CORNER else x >= cx
            iny = y < cy if cy == PATH_MIN + CORNER else y >= cy
            if closed and inx and iny and math.hypot(x + 0.5 - cx, y + 0.5 - cy) > CORNER:
                return False
        return True
    return ((mask & N and y < PATH_MIN) or (mask & S and y >= PATH_MAX)) and band(x) or \
           ((mask & W and x < PATH_MIN) or (mask & E and x >= PATH_MAX)) and band(y)


def path_tile(biome, mask, rng):
    c = Canvas(T, T)
    ground(c, biome, rng)
    cells = [(x, y) for y in range(T) for x in range(T) if path_inside(x, y, mask)]
    for x, y in cells:
        edge = any(not path_inside(x + dx, y + dy, mask)
                   for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
        c.set(x, y, "P" if edge else "Q")
    # Soft inner shading next to the border
    for x, y in cells:
        if c.px[y][x] == "Q" and any(
                not path_inside(x + dx, y + dy, mask)
                for dx, dy in ((2, 0), (-2, 0), (0, 2), (0, -2))):
            c.set(x, y, "R" if rng.random() < 0.5 else "Q")
    inner = [(x, y) for x, y in cells if c.px[y][x] == "Q"]
    for _ in range(14):                                   # pebbles / speckles
        x, y = rng.choice(inner)
        c.set(x, y, rng.choice("RS"))
    if biome == "volcano":                                # glowing cracks
        for _ in range(2):
            x, y = rng.choice(inner)
            for _ in range(7):
                if (x, y) in inner:
                    c.set(x, y, "S")
                x, y = x + rng.choice((-1, 0, 1)), y + rng.choice((-1, 1))
    return c


# ── Buildable slab ───────────────────────────────────────────────────────────

def buildable_tile(biome, variant, rng):
    c = Canvas(T, T)
    ground(c, biome, rng)
    x0, y0, size = 4, 4, 24
    c.rect(x0, y0, size, size, "T")
    c.rect(x0 + 1, y0 + 1, size - 2, size - 2, "V")
    c.rect(x0 + 1, y0 + 1, size - 2, 1, "W")              # bevel light
    c.rect(x0 + 1, y0 + 1, 1, size - 2, "W")
    c.rect(x0 + 1, y0 + size - 2, size - 2, 1, "U")       # bevel shadow
    c.rect(x0 + size - 2, y0 + 1, 1, size - 2, "U")
    for cx, cy in ((7, 7), (24, 7), (7, 24), (24, 24)):   # corner studs
        c.set(cx, cy, "U")
        c.set(cx - 1, cy - 1, "W")
    if variant == 1:                                      # crack
        x, y = 11, 8
        for _ in range(9):
            c.set(x, y, "U")
            x, y = x + rng.choice((0, 1)), y + 1
    elif variant == 2:                                    # ground creeping on it
        for _ in range(12):
            c.set(rng.randrange(5, 27), rng.randrange(20, 27), rng.choice("DA"))
    elif variant == 3:                                    # glowing rune
        for dx, dy in ((0, -3), (0, 3), (-3, 0), (3, 0), (0, 0), (-1, -1), (1, 1), (1, -1), (-1, 1)):
            c.set(16 + dx, 16 + dy, "X")
    # Small dark mark in the middle of plain slabs, to read as "build here"
    if variant != 3:
        c.rect(15, 15, 2, 2, "U")
    return c


# ── Obstacles ────────────────────────────────────────────────────────────────

def shadow(c, cx, cy, rx, ry=2.2):
    c.ellipse(cx, cy, rx, ry, "x")


def crystal(c, cx, base, h, w, ramp="1234", outline="K"):
    """Upright faceted crystal: lit left face, dark right face."""
    cells = {}
    for k in range(h):
        y = base - k
        t = k / h
        half = w if t < 0.7 else w * (1 - t) / 0.3
        for x in range(round(cx - half), round(cx + half) + 1):
            cells[(x, y)] = x - cx
    for (x, y) in cells:
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            if (x + dx, y + dy) not in cells:
                c.set(x + dx, y + dy, outline)
    for (x, y), rel in cells.items():
        c.set(x, y, ramp[3] if rel < -w * 0.5 else ramp[2] if rel < 0 else ramp[1] if rel < w * 0.5 else ramp[0])


def tube(c, pts, r, ramp, outline="K"):
    c.blob([(x, y, r, r) for x, y in pts], ramp=ramp, outline=outline, highlight=False)


def line(p0, p1, n=8):
    return [(p0[0] + (p1[0] - p0[0]) * i / n, p0[1] + (p1[1] - p0[1]) * i / n) for i in range(n + 1)]


def pine(c, cx, base, layers, snow=False, ramp="1234"):
    c.rect(cx - 1, base - 3, 3, 4, "K")
    c.rect(cx, base - 3, 1, 3, "l")
    top = base - 3 - layers * 6
    for i in range(layers):
        y0 = top + i * 5
        h = 9
        cells = {}
        for k in range(h):
            half = 2 + k * (4 + i) / h
            for x in range(round(cx - half), round(cx + half) + 1):
                cells[(x, y0 + k)] = x - cx
        for (x, y) in cells:
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                if (x + dx, y + dy) not in cells:
                    c.set(x + dx, y + dy, "K")
        for (x, y), rel in cells.items():
            key = ramp[2] if rel < -1 else ramp[1] if rel < 2 else ramp[0]
            if snow and y - y0 < 2 + (abs(rel) < 2):
                key = "4" if rel < 1 else "3"
            c.set(x, y, key)


def desert_obstacle(c, v, rng):
    if v == 0:                                           # tall cactus
        shadow(c, 16, 28, 8)
        tube(c, line((16, 28), (16, 8)), 3, "1234")
        tube(c, line((16, 18), (10, 18), 4) + line((10, 18), (10, 11), 4), 2, "1234")
        tube(c, line((16, 15), (22, 15), 4) + line((22, 15), (22, 10), 4), 2, "1234")
        for x, y in ((15, 12), (17, 20), (15, 24), (10, 14), (22, 12)):
            c.set(x, y, "4")
        c.set(16, 5, "a")
        c.set(15, 5, "b")
        c.set(17, 5, "a")
    elif v == 1:                                         # barrel cactus + flowers
        shadow(c, 16, 27, 9)
        c.blob([(16, 21, 7, 6.5)], ramp="1234", outline="K")
        for x in (12, 16, 20):
            c.rect(x, 16, 1, 10, "1")
        for x in (13, 16, 19):
            c.set(x, 14, "a")
            c.set(x, 13, "b")
    elif v == 2:                                         # sandstone rocks
        shadow(c, 16, 27, 11)
        c.blob([(12, 22, 6, 5)], ramp="jlmn", outline="K")
        c.blob([(21, 23, 5, 4)], ramp="jlmn", outline="K")
        c.blob([(16, 16, 4, 3.5)], ramp="jlmn", outline="K")
    else:                                                # mesa pillar
        shadow(c, 16, 28, 10)
        c.rect(8, 9, 17, 19, "K")
        for i, key in enumerate("nmlmjlmn"):
            c.rect(9, 10 + i * 2, 15, 2, key)
        c.rect(9, 10, 15, 1, "n")
        c.rect(20, 11, 3, 16, "j")


def ice_obstacle(c, v, rng):
    if v == 0:                                           # crystal cluster
        shadow(c, 16, 28, 10)
        crystal(c, 11, 27, 14, 3)
        crystal(c, 21, 27, 12, 3)
        crystal(c, 16, 28, 21, 4)
    elif v == 1:                                         # snowy pine
        shadow(c, 16, 29, 8)
        pine(c, 16, 29, 3, snow=True, ramp="aabb")
    elif v == 2:                                         # rocks with snow caps
        shadow(c, 16, 27, 11)
        c.blob([(12, 22, 6, 5)], ramp="jlmn", outline="K")
        c.blob([(21, 23, 5, 4)], ramp="jlmn", outline="K")
        c.ellipse(11, 18.5, 4.5, 1.6, "E")
        c.ellipse(21, 20, 3.5, 1.3, "E")
    else:                                                # frozen boulder
        shadow(c, 16, 28, 11)
        c.blob([(16, 19, 10, 8.5)], ramp="1234", outline="K")
        for x, y in ((11, 14), (12, 15), (13, 16), (19, 21), (20, 22)):
            c.set(x, y, "4")


def volcano_obstacle(c, v, rng):
    if v == 0:                                           # lava pool
        c.blob([(16, 17, 12, 10)], ramp="jlmn", outline="K", highlight=False)
        c.ellipse(16, 17, 9.5, 7.5, "1")
        c.ellipse(16, 17, 8, 6.2, "2")
        c.ellipse(15, 16, 5, 3.6, "3")
        for x, y in ((12, 14), (19, 19), (17, 13)):
            c.set(x, y, "4")
            c.set(x + 1, y, "4")
    elif v == 1:                                         # obsidian spikes
        shadow(c, 16, 28, 10)
        crystal(c, 10, 27, 12, 3, ramp="jlmn")
        crystal(c, 22, 27, 14, 3, ramp="jlmn")
        crystal(c, 16, 28, 20, 4, ramp="jlmn")
        c.set(16, 22, "2")
        c.set(15, 23, "1")
    elif v == 2:                                         # rocks with ember veins
        shadow(c, 16, 27, 11)
        c.blob([(12, 22, 6, 5)], ramp="jlmn", outline="K")
        c.blob([(21, 23, 5, 4)], ramp="jlmn", outline="K")
        for x, y in ((10, 21), (11, 22), (12, 22), (13, 23), (21, 22), (22, 23)):
            c.set(x, y, "2")
    else:                                                # smoking vent
        shadow(c, 16, 28, 11)
        c.blob([(16, 23, 10, 5.5)], ramp="jlmn", outline="K")
        c.ellipse(16, 19.5, 3.5, 1.4, "1")
        c.ellipse(16, 19.5, 2, 0.8, "3")
        for x, y, r in ((15, 14, 2.5), (18, 9, 3), (15, 4, 2.4)):
            c.blob([(x, y, r, r * 0.9)], ramp="aabb", outline="K", highlight=False)


def forest_obstacle(c, v, rng):
    if v == 0:                                           # round tree
        shadow(c, 16, 28, 9)
        c.rect(14, 19, 5, 10, "K")
        c.rect(15, 19, 3, 9, "l")
        c.rect(15, 19, 1, 9, "m")
        c.blob([(16, 12, 9, 7), (10, 15, 5, 4.5), (22, 15, 5, 4.5)], ramp="1234", outline="K")
        for x, y in ((12, 10), (20, 13), (16, 16)):
            c.set(x, y, "a")
    elif v == 1:                                         # pine
        shadow(c, 16, 29, 8)
        pine(c, 16, 29, 3)
    elif v == 2:                                         # mossy rocks
        shadow(c, 16, 27, 11)
        c.blob([(12, 22, 6, 5)], ramp="jlmn", outline="K")
        c.blob([(21, 23, 5, 4)], ramp="jlmn", outline="K")
        c.ellipse(11, 18.5, 4, 1.5, "3")
        c.ellipse(21, 20, 3, 1.2, "3")
        for x in range(8, 15, 2):
            c.set(x, 19, "4")
    else:                                                # stump + mushroom
        shadow(c, 16, 28, 10)
        c.rect(9, 17, 13, 11, "K")
        c.rect(10, 18, 11, 9, "m")
        c.rect(18, 18, 3, 9, "l")
        c.ellipse(15.5, 17.5, 6.5, 2.4, "K")
        c.ellipse(15.5, 17.5, 5.5, 1.6, "n")
        c.ellipse(15.5, 17.5, 2.5, 0.8, "m")
        c.rect(23, 23, 2, 5, "w")                         # mushroom
        c.blob([(23.5, 22, 3.6, 2.4)], ramp="aaab", outline="K", highlight=False)
        c.set(22, 21, "w")
        c.set(25, 22, "w")


OBSTACLES = {
    "desert": desert_obstacle,
    "ice": ice_obstacle,
    "volcano": volcano_obstacle,
    "forest": forest_obstacle,
}


def obstacle_tile(biome, variant, rng):
    c = Canvas(T, T)
    ground(c, biome, rng)
    OBSTACLES[biome](c, variant, rng)
    return c


def ground_tile(biome, variant, rng):
    c = Canvas(T, T)
    ground(c, biome, rng)
    return c


# ── Sheet ────────────────────────────────────────────────────────────────────

def biome_sheet(biome):
    """Returns the 16x2 grid of tiles (list of rows of Canvas)."""
    seed = sum(map(ord, biome))
    rows = [[], []]
    for mask in range(16):
        rows[0].append(path_tile(biome, mask, random.Random(seed * 100 + mask)))
    for builder in (buildable_tile, obstacle_tile, ground_tile):
        for v in range(VARIANTS):
            rows[1].append(builder(biome, v, random.Random(seed * 1000 + len(rows[1]))))
    while len(rows[1]) < 16:
        rows[1].append(Canvas(T, T))
    return rows
