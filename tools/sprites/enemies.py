"""Enemy sprites: 32x32 frames, facing right, 4-frame animation.

Chibi style: big round head, small body, stubby legs, shiny eyes.
Round shapes are drawn with Canvas.blob (outlined, shaded ellipses),
curved parts (tails, arms, tentacles) with tube() along a curve, and
small details with key grids (see palettes.py for the keys).

Walkers get a leg cycle + body bob, jumpers a crouch/launch/air/land
cycle with a ground shadow that shrinks with height.
"""

from canvas import Canvas, grid

SIZE = 32
GROUND = 29  # lowest row of the feet; shadow sits right below
FRAMES = 4

BOB = [0, -1, 0, -1]
FAR = "1122"  # darker ramp for limbs on the far side


# ── Helpers ───────────────────────────────────────────────────────────────────

def curve(p0, p1, p2, n=10):
    """Points along a quadratic Bézier curve."""
    pts = []
    for i in range(n + 1):
        t = i / n
        x = (1 - t) ** 2 * p0[0] + 2 * (1 - t) * t * p1[0] + t * t * p2[0]
        y = (1 - t) ** 2 * p0[1] + 2 * (1 - t) * t * p1[1] + t * t * p2[1]
        pts.append((x, y))
    return pts


def tube(c, pts, r0, r1=None, ramp="1234"):
    """Outlined tube along points, radius going from r0 to r1."""
    r1 = r0 if r1 is None else r1
    n = max(1, len(pts) - 1)
    c.blob([(x, y, r0 + (r1 - r0) * i / n, r0 + (r1 - r0) * i / n)
            for i, (x, y) in enumerate(pts)], ramp=ramp, highlight=False)


def eye(c, x, y, tall=3):
    """Shiny black eye (2 wide) with a white glint."""
    c.rect(x, y, 2, tall, "k")
    c.set(x, y, "w")


def blush(c, x, y):
    c.set(x, y, "r")
    c.set(x + 1, y, "r")


def stubby_leg(c, x, y, ramp="1234"):
    c.blob([(x + 0.5, y, 1.7, 2.6)], ramp=ramp, highlight=False)


def walk_legs(frame, back, front):
    """[(x, lift, far)] for a 4-frame diagonal walk cycle."""
    shift = [(-1, 1), (0, 0), (1, -1), (0, 0)][frame]
    lift = [(0, 0), (1, 0), (0, 0), (0, 1)][frame]
    return [
        (back[1] + shift[1], lift[1], True),
        (front[1] + shift[0], lift[0], True),
        (back[0] + shift[0], lift[0], False),
        (front[0] + shift[1], lift[1], False),
    ]


# ── Cat ───────────────────────────────────────────────────────────────────────

CAT_EAR = grid("""
    OO...
    O3O..
    O3rO.
    O3rrO
    O333O
""")


def cat(frame):
    c = Canvas(SIZE, SIZE)
    dy = BOB[frame]
    c.ground_shadow(16, GROUND + 1, 11)

    sway = [0, 1, 2, 1][frame]
    tube(c, curve((7, 21 + dy), (0, 18 + dy), (3 + sway, 9 + dy)), 1.4, 1.1)
    for x, lift, far in walk_legs(frame, (8, 11), (15, 18)):
        stubby_leg(c, x, 26.5 - lift, FAR if far else "1234")
    c.blob([(13, 22 + dy, 8, 4.8)])
    c.ellipse(16, 24.6 + dy, 4, 1.3, "3")                 # light belly
    for x in (9, 12):                                       # back stripes
        c.rect(x, 18 + dy, 1, 2, "1")

    ear = 1 if frame == 3 else 0                            # ear twitch
    c.blit(CAT_EAR, 13, 3 + dy + ear)
    c.blit(CAT_EAR, 24, 3 + dy, flip=True)
    c.blob([(21, 14 + dy, 8.5, 6.8)])
    for x in (19, 21, 23):                                  # forehead stripes
        c.rect(x, 8 + dy, 1, 2, "1")
    c.ellipse(21.5, 17.6 + dy, 3.2, 1.8, "4")               # muzzle
    eye(c, 17, 12 + dy)
    eye(c, 24, 12 + dy)
    blush(c, 15, 16 + dy)
    blush(c, 27, 16 + dy)
    c.set(21, 16 + dy, "r")                                 # nose
    c.set(22, 16 + dy, "r")
    c.set(20, 18 + dy, "O")                                 # ω mouth
    c.set(21, 17 + dy, "O")
    c.set(22, 17 + dy, "O")
    c.set(23, 18 + dy, "O")
    return c


# ── Dog ───────────────────────────────────────────────────────────────────────

