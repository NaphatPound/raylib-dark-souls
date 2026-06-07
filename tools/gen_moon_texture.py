#!/usr/bin/env python3
"""Generate a procedural 8-bit grayscale moon surface PNG.

This script intentionally uses only the Python standard library so it works on
fresh Python installs without Pillow or numpy wheels.
"""

from __future__ import annotations

import math
import os
import random
import struct
import zlib
from array import array


WIDTH = 1024
HEIGHT = 1024
SEED = 447190
OUT_PATH = os.path.join("assets", "textures", "moon", "moon_surface.png")


def clamp(value: float, lo: float, hi: float) -> float:
    return lo if value < lo else hi if value > hi else value


def smoothstep(edge0: float, edge1: float, value: float) -> float:
    if edge0 == edge1:
        return 0.0
    t = clamp((value - edge0) / (edge1 - edge0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def build_value_grid(rng: random.Random, cells_x: int, cells_y: int) -> list[list[float]]:
    return [[rng.random() * 2.0 - 1.0 for _ in range(cells_x + 1)] for _ in range(cells_y + 1)]


def sample_grid(grid: list[list[float]], x: float, y: float) -> float:
    max_y = len(grid) - 2
    max_x = len(grid[0]) - 2
    gx = clamp(x, 0.0, float(max_x + 0.999999))
    gy = clamp(y, 0.0, float(max_y + 0.999999))
    ix = int(gx)
    iy = int(gy)
    tx = gx - ix
    ty = gy - iy
    sx = tx * tx * (3.0 - 2.0 * tx)
    sy = ty * ty * (3.0 - 2.0 * ty)

    a = grid[iy][ix] * (1.0 - sx) + grid[iy][ix + 1] * sx
    b = grid[iy + 1][ix] * (1.0 - sx) + grid[iy + 1][ix + 1] * sx
    return a * (1.0 - sy) + b * sy


def add_maria(surface: array, rng: random.Random, fine_grid: list[list[float]]) -> None:
    patches = [
        (0.31, 0.38, 0.23, 0.15, -0.4, 0.16),
        (0.64, 0.29, 0.18, 0.11, 0.7, 0.13),
        (0.72, 0.68, 0.22, 0.13, -0.2, 0.12),
        (0.43, 0.76, 0.15, 0.10, 0.5, 0.10),
    ]
    for cxn, cyn, rxn, ryn, angle, strength in patches:
        cx = cxn * WIDTH + rng.uniform(-24.0, 24.0)
        cy = cyn * HEIGHT + rng.uniform(-24.0, 24.0)
        rx = rxn * WIDTH * rng.uniform(0.85, 1.12)
        ry = ryn * HEIGHT * rng.uniform(0.85, 1.12)
        ca = math.cos(angle)
        sa = math.sin(angle)
        min_x = max(0, int(cx - rx * 1.3))
        max_x = min(WIDTH - 1, int(cx + rx * 1.3))
        min_y = max(0, int(cy - rx * 1.3))
        max_y = min(HEIGHT - 1, int(cy + rx * 1.3))

        for y in range(min_y, max_y + 1):
            row = y * WIDTH
            for x in range(min_x, max_x + 1):
                dx = x - cx
                dy = y - cy
                ex = (dx * ca + dy * sa) / rx
                ey = (-dx * sa + dy * ca) / ry
                dist = math.sqrt(ex * ex + ey * ey)
                if dist >= 1.18:
                    continue
                ragged = sample_grid(fine_grid, x / 64.0, y / 64.0) * 0.12
                mask = 1.0 - smoothstep(0.62 + ragged, 1.16 + ragged, dist)
                surface[row + x] -= strength * mask


def stamp_crater(
    surface: array,
    cx: float,
    cy: float,
    radius: float,
    rim_gain: float,
    floor_drop: float,
    peak: bool,
) -> None:
    extent = int(radius * 1.35) + 2
    min_x = max(0, int(cx - extent))
    max_x = min(WIDTH - 1, int(cx + extent))
    min_y = max(0, int(cy - extent))
    max_y = min(HEIGHT - 1, int(cy + extent))
    inv_r = 1.0 / radius

    for y in range(min_y, max_y + 1):
        row = y * WIDTH
        dy = y - cy
        for x in range(min_x, max_x + 1):
            dx = x - cx
            d = math.sqrt(dx * dx + dy * dy) * inv_r
            if d > 1.28:
                continue

            rim = math.exp(-((d - 1.0) / 0.075) ** 2) * rim_gain
            inner_shadow = (1.0 - smoothstep(0.10, 0.92, d)) * floor_drop
            outer_ejecta = math.exp(-((d - 1.10) / 0.24) ** 2) * rim_gain * 0.32
            wall_bright = math.exp(-((d - 0.82) / 0.16) ** 2) * rim_gain * 0.23
            delta = rim + outer_ejecta + wall_bright - inner_shadow

            if peak:
                delta += math.exp(-(d / 0.16) ** 2) * rim_gain * 0.55

            # Slight directional lighting gives rims a less synthetic look.
            light = 0.018 * (dx * 0.72 - dy * 0.42) * inv_r * math.exp(-((d - 0.95) / 0.33) ** 2)
            surface[row + x] += delta + light


def add_craters(surface: array, rng: random.Random) -> None:
    craters: list[tuple[float, float, float]] = []
    for _ in range(10):
        craters.append((rng.uniform(80, 944), rng.uniform(80, 944), rng.uniform(58, 128)))
    for _ in range(70):
        craters.append((rng.uniform(20, 1004), rng.uniform(20, 1004), rng.uniform(14, 46)))
    for _ in range(420):
        craters.append((rng.uniform(0, WIDTH), rng.uniform(0, HEIGHT), rng.uniform(2.0, 12.0)))

    craters.sort(key=lambda item: item[2], reverse=True)
    for cx, cy, radius in craters:
        scale = radius / 128.0
        rim_gain = rng.uniform(0.035, 0.085) * (0.65 + scale * 0.8)
        floor_drop = rng.uniform(0.025, 0.075) * (0.65 + scale * 0.65)
        stamp_crater(surface, cx, cy, radius, rim_gain, floor_drop, radius > 30 and rng.random() < 0.35)


def write_png_grayscale(path: str, pixels: bytes) -> None:
    def chunk(kind: bytes, data: bytes) -> bytes:
        return (
            struct.pack(">I", len(data))
            + kind
            + data
            + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)
        )

    raw = bytearray()
    stride = WIDTH
    for y in range(HEIGHT):
        raw.append(0)
        start = y * stride
        raw.extend(pixels[start : start + stride])

    ihdr = struct.pack(">IIBBBBB", WIDTH, HEIGHT, 8, 0, 0, 0, 0)
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", zlib.compress(bytes(raw), 9)) + chunk(b"IEND", b"")
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as handle:
        handle.write(png)


def generate() -> bytes:
    rng = random.Random(SEED)
    grids = [
        (build_value_grid(rng, 4, 4), 4.0, 0.085),
        (build_value_grid(rng, 8, 8), 8.0, 0.060),
        (build_value_grid(rng, 16, 16), 16.0, 0.040),
        (build_value_grid(rng, 32, 32), 32.0, 0.025),
        (build_value_grid(rng, 128, 128), 128.0, 0.016),
    ]
    grain_grid = build_value_grid(rng, 64, 64)
    surface = array("f", [0.0]) * (WIDTH * HEIGHT)

    for y in range(HEIGHT):
        ny = y / (HEIGHT - 1)
        row = y * WIDTH
        for x in range(WIDTH):
            nx = x / (WIDTH - 1)
            value = 0.68
            for grid, cells, amp in grids:
                value += sample_grid(grid, nx * cells, ny * cells) * amp
            value += rng.uniform(-0.018, 0.018)
            value += 0.045 * (0.55 - nx) + 0.026 * math.sin((nx * 1.7 + ny * 0.9) * math.tau)
            value += sample_grid(grain_grid, nx * 64.0, ny * 64.0) * 0.018
            surface[row + x] = value

    add_maria(surface, rng, grain_grid)
    add_craters(surface, rng)

    out = bytearray(WIDTH * HEIGHT)
    for i, value in enumerate(surface):
        value = clamp(value, 0.30, 1.0)
        # Gentle contrast curve while preserving the requested brightness range.
        value = 0.30 + ((value - 0.30) / 0.70) ** 0.92 * 0.70
        out[i] = int(clamp(value * 255.0, 77.0, 255.0) + 0.5)
    return bytes(out)


def main() -> None:
    pixels = generate()
    write_png_grayscale(OUT_PATH, pixels)
    size = os.path.getsize(OUT_PATH)
    print(f"Wrote {OUT_PATH} ({WIDTH}x{HEIGHT}, {size} bytes)")


if __name__ == "__main__":
    main()
