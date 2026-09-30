"""SM_Desk: the grinder's desk, a solid walnut plank top on a black powder-coated steel frame.

The top is five glued boards with flat-sawn grain. It shows years of all-night sessions:
- the lacquer is worn matte where forearms rest;
- coffee and soda rings sit where the mug and the cans stand in the game;
- there are scratches, dust toward the wall, and scuffed edges.
The frame has chipped paint and plastic leveling feet.

Coordinates (meters), front (the chair side) toward -Y, Z up: the origin is on the floor under the
middle of the top, and the top surface is at 0.75 m. In the stage, prop spots on the desk map to
x = web x, y = DeskZ - web z.
"""
from artkit import core
from artkit.shade import circle_dist, noise, object_coords, powder_coat, ramp, scaled, smooth_less

MESHES = ['SM_Desk']
DOUBLE_SIDED = False
TEXTURE_SIZE = 2048
TEXTURE_SIZES = {'desk_wood': (4096, 2048), 'desk_rubber': 256}
AO_DISTANCE = 0.05

W, D = 1.6, 0.7
TOP_Z = 0.75
THICK = 0.030
BOARDS = 5
LEG = 0.040
LEG_X, LEG_Y = 0.755, 0.305
FOOT = 0.012
RAIL_Z0 = TOP_Z - THICK - 0.060

# Stains where the props stand in the stage: (x, y, radius, kind).
RINGS = [
    (-0.40, -0.06, 0.036, 'coffee'),  # the mug
    (-0.43, -0.03, 0.036, 'coffee'),  # ...and where it stood last week
    (-0.21, -0.16, 0.035, 'coffee'),
    (0.52, 0.20, 0.026, 'soda'),      # can spots 1-3
    (0.61, 0.14, 0.026, 'soda'),
    (0.47, 0.08, 0.026, 'soda'),
    (0.56, 0.02, 0.026, 'soda'),
]

REVIEW_VIEWS = [('front', -32, 20, 2.1), ('top', 8, 56, 1.9), ('detail', -18, 48, 0.2, (-0.38, -0.08, TOP_Z))]


# ------------------------------------------------------------------ materials

