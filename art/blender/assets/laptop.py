"""SM_Laptop_Base and SM_Laptop_Lid: the grinder's aging KESTREL 15 laptop.

A machined aluminum unibody in dark gunmetal:
- a backlit chiclet keyboard (every cap modeled, legends printed);
- a glass trackpad, speaker grilles, side ports, a lid scoop and a hinge barrel;
- a glass-front lid with a webcam.
Years of poker show: the F, C, R and A keys (fold, call, raise, all-in), the arrows and the space bar are
worn shiny; the palm rests are polished smooth; the edges are scuffed.

Coordinates (meters), front toward -Y, Z up:
- SM_Laptop_Base: origin on the desk directly below the hinge axis (18.5 mm up); the base runs forward from it.
- SM_Laptop_Lid: origin on the hinge axis, modeled upright (screen facing -Y). NightOneStage tilts
  it back about that axis.
- The display (316 x 197.5 mm, 16:10) is centered 115.25 mm above the hinge, and its glass front
  is 2.6 mm in front of the axis. The game draws the RiverLine client there.
"""
import math
import os

import bpy  # noqa: I001 (bpy first: the PyPI build registers bmesh and mathutils on import)
import bmesh
from mathutils import Matrix

from artkit import core
from artkit.sheet import Sheet

TEXTURE_SIZE = 2048
TEXTURE_SIZES = {'laptop_chassis': 4096, 'laptop_keys': (4096, 2048), 'laptop_trim': 512, 'laptop_rubber': 256}
AO_DISTANCE = 0.012

W = 0.340
FOOT = 0.0015  # rubber feet lift the chassis off the desk
Y_FRONT = -0.2395
Y_BACK = 0.0055
TOP = 0.0155
HINGE_Z = 0.017  # above the chassis bottom; above the desk it is HINGE_HEIGHT
HINGE_HEIGHT = HINGE_Z + FOOT
HINGE_R = 0.0045
HINGE_HALF = 0.125
WELL = (-0.1455, 0.1455, -0.12875, -0.0165)  # x0, x1, y0, y1
WELL_DEPTH = 0.0012
U = 0.01905
KB_X0 = -7.5 * U
KB_TOP = -0.019
FN_D = 0.012
KB_Y0 = KB_TOP - FN_D - 5 * U  # front edge of the key block
KEYS_V = 0.75  # the key block fills the bottom 3/4 of the 4096 x 2048 key texture
TRACKPAD = (0.0, -0.1895, 0.060, 0.039, 0.0055)  # cx, cy, half w, half d, corner radius
GRILLE = (0.1488, 0.1622, -0.1255, -0.0195)  # |x| from, |x| to, y from, y to
STICKERS = (0.092, 0.132, -0.2290, -0.2110)
LID_Z0 = -0.0012
LID_Z1 = 0.2318
LID_FRONT = -0.0026
LID_BACK = 0.0040
DISPLAY = (-0.158, 0.158, 0.0165, 0.214)
WORN = {'F', 'C', 'R', 'A', 'UP', 'DOWN', 'SPACE'}
SCREEN_REFERENCE = os.path.join(core.ROOT, 'art', 'reference', 'screen_riverline.png')

# Review: hero three-quarter, top-down, and a close-up of the worn keys and the trackpad corner.
REVIEW_VIEWS = [('front', -28, 20, 2.5), ('top', 12, 56, 2.2), ('detail', -32, 40, 0.62, (-0.055, -0.105, 0.016))]

CAP = 0x16171b
CAP_WORN = 0x1c1d22
LEGEND = 0xdfe5ec
LEGEND_WORN = 0x80868d


# ------------------------------------------------------------------ keyboard layout

