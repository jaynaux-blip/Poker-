"""SM_FS_Belt (Update 4 G1, F-look FL-8a): the festival champion's belt for the apartment's shelf; one mesh for all ten
festivals, tinted per festival by the game (SPEC §2.7.5's strap and buckle colours, §9.2).

The belt stands as a display loop on the shelf's top board (fest_shelf.BELT_SLOTS): a leather strap closed into a flattened
oval (0.35 x 0.16 m) with the main plate at the front, a side plate on each flank, and two slim posts under the strap's
back holding it level with the plate. The plate stands on the board (z = 0), the strap round its middle.

- The strap is light neutral leather (pebbled grain, a stitch line along each edge; Leather 026 outside and Leather 027
  inside in the CC0 build); the plates and posts a neutral polished metal, the main plate's inner panel satin. The game
  multiplies the baked colour by its tint: FS_Tint (vertex colour, exported as COLOR_0) is white with alpha 1 on the strap
  and 0 on the metal, so one material takes the strap's colour and the buckle's metal (gold, silver, chrome, platinum,
  brass) as two parameters. (White RGB leaves a plain glTF import unchanged.)
- UV1 ("Decal", TEXCOORD_1) maps the main plate's inner panel and medallion, seen from the front, onto 0..1 (an upright
  image in Unreal) for the festival's buckle emblem drawn by the game's code (no logo, text or font here); every other face
  sits outside 0..1 (fest_shelf.fs_decal_uv).

Coordinates (meters), front toward -Y, Z up; the origin under the loop's middle on the board.
"""
import math

import bpy  # noqa: F401,I001
from mathutils import Matrix, Vector

from artkit import core, parts
from artkit.shade import mix_float, ramp
from assets import fest_shelf as kit

A, B, N_EXP = 0.17, 0.075, 2.6        # the loop: a superellipse through the strap's middle
STRAP_H, STRAP_T = 0.066, 0.0042
PLATE_W, PLATE_H, PLATE_T = 0.21, 0.128, 0.005
STRAP_Z = PLATE_H / 2                 # the strap round the plate's middle: z 0.031-0.097
PATH_N = 44


def loop_point(t):
    c, s = math.cos(t), math.sin(t)
    return Vector((A * math.copysign(abs(c) ** (2 / N_EXP), c), B * math.copysign(abs(s) ** (2 / N_EXP), s), STRAP_Z))


def loop_outward(p):
    """The loop's outward normal at a point on it (the superellipse's gradient)."""
    gx = math.copysign(abs(p.x / A) ** (N_EXP - 1) / A, p.x)
    gy = math.copysign(abs(p.y / B) ** (N_EXP - 1) / B, p.y)
    return Vector((gx, gy, 0.0)).normalized()


def strap_outer(m, sep, tag):
    """The outer leather with a stitch line 6 mm in from each edge."""
    s = kit.fs_leather('26')(m, sep, tag)
    ang = m.math('ARCTAN2', m.math('DIVIDE', sep.outputs['Y'], B), m.math('DIVIDE', sep.outputs['X'], A))
    dash = ramp(m, m.math('SINE', m.math('MULTIPLY', ang, 190.0)), 0.15, 0.45)
    rows = None
    for zr in (STRAP_Z - STRAP_H / 2 + 0.006, STRAP_Z + STRAP_H / 2 - 0.006):
        d = m.math('ABSOLUTE', m.math('SUBTRACT', sep.outputs['Z'], zr))
        row = m.math('SUBTRACT', 1.0, ramp(m, d, 0.0005, 0.0009))
        rows = row if rows is None else m.math('MAXIMUM', rows, row)
    stitch = m.math('MULTIPLY', rows, dash)
    return {'color': m.mix(stitch, s['color'], core.hex_linear(0xe6e1d8)), 'rough': mix_float(m, stitch, s['rough'], 0.6),
            'metal': 0.0, 'height': m.math('ADD', s['height'], m.math('MULTIPLY', stitch, 0.0003))}


