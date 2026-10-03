"""SM_Shelf: a double-sided gondola run down the middle of the Lucky Penny, 6 m long.

A kick-plate base deck, a slotted center spine with uprights every 1.2 m, three shelves a side on
brackets, each with a clear price channel holding yellow and white tags, and end panels. Powder-coated
in the store's off-white, scuffed low down by carts and mops. AStreetStage stocks the shelves.

Coordinates (meters), the run along X, its faces toward -Y and +Y, Z up: origin on the floor at the
middle. Shelf tops at SHELF_Z; each shelf DEPTH deep from the spine.
"""
import math

import bpy  # noqa: F401,I001
from mathutils import Matrix, Vector

from artkit import core, parts, street
from artkit.shade import image_surface, object_coords, ramp, vec

MESHES = ['SM_Shelf']
DOUBLE_SIDED = False
TEXTURE_SIZE = 2048
TEXTURE_SIZES = {'shelf_channel': 1024, 'shelf_feet': 128}
AO_DISTANCE = 0.1
REVIEW_VIEWS = [('front', -30, 12, 1.15), ('end', 70, 15, 0.7, (2.6, 0.0, 0.8)), ('detail', -20, 6, 0.2, (0.6, -0.3, 0.85))]

LENGTH = 6.0
HEIGHT = 1.6
SPINE = 0.06
DEPTH = 0.27
BASE = 0.12
SHELF_Z = (0.40, 0.85, 1.30)
SECTION = 1.2


def channel_sheet():
    """Tags in the channel: a strip one shelf long, tiled along the run."""
    from artkit.sheet import Sheet
    w, h = SECTION * 1000, 32
    s = Sheet(w, h)
    s.rect(0, 0, w, h, 0xdfe6ea, rough=0.15)
    prices = ['1.79', '2.29', '0.99', '3.49', '1.49', '4.29', '2.79', '1.29', '5.99', '2.49']
    x = 18.0
    k = 0
    while x < w - 50:
        sale = k % 4 == 1
        s.rect(x, 3, 46, 26, 0xffd23f if sale else 0xfbfbf8, rough=0.3)
        s.text(prices[k % len(prices)], x + 23, 9, 13, 0xc8202c if sale else 0x1c1c1e, face='Black', align='CENTER', rough=0.3)
        x += 92 + (k % 3) * 14
        k += 1
    return s.render('shelf_channel', 2048)


def channel_material(image):
    """The tag strip, tiled per section along X and per shelf height (both faces read left to right)."""
    m = core.Mat('shelf_channel')
    tc, sep = object_coords(m)
    u = m.math('DIVIDE', m.math('FLOORED_MODULO', m.math('MULTIPLY', sep.outputs['X'], m.math('SIGN', m.math('MULTIPLY', sep.outputs['Y'], -1.0))), SECTION), SECTION)
    pitch = SHELF_Z[1] - SHELF_Z[0]
    v = m.math('DIVIDE', m.math('FLOORED_MODULO', m.math('SUBTRACT', sep.outputs['Z'], SHELF_Z[0] - 0.05), pitch), 0.032)
    ink, alpha, metal, rough = image_surface(m, image, vec(m, u, v, 0.0))
    m.set('Base Color', ink)
    m.set('Roughness', rough)
    m.set('Coat Weight', 0.6)
    return m