def layout():
    """Keys as (id, legends, x center, y center, width, depth) in meters."""
    keys = []
    fn = ['esc'] + [f'F{i}' for i in range(1, 13)] + ['POWER']
    pitch = 15 * U / 14
    for i, k in enumerate(fn):
        keys.append((k.upper(), (k,), KB_X0 + pitch * (i + 0.5), KB_TOP - FN_D / 2, pitch - 0.003, FN_D - 0.003))
    pair = lambda k, top: (k, (top, k), 1)  # noqa: E731
    rows = [
        [pair('`', '~'), pair('1', '!'), pair('2', '@'), pair('3', '#'), pair('4', '$'), pair('5', '%'), pair('6', '^'),
         pair('7', '&'), pair('8', '*'), pair('9', '('), pair('0', ')'), pair('-', '_'), pair('=', '+'), ('BACKSPACE', ('backspace',), 2)],
        [('TAB', ('tab',), 1.5)] + [(c, (c,), 1) for c in 'QWERTYUIOP'] + [pair('[', '{'), pair(']', '}'), ('\\', ('|', '\\'), 1.5)],
        [('CAPS', ('caps lock',), 1.75)] + [(c, (c,), 1) for c in 'ASDFGHJKL'] + [pair(';', ':'), pair("'", '"'), ('ENTER', ('enter',), 2.25)],
        [('LSHIFT', ('shift',), 2.25)] + [(c, (c,), 1) for c in 'ZXCVBNM'] + [pair(',', '<'), pair('.', '>'), pair('/', '?'), ('RSHIFT', ('shift',), 2.75)],
        [('LCTRL', ('ctrl',), 1.25), ('FN', ('fn',), 1), ('SUPER', (), 1.25), ('LALT', ('alt',), 1.25), ('SPACE', (), 5.25), ('RALT', ('alt',), 1),
         ('RCTRL', ('ctrl',), 1), ('LEFT', (), 1), ('UPDOWN', (), 1), ('RIGHT', (), 1)],
    ]
    for r, row in enumerate(rows):
        y = KB_TOP - FN_D - (r + 0.5) * U
        x = KB_X0
        for kid, legends, units in row:
            w = units * U
            if kid == 'UPDOWN':
                half = (U - 0.003) / 2 - 0.0004
                keys.append(('UP', (), x + w / 2, y + half / 2 + 0.0004, w - 0.003, half))
                keys.append(('DOWN', (), x + w / 2, y - half / 2 - 0.0004, w - 0.003, half))
            else:
                keys.append((kid, legends, x + w / 2, y, w - 0.003, U - 0.003))
            x += w
    return keys


def keyboard_sheet():
    """Keycap tops with legends, top-down over the key block (millimeters, origin at its front-left)."""
    sw = 15 * U * 1000
    sh = (KB_TOP - KB_Y0) * 1000
    s = Sheet(sw, sh)
    s.rect(0, 0, sw, sh, CAP, rough=0.55)
    for kid, legends, x, y, w, d in layout():
        cx, cy = (x - KB_X0) * 1000, (y - KB_Y0) * 1000
        kw, kd = w * 1000, d * 1000
        worn = kid in WORN
        s.rect(cx - kw / 2, cy - kd / 2, kw, kd, CAP_WORN if worn else CAP, rough=0.3 if worn else 0.55)
        col = LEGEND_WORN if worn else LEGEND
        rough = 0.32 if worn else 0.5
        a = 1.3
        if kid == 'LEFT':
            s.poly([(cx - a, cy), (cx + a * 0.7, cy - a), (cx + a * 0.7, cy + a)], col, rough=rough)
        elif kid == 'RIGHT':
            s.poly([(cx + a, cy), (cx - a * 0.7, cy + a), (cx - a * 0.7, cy - a)], col, rough=rough)
        elif kid == 'UP':
            s.poly([(cx, cy + a * 0.8), (cx - a, cy - a * 0.5), (cx + a, cy - a * 0.5)], col, rough=rough)
        elif kid == 'DOWN':
            s.poly([(cx, cy - a * 0.8), (cx + a, cy + a * 0.5), (cx - a, cy + a * 0.5)], col, rough=rough)
        elif kid == 'POWER':
            s.ring(cx, cy, 1.55, 2.05, col, rough=rough, a0=math.radians(120), a1=math.radians(420))
            s.rect(cx - 0.25, cy - 0.1, 0.5, 2.5, col, rough=rough)
        elif kid == 'SUPER':
            s.poly([(cx, cy + 2.0), (cx + 2.0, cy), (cx, cy - 2.0), (cx - 2.0, cy)], col, rough=rough)
            s.poly([(cx, cy + 1.2), (cx + 1.2, cy), (cx, cy - 1.2), (cx - 1.2, cy)], CAP, rough=0.55)
        elif len(legends) == 2:
            s.text(legends[0], cx, cy + 0.8, 3.3, col, face='Regular', align='CENTER', rough=rough)
            s.text(legends[1], cx, cy - 3.8, 3.6, col, face='Regular', align='CENTER', rough=rough)
        elif len(legends) == 1:
            text = legends[0]
            if len(text) == 1 and text.isalpha():
                s.text(text, cx, cy - 1.8, 5.0, col, face='Medium', align='CENTER', rough=rough)
            elif kid == 'ESC' or (kid.startswith('F') and kid[1:].isdigit()):
                s.text(text, cx, cy - 1.0, 2.8, col, face='Regular', align='CENTER', rough=rough)
            else:
                s.text(text, cx - kw / 2 + 1.7, cy - kd / 2 + 1.6, 2.7, col, face='Regular', rough=rough)
    return s.render('laptop_keys', 4096)


