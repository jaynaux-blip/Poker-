"""SM_Register: the Lucky Penny's point-of-sale on the counter.

A cash drawer base, the clerk's touchscreen on a tilting stand (the sale on it), a receipt printer
beside it, a pole display facing the customer with the total, a PIN pad on a swivel arm on the customer's
side and a handheld scanner in its cradle. Shop plastics in charcoal, worn by a thousand shifts.

Coordinates (meters), front toward -Y (the clerk's side), Z up: origin on the counter under the drawer's
middle; the customer stands on the +Y side.
"""
import math

import bpy  # noqa: F401,I001
from mathutils import Matrix, Vector

from artkit import core, parts, street
from artkit.shade import image_surface, object_coords, planar
from artkit.sheet import Sheet

MESHES = ['SM_Register']
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {'pos_screen': 1024, 'pos_pole': 256, 'pos_keys': 256, 'pos_paper': 128, 'pos_metal': 256}
AO_DISTANCE = 0.02
REVIEW_VIEWS = [('clerk', -30, 22, 2.3), ('customer', 150, 20, 2.3), ('screen', -8, 18, 0.9, (0.0, -0.04, 0.27))]

SCREEN = (0.34, 0.215)  # display area of a 15.6" panel
TILT = 25.0
SCREEN_C = Vector((0.0, -0.02, 0.27))
POLE = (0.16, 0.05)