def spine_material():
    """Slotted spine panels: rows of slots every 2.5 cm, painted, scuffed toward the floor."""
    m = street.painted('shelf_spine', 0xe4e1d9, rough=0.5, chips=0.15, rust=0.05, grime=0.5, grime_height=0.35, streaks=0.0)
    tc, sep = object_coords(m)
    sx = m.math('ABSOLUTE', m.math('SINE', m.math('MULTIPLY', sep.outputs['X'], 2 * math.pi / 0.05)))
    sz = m.math('ABSOLUTE', m.math('SINE', m.math('MULTIPLY', sep.outputs['Z'], 2 * math.pi / 0.05)))
    hole = m.math('MULTIPLY', m.math('LESS_THAN', sx, 0.22), m.math('LESS_THAN', sz, 0.5))
    base = m.bsdf.inputs['Base Color'].links[0].from_socket
    m.set('Base Color', m.mix(hole, base, core.hex_linear(0x18181a)))
    bump = m.node('ShaderNodeBump', Strength=0.8, Distance=0.003)
    m.link(m.math('SUBTRACT', 1.0, hole), bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def build():
    core.reset()
    paint = street.painted('shelf_paint', 0xe4e1d9, rough=0.48, chips=0.25, rust=0.05, grime=0.6, grime_height=0.3, streaks=0.0, primer_hex=0x8f8d88)
    spine = spine_material()
    kick = street.painted('shelf_kick', 0x2b2d31, rough=0.55, chips=0.3, rust=0.05, grime=0.8, grime_height=0.15, streaks=0.0)
    channel = channel_material(channel_sheet())
    feet_m = street.shop_plastic('shelf_feet', 0x111113, rough=0.7)
    out = []
    # Base deck and kick plates both sides.
    out.append((parts.rbox('deck', (0, 0, BASE - 0.01), (LENGTH, 2 * DEPTH + SPINE, 0.02), 0.004), paint))
    for sy in (-1, 1):
        out.append((parts.rbox('kick', (0, sy * (DEPTH + SPINE / 2 - 0.01), BASE / 2), (LENGTH, 0.02, BASE), 0.004), kick))
    # The spine and its uprights.
    out.append((parts.rbox('spine', (0, 0, (BASE + HEIGHT) / 2), (LENGTH, SPINE * 0.5, HEIGHT - BASE), 0.003), spine))
    sections = int(round(LENGTH / SECTION))
    for i in range(sections + 1):
        x = -LENGTH / 2 + i * SECTION
        x = max(-LENGTH / 2 + 0.03, min(LENGTH / 2 - 0.03, x))
        out.append((parts.rbox('upright', (x, 0, HEIGHT / 2), (0.05, SPINE, HEIGHT), 0.004), paint))
    out.append((parts.rbox('cap', (0, 0, HEIGHT + 0.01), (LENGTH, SPINE + 0.01, 0.02), 0.005), paint))
    # Shelves both sides: the deck, a turned-down lip, the price channel, brackets at each upright.
    for sy in (-1, 1):
        for z in SHELF_Z:
            y = sy * (SPINE / 2 + DEPTH / 2)
            out.append((parts.rbox('shelf', (0, y, z - 0.01), (LENGTH - 0.02, DEPTH, 0.02), 0.004), paint))
            lip_y = sy * (SPINE / 2 + DEPTH + 0.004)
            out.append((parts.rbox('lip', (0, lip_y, z - 0.025), (LENGTH - 0.02, 0.008, 0.05), 0.002), paint))
            out.append((parts.rbox('channel', (0, lip_y + sy * 0.006, z - 0.026), (LENGTH - 0.04, 0.004, 0.034), 0.0015), channel))
            for i in range(sections + 1):
                x = -LENGTH / 2 + i * SECTION
                x = max(-LENGTH / 2 + 0.06, min(LENGTH / 2 - 0.06, x))
                br = parts.panel('bracket', [(0.0, 0.0), (DEPTH - 0.02, 0.0), (0.02, -0.16), (0.0, -0.16)], -0.003, 0.003, 0.0)
                # Panel outline is in its XY: x out from the spine, y down from the shelf. Stand it up in YZ.
                br.data.transform(Matrix.Translation((x, sy * SPINE / 2, z - 0.02)) @ Matrix(((0, 0, 1, 0), (sy, 0, 0, 0), (0, 1, 0, 0), (0, 0, 0, 1))))
                core.orient_normals(br)
                out.append((br, paint))
    # End panels, and leveling feet.
    for sx in (-1, 1):
        out.append((parts.rbox('end', (sx * (LENGTH / 2 + 0.01), 0, (HEIGHT + 0.02) / 2), (0.02, 2 * DEPTH + SPINE + 0.02, HEIGHT + 0.02), 0.006), paint))
        for sy in (-1, 1):
            out.append((parts.disc('foot', (sx * (LENGTH / 2 - 0.05), sy * (DEPTH - 0.02), 0.006), (0, 0, 1), 0.02, 0.012, 12), feet_m))
    for o, m in out:
        core.assign(o, m)
    return [parts.finish([o for o, _ in out], 'SM_Shelf', 40)]