def dog(frame):
    c = Canvas(SIZE, SIZE)
    dy = BOB[frame]
    c.ground_shadow(16, GROUND + 1, 11)

    wag = [3, 0, 3, 0][frame]                             # fast wag
    tube(c, curve((6, 20 + dy), (2, 17 + dy), (2 + wag, 12 + dy), 8), 1.4, 1.2)
    for x, lift, far in walk_legs(frame, (7, 10), (14, 17)):
        stubby_leg(c, x, 26.5 - lift, FAR if far else "1234")
    c.blob([(12, 22 + dy, 8, 4.8)])
    c.ellipse(9, 20.5 + dy, 2.5, 1.6, "1")                  # spot on the back
    c.ellipse(15, 24.6 + dy, 4, 1.3, "3")

    flop = 1 if dy else 0                                   # ears bounce
    c.blob([(26.5, 11 + dy, 2.4, 4.2)], ramp=FAR, highlight=False)  # far ear
    c.blob([(20, 13 + dy, 8, 6.5)])
    c.blob([(26.5, 16.5 + dy, 3.8, 2.6)], ramp="2344")     # snout
    c.ellipse(18.5, 12.5 + dy, 2.6, 2.6, "1")               # eye patch
    eye(c, 17, 11 + dy)
    eye(c, 23, 11 + dy)
    blush(c, 15, 16 + dy)
    c.rect(29, 15 + dy, 2, 2, "k")                          # nose
    c.set(29, 15 + dy, "w")
    c.rect(26, 19 + dy, 2, 2, "r")                          # tongue
    c.set(26, 21 + dy, "O")
    c.set(27, 21 + dy, "O")
    c.blob([(13.5, 13 + dy + flop, 2.6, 5)], ramp=FAR, highlight=False)  # near ear
    return c


# ── Orangutan ────────────────────────────────────────────────────────────────

