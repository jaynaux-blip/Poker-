"""Shirt prints for the MetaHuman default garment: repeating patterns and chest graphics, as masks.

    blender -b -P art/blender/prints.py

Each texture is a three-channel mask on black (R, G and B choose the garment material's print colors A, B and C;
the game picks the colors per person). Patterns tile exactly (Print1Map); graphics are single motifs
(PrintGraphicMap) and also carry their coverage as alpha, which is what the garment lays the graphic on with (an
opaque graphic paints its black over the whole shirt). Written to unreal/Art/Textures/Clothing/T_Print_*.png and T_Graphic_*.png, which the editor's
setup imports as linear masks.
"""
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import bpy  # noqa: E402
import numpy as np  # noqa: E402

from artkit import core  # noqa: E402
from artkit.sheet import Sheet  # noqa: E402

OUT = os.path.join(core.ROOT, 'unreal', 'Art', 'Textures', 'Clothing')
N = 512


def save(name, rgb, alpha=None):
    """rgb: HxWx3 floats 0..1 (row 0 is the top of the image), written as an 8-bit PNG exactly as given (linear masks);
    with alpha (HxW), an RGBA PNG."""
    import struct
    import zlib
    os.makedirs(OUT, exist_ok=True)
    h, w, _ = rgb.shape
    if alpha is not None:
        rgb = np.concatenate([rgb, alpha[..., None]], axis=2)
    data = (np.clip(rgb, 0.0, 1.0) * 255.0 + 0.5).astype(np.uint8)
    raw = b''.join(bytes([0]) + data[y].tobytes() for y in range(h))  # filter type 0 on every row

    def chunk(tag, body):
        return struct.pack('>I', len(body)) + tag + body + struct.pack('>I', zlib.crc32(tag + body) & 0xffffffff)
    signature = bytes([0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A])
    png = signature + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2 if alpha is None else 6, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b'')
    with open(os.path.join(OUT, name + '.png'), 'wb') as f:
        f.write(png)
    print('wrote', name)


def grid(n=N):
    y, x = np.mgrid[0:n, 0:n].astype(np.float32)
    return (x + 0.5) / n, (y + 0.5) / n


def soft(d, edge=0.004):
    """1 inside (d < 0), 0 outside, with a pixel's worth of antialiasing."""
    return np.clip(0.5 - d / edge, 0.0, 1.0)


def stripes(name, period, duty, accent=0.0):
    u, v = grid()
    f = (v * period) % 1.0
    band = soft(np.abs(f - duty * 0.5) - duty * 0.5, 0.5 * period / N * 2)
    rgb = np.zeros((N, N, 3), np.float32)
    rgb[..., 0] = band
    if accent > 0.0:
        rgb[..., 1] = soft(np.abs(f - (duty + (1.0 - duty) * 0.5)) - accent * 0.5, 0.5 * period / N * 2) * (1.0 - band)
    save(name, rgb)


def tartan(name):
    u, v = grid()
    def bands(t, period):
        f = (t * period) % 1.0
        wide = soft(np.abs(f - 0.25) - 0.17, 0.01)
        thin = soft(np.abs(f - 0.72) - 0.025, 0.006)
        return wide, thin
    wu, tu = bands(u, 4)
    wv, tv = bands(v, 4)
    rgb = np.zeros((N, N, 3), np.float32)
    either = np.maximum(wu, wv)
    both = wu * wv
    rgb[..., 0] = either * (1.0 - both)
    rgb[..., 2] = both
    rgb[..., 1] = np.maximum(tu, tv) * (1.0 - either)
    save(name, rgb)


def gingham(name, period=8):
    u, v = grid()
    a = soft(np.abs((u * period) % 1.0 - 0.25) - 0.25, 0.02)
    b = soft(np.abs((v * period) % 1.0 - 0.25) - 0.25, 0.02)
    rgb = np.zeros((N, N, 3), np.float32)
    rgb[..., 0] = a * (1.0 - b)
    rgb[..., 1] = b * (1.0 - a)
    rgb[..., 2] = a * b
    save(name, rgb)


def dots(name, period=8, r=0.17):
    u, v = grid()
    fu = (u * period) % 1.0 - 0.5
    fv = (v * period) % 1.0 - 0.5
    d = np.sqrt(fu * fu + fv * fv) - r
    rgb = np.zeros((N, N, 3), np.float32)
    rgb[..., 0] = soft(d, 0.03)
    save(name, rgb)


def periodic_noise(seed, octaves=4, base=3):
    """Smooth noise that tiles exactly: a sum of whole-number-frequency waves."""
    rng = np.random.default_rng(seed)
    u, v = grid()
    out = np.zeros_like(u)
    amp = 1.0
    for o in range(octaves):
        f = base * (2 ** o)
        for _ in range(6):
            kx, ky = rng.integers(-f, f + 1, size=2)
            ph = rng.uniform(0, 2 * math.pi)
            out += amp * np.sin(2 * math.pi * (kx * u + ky * v) + ph)
        amp *= 0.5
    return (out - out.min()) / (out.max() - out.min())


