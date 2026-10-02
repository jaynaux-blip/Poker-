"""SM_Monitor and SM_MonitorWide: GearDrop's VANTAGE monitors, each on a desk-clamp arm.

- SM_Monitor: the 24" 144 Hz panel, on the right of the desk.
- SM_MonitorWide: the 27" 1440p panel, on the left (the arm mirrored).
Each is one mesh: a C-clamp on the desk's back edge, a pole, a two-link arm with a tilt head and a VESA
plate, and the panel itself (thin bezels, a tapered back housing, a power LED), turned toward the chair and
tilted back. A cable runs from the panel along the arm and down behind the desk.

Coordinates (meters), front toward -Y, Z up: the origin is on the desk top at the pole's axis, 3 cm in
front of the desk's back edge. The display's center and size are in SCREENS, for the game to draw on.
"""
import math

import bpy  # noqa: F401,I001
from mathutils import Matrix, Vector

from artkit import core, parts
from artkit.shade import object_coords, powder_coat

MESHES = ['SM_Monitor', 'SM_MonitorWide']
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {'monitor_screen': 256, 'monitor_led': 64, 'monitor_steel': 256, 'monitor_cable': 256}
AO_DISTANCE = 0.03
REVIEW_VIEWS = [('front', -20, 12, 2.2), ('back', 150, 20, 2.2), ('detail', 120, 15, 0.8, (0.0, 0.0, 0.25))]

EDGE = 0.03     # the desk's back edge, behind the pole
DESK_T = 0.030  # desk thickness
POLE_H = 0.40
LINK = 0.12     # each arm link, pivot to pivot

# name: (active width, active height, side bezel, top bezel, chin, side, panel center, yaw, pitch)
# Panel center (the display's center, on its front) is relative to the pole; yaw turns the screen toward the
# chair (from the stage: the eye is about 0.95 m in front of the panels and 12 cm above their centers).
SCREENS = {
    'SM_Monitor': (0.5313, 0.2989, 0.0065, 0.0065, 0.0170, 1, (-0.060, -0.235, 0.295), -31.9, -7.5),
    'SM_MonitorWide': (0.5968, 0.3357, 0.0065, 0.0065, 0.0170, -1, (-0.040, -0.245, 0.315), 33.2, -6.3),
}
THIN = 0.011     # the panel's edge thickness
HOUSING = 0.040  # the back housing's depth behind the screen


# ------------------------------------------------------------------ materials

def paint_material():
    m = core.Mat('monitor_paint')
    tc, _ = object_coords(m)
    color, rough, metal, bump = powder_coat(m, tc, 0x18191c, rough=0.5, chips=0.25)
    m.set('Base Color', color)
    m.set('Roughness', rough)
    m.set('Metallic', metal)
    m.set('Normal', bump.outputs['Normal'])
    return m


# ------------------------------------------------------------------ geometry

def xz_panel(name, outline, y0, y1, r):
    """A prism from an outline in the XZ plane (x right, z up), from y0 back to y1, rims rounded by r."""
    return parts.panel(name, outline, -y1, -y0, r, 3, parts.rot_x(90))


def ring_xz(cx, cz, w, h, r, y, steps=6):
    return [Vector((cx + x, y, cz + z)) for x, z in core.rounded_rect(-w / 2, -h / 2, w / 2, h / 2, r, steps=steps)]


def display(name, aw, ah, side_b, top_b, chin, mats):
    """The panel in its own frame: the display centered at the origin on the front plane (y = 0), facing -Y."""
    plastic, screen, led = mats
    x0, x1 = -aw / 2 - side_b, aw / 2 + side_b
    z0, z1 = -ah / 2 - chin, ah / 2 + top_b
    shell = xz_panel('shell', core.rounded_rect(x0, z0, x1, z1, 0.005, steps=4), 0.0, THIN, 0.0025)
    # The back housing: rounded slabs stepping in, lofted, a little below center where the electronics sit.
    w, h = x1 - x0, z1 - z0
    cz = (z0 + z1) / 2 - 0.012
    rings = [ring_xz(0.0, cz, w * 0.86, h * 0.84, 0.04, THIN - 0.001), ring_xz(0.0, cz, w * 0.8, h * 0.76, 0.04, THIN + 0.012),
             ring_xz(0.0, cz, w * 0.68, h * 0.62, 0.035, THIN + 0.024), ring_xz(0.0, cz, w * 0.56, h * 0.5, 0.03, HOUSING)]
    housing = core.loft('housing', rings)
    # Vent slots along the housing's top: shallow ribs.
    vents = [parts.rbox('vent', (x, THIN + 0.016, cz + h * 0.36), (0.004, 0.012, 0.003), 0.0012) for x in [k * 0.012 for k in range(-9, 10)]]
    parts.assign_all([shell, housing] + vents, plastic)
    glass = xz_panel('screen', core.rounded_rect(-aw / 2, -ah / 2, aw / 2, ah / 2, 0.0015, steps=2), -0.0004, 0.0004, 0.0)
    core.assign(glass, screen)
    # The power LED in the chin, bottom right, and a joystick nub under it.
    dot = parts.disc('led', (x1 - 0.028, -0.0006, z0 + chin * 0.45), (0, 1, 0), 0.0016, 0.0012, 16)
    core.assign(dot, led)
    nub = parts.disc('nub', (x1 - 0.05, THIN * 0.5, z0 - 0.002), (0, 0, 1), 0.004, 0.005, 16)
    core.assign(nub, plastic)
    return [shell, housing, glass, dot, nub] + vents


