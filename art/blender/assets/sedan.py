"""SM_Sedan: the parked cars along Fifth Street's far curb (one mesh; the game paints each a color).

A mid-size four-door: a lofted body (bumpers, hood, beltline, trunk), the greenhouse with its pillars and
glass, wheel arches cut and lined, 17" alloys on tires, headlights and the grille, tail lights, mirrors,
door handles and shut lines, Riverside plates front and back. The paint is metallic light grey under clear
coat with road film along the sills: AStreetStage tints its first material per car.

Coordinates (meters), front toward -Y, Z up: origin on the road under the middle of the car.
"""
import math

import bpy  # noqa: F401,I001
from mathutils import Matrix, Vector

from artkit import core, parts, street
from artkit.shade import image_surface, mix_float, object_coords, planar, smooth_less
from artkit.sheet import Sheet

MESHES = ['SM_Sedan']
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {'car_paint': 2048, 'car_glass': 1024, 'car_plate': 512, 'car_chrome': 256, 'car_red': 256, 'car_amber': 128, 'car_head': 512}
AO_DISTANCE = 0.15
REVIEW_VIEWS = [('front', -35, 12, 2.4), ('rear', 145, 15, 2.4), ('wheel', -75, 5, 0.75, (-0.9, -1.45, 0.4))]

WHEEL_R = 0.33
WHEEL_W = 0.215
TRACK = 0.79      # wheel center from the middle
AXLES = (-1.42, 1.38)
BELT = 0.95
ROOF = 1.44

# Lower body stations: y, half width, bottom, top.
BODY = [(-2.39, 0.66, 0.36, 0.6), (-2.36, 0.8, 0.26, 0.7), (-2.25, 0.87, 0.2, 0.78), (-2.0, 0.895, 0.18, 0.83), (-1.6, 0.9, 0.17, 0.865),
        (-1.2, 0.905, 0.17, 0.895), (-0.85, 0.91, 0.17, 0.925), (-0.4, 0.912, 0.17, BELT), (0.3, 0.912, 0.17, BELT + 0.01),
        (1.0, 0.91, 0.17, BELT + 0.02), (1.45, 0.905, 0.18, BELT + 0.035), (1.8, 0.895, 0.2, 1.0), (2.1, 0.88, 0.24, 1.0),
        (2.3, 0.84, 0.3, 0.97), (2.37, 0.76, 0.36, 0.9), (2.39, 0.64, 0.42, 0.8)]
# Greenhouse stations: y, roof height (the windshield climbs from the cowl, the backlight drops to the deck).
CABIN = [(-0.86, BELT - 0.02), (-0.7, 1.06), (-0.5, 1.2), (-0.3, 1.32), (-0.1, 1.405), (0.1, ROOF), (0.35, ROOF), (0.8, 1.435), (1.05, 1.41),
         (1.25, 1.33), (1.45, 1.21), (1.65, 1.09), (1.8, BELT + 0.04)]
PILLARS = {'a': (-0.86, -0.22), 'b': (0.3, 0.42), 'c': (1.02, 1.8)}
DOOR_SEAMS = (-0.84, 0.36, 1.12)


def section(hw, zb, zt, n=32):
    """A body section: flat-ish sides that tuck in toward the top (tumblehome) and under at the sill."""
    pts = []
    zc = (zb + zt) / 2
    hh = (zt - zb) / 2
    for k in range(n):
        t = 2 * math.pi * k / n
        c, s = math.cos(t), math.sin(t)
        x = hw * math.copysign(abs(c) ** 0.22, c)
        z = zc + hh * math.copysign(abs(s) ** 0.45, s)
        x *= 1.0 - 0.07 * max(0.0, s) ** 2 - 0.05 * max(0.0, -s) ** 3
        pts.append((x, z))
    return pts


