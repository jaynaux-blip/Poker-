"""SM_Cooler: the Lucky Penny's walk-in cooler wall, ten glass doors along the back of the store.

A steel cabinet with a lit header (COLD DRINKS · ICE · DAIRY), ten door frames with chrome pull bars,
the mullions between them carrying LED strips that light the shelves, four wire shelves per door with
white price rails, a white interior, a black kick plate. The glass itself is left out (the bake has no
transmission); AStreetStage stocks the shelves with drinks.

Coordinates (meters), front toward -Y (into the store), Z up: origin on the floor at the middle of the
wall, the cabinet DEPTH deep centered on it. Shelf tops at SHELF_Z.
"""
import math

import bpy  # noqa: F401,I001
from mathutils import Matrix, Vector

from artkit import core, parts, street
from artkit.shade import image_surface, object_coords, planar
from artkit.sheet import Sheet

MESHES = ['SM_Cooler']
DOUBLE_SIDED = False
TEXTURE_SIZE = 2048
TEXTURE_SIZES = {'cooler_led': 128, 'cooler_header': 2048, 'cooler_rail': 1024, 'cooler_kick': 512, 'cooler_shelf': 1024, 'cooler_chrome': 512}
AO_DISTANCE = 0.12
REVIEW_VIEWS = [('front', -25, 8, 1.2), ('doors', -40, 10, 0.5, (2.0, -0.4, 1.1)), ('detail', -20, 5, 0.18, (0.4, -0.45, 1.2))]

DOORS = 10
DOOR_W = 0.76
WIDTH = DOORS * DOOR_W + 0.12
DEPTH = 0.9
HEIGHT = 2.25
KICK = 0.2
HEADER0 = 2.02
SHELF_Z = (0.28, 0.73, 1.18, 1.63)


def header_sheet():
    w, h = WIDTH * 1000, 0.16 * 1000
    s = Sheet(w, h)
    s.rect(0, 0, w, h, 0xf3f6fa, rough=0.3)
    s.rect(0, 0, w, 8, 0x1e5fa8, rough=0.3)
    s.rect(0, h - 8, w, 8, 0x1e5fa8, rough=0.3)
    labels = ['COLD DRINKS', 'ENERGY', 'WATER', 'JUICE', 'ICE', 'DAIRY']
    for k, label in enumerate(labels):
        x = w * (k + 0.5) / len(labels)
        s.text(label, x, h * 0.32, 78, 0x1e5fa8, face='Black', align='CENTER', tracking=1.15, rough=0.3)
    return s.render('cooler_header', 4096)


def rail_sheet():
    """The price rail: white with a price tag under each facing."""
    w, h = WIDTH * 1000, 40
    s = Sheet(w, h)
    s.rect(0, 0, w, h, 0xf4f4f2, rough=0.35)
    x = 30.0
    k = 0
    prices = ['1.99', '2.49', '2.99', '1.49', '3.49', '2.29', '1.29', '3.99']
    while x < w - 60:
        s.rect(x, 4, 52, 32, 0xffd84a if k % 5 == 2 else 0xffffff, rough=0.35)
        s.text(prices[k % len(prices)], x + 26, 12, 15, 0x1b1b1d, face='Black', align='CENTER', rough=0.35)
        x += 76 + (k % 3) * 8
        k += 1
    return s.render('cooler_rail', 4096)


def printed(name, image, x0, x1, z0, z1, glow=0.0):
    m = core.Mat(name)
    tc, sep = object_coords(m)
    uv = planar(m, sep, 'X', 'Z', x0, x1, z0, z1)
    ink, alpha, metal, rough = image_surface(m, image, uv)
    m.set('Base Color', ink)
    m.set('Roughness', rough)
    if glow > 0.0:
        m.link(ink, m.bsdf.inputs['Emission Color'])
        m.set('Emission Strength', glow)
    return m


def rail_material(image):
    """The rail print, repeated up the shelves: v comes from the height above each shelf's rail."""
    from artkit.shade import vec
    m = core.Mat('cooler_rail')
    tc, sep = object_coords(m)
    u = m.math('DIVIDE', m.math('ADD', sep.outputs['X'], WIDTH / 2), WIDTH)
    pitch = SHELF_Z[1] - SHELF_Z[0]
    v = m.math('DIVIDE', m.math('FLOORED_MODULO', m.math('SUBTRACT', sep.outputs['Z'], SHELF_Z[0] - 0.024), pitch), 0.04)
    ink, alpha, metal, rough = image_surface(m, image, vec(m, u, v, 0.0))
    m.set('Base Color', ink)
    m.set('Roughness', rough)
    return m


