"""SM_StackDryer: a coin-op stacked commercial dryer, two drums, one of the bank behind the dealer.

- An almond enamel cabinet gone yellow with age: scuffed where carts and shins hit it, rust bleeding
  from the bottom seam, lint grey on top, grime around the handles and the coin drops.
- Two round doors: a chrome bezel, dark tempered glass with the drum's perforations just visible
  behind it, a bar handle on the right.
- A coin panel between the drums: a coin drop and a red LED counter for each drum, printed
  instructions, and the maker's plate (Speed Spin, as on the cards).
- A dark kick plate with the two lint drawers.

Coordinates (meters): origin on the floor under the middle of the cabinet, Z up, the doors facing -X
(the stage stands the bank against the far wall, facing the table).
"""
import math

from artkit import core
from artkit.shade import convex_edges, image_surface, mix_float, noise, object_coords, planar, ramp
from artkit.sheet import Sheet

MESHES = ['SM_StackDryer']
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {'dryer_enamel': 2048, 'dryer_panel': 2048, 'dryer_glass': 1024}
AO_DISTANCE = 0.06

W = 0.79          # across (Y)
D = 0.78          # front to back (X)
H = 1.96
FRONT = -D / 2
DRUMS = (0.47, 1.43)      # door centers (Z)
PANEL_Z = (0.86, 1.05)    # the coin panel band
BEZEL_R = 0.33
GLASS_R = 0.268
ENAMEL = 0xd6ccb2
REVIEW_VIEWS = [('front', -88, 10, 3.6), ('angle', -55, 18, 3.6), ('detail', -75, 8, 1.1, (FRONT, 0.0, 0.96))]


def panel_sheet():
    """The coin panel's print (millimeters), as seen from the front: x runs to the viewer's right."""
    w, h = W * 1000 - 20, (PANEL_Z[1] - PANEL_Z[0]) * 1000
    s = Sheet(w, h)
    s.rect(0, 0, w, h, 0x2a2b2d, rough=0.55)
    s.rect(6, 6, w - 12, h - 12, 0x323436, rough=0.5)
    for k, (cx, label) in enumerate(((w * 0.22, 'UPPER'), (w * 0.78, 'LOWER'))):
        # The LED counter window and the coin drop's printed surround.
        s.rect(cx - 52, h - 62, 104, 40, 0x080808, rough=0.08)
        s.text('00', cx, h - 54, 26, 0x3a0b08, face='Bold', align='CENTER')
        s.text(label, cx, h - 14, 9, 0xd8d2c2, face='Bold', align='CENTER', tracking=1.3)
        s.text('25¢ = 8 MIN', cx, 44, 8, 0xd8d2c2, face='Bold', align='CENTER', tracking=1.1)
        s.text('PUSH TO START', cx, 28, 6.5, 0x9a968c, face='Bold', align='CENTER', tracking=1.2)
        for j, temp in enumerate(('HIGH', 'MED', 'LOW')):
            tx = cx - 44 + j * 44
            s.circle(tx, 70, 9, 0x111213, rough=0.3)
            s.text(temp, tx, 54, 6, 0xb8b2a4, face='Bold', align='CENTER')
    # The maker's plate in the middle, riveted on.
    mx = w / 2
    s.rect(mx - 78, h / 2 - 34, 156, 68, 0xb9b6ad, metal=1.0, rough=0.35)
    s.text('SPEED', mx, h / 2 + 4, 15, 0x1b2c4a, face='Black', align='CENTER', tracking=1.15, metal=1.0, rough=0.4)
    s.text('SPIN', mx, h / 2 - 16, 15, 0x9e1f1a, face='Black', align='CENTER', tracking=1.4, metal=1.0, rough=0.4)
    s.text('COMMERCIAL TUMBLE DRYER  30 LB', mx, h / 2 - 28, 4.2, 0x3a3a3a, face='Bold', align='CENTER', metal=1.0, rough=0.4)
    for rx in (mx - 72, mx + 72):
        for ry in (h / 2 - 28, h / 2 + 28):
            s.circle(rx, ry, 2.2, 0x8c8a84, metal=1.0, rough=0.3)
    # A warning sticker, yellowed and curling.
    s.rect(mx - 70, 12, 140, 26, 0xd9c36a, rough=0.6)
    s.text('DO NOT DRY RUBBER OR FOAM', mx, 26, 5.6, 0x1d1a12, face='Black', align='CENTER')
    s.text('REMOVE LINT BEFORE EACH LOAD', mx, 17, 4.6, 0x1d1a12, face='Bold', align='CENTER')
    return s.render('dryer_panel', 2048)