def clamp(paint, steel):
    """The C-clamp over the desk's back edge (the edge is EDGE behind the pole)."""
    p = [parts.rbox('clamp_top', (0.0, EDGE * 0.35, 0.005), (0.075, 0.09, 0.010), 0.003),
         parts.rbox('clamp_spine', (0.0, EDGE + 0.008, -DESK_T / 2 - 0.004), (0.05, 0.010, DESK_T + 0.028), 0.003),
         parts.rbox('clamp_jaw', (0.0, EDGE - 0.022, -DESK_T - 0.016), (0.05, 0.07, 0.010), 0.003)]
    parts.assign_all(p, paint)
    screw = parts.rod('screw', (0.0, EDGE - 0.035, -DESK_T - 0.05), (0.0, EDGE - 0.035, -DESK_T - 0.003), 0.004, 12)
    pad = parts.disc('pad', (0.0, EDGE - 0.035, -DESK_T - 0.0025), (0, 0, 1), 0.014, 0.004, 24)
    handle = parts.rod('handle', (-0.03, EDGE - 0.035, -DESK_T - 0.046), (0.03, EDGE - 0.035, -DESK_T - 0.046), 0.003, 10)
    parts.assign_all([screw, pad, handle], steel)
    return p + [screw, pad, handle]


def elbow_point(h, side):
    """Where the two equal links meet between the pole (origin) and h, bending out to the panel's side."""
    d = Vector((h.x, h.y))
    dist = min(d.length, 2 * LINK - 1e-4)
    mid = d.normalized() * (dist / 2)
    off = math.sqrt(max(LINK * LINK - (dist / 2) ** 2, 0.0))
    perp = Vector((-d.y, d.x)).normalized()
    if perp.x * side < 0:
        perp = -perp
    e = mid + perp * off
    return Vector((e.x, e.y, h.z))


def link(name, a, b):
    """An arm link from a to b (horizontal): a rounded box section, 30 mm tall, 22 mm wide."""
    profile = core.rounded_rect(-0.015, -0.011, 0.015, 0.011, 0.007, steps=4)
    return core.sweep(name, [a, b], profile)


def materials():
    return {'plastic': parts.plastic('monitor_plastic', 0x111214, rough=0.48, scuffs=0.3), 'screen': parts.screen_glass('monitor_screen'),
            'led': parts.emissive('monitor_led', 0x9fd0ff, 1.0), 'paint': paint_material(),
            'steel': parts.metal('monitor_steel', 0xa9acb1, rough=0.32), 'cable': parts.rubber('monitor_cable', 0x101010, rough=0.6)}