def wood_material():
    m = core.Mat('desk_wood')
    tc, sep = object_coords(m)
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    # Which board, and per-board random values.
    bw = D / BOARDS
    bid = m.math('MINIMUM', m.math('MAXIMUM', m.math('FLOOR', m.math('DIVIDE', m.math('ADD', y, D / 2), bw)), 0.0), BOARDS - 1)

    def rnd(k):
        n = m.node('ShaderNodeTexWhiteNoise')
        n.noise_dimensions = '1D'
        m.link(m.math('ADD', m.math('MULTIPLY', bid, 7.13), k * 1.37), n.inputs['W'])
        return n.outputs['Value']

    yl = m.math('SUBTRACT', y, m.math('ADD', -D / 2 + bw / 2, m.math('MULTIPLY', bid, bw)))
    # Flat-sawn boards: the log's axis runs along X, below and slightly tilted to the board, so the
    # growth rings cut the surface in long cathedral arches.
    yc = m.math('MULTIPLY', m.math('SUBTRACT', rnd(1), 0.5), 0.12)
    zc = m.math('ADD', m.math('SUBTRACT', TOP_Z - 0.06, m.math('MULTIPLY', rnd(2), 0.22)),
                m.math('MULTIPLY', x, m.math('MULTIPLY', m.math('SUBTRACT', rnd(3), 0.5), 0.09)))
    dy = m.math('SUBTRACT', yl, yc)
    dz = m.math('SUBTRACT', z, zc)
    r = m.math('SQRT', m.math('ADD', m.math('MULTIPLY', dy, dy), m.math('MULTIPLY', dz, dz)))
    wobble = noise(m, scaled(m, sep, 3.0, 35.0, 35.0), scale=1.0, detail=4.0)
    drift = noise(m, scaled(m, sep, 0.8, 6.0, 6.0), scale=1.0, detail=2.0)
    r = m.math('ADD', r, m.math('MULTIPLY', m.math('SUBTRACT', wobble, 0.5), 0.018))
    r = m.math('ADD', r, m.math('MULTIPLY', m.math('SUBTRACT', drift, 0.5), 0.03))
    freq = m.math('ADD', 170.0, m.math('MULTIPLY', rnd(4), 110.0))
    ring = m.math('FRACT', m.math('MULTIPLY', r, freq))
    late = m.math('MULTIPLY', ramp(m, ring, 0.55, 0.86), m.math('SUBTRACT', 1.0, ramp(m, ring, 0.93, 1.0)))
    tint = m.mix(rnd(5), core.hex_linear(0x5f3e27), core.hex_linear(0x7a5638))
    early = m.mix(0.5, tint, core.hex_linear(0x6e4a30))
    # Walnut drifts between warm and cool browns across a board.
    figure = noise(m, scaled(m, sep, 2.0, 9.0, 9.0), scale=1.0, detail=3.0)
    early = m.mix(m.math('MULTIPLY', ramp(m, figure, 0.35, 0.7), 0.6), early, core.hex_linear(0x5c3d2c))
    color = m.mix(m.math('MULTIPLY', late, 0.55), early, core.hex_linear(0x472c19))
    # Pores: dark streaks along the grain, about a millimeter apart (what a 2.5 px/mm bake can hold).
    pores = ramp(m, noise(m, scaled(m, sep, 12.0, 900.0, 900.0), detail=2.0), 0.6, 0.72)
    color = m.mix(m.math('MULTIPLY', pores, 0.4), color, core.hex_linear(0x2a180c))
    # Board seams: a hairline glue line with the slightest groove.
    seam = m.math('MULTIPLY', smooth_less(m, m.math('SUBTRACT', bw / 2, m.math('ABSOLUTE', yl)), 0.00025, 0.00015),
                  m.math('LESS_THAN', m.math('ABSOLUTE', y), D / 2 - 0.004))
    color = m.mix(m.math('MULTIPLY', seam, 0.7), color, core.hex_linear(0x24150b))

    top = m.math('GREATER_THAN', z, TOP_Z - 0.0015)
    rough = m.math('ADD', 0.34, m.math('MULTIPLY', noise(m, tc.outputs['Object'], scale=9.0, detail=3.0), 0.06))
    rough = m.math('ADD', rough, m.math('MULTIPLY', pores, 0.08))
    # Forearms have worn the lacquer matte along the front, either side of where the laptop sits.
    arms = m.math('MULTIPLY', ramp(m, m.math('MULTIPLY', y, -1.0), 0.17, 0.33),
                  m.math('SUBTRACT', 1.0, ramp(m, m.math('ABSOLUTE', x), 0.42, 0.62)))
    arms = m.math('MULTIPLY', m.math('MULTIPLY', arms, top), ramp(m, noise(m, tc.outputs['Object'], scale=14.0, detail=4.0), 0.3, 0.7))
    rough = m.math('ADD', rough, m.math('MULTIPLY', arms, 0.22))
    color = m.mix(m.math('MULTIPLY', arms, 0.35), color, core.hex_linear(0x8a6a50))
    # Edge wear: the lacquer and stain rubbed through on the top edges, most along the front.
    edge_d = m.math('MINIMUM', m.math('SUBTRACT', W / 2, m.math('ABSOLUTE', x)), m.math('SUBTRACT', D / 2, m.math('ABSOLUTE', y)))
    edge = m.math('MULTIPLY', smooth_less(m, edge_d, 0.0035, 0.002), m.math('GREATER_THAN', z, TOP_Z - 0.006))
    edge = m.math('MULTIPLY', edge, ramp(m, noise(m, tc.outputs['Object'], scale=60.0, detail=5.0), 0.42, 0.62))
    edge = m.math('MULTIPLY', edge, m.math('ADD', 0.45, m.math('MULTIPLY', ramp(m, m.math('MULTIPLY', y, -1.0), 0.2, 0.35), 0.55)))
    color = m.mix(m.math('MULTIPLY', edge, 0.7), color, core.hex_linear(0x9b7856))
    rough = m.math('ADD', rough, m.math('MULTIPLY', edge, 0.25))
    # Scratches: two sparse layers of thin lines, mostly along the boards.
    scratch_total = None
    for angle, sx, sy, seed in ((0.12, 7.0, 260.0, 0.0), (-0.6, 5.0, 180.0, 3.1)):
        mp = m.node('ShaderNodeMapping')
        mp.inputs['Rotation'].default_value = (0.0, 0.0, angle)
        mp.inputs['Scale'].default_value = (sx, sy, 1.0)
        mp.inputs['Location'].default_value = (seed, seed * 0.7, 0.0)
        m.link(tc.outputs['Object'], mp.inputs['Vector'])
        vor = m.node('ShaderNodeTexVoronoi')
        vor.feature = 'DISTANCE_TO_EDGE'
        m.link(mp.outputs['Vector'], vor.inputs['Vector'])
        lines = smooth_less(m, vor.outputs['Distance'], 0.018, 0.012)
        sparse = ramp(m, noise(m, tc.outputs['Object'], scale=6.0 + seed, detail=2.0), 0.55, 0.7)
        layer = m.math('MULTIPLY', lines, sparse)
        scratch_total = layer if scratch_total is None else m.math('MAXIMUM', scratch_total, layer)
    scratch = m.math('MULTIPLY', m.math('MULTIPLY', scratch_total, top), 0.8)
    color = m.mix(m.math('MULTIPLY', scratch, 0.45), color, core.hex_linear(0xa3845f))
    rough = m.math('ADD', rough, m.math('MULTIPLY', scratch, 0.18))
    # Rings: coffee leaves a brown line; cold cans leave a hazy white bloom in the lacquer.
    breakup = noise(m, tc.outputs['Object'], scale=40.0, detail=3.0)
    for cx, cy, rad, kind in RINGS:
        d = circle_dist(m, sep, cx, cy)
        wobble_r = m.math('ADD', rad, m.math('MULTIPLY', m.math('SUBTRACT', breakup, 0.5), 0.004))
        # The line swells and thins where the liquid pooled against the base.
        width = m.math('ADD', 0.0005, m.math('MULTIPLY', noise(m, tc.outputs['Object'], scale=70.0 + cy * 20.0, detail=2.0), 0.0011))
        line = ramp(m, m.math('SUBTRACT', width, m.math('ABSOLUTE', m.math('SUBTRACT', d, wobble_r))), -0.0003, 0.0003)
        fill = m.math('MULTIPLY', smooth_less(m, d, rad, 0.002), 0.25)
        partial = ramp(m, noise(m, tc.outputs['Object'], scale=22.0 + cx * 10.0, detail=2.0), 0.35, 0.55)
        stain = m.math('MULTIPLY', m.math('MULTIPLY', m.math('MAXIMUM', line, fill), partial), top)
        if kind == 'coffee':
            color = m.mix(m.math('MULTIPLY', stain, 0.75), color, core.hex_linear(0x2b170a))
            rough = m.math('SUBTRACT', rough, m.math('MULTIPLY', stain, 0.08))
        else:
            color = m.mix(m.math('MULTIPLY', stain, 0.4), color, core.hex_linear(0xb9ab9a))
            rough = m.math('ADD', rough, m.math('MULTIPLY', stain, 0.25))
    # Dust gathers toward the wall, where nothing moves.
    dust = m.math('MULTIPLY', m.math('MULTIPLY', ramp(m, y, 0.22, 0.345), top), ramp(m, noise(m, tc.outputs['Object'], scale=160.0, detail=3.0), 0.35, 0.8))
    color = m.mix(m.math('MULTIPLY', dust, 0.16), color, core.hex_linear(0x8f8a83))
    rough = m.math('ADD', rough, m.math('MULTIPLY', dust, 0.18))
    m.set('Base Color', color)
    m.set('Roughness', m.math('MINIMUM', rough, 0.95))
    m.set('Metallic', 0.0)
    height = m.math('ADD', m.math('MULTIPLY', pores, -0.4), m.math('MULTIPLY', seam, -1.0))
    height = m.math('SUBTRACT', height, m.math('MULTIPLY', scratch, 0.6))
    bump = m.node('ShaderNodeBump', Strength=0.25, Distance=0.00004)
    m.link(height, bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def steel_material():
    m = core.Mat('desk_steel')
    tc, sep = object_coords(m)
    color, rough, metal, bump = powder_coat(m, tc, 0x18191c, rough=0.55)
    # Scuffs from shoes and the chair's casters low on the legs.
    low = m.math('MULTIPLY', m.math('SUBTRACT', 1.0, ramp(m, sep.outputs['Z'], 0.05, 0.3)),
                 ramp(m, noise(m, scaled(m, sep, 40.0, 40.0, 4.0), detail=3.0), 0.55, 0.7))
    color = m.mix(m.math('MULTIPLY', low, 0.5), color, core.hex_linear(0x4a4b4e))
    rough = m.math('ADD', rough, m.math('MULTIPLY', low, 0.15))
    m.set('Base Color', color)
    m.set('Roughness', rough)
    m.set('Metallic', metal)
    m.set('Normal', bump.outputs['Normal'])
    return m


def rubber_material():
    m = core.Mat('desk_rubber')
    m.set('Base Color', core.hex_linear(0x0d0d0e))
    m.set('Roughness', 0.78)
    return m


# ------------------------------------------------------------------ geometry

def box(name, x0, y0, x1, y1, z0, z1, r=0.0, bevel_z=0.0):
    """A block with rounded vertical edges (r) and optionally bevelled top and bottom edges."""
    outline = core.rounded_rect(x0, y0, x1, y1, r, steps=4) if r > 0 else [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]
    bm = core.slab(outline, z0, z1)
    if bevel_z > 0:
        core.bevel(bm, lambda e: all(abs(v.co.z - z1) < 1e-7 for v in e.verts) or all(abs(v.co.z - z0) < 1e-7 for v in e.verts),
                   bevel_z, segments=2)
    return core.mesh_object(name, bm)


def build():
    core.reset()
    wood, steel, rubber = wood_material(), steel_material(), rubber_material()

    bm = core.slab(core.rounded_rect(-W / 2, -D / 2, W / 2, D / 2, 0.006, steps=4), TOP_Z - THICK, TOP_Z)
    core.bevel(bm, lambda e: all(abs(v.co.z - TOP_Z) < 1e-7 for v in e.verts), 0.004, segments=4)
    core.bevel(bm, lambda e: all(abs(v.co.z - (TOP_Z - THICK)) < 1e-7 for v in e.verts), 0.0015, segments=2)
    top = core.mesh_object('top', bm)
    core.assign(top, wood)
    core.finish_hard_surface(top)

    frame = []
    for sx in (-1, 1):
        for sy in (-1, 1):
            cx, cy = sx * LEG_X, sy * LEG_Y
            frame.append(box('leg', cx - LEG / 2, cy - LEG / 2, cx + LEG / 2, cy + LEG / 2, FOOT, TOP_Z - THICK, r=0.003))
        # Side rail joining the front and back legs, just under the top.
        frame.append(box('side', sx * LEG_X - 0.010, -LEG_Y + LEG / 2, sx * LEG_X + 0.010, LEG_Y - LEG / 2, RAIL_Z0, TOP_Z - THICK,
                         r=0.002, bevel_z=0.001))
    frame.append(box('back', -LEG_X + LEG / 2, LEG_Y - 0.010, LEG_X - LEG / 2, LEG_Y + 0.010, RAIL_Z0, TOP_Z - THICK, r=0.002, bevel_z=0.001))
    # A lower stretcher at the back keeps it from racking, and gives the chair something to bump.
    frame.append(box('stretcher', -LEG_X + LEG / 2, LEG_Y - 0.008, LEG_X - LEG / 2, LEG_Y + 0.008, 0.16, 0.19, r=0.002, bevel_z=0.001))
    steel_obj = core.join(frame, 'frame')
    core.assign(steel_obj, steel)
    core.finish_hard_surface(steel_obj)

    feet = []
    for sx in (-1, 1):
        for sy in (-1, 1):
            cx, cy = sx * LEG_X, sy * LEG_Y
            feet.append(box('foot', cx - 0.018, cy - 0.018, cx + 0.018, cy + 0.018, 0.0, FOOT, r=0.004, bevel_z=0.002))
    feet_obj = core.join(feet, 'feet')
    core.assign(feet_obj, rubber)
    core.finish_hard_surface(feet_obj)

    obj = core.join([top, steel_obj, feet_obj], 'SM_Desk')
    is_wood = core.material_is(obj, 'desk_wood')
    core.uv_layout(obj, [
        (lambda f: is_wood(f) and f.normal.z > 0.9 and f.calc_center_median().z > TOP_Z - 0.002, 'planar',
         ('x', 'y', -W / 2, W / 2, -D / 2, D / 2), (0.0, 0.2, 1.0, 1.0)),
        (lambda f: is_wood(f) and f.normal.z < -0.9, 'box', None, (0.62, 0.0, 1.0, 0.19)),
        (is_wood, 'box', None, (0.0, 0.0, 0.61, 0.19)),
        (core.material_is(obj, 'desk_steel'), 'box', None, (0.0, 0.0, 1.0, 1.0)),
        (lambda f: True, 'box', None, (0.0, 0.0, 1.0, 1.0)),
    ])
    return [obj]
