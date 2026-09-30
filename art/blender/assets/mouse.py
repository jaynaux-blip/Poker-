"""SM_Mouse and SM_MousePad: the grinder's wireless mouse on a RiverLine swag pad.

The mouse is a KESTREL (the laptop's brand) in graphite plastic: a lofted shell, split buttons, a
rubber scroll wheel in its slot, rubber side grips and PTFE feet. Years of clicking left gloss where
the index and middle fingers rest and where the palm sits.

The pad is 260 x 210 mm cloth on rubber with a stitched edge. The pile is flattened and pilled where
the mouse lives, a coffee ring dried near the top, and the RiverLine print in the corner is wearing off.

Coordinates (meters): each origin is centered on its footprint at the bottom, Z up. The mouse's
buttons point +Y (away from the player); the pad's printed corner is bottom right as the player sees it.
"""
import math

from artkit import core
from artkit.shade import circle_dist, image_surface, mix_float, noise, object_coords, planar, ramp, smooth_less
from artkit.sheet import Sheet

MESHES = ['SM_Mouse', 'SM_MousePad']
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {'pad_cloth': 2048, 'pad_rubber': 256, 'pad_thread': 512, 'mouse_shell': 2048, 'mouse_wheel': 512, 'mouse_feet': 128}
AO_DISTANCE = 0.004

# Mouse: length along Y, planform half-width W(s) and shell height H(s), s from -1 (palm) to +1 (buttons).
LENGTH = 0.106
FOOT = 0.0009  # PTFE feet lift the shell off the pad
SPLIT_S = 0.12  # where the buttons begin
WHEEL_S = 0.52
WHEEL_R, WHEEL_W = 0.0102, 0.0066

# Pad.
PAD_W, PAD_D, PAD_R = 0.26, 0.21, 0.011
PAD_T, CLOTH_T = 0.0030, 0.0008
MOUSE_ON_PAD = (0.01, -0.02)  # where the mouse rests, as the stage places it

REVIEW_VIEWS = [('front', -35, 26, 2.2), ('top', 10, 64, 2.0), ('detail', -40, 36, 0.8, (0.01, -0.02, 0.03))]


def end_factor(s, radius):
    """Rounds the ends in plan: 1 away from the tips, falling along a circle of the given radius to 0 at them."""
    d = (1.0 - abs(s)) * LENGTH / 2
    if d >= radius:
        return 1.0
    return math.sqrt(max(0.0, 1.0 - ((radius - d) / radius) ** 2))


def half_width(s):
    """Planform half-width: rounded palm, a slight waist, fuller under the fingers."""
    base = 0.0298 - 0.0009 * math.exp(-((s - 0.05) / 0.35) ** 2) + 0.0004 * s
    return base * end_factor(s, 0.026)


# Shell height above the feet along the length: low heel, the palm hump behind the middle, a
# gentle slope down the buttons to a blunt nose.
HEIGHT_KEYS = [(-1.0, 0.013), (-0.75, 0.025), (-0.4, 0.0340), (-0.15, 0.0355), (0.2, 0.0310), (0.6, 0.0225), (1.0, 0.0140)]


def smooth_keys(keys, s):
    """Cubic Hermite through (s, value) keys with Catmull-Rom slopes: no flat spots at the keys."""
    n = len(keys)
    s = max(keys[0][0], min(keys[-1][0], s))
    i = next(k for k in range(n - 1) if s <= keys[k + 1][0])
    (s0, v0), (s1, v1) = keys[i], keys[i + 1]

    def slope(k):
        a, b = keys[max(k - 1, 0)], keys[min(k + 1, n - 1)]
        return (b[1] - a[1]) / (b[0] - a[0])

    h = s1 - s0
    t = (s - s0) / h
    m0, m1 = slope(i) * h, slope(i + 1) * h
    return ((2 * t ** 3 - 3 * t ** 2 + 1) * v0 + (t ** 3 - 2 * t ** 2 + t) * m0 + (-2 * t ** 3 + 3 * t ** 2) * v1 + (t ** 3 - t ** 2) * m1)


def height(s):
    # The front and back walls stay nearly upright, rounding over only at the very tips.
    return max(smooth_keys(HEIGHT_KEYS, s) * end_factor(s, 0.026) ** 0.35, 0.0008)


