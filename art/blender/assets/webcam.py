"""SM_Webcam, SM_WebcamPro and SM_Mirrorless: GearDrop's HALO cameras.

- SM_Webcam: the 720p webcam, a small rounded body clipped on the laptop lid's top edge.
- SM_WebcamPro: the 1080p60 webcam, a wider bar with a glass front, IR windows and a status light.
- SM_Mirrorless: the mirrorless camera (body, kit lens, hot shoe, grip) on a mini tripod with a ball head,
  standing behind the laptop and aimed at the chair, its HDMI cable trailing off the back.

Coordinates (meters), front toward -Y, Z up. The webcams use the laptop lid's frame (art/blender/assets/
laptop.py): origin on the hinge axis, the lid upright with its top edge at LID_TOP, so the game hangs
them on the lid and they tilt with it. SM_Mirrorless: origin on the desk under the tripod.
"""
import math

import bpy  # noqa: F401,I001
from mathutils import Matrix, Vector

from artkit import core, parts
from artkit.shade import object_coords, powder_coat

MESHES = ['SM_Webcam', 'SM_WebcamPro', 'SM_Mirrorless']
DOUBLE_SIDED = False
TEXTURE_SIZE = 512
TEXTURE_SIZES = {'cam_body': 1024, 'cam_grip': 1024, 'cam_led': 64}
AO_DISTANCE = 0.006
REVIEW_VIEWS = [('front', -25, 10, 0.9), ('back', 150, 25, 0.9), ('detail', -20, 8, 0.35, (0.0, 0.0, 0.2))]

LID_TOP = 0.2318
LID_FRONT = -0.0026
LID_BACK = 0.0040
TILT = 5.0  # the lid leans back 18.3 degrees; the camera tips forward a little on its clip to frame the face


# ------------------------------------------------------------------ materials

def materials():
    m = {'body': parts.plastic('cam_body', 0x141518, rough=0.42, scuffs=0.25), 'gloss': parts.gloss('cam_gloss', 0x060607, rough=0.06),
         'glass': parts.gloss('cam_glass', 0x0a0f1a, rough=0.03), 'ring': parts.metal('cam_ring', 0x8d9096, rough=0.28, brushed_axis='Z'),
         'rubber': parts.rubber('cam_rubber'), 'led': parts.emissive('cam_led', 0xffffff, 1.0, base_hex=0xdddddd),
         'cable': parts.rubber('cam_cable', 0x0e0e0e, rough=0.55)}
    grip = core.Mat('cam_grip')
    tc, _ = object_coords(grip)
    color, rough, metal, bump = powder_coat(grip, tc, 0x1b1c1f, rough=0.55, chips=0.2)
    grip.set('Base Color', color)
    grip.set('Roughness', rough)
    grip.set('Metallic', metal)
    grip.set('Normal', bump.outputs['Normal'])
    m['grip'] = grip
    m['tripod'] = parts.metal('cam_tripod', 0x2a2b2f, rough=0.35, anodized=True)
    return m


def lens(name, center, r, depth, mats, rings=3):
    """A lens looking along -Y from center: a stepped barrel, then the glass, slightly domed."""
    prof = [(0.0, 0.0), (r, 0.0)]
    for k in range(rings):
        rr = r * (1.0 - 0.08 * k)
        z = depth * (k + 1) / rings
        prof += [(rr, z - depth / rings * 0.25), (rr * 0.96, z)]
    prof += [(r * 0.62, depth), (0.0, depth)]
    barrel = core.lathe(name, prof, segments=48)
    core.orient_normals(barrel)
    barrel.data.transform(Matrix.Translation(center) @ parts.rot_x(90))
    core.assign(barrel, mats['ring'] if rings > 1 else mats['gloss'])
    glass = core.lathe(name + '_glass', [(0.0, depth + 0.0012), (r * 0.6, depth), (r * 0.6, depth - 0.001), (0.0, depth - 0.001)], segments=48)
    core.orient_normals(glass)
    glass.data.transform(Matrix.Translation(center) @ parts.rot_x(90))
    core.assign(glass, mats['glass'])
    return [barrel, glass]


def clip(mats, width):
    """The hinged clip over the lid's top edge: a lip in front of the bezel, a leg down the lid's back."""
    lip = parts.rbox('lip', (0.0, LID_FRONT - 0.002, LID_TOP - 0.004), (width * 0.5, 0.003, 0.012), 0.001)
    leg = parts.rbox('leg', (0.0, LID_BACK + 0.0025, LID_TOP - 0.03), (width * 0.55, 0.004, 0.064), 0.0015)
    seat = parts.rbox('seat', (0.0, (LID_FRONT + LID_BACK) / 2, LID_TOP + 0.002), (width * 0.55, 0.012, 0.004), 0.0012)
    hinge = parts.disc('hinge', (0.0, LID_BACK + 0.003, LID_TOP + 0.002), (1, 0, 0), 0.003, width * 0.5, 16)
    pad = parts.rbox('pad', (0.0, LID_BACK + 0.0005, LID_TOP - 0.055), (width * 0.5, 0.001, 0.01), 0.0004)
    out = [lip, leg, seat, hinge]
    parts.assign_all(out, mats['body'])
    core.assign(pad, mats['rubber'])
    return out + [pad]


