"""Preview only: lays tiles out on a sample perfect-maze map.

The real generator lives in the game (src/map/MapGenerator.cpp); this is
a small Python copy of the same rules so the tiles can be checked in
context without running the game.
"""

import random

import tiles

PATH, BUILD, BLOCK = "path", "buildable", "obstacle"
OBSTACLE_OBJECT_PERCENT = 45  # same rule as src/map/MapRenderer.cpp


def mix(x, y, salt=0):
    """Well-mixed integer hash of a tile position (variant picking)."""
    h = (x * 0x27D4EB2D) ^ (y * 0x165667B1) ^ (salt * 0x9E3779B9)
    h &= 0xFFFFFFFF
    h ^= h >> 15
    h = (h * 0x2C1B3C6D) & 0xFFFFFFFF
    h ^= h >> 12
    h = (h * 0x297A2D39) & 0xFFFFFFFF
    h ^= h >> 15
    return h


def sample_map(cols, rows, seed, reach=2):
    rng = random.Random(seed)
    cw, ch = cols // 2, rows // 2
    grid = [[BLOCK] * cols for _ in range(rows)]
    seen = {(0, 0)}
    stack = [(0, 0)]
    grid[1][1] = BUILD
    while stack:  # recursive backtracker
        cx, cy = stack[-1]
        nbrs = [(cx + dx, cy + dy) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))
                if 0 <= cx + dx < cw and 0 <= cy + dy < ch and (cx + dx, cy + dy) not in seen]
        if not nbrs:
            stack.pop()
            continue
        nx, ny = rng.choice(nbrs)
        grid[cy + ny + 1][cx + nx + 1] = BUILD
        grid[2 * ny + 1][2 * nx + 1] = BUILD
        seen.add((nx, ny))
        stack.append((nx, ny))

    # Entry / exit: the left and right column cells farthest apart
    best = None
    for sy in range(1, rows, 2):
        start = (1, sy)
        prev = {start: None}
        queue = [start]
        for x, y in queue:
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                p = (x + dx, y + dy)
                if 0 <= p[0] < cols and 0 <= p[1] < rows and grid[p[1]][p[0]] == BUILD and p not in prev:
                    prev[p] = (x, y)
                    queue.append(p)
        for ey in range(1, rows, 2):
            route = _walk(prev, (cols - 2, ey))
            if best is None or len(route) > len(best):
                best = route
    path = [(0, best[0][1])] + best + [(cols - 1, best[-1][1])]
    for x, y in path:
        grid[y][x] = PATH
    on_path = set(path)

    def near(x, y, r):
        return any((x + dx, y + dy) in on_path
                   for dx in range(-r, r + 1) for dy in range(-r, r + 1))

    for y in range(1, rows - 1):
        for x in range(1, cols - 1):
            if grid[y][x] == BUILD and not near(x, y, reach):
                grid[y][x] = BLOCK            # dead end far from the path
            elif grid[y][x] == BLOCK and rng.random() < 0.25 and near(x, y, 1):
                grid[y][x] = BUILD            # some walls along the path
    return grid, path


def _walk(prev, p):
    out = []
    while p:
        out.append(p)
        p = prev[p]
    return out[::-1]


def compose(sheet, grid):
    """Returns rows of Canvas, one per map tile."""
    rows, cols = len(grid), len(grid[0])
    out = []
    for y in range(rows):
        line = []
        for x in range(cols):
            kind = grid[y][x]
            if kind == PATH:
                mask = 0
                for bit, dx, dy in ((tiles.N, 0, -1), (tiles.E, 1, 0), (tiles.S, 0, 1), (tiles.W, -1, 0)):
                    nx, ny = x + dx, y + dy
                    outside = not (0 <= nx < cols and 0 <= ny < rows)
                    if (outside and (x in (0, cols - 1)) and dy == 0) or \
                            (not outside and grid[ny][nx] == PATH):
                        mask |= bit
                line.append(sheet[0][mask])
            else:
                if kind == BLOCK and mix(x, y, 1) % 100 >= OBSTACLE_OBJECT_PERCENT:
                    kind = "ground"           # plain, non-buildable ground
                row, col = tiles.LAYOUT[kind]
                variant = mix(x, y) % tiles.VARIANTS
                line.append(sheet[row][col + variant])
        out.append(line)
    return out
