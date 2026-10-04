"""SM_Streetlight: Fifth Street's cobra-head streetlights.

A tapered round steel pole on a fluted cast base cover, an access hatch near the foot, a davit arm that
rises and curves out over the road, and the cobra head: a cast housing, its flat lens underneath (the warm
glow of the lamp in it), and the photocell on top. Painted the city's dark green-grey; weathered.

Coordinates (meters), front toward -Y (the way the arm reaches, over the road), Z up: origin on the
sidewalk at the pole's axis. The lens is centered REACH out and LENS_Z up (AStreetStage puts its light
there).
"""
import math

import bpy  # noqa: F401,I001
from mathutils import Matrix, Vector

from artkit import core, parts, street

MESHES = ['SM_Streetlight']
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {'light_pole': 2048, 'light_lens': 256, 'light_cell': 128, 'light_base': 512}
AO_DISTANCE = 0.08
REVIEW_VIEWS = [('front', -55, 8, 2.5, (0.0, -0.6, 4.1)), ('head', -30, -12, 0.3, (0.0, -1.5, 7.95)), ('base', -25, 15, 0.22, (0.0, 0.0, 0.5))]

POLE_H = 7.6
REACH = 1.5
LENS_Z = 7.95


def build():
    core.reset()
    paint = street.painted('light_pole', 0x2c3033, rough=0.5, chips=0.5, rust=0.6, grime=0.8, grime_height=0.8, streaks=0.7)
    base_m = street.painted('light_base', 0x24282a, rough=0.6, chips=0.7, rust=0.8, grime=1.0, grime_height=0.6)
    lens_m = street.lens('light_lens', 0xffb066, strength=6.0, rough=0.25)
    cell_m = street.shop_plastic('light_cell', 0x1c1d20, rough=0.4)
    out = []
    # The pole, tapering from 9.5 cm to 5.5 cm radius.
    pole = [(0.0, 0.0), (0.095, 0.0), (0.093, 1.0), (0.08, 3.5), (0.066, 6.0), (0.058, POLE_H), (0.0, POLE_H)]
    out.append((parts.lathe_at('pole', pole, Matrix(), segments=40), paint))
    # A fluted base cover over the bolts, and its collar.
    flutes = 16
    base = core.lathe('base', [(0.0, 0.0), (0.2, 0.0), (0.2, 0.05), (0.175, 0.09), (0.15, 0.42), (0.13, 0.5), (0.12, 0.62), (0.0, 0.62)], segments=96)
    for v in base.data.vertices:
        if 0.08 < v.co.z < 0.45:
            a = math.atan2(v.co.y, v.co.x)
            r = v.co.xy.length
            if r > 0.05:
                k = 1.0 + 0.045 * math.cos(a * flutes)
                v.co.x *= k
                v.co.y *= k
    core.orient_normals(base)
    out.append((base, base_m))
    out.append((parts.lathe_at('collar', [(0.098, 0.6), (0.112, 0.61), (0.112, 0.66), (0.098, 0.67)], Matrix(), 40), base_m))
    # The access hatch, facing the sidewalk (+Y), with its two screws.
    hatch = parts.rbox('hatch', (0.0, 0.0, 0.0), (0.075, 0.012, 0.16), 0.006)
    hatch.data.transform(Matrix.Translation((0.0, 0.088, 0.95)))
    out.append((hatch, paint))
    for z in (0.89, 1.01):
        out.append((parts.disc('screw', (0.0, 0.096, z), (0, 1, 0), 0.006, 0.004, 6), base_m))
    # The davit arm: up out of the pole top, curving over to level out above the road.
    arm_pts = [(0.0, 0.0, POLE_H - 0.3), (0.0, 0.0, POLE_H + 0.05), (0.0, -0.08, POLE_H + 0.32), (0.0, -0.35, POLE_H + 0.48),
               (0.0, -0.8, POLE_H + 0.52), (0.0, -REACH + 0.25, LENS_Z + 0.1)]
    out.append((parts.tube('arm', arm_pts, 0.032, 16, 10), paint))
    out.append((parts.lathe_at('arm_sleeve', [(0.06, POLE_H - 0.02), (0.064, POLE_H), (0.064, POLE_H + 0.12), (0.036, POLE_H + 0.18)], Matrix(), 32), paint))
    # The cobra head: a cast housing (lofted), the lens under it, the photocell on top.
    head_c = Vector((0.0, -REACH, LENS_Z))
    rings = []
    stations = [(-0.36, 0.06, 0.035), (-0.3, 0.13, 0.07), (-0.15, 0.19, 0.1), (0.05, 0.2, 0.11), (0.22, 0.17, 0.095), (0.32, 0.1, 0.07), (0.36, 0.04, 0.04)]
    for y, hw, hh in stations:
        ring = []
        for k in range(24):
            t = 2 * math.pi * k / 24
            # A rounded top, a flatter bottom where the lens sits.
            x = hw * math.cos(t)
            z = hh * math.sin(t) if math.sin(t) > 0 else hh * 0.35 * math.sin(t)
            ring.append(Vector((x, -y, z)) + head_c + Vector((0, 0, 0.03)))
        rings.append(ring)
    housing = core.loft('housing', rings)
    core.orient_normals(housing)
    out.append((housing, paint))
    lens = parts.rbox('lens', head_c + Vector((0.0, 0.03, -0.008)), (0.3, 0.46, 0.03), 0.012)
    out.append((lens, lens_m))
    out.append((parts.disc('cell_seat', head_c + Vector((0.0, 0.18, 0.13)), (0, 0, 1), 0.04, 0.02, 24), paint))
    out.append((parts.lathe_at('cell', [(0.0, 0.0), (0.036, 0.0), (0.036, 0.035), (0.03, 0.05), (0.0, 0.055)], Matrix.Translation(head_c + Vector((0.0, 0.18, 0.14))), 24), cell_m))
    for o, m in out:
        core.assign(o, m)
    return [parts.finish([o for o, _ in out], 'SM_Streetlight', 40)]
