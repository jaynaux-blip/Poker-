"""SM_Phone: the grinder's phone, face up in a worn black silicone case, its glass cracked from a drop.

The stage lights the display with a lock-screen widget (68 x 146 mm) laid just above the glass. The
crack starts at the bottom-left corner and runs out past the display, so it stays visible over the
bezel when the screen is lit.

Coordinates (meters): origin centered under the case on the desk, Z up; the top of the phone
points +Y (away from the player) and the glass is at DISPLAY_Z.
"""
import math
import random

from artkit import core
from artkit.shade import convex_edges, image_surface, noise, object_coords, planar, ramp
from artkit.sheet import Sheet

MESHES = ['SM_Phone']
DOUBLE_SIDED = False
TEXTURE_SIZE = 2048
TEXTURE_SIZES = {'phone_glass': (1024, 2048), 'phone_case': 1024, 'phone_port': 128}
AO_DISTANCE = 0.006

CASE_W, CASE_H, CASE_R, CASE_T = 0.078, 0.159, 0.0105, 0.0102
GLASS_W, GLASS_H, GLASS_R = 0.0714, 0.1524, 0.0078
DISPLAY_Z = 0.0094
SCREEN_W, SCREEN_H = 0.068, 0.146  # the stage's lock-screen widget

REVIEW_VIEWS = [('front', -25, 30, 2.3), ('top', 5, 70, 2.1), ('detail', -35, 42, 0.75, (-0.022, -0.055, DISPLAY_Z))]


def glass_sheet():
    """The glass seen from above (millimeters, origin at its bottom-left): print, camera, and the crack."""
    w, h = GLASS_W * 1000, GLASS_H * 1000
    s = Sheet(w, h)
    s.rect(0, 0, w, h, 0x050607, rough=0.035)
    # Front camera and earpiece at the top.
    s.circle(w / 2, h - 5.2, 1.6, 0x010102, rough=0.02)
    s.circle(w / 2 + 0.3, h - 4.9, 0.35, 0x1d2a44, rough=0.02)
    s.poly(core.rounded_rect(w / 2 - 6, h - 2.1, w / 2 + 6, h - 1.3, 0.4, steps=3), 0x0b0c0e, rough=0.5)
    # The crack: a spiderweb from an impact near the bottom-left corner.
    rnd = random.Random(7)
    ox, oy = 4.5, 6.0

    def hairline(pts, width, color=0x2e343b):
        # The crack is mostly invisible: a faint line whose broken surface glints. The surface
        # map's metal channel carries the crack mask for the glass shader.
        for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
            dx, dy = x1 - x0, y1 - y0
            ln = math.hypot(dx, dy) or 1.0
            nx, ny = -dy / ln * width / 2, dx / ln * width / 2
            s.poly([(x0 - nx, y0 - ny), (x1 - nx, y1 - ny), (x1 + nx, y1 + ny), (x0 + nx, y0 + ny)], color, metal=1.0, rough=0.3)

    for k in range(11):
        ang = math.radians(-8 + k * 10.5 + rnd.uniform(-4, 4))
        length = rnd.uniform(14, 58) if k % 3 else rnd.uniform(55, 95)
        pts, x, y = [(ox, oy)], ox, oy
        seg = 0
        while math.hypot(x - ox, y - oy) < length:
            ang += rnd.uniform(-0.09, 0.09)
            step = rnd.uniform(3.0, 7.0)
            x, y = x + math.cos(ang) * step, y + math.sin(ang) * step
            if not (0.3 < x < w - 0.3 and 0.3 < y < h - 0.3):
                break
            pts.append((x, y))
            seg += 1
            if seg % 3 == 0 and rnd.random() < 0.3:  # a short branch
                b_ang, bx, by, bpts = ang + rnd.choice((-1, 1)) * rnd.uniform(0.5, 1.0), x, y, [(x, y)]
                for _ in range(rnd.randint(1, 3)):
                    b_ang += rnd.uniform(-0.1, 0.1)
                    bx, by = bx + math.cos(b_ang) * 3.0, by + math.sin(b_ang) * 3.0
                    bpts.append((bx, by))
                hairline(bpts, 0.09)
        hairline(pts, 0.11 if k % 3 else 0.14)
    # Rings of the web close to the impact.
    for radius in (2.2, 4.6, 7.8):
        a0 = math.radians(rnd.uniform(-10, 20))
        pts = [(ox + radius * math.cos(a0 + t * 1.6 / 18), oy + radius * math.sin(a0 + t * 1.6 / 18)) for t in range(19)]
        hairline(pts, 0.1)
    s.circle(ox, oy, 0.6, 0x3a4149, metal=1.0, rough=0.4)  # the crushed point of impact
    return s.render('phone_glass', 2048)