def wire_shelf_material():
    """A chrome wire shelf seen from a step away: the wires as a fine grid over the gaps' shadow."""
    m = core.Mat('cooler_shelf')
    tc, sep = object_coords(m)
    gx = m.math('ABSOLUTE', m.math('SINE', m.math('MULTIPLY', sep.outputs['X'], 2 * math.pi / 0.025)))
    gy = m.math('ABSOLUTE', m.math('SINE', m.math('MULTIPLY', sep.outputs['Y'], 2 * math.pi / 0.1)))
    wire = m.math('MAXIMUM', m.math('GREATER_THAN', gx, 0.82), m.math('GREATER_THAN', gy, 0.95))
    m.set('Base Color', m.mix(wire, core.hex_linear(0x2a2c30), core.hex_linear(0xc9ccd0)))
    m.set('Metallic', wire)
    m.set('Roughness', m.mix(wire, (0.7, 0.7, 0.7, 1.0), (0.25, 0.25, 0.25, 1.0)))
    bump = m.node('ShaderNodeBump', Strength=0.6, Distance=0.002)
    m.link(wire, bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def build():
    core.reset()
    cab = street.painted('cooler_cabinet', 0x1b1d21, rough=0.45, chips=0.25, rust=0.1, grime=0.3, grime_height=0.3, streaks=0.0)
    inside = street.shop_plastic('cooler_inside', 0xe9ecef, rough=0.4, grime=0.2)
    chrome = street.bare_metal('cooler_chrome', 0xc8ccd0, rough=0.18, grime=0.3)
    kick = street.shop_plastic('cooler_kick', 0x101012, rough=0.6, grime=0.6)
    led = street.lens('cooler_led', 0xeaf4ff, strength=8.0, rough=0.3)
    shelf = wire_shelf_material()
    header = printed('cooler_header', header_sheet(), -WIDTH / 2, WIDTH / 2, HEADER0 + 0.04, HEADER0 + 0.2, glow=2.5)
    rail = rail_material(rail_sheet())
    out = []
    y0, y1 = -DEPTH / 2, DEPTH / 2
    front = y0 + 0.04
    # The cabinet: back, ends, top, floor deck, kick plate, header box.
    out.append((parts.rbox('back', (0, y1 - 0.02, HEIGHT / 2), (WIDTH, 0.04, HEIGHT), 0.004), inside))
    for sx in (-1, 1):
        out.append((parts.rbox('end', (sx * (WIDTH / 2 - 0.03), 0, HEIGHT / 2), (0.06, DEPTH, HEIGHT), 0.008), cab))
    out.append((parts.rbox('top', (0, 0, HEIGHT - 0.02), (WIDTH, DEPTH, 0.04), 0.006), cab))
    out.append((parts.rbox('deck', (0, 0.02, KICK - 0.015), (WIDTH - 0.12, DEPTH - 0.06, 0.03), 0.004), inside))
    out.append((parts.rbox('kick', (0, front - 0.005, KICK / 2), (WIDTH, 0.02, KICK), 0.006), kick))
    out.append((parts.rbox('header_box', (0, front + 0.02, (HEADER0 + HEIGHT) / 2), (WIDTH, 0.06, HEIGHT - HEADER0), 0.008), cab))
    # The sign in six panels, one per label (separate islands keep the print sharp in the bake).
    seg = (WIDTH - 0.14) / 6
    for k in range(6):
        out.append((parts.rbox('header_face', (-WIDTH / 2 + 0.07 + seg * (k + 0.5), front - 0.012, HEADER0 + 0.12), (seg - 0.004, 0.004, 0.16), 0.001), header))
    # The ceiling inside, lit from the header.
    out.append((parts.rbox('ceiling', (0, 0.02, HEADER0 - 0.01), (WIDTH - 0.12, DEPTH - 0.06, 0.02), 0.0), inside))
    # Doors: frames, pull bars; mullions with LED strips facing in.
    x_left = -DOORS * DOOR_W / 2
    for i in range(DOORS + 1):
        x = x_left + i * DOOR_W
        out.append((parts.rbox('mullion', (x, front + 0.01, (KICK + HEADER0) / 2), (0.05, 0.05, HEADER0 - KICK), 0.006), cab))
        for side in (-1, 1):
            if (i == 0 and side < 0) or (i == DOORS and side > 0):
                continue
            out.append((parts.rbox('led', (x + side * 0.027, front + 0.03, (KICK + HEADER0) / 2), (0.006, 0.02, HEADER0 - KICK - 0.1), 0.002), led))
    for i in range(DOORS):
        x = x_left + (i + 0.5) * DOOR_W
        # The door's frame: stiles and rails, a hair proud of the cabinet.
        for (cx, cz, sx, sz) in ((x, KICK + 0.03, DOOR_W - 0.06, 0.05), (x, HEADER0 - 0.03, DOOR_W - 0.06, 0.05),
                                 (x - DOOR_W / 2 + 0.05, (KICK + HEADER0) / 2, 0.035, HEADER0 - KICK - 0.02),
                                 (x + DOOR_W / 2 - 0.05, (KICK + HEADER0) / 2, 0.035, HEADER0 - KICK - 0.02)):
            out.append((parts.rbox('door_frame', (cx, front - 0.015, cz), (sx, 0.025, sz), 0.005), cab))
        # The pull bar on the side away from the hinge (doors alternate).
        hx = x + (DOOR_W / 2 - 0.1) * (1 if i % 2 == 0 else -1)
        out.append((parts.rod('pull', (hx, front - 0.06, 0.85), (hx, front - 0.06, 1.55), 0.012, 16), chrome))
        for z in (0.88, 1.52):
            out.append((parts.rod('pull_leg', (hx, front - 0.03, z), (hx, front - 0.06, z), 0.009, 12), chrome))
        # The shelves and their price rails.
        for z in SHELF_Z:
            out.append((parts.rbox('shelf', (x, 0.0, z - 0.008), (DOOR_W - 0.06, DEPTH - 0.18, 0.016), 0.003), shelf))
            out.append((parts.rbox('rail', (x, -(DEPTH - 0.18) / 2 - 0.008, z - 0.004), (DOOR_W - 0.06, 0.008, 0.034), 0.002), rail))
    for o, m in out:
        core.assign(o, m)
    return [parts.finish([o for o, _ in out], 'SM_Cooler', 40)]
