"""The Embercrest's identity: the mark, the wordmark, its colors, and the brand's textures for the card room.

    blender -b -P art/blender/brand.py

The mark is a faceted three-peak crest with the ember rising behind it (the last light on the ridge, the casino's
name). Midnight navy grounds, ash-white peaks and type, ember copper and gold for the light. Other assets import
this module to print the mark (the felt, the banners, the trophy); main() writes the textures the game shows on its
screens and signs to unreal/Art/Textures/Brand/ (imported in color).
"""
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)

import bpy  # noqa: E402
import numpy as np  # noqa: E402

from artkit import core  # noqa: E402
from artkit.sheet import Sheet  # noqa: E402

OUT = os.path.join(core.ROOT, 'unreal', 'Art', 'Textures', 'Brand')

# The palette.
MIDNIGHT = 0x0a1020
NAVY = 0x121c33
SLATE = 0x1d2a47
STEEL = 0x3a4a6a
ICE = 0x8d9ab4
ASH = 0xebe7df
EMBER_DEEP = 0x9e2f0c
EMBER = 0xd9531e
EMBER_HOT = 0xf2832e
GOLD = 0xf6b04a
FLAME = 0xffd78a


def mix(c0, c1, f):
    out = 0
    for sh in (16, 8, 0):
        out |= round(((c0 >> sh) & 0xff) * (1 - f) + ((c1 >> sh) & 0xff) * f) << sh
    return out


def ember_disc(s, cx, cy, r, rings=40):
    """The ember: a disc running from deep ember at its rim to gold at its heart (rings stand in for a gradient)."""
    stops = [EMBER_DEEP, EMBER, EMBER_HOT, GOLD, FLAME]
    for k in range(rings):
        t = k / (rings - 1)
        a = t * (len(stops) - 1)
        i = min(int(a), len(stops) - 2)
        # Hotter low in the disc, where the light breaks over the ridge.
        s.circle(cx, cy - r * 0.22 * t, r * (1.0 - 0.8 * t), mix(stops[i], stops[i + 1], a - i), n=128)


def stroke(s, a, b, half, color):
    """A straight stroke from a to b, half its width either side."""
    dx, dy = b[0] - a[0], b[1] - a[1]
    n = (dx * dx + dy * dy) ** 0.5 or 1.0
    ox, oy = -dy / n * half, dx / n * half
    s.poly([(a[0] + ox, a[1] + oy), (b[0] + ox, b[1] + oy), (b[0] - ox, b[1] - oy), (a[0] - ox, a[1] - oy)], color)


def mark(s, cx, cy, size, ember=True, mono=None, ground=MIDNIGHT, mono_ember=None):
    """The mark in a size x size box centered at (cx, cy): the ember rising behind a faceted three-peak crest.
    mono: one ink for everything (a print or an engraving), with ground showing through the cuts (the ridges as
    engraved lines); mono_ember, another ink for the ember's ring."""
    u = size / 100.0
    base = cy - 28 * u

    def P(x, y):
        return (cx + x * u, base + y * u)
    if ember:
        if mono is None:
            for k in range(12):
                s.circle(cx, base + 42 * u, (47 - k * 0.8) * u, EMBER_HOT, n=128, alpha=0.035)
            ember_disc(s, cx, base + 42 * u, 34 * u)
        else:
            # One ink: the ember as a ring round the crest.
            s.ring(cx, base + 42 * u, 30 * u, 34 * u, mono_ember if mono_ember is not None else mono, n=128)
    # The crest: two shoulders and the high middle peak, in front of the ember (a ground-colored rim parts them).
    Lb, Lp, V1, C, V2, Rp, Rb = P(-48, 0), P(-26, 34), P(-15, 22), P(0, 60), P(13, 29), P(25, 40), P(48, 0)
    rim = [P(-51, -1.5), P(-26, 37.5), P(-15, 25.5), P(0, 64), P(13, 32.5), P(25, 43.5), P(51, -1.5)]
    s.poly(rim, ground)
    light = mono if mono is not None else ASH
    shade = ground if mono is not None else ICE
    s.poly([Lb, Lp, V1, C, V2, Rp, Rb], light)
    if mono is None:
        # The faces away from the light, one per peak.
        s.poly([Lp, V1, P(-13, 0), P(-21, 0)], shade)
        s.poly([C, V2, P(15, 0), P(3, 0)], shade)
        s.poly([Rp, Rb, P(31, 0)], shade)
    else:
        # One ink: each peak's ridge down to the ground, engraved.
        for top, foot in ((Lp, P(-21, 0)), (C, P(3, 0)), (Rp, P(31, 0))):
            stroke(s, top, foot, 1.1 * u, ground)
    if mono is None:
        # Snow on the high peak's lit face.
        s.poly([C, P(-8, 43), P(-4.5, 46.5), P(-1.5, 42), P(1.6, 49)], 0xffffff)
    # The ground line, parted under the high peak.
    line = mono if mono is not None else EMBER_HOT
    s.rect(cx - 48 * u, base - 6.5 * u, 45 * u, 2.4 * u, line)
    s.rect(cx + 3 * u, base - 6.5 * u, 45 * u, 2.4 * u, line)