def section(s, n=56, bottom=8):
    """A ring around the shell at station s: a superellipse arch over a flat bottom."""
    y = s * LENGTH / 2
    w, h = half_width(s), height(s)
    pts = []
    for i in range(n + 1):
        t = math.pi * i / n  # 0 on the right, pi on the left
        c, sn = math.cos(t), math.sin(t)
        # Fuller shoulders than an ellipse; the walls flare a little toward the base.
        x = w * math.copysign(abs(c) ** 0.72, c) * (1.0 + 0.04 * (1 - sn))
        z = FOOT + h * sn ** 0.85
        pts.append((x, y, z))
    for i in range(1, bottom):
        x = -w + 2 * w * i / bottom
        pts.append((x, y, FOOT))
    return pts


def shell_material():
    m = core.Mat('mouse_shell')
    tc, sep = object_coords(m)
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    ax = m.math('ABSOLUTE', x)
    y_split = SPLIT_S * LENGTH / 2
    # Button seams: down the middle from the split, and an arc across the top that bows forward at the sides.
    center_seam = m.math('MULTIPLY', smooth_less(m, ax, 0.00022, 0.00012), ramp(m, y, y_split - 0.0003, y_split + 0.0003))
    arc = m.math('ADD', y_split, m.math('MULTIPLY', m.math('MULTIPLY', x, x), 9.0))
    cross_seam = m.math('MULTIPLY', smooth_less(m, m.math('ABSOLUTE', m.math('SUBTRACT', y, arc)), 0.00022, 0.00012), ramp(m, z, 0.014, 0.018))
    seam = m.math('MAXIMUM', center_seam, cross_seam)
    # Base seam where the shell meets the bottom plate.
    base_seam = smooth_less(m, m.math('ABSOLUTE', m.math('SUBTRACT', z, FOOT + 0.0032)), 0.00022, 0.00012)
    seam = m.math('MAXIMUM', seam, base_seam)

    # Rubber side grips: a band low on each flank under the thumb and ring finger.
    grip = m.math('MULTIPLY', m.math('MULTIPLY', ramp(m, z, FOOT + 0.0045, FOOT + 0.0060), m.math('SUBTRACT', 1.0, ramp(m, z, 0.019, 0.022))),
                  m.math('MULTIPLY', ramp(m, y, -0.036, -0.030), m.math('SUBTRACT', 1.0, ramp(m, y, 0.018, 0.024))))
    grip = m.math('MULTIPLY', grip, ramp(m, ax, 0.0215, 0.0235))
    dimples = m.node('ShaderNodeTexVoronoi', Scale=1300.0)
    m.link(tc.outputs['Object'], dimples.inputs['Vector'])
    dimple = ramp(m, dimples.outputs['Distance'], 0.15, 0.45)

    # Wear: glossy where fingertips and the palm rest.
    tip_l = smooth_less(m, circle_dist(m, sep, -0.0125, 0.030), 0.006, 0.004)
    tip_r = smooth_less(m, circle_dist(m, sep, 0.0125, 0.031), 0.006, 0.004)
    palm = smooth_less(m, circle_dist(m, sep, 0.0, -0.022), 0.017, 0.01)
    wear = m.math('MAXIMUM', m.math('MAXIMUM', tip_l, tip_r), m.math('MULTIPLY', palm, 0.8))
    wear = m.math('MULTIPLY', wear, m.math('ADD', 0.55, m.math('MULTIPLY', noise(m, tc.outputs['Object'], 260.0, 4.0), 0.7)))
    wear = m.math('MULTIPLY', wear, ramp(m, z, 0.013, 0.017))

    # The wordmark on the palm rest, mostly intact.
    logo = Sheet(34, 8)
    logo.text('KESTREL', 17, 2.2, 5.2, 0x8e9197, face='Bold', align='CENTER', tracking=1.6, rough=0.3)
    paths = logo.render('mouse_logo', 512)
    uv = planar(m, sep, 'X', 'Y', -0.0085, 0.0085, -0.0395, -0.0355)
    lcol, lalpha, _, _ = image_surface(m, paths, uv, extension='CLIP')
    lalpha = m.math('MULTIPLY', lalpha, ramp(m, noise(m, tc.outputs['Object'], 900.0, 3.0), 0.25, 0.4))

    speck = noise(m, tc.outputs['Object'], 3200.0, 1.0)
    color = m.mix(grip, core.hex_linear(0x1f2023), core.hex_linear(0x161719))
    color = m.mix(m.math('MULTIPLY', wear, 0.12), color, core.hex_linear(0x2a2c30))
    color = m.mix(m.math('MULTIPLY', seam, 0.85), color, core.hex_linear(0x060607))
    color = m.mix(lalpha, color, lcol)
    rough = m.math('ADD', 0.56, m.math('MULTIPLY', m.math('SUBTRACT', speck, 0.5), 0.1))
    rough = mix_float(m, grip, rough, m.math('ADD', 0.78, m.math('MULTIPLY', dimple, 0.08)))
    rough = mix_float(m, m.math('MULTIPLY', wear, 0.8), rough, 0.34)
    rough = mix_float(m, lalpha, rough, 0.34)
    m.set('Base Color', color)
    m.set('Roughness', rough)
    m.set('Metallic', 0.0)
    height_map = m.math('SUBTRACT', m.math('MULTIPLY', speck, 0.15), m.math('MULTIPLY', seam, 1.0))
    height_map = m.math('ADD', height_map, m.math('MULTIPLY', m.math('MULTIPLY', grip, dimple), 0.5))
    bump = m.node('ShaderNodeBump', Strength=0.35, Distance=0.00025)
    m.link(height_map, bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def wheel_material():
    m = core.Mat('mouse_wheel')
    tc, sep = object_coords(m)
    # Tread ribs run across the tire: 30 around the circumference (the wheel turns about X).
    ang = m.math('ARCTAN2', sep.outputs['Z'], sep.outputs['Y'])
    rib = m.math('ABSOLUTE', m.math('SINE', m.math('MULTIPLY', ang, 30.0)))
    grime = ramp(m, noise(m, tc.outputs['Object'], 700.0, 3.0), 0.55, 0.7)
    m.set('Base Color', m.mix(grime, core.hex_linear(0x3a3c40), core.hex_linear(0x55534e)))
    m.set('Roughness', m.math('ADD', 0.62, m.math('MULTIPLY', rib, 0.14)))
    m.set('Metallic', 0.0)
    bump = m.node('ShaderNodeBump', Strength=0.9, Distance=0.0003)
    m.link(rib, bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def build_mouse():
    shell_mat, wheel_mat = shell_material(), wheel_material()
    feet_mat = core.Mat('mouse_feet')
    feet_mat.set('Base Color', core.hex_linear(0xb9b6ae))
    feet_mat.set('Roughness', 0.3)

    # Stations bunch up at the ends, where the shell turns sharply.
    count = 64
    stations = [-math.cos(math.pi * k / (count - 1)) for k in range(count)]
    rings = [section(max(-0.998, min(0.998, s))) for s in stations]
    shell = core.loft('shell', rings)
    core.subdivide(shell, 1)
    shell.data.shade_smooth()
    core.assign(shell, shell_mat)

    # The wheel sits in a slot through the shell, standing proud of the top by about 4 mm.
    wy = WHEEL_S * LENGTH / 2
    top = FOOT + height(WHEEL_S)
    cutter = core.mesh_object('slot', core.slab(core.rounded_rect(-0.0043, wy - 0.0125, 0.0043, wy + 0.0125, 0.0028, steps=6),
                                                 top - 0.009, top + 0.02))
    core.boolean(shell, cutter)
    hw = WHEEL_W / 2
    profile = [(0.0, -hw), (0.0082, -hw), (0.0095, -hw + 0.0002), (0.0101, -hw + 0.0008), (WHEEL_R, -hw + 0.0019),
               (WHEEL_R, hw - 0.0019), (0.0101, hw - 0.0008), (0.0095, hw - 0.0002), (0.0082, hw), (0.0, hw)]
    wheel = core.lathe('wheel', profile, segments=72)
    wheel.rotation_euler = (0.0, math.radians(90), 0.0)
    wheel.location = (0.0, wy, top - 0.0074)
    core.apply_transforms(wheel)
    core.assign(wheel, wheel_mat)

    # Two PTFE feet: a bar across the nose and a horseshoe at the palm end, drawn as rounded slabs.
    feet = []
    for y0, y1, w in ((0.036, 0.044, 0.030), (-0.046, -0.038, 0.034)):
        bm = core.slab(core.rounded_rect(-w / 2, y0, w / 2, y1, 0.0035, steps=5), 0.0, FOOT + 0.0003)
        feet.append(core.mesh_object('foot', bm))
    foot = core.join(feet, 'feet')
    core.assign(foot, feet_mat)

    obj = core.join([shell, wheel, foot], 'SM_Mouse')
    core.uv_layout(obj, [
        (core.material_is(obj, 'mouse_shell'), 'smart', None, (0.0, 0.0, 1.0, 1.0)),
        (core.material_is(obj, 'mouse_wheel'), 'smart', None, (0.0, 0.0, 1.0, 1.0)),
        (lambda f: True, 'box', None, (0.0, 0.0, 1.0, 1.0)),
    ])
    return obj


def pad_print():
    """The RiverLine corner print (millimeters), as on the mug: the wave mark over the wordmark."""
    s = Sheet(80, 36)
    cx = 40
    for k, dy in enumerate((0.0, -4.6)):
        top, bot = [], []
        for i in range(41):
            t = i / 40
            x = cx - 12 + 24 * t
            y = 27 + dy + 2.3 * math.sin(t * 2 * math.pi * 1.15 + 0.4)
            top.append((x, y + 1.2))
            bot.append((x, y - 1.2))
        s.poly(top + list(reversed(bot)), 0x14b8a0 if k == 0 else 0x0f8a78, rough=0.5)
    s.text('River', cx + 1.5, 8.5, 11.0, 0xd9dee4, face='Bold', align='RIGHT', rough=0.55)
    s.text('Line', cx + 1.5, 8.5, 11.0, 0x14b8a0, face='Bold', rough=0.55)
    return s.render('pad_print', 1024)


def cloth_material():
    m = core.Mat('pad_cloth')
    tc, sep = object_coords(m)
    x, y = sep.outputs['X'], sep.outputs['Y']
    fiber = noise(m, tc.outputs['Object'], 4200.0, 2.0)
    mottle = noise(m, tc.outputs['Object'], 18.0, 3.0)
    # Pile flattened and pilled where the mouse slides: an ellipse around its resting spot.
    mx, my = MOUSE_ON_PAD
    dx = m.math('DIVIDE', m.math('SUBTRACT', x, mx), 0.075)
    dy = m.math('DIVIDE', m.math('SUBTRACT', y, my), 0.058)
    worn = smooth_less(m, m.math('SQRT', m.math('ADD', m.math('MULTIPLY', dx, dx), m.math('MULTIPLY', dy, dy))), 0.8, 0.35)
    worn = m.math('MULTIPLY', worn, m.math('ADD', 0.6, m.math('MULTIPLY', noise(m, tc.outputs['Object'], 40.0, 4.0), 0.6)))
    pills = ramp(m, noise(m, tc.outputs['Object'], 1400.0, 1.0), 0.66, 0.72)
    # A dried coffee ring near the top left.
    ring_d = circle_dist(m, sep, -0.075, 0.058)
    ring = smooth_less(m, m.math('ABSOLUTE', m.math('SUBTRACT', ring_d, 0.034)), 0.0012, 0.0008)
    ring_fill = m.math('MULTIPLY', smooth_less(m, ring_d, 0.034, 0.002), 0.25)
    stain = m.math('MULTIPLY', m.math('MAXIMUM', ring, ring_fill), ramp(m, noise(m, tc.outputs['Object'], 120.0, 4.0), 0.3, 0.65))
    stain = m.math('MULTIPLY', stain, 0.6)

    paths = pad_print()
    uv = planar(m, sep, 'X', 'Y', PAD_W / 2 - 0.094, PAD_W / 2 - 0.014, -PAD_D / 2 + 0.012, -PAD_D / 2 + 0.048)
    pcol, palpha, _, _ = image_surface(m, paths, uv, extension='CLIP')
    # The print cracks and fades with washing.
    palpha = m.math('MULTIPLY', palpha, m.math('ADD', 0.35, m.math('MULTIPLY', ramp(m, noise(m, tc.outputs['Object'], 700.0, 4.0), 0.35, 0.55), 0.45)))

    color = m.mix(m.math('MULTIPLY', mottle, 0.3), core.hex_linear(0x131417), core.hex_linear(0x1a1b20))
    color = m.mix(m.math('MULTIPLY', worn, 0.8), color, core.hex_linear(0x2a2b30))
    color = m.mix(m.math('MULTIPLY', m.math('MULTIPLY', pills, worn), 0.6), color, core.hex_linear(0x3a3a3e))
    color = m.mix(m.math('MULTIPLY', stain, 0.7), color, core.hex_linear(0x21180f))
    color = m.mix(palpha, color, pcol)
    rough = m.math('SUBTRACT', m.math('ADD', 0.9, m.math('MULTIPLY', fiber, 0.06)), m.math('MULTIPLY', worn, 0.18))
    rough = mix_float(m, palpha, rough, 0.62)
    m.set('Base Color', color)
    m.set('Roughness', rough)
    m.set('Metallic', 0.0)
    bump = m.node('ShaderNodeBump', Strength=0.25, Distance=0.0002)
    m.link(m.math('ADD', fiber, m.math('MULTIPLY', pills, 0.6)), bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def thread_material():
    m = core.Mat('pad_thread')
    tc, sep = object_coords(m)
    # Stitches every 3 mm along whichever edge the thread follows.
    along = m.math('ADD', sep.outputs['X'], sep.outputs['Y'])
    stitch = m.math('ABSOLUTE', m.math('SINE', m.math('MULTIPLY', along, math.pi / 0.003)))
    fray = ramp(m, noise(m, tc.outputs['Object'], 300.0, 3.0), 0.6, 0.75)
    m.set('Base Color', m.mix(fray, core.hex_linear(0x2b2c31), core.hex_linear(0x46454a)))
    m.set('Roughness', 0.85)
    bump = m.node('ShaderNodeBump', Strength=0.8, Distance=0.0004)
    m.link(stitch, bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def build_pad():
    cloth_mat, thread_mat = cloth_material(), thread_material()
    rubber_mat = core.Mat('pad_rubber')
    rubber_mat.set('Base Color', core.hex_linear(0x1b1b1d))
    rubber_mat.set('Roughness', 0.92)

    outline = core.rounded_rect(-PAD_W / 2, -PAD_D / 2, PAD_W / 2, PAD_D / 2, PAD_R, steps=10)
    base = core.mesh_object('pad_base', core.slab(outline, 0.0, PAD_T - CLOTH_T))
    core.assign(base, rubber_mat)
    bm = core.slab(outline, PAD_T - CLOTH_T, PAD_T)
    core.bevel(bm, lambda e: all(abs(v.co.z - PAD_T) < 1e-7 for v in e.verts), 0.0005, segments=2)
    cloth = core.mesh_object('pad_cloth', bm)
    core.assign(cloth, cloth_mat)
    # The stitched binding: a round bead swept around the edge.
    inset = 0.0011
    path = [(x * (1 - 2 * inset / PAD_W), y * (1 - 2 * inset / PAD_D), PAD_T - 0.0006) for x, y in outline]
    bead_profile = [(0.0011 * math.cos(2 * math.pi * k / 10), 0.0013 * math.sin(2 * math.pi * k / 10)) for k in range(10)]
    bead = core.sweep('bead', path, bead_profile, closed=True)
    bead.data.shade_smooth()
    core.assign(bead, thread_mat)

    obj = core.join([base, cloth, bead], 'SM_MousePad')
    is_cloth = core.material_is(obj, 'pad_cloth')
    core.uv_layout(obj, [
        (lambda f: is_cloth(f) and f.normal.z > 0.9, 'planar', ('x', 'y', -PAD_W / 2, PAD_W / 2, -PAD_D / 2, PAD_D / 2), (0.0, 0.0, 1.0, 0.94)),
        (is_cloth, 'box', None, (0.0, 0.95, 1.0, 1.0)),
        (lambda f: True, 'box', None, (0.0, 0.0, 1.0, 1.0)),
    ])
    return obj


def build():
    core.reset()
    mouse = build_mouse()
    pad = build_pad()
    # Side by side for the review: the pad in front, the mouse on it where the stage puts it.
    mouse.location = (MOUSE_ON_PAD[0], MOUSE_ON_PAD[1], PAD_T)
    mouse.rotation_euler.z = math.radians(-8)
    return [mouse, pad]


def bake_parts(objs):
    """Bake each at its own origin so nothing shadows the other, then put them back for the review."""
    mouse, pad = objs
    mouse.location = (0.0, 0.0, 0.0)
    mouse.rotation_euler.z = 0.0
    return objs


def pose_for_review(objs):
    mouse = next(o for o in objs if o.name.startswith('SM_Mouse'))
    mouse.location = (MOUSE_ON_PAD[0], MOUSE_ON_PAD[1], PAD_T)
    mouse.rotation_euler.z = math.radians(-8)
