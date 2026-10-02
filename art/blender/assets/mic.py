"""SM_MicUsb and SM_MicArm: GearDrop's CORVID microphones.

- SM_MicUsb: a USB condenser on its desk stand: a weighted round base, a U-yoke with side knobs, the
  capsule body (painted below, a perforated grille above), a mute button lit amber, a gain knob, the cable.
- SM_MicArm: the broadcast kit: a dynamic mic with a foam windscreen in a yoke, hung from a spring-balanced
  boom arm clamped to the desk's left edge and reaching in front of the player's mouth; the XLR cable is
  strapped along the arm and drops off the edge.

Coordinates (meters), front toward -Y, Z up. SM_MicUsb: origin on the desk under the base's center.
SM_MicArm: origin on the desk top at the arm's post, 3 cm inside the desk's left edge (the edge is at
x = -EDGE); the mic hangs about 0.53 m to its right, 0.16 m toward the player, 0.3 m up.
"""
import math

import bpy  # noqa: F401,I001
from mathutils import Matrix, Vector

from artkit import core, parts
from artkit.shade import noise, object_coords, powder_coat, ramp

MESHES = ['SM_MicUsb', 'SM_MicArm']
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {'mic_led': 64, 'mic_steel': 256, 'mic_cable': 256, 'mic_knob': 256}
AO_DISTANCE = 0.012
REVIEW_VIEWS = [('front', -25, 12, 1.6), ('side', 60, 20, 1.6), ('detail', -20, 10, 0.45, (0.0, 0.0, 0.15))]

EDGE = 0.03
DESK_T = 0.03
POST = Vector((0.0, 0.0, 0.105))     # the arm's shoulder pivot
MOUNT = Vector((0.475, -0.15, 0.37))  # where the arm ends above the mic
MOUTH = Vector((0.775, -0.58, 0.37))  # the player's mouth, from the post
ARM = 0.40                             # each arm, pivot to pivot
ROD_GAP = 0.011


# ------------------------------------------------------------------ materials

def paint(name, hex_color, rough=0.45):
    m = core.Mat(name)
    tc, _ = object_coords(m)
    color, r, metal, bump = powder_coat(m, tc, hex_color, rough=rough, chips=0.35)
    m.set('Base Color', color)
    m.set('Roughness', r)
    m.set('Metallic', metal)
    m.set('Normal', bump.outputs['Normal'])
    return m