def wordmark(s, x, y, size, color=ASH, sub='POKER SERIES', sub_color=GOLD, align='CENTER'):
    """EMBERCREST, wide and heavy, with its line under it. size: the type's size (layout units)."""
    s.text('EMBERCREST', x, y, size, color, face='Black', align=align, tracking=1.22)
    if sub:
        s.text(sub, x + (size * 0.04 if align == 'LEFT' else 0.0), y - size * 0.58, size * 0.36, sub_color, face='Bold', align=align, tracking=1.9)


def lockup(s, cx, cy, w, sub='POKER SERIES', mono=None, ground=MIDNIGHT):
    """The mark over the wordmark, centered, w wide."""
    mark(s, cx, cy + w * 0.2, w * 0.46, mono=mono, ground=ground)
    wordmark(s, cx, cy - w * 0.16, w * 0.118, color=mono or ASH, sub=sub, sub_color=mono or GOLD)


def save_png(name, rgba):
    import struct
    import zlib
    os.makedirs(OUT, exist_ok=True)
    h, w, ch = rgba.shape
    data = (np.clip(rgba, 0.0, 1.0) * 255.0 + 0.5).astype(np.uint8)
    raw = b''.join(bytes([0]) + data[y].tobytes() for y in range(h))

    def chunk(tag, body):
        return struct.pack('>I', len(body)) + tag + body + struct.pack('>I', zlib.crc32(tag + body) & 0xffffffff)
    signature = bytes([0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A])
    png = signature + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6 if ch == 4 else 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b'')
    with open(os.path.join(OUT, name + '.png'), 'wb') as f:
        f.write(png)
    print('wrote', name, w, h)


def clear():
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o)


def render(name, s, px, alpha=False):
    """Renders a sheet and writes it to Brand/<name>.png (sRGB color; with alpha, RGBA)."""
    color, _surface = s.render(name, px, samples=32)
    img = bpy.data.images.load(color)
    w, h = img.size
    rgba = np.array(img.pixels[:], dtype=np.float32).reshape(h, w, 4)[::-1]
    save_png(name, rgba if alpha else rgba[..., :3])


def sheet_rgba(name, s, px):
    """A sheet rendered to an RGBA float array (row 0 at the top), sRGB."""
    color, _surface = s.render(name, px, samples=32)
    img = bpy.data.images.load(color)
    w, h = img.size
    out = np.array(img.pixels[:], dtype=np.float32).reshape(h, w, 4)[::-1].copy()
    bpy.data.images.remove(img)
    return out


def over(bg, fg):
    """fg (RGBA) over bg (RGB), same size."""
    a = fg[..., 3:4]
    return bg * (1.0 - a) + fg[..., :3] * a


def srgb(hexc):
    return np.array([((hexc >> 16) & 0xff) / 255.0, ((hexc >> 8) & 0xff) / 255.0, (hexc & 0xff) / 255.0], np.float32)


def ridge(width, seed, base, amp, octaves=6):
    """A mountain skyline: a height (0..1 of the image, from the bottom) per column."""
    rng = np.random.default_rng(seed)
    x = np.linspace(0.0, 1.0, width)
    h = np.zeros(width, np.float32)
    a, f = amp, 2.0
    for _ in range(octaves):
        ph = rng.uniform(0, 6.283, 3)
        h += a * (np.abs(np.sin(x * f * 3.1 + ph[0])) * 0.6 + 0.4 * np.sin(x * f * 5.3 + ph[1]))
        a *= 0.5
        f *= 2.1
    return base + h


def night_ground(w, h, top=MIDNIGHT, bottom=SLATE, ember=0.0, ranges=((7, 0.30, 0.16, STEEL, 0.35), (3, 0.18, 0.12, NAVY, 0.0))):
    """A midnight gradient with mountain ranges across the bottom and an ember glow along the horizon."""
    y = np.linspace(1.0, 0.0, h, dtype=np.float32)[:, None, None]  # 1 at the top
    img = srgb(bottom) * (1.0 - y) + srgb(top) * y
    img = np.broadcast_to(img, (h, w, 3)).copy()
    if ember > 0.0:
        glow = np.exp(-((y - 0.28) / 0.16) ** 2) * ember
        img = img + glow * (srgb(EMBER) - img) * 0.9
    rows = np.arange(h, dtype=np.float32)[:, None] / h  # 0 at the top
    for seed, base, amp, color, rim in ranges:
        line = ridge(w, seed, base, amp)
        height = 1.0 - rows  # 0 at the bottom
        inside = (height < line[None, :]).astype(np.float32)
        edge = np.clip(1.0 - np.abs(height - line[None, :]) * h / 2.5, 0.0, 1.0) * rim
        fill = srgb(color)
        img = img * (1.0 - inside[..., None]) + fill * inside[..., None]
        img = img + edge[..., None] * (srgb(EMBER_HOT) - img) * 0.8
    return img