def monitor(name, mats):
    aw, ah, sb, tb, chin, side, center, yaw, pitch = SCREENS[name]
    plastic, screen, led, paint, steel, cable = (mats[k] for k in ('plastic', 'screen', 'led', 'paint', 'steel', 'cable'))

    # The panel, turned and tilted into place.
    pm = Matrix.Translation(center) @ parts.rot_z(yaw) @ parts.rot_x(pitch)
    panel = parts.xform(display('display', aw, ah, sb, tb, chin, (plastic, screen, led)), pm)
    back = (pm.to_3x3() @ Vector((0, 1, 0))).normalized()
    up = (pm.to_3x3() @ Vector((0, 0, 1))).normalized()
    plate_c = Vector(center) + back * (HOUSING + 0.003) + up * (-0.012)
    # The VESA plate and the tilt hinge behind it.
    pieces = []
    plate = parts.rbox('vesa', (0, 0, 0), (0.105, 0.006, 0.105), 0.004)
    parts.xform(plate, Matrix.Translation(plate_c) @ pm.to_3x3().to_4x4())
    pieces.append(plate)
    tilt_c = plate_c + back * 0.016
    flat_back = Vector((back.x, back.y, 0.0)).normalized()
    hinge = parts.disc('tilt', tilt_c, Vector((-flat_back.y, flat_back.x, 0.0)), 0.012, 0.05, 24)
    pieces.append(hinge)
    for s in (-1, 1):
        cheek = parts.rbox('cheek', (0, 0, 0), (0.006, 0.026, 0.04), 0.002)
        across = Vector((-flat_back.y, flat_back.x, 0.0))
        parts.xform(cheek, Matrix.Translation(tilt_c - back * 0.006 + across * s * 0.022) @ parts.frame((0, 0, 0), -flat_back).to_3x3().to_4x4())
        pieces.append(cheek)
    head = Vector((tilt_c.x, tilt_c.y, tilt_c.z)) + flat_back * 0.024
    z_arm = head.z
    head = Vector((head.x, head.y, z_arm))
    pieces.append(parts.rod('neck', tilt_c + flat_back * 0.004, head, 0.009, 16))
    # Pole, collar and the two links.
    pole = parts.rod('pole', (0, 0, 0.009), (0, 0, POLE_H), 0.016, 32)
    cap = parts.disc('cap', (0, 0, POLE_H + 0.003), (0, 0, 1), 0.0175, 0.008, 32)
    collar = parts.disc('collar', (0, 0, z_arm - 0.028), (0, 0, 1), 0.024, 0.022, 32)
    lever = parts.rod('lever', (0.0, 0.0, z_arm - 0.028), (0.034 * side, 0.02, z_arm - 0.028), 0.004, 10)
    elbow = elbow_point(head, side)
    hub0 = parts.disc('hub0', (0, 0, z_arm), (0, 0, 1), 0.024, 0.034, 32)
    hub1 = parts.disc('hub1', elbow, (0, 0, 1), 0.019, 0.036, 32)
    hub2 = parts.disc('hub2', head, (0, 0, 1), 0.016, 0.036, 32)
    l1 = link('link1', Vector((0, 0, z_arm)), elbow)
    l2 = link('link2', elbow, head)
    caps = [parts.disc('pin', p + Vector((0, 0, 0.019)), (0, 0, 1), 0.008, 0.003, 20) for p in (Vector((0, 0, z_arm)), elbow, head)]
    painted = [pole, cap, collar, hub0, hub1, hub2, l1, l2] + pieces + clamp(paint, steel)
    for o in painted:
        if not o.data.materials:
            core.assign(o, paint)
    parts.assign_all(caps + [lever], steel)

    # The cable: out of the panel's back, under the links, down the back of the pole and over the edge.
    start = Vector(center) + back * (HOUSING * 0.7) + up * (-ah * 0.22)
    path = [start, start + back * 0.03 + Vector((0, 0, -0.03)), head + Vector((0, 0, -0.03)) - flat_back * 0.01,
            (head + elbow) / 2 + Vector((0, 0, -0.026)), elbow + Vector((0, 0, -0.027)), elbow / 2 + Vector((0, 0, z_arm / 2 - 0.028)),
            Vector((0, 0.019, z_arm - 0.06)), Vector((0, 0.019, 0.12)), Vector((0, 0.022, 0.03)), Vector((0, EDGE + 0.004, 0.012)),
            Vector((0, EDGE + 0.013, -0.02)), Vector((0, EDGE + 0.015, -0.2))]
    cord = parts.tube('cable', path, 0.0028, 10, 6)
    core.assign(cord, cable)
    return parts.finish(panel + painted + caps + [lever, cord], name, 40)


def build():
    core.reset()
    mats = materials()
    objs = [monitor(n, mats) for n in MESHES]
    # Built side by side for the review; each exports at its own origin.
    return objs


def pose_for_review(objs):
    objs[1].location.x = -0.9
    for o in objs:
        slab = parts.rbox('review_desk', (o.location.x, -0.3, -DESK_T / 2), (0.8, 0.66, DESK_T), 0.0)
        m = core.Mat('review_desk')
        m.set('Base Color', core.hex_linear(0x3b2616))
        m.set('Roughness', 0.45)
        core.assign(slab, m)