def sticker_sheet():
    """Two stickers on the right palm rest (millimeters, 40 x 18)."""
    s = Sheet(40.0, 18.0)
    s.poly(core.rounded_rect(0.5, 0.5, 17.5, 17.5, 1.6), 0x1c3f8f, rough=0.3)
    s.poly(core.rounded_rect(1.2, 1.2, 16.8, 16.8, 1.2), 0x2a61d6, rough=0.3)
    s.text('ZENTRA', 9, 11.4, 2.6, 0xffffff, face='Black', align='CENTER', rough=0.3)
    s.text('X7', 9, 5.9, 4.8, 0xffffff, face='Black', align='CENTER', rough=0.3)
    s.text('12-CORE', 9, 2.7, 1.5, 0xc9d8ff, face='Bold', align='CENTER', tracking=1.2, rough=0.3)
    s.poly(core.rounded_rect(22.5, 0.5, 39.5, 17.5, 1.6), 0x101113, rough=0.3)
    s.poly([(24.8, 12.6), (31, 16.2), (37.2, 12.6), (31, 9.0)], 0x7ad12a, rough=0.3)
    s.text('NOVA', 31, 5.6, 3.2, 0xffffff, face='Black', align='CENTER', rough=0.3)
    s.text('GRAPHICS', 31, 2.6, 1.5, 0x9aa0a8, face='Bold', align='CENTER', tracking=1.2, rough=0.3)
    return s.render('laptop_stickers', 1024)


def bezel_sheet():
    """The lid's glass front, looking at the screen (millimeters over the lid's 340 x 233 face)."""
    w = W * 1000
    h = (LID_Z1 - LID_Z0) * 1000
    s = Sheet(w, h)
    s.rect(0, 0, w, h, 0x07080a, rough=0.06)

    def mm(x, z):
        return (x + W / 2) * 1000, (z - LID_Z0) * 1000

    x0, z0 = mm(DISPLAY[0], DISPLAY[2])
    x1, z1 = mm(DISPLAY[1], DISPLAY[3])
    s.rect(x0, z0, x1 - x0, z1 - z0, 0x020304, rough=0.03)
    cx, cz = mm(0.0, 0.2229)
    s.circle(cx, cz, 2.2, 0x1b1d22, metal=0.6, rough=0.25)
    s.circle(cx, cz, 1.3, 0x0a1224, rough=0.05)
    s.circle(cx + 0.35, cz + 0.35, 0.3, 0x3a4a6a, rough=0.05)
    s.circle(cx + 6.0, cz, 0.45, 0x14201a, rough=0.2)
    s.text('KESTREL', w / 2, 7.2, 3.0, 0x3b3f45, face='Bold', align='CENTER', tracking=1.6, rough=0.3)
    return s.render('laptop_bezel', 2048)


def lid_back_sheet():
    """The emblem on the back of the lid, as seen from behind (millimeters)."""
    w = W * 1000
    h = (LID_Z1 - LID_Z0) * 1000
    s = Sheet(w, h)
    s.rect(0, 0, w, h, 0x484b51, metal=0.95, rough=0.36)
    cx, cy = w / 2, h * 0.55
    polished = dict(metal=1.0, rough=0.08)
    s.ring(cx, cy, 11.2, 12.6, 0x8a8e96, **polished)
    s.text('K', cx + 0.6, cy - 6.1, 17.0, 0x8a8e96, face='Bold', align='CENTER', **polished)
    return s.render('laptop_lid_back', 2048)


# ------------------------------------------------------------------ shader helpers

def smooth_less(m, d, edge, width):
    """About 1 where d < edge, fading to 0 across edge +/- width: a soft step that bakes without jaggies."""
    n = m.node('ShaderNodeMapRange')
    n.interpolation_type = 'SMOOTHSTEP'
    n.inputs['From Min'].default_value = edge - width
    n.inputs['From Max'].default_value = edge + width
    n.inputs['To Min'].default_value = 1.0
    n.inputs['To Max'].default_value = 0.0
    m.link(d, n.inputs['Value'])
    return n.outputs['Result']


def rrect_mask(m, sep, cx, cy, hw, hh, r, width=0.00008):
    """1 inside a rounded rectangle in object XY, 0 outside."""
    dx = m.math('MAXIMUM', m.math('SUBTRACT', m.math('ABSOLUTE', m.math('SUBTRACT', sep.outputs['X'], cx)), hw - r), 0.0)
    dy = m.math('MAXIMUM', m.math('SUBTRACT', m.math('ABSOLUTE', m.math('SUBTRACT', sep.outputs['Y'], cy)), hh - r), 0.0)
    d = m.math('SQRT', m.math('ADD', m.math('MULTIPLY', dx, dx), m.math('MULTIPLY', dy, dy)))
    return smooth_less(m, d, r, width)


