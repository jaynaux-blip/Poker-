"""Hats and glasses for the player's character (the creator's Hat and Glasses choices).

- SM_Hat_Beanie: a cuffed rib-knit beanie with a woven label on the cuff.
- SM_Hat_Cap: a six-panel ball cap: seams, eyelets, the top button, a pre-curved bill, a stitched spade
  on the front panel. The creator's "backwards" cap is this one turned round in the game.
- SM_Hat_Bucket: a cotton-twill bucket hat, its brim stitched in rings.
- SM_Glasses_Round, _Square, _Wire, _Shades: acetate rounds, acetate rectangles, thin gold wire ovals with
  nose pads, and dark wraparound shades.

The hats' fabric and the frames' fronts are light neutral grey: the game tints the first material of each
to the color the character wears (the hat goes with the jacket; the frames black or gold).

Coordinates (meters), front toward -Y, Z up. A hat's origin is the top of the head inside it; a pair of
glasses' origin is the bridge of the nose, the temples running back along +Y to the ears. The head they fit:
HEAD_W wide, HEAD_L long, its crown CROWN above the hat line.
"""
import math

import bpy  # noqa: F401,I001
import bmesh
from mathutils import Matrix, Vector

from artkit import core, parts, street
from artkit.shade import noise, object_coords, ramp, scaled, image_surface, planar
from artkit.sheet import Sheet

MESHES = ['SM_Hat_Beanie', 'SM_Hat_Cap', 'SM_Hat_Bucket', 'SM_Glasses_Round', 'SM_Glasses_Square', 'SM_Glasses_Wire', 'SM_Glasses_Shades']
DOUBLE_SIDED = True
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {'beanie_label': 128, 'cap_under': 512, 'cap_button': 64, 'lens_clear': 128, 'lens_dark': 128, 'hinge_metal': 128,
                 'pads': 64, 'frame_acetate': 512, 'frame_wire': 512, 'frame_shades': 512}
AO_DISTANCE = 0.006
REVIEW_VIEWS = [('front', -25, 15, 1.1), ('top', 10, 60, 1.0), ('detail', -35, 12, 0.55, (0.0, -0.09, -0.05))]

HEAD_W = 0.156  # ear to ear
HEAD_L = 0.196  # brow to back
CROWN = 0.11    # crown above the widest ring


def head_r(z, extra=0.0):
    """Radius (along the head's length) of the head at height z below the crown (z <= 0)."""
    t = min(1.0, max(-1.0, (z + CROWN) / CROWN))
    return HEAD_L / 2 * math.sqrt(max(0.0, 1.0 - t * t)) + extra


def oval(obj):
    """Squeezes a revolved shape (built round, HEAD_L across) to the head's width."""
    obj.data.transform(Matrix.Diagonal((HEAD_W / HEAD_L, 1.0, 1.0, 1.0)))
    return obj


# ------------------------------------------------------------------ materials

