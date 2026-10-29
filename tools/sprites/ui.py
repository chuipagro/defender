"""UI sprites: a 5x7 pixel font and 16x16 icons.

font.png   16 glyphs per row, cells of 6x8 (5x7 glyph + spacing, row 7 for
           the cedilla). Order: ASCII 32..126, then FONT_EXTRA.
icons.png  16x16 icons side by side, in the order of ICONS (must match
           enum class Icon in include/defender/ui/Icon.hpp).
"""

import math

from canvas import Canvas

# ── Font ──────────────────────────────────────────────────────────────────────

FONT_CELL_W, FONT_CELL_H = 6, 8
FONT_COLUMNS = 16

# Classic 5x7 font, column-encoded: one byte per column, bit 0 = top row.
ASCII_GLYPHS = [
    "0000000000", "00005F0000", "0007000700", "147F147F14", "242A7F2A12",  # space ! " # $
    "2313086462", "3649552250", "0005030000", "001C224100", "0041221C00",  # % & ' ( )
    "082A1C2A08", "08083E0808", "0050300000", "0808080808", "0060600000",  # * + , - .
    "2010080402", "3E5149453E", "00427F4000", "4261514946", "2141454B31",  # / 0 1 2 3
    "1814127F10", "2745454539", "3C4A494930", "0171090503", "3649494936",  # 4 5 6 7 8
    "064949291E", "0036360000", "0056360000", "0008142241", "1414141414",  # 9 : ; < =
    "4122140800", "0201510906", "324979413E", "7E1111117E", "7F49494936",  # > ? @ A B
    "3E41414122", "7F4141221C", "7F49494941", "7F09090101", "3E41415132",  # C D E F G
    "7F0808087F", "00417F4100", "2040413F01", "7F08142241", "7F40404040",  # H I J K L
    "7F0204027F", "7F0408107F", "3E4141413E", "7F09090906", "3E4151215E",  # M N O P Q
    "7F09192946", "4649494931", "01017F0101", "3F4040403F", "1F2040201F",  # R S T U V
    "7F2018207F", "6314081463", "0304780403", "6151494543", "00007F4141",  # W X Y Z [
    "0204081020", "41417F0000", "0402010204", "4040404040", "0001020400",  # \\ ] ^ _ `
    "2054545478", "7F48444438", "3844444420", "384444487F", "3854545418",  # a b c d e
    "087E090102", "18A4A4A478", "7F08040478", "00447D4000", "2040443D00",  # f g h i j
    "007F102844", "00417F4000", "7C04180478", "7C08040478", "3844444438",  # k l m n o
    "FC24242418", "18242424FC", "7C08040408", "4854545420", "043F444020",  # p q r s t
    "3C4040207C", "1C2040201C", "3C4030403C", "4428102844", "1CA0A0A07C",  # u v w x y
    "4464544C44", "0008364100", "00007F0000", "0041360800", "0201020402",  # z { | } ~
]

# Extra glyphs for French text (index 95 onwards, same order as in C++).
FONT_EXTRA = "éèêàâçùûôî"
EXTRA_GLYPHS = [
    "3854565518",  # é
    "3855565418",  # è
    "3856555618",  # ê
    "2055565478",  # à
    "2056555678",  # â
    "384444C420",  # ç (bit 7 = cedilla)
    "3C4142207C",  # ù
    "3C4241227C",  # û
    "3846454638",  # ô
    "00467D4200",  # î
]