def planar(m, sep, a, b, a0, a1, b0, b1, mirror_a=False):
    """Object-space planar coordinates mapping [a0, a1] x [b0, b1] onto [0, 1]^2."""
    uv = m.node('ShaderNodeCombineXYZ')
    src = m.math('MULTIPLY', sep.outputs[a], -1.0) if mirror_a else sep.outputs[a]
    m.link(m.math('DIVIDE', m.math('SUBTRACT', src, a0), a1 - a0), uv.inputs['X'])
    m.link(m.math('DIVIDE', m.math('SUBTRACT', sep.outputs[b], b0), b1 - b0), uv.inputs['Y'])
    return uv.outputs['Vector']


def mix_float(m, factor, a, b):
    n = m.node('ShaderNodeMix')
    n.data_type = 'FLOAT'
    for sock, v in ((n.inputs['Factor'], factor), (n.inputs['A'], a), (n.inputs['B'], b)):
        if isinstance(v, (int, float)):
            sock.default_value = v
        else:
            m.link(v, sock)
    return n.outputs['Result']


def object_coords(m):
    tc = m.node('ShaderNodeTexCoord')
    sep = m.node('ShaderNodeSeparateXYZ')
    m.link(tc.outputs['Object'], sep.inputs['Vector'])
    return tc, sep


def anodized(m, sep, tc):
    """Dark gunmetal anodized aluminum, brushed along X and scuffed bright at the edges.

    Returns (color, roughness, bump node); the bump's Height input carries the brushed grain.
    """
    grain_v = m.node('ShaderNodeCombineXYZ')
    m.link(m.math('MULTIPLY', sep.outputs['X'], 6.0), grain_v.inputs['X'])
    m.link(m.math('MULTIPLY', sep.outputs['Y'], 900.0), grain_v.inputs['Y'])
    m.link(m.math('MULTIPLY', sep.outputs['Z'], 900.0), grain_v.inputs['Z'])
    brush = m.node('ShaderNodeTexNoise', Scale=1.0, Detail=3.0, Roughness=0.5)
    m.link(grain_v.outputs['Vector'], brush.inputs['Vector'])
    # Edge wear on convex edges only: occlusion traced inside the solid finds edges whatever the
    # topology (Pointiness smears across the long triangles booleans leave on flat faces).
    inside = m.node('ShaderNodeAmbientOcclusion', Distance=0.0007)
    inside.inside = True
    inside.only_local = True
    inside.samples = 24
    breakup = m.node('ShaderNodeTexNoise', Scale=90.0, Detail=5.0)
    m.link(tc.outputs['Object'], breakup.inputs['Vector'])
    convex = m.math('MULTIPLY', m.math('SUBTRACT', 0.93, inside.outputs['AO']), 4.0, clamp=True)
    edge = m.math('MULTIPLY', convex, m.math('MULTIPLY', m.math('SUBTRACT', breakup.outputs['Fac'], 0.3), 2.2, clamp=True))
    # Large, soft tonal variation, as real anodizing is never perfectly even.
    tone = m.node('ShaderNodeTexNoise', Scale=6.0, Detail=2.0)
    m.link(tc.outputs['Object'], tone.inputs['Vector'])
    base = m.mix(m.math('MULTIPLY', tone.outputs['Fac'], 0.5), core.hex_linear(0x44474d), core.hex_linear(0x50535a))
    color = m.mix(edge, base, core.hex_linear(0x92969d))
    rough = m.math('ADD', m.math('MULTIPLY', brush.outputs['Fac'], 0.08), 0.30)
    rough = m.math('SUBTRACT', rough, m.math('MULTIPLY', edge, 0.08))
    bump = m.node('ShaderNodeBump', Strength=0.05, Distance=0.00005)
    m.link(brush.outputs['Fac'], bump.inputs['Height'])
    return color, rough, bump


# ------------------------------------------------------------------ base