def knit():
    """Rib knit: columns of V stitches, a soft fuzz. Light grey (the game tints it)."""
    m = core.Mat('beanie_knit')
    tc, sep = object_coords(m)
    ang = m.math('ARCTAN2', sep.outputs['Y'], sep.outputs['X'])
    ribs = m.math('ABSOLUTE', m.math('SINE', m.math('MULTIPLY', ang, 64.0)))
    rows = m.math('ABSOLUTE', m.math('SINE', m.math('ADD', m.math('MULTIPLY', sep.outputs['Z'], 2 * math.pi / 0.004), m.math('MULTIPLY', ribs, 2.0))))
    fuzz = noise(m, tc.outputs['Object'], scale=900.0, detail=3.0)
    h = m.math('ADD', m.math('MULTIPLY', ribs, 0.7), m.math('MULTIPLY', rows, 0.3))
    m.set('Base Color', m.mix(m.math('ADD', m.math('MULTIPLY', h, 0.3), m.math('MULTIPLY', fuzz, 0.2)), core.hex_linear(0x9a9a9a), core.hex_linear(0xd6d6d6)))
    m.set('Roughness', 0.95)
    m.set('Sheen Weight', 0.6)
    bump = m.node('ShaderNodeBump', Strength=0.8, Distance=0.0015)
    m.link(m.math('ADD', h, m.math('MULTIPLY', fuzz, 0.3)), bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def twill(name, seams=0, rings=False):
    """Cotton twill, light grey; seams (count of panels round the crown) or stitched rings on a brim."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    diag = m.math('SINE', m.math('MULTIPLY', m.math('ADD', sep.outputs['X'], sep.outputs['Z']), 2 * math.pi / 0.0012))
    fuzz = noise(m, tc.outputs['Object'], scale=600.0, detail=2.0)
    h = m.math('ADD', m.math('MULTIPLY', diag, 0.4), m.math('MULTIPLY', fuzz, 0.5))
    lines = None
    if seams:
        ang = m.math('ARCTAN2', sep.outputs['Y'], sep.outputs['X'])
        seam = m.math('ABSOLUTE', m.math('SINE', m.math('MULTIPLY', m.math('ADD', ang, math.pi / seams), seams / 2.0)))
        lines = ramp(m, m.math('SUBTRACT', 1.0, seam), 0.965, 0.99)
    if rings:
        r = m.math('SQRT', m.math('ADD', m.math('MULTIPLY', sep.outputs['X'], sep.outputs['X']), m.math('MULTIPLY', sep.outputs['Y'], sep.outputs['Y'])))
        ring = m.math('ABSOLUTE', m.math('SINE', m.math('MULTIPLY', r, 2 * math.pi / 0.008)))
        lines = ramp(m, ring, 0.93, 0.99)
    col = m.mix(m.math('MULTIPLY', h, 0.25), core.hex_linear(0xc4c4c4), core.hex_linear(0xdedede))
    if lines is not None:
        col = m.mix(m.math('MULTIPLY', lines, 0.5), col, core.hex_linear(0x8a8a8a))
        h = m.math('SUBTRACT', h, m.math('MULTIPLY', lines, 1.5))
    m.set('Base Color', col)
    m.set('Roughness', 0.88)
    m.set('Sheen Weight', 0.4)
    bump = m.node('ShaderNodeBump', Strength=0.5, Distance=0.0006)
    m.link(h, bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def spade_sheet():
    s = Sheet(100, 100)
    s.rect(0, 0, 100, 100, 0xc8c8c8, rough=0.9)
    pts = []
    for k in range(64):
        t = 2 * math.pi * k / 64
        # An inverted heart.
        x = 16 * math.sin(t) ** 3
        y = 13 * math.cos(t) - 5 * math.cos(2 * t) - 2 * math.cos(3 * t) - math.cos(4 * t)
        pts.append((50 + x * 1.9, 58 - y * 1.9))
    s.poly(pts, 0x121214, rough=0.7)
    s.poly([(50, 40), (40, 14), (60, 14)], 0x121214, rough=0.7)
    return s.render('cap_spade', 512)


def label_material(depth):
    s = Sheet(60, 20)
    s.rect(0, 0, 60, 20, 0x1a1a1c, rough=0.8)
    s.text('FIFTH ST.', 30, 6, 9, 0xe8e2d0, face='Black', align='CENTER', tracking=1.3, rough=0.8)
    img = s.render('beanie_label', 512)
    m = core.Mat('beanie_label')
    tc, sep = object_coords(m)
    uv = planar(m, sep, 'X', 'Z', -0.025, 0.025, -depth - 0.0085, -depth + 0.0085)
    ink, alpha, metal, rough = image_surface(m, img, uv)
    m.set('Base Color', ink)
    m.set('Roughness', 0.8)
    return m


def acetate(name, base=0xbdbdbd):
    m = core.Mat(name)
    tc, sep = object_coords(m)
    swirl = noise(m, scaled(m, sep, 80.0, 80.0, 400.0), detail=3.0, distortion=2.0)
    m.set('Base Color', m.mix(m.math('MULTIPLY', swirl, 0.25), core.hex_linear(base), core.hex_linear(street._shade(base, 0.8))))
    m.set('Roughness', 0.12)
    m.set('Coat Weight', 0.8)
    return m


# ------------------------------------------------------------------ hats

CUFF = (-0.068, -0.118)  # the folded cuff, top and bottom (the bottom just above the brows)


def beanie():
    knit_m = knit()
    r_cuff = head_r(-CROWN) + 0.01
    label_m = label_material(-(CUFF[0] + CUFF[1]) / 2)
    # Outer from the crown (a little slouch on top) down to the cuff, the cuff out and round the hem, the inside back up.
    prof = [(0.0, 0.014)]
    for k in range(1, 12):
        z = CUFF[0] * k / 11
        prof.append((head_r(z, 0.008), z + 0.014 * (1 - k / 11) ** 2))
    prof += [(r_cuff, CUFF[0] - 0.002), (r_cuff + 0.002, CUFF[0] - 0.01), (r_cuff + 0.002, CUFF[1] + 0.004), (r_cuff - 0.002, CUFF[1]),
             (r_cuff - 0.007, CUFF[1] + 0.002), (r_cuff - 0.008, CUFF[0]), (head_r(-0.04, 0.002), -0.04), (head_r(-0.015, 0.002), -0.015), (0.0, 0.001)]
    shell = core.lathe('beanie', list(reversed(prof)), segments=96)
    oval(shell)
    core.orient_normals(shell)
    core.assign(shell, knit_m)
    zc = (CUFF[0] + CUFF[1]) / 2
    label = parts.rbox('label', (0.0, -(r_cuff + 0.002), zc), (0.05, 0.002, 0.017), 0.0008)
    core.assign(label, label_m)
    return parts.finish([shell, label], 'SM_Hat_Beanie', 50)


def cap():
    crown_m = twill('cap_crown', seams=6)
    under_m = street.shop_plastic('cap_under', 0x2e3a2e, rough=0.9, grime=0.2)
    button_m = twill('cap_button')
    spade_img = spade_sheet()
    band = -0.082
    prof = [(0.0, 0.004)]
    for k in range(1, 11):
        z = band * k / 10
        prof.append((head_r(z, 0.005), z + 0.004 * (1 - k / 10)))
    prof += [(head_r(band, 0.005), band - 0.006), (head_r(band, 0.001), band - 0.006), (head_r(band * 0.5, 0.001), band * 0.5), (0.0, 0.0)]
    shell = core.lathe('cap', list(reversed(prof)), segments=96)
    oval(shell)
    core.orient_normals(shell)
    # The front panels carry the stitched spade.
    m = crown_m
    tc, sep = object_coords(m)
    uv = planar(m, sep, 'X', 'Z', -0.03, 0.03, -0.065, -0.005)
    ink, alpha, metal, rough = image_surface(m, spade_img, uv)
    front = m.math('MULTIPLY', m.math('LESS_THAN', sep.outputs['Y'], -0.05), m.math('SUBTRACT', 1.0, m.math('GREATER_THAN', m.math('ABSOLUTE', sep.outputs['X']), 0.03)))
    front = m.math('MULTIPLY', front, m.math('GREATER_THAN', sep.outputs['Z'], -0.07))  # the front panel, not the bill
    mask = m.math('MULTIPLY', front, m.math('SUBTRACT', 1.0, ramp(m, ink_mask(m, ink), 0.4, 0.6)))
    base = m.bsdf.inputs['Base Color'].links[0].from_socket
    m.set('Base Color', m.mix(mask, base, core.hex_linear(0x121214)))
    core.assign(shell, crown_m)
    button = parts.lathe_at('button', [(0.0, 0.002), (0.009, 0.002), (0.009, 0.005), (0.006, 0.008), (0.0, 0.0085)], Matrix(), 24)
    core.assign(button, button_m)
    # Eyelets: small dark rings in the upper panels.
    eyes = []
    for k in range(6):
        a = 2 * math.pi * (k + 0.5) / 6
        z = -0.03
        r = head_r(z, 0.0055)
        c = Vector((r * math.cos(a) * HEAD_W / HEAD_L, r * math.sin(a), z + 0.0035 * 0.3))
        n = Vector((math.cos(a) * HEAD_L / HEAD_W, math.sin(a), 0.6)).normalized()
        ring = parts.annulus('eyelet', 0.0012, 0.0026, -0.0005, 0.0005, 16)
        ring.data.transform(Matrix.Translation(c) @ n.to_track_quat('Z', 'Y').to_matrix().to_4x4())
        eyes.append(ring)
    parts.assign_all(eyes, under_m)
    # The bill: a pre-curved plate out from the band at the front, its underside green-grey.
    outline = []
    for k in range(33):
        t = math.pi * k / 32
        outline.append((0.098 * math.cos(t), -0.075 * math.sin(t)))
    bill = core.mesh_object('bill', core.slab(outline, 0.0, 0.004))
    for v in bill.data.vertices:
        x, y = v.co.x, v.co.y
        # Curved across, a little down at the tip.
        v.co.z += -1.6 * x * x * (0.4 + 0.6 * (-y / 0.075)) - 0.12 * (-y)
        v.co.y = y - head_r(band, 0.004) * 0.94
        v.co.z += band + 0.004
    core.orient_normals(bill)
    bill.data.materials.append(crown_m.m)
    bill.data.materials.append(under_m.m)
    for poly in bill.data.polygons:
        poly.material_index = 1 if poly.normal.z < -0.3 else 0
    return parts.finish([shell, bill, button] + eyes, 'SM_Hat_Cap', 50)


def ink_mask(m, ink):
    """Luminance of a printed color socket (dark ink low)."""
    rgb = m.node('ShaderNodeRGBToBW')
    m.link(ink, rgb.inputs['Color'])
    return rgb.outputs['Val']


def bucket():
    m = twill('bucket_twill', rings=False)
    brim_m = twill('bucket_brim', rings=True)
    band = -0.08
    prof = [(0.0, 0.016), (0.045, 0.015), (0.062, 0.012), (0.072, 0.005), (0.08, -0.006), (head_r(-0.03, 0.012), -0.022), (head_r(band, 0.012), band),
            (head_r(band, 0.002), band), (head_r(band * 0.5, 0.002), band * 0.5), (0.05, 0.005), (0.0, 0.005)]
    crown = core.lathe('bucket', list(reversed(prof)), segments=96)
    oval(crown)
    core.orient_normals(crown)
    core.assign(crown, m)
    r0 = head_r(band, 0.012)
    brim = core.lathe('brim', [(r0 - 0.001, band + 0.002), (r0 + 0.03, band - 0.016), (r0 + 0.058, band - 0.032), (r0 + 0.06, band - 0.036),
                               (r0 + 0.056, band - 0.036), (r0 + 0.028, band - 0.02), (r0 - 0.001, band - 0.002)], segments=96)
    oval(brim)
    core.orient_normals(brim)
    core.assign(brim, brim_m)
    return parts.finish([crown, brim], 'SM_Hat_Bucket', 50)


# ------------------------------------------------------------------ glasses

LENS_X = 0.032
HINGE_X = 0.068
EAR_Y = 0.098


def frame_ring(name, shape, w, h, rim, depth, cx, wrap=6.0):
    """A lens rim: the outline shape(t) -> (x, z) offset outward by rim, depth thick along Y, at cx."""
    outer, inner = [], []
    n = 48
    for k in range(n):
        t = 2 * math.pi * k / n
        x, z = shape(t, w, h)
        d = Vector((x, z))
        nrm = d.normalized() if d.length > 1e-6 else Vector((1, 0))
        outer.append(((x + nrm.x * rim), (z + nrm.y * rim)))
        inner.append((x, z))
    obj = parts.outline_prism(name, outer, [inner], -depth / 2, depth / 2)
    # Outline was in XY with thickness along Z: stand it up (Y becomes depth), wrap it a little, move it out.
    obj.data.transform(Matrix.Translation((cx, 0, 0)) @ Matrix.Rotation(math.radians(wrap if cx > 0 else -wrap), 4, 'Z') @ Matrix(((1, 0, 0, 0), (0, 0, 1, 0), (0, 1, 0, 0), (0, 0, 0, 1))))
    core.orient_normals(obj)
    return obj


def lens_obj(name, shape, w, h, cx, wrap=6.0):
    pts = [shape(2 * math.pi * k / 48, w, h) for k in range(48)]
    obj = parts.outline_prism(name, pts, [], -0.0008, 0.0008)
    obj.data.transform(Matrix.Translation((cx, 0, 0)) @ Matrix.Rotation(math.radians(wrap if cx > 0 else -wrap), 4, 'Z') @ Matrix(((1, 0, 0, 0), (0, 0, 1, 0), (0, 1, 0, 0), (0, 0, 0, 1))))
    core.orient_normals(obj)
    return obj


def round_shape(t, w, h):
    return (w / 2 * math.cos(t), h / 2 * math.sin(t))


def rect_shape(t, w, h):
    # A superellipse: squarish with soft corners, a little deeper on the outside.
    c, s = math.cos(t), math.sin(t)
    e = 0.3
    x = w / 2 * math.copysign(abs(c) ** e, c)
    z = h / 2 * math.copysign(abs(s) ** e, s)
    return (x, z - 0.002 * (1 - abs(c)) * (1 if s < 0 else 0))


def shades_shape(t, w, h):
    c, s = math.cos(t), math.sin(t)
    x = w / 2 * math.copysign(abs(c) ** 0.5, c)
    z = h / 2 * math.copysign(abs(s) ** 0.6, s)
    # Higher at the outer corner, swept down toward the nose.
    return (x, z + 0.004 * c)


def glasses(name, shape, w, h, rim, depth, frame_m, lens_m, metal_m, bridge_h=0.0, wire=False, wrap=6.0):
    out = []
    for side in (-1, 1):
        cx = side * LENS_X
        out.append((frame_ring('rim', shape, w, h, rim, depth, cx, wrap), frame_m))
        out.append((lens_obj('lens', shape, w, h, cx, wrap), lens_m))
        # The temple: from the hinge back to the ear, bent down behind it.
        hx = side * (HINGE_X if not wire else HINGE_X - 0.002)
        pts = [(hx, 0.004, 0.004), (hx + side * 0.004, 0.03, 0.004), (hx + side * 0.006, 0.07, 0.0), (hx + side * 0.005, EAR_Y, -0.006), (hx + side * 0.002, EAR_Y + 0.02, -0.028)]
        # Acetate temples are flat bars (tall, thin); wire ones round.
        if wire:
            section = parts.circle(0.0011, 8)
        else:
            section = [(0.0028 * math.cos(2 * math.pi * k / 12), 0.0014 * math.sin(2 * math.pi * k / 12)) for k in range(12)]
        temple = core.sweep('temple', core.catmull_rom(pts, 8), section)
        out.append((temple, frame_m))
        out.append((parts.rbox('hinge', (hx, 0.002, 0.004), (0.006, 0.006, 0.004), 0.001), metal_m))
        if wire:
            # Nose pads on little arms.
            out.append((parts.disc('pad', (side * 0.009, -0.004, -0.012), (side, 0.4, 0.0), 0.0045, 0.0015, 16), lens_m))
            out.append((parts.tube('pad_arm', [(side * 0.013, 0.0, -0.004), (side * 0.011, -0.003, -0.008), (side * 0.0095, -0.004, -0.011)], 0.0005, 6, 4), metal_m))
    # The bridge.
    if wire:
        out.append((parts.tube('bridge', [(-0.014, 0.0, 0.004), (0.0, -0.001, 0.008), (0.014, 0.0, 0.004)], 0.0011, 8, 6), frame_m))
        out.append((parts.tube('brow_bar', [(-0.03, 0.0, 0.017), (0.0, -0.001, 0.016), (0.03, 0.0, 0.017)], 0.0009, 8, 6), frame_m))
    else:
        out.append((parts.rbox('bridge', (0.0, 0.0, bridge_h), (0.018, depth, 0.006), 0.0015), frame_m))
    for o, m in out:
        core.assign(o, m)
    # The frame material first: the game tints the first slot.
    objs = [o for o, m in out if m is frame_m] + [o for o, m in out if m is not frame_m]
    return parts.finish(objs, name, 45)


def build():
    core.reset()
    out = [beanie(), cap(), bucket()]
    acetate_m = acetate('frame_acetate')
    wire_m = street.bare_metal('frame_wire', 0xd8d8d8, rough=0.22, grime=0.1)
    shades_m = acetate('frame_shades', 0xb0b0b0)
    clear = street.glass('lens_clear', 0x1a1f24, rough=0.02)
    dark = street.glass('lens_dark', 0x050607, rough=0.03)
    hinge = street.bare_metal('hinge_metal', 0xb5b8bc, rough=0.25)
    pads = street.shop_plastic('pads', 0xd8dde0, rough=0.2, grime=0.0)
    out.append(glasses('SM_Glasses_Round', round_shape, 0.046, 0.044, 0.0042, 0.0045, acetate_m, clear, hinge))
    out.append(glasses('SM_Glasses_Square', rect_shape, 0.052, 0.036, 0.0048, 0.005, acetate_m, clear, hinge, bridge_h=0.006))
    out.append(glasses('SM_Glasses_Wire', round_shape, 0.05, 0.04, 0.0012, 0.0016, wire_m, pads, hinge, wire=True))
    out.append(glasses('SM_Glasses_Shades', shades_shape, 0.058, 0.044, 0.004, 0.005, shades_m, dark, hinge, bridge_h=0.008, wrap=12.0))
    return out


def pose_for_review(objs):
    """Hats in a row at the back, the glasses in front of them."""
    for i, o in enumerate(objs):
        if o.name.startswith('SM_Hat'):
            o.location = ((i - 1) * 0.24, 0.12, 0.16)
        else:
            o.location = ((i - 4.5) * 0.17, -0.12, 0.05)
