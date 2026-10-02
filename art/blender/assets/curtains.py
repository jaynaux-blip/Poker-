"""SM_Curtains: GearDrop's blackout curtains, open either side of the window.

A black rod on two wall brackets above the window, ball finials, and two heavy panels gathered on rings:
pinch-pleated at the top, the folds deepening and flaring a little toward the hem, which stops above the
desk.

Coordinates (meters), front toward -Y, Z up: origin on the floor at the wall face, under the window's
middle (the window spans x +/-0.72, z 0.98 to 2.12; the desk's top is at 0.75 and its corners at x +/-0.8).
"""
import math
import random

import bpy  # noqa: F401,I001
import bmesh
from mathutils import Matrix, Vector

from artkit import core, parts

MESHES = ['SM_Curtains']
DOUBLE_SIDED = False
TEXTURE_SIZE = 2048
TEXTURE_SIZES = {'curtain_rod': 512, 'curtain_ring': 256}
AO_DISTANCE = 0.06
REVIEW_VIEWS = [('front', -10, 8, 4.2), ('side', 60, 10, 4.0), ('detail', -30, 10, 0.9, (-0.9, -0.09, 2.1))]

ROD_Y = -0.095
ROD_Z = 2.27
ROD_X = 1.16
TOP, HEM = 2.245, 0.81
INNER, OUTER = 0.79, 1.10  # the panels' gathered extent from the middle
FOLDS = 7


def panel(side, rng, fabric):
    """One panel (side -1 left, +1 right) as a pleated sheet, given thickness."""
    nu, nv = 84, 34
    phase = [rng.uniform(-0.4, 0.4) for _ in range(FOLDS * 2 + 2)]
    amp = [rng.uniform(0.8, 1.2) for _ in range(FOLDS * 2 + 2)]
    bm = bmesh.new()
    grid = []
    for j in range(nv + 1):
        v = j / nv
        row = []
        for i in range(nu + 1):
            u = i / nu
            k = int(u * FOLDS * 2)
            # Tight pinch pleats under the rings, opening up below; the hem flares toward the window.
            a = (0.012 + 0.02 * min(v * 4.0, 1.0) + 0.012 * v) * (amp[k] * (1 - u % 1) + amp[min(k + 1, len(amp) - 1)] * (u % 1))
            wave = math.sin(2 * math.pi * FOLDS * u + phase[k] * v)
            flare = 0.05 * v ** 1.5
            x = INNER - flare + u * (OUTER - INNER + flare * 0.6)
            y = ROD_Y + 0.004 + a * wave - 0.004 * v
            z = TOP - v * (TOP - HEM) + (0.012 * math.cos(2 * math.pi * FOLDS * u) * (v > 0.98)) + 0.004 * wave * v
            row.append(bm.verts.new((side * x, y, z)))
        grid.append(row)
    for j in range(nv):
        for i in range(nu):
            f = (grid[j][i], grid[j][i + 1], grid[j + 1][i + 1], grid[j + 1][i])
            bm.faces.new(f if side > 0 else tuple(reversed(f)))
    obj = core.mesh_object('panel', bm)
    core.orient_normals(obj, toward=(0, -1, 0))
    mod = obj.modifiers.new('solid', 'SOLIDIFY')
    mod.thickness = 0.004
    mod.offset = 0.0
    core.select_only([obj])
    bpy.ops.object.modifier_apply(modifier=mod.name)
    core.assign(obj, fabric)
    # Rings on the rod at every other fold, over the panel's top.
    rings = []
    for k in range(FOLDS + 1):
        u = k / FOLDS
        x = side * (INNER + u * (OUTER - INNER))
        ring = core.lathe('ring', [(0.0175 + 0.0026 * math.cos(math.radians(a)), 0.0026 * math.sin(math.radians(a))) for a in range(0, 361, 30)], segments=24)
        core.orient_normals(ring)
        ring.data.transform(Matrix.Translation((x, ROD_Y, ROD_Z - 0.003)) @ parts.rot_y(90))
        hook = parts.rod('hook', (x, ROD_Y, ROD_Z - 0.022), (x, ROD_Y + 0.003, TOP - 0.004), 0.0012, 6)
        rings += [ring, hook]
    return obj, rings


def build():
    core.reset()
    rng = random.Random(3)
    fabric = parts.fabric('curtain_fabric', 0x222937, period=0.0012, rough=0.9, sheen=0.6, hex_alt=0x2c3446)
    rod_m = parts.metal('curtain_rod', 0x18191b, rough=0.35, anodized=True)
    ring_m = parts.metal('curtain_ring', 0x2a2b2e, rough=0.3)
    out = []
    rod = parts.rod('rod', (-ROD_X, ROD_Y, ROD_Z), (ROD_X, ROD_Y, ROD_Z), 0.0115, 24)
    out.append((rod, rod_m))
    for s in (-1, 1):
        ball = core.lathe('finial', [(0.0, -0.03)] + [(0.026 * math.sin(math.radians(a)), -0.026 * math.cos(math.radians(a))) for a in range(15, 180, 15)] + [(0.0, 0.026)], segments=32)
        core.orient_normals(ball)
        ball.data.transform(Matrix.Translation((s * (ROD_X + 0.028), ROD_Y, ROD_Z)) @ parts.rot_y(90))
        collar = parts.disc('collar', (s * (ROD_X + 0.002), ROD_Y, ROD_Z), (1, 0, 0), 0.015, 0.012, 24)
        bracket = parts.rbox('bracket', (s * 1.02, ROD_Y / 2, ROD_Z + 0.005), (0.016, -ROD_Y + 0.01, 0.022), 0.004)
        plate = parts.rbox('plate', (s * 1.02, -0.004, ROD_Z + 0.005), (0.04, 0.008, 0.07), 0.003)
        cradle = parts.disc('cradle', (s * 1.02, ROD_Y, ROD_Z), (1, 0, 0), 0.016, 0.02, 24)
        out += [(ball, rod_m), (collar, rod_m), (bracket, rod_m), (plate, rod_m), (cradle, rod_m)]
        p, rings = panel(s, rng, fabric)
        out.append((p, None))
        out += [(r, ring_m) for r in rings]
    objs = []
    for o, m in out:
        if m is not None:
            core.assign(o, m)
        objs.append(o)
    return [parts.finish(objs, 'SM_Curtains', 50)]


def pose_for_review(objs):
    wall = parts.rbox('review_wall', (0.0, 0.05, 1.3), (2.8, 0.1, 2.6), 0.0)
    m = core.Mat('review_wall')
    m.set('Base Color', core.hex_linear(0xa1a394))
    m.set('Roughness', 0.9)
    core.assign(wall, m)
