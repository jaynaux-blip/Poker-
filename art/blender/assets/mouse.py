"""SM_Mouse and SM_MousePad: a matte black wireless gaming mouse on a cloth pad.

The mouse is sculpted as a signed distance field:
- a palm hump toward the back, left and right buttons split by a seam, and a ribbed scroll wheel;
- two thumb buttons, and rubber grips on the sides.
The finish shows use: glossy where the palm and fingertips rest, grime in the seams.

The pad is black cloth with a stitched edge, a RiverLine print in the corner, and a patch rubbed
smooth where the mouse lives.

Coordinates (meters): origin on the desk (the pad's top for the mouse, the desk for the pad), Z up;
the mouse's buttons point +Y, away from the player.
"""
import math

import bpy

from artkit import core, sdf
from artkit.shade import convex_edges, image_surface, mix_float, noise, object_coords, planar, ramp, smooth_less
from artkit.sheet import Sheet

MESHES = ['SM_Mouse', 'SM_MousePad']
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {'mouse_shell': 2048, 'pad_cloth': 2048}
AO_DISTANCE = 0.006

L, WID, H = 0.124, 0.066, 0.040
PAD_W, PAD_D, PAD_T = 0.26, 0.21, 0.003
REVIEW_VIEWS = [('front', -35, 26, 1.4), ('top', 8, 70, 1.3), ('detail', -40, 28, 0.35, (0.01, 0.03, 0.03))]


def mouse_prims():
    P = []
    # Body: the shell swells toward the back under the palm and tapers to the buttons.
    P.append(sdf.ellipsoid((0.0, -0.004, 0.0), (0.031, 0.061, H), k=0.012))
    P.append(sdf.ellipsoid((0.0, -0.018, 0.004), (0.032, 0.036, 0.036), k=0.02))
    P.append(sdf.ellipsoid((0.0, 0.03, 0.0), (0.029, 0.03, 0.028), k=0.02))
    # Flat underside.
    P.append(sdf.carve(sdf.rounded_box((0.0, 0.0, -0.05), (0.1, 0.1, 0.05), 0.001, k=0.002)))
    # The waist where thumb and ring finger grip.
    for s in (1.0, -1.0):
        P.append(sdf.carve(sdf.ellipsoid((s * 0.047, -0.002, 0.016), (0.02, 0.038, 0.02), k=0.01)))
    # Seam between the buttons, and across behind them.
    P.append(sdf.carve(sdf.rounded_box((0.0, 0.043, 0.04), (0.0004, 0.03, 0.02), 0.0002, k=0.0006)))
    for s in (1.0, -1.0):
        P.append(sdf.carve(sdf.rounded_box((s * 0.017, 0.013, 0.045), (0.016, 0.0004, 0.02), 0.0002, k=0.0006)))
    # Scroll wheel slot and wheel.
    P.append(sdf.carve(sdf.rounded_box((0.0, 0.034, 0.04), (0.0045, 0.0125, 0.012), 0.002, k=0.001)))
    return P


def wheel_prims():
    return [sdf.rounded_cylinder((-0.0034, 0.034, 0.0265), (0.0034, 0.034, 0.0265), 0.0105, 0.0012, k=0.001)]


def side_buttons_prims():
    """Two thumb buttons standing 1.5 mm proud of the left side, wherever the shell's surface is there."""
    out = []
    for y in (0.004, 0.023):
        hit = sdf.surface(mouse_prims(), (-0.06, y, 0.0215), (1.0, 0.0, 0.0))
        x = hit[0] if hit is not None else -0.026
        out.append(sdf.rounded_box((x + 0.0005, y, 0.0215), (0.002, 0.0085, 0.0035), 0.0018, k=0.0008))
    return out


