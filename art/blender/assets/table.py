"""SM_PokerTable: the Spin Cycle Club's table in the laundromat's back room.

An 8-foot oval home table (2.44 x 1.22 m) that has hosted years of Tuesday games:
- worn casino-green wool: pilled and faded where hands slide cards in front of each seat and
  worst in front of the dealer, rings where chip stacks sit, beer rings, two cigarette burns, and
  the club's spade printed in the middle, washing out;
- a padded oxblood vinyl rail with panel seams and top-stitching, glossed and cracked where
  forearms rest, split open at one seat and patched with duct tape;
- stainless cup holders sunk into the rail, and two heavy steel pedestals.

Coordinates (meters): origin on the floor under the middle of the table, Z up; the player's seat is
at -Y (the front), the dealer's at +Y. The felt is at FELT_Z. SEATS lists where each chair faces the
table, counterclockwise from the player; the stage uses the same numbers.
"""
import math

from artkit import core
from artkit.shade import circle_dist, image_surface, mix_float, noise, object_coords, planar, powder_coat, ramp, scaled, smooth_less
from artkit.sheet import Sheet

MESHES = ['SM_PokerTable']
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {'table_felt': (4096, 2048), 'table_rail': (1024, 8192), 'table_steel': 512, 'table_base': 1024, 'table_wood': 1024}
AO_DISTANCE = 0.03

HALF_L = 0.61          # half the straight run; the ends are half circles
RADIUS = 0.61          # table half-width
RAIL_W = 0.13
RAIL_C = RADIUS - RAIL_W / 2
FELT_R = RADIUS - RAIL_W + 0.006  # the felt tucks under the rail
FELT_Z = 0.76
PLY_Z0 = 0.715



def rail_point(s):
    """The point s meters along the rail's outer edge from the player's seat (x = 0, y = -RADIUS), counterclockwise."""
    arc = math.pi * RADIUS
    if s < HALF_L:
        return (s, -RADIUS)
    s -= HALF_L
    if s < arc:
        a = -math.pi / 2 + s / RADIUS
        return (HALF_L + RADIUS * math.cos(a), RADIUS * math.sin(a))
    s -= arc
    if s < 2 * HALF_L:
        return (HALF_L - s, RADIUS)
    s -= 2 * HALF_L
    if s < arc:
        a = math.pi / 2 + s / RADIUS
        return (-HALF_L + RADIUS * math.cos(a), RADIUS * math.sin(a))
    s -= arc
    return (-HALF_L + s, -RADIUS)


# Seven seats spaced evenly round the rail's outer edge (89 cm apart: room for a pair of shoulders between chairs),
# counterclockwise from the player: (x, y). 0 is the player, 4 the dealer, who sits a little to the side of straight
# across. The game (BackRoomStage.cpp SeatEdge) lays its seats out the same way.
SEAT_COUNT = 7
PERIMETER = 4 * HALF_L + 2 * math.pi * RADIUS
SEATS = [rail_point(i * PERIMETER / SEAT_COUNT) for i in range(SEAT_COUNT)]
PEDESTAL_X = 0.62

REVIEW_VIEWS = [('front', -25, 30, 1.6), ('top', 0, 80, 1.35), ('detail', -40, 30, 0.45, (0.25, -0.5, FELT_Z))]


def stadium(half_l, radius, n_end=64, n_side=32):
    """Counterclockwise outline of a stadium (a rectangle with half-circle ends), in XY."""
    pts = []
    for i in range(n_side):
        pts.append((-half_l + 2 * half_l * i / n_side, -radius))
    for i in range(n_end):
        a = -math.pi / 2 + math.pi * i / n_end
        pts.append((half_l + radius * math.cos(a), radius * math.sin(a)))
    for i in range(n_side):
        pts.append((half_l - 2 * half_l * i / n_side, radius))
    for i in range(n_end):
        a = math.pi / 2 + math.pi * i / n_end
        pts.append((-half_l + radius * math.cos(a), radius * math.sin(a)))
    return pts