def camo(name):
    a = periodic_noise(11)
    b = periodic_noise(23)
    c = periodic_noise(37)
    rgb = np.zeros((N, N, 3), np.float32)
    ma = soft(0.36 - a, 0.02)
    mb = soft(0.40 - b, 0.02) * (1.0 - ma)
    mc = soft(0.40 - c, 0.02) * (1.0 - ma) * (1.0 - mb)
    rgb[..., 0], rgb[..., 1], rgb[..., 2] = ma, mb, mc
    save(name, rgb)


# The shirt clamps the graphic at its edges, so a graphic keeps this much black all round: the small mips of a
# distant shirt then sample only black outside the motif instead of tinting the whole garment.
MARGIN = 0.15


def graphic(name, draw, size=1024):
    """A chest motif drawn with the Sheet (pure red, green and blue), composited onto black inside a margin."""
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o)
    s = Sheet(100.0, 100.0)
    draw(s)
    inner = int(round(size * (1.0 - 2.0 * MARGIN)))
    color, _surface = s.render(name, inner, samples=16)
    img = bpy.data.images.load(color)
    w, h = img.size
    px = np.array(img.pixels[:], dtype=np.float32).reshape(h, w, 4)[::-1]
    rgb = px[..., :3] * px[..., 3:4]
    # The render is sRGB: back to linear-ish masks, and clean primaries.
    rgb = np.where(rgb > 0.5, 1.0, rgb * 2.0 * rgb)
    out = np.zeros((size, size, 3), np.float32)
    at = (size - inner) // 2
    out[at:at + h, at:at + w] = rgb
    # Coverage, and the colors un-premultiplied by it: an antialiased edge is the full ink, half laid on.
    alpha = np.clip(out.sum(axis=2), 0.0, 1.0)
    out = np.where(alpha[..., None] > 0.0, out / np.maximum(alpha[..., None], 1e-6), 0.0)
    save(name, out, alpha)


RED, GREEN, BLUE = 0xff0000, 0x00ff00, 0x0000ff


def g_embercrest(s):
    """The Embercrest Poker Series' tee: the mark (the crest in ink A, its ember ring in B), the name, the series in C."""
    import brand
    brand.mark(s, 50, 60, 54, mono=RED, ground=0x000000, mono_ember=GREEN)
    s.text('EMBERCREST', 50, 22, 12.0, RED, face='Black', align='CENTER', tracking=1.15)
    s.text('POKER SERIES', 50, 13, 5.0, BLUE, face='Bold', align='CENTER', tracking=1.8)


def g_allin(s):
    s.text('ALL', 50, 52, 30, RED, face='Black', align='CENTER')
    s.text('IN', 50, 22, 30, RED, face='Black', align='CENTER')
    s.rect(12, 14, 76, 4, GREEN)


def g_chip(s):
    s.ring(50, 50, 30, 42, RED)
    for k in range(8):
        a = k * math.pi / 4
        s.poly([(50 + 30 * math.cos(a - 0.14), 50 + 30 * math.sin(a - 0.14)), (50 + 42 * math.cos(a - 0.14), 50 + 42 * math.sin(a - 0.14)),
                (50 + 42 * math.cos(a + 0.14), 50 + 42 * math.sin(a + 0.14)), (50 + 30 * math.cos(a + 0.14), 50 + 30 * math.sin(a + 0.14))], BLUE)
    s.circle(50, 50, 24, GREEN)
    s.text('$5', 50, 42, 18, RED, face='Black', align='CENTER')


def g_badbeat(s):
    s.text('BAD BEAT', 50, 56, 17, RED, face='Black', align='CENTER')
    s.text('CLUB', 50, 30, 22, GREEN, face='Black', align='CENTER', tracking=1.3)
    s.rect(14, 50, 72, 2.2, BLUE)


def g_sunset(s):
    # A retro sunset: a disc cut by bars, over waves.
    for k in range(5):
        y0 = 30 + k * 9
        s.rect(20, y0, 60, 6.5 - k * 0.6, RED if k % 2 == 0 else GREEN)
    s.ring(50, 46, 31, 33, BLUE)
    s.text('RIVER CITY', 50, 12, 9, BLUE, face='Black', align='CENTER', tracking=1.4)


def g_number(s):
    s.text('77', 50, 24, 56, RED, face='Black', align='CENTER')
    s.text('NIGHTLY', 50, 80, 9, GREEN, face='Bold', align='CENTER', tracking=2.0)


def main():
    stripes('T_Print_Stripes', 10, 0.5)
    stripes('T_Print_Breton', 16, 0.3)
    stripes('T_Print_Ringer', 6, 0.42, accent=0.08)
    tartan('T_Print_Tartan')
    gingham('T_Print_Gingham')
    dots('T_Print_Dots')
    camo('T_Print_Camo')
    for name, draw in (('T_Graphic_Embercrest', g_embercrest), ('T_Graphic_AllIn', g_allin), ('T_Graphic_Chip', g_chip),
                       ('T_Graphic_BadBeat', g_badbeat), ('T_Graphic_Sunset', g_sunset), ('T_Graphic_Number', g_number)):
        graphic(name, draw)


if __name__ == "__main__":
    main()