def plate_sheet():
    s = Sheet(300, 150)
    s.rect(0, 0, 300, 150, 0xf2f1ec, rough=0.3)
    s.rect(4, 4, 292, 142, 0x1e3a6e, rough=0.3)
    s.rect(8, 8, 284, 134, 0xf2f1ec, rough=0.3)
    s.text('RIVERSIDE', 150, 118, 22, 0xb5262f, face='Black', align='CENTER', tracking=1.3, rough=0.3)
    s.text('7FTH 212', 150, 40, 62, 0x1e3a6e, face='Black', align='CENTER', rough=0.3)
    s.text('THE RIVER CITY', 150, 14, 13, 0x1e3a6e, face='Bold', align='CENTER', tracking=1.3, rough=0.3)
    return s.render('car_plate', 1024)


def paint_material():
    """Car paint with the shut lines of the doors, hood and trunk cut into it."""
    m = street.car_paint('car_paint', 0xd8dadc, rough=0.16, flake=0.3, dirt=0.4)
    tc, sep = object_coords(m)
    y, z = sep.outputs['Y'], sep.outputs['Z']
    side = m.math('GREATER_THAN', m.math('ABSOLUTE', sep.outputs['X']), 0.8)
    lines = None
    for y0 in DOOR_SEAMS:
        l = m.math('MULTIPLY', smooth_less(m, m.math('ABSOLUTE', m.math('SUBTRACT', y, y0)), 0.002, 0.0008), side)
        lines = l if lines is None else m.math('MAXIMUM', lines, l)
    # The hood's and trunk's shut lines run across the top.
    top = m.math('GREATER_THAN', z, 0.75)
    for y0 in (-0.88, 1.72):
        l = m.math('MULTIPLY', smooth_less(m, m.math('ABSOLUTE', m.math('SUBTRACT', y, y0)), 0.002, 0.0008), top)
        lines = m.math('MAXIMUM', lines, l)
    # Doors stop above the sill.
    lines = m.math('MULTIPLY', lines, m.math('GREATER_THAN', z, 0.26))
    base = m.bsdf.inputs['Base Color'].links[0].from_socket
    m.set('Base Color', m.mix(lines, base, core.hex_linear(0x0c0c0d)))
    bump = m.node('ShaderNodeBump', Strength=0.6, Distance=0.002)
    m.link(m.math('SUBTRACT', 1.0, lines), bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def plate_material(img):
    m = core.Mat('car_plate')
    tc, sep = object_coords(m)
    # Front plate faces -Y (read along +X), the rear +Y (read along -X): mirror u by the side of the car.
    u = m.math('ADD', m.math('MULTIPLY', m.math('DIVIDE', sep.outputs['X'], 0.3), m.math('SIGN', m.math('MULTIPLY', sep.outputs['Y'], -1.0))), 0.5)
    from artkit.shade import vec
    zc = m.math('SUBTRACT', sep.outputs['Z'], m.math('ADD', 0.42, m.math('MULTIPLY', m.math('GREATER_THAN', sep.outputs['Y'], 0.0), 0.2)))
    v = m.math('ADD', m.math('DIVIDE', zc, 0.15), 0.5)
    ink, alpha, metal, rough = image_surface(m, img, vec(m, u, v, 0.0))
    m.set('Base Color', ink)
    m.set('Roughness', rough)
    m.set('Metallic', 0.3)
    return m


def wheel(cx, cy, tire_m, rim_m, brake_m):
    side = 1 if cx > 0 else -1
    out = []
    # Tire: revolved round the axle (built round Z, turned onto X).
    r0, r1, w = 0.218, WHEEL_R, WHEEL_W
    prof = [(r0, -w / 2 + 0.01), (r0 + 0.02, -w / 2), (r1 - 0.03, -w / 2 - 0.004), (r1 - 0.008, -w / 2 + 0.012), (r1, -w / 2 + 0.035),
            (r1, w / 2 - 0.035), (r1 - 0.008, w / 2 - 0.012), (r1 - 0.03, w / 2 + 0.004), (r0 + 0.02, w / 2), (r0, w / 2 - 0.01)]
    to_axle = Matrix.Translation((cx, cy, WHEEL_R)) @ Matrix.Rotation(math.radians(90), 4, 'Y')
    out.append((parts.lathe_at('tire', prof, to_axle, 64), tire_m))
    # The rim: the barrel and the dished face, five twin spokes, the center cap.
    face = side * (w / 2 - 0.03)
    barrel = [(0.215, -w / 2 + 0.012), (0.21, -w / 2 + 0.02), (0.205, w / 2 - 0.02), (0.215, w / 2 - 0.012)]
    out.append((parts.lathe_at('barrel', barrel, to_axle, 48), rim_m))
    lip = parts.annulus('lip', 0.19, 0.222, -0.006, 0.006, 64)
    lip.data.transform(Matrix.Translation((cx + face, cy, WHEEL_R)) @ Matrix.Rotation(math.radians(90), 4, 'Y'))
    out.append((lip, rim_m))
    hub = parts.disc('hub', (cx + face - side * 0.02, cy, WHEEL_R), (1, 0, 0), 0.065, 0.04, 32)
    out.append((hub, rim_m))
    out.append((parts.disc('cap', (cx + face + side * 0.002, cy, WHEEL_R), (1, 0, 0), 0.03, 0.01, 24), brake_m))
    for k in range(5):
        a = 2 * math.pi * k / 5
        for off in (-0.1, 0.1):
            aa = a + off
            d = Vector((0.0, math.cos(aa), math.sin(aa)))
            p0 = Vector((cx + face - side * 0.022, cy, WHEEL_R)) + d * 0.06
            p1 = Vector((cx + face, cy, WHEEL_R)) + d * 0.195
            out.append((parts.rod('spoke', p0, p1, 0.011, 8), rim_m))
    # The brake disc and caliper behind the spokes.
    out.append((parts.disc('rotor', (cx + side * 0.02, cy, WHEEL_R), (1, 0, 0), 0.16, 0.025, 48), brake_m))
    caliper = parts.rbox('caliper', (cx + side * 0.04, cy - 0.12, WHEEL_R + 0.06), (0.04, 0.09, 0.08), 0.015)
    out.append((caliper, brake_m))
    return out


def build():
    core.reset()
    paint = paint_material()
    glass_m = street.glass('car_glass', 0x0b0e12, rough=0.03)
    trim = street.shop_plastic('car_trim', 0x0e0f10, rough=0.55, grime=0.4)
    chrome = street.bare_metal('car_chrome', 0xc9cdd1, rough=0.12, grime=0.2)
    tire_m = street.tire('car_tire')
    rim_m = street.bare_metal('car_rim', 0xa9adb2, rough=0.28, grime=0.6)
    brake_m = street.bare_metal('car_brake', 0x55585c, rough=0.6, grime=0.8)
    head_m = street.lens('car_head', 0xd8dde2, rough=0.05)
    red = street.lens('car_red', 0x9b0f14, rough=0.08)
    amber = street.lens('car_amber', 0xd9821e, rough=0.1)
    plate_m = plate_material(plate_sheet())
    out = []
    # The body, arches cut for the wheels.
    rings = [[Vector((x, y, z)) for x, z in section(hw, zb, zt)] for y, hw, zb, zt in BODY]
    body = core.loft('body', rings)
    for ay in AXLES:
        cut = parts.rod('arch_cut', (-1.2, ay, WHEEL_R + 0.02), (1.2, ay, WHEEL_R + 0.02), WHEEL_R + 0.055, 48)
        core.boolean(body, cut)
    out.append((body, paint))
    # Arch liners: open tubes round the wheels, seen through the cuts.
    for ay in AXLES:
        for sx in (-1, 1):
            liner = parts.lathe_at('liner', [(WHEEL_R + 0.05, 0.58), (WHEEL_R + 0.05, 0.9), (WHEEL_R + 0.065, 0.9), (WHEEL_R + 0.065, 0.58), (WHEEL_R + 0.05, 0.58)],
                                   Matrix.Translation((0, ay, WHEEL_R + 0.02)) @ Matrix.Rotation(math.radians(90 * sx), 4, 'Y'), 40)
            # Only the arch over the wheel: nothing below the sills.
            import bmesh
            bm = bmesh.new()
            bm.from_mesh(liner.data)
            bmesh.ops.delete(bm, geom=[f for f in bm.faces if f.calc_center_median().z < 0.22], context='FACES')
            bm.to_mesh(liner.data)
            bm.free()
            out.append((liner, trim))
    # The greenhouse: a trapezoid section lofted from the cowl to the deck.
    cab_rings = []
    for y, roof in CABIN:
        h = max(roof - (BELT - 0.03), 0.01)
        f = h / (ROOF - BELT + 0.03)
        bw, tw = 0.86, 0.86 - 0.17 * f
        zb = BELT - 0.03
        ring = [(bw, zb), (bw - 0.03 * f, zb + h * 0.45), (tw + 0.02 * f, zb + h * 0.9), (tw, zb + h), (-tw, zb + h), (-tw - 0.02 * f, zb + h * 0.9),
                (-bw + 0.03 * f, zb + h * 0.45), (-bw, zb)]
        cab_rings.append([Vector((x, y, z)) for x, z in ring])
    cabin = core.loft('cabin', cab_rings)
    cabin.data.materials.append(paint.m)
    cabin.data.materials.append(glass_m.m)
    cabin.data.materials.append(trim.m)
    for poly in cabin.data.polygons:
        c = poly.center
        n = poly.normal
        if n.z > 0.75 and abs(c.x) < 0.66 and -0.12 < c.y < 0.95:
            poly.material_index = 0  # the roof
        elif abs(n.x) > 0.6 and abs(n.z) < 0.75:
            in_pillar = any(a <= c.y <= b for a, b in PILLARS.values())
            poly.material_index = 2 if in_pillar else 1
        else:
            poly.material_index = 1  # windshield and backlight
    out.append((cabin, None))
    # Front: the grille, headlights, the plate; lower intake.
    out.append((parts.rbox('grille', (0, -2.375, 0.6), (0.9, 0.03, 0.14), 0.02), trim))
    for k in range(5):
        out.append((parts.rbox('grille_bar', (0, -2.393, 0.55 + k * 0.025), (0.86, 0.006, 0.006), 0.002), chrome))
    out.append((parts.rbox('intake', (0, -2.37, 0.34), (1.1, 0.03, 0.1), 0.02), trim))
    for sx in (-1, 1):
        hl = parts.rbox('headlight', (sx * 0.56, -2.352, 0.64), (0.3, 0.06, 0.085), 0.022, rot=Matrix.Rotation(math.radians(sx * 8), 3, 'Z'))
        out.append((hl, head_m))
        out.append((parts.rbox('marker', (sx * 0.84, -2.2, 0.62), (0.06, 0.12, 0.04), 0.01), amber))
        tl = parts.rbox('taillight', (sx * 0.62, 2.335, 0.86), (0.34, 0.06, 0.11), 0.02, rot=Matrix.Rotation(math.radians(-sx * 10), 3, 'Z'))
        out.append((tl, red))
        # Mirrors on stalks at the A pillars, door handles.
        out.append((parts.rbox('mirror_stalk', (sx * 0.92, -0.76, BELT + 0.03), (0.06, 0.06, 0.03), 0.01), trim))
        out.append((parts.rbox('mirror', (sx * 0.99, -0.77, BELT + 0.07), (0.11, 0.06, 0.09), 0.025), paint))
        for hy in (-0.25, 0.85):
            out.append((parts.rbox('handle', (sx * 0.905, hy, BELT - 0.08), (0.02, 0.14, 0.025), 0.008), chrome))
        # Sill trim.
        out.append((parts.rbox('sill', (sx * 0.86, (AXLES[0] + AXLES[1]) / 2, 0.2), (0.06, AXLES[1] - AXLES[0] - 0.85, 0.06), 0.015), trim))
    out.append((parts.rbox('plate_f', (0, -2.398, 0.42), (0.3, 0.008, 0.15), 0.004), plate_m))
    out.append((parts.rbox('plate_r', (0, 2.398, 0.62), (0.3, 0.008, 0.15), 0.004), plate_m))
    out.append((parts.rbox('rear_bumper_trim', (0, 2.385, 0.38), (1.3, 0.02, 0.06), 0.01), trim))
    # Wheels.
    for ay in AXLES:
        for sx in (-1, 1):
            out += wheel(sx * TRACK, ay, tire_m, rim_m, brake_m)
    for o, m in out:
        if m is not None:
            core.assign(o, m)
    # The body first: its paint is the material the game tints.
    return [parts.finish([o for o, _ in out], 'SM_Sedan', 35)]