def font_sheet():
    """Returns a Canvas with every glyph in white ('w')."""
    glyphs = ASCII_GLYPHS + EXTRA_GLYPHS
    rows = math.ceil(len(glyphs) / FONT_COLUMNS)
    c = Canvas(FONT_COLUMNS * FONT_CELL_W, rows * FONT_CELL_H)
    for index, code in enumerate(glyphs):
        gx = (index % FONT_COLUMNS) * FONT_CELL_W
        gy = (index // FONT_COLUMNS) * FONT_CELL_H
        for col in range(5):
            bits = int(code[col * 2:col * 2 + 2], 16)
            for row in range(8):
                if bits & (1 << row):
                    c.set(gx + col, gy + row, "w")
    return c


# ── Icons ─────────────────────────────────────────────────────────────────────

ICON = 16

UI_PALETTE = {
    ".": None,
    "K": "#1b1622",                                                         # outline
    "g": "#9a5f12", "G": "#d08a1a", "y": "#f5c542", "Y": "#fff1a0",          # gold
    "r": "#8a1626", "R": "#d8344a", "p": "#f2707e", "P": "#ffc0c8",          # red
    "b": "#1f3f8a", "B": "#3a72d8", "c": "#7fb2f5", "C": "#d0e6ff",          # blue
    "m": "#4a4a58", "M": "#7a7a8a", "n": "#b0b0bc", "N": "#f0f0f4",          # grey
    "e": "#1f6a2a", "E": "#3aa040", "h": "#7ad860", "H": "#d0ffb0",          # green
    "d": "#5a3418", "D": "#8a5a2e", "q": "#d8ae76", "Q": "#f5dcb0",          # wood
    "w": "#ffffff",
}

GOLD, RED, BLUE, GREY, GREEN, WOOD = "gGyY", "rRpP", "bBcC", "mMnN", "eEhH", "dDqQ"


def fill(c, inside, ramp, outline="K"):
    """Fills the cells where inside(x, y) is true (tested at the cell center),
    lit from the top-left, with an outline around."""
    cells = {(x, y) for y in range(ICON) for x in range(ICON) if inside(x + 0.5, y + 0.5)}
    if not cells:
        return
    xs = [x for x, _ in cells]
    ys = [y for _, y in cells]
    x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
    for x, y in cells:
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            if (x + dx, y + dy) not in cells:
                c.set(x + dx, y + dy, outline)
    for x, y in cells:
        t = ((x - x0) / max(1, x1 - x0) + (y - y0) / max(1, y1 - y0)) / 2  # 0 = top-left
        c.set(x, y, ramp[3] if t < 0.15 else ramp[2] if t < 0.45 else ramp[1] if t < 0.8 else ramp[0])


def polygon(points):
    """inside() test for a polygon (even-odd rule)."""
    def inside(x, y):
        hit = False
        for i in range(len(points)):
            (ax, ay), (bx, by) = points[i], points[i - 1]
            if (ay > y) != (by > y) and x < (bx - ax) * (y - ay) / (by - ay) + ax:
                hit = not hit
        return hit
    return inside


def circle(cx, cy, r):
    return lambda x, y: (x - cx) ** 2 + (y - cy) ** 2 <= r * r


def coin():
    c = Canvas(ICON, ICON)
    fill(c, circle(8, 8, 6.5), GOLD)
    for y in range(4, 12):                       # engraved bar
        c.set(8, y, "g")
    c.set(7, 4, "Y")
    c.set(7, 5, "Y")
    return c


def heart():
    c = Canvas(ICON, ICON)
    lobes = lambda x, y: circle(5, 6, 3.4)(x, y) or circle(11, 6, 3.4)(x, y)
    point = polygon([(1.6, 7), (14.4, 7), (8, 14.5)])
    fill(c, lambda x, y: lobes(x, y) or point(x, y), RED)
    c.set(4, 4, "P")
    c.set(4, 5, "P")
    return c


def shield():
    c = Canvas(ICON, ICON)

    def inside(x, y):
        if y < 1.5 or y > 14.8:
            return False
        if y < 8:
            return 2.5 <= x <= 13.5
        half = 5.5 * (1 - ((y - 8) / 6.8) ** 1.6)
        return abs(x - 8) <= half

    fill(c, inside, BLUE)
    for y in range(3, 13):                       # central stripe
        c.set(8, y, "C" if y < 8 else "c")
    return c


def star():
    c = Canvas(ICON, ICON)
    pts = []
    for i in range(10):
        r = 7.2 if i % 2 == 0 else 3.0
        a = -math.pi / 2 + i * math.pi / 5
        pts.append((8 + r * math.cos(a), 8.6 + r * math.sin(a)))
    fill(c, polygon(pts), GOLD)
    return c


def flag():
    c = Canvas(ICON, ICON)
    c.rect(2, 1, 3, 15, "K")                     # pole
    c.rect(3, 2, 1, 13, "n")
    c.rect(1, 14, 5, 2, "K")
    fill(c, lambda x, y: 4.5 < x < 14.5 and 2.5 + math.sin(x * 0.8) * 0.8 < y < 9 + math.sin(x * 0.8) * 0.8, RED)
    return c


def paw():
    c = Canvas(ICON, ICON)
    fill(c, lambda x, y: ((x - 8) / 4.2) ** 2 + ((y - 11) / 3.4) ** 2 <= 1, WOOD)
    for cx, cy in ((3.2, 7), (6.2, 3.8), (9.8, 3.8), (12.8, 7)):
        fill(c, circle(cx, cy, 1.9), WOOD)
    return c


def hourglass():
    c = Canvas(ICON, ICON)
    glass = polygon([(3.5, 2.5), (12.5, 2.5), (8.6, 8), (12.5, 13.5), (3.5, 13.5), (7.4, 8)])
    fill(c, glass, "MnNN")
    fill(c, polygon([(5.5, 13.5), (10.5, 13.5), (8, 10.5)]), GOLD, outline="n")   # sand
    c.set(8, 9, "y")
    c.set(8, 10, "y")
    c.rect(2, 1, 12, 2, "K")
    c.rect(3, 1, 10, 1, "q")
    c.rect(2, 13, 12, 2, "K")
    c.rect(3, 14, 10, 1, "D")
    return c


def fast_forward():
    c = Canvas(ICON, ICON)
    fill(c, lambda x, y: polygon([(1.5, 3), (8, 8), (1.5, 13)])(x, y)
         or polygon([(8, 3), (14.5, 8), (8, 13)])(x, y), GREY)
    return c


def play():
    c = Canvas(ICON, ICON)
    fill(c, polygon([(3.5, 2), (14, 8), (3.5, 14)]), GREEN)
    return c


def tree():
    c = Canvas(ICON, ICON)
    c.rect(6, 10, 4, 6, "K")
    c.rect(7, 10, 2, 5, "D")
    fill(c, lambda x, y: circle(8, 6, 4.6)(x, y) or circle(4.2, 9, 3)(x, y)
         or circle(11.8, 9, 3)(x, y), GREEN)
    for x, y in ((5, 5), (10, 7), (7, 9)):
        c.set(x, y, "y")                          # fruits
    return c


def lock():
    c = Canvas(ICON, ICON)
    ring = lambda x, y: 3.2 <= math.hypot(x - 8, y - 7) <= 5 and y < 8
    fill(c, ring, GREY)
    fill(c, lambda x, y: 2.5 < x < 13.5 and 7.5 < y < 14.5, GOLD)
    c.set(8, 10, "K")
    c.set(8, 11, "K")
    return c


def sword():
    c = Canvas(ICON, ICON)
    blade = polygon([(14.5, 1.5), (15, 3), (6.5, 11.5), (4.5, 9.5), (13, 1)])
    fill(c, blade, GREY)
    fill(c, polygon([(2, 8.5), (3.5, 7), (9, 12.5), (7.5, 14)]), GOLD)  # guard
    fill(c, polygon([(1, 13.8), (4, 10.8), (5.2, 12), (2.2, 15)]), WOOD)  # handle
    return c


def target():
    c = Canvas(ICON, ICON)
    fill(c, circle(8, 8, 7), RED)
    fill(c, circle(8, 8, 4.6), "nNNN", outline="R")
    fill(c, circle(8, 8, 2), RED, outline="N")
    return c


def bolt():
    c = Canvas(ICON, ICON)
    fill(c, polygon([(9.5, 0.5), (3, 9), (7.5, 9), (5.5, 15.5), (13, 6.5), (8.5, 6.5), (11, 0.5)]),
         GOLD)
    return c


def close():
    c = Canvas(ICON, ICON)
    fill(c, lambda x, y: (abs((x - 8) - (y - 8)) < 1.9 or abs((x - 8) + (y - 8)) < 1.9)
         and abs(x - 8) < 5.6 and abs(y - 8) < 5.6, RED)
    return c


def water_drop():
    c = Canvas(ICON, ICON)

    def inside(x, y):
        if y < 9.5:  # pointed top
            return abs(x - 8) <= (y - 1.5) * 0.62
        return (x - 8) ** 2 + (y - 9.8) ** 2 <= 5 ** 2

    fill(c, inside, BLUE)
    c.set(6, 9, "C")
    c.set(6, 10, "C")
    return c


def ember():
    c = Canvas(ICON, ICON)
    flame = polygon([(8, 0.8), (11, 5), (13.4, 9.5), (12.5, 13), (8, 15), (3.5, 13), (2.6, 9.5),
                     (5, 6.5), (6.4, 8.5)])
    fill(c, flame, "rRyY")
    fill(c, polygon([(8, 7), (10.4, 11), (8, 13.5), (5.6, 11)]), "yYww", outline="R")
    return c


def stone():
    c = Canvas(ICON, ICON)
    fill(c, polygon([(2, 12), (3.5, 6), (7, 3.5), (11.5, 4.5), (14, 9), (13, 13.5), (5, 14.5)]),
         GREY)
    fill(c, polygon([(8, 3), (10.2, 6), (8.8, 10.5), (6.6, 7)]), GREEN)
    return c


def feather():
    c = Canvas(ICON, ICON)

    def inside(x, y):
        # Leaf shape along the diagonal from bottom-left to top-right
        u = (x + (16 - y)) / 2          # along the quill
        v = (x - (16 - y)) / 1.4        # across
        return 2.5 < u < 14.5 and abs(v) < 3.2 * math.sin(math.pi * (u - 2.5) / 12)

    fill(c, inside, "MnNw")
    for i in range(3, 13):
        c.set(i, 16 - i, "M")           # quill
    c.set(2, 14, "D")
    c.set(1, 15, "D")
    return c


def upgrade():
    c = Canvas(ICON, ICON)
    fill(c, polygon([(8, 1), (14.5, 8), (10.5, 8), (10.5, 15), (5.5, 15), (5.5, 8), (1.5, 8)]),
         GREEN)
    return c


def check():
    c = Canvas(ICON, ICON)
    fill(c, polygon([(1.5, 8.5), (4, 6), (6.5, 8.5), (12.5, 2), (15, 4.5), (6.5, 13.5)]), GREEN)
    return c


ICONS = {
    "coin": coin,
    "heart": heart,
    "shield": shield,
    "star": star,
    "flag": flag,
    "paw": paw,
    "hourglass": hourglass,
    "fast_forward": fast_forward,
    "play": play,
    "tree": tree,
    "lock": lock,
    "sword": sword,
    "target": target,
    "bolt": bolt,
    "close": close,
    "check": check,
    "water_drop": water_drop,
    "ember": ember,
    "stone": stone,
    "feather": feather,
    "upgrade": upgrade,
}
