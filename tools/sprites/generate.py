#!/usr/bin/env python3
"""Generates every sprite sheet from the key grids and the palettes.

Usage: python3 tools/sprites/generate.py [--out assets/sprites] [--preview DIR]

Output (one horizontal sprite sheet per file, frames left to right):
    <out>/enemies/<element>/<enemy>.png   4 frames of 32x32
    <out>/towers/<element>.png            4 frames of 32x48
    <out>/tiles/<biome>.png               16x2 tiles of 32x32 (see tiles.py)
    <out>/projectiles/<name>.png          4 frames of 16x16, flying right
    <out>/ui/font.png, <out>/ui/icons.png  pixel font and 16x16 icons (see ui.py)
<element> is "normal" (natural colors) or one of the elements.
With --preview, also writes zoomed overview images into DIR.
"""

import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from canvas import Canvas, render, scale, write_png  # noqa: E402
from palettes import ALL_ELEMENTS, BIOMES, hex_to_rgba, resolve, resolve_biome  # noqa: E402
import enemies  # noqa: E402
import map_preview  # noqa: E402
import projectiles  # noqa: E402
import tiles  # noqa: E402
import towers  # noqa: E402
import ui  # noqa: E402

PREVIEW_BG = (58, 63, 74, 255)
PREVIEW_ZOOM = 4


def sheet(builder, frames):
    return [builder(f) for f in range(frames)]


def save_sheet(path, canvases, palette):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    _, _, rows = render(canvases, palette)
    write_png(path, rows)


def render_grid(rows_of_canvases, palette):
    """Renders rows of canvases stacked vertically; returns RGBA rows."""
    out = []
    for line in rows_of_canvases:
        out.extend(render(line, palette)[2])
    return out


def preview(path, lines, gap=4):
    """lines: list of rows of (canvases, palette) groups, drawn on a dark background."""
    blocks = []
    for line in lines:
        rendered = [render(c, p)[2] for c, p in line]
        h = max(len(r) for r in rendered)
        out = [[] for _ in range(h)]
        for r in rendered:
            for y in range(h):
                src = r[y] if y < len(r) else [(0, 0, 0, 0)] * len(r[0])
                out[y].extend(src + [(0, 0, 0, 0)] * gap)
        blocks.append(out)
    width = max(len(b[0]) for b in blocks) + gap
    rows = [[PREVIEW_BG] * width for _ in range(gap)]
    for b in blocks:
        for row in b:
            full = [PREVIEW_BG] * gap
            for p in row:
                if p[3] == 255:
                    full.append(p)
                elif p[3] == 0:
                    full.append(PREVIEW_BG)
                else:  # blend the semi-transparent shadow
                    a = p[3] / 255
                    full.append(tuple(int(p[i] * a + PREVIEW_BG[i] * (1 - a)) for i in range(3)) + (255,))
            full += [PREVIEW_BG] * (width - len(full))
            rows.append(full)
        rows += [[PREVIEW_BG] * width for _ in range(gap)]
    write_png(path, scale(rows, PREVIEW_ZOOM))


def main():
    parser = argparse.ArgumentParser()
    root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    parser.add_argument("--out", default=os.path.join(root, "assets", "sprites"))
    parser.add_argument("--preview")
    args = parser.parse_args()

    count = 0
    for element in ALL_ELEMENTS:
        for name, builder in enemies.ENEMIES.items():
            save_sheet(os.path.join(args.out, "enemies", element, f"{name}.png"),
                       sheet(builder, enemies.FRAMES), resolve(element, name))
            count += 1
        for name, builder in towers.TOWERS.items():
            if towers.ELEMENT_OF.get(name, name) != element:
                continue
            save_sheet(os.path.join(args.out, "towers", f"{name}.png"),
                       sheet(builder, towers.FRAMES), resolve(element))
            count += 1
    for name, (builder, element) in projectiles.PROJECTILES.items():
        save_sheet(os.path.join(args.out, "projectiles", f"{name}.png"),
                   sheet(builder, projectiles.FRAMES), resolve(element))
        count += 1
    ui_palette = {k: hex_to_rgba(v) for k, v in ui.UI_PALETTE.items()}
    save_sheet(os.path.join(args.out, "ui", "font.png"), [ui.font_sheet()], ui_palette)
    icons = [builder() for builder in ui.ICONS.values()]
    save_sheet(os.path.join(args.out, "ui", "icons.png"), icons, ui_palette)
    count += 2
    for biome in BIOMES:
        path = os.path.join(args.out, "tiles", f"{biome}.png")
        os.makedirs(os.path.dirname(path), exist_ok=True)
        write_png(path, render_grid(tiles.biome_sheet(biome), resolve_biome(biome)))
        count += 1
    print(f"  wrote {count} sprite sheets to {os.path.relpath(args.out, root)}/")

    if args.preview:
        os.makedirs(args.preview, exist_ok=True)
        # Enemies: one row per enemy, every element side by side.
        lines = [[(sheet(b, enemies.FRAMES), resolve(e, n)) for e in ALL_ELEMENTS]
                 for n, b in enemies.ENEMIES.items()]
        preview(os.path.join(args.preview, "enemies.png"), lines)
        lines = [[(sheet(b, towers.FRAMES), resolve(towers.ELEMENT_OF.get(n, n)))
                  for n, b in towers.TOWERS.items()]]
        preview(os.path.join(args.preview, "towers.png"), lines)
        preview(os.path.join(args.preview, "projectiles.png"),
                [[(sheet(b, projectiles.FRAMES), resolve(e))
                  for b, e in projectiles.PROJECTILES.values()]])
        preview(os.path.join(args.preview, "ui.png"),
                [[(icons, ui_palette)], [([ui.font_sheet()], ui_palette)]])
        # Maps: the same sample maze in every biome, 2x2.
        grid, _ = map_preview.sample_map(25, 17, seed=42)
        maps = [render_grid(map_preview.compose(tiles.biome_sheet(b), grid), resolve_biome(b))
                for b in BIOMES]
        names = list(BIOMES)
        top = [a + b for a, b in zip(maps[0], maps[1])]
        bottom = [a + b for a, b in zip(maps[2], maps[3])]
        write_png(os.path.join(args.preview, "maps.png"), top + bottom)
        for b in names:
            sheet_rows = render_grid(tiles.biome_sheet(b), resolve_biome(b))
            write_png(os.path.join(args.preview, f"tiles_{b}.png"), scale(sheet_rows, 2))
        print(f"  wrote previews to {args.preview}/")


if __name__ == "__main__":
    main()
