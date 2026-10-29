"""Tiny key-based canvas and a dependency-free PNG writer."""

import struct
import zlib


def grid(text):
    """Turns a multi-line string into a list of rows (blank lines removed).

    Rows are right-padded with '.' to the longest row, and a common
    leading indentation is stripped, so art can be indented in code.
    """
    rows = [r for r in text.split("\n") if r.strip()]
    indent = min(len(r) - len(r.lstrip(" ")) for r in rows)
    rows = [r[indent:].rstrip() for r in rows]
    width = max(len(r) for r in rows)
    return [r.replace(" ", ".").ljust(width, ".") for r in rows]


class Canvas:
    def __init__(self, width, height):
        self.w = width
        self.h = height
        self.px = [["."] * width for _ in range(height)]

    def set(self, x, y, key):
        if 0 <= x < self.w and 0 <= y < self.h and key != ".":
            self.px[y][x] = key

    def blit(self, rows, x, y, flip=False):
        """Draws a grid; '.' is transparent. flip mirrors it horizontally."""
        for r, row in enumerate(rows):
            if flip:
                row = row[::-1]
            for c, key in enumerate(row):
                self.set(x + c, y + r, key)

    def rect(self, x, y, w, h, key):
        for j in range(y, y + h):
            for i in range(x, x + w):
                self.set(i, j, key)

    def ellipse(self, cx, cy, rx, ry, key):
        for j in range(int(cy - ry) - 1, int(cy + ry) + 2):
            for i in range(int(cx - rx) - 1, int(cx + rx) + 2):
                if ((i + 0.5 - cx) / rx) ** 2 + ((j + 0.5 - cy) / ry) ** 2 <= 1:
                    self.set(i, j, key)

    def orb(self, cx, cy, r, outline="O", ramp="1234"):
        """Shaded sphere lit from the top-left, with an outline."""
        self.ellipse(cx, cy, r + 1, r + 1, outline)
        for j in range(int(cy - r) - 1, int(cy + r) + 2):
            for i in range(int(cx - r) - 1, int(cx + r) + 2):
                dx, dy = i + 0.5 - cx, j + 0.5 - cy
                d = (dx * dx + dy * dy) ** 0.5
                if d > r:
                    continue
                # Distance to the light spot sets the shade.
                light = ((dx + r * 0.35) ** 2 + (dy + r * 0.35) ** 2) ** 0.5 / (r * 1.6)
                shade = min(len(ramp) - 1, max(0, int((1 - light) * len(ramp))))
                self.set(i, j, ramp[shade])

    def blob(self, ellipses, ramp="1234", outline="O", highlight=True):
        """Union of ellipses (cx, cy, rx, ry), outlined and shaded top to bottom."""
        def inside(i, j):
            return any(((i + 0.5 - cx) / rx) ** 2 + ((j + 0.5 - cy) / ry) ** 2 <= 1
                       for cx, cy, rx, ry in ellipses)

        x0 = int(min(cx - rx for cx, _, rx, _ in ellipses)) - 2
        x1 = int(max(cx + rx for cx, _, rx, _ in ellipses)) + 2
        y0 = int(min(cy - ry for _, cy, _, ry in ellipses)) - 2
        y1 = int(max(cy + ry for _, cy, _, ry in ellipses)) + 2
        cells = {(i, j) for j in range(y0, y1 + 1) for i in range(x0, x1 + 1) if inside(i, j)}
        if not cells:
            return
        top = min(j for _, j in cells)
        bottom = max(j for _, j in cells)
        for i, j in cells:
            for di, dj in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                if (i + di, j + dj) not in cells:
                    self.set(i + di, j + dj, outline)
        for i, j in cells:
            t = (j - top) / max(1, bottom - top)
            self.set(i, j, ramp[2] if t < 0.3 else ramp[1] if t < 0.75 else ramp[0])
        if highlight:
            for cx, cy, rx, ry in ellipses:
                hx, hy = cx - rx * 0.35, cy - ry * 0.55
                for i, j in cells:
                    if ((i + 0.5 - hx) / max(1, rx * 0.28)) ** 2 + ((j + 0.5 - hy) / max(1, ry * 0.16)) ** 2 <= 1:
                        self.set(i, j, ramp[3])

    def ground_shadow(self, cx, y, half_width):
        for i in range(-half_width, half_width + 1):
            self.set(cx + i, y, "x")
        for i in range(-half_width + 2, half_width - 1):
            self.set(cx + i, y + 1, "x")


def render(canvases, palette):
    """Places canvases side by side and returns (w, h, RGBA rows)."""
    h = max(c.h for c in canvases)
    rows = []
    for y in range(h):
        row = []
        for c in canvases:
            for x in range(c.w):
                key = c.px[y][x] if y < c.h else "."
                if key not in palette:
                    raise KeyError(f"unknown color key {key!r}")
                row.append(palette[key])
        rows.append(row)
    return len(rows[0]), h, rows


def scale(rows, factor):
    out = []
    for row in rows:
        big = [p for p in row for _ in range(factor)]
        out.extend([big] * factor)
    return out


def write_png(path, rows):
    h, w = len(rows), len(rows[0])
    raw = b"".join(b"\x00" + b"".join(struct.pack("4B", *p) for p in row) for row in rows)

    def chunk(tag, data):
        body = tag + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)

    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw, 9))
    png += chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(png)