def grille_material():
    """A perforated steel grille wrapped around a cylinder about Z: holes in rows around and along it."""
    m = core.Mat('mic_grille')
    tc, sep = object_coords(m)
    ang = m.node('ShaderNodeMath', _operation='ARCTAN2')
    m.link(sep.outputs['Y'], ang.inputs[0])
    m.link(sep.outputs['X'], ang.inputs[1])
    u = m.math('MULTIPLY', ang.outputs[0], 0.04)  # arc length on the 40 mm body
    pitch = 0.0028
    # Every other row shifted half a pitch: a staggered pattern.
    row = m.math('FLOOR', m.math('DIVIDE', sep.outputs['Z'], pitch))
    shift = m.math('MULTIPLY', m.math('MODULO', row, 2.0), 0.5)
    fa = m.math('SUBTRACT', m.math('FRACT', m.math('ADD', m.math('DIVIDE', u, pitch), shift)), 0.5)
    fb = m.math('SUBTRACT', m.math('FRACT', m.math('DIVIDE', sep.outputs['Z'], pitch)), 0.5)
    d = m.math('SQRT', m.math('ADD', m.math('MULTIPLY', fa, fa), m.math('MULTIPLY', fb, fb)))
    hole = ramp(m, m.math('SUBTRACT', 0.3, d), 0.0, 0.06)
    m.set('Base Color', m.mix(hole, core.hex_linear(0x3a3c40), core.hex_linear(0x050505)))
    m.set('Metallic', m.math('SUBTRACT', 1.0, hole))
    m.set('Roughness', m.math('ADD', 0.35, m.math('MULTIPLY', hole, 0.5)))
    bump = m.node('ShaderNodeBump', Strength=0.6, Distance=0.0005)
    m.link(m.math('SUBTRACT', 1.0, hole), bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def foam_material():
    m = core.Mat('mic_foam')
    tc, _ = object_coords(m)
    cells = noise(m, tc.outputs['Object'], scale=900.0, detail=6.0, roughness=0.7)
    m.set('Base Color', m.mix(m.math('MULTIPLY', cells, 0.5), core.hex_linear(0x101011), core.hex_linear(0x232326)))
    m.set('Roughness', 0.95)
    bump = m.node('ShaderNodeBump', Strength=0.9, Distance=0.001)
    m.link(cells, bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


# ------------------------------------------------------------------ USB mic

def usb_mic(mats):
    body_m, grille_m, knob_m, rubber_m, led_m, cable_m, steel_m = (mats[k] for k in ('body', 'grille', 'knob', 'rubber', 'led', 'cable', 'steel'))
    base = core.lathe('base', [(0.0, 0.0), (0.056, 0.0), (0.059, 0.003), (0.0595, 0.011), (0.056, 0.016), (0.045, 0.019), (0.0, 0.0195)], segments=72)
    core.orient_normals(base)
    core.assign(base, body_m)
    ring = core.lathe('foot', [(0.0, -0.0001), (0.05, -0.0001), (0.05, 0.002), (0.0, 0.002)], segments=48)
    core.orient_normals(ring)
    core.assign(ring, rubber_m)
    # The yoke: a U from the base up both sides, a knob at the top of each arm.
    yoke = [parts.rbox('yoke_bar', (0.0, 0.0, 0.026), (0.104, 0.024, 0.012), 0.004)]
    yoke += [parts.rbox('yoke_arm', (s * 0.048, 0.0, 0.075), (0.008, 0.024, 0.11), 0.0035) for s in (-1, 1)]
    yoke.append(parts.disc('yoke_post', (0.0, 0.0, 0.0225), (0, 0, 1), 0.012, 0.008, 24))
    parts.assign_all(yoke, body_m)
    pivot = Vector((0.0, 0.0, 0.118))
    knobs = [parts.disc('knob', pivot + Vector((s * 0.059, 0, 0)), (1, 0, 0), 0.015, 0.013, 32) for s in (-1, 1)]
    knobs += [parts.disc('knob_cap', pivot + Vector((s * 0.0662, 0, 0)), (1, 0, 0), 0.011, 0.0015, 32) for s in (-1, 1)]
    parts.assign_all(knobs, knob_m)
    # The capsule body around Z, pivoting between the knobs: painted below, grille above.
    r = 0.04
    lower = core.lathe('body', [(0.0, -0.062), (r - 0.006, -0.062), (r - 0.001, -0.058), (r, -0.052), (r, 0.03), (r + 0.0006, 0.03), (r + 0.0006, 0.034), (0.0, 0.034)], segments=72)
    dome = [(r - 0.0004, 0.034), (r - 0.0004, 0.072)] + [(r * 0.99 * math.cos(math.radians(a)), 0.072 + 0.04 * 0.95 * math.sin(math.radians(a))) for a in range(6, 91, 6)]
    dome[-1] = (0.0, dome[-1][1])
    upper = core.lathe('grille', [(0.0, 0.034)] + dome, segments=72)
    for o in (lower, upper):
        core.orient_normals(o)
    core.assign(lower, body_m)
    core.assign(upper, grille_m)
    # Mute button with its lit ring on the front, a gain knob on the back, the headphone jack below it.
    mute = parts.disc('mute', (0.0, -r - 0.002, 0.0), (0, 1, 0), 0.0085, 0.005, 32)
    core.assign(mute, knob_m)
    led = parts.annulus('mute_ring', 0.0085, 0.0105, 0.0, 0.0012, 32, Matrix.Translation((0.0, -r - 0.0002, 0.0)) @ parts.rot_x(90))
    core.assign(led, led_m)
    gain = parts.disc('gain', (0.0, r + 0.004, 0.0), (0, 1, 0), 0.009, 0.009, 32)
    jack = parts.disc('jack', (0.0, r + 0.0005, -0.03), (0, 1, 0), 0.0035, 0.002, 16)
    core.assign(gain, knob_m)
    core.assign(jack, steel_m)
    plate = parts.rbox('badge', (0.0, -r - 0.0004, -0.03), (0.022, 0.0012, 0.009), 0.0005)
    core.assign(plate, steel_m)
    capsule = [lower, upper, mute, led, gain, jack, plate]
    parts.xform(capsule, Matrix.Translation(pivot))
    # The USB cable out of the bottom, down behind the base.
    bottom = pivot + Vector((0.0, 0.01, -0.062))
    cord = parts.tube('cable', [bottom, bottom + Vector((0, 0.012, -0.02)), Vector((0.0, 0.07, 0.022)), Vector((0.01, 0.1, 0.0025)), Vector((0.03, 0.2, 0.0025))], 0.0022, 10, 8)
    core.assign(cord, cable_m)
    return parts.finish([base, ring] + yoke + knobs + capsule + [cord], 'SM_MicUsb', 40)


# ------------------------------------------------------------------ boom arm

def arm_rods(a, b, side_axis, parts_list, mats):
    paint_m = mats['arm']
    for s in (-1, 1):
        off = side_axis * (s * ROD_GAP)
        parts_list.append((core.sweep('rod', [a + off, b + off], core.rounded_rect(-0.0045, -0.004, 0.0045, 0.004, 0.0015, steps=2)), paint_m))
    for t in (0.1, 0.5, 0.9):
        parts_list.append((parts.disc('spacer', a + (b - a) * t, side_axis, 0.004, 2 * ROD_GAP + 0.008, 16), paint_m))


def boom(mats):
    paint_m, steel_m, knob_m, cable_m = mats['arm'], mats['steel'], mats['knob'], mats['cable']
    out = []
    # Clamp on the left edge (outside is -X).
    out += [(parts.rbox('clamp_top', (-EDGE * 0.4, 0.0, 0.005), (0.085, 0.06, 0.01), 0.003), paint_m),
            (parts.rbox('clamp_spine', (-EDGE - 0.008, 0.0, -DESK_T / 2 - 0.004), (0.01, 0.05, DESK_T + 0.028), 0.003), paint_m),
            (parts.rbox('clamp_jaw', (-EDGE + 0.022, 0.0, -DESK_T - 0.016), (0.07, 0.05, 0.01), 0.003), paint_m),
            (parts.rod('screw', (-EDGE + 0.035, 0.0, -DESK_T - 0.06), (-EDGE + 0.035, 0.0, -DESK_T - 0.003), 0.004, 12), steel_m),
            (parts.disc('pad', (-EDGE + 0.035, 0.0, -DESK_T - 0.0025), (0, 0, 1), 0.014, 0.004, 24), steel_m),
            (parts.rod('handle', (-EDGE + 0.035, -0.03, -DESK_T - 0.055), (-EDGE + 0.035, 0.03, -DESK_T - 0.055), 0.003, 10), steel_m)]
    # The post, its swivel collar and the shoulder.
    out += [(parts.rod('post', (0, 0, 0.009), (0, 0, POST.z - 0.012), 0.011, 24), paint_m),
            (parts.disc('collar', (0, 0, 0.03), (0, 0, 1), 0.016, 0.02, 24), knob_m)]
    flat = Vector((MOUNT.x, MOUNT.y, 0.0)).normalized()
    side = Vector((-flat.y, flat.x, 0.0))
    up = Vector((0, 0, 1))
    sm = MOUNT - POST
    dist = Vector((sm.dot(flat), sm.z)).length
    along = Vector((sm.dot(flat), sm.z)) / dist
    h = math.sqrt(max(ARM * ARM - (dist / 2) ** 2, 0.0))
    mid = Vector((sm.dot(flat), sm.z)) / 2
    e2 = mid + Vector((-along.y, along.x)) * h
    elbow = POST + flat * e2.x + up * e2.y
    out.append((parts.rbox('shoulder', POST + Vector((0, 0, -0.004)), (0.03, 0.03, 0.026), 0.006), paint_m))
    rods = []
    arm_rods(POST, elbow, side, rods, mats)
    arm_rods(elbow, MOUNT, side, rods, mats)
    out += rods
    for p, r in ((POST, 0.012), (elbow, 0.013), (MOUNT, 0.011)):
        out.append((parts.disc('knuckle', p, side, r, 2 * ROD_GAP + 0.006, 28), paint_m))
        out.append((parts.disc('knob', p + side * (ROD_GAP + 0.01), side, 0.009, 0.008, 24), knob_m))
    # Springs: along the lower arm on both sides, and one under the upper arm.
    lower = (elbow - POST).normalized()
    below = lower.cross(side).normalized()
    if below.z > 0:
        below = -below
    for s in (-1, 1):
        a = POST + lower * 0.05 + side * (s * (ROD_GAP + 0.009)) + below * 0.012
        out.append((parts.spring('spring', a, a + lower * 0.16, coil_r=0.0045), steel_m))
        out.append((parts.rod('link', a + lower * 0.16, a + lower * 0.26 - below * 0.008, 0.0009, 6), steel_m))
    upper = (MOUNT - elbow).normalized()
    a = elbow + upper * 0.05 + below * 0.014
    out.append((parts.spring('spring_upper', a, a + upper * 0.12, coil_r=0.004), steel_m))

    # The mic hangs from the mount on a yoke and points at the mouth.
    aim = (MOUTH - (MOUNT - Vector((0, 0, 0.075)))).normalized()
    center = MOUNT - Vector((0, 0, 0.075))
    m = parts.frame(center, aim)  # -Y of the mic's frame points at the mouth
    mic = []
    r = 0.031
    body = core.lathe('mic_body', [(0.0, -0.07), (r - 0.004, -0.07), (r, -0.066), (r, 0.03), (r - 0.002, 0.034), (0.0, 0.034)], segments=64)
    core.orient_normals(body)
    body.data.transform(parts.rot_x(-90))  # its axis along Y: the front (z = -0.07) toward -Y
    core.assign(body, mats['mic_body'])
    foam = core.lathe('foam', [(0.0, 0.0), (0.03, 0.0)] + [(0.034 + 0.006 * math.sin(math.radians(a)), 0.004 + 0.075 * a / 180.0) for a in range(0, 181, 15)] +
                      [(0.026, 0.083), (0.0, 0.086)], segments=64)
    core.orient_normals(foam)
    foam.data.transform(Matrix.Translation((0, -0.06, 0)) @ parts.rot_x(90))
    core.assign(foam, mats['foam'])
    xlr = parts.disc('xlr', (0.0, 0.04, 0.0), (0, 1, 0), 0.012, 0.012, 24)
    core.assign(xlr, steel_m)
    mic += [body, foam, xlr]
    # The yoke: a U around the body, its knobs at the sides, its stem up to the mount.
    yoke_bar = core.sweep('yoke', core.catmull_rom([(-0.038, 0.0, 0.0), (-0.038, 0.0, 0.03), (-0.025, 0.0, 0.05), (0.0, 0.0, 0.056), (0.025, 0.0, 0.05), (0.038, 0.0, 0.03), (0.038, 0.0, 0.0)], 6),
                           core.rounded_rect(-0.002, -0.009, 0.002, 0.009, 0.0015, steps=2))
    core.assign(yoke_bar, mats['mic_body'])
    yk = [yoke_bar] + [parts.disc('yoke_knob', (s * 0.044, 0.0, 0.0), (1, 0, 0), 0.011, 0.01, 24) for s in (-1, 1)]
    for o in yk[1:]:
        core.assign(o, knob_m)
    mic += yk
    parts.xform(mic, m)
    stem_top = MOUNT - Vector((0, 0, 0.004))
    stem_bot = m @ Vector((0.0, 0.0, 0.056))
    out.append((parts.rod('stem', stem_top, stem_bot, 0.0055, 16), steel_m))
    for o in mic:
        out.append((o, None))

    # The XLR cable: out of the mic's back, strapped along the arms, down the post and over the edge.
    back = m @ Vector((0.0, 0.05, 0.0))
    path = [back, back + Vector((0, 0, 0.02)) + (back - center).normalized() * 0.02, MOUNT + below * 0.02 + Vector((0, 0, -0.01)),
            (MOUNT + elbow) / 2 + below * 0.022, elbow + below * 0.02, (elbow + POST) / 2 + below * 0.024,
            POST + below * 0.02 + Vector((-0.004, 0, 0)), Vector((-0.016, 0.0, 0.05)), Vector((-0.02, 0.0, 0.012)),
            Vector((-EDGE - 0.006, 0.0, 0.01)), Vector((-EDGE - 0.016, 0.0, -0.02)), Vector((-EDGE - 0.018, 0.0, -0.22))]
    out.append((parts.tube('cable', path, 0.003, 10, 6), cable_m))
    # Velcro straps where the cable meets the arm.
    for t in (0.3, 0.7):
        for a, b in ((POST, elbow), (elbow, MOUNT)):
            p = a + (b - a) * t
            out.append((parts.disc('strap', p + below * 0.012, (b - a), 0.016, 0.012, 16), mats['strap']))
    objs = []
    for o, mat in out:
        if mat is not None:
            core.assign(o, mat)
        objs.append(o)
    return parts.finish(objs, 'SM_MicArm', 40)


def build():
    core.reset()
    mats = {'body': paint('mic_paint', 0x1d1e22, 0.4), 'grille': grille_material(), 'knob': parts.plastic('mic_knob', 0x0f0f11, rough=0.4),
            'rubber': parts.rubber('mic_rubber'), 'led': parts.emissive('mic_led', 0xffb020, 1.0), 'cable': parts.rubber('mic_cable', 0x0d0d0d, rough=0.55),
            'steel': parts.metal('mic_steel', 0xb8bbc0, rough=0.25), 'arm': paint('mic_arm', 0x141517, 0.45),
            'mic_body': paint('mic_dynamic', 0x101113, 0.5), 'foam': foam_material(), 'strap': parts.fabric('mic_strap', 0x151515, period=0.001)}
    usb = usb_mic(mats)
    arm = boom(mats)
    return [usb, arm]


def pose_for_review(objs):
    objs[0].location.x = -0.25
    objs[0].location.y = -0.2
    slab = parts.rbox('review_desk', (0.3, -0.25, -DESK_T / 2), (0.75, 0.6, DESK_T), 0.0)
    m = core.Mat('review_desk')
    m.set('Base Color', core.hex_linear(0x3b2616))
    m.set('Roughness', 0.45)
    core.assign(slab, m)