def base_materials(stickers, keys_sheet):
    chassis = core.Mat('laptop_chassis')
    tc, sep = object_coords(chassis)
    color, rough, bump = anodized(chassis, sep, tc)
    grain = bump.inputs['Height'].links[0].from_socket
    deck = TOP + FOOT  # the shader runs after the chassis is lifted onto its feet
    top = chassis.math('GREATER_THAN', sep.outputs['Z'], deck - 0.0002)
    # Palm rests polished by years of wrists.
    palms = None
    for side in (-1.0, 1.0):
        dx = chassis.math('DIVIDE', chassis.math('SUBTRACT', sep.outputs['X'], side * 0.105), 0.058)
        dy = chassis.math('DIVIDE', chassis.math('SUBTRACT', sep.outputs['Y'], -0.196), 0.042)
        p = chassis.math('SUBTRACT', 1.0, chassis.math('ADD', chassis.math('MULTIPLY', dx, dx), chassis.math('MULTIPLY', dy, dy)), clamp=True)
        palms = p if palms is None else chassis.math('MAXIMUM', palms, p)
    smudge = chassis.node('ShaderNodeTexNoise', Scale=35.0, Detail=4.0)
    chassis.link(tc.outputs['Object'], smudge.inputs['Vector'])
    palms = chassis.math('MULTIPLY', palms, chassis.math('ADD', 0.4, smudge.outputs['Fac']))
    rough = chassis.math('SUBTRACT', rough, chassis.math('MULTIPLY', chassis.math('MULTIPLY', palms, top), 0.06))
    metal = 0.95
    # Trackpad: etched glass.
    cx, cy, hw, hh, r = TRACKPAD
    pad = chassis.math('MULTIPLY', rrect_mask(chassis, sep, cx, cy, hw, hh, r), chassis.math('GREATER_THAN', sep.outputs['Z'], deck - 0.0006))
    color = chassis.mix(pad, color, core.hex_linear(0x2c2e33))
    rough = mix_float(chassis, pad, rough, 0.26)
    metal = mix_float(chassis, pad, metal, 0.05)
    # Keyboard well floor: the backplate seen between the keys.
    x0, x1, y0, y1 = WELL
    floor = chassis.math('MULTIPLY', rrect_mask(chassis, sep, (x0 + x1) / 2, (y0 + y1) / 2, (x1 - x0) / 2, (y1 - y0) / 2, 0.003),
                         chassis.math('LESS_THAN', sep.outputs['Z'], deck - 0.0006))
    color = chassis.mix(floor, color, core.hex_linear(0x0e0f12))
    rough = mix_float(chassis, floor, rough, 0.62)
    metal = mix_float(chassis, floor, metal, 0.3)
    # Speaker grilles: 0.8 mm holes on a 1.6 mm grid, either side of the keyboard.
    gx0, gx1, gy0, gy1 = GRILLE
    ax = chassis.math('ABSOLUTE', sep.outputs['X'])
    band = chassis.math('MULTIPLY', chassis.math('MULTIPLY', chassis.math('GREATER_THAN', ax, gx0), chassis.math('LESS_THAN', ax, gx1)),
                        chassis.math('MULTIPLY', chassis.math('GREATER_THAN', sep.outputs['Y'], gy0), chassis.math('LESS_THAN', sep.outputs['Y'], gy1)))
    band = chassis.math('MULTIPLY', band, top)
    fx = chassis.math('SUBTRACT', chassis.math('FRACT', chassis.math('DIVIDE', sep.outputs['X'], 0.0016)), 0.5)
    fy = chassis.math('SUBTRACT', chassis.math('FRACT', chassis.math('DIVIDE', sep.outputs['Y'], 0.0016)), 0.5)
    dist = chassis.math('SQRT', chassis.math('ADD', chassis.math('MULTIPLY', fx, fx), chassis.math('MULTIPLY', fy, fy)))
    hole = chassis.math('MULTIPLY', smooth_less(chassis, dist, 0.25, 0.05), band)
    color = chassis.mix(hole, color, core.hex_linear(0x040405))
    rough = mix_float(chassis, hole, rough, 0.85)
    metal = mix_float(chassis, hole, metal, 0.0)
    # Stickers.
    sx0, sx1, sy0, sy1 = STICKERS
    st_uv = planar(chassis, sep, 'X', 'Y', sx0, sx1, sy0, sy1)
    st = chassis.image(stickers[0], vector=st_uv, extension='CLIP')
    st_s = chassis.image(stickers[1], non_color=True, vector=st_uv, extension='CLIP')
    st_mask = chassis.math('MULTIPLY', st.outputs['Alpha'], top)
    color = chassis.mix(st_mask, color, st.outputs['Color'])
    st_sep = chassis.node('ShaderNodeSeparateColor')
    chassis.link(st_s.outputs['Color'], st_sep.inputs['Color'])
    rough = mix_float(chassis, st_mask, rough, st_sep.outputs['Green'])
    metal = mix_float(chassis, st_mask, metal, 0.0)
    height = chassis.math('SUBTRACT', chassis.math('MULTIPLY', grain, chassis.math('SUBTRACT', 1.0, st_mask)), chassis.math('MULTIPLY', hole, 3.0))
    chassis.link(height, bump.inputs['Height'])
    chassis.set('Base Color', color)
    chassis.set('Roughness', rough)
    chassis.set('Metallic', metal)
    chassis.set('Normal', bump.outputs['Normal'])

    keys = core.Mat('laptop_keys')
    ktc, ksep = object_coords(keys)
    kuv = planar(keys, ksep, 'X', 'Y', KB_X0, -KB_X0, KB_Y0, KB_TOP)
    kc = keys.image(keys_sheet[0], vector=kuv, extension='EXTEND')
    ks = keys.image(keys_sheet[1], non_color=True, vector=kuv, extension='EXTEND')
    kss = keys.node('ShaderNodeSeparateColor')
    keys.link(ks.outputs['Color'], kss.inputs['Color'])
    keys.set('Base Color', kc.outputs['Color'])
    keys.set('Roughness', kss.outputs['Green'])
    keys.set('Metallic', 0.0)
    # The white backlight shines through the legends only.
    lum = keys.node('ShaderNodeRGBToBW')
    keys.link(kc.outputs['Color'], lum.inputs['Color'])
    glow = keys.math('MULTIPLY', keys.math('SUBTRACT', lum.outputs['Val'], 0.12), 1.4, clamp=True)
    keys.set('Emission Color', keys.mix(glow, (0.0, 0.0, 0.0, 1.0), core.hex_linear(0xd7e8ff)))
    # Dim next to the screen (1.0 = the display's white), as in the game, where it bakes into the emissive factor.
    keys.set('Emission Strength', 0.3)
    texture = keys.node('ShaderNodeTexNoise', Scale=1500.0, Detail=2.0)
    keys.link(ktc.outputs['Object'], texture.inputs['Vector'])
    kb = keys.node('ShaderNodeBump', Strength=0.05, Distance=0.00003)
    keys.link(texture.outputs['Fac'], kb.inputs['Height'])
    keys.set('Normal', kb.outputs['Normal'])

    trim = core.Mat('laptop_trim')  # satin black plastic: keycap sides, hinge cover
    trim.set('Base Color', core.hex_linear(0x141518))
    trim.set('Metallic', 0.0)
    trim.set('Roughness', 0.5)

    rubber = core.Mat('laptop_rubber')  # port interiors and feet
    rubber.set('Base Color', core.hex_linear(0x060607))
    rubber.set('Roughness', 0.8)
    return chassis, keys, trim, rubber