def webcam(mats):
    w, d, h = 0.072, 0.026, 0.03
    base_z = LID_TOP + 0.004
    c = Vector((0.0, 0.0, base_z + h / 2))
    body = parts.rbox('body', c, (w, d, h), 0.011, 6)
    core.assign(body, mats['body'])
    face = parts.rbox('face', c + Vector((0, -d / 2 + 0.0004, 0)), (w - 0.012, 0.002, h - 0.01), 0.004, 4)
    core.assign(face, mats['gloss'])
    cam = lens('lens', c + Vector((0.0, -d / 2 - 0.0005, 0.0)), 0.0085, 0.004, mats, rings=1)
    led = parts.disc('led', c + Vector((0.022, -d / 2 - 0.0002, 0.0)), (0, 1, 0), 0.0012, 0.0012, 12)
    core.assign(led, mats['led'])
    mic = [parts.disc('mic', c + Vector((s * 0.03, -d / 2 + 0.0002, 0.0)), (0, 1, 0), 0.0011, 0.001, 10) for s in (-1, 1)]
    parts.assign_all(mic, mats['rubber'])
    head = [body, face, led] + cam + mic
    parts.xform(head, Matrix.Translation((0, 0, base_z)) @ parts.rot_x(-TILT) @ Matrix.Translation((0, 0, -base_z)))
    cord = parts.tube('cable', [c + Vector((0.0, d / 2, -0.004)), (0.0, LID_BACK + 0.014, LID_TOP - 0.004), (0.004, LID_BACK + 0.012, LID_TOP - 0.05),
                                (0.02, LID_BACK + 0.008, LID_TOP - 0.13), (0.03, LID_BACK + 0.006, 0.02)], 0.0016, 8, 8)
    core.assign(cord, mats['cable'])
    return parts.finish(head + clip(mats, 0.05) + [cord], 'SM_Webcam', 40)


def webcam_pro(mats):
    w, d, h = 0.102, 0.027, 0.027
    base_z = LID_TOP + 0.004
    c = Vector((0.0, 0.0, base_z + h / 2))
    body = parts.rbox('body', c, (w, d, h), 0.0105, 6)
    core.assign(body, mats['body'])
    face = parts.rbox('face', c + Vector((0, -d / 2 + 0.0006, 0)), (w - 0.006, 0.002, h - 0.005), 0.009, 5)
    core.assign(face, mats['gloss'])
    cam = lens('lens', c + Vector((0.0, -d / 2 - 0.0004, 0.0)), 0.0075, 0.0025, mats, rings=1)
    ir = [parts.disc('ir', c + Vector((s * 0.024, -d / 2 - 0.0002, 0.0)), (0, 1, 0), 0.0035, 0.0012, 20) for s in (-1, 1)]
    parts.assign_all(ir, mats['glass'])
    led = parts.disc('led', c + Vector((-0.0125, -d / 2 - 0.0002, 0.0)), (0, 1, 0), 0.0011, 0.0012, 12)
    core.assign(led, mats['led'])
    head = [body, face, led] + cam + ir
    parts.xform(head, Matrix.Translation((0, 0, base_z)) @ parts.rot_x(-TILT) @ Matrix.Translation((0, 0, -base_z)))
    cord = parts.tube('cable', [c + Vector((0.03, d / 2, -0.004)), (0.03, LID_BACK + 0.014, LID_TOP - 0.004), (0.034, LID_BACK + 0.012, LID_TOP - 0.05),
                                (0.045, LID_BACK + 0.008, LID_TOP - 0.13), (0.05, LID_BACK + 0.006, 0.02)], 0.0018, 8, 8)
    core.assign(cord, mats['cable'])
    return parts.finish(head + clip(mats, 0.06) + [cord], 'SM_WebcamPro', 40)


