"""SM_MacroPad: GearDrop's KEYSTRIP 15-key macro pad, its LCD keys lit with the stream's actions.

A slim body on a wedge stand (rubber feet, the USB-C cable off the back), tilted toward the player, with
fifteen square LCD keys in a 5 x 3 grid: going live, scenes, the mic and camera, clips and alerts, and
the poker actions the grinder binds to them. The key icons are printed by a Sheet into an emissive
texture, so they glow without a light.

Coordinates (meters), front toward -Y, Z up: origin on the desk under the middle of the stand.
"""
import math

import bpy  # noqa: F401,I001
from mathutils import Matrix, Vector

from artkit import core, parts
from artkit.sheet import Sheet
from artkit.shade import image_surface, object_coords, planar

MESHES = ['SM_MacroPad']
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {'pad_keys': 2048, 'pad_rubber': 128, 'pad_cable': 256}
AO_DISTANCE = 0.004
REVIEW_VIEWS = [('front', -15, 35, 0.45), ('side', 70, 20, 0.45), ('detail', -5, 40, 0.22, (0.0, 0.0, 0.03))]

W, D, T = 0.118, 0.084, 0.021   # the body
KEY, GAP = 0.0145, 0.0045
TILT = 28.0                     # the face leans back toward the player
PIVOT = Vector((0.0, 0.012, 0.012))  # the body's center as built (face up), before it is tilted and lifted
LIFT = 0.026                          # onto the stand
GRID_W = 5 * KEY + 4 * GAP
GRID_D = 3 * KEY + 2 * GAP

# Top row (far from the player) first: (label, fill, ink).
KEYS = [
    [('LIVE', 0xe11d48, 0xffffff), ('BRB', 0xf59e0b, 0x1a1205), ('SCENE', 0x6366f1, 0xffffff), ('CAM', 0x0ea5e9, 0xffffff), ('CLIP', 0xa855f7, 0xffffff)],
    [('MUTE', 0x334155, 0xf87171), ('ALERT', 0xec4899, 0xffffff), ('♠', 0x0f172a, 0xe2e8f0), ('♥', 0x0f172a, 0xf43f5e), ('GG', 0x14b8a6, 0x04201c)],
    [('FOLD', 0x374151, 0xe5e7eb), ('CALL', 0x16a34a, 0xffffff), ('RAISE', 0x0d9488, 0xffffff), ('ALL IN', 0xdc2626, 0xffffff), ('TILT', 0x111827, 0xfacc15)],
]


def key_sheet():
    """The LCD faces, top down in millimeters over the key grid (x right, y away from the player)."""
    w, h = GRID_W * 1000, GRID_D * 1000
    s = Sheet(w, h)
    s.rect(0, 0, w, h, 0x050506, rough=0.3)
    k, g = KEY * 1000, GAP * 1000
    for row, keys in enumerate(KEYS):
        y = h - (row + 1) * k - row * g
        for col, (label, fill, ink) in enumerate(keys):
            x = col * (k + g)
            s.poly(core.rounded_rect(x + 0.9, y + 0.9, x + k - 0.9, y + k - 0.9, 1.6, steps=4), fill, rough=0.2)
            # A soft highlight across the top of each LCD, as the panels show.
            s.poly(core.rounded_rect(x + 0.9, y + k * 0.62, x + k - 0.9, y + k - 0.9, 1.6, steps=4, radii=(0.0, 0.0, 1.6, 1.6)), _blend(fill, 0xffffff, 0.12), rough=0.2)
            cx, cy = x + k / 2, y + k / 2
            if label in ('♠', '♥'):
                # The font has no suits: drawn, two lobes and a point (and a stem for the spade).
                u = k * 0.2
                up = 1 if label == '♥' else -1
                s.circle(cx - u * 0.52, cy + up * u * 0.35, u * 0.56, ink, n=40, rough=0.2)
                s.circle(cx + u * 0.52, cy + up * u * 0.35, u * 0.56, ink, n=40, rough=0.2)
                s.poly([(cx - u * 1.05, cy + up * u * 0.2), (cx + u * 1.05, cy + up * u * 0.2), (cx, cy - up * u * 1.25)], ink, rough=0.2)
                if label == '♠':
                    s.poly([(cx, cy - u * 0.2), (cx + u * 0.42, cy - u * 1.3), (cx - u * 0.42, cy - u * 1.3)], ink, rough=0.2)
                continue
            size = 3.0 if len(label) <= 4 else 2.4
            s.text(label, cx, cy - size * 0.36, size, ink, face='Black', align='CENTER', rough=0.2)
    return s.render('macro_keys', 2048)