def build_keys(keys_mat, trim):
    bm = bmesh.new()
    z0, z1 = TOP - WELL_DEPTH - 0.0002, TOP + 0.0002
    for kid, legends, x, y, w, d in layout():
        core.keycap(bm, x, y, w, d, z0, z1)
    bm.normal_update()
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if f.normal.z < -0.9], context='FACES_ONLY')  # hidden bottoms
    core.bevel(bm, lambda e: all(abs(v.co.z - z1) < 1e-7 for v in e.verts), 0.00035, segments=2)
    bm.normal_update()
    for f in bm.faces:
        f.material_index = 0 if f.normal.z > 0.5 else 1
    keys = core.mesh_object('laptop_keycaps', bm)
    keys.data.materials.append(keys_mat.m)
    keys.data.materials.append(trim.m)
    core.finish_hard_surface(keys, 50)
    return keys


def build_barrel(trim):
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=48, radius1=HINGE_R, radius2=HINGE_R, depth=2 * HINGE_HALF)
    core.bevel(bm, lambda e: all(abs(v.co.z) > HINGE_HALF - 1e-6 for v in e.verts), 0.0008, segments=3)
    bmesh.ops.rotate(bm, verts=bm.verts[:], cent=(0, 0, 0), matrix=Matrix.Rotation(math.pi / 2, 3, 'Y'))
    bmesh.ops.translate(bm, verts=bm.verts[:], vec=(0.0, 0.0, HINGE_Z))
    barrel = core.mesh_object('hinge', bm)
    core.assign(barrel, trim)
    core.finish_hard_surface(barrel, 40)
    return barrel


def build_feet(rubber):
    bm = bmesh.new()
    for y in (-0.212, -0.028):
        foot = core.slab(core.rounded_rect(-0.125, y - 0.003, 0.125, y + 0.003, 0.003, steps=6), -FOOT, 0.0002)
        core.bevel(foot, lambda e: all(v.co.z < -FOOT + 1e-6 for v in e.verts), 0.0006, segments=2)
        me = bpy.data.meshes.new('foot')
        foot.to_mesh(me)
        foot.free()
        bm.from_mesh(me)
        bpy.data.meshes.remove(me)
    feet = core.mesh_object('feet', bm)
    core.assign(feet, rubber)
    core.finish_hard_surface(feet)
    return feet


