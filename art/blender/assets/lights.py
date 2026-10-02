"""SM_RingLight and SM_KeyLight_L/_R: GearDrop's LUMEN streaming lights, each with a _Glow mesh.

- SM_RingLight: a 12" LED ring on a desk tripod with an extension pole and a ball head, an inline dimmer on
  its cable. It stands behind the laptop, facing the chair, tipped down to the face.
- SM_KeyLight_L / _R: a pair of LED panels (aluminum backs with cooling fins) on telescoping poles clamped to
  the desk's left and right edges, each turned toward the player's face.
- The _Glow meshes are the diffusers, exported unbaked: the game lights them (warm white) while streaming.

Coordinates (meters), front toward -Y, Z up. SM_RingLight: origin on the desk under the tripod. The key
lights: origin on the desk top at the pole, 3 cm inside the desk's edge (the left one's edge is at x = -EDGE,
the right one's at +EDGE).
"""
import math

import bpy  # noqa: F401,I001
from mathutils import Matrix, Vector

from artkit import core, parts
from artkit.shade import object_coords, powder_coat

MESHES = ['SM_RingLight', 'SM_RingLight_Glow', 'SM_KeyLight_L', 'SM_KeyLight_L_Glow', 'SM_KeyLight_R', 'SM_KeyLight_R_Glow']
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {'light_steel': 256, 'light_cable': 256, 'light_rubber': 128, 'light_knob': 256}
AO_DISTANCE = 0.02
REVIEW_VIEWS = [('front', -15, 12, 2.6), ('back', 160, 20, 2.6), ('detail', -30, 10, 0.8, (0.0, 0.0, 0.5))]

EDGE = 0.03
DESK_T = 0.03
RING_Z = 0.50   # the ring's center above the desk
RING_R0, RING_R1 = 0.115, 0.155
RING_TILT = 8.0
FACE = Vector((0.775, -0.82, 0.37))  # the player's face from the left key light's pole
KEY_Z = 0.73


def paint(name, hex_color, rough=0.45):
    m = core.Mat(name)
    tc, _ = object_coords(m)
    color, r, metal, bump = powder_coat(m, tc, hex_color, rough=rough, chips=0.3)
    m.set('Base Color', color)
    m.set('Roughness', r)
    m.set('Metallic', metal)
    m.set('Normal', bump.outputs['Normal'])
    return m


def materials():
    return {'paint': paint('light_paint', 0x161719), 'housing': parts.plastic('light_housing', 0x1b1c1f, rough=0.42, scuffs=0.2),
            'alu': parts.metal('light_alu', 0x2c2e33, rough=0.38, brushed_axis='X', anodized=True),
            'steel': parts.metal('light_steel', 0xb6b9be, rough=0.25), 'cable': parts.rubber('light_cable', 0x0f0f0f, rough=0.55),
            'rubber': parts.rubber('light_rubber'), 'knob': parts.plastic('light_knob', 0x0e0e10, rough=0.4),
            'diffuser': parts.plastic('light_diffuser', 0xd9d6cf, rough=0.6, scuffs=0.0)}


