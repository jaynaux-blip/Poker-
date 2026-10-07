"""SM_NS_Pawn_*: NORTHSIDE PAWN & LOAN, 232 Market St (M-S3, stage X -3700..-4500 on Market's south side): its
storefront in M-S3's opening and its interior (NORTHSIDE spec §4.6b / §5.8 / §4.9, art-shops-b).

FRAMES (§0.4). SITE meshes: Blender origin = the stage origin; placed at the stage's transform, yaw 180:
stage (X, Y, Z) cm = (-100 x, 100 y, 100 z). PROP meshes: their own frame, front toward Blender -X, origin as listed;
placed at L (stage cm, below) with yaw F + 180. Every PROP here faces Market (north, stage +Y, F = 90): yaw 270.
A hinged PROP opens by adding positive yaw (the shop door swings in, the safe's door swings out toward the window).
The code builds in a shop frame (x = depth from the front's plane, y = -X/100, z up; see `SITE_M`) and moves SITE
meshes into the SITE frame at the end. Every print reads correctly from outside its face in Blender (checked on
art/review/ns_pawn_detail_*.jpg).

THE PLAN (stage cm). The opening X -3760..-4440 on the Y 4950 plane, Z 0..380 (the sign band over it is art-fronts').
The room X -3740 (west) .. -4460 (east) by Y 4920 .. 3600, the pressed tin at Z 360.
- Storefront: the roll-down grille's curtain at Y 4941.5 (hood Z 345..380); the display glass at Y 4930 with fixed
  bars inside it at Y 4920 and the blue tube between them; the display platform X -3900..-4440, Y 4920..4830, deck
  Z 55; the door's frame X -3792..-3886 recessed to Y 4860 (terrazzo vestibule, brass LOANS inlay), the leaf hinged on
  the west jamb, swinging in; the outside buzzer on the east jamb; the gold-leaf 232 on the transom.
- Sales floor Y 4920..4380: the guitar wall on the west wall (Y 4820..4400, the guitars out to X -3790); the steel
  shelving on the east wall (Y 4810..4390, its front at X -4400): 6.1 m clear between them. The 2.4 m aisle X -3740..
  -3980 runs from the door to the loan window, clear. The P1 floor pieces stand in an island down the middle (tower
  case X -4115 Y 4770, slot machine -4105/4688, tool chest -4110/4615, bicycle -4235/4525..4695, jacket bust
  -4115/4525), under the ceiling fan at X -4100 Y 4650.
- Counter Y 4380..4300: four glass cases from the east wall to X -4040 (curios -4040..-4145, watches -4145..-4250,
  jewelry -4250..-4355 and -4355..-4460), the lift-up flap X -3980..-4040 at its west end.
- Loan window: the cage's bay X -3760..-3960 in the Y 4300 line, the bulletproof glass X -3810..-3910 Z 110..208 over
  the stainless pass-through tray and the speak-through.
- Behind: the cage X -3740..-4100, Y 4300..3600 (its gate behind the flap X -3985..-4035; a gate in its east face Y
  4280..4190): Augie's desk X -3745..-3966 Y 4294..4226 under the window, the safe at X -3860 Y 4172 facing the window,
  the pledge shelving Y 4095..4045 and down the west wall to Y 3625. East of the cage: the partition at Y 4050 (the wall
  of clocks over the counter; its cased opening X -4115..-4205), the repair bench against it (X -4230..-4445, P1), the
  back stock behind it, the steel back-room door X -4330..-4425 in the back wall (not enterable).

MESHES (tris from the build log; slot order as exported, Unreal names them M_<Mesh>_<i>):
  SITE  SM_NS_Pawn_Front         680 x 124 x 382  ~38k   baked: frame, tile, display, print
  SITE  SM_NS_Pawn_Front_Glass   display, return and transom panes                 18   [glass] (ns_glass)
  PROP  SM_NS_Pawn_Door          origin on the hinge line at the floor (the leaf's inside face), leaf along +Y of its
                                 frame; L (-3792.5, 4854.5, 0)  19 x 94 x 217  2.0k  baked: body, print; [glass]; [screen]
                                 ns_screen_pawn_openclosed (the OPEN/CLOSED card, UV 0..1 as read from the street)
  PROP  SM_NS_Pawn_Grille        origin at the curtain's top middle; L (-4100, 4941.5, 345); hangs 344 down to the
                                 sidewalk; the game scales it in Z (1 down, 0 rolled into the hood)  5.9k  baked
  PROP  SM_NS_Pawn_Sign          origin at the bracket's wall plate on the west pier's face; L (-3730, 4950, 425); the
                                 balls, the scrolled bracket, the gooseneck lamp over them, the tube's clips and
                                 electrode boots round the window  5.3k  baked
  PROP  SM_NS_Pawn_Sign_GlowNeon the blue tube round the barred window (same frame and L as the Sign)  0.7k  [glow:Neon]
  PROP  SM_NS_Pawn_Sign_GlowWarm the gooseneck lamp's lens over the balls (same frame and L)  92  [glow:Warm]
  SITE  SM_NS_Pawn_Shell         720 x 1323 x 362  ~18k  baked: floor (2048 x 4096, VCT laid over the plan with the
                                 worn paths), wall (painted-over brick, partition, trim, signs, clocks), ceil (2048 x
                                 4096, the tin), print
  SITE  SM_NS_Pawn_Shell_GlowCool the six strips' tubes  0.5k  [glow:Cool]
  PROP  SM_NS_Pawn_FanBlades     origin at the hub, axis +Z, turns about Z; L (-4100, 4650, 312)  2.1k  baked
  SITE  SM_NS_Pawn_Counter       480 x 72 x 132  ~70k  baked: body (glass_case frame, satin black 0x1a1b1d), jewel, print;
                                 [glass] (case fronts, tops, the staff side's sliding panes): Nanite off (§0.5)
  SITE  SM_NS_Pawn_Counter_GlowCool LED strips under the cases' rails  96  [glow:Cool]
  SITE  SM_NS_Pawn_Cage          362 x 730 x 360  ~44k  baked: body, goods, print; [glass] (the window): Nanite off
  SITE  SM_NS_Pawn_Cage_GlowWarm the desk lamp's lens  92  [glow:Warm]
  PROP  SM_NS_Pawn_SafeDoor      origin on its hinge axis at the floor, its front toward -X (the window); L (-3892.6,
                                 4209.2, 0); opens by +yaw up to ~100  4.9k  baked: body, print
  SITE  SM_NS_Pawn_GuitarWall    101 x 424 x 334  ~85k  baked: body (4096), print
  SITE  SM_NS_Pawn_Shelves       61 x 1086 x 249  ~36k  baked: body (4096), print; [screen] ns_screen_pawn_tv (the
                                 demo TV, UV 0..1 across the picture)
  SITE  SM_NS_Pawn_Tickets       the ticket book on the counter, the spike of pink carbons on Augie's desk  0.8k  baked
  PROP  SM_NS_Pawn_Ticket        the loose ticket for the hero's hand: origin at the grip, +X along the slip, face up
                                 (+Z); 15 x 9 cm  64  baked: print (the back); [screen] ns_screen_pawn_ticket (the face,
                                 core draws pawnticket:<n>, UV 0..1 as read)
  PROP  SM_NS_Pawn_Tag0..7       eight hanging-tag shapes for instancing on the stock: origin at the string's top, the
                                 card hanging below, its face toward -X  ~80..180 each  baked: body (the string, the
                                 back); [screen] ns_screen_pawn_tag (the face, core draws the price, UV 0..1 across it)
  PROP  SM_NS_Pawn_Loupe         origin at the grip, +X along it  1.2k  baked
  SITE  SM_NS_Pawn_Bench         P1  215 x 122 x 141  ~9k  baked: body, print
  SITE  SM_NS_Pawn_Bench_GlowCool the magnifier lamp's ring  0.6k  [glow:Cool]
  SITE  SM_NS_Pawn_Floor         P1  192 x 285 x 180  ~13k  baked: body, print; [glass] (the tower case): Nanite off

MARKS (stage cm; facing as the stage direction the thing's front looks):
  Item     (-4198, 4352, 97.5)  on the watch case's glass: where Augie sets the appraised thing down
  Tray     (-4228, 4352, 99.1)  the velvet tray on the glass
  C0..C5   curio sockets on the curio case's deck, facing north (to the customer), each ~30 x 23 cm with 57 cm clear:
           C0 (-4062, 4357, 36.5)  C1 (-4093, 4357, 36.5)  C2 (-4124, 4357, 36.5)
           C3 (-4062, 4334, 36.5)  C4 (-4093, 4334, 36.5)  C5 (-4124, 4334, 36.5)
  G0..G11  gear sockets in the shelving's two middle bays (their own units, shelves 54 cm apart), facing west (+X,
           into the room), each a 50 x 50 cm spot with 50 cm clear over it; a wider piece (the 27-inch monitor) takes a
           pair and stands at its middle: (G0,G1) (G2,G3) ... The chair fits none (it stays off the shelf).
           G0 (-4428, 4677.2, 12)   G1 (-4428, 4627.8, 12)   G2 (-4428, 4677.2, 66)   G3 (-4428, 4627.8, 66)
           G4 (-4428, 4677.2, 120)  G5 (-4428, 4627.8, 120)  G6 (-4428, 4572.2, 12)   G7 (-4428, 4522.8, 12)
           G8 (-4428, 4572.2, 66)   G9 (-4428, 4522.8, 66)   G10 (-4428, 4572.2, 120) G11 (-4428, 4522.8, 120)
  BuzzerButton  (-3994, 4308.5, 89.3)  under the counter's lip by the flap, on the staff side (pawn-door's buzzer)
  HoursDecal    centered (-3940, 4930.3, 152), 30 x 20, on the display glass's inside face east of the door, read
                from the street: left clear for the core-drawn hours decal
  LiveClocks    two clocks in the wall of clocks have no hands baked; PawnStage draws hour and minute hands from the
                world time on their dials (facing north): center (-4330, 4060, 248) dial radius 13.3, and center
                (-4398, 4060, 258) dial radius 16.3 (a brass center pin stands 8 mm proud of each dial)

LIGHTS (for PawnStage): six 8 ft strips at Z 302, centered (-3940, 4760) (-4260, 4760) (-3940, 4520) (-4260, 4520)
(-4280, 4180) (-3920, 3960), running along Y; the cases' LEDs X -4043..-4457 at Y 4376 and 4312, Z 93.5 (two rect
lights); the desk lamp at (-3938, 4255, 134.5), pointing down at the desk; the blue tube X -3910..-4430, Y 4924,
Z 62..264; the gooseneck shade over the gold balls at (-3730, 5012, 446) (balls at Z 362..386); the bench's magnifier
ring at (-4305, 4098, 132).
"""
import math
import os
import random

import bmesh
import bpy  # noqa: F401
import numpy as np
from mathutils import Matrix, Vector

from artkit import core, parts, review, texmaps
from artkit.shade import convex_edges, mix_float, noise, object_coords, ramp, scaled, smooth_less, vec
from artkit.sheet import Sheet

if os.environ.get('PAWN_CPU'):
    # A build machine whose GPU is shared by many Blender processes can stall Cycles for many minutes on each render
    # and bake: PAWN_CPU=1 renders and bakes this asset on the CPU instead.
    core.cycles_device = lambda *a, **k: 'CPU'

# ================================================================== the contract: frames, lines, layout
#
# Every SITE mesh is built in the shop's own frame and moved into the SITE frame (spec §0.4) at the end by SITE_M:
#   shop x = depth into the shop from the front's plane, Market's south building line (stage Y 4950): x = (4950 - Y)/100
#   shop y = Blender SITE x = -X/100: west (the opening's west jamb, 37.60) .. east (its east jamb, 44.40)
#   shop z = up (Z 0 the sidewalk's top and the shop's floor)
#   Blender SITE (x, y, z) = (shop y, 49.50 - shop x, shop z), i.e. SITE_M = T(0, 49.50, 0) @ RZ(-90)
# so the street is at shop -x, the west wall (the guitar wall) at low shop y, the east wall (the shelves) at high y.
# Seen from Market's south sidewalk (facing south), west is on your right.

FRONT_Y = 49.50                                    # Blender y of the front's plane (stage Y 4950)
SITE_M = Matrix.Translation((0.0, FRONT_Y, 0.0)) @ Matrix.Rotation(math.radians(-90.0), 4, 'Z')
PLOT_Y0, PLOT_Y1 = 37.00, 45.00                    # M-S3: stage X -3700 .. -4500
OPEN_Y0, OPEN_Y1 = 37.60, 44.40                    # the opening art-fronts leaves clear: stage X -3760 .. -4440 (§2.4)
OPEN_Z1 = 3.80                                     # its head (the sign band, NORTHSIDE PAWN & LOAN, is art-fronts')
RX0, RX1 = 0.30, 13.50                             # the room: stage Y 4920 (front) .. 3600 (back)
RY0, RY1 = 37.40, 44.60                            # the room: stage X -3740 (west) .. -4460 (east)
CEIL = 3.60                                        # the pressed-tin ceiling
# the storefront (in the opening's 30 cm reveal and behind it)
GRILLE_X = 0.085                                   # the roll-down grille's curtain plane (its guides in the jambs)
GLASS_X = 0.20                                     # the display glass
BARS_X = 0.30                                      # the fixed bars, inside the glass (§4.6b)
TUBE_X = 0.26                                      # the blue tube, between the glass and the bars
HOOD_Z = 3.45                                      # the grille hood fills the opening's head from here to OPEN_Z1
SILL_Z = 0.55                                      # the bulkhead's top and the display platform's deck
TRANSOM_Z = (2.70, 2.78)                           # the transom bar
VEST_X = 0.90                                      # the door's plane: recessed 0.9 m (§2.4)
DOOR_Y = (37.92, 38.86)                            # the door frame's clear opening (stage X -3792 .. -3886)
DOOR_CLEAR = (37.925, 38.855)                      # the leaf's extent: hinged on the west jamb (low y), swings in (+x)
DOOR_H = 2.18                                      # the leaf's top
RETURN_Y = 39.00                                   # the display window's return pane: the vestibule's east side
MULL_Y = [39.00, 40.80, 42.60, OPEN_Y1]            # the display window's verticals: the corner post, two mullions, the jamb
DECK_X = (0.30, 1.20)                              # the display platform (§4.6b: "an inside platform")
# inside (§4.6b's plan, front to back)
GUITAR_X = (1.30, 5.50)                            # the guitar wall's slatwall on the west wall
SHELF_X = (1.40, 5.60)                             # the steel shelving down the east wall
SHELF_Y = (44.00, RY1)
COUNTER_X = (5.70, 6.50)                           # the counter (stage Y 4380 .. 4300)
CASE_X = (5.74, 6.38)                              # the glass cases' customer face .. staff face
CASE_Y = [40.40, 41.45, 42.50, 43.55, RY1]         # four cases west to east: curios, watches, jewelry, jewelry
FLAP_Y = (39.80, 40.40)                            # the lift-up flap at the counter's west end (stage X -3980 ..)
CAGE_X = 6.50                                      # the cage's front: the Y 4300 line
CAGE_Y1 = 41.00                                    # the cage's east face (stage X -4100)
WINDOW_Y = (37.60, 39.60)                          # the loan window's bay in the cage's front (stage X -3760 .. -3960)
BANDIT_Y = (38.10, 39.10)                          # the bulletproof glass in it
CAGE_GATE = (39.85, 40.35)                         # the cage's gate behind the counter's flap
CAGE_GATE_E = (6.70, 7.60)                         # the gate in the cage's east face, to the space behind the counter
PARTITION_X = 9.00                                 # the back-stock wall behind the counter (the wall of clocks)
PARTITION_DOOR = (41.15, 42.05)                    # its cased opening to the back stock
BACKDOOR_Y = (43.30, 44.25)                        # the steel door to the back room in the back wall (not enterable)
FAN_AT = (3.00, 41.00)                             # the ceiling fan's hub over the sales floor
STRIPS = [(1.90, 39.40), (1.90, 42.60), (4.30, 39.40), (4.30, 42.60), (7.70, 42.80), (9.90, 39.20)]   # six 8 ft strips
SAFE_AT = (7.78, 38.60, -90.0)                     # the floor safe: at Augie's back at the window, its door toward it (-x)

INK = 0x161616
PAPER = 0xf1eee4


def H(h, a=1.0):
    return core.hex_linear(h, a)


def lin3(h):
    c = core.hex_linear(h)
    return (c[0], c[1], c[2])


def stage_cm(p):
    """A shop-frame point -> stage cm (for the docstring's marks)."""
    s = SITE_M @ Vector(p)
    return (round(-100 * s.x, 1), round(100 * s.y, 1), round(100 * s.z, 1))


# ================================================================== mesh registry

NAMES = {
    'front': ['SM_NS_Pawn_Front', 'SM_NS_Pawn_Front_Glass'],
    'door': ['SM_NS_Pawn_Door'],
    'grille': ['SM_NS_Pawn_Grille'],
    'sign': ['SM_NS_Pawn_Sign', 'SM_NS_Pawn_Sign_GlowNeon', 'SM_NS_Pawn_Sign_GlowWarm'],
    'shell': ['SM_NS_Pawn_Shell', 'SM_NS_Pawn_Shell_GlowCool', 'SM_NS_Pawn_FanBlades'],
    'counter': ['SM_NS_Pawn_Counter', 'SM_NS_Pawn_Counter_GlowCool'],
    'cage': ['SM_NS_Pawn_Cage', 'SM_NS_Pawn_Cage_GlowWarm', 'SM_NS_Pawn_SafeDoor'],
    'guitars': ['SM_NS_Pawn_GuitarWall'],
    'shelves': ['SM_NS_Pawn_Shelves'],
    'tickets': ['SM_NS_Pawn_Tickets', 'SM_NS_Pawn_Ticket'] + [f'SM_NS_Pawn_Tag{i}' for i in range(8)],
    'loupe': ['SM_NS_Pawn_Loupe'],
    'bench': ['SM_NS_Pawn_Bench', 'SM_NS_Pawn_Bench_GlowCool'],
    'floor': ['SM_NS_Pawn_Floor'],
}
_ONLY = [s.strip() for s in os.environ.get('PAWN_ONLY', '').split(',') if s.strip()]
WHICH = [k for k in NAMES if not _ONLY or k in _ONLY]
MESHES = [n for k in WHICH for n in NAMES[k]]
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {
    'pwf_frame': 2048, 'pwf_tile': 1024, 'pwf_display': 2048, 'pwf_print': 2048,
    'pwd_body': 1024, 'pwd_print': 1024,
    'pwr_body': 1024,
    'pwn_body': 1024, 'pwn_print': 512,
    'pws_floor': (2048, 4096), 'pws_wall': 4096, 'pws_ceil': (2048, 4096), 'pws_print': 2048,
    'pwfb_body': 512,
    'pwc_body': 2048, 'pwc_jewel': 2048, 'pwc_print': 2048,
    'pwk_body': 2048, 'pwk_goods': 2048, 'pwk_print': 2048,
    'pwsd_body': 1024, 'pwsd_print': 1024,
    'pwg_body': 4096, 'pwg_print': 1024,
    'pwe_body': 4096, 'pwe_print': 2048,
    'pwt_body': 512, 'pwt_print': 1024,
    'pwx_body': 256, 'pwx_print': 512,
    **{f'pwtg{i}_body': 256 for i in range(8)},
    'pwl_body': 512,
    'pwb_body': 2048, 'pwb_print': 1024,
    'pwo_body': 2048, 'pwo_print': 1024,
}
if os.environ.get('PAWN_TEXSCALE'):
    # a quick look-see build (PAWN_TEXSCALE=0.5): every texture at a fraction of its size
    _f = float(os.environ['PAWN_TEXSCALE'])
    TEXTURE_SIZES = {n: (tuple(max(256, int(c * _f)) for c in v) if isinstance(v, tuple) else max(256, int(v * _f)))
                     for n, v in TEXTURE_SIZES.items()}
# Ambient occlusion bake distance per mesh (build.py asks the module once per mesh, while only that mesh renders).
AO_BY_MESH = {'SM_NS_Pawn_Shell': 0.5, 'SM_NS_Pawn_Front': 0.18, 'SM_NS_Pawn_Counter': 0.08, 'SM_NS_Pawn_Cage': 0.15,
              'SM_NS_Pawn_SafeDoor': 0.03, 'SM_NS_Pawn_GuitarWall': 0.07, 'SM_NS_Pawn_Shelves': 0.08, 'SM_NS_Pawn_Door': 0.04,
              'SM_NS_Pawn_Grille': 0.03, 'SM_NS_Pawn_Sign': 0.05, 'SM_NS_Pawn_Tickets': 0.01, 'SM_NS_Pawn_Ticket': 0.004,
              'SM_NS_Pawn_Loupe': 0.005, 'SM_NS_Pawn_FanBlades': 0.02, 'SM_NS_Pawn_Bench': 0.06, 'SM_NS_Pawn_Floor': 0.06}
REVIEW_SCREEN_LIGHT = False
REVIEW_VIEWS = None   # set by pose_for_review

UNBAKED_MATS = ('ns_glass', 'ns_screen_pawn_openclosed', 'ns_screen_pawn_tv', 'ns_screen_pawn_ticket', 'ns_screen_pawn_tag')
GLASS_MESHES = {'SM_NS_Pawn_Front_Glass'}      # the only glass-only mesh; the counter's, the cage's and the tower case's glass are
                                              # [glass] slots in their meshes (§0.5: Nanite off on those)
GLOW_MESHES = {n for v in NAMES.values() for n in v if '_Glow' in n}
PROP_MESHES = {'SM_NS_Pawn_Door', 'SM_NS_Pawn_Grille', 'SM_NS_Pawn_Sign', 'SM_NS_Pawn_Sign_GlowNeon', 'SM_NS_Pawn_Sign_GlowWarm',
               'SM_NS_Pawn_FanBlades', 'SM_NS_Pawn_SafeDoor', 'SM_NS_Pawn_Ticket', 'SM_NS_Pawn_Loupe'} | {f'SM_NS_Pawn_Tag{i}' for i in range(8)}
_BAKED = []           # the meshes build.py bakes, for the per-mesh AO distance


def __getattr__(name):
    """build.py reads AO_DISTANCE once per mesh it bakes, while only that mesh renders: answer for that mesh."""
    if name == 'AO_DISTANCE':
        live = [o for o in _BAKED if _alive(o) and not o.hide_render]
        if len(live) == 1:
            return AO_BY_MESH.get(live[0].name, 0.06)
        return 0.06
    raise AttributeError(name)


def _alive(o):
    try:
        o.name
        return True
    except ReferenceError:
        return False


# ================================================================== geometry helpers

def set_attr(obj, name, value):
    """A per-face attribute on every face of obj (a float, or a 3-vector)."""
    me = obj.data
    n = len(me.polygons)
    is_vec = isinstance(value, (tuple, list, Vector))
    a = me.attributes.get(name)
    if a is None:
        a = me.attributes.new(name, 'FLOAT_VECTOR' if is_vec else 'FLOAT', 'FACE')
    if is_vec:
        v = tuple(value)
        a.data.foreach_set('vector', [float(v[i % 3]) for i in range(n * 3)])
    else:
        a.data.foreach_set('value', [float(value)] * n)


def bx(name, x0, x1, y0, y1, z0, z1, r=0.0, seg=2):
    """An axis-aligned box between the given bounds, its edges rounded by r."""
    x0, x1 = min(x0, x1), max(x0, x1)
    y0, y1 = min(y0, y1), max(y0, y1)
    z0, z1 = min(z0, z1), max(z0, z1)
    if 0.0 < r < 0.0025:
        seg = 1
    return parts.rbox(name, ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), (max(x1 - x0, 1e-5), max(y1 - y0, 1e-5), max(z1 - z0, 1e-5)), r, seg)


def obox(name, center, size, rot_z=0.0, rot_x=0.0, rot_y=0.0, r=0.0, seg=2):
    """A box of full size centered at center, turned about Z (then X, Y) in degrees."""
    m = Matrix.Rotation(math.radians(rot_z), 4, 'Z') @ Matrix.Rotation(math.radians(rot_y), 4, 'Y') @ Matrix.Rotation(math.radians(rot_x), 4, 'X')
    if 0.0 < r < 0.0025:
        seg = 1
    o = parts.rbox(name, (0.0, 0.0, 0.0), size, r, seg)
    o.data.transform(Matrix.Translation(Vector(center)) @ m)
    return o


def weld(obj, dist=1e-6):
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=dist)
    bm.to_mesh(obj.data)
    bm.free()


def lathe_axis(name, profile, matrix, segments=48):
    """A solid of revolution about local Z whose (r, z) profile starts and ends on the axis, moved by matrix."""
    obj = core.lathe(name, profile, segments)
    weld(obj)
    core.orient_normals(obj)
    obj.data.transform(matrix)
    return obj


def tube(name, pts, r, n=12, closed=False):
    return core.sweep(name, pts, parts.circle(r, n), closed=closed, caps=not closed)


def wires(name, segs, r):
    """Straight wires (p, q) as one mesh: four-sided, open-ended (their ends run into frames), smooth shaded."""
    bm = bmesh.new()
    for p, q in segs:
        P, Q = Vector(p), Vector(q)
        ax = (Q - P).normalized()
        u = ax.cross(Vector((0, 0, 1)) if abs(ax.z) < 0.95 else Vector((1, 0, 0))).normalized() * r
        v = ax.cross(u).normalized() * r
        ring = [u, v, -u, -v]
        a = [bm.verts.new(P + d) for d in ring]
        b = [bm.verts.new(Q + d) for d in ring]
        for i in range(4):
            j = (i + 1) % 4
            bm.faces.new((a[i], a[j], b[j], b[i]))
    bm.normal_update()
    o = core.mesh_object(name, bm)
    core.orient_normals(o)
    o.data.shade_smooth()
    return o


def coil(name, a, b, coil_r, wire_r, turns, per_turn=8):
    """A coil spring from a to b: a light helix (per_turn points a turn, a four-sided wire)."""
    A, B = Vector(a), Vector(b)
    ax = (B - A).normalized()
    u = ax.cross(Vector((0, 0, 1)) if abs(ax.z) < 0.95 else Vector((1, 0, 0))).normalized()
    v = ax.cross(u)
    n = turns * per_turn
    pts = [tuple(A.lerp(B, i / n) + (u * math.cos(2 * math.pi * i / per_turn) + v * math.sin(2 * math.pi * i / per_turn)) * coil_r)
           for i in range(n + 1)]
    return tube(name, pts, wire_r, 4)


def bend(points, radius, steps=6):
    """A 3D polyline with its corners rounded (bent tube, wire)."""
    P = [Vector(p) for p in points]
    out = [P[0]]
    for i in range(1, len(P) - 1):
        a, c, b = P[i - 1], P[i], P[i + 1]
        d0, d1 = a - c, b - c
        l0, l1 = d0.length, d1.length
        d0.normalize()
        d1.normalize()
        ang = math.acos(max(-1.0, min(1.0, d0.dot(d1))))
        if ang > math.pi - 1e-3 or ang < 1e-3:
            out.append(c)
            continue
        t = min(radius / math.tan(ang / 2), l0 * 0.49, l1 * 0.49)
        p0, p1 = c + d0 * t, c + d1 * t
        for k in range(steps + 1):
            s = k / steps
            out.append((1 - s) ** 2 * p0 + 2 * (1 - s) * s * c + s ** 2 * p1)
    out.append(P[-1])
    return [tuple(p) for p in out]


def quad(name, corners):
    bm = bmesh.new()
    vs = [bm.verts.new(c) for c in corners]
    bm.faces.new(vs)
    return core.mesh_object(name, bm)


def frame_matrix(u, v, origin):
    """Local X -> u, local Y -> v, local Z -> u x v, at origin."""
    U, V = Vector(u).normalized(), Vector(v).normalized()
    N = U.cross(V).normalized()
    m = Matrix((U, V, N)).transposed().to_4x4()
    m.translation = Vector(origin)
    return m


# Frames for flat things laid out in their own XY (x across, y up, z out of the face), by the way the face looks.
def face_m(origin, facing):
    """facing: '-x' (toward the street), '+x' (toward the back wall), '-y' (south), '+y' (north), '+z' (up)."""
    u, v = {'-x': ((0, -1, 0), (0, 0, 1)), '+x': ((0, 1, 0), (0, 0, 1)), '-y': ((1, 0, 0), (0, 0, 1)), '+y': ((-1, 0, 0), (0, 0, 1)),
            '+z': ((1, 0, 0), (0, 1, 0))}[facing]
    return frame_matrix(u, v, origin)


def prism(name, outline, depth, matrix, r=0.0, seg=2):
    """A flat shape (counter-clockwise (x, y) outline) extruded from z 0 to depth, its rim rounded by r, placed by matrix."""
    return parts.panel(name, outline, 0.0, depth, r, seg, matrix)


def blob(name, center, size, seed=1, detail=0.18, subdiv=3, flatten=0.0, smooth=0, pinch=0.0):
    """A soft lump (a bag of goods, a cloth): an icosphere scaled to size and pushed about by noise."""
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=subdiv, radius=1.0)
    P = np.array([v.co[:] for v in bm.verts])
    n = texmaps.value_noise(P * 0.5 + seed * 3.1, 0.6, seed=seed, octaves=3) - 0.5
    n2 = texmaps.value_noise(P * 1.3 + seed * 1.7, 0.35, seed=seed + 7, octaves=2) - 0.5
    for v, a, b in zip(bm.verts, n, n2):
        d = v.co.normalized()
        k = 1.0 + detail * a * 2.0 + detail * 0.6 * b
        co = d * k
        if flatten > 0.0 and co.z < 0.0:
            co.z *= (1.0 - flatten)
        if pinch > 0.0 and d.z > 0.0:
            # gathered at the top (a tied bag): the sides drawn in toward the neck, the neck pulled up
            t = d.z ** 2.2
            co.x *= 1.0 - pinch * t
            co.y *= 1.0 - pinch * t
            co.z *= 1.0 + 0.3 * pinch * t
        v.co = Vector((center[0] + co.x * size[0] / 2, center[1] + co.y * size[1] / 2, center[2] + co.z * size[2] / 2))
    for _ in range(smooth):
        bmesh.ops.smooth_vert(bm, verts=bm.verts[:], factor=0.5, use_axis_x=True, use_axis_y=True, use_axis_z=True)
    obj = core.mesh_object(name, bm)
    obj.data.shade_smooth()
    return obj


def xform(obj, m):
    obj.data.transform(m)
    return obj


def T(x, y, z):
    return Matrix.Translation((x, y, z))


def RZ(deg):
    return Matrix.Rotation(math.radians(deg), 4, 'Z')


def RX(deg):
    return Matrix.Rotation(math.radians(deg), 4, 'X')


def RY(deg):
    return Matrix.Rotation(math.radians(deg), 4, 'Y')


def tri_count(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


# ================================================================== the kit: parts, finishes, decals

FIN = {n: i + 1 for i, n in enumerate([
    'paint', 'enamel', 'steel', 'chrome', 'alu', 'bronze', 'plastic', 'gloss', 'rubber', 'powder', 'laminate', 'paper', 'cardboard',
    'cloth', 'felt', 'gold', 'gem', 'velvet', 'leather', 'wood', 'lacquer', 'tolex', 'grill', 'screen', 'brass', 'wire', 'tile',
    'terrazzo', 'vct', 'acoustic', 'wallpaint', 'melamine', 'rust', 'particle', 'kick', 'galv', 'nickel', 'black_metal', 'cast',
    'mat', 'foam', 'pearl'])}


class Kit:
    """One mesh's parts. Body parts carry a finish (pw_fin), a tint (pw_col) and a per-finish parameter vector (pw_aux), and
    go to one of the mesh's body slots; decals carry atlas UVs (the print slot); unbaked parts carry one of the shared
    unbaked materials (glass, screens)."""

    def __init__(self, prefix, name, slots=('body',), dust=0.25, low_grime=0.12, cavity=0.35, site=True):
        self.prefix, self.name = prefix, name
        self.site = site            # built in the shop's frame: finish() moves it into the SITE frame (SITE_M)
        self.slots = list(slots)
        self.default = self.slots[0]  # the slot a part goes to when it names none
        self.objs = []
        self.kinds = {s: set() for s in self.slots}
        self.atlas = None
        self.dust = dust if isinstance(dust, dict) else {s: dust for s in self.slots}
        self.low = low_grime if isinstance(low_grime, dict) else {s: low_grime for s in self.slots}
        self.cavity = cavity if isinstance(cavity, dict) else {s: cavity for s in self.slots}
        self.grime = []             # (center, radius, amount): hand grime
        self.polish = []            # (center, radius, amount): rubbed glossy where hands go
        self.laminate_chips = []    # (x, y, r)
        self.rings = []             # (x, y, r, hex): cup rings on a laminate top
        self.images = {}            # name -> (path, (x0, y0, x1, y1) planar extent in object XY): control maps for finishes
        self.tri_by = []            # (part name, triangles) for PAWN_TRIS=1

    def add(self, obj, fin, col=0xffffff, slot=None, aux=(0.0, 0.0, 0.0), aux_point=None, aux2=(0.0, 0.0, 0.0)):
        """aux_point: a point in the part's own frame that moves with it (a guitar's sunburst center): finish() writes
        where it ends up into pw_aux, so a finish can measure from it whatever way the part was turned."""
        slot = slot or self.default
        set_attr(obj, 'pw_fin', float(FIN[fin]))
        set_attr(obj, 'pw_col', lin3(col))
        set_attr(obj, 'pw_aux', tuple(float(a) for a in aux))
        set_attr(obj, 'pw_aux2', tuple(float(a) for a in aux2))
        set_attr(obj, 'pw_lock', 0.0)
        if aux_point is not None:
            obj['pw_auxpt'] = tuple(float(a) for a in aux_point)
        obj['pw_slot'] = slot
        self.tri_by.append((obj.name.split(".")[0], tri_count(obj)))
        self.kinds[slot].add(fin)
        self.objs.append(obj)
        return obj

    def mark(self):
        """Where the parts (and the grime spots) made from here on start: place() moves them together."""
        return (len(self.objs), len(self.grime), len(self.polish))

    def place(self, mk, matrix):
        """Moves everything made since mark() by matrix (an item built in its own frame, then set in the room)."""
        i0, g0, p0 = mk
        for o in self.objs[i0:]:
            o.data.transform(matrix)
            if 'pw_auxpt' in o:
                o['pw_auxpt'] = tuple(matrix @ Vector(o['pw_auxpt']))
        self.grime[g0:] = [(tuple(matrix @ Vector(cc)), r, a) for cc, r, a in self.grime[g0:]]
        self.polish[p0:] = [(tuple(matrix @ Vector(cc)), r, a) for cc, r, a in self.polish[p0:]]

    def special(self, obj, mat_name, locked=False):
        set_attr(obj, 'pw_fin', 0.0)
        set_attr(obj, 'pw_col', (1.0, 1.0, 1.0))
        set_attr(obj, 'pw_aux', (0.0, 0.0, 0.0))
        set_attr(obj, 'pw_aux2', (0.0, 0.0, 0.0))
        set_attr(obj, 'pw_lock', 1.0 if locked else 0.0)
        obj['pw_slot'] = mat_name
        self.objs.append(obj)
        return obj

    def screen(self, name, mat_name, origin, facing, w, h):
        """A [screen] quad (UV 0..1 across it as seen, u right, v up), w x h meters centered at origin."""
        M = face_m(origin, facing)
        bm = bmesh.new()
        uvl = bm.loops.layers.uv.new('UVMap')
        vs = [bm.verts.new(M @ Vector((sx * w / 2, sy * h / 2, 0.0))) for sx, sy in ((-1, -1), (1, -1), (1, 1), (-1, 1))]
        f = bm.faces.new(vs)
        for loop, uv in zip(f.loops, ((0, 0), (1, 0), (1, 1), (0, 1))):
            loop[uvl].uv = uv
        obj = core.mesh_object(name, bm)
        return self.special(obj, mat_name, locked=True)

    def panel(self, key, w, h, draw, glow=0.0, density=1.0):
        """A print panel w x h meters (drawn in millimeters)."""
        if self.atlas is None:
            self.atlas = Atlas(f'{self.prefix}_print', TEXTURE_SIZES.get(f'{self.prefix}_print', 1024))
        self.atlas.add(key, w * 1000.0, h * 1000.0, draw, glow, density)

    def decal(self, key, origin, u=(1, 0, 0), v=(0, 0, 1), w=None, h=None, lift=0.0004, outline=None, nu=1, nv=1, disp=None, matrix=None,
              crop=None):
        """Panel `key` laid on the plane through origin spanned by u (the print's right) and v (its up), facing u x v.
        crop: (s0, t0, s1, t1) of the panel to show (default all of it)."""
        p = self.atlas.panels[key]
        w = p['w'] / 1000.0 if w is None else w
        h = p['h'] / 1000.0 if h is None else h
        u0, v0, u1, v1 = self.atlas.rect(key)
        if crop is not None:
            cs0, ct0, cs1, ct1 = crop
            u0, u1 = u0 + (u1 - u0) * cs0, u0 + (u1 - u0) * cs1
            v0, v1 = v0 + (v1 - v0) * ct0, v0 + (v1 - v0) * ct1
        U, V = Vector(u).normalized(), Vector(v).normalized()
        N = U.cross(V).normalized()
        O = Vector(origin) + N * lift
        bm = bmesh.new()
        uvl = bm.loops.layers.uv.new('UVMap')

        def pos(s, t):
            d = disp(s, t) if disp else 0.0
            return O + U * ((s - 0.5) * w) + V * ((t - 0.5) * h) + N * d

        if outline:
            vs = [bm.verts.new(pos(s, t)) for s, t in outline]
            f = bm.faces.new(vs)
            for loop, (s, t) in zip(f.loops, outline):
                loop[uvl].uv = (u0 + (u1 - u0) * s, v0 + (v1 - v0) * t)
        else:
            grid = [[bm.verts.new(pos(i / nu, j / nv)) for i in range(nu + 1)] for j in range(nv + 1)]
            for j in range(nv):
                for i in range(nu):
                    f = bm.faces.new((grid[j][i], grid[j][i + 1], grid[j + 1][i + 1], grid[j + 1][i]))
                    for loop, (a, b) in zip(f.loops, ((i, j), (i + 1, j), (i + 1, j + 1), (i, j + 1))):
                        loop[uvl].uv = (u0 + (u1 - u0) * a / nu, v0 + (v1 - v0) * b / nv)
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
        for f in bm.faces:
            if f.normal.dot(N) < 0:
                f.normal_flip()
        obj = core.mesh_object(f'decal_{key}', bm)
        if matrix is not None:
            obj.data.transform(matrix)
        set_attr(obj, 'pw_fin', 0.0)
        set_attr(obj, 'pw_col', (1.0, 1.0, 1.0))
        set_attr(obj, 'pw_aux', (0.0, 0.0, 0.0))
        set_attr(obj, 'pw_aux2', (0.0, 0.0, 0.0))
        set_attr(obj, 'pw_lock', 1.0)
        obj['pw_slot'] = 'print'
        self.objs.append(obj)
        return obj

    def fdecal(self, key, origin, facing, w=None, h=None, lift=0.0004, **kw):
        """A decal laid on a face by the way it looks ('-x', '+x', '-y', '+y', '+z'): reads correctly from outside it."""
        u, v = {'-x': ((0, -1, 0), (0, 0, 1)), '+x': ((0, 1, 0), (0, 0, 1)), '-y': ((1, 0, 0), (0, 0, 1)), '+y': ((-1, 0, 0), (0, 0, 1)),
                '+z': ((1, 0, 0), (0, 1, 0))}[facing]
        return self.decal(key, origin, u=u, v=v, w=w, h=h, lift=lift, **kw)

    def printed(self, obj, key, uvfun):
        """Any mesh printed with panel `key`: uvfun(co) -> (s, t) in [0, 1]^2 of the panel."""
        u0, v0, u1, v1 = self.atlas.rect(key)
        me = obj.data
        uv = me.uv_layers.get('UVMap') or me.uv_layers.new(name='UVMap')
        for poly in me.polygons:
            for li in poly.loop_indices:
                s, t = uvfun(me.vertices[me.loops[li].vertex_index].co)
                uv.data[li].uv = (u0 + (u1 - u0) * s, v0 + (v1 - v0) * t)
        set_attr(obj, 'pw_fin', 0.0)
        set_attr(obj, 'pw_col', (1.0, 1.0, 1.0))
        set_attr(obj, 'pw_aux', (0.0, 0.0, 0.0))
        set_attr(obj, 'pw_aux2', (0.0, 0.0, 0.0))
        set_attr(obj, 'pw_lock', 1.0)
        obj['pw_slot'] = 'print'
        self.objs.append(obj)
        return obj

    def finish(self, angle=35.0, name=None):
        name = name or self.name
        if self.site:
            # into the SITE frame: the parts, the grime spots and the moving aux points
            for o in self.objs:
                o.data.transform(SITE_M)
                if 'pw_auxpt' in o:
                    o['pw_auxpt'] = tuple(SITE_M @ Vector(o['pw_auxpt']))
            self.grime = [(tuple(SITE_M @ Vector(cc)), r, a) for cc, r, a in self.grime]
            self.polish = [(tuple(SITE_M @ Vector(cc)), r, a) for cc, r, a in self.polish]
        for o in self.objs:
            if 'pw_auxpt' in o:
                set_attr(o, 'pw_aux', tuple(o['pw_auxpt']))
        mats = [body_material(self, s) for s in self.slots if self.kinds[s]]
        by = {s: m for s, m in zip([s for s in self.slots if self.kinds[s]], mats)}
        if any(o['pw_slot'] == 'print' for o in self.objs):
            pm = print_material(self)
            mats.append(pm)
            by['print'] = pm
        for n in UNBAKED_MATS:
            if any(o['pw_slot'] == n for o in self.objs):
                mats.append(UNB[n])
        for o in self.objs:
            slot = o['pw_slot']
            core.assign(o, by[slot] if slot in by else UNB[slot])
            if not o.data.uv_layers:
                o.data.uv_layers.new(name='UVMap')
            elif o.data.uv_layers[0].name != 'UVMap':
                o.data.uv_layers[0].name = 'UVMap'
        obj = core.join(self.objs, name) if len(self.objs) > 1 else self.objs[0]
        obj.name = name
        obj.data.name = name
        order_slots(obj, [m.m if isinstance(m, core.Mat) else m for m in mats])
        core.finish_hard_surface(obj, angle)
        lock = obj.data.attributes.get('pw_lock')
        vals = np.zeros(len(obj.data.polygons), dtype=np.float32)
        lock.data.foreach_get('value', vals)
        locked = set(np.nonzero(vals > 0.5)[0].tolist())
        groups = [(lambda f: f.index in locked, 'keep', None, (0.0, 0.0, 1.0, 1.0))]
        for s in obj.material_slots:
            groups.append((core.material_is(obj, s.material.name), 'box', None, (0.0, 0.0, 1.0, 1.0)))
        groups.append((lambda f: True, 'box', None, (0.0, 0.0, 1.0, 1.0)))
        core.uv_layout(obj, groups, margin=0.004)
        print(f'[ns_pawn] {name}: {tri_count(obj)} triangles, slots {[s.material.name for s in obj.material_slots]}')
        if os.environ.get("PAWN_TRIS"):
            agg = {}
            for nm, t in self.tri_by:
                agg[nm] = agg.get(nm, 0) + t
            for nm, t in sorted(agg.items(), key=lambda kv: -kv[1])[:25]:
                print(f"[ns_pawn]   {nm}: {t}")
        return obj


def order_slots(obj, mats):
    """Puts obj's material slots in the order of mats and remaps the faces."""
    me = obj.data
    old = [s.material for s in obj.material_slots]
    remap = [mats.index(mt) for mt in old]
    fi = np.zeros(len(me.polygons), dtype=np.int32)
    me.polygons.foreach_get('material_index', fi)
    new = np.array(remap, dtype=np.int32)[fi] if remap else fi
    me.materials.clear()
    for mt in mats:
        me.materials.append(mt)
    me.polygons.foreach_set('material_index', new)


# ------------------------------------------------------------------ print atlas

class Sub:
    """A panel's drawing space inside an atlas Sheet: millimeters from the panel's corner."""

    def __init__(self, s, ox, oy, k):
        self.s, self.ox, self.oy, self.k = s, ox, oy, k

    def P(self, x, y):
        return (self.ox + x * self.k, self.oy + y * self.k)

    def rect(self, x, y, w, h, color, metal=0.0, rough=0.4, rot=0.0, alpha=1.0):
        X, Y = self.P(x, y)
        return self.s.rect(X, Y, w * self.k, h * self.k, color, metal, rough, rot, alpha)

    def poly(self, pts, color, metal=0.0, rough=0.4, alpha=1.0):
        return self.s.poly([self.P(x, y) for x, y in pts], color, metal, rough, alpha)

    def circle(self, cx, cy, r, color, metal=0.0, rough=0.4, n=48, alpha=1.0):
        X, Y = self.P(cx, cy)
        return self.s.circle(X, Y, r * self.k, color, metal, rough, n, alpha)

    def ring(self, cx, cy, r0, r1, color, metal=0.0, rough=0.4, n=64, a0=0.0, a1=2 * math.pi):
        X, Y = self.P(cx, cy)
        return self.s.ring(X, Y, r0 * self.k, r1 * self.k, color, metal, rough, n, a0, a1)

    def text(self, txt, x, y, size, color, face='Black', align='LEFT', rot=0.0, tracking=1.0, metal=0.0, rough=0.4, alpha=1.0):
        X, Y = self.P(x, y)
        return self.s.text(txt, X, Y, size * self.k, color, face=face, align=align, rot=rot, tracking=tracking, metal=metal, rough=rough,
                           alpha=alpha)

    def fit(self, txt, x, y, size, max_w, color, face='Black', align='CENTER', tracking=1.0, metal=0.0, rough=0.4, rot=0.0):
        size = min(size, max_w / text_w(self.s, txt, face, tracking))
        self.text(txt, x, y, size, color, face, align, rot, tracking, metal, rough)
        return size

    def mid(self, txt, x, cy, size, max_w, color, face='Black', align='CENTER', tracking=1.0, metal=0.0, rough=0.4, alpha=1.0):
        """Text whose capitals are centered on cy, shrunk to fit max_w."""
        size = min(size, max_w / text_w(self.s, txt, face, tracking))
        self.text(txt, x, cy - 0.355 * size, size, color, face, align, 0.0, tracking, metal, rough, alpha)
        return size

    def mesh(self, verts, faces, color, metal=0.0, rough=0.5, alpha=1.0):
        me = bpy.data.meshes.new('poly')
        me.from_pydata([(self.P(x, y)[0], self.P(x, y)[1], 0.0) for x, y in verts], [], faces)
        return self.s._add(bpy.data.objects.new('poly', me), color, metal, rough, alpha)

    def stroke(self, pts, width, color, rough=0.6, alpha=1.0):
        """A pen line through pts (rounded joints)."""
        verts, faces = [], []
        hw = width / 2
        for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
            dx, dy = x1 - x0, y1 - y0
            ln = math.hypot(dx, dy)
            if ln < 1e-9:
                continue
            nx, ny = -dy / ln * hw, dx / ln * hw
            b = len(verts)
            verts += [(x0 + nx, y0 + ny), (x0 - nx, y0 - ny), (x1 - nx, y1 - ny), (x1 + nx, y1 + ny)]
            faces.append((b, b + 1, b + 2, b + 3))
        for x, y in pts:
            b = len(verts)
            verts += [(x + hw * math.cos(2 * math.pi * k / 10), y + hw * math.sin(2 * math.pi * k / 10)) for k in range(10)]
            faces.append(tuple(range(b, b + 10)))
        if faces:
            self.mesh(verts, faces, color, 0.0, rough, alpha)


_WIDTHS = {}


def text_w(s, text, face='Black', tracking=1.0):
    """Width of text at size 1 (cached)."""
    key = (text, face, tracking)
    if key not in _WIDTHS:
        cu = bpy.data.curves.new('m', 'FONT')
        cu.body = text
        cu.font = s._font(face)
        cu.size = 1.0
        cu.space_character = tracking
        ob = bpy.data.objects.new('m', cu)
        core.link(ob)
        bpy.context.view_layer.update()
        _WIDTHS[key] = max(ob.dimensions.x, 1e-3)
        bpy.data.objects.remove(ob)
        bpy.data.curves.remove(cu)
    return _WIDTHS[key]


class Atlas:
    """Print panels (millimeters) packed into one square image, rendered in one pass."""

    def __init__(self, name, px, gap=0.006):
        self.name, self.px, self.gap = name, px, gap
        self.panels = {}
        self.rects = {}
        self.color = self.surface = None

    def add(self, key, w, h, draw, glow=0.0, density=1.0):
        self.panels[key] = {'w': w, 'h': h, 'draw': draw, 'glow': glow, 'd': density}

    def _pack(self, s):
        order = sorted(self.panels.items(), key=lambda kv: -kv[1]['h'] * kv[1]['d'])
        x = y = shelf = 0.0
        pos = {}
        for k, p in order:
            w, h = p['w'] * p['d'] * s, p['h'] * p['d'] * s
            if w > 1.0:
                return None
            if x + w > 1.0:
                y += shelf + self.gap
                x = shelf = 0.0
            if y + h > 1.0:
                return None
            pos[k] = (x, y, w, h)
            x += w + self.gap
            shelf = max(shelf, h)
        return pos

    def render(self):
        lo, hi = 0.0, 1.0
        while self._pack(hi) is not None:
            hi *= 2.0
        for _ in range(40):
            mid = (lo + hi) / 2
            if self._pack(mid) is not None:
                lo = mid
            else:
                hi = mid
        pos = self._pack(lo)
        s = Sheet(1000.0, 1000.0)
        for k, (x, y, w, h) in pos.items():
            p = self.panels[k]
            p['draw'](Sub(s, x * 1000.0, y * 1000.0, w * 1000.0 / p['w']), p['w'], p['h'])
            self.rects[k] = (x, y, x + w, y + h)
        self.color, self.surface = s.render(self.name, self.px, samples=24)
        for obj, *_ in s.items:
            data = obj.data
            bpy.data.objects.remove(obj)
            if isinstance(data, bpy.types.Mesh):
                bpy.data.meshes.remove(data)
            elif data is not None:
                bpy.data.curves.remove(data)
        for o in [o for o in bpy.data.objects if o.name.startswith('sheet_cam')]:
            bpy.data.objects.remove(o)
        for mt in [mt for mt in bpy.data.materials if mt.name.startswith('sheet') and mt.users == 0]:
            bpy.data.materials.remove(mt)
        dens = 1000.0 * self.px * lo
        print(f'[ns_pawn] atlas {self.name}: {len(self.panels)} panels, {dens / 1000.0:.1f} px/mm at density 1')
        return self

    def rect(self, key):
        """The panel's UV rect, inset half a texel."""
        x0, y0, x1, y1 = self.rects[key]
        e = 0.6 / self.px
        return (x0 + e, y0 + e, x1 - e, y1 - e)


# ------------------------------------------------------------------ handwriting (a pen font of strokes)

_HAND = {
    'T': ([[(0.0, 0.70), (0.26, 0.72), (0.54, 0.73)], [(0.27, 0.72), (0.26, 0.36), (0.25, 0.0)]], 0.52),
    'u': ([[(0.04, 0.50), (0.04, 0.16), (0.10, 0.03), (0.22, 0.0), (0.34, 0.06), (0.41, 0.22), (0.42, 0.5), (0.42, 0.22), (0.43, 0.04), (0.48, 0.0)]], 0.52),
    'e': ([[(0.06, 0.25), (0.36, 0.28), (0.40, 0.38), (0.32, 0.48), (0.18, 0.50), (0.07, 0.40), (0.03, 0.22), (0.10, 0.06), (0.24, 0.0), (0.40, 0.06)]], 0.46),
    'h': ([[(0.05, 0.78), (0.05, 0.36), (0.05, 0.0)], [(0.05, 0.28), (0.14, 0.44), (0.28, 0.50), (0.38, 0.42), (0.40, 0.26), (0.40, 0.0)]], 0.50),
    'a': ([[(0.38, 0.40), (0.28, 0.49), (0.14, 0.48), (0.04, 0.34), (0.04, 0.14), (0.12, 0.02), (0.26, 0.02), (0.38, 0.16)],
           [(0.39, 0.50), (0.39, 0.10), (0.46, 0.0)]], 0.52),
    't': ([[(0.16, 0.70), (0.16, 0.10), (0.22, 0.0), (0.32, 0.03)], [(0.02, 0.47), (0.18, 0.48), (0.34, 0.50)]], 0.38),
    's': ([[(0.36, 0.44), (0.24, 0.50), (0.10, 0.47), (0.06, 0.37), (0.15, 0.28), (0.30, 0.22), (0.38, 0.11), (0.29, 0.01), (0.13, 0.0), (0.03, 0.07)]], 0.44),
    'S': ([[(0.46, 0.64), (0.36, 0.72), (0.18, 0.72), (0.06, 0.62), (0.08, 0.47), (0.26, 0.38), (0.42, 0.28), (0.47, 0.14), (0.37, 0.02), (0.18, 0.0), (0.04, 0.08)]], 0.54),
    'P': ([[(0.06, 0.0), (0.06, 0.36), (0.06, 0.72), (0.30, 0.72), (0.44, 0.64), (0.46, 0.50), (0.38, 0.40), (0.24, 0.37), (0.06, 0.37)]], 0.50),
    'M': ([[(0.04, 0.0), (0.05, 0.36), (0.06, 0.72), (0.30, 0.24), (0.54, 0.72), (0.55, 0.36), (0.56, 0.0)]], 0.64),
    'D': ([[(0.06, 0.0), (0.06, 0.36), (0.06, 0.72)], [(0.06, 0.72), (0.26, 0.71), (0.42, 0.60), (0.48, 0.38), (0.42, 0.14), (0.26, 0.02), (0.06, 0.0)]], 0.54),
    'N': ([[(0.05, 0.0), (0.05, 0.36), (0.05, 0.72), (0.25, 0.36), (0.45, 0.0), (0.45, 0.36), (0.45, 0.72)]], 0.52),
    'O': ([[(0.28, 0.72), (0.12, 0.66), (0.04, 0.48), (0.04, 0.24), (0.12, 0.06), (0.28, 0.0), (0.44, 0.06), (0.52, 0.24), (0.52, 0.48), (0.44, 0.66),
            (0.28, 0.72), (0.22, 0.70)]], 0.58),
    'L': ([[(0.06, 0.72), (0.06, 0.36), (0.06, 0.0), (0.26, 0.0), (0.44, 0.01)]], 0.46),
    'F': ([[(0.44, 0.73), (0.26, 0.72), (0.06, 0.72), (0.06, 0.36), (0.06, 0.0)], [(0.06, 0.38), (0.34, 0.39)]], 0.46),
    'U': ([[(0.06, 0.72), (0.06, 0.24), (0.14, 0.04), (0.26, 0.0), (0.38, 0.04), (0.46, 0.24), (0.46, 0.72)]], 0.52),
    'A': ([[(0.02, 0.0), (0.14, 0.36), (0.26, 0.72), (0.38, 0.36), (0.50, 0.0)], [(0.10, 0.25), (0.42, 0.26)]], 0.54),
    'B': ([[(0.06, 0.0), (0.06, 0.72), (0.30, 0.72), (0.42, 0.64), (0.42, 0.48), (0.30, 0.39), (0.06, 0.38)],
           [(0.30, 0.39), (0.44, 0.31), (0.46, 0.14), (0.34, 0.02), (0.06, 0.0)]], 0.52),
    'C': ([[(0.46, 0.62), (0.34, 0.72), (0.18, 0.70), (0.06, 0.56), (0.04, 0.32), (0.10, 0.10), (0.24, 0.0), (0.38, 0.02), (0.48, 0.10)]], 0.52),
    'E': ([[(0.44, 0.72), (0.06, 0.72), (0.06, 0.36), (0.06, 0.0), (0.46, 0.0)], [(0.06, 0.38), (0.36, 0.38)]], 0.50),
    'G': ([[(0.46, 0.62), (0.34, 0.72), (0.18, 0.70), (0.06, 0.56), (0.04, 0.32), (0.10, 0.10), (0.24, 0.0), (0.40, 0.03), (0.48, 0.14), (0.48, 0.32),
            (0.30, 0.32)]], 0.54),
    'H': ([[(0.06, 0.72), (0.06, 0.0)], [(0.46, 0.72), (0.46, 0.0)], [(0.06, 0.37), (0.46, 0.37)]], 0.54),
    'I': ([[(0.10, 0.72), (0.10, 0.0)]], 0.22),
    'K': ([[(0.06, 0.72), (0.06, 0.0)], [(0.44, 0.72), (0.08, 0.32)], [(0.20, 0.42), (0.46, 0.0)]], 0.50),
    'R': ([[(0.06, 0.0), (0.06, 0.72), (0.30, 0.72), (0.44, 0.64), (0.46, 0.50), (0.38, 0.40), (0.24, 0.37), (0.06, 0.37)], [(0.24, 0.37), (0.46, 0.0)]], 0.52),
    'V': ([[(0.02, 0.72), (0.14, 0.36), (0.26, 0.0), (0.38, 0.36), (0.50, 0.72)]], 0.52),
    'W': ([[(0.02, 0.72), (0.12, 0.30), (0.20, 0.0), (0.30, 0.46), (0.40, 0.0), (0.48, 0.30), (0.58, 0.72)]], 0.62),
    'Y': ([[(0.02, 0.72), (0.24, 0.38), (0.46, 0.72)], [(0.24, 0.38), (0.24, 0.0)]], 0.48),
    '9': ([[(0.40, 0.50), (0.31, 0.38), (0.18, 0.36), (0.06, 0.46), (0.06, 0.62), (0.18, 0.72), (0.34, 0.72), (0.42, 0.62), (0.42, 0.40), (0.36, 0.12),
            (0.24, 0.0), (0.10, 0.04)]], 0.50),
    '0': ([[(0.24, 0.72), (0.10, 0.64), (0.04, 0.40), (0.06, 0.14), (0.16, 0.0), (0.32, 0.0), (0.42, 0.14), (0.44, 0.40), (0.38, 0.64), (0.24, 0.72),
            (0.18, 0.69)]], 0.50),
    '1': ([[(0.08, 0.58), (0.22, 0.72), (0.22, 0.36), (0.22, 0.0)]], 0.36),
    '2': ([[(0.06, 0.58), (0.16, 0.70), (0.30, 0.72), (0.42, 0.62), (0.40, 0.46), (0.24, 0.26), (0.04, 0.0), (0.24, 0.01), (0.44, 0.02)]], 0.50),
    '3': ([[(0.06, 0.64), (0.20, 0.72), (0.36, 0.70), (0.42, 0.58), (0.36, 0.44), (0.20, 0.40), (0.38, 0.34), (0.44, 0.18), (0.36, 0.04), (0.20, 0.0),
            (0.05, 0.06)]], 0.50),
    '4': ([[(0.34, 0.0), (0.34, 0.36), (0.34, 0.72), (0.04, 0.22), (0.46, 0.22)]], 0.50),
    '5': ([[(0.42, 0.72), (0.12, 0.72), (0.08, 0.42), (0.22, 0.46), (0.36, 0.42), (0.44, 0.27), (0.40, 0.09), (0.26, 0.0), (0.09, 0.05)]], 0.50),
    '6': ([[(0.40, 0.68), (0.28, 0.72), (0.14, 0.66), (0.06, 0.44), (0.06, 0.18), (0.14, 0.03), (0.28, 0.0), (0.40, 0.08), (0.42, 0.24), (0.32, 0.38),
            (0.18, 0.38), (0.07, 0.28)]], 0.50),
    '7': ([[(0.04, 0.72), (0.24, 0.72), (0.44, 0.72), (0.30, 0.40), (0.18, 0.0)]], 0.48),
    '8': ([[(0.24, 0.40), (0.10, 0.48), (0.08, 0.62), (0.18, 0.72), (0.32, 0.72), (0.40, 0.62), (0.38, 0.48), (0.24, 0.40), (0.08, 0.30), (0.05, 0.14),
            (0.14, 0.02), (0.30, 0.0), (0.42, 0.08), (0.44, 0.24), (0.34, 0.34), (0.24, 0.40)]], 0.50),
    '$': ([[(0.40, 0.58), (0.30, 0.64), (0.16, 0.64), (0.06, 0.55), (0.10, 0.43), (0.26, 0.36), (0.38, 0.27), (0.40, 0.14), (0.30, 0.06), (0.15, 0.06),
            (0.04, 0.13)], [(0.23, 0.76), (0.23, 0.36), (0.22, -0.06)]], 0.48),
    'r': ([[(0.05, 0.50), (0.05, 0.25), (0.05, 0.0)], [(0.05, 0.30), (0.12, 0.44), (0.24, 0.50), (0.34, 0.47)]], 0.36),
    'z': ([[(0.04, 0.49), (0.20, 0.50), (0.36, 0.50), (0.20, 0.25), (0.04, 0.01), (0.22, 0.0), (0.38, 0.0)]], 0.42),
    'k': ([[(0.05, 0.76), (0.05, 0.36), (0.05, 0.0)], [(0.36, 0.50), (0.06, 0.22), (0.38, 0.0)]], 0.42),
    'f': ([[(0.36, 0.72), (0.26, 0.76), (0.16, 0.70), (0.13, 0.55), (0.13, 0.25), (0.12, 0.0)], [(0.0, 0.46), (0.30, 0.47)]], 0.34),
    'o': ([[(0.22, 0.50), (0.08, 0.43), (0.03, 0.25), (0.08, 0.07), (0.22, 0.0), (0.36, 0.07), (0.41, 0.25), (0.36, 0.43), (0.22, 0.50), (0.17, 0.48)]], 0.46),
    'n': ([[(0.05, 0.50), (0.05, 0.25), (0.05, 0.0)], [(0.05, 0.30), (0.14, 0.45), (0.28, 0.50), (0.38, 0.42), (0.40, 0.26), (0.40, 0.0)]], 0.48),
    'i': ([[(0.08, 0.48), (0.08, 0.25), (0.08, 0.02), (0.12, 0.0)], [(0.08, 0.66), (0.085, 0.67)]], 0.20),
    'l': ([[(0.08, 0.76), (0.08, 0.40), (0.08, 0.05), (0.13, 0.0)]], 0.20),
    'c': ([[(0.38, 0.42), (0.26, 0.50), (0.12, 0.46), (0.04, 0.30), (0.06, 0.12), (0.18, 0.0), (0.32, 0.02), (0.40, 0.10)]], 0.44),
    'd': ([[(0.38, 0.40), (0.28, 0.49), (0.14, 0.48), (0.04, 0.34), (0.04, 0.14), (0.12, 0.02), (0.26, 0.02), (0.38, 0.16)],
           [(0.40, 0.78), (0.40, 0.36), (0.40, 0.08), (0.46, 0.0)]], 0.50),
    'm': ([[(0.04, 0.50), (0.04, 0.25), (0.04, 0.0)], [(0.04, 0.30), (0.12, 0.46), (0.22, 0.50), (0.30, 0.42), (0.31, 0.0)],
           [(0.31, 0.32), (0.40, 0.47), (0.50, 0.50), (0.58, 0.42), (0.59, 0.0)]], 0.66),
    'p': ([[(0.05, 0.50), (0.05, 0.10), (0.05, -0.26)], [(0.05, 0.36), (0.16, 0.48), (0.30, 0.50), (0.40, 0.38), (0.42, 0.20), (0.34, 0.04),
                                                      (0.20, 0.0), (0.06, 0.08)]], 0.48),
    'v': ([[(0.02, 0.50), (0.12, 0.25), (0.21, 0.0), (0.30, 0.25), (0.40, 0.50)]], 0.44),
    'w': ([[(0.02, 0.50), (0.09, 0.22), (0.16, 0.0), (0.25, 0.34), (0.34, 0.0), (0.41, 0.22), (0.48, 0.50)]], 0.52),
    'x': ([[(0.04, 0.50), (0.36, 0.0)], [(0.36, 0.50), (0.04, 0.0)]], 0.42),
    'j': ([[(0.14, 0.48), (0.14, 0.10), (0.12, -0.14), (0.04, -0.24), (-0.04, -0.22)], [(0.14, 0.66), (0.145, 0.67)]], 0.24),
    'q': ([[(0.38, 0.40), (0.28, 0.49), (0.14, 0.48), (0.04, 0.34), (0.04, 0.14), (0.12, 0.02), (0.26, 0.02), (0.38, 0.16)],
           [(0.39, 0.50), (0.39, 0.10), (0.39, -0.26)]], 0.48),
    'L2': ([[(0.10, 0.70), (0.06, 0.40), (0.04, 0.08), (0.10, 0.0), (0.30, 0.02)]], 0.40),
    'U2': ([[(0.06, 0.72), (0.06, 0.24), (0.14, 0.04), (0.26, 0.0), (0.38, 0.04), (0.46, 0.24), (0.46, 0.72)]], 0.52),
    'y': ([[(0.04, 0.50), (0.10, 0.24), (0.21, 0.02)], [(0.40, 0.50), (0.28, 0.16), (0.16, -0.14), (0.08, -0.24), (0.0, -0.25)]], 0.44),
    'g': ([[(0.38, 0.40), (0.28, 0.49), (0.14, 0.48), (0.04, 0.34), (0.04, 0.16), (0.12, 0.04), (0.26, 0.04), (0.38, 0.18)],
           [(0.39, 0.50), (0.39, 0.10), (0.36, -0.14), (0.24, -0.24), (0.08, -0.20)]], 0.50),
    'b': ([[(0.05, 0.78), (0.05, 0.36), (0.05, 0.0)], [(0.05, 0.26), (0.14, 0.44), (0.28, 0.50), (0.39, 0.40), (0.42, 0.22), (0.35, 0.05),
                                                     (0.21, 0.0), (0.05, 0.05)]], 0.48),
    ':': ([[(0.08, 0.40), (0.09, 0.41)], [(0.07, 0.03), (0.08, 0.04)]], 0.20),
    '.': ([[(0.08, 0.02), (0.09, 0.03)]], 0.22),
    ',': ([[(0.10, 0.04), (0.06, -0.10)]], 0.22),
    '-': ([[(0.04, 0.30), (0.20, 0.31), (0.36, 0.30)]], 0.42),
    '/': ([[(0.02, -0.04), (0.20, 0.36), (0.38, 0.76)]], 0.40),
    '#': ([[(0.14, 0.0), (0.20, 0.72)], [(0.30, 0.0), (0.36, 0.72)], [(0.04, 0.24), (0.46, 0.25)], [(0.06, 0.48), (0.48, 0.49)]], 0.52),
    '"': ([[(0.08, 0.72), (0.08, 0.54)], [(0.18, 0.72), (0.18, 0.54)]], 0.26),
    "'": ([[(0.08, 0.72), (0.07, 0.54)]], 0.16),
    '+': ([[(0.06, 0.34), (0.40, 0.35)], [(0.23, 0.52), (0.23, 0.17)]], 0.46),
}


def hand_strokes(text, x, y, size, slant=0.14, seed=1, smooth=4, jitter=1.0):
    """The strokes of handwritten text as 2D polylines (baseline y, size = cap-to-baseline scale); returns (strokes, end x)."""
    rnd = random.Random(seed)
    cx = x
    out = []
    for ch in text:
        if ch == ' ':
            cx += 0.30 * size
            continue
        if ch not in _HAND:
            ch = ch.upper() if ch.upper() in _HAND else (ch.lower() if ch.lower() in _HAND else '-')
        strokes, adv = _HAND[ch]
        js = 1.0 + rnd.uniform(-0.06, 0.06) * jitter
        jx, jy = rnd.uniform(-0.03, 0.03) * jitter, rnd.uniform(-0.04, 0.03) * jitter
        for st in strokes:
            pts = [(cx + (px * js + py * js * slant + jx + rnd.uniform(-0.012, 0.012) * jitter) * size,
                    y + (py * js + jy + rnd.uniform(-0.012, 0.012) * jitter) * size) for px, py in st]
            if len(pts) > 2 and smooth:
                pts = [tuple(p) for p in core.catmull_rom(pts, smooth)]
            out.append(pts)
        cx += (adv * js + 0.05) * size
    return out, cx


def hand(s, text, x, y, size, color, width=0.075, slant=0.14, seed=1, smooth=4):
    """Handwritten text (ballpoint or marker strokes) on a print panel."""
    rnd = random.Random(seed + 101)
    strokes, cx = hand_strokes(text, x, y, size, slant, seed, smooth)
    for pts in strokes:
        s.stroke(pts, width * size * rnd.uniform(0.9, 1.1), color)
    return cx


def hand_w(text, size):
    return sum((0.30 if ch == ' ' else _HAND.get(ch, _HAND.get(ch.upper(), _HAND.get(ch.lower(), _HAND['-'])))[1] + 0.05) for ch in text) * size


# ================================================================== shared unbaked materials

UNB = {}


def make_unbaked():
    """The slots the game drives, one material each, shared by every mesh (so the names never pick up a .001)."""
    g = core.Mat('ns_glass')
    g.set('Base Color', H(0x10161a))
    g.set('Roughness', 0.02)
    g.set('Transmission Weight', 0.0)
    g.set('IOR', 1.5)
    g.set('Alpha', 0.10)
    g.m.use_backface_culling = False
    try:
        g.m.surface_render_method = 'BLENDED'
    except (AttributeError, TypeError):
        pass
    UNB['ns_glass'] = g.m
    # [screen]s: a stand-in picture each, so the review shows what goes there; the game draws them from core
    for n, img, glow in (('ns_screen_pawn_openclosed', openclosed_preview, 0.0), ('ns_screen_pawn_tv', tv_preview, 1.6),
                         ('ns_screen_pawn_ticket', ticket_preview, 0.0), ('ns_screen_pawn_tag', tag_preview, 0.0)):
        t = core.Mat(n)
        path = img()
        timg = t.image(path)
        if glow > 0.0:
            t.set('Base Color', H(0x050505))
            t.link(timg.outputs['Color'], t.bsdf.inputs['Emission Color'])
            t.set('Emission Strength', glow)
        else:
            t.link(timg.outputs['Color'], t.bsdf.inputs['Base Color'])
        t.set('Roughness', 0.4 if glow == 0.0 else 0.2)
        t.m.use_backface_culling = True
        UNB[n] = t.m
    for n, mt in UNB.items():
        if mt.name != n:
            raise RuntimeError(f'material name clash: wanted {n}, got {mt.name}')


def _sheet_png(s, name, px):
    color, _surface = s.render(name, px, samples=16)
    for obj, *_ in s.items:
        bpy.data.objects.remove(obj)
    return color


def openclosed_preview():
    """The door's hanging OPEN / CLOSED sign (the game flips it by the hours): OPEN shown."""
    w, h = 600.0, 260.0
    s = Sheet(w, h)
    s.rect(0, 0, w, h, 0xb3141c)
    s.rect(10, 10, w - 20, h - 20, 0xf3f1ea)
    s.text('OPEN', w / 2, h / 2 - 45, 130, 0xb3141c, 'Black', align='CENTER')
    return _sheet_png(s, 'pw_openclosed_preview', 300)


def ticket_preview():
    """The hero's pawn ticket, core-drawn as pawnticket:<n>: the pink carbon copy's layout with stand-in values."""
    w, h = 900.0, 1500.0
    s = Sheet(w, h)
    s.rect(0, 0, w, h, 0xf2c8c8)
    s.text('NORTHSIDE PAWN & LOAN', w / 2, h - 110, 52, 0x2a3a6a, 'Black', align='CENTER')
    s.text('232 MARKET ST  ·  555-0132', w / 2, h - 170, 30, 0x2a3a6a, 'Bold', align='CENTER')
    s.text('PAWN TICKET', w / 2, h - 280, 60, 0x8a1418, 'Black', align='CENTER')
    s.text('No. 0417', w - 60, h - 360, 44, 0x8a1418, 'Black', align='RIGHT')
    for i, (a, b) in enumerate((('ITEM', '(from core)'), ('LOANED', '$ --'), ('TO REDEEM', '$ --'), ('DUE', '--/--'))):
        y = h - 520 - i * 170
        s.rect(60, y - 20, w - 120, 3, 0x7a6a8a)
        s.text(a, 60, y, 36, 0x2a3a6a, 'Bold')
        s.text(b, w - 60, y, 44, 0x1b2f7a, 'Bold', align='RIGHT')
    return _sheet_png(s, 'pw_ticket_preview', 450)


def tag_preview():
    """A price tag on the used stock (core-drawn: only what the BUY tab sells shows a price)."""
    w, h = 300.0, 500.0
    s = Sheet(w, h)
    s.rect(0, 0, w, h, 0xe9d7a6)
    s.text('$ --', w / 2, h / 2 - 30, 90, 0xa3161c, 'Black', align='CENTER')
    return _sheet_png(s, 'pw_tag_preview', 128)


def tv_preview():
    """The demo TV on the electronics wall: a muted ball game, until the game draws its own picture."""
    w, h = 1280.0, 720.0
    s = Sheet(w, h)
    s.rect(0, 0, w, h, 0x2f6b3a)
    for k in range(9):
        s.rect(0, k * 80, w, 40, 0x2a6135)
    s.poly([(0, 260), (w, 330), (w, 340), (0, 270)], 0xe8e8e0)
    s.poly([(560, 0), (640, h), (650, h), (570, 0)], 0xe8e8e0, alpha=0.6)
    for i in range(7):
        s.circle(220 + i * 140, 380 + (i % 3) * 40, 12, 0xd0d0d0 if i % 2 else 0x1c2a6e)
    s.rect(0, 0, w, 110, 0x101418)
    s.rect(40, 30, 420, 52, 0xb3141c)
    s.text('NORTHSIDE  3   HARBOR  1', 60, 42, 30, 0xffffff, 'Black')
    s.text('4TH  12:41', w - 60, 42, 30, 0xffffff, 'Bold', align='RIGHT')
    return _sheet_png(s, 'pw_tv_preview', 1024)


# ================================================================== body finishes (shader)

class Ctx:
    def __init__(self, m, kit, slot):
        self.m, self.kit, self.slot = m, kit, slot
        self.tc, self.sep = object_coords(m)
        self.P = self.tc.outputs['Object']
        self.x, self.y, self.z = self.sep.outputs['X'], self.sep.outputs['Y'], self.sep.outputs['Z']
        geo = m.node('ShaderNodeNewGeometry')
        ns = m.node('ShaderNodeSeparateXYZ')
        m.link(geo.outputs['Normal'], ns.inputs['Vector'])
        self.nx, self.ny, self.nz = ns.outputs['X'], ns.outputs['Y'], ns.outputs['Z']
        self.fin = attr(m, 'pw_fin')
        self.col = attr(m, 'pw_col', 'Color')
        aux = attr(m, 'pw_aux', 'Vector')
        sa = m.node('ShaderNodeSeparateXYZ')
        m.link(aux, sa.inputs['Vector'])
        self.ax, self.ay, self.az = sa.outputs['X'], sa.outputs['Y'], sa.outputs['Z']
        aux2 = attr(m, 'pw_aux2', 'Vector')
        sb = m.node('ShaderNodeSeparateXYZ')
        m.link(aux2, sb.inputs['Vector'])
        self.bx, self.by, self.bz = sb.outputs['X'], sb.outputs['Y'], sb.outputs['Z']
        self.aux = aux


def attr(m, name, out='Fac'):
    n = m.node('ShaderNodeAttribute')
    n.attribute_type = 'GEOMETRY'
    n.attribute_name = name
    return n.outputs[out]


def mul(m, a, b):
    return m.math('MULTIPLY', a, b)


def add(m, *xs):
    out = xs[0]
    for x in xs[1:]:
        out = m.math('ADD', out, x)
    return out


def sub(m, a, b):
    return m.math('SUBTRACT', a, b)


def is_k(m, s, k):
    return m.math('LESS_THAN', m.math('ABSOLUTE', m.math('SUBTRACT', s, float(k))), 0.5)


def lighten(m, col, amount):
    return m.mix(amount, col, (1.0, 1.0, 1.0, 1.0))


def darken(m, col, f):
    return m.mix(1.0, col, (f, f, f, 1.0), blend='MULTIPLY')


def isolines(m, vector, scale, width, detail=1.0):
    """Thin wandering lines (scratches, creases): where a noise crosses its middle."""
    n = noise(m, vector, scale=scale, detail=detail)
    return ramp(m, m.math('ABSOLUTE', m.math('SUBTRACT', n, 0.5)), width, 0.0)


def white(m, vector):
    """A random value per cell of an integer lattice (vector already floored)."""
    wn = m.node('ShaderNodeTexWhiteNoise')
    wn.noise_dimensions = '3D'
    m.link(vector, wn.inputs['Vector'])
    return wn.outputs['Value']


def vfloor(m, v):
    n = m.node('ShaderNodeVectorMath')
    n.operation = 'FLOOR'
    m.link(v, n.inputs[0])
    return n.outputs[0]


def face_uv(m, c):
    """Planar coordinates on whichever axis plane the face is closest to: (u, v) sockets in meters."""
    ax_ = m.math('ABSOLUTE', c.nx)
    ay_ = m.math('ABSOLUTE', c.ny)
    az_ = m.math('ABSOLUTE', c.nz)
    is_x = m.math('GREATER_THAN', ax_, m.math('MAXIMUM', ay_, az_))
    is_y = mul(m, m.math('SUBTRACT', 1.0, is_x), m.math('GREATER_THAN', ay_, az_))
    is_z = m.math('SUBTRACT', 1.0, m.math('ADD', is_x, is_y))
    u = add(m, mul(m, is_x, c.y), mul(m, is_y, c.x), mul(m, is_z, c.x))
    v = add(m, mul(m, is_x, c.z), mul(m, is_y, c.z), mul(m, is_z, c.y))
    return u, v


def edges_of(c, distance=0.002, breakup=60.0, gain=4.0, samples=12):
    return convex_edges(c.m, c.tc, distance=distance, samples=samples, breakup_scale=breakup, gain=gain)


def f_paint(c):
    """Paint on steel or wood: roller stipple, chipped to primer and steel on the edges, scuffed low, scratched."""
    m = c.m
    stip = noise(m, c.P, scale=350.0, detail=2.0)
    edge = edges_of(c, 0.003, 50.0, 5.0)
    chip = ramp(m, edge, 0.45, 0.6)
    bare = ramp(m, edge, 0.7, 0.85)
    col = m.mix(mul(m, stip, 0.2), c.col, darken(m, c.col, 0.88))
    col = m.mix(chip, col, H(0x8a8478))
    col = m.mix(bare, col, H(0x6f7275))
    scuff = mul(m, ramp(m, noise(m, scaled(m, c.sep, 4.0, 4.0, 60.0), detail=4.0), 0.56, 0.7), ramp(m, c.z, 0.6, 0.05))
    col = m.mix(mul(m, scuff, 0.8), col, H(0x2e2b27))
    scr = mul(m, isolines(m, scaled(m, c.sep, 8.0, 8.0, 260.0), 1.0, 0.007, 2.0), mul(m, ramp(m, noise(m, c.P, scale=5.0, detail=2.0), 0.45, 0.62),
                                                                                      ramp(m, c.z, 1.6, 1.0)))
    col = m.mix(mul(m, scr, 0.22), col, lighten(m, c.col, 0.3))
    rough = add(m, 0.42, mul(m, stip, 0.1), mul(m, chip, 0.15), mul(m, scuff, 0.15))
    return col, rough, mul(m, bare, 0.8), add(m, mul(m, stip, 0.01), mul(m, chip, -0.06), mul(m, scr, -0.02))


def f_enamel(c):
    """Baked enamel (the safe, appliances): orange peel, yellowed, wiped, scratched, scuffed low, chipped."""
    m = c.m
    peel = noise(m, c.P, scale=500.0, detail=1.0)
    age = ramp(m, noise(m, c.P, scale=2.5, detail=3.0), 0.3, 0.75)
    col = m.mix(mul(m, age, 0.5), c.col, darken(m, c.col, 0.84))
    wipe = ramp(m, noise(m, scaled(m, c.sep, 50.0, 50.0, 3.0), detail=3.0), 0.56, 0.7)
    col = m.mix(mul(m, wipe, 0.08), col, lighten(m, c.col, 0.25))
    scr = mul(m, isolines(m, scaled(m, c.sep, 6.0, 6.0, 140.0), 1.0, 0.006, 2.0), ramp(m, noise(m, c.P, scale=6.0, detail=2.0), 0.45, 0.65))
    col = m.mix(mul(m, scr, 0.12), col, lighten(m, c.col, 0.35))
    scuff = mul(m, ramp(m, c.z, 0.45, 0.06), ramp(m, noise(m, scaled(m, c.sep, 8.0, 8.0, 150.0), detail=4.0), 0.58, 0.72))
    col = m.mix(mul(m, scuff, 0.55), col, H(0x4e483c))
    chips = ramp(m, edges_of(c, 0.002, 60.0), 0.45, 0.6)
    col = m.mix(chips, col, m.mix(ramp(m, noise(m, c.P, scale=120.0, detail=2.0), 0.4, 0.6), H(0x5b5d5f), H(0x5a2e16)))
    rough = mix_float(m, chips, add(m, 0.28, mul(m, peel, 0.06), mul(m, wipe, -0.06), mul(m, scuff, 0.18), mul(m, scr, 0.1)), 0.6)
    return col, rough, mix_float(m, chips, 0.0, 0.4), add(m, mul(m, peel, 0.012), mul(m, chips, -0.05), mul(m, scr, -0.01))


def f_steel(c):
    """Brushed stainless, wiped: faint smears and fingerprints."""
    m = c.m
    br = noise(m, scaled(m, c.sep, 4.0, 400.0, 900.0), detail=3.0)
    fine = noise(m, scaled(m, c.sep, 20.0, 2400.0, 3000.0), detail=1.0)
    smudge = ramp(m, noise(m, c.P, scale=11.0, detail=4.0), 0.56, 0.8)
    prints = ramp(m, noise(m, c.P, scale=40.0, detail=3.0, distortion=1.2), 0.62, 0.72)
    col = m.mix(br, H(0x8a8e92), H(0xb4b8bb))
    col = m.mix(mul(m, smudge, 0.14), col, H(0x74777a))
    rough = add(m, 0.22, mul(m, br, 0.05), mul(m, fine, 0.06), mul(m, smudge, 0.07), mul(m, prints, 0.08))
    return col, rough, 1.0, mul(m, fine, 0.004)


def f_chrome(c):
    m = c.m
    sm = ramp(m, noise(m, c.P, scale=12.0, detail=4.0), 0.45, 0.8)
    pit = ramp(m, noise(m, c.P, scale=600.0, detail=2.0), 0.72, 0.8)
    col = m.mix(mul(m, pit, 0.6), H(0xd0d2d4), H(0x77736c))
    rough = add(m, 0.05, mul(m, sm, 0.10), mul(m, pit, 0.25))
    return col, rough, 1.0, mul(m, pit, -0.01)


def f_nickel(c):
    """Satin nickel (guitar hardware, tool chucks): a little warmer and softer than chrome, tarnished in the recesses."""
    m = c.m
    sm = ramp(m, noise(m, c.P, scale=30.0, detail=4.0), 0.4, 0.8)
    col = m.mix(mul(m, sm, 0.3), H(0xb7b4ab), H(0x8a8576))
    return col, add(m, 0.16, mul(m, sm, 0.12)), 1.0, 0.0


def f_alu(c):
    m = c.m
    br = noise(m, scaled(m, c.sep, 600.0, 600.0, 6.0), detail=3.0)
    sm = ramp(m, noise(m, c.P, scale=10.0, detail=4.0), 0.5, 0.8)
    col = m.mix(br, H(0xa9acaf), H(0xc4c7ca))
    return m.mix(mul(m, sm, 0.3), col, H(0x7c7f82)), add(m, 0.3, mul(m, br, 0.06), mul(m, sm, 0.12)), 1.0, mul(m, br, 0.003)


def f_bronze(c):
    """Dark-bronze anodized storefront aluminum: satin, oxidized chalky where the weather reaches, scratched bright at
    hand height and where keys and carts go, grime at the foot."""
    m = c.m
    br = noise(m, scaled(m, c.sep, 300.0, 300.0, 5.0), detail=3.0)
    chalk = ramp(m, noise(m, c.P, scale=3.0, detail=4.0), 0.45, 0.75)
    col = m.mix(br, H(0x2b2520), H(0x372f28))
    col = m.mix(mul(m, chalk, 0.35), col, H(0x5a524a))
    scr = mul(m, isolines(m, scaled(m, c.sep, 30.0, 30.0, 180.0), 1.0, 0.006, 2.0),
              mul(m, ramp(m, noise(m, c.P, scale=6.0, detail=2.0), 0.4, 0.62), ramp(m, c.z, 1.8, 1.2)))
    col = m.mix(mul(m, scr, 0.8), col, H(0x9a948c))
    edge = ramp(m, edges_of(c, 0.0015, 90.0), 0.5, 0.7)
    col = m.mix(mul(m, edge, 0.5), col, H(0x8c857c))
    rough = add(m, 0.38, mul(m, br, 0.06), mul(m, chalk, 0.2), mul(m, scr, -0.1))
    return col, rough, mix_float(m, chalk, 0.85, 0.5), mul(m, scr, -0.01)


def f_black_metal(c):
    """Black powder-coated or blackened steel (bars, brackets, tool bodies' metal): satin, rubbed to steel on edges."""
    m = c.m
    peel = noise(m, c.P, scale=700.0, detail=1.0)
    edge = ramp(m, edges_of(c, 0.002, 70.0), 0.5, 0.7)
    rustp = ramp(m, noise(m, c.P, scale=40.0, detail=5.0), 0.68, 0.76)
    col = m.mix(peel, H(0x141516), H(0x1c1d1f))
    col = m.mix(mul(m, edge, 0.7), col, H(0x6c6e70))
    col = m.mix(mul(m, rustp, 0.6), col, H(0x4a2a16))
    return col, add(m, 0.5, mul(m, peel, 0.08), mul(m, edge, -0.18), mul(m, rustp, 0.3)), mul(m, edge, 0.8), mul(m, peel, 0.004)


def f_plastic(c, rough0=0.5):
    m = c.m
    g = noise(m, c.P, scale=1800.0, detail=1.0)
    edges = mul(m, edges_of(c, 0.0012, 220.0), 0.6)
    col = m.mix(mul(m, edges, 0.5), c.col, lighten(m, c.col, 0.25))
    scr = mul(m, isolines(m, scaled(m, c.sep, 20.0, 20.0, 90.0), 1.0, 0.005, 2.0), ramp(m, noise(m, c.P, scale=8.0, detail=2.0), 0.5, 0.66))
    col = m.mix(mul(m, scr, 0.3), col, lighten(m, c.col, 0.3))
    rough = m.math('SUBTRACT', add(m, rough0, mul(m, g, 0.06), mul(m, scr, 0.1)), mul(m, edges, 0.1))
    return col, rough, 0.0, mul(m, g, 0.004)


def f_gloss(c):
    """Gloss plastic or lacquer: smooth, smeared, micro-scratched."""
    m = c.m
    scratch = ramp(m, noise(m, c.P, scale=900.0, detail=4.0, distortion=2.0), 0.62, 0.7)
    smear = ramp(m, noise(m, c.P, scale=14.0, detail=3.0), 0.5, 0.8)
    col = m.mix(mul(m, smear, 0.08), c.col, lighten(m, c.col, 0.15))
    return col, add(m, 0.08, mul(m, scratch, 0.12), mul(m, smear, 0.1)), 0.0, 0.0


def f_rubber(c):
    m = c.m
    g = noise(m, c.P, scale=900.0, detail=1.0)
    dust = ramp(m, noise(m, c.P, scale=20.0, detail=3.0), 0.5, 0.8)
    col = m.mix(mul(m, dust, 0.3), c.col, H(0x5a5650))
    return col, add(m, 0.8, mul(m, g, 0.08)), 0.0, mul(m, g, 0.01)


def f_powder(c):
    m = c.m
    peel = noise(m, c.P, scale=900.0, detail=1.0)
    edge = ramp(m, edges_of(c, 0.0015, 120.0), 0.45, 0.6)
    col = m.mix(edge, c.col, H(0x8b8e93))
    rough = mix_float(m, edge, add(m, 0.45, mul(m, peel, 0.08)), 0.32)
    return col, rough, mix_float(m, edge, 0.0, 1.0), mul(m, peel, 0.006)


def f_laminate(c, flecks=1.0):
    """High-pressure laminate: a speckled print, worn smooth where hands slide, stained, scratched."""
    m = c.m
    mott = noise(m, c.P, scale=40.0, detail=3.0)
    sp1 = ramp(m, noise(m, c.P, scale=330.0, detail=1.0), 0.68, 0.71)
    sp2 = ramp(m, noise(m, vec(m, c.x, c.y, add(m, c.z, 3.1)), scale=420.0, detail=1.0), 0.3, 0.27)
    col = m.mix(mul(m, mott, 0.25), c.col, darken(m, c.col, 0.9))
    col = m.mix(mul(m, sp1, 0.6 * flecks), col, darken(m, c.col, 0.55))
    col = m.mix(mul(m, sp2, 0.5 * flecks), col, lighten(m, c.col, 0.5))
    worn = ramp(m, noise(m, c.P, scale=1.6, detail=3.0), 0.45, 0.7)
    col = m.mix(mul(m, worn, 0.12), col, lighten(m, c.col, 0.3))
    stain = ramp(m, noise(m, c.P, scale=5.0, detail=5.0, distortion=0.6), 0.68, 0.75)
    col = m.mix(mul(m, stain, 0.3), col, H(0x8a7a5a))
    scr = mul(m, isolines(m, scaled(m, c.sep, 120.0, 9.0, 1.0), 1.0, 0.005, 2.0), ramp(m, noise(m, c.P, scale=4.0, detail=2.0), 0.4, 0.6))
    col = m.mix(mul(m, scr, 0.3), col, darken(m, c.col, 0.7))
    rough = add(m, 0.34, mul(m, worn, -0.1), mul(m, stain, 0.12), mul(m, scr, 0.08))
    return col, rough, 0.0, mul(m, sp1, 0.002)


def f_melamine(c):
    """Melamine board (slatwall, shelf panels, the cage's desk): a faint texture, worn and grubby at the edges."""
    m = c.m
    g = noise(m, c.P, scale=300.0, detail=2.0)
    grub = ramp(m, edges_of(c, 0.004, 40.0, 4.0), 0.4, 0.7)
    col = m.mix(mul(m, g, 0.12), c.col, darken(m, c.col, 0.9))
    col = m.mix(mul(m, grub, 0.4), col, darken(m, c.col, 0.6))
    return col, add(m, 0.42, mul(m, g, 0.08), mul(m, grub, 0.12)), 0.0, mul(m, g, 0.003)


def f_particle(c):
    m = c.m
    n = noise(m, c.P, scale=400.0, detail=3.0)
    return m.mix(n, H(0x9c7a50), H(0x6e5232)), 0.9, 0.0, mul(m, n, 0.1)


def f_paper(c):
    m = c.m
    fib = noise(m, scaled(m, c.sep, 900.0, 300.0, 900.0), detail=2.0)
    yel = ramp(m, noise(m, c.P, scale=8.0, detail=3.0), 0.4, 0.8)
    col = m.mix(mul(m, fib, 0.12), c.col, darken(m, c.col, 0.85))
    col = m.mix(mul(m, yel, 0.15), col, H(0xc8b88a))
    return col, add(m, 0.78, mul(m, fib, 0.1)), 0.0, mul(m, fib, 0.004)


def f_cardboard(c):
    """Corrugated board: a kraft brown with the flutes showing faintly through the liner, crushed corners, tape shine."""
    m = c.m
    u, v = face_uv(m, c)
    flute = m.math('SINE', mul(m, add(m, u, mul(m, v, 0.0)), 2 * math.pi / 0.008))
    fib = noise(m, scaled(m, c.sep, 600.0, 600.0, 600.0), detail=2.0)
    blot = ramp(m, noise(m, c.P, scale=6.0, detail=3.0), 0.4, 0.8)
    col = m.mix(mul(m, fib, 0.2), c.col, darken(m, c.col, 0.85))
    col = m.mix(mul(m, blot, 0.25), col, darken(m, c.col, 0.7))
    crush = ramp(m, edges_of(c, 0.004, 30.0), 0.4, 0.7)
    col = m.mix(mul(m, crush, 0.4), col, lighten(m, c.col, 0.2))
    return col, add(m, 0.82, mul(m, fib, 0.06)), 0.0, add(m, mul(m, flute, 0.02), mul(m, fib, 0.01))


def f_cloth(c):
    m = c.m
    period = 0.002
    wa = m.math('SINE', mul(m, add(m, c.x, c.z), 2 * math.pi / period))
    wb = m.math('SINE', mul(m, add(m, c.y, c.z), 2 * math.pi / period))
    weave = mul(m, add(m, mul(m, wa, wb), 1.0), 0.5)
    heather = noise(m, c.P, scale=600.0, detail=2.0)
    fold = ramp(m, noise(m, c.P, scale=30.0, detail=3.0), 0.3, 0.7)
    col = m.mix(mul(m, heather, 0.4), c.col, lighten(m, c.col, 0.12))
    col = m.mix(mul(m, fold, 0.25), col, darken(m, c.col, 0.7))
    return col, 0.92, 0.0, add(m, mul(m, weave, 0.08), mul(m, heather, 0.04), mul(m, fold, 0.6))


def f_felt(c):
    m = c.m
    n = noise(m, c.P, scale=500.0, detail=4.0)
    lint = ramp(m, noise(m, c.P, scale=60.0, detail=3.0), 0.66, 0.76)
    col = m.mix(mul(m, n, 0.4), c.col, lighten(m, c.col, 0.12))
    col = m.mix(mul(m, lint, 0.5), col, H(0x8a8478))
    return col, 0.95, 0.0, mul(m, n, 0.05)


def f_velvet(c):
    """Jeweler's velvet: deep color, a fine pile that's crushed where things have sat, lint and dust caught in it."""
    m = c.m
    pile = noise(m, c.P, scale=2600.0, detail=2.0)
    crush = ramp(m, noise(m, c.P, scale=45.0, detail=3.0), 0.45, 0.75)
    lint = ramp(m, noise(m, c.P, scale=220.0, detail=2.0), 0.81, 0.84)
    col = m.mix(mul(m, crush, 0.35), c.col, lighten(m, c.col, 0.14))
    col = m.mix(mul(m, pile, 0.2), col, darken(m, c.col, 0.8))
    col = m.mix(mul(m, lint, 0.3), col, H(0x9a958b))
    return col, add(m, 0.86, mul(m, crush, -0.12)), 0.0, mul(m, pile, 0.02)


def f_gold(c):
    """Gold (jewelry, gilding): pw_col tints it (yellow, rose, white); polished, with micro-scratches and a little tarnish
    in the recesses; aux.x adds roughness (brushed or satin finishes)."""
    m = c.m
    scr = ramp(m, noise(m, c.P, scale=1400.0, detail=4.0, distortion=2.0), 0.6, 0.7)
    tarn = ramp(m, noise(m, c.P, scale=90.0, detail=3.0), 0.55, 0.8)
    col = m.mix(mul(m, tarn, 0.25), c.col, darken(m, c.col, 0.7))
    rough = add(m, 0.1, c.ax, mul(m, scr, 0.1), mul(m, tarn, 0.08))
    return col, rough, 1.0, mul(m, scr, -0.002)


def f_gem(c):
    """A cut stone: deep color with bright facet flashes baked as variation (no refraction in a bake)."""
    m = c.m
    flash = ramp(m, noise(m, scaled(m, c.sep, 900.0, 900.0, 900.0), detail=0.0), 0.62, 0.66)
    col = m.mix(mul(m, flash, 0.7), c.col, lighten(m, c.col, 0.7))
    return col, 0.03, 0.0, 0.0


def f_pearl(c):
    m = c.m
    n = noise(m, c.P, scale=900.0, detail=3.0)
    col = m.mix(n, c.col, H(0xd8cfe2))
    return col, add(m, 0.18, mul(m, n, 0.05)), 0.0, 0.0


def f_leather(c):
    m = c.m
    vor = m.node('ShaderNodeTexVoronoi', Scale=900.0)
    m.link(c.P, vor.inputs['Vector'])
    grain = vor.outputs['Distance']
    crease = isolines(m, c.P, 60.0, 0.03, 3.0)
    worn = ramp(m, edges_of(c, 0.002, 80.0), 0.4, 0.7)
    col = m.mix(mul(m, grain, 0.3), c.col, darken(m, c.col, 0.75))
    col = m.mix(mul(m, worn, 0.5), col, lighten(m, c.col, 0.2))
    return col, add(m, 0.5, mul(m, grain, 0.15), mul(m, worn, -0.15)), 0.0, add(m, mul(m, grain, 0.03), mul(m, crease, -0.1))


def _grain(c, axis_select):
    """A wood figure: long fibers along the axis aux.x picks (0 z, 1 x, 2 y), rings as stretched noise."""
    m = c.m
    along_z = noise(m, scaled(m, c.sep, 60.0, 60.0, 2.0), detail=5.0, distortion=0.8)
    along_x = noise(m, scaled(m, c.sep, 2.0, 60.0, 60.0), detail=5.0, distortion=0.8)
    along_y = noise(m, scaled(m, c.sep, 60.0, 2.0, 60.0), detail=5.0, distortion=0.8)
    fz = noise(m, scaled(m, c.sep, 900.0, 900.0, 12.0), detail=2.0)
    fx = noise(m, scaled(m, c.sep, 12.0, 900.0, 900.0), detail=2.0)
    fy = noise(m, scaled(m, c.sep, 900.0, 12.0, 900.0), detail=2.0)
    sz = m.math('LESS_THAN', axis_select, 0.5)
    sx = mul(m, m.math('GREATER_THAN', axis_select, 0.5), m.math('LESS_THAN', axis_select, 1.5))
    sy = m.math('GREATER_THAN', axis_select, 1.5)
    rings = add(m, mul(m, along_z, sz), mul(m, along_x, sx), mul(m, along_y, sy))
    fibers = add(m, mul(m, fz, sz), mul(m, fx, sx), mul(m, fy, sy))
    return rings, fibers


def f_wood(c):
    """Wood with a clear finish (desk tops, fretboards, guitar necks, the display deck's trim)."""
    m = c.m
    rings, fibers = _grain(c, c.ax)
    band = ramp(m, m.math('FRACT', mul(m, rings, 9.0)), 0.3, 0.7)
    col = m.mix(mul(m, band, 0.45), c.col, darken(m, c.col, 0.62))
    col = m.mix(mul(m, fibers, 0.25), col, darken(m, c.col, 0.8))
    worn = ramp(m, edges_of(c, 0.002, 80.0), 0.45, 0.7)
    col = m.mix(mul(m, worn, 0.3), col, lighten(m, c.col, 0.2))
    return col, add(m, 0.38, mul(m, fibers, 0.12), mul(m, worn, -0.1)), 0.0, add(m, mul(m, band, 0.004), mul(m, fibers, 0.004))


def f_lacquer(c):
    """A guitar's finish: gloss over wood. pw_col is the color coat; pw_aux the body's middle (it moves with the guitar)
    and pw_aux2 = (burst radius, trans, 0): a burst darkens toward near-black at the rim (radius 0: none); through a
    trans coat (trans 1) the grain shows. Buckle rash, pick wear and dings."""
    m = c.m
    rings, fibers = _grain(c, 0.0)
    band = ramp(m, m.math('FRACT', mul(m, rings, 7.0)), 0.3, 0.7)
    d = m.node('ShaderNodeVectorMath')
    d.operation = 'DISTANCE'
    m.link(c.P, d.inputs[0])
    m.link(c.aux, d.inputs[1])
    dist = d.outputs['Value']
    r = c.bx
    burst = mul(m, m.math('GREATER_THAN', r, 0.001), ramp(m, m.math('DIVIDE', dist, m.math('MAXIMUM', r, 0.01)), 0.55, 1.0))
    trans = m.math('GREATER_THAN', c.by, 0.5)
    col = m.mix(mul(m, trans, mul(m, band, 0.5)), c.col, darken(m, c.col, 0.55))
    col = m.mix(mul(m, burst, 0.92), col, H(0x140a05))
    ding = ramp(m, noise(m, c.P, scale=160.0, detail=2.0), 0.73, 0.78)
    col = m.mix(mul(m, ding, 0.5), col, H(0xd8c9a8))
    scr = ramp(m, noise(m, c.P, scale=700.0, detail=4.0, distortion=2.0), 0.63, 0.7)
    smear = ramp(m, noise(m, c.P, scale=12.0, detail=3.0), 0.55, 0.8)
    rough = add(m, 0.05, mul(m, scr, 0.12), mul(m, smear, 0.08), mul(m, ding, 0.3))
    return col, rough, 0.0, add(m, mul(m, ding, -0.04), mul(m, fibers, 0.001))


def f_tolex(c):
    """Amp covering: pebble-grain vinyl, scuffed grey on the corners and the top where things were set down."""
    m = c.m
    vor = m.node('ShaderNodeTexVoronoi', Scale=700.0)
    m.link(c.P, vor.inputs['Vector'])
    peb = vor.outputs['Distance']
    scuff = ramp(m, edges_of(c, 0.006, 40.0, 4.0), 0.35, 0.65)
    col = m.mix(mul(m, peb, 0.4), c.col, lighten(m, c.col, 0.1))
    col = m.mix(mul(m, scuff, 0.55), col, lighten(m, c.col, 0.35))
    return col, add(m, 0.55, mul(m, peb, 0.15), mul(m, scuff, 0.15)), 0.0, add(m, mul(m, peb, 0.25), mul(m, scuff, -0.05))


def f_grill(c):
    """Speaker grill cloth: a coarse basket weave, faded, dust in the weave."""
    m = c.m
    u, v = face_uv(m, c)
    p = 0.0035
    wa = m.math('SINE', mul(m, u, 2 * math.pi / p))
    wb = m.math('SINE', mul(m, v, 2 * math.pi / p))
    weave = mul(m, add(m, mul(m, wa, wb), 1.0), 0.5)
    fade = ramp(m, noise(m, c.P, scale=4.0, detail=3.0), 0.4, 0.8)
    col = m.mix(mul(m, weave, 0.35), c.col, darken(m, c.col, 0.6))
    col = m.mix(mul(m, fade, 0.2), col, lighten(m, c.col, 0.2))
    return col, 0.85, 0.0, mul(m, weave, 0.05)


def f_screen(c):
    """A display switched off: near-black glass, smudged and dusty."""
    m = c.m
    smear = ramp(m, noise(m, c.P, scale=9.0, detail=4.0), 0.45, 0.85)
    dust = ramp(m, noise(m, c.P, scale=300.0, detail=2.0), 0.6, 0.75)
    col = m.mix(mul(m, dust, 0.12), H(0x07080a), H(0x6a6a66))
    return col, add(m, 0.07, mul(m, smear, 0.12), mul(m, dust, 0.1)), 0.0, 0.0


def f_brass(c):
    m = c.m
    tarn = ramp(m, noise(m, c.P, scale=25.0, detail=4.0), 0.35, 0.8)
    rub = ramp(m, edges_of(c, 0.002, 70.0), 0.4, 0.7)
    col = m.mix(mul(m, tarn, 0.7), H(0xb08a3e), H(0x5e4a22))
    col = m.mix(rub, col, H(0xd8b060))
    return col, add(m, 0.3, mul(m, tarn, 0.25), mul(m, rub, -0.2)), 1.0, 0.0


def f_galv(c):
    """Galvanized steel (the cage's wire, conduit, shelving posts)."""
    m = c.m
    spangle = noise(m, c.P, scale=60.0, detail=1.0, distortion=1.5)
    patina = noise(m, c.P, scale=5.0, detail=4.0)
    col = m.mix(mul(m, spangle, 0.25), H(0x9ea3a6), H(0xc3c7c9))
    col = m.mix(mul(m, patina, 0.5), col, H(0x7d8184))
    return col, add(m, 0.45, mul(m, sub(m, spangle, 0.5), 0.15)), 0.9, 0.0


def f_wire(c):
    """Welded wire mesh, painted: c.col over galvanizing that shows on the crossings."""
    m = c.m
    edge = ramp(m, edges_of(c, 0.0012, 120.0), 0.4, 0.6)
    col = m.mix(edge, c.col, H(0x9ea3a6))
    return col, add(m, 0.5, mul(m, edge, -0.15)), mul(m, edge, 0.8), 0.0


def f_rust(c):
    m = c.m
    n = noise(m, c.P, scale=40.0, detail=6.0)
    col = m.mix(n, H(0x5a3018), H(0x8a4a20))
    return col, 0.85, 0.2, mul(m, n, 0.02)


def f_kick(c):
    m = c.m
    streak = noise(m, scaled(m, c.sep, 3.0, 3.0, 80.0), detail=4.0)
    scuff = mul(m, ramp(m, streak, 0.58, 0.72), ramp(m, noise(m, c.P, scale=9.0, detail=3.0), 0.35, 0.65))
    col = m.mix(scuff, c.col, H(0x8a8a86))
    return col, add(m, 0.4, mul(m, scuff, 0.2)), mul(m, scuff, 0.5), 0.0


def f_cast(c):
    """Die-cast aluminum (tool gearboxes, the safe's handle hub): a fine sand-cast grain, satin."""
    m = c.m
    g = noise(m, c.P, scale=1500.0, detail=2.0)
    col = m.mix(g, H(0x8e9194), H(0xa4a7aa))
    return col, add(m, 0.42, mul(m, g, 0.1)), 1.0, mul(m, g, 0.006)


def f_mat(c):
    """A rubber entrance mat: ribbed, gritty, worn pale in the middle."""
    m = c.m
    rib = m.math('SINE', mul(m, c.y, 2 * math.pi / 0.012))
    grit = ramp(m, noise(m, c.P, scale=200.0, detail=2.0), 0.55, 0.7)
    worn = ramp(m, noise(m, c.P, scale=2.0, detail=3.0), 0.35, 0.7)
    col = m.mix(mul(m, grit, 0.5), c.col, H(0x6a645a))
    col = m.mix(mul(m, worn, 0.3), col, H(0x4a463f))
    return col, 0.9, 0.0, mul(m, rib, 0.08)


def f_foam(c):
    m = c.m
    g = noise(m, c.P, scale=1200.0, detail=2.0)
    return m.mix(mul(m, g, 0.3), c.col, darken(m, c.col, 0.8)), 0.95, 0.0, mul(m, g, 0.02)


def f_tile(c):
    """Glazed ceramic tile (the bulkhead): aux = (tile width, tile height, 0); grout lines, a tone per tile, crazing and
    chips, road grime in the grout. Tiles run on the face's own plane."""
    m = c.m
    u, v = face_uv(m, c)
    tw = m.math('MAXIMUM', c.ax, 0.02)
    th = m.math('MAXIMUM', c.ay, 0.02)
    fu = m.math('DIVIDE', u, tw)
    fv = m.math('DIVIDE', v, th)
    gu = m.math('ABSOLUTE', sub(m, m.math('FRACT', fu), 0.5))
    gv = m.math('ABSOLUTE', sub(m, m.math('FRACT', fv), 0.5))
    gw_u = sub(m, 0.5, m.math('DIVIDE', 0.0025, tw))
    gw_v = sub(m, 0.5, m.math('DIVIDE', 0.0025, th))
    grout = m.math('MAXIMUM', m.math('GREATER_THAN', gu, gw_u), m.math('GREATER_THAN', gv, gw_v))
    cell = vfloor(m, vec(m, fu, fv, mul(m, c.nx, 3.0)))
    tone = white(m, cell)
    col = m.mix(mul(m, tone, 0.35), c.col, darken(m, c.col, 0.78))
    craze = isolines(m, c.P, 90.0, 0.01, 3.0)
    col = m.mix(mul(m, craze, 0.25), col, darken(m, c.col, 0.6))
    grime = ramp(m, c.z, 0.5, 0.0)
    col = m.mix(grout, col, m.mix(grime, H(0x8a8478), H(0x3a3630)))
    chip = mul(m, ramp(m, edges_of(c, 0.003, 50.0), 0.5, 0.7), 0.8)
    col = m.mix(chip, col, H(0xa89c88))
    rough = add(m, mix_float(m, grout, 0.12, 0.85), mul(m, chip, 0.5), mul(m, craze, 0.05))
    return col, rough, 0.0, add(m, mul(m, grout, -0.4), mul(m, tone, 0.02), mul(m, chip, -0.1))


def f_terrazzo(c):
    """Terrazzo (the entry's floor): marble chips in a grey cement matrix, honed, scuffed dull where feet go."""
    m = c.m
    vor = m.node('ShaderNodeTexVoronoi', Scale=160.0)
    vor.feature = 'F1'
    m.link(c.P, vor.inputs['Vector'])
    chip_id = vor.outputs['Color']
    dist = vor.outputs['Distance']
    chips = ramp(m, dist, 0.38, 0.30)
    chip_col = m.mix(ramp(m, noise(m, chip_id, scale=1.0), 0.3, 0.7), H(0xe9e4d8), H(0x7a6f62))
    chip_col = m.mix(ramp(m, noise(m, chip_id, scale=3.0), 0.7, 0.75), chip_col, H(0x2a2a2a))
    base = m.mix(noise(m, c.P, scale=40.0, detail=2.0), c.col, darken(m, c.col, 0.85))
    col = m.mix(chips, base, chip_col)
    dull = ramp(m, noise(m, c.P, scale=1.5, detail=3.0), 0.4, 0.7)
    col = m.mix(mul(m, dull, 0.25), col, H(0x5a554c))
    return col, add(m, 0.3, mul(m, dull, 0.3)), 0.0, mul(m, chips, 0.002)


def f_vct(c):
    """Vinyl composition tile, 12 in (the shop's floor): a warm putty field with every tile its own tone and its marbling
    quarter-turned from its neighbors', a few darker tiles from a later patch, a wax film built up at the walls and worn
    through on the paths (the kit's 'floor' image: R traffic, G wax at edges, B spills), heel marks, grit in the joints."""
    m = c.m
    ts = 0.3048
    fu = m.math('DIVIDE', c.x, ts)
    fv = m.math('DIVIDE', c.y, ts)
    gu = m.math('ABSOLUTE', sub(m, m.math('FRACT', fu), 0.5))
    gv = m.math('ABSOLUTE', sub(m, m.math('FRACT', fv), 0.5))
    joint = m.math('MAXIMUM', ramp(m, gu, 0.4955, 0.4995), ramp(m, gv, 0.4955, 0.4995))
    cell = vfloor(m, vec(m, fu, fv, 0.0))
    t1 = white(m, cell)
    t2 = white(m, add_vec(m, cell, (17.0, 3.0, 5.0)))
    sc = m.node('ShaderNodeSeparateXYZ')
    m.link(cell, sc.inputs['Vector'])
    parity = mul(m, m.math('FRACT', mul(m, add(m, sc.outputs['X'], sc.outputs['Y']), 0.5)), 2.0)
    marb_x = noise(m, scaled(m, c.sep, 9.0, 70.0, 1.0), detail=4.0, distortion=0.6)
    marb_y = noise(m, scaled(m, c.sep, 70.0, 9.0, 1.0), detail=4.0, distortion=0.6)
    marb = mix_float(m, parity, marb_x, marb_y)
    accent = m.math('GREATER_THAN', t1, 0.90)
    field = m.mix(t2, H(0xb8b1a0), H(0xaea694))
    col = m.mix(accent, field, H(0x8f8878))
    col = m.mix(mul(m, ramp(m, marb, 0.58, 0.78), 0.55), col, lighten(m, col, 0.35))
    col = m.mix(mul(m, ramp(m, marb, 0.40, 0.22), 0.45), col, darken(m, col, 0.72))
    chips_ = ramp(m, noise(m, c.P, scale=380.0, detail=1.0), 0.67, 0.70)
    col = m.mix(mul(m, chips_, 0.4), col, darken(m, col, 0.6))
    traffic, wax, spill = c.kit_floor
    dirt = m.mix(ramp(m, noise(m, c.P, scale=9.0, detail=4.0), 0.3, 0.7), H(0x7a7466), H(0x6a6457))
    col = m.mix(mul(m, traffic, 0.5), col, dirt)
    col = m.mix(mul(m, wax, 0.35), col, H(0x9a8a62))
    col = m.mix(mul(m, spill, 0.5), col, H(0x4a4234))
    heel = mul(m, ramp(m, noise(m, scaled(m, c.sep, 30.0, 30.0, 1.0), detail=3.0), 0.69, 0.72), add(m, 0.3, traffic))
    heel = mul(m, heel, ramp(m, noise(m, c.P, scale=3.0, detail=2.0), 0.45, 0.6))
    col = m.mix(mul(m, heel, 0.75), col, H(0x1a1816))
    col = m.mix(mul(m, joint, 0.8), col, H(0x4a453c))
    scr = mul(m, isolines(m, scaled(m, c.sep, 14.0, 14.0, 1.0), 1.0, 0.006, 2.0), add(m, 0.2, traffic))
    col = m.mix(mul(m, scr, 0.25), col, H(0x8c877c))
    rough = add(m, 0.30, mul(m, traffic, 0.32), mul(m, wax, -0.12), mul(m, joint, 0.3), mul(m, marb, 0.05), mul(m, scr, 0.1))
    return col, rough, 0.0, add(m, mul(m, joint, -0.2), mul(m, scr, -0.02), mul(m, marb, 0.003))


def add_vec(m, a, b):
    n = m.node('ShaderNodeVectorMath')
    n.operation = 'ADD'
    m.link(a, n.inputs[0])
    n.inputs[1].default_value = b
    return n.outputs[0]


def f_acoustic(c):
    """Lay-in acoustic ceiling tile: fissured mineral fiber, yellowed, with the water stains (the kit's 'ceil' image)."""
    m = c.m
    fis = isolines(m, scaled(m, c.sep, 30.0, 30.0, 30.0), 1.0, 0.03, 4.0)
    pin = ramp(m, noise(m, c.P, scale=500.0, detail=1.0), 0.66, 0.7)
    yel = ramp(m, noise(m, c.P, scale=0.8, detail=3.0), 0.3, 0.8)
    col = m.mix(mul(m, yel, 0.3), c.col, H(0xcfc3a2))
    col = m.mix(mul(m, fis, 0.4), col, darken(m, c.col, 0.7))
    col = m.mix(mul(m, pin, 0.3), col, darken(m, c.col, 0.75))
    stain = c.kit_stain
    ring_ = ramp(m, m.math('ABSOLUTE', sub(m, stain, 0.5)), 0.12, 0.0)
    col = m.mix(mul(m, stain, 0.55), col, H(0xa88a52))
    col = m.mix(mul(m, ring_, 0.5), col, H(0x7a5a2a))
    return col, 0.92, 0.0, add(m, mul(m, fis, -0.15), mul(m, pin, -0.05))


def f_wallpaint(c):
    """Flat latex on plaster and drywall: roller texture, scuffed at chair and cart height and grubby at the base, hand
    marks along the walls, patched and touched-up in a slightly different white, nail holes."""
    m = c.m
    roll = noise(m, scaled(m, c.sep, 25.0, 25.0, 4.0), detail=3.0)
    orange = noise(m, c.P, scale=450.0, detail=2.0)
    col = m.mix(mul(m, roll, 0.12), c.col, darken(m, c.col, 0.9))
    patch = ramp(m, noise(m, scaled(m, c.sep, 1.4, 1.4, 1.4), detail=2.0), 0.62, 0.64)
    col = m.mix(mul(m, patch, 0.5), col, lighten(m, c.col, 0.08))
    scuff = mul(m, mul(m, ramp(m, c.z, 0.0, 0.08), ramp(m, c.z, 1.15, 0.6)),
                ramp(m, noise(m, scaled(m, c.sep, 10.0, 10.0, 60.0), detail=4.0), 0.6, 0.72))
    col = m.mix(mul(m, scuff, 0.7), col, H(0x5a554c))
    base = mul(m, ramp(m, c.z, 0.25, 0.0), ramp(m, noise(m, c.P, scale=6.0, detail=3.0), 0.3, 0.7))
    col = m.mix(mul(m, base, 0.45), col, H(0x6a6252))
    hands = mul(m, mul(m, ramp(m, c.z, 0.9, 1.2), ramp(m, c.z, 1.7, 1.4)), ramp(m, noise(m, c.P, scale=3.0, detail=4.0), 0.55, 0.75))
    col = m.mix(mul(m, hands, 0.25), col, H(0x8a7f68))
    holes = ramp(m, noise(m, c.P, scale=90.0, detail=1.0), 0.78, 0.8)
    col = m.mix(mul(m, holes, 0.7), col, H(0x3a3630))
    return col, add(m, 0.82, mul(m, roll, 0.06), mul(m, scuff, -0.1)), 0.0, add(m, mul(m, orange, 0.004), mul(m, holes, -0.05), mul(m, roll, 0.002))


FINISH = {'paint': f_paint, 'enamel': f_enamel, 'steel': f_steel, 'chrome': f_chrome, 'alu': f_alu, 'bronze': f_bronze, 'plastic': f_plastic,
          'gloss': f_gloss, 'rubber': f_rubber, 'powder': f_powder, 'laminate': f_laminate, 'paper': f_paper, 'cardboard': f_cardboard,
          'cloth': f_cloth, 'felt': f_felt, 'gold': f_gold, 'gem': f_gem, 'velvet': f_velvet, 'leather': f_leather, 'wood': f_wood,
          'lacquer': f_lacquer, 'tolex': f_tolex, 'grill': f_grill, 'screen': f_screen, 'brass': f_brass, 'wire': f_wire, 'tile': f_tile,
          'terrazzo': f_terrazzo, 'vct': f_vct, 'acoustic': f_acoustic, 'wallpaint': f_wallpaint, 'melamine': f_melamine, 'rust': f_rust,
          'particle': f_particle, 'kick': f_kick, 'galv': f_galv, 'nickel': f_nickel, 'black_metal': f_black_metal, 'cast': f_cast,
          'mat': f_mat, 'foam': f_foam, 'pearl': f_pearl}
NO_DUST = {'gold', 'gem', 'pearl', 'screen', 'vct', 'acoustic', 'wallpaint', 'terrazzo', 'mat', 'chrome'}


def f_brickpaint(c):
    """Brick painted over, many times (the shop's walls): running bond under a cream latex, the joints still showing as
    shallow grooves; the paint chipped to the red brick here and there, more of it in the lower 1.2 m; scuffs and grime
    rising from the floor, hand marks."""
    m = c.m
    u, v = face_uv(m, c)
    course, row = 0.213, 0.0667
    rr = m.math('FLOOR', m.math('DIVIDE', v, row))
    off = mul(m, m.math('FRACT', mul(m, rr, 0.5)), 1.0)
    bu = m.math('FRACT', add(m, m.math('DIVIDE', u, course), off))
    bv = m.math('FRACT', m.math('DIVIDE', v, row))
    joint = m.math('MAXIMUM', ramp(m, m.math('ABSOLUTE', sub(m, bu, 0.5)), 0.465, 0.48), ramp(m, m.math('ABSOLUTE', sub(m, bv, 0.5)), 0.41, 0.45))
    cell = vfloor(m, vec(m, add(m, m.math('DIVIDE', u, course), off), rr, mul(m, c.nx, 7.0)))
    tone = white(m, cell)
    brick = m.mix(tone, H(0x6a3a2c), H(0x7e4636))
    brick = m.mix(joint, brick, H(0x8d877c))
    # the coats under the cream: a white and an old green, ghosting through where the brick shows
    ghost = ramp(m, noise(m, c.P, scale=14.0, detail=3.0), 0.5, 0.62)
    brick = m.mix(mul(m, ghost, 0.45), brick, H(0xb8b4a2))
    loss_n = noise(m, c.P, scale=3.5, detail=5.0, roughness=0.65)
    low = ramp(m, c.z, 1.25, 0.2)
    loss = ramp(m, add(m, loss_n, mul(m, low, 0.13)), 0.675, 0.705)
    roll = noise(m, scaled(m, c.sep, 25.0, 25.0, 4.0), detail=3.0)
    paint = m.mix(mul(m, roll, 0.12), c.col, darken(m, c.col, 0.9))
    paint = m.mix(mul(m, joint, 0.25), paint, darken(m, c.col, 0.82))
    col = m.mix(loss, paint, brick)
    edge = mul(m, ramp(m, add(m, loss_n, mul(m, low, 0.13)), 0.655, 0.675), m.math('SUBTRACT', 1.0, loss))
    col = m.mix(mul(m, edge, 0.5), col, lighten(m, c.col, 0.12))
    scuff = mul(m, mul(m, ramp(m, c.z, 0.0, 0.08), ramp(m, c.z, 1.1, 0.5)), ramp(m, noise(m, scaled(m, c.sep, 10.0, 10.0, 60.0), detail=4.0), 0.6, 0.72))
    col = m.mix(mul(m, scuff, 0.6), col, H(0x5a554c))
    base = mul(m, ramp(m, c.z, 0.3, 0.0), ramp(m, noise(m, c.P, scale=6.0, detail=3.0), 0.3, 0.7))
    col = m.mix(mul(m, base, 0.45), col, H(0x5a5244))
    hands = mul(m, mul(m, ramp(m, c.z, 0.9, 1.2), ramp(m, c.z, 1.7, 1.4)), ramp(m, noise(m, c.P, scale=3.0, detail=4.0), 0.55, 0.75))
    col = m.mix(mul(m, hands, 0.2), col, H(0x8a7f68))
    rough = add(m, mix_float(m, loss, 0.6, 0.85), mul(m, joint, 0.1), mul(m, roll, 0.05))
    h = add(m, mul(m, joint, -0.25), mul(m, loss, -0.12), mul(m, roll, 0.004), mul(m, tone, 0.01))
    return col, rough, 0.0, h


def f_tin(c):
    """Pressed tin, 2 ft squares, painted cream over the years (the ceiling): a beaded border, a medallion of rings and a
    rosette, diamonds between, egg-and-dart on the fields; the paint yellowed, darker in the recesses, flaking to rusty
    tin here and there; the stains from the kit's 'ceil' image."""
    m = c.m
    t = 0.6096
    fu = sub(m, m.math('FRACT', m.math('DIVIDE', c.x, t)), 0.5)
    fv = sub(m, m.math('FRACT', m.math('DIVIDE', c.y, t)), 0.5)
    au, av = m.math('ABSOLUTE', fu), m.math('ABSOLUTE', fv)
    edge = m.math('MAXIMUM', au, av)
    r = m.math('SQRT', add(m, mul(m, fu, fu), mul(m, fv, fv)))
    bead = mul(m, ramp(m, edge, 0.44, 0.46), ramp(m, edge, 0.495, 0.475))
    rim = ramp(m, edge, 0.485, 0.5)
    ring1 = ramp(m, m.math('ABSOLUTE', sub(m, r, 0.13)), 0.02, 0.0)
    ring2 = ramp(m, m.math('ABSOLUTE', sub(m, r, 0.20)), 0.012, 0.0)
    boss = ramp(m, r, 0.07, 0.02)
    ang = m.math('ARCTAN2', fv, fu)
    petals = mul(m, ramp(m, m.math('COSINE', mul(m, ang, 8.0)), 0.3, 0.9), mul(m, ramp(m, r, 0.11, 0.06), ramp(m, r, 0.02, 0.05)))
    dia = m.math('ADD', au, av)
    diamond = mul(m, ramp(m, m.math('ABSOLUTE', sub(m, dia, 0.36)), 0.015, 0.0), ramp(m, r, 0.21, 0.24))
    dots = mul(m, ramp(m, m.math('ABSOLUTE', sub(m, m.math('FRACT', mul(m, add(m, fu, 0.5), 12.0)), 0.5)), 0.25, 0.1),
               mul(m, ramp(m, m.math('ABSOLUTE', sub(m, m.math('FRACT', mul(m, add(m, fv, 0.5), 12.0)), 0.5)), 0.25, 0.1), ramp(m, edge, 0.36, 0.40)))
    dots = mul(m, dots, ramp(m, edge, 0.44, 0.42))
    height = add(m, mul(m, bead, 1.0), mul(m, rim, -0.6), mul(m, ring1, 0.8), mul(m, ring2, 0.6), mul(m, boss, 1.0), mul(m, petals, 0.5),
                 mul(m, diamond, 0.6), mul(m, dots, 0.35))
    recess = ramp(m, height, 0.25, 0.0)
    yel = ramp(m, noise(m, c.P, scale=0.7, detail=3.0), 0.3, 0.8)
    col = m.mix(mul(m, yel, 0.3), c.col, H(0xcdbd94))
    col = m.mix(mul(m, recess, 0.22), col, darken(m, col, 0.72))
    col = m.mix(mul(m, ramp(m, height, 0.6, 1.0), 0.15), col, lighten(m, c.col, 0.2))
    flake = ramp(m, noise(m, c.P, scale=14.0, detail=5.0), 0.72, 0.75)
    col = m.mix(flake, col, m.mix(ramp(m, noise(m, c.P, scale=90.0), 0.4, 0.6), H(0x6a5a48), H(0x7a4a2a)))
    stain = c.kit_stain
    ring_ = ramp(m, m.math('ABSOLUTE', sub(m, stain, 0.5)), 0.1, 0.0)
    col = m.mix(mul(m, stain, 0.5), col, H(0xa88a52))
    col = m.mix(mul(m, ring_, 0.55), col, H(0x6a4a24))
    rough = add(m, 0.45, mul(m, recess, 0.15), mul(m, flake, 0.3), mul(m, stain, 0.1))
    return col, rough, mul(m, flake, 0.3), add(m, mul(m, height, 0.0015), mul(m, flake, -0.0006))


def f_diamond(c):
    """Diamond plate (the cage's base, the safe's surround): raised lugs on a 30 mm pitch at alternating angles, worn bright
    on top, dirt in the field."""
    m = c.m
    u, v = face_uv(m, c)
    p = 0.03
    a = add(m, u, v)
    b = sub(m, u, v)
    ga = m.math('FRACT', m.math('DIVIDE', a, p * 1.414))
    gb = m.math('FRACT', m.math('DIVIDE', b, p * 1.414))
    ia = m.math('FLOOR', m.math('DIVIDE', a, p * 1.414))
    ib = m.math('FLOOR', m.math('DIVIDE', b, p * 1.414))
    alt = m.math('FRACT', mul(m, add(m, ia, ib), 0.5))
    da = m.math('ABSOLUTE', sub(m, ga, 0.5))
    db = m.math('ABSOLUTE', sub(m, gb, 0.5))
    lug1 = mul(m, ramp(m, da, 0.36, 0.30), ramp(m, db, 0.10, 0.05))
    lug2 = mul(m, ramp(m, db, 0.36, 0.30), ramp(m, da, 0.10, 0.05))
    lug = mix_float(m, m.math('GREATER_THAN', alt, 0.25), lug1, lug2)
    dirt = ramp(m, noise(m, c.P, scale=8.0, detail=4.0), 0.35, 0.75)
    col = m.mix(mul(m, dirt, 0.5), H(0x7a7e84), H(0x4a4c4e))
    col = m.mix(lug, col, H(0xb8bcc0))
    rough = mix_float(m, lug, add(m, 0.45, mul(m, dirt, 0.25)), 0.25)
    return col, rough, mix_float(m, dirt, 1.0, 0.6), mul(m, lug, 0.0025)


for _n, _f in (('brickpaint', f_brickpaint), ('tin', f_tin), ('diamond', f_diamond)):
    FIN[_n] = len(FIN) + 1
    FINISH[_n] = _f
NO_DUST |= {'brickpaint', 'tin'}


def body_material(kit, slot):
    m = core.Mat(f'{kit.prefix}_{slot}')
    c = Ctx(m, kit, slot)
    # Control images (planar in object XY) some finishes read: the floor's traffic / wax / spills, the ceiling's stains.
    c.kit_floor = (0.0, 0.0, 0.0)
    c.kit_stain = 0.0
    if 'floor' in kit.images and 'vct' in kit.kinds[slot]:
        path, (x0, y0, x1, y1) = kit.images['floor']
        uvv = vec(m, m.math('DIVIDE', sub(m, c.x, x0), x1 - x0), m.math('DIVIDE', sub(m, c.y, y0), y1 - y0), 0.0)
        img = m.image(path, non_color=True, vector=uvv, extension='EXTEND')
        sc = m.node('ShaderNodeSeparateColor')
        m.link(img.outputs['Color'], sc.inputs['Color'])
        c.kit_floor = (sc.outputs['Red'], sc.outputs['Green'], sc.outputs['Blue'])
    if 'ceil' in kit.images and ('acoustic' in kit.kinds[slot] or 'tin' in kit.kinds[slot]):
        path, (x0, y0, x1, y1) = kit.images['ceil']
        uvv = vec(m, m.math('DIVIDE', sub(m, c.x, x0), x1 - x0), m.math('DIVIDE', sub(m, c.y, y0), y1 - y0), 0.0)
        img = m.image(path, non_color=True, vector=uvv, extension='EXTEND')
        sc = m.node('ShaderNodeSeparateColor')
        m.link(img.outputs['Color'], sc.inputs['Color'])
        c.kit_stain = sc.outputs['Red']
    col, rough, metal, h = H(0x808080), 0.5, 0.0, 0.0
    for kind in sorted(kit.kinds[slot], key=FIN.get):
        fc, fr, fm, fh = FINISH[kind](c)
        mk = is_k(m, c.fin, FIN[kind])
        col = m.mix(mk, col, fc)
        rough = mix_float(m, mk, rough, fr)
        metal = mix_float(m, mk, metal, fm)
        h = mix_float(m, mk, h, fh)
    # Wear every finish shares: grime in the crevices, rising from the floor, hand grime, polish, dust on top.
    if kit.cavity[slot] > 0.0:
        ao = m.node('ShaderNodeAmbientOcclusion', Distance=0.012)
        ao.only_local = True
        ao.samples = 8
        cav = ramp(m, ao.outputs['AO'], 0.85, 0.4)
        col = m.mix(mul(m, cav, kit.cavity[slot]), col, H(0x2e2a24))
        rough = add(m, rough, mul(m, cav, 0.08))
    if kit.low[slot] > 0.0:
        breakup = ramp(m, noise(m, c.P, scale=7.0, detail=4.0), 0.3, 0.7)
        low = mul(m, ramp(m, c.z, kit.low[slot], 0.0), breakup)
        col = m.mix(mul(m, low, 0.5), col, H(0x332e27))
        rough = mix_float(m, mul(m, low, 0.6), rough, 0.85)
        metal = mix_float(m, mul(m, low, 0.5), metal, 0.0)
    for (cx, cy, cz), rad, amt in kit.grime:
        dx, dy, dz = sub(m, c.x, cx), sub(m, c.y, cy), sub(m, c.z, cz)
        d = m.math('SQRT', add(m, mul(m, dx, dx), mul(m, dy, dy), mul(m, dz, dz)))
        g = mul(m, mul(m, smooth_less(m, d, rad * 0.6, rad * 0.4), ramp(m, noise(m, c.P, scale=35.0, detail=3.0), 0.3, 0.7)), amt)
        col = m.mix(mul(m, g, 0.7), col, H(0x332d24))
        rough = mix_float(m, g, rough, 0.62)
    for (cx, cy, cz), rad, amt in kit.polish:
        dx, dy, dz = sub(m, c.x, cx), sub(m, c.y, cy), sub(m, c.z, cz)
        d = m.math('SQRT', add(m, mul(m, dx, dx), mul(m, dy, dy), mul(m, dz, dz)))
        g = mul(m, mul(m, smooth_less(m, d, rad * 0.55, rad * 0.45), ramp(m, noise(m, c.P, scale=18.0, detail=3.0), 0.25, 0.65)), amt)
        col = m.mix(mul(m, g, 0.1), col, darken(m, col, 0.85))
        rough = mix_float(m, g, rough, 0.22)
    if kit.dust[slot] > 0.0:
        keep = 1.0
        for k in kit.kinds[slot]:
            if k in NO_DUST:
                keep = mul(m, keep, m.math('SUBTRACT', 1.0, is_k(m, c.fin, FIN[k])))
        metal_less = m.math('SUBTRACT', 1.0, mul(m, metal, 0.5)) if not isinstance(metal, (int, float)) else 1.0 - 0.5 * metal
        up = mul(m, ramp(m, c.nz, 0.6, 0.95), mul(m, add(m, 0.45, mul(m, ramp(m, noise(m, c.P, scale=35.0, detail=5.0), 0.3, 0.85), 0.55)), kit.dust[slot]))
        up = mul(m, mul(m, up, metal_less), keep)
        col = m.mix(up, col, H(0x9a958b))
        rough = mix_float(m, up, rough, 0.9)
        metal = mix_float(m, up, metal, 0.0)
    m.set('Base Color', col)
    m.set('Roughness', rough)
    m.set('Metallic', metal)
    bump = m.node('ShaderNodeBump', Strength=1.0, Distance=0.001)
    m.link(h if not isinstance(h, (int, float)) else m.math('ADD', h, 0.0), bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def print_material(kit):
    """The atlas through the decals' UVs: its color and finish, a film of grime; lit panels glow."""
    A = kit.atlas
    m = core.Mat(f'{kit.prefix}_print')
    uvn = m.node('ShaderNodeUVMap')
    uvn.uv_map = 'UVMap'
    img = m.image(A.color, vector=uvn.outputs['UV'], extension='EXTEND')
    surf = m.image(A.surface, non_color=True, vector=uvn.outputs['UV'], extension='EXTEND')
    ss = m.node('ShaderNodeSeparateColor')
    m.link(surf.outputs['Color'], ss.inputs['Color'])
    tc, sep = object_coords(m)
    grime = ramp(m, noise(m, tc.outputs['Object'], scale=25.0, detail=4.0), 0.55, 0.85)
    col = m.mix(mul(m, grime, 0.14), img.outputs['Color'], H(0x3a342c))
    m.set('Base Color', col)
    m.set('Roughness', add(m, ss.outputs['Green'], mul(m, grime, 0.08)))
    m.set('Metallic', ss.outputs['Red'])
    lit = [(k, p['glow']) for k, p in A.panels.items() if p['glow'] > 0.0]
    if lit:
        gmax = max(g for _, g in lit)
        us = m.node('ShaderNodeSeparateXYZ')
        m.link(uvn.outputs['UV'], us.inputs['Vector'])
        u, v = us.outputs['X'], us.outputs['Y']
        mask = None
        for k, g in lit:
            u0, v0, u1, v1 = A.rects[k]
            r = mul(m, mul(m, m.math('GREATER_THAN', u, u0), m.math('LESS_THAN', u, u1)), mul(m, m.math('GREATER_THAN', v, v0), m.math('LESS_THAN', v, v1)))
            r = mul(m, r, g / gmax)
            mask = r if mask is None else m.math('MAXIMUM', mask, r)
        m.set('Emission Color', m.mix(1.0, img.outputs['Color'], vec(m, mask, mask, mask), blend='MULTIPLY'))
        m.set('Emission Strength', gmax)
    return m


# ================================================================== print panels: shared pieces


def photocopy(s, w, h, seed, amount=1.0):
    """Toner speckle and a grey copier edge."""
    rnd = random.Random(seed)
    for _ in range(int(40 * amount)):
        s.rect(rnd.uniform(0, w), rnd.uniform(0, h), rnd.uniform(0.3, 1.2), rnd.uniform(0.3, 1.2), 0x6a6a6a, rough=0.8, alpha=rnd.uniform(0.2, 0.6))
    s.rect(0, h - 2.0, w, 2.0, 0xa8a49a, rough=0.8, alpha=0.4 * amount)


# The pawn tickets: numbered manila tags and the shop's two-part loan slip. A ticket's number is "No. " + 6 digits.
def ticket_no(seed):
    return f'{40000 + (seed * 7919) % 9000:06d}'


def p_tag(seed, desc=None, price=None, ground=0xe9d7a6, string_hole=True):
    """A manila string tag: the shop's stamp, the ticket number, a handwritten description and (for sale) a price."""
    def draw(s, w, h):
        rnd = random.Random(seed)
        cut = w * 0.22
        s.poly([(0, 0), (w, 0), (w, h - cut), (w - cut, h), (cut, h), (0, h - cut)], ground, rough=0.8)
        if string_hole:
            s.circle(w / 2, h - cut * 0.85, w * 0.07, 0x9a8a5a, rough=0.8)
            s.circle(w / 2, h - cut * 0.85, w * 0.045, 0x1a1a1a, rough=0.8)
        s.mid('NORTHSIDE PAWN', w / 2, h - cut - h * 0.06, h * 0.05, w * 0.86, 0x2a3a6a, 'Black')
        s.mid('No. ' + ticket_no(seed), w / 2, h - cut - h * 0.14, h * 0.075, w * 0.9, 0xa3161c, 'Black')
        s.rect(w * 0.08, h - cut - h * 0.19, w * 0.84, h * 0.006, 0x8a7a50)
        ink = 0x1b2f7a if rnd.random() < 0.6 else 0x161616
        if desc:
            size = min(h * 0.11, w * 0.8 / max(hand_w(desc, 1.0), 1e-3))
            hand(s, desc, w * 0.1, h * 0.38, size, ink, width=0.08, seed=seed)
        if price:
            size = min(h * 0.16, w * 0.75 / max(hand_w(price, 1.0), 1e-3))
            hand(s, price, w * 0.14, h * 0.1, size, 0xa3161c, width=0.1, seed=seed + 3)
    return draw


# ================================================================== SM_NS_Pawn_Safe

GOLD_LEAF = 0xd2a548


def pinstripes(s, w, h, g=GOLD_LEAF, insets=((6, 1.6), (11, 0.7)), dots=True):
    for inset, wd in insets:
        s.rect(inset, inset, w - 2 * inset, wd, g, metal=1.0, rough=0.3)
        s.rect(inset, h - inset - wd, w - 2 * inset, wd, g, metal=1.0, rough=0.3)
        s.rect(inset, inset, wd, h - 2 * inset, g, metal=1.0, rough=0.3)
        s.rect(w - inset - wd, inset, wd, h - 2 * inset, g, metal=1.0, rough=0.3)
    if dots:
        inset = insets[-1][0]
        for cx in (inset + 4, w - inset - 4):
            for cy in (inset + 4, h - inset - 4):
                s.circle(cx, cy, 2.2, g, metal=1.0, rough=0.3, n=20)


def draw_safe_top(s, w, h):
    """The door's upper panel: gold-leaf lettering and pinstripes on the green-black enamel."""
    g = GOLD_LEAF
    s.rect(0, 0, w, h, 0x18211b, rough=0.32)
    pinstripes(s, w, h)
    s.mid('NORTHSIDE', w / 2, h * 0.70, h * 0.2, w * 0.7, g, 'Black', tracking=1.2, metal=1.0, rough=0.3)
    s.mid('PAWN & LOAN', w / 2, h * 0.43, h * 0.17, w * 0.8, g, 'Black', tracking=1.15, metal=1.0, rough=0.3)
    s.mid('EST. 1961', w / 2, h * 0.21, h * 0.08, w * 0.4, 0xb08a3a, 'Bold', tracking=1.6, metal=1.0, rough=0.35)


def draw_safe_low(s, w, h):
    """The lower panel: a painted vignette in a gold-leaf oval (a river, hills and a mill at dusk), the pinstripes."""
    g = GOLD_LEAF
    s.rect(0, 0, w, h, 0x18211b, rough=0.32)
    pinstripes(s, w, h, insets=((4, 1.2), (8, 0.6)))
    cx, cy, rx, ry = w / 2, h * 0.5, w * 0.40, h * 0.36
    s.poly([(cx + rx * 1.04 * math.cos(2 * math.pi * k / 128), cy + (ry + rx * 0.04) * math.sin(2 * math.pi * k / 128)) for k in range(128)], g,
           metal=1.0, rough=0.3)

    def lerp(a, b, t):
        ca = [(a >> 16) & 255, (a >> 8) & 255, a & 255]
        cb = [(b >> 16) & 255, (b >> 8) & 255, b & 255]
        c = [int(ca[i] + (cb[i] - ca[i]) * max(0.0, min(1.0, t))) for i in range(3)]
        return (c[0] << 16) | (c[1] << 8) | c[2]

    def far(u):
        return 0.02 + 0.16 * math.sin(2.1 * u + 0.6) ** 2 + 0.05 * math.sin(7.0 * u)

    def near(u):
        return -0.22 + 0.34 * math.exp(-((u - 0.30) / 0.42) ** 2) + 0.04 * math.sin(5.0 * u)

    def river_top(u):
        return -0.50 + 0.06 * u

    def river_bot(u):
        return -0.70 + 0.04 * u

    def layer(u, v):
        if v < river_bot(u):
            return 'bank'
        if v < river_top(u):
            return 'river'
        if v < near(u):
            return 'near'
        if v < far(u):
            return 'far'
        return 'sky'

    def color(name, u, v):
        if name == 'sky':
            return lerp(0xd8a868, 0x33445e, (v - 0.0) / 1.0)
        if name == 'far':
            return lerp(0x6a7686, 0x56627a, (v + 0.2) / 0.5)
        if name == 'near':
            return lerp(0x2a3a26, 0x3e5230, (v + 0.4) / 0.6)
        if name == 'river':
            return lerp(0x8e9aa8, 0x5c6c80, abs(u * 0.6 + 0.2 * math.sin(v * 40)))
        return 0x222c1e

    rows = 70
    for j in range(rows):
        v0 = -1.0 + 2.0 * j / rows
        v = v0 + 1.0 / rows
        half = math.sqrt(max(0.0, 1.0 - v * v))
        if half <= 0:
            continue
        segs = 64
        start = -half
        cur = layer(start, v)
        for i in range(1, segs + 1):
            u = -half + 2 * half * i / segs
            nl = layer(u, v) if i < segs else None
            if nl != cur:
                um = (start + u) / 2
                s.rect(cx + start * rx, cy + v0 * ry, (u - start) * rx + 0.3, 2.0 * ry / rows + 0.3, color(cur, um, v), rough=0.5)
                start, cur = u, nl
    # the sun going down behind the far hills, the mill on the near hill with a lit window, trees, the far bridge
    s.circle(cx - rx * 0.42, cy + ry * 0.12, ry * 0.12, 0xf0d090, rough=0.5)
    s.poly([(cx - rx * 0.62, cy + ry * far(-0.5)), (cx - rx * 0.25, cy + ry * far(-0.5)), (cx - rx * 0.25, cy - ry * 0.05),
            (cx - rx * 0.62, cy - ry * 0.05)], lerp(0x6a7686, 0x56627a, 0.6), rough=0.5)
    mx, mz = cx + rx * 0.28, cy + ry * (near(0.30) - 0.05)
    s.rect(mx, mz, rx * 0.14, ry * 0.30, 0x6a4a34, rough=0.5)
    s.poly([(mx - rx * 0.02, mz + ry * 0.30), (mx + rx * 0.07, mz + ry * 0.48), (mx + rx * 0.16, mz + ry * 0.30)], 0x3a2418, rough=0.5)
    s.rect(mx + rx * 0.05, mz + ry * 0.10, rx * 0.03, ry * 0.08, 0xf0c070, rough=0.5)
    s.circle(mx - rx * 0.05, mz + ry * 0.08, ry * 0.16, 0x4a3a28, rough=0.5)
    for tx, tr in ((-0.75, 0.14), (-0.66, 0.11), (0.62, 0.16), (0.72, 0.12), (0.08, 0.09)):
        ty = near(tx) if tx > -0.5 else far(tx) - 0.1
        s.circle(cx + rx * tx, cy + ry * (ty + tr * 0.6), ry * tr, 0x1e2a1a, rough=0.5)
    for i in range(14):
        u = -0.7 + i * 0.1
        s.rect(cx + rx * u, cy + ry * (river_top(u) - 0.06 - 0.05 * (i % 3)), rx * 0.06, 0.35, 0xd8c8a0, rough=0.4, alpha=0.6)
    rnd = random.Random(5)
    for _ in range(60):
        a = rnd.uniform(0, 2 * math.pi)
        rr = rnd.uniform(0, 0.95)
        x0, y0 = cx + rx * rr * math.cos(a), cy + ry * rr * math.sin(a)
        s.rect(x0, y0, rnd.uniform(2, 7), 0.2, 0x1a1a14, rough=0.5, rot=rnd.uniform(0, 3.14), alpha=0.3)


def draw_dial(s, w, h):
    """The combination dial's face: 0..99 round the rim, a tick per number, longer every 5 (read at the fixed ring's
    index mark at the top)."""
    cx, cy = w / 2, h / 2
    r = w / 2
    s.circle(cx, cy, r, 0x1a1a1a, rough=0.35, n=128)
    s.circle(cx, cy, r * 0.985, 0xb9b6ad, metal=1.0, rough=0.22, n=128)
    for k in range(100):
        a = math.pi / 2 - 2 * math.pi * k / 100
        long_ = k % 5 == 0
        r0 = r * (0.80 if long_ else 0.87)
        hw = 0.3 if long_ else 0.2
        ca, sa = math.cos(a), math.sin(a)
        s.poly([(cx + r0 * ca - hw * sa, cy + r0 * sa + hw * ca), (cx + r * 0.95 * ca - hw * sa, cy + r * 0.95 * sa + hw * ca),
                (cx + r * 0.95 * ca + hw * sa, cy + r * 0.95 * sa - hw * ca), (cx + r0 * ca + hw * sa, cy + r0 * sa - hw * ca)], 0x151515, rough=0.4)
        if k % 10 == 0:
            rt = r * 0.66
            s.text(str(k), cx + rt * ca + 0.05 * r * sa, cy + rt * sa - 0.05 * r * ca, r * 0.13, 0x151515, 'Bold', align='CENTER', rot=a - math.pi / 2)
    s.circle(cx, cy, r * 0.42, 0x9a978e, metal=1.0, rough=0.3, n=96)


def draw_safe_plate(s, w, h):
    s.rect(0, 0, w, h, 0x9a7a3a, metal=1.0, rough=0.35)
    s.rect(1.2, 1.2, w - 2.4, h - 2.4, 0xb8944a, metal=1.0, rough=0.3)
    s.mid('NORTHSIDE SAFE & LOCK CO.', w / 2, h * 0.66, h * 0.22, w - 10, 0x2a2010, 'Black', metal=1.0, rough=0.4)
    s.mid('FIRE & BURGLAR PROOF', w / 2, h * 0.32, h * 0.16, w - 14, 0x2a2010, 'Bold', tracking=1.3, metal=1.0, rough=0.4)


def p_spine(title, col, ink=0xd8c890, year=None):
    """A ledger's spine, lettered up the spine (bottom to top)."""
    def draw(s, w, h):
        s.rect(0, 0, w, h, col, rough=0.7)
        for yy in (0.1, 0.86):
            s.rect(0, h * yy, w, h * 0.02, ink, rough=0.5, alpha=0.7)
        s.text(title, w * 0.5 + w * 0.17, h * 0.5, w * 0.34, ink, 'Black', align='CENTER', rot=math.pi / 2)
        if year:
            s.mid(year, w / 2, h * 0.05, w * 0.22, w * 0.9, ink, 'Bold')
    return draw


# ================================================================== the goods: an item library (each built in its own frame)
#
# Every item: front toward -y, origin on the floor (or the surface it stands on) under its middle, unless it says. They
# add their parts to a Kit (slot given), so the window, the shelves and the walls draw from one set of makers.

def closed_cr(pts, steps=6):
    """A closed Catmull-Rom curve through 2D points."""
    P = [Vector((p[0], p[1], 0.0)) for p in pts]
    n = len(P)
    out = []
    for i in range(n):
        p0, p1, p2, p3 = P[(i - 1) % n], P[i], P[(i + 1) % n], P[(i + 2) % n]
        for k in range(steps):
            t = k / steps
            q = 0.5 * ((2 * p1) + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t + (-p0 + 3 * p1 - 3 * p2 + p3) * t * t * t)
            out.append((q.x, q.y))
    return out


def ccw2(pts):
    a = sum(x0 * y1 - x1 * y0 for (x0, y0), (x1, y1) in zip(pts, pts[1:] + pts[:1]))
    return pts if a > 0 else list(reversed(pts))


def xz_slab(name, outline, y0, y1, r=0.0, seg=2, matrix=None):
    """A flat shape drawn in the XZ plane ((x, z) points), extruded along y from y0 (front) to y1 (back), its rims rounded
    by r, then moved by matrix."""
    M = frame_matrix((1, 0, 0), (0, 0, 1), (0.0, 0.0, 0.0))   # local z -> -y
    o = parts.panel(name, ccw2(list(outline)), -y1, -y0, r, seg, M)
    if matrix is not None:
        o.data.transform(matrix)
    return o


def yz_slab(name, outline, x0, x1, r=0.0, seg=2, matrix=None):
    """A flat shape drawn in the YZ plane ((y, z) points, y to the right as seen from -x), extruded along x."""
    M = frame_matrix((0, -1, 0), (0, 0, 1), (0.0, 0.0, 0.0))  # local z -> -x
    o = parts.panel(name, ccw2([(-a, b) for a, b in outline]), -x1, -x0, r, seg, M)
    if matrix is not None:
        o.data.transform(matrix)
    return o


def xy_slab(name, outline, z0, z1, r=0.0, seg=2, matrix=None):
    o = parts.panel(name, ccw2(list(outline)), z0, z1, r, seg)
    if matrix is not None:
        o.data.transform(matrix)
    return o


def torus(name, center, R, r, axis='y', nu=20, nv=8):
    """A ring (torus) of major radius R and tube radius r around the given axis through center."""
    bm = bmesh.new()
    verts = []
    for i in range(nu):
        a = 2 * math.pi * i / nu
        ring = []
        for j in range(nv):
            b = 2 * math.pi * j / nv
            rr = R + r * math.cos(b)
            p = Vector((rr * math.cos(a), rr * math.sin(a), r * math.sin(b)))
            ring.append(bm.verts.new(p))
        verts.append(ring)
    for i in range(nu):
        for j in range(nv):
            a, b = verts[i][j], verts[(i + 1) % nu][j]
            c, d = verts[(i + 1) % nu][(j + 1) % nv], verts[i][(j + 1) % nv]
            bm.faces.new((a, b, c, d))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    o = core.mesh_object(name, bm)
    rot = {'z': Matrix(), 'y': RX(90), 'x': RY(90)}[axis]
    o.data.transform(T(*center) @ rot)
    return o


def chain_links(k, pts, link=0.008, wire=0.0022, slot=None, col=0xf0c35a, flat=0.6, aux=(0.08, 0, 0)):
    """A curb chain along a 3D path: links alternately turned 90 degrees, each a squashed torus."""
    path = [Vector(p) for p in core.catmull_rom(pts, 12)]
    lengths = [0.0]
    for a, b in zip(path, path[1:]):
        lengths.append(lengths[-1] + (b - a).length)
    total = lengths[-1]
    step = link * 1.25
    n = max(int(total / step), 2)
    j = 0
    objs = []
    for i in range(n):
        s = i * step + step / 2
        while j < len(lengths) - 2 and lengths[j + 1] < s:
            j += 1
        t = (s - lengths[j]) / max(lengths[j + 1] - lengths[j], 1e-9)
        p = path[j].lerp(path[j + 1], t)
        d = (path[j + 1] - path[j]).normalized()
        up = Vector((0, 0, 1)) if abs(d.z) < 0.9 else Vector((1, 0, 0))
        side = d.cross(up).normalized()
        nrm = side.cross(d).normalized()
        if i % 2:
            side, nrm = nrm, -side
        M = Matrix((d, side, nrm)).transposed().to_4x4()
        M.translation = p
        o = torus('link', (0, 0, 0), link * 0.5, wire, axis='z', nu=8, nv=4)
        o.data.transform(Matrix.Diagonal((1.0, flat * 1.0 + 0.0, 1.0, 1.0)))
        o.data.transform(M)
        objs.append(o)
    joined = core.join(objs, 'chain') if len(objs) > 1 else objs[0]
    k.add(joined, 'gold', col, slot=slot, aux=aux)
    return joined


# ------------------------------------------------------------------ guitars

GUITAR_SHAPES = {
    # (x, z) control points of the body (bottom at z 0), closed; neck pocket at the top middle.
    'double': [(0.0, 0.0), (0.12, 0.012), (0.163, 0.09), (0.158, 0.17), (0.118, 0.245), (0.135, 0.31), (0.142, 0.365), (0.122, 0.418),
               (0.088, 0.40), (0.062, 0.352), (0.03, 0.352), (-0.03, 0.352), (-0.06, 0.38), (-0.088, 0.448), (-0.118, 0.452),
               (-0.142, 0.39), (-0.13, 0.30), (-0.122, 0.245), (-0.166, 0.15), (-0.155, 0.06), (-0.11, 0.008)],
    'single': [(0.0, 0.0), (0.13, 0.018), (0.168, 0.10), (0.155, 0.19), (0.125, 0.25), (0.13, 0.31), (0.115, 0.375), (0.088, 0.415),
               (0.062, 0.40), (0.05, 0.365), (0.03, 0.37), (-0.03, 0.395), (-0.07, 0.42), (-0.11, 0.395), (-0.13, 0.33), (-0.126, 0.26),
               (-0.16, 0.18), (-0.168, 0.10), (-0.13, 0.018)],
    'tele': [(0.0, 0.0), (0.13, 0.006), (0.162, 0.07), (0.162, 0.18), (0.145, 0.26), (0.14, 0.32), (0.112, 0.37), (0.09, 0.355), (0.06, 0.33),
             (0.03, 0.33), (-0.03, 0.36), (-0.07, 0.385), (-0.115, 0.39), (-0.14, 0.33), (-0.145, 0.25), (-0.162, 0.17), (-0.162, 0.07),
             (-0.13, 0.006)],
    'offset': [(0.02, 0.0), (0.13, 0.02), (0.17, 0.11), (0.15, 0.20), (0.12, 0.26), (0.142, 0.33), (0.14, 0.40), (0.105, 0.432),
               (0.075, 0.395), (0.03, 0.37), (-0.03, 0.37), (-0.07, 0.39), (-0.105, 0.445), (-0.14, 0.43), (-0.148, 0.34), (-0.128, 0.27),
               (-0.17, 0.16), (-0.16, 0.06), (-0.09, 0.0)],
    'horns': [(0.0, 0.0), (0.12, 0.01), (0.158, 0.09), (0.145, 0.18), (0.118, 0.235), (0.128, 0.30), (0.118, 0.37), (0.095, 0.425),
              (0.07, 0.40), (0.04, 0.33), (0.026, 0.33), (-0.026, 0.33), (-0.04, 0.33), (-0.07, 0.40), (-0.095, 0.425), (-0.118, 0.37),
              (-0.128, 0.30), (-0.118, 0.235), (-0.145, 0.18), (-0.158, 0.09), (-0.12, 0.01)],
    'hollow': [(0.0, 0.0), (0.15, 0.015), (0.198, 0.10), (0.19, 0.20), (0.152, 0.27), (0.16, 0.34), (0.15, 0.41), (0.118, 0.47),
               (0.088, 0.45), (0.06, 0.39), (0.03, 0.39), (-0.03, 0.39), (-0.06, 0.39), (-0.088, 0.45), (-0.118, 0.47), (-0.15, 0.41),
               (-0.16, 0.34), (-0.152, 0.27), (-0.19, 0.20), (-0.198, 0.10), (-0.15, 0.015)],
    'acoustic': [(0.0, 0.0), (0.15, 0.015), (0.198, 0.09), (0.19, 0.19), (0.142, 0.27), (0.142, 0.30), (0.15, 0.37), (0.13, 0.455),
                 (0.07, 0.495), (0.03, 0.50), (-0.03, 0.50), (-0.07, 0.495), (-0.13, 0.455), (-0.15, 0.37), (-0.142, 0.30), (-0.142, 0.27),
                 (-0.19, 0.19), (-0.198, 0.09), (-0.15, 0.015)],
    'bass': [(0.0, 0.0), (0.13, 0.012), (0.178, 0.10), (0.17, 0.19), (0.128, 0.265), (0.15, 0.33), (0.16, 0.40), (0.135, 0.455),
             (0.095, 0.43), (0.065, 0.37), (0.032, 0.37), (-0.032, 0.37), (-0.065, 0.40), (-0.10, 0.49), (-0.135, 0.50), (-0.158, 0.42),
             (-0.14, 0.31), (-0.132, 0.26), (-0.18, 0.15), (-0.168, 0.06), (-0.12, 0.008)],
}
GUITAR_TOP = {k: max(z for _, z in v if abs(_) < 0.035) for k, v in GUITAR_SHAPES.items()}


def edge_band(name, outline, s, y0, y1):
    """A strip standing round an (x, z) outline (scaled by s about the origin) from y0 to y1, facing out."""
    pts = [(x * s, z * s - 0.001) for x, z in outline]
    if (Vector((pts[0][0], pts[0][1])) - Vector((pts[-1][0], pts[-1][1]))).length < 1e-6:
        pts = pts[:-1]
    n = len(pts)
    area = sum(pts[i][0] * pts[(i + 1) % n][1] - pts[(i + 1) % n][0] * pts[i][1] for i in range(n))
    bm = bmesh.new()
    a = [bm.verts.new((x, y0, z)) for x, z in pts]
    b = [bm.verts.new((x, y1, z)) for x, z in pts]
    for i in range(n):
        j = (i + 1) % n
        bm.faces.new((a[j], a[i], b[i], b[j]) if area > 0 else (a[i], a[j], b[j], b[i]))
    return core.mesh_object(name, bm)


def guitar(k, M, kind='double', color=0xb5221c, burst=False, trans=False, board='rose', guard=0xf2efe6, hw='nickel', seed=0, slot=None,
           head='inline', strings=6, tag=None, logo=None):
    """A guitar in its own frame: its face toward -y, body bottom at z 0, the neck straight up; moved by M. kind is a
    GUITAR_SHAPES key. Returns (the top of the headstock z, the hook point under the headstock's tip)."""
    rnd = random.Random(seed)
    mk = k.mark()
    acoustic = kind == 'acoustic'
    hollowb = kind == 'hollow'
    bass = kind == 'bass'
    th = 0.105 if acoustic else (0.048 if hollowb else 0.044)
    ctrl = GUITAR_SHAPES[kind]
    outline = closed_cr(ctrl, 6)
    top = GUITAR_TOP[kind]
    yf = -th / 2
    burst_c = (0.0, 0.0, 0.17 if not acoustic else 0.20)
    burst_aux2 = (0.30 if burst else 0.0, 1.0 if trans else 0.0, 0.0)
    k.add(xz_slab('body', outline, yf, -yf, r=0.006 if not acoustic else 0.004, seg=2), 'lacquer', color, slot=slot, aux_point=burst_c, aux2=burst_aux2)
    if acoustic or hollowb:
        # binding: a cream strip round the top's edge and the back's
        for (y0b, y1b) in ((yf - 0.0004, yf + 0.0045), (-yf - 0.0045, -yf + 0.0004)):
            k.add(edge_band('binding', outline, 1.004, y0b, y1b), 'plastic', 0xf0ead8, slot=slot)
    # The neck: heel in the body's pocket, a tapered neck, the fretboard with frets and dots, the nut, the headstock.
    scale = 0.86 if bass else (0.645 if not acoustic else 0.645)
    nfrets = 20 if bass else (20 if acoustic else 21)
    z_nut = (top + 0.17 if not acoustic else top + 0.19) if not bass else top + 0.40
    if acoustic:
        z_nut = top + 0.24
    z_heel = top - (0.07 if not acoustic else 0.0)
    w_nut, w_heel = (0.042, 0.056) if not bass else (0.040, 0.062)
    board_top = yf - 0.006 if not acoustic else yf - 0.004
    neck_back = yf + 0.026
    ring_h = []
    for zz, ww in ((z_heel, w_heel), (z_nut, w_nut)):
        ring = []
        for i in range(13):
            a = math.pi * i / 12
            ring.append((ww / 2 * math.cos(a), yf - 0.0005 + (neck_back - yf) * math.sin(a) * 0.9, zz))
        ring_h.append([(x, y, z) for x, y, z in ring])
    neck = core.loft('neck', [r_ for r_ in ring_h], caps=True)
    k.add(neck, 'wood', 0xc8a068 if board != 'maple' else 0xd8b47a, slot=slot, aux=(0, 0, 0))
    board_col = {'rose': 0x2e1c12, 'ebony': 0x141010, 'maple': 0xd8b47a}[board]
    fb = core.loft('fretboard', [[(-w_heel / 2, yf - 0.0005, z_heel + 0.002), (w_heel / 2, yf - 0.0005, z_heel + 0.002),
                                  (w_heel / 2, board_top, z_heel + 0.002), (-w_heel / 2, board_top, z_heel + 0.002)],
                                 [(-w_nut / 2, yf - 0.0005, z_nut), (w_nut / 2, yf - 0.0005, z_nut), (w_nut / 2, board_top, z_nut),
                                  (-w_nut / 2, board_top, z_nut)]], caps=True)
    k.add(fb, 'wood', board_col, slot=slot, aux=(0, 0, 0))
    L = scale
    for n in range(1, nfrets + 1):
        d = L - L / (2 ** (n / 12.0))
        zz = z_nut - d
        if zz < z_heel + 0.01:
            break
        t = (z_nut - zz) / (z_nut - z_heel)
        ww = w_nut + (w_heel - w_nut) * t
        k.add(bx('fret', -ww / 2, ww / 2, board_top - 0.0012, board_top, zz - 0.0011, zz + 0.0011, 0.0005), hw if hw != 'gold' else 'nickel', slot=slot)
        if n in (3, 5, 7, 9, 15, 17) or n == 12:
            zc = zz + (L / (2 ** ((n - 1) / 12.0)) - L / (2 ** (n / 12.0))) / 2
            for dx in ((-0.009, 0.009) if n == 12 else (0.0,)):
                k.add(parts.rod('dot', (dx, board_top + 0.0002, zc), (dx, board_top - 0.0003, zc), 0.0032, 8), 'pearl', 0xf4f0e6, slot=slot)
    k.add(bx('nut', -w_nut / 2, w_nut / 2, board_top - 0.0018, yf, z_nut, z_nut + 0.005, 0.0008), 'plastic', 0xf0e8d0, slot=slot)
    # Headstock: a slab tilted back a little (acoustics and the single-cut more), its outline by style.
    tilt = 0.0 if head == 'inline' else 13.0
    hl = 0.19 if not bass else 0.21
    if head == 'inline':
        hs = [(-0.022, 0.0), (0.022, 0.0), (0.03, 0.03), (0.026, 0.07), (0.036, 0.13), (0.048, 0.175), (0.035, hl), (0.0, hl - 0.006),
              (-0.03, hl - 0.03), (-0.036, 0.11), (-0.03, 0.05)]
    else:
        hs = [(-0.022, 0.0), (0.022, 0.0), (0.042, 0.04), (0.046, 0.12), (0.05, hl - 0.012), (0.03, hl), (0.0, hl - 0.01), (-0.03, hl),
              (-0.05, hl - 0.012), (-0.046, 0.12), (-0.042, 0.04)]
    head_M = T(0.0, yf + 0.001, z_nut + 0.004) @ RX(-tilt)
    k.add(xz_slab('headstock', closed_cr(hs, 4) if head != 'inline' else closed_cr(hs, 4), -0.0, 0.016, r=0.003, matrix=head_M), 'lacquer',
          color if not burst else 0x150a06, slot=slot, aux=(0, 0, 0))
    if logo:
        lx, lz, lw = (0.021, 0.16, 0.042) if head == "inline" else (0.0, hl - 0.026, 0.06)
        k.decal(logo, (lx, -0.0006, lz), u=(1, 0, 0), v=(0, 0, 1), lift=0.0, w=lw, h=lw * 0.23, matrix=head_M)
    # tuners: posts on the face, the gear housings behind, the keys out past the edge(s)
    n_str = strings
    posts = []
    for i in range(n_str):
        if head == 'inline':
            side = -1
            px, pz = 0.006 - 0.0035 * i, 0.03 + i * (0.15 if not bass else 0.17) / max(n_str - 1, 1)
        else:
            side = -1 if i < n_str // 2 else 1
            j = i if i < n_str // 2 else i - n_str // 2
            px, pz = side * 0.03, 0.05 + j * (0.045 if n_str == 6 else 0.07)
        posts.append((px, pz, side))
    for (px, pz, side) in posts:
        kx = -0.047 if head == 'inline' else side * 0.064
        base = head_M @ Vector((px, 0.0, pz))
        k.add(parts.rod('bushing', tuple(base), tuple(head_M @ Vector((px, -0.003, pz))), 0.0045, 8), hw, slot=slot)
        k.add(parts.rod('post', tuple(base), tuple(head_M @ Vector((px, -0.012, pz))), 0.003 if not bass else 0.005, 6), hw, slot=slot)
        k.add(obox('housing', tuple(head_M @ Vector((px + side * 0.006, 0.021, pz))), (0.016, 0.011, 0.015), rot_x=tilt, r=0.0), hw, slot=slot)
        k.add(parts.rod('shaft', tuple(head_M @ Vector((px + side * 0.012, 0.021, pz))), tuple(head_M @ Vector((kx, 0.021, pz))), 0.0022, 8), hw,
              slot=slot)
        k.add(obox('key', tuple(head_M @ Vector((kx + side * 0.006, 0.021, pz))), (0.012, 0.006, 0.018) if not bass else (0.02, 0.008, 0.026),
                   rot_x=tilt, r=0.0), 'plastic' if (hw == 'nickel' and rnd.random() < 0.3) else hw, 0xf2ead8, slot=slot)
    # Bridge, pickups, knobs, guard, strap buttons, jack; or for the acoustic, the soundhole, rosette, bridge and pins.
    zb = 0.10 if not acoustic else 0.14
    if acoustic:
        k.add(xz_slab('soundhole', closed_cr([(0.043 * math.cos(a), 0.30 + 0.043 * math.sin(a)) for a in [i * math.pi / 8 for i in range(16)]], 2),
                      yf - 0.0006, yf + 0.004), 'rubber', 0x0a0806, slot=slot)
        k.add(torus('rosette', (0.0, yf - 0.0003, 0.30), 0.053, 0.0035, axis='y', nu=32, nv=4), 'pearl', 0xd8cbb0, slot=slot)
        k.add(torus('rosette2', (0.0, yf - 0.0003, 0.30), 0.062, 0.0018, axis='y', nu=32, nv=4), 'wood', 0x2a1a10, slot=slot)
        guard_pts = [(0.045, 0.25), (0.11, 0.26), (0.13, 0.31), (0.11, 0.36), (0.07, 0.37), (0.055, 0.34), (0.05, 0.29)]
        k.add(xz_slab('guard', closed_cr(guard_pts, 4), yf - 0.0012, yf), 'gloss', 0x3a1a0c, slot=slot)
        k.add(xz_slab('bridge', [(-0.08, zb - 0.012), (0.08, zb - 0.012), (0.085, zb + 0.004), (0.06, zb + 0.016), (-0.06, zb + 0.016),
                                 (-0.085, zb + 0.004)], yf - 0.008, yf + 0.0002, r=0.002), 'wood', 0x1a100a, slot=slot)
        k.add(bx('saddle', -0.036, 0.036, yf - 0.012, yf - 0.008, zb + 0.002, zb + 0.005, 0.0008), 'plastic', 0xf0e8d0, slot=slot)
        for i in range(6):
            k.add(parts.rod('pin', (-0.025 + i * 0.01, yf - 0.008, zb - 0.004), (-0.025 + i * 0.01, yf - 0.012, zb - 0.004), 0.0028, 10), 'plastic',
                  0xf2eee2, slot=slot)
        string_z0 = zb + 0.003
    else:
        if guard is not None and kind in ('double', 'offset', 'tele', 'bass', 'horns'):
            gp = {'double': [(0.13, 0.05), (0.12, 0.13), (0.10, 0.23), (0.11, 0.29), (0.08, 0.35), (0.03, 0.35), (-0.03, 0.35), (-0.05, 0.37),
                             (-0.085, 0.36), (-0.09, 0.28), (-0.06, 0.20), (-0.02, 0.17), (0.04, 0.08)],
                  'offset': [(0.12, 0.06), (0.13, 0.15), (0.11, 0.26), (0.10, 0.33), (0.06, 0.36), (-0.03, 0.36), (-0.09, 0.38), (-0.12, 0.33),
                             (-0.10, 0.25), (-0.04, 0.20), (0.02, 0.10)],
                  'tele': [(0.10, 0.20), (0.11, 0.30), (0.07, 0.33), (-0.03, 0.33), (-0.08, 0.36), (-0.11, 0.33), (-0.08, 0.24), (-0.04, 0.17),
                           (0.04, 0.16)],
                  'bass': [(0.12, 0.10), (0.10, 0.24), (0.11, 0.33), (0.08, 0.37), (-0.03, 0.37), (-0.07, 0.42), (-0.10, 0.40), (-0.10, 0.28),
                           (-0.04, 0.22), (0.02, 0.12)],
                  'horns': [(0.09, 0.18), (0.10, 0.27), (0.06, 0.33), (-0.06, 0.33), (-0.10, 0.27), (-0.09, 0.18), (-0.04, 0.15), (0.04, 0.15)]}[kind]
            k.add(xz_slab('guard', closed_cr(gp, 4), yf - 0.0024, yf - 0.0002, r=0.0008), 'gloss', guard, slot=slot)
        two_hum = kind in ('single', 'hollow', 'horns')
        pzs = [(0.15, 'hum'), (0.29, 'hum')] if two_hum else ([(0.13, 'single'), (0.21, 'single'), (0.28, 'single')] if kind in ('double', 'offset')
                                                            else ([(0.09, 'single'), (0.28, 'single')] if kind == 'tele' else [(0.19, 'split')]))
        for pz, kind_p in pzs:
            if kind_p == 'hum':
                k.add(bx('ring', -0.047, 0.047, yf - 0.006, yf, pz - 0.028, pz + 0.028, 0.004), 'plastic', 0x101010, slot=slot)
                k.add(bx('pickup', -0.036, 0.036, yf - 0.011, yf - 0.004, pz - 0.019, pz + 0.019, 0.003), hw if hw != 'black' else 'plastic', 0x111111,
                      slot=slot)
                for row in (-0.009, 0.009):
                    for i in range(6):
                        k.add(parts.rod('pole', (-0.025 + i * 0.01, yf - 0.011, pz + row), (-0.025 + i * 0.01, yf - 0.0118, pz + row), 0.0021, 5), 'nickel',
                              slot=slot)
            elif kind_p == 'single':
                k.add(bx('pickup', -0.036, 0.036, yf - 0.012, yf - 0.002, pz - 0.009, pz + 0.009, 0.004), 'plastic', 0xf2eee0 if guard != 0x101010 else 0x111111,
                      slot=slot)
                for i in range(6):
                    k.add(parts.rod('pole', (-0.025 + i * 0.01, yf - 0.012, pz), (-0.025 + i * 0.01, yf - 0.0126, pz), 0.0022, 5), 'nickel', slot=slot)
            else:
                for dz, sx in ((-0.012, -1), (0.012, 1)):
                    k.add(bx('pickup', sx * 0.022 - 0.022, sx * 0.022 + 0.022, yf - 0.012, yf - 0.002, pz + dz - 0.009, pz + dz + 0.009, 0.004), 'plastic',
                          0x111111, slot=slot)
        # bridge
        if kind in ('single', 'hollow', 'horns'):
            k.add(bx('tunematic', -0.04, 0.04, yf - 0.014, yf - 0.002, zb + 0.008, zb + 0.016, 0.002), hw, slot=slot)
            k.add(bx('stopbar', -0.042, 0.042, yf - 0.012, yf - 0.001, zb - 0.03, zb - 0.02, 0.004), hw, slot=slot)
            string_z0 = zb - 0.025
        else:
            k.add(bx('bridgeplate', -0.04, 0.04, yf - 0.003, yf, zb - 0.03, zb + 0.016, 0.002), hw, slot=slot)
            for i in range(n_str):
                xx = -0.025 + i * 0.01 if n_str == 6 else -0.027 + i * 0.018
                k.add(bx('saddle', xx - 0.004, xx + 0.004, yf - 0.009, yf - 0.003, zb - 0.002, zb + 0.012, 0.001), hw, slot=slot)
            string_z0 = zb + 0.006
        # knobs and switch
        kp = {'double': [(0.075, 0.07), (0.09, 0.11), (0.095, 0.155)], 'offset': [(0.085, 0.08), (0.10, 0.12)], 'tele': [(0.085, 0.06), (0.09, 0.10)],
              'bass': [(0.08, 0.07), (0.09, 0.11)], 'single': [(0.075, 0.06), (0.11, 0.075), (0.085, 0.11), (0.12, 0.125)],
              'hollow': [(0.085, 0.06), (0.12, 0.075), (0.095, 0.11), (0.13, 0.125)], 'horns': [(0.06, 0.07), (0.10, 0.08), (0.075, 0.11)]}[kind]
        for (kx, kz) in kp:
            k.add(lathe_axis('knob', [(0.0, 0.0), (0.0115, 0.0), (0.0115, 0.006), (0.0105, 0.016), (0.0, 0.018)], T(kx, yf - 0.002, kz) @ RX(90), 12),
                  'plastic' if hw != 'gold' else 'gold', 0x111111 if kind in ('single', 'hollow') else 0xf2eee0, slot=slot)
        k.add(parts.rod('switch', (-0.09 if kind != 'double' else 0.06, yf, 0.36 if kind == 'single' else 0.13),
                        (-0.09 if kind != 'double' else 0.06, yf - 0.025, 0.36 if kind == 'single' else 0.135), 0.0015, 8), hw, slot=slot)
        if hollowb:
            for sx in (-1, 1):
                fh = [(sx * 0.12, 0.08), (sx * 0.125, 0.14), (sx * 0.13, 0.22), (sx * 0.126, 0.30), (sx * 0.131, 0.30), (sx * 0.136, 0.22),
                      (sx * 0.131, 0.14), (sx * 0.126, 0.08)]
                k.add(xz_slab('fhole', fh, yf - 0.0004, yf + 0.003), 'rubber', 0x050403, slot=slot)
    # Strings: from the bridge to the nut, then fanned to the posts.
    for i, (px, pz, side) in enumerate(posts):
        xs = (-0.025 + i * 0.01) if n_str == 6 else (-0.027 + i * 0.018)
        xn = (-w_nut / 2 + 0.004 + i * (w_nut - 0.008) / max(n_str - 1, 1))
        a = Vector((xs * (1.0 if not acoustic else 1.3), board_top - 0.004 if not acoustic else yf - 0.012, string_z0))
        b = Vector((xn, board_top - 0.002, z_nut + 0.002))
        c = head_M @ Vector((px, -0.008, pz))
        rr = (0.0007 + 0.00012 * (5 - i)) if not bass else 0.0013 + 0.0002 * (3 - i)
        k.add(tube('string', [tuple(a), tuple(b), tuple(c)], rr, n=6), 'nickel', slot=slot)
    # strap buttons
    k.add(parts.rod('strapbtn', (0.0, 0.0, 0.0), (0.0, 0.0, -0.008), 0.006, 10), hw, slot=slot)
    hook_z = z_nut + 0.006
    if tag:
        # a manila tag on a string round the lowest tuner
        px, pz, side = posts[-1] if head == 'inline' else posts[0]
        anchor = head_M @ Vector((px, -0.013, pz))
        tg = anchor + Vector((0.012 * side, -0.004, -0.09))
        k.add(tube('tag_string', [tuple(anchor), tuple((anchor + tg) / 2 + Vector((0, -0.003, 0))), tuple(tg + Vector((0, 0, 0.03)))], 0.0005, 5),
              'cloth', 0xe8e0c8, slot=slot)
        k.decal(tag, tuple(tg), u=(1, 0, 0), v=(0, 0, 1), lift=0.0, w=0.035, h=0.06)
    k.place(mk, M)
    top_z = z_nut + hl
    return top_z, hook_z


def guitar_stand(k, M, slot=None):
    """A folding A-frame guitar stand: tubular legs, padded cradle arms, a neck yoke at 0.72 m (the guitar leans back 12
    degrees into it)."""
    mk = k.mark()
    for sx in (-1, 1):
        k.add(tube('leg', [(sx * 0.16, -0.10, 0.0), (sx * 0.12, -0.05, 0.12), (sx * 0.05, 0.02, 0.62)], 0.009, 10), 'black_metal', slot=slot)
        k.add(parts.rod('foot', (sx * 0.16, -0.13, 0.008), (sx * 0.16, -0.07, 0.008), 0.012, 12), 'rubber', 0x111111, slot=slot)
        k.add(parts.rod('cradle', (sx * 0.10, -0.12, 0.05), (sx * 0.10, -0.02, 0.07), 0.011, 12), 'foam', 0x1a1a1a, slot=slot)
    k.add(tube('back', [(0.0, 0.16, 0.0), (0.0, 0.10, 0.25), (0.0, 0.03, 0.62)], 0.009, 10), 'black_metal', slot=slot)
    k.add(tube('yoke', bend([(0.0, 0.03, 0.62), (0.0, 0.0, 0.70), (-0.03, -0.01, 0.74), (0.03, -0.01, 0.74)], 0.02), 0.006, 10), 'foam', 0x1a1a1a, slot=slot)
    k.add(parts.rod('crossbar', (-0.12, -0.08, 0.06), (0.12, -0.08, 0.06), 0.008, 10), 'black_metal', slot=slot)
    k.place(mk, M)


# ------------------------------------------------------------------ amps, speakers

def amp(k, M, w=0.56, h=0.46, d=0.25, tolex=0x141414, grill=0x7a7468, panel='top', logo=None, slot=None, seed=0, knobs=6, tag=None):
    """A combo amp: tolex box, recessed grill cloth framed in piping, a chrome control panel along the top (facing up and
    out), a leather handle, metal corners, a logo plate."""
    mk = k.mark()
    r = 0.012
    k.add(bx('box', -w / 2, w / 2, -d / 2, d / 2, 0.012, h, r, 3), 'tolex', tolex, slot=slot)
    gz0, gz1 = 0.04, h - (0.09 if panel == 'front' else 0.05)
    k.add(bx('grill', -w / 2 + 0.03, w / 2 - 0.03, -d / 2 - 0.0005, -d / 2 + 0.01, gz0, gz1, 0.006), 'grill', grill, slot=slot)
    for (a0, a1, b0, b1) in ((-w / 2 + 0.028, w / 2 - 0.028, gz1, gz1 + 0.006), (-w / 2 + 0.028, w / 2 - 0.028, gz0 - 0.006, gz0),
                             (-w / 2 + 0.028, -w / 2 + 0.034, gz0, gz1), (w / 2 - 0.034, w / 2 - 0.028, gz0, gz1)):
        k.add(bx('piping', a0, a1, -d / 2 - 0.004, -d / 2 + 0.004, b0, b1, 0.002), 'plastic', 0xd8cfb8, slot=slot)
    if panel == 'front':
        k.add(bx('panel', -w / 2 + 0.03, w / 2 - 0.03, -d / 2 - 0.002, -d / 2 + 0.005, h - 0.08, h - 0.02, 0.002), 'chrome', slot=slot)
        for i in range(knobs):
            kx = -w / 2 + 0.08 + i * (w - 0.16) / max(knobs - 1, 1)
            k.add(lathe_axis('knob', [(0.0, 0.0), (0.012, 0.0), (0.011, 0.014), (0.0, 0.016)], T(kx, -d / 2 - 0.002, h - 0.05) @ RX(90), 16), 'plastic',
                  0x111111, slot=slot)
    else:
        k.add(bx('panel', -w / 2 + 0.03, w / 2 - 0.03, -d / 2 + 0.01, -d / 2 + 0.07, h - 0.002, h + 0.003, 0.002), 'chrome', slot=slot)
        for i in range(knobs):
            kx = -w / 2 + 0.08 + i * (w - 0.16) / max(knobs - 1, 1)
            k.add(lathe_axis('knob', [(0.0, 0.0), (0.011, 0.0), (0.010, 0.016), (0.0, 0.018)], T(kx, -d / 2 + 0.045, h + 0.003), 16), 'plastic', 0x111111,
                  slot=slot)
        k.add(parts.rod('jewel', (w / 2 - 0.06, -d / 2 + 0.03, h + 0.003), (w / 2 - 0.06, -d / 2 + 0.03, h + 0.012), 0.008, 12), 'gem', 0xb01010,
              slot=slot)
    # handle: a strap with end caps
    k.add(tube('handle', bend([(-0.09, 0.0, h), (-0.08, 0.0, h + 0.03), (0.08, 0.0, h + 0.03), (0.09, 0.0, h)], 0.02), 0.009, 10), 'leather',
          0x1a1410, slot=slot)
    for sx in (-1, 1):
        k.add(bx('cap', sx * 0.09 - 0.015, sx * 0.09 + 0.015, -0.015, 0.015, h - 0.001, h + 0.008, 0.003), 'chrome', slot=slot)
    for sx in (-1, 1):
        for sy in (-1, 1):
            for sz in (0, 1):
                cz = 0.012 if sz == 0 else h
                k.add(obox('corner', (sx * (w / 2 - 0.012), sy * (d / 2 - 0.012), cz + (0.012 if sz == 0 else -0.012)), (0.036, 0.036, 0.036), r=0.008, seg=1),
                      'chrome', slot=slot)
    for sx in (-1, 1):
        for sy in (-1, 1):
            k.add(parts.rod('foot', (sx * (w / 2 - 0.05), sy * (d / 2 - 0.05), 0.0), (sx * (w / 2 - 0.05), sy * (d / 2 - 0.05), 0.012), 0.016, 12), 'rubber',
                  0x111111, slot=slot)
    if logo:
        k.decal(logo, (w * 0.28, -d / 2 - 0.0012, gz1 - 0.035), u=(1, 0, 0), v=(0, 0, 1), lift=0.0, w=0.11, h=0.028)
    if tag:
        k.decal(tag, (-w / 2 + 0.04, -d / 2 - 0.001, h - 0.06), u=(1, 0, 0), v=(0, 0, 1), lift=0.0, w=0.035, h=0.06)
    k.place(mk, M)


def speaker(k, M, w=0.26, h=0.40, d=0.26, col=0x1a1a1a, woofer=0.085, slot=None):
    """A bookshelf or floor speaker: veneer-wrapped box, a woofer with its surround and dust cap, a tweeter, a port."""
    mk = k.mark()
    k.add(bx('box', -w / 2, w / 2, -d / 2, d / 2, 0.0, h, 0.004, 2), 'wood' if col != 0x1a1a1a else 'plastic', col, slot=slot, aux=(1, 0, 0))
    k.add(bx('baffle', -w / 2 + 0.004, w / 2 - 0.004, -d / 2 - 0.003, -d / 2, 0.004, h - 0.004, 0.003), 'plastic', 0x141414, slot=slot)
    cz = woofer + 0.04
    FR = RX(90)
    k.add(lathe_axis('woofer', [(0.0, 0.004), (woofer * 0.22, 0.004), (woofer * 0.3, 0.012), (woofer * 0.8, 0.004), (woofer * 0.95, 0.008),
                                (woofer, 0.0), (0.0, 0.0)][::-1], T(0.0, -d / 2 - 0.003, cz) @ FR @ RX(180), 40), 'plastic', 0x202020, slot=slot)
    k.add(torus('surround', (0.0, -d / 2 - 0.006, cz), woofer * 0.88, 0.006, axis='y', nu=40, nv=8), 'rubber', 0x151515, slot=slot)
    k.add(lathe_axis('cap', [(0.0, 0.0), (woofer * 0.25, 0.0), (woofer * 0.2, 0.012), (0.0, 0.016)], T(0.0, -d / 2 - 0.005, cz) @ FR, 24), 'plastic',
          0x2a2a2a, slot=slot)
    tz = h - 0.06
    k.add(lathe_axis('tweeter', [(0.0, 0.0), (0.022, 0.0), (0.02, 0.006), (0.012, 0.012), (0.0, 0.014)], T(0.0, -d / 2 - 0.003, tz) @ FR, 24), 'chrome',
          slot=slot)
    k.add(torus('tw_ring', (0.0, -d / 2 - 0.004, tz), 0.03, 0.004, axis='y', nu=28, nv=6), 'plastic', 0x101010, slot=slot)
    k.place(mk, M)


# ------------------------------------------------------------------ screens, consoles, cameras, laptops

def tv(k, M, w=1.11, h=0.64, foot='feet', slot=None, tag=None, logo=None, screen_mat=None):
    """A flat-panel TV: a thin bezel round the dark screen, the back's bulge, its feet (or a pedestal). Front -y; the
    screen's lower edge at z = 0.07 above its feet. screen_mat: a [screen] material to put on the picture instead."""
    mk = k.mark()
    z0 = 0.075
    t = 0.012
    k.add(bx('bezel', -w / 2 - 0.008, w / 2 + 0.008, -t, 0.0, z0 - 0.012, z0 + h + 0.008, 0.003), 'plastic', 0x141414, slot=slot)
    if screen_mat:
        k.screen('screen', screen_mat, (0.0, -t - 0.0004, z0 + h / 2), '-y', w, h)
    else:
        k.add(bx('screen', -w / 2, w / 2, -t - 0.0006, -t + 0.001, z0, z0 + h, 0.0), 'screen', slot=slot)
    k.add(bx('back', -w * 0.38, w * 0.38, 0.0, 0.045, z0 + 0.02, z0 + h * 0.7, 0.02, 3), 'plastic', 0x1a1a1a, slot=slot)
    k.add(bx('logo_bar', -0.04, 0.04, -t - 0.001, -t, z0 - 0.009, z0 - 0.004), 'chrome', slot=slot)
    if foot == 'feet':
        for sx in (-1, 1):
            fx = sx * w * 0.38
            k.add(tube('foot', bend([(fx - sx * 0.05, -0.12, 0.0), (fx, -0.02, 0.0), (fx + sx * 0.0, 0.0, z0 + 0.02)], 0.02), 0.008, 10), 'plastic',
                  0x1c1c1c, slot=slot)
            k.add(tube('foot2', bend([(fx, -0.02, 0.0), (fx + sx * 0.05, 0.12, 0.0)], 0.02), 0.008, 10), 'plastic', 0x1c1c1c, slot=slot)
    elif foot == 'pedestal':
        k.add(bx('neck', -0.03, 0.03, 0.005, 0.03, 0.0, z0 + 0.15, 0.01), 'plastic', 0x1c1c1c, slot=slot)
        k.add(bx('base', -0.16, 0.16, -0.10, 0.12, 0.0, 0.012, 0.006), 'plastic', 0x1c1c1c, slot=slot)
    if logo:
        k.decal(logo, (0.0, -t - 0.0012, z0 - 0.0065), u=(1, 0, 0), v=(0, 0, 1), lift=0.0, w=0.05, h=0.007)
    if tag:
        k.decal(tag, (w / 2 - 0.03, -t - 0.0025, z0 + h - 0.05), u=(1, 0, 0), v=(0, 0, 1), lift=0.0, w=0.035, h=0.06)
    k.place(mk, M)


def console(k, M, slot=None, col=0xe8e8e4, logo=None):
    """A game console standing on its side: a tall slab with a curved shell, a disc slot, a power LED."""
    mk = k.mark()
    k.add(bx('core', -0.045, 0.045, -0.13, 0.13, 0.0, 0.32, 0.01, 3), 'plastic', 0x141414, slot=slot)
    for sx in (-1, 1):
        k.add(bx('shell', sx * 0.045 - (0.012 if sx > 0 else 0.0), sx * 0.045 + (0.012 if sx < 0 else 0.0), -0.13, 0.13, 0.008, 0.34, 0.012, 3),
              'gloss', col, slot=slot)
    k.add(bx('slot', -0.002, 0.002, -0.1305, -0.129, 0.12, 0.26), 'rubber', 0x050505, slot=slot)
    k.add(bx('stand', -0.06, 0.06, -0.08, 0.08, 0.0, 0.01, 0.004), 'plastic', 0x141414, slot=slot)
    if logo:
        k.decal(logo, (0.058, 0.0, 0.25), u=(0, 1, 0), v=(0, 0, 1), lift=0.0002, w=0.08, h=0.02)
    k.place(mk, M)


def controller(k, M, col=0x1a1a1a, slot=None):
    """A gamepad lying face up: two grips, sticks, a d-pad, four buttons."""
    mk = k.mark()
    body = [(-0.075, -0.035), (-0.06, -0.055), (-0.035, -0.05), (-0.02, -0.025), (0.02, -0.025), (0.035, -0.05), (0.06, -0.055), (0.075, -0.035),
            (0.07, 0.01), (0.05, 0.03), (-0.05, 0.03), (-0.07, 0.01)]
    k.add(xy_slab('pad', closed_cr(body, 4), 0.0, 0.03, r=0.01, seg=3), 'plastic', col, slot=slot)
    for sx in (-1, 1):
        k.add(lathe_axis('stick', [(0.0, 0.0), (0.006, 0.0), (0.006, 0.012), (0.011, 0.014), (0.011, 0.018), (0.0, 0.019)],
                         T(sx * 0.025, -0.012, 0.028), 16), 'rubber', 0x222222, slot=slot)
    for (bx_, by_) in ((0.045, 0.0), (0.055, 0.01), (0.055, -0.01), (0.065, 0.0)):
        k.add(parts.rod('btn', (bx_, by_, 0.028), (bx_, by_, 0.034), 0.0045, 12), 'gloss', 0x2a2a2a, slot=slot)
    k.add(bx('dpad', -0.06, -0.04, -0.004, 0.004, 0.028, 0.033, 0.001), 'plastic', 0x2a2a2a, slot=slot)
    k.add(bx('dpad2', -0.054, -0.046, -0.01, 0.01, 0.028, 0.033, 0.001), 'plastic', 0x2a2a2a, slot=slot)
    k.place(mk, M)


def camera(k, M, slot=None, logo=None):
    """A DSLR with a zoom lens: grip, prism hump, mode dial, the lens's rings."""
    mk = k.mark()
    k.add(bx('body', -0.07, 0.07, -0.035, 0.035, 0.0, 0.09, 0.012, 3), 'plastic', 0x161616, slot=slot)
    k.add(bx('grip', 0.04, 0.075, -0.05, 0.03, 0.0, 0.088, 0.015, 3), 'rubber', 0x101010, slot=slot)
    k.add(xz_slab('prism', [(-0.03, 0.09), (0.03, 0.09), (0.02, 0.125), (-0.02, 0.125)], -0.02, 0.03, r=0.004), 'plastic', 0x161616, slot=slot)
    k.add(parts.rod('dial', (-0.05, 0.0, 0.09), (-0.05, 0.0, 0.102), 0.014, 20), 'plastic', 0x222222, slot=slot)
    k.add(parts.rod('shutter', (0.055, -0.02, 0.09), (0.055, -0.02, 0.096), 0.006, 14), 'chrome', slot=slot)
    lens = [(0.0, 0.0), (0.03, 0.0), (0.033, 0.01), (0.033, 0.03), (0.035, 0.032), (0.035, 0.06), (0.038, 0.065), (0.038, 0.09), (0.034, 0.092),
            (0.0, 0.092)]
    k.add(lathe_axis('lens', lens, T(-0.005, -0.035, 0.05) @ RX(90), 32), 'plastic', 0x141414, slot=slot)
    k.add(torus('zoom', (-0.005, -0.08, 0.05), 0.0365, 0.004, 'y', 32, 6), 'rubber', 0x0c0c0c, slot=slot)
    k.add(parts.rod('glass', (-0.005, -0.126, 0.05), (-0.005, -0.1275, 0.05), 0.028, 32), 'screen', slot=slot)
    if logo:
        k.decal(logo, (-0.005, -0.0351, 0.112), u=(1, 0, 0), v=(0, 0, 1), lift=0.0002, w=0.035, h=0.008)
    k.place(mk, M)


def laptop(k, M, w=0.34, d=0.23, open_deg=105.0, col=0x2a2c30, slot=None):
    """A laptop, open: base with a keyboard field and trackpad, the lid tilted back with its dark screen."""
    mk = k.mark()
    k.add(bx('base', -w / 2, w / 2, -d / 2, d / 2, 0.0, 0.018, 0.006, 3), 'alu' if col == 0xa8abb0 else 'plastic', col, slot=slot)
    k.add(bx('keys', -w / 2 + 0.02, w / 2 - 0.02, -d / 2 + 0.085, d / 2 - 0.02, 0.0175, 0.0185), 'plastic', 0x101010, slot=slot)
    for r in range(5):
        for c in range(14):
            kx = -w / 2 + 0.03 + c * (w - 0.06) / 13
            ky = d / 2 - 0.03 - r * 0.019
            k.add(bx('key', kx - 0.0075, kx + 0.0075, ky - 0.0075, ky + 0.0075, 0.018, 0.0195, 0.0), 'plastic', 0x1a1a1a, slot=slot)
    k.add(bx('pad', -0.05, 0.05, -d / 2 + 0.015, -d / 2 + 0.075, 0.0178, 0.0182, 0.002), 'plastic', 0x34363a, slot=slot)
    lidM = T(0.0, d / 2, 0.018) @ RX(90.0 - open_deg)
    o = bx('lid', -w / 2, w / 2, -0.006, 0.0, 0.0, d, 0.004)
    o.data.transform(lidM)
    k.add(o, 'alu' if col == 0xa8abb0 else 'plastic', col, slot=slot)
    s = bx('lcd', -w / 2 + 0.012, w / 2 - 0.012, -0.0062, -0.0058, 0.012, d - 0.012)
    s.data.transform(lidM)
    k.add(s, 'screen', slot=slot)
    k.place(mk, M)


# ------------------------------------------------------------------ tools

TOOL_COLORS = {'voltex': (0xe8661a, 0x1a1a1a), 'kragen': (0xb3141c, 0x161616), 'tealco': (0x0f8a8a, 0x1c1c1c), 'yellow': (0xe8b81a, 0x161616)}


def drill(k, M, brand='voltex', slot=None, battery=True, impact=False, logo=None):
    """A cordless drill (or impact driver) standing on its battery: the motor housing, the grip, the trigger, the chuck
    (or the hex collet), the battery pack."""
    c1, c2 = TOOL_COLORS[brand]
    mk = k.mark()
    if battery:
        k.add(bx('battery', -0.045, 0.045, -0.06, 0.05, 0.0, 0.07, 0.008, 3), 'plastic', c2, slot=slot)
        k.add(bx('battery_band', -0.046, 0.046, -0.061, 0.051, 0.045, 0.055, 0.004), 'plastic', c1, slot=slot)
    k.add(bx('foot', -0.04, 0.04, -0.05, 0.045, 0.07, 0.09, 0.008, 3), 'plastic', c1, slot=slot)
    grip = [(-0.022, 0.085), (0.022, 0.085), (0.026, 0.13), (0.02, 0.20), (-0.022, 0.20), (-0.028, 0.13)]
    k.add(yz_slab('grip', [(y, z) for y, z in grip], -0.022, 0.022, r=0.01, seg=3), 'rubber', c2, slot=slot)
    hl = 0.15 if not impact else 0.12
    k.add(bx('housing', -0.032, 0.032, -0.05, -0.05 + hl, 0.19, 0.265, 0.02, 3), 'plastic', c1, slot=slot)
    k.add(bx('trigger', -0.01, 0.01, -0.035, -0.022, 0.17, 0.2, 0.004), 'plastic', c2, slot=slot)
    for i in range(5):
        k.add(bx('vent', -0.0325, 0.0325, 0.045 - i * 0.008, 0.0485 - i * 0.008, 0.215, 0.245, 0.001), 'plastic', c2, slot=slot)
    FR = RX(90)
    if impact:
        k.add(lathe_axis('collet', [(0.0, 0.0), (0.018, 0.0), (0.016, 0.03), (0.01, 0.045), (0.0, 0.045)], T(0.0, -0.05, 0.228) @ FR, 20), 'black_metal',
              slot=slot)
    else:
        k.add(lathe_axis('clutch', [(0.0, 0.0), (0.03, 0.0), (0.03, 0.02), (0.0, 0.02)], T(0.0, -0.05, 0.228) @ FR, 24), 'plastic', c2, slot=slot)
        k.add(lathe_axis('chuck', [(0.0, 0.0), (0.022, 0.0), (0.022, 0.035), (0.015, 0.055), (0.0, 0.056)], T(0.0, -0.07, 0.228) @ FR, 24), 'black_metal',
              slot=slot)
        k.add(parts.rod('bit', (0.0, -0.125, 0.228), (0.0, -0.17, 0.228), 0.003, 6), 'steel', slot=slot)
    if logo:
        k.decal(logo, (0.0325, -0.05 + hl / 2, 0.228), u=(0, 1, 0), v=(0, 0, 1), lift=0.0003, w=hl * 0.7, h=0.022)
    k.place(mk, M)


def tool_case(k, M, w=0.48, d=0.36, h=0.11, brand='voltex', slot=None, logo=None):
    """A blow-molded tool case lying flat: ribbed lid, two latches, a molded handle."""
    c1, c2 = TOOL_COLORS[brand]
    mk = k.mark()
    k.add(bx('bottom', -w / 2, w / 2, -d / 2, d / 2, 0.0, h * 0.55, 0.02, 3), 'plastic', c2, slot=slot)
    k.add(bx('lid', -w / 2, w / 2, -d / 2, d / 2, h * 0.55, h, 0.02, 3), 'plastic', c1, slot=slot)
    for i in range(3):
        k.add(bx('rib', -w / 2 + 0.05, w / 2 - 0.05, -d / 2 + 0.06 + i * 0.09, -d / 2 + 0.08 + i * 0.09, h - 0.002, h + 0.006, 0.005), 'plastic', c1,
              slot=slot)
    for sx in (-1, 1):
        k.add(bx('latch', sx * w * 0.3 - 0.025, sx * w * 0.3 + 0.025, -d / 2 - 0.008, -d / 2 + 0.002, h * 0.4, h * 0.75, 0.004), 'plastic', 0x111111,
              slot=slot)
    k.add(tube('handle', bend([(-0.07, -d / 2 + 0.01, h * 0.6), (-0.06, -d / 2 - 0.035, h * 0.6), (0.06, -d / 2 - 0.035, h * 0.6),
                               (0.07, -d / 2 + 0.01, h * 0.6)], 0.02), 0.011, 10), 'plastic', c2, slot=slot)
    if logo:
        k.decal(logo, (0.0, 0.0, h + 0.0062), u=(1, 0, 0), v=(0, 1, 0), lift=0.0003, w=0.16, h=0.04)
    k.place(mk, M)


def circ_saw(k, M, brand='kragen', slot=None):
    """A circular saw resting on its shoe: blade guard, motor, the handle loop, the blade's teeth showing below."""
    c1, c2 = TOOL_COLORS[brand]
    mk = k.mark()
    k.add(bx('shoe', -0.08, 0.08, -0.14, 0.14, 0.0, 0.006, 0.004), 'alu', slot=slot)
    k.add(xz_slab('guard', closed_cr([(0.0, 0.006), (0.09, 0.03), (0.10, 0.10), (0.05, 0.15), (-0.03, 0.155), (-0.09, 0.11), (-0.10, 0.04)], 4),
                  -0.04, -0.01, r=0.005), 'cast', slot=slot)
    k.add(parts.rod('motor', (0.0, -0.01, 0.08), (0.0, 0.10, 0.08), 0.045, 24), 'plastic', c1, slot=slot)
    k.add(parts.rod('cap', (0.0, 0.10, 0.08), (0.0, 0.115, 0.08), 0.04, 24), 'plastic', c2, slot=slot)
    k.add(tube('handle', bend([(0.07, 0.02, 0.10), (0.07, 0.06, 0.20), (-0.02, 0.06, 0.21), (-0.06, 0.03, 0.12)], 0.03), 0.014, 12), 'rubber', c2,
          slot=slot)
    k.add(parts.rod('blade', (0.0, -0.035, 0.07), (0.0, -0.037, 0.07), 0.092, 48), 'steel', slot=slot)
    k.place(mk, M)


# ------------------------------------------------------------------ jewelry and watches

BUST_PROFILE = ((0.05, 0.095, 0.042), (0.07, 0.122, 0.048), (0.12, 0.142, 0.053), (0.17, 0.156, 0.054), (0.205, 0.162, 0.051), (0.225, 0.148, 0.047),
                (0.245, 0.098, 0.043), (0.265, 0.054, 0.04), (0.29, 0.042, 0.038), (0.39, 0.038, 0.035), (0.414, 0.036, 0.034), (0.42, 0.033, 0.031))


def bust_at(h, z):
    """The neck form's half width and half depth at height z over its foot (a form h tall)."""
    f_ = (h - 0.05) / 0.37
    pr = [(0.05 + (zz - 0.05) * f_, wx, wy) for zz, wx, wy in BUST_PROFILE]
    if z <= pr[0][0]:
        return pr[0][1], pr[0][2]
    for (z0, x0, y0), (z1, x1, y1) in zip(pr, pr[1:]):
        if z <= z1:
            t = (z - z0) / (z1 - z0)
            return x0 + (x1 - x0) * t, y0 + (y1 - y0) * t
    return pr[-1][1], pr[-1][2]


def bust_chain(bxx, by_, base, h, nz, sag):
    """A necklace's path on a neck form at (bxx, by_) standing on z = base, its chest toward -x: from behind the neck at
    nz, round the sides and down the front to its lowest point nz - sag, lying on the form."""
    lo = nz - sag
    mid = nz - sag * 0.45
    wx_m, wy_m = bust_at(h, mid - base)
    _, wy_lo = bust_at(h, lo - base)
    _, wy_top = bust_at(h, nz - base)
    return [(bxx + wy_top + 0.006, by_ - 0.03, nz), (bxx - wy_m * 0.55, by_ - wx_m - 0.003, mid), (bxx - wy_lo - 0.003, by_, lo),
            (bxx - wy_m * 0.55, by_ + wx_m + 0.003, mid), (bxx + wy_top + 0.006, by_ + 0.03, nz)]


def neck_bust(k, M, h=0.42, col=0x141414, slot=None):
    """A jeweler's neck form in velvet: shoulders, a long neck, on a round foot."""
    mk = k.mark()
    prof = [(0.0, 0.0), (0.07, 0.0), (0.075, 0.008), (0.05, 0.016), (0.03, 0.03), (0.03, 0.05)]
    k.add(lathe_axis('foot', prof + [(0.0, 0.05)], Matrix(), 40), 'black_metal', slot=slot)
    # the form: a flat-fronted chest sitting on the stem, square shoulders, a long neck cut level at the top
    rings = []
    f_ = (h - 0.05) / 0.37                # drawn for a 42 cm form, scaled to h
    for z, wx, wy in BUST_PROFILE:
        z = 0.05 + (z - 0.05) * f_
        ring = []
        for i in range(48):
            a = 2 * math.pi * i / 48
            ca, sa = math.cos(a), math.sin(a)
            ring.append((wx * math.copysign(abs(ca) ** 0.8, ca), wy * math.copysign(abs(sa) ** 0.8, sa), z))
        rings.append(ring)
    o = core.loft('bust', rings, caps=True)
    o.data.shade_smooth()
    k.add(o, 'velvet', col, slot=slot)
    k.place(mk, M)


def ring_band(k, center, r=0.0095, w=0.0026, col=0xf0c35a, stone=None, stone_r=0.003, slot=None, tilt=0.0):
    """A ring standing upright in a tray's slot (its plane XZ facing -y): a band, and a stone in a four-prong head."""
    c = Vector(center)
    o = torus('band', (0, 0, 0), r, w * 0.5, axis='y', nu=20, nv=5)
    o.data.transform(T(*c) @ RX(tilt))
    k.add(o, 'gold', col, slot=slot)
    if stone is not None:
        top = c + Vector((0.0, 0.0, r + 0.0005))
        g = lathe_axis('stone', [(0.0, -stone_r * 0.9), (stone_r, 0.0), (stone_r * 0.6, stone_r * 0.45), (0.0, stone_r * 0.5)], T(*(top + Vector((0, 0,
                                                                                                                                                       stone_r * 0.9)))), 8)
        k.add(g, 'gem', stone, slot=slot)
        for i in range(4):
            a = math.pi / 4 + i * math.pi / 2
            k.add(parts.rod('prong', tuple(top), tuple(top + Vector((stone_r * math.cos(a), stone_r * math.sin(a), stone_r * 1.4))), 0.0005, 5), 'gold', col,
                  slot=slot)


def ring_tray(k, M, rows=4, cols=9, col=0x141414, seed=0, slot=None, ring_slot=None, sparse=0.25):
    """A velvet ring tray: ridged rows that grip the rings, rings in most slots."""
    rnd = random.Random(seed)
    mk = k.mark()
    W, D = 0.30, 0.20
    k.add(bx('tray', -W / 2, W / 2, -D / 2, D / 2, 0.0, 0.025, 0.004), 'velvet', col, slot=slot)
    for r_ in range(rows + 1):
        y = -D / 2 + 0.02 + r_ * (D - 0.04) / rows
        k.add(parts.rod('ridge', (-W / 2 + 0.01, y, 0.025), (W / 2 - 0.01, y, 0.025), 0.009, 12), 'velvet', col, slot=slot)
    for r_ in range(rows):
        y = -D / 2 + 0.02 + (r_ + 0.5) * (D - 0.04) / rows
        for c_ in range(cols):
            if rnd.random() < sparse:
                continue
            x = -W / 2 + 0.025 + c_ * (W - 0.05) / (cols - 1)
            gold = rnd.choice([0xf0c35a, 0xf0c35a, 0xe8b04a, 0xe8d8b8, 0xdca07a])
            stone = rnd.choice([None, None, 0xf2f6ff, 0xf2f6ff, 0xa0142a, 0x1a3aa0, 0x168a4a])
            ring_band(k, (x, y, 0.03), r=rnd.uniform(0.0085, 0.0105), w=rnd.uniform(0.002, 0.0045), col=gold, stone=stone,
                      stone_r=rnd.uniform(0.0022, 0.0042), slot=ring_slot, tilt=rnd.uniform(-8, 8))
    k.place(mk, M)


def watch(k, M, col=0xd8d8d8, metal='chrome', dial=0x141414, strap='bracelet', strap_col=0x2a1a10, slot=None, dial_key=None):
    """A wristwatch closed round a pillow (its face toward -y, tilted up 20 degrees): the case with lugs, bezel, crown, the
    dial (a print when dial_key), and a bracelet of links or a leather strap wrapping round the back."""
    mk = k.mark()
    R = 0.019
    FR = RX(90)
    case = [(0.0, 0.0), (R, 0.0), (R + 0.001, 0.004), (R, 0.011), (R * 0.9, 0.013), (0.0, 0.013)]
    k.add(lathe_axis('case', case, T(0.0, 0.0, 0.0) @ FR, 40), 'gold' if metal == 'gold' else metal, col, slot=slot, aux=(0.08, 0, 0))
    k.add(torus('bezel', (0.0, -0.013, 0.0), R * 0.92, 0.0018, 'y', 40, 6), 'gold' if metal == 'gold' else metal, col, slot=slot, aux=(0.08, 0, 0))
    if dial_key:
        k.decal(dial_key, (0.0, -0.0122, 0.0), u=(1, 0, 0), v=(0, 0, 1), lift=0.0, w=2 * R * 0.84, h=2 * R * 0.84,
                outline=[(0.5 + 0.5 * math.cos(2 * math.pi * j / 40), 0.5 + 0.5 * math.sin(2 * math.pi * j / 40)) for j in range(40)])
    else:
        k.add(parts.rod('dial', (0.0, -0.011, 0.0), (0.0, -0.0122, 0.0), R * 0.84, 40), 'gloss', dial, slot=slot)
    k.add(parts.rod('crown', (R, -0.006, 0.0), (R + 0.005, -0.006, 0.0), 0.0028, 12), metal if metal != 'gold' else 'gold', col, slot=slot)
    for sz in (-1, 1):
        for sx in (-1, 1):
            k.add(bx('lug', sx * 0.011 - 0.0025, sx * 0.011 + 0.0025, -0.009, 0.0, sz * R * 0.85, sz * (R + 0.006), 0.0008),
                  'gold' if metal == 'gold' else metal, col, slot=slot)
    # the strap: an oval loop round the pillow behind the case
    pts = []
    for i in range(41):
        a = math.pi / 2 + 2 * math.pi * i / 40
        pts.append((0.0, 0.025 + 0.03 * math.cos(a) * 0.9 - 0.004, 0.028 * math.sin(a) * 1.15))
    if strap == 'bracelet':
        for i in range(0, 40):
            a = 2 * math.pi * (i + 0.5) / 40 + math.pi / 2
            if abs(math.sin(a)) < 0.6 and math.cos(a) < 0:
                continue
            p = Vector((0.0, 0.025 + 0.027 * math.cos(a) - 0.004, 0.032 * math.sin(a)))
            o = obox('link', (0.0, 0.0, 0.0), (0.019, 0.0028, 0.0042), r=0.0008)
            o.data.transform(T(*p) @ RX(-math.degrees(a) + 90))
            k.add(o, 'gold' if metal == 'gold' else metal, col, slot=slot, aux=(0.12, 0, 0))
    else:
        o = tube('strap', [p for p in pts], 0.001, n=4, closed=True)
        o.data.transform(Matrix.Diagonal((9.0, 1.0, 1.0, 1.0)))
        k.add(o, 'leather', strap_col, slot=slot)
    k.place(mk, M)


def pillow(k, M, w=0.06, d=0.05, h=0.045, col=0xe8e2d4, slot=None):
    mk = k.mark()
    o = blob('pillow', (0.0, 0.0, h / 2), (w, d, h), seed=3, detail=0.04, subdiv=3, smooth=2)
    k.add(o, 'velvet', col, slot=slot)
    k.place(mk, M)


def riser(k, M, w=0.30, d=0.20, h=0.06, col=0x141414, slot=None):
    mk = k.mark()
    k.add(bx('riser', -w / 2, w / 2, -d / 2, d / 2, 0.0, h, 0.004), 'velvet', col, slot=slot)
    k.place(mk, M)


# ------------------------------------------------------------------ brass


# ------------------------------------------------------------------ boxes and paper

def box_print(k, M, w, d, h, key, col=0xdedad0, slot=None, fin='cardboard', top_key=None, side_key=None):
    """A retail box with its front printed by panel `key` (and its top/side when given)."""
    mk = k.mark()
    k.add(bx('box', -w / 2, w / 2, -d / 2, d / 2, 0.0, h, min(0.004, w * 0.05)), fin, col, slot=slot)
    k.decal(key, (0.0, -d / 2 - 0.0004, h / 2), u=(1, 0, 0), v=(0, 0, 1), lift=0.0, w=w * 0.98, h=h * 0.98)
    if top_key:
        k.decal(top_key, (0.0, 0.0, h + 0.0004), u=(1, 0, 0), v=(0, 1, 0), lift=0.0, w=w * 0.98, h=d * 0.98)
    if side_key:
        k.decal(side_key, (w / 2 + 0.0004, 0.0, h / 2), u=(0, 1, 0), v=(0, 0, 1), lift=0.0, w=d * 0.98, h=h * 0.98)
    k.place(mk, M)


def hang_tag(k, anchor, key, drop=0.06, side=1.0, facing='-y', w=0.035, h=0.06, slot=None):
    """A manila tag on a string from anchor, hanging `drop` below it, laid on the plane by facing."""
    A = Vector(anchor)
    u = {'-y': Vector((1, 0, 0)), '-x': Vector((0, -1, 0)), '+y': Vector((-1, 0, 0)), '+x': Vector((0, 1, 0))}[facing]
    n = {'-y': Vector((0, -1, 0)), '-x': Vector((-1, 0, 0)), '+y': Vector((0, 1, 0)), '+x': Vector((1, 0, 0))}[facing]
    top = A + u * (0.008 * side) + n * 0.002 - Vector((0, 0, drop))
    k.add(tube('tag_string', [tuple(A), tuple((A + top) / 2 + n * 0.003), tuple(top)], 0.0005, 5), 'cloth', 0xe8e0c8, slot=slot)
    k.decal(key, tuple(top - Vector((0, 0, h / 2 - 0.004))), u=tuple(u), v=(0, 0, 1), lift=0.0, w=w, h=h)


# ================================================================== lettering as geometry (vinyl on clear glass, gold leaf, brass inlay)

_CAP = {}


def _font(face):
    return bpy.data.fonts.load(os.path.join(core.FONTS, f'Roboto-{face}.ttf'), check_existing=True)


def cap_ratio(face):
    """Cap height of Roboto <face> per unit of type size."""
    if face not in _CAP:
        cu = bpy.data.curves.new('cap', 'FONT')
        cu.body = 'H'
        cu.font = _font(face)
        cu.size = 1.0
        o = core.link(bpy.data.objects.new('cap', cu))
        bpy.context.view_layer.update()
        _CAP[face] = o.dimensions.y
        bpy.data.objects.remove(o)
        bpy.data.curves.remove(cu)
    return _CAP[face]


def flat_text(name, s, cap, matrix, face='Black', align='CENTER', tracking=1.0, depth=0.0, condense=1.0, max_w=None):
    """Text as a flat mesh (or a slab `depth` deep, standing out of the face): in its own frame x along the line, y up
    from the baseline, z out of the face; cap is the capital height in meters; moved by matrix (face_m gives one that
    reads correctly from outside the face). condense squeezes it horizontally; max_w shrinks it to fit."""
    cu = bpy.data.curves.new(name, 'FONT')
    cu.body = s
    cu.font = _font(face)
    cu.size = cap / cap_ratio(face)
    cu.space_character = tracking
    cu.align_x = align
    cu.extrude = depth / 2 if depth > 0 else 0.0
    cu.resolution_u = 2
    o = core.link(bpy.data.objects.new(name, cu))
    core.select_only([o])
    bpy.ops.object.convert(target='MESH')
    o = bpy.context.view_layer.objects.active
    o.name = name
    o.data.name = name
    sx = condense
    if max_w is not None:
        xs = [v.co.x for v in o.data.vertices]
        if xs:
            wnow = (max(xs) - min(xs)) * condense
            if wnow > max_w:
                sx *= max_w / wnow
    pre = Matrix.Diagonal((sx, 1.0, 1.0, 1.0))
    if depth > 0:
        pre = T(0.0, 0.0, depth / 2) @ pre
    o.data.transform(matrix @ pre)
    bm = bmesh.new()
    bm.from_mesh(o.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=1e-6)
    N = (matrix.to_3x3() @ Vector((0, 0, 1))).normalized()
    if depth <= 0:
        for f in bm.faces:
            f.normal_update()
            if f.normal.dot(N) < 0:
                f.normal_flip()
    else:
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    bm.to_mesh(o.data)
    bm.free()
    return o


# ================================================================== the neon (shared by the baked hardware and the glow meshes)

NEON_R = 0.0062          # 12 mm tube


def _rrect_path(c0, c1, r, steps=6):
    """A closed rounded rectangle in a plane given by two opposite corners (3D, same x): for the window's tube."""
    (x, ya, za), (_, yb, zb) = c0, c1
    pts = []
    for (cy, cz, a0) in ((yb - r, zb - r, 0.0), (ya + r, zb - r, 90.0), (ya + r, za + r, 180.0), (yb - r, za + r, 270.0)):
        for k in range(steps + 1):
            a = math.radians(a0 + 90.0 * k / steps)
            pts.append((x, cy + r * math.cos(a), cz + r * math.sin(a)))
    return pts


def neon_hardware(k, strokes, back, slot=None, frame=True, hang_to=None, transformer=True):
    """What a neon sign has besides its glowing tube: the blocked-out (black-painted) jumpers between strokes running
    behind the letters, the electrode caps at each end, the clear tube supports, and (frame) the black skeleton the tubes
    are wired to, its transformer and, hung_to a height, two chains. back: the unit vector toward the sign's back."""
    B = Vector(back)
    for a, b in zip(strokes, strokes[1:]):
        p, q = Vector(a[-1]), Vector(b[0])
        k.add(tube('jumper', bend([tuple(p), tuple(p + B * 0.022), tuple(q + B * 0.022), tuple(q)], 0.01), NEON_R * 0.95, n=6), 'gloss', 0x0c0c0c,
              slot=slot)
    for s, end in ((strokes[0], 0), (strokes[-1], -1)):
        p = Vector(s[end])
        k.add(parts.rod('electrode', tuple(p), tuple(p + B * 0.05), NEON_R * 1.35, 10), 'gloss', 0x101010, slot=slot)
        k.add(parts.rod('boot', tuple(p + B * 0.05), tuple(p + B * 0.075), NEON_R * 1.8, 10), 'rubber', 0x111111, slot=slot)
    pts = [Vector(p) for s in strokes for p in s]
    step = 0
    for s in strokes:
        L = 0.0
        for a, b in zip(s, s[1:]):
            L += (Vector(b) - Vector(a)).length
            if L > 0.22:
                L = 0.0
                p = Vector(a)
                k.add(parts.rod('support', tuple(p + B * NEON_R), tuple(p + B * 0.045), 0.0028, 8), 'gloss', 0xd8dcd8, slot=slot)
                k.add(parts.rod('support_clip', tuple(p + B * 0.008), tuple(p + B * 0.012), NEON_R * 1.3, 8), 'gloss', 0xd8dcd8, slot=slot)
                step += 1
    if frame and pts:
        # the skeleton: a black rod rectangle behind the letters
        lo = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
        hi = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
        c = (lo + hi) / 2 + B * 0.045
        if abs(B.x) > 0.5:
            y0, y1, z0, z1 = lo.y - 0.04, hi.y + 0.04, lo.z - 0.04, hi.z + 0.03
            corners = [(c.x, y0, z0), (c.x, y1, z0), (c.x, y1, z1), (c.x, y0, z1), (c.x, y0, z0)]
        else:
            x0, x1, z0, z1 = lo.x - 0.04, hi.x + 0.04, lo.z - 0.04, hi.z + 0.03
            corners = [(x0, c.y, z0), (x1, c.y, z0), (x1, c.y, z1), (x0, c.y, z1), (x0, c.y, z0)]
        for a, b in zip(corners, corners[1:]):
            k.add(parts.rod('skeleton', a, b, 0.004, 8), 'black_metal', slot=slot)
        for i in range(1, 4):
            t = i / 4
            a, b = Vector(corners[0]).lerp(Vector(corners[1]), t), Vector(corners[3]).lerp(Vector(corners[2]), t)
            k.add(parts.rod('skeleton', tuple(a), tuple(b), 0.0028, 8), 'black_metal', slot=slot)
        if transformer:
            base = Vector(corners[0]).lerp(Vector(corners[1]), 0.85)
            k.add(bx('transformer', base.x - 0.04, base.x + 0.04, base.y - 0.06, base.y + 0.06, base.z - 0.09, base.z - 0.005, 0.006), 'powder', 0x1a1a1a,
                  slot=slot)
            k.add(tube('cord', [tuple(base + Vector((0, 0, -0.05))), tuple(base + Vector((0.06, 0.02, -0.15))), tuple(base + Vector((0.12, 0.05, -0.2)))],
                       0.003, 6), 'rubber', 0x111111, slot=slot)
        if hang_to is not None:
            for corner in (corners[3], corners[2]):
                p = Vector(corner)
                k.add(parts.rod('cable', tuple(p), (p.x, p.y, hang_to), 0.0012, 6), 'steel', slot=slot)


# ================================================================== SM_NS_Pawn_Front (+ _Glass)


def draw_sticker_camera(s, w, h):
    s.rect(0, 0, w, h, 0xf2d21a, rough=0.35)
    s.rect(3, 3, w - 6, h - 6, 0x111111, rough=0.35)
    s.rect(5, 5, w - 10, h - 10, 0xf2d21a, rough=0.35)
    s.rect(w * 0.12, h * 0.55, w * 0.26, h * 0.2, 0x111111)
    s.poly([(w * 0.38, h * 0.6), (w * 0.48, h * 0.55), (w * 0.48, h * 0.8), (w * 0.38, h * 0.72)], 0x111111)
    s.mid('SMILE!', w * 0.72, h * 0.66, h * 0.16, w * 0.4, 0x111111, 'Black')
    s.mid('YOU ARE ON CAMERA', w / 2, h * 0.3, h * 0.1, w * 0.84, 0x111111, 'Black')
    s.mid('24 HR VIDEO SURVEILLANCE', w / 2, h * 0.14, h * 0.06, w * 0.84, 0x111111, 'Bold')


def logo_panel(text, ground, ink, face='Black', tracking=1.1, italic=False):
    def draw(s, w, h):
        s.rect(0, 0, w, h, ground, rough=0.4)
        s.mid(text, w / 2, h / 2, h * 0.62, w * 0.9, ink, face, tracking=tracking)
    return draw


def p_box_art(title, sub, ground, ink, accent, seed=0, img='console'):
    """A retail box's front: a color field, the product drawn simply, the name, badges."""
    def draw(s, w, h):
        rnd = random.Random(seed)
        s.rect(0, 0, w, h, ground, rough=0.45)
        s.rect(0, h * 0.82, w, h * 0.18, accent, rough=0.45)
        s.mid(title, w / 2, h * 0.91, h * 0.1, w * 0.86, ink, 'Black')
        if img == 'console':
            s.rect(w * 0.3, h * 0.25, w * 0.4, h * 0.45, 0xf0f0ee, rough=0.3)
            s.rect(w * 0.48, h * 0.25, w * 0.04, h * 0.45, 0x141414)
        elif img == 'drone':
            for cx, cy in ((0.3, 0.6), (0.7, 0.6), (0.3, 0.3), (0.7, 0.3)):
                s.ring(w * cx, h * cy, w * 0.08, w * 0.1, 0x222222)
            s.rect(w * 0.42, h * 0.38, w * 0.16, h * 0.14, 0x222222)
        elif img == 'phone':
            s.rect(w * 0.36, h * 0.15, w * 0.28, h * 0.6, 0x141414, rough=0.2)
            s.rect(w * 0.38, h * 0.18, w * 0.24, h * 0.54, 0x2a4a8a, rough=0.2)
        elif img == 'headphones':
            s.ring(w * 0.5, h * 0.45, w * 0.22, w * 0.26, 0x222222, a0=0.0, a1=math.pi)
            for cx in (0.28, 0.72):
                s.rect(w * (cx - 0.07), h * 0.25, w * 0.14, h * 0.22, 0x222222)
        s.mid(sub, w / 2, h * 0.1, h * 0.05, w * 0.8, ink, 'Bold')
    return draw


# ================================================================== the glow meshes (unbaked: the game colors them by the hour)


# ================================================================== SM_NS_Pawn_Shell: floor, walls, ceiling, the back-room door, signs


def draw_license(s, w, h):
    s.rect(0, 0, w, h, 0xf4f0e2, rough=0.6)
    s.rect(w * 0.04, h * 0.04, w * 0.92, h * 0.92, 0x2a4a6a, rough=0.6)
    s.rect(w * 0.05, h * 0.05, w * 0.90, h * 0.90, 0xf4f0e2, rough=0.6)
    s.mid('CITY OF NORTHSIDE', w / 2, h * 0.86, h * 0.06, w * 0.8, 0x2a4a6a, 'Black', tracking=1.3)
    s.mid('PAWNBROKER LICENSE', w / 2, h * 0.73, h * 0.09, w * 0.84, 0x161616, 'Black')
    s.mid('NORTHSIDE PAWN & LOAN', w / 2, h * 0.58, h * 0.06, w * 0.8, 0x161616, 'Bold')
    s.mid('232 MARKET STREET', w / 2, h * 0.49, h * 0.045, w * 0.7, 0x3a3a3a, 'Bold')
    s.mid('LICENSE No. PB-0417', w / 2, h * 0.38, h * 0.045, w * 0.7, 0x3a3a3a, 'Bold')
    s.mid('DISPLAY IN A CONSPICUOUS PLACE', w / 2, h * 0.14, h * 0.035, w * 0.8, 0x6a6a6a, 'Bold')
    s.circle(w * 0.78, h * 0.24, h * 0.08, 0xb08a3a, metal=1.0, rough=0.35)
    s.circle(w * 0.78, h * 0.24, h * 0.06, 0xc8a050, metal=1.0, rough=0.3)
    hand(s, 'R. Hollis', w * 0.12, h * 0.2, h * 0.07, 0x1b2f7a, width=0.07, seed=5)


def draw_exit(s, w, h):
    s.rect(0, 0, w, h, 0xf2efe6, rough=0.4)
    s.mid('EXIT', w / 2, h * 0.5, h * 0.62, w * 0.8, 0xd81a14, 'Black', tracking=1.15)


def draw_staff(s, w, h):
    s.rect(0, 0, w, h, 0xa3221c, rough=0.35)
    s.rect(3, 3, w - 6, h - 6, 0xf4f1ea, rough=0.4)
    s.rect(5, 5, w - 10, h - 10, 0xa3221c, rough=0.35)
    s.mid('EMPLOYEES ONLY', w / 2, h * 0.5, h * 0.4, w - 30, 0xf4f1ea, 'Black', tracking=1.15)


def draw_extinguisher_sign(s, w, h):
    s.rect(0, 0, w, h, 0xc8161c, rough=0.4)
    s.mid('FIRE', w / 2, h * 0.68, h * 0.2, w * 0.8, 0xffffff, 'Black')
    s.mid('EXTINGUISHER', w / 2, h * 0.36, h * 0.12, w * 0.86, 0xffffff, 'Black')
    s.poly([(w * 0.5, h * 0.1), (w * 0.42, h * 0.2), (w * 0.58, h * 0.2)], 0xffffff)


def draw_panel_label(s, w, h):
    s.rect(0, 0, w, h, 0xe8e6e0, rough=0.5)
    s.mid('PANEL A  ·  120/208V', w / 2, h * 0.62, h * 0.22, w * 0.86, 0x161616, 'Black')
    s.mid('KEEP 36 IN CLEAR', w / 2, h * 0.28, h * 0.16, w * 0.8, 0xb3141c, 'Black')


# ================================================================== SM_NS_Pawn_Cases (+ _Glass): the jewelry and watch showcases

WATCH_DIALS = [('CORVAN', 0x101418, 0xe8e4d8, 'AUTOMATIC'), ('CORVAN', 0xf2efe6, 0x161616, 'QUARTZ'), ('HALLER', 0x1c3a6e, 0xe8e4d8, 'CHRONOMETER'),
               ('HALLER', 0xc8b07a, 0x2a2010, 'AUTOMATIC'), ('VEDRA', 0x1e4a2e, 0xe8e4d8, 'DIVER 200M')]


def p_dial(brand, ground, ink, sub):
    """A watch dial at ten past ten (the way every watch in a case is set): markers, hands, the maker's name."""
    def draw(s, w, h):
        cx, cy, r = w / 2, h / 2, w / 2
        s.circle(cx, cy, r, ground, rough=0.25, n=64)
        for i in range(60):
            a = math.pi / 2 - 2 * math.pi * i / 60
            big = i % 5 == 0
            r0, r1 = r * (0.80 if big else 0.88), r * 0.94
            hw = r * (0.035 if big else 0.012)
            ca, sa = math.cos(a), math.sin(a)
            s.poly([(cx + r0 * ca - hw * sa, cy + r0 * sa + hw * ca), (cx + r1 * ca - hw * sa, cy + r1 * sa + hw * ca),
                    (cx + r1 * ca + hw * sa, cy + r1 * sa - hw * ca), (cx + r0 * ca + hw * sa, cy + r0 * sa - hw * ca)], ink, metal=0.6 if big else 0.0,
                   rough=0.3)
        s.mid(brand, cx, cy + r * 0.38, r * 0.14, r * 0.9, ink, 'Black', tracking=1.2)
        s.mid(sub, cx, cy - r * 0.42, r * 0.07, r * 0.8, ink, 'Bold', tracking=1.2)
        for ang, ln, wd in ((math.pi / 2 - 2 * math.pi * (10 + 10 / 60) / 12, 0.52, 0.055), (math.pi / 2 - 2 * math.pi * 10 / 60, 0.78, 0.04)):
            ca, sa = math.cos(ang), math.sin(ang)
            s.poly([(cx - r * wd * sa, cy + r * wd * ca), (cx + r * ln * ca, cy + r * ln * sa), (cx + r * wd * sa, cy - r * wd * ca),
                    (cx - r * 0.1 * ca, cy - r * 0.1 * sa)], 0xd8d8d0 if ground != 0xf2efe6 else 0x161616, metal=1.0, rough=0.25)
        s.circle(cx, cy, r * 0.05, 0xd8d8d0, metal=1.0, rough=0.25)
    return draw


def p_jtag(seed):
    """A jeweler's barbell tag: a tiny white card with the stock number and a karat stamp."""
    def draw(s, w, h):
        rnd = random.Random(seed)
        s.rect(0, 0, w, h, 0xf4f2ec, rough=0.6)
        s.mid(ticket_no(seed)[-4:], w / 2, h * 0.65, h * 0.26, w * 0.9, 0x161616, 'Black')
        s.mid(rnd.choice(['10K', '14K', '14K', '18K', '925', 'PLAT']), w / 2, h * 0.25, h * 0.22, w * 0.8, 0xa3161c, 'Black')
    return draw


def p_earrings(seed):
    def draw(s, w, h):
        s.rect(0, 0, w, h, 0x141414, rough=0.6)
        s.rect(w * 0.06, h * 0.06, w * 0.88, h * 0.88, 0x1a1a1c, rough=0.6)
        s.mid('14K', w / 2, h * 0.14, h * 0.07, w * 0.4, 0xc8a050, 'Black', metal=1.0)
    return draw


def draw_lean(s, w, h):
    s.rect(0, 0, w, h, 0xb3141c, rough=0.35)
    s.mid('PLEASE DO NOT LEAN ON GLASS', w / 2, h / 2, h * 0.5, w * 0.92, 0xffffff, 'Black')


def draw_register_keys(s, w, h):
    s.rect(0, 0, w, h, 0x2b2d31, rough=0.45)
    labels = [['CLK', 'VOID', 'RF', 'NS', 'PO', 'RA'], ['7', '8', '9', 'GOLD', 'LOAN', 'SUBT'], ['4', '5', '6', 'JEWL', 'PMT', 'CHK'],
              ['1', '2', '3', 'MUSC', 'TOOL', 'CARD'], ['0', '00', '.', 'ELEC', 'X', 'CASH']]
    kw, kh = (w - 14) / 6, (h - 14) / 5
    for r, row in enumerate(labels):
        for c, lab in enumerate(row):
            x0 = 7 + c * kw
            y0 = h - 7 - (r + 1) * kh
            col = 0xc4161c if lab == 'CASH' else (0x2f6fb5 if lab in ('GOLD', 'LOAN', 'JEWL', 'PMT', 'MUSC', 'TOOL', 'ELEC') else
                                                  (0xe6e2d6 if lab.isdigit() or lab in ('00', '.') else 0x8d9196))
            s.rect(x0 + 1.5, y0 + 1.5, kw - 3, kh - 3, col, rough=0.35)
            s.mid(lab, x0 + kw / 2, y0 + kh / 2, kh * 0.32, kw - 8, 0x141414 if col in (0xe6e2d6, 0x8d9196) else 0xffffff, 'Black')


def p_vfd(lines, ink=0x6effc8, ground=0x041410):
    def draw(s, w, h):
        s.rect(0, 0, w, h, ground, rough=0.06)
        n = len(lines)
        for i, (a, b) in enumerate(lines):
            y = h - (i + 0.5) * h / n
            s.mid(a, 4, y, h / n * 0.55, w * 0.55, ink, 'Bold', align='LEFT', rough=0.06)
            s.mid(b, w - 4, y, h / n * 0.55, w * 0.45, ink, 'Bold', align='RIGHT', rough=0.06)
    return draw


def draw_ticket_pad(s, w, h):
    """The top sheet of a pad of pawn tickets: the two-part form."""
    s.rect(0, 0, w, h, 0xf2ead2, rough=0.8)
    s.mid('NORTHSIDE PAWN & LOAN', w / 2, h * 0.93, h * 0.035, w * 0.9, 0x2a3a6a, 'Black')
    s.mid('232 MARKET ST  ·  NORTHSIDE  ·  555-0132', w / 2, h * 0.885, h * 0.022, w * 0.9, 0x2a3a6a, 'Bold')
    s.mid('PAWN TICKET', w / 2, h * 0.83, h * 0.04, w * 0.6, 0xa3161c, 'Black')
    s.text('No. ' + ticket_no(77), w * 0.94, h * 0.79, h * 0.03, 0xa3161c, 'Black', align='RIGHT')
    rows = ['DATE', 'NAME', 'ADDRESS', 'ID TYPE / No.', 'DESCRIPTION', '', '', 'SERIAL No.', 'AMOUNT LOANED', 'FINANCE CHARGE', 'TOTAL TO REDEEM',
            'MATURITY DATE']
    for i, r in enumerate(rows):
        y = h * (0.74 - i * 0.052)
        s.rect(w * 0.06, y, w * 0.88, 0.4, 0x7a8aa8)
        s.text(r, w * 0.07, y + 1.0, h * 0.017, 0x2a3a6a, 'Bold')
    s.rect(0, h * 0.09, w, 0.5, 0x9a9a9a)
    for i in range(30):
        s.circle(w * (i + 0.5) / 30, h * 0.09, 0.6, 0xb8b0a0)
    s.mid('PLEDGOR SIGNATURE', w * 0.3, h * 0.04, h * 0.018, w * 0.4, 0x2a3a6a, 'Bold')


def register(k, M, slot=None, keys='reg_keys', clerk='reg_clerk', cust='reg_cust'):
    """A cash register: the drawer base, the sloped keyboard toward the staff side (+y), the clerk's and the customer's
    displays, the receipt printer. Built with the customer at -y."""
    mk = k.mark()
    k.add(bx('reg_base', -0.20, 0.20, -0.20, 0.22, 0.0, 0.10, 0.008, 3), 'plastic', 0x2d2f33, slot=slot)
    k.add(bx('reg_drawer', -0.18, 0.18, 0.218, 0.228, 0.006, 0.092, 0.004), 'plastic', 0x3a3c40, slot=slot)
    k.add(parts.rod('reg_lock', (0.14, 0.228, 0.07), (0.14, 0.232, 0.07), 0.008, 16), 'chrome', slot=slot)
    body = core.prism_x('reg_body', [(-0.20, 0.10), (0.22, 0.10), (0.22, 0.12), (0.02, 0.165), (-0.08, 0.19), (-0.20, 0.19)], -0.19, 0.19)
    k.add(body, 'plastic', 0x34363a, slot=slot)
    va = Vector((0.0, 0.02 - 0.22, 0.165 - 0.12))
    k.decal(keys, (0.0, 0.12, 0.1425), u=(-1, 0, 0), v=tuple(va.normalized()), lift=0.0005)
    k.add(bx('pod', -0.15, -0.03, -0.075, -0.02, 0.168, 0.235, 0.006, 3), 'plastic', 0x2b2d31, slot=slot)
    k.decal(clerk, (-0.09, -0.02, 0.21), u=(-1, 0, 0), v=(0, 0, 1), lift=0.0004)
    k.add(parts.rod('pole', (0.13, -0.15, 0.19), (0.13, -0.15, 0.29), 0.011, 16), 'plastic', 0x2b2d31, slot=slot)
    k.add(bx('cust_head', 0.06, 0.20, -0.172, -0.128, 0.28, 0.34, 0.008, 3), 'plastic', 0x2b2d31, slot=slot)
    k.decal(cust, (0.13, -0.172, 0.31), u=(1, 0, 0), v=(0, 0, 1), lift=0.0004)
    k.add(bx('printer', 0.03, 0.17, -0.19, -0.07, 0.19, 0.215, 0.008, 3), 'plastic', 0x45474b, slot=slot)
    k.place(mk, M)


def bracelet_bar(k, M, n=3, seed=0, slot=None):
    """A velvet T-bar with bracelets draped over it."""
    rnd = random.Random(seed)
    mk = k.mark()
    k.add(bx('tbar_foot', -0.06, 0.06, -0.05, 0.05, 0.0, 0.012, 0.004), 'velvet', 0x141414, slot=slot)
    k.add(parts.rod('tbar_post', (0.0, 0.0, 0.012), (0.0, 0.0, 0.16), 0.008, 12), 'velvet', 0x141414, slot=slot)
    k.add(parts.rod('tbar', (-0.11, 0.0, 0.17), (0.11, 0.0, 0.17), 0.012, 16), 'velvet', 0x141414, slot=slot)
    for i in range(n):
        x = -0.08 + i * 0.16 / max(n - 1, 1)
        pts = [(x, 0.0, 0.182)]
        for t in range(1, 12):
            a = math.pi * t / 12
            pts.append((x + 0.004 * math.sin(a * 3), 0.032 * math.sin(a) * (1 if t < 6 else 1), 0.182 - 0.07 * (1 - math.cos(a)) / 2 * 1.6))
        pts2 = [(p[0], -p[1], p[2]) for p in reversed(pts)]
        path = pts + pts2[1:]
        chain_links(k, path, link=0.0065, wire=0.0016, slot=slot, col=rnd.choice([0xf0c35a, 0xe8b04a, 0xe0e0e0]), aux=(0.1, 0, 0))
    k.place(mk, M)


def earring_card(k, M, key, seed=0, slot=None, col=0xf0c35a):
    """A black card on a little easel with a pair of studs or hoops."""
    rnd = random.Random(seed)
    mk = k.mark()
    a = math.radians(15.0)
    k.add(quad('ec_card', [(-0.025, 0.0, 0.0), (0.025, 0.0, 0.0), (0.025, math.sin(a) * 0.065, math.cos(a) * 0.065),
                           (-0.025, math.sin(a) * 0.065, math.cos(a) * 0.065)]), 'paper', 0x161616, slot=slot)
    k.decal(key, (0.0, math.sin(a) * 0.0325 - 0.0004, math.cos(a) * 0.0325), u=(1, 0, 0), v=(0, math.sin(a), math.cos(a)), lift=0.0, w=0.05, h=0.065)
    hoop = rnd.random() < 0.5
    for sx in (-1, 1):
        c = Vector((sx * 0.012, math.sin(a) * 0.04 - 0.003, math.cos(a) * 0.04))
        if hoop:
            o = torus('hoop', (0, 0, 0), 0.008, 0.0012, 'y', 16, 4)
            o.data.transform(T(*c) @ RX(-15.0))
            k.add(o, 'gold', col, slot=slot)
        else:
            k.add(lathe_axis('stud', [(0.0, -0.002), (0.0028, 0.0), (0.0016, 0.0018), (0.0, 0.002)], T(*c) @ RX(75.0), 8), 'gem',
                  rnd.choice([0xf2f6ff, 0xa0142a, 0x1a3aa0]), slot=slot)
    k.place(mk, M)


def coin(k, center, r=0.016, col=0xf0c35a, slot=None):
    k.add(lathe_axis('coin', [(0.0, 0.0), (r, 0.0), (r, 0.0025), (0.0, 0.0025)], T(*center), 24), 'gold', col, slot=slot, aux=(0.2, 0, 0))
    k.add(torus('coin_rim', (center[0], center[1], center[2] + 0.0025), r * 0.92, 0.0006, 'z', 24, 3), 'gold', col, slot=slot)


def phone(k, M, col=0x141414, slot=None):
    """A phone on a little acrylic stand, its screen dark."""
    mk = k.mark()
    a = math.radians(18.0)
    M0 = T(0.0, 0.0, 0.008) @ RX(-a * 180 / math.pi)
    o = bx('phone', -0.036, 0.036, -0.004, 0.004, 0.0, 0.15, 0.004)
    o.data.transform(M0)
    k.add(o, 'gloss', col, slot=slot)
    s = bx('phone_screen', -0.033, 0.033, -0.0042, -0.0039, 0.006, 0.144)
    s.data.transform(M0)
    k.add(s, 'screen', slot=slot)
    k.add(bx('phone_stand', -0.03, 0.03, -0.02, 0.04, 0.0, 0.008, 0.003), 'gloss', 0xd8dcd8, slot=slot)
    k.place(mk, M)


# ================================================================== SM_NS_Pawn_Cage (+ _Glass): the loan window, the cage and the office behind it


def f_bag(c):
    """A milky plastic bag round a pledge: what's inside shows faintly through it, crinkles."""
    m = c.m
    vor = m.node('ShaderNodeTexVoronoi', Scale=45.0)
    vor.feature = 'DISTANCE_TO_EDGE'
    m.link(c.P, vor.inputs['Vector'])
    crease = ramp(m, vor.outputs['Distance'], 0.04, 0.0)
    band = noise(m, scaled(m, c.sep, 3.0, 3.0, 8.0), detail=2.0)
    inside = m.mix(ramp(m, band, 0.35, 0.65), c.col, darken(m, c.col, 0.4))
    col = m.mix(0.45, H(0xd8dde2), inside)
    col = m.mix(mul(m, crease, 0.4), col, H(0xf2f4f6))
    return col, add(m, 0.22, mul(m, crease, -0.08), mul(m, band, 0.1)), 0.0, add(m, mul(m, crease, 0.25), mul(m, band, 0.3))


FIN['bag'] = len(FIN) + 1
FINISH['bag'] = f_bag


def draw_cctv(s, w, h):
    """A four-way CCTV picture: the door, the cases, the guitars, the cage (grey, grainy, time-stamped)."""
    s.rect(0, 0, w, h, 0x0a0a0a)
    rnd = random.Random(8)
    for qx in range(2):
        for qy in range(2):
            x0, y0, qw, qh = qx * w / 2 + 2, qy * h / 2 + 2, w / 2 - 4, h / 2 - 4
            s.rect(x0, y0, qw, qh, 0x3a3e3a)
            s.poly([(x0, y0), (x0 + qw, y0), (x0 + qw * 0.75, y0 + qh * 0.45), (x0 + qw * 0.25, y0 + qh * 0.45)], 0x5a5e58)
            for _ in range(6):
                s.rect(x0 + rnd.uniform(0, qw * 0.8), y0 + rnd.uniform(qh * 0.1, qh * 0.7), rnd.uniform(4, 18), rnd.uniform(6, 30), rnd.choice([0x2a2c2a,
                                                                                                                                                0x6a6e68]))
            s.text(f'CAM {qy * 2 + qx + 1}', x0 + 4, y0 + qh - 10, 7, 0xe8e8e8, 'Bold')
    s.text('10-05  01:12:44', w - 6, 6, 8, 0xe8e8e8, 'Bold', align='RIGHT')


def p_box_label(text, seed):
    def draw(s, w, h):
        s.rect(0, 0, w, h, 0xf2eee0, rough=0.8)
        s.rect(0, 0, w, h * 0.12, 0xd8c890, rough=0.8)
        size = min(h * 0.35, w * 0.85 / max(hand_w(text, 1.0), 1e-3))
        hand(s, text, w * 0.06, h * 0.42, size, 0x141414, width=0.11, seed=seed)
        s.text('No. ' + ticket_no(seed), w * 0.94, h * 0.08, h * 0.12, 0xa3161c, 'Black', align='RIGHT')
    return draw


def mesh_panel(k, plane, a0, a1, z0, z1, fixed, pitch=0.05, r=0.0022, col=0x1a1a1a, slot=None):
    """A welded wire mesh panel in a plane x = fixed (plane 'x', spanning y a0..a1) or y = fixed (plane 'y', spanning x)."""
    n_a = max(int((a1 - a0) / pitch), 1)
    n_z = max(int((z1 - z0) / pitch), 1)
    objs = []
    for i in range(n_a + 1):
        a = a0 + (a1 - a0) * i / n_a
        p, q = ((fixed, a, z0), (fixed, a, z1)) if plane == 'x' else ((a, fixed, z0), (a, fixed, z1))
        objs.append((p, q))
    for j in range(n_z + 1):
        z = z0 + (z1 - z0) * j / n_z
        off = r * 1.6
        p, q = ((fixed - off, a0, z), (fixed - off, a1, z)) if plane == 'x' else ((a0, fixed - off, z), (a1, fixed - off, z))
        objs.append((p, q))
    o = wires('mesh', objs, r)
    k.add(o, 'wire', col, slot=slot)
    return o


def paper_sack(k, center, size, seed, tag=None, slot=None, facing='-x'):
    """A pledge in a kraft paper sack (center: its bottom's middle on the shelf; size: x, y, z extents): the sack closed to
    a wedge at the top, the top rolled down twice and stapled, the ticket stapled to the face toward `facing` ('-x' or
    '+y'); crumpled a little, leaning a little."""
    cx, cy, cz = center
    W, D, Hh = size
    rnd = random.Random(seed * 7 + 3)
    along_y = facing == '-x'                       # the roll runs along the face: y for a '-x' face, x for a '+y' one
    deep = W if along_y else D                     # the sack's depth across the roll
    rr = max(0.010, min(0.016, deep * 0.06))
    hb = Hh - 1.4 * rr                             # the body's top (the roll sits on it)
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    bmesh.ops.subdivide_edges(bm, edges=bm.edges[:], cuts=3, use_grid_fill=True)
    P = np.array([v.co[:] for v in bm.verts])
    n = texmaps.value_noise(P * 2.2 + seed * 0.37, 1.0, seed=seed, octaves=2) - 0.5
    lean = rnd.uniform(-0.015, 0.015)
    close = 1.0 - 2.0 * rr * 0.85 / deep           # how far the depth closes up at the top

    def depth_scale(t):
        s = min(1.0, max(0.0, (t - 0.62) / 0.38))
        return 1.0 - close * s * s * (3.0 - 2.0 * s)

    for v, a in zip(bm.verts, n):
        x, y, z = v.co
        t = z + 0.5
        ds = depth_scale(t)
        sx = W * (ds if along_y else 1.0) * (1.0 - 0.04 * t)
        sy = D * (1.0 if along_y else ds) * (1.0 - 0.04 * t)
        d = Vector((x, y, 0.0))
        amp = 0.010 * min(1.0, t * 3.0)
        if (along_y and x < -0.49) or (not along_y and y > 0.49):
            amp *= 0.2                             # the ticket's face stays nearly flat under the staple
        push = d.normalized() * (a * amp) if d.length > 1e-6 else Vector()
        v.co = Vector((cx + x * sx + push.x + (lean * t if along_y else 0.0), cy + y * sy + push.y + (0.0 if along_y else -lean * t), cz + t * hb))
    o = core.mesh_object('sack', bm)
    core.orient_normals(o)
    col = rnd.choice([0xa8845a, 0xb08a5a, 0x9c7a50, 0xb89466])
    k.add(o, 'paper', col, slot=slot)
    # the roll along the top edge, flattened a little, and two staples through it
    span = (D if along_y else W) * 0.96 / 2
    zr = cz + hb + rr * 0.55
    if along_y:
        rc = (cx + lean, cy)
        a_, b_ = (rc[0], cy - span, zr), (rc[0], cy + span, zr)
    else:
        rc = (cx, cy - lean)
        a_, b_ = (cx - span, rc[1], zr), (cx + span, rc[1], zr)
    roll = parts.rod('sack_roll', a_, b_, rr, 12)
    roll.data.transform(T(0, 0, zr) @ Matrix.Diagonal((1.0, 1.0, 0.75, 1.0)) @ T(0, 0, -zr))
    k.add(roll, 'paper', col, slot=slot)
    for s_ in (-0.5, 0.5):
        if along_y:
            p = (rc[0] - rr - 0.0004, cy + s_ * span, zr)
            k.add(bx('staple', p[0] - 0.0006, p[0] + 0.0004, p[1] - 0.006, p[1] + 0.006, p[2] - 0.0006, p[2] + 0.0006), 'steel', slot=slot)
        else:
            p = (cx + s_ * span, rc[1] + rr + 0.0004, zr)
            k.add(bx('staple', p[0] - 0.006, p[0] + 0.006, p[1] - 0.0004, p[1] + 0.0006, p[2] - 0.0006, p[2] + 0.0006), 'steel', slot=slot)
    if tag:
        zt = cz + max(0.05, hb * 0.45)
        t = (zt - cz) / hb
        if along_y:
            k.decal(tag, (cx - W / 2 * depth_scale(t) * (1.0 - 0.04 * t) + lean * t - 0.0022, cy, zt), u=(0, -1, 0), v=(0, 0, 1), lift=0.0,
                    w=0.035, h=0.06)
        else:
            k.decal(tag, (cx, cy + D / 2 * depth_scale(t) * (1.0 - 0.04 * t) - lean * t + 0.0022, zt), u=(-1, 0, 0), v=(0, 0, 1), lift=0.0,
                    w=0.035, h=0.06)


def pledge_bag(k, center, size, seed, inside=0x3a3a3a, tag=None, slot=None, facing='-x', subdiv=3):
    """A pledge in a knotted bag with its ticket stapled to the front (the face toward `facing`); about a third of them
    in kraft paper sacks instead."""
    if facing in ('-x', '+y') and random.Random(seed * 31 + 11).random() < 0.36:
        return paper_sack(k, center, size, seed, tag=tag, slot=slot, facing=facing)
    cx, cy, cz = center
    W, D, Hh = size
    hz = (Hh + 0.003) / 1.69            # its half height: the flattened bottom on the shelf, the gathered neck at Hh
    o = blob('bag', (cx, cy, cz - 0.003 + 0.45 * hz), (W, D, 2 * hz), seed=seed, detail=0.16, subdiv=subdiv, flatten=0.55, smooth=1, pinch=0.8)
    k.add(o, 'bag', inside, slot=slot)
    k.add(blob('knot', (cx, cy, cz + Hh + 0.015), (0.04, 0.035, 0.035), seed=seed + 1, detail=0.25, subdiv=1), 'bag', 0xd8dde2, slot=slot)
    if tag:
        if facing == '-x':
            k.decal(tag, (cx - W / 2 - 0.002, cy, cz + max(0.04, Hh * 0.27)), u=(0, -1, 0), v=(0, 0, 1), lift=0.0, w=0.035, h=0.06)
        elif facing == '+y':
            k.decal(tag, (cx, cy + D / 2 + 0.002, cz + max(0.04, Hh * 0.27)), u=(-1, 0, 0), v=(0, 0, 1), lift=0.0, w=0.035, h=0.06)
        else:
            k.decal(tag, (cx, cy - D / 2 - 0.002, cz + max(0.04, Hh * 0.27)), u=(1, 0, 0), v=(0, 0, 1), lift=0.0, w=0.035, h=0.06)


# ================================================================== SM_NS_Pawn_GuitarWall: guitars on the slatwall, amps under them

GUITAR_LOGOS = ['LARKIN', 'HARTWELL', 'COREY', 'HALLOW & SON', 'VANCE']


def draw_headlogo(text):
    def draw(s, w, h):
        s.rect(0, 0, w, h, 0x000000, alpha=0.0)
        s.mid(text, w / 2, h / 2, h * 0.7, w * 0.95, 0xf2efe6, 'Black', tracking=1.05)
    return draw


def hard_case(k, M, length=1.05, w=0.40, d=0.13, col=0x141414, slot=None):
    """A guitar's hard case standing on end: the body bout, the neck's narrowing, latches, a handle."""
    mk = k.mark()
    prof = [(-w / 2, 0.0), (w / 2, 0.0), (w / 2, 0.42), (w * 0.38, 0.52), (0.07, 0.62), (0.07, length - 0.05), (0.0, length), (-0.07, length - 0.05),
            (-0.07, 0.62), (-w * 0.38, 0.52), (-w / 2, 0.42)]
    k.add(xz_slab('case', closed_cr(prof, 3), -d / 2, d / 2, r=0.025, seg=2), 'tolex', col, slot=slot)
    k.add(xz_slab('case_seam', closed_cr([(x * 1.01, z) for x, z in prof], 3), -0.004, 0.004, r=0.002), 'chrome', slot=slot)
    for zz in (0.12, 0.30, 0.48):
        k.add(bx('latch', w / 2 - 0.004, w / 2 + 0.008, -0.02, 0.02, zz - 0.02, zz + 0.02, 0.002), 'chrome', slot=slot)
    k.add(tube('case_handle', bend([(w / 2, -0.04, 0.42), (w / 2 + 0.04, -0.04, 0.44), (w / 2 + 0.04, 0.04, 0.44), (w / 2, 0.04, 0.42)], 0.015), 0.01, 8),
          'leather', 0x111111, slot=slot)
    k.place(mk, M)


def stack(k, M, logo=None, slot=None):
    """A half stack: a 4x12 cabinet with a basket-weave grill and a head on top with its chrome panel and handle."""
    mk = k.mark()
    W, D, Hc = 0.76, 0.36, 0.76
    k.add(bx('cab', -W / 2, W / 2, -D / 2, D / 2, 0.02, Hc, 0.015, 3), 'tolex', 0x121212, slot=slot)
    k.add(bx('cab_grill', -W / 2 + 0.03, W / 2 - 0.03, -D / 2 - 0.001, -D / 2 + 0.01, 0.06, Hc - 0.04, 0.008), 'grill', 0x2a2620, slot=slot)
    for sx in (-1, 1):
        k.add(tube('cab_handle', bend([(sx * W / 2, -0.06, Hc * 0.6), (sx * (W / 2 + 0.03), -0.06, Hc * 0.62), (sx * (W / 2 + 0.03), 0.06, Hc * 0.62),
                                       (sx * W / 2, 0.06, Hc * 0.6)], 0.015), 0.01, 8), 'black_metal', slot=slot)
    for sx in (-1, 1):
        for sy in (-1, 1):
            k.add(parts.rod('caster', (sx * (W / 2 - 0.06), sy * (D / 2 - 0.06), 0.0), (sx * (W / 2 - 0.06), sy * (D / 2 - 0.06), 0.02), 0.02, 12),
                  'rubber', 0x111111, slot=slot)
    amp(k, T(0.0, 0.0, Hc) @ Matrix(), w=0.72, h=0.27, d=0.24, panel='front', grill=0x1c1c1c, slot=slot, knobs=8, logo=logo)
    k.place(mk, M)


# ================================================================== shelving and the electronics wall

def f_pegboard(c):
    """Perforated hardboard: a 1 in grid of 1/4 in holes on a painted or brown board, grubby round the hooks."""
    m = c.m
    u, v = face_uv(m, c)
    p = 0.0254
    du = sub(m, m.math('FRACT', m.math('DIVIDE', u, p)), 0.5)
    dv = sub(m, m.math('FRACT', m.math('DIVIDE', v, p)), 0.5)
    d = mul(m, m.math('SQRT', add(m, mul(m, du, du), mul(m, dv, dv))), p)
    hole = smooth_less(m, d, 0.0032, 0.0004)
    g = noise(m, c.P, scale=200.0, detail=2.0)
    col = m.mix(mul(m, g, 0.15), c.col, darken(m, c.col, 0.85))
    grub = ramp(m, noise(m, c.P, scale=6.0, detail=3.0), 0.5, 0.75)
    col = m.mix(mul(m, grub, 0.25), col, darken(m, c.col, 0.6))
    col = m.mix(hole, col, H(0x0c0b0a))
    return col, add(m, 0.7, mul(m, hole, 0.25)), 0.0, mul(m, hole, -0.5)


FIN['pegboard'] = len(FIN) + 1
FINISH['pegboard'] = f_pegboard


def p_spines(seed, n=40, h_frac=1.0):
    """A row of game cases' spines: colored bands, white title marks, a little logo block at the top."""
    def draw(s, w, h):
        rnd = random.Random(seed)
        x = 0.0
        while x < w - 2:
            sw = rnd.choice([14.0, 14.0, 14.0, 10.0, 7.0])
            col = rnd.choice([0x1a3aa0, 0x1a3aa0, 0x1a1a1a, 0xe8e8e8, 0x0a6a2a, 0xb3141c, 0x2a2a3a, 0x1c5a8a])
            s.rect(x, 0, sw - 0.6, h, col, rough=0.25)
            ink = 0xffffff if col not in (0xe8e8e8,) else 0x161616
            s.rect(x + 1.5, h * 0.82, sw - 3.6, h * 0.12, 0xf2f2f2 if col != 0xe8e8e8 else 0x1a3aa0, rough=0.3)
            for j in range(rnd.randint(2, 4)):
                s.rect(x + sw * 0.3, h * (0.2 + j * 0.13), sw * 0.3, h * rnd.uniform(0.06, 0.1), ink, rough=0.3)
            s.rect(x + sw - 0.6, 0, 0.6, h, 0x050505)
            x += sw
    return draw


def tablet(k, M, col=0x1a1a1a, slot=None):
    mk = k.mark()
    a = math.radians(15.0)
    o = bx('tab', -0.085, 0.085, -0.004, 0.004, 0.0, 0.24, 0.006)
    o.data.transform(T(0.0, 0.0, 0.01) @ RX(-15.0))
    k.add(o, 'alu' if col == 0xa8abb0 else 'gloss', col, slot=slot)
    s = bx('tab_scr', -0.078, 0.078, -0.0042, -0.0039, 0.008, 0.232)
    s.data.transform(T(0.0, 0.0, 0.01) @ RX(-15.0))
    k.add(s, 'screen', slot=slot)
    k.add(bx('tab_stand', -0.04, 0.04, -0.03, 0.06, 0.0, 0.01, 0.003), 'gloss', 0xd8dcd8, slot=slot)
    k.place(mk, M)


def headphones(k, M, col=0x1a1a1a, slot=None):
    """Headphones hanging from a peg hook (its tip at the origin)."""
    mk = k.mark()
    k.add(tube('hp_band', [(0.0, -0.085, -0.10), (0.0, -0.08, -0.02), (0.0, 0.0, 0.012), (0.0, 0.08, -0.02), (0.0, 0.085, -0.10)], 0.009, 10), 'plastic',
          col, slot=slot)
    for sy in (-1, 1):
        k.add(parts.rod('hp_cup', (-0.02, sy * 0.087, -0.13), (0.02, sy * 0.087, -0.13), 0.045, 24), 'plastic', col, slot=slot)
        k.add(torus('hp_pad', (0.0, sy * 0.072, -0.13), 0.036, 0.012, 'y', 20, 6), 'leather', 0x111111, slot=slot)
    k.place(mk, M)


def draw_tv_box(title, size):
    def draw(s, w, h):
        s.rect(0, 0, w, h, 0xe8e4dc, rough=0.6)
        s.rect(w * 0.06, h * 0.12, w * 0.88, h * 0.62, 0x101418, rough=0.3)
        s.poly([(w * 0.1, h * 0.16), (w * 0.9, h * 0.16), (w * 0.9, h * 0.5), (w * 0.4, h * 0.7), (w * 0.1, h * 0.4)], 0x1c5a8a, rough=0.3)
        s.mid(title, w * 0.3, h * 0.86, h * 0.07, w * 0.5, 0x161616, 'Black')
        s.mid(size, w * 0.82, h * 0.86, h * 0.09, w * 0.3, 0xb3141c, 'Black')
        s.mid('4K ULTRA HD  ·  SMART TV', w / 2, h * 0.06, h * 0.035, w * 0.8, 0x3a3a3a, 'Bold')
        for i in range(6):
            s.rect(w * (0.02 + i * 0.05), h * 0.02, w * 0.03, h * 0.02, 0x8a8a8a, alpha=0.5)
    return draw


def draw_rcv_disp(s, w, h):
    s.rect(0, 0, w, h, 0x050a0e, rough=0.05)
    s.mid('STANDBY', w * 0.5, h * 0.5, h * 0.4, w * 0.7, 0x1a3a5a, 'Bold')


def draw_just_in(s, w, h):
    s.rect(0, 0, w, h, 0xf2d21a, rough=0.5)
    s.rect(3, 3, w - 6, h - 6, 0x161616, rough=0.5)
    s.rect(6, 6, w - 12, h - 12, 0xf2d21a, rough=0.5)
    s.mid('JUST IN', w / 2, h * 0.64, h * 0.32, w * 0.86, 0x161616, 'Black')
    s.mid('STORAGE UNIT LOTS', w / 2, h * 0.28, h * 0.13, w * 0.84, 0xa3161c, 'Black')


# ================================================================== SM_NS_Pawn_Tools: the gondola island of power tools, the floor machines


def sander(k, M, brand='yellow', slot=None):
    c1, c2 = TOOL_COLORS[brand]
    mk = k.mark()
    k.add(lathe_axis('s_pad', [(0.0, 0.0), (0.063, 0.0), (0.063, 0.02), (0.0, 0.02)], Matrix(), 32), 'rubber', 0x2a2a2a, slot=slot)
    k.add(lathe_axis('s_body', [(0.0, 0.02), (0.06, 0.02), (0.05, 0.08), (0.035, 0.11), (0.0, 0.115)], Matrix(), 32), 'plastic', c1, slot=slot)
    k.add(parts.rod('s_bag', (0.03, 0.0, 0.07), (0.15, 0.0, 0.06), 0.02, 12), 'cloth', 0x2a2a2a, slot=slot)
    k.place(mk, M)


def tool_chest(k, M, slot=None):
    """A rolling tool chest: a top box with a lid on a cabinet of drawers, side handle, casters."""
    mk = k.mark()
    W, D = 0.66, 0.46
    k.add(bx('tc_cab', -W / 2, W / 2, -D / 2, D / 2, 0.10, 0.72, 0.008), 'powder', 0xb3141c, slot=slot)
    k.add(bx('tc_top', -W / 2, W / 2, -D / 2 + 0.02, D / 2, 0.72, 1.02, 0.008), 'powder', 0xb3141c, slot=slot)
    k.add(bx('tc_lid', -W / 2, W / 2, -D / 2 + 0.02, D / 2, 1.02, 1.08, 0.01), 'powder', 0xb3141c, slot=slot)
    for i, (z0, z1) in enumerate(((0.12, 0.26), (0.28, 0.38), (0.40, 0.50), (0.52, 0.60), (0.62, 0.70), (0.74, 0.82), (0.84, 0.92), (0.94, 1.0))):
        k.add(bx('tc_drawer', -W / 2 + 0.02, W / 2 - 0.02, -D / 2 - 0.006 + (0.02 if z0 > 0.72 else 0.0), -D / 2 + (0.02 if z0 > 0.72 else 0.0), z0, z1,
                 0.003), 'powder', 0xb3141c, slot=slot)
        k.add(bx('tc_pull', -W / 2 + 0.04, W / 2 - 0.04, -D / 2 - 0.016 + (0.02 if z0 > 0.72 else 0.0), -D / 2 - 0.006 + (0.02 if z0 > 0.72 else 0.0),
                 z1 - 0.025, z1 - 0.012, 0.003), 'alu', slot=slot)
    k.add(tube('tc_handle', bend([(W / 2, -0.15, 0.62), (W / 2 + 0.06, -0.15, 0.62), (W / 2 + 0.06, 0.15, 0.62), (W / 2, 0.15, 0.62)], 0.03), 0.012, 10),
          'chrome', slot=slot)
    for sx in (-1, 1):
        for sy in (-1, 1):
            k.add(parts.rod('tc_wheel', (sx * (W / 2 - 0.06) - 0.02, sy * (D / 2 - 0.06), 0.05), (sx * (W / 2 - 0.06) + 0.02, sy * (D / 2 - 0.06), 0.05),
                            0.05, 20), 'rubber', 0x1a1a1a, slot=slot)
    k.place(mk, M)


# ================================================================== SM_NS_Pawn_Door: the shop door's leaf (PROP: origin on its hinge line at the floor)


# ================================================================== SM_NS_Pawn_Grille: the roll-down grille (origin at the top middle of the opening)


# ================================================================== SM_NS_Pawn_Blade: the projecting double-faced sign (SITE)


# ================================================================== SM_NS_Pawn_Ticket: a pawn ticket in the hand (PROP: origin at the grip)


# ================================================================== SM_NS_Pawn_Front (+ _Glass): the storefront in M-S3's opening

FRAME_BLACK = 0x18191b                 # the storefront's black-enamel aluminum
OXBLOOD = 0x5c1c18                     # the bulkhead's glazed tile
HOURS_DECAL = ((GLASS_X - 0.003, 39.40, 1.52), 0.30, 0.20)   # the clear space left for the core-drawn hours decal (shop frame,
                                                             # on light 1's inside face, read from the street; w, h)
ITEM_SPOTS = {}                        # filled by the builders: marks published in the docstring (shop frame)


def front_frame_member(k, x0, x1, y0, y1, z0, z1, slot='frame'):
    """A storefront member: black enamel over aluminum, its edges rubbed to the metal."""
    k.add(bx('member', x0, x1, y0, y1, z0, z1, 0.003), 'enamel', FRAME_BLACK, slot=slot)


def draw_buzzer_plate(s, w, h):
    s.rect(0, 0, w, h, 0xa8aaa8, metal=1.0, rough=0.35)
    s.mid('BUZZER', w / 2, h * 0.82, h * 0.09, w * 0.8, 0x202020, 'Black', metal=1.0)
    s.mid('PUSH', w / 2, h * 0.16, h * 0.08, w * 0.6, 0x202020, 'Black', metal=1.0)


def front():
    k = Kit('pwf', 'SM_NS_Pawn_Front', slots=('frame', 'tile', 'display'), dust={'frame': 0.12, 'tile': 0.0, 'display': 0.35},
            low_grime={'frame': 0.45, 'tile': 0.35, 'display': 0.0}, cavity={'frame': 0.35, 'tile': 0.3, 'display': 0.3})
    k.panel('buzzer', 0.07, 0.11, draw_buzzer_plate, density=3.0)
    k.panel('sticker_cam', 0.16, 0.12, draw_sticker_camera, density=2.4)
    k.panel('logo_tv', 0.05, 0.007, logo_panel('NOVAVIEW', 0x141414, 0xb8b8b8, 'Bold'), density=3.0)
    for i, (desc, sd) in enumerate((('43in TV', 102), ('elec sunburst', 104))):
        k.panel(f'tag{sd}', 0.035, 0.06, p_tag(sd, desc), density=3.0)
    k.atlas.render()
    F, Tl, D = 'frame', 'tile', 'display'
    Y0, Y1 = OPEN_Y0, OPEN_Y1
    # ---- the grille's hood across the opening's head, the guides in the jambs
    k.add(bx('hood_face', 0.02, 0.028, Y0, Y1, HOOD_Z, OPEN_Z1, 0.002), 'paint', 0x222426)
    k.add(bx('hood_bottom', 0.02, 0.30, Y0, Y1, HOOD_Z, HOOD_Z + 0.006), 'paint', 0x222426)
    k.add(bx('hood_slot', 0.06, 0.12, Y0 + 0.1, Y1 - 0.1, HOOD_Z - 0.0005, HOOD_Z + 0.001), 'rubber', 0x050505)
    for yy in np.arange(Y0 + 1.36, Y1 - 0.5, 1.36):
        k.add(bx('hood_seam', 0.017, 0.021, yy - 0.003, yy + 0.003, HOOD_Z + 0.01, OPEN_Z1 - 0.01), 'paint', 0x161718)
        for zz in (HOOD_Z + 0.05, OPEN_Z1 - 0.05):
            for dy in (-0.03, 0.03):
                k.add(parts.rod('bolt', (0.02, yy + dy, zz), (0.014, yy + dy, zz), 0.005, 8), 'galv')
    for (ga, gb) in ((Y0, Y0 + 0.10), (Y1 - 0.10, Y1)):
        k.add(bx('guide_web', 0.115, 0.125, ga, gb, 0.0, HOOD_Z), 'galv')
        for yy in (ga, gb - 0.006):
            k.add(bx('guide_flange', 0.045, 0.125, yy, yy + 0.006, 0.0, HOOD_Z, 0.001), 'galv')
    # ---- the bulkhead under the display: oxblood glazed tile with a black bullnose cap, the sill
    tile = (0.108, 0.108, 0.0)
    k.add(bx('bulkhead', 0.10, 0.32, RETURN_Y - 0.04, Y1, 0.0, 0.50, 0.0), 'tile', OXBLOOD, slot=Tl, aux=tile)
    k.add(bx('bulk_cap', 0.088, 0.32, RETURN_Y - 0.052, Y1, 0.50, 0.535, 0.012, 3), 'tile', 0x121212, slot=Tl, aux=(0.152, 0.4, 0))
    k.add(bx('bulk_ret', 0.32, VEST_X - 0.06, RETURN_Y - 0.04, RETURN_Y + 0.10, 0.0, 0.50, 0.0), 'tile', OXBLOOD, slot=Tl, aux=tile)
    k.add(bx('bulk_ret_cap', 0.30, VEST_X - 0.06, RETURN_Y - 0.052, RETURN_Y + 0.112, 0.50, 0.535, 0.012, 3), 'tile', 0x121212, slot=Tl,
          aux=(0.152, 0.4, 0))
    front_frame_member(k, GLASS_X - 0.04, GLASS_X + 0.07, RETURN_Y, Y1, 0.535, 0.58)
    front_frame_member(k, GLASS_X, VEST_X - 0.05, RETURN_Y - 0.035, RETURN_Y + 0.035, 0.535, 0.58)
    # ---- the storefront: the corner post, two mullions and the east jamb; the transom bar; the head; the return's members
    for i, yy in enumerate(MULL_Y):
        w = 0.035 if i in (0, len(MULL_Y) - 1) else 0.0225
        a, b = (yy - w, yy + w) if i < len(MULL_Y) - 1 else (yy - 2 * w, yy)
        front_frame_member(k, GLASS_X - 0.04, GLASS_X + 0.07, a, b, 0.58, HOOD_Z)
    front_frame_member(k, GLASS_X - 0.04, GLASS_X + 0.07, RETURN_Y, Y1, TRANSOM_Z[0], TRANSOM_Z[1])
    front_frame_member(k, GLASS_X - 0.04, GLASS_X + 0.07, RETURN_Y - 0.035, Y1, HOOD_Z - 0.045, HOOD_Z)
    front_frame_member(k, GLASS_X, VEST_X - 0.05, RETURN_Y - 0.035, RETURN_Y + 0.035, TRANSOM_Z[0], TRANSOM_Z[1])
    front_frame_member(k, GLASS_X, VEST_X - 0.05, RETURN_Y - 0.035, RETURN_Y + 0.035, HOOD_Z - 0.045, HOOD_Z)
    # ---- the door's frame in the x = VEST_X plane: the west filler (the buzzer on it), the jambs (the electric strike in
    # the latch jamb), the head, the transom over the door with 232 in gold leaf, the threshold
    front_frame_member(k, VEST_X - 0.05, VEST_X + 0.06, OPEN_Y0, DOOR_Y[0], 0.0, HOOD_Z)
    front_frame_member(k, VEST_X - 0.05, VEST_X + 0.06, DOOR_Y[1], RETURN_Y + 0.035, 0.0, HOOD_Z)
    front_frame_member(k, VEST_X - 0.05, VEST_X + 0.06, DOOR_Y[0], DOOR_Y[1], DOOR_H + 0.015, DOOR_H + 0.075)
    front_frame_member(k, VEST_X - 0.05, VEST_X + 0.06, DOOR_Y[0], DOOR_Y[1], HOOD_Z - 0.045, HOOD_Z)
    k.add(bx('door_stop', VEST_X + 0.06, VEST_X + 0.075, DOOR_Y[0], DOOR_Y[1], DOOR_H, DOOR_H + 0.015), 'enamel', FRAME_BLACK)
    for (a, b) in ((DOOR_Y[0] - 0.015, DOOR_Y[0]), (DOOR_Y[1], DOOR_Y[1] + 0.015)):
        k.add(bx('jamb_stop', VEST_X + 0.06, VEST_X + 0.075, a, b, 0.0, DOOR_H), 'enamel', FRAME_BLACK)
    k.add(bx('threshold', VEST_X - 0.07, VEST_X + 0.09, DOOR_Y[0], DOOR_Y[1], 0.0, 0.012, 0.004), 'alu')
    # the electric strike: its body in the latch jamb's face, the faceplate, a conduit up the frame to the head
    sy = DOOR_Y[1] + 0.001
    k.add(bx('strike_body', VEST_X + 0.005, VEST_X + 0.05, sy, sy + 0.012, 0.95, 1.13, 0.002), 'steel')
    k.add(bx('strike_lip', VEST_X + 0.012, VEST_X + 0.038, sy - 0.0015, sy + 0.001, 1.00, 1.08, 0.0005), 'chrome')
    k.add(tube('strike_conduit', [(VEST_X + 0.075, DOOR_Y[1] + 0.03, 1.13), (VEST_X + 0.075, DOOR_Y[1] + 0.03, DOOR_H - 0.02),
                                  (VEST_X + 0.075, DOOR_Y[1] - 0.10, DOOR_H + 0.0)], 0.008, 8), 'galv')
    # the outside buzzer on the west filler, a camera sticker beside it
    k.add(bx('buzzer_plate', VEST_X - 0.055, VEST_X - 0.05, OPEN_Y0 + 0.08, OPEN_Y0 + 0.15, 1.16, 1.27, 0.002), 'steel')
    k.fdecal('buzzer', (VEST_X - 0.0555, OPEN_Y0 + 0.115, 1.215), '-x', w=0.07, h=0.11, lift=0.0002)
    k.add(parts.rod('buzzer_btn', (VEST_X - 0.0555, OPEN_Y0 + 0.115, 1.215), (VEST_X - 0.064, OPEN_Y0 + 0.115, 1.215), 0.011, 16), 'chrome')
    k.fdecal('sticker_cam', (VEST_X - 0.0505, OPEN_Y0 + 0.115, 1.48), '-x', w=0.12, h=0.09, lift=0.0002)
    Md = face_m((VEST_X + 0.002, (DOOR_Y[0] + DOOR_Y[1]) / 2, 2.82), '-x')
    k.add(flat_text('num_shade', '232', 0.22, Md @ T(0.006, -0.006, -0.0005), 'Black', tracking=1.06), 'paint', 0x060606)
    k.add(flat_text('num', '232', 0.22, Md, 'Black', tracking=1.06), 'gold', 0xe8c060, aux=(0.12, 0, 0))
    # ---- the vestibule: terrazzo with LOANS in brass, reading as you walk in; its tiled west side; the soffit and a camera
    vx0, vx1, vy0, vy1 = 0.0, VEST_X - 0.07, OPEN_Y0, RETURN_Y - 0.052
    k.add(bx('vest_floor', vx0, vx1, vy0, vy1, -0.02, 0.006, 0.0), 'terrazzo', 0x8c8a84, slot=Tl)
    for (a0, a1, b0, b1) in ((vx0 + 0.04, vx1 - 0.03, vy0 + 0.04, vy0 + 0.046), (vx0 + 0.04, vx1 - 0.03, vy1 - 0.046, vy1 - 0.04),
                             (vx0 + 0.04, vx0 + 0.046, vy0 + 0.04, vy1 - 0.04), (vx1 - 0.036, vx1 - 0.03, vy0 + 0.04, vy1 - 0.04)):
        k.add(bx('brass_strip', a0, a1, b0, b1, 0.0055, 0.0068), 'brass', slot=Tl)
    Mf = frame_matrix((0, -1, 0), (1, 0, 0), ((vx0 + vx1) / 2, (vy0 + vy1) / 2, 0.0066))
    k.add(flat_text('inlay', 'LOANS', 0.16, Mf @ T(0.0, -0.08, 0.0), 'Black', tracking=1.2, depth=0.0012, max_w=1.05), 'brass', slot=Tl)
    k.add(bx('vest_nosing', -0.02, 0.04, vy0, vy1, -0.02, 0.008, 0.004), 'steel', slot=Tl)
    k.add(bx('vest_wall_tile', 0.30, VEST_X - 0.05, OPEN_Y0, OPEN_Y0 + 0.02, 0.0, 0.50), 'tile', OXBLOOD, slot=Tl, aux=tile)
    k.add(bx('vest_wall_cap', 0.30, VEST_X - 0.05, OPEN_Y0, OPEN_Y0 + 0.032, 0.50, 0.535, 0.012), 'tile', 0x121212, slot=Tl, aux=(0.152, 0.4, 0))
    k.add(bx('vest_wall', 0.30, VEST_X - 0.05, OPEN_Y0, OPEN_Y0 + 0.012, 0.535, HOOD_Z - 0.01), 'enamel', FRAME_BLACK)
    k.add(bx('soffit', 0.28, VEST_X - 0.05, OPEN_Y0, RETURN_Y + 0.03, HOOD_Z - 0.01, HOOD_Z, 0.0), 'paint', 0x2a2826)
    k.add(lathe_axis('cctv', [(0.0, 0.0), (0.06, 0.0), (0.06, 0.012), (0.05, 0.02), (0.0, 0.021)], T(0.62, OPEN_Y0 + 0.30, HOOD_Z - 0.01) @ RX(180), 24),
          'plastic', 0xe8e6e0)
    k.add(lathe_axis('cctv_dome', [(0.0, 0.0), (0.045, 0.0)] + [(0.045 * math.cos(math.radians(a)), 0.045 * math.sin(math.radians(a))) for a in
                                                               range(15, 91, 15)], T(0.62, OPEN_Y0 + 0.30, HOOD_Z - 0.03) @ RX(180), 24), 'screen')
    # ---- the bars, inside the glass: flat end bars and three rails, round bars every 12 cm; and inside the return
    k.add(bx('bar_end_w', BARS_X - 0.008, BARS_X + 0.008, RETURN_Y + 0.04, RETURN_Y + 0.08, 0.535, TRANSOM_Z[0], 0.002), 'black_metal')
    k.add(bx('bar_end_e', BARS_X - 0.008, BARS_X + 0.008, Y1 - 0.12, Y1 - 0.08, 0.535, TRANSOM_Z[0], 0.002), 'black_metal')
    for zz in (0.64, 1.62, 2.62):
        k.add(bx('bar_rail', BARS_X - 0.005, BARS_X + 0.005, RETURN_Y + 0.04, Y1 - 0.08, zz - 0.022, zz + 0.022, 0.0015), 'black_metal')
        k.add(bx('ret_rail', GLASS_X + 0.08, VEST_X - 0.07, RETURN_Y + 0.055, RETURN_Y + 0.065, zz - 0.022, zz + 0.022, 0.0015), 'black_metal')
    yy = RETURN_Y + 0.17
    while yy < Y1 - 0.14:
        k.add(parts.rod('bar', (BARS_X, yy, 0.535), (BARS_X, yy, TRANSOM_Z[0]), 0.0085, 10), 'black_metal')
        yy += 0.12
    xx = GLASS_X + 0.16
    while xx < VEST_X - 0.1:
        k.add(parts.rod('ret_bar', (xx, RETURN_Y + 0.06, 0.535), (xx, RETURN_Y + 0.06, TRANSOM_Z[0]), 0.0085, 10), 'black_metal')
        xx += 0.12
    for zz in (0.64, 1.62, 2.62):
        for yb in (RETURN_Y + 0.06, Y1 - 0.10):
            k.add(parts.rod('rail_bolt', (BARS_X + 0.009, yb, zz), (BARS_X + 0.015, yb, zz), 0.008, 8), 'galv')
    # ---- vinyl lettering inside the display glass (§4.9): WE BUY GOLD · LOANS · GUITARS · ELECTRONICS, read from the
    # street (left to right there runs east to west: light 3, light 2, light 1)
    lights = [(MULL_Y[i], MULL_Y[i + 1]) for i in range(3)]
    for (a, b), (txt, cap, col) in zip(reversed(lights), (('WE BUY GOLD', 0.15, 0xe8b830), ('LOANS', 0.17, 0xf2efe6),
                                                          ('GUITARS · ELECTRONICS', 0.085, 0xf2efe6))):
        Mb = face_m((GLASS_X + 0.0025, (a + b) / 2, 2.30), '-x')
        k.add(flat_text('vinyl_shade', txt, cap, Mb @ T(0.004, -0.004, -0.0004), 'Black', tracking=1.1, max_w=b - a - 0.30), 'gloss', 0x111111)
        k.add(flat_text('vinyl', txt, cap, Mb, 'Black', tracking=1.1, max_w=b - a - 0.30), 'gloss', col)
    # ---- the display platform: a carpeted deck behind the bulkhead, a low lip at its back; on it the TV, a guitar on its
    # stand, gold chains on two velvet busts, watches on pillows on a riser (no legible prices: §4.6b)
    dx0, dx1 = DECK_X
    k.add(bx('deck', BARS_X + 0.02, dx1, RETURN_Y + 0.04, Y1 - 0.04, 0.0, SILL_Z - 0.008, 0.0), 'paint', 0x1a1a1a, slot=D)
    k.add(bx('carpet', BARS_X + 0.02, dx1, RETURN_Y + 0.04, Y1 - 0.04, SILL_Z - 0.008, SILL_Z, 0.003), 'felt', 0x3a1216, slot=D)
    k.add(bx('deck_lip', dx1 - 0.02, dx1 + 0.02, RETURN_Y + 0.04, Y1 - 0.04, SILL_Z, SILL_Z + 0.10, 0.004), 'wood', 0x3a2414, slot=D, aux=(2, 0, 0))
    k.add(bx('deck_face', dx1, dx1 + 0.01, RETURN_Y + 0.04, Y1 - 0.04, 0.0, SILL_Z, 0.0), 'paint', 0x2a2a2c, slot=D)
    turn = RZ(-90.0)                         # an item's front (-y) -> the street (-x)
    z = SILL_Z

    def at(x, y, rz=0.0, zz=z):
        return T(x, y, zz) @ turn @ RZ(rz)
    tv(k, at(0.86, 43.62, 4.0), w=0.95, h=0.54, slot=D, tag='tag102', logo='logo_tv')
    guitar_stand(k, at(0.74, 42.72), slot=D)
    guitar(k, at(0.74, 42.72, 0.0, z + 0.06) @ T(0.0, 0.02, 0.0) @ RX(-12.0), kind='single', color=0x8a2a10, burst=True, board='rose', guard=None,
           seed=11, slot=D, head='3+3', tag='tag104', hw='nickel')
    for i, (by_, bxx) in enumerate(((41.62, 0.88), (41.08, 0.70))):
        neck_bust(k, at(bxx, by_), h=0.42 - i * 0.06, slot=D)
        top = z + 0.42 - i * 0.06
        for j, (sag, col, link) in enumerate(((0.09, 0xf0c35a, 0.009), (0.14, 0xe8b04a, 0.0068), (0.19, 0xf2c860, 0.0115))[:3 - i]):
            nz = top - 0.02 - j * 0.004
            # (the bust faces the street: its chest toward -x)
            pts = bust_chain(bxx, by_, z, 0.42 - i * 0.06, nz, sag)
            chain_links(k, pts, link=link, wire=link * 0.24, slot=D, col=col)
    riser(k, at(0.62, 40.22), w=0.62, d=0.22, h=0.08, slot=D)
    WATCHES = [('gold', 0xf0c35a, 'bracelet'), ('chrome', 0xd8d8d8, 'bracelet'), ('chrome', 0xd0d0d0, 'leather'), ('gold', 0xe8c070, 'leather')]
    for i, (metal, col, strap) in enumerate(WATCHES):
        yy = 39.97 + i * 0.165
        pillow(k, at(0.62, yy, 0.0, z + 0.08), w=0.07, d=0.055, h=0.05, col=0xece6da if i % 2 else 0x1a1a1a, slot=D)
        watch(k, at(0.62, yy, 0.0, z + 0.115) @ RX(-20.0), metal=metal, col=col, strap=strap, slot=D)
    k.grime = [((0.6, 38.4, 0.02), 0.7, 0.8), ((VEST_X - 0.06, OPEN_Y0 + 0.115, 1.22), 0.08, 0.9), ((VEST_X, DOOR_Y[1] - 0.05, 1.05), 0.25, 0.7),
               ((0.1, 41.0, 0.0), 3.0, 0.3)]
    ITEM_SPOTS['HoursDecal'] = HOURS_DECAL
    return [k.finish(35.0), front_glass()]


def front_glass():
    """The shop glass: three display lights and their transoms, the return pane and its transom, the door's transom."""
    objs = []
    for i in range(3):
        a, b = MULL_Y[i] + (0.035 if i == 0 else 0.0225), MULL_Y[i + 1] - (0.0225 if i < 2 else 0.07)
        objs.append(quad('pane', [(GLASS_X, b, 0.58), (GLASS_X, a, 0.58), (GLASS_X, a, TRANSOM_Z[0]), (GLASS_X, b, TRANSOM_Z[0])]))
        objs.append(quad('transom', [(GLASS_X, b, TRANSOM_Z[1]), (GLASS_X, a, TRANSOM_Z[1]), (GLASS_X, a, HOOD_Z - 0.045), (GLASS_X, b, HOOD_Z - 0.045)]))
    for z0, z1 in ((0.58, TRANSOM_Z[0]), (TRANSOM_Z[1], HOOD_Z - 0.045)):
        objs.append(quad('ret', [(GLASS_X + 0.07, RETURN_Y, z0), (VEST_X - 0.05, RETURN_Y, z0), (VEST_X - 0.05, RETURN_Y, z1), (GLASS_X + 0.07, RETURN_Y, z1)]))
    objs.append(quad('door_transom', [(VEST_X, DOOR_Y[1], DOOR_H + 0.075), (VEST_X, DOOR_Y[0], DOOR_H + 0.075), (VEST_X, DOOR_Y[0], HOOD_Z - 0.045),
                                      (VEST_X, DOOR_Y[1], HOOD_Z - 0.045)]))
    return glass_mesh(objs, 'SM_NS_Pawn_Front_Glass')


def glass_mesh(objs, name, site=True):
    g = core.join(objs, name) if len(objs) > 1 else objs[0]
    g.name = g.data.name = name
    if site:
        g.data.transform(SITE_M)
    core.assign(g, UNB['ns_glass'])
    if not g.data.uv_layers:
        g.data.uv_layers.new(name='UVMap')
    core.uv_layout(g, [(lambda f: True, 'box', None, (0.0, 0.0, 1.0, 1.0))])
    print(f'[ns_pawn] {name}: {tri_count(g)} triangles, slots [ns_glass]')
    return g


# ================================================================== SM_NS_Pawn_Door: the shop door's leaf (PROP: origin on its hinge line at the floor)

DOOR_T = 0.045
DOOR_HINGE = (VEST_X + 0.055, DOOR_CLEAR[0])     # shop frame: the leaf's hinge axis (its inside face, on the west jamb)


def draw_wait_buzzer(s, w, h):
    s.rect(0, 0, w, h, 0xf2d21a, rough=0.35)
    s.rect(3, 3, w - 6, h - 6, 0x161616, rough=0.35)
    s.rect(5, 5, w - 10, h - 10, 0xf2d21a, rough=0.35)
    s.mid('PLEASE WAIT', w / 2, h * 0.66, h * 0.24, w * 0.86, 0x161616, 'Black')
    s.mid('FOR THE BUZZER', w / 2, h * 0.32, h * 0.18, w * 0.86, 0x161616, 'Black')


def door():
    """Built in its PROP frame (= the shop frame moved to the hinge): the leaf runs from the hinge toward +y, its outside
    face toward -x (the street). Steel stiles painted black, full glass, a push bar outside, a pull inside, the buzzer
    sticker, the OPEN/CLOSED sign hung inside the glass facing out ([screen])."""
    k = Kit('pwd', 'SM_NS_Pawn_Door', slots=('body',), dust=0.1, low_grime=0.3, site=False)
    k.panel('wait', 0.20, 0.09, draw_wait_buzzer, density=2.4)
    k.panel('cam', 0.12, 0.09, draw_sticker_camera, density=2.4)
    k.atlas.render()
    W = DOOR_CLEAR[1] - DOOR_CLEAR[0]
    x0, x1 = -DOOR_T, 0.0
    z0, z1 = 0.012, DOOR_H
    st, top, bot = 0.05, 0.07, 0.22
    for (a0, a1, b0, b1) in ((0.0, st, z0, z1), (W - st, W, z0, z1), (0.0, W, z1 - top, z1), (0.0, W, z0, z0 + bot)):
        k.add(bx('stile', x0, x1, a0, a1, b0, b1, 0.003), 'paint', 0x1a1b1c)
    for xx in (x0 + 0.008, x1 - 0.008):
        for (zz0, zz1) in ((z0 + bot, z0 + bot + 0.012), (z1 - top - 0.012, z1 - top)):
            k.add(bx('bead', xx - 0.004, xx + 0.004, st, W - st, zz0, zz1, 0.001), 'rubber', 0x111111)
    glass = quad('door_glass', [(-DOOR_T / 2, st, z0 + bot), (-DOOR_T / 2, W - st, z0 + bot), (-DOOR_T / 2, W - st, z1 - top), (-DOOR_T / 2, st, z1 - top)])
    k.special(glass, 'ns_glass')
    k.add(bx('kick_out', x0 - 0.0015, x0, 0.01, W - 0.01, z0 + 0.01, z0 + 0.20, 0.0005), 'steel')
    # outside: the push bar across the glass on two standoffs; inside: an offset pull on the latch stile
    for yb in (0.08, W - 0.08):
        k.add(bx('push_post', x0 - 0.06, x0, yb - 0.015, yb + 0.015, 1.02, 1.08, 0.006), 'steel')
    k.add(bx('push_bar', x0 - 0.065, x0 - 0.035, 0.06, W - 0.06, 1.03, 1.07, 0.012, 3), 'steel')
    py = W - st / 2
    k.add(tube('pull', bend([(x1, py, 0.85), (x1 + 0.07, py, 0.87), (x1 + 0.07, py, 1.25), (x1, py, 1.27)], 0.03), 0.014, 14), 'steel')
    k.add(parts.rod('cylinder', (x0, py, 1.36), (x0 - 0.008, py, 1.36), 0.016, 18), 'brass')
    k.add(bx('thumbturn', x1, x1 + 0.025, py - 0.004, py + 0.004, 1.34, 1.38, 0.002), 'brass')
    for zz in (0.25, 1.15, 2.00):
        k.add(parts.rod('hinge_knuckle', (0.002, 0.0, zz - 0.055), (0.002, 0.0, zz + 0.055), 0.008, 14), 'paint', 0x1a1b1c)
    xg = -DOOR_T / 2 - 0.0012
    k.fdecal('wait', (xg, W / 2, 1.30), '-x', lift=0.0)
    k.fdecal('cam', (xg, W - 0.16, 0.85), '-x', lift=0.0)
    # the OPEN / CLOSED sign hangs inside the glass on a chain from a suction hook, its face out ([screen], 0.30 x 0.13)
    sx = -DOOR_T / 2 + 0.012
    k.add(bx('sign_board', sx, sx + 0.006, W / 2 - 0.16, W / 2 + 0.16, 1.49, 1.635, 0.003), 'plastic', 0xe8e4da)
    k.screen('openclosed', 'ns_screen_pawn_openclosed', (sx - 0.0005, W / 2, 1.5625), '-x', 0.30, 0.13)
    for sy in (-0.12, 0.12):
        k.add(parts.rod('sign_chain', (sx + 0.003, W / 2 + sy, 1.635), (sx + 0.003, W / 2 + sy * 0.4, 1.74), 0.0012, 5), 'chrome')
    k.add(parts.rod('suction', (sx + 0.003, W / 2, 1.74), (-DOOR_T / 2 + 0.001, W / 2, 1.74), 0.012, 14), 'gloss', 0xd8dcd8)
    k.grime = [((x0 - 0.05, W / 2, 1.05), 0.4, 0.9), ((x1 + 0.05, py, 1.05), 0.2, 0.7), ((0.0, W / 2, 0.1), 0.6, 0.6)]
    k.polish = [((x0 - 0.05, W / 2, 1.05), 0.3, 0.6)]
    return [k.finish(40.0)]


# ================================================================== SM_NS_Pawn_Grille (PROP: origin at the top middle of the opening)

GRILLE_ORIGIN = (GRILLE_X, (OPEN_Y0 + OPEN_Y1) / 2, HOOD_Z)     # shop frame


def grille():
    """One link-grille curtain across the opening, hanging from the hood's slot to the sidewalk: 8 mm rods every 10 cm
    tied by flat links in a brick bond (the lit window and the blue tube show through it at night), a bottom bar with
    lift handles, two floor locks with padlocks. Its frame: the shop frame moved to the top middle (front toward -x);
    the game scales it in Z (1 down, 0 rolled up into the hood)."""
    k = Kit('pwr', 'SM_NS_Pawn_Grille', slots=('body',), dust=0.0, low_grime=0.3, site=False)
    gx = GRILLE_X
    ya, yb = OPEN_Y0 + 0.075, OPEN_Y1 - 0.075
    rods, spans = [], []
    zz = HOOD_Z - 0.02
    row = 0
    while zz > 0.12:
        rods.append(bx('grille_rod', gx - 0.004, gx + 0.004, ya, yb, zz - 0.004, zz + 0.004))
        if zz > 0.16:
            off = 0.15 if row % 2 else 0.0
            yy = ya + 0.08 + off
            while yy < yb - 0.04:
                spans.append((yy, zz - 0.10, zz))
                yy += 0.30
        zz -= 0.10
        row += 1
    k.add(core.join(rods, 'rods'), 'galv')
    # the links: flat bars seen edge-on from the street, each a three-sided prism (its ends hide in the rods)
    bm = bmesh.new()
    hw, d = 0.004, 0.014
    for (yy, z0, z1) in spans:
        v = [bm.verts.new(p) for z in (z0, z1) for p in ((gx - d / 2, yy - hw, z), (gx - d / 2, yy + hw, z), (gx + d / 2, yy, z))]
        for f in ((0, 3, 4, 1), (1, 4, 5, 2), (2, 5, 3, 0)):
            bm.faces.new([v[i] for i in f])
    k.add(core.mesh_object('links', bm), 'galv')
    k.add(bx('bottom_bar', gx - 0.025, gx + 0.025, ya - 0.04, yb + 0.04, 0.0, 0.10, 0.004), 'galv')
    k.add(bx('bottom_rubber', gx - 0.02, gx + 0.02, ya - 0.04, yb + 0.04, 0.0, 0.012, 0.002), 'rubber', 0x111111)
    for yh in ((ya + yb) / 2 - 1.2, (ya + yb) / 2 + 1.2):
        k.add(tube('lift_handle', bend([(gx - 0.025, yh - 0.06, 0.05), (gx - 0.07, yh - 0.06, 0.05), (gx - 0.07, yh + 0.06, 0.05),
                                        (gx - 0.025, yh + 0.06, 0.05)], 0.015), 0.007, 5), 'galv')
    for yl in (ya + 0.12, yb - 0.12):
        k.add(bx('floor_lock', gx - 0.035, gx + 0.035, yl - 0.03, yl + 0.03, 0.0, 0.07, 0.004), 'steel')
        k.add(tube('shackle', bend([(gx - 0.035, yl - 0.012, 0.03), (gx - 0.06, yl - 0.012, 0.03), (gx - 0.06, yl + 0.012, 0.03),
                                    (gx - 0.035, yl + 0.012, 0.03)], 0.008), 0.004, 4), 'chrome')
        k.add(bx('padlock', gx - 0.075, gx - 0.045, yl - 0.022, yl + 0.022, -0.01, 0.025, 0.006), 'brass')
    ox, oy, oz = GRILLE_ORIGIN
    for o in k.objs:
        o.data.transform(T(-ox, -oy, -oz))
    k.grime = [((0.0, 0.0, -oz), 2.0, 0.6)]
    return [k.finish(40.0)]


# ================================================================== SM_NS_Pawn_Sign (PROP: origin at the bracket's wall plate) + _GlowNeon, _GlowWarm

SIGN_PLATE = (0.0, (PLOT_Y0 + OPEN_Y0) / 2, 4.25)    # shop frame: the wall plate's middle on the west pier's face
SIGN_BALL_R = 0.10
SIGN_BALLS = [(-0.46, 3.86), (-0.78, 3.86), (-0.62, 3.62)]   # (x, z) in the shop frame, at the plate's y


def sign_neon():
    """The blue tube round the barred display window (between the glass and the bars), broken at the west foot for its
    electrodes. Returned in the shop frame."""
    loop = _rrect_path((TUBE_X, RETURN_Y + 0.10, 0.62), (TUBE_X, OPEN_Y1 - 0.10, 2.64), 0.07, 8)
    i0 = 2 * 9 + 5
    return [loop[i0:] + loop[:i0 - 1]]


def sign():
    """The pawnbroker's three gold balls on a scrolled iron bracket off the west pier, a gooseneck lamp over them, and the
    blue tube's electrodes and clear standoffs. Its frame: the shop frame moved to the wall plate (front toward -x, the
    street); its _GlowNeon (the tube) and _GlowWarm (the lamp's lens) share it."""
    k = Kit('pwn', 'SM_NS_Pawn_Sign', slots=('body',), dust=0.0, low_grime=0.0, site=False)
    px, py, pz = SIGN_PLATE
    k.add(bx('plate', -0.014, 0.0, py - 0.06, py + 0.06, pz - 0.22, pz + 0.14, 0.004), 'black_metal')
    for dy in (-0.035, 0.035):
        for dz in (-0.17, 0.09):
            k.add(parts.rod('anchor', (-0.014, py + dy, pz + dz), (-0.024, py + dy, pz + dz), 0.008, 8), 'galv')
    k.add(tube('arm', [(-0.012, py, pz), (-1.02, py, pz)], 0.013, 12), 'black_metal')
    k.add(tube('brace', bend([(-0.012, py, pz - 0.20), (-0.30, py, pz - 0.09), (-0.58, py, pz - 0.012)], 0.1), 0.009, 10), 'black_metal')
    for c0, sc in ((-0.13, 1.0), (-0.36, 0.7)):
        scroll = [(c0 - 0.08 * sc * math.cos(t) * (1 - t / 9), py, pz - 0.06 * sc - 0.06 * sc * math.sin(t) * (1 - t / 9)) for t in
                  [i * 0.35 for i in range(19)]]
        k.add(tube('scroll', scroll, 0.006, 8), 'black_metal')
    k.add(lathe_axis('arm_finial', [(0.0, 0.0), (0.018, 0.0), (0.022, 0.02), (0.012, 0.05), (0.0, 0.06)], T(-1.02, py, pz) @ RY(-90), 16), 'gold',
          0xe0b050, aux=(0.15, 0, 0))
    for (bxx, bzz) in SIGN_BALLS:
        k.add(parts.rod('ball_rod', (bxx, py, pz), (bxx, py, bzz + SIGN_BALL_R), 0.005, 8), 'black_metal')
        k.add(lathe_axis('ball_cap', [(0.0, 0.0), (0.025, 0.0), (0.018, 0.018), (0.0, 0.02)], T(bxx, py, bzz + SIGN_BALL_R - 0.008), 16), 'gold',
              0xd8a840, aux=(0.15, 0, 0))
        prof = [(0.0, -SIGN_BALL_R)] + [(SIGN_BALL_R * math.sin(math.radians(t)), -SIGN_BALL_R * math.cos(math.radians(t))) for t in range(12, 180, 12)] + \
            [(0.0, SIGN_BALL_R)]
        k.add(lathe_axis('ball', prof, T(bxx, py, bzz), 32), 'gold', 0xe8b850, aux=(0.06, 0, 0))
    # the gooseneck lamp over the balls (the game's spot light lives in it; its lens in _GlowWarm)
    lamp = [(-0.02, py, pz + 0.40), (-0.10, py, pz + 0.43), (-0.42, py, pz + 0.36), (-0.58, py, pz + 0.22)]
    k.add(tube('gooseneck', bend(lamp, 0.12), 0.011, 10), 'black_metal')
    k.add(bx('lamp_plate', -0.012, 0.0, py - 0.04, py + 0.04, pz + 0.34, pz + 0.46, 0.004), 'black_metal')
    k.add(lathe_axis('shade', [(0.0, 0.0), (0.024, 0.0), (0.028, 0.016), (0.078, 0.07), (0.08, 0.075), (0.0, 0.075)][::-1],
                     T(*SIGN_SHADE_AT) @ RY(180 + 25), 32), 'enamel', 0x1c3a2c)
    # the tube's hardware: electrodes, the clear standoffs to the window's frame (the glowing tube is the _GlowNeon)
    for s_ in sign_neon():
        neon_hardware(k, [s_], (1, 0, 0), frame=False)
        L = 0.0
        for a, b in zip(s_, s_[1:]):
            L += (Vector(b) - Vector(a)).length
            if L > 0.5:
                L = 0.0
                p = Vector(a)
                k.add(parts.rod('tube_mount', tuple(p), (GLASS_X + 0.07, p.y, p.z), 0.0025, 8), 'gloss', 0xd8dcd8)
    k.grime = [((-0.6, py, 3.6), 0.6, 0.3)]
    for o in k.objs:
        o.data.transform(T(-px, -py, -pz))
    return [k.finish(40.0)]


SIGN_SHADE_AT = (-0.62, SIGN_PLATE[1], SIGN_PLATE[2] + 0.21)


def sign_glow():
    """The tube (_GlowNeon) and the lamp's lens (_GlowWarm), in the sign's frame."""
    px, py, pz = SIGN_PLATE
    back = T(-px, -py, -pz)
    neon = [xform(o, back) for o in _sweep_all(sign_neon(), NEON_R)]
    sh = T(*SIGN_SHADE_AT) @ RY(180 + 25)
    lens = _lens('balls_lamp', tuple(sh @ Vector((0.0, 0.0, 0.062))), tuple((sh.to_3x3() @ Vector((0, 0, 1))).normalized()), 0.068)
    xform(lens, back)
    return {'SM_NS_Pawn_Sign_GlowNeon': neon, 'SM_NS_Pawn_Sign_GlowWarm': [lens]}


# ================================================================== SM_NS_Pawn_Shell (+ _GlowCool, SM_NS_Pawn_FanBlades)

WALL_CREAM = 0xd8cfb8
SITE_FLOOR = (37.40, 36.00, 44.60, 49.20)     # the floor's plan in Blender SITE XY (x0, y0, x1, y1): its texture covers it
# The wall of clocks on the back-stock wall over the counter: (y, z, radius, style). The two marked live have no hands
# baked: PawnStage draws their hands from the world time (§5.8).
CLOCKS = [(42.40, 3.05, 0.17, 'school', False), (42.82, 2.55, 0.13, 'wood', False), (43.18, 3.12, 0.22, 'sunburst', False),
          (43.30, 2.48, 0.155, 'school', True), (43.72, 3.02, 0.12, 'red', False), (43.98, 2.58, 0.19, 'roman', True),
          (44.30, 3.10, 0.11, 'octagon', False), (42.58, 2.12, 0.10, 'red', False), (44.32, 2.18, 0.12, 'school', False),
          (42.05, 2.62, 0.09, 'wood', False)]
CLOCK_X = PARTITION_X - 0.116                   # the clocks' fronts: their backs on the partition's face (PARTITION_X - 0.06)


def shell_floor_maps():
    """The floor's control image over its SITE plan: R where feet go (in from the door, down the guitar aisle to the loan
    window, to the counter, along the shelves; Augie's paths behind the counter and in the cage), G the wax built up along
    the walls and the fixtures' feet, B spills, gum and a rust ring."""
    x0, y0, x1, y1 = SITE_FLOOR
    W = 512
    Hh = int(round(W * (y1 - y0) / (x1 - x0)))
    u = (np.arange(W) + 0.5) / W
    v = (np.arange(Hh) + 0.5) / Hh
    U, V = np.meshgrid(u, v)
    X = x0 + U * (x1 - x0)
    Y = y0 + V * (y1 - y0)
    # into the shop frame (x depth, y lateral)
    P = np.stack([(FRONT_Y - Y).ravel(), X.ravel()], axis=1)
    paths = [
        ([(VEST_X, 38.40), (1.6, 38.55), (3.5, 38.70), (5.6, 38.65), (6.05, 38.60)], 0.40, 1.0),           # door -> the loan window
        ([(1.6, 38.55), (3.4, 39.8), (5.25, 41.2), (5.30, 43.6)], 0.38, 0.85),                              # -> along the counter
        ([(1.8, 43.2), (3.6, 43.45), (5.2, 43.5)], 0.30, 0.55),                                             # along the shelves
        ([(1.5, 38.5), (1.4, 40.5), (1.45, 43.0)], 0.25, 0.4),                                              # across the front
        ([(6.85, 41.4), (6.9, 43.0), (6.95, 44.2)], 0.28, 0.65),                                            # Augie behind the counter
        ([(6.95, 41.3), (7.1, 40.2), (6.95, 39.0), (7.6, 38.6)], 0.25, 0.6),                                # into the cage, the window
        ([(7.1, 41.6), (8.4, 41.6), (9.6, 41.6)], 0.25, 0.4),                                               # to the back stock
    ]
    traffic = np.zeros(len(P))
    for pts, width, wgt in paths:
        d, _ = texmaps.polyline_distance(P, pts)
        traffic = np.maximum(traffic, wgt * np.exp(-(d / width) ** 2))
    br = texmaps.value_noise(np.stack([P[:, 0] * 1.4, P[:, 1] * 1.4, np.zeros(len(P))], axis=1), 1.0, seed=7, octaves=3)
    traffic *= 0.55 + 0.6 * br
    dw = np.minimum.reduce([P[:, 1] - RY0, RY1 - P[:, 1], RX1 - P[:, 0], P[:, 0] - RX0])
    feet = []
    for (a0, a1, b0, b1) in ((COUNTER_X[0], COUNTER_X[1], FLAP_Y[0], RY1), (SHELF_X[0], SHELF_X[1], SHELF_Y[0], SHELF_Y[1]),
                             (DECK_X[0], DECK_X[1], RETURN_Y, OPEN_Y1), (CAGE_X - 0.04, CAGE_X + 0.04, RY0, CAGE_Y1),
                             (CAGE_X, RX1, CAGE_Y1 - 0.04, CAGE_Y1 + 0.04), (PARTITION_X - 0.07, PARTITION_X + 0.07, CAGE_Y1, RY1)):
        dx = np.maximum(np.maximum(a0 - P[:, 0], P[:, 0] - a1), 0.0)
        dy = np.maximum(np.maximum(b0 - P[:, 1], P[:, 1] - b1), 0.0)
        feet.append(np.hypot(dx, dy))
    df = np.minimum(dw, np.minimum.reduce(feet))
    wn = texmaps.value_noise(np.stack([P[:, 0] * 6, P[:, 1] * 6, np.zeros(len(P))], axis=1), 1.0, seed=11, octaves=2)
    wax = texmaps.smoothstep(0.22, 0.0, df) * (0.5 + 0.5 * wn) * (1.0 - 0.7 * np.clip(traffic, 0, 1))
    spill = np.zeros(len(P))
    rnd = random.Random(23)
    for cx, cy, r, a in ((3.30, 40.10, 0.12, 0.8), (5.42, 42.80, 0.10, 0.5), (1.40, 38.20, 0.05, 0.9), (1.75, 38.05, 0.04, 0.9),
                         (1.20, 38.70, 0.035, 1.0), (8.30, 43.90, 0.16, 0.45), (6.10, 38.75, 0.08, 0.6)):
        d = np.hypot(P[:, 0] - cx, P[:, 1] - cy)
        n = texmaps.value_noise(np.stack([P[:, 0] * 30 + cx, P[:, 1] * 30, np.zeros(len(P))], axis=1), 1.0, seed=int(cx * 10), octaves=2)
        spill = np.maximum(spill, a * texmaps.smoothstep(r * 1.1, r * 0.6, d + (n - 0.5) * r * 0.5))
    for i in range(24):
        t = i / 23
        cx = 1.2 + t * (5.9 - 1.2) + rnd.uniform(-0.05, 0.05)
        cy = 38.45 + rnd.uniform(-0.08, 0.08)
        d = np.hypot(P[:, 0] - cx, P[:, 1] - cy)
        spill = np.maximum(spill, 0.5 * texmaps.smoothstep(0.018, 0.008, d))
    for i in range(12):
        cx, cy = rnd.uniform(1.0, 2.6), rnd.uniform(37.6, 39.4)
        d = np.hypot(P[:, 0] - cx, P[:, 1] - cy)
        spill = np.maximum(spill, 1.0 * texmaps.smoothstep(0.012, 0.006, d))
    rgb = np.stack([traffic, wax, spill], axis=1).reshape(Hh, W, 3)
    return texmaps.save('T_NS_Pawn_FloorMask', rgb, 'Non-Color'), SITE_FLOOR


def shell_ceiling_maps():
    """The tin ceiling's stains over its SITE plan (R: 1 at a stain's heart, through 0.5 at its tide line)."""
    x0, y0, x1, y1 = SITE_FLOOR
    W = 256
    Hh = int(round(W * (y1 - y0) / (x1 - x0)))
    u = (np.arange(W) + 0.5) / W
    v = (np.arange(Hh) + 0.5) / Hh
    U, V = np.meshgrid(u, v)
    X = x0 + U * (x1 - x0)
    Y = y0 + V * (y1 - y0)
    P = np.stack([X.ravel(), Y.ravel(), np.zeros(X.size)], axis=1)
    out = np.zeros(X.size)
    for sx, sy, r in ((39.0, 46.6, 0.42), (43.4, 42.9, 0.30), (40.2, 40.1, 0.26), (44.0, 37.2, 0.34)):
        d = np.hypot(P[:, 0] - sx, P[:, 1] - sy)
        n = texmaps.value_noise(P * 6.0 + sx, 1.0, seed=int(sx * 7), octaves=3) - 0.5
        out = np.maximum(out, np.clip(1.0 - (d + n * r * 0.6) / (r * 2.0), 0.0, 1.0))
    rgb = np.stack([out, out, out], axis=1).reshape(Hh, W, 3)
    return texmaps.save('T_NS_Pawn_CeilStain', rgb, 'Non-Color'), SITE_FLOOR


def planar_site_uv(obj):
    """Lay obj's UVs (it is in the shop frame) over the SITE plan SITE_FLOOR: u along SITE x, v along SITE y."""
    x0, y0, x1, y1 = SITE_FLOOR
    me = obj.data
    uv = me.uv_layers.get('UVMap') or me.uv_layers.new(name='UVMap')
    for poly in me.polygons:
        for li in poly.loop_indices:
            co = me.vertices[me.loops[li].vertex_index].co
            sx, sy = co.y, FRONT_Y - co.x
            uv.data[li].uv = ((sx - x0) / (x1 - x0), (sy - y0) / (y1 - y0))
    set_attr(obj, 'pw_lock', 1.0)


def p_clock(style, seed, hands=True):
    """A clock's face. hands=False leaves them off (the game draws them)."""
    def draw(s, w, h):
        rnd = random.Random(seed)
        cx, cy, r = w / 2, h / 2, w / 2
        ground, ink = {'school': (0xf4f2ea, 0x161616), 'wood': (0xe8dcc0, 0x2a1a10), 'sunburst': (0x1a1a1a, 0xe0b050), 'red': (0xf4f2ea, 0xb3141c),
                       'roman': (0xf0e8d4, 0x1a1a1a), 'octagon': (0xece4cc, 0x2a1a10)}[style]
        s.circle(cx, cy, r, ground, rough=0.3, n=96)
        for i in range(60):
            a = math.pi / 2 - 2 * math.pi * i / 60
            big = i % 5 == 0
            r0 = r * (0.86 if big else 0.91)
            hw = r * (0.018 if big else 0.006)
            ca, sa = math.cos(a), math.sin(a)
            s.poly([(cx + r0 * ca - hw * sa, cy + r0 * sa + hw * ca), (cx + r * 0.95 * ca - hw * sa, cy + r * 0.95 * sa + hw * ca),
                    (cx + r * 0.95 * ca + hw * sa, cy + r * 0.95 * sa - hw * ca), (cx + r0 * ca + hw * sa, cy + r0 * sa - hw * ca)], ink)
        romans = ['XII', 'I', 'II', 'III', 'IIII', 'V', 'VI', 'VII', 'VIII', 'IX', 'X', 'XI']
        for i in range(12):
            a = math.pi / 2 - 2 * math.pi * i / 12
            label = romans[i] if style in ('roman', 'octagon') else str(12 if i == 0 else i)
            rt = r * 0.72
            size = r * (0.16 if style != 'roman' else 0.13)
            s.text(label, cx + rt * math.cos(a), cy + rt * math.sin(a) - size * 0.36, size, ink, 'Bold', align='CENTER')
        if style == 'school':
            s.mid('NORTHSIDE', cx, cy + r * 0.3, r * 0.07, r * 0.6, ink, 'Bold', tracking=1.3)
        if hands:
            hh, mm = rnd.randint(1, 12), rnd.randint(0, 59)
            for ang, ln, wd in ((math.pi / 2 - 2 * math.pi * (hh + mm / 60) / 12, 0.5, 0.035), (math.pi / 2 - 2 * math.pi * mm / 60, 0.75, 0.022)):
                ca, sa = math.cos(ang), math.sin(ang)
                s.poly([(cx - r * wd * sa, cy + r * wd * ca), (cx + r * ln * ca, cy + r * ln * sa), (cx + r * wd * sa, cy - r * wd * ca),
                        (cx - r * 0.12 * ca, cy - r * 0.12 * sa)], ink)
            if style in ('school', 'red'):
                ang = math.pi / 2 - 2 * math.pi * rnd.random()
                s.poly([(cx, cy), (cx + r * 0.82 * math.cos(ang), cy + r * 0.82 * math.sin(ang)),
                        (cx + r * 0.82 * math.cos(ang) + 0.6, cy + r * 0.82 * math.sin(ang) + 0.6)], 0xc8141c)
            s.circle(cx, cy, r * 0.035, ink)
    return draw


def clock(k, y, z, r, style, key, live):
    """A wall clock on the back-stock wall, its face toward -x: the case (a rim per style), the face (a print), a domed
    glass over it as a faint ring, the hands unless they're the game's."""
    X = CLOCK_X
    FR = RY(-90.0)       # lathe +Z -> -X
    if style == 'sunburst':
        k.add(lathe_axis('sb_hub', [(0.0, 0.0), (r * 0.62, 0.0), (r * 0.62, 0.03), (0.0, 0.035)], T(X + 0.04, y, z) @ FR, 40), 'gold', 0xd8a840,
              aux=(0.2, 0, 0))
        for i in range(24):
            a = 2 * math.pi * i / 24
            ln = r * (1.0 if i % 2 == 0 else 0.8)
            p0 = Vector((X + 0.035, y + r * 0.6 * math.cos(a), z + r * 0.6 * math.sin(a)))
            p1 = Vector((X + 0.05, y + (r * 0.6 + ln) * math.cos(a), z + (r * 0.6 + ln) * math.sin(a)))
            k.add(parts.rod('ray', tuple(p0), tuple(p1), 0.004, 6), 'gold', 0xd8a840, aux=(0.2, 0, 0))
        face_r = r * 0.55
        rim_x = X + 0.005
    elif style == 'octagon':
        oct_ = [(r * math.cos(math.pi / 8 + i * math.pi / 4), r * math.sin(math.pi / 8 + i * math.pi / 4)) for i in range(8)]
        k.add(yz_slab('oct', [(y + a, z + b) for a, b in oct_], X - 0.0, X + 0.05, r=0.008), 'wood', 0x5a3418, aux=(0, 0, 0))
        face_r = r * 0.78
        rim_x = X - 0.002
    else:
        rim_col = {'school': 0x161616, 'wood': 0x4a2a14, 'red': 0xb3141c, 'roman': 0x2a1a10}[style]
        fin = 'wood' if style in ('wood', 'roman') else ('enamel' if style == 'red' else 'plastic')
        prof = [(0.0, 0.0), (r, 0.0), (r * 1.0, 0.03), (r * 0.97, 0.05), (r * 0.9, 0.055), (r * 0.88, 0.035), (0.0, 0.035)]
        k.add(lathe_axis('clock_case', prof, T(X + 0.055, y, z) @ FR, 48), fin, rim_col)
        face_r = r * 0.86
        rim_x = X + 0.02
    k.fdecal(key, (rim_x - 0.0005, y, z), '-x', w=face_r * 2, h=face_r * 2, lift=0.0,
             outline=[(0.5 + 0.5 * math.cos(2 * math.pi * j / 48), 0.5 + 0.5 * math.sin(2 * math.pi * j / 48)) for j in range(48)])
    k.add(parts.rod('pin', (rim_x - 0.001, y, z), (rim_x - 0.008, y, z), face_r * 0.04, 10), 'brass')
    if live:
        ITEM_SPOTS.setdefault('LiveClocks', []).append(((rim_x - 0.004, y, z), face_r, 'faces -x (stage north)'))


def strip_fixture(k, x, y, slot='wall'):
    """An 8 ft two-lamp strip fixture hung on chains: the white channel, the tombstone lamp holders; its tubes glow
    (_GlowCool, from strip_tubes)."""
    z = STRIP_Z
    L = 2.44
    k.add(bx('strip_channel', x - L / 2, x + L / 2, y - 0.055, y + 0.055, z + 0.02, z + 0.07, 0.006), 'enamel', 0xece8e0, slot=slot)
    for sx in (-1, 1):
        k.add(bx('tombstone', x + sx * (L / 2 - 0.02) - 0.012, x + sx * (L / 2 - 0.02) + 0.012, y - 0.05, y + 0.05, z - 0.03, z + 0.02, 0.004), 'plastic',
              0xe8e4da, slot=slot)
        k.add(parts.rod('strip_chain', (x + sx * L * 0.35, y, z + 0.07), (x + sx * L * 0.35, y, CEIL), 0.0025, 6), 'chrome', slot=slot)
        k.add(parts.rod('strip_hook', (x + sx * L * 0.35, y, CEIL - 0.01), (x + sx * L * 0.35, y, CEIL), 0.012, 10), 'chrome', slot=slot)
    k.add(parts.rod('strip_cord', (x + L * 0.35 + 0.02, y, z + 0.07), (x + L * 0.35 + 0.02, y, CEIL), 0.0035, 6), 'rubber', 0xe8e4da, slot=slot)


STRIP_Z = 3.02


def strip_tubes():
    out = []
    for (x, y) in STRIPS:
        for dy in (-0.025, 0.025):
            out.append(parts.rod('tube', (x - 1.20, y + dy, STRIP_Z - 0.005), (x + 1.20, y + dy, STRIP_Z - 0.005), 0.0125, 12))
    return out


def draw_sign_board(lines, ground=0xf3f1ea, ink=INK, band=0xb3141c, band_ink=0xffffff):
    """A printed sign board: the first line in a colored band, the rest below."""
    def draw(s, w, h):
        s.rect(0, 0, w, h, ground, rough=0.5)
        s.rect(w * 0.02, h * 0.04, w * 0.96, h * 0.92, band, rough=0.5)
        s.rect(w * 0.035, h * 0.07, w * 0.93, h * 0.86, ground, rough=0.5)
        n = len(lines)
        if n == 1:
            s.mid(lines[0], w / 2, h / 2, h * 0.42, w * 0.86, ink, 'Black', tracking=1.08)
        else:
            s.rect(w * 0.035, h * 0.52, w * 0.93, h * 0.41, band, rough=0.5)
            s.mid(lines[0], w / 2, h * 0.725, h * 0.24, w * 0.86, band_ink, 'Black', tracking=1.08)
            s.mid(lines[1], w / 2, h * 0.30, h * 0.2, w * 0.86, ink, 'Black', tracking=1.05)
        photocopy(s, w, h, len(lines[0]), 0.3)
    return draw


def shell():
    k = Kit('pws', 'SM_NS_Pawn_Shell', slots=('floor', 'wall', 'ceil'), dust={'floor': 0.0, 'wall': 0.25, 'ceil': 0.0},
            low_grime={'floor': 0.0, 'wall': 0.0, 'ceil': 0.0}, cavity={'floor': 0.0, 'wall': 0.25, 'ceil': 0.2})
    k.default = 'wall'            # the floor and the tin keep their slots to their one plate each (laid over the plan)
    k.images['floor'] = shell_floor_maps()
    k.images['ceil'] = shell_ceiling_maps()
    k.panel('sales_final', 1.40, 0.36, draw_sign_board(['ALL SALES FINAL']), density=0.8)
    k.panel('sales_final_b', 1.40, 0.36, draw_sign_board(['ALL SALES FINAL']), density=0.8)
    k.panel('loans30', 0.86, 0.30, draw_sign_board(['30-DAY LOANS'], band=0x1c3a6e), density=1.0)
    k.panel('noid', 0.86, 0.30, draw_sign_board(['NO ID', 'NO LOAN'], band=0xb3141c), density=1.0)
    k.panel('ticket_req', 0.86, 0.30, draw_sign_board(['TICKET REQUIRED', 'TO REDEEM'], band=0x1c3a6e), density=1.0)
    k.panel('exit', 0.30, 0.12, draw_exit, glow=3.0, density=1.4)
    k.panel('staff', 0.30, 0.10, draw_staff, density=1.6)
    k.panel('ext_sign', 0.16, 0.24, draw_extinguisher_sign, density=1.4)
    k.panel('panel_label', 0.18, 0.06, draw_panel_label, density=2.0)
    for i, (y, z, r, style, live) in enumerate(CLOCKS):
        k.panel(f'clock{i}', 0.2, 0.2, p_clock(style, 900 + i, hands=not live), density=1.6 + r * 4)
    k.atlas.render()
    F, W_, C = 'floor', 'wall', 'ceil'
    # ---- the floor: VCT over the room, its texture laid over the plan
    fl = k.add(quad('floor', [(RX0, RY0, 0.0), (RX1, RY0, 0.0), (RX1, RY1, 0.0), (RX0, RY1, 0.0)][::-1]), 'vct', 0xb9b4a6, slot=F)
    core.orient_normals(fl, (0, 0, 1))
    planar_site_uv(fl)
    k.add(bx('mat', VEST_X + 0.08, VEST_X + 0.98, DOOR_Y[0] - 0.05, DOOR_Y[1] + 0.25, 0.0, 0.009, 0.004), 'mat', 0x26241f, slot=W_)
    # ---- the walls: painted-over brick, west (the guitar wall's back), east (behind the shelves), back; the front's inside
    # returns; the partition (the back-stock wall) with its cased opening
    def wall(name, corners):
        o = quad(name, corners)
        k.add(o, 'brickpaint', WALL_CREAM, slot=W_)
        return o
    wall('wall_w', [(VEST_X + 0.06, RY0, 0.0), (RX1, RY0, 0.0), (RX1, RY0, CEIL), (VEST_X + 0.06, RY0, CEIL)][::-1])
    wall('wall_w_ret', [(VEST_X + 0.06, OPEN_Y0, 0.0), (VEST_X + 0.06, RY0, 0.0), (VEST_X + 0.06, RY0, CEIL), (VEST_X + 0.06, OPEN_Y0, CEIL)][::-1])
    wall('wall_e', [(RX1, RY1, 0.0), (RX0, RY1, 0.0), (RX0, RY1, CEIL), (RX1, RY1, CEIL)][::-1])
    by0, by1 = BACKDOOR_Y
    wall('wall_b0', [(RX1, RY0, 0.0), (RX1, by0 - 0.05, 0.0), (RX1, by0 - 0.05, CEIL), (RX1, RY0, CEIL)][::-1])
    wall('wall_b1', [(RX1, by1 + 0.05, 0.0), (RX1, RY1, 0.0), (RX1, RY1, CEIL), (RX1, by1 + 0.05, CEIL)][::-1])
    wall('wall_b2', [(RX1, by0 - 0.05, 2.18), (RX1, by1 + 0.05, 2.18), (RX1, by1 + 0.05, CEIL), (RX1, by0 - 0.05, CEIL)][::-1])
    wall('wall_fe', [(RX0, OPEN_Y1, 0.0), (RX0, RY1, 0.0), (RX0, RY1, CEIL), (RX0, OPEN_Y1, CEIL)][::-1])
    wall('wall_fh', [(RX0, RY0, HOOD_Z), (RX0, RY1, HOOD_Z), (RX0, RY1, CEIL), (RX0, RY0, CEIL)][::-1])
    # face each wall into the room
    for o, n in zip(k.objs[-8:], ((0, 1, 0), (1, 0, 0), (0, -1, 0), (-1, 0, 0), (-1, 0, 0), (-1, 0, 0), (1, 0, 0), (1, 0, 0))):
        core.orient_normals(o, n)
    px0, px1 = PARTITION_X - 0.06, PARTITION_X + 0.06
    pa, pb = PARTITION_DOOR
    for (ya, yb, za, zb) in ((CAGE_Y1, pa, 0.0, CEIL), (pb, RY1, 0.0, CEIL), (pa, pb, 2.15, CEIL)):
        k.add(bx('partition', px0, px1, ya, yb, za, zb, 0.0), 'wallpaint', 0xd2c9b2, slot=W_)
    for (a0, a1, b0, b1) in ((pa - 0.07, pa, 0.0, 2.22), (pb, pb + 0.07, 0.0, 2.22), (pa - 0.07, pb + 0.07, 2.15, 2.22)):
        for xx in (px0 - 0.012, px1):
            k.add(bx('casing', xx, xx + 0.012, a0, a1, b0, b1, 0.003), 'paint', 0x6a5a48, slot=W_)
    # cove base round the room and the partition
    for (a0, a1, b0, b1) in ((VEST_X + 0.06, RX1, RY0, RY0 + 0.006), (RX0, RX1, RY1 - 0.006, RY1), (RX1 - 0.006, RX1, RY0, by0 - 0.05),
                             (RX1 - 0.006, RX1, by1 + 0.05, RY1), (px0 - 0.006, px0, CAGE_Y1, pa), (px0 - 0.006, px0, pb, RY1)):
        k.add(bx('cove', a0, a1, b0, b1, 0.0, 0.10, 0.002), 'rubber', 0x2a2826, slot=W_)
    # crown molding round the room at the tin
    for (a0, a1, b0, b1) in ((RX0, RX1, RY0, RY0 + 0.07), (RX0, RX1, RY1 - 0.07, RY1), (RX1 - 0.07, RX1, RY0, RY1), (RX0, RX0 + 0.07, RY0, RY1)):
        k.add(bx('crown', a0, a1, b0, b1, CEIL - 0.11, CEIL, 0.02, 3), 'paint', 0xe4dcc6, slot=W_)
    # ---- the back-room door (closed): frame, leaf, kick plate, lever, closer, EMPLOYEES ONLY; the extinguisher by it
    k.add(bx('bd_jamb', RX1 - 0.05, RX1 + 0.01, by0 - 0.05, by0, 0.0, 2.18, 0.003), 'paint', 0x4a4e52, slot=W_)
    k.add(bx('bd_jamb', RX1 - 0.05, RX1 + 0.01, by1, by1 + 0.05, 0.0, 2.18, 0.003), 'paint', 0x4a4e52, slot=W_)
    k.add(bx('bd_head', RX1 - 0.05, RX1 + 0.01, by0 - 0.05, by1 + 0.05, 2.13, 2.18, 0.003), 'paint', 0x4a4e52, slot=W_)
    k.add(bx('bd_leaf', RX1 - 0.012, RX1 + 0.03, by0 + 0.004, by1 - 0.004, 0.012, 2.126, 0.003), 'paint', 0x5a6266, slot=W_)
    k.add(bx('bd_kick', RX1 - 0.0135, RX1 - 0.012, by0 + 0.03, by1 - 0.03, 0.02, 0.26, 0.0005), 'steel', slot=W_)
    k.add(tube('bd_lever', bend([(RX1 - 0.012, by0 + 0.08, 1.02), (RX1 - 0.07, by0 + 0.08, 1.02), (RX1 - 0.07, by0 + 0.2, 1.02)], 0.015), 0.009, 12),
          'steel', slot=W_)
    k.add(parts.rod('bd_rose', (RX1 - 0.012, by0 + 0.08, 1.02), (RX1 - 0.022, by0 + 0.08, 1.02), 0.03, 20), 'steel', slot=W_)
    k.add(bx('bd_closer', RX1 - 0.07, RX1 - 0.012, by1 - 0.40, by1 - 0.12, 2.02, 2.09, 0.01), 'alu', slot=W_)
    k.fdecal('staff', (RX1 - 0.0125, (by0 + by1) / 2, 1.58), '-x', lift=0.0003)
    ex, ey = RX1 - 0.09, by0 - 0.30
    k.add(lathe_axis('extinguisher', [(0.0, 0.0), (0.075, 0.0), (0.078, 0.01), (0.078, 0.40), (0.06, 0.45), (0.02, 0.47), (0.0, 0.47)],
                     T(ex, ey, 0.85), 32), 'enamel', 0xb5141a, slot=W_)
    k.add(bx('ext_valve', ex - 0.02, ex + 0.02, ey - 0.02, ey + 0.02, 1.32, 1.40, 0.006), 'chrome', slot=W_)
    k.add(tube('ext_hose', bend([(ex, ey + 0.02, 1.37), (ex - 0.02, ey + 0.09, 1.30), (ex - 0.03, ey + 0.09, 0.95)], 0.03), 0.009, 10), 'rubber',
          0x141414, slot=W_)
    k.add(bx('ext_bracket', RX1 - 0.012, RX1, ey - 0.05, ey + 0.05, 1.08, 1.30, 0.003), 'black_metal', slot=W_)
    k.fdecal('ext_sign', (RX1 - 0.0005, ey, 1.75), '-x', lift=0.0003)
    # the electrical panel in the back stock, its conduits up into the tin
    k.add(bx('panel_box', RX1 - 0.10, RX1, 41.70, 42.20, 1.15, 1.95, 0.006), 'paint', 0x8a8e8e, slot=W_)
    k.add(bx('panel_door', RX1 - 0.103, RX1 - 0.1, 41.72, 42.18, 1.17, 1.93, 0.003), 'paint', 0x8a8e8e, slot=W_)
    k.add(bx('panel_latch', RX1 - 0.112, RX1 - 0.103, 42.12, 42.15, 1.50, 1.60, 0.003), 'chrome', slot=W_)
    k.fdecal('panel_label', (RX1 - 0.1035, 41.95, 1.84), '-x', lift=0.0002)
    for yy in (41.80, 41.95, 42.10):
        k.add(parts.rod('conduit', (RX1 - 0.03, yy, 1.95), (RX1 - 0.03, yy, CEIL + 0.02), 0.012, 10), 'galv', slot=W_)
    # ---- by the door inside: the switches, the exit sign over the door
    k.add(bx('switch_plate', VEST_X + 0.25, VEST_X + 0.37, RY0, RY0 + 0.008, 1.20, 1.32, 0.003), 'plastic', 0xe8e4da, slot=W_)
    for dx_ in (-0.025, 0.025):
        k.add(bx('switch', VEST_X + 0.31 + dx_ - 0.006, VEST_X + 0.31 + dx_ + 0.006, RY0 + 0.008, RY0 + 0.014, 1.245, 1.275, 0.002), 'plastic', 0xf2eee4,
              slot=W_)
    ey_ = (DOOR_Y[0] + DOOR_Y[1]) / 2
    k.add(bx('exit_box', VEST_X + 0.22, VEST_X + 0.28, ey_ - 0.18, ey_ + 0.18, 2.62, 2.78, 0.006), 'plastic', 0xe8e4da, slot=W_)
    k.add(parts.rod('exit_stem', (VEST_X + 0.25, ey_, 2.78), (VEST_X + 0.25, ey_, CEIL), 0.008, 10), 'plastic', 0xe8e4da, slot=W_)
    k.decal('exit', (VEST_X + 0.2805, ey_, 2.70), u=(0, 1, 0), v=(0, 0, 1), w=0.30, h=0.12, lift=0.0)
    # outlets
    for (x_, y_, face) in ((3.2, RY1, '-y'), (8.0, RY1, '-y'), (11.0, RY0, '+y'), (12.5, RY1, '-y')):
        sgn = -1 if face == '-y' else 1
        k.add(bx('outlet', x_ - 0.04, x_ + 0.04, y_ + sgn * 0.007, y_, 0.30, 0.42, 0.003), 'plastic', 0xe8e4da, slot=W_)
    # ---- the signs (§4.9): over the counter, ALL SALES FINAL on two cables; by the loan window on the west wall,
    # 30-DAY LOANS · NO ID NO LOAN · TICKET REQUIRED TO REDEEM
    sx_, sy_, sz_ = 6.10, 42.60, 2.95
    k.add(bx('sign_board', sx_ - 0.006, sx_ + 0.006, sy_ - 0.70, sy_ + 0.70, sz_ - 0.18, sz_ + 0.18, 0.003), 'paint', 0xe8e6e0, slot=W_)
    k.fdecal('sales_final', (sx_ - 0.0065, sy_, sz_), '-x', lift=0.0003)
    k.fdecal('sales_final_b', (sx_ + 0.0065, sy_, sz_), '+x', lift=0.0003)
    for yy in (sy_ - 0.55, sy_ + 0.55):
        k.add(parts.rod('sign_cable', (sx_, yy, sz_ + 0.18), (sx_, yy, CEIL), 0.0012, 6), 'steel', slot=W_)
    for key, zz in (('loans30', 2.92), ('noid', 2.50), ('ticket_req', 2.08)):
        k.add(bx('wall_sign', 5.55, 6.43, RY0, RY0 + 0.008, zz - 0.16, zz + 0.16, 0.003), 'paint', 0xe8e6e0, slot=W_)
        k.fdecal(key, (5.99, RY0 + 0.0085, zz), '+y', lift=0.0003)
    # ---- the wall of clocks on the back-stock wall over the counter
    for i, (y, z, r, style, live) in enumerate(CLOCKS):
        clock(k, y, z, r, style, f'clock{i}', live)
    # ---- the tin ceiling: one plate over the room (its texture laid over the plan), the strips, the fan's motor, the
    # sprinkler main, cameras, the convex mirror
    tin = k.add(quad('tin', [(RX0, RY0, CEIL), (RX1, RY0, CEIL), (RX1, RY1, CEIL), (RX0, RY1, CEIL)]), 'tin', 0xe9dfc4, slot=C)
    core.orient_normals(tin, (0, 0, -1))
    planar_site_uv(tin)
    for (x, y) in STRIPS:
        strip_fixture(k, x, y)
    fx, fy = FAN_AT
    k.add(lathe_axis('fan_canopy', [(0.0, 0.0), (0.09, 0.0), (0.07, 0.06), (0.0, 0.07)][::-1], T(fx, fy, CEIL) @ RX(180), 32), 'brass', slot=W_)
    k.add(parts.rod('fan_rod', (fx, fy, CEIL - 0.05), (fx, fy, FAN_HUB_Z + 0.10), 0.012, 12), 'brass', slot=W_)
    motor = [(0.0, 0.0), (0.06, 0.0), (0.13, 0.02), (0.14, 0.06), (0.13, 0.10), (0.06, 0.12), (0.02, 0.13), (0.0, 0.13)]
    k.add(lathe_axis('fan_motor', motor, T(fx, fy, FAN_HUB_Z - 0.03), 40), 'brass', slot=W_)
    k.add(lathe_axis('fan_cap', [(0.0, 0.0), (0.06, 0.0), (0.04, -0.04), (0.0, -0.05)][::-1], T(fx, fy, FAN_HUB_Z - 0.03), 24), 'brass', slot=W_)
    for yy in (40.20, 43.10):
        k.add(parts.rod('sprinkler_main', (0.6, yy, CEIL - 0.13), (RX1 - 0.3, yy, CEIL - 0.13), 0.018, 12), 'paint', 0xb3141c, slot=W_)
        for xx in (1.6, 4.4, 7.4, 10.4, 12.6):
            k.add(parts.rod('drop', (xx, yy, CEIL - 0.13), (xx, yy, CEIL - 0.22), 0.012, 10), 'paint', 0xb3141c, slot=W_)
            k.add(parts.rod('head', (xx, yy, CEIL - 0.22), (xx, yy, CEIL - 0.26), 0.007, 10), 'brass', slot=W_)
            k.add(parts.rod('deflector', (xx, yy, CEIL - 0.26), (xx, yy, CEIL - 0.264), 0.014, 12), 'brass', slot=W_)
        for xx in (1.0, 3.0, 5.0, 7.0, 9.0, 11.0, 13.0):
            k.add(tube('hanger', [(xx, yy, CEIL), (xx, yy, CEIL - 0.11)], 0.004, 6), 'galv', slot=W_)
    for (cx_, cy_) in ((3.2, 39.10), (5.9, 43.6), (1.2, 38.0)):
        k.add(lathe_axis('cam_base', [(0.0, 0.0), (0.07, 0.0), (0.07, 0.015), (0.0, 0.016)], T(cx_, cy_, CEIL) @ RX(180), 24), 'plastic', 0xeeebe4,
              slot=W_)
        k.add(lathe_axis('cam_dome', [(0.0, 0.0), (0.055, 0.0)] + [(0.055 * math.cos(math.radians(a)), 0.055 * math.sin(math.radians(a))) for a in
                                                                  range(15, 91, 15)], T(cx_, cy_, CEIL - 0.016) @ RX(180), 24), 'screen', slot=W_)
    mc = Vector((RX0 + 0.35, RY1 - 0.35, 2.95))
    k.add(tube('mirror_arm', [(RX0 + 0.02, RY1 - 0.02, 3.10), tuple(mc + Vector((-0.08, 0.08, 0.1)))], 0.008, 8), 'black_metal', slot=W_)
    dome = [(0.0, 0.0), (0.22, 0.0)] + [(0.22 * math.cos(math.radians(a)), 0.08 * math.sin(math.radians(a))) for a in range(15, 91, 15)]
    dir_ = Vector((1.0, -1.0, -0.6)).normalized()
    Md = Matrix(Vector((0, 0, 1)).rotation_difference(dir_).to_matrix()).to_4x4()
    Md.translation = mc
    k.add(lathe_axis('mirror', dome, Md, 40), 'chrome', slot=W_)
    k.add(xform(torus('mirror_rim', (0, 0, 0), 0.222, 0.008, 'z', 40, 6), Md), 'plastic', 0x1a1a1a, slot=W_)
    k.grime += [((VEST_X + 0.3, RY0, 1.25), 0.25, 0.8), ((RX1, 41.95, 1.55), 0.3, 0.5), ((PARTITION_X - 0.06, (pa + pb) / 2, 1.05), 0.5, 0.5)]
    return [k.finish(40.0), fan_blades()]


FAN_HUB_Z = 3.12


def fan_blades():
    """The ceiling fan's five blades on their irons (PROP: origin at the hub, the axis +z): the game turns it."""
    k = Kit('pwfb', 'SM_NS_Pawn_FanBlades', slots=('body',), dust=0.5, low_grime=0.0, cavity=0.2, site=False)
    for i in range(5):
        a = 2 * math.pi * i / 5
        M = RZ(math.degrees(a))
        iron = tube('iron', bend([(0.10, 0.0, 0.0), (0.18, 0.0, -0.01), (0.26, 0.0, 0.0)], 0.03), 0.012, 8)
        iron.data.transform(M)
        k.add(iron, 'brass')
        blade = [(0.24, -0.06), (0.62, -0.075), (0.66, -0.06), (0.67, 0.0), (0.66, 0.06), (0.62, 0.075), (0.24, 0.06)]
        o = xy_slab('blade', closed_cr(blade, 3), -0.006, 0.0, r=0.002)
        o.data.transform(M @ RX(-8.0))
        k.add(o, 'wood', 0x5a3a20, aux=(1, 0, 0))
    k.add(lathe_axis('hub', [(0.0, 0.0), (0.10, 0.0), (0.11, 0.02), (0.10, 0.035), (0.0, 0.04)], T(0, 0, -0.03), 32), 'brass')
    return k.finish(40.0)


def shell_glow():
    """The six strips' tubes (cool, 4100 K)."""
    return {'SM_NS_Pawn_Shell_GlowCool': [xform(o, SITE_M) for o in strip_tubes()]}


# ================================================================== SM_NS_Pawn_Counter (+ _Glass, _GlowCool): the glass-case counter

CASE_FRAME = 0x1a1b1d          # glass_case (§0.6): satin-black steel
CASE_BASE = 0.34               # the cabinet's top: the display deck sits on it
CASE_DECK = 0.365
CASE_TOP = 0.965               # the top glass's underside
CURIO_SOCKETS = [(5.93, 40.62), (5.93, 40.93), (5.93, 41.24), (6.16, 40.62), (6.16, 40.93), (6.16, 41.24)]   # C0..C5 (shop x, y)


def lighter(k, M, col=0xd8d8d8, metal='chrome', slot=None):
    """A flip-top lighter standing up: the case, the lid's hinge line, the chimney showing over it."""
    mk = k.mark()
    k.add(bx('lt_case', -0.019, 0.019, -0.0065, 0.0065, 0.0, 0.039, 0.003), metal if metal != 'gold' else 'gold', col, slot=slot, aux=(0.15, 0, 0))
    k.add(bx('lt_lid', -0.019, 0.019, -0.0065, 0.0065, 0.040, 0.057, 0.003), metal if metal != 'gold' else 'gold', col, slot=slot, aux=(0.15, 0, 0))
    k.add(bx('lt_seam', -0.0192, 0.0192, -0.0067, 0.0067, 0.0392, 0.0402), 'rubber', 0x111111, slot=slot)
    k.place(mk, M)


def chip_stack(k, M, n, col, slot=None):
    mk = k.mark()
    for i in range(n):
        k.add(lathe_axis('chip', [(0.0, 0.0), (0.0195, 0.0), (0.0195, 0.0033), (0.0, 0.0033)], T(0.0, 0.0, i * 0.0034), 24), 'plastic', col, slot=slot)
        for j in range(6):
            a = 2 * math.pi * j / 6 + i
            k.add(obox('chip_spot', (0.0196 * math.cos(a), 0.0196 * math.sin(a), i * 0.0034 + 0.00165), (0.0011, 0.006, 0.0033),
                       rot_z=math.degrees(a)), 'plastic', 0xf2ede2, slot=slot)
    k.place(mk, M)


def harmonica(k, M, slot=None):
    mk = k.mark()
    k.add(bx('hm_comb', -0.05, 0.05, -0.012, 0.012, 0.0, 0.01, 0.001), 'wood', 0x5a3a20, slot=slot, aux=(1, 0, 0))
    for zz in (0.0, 0.010):
        k.add(bx('hm_cover', -0.052, 0.052, -0.0135, 0.0135, zz - 0.003 if zz == 0 else zz, zz + 0.0 if zz == 0 else zz + 0.003, 0.002), 'chrome', slot=slot)
    for i in range(10):
        k.add(bx('hm_hole', -0.045 + i * 0.01 - 0.003, -0.045 + i * 0.01 + 0.003, -0.0122, -0.0118, 0.002, 0.008), 'rubber', 0x0a0a0a, slot=slot)
    k.place(mk, M)


def coin_flip(k, M, key, slot=None):
    """A coin in a cardboard 2 x 2 flip, leaning back on its fold."""
    mk = k.mark()
    a = math.radians(70.0)
    o = bx('flip', -0.025, 0.025, -0.0006, 0.0006, 0.0, 0.05, 0.0)
    o.data.transform(RX(90.0 - 70.0))
    k.add(o, 'cardboard', 0xe8e0cc, slot=slot)
    k.decal(key, (0.0, -0.0008, 0.025), u=(1, 0, 0), v=(0, 0, 1), lift=0.0, w=0.05, h=0.05, matrix=RX(90.0 - 70.0))
    c = lathe_axis('coin', [(0.0, 0.0), (0.0125, 0.0), (0.0125, 0.002), (0.0, 0.002)], T(0.0, -0.0026, 0.03) @ RX(90), 24)
    c.data.transform(RX(90.0 - 70.0))
    k.add(c, 'gold', 0xd8d8d8 if 'flip0' in key else 0xe8c060, slot=slot, aux=(0.2, 0, 0))
    k.place(mk, M)


def p_flip(seed):
    def draw(s, w, h):
        rnd = random.Random(seed)
        s.rect(0, 0, w, h, 0xe8e0cc, rough=0.8)
        s.ring(w / 2, h * 0.58, w * 0.27, w * 0.29, 0xb8aa88)
        hand(s, rnd.choice(['1964', '1921', '1886', '1958']), w * 0.2, h * 0.08, h * 0.14, 0x1b2f7a, width=0.09, seed=seed)
    return draw


def pocket_watch_stand(k, M, slot=None):
    mk = k.mark()
    k.add(lathe_axis('pw_base', [(0.0, 0.0), (0.035, 0.0), (0.03, 0.008), (0.0, 0.009)], Matrix(), 24), 'wood', 0x3a1e10, slot=slot, aux=(1, 0, 0))
    k.add(tube('pw_post', bend([(0.0, 0.0, 0.009), (0.0, 0.0, 0.10), (0.0, -0.02, 0.11)], 0.02), 0.002, 6), 'brass', slot=slot)
    k.add(lathe_axis('pw_case', [(0.0, 0.0), (0.024, 0.0), (0.025, 0.006), (0.024, 0.012), (0.0, 0.013)], T(0.0, -0.02, 0.075) @ RX(90), 32), 'gold',
          0xe8c070, slot=slot, aux=(0.08, 0, 0))
    k.add(parts.rod('pw_face', (0.0, -0.0331, 0.075), (0.0, -0.0336, 0.075), 0.021, 32), 'gloss', 0xf0ead8, slot=slot)
    k.add(torus('pw_bow', (0.0, -0.02, 0.105), 0.008, 0.0015, 'y', 12, 4), 'gold', 0xe8c070, slot=slot)
    k.place(mk, M)


def counter():
    k = Kit('pwc', 'SM_NS_Pawn_Counter', slots=('body', 'jewel'), dust={'body': 0.12, 'jewel': 0.0}, low_grime={'body': 0.25, 'jewel': 0.0},
            cavity={'body': 0.3, 'jewel': 0.15})
    for i, (b, g, ink, sub) in enumerate(WATCH_DIALS):
        k.panel(f'dial{i}', 0.04, 0.04, p_dial(b, g, ink, sub), density=6.0)
    for i in range(16):
        k.panel(f'jt{i}', 0.010, 0.022, p_jtag(300 + i), density=5.0)
    for i in range(6):
        k.panel(f'ear{i}', 0.05, 0.065, p_earrings(i), density=3.0)
    for i in range(5):
        k.panel(f'flip{i}', 0.05, 0.05, p_flip(40 + i), density=3.0)
    k.panel('reg_keys', 0.30, 0.16, draw_register_keys, density=2.4)
    k.panel('reg_clerk', 0.10, 0.03, p_vfd([('SUBTL', '0.00'), ('CASH', '0.00')]), glow=3.0, density=3.0)
    k.panel('reg_cust', 0.12, 0.035, p_vfd([('THANK YOU', '')]), glow=3.0, density=3.0)
    k.panel('pad', 0.14, 0.20, draw_ticket_pad, density=3.0)
    k.panel('lean0', 0.20, 0.03, draw_lean, density=3.0)
    k.atlas.render()
    J = 'jewel'
    x0, x1 = CASE_X
    # ---- the four cases: a cabinet on a recessed kick, the velvet deck, the satin-black frame round the glass, the sliding
    # doors' track on the staff side with a lock each
    decks = [0x2a2a2c, 0x3a1418, 0x141416, 0x141416]
    for ci in range(4):
        a, b = CASE_Y[ci], CASE_Y[ci + 1]
        k.add(bx('kick', x0 + 0.05, x1 - 0.02, a + 0.005, b - 0.005, 0.0, 0.09, 0.0), 'rubber', 0x141414)
        k.add(bx('cabinet', x0, x1, a + 0.004, b - 0.004, 0.09, CASE_BASE, 0.004), 'melamine', 0x1c1c1e)
        k.add(bx('cab_rail', x0 - 0.006, x0, a + 0.004, b - 0.004, CASE_BASE - 0.03, CASE_BASE, 0.002), 'powder', CASE_FRAME)
        k.add(bx('deck', x0 + 0.02, x1 - 0.02, a + 0.02, b - 0.02, CASE_BASE, CASE_DECK, 0.003), 'velvet' if ci else 'felt', decks[ci])
        for (xa, xb) in ((x0, x0 + 0.02), (x1 - 0.02, x1)):
            k.add(bx('rail_lo', xa, xb, a, b, CASE_BASE, CASE_DECK + 0.004, 0.002), 'powder', CASE_FRAME)
            k.add(bx('rail_hi', xa, xb, a, b, CASE_TOP - 0.022, CASE_TOP, 0.002), 'powder', CASE_FRAME)
        for yy in (a, b - 0.02):
            for (xa, xb) in ((x0, x0 + 0.02), (x1 - 0.02, x1)):
                k.add(bx('post', xa, xb, yy, yy + 0.02, CASE_BASE, CASE_TOP, 0.002), 'powder', CASE_FRAME)
            k.add(bx('end_rail', x0, x1, yy, yy + 0.02, CASE_TOP - 0.022, CASE_TOP, 0.002), 'powder', CASE_FRAME)
        k.add(bx('track', x1 - 0.03, x1 - 0.002, a + 0.02, b - 0.02, CASE_DECK, CASE_DECK + 0.012, 0.001), 'alu')
        ym = (a + b) / 2
        k.add(bx('lock_body', x1 - 0.006, x1 + 0.006, ym - 0.012, ym + 0.012, CASE_TOP - 0.06, CASE_TOP - 0.025, 0.002), 'chrome')
        k.add(parts.rod('lock_cyl', (x1 + 0.006, ym, CASE_TOP - 0.042), (x1 + 0.012, ym, CASE_TOP - 0.042), 0.007, 14), 'chrome')
        for sy in (-1, 1):
            k.add(bx('pull', x1, x1 + 0.004, ym + sy * 0.20 - 0.012, ym + sy * 0.20 + 0.012, CASE_DECK + 0.15, CASE_DECK + 0.40, 0.002), 'alu')
        if ci in (1, 3):
            k.decal('lean0', (x0 + 0.06, ym - 0.25, CASE_TOP + 0.0102), u=(0, -1, 0), v=(1, 0, 0), lift=0.0)
        k.grime += [((x0 - 0.02, ym, CASE_TOP), 0.5, 0.3), ((x1 + 0.02, ym, CASE_TOP - 0.04), 0.15, 0.7)]
    # ---- the flap at the west end: a solid counter section, its top lifting on a piano hinge, a swing gate below
    fa, fb = FLAP_Y
    k.add(bx('flap_end', x0, x1 + 0.04, fa, fa + 0.04, 0.0, 0.95, 0.004), 'melamine', 0x1c1c1e)
    k.add(bx('flap_gate', x0, x0 + 0.03, fa + 0.045, fb - 0.005, 0.07, 0.92, 0.004), 'melamine', 0x1c1c1e)
    for zz in (0.25, 0.75):
        k.add(parts.rod('gate_hinge', (x0 - 0.003, fb - 0.006, zz - 0.04), (x0 - 0.003, fb - 0.006, zz + 0.04), 0.006, 10), 'chrome')
    k.add(bx('flap_top_fixed', x0 - 0.02, x1 + 0.06, fa, fa + 0.05, 0.95, 0.985, 0.006, 3), 'wood', 0x5a3418, aux=(2, 0, 0))
    k.add(bx('flap_top', x0 - 0.02, x1 + 0.06, fa + 0.052, fb + 0.0, 0.95, 0.985, 0.006, 3), 'wood', 0x5a3418, aux=(2, 0, 0))
    k.add(bx('flap_hinge', x0 - 0.01, x1 + 0.05, fb - 0.004, fb + 0.002, 0.983, 0.99, 0.001), 'brass')
    k.add(bx('flap_notch', x0 - 0.021, x0 + 0.02, fa + 0.24, fa + 0.34, 0.955, 0.98, 0.006), 'rubber', 0x2a1a10)
    # the door buzzer's button, under the lip on the staff side (station pawn-door)
    k.add(bx('buzz_box', x1 + 0.01, x1 + 0.06, fa + 0.10, fa + 0.18, 0.90, 0.948, 0.004), 'plastic', 0x1a1a1a)
    k.add(parts.rod('buzz_btn', (x1 + 0.035, fa + 0.14, 0.90), (x1 + 0.035, fa + 0.14, 0.893), 0.011, 14), 'gem', 0xb01010)
    ITEM_SPOTS['BuzzerButton'] = (x1 + 0.035, fa + 0.14, 0.893)
    # ---- the curio case: lighters on a step at the back, a pocket watch on its stand, old chips, a harmonica, coins in
    # flips; its six sockets C0..C5 kept clear on the deck
    a, b = CASE_Y[0], CASE_Y[1]
    k.add(bx('curio_step', 6.22, x1 - 0.03, a + 0.03, b - 0.03, CASE_DECK, CASE_DECK + 0.06, 0.004), 'felt', 0x2a2a2c)
    for i in range(9):
        yy = a + 0.07 + i * 0.105
        lighter(k, T(6.29, yy, CASE_DECK + 0.06) @ RZ(90.0 + (i % 3 - 1) * 8), col=[0xd8d8d8, 0xc8a050, 0x2a2a2a][i % 3],
                metal=['chrome', 'gold', 'black_metal'][i % 3])
    pocket_watch_stand(k, T(5.86, a + 0.06, CASE_DECK) @ RZ(-90.0), slot=J)
    for i, (dy, n, col) in enumerate(((0.0, 7, 0xa8161e), (0.05, 4, 0x1e4a8a), (0.10, 9, 0x141414))):
        chip_stack(k, T(5.86, b - 0.10 - dy, CASE_DECK), n, col)
    harmonica(k, T(6.05, b - 0.08, CASE_DECK) @ RZ(80.0))
    for i in range(5):
        coin_flip(k, T(6.20 - 0.0, a + 0.12 + i * 0.18, CASE_DECK + 0.06) @ RZ(-90.0 + (i % 2) * 6), f'flip{i}')
    for i, (sx, sy) in enumerate(CURIO_SOCKETS):
        ITEM_SPOTS[f'C{i}'] = ((sx, sy, CASE_DECK), 'facing -x (stage north, to the customer)')
    # ---- the watch case: watches on pillows on a two-step riser
    a, b = CASE_Y[1], CASE_Y[2]
    k.add(bx('w_step', 6.08, x1 - 0.03, a + 0.03, b - 0.03, CASE_DECK, CASE_DECK + 0.10, 0.006), 'velvet', 0x3a1418)
    WATCHES = [('chrome', 0xd8d8d8, 'bracelet', 0), ('gold', 0xf0c35a, 'bracelet', 3), ('chrome', 0xd0d0d0, 'leather', 1), ('chrome', 0xc8c8c8, 'bracelet', 2),
               ('gold', 0xe8c070, 'leather', 3), ('chrome', 0xd8d8d8, 'bracelet', 4), ('chrome', 0x2a2a2a, 'leather', 0)]
    for i, (metal, col, strap, dial) in enumerate(WATCHES):
        row = 0 if i < 4 else 1
        yy = a + 0.14 + (i % 4) * 0.25 if row == 0 else a + 0.26 + (i - 4) * 0.27
        xx = 5.90 if row == 0 else 6.20
        zz = CASE_DECK if row == 0 else CASE_DECK + 0.10
        pillow(k, T(xx, yy, zz), w=0.07, d=0.055, h=0.05, col=0xece6da if i % 3 else 0x1a1a1a, slot=J)
        watch(k, T(xx, yy, zz + 0.035) @ RZ(-90.0) @ RX(-20.0), metal=metal, col=col, strap=strap, strap_col=[0x2a1a10, 0x111111, 0x4a2a14][i % 3],
              slot=J, dial_key=f'dial{dial}')
        k.decal(f'jt{i}', (xx - 0.04, yy + 0.03, zz + 0.006), u=(0, -1, 0), v=(1, 0, 0), lift=0.0, w=0.010, h=0.022)
    # ---- jewelry case 1: ring trays, earring cards, bracelets on a T-bar
    a, b = CASE_Y[2], CASE_Y[3]
    for i, yy in enumerate((a + 0.25, a + 0.62)):
        ring_tray(k, T(5.95, yy, CASE_DECK) @ RZ(-90.0) @ RX(-8.0), seed=40 + i, slot=J, ring_slot=J, sparse=0.4)
    for i in range(6):
        earring_card(k, T(6.24, a + 0.10 + i * 0.075, CASE_DECK) @ RZ(-90.0), f'ear{i}', seed=i, slot=J)
    bracelet_bar(k, T(6.22, b - 0.20, CASE_DECK) @ RZ(-90.0), n=3, seed=61, slot=J)
    # ---- jewelry case 2: chains on two velvet busts, a tray of chains and a T-bar of bracelets
    a, b = CASE_Y[3], CASE_Y[4]
    for i, (by_, bxx) in enumerate(((a + 0.22, 6.18), (a + 0.58, 6.18))):
        neck_bust(k, T(bxx, by_, CASE_DECK) @ RZ(-90.0), h=0.40 - i * 0.04, slot=J)   # its chest to the customer (-x)
        top = CASE_DECK + 0.40 - i * 0.04
        for j, (sag, col, link) in enumerate(((0.09, 0xf0c35a, 0.008), (0.13, 0xe8b04a, 0.006), (0.17, 0xf2c860, 0.0105))[:2 + i]):
            nz = top - 0.02 - j * 0.004
            pts = bust_chain(bxx, by_, CASE_DECK, 0.40 - i * 0.04, nz, sag)
            chain_links(k, pts, link=link, wire=link * 0.24, slot=J, col=col, aux=(0.07, 0, 0))
    k.add(bx('chain_tray', 5.80, 6.02, b - 0.36, b - 0.06, CASE_DECK, CASE_DECK + 0.018, 0.004), 'velvet', 0x141416, slot=J)
    rnd = random.Random(9)
    for j in range(5):
        y0_ = b - 0.33 + j * 0.055
        pts = [(5.83, y0_, CASE_DECK + 0.02), (5.91, y0_ + rnd.uniform(-0.01, 0.01), CASE_DECK + 0.02), (5.99, y0_ + 0.01, CASE_DECK + 0.02)]
        chain_links(k, pts, link=0.006 + 0.002 * (j % 3), wire=0.0016, slot=J, col=rnd.choice([0xf0c35a, 0xe8b04a, 0xdcdcdc]))
    # ---- on top: the register at the east end, the loupe on its mat by the Item mark and the Tray, the ticket book
    zt = CASE_TOP + 0.010
    register(k, T(6.08, 44.18, zt) @ RZ(-90.0))
    tray = (5.98, 42.28)
    k.add(bx('tray', tray[0] - 0.11, tray[0] + 0.11, tray[1] - 0.075, tray[1] + 0.075, zt, zt + 0.016, 0.006), 'velvet', 0x141416)
    k.add(bx('tray_rim', tray[0] - 0.115, tray[0] + 0.115, tray[1] - 0.08, tray[1] + 0.08, zt, zt + 0.006, 0.004), 'velvet', 0x141416)
    ITEM_SPOTS['Tray'] = ((tray[0], tray[1], zt + 0.016), 'a velvet tray on the glass')
    ITEM_SPOTS['Item'] = ((5.98, 41.98, zt), 'on the watch case\'s glass, where Augie sets the appraised thing down')
    k.add(bx('mat', 6.06, 6.30, 42.50, 42.74, zt, zt + 0.004, 0.002), 'velvet', 0x141414)
    k.add(lathe_axis('loupe', [(0.0, 0.0), (0.013, 0.0), (0.015, 0.01), (0.013, 0.022), (0.0, 0.022)], T(6.18, 42.62, zt + 0.004) @ RX(90) @ RY(30), 20),
          'black_metal')
    k.add(parts.rod('loupe_lens', (6.18, 42.62, zt + 0.0175), (6.18, 42.628, zt + 0.0175), 0.011, 16), 'screen')
    k.add(bx('book', 6.08, 6.36, 43.05, 43.25, zt, zt + 0.03, 0.004), 'paper', 0xf2ead2)
    k.add(bx('book_cover', 6.08, 6.36, 43.05, 43.25, zt + 0.03, zt + 0.034, 0.002), 'cardboard', 0x2a3a6a)
    k.decal('pad', (6.22, 43.15, zt + 0.0345), u=(0, -1, 0), v=(1, 0, 0), lift=0.0, w=0.19, h=0.27)
    k.add(parts.rod('pen', (6.12, 43.30, zt + 0.005), (6.28, 43.34, zt + 0.005), 0.0045, 8), 'plastic', 0x1a2a8a)
    k.polish = [((x0, 42.5, CASE_TOP), 2.0, 0.4)]
    for g in counter_glass():
        k.special(g, 'ns_glass')
    return [k.finish(35.0)]


def counter_glass():
    """The cases' glass: fronts, tops, the staff side's sliding doors (two panes per case, staggered)."""
    x0, x1 = CASE_X
    objs = []
    for ci in range(4):
        a, b = CASE_Y[ci] + 0.02, CASE_Y[ci + 1] - 0.02
        objs.append(quad('front', [(x0 + 0.01, b, CASE_DECK), (x0 + 0.01, a, CASE_DECK), (x0 + 0.01, a, CASE_TOP - 0.022), (x0 + 0.01, b, CASE_TOP - 0.022)]))
        objs.append(quad('top', [(x0, a - 0.02, CASE_TOP + 0.01), (x1, a - 0.02, CASE_TOP + 0.01), (x1, b + 0.02, CASE_TOP + 0.01), (x0, b + 0.02, CASE_TOP + 0.01)]))
        ym = (a + b) / 2
        for (ya, yb, xx) in ((a, ym + 0.04, x1 - 0.012), (ym - 0.04, b, x1 - 0.024)):
            objs.append(quad('door', [(xx, ya, CASE_DECK + 0.012), (xx, yb, CASE_DECK + 0.012), (xx, yb, CASE_TOP - 0.022), (xx, ya, CASE_TOP - 0.022)]))
    return objs


def counter_glow():
    """LED strips under each case's top rails, front and back (cool, 4000 K)."""
    x0, x1 = CASE_X
    out = []
    for ci in range(4):
        a, b = CASE_Y[ci] + 0.03, CASE_Y[ci + 1] - 0.03
        out.append(bx('case_led', x0 + 0.025, x0 + 0.045, a, b, CASE_TOP - 0.03, CASE_TOP - 0.024))
        out.append(bx('case_led_b', x1 - 0.045, x1 - 0.025, a, b, CASE_TOP - 0.03, CASE_TOP - 0.024))
    return {'SM_NS_Pawn_Counter_GlowCool': [xform(o, SITE_M) for o in out]}


# ================================================================== SM_NS_Pawn_Cage (+ _Glass, _GlowWarm, SM_NS_Pawn_SafeDoor)

CAGE_RAIL = 1.07               # the diamond-plate base's top; the mesh above
DESK = (6.56, 7.24, 37.45, 39.66)        # Augie's desk inside the loan window (x0, x1, y0, y1)
PLEDGE_SHELF = (8.55, 9.05, 37.50, 40.80)   # the pledge shelves behind the safe (facing the window)
SAFE_W, SAFE_D, SAFE_H = 0.78, 0.70, 1.32
SAFE_DOOR = (-SAFE_W / 2 + 0.07, SAFE_W / 2 - 0.07, 0.22, SAFE_H - 0.12)   # the door's opening in the safe's own frame (x0, x1, z0, z1)


def safe_matrix():
    x, y, yaw = SAFE_AT
    return T(x, y, 0.0) @ RZ(yaw)


def safe_body(k, slot=None):
    """The safe's body (its own frame: front toward -y, the floor under its middle): plinth on casters, the walls round
    the door's opening, the cap, the hinge barrels (the door, SM_NS_Pawn_SafeDoor, is separate), the dark inside with a
    shelf and a cash box behind the door, the ledgers and a can of pens on top."""
    W, D, Hh = SAFE_W, SAFE_D, SAFE_H
    green, dark = 0x1d2620, 0x141a16
    yf = -D / 2
    dx0, dx1, dz0, dz1 = SAFE_DOOR
    mk = k.mark()
    k.add(bx('plinth', -W / 2 - 0.015, W / 2 + 0.015, -D / 2 - 0.015, D / 2 + 0.015, 0.075, 0.16, 0.012, 3), 'enamel', dark, slot=slot)
    k.add(bx('back', -W / 2, W / 2, yf + 0.10, D / 2, 0.15, Hh - 0.045, 0.035, 4), 'enamel', green, slot=slot)
    for (a0, a1, b0, b1) in ((-W / 2, dx0, 0.15, Hh - 0.045), (dx1, W / 2, 0.15, Hh - 0.045), (dx0, dx1, 0.15, dz0), (dx0, dx1, dz1, Hh - 0.045)):
        k.add(bx('front_wall', a0, a1, yf, yf + 0.11, b0, b1, 0.02 if a0 in (-W / 2,) or a1 == W / 2 else 0.004, 3), 'enamel', green, slot=slot)
    k.add(bx('cap', -W / 2 - 0.012, W / 2 + 0.012, -D / 2 - 0.012, D / 2 + 0.012, Hh - 0.05, Hh - 0.02, 0.01, 3), 'enamel', dark, slot=slot)
    k.add(bx('cap_top', -W / 2 + 0.005, W / 2 - 0.005, -D / 2 + 0.005, D / 2 - 0.005, Hh - 0.022, Hh, 0.012, 3), 'enamel', dark, slot=slot)
    # the inside, seen when the door swings: dark steel, a shelf, a cash box
    k.add(bx('inside', dx0 + 0.005, dx1 - 0.005, yf + 0.11, yf + 0.112, dz0 + 0.005, dz1 - 0.005), 'black_metal', slot=slot)
    k.add(bx('shelf', dx0, dx1, yf + 0.11, D / 2 - 0.05, 0.70, 0.715, 0.0), 'black_metal', slot=slot)
    k.add(bx('cashbox', -0.15, 0.12, yf + 0.14, yf + 0.40, 0.715, 0.80, 0.008), 'powder', 0x3a3e42, slot=slot)
    for sx in (-1, 1):
        for sy in (-1, 1):
            cx_, cy_ = sx * (W / 2 - 0.07), sy * (D / 2 - 0.07)
            k.add(bx('fork', cx_ - 0.03, cx_ + 0.03, cy_ - 0.024, cy_ + 0.024, 0.06, 0.078, 0.004), 'black_metal', slot=slot)
            for side in (-1, 1):
                k.add(bx('fork_cheek', cx_ - 0.025, cx_ + 0.025, cy_ + side * 0.022 - 0.003, cy_ + side * 0.022 + 0.003, 0.022, 0.07, 0.003), 'black_metal',
                      slot=slot)
            k.add(parts.rod('wheel', (cx_, cy_ - 0.017, 0.038), (cx_, cy_ + 0.017, 0.038), 0.038, 28), 'black_metal', slot=slot)
            k.add(parts.rod('axle', (cx_, cy_ - 0.027, 0.038), (cx_, cy_ + 0.027, 0.038), 0.007, 12), 'nickel', slot=slot)
    # hinge barrels on the body (the door's straps meet them): left as you face it
    for z in (dz0 + 0.12, dz1 - 0.12):
        k.add(parts.rod('hinge', (dx0 - 0.006, yf - 0.022, z - 0.07), (dx0 - 0.006, yf - 0.022, z + 0.07), 0.019, 24), 'enamel', dark, slot=slot)
        for sz in (-1, 1):
            k.add(lathe_axis('finial', [(0.0, 0.0), (0.012, 0.0), (0.014, 0.008), (0.006, 0.02), (0.0, 0.026)],
                             T(dx0 - 0.006, yf - 0.022, z + sz * 0.07) @ (RX(180) if sz < 0 else Matrix()), 20), 'brass', slot=slot)
    zt = Hh
    LEDGERS = [('LOAN BOOK', 0x5a1c18, '1989'), ('LOAN BOOK', 0x1c2a3a, '2007'), ('POLICE LOG', 0x23331f, '2019')]
    for i, (x0_, wl, hl) in enumerate(((-0.30, 0.042, 0.30), (-0.255, 0.040, 0.292), (-0.212, 0.042, 0.30))):
        k.add(bx('ledger', x0_, x0_ + wl, -0.12, 0.12, zt, zt + hl, 0.003), 'cardboard', LEDGERS[i][1], slot=slot)
        k.fdecal(f'led{i}', (x0_ + wl / 2, -0.1203, zt + hl / 2), '-y', w=wl - 0.003, h=hl - 0.006, lift=0.0002)
    k.add(bx('bookend', -0.168, -0.163, -0.08, 0.08, zt, zt + 0.16, 0.002), 'black_metal', slot=slot)
    k.add(bx('bookend_foot', -0.168, -0.10, -0.08, 0.08, zt, zt + 0.004), 'black_metal', slot=slot)
    k.add(lathe_axis('can', [(0.0, 0.0), (0.05, 0.0), (0.05, 0.13), (0.048, 0.135), (0.0, 0.135)], T(0.12, 0.08, zt), 32), 'paint', 0xb5221c, slot=slot)
    rnd = random.Random(31)
    for j in range(7):
        a, rr = rnd.uniform(0, 6.28), rnd.uniform(0.0, 0.03)
        b = Vector((0.12 + rr * math.cos(a), 0.08 + rr * math.sin(a), zt + 0.02))
        t = b + Vector((rnd.uniform(-0.03, 0.03), rnd.uniform(-0.03, 0.03), 0.15))
        k.add(parts.rod('pen', tuple(b), tuple(t), 0.0045, 8), 'plastic', rnd.choice([0x1a2a8a, 0x111111, 0xb01818, 0xe8e2d0]), slot=slot)
    k.grime += [((0.0, yf, 0.15), 0.4, 0.4)]
    k.place(mk, safe_matrix())


def safe_door():
    """The safe's door (PROP: origin on its hinge axis at the floor; its front toward -x, as the safe faces the loan
    window): the slab with its molded panels (the shop's name and a painted river at dusk in gold leaf), the combination
    dial, the spoked handle, the brass nameplate, the hinge straps. The game swings it about Z."""
    k = Kit('pwsd', 'SM_NS_Pawn_SafeDoor', slots=('body',), dust=0.3, low_grime=0.0, site=False)
    k.panel('top', 0.52, 0.21, draw_safe_top, density=1.2)
    k.panel('low', 0.52, 0.17, draw_safe_low, density=1.4)
    k.panel('dial', 0.11, 0.11, draw_dial, density=2.4)
    k.panel('plate', 0.12, 0.035, draw_safe_plate, density=2.0)
    k.atlas.render()
    green = 0x1d2620
    yf = -SAFE_D / 2
    dx0, dx1, dz0, dz1 = SAFE_DOOR
    k.add(bx('door', dx0 + 0.002, dx1 - 0.002, yf - 0.006, yf + 0.10, dz0 + 0.002, dz1 - 0.002, 0.004, 2), 'enamel', green)
    PANELS = (('top', 0.90, dz1 - 0.035), ('low', dz0 + 0.035, 0.47))
    pw = (dx1 - 0.045) - (dx0 + 0.045)
    for key, pz0, pz1 in PANELS:
        k.add(bx('molding', dx0 + 0.035, dx1 - 0.035, yf - 0.012, yf - 0.006, pz0, pz1, 0.004, 2), 'enamel', 0x141a16)
        k.add(bx('panel', dx0 + 0.045, dx1 - 0.045, yf - 0.0125, yf - 0.009, pz0 + 0.01, pz1 - 0.01, 0.002), 'enamel', green)
        k.fdecal(key, (0.0, yf - 0.0125, (pz0 + pz1) / 2), '-y', w=pw - 0.004, h=(pz1 - pz0) - 0.024, lift=0.0003)
    for (a0, a1, b0, b1) in ((dx0 + 0.03, dx1 - 0.03, 0.508, 0.5095), (dx0 + 0.03, dx1 - 0.03, 0.8605, 0.862), (dx0 + 0.03, dx0 + 0.0315, 0.508, 0.862),
                             (dx1 - 0.0315, dx1 - 0.03, 0.508, 0.862)):
        k.add(bx('pinstripe', a0, a1, yf - 0.0063, yf - 0.0058, b0, b1), 'gold', 0xe0b860, aux=(0.15, 0, 0))
    for z in (dz0 + 0.12, dz1 - 0.12):
        k.add(bx('hinge_strap', dx0 - 0.01, dx0 + 0.07, yf - 0.012, yf - 0.006, z - 0.045, z + 0.045, 0.003), 'enamel', 0x141a16)
    dial_c = Vector((-0.02, yf - 0.012, 0.70))
    FR = RX(90)
    k.add(lathe_axis('dial_ring', [(0.0, 0.0), (0.074, 0.0), (0.075, 0.006), (0.071, 0.012), (0.062, 0.013), (0.0, 0.013)], T(*dial_c) @ FR, 64), 'chrome')
    k.add(bx('index', dial_c.x - 0.002, dial_c.x + 0.002, dial_c.y - 0.0135, dial_c.y - 0.012, dial_c.z + 0.062, dial_c.z + 0.073), 'gold', 0xffd38a)
    k.add(lathe_axis('dial', [(0.0, 0.0), (0.058, 0.0), (0.0585, 0.012), (0.056, 0.018), (0.0, 0.018)], T(*(dial_c + Vector((0, -0.012, 0)))) @ FR, 96),
          'chrome')
    for i in range(48):
        a = 2 * math.pi * i / 48
        k.add(obox('knurl', (dial_c.x + 0.0585 * math.cos(a), dial_c.y - 0.021, dial_c.z + 0.0585 * math.sin(a)), (0.0022, 0.0110, 0.0024),
                   rot_y=-math.degrees(a)), 'chrome')
    k.fdecal('dial', (dial_c.x, dial_c.y - 0.0302, dial_c.z), '-y', w=0.104, h=0.104, lift=0.0002,
             outline=[(0.5 + 0.5 * math.cos(2 * math.pi * j / 64), 0.5 + 0.5 * math.sin(2 * math.pi * j / 64)) for j in range(64)])
    k.add(lathe_axis('knob', [(0.0, 0.0), (0.024, 0.0), (0.026, 0.01), (0.022, 0.03), (0.016, 0.036), (0.0, 0.037)],
                     T(dial_c.x, dial_c.y - 0.03, dial_c.z) @ FR, 40), 'chrome')
    hc = Vector((0.20, yf - 0.012, 0.70))
    k.add(lathe_axis('hub', [(0.0, 0.0), (0.03, 0.0), (0.03, 0.01), (0.022, 0.035), (0.0, 0.04)], T(*hc) @ FR, 32), 'nickel')
    for i in range(3):
        a = math.radians(90 + 120 * i + 15)
        tip = hc + Vector((0.11 * math.cos(a), -0.03, 0.11 * math.sin(a)))
        k.add(parts.rod('spoke', tuple(hc + Vector((0, -0.03, 0))), tuple(tip), 0.0085, 16), 'nickel')
        prof = [(0.0, -0.022)] + [(0.022 * math.sin(math.radians(t)), -0.022 * math.cos(math.radians(t))) for t in range(15, 180, 15)] + [(0.0, 0.022)]
        k.add(lathe_axis('ball', prof, T(*tip), 24), 'nickel')
    k.add(lathe_axis('escutcheon', [(0.0, 0.0), (0.02, 0.0), (0.021, 0.004), (0.0, 0.005)], T(-0.205, yf - 0.006, 0.70) @ FR, 32), 'brass')
    k.add(bx('keyhole', -0.207, -0.203, yf - 0.0115, yf - 0.0105, 0.688, 0.706), 'rubber', 0x050505)
    k.add(bx('plate', -0.08, 0.04, yf - 0.0095, yf - 0.006, 0.508, 0.543, 0.002), 'brass')
    k.fdecal('plate', (-0.02, yf - 0.0096, 0.5255), '-y', w=0.116, h=0.031, lift=0.0002)
    k.grime = [((0.20, yf - 0.05, 0.70), 0.14, 0.9), ((-0.02, yf - 0.04, 0.70), 0.1, 0.7)]
    k.polish = [((0.20, yf - 0.06, 0.70), 0.12, 0.8)]
    # into the shop's orientation, the origin on the hinge axis at the floor
    M = safe_matrix()
    hinge = Vector(safe_hinge())
    for o in k.objs:
        o.data.transform(T(-hinge.x, -hinge.y, 0.0) @ M)
    k.grime = [(tuple(T(-hinge.x, -hinge.y, 0.0) @ M @ Vector(cc)), r, a) for cc, r, a in k.grime]
    k.polish = [(tuple(T(-hinge.x, -hinge.y, 0.0) @ M @ Vector(cc)), r, a) for cc, r, a in k.polish]
    return k.finish(40.0)


def safe_hinge():
    """The safe door's hinge axis at the floor, in the shop frame."""
    return tuple(safe_matrix() @ Vector((SAFE_DOOR[0] - 0.006, -SAFE_D / 2 - 0.022, 0.0)))


def cage():
    k = Kit('pwk', 'SM_NS_Pawn_Cage', slots=('body', 'goods'), dust={'body': 0.2, 'goods': 0.4}, low_grime={'body': 0.35, 'goods': 0.0})
    LEDGERS = [('LOAN BOOK', 0x5a1c18, '1989'), ('LOAN BOOK', 0x1c2a3a, '2007'), ('POLICE LOG', 0x23331f, '2019')]
    for i, (t, c, yr) in enumerate(LEDGERS):
        k.panel(f'led{i}', 0.04, 0.30, p_spine(t, c, year=yr), density=1.2)
    k.panel('license', 0.28, 0.36, draw_license, density=1.8)
    k.panel('cctv', 0.40, 0.25, draw_cctv, glow=1.5, density=1.6)
    BAG_TAGS = [('ring 14k', 401), ('drill', 402), ('console', 403), ('laptop', 404), ('watch', 405), ('camera', 406), ('chain', 407), ('tablet', 408),
                ('speaker', 409), ('saw', 410), ('phone', 411), ('lens', 412), ('ring', 413), ('drone', 414), ('game', 415), ('mic', 416),
                ('sander', 417), ('player', 418), ('flute', 419), ('pedal', 420)]
    for desc, sd in BAG_TAGS:
        k.panel(f'bt{sd}', 0.035, 0.06, p_tag(sd, desc), density=2.6)
    BOXES = [('TOOLS', 431), ('DVDs', 432), ('JEWELRY', 433), ('CORDS', 434), ('GAMES', 435), ('PAPER', 436)]
    for t, sd in BOXES:
        k.panel(f'bx{sd}', 0.20, 0.10, p_box_label(t, sd), density=1.6)
    k.atlas.render()
    G = 'goods'
    FRAME = 0x2e3236
    cx = CAGE_X
    wa, wb = WINDOW_Y
    ga, gb = CAGE_GATE
    # ---- the frame: posts, rails at the tin, at the base's top and the floor, on the front (x = CAGE_X) and the east face
    posts_y = [RY0 + 0.025, wa, wb, ga - 0.03, gb + 0.03, CAGE_Y1 - 0.025]
    for py in posts_y:
        k.add(bx('post', cx - 0.025, cx + 0.025, py - 0.025, py + 0.025, 0.0, CEIL, 0.003), 'paint', FRAME)
    ea, eb = CAGE_GATE_E
    for px in (ea - 0.03, eb + 0.03, PARTITION_X - 0.06, 11.2, RX1 - 0.025):
        k.add(bx('post', px - 0.025, px + 0.025, CAGE_Y1 - 0.05, CAGE_Y1, 0.0, CEIL, 0.003), 'paint', FRAME)
    for z0_, z1_ in ((CEIL - 0.05, CEIL), (CAGE_RAIL - 0.05, CAGE_RAIL), (0.0, 0.05)):
        k.add(bx('rail', cx - 0.02, cx + 0.02, RY0, CAGE_Y1, z0_, z1_, 0.003), 'paint', FRAME)
        k.add(bx('rail', cx, RX1, CAGE_Y1 - 0.045, CAGE_Y1 - 0.005, z0_, z1_, 0.003), 'paint', FRAME)
    # the diamond-plate base and the mesh above it
    for (ya, yb) in ((RY0 + 0.05, wa - 0.025), (wb + 0.025, ga - 0.055), (gb + 0.055, CAGE_Y1 - 0.05)):
        k.add(bx('base', cx - 0.008, cx + 0.008, ya, yb, 0.05, CAGE_RAIL - 0.05, 0.002), 'diamond', 0x8c9096)
        mesh_panel(k, 'x', ya, yb, CAGE_RAIL, CEIL - 0.05, cx)
    for (xa, xb) in ((cx + 0.05, ea - 0.055), (eb + 0.055, PARTITION_X - 0.085), (PARTITION_X - 0.035, 11.175), (11.225, RX1 - 0.05)):
        k.add(bx('base_e', xa, xb, CAGE_Y1 - 0.033, CAGE_Y1 - 0.017, 0.05, CAGE_RAIL - 0.05, 0.002), 'diamond', 0x8c9096)
        mesh_panel(k, 'y', xa, xb, CAGE_RAIL, CEIL - 0.05, CAGE_Y1 - 0.025)
    # the gates: behind the counter's flap, and in the east face to the space behind the counter (both shut, locked)
    def gate(plane, a0, a1, fixed):
        for (p0, p1, z0, z1) in ((a0, a0 + 0.035, 0.03, 2.10), (a1 - 0.035, a1, 0.03, 2.10), (a0, a1, 0.03, 0.065), (a0, a1, 2.065, 2.10),
                                 (a0, a1, 1.03, 1.065)):
            if plane == 'x':
                k.add(bx('gate', fixed - 0.017, fixed + 0.017, p0, p1, z0, z1, 0.003), 'paint', FRAME)
            else:
                k.add(bx('gate', p0, p1, fixed - 0.017, fixed + 0.017, z0, z1, 0.003), 'paint', FRAME)
        mesh_panel(k, plane, a0 + 0.035, a1 - 0.035, 0.065, 2.065, fixed, pitch=0.05)
        mesh_panel(k, plane, a0, a1, 2.15, CEIL - 0.05, fixed)
        if plane == 'x':
            k.add(bx('gate_head', fixed - 0.02, fixed + 0.02, a0, a1, 2.10, 2.15, 0.0), 'paint', FRAME)
            k.add(bx('lockbox', fixed + 0.017, fixed + 0.05, a1 - 0.16, a1 - 0.04, 0.98, 1.12, 0.004), 'paint', 0x1a1c1e)
        else:
            k.add(bx('gate_head', a0, a1, fixed - 0.02, fixed + 0.02, 2.10, 2.15, 0.0), 'paint', FRAME)
            k.add(bx('lockbox', a1 - 0.16, a1 - 0.04, fixed + 0.017, fixed + 0.05, 0.98, 1.12, 0.004), 'paint', 0x1a1c1e)
    gate('x', ga, gb, cx)
    gate('y', ea, eb, CAGE_Y1 - 0.025)
    # ---- the loan window: bullet-resistant steel round a bandit-glass panel, the speak-through, the stainless tray in the
    # ledge under it, the steel header to the tin; the customer's ledge outside on its brackets
    ba, bb = BANDIT_Y
    for (a0, a1) in ((wa + 0.025, ba), (bb, wb - 0.025)):
        k.add(bx('bay_side', cx - 0.03, cx + 0.03, a0, a1, 0.05, 2.12, 0.004), 'diamond', 0x8c9096)
    k.add(bx('bay_low', cx - 0.03, cx + 0.03, ba, bb, 0.05, 1.00, 0.004), 'diamond', 0x8c9096)
    k.add(bx('bay_hdr', cx - 0.03, cx + 0.03, wa + 0.025, wb - 0.025, 2.12, CEIL - 0.05, 0.004), 'paint', 0x2a2c2e)
    gz0, gz1 = 1.10, 2.08
    for (a0, a1, b0, b1) in ((ba, bb, 1.00, gz0), (ba, bb, gz1, 2.12), (ba, ba + 0.05, gz0, gz1), (bb - 0.05, bb, gz0, gz1)):
        k.add(bx('win_frame', cx - 0.045, cx + 0.045, a0, a1, b0, b1, 0.004), 'steel')
    wm = (ba + bb) / 2
    k.add(parts.rod('speak', (cx - 0.012, wm, 1.52), (cx + 0.012, wm, 1.52), 0.065, 32), 'steel')
    for rr in (0.02, 0.035, 0.05):
        k.add(torus('speak_ring', (cx - 0.013, wm, 1.52), rr, 0.0035, 'x', 28, 4), 'rubber', 0x111111)
    k.add(bx('ledge', cx - 0.30, cx + 0.0, wa + 0.05, wb - 0.05, 1.00, 1.035, 0.008, 3), 'laminate', 0xc8c0b0)
    k.add(bx('tray', cx - 0.20, cx + 0.20, wm - 0.22, wm + 0.22, 0.975, 1.0, 0.004), 'steel')
    k.add(bx('tray_well', cx - 0.19, cx + 0.19, wm - 0.21, wm + 0.21, 0.99, 1.036, 0.0), 'steel')
    for sy in (-1, 1):
        k.add(tube('ledge_bracket', bend([(cx - 0.03, wm + sy * 0.7, 0.78), (cx - 0.03, wm + sy * 0.7, 0.995), (cx - 0.26, wm + sy * 0.7, 0.995)], 0.02),
                   0.012, 8), 'paint', FRAME)
    k.add(parts.rod('pen', (cx - 0.25, wm + 0.42, 1.04), (cx - 0.11, wm + 0.48, 1.04), 0.0045, 8), 'plastic', 0x111111)
    chain_links(k, [(cx - 0.11, wm + 0.48, 1.04), (cx - 0.06, wm + 0.52, 1.037), (cx - 0.03, wm + 0.54, 1.036)], link=0.006, wire=0.0012, col=0xb8b8b8,
                aux=(0.25, 0, 0))
    k.add(lathe_axis('cam_base', [(0.0, 0.0), (0.06, 0.0), (0.06, 0.012), (0.0, 0.013)], T(cx - 0.03, wm, 2.95) @ RY(-90), 24), 'plastic', 0xeeebe4)
    # ---- inside the window: Augie's desk (the ticket printer, a radio, his lamp, the CCTV monitor), his stool
    dx0, dx1, dy0, dy1 = DESK
    k.add(bx('desk_top', dx0, dx1, dy0, dy1, 0.95, 0.985, 0.006), 'laminate', 0xb8ae98)
    k.add(bx('desk_body', dx0 + 0.02, dx1 - 0.02, dy0 + 0.02, dy1 - 0.02, 0.0, 0.95, 0.004), 'paint', 0x5a5e62)
    for i in range(3):
        k.add(bx('drawer_front', dx1 - 0.02, dx1 - 0.015, dy1 - 0.50, dy1 - 0.04, 0.10 + i * 0.27, 0.34 + i * 0.27, 0.003), 'paint', 0x6a6e72)
        k.add(bx('drawer_pull', dx1 - 0.015, dx1 - 0.005, dy1 - 0.33, dy1 - 0.21, 0.28 + i * 0.27, 0.295 + i * 0.27, 0.002), 'chrome')
    zt = 0.985
    k.add(bx('printer', dx0 + 0.08, dx0 + 0.30, ba - 0.42, ba - 0.16, zt, zt + 0.11, 0.01, 2), 'plastic', 0x2a2c30)
    k.add(bx('printer_slot', dx0 + 0.075, dx0 + 0.085, ba - 0.38, ba - 0.20, zt + 0.07, zt + 0.08), 'rubber', 0x050505)
    # a transistor radio, its dial and speaker grille
    rp = (dx0 + 0.38, dy1 - 0.25)
    k.add(bx('radio', rp[0] - 0.05, rp[0] + 0.05, rp[1] - 0.13, rp[1] + 0.13, zt, zt + 0.16, 0.02, 3), 'plastic', 0x6a1a14)
    k.add(bx('radio_grill', rp[0] - 0.051, rp[0] - 0.049, rp[1] - 0.10, rp[1] + 0.03, zt + 0.02, zt + 0.13), 'grill', 0xd8d0c0)
    k.add(parts.rod('radio_dial', (rp[0] - 0.052, rp[1] + 0.08, zt + 0.08), (rp[0] - 0.058, rp[1] + 0.08, zt + 0.08), 0.022, 20), 'chrome')
    k.add(parts.rod('radio_ant', (rp[0], rp[1] + 0.10, zt + 0.16), (rp[0] - 0.08, rp[1] + 0.18, zt + 0.55), 0.0025, 6), 'chrome')
    # his lamp (its bulb in _GlowWarm)
    lp = Vector((dx1 - 0.15, dy1 - 0.12, zt))
    k.add(lathe_axis('lamp_base', [(0.0, 0.0), (0.075, 0.0), (0.07, 0.02), (0.0, 0.025)], T(*lp), 24), 'enamel', 0x1c3a2c)
    k.add(tube('lamp_arm', bend([tuple(lp + Vector((0, 0, 0.02))), tuple(lp + Vector((-0.02, -0.02, 0.32))), tuple(lp + Vector((-0.10, -0.12, 0.40)))],
                                0.05), 0.007, 8), 'enamel', 0x1c3a2c)
    k.add(lathe_axis('lamp_shade', [(0.0, -0.005), (0.095, -0.005), (0.09, 0.0), (0.03, 0.10), (0.0, 0.10)], T(*cage_lamp_at()) @ RX(25.0), 24),
          'enamel', 0x1c3a2c)
    # the CCTV monitor at the desk's back corner
    Mc = T(dx1 - 0.25, dy0 + 0.25, zt) @ RZ(-90.0 - 25.0)
    crt = bx('cctv_body', -0.20, 0.20, -0.02, 0.24, 0.0, 0.30, 0.02, 3)
    crt.data.transform(Mc)
    k.add(crt, 'plastic', 0x2a2a2c)
    k.decal('cctv', tuple(Mc @ Vector((0.0, -0.0205, 0.16))), u=tuple(Mc.to_3x3() @ Vector((1, 0, 0))), v=(0, 0, 1), lift=0.0, w=0.34, h=0.22)
    k.add(bx('mug', dx0 + 0.45, dx0 + 0.53, dy0 + 0.15, dy0 + 0.23, zt, zt + 0.10, 0.03, 3), 'gloss', 0xe8e4d8)
    sxp, syp = dx1 + 0.35, wm
    k.add(parts.rod('stool_post', (sxp, syp, 0.05), (sxp, syp, 0.66), 0.02, 12), 'chrome')
    k.add(lathe_axis('stool_seat', [(0.0, 0.0), (0.17, 0.0), (0.18, 0.03), (0.17, 0.07), (0.0, 0.075)], T(sxp, syp, 0.66), 32), 'leather', 0x1a1a1a)
    for i in range(5):
        a = 2 * math.pi * i / 5
        k.add(parts.rod('stool_leg', (sxp, syp, 0.06), (sxp + 0.27 * math.cos(a), syp + 0.27 * math.sin(a), 0.02), 0.012, 8), 'black_metal')
    k.add(torus('stool_ring', (sxp, syp, 0.34), 0.18, 0.008, 'z', 28, 6), 'chrome')
    # ---- the safe at his back (its door is SM_NS_Pawn_SafeDoor)
    safe_body(k)
    k.add(bx('safe_pad', SAFE_AT[0] - 0.50, SAFE_AT[0] + 0.50, SAFE_AT[1] - 0.55, SAFE_AT[1] + 0.55, 0.0, 0.006, 0.002), 'diamond', 0x8c9096)
    # ---- the pledges: steel shelving behind the safe, bagged pledges with their tickets and labeled boxes; more down the
    # west wall; the license on the cage's back
    bx0, bx1, by0, by1 = PLEDGE_SHELF
    levels = [0.10, 0.52, 0.94, 1.36, 1.78, 2.20]

    def shelving(x0_, x1_, y0_, y1_, facing, seed):
        """Steel shelving of pledges: uprights, shelves, bagged pledges with their tickets and labeled boxes, the
        labels toward `facing` ('-x' or '+y')."""
        rnd = random.Random(seed)
        along_y = facing == '-x'
        a0, a1 = (y0_, y1_) if along_y else (x0_, x1_)
        for t in (a0, (a0 + a1) / 2, a1 - 0.035):
            for d in ((x0_, x1_ - 0.035) if along_y else (y0_, y1_ - 0.035)):
                if along_y:
                    k.add(bx('upright', d, d + 0.035, t, t + 0.035, 0.0, 2.24, 0.002), 'galv')
                else:
                    k.add(bx('upright', t, t + 0.035, d, d + 0.035, 0.0, 2.24, 0.002), 'galv')
        for zl in levels:
            k.add(bx('shelf', x0_, x1_, y0_, y1_, zl - 0.03, zl, 0.002), 'galv')
        ti = seed
        for zl in levels[:-1]:
            a = a0 + 0.06
            while a < a1 - 0.12:
                box_ = rnd.random() < 0.25
                w = rnd.uniform(0.30, 0.40) if box_ else rnd.uniform(0.18, 0.34)
                bh = rnd.uniform(0.14, 0.30)
                if box_:
                    key = f'bx{BOXES[rnd.randrange(len(BOXES))][1]}'
                    col = rnd.choice([0xb08a5a, 0xa8845a])
                    if along_y:
                        k.add(bx('box', x0_ + 0.04, x0_ + 0.42, a, a + w, zl, zl + bh, 0.004), 'cardboard', col, slot=G)
                        k.decal(key, (x0_ + 0.039, a + w / 2, zl + bh * 0.55), u=(0, -1, 0), v=(0, 0, 1), lift=0.0, w=min(0.18, w - 0.04), h=0.09)
                    else:
                        k.add(bx('box', a, a + w, y1_ - 0.42, y1_ - 0.04, zl, zl + bh, 0.004), 'cardboard', col, slot=G)
                        k.decal(key, (a + w / 2, y1_ - 0.039, zl + bh * 0.55), u=(-1, 0, 0), v=(0, 0, 1), lift=0.0, w=min(0.18, w - 0.04), h=0.09)
                else:
                    tag = f'bt{BAG_TAGS[ti % len(BAG_TAGS)][1]}'
                    ti += 1
                    if along_y:
                        pledge_bag(k, (x0_ + 0.22, a + w / 2, zl), (rnd.uniform(0.24, 0.36), w, bh), seed=ti * 7, inside=rnd.choice(INSIDE_COLS),
                                   tag=tag, slot=G)
                    else:
                        pledge_bag(k, (a + w / 2, y1_ - 0.24, zl), (w, rnd.uniform(0.24, 0.32), bh), seed=ti * 7, inside=rnd.choice(INSIDE_COLS),
                                   tag=tag, slot=G, facing='+y', subdiv=2)
                a += w + 0.015
    shelving(bx0, bx1, by0, by1, '-x', 401)
    shelving(9.45, 13.25, RY0 + 0.05, RY0 + 0.50, '+y', 431)
    k.add(bx('license_frame', RX1 - 0.02, RX1, 39.20, 39.52, 1.42, 1.82, 0.004), 'wood', 0x2a1a10, aux=(2, 0, 0))
    k.fdecal('license', (RX1 - 0.0205, 39.36, 1.62), '-x', w=0.28, h=0.36, lift=0.0)
    # a guitar case and a boxed TV waiting their turn on the floor in the cage
    k.add(bx('gcase', 9.6, 10.0, 39.5, 39.62, 0.0, 1.05, 0.04, 3), 'tolex', 0x141414, slot=G)
    k.add(bx('tvbox', 10.3, 11.2, 39.3, 39.45, 0.0, 0.62, 0.004), 'cardboard', 0xd8d0c0, slot=G)
    k.grime = [((cx - 0.15, wm, 1.04), 0.4, 0.6), ((cx, gb - 0.10, 1.0), 0.2, 0.8), ((dx0 + 0.2, wm, 0.99), 0.4, 0.4)]
    k.polish = [((cx - 0.15, wm, 1.04), 0.35, 0.6)]
    for g in cage_glass():
        k.special(g, 'ns_glass')
    return [k.finish(35.0), safe_door()]


INSIDE_COLS = [0x2a2a2a, 0x8a2a1a, 0xc8a050, 0x1c3a6e, 0x3a3a3a, 0x6a6a66, 0x1a1a1a, 0xe8661a]


def cage_lamp_at():
    dx0, dx1, dy0, dy1 = DESK
    lp = Vector((dx1 - 0.15, dy1 - 0.12, 0.985))
    return tuple(lp + Vector((-0.14, -0.16, 0.36)))


def cage_glass():
    ba, bb = BANDIT_Y
    g = quad('bandit', [(CAGE_X, bb - 0.05, 1.10), (CAGE_X, ba + 0.05, 1.10), (CAGE_X, ba + 0.05, 2.08), (CAGE_X, bb - 0.05, 2.08)])
    return [g]


def cage_glow():
    """Augie's desk lamp bulb (warm)."""
    M = T(*cage_lamp_at()) @ RX(25.0)
    lens = _lens('desk_lamp', tuple(M @ Vector((0.0, 0.0, 0.0))), tuple(M.to_3x3() @ Vector((0, 0, -1))), 0.085)
    return {'SM_NS_Pawn_Cage_GlowWarm': [xform(lens, SITE_M)]}


# ================================================================== SM_NS_Pawn_GuitarWall: 14 pledged instruments on the slatwall, amps under them

# (row, x, kind, color, burst, trans, board, guard, head, hardware, ticket's description)
GUITAR_RACK = [
    (0, 1.62, 'double', 0x8a2a10, True, False, 'rose', 0xf2efe6, 'inline', 'nickel', 'elec sunburst'),
    (0, 2.22, 'acoustic', 0xc89858, False, True, 'rose', None, '3+3', 'nickel', 'acoustic'),
    (0, 2.82, 'single', 0xc8a040, False, False, 'rose', None, '3+3', 'gold', 'gold top'),
    (0, 3.42, 'bass', 0x161618, False, False, 'rose', 0xf2efe6, 'inline', 'nickel', 'bass 4 str'),
    (0, 4.02, 'hollow', 0x9a5a1a, True, False, 'rose', None, '3+3', 'gold', 'hollowbody'),
    (0, 4.62, 'banjo', 0x5a3418, False, False, 'rose', None, '4+1', 'nickel', 'banjo 5 str'),
    (0, 5.22, 'acoustic', 0x1a1210, False, False, 'rose', None, '3+3', 'nickel', '12 string'),
    (1, 1.92, 'tele', 0xd9a441, False, True, 'maple', 0x161616, 'inline', 'nickel', 'butterscotch'),
    (1, 2.49, 'ukulele', 0xb07a40, False, True, 'rose', None, '2+2', 'nickel', 'ukulele'),
    (1, 3.06, 'offset', 0x7fb8a8, False, False, 'rose', 0xe8e0c8, 'inline', 'nickel', 'seafoam'),
    (1, 3.63, 'horns', 0x7a0e14, False, False, 'rose', None, '3+3', 'nickel', 'cherry'),
    (1, 4.20, 'acoustic', 0xd8b07a, True, False, 'rose', None, '3+3', 'nickel', 'dreadnought'),
    (1, 4.77, 'double', 0x0e0e10, False, False, 'rose', 0xf2efe6, 'inline', 'nickel', 'elec black'),
    (1, 5.34, 'double', 0xb5221c, False, False, 'maple', 0xf2efe6, 'inline', 'nickel', 'elec red'),
]
ROW_HOOK = (3.08, 1.98)            # the hooks' heights, top row and bottom row


def banjo(k, M, color=0x5a3418, seed=0, slot=None, tag=None, logo=None):
    """A five-string banjo in a guitar's frame (face -y, the pot's bottom at z 0, the neck up): the resonator, the tension
    hoop with its brackets, the white head, the bridge and tailpiece, the neck with its fifth-string peg. Returns
    (top z, hook z) as guitar() does."""
    mk = k.mark()
    R = 0.165
    cz = R
    FRn = RX(90)
    k.add(lathe_axis('resonator', [(0.0, 0.0), (R, 0.0), (R + 0.004, 0.02), (R, 0.07), (0.0, 0.07)], T(0.0, 0.035, cz) @ RX(-90), 48), 'lacquer',
          color, slot=slot, aux2=(0.0, 1.0, 0.0), aux_point=(0.0, 0.0, cz))
    k.add(torus('hoop', (0.0, -0.035, cz), R - 0.008, 0.007, 'y', 48, 6), 'chrome', slot=slot)
    k.add(parts.rod('head', (0.0, -0.036, cz), (0.0, -0.038, cz), R - 0.012, 48), 'paper', 0xf0ece0, slot=slot)
    for i in range(20):
        a = 2 * math.pi * i / 20
        p = Vector((R * 0.98 * math.cos(a), 0.0, cz + R * 0.98 * math.sin(a)))
        k.add(parts.rod('hook', tuple(p + Vector((0, -0.036, 0))), tuple(p + Vector((0, 0.02, 0))), 0.0025, 6), 'chrome', slot=slot)
        k.add(obox('shoe', tuple(p + Vector((0, 0.01, 0))), (0.012, 0.012, 0.008), rot_y=-math.degrees(a)), 'chrome', slot=slot)
    k.add(bx('bridge', -0.03, 0.03, -0.044, -0.038, cz - 0.06, cz - 0.045, 0.001), 'wood', 0xd8b47a, slot=slot)
    k.add(xz_slab('tailpiece', [(-0.025, 0.02), (0.025, 0.02), (0.012, 0.08), (-0.012, 0.08)], -0.044, -0.038, r=0.002), 'chrome', slot=slot)
    z_heel, z_nut = cz + R - 0.02, cz + R + 0.62
    neck = core.loft('neck', [[(0.024 * math.cos(math.pi * i / 12), -0.036 + 0.02 * math.sin(math.pi * i / 12) * 0.9, z) for i in range(13)] for z in
                              (z_heel, z_nut)], caps=True)
    k.add(neck, 'wood', 0x6a3a1c, slot=slot)
    k.add(bx('fretboard', -0.024, 0.024, -0.044, -0.036, z_heel, z_nut, 0.0), 'wood', 0x2e1c12, slot=slot)
    for n in range(1, 18):
        zz = z_nut - (0.66 - 0.66 / (2 ** (n / 12.0)))
        k.add(bx('fret', -0.024, 0.024, -0.0452, -0.044, zz - 0.001, zz + 0.001, 0.0004), 'nickel', slot=slot)
    head_M = T(0.0, -0.036, z_nut)
    hs = [(-0.022, 0.0), (0.022, 0.0), (0.04, 0.05), (0.035, 0.13), (0.0, 0.16), (-0.035, 0.13), (-0.04, 0.05)]
    k.add(xz_slab('peghead', closed_cr(hs, 4), 0.0, 0.016, r=0.003, matrix=head_M), 'lacquer', color, slot=slot)
    for i, (px, pz) in enumerate(((-0.03, 0.05), (-0.03, 0.11), (0.03, 0.05), (0.03, 0.11))):
        k.add(parts.rod('peg', tuple(head_M @ Vector((px, 0.0, pz))), tuple(head_M @ Vector((px * 1.9, 0.0, pz))), 0.004, 8), 'pearl', 0xf0ebe0, slot=slot)
    p5 = Vector((0.028, -0.04, z_heel + 0.22))
    k.add(parts.rod('fifth_peg', tuple(p5), tuple(p5 + Vector((0.03, 0.0, 0.0))), 0.004, 8), 'pearl', 0xf0ebe0, slot=slot)
    for i in range(5):
        xs = -0.012 + i * 0.006
        top = z_nut + 0.002 if i < 4 else p5.z
        k.add(tube('string', [(xs, -0.046, cz - 0.05), (xs * 1.2, -0.0465, top)], 0.0005, 5), 'nickel', slot=slot)
    hook_z = z_nut + 0.006
    if tag:
        anchor = Vector((0.0, -0.045, z_nut - 0.05))
        tg = anchor + Vector((0.01, -0.004, -0.09))
        k.add(tube('tag_string', [tuple(anchor), tuple((anchor + tg) / 2 + Vector((0, -0.003, 0))), tuple(tg + Vector((0, 0, 0.03)))], 0.0005, 5), 'cloth',
              0xe8e0c8, slot=slot)
        k.decal(tag, tuple(tg), u=(1, 0, 0), v=(0, 0, 1), lift=0.0, w=0.035, h=0.06)
    k.place(mk, M)
    return z_nut + 0.16, hook_z


def ukulele(k, M, color=0xb07a40, seed=0, slot=None, tag=None, logo=None):
    """A soprano ukulele (in a guitar's frame): a small figure-eight body with a soundhole, four strings, a 2+2 head."""
    mk = k.mark()
    S = 0.5
    ctrl = [(x * S, z * S) for x, z in GUITAR_SHAPES['acoustic']]
    th = 0.06
    yf = -th / 2
    k.add(xz_slab('body', closed_cr(ctrl, 6), yf, -yf, r=0.003, seg=2), 'lacquer', color, slot=slot, aux_point=(0.0, 0.0, 0.1), aux2=(0.0, 1.0, 0.0))
    k.add(xz_slab('soundhole', closed_cr([(0.022 * math.cos(a), 0.15 + 0.022 * math.sin(a)) for a in [i * math.pi / 8 for i in range(16)]], 2),
                  yf - 0.0006, yf + 0.003), 'rubber', 0x0a0806, slot=slot)
    k.add(xz_slab('bridge', [(-0.03, 0.06), (0.03, 0.06), (0.03, 0.072), (-0.03, 0.072)], yf - 0.006, yf, r=0.001), 'wood', 0x1a100a, slot=slot)
    top = 0.25
    z_nut = top + 0.20
    k.add(core.loft('neck', [[(0.018 * math.cos(math.pi * i / 12), yf + 0.016 * math.sin(math.pi * i / 12), z) for i in range(13)] for z in (top - 0.01, z_nut)],
                    caps=True), 'wood', 0xc8a068, slot=slot)
    k.add(bx('fretboard', -0.018, 0.018, yf - 0.004, yf, top - 0.01, z_nut, 0.0), 'wood', 0x2e1c12, slot=slot)
    head_M = T(0.0, yf + 0.001, z_nut + 0.003) @ RX(-10.0)
    hs = [(-0.018, 0.0), (0.018, 0.0), (0.028, 0.03), (0.026, 0.085), (0.0, 0.09), (-0.026, 0.085), (-0.028, 0.03)]
    k.add(xz_slab('head', closed_cr(hs, 4), 0.0, 0.012, r=0.002, matrix=head_M), 'lacquer', color, slot=slot)
    for (px, pz) in ((-0.02, 0.03), (-0.02, 0.065), (0.02, 0.03), (0.02, 0.065)):
        k.add(parts.rod('peg', tuple(head_M @ Vector((px, 0.0, pz))), tuple(head_M @ Vector((px * 2.0, 0.01, pz))), 0.0035, 8), 'pearl', 0xf0ebe0,
              slot=slot)
    for i in range(4):
        xs = -0.009 + i * 0.006
        k.add(tube('string', [(xs, yf - 0.007, 0.066), (xs, yf - 0.006, z_nut)], 0.0005, 5), 'plastic', 0xe8e4da, slot=slot)
    if tag:
        anchor = Vector((0.0, yf - 0.005, z_nut - 0.03))
        tg = anchor + Vector((0.01, -0.004, -0.08))
        k.add(tube('tag_string', [tuple(anchor), tuple(tg + Vector((0, 0, 0.03)))], 0.0005, 5), 'cloth', 0xe8e0c8, slot=slot)
        k.decal(tag, tuple(tg), u=(1, 0, 0), v=(0, 0, 1), lift=0.0, w=0.035, h=0.06)
    k.place(mk, M)
    return z_nut + 0.09, z_nut + 0.004


def drum_stool(k, M, slot=None):
    """A drummer's throne: a round padded seat on a threaded post and a double-braced tripod."""
    mk = k.mark()
    k.add(lathe_axis('seat', [(0.0, 0.0), (0.17, 0.0), (0.18, 0.03), (0.17, 0.08), (0.10, 0.095), (0.0, 0.097)], T(0.0, 0.0, 0.52), 36), 'leather',
          0x161616, slot=slot)
    k.add(torus('seat_band', (0.0, 0.0, 0.535), 0.176, 0.006, 'z', 36, 4), 'chrome', slot=slot)
    k.add(parts.rod('post', (0.0, 0.0, 0.22), (0.0, 0.0, 0.52), 0.016, 14), 'chrome', slot=slot)
    k.add(parts.rod('collar', (0.0, 0.0, 0.40), (0.0, 0.0, 0.43), 0.022, 14), 'chrome', slot=slot)
    for i in range(3):
        a = math.radians(90 + 120 * i)
        foot = (0.27 * math.cos(a), 0.27 * math.sin(a), 0.01)
        k.add(parts.rod('leg', (0.0, 0.0, 0.25), foot, 0.011, 10), 'chrome', slot=slot)
        k.add(parts.rod('brace', (0.0, 0.0, 0.16), (0.16 * math.cos(a), 0.16 * math.sin(a), 0.11), 0.007, 8), 'chrome', slot=slot)
        k.add(lathe_axis('foot', [(0.0, 0.0), (0.02, 0.0), (0.016, 0.02), (0.0, 0.022)], T(*foot) @ T(0, 0, -0.01), 12), 'rubber', 0x111111, slot=slot)
    k.place(mk, M)


def guitar_wall():
    k = Kit('pwg', 'SM_NS_Pawn_GuitarWall', slots=('body',), dust=0.3, low_grime=0.08, cavity=0.25)
    for i, g in enumerate(GUITAR_RACK):
        k.panel(f'gt{i}', 0.035, 0.06, p_tag(500 + i, g[-1]), density=2.2)
    for i, name in enumerate(GUITAR_LOGOS):
        k.panel(f'hl{i}', 0.07, 0.016, draw_headlogo(name), density=3.0)
    k.panel('logo_kestrel', 0.11, 0.028, logo_panel('KESTREL', 0x111111, 0xd8c8a0, 'Black', 1.3), density=2.0)
    k.panel('logo_orla', 0.11, 0.028, logo_panel('TALLIS', 0x111111, 0xe8e8e8, 'Black', 1.4), density=2.0)
    k.panel('logo_marlo', 0.11, 0.028, logo_panel('HOLLOWAY', 0xc8c0a8, 0x2a1a10, 'Black', 1.1), density=2.0)
    k.atlas.render()
    # the slatwall on the west wall: five 4 ft panels' worth, grooves with aluminum inserts every 3 in, trimmed round
    sx0, sx1 = GUITAR_X
    sz0, sz1 = 0.28, 3.26
    yw = RY0
    k.add(bx('slatwall', sx0, sx1, yw, yw + 0.019, sz0, sz1, 0.0), 'melamine', 0xcbc7be)
    zz = sz0 + 0.038
    while zz < sz1 - 0.02:
        k.add(bx('slat_insert', sx0, sx1, yw + 0.017, yw + 0.0195, zz - 0.007, zz + 0.007), 'alu')
        zz += 0.0762
    for xx in np.arange(sx0, sx1 + 0.01, 1.05):
        k.add(bx('slat_joint', xx - 0.0015, xx + 0.0015, yw + 0.018, yw + 0.0198, sz0, sz1), 'rubber', 0x3a3a3a)
    for (a0, a1, b0, b1) in ((sx0 - 0.02, sx1 + 0.02, sz1, sz1 + 0.025), (sx0 - 0.02, sx1 + 0.02, sz0 - 0.025, sz0), (sx0 - 0.02, sx0, sz0, sz1),
                             (sx1, sx1 + 0.02, sz0, sz1)):
        k.add(bx('slat_trim', a0, a1, yw, yw + 0.024, b0, b1, 0.003), 'alu')
    face = yw + 0.019
    turn_r = RZ(180.0)            # an instrument's face (-y) -> into the room (+y)
    for i, (row, x, kind, col, burst, trans, board, guard, head, hw, tag) in enumerate(GUITAR_RACK):
        hz = ROW_HOOK[row]
        # the hanger: a clip in the groove, the arm out into the room, the padded yoke
        k.add(bx('hook_clip', x - 0.02, x + 0.02, face - 0.002, face + 0.004, hz - 0.03, hz + 0.02, 0.002), 'chrome')
        k.add(tube('hook_arm', bend([(x, face + 0.004, hz), (x, face + 0.11, hz - 0.01), (x, face + 0.13, hz + 0.01)], 0.02), 0.005, 8), 'chrome')
        for sxx in (-1, 1):
            k.add(tube('yoke', bend([(x, face + 0.10, hz - 0.01), (x + sxx * 0.03, face + 0.10, hz - 0.008), (x + sxx * 0.03, face + 0.10, hz + 0.03)],
                                    0.012), 0.006, 8), 'foam', 0x1a1a1a)
        mk = k.mark()
        kw = dict(color=col, seed=600 + i, slot=None, tag=f'gt{i}', logo=f'hl{i % len(GUITAR_LOGOS)}')
        if kind == 'banjo':
            top_z, hook_z = banjo(k, Matrix(), **kw)
        elif kind == 'ukulele':
            top_z, hook_z = ukulele(k, Matrix(), **kw)
        else:
            top_z, hook_z = guitar(k, Matrix(), kind=kind, burst=burst, trans=trans, board=board, guard=guard, hw=hw, head=head,
                                   strings=4 if kind == 'bass' else 6, **kw)
        # hung from the yoke at its nut, its body's back leaning toward the slatwall
        R = turn_r @ RX(-4.0)
        nut = R @ Vector((0.0, 0.0, hook_z))
        k.place(mk, T(x - nut.x, face + 0.105 - nut.y, hz - 0.012 - nut.z) @ R)
    # under the bottom row: amps along the wall, a half stack at the back end, two hard cases, a drum stool
    ay = face + 0.17
    amp(k, T(1.55, ay, 0.0) @ turn_r, w=0.42, h=0.36, d=0.22, tolex=0x1a1a1a, grill=0x8a8478, logo='logo_orla', knobs=4)
    amp(k, T(2.20, ay + 0.02, 0.0) @ turn_r @ RZ(-4.0), w=0.62, h=0.50, d=0.26, tolex=0x141414, grill=0x7a7468, logo='logo_kestrel', knobs=6)
    hard_case(k, T(2.82, face + 0.09, 0.0) @ turn_r @ RX(-8.0), col=0x141414)
    amp(k, T(3.42, ay, 0.0) @ turn_r, w=0.56, h=0.46, d=0.25, tolex=0xd8c8a0, grill=0x6a3a2a, logo='logo_marlo', knobs=5)
    amp(k, T(4.08, ay + 0.03, 0.0) @ turn_r @ RZ(3.0), w=0.50, h=0.62, d=0.32, tolex=0x161616, grill=0x2a2a2a, panel='front', logo='logo_kestrel', knobs=6)
    stack(k, T(5.05, face + 0.22, 0.0) @ turn_r, logo='logo_kestrel')
    drum_stool(k, T(4.55, face + 0.62, 0.0) @ RZ(20.0))
    for j in range(3):
        k.add(torus('coil', (2.20 + 0.02 * j, ay, 0.51 + j * 0.008), 0.09 - j * 0.006, 0.0035, 'z', 28, 5), 'rubber', 0x111111)
    k.grime = [((3.4, face + 0.2, 0.0), 2.0, 0.3)]
    return [k.finish(35.0)]


# ================================================================== SM_NS_Pawn_Shelves: the steel shelving down the east wall, and the back stock

BAYS = [(SHELF_X[0] + i * (SHELF_X[1] - SHELF_X[0]) / 4, SHELF_X[0] + (i + 1) * (SHELF_X[1] - SHELF_X[0]) / 4) for i in range(4)]
SH_LEVELS = [0.12, 0.56, 1.00, 1.44, 1.88]      # the deck and the shelves' tops
MID_LEVELS = [0.12, 0.66, 1.20, 1.74]           # the middle bays' (the gear sockets'): 50 cm clear over each socket
GEAR_SOCKETS = []                                # G0..G11 (shop frame), filled by shelves()


def p_blank_tag(seed):
    """A hanging tag whose writing is a faint scribble: nothing legible at any reading distance (§4.6b)."""
    def draw(s, w, h):
        rnd = random.Random(seed)
        cut = w * 0.22
        s.poly([(0, 0), (w, 0), (w, h - cut), (w - cut, h), (cut, h), (0, h - cut)], 0xe9d7a6, rough=0.8)
        s.circle(w / 2, h - cut * 0.85, w * 0.07, 0x9a8a5a, rough=0.8)
        for i in range(rnd.randint(2, 3)):
            y = h * (0.55 - i * 0.16)
            pts = [(w * 0.12 + j * w * 0.08, y + rnd.uniform(-1.0, 1.0)) for j in range(10)]
            s.stroke(pts, 0.7, 0x5a6a9a, rough=0.8, alpha=0.45)
    return draw


def steel_shelving(k, x0, x1, y0, y1, levels, col=0x5a5e62, slot=None):
    """A boltless steel shelving unit: four angle posts, beams under each shelf front and back, particle-board decks."""
    for (xx, yy) in ((x0, y0), (x1 - 0.035, y0), (x0, y1 - 0.035), (x1 - 0.035, y1 - 0.035)):
        k.add(bx('post', xx, xx + 0.035, yy, yy + 0.035, 0.0, levels[-1] + 0.04, 0.002), 'paint', col, slot=slot)
        k.add(bx('foot', xx - 0.005, xx + 0.04, yy - 0.005, yy + 0.04, 0.0, 0.006, 0.002), 'rubber', 0x1a1a1a, slot=slot)
    for zl in levels:
        k.add(bx('deck', x0 + 0.005, x1 - 0.005, y0 + 0.005, y1 - 0.005, zl - 0.016, zl, 0.002), 'particle', slot=slot)
        for (ya, yb) in ((y0, y0 + 0.012), (y1 - 0.012, y1)):
            k.add(bx('beam', x0, x1, ya, yb, zl - 0.05, zl - 0.006, 0.002), 'paint', col, slot=slot)
    for (xa, xb) in ((x0, x0 + 0.012), (x1 - 0.012, x1)):
        k.add(bx('end_brace', xa, xb, y0 + 0.03, y1 - 0.03, levels[0] - 0.05, levels[0] - 0.02, 0.0), 'paint', col, slot=slot)


def cord_bin(k, M, seed=0, slot=None):
    """A plastic tote of tangled cords and wall chargers."""
    rnd = random.Random(seed)
    mk = k.mark()
    W, D, Hh = 0.42, 0.30, 0.18
    k.add(bx('bin_floor', -W / 2, W / 2, -D / 2, D / 2, 0.0, 0.006, 0.004), 'plastic', 0x2a4a8a, slot=slot)
    for (a0, a1, b0, b1) in ((-W / 2, W / 2, -D / 2, -D / 2 + 0.006), (-W / 2, W / 2, D / 2 - 0.006, D / 2), (-W / 2, -W / 2 + 0.006, -D / 2, D / 2),
                             (W / 2 - 0.006, W / 2, -D / 2, D / 2)):
        k.add(bx('bin_wall', a0, a1, b0, b1, 0.0, Hh, 0.002), 'plastic', 0x2a4a8a, slot=slot)
    for i in range(12):
        pts = [(rnd.uniform(-W / 2 + 0.03, W / 2 - 0.03), rnd.uniform(-D / 2 + 0.03, D / 2 - 0.03), rnd.uniform(0.02, Hh + 0.04)) for _ in range(6)]
        k.add(tube('cord', core.catmull_rom(pts, 5), rnd.uniform(0.0025, 0.004), 6), 'rubber', rnd.choice([0x111111, 0x1a1a1a, 0xe8e4da, 0x2a2a2a]), slot=slot)
    for i in range(4):
        k.add(obox('charger', (rnd.uniform(-0.15, 0.15), rnd.uniform(-0.1, 0.1), Hh + 0.01), (0.04, 0.03, 0.025), rot_z=rnd.uniform(0, 90), r=0.004),
              'plastic', 0xe8e4da if i % 2 else 0x141414, slot=slot)
    k.place(mk, M)


def tool_box(k, M, col=0xb3141c, w=0.50, slot=None):
    """A portable steel tool box: the body, the cantilever lid, its handle and latches."""
    mk = k.mark()
    d, h = 0.22, 0.22
    k.add(bx('tb_body', -w / 2, w / 2, -d / 2, d / 2, 0.0, h * 0.7, 0.008), 'powder', col, slot=slot)
    k.add(bx('tb_lid', -w / 2, w / 2, -d / 2, d / 2, h * 0.7, h, 0.01), 'powder', col, slot=slot)
    k.add(tube('tb_handle', bend([(-0.10, 0.0, h), (-0.09, 0.0, h + 0.05), (0.09, 0.0, h + 0.05), (0.10, 0.0, h)], 0.02), 0.009, 8), 'black_metal',
          slot=slot)
    for sx in (-1, 1):
        k.add(bx('tb_latch', sx * w * 0.3 - 0.02, sx * w * 0.3 + 0.02, -d / 2 - 0.006, -d / 2, h * 0.6, h * 0.78, 0.002), 'chrome', slot=slot)
    k.place(mk, M)


def shelves():
    k = Kit('pwe', 'SM_NS_Pawn_Shelves', slots=('body',), dust=0.35, low_grime=0.1, cavity=0.3)
    k.panel('tvbox43', 1.00, 0.62, draw_tv_box('NOVAVIEW', '43"'), density=0.7)
    for i in range(1):
        k.panel(f'spines{i}', 0.52, 0.17, p_spines(800 + i), density=1.6)
    k.panel('rcv_disp', 0.18, 0.045, draw_rcv_disp, density=2.0)
    k.panel('box_drone', 0.36, 0.28, p_box_art('AIRWREN 2', 'FOLDABLE · 4K CAMERA', 0xe8e8e8, 0x161616, 0x2a2a2a, img='drone'), density=1.2)
    k.panel('box_console', 0.36, 0.42, p_box_art('NEXA 5', 'DIGITAL EDITION · 1 TB', 0x1c1c22, 0xf2f2f2, 0x2a5ab0, img='console'), density=1.0)
    k.panel('logo_tv', 0.05, 0.007, logo_panel('NOVAVIEW', 0x141414, 0xb8b8b8, 'Bold'), density=3.0)
    k.panel('logo_console', 0.08, 0.02, logo_panel('NEXA 5', 0xe8e8e4, 0x2a2a2a, 'Black'), density=2.4)
    k.panel('logo_v', 0.10, 0.022, logo_panel('VOLTEX 20V', 0xe8661a, 0x161616, 'Black'), density=2.4)
    k.panel('logo_k', 0.10, 0.022, logo_panel('KRAGEN 18V', 0xb3141c, 0xffffff, 'Black'), density=2.4)
    k.panel('case_k', 0.16, 0.04, logo_panel('KRAGEN', 0x161616, 0xb3141c, 'Black', 1.3), density=1.8)
    k.panel('just_in', 0.50, 0.20, draw_just_in, density=1.4)
    for i in range(14):
        k.panel(f'bt{i}', 0.035, 0.06, p_blank_tag(700 + i), density=1.6)
    BOXES = [('TV', 731), ('AUDIO', 732), ('GAMES', 733), ('CORDS', 734), ('TOOLS', 735), ('PHONES', 736)]
    for t, sd in BOXES:
        k.panel(f'bx{sd}', 0.20, 0.10, p_box_label(t, sd), density=1.4)
    k.atlas.render()
    y0, y1 = SHELF_Y
    L = SH_LEVELS
    M_ = MID_LEVELS
    for bi, (a, b) in enumerate(BAYS):
        steel_shelving(k, a, b, y0, y1, M_ if bi in (1, 2) else L)
    yc = (y0 + y1) / 2 - 0.02
    # the east wall's shelves face -y (west, into the room): an item's front (-y) needs no turn

    def on(x, level, rz=0.0, dy=0.0):
        return T(x, yc + dy, L[level]) @ RZ(rz)

    def on_m(x, level, rz=0.0, dy=0.0):
        return T(x, yc + dy, M_[level]) @ RZ(rz)
    # bay 1: TVs. A 43 in boxed on the deck; the 32 in that's on ([screen]) at eye level; a soundbar; speakers above
    a, b = BAYS[0]
    box_print(k, T((a + b) / 2, yc, L[0]), 0.92, 0.14, 0.58, 'tvbox43', col=0xe8e4dc)
    k.add(bx('soundbar', a + 0.10, b - 0.10, yc - 0.05, yc + 0.05, L[1], L[1] + 0.07, 0.02, 3), 'plastic', 0x1a1a1a)
    k.add(bx('soundbar_grill', a + 0.11, b - 0.11, yc - 0.055, yc - 0.05, L[1] + 0.01, L[1] + 0.06, 0.004), 'grill', 0x2a2a2a)
    speaker(k, on(a + 0.22, 1, 0.0) @ T(0, 0, 0.0) @ Matrix.Diagonal((0.8, 0.8, 0.8, 1.0)), w=0.2, h=0.32, d=0.2)
    tv(k, on((a + b) / 2, 2), w=0.72, h=0.41, foot='feet', logo='logo_tv', screen_mat='ns_screen_pawn_tv')
    for xx in (a + 0.25, b - 0.25):
        speaker(k, on(xx, 3), w=0.22, h=0.36, d=0.22)
    hang_tag(k, (b - 0.06, y0 + 0.005, L[2] + 0.42), 'bt0', drop=0.05, facing='-y')
    hang_tag(k, (a + 0.10, y0 + 0.005, L[0] + 0.50), 'bt1', drop=0.05, facing='-y')
    # bays 2 and 3 (their own units, shelves 54 cm apart): the used-gear sockets G0..G11 on the deck and the two shelves
    # above it, two a level, each a 50 x 50 cm spot with 50 cm clear over it (PawnStage stands the gear there with
    # core-drawn tags; a wider piece takes a pair); consoles, controllers, games and a cord bin on top
    for bi in (1, 2):
        a, b = BAYS[bi]
        for lv in (0, 1, 2):
            for xs in (a + 0.278, b - 0.278):        # PawnStage.cpp's GearSockets (stage Y 4677.2 / 4627.8, 4572.2 / 4522.8)
                GEAR_SOCKETS.append((xs, yc, M_[lv]))
        if bi == 1:
            k.decal('just_in', ((a + b) / 2, y1 - 0.006, M_[3] + 0.42), u=(1, 0, 0), v=(0, 0, 1), lift=0.0, w=0.40, h=0.16)
    a, b = BAYS[1]
    console(k, on_m(a + 0.22, 3, 10.0), logo='logo_console')
    box_print(k, T(b - 0.25, yc, M_[3]), 0.36, 0.12, 0.42, 'box_console', col=0x1c1c22)
    for (dy, rz) in ((-0.12, 10.0), (0.08, -15.0)):
        controller(k, on_m(a + 0.47, 3, rz, dy))
    a, b = BAYS[2]
    k.add(bx('games', a + 0.04, (a + b) / 2 - 0.02, yc - 0.08, yc + 0.06, M_[3] + 0.002, M_[3] + 0.172, 0.0), 'plastic', 0x1a1a1a)
    k.decal('spines0', ((3 * a + b) / 4 + 0.01, yc - 0.0805, M_[3] + 0.087), u=(1, 0, 0), v=(0, 0, 1), lift=0.0, w=(b - a) / 2 - 0.06, h=0.17)
    cord_bin(k, T((a + 3 * b) / 4 + 0.01, yc, M_[3]), seed=5)
    # bay 4: laptops stacked, power tools (drills, a circular saw in its case), a tool chest on the deck
    a, b = BAYS[3]
    tool_box(k, on((a + b) / 2 - 0.12, 0, 4.0))
    tool_box(k, on((a + b) / 2 + 0.25, 0, -8.0), col=0x2a4a8a, w=0.40)
    for i in range(3):
        k.add(bx('laptop_closed', a + 0.10, a + 0.44, yc - 0.12, yc + 0.11, L[1] + i * 0.022, L[1] + 0.02 + i * 0.022, 0.006, 2), 'plastic',
              [0x2a2c30, 0xa8abb0, 0x1a1a1a][i])
    laptop(k, on(b - 0.30, 1, 5.0), col=0xa8abb0)
    tool_case(k, on((a + b) / 2, 2, 0.0), w=0.48, d=0.30, h=0.11, brand='kragen', logo='case_k')
    circ_saw(k, on((a + b) / 2, 2) @ T(0.0, 0.0, 0.11) @ RZ(15.0), brand='kragen')
    for i, xx in enumerate((a + 0.20, a + 0.48, b - 0.22)):
        drill(k, on(xx, 3, -60.0 + i * 40.0), brand=['voltex', 'kragen', 'voltex'][i], impact=i == 1, logo='logo_v' if i != 1 else 'logo_k')
    k.add(bx('drill_box', a + 0.10, a + 0.40, yc - 0.10, yc + 0.12, L[4], L[4] + 0.30, 0.004), 'cardboard', 0xb08a5a)
    box_print(k, T(b - 0.25, yc, L[4]), 0.36, 0.14, 0.28, 'box_drone', col=0xe8e8e8)
    for i, (x, z) in enumerate(((a + 0.10, L[1] + 0.06), (b - 0.12, L[2] + 0.12), (a + 0.25, L[3] + 0.28))):
        hang_tag(k, (x, y0 + 0.005, z), f'bt{3 + i}', drop=0.04, facing='-y')
    # the back stock behind the partition: two units of boxes and bags along the east wall
    rnd = random.Random(88)
    for ui, (a, b) in enumerate(((9.55, 10.85), (10.95, 12.25))):
        steel_shelving(k, a, b, y0, y1, [0.12, 0.62, 1.12, 1.62, 2.10])
        for zl in (0.12, 0.62, 1.12, 1.62, 2.10):
            xx = a + 0.05
            while xx < b - 0.15:
                w = rnd.uniform(0.22, 0.40)
                h = rnd.uniform(0.18, 0.36)
                if rnd.random() < 0.7:
                    k.add(bx('box', xx, min(xx + w, b - 0.05), y0 + 0.08, y1 - 0.05, zl, zl + h, 0.004), 'cardboard', rnd.choice([0xb08a5a, 0xa8845a, 0x9a7a50]))
                    if rnd.random() < 0.5:
                        key = f'bx{BOXES[rnd.randrange(len(BOXES))][1]}'
                        k.decal(key, (xx + min(w, b - 0.05 - xx) / 2, y0 + 0.079, zl + h * 0.5), u=(1, 0, 0), v=(0, 0, 1), lift=0.0, w=min(0.18, w - 0.04),
                                h=0.09)
                else:
                    pledge_bag(k, (xx + w / 2, (y0 + y1) / 2, zl), (w, 0.36, h), seed=int(xx * 100) + int(zl * 10), inside=rnd.choice(INSIDE_COLS),
                               tag=None)
                xx += w + 0.02
    k.grime = [((3.5, y0, 0.0), 2.0, 0.3)]
    for i, p in enumerate(GEAR_SOCKETS):
        ITEM_SPOTS[f'G{i}'] = (p, 'facing -y (stage west, into the room)')
    return [k.finish(35.0)]


# ================================================================== SM_NS_Pawn_Tickets, SM_NS_Pawn_Ticket, SM_NS_Pawn_Tag0..7

def draw_ticket_back(s, w, h):
    s.rect(0, 0, w, h, 0xf2c8c8, rough=0.8)
    s.mid('TERMS OF THIS LOAN', w / 2, h * 0.92, h * 0.035, w * 0.8, 0x2a3a6a, 'Black')
    rnd = random.Random(3)
    for i in range(24):
        y = h * (0.85 - i * 0.03)
        s.rect(w * 0.08, y, w * rnd.uniform(0.6, 0.84), h * 0.007, 0x8a7a8a)
    s.mid('BRING THIS TICKET TO REDEEM', w / 2, h * 0.05, h * 0.028, w * 0.8, 0x8a1418, 'Black')


def draw_carbon(seed):
    """A pink carbon copy of a pawn ticket on the spike: the form, the handwriting in carbon blue."""
    def draw(s, w, h):
        rnd = random.Random(seed)
        s.rect(0, 0, w, h, 0xf2c8c8, rough=0.85)
        s.mid('NORTHSIDE PAWN & LOAN', w / 2, h * 0.93, h * 0.04, w * 0.9, 0x6a3a5a, 'Black')
        s.text('No. ' + ticket_no(seed), w * 0.94, h * 0.83, h * 0.035, 0x8a1418, 'Black', align='RIGHT')
        for i in range(8):
            y = h * (0.74 - i * 0.08)
            s.rect(w * 0.06, y, w * 0.88, 0.35, 0x9a7a9a)
        for i, t in enumerate(rnd.sample(['ring 14k', 'drill', 'watch', 'laptop', 'guitar', 'chain', 'camera', 'console'], 2)):
            hand(s, t, w * 0.1, h * (0.62 - i * 0.16), h * 0.05, 0x2a3a8a, width=0.08, seed=seed + i)
        s.circle(w / 2, h * 0.5, w * 0.03, 0x5a4a4a, rough=0.9)
    return draw


def tickets():
    """The ticket book and the spike of pink carbons on Augie's desk, inside the loan window (SITE)."""
    k = Kit('pwt', 'SM_NS_Pawn_Tickets', slots=('body',), dust=0.2, low_grime=0.0, cavity=0.2)
    k.panel('pad', 0.14, 0.20, draw_ticket_pad, density=3.0)
    for i in range(6):
        k.panel(f'carbon{i}', 0.09, 0.15, draw_carbon(60 + i), density=2.4)
    k.atlas.render()
    dx0, dx1, dy0, dy1 = DESK
    zt = 0.985
    ba = BANDIT_Y[0]
    # the book, open: the bound stub side and the top ticket; its cover folded under
    bxc, byc = 6.88, ba + 0.10
    k.add(bx('book_back', bxc - 0.11, bxc + 0.11, byc - 0.16, byc + 0.16, zt, zt + 0.004, 0.002), 'cardboard', 0x2a3a6a)
    for i in range(6):
        k.add(bx('sheets', bxc - 0.10 + i * 0.0, bxc + 0.10, byc - 0.15, byc + 0.15, zt + 0.004 + i * 0.002, zt + 0.006 + i * 0.002, 0.0), 'paper',
              [0xf2ead2, 0xf2c8c8][i % 2])
    k.decal('pad', (bxc, byc, zt + 0.0165), u=(0, -1, 0), v=(1, 0, 0), lift=0.0, w=0.19, h=0.27)
    k.add(bx('binding', bxc + 0.10, bxc + 0.115, byc - 0.16, byc + 0.16, zt, zt + 0.02, 0.004), 'cloth', 0x1a2a4a)
    k.add(parts.rod('pen', (bxc - 0.04, byc + 0.20, zt + 0.006), (bxc + 0.10, byc + 0.24, zt + 0.006), 0.0045, 8), 'plastic', 0x1a2a8a)
    # the spike: a nail through a weighted base, a stack of carbons on it
    sx, sy = 6.74, dy1 - 0.30
    k.add(lathe_axis('spike_base', [(0.0, 0.0), (0.045, 0.0), (0.046, 0.004), (0.044, 0.014), (0.016, 0.018), (0.0, 0.018)], T(sx, sy, zt), 40), 'powder',
          0x161616)
    k.add(lathe_axis('spike', [(0.0, 0.0), (0.0018, 0.0), (0.0015, 0.14), (0.0, 0.165)], T(sx, sy, zt + 0.018), 12), 'steel')
    rnd = random.Random(70)
    for j in range(14):
        M = (T(sx, sy, zt + 0.024 + j * 0.0055) @ RZ(rnd.uniform(-45, 45)) @ RX(rnd.uniform(-4, 4)) @ T(0.0, -0.03, 0.0))
        k.decal(f'carbon{j % 6}', (0.0, 0.0, 0.0), u=(1, 0, 0), v=(0, 1, 0), lift=0.0, w=0.09, h=0.15, nu=2, nv=3,
                disp=lambda s_, t, a=rnd.uniform(0.004, 0.012): a * (t - 0.6) ** 2 * (1 if t < 0.6 else 0.4), matrix=M)
    return [k.finish(60.0)]


def ticket():
    """The hero's pawn ticket in the hand (PROP: origin at the grip, +x along the slip, its face up +z): the face a [screen]
    core draws as pawnticket:<n>, the back the printed terms; a little curl along its length."""
    k = Kit('pwx', 'SM_NS_Pawn_Ticket', slots=('body',), dust=0.0, low_grime=0.0, cavity=0.0, site=False)
    k.panel('back', 0.09, 0.15, draw_ticket_back, density=2.0)
    k.atlas.render()
    L, W = 0.15, 0.09
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new('UVMap')
    nu, nv = 2, 8
    grid = [[bm.verts.new((L * j / nv, W * (i / nu - 0.5), 0.008 * (j / nv - 0.2) ** 2 + 0.0002)) for i in range(nu + 1)] for j in range(nv + 1)]
    for j in range(nv):
        for i in range(nu):
            f = bm.faces.new((grid[j][i], grid[j + 1][i], grid[j + 1][i + 1], grid[j][i + 1]))
            for loop, (a, b) in zip(f.loops, ((i, j), (i, j + 1), (i + 1, j + 1), (i + 1, j))):
                loop[uvl].uv = (1.0 - a / nu, b / nv)
    face = core.mesh_object('ticket_face', bm)
    core.orient_normals(face, (0, 0, 1))
    k.special(face, 'ns_screen_pawn_ticket', locked=True)
    k.decal('back', (L / 2, 0.0, -0.0002), u=(0, 1, 0), v=(1, 0, 0), lift=0.0, nu=2, nv=8, disp=lambda s_, t: -0.008 * (t - 0.2) ** 2)
    return [k.finish(60.0)]


TAG_SHAPES = [
    ('manila', [(0.0, 0.0), (0.035, 0.0), (0.035, 0.052), (0.027, 0.06), (0.008, 0.06), (0.0, 0.052)], 0.03),
    ('round', [(0.0175 + 0.0175 * math.cos(2 * math.pi * i / 24), 0.0175 + 0.0175 * math.sin(2 * math.pi * i / 24)) for i in range(24)], 0.03),
    ('rect', [(0.0, 0.0), (0.04, 0.0), (0.04, 0.025), (0.0, 0.025)], 0.04),
    ('barbell', [(0.0, 0.0), (0.012, 0.0), (0.012, 0.022), (0.0, 0.022)], 0.02),
    ('large', [(0.0, 0.0), (0.05, 0.0), (0.05, 0.085), (0.039, 0.096), (0.011, 0.096), (0.0, 0.085)], 0.04),
    ('notch', [(0.0, 0.0), (0.03, 0.0), (0.03, 0.045), (0.022, 0.05), (0.015, 0.045), (0.008, 0.05), (0.0, 0.045)], 0.03),
    ('oval', [(0.02 + 0.02 * math.cos(2 * math.pi * i / 24), 0.013 + 0.013 * math.sin(2 * math.pi * i / 24)) for i in range(24)], 0.035),
    ('diamond', [(0.02, 0.0), (0.04, 0.02), (0.02, 0.04), (0.0, 0.02)], 0.03),
]


def tags():
    """Eight hanging-tag shapes for PawnStage to hang on its stock (PROP each: origin at the string's top, the card hanging
    below it, its face toward -x): the face a [screen] core draws the price on (UV 0..1 across the card), the back blank
    card, the string."""
    out = []
    for ti, (kind, outline, string) in enumerate(TAG_SHAPES):
        k = Kit(f'pwtg{ti}', f'SM_NS_Pawn_Tag{ti}', slots=('body',), dust=0.0, low_grime=0.0, cavity=0.0, site=False)
        xs = [p[0] for p in outline]
        zs = [p[1] for p in outline]
        w, h = max(xs) - min(xs), max(zs) - min(zs)
        drop = string + h
        # the card in the YZ plane facing -x, its top `string` below the origin
        pts = [(y - w / 2, z - drop) for y, z in outline]
        k.add(yz_slab('card', pts, 0.0002, 0.0006), 'paper', 0xe9d7a6)
        bm = bmesh.new()
        uvl = bm.loops.layers.uv.new('UVMap')
        f = bm.faces.new([bm.verts.new((0.0, -p[0], p[1])) for p in pts])
        for loop, p in zip(f.loops, pts):
            loop[uvl].uv = ((p[0] + w / 2) / w, (p[1] + drop) / h)
        face = core.mesh_object('face', bm)
        core.orient_normals(face, (-1, 0, 0))
        k.special(face, 'ns_screen_pawn_tag', locked=True)
        k.add(parts.rod('eyelet', (0.0003, 0.0, -string - 0.003), (0.0006, 0.0, -string - 0.003), 0.0025, 10), 'brass')
        k.add(tube('string', [(0.0, 0.0, 0.0), (-0.002, 0.003, -string * 0.5), (0.0003, 0.0, -string - 0.003)], 0.0005, 5), 'cloth', 0xe8e0c8)
        out.append(k.finish(60.0))
    return out


# ================================================================== SM_NS_Pawn_Loupe (PROP: origin at the grip)

def loupe():
    """A jeweler's 10x loupe on a cord (PROP, as §4.5's hand props: origin at the grip, +x along it, z up): the folding
    chrome housing open, the black lens barrel, the cord's loop."""
    k = Kit('pwl', 'SM_NS_Pawn_Loupe', slots=('body',), dust=0.0, low_grime=0.0, cavity=0.2, site=False)
    # the housing: two curved chrome shells hinged at the grip, the barrel swung out on +x
    for side in (-1, 1):
        prof = [(0.0, 0.0), (0.016, 0.0), (0.018, 0.004), (0.018, 0.008), (0.0, 0.008)]
        o = lathe_axis('shell', prof, T(0.0, 0.0, side * 0.006) @ (RX(180) if side < 0 else Matrix()), 24)
        k.add(o, 'chrome')
    k.add(parts.rod('pivot', (0.0, 0.0, -0.012), (0.0, 0.0, 0.012), 0.003, 10), 'chrome')
    k.add(lathe_axis('barrel', [(0.0, 0.0), (0.012, 0.0), (0.013, 0.004), (0.012, 0.022), (0.0, 0.022)], T(0.03, 0.0, -0.011), 24), 'black_metal')
    k.add(parts.rod('lens', (0.03, 0.0, 0.0112), (0.03, 0.0, 0.0118), 0.0105, 20), 'screen')
    k.add(parts.rod('lens_b', (0.03, 0.0, -0.0112), (0.03, 0.0, -0.0118), 0.0105, 20), 'screen')
    k.add(tube('arm', [(0.0, 0.0, 0.0), (0.03, 0.0, 0.0)], 0.004, 8), 'chrome')
    k.add(tube('cord', bend([(-0.012, 0.0, 0.0), (-0.05, 0.01, -0.02), (-0.10, 0.02, -0.06), (-0.11, -0.02, -0.08), (-0.06, -0.015, -0.03),
                             (-0.014, 0.0, 0.0)], 0.02), 0.0012, 6), 'cloth', 0x1a1a1a)
    return [k.finish(50.0)]


# ================================================================== build, bake, assemble


_SPLIT = {}


def _delete_faces(obj, test):
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if test(f)], context='FACES')
    bm.to_mesh(obj.data)
    bm.free()


def _prune_slots(obj):
    me = obj.data
    fi = np.zeros(len(me.polygons), dtype=np.int32)
    me.polygons.foreach_get('material_index', fi)
    used = sorted(set(fi.tolist()))
    mats = [obj.material_slots[i].material for i in used]
    remap = {old: new for new, old in enumerate(used)}
    me.materials.clear()
    for mt in mats:
        me.materials.append(mt)
    me.polygons.foreach_set('material_index', np.array([remap[i] for i in fi], dtype=np.int32))


def bake_parts(objs):
    """The glass and glow meshes aren't baked (the game shades them). A baked mesh's unbaked slots (a door's glass, a
    [screen]) are split off, hidden from its bake (so they don't shade what's behind them) and joined back after it."""
    _SPLIT.clear()
    bake = []
    for o in objs:
        if o.name in GLASS_MESHES or o.name in GLOW_MESHES:
            o.hide_render = True
            continue
        idx = [i for i, s in enumerate(o.material_slots) if s.material and s.material.name in UNBAKED_MATS]
        if idx:
            dup = o.copy()
            dup.data = o.data.copy()
            core.link(dup)
            dup.name = o.name + '__unbaked'
            _delete_faces(o, lambda f: f.material_index in idx)
            _delete_faces(dup, lambda f: f.material_index not in idx)
            _prune_slots(o)
            _prune_slots(dup)
            dup.hide_render = True
            _SPLIT[o.name] = dup
        bake.append(o)
    _BAKED[:] = bake
    return bake


def assemble(baked):
    by = {o.name: o for o in baked}
    out = []
    for name in MESHES:
        o = by.get(name) or bpy.data.objects.get(name)
        if o is None:
            continue
        dup = _SPLIT.get(name)
        if dup is not None:
            dup.hide_render = False
            o = core.join([o, dup], name)
            o.name = name
            o.data.name = name
        o.hide_render = False
        out.append(o)
    for o in out:
        print(f'[ns_pawn] {o.name}: {tri_count(o)} triangles, slots {[s.material.name for s in o.material_slots]}')
    return out


# ================================================================== review

SCRATCH = os.path.join(core.BUILD_DIR, 'review', 'ns_pawn')


def _which_review():
    v = os.environ.get('PAWN_REVIEW', 'detail,room,street')
    return {s.strip() for s in v.split(',') if s.strip()}


def _rv_mat(name, hex_color, rough=0.5, metal=0.0, emit=None, strength=0.0):
    m = core.Mat(name)
    m.set('Base Color', H(hex_color))
    m.set('Roughness', rough)
    m.set('Metallic', metal)
    if emit is not None:
        m.set('Emission Color', H(emit))
        m.set('Emission Strength', strength)
    return m


def _light(name, kind_, loc, energy, color, size=0.1, target=None, spot=None, blend=0.6):
    data = bpy.data.lights.new(name, kind_)
    data.energy = energy
    data.color = color
    if kind_ in ('POINT', 'SPOT'):
        data.shadow_soft_size = size if not isinstance(size, tuple) else size[0]
    if kind_ == 'AREA':
        data.shape = 'RECTANGLE'
        data.size, data.size_y = size if isinstance(size, tuple) else (size, size)
    if kind_ == 'SPOT' and spot:
        data.spot_size = math.radians(spot)
        data.spot_blend = blend
    obj = core.link(bpy.data.objects.new(name, data))
    obj.location = loc
    if target is not None:
        d = Vector(target) - Vector(loc)
        obj.rotation_euler = d.to_track_quat('-Z', 'Y').to_euler()
    return obj


def _cam(loc, target, lens=35.0):
    sc = bpy.context.scene
    cam = bpy.data.objects.get('pw_cam')
    if cam is None:
        data = bpy.data.cameras.new('pw_cam')
        cam = core.link(bpy.data.objects.new('pw_cam', data))
    sc.camera = cam
    cam.data.lens = lens
    cam.data.clip_start = 0.02
    cam.data.clip_end = 300.0
    cam.location = loc
    d = Vector(target) - Vector(loc)
    cam.rotation_euler = d.to_track_quat('-Z', 'Y').to_euler()
    return cam


def _shot(path, w, h, samples=64):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    return review._render(path, w, h, samples)


def compose_grid(name, paths, cols):
    imgs = [review._pixels(p) for p in paths]
    h, w = imgs[0].shape[:2]
    rows = (len(imgs) + cols - 1) // cols
    gap = 6
    W = cols * w + (cols - 1) * gap
    Hh = rows * h + (rows - 1) * gap
    out = np.zeros((Hh, W, 4), np.float32)
    out[:, :, :3] = 0.05
    out[:, :, 3] = 1.0
    for k, im in enumerate(imgs):
        r, c = k // cols, k % cols
        y0 = Hh - (r + 1) * h - r * gap
        x0 = c * (w + gap)
        out[y0:y0 + h, x0:x0 + w] = im
    os.makedirs(core.REVIEW_DIR, exist_ok=True)
    path = os.path.join(core.REVIEW_DIR, f'{name}.jpg')
    img = bpy.data.images.new(f'rv_{name}', W, Hh, alpha=False)
    img.pixels.foreach_set(out.ravel())
    img.filepath_raw = path
    img.file_format = 'JPEG'
    img.save(quality=90)
    bpy.data.images.remove(img)
    print(f'[ns_pawn] review sheet {os.path.relpath(path, core.ROOT)}')
    return path


def _detail_review(by, tmp):
    """Each mesh alone under a soft key and a cool fill on a grey floor (art/review/ns_pawn_detail_N.jpg)."""
    sc = bpy.context.scene
    sc.world.color = (0.035, 0.036, 0.04)
    floor = core.link(bpy.data.objects.new('rv_floor', bpy.data.meshes.new('rv_floor')))
    floor.data.from_pydata([(-40, -40, 0), (60, -40, 0), (60, 60, 0), (-40, 60, 0)], [], [(0, 1, 2, 3)])
    core.assign(floor, _rv_mat('rv_floor_m', 0x5a5852, 0.6))
    tmp.append(floor)
    paths = []
    for name, shots in DETAIL_SHOTS.items():
        if name not in by:
            continue
        show = {name} | {n for n in by if n.startswith(name + '_')}
        for n, o in by.items():
            o.hide_render = n not in show
        for i, (loc, tgt, lens) in enumerate(shots):
            L, Tg = Vector(loc), Vector(tgt)
            d = (Tg - L)
            side = d.cross(Vector((0, 0, 1))).normalized()
            key = _light('rv_key', 'AREA', tuple(L + side * -1.2 + Vector((0, 0, 1.4))), 140.0, (1.0, 0.92, 0.82), 1.2, tgt)
            fill = _light('rv_fill', 'AREA', tuple(L + side * 1.5 + Vector((0, 0, 0.3))), 45.0, (0.75, 0.85, 1.0), 1.5, tgt)
            rim = _light('rv_rim', 'AREA', tuple(Tg + d.normalized() * 1.5 + Vector((0, 0, 1.5))), 90.0, (1.0, 1.0, 1.0), 1.0, tgt)
            _cam(loc, tgt, lens)
            paths.append(_shot(os.path.join(SCRATCH, f'd_{name}_{i}.png'), 760, 760, 64))
            for l in (key, fill, rim):
                bpy.data.objects.remove(l)
    for o in by.values():
        o.hide_render = False
    floor.hide_render = True
    out = []
    for i in range(0, len(paths), 16):
        out.append(compose_grid(f'ns_pawn_detail{os.environ.get("PAWN_TAG", "")}_{i // 16 + 1}', paths[i:i + 16], 4))
    return out


def _concrete_mat(name, hexc=0x8a867e, joint=1.5):
    m = core.Mat(name)
    tc, sep = object_coords(m)
    n = noise(m, tc.outputs['Object'], scale=4.0, detail=5.0)
    fine = noise(m, tc.outputs['Object'], scale=60.0, detail=3.0)
    col = m.mix(m.math('MULTIPLY', n, 0.4), H(hexc), H(0x5e5a54))
    col = m.mix(m.math('MULTIPLY', fine, 0.2), col, H(0xa8a49a))
    jx = smooth_less(m, m.math('ABSOLUTE', m.math('SUBTRACT', m.math('FRACT', m.math('DIVIDE', sep.outputs['Y'], joint)), 0.5)), 0.495, 0.002)
    col = m.mix(m.math('SUBTRACT', 1.0, jx), col, H(0x3a3834))
    m.set('Base Color', col)
    m.set('Roughness', m.math('ADD', 0.75, m.math('MULTIPLY', n, 0.15)))
    return m


def _brick_mat(name, hexa=0xb9b2a3, hexb=0xa8a090, mortar=0x8d877c):
    m = core.Mat(name)
    tc, sep = object_coords(m)
    br = m.node('ShaderNodeTexBrick', Scale=1.0)
    br.inputs['Brick Width'].default_value = 0.2
    br.inputs['Row Height'].default_value = 0.0667
    br.inputs['Mortar Size'].default_value = 0.01
    br.inputs['Color1'].default_value = H(hexa)
    br.inputs['Color2'].default_value = H(hexb)
    br.inputs['Mortar'].default_value = H(mortar)
    m.link(vec(m, sep.outputs['Y'], sep.outputs['Z'], sep.outputs['X']), br.inputs['Vector'])
    n = noise(m, tc.outputs['Object'], scale=3.0, detail=4.0)
    col = m.mix(m.math('MULTIPLY', n, 0.3), br.outputs['Color'], H(0x6a6458))
    m.set('Base Color', col)
    m.set('Roughness', 0.85)
    return m


def _glow_on(by, on=True):
    for n, (col, s) in GLOW_RV.items():
        o = by.get(n)
        if o is None or not o.material_slots:
            continue
        m = o.material_slots[0].material
        b = next(nd for nd in m.node_tree.nodes if nd.type == 'BSDF_PRINCIPLED')
        b.inputs['Emission Color'].default_value = (col[0], col[1], col[2], 1.0)
        b.inputs['Emission Strength'].default_value = s if on else 0.0
        b.inputs['Base Color'].default_value = (col[0] * 0.5 + 0.3, col[1] * 0.5 + 0.3, col[2] * 0.5 + 0.3, 1.0)


# ================================================================== the glow meshes, build

# ================================================================== SM_NS_Pawn_Bench (+ _GlowCool): the repair bench behind the counter (P1)

BENCH = (8.30, PARTITION_X - 0.06, 42.30, 44.45)   # x0, x1 (its back on the back-stock wall), y0, y1
BENCH_TOP = 0.90
MAGNIFIER = (8.52, 43.05, 1.32)                    # the magnifier lamp's lens center (shop frame)


def draw_drawer_labels(s, w, h):
    """The parts cabinet's face: a blue-gray frame, twelve clear drawer fronts showing their parts, hand-lettered labels."""
    s.rect(0, 0, w, h, 0x5a6a7a, rough=0.5)
    rnd = random.Random(4)
    labels = ['JUMP RINGS', 'CLASPS', 'PINS', 'BATTERIES', 'CRYSTALS', 'STONES', 'SOLDER', 'SCREWS', 'CROWNS', 'BANDS', 'LINKS', 'MISC']
    cols, rows = 4, 3
    cw, rh = w / cols, h / rows
    for i, lab in enumerate(labels):
        c_, r_ = i % cols, i // cols
        x0, y0 = c_ * cw, h - (r_ + 1) * rh
        s.rect(x0 + 3, y0 + 3, cw - 6, rh - 6, 0x2c3238, rough=0.15)
        for j in range(18):
            s.circle(x0 + rnd.uniform(8, cw - 8), y0 + rnd.uniform(6, rh * 0.42), rnd.uniform(1.0, 2.6),
                     rnd.choice([0xc8a050, 0xb8b8b8, 0x8a8a8a, 0xd8c890, 0x6a6a6a]), metal=0.6, rough=0.4)
        s.rect(x0 + 3, y0 + rh - 9, cw - 6, 2.0, 0xd8e0e8, rough=0.1, alpha=0.5)
        s.rect(x0 + cw * 0.3, y0 + 5, cw * 0.4, 3.0, 0x8a96a2, rough=0.4)
        s.rect(x0 + 8, y0 + rh * 0.52, cw - 16, rh * 0.30, 0xf2eee4, rough=0.8)
        hand(s, lab.lower()[:10], x0 + 11, y0 + rh * 0.60, rh * 0.15, 0x161616, width=0.08, seed=i + 3)


def draw_ultrasonic(s, w, h):
    s.rect(0, 0, w, h, 0x2a2c30, rough=0.4)
    s.rect(w * 0.08, h * 0.45, w * 0.4, h * 0.35, 0x0a1a10, rough=0.05)
    s.mid('03:00', w * 0.28, h * 0.62, h * 0.18, w * 0.34, 0x6effc8, 'Bold')
    for i, lab in enumerate(('ON', 'TIMER', 'HEAT')):
        s.circle(w * (0.62 + i * 0.12), h * 0.62, h * 0.09, 0x6a6a6a)
        s.mid(lab, w * (0.62 + i * 0.12), h * 0.3, h * 0.08, w * 0.11, 0xe8e8e8, 'Bold')


def draw_solder_face(s, w, h):
    s.rect(0, 0, w, h, 0x1a3a6e, rough=0.4)
    s.rect(w * 0.1, h * 0.55, w * 0.8, h * 0.3, 0x081008, rough=0.05)
    s.mid('350', w * 0.5, h * 0.7, h * 0.2, w * 0.6, 0xff5a3a, 'Bold')
    s.circle(w * 0.5, h * 0.25, h * 0.12, 0xd8d8d8)


def bench():
    k = Kit('pwb', 'SM_NS_Pawn_Bench', slots=('body',), dust=0.35, low_grime=0.15, cavity=0.3)
    k.panel('drawers', 0.36, 0.22, draw_drawer_labels, density=2.0)
    k.panel('ultra', 0.16, 0.06, draw_ultrasonic, glow=1.5, density=2.4)
    k.panel('solder', 0.08, 0.10, draw_solder_face, glow=1.5, density=2.4)
    k.atlas.render()
    x0, x1, y0, y1 = BENCH
    zt = BENCH_TOP
    # the bench: a scarred maple top, apron, legs, a drawer pedestal, a shelf under; a bench pin clamped to its front
    k.add(bx('top', x0, x1, y0, y1, zt - 0.045, zt, 0.006, 3), 'wood', 0xb08850, aux=(2, 0, 0))
    for yy in (y0 + 0.03, y1 - 0.08):
        for xx in (x0 + 0.03, x1 - 0.08):
            k.add(bx('leg', xx, xx + 0.05, yy, yy + 0.05, 0.0, zt - 0.045, 0.004), 'wood', 0x6a4a2a, aux=(0, 0, 0))
    k.add(bx('shelf', x0 + 0.04, x1 - 0.04, y0 + 0.04, y1 - 0.04, 0.12, 0.14, 0.003), 'particle')
    k.add(bx('pedestal', x0 + 0.02, x1 - 0.02, y1 - 0.55, y1 - 0.05, 0.14, zt - 0.06, 0.004), 'paint', 0x5a5e62)
    for i in range(4):
        z0 = 0.17 + i * 0.17
        k.add(bx('drawer', x0 + 0.015, x0 + 0.02, y1 - 0.53, y1 - 0.07, z0, z0 + 0.15, 0.003), 'paint', 0x6a6e72)
        k.add(bx('pull', x0 + 0.0, x0 + 0.015, y1 - 0.36, y1 - 0.24, z0 + 0.11, z0 + 0.125, 0.002), 'chrome')
    k.add(bx('bench_pin', x0 - 0.12, x0 + 0.03, 42.95, 43.05, zt - 0.02, zt, 0.003), 'wood', 0xd8b47a, aux=(1, 0, 0))
    k.add(bx('pin_clamp', x0 - 0.01, x0 + 0.06, 42.94, 43.06, zt - 0.08, zt + 0.012, 0.004), 'cast')
    # the ring mandrel on its block, the sizer rings on their key ring, pliers and files in a holder
    k.add(bx('mandrel_block', x0 + 0.06, x0 + 0.18, 43.30, 43.40, zt, zt + 0.05, 0.004), 'wood', 0x5a3418, aux=(1, 0, 0))
    k.add(lathe_axis('mandrel', [(0.0, 0.0), (0.012, 0.0), (0.008, 0.30), (0.0, 0.31)], T(x0 + 0.12, 43.35, zt + 0.05), 20), 'steel')
    for j in range(5):
        k.add(torus('sizer', (x0 + 0.12, 43.35, zt + 0.10 + j * 0.035), 0.0095 + j * 0.0012, 0.0025, 'z', 20, 4), 'steel')
    k.add(torus('sizer_ring', (x0 + 0.26, 43.18, zt + 0.004), 0.05, 0.002, 'z', 28, 4), 'steel')
    rnd = random.Random(12)
    for j in range(14):
        a = 2 * math.pi * j / 14
        k.add(torus('sizer', (x0 + 0.26 + 0.05 * math.cos(a), 43.18 + 0.05 * math.sin(a), zt + 0.004), 0.009 + rnd.uniform(0, 0.004), 0.0018, 'y' if j % 2
                        else 'x', 16, 4), 'steel')
    k.add(lathe_axis('tool_cup', [(0.0, 0.0), (0.05, 0.0), (0.05, 0.12), (0.0, 0.12)], T(x1 - 0.12, 43.85, zt), 24), 'paint', 0x2a2a2a)
    for j in range(7):
        a = rnd.uniform(0, 6.28)
        b = Vector((x1 - 0.12 + 0.025 * math.cos(a), 43.85 + 0.025 * math.sin(a), zt + 0.02))
        t = b + Vector((rnd.uniform(-0.05, 0.05), rnd.uniform(-0.05, 0.05), 0.18))
        k.add(parts.rod('tool', tuple(b), tuple(t), 0.004, 6), 'steel' if j % 2 else 'plastic', 0xb3141c)
    # the ultrasonic cleaner: a stainless tank with its lid and the control face
    ux, uy = x0 + 0.22, 43.80
    k.add(bx('ultra', ux - 0.12, ux + 0.12, uy - 0.09, uy + 0.09, zt, zt + 0.13, 0.01, 2), 'steel')
    k.add(bx('ultra_lid', ux - 0.115, ux + 0.115, uy - 0.085, uy + 0.085, zt + 0.13, zt + 0.15, 0.008), 'plastic', 0xd8dcd8)
    k.decal('ultra', (ux - 0.1205, uy, zt + 0.065), u=(0, -1, 0), v=(0, 0, 1), lift=0.0, w=0.16, h=0.06)
    # the soldering station: its box with the dial, the iron in its spiral holder, the sponge tray
    sx, sy = x0 + 0.20, 42.60
    k.add(bx('solder_box', sx - 0.06, sx + 0.06, sy - 0.07, sy + 0.07, zt, zt + 0.10, 0.008, 2), 'plastic', 0x1a3a6e)
    k.decal('solder', (sx - 0.0605, sy, zt + 0.05), u=(0, -1, 0), v=(0, 0, 1), lift=0.0, w=0.08, h=0.09)
    k.add(tube('solder_spiral', [(sx + 0.10 + 0.02 * math.cos(t * 0.8), sy + 0.12 + 0.02 * math.sin(t * 0.8), zt + 0.02 + t * 0.008) for t in
                                 range(14)], 0.002, 6), 'chrome')
    k.add(parts.rod('iron', (sx + 0.10, sy + 0.12, zt + 0.05), (sx + 0.24, sy + 0.20, zt + 0.12), 0.009, 12), 'rubber', 0x1a1a1a)
    k.add(tube('iron_cord', core.catmull_rom([(sx + 0.24, sy + 0.20, zt + 0.12), (sx + 0.15, sy + 0.25, zt + 0.02), (sx, sy + 0.05, zt + 0.06)], 6),
               0.003, 6), 'rubber', 0x1a1a1a)
    # the parts drawers: a small cabinet of clear-fronted plastic drawers, labeled by hand
    px, py = x1 - 0.12, 42.75
    k.add(bx('cabinet', px - 0.10, px + 0.10, py - 0.19, py + 0.19, zt, zt + 0.24, 0.004), 'plastic', 0x6a7a8a)
    k.decal('drawers', (px - 0.1005, py, zt + 0.12), u=(0, -1, 0), v=(0, 0, 1), lift=0.0, w=0.36, h=0.22)
    # the magnifier lamp: a clamp on the bench's back edge, two arms with their springs, the round head (its ring glows)
    cx_, cy_ = x1 - 0.04, 43.30
    k.add(bx('clamp', cx_ - 0.03, cx_ + 0.03, cy_ - 0.03, cy_ + 0.03, zt - 0.06, zt + 0.04, 0.004), 'enamel', 0xe8e4da)
    mx, my, mz = MAGNIFIER
    elbow = Vector((cx_ - 0.05, cy_ - 0.05, zt + 0.45))
    k.add(tube('arm1', [(cx_, cy_, zt + 0.04), tuple(elbow)], 0.007, 8), 'enamel', 0xe8e4da)
    k.add(tube('arm2', [tuple(elbow), (mx + 0.10, my + 0.05, mz + 0.05)], 0.007, 8), 'enamel', 0xe8e4da)
    for a, b in (((cx_, cy_, zt + 0.04), tuple(elbow)), (tuple(elbow), (mx + 0.10, my + 0.05, mz + 0.05))):
        # each arm's tension spring: a short coil alongside the arm's middle, its hooked ends on two studs
        A, B = Vector(a), Vector(b)
        ax = (B - A).normalized()
        off = ax.cross(Vector((0, 0, 1))).normalized() * 0.016 if abs(ax.z) < 0.95 else Vector((0.016, 0, 0))
        s0, s1 = A.lerp(B, 0.32) + off, A.lerp(B, 0.68) + off
        k.add(coil('spring', s0, s1, 0.0045, 0.0009, 16, 8), 'steel')
        for p, q in ((A.lerp(B, 0.27), s0), (A.lerp(B, 0.73), s1)):
            k.add(parts.rod('stud', tuple(p), tuple(q), 0.0018, 6), 'steel')
    M = T(mx, my, mz) @ RY(-20.0)
    k.add(lathe_axis('mag_head', [(0.0, 0.0), (0.075, 0.0), (0.078, 0.01), (0.078, 0.045), (0.06, 0.05), (0.0, 0.05)], M, 40), 'enamel', 0xe8e4da)
    k.add(parts.rod('mag_lens', tuple(M @ Vector((0, 0, -0.002))), tuple(M @ Vector((0, 0, 0.0))), 0.05, 32), 'screen')
    # a stool at the bench
    stx, sty = x0 - 0.40, 43.20
    k.add(parts.rod('stool_post', (stx, sty, 0.05), (stx, sty, 0.62), 0.02, 12), 'chrome')
    k.add(lathe_axis('stool_seat', [(0.0, 0.0), (0.16, 0.0), (0.17, 0.03), (0.16, 0.06), (0.0, 0.065)], T(stx, sty, 0.62), 32), 'leather', 0x3a1a14)
    for i in range(4):
        a = math.pi / 4 + i * math.pi / 2
        k.add(parts.rod('stool_leg', (stx, sty, 0.08), (stx + 0.24 * math.cos(a), sty + 0.24 * math.sin(a), 0.02), 0.012, 8), 'black_metal')
    k.grime = [((x0 + 0.1, 43.2, zt), 0.4, 0.6)]
    return [k.finish(35.0)]


def bench_glow():
    """The magnifier lamp's ring tube (cool)."""
    mx, my, mz = MAGNIFIER
    M = T(mx, my, mz) @ RY(-20.0)
    ring = torus('mag_ring', (0, 0, 0), 0.064, 0.007, 'z', 40, 8)
    ring.data.transform(M @ T(0, 0, 0.02))
    return {'SM_NS_Pawn_Bench_GlowCool': [xform(ring, SITE_M)]}


# ================================================================== SM_NS_Pawn_Floor (+ _Glass): the floor pieces (P1)

FLOOR_AT = {'tower': (1.80, 41.15, 0.0), 'slot': (2.62, 41.05, -90.0), 'chest': (3.35, 41.10, -90.0), 'bike': (3.40, 42.35, 0.0),
            'bust': (4.25, 41.15, -90.0)}


def draw_not_for_sale(s, w, h):
    s.rect(0, 0, w, h, 0xf3f1ea, rough=0.6)
    s.rect(2, 2, w - 4, h - 4, 0xb3141c, rough=0.6)
    s.rect(4, 4, w - 8, h - 8, 0xf3f1ea, rough=0.6)
    hand(s, 'NOT FOR SALE', w * 0.07, h * 0.36, h * 0.3, 0xb3141c, width=0.1, seed=3)


def draw_reels(s, w, h):
    s.rect(0, 0, w, h, 0xf2efe6, rough=0.3)
    syms = [('7', 0xb3141c), ('BAR', 0x161616), ('7', 0xb3141c)]
    for i, (t, c) in enumerate(syms):
        x0 = i * w / 3
        s.rect(x0 + 1, 0, w / 3 - 2, h, 0xfaf8f0, rough=0.3)
        s.mid(t, x0 + w / 6, h * 0.5, h * 0.42, w / 3 * 0.8, c, 'Black')
    s.rect(0, h * 0.48, w, 0.8, 0xb3141c, alpha=0.6)


def draw_slot_glass(s, w, h):
    s.rect(0, 0, w, h, 0x1a1a3a, rough=0.2)
    s.mid('LUCKY', w / 2, h * 0.72, h * 0.22, w * 0.8, 0xf2c860, 'Black', tracking=1.15)
    s.mid('BELL', w / 2, h * 0.38, h * 0.26, w * 0.8, 0xf2c860, 'Black', tracking=1.15)
    for i in range(12):
        s.circle(w * (0.06 + i * 0.08), h * 0.1, h * 0.04, [0xb3141c, 0xf2c860][i % 2])


def draw_two_dollar(s, w, h):
    """A stylized two-dollar note in its frame (a keepsake, not a facsimile: no seals, no serials, a blank oval)."""
    s.rect(0, 0, w, h, 0xe8e6d4, rough=0.6)
    s.rect(w * 0.03, h * 0.05, w * 0.94, h * 0.9, 0x5a7a5a, rough=0.6)
    s.rect(w * 0.05, h * 0.09, w * 0.90, h * 0.82, 0xe8e6d4, rough=0.6)
    s.ring(w * 0.5, h * 0.5, h * 0.22, h * 0.25, 0x3a5a3a)
    for x in (0.12, 0.88):
        for y in (0.22, 0.78):
            s.mid('2', w * x, h * y, h * 0.18, w * 0.1, 0x3a5a3a, 'Black')
    s.mid('TWO DOLLARS', w / 2, h * 0.16, h * 0.08, w * 0.4, 0x3a5a3a, 'Black', tracking=1.3)


def slot_machine(k, M, slot=None):
    """A 1960s three-reel slot machine on its stand: the cabinet with chrome trim, the lit-look marquee glass, the reel window,
    the side handle with its red ball, the payout cup; NOT FOR SALE taped on."""
    mk = k.mark()
    # the stand
    k.add(bx('stand', -0.24, 0.24, -0.22, 0.22, 0.0, 0.62, 0.01), 'wood', 0x4a2a14, aux=(0, 0, 0))
    k.add(bx('stand_top', -0.26, 0.26, -0.24, 0.24, 0.62, 0.65, 0.008), 'wood', 0x5a3418, aux=(1, 0, 0))
    z0 = 0.65
    k.add(bx('cabinet', -0.20, 0.20, -0.18, 0.18, z0, z0 + 0.52, 0.02, 3), 'enamel', 0x7a1a14)
    k.add(xz_slab('crown', closed_cr([(-0.20, 0.0), (0.20, 0.0), (0.19, 0.10), (0.12, 0.16), (-0.12, 0.16), (-0.19, 0.10)], 3), -0.17, 0.17, r=0.012),
          'enamel', 0x7a1a14)
    k.objs[-1].data.transform(T(0.0, 0.0, z0 + 0.52))
    k.add(bx('trim', -0.205, 0.205, -0.185, -0.17, z0 + 0.20, z0 + 0.215, 0.002), 'chrome')
    k.add(bx('trim2', -0.205, 0.205, -0.185, -0.17, z0 + 0.46, z0 + 0.475, 0.002), 'chrome')
    k.add(bx('reel_frame', -0.15, 0.15, -0.19, -0.17, z0 + 0.25, z0 + 0.42, 0.004), 'chrome')
    k.decal('reels', (0.0, -0.1905, z0 + 0.335), u=(1, 0, 0), v=(0, 0, 1), lift=0.0, w=0.26, h=0.13)
    k.decal('slotglass', (0.0, -0.181, z0 + 0.60), u=(1, 0, 0), v=(0, 0, 1), lift=0.0, w=0.34, h=0.12)
    k.add(bx('payout', -0.10, 0.10, -0.24, -0.17, z0 + 0.04, z0 + 0.10, 0.01, 2), 'chrome')
    k.add(bx('coin_slot', 0.08, 0.12, -0.181, -0.17, z0 + 0.48, z0 + 0.50, 0.002), 'chrome')
    k.add(tube('handle', [(0.20, 0.0, z0 + 0.30), (0.26, 0.0, z0 + 0.30), (0.27, 0.0, z0 + 0.62)], 0.012, 10), 'chrome')
    k.add(lathe_axis('handle_ball', [(0.0, -0.035)] + [(0.035 * math.sin(math.radians(t)), -0.035 * math.cos(math.radians(t))) for t in range(15, 180, 15)] +
                     [(0.0, 0.035)], T(0.27, 0.0, z0 + 0.65), 24), 'gloss', 0xb3141c)
    k.add(parts.rod('handle_hub', (0.20, 0.0, z0 + 0.30), (0.23, 0.0, z0 + 0.30), 0.04, 20), 'chrome')
    k.decal('nfs', (0.0, -0.2205, 0.45), u=(1, 0, 0), v=(0, 0, 1), lift=0.0, w=0.20, h=0.07)
    k.place(mk, M)


def bicycle(k, M, slot=None, tag=None):
    """A ten-speed road bike on its kickstand (built along x, the drive side toward -y): the wheels' rims, spokes and
    tires, the double-triangle frame, the fork, drop bars, the saddle, the cranks and chainring; a pawn ticket hangs
    from the bars."""
    mk = k.mark()
    R = 0.34
    wb = 1.0
    rear, front = Vector((-wb / 2, 0.0, R)), Vector((wb / 2, 0.0, R))
    lean = RX(-4.0)
    for c in (rear, front):
        k.add(torus('tire', tuple(c), R - 0.012, 0.012, 'y', 48, 8), 'rubber', 0x141414)
        k.add(torus('rim', tuple(c), R - 0.03, 0.008, 'y', 48, 6), 'alu')
        k.add(parts.rod('hub', tuple(c + Vector((0, -0.05, 0))), tuple(c + Vector((0, 0.05, 0))), 0.02, 14), 'alu')
        for i in range(24):
            a = 2 * math.pi * i / 24
            side = 0.04 if i % 2 else -0.04
            k.add(parts.rod('spoke', tuple(c + Vector((0, side, 0))), tuple(c + Vector(((R - 0.035) * math.cos(a), 0.0, (R - 0.035) * math.sin(a)))),
                            0.0012, 4), 'steel')
    bb = Vector((-0.05, 0.0, 0.28))
    seat = Vector((-0.20, 0.0, 0.85))
    head_top, head_bot = Vector((0.36, 0.0, 0.82)), Vector((0.39, 0.0, 0.70))
    col = 0x1c4a8a
    for a, b in ((bb, seat), (seat, head_top), (bb, head_bot), (seat + Vector((0.0, 0.0, -0.02)), rear), (bb, rear)):
        for dy in ((0.0,) if (a, b) not in ((seat + Vector((0.0, 0.0, -0.02)), rear), (bb, rear)) else (-0.04, 0.04)):
            k.add(parts.rod('tube', tuple(a + Vector((0, dy * 0.5, 0))), tuple(b + Vector((0, dy, 0))), 0.014, 12), 'paint', col)
    k.add(parts.rod('head', tuple(head_bot - Vector((0.01, 0, 0.03))), tuple(head_top + Vector((-0.01, 0, 0.04))), 0.018, 12), 'paint', col)
    for dy in (-0.045, 0.045):
        k.add(tube('fork', [tuple(head_bot + Vector((0, dy * 0.5, 0))), tuple(front + Vector((0.03, dy, 0.12))), tuple(front + Vector((0.0, dy, 0.0)))], 0.01,
                   10), 'paint', col)
    k.add(parts.rod('stem', tuple(head_top + Vector((-0.01, 0, 0.04))), tuple(head_top + Vector((0.08, 0, 0.08))), 0.012, 10), 'alu')
    bars = [(0.44, -0.20, 0.90), (0.48, -0.20, 0.88), (0.50, -0.20, 0.82), (0.46, -0.20, 0.78), (0.44, -0.20, 0.80)]
    for sgn in (-1, 1):
        k.add(tube('bar', [(0.44, 0.0, 0.90)] + [(x, sgn * abs(y), z) for x, y, z in bars], 0.011, 10), 'cloth', 0x141414)
    k.add(xy_slab('saddle', closed_cr([(-0.14, 0.0), (-0.12, 0.07), (-0.02, 0.06), (0.10, 0.025), (0.12, 0.0), (0.10, -0.025), (-0.02, -0.06),
                                       (-0.12, -0.07)], 4), 0.0, 0.04, r=0.015, seg=2), 'leather', 0x1a1410)
    k.objs[-1].data.transform(T(seat.x - 0.02, 0.0, seat.z + 0.07))
    k.add(parts.rod('seatpost', tuple(seat), tuple(seat + Vector((-0.02, 0, 0.08))), 0.012, 10), 'alu')
    k.add(torus('chainring', tuple(bb + Vector((0, -0.07, 0))), 0.10, 0.004, 'y', 40, 4), 'steel')
    k.add(parts.rod('crank', tuple(bb + Vector((0, -0.075, 0))), tuple(bb + Vector((0.12, -0.075, -0.12))), 0.008, 8), 'alu')
    k.add(parts.rod('crank2', tuple(bb + Vector((0, 0.075, 0))), tuple(bb + Vector((-0.12, 0.075, 0.12))), 0.008, 8), 'alu')
    for p in (bb + Vector((0.12, -0.11, -0.12)), bb + Vector((-0.12, 0.11, 0.12))):
        k.add(bx('pedal', p.x - 0.04, p.x + 0.04, p.y - 0.04, p.y + 0.04, p.z - 0.008, p.z + 0.008, 0.003), 'rubber', 0x1a1a1a)
    k.add(tube('chain', [tuple(bb + Vector((0, -0.07, 0.10))), tuple(rear + Vector((0, -0.07, 0.04))), tuple(rear + Vector((0, -0.07, -0.04))),
                         tuple(bb + Vector((0, -0.07, -0.10)))], 0.004, 6), 'black_metal')
    k.add(parts.rod('kickstand', tuple(bb + Vector((-0.08, 0.05, -0.02))), (bb.x - 0.12, 0.20, 0.015), 0.008, 8), 'alu')
    if tag:
        hang_tag(k, (0.47, -0.215, 0.82), tag, drop=0.06, facing='-y')
    for o in k.objs[mk[0]:]:
        o.data.transform(lean)
    k.place(mk, M)


def leather_bust(k, M, tag=None, slot=None):
    """A mannequin bust on a pedestal in a black leather rider jacket: the torso form, the jacket's shell over it with an
    off-center zip, the notched lapels laid back, the epaulettes, snaps; a pawn ticket safety-pinned to the lapel."""
    mk = k.mark()
    k.add(lathe_axis('ped_base', [(0.0, 0.0), (0.18, 0.0), (0.18, 0.02), (0.05, 0.04), (0.0, 0.04)], Matrix(), 32), 'black_metal')
    k.add(parts.rod('ped_pole', (0.0, 0.0, 0.04), (0.0, 0.0, 0.92), 0.015, 12), 'chrome')
    rings = []
    for z, wx, wy in ((0.92, 0.13, 0.09), (0.98, 0.16, 0.11), (1.10, 0.17, 0.115), (1.24, 0.18, 0.12), (1.34, 0.22, 0.12), (1.40, 0.20, 0.10),
                      (1.44, 0.07, 0.06), (1.52, 0.055, 0.05), (1.56, 0.05, 0.045)):
        rings.append([(wx * math.cos(2 * math.pi * i / 32), wy * math.sin(2 * math.pi * i / 32), z) for i in range(32)])
    k.add(core.loft('form', rings, caps=True), 'cloth', 0xd8d0c0)
    JK = [(0.90, 0.17, 0.125), (1.00, 0.175, 0.13), (1.14, 0.185, 0.135), (1.27, 0.195, 0.135), (1.36, 0.235, 0.135), (1.42, 0.215, 0.115)]

    def jw(z):
        for (z0, x0, y0), (z1, x1, y1) in zip(JK, JK[1:]):
            if z <= z1:
                t = max(0.0, (z - z0) / (z1 - z0))
                return x0 + (x1 - x0) * t, y0 + (y1 - y0) * t
        return JK[-1][1], JK[-1][2]

    def jy(x, z, off):
        """The jacket's front surface (y) at (x, z), off in front of it."""
        wx, wy = jw(z)
        return -(wy * math.sqrt(max(0.0, 1.0 - (x / wx) ** 2)) + off)
    jr = []
    for z, wx, wy in JK + [(1.45, 0.12, 0.09), (1.49, 0.078, 0.07)]:
        jr.append([(wx * math.cos(2 * math.pi * i / 32), wy * math.sin(2 * math.pi * i / 32), z) for i in range(32)])
    k.add(core.loft('jacket', jr, caps=False), 'leather', 0x141210)
    core.orient_normals(k.objs[-1])
    # the belt at the waist with its buckle
    belt = []
    for z in (0.925, 0.968):
        wx, wy = jw(z)
        belt.append([((wx + 0.004) * math.cos(2 * math.pi * i / 32), (wy + 0.004) * math.sin(2 * math.pi * i / 32), z) for i in range(32)])
    k.add(core.loft('belt', belt, caps=False), 'leather', 0x1a1714)
    core.orient_normals(k.objs[-1])
    by_ = jy(-0.05, 0.946, 0.006)
    k.add(bx('buckle', -0.075, -0.035, by_ - 0.004, by_ + 0.002, 0.922, 0.971, 0.003), 'chrome')
    for sgn in (-1, 1):
        # the lapel laid back: a patch on the chest's curve, 3 mm proud of it
        A, B, C, D = (Vector((sgn * 0.02, 1.06)), Vector((sgn * 0.135, 1.30)), Vector((sgn * 0.10, 1.40)), Vector((sgn * 0.05, 1.30)))
        bm = bmesh.new()
        nu, nv = 4, 6
        g = []
        for vi in range(nv + 1):
            row = []
            for ui in range(nu + 1):
                u, v = ui / nu, vi / nv
                q = A * (1 - u) * (1 - v) + B * u * (1 - v) + C * u * v + D * (1 - u) * v
                row.append(bm.verts.new((q.x, jy(q.x, q.y, 0.003), q.y)))
            g.append(row)
        for vi in range(nv):
            for ui in range(nu):
                bm.faces.new((g[vi][ui], g[vi][ui + 1], g[vi + 1][ui + 1], g[vi + 1][ui]))
        bm.normal_update()
        if sum(f.normal.y for f in bm.faces) > 0.0:
            bmesh.ops.reverse_faces(bm, faces=bm.faces[:])
        k.add(core.mesh_object('lapel', bm), 'leather', 0x1d1a17)
        # the snap at the lapel's point, the epaulette on the shoulder's slope
        sx = sgn * 0.118
        sy = jy(sx, 1.29, 0.003)
        k.add(parts.rod('snap', (sx, sy, 1.29), (sx, sy - 0.004, 1.29), 0.0055, 10), 'chrome')
        k.add(obox('epaulette', (sgn * 0.168, 0.0, 1.441), (0.095, 0.04, 0.007), rot_y=17.5 * sgn, r=0.002), 'leather', 0x1a1714)
        k.add(parts.rod('ep_snap', (sgn * 0.13, 0.0, 1.452), (sgn * 0.13, 0.0, 1.457), 0.005, 8), 'chrome')
    # the zip, off center, from the right hip to the left of the collar, lying on the leather
    path = core.catmull_rom([(0.07, 0.905), (0.06, 1.05), (0.03, 1.20), (-0.035, 1.34)], 4)
    k.add(tube('zip', [(x, jy(x, z, 0.003), z) for x, z in path], 0.003, 6), 'chrome')
    k.add(bx('zip_pull', 0.055, 0.067, jy(0.06, 1.05, 0.006) - 0.003, jy(0.06, 1.05, 0.006), 1.02, 1.05, 0.002), 'chrome')
    for z in (1.0, 1.15):
        y0 = jy(0.135, z, 0.0)
        k.add(parts.rod('snap', (0.135, y0, z), (0.135, y0 - 0.005, z), 0.006, 10), 'chrome')
    if tag:
        hang_tag(k, (-0.085, jy(-0.085, 1.30, 0.006), 1.30), tag, drop=0.03, facing='-y')
    k.place(mk, M)


def tower_case(k, M, slot=None):
    """A glass tower case (its glass in the mesh's [glass] slot): a black base and cap, steel corner posts, two glass shelves;
    a two-handled cup and a framed two-dollar note inside, NOT FOR SALE on the glass."""
    mk = k.mark()
    W, Hc = 0.45, 1.80
    k.add(bx('tw_base', -W / 2, W / 2, -W / 2, W / 2, 0.0, 0.20, 0.006), 'melamine', 0x1c1c1e)
    k.add(bx('tw_cap', -W / 2, W / 2, -W / 2, W / 2, Hc - 0.06, Hc, 0.006), 'melamine', 0x1c1c1e)
    for sx in (-1, 1):
        for sy in (-1, 1):
            xa = (W / 2 - 0.02, W / 2) if sx > 0 else (-W / 2, -W / 2 + 0.02)
            ya_ = (W / 2 - 0.02, W / 2) if sy > 0 else (-W / 2, -W / 2 + 0.02)
            k.add(bx('tw_post', xa[0], xa[1], ya_[0], ya_[1], 0.20, Hc - 0.06, 0.002), 'powder', 0x1a1b1d)
            for zs in (0.75, 1.25):
                cx_, cy_ = sx * (W / 2 - 0.035), sy * (W / 2 - 0.035)
                k.add(bx('tw_clip', cx_ - 0.015, cx_ + 0.015, cy_ - 0.015, cy_ + 0.015, zs - 0.012, zs, 0.002), 'chrome')
    # the cup on the middle shelf, the framed note on the top
    # an open loving cup: foot, stem, knop, the bowl to its lip and back down inside it
    cup = [(0.0, 0.0), (0.05, 0.0), (0.05, 0.012), (0.02, 0.02), (0.016, 0.07), (0.03, 0.08), (0.065, 0.12), (0.07, 0.20), (0.073, 0.205), (0.067, 0.203),
           (0.063, 0.13), (0.03, 0.103), (0.0, 0.1)]
    k.add(lathe_axis('cup', cup, T(0.0, 0.0, 0.75), 40), 'gold', 0xe8c060, aux=(0.12, 0, 0))
    for sx in (-1, 1):
        k.add(tube('cup_handle', [(sx * 0.065, 0.0, 0.75 + 0.18), (sx * 0.11, 0.0, 0.75 + 0.15), (sx * 0.06, 0.0, 0.75 + 0.10)], 0.005, 8), 'gold', 0xe8c060)
    k.add(bx('frame', -0.12, 0.12, -0.01, 0.01, 1.25, 1.42, 0.004), 'wood', 0x2a1a10, aux=(1, 0, 0))
    k.decal('two', (0.0, -0.0105, 1.335), u=(1, 0, 0), v=(0, 0, 1), lift=0.0, w=0.20, h=0.11)
    k.decal('nfs', (0.0, -W / 2 + 0.009, 1.05), u=(1, 0, 0), v=(0, 0, 1), lift=0.0, w=0.18, h=0.06)
    k.place(mk, M)


def floor_pieces():
    k = Kit('pwo', 'SM_NS_Pawn_Floor', slots=('body',), dust=0.35, low_grime=0.12, cavity=0.3)
    k.panel('nfs', 0.20, 0.07, draw_not_for_sale, density=2.0)
    k.panel('reels', 0.26, 0.13, draw_reels, density=2.0)
    k.panel('slotglass', 0.34, 0.12, draw_slot_glass, density=1.6)
    k.panel('two', 0.20, 0.11, draw_two_dollar, density=2.0)
    k.panel('ft0', 0.035, 0.06, p_tag(951, 'road bike'), density=2.4)
    k.panel('ft1', 0.035, 0.06, p_tag(952, 'leather jkt'), density=2.4)
    k.panel('ft2', 0.035, 0.06, p_tag(953, 'tool chest'), density=2.4)
    k.atlas.render()
    x, y, yaw = FLOOR_AT['tower']
    tower_case(k, T(x, y, 0.0) @ RZ(yaw))
    x, y, yaw = FLOOR_AT['slot']
    slot_machine(k, T(x, y, 0.0) @ RZ(yaw))
    x, y, yaw = FLOOR_AT['chest']
    tool_chest(k, T(x, y, 0.0) @ RZ(yaw))
    hang_tag(k, (x - 0.25, y + 0.02, 0.80), 'ft2', drop=0.05, facing='-x')
    x, y, yaw = FLOOR_AT['bike']
    bicycle(k, T(x, y, 0.0) @ RZ(yaw), tag='ft0')
    x, y, yaw = FLOOR_AT['bust']
    leather_bust(k, T(x, y, 0.0) @ RZ(yaw), tag='ft1')
    for g in floor_glass():
        k.special(g, 'ns_glass')
    return [k.finish(35.0)]


def floor_glass():
    """The tower case's four sides and its two glass shelves."""
    x, y, _ = FLOOR_AT['tower']
    W = 0.45
    objs = []
    for (p, q) in (((x - W / 2 + 0.01, y - W / 2), (x - W / 2 + 0.01, y + W / 2)), ((x + W / 2 - 0.01, y - W / 2), (x + W / 2 - 0.01, y + W / 2)),
                   ((x - W / 2, y - W / 2 + 0.01), (x + W / 2, y - W / 2 + 0.01)), ((x - W / 2, y + W / 2 - 0.01), (x + W / 2, y + W / 2 - 0.01))):
        objs.append(quad('tower', [(p[0], p[1], 0.20), (q[0], q[1], 0.20), (q[0], q[1], 1.74), (p[0], p[1], 1.74)]))
    for zs in (0.75, 1.25):
        objs.append(quad('shelf', [(x - W / 2 + 0.02, y - W / 2 + 0.02, zs), (x + W / 2 - 0.02, y - W / 2 + 0.02, zs), (x + W / 2 - 0.02, y + W / 2 - 0.02, zs),
                                   (x - W / 2 + 0.02, y + W / 2 - 0.02, zs)]))
    return objs


def make_glows(parts_by_name):
    out = []
    for name, objs in parts_by_name.items():
        if not objs or name not in MESHES:
            for o in objs:
                bpy.data.objects.remove(o)
            continue
        o = parts.glow_object(objs, name)
        if not o.data.uv_layers:
            o.data.uv_layers.new(name='UVMap')
        print(f'[ns_pawn] {name}: {tri_count(o)} triangles, slots [{o.material_slots[0].material.name}]')
        out.append(o)
    return out


def _sweep_all(paths, r, n=10):
    return [tube('neon', p, r, n=n) for p in paths if len(p) > 1]


def _lens(name, center, normal, r, n=24):
    """A thin disc facing along normal: a lamp's lens."""
    N = Vector(normal).normalized()
    c = Vector(center)
    return parts.rod(name, tuple(c - N * 0.0015), tuple(c + N * 0.0015), r, n)


def _stage_marks(v):
    """A mark's shop-frame points (3-tuples of floats, nested anywhere in v) as stage cm."""
    if isinstance(v, (tuple, list)) and len(v) == 3 and all(isinstance(c, float) for c in v):
        return stage_cm(v)
    if isinstance(v, (tuple, list)):
        return type(v)(_stage_marks(c) for c in v)
    return v


BUILDERS = {
    'front': lambda: front(),
    'door': lambda: door(),
    'grille': lambda: grille(),
    'sign': lambda: sign() + make_glows(sign_glow()),
    'shell': lambda: shell() + make_glows(shell_glow()),
    'counter': lambda: counter() + make_glows(counter_glow()),
    'cage': lambda: cage() + make_glows(cage_glow()),
    'guitars': lambda: guitar_wall(),
    'shelves': lambda: shelves(),
    'tickets': lambda: tickets() + ticket() + tags(),
    'loupe': lambda: loupe(),
    'bench': lambda: bench() + make_glows(bench_glow()),
    'floor': lambda: floor_pieces(),
}


def build():
    core.reset()
    random.seed(2300)
    make_unbaked()
    out = []
    for key in WHICH:
        if key in BUILDERS:
            out += [o for o in BUILDERS[key]() if o.name in MESHES]
    for o in out:
        o.data.name = o.name
    _BAKED[:] = [o for o in out if o.name not in GLASS_MESHES and o.name not in GLOW_MESHES]
    for k_, v in sorted(ITEM_SPOTS.items()):
        print(f'[ns_pawn] mark {k_}: {_stage_marks(v)}')
    origins = {'Door hinge': (DOOR_HINGE[0], DOOR_HINGE[1], 0.0), 'Grille': GRILLE_ORIGIN, 'Sign plate': SIGN_PLATE,
               'Fan hub': (FAN_AT[0], FAN_AT[1], FAN_HUB_Z), 'SafeDoor hinge': safe_hinge(), 'Lamp': cage_lamp_at(), 'Magnifier': MAGNIFIER}
    for k_, v in origins.items():
        print(f'[ns_pawn] origin {k_}: {stage_cm(tuple(float(c) for c in v))}')
    for i, (y, z, r, style, live) in enumerate(CLOCKS):
        if live:
            print(f'[ns_pawn] live clock {i}: center {stage_cm((CLOCK_X, y, z))} radius {r * 100:.1f} cm, face toward +Y (north)')
    return out


# ================================================================== review: the stand-in street, the lights, the shots
#
# Everything below is review-only (art/review/ns_pawn*.jpg): Market's south sidewalk and road, the M-S3 building's
# painted brick round the opening with its sign band (art-fronts builds the real one), the thrift and the gym either
# side, a streetlight; the shop lit as PawnStage lights it.

def S(p):
    return tuple(SITE_M @ Vector(p))


def _site_box(name, x0, x1, y0, y1, z0, z1, mat):
    """A review box given in Blender SITE coordinates."""
    o = bx(name, x0, x1, y0, y1, z0, z1)
    core.assign(o, mat)
    return o


def _street_context(tmp, with_room):
    objs = []
    side = _concrete_mat('rv_sidewalk')
    road = _rv_mat('rv_road', 0x2a2b2e, 0.6)
    # Market's south sidewalk is Y 4950..5400: Blender y 49.5..54.0; the road beyond
    objs.append(_site_box('rv_sidewalk', 20.0, 70.0, FRONT_Y, 54.0, -0.2, 0.0, side))
    objs.append(_site_box('rv_curb', 20.0, 70.0, 54.0, 54.15, -0.3, 0.0, _rv_mat('rv_curb_m', 0x8a8780, 0.8)))
    objs.append(_site_box('rv_road', 20.0, 70.0, 54.15, 65.0, -0.4, -0.15, road))
    objs.append(_site_box('rv_far', 20.0, 70.0, 69.5, 75.0, -0.4, 12.0, _brick_mat('rv_far_brick', 0x6a3a2a, 0x7a4632)))
    paint = _brick_mat('rv_paint_brick')
    red = _brick_mat('rv_red_brick', 0x7a3a2a, 0x8f4a35)
    # M-S3's front (Blender x = -X/100: the plot 37.0..45.0), its face at y 49.5, the opening 37.6..44.4 x 0..3.8
    objs.append(_site_box('rv_pier_w', PLOT_Y0, OPEN_Y0, FRONT_Y - 0.6, FRONT_Y, 0.0, OPEN_Z1, paint))
    objs.append(_site_box('rv_pier_e', OPEN_Y1, PLOT_Y1, FRONT_Y - 0.6, FRONT_Y, 0.0, OPEN_Z1, paint))
    objs.append(_site_box('rv_upper', PLOT_Y0, PLOT_Y1, FRONT_Y - 0.6, FRONT_Y, OPEN_Z1, 13.5, paint))
    band = _rv_mat('rv_band', 0x1c2a22, 0.4)
    objs.append(_site_box('rv_band', OPEN_Y0, OPEN_Y1, FRONT_Y, FRONT_Y + 0.06, 3.86, 4.60, band))
    t = flat_text('rv_band_text', 'NORTHSIDE PAWN & LOAN', 0.40, frame_matrix((-1, 0, 0), (0, 0, 1), ((OPEN_Y0 + OPEN_Y1) / 2, FRONT_Y + 0.061, 4.02)),
                  'Black', tracking=1.08, max_w=6.2)
    core.assign(t, _rv_mat('rv_gold', 0xe0b050, 0.3, 1.0))
    objs.append(t)
    win = _rv_mat('rv_win', 0x0b0e12, 0.05)
    for zz in (4.9, 8.1, 11.3):
        for i in range(4):
            xc = PLOT_Y0 + 1.0 + i * 2.0
            objs.append(_site_box('rv_win', xc - 0.5, xc + 0.5, FRONT_Y - 0.12, FRONT_Y + 0.01, zz, zz + 1.6, win))
    objs.append(_site_box('rv_thrift', 24.0, PLOT_Y0 - 0.02, FRONT_Y - 12.0, FRONT_Y, 0.0, 8.5, _brick_mat('rv_thrift_brick', 0x9c8160, 0xb39874)))
    objs.append(_site_box('rv_gym', PLOT_Y1 + 0.02, 66.0, FRONT_Y - 12.0, FRONT_Y, 0.0, 9.5, red))
    objs.append(_site_box('rv_gym_win', PLOT_Y1 + 1.0, 52.0, FRONT_Y, FRONT_Y + 0.02, 0.6, 4.0, _rv_mat('rv_gym_glass', 0x1a2026, 0.08, 0.0,
                                                                                                      0x9ab4d0, 0.12)))
    if not with_room:
        wall = _rv_mat('rv_room', 0xc8c2b4, 0.8)
        for (x0, x1, y0, y1, z0, z1) in ((RX0, RX1, RY0 - 0.1, RY0, 0.0, CEIL), (RX0, RX1, RY1, RY1 + 0.1, 0.0, CEIL),
                                         (RX1, RX1 + 0.1, RY0, RY1, 0.0, CEIL), (RX0, RX1, RY0, RY1, -0.1, 0.0), (RX0, RX1, RY0, RY1, CEIL, CEIL + 0.1)):
            o = bx('rv_room', x0, x1, y0, y1, z0, z1)
            o.data.transform(SITE_M)
            core.assign(o, wall)
            objs.append(o)
    tmp += objs
    return objs


GLOW_RV = {'SM_NS_Pawn_Sign_GlowNeon': ((0.12, 0.42, 1.0), 40.0), 'SM_NS_Pawn_Sign_GlowWarm': ((1.0, 0.78, 0.52), 25.0),
           'SM_NS_Pawn_Shell_GlowCool': ((0.86, 0.93, 1.0), 14.0), 'SM_NS_Pawn_Counter_GlowCool': ((0.86, 0.93, 1.0), 14.0),
           'SM_NS_Pawn_Cage_GlowWarm': ((1.0, 0.78, 0.52), 25.0), 'SM_NS_Pawn_Bench_GlowCool': ((0.86, 0.93, 1.0), 14.0)}


def _shop_lights():
    """What PawnStage lights awake: six strips, the cases' LEDs, the desk lamp, a spot on the gold balls (+ the tube's
    spill)."""
    L = []
    for (x, y) in STRIPS:
        L.append(_light('rv_strip', 'AREA', S((x, y, STRIP_Z - 0.03)), 55.0, (0.88, 0.93, 1.0), (0.12, 2.4), S((x, y, 0.0))))
        L[-1].rotation_euler.z += math.radians(90.0)
    for ci in range(4):
        a, b = CASE_Y[ci], CASE_Y[ci + 1]
        L.append(_light('rv_case', 'AREA', S(((CASE_X[0] + CASE_X[1]) / 2, (a + b) / 2, CASE_TOP - 0.03)), 12.0, (0.92, 0.96, 1.0), (0.5, 0.9),
                        S(((CASE_X[0] + CASE_X[1]) / 2, (a + b) / 2, 0.0))))
    L.append(_light('rv_desk', 'SPOT', S(cage_lamp_at()), 30.0, (1.0, 0.8, 0.55), 0.05, S((DESK[0] + 0.3, BANDIT_Y[0], 0.9)), spot=80.0))
    bx_, by_, bz_ = SIGN_BALLS[2][0], SIGN_PLATE[1], SIGN_BALLS[2][1]
    L.append(_light('rv_balls', 'SPOT', S(SIGN_SHADE_AT), 30.0, (1.0, 0.8, 0.55), 0.05, S((bx_, by_, bz_)), spot=60.0))
    for yy in (40.0, 41.5, 43.0):
        L.append(_light('rv_spill_blue', 'AREA', S((TUBE_X + 0.05, yy, 1.6)), 6.0, (0.2, 0.45, 1.0), (0.05, 1.5), S((-1.0, yy, 1.6))))
    return L


def _street_lights():
    L = []
    for xx in (35.0, 47.0):
        L.append(_light('rv_street', 'SPOT', (xx, 53.6, 7.6), 2600.0, (1.0, 0.62, 0.30), 0.25, (xx, 51.5, 0.0), spot=110.0, blend=0.9))
    return L


STREET_SHOTS = [
    ('night_w8', 'night', (69.0, 67.0, 1.7), (45.0, 49.5, 2.5), 34.0),          # §9.1 W8: from Market's north side
    ('night_wide', 'night', (41.0, 62.0, 1.75), (41.0, 49.5, 2.0), 30.0),
    ('night_angle', 'night', (33.5, 52.2, 1.65), (42.5, 49.5, 1.6), 30.0),
    ('night_window', 'night', (42.3, 51.2, 1.55), (42.0, 48.9, 1.0), 32.0),
    ('night_door', 'night', (39.6, 52.0, 1.6), (38.4, 48.6, 1.35), 30.0),
    ('night_balls', 'night', (39.2, 52.4, 2.4), (37.3, 50.2, 3.7), 40.0),
    ('night_grille', 'night', (44.5, 56.5, 1.7), (41.0, 49.5, 1.7), 30.0),
    ('day_wide', 'day', (41.0, 62.0, 1.75), (41.0, 49.5, 2.0), 30.0),
    ('day_w8', 'day', (69.0, 67.0, 1.7), (45.0, 49.5, 2.5), 34.0),
    ('day_window', 'day', (43.6, 52.6, 1.6), (41.8, 49.0, 1.1), 32.0),
]


def _street_review(by, tmp):
    sc = bpy.context.scene
    with_room = 'SM_NS_Pawn_Shell' in by
    _street_context(tmp, with_room)
    paths = []
    world = sc.world
    for label, mode, loc, tgt, lens in STREET_SHOTS:
        if 'SM_NS_Pawn_Grille' in by:
            by['SM_NS_Pawn_Grille'].hide_render = 'grille' not in label
        if mode == 'night':
            world.color = (0.006, 0.008, 0.014)
            sc.view_settings.exposure = 0.6
            _glow_on(by, True)
            lights = _street_lights() + _shop_lights()
        else:
            world.color = (0.42, 0.45, 0.50)
            sc.view_settings.exposure = -0.4
            _glow_on(by, False)
            sun = bpy.data.lights.new('rv_sun', 'SUN')
            sun.energy = 1.2
            sun.angle = math.radians(12.0)
            so = core.link(bpy.data.objects.new('rv_sun', sun))
            so.rotation_euler = (math.radians(55.0), 0.0, math.radians(160.0))
            lights = [so] + [l for l in _shop_lights() if l.name.startswith('rv_strip') or l.name.startswith('rv_case')]
        _cam(loc, tgt, lens)
        paths.append(_shot(os.path.join(SCRATCH, f's_{label}.png'), 1000, 680, 96))
        for l in lights:
            bpy.data.objects.remove(l)
    sc.view_settings.exposure = 0.0
    _glow_on(by, True)
    if 'SM_NS_Pawn_Grille' in by:
        by['SM_NS_Pawn_Grille'].hide_render = False
    return compose_grid('ns_pawn_street', paths, 2)


# (label, camera, target, lens) in the shop frame
ROOM_SHOTS = [
    ('i5_counter', (5.05, 41.55, 1.62), (6.40, 42.35, 1.05), 26.0),        # §9.1 I5: pawn-counter, over the hero's shoulder
    ('i6_cage', (5.85, 38.60, 1.58), (8.30, 38.65, 1.10), 30.0),          # §9.1 I6: pawn-cage, through the window
    ('door_in', (1.25, 38.40, 1.66), (6.2, 41.6, 1.1), 20.0),
    ('guitars', (3.40, 42.90, 1.62), (3.40, 37.40, 1.85), 22.0),
    ('shelves', (3.40, 39.10, 1.62), (3.50, 44.60, 1.15), 22.0),
    ('to_front', (5.30, 42.80, 1.66), (0.50, 40.6, 1.55), 20.0),
    ('behind_counter', (7.80, 42.40, 1.66), (2.0, 40.8, 1.25), 20.0),
    ('clocks', (5.20, 41.80, 1.55), (9.00, 43.30, 2.60), 26.0),
]


def _room_review(by, tmp):
    """Every mesh in place at night with the shop lit as PawnStage lights it, the street dark outside
    (art/review/ns_pawn_room.jpg)."""
    sc = bpy.context.scene
    _street_context(tmp, True)
    for n, o in by.items():
        o.hide_render = n == 'SM_NS_Pawn_Grille'
    _glow_on(by, True)
    sc.world.color = (0.006, 0.008, 0.014)
    sc.view_settings.exposure = -0.2
    lights = _shop_lights() + _street_lights()
    paths = []
    for label, loc, tgt, lens in ROOM_SHOTS:
        _cam(S(loc), S(tgt), lens)
        paths.append(_shot(os.path.join(SCRATCH, f'r_{label}.png'), 1000, 680, 128))
    for l in lights:
        bpy.data.objects.remove(l)
    sc.view_settings.exposure = 0.0
    return compose_grid('ns_pawn_room', paths, 2)


# Close looks: mesh -> [(camera, target, lens)] in the shop frame (the mesh where it stands)
DETAIL_SHOP = {
    'SM_NS_Pawn_Front': [((-1.2, 41.0, 1.4), (0.6, 41.2, 0.95), 30.0), ((-0.9, 43.4, 1.3), (0.8, 43.0, 0.9), 30.0), ((-1.0, 39.8, 1.3), (0.6, 40.3, 0.8), 30.0),
                         ((-0.8, 38.6, 1.6), (0.9, 38.4, 1.3), 26.0), ((-0.6, 40.0, 3.1), (0.2, 42.0, 2.6), 28.0), ((2.0, 41.5, 1.6), (0.6, 41.8, 1.0), 28.0)],
    'SM_NS_Pawn_Door': [((-0.6, 38.9, 1.5), (0.9, 38.4, 1.25), 32.0), ((2.4, 39.4, 1.6), (0.95, 38.4, 1.2), 32.0)],
    'SM_NS_Pawn_Grille': [((-3.5, 39.0, 1.7), (0.1, 41.0, 1.6), 28.0), ((-0.9, 38.6, 0.6), (0.08, 38.2, 0.2), 35.0)],
    'SM_NS_Pawn_Sign': [((-2.6, 39.4, 3.0), (-0.6, 37.3, 3.8), 35.0), ((-0.9, 39.95, 1.05), (0.26, 39.3, 0.72), 32.0)],
    'SM_NS_Pawn_Shell': [((2.0, 42.0, 1.6), (6.0, 41.0, 2.4), 22.0), ((7.4, 43.2, 1.6), (9.0, 43.3, 2.6), 26.0), ((3.0, 41.0, 1.4), (3.0, 41.1, 3.6), 24.0)],
    'SM_NS_Pawn_Counter': [((5.15, 40.95, 1.45), (5.95, 40.95, 0.55), 30.0), ((5.15, 42.0, 1.45), (5.95, 41.95, 0.6), 30.0),
                           ((5.15, 43.1, 1.45), (5.95, 43.0, 0.6), 30.0), ((5.15, 44.1, 1.45), (5.95, 44.0, 0.6), 30.0),
                           ((5.2, 42.6, 1.5), (6.2, 43.6, 1.05), 30.0), ((7.0, 40.2, 1.4), (6.0, 40.4, 0.8), 30.0)],
    'SM_NS_Pawn_Cage': [((5.8, 38.6, 1.6), (6.5, 38.6, 1.5), 28.0), ((5.4, 40.6, 1.8), (8.5, 39.0, 1.2), 24.0), ((7.4, 40.4, 1.7), (8.8, 38.2, 1.1), 26.0),
                        ((7.6, 39.9, 1.5), (6.9, 38.8, 1.0), 30.0)],
    'SM_NS_Pawn_SafeDoor': [((6.9, 38.0, 1.1), (7.43, 38.6, 0.7), 32.0)],
    'SM_NS_Pawn_GuitarWall': [((3.4, 40.8, 1.7), (3.4, 37.4, 1.9), 26.0), ((2.2, 39.2, 2.6), (2.0, 37.5, 2.6), 32.0), ((4.6, 39.1, 1.4), (4.8, 37.5, 1.3), 32.0),
                              ((3.0, 39.0, 0.9), (3.2, 37.5, 0.35), 32.0)],
    'SM_NS_Pawn_Shelves': [((2.0, 42.3, 1.6), (2.0, 44.6, 1.2), 30.0), ((3.5, 42.3, 1.6), (3.5, 44.6, 1.0), 30.0), ((5.0, 42.3, 1.6), (5.0, 44.6, 1.1), 30.0),
                           ((9.4, 41.6, 1.6), (11.0, 44.3, 1.2), 26.0)],
    'SM_NS_Pawn_Tickets': [((6.3, 38.4, 1.3), (6.85, 38.3, 1.0), 40.0), ((6.4, 39.6, 1.3), (6.75, 39.36, 1.05), 45.0)],
    'SM_NS_Pawn_Ticket': [((6.15, 38.5, 1.25), (6.30, 38.55, 1.04), 50.0)],
    'SM_NS_Pawn_Loupe': [((5.80, 42.15, 1.15), (5.96, 42.28, 0.99), 55.0)],
    'SM_NS_Pawn_Bench': [((7.30, 42.70, 1.55), (8.60, 43.40, 0.95), 28.0), ((7.80, 43.10, 1.25), (8.50, 42.75, 1.0), 40.0)],
    'SM_NS_Pawn_Floor': [((0.9, 42.2, 1.7), (3.0, 41.5, 0.9), 24.0), ((1.4, 41.3, 1.2), (2.6, 41.05, 1.0), 32.0),
                         ((2.6, 43.4, 1.3), (3.6, 42.3, 0.7), 28.0), ((3.2, 40.6, 1.5), (4.25, 41.15, 1.25), 35.0),
                         ((1.0, 41.0, 1.4), (1.8, 41.15, 1.1), 30.0)],
}


def _site_detail():
    return {n: [(S(a), S(b), lens) for a, b, lens in shots] for n, shots in DETAIL_SHOP.items()}


def _place_props(by):
    """The PROP meshes where they stand in the shop (their frames are the shop's, so each turns by -90 about Z into the
    SITE frame): the door on its hinge, the grille under its hood (rolled down), the sign on the west pier with its glows,
    the fan's blades on the hub, the safe's door, the ticket on the cage's ledge, the loupe by the Item mark, a tag."""
    spots = {'SM_NS_Pawn_Door': (DOOR_HINGE[0], DOOR_HINGE[1], 0.0), 'SM_NS_Pawn_Grille': GRILLE_ORIGIN, 'SM_NS_Pawn_Sign': SIGN_PLATE,
             'SM_NS_Pawn_Sign_GlowNeon': SIGN_PLATE, 'SM_NS_Pawn_Sign_GlowWarm': SIGN_PLATE, 'SM_NS_Pawn_FanBlades': (FAN_AT[0], FAN_AT[1], FAN_HUB_Z),
             'SM_NS_Pawn_SafeDoor': safe_hinge(), 'SM_NS_Pawn_Ticket': (6.22, 38.50, 1.036), 'SM_NS_Pawn_Loupe': (5.96, 42.30, 0.991)}
    extra = {'SM_NS_Pawn_Ticket': 15.0, 'SM_NS_Pawn_Loupe': 40.0}
    for n, p in spots.items():
        o = by.get(n)
        if o is None:
            continue
        o.location = S(p)
        o.rotation_mode = 'XYZ'
        o.rotation_euler = (0.0, 0.0, math.radians(-90.0 + extra.get(n, 0.0)))
    for i in range(8):
        o = by.get(f'SM_NS_Pawn_Tag{i}')
        if o is not None:
            o.location = S((5.25, 40.4 + i * 0.08, 1.40))
            o.rotation_mode = 'XYZ'
            o.rotation_euler = (0.0, 0.0, math.radians(-90.0))
    bpy.context.view_layer.update()


def pose_for_review(objs):
    global REVIEW_VIEWS, DETAIL_SHOTS
    if os.environ.get('PAWN_NO_SHEET'):
        # A review split over several processes (PAWN_REVIEW=detail in one, street in another): only one of them renders
        # build.py's standard sheet (art/review/ns_pawn.jpg).
        review.sheet = lambda o, name, **kw: os.path.join(core.REVIEW_DIR, f'{name}.jpg')
    by = {o.name: o for o in objs}
    _place_props(by)
    DETAIL_SHOTS = _site_detail()
    which = _which_review()
    tmp = []
    try:
        if 'detail' in which:
            _detail_review(by, tmp)
        if 'street' in which and 'SM_NS_Pawn_Front' in by:
            _street_review(by, tmp)
        if 'room' in which and 'SM_NS_Pawn_Shell' in by:
            _room_review(by, tmp)
    finally:
        for o in tmp:
            try:
                bpy.data.objects.remove(o)
            except ReferenceError:
                pass
        for o in objs:
            o.hide_render = False
        bpy.context.scene.view_settings.exposure = 0.0
    lo, hi = core.bounds(objs)
    c = (lo + hi) / 2
    REVIEW_VIEWS = [('front', 160, 18, 1.4, (c.x, c.y, 1.8)), ('top', 140, 55, 1.4, (c.x, c.y, 1.0)), ('detail', 175, 10, 0.7, (c.x, FRONT_Y, 1.8))]


DETAIL_SHOTS = {}