def enamel_material():
    m = core.Mat('dryer_enamel')
    tc, sep = object_coords(m)
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    o = tc.outputs['Object']
    # Baked enamel: smooth, a faint orange peel, yellowed unevenly.
    peel = noise(m, o, scale=700.0, detail=1.0)
    age = noise(m, o, scale=3.0, detail=3.0)
    color = m.mix(ramp(m, age, 0.35, 0.7), core.hex_linear(ENAMEL), core.hex_linear(0xc9bc98))
    rough = m.math('ADD', 0.32, m.math('MULTIPLY', peel, 0.06))
    # Chips to grey primer and steel on the edges and corners.
    chips = ramp(m, convex_edges(m, tc, distance=0.002, breakup_scale=70.0), 0.5, 0.65)
    color = m.mix(chips, color, m.mix(ramp(m, noise(m, o, scale=300.0), 0.4, 0.6), core.hex_linear(0x8d8f90), core.hex_linear(0x6b6e70)))
    metal = mix_float(m, chips, 0.0, 0.6)
    rough = mix_float(m, chips, rough, 0.45)
    # Scuffs along the lower front, where carts and shoes meet it: dark streaks running sideways.
    streak = noise(m, m.math('MULTIPLY', o, 1.0), scale=12.0, detail=6.0)
    scuff_band = m.math('MULTIPLY', ramp(m, m.math('MULTIPLY', z, -1.0), -0.55, -0.15), ramp(m, m.math('MULTIPLY', x, -1.0), -FRONT - 0.02, -FRONT + 0.005))
    scuffs = m.math('MULTIPLY', scuff_band, ramp(m, noise(m, vec_stretch(m, sep, 4.0, 60.0, 900.0), scale=1.0, detail=4.0), 0.58, 0.7))
    color = m.mix(m.math('MULTIPLY', scuffs, 0.6), color, core.hex_linear(0x4a4235))
    # Rust bleeding up from the bottom seam.
    rust = m.math('MULTIPLY', ramp(m, m.math('MULTIPLY', z, -1.0), -0.16, -0.02), ramp(m, noise(m, vec_stretch(m, sep, 40.0, 40.0, 4.0), scale=1.0, detail=5.0), 0.45, 0.65))
    color = m.mix(rust, color, m.mix(streak, core.hex_linear(0x6a3418), core.hex_linear(0x9a5a2a)))
    rough = mix_float(m, rust, rough, 0.8)
    # Lint and dust on top.
    top = ramp(m, z, H - 0.01, H - 0.002)
    color = m.mix(m.math('MULTIPLY', top, 0.85), color, m.mix(ramp(m, noise(m, o, scale=60.0, detail=4.0), 0.4, 0.7), core.hex_linear(0x9a958a), core.hex_linear(0x7c776c)))
    rough = mix_float(m, top, rough, 0.95)
    # Hand grime around each door's handle, on the front only.
    on_front = ramp(m, m.math('MULTIPLY', x, -1.0), -FRONT - 0.03, -FRONT - 0.005)
    for zc in DRUMS:
        hd = m.math('ADD', m.math('MULTIPLY', m.math('SUBTRACT', y, -0.31), m.math('SUBTRACT', y, -0.31)), m.math('MULTIPLY', m.math('SUBTRACT', z, zc), m.math('SUBTRACT', z, zc)))
        grime = m.math('MULTIPLY', ramp(m, m.math('SUBTRACT', 0.02, hd), 0.0, 0.016), ramp(m, noise(m, o, scale=40.0, detail=3.0), 0.3, 0.7))
        color = m.mix(m.math('MULTIPLY', m.math('MULTIPLY', grime, on_front), 0.45), color, core.hex_linear(0x5c5444))
    # Panel seams: the front, top and kick panels meet in dark hairlines.
    seam = None
    for zs in (0.11, PANEL_Z[0] - 0.004, PANEL_Z[1] + 0.004, H - 0.06):
        line = ramp(m, m.math('SUBTRACT', 0.0016, m.math('ABSOLUTE', m.math('SUBTRACT', z, zs))), 0.0, 0.0008)
        seam = line if seam is None else m.math('MAXIMUM', seam, line)
    side = ramp(m, m.math('SUBTRACT', 0.0016, m.math('ABSOLUTE', m.math('SUBTRACT', x, FRONT + 0.03))), 0.0, 0.0008)
    seam = m.math('MAXIMUM', seam, side)
    color = m.mix(seam, color, core.hex_linear(0x2e2a22))
    m.set('Base Color', color)
    m.set('Roughness', rough)
    m.set('Metallic', metal)
    bump = m.node('ShaderNodeBump', Strength=0.05, Distance=0.0003)
    m.link(m.math('ADD', m.math('MULTIPLY', peel, 0.3), m.math('MULTIPLY', rust, 0.6)), bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def vec_stretch(m, sep, sx, sy, sz):
    n = m.node('ShaderNodeCombineXYZ')
    m.link(m.math('MULTIPLY', sep.outputs['X'], sx), n.inputs['X'])
    m.link(m.math('MULTIPLY', sep.outputs['Y'], sy), n.inputs['Y'])
    m.link(m.math('MULTIPLY', sep.outputs['Z'], sz), n.inputs['Z'])
    return n.outputs['Vector']


def chrome_material():
    m = core.Mat('dryer_chrome')
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    smudge = ramp(m, noise(m, o, scale=35.0, detail=4.0), 0.45, 0.75)
    pits = ramp(m, noise(m, o, scale=900.0, detail=2.0), 0.7, 0.78)
    m.set('Base Color', m.mix(pits, core.hex_linear(0xd8d9d8), core.hex_linear(0x6e6a62)))
    m.set('Roughness', m.math('ADD', 0.12, m.math('ADD', m.math('MULTIPLY', smudge, 0.18), m.math('MULTIPLY', pits, 0.3))))
    m.set('Metallic', 1.0)
    return m


def glass_material():
    """Tempered glass with the drum behind it: dark, glossy, the perforated drum faintly showing."""
    m = core.Mat('dryer_glass')
    tc, sep = object_coords(m)
    y, z = sep.outputs['Y'], sep.outputs['Z']
    o = tc.outputs['Object']
    # Distance from the door's axis (doors at DRUMS heights).
    zl = m.math('SUBTRACT', z, DRUMS[1])
    zu = m.math('SUBTRACT', z, DRUMS[0])
    dz = m.math('MINIMUM', m.math('ABSOLUTE', zl), m.math('ABSOLUTE', zu))
    r = m.math('SQRT', m.math('ADD', m.math('MULTIPLY', y, y), m.math('MULTIPLY', dz, dz)))
    # The drum: stainless with a grid of holes, lit dimly through the glass, darker toward the middle.
    holes = m.node('ShaderNodeTexVoronoi', Scale=180.0)
    m.link(o, holes.inputs['Vector'])
    hole = ramp(m, m.math('SUBTRACT', 0.18, holes.outputs['Distance']), 0.0, 0.05)
    drum = m.mix(hole, core.hex_linear(0x3b3d3e), core.hex_linear(0x0a0a0a))
    drum = m.mix(ramp(m, r, 0.05, GLASS_R), core.hex_linear(0x101112), drum)
    color = m.mix(0.6, drum, core.hex_linear(0x070708))
    smears = ramp(m, noise(m, o, scale=25.0, detail=5.0, distortion=0.8), 0.5, 0.8)
    m.set('Base Color', color)
    m.set('Roughness', m.math('ADD', 0.03, m.math('MULTIPLY', smears, 0.2)))
    m.set('Metallic', 0.0)
    m.set('Specular IOR Level', 0.6)
    return m


def panel_material(printed):
    m = core.Mat('dryer_panel')
    tc, sep = object_coords(m)
    # Seen from the front (-X), the viewer's right is -Y.
    uv = planar(m, sep, 'Y', 'Z', -(W / 2 - 0.01), W / 2 - 0.01, PANEL_Z[0], PANEL_Z[1], mirror_a=True)
    ink, alpha, metal, rough = image_surface(m, printed, uv, extension='CLIP')
    wear = ramp(m, noise(m, tc.outputs['Object'], scale=80.0, detail=4.0), 0.55, 0.75)
    m.set('Base Color', m.mix(m.math('MULTIPLY', wear, 0.3), ink, core.hex_linear(0x56524a)))
    m.set('Roughness', rough)
    m.set('Metallic', metal)
    return m


def kick_material():
    m = core.Mat('dryer_kick')
    tc, sep = object_coords(m)
    scuff = ramp(m, noise(m, tc.outputs['Object'], scale=30.0, detail=5.0), 0.5, 0.7)
    m.set('Base Color', m.mix(scuff, core.hex_linear(0x2b2b2a), core.hex_linear(0x4a4740)))
    m.set('Roughness', mix_float(m, scuff, 0.55, 0.8))
    m.set('Metallic', 0.2)
    return m


def axis_x(obj, x, z):
    """Turns a part built around +Z to face -X (the doors), placed at (x, 0, z)."""
    obj.rotation_euler = (0.0, -math.pi / 2, 0.0)
    obj.location = (x, 0.0, z)
    core.apply_transforms(obj)


def build():
    core.reset()
    printed = panel_sheet()
    core.reset()
    enamel, chrome, glass, panel, kick = enamel_material(), chrome_material(), glass_material(), panel_material(printed), kick_material()
    parts = {enamel: [], chrome: [], glass: [], panel: [], kick: []}

    # The cabinet, its vertical corners rounded; the kick plate set in under it.
    cab = core.slab(core.rounded_rect(-D / 2, -W / 2, D / 2, W / 2, 0.012, steps=4), 0.11, H)
    core.bevel(cab, lambda e: all(v.co.z > H - 0.001 for v in e.verts), 0.006, segments=2)
    parts[enamel].append(core.mesh_object('cabinet', cab))
    parts[kick].append(core.mesh_object('kick', core.slab(core.rounded_rect(-D / 2 + 0.015, -W / 2 + 0.01, D / 2, W / 2 - 0.01, 0.004, steps=2), 0.0, 0.11)))
    # The coin panel, standing a centimeter proud of the front.
    pn = core.slab(core.rounded_rect(FRONT - 0.012, -(W / 2 - 0.01), FRONT + 0.002, W / 2 - 0.01, 0.003, steps=2), PANEL_Z[0], PANEL_Z[1])
    parts[panel].append(core.mesh_object('panel', pn))

    for k, zc in enumerate(DRUMS):
        # Chrome bezel: a rounded ring standing off the front.
        bezel = core.lathe(f'bezel{k}', [(GLASS_R - 0.004, 0.0), (GLASS_R - 0.004, 0.016), (GLASS_R + 0.012, 0.034), (BEZEL_R - 0.02, 0.041),
                                         (BEZEL_R - 0.004, 0.035), (BEZEL_R, 0.018), (BEZEL_R, 0.0)], segments=96)
        axis_x(bezel, FRONT, zc)
        parts[chrome].append(bezel)
        # The glass, domed a little, set back inside the bezel.
        pane = core.lathe(f'glass{k}', [(GLASS_R, 0.012), (GLASS_R * 0.7, 0.026), (0.0, 0.031)], segments=96)
        axis_x(pane, FRONT, zc)
        parts[glass].append(pane)
        # The handle on the right, a bar on two posts.
        yh = -(BEZEL_R - 0.015)
        handle = core.sweep(f'handle{k}', core.catmull_rom([(FRONT - 0.03, yh, zc - 0.085), (FRONT - 0.058, yh - 0.012, zc - 0.06),
                                                             (FRONT - 0.062, yh - 0.014, zc), (FRONT - 0.058, yh - 0.012, zc + 0.06),
                                                             (FRONT - 0.03, yh, zc + 0.085)], 6),
                            [(0.009 * math.cos(2 * math.pi * i / 14), 0.009 * math.sin(2 * math.pi * i / 14)) for i in range(14)])
        parts[chrome].append(handle)
        # The hinge block on the left.
        hinge = core.mesh_object(f'hinge{k}', core.slab(core.rounded_rect(FRONT - 0.03, BEZEL_R - 0.03, FRONT + 0.001, BEZEL_R + 0.012, 0.004, steps=2), zc - 0.12, zc + 0.12))
        parts[chrome].append(hinge)
        # The coin drop for this drum: a chrome plate with a push knob.
        cy = (W / 2 - 0.01) - (W - 0.02) * (0.22 if k == 0 else 0.78)
        plate = core.mesh_object(f'coin{k}', core.slab(core.rounded_rect(FRONT - 0.026, cy - 0.032, FRONT - 0.01, cy + 0.032, 0.004, steps=2), PANEL_Z[0] + 0.065, PANEL_Z[0] + 0.125))
        parts[chrome].append(plate)
        knob = core.lathe(f'knob{k}', [(0.0, 0.0), (0.014, 0.0), (0.015, 0.012), (0.012, 0.02), (0.0, 0.022)], segments=32)
        axis_x(knob, FRONT - 0.026, PANEL_Z[0] + 0.095)
        knob.location.y += cy
        core.apply_transforms(knob)
        parts[chrome].append(knob)

    # Lint drawers: two pulls on the kick plate.
    for yy in (-0.2, 0.2):
        pull = core.sweep('pull', [(FRONT + 0.015, yy - 0.05, 0.065), (FRONT - 0.005, yy - 0.045, 0.065), (FRONT - 0.005, yy + 0.045, 0.065), (FRONT + 0.015, yy + 0.05, 0.065)],
                          [(0.006 * math.cos(2 * math.pi * i / 10), 0.006 * math.sin(2 * math.pi * i / 10)) for i in range(10)])
        parts[chrome].append(pull)

    joined = []
    for mat, objs in parts.items():
        for o in objs:
            core.assign(o, mat)
            core.finish_hard_surface(o, 40)
        j = core.join(objs, mat.m.name)
        core.smart_uv(j)
        joined.append(j)
    return [core.join(joined, 'SM_StackDryer')]