def build_base(stickers, keys_sheet):
    chassis, keys_mat, trim, ports = base_materials(stickers, keys_sheet)
    outline = core.rounded_rect(-W / 2, Y_FRONT, W / 2, Y_BACK, 0.0, steps=10, radii=(0.012, 0.012, 0.008, 0.008))
    bm = core.slab(outline, 0.0, TOP)
    core.bevel(bm, lambda e: all(v.co.z > TOP - 1e-6 for v in e.verts), 0.0022, segments=4)
    core.bevel(bm, lambda e: all(v.co.z < 1e-6 for v in e.verts), 0.0030, segments=4)
    base = core.mesh_object('laptop_base', bm)
    core.assign(base, chassis)

    # Keyboard well, trackpad recess, and the scoop at the front edge for lifting the lid.
    x0, x1, y0, y1 = WELL
    core.boolean(base, core.mesh_object('well', core.slab(core.rounded_rect(x0, y0, x1, y1, 0.003, steps=6), TOP - WELL_DEPTH, TOP + 0.01)))
    cx, cy, hw, hh, r = TRACKPAD
    core.boolean(base, core.mesh_object('pad', core.slab(core.rounded_rect(cx - hw, cy - hh, cx + hw, cy + hh, r, steps=8), TOP - 0.0003, TOP + 0.01)))
    bpy.ops.mesh.primitive_uv_sphere_add(segments=64, ring_count=32, radius=1.0, location=(0.0, Y_FRONT, TOP + 0.0035))
    scoop = bpy.context.view_layer.objects.active
    scoop.scale = (0.022, 0.012, 0.006)
    core.boolean(base, scoop)

    # Side ports: two USB-C and the headphone jack on the left, USB-A and HDMI on the right.
    def slot(yc, zc, w, h, r):
        return core.rounded_rect(yc - w / 2, zc - h / 2, yc + w / 2, zc + h / 2, r, steps=6)

    left = (-W / 2 - 0.004, -W / 2 + 0.007)
    right = (W / 2 - 0.008, W / 2 + 0.004)
    cutters = [
        (slot(-0.052, 0.0072, 0.0084, 0.0026, 0.0012), left),
        (slot(-0.068, 0.0072, 0.0084, 0.0026, 0.0012), left),
        (core.circle_points(-0.092, 0.0075, 0.00175, 24), left),
        (slot(-0.058, 0.0078, 0.0125, 0.0048, 0.0003), right),
        ([(-0.0855, 0.0055), (-0.0745, 0.0055), (-0.073, 0.0066), (-0.073, 0.0101), (-0.087, 0.0101), (-0.087, 0.0066)], right),
    ]
    for i, (outline_yz, (xa, xb)) in enumerate(cutters):
        c = core.prism_x(f'port{i}', outline_yz, xa, xb)
        core.assign(c, ports)
        core.boolean(base, c)
    core.finish_hard_surface(base)

    obj = core.join([base, build_keys(keys_mat, trim), build_barrel(trim), build_feet(ports)], 'SM_Laptop_Base')
    obj.data.transform(Matrix.Translation((0.0, 0.0, FOOT)))
    is_keys = core.material_is(obj, 'laptop_keys')
    is_chassis = core.material_is(obj, 'laptop_chassis')
    core.uv_layout(obj, [
        (is_keys, 'planar', ('x', 'y', KB_X0, -KB_X0, KB_Y0, KB_TOP), (0.0, 0.0, 1.0, KEYS_V)),
        (lambda f: is_chassis(f) and f.normal.z > 0.9, 'planar', ('x', 'y', -W / 2, W / 2, Y_FRONT, Y_BACK), (0.0, 0.28, 1.0, 1.0)),
        (lambda f: is_chassis(f) and f.normal.z < -0.9, 'box', None, (0.70, 0.0, 1.0, 0.27)),
        (is_chassis, 'box', None, (0.0, 0.0, 0.69, 0.27)),
        (core.material_is(obj, 'laptop_trim'), 'box', None, (0.0, 0.0, 1.0, 1.0)),
        (lambda f: True, 'box', None, (0.0, 0.0, 1.0, 1.0)),  # port interiors and feet
    ])
    return obj


# ------------------------------------------------------------------ lid