def inward(seat, dist):
    """A point dist meters in from a seat, toward the table's middle line."""
    x, y = seat
    cx = max(-HALF_L, min(HALF_L, x))
    dx, dy = cx - x, -y
    n = math.hypot(dx, dy) or 1.0
    return x + dx / n * dist, y + dy / n * dist


def stadium_dist(m, sep):
    """Shader: distance from the stadium's core segment (the table edge is at RADIUS)."""
    qx = m.math('MAXIMUM', m.math('SUBTRACT', m.math('ABSOLUTE', sep.outputs['X']), HALF_L), 0.0)
    return m.math('SQRT', m.math('ADD', m.math('MULTIPLY', qx, qx), m.math('MULTIPLY', sep.outputs['Y'], sep.outputs['Y'])))


# ------------------------------------------------------------------ felt

def felt_print():
    """The club's mark in the middle of the felt (millimeters, 700 x 360): a spade inside the name."""
    s = Sheet(700, 360)
    cx, cy = 350, 180
    ink = 0x9fc7a8
    size = 115
    s.circle(cx - 0.27 * size, cy - 0.02 * size, 0.28 * size, ink, n=96, rough=0.9)
    s.circle(cx + 0.27 * size, cy - 0.02 * size, 0.28 * size, ink, n=96, rough=0.9)
    s.poly([(cx, cy + 0.62 * size), (cx - 0.52 * size, cy + 0.02 * size), (cx + 0.52 * size, cy + 0.02 * size)], ink, rough=0.9)
    s.poly([(cx, cy - 0.05 * size), (cx + 0.2 * size, cy - 0.55 * size), (cx - 0.2 * size, cy - 0.55 * size)], ink, rough=0.9)
    s.text('SPIN CYCLE CLUB', cx, cy - 118, 34, ink, face='Black', align='CENTER', tracking=1.25, rough=0.9)
    s.text('EST. TUESDAYS', cx, cy + 92, 16, ink, face='Bold', align='CENTER', tracking=1.6, rough=0.9)
    s.ring(cx, cy, 150, 156, ink, rough=0.9, n=160)
    return s.render('table_print', 2048)