def _blend(a, b, t):
    ca = [(a >> s) & 255 for s in (16, 8, 0)]
    cb = [(b >> s) & 255 for s in (16, 8, 0)]
    c = [int(x + (y - x) * t) for x, y in zip(ca, cb)]
    return (c[0] << 16) | (c[1] << 8) | c[2]


def keys_material(printed):
    """The keys: the printed LCD image, emissive; lookups in the body's own (untilted) frame."""
    m = core.Mat('pad_keys')
    tc, _ = object_coords(m)
    rot = m.node('ShaderNodeVectorRotate', _rotation_type='X_AXIS')
    m.link(tc.outputs['Object'], rot.inputs['Vector'])
    rot.inputs['Center'].default_value = PIVOT + Vector((0.0, 0.0, LIFT))  # x and y come out as built
    rot.inputs['Angle'].default_value = math.radians(-TILT)
    sep = m.node('ShaderNodeSeparateXYZ')
    m.link(rot.outputs['Vector'], sep.inputs['Vector'])
    uv = planar(m, sep, 'X', 'Y', -GRID_W / 2, GRID_W / 2, PIVOT.y - GRID_D / 2, PIVOT.y + GRID_D / 2)
    ink, _, _, rough = image_surface(m, printed, uv)
    m.set('Base Color', m.mix(0.7, ink, (0.0, 0.0, 0.0, 1.0)))
    m.set('Emission Color', ink)
    m.set('Emission Strength', 1.0)
    m.set('Roughness', rough)
    return m


def build():
    core.reset()
    printed = key_sheet()
    core.reset()
    body_m = parts.plastic('pad_body', 0x111214, rough=0.4, scuffs=0.25)
    stand_m = parts.plastic('pad_stand', 0x1a1b1e, rough=0.5, scuffs=0.2)
    keys_m = keys_material(printed)
    rubber = parts.rubber('pad_rubber')
    cable_m = parts.rubber('pad_cable', 0x0e0e0e, rough=0.55)

    # The stand: a wedge seen from the side (y, z), extruded across X, its slope just under the tilted body.
    lift = Matrix.Translation((0.0, 0.0, LIFT))
    tilt = Matrix.Translation(PIVOT) @ parts.rot_x(TILT) @ Matrix.Translation(-PIVOT)
    under = lift @ tilt @ (PIVOT - Vector((0.0, 0.0, T / 2)))
    slope = math.tan(math.radians(TILT))

    def z_at(y):
        return under.z + slope * (y - under.y) - 0.0005

    wedge = core.prism_x('stand', [(-0.036, 0.0), (0.042, 0.0), (0.042, z_at(0.042)), (-0.032, z_at(-0.032)), (-0.036, max(z_at(-0.032) - 0.001, 0.001))], -0.05, 0.05)
    core.orient_normals(wedge)
    core.assign(wedge, stand_m)
    feet = [parts.rbox('foot', (sx * 0.04, sy, 0.0008), (0.012, 0.012, 0.0016), 0.003) for sx in (-1, 1) for sy in (-0.03, 0.034)]
    parts.assign_all(feet, rubber)

    # The body in its own frame (centered at PIVOT, face up), tilted back afterwards.
    body = parts.rbox('body', PIVOT, (W, D, T), 0.006, 4)
    core.assign(body, body_m)
    bezel = parts.rbox('bezel', PIVOT + Vector((0, 0, T / 2)), (GRID_W + 0.006, GRID_D + 0.006, 0.0012), 0.003, 3)
    core.assign(bezel, parts.gloss('pad_gloss', 0x060607, rough=0.08))
    caps = []
    for row in range(3):
        for col in range(5):
            x = -GRID_W / 2 + KEY / 2 + col * (KEY + GAP)
            y = PIVOT.y + GRID_D / 2 - KEY / 2 - row * (KEY + GAP)
            caps.append(parts.rbox('key', (x, y, PIVOT.z + T / 2 + 0.0012), (KEY, KEY, 0.0035), 0.0012, 3))
    parts.assign_all(caps, keys_m)
    logo = parts.rbox('logo', PIVOT + Vector((0.0, -D / 2 + 0.006, T / 2 + 0.0002)), (0.02, 0.003, 0.0005), 0.0002)
    core.assign(logo, parts.metal('pad_logo', 0x9a9da3, rough=0.25))
    face = [body, bezel, logo] + caps
    parts.xform(face, lift @ tilt)
    port = lift @ tilt @ (PIVOT + Vector((0.0, D / 2, 0.0)))
    cord = parts.tube('cable', [port, port + Vector((0.0, 0.015, -0.004)), (0.0, 0.07, 0.008), (0.006, 0.1, 0.0022), (0.03, 0.22, 0.0022)], 0.0018, 8, 8)
    core.assign(cord, cable_m)
    return [parts.finish([wedge] + feet + face + [cord], 'SM_MacroPad', 40)]