def build_lid(back_sheet, bezel):
    shell = core.Mat('laptop_lid')
    tc, sep = object_coords(shell)
    color, rough, bump = anodized(shell, sep, tc)
    luv = planar(shell, sep, 'X', 'Z', -W / 2, W / 2, LID_Z0, LID_Z1, mirror_a=True)
    logo = shell.image(back_sheet[0], vector=luv, extension='EXTEND')
    logo_s = shell.image(back_sheet[1], non_color=True, vector=luv, extension='EXTEND')
    ls = shell.node('ShaderNodeSeparateColor')
    shell.link(logo_s.outputs['Color'], ls.inputs['Color'])
    on_back = shell.math('GREATER_THAN', sep.outputs['Y'], LID_BACK - 0.0004)
    polish = shell.math('DIVIDE', shell.math('SUBTRACT', 0.36, ls.outputs['Green']), 0.28, clamp=True)
    is_logo = shell.math('MULTIPLY', on_back, polish)
    shell.set('Base Color', shell.mix(is_logo, color, logo.outputs['Color']))
    shell.set('Roughness', mix_float(shell, is_logo, rough, ls.outputs['Green']))
    shell.set('Metallic', 0.95)
    shell.set('Normal', bump.outputs['Normal'])

    glass = core.Mat('laptop_bezel')
    _, gsep = object_coords(glass)
    guv = planar(glass, gsep, 'X', 'Z', -W / 2, W / 2, LID_Z0, LID_Z1)
    bc = glass.image(bezel[0], vector=guv, extension='EXTEND')
    bs = glass.image(bezel[1], non_color=True, vector=guv, extension='EXTEND')
    bss = glass.node('ShaderNodeSeparateColor')
    glass.link(bs.outputs['Color'], bss.inputs['Color'])
    glass.set('Base Color', bc.outputs['Color'])
    glass.set('Roughness', bss.outputs['Green'])
    glass.set('Metallic', bss.outputs['Red'])

    # Modeled lying flat (XY = the lid face, Z = thickness), then stood up.
    outline = core.rounded_rect(-W / 2, LID_Z0, W / 2, LID_Z1, 0.0, steps=10, radii=(0.004, 0.004, 0.010, 0.010))
    bm = core.slab(outline, -LID_BACK, -LID_FRONT)
    core.bevel(bm, lambda e: all(v.co.z < -LID_BACK + 1e-6 for v in e.verts), 0.0024, segments=4)
    core.bevel(bm, lambda e: all(v.co.z > -LID_FRONT - 1e-6 for v in e.verts), 0.0006, segments=2)
    bmesh.ops.rotate(bm, verts=bm.verts[:], cent=(0, 0, 0), matrix=Matrix.Rotation(math.pi / 2, 3, 'X'))
    bm.normal_update()
    for f in bm.faces:
        f.material_index = 1 if f.normal.y < -0.9 else 0
    lid = core.mesh_object('SM_Laptop_Lid', bm)
    lid.data.materials.append(shell.m)
    lid.data.materials.append(glass.m)
    core.finish_hard_surface(lid)
    is_glass = core.material_is(lid, 'laptop_bezel')
    core.uv_layout(lid, [
        (is_glass, 'planar', ('x', 'z', -W / 2, W / 2, LID_Z0, LID_Z1), (0.0, 0.0, 1.0, 1.0)),
        (lambda f: f.normal.y > 0.9, 'planar', ('-x', 'z', -W / 2, W / 2, LID_Z0, LID_Z1), (0.0, 0.2, 1.0, 1.0)),
        (lambda f: True, 'box', None, (0.0, 0.0, 1.0, 0.19)),
    ])
    return lid


def build():
    core.reset()
    keys_sheet = keyboard_sheet()
    stickers = sticker_sheet()
    bezel = bezel_sheet()
    back = lid_back_sheet()
    core.reset()
    base = build_base(stickers, keys_sheet)
    lid = build_lid(back, bezel)
    return [base, lid]


def pose_for_review(objs):
    """Opens the lid and lights the screen with the RiverLine client, for the review renders only."""
    _, lid = objs
    lid.location = (0.0, 0.0, HINGE_HEIGHT)
    lid.rotation_euler.x = math.radians(-18.3)
    x0, x1, z0, z1 = DISPLAY
    bpy.ops.mesh.primitive_plane_add(size=1.0)
    screen = bpy.context.view_layer.objects.active
    screen.name = 'review_screen'
    screen.scale = (x1 - x0, z1 - z0, 1.0)
    screen.rotation_euler.x = math.pi / 2
    screen.location = (0.0, LID_FRONT - 0.0002, (z0 + z1) / 2)
    m = core.Mat('review_screen')
    img = m.image(SCREEN_REFERENCE)
    m.set('Base Color', (0.0, 0.0, 0.0, 1.0))
    m.set('Emission Color', img.outputs['Color'])
    m.set('Emission Strength', 1.2)
    m.set('Roughness', 0.05)
    core.assign(screen, m)
    screen.parent = lid
    bpy.context.view_layer.update()