def screen_sheet():
    w, h = SCREEN[0] * 1000, SCREEN[1] * 1000
    s = Sheet(w, h)
    s.rect(0, 0, w, h, 0x10131a, rough=0.2)
    s.rect(0, h - 22, w, 22, 0xd7263d, rough=0.2)
    s.text('LUCKY PENNY #212', 10, h - 15, 10, 0xffffff, face='Black', rough=0.2)
    s.text('REG 1 · BENNY · 11:42 PM', w - 10, h - 15, 7, 0xffe4e8, face='Bold', align='RIGHT', rough=0.2)
    # The sale on the left, the item buttons on the right.
    s.rect(6, 30, 140, h - 58, 0x1b2030, rough=0.2)
    items = [('FIZZ COLA 20OZ', '1.99'), ('HILLTOP CHIPS', '1.79'), ('ROLLER DOG', '1.99'), ('DRIP COFFEE 16', '1.59')]
    for k, (name, price) in enumerate(items):
        y = h - 42 - k * 15
        s.text(name, 12, y, 6.5, 0xd9dde8, face='Bold', rough=0.2)
        s.text(price, 140, y, 6.5, 0xd9dde8, face='Bold', align='RIGHT', rough=0.2)
    s.rect(12, 52, 128, 0.8, 0x4a5370, rough=0.2)
    s.text('TOTAL', 12, 38, 9, 0xffffff, face='Black', rough=0.2)
    s.text('$7.86', 140, 38, 11, 0x7ee08a, face='Black', align='RIGHT', rough=0.2)
    s.rect(6, 6, 140, 20, 0x2e9e4f, rough=0.2)
    s.text('PAY', 76, 12, 9, 0xffffff, face='Black', align='CENTER', rough=0.2)
    colors = [0x3a6fd8, 0xd8803a, 0x3aa6a0, 0x8a4fd0, 0xc9a23a, 0x4f8f3a, 0xd04f6a, 0x5a6478, 0x2f86c9]
    labels = ['DRINKS', 'SNACKS', 'HOT', 'COFFEE', 'LOTTO', 'TOBACCO', 'VOID', 'NO SALE', 'PRICE']
    for k in range(9):
        cx = 156 + (k % 3) * 60
        cy = h - 62 - (k // 3) * 46
        s.rect(cx, cy, 54, 40, colors[k], rough=0.2)
        s.text(labels[k], cx + 27, cy + 16, 7, 0xffffff, face='Black', align='CENTER', rough=0.2)
    return s.render('pos_screen', 1024)


def pole_sheet():
    w, h = POLE[0] * 1000, POLE[1] * 1000
    s = Sheet(w, h)
    s.rect(0, 0, w, h, 0x05140a, rough=0.2)
    s.text('TOTAL  $7.86', w / 2, h * 0.52, 15, 0x5dff8a, face='Bold', align='CENTER', rough=0.2)
    s.text('THANK YOU · COME AGAIN', w / 2, h * 0.14, 7.5, 0x3fbf66, face='Bold', align='CENTER', rough=0.2)
    return s.render('pos_pole', 512)


def screen_material(name, image, x0, x1, z0, z1, glow, center=None, tilt=0.0):
    """A lit display: its picture mapped in the screen's own XZ (untilted about center by tilt degrees)."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    if center is not None:
        rot = m.node('ShaderNodeVectorRotate')
        rot.rotation_type = 'X_AXIS'
        rot.inputs['Center'].default_value = tuple(center)
        rot.inputs['Angle'].default_value = math.radians(-tilt)
        m.link(tc.outputs['Object'], rot.inputs['Vector'])
        sep = m.node('ShaderNodeSeparateXYZ')
        m.link(rot.outputs['Vector'], sep.inputs['Vector'])
        z0, z1 = z0 + center[2], z1 + center[2]
        x0, x1 = x0 + center[0], x1 + center[0]
    uv = planar(m, sep, 'X', 'Z', x0, x1, z0, z1)
    ink, alpha, metal, rough = image_surface(m, image, uv)
    m.set('Base Color', m.mix(0.85, ink, core.hex_linear(0x050608)))
    m.link(ink, m.bsdf.inputs['Emission Color'])
    m.set('Emission Strength', glow)
    m.set('Roughness', 0.12)
    return m


def build():
    core.reset()
    body = street.shop_plastic('pos_body', 0x222428, rough=0.45, grime=0.5)
    trim = street.shop_plastic('pos_trim', 0x111214, rough=0.35, grime=0.3)
    metal = street.bare_metal('pos_metal', 0x8e9296, rough=0.3, grime=0.4)
    keys = street.shop_plastic('pos_keys', 0x3a3d42, rough=0.5, grime=0.6)
    paper = street.shop_plastic('pos_paper', 0xf3f0e8, rough=0.8, grime=0.0)
    out = []
    # The cash drawer: a steel box with a plastic front, the key lock.
    out.append((parts.rbox('drawer', (0, 0, 0.05), (0.41, 0.42, 0.1), 0.008), metal))
    out.append((parts.rbox('drawer_front', (0, -0.212, 0.05), (0.39, 0.008, 0.085), 0.004), body))
    out.append((parts.disc('lock', (0.15, -0.218, 0.07), (0, 1, 0), 0.008, 0.006, 16), metal))
    out.append((parts.rbox('drawer_lip', (0, -0.218, 0.03), (0.16, 0.012, 0.012), 0.004), trim))
    # The screen stand: a foot, a neck, the tilting head with the panel.
    out.append((parts.rbox('stand_foot', (0, 0.02, 0.108), (0.22, 0.2, 0.016), 0.006), body))
    out.append((parts.rbox('stand_neck', (0, 0.06, 0.17), (0.08, 0.04, 0.12), 0.01, rot=Matrix.Rotation(math.radians(-10), 3, 'X')), body))
    tilt = Matrix.Translation(SCREEN_C) @ Matrix.Rotation(math.radians(TILT), 4, 'X')
    bezel = parts.rbox('bezel', (0, 0.0, 0), (SCREEN[0] + 0.04, 0.035, SCREEN[1] + 0.04), 0.008)
    bezel.data.transform(tilt @ Matrix.Translation((0, 0.012, 0)))
    out.append((bezel, body))
    # The panel is modeled upright (so its print maps in X and Z), then tilted with the head.
    panel_m = screen_material('pos_screen', screen_sheet(), -SCREEN[0] / 2, SCREEN[0] / 2, -SCREEN[1] / 2, SCREEN[1] / 2, 1.6, SCREEN_C, TILT)
    panel = parts.rbox('panel', (0, -0.006, 0), (SCREEN[0], 0.004, SCREEN[1]), 0.0)
    out.append((panel, panel_m))
    panel.data.transform(tilt)
    # Receipt printer on the right, a curl of paper out of it.
    out.append((parts.rbox('printer', (0.26, -0.04, 0.075), (0.14, 0.2, 0.15), 0.02), body))
    out.append((parts.rbox('printer_lid', (0.26, -0.02, 0.152), (0.12, 0.14, 0.008), 0.004), trim))
    curl = parts.tube('paper', [(0.26, -0.11, 0.15), (0.26, -0.15, 0.17), (0.26, -0.18, 0.16), (0.26, -0.19, 0.13)], 0.0012, 4, 6)
    curl.data.transform(Matrix.Diagonal((40.0, 1.0, 1.0, 1.0)))
    curl.data.transform(Matrix.Translation((0.26 - 0.26 * 40.0, 0, 0)))
    out.append((curl, paper))
    # The customer's pole display, on the back corner, facing +Y.
    out.append((parts.rod('pole', (-0.16, 0.16, 0.1), (-0.16, 0.16, 0.36), 0.012, 16), metal))
    pole_head = parts.rbox('pole_head', (-0.16, 0.165, 0.39), (POLE[0] + 0.03, 0.04, POLE[1] + 0.03), 0.008)
    out.append((pole_head, trim))
    pole_m = screen_material('pos_pole', pole_sheet(), -0.16 + POLE[0] / 2, -0.16 - POLE[0] / 2, 0.39 - POLE[1] / 2, 0.39 + POLE[1] / 2, 2.5)
    out.append((parts.rbox('pole_glass', (-0.16, 0.186, 0.39), (POLE[0], 0.004, POLE[1]), 0.0), pole_m))
    # The PIN pad on its swivel, on the customer's side to the right (their right: -X).
    out.append((parts.rod('pin_post', (-0.05, 0.3, 0.0), (-0.05, 0.3, 0.06), 0.015, 16), metal))
    pad = parts.rbox('pin_pad', (0, 0, 0), (0.085, 0.16, 0.035), 0.012)
    pad.data.transform(Matrix.Translation((-0.05, 0.3, 0.085)) @ Matrix.Rotation(math.radians(-20), 4, 'X') @ Matrix.Rotation(math.radians(180), 4, 'Z'))
    out.append((pad, body))
    for r in range(4):
        for c in range(3):
            key = parts.rbox('pin_key', (0, 0, 0), (0.018, 0.012, 0.006), 0.002)
            key.data.transform(Matrix.Translation((-0.05, 0.3, 0.085)) @ Matrix.Rotation(math.radians(-20), 4, 'X') @ Matrix.Rotation(math.radians(180), 4, 'Z')
                               @ Matrix.Translation(((c - 1) * 0.024, -0.05 + r * 0.018, 0.019)))
            out.append((key, keys))
    # The handheld scanner in its cradle, by the drawer's left front.
    out.append((parts.rbox('cradle', (-0.27, -0.08, 0.02), (0.08, 0.09, 0.04), 0.012), body))
    gun = parts.rbox('scanner', (0, 0, 0), (0.06, 0.16, 0.05), 0.02)
    gun.data.transform(Matrix.Translation((-0.27, -0.08, 0.085)) @ Matrix.Rotation(math.radians(55), 4, 'X'))
    out.append((gun, body))
    win = parts.rbox('scanner_window', (0, 0, 0), (0.045, 0.004, 0.03), 0.004)
    win.data.transform(Matrix.Translation((-0.27, -0.08, 0.085)) @ Matrix.Rotation(math.radians(55), 4, 'X') @ Matrix.Translation((0, -0.081, 0.0)))
    out.append((win, street.lens('pos_scan', 0x6a1010, strength=0.0, rough=0.08)))
    for o, m in out:
        core.assign(o, m)
    return [parts.finish([o for o, _ in out], 'SM_Register', 40)]
