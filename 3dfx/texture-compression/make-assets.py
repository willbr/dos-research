"""Original codec fixtures. No external images or image libraries required."""
import math
import struct
from pathlib import Path

GLYPHS = {
    "3": [30, 1, 1, 14, 1, 1, 30],
    "D": [30, 17, 17, 17, 17, 17, 30],
    "F": [31, 16, 16, 30, 16, 16, 16],
    "X": [17, 17, 10, 4, 10, 17, 17],
}


def make_image(kind):
    pixels = bytearray()
    for y in range(256):
        for x in range(256):
            u, v = x / 255, y / 255
            if kind == 0:
                rgb = [35 + 135 * u, 25 + 85 * v, 20 + 45 * (1 - u)]
            else:
                rgb = [15 + 55 * v, 55 + 120 * u, 105 + 125 * (1 - v)]
            nx, ny = (x - 128) / 82, (y - 120) / 82
            radius = nx * nx + ny * ny
            if radius < 1:
                nz = math.sqrt(1 - radius)
                light = max(0, -.30 * nx - .44 * ny + .84 * nz)
                base = (225, 135, 55) if kind == 0 else (55, 160, 235)
                shine = 165 * max(0, -.20 * nx - .30 * ny + .93 * nz) ** 24
                rgb = [c * (.20 + .80 * light) + shine for c in base]
            if y < 24:
                t = (x // 32) / 7
                rgb = [40 + 200 * t, 18 + 130 * t, 10 + 60 * t] if kind == 0 else [15 + 65 * t, 45 + 140 * t, 100 + 145 * t]
            if y >= 232:
                rgb = [x, x, x]
            if 32 <= y < 216 and x < 24:
                level = 220 if ((x // 4) ^ (y // 4)) & 1 else 20
                rgb = [level] * 3
            gx, gy = (x - 56) // 6, (y - 99) // 6
            if 0 <= gx < 24 and 0 <= gy < 7 and gx % 6 < 5:
                if GLYPHS["3DFX"[gx // 6]][gy] & (1 << (4 - gx % 6)):
                    rgb = [22, 22, 22] if kind == 0 else [225, 235, 240]
            r, g, b = (max(0, min(255, int(c + .5))) for c in rgb)
            pixels.extend((b, g, r))
    # Uncompressed 24-bit, top-left TGA. TexUS reads this original source.
    header = struct.pack("<BBBHHBHHHHBB", 0, 0, 2, 0, 0, 0, 0, 0, 256, 256, 24, 0x20)
    return header + pixels


if __name__ == "__main__":
    destination = Path(__file__).parent / "assets"
    destination.mkdir(exist_ok=True)
    for index, name in enumerate(("A", "B")):
        (destination / f"{name}.TGA").write_bytes(make_image(index))