def felt_material():
    m = core.Mat('table_felt')
    tc, sep = object_coords(m)
    obj = tc.outputs['Object']
    x, y = sep.outputs['X'], sep.outputs['Y']
    fiber = noise(m, obj, 5200.0, 2.0)
    fuzz = noise(m, scaled(m, sep, 900.0, 900.0, 1.0), 1.0, 3.0)
    mottle = noise(m, obj, 5.0, 4.0)
    fade = noise(m, obj, 1.4, 3.0)

    # Wear where cards slide in front of each seat, and most in front of the dealer.
    wear = None
    for i, seat in enumerate(SEATS):
        wx, wy = inward(seat, 0.24)
        d = circle_dist(m, sep, wx, wy)
        w = smooth_less(m, d, 0.2 if i == 4 else 0.15, 0.1)
        w = m.math('MULTIPLY', w, 1.0 if i == 4 else 0.7)
        wear = w if wear is None else m.math('MAXIMUM', wear, w)
    # The strip down the middle where the board is dealt.
    board = m.math('MULTIPLY', smooth_less(m, m.math('ABSOLUTE', y), 0.09, 0.05), smooth_less(m, m.math('ABSOLUTE', x), 0.34, 0.08))
    wear = m.math('MAXIMUM', wear, m.math('MULTIPLY', board, 0.55))
    wear = m.math('MULTIPLY', wear, m.math('ADD', 0.7, m.math('MULTIPLY', m.math('SUBTRACT', noise(m, obj, 60.0, 5.0), 0.5), 0.5)))
    pills = m.math('MULTIPLY', ramp(m, noise(m, obj, 1600.0, 1.0), 0.64, 0.7), wear)

    # Faint rings where each player's chips have stood for years.
    rings = None
    for i, seat in enumerate(SEATS):
        if i == 4:
            continue
        for k, off in enumerate((-0.05, 0.05)):
            sx, sy = inward(seat, 0.14)
            ang = math.atan2(-sy, -(sx - max(-HALF_L, min(HALF_L, sx))) or 1e-6)
            px, py = sx + math.cos(ang + math.pi / 2) * off * 2.2, sy + math.sin(ang + math.pi / 2) * off * 2.2
            d = circle_dist(m, sep, px, py)
            r = smooth_less(m, m.math('ABSOLUTE', m.math('SUBTRACT', d, 0.0195)), 0.0012, 0.001)
            rings = r if rings is None else m.math('MAXIMUM', rings, r)
    rings = m.math('MULTIPLY', rings, m.math('MULTIPLY', ramp(m, noise(m, obj, 60.0, 3.0), 0.35, 0.6), 0.5))

    # Spills: beer rings near three seats and a dark sticky patch.
    breakup = noise(m, obj, 45.0, 3.0)
    spills = None
    for cx, cy, rad in ((0.42, -0.36, 0.034), (-0.95, 0.12, 0.034), (0.88, 0.3, 0.032), (0.47, -0.33, 0.033)):
        d = circle_dist(m, sep, cx, cy)
        line = smooth_less(m, m.math('ABSOLUTE', m.math('SUBTRACT', d, m.math('ADD', rad, m.math('MULTIPLY', m.math('SUBTRACT', breakup, 0.5), 0.004)))), 0.0014, 0.001)
        s = m.math('MAXIMUM', line, m.math('MULTIPLY', smooth_less(m, d, rad, 0.003), 0.3))
        s = m.math('MULTIPLY', s, ramp(m, noise(m, obj, 30.0 + cx * 10.0, 2.0), 0.3, 0.55))
        spills = s if spills is None else m.math('MAXIMUM', spills, s)
    sticky = m.math('MULTIPLY', smooth_less(m, circle_dist(m, sep, -0.36, 0.3), 0.04, 0.02), ramp(m, noise(m, obj, 40.0, 4.0), 0.45, 0.65))
    spills = m.math('MAXIMUM', spills, m.math('MULTIPLY', sticky, 0.3))

    # Cigarette burns: a scorched halo around a charred hole.
    burn_ring, burn_core = None, None
    for cx, cy in ((-0.52, -0.28), (0.71, 0.18)):
        d = circle_dist(m, sep, cx, cy)
        halo = smooth_less(m, d, 0.009, 0.005)
        core_ = smooth_less(m, d, 0.0035, 0.0012)
        burn_ring = halo if burn_ring is None else m.math('MAXIMUM', burn_ring, halo)
        burn_core = core_ if burn_core is None else m.math('MAXIMUM', burn_core, core_)

    paths = felt_print()
    uv = planar(m, sep, 'X', 'Y', -0.35, 0.35, -0.18, 0.18)
    pcol, palpha, _, _ = image_surface(m, paths, uv, extension='CLIP')
    palpha = m.math('MULTIPLY', palpha, m.math('ADD', 0.12, m.math('MULTIPLY', ramp(m, noise(m, obj, 90.0, 4.0), 0.3, 0.7), 0.3)))
    palpha = m.math('MULTIPLY', palpha, m.math('SUBTRACT', 1.0, m.math('MULTIPLY', board, 0.7)))

    green = m.mix(m.math('MULTIPLY', mottle, 0.5), core.hex_linear(0x1d4a31), core.hex_linear(0x245a3b))
    green = m.mix(m.math('MULTIPLY', ramp(m, fade, 0.45, 0.75), 0.18), green, core.hex_linear(0x335a40))
    color = m.mix(m.math('MULTIPLY', fuzz, 0.25), green, core.hex_linear(0x2f6346))
    color = m.mix(palpha, color, pcol)
    color = m.mix(m.math('MULTIPLY', wear, 0.32), color, core.hex_linear(0x4a6b53))
    color = m.mix(m.math('MULTIPLY', pills, 0.35), color, core.hex_linear(0x6f8472))
    color = m.mix(rings, color, core.hex_linear(0x173a27))
    color = m.mix(m.math('MULTIPLY', spills, 0.55), color, core.hex_linear(0x3b3a22))
    color = m.mix(m.math('MULTIPLY', burn_ring, 0.8), color, core.hex_linear(0x2e2412))
    color = m.mix(burn_core, color, core.hex_linear(0x060504))
    rough = m.math('ADD', 0.9, m.math('MULTIPLY', fiber, 0.07))
    rough = m.math('SUBTRACT', rough, m.math('MULTIPLY', wear, 0.12))
    rough = mix_float(m, m.math('MULTIPLY', sticky, 0.8), rough, 0.55)
    m.set('Base Color', color)
    m.set('Roughness', m.math('MINIMUM', rough, 0.98))
    m.set('Metallic', 0.0)
    height = m.math('ADD', m.math('MULTIPLY', fiber, 0.6), m.math('MULTIPLY', pills, 0.8))
    height = m.math('SUBTRACT', height, m.math('MULTIPLY', burn_core, 2.0))
    bump = m.node('ShaderNodeBump', Strength=0.3, Distance=0.00025)
    m.link(height, bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


# ------------------------------------------------------------------ rail

# The rail's profile around its centerline: (height above the felt, outward), inner edge to outer
# edge over the top, then down the outer skirt and back underneath. The sweep keeps these as u.
def rail_profile():
    w = RAIL_W / 2
    pts = [(-0.004, -w), (0.018, -w)]
    n = 24
    for i in range(1, n):
        t = math.pi * (1 - i / n)  # pi (inner) to 0 (outer)
        c, s = math.cos(t), math.sin(t)
        b = w * math.copysign(abs(c) ** 0.55, c)
        a = 0.018 + 0.036 * s ** 0.6
        pts.append((a, b))
    pts += [(0.018, w), (-0.02, w + 0.002), (-0.044, w + 0.001), (-0.046, w - 0.01), (-0.004, w - 0.012)]
    return pts


PROFILE = rail_profile()
U_TOP = 13 / len(PROFILE)           # about where the rail crowns
U_OUTER_EDGE = 24 / len(PROFILE)    # the top-stitch line near the outer shoulder


def rail_material(perimeter):
    m = core.Mat('table_rail')
    tc, sep = object_coords(m)
    obj = tc.outputs['Object']
    uvs = m.node('ShaderNodeSeparateXYZ')
    m.link(tc.outputs['UV'], uvs.inputs['Vector'])
    u, v = uvs.outputs['X'], uvs.outputs['Y']
    along = m.math('MULTIPLY', v, perimeter)  # meters around the table
    pebble = m.node('ShaderNodeTexVoronoi', Scale=2600.0)
    m.link(obj, pebble.inputs['Vector'])
    grain = ramp(m, pebble.outputs['Distance'], 0.1, 0.5)
    # Panel seams across the rail every 0.58 m, and a line of top-stitching around the outer shoulder.
    phase = m.math('FRACT', m.math('DIVIDE', along, 0.58))
    seam = smooth_less(m, m.math('ABSOLUTE', m.math('SUBTRACT', phase, 0.5)), 0.0009, 0.0006)
    stitch_line = smooth_less(m, m.math('ABSOLUTE', m.math('SUBTRACT', u, U_OUTER_EDGE)), 0.004, 0.002)
    dashes = ramp(m, m.math('SINE', m.math('MULTIPLY', along, 2 * math.pi / 0.0045)), 0.0, 0.4)
    stitch = m.math('MULTIPLY', stitch_line, dashes)
    # Forearm wear on the crown in front of each seat: glossy, lighter, then cracked.
    crown = smooth_less(m, m.math('ABSOLUTE', m.math('SUBTRACT', u, U_TOP)), 0.18, 0.1)
    arms = None
    for i, (sx, sy) in enumerate(SEATS):
        if i == 4:
            continue
        a = smooth_less(m, circle_dist(m, sep, sx, sy), 0.26, 0.12)
        arms = a if arms is None else m.math('MAXIMUM', arms, a)
    arms = m.math('MULTIPLY', m.math('MULTIPLY', arms, crown), m.math('ADD', 0.5, m.math('MULTIPLY', noise(m, obj, 25.0, 3.0), 0.7)))
    cracks_v = m.node('ShaderNodeTexVoronoi', Scale=300.0)
    cracks_v.feature = 'DISTANCE_TO_EDGE'
    m.link(obj, cracks_v.inputs['Vector'])
    cracks = m.math('MULTIPLY', smooth_less(m, cracks_v.outputs['Distance'], 0.015, 0.01), ramp(m, arms, 0.62, 0.8))
    cracks = m.math('MULTIPLY', cracks, ramp(m, noise(m, obj, 9.0, 3.0), 0.5, 0.65))
    # A split in the vinyl at the left end, patched with a square of duct tape.
    tape_c = (-HALF_L - RAIL_C * math.cos(math.radians(20)), -RAIL_C * math.sin(math.radians(20)))
    dx = m.math('ABSOLUTE', m.math('SUBTRACT', sep.outputs['X'], tape_c[0]))
    dy = m.math('ABSOLUTE', m.math('SUBTRACT', sep.outputs['Y'], tape_c[1]))
    tape = m.math('MULTIPLY', smooth_less(m, m.math('MAXIMUM', dx, m.math('MULTIPLY', dy, 1.6)), 0.045, 0.0015),
                  ramp(m, sep.outputs['Z'], FELT_Z + 0.01, FELT_Z + 0.02))
    tape_wrinkle = noise(m, scaled(m, sep, 300.0, 60.0, 300.0), 1.0, 3.0)
    # A cigarette burn on the rail by the right end.
    burn = smooth_less(m, circle_dist(m, sep, HALF_L + 0.52, 0.18), 0.006, 0.003)

    base = m.mix(m.math('MULTIPLY', noise(m, obj, 12.0, 3.0), 0.4), core.hex_linear(0x2a1110), core.hex_linear(0x331614))
    color = m.mix(m.math('MULTIPLY', grain, 0.3), base, core.hex_linear(0x1c0a09))
    color = m.mix(m.math('MULTIPLY', arms, 0.45), color, core.hex_linear(0x4a2622))
    color = m.mix(m.math('MULTIPLY', cracks, 0.9), color, core.hex_linear(0x8c7a5e))  # the fabric backing shows
    color = m.mix(m.math('MULTIPLY', seam, 0.8), color, core.hex_linear(0x0a0404))
    color = m.mix(m.math('MULTIPLY', stitch, 0.9), color, core.hex_linear(0x3b2320))
    color = m.mix(burn, color, core.hex_linear(0x0b0706))
    color = m.mix(tape, color, m.mix(tape_wrinkle, core.hex_linear(0x7d8083), core.hex_linear(0x9ea1a4)))
    rough = m.math('ADD', 0.52, m.math('MULTIPLY', grain, 0.12))
    rough = m.math('SUBTRACT', rough, m.math('MULTIPLY', arms, 0.26))
    rough = mix_float(m, cracks, rough, 0.85)
    rough = mix_float(m, tape, rough, 0.42)
    metal = m.math('MULTIPLY', tape, 0.35)
    m.set('Base Color', color)
    m.set('Roughness', rough)
    m.set('Metallic', metal)
    height = m.math('SUBTRACT', m.math('MULTIPLY', grain, 0.25), m.math('ADD', m.math('MULTIPLY', seam, 1.2), m.math('MULTIPLY', cracks, 0.8)))
    height = m.math('ADD', height, m.math('MULTIPLY', stitch, 0.5))
    height = m.math('ADD', height, m.math('MULTIPLY', m.math('MULTIPLY', tape, tape_wrinkle), 0.6))
    bump = m.node('ShaderNodeBump', Strength=0.35, Distance=0.0004)
    m.link(height, bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def steel_material():
    m = core.Mat('table_steel')
    tc, sep = object_coords(m)
    brushed = noise(m, scaled(m, sep, 3000.0, 3000.0, 20.0), 1.0, 2.0)
    grime = ramp(m, noise(m, tc.outputs['Object'], 200.0, 4.0), 0.5, 0.75)
    # Old drinks: a sticky brown film pooled in the bottom of each holder.
    low = m.math('SUBTRACT', 1.0, ramp(m, sep.outputs['Z'], FELT_Z - 0.03, FELT_Z - 0.005))
    color = m.mix(m.math('MULTIPLY', m.math('MAXIMUM', grime, low), 0.8), core.hex_linear(0xb8b9b8), core.hex_linear(0x3b3226))
    m.set('Base Color', color)
    m.set('Roughness', m.math('ADD', 0.28, m.math('ADD', m.math('MULTIPLY', brushed, 0.1), m.math('MULTIPLY', grime, 0.35))))
    m.set('Metallic', m.math('SUBTRACT', 1.0, m.math('MULTIPLY', low, 0.8)))
    return m


def base_material():
    m = core.Mat('table_base')
    tc, sep = object_coords(m)
    color, rough, metal, bump = powder_coat(m, tc, 0x141416, rough=0.5, chips=0.35)
    kick = m.math('MULTIPLY', m.math('SUBTRACT', 1.0, ramp(m, sep.outputs['Z'], 0.02, 0.09)), ramp(m, noise(m, tc.outputs['Object'], 50.0, 4.0), 0.45, 0.6))
    color = m.mix(m.math('MULTIPLY', kick, 0.6), color, core.hex_linear(0x55565a))
    m.set('Base Color', color)
    m.set('Roughness', rough)
    m.set('Metallic', metal)
    m.set('Normal', bump.outputs['Normal'])
    return m


def wood_material():
    """The underside and apron: black-painted plywood, scuffed by knees."""
    m = core.Mat('table_wood')
    tc, sep = object_coords(m)
    paint = noise(m, tc.outputs['Object'], 400.0, 3.0)
    scuff = ramp(m, noise(m, scaled(m, sep, 60.0, 60.0, 8.0), 1.0, 4.0), 0.55, 0.7)
    color = m.mix(m.math('MULTIPLY', scuff, 0.6), core.hex_linear(0x121212), core.hex_linear(0x4a3a2a))
    m.set('Base Color', color)
    m.set('Roughness', m.math('ADD', 0.62, m.math('MULTIPLY', paint, 0.1)))
    return m


# ------------------------------------------------------------------ geometry

def cup_positions(path):
    """A cup holder in the rail 30 cm to the right of each player's seat (none at the dealer's).

    The rail's path runs counterclockwise, which is to the right for everyone facing the table.
    """
    cum = [0.0]
    for i in range(len(path)):
        cum.append(cum[-1] + math.dist(path[i][:2], path[(i + 1) % len(path)][:2]))
    total = cum[-1]
    out = []
    for i, (sx, sy) in enumerate(SEATS):
        if i == 4:
            continue
        k = min(range(len(path)), key=lambda j: math.dist(path[j][:2], (sx, sy)))
        s = (cum[k] + 0.3) % total
        j = next(j for j in range(len(path)) if cum[j] <= s < cum[j + 1])
        t = (s - cum[j]) / (cum[j + 1] - cum[j])
        a, b = path[j], path[(j + 1) % len(path)]
        out.append((a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t))
    return out


def build():
    core.reset()
    path = [(x, y, FELT_Z) for x, y in stadium(HALF_L, RAIL_C, n_end=96, n_side=48)]
    perimeter = sum(math.dist(path[i], path[(i + 1) % len(path)]) for i in range(len(path)))
    felt_mat, rail_mat, steel_mat, base_mat, wood_mat = felt_material(), rail_material(perimeter), steel_material(), base_material(), wood_material()

    felt = core.mesh_object('felt', core.slab(stadium(HALF_L, FELT_R), FELT_Z - 0.004, FELT_Z))
    core.assign(felt, felt_mat)

    rail = core.sweep('rail', path, PROFILE, closed=True)
    rail.data.shade_smooth()
    core.assign(rail, rail_mat)
    # Cup holders: bore the rail, then set a stainless cup in each hole.
    cups = []
    for i, (cx, cy) in enumerate(cup_positions(path)):
        cutter = core.mesh_object('bore', core.slab(core.circle_points(cx, cy, 0.043, 48), FELT_Z - 0.06, FELT_Z + 0.1))
        core.boolean(rail, cutter)
        cup = core.lathe(f'cup_{i}', [(0.0, FELT_Z - 0.045), (0.036, FELT_Z - 0.045), (0.039, FELT_Z - 0.042), (0.041, FELT_Z + 0.048),
                                      (0.047, FELT_Z + 0.052), (0.049, FELT_Z + 0.055), (0.045, FELT_Z + 0.057), (0.0395, FELT_Z + 0.055),
                                      (0.0385, FELT_Z - 0.04), (0.0, FELT_Z - 0.04)], segments=48)
        cup.location = (cx, cy, 0.0)
        core.apply_transforms(cup)
        cups.append(cup)
    cup_obj = core.join(cups, 'cups')
    core.orient_normals(cup_obj)
    core.assign(cup_obj, steel_mat)

    # Plywood top under the felt and rail, and an apron ring below it.
    ply = core.mesh_object('ply', core.slab(stadium(HALF_L, RADIUS - 0.004), PLY_Z0, FELT_Z - 0.004))
    apron = core.extrude_outline('apron', stadium(HALF_L, RADIUS - 0.09, 32, 16), holes=[list(reversed(stadium(HALF_L, RADIUS - 0.11, 32, 16)))],
                                 depth=0.07, z=PLY_Z0 - 0.07)
    wood = core.join([ply, apron], 'wood')
    core.assign(wood, wood_mat)
    core.finish_hard_surface(wood)

    # Two pedestals: a column on a domed disc, with a plate under the top.
    parts = []
    for px in (-PEDESTAL_X, PEDESTAL_X):
        col = core.lathe('pedestal', [(0.0, 0.0), (0.30, 0.0), (0.305, 0.008), (0.29, 0.02), (0.18, 0.05), (0.08, 0.075), (0.058, 0.09),
                                      (0.055, 0.1), (0.055, PLY_Z0 - 0.03), (0.07, PLY_Z0 - 0.018), (0.2, PLY_Z0 - 0.018),
                                      (0.2, PLY_Z0), (0.0, PLY_Z0)], segments=64)
        col.location.x = px
        core.apply_transforms(col)
        parts.append(col)
    base = core.join(parts, 'base')
    core.orient_normals(base)
    core.assign(base, base_mat)
    core.finish_hard_surface(base, 40)

    obj = core.join([felt, rail, cup_obj, wood, base], 'SM_PokerTable')
    is_felt = core.material_is(obj, 'table_felt')
    core.uv_layout(obj, [
        (lambda f: is_felt(f) and f.normal.z > 0.9, 'planar', ('x', 'y', -HALF_L - FELT_R, HALF_L + FELT_R, -FELT_R, FELT_R), (0.0, 0.0, 1.0, 0.97)),
        (is_felt, 'box', None, (0.0, 0.975, 1.0, 1.0)),
        (core.material_is(obj, 'table_rail'), 'keep', None, (0.0, 0.0, 1.0, 1.0)),
        (lambda f: True, 'smart', None, (0.0, 0.0, 1.0, 1.0)),
    ])
    return [obj]