def orangutan(frame):
    c = Canvas(SIZE, SIZE)
    dy = BOB[frame]
    c.ground_shadow(16, GROUND + 1, 12)
    swing = [-2, 0, 2, 0][frame]
    lift = [(0, 0), (1, 0), (0, 0), (0, 1)][frame]

    # Far arm (behind everything), reaching forward to the ground
    tube(c, curve((15, 17 + dy), (22, 19 + dy), (22 - swing, 26), 6), 1.7, 1.5, ramp=FAR)
    c.blob([(22.5 - swing, 27.2, 1.9, 1.4)], ramp="FFff", highlight=False)
    stubby_leg(c, 6 + swing // 2, 26.5 - lift[1], FAR)
    stubby_leg(c, 10 - swing // 2, 26.5 - lift[0])

    c.blob([(10, 21 + dy, 7, 5.5)])                          # round hairy body
    for x in range(5, 16, 3):                                # shaggy fringe
        c.set(x, 25 + dy + (x % 2), "1")
        c.set(x + 1, 24 + dy, "1")

    # Head: hair tufts, cheek pads, big face
    for hx, hy in ((14.5, 5.2), (17.5, 4.4), (20.5, 5.2)):
        c.blob([(hx, hy + dy, 1.4, 2.2)], highlight=False)
    c.blob([(17.5, 11 + dy, 7.5, 6.5)])
    c.blob([(11.5, 13 + dy, 2.4, 3.4), (23.5, 13 + dy, 2.4, 3.4)], ramp="FFFf", highlight=False)
    c.ellipse(17.5, 12.6 + dy, 5.6, 4.9, "F")                # face
    c.ellipse(17.5, 15 + dy, 3.4, 2.1, "f")                  # muzzle
    eye(c, 14, 10 + dy)
    eye(c, 19, 10 + dy)
    blush(c, 12, 14 + dy)
    blush(c, 22, 14 + dy)
    c.set(17, 14 + dy, "k")                                  # nostrils
    c.set(18, 14 + dy, "k")
    c.rect(16, 16 + dy, 4, 1, "O")                           # grin
    c.set(15, 15 + dy, "O")
    c.set(20, 15 + dy, "O")

    # Near arm, in front, swinging forward to the ground
    tube(c, curve((12, 18 + dy), (19, 21 + dy), (18 + swing, 26), 6), 1.9, 1.7)
    c.blob([(18.5 + swing, 27.2, 2.1, 1.5)], ramp="FFff", highlight=False)
    return c


# ── Duck ─────────────────────────────────────────────────────────────────────

DUCK_FOOT = grid("""
    .OaO.
    OaaaO
    OOOOO
""")


def duck(frame):
    c = Canvas(SIZE, SIZE)
    sway = [0, 1, 0, -1][frame]                              # waddle
    lift = [(0, 0), (2, 0), (0, 0), (0, 2)][frame]
    dy = BOB[frame]
    c.ground_shadow(15, GROUND + 1, 10)

    c.blit(DUCK_FOOT, 9, GROUND - 2 - lift[0])
    c.blit(DUCK_FOOT, 15, GROUND - 2 - lift[1])
    c.blob([(5 + sway, 16 + dy, 2.6, 2.2)], highlight=False)  # tail tuft
    c.blob([(13 + sway, 21 + dy, 9, 5.8)])                     # body
    c.ellipse(16 + sway, 24 + dy, 5, 1.6, "3")
    flap = 1 if frame in (1, 3) else 0
    c.blob([(10.5 + sway, 20 + dy - flap, 5, 2.8)], ramp=FAR, highlight=False)  # wing
    c.set(7 + sway, 21 + dy - flap, "2")
    c.set(8 + sway, 22 + dy - flap, "2")

    c.blob([(21 + sway, 11 + dy, 6.2, 5.6)])                   # head
    c.blob([(27 + sway, 13.5 + dy, 3, 1.5)], ramp="aabb", highlight=False)  # beak
    c.set(26 + sway, 14 + dy, "O")
    eye(c, 22 + sway, 9 + dy)
    blush(c, 20 + sway, 13 + dy)
    return c


# ── Frog (jumper) ────────────────────────────────────────────────────────────

FROG_FOOT = grid("""
    OaaaaO
    OOOOOO
""")

JUMP = [  # (body dy, pose)
    (1, "crouch"),
    (-4, "stretch"),
    (-7, "tuck"),
    (-3, "stretch"),
]


def frog(frame):
    c = Canvas(SIZE, SIZE)
    dy, pose = JUMP[frame]
    c.ground_shadow(16, GROUND + 1, max(5, 11 + dy))
    by = 20 + dy  # body center

    if pose == "crouch":
        for x in (6, 26):
            c.blob([(x, by + 4, 4, 3.4)], highlight=False)
        c.blit(FROG_FOOT, 2, by + 7)
        c.blit(FROG_FOOT, 24, by + 7)
    elif pose == "stretch":
        for x in (9, 23):
            c.blob([(x, by + 7, 2.4, 4)], highlight=False)
            c.blit(FROG_FOOT, x - 3, by + 10)
    else:
        for x in (8, 24):
            c.blob([(x, by + 5, 3, 2.4)], highlight=False)

    c.blob([(16, by, 10, 6.5)])                                # body
    c.ellipse(16, by + 3.5, 6.5, 2, "4")                       # light belly
    for ex in (10, 22):                                        # eye bulbs
        c.blob([(ex, by - 6, 3.8, 3.6)])
        c.ellipse(ex, by - 5.6, 2.6, 2.4, "w")
        c.rect(ex, by - 7, 2, 3, "k")
        c.set(ex, by - 7, "w")
    blush(c, 7, by)
    blush(c, 24, by)
    for x in range(11, 22):                                    # big smile
        c.set(x, by + (1 if 12 < x < 20 else 0), "O")
    for x in (13, 18):
        c.set(x, by - 2, "1")                                  # nostrils
    return c


# ── Octopus (jumper) ─────────────────────────────────────────────────────────

def octopus(frame):
    c = Canvas(SIZE, SIZE)
    dy, pose = JUMP[frame]
    c.ground_shadow(16, GROUND + 1, max(5, 10 + dy))
    hy = 16 + dy  # head center

    # 5 tentacles: (root x, outward direction)
    for i, (rx, d) in enumerate(((9, -1), (12.5, -0.5), (16, 0), (19.5, 0.5), (23, 1))):
        far = i in (1, 3)
        root = (rx, hy + 5)
        if pose == "crouch":       # splayed on the ground, tips curled up
            mid, tip = (rx + d * 4, hy + 13), (rx + d * 7, hy + 9 - abs(d) * 2)
        elif pose == "stretch":    # trailing down, a little wavy
            wave = 1 if (i + frame) % 2 else -1
            mid, tip = (rx + wave, hy + 10), (rx + d * 2, hy + 14)
        else:                      # tucked under the head
            mid, tip = (rx + d * 5, hy + 8), (rx + d * 2, hy + 6)
        pts = curve(root, mid, tip, 8)
        tube(c, pts, 1.8, 1.0, ramp=FAR if far else "1234")
        if not far:                # suckers on the underside
            for x, y in pts[3:7:2]:
                c.set(round(x), round(y) + 1, "a")

    c.blob([(16, hy, 9, 7.6)])                                 # head
    for sx, sy in ((11, hy - 4), (20, hy - 5), (22, hy - 2)):  # spots
        c.set(sx, sy, "3")
        c.set(sx + 1, sy, "3")
    eye(c, 11, hy, tall=3)
    eye(c, 19, hy, tall=3)
    blush(c, 9, hy + 3)
    blush(c, 21, hy + 3)
    c.set(15, hy + 3, "O")                                     # little "u" mouth
    c.set(16, hy + 4, "O")
    c.set(17, hy + 3, "O")
    return c


ENEMIES = {
    "octopus": octopus,
    "orangutan": orangutan,
    "cat": cat,
    "dog": dog,
    "duck": duck,
    "frog": frog,
}