def mirrorless(mats):
    out = []
    # Mini tripod: three splayed legs with rubber feet, a center column, a ball head and a clamp plate.
    hub = Vector((0.0, 0.0, 0.17))
    for k in range(3):
        a = math.radians(90 + 120 * k)
        foot = Vector((0.11 * math.cos(a), 0.11 * math.sin(a), 0.006))
        out.append((parts.rod('leg', hub + Vector((0, 0, -0.006)), foot + Vector((0, 0, 0.004)), 0.0055, 14), mats['tripod']))
        out.append((parts.rbox('foot', foot, (0.014, 0.014, 0.012), 0.005, 4), mats['rubber']))
        out.append((parts.rbox('lock', hub + (foot - hub) * 0.45, (0.012, 0.012, 0.012), 0.003, 3), mats['body']))
    out.append((parts.disc('crown', hub, (0, 0, 1), 0.017, 0.022, 32), mats['tripod']))
    out.append((parts.rod('column', hub, hub + Vector((0, 0, 0.06)), 0.008, 20), mats['tripod']))
    ball_c = hub + Vector((0, 0, 0.078))
    ball = core.lathe('ball', [(0.0, -0.015)] + [(0.015 * math.sin(math.radians(a)), -0.015 * math.cos(math.radians(a))) for a in range(15, 180, 15)] + [(0.0, 0.015)], segments=32)
    core.orient_normals(ball)
    ball.data.transform(Matrix.Translation(ball_c))
    out.append((ball, mats['ring']))
    out.append((parts.disc('socket', ball_c - Vector((0, 0, 0.012)), (0, 0, 1), 0.017, 0.014, 32), mats['tripod']))
    out.append((parts.rod('ball_knob', ball_c - Vector((0, 0, 0.01)), ball_c + Vector((0.032, 0, -0.012)), 0.003, 10), mats['tripod']))
    out.append((parts.disc('ball_knob_cap', ball_c + Vector((0.034, 0, -0.012)), (1, 0, 0), 0.007, 0.008, 16), mats['body']))
    # The camera, tipped up a little toward the face it frames.
    cam = []
    w, d, h = 0.12, 0.042, 0.066
    plate = parts.rbox('plate', (0.0, 0.0, 0.0045), (0.04, 0.04, 0.009), 0.002)
    body = parts.rbox('cam_body', (0.0, 0.0, 0.009 + h / 2), (w, d, h), 0.008, 4)
    grip = parts.rbox('cam_grip', (-w / 2 + 0.017, -d / 2 - 0.009, 0.009 + h / 2 - 0.004), (0.032, 0.022, h - 0.01), 0.009, 4)
    finder = parts.rbox('finder', (-0.03, 0.006, 0.009 + h + 0.007), (0.034, 0.03, 0.016), 0.004, 3)
    shoe = parts.rbox('shoe', (0.016, 0.004, 0.009 + h + 0.002), (0.02, 0.018, 0.004), 0.001)
    dials = [parts.disc('dial', (x, 0.004, 0.009 + h + 0.004), (0, 0, 1), 0.009, 0.007, 24) for x in (0.036, 0.008)]
    mount = parts.disc('mount', (0.012, -d / 2 - 0.002, 0.009 + h / 2), (0, 1, 0), 0.026, 0.006, 48)
    screen = parts.rbox('lcd', (0.008, d / 2 + 0.002, 0.009 + h / 2), (0.075, 0.004, 0.05), 0.002)
    cam.append((plate, mats['tripod']))
    for o in (body, finder, shoe):
        cam.append((o, mats['grip']))
    cam.append((grip, mats['rubber']))
    for o in dials:
        cam.append((o, mats['ring']))
    cam.append((mount, mats['ring']))
    cam.append((screen, mats['gloss']))
    for o in lens('kit', Vector((0.012, -d / 2 - 0.004, 0.009 + h / 2)), 0.031, 0.05, mats, rings=3):
        cam.append((o, None))
    tally = parts.disc('tally', (-0.045, -d / 2 - 0.0004, 0.009 + h - 0.008), (0, 1, 0), 0.0016, 0.0012, 12)
    cam.append((tally, mats['led']))
    objs = []
    for o, mat in cam:
        if mat is not None:
            core.assign(o, mat)
        objs.append(o)
    top = ball_c + Vector((0, 0, 0.015))
    parts.xform(objs, Matrix.Translation(top) @ parts.rot_x(6))
    cord = parts.tube('hdmi', [top + Vector((-0.06, 0.004, 0.05)), top + Vector((-0.075, 0.03, 0.03)), (-0.07, 0.08, 0.1), (-0.05, 0.14, 0.004), (-0.02, 0.3, 0.004)], 0.0026, 10, 8)
    core.assign(cord, mats['cable'])
    for o, mat in out:
        core.assign(o, mat)
    return parts.finish([o for o, _ in out] + objs + [cord], 'SM_Mirrorless', 40)


def build():
    core.reset()
    mats = materials()
    return [webcam(mats), webcam_pro(mats), mirrorless(mats)]


def pose_for_review(objs):
    """The two webcams on stand-in lid edges, beside the tripod."""
    for i, o in enumerate(objs[:2]):
        o.location = (-0.32 + i * 0.16, 0.0, -LID_TOP + 0.1)
        lid = parts.rbox('review_lid', (o.location.x, (LID_FRONT + LID_BACK) / 2, 0.1 - 0.06), (0.14, LID_BACK - LID_FRONT, 0.12), 0.001)
        m = core.Mat('review_lid')
        m.set('Base Color', core.hex_linear(0x3a3c40))
        m.set('Metallic', 1.0)
        m.set('Roughness', 0.35)
        core.assign(lid, m)
