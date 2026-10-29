"""Tower sprites: 32x48 frames, 4-frame idle animation.

Every tower shares the same stone pillar (fixed stone colors) with a
band in the element colors, and has its own animated top.

TOWERS maps a tower name to its builder, ELEMENT_OF maps it to the
palette to use. A future tower can reuse a shape with another palette:
    TOWERS["magma"] = fire_tower ; ELEMENT_OF["magma"] = "magma"
"""

import math

from canvas import Canvas, grid

W, H = 32, 48
FRAMES = 4


# ── Shared pillar ─────────────────────────────────────────────────────────────

def pillar(c, top=24):
    """Stone pillar from row `top` down to the ground, element band on top."""
    # Foot
    c.rect(4, 42, 24, 5, "S")
    c.rect(5, 43, 22, 3, "t")
    c.rect(5, 43, 22, 1, "u")
    c.rect(5, 45, 22, 1, "s")
    # Shaft with a left-to-right light gradient
    c.rect(7, top, 18, 19, "S")
    for x in range(8, 24):
        key = "u" if x < 11 else "t" if x < 19 else "s"
        c.rect(x, top + 1, 1, 17, key)
    # Brick joints
    for i, y in enumerate(range(top + 4, 42, 4)):
        c.rect(8, y, 16, 1, "s")
        for x in range(10 + (i % 2) * 3, 24, 6):
            c.rect(x, y - 3, 1, 3, "s")
    # Arrow slit
    c.rect(15, top + 7, 2, 6, "S")
    c.rect(15, top + 7, 1, 6, "k")
    # Element band
    c.rect(5, top - 3, 22, 5, "O")
    c.rect(6, top - 2, 20, 1, "3")
    c.rect(6, top - 1, 20, 1, "2")
    c.rect(6, top, 20, 1, "1")
    for x in range(8, 25, 4):
        c.set(x, top - 1, "b")
    # Ground shadow
    for x in range(3, 29):
        c.set(x, 47, "x")


# ── Fire: brazier + flame ────────────────────────────────────────────────────

def flame(c, cx, base, height, frame, width=6.0):
    cells = {}
    phase = frame * math.pi / 2
    for k in range(height):
        t = k / height
        half = width * (1 - t) ** 0.75 * (0.85 + 0.15 * math.sin(phase + t * 6))
        center = cx + math.sin(phase + t * 5) * 1.6 * t
        for x in range(int(center - half) - 1, int(center + half) + 2):
            d = abs(x + 0.5 - center) / max(half, 0.5)
            if d <= 1:
                cells[(x, base - k)] = (d, t)
    # Two flying sparks
    for i in range(2):
        sx = cx + (-3 if i else 3) + (frame % 2)
        sy = base - height + 7 - ((frame + i * 2) % 4) * 2
        c.set(sx, sy, "b")
    for (x, y) in cells:
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            if (x + dx, y + dy) not in cells:
                c.set(x + dx, y + dy, "O")
    for (x, y), (d, t) in cells.items():
        key = "b" if d < 0.35 and t < 0.55 else "4" if d < 0.6 else "3" if d < 0.85 else "2"
        c.set(x, y, key)


BRAZIER = grid("""
    OOOOOOOOOOOOOOOOOOOO
    OtuuuuuuuuuuuuuuuttO
    .OsttttttttttttttsO.
    ..OSssssssssssssSO..
    ...OOOOOOOOOOOOOO...
""")


def fire_tower(frame):
    c = Canvas(W, H)
    pillar(c)
    flame(c, 16, 17, 15, frame)
    c.blit(BRAZIER, 6, 16)
    return c


# ── Water: floating orb dripping into a basin ────────────────────────────────

BASIN = grid("""
    OOOOOOOOOOOOOOOO
    Ot223333333322tO
    .Ot1111111111tO.
    ..OOOOOOOOOOOO..
""")

PRONGS = grid("""
    OO..............OO
    OuO............OsO
    OuO............OsO
    OttO..........OssO
""")


def water_tower(frame):
    c = Canvas(W, H)
    pillar(c)
    bob = [0, -1, -2, -1][frame]
    c.blit(PRONGS, 7, 17)
    c.blit(BASIN, 8, 19)
    c.orb(16, 9 + bob, 6)
    c.set(13, 6 + bob, "w")
    c.set(14, 6 + bob, "w")
    c.set(13, 7 + bob, "w")
    # Falling drop
    drop_y = 16 + bob + frame * 1
    c.set(16, drop_y, "3")
    c.set(16, drop_y + 1, "2")
    # Ripple in the basin
    if frame in (2, 3):
        c.set(14, 20, "4")
        c.set(18, 20, "4")
    return c


# ── Earth: rock spire + levitating pebbles ───────────────────────────────────

