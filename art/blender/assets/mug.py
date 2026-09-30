"""SM_Mug: a RiverLine swag mug, 11 oz stoneware, half full of cold coffee.

- White glaze with the site's logo facing the player and the handle on the left, as the stage places it.
- Unglazed foot ring and underside.
- A dried drip down the side, rings inside where the level sat on other nights, and a chip in the rim.

Coordinates (meters): origin at the center of the foot on the desk, Z up; the print faces -Y and
the handle points to -X.
"""
import math

from artkit import core
from artkit.shade import image_surface, mix_float, noise, object_coords, ramp, smooth_less
from artkit.sheet import Sheet

MESHES = ['SM_Mug']
DOUBLE_SIDED = False
TEXTURE_SIZE = 2048
TEXTURE_SIZES = {'mug_coffee': 512}
AO_DISTANCE = 0.02

R_OUT = 0.0412
R_IN = 0.0376
PRINT_Z0, PRINT_Z1 = 0.020, 0.088
COFFEE_Z = 0.077
NAVY = 0x1b2a3a
TEAL = 0x16b39a
GLAZE = 0xf1eee7
STONEWARE = 0xcbbca4

REVIEW_VIEWS = [('front', -25, 18, 2.6), ('top', 20, 62, 2.4), ('detail', -40, 48, 1.1, (-0.012, -0.018, 0.088))]

# Outer profile up over the rim and down the inside, bottom center to inner center: (radius, z).
PROFILE = [
    (0.0, 0.0028), (0.026, 0.0028), (0.0285, 0.0012), (0.0300, 0.0), (0.0335, 0.0), (0.0366, 0.0013), (0.0394, 0.0046),
    (0.0407, 0.0100), (0.0412, 0.0200), (0.0412, 0.0880), (0.0411, 0.0925), (0.0405, 0.0948), (0.0396, 0.0955),
    (0.0386, 0.0952), (0.0380, 0.0940), (0.0378, 0.0900), (0.0376, 0.0200), (0.0368, 0.0125), (0.0340, 0.0092),
    (0.0280, 0.0080), (0.0, 0.0078),
]


def print_sheet():
    """The wrap-around print (millimeters): circumference wide, PRINT_Z0..PRINT_Z1 tall; x runs counterclockwise from -X."""
    w = 2 * math.pi * R_OUT * 1000
    h = (PRINT_Z1 - PRINT_Z0) * 1000
    s = Sheet(w, h)
    cx = w * 0.25  # facing -Y
    # The wave mark: two teal swells, as in the client's logo.
    for k, dy in enumerate((0.0, -5.2)):
        top, bot = [], []
        for i in range(41):
            t = i / 40
            x = cx - 14 + 28 * t
            y = 47 + dy + 2.6 * math.sin(t * 2 * math.pi * 1.15 + 0.4)
            top.append((x, y + 1.35))
            bot.append((x, y - 1.35))
        s.poly(top + list(reversed(bot)), TEAL if k == 0 else 0x0f8a78, rough=0.08)
    s.text('River', cx + 1.8, 26, 12.5, NAVY, face='Bold', align='RIGHT', rough=0.08)
    s.text('Line', cx + 1.8, 26, 12.5, TEAL, face='Bold', rough=0.08)
    s.text('HIGH STAKES. LOW RAKE.', cx, 18, 3.1, 0x6b7785, face='Bold', align='CENTER', tracking=1.35, rough=0.08)
    back = w * 0.75
    s.text('gg', back, 38, 16.0, NAVY, face='Black', align='CENTER', rough=0.08)
    s.text('RIVERLINE POKER', back, 30, 3.0, 0x6b7785, face='Bold', align='CENTER', tracking=1.4, rough=0.08)
    return s.render('mug_print', 2048)


def vec_z(m, z, scale):
    """A 1D coordinate along the mug's height, for noise that varies only with z."""
    n = m.node('ShaderNodeCombineXYZ')
    m.link(m.math('MULTIPLY', z, scale), n.inputs['Z'])
    return n.outputs['Vector']