def shell_material():
    m = core.Mat('mouse_shell')
    tc, sep = object_coords(m)
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    # Rubber grips on the sides, in the waist.
    ax = m.math('ABSOLUTE', x)
    grip = m.math('MULTIPLY', ramp(m, ax, 0.024, 0.027), m.math('MULTIPLY', ramp(m, z, 0.004, 0.007), m.math('SUBTRACT', 1.0, ramp(m, z, 0.026, 0.029))))
    grip = m.math('MULTIPLY', grip, m.math('MULTIPLY', ramp(m, y, -0.045, -0.035), m.math('SUBTRACT', 1.0, ramp(m, y, 0.03, 0.04))))
    diamond_a = m.math('SINE', m.math('MULTIPLY', m.math('ADD', y, z), 2 * math.pi / 0.0016))
    diamond_b = m.math('SINE', m.math('MULTIPLY', m.math('SUBTRACT', y, z), 2 * math.pi / 0.0016))
    knurl = m.math('MAXIMUM', diamond_a, diamond_b)
    # Soft-touch coating, polished glossy where the palm and fingertips rest.
    palm = m.math('MULTIPLY', m.math('SUBTRACT', 1.0, ramp(m, m.math('ADD', m.math('MULTIPLY', x, x), m.math('MULTIPLY', m.math('ADD', y, 0.016), m.math('ADD', y, 0.016))), 0.0002, 0.0009)),
                  ramp(m, z, 0.03, 0.037))
    tips = m.math('MULTIPLY', ramp(m, y, 0.036, 0.05), m.math('MULTIPLY', ramp(m, z, 0.02, 0.026), m.math('SUBTRACT', 1.0, ramp(m, ax, 0.018, 0.026))))
    wear = m.math('MULTIPLY', m.math('MAXIMUM', palm, tips), m.math('ADD', 0.55, m.math('MULTIPLY', noise(m, tc.outputs['Object'], scale=900.0, detail=2.0), 0.45)))
    edges = m.math('MULTIPLY', convex_edges(m, tc, distance=0.0008, samples=12, breakup_scale=500.0), 0.6)
    grime = m.math('MULTIPLY', ramp(m, noise(m, tc.outputs['Object'], scale=700.0, detail=3.0), 0.55, 0.8), 0.35)
    color = m.mix(grip, core.hex_linear(0x17181a), core.hex_linear(0x121213))
    color = m.mix(m.math('MULTIPLY', edges, 0.5), color, core.hex_linear(0x3a3b3f))
    color = m.mix(grime, color, core.hex_linear(0x2a2620))
    rough = mix_float(m, grip, 0.58, 0.8)
    rough = m.math('SUBTRACT', rough, m.math('MULTIPLY', wear, 0.16))
    rough = m.math('SUBTRACT', rough, m.math('MULTIPLY', edges, 0.15))
    # A small glossy brand mark on the hump.
    mark = planar(m, sep, 'X', 'Y', -0.012, 0.012, -0.03, -0.018)
    ink, alpha, _, ink_rough = image_surface(m, MARK, mark, extension='CLIP')
    on_top = m.math('MULTIPLY', alpha, ramp(m, z, 0.034, 0.037))
    color = m.mix(on_top, color, ink)
    rough = mix_float(m, on_top, rough, ink_rough)
    m.set('Base Color', color)
    m.set('Roughness', rough)
    bump = m.node('ShaderNodeBump', Strength=0.3, Distance=0.0002)
    m.link(m.math('MULTIPLY', knurl, grip), bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def wheel_material():
    m = core.Mat('mouse_wheel')
    tc, sep = object_coords(m)
    ang = m.math('ARCTAN2', m.math('SUBTRACT', sep.outputs['Z'], 0.0265), m.math('SUBTRACT', sep.outputs['Y'], 0.034))
    ribs = m.math('SINE', m.math('MULTIPLY', ang, 36.0))
    m.set('Base Color', core.hex_linear(0x1c1c1e))
    m.set('Roughness', 0.7)
    bump = m.node('ShaderNodeBump', Strength=0.6, Distance=0.0003)
    m.link(ribs, bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def mark_sheet():
    s = Sheet(24.0, 12.0)
    s.poly([(4.0, 4.0), (12.0, 10.5), (20.0, 4.0), (17.4, 4.0), (12.0, 8.4), (6.6, 4.0)], 0x5c5f66, metal=0.0, rough=0.12)
    s.text('VANTA', 12.0, 0.6, 2.4, 0x5c5f66, face='Black', align='CENTER', tracking=1.5, rough=0.12)
    return s.render('mouse_mark', 512)


def pad_sheet():
    """The pad's print, top down (millimeters): a small RiverLine mark in the far corner."""
    w, h = PAD_W * 1000, PAD_D * 1000
    s = Sheet(w, h)
    s.rect(0, 0, w, h, 0x111214, rough=0.9)
    cx, cy = w - 34, h - 20
    for k, dy in enumerate((0.0, -3.4)):
        top, bot = [], []
        for i in range(31):
            t = i / 30
            x = cx - 9 + 18 * t
            yy = cy + dy + 1.6 * math.sin(t * 2 * math.pi * 1.15 + 0.4)
            top.append((x, yy + 0.9))
            bot.append((x, yy - 0.9))
        s.poly(top + list(reversed(bot)), 0x14806e if k == 0 else 0x0f5f52, rough=0.85)
    s.text('RiverLine', cx + 12, cy - 9, 5.0, 0x3d4550, face='Bold', align='RIGHT', rough=0.85)
    return s.render('pad_print', 2048)


def pad_material(printed):
    m = core.Mat('pad_cloth')
    tc, sep = object_coords(m)
    uv = planar(m, sep, 'X', 'Y', -PAD_W / 2, PAD_W / 2, -PAD_D / 2, PAD_D / 2)
    ink, alpha, _, ink_rough = image_surface(m, printed, uv)
    top = ramp(m, sep.outputs['Z'], PAD_T - 0.0006, PAD_T - 0.0002)
    # Woven cloth: a fine basket weave.
    wx = m.math('SINE', m.math('MULTIPLY', sep.outputs['X'], 2 * math.pi / 0.0007))
    wy = m.math('SINE', m.math('MULTIPLY', sep.outputs['Y'], 2 * math.pi / 0.0007))
    weave = m.math('MULTIPLY', m.math('ADD', m.math('MULTIPLY', wx, wy), 1.0), 0.5)
    # Where the mouse lives the weave is rubbed flat and a shade lighter.
    worn = m.math('SUBTRACT', 1.0, ramp(m, m.math('ADD', m.math('MULTIPLY', m.math('SUBTRACT', sep.outputs['X'], 0.01), m.math('SUBTRACT', sep.outputs['X'], 0.01)),
                                                      m.math('MULTIPLY', m.math('ADD', sep.outputs['Y'], 0.02), m.math('ADD', sep.outputs['Y'], 0.02))), 0.0008, 0.0045))
    worn = m.math('MULTIPLY', m.math('MULTIPLY', worn, top), ramp(m, noise(m, tc.outputs['Object'], scale=40.0, detail=3.0), 0.2, 0.7))
    # The stitched edge: a band of thread around the rim.
    edge_d = m.math('MINIMUM', m.math('SUBTRACT', PAD_W / 2, m.math('ABSOLUTE', sep.outputs['X'])), m.math('SUBTRACT', PAD_D / 2, m.math('ABSOLUTE', sep.outputs['Y'])))
    stitch = smooth_less(m, edge_d, 0.0022, 0.0003)
    thread = m.math('MULTIPLY', m.math('ADD', m.math('SINE', m.math('MULTIPLY', m.math('ADD', sep.outputs['X'], sep.outputs['Y']), 2 * math.pi / 0.0012)), 1.0), 0.5)
    color = m.mix(m.math('MULTIPLY', worn, 0.18), ink, core.hex_linear(0x3a3c40))
    color = m.mix(stitch, color, m.mix(m.math('MULTIPLY', thread, 0.5), core.hex_linear(0x16171a), core.hex_linear(0x2b2c30)))
    m.set('Base Color', color)
    m.set('Roughness', m.math('SUBTRACT', mix_float(m, stitch, ink_rough, 0.7), m.math('MULTIPLY', worn, 0.12)))
    height = m.math('ADD', m.math('MULTIPLY', weave, m.math('SUBTRACT', 1.0, m.math('MULTIPLY', worn, 0.7))), m.math('MULTIPLY', m.math('MULTIPLY', thread, stitch), 1.5))
    bump = m.node('ShaderNodeBump', Strength=0.35, Distance=0.00015)
    m.link(height, bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


MARK = None


def build():
    global MARK
    core.reset()
    MARK = mark_sheet()
    printed = pad_sheet()
    core.reset()
    shell, wheel = shell_material(), wheel_material()
    body = sdf.mesh(mouse_prims(), 0.00035, 'mouse_body')
    body_side = sdf.mesh(side_buttons_prims(), 0.0003, 'mouse_side_buttons')
    for o in (body, body_side):
        core.assign(o, shell)
    wh = sdf.mesh(wheel_prims(), 0.0003, 'mouse_wheel')
    core.assign(wh, wheel)
    for o, tris in ((body, 9000), (body_side, 400), (wh, 900)):
        mod = o.modifiers.new('decimate', 'DECIMATE')
        mod.ratio = min(1.0, tris / max(len(o.data.polygons), 1))
        core.select_only([o])
        bpy.ops.object.modifier_apply(modifier=mod.name)
        o.data.shade_smooth()
    mouse = core.join([body, body_side, wh], 'SM_Mouse')
    core.uv_layout(mouse, [
        (core.material_is(mouse, 'mouse_shell'), 'box', None, (0.0, 0.0, 1.0, 1.0)),
        (lambda f: True, 'box', None, (0.0, 0.0, 1.0, 1.0)),
    ])

    bm = core.slab(core.rounded_rect(-PAD_W / 2, -PAD_D / 2, PAD_W / 2, PAD_D / 2, 0.012, steps=8), 0.0, PAD_T)
    core.bevel(bm, lambda e: all(abs(v.co.z - PAD_T) < 1e-7 for v in e.verts), 0.0012, segments=3)
    pad = core.mesh_object('SM_MousePad', bm)
    core.assign(pad, pad_material(printed))
    core.finish_hard_surface(pad)
    core.uv_layout(pad, [
        (lambda f: f.normal.z > 0.9, 'planar', ('x', 'y', -PAD_W / 2, PAD_W / 2, -PAD_D / 2, PAD_D / 2), (0.0, 0.1, 1.0, 1.0)),
        (lambda f: True, 'box', None, (0.0, 0.0, 1.0, 0.09)),
    ])
    return [mouse, pad]


def pose_for_review(objs):
    """The mouse sits on the pad where the stage puts it (1 cm right, 2 cm further from the player)."""
    mouse, pad = objs
    mouse.location = (0.01, 0.02, PAD_T + 0.001)