def ring_light(mats):
    out = []
    hub = Vector((0.0, 0.0, 0.10))
    for k in range(3):
        a = math.radians(90 + 120 * k)
        foot = Vector((0.09 * math.cos(a), 0.09 * math.sin(a), 0.006))
        out.append((parts.rod('leg', hub, foot + Vector((0, 0, 0.004)), 0.005, 12), mats['paint']))
        out.append((parts.rbox('foot', foot, (0.016, 0.016, 0.012), 0.005, 3), mats['rubber']))
    out.append((parts.disc('crown', hub, (0, 0, 1), 0.016, 0.024, 28), mats['paint']))
    pole_top = RING_Z - RING_R1 - 0.05
    out.append((parts.rod('pole_lo', hub, (0, 0, 0.26), 0.0095, 20), mats['paint']))
    out.append((parts.disc('lock', (0, 0, 0.26), (0, 0, 1), 0.0135, 0.018, 24), mats['knob']))
    out.append((parts.rod('lock_knob', (0, 0, 0.26), (0.0, -0.022, 0.26), 0.0035, 10), mats['knob']))
    out.append((parts.rod('pole_hi', (0, 0, 0.26), (0, 0, pole_top), 0.0075, 20), mats['steel']))
    ball_c = Vector((0.0, 0.0, pole_top + 0.014))
    ball = core.lathe('ball', [(0.0, -0.013)] + [(0.013 * math.sin(math.radians(a)), -0.013 * math.cos(math.radians(a))) for a in range(15, 180, 15)] + [(0.0, 0.013)], segments=28)
    core.orient_normals(ball)
    ball.data.transform(Matrix.Translation(ball_c))
    out.append((ball, mats['steel']))
    out.append((parts.disc('socket', ball_c - Vector((0, 0, 0.011)), (0, 0, 1), 0.015, 0.012, 28), mats['paint']))
    # The ring: a rounded-section housing around Y, its bracket at the bottom, the dimmer on the cable.
    sec = core.rounded_rect(RING_R0, -0.011, RING_R1, 0.011, 0.0065, steps=4)
    housing = core.lathe('ring', sec + [sec[0]], segments=120)
    core.orient_normals(housing)
    housing.data.transform(parts.rot_x(90))
    mount = parts.rbox('mount', (0.0, 0.004, -RING_R1 - 0.012), (0.036, 0.03, 0.026), 0.006)
    switch = parts.rbox('switch', (RING_R1 * 0.72, 0.012, -RING_R1 * 0.72), (0.022, 0.006, 0.014), 0.002, rot=parts.rot_y(-45).to_3x3())
    glow = parts.annulus('diffuser', RING_R0 + 0.004, RING_R1 - 0.004, 0.0, 0.0012, 120, Matrix.Translation((0, -0.0112, 0)) @ parts.rot_x(90))
    place = Matrix.Translation((0, 0, RING_Z)) @ parts.rot_x(RING_TILT)
    ring = [(housing, mats['housing']), (mount, mats['paint']), (switch, mats['knob'])]
    parts.xform([o for o, _ in ring] + [glow], place)
    out += ring
    out.append((parts.rod('neck', ball_c + Vector((0, 0, 0.008)), place @ Vector((0.0, 0.004, -RING_R1 - 0.02)), 0.006, 14), mats['steel']))
    # The cable: from the mount down the pole, the dimmer halfway, off the desk's back.
    m_bot = place @ Vector((0.0, 0.014, -RING_R1 - 0.02))
    path = [m_bot, m_bot + Vector((0, 0.008, -0.03)), Vector((0.0, 0.016, pole_top - 0.04)), Vector((0.0, 0.016, 0.2)), Vector((0.008, 0.03, 0.12)),
            Vector((0.03, 0.06, 0.03)), Vector((0.04, 0.08, 0.004)), Vector((0.06, 0.2, 0.004))]
    cord = parts.tube('cable', path, 0.0022, 8, 8)
    dimmer = parts.rbox('dimmer', (0.0, 0.016, 0.2), (0.016, 0.012, 0.05), 0.004)
    out += [(cord, mats['cable']), (dimmer, mats['knob'])]
    for o, mat in out:
        core.assign(o, mat)
    return parts.finish([o for o, _ in out], 'SM_RingLight', 40), parts.glow_object([glow], 'SM_RingLight_Glow')