def glaze_material(printed):
    m = core.Mat('mug_glaze')
    tc, sep = object_coords(m)
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    r = m.math('SQRT', m.math('ADD', m.math('MULTIPLY', x, x), m.math('MULTIPLY', y, y)))
    u = m.math('ADD', m.math('DIVIDE', m.math('ARCTAN2', y, x), 2 * math.pi), 0.5)
    uv = m.node('ShaderNodeCombineXYZ')
    m.link(u, uv.inputs['X'])
    m.link(m.math('DIVIDE', m.math('SUBTRACT', z, PRINT_Z0), PRINT_Z1 - PRINT_Z0), uv.inputs['Y'])
    ink, alpha, _, ink_rough = image_surface(m, printed, uv.outputs['Vector'], extension='CLIP')
    outer = m.math('MULTIPLY', m.math('GREATER_THAN', r, R_OUT - 0.0009), m.math('LESS_THAN', r, R_OUT + 0.0006))
    on_print = m.math('MULTIPLY', alpha, outer)
    # Glaze: warm white, glassy, with the faint waviness of a dipped finish.
    wave = noise(m, tc.outputs['Object'], scale=45.0, detail=3.0)
    color = m.mix(on_print, core.hex_linear(GLAZE), ink)
    rough = m.math('ADD', 0.05, m.math('MULTIPLY', wave, 0.03))
    rough = mix_float(m, on_print, rough, ink_rough)
    # Unglazed foot and underside.
    foot = ramp(m, m.math('MULTIPLY', z, -1.0), -0.0042, -0.0030)
    speck = noise(m, tc.outputs['Object'], scale=900.0, detail=2.0)
    color = m.mix(foot, color, m.mix(ramp(m, speck, 0.55, 0.7), core.hex_linear(STONEWARE), core.hex_linear(0x8f8270)))
    rough = mix_float(m, foot, rough, 0.82)
    # Inside: tide lines where the coffee level sat on other nights.
    inside = m.math('LESS_THAN', r, R_IN + 0.0006)
    tide = None
    for level, strength in ((COFFEE_Z + 0.0012, 0.8), (0.0852, 0.22)):
        # Each line wanders a little around the cup, as the mug never sat quite level.
        wander = m.math('MULTIPLY', m.math('SUBTRACT', noise(m, tc.outputs['Object'], scale=35.0 + level * 100, detail=2.0), 0.5), 0.0016)
        band = m.math('MULTIPLY', smooth_less(m, m.math('ABSOLUTE', m.math('SUBTRACT', m.math('ADD', z, wander), level)), 0.0006, 0.0005), strength)
        tide = band if tide is None else m.math('MAXIMUM', tide, band)
    haze = m.math('MULTIPLY', ramp(m, m.math('MULTIPLY', z, -1.0), -COFFEE_Z - 0.006, -COFFEE_Z), 0.25)  # tint just above the coffee
    stain = m.math('MULTIPLY', m.math('MAXIMUM', tide, haze), inside)
    stain = m.math('MULTIPLY', stain, ramp(m, noise(m, tc.outputs['Object'], scale=90.0, detail=3.0), 0.3, 0.65))
    color = m.mix(m.math('MINIMUM', stain, 1.0), color, core.hex_linear(0x6b4222))
    # A dried drip down the outside from the rim: it meanders, thins as it runs, and ends in a bead.
    wander = m.math('MULTIPLY', m.math('SUBTRACT', noise(m, vec_z(m, z, 90.0), scale=1.0, detail=2.0), 0.5), 0.012)
    du = m.math('ABSOLUTE', m.math('SUBTRACT', m.math('ADD', u, wander), 0.31))
    along = ramp(m, z, 0.066, 0.094)  # 0 at the tip, 1 at the rim
    drip_w = m.math('ADD', 0.0022, m.math('MULTIPLY', along, 0.0035))
    body = m.math('MULTIPLY', ramp(m, m.math('SUBTRACT', drip_w, du), 0.0, 0.0012), ramp(m, z, 0.0655, 0.0675))
    bead_d = m.math('ADD', m.math('MULTIPLY', m.math('DIVIDE', du, 0.0034), m.math('DIVIDE', du, 0.0034)),
                    m.math('MULTIPLY', m.math('DIVIDE', m.math('SUBTRACT', z, 0.0668), 0.0022), m.math('DIVIDE', m.math('SUBTRACT', z, 0.0668), 0.0022)))
    bead = ramp(m, m.math('SUBTRACT', 1.0, bead_d), 0.0, 0.25)
    drip = m.math('MULTIPLY', m.math('MAXIMUM', body, bead), outer)
    # Dried coffee: translucent tan in the thin run, darker in the bead and along its edges.
    inner = ramp(m, m.math('SUBTRACT', drip_w, du), 0.0008, 0.002)
    edge = m.math('MULTIPLY', body, m.math('SUBTRACT', 1.0, inner))
    tone = m.mix(m.math('MAXIMUM', bead, edge), core.hex_linear(0xa77b52), core.hex_linear(0x5a3417))
    color = m.mix(m.math('MULTIPLY', drip, 0.8), color, tone)
    rough = m.math('ADD', rough, m.math('MULTIPLY', drip, 0.05))
    # A chip out of the rim shows the stoneware body.
    chip_at = (R_OUT * 0.99 * math.cos(math.radians(-137)), R_OUT * 0.99 * math.sin(math.radians(-137)), 0.0952)
    cd = m.node('ShaderNodeVectorMath')
    cd.operation = 'DISTANCE'
    m.link(tc.outputs['Object'], cd.inputs[0])
    cd.inputs[1].default_value = chip_at
    ragged = m.math('ADD', 0.0024, m.math('MULTIPLY', m.math('SUBTRACT', noise(m, tc.outputs['Object'], scale=1500.0, detail=3.0), 0.5), 0.0014))
    chip = ramp(m, m.math('SUBTRACT', ragged, cd.outputs['Value']), 0.0, 0.0003)
    color = m.mix(chip, color, m.mix(ramp(m, speck, 0.55, 0.7), core.hex_linear(STONEWARE), core.hex_linear(0x8f8270)))
    rough = mix_float(m, chip, rough, 0.75)
    m.set('Base Color', color)
    m.set('Roughness', rough)
    m.set('Metallic', 0.0)
    height = m.math('ADD', m.math('MULTIPLY', wave, 0.3), m.math('MULTIPLY', chip, -2.0))
    bump = m.node('ShaderNodeBump', Strength=0.06, Distance=0.0002)
    m.link(height, bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def coffee_material():
    m = core.Mat('mug_coffee')
    tc, sep = object_coords(m)
    r = m.math('SQRT', m.math('ADD', m.math('MULTIPLY', sep.outputs['X'], sep.outputs['X']), m.math('MULTIPLY', sep.outputs['Y'], sep.outputs['Y'])))
    # A ring of fine bubbles where the coffee meets the cup, gone patchy as it cooled.
    rim = m.math('MULTIPLY', ramp(m, r, R_IN - 0.0035, R_IN - 0.0004), ramp(m, noise(m, tc.outputs['Object'], scale=700.0, detail=2.0), 0.45, 0.62))
    color = m.mix(m.math('MULTIPLY', rim, 0.45), core.hex_linear(0x170b05), core.hex_linear(0x7a5433))
    m.set('Base Color', color)
    m.set('Roughness', mix_float(m, rim, 0.03, 0.35))
    m.set('IOR', 1.34)
    return m


def build():
    core.reset()
    printed = print_sheet()
    core.reset()
    glaze, coffee = glaze_material(printed), coffee_material()

    body = core.lathe('body', PROFILE, segments=112)
    core.orient_normals(body)
    core.assign(body, glaze)

    # The handle: an oval section swept around a D-shaped loop; both ends sink into the wall.
    ctrl = [(-0.0385, 0.0805), (-0.0480, 0.0835), (-0.0585, 0.0805), (-0.0655, 0.0712), (-0.0675, 0.0585),
            (-0.0662, 0.0458), (-0.0612, 0.0352), (-0.0522, 0.0284), (-0.0385, 0.0262)]
    path = [(px, 0.0, pz) for px, pz in core.catmull_rom(ctrl, 8)]
    oval = [(0.0046 * math.cos(2 * math.pi * k / 24), 0.0066 * math.sin(2 * math.pi * k / 24)) for k in range(24)]
    handle = core.sweep('handle', path, oval)
    core.assign(handle, glaze)

    liquid = core.lathe('coffee', [(0.0, COFFEE_Z), (R_IN - 0.0012, COFFEE_Z), (R_IN + 0.0003, COFFEE_Z + 0.0006)], segments=112)
    core.orient_normals(liquid, toward=(0.0, 0.0, 1.0))
    core.assign(liquid, coffee)

    # UVs per part before joining: the body keeps its lathe unwrap, the handle is box-projected
    # below it, and the coffee has a texture of its own.
    core.uv_layout(body, [(lambda f: True, 'keep', None, (0.0, 0.26, 1.0, 1.0))])
    core.uv_layout(handle, [(lambda f: True, 'box', None, (0.0, 0.0, 1.0, 0.25))])
    core.uv_layout(liquid, [(lambda f: True, 'planar', ('x', 'y', -R_IN, R_IN, -R_IN, R_IN), (0.0, 0.0, 1.0, 1.0))])
    for part in (body, handle, liquid):
        core.finish_hard_surface(part, 60)
    return [core.join([body, handle, liquid], 'SM_Mug')]