def case_material():
    m = core.Mat('phone_case')
    tc, sep = object_coords(m)
    skin = noise(m, tc.outputs['Object'], scale=2500.0, detail=1.0)
    worn = convex_edges(m, tc, distance=0.0012, samples=16, breakup_scale=300.0)
    # Silicone polishes where hands rub it and picks up lint in the recesses.
    color = m.mix(m.math('MULTIPLY', worn, 0.5), core.hex_linear(0x151518), core.hex_linear(0x2f3035))
    rough = m.math('SUBTRACT', m.math('ADD', 0.62, m.math('MULTIPLY', skin, 0.06)), m.math('MULTIPLY', worn, 0.25))
    lint = m.math('MULTIPLY', ramp(m, noise(m, tc.outputs['Object'], scale=400.0, detail=3.0), 0.7, 0.8), 0.6)
    color = m.mix(lint, color, core.hex_linear(0x6b6862))
    m.set('Base Color', color)
    m.set('Roughness', rough)
    m.set('Metallic', 0.0)
    bump = m.node('ShaderNodeBump', Strength=0.05, Distance=0.00003)
    m.link(skin, bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def glass_material(sheet):
    m = core.Mat('phone_glass')
    tc, sep = object_coords(m)
    uv = planar(m, sep, 'X', 'Y', -GLASS_W / 2, GLASS_W / 2, -GLASS_H / 2, GLASS_H / 2)
    color, _, crack, rough = image_surface(m, sheet, uv)
    # Thumb smudges over the lower half, where it gets unlocked a hundred times a night.
    smudge = m.math('MULTIPLY', ramp(m, noise(m, tc.outputs['Object'], scale=60.0, detail=4.0), 0.5, 0.75),
                    m.math('SUBTRACT', 1.0, ramp(m, sep.outputs['Y'], -0.01, 0.05)))
    rough = m.math('ADD', rough, m.math('MULTIPLY', smudge, 0.16))
    m.set('Base Color', color)
    m.set('Roughness', rough)
    m.set('Metallic', 0.0)
    m.set('IOR', 1.52)
    bump = m.node('ShaderNodeBump', Strength=0.8, Distance=0.00008)
    m.link(m.math('MULTIPLY', crack, -1.0), bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def build():
    core.reset()
    sheet = glass_sheet()
    core.reset()
    case_mat, glass_mat = case_material(), glass_material(sheet)
    port_mat = core.Mat('phone_port')
    port_mat.set('Base Color', core.hex_linear(0x050506))
    port_mat.set('Roughness', 0.7)

    bm = core.slab(core.rounded_rect(-CASE_W / 2, -CASE_H / 2, CASE_W / 2, CASE_H / 2, CASE_R, steps=10), 0.0, CASE_T)
    core.bevel(bm, lambda e: all(abs(v.co.z - CASE_T) < 1e-7 for v in e.verts), 0.0026, segments=5)
    core.bevel(bm, lambda e: all(abs(v.co.z) < 1e-7 for v in e.verts), 0.002, segments=4)
    case = core.mesh_object('case', bm)
    core.assign(case, case_mat)
    # The window over the screen, down to the glass.
    window = core.mesh_object('window', core.slab(core.rounded_rect(-0.0355, -0.0760, 0.0355, 0.0760, 0.0075, steps=10), DISPLAY_Z - 0.003, 0.03))
    core.boolean(case, window)
    # USB-C port at the bottom end.
    port = core.mesh_object('port', core.slab(core.rounded_rect(-0.0046, -CASE_H / 2 - 0.003, 0.0046, -CASE_H / 2 + 0.004, 0.0012, steps=4),
                                              0.0036, 0.0064))
    core.assign(port, port_mat)
    core.boolean(case, port)
    core.finish_hard_surface(case)

    # Button covers molded into the case: power on the right, volume on the left.
    buttons = []
    for x, y0, y1 in ((CASE_W / 2, 0.018, 0.034), (-CASE_W / 2, 0.022, 0.048)):
        side = 1 if x > 0 else -1
        bmb = core.slab(core.rounded_rect(x - 0.0012, y0, x + 0.0012, y1, 0.0011, steps=4), 0.0036, 0.0066)
        core.bevel(bmb, lambda e: all(abs(v.co.z - 0.0066) < 1e-7 or abs(v.co.z - 0.0036) < 1e-7 for v in e.verts), 0.0006, segments=2)
        b = core.mesh_object('button', bmb)
        b.location.x = side * 0.0004
        core.apply_transforms(b)
        buttons.append(b)
    btn = core.join(buttons, 'buttons')
    core.assign(btn, case_mat)
    core.finish_hard_surface(btn)

    gbm = core.slab(core.rounded_rect(-GLASS_W / 2, -GLASS_H / 2, GLASS_W / 2, GLASS_H / 2, GLASS_R, steps=10), DISPLAY_Z - 0.0015, DISPLAY_Z)
    core.bevel(gbm, lambda e: all(abs(v.co.z - DISPLAY_Z) < 1e-7 for v in e.verts), 0.0005, segments=3)
    glass = core.mesh_object('glass', gbm)
    core.assign(glass, glass_mat)
    core.finish_hard_surface(glass)

    obj = core.join([case, btn, glass], 'SM_Phone')
    core.uv_layout(obj, [
        (core.material_is(obj, 'phone_glass'), 'planar', ('x', 'y', -GLASS_W / 2, GLASS_W / 2, -GLASS_H / 2, GLASS_H / 2), (0.0, 0.0, 1.0, 1.0)),
        (core.material_is(obj, 'phone_case'), 'box', None, (0.0, 0.0, 1.0, 1.0)),
        (lambda f: True, 'box', None, (0.0, 0.0, 1.0, 1.0)),
    ])
    return [obj]
