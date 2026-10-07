"""SM_NS_Gym_*: IronHouse Gym, 246 Market St (plot M-S4), a two-storey brick warehouse halfway down Market next door to
the pawn (world realism spec section 4.6, section 5.3). Built with the shop kit in ns_thrift.py.

Frames (spec 0.4): SITE meshes have Blender origin = the stage origin (place at the stage's transform, yaw 180; stage
(X, Y, Z) cm = (-100x, 100y, 100z)); PROP meshes face Blender -X and go at stage L with yaw F + 180 to face stage yaw F
(Blender yaw -F in a SITE scene: ns_thrift.S2B). Meters, Z up; prints read from outside their face (spec 0.3).

The plot (spec 2.4, M-S4 since the spec's v2): the opening X -4560..-6540 in the Y 4950 plane, Z 0..450; the room
X -4560..-6540, Y 3450..4920, the roof deck at Z 520 over exposed trusses; the glass door X -4640..-4840 (its leaf
X -4646..-4742 hinged on the west jamb, a fixed sidelite east of it) with the fob reader on the leaf's east jamb.

Slots: baked slots <prefix>_<group> (Unreal: M_<Mesh>_<i>); unbaked: ns_glass ([glass]), ns_runtime_haze ([runtime]), ns_mirror ([mirror], the
mirror wall's panels, UV 0..1 each, base color = T_NSGY_MirrorMask), ns_belt ([runtime]: the treadmill belt's top, UV v
along the run, T_NS_GymBelt tiles along V: pan V while someone runs), ns_screen_console / ns_screen_tablet /
ns_screen_wod / ns_screen_tv ([screen], UV 0..1 across each as seen, a placeholder picture until core draws), glow_*
(the _Glow<Family> meshes). The build log prints each mesh's slot order.

SITE meshes:
- SM_NS_Gym_Front (baked: body, print): a black-painted steel storefront on a concrete curb (Z 0..45): the door frame
  and its transom, eight 1.56 m glazing bays of vision glass (Z 50..305) under an upper sash of small lights (Z 312..369),
  the roll-up loading door (X -6100..-6460, closed, painted IronHouse red with the kettlebell mark, IRONHOUSE, 24/7 ·
  MEMBERS ANY HOUR, DOOR IN USE · NO PARKING; slats, guides, hood, bottom bar and lock), the charcoal sign band
  (Z 372..450) with IRONHOUSE and 24/7 channel letters (red returns; faces in _GlowCool) and MEMBERS ANY HOUR painted;
  the fob reader on the leaf's east jamb (its MEMBERS / TAP FOB plate and green LED lit in the print slot); the 24/7
  neon's clear back and transformer hung in the first bay.
- SM_NS_Gym_Front_Glass ([glass]): vision glass and sash lights (Y 4940), the sidelite and transom.
- SM_NS_Gym_Front_GlowCool: the channel letters' faces (IRONHOUSE centered at X -5430, 24/7 at X -6280; Z 384..442).
- SM_NS_Gym_Front_GlowNeon: the 24/7 neon tubes in the first window bay (X -4918, Y 4930, Z 236..263; the game sets
  red). Its light: a red rect the sign's size facing out.
- SM_NS_Gym_Shell (baked: floor, wall, roof, steel, trim, print; then ns_mirror): 1 m recycled-rubber tiles (flecks,
  seams, chalk, worn paths door -> desk -> treadmills -> weights -> bags) with the IRONHOUSE emblem inlaid (3.6 m, at
  X -5540..-5900, Y 4040..4400); painted brick walls (charcoal to 1.2 m, a red stripe, white above), the IRONHOUSE
  mural on the east wall; a corrugated deck on purlins and five Warren trusses (X -4900, -5230, -5560, -5890, -6220,
  bottom chords at Z 462) painted black; two galvanized duct runs (Y 4560, 3860, Z 415) with drum louvers; the red
  sprinkler main and heads; eight LED high-bays on cords; the mirror wall on the back wall (X -4960..-6400, Z 35..245,
  aluminum J-trim); three heavy-bag brackets on the east wall; the red door to the lockers and sauna with LOCKERS ·
  SAUNA / MEMBERS and a lit EXIT sign; an extinguisher, the electrical panel and conduit, two speakers.
- SM_NS_Gym_Shell_GlowCool: the eight high-bay lenses, at stage X -4880, -5370, -5860, -6350 by Y 4460, 3980, Z 404
  (5000 K; one MegaLight each).
- SM_NS_Gym_Banners (baked: body, print): the back wall's dressing over the mirrors, seen from the street through the
  windows: the club's vinyl banner (X -5040..-5360, Z 302..408), Rosa's 1999 city amateur championship banner in red
  felt on a dowel (X -5575..-5675, Z 280..420), two framed fight-night posters (X -5970, -6090, Z 305..395).
- SM_NS_Gym_Haze (optional; one [runtime] slot ns_runtime_haze): the chalk haze card, three horizontal sheets over the
  lifting end (stage X -5550..-6520, Y 3470..4350; Z 280, 330, 380), UV 0..1 across each; base color T_NSGY_HazeMask
  (white = dust) for a translucent unlit runtime material. Kept out of the shell so the game can leave it off.

PROP meshes (origin on the floor under the middle unless noted; front -X):
- SM_NS_Gym_Door: the full-glass leaf (origin on its hinge axis at the floor, the closed leaf along +Y, the street face
  -X): hinge at stage (X -4646, Y 4940), F 90 closed; it swings in as F rises toward 180. A long pull and the black
  vinyl hours panel outside (IRONHOUSE · 24/7 · MEMBERS ANY HOUR · STAFFED 6 AM–12 PM & 4–10 PM, spec 4.9), a push bar
  and a closer inside.
- SM_NS_Gym_Treadmill (2.0 x 0.86 m; the runner faces -X, the console at the -X end): the belt top at Z 0.22
  (x -0.62..0.86), the console screen tilted toward the runner. The row (gym-treadmill1..5): stage X -4960, -5080, -5200,
  -5320, -5440 at Y 4725, F 90 (the runners face the street).
- SM_NS_Gym_Bike (indoor cycle, the rider faces -X): saddle top 0.98 at (0.16, 0, 0.98).
- SM_NS_Gym_Bench (adjustable bench laid flat; sitting on its -X end faces -X): seat top 0.45 at (-0.42, 0, 0.45).
- SM_NS_Gym_SquatRack (power rack, open toward -X): a barbell rests at RACK_BAR (-0.515, 0, 1.315); plates on the back
  horns.
- SM_NS_Gym_Barbell (origin at its middle, along Y), SM_NS_Gym_Dumbbell (25 lb hex, origin at the handle's middle,
  along Y; scale 0.8..1.35 for the other weights).
- SM_NS_Gym_DumbbellRack (2-tier, 2.4 m along Y): DB0..DB19 (dumbbell_points()): (x, y, z) with yaw 90 and pitch 15
  for a dumbbell on the saddles: lower tier x 0.24, z 0.575; upper x -0.02, z 0.935; Y -1.05 + 0.233 i.
- SM_NS_Gym_PlateTree: loaded (45s, 35s, 25s, 10s, 5s), horns toward +-Y.
- SM_NS_Gym_HeavyBag: origin at the hook's eye (the game sways it there); hang it at a bracket's eye: stage
  (X -6462, Y 4580 | 4350 | 4120, Z 247), F 0 (IRONHOUSE toward the room). The bag spans Z -0.62..-1.88 below it.
- SM_NS_Gym_Desk (2.2 m; customer -X; transaction ledge 1.10, work top 0.76): the fob reader (lit LED) and the
  sign-in tablet ([screen], also where core can draw BACK AT 4) on the ledge.
- SM_NS_Gym_Cooler (0.78 x 0.86 x 2.04, the door -X): shelf tops at 0.28, 0.73, 1.18, 1.63, as the Lucky Penny's: a
  SM_Stock_CoolerRow_* (Cascade water, Volt Rush) stands at (0, 0, shelf top) with yaw -90 (its front -Y turned to -X).
- P1: SM_NS_Gym_SpeedBag (wall piece: origin at the wall plane, toward -X; board underside 1.85),
  SM_NS_Gym_CableStation, SM_NS_Gym_Mats (the stretch corner: mats, yoga mats in a bin, rollers, kettlebells, chalk),
  SM_NS_Gym_Lockers (wall piece, six double-tier lockers 11-22 and a bench), SM_NS_Gym_Whiteboard (wall piece: the WOD
  board [screen] ns_screen_wod and a 55 in TV [screen] ns_screen_tv), SM_NS_Gym_Fan + SM_NS_Gym_Fan_Blades (wall piece;
  the blades' origin is the hub: place them at the fan's FAN_HUB (-0.32, 0, 3.2) and spin about X).

LAYOUT below is the review's floor plan (stage cm, yaw): the desk and cooler inside the door, the treadmill row and two
bikes along the windows, the dumbbell rack and benches at the mirror wall, two racks with plate trees in the back-east,
the bags on the east wall, the stretch corner inside the roll-up door, lockers and the whiteboard on the west wall.

GYM_ONLY=front,shell,... (keys of NAMES) builds just those; GYM_REVIEW=0 skips the store and close-look sheets.
"""
import math
import os
import random

import bmesh
import bpy  # noqa: F401
import numpy as np
from mathutils import Matrix, Vector

from artkit import core, parts, texmaps
from artkit.sheet import Sheet
from assets import ns_thrift as kit
from assets.ns_thrift import (H, Kit, M, S2B, UNB, bend, bx, bxc, glow, hand_mid, image_mat, lathe_axis, lathe_solid, quad, render_sheet, rod,
                              screen_mat, seg_box, soft_slab, text_mesh, text_w, tube, xf, _tone)

MODULE = 'ns_gym'

TEXTURE_SIZE = 1024
TEXTURE_SIZES = {
    'gyf_body': 4096, 'gyf_print': 4096, 'gys_floor': (4096, 3072), 'gys_wall': 4096, 'gys_roof': 2048, 'gys_steel': 2048, 'gys_trim': 2048,
    'gys_print': 4096, 'gyt_body': 2048, 'gyt_print': 1024, 'gyb_body': 2048, 'gyb_print': 512, 'gyr_body': 2048, 'gyr_print': 1024,
    'gyn_body': 1024, 'gyl_body': 1024, 'gyd_body': 512, 'gyk_body': 2048, 'gyp_body': 2048, 'gyp_print': 1024, 'gyh_body': 1024,
    'gyh_print': 512, 'gye_body': 2048, 'gye_print': 2048, 'gyc_body': 2048, 'gyc_print': 1024, 'gyw_body': 512, 'gyw_print': 1024,
    'gyfan_body': 1024, 'gyfb_body': 512, 'gysb_body': 1024, 'gycs_body': 2048, 'gycs_print': 512, 'gym_body': 2048, 'gylk_body': 2048,
    'gylk_print': 1024, 'gyd_print': 256, 'gyo_body': 1024, 'gyq_body': 2048, 'gyq_print': 1024, 'gyg_body': 1024, 'gyg_print': 512,
    'gybn_body': 1024, 'gybn_print': 4096,
}
AO_DISTANCE = 0.05
DOUBLE_SIDED = False
REVIEW_SCREEN_LIGHT = False
REVIEW_VIEWS = None

BLACK = 0x1c1d1f        # powder-coated frames
IRON_RED = 0xb23a1e     # the IronHouse brand red-orange
STEEL_GREY = 0x55595d

# ------------------------------------------------------------------ the plot (stage cm, spec 2.3 / 2.4)

OPEN_X = (-4560.0, -6540.0)       # the storefront opening in the Y 4950 plane, Z 0..450
OPEN_Y = 4950.0
OPEN_TOP = 450.0
IN_X = (-4560.0, -6540.0)
IN_Y = (3450.0, 4920.0)
CEIL = 520.0                      # the roof deck's underside over the exposed trusses
DOOR_X = (-4640.0, -4840.0)       # the glass door's frame (the leaf and a sidelite); the fob reader on the leaf's east jamb

OX0, OX1 = -OPEN_X[0] / 100, -OPEN_X[1] / 100      # 45.60 .. 65.40
IX0, IX1 = -IN_X[0] / 100, -IN_X[1] / 100
IY0, IY1 = IN_Y[0] / 100, IN_Y[1] / 100            # 34.50 .. 49.20
FRONT_Y = OPEN_Y / 100
GY = FRONT_Y - 0.10                                # the glass plane (49.40)
TOPZ = OPEN_TOP / 100                              # 4.50
CZ = CEIL / 100                                    # 5.20
DX0, DX1 = -DOOR_X[0] / 100, -DOOR_X[1] / 100      # 46.40 .. 48.40
LEAF_X = (DX0 + 0.06, DX0 + 0.06 + 0.96)           # the leaf's opening (46.46 .. 47.42); hinged on the west jamb
DOOR_H = 2.29
BASE_Z = 0.45                                      # the storefront's base (a painted steel curb)
TRANSOM_Z = 3.05                                   # the transom bar between the vision glass and the upper sash
SIGN_Z = (3.72, 4.50)                              # the sign band in the opening's top
ROLL_X = (61.0, 64.6)                              # the roll-up door (closed) at the east end
ROLL_TOP = 3.40


# ================================================================== equipment (PROP: front toward -X, origin on the floor)