def belt():
    mat = kit.fs_material('fs_belt', [strap_outer, kit.fs_leather('27'), kit.fs_metal('neutral', 0.16), kit.fs_metal('neutral', 0.3, 'X')])
    out = []
    # The strap: swept round the loop from the back (its seam hidden there), on edge.
    path = [tuple(loop_point(math.pi / 2 + 2 * math.pi * k / PATH_N)) for k in range(PATH_N)]
    prof = [(-STRAP_H / 2, -STRAP_T / 2), (STRAP_H / 2, -STRAP_T / 2), (STRAP_H / 2, STRAP_T / 2), (-STRAP_H / 2, STRAP_T / 2)]
    strap = kit.fs_tag(core.sweep('strap', path, prof, closed=True), 0, tint=1.0)

    def inner_face(p, me):
        c = p.center
        return abs(p.normal.z) < 0.5 and p.normal.dot(loop_outward(Vector((c.x, c.y, STRAP_Z)))) < -0.5
    kit.fs_tag_faces(strap, inner_face, 1, tint=1.0)
    out.append(strap)
    # The main plate, its raised inner panel and the medallion, standing on the board in front of the strap.
    y_back = -B - STRAP_T / 2
    at = Matrix.Translation((0.0, y_back, STRAP_Z)) @ parts.rot_x(90)
    main = parts.outline_prism('plate', core.rounded_rect(-PLATE_W / 2, -PLATE_H / 2, PLATE_W / 2, PLATE_H / 2, 0.03, steps=5), [], 0.0, PLATE_T, at)
    out.append(kit.fs_tag(main, 2, tint=0.0))
    panel = parts.outline_prism('panel', core.rounded_rect(-0.086, -0.045, 0.086, 0.045, 0.018, steps=4), [], PLATE_T, PLATE_T + 0.0022, at)
    out.append(kit.fs_tag(panel, 3, tint=0.0))
    medal = kit.fs_lathe('medal', [(0.03, 0.0), (0.026, 0.0022), (0.016, 0.0038), (0.0, 0.0045)],
                         Matrix.Translation((0.0, y_back - PLATE_T - 0.0022, STRAP_Z)) @ parts.rot_x(90), segments=20, toward=(0.0, 0.0, 1.0))
    out.append(kit.fs_tag(medal, 2, tint=0.0))
    # A side plate on each flank, square to the strap.
    for sx in (-1, 1):
        best = min((loop_point(-math.pi / 2 + sx * 2 * math.pi * k / 720) for k in range(181)), key=lambda p: abs(p.x - sx * 0.142))
        nrm = loop_outward(best)
        frame = parts.frame(best + nrm * (STRAP_T / 2), nrm)
        side = parts.outline_prism('side', core.rounded_rect(-0.025, -0.043, 0.025, 0.043, 0.012, steps=3), [], 0.0, 0.004, frame @ parts.rot_x(90))
        out.append(kit.fs_tag(side, 2, tint=0.0))
    # Two posts under the strap's back with their feet.
    for sx in (-1, 1):
        back = min((loop_point(math.pi / 2 - sx * 2 * math.pi * k / 720) for k in range(181)), key=lambda p: abs(p.x - sx * 0.11))
        top = STRAP_Z - STRAP_H / 2 + 0.004
        out.append(kit.fs_tag(parts.rod('post', (back.x, back.y, 0.003), (back.x, back.y, top), 0.0032, 8), 2, tint=0.0))
        out.append(kit.fs_tag(parts.disc('foot', (back.x, back.y, 0.0015), (0, 0, 1), 0.008, 0.003, 8), 2, tint=0.0))
    obj = kit.fs_finish(out, 'SM_FS_Belt', mat, tint=True)
    kit.fs_decal_uv(obj, lambda p, me: p.normal.y < -0.3 and p.center.y < y_back - PLATE_T - 0.0019
                    and abs(p.center.x) < 0.087 and abs(p.center.z - STRAP_Z) < 0.046)
    return obj


MESHES = ['SM_FS_Belt']
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {'fs_belt': 1024}
AO_DISTANCE = 0.01
GLTF_OPTIONS = {'export_vertex_color': 'ACTIVE'}
REVIEW_VIEWS = [('front', -16, 14, 2.3), ('top', 25, 55, 2.3), ('detail', -12, 8, 0.6, (0.0, -0.08, STRAP_Z))]
# Review only: three festivals' tints (strap, buckle metal) as the game will set them (SPEC §2.7.5).
REVIEW_TINTS = [(0x4b3a9c, 0xc9d3e6), (0x7a1020, 0xd9b25f), (0xff8a3d, 0xffd45c)]


def build():
    core.reset()
    kit.FS_CC0_USED.clear()
    return [belt()]


def bake_parts(objs):
    kit.fs_prebake('fest_belt', MESHES, objs, TEXTURE_SIZE, TEXTURE_SIZES)
    return objs


def assemble(objs):
    kit.fs_postbake('fest_belt', MESHES, objs)
    return objs


def pose_for_review(objs):
    """Three belts side by side as the top board holds them, each in a festival's tint."""
    first = objs[0]
    for i, (strap_hex, metal_hex) in enumerate(REVIEW_TINTS):
        o = first if i == 0 else kit.fs_copy(first)
        o.location = (kit.BELT_SLOTS[i][0], 0.0, 0.0)
        kit.fs_review_tint(o, strap_hex, metal_hex)
        if i:
            objs.append(o)
    bpy.context.view_layer.update()   # the review frames all three