def key_light(mats, side, name):
    """side -1: clamped to the desk's left edge; +1: the right. Mirrored in X, both aimed at the face."""
    out = []
    e = side * EDGE  # the edge, from the pole
    out += [(parts.rbox('clamp_top', (e * 0.4, 0.0, 0.005), (0.085, 0.06, 0.01), 0.003), mats['paint']),
            (parts.rbox('clamp_spine', (e + side * 0.008, 0.0, -DESK_T / 2 - 0.004), (0.01, 0.05, DESK_T + 0.028), 0.003), mats['paint']),
            (parts.rbox('clamp_jaw', (e - side * 0.022, 0.0, -DESK_T - 0.016), (0.07, 0.05, 0.01), 0.003), mats['paint']),
            (parts.rod('screw', (e - side * 0.035, 0.0, -DESK_T - 0.06), (e - side * 0.035, 0.0, -DESK_T - 0.003), 0.004, 12), mats['steel']),
            (parts.disc('pad', (e - side * 0.035, 0.0, -DESK_T - 0.0025), (0, 0, 1), 0.014, 0.004, 24), mats['steel'])]
    out.append((parts.rod('pole_lo', (0, 0, 0.009), (0, 0, 0.42), 0.0125, 24), mats['paint']))
    out.append((parts.disc('collar', (0, 0, 0.42), (0, 0, 1), 0.016, 0.022, 24), mats['knob']))
    out.append((parts.rod('collar_knob', (0, 0, 0.42), (0.0, -0.026, 0.42), 0.004, 10), mats['knob']))
    out.append((parts.rod('pole_hi', (0, 0, 0.42), (0, 0, KEY_Z - 0.03), 0.0095, 20), mats['steel']))
    face = Vector((FACE.x * -side, FACE.y, FACE.z)) if side > 0 else FACE
    ball_c = Vector((0.0, 0.0, KEY_Z - 0.016))
    out.append((parts.disc('head', ball_c, (0, 0, 1), 0.013, 0.03, 24), mats['paint']))
    aim = (face - ball_c).normalized()
    center = ball_c + aim * 0.05 + Vector((0, 0, 0.03))
    f = parts.frame(center, aim)
    w, h, d = 0.35, 0.25, 0.034
    panel = [(parts.rbox('panel', (0, 0, 0), (w, d, h), 0.012, 4), mats['alu']),
             (parts.rbox('bezel', (0, -d / 2 + 0.002, 0), (w - 0.004, 0.005, h - 0.004), 0.01, 4), mats['housing'])]
    for k in range(13):
        x = -w / 2 + 0.03 + k * (w - 0.06) / 12
        panel.append((parts.rbox('fin', (x, d / 2 + 0.004, 0.0), (0.003, 0.008, h - 0.05), 0.0012), mats['alu']))
    panel.append((parts.rbox('badge', (0.0, d / 2 + 0.0085, -h / 2 + 0.018), (0.05, 0.002, 0.01), 0.001), mats['steel']))
    panel.append((parts.disc('button', (w / 2 - 0.03, 0.0, h / 2 + 0.002), (0, 0, 1), 0.006, 0.004, 20), mats['knob']))
    panel.append((parts.rbox('bracket', (0.0, d / 2 + 0.018, -h / 2 + 0.03), (0.03, 0.03, 0.05), 0.006), mats['paint']))
    glow = parts.rbox('diffuser', (0, -d / 2 - 0.0008, 0), (w - 0.022, 0.002, h - 0.022), 0.006, 3)
    parts.xform([o for o, _ in panel] + [glow], f)
    out += panel
    out.append((parts.rod('arm', ball_c + Vector((0, 0, 0.012)), f @ Vector((0.0, d / 2 + 0.03, -h / 2 + 0.03)), 0.007, 14), mats['steel']))
    # Power cable: from the panel's back, down the pole, over the edge.
    start = f @ Vector((0.06 * -side, d / 2 + 0.004, -h / 2 + 0.01))
    path = [start, start + Vector((0, 0, -0.04)), Vector((0.0, 0.016, KEY_Z - 0.12)), Vector((0.0, 0.016, 0.3)), Vector((0.0, 0.018, 0.05)),
            Vector((e * 0.6, 0.02, 0.012)), Vector((e + side * 0.012, 0.02, -0.01)), Vector((e + side * 0.014, 0.02, -0.25))]
    out.append((parts.tube('cable', path, 0.0022, 8, 6), mats['cable']))
    for o, mat in out:
        core.assign(o, mat)
    return parts.finish([o for o, _ in out], name, 40), parts.glow_object([glow], name + '_Glow')


def build():
    core.reset()
    mats = materials()
    ring, ring_glow = ring_light(mats)
    kl, kl_glow = key_light(mats, -1, 'SM_KeyLight_L')
    kr, kr_glow = key_light(mats, 1, 'SM_KeyLight_R')
    return [ring, ring_glow, kl, kl_glow, kr, kr_glow]


def bake_parts(objs):
    return [o for o in objs if not o.name.endswith('_Glow')]


def pose_for_review(objs):
    for o in objs[2:4]:
        o.location.x = -0.8
    for o in objs[4:6]:
        o.location.x = 0.8
    slab = parts.rbox('review_desk', (0.0, -0.3, -DESK_T / 2), (1.66, 0.7, DESK_T), 0.0)
    m = core.Mat('review_desk')
    m.set('Base Color', core.hex_linear(0x3b2616))
    m.set('Roughness', 0.45)
    core.assign(slab, m)