CRYSTAL = grid("""
    .O.
    ObO
    ObO
    OaO
    OaO
    .O.
""")


def spire(c, cx, top, bottom, half):
    """Faceted rock peak: lit left face, shaded right face."""
    cells = {}
    for y in range(top, bottom + 1):
        w = half * (y - top + 1) / (bottom - top + 1)
        # Jagged edges
        w += 0.6 if (y * 7) % 5 == 0 else 0
        for x in range(round(cx - w), round(cx + w) + 1):
            cells[(x, y)] = x - cx
    for (x, y) in cells:
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            if (x + dx, y + dy) not in cells:
                c.set(x + dx, y + dy, "O")
    for (x, y), rel in cells.items():
        c.set(x, y, "4" if rel < -half * 0.45 else "3" if rel < 0 else "2" if rel < half * 0.4 else "1")


def earth_tower(frame):
    c = Canvas(W, H)
    pillar(c)
    bob = [0, -1, -2, -1][frame]
    c.blit(CRYSTAL, 21, 9)
    spire(c, 20, 7, 20, 5)
    spire(c, 11, 9, 20, 5)
    spire(c, 15, 2, 20, 7)
    c.blit(CRYSTAL, 9, 13)
    c.blit(CRYSTAL, 17, 14)
    # Pebbles float out of phase on each side
    c.orb(5, 12 + bob, 1.6)
    c.orb(27, 10 - bob - 2, 1.6)
    c.orb(25, 4 + bob, 1.1)
    return c


# ── Wind: rotating swirl around a light orb ──────────────────────────────────

def wind_tower(frame):
    c = Canvas(W, H)
    pillar(c)
    c.blit(PRONGS, 7, 17)
    cx, cy = 16, 10
    rot = frame * math.pi / 6           # 4 frames x 30 deg = 120 deg = one period
    for arm in range(3):
        start = rot + arm * 2 * math.pi / 3
        for step in range(14):
            a = start + step * 0.13
            r = 4 + step * 0.32
            x = round(cx + math.cos(a) * r * 1.15)
            y = round(cy + math.sin(a) * r * 0.85)
            c.set(x, y, "4" if step > 9 else "3" if step > 4 else "a")
        # Leaf riding the wind at the tip
        a = start + 14 * 0.13
        c.set(round(cx + math.cos(a) * 9.6), round(cy + math.sin(a) * 7.1), "b")
    c.orb(cx, cy, 2.6, ramp="a344")
    return c


# ── Normal: wooden parapet + crossbow ────────────────────────────────────────

def crossbow_arm(c, x, y0, y1, bend):
    """Steel bow arm from (x, y0) to (x, y1), bent forward by `bend` pixels."""
    mid = (y0 + y1) / 2
    for y in range(y0, y1 + 1):
        t = abs(y - mid) / ((y1 - y0) / 2)
        bx = round(x + bend * (1 - t * t))
        c.set(bx - 1, y, "O")
        c.set(bx, y, "b" if y < mid else "a")
        c.set(bx + 1, y, "O")


def normal_tower(frame):
    c = Canvas(W, H)
    pillar(c)
    # Wooden parapet with crenels
    c.rect(5, 15, 22, 7, "O")
    for y in range(16, 21):
        c.rect(6, y, 20, 1, "3" if y == 16 else "2" if y < 20 else "1")
    for x in range(9, 25, 5):
        c.rect(x, 16, 1, 5, "1")                     # plank joints
    for x in (5, 12, 19):
        c.rect(x, 11, 8 if x == 19 else 6, 5, "O")    # crenels
        c.rect(x + 1, 12, (6 if x == 19 else 4), 3, "3")
        c.rect(x + 1, 12, 1, 3, "4")
    # Crossbow: the string is pulled back, then the bolt is shot (frame 3).
    pull = [0, 2, 4, 0][frame]
    c.rect(8, 8, 15, 3, "O")                          # stock
    c.rect(9, 9, 13, 1, "3")
    crossbow_arm(c, 22, 1, 15, 2)
    for y in range(2, 15):                            # string, to the pulled point
        t = abs(y - 8) / 6
        sx = round(21 - pull * (1 - t))
        c.set(sx, y, "4")
    if frame < 3:
        c.rect(21 - pull, 7, 9, 1, "k")               # loaded bolt
        c.set(30 - pull, 7, "b")
        c.set(29 - pull, 6, "a")
        c.set(29 - pull, 8, "a")
    else:
        c.rect(26, 7, 5, 1, "k")                      # bolt leaving
        c.set(31, 7, "b")
    return c


TOWERS = {
    "normal": normal_tower,
    "fire": fire_tower,
    "water": water_tower,
    "earth": earth_tower,
    "wind": wind_tower,
}

ELEMENT_OF = {name: name for name in TOWERS}