def logo_layout(s, ground=None):
    if ground is not None:
        s.rect(0, 0, 200, 100, ground)
    mark(s, 36.0, 52.0, 62.0)
    wordmark(s, 74.0, 50.0, 16.0, align='LEFT')


def screen_bg():
    """The tournament clocks' background (16:9): night over the ranges, the lockup top-left, a frame, a ticker band.
    The game writes the clock over the middle."""
    w, h = 2048, 1152
    img = night_ground(w, h, MIDNIGHT, 0x16223d, ember=0.4, ranges=((11, 0.10, 0.10, 0x101a30, 0.5), (5, 0.04, 0.07, MIDNIGHT, 0.0)))
    clear()
    s = Sheet(160.0, 90.0)
    mark(s, 12.0, 79.0, 13.0)
    wordmark(s, 21.0, 78.4, 4.6, align='LEFT', sub='POKER SERIES')
    s.text('LIVE TOURNAMENT CLOCK', 150.0, 78.6, 2.6, ICE, face='Bold', align='RIGHT', tracking=1.6)
    s.rect(4.0, 71.5, 152.0, 0.35, EMBER_HOT)
    s.rect(4.0, 11.0, 152.0, 0.35, STEEL)
    s.text('PLAY  ·  COMPETE  ·  RISE', 80.0, 5.2, 2.6, GOLD, face='Bold', align='CENTER', tracking=1.8)
    # The frame: a hairline round the screen, ember corners.
    for x, y, ww, hh in ((2, 2, 156, 0.4), (2, 87.6, 156, 0.4), (2, 2, 0.4, 86), (157.6, 2, 0.4, 86)):
        s.rect(x, y, ww, hh, STEEL)
    for x, y in ((2, 2), (2, 87.6), (148, 2), (148, 87.6)):
        s.rect(x, y, 10, 0.4, EMBER_HOT)
    save_png('T_EC_ScreenBg', over(img, sheet_rgba('ec_screen', s, w)))


def stage_wall():
    """The final table's backdrop (2:1): the big lockup over a mountain panorama, the tagline."""
    w, h = 2048, 1024
    img = night_ground(w, h, MIDNIGHT, 0x1a2644, ember=0.8, ranges=((21, 0.13, 0.15, 0x17223c, 0.7), (8, 0.07, 0.08, 0x0f1729, 0.4),
                                                                      (2, 0.03, 0.04, MIDNIGHT, 0.0)))
    clear()
    s = Sheet(200.0, 100.0)
    lockup(s, 100.0, 54.0, 64.0, sub='POKER SERIES')
    s.text('PLAY  ·  COMPETE  ·  RISE', 100.0, 8.0, 4.2, ASH, face='Bold', align='CENTER', tracking=1.5)
    save_png('T_EC_StageWall', over(img, sheet_rgba('ec_stage', s, w)))


def sign_face():
    """A table sign's face (2:1): the mark and TABLE on the left; the game writes the number on the right."""
    w, h = 1024, 512
    y = np.linspace(1.0, 0.0, h, dtype=np.float32)[:, None, None]
    img = np.broadcast_to(srgb(0x0d1528) * y + srgb(0x172443) * (1.0 - y), (h, w, 3)).copy()
    clear()
    s = Sheet(200.0, 100.0)
    mark(s, 36.0, 58.0, 48.0)
    s.text('TABLE', 36.0, 14.0, 13.0, ASH, face='Black', align='CENTER', tracking=1.4)
    s.rect(72.0, 12.0, 0.8, 76.0, STEEL)
    s.rect(4.0, 3.0, 192.0, 1.6, EMBER_HOT)
    save_png('T_EC_SignFace', over(img, sheet_rgba('ec_sign', s, w)))


