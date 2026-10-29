"""Projectile sprites: 16x16 frames, 4-frame animation, flying to the right.

The game rotates arrows, fireballs and gusts toward their target; bubbles and
rocks are drawn as they are. PROJECTILES maps a name to (builder, palette):
the palette is an element name, or "normal" for the natural (wood) colors.
"""

import math

from canvas import Canvas

SIZE = 16
FRAMES = 4


def arrow(frame):
    c = Canvas(SIZE, SIZE)
    y = 7
    # Fletching (left), shaft, steel head (right)
    for i, (dx, dy) in enumerate(((0, -2), (1, -1), (0, 2), (1, 1))):
        c.set(1 + dx, y + dy, "4" if i < 2 else "3")
    c.set(2, y - 2, "4")
    c.set(2, y + 2, "3")
    c.rect(1, y, 11, 1, "2")
    c.rect(2, y - 1, 9, 1, "O")
    c.rect(2, y + 1, 9, 1, "O")
    c.rect(3, y, 8, 1, "3")
    head = [(11, -2), (12, -1), (13, 0), (12, 1), (11, 2)]
    for x, dy in head:
        c.set(x, y + dy, "O")
    c.rect(11, y - 1, 2, 3, "a")
    c.set(11, y - 1, "b")
    c.set(12, y, "b")
    c.set(14, y, "O")
    if frame % 2:                       # glint running on the head
        c.set(12, y - 1, "b")
    return c


def bubble(frame):
    c = Canvas(SIZE, SIZE)
    wobble = [(4.6, 4.6), (5.0, 4.2), (4.6, 4.6), (4.2, 5.0)][frame]
    c.blob([(8, 8, wobble[0], wobble[1])], ramp="1234")
    c.ellipse(8, 8, wobble[0] - 1.6, wobble[1] - 1.6, "3")
    c.set(6, 5, "w")
    c.set(5, 6, "w")
    c.set(6, 6, "4")
    # Small droplet trailing behind
    c.set(1 + frame % 2, 9, "3")
    return c


def fireball(frame):
    c = Canvas(SIZE, SIZE)
    phase = frame * math.pi / 2
    # Flame tail to the left, flickering
    for i in range(7):
        x = 8 - i
        half = 3.6 * (1 - i / 8) + 0.6 * math.sin(phase + i)
        for dy in range(-int(half), int(half) + 1):
            c.set(x, 8 + dy + round(math.sin(phase + i * 0.9) * 0.6),
                  "2" if abs(dy) > half * 0.6 else "3")
    c.orb(10, 8, 3.6, ramp="34bb")
    c.set(11, 6, "w")
    return c


def rock(frame):
    c = Canvas(SIZE, SIZE)
    angle = frame * math.pi / 8
    radii = [5.2, 4.2, 5.6, 4.6, 5.0, 3.8, 5.4, 4.4]
    pts = []
    for i, r in enumerate(radii):
        a = angle + i * 2 * math.pi / len(radii)
        pts.append((8 + r * math.cos(a), 8 + r * math.sin(a)))

    def inside(x, y):
        hit = False
        for i in range(len(pts)):
            (ax, ay), (bx, by) = pts[i], pts[i - 1]
            if (ay > y) != (by > y) and x < (bx - ax) * (y - ay) / (by - ay) + ax:
                hit = not hit
        return hit

    cells = {(x, y) for y in range(SIZE) for x in range(SIZE) if inside(x + 0.5, y + 0.5)}
    for x, y in cells:
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            if (x + dx, y + dy) not in cells:
                c.set(x + dx, y + dy, "O")
    for x, y in cells:
        t = ((x - 3) + (y - 3)) / 20
        c.set(x, y, "4" if t < 0.25 else "3" if t < 0.5 else "2" if t < 0.8 else "1")
    # Moss crystal turning with the rock
    a = angle + 0.6
    c.set(round(8 + 2 * math.cos(a)), round(8 + 2 * math.sin(a)), "b")
    c.set(round(8 + 2 * math.cos(a)) + 1, round(8 + 2 * math.sin(a)), "a")
    return c


def gust(frame):
    c = Canvas(SIZE, SIZE)
    # Three crescents opening to the left, swirling
    for k, (cx, r, key) in enumerate(((10, 5.5, "4"), (7, 4.2, "3"), (4, 3.0, "a"))):
        spin = frame * 0.35 + k * 0.4
        for step in range(13):
            a = -1.2 + step * 0.2 + math.sin(spin) * 0.15
            x = round(cx + math.cos(a) * r * 0.7)
            y = round(8 + math.sin(a) * r)
            c.set(x, y, key)
            c.set(x - 1, y, "O" if k == 0 and step in (0, 12) else key if k == 0 else ".")
    c.set(13, 8, "b")
    return c


PROJECTILES = {
    "arrow": (arrow, "normal"),
    "bubble": (bubble, "water"),
    "fireball": (fireball, "fire"),
    "rock": (rock, "earth"),
    "gust": (gust, "wind"),
}