def holes_panel(n, label=True):
    """A strip of an upright's punched holes (numbered), 75 mm wide, drawn for the decal on its front face."""
    def draw(s, w, h):
        s.rect(0, 0, w, h, BLACK, rough=0.5)
        pitch = 50.8
        for i in range(n):
            y = 40 + i * pitch
            if y > h - 30:
                break
            s.circle(w / 2, y, 11.0, 0x050505, rough=0.8, n=24)
            s.ring(w / 2, y, 11.0, 12.6, 0x6a6c70, metal=0.9, rough=0.35, n=24)
            if label and i % 2 == 0:
                s.mid(str(i // 2 + 1), w * 0.17, y, 10, 14, 0xd8d6d0, 'Bold')
    return draw


# ------------------------------------------------------------------ the treadmill

TM_L, TM_W, TM_DECK = 2.02, 0.86, 0.22
TM_BELT = (-0.62, 0.86, 0.255)     # the belt's run (x0, x1) and half width


def belt_texture():
    """T_NS_GymBelt: the running belt, tileable along V (the game pans it): black rubber with a fine diamond grip, the
    splice across it, the sides worn smooth where feet land."""
    W, Hh = 256, 2048
    u = (np.arange(W) + 0.5) / W
    v = (np.arange(Hh) + 0.5) / Hh
    U, V = np.meshgrid(u, v)
    P = np.stack([U * 0.51 * 40, V * 3.2 * 40, np.zeros_like(U)], -1).reshape(-1, 3)
    n = texmaps.value_noise(P, 0.4, seed=4, octaves=2).reshape(Hh, W)
    a = np.abs(((U * 0.51 + V * 3.2) / 0.004) % 1.0 - 0.5)
    b = np.abs(((U * 0.51 - V * 3.2) / 0.004) % 1.0 - 0.5)
    grip = np.minimum(a, b)
    lum = 0.035 + 0.012 * (grip < 0.12) + 0.01 * (n - 0.5)
    worn = np.exp(-((U - 0.5) / 0.22) ** 2) * 0.012
    lum = lum + worn
    splice = np.abs(V - 0.37) < 0.0016
    lum = np.where(splice, 0.08, lum)
    rgb = np.stack([lum, lum, lum * 1.02], -1)
    return texmaps.save('T_NS_GymBelt', rgb, 'sRGB')


def console_picture():
    """The treadmill console's screen until the game draws its own: speed, incline, time, a track loop."""
    s = Sheet(800.0, 520.0)
    sub = kit.Sub(s, 0.0, 0.0, 1.0)
    sub.rect(0, 0, 800, 520, 0x061018)
    sub.rect(0, 440, 800, 80, 0x0c2236)
    sub.mid('QUICK START', 400, 480, 30, 500, 0xe8eef4, 'Black', tracking=1.2)
    for i, (lab, val) in enumerate((('SPEED', '6.0'), ('INCLINE', '1.5'), ('TIME', '12:40'))):
        x = 140 + i * 260
        sub.mid(lab, x, 380, 22, 220, 0x7fb6e0, 'Bold')
        sub.mid(val, x, 300, 70, 230, 0xffffff, 'Black')
    sub.ring(400, 120, 70, 80, 0x2d5f86, n=80)
    sub.ring(400, 120, 70, 80, 0x7fe0a0, n=80, a0=math.pi * 0.5, a1=math.pi * 1.55)
    sub.mid('0.84 MI', 400, 120, 22, 110, 0xe8eef4, 'Bold')
    return render_sheet(s, 'gyt_console', 640, samples=8)


def draw_tm_buttons(s, w, h):
    s.rect(0, 0, w, h, 0x1d2024, rough=0.5)
    for i, (lab, col) in enumerate((('START', 0x2f9a4a), ('STOP', 0xc0261c))):
        s.circle(w * (0.2 + 0.6 * i), h * 0.5, h * 0.3, col, rough=0.4)
        s.mid(lab, w * (0.2 + 0.6 * i), h * 0.5, h * 0.16, h * 0.5, 0xffffff, 'Black')
    for j, lab in enumerate(('+', '-')):
        s.rect(w * 0.42, h * (0.56 - 0.4 * j), w * 0.16, h * 0.3, 0x3a3f46, rough=0.4)
        s.mid(lab, w * 0.5, h * (0.71 - 0.4 * j), h * 0.25, w * 0.14, 0xffffff, 'Black')


def draw_hood(s, w, h):
    s.rect(0, 0, w, h, 0x26292d, rough=0.45)
    s.mid('KORVANE', w * 0.5, h * 0.62, h * 0.34, w * 0.8, 0xc9ccd0, 'Black', tracking=1.35, metal=0.6, rough=0.35)
    s.mid('COMMERCIAL  ·  T9', w * 0.5, h * 0.24, h * 0.14, w * 0.8, IRON_RED, 'Bold', tracking=1.3)


def treadmill():
    """SM_NS_Gym_Treadmill: a commercial treadmill (the runner faces -X, the console at the -X end); the belt's top is
    [runtime] ns_belt (UV v along the run, so the game pans V), the console's screen [screen] ns_screen_console."""
    k = Kit('gyt', 'SM_NS_Gym_Treadmill', TEXTURE_SIZES, dust=0.1, low_grime=0.1, module=MODULE)
    x0, x1, hw = TM_BELT
    # the deck frame: side rails in aluminum with grip strips, the rear roller covers, the motor hood
    for s_ in (-1, 1):
        k.add(bx('rail', -0.66, x1 + 0.1, s_ * (hw + 0.005), s_ * (hw + 0.12), 0.12, TM_DECK + 0.02, 0.01), 'alu', 0xffffff)
        k.add(bx('grip', -0.6, x1 + 0.04, s_ * (hw + 0.02), s_ * (hw + 0.11), TM_DECK + 0.02, TM_DECK + 0.023, 0.002), 'rubber', 0x18181a)
        k.add(bx('frame', -0.9, x1 + 0.12, s_ * (hw + 0.03), s_ * (hw + 0.09), 0.04, 0.13, 0.01), 'powder', BLACK)
        k.add(bx('endcap', x1 + 0.08, x1 + 0.16, s_ * (hw + 0.0), s_ * (hw + 0.13), 0.04, TM_DECK + 0.02, 0.02), 'plastic', 0x2a2c30, var=0.1)
        for xx in (-0.85, x1 + 0.08):
            k.add(lathe_axis('foot', [(0.0, 0.0), (0.03, 0.0), (0.028, 0.04), (0.0, 0.04)], M(xx, s_ * (hw + 0.06), 0.0), 16), 'rubber', 0x111111)
    k.add(bx('deck', x0 - 0.02, x1 + 0.02, -hw - 0.005, hw + 0.005, 0.15, TM_DECK - 0.004, 0.004), 'particle', 0xffffff)
    k.add(xf(rod('roller_r', (x1 + 0.02, -hw, TM_DECK - 0.03), (x1 + 0.02, hw, TM_DECK - 0.03), 0.033, 24), Matrix()), 'steel', 0xffffff)
    k.add(rod('roller_f', (x0 - 0.0, -hw, TM_DECK - 0.03), (x0 - 0.0, hw, TM_DECK - 0.03), 0.033, 24), 'steel', 0xffffff)
    hood = soft_slab('hood', 0.44, 0.80, 0.30, 0.05, 10, 16, 6, matrix=M(-0.80, 0.0, 0.035))
    for v in hood.data.vertices:          # the cover slopes down toward the belt
        t = min(1.0, max(0.0, (v.co.x + 1.02) / 0.44))
        if v.co.z > 0.12:
            v.co.z -= 0.09 * t * t * min(1.0, (v.co.z - 0.12) / 0.2)
    k.add(hood, 'plastic', 0x26292d, var=0.15)
    for s_ in (-1, 1):
        for i in range(6):
            zv = 0.10 + i * 0.022
            k.add(bx('vent', -0.95 + i * 0.0, -0.70, s_ * 0.401 - 0.002, s_ * 0.401 + 0.002, zv, zv + 0.008, 0.002), 'plastic', 0x101112)
    k.panel('hood', 0.34, 0.07, draw_hood)
    k.decal('hood', (-1.0205, 0.0, 0.22), u=(0, -1, 0), v=(0, 0, 1), lift=0.0)
    # the belt: its top run as [runtime], the wrap round the rollers in the body
    belt = bmesh.new()
    uvl = belt.loops.layers.uv.new('UVMap')
    L = x1 - x0
    nseg = 12
    vs = [[belt.verts.new((x0 + L * i / nseg, y, TM_DECK)) for y in (-hw, hw)] for i in range(nseg + 1)]
    for i in range(nseg):
        f = belt.faces.new((vs[i][0], vs[i + 1][0], vs[i + 1][1], vs[i][1]))
        for loop, (a, b) in zip(f.loops, ((0, i), (0, i + 1), (1, i + 1), (1, i))):
            loop[uvl].uv = (a, b / nseg * (L / 1.6))
    bobj = core.mesh_object('belt_top', belt)
    k.special(bobj, image_mat('ns_belt', belt_texture(), rough=0.7))
    for xx in (x0, x1):
        wrap = core.lathe('belt_wrap', [(0.036, -hw), (0.036, hw)], 24)
        wrap.data.transform(M(xx, 0.0, TM_DECK - 0.036) @ Matrix.Rotation(math.radians(90), 4, 'X'))
        k.add(wrap, 'rubber', 0x121212)
    k.add(bx('belt_under', x0, x1, -hw, hw, TM_DECK - 0.075, TM_DECK - 0.071, 0.0), 'rubber', 0x121212)
    # uprights from the hood to the console, the handrails, the console
    for s_ in (-1, 1):
        up = bend([(-0.86, s_ * 0.36, 0.2), (-0.98, s_ * 0.37, 0.8), (-1.0, s_ * 0.37, 1.18)], 0.2)
        k.add(core.sweep('upright', up, [(-0.035, -0.022), (0.035, -0.022), (0.035, 0.022), (-0.035, 0.022)]), 'powder', BLACK)
        rail = bend([(-1.0, s_ * 0.37, 1.18), (-0.86, s_ * 0.40, 1.03), (-0.45, s_ * 0.40, 1.0), (-0.3, s_ * 0.40, 0.98)], 0.06)
        k.add(tube('handrail', rail, 0.017, 14), 'powder', BLACK)
        k.add(tube('grip', rail[3:], 0.021, 14), 'rubber', 0x1a1a1a)
        k.add(xf(lathe_axis('rail_cap', [(0.0, 0.0), (0.021, 0.0), (0.02, 0.012), (0.0, 0.016)], segments=14), M(-0.3, s_ * 0.40, 0.98, 0, -90)), 'plastic', 0x2a2c30)
        k.add(bx('pulse', -0.62, -0.5, s_ * 0.40 - 0.022, s_ * 0.40 + 0.022, 1.0, 1.03, 0.008), 'steel', 0xffffff)
    con = parts.panel('console', core.rounded_rect(-0.15, -0.42, 0.15, 0.42, 0.04, 4), -0.05, 0.05, 0.02)
    con_m = M(-1.02, 0.0, 1.32, 0, -25.0)
    k.add(xf(con, con_m @ Matrix.Rotation(math.radians(90), 4, 'Y')), 'plastic', 0x26292d, var=0.12)
    # the screen faces the runner (+X, tilted up): UV u to the runner's right (+Y), v up
    sc = quad('screen', [(0.0505, -0.125, -0.03), (0.0505, 0.125, -0.03), (0.0505, 0.125, 0.13), (0.0505, -0.125, 0.13)])
    sc.data.transform(con_m)
    k.special(sc, screen_mat('ns_screen_console', console_picture(), 1.6))
    k.add(xf(bx('bezel', 0.048, 0.0502, -0.135, 0.135, -0.04, 0.14, 0.004), con_m), 'gloss', 0x0b0c0e)
    k.panel('tmbuttons', 0.24, 0.07, draw_tm_buttons)
    btn_c = con_m @ Vector((0.052, 0.0, -0.10))
    k.decal('tmbuttons', tuple(btn_c), u=tuple(con_m.to_3x3() @ Vector((0, 1, 0))), v=tuple(con_m.to_3x3() @ Vector((0, 0, 1))), lift=0.001)
    # the safety key on its lanyard, the cup holders, the towel bar
    k.add(xf(bx('key', -0.01, 0.01, -0.02, 0.02, -0.02, 0.02, 0.006), con_m @ M(0.06, -0.3, -0.03)), 'plastic', 0xc0261c)
    k.add(tube('lanyard', [tuple(con_m @ Vector((0.07, -0.3, -0.05))), (-0.86, -0.33, 1.05), (-0.8, -0.28, 0.95), (-0.86, -0.25, 1.05)], 0.003, 6), 'cloth',
          IRON_RED)
    for s_ in (-1, 1):
        k.add(xf(lathe_axis('cup', [(0.0, 0.0), (0.04, 0.0), (0.045, 0.09), (0.043, 0.095), (0.0, 0.006)], segments=20), con_m @ M(0.05, s_ * 0.33, -0.12)),
              'plastic', 0x1a1c1f)
    k.add(rod('towelbar', (-0.92, -0.3, 1.12), (-0.92, 0.3, 1.12), 0.01, 12), 'steel', 0xffffff)
    k.polish += [((-0.4, 0.4, 1.0), 0.15, 0.7), ((-0.4, -0.4, 1.0), 0.15, 0.7)]
    k.grime += [((-0.55, 0.4, 1.0), 0.18, 0.4), ((-0.55, -0.4, 1.0), 0.18, 0.4), ((-1.0, 0.0, 1.25), 0.25, 0.3)]
    return [k.finish(angle=40.0, unbaked_order=['ns_belt', 'ns_screen_console'])]


# ------------------------------------------------------------------ the spin bike

def draw_bike_badge(s, w, h):
    s.rect(0, 0, w, h, 0x16181b, rough=0.4)
    s.mid('KORVANE', w / 2, h * 0.6, h * 0.42, w * 0.86, 0xd9dcdf, 'Black', tracking=1.3, metal=0.7, rough=0.35)
    s.mid('INDOOR CYCLE', w / 2, h * 0.2, h * 0.16, w * 0.8, IRON_RED, 'Bold', tracking=1.4)


BIKE_SEAT = 0.98      # the saddle's top


def bike():
    """SM_NS_Gym_Bike: an indoor cycle (the rider faces -X): a weighted flywheel under a shroud at the front, the frame,
    the crank and caged pedals, a saddle on its post (top BIKE_SEAT), multi-grip bars, a small console, the brake knob."""
    k = Kit('gyb', 'SM_NS_Gym_Bike', TEXTURE_SIZES, dust=0.1, low_grime=0.1, module=MODULE)
    sq = [(-0.035, -0.03), (0.035, -0.03), (0.035, 0.03), (-0.035, 0.03)]
    for xx in (-0.52, 0.48):
        k.add(bx('foot', xx - 0.04, xx + 0.04, -0.27, 0.27, 0.02, 0.08, 0.01), 'powder', BLACK, dir=1)
        for s_ in (-1, 1):
            ya, yb = sorted((s_ * 0.27, s_ * 0.30))
            k.add(bx('foot_cap', xx - 0.042, xx + 0.042, ya, yb, 0.0, 0.085, 0.012), 'rubber', 0x141414)
    k.add(core.sweep('seat_tube', [(0.48, 0.0, 0.08), (0.30, 0.0, 0.45), (0.12, 0.0, 0.80)], sq), 'powder', BLACK)
    k.add(core.sweep('down_tube', [(-0.52, 0.0, 0.08), (-0.25, 0.0, 0.38), (0.2, 0.0, 0.45)], sq), 'powder', BLACK)
    k.add(core.sweep('head_tube', [(-0.32, 0.0, 0.40), (-0.38, 0.0, 0.85), (-0.40, 0.0, 0.95)], sq), 'powder', BLACK)
    # the flywheel: a chrome disc under a red shroud, its axle at the front fork
    fw_c = Vector((-0.42, 0.0, 0.34))
    k.add(xf(lathe_axis('flywheel', [(0.0, -0.022), (0.24, -0.022), (0.255, -0.012), (0.255, 0.012), (0.24, 0.022), (0.0, 0.022)], segments=64),
             Matrix.Translation(fw_c) @ Matrix.Rotation(math.radians(90), 4, 'X')), 'chrome', var=0.2)
    arc = [(-0.27 * math.cos(a), 0.27 * math.sin(a)) for a in np.linspace(0.0, math.pi * 0.62, 20)] + [(0.0, 0.0)]
    if kit._area2(arc) < 0:
        arc = list(reversed(arc))
    shroud = parts.panel('shroud', arc, -0.04, 0.04, 0.008)
    shroud.data.transform(Matrix.Translation(fw_c) @ Matrix(((1, 0, 0, 0), (0, 0, -1, 0), (0, 1, 0, 0), (0, 0, 0, 1))))
    k.add(shroud, 'plastic', IRON_RED, var=0.25)
    for s_ in (-1, 1):
        k.add(bx('fork', fw_c.x - 0.03, fw_c.x + 0.03, s_ * 0.05 - 0.006, s_ * 0.05 + 0.006, 0.08, fw_c.z + 0.04, 0.004), 'powder', BLACK)
    k.add(rod('axle', (fw_c.x, -0.065, fw_c.z), (fw_c.x, 0.065, fw_c.z), 0.012, 16), 'steel', 0xffffff)
    # crank, pedals with cages, the belt guard
    cr = Vector((-0.05, 0.0, 0.33))
    k.add(xf(lathe_axis('guard', [(0.0, -0.03), (0.13, -0.03), (0.13, 0.03), (0.0, 0.03)], segments=40),
             Matrix.Translation(cr) @ Matrix.Rotation(math.radians(90), 4, 'X')), 'plastic', 0x1f2124, var=0.1)
    k.add(core.sweep('belt_guard', [(cr.x, 0.04, cr.z + 0.1), (fw_c.x, 0.04, fw_c.z + 0.08)], [(-0.02, -0.06), (0.02, -0.06), (0.02, 0.06), (-0.02, 0.06)]),
          'plastic', 0x1f2124)
    for s_, ang in ((-1, 30.0), (1, 210.0)):
        tip = cr + Vector((0.17 * math.cos(math.radians(ang)), s_ * 0.075, 0.17 * math.sin(math.radians(ang))))
        k.add(core.sweep('crank', [(cr.x, s_ * 0.07, cr.z), (tip.x, s_ * 0.075, tip.z)], [(-0.012, -0.02), (0.012, -0.02), (0.012, 0.02), (-0.012, 0.02)]),
              'alu', 0xffffff)
        py = tip.y + s_ * 0.02
        k.add(bx('pedal', tip.x - 0.055, tip.x + 0.055, py - 0.045, py + 0.045, tip.z - 0.012, tip.z + 0.012, 0.006), 'plastic', 0x161616)
        k.add(tube('cage', bend([(tip.x - 0.05, py - 0.04, tip.z + 0.012), (tip.x - 0.07, py, tip.z + 0.06), (tip.x + 0.02, py, tip.z + 0.07),
                                (tip.x + 0.02, py + 0.04, tip.z + 0.012)], 0.02), 0.006, 8), 'plastic', 0x161616)
    # the saddle on its post and slider
    k.add(rod('seat_post', (0.12, 0.0, 0.75), (0.15, 0.0, BIKE_SEAT - 0.06), 0.022, 16), 'chrome', var=0.3)
    k.add(bx('slider', 0.06, 0.24, -0.02, 0.02, BIKE_SEAT - 0.07, BIKE_SEAT - 0.05, 0.006), 'alu', 0xffffff)
    saddle = soft_slab('saddle', 0.27, 0.16, 0.06, 0.025, 8, 6, 3, sag=0.008, matrix=M(0.16, 0.0, BIKE_SEAT - 0.06))
    for v in saddle.data.vertices:
        t = (v.co.x - 0.16 + 0.135) / 0.27
        v.co.y *= 0.35 + 0.65 * min(1.0, max(0.0, t) * 1.6)
    k.add(saddle, 'vinyl', 0x161616)
    k.add(xf(lathe_axis('knob', [(0.0, 0.0), (0.02, 0.0), (0.022, 0.03), (0.0, 0.032)], segments=14), M(0.14, 0.0, 0.72, 0, 90)), 'plastic', IRON_RED)
    # bars: the stem up to a multi-position bar, the console
    k.add(rod('stem', (-0.40, 0.0, 0.95), (-0.42, 0.0, 1.10), 0.02, 14), 'chrome', var=0.3)
    bar = bend([(-0.22, -0.20, 1.06), (-0.40, -0.21, 1.10), (-0.48, -0.10, 1.12), (-0.48, 0.10, 1.12), (-0.40, 0.21, 1.10), (-0.22, 0.20, 1.06)], 0.06)
    k.add(tube('bar', bar, 0.014, 12), 'powder', BLACK)
    k.add(tube('bar_grip', bar[:6], 0.018, 12), 'foam', 0x1a1a1a)
    k.add(tube('bar_grip2', bar[-6:], 0.018, 12), 'foam', 0x1a1a1a)
    k.add(xf(bx('console', -0.03, 0.03, -0.08, 0.08, -0.05, 0.05, 0.01), M(-0.47, 0.0, 1.16, 0, -30)), 'plastic', 0x1f2124, var=0.1)
    k.add(xf(bx('console_lcd', 0.03, 0.032, -0.06, 0.06, -0.03, 0.035, 0.002), M(-0.47, 0.0, 1.16, 0, -30)), 'glassbaked', 0x203028)
    k.add(xf(lathe_axis('brake', [(0.0, 0.0), (0.03, 0.0), (0.032, 0.03), (0.0, 0.034)], segments=16), M(-0.33, 0.0, 0.64)), 'plastic', IRON_RED)
    k.add(rod('brake_shaft', (-0.33, 0.0, 0.5), (-0.33, 0.0, 0.64), 0.008, 10), 'chrome')
    k.panel('badge', 0.2, 0.06, draw_bike_badge)
    k.decal('badge', (-0.08, -0.0305, 0.415), u=(1, 0, 0), v=(0, 0, 1), lift=0.0)      # the -Y side, facing -Y
    k.decal('badge', (-0.08, 0.0305, 0.415), u=(-1, 0, 0), v=(0, 0, 1), lift=0.0)      # the +Y side, facing +Y
    k.polish += [((-0.40, 0.0, 1.10), 0.25, 0.7), ((0.16, 0.0, BIKE_SEAT), 0.12, 0.6)]
    k.grime += [((-0.05, 0.0, 0.33), 0.25, 0.3)]
    return [k.finish(angle=40.0)]


# ------------------------------------------------------------------ the bench

BENCH_SEAT = 0.45


def draw_bench_logo(s, w, h):
    s.rect(0, 0, w, h, 0x141414, rough=0.55)
    s.mid('IRONHOUSE', w / 2, h / 2, h * 0.55, w * 0.9, IRON_RED, 'Black', tracking=1.25, rough=0.5)


def bench():
    """SM_NS_Gym_Bench: an adjustable bench laid flat (its seat end toward -X: someone sitting faces -X, seat top
    BENCH_SEAT), the steel spine on a front stabilizer and a rear one with wheels and a handle."""
    k = Kit('gyr', 'SM_NS_Gym_Bench', TEXTURE_SIZES, dust=0.08, low_grime=0.1, module=MODULE)
    k.add(bx('spine', -0.58, 0.62, -0.038, 0.038, 0.18, 0.28, 0.008), 'powder', BLACK, dir=0)
    for xx, w in ((-0.62, 0.48), (0.62, 0.42)):
        k.add(bx('stab', xx - 0.04, xx + 0.04, -w / 2, w / 2, 0.02, 0.09, 0.008), 'powder', BLACK, dir=1)
        k.add(bx('post', min(xx * 0.97, xx * 0.9) - 0.03, max(xx * 0.97, xx * 0.9) + 0.03, -0.03, 0.03, 0.08, 0.22, 0.006), 'powder', BLACK)
        for s_ in (-1, 1):
            if xx < 0:
                k.add(bx('foot', xx - 0.045, xx + 0.045, s_ * w / 2 - 0.04, s_ * w / 2 + 0.04, 0.0, 0.03, 0.008), 'rubber', 0x141414)
            else:
                k.add(xf(lathe_axis('wheel', [(0.0, -0.02), (0.04, -0.02), (0.045, 0.0), (0.04, 0.02), (0.0, 0.02)], segments=24),
                         M(xx + 0.06, s_ * (w / 2 + 0.03), 0.045) @ Matrix.Rotation(math.radians(90), 4, 'X')), 'rubber', 0x141414)
    k.add(tube('handle', bend([(0.66, -0.14, 0.12), (0.8, -0.14, 0.2), (0.8, 0.14, 0.2), (0.66, 0.14, 0.12)], 0.05), 0.014, 12), 'powder', BLACK)
    k.add(tube('handle_grip', [(0.8, -0.1, 0.2), (0.8, 0.1, 0.2)], 0.019, 12), 'foam', 0x1a1a1a)
    # the pads: a seat at the front, the back pad behind it, plywood under the vinyl, the adjuster ladder
    k.add(soft_slab('seat', 0.30, 0.30, 0.075, 0.03, 8, 8, 3, sag=0.006, matrix=M(-0.42, 0.0, BENCH_SEAT - 0.075)), 'vinyl', 0x141414)
    k.add(soft_slab('back', 0.78, 0.27, 0.075, 0.03, 16, 8, 3, sag=0.008, matrix=M(0.2, 0.0, BENCH_SEAT - 0.075)), 'vinyl', 0x141414)
    for xx, L in ((-0.42, 0.28), (0.2, 0.76)):
        k.add(bx('board', xx - L / 2, xx + L / 2, -0.12, 0.12, BENCH_SEAT - 0.095, BENCH_SEAT - 0.075, 0.004), 'plywood', 0x8a6a46)
        k.add(bx('mount', xx - L / 2 + 0.05, xx + L / 2 - 0.05, -0.05, 0.05, 0.28, BENCH_SEAT - 0.095, 0.004), 'powder', BLACK)
    k.add(bx('ladder', 0.0, 0.42, -0.05, 0.05, 0.26, 0.31, 0.004), 'powder', BLACK)
    k.add(rod('pin', (0.32, -0.08, 0.30), (0.32, 0.08, 0.30), 0.009, 10), 'chrome')
    k.panel('logo', 0.18, 0.04, draw_bench_logo)
    k.decal('logo', (0.38, 0.0, BENCH_SEAT + 0.0005), u=(1, 0, 0), v=(0, 1, 0), lift=0.0)
    k.polish += [((-0.42, 0.0, BENCH_SEAT), 0.18, 0.8), ((0.25, 0.0, BENCH_SEAT), 0.3, 0.6)]
    return [k.finish(angle=40.0)]


# ------------------------------------------------------------------ plates, the barbell, dumbbells

def draw_plate(label, ink=0xd9d6cf):
    def draw(s, w, h):
        s.rect(0, 0, w, h, 0x0e0e0f, rough=0.6)
        s.mid(label, w / 2, h / 2, h * 0.7, w * 0.95, ink, 'Black', tracking=1.1, rough=0.55)
    return draw


PLATES = {45: (0.2255, 0.058), 35: (0.2, 0.05), 25: (0.17, 0.045), 10: (0.125, 0.035), 5: (0.1, 0.03)}


def plate(k, center, normal_yaw, lb, label=True, seed=0.0, col=0x1a1b1d):
    """An iron plate (radius and thickness by weight) centered at center, its face (and the weight cast in it) toward the
    horizontal direction normal_yaw (degrees from +X): the hub collar, the recessed face, the raised rim."""
    R, T = PLATES[lb]
    prof = [(0.026, -T / 2), (R - 0.008, -T / 2), (R, -T / 2 + 0.006), (R, T / 2 - 0.006), (R - 0.008, T / 2), (R - 0.02, T / 2), (R - 0.025, T / 2 - 0.006),
            (0.06, T / 2 - 0.006), (0.05, T / 2 + 0.004), (0.026, T / 2 + 0.004)]
    # lathe +Z -> the face normal (horizontal, at normal_yaw)
    m = Matrix.Translation(center) @ Matrix.Rotation(math.radians(normal_yaw), 4, 'Z') @ Matrix.Rotation(math.radians(90), 4, 'Y')
    k.add(lathe_solid('plate', prof, m, 48), 'paint', col, var=seed)
    if label and lb >= 25:
        n = m.to_3x3() @ Vector((0, 0, 1))
        up = Vector((0.0, 0.0, 1.0))
        right = up.cross(n)
        c_ = Vector(center) + n * (T / 2 - 0.0055) + up * (R * 0.62)
        k.decal(f'plate{lb}', tuple(c_), u=tuple(right), v=tuple(up), w=0.11, h=0.026, lift=0.0005)


BAR_L, BAR_SHAFT = 2.2, 1.31


def barbell():
    """SM_NS_Gym_Barbell: a 20 kg bar (origin at its middle, along Y): the knurled shaft, collars, sleeves, end caps."""
    k = Kit('gyl', 'SM_NS_Gym_Barbell', TEXTURE_SIZES, dust=0.0, low_grime=0.0, module=MODULE)
    k.cavity_amt = 0.2
    hs = BAR_SHAFT / 2
    k.add(rod('shaft', (0.0, -hs, 0.0), (0.0, hs, 0.0), 0.014, 24), 'knurl', 0xffffff, dir=1)
    for s_ in (-1, 1):
        r_ = Matrix.Rotation(math.radians(-90 * s_), 4, 'X')
        k.add(xf(lathe_axis('collar', [(0.0, 0.0), (0.038, 0.0), (0.038, 0.035), (0.026, 0.04), (0.026, 0.07), (0.0, 0.07)], segments=32),
                 M(0.0, s_ * hs, 0.0) @ r_), 'chrome', var=0.3)
        k.add(xf(lathe_axis('sleeve', [(0.0, 0.07), (0.025, 0.07), (0.025, 0.445), (0.022, 0.448), (0.0, 0.448)], segments=32), M(0.0, s_ * hs, 0.0) @ r_),
              'steel', 0xffffff, dir=1)
    return [k.finish(angle=40.0)]


def draw_db(s, w, h):
    s.rect(0, 0, w, h, 0x101011, rough=0.7)
    s.mid('25', w / 2, h * 0.58, h * 0.48, w * 0.7, 0xd9d6cf, 'Black', rough=0.6)
    s.mid('LB', w / 2, h * 0.24, h * 0.18, w * 0.5, 0xd9d6cf, 'Black', tracking=1.3, rough=0.6)


def dumbbell():
    """SM_NS_Gym_Dumbbell: a 25 lb rubber hex dumbbell (origin at the handle's middle, the handle along Y): chrome knurled
    handle, rubber-coated hex heads with the weight molded in. Instance it at the rack's DB points, scaled for the
    others (0.8 for 10-15 lb .. 1.35 for 50 lb)."""
    k = Kit('gyd', 'SM_NS_Gym_Dumbbell', TEXTURE_SIZES, dust=0.0, low_grime=0.0, module=MODULE)
    k.add(rod('handle', (0.0, -0.068, 0.0), (0.0, 0.068, 0.0), 0.0165, 20), 'knurl', 0xffffff, dir=1)
    k.panel('db', 0.06, 0.06, draw_db)
    r = 0.062
    hexo_pts = [(r * math.cos(math.radians(30 + 60 * i)), r * math.sin(math.radians(30 + 60 * i))) for i in range(6)]
    for s_ in (-1, 1):
        hexo = parts.panel('head', hexo_pts, 0.0, 0.078, 0.008)
        hexo.data.transform(M(0.0, s_ * 0.068, 0.0) @ Matrix.Rotation(math.radians(-90 * s_), 4, 'X'))
        k.add(hexo, 'rubber', 0x121213)
        k.add(xf(lathe_axis('ring', [(0.0, -0.004), (0.022, -0.004), (0.022, 0.004), (0.0, 0.004)], segments=20),
                 M(0.0, s_ * 0.066, 0.0) @ Matrix.Rotation(math.radians(90), 4, 'X')), 'chrome', var=0.2)
        # the end face reads from outside: a viewer at +Y facing -Y has +X... at s_ = 1 the right is -X
        k.decal('db', (0.0, s_ * (0.068 + 0.0785), 0.0), u=(-s_ * 1.0, 0, 0), v=(0, 0, 1), w=0.05, h=0.05, lift=0.0)
    return [k.finish(angle=40.0)]


DB_TIERS = ((0.50, 0.24), (0.86, -0.02))     # (z, x) of each tier's saddle line
DB_YS = [(-1.05 + 0.2333 * i) for i in range(10)]


def dumbbell_points():
    """DB0..DB19: the dumbbells' places on the rack (lower tier first, -Y to +Y): (x, y, z, yaw, pitch) of a dumbbell's
    origin; at yaw 90 its handle runs front-to-back across the saddles, pitch tilts it with the tier."""
    return [(x, y, z + 0.075, 90.0, 15.0) for (z, x) in DB_TIERS for y in DB_YS]


def dumbbell_rack():
    """SM_NS_Gym_DumbbellRack: a two-tier saddle rack, 2.4 m (front toward -X, origin on the floor at its middle)."""
    k = Kit('gyk', 'SM_NS_Gym_DumbbellRack', TEXTURE_SIZES, dust=0.1, low_grime=0.1, module=MODULE)
    sq = [(-0.04, -0.03), (0.04, -0.03), (0.04, 0.03), (-0.04, 0.03)]
    for yy in (-1.2, 0.0, 1.2):
        k.add(bx('foot', -0.42, 0.42, yy - 0.04, yy + 0.04, 0.0, 0.06, 0.008), 'powder', BLACK)
        k.add(core.sweep('upright', [(0.2, yy, 0.06), (0.06, yy, 0.95)], sq), 'powder', BLACK)
        for (z, x) in DB_TIERS:
            k.add(core.sweep('arm', [(x + 0.2, yy, z - 0.06), (x - 0.16, yy, z + 0.035)], [(-0.02, -0.03), (0.02, -0.03), (0.02, 0.03), (-0.02, 0.03)]),
                  'powder', BLACK)
        for xa, xb in ((-0.44, -0.38), (0.38, 0.44)):
            k.add(bx('foot_pad', xa, xb, yy - 0.045, yy + 0.045, 0.0, 0.065, 0.01), 'rubber', 0x111111)
    for (z, x) in DB_TIERS:
        for dx in (-0.1, 0.1):
            zr = z - dx * 0.27
            k.add(core.sweep('rail', [(x + dx, -1.24, zr), (x + dx, 1.24, zr)], [(-0.012, -0.025), (0.012, -0.025), (0.012, 0.025), (-0.012, 0.025)]),
                  'powder', BLACK, dir=1)
            for y in DB_YS:
                k.add(bx('saddle', x + dx - 0.03, x + dx + 0.03, y - 0.05, y + 0.05, zr + 0.012, zr + 0.03, 0.008), 'rubber', 0x1a1a1a)
    return [k.finish(angle=40.0)]


def plate_tree():
    """SM_NS_Gym_PlateTree: a plate tree (front toward -X; its horns point -Y and +Y), loaded with iron plates."""
    k = Kit('gyp', 'SM_NS_Gym_PlateTree', TEXTURE_SIZES, dust=0.1, low_grime=0.1, module=MODULE)
    for lb in (45, 35, 25):
        k.panel(f'plate{lb}', 0.12, 0.03, draw_plate(f'{lb} LB'))
    for s_ in (-1, 1):
        k.add(bx('foot', -0.3, 0.3, s_ * 0.06 - 0.04, s_ * 0.06 + 0.04, 0.0, 0.05, 0.008), 'powder', BLACK)
    k.add(bx('base', -0.05, 0.05, -0.35, 0.35, 0.0, 0.06, 0.008), 'powder', BLACK)
    k.add(bx('post', -0.04, 0.04, -0.04, 0.04, 0.06, 1.25, 0.008), 'powder', BLACK, dir=2)
    k.add(bx('cap', -0.045, 0.045, -0.045, 0.045, 1.25, 1.27, 0.008), 'plastic', 0x111111)
    loads = {(-1, 0.32): [45, 45, 45], (1, 0.32): [45, 45], (-1, 0.70): [35, 35], (1, 0.70): [25, 25, 25, 25], (-1, 1.02): [10, 10, 10], (1, 1.02): [5, 5]}
    rnd = random.Random(3)
    tilt = math.tan(math.radians(8))
    for (s_, z), stack in loads.items():
        k.add(rod('horn', (0.0, s_ * 0.04, z), (0.0, s_ * 0.27, z + 0.23 * tilt), 0.024, 20), 'powder', BLACK)
        y = s_ * 0.052
        for i, lb in enumerate(stack):
            R, T = PLATES[lb]
            y += s_ * T / 2
            plate(k, Vector((rnd.uniform(-0.004, 0.004), y, z + (abs(y) - 0.04) * tilt)), 90 * s_ + rnd.uniform(-1.5, 1.5), lb, label=(i == len(stack) - 1),
                  seed=rnd.random())
            y += s_ * (T / 2 + 0.003)
    k.grime += [((0.0, 0.0, 0.7), 0.4, 0.2)]
    return [k.finish(angle=40.0)]


# ------------------------------------------------------------------ the heavy bag

def cyl_patch(name, r, a0, a1, z0, z1, n=16):
    """A cylinder's patch about Z (radius r, angles a0..a1 degrees, z0..z1) facing out, UV 0..1 reading from outside."""
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new('UVMap')
    cols = []
    for i in range(n + 1):
        a = math.radians(a0 + (a1 - a0) * i / n)       # counter-clockwise: left to right seen from outside
        cols.append((bm.verts.new((r * math.cos(a), r * math.sin(a), z0)), bm.verts.new((r * math.cos(a), r * math.sin(a), z1))))
    for i in range(n):
        f = bm.faces.new((cols[i][0], cols[i + 1][0], cols[i + 1][1], cols[i][1]))
        for loop, uv in zip(f.loops, ((i / n, 0.0), ((i + 1) / n, 0.0), ((i + 1) / n, 1.0), (i / n, 1.0))):
            loop[uvl].uv = uv
    return core.mesh_object(name, bm)


def draw_bag_logo(s, w, h):
    s.rect(0, 0, w, h, 0x151515, rough=0.5)
    s.mid('IRONHOUSE', w / 2, h * 0.62, h * 0.42, w * 0.95, IRON_RED, 'Black', tracking=1.15, rough=0.45)
    s.mid('246 MARKET ST', w / 2, h * 0.2, h * 0.16, w * 0.7, 0xb8b4ac, 'Bold', tracking=1.4, rough=0.45)


BAG_TOP, BAG_BOT, BAG_R = -0.62, -1.82, 0.18


def heavy_bag():
    """SM_NS_Gym_HeavyBag: a 100 lb heavy bag hung by four chains from a swivel (origin at the hook's eye, where it hangs
    from the wall bracket; the game sways it about there): pebbled black leather, seams, a duct-tape repair, a bottom
    D-ring, the IRONHOUSE stencil toward -X."""
    k = Kit('gyh', 'SM_NS_Gym_HeavyBag', TEXTURE_SIZES, dust=0.05, low_grime=0.0, module=MODULE)
    k.add(lathe_axis('swivel', [(0.0, -0.02), (0.012, -0.02), (0.014, -0.05), (0.02, -0.055), (0.02, -0.09), (0.012, -0.095), (0.0, -0.095)], segments=16),
          'steel', 0xffffff)
    k.add(xf(core.lathe('eye', [(0.012, -0.003), (0.015, 0.0), (0.012, 0.003), (0.009, 0.0), (0.012, -0.003)], 20), M(0.0, 0.0, -0.005, 0, 0, 90)),
          'steel', 0xffffff)
    k.add(xf(core.lathe('ring', [(0.035, -0.005), (0.04, 0.0), (0.035, 0.005), (0.03, 0.0), (0.035, -0.005)], 24), M(0.0, 0.0, -0.13, 0, 0, 90)), 'steel', 0xffffff)
    for i in range(4):
        a = math.radians(45 + 90 * i)
        top = Vector((0.03 * math.cos(a), 0.03 * math.sin(a), -0.13))
        bot = Vector((0.13 * math.cos(a), 0.13 * math.sin(a), BAG_TOP + 0.03))
        d = bot - top
        L = d.length
        n_ = int(L / 0.031)
        links = []
        for j in range(n_):
            p = top + d * ((j + 0.5) / n_)
            m = Matrix.Translation(p) @ d.to_track_quat('Z', 'X').to_matrix().to_4x4() @ Matrix.Rotation(math.radians(90 * (j % 2)), 4, 'Z') \
                @ Matrix.Diagonal((1.0, 1.0, 1.45, 1.0)) @ Matrix.Rotation(math.radians(90), 4, 'X') @ Matrix.Diagonal((1.0, 1.0, 0.62, 1.0)) \
                @ Matrix.Rotation(math.radians(22.5), 4, 'Z')
            # long links (longer than the pitch, so each passes through the next): 8-sided turned flat-ended, so stretched
            # they read as straight-sided links; 31 mm pitch keeps the bag near the spec's 6k (+15%, inside its 25%)
            links.append(xf(core.lathe('link', [(0.0105, -0.003), (0.0135, 0.0), (0.0105, 0.003), (0.0075, 0.0), (0.0105, -0.003)], 8), m))
        k.add(core.join(links, 'chain'), 'steel', 0xffffff)
        k.add(xf(core.lathe('dring', [(0.016, -0.003), (0.019, 0.0), (0.016, 0.003), (0.013, 0.0), (0.016, -0.003)], 16),
                 M(bot.x, bot.y, bot.z, math.degrees(a), 0, 90)), 'steel', 0xffffff)
        k.add(xf(bx('tab', -0.012, 0.012, -0.02, 0.02, -0.05, 0.0, 0.004), M(0.14 * math.cos(a), 0.14 * math.sin(a), BAG_TOP + 0.02, math.degrees(a) + 90)),
              'leather', 0x161616)
    prof = [(0.0, BAG_TOP + 0.015), (0.12, BAG_TOP + 0.01), (0.17, BAG_TOP - 0.02), (BAG_R, BAG_TOP - 0.06), (BAG_R + 0.004, -1.2), (BAG_R + 0.012, -1.6),
            (BAG_R + 0.005, BAG_BOT + 0.06), (0.16, BAG_BOT + 0.02), (0.1, BAG_BOT), (0.0, BAG_BOT - 0.005)]
    k.add(lathe_axis('bag', prof, segments=48), 'leather', 0x161616)
    for i in range(4):
        a = math.radians(i * 90)
        seam = [(math.cos(a) * (rr + 0.002), math.sin(a) * (rr + 0.002), z) for rr, z in prof[3:7]]
        k.add(tube('seam', seam, 0.004, 6), 'leather', 0x101010)
    k.add(lathe_solid('tape', [(BAG_R + 0.0065, -1.30), (BAG_R + 0.0075, -1.30), (BAG_R + 0.0085, -1.37), (BAG_R + 0.0075, -1.37)], segments=48), 'galv',
          0xffffff, var=0.9)
    k.add(xf(core.lathe('bottom_ring', [(0.03, -0.004), (0.035, 0.0), (0.03, 0.004), (0.025, 0.0), (0.03, -0.004)], 20), M(0.0, 0.0, BAG_BOT - 0.03, 0, 0, 90)),
          'steel', 0xffffff)
    k.panel('baglogo', 0.36, 0.20, draw_bag_logo)
    patch = cyl_patch('logo', BAG_R + 0.0045, 180 - 33, 180 + 33, -1.10, -0.90, 16)
    k.uv_print(patch, 'baglogo')
    k.grime += [((-0.18, 0.0, -1.15), 0.3, 0.4)]
    k.polish += [((-0.18, 0.0, -1.05), 0.25, 0.6), ((0.0, -0.18, -1.05), 0.2, 0.4)]
    return [k.finish(angle=50.0)]


# ------------------------------------------------------------------ the front desk

DESK_L, DESK_D, LEDGE_Z, WORK_Z = 2.2, 0.75, 1.10, 0.76


def tablet_picture():
    s = Sheet(800.0, 560.0)
    sub = kit.Sub(s, 0.0, 0.0, 1.0)
    sub.rect(0, 0, 800, 560, 0x0f1114)
    sub.rect(0, 470, 800, 90, IRON_RED)
    sub.mid('IRONHOUSE', 400, 515, 50, 600, 0xffffff, 'Black', tracking=1.2)
    sub.mid('TAP YOUR FOB', 400, 360, 54, 700, 0xf2f2f2, 'Black')
    sub.mid('or sign in as a guest', 400, 290, 28, 600, 0x9aa3ad, 'Bold')
    sub.rect(250, 120, 300, 90, 0x2a2e33)
    sub.mid('GUEST', 400, 165, 36, 260, 0xffffff, 'Black', tracking=1.2)
    return render_sheet(s, 'gye_tablet', 640, samples=8)


def draw_fob(s, w, h):
    s.rect(0, 0, w, h, 0x111214, rough=0.35)
    s.circle(w * 0.5, h * 0.62, w * 0.24, 0x2a2d31, rough=0.3, n=40)
    for r in (0.08, 0.13, 0.18):
        s.ring(w * 0.5, h * 0.62, w * r, w * (r + 0.02), 0x8a9098, rough=0.35, n=40, a0=-0.7, a1=0.7)
    s.circle(w * 0.5, h * 0.18, w * 0.05, 0x3cff6a, rough=0.2, n=24)


def draw_desk_front(s, w, h):
    s.rect(0, 0, w, h, 0x9ea2a6, metal=1.0, rough=0.32)
    s.mid('IRONHOUSE', w / 2, h * 0.56, h * 0.36, w * 0.9, 0x1a1a1a, 'Black', tracking=1.18, rough=0.45)
    s.mid('24/7  ·  MEMBERS ANY HOUR', w / 2, h * 0.2, h * 0.12, w * 0.8, IRON_RED, 'Bold', tracking=1.3, rough=0.45)


def shaker(k, x, y, z, col, cap=0x111111):
    k.add(lathe_axis('shaker', [(0.0, 0.0), (0.042, 0.0), (0.045, 0.01), (0.046, 0.15), (0.043, 0.16), (0.0, 0.16)], M(x, y, z), 24), 'plastic', col, var=0.3)
    k.add(lathe_axis('shaker_cap', [(0.0, 0.16), (0.047, 0.16), (0.047, 0.19), (0.03, 0.2), (0.012, 0.215), (0.0, 0.216)], M(x, y, z), 24), 'plastic', cap)


def towel_stack(k, x, y, z, n=6, seed=1):
    rnd = random.Random(seed)
    for i in range(n):
        k.add(soft_slab('towel', 0.22, 0.30, 0.035, 0.014, 6, 8, 2, sag=0.004, seed=rnd.randint(1, 99), noise_amt=0.004,
                        matrix=M(x + rnd.uniform(-0.01, 0.01), y + rnd.uniform(-0.01, 0.01), z + i * 0.032, rnd.uniform(-4, 4))), 'cloth', 0xeeeeea)


def desk():
    """SM_NS_Gym_Desk: the front desk (customer side -X): a brushed-stainless front with the IRONHOUSE mark, a black
    transaction ledge at LEDGE_Z with the fob reader (its LED lit), the sign-in tablet on a stand ([screen]
    ns_screen_tablet, facing -X), shaker bottles and a stack of towels; behind, the work top with the shake blender, two
    protein tubs, a binder and a mug; towel bins under it."""
    k = Kit('gye', 'SM_NS_Gym_Desk', TEXTURE_SIZES, dust=0.06, low_grime=0.12, module=MODULE)
    hl, hd = DESK_L / 2, DESK_D / 2
    k.add(bx('kick', -hd + 0.05, hd, -hl + 0.03, hl - 0.03, 0.0, 0.10, 0.003), 'rubber', 0x121212)
    k.add(bx('front', -hd, -hd + 0.03, -hl, hl, 0.10, LEDGE_Z - 0.03, 0.004), 'steel', 0xffffff, dir=1)
    for s_ in (-1, 1):
        ya, yb = sorted((s_ * hl, s_ * (hl - 0.03)))
        k.add(bx('end', -hd, hd, ya, yb, 0.10, WORK_Z, 0.004), 'steel', 0xffffff, dir=0)
        k.add(bx('end_up', -hd, -0.1, ya, yb, WORK_Z, LEDGE_Z - 0.03, 0.004), 'steel', 0xffffff, dir=0)
    k.add(bx('ledge', -hd - 0.04, -0.08, -hl - 0.02, hl + 0.02, LEDGE_Z - 0.03, LEDGE_Z, 0.006), 'laminate', 0x1c1c1e, var=0.9)
    k.add(bx('work', -0.1, hd + 0.01, -hl + 0.02, hl - 0.02, WORK_Z - 0.03, WORK_Z, 0.006), 'laminate', 0x2a2a2c, var=0.9)
    k.add(bx('modesty', -0.12, -0.09, -hl + 0.03, hl - 0.03, WORK_Z, LEDGE_Z - 0.03, 0.003), 'laminate', 0x2a2a2c, var=0.9)
    k.add(bx('shelf', -0.08, hd - 0.02, -hl + 0.04, hl - 0.04, 0.12, 0.14, 0.003), 'plywood', 0x8a6a46)
    k.panel('front', 1.2, 0.3, draw_desk_front)
    k.decal('front', (-hd - 0.0005, 0.0, 0.70), u=(0, -1, 0), v=(0, 0, 1), lift=0.0)
    # towel bins under the work top
    for i, yy in enumerate((-0.6, 0.0, 0.6)):
        k.add(bx('bin', -0.02, 0.3, yy - 0.24, yy + 0.24, 0.14, 0.42, 0.01), 'plastic', 0x2a2c30, var=0.2)
        towel_stack(k, 0.14, yy, 0.42, 2, seed=i + 3)
    # the fob reader on the ledge, angled to the customer, its LED lit
    fr = M(-hd - 0.0, 0.62, LEDGE_Z, 0, 0, 0)
    k.add(xf(bx('fob', -0.06, 0.04, -0.045, 0.045, 0.0, 0.035, 0.008), fr), 'plastic', 0x16171a)
    k.panel('fob', 0.08, 0.08, draw_fob, glow=2.0)
    k.decal('fob', (-hd - 0.01, 0.62, LEDGE_Z + 0.0355), u=(0, -1, 0), v=(1, 0, 0), w=0.075, h=0.075, lift=0.0)
    # the sign-in tablet on a stand
    tm = M(-hd + 0.02, 0.15, LEDGE_Z, 0, 18, 0)
    k.add(lathe_axis('stand_base', [(0.0, 0.0), (0.07, 0.0), (0.068, 0.012), (0.0, 0.014)], M(-hd + 0.05, 0.15, LEDGE_Z), 24), 'alu', 0xffffff)
    k.add(rod('stand_arm', (-hd + 0.05, 0.15, LEDGE_Z + 0.01), (-hd + 0.03, 0.15, LEDGE_Z + 0.16), 0.01, 12), 'alu', 0xffffff)
    tab = M(-hd + 0.02, 0.15, LEDGE_Z + 0.2, 0, 22, 0)
    k.add(xf(bx('tablet', -0.006, 0.006, -0.125, 0.125, -0.09, 0.09, 0.008), tab), 'plastic', 0x0e0f11)
    scr = quad('tablet_screen', [(-0.0065, 0.115, -0.08), (-0.0065, -0.115, -0.08), (-0.0065, -0.115, 0.08), (-0.0065, 0.115, 0.08)])
    scr.data.transform(tab)
    k.special(scr, screen_mat('ns_screen_tablet', tablet_picture(), 1.4))
    # shakers, towels on the ledge's end
    for j, (yy, col) in enumerate(((-0.62, IRON_RED), (-0.52, 0x1a1a1a), (-0.42, 0x2a6ab0))):
        shaker(k, -hd + 0.06, yy, LEDGE_Z, col)
    towel_stack(k, -hd + 0.12, -0.92, LEDGE_Z, 6, seed=9)
    # behind: the blender, protein tubs, a binder, a mug
    k.add(bx('blender_base', 0.14, 0.30, 0.55, 0.71, WORK_Z, WORK_Z + 0.12, 0.02), 'plastic', 0x1a1a1c, var=0.2)
    k.add(lathe_axis('blender_jar', [(0.0, 0.0), (0.05, 0.0), (0.075, 0.2), (0.075, 0.23), (0.0, 0.23)], M(0.22, 0.63, WORK_Z + 0.12), 24), 'glassbaked',
          0x8a9a9a)
    k.add(lathe_axis('blender_lid', [(0.0, 0.23), (0.08, 0.23), (0.08, 0.255), (0.0, 0.26)], M(0.22, 0.63, WORK_Z + 0.12), 24), 'plastic', 0x111111)
    for j, (yy, col) in enumerate(((0.32, 0x1a1a1a), (0.12, IRON_RED))):
        k.add(lathe_axis('tub', [(0.0, 0.0), (0.09, 0.0), (0.095, 0.24), (0.0, 0.24)], M(0.24, yy, WORK_Z), 32), 'plastic', col, var=0.3)
        k.add(lathe_axis('tub_lid', [(0.0, 0.24), (0.1, 0.24), (0.1, 0.27), (0.0, 0.27)], M(0.24, yy, WORK_Z), 32), 'plastic', 0x111111)
    k.add(bx('binder', 0.0, 0.28, -0.5, -0.2, WORK_Z, WORK_Z + 0.045, 0.004), 'plastic', 0x1e3a6a)
    k.add(lathe_axis('mug', [(0.0, 0.0), (0.04, 0.0), (0.042, 0.095), (0.0, 0.095)], M(0.1, -0.75, WORK_Z), 24), 'gloss', 0xe8e4da)
    k.grime += [((-hd, 0.62, LEDGE_Z), 0.15, 0.6), ((-hd, 0.0, LEDGE_Z), 0.8, 0.25)]
    k.polish += [((-hd - 0.03, 0.0, LEDGE_Z), 0.9, 0.5)]
    return [k.finish(angle=40.0, unbaked_order=['ns_screen_tablet'])]


# ------------------------------------------------------------------ the cooler

CL_W, CL_D, CL_H = 0.78, 0.86, 2.04
COOLER_SHELF_Z = (0.28, 0.73, 1.18, 1.63)   # shelf tops, as the Lucky Penny's (SM_Stock_CoolerRow_* fit them)


def draw_cooler_header(s, w, h):
    s.rect(0, 0, w, h, 0xf3f6fa, rough=0.3)
    s.rect(0, 0, w, h * 0.16, 0x1d6fb8, rough=0.3)
    s.mid('ICE COLD', w / 2, h * 0.64, h * 0.42, w * 0.86, 0x1d6fb8, 'Black', tracking=1.15)
    s.mid('WATER  ·  ENERGY', w / 2, h * 0.27, h * 0.15, w * 0.8, 0x2a2a2a, 'Bold', tracking=1.3)


def draw_led(s, w, h):
    s.rect(0, 0, w, h, 0xf4f8ff, rough=0.2)


def cooler():
    """SM_NS_Gym_Cooler: a one-door glass merchandiser (front -X, origin on the floor at its middle): black cabinet, white
    lit interior, four wire shelves with price rails at COOLER_SHELF_Z (a SM_Stock_CoolerRow_* stands on a shelf at
    (0, 0, shelf top), yaw -90), LED strips, a lit header, the door ([glass]) on the left hinges, a pull, the
    compressor's louvers."""
    k = Kit('gyc', 'SM_NS_Gym_Cooler', TEXTURE_SIZES, dust=0.1, low_grime=0.15, module=MODULE)
    hw, hd = CL_W / 2, CL_D / 2
    t = 0.04
    # the cabinet: sides, top, back, the base with louvers
    for s_ in (-1, 1):
        ya, yb = sorted((s_ * hw, s_ * (hw - t)))
        k.add(bx('side', -hd + 0.06, hd, ya, yb, 0.0, CL_H, 0.006), 'enamel', 0x18191b, dir=2)
    k.add(bx('back', hd - t, hd, -hw, hw, 0.0, CL_H, 0.004), 'enamel', 0x18191b)
    k.add(bx('top', -hd + 0.06, hd, -hw, hw, CL_H - 0.04, CL_H, 0.006), 'enamel', 0x18191b)
    k.add(bx('base', -hd + 0.06, hd, -hw, hw, 0.0, 0.22, 0.004), 'enamel', 0x18191b)
    k.add(bx('louver_frame', -hd + 0.05, -hd + 0.07, -hw + 0.03, hw - 0.03, 0.03, 0.19, 0.004), 'plastic', 0x111111)
    for i in range(7):
        z = 0.05 + i * 0.02
        k.add(xf(bx('louver', -0.004, 0.004, -hw + 0.05, hw - 0.05, -0.006, 0.006, 0.002), M(-hd + 0.05, 0.0, z, 0, 0, 0) @ Matrix.Rotation(math.radians(35), 4, 'Y')),
              'plastic', 0x0c0c0c)
    # the interior: white liner, the shelves, the price rails, the LED strips
    ix0, ix1 = -hd + 0.10, hd - t
    k.add(bx('liner_back', ix1 - 0.01, ix1, -hw + t, hw - t, 0.22, CL_H - 0.18, 0.0), 'enamel', 0xeeeeea)
    for s_ in (-1, 1):
        ya, yb = sorted((s_ * (hw - t), s_ * (hw - t - 0.01)))
        k.add(bx('liner_side', ix0, ix1, ya, yb, 0.22, CL_H - 0.18, 0.0), 'enamel', 0xeeeeea)
    k.add(bx('liner_floor', ix0, ix1, -hw + t, hw - t, 0.22, 0.24, 0.0), 'enamel', 0xeeeeea)
    k.add(bx('liner_top', ix0, ix1, -hw + t, hw - t, CL_H - 0.20, CL_H - 0.18, 0.0), 'enamel', 0xeeeeea)
    for z in COOLER_SHELF_Z:
        for j in range(12):
            xx = ix0 + 0.03 + j * (ix1 - ix0 - 0.06) / 11
            k.add(rod('wire', (xx, -hw + t + 0.005, z - 0.004), (xx, hw - t - 0.005, z - 0.004), 0.0025, 6), 'chrome', var=0.3)
        for xx in (ix0 + 0.03, ix1 - 0.03):
            k.add(rod('wire_frame', (xx, -hw + t + 0.005, z - 0.008), (xx, hw - t - 0.005, z - 0.008), 0.004, 6), 'chrome', var=0.3)
        k.add(bx('rail', ix0 - 0.005, ix0 + 0.01, -hw + t + 0.005, hw - t - 0.005, z - 0.03, z + 0.008, 0.002), 'plastic', 0xf4f4f0, var=0.6)
    # the door: a black frame round the glass, a pull on the right, hinges on the left
    fx = -hd + 0.05
    dw = 0.035
    for (a, b, c_, d_) in ((-hw + 0.01, hw - 0.01, 0.22, 0.22 + dw), (-hw + 0.01, hw - 0.01, CL_H - 0.2 - dw, CL_H - 0.2), (-hw + 0.01, -hw + 0.01 + dw, 0.22, CL_H - 0.2),
                           (hw - 0.01 - dw, hw - 0.01, 0.22, CL_H - 0.2)):
        k.add(bx('dframe', fx - 0.03, fx + 0.01, a, b, c_, d_, 0.004), 'enamel', 0x101113, dir=1 if (b - a) > (d_ - c_) else 2)
    gl = quad('door_glass', [(fx - 0.012, hw - 0.01 - dw, 0.22 + dw), (fx - 0.012, -hw + 0.01 + dw, 0.22 + dw), (fx - 0.012, -hw + 0.01 + dw, CL_H - 0.2 - dw),
                             (fx - 0.012, hw - 0.01 - dw, CL_H - 0.2 - dw)])
    k.special(gl, UNB['ns_glass'])
    k.add(tube('pull', bend([(fx - 0.03, hw - 0.06, 0.9), (fx - 0.075, hw - 0.06, 0.92), (fx - 0.075, hw - 0.06, 1.38), (fx - 0.03, hw - 0.06, 1.40)], 0.02), 0.012, 12),
          'chrome', var=0.3)
    for z in (0.3, CL_H - 0.3):
        k.add(bx('hinge', fx - 0.04, fx - 0.02, -hw + 0.0, -hw + 0.03, z, z + 0.08, 0.004), 'chrome', var=0.4)
    k.panel('header', CL_W - 0.06, 0.15, draw_cooler_header, glow=2.2)
    k.decal('header', (-hd + 0.0595, 0.0, CL_H - 0.11), u=(0, -1, 0), v=(0, 0, 1), lift=0.0)
    k.add(bx('header_box', -hd + 0.06, -hd + 0.10, -hw, hw, CL_H - 0.20, CL_H - 0.02, 0.004), 'enamel', 0x18191b)
    k.panel('led', 0.012, 1.4, draw_led, glow=3.0)
    for s_ in (-1, 1):
        k.decal('led', (ix0 + 0.02, s_ * (hw - t - 0.0105), 1.0), u=(-1, 0, 0) if s_ < 0 else (1, 0, 0), v=(0, 0, 1), w=0.012, h=1.4, lift=0.0)
    k.grime += [((fx - 0.075, hw - 0.06, 1.15), 0.2, 0.6)]
    k.polish += [((fx - 0.075, hw - 0.06, 1.15), 0.12, 0.6)]
    return [k.finish(angle=40.0, unbaked_order=['ns_glass'])]


# ================================================================== the site (SITE frame: Blender x = -X/100)

SASH = 0x1d1e20          # the steel sash's black paint
BAND = 0x232527          # the sign band's charcoal
BAYS = [48.40 + i * 1.5625 for i in range(9)]      # the glazing bays' mullion lines from the door frame to the roll-up door
TREADMILLS = [49.6, 50.8, 52.0, 53.2, 54.4]       # treadmill centers (Blender x) at y TM_Y, facing the street
TM_Y = 47.25


def split_by_normal(obj, test):
    """Splits obj into (faces where test(normal) is true, the rest) as two objects."""
    a = obj.copy()
    a.data = obj.data.copy()
    core.link(a)
    for o, keep in ((a, True), (obj, False)):
        bm = bmesh.new()
        bm.from_mesh(o.data)
        bm.normal_update()
        bmesh.ops.delete(bm, geom=[f for f in bm.faces if bool(test(f.normal)) != keep], context='FACES')
        bm.to_mesh(o.data)
        bm.free()
    return a, obj


def street_text_matrix(origin):
    """Letters built in XY (reading +X, up +Y, front +Z) stood up on a north-facing front: reading toward -x (the street's
    right), up +z, front +y."""
    return Matrix.Translation(origin) @ Matrix(((-1, 0, 0, 0), (0, 0, 1, 0), (0, 1, 0, 0), (0, 0, 0, 1)))


def channel_letters(k, txt, size, origin, depth=0.10, face='Black', tracking=1.05):
    """Channel letters on the sign band: painted returns (baked, the body) and their faces (returned for _GlowCool)."""
    t = text_mesh('letters', txt, size, face, depth, 'CENTER', tracking, resolution=6)
    t.data.transform(street_text_matrix(origin))
    faces, rest = split_by_normal(t, lambda n: n.y > 0.7)
    backs, rest = split_by_normal(rest, lambda n: n.y < -0.7)   # drop the backs against the band
    bpy.data.objects.remove(backs)
    k.add(rest, 'paint', IRON_RED)
    faces.data.transform(Matrix.Translation((0.0, 0.003, 0.0)))
    return faces


def neon_text(txt, size, origin, r=0.006):
    """Neon tube lettering from the kit's marker strokes, stood in the street-facing plane at origin (reading for the
    street): returns the tubes as one object."""
    out = []
    cx = 0.0
    for ch in txt:
        if ch == ' ':
            cx += 0.3 * size
            continue
        g = kit._glyph(ch)
        if g is None:
            continue
        strokes, adv = g
        for st in strokes:
            pts = [(cx + px * size, py * size) for px, py in st]
            if len(pts) > 2:
                pts = [tuple(p) for p in core.catmull_rom(pts, 5)]
            out.append(tube('neon', [(p[0], p[1], 0.0) for p in pts], r, 8))
        cx += (adv + 0.08) * size
    o = core.join(out, 'neon') if len(out) > 1 else out[0]
    o.data.transform(street_text_matrix(origin) @ Matrix.Translation((-cx / 2, 0.0, 0.0)))
    return o


def draw_rollup(s, w, h):
    """The roll-up door's paint: IronHouse red with a white kettlebell and the name, weathered, a NO PARKING stencil."""
    rnd = random.Random(14)
    s.rect(0, 0, w, h, IRON_RED, rough=0.5)
    for i in range(40):
        s.rect(0, rnd.uniform(0, h), w, rnd.uniform(4, 20), rnd.choice((0xa8361c, 0xbc4426, 0x9c3018)), rough=0.5, alpha=rnd.uniform(0.2, 0.5))
    cx, cy = w / 2, h * 0.6
    s.circle(cx, cy, 520, 0xece8e0, rough=0.5, n=96)
    s.circle(cx, cy, 470, IRON_RED, rough=0.5, n=96)
    # a kettlebell
    s.circle(cx, cy - 60, 230, 0xece8e0, rough=0.5, n=80)
    s.ring(cx, cy + 190, 90, 150, 0xece8e0, rough=0.5, n=80, a0=-0.2, a1=math.pi + 0.2)
    s.rect(cx - 150, cy + 120, 60, 120, 0xece8e0, rough=0.5)
    s.rect(cx + 90, cy + 120, 60, 120, 0xece8e0, rough=0.5)
    s.mid('IRONHOUSE', cx, cy - 720, 300, w * 0.9, 0xece8e0, 'Black', tracking=1.15, rough=0.5)
    s.mid('24/7  ·  MEMBERS ANY HOUR', cx, cy - 1010, 120, w * 0.8, 0xece8e0, 'Bold', tracking=1.3, rough=0.5)
    s.mid('DOOR IN USE  ·  NO PARKING', cx, 240, 110, w * 0.7, 0xece8e0, 'Black', tracking=1.3, rough=0.5)
    for i in range(70):
        s.rect(rnd.uniform(0, w), rnd.uniform(0, h * 0.4), rnd.uniform(20, 160), rnd.uniform(3, 10), rnd.choice((0x5a2a1a, 0x8a8478, 0x2a2420)), rough=0.7,
               alpha=rnd.uniform(0.3, 0.8))
    for i in range(30):
        cx_, cy_ = rnd.uniform(0, w), rnd.uniform(0, h)
        r = rnd.uniform(10, 40)
        s.poly([(cx_ + r * rnd.uniform(0.5, 1.3) * math.cos(a), cy_ + r * rnd.uniform(0.4, 1.1) * math.sin(a)) for a in np.linspace(0, 2 * math.pi, 8, endpoint=False)],
               0x6a6a66, rough=0.8, alpha=0.8)


def draw_band_text(s, w, h):
    s.rect(0, 0, w, h, BAND, rough=0.6)
    s.mid('MEMBERS ANY HOUR', w / 2, h / 2, h * 0.62, w * 0.96, 0xd8d4cc, 'Black', tracking=1.35, rough=0.55)


def draw_fob_sign(s, w, h):
    s.rect(0, 0, w, h, 0x111214, rough=0.4)
    s.mid('MEMBERS', w / 2, h * 0.82, h * 0.1, w * 0.86, 0xffffff, 'Black', tracking=1.2)
    s.mid('TAP FOB', w / 2, h * 0.68, h * 0.1, w * 0.86, 0xffffff, 'Black', tracking=1.2)
    s.circle(w / 2, h * 0.42, w * 0.25, 0x2a2d31, rough=0.3, n=40)
    for r in (0.08, 0.14, 0.2):
        s.ring(w / 2, h * 0.42, w * r, w * (r + 0.025), 0x8a9098, rough=0.35, n=40, a0=-0.7, a1=0.7)
    s.circle(w / 2, h * 0.13, w * 0.07, 0x3cff6a, rough=0.2, n=24)


def front():
    """SM_NS_Gym_Front + _Glass + _GlowCool (the channel letters' faces) + _GlowNeon (the 24/7 neon in the window)."""
    k = Kit('gyf', 'SM_NS_Gym_Front', TEXTURE_SIZES, dust=0.08, low_grime=0.15, module=MODULE)
    k.cavity = 0.03
    gl, cool = [], []
    gy = GY
    # --- the base curb, the end jambs, the sign band and its cap
    k.add(bx('curb', OX0, OX1, gy - 0.12, FRONT_Y + 0.02, 0.0, BASE_Z, 0.006), 'paint', 0x2c2d2f)
    for xa, xb in ((OX0, OX0 + 0.10), (OX1 - 0.10, OX1)):
        k.add(bx('jamb', xa, xb, gy - 0.10, FRONT_Y + 0.02, 0.0, TOPZ, 0.006), 'powder', SASH, dir=2)
    k.add(bx('band', OX0, OX1, FRONT_Y - 0.12, FRONT_Y + 0.02, SIGN_Z[0], SIGN_Z[1], 0.004), 'paint', BAND)
    k.add(bx('band_cap', OX0, OX1, FRONT_Y - 0.12, FRONT_Y + 0.06, SIGN_Z[1] - 0.04, SIGN_Z[1], 0.006), 'powder', SASH)
    k.add(bx('band_sill', OX0, OX1, FRONT_Y - 0.12, FRONT_Y + 0.05, SIGN_Z[0] - 0.03, SIGN_Z[0] + 0.01, 0.006), 'powder', SASH)
    # --- the door frame: the leaf's opening, the fob mullion, a sidelite, the transom; a narrow sidelite west of it
    for xa, xb in ((DX0 - 0.0, DX0 + 0.06), (LEAF_X[1], LEAF_X[1] + 0.08), (DX1 - 0.06, DX1)):
        k.add(bx('dpost', xa, xb, gy - 0.10, gy + 0.05, BASE_Z * 0.0, TRANSOM_Z, 0.004), 'powder', SASH, dir=2)
    k.add(bx('dhead', DX0, DX1, gy - 0.10, gy + 0.05, DOOR_H, DOOR_H + 0.08, 0.004), 'powder', SASH)
    k.add(bx('threshold', LEAF_X[0], LEAF_X[1], gy - 0.12, gy + 0.08, 0.0, 0.015, 0.004), 'alu', 0xffffff)
    side0 = LEAF_X[1] + 0.08
    gl.append(quad('sidelite', [(DX1 - 0.06, gy, 0.05), (side0, gy, 0.05), (side0, gy, DOOR_H), (DX1 - 0.06, gy, DOOR_H)]))
    gl.append(quad('dtransom', [(DX1 - 0.06, gy, DOOR_H + 0.08), (DX0 + 0.06, gy, DOOR_H + 0.08), (DX0 + 0.06, gy, TRANSOM_Z), (DX1 - 0.06, gy, TRANSOM_Z)]))
    k.add(bx('kickpanel', side0, DX1 - 0.06, gy - 0.02, gy + 0.02, 0.0, 0.05, 0.002), 'powder', SASH)
    # west of the door: a narrow fixed panel on the curb
    k.add(bx('wpost', OX0 + 0.10, DX0, gy - 0.1, gy + 0.05, BASE_Z, BASE_Z + 0.05, 0.004), 'powder', SASH)
    gl.append(quad('wlite', [(DX0, gy, BASE_Z + 0.05), (OX0 + 0.10, gy, BASE_Z + 0.05), (OX0 + 0.10, gy, TRANSOM_Z), (DX0, gy, TRANSOM_Z)]))
    # the fob reader on the leaf's east jamb (street side), its LED lit; the members' sign over it
    fx = LEAF_X[1] + 0.04
    k.add(bx('fob', fx - 0.035, fx + 0.035, gy + 0.05, gy + 0.08, 1.12, 1.26, 0.008), 'plastic', 0x111214, var=0.1)
    k.panel('fobsign', 0.07, 0.14, draw_fob_sign, glow=1.5)
    k.decal('fobsign', (fx, gy + 0.0805, 1.19), u=(-1, 0, 0), v=(0, 0, 1), w=0.068, h=0.136, lift=0.0)
    # --- the glazing bays: steel mullions, the vision glass, the transom bar, the upper sash's small lights
    k.add(bx('transom', DX0, ROLL_X[0], gy - 0.10, gy + 0.06, TRANSOM_Z, TRANSOM_Z + 0.07, 0.004), 'powder', SASH)
    k.add(bx('sill', DX1, ROLL_X[0], gy - 0.06, gy + 0.10, BASE_Z, BASE_Z + 0.05, 0.004), 'powder', SASH)
    for i, xb_ in enumerate(BAYS):
        k.add(bx('mullion', xb_ - 0.03, xb_ + 0.03, gy - 0.14, gy + 0.05, BASE_Z, SIGN_Z[0], 0.004), 'powder', SASH, dir=2)
        if i < len(BAYS) - 1:
            xa, xb = xb_ + 0.03, BAYS[i + 1] - 0.03
            gl.append(quad('vision', [(xb, gy, BASE_Z + 0.05), (xa, gy, BASE_Z + 0.05), (xa, gy, TRANSOM_Z), (xb, gy, TRANSOM_Z)]))
    # the upper sash runs over the door too: 4 x 2 lights a bay in a T-bar grid
    sash_bays = [(OX0 + 0.10, DX0), (DX0, DX1)] + [(BAYS[i] + 0.03, BAYS[i + 1] - 0.03) for i in range(len(BAYS) - 1)]
    for xa, xb in sash_bays:
        z0, z1 = TRANSOM_Z + 0.07, SIGN_Z[0] - 0.03
        nx_ = max(1, int(round((xb - xa) / 0.38)))
        for j in range(nx_ + 1):
            x_ = xa + (xb - xa) * j / nx_
            k.add(bx('muntin', x_ - 0.012, x_ + 0.012, gy - 0.03, gy + 0.03, z0, z1, 0.002), 'powder', SASH, dir=2)
        for zz in (z0, (z0 + z1) / 2, z1):
            k.add(bx('muntin_h', xa, xb, gy - 0.03, gy + 0.03, zz - 0.012, zz + 0.012, 0.002), 'powder', SASH)
        for j in range(nx_):
            for zz0, zz1 in ((z0, (z0 + z1) / 2), ((z0 + z1) / 2, z1)):
                a_ = xa + (xb - xa) * j / nx_ + 0.012
                b_ = xa + (xb - xa) * (j + 1) / nx_ - 0.012
                gl.append(quad('sash_lite', [(b_, gy, zz0 + 0.012), (a_, gy, zz0 + 0.012), (a_, gy, zz1 - 0.012), (b_, gy, zz1 - 0.012)]))
    # --- the roll-up door, closed: slats, the bottom bar and its lock, the guides, the hood
    rx0, rx1 = ROLL_X
    k.add(bx('roll_pier_e', rx1 + 0.08, OX1 - 0.10, gy - 0.12, FRONT_Y + 0.02, 0.0, SIGN_Z[0], 0.004), 'powder', SASH)
    sl = 0.075
    k.panel('rollup', rx1 - rx0, ROLL_TOP - 0.06, draw_rollup, density=1.0)
    n_sl = int((ROLL_TOP - 0.06) / sl)
    slat_y = FRONT_Y - 0.08
    for i in range(n_sl):
        z = 0.06 + i * sl
        prof = [(0.0, 0.0), (0.012, 0.004), (0.016, sl * 0.5), (0.012, sl - 0.004), (0.0, sl), (-0.004, sl - 0.01), (-0.004, 0.01)]
        slat = parts.panel('slat', prof, rx0, rx1, 0.0)
        slat.data.transform(Matrix(((0, 0, 1, 0), (1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 0, 1))))
        slat.data.transform(Matrix.Translation((0.0, slat_y, z)))
        face, back = split_by_normal(slat, lambda n: n.y > 0.2)     # the paint job on the street face only
        k.locked_print(face, 'rollup', lambda co: ((rx1 - co.x) / (rx1 - rx0), (co.z - 0.06) / (ROLL_TOP - 0.06)))
        k.add(back, 'paint', 0x8a2e18)
    k.add(bx('bottom_bar', rx0, rx1, slat_y - 0.02, slat_y + 0.03, 0.0, 0.07, 0.006), 'galv', 0xffffff)
    k.add(bx('lock', (rx0 + rx1) / 2 - 0.05, (rx0 + rx1) / 2 + 0.05, slat_y + 0.03, slat_y + 0.05, 0.12, 0.2, 0.006), 'steel', 0xffffff)
    k.add(tube('handle', bend([((rx0 + rx1) / 2 - 0.4, slat_y + 0.03, 0.10), ((rx0 + rx1) / 2 - 0.4, slat_y + 0.09, 0.12), ((rx0 + rx1) / 2 - 0.2, slat_y + 0.09, 0.12),
                               ((rx0 + rx1) / 2 - 0.2, slat_y + 0.03, 0.10)], 0.02), 0.01, 10), 'galv', 0xffffff)
    for xa, xb in ((rx0 - 0.08, rx0), (rx1, rx1 + 0.08)):
        k.add(bx('guide', xa, xb, slat_y - 0.05, slat_y + 0.06, 0.0, ROLL_TOP + 0.02, 0.004), 'galv', 0xffffff, dir=2)
    k.add(bx('hood', rx0 - 0.1, rx1 + 0.1, slat_y - 0.30, slat_y + 0.08, ROLL_TOP, SIGN_Z[0] - 0.02, 0.01), 'powder', SASH)
    # --- channel letters IRONHOUSE and 24/7 on the band, MEMBERS ANY HOUR painted beside
    letter_z = SIGN_Z[0] + 0.12
    cool.append(channel_letters(k, 'IRONHOUSE', 0.80, ((DX1 + ROLL_X[0]) / 2 - 0.4, FRONT_Y + 0.02, letter_z)))
    cool.append(channel_letters(k, '24/7', 0.80, ((ROLL_X[0] + ROLL_X[1]) / 2, FRONT_Y + 0.02, letter_z)))
    k.panel('bandtext', 4.2, 0.34, draw_band_text)
    k.decal('bandtext', (48.75, FRONT_Y + 0.0205, (SIGN_Z[0] + SIGN_Z[1]) / 2), u=(-1, 0, 0), v=(0, 0, 1), lift=0.0)
    for xr in (49.0, 51.4, 53.8, 56.2, 58.6, 62.7):
        k.add(bx('raceway', xr - 0.05, xr + 0.05, FRONT_Y + 0.02, FRONT_Y + 0.06, SIGN_Z[0] + 0.06, SIGN_Z[0] + 0.14, 0.006), 'paint', BAND)
    # --- the neon 24/7 sign hung in the window's first bay, on a clear acrylic back with its transformer
    nx = (BAYS[0] + BAYS[1]) / 2
    neon = neon_text('24/7', 0.32, (nx, gy - 0.10, 2.38), r=0.0065)
    k.add(bx('neon_back', nx - 0.48, nx + 0.48, gy - 0.135, gy - 0.125, 2.3, 2.78, 0.006), 'glassbaked', 0x202428)
    k.add(bx('xformer', nx - 0.1, nx + 0.1, gy - 0.16, gy - 0.135, 2.32, 2.4, 0.006), 'powder', 0x111111)
    for s_ in (-1, 1):
        k.add(rod('neon_chain', (nx + s_ * 0.4, gy - 0.13, 2.78), (nx + s_ * 0.4, gy - 0.13, TRANSOM_Z), 0.002, 6), 'chrome', var=0.5)
    # --- the hours on the glass by the door: black vinyl, white letters (spec 4.9)
    k.grime += [((LEAF_X[1], gy, 1.1), 0.5, 0.5), ((rx0 + 0.5, slat_y, 0.3), 1.2, 0.4)]
    obj = k.finish(angle=40.0)
    gobj = core.join(gl, 'SM_NS_Gym_Front_Glass')
    gobj.name = gobj.data.name = 'SM_NS_Gym_Front_Glass'
    core.assign(gobj, UNB['ns_glass'])
    gobj['ns_unbaked'] = True
    print(f'[{MODULE}] SM_NS_Gym_Front_Glass: {sum(len(p.vertices) - 2 for p in gobj.data.polygons)} triangles, slots [\'ns_glass\']')
    return [obj, gobj, glow(cool, 'SM_NS_Gym_Front_GlowCool', MODULE), glow([neon], 'SM_NS_Gym_Front_GlowNeon', MODULE)]


# ------------------------------------------------------------------ the room

TRUSS_X = [49.0, 52.3, 55.6, 58.9, 62.2]
HIGHBAYS = [(x, y) for x in (48.8, 53.7, 58.6, 63.5) for y in (44.6, 39.8)]
BAG_HOOKS = [(IX1 - 0.78, y, 2.47) for y in (45.8, 43.5, 41.2)]   # the heavy bags' hook eyes (Blender), on the east wall's brackets
MIRROR_X = (49.6, 64.0)
WALL_HI = 0xe9e7e1
WALL_LO = 0x2a2b2d


def logo_image():
    """The floor's inlaid IRONHOUSE emblem (an RGBA image: the inlay's colors, transparent where the floor shows)."""
    w = 2000.0
    s = Sheet(w, w)
    sub = kit.Sub(s, 0.0, 0.0, 1.0)
    c = w / 2
    sub.circle(c, c, 990, 0x8a8c8e, n=128)
    sub.circle(c, c, 930, 0x1d1e20, n=128)
    sub.circle(c, c, 880, IRON_RED, n=128)
    sub.circle(c, c - 70, 400, 0x1d1e20, n=96)
    sub.ring(c, c + 330, 170, 280, 0x1d1e20, n=96, a0=-0.25, a1=math.pi + 0.25)
    sub.rect(c - 280, c + 230, 110, 140, 0x1d1e20)
    sub.rect(c + 170, c + 230, 110, 140, 0x1d1e20)
    sub.mid('IRONHOUSE', c, c - 560, 230, 1500, 0x1d1e20, 'Black', tracking=1.12)
    sub.mid('EST. 2009', c, c - 760, 90, 700, 0x1d1e20, 'Black', tracking=1.4)
    color, _ = s.render('gys_logo', 1024, samples=8)
    for obj, *_ in s.items:
        bpy.data.objects.remove(obj)
    return color


def draw_mural(s, w, h):
    """The painted mural on the east wall's brick: IRONHOUSE and the house rules, sign-painter style, a bit flaked."""
    rnd = random.Random(31)
    s.rect(0, 0, w, h, WALL_HI, rough=0.75)
    for i in range(int(h / 67.5)):
        s.rect(0, i * 67.5, w, 9, 0xb9b6ae, rough=0.85, alpha=0.5)
        off = (i % 2) * 101.6
        for j in range(int(w / 203.2) + 2):
            s.rect(j * 203.2 + off, i * 67.5, 9, 67.5, 0xb9b6ae, rough=0.85, alpha=0.4)
    s.mid('IRONHOUSE', w / 2, h * 0.66, h * 0.42, w * 0.92, IRON_RED, 'Black', tracking=1.1, rough=0.6)
    s.mid('NO EGOS  ·  RACK YOUR WEIGHTS  ·  WIPE IT DOWN', w / 2, h * 0.22, h * 0.13, w * 0.9, WALL_LO, 'Black', tracking=1.25, rough=0.6)
    for i in range(90):
        cx_, cy_ = rnd.uniform(0, w), rnd.uniform(0, h)
        r = rnd.uniform(5, 25)
        s.poly([(cx_ + r * rnd.uniform(0.5, 1.3) * math.cos(a), cy_ + r * rnd.uniform(0.4, 1.1) * math.sin(a)) for a in np.linspace(0, 2 * math.pi, 7, endpoint=False)],
               rnd.choice((0x8f4a35, 0x7a3a2a, WALL_HI)), rough=0.85, alpha=0.85)


def draw_lockers_sign(s, w, h):
    s.rect(0, 0, w, h, 0x1d1e20, rough=0.4)
    s.mid('LOCKERS  ·  SAUNA', w / 2, h * 0.62, h * 0.32, w * 0.9, 0xf0eee8, 'Black', tracking=1.2)
    s.mid('MEMBERS', w / 2, h * 0.24, h * 0.18, w * 0.6, IRON_RED, 'Black', tracking=1.4)


def shell():
    """SM_NS_Gym_Shell + _GlowCool (the eight LED high-bays' lenses)."""
    k = Kit('gys', 'SM_NS_Gym_Shell', TEXTURE_SIZES, dust=0.3, low_grime=0.15, module=MODULE, groups=['floor', 'wall', 'roof', 'steel', 'trim'])
    k.cavity = 0.03
    k.params['rubberfloor'] = {
        'tile': 1.0, 'origin': (IX0, IY0), 'chalk': 0.5,
        'logo': (logo_image(), (55.4, 40.4, 59.0, 44.0)),
        'paths': [([(47.0, 49.2), (47.3, 46.6)], 1.2), ([(47.6, 46.6), (52.0, 45.6), (56.0, 41.5), (54.0, 36.2)], 1.3),
                  ([(56.0, 41.5), (61.0, 38.0)], 1.0), ([(56.0, 42.0), (63.6, 44.0)], 1.1), ([(52.0, 45.6), (52.0, 46.5)], 0.9)],
    }
    lamps = []
    k.add(quad('floor', [(IX0, IY0, 0.0), (IX1, IY0, 0.0), (IX1, GY - 0.10, 0.0), (IX0, GY - 0.10, 0.0)]), 'rubberfloor', 0x1d1e20, group='floor', keep_uv=True)
    # --- the painted brick walls: charcoal to 1.2 m, a red stripe, white above; the band over the storefront inside
    walls = [((IX0, GY - 0.12), (IX0, IY0), 1), ((IX0, IY0), (IX1, IY0), 0), ((IX1, IY0), (IX1, GY - 0.12), 1)]
    back_door = (46.40, 47.40, 2.13)
    for p0, p1, dirv in walls:
        a = Vector((p0[0], p0[1], 0.0))
        b = Vector((p1[0], p1[1], 0.0))
        d = (b - a).normalized()
        L = (b - a).length
        holes = [(back_door[0] - IX0, back_door[1] - IX0, back_door[2])] if p0[1] == IY0 and p1[1] == IY0 else []
        for z0, z1, col in ((0.0, 1.2, WALL_LO), (1.2, 1.3, IRON_RED), (1.3, CZ, WALL_HI)):
            segs = [(0.0, L)]
            for h0, h1, hz in holes:
                segs = [(0.0, h0), (h1, L)]
                if z1 > hz:
                    ha, hb = a + d * h0, a + d * h1
                    k.add(quad('wall', [(hb.x, hb.y, max(z0, hz)), (ha.x, ha.y, max(z0, hz)), (ha.x, ha.y, z1), (hb.x, hb.y, z1)]), 'brickpaint', col,
                          dir=dirv, group='wall')
            for s0, s1 in segs:
                q0, q1 = a + d * s0, a + d * s1
                k.add(quad('wall', [(q1.x, q1.y, z0), (q0.x, q0.y, z0), (q0.x, q0.y, z1), (q1.x, q1.y, z1)]), 'brickpaint', col, dir=dirv, group='wall')
    k.add(quad('wall_front', [(IX0, GY - 0.12, TOPZ), (IX1, GY - 0.12, TOPZ), (IX1, GY - 0.12, CZ), (IX0, GY - 0.12, CZ)]), 'brickpaint', WALL_HI, dir=0,
          group='wall')
    # the mural on the east wall
    k.panel('mural', 7.0, 1.6, draw_mural)
    k.decal('mural', (IX1 - 0.001, 41.0, 3.2), u=(0, -1, 0), v=(0, 0, 1), lift=0.0)
    # --- the roof: a corrugated deck over purlins on five steel trusses (spanning N-S), painted black
    rib = 0.15
    n_rib = int((IX1 - IX0) / rib)
    for i in range(n_rib):
        x0 = IX0 + i * rib
        k.add(bx('deck_low', x0, x0 + rib * 0.5, IY0, GY - 0.12, CZ, CZ + 0.004, 0.0), 'corrugated', 0x1a1b1c, group='roof')
        k.add(bx('deck_high', x0 + rib * 0.55, x0 + rib * 0.95, IY0, GY - 0.12, CZ + 0.04, CZ + 0.044, 0.0), 'corrugated', 0x1a1b1c, group='roof')
        for xa, xb in ((x0 + rib * 0.5, x0 + rib * 0.55), (x0 + rib * 0.95, x0 + rib)):
            k.add(bx('deck_web', xa, xb, IY0, GY - 0.12, CZ, CZ + 0.044, 0.0), 'corrugated', 0x1a1b1c, group='roof')
    for yp in np.arange(IY0 + 1.2, GY - 0.3, 1.5):
        k.add(bx('purlin', IX0, IX1, yp - 0.03, yp + 0.03, CZ - 0.20, CZ, 0.002), 'paint', 0x1c1d1e, group='steel')
    zb, zt = 4.62, CZ - 0.22
    for tx in TRUSS_X:
        for zc in (zb, zt):
            k.add(bx('chord', tx - 0.06, tx + 0.06, IY0, GY - 0.12, zc - 0.008, zc + 0.008, 0.002), 'paint', 0x1c1d1e, dir=1, group='steel')
            k.add(bx('chord_web', tx - 0.006, tx + 0.006, IY0, GY - 0.12, zc - (0.07 if zc == zb else 0.0), zc + (0.0 if zc == zb else 0.07), 0.0), 'paint', 0x1c1d1e,
                  dir=1, group='steel')
        panel_n = 10
        span = (GY - 0.12) - IY0
        for j in range(panel_n + 1):
            y_ = IY0 + span * j / panel_n
            k.add(bx('vert', tx - 0.004, tx + 0.004, y_ - 0.03, y_ + 0.03, zb, zt, 0.0), 'paint', 0x1c1d1e, group='steel')
            if j < panel_n:
                y1_ = IY0 + span * (j + 1) / panel_n
                a_, b_ = (y_, zb), (y1_, zt) if j % 2 == 0 else (y1_, zb)
                if j % 2:
                    a_, b_ = (y_, zt), (y1_, zb)
                k.add(core.sweep('diag', [(tx, a_[0], a_[1]), (tx, b_[0], b_[1])], [(-0.03, -0.004), (0.03, -0.004), (0.03, 0.004), (-0.03, 0.004)]), 'paint',
                      0x1c1d1e, group='steel')
        for yb in (IY0, GY - 0.12):
            k.add(bx('bearing', tx - 0.15, tx + 0.15, yb - (0.0 if yb == IY0 else 0.25), yb + (0.25 if yb == IY0 else 0.0), zb - 0.3, zb, 0.004), 'paint', 0x1c1d1e,
                  group='steel')
    # --- ducts: two spiral runs along X with drum louvers, hung from the chords; the sprinkler main in red
    for dy in (45.6, 38.6):
        k.add(rod('duct', (IX0 + 0.3, dy, 4.15), (IX1 - 0.4, dy, 4.15), 0.28, 32), 'galv', 0xffffff, dir=0, group='steel')
        for xj in np.arange(IX0 + 1.5, IX1 - 0.5, 3.0):
            k.add(rod('duct_band', (xj - 0.03, dy, 4.15), (xj + 0.03, dy, 4.15), 0.286, 32), 'galv', 0xffffff, group='steel')
            k.add(bx('duct_strap', xj - 0.02, xj + 0.02, dy - 0.004, dy + 0.004, 4.43, zb, 0.0), 'galv', 0xffffff, group='steel')
        for xd in np.arange(IX0 + 3.0, IX1 - 1.0, 4.0):
            k.add(bx('louver', xd - 0.2, xd + 0.2, dy - 0.16, dy - 0.10, 3.9, 4.06, 0.01), 'galv', 0xffffff, group='steel')
            for zz in np.linspace(3.92, 4.04, 5):
                k.add(bx('louver_blade', xd - 0.19, xd + 0.19, dy - 0.17, dy - 0.15, zz - 0.006, zz + 0.006, 0.0), 'galv', 0xffffff, group='steel')
        k.add(xf(lathe_axis('duct_cap', [(0.0, 0.0), (0.29, 0.0), (0.29, 0.03), (0.0, 0.03)], segments=32), M(IX1 - 0.4, dy, 4.15, 0, 90)), 'galv', 0xffffff,
              group='steel')
    k.add(rod('sprinkler_main', (IX0 + 0.2, 42.1, 4.9), (IX1 - 0.2, 42.1, 4.9), 0.05, 16), 'paint', 0xa82418, dir=0, group='steel')
    for xs in np.arange(IX0 + 1.6, IX1 - 1.0, 3.2):
        for ys in (36.6, 40.2, 44.0, 47.6):
            k.add(rod('branch', (xs, 42.1, 4.9), (xs, ys, 4.9), 0.018, 10), 'paint', 0xa82418, dir=1, group='steel')
            k.add(rod('drop', (xs, ys, 4.9), (xs, ys, 4.82), 0.01, 8), 'paint', 0xa82418, group='steel')
            k.add(lathe_axis('head', [(0.0, 4.76), (0.012, 4.765), (0.007, 4.78), (0.01, 4.82), (0.0, 4.82)], M(xs, ys, 0.0), 12), 'brass', 0xffffff, group='steel')
    # --- the LED high-bays on their cords
    for hx, hy in HIGHBAYS:
        z0 = 4.05
        k.add(rod('cord', (hx, hy, z0 + 0.2), (hx, hy, zb - 0.07), 0.005, 8), 'rubber', 0x111111, group='trim')
        k.add(lathe_axis('hb_body', [(0.0, z0 + 0.2), (0.06, z0 + 0.2), (0.07, z0 + 0.12), (0.17, z0 + 0.06), (0.17, z0 + 0.0), (0.16, z0 - 0.01), (0.0, z0 - 0.01)],
                         M(hx, hy, 0.0), 40), 'alu', 0xffffff, group='trim')
        for f_ in range(16):
            a = 2 * math.pi * f_ / 16
            k.add(xf(bx('fin', 0.07, 0.165, -0.002, 0.002, z0 + 0.06, z0 + 0.15, 0.0), M(hx, hy, 0.0, math.degrees(a))), 'alu', 0xffffff, group='trim')
        lamps.append(lathe_axis('hb_lens', [(0.0, z0 - 0.015), (0.15, z0 - 0.015), (0.155, z0 - 0.008), (0.0, z0 - 0.008)], M(hx, hy, 0.0), 40))
    # --- the mirror wall (back wall), aluminum J-trim between panels
    mz0, mz1 = 0.35, 2.45
    n_m = int(round((MIRROR_X[1] - MIRROR_X[0]) / 1.2))
    pw = (MIRROR_X[1] - MIRROR_X[0]) / n_m
    for i in range(n_m):
        xa, xb = MIRROR_X[0] + i * pw + 0.004, MIRROR_X[0] + (i + 1) * pw - 0.004
        mq = quad('mirror', [(xb, IY0 + 0.012, mz0), (xa, IY0 + 0.012, mz0), (xa, IY0 + 0.012, mz1), (xb, IY0 + 0.012, mz1)])
        k.special(mq, UNB['ns_mirror'])
        k.add(bx('mirror_back', xa, xb, IY0, IY0 + 0.011, mz0, mz1, 0.0), 'paint', 0x111111, group='trim')
    for (a, b, c_, d_) in ((MIRROR_X[0] - 0.02, MIRROR_X[1] + 0.02, mz0 - 0.03, mz0), (MIRROR_X[0] - 0.02, MIRROR_X[1] + 0.02, mz1, mz1 + 0.03)):
        k.add(bx('jtrim', a, b, IY0, IY0 + 0.025, c_, d_, 0.003), 'alu', 0xffffff, group='trim')
    for i in range(n_m + 1):
        x_ = MIRROR_X[0] + i * pw
        k.add(bx('jtrim_v', x_ - 0.012, x_ + 0.012, IY0, IY0 + 0.02, mz0, mz1, 0.002), 'alu', 0xffffff, group='trim')
    # --- the heavy-bag brackets on the east wall
    for hx, hy, hz in BAG_HOOKS:
        k.add(bx('bag_plate', IX1 - 0.02, IX1, hy - 0.12, hy + 0.12, hz - 0.45, hz + 0.15, 0.006), 'powder', 0x1c1d1f, group='trim')
        k.add(bx('bag_arm', hx - 0.04, IX1 - 0.02, hy - 0.04, hy + 0.04, hz + 0.02, hz + 0.10, 0.006), 'powder', 0x1c1d1f, dir=0, group='trim')
        k.add(core.sweep('bag_gusset', [(IX1 - 0.02, hy, hz - 0.42), (hx + 0.2, hy, hz + 0.02)], [(-0.02, -0.01), (0.02, -0.01), (0.02, 0.01), (-0.02, 0.01)]),
              'powder', 0x1c1d1f, group='trim')
        k.add(xf(core.lathe('bag_eye', [(0.014, -0.004), (0.018, 0.0), (0.014, 0.004), (0.01, 0.0), (0.014, -0.004)], 16), M(hx, hy, hz + 0.0, 0, 0, 90)), 'steel',
              0xffffff, group='trim')
        k.add(rod('bag_eyebolt', (hx, hy, hz + 0.014), (hx, hy, hz + 0.02), 0.006, 8), 'steel', 0xffffff, group='trim')
    # --- the back door to the lockers and sauna, an EXIT sign, an extinguisher, a speaker, an electrical panel
    x0, x1, dh = back_door
    for xa, xb in ((x0 - 0.05, x0), (x1, x1 + 0.05)):
        k.add(bx('frame', xa, xb, IY0 - 0.02, IY0 + 0.03, 0.0, dh + 0.05, 0.004), 'paint', 0x2a2b2d, dir=2, group='trim')
    k.add(bx('frame_head', x0 - 0.05, x1 + 0.05, IY0 - 0.02, IY0 + 0.03, dh, dh + 0.05, 0.004), 'paint', 0x2a2b2d, group='trim')
    k.add(bx('leaf', x0 + 0.003, x1 - 0.003, IY0 - 0.03, IY0 + 0.012, 0.01, dh - 0.003, 0.004), 'paint', IRON_RED, group='trim')
    k.add(bx('kick', x0 + 0.02, x1 - 0.02, IY0 + 0.012, IY0 + 0.014, 0.02, 0.27, 0.002), 'steel', 0xffffff, group='trim')
    k.add(bx('pushbar', x0 + 0.1, x1 - 0.1, IY0 + 0.04, IY0 + 0.07, 0.975, 1.025, 0.012), 'steel', 0xffffff, dir=0, group='trim')
    for xb_ in (x0 + 0.08, x1 - 0.08):
        k.add(bx('bar_mount', xb_ - 0.04, xb_ + 0.04, IY0 + 0.012, IY0 + 0.06, 0.96, 1.04, 0.01), 'powder', 0x2a2a2a, group='trim')
    k.panel('lockers', 0.5, 0.16, draw_lockers_sign)
    k.decal('lockers', ((x0 + x1) / 2, IY0 + 0.0125, 1.62), u=(-1, 0, 0), v=(0, 0, 1), lift=0.0005)     # faces +Y, into the room
    k.panel('exit', 0.30, 0.18, kit.draw_exit, glow=2.5)
    kit.ext_sign(k, ((x0 + x1) / 2, IY0 + 0.05, dh + 0.24), 1)
    k.add(bx('exit_mount', (x0 + x1) / 2 - 0.12, (x0 + x1) / 2 + 0.12, IY0, IY0 + 0.025, dh + 0.18, dh + 0.30, 0.004), 'plastic', 0xefece4, group='trim')
    ex_x = x1 + 0.6
    k.add(lathe_axis('ext', [(0.0, 0.62), (0.075, 0.62), (0.078, 0.64), (0.078, 1.02), (0.06, 1.07), (0.02, 1.1), (0.0, 1.1)], M(ex_x, IY0 + 0.1, 0.0), 32),
          'paint', 0xb3161a, group='trim')
    k.add(bx('ext_bracket', ex_x - 0.05, ex_x + 0.05, IY0, IY0 + 0.03, 0.95, 1.0, 0.004), 'powder', 0x2a2a2a, group='trim')
    k.add(bx('panel_box', IX0, IX0 + 0.12, 38.0, 38.6, 1.2, 2.1, 0.006), 'paint', 0x8a8c8e, group='trim')
    k.add(rod('conduit', (IX0 + 0.06, 38.3, 2.1), (IX0 + 0.06, 38.3, 4.4), 0.012, 10), 'galv', 0xffffff, group='trim')
    for sx_, sy_ in ((IX0 + 0.12, 44.0), (IX1 - 0.12, 38.0)):
        k.add(bx('speaker', sx_ - 0.1, sx_ + 0.1, sy_ - 0.14, sy_ + 0.14, 3.2, 3.6, 0.02), 'plastic', 0x16171a, group='trim', var=0.1)
    k.grime += [((x0 + 0.1, IY0, 1.0), 0.35, 0.6)]
    obj = k.finish(angle=40.0, unbaked_order=['ns_mirror'])
    return [obj, glow(lamps, 'SM_NS_Gym_Shell_GlowCool', MODULE)]


# ------------------------------------------------------------------ the power rack

RACK_FX, RACK_BX, RACK_Y, RACK_H = -0.45, 0.55, 0.58, 2.30
RACK_BAR = (RACK_FX - 0.065, 0.0, 1.315)      # where a racked barbell's middle rests in the J-hooks (rack frame)


def draw_brand(s, w, h):
    s.rect(0, 0, w, h, BLACK, rough=0.5)
    s.mid('IRONHOUSE', w / 2, h / 2, h * 0.6, w * 0.92, 0xe8e6e0, 'Black', tracking=1.3, rough=0.5)


def squat_rack():
    """SM_NS_Gym_SquatRack: a four-post power rack (the open front toward -X, origin on the floor at its middle): 3 in
    uprights with numbered holes, J-hooks at 1.3 m (RACK_BAR is where a barbell rests), pin safeties, a pull-up bar,
    floor rails on rubber feet, plate horns on the back posts loaded with plates."""
    k = Kit('gyq', 'SM_NS_Gym_SquatRack', TEXTURE_SIZES, dust=0.1, low_grime=0.12, module=MODULE)
    u = 0.038
    k.panel('holes', 0.075, 2.05, holes_panel(38))
    k.panel('brand', 0.5, 0.06, draw_brand)
    for x in (RACK_FX, RACK_BX):
        for s_ in (-1, 1):
            k.add(bx('upright', x - u, x + u, s_ * RACK_Y - u, s_ * RACK_Y + u, 0.05, RACK_H, 0.004), 'powder', BLACK, dir=2)
            k.add(bx('cap', x - u - 0.002, x + u + 0.002, s_ * RACK_Y - u - 0.002, s_ * RACK_Y + u + 0.002, RACK_H, RACK_H + 0.006, 0.003), 'plastic', 0x111111)
            k.decal('holes', (x - u - 0.0005, s_ * RACK_Y, 0.12 + 2.05 / 2), u=(0, -1, 0), v=(0, 0, 1), w=0.074, h=2.05, lift=0.0)
            k.decal('holes', (x + u + 0.0005, s_ * RACK_Y, 0.12 + 2.05 / 2), u=(0, 1, 0), v=(0, 0, 1), w=0.074, h=2.05, lift=0.0)
    for s_ in (-1, 1):
        k.add(bx('rail', RACK_FX - 0.1, RACK_BX + 0.1, s_ * RACK_Y - u, s_ * RACK_Y + u, 0.0, 0.05, 0.004), 'powder', BLACK, dir=0)
        for x in (RACK_FX - 0.08, RACK_BX + 0.08):
            k.add(bx('foot', x - 0.04, x + 0.04, s_ * RACK_Y - 0.045, s_ * RACK_Y + 0.045, -0.002, 0.01, 0.004), 'rubber', 0x111111)
        k.add(bx('top_side', RACK_FX - u, RACK_BX + u, s_ * RACK_Y - u, s_ * RACK_Y + u, RACK_H - 0.08, RACK_H, 0.004), 'powder', BLACK, dir=0)
        # pin safeties front to back, J-hooks on the front posts
        k.add(rod('safety', (RACK_FX - 0.04, s_ * (RACK_Y - 0.07), 0.80), (RACK_BX + 0.04, s_ * (RACK_Y - 0.07), 0.80), 0.016, 16), 'steel', 0xffffff, dir=0)
        for x in (RACK_FX, RACK_BX):
            k.add(rod('pin', (x, s_ * (RACK_Y - 0.06), 0.78), (x, s_ * (RACK_Y + 0.08), 0.78), 0.009, 10), 'steel', 0xffffff)
        jy = s_ * RACK_Y
        k.add(bx('jplate', RACK_FX - u - 0.012, RACK_FX - u, jy - 0.035, jy + 0.035, 1.2, 1.36, 0.003), 'powder', BLACK)
        k.add(bx('jcup', RACK_FX - u - 0.11, RACK_FX - u - 0.012, jy - 0.03, jy + 0.03, 1.27, 1.30, 0.006), 'powder', BLACK)
        k.add(bx('jlip', RACK_FX - u - 0.12, RACK_FX - u - 0.10, jy - 0.03, jy + 0.03, 1.27, 1.36, 0.006), 'powder', BLACK)
        k.add(bx('jliner', RACK_FX - u - 0.10, RACK_FX - u - 0.012, jy - 0.028, jy + 0.028, 1.30, 1.305, 0.002), 'plastic', 0x111111)
    for x in (RACK_FX, RACK_BX):
        k.add(bx('top_x', x - u, x + u, -RACK_Y - u, RACK_Y + u, RACK_H - 0.08, RACK_H, 0.004), 'powder', BLACK, dir=1)
    k.add(bx('floor_back', RACK_BX - u, RACK_BX + u, -RACK_Y, RACK_Y, 0.0, 0.05, 0.004), 'powder', BLACK, dir=1)
    k.add(rod('pullup', (RACK_FX - 0.12, -RACK_Y - 0.05, RACK_H - 0.04), (RACK_FX - 0.12, RACK_Y + 0.05, RACK_H - 0.04), 0.016, 16), 'knurl', 0xffffff, dir=1)
    for s_ in (-1, 1):
        k.add(bx('pullup_bracket', RACK_FX - 0.14, RACK_FX - u, s_ * RACK_Y - 0.02, s_ * RACK_Y + 0.02, RACK_H - 0.08, RACK_H, 0.003), 'powder', BLACK)
    k.decal('brand', (RACK_FX - u - 0.0005, 0.0, RACK_H - 0.04), u=(0, -1, 0), v=(0, 0, 1), lift=0.0)
    # plate horns on the back posts with plates on them
    rnd = random.Random(7)
    for s_ in (-1, 1):
        for z, stack in ((0.42, [45, 45, 45]), (0.95, [25, 25, 10])):
            k.add(rod('horn', (RACK_BX + u, s_ * RACK_Y, z), (RACK_BX + u + 0.26, s_ * RACK_Y, z + 0.02), 0.024, 20), 'powder', BLACK, dir=0)
            x = RACK_BX + u + 0.012
            for i, lb in enumerate(stack):
                R, T = PLATES[lb]
                x += T / 2
                plate(k, Vector((x, s_ * RACK_Y, z + (x - RACK_BX) * 0.07)), 0.0, lb, label=(i == len(stack) - 1), seed=rnd.random())
                x += T / 2 + 0.003
    for lb in (45, 25):
        k.panel(f'plate{lb}', 0.12, 0.03, draw_plate(f'{lb} LB'))
    k.polish += [((RACK_FX - 0.12, 0.0, RACK_H - 0.04), 0.5, 0.6)]
    k.grime += [((RACK_FX, 0.0, 1.3), 0.6, 0.3)]
    return [k.finish(angle=40.0)]


# ================================================================== P1 equipment and dressing

def speed_bag():
    """SM_NS_Gym_SpeedBag: a wall-mounted speed-bag platform (origin on the floor at the wall plane, toward -X): a steel
    bracket, the round rebound board (its underside at 1.85 m), the swivel and a red leather bag."""
    k = Kit('gysb', 'SM_NS_Gym_SpeedBag', TEXTURE_SIZES, dust=0.08, low_grime=0.0, module=MODULE)
    z0 = 1.85
    k.add(bx('wall_plate', -0.02, 0.0, -0.25, 0.25, z0 - 0.35, z0 + 0.15, 0.006), 'powder', BLACK)
    for s_ in (-1, 1):
        k.add(bx('arm', -0.62, -0.02, s_ * 0.2 - 0.02, s_ * 0.2 + 0.02, z0 + 0.06, z0 + 0.1, 0.004), 'powder', BLACK, dir=0)
        k.add(core.sweep('brace', [(-0.02, s_ * 0.2, z0 - 0.3), (-0.4, s_ * 0.2, z0 + 0.06)], [(-0.015, -0.015), (0.015, -0.015), (0.015, 0.015), (-0.015, 0.015)]),
              'powder', BLACK)
    k.add(lathe_axis('board', [(0.0, z0), (0.30, z0), (0.31, z0 + 0.01), (0.31, z0 + 0.05), (0.30, z0 + 0.06), (0.0, z0 + 0.06)], M(-0.5, 0.0, 0.0), 48),
          'wood', 0x7a5232, dir=0)
    k.add(lathe_axis('swivel', [(0.0, z0 - 0.06), (0.012, z0 - 0.055), (0.014, z0 - 0.02), (0.03, z0 - 0.01), (0.03, z0), (0.0, z0)], M(-0.5, 0.0, 0.0), 16), 'steel',
          0xffffff)
    bag = lathe_axis('bag', [(0.0, z0 - 0.30), (0.05, z0 - 0.29), (0.085, z0 - 0.24), (0.092, z0 - 0.18), (0.08, z0 - 0.12), (0.045, z0 - 0.08), (0.02, z0 - 0.065),
                             (0.0, z0 - 0.062)], M(-0.5, 0.0, 0.0), 32)
    k.add(bag, 'leather', 0x8a1a16)
    for i in range(4):
        a = math.radians(i * 90)
        k.add(tube('bag_seam', [(-0.5 + 0.09 * math.cos(a), 0.09 * math.sin(a), z0 - 0.2), (-0.5 + 0.075 * math.cos(a), 0.075 * math.sin(a), z0 - 0.12)], 0.003, 6),
              'leather', 0x5a0f0c)
    return [k.finish(angle=45.0)]


def cable_station():
    """SM_NS_Gym_CableStation: a functional trainer (front toward -X): two weight-stack towers with shrouds, sliding
    pulley carriages, cables to D-handles, a pull-up bar joining the towers, a base frame."""
    k = Kit('gycs', 'SM_NS_Gym_CableStation', TEXTURE_SIZES, dust=0.1, low_grime=0.1, module=MODULE)
    for s_ in (-1, 1):
        ty = s_ * 0.85
        k.add(bx('tower_base', -0.35, 0.35, ty - 0.3, ty + 0.3, 0.0, 0.06, 0.006), 'powder', BLACK)
        for xx in (-0.2, 0.2):
            k.add(bx('post', xx - 0.04, xx + 0.04, ty - 0.04, ty + 0.04, 0.06, 2.2, 0.004), 'powder', BLACK, dir=2)
        k.add(bx('shroud', 0.08, 0.16, ty - 0.18, ty + 0.18, 0.1, 2.0, 0.008), 'plastic', 0x26282b, var=0.2)
        # the weight stack behind the shroud's window, its guide rods
        for i in range(16):
            k.add(bx('plate', -0.06, 0.06, ty - 0.13, ty + 0.13, 0.1 + i * 0.026, 0.1 + i * 0.026 + 0.024, 0.004), 'paint', 0x1a1b1d)
        for yy in (ty - 0.09, ty + 0.09):
            k.add(rod('guide', (0.0, yy, 0.06), (0.0, yy, 1.9), 0.01, 10), 'chrome', var=0.4)
        k.add(bx('topcap', -0.3, 0.3, ty - 0.12, ty + 0.12, 2.2, 2.26, 0.008), 'powder', BLACK)
        # the carriage on the front post with its pulley, the cable, a D-handle
        cz = 1.15
        k.add(bx('carriage', -0.32, -0.22, ty - 0.06, ty + 0.06, cz - 0.1, cz + 0.1, 0.008), 'powder', IRON_RED)
        k.add(rod('pop_pin', (-0.32, ty, cz + 0.05), (-0.38, ty, cz + 0.05), 0.008, 10), 'chrome')
        k.add(xf(lathe_axis('pulley', [(0.0, -0.012), (0.05, -0.012), (0.05, 0.012), (0.0, 0.012)], segments=24), M(-0.38, ty, cz) @ Matrix.Rotation(math.radians(90), 4, 'Z')
                 @ Matrix.Rotation(math.radians(90), 4, 'X')), 'plastic', 0x111111)
        k.add(tube('cable', [(-0.38, ty, cz - 0.05), (-0.45, ty - s_ * 0.05, cz - 0.6), (-0.46, ty - s_ * 0.06, cz - 0.75)], 0.003, 6), 'rubber', 0x111111)
        k.add(tube('dhandle', bend([(-0.42, ty - s_ * 0.1, cz - 0.78), (-0.5, ty - s_ * 0.1, cz - 0.88), (-0.5, ty - s_ * 0.0, cz - 0.88), (-0.42, ty - s_ * 0.0, cz - 0.78)],
                                   0.03), 0.008, 10), 'steel', 0xffffff)
        k.add(rod('dgrip', (-0.5, ty - s_ * 0.09, cz - 0.88), (-0.5, ty - s_ * 0.01, cz - 0.88), 0.016, 12), 'foam', 0x1a1a1a)
    k.add(bx('cross_base', -0.04, 0.04, -0.85, 0.85, 0.0, 0.06, 0.004), 'powder', BLACK, dir=1)
    k.add(rod('pullup', (-0.2, -0.85, 2.18), (-0.2, 0.85, 2.18), 0.016, 16), 'knurl', 0xffffff, dir=1)
    k.add(bx('top_beam', -0.04, 0.04, -0.85, 0.85, 2.22, 2.28, 0.004), 'powder', BLACK, dir=1)
    return [k.finish(angle=40.0)]


def mats():
    """SM_NS_Gym_Mats: the stretch corner (origin on the floor at its middle): two folding mats laid out, a stack of three
    folded, yoga mats rolled in a bin, foam rollers, three kettlebells and a chalk bucket."""
    k = Kit('gym', 'SM_NS_Gym_Mats', TEXTURE_SIZES, dust=0.0, low_grime=0.0, module=MODULE)
    rnd = random.Random(5)
    for i, yy in enumerate((-0.65, 0.65)):
        for j in range(4):
            k.add(soft_slab('mat_panel', 0.6, 1.2, 0.04, 0.012, 4, 6, 2, seed=rnd.randint(1, 99), noise_amt=0.002, matrix=M(-0.9 + j * 0.605, yy, 0.0)), 'vinyl',
                  0x2a4a7a if i == 0 else 0x1d1e20)
    for j in range(3):
        k.add(soft_slab('mat_stack', 0.62, 1.22, 0.16, 0.03, 4, 6, 3, seed=rnd.randint(1, 99), noise_amt=0.004, matrix=M(1.25, 0.0, j * 0.16, rnd.uniform(-3, 3))),
              'vinyl', 0x2a4a7a)
    bx0, by0 = 1.25, 1.05
    k.add(lathe_solid('bin', [(0.2, 0.0), (0.22, 0.0), (0.24, 0.45), (0.22, 0.45), (0.2, 0.02)], M(bx0, by0, 0.0), 32), 'plastic', 0x1a1a1c)
    for i in range(6):
        a = 2 * math.pi * i / 6
        k.add(rod('yogamat', (bx0 + 0.11 * math.cos(a), by0 + 0.11 * math.sin(a), 0.02), (bx0 + 0.13 * math.cos(a), by0 + 0.13 * math.sin(a), 0.62), 0.05, 20),
              'foam', rnd.choice((0x6a2a5a, 0x2a6a6a, 0x3a3a3a, 0x8a5a2a)))
    for i in range(2):
        k.add(rod('roller', (-1.2, -1.15 + i * 0.18, 0.075), (-0.3, -1.15 + i * 0.18, 0.075), 0.075, 24), 'foam', (0x1a1a1a, 0x2a5a8a)[i], dir=0)
    for i, (kx, ky, s_) in enumerate(((0.4, -1.3, 1.0), (0.65, -1.3, 1.15), (0.95, -1.3, 1.3))):
        k.add(lathe_axis('kb_body', [(0.0, 0.0), (0.06 * s_, 0.0), (0.1 * s_, 0.04 * s_), (0.11 * s_, 0.1 * s_), (0.09 * s_, 0.17 * s_), (0.05 * s_, 0.2 * s_),
                                     (0.0, 0.205 * s_)], M(kx, ky, 0.0), 32), 'paint', 0x1a1b1d)
        k.add(tube('kb_handle', bend([(kx - 0.07 * s_, ky, 0.18 * s_), (kx - 0.07 * s_, ky, 0.29 * s_), (kx + 0.07 * s_, ky, 0.29 * s_), (kx + 0.07 * s_, ky, 0.18 * s_)],
                                     0.05 * s_), 0.017 * s_, 14), 'paint', 0x1a1b1d)
    k.add(lathe_solid('chalk_bucket', [(0.14, 0.0), (0.155, 0.0), (0.165, 0.28), (0.15, 0.28), (0.14, 0.02)], M(-1.4, 1.0, 0.0), 32), 'plastic', 0xd8d6d0, var=0.6)
    k.add(lathe_axis('chalk', [(0.0, 0.18), (0.14, 0.18), (0.08, 0.2), (0.0, 0.215)], M(-1.4, 1.0, 0.0), 24), 'chalk', 0xffffff)
    return [k.finish(angle=45.0)]


def draw_locker_no(n):
    def draw(s, w, h):
        s.rect(0, 0, w, h, 0xd8d6cf, metal=0.8, rough=0.35)
        s.mid(str(n), w / 2, h / 2, h * 0.7, w * 0.8, 0x1a1a1a, 'Black')
    return draw


def lockers():
    """SM_NS_Gym_Lockers: a bank of six double-tier steel lockers against a wall (origin on the floor at the middle of
    the back, the doors toward -X): louvered doors, latches, numbers 11-22, two padlocks, a bench in front."""
    k = Kit('gylk', 'SM_NS_Gym_Lockers', TEXTURE_SIZES, dust=0.3, low_grime=0.15, module=MODULE)
    W, D, Hh = 0.38, 0.45, 1.83
    n = 6
    y0 = -n * W / 2
    k.add(bx('carcass', -D, 0.0, y0, -y0, 0.1, Hh, 0.004), 'paint', 0x5f666b)
    k.add(bx('base', -D + 0.02, 0.0, y0, -y0, 0.0, 0.1, 0.002), 'paint', 0x2a2b2d)
    k.add(bx('top', -D - 0.01, 0.0, y0 - 0.01, -y0 + 0.01, Hh, Hh + 0.02, 0.004), 'paint', 0x5f666b)
    rnd = random.Random(11)
    for i in range(n):
        yc = y0 + (i + 0.5) * W
        for t, (z0, z1) in enumerate(((0.12, 0.96), (0.98, Hh - 0.02))):
            k.add(bx('door', -D - 0.018, -D, yc - W / 2 + 0.008, yc + W / 2 - 0.008, z0, z1, 0.004), 'paint', 0x6a7378, var=rnd.random())
            for j in range(5):
                lz = z1 - 0.08 - j * 0.025
                k.add(bx('louver', -D - 0.024, -D - 0.018, yc - 0.1, yc + 0.1, lz, lz + 0.012, 0.002), 'paint', 0x2a2d30)
            k.add(bx('latch', -D - 0.04, -D - 0.018, yc + W / 2 - 0.07, yc + W / 2 - 0.04, (z0 + z1) / 2 - 0.06, (z0 + z1) / 2 + 0.06, 0.006), 'chrome', var=0.4)
            num = 11 + i * 2 + t
            key = f'no{num}'
            k.panel(key, 0.06, 0.035, draw_locker_no(num))
            k.decal(key, (-D - 0.0185, yc, z1 - 0.04), u=(0, -1, 0), v=(0, 0, 1), lift=0.0)
            if rnd.random() < 0.25:
                lx_, ly_, lz_ = -D - 0.05, yc + W / 2 - 0.055, (z0 + z1) / 2 - 0.09
                k.add(bx('padlock', lx_ - 0.012, lx_ + 0.012, ly_ - 0.022, ly_ + 0.022, lz_ - 0.05, lz_, 0.008), 'brass', 0xffffff)
                k.add(tube('shackle', bend([(lx_, ly_ - 0.014, lz_), (lx_, ly_ - 0.014, lz_ + 0.035), (lx_, ly_ + 0.014, lz_ + 0.035), (lx_, ly_ + 0.014, lz_)], 0.012),
                           0.003, 8), 'steel', 0xffffff)
    # the bench
    k.add(bx('bench_top', -D - 0.75, -D - 0.45, y0 + 0.2, -y0 - 0.2, 0.42, 0.46, 0.008), 'wood', 0x9a7044, dir=1)
    for yy in (y0 + 0.4, -y0 - 0.4):
        k.add(bx('bench_leg', -D - 0.62, -D - 0.58, yy - 0.02, yy + 0.02, 0.0, 0.42, 0.004), 'powder', BLACK)
        k.add(bx('bench_foot', -D - 0.72, -D - 0.48, yy - 0.025, yy + 0.025, 0.0, 0.03, 0.004), 'powder', BLACK)
    k.grime += [((-D, 0.0, 1.0), 1.0, 0.25)]
    return [k.finish(angle=40.0)]


def wod_picture():
    s = Sheet(1200.0, 900.0)
    sub = kit.Sub(s, 0.0, 0.0, 1.0)
    sub.rect(0, 0, 1200, 900, 0xf6f6f2)
    kit.hand_mid(sub, 'WOD', 600, 800, 90, 400, IRON_RED, width=0.12, seed=3)
    for i, ln in enumerate(('5 rounds:', '10 kettlebell swings', '15 push-ups', '20 squats', '400 m run', 'cash out: 50 sit-ups')):
        kit.hand(sub, ln, 120, 640 - i * 95, 56, 0x1a2a5a if i else 0x1a1a1a, width=0.1, seed=10 + i)
    return render_sheet(s, 'gyw_wod', 768, samples=8)


def tv_picture():
    s = Sheet(1600.0, 900.0)
    sub = kit.Sub(s, 0.0, 0.0, 1.0)
    sub.rect(0, 0, 1600, 900, 0x10233a)
    sub.rect(0, 0, 1600, 160, 0x0a0f18)
    sub.mid('LATE NIGHT REPLAY', 800, 80, 60, 1300, 0xffffff, 'Black', tracking=1.2)
    sub.rect(200, 280, 1200, 480, 0x1e5a3a)
    sub.circle(800, 520, 160, 0xd8d0c0, n=64)
    return render_sheet(s, 'gyw_tv', 768, samples=8)


def whiteboard():
    """SM_NS_Gym_Whiteboard: on the wall (origin on the floor at the wall plane, toward -X): the workout of the day
    (whiteboard face [screen] ns_screen_wod, UV 0..1), its marker tray, and a 55 in TV on a bracket ([screen]
    ns_screen_tv)."""
    k = Kit('gyw', 'SM_NS_Gym_Whiteboard', TEXTURE_SIZES, dust=0.08, low_grime=0.0, module=MODULE)
    wy, wz, ww, wh = -0.75, 1.45, 1.2, 0.9
    for (a, b, c_, d_) in ((wy - ww / 2 - 0.02, wy + ww / 2 + 0.02, wz - wh / 2 - 0.02, wz - wh / 2), (wy - ww / 2 - 0.02, wy + ww / 2 + 0.02, wz + wh / 2, wz + wh / 2 + 0.02),
                           (wy - ww / 2 - 0.02, wy - ww / 2, wz - wh / 2, wz + wh / 2), (wy + ww / 2, wy + ww / 2 + 0.02, wz - wh / 2, wz + wh / 2)):
        k.add(bx('wb_frame', -0.025, 0.0, a, b, c_, d_, 0.003), 'alu', 0xffffff, dir=1 if (b - a) > (d_ - c_) else 2)
    k.add(bx('wb_back', -0.015, 0.0, wy - ww / 2, wy + ww / 2, wz - wh / 2, wz + wh / 2, 0.0), 'paint', 0xeeeeea)
    q = quad('wod', [(-0.0155, wy + ww / 2, wz - wh / 2), (-0.0155, wy - ww / 2, wz - wh / 2), (-0.0155, wy - ww / 2, wz + wh / 2), (-0.0155, wy + ww / 2, wz + wh / 2)])
    k.special(q, image_mat('ns_screen_wod', wod_picture(), rough=0.15))
    k.add(bx('tray', -0.07, -0.0, wy - ww / 2 + 0.1, wy + ww / 2 - 0.1, wz - wh / 2 - 0.05, wz - wh / 2 - 0.02, 0.004), 'alu', 0xffffff, dir=1)
    for i, col in enumerate((0x1a1a1a, IRON_RED, 0x1a3a8a)):
        k.add(rod('marker', (-0.045, wy - 0.3 + i * 0.05, wz - wh / 2 - 0.012), (-0.045, wy - 0.17 + i * 0.05, wz - wh / 2 - 0.012), 0.008, 10), 'plastic', col)
    k.add(bx('eraser', -0.06, -0.02, wy + 0.2, wy + 0.33, wz - wh / 2 - 0.02, wz - wh / 2 + 0.01, 0.006), 'felt', 0x2a2a2a)
    # the TV
    ty, tz, tw, th = 0.75, 2.05, 1.23, 0.71
    k.add(bx('tv_mount', -0.06, 0.0, ty - 0.2, ty + 0.2, tz - 0.15, tz + 0.15, 0.004), 'powder', BLACK)
    k.add(bx('tv', -0.11, -0.06, ty - tw / 2, ty + tw / 2, tz - th / 2, tz + th / 2, 0.008), 'plastic', 0x0e0f11, var=0.1)
    tq = quad('tv_screen', [(-0.1105, ty + tw / 2 - 0.012, tz - th / 2 + 0.012), (-0.1105, ty - tw / 2 + 0.012, tz - th / 2 + 0.012),
                            (-0.1105, ty - tw / 2 + 0.012, tz + th / 2 - 0.012), (-0.1105, ty + tw / 2 - 0.012, tz + th / 2 - 0.012)])
    k.special(tq, screen_mat('ns_screen_tv', tv_picture(), 1.6))
    k.add(tube('tv_cable', [(-0.08, ty + 0.3, tz - th / 2), (-0.03, ty + 0.35, tz - th / 2 - 0.3), (-0.01, ty + 0.35, 0.3)], 0.005, 8), 'rubber', 0x111111)
    return [k.finish(angle=40.0, unbaked_order=['ns_screen_wod', 'ns_screen_tv'])]


FAN_HUB = (-0.32, 0.0, 3.2)      # the blades' hub (fan frame): SM_NS_Gym_Fan_Blades goes here, spinning about X


def fan():
    """SM_NS_Gym_Fan (+ _Blades): a 30 in industrial wall fan on a tilt bracket (origin on the floor at the wall plane,
    blowing toward -X); its blades are a separate mesh whose origin is the hub, to place at FAN_HUB and spin about X."""
    k = Kit('gyfan', 'SM_NS_Gym_Fan', TEXTURE_SIZES, dust=0.5, low_grime=0.0, module=MODULE)
    hx, hy, hz = FAN_HUB
    k.add(bx('wall_plate', -0.015, 0.0, -0.12, 0.12, hz - 0.12, hz + 0.12, 0.006), 'powder', BLACK)
    k.add(bx('yoke_arm', hx + 0.16, -0.015, -0.03, 0.03, hz - 0.03, hz + 0.03, 0.004), 'powder', BLACK, dir=0)
    for s_ in (-1, 1):
        k.add(bx('yoke', hx + 0.12, hx + 0.18, s_ * 0.42 - 0.015, s_ * 0.42 + 0.015, hz - 0.03, hz + 0.03, 0.004), 'powder', BLACK)
    k.add(rod('yoke_bar', (hx + 0.15, -0.42, hz), (hx + 0.15, 0.42, hz), 0.015, 12), 'powder', BLACK)
    k.add(xf(lathe_axis('motor', [(0.0, 0.0), (0.09, 0.0), (0.1, 0.03), (0.1, 0.2), (0.08, 0.24), (0.0, 0.25)], segments=32), M(hx + 0.04, hy, hz, 0, 90)), 'powder',
          0x2a2b2d)
    for side, dx in ((-1, -0.09), (1, 0.06)):
        ring = core.lathe('guard_ring', [(0.40, -0.006), (0.41, 0.0), (0.40, 0.006), (0.39, 0.0), (0.40, -0.006)], 64)
        ring.data.transform(M(hx + dx, hy, hz, 0, 90))
        k.add(ring, 'chrome', var=0.6)
        for r_ in (0.1, 0.18, 0.26, 0.34):
            rr = core.lathe('guard_wire', [(r_, -0.002), (r_ + 0.002, 0.0), (r_, 0.002), (r_ - 0.002, 0.0), (r_, -0.002)], 48)
            rr.data.transform(M(hx + dx * (1.0 - 0.3 * r_ / 0.4), hy, hz, 0, 90))
            k.add(rr, 'chrome', var=0.6)
        for i in range(16):
            a = 2 * math.pi * i / 16
            pts = [(hx + dx * (1.0 - 0.3 * t), hy + 0.4 * t * math.cos(a), hz + 0.4 * t * math.sin(a)) for t in np.linspace(0.05, 1.0, 6)]
            k.add(tube('guard_spoke', pts, 0.002, 4), 'chrome', var=0.6)
    obj = k.finish(angle=45.0)
    kb = Kit('gyfb', 'SM_NS_Gym_Fan_Blades', TEXTURE_SIZES, dust=0.6, low_grime=0.0, module=MODULE)
    kb.add(xf(lathe_axis('spinner', [(0.0, -0.05), (0.04, -0.04), (0.06, 0.0), (0.06, 0.04), (0.0, 0.04)], segments=24), Matrix.Rotation(math.radians(-90), 4, 'Y')),
           'alu', 0xffffff)
    for i in range(3):
        a = 120 * i
        blade = parts.panel('blade', core.rounded_rect(0.05, -0.07, 0.36, 0.08, 0.05, 4), -0.002, 0.002, 0.001)
        blade.data.transform(Matrix.Rotation(math.radians(a), 4, 'X') @ Matrix(((0, 0, -1, 0), (0, 1, 0, 0), (1, 0, 0, 0), (0, 0, 0, 1)))
                             @ Matrix.Rotation(math.radians(22), 4, 'X'))
        kb.add(blade, 'alu', 0xffffff, var=0.3)
    blades = kb.finish(angle=45.0)
    return [obj, blades]


# ------------------------------------------------------------------ the back wall's dressing over the mirrors (SITE)

def draw_club_banner(s, w, h):
    """The club's vinyl banner: the kettlebell mark, IRONHOUSE, what's done here and where (3.2 x 1.05 m)."""
    rnd = random.Random(41)
    s.rect(0, 0, w, h, 0x111214, rough=0.45)
    s.rect(30, 30, w - 60, 14, IRON_RED, rough=0.4)
    s.rect(30, h - 44, w - 60, 14, IRON_RED, rough=0.4)
    cx, cy = 520, h / 2
    s.circle(cx, cy, 400, IRON_RED, rough=0.4, n=96)
    s.circle(cx, cy - 50, 190, 0xece8e0, rough=0.4, n=80)
    s.ring(cx, cy + 135, 75, 125, 0xece8e0, rough=0.4, n=80, a0=-0.2, a1=math.pi + 0.2)
    s.rect(cx - 125, cy + 75, 50, 70, 0xece8e0, rough=0.4)
    s.rect(cx + 75, cy + 75, 50, 70, 0xece8e0, rough=0.4)
    tx = (1000 + w - 80) / 2
    s.mid('IRONHOUSE', tx, h * 0.64, 330, w - 1120, 0xf2efe8, 'Black', tracking=1.1, rough=0.4)
    s.mid('BOXING  ·  BARBELL  ·  CONDITIONING', tx, h * 0.33, 92, w - 1160, IRON_RED, 'Black', tracking=1.25, rough=0.4)
    s.mid('246 MARKET ST  ·  EST. 2009', tx, h * 0.17, 62, w - 1400, 0xa9a6a0, 'Bold', tracking=1.4, rough=0.45)
    for i in range(16):        # fold creases from the box it shipped in, and scuffs
        y = rnd.choice((h * 0.33, h * 0.5, h * 0.66)) + rnd.uniform(-6, 6)
        s.rect(rnd.uniform(0, w * 0.6), y, rnd.uniform(200, 900), 3, 0x34363a, rough=0.6, alpha=rnd.uniform(0.25, 0.5))
    for i in range(30):
        s.rect(rnd.uniform(0, w), rnd.uniform(0, h), rnd.uniform(10, 60), rnd.uniform(2, 5), 0x8a8a86, rough=0.7, alpha=rnd.uniform(0.15, 0.35))


def draw_champ_banner(s, w, h):
    """Rosa's banner: red felt, gold appliqué letters, from the city amateurs in 1999 (1.0 x 1.4 m)."""
    gold, cream = 0xc9a24a, 0xefe6d2
    s.rect(0, 0, w, h, 0x7e1714, rough=0.95)
    s.rect(30, 30, w - 60, h - 60, gold, rough=0.8)
    s.rect(46, 46, w - 92, h - 92, 0x8a1a16, rough=0.95)
    s.mid('CITY AMATEUR', w / 2, h * 0.86, 70, w - 140, cream, 'Black', tracking=1.15, rough=0.9)
    s.mid('BOXING', w / 2, h * 0.79, 70, w - 140, cream, 'Black', tracking=1.3, rough=0.9)
    for i, x in enumerate((w * 0.3, w * 0.5, w * 0.7)):
        r = 34 if i == 1 else 26
        s.poly([(x + r * (1.0 if j % 2 == 0 else 0.42) * math.cos(math.pi / 2 + j * math.pi / 5),
                 h * 0.69 + r * (1.0 if j % 2 == 0 else 0.42) * math.sin(math.pi / 2 + j * math.pi / 5)) for j in range(10)], gold, rough=0.8)
    s.mid('CHAMPION', w / 2, h * 0.56, 130, w - 120, gold, 'Black', tracking=1.05, rough=0.8)
    s.mid('1999', w / 2, h * 0.41, 150, w - 200, cream, 'Black', tracking=1.1, rough=0.9)
    s.mid("WOMEN'S 125 LB", w / 2, h * 0.28, 62, w - 160, cream, 'Black', tracking=1.2, rough=0.9)
    s.mid('R. DELGADO', w / 2, h * 0.15, 84, w - 160, gold, 'Black', tracking=1.2, rough=0.8)


def draw_fight_poster(card):
    def draw(s, w, h):
        """An old fight-card poster, framed: the club's night at a parish hall (a memento; its date long past)."""
        rnd = random.Random(len(card[0]))
        s.rect(0, 0, w, h, 0xe9dfc6, rough=0.8)
        s.rect(0, h * 0.78, w, h * 0.22, 0x141414, rough=0.8)
        s.mid(card[0], w / 2, h * 0.89, 70, w - 50, 0xf2d24a, 'Black', tracking=1.05, rough=0.8)
        s.mid('IRONHOUSE BOXING CLUB PRESENTS', w / 2, h * 0.73, 22, w - 60, 0x1a1a1a, 'Bold', tracking=1.3, rough=0.8)
        for i, (a, b) in enumerate(card[1]):
            y = h * (0.62 - i * 0.1)
            s.mid(a, w * 0.27, y, 30, w * 0.42, 0xb0161c if i == 0 else 0x1a1a1a, 'Black', rough=0.8)
            s.mid('vs', w / 2, y, 18, 40, 0x5a5a5a, 'Bold', rough=0.8)
            s.mid(b, w * 0.73, y, 30, w * 0.42, 0xb0161c if i == 0 else 0x1a1a1a, 'Black', rough=0.8)
        s.mid(card[2], w / 2, h * 0.17, 34, w - 60, 0x1a1a1a, 'Black', tracking=1.1, rough=0.8)
        s.mid(card[3], w / 2, h * 0.09, 22, w - 60, 0x3a3a3a, 'Bold', tracking=1.2, rough=0.8)
        for i in range(40):        # age: foxing and a sun-faded edge
            s.circle(rnd.uniform(0, w), rnd.uniform(0, h), rnd.uniform(2, 9), 0xb89a66, rough=0.9, alpha=rnd.uniform(0.1, 0.3), n=10)
        s.rect(0, 0, w, h * 0.12, 0xfff8e0, rough=0.8, alpha=0.15)
    return draw


FIGHT_CARDS = [('FIGHT NIGHT', [('DELGADO', 'OKAFOR'), ('RUIZ', 'MARSH'), ('BELL', 'KOWALSKI')], 'ST. AGNES HALL', 'SAT OCT 14 2017  ·  DOORS 7 PM'),
               ('SMOKER', [('TRAN', 'PRUITT'), ('VEGA', 'DOYLE'), ('AMARI', 'COLE')], 'IRONHOUSE  ·  246 MARKET', 'FRI MAR 3 2023  ·  8 PM')]


def wall_dressing():
    """SM_NS_Gym_Banners (SITE; baked body + print): what hangs on the back wall over the mirrors (the wall's face at
    stage Y 3450; all of it between X -4900 and -6200, Z 270..430): the club's vinyl banner (3.2 x 1.05 m, centered
    X -5200, Z 355) on zip ties through its grommets to eye screws; Rosa's red felt championship banner (1.0 x 1.4 m,
    X -5625, Z 280..420) on a dowel and cord from a nail, a gold fringe; two framed fight-night posters (0.6 x 0.9 m,
    X -5970 and -6090, Z 350)."""
    k = Kit('gybn', 'SM_NS_Gym_Banners', TEXTURE_SIZES, dust=0.45, low_grime=0.0, module=MODULE)
    wy = IY0
    # --- the club banner: a rippled vinyl sheet with eyelets, tied up at its corners and mid-top
    bw, bh, bxc_, bz = 3.2, 1.05, 52.0, 3.55
    k.panel('club', bw, bh, draw_club_banner)

    def ripple(s, t):
        corner = min(1.0, (min(s, 1 - s) * bw) / 0.3) * min(1.0, (min(t, 1 - t) * bh) / 0.3)
        return 0.004 + 0.007 * corner * (0.5 + 0.5 * math.sin(2 * math.pi * (s * 3.1 + t * 1.2))) + 0.004 * corner * math.sin(2 * math.pi * s * 7.0)
    k.decal('club', (bxc_, wy + 0.0, bz), u=(-1, 0, 0), v=(0, 0, 1), lift=0.0, nu=32, nv=10, disp=ripple)
    for gx, gz in ((bxc_ + bw / 2 - 0.04, bz + bh / 2 - 0.04), (bxc_ - bw / 2 + 0.04, bz + bh / 2 - 0.04), (bxc_, bz + bh / 2 - 0.04),
                   (bxc_ + bw / 2 - 0.04, bz - bh / 2 + 0.04), (bxc_ - bw / 2 + 0.04, bz - bh / 2 + 0.04)):
        k.add(xf(core.lathe('grommet', [(0.008, -0.0015), (0.012, 0.0), (0.008, 0.0015), (0.005, 0.0), (0.008, -0.0015)], 16), M(gx, wy + 0.007, gz, 0, 0, 90)),
              'brass', 0xffffff)
        ex, ez = gx + (0.05 if gx > bxc_ else -0.05 if gx < bxc_ else 0.0), gz + (0.09 if gz > bz else -0.09)
        k.add(rod('eye_screw', (ex, wy, ez), (ex, wy + 0.025, ez), 0.003, 6), 'steel', 0xffffff)
        k.add(xf(core.lathe('eye', [(0.007, -0.0015), (0.009, 0.0), (0.007, 0.0015), (0.005, 0.0), (0.007, -0.0015)], 12), M(ex, wy + 0.03, ez, 0, 0, 90)), 'steel',
              0xffffff)
        k.add(tube('ziptie', [(gx, wy + 0.008, gz), ((gx + ex) / 2, wy + 0.02, (gz + ez) / 2), (ex, wy + 0.03, ez)], 0.0025, 6), 'plastic', 0x101010)
    # --- Rosa's banner: felt on a dowel, a cord to a nail, a gold fringe along the bottom
    cw_, ch_, cxc, ctop = 1.0, 1.4, 56.25, 4.20
    k.panel('champ', cw_, ch_, draw_champ_banner)

    def hang(s, t):
        return 0.012 + 0.006 * math.sin(math.pi * s) * (1.0 - t) + 0.003 * math.sin(2 * math.pi * s * 4.0) * (1.0 - t)
    k.decal('champ', (cxc, wy, ctop - ch_ / 2), u=(-1, 0, 0), v=(0, 0, 1), lift=0.0, nu=12, nv=8, disp=hang)
    k.add(rod('dowel', (cxc + cw_ / 2 + 0.04, wy + 0.022, ctop), (cxc - cw_ / 2 - 0.04, wy + 0.022, ctop), 0.011, 14), 'wood', 0x8a6a46, dir=0)
    for s_ in (-1, 1):
        k.add(xf(lathe_axis('finial', [(0.0, 0.0), (0.016, 0.004), (0.018, 0.02), (0.008, 0.035), (0.0, 0.038)], segments=14),
                 M(cxc + s_ * (cw_ / 2 + 0.04), wy + 0.022, ctop, 0, -90 * s_)), 'brass', 0xffffff)
    k.add(tube('cord', [(cxc + cw_ / 2 - 0.02, wy + 0.022, ctop), (cxc, wy + 0.012, ctop + 0.2), (cxc - cw_ / 2 + 0.02, wy + 0.022, ctop)], 0.003, 6), 'cloth',
          0xc9a24a)
    k.add(rod('nail', (cxc, wy, ctop + 0.2), (cxc, wy + 0.02, ctop + 0.2), 0.0025, 6), 'steel', 0xffffff)
    rnd = random.Random(9)
    for i in range(34):
        fx = cxc + cw_ / 2 - 0.02 - i * (cw_ - 0.04) / 33
        z0 = ctop - ch_ + 0.002
        k.add(tube('fringe', [(fx, wy + 0.013, z0), (fx + rnd.uniform(-0.004, 0.004), wy + 0.014, z0 - 0.07 - rnd.uniform(0.0, 0.01))], 0.0035, 5), 'cloth', 0xc9a24a)
    # --- the framed fight posters
    for i, (px, card) in enumerate(((59.7, FIGHT_CARDS[0]), (60.9, FIGHT_CARDS[1]))):
        pw_, ph_, pz = 0.6, 0.9, 3.5
        key = f'poster{i}'
        k.panel(key, pw_ - 0.06, ph_ - 0.06, draw_fight_poster(card))
        k.add(bx('pframe_back', px - pw_ / 2, px + pw_ / 2, wy, wy + 0.012, pz - ph_ / 2, pz + ph_ / 2, 0.0), 'cardboard', 0x6a5a44)
        for (a, b, c_, d_) in ((px - pw_ / 2, px + pw_ / 2, pz + ph_ / 2 - 0.03, pz + ph_ / 2), (px - pw_ / 2, px + pw_ / 2, pz - ph_ / 2, pz - ph_ / 2 + 0.03),
                               (px - pw_ / 2, px - pw_ / 2 + 0.03, pz - ph_ / 2, pz + ph_ / 2), (px + pw_ / 2 - 0.03, px + pw_ / 2, pz - ph_ / 2, pz + ph_ / 2)):
            k.add(bx('pframe', a, b, wy + 0.012, wy + 0.03, c_, d_, 0.003), 'alu', 0xffffff)
        k.decal(key, (px, wy + 0.0125, pz), u=(-1, 0, 0), v=(0, 0, 1), lift=0.0)
    return [k.finish(angle=40.0)]


# ------------------------------------------------------------------ the chalk haze (SITE, optional, [runtime])

HAZE_Z = (2.8, 3.3, 3.8)          # the haze sheets' heights, under the high-bays (4.05) and the trusses' chords (4.62)


def haze_mask():
    """T_NSGY_HazeMask: soft drifting blotches of chalk dust, fading to nothing at the sheet's edges (its border never
    shows), grey 0..1 = how much haze."""
    n = 512
    u = (np.arange(n) + 0.5) / n
    U, V = np.meshgrid(u, u)
    P = np.stack([U * 9.7, V * 8.8, np.zeros_like(U)], -1).reshape(-1, 3)
    a = texmaps.value_noise(P, 1.6, seed=31, octaves=4).reshape(n, n)
    b = texmaps.value_noise(P + 5.3, 0.5, seed=37, octaves=3).reshape(n, n)
    edge = np.minimum(np.minimum(U, 1 - U), np.minimum(V, 1 - V))
    m = texmaps.smoothstep(0.38, 0.72, a * 0.75 + b * 0.25) * texmaps.smoothstep(0.0, 0.18, edge)
    return texmaps.save('T_NSGY_HazeMask', np.stack([m] * 3, -1), 'Non-Color')


def haze():
    """SM_NS_Gym_Haze (SITE, optional; one [runtime] slot ns_runtime_haze): the chalk haze over the lifting end (stage
    X -5550..-6520, Y 3470..4350), three horizontal sheets at Z 280, 330, 380 under the high-bays. UV 0..1 across each
    sheet; its base color is T_NSGY_HazeMask (white = haze), for a translucent unlit runtime material that fades the
    sheets by `Daylight` and how busy the floor is (none at 2 AM but the runner's). Seen at a grazing angle from the
    floor they stack into a dusty layer the high-bays light."""
    me = image_mat('ns_runtime_haze', haze_mask(), rough=1.0)
    me.use_backface_culling = False
    bsdf = next(n for n in me.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Alpha'].default_value = 0.12
    try:
        me.surface_render_method = 'BLENDED'
    except (AttributeError, TypeError):
        pass
    x0, x1, y0, y1 = 55.5, 65.2, 34.7, 43.5
    sheets = []
    for i, z in enumerate(HAZE_Z):
        dx, dy = 0.3 * (i - 1), -0.2 * (i - 1)
        sheets.append(quad('haze', [(x0 + dx, y0, z), (x1, y0 + dy, z), (x1, y1, z), (x0 + dx, y1 + dy, z)]))
    obj = core.join(sheets, 'SM_NS_Gym_Haze')
    obj.name = obj.data.name = 'SM_NS_Gym_Haze'
    obj.data.materials.clear()
    obj.data.materials.append(me)
    obj['ns_unbaked'] = True
    print(f'[{MODULE}] SM_NS_Gym_Haze: {sum(len(p.vertices) - 2 for p in obj.data.polygons)} triangles, slots [\'ns_runtime_haze\']')
    return [obj]


# ------------------------------------------------------------------ the door leaf (PROP)

def draw_hours(s, w, h):
    """The black vinyl panel on the door's glass (spec 4.9's text, exactly), white letters."""
    s.rect(0, 0, w, h, 0x0c0c0d, rough=0.25)
    s.mid('IRONHOUSE', w / 2, h * 0.82, h * 0.15, w * 0.86, 0xffffff, 'Black', tracking=1.2, rough=0.3)
    s.rect(w * 0.12, h * 0.705, w * 0.76, 2.0, IRON_RED, rough=0.3)
    s.mid('24/7', w / 2, h * 0.56, h * 0.14, w * 0.6, IRON_RED, 'Black', tracking=1.1, rough=0.3)
    s.mid('MEMBERS ANY HOUR', w / 2, h * 0.40, h * 0.075, w * 0.86, 0xffffff, 'Black', tracking=1.2, rough=0.3)
    s.mid('STAFFED', w / 2, h * 0.26, h * 0.06, w * 0.6, 0xbfc3c8, 'Bold', tracking=1.4, rough=0.3)
    s.mid('6 AM–12 PM & 4–10 PM', w / 2, h * 0.13, h * 0.07, w * 0.86, 0xffffff, 'Black', tracking=1.05, rough=0.3)


GD_W = LEAF_X[1] - LEAF_X[0] - 0.01      # the leaf's width (0.95 m)


def door():
    """SM_NS_Gym_Door: the full-glass door leaf (origin on its hinge line at the floor, the closed leaf along +Y, its
    street face toward -X): a narrow black-painted steel stile frame, the glass ([glass]), a long pull outside with the
    hours panel (black vinyl) beside it, a push bar and a closer arm inside, three hinges."""
    k = Kit('gyg', 'SM_NS_Gym_Door', TEXTURE_SIZES, dust=0.06, low_grime=0.25, module=MODULE)
    W, Hh, T = GD_W, DOOR_H - 0.01, 0.045
    y0, y1 = 0.006, W
    st, top, bot = 0.06, 0.08, 0.13
    xs = (-T / 2, T / 2)
    k.add(bx('stile_h', xs[0], xs[1], y0, y0 + st, 0.008, Hh, 0.004), 'powder', SASH, dir=2)
    k.add(bx('stile_l', xs[0], xs[1], y1 - st, y1, 0.008, Hh, 0.004), 'powder', SASH, dir=2)
    k.add(bx('rail_t', xs[0], xs[1], y0, y1, Hh - top, Hh, 0.004), 'powder', SASH, dir=1)
    k.add(bx('rail_b', xs[0], xs[1], y0, y1, 0.008, bot, 0.004), 'powder', SASH, dir=1)
    gy0, gy1 = y0 + st, y1 - st
    for xx in (xs[0] - 0.005, xs[1] + 0.005):
        for (a, b, c_, d_) in ((gy0, gy1, bot, bot + 0.015), (gy0, gy1, Hh - top - 0.015, Hh - top), (gy0, gy0 + 0.015, bot, Hh - top), (gy1 - 0.015, gy1, bot, Hh - top)):
            k.add(bx('gstop', xx - 0.005, xx + 0.005, a, b, c_, d_, 0.002), 'powder', SASH)
    glass = quad('glass', [(0.0, gy0, bot), (0.0, gy1, bot), (0.0, gy1, Hh - top), (0.0, gy0, Hh - top)])
    k.special(glass, UNB['ns_glass'])
    # the long pull outside, the push bar and the closer inside
    py = y1 - 0.11
    for zz in (0.75, 1.55):
        k.add(rod('pull_post', (xs[0], py, zz), (xs[0] - 0.065, py, zz), 0.01, 12), 'steel', 0xffffff)
    k.add(rod('pull', (xs[0] - 0.065, py, 0.70), (xs[0] - 0.065, py, 1.60), 0.0165, 16), 'steel', 0xffffff, dir=2)
    k.add(bx('pushbar', xs[1] + 0.03, xs[1] + 0.055, y0 + 0.12, y1 - 0.08, 0.98, 1.03, 0.012), 'steel', 0xffffff, dir=1)
    for yb in (y0 + 0.14, y1 - 0.1):
        k.add(bx('bar_mount', xs[1], xs[1] + 0.05, yb - 0.035, yb + 0.035, 0.965, 1.045, 0.01), 'powder', 0x1a1a1a)
    k.add(bx('closer', xs[1], xs[1] + 0.06, y0 + 0.08, y0 + 0.40, Hh - 0.07, Hh - 0.005, 0.01), 'powder', 0x2a2b2d)
    k.add(rod('closer_arm', (xs[1] + 0.05, y0 + 0.35, Hh - 0.03), (xs[1] + 0.22, y0 + 0.56, Hh + 0.02), 0.008, 8), 'powder', 0x2a2b2d)
    for zz in (0.22, 1.10, 1.98):
        k.add(bx('hinge', xs[0] - 0.004, xs[1] + 0.004, 0.0, 0.01, zz, zz + 0.10, 0.002), 'steel', 0xffffff, dir=2)
        k.add(rod('knuckle', (0.0, 0.0, zz), (0.0, 0.0, zz + 0.10), 0.008, 12), 'steel', 0xffffff, dir=2)
    k.panel('hours', 0.30, 0.40, draw_hours)
    k.decal('hours', (-0.0025, (gy0 + gy1) / 2 - 0.08, 1.45), u=(0, -1, 0), v=(0, 0, 1), lift=0.0)
    k.grime += [((xs[0] - 0.06, py, 1.15), 0.3, 0.6), ((xs[1] + 0.04, (y0 + y1) / 2, 1.0), 0.4, 0.4), ((0.0, (y0 + y1) / 2, 0.06), 0.5, 0.5)]
    k.polish += [((xs[0] - 0.065, py, 1.15), 0.25, 0.8)]
    return [k.finish(angle=40.0, unbaked_order=['ns_glass'])]


# ================================================================== build

NAMES = {
    'front': ['SM_NS_Gym_Front', 'SM_NS_Gym_Front_Glass', 'SM_NS_Gym_Front_GlowCool', 'SM_NS_Gym_Front_GlowNeon'],
    'shell': ['SM_NS_Gym_Shell', 'SM_NS_Gym_Shell_GlowCool'],
    'door': ['SM_NS_Gym_Door'], 'treadmill': ['SM_NS_Gym_Treadmill'], 'bike': ['SM_NS_Gym_Bike'], 'bench': ['SM_NS_Gym_Bench'], 'squatrack': ['SM_NS_Gym_SquatRack'],
    'barbell': ['SM_NS_Gym_Barbell'], 'dumbbell': ['SM_NS_Gym_Dumbbell'], 'dbrack': ['SM_NS_Gym_DumbbellRack'], 'platetree': ['SM_NS_Gym_PlateTree'],
    'bag': ['SM_NS_Gym_HeavyBag'], 'desk': ['SM_NS_Gym_Desk'], 'cooler': ['SM_NS_Gym_Cooler'],
    'speedbag': ['SM_NS_Gym_SpeedBag'], 'cable': ['SM_NS_Gym_CableStation'], 'mats': ['SM_NS_Gym_Mats'], 'lockers': ['SM_NS_Gym_Lockers'],
    'whiteboard': ['SM_NS_Gym_Whiteboard'], 'fan': ['SM_NS_Gym_Fan', 'SM_NS_Gym_Fan_Blades'], 'haze': ['SM_NS_Gym_Haze'],
    'banners': ['SM_NS_Gym_Banners'],
}
BUILDERS = {
    'front': front, 'shell': shell, 'door': door, 'treadmill': treadmill, 'bike': bike, 'bench': bench, 'squatrack': squat_rack, 'barbell': barbell,
    'dumbbell': dumbbell, 'dbrack': dumbbell_rack, 'platetree': plate_tree, 'bag': heavy_bag, 'desk': desk, 'cooler': cooler,
    'speedbag': speed_bag, 'cable': cable_station, 'mats': mats, 'lockers': lockers, 'whiteboard': whiteboard, 'fan': fan, 'haze': haze,
    'banners': wall_dressing,
}
_ONLY = [s.strip() for s in os.environ.get('GYM_ONLY', '').split(',') if s.strip()]
WHICH = [k_ for k_ in NAMES if not _ONLY or k_ in _ONLY]
MESHES = [n for k_ in WHICH for n in NAMES[k_]]


def build():
    core.reset()
    random.seed(246)
    kit.make_unbaked('GY', (1.2, 2.1))     # the mirror wall's panels
    out = []
    for key in WHICH:
        out += BUILDERS[key]()
    for o in out:
        o.data.name = o.name
    return out


def bake_parts(objs):
    return kit.bake_parts(objs)


def assemble(objs):
    return kit.assemble_core(objs, MESHES)


# ================================================================== review

# (mesh, stage X, stage Y, stage yaw F[, stage Z]) as GymStage might place them
LAYOUT = [('SM_NS_Gym_Treadmill', -100 * x, 100 * TM_Y, 90) for x in TREADMILLS] + [
    ('SM_NS_Gym_Bike', -5600, 4740, 90), ('SM_NS_Gym_Bike', -5720, 4740, 90),
    ('SM_NS_Gym_Door', -100 * LEAF_X[0], 100 * GY, 90), ('SM_NS_Gym_Desk', -4760, 4580, 90), ('SM_NS_Gym_Cooler', -4603, 4420, 180),
    ('SM_NS_Gym_DumbbellRack', -5400, 3500, 90), ('SM_NS_Gym_Bench', -5250, 3700, -90), ('SM_NS_Gym_Bench', -5560, 3730, -80),
    ('SM_NS_Gym_SquatRack', -5960, 3565, 90), ('SM_NS_Gym_SquatRack', -6290, 3565, 90),
    ('SM_NS_Gym_PlateTree', -6125, 3760, 90), ('SM_NS_Gym_PlateTree', -6440, 3760, 90),
    ('SM_NS_Gym_SpeedBag', -6540, 3850, 0), ('SM_NS_Gym_CableStation', -6250, 4000, 0), ('SM_NS_Gym_Mats', -6280, 4760, 90),
    ('SM_NS_Gym_Lockers', -4560, 3720, 180), ('SM_NS_Gym_Whiteboard', -4560, 4150, 180), ('SM_NS_Gym_Fan', -6400, 3450, 90),
] + [('SM_NS_Gym_HeavyBag', -100 * hx, 100 * hy, 0, 100 * hz) for hx, hy, hz in BAG_HOOKS]

PROP_SHOTS = {
    'SM_NS_Gym_Treadmill': [((-2.5, -2.3, 1.7), (0.0, 0.0, 0.75), 30), ((1.9, 0.35, 1.75), (-1.0, 0.0, 1.15), 32), ((-0.35, 0.3, 1.6), (-1.02, 0.0, 1.32), 34)],
    'SM_NS_Gym_Bike': [((-1.9, -1.5, 1.3), (0.0, 0.0, 0.62), 32), ((0.9, 1.0, 1.5), (-0.2, 0.0, 0.8), 32)],
    'SM_NS_Gym_Bench': [((-1.6, -1.3, 1.0), (0.0, 0.0, 0.3), 32), ((0.5, 1.1, 0.95), (0.0, 0.0, 0.35), 35)],
    'SM_NS_Gym_SquatRack': [((-3.2, -2.0, 1.7), (0.0, 0.0, 1.1), 28), ((-1.2, -1.0, 1.5), (-0.5, -0.58, 1.3), 35), ((2.4, 1.6, 1.3), (0.6, 0.0, 0.7), 32)],
    'SM_NS_Gym_Barbell': [((-1.0, -1.5, 1.3), (0.0, -0.55, 1.0), 35)],
    'SM_NS_Gym_Dumbbell': [((-0.45, -0.35, 0.68), (0.0, 0.0, 0.5), 40)],
    'SM_NS_Gym_DumbbellRack': [((-3.0, -1.9, 1.5), (0.0, 0.0, 0.6), 30), ((-1.0, -0.55, 1.05), (0.0, -0.6, 0.65), 36)],
    'SM_NS_Gym_PlateTree': [((-1.9, -1.1, 1.25), (0.0, 0.0, 0.65), 32)],
    'SM_NS_Gym_HeavyBag': [((-2.4, -0.9, 1.5), (0.0, 0.0, 1.35), 30), ((-0.9, 0.35, 2.35), (0.0, 0.0, 2.15), 35)],
    'SM_NS_Gym_Desk': [((-2.7, -1.6, 1.75), (0.0, 0.0, 0.9), 30), ((-1.0, 0.45, 1.55), (-0.3, 0.25, 1.15), 34), ((1.9, 1.4, 1.65), (0.0, 0.0, 0.9), 32)],
    'SM_NS_Gym_Cooler': [((-2.1, -1.1, 1.45), (0.0, 0.0, 1.0), 30), ((-0.95, 0.25, 1.5), (-0.35, 0.0, 1.2), 32)],
    'SM_NS_Gym_Door': [((-2.2, 1.6, 1.4), (0.0, 0.48, 1.15), 32), ((1.9, 1.3, 1.45), (0.0, 0.48, 1.15), 32), ((-0.75, 0.45, 1.5), (0.0, 0.5, 1.4), 38)],
    'SM_NS_Gym_SpeedBag': [((-2.0, -0.9, 1.6), (-0.4, 0.0, 1.65), 32)],
    'SM_NS_Gym_CableStation': [((-3.2, -1.6, 1.5), (0.0, 0.0, 1.1), 28)],
    'SM_NS_Gym_Mats': [((-3.0, -2.4, 1.7), (0.0, 0.0, 0.2), 30)],
    'SM_NS_Gym_Lockers': [((-3.0, -1.4, 1.5), (-0.45, 0.0, 1.0), 28)],
    'SM_NS_Gym_Whiteboard': [((-2.8, 0.0, 1.6), (0.0, 0.0, 1.7), 28)],
    'SM_NS_Gym_Fan': [((-1.8, -1.1, 2.7), (-0.3, 0.0, 3.2), 32)],
}
PLACE = {'SM_NS_Gym_Barbell': Matrix.Translation((0.0, 0.0, 1.0)), 'SM_NS_Gym_Dumbbell': Matrix.Translation((0.0, 0.0, 0.5)),
         'SM_NS_Gym_HeavyBag': Matrix.Translation((0.0, 0.0, 2.47))}


def _find(objs, name):
    o = next((o for o in objs if o.name == name), None)
    return o if o is not None else kit.import_glb(name)


def dress_dbrack(objs, base):
    src = _find(objs, 'SM_NS_Gym_Dumbbell')
    out = []
    if src is None:
        return out
    for i, (x, y, z, yaw, pitch) in enumerate(dumbbell_points()):
        if i in (6, 13):
            continue
        sc = 0.8 + 0.06 * (i % 10)
        d = src.copy()
        core.link(d)
        d.matrix_world = base @ M(x, y, z, yaw, 0.0, 0.0) @ Matrix.Rotation(math.radians(pitch), 4, 'Y') @ Matrix.Diagonal((sc, 1.0, sc, 1.0))
        d.hide_render = False
        out.append(d)
    src.hide_render = True
    return out


def dress_rack_bar(objs, base):
    src = _find(objs, 'SM_NS_Gym_Barbell')
    if src is None:
        return []
    d = src.copy()
    core.link(d)
    d.matrix_world = base @ Matrix.Translation(RACK_BAR)
    d.hide_render = False
    return [d]


def stocked_cooler(base):
    out = []
    for row, z in (('SM_Stock_CoolerRow_Water', COOLER_SHELF_Z[0]), ('SM_Stock_CoolerRow_Energy', COOLER_SHELF_Z[1]), ('SM_Stock_CoolerRow_Water', COOLER_SHELF_Z[2]),
                   ('SM_Stock_CoolerRow_Energy', COOLER_SHELF_Z[3])):
        o = kit.import_glb(row)
        if o is None:
            continue
        o.matrix_world = base @ M(0.0, 0.0, z, -90)
        out.append(o)
    return out


def gym_review(objs):
    """art/review/ns_gym_store.jpg: the front by night (the treadmill row lit, the letters and the neon on) and by day,
    the door and its fob reader, the letters close, and the room from the door, from the back and at the weights."""
    sc = bpy.context.scene
    by = {o.name: o for o in objs}
    temp = []
    saved_mw = {o.name: (o.matrix_world.copy(), o.hide_render) for o in objs}
    glow_cols = {'GlowCool': (kit.kelvin(5000), 8.0), 'GlowNeon': ((1.0, 0.08, 0.05), 14.0), 'GlowWarm': (kit.kelvin(2800), 6.0)}
    swaps = kit.swap_for_review(objs, glow_cols)
    street = kit.StreetMock(FRONT_Y, [(OX0, OX1, -0.31, TOPZ)], (OX0 - 9.0, OX1 + 9.0), top=9.0, brick=0x6e3a2c)
    try:
        used = {}
        for row in LAYOUT:
            name, X, Y, F = row[:4]
            Z = row[4] if len(row) > 4 else 0.0
            src = by.get(name)
            if src is None:
                continue
            o = src if name not in used else src.copy()
            if name in used:
                core.link(o)
                temp.append(o)
            used[name] = True
            o.matrix_world = S2B(X, Y, F, Z / 100.0)
            o.hide_render = False
            if name == 'SM_NS_Gym_DumbbellRack':
                temp += dress_dbrack(objs, o.matrix_world)
            elif name == 'SM_NS_Gym_SquatRack':
                temp += dress_rack_bar(objs, o.matrix_world)
            elif name == 'SM_NS_Gym_Cooler':
                temp += stocked_cooler(o.matrix_world)
            elif name == 'SM_NS_Gym_Fan' and 'SM_NS_Gym_Fan_Blades' in by:
                fb = by['SM_NS_Gym_Fan_Blades']
                fb.matrix_world = o.matrix_world @ Matrix.Translation(FAN_HUB)
                fb.hide_render = False
        for n in ('SM_NS_Gym_Barbell', 'SM_NS_Gym_Dumbbell'):
            if n in by and n not in used:
                by[n].hide_render = True
        # a runner on the second treadmill, a figure at the desk, one on the sidewalk
        temp.append(kit.scale_figure((TREADMILLS[1], TM_Y - 0.1, 0.22), 90))
        temp.append(kit.scale_figure((47.6, 44.95, 0.0), 90))
        temp.append(kit.scale_figure((52.5, 51.6, 0.0), -60))
        inside = []
        for hx, hy in HIGHBAYS:
            inside.append(kit.area_light('rv_highbay', (hx, hy, 3.99), 0.3, 260.0, kit.kelvin(5000), shape='DISK'))
        temp += inside
        street.lamp((49.0, 53.4, 7.6), (49.0, 51.5, 0.0), 4000, 3500.0)
        street.lamp((60.5, 53.4, 7.6), (60.5, 51.5, 0.0), 4000, 3500.0)
        cd = bpy.data.cameras.new('rv_cam')
        cd.clip_start = 0.02
        cd.clip_end = 300.0
        cam = core.link(bpy.data.objects.new('rv_cam', cd))
        sc.camera = cam
        temp.append(cam)
        tmp_dir = os.path.join(core.BUILD_DIR, 'review', 'ns_gym_store')
        os.makedirs(tmp_dir, exist_ok=True)
        paths = []
        street.night()
        paths.append(kit.shot(cam, os.path.join(tmp_dir, 'night_front.png'), (55.5, 60.0, 1.7), (55.5, 49.5, 2.3), 20, exposure=1.0))
        paths.append(kit.shot(cam, os.path.join(tmp_dir, 'night_angle.png'), (41.5, 53.3, 1.65), (54.0, 49.3, 2.0), 24, exposure=1.0))
        paths.append(kit.shot(cam, os.path.join(tmp_dir, 'night_door.png'), (49.2, 51.3, 1.6), (47.3, 49.4, 1.4), 30, exposure=1.0))
        street.day()
        for L in inside:
            L.data.energy *= 0.8
        paths.append(kit.shot(cam, os.path.join(tmp_dir, 'day_front.png'), (55.5, 60.0, 1.7), (55.5, 49.5, 2.3), 20, exposure=-0.3))
        paths.append(kit.shot(cam, os.path.join(tmp_dir, 'day_angle.png'), (69.5, 53.0, 1.65), (57.0, 49.3, 2.0), 24, exposure=-0.3))
        paths.append(kit.shot(cam, os.path.join(tmp_dir, 'day_letters.png'), (51.5, 52.6, 2.6), (54.6, 49.5, 4.0), 28, exposure=-0.3))
        street.night()
        paths.append(kit.shot(cam, os.path.join(tmp_dir, 'in_door.png'), (47.0, 48.7, 1.7), (60.0, 37.0, 1.4), 16, exposure=0.4))
        paths.append(kit.shot(cam, os.path.join(tmp_dir, 'in_back.png'), (63.8, 36.2, 1.8), (50.0, 48.0, 1.6), 16, exposure=0.4))
        paths.append(kit.shot(cam, os.path.join(tmp_dir, 'in_weights.png'), (57.0, 42.6, 1.7), (57.5, 35.0, 1.1), 20, exposure=0.4))
        paths.append(kit.shot(cam, os.path.join(tmp_dir, 'in_treadmills.png'), (55.6, 45.6, 1.65), (50.0, 48.4, 1.15), 24, exposure=0.4))
        paths.append(kit.shot(cam, os.path.join(tmp_dir, 'in_bags.png'), (60.6, 42.2, 1.7), (64.8, 44.2, 1.5), 22, exposure=0.4))
        paths.append(kit.shot(cam, os.path.join(tmp_dir, 'in_west.png'), (52.4, 40.4, 1.7), (45.8, 39.6, 1.4), 22, exposure=0.4))
        kit.compose('ns_gym_store', paths, 3, module=MODULE)
    finally:
        kit.restore_swaps(swaps)
        street.close()
        for o in temp:
            try:
                bpy.data.objects.remove(o)
            except ReferenceError:
                pass
        for n, (mw, hid) in saved_mw.items():
            ob = bpy.data.objects.get(n)
            if ob is not None:
                ob.matrix_world, ob.hide_render = mw, hid
        sc.camera = None
        sc.view_settings.exposure = 0.0


def pose_for_review(objs):
    """The store (art/review/ns_gym_store.jpg) when the site meshes are built, close looks at the equipment
    (ns_gym_props.jpg), then the standard sheet of the equipment in a row."""
    global REVIEW_VIEWS
    names = {o.name for o in objs}
    if os.environ.get('GYM_REVIEW', '1') != '0':
        if names & {'SM_NS_Gym_Front', 'SM_NS_Gym_Shell'}:
            gym_review(objs)

        def dress(n):
            if n == 'SM_NS_Gym_DumbbellRack':
                return dress_dbrack(objs, Matrix())
            if n == 'SM_NS_Gym_SquatRack':
                return dress_rack_bar(objs, Matrix())
            if n == 'SM_NS_Gym_Cooler':
                return stocked_cooler(Matrix())
            if n == 'SM_NS_Gym_Fan':
                b = _find(objs, 'SM_NS_Gym_Fan_Blades')
                if b is not None:
                    d = b.copy()
                    core.link(d)
                    d.matrix_world = Matrix.Translation(FAN_HUB)
                    d.hide_render = False
                    return [d]
            return []
        room = kit.ReviewRoom(floor_hex=0x26272a, wall_hex=0xe6e3dc, kelvin_k=5000, energy=80.0)
        wall = {'SM_NS_Gym_Door': None, 'SM_NS_Gym_HeavyBag': 0.45, 'SM_NS_Gym_Barbell': None, 'SM_NS_Gym_Dumbbell': None, 'SM_NS_Gym_SpeedBag': 0.0, 'SM_NS_Gym_Lockers': 0.0,
                'SM_NS_Gym_Whiteboard': 0.0, 'SM_NS_Gym_Fan': 0.0}
        kit.detail_review(objs, PROP_SHOTS, 'ns_gym_props', MODULE, dress=dress, wall=wall, room=room, place=PLACE)
    props = [o for o in objs if o.name in PROP_SHOTS]
    if not props:
        REVIEW_VIEWS = [('front', 0, 8, 0.6, (55.5, 49.5, 2.0)), ('angle', -40, 12, 0.6, (55.5, 49.5, 2.0)), ('detail', -10, 10, 0.25, (47.3, 49.0, 1.4))]
        return
    for o in objs:
        if o not in props:
            o.hide_render = True
    objs[:] = props
    REVIEW_VIEWS = kit.grid_layout(props, PLACE)