def banner_art(s, w, h, tagline=True):
    """A hanging banner's print (w x h, portrait): navy, the lockup, an ember band over the ranges at the foot."""
    s.rect(0, 0, w, h, NAVY)
    for k in range(40):
        t = k / 39.0
        s.rect(0, h * (0.55 + 0.45 * t), w, h * 0.012 + 0.01, mix(NAVY, MIDNIGHT, t))
    lockup(s, w * 0.5, h * 0.66, w * 0.82, sub='POKER SERIES')
    if tagline:
        s.text('PLAY', w * 0.5, h * 0.36, w * 0.09, ASH, face='Black', align='CENTER', tracking=1.6)
        s.text('COMPETE', w * 0.5, h * 0.30, w * 0.09, ASH, face='Black', align='CENTER', tracking=1.6)
        s.text('RISE', w * 0.5, h * 0.24, w * 0.09, EMBER_HOT, face='Black', align='CENTER', tracking=1.6)
    # The foot: ember light over a dark range.
    for k in range(24):
        t = k / 23.0
        s.rect(0, h * (0.04 + 0.12 * t), w, h * 0.006 + 0.01, mix(EMBER, NAVY, t))
    pts = [(0, h * 0.04)]
    import random
    rnd = random.Random(4)
    x = 0.0
    while x < w:
        pts.append((x, h * (0.08 + rnd.uniform(0.0, 0.07))))
        x += w * rnd.uniform(0.05, 0.12)
    pts += [(w, h * 0.09), (w, h * 0.04)]
    s.poly(pts, MIDNIGHT)
    s.rect(0, 0, w, h * 0.04, MIDNIGHT)
    s.rect(0, h * 0.985, w, h * 0.015, EMBER_HOT)


def view():
    """The night from the terrace (4:1): stars, the ranges with snow catching the moon, the resort's lights warm on
    the lower slopes, the last ember along the far ridge."""
    w, h = 4096, 1024
    rng = np.random.default_rng(7)
    img = night_ground(w, h, 0x05080f, 0x1a2338, ember=0.35, ranges=())
    # Stars, fainter toward the horizon.
    ys = rng.integers(0, int(h * 0.55), 2600)
    xs = rng.integers(0, w, 2600)
    b = rng.uniform(0.25, 1.0, 2600) * (1.0 - ys / (h * 0.6))
    for y, x, v in zip(ys, xs, b):
        img[y, x] = np.maximum(img[y, x], v * np.array([0.85, 0.9, 1.0]))
    rows = np.arange(h, dtype=np.float32)[:, None] / h
    height = 1.0 - rows
    for seed, base, amp, color, snow in ((31, 0.30, 0.22, 0x1b2540, 0.55), (17, 0.20, 0.16, 0x121a2e, 0.35), (5, 0.10, 0.10, 0x0a101d, 0.0)):
        line = ridge(w, seed, base, amp)[None, :]
        inside = (height < line).astype(np.float32)
        # Snow near each crest, thinning down the slope.
        cap = np.clip(1.0 - (line - height) / (amp * 0.45), 0.0, 1.0) * inside * snow
        fill = srgb(color)[None, None, :] * (0.75 + 0.25 * cap[..., None]) + cap[..., None] * np.array([0.30, 0.34, 0.42])
        img = img * (1.0 - inside[..., None]) + fill * inside[..., None]
        edge = np.clip(1.0 - np.abs(height - line) * h / 2.0, 0.0, 1.0) * (snow * 0.6)
        img = img + edge[..., None] * (np.array([0.75, 0.8, 0.9]) - img) * 0.5
    # The resort: warm lights in clusters on the nearest slopes, a string of them up a lit run.
    for k in range(9):
        cx, cy = rng.uniform(0.05, 0.95) * w, rng.uniform(0.80, 0.93) * h
        for _ in range(60):
            x = int(cx + rng.normal(0, 40))
            y = int(cy + rng.normal(0, 10))
            if 0 <= x < w - 1 and 0 <= y < h - 1:
                warm = np.array([1.0, 0.72, 0.38]) * rng.uniform(0.5, 1.0)
                img[y:y + 2, x:x + 2] = np.maximum(img[y:y + 2, x:x + 2], warm)
    for k in range(70):
        t = k / 69.0
        x = int(w * (0.62 + 0.09 * t + 0.01 * math.sin(t * 9)))
        y = int(h * (0.93 - 0.36 * t))
        img[y:y + 2, x:x + 2] = np.array([1.0, 0.85, 0.6])
    save_png('T_EC_View', np.clip(img, 0.0, 1.0))


def main():
    view()
    screen_bg()
    stage_wall()
    sign_face()
    # The lockup on transparent, for screens and signs.
    clear()
    s = Sheet(200.0, 100.0)
    logo_layout(s)
    render('T_EC_Logo', s, 2048, alpha=True)
    if os.environ.get('EC_PREVIEW'):
        clear()
        s = Sheet(200.0, 100.0)
        logo_layout(s, NAVY)
        render('preview_logo', s, 1600)


if __name__ == '__main__':
    main()
