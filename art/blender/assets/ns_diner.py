"""SM_NS_Ruby_*: Ruby's, the 24-hour diner on the northeast corner of Fifth & Market (201 Market St), Northside (spec
world_realism_spec.md section 4.5; the diner itself section 5.4). A curved glass corner under red neon, a long L
counter with nine stools, five booths down the Market windows, the backbar and the kitchen pass.

Built with the shop kit in ns_barber.py (finishes by face attribute, print atlases, tiled bakes, the unbaked slots, the
bake split, the review rig): see that module's docstring for the slot conventions (ns_glass, ns_screen_*, glow meshes).

FRAMES (spec 0.4). SITE meshes: Blender (0, 0, 0) = the stage origin, placed at the stage's transform with yaw 180:
stage (X, Y, Z) cm = (-100x, +100y, +100z). They are modelled round a local anchor at the building lines' corner,
stage (0, 6950, 0) (Blender (0, 69.5, 0)), and moved to the stage frame after the bake. In local meters: the Fifth face
is the x = 0 plane (y 2.0..10.5), the Market face the y = 0 plane (x 2.0..15.0), the quarter-round corner's center
(2.0, 2.0) with R 2.0 (stage center (-200, 7150), R 200). The interior: x 0.40..14.60, y 0.40..10.10 (stage X -40..-1460,
Y 6990..7960); the kitchen behind the pass wall y 7.50..10.10 (stage Y 7700..7960). PROP meshes are in their own frame,
front toward Blender -X, placed at L with yaw F + 180 to face stage yaw F.

Picks inside the spec's ranges: the ground storey (SM_NS_Ruby_Front) fills the openings from Z 0 to 430 cm (the
upper floors of art-fronts' B4 corner body start at Z 430); the glass from Z 62 to 305; the red enamel fascia Z 315..430
with three stainless speed lines; the dining room ceiling at Z 380 and the soffit over the counter at Z 325; the
kitchen ceiling at Z 300.

THE MESHES (meters; slots in order; M_<mesh>_<i> are baked):
- SM_NS_Ruby_Front (SITE, P0, 70k): both faces' ground storey and the quarter-round corner: stainless bulkheads with
  speed-line ribs, stainless mullions and head, curved at the corner (r 1.88 glass), the door frame on the corner's
  chord (stage centers on the 225-degree line), glass block (20 cm, lit from the kitchen: baked emissive) at the
  Fifth face's kitchen end (Y 7700..8000), the red enamel fascia, the script neon's standoffs and GTO housings (each
  run has its own electrodes, so no jumpers show), the window sign's frame, chains and OPEN's blacked-out jumpers, the window
  lettering COFFEE · PIE · BREAKFAST · ALL DAY on four Market panes (lettering geometry, read from the street; the
  reverse shows inside), the door's terrazzo apron. Slots: 0 frame, 1 lit (the glass block; emission 0.7, the
  kitchen's light through it brightest high up under its tube fitting and falling off, the dish shelves' shapes dark
  behind the north end, each block a little different: scale it with DimImportedGlow if the kitchen goes dark).
- SM_NS_Ruby_Front_Glass (SITE, P0): every shop pane, curved at the corner, and the transom over the doors. ns_glass.
- SM_NS_Ruby_Door (PROP, P0, 6k): one leaf of the corner's double door, 0.82 x 0.045 x 2.22 (the chord between the
  jambs is 1.67 m); a stainless-framed glass leaf with the same pull and kick plates on both faces, so the one mesh
  serves both leaves. Origin on its hinge axis at the floor; the leaf runs along +Y from the hinge, centered on it in
  thickness. Placements (stage cm, UE yaw): the west leaf, hinge (-23.5, 7090.6), yaw 315; the south leaf, hinge
  (-140.6, 6973.5), yaw 135; shut, they meet on the 225-degree line. Both swing out toward the corner (egress).
  Slots: 0 baked, 1 ns_glass.
- SM_NS_Ruby_Neon_Red (SITE, P0, glow): RUBY'S in red script tubes on the corner's fascia (letters 62 cm tall, 4 cm
  proud of the enamel). SM_NS_Ruby_Neon_Blue (SITE, P0, glow): OPEN (15 cm) over 24 HOURS (7.5 cm), single-stroke
  tubes hanging in the Fifth face's first pane, facing the street (centered on stage Y 7240, Z 139..182; its frame and
  chains in the Front).
- SM_NS_Ruby_Shell (SITE, P0, 60k): the terrazzo floor (cream with chips, brass strips on a 1.2 m grid; a tiled bake),
  the pass wall and the east wall with the red and stainless wainscot, cream walls, the soffit over the counter with
  its red face and speed lines, the plaster ceiling, the restroom and kitchen doors, the jukebox, two gumball machines,
  a newspaper rack and a coat tree by the door, the RESTROOMS, EMPLOYEES ONLY and NO SMOKING signs, a framed
  clipping; on the ceiling four supply diffusers, a return grille, two smoke detectors and sprinkler heads, and the
  EXIT sign hung inside the corner doors at stage (-102, 7052), Z 256..276 (both faces print, baked emissive: no
  light of its own). + SM_NS_Ruby_Shell_GlowWarm: eight milk-glass globes (their stems and fitters baked in the Shell): three
  over the counter at stage X -550, -770, -990, Y 7495, Z 255 (globe middles), five over the booths' tables at
  X -500, -690, -880, -1070, -1260, Y 7045, Z 245. The jukebox stands against the east wall at stage (-1420, 7410),
  its front toward the room; its arch's colored bands bake emissive (print slot).
  Slots: 0 floor (tiled: one 2.4 m terrazzo square repeats; its UVs run past 0..1), 1 walls, 2 ceiling (tiled 2 m),
  3 prints.
- SM_NS_Ruby_Counter (SITE, P0, 40k): the L counter: the long leg's top along stage Y 7455..7530 from X -485 to
  -1160, the short leg turning north at its west end (X -410..-485, Y 7455..7590). A boomerang laminate top (Z 106) with
  a ribbed chrome edge, a stainless front with chrome battens set 10 cm under the top, a chrome foot rail on brackets;
  the waitress side's work shelf, plate stacks, glass racks and bus tub; condiment sets and a stack of menus on top.
  Its nine stool positions (stations diner-stool1..9, stage cm, the stool's axis on the floor): 1..7 along the long
  leg at Y 7420, X -560, -635, -710, -785, -860, -935, -1010; 8, 9 west of the short leg at X -372, Y 7495 and 7565.
  The cup marks (where a mug or a plate is set down in front of each stool, the waitress's Reach target): stools
  1..7 at the stool's X, Y 7477, Z 106; stools 8, 9 at X -432, the stool's Y, Z 106. Station diner-counter (the
  waitress, behind the long leg): stage (-780, 7585, 0) facing south (stage yaw -90); the coffee station is at her
  back (X -935..-1050). Station diner-pass (the cook): stage (-675, 7845, 0) facing north, his back to the pass.
  Slots: 0 baked, 1 prints.
- SM_NS_Ruby_Stool (PROP, P0, 5k): a chrome swivel stool bolted to the floor, a red vinyl seat; seat top 0.75 m.
  Origin on the floor at its axis.
- SM_NS_Ruby_Booth (PROP, P0, 35k; built 12k): a booth pair with its table: two high-backed red vinyl benches facing each other
  across a laminate table (the table's long axis along X, its window end at +X). Origin on the floor under the table's
  middle; the open side (the aisle) toward -X. Seats 0.46 m. Sockets: SeatA (-0.05, +0.60, 0.46) facing -Y, SeatB
  (-0.05, -0.60, 0.46) facing +Y; Table (0.0, 0.0, 0.76). Placements (booth1..5, stations diner-booth<k>a/b): stage
  (-500 - 190 (k - 1), 7050, 0), yaw 270 (the aisle to the north, stage yaw 90).
- SM_NS_Ruby_Backbar (SITE, P0, 60k): the backbar on the pass wall (stage Y 7630..7700, X -440..-1160): stainless
  cabinets, the register, the pie case (its glass ns_glass, its lamp in the glow mesh), the coffee station (a brewer,
  carafes on warmers), the milkshake mixer, a toaster, cups, saucers and glasses on stainless shelves, the cereal
  shelf. + SM_NS_Ruby_Backbar_GlowWarm: the warmers' rings and the pie case's lamp. Slots: 0 baked, 1 prints, 2 ns_glass.
- SM_NS_Ruby_PieCarousel (PROP, P0): the pie case's three turning shelves with pies; origin at the turning axis on the
  case's floor: place it at stage (-583, 7665, 98) with any yaw (the game turns it about Z).
- SM_NS_Ruby_Pass (SITE, P0, 70k): the pass through the wall (stage X -620..-920, Z 110..175): the heated stainless
  shelf, the ticket rail with tickets, an order bell, two plates up; behind it the kitchen: quarry tile floor, white
  tiled walls, the cook line on the north wall (a range with a stock pot, the flat-top with patties and pancakes on
  it and its grease trough, a two-basket fryer, a prep cooler, a reach-in), the hood and its filters, the dish sink,
  a prep table under the pass, shelves of cans, containers and pots, the steam card (ns_steam, UV 0..1 as seen through
  the pass, over the flat-top: stage X -595..-740, Y 7920, Z 95..195). The cook (a baked figure, people track) stands
  at stage (-675, 7845, 0) facing north, his back to the pass.
  + SM_NS_Ruby_Pass_GlowWarm (three heat lamps), SM_NS_Ruby_Pass_GlowCool (two kitchen tube fittings).
  Slots: 0 baked, 1 tile (tiled), 2 floor (tiled), 3 prints, 4 ns_steam.
- SM_NS_Ruby_MenuBoard (SITE, P0, 2k): the letter-board menu over the pass, its frame baked (slot 0) and its face
  ns_screen_menu (slot 1; UV 0..1 as seen from the dining room, 3.4 x 1.0 m at stage X -600..-940, Z 200..300, its
  face at stage Y 7697); core draws the menu (section 5.4).

NSD_ONLY=front,counter,... (keys of BUILDERS) builds a subset; NSD_REVIEW=room,detail picks the review sheets
(art/review/ns_diner_room.jpg: the corner from the street at night and by day, the room; ns_diner_detail.jpg).
"""
import math
import os
import random

import bmesh
import bpy  # noqa: F401
import numpy as np
from mathutils import Matrix, Vector

from artkit import core, parts
from artkit.shade import mix_float, noise, ramp, scaled, smooth_less, vec
from assets import ns_barber as K
from assets.ns_barber import (H, Kit, add, bend, bx, cushion, finish_kind, frame_matrix, glow, grid_plane, is_k, lathe_shell, lathe_solid,
                              lighten, darken, molding, mul, prism_y, quad, render_image, screen_material, sub, text_mesh, text_width, tube,
                              unbaked, unbaked_mesh, voronoi, wall_plane)

LOG = 'ns_diner'
ANCHOR = (0.0, 69.5, 0.0)
CX, CY, R = 2.0, 2.0, 2.0         # the corner
RG = 1.88                          # the glass's radius at the corner (12 cm in from the face)
GZ = (0.62, 3.05)                  # the glass's bottom and top
FZ = (3.15, 4.30)                  # the fascia
CEIL = 3.80
PASS_Y = 7.50                      # the pass wall's dining face
DOOR_T = (math.radians(197.5), math.radians(252.5))   # the door's chord on the corner (angles from the corner's center)
RUBY = 0xb3201c
CREAM = 0xeee3c8
STEEL = 0xffffff

NAMES = {
    'front': ['SM_NS_Ruby_Front', 'SM_NS_Ruby_Front_Glass', 'SM_NS_Ruby_Neon_Red', 'SM_NS_Ruby_Neon_Blue'],
    'door': ['SM_NS_Ruby_Door'],
    'shell': ['SM_NS_Ruby_Shell', 'SM_NS_Ruby_Shell_GlowWarm'],
    'counter': ['SM_NS_Ruby_Counter'],
    'stool': ['SM_NS_Ruby_Stool'],
    'booth': ['SM_NS_Ruby_Booth'],
    'backbar': ['SM_NS_Ruby_Backbar', 'SM_NS_Ruby_Backbar_GlowWarm', 'SM_NS_Ruby_PieCarousel'],
    'pass': ['SM_NS_Ruby_Pass', 'SM_NS_Ruby_Pass_GlowWarm', 'SM_NS_Ruby_Pass_GlowCool'],
    'menu': ['SM_NS_Ruby_MenuBoard'],
}
_ONLY = [s.strip() for s in os.environ.get('NSD_ONLY', '').split(',') if s.strip()]
WHICH = [k for k in NAMES if not _ONLY or k in _ONLY]
MESHES = [n for k in WHICH for n in NAMES[k]]
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {}
AO_DISTANCE = 0.04
REVIEW_SCREEN_LIGHT = False
REVIEW_VIEWS = None
ALL = []


def kit(prefix, name, slots, anchor=ANCHOR, **kw):
    return Kit(prefix, name, slots, sizes=TEXTURE_SIZES, anchor=anchor, log=LOG, **kw)


def arc_pt(theta, r, z=0.0):
    return (CX + r * math.cos(theta), CY + r * math.sin(theta), z)


def front_line(t0=None, t1=None, fifth=(10.5, 2.0), market=(2.0, 15.0), step=math.radians(4.0)):
    """The storefront's line (on the faces), walked with the shop on the left: down the Fifth face (-y), round the
    corner (increasing angle) and along the Market face (+x). t0/t1 trim it to a span of the corner's angles."""
    a0, a1 = math.pi, 1.5 * math.pi
    pts = []
    if fifth is not None:
        pts += [(0.0, fifth[0]), (0.0, fifth[1])]
    lo = a0 if t0 is None else t0
    hi = a1 if t1 is None else t1
    n = max(2, int((hi - lo) / step) + 1)
    pts += [(CX + R * math.cos(lo + (hi - lo) * i / n), CY + R * math.sin(lo + (hi - lo) * i / n)) for i in range(n + 1)]
    if market is not None:
        pts += [(market[0], 0.0), (market[1], 0.0)]
    out = [pts[0]]
    for p in pts[1:]:
        if (Vector(p) - Vector(out[-1])).length > 1e-4:
            out.append(p)
    return out


# ================================================================== finishes the diner adds

@finish_kind('terrazzo')
def f_terrazzo(c):
    """Cast terrazzo: a cream cement matrix with marble chips (white, grey, black, rust, green) in two sizes, ground and
    sealed; brass divider strips on a 1.2 m grid (the 2.4 m tiled bake's seams fall under them)."""
    m = c.m
    P = c.P
    chips = []
    for scale, thr, seed in ((140.0, 0.30, 0.0), (330.0, 0.33, 3.0)):
        d = voronoi(m, vec(m, add(m, c.x, seed), c.y, 0.0), scale, 'F1', 'Distance')
        cr = voronoi(m, vec(m, add(m, c.x, seed), c.y, 0.0), scale, 'F1', 'Color')
        sepc = m.node('ShaderNodeSeparateColor')
        m.link(cr, sepc.inputs['Color'])
        chips.append((smooth_less(m, d, thr, 0.03), sepc.outputs['Red'], sepc.outputs['Green']))
    col = m.mix(ramp(m, noise(m, P, scale=3.0, detail=3.0), 0.3, 0.8), H(0xd2cbbd), H(0xc4bcad))
    for mask, r1, r2 in chips:
        tone = m.mix(ramp(m, r1, 0.0, 0.33), H(0x2a2826), H(0x8a8780))
        tone = m.mix(ramp(m, r1, 0.33, 0.66), tone, H(0xeeeae2))
        tone = m.mix(m.math('GREATER_THAN', r2, 0.86), tone, H(0x9a4a34))
        tone = m.mix(m.math('LESS_THAN', r2, 0.08), tone, H(0x5a7a5a))
        col = m.mix(mask, col, tone)
    gx = mul(m, m.math('MINIMUM', m.math('FRACT', m.math('DIVIDE', c.x, 1.2)), sub(m, 1.0, m.math('FRACT', m.math('DIVIDE', c.x, 1.2)))), 1.2)
    gy = mul(m, m.math('MINIMUM', m.math('FRACT', m.math('DIVIDE', c.y, 1.2)), sub(m, 1.0, m.math('FRACT', m.math('DIVIDE', c.y, 1.2)))), 1.2)
    strip = smooth_less(m, m.math('MINIMUM', gx, gy), 0.0022, 0.0004)
    col = m.mix(strip, col, H(0xb08d4a))
    wax = ramp(m, noise(m, P, scale=1.5, detail=4.0), 0.4, 0.8)
    rough = mix_float(m, strip, add(m, 0.22, mul(m, wax, 0.18)), 0.3)
    metal = mix_float(m, strip, 0.0, 1.0)
    return col, rough, metal, add(m, mul(m, strip, 0.2), mul(m, noise(m, P, scale=600.0, detail=1.0), 0.01))


_BOOMERANG = {}


def boomerang_image():
    """A seamless 0.4 m tile of 1950s boomerang laminate: cream with aqua, coral and charcoal boomerangs, gold flecks."""
    if 'path' in _BOOMERANG:
        return _BOOMERANG['path']
    W = 400.0
    rnd = random.Random(1950)

    def draw(s, w, h):
        s.rect(0, 0, w, h, 0xd9cfbd, rough=0.3)
        for _ in range(900):
            x, y = rnd.uniform(0, w), rnd.uniform(0, h)
            s.circle(x, y, rnd.uniform(0.3, 1.0), rnd.choice([0xc4b8a2, 0xe6ddcc, 0xbfae8c]), n=6)
        shapes = []
        for i in range(34):
            x, y = rnd.uniform(0, w), rnd.uniform(0, h)
            a = rnd.uniform(0, 2 * math.pi)
            L1, L2 = rnd.uniform(14, 26), rnd.uniform(12, 22)
            open_ = rnd.uniform(1.6, 2.4)
            col = rnd.choice([0x6fb7b0, 0x6fb7b0, 0xd88a78, 0x4a4a4a, 0xe0c060])
            shapes.append((x, y, a, L1, L2, open_, col, rnd.uniform(2.6, 4.0)))
        for (x, y, a, L1, L2, op, col, wd) in shapes:
            for dx in (-w, 0.0, w):
                for dy in (-h, 0.0, h):
                    p0 = (x + dx + L1 * math.cos(a), y + dy + L1 * math.sin(a))
                    p1 = (x + dx, y + dy)
                    p2 = (x + dx + L2 * math.cos(a + op), y + dy + L2 * math.sin(a + op))
                    if -40 < p1[0] < w + 40 and -40 < p1[1] < h + 40:
                        s.stroke([p0, ((p0[0] + p1[0]) / 2 + 2, (p0[1] + p1[1]) / 2 + 2), p1, ((p1[0] + p2[0]) / 2 - 2, (p1[1] + p2[1]) / 2 + 2), p2], wd, col, 0.3)
        for _ in range(140):
            x, y = rnd.uniform(0, w), rnd.uniform(0, h)
            for dx in (-w, 0.0, w):
                for dy in (-h, 0.0, h):
                    s.circle(x + dx, y + dy, rnd.uniform(0.6, 1.4), 0xc9a24a, metal=0.8, n=6)
    _BOOMERANG['path'] = render_image('ns_boomerang', W, W, 1024, draw)
    return _BOOMERANG['path']


@finish_kind('formica')
def f_formica(c):
    """Boomerang plastic laminate (0xd9cfbd): the printed tile repeating every 0.4 m in the plane the face lies in, cup rings,
    worn dull where plates slide."""
    m = c.m
    a = mix_float(m, m.math('GREATER_THAN', m.math('ABSOLUTE', c.nz), 0.5), add(m, c.x, c.y), c.x)
    b = mix_float(m, m.math('GREATER_THAN', m.math('ABSOLUTE', c.nz), 0.5), c.z, c.y)
    img = m.image(boomerang_image(), vector=vec(m, m.math('DIVIDE', a, 0.4), m.math('DIVIDE', b, 0.4), 0.0))
    worn = ramp(m, noise(m, c.P, scale=2.0, detail=4.0), 0.45, 0.75)
    col = m.mix(mul(m, worn, 0.15), img.outputs['Color'], H(0xe6dece))
    rings = mul(m, ramp(m, m.math('ABSOLUTE', sub(m, voronoi(m, c.P, 9.0, 'F1', 'Distance'), 0.35)), 0.012, 0.0),
                ramp(m, noise(m, c.P, scale=4.0, detail=2.0), 0.55, 0.7))
    col = m.mix(mul(m, rings, 0.35), col, H(0x8a6a48))
    scr = mul(m, K.isolines(m, scaled(m, c.sep, 100.0, 9.0, 1.0), 1.0, 0.004, 2.0), ramp(m, noise(m, c.P, scale=4.0, detail=2.0), 0.4, 0.6))
    col = m.mix(mul(m, scr, 0.3), col, H(0xa89880))
    return col, add(m, 0.22, mul(m, worn, 0.14), mul(m, scr, 0.06)), 0.0, mul(m, scr, -0.01)


@finish_kind('glassblock')
def f_glassblock(c):
    """Pressed glass block: pale blue-green, wavy flutes inside, a lighter rim; the kitchen's light through it."""
    m = c.m
    flute = m.math('SINE', mul(m, add(m, c.y, c.x, mul(m, c.z, 0.0)), 2 * math.pi / 0.025))
    wav = noise(m, scaled(m, c.sep, 40.0, 40.0, 40.0), detail=2.0)
    col = m.mix(mul(m, add(m, mul(m, flute, 0.5), 0.5), 0.45), H(0x8fb0aa), H(0xcfe2dc))
    col = m.mix(mul(m, wav, 0.35), col, H(0x6a8c88))
    return col, 0.06, 0.0, add(m, mul(m, flute, 0.4), mul(m, wav, 0.3))


@finish_kind('crust')
def f_crust(c):
    """Baked pastry or pancake: golden, browned in a mottle, a crumb texture."""
    m = c.m
    mott = noise(m, c.P, scale=60.0, detail=4.0)
    crumb = voronoi(m, c.P, 900.0, 'F1', 'Distance')
    side = ramp(m, m.math('ABSOLUTE', c.nz), 0.65, 0.25)
    col = m.mix(ramp(m, mott, 0.35, 0.75), lighten(m, c.col, 0.35), darken(m, c.col, 0.62))
    col = m.mix(mul(m, side, 0.85), col, lighten(m, c.col, 0.95))
    col = m.mix(mul(m, ramp(m, crumb, 0.4, 0.0), add(m, 0.2, mul(m, side, 0.25))), col, darken(m, c.col, 0.72))
    return col, add(m, 0.5, mul(m, mott, 0.15), mul(m, side, 0.15)), 0.0, add(m, mul(m, crumb, add(m, 0.25, mul(m, side, 0.4))), mul(m, mott, 0.15))


@finish_kind('syrup')
def f_syrup(c):
    """Fruit filling, syrup or glaze: glossy, deep, a little translucent-looking (darker in the hollows)."""
    m = c.m
    n = noise(m, c.P, scale=80.0, detail=3.0)
    col = m.mix(mul(m, n, 0.4), c.col, lighten(m, c.col, 0.25))
    return col, add(m, 0.08, mul(m, n, 0.05)), 0.0, mul(m, n, 0.15)


@finish_kind('cream')
def f_cream(c):
    """Whipped cream, meringue or butter: soft, matte, swirled."""
    m = c.m
    sw = noise(m, c.P, scale=90.0, detail=3.0, distortion=2.0)
    col = m.mix(mul(m, sw, 0.25), c.col, darken(m, c.col, 0.88))
    return col, add(m, 0.45, mul(m, sw, 0.1)), 0.0, mul(m, sw, 0.4)


# ================================================================== the storefront

def ruby_script():
    """RUBY'S as a neon script, in capital heights (baseline 0), left to right, slanted 10 degrees: lit runs (each one
    continuous tube with its own electrodes through the fascia, so no jumpers show)."""
    R_ = [(0.06, 0.0), (0.06, 0.5), (0.07, 1.0), (0.20, 1.03), (0.36, 1.02), (0.47, 0.93), (0.50, 0.80), (0.46, 0.66), (0.34, 0.57), (0.16, 0.55),
          (0.09, 0.555), (0.22, 0.53), (0.32, 0.42), (0.42, 0.20), (0.50, 0.04), (0.58, 0.0), (0.66, 0.06), (0.70, 0.22)]
    u_ = [(0.70, 0.22), (0.735, 0.50), (0.71, 0.30), (0.71, 0.10), (0.77, 0.0), (0.87, 0.02), (0.93, 0.16), (0.96, 0.50), (0.955, 0.26), (0.96, 0.07),
          (1.01, 0.0), (1.07, 0.06), (1.12, 0.26)]
    b_ = [(1.12, 0.26), (1.18, 0.62), (1.23, 0.95), (1.28, 1.05), (1.34, 1.03), (1.33, 0.90), (1.27, 0.62), (1.22, 0.30), (1.21, 0.08), (1.27, 0.0),
          (1.38, 0.02), (1.45, 0.17), (1.43, 0.36), (1.34, 0.46), (1.25, 0.42), (1.36, 0.38), (1.48, 0.42), (1.55, 0.50)]
    y_ = [(1.55, 0.50), (1.53, 0.30), (1.54, 0.10), (1.60, 0.0), (1.69, 0.03), (1.74, 0.20), (1.77, 0.50), (1.76, 0.15), (1.73, -0.15), (1.67, -0.38),
          (1.57, -0.45), (1.49, -0.38), (1.52, -0.26), (1.64, -0.17), (1.80, -0.06), (1.90, 0.04)]
    run1 = R_ + u_[1:] + b_[1:] + y_[1:]
    runs = [run1, [(2.00, 1.0), (2.01, 0.91), (1.97, 0.78)],
            [(2.06, 0.10), (2.13, 0.01), (2.24, 0.0), (2.31, 0.08), (2.28, 0.19), (2.17, 0.27), (2.11, 0.37), (2.15, 0.46), (2.24, 0.50), (2.31, 0.45)]]
    slant = math.tan(math.radians(10.0))
    runs = [core.catmull_rom([(x + slant * y, y) for x, y in r], 4) for r in runs]
    xs = [p[0] for r in runs for p in r]
    mid = (min(xs) + max(xs)) / 2
    runs = [[(p[0] - mid, p[1]) for p in r] for r in runs]
    return runs, [], max(xs) - min(xs)


def block_neon_paths(text, h, track=0.22):
    """`text` (2 4 H O U R S and spaces) in single-stroke block neon letters h tall: lit paths of (s, t) points in meters,
    s to the print's right (centered), t up from the baseline; and the total width."""
    w = h * 0.62

    def arc(xc, yc, rx, ry, a0, a1, n=10):
        return [(xc + rx * math.cos(a0 + (a1 - a0) * i / n), yc + ry * math.sin(a0 + (a1 - a0) * i / n)) for i in range(n + 1)]

    def glyph(c):
        if c == '2':
            return [arc(w / 2, h * 0.72, w / 2, h * 0.28, math.pi * 0.95, -math.pi * 0.22, 10) + [(0.0, 0.0), (w, 0.0)]]
        if c == '4':
            return [[(w * 0.74, 0.0), (w * 0.74, h), (0.0, h * 0.32), (w, h * 0.32)]]
        if c == 'H':
            return [[(0.0, h), (0.0, 0.0)], [(0.0, h * 0.5), (w, h * 0.5)], [(w, h), (w, 0.0)]]
        if c == 'O':
            return [arc(w / 2, h / 2, w / 2, h / 2, math.pi / 2, math.pi / 2 + 2 * math.pi - 0.12, 24)]
        if c == 'U':
            return [[(0.0, h)] + arc(w / 2, h * 0.32, w / 2, h * 0.32, math.pi, 2 * math.pi, 10) + [(w, h)]]
        if c == 'R':
            return [[(0.0, 0.0), (0.0, h), (w * 0.52, h)] + arc(w * 0.52, h * 0.75, w * 0.46, h * 0.25, math.pi / 2, -math.pi / 2, 8) +
                    [(0.0, h * 0.5)], [(w * 0.45, h * 0.5), (w, 0.0)]]
        if c == 'S':
            return [core.catmull_rom([(w * 0.96, h * 0.84), (w * 0.72, h * 0.99), (w * 0.28, h * 0.99), (w * 0.03, h * 0.80), (w * 0.12, h * 0.58),
                                      (w * 0.50, h * 0.49), (w * 0.88, h * 0.40), (w * 0.98, h * 0.18), (w * 0.72, h * 0.01), (w * 0.28, h * 0.01),
                                      (w * 0.03, h * 0.16)], 3)]
        return []

    paths, x = [], 0.0
    for c in text:
        if c == ' ':
            x += w * 0.55
            continue
        for p in glyph(c):
            paths.append([(x + s, t) for s, t in p])
        x += w + h * track
    total = x - h * track
    return [[(s - total / 2, t) for s, t in p] for p in paths], total


def on_corner(s, t, r, z0, h):
    """A point of the corner's script (s right, t up, in capital heights h) on the cylinder of radius r: reading
    left to right from the street, the angle grows to the right."""
    th = math.radians(225.0) + s * h / r
    return (CX + r * math.cos(th), CY + r * math.sin(th), z0 + t * h)


def front():
    k = kit('nsdf', 'SM_NS_Ruby_Front', ('frame', 'lit', 'print'), dust=0.12, low=0.3, print_px=512)
    TEXTURE_SIZES.update({'nsdf_frame': 2048, 'nsdf_lit': 1024})
    k.emit['lit'] = 0.7
    S = lambda o, aux=0: k.add(o, 'stainless', STEEL, aux=aux)
    # --- bulkheads: stainless skins over a black base, three speed-line ribs; the inner face red laminate with a
    # stainless cap (the wainscot under the windows)
    door_gap = (DOOR_T[0], DOOR_T[1])
    for path in (front_line(math.pi, door_gap[0], fifth=(10.5, 2.0), market=None), front_line(door_gap[1], 1.5 * math.pi, fifth=None, market=(2.0, 15.0))):
        k.add(molding('bulk', path, [(0.004, 0.08), (0.35, 0.08), (0.35, 0.55), (0.004, 0.55)]), 'stainless', STEEL, aux=0)
        k.add(molding('bulk_base', path, [(0.012, 0.0), (0.36, 0.0), (0.36, 0.08), (0.012, 0.08)]), 'rubber', 0x141414)
        for zr in (0.20, 0.27, 0.34):
            k.add(molding('rib', path, [(-0.006, zr - 0.008), (0.006, zr - 0.008), (0.006, zr + 0.008), (-0.006, zr + 0.008)]), 'chrome')
        k.add(molding('sill', path, [(-0.02, 0.55), (0.40, 0.55), (0.40, 0.62), (-0.01, 0.62)]), 'stainless', STEEL)
        k.add(molding('inner', path, [(0.35, 0.08), (0.37, 0.08), (0.37, 0.53), (0.35, 0.53)]), 'enamel', 0x8a1c1c)
    # --- the head and the fascia over the whole line, the speed lines, the coping; the inner band to the ceiling
    full = front_line()
    k.add(molding('head', full, [(-0.01, GZ[1]), (0.30, GZ[1]), (0.30, FZ[0]), (-0.01, FZ[0])]), 'stainless', STEEL)
    k.add(molding('fascia', full, [(0.0, FZ[0]), (0.28, FZ[0]), (0.28, FZ[1]), (0.0, FZ[1])]), 'enamel', RUBY)
    for zr in (3.24, 3.31, 3.38):
        k.add(molding('speed', full, [(-0.012, zr - 0.012), (0.0, zr - 0.012), (0.0, zr + 0.012), (-0.012, zr + 0.012)]), 'chrome')
    k.add(molding('coping', full, [(-0.03, FZ[1] - 0.02), (0.30, FZ[1] - 0.02), (0.30, FZ[1] + 0.01), (-0.03, FZ[1] + 0.01)]), 'stainless', STEEL)
    k.add(molding('inner_band', full, [(0.30, GZ[1]), (0.40, GZ[1]), (0.40, CEIL), (0.30, CEIL)]), 'paint', CREAM)
    k.add(molding('valance', full, [(0.30, GZ[1] - 0.02), (0.42, GZ[1] - 0.02), (0.42, GZ[1] + 0.10), (0.30, GZ[1] + 0.10)]), 'enamel', RUBY)
    # --- mullions: Fifth, the corner, Market; the glass block's frame
    mull_y = [2.0, 3.85, 5.70, 7.50]
    mull_x = [2.0, 3.95, 5.90, 7.85, 9.80, 11.75, 13.55]
    for y in mull_y:
        S(bx('mullion', 0.06, 0.18, y - 0.03, y + 0.03, GZ[0], GZ[1]), 2)
    for x in mull_x:
        S(bx('mullion', x - 0.03, x + 0.03, 0.06, 0.18, GZ[0], GZ[1]), 2)
    for th in (math.radians(188.0), DOOR_T[0], DOOR_T[1], math.radians(261.5)):
        p = Vector(arc_pt(th, RG))
        rad = (p - Vector((CX, CY, 0.0))).normalized()
        tang = Vector((-rad.y, rad.x, 0.0))
        o = bx('cmullion', -0.06, 0.06, -0.03, 0.03, GZ[0], GZ[1])
        o.data.transform(Matrix.Translation(p) @ Matrix((rad, tang, Vector((0, 0, 1)))).transposed().to_4x4())
        S(o, 2)
    S(bx('pier_e', 14.60, 15.0, 0.0, 0.40, 0.0, FZ[0]), 2)
    # --- the corner door's frame on the chord: jambs, the meeting stile stop, the header, a curved soffit to the transom
    pa, pb = Vector(arc_pt(DOOR_T[0], RG)), Vector(arc_pt(DOOR_T[1], RG))
    along = (pb - pa).normalized()
    out_n = Vector((-along.y, along.x, 0.0))
    if out_n.dot(Vector((-1, -1, 0))) < 0:
        out_n = -out_n
    DM = Matrix((along, -out_n, Vector((0, 0, 1)))).transposed().to_4x4()
    DM.translation = pa
    L = (pb - pa).length
    for (a, b) in ((0.0, 0.035), (L - 0.035, L)):
        o = bx('door_jamb', a, b, -0.06, 0.06, 0.0, 2.30)
        o.data.transform(DM)
        S(o, 2)
    o = bx('door_head', 0.0, L, -0.07, 0.07, 2.25, 2.33)
    o.data.transform(DM)
    S(o)
    o = bx('threshold', -0.05, L + 0.05, -0.09, 0.09, 0.0, 0.012, 0.004)
    o.data.transform(DM)
    k.add(o, 'stainless', STEEL)
    # the transom over the doors: its bar along the arc, the soffit closing the gap to the chord
    tr = [(CX + RG * math.cos(t), CY + RG * math.sin(t)) for t in np.linspace(DOOR_T[0], DOOR_T[1], 14)]
    k.add(molding('transom_bar', tr, [(-0.06, 2.30), (0.06, 2.30), (0.06, 2.36), (-0.06, 2.36)]), 'stainless', STEEL)
    bm = bmesh.new()
    arc_v = [bm.verts.new((x, y, 2.30)) for x, y in tr]
    chord_v = [bm.verts.new(tuple(pa.lerp(pb, i / 13) + Vector((0, 0, 2.30 - pa.z)))) for i in range(14)]
    for i in range(13):
        bm.faces.new((arc_v[i], arc_v[i + 1], chord_v[i + 1], chord_v[i]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    sof = core.mesh_object('soffit', bm)
    core.orient_normals(sof, (0, 0, -1))
    k.add(sof, 'stainless', STEEL)
    # the door's threshold apron between the chord and the corner's face (terrazzo, flush with the sidewalk)
    bm = bmesh.new()
    outer = [bm.verts.new((CX + R * math.cos(t), CY + R * math.sin(t), 0.004)) for t in np.linspace(DOOR_T[0], DOOR_T[1], 14)]
    inner = [bm.verts.new((CX + 1.60 * math.cos(t), CY + 1.60 * math.sin(t), 0.004)) for t in np.linspace(DOOR_T[0], DOOR_T[1], 14)]
    for i in range(13):
        bm.faces.new((outer[i], outer[i + 1], inner[i + 1], inner[i]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    ap = core.mesh_object('apron', bm)
    core.orient_normals(ap, (0, 0, 1))
    k.add(ap, 'terrazzo', 0xffffff)
    # --- the glass block wall at the kitchen end of the Fifth face (lit from behind: the lit slot)
    gb_y0, gb_y1 = 7.53, 10.47
    rows = int((GZ[1] - GZ[0]) / 0.20)
    cols = int((gb_y1 - gb_y0) / 0.20)
    z0 = GZ[0] + ((GZ[1] - GZ[0]) - rows * 0.20) / 2
    y00 = gb_y0 + ((gb_y1 - gb_y0) - cols * 0.20) / 2
    k.add(bx('mortar', 0.09, 0.13, gb_y0, gb_y1, GZ[0], GZ[1]), 'paint', 0x5e5a54)
    rnd = random.Random(7)
    for i in range(cols):
        for j in range(rows):
            y = y00 + 0.10 + i * 0.20
            z = z0 + 0.10 + j * 0.20
            o = bx('block', 0.06, 0.15, y - 0.094, y + 0.094, z - 0.094, z + 0.094, 0.012, 2)
            # the kitchen's light through the blocks: brightest high up under its tube fitting (stage Y 7840), falling off
            # down and along; the dish shelves' dark shapes behind the north end; each block a little different
            lamp = math.exp(-((y - 8.90) / 1.5) ** 2) * (0.55 + 0.45 * min(1.0, z / 2.6))
            lvl = 0.42 + 0.40 * lamp + rnd.uniform(-0.05, 0.05)
            if y > 9.55 and 1.0 < z < 2.15:
                lvl *= 0.55
            k.add(o, 'glassblock', 0xffffff, 'lit', aux=lvl)
    for (a, b) in ((gb_y0 - 0.03, gb_y0), (gb_y1, gb_y1 + 0.03)):
        S(bx('gb_frame', 0.04, 0.17, a, b, GZ[0], GZ[1]), 2)
    # --- the neon: RUBY'S on the corner's fascia (red script), OPEN / 24 HOURS in the first Fifth pane (blue)
    runs, jumps, wid = ruby_script()
    h = 0.62
    zN = 3.47
    rn = R + 0.04
    red = []
    for run in runs:
        pts = [on_corner(s_, t_, rn, zN, h) for s_, t_ in run]
        red.append(tube('ruby', pts, 0.0075, 8))
        for q in run[3::9]:
            a = Vector(on_corner(q[0], q[1], rn - 0.008, zN, h))
            b = Vector(on_corner(q[0], q[1], R + 0.002, zN, h))
            k.add(parts.rod('standoff', tuple(a), tuple(b), 0.005, 8), 'plastic', 0xd8d8d0)
        for end in (run[0], run[-1]):
            a = Vector(on_corner(end[0], end[1], rn, zN, h))
            b = Vector(on_corner(end[0], end[1], R + 0.002, zN, h))
            k.add(parts.rod('gto', tuple(a), tuple(b), 0.012, 12), 'rubber', 0x111111)
    for j in jumps:
        pts = [on_corner(s_, t_, rn - 0.01, zN, h) for s_, t_ in core.catmull_rom(j, 4)]
        k.add(tube('jumper', pts, 0.0065, 8), 'enamel', 0x0d0d0d)
    blue = []
    NX, NY, NZ = 0.16, 2.90, 1.62
    M = frame_matrix((0, -1, 0), (0, 0, 1), (NX, NY, NZ))
    lit, jump, total = K.neon_open_paths(0.15)
    for p in lit:
        blue.append(K.neon_tube('open', p, M, 0.0055))
    for p in jump:
        k.add(K.neon_tube('jumper', p, M, 0.0045), 'enamel', 0x0d0d0d)
    hours, _ = block_neon_paths('24 HOURS', 0.075)
    for p in hours:
        blue.append(K.neon_tube('hours', [(s_, t_ - 0.165) for s_, t_ in p], M, 0.0035, 6))
    fw, fh = max(total, 0.60) + 0.10, 0.43
    fx = NX + 0.04
    for (a, b, c_, d) in ((-fw / 2, fw / 2, -0.23, -0.218), (-fw / 2, fw / 2, fh - 0.24, fh - 0.228), (-fw / 2, -fw / 2 + 0.012, -0.23, fh - 0.228),
                          (fw / 2 - 0.012, fw / 2, -0.23, fh - 0.228)):
        k.add(bx('sign_frame', fx - 0.006, fx + 0.006, NY - b, NY - a, NZ + c_, NZ + d), 'enamel', 0x101010)
    for s_ in (-fw / 2 + 0.03, fw / 2 - 0.03):
        z = NZ + fh - 0.228
        while z < GZ[1] - 0.01:
            k.add(parts.rod('chain', (fx, NY - s_, z), (fx, NY - s_, min(z + 0.018, GZ[1])), 0.0018, 6), 'chrome')
            z += 0.02
    # --- window lettering on the Market panes (white vinyl with a red shade, read from the street)
    words = ['COFFEE', 'PIE', 'BREAKFAST', 'ALL DAY']
    for word, (xa, xb) in zip(words, zip(mull_x[1:], mull_x[2:])):
        cxw = (xa + xb) / 2
        size = min(0.22, (xb - xa - 0.30) / text_width(word, 1.0, 'Black', 1.05))
        K.street_letters(k, word, size, (cxw, 0.1222, 2.40), (1, 0, 0), (0, 0, 1), gold=False, face_col=0xf4f0e6, shade=0x9a1a16, line=0x9a1a16,
                         back=True, back_col=0xdcd6c8, tracking=1.05)
    k.grime = [((0.6, 0.6, 1.1), 0.5, 0.5)]
    obj = k.finish(40.0)
    # --- glass: Fifth panes, the corner's two curved lights and the transom, Market panes
    g = []
    for (a, b) in zip(mull_y, mull_y[1:]):
        g.append(K.pane_x(0.12, a + 0.03, b - 0.03, GZ[0], GZ[1]))
    for (a, b) in zip(mull_x, mull_x[1:] + [14.60]):
        o = quad('pane', [(a + 0.03, 0.12, GZ[0]), (b - 0.03, 0.12, GZ[0]), (b - 0.03, 0.12, GZ[1]), (a + 0.03, 0.12, GZ[1])])
        uv = o.data.uv_layers.new(name='UVMap')
        for li, t in enumerate(((0, 0), (1, 0), (1, 1), (0, 1))):
            uv.data[li].uv = t
        g.append(o)
    for (t0, t1, z0_, z1_) in ((math.pi, DOOR_T[0], GZ[0], GZ[1]), (DOOR_T[1], 1.5 * math.pi, GZ[0], GZ[1]), (DOOR_T[0], DOOR_T[1], 2.36, GZ[1])):
        n = 10
        bm = bmesh.new()
        uvl = bm.loops.layers.uv.new('UVMap')
        lo_ = [bm.verts.new(arc_pt(t0 + (t1 - t0) * i / n, RG, z0_)) for i in range(n + 1)]
        hi_ = [bm.verts.new(arc_pt(t0 + (t1 - t0) * i / n, RG, z1_)) for i in range(n + 1)]
        for i in range(n):
            f = bm.faces.new((lo_[i], lo_[i + 1], hi_[i + 1], hi_[i]))
            for l, (a, b) in zip(f.loops, ((i / n, 0), ((i + 1) / n, 0), ((i + 1) / n, 1), (i / n, 1))):
                l[uvl].uv = (a, b)
        g.append(core.mesh_object('curved', bm))
    glass = unbaked_mesh(g, 'SM_NS_Ruby_Front_Glass', 'ns_glass', ANCHOR, LOG)
    return [obj, glass, glow(red, 'SM_NS_Ruby_Neon_Red', ANCHOR, LOG), glow(blue, 'SM_NS_Ruby_Neon_Blue', ANCHOR, LOG)]


def door():
    """SM_NS_Ruby_Door (PROP): one leaf, hinge axis at the origin, running along +y; the same on both faces."""
    k = kit('nsdd', 'SM_NS_Ruby_Door', ('body', 'ns_glass'), anchor=(0.0, 0.0, 0.0), dust=0.05, low=0.25)
    TEXTURE_SIZES.update({'nsdd_body': 1024})
    W_, T, Hh = 0.82, 0.045, 2.22
    y0, y1 = 0.012, 0.012 + W_
    S = lambda o, aux=2: k.add(o, 'stainless', STEEL, aux=aux)
    S(bx('stile_h', -T / 2, T / 2, y0, y0 + 0.10, 0.01, Hh, 0.004))
    S(bx('stile_l', -T / 2, T / 2, y1 - 0.10, y1, 0.01, Hh, 0.004))
    S(bx('rail_t', -T / 2, T / 2, y0 + 0.10, y1 - 0.10, Hh - 0.10, Hh, 0.004), 0)
    S(bx('rail_b', -T / 2, T / 2, y0 + 0.10, y1 - 0.10, 0.01, 0.30, 0.004), 0)
    for sgn in (-1, 1):
        x = sgn * (T / 2)
        k.add(bx('kick', min(x, x + sgn * 0.0015), max(x, x + sgn * 0.0015), y0 + 0.01, y1 - 0.01, 0.02, 0.26, 0.001), 'chrome')
        py = y1 - 0.06
        for z in (0.92, 1.38):
            k.add(parts.rod('pull_post', (x, py, z), (x + sgn * 0.06, py, z), 0.009, 12), 'chrome')
        k.add(parts.rod('pull', (x + sgn * 0.06, py, 0.88), (x + sgn * 0.06, py, 1.42), 0.016, 20), 'chrome')
        for (a, b) in ((0.30, 0.32), (Hh - 0.12, Hh - 0.10)):
            k.add(bx('stop', min(x, x + sgn * 0.01), max(x, x + sgn * 0.01), y0 + 0.10, y1 - 0.10, a, b, 0.002), 'stainless', STEEL)
    for z in (0.25, 1.11, 1.97):
        k.add(parts.rod('hinge', (0.0, 0.0, z - 0.05), (0.0, 0.0, z + 0.05), 0.008, 12), 'stainless', STEEL)
    k.add(bx('sweep', -0.008, 0.008, y0 + 0.01, y1 - 0.01, 0.0, 0.012, 0.003), 'rubber', 0x111111)
    g = K.pane_x(0.0, y0 + 0.10, y1 - 0.10, 0.30, Hh - 0.10)
    k.special(g, 'ns_glass')
    k.polish = [((-0.08, y1 - 0.06, 1.15), 0.3, 0.6), ((0.08, y1 - 0.06, 1.15), 0.3, 0.6)]
    k.grime = [((0.0, y1 - 0.12, 1.15), 0.25, 0.6), ((0.0, (y0 + y1) / 2, 0.15), 0.4, 0.4)]
    return [k.finish(35.0)]


# ================================================================== the room

COUNTER_Y = (5.05, 5.80)          # the long leg's top, customer edge to the waitress edge
COUNTER_X = (4.85, 11.60)
SHORT_X = (4.10, 4.85)            # the short leg's top (west face at 4.10)
SHORT_Y1 = 6.40
TOP_Z = 1.06
BACKBAR_Y = (6.80, PASS_Y)
BB_Z = 0.91
PASS_X = (6.20, 9.20)
PASS_Z = (1.10, 1.75)
KDOOR_X = (11.85, 12.75)
RDOOR_X = (13.45, 14.30)
BOOTH_X = [5.00, 6.90, 8.80, 10.70, 12.60]
BOOTH_Y = 1.00
STOOLS = [(5.60 + 0.75 * i, 4.70) for i in range(7)] + [(3.72, 5.45), (3.72, 6.15)]
GLOBES = [(5.50, 5.45, 2.55), (7.70, 5.45, 2.55), (9.90, 5.45, 2.55)] + [(x, 0.95, 2.45) for x in BOOTH_X]
WAIN = 0x8a1c1c


def rounded_outline(r_in, x1=14.60, y1=PASS_Y, step=math.radians(5.0)):
    """The dining room's floor or ceiling outline: the corner rounded at radius r_in about the corner's center."""
    pts = [(R - r_in, y1), (R - r_in, CY)]
    n = int((0.5 * math.pi) / step)
    for i in range(1, n):
        t = math.pi + 0.5 * math.pi * i / n
        pts.append((CX + r_in * math.cos(t), CY + r_in * math.sin(t)))
    pts += [(CX, R - r_in), (x1, R - r_in), (x1, y1)]
    return pts


def poly_plane(name, outline, z, up=True):
    bm = bmesh.new()
    vs = [bm.verts.new((x, y, z)) for x, y in outline]
    f = bm.faces.new(vs if up else list(reversed(vs)))
    bm.normal_update()
    if (f.normal.z > 0) != up:
        f.normal_flip()
    bmesh.ops.triangulate(bm, faces=bm.faces[:])
    return core.mesh_object(name, bm)


def draw_sign(text, ground, ink, sub_=None):
    def draw(s, w, h):
        s.rect(0, 0, w, h, ground, rough=0.35)
        s.rect(3, 3, w - 6, h - 6, ink, rough=0.35)
        s.rect(5, 5, w - 10, h - 10, ground, rough=0.35)
        if sub_:
            s.mid(text, w / 2, h * 0.62, h * 0.36, w - 20, ink, 'Black', tracking=1.2)
            s.mid(sub_, w / 2, h * 0.26, h * 0.18, w - 20, ink, 'Bold', tracking=1.3)
        else:
            s.mid(text, w / 2, h * 0.5, h * 0.42, w - 20, ink, 'Black', tracking=1.2)
    return draw


def draw_exit_plain(s, w, h):
    s.rect(0, 0, w, h, 0xf4f2ec, rough=0.3)
    s.mid('EXIT', w / 2, h * 0.5, h * 0.62, w * 0.8, 0xd8261e, 'Black', tracking=1.1)


def draw_clipping(s, w, h):
    s.rect(0, 0, w, h, 0xe9e2cc, rough=0.8)
    s.mid('NORTHSIDE HERALD', w / 2, h - 12, 10, w - 20, 0x1a1a1a, 'Black')
    s.mid("RUBY'S AT 30: THE GRILL", w / 2, h - 34, 12, w - 16, 0x1a1a1a, 'Black')
    s.mid('THAT NEVER SLEEPS', w / 2, h - 50, 12, w - 16, 0x1a1a1a, 'Black')
    s.rect(10, h * 0.32, w * 0.5, h * 0.36, 0x5a5650)
    s.circle(10 + w * 0.25, h * 0.52, h * 0.08, 0x8a8478)
    r = random.Random(30)
    y = h * 0.66
    while y > 8:
        x0 = w * 0.58 if y > h * 0.3 else 10
        s.rect(x0, y, (w - 10 - x0) * r.uniform(0.75, 1.0), 1.4, 0x3a3a36)
        y -= 4.0


def draw_juke_panel(col_a, col_b):
    def draw(s, w, h):
        s.rect(0, 0, w, h, col_a, rough=0.2)
        for i in range(8):
            s.rect(0, h * i / 8, w, h * 0.06, col_b, rough=0.2)
    return draw


def draw_juke_titles(s, w, h):
    s.rect(0, 0, w, h, 0xf4efe0, rough=0.4)
    r = random.Random(45)
    songs = ['BLUE NIGHT', 'FIFTH ST STOMP', 'RIVER RUN', 'ALL NIGHT LONG', 'CHERRY PIE', 'LAST CALL', 'MOON OVER MARKET', 'NIGHT SHIFT',
             'SATURDAY SWING', 'HOLD ME', 'CORNER BOOTH', 'TWO EGGS']
    for i in range(12):
        col_ = i % 4
        row = i // 4
        x = w * (0.02 + 0.245 * col_)
        y = h * (0.70 - row * 0.32)
        s.rect(x, y, w * 0.23, h * 0.26, 0xfffaf0, rough=0.5)
        s.rect(x, y + h * 0.12, w * 0.23, 0.6, 0xc4231c)
        s.mid(songs[i], x + w * 0.115, y + h * 0.19, h * 0.06, w * 0.21, 0x1a1a1a, 'Bold')
        s.mid(f'{chr(65 + i)}{r.randint(1, 9)}', x + w * 0.115, y + h * 0.06, h * 0.06, w * 0.21, 0xc4231c, 'Black')


def jukebox(k, M):
    """A floor jukebox in its own frame (front toward -y, origin on the floor at its middle), placed by M."""
    objs = []
    W, D = 0.86, 0.62
    objs.append((bx('jb_base', -W / 2, W / 2, -D / 2, D / 2, 0.0, 0.10, 0.01), 'enamel', 0x1a1a1a, 0))
    objs.append((bx('jb_body', -W / 2 + 0.02, W / 2 - 0.02, -D / 2 + 0.02, D / 2, 0.10, 0.95, 0.02, 2), 'wood', 0x5a2a14, 0))
    objs.append((bx('jb_grille', -0.30, 0.30, -D / 2 + 0.005, -D / 2 + 0.03, 0.18, 0.62, 0.02, 2), 'chrome', 0xffffff, 0))
    for i in range(9):
        z = 0.21 + i * 0.045
        objs.append((parts.rod('jb_bar', (-0.28, -D / 2 + 0.0, z), (0.28, -D / 2 + 0.0, z), 0.006, 8), 'chrome', 0xffffff, 0))
    objs.append((bx('jb_window', -0.33, 0.33, -D / 2 + 0.06, -D / 2 + 0.10, 0.95, 1.22, 0.01), 'glassdark', 0x1a1410, 0))
    objs.append((bx('jb_buttons', -0.33, 0.33, -D / 2 + 0.015, -D / 2 + 0.06, 0.70, 0.78, 0.01), 'chrome', 0xffffff, 0))
    for o, f, c, a in objs:
        o.data.transform(M)
        k.add(o, f, c, aux=a)
    # the arch: a lit band (printed glow panels) round a chrome frame, and the dome
    arch = []
    for i in range(25):
        a = math.pi * i / 24
        arch.append((0.42 * math.cos(a), 0.95 + 0.30 + 0.30 * math.sin(a)))
    for side, yy in (('front', -D / 2 + 0.035),):
        bm = bmesh.new()
        uvl = bm.loops.layers.uv.new('UVMap')
        outer = [bm.verts.new(tuple(M @ Vector((x, yy, z)))) for x, z in arch]
        inner = [bm.verts.new(tuple(M @ Vector((x * 0.82, yy - 0.002, 0.95 + 0.30 + (z - 1.25) * 0.78)))) for x, z in arch]
        for i in range(24):
            f = bm.faces.new((outer[i], outer[i + 1], inner[i + 1], inner[i]))
            for l, (u, v) in zip(f.loops, ((i / 24, 1), ((i + 1) / 24, 1), ((i + 1) / 24, 0), (i / 24, 0))):
                l[uvl].uv = (u, v)
        bm.normal_update()
        for f in bm.faces:
            if f.normal.dot(M.to_3x3() @ Vector((0, -1, 0))) < 0:
                f.normal_flip()
        o = core.mesh_object('jb_arch', bm)
        u0, v0, u1, v1 = k.atlas.rect('juke_arch')
        uv = o.data.uv_layers['UVMap']
        for li in range(len(uv.data)):
            a_, b_ = uv.data[li].uv
            uv.data[li].uv = (u0 + (u1 - u0) * a_, v0 + (v1 - v0) * b_)
        K.set_attr(o, 'ns_fin', 0.0)
        K.set_attr(o, 'ns_col', (1.0, 1.0, 1.0))
        K.set_attr(o, 'ns_aux', 0.0)
        K.set_attr(o, 'ns_lock', 1.0)
        o['ns_slot'] = 'print'
        k.objs.append(o)
    hood = core.loft('jb_hood', [[tuple(M @ Vector((x * s_, y_, z * s_ + 1.25 * (1 - s_)))) for x, z in [(0.43 * math.cos(math.pi * i / 16), 1.25 + 0.31 * math.sin(math.pi * i / 16))
                                                                                                 for i in range(17)] + [(-0.43, 0.95), (0.43, 0.95)]]
                                  for y_, s_ in ((-D / 2 + 0.03, 1.0), (D / 2 - 0.02, 1.0))])
    k.add(hood, 'wood', 0x5a2a14)
    k.decal('juke_titles', tuple(M @ Vector((0.0, -D / 2 + 0.059, 0.86))), u=tuple(M.to_3x3() @ Vector((1, 0, 0))), v=tuple(M.to_3x3() @ Vector((0, 0, 1))),
            w=0.62, h=0.13, lift=0.0004)


def gumball(k, M, col_seed):
    """A gumball machine on a cast stand, in its own frame (origin on the floor)."""
    rnd = random.Random(col_seed)
    k.add(lathe_solid('gb_foot', [(0.0, 0.0), (0.16, 0.0), (0.15, 0.03), (0.04, 0.06), (0.03, 0.70), (0.06, 0.74), (0.0, 0.74)], M, 32), 'enamel', 0xb3201c)
    k.add(lathe_solid('gb_body', [(0.0, 0.74), (0.10, 0.74), (0.11, 0.78), (0.09, 0.90), (0.0, 0.90)], M, 32), 'enamel', 0xb3201c)
    k.add(lathe_solid('gb_glass', [(0.0, 0.90), (0.10, 0.905), (0.14, 0.98), (0.145, 1.04), (0.12, 1.12), (0.06, 1.16), (0.0, 1.165)], M, 40), 'glassdark', 0xd8e4e4)
    for i in range(70):
        a = rnd.uniform(0, 2 * math.pi)
        zz = rnd.uniform(0.92, 1.08)
        rr = min(0.125, 0.10 + 0.04 * math.sin((zz - 0.90) / 0.26 * math.pi)) * rnd.uniform(0.7, 1.0) + 0.012
        p = M @ Vector((rr * math.cos(a), rr * math.sin(a), zz))
        k.add(parts.rbox('gum', tuple(p), (0.024, 0.024, 0.024), 0.0119, 2), 'gloss', rnd.choice([0xd8261e, 0xf2c230, 0x2a6fd8, 0x2a9a4a, 0xf2f2ea, 0xe8681c]))
    k.add(lathe_solid('gb_cap', [(0.0, 1.16), (0.06, 1.16), (0.065, 1.18), (0.02, 1.20), (0.0, 1.21)], M, 24), 'enamel', 0xb3201c)
    k.add(parts.rod('gb_knob', tuple(M @ Vector((0, -0.10, 0.83))), tuple(M @ Vector((0, -0.13, 0.83))), 0.03, 20), 'chrome')
    ch = bx('gb_chute', -0.03, 0.03, -0.12, -0.09, 0.76, 0.79, 0.006)
    ch.data.transform(M)
    k.add(ch, 'chrome')


def shell():
    k = kit('nsds', 'SM_NS_Ruby_Shell', ('floor', 'walls', 'ceiling', 'print'), dust=0.0, low=0.0, cavity=0.02, print_px=1024)
    # the floor and the ceiling are tiled bakes: anything added without a slot (the furniture, the trims) goes to walls
    k.default = 'walls'
    TEXTURE_SIZES.update({'nsds_floor': 2048, 'nsds_walls': 2048, 'nsds_ceiling': 1024, 'nsds_print': 1024})
    k.tile_slot('floor', 'xy', (2.4, 2.4), origin=(0.0, 0.0), z=0.0)
    k.tile_slot('ceiling', 'xy', (2.0, 2.0), origin=(0.0, 0.0), z=CEIL, down=True)
    k.panel('restrooms', 0.30, 0.10, draw_sign('RESTROOMS', 0xf2efe6, 0x1a1a1a))
    k.panel('employees', 0.30, 0.10, draw_sign('EMPLOYEES ONLY', 0x1a1a1a, 0xf2efe6))
    k.panel('nosmoke', 0.20, 0.20, draw_sign('NO SMOKING', 0xf2efe6, 0xb3201c, 'BY LAW'))
    k.panel('clipping', 0.24, 0.30, draw_clipping, density=1.3)
    k.panel('juke_arch', 0.80, 0.10, draw_juke_panel(0xff8a2a, 0xffd23a), glow=3.0)
    k.panel('juke_titles', 0.62, 0.13, draw_juke_titles, density=1.5)
    k.panel('exit', 0.30, 0.15, draw_exit_plain, glow=2.0)
    k.atlas.render()
    # the floor (terrazzo, tiled) inside the bulkheads' inner face; the ceiling inside the head's inner band
    k.add(poly_plane('floor', rounded_outline(R - 0.355), 0.0), 'terrazzo', slot='floor')
    k.add(poly_plane('ceiling', rounded_outline(R - 0.40), CEIL, up=False), 'paint', 0xf2ece0, slot='ceiling')
    # the pass wall (y 7.50) and the east wall (x 14.60): a red laminate wainscot with a stainless kick and cap, cream above
    pw = ((0.40, PASS_Y), (14.60, PASS_Y))
    holes = [(PASS_X[0] - 0.40, PASS_X[1] - 0.40, PASS_Z[0], PASS_Z[1]), (KDOOR_X[0] - 0.40, KDOOR_X[1] - 0.40, 0.0, 2.13),
             (RDOOR_X[0] - 0.40, RDOOR_X[1] - 0.40, 0.0, 2.13)]
    for (p0, p1, nrm, hs) in ((pw[0], pw[1], (0, -1), holes), ((14.60, 0.40), (14.60, PASS_Y), (-1, 0), [])):
        k.add(wall_plane('wain', p0, p1, 0.0, 1.06, nrm, 2.0, [h for h in hs if h[2] < 1.06]), 'enamel', WAIN, slot='walls')
        k.add(wall_plane('wall', p0, p1, 1.06, CEIL, nrm, 2.4, [h for h in hs if h[3] > 1.06]), 'paint', CREAM, slot='walls')
    # the kick, cap and chrome line stop either side of the backbar (its splash covers the wall there)
    wpath = [[(14.60, 0.40), (14.60, PASS_Y), (RDOOR_X[1] + 0.05, PASS_Y)], [(RDOOR_X[0] - 0.05, PASS_Y), (KDOOR_X[1] + 0.05, PASS_Y)],
             [(KDOOR_X[0] - 0.05, PASS_Y), (11.62, PASS_Y)], [(4.38, PASS_Y), (0.40, PASS_Y)]]
    for wp in wpath:
        k.add(molding('kick', wp, [(0.0, 0.0), (0.012, 0.0), (0.012, 0.15), (0.0, 0.15)]), 'stainless', STEEL)
        k.add(molding('cap', wp, [(0.0, 1.0), (0.025, 1.0), (0.03, 1.02), (0.03, 1.05), (0.02, 1.065), (0.0, 1.065)]), 'stainless', STEEL)
        k.add(molding('trim', wp, [(0.0, 0.545), (0.006, 0.545), (0.006, 0.565), (0.0, 0.565)]), 'chrome')
    k.add(molding('crown', [(14.60, 0.40), (14.60, PASS_Y), (0.40, PASS_Y)], [(0.0, CEIL - 0.08), (0.02, CEIL - 0.08), (0.06, CEIL - 0.03),
                                                                            (0.07, CEIL), (0.0, CEIL)]), 'enamel', CREAM)
    # the soffit over the counter: red face, speed lines, cream underside
    sx0, sx1, sy0, sz0 = 3.80, 11.90, 4.95, 3.25
    k.add(bx('soffit_face', sx0, sx1, sy0, sy0 + 0.03, sz0, CEIL, 0.004), 'enamel', RUBY, slot='walls')
    for xe in (sx0, sx1 - 0.03):
        k.add(bx('soffit_end', xe, xe + 0.03, sy0, PASS_Y, sz0, CEIL, 0.004), 'enamel', RUBY, slot='walls')
    k.add(bx('soffit_under', sx0, sx1, sy0, PASS_Y, sz0 - 0.002, sz0 + 0.02), 'paint', 0xf2ece0, slot='walls')
    for zr in (sz0 + 0.10, sz0 + 0.17, sz0 + 0.24):
        k.add(parts.rod('soffit_speed', (sx0 - 0.005, sy0 - 0.006, zr), (sx1 + 0.005, sy0 - 0.006, zr), 0.008, 10), 'chrome')
    # the ceiling's fixtures: four square supply diffusers and a return grille, two smoke detectors, a sprinkler
    # head per bay, and the EXIT sign hung inside the corner doors (both faces lit)
    for (cx, cy) in ((4.6, 2.5), (8.6, 2.5), (12.6, 2.5), (13.2, 6.1)):
        k.add(bx('diffuser', cx - 0.3, cx + 0.3, cy - 0.3, cy + 0.3, CEIL - 0.012, CEIL, 0.004), 'enamel', 0xece9e0)
        for j in range(4):
            r_ = 0.26 - j * 0.06
            k.add(bx('diff_ring', cx - r_, cx + r_, cy - r_, cy + r_, CEIL - 0.035 + j * 0.005, CEIL - 0.012, 0.003), 'enamel', 0xe2ded4)
        k.add(bx('diff_core', cx - 0.05, cx + 0.05, cy - 0.05, cy + 0.05, CEIL - 0.05, CEIL - 0.03, 0.003), 'enamel', 0xe2ded4)
    rx_, ry_ = 2.9, 6.4
    k.add(bx('return', rx_ - 0.31, rx_ + 0.31, ry_ - 0.31, ry_ + 0.31, CEIL - 0.014, CEIL, 0.004), 'enamel', 0xe6e2d8)
    for j in range(12):
        x_ = rx_ - 0.26 + j * 0.0473
        k.add(bx('return_bar', x_ - 0.006, x_ + 0.006, ry_ - 0.27, ry_ + 0.27, CEIL - 0.03, CEIL - 0.014, 0.002), 'enamel', 0xd8d4ca)
    for (cx, cy) in ((6.6, 3.2), (12.4, 4.6)):
        k.add(lathe_solid('smoke', [(0.0, CEIL - 0.045), (0.06, CEIL - 0.045), (0.065, CEIL - 0.03), (0.065, CEIL), (0.0, CEIL)], Matrix.Translation((cx, cy, 0.0)),
                          40), 'plastic', 0xefede6)
        k.add(lathe_solid('smoke_led', [(0.0, CEIL - 0.048), (0.004, CEIL - 0.048), (0.004, CEIL - 0.045), (0.0, CEIL - 0.045)],
                          Matrix.Translation((cx + 0.03, cy, 0.0)), 8), 'plastic', 0xc4231c)
    for (cx, cy) in ((3.2, 1.6), (6.6, 1.6), (10.6, 1.6), (13.6, 4.0)):
        k.add(lathe_solid('sprinkler', [(0.0, CEIL - 0.006), (0.035, CEIL - 0.006), (0.035, CEIL), (0.0, CEIL)], Matrix.Translation((cx, cy, 0.0)), 24), 'chrome')
        k.add(parts.rod('spr_body', (cx, cy, CEIL - 0.006), (cx, cy, CEIL - 0.045), 0.006, 10), 'brass')
        k.add(lathe_solid('spr_deflector', [(0.0, CEIL - 0.052), (0.014, CEIL - 0.052), (0.014, CEIL - 0.048), (0.0, CEIL - 0.048)],
                          Matrix.Translation((cx, cy, 0.0)), 12), 'brass')
    ex_c = Vector((1.02, 1.02, 2.66))
    ex_u = Vector((-1.0, 1.0, 0.0)).normalized()
    ex_n = Vector((1.0, 1.0, 0.0)).normalized()
    eb = bx('exit_box', -0.17, 0.17, -0.022, 0.022, -0.10, 0.10, 0.008)
    eb.data.transform(Matrix.Translation(ex_c) @ Matrix((ex_u, -ex_n, Vector((0, 0, 1)))).transposed().to_4x4())
    k.add(eb, 'plastic', 0xf1efe8)
    for s_ in (-0.12, 0.12):
        a_ = ex_c + ex_u * s_ + Vector((0, 0, 0.10))
        k.add(parts.rod('exit_stem', tuple(a_), (a_.x, a_.y, CEIL), 0.004, 8), 'chrome')
    k.add(lathe_solid('exit_canopy', [(0.0, CEIL - 0.02), (0.05, CEIL - 0.02), (0.055, CEIL), (0.0, CEIL)], Matrix.Translation((ex_c.x, ex_c.y, 0.0)), 24),
          'plastic', 0xf1efe8)
    k.decal('exit', tuple(ex_c + ex_n * 0.0225), u=tuple(ex_u), v=(0, 0, 1), lift=0.0002)
    k.decal('exit', tuple(ex_c - ex_n * 0.0225), u=tuple(-ex_u), v=(0, 0, 1), lift=0.0002)
    # the kitchen's swing door and the restroom door (closed), their signs
    kd0, kd1 = KDOOR_X
    k.add(bx('kd_slab', kd0 + 0.02, kd1 - 0.02, PASS_Y + 0.005, PASS_Y + 0.05, 0.01, 2.11, 0.004), 'stainless', STEEL, slot='walls', aux=2)
    k.add(parts.rod('kd_port', ((kd0 + kd1) / 2, PASS_Y + 0.004, 1.55), ((kd0 + kd1) / 2, PASS_Y + 0.05, 1.55), 0.17, 32), 'glassdark', 0x2a2a28, slot='walls')
    k.add(lathe_solid('kd_ring', [(0.0, -0.002), (0.19, -0.002), (0.19, 0.006), (0.0, 0.006)],
                      Matrix.Translation(((kd0 + kd1) / 2, PASS_Y + 0.002, 1.55)) @ Matrix.Rotation(math.radians(90), 4, 'X'), 40), 'chrome', slot='walls')
    k.add(bx('kd_kick', kd0 + 0.03, kd1 - 0.03, PASS_Y + 0.002, PASS_Y + 0.005, 0.02, 0.35, 0.002), 'chrome', slot='walls')
    for xx in (kd0, kd1 - 0.04):
        k.add(bx('kd_jamb', xx, xx + 0.04, PASS_Y - 0.01, PASS_Y + 0.06, 0.0, 2.15, 0.004), 'stainless', STEEL, slot='walls')
    k.add(bx('kd_head', kd0, kd1, PASS_Y - 0.01, PASS_Y + 0.06, 2.11, 2.19, 0.004), 'stainless', STEEL, slot='walls')
    rd0, rd1 = RDOOR_X
    k.add(bx('rd_slab', rd0 + 0.02, rd1 - 0.02, PASS_Y + 0.005, PASS_Y + 0.05, 0.01, 2.11, 0.004), 'enamel', 0x5a2a1c, slot='walls')
    for xx in (rd0 - 0.05, rd1):
        k.add(bx('rd_casing', xx, xx + 0.05, PASS_Y - 0.015, PASS_Y, 0.0, 2.18, 0.004), 'enamel', CREAM, slot='walls')
    k.add(bx('rd_head', rd0 - 0.05, rd1 + 0.05, PASS_Y - 0.015, PASS_Y, 2.13, 2.20, 0.004), 'enamel', CREAM, slot='walls')
    k.add(bx('rd_push', rd1 - 0.14, rd1 - 0.04, PASS_Y + 0.0, PASS_Y + 0.004, 1.05, 1.40, 0.002), 'chrome', slot='walls')
    k.decal('restrooms', ((rd0 + rd1) / 2, PASS_Y + 0.0045, 1.62), u=(1, 0, 0), v=(0, 0, 1), lift=0.0003)
    k.decal('employees', ((kd0 + kd1) / 2, PASS_Y + 0.0045, 1.25), u=(1, 0, 0), v=(0, 0, 1), lift=0.0003)
    k.decal('nosmoke', (14.598, 2.2, 1.70), u=(0, 1, 0), v=(0, 0, 1), lift=0.0003)
    k.add(bx('clip_frame', 14.575, 14.60, 3.30, 3.62, 1.50, 1.88, 0.004), 'wood', 0x2a1a10, slot='walls')
    k.decal('clipping', (14.574, 3.46, 1.69), u=(0, 1, 0), v=(0, 0, 1), lift=0.0003)
    # the jukebox on the east wall; gumball machines, the newspaper rack and a coat tree by the door
    jukebox(k, Matrix.Translation((14.20, 4.60, 0.0)) @ Matrix.Rotation(math.radians(-90), 4, 'Z'))
    gumball(k, Matrix.Translation((0.62, 3.25, 0.0)), 3)
    gumball(k, Matrix.Translation((0.62, 3.65, 0.0)), 8)
    nm = Matrix.Translation((0.62, 4.40, 0.0))
    for (dx, dy) in ((-0.18, -0.22), (0.18, -0.22), (-0.18, 0.22), (0.18, 0.22)):
        k.add(parts.rod('rack_leg', tuple(nm @ Vector((dx, dy, 0.0))), tuple(nm @ Vector((dx, dy, 0.95))), 0.008, 8), 'iron')
    for zz in (0.35, 0.65, 0.95):
        k.add(bx('rack_tray', 0.44, 0.80, 4.18, 4.62, zz - 0.01, zz, 0.003), 'iron')
        for i in range(3):
            k.add(bx('papers', 0.47 + i * 0.003, 0.77 - i * 0.004, 4.21, 4.59, zz, zz + 0.02 + i * 0.01, 0.003), 'paper', 0xe0dac8)
    k.add(parts.rod('tree_pole', (0.70, 6.95, 0.0), (0.70, 6.95, 1.78), 0.016, 12), 'wood', 0x2a1a10, aux=2)
    k.add(lathe_solid('tree_foot', [(0.0, 0.0), (0.22, 0.0), (0.20, 0.03), (0.05, 0.06), (0.0, 0.06)], Matrix.Translation((0.70, 6.95, 0.0)), 32), 'wood', 0x2a1a10, aux=2)
    for a in range(6):
        aa = a * math.pi / 3
        k.add(tube('tree_hook', bend([(0.70, 6.95, 1.70), (0.70 + 0.14 * math.cos(aa), 6.95 + 0.14 * math.sin(aa), 1.66),
                                      (0.70 + 0.17 * math.cos(aa), 6.95 + 0.17 * math.sin(aa), 1.72)], 0.03, 4), 0.006, 8), 'brass')
    # the globes' canopies, stems and fitters (the globes themselves are the glow mesh)
    for (x, y, z) in GLOBES:
        top = sz0 if (sx0 < x < sx1 and y > sy0) else CEIL
        M = Matrix.Translation((x, y, 0.0))
        k.add(lathe_solid('canopy', [(0.0, top - 0.035), (0.06, top - 0.035), (0.07, top - 0.02), (0.07, top), (0.0, top)], M, 32), 'chrome', slot='walls')
        k.add(parts.rod('stem', (x, y, top - 0.035), (x, y, z + 0.20), 0.008, 12), 'chrome', slot='walls')
        k.add(lathe_solid('fitter', [(0.0, z + 0.20), (0.02, z + 0.20), (0.05, z + 0.18), (0.062, z + 0.15), (0.06, z + 0.14), (0.0, z + 0.14)], M, 32),
              'chrome', slot='walls')
    obj = k.finish(40.0)
    globes = []
    for (x, y, z) in GLOBES:
        M = Matrix.Translation((x, y, 0.0))
        prof = core.catmull_rom([(0.0, z - 0.17), (0.07, z - 0.16), (0.13, z - 0.10), (0.16, z), (0.15, z + 0.08), (0.10, z + 0.14), (0.05, z + 0.16),
                                 (0.0, z + 0.165)], 3)
        globes.append(lathe_solid('globe', prof, M, 32))
    gl = glow(globes, 'SM_NS_Ruby_Shell_GlowWarm', ANCHOR, LOG)
    return [obj, gl]


def counter():
    k = kit('nsdc', 'SM_NS_Ruby_Counter', ('body', 'print'), dust=0.05, low=0.15, print_px=1024)
    TEXTURE_SIZES.update({'nsdc_body': 4096})
    k.panel('menu_card', 0.22, 0.30, draw_menu_card)
    k.atlas.render()
    cx0, cx1 = COUNTER_X
    cy0, cy1 = COUNTER_Y
    sx0, sx1 = SHORT_X
    # the laminate top: an L with its outer corner rounded, a ribbed chrome edge on the customer side
    outline = core.rounded_profile([(sx1, SHORT_Y1), (sx0, SHORT_Y1), (sx0, cy0), (cx1, cy0), (cx1, cy1), (sx1, cy1), (sx1, SHORT_Y1)],
                                   [0, 0.04, 0.25, 0.04, 0.04, 0, 0], 8)[:-1]
    top = parts.panel('top', outline, TOP_Z - 0.03, TOP_Z)
    core.orient_normals(top)
    k.add(top, 'formica', 0xd9cfbd)
    edge_path = core.rounded_profile([(sx1, SHORT_Y1), (sx0, SHORT_Y1), (sx0, cy0), (cx1, cy0), (cx1, cy1)], [0, 0.04, 0.25, 0.04, 0], 8)
    band = [(-0.012, TOP_Z - 0.045), (0.0, TOP_Z - 0.045), (0.0, TOP_Z + 0.004), (-0.012, TOP_Z + 0.004)]
    k.add(molding('edge', edge_path, band), 'chrome')
    for zr in (TOP_Z - 0.035, TOP_Z - 0.022, TOP_Z - 0.009):
        k.add(molding('edge_rib', edge_path, [(-0.016, zr - 0.003), (-0.012, zr - 0.003), (-0.012, zr + 0.003), (-0.016, zr + 0.003)]), 'chrome')
    # the body: stainless faces set 10 cm back, chrome battens, a black kick; the foot rail on brackets
    body_path = core.rounded_profile([(sx1, SHORT_Y1 - 0.10), (sx0 + 0.10, SHORT_Y1 - 0.10), (sx0 + 0.10, cy0 + 0.10), (cx1 - 0.10, cy0 + 0.10),
                                      (cx1 - 0.10, cy1)], [0, 0.0, 0.18, 0.0, 0], 8)
    k.add(molding('body', body_path, [(0.0, 0.15), (0.40, 0.15), (0.40, TOP_Z - 0.03), (0.0, TOP_Z - 0.03)]), 'stainless', STEEL, aux=2)
    k.add(molding('kick', body_path, [(0.06, 0.0), (0.40, 0.0), (0.40, 0.15), (0.06, 0.15)]), 'rubber', 0x141414)
    L_ = 0.0
    for a, b in zip(body_path, body_path[1:]):
        seg = Vector(b) - Vector(a)
        n_ = int(seg.length / 0.60)
        nrm = Vector((-seg.y, seg.x, 0.0)).normalized() if seg.length > 0 else Vector((0, 0, 0))
        for i in range(1, n_ + 1):
            p = Vector(a) + seg * (i / (n_ + 1))
            k.add(parts.rod('batten', (p.x - nrm.x * 0.006, p.y - nrm.y * 0.006, 0.16), (p.x - nrm.x * 0.006, p.y - nrm.y * 0.006, TOP_Z - 0.05), 0.009, 10), 'chrome')
    rail_path = core.rounded_profile([(sx0 - 0.08, SHORT_Y1 - 0.25), (sx0 - 0.08, cy0 - 0.08), (cx1 - 0.25, cy0 - 0.08)], [0, 0.30, 0], 8)
    k.add(tube('foot_rail', [(x, y, 0.22) for x, y in rail_path], 0.022, 14), 'chrome')
    for (a, b) in ((rail_path[0], (sx0 - 0.08, cy0 - 0.08)), ((sx0 - 0.08, cy0 - 0.08), rail_path[-1])):
        A, B = Vector((a[0], a[1], 0.0)), Vector((b[0], b[1], 0.0))
        d = (B - A).normalized()
        left = Vector((-d.y, d.x, 0.0))
        n_ = max(1, int((B - A).length / 1.1))
        for i in range(n_ + 1):
            p = A.lerp(B, 0.08 + 0.84 * i / n_)
            k.add(tube('rail_bracket', bend([(p.x, p.y, 0.22), (p.x + left.x * 0.06, p.y + left.y * 0.06, 0.20), (p.x + left.x * 0.10, p.y + left.y * 0.10, 0.10)],
                                            0.03, 3), 0.011, 10), 'chrome')
    # the waitress side: a work shelf, under-shelves with plate stacks, glass racks, a bus tub
    k.add(bx('work', cx0, cx1 - 0.10, cy0 + 0.50, cy1, 0.84, 0.86, 0.004), 'stainless', STEEL)
    for zz in (0.12, 0.48):
        k.add(bx('shelf', cx0, cx1 - 0.10, cy0 + 0.50, cy1 - 0.02, zz - 0.02, zz, 0.004), 'stainless', STEEL)
    k.add(bx('short_back', sx1 - 0.35, sx1, cy1, SHORT_Y1 - 0.10, 0.12, 0.86, 0.004), 'stainless', STEEL)
    rnd = random.Random(55)
    for x in np.arange(cx0 + 0.25, cx1 - 0.4, 0.55):
        n_ = rnd.randint(8, 18)
        for j in range(n_):
            z = 0.48 + j * 0.012
            k.add(lathe_solid('plate', [(0.0, 0.0), (0.12, 0.0), (0.125, 0.006), (0.13, 0.012), (0.0, 0.010)], Matrix.Translation((x, cy1 - 0.15, z)), 24),
                  'ceramic', 0xf4f1ea)
        if rnd.random() < 0.5:
            k.add(bx('rack', x - 0.22, x + 0.22, cy1 - 0.27, cy1 - 0.03, 0.12, 0.22, 0.01), 'plastic', 0x3a4a5a)
            for gx in np.arange(x - 0.17, x + 0.18, 0.085):
                for gy in (cy1 - 0.21, cy1 - 0.09):
                    k.add(lathe_solid('glass', [(0.0, 0.22), (0.035, 0.22), (0.032, 0.34), (0.0, 0.34)], Matrix.Translation((gx, gy, 0.0)), 16), 'glassdark', 0xb8c8c8)
    k.add(bx('bus_tub', cx1 - 1.20, cx1 - 0.70, cy0 + 0.52, cy1 - 0.04, 0.86, 1.00, 0.02), 'plastic', 0x5a5a5e)
    # on the customer side: condiment sets every couple of stools, a stack of menus at the short leg
    for (x, y) in ((5.97, cy0 + 0.20), (7.47, cy0 + 0.20), (8.97, cy0 + 0.20), (10.47, cy0 + 0.20), (sx0 + 0.20, 5.80)):
        condiments(k, x, y, TOP_Z)
    for i in range(6):
        mm = Matrix.Translation((sx0 + 0.42, SHORT_Y1 - 0.30, TOP_Z + i * 0.004)) @ Matrix.Rotation(math.radians(rnd.uniform(-8, 8)), 4, 'Z')
        o = bx('menu', -0.11, 0.11, -0.15, 0.15, 0.0, 0.0035, 0.001)
        o.data.transform(mm)
        k.add(o, 'paper', 0x8a1c1c)
    k.decal('menu_card', tuple(mm @ Vector((0, 0, 0.0037))), u=tuple(mm.to_3x3() @ Vector((1, 0, 0))), v=tuple(mm.to_3x3() @ Vector((0, 1, 0))),
            w=0.22, h=0.30, lift=0.0002)
    k.polish = [((x, cy0 + 0.15, TOP_Z), 0.35, 0.6) for x, _ in STOOLS[:7]] + [((x, cy0 - 0.10, 0.22), 0.25, 0.8) for x, _ in STOOLS[:7]]
    k.grime = [((x, cy0 + 0.10, 0.4), 0.3, 0.4) for x, _ in STOOLS[:7]]
    return [k.finish(35.0)]


def draw_menu_card(s, w, h):
    s.rect(0, 0, w, h, 0x8a1c1c, rough=0.5)
    s.rect(8, 8, w - 16, h - 16, 0xf2ead6, rough=0.6)
    s.mid("RUBY'S", w / 2, h - 34, 28, w - 40, 0x8a1c1c, 'Black', tracking=1.2)
    s.mid('OPEN 24 HOURS', w / 2, h - 56, 9, w - 40, 0x1a1a1a, 'Bold', tracking=1.4)
    items = [('THE SHORT STACK', '6.99'), ('TWO EGGS, HOME FRIES', '7.49'), ('PATTY MELT & FRIES', '10.99'), ('GRILLED CHEESE & SOUP', '6.49'),
             ('CHICKEN NOODLE SOUP', '5.49'), ('STEAK & EGGS', '14.99'), ('SLICE OF PIE', '4.50'), ('COFFEE', '2.25'), ('ORANGE JUICE', '3.25'),
             ('MILKSHAKE', '5.25')]
    for i, (a, b) in enumerate(items):
        y = h - 90 - i * 18
        s.text(a, 18, y, 8, 0x1a1a1a, 'Bold')
        s.text(b, w - 18, y, 8, 0x1a1a1a, 'Bold', align='RIGHT')


def condiments(k, x, y, z):
    """A napkin dispenser, a sugar pourer, salt and pepper, ketchup and mustard."""
    k.add(bx('napkin', x - 0.07, x + 0.07, y - 0.05, y + 0.05, z, z + 0.15, 0.01, 2), 'chrome')
    k.add(bx('napkins', x - 0.055, x + 0.055, y - 0.052, y - 0.050, z + 0.02, z + 0.13), 'paper', 0xf4f2ec)
    k.add(lathe_solid('sugar', [(0.0, z), (0.035, z), (0.037, z + 0.01), (0.037, z + 0.11), (0.0, z + 0.11)], Matrix.Translation((x + 0.13, y, 0.0)), 24),
          'glassdark', 0xd8dcdc)
    k.add(lathe_solid('sugar_top', [(0.0, z + 0.11), (0.038, z + 0.11), (0.036, z + 0.13), (0.012, z + 0.15), (0.006, z + 0.17), (0.0, z + 0.17)],
                      Matrix.Translation((x + 0.13, y, 0.0)), 24), 'chrome')
    for dx, col in ((0.20, 0xf4f2ec), (0.245, 0x3a3634)):
        k.add(lathe_solid('shaker', [(0.0, z), (0.017, z), (0.019, z + 0.06), (0.0, z + 0.06)], Matrix.Translation((x + dx, y - 0.01, 0.0)), 16), 'glassdark', col)
        k.add(lathe_solid('shaker_top', [(0.0, z + 0.06), (0.019, z + 0.06), (0.016, z + 0.08), (0.0, z + 0.085)], Matrix.Translation((x + dx, y - 0.01, 0.0)), 16),
              'chrome')
    for dx, col in ((-0.13, 0xc4231c), (-0.19, 0xe8b820)):
        k.add(lathe_solid('squeeze', [(0.0, z), (0.026, z), (0.028, z + 0.02), (0.028, z + 0.15), (0.014, z + 0.17), (0.005, z + 0.20), (0.0, z + 0.205)],
                          Matrix.Translation((x + dx, y, 0.0)), 20), 'plastic', col)


def stool():
    """SM_NS_Ruby_Stool (PROP): a chrome swivel stool bolted to the floor; the seat's top at 0.75."""
    k = kit('nsdt', 'SM_NS_Ruby_Stool', ('body',), anchor=(0.0, 0.0, 0.0), dust=0.05, low=0.1)
    TEXTURE_SIZES.update({'nsdt_body': 1024})
    k.add(lathe_solid('flange', [(0.0, 0.0), (0.16, 0.0), (0.162, 0.006), (0.15, 0.015), (0.08, 0.045), (0.045, 0.06), (0.0, 0.06)], None, 48), 'chrome')
    for a in range(4):
        aa = a * math.pi / 2 + math.pi / 4
        k.add(lathe_solid('bolt', [(0.0, 0.0), (0.012, 0.0), (0.012, 0.012), (0.008, 0.016), (0.0, 0.016)],
                          Matrix.Translation((0.125 * math.cos(aa), 0.125 * math.sin(aa), 0.008)), 6), 'chrome')
    k.add(lathe_solid('column', [(0.0, 0.05), (0.032, 0.05), (0.032, 0.62), (0.0, 0.62)], None, 32), 'chrome')
    k.add(lathe_solid('swivel', [(0.0, 0.60), (0.04, 0.60), (0.06, 0.63), (0.07, 0.66), (0.0, 0.66)], None, 32), 'chrome')
    k.add(lathe_solid('skirt', [(0.0, 0.655), (0.185, 0.655), (0.19, 0.665), (0.19, 0.71), (0.0, 0.71)], None, 64), 'chrome')
    seat = lathe_solid('seat', [(0.0, 0.705), (0.188, 0.705), (0.192, 0.715), (0.19, 0.73), (0.178, 0.743), (0.14, 0.75), (0.06, 0.752), (0.02, 0.746),
                                (0.0, 0.744)], None, 64)
    core.subdivide(seat, 1)
    k.add(seat, 'vinyl', 0x7a1418)
    k.add(lathe_solid('piping', [(0.0, 0.725), (0.193, 0.725), (0.197, 0.73), (0.193, 0.735), (0.0, 0.735)], None, 64), 'vinyl', 0x5a1012)
    k.add(lathe_solid('button', [(0.0, 0.742), (0.012, 0.742), (0.01, 0.748), (0.0, 0.749)], None, 16), 'vinyl', 0x5a1012)
    k.polish = [((0.0, 0.0, 0.75), 0.18, 0.9), ((0.0, 0.0, 0.03), 0.2, 0.5)]
    return [k.finish(35.0)]


def channel_back(name, half, M, n=6, depth=0.012):
    """A channel-tufted back pad (vertical channels): a rounded box whose +x face is creased every 1/n of its width."""
    hx, hy, hz = half
    obj = bx(name, -hx, hx, -hy, hy, -hz, hz, min(half) * 0.5, 1)
    core.subdivide(obj, 3)
    for v in obj.data.vertices:
        p = v.co
        if p.x > 0:
            t = (p.y + hy) / (2 * hy) * n
            kk = max(0.0, 1.0 - (p.y / hy) ** 6) * max(0.0, 1.0 - (p.z / hz) ** 6)
            p.x += (0.01 - depth * (1.0 - abs(math.sin(math.pi * t))) ** 8) * kk * (p.x / hx)
    obj.data.transform(M)
    obj.data.shade_smooth()
    return obj


def booth():
    """SM_NS_Ruby_Booth (PROP): two benches facing each other across a table; the aisle toward -X, the window +X."""
    k = kit('nsdb', 'SM_NS_Ruby_Booth', ('body', 'print'), anchor=(0.0, 0.0, 0.0), dust=0.08, low=0.12, print_px=512)
    TEXTURE_SIZES.update({'nsdb_body': 2048})
    k.panel('menu_card', 0.22, 0.30, draw_menu_card)
    k.panel('wallbox', 0.20, 0.06, draw_juke_titles, density=1.5)
    k.atlas.render()
    L0, L1 = -0.60, 0.60
    for s in (-1, 1):
        # the bench: a laminate box, stainless kick, the seat cushion with piping, a channel-tufted back, the stainless cap
        def yr(a, b):
            return (min(s * a, s * b), max(s * a, s * b))
        y_in = s * 0.375
        k.add(bx('bench', L0, L1, *yr(0.415, 0.95), 0.10, 0.36, 0.01), 'enamel', WAIN)
        k.add(bx('bench_kick', L0 + 0.01, L1, *yr(0.435, 0.95), 0.0, 0.10, 0.004), 'stainless', STEEL)
        seat = cushion('seat', L0 + 0.02, L1 - 0.01, min(y_in, s * 0.83), max(y_in, s * 0.83), 0.36, 0.455, 0.05, crown=0.012, sub=2)
        k.add(seat, 'vinyl', 0x7a1418)
        rr = core.rounded_rect(L0 + 0.03, min(y_in, s * 0.83) + 0.01, L1 - 0.02, max(y_in, s * 0.83) - 0.01, 0.04, 4)
        k.add(tube('piping', [(x, y, 0.445) for x, y in rr], 0.005, 6, closed=True), 'vinyl', 0x5a1012)
        tilt = Matrix.Rotation(math.radians(-s * 9.0), 4, 'X')
        # the back pad leans back 9 degrees; its top stays inside the back frame (inner face at 0.90)
        BM = Matrix.Translation((0.0, s * 0.80, 0.80)) @ tilt @ Matrix.Rotation(math.radians(90 if s < 0 else -90), 4, 'Z')
        back = channel_back('back', (0.05, 0.58, 0.34), BM, 6)
        k.add(back, 'vinyl', 0x7a1418)
        k.add(bx('back_frame', L0, L1, *yr(0.90, 0.95), 0.36, 1.18, 0.008), 'enamel', WAIN)
        k.add(bx('back_cap', L0 - 0.01, L1, *yr(0.86, 0.97), 1.17, 1.20, 0.008), 'stainless', STEEL)
        k.add(bx('end_panel', L0 - 0.012, L0, *yr(0.415, 0.95), 0.0, 1.18, 0.004), 'enamel', WAIN)
        y_a, y_b = yr(0.415, 0.95)
        k.add(bx('end_edge', L0 - 0.016, L0 + 0.004, y_a, y_b, 1.165, 1.19, 0.004), 'chrome')
        y_front = y_a if s > 0 else y_b
        k.add(bx('end_edge_f', L0 - 0.016, L0 + 0.004, y_front - 0.006, y_front + 0.006, 0.0, 1.19, 0.003), 'chrome')
        k.add(bx('end_kick', L0 - 0.016, L0 + 0.004, y_a, y_b, 0.0, 0.10, 0.003), 'stainless', STEEL)
        k.polish.append(((-0.05, s * 0.60, 0.46), 0.3, 0.9))
        k.polish.append(((-0.05, s * 0.86, 0.85), 0.25, 0.6))
    # the table: a laminate top with a ribbed chrome edge on a pedestal and a cross foot
    tl = core.rounded_rect(-0.55, -0.36, 0.55, 0.36, 0.04, 4)
    k.add(parts.panel('ttop', tl, 0.735, 0.76), 'formica', 0xd9cfbd)
    k.add(molding('tedge', list(tl) + [tl[0]], [(-0.012, 0.725), (0.0, 0.725), (0.0, 0.764), (-0.012, 0.764)]), 'chrome')
    k.add(lathe_solid('tcol', [(0.0, 0.03), (0.04, 0.03), (0.04, 0.70), (0.07, 0.72), (0.07, 0.735), (0.0, 0.735)], None, 32), 'chrome')
    for a in (0.0, math.pi / 2, math.pi, 1.5 * math.pi):
        k.add(tube('tfoot', [(0.0, 0.0, 0.03), (0.30 * math.cos(a), 0.22 * math.sin(a), 0.015)], 0.025, 12), 'chrome')
        k.add(lathe_solid('tglide', [(0.0, 0.0), (0.03, 0.0), (0.03, 0.012), (0.0, 0.012)], Matrix.Translation((0.30 * math.cos(a), 0.22 * math.sin(a), 0.0)), 16),
              'rubber', 0x111111)
    # on the table: the condiments against the window end, a menu card in a stand, the jukebox wallbox
    condiments(k, 0.30, -0.10, 0.76)
    wb = Matrix.Translation((0.43, 0.12, 0.76)) @ Matrix.Rotation(math.radians(90), 4, 'Z')
    o = bx('wallbox', -0.12, 0.12, -0.06, 0.06, 0.0, 0.16, 0.025, 2)
    o.data.transform(wb)
    k.add(o, 'chrome')
    k.decal('wallbox', tuple(wb @ Vector((0.0, -0.0605, 0.10))), u=tuple(wb.to_3x3() @ Vector((1, 0, 0))), v=tuple(wb.to_3x3() @ Vector((0, 0, 1))),
            w=0.20, h=0.06, lift=0.0004)
    mm = Matrix.Translation((0.35, 0.25, 0.76)) @ Matrix.Rotation(math.radians(90), 4, 'Z') @ Matrix.Rotation(math.radians(-75), 4, 'X')
    k.decal('menu_card', tuple(mm @ Vector((0.0, 0.0, 0.15))), u=tuple(mm.to_3x3() @ Vector((1, 0, 0))), v=tuple(mm.to_3x3() @ Vector((0, 0, 1))),
            w=0.16, h=0.22, lift=0.0)
    mb = bx('menu_back', -0.08, 0.08, 0.0005, 0.0025, 0.04, 0.26)
    mb.data.transform(mm)
    k.add(mb, 'paper', 0x8a1c1c)
    k.grime = [((-0.5, 0.0, 0.76), 0.2, 0.4)]
    return [k.finish(35.0)]


# ================================================================== the backbar, the pie case and its carousel

PIE_C = (5.83, 7.15)
PIE_Z = 0.98                      # the case's floor (the carousel's origin)


@finish_kind('quarry')
def f_quarry(c):
    """Red quarry tile, 15 cm squares, dark grout, greasy and scuffed (a kitchen floor); seamless over 1.2 m."""
    m = c.m
    T = 0.15
    fu = m.math('FRACT', m.math('DIVIDE', c.x, T))
    fv = m.math('FRACT', m.math('DIVIDE', c.y, T))
    e = mul(m, m.math('MINIMUM', m.math('MINIMUM', fu, sub(m, 1.0, fu)), m.math('MINIMUM', fv, sub(m, 1.0, fv))), T)
    grout = smooth_less(m, e, 0.003, 0.0006)
    tid = vec(m, m.math('FLOORED_MODULO', m.math('FLOOR', m.math('DIVIDE', c.x, T)), 8.0), m.math('FLOORED_MODULO', m.math('FLOOR', m.math('DIVIDE', c.y, T)), 8.0), 0.0)
    rnd = K.cell_rand(m, tid, (1.0, 1.0, 1.0), 4.0)
    col = m.mix(rnd, H(0x8a3a28), H(0x9e4a32))
    grease = ramp(m, noise(m, vec(m, add(m, mul(m, fu, 0.15), mul(m, rnd, 3.0)), mul(m, fv, 0.15), 0.0), scale=12.0, detail=4.0), 0.5, 0.8)
    col = m.mix(mul(m, grease, 0.45), col, H(0x4a2418))
    col = m.mix(grout, col, H(0x2a2420))
    rough = add(m, 0.55, mul(m, grease, -0.3), mul(m, rnd, 0.08))
    rough = mix_float(m, grout, rough, 0.9)
    return col, rough, 0.0, add(m, K.bevel_h(m, e, 0.004, 0.8), mul(m, grout, -1.5))


@finish_kind('sqtile')
def f_sqtile(c):
    """White glazed 10 cm square wall tile (a kitchen), its grout yellowed; seamless over 1.2 m (the tiled bake)."""
    m = c.m
    T = 0.10
    along = add(m, c.x, c.y)
    fu = m.math('FRACT', m.math('DIVIDE', along, T))
    fv = m.math('FRACT', m.math('DIVIDE', c.z, T))
    e = mul(m, m.math('MINIMUM', m.math('MINIMUM', fu, sub(m, 1.0, fu)), m.math('MINIMUM', fv, sub(m, 1.0, fv))), T)
    grout = smooth_less(m, e, 0.0012, 0.0003)
    tid = vec(m, m.math('FLOORED_MODULO', m.math('FLOOR', m.math('DIVIDE', along, T)), 12.0), m.math('FLOOR', m.math('DIVIDE', c.z, T)), 0.0)
    rnd = K.cell_rand(m, tid, (1.0, 1.0, 1.0), 6.0)
    col = m.mix(rnd, H(0xeeeee8), H(0xe2e0d6))
    splat = mul(m, ramp(m, noise(m, vec(m, add(m, mul(m, fu, T), mul(m, rnd, 2.0)), mul(m, fv, T), 0.0), scale=40.0, detail=3.0), 0.62, 0.75),
                ramp(m, c.z, 1.5, 0.8))
    col = m.mix(mul(m, splat, 0.4), col, H(0xc8b890))
    col = m.mix(grout, col, H(0xa89e88))
    rough = mix_float(m, grout, add(m, 0.1, mul(m, splat, 0.3)), 0.85)
    return col, rough, 0.0, add(m, K.bevel_h(m, e, 0.003, 0.6), mul(m, grout, -0.8))


def mug(k, x, y, z, inverted=False, col=0xf4f1ea):
    """A heavy white diner mug (upside down on a tray with inverted=True)."""
    prof = [(0.0, 0.0), (0.038, 0.0), (0.042, 0.006), (0.044, 0.095), (0.04, 0.098), (0.036, 0.012), (0.0, 0.012)]
    M = Matrix.Translation((x, y, z))
    if inverted:
        M = Matrix.Translation((x, y, z + 0.098)) @ Matrix.Rotation(math.pi, 4, 'X')
    k.add(lathe_solid('mug', prof, M, 24), 'ceramic', col)
    k.add(tube('mug_handle', [tuple(M @ Vector((0.042, 0.0, 0.025))), tuple(M @ Vector((0.07, 0.0, 0.05))), tuple(M @ Vector((0.042, 0.0, 0.08)))], 0.007, 8),
          'ceramic', col)


def carafe(k, x, y, z, collar=0x111111, full=True):
    M = Matrix.Translation((x, y, z))
    k.add(lathe_solid('carafe', [(0.0, 0.0), (0.075, 0.0), (0.085, 0.02), (0.085, 0.07), (0.06, 0.14), (0.045, 0.17), (0.0, 0.17)], M, 32), 'glassdark',
          0x2a1a10 if full else 0xb8c4c4)
    k.add(lathe_solid('carafe_collar', [(0.0, 0.15), (0.05, 0.15), (0.058, 0.17), (0.058, 0.21), (0.03, 0.22), (0.0, 0.22)], M, 24), 'plastic', collar)
    k.add(tube('carafe_handle', [tuple(M @ Vector((0.055, 0.0, 0.20))), tuple(M @ Vector((0.11, 0.0, 0.17))), tuple(M @ Vector((0.11, 0.0, 0.08))),
                                 tuple(M @ Vector((0.08, 0.0, 0.05)))], 0.009, 8), 'plastic', collar)


def pie_wedges(k, cx, cy, z, r, kind, missing=0, rot=0.0, plate=True):
    """A pie as eight wedges (some gone), each layered so a cut face shows its crust and filling."""
    fill = {'cherry': (0x9a0c16, 'syrup', 0.028), 'lemon': (0xf0c020, 'syrup', 0.03), 'choc': (0x3a1c10, 'cream', 0.032), 'apple': (0xc88a3a, 'syrup', 0.026),
            'coconut': (0xeee8d8, 'cream', 0.03)}[kind]
    if plate:
        k.add(lathe_solid('pie_tin', [(0.0, z), (r * 0.82, z), (r + 0.012, z + 0.03), (r + 0.016, z + 0.032), (0.0, z + 0.006)], Matrix.Translation((cx, cy, 0.0)), 40),
              'chrome')
    n = 8
    for i in range(missing, n):
        a0 = rot + 2 * math.pi * i / n + 0.004
        a1 = rot + 2 * math.pi * (i + 1) / n - 0.004
        arc = [(cx + r * math.cos(a0 + (a1 - a0) * j / 6), cy + r * math.sin(a0 + (a1 - a0) * j / 6)) for j in range(7)]
        outline = [(cx, cy)] + arc
        k.add(parts.panel('crust_b', outline, z + 0.004, z + 0.012), 'crust', 0xc89a5a)
        k.add(parts.panel('fill', outline, z + 0.012, z + fill[2]), fill[1], fill[0])
        if kind in ('cherry', 'apple'):
            am = (a0 + a1) / 2
            for q in (-1, 0, 1):
                off = q * 0.022
                pa = (cx + off * -math.sin(am) + 0.012 * math.cos(am), cy + off * math.cos(am) + 0.012 * math.sin(am))
                pb = (cx + off * -math.sin(am) + (r - 0.012) * math.cos(am), cy + off * math.cos(am) + (r - 0.012) * math.sin(am))
                k.add(tube('lattice', [(pa[0], pa[1], z + fill[2] + 0.002), (pb[0], pb[1], z + fill[2] + 0.002)], 0.0045, 6), 'crust', 0xd8a050)
        elif kind == 'lemon':
            mer = parts.panel('meringue', outline, z + fill[2], z + fill[2] + 0.018)
            for v in mer.data.vertices:
                if v.co.z > z + fill[2] + 0.01:
                    v.co.z += 0.008 * math.sin(v.co.x * 260.0) * math.sin(v.co.y * 230.0)
            k.add(mer, 'crust', 0xf0dcb0)
        else:
            k.add(parts.panel('choc_top', outline, z + fill[2], z + fill[2] + 0.003), 'cream', fill[0] if kind == 'choc' else 0xf6f2ea)
            am = (a0 + a1) / 2
            k.add(lathe_solid('rosette', [(0.0, 0.0), (0.009, 0.0), (0.008, 0.006), (0.004, 0.012), (0.0, 0.014)],
                              Matrix.Translation((cx + (r - 0.018) * math.cos(am), cy + (r - 0.018) * math.sin(am), z + fill[2] + 0.003)), 12), 'cream',
                  0xf6f2ea)
        rim = [(cx + (r - 0.004) * math.cos(a0 + (a1 - a0) * j / 6), cy + (r - 0.004) * math.sin(a0 + (a1 - a0) * j / 6), z + fill[2] + 0.006) for j in range(7)]
        k.add(tube('rim', rim, 0.011, 8), 'crust', 0xc8904a)


def draw_disp(s, w, h):
    s.rect(0, 0, w, h, 0x0a1a14, rough=0.1)
    s.mid('TOTAL  12.84', w / 2, h / 2, h * 0.5, w * 0.9, 0x6effc8, 'Bold')


def backbar():
    k = kit('nsdk', 'SM_NS_Ruby_Backbar', ('body', 'print', 'ns_glass'), dust=0.08, low=0.12, print_px=1024)
    TEXTURE_SIZES.update({'nsdk_body': 4096})
    k.panel('tips', 0.10, 0.06, draw_sign('TIPS', 0xf2efe6, 0x1a1a1a, 'THANK YOU'))
    cereals = [('OAT RINGS', 0xe8b820, 0x2a4a8a), ('SUGAR STARS', 0x2a6ad8, 0xf2f2f2), ('CORN FLAKES', 0xf2f2e8, 0xc4231c), ('RAISIN CRUNCH', 0x6a2a6a, 0xf2c230),
               ('COCOA BITES', 0x5a3018, 0xf2e6c8), ('BERRY O\'S', 0xe83a6a, 0xf2f2f2)]
    for i, (nm, g, ink) in enumerate(cereals):
        k.panel(f'cereal{i}', 0.07, 0.10, K.p_label([], g, ink, nm, ink, g))
    k.panel('reg_disp', 0.10, 0.03, draw_disp, glow=2.0, density=2.0)
    k.panel('brewer', 0.30, 0.05, draw_sign('CAFE MASTER', 0x1a1a1a, 0xd8d8d8))
    k.atlas.render()
    x0, x1 = 4.40, 11.60
    y0, y1 = BACKBAR_Y
    S = lambda o, aux=0: k.add(o, 'stainless', STEEL, aux=aux)
    k.add(bx('kick', x0, x1, y0 + 0.08, y1, 0.0, 0.10), 'rubber', 0x141414)
    S(bx('carcass', x0, x1, y0 + 0.02, y1, 0.10, BB_Z - 0.03, 0.003), 2)
    for xa in np.arange(x0 + 0.02, x1 - 0.5, 0.60):
        S(bx('door', xa + 0.01, xa + 0.58, y0, y0 + 0.02, 0.13, BB_Z - 0.06, 0.004), 2)
        k.add(bx('pull', xa + 0.20, xa + 0.39, y0 - 0.025, y0, BB_Z - 0.12, BB_Z - 0.10, 0.006), 'chrome')
    S(bx('top', x0 - 0.01, x1 + 0.01, y0 - 0.03, y1, BB_Z - 0.03, BB_Z, 0.004))
    S(bx('lip', x0 - 0.01, x1 + 0.01, y0 - 0.035, y0 - 0.02, BB_Z, BB_Z + 0.012, 0.003))
    S(bx('splash', x0, x1, y1 - 0.012, y1, BB_Z, PASS_Z[0]), 2)
    for (sx0, sx1) in ((x0 + 0.05, PASS_X[0] - 0.05), (PASS_X[1] + 0.05, x1 - 0.05)):
        for zz in (1.55, 1.95):
            S(bx('shelf', sx0, sx1, y1 - 0.30, y1 - 0.005, zz - 0.02, zz, 0.004))
            for bxp in (sx0 + 0.1, sx1 - 0.1):
                k.add(bx('bracket', bxp - 0.008, bxp + 0.008, y1 - 0.25, y1 - 0.005, zz - 0.18, zz - 0.02), 'stainless', STEEL)
    # the register on the west end, the tip jar
    rx = 4.72
    k.add(bx('reg_base', rx - 0.20, rx + 0.20, 6.95, 7.40, BB_Z, BB_Z + 0.10, 0.008, 3), 'plastic', 0x2d2f33)
    k.add(core.prism_x('reg_body', [(6.95, BB_Z + 0.10), (7.40, BB_Z + 0.10), (7.40, BB_Z + 0.21), (7.10, BB_Z + 0.19), (6.95, BB_Z + 0.13)], rx - 0.19, rx + 0.19),
          'plastic', 0x34363a)
    for i in range(5):
        for j in range(4):
            k.add(bx('key', rx - 0.15 + i * 0.06, rx - 0.11 + i * 0.06, 7.0 + j * 0.035, 7.03 + j * 0.035, BB_Z + 0.13 + j * 0.012, BB_Z + 0.145 + j * 0.012, 0.004),
                  'plastic', 0xe8e6de if i < 3 else 0x8a8c90)
    k.add(parts.rod('reg_pole', (rx + 0.12, 7.30, BB_Z + 0.20), (rx + 0.12, 7.30, BB_Z + 0.32), 0.012, 12), 'plastic', 0x2b2d31)
    k.add(bx('reg_head', rx + 0.05, rx + 0.19, 7.27, 7.33, BB_Z + 0.31, BB_Z + 0.37, 0.008), 'plastic', 0x2b2d31)
    k.decal('reg_disp', (rx + 0.12, 7.2695, BB_Z + 0.34), w=0.10, h=0.03, lift=0.0003)
    k.add(lathe_solid('tip_jar', [(0.0, BB_Z), (0.05, BB_Z), (0.052, BB_Z + 0.01), (0.052, BB_Z + 0.14), (0.045, BB_Z + 0.15), (0.0, BB_Z + 0.15)],
                      Matrix.Translation((rx + 0.30, 6.98, 0.0)), 24), 'glassdark', 0xc8d4d4)
    K.wrap_cyl(k, 'tips', rx + 0.30, 6.98, BB_Z + 0.05, BB_Z + 0.11, 0.052, -0.9, 0.9)
    # the pie case: drum, glass, the domed top
    pc = PIE_C
    M = Matrix.Translation((pc[0], pc[1], 0.0))
    k.add(lathe_solid('pie_base', [(0.0, BB_Z), (0.30, BB_Z), (0.31, BB_Z + 0.01), (0.31, PIE_Z - 0.01), (0.295, PIE_Z), (0.0, PIE_Z)], M, 64), 'chrome')
    g = core.lathe('pie_glass', [(0.29, PIE_Z), (0.29, 1.62)], segments=64)
    core.orient_normals(g)
    g.data.transform(M)
    k.special(g, 'ns_glass')
    k.add(lathe_solid('pie_top', [(0.0, 1.61), (0.31, 1.61), (0.315, 1.63), (0.29, 1.68), (0.20, 1.715), (0.06, 1.73), (0.03, 1.76), (0.0, 1.77)], M, 64), 'chrome')
    for a in (0.0, 2.09, 4.19):
        p = (pc[0] + 0.293 * math.cos(a), pc[1] + 0.293 * math.sin(a))
        k.add(parts.rod('pie_post', (p[0], p[1], PIE_Z), (p[0], p[1], 1.61), 0.007, 8), 'chrome')
    # coffee: the brewer, the warmer stand, carafes; mugs and saucers; the milkshake mixer; a toaster
    bx0 = 9.35
    S(bx('brewer', bx0, bx0 + 0.48, 6.95, 7.45, BB_Z, BB_Z + 0.62, 0.012), 2)
    k.add(bx('brew_funnel', bx0 + 0.06, bx0 + 0.42, 6.92, 7.20, BB_Z + 0.36, BB_Z + 0.46, 0.02), 'plastic', 0x1a1a1a)
    k.add(bx('brew_handle', bx0 + 0.20, bx0 + 0.28, 6.86, 6.93, BB_Z + 0.40, BB_Z + 0.43, 0.01), 'plastic', 0x1a1a1a)
    k.decal('brewer', (bx0 + 0.24, 6.9495, BB_Z + 0.55), w=0.30, h=0.05, lift=0.0003)
    warm_rings = []
    for (wx, wz) in ((bx0 + 0.24, BB_Z + 0.015), (bx0 + 0.12, BB_Z + 0.62), (bx0 + 0.36, BB_Z + 0.62)):
        k.add(lathe_solid('warmer', [(0.0, wz - 0.012), (0.085, wz - 0.012), (0.085, wz), (0.0, wz)], Matrix.Translation((wx, 7.10, 0.0)), 32), 'iron')
        warm_rings.append(parts.annulus('ring', 0.05, 0.075, wz + 0.0005, wz + 0.002, 32, Matrix.Translation((wx, 7.10, 0.0))))
    S(bx('warm_stand', bx0 + 0.55, bx0 + 1.15, 6.95, 7.40, BB_Z, BB_Z + 0.08, 0.008))
    for wx in (bx0 + 0.70, bx0 + 1.00):
        k.add(lathe_solid('warmer', [(0.0, BB_Z + 0.068), (0.085, BB_Z + 0.068), (0.085, BB_Z + 0.08), (0.0, BB_Z + 0.08)], Matrix.Translation((wx, 7.10, 0.0)), 32), 'iron')
        warm_rings.append(parts.annulus('ring', 0.05, 0.075, BB_Z + 0.0805, BB_Z + 0.082, 32, Matrix.Translation((wx, 7.10, 0.0))))
    carafe(k, bx0 + 0.24, 7.10, BB_Z + 0.015)
    carafe(k, bx0 + 0.12, 7.10, BB_Z + 0.62, full=False)
    carafe(k, bx0 + 0.70, 7.10, BB_Z + 0.08, 0xe8681c)
    carafe(k, bx0 + 1.00, 7.10, BB_Z + 0.08)
    for i in range(3):
        for j in range(4):
            mug(k, 10.62 + j * 0.095, 7.05 + i * 0.12, BB_Z, inverted=True)
    for sx_ in (10.66, 10.86):
        for i in range(10):
            k.add(lathe_solid('saucer', [(0.0, 0.0), (0.07, 0.0), (0.075, 0.008), (0.078, 0.014), (0.0, 0.010)], Matrix.Translation((sx_, 7.38, 1.55 + i * 0.011)), 24),
                  'ceramic', 0xf4f1ea)
    mx = 11.25
    k.add(bx('mixer_base', mx - 0.14, mx + 0.14, 7.12, 7.42, BB_Z, BB_Z + 0.05, 0.015), 'enamel', 0x7fb8a0)
    k.add(bx('mixer_col', mx - 0.05, mx + 0.05, 7.32, 7.42, BB_Z + 0.05, BB_Z + 0.48, 0.02), 'enamel', 0x7fb8a0)
    k.add(bx('mixer_head', mx - 0.15, mx + 0.15, 7.18, 7.42, BB_Z + 0.42, BB_Z + 0.52, 0.03), 'enamel', 0x7fb8a0)
    for i in range(3):
        sx_ = mx - 0.10 + i * 0.10
        k.add(parts.rod('spindle', (sx_, 7.24, BB_Z + 0.42), (sx_, 7.24, BB_Z + 0.22), 0.004, 8), 'chrome')
    k.add(lathe_solid('shake_cup', [(0.0, BB_Z + 0.06), (0.04, BB_Z + 0.06), (0.05, BB_Z + 0.26), (0.0, BB_Z + 0.26)], Matrix.Translation((mx, 7.24, 0.0)), 24),
          'stainless', STEEL)
    k.add(bx('toaster', 6.28, 6.68, 7.05, 7.40, BB_Z, BB_Z + 0.22, 0.03, 3), 'chrome')
    for i in range(4):
        k.add(bx('slot', 6.33 + i * 0.085, 6.37 + i * 0.085, 7.10, 7.35, BB_Z + 0.215, BB_Z + 0.222), 'iron')
    # plates stacked under the pass; glasses, syrups and the cereal boxes on the shelves
    for (px_, n_) in ((7.05, 16), (7.40, 12), (7.75, 18), (8.20, 9)):
        for i in range(n_):
            k.add(lathe_solid('plate', [(0.0, 0.0), (0.13, 0.0), (0.135, 0.006), (0.14, 0.013), (0.0, 0.010)], Matrix.Translation((px_, 7.20, BB_Z + i * 0.013)), 32),
                  'ceramic', 0xf4f1ea)
    for i in range(12):
        gx = 4.55 + i * 0.12
        if gx > PASS_X[0] - 0.12:
            break
        k.add(lathe_solid('tumbler', [(0.0, 1.55), (0.033, 1.55), (0.04, 1.68), (0.0, 1.68)], Matrix.Translation((gx, 7.36, 0.0)), 20), 'glassdark', 0xb8c8c8)
    for i in range(4):
        sx_ = 9.45 + i * 0.16
        k.add(lathe_solid('syrup', [(0.0, 1.55), (0.04, 1.55), (0.04, 1.68), (0.03, 1.71), (0.0, 1.71)], Matrix.Translation((sx_, 7.36, 0.0)), 24), 'glassdark', 0x5a2a0a)
        k.add(lathe_solid('syrup_top', [(0.0, 1.71), (0.032, 1.71), (0.03, 1.74), (0.006, 1.77), (0.0, 1.77)], Matrix.Translation((sx_, 7.36, 0.0)), 24), 'chrome')
    for i in range(10):
        cx_ = 9.45 + i * 0.09
        k.add(bx('cereal', cx_ - 0.035, cx_ + 0.035, 7.30, 7.45, 1.95, 2.05, 0.002), 'paper', 0xe8e4da)
        k.decal(f'cereal{i % 6}', (cx_, 7.2995, 2.0), w=0.07, h=0.10, lift=0.0003)
    k.polish = [((rx, 6.95, BB_Z + 0.15), 0.2, 0.6), ((bx0 + 0.24, 6.9, BB_Z + 0.4), 0.2, 0.5)]
    k.grime = [((bx0 + 0.6, 6.9, BB_Z), 0.6, 0.6)]
    obj = k.finish(35.0)
    lamp = parts.annulus('pie_lamp', 0.18, 0.22, 1.585, 1.605, 48, Matrix.Translation((pc[0], pc[1], 0.0)))
    gl = glow(warm_rings + [lamp], 'SM_NS_Ruby_Backbar_GlowWarm', ANCHOR, LOG)
    return [obj, gl, carousel()]


def carousel():
    """SM_NS_Ruby_PieCarousel (PROP): the pie case's turning shelves; origin at the axis on the case's floor."""
    k = kit('nsdp', 'SM_NS_Ruby_PieCarousel', ('body',), anchor=(0.0, 0.0, 0.0), dust=0.0, low=0.0)
    TEXTURE_SIZES.update({'nsdp_body': 1024})
    k.add(parts.rod('post', (0.0, 0.0, 0.0), (0.0, 0.0, 0.60), 0.012, 16), 'chrome')
    pies = [[('cherry', 2, 0.3), ('lemon', 0, 2.0)], [('choc', 3, 1.0), ('apple', 0, 4.0)], [('coconut', 1, 0.5), ('cherry', 5, 3.0)]]
    for i, zz in enumerate((0.03, 0.23, 0.43)):
        k.add(lathe_solid('shelf', [(0.0, zz - 0.006), (0.27, zz - 0.006), (0.27, zz), (0.0, zz)], None, 64), 'glassdark', 0xc8d8d8)
        k.add(tube('shelf_rim', [(0.272 * math.cos(a), 0.272 * math.sin(a), zz - 0.003) for a in np.linspace(0, 2 * math.pi, 48, endpoint=False)], 0.004, 6,
                   closed=True), 'chrome')
        for (kind, missing, ang), a in zip(pies[i], (0.0, math.pi)):
            pie_wedges(k, 0.14 * math.cos(a), 0.14 * math.sin(a), zz, 0.105, kind, missing, ang)
    return k.finish(35.0)


# ================================================================== the pass and the kitchen

KY0, KY1, KX0, KX1, KZ = PASS_Y + 0.15, 10.10, 0.35, 13.0, 3.00
COOK = (6.75, 8.95)


def tickets_draw(seed, lines):
    def draw(s, w, h):
        s.rect(0, 0, w, h, 0xf6f4ee, rough=0.8)
        s.mid("RUBY'S", w / 2, h - 7, 5, w - 8, 0x1a1a1a, 'Black')
        s.mid(f'TBL {seed}   #{400 + seed * 7}', w / 2, h - 15, 3.6, w - 8, 0x1a1a1a, 'Bold')
        for i, ln in enumerate(lines):
            s.text(ln, 5, h - 27 - i * 7, 4.2, 0x1a1a1a, 'Bold')
        s.rect(4, 8, w - 8, 0.5, 0x1a1a1a)
    return draw


def steam_image():
    W, Hh = 256, 256
    u = (np.arange(W) + 0.5) / W
    v = (np.arange(Hh) + 0.5) / Hh
    U, V = np.meshgrid(u, v)
    P = np.stack([U * 4.0, V * 3.0, np.zeros_like(U)], -1).reshape(-1, 3)
    n = K.texmaps.value_noise(P, 1.0, seed=5, octaves=4).reshape(Hh, W)
    a = np.clip((n - 0.45) * 2.5, 0, 1) * np.clip(np.sin(U * math.pi), 0, 1) ** 1.5 * np.clip(1.0 - V, 0, 1) * np.clip(V * 4, 0, 1) * 0.6
    img = np.ones((Hh, W, 4), np.float32)
    img[:, :, 3] = a
    path = os.path.join(core.BUILD_DIR, 'textures', 'ns_steam.png')
    os.makedirs(os.path.dirname(path), exist_ok=True)
    im = bpy.data.images.new('ns_steam_img', W, Hh, alpha=True)
    im.pixels.foreach_set(img.ravel())
    im.filepath_raw = path
    im.file_format = 'PNG'
    im.save()
    bpy.data.images.remove(im)
    return path


def make_steam(n):
    """ns_steam: the steam card's placeholder (M_NS_Steam's flipbook replaces it): white, alpha from a wisp texture."""
    m = core.Mat(n)
    img = m.image(steam_image())
    m.set('Base Color', (1.0, 1.0, 1.0, 1.0))
    m.link(img.outputs['Alpha'], m.bsdf.inputs['Alpha'])
    m.set('Roughness', 0.9)
    m.m.use_backface_culling = False
    try:
        m.m.surface_render_method = 'BLENDED'
    except (AttributeError, TypeError):
        pass
    return m.m


def plate_food(k, x, y, z, kind):
    """A plate up on the pass: the short stack (butter on top) or a patty melt with fries."""
    k.add(lathe_solid('plate', [(0.0, z), (0.10, z), (0.13, z + 0.012), (0.135, z + 0.018), (0.0, z + 0.008)], Matrix.Translation((x, y, 0.0)), 32), 'ceramic', 0xf4f1ea)
    if kind == 'stack':
        for i in range(3):
            k.add(lathe_solid('pancake', [(0.0, 0.0), (0.075, 0.0), (0.082, 0.006), (0.08, 0.014), (0.07, 0.017), (0.0, 0.018)],
                              Matrix.Translation((x + 0.004 * i, y - 0.003 * i, z + 0.009 + i * 0.017)), 32), 'crust', 0xb8742c)
        k.add(bx('butter', x - 0.018, x + 0.018, y - 0.015, y + 0.015, z + 0.06, z + 0.075, 0.005), 'cream', 0xf2dc78)
    else:
        k.add(bx('melt', x - 0.07, x + 0.03, y - 0.05, y + 0.05, z + 0.01, z + 0.05, 0.012), 'crust', 0xb06a2a)
        rnd = random.Random(int(x * 100))
        for i in range(14):
            o = bx('fry', -0.04, 0.04, -0.005, 0.005, -0.005, 0.005, 0.003)
            o.data.transform(Matrix.Translation((x + 0.06 + rnd.uniform(-0.02, 0.03), y + rnd.uniform(-0.06, 0.06), z + 0.02 + i * 0.002)) @
                             Matrix.Rotation(rnd.uniform(0, math.pi), 4, 'Z'))
            k.add(o, 'crust', 0xd8a040)


def kpass():
    unbaked('ns_steam', make_steam)
    k = kit('nsdq', 'SM_NS_Ruby_Pass', ('body', 'tile', 'floor', 'print', 'ns_steam'), dust=0.25, low=0.15, print_px=1024)
    TEXTURE_SIZES.update({'nsdq_body': 2048, 'nsdq_tile': 2048, 'nsdq_floor': 1024})
    k.tile_slot('tile', 'wall', (1.2, 1.0), origin=(0.0, 0.0), z_range=(0.0, 2.0))
    k.tile_slot('floor', 'xy', (1.2, 1.2), origin=(0.0, 0.0), z=0.0)
    orders = [['2 SHORT STACK', '  side bacon', '1 COFFEE'], ['1 PATTY MELT', '  no onion', '1 OJ'], ['1 STEAK & EGGS', '  med rare / over easy'],
              ['2 GRILLED CHEESE', '2 TOMATO SOUP'], ['1 SLICE CHERRY', '1 COFFEE DECAF']]
    for i, o in enumerate(orders):
        k.panel(f't{i}', 0.075, 0.12, tickets_draw(i + 3, o), density=2.0)
    k.atlas.render()
    ky0, ky1, kx0, kx1, kz = KY0, KY1, KX0, KX1, KZ
    px0, px1 = PASS_X
    pz0, pz1 = PASS_Z
    S = lambda o, aux=0: k.add(o, 'stainless', STEEL, aux=aux)
    # the pass through the wall: the heated shelf, its stainless lining, the header
    S(bx('pass_shelf', px0 - 0.05, px1 + 0.05, PASS_Y - 0.12, ky0 + 0.15, pz0, pz0 + 0.04, 0.006))
    for xx in (px0, px1 - 0.02):
        S(bx('pass_jamb', xx, xx + 0.02, PASS_Y, ky0, pz0, pz1), 2)
    S(bx('pass_head', px0, px1, PASS_Y, ky0, pz1 - 0.02, pz1 + 0.04))
    # the kitchen: white tile to 2 m (tiled), paint above, the ceiling; the quarry tile floor (tiled)
    walls = [((kx0, ky0), (kx1, ky0), (0, 1), [(px0 - kx0, px1 - kx0, pz0, pz1), (KDOOR_X[0] - kx0, KDOOR_X[1] - kx0, 0.0, 2.13)]),
             ((kx0, ky1), (kx1, ky1), (0, -1), []), ((kx1, ky0), (kx1, ky1), (-1, 0), [])]
    for p0, p1, nrm, holes in walls:
        k.add(wall_plane('ktile', p0, p1, 0.0, 2.0, nrm, 2.0, [h for h in holes if h[2] < 2.0]), 'sqtile', 0xffffff, slot='tile')
        k.add(wall_plane('kwall', p0, p1, 2.0, kz, nrm, 3.0, [h for h in holes if h[3] > 2.0]), 'paint', 0xe8e4d8)
    k.add(grid_plane('kfloor', kx0, kx1, ky0, ky1, 0.0, 2.4), 'quarry', 0xffffff, slot='floor')
    k.add(grid_plane('kceil', kx0, kx1, ky0, ky1, kz, 3.0, up=False), 'paint', 0xe4e0d4)

    def line_box(name, a, b, d, h):
        k.add(bx(name, a, b, ky1 - d, ky1, 0.0, h, 0.01), 'stainless', STEEL, aux=2)

    # the cook line on the north wall: a range, the flat-top, the fryer, a prep cooler, a reach-in; the hood
    line_box('range', 4.60, 5.85, 0.80, 0.86)
    for (gx, gy) in ((4.85, 9.55), (5.30, 9.55), (4.85, 9.85), (5.30, 9.85)):
        k.add(bx('grate', gx - 0.18, gx + 0.18, gy - 0.14, gy + 0.14, 0.86, 0.90), 'iron')
    k.add(lathe_solid('pot', [(0.0, 0.90), (0.16, 0.90), (0.165, 0.91), (0.165, 1.18), (0.17, 1.19), (0.0, 1.19)], Matrix.Translation((4.85, 9.85, 0.0)), 32),
          'stainless', STEEL, aux=2)
    k.add(bx('range_door', 4.75, 5.70, ky1 - 0.81, ky1 - 0.80, 0.15, 0.65, 0.006), 'stainless', STEEL)
    for kx_ in np.linspace(4.75, 5.70, 5):
        k.add(parts.rod('knob', (kx_, ky1 - 0.80, 0.78), (kx_, ky1 - 0.83, 0.78), 0.018, 16), 'plastic', 0x1a1a1a)
    line_box('flattop_base', 5.95, 7.45, 0.80, 0.84)
    k.add(bx('griddle', 5.97, 7.43, ky1 - 0.70, ky1 - 0.02, 0.84, 0.885, 0.003), 'iron')
    k.add(bx('griddle_splash', 5.95, 7.45, ky1 - 0.03, ky1, 0.885, 1.10), 'stainless', STEEL)
    for xs in (5.95, 7.43):
        k.add(bx('griddle_side', xs, xs + 0.02, ky1 - 0.72, ky1, 0.885, 1.0), 'stainless', STEEL)
    k.add(bx('trough', 5.97, 7.43, ky1 - 0.80, ky1 - 0.70, 0.80, 0.86, 0.006), 'stainless', STEEL)
    for i in range(4):
        o = bx('patty', -0.05, 0.05, -0.05, 0.05, 0.0, 0.012, 0.02)
        o.data.transform(Matrix.Translation((6.3 + i * 0.16, ky1 - 0.35, 0.886)))
        k.add(o, 'crust', 0x5a2a14)
    for i in range(3):
        k.add(lathe_solid('cake', [(0.0, 0.0), (0.07, 0.0), (0.072, 0.006), (0.0, 0.009)],
                          Matrix.Translation((6.95 + (i % 2) * 0.17, ky1 - 0.20 - (i // 2) * 0.18, 0.886)), 24), 'crust', 0xc8843a)
    k.add(bx('spatula', 7.15, 7.35, ky1 - 0.62, ky1 - 0.52, 0.887, 0.89, 0.002), 'stainless', STEEL)
    line_box('fryer', 7.55, 8.10, 0.80, 0.90)
    for fx in (7.69, 7.96):
        k.add(bx('fry_oil', fx - 0.11, fx + 0.11, ky1 - 0.62, ky1 - 0.20, 0.86, 0.885), 'liquid', 0x6a4a10)
        k.add(bx('basket', fx - 0.10, fx + 0.10, ky1 - 0.30, ky1 - 0.08, 0.95, 1.08, 0.004), 'chrome')
        k.add(tube('basket_handle', [(fx, ky1 - 0.30, 1.05), (fx, ky1 - 0.55, 1.12)], 0.008, 8), 'plastic', 0x1a1a1a)
    line_box('prep_cooler', 8.25, 9.35, 0.75, 0.88)
    k.add(bx('cutboard', 8.30, 9.30, ky1 - 0.70, ky1 - 0.40, 0.88, 0.90, 0.004), 'plastic', 0xe8e6de)
    for i in range(5):
        k.add(bx('pan', 8.32 + i * 0.20, 8.50 + i * 0.20, ky1 - 0.38, ky1 - 0.08, 0.86, 0.90, 0.004), 'stainless', STEEL)
        k.add(bx('pan_fill', 8.33 + i * 0.20, 8.49 + i * 0.20, ky1 - 0.37, ky1 - 0.09, 0.87, 0.895, 0.004), 'syrup',
              [0xc4231c, 0x5a8a3a, 0xe8d8a8, 0xd8b030, 0x6a2a10][i])
    line_box('reachin', 9.50, 10.30, 0.78, 2.0)
    for (za, zb) in ((0.10, 0.98), (1.02, 1.95)):
        k.add(bx('reach_door', 9.52, 10.28, ky1 - 0.79, ky1 - 0.78, za, zb, 0.006), 'stainless', STEEL, aux=2)
        k.add(bx('reach_handle', 10.18, 10.22, ky1 - 0.83, ky1 - 0.79, za + 0.2, zb - 0.2, 0.01), 'chrome')
    k.add(core.prism_x('hood', [(ky1, 2.05), (ky1 - 1.05, 2.05), (ky1 - 1.10, 2.25), (ky1 - 0.95, 2.80), (ky1, 2.80)], 4.40, 8.30), 'stainless', STEEL)
    for i in range(9):
        fx = 4.55 + i * 0.42
        k.add(bx('filter', fx, fx + 0.40, ky1 - 0.62, ky1 - 0.55, 2.10, 2.52, 0.004), 'iron')
    # the dish area (west): a three-bay sink and its sprayer; wall shelves of cans, containers, stock pots
    line_box('sink', 1.00, 2.80, 0.70, 0.90)
    for i in range(3):
        k.add(bx('bay', 1.08 + i * 0.58, 1.60 + i * 0.58, ky1 - 0.62, ky1 - 0.10, 0.70, 0.905, 0.01), 'stainless', STEEL)
    k.add(tube('sprayer', bend([(2.0, ky1 - 0.06, 0.90), (2.0, ky1 - 0.06, 1.55), (2.0, ky1 - 0.35, 1.55)], 0.06, 4), 0.012, 10), 'chrome')
    rnd = random.Random(12)
    for (sx0, sx1) in ((1.0, 3.6), (10.5, 12.8)):
        for zz in (1.55, 1.95):
            k.add(bx('wshelf', sx0, sx1, ky1 - 0.35, ky1 - 0.01, zz - 0.02, zz, 0.004), 'stainless', STEEL)
            x = sx0 + 0.05
            while x < sx1 - 0.35:
                kind = rnd.random()
                if kind < 0.4:
                    k.add(lathe_solid('can', [(0.0, zz), (0.08, zz), (0.08, zz + 0.18), (0.0, zz + 0.18)], Matrix.Translation((x + 0.08, ky1 - 0.18, 0.0)), 20),
                          'enamel', rnd.choice([0xc4231c, 0xe8b820, 0x2a6a3a, 0xe8e4da]))
                    x += 0.18
                elif kind < 0.75:
                    k.add(bx('cambro', x, x + 0.22, ky1 - 0.30, ky1 - 0.06, zz, zz + 0.20, 0.01), 'plastic', 0xe8ecec)
                    x += 0.25
                else:
                    k.add(lathe_solid('stockpot', [(0.0, zz), (0.14, zz), (0.14, zz + 0.25), (0.0, zz + 0.25)], Matrix.Translation((x + 0.15, ky1 - 0.18, 0.0)), 24),
                          'stainless', STEEL, aux=2)
                    x += 0.32
    # the prep table under the pass (kitchen side); the ticket rail and its tickets; plates up; the order bell
    k.add(bx('prep_top', px0 - 0.10, px1 + 0.10, ky0 + 0.02, ky0 + 0.72, 0.88, 0.92, 0.006), 'stainless', STEEL)
    k.add(bx('prep_shelf', px0 - 0.08, px1 + 0.08, ky0 + 0.04, ky0 + 0.70, 0.20, 0.22, 0.004), 'stainless', STEEL)
    for (lx, ly) in ((px0 - 0.07, ky0 + 0.06), (px1 + 0.07, ky0 + 0.06), (px0 - 0.07, ky0 + 0.68), (px1 + 0.07, ky0 + 0.68)):
        k.add(parts.rod('prep_leg', (lx, ly, 0.0), (lx, ly, 0.88), 0.018, 12), 'stainless', STEEL)
    for i in range(10):
        k.add(lathe_solid('plate', [(0.0, 0.0), (0.13, 0.0), (0.135, 0.006), (0.14, 0.013), (0.0, 0.010)], Matrix.Translation((px0 + 0.3, ky0 + 0.35, 0.92 + i * 0.013)), 32),
              'ceramic', 0xf4f1ea)
    k.add(bx('rail', px0 + 0.10, px1 - 0.10, ky0 + 0.02, ky0 + 0.05, pz1 - 0.06, pz1 - 0.03, 0.003), 'chrome')
    for i in range(5):
        tx = px0 + 0.4 + i * 0.5
        k.decal(f't{i}', (tx, ky0 + 0.0185, pz1 - 0.11), u=(1, 0, 0), v=(0, 0, 1), w=0.075, h=0.12, lift=0.0, nu=1, nv=3,
                disp=lambda s_, t: -0.006 * (1 - t) ** 2)
    plate_food(k, 7.10, PASS_Y + 0.08, pz0 + 0.04, 'stack')
    plate_food(k, 8.10, PASS_Y + 0.10, pz0 + 0.04, 'melt')
    bm_ = Matrix.Translation((px1 - 0.15, PASS_Y - 0.06, pz0 + 0.04))
    k.add(lathe_solid('bell_base', [(0.0, 0.0), (0.045, 0.0), (0.046, 0.004), (0.044, 0.014), (0.03, 0.018), (0.0, 0.018)], bm_, 32), 'enamel', 0x121212)
    k.add(lathe_solid('bell', [(0.0, 0.0185)] + [(0.042 * math.sin(math.radians(a)), 0.0185 + 0.034 * math.cos(math.radians(a))) for a in range(90, -1, -10)],
                      bm_, 32), 'chrome')
    # the heat lamps' stems and shades (their bulbs glow); the kitchen's tube fittings (their lenses glow)
    heat = []
    for hx in (6.70, 7.70, 8.70):
        M = Matrix.Translation((hx, PASS_Y + 0.075, 0.0))
        k.add(parts.rod('hl_stem', (hx, PASS_Y + 0.075, pz1 - 0.02), (hx, PASS_Y + 0.075, pz1 - 0.10), 0.006, 10), 'chrome')
        shade = lathe_shell('hl_shade', [(0.03, pz1 - 0.10), (0.06, pz1 - 0.13), (0.085, pz1 - 0.19), (0.088, pz1 - 0.20)], M, 32)
        K.solidify(shade, 0.0015)
        k.add(shade, 'chrome')
        heat.append(lathe_solid('hl_bulb', [(0.0, pz1 - 0.12), (0.035, pz1 - 0.125), (0.06, pz1 - 0.16), (0.062, pz1 - 0.18), (0.04, pz1 - 0.195), (0.0, pz1 - 0.20)],
                                M, 24))
    cool = []
    for (fx0, fy) in ((2.4, 8.9), (8.4, 8.9)):
        k.add(bx('kfix', fx0, fx0 + 1.22, fy - 0.12, fy + 0.12, kz - 0.06, kz, 0.006), 'enamel', 0xf0eee6)
        cool.append(bx('klens', fx0 + 0.02, fx0 + 1.20, fy - 0.10, fy + 0.10, kz - 0.09, kz - 0.06, 0.01))
    # the steam card over the flat-top (UV 0..1 as seen through the pass)
    st = quad('steam', [(5.95, 9.70, 0.95), (7.40, 9.70, 0.95), (7.40, 9.70, 1.95), (5.95, 9.70, 1.95)])
    uv = st.data.uv_layers.new(name='UVMap')
    for li, t in enumerate(((0, 0), (1, 0), (1, 1), (0, 1))):
        uv.data[li].uv = t
    core.orient_normals(st, (0, -1, 0))
    k.special(st, 'ns_steam')
    k.grime = [((6.7, ky1 - 0.4, 0.9), 0.9, 0.8), ((7.8, ky1 - 0.4, 0.9), 0.5, 0.7), ((6.7, ky1 - 0.5, 2.3), 1.2, 0.6)]
    obj = k.finish(35.0)
    return [obj, glow(heat, 'SM_NS_Ruby_Pass_GlowWarm', ANCHOR, LOG), glow(cool, 'SM_NS_Ruby_Pass_GlowCool', ANCHOR, LOG)]


def menu_image():
    def draw(s, w, h):
        s.rect(0, 0, w, h, 0x141414, rough=0.7)
        for i in range(30):
            s.rect(0, i * h / 30, w, h * 0.006, 0x0c0c0c, rough=0.8)
        s.mid("RUBY'S", w / 2, h * 0.86, h * 0.13, w * 0.3, 0xf2efe6, 'Black', tracking=1.3)
        items = [('THE SHORT STACK', '6.99'), ('TWO EGGS HOME FRIES TOAST', '7.49'), ('PATTY MELT & FRIES', '10.99'), ('GRILLED CHEESE & TOMATO SOUP', '6.49'),
                 ('CHICKEN NOODLE SOUP', '5.49'), ('STEAK & EGGS', '14.99'), ('SLICE OF PIE', '4.50'), ('COFFEE BOTTOMLESS', '2.25'), ('ORANGE JUICE', '3.25'),
                 ('MILKSHAKE', '5.25')]
        for i, (a, b) in enumerate(items):
            col = i % 2
            row = i // 2
            x = w * (0.04 + col * 0.5)
            y = h * (0.64 - row * 0.13)
            s.text(a, x, y, h * 0.058, 0xf2efe6, 'Bold', tracking=1.08)
            s.text(b, x + w * 0.44, y, h * 0.058, 0xf2efe6, 'Bold', align='RIGHT')
        s.mid('BREAKFAST ALL DAY · PIE IS CHERRY', w / 2, h * 0.025, h * 0.045, w * 0.6, 0xe8b820, 'Bold', tracking=1.3)
    return render_image('ns_screen_menu', 340.0, 100.0, 2048, draw)


def menu_board():
    screen_material('ns_screen_menu', menu_image(), lit=False, rough=0.7)
    k = kit('nsdm', 'SM_NS_Ruby_MenuBoard', ('body', 'ns_screen_menu'), dust=0.3, low=0.0)
    TEXTURE_SIZES.update({'nsdm_body': 512})
    x0, x1, z0, z1 = 6.00, 9.40, 2.00, 3.00
    yf = PASS_Y - 0.03
    for (a, b, c_, d) in ((x0 - 0.04, x1 + 0.04, z0 - 0.04, z0), (x0 - 0.04, x1 + 0.04, z1, z1 + 0.04), (x0 - 0.04, x0, z0, z1), (x1, x1 + 0.04, z0, z1)):
        k.add(bx('mframe', a, b, yf - 0.01, PASS_Y, c_, d, 0.004), 'stainless', STEEL)
    k.add(bx('mback', x0, x1, yf, PASS_Y, z0, z1), 'felt', 0x141414)
    scr = quad('menu', [(x0, yf - 0.0005, z0), (x1, yf - 0.0005, z0), (x1, yf - 0.0005, z1), (x0, yf - 0.0005, z1)])
    uv = scr.data.uv_layers.new(name='UVMap')
    for li, t in enumerate(((0, 0), (1, 0), (1, 1), (0, 1))):
        uv.data[li].uv = t
    core.orient_normals(scr, (0, -1, 0))
    k.special(scr, 'ns_screen_menu')
    return [k.finish(35.0)]


# ================================================================== build

BUILDERS = {'front': front, 'door': door, 'shell': shell, 'counter': counter, 'stool': stool, 'booth': booth, 'backbar': backbar, 'pass': kpass,
            'menu': menu_board}


def build():
    core.reset()
    random.seed(1995)
    K.UNB.clear()
    K.PROXIES.clear()
    _BOOMERANG.clear()
    unbaked('ns_glass')
    ALL.clear()
    for key in WHICH:
        out = BUILDERS[key]()
        ALL.extend(out if isinstance(out, list) else [out])
    for o in ALL:
        o.data.name = o.name
    return list(ALL)


def bake_parts(objs):
    return K.split_for_bake(objs)


def assemble(objs):
    return K.join_after_bake(ALL, MESHES)


# ================================================================== review

AY = ANCHOR[1]
L = lambda x, y, z=0.0: (x, y + AY, z)
_PA, _PB = arc_pt(DOOR_T[0], RG), arc_pt(DOOR_T[1], RG)
_AL = (Vector(_PB) - Vector(_PA)).normalized()
DOOR_W = Vector(_PA) + _AL * 0.04
DOOR_S = Vector(_PB) - _AL * 0.04
# PROP placements in the review: (Blender SITE location, stage yaw F); the doors' F from their docstring yaws (UE yaw - 180).
PLACE = {
    'SM_NS_Ruby_Stool': [(L(x, y), 0.0) for x, y in STOOLS],
    'SM_NS_Ruby_Booth': [(L(x, BOOTH_Y), 90.0) for x in BOOTH_X],
    'SM_NS_Ruby_Door': [(L(DOOR_W.x, DOOR_W.y), 135.0), (L(DOOR_S.x, DOOR_S.y), -45.0)],
    'SM_NS_Ruby_PieCarousel': [(L(PIE_C[0], PIE_C[1], PIE_Z), 0.0)],
}
GLOW_REVIEW = {'SM_NS_Ruby_Neon_Red': (0xff1a10, 14.0), 'SM_NS_Ruby_Neon_Blue': (0x2a6aff, 12.0), 'SM_NS_Ruby_Shell_GlowWarm': (0xffe0b4, 7.0),
               'SM_NS_Ruby_Backbar_GlowWarm': (0xff7a2a, 3.0), 'SM_NS_Ruby_Pass_GlowWarm': (0xff5a1a, 6.0), 'SM_NS_Ruby_Pass_GlowCool': (0xf2f6ff, 7.0)}


def _context(temp, day):
    brick = K.rv_mat('rv_brick', 0x7a3a2a, 0.85)
    walk = K.rv_mat('rv_walk', 0x7d7a73 if day else 0x4a4844, 0.8 if day else 0.3)
    road = K.rv_mat('rv_road', 0x2a2b2e, 0.7 if day else 0.15)
    boxes = [(-4.5, 0.0, -4.5, 12.0, -0.02, 0.0, walk), (0.0, 16.0, -4.5, 0.0, -0.02, 0.0, walk), (-16.0, -4.5, -16.0, 12.0, -0.17, -0.15, road),
             (-4.5, 16.0, -16.0, -4.5, -0.17, -0.15, road),
             (0.0, 15.0, 0.0, 10.5, 4.30, 14.0, brick), (15.0, 15.3, 0.0, 10.5, 0.0, 14.0, brick), (0.0, 15.3, 10.5, 10.8, 0.0, 14.0, brick),
             (15.0, 30.0, 0.0, 10.0, 0.0, 9.0, K.rv_mat('rv_brick2', 0x5a4a3a, 0.85)),
             # the slab over the room's ceiling, kept clear of the corner's quadrant (the neon is out there)
             (2.0, 15.0, 0.40, 10.5, 3.81, 4.30, brick), (0.40, 2.0, 2.0, 10.5, 3.81, 4.30, brick)]
    for (x0, x1, y0, y1, z0, z1, mt) in boxes:
        o = bx('rv_ctx', x0, x1, y0 + AY, y1 + AY, z0, z1)
        core.assign(o, mt)
        temp.append(o)
    # cut the corner off the upper body (a stand-in: a chamfer box over the curve)
    o = bx('rv_corner', -1.0, 1.0, -1.0, 1.0, 4.30, 14.0)
    o.data.transform(Matrix.Translation((0.6, AY + 0.6, 0.0)) @ Matrix.Rotation(math.radians(45), 4, 'Z'))
    core.assign(o, brick)
    temp.append(o)


def _lights(temp, day):
    for (x, y, z) in GLOBES:
        temp.append(K.rv_light('rv_globe', 'POINT', L(x, y, z), 55.0, (1.0, 0.85, 0.65), size=0.12))
    temp.append(K.rv_light('rv_kitchen', 'AREA', L(3.0, 8.9, 2.94), 160.0, (0.95, 0.97, 1.0), size=1.2, size_y=0.2))
    temp.append(K.rv_light('rv_kitchen2', 'AREA', L(9.0, 8.9, 2.94), 160.0, (0.95, 0.97, 1.0), size=1.2, size_y=0.2))
    for hx in (6.70, 7.70, 8.70):
        temp.append(K.rv_light('rv_heat', 'POINT', L(hx, PASS_Y + 0.075, 1.50), 12.0, (1.0, 0.45, 0.15), size=0.05))
    temp.append(K.rv_light('rv_neon', 'AREA', L(-0.25, -0.25, 3.75), 60.0, (1.0, 0.12, 0.06), size=1.6, size_y=0.6, target=L(-2.0, -2.0, 0.0)))
    temp.append(K.rv_light('rv_neonb', 'POINT', L(0.2, 2.9, 1.65), 8.0, (0.2, 0.4, 1.0), size=0.2))
    if day:
        temp.append(K.rv_light('rv_sun', 'SUN', (0, 0, 10), 4.0, (1.0, 0.96, 0.9), size=2.0, rot=(math.radians(50), 0.0, math.radians(-150))))
        bpy.context.scene.world.color = (0.55, 0.6, 0.68)
    else:
        temp.append(K.rv_light('rv_lamp', 'SPOT', L(-3.5, -3.0, 8.5), 6000.0, (1.0, 0.62, 0.3), size=0.3, target=L(-1.5, -1.0, 0.0), spot=85))
        temp.append(K.rv_light('rv_led', 'SPOT', L(8.0, -3.5, 8.5), 5000.0, (1.0, 0.93, 0.85), size=0.3, target=L(8.0, -1.0, 0.0), spot=85))
        bpy.context.scene.world.color = (0.004, 0.005, 0.009)


SHEETS = {
    'ns_diner_room': [
        ('corner', L(-7.0, -7.5, 1.65), L(1.2, 1.6, 2.1), 26, 'night'),
        ('corner_day', L(-7.5, -6.5, 1.7), L(1.2, 1.6, 2.1), 26, 'day'),
        ('market', L(8.0, -9.0, 1.65), L(7.5, 2.0, 1.9), 22, 'night'),
        ('inside', L(1.4, 1.6, 1.62), L(10.0, 6.2, 1.2), 18, 'night'),
        ('inside_back', L(13.6, 3.4, 1.65), L(3.0, 3.6, 1.3), 18, 'night'),
        ('fifth', L(-6.5, 5.0, 1.65), L(0.5, 5.5, 1.8), 24, 'night'),
    ],
    'ns_diner_detail': [
        ('counter', L(3.0, 3.6, 1.55), L(7.5, 5.4, 0.9), 22, 'night'),
        ('backbar', L(8.3, 5.2, 1.55), L(9.9, 7.4, 1.2), 22, 'night'),
        ('pie', L(5.0, 5.6, 1.45), L(5.83, 7.15, 1.25), 30, 'night'),
        ('pass', L(7.7, 5.0, 1.55), L(7.0, 9.6, 1.25), 24, 'night'),
        ('booth', L(7.6, 3.2, 1.55), L(8.8, 0.8, 0.8), 24, 'night'),
        ('neon', L(-2.4, -2.4, 3.0), L(0.6, 0.6, 3.75), 28, 'night'),
    ],
}


def room_review(objs, which):
    sc = bpy.context.scene
    tmp_dir = os.path.join(core.BUILD_DIR, 'review', 'ns_diner')
    os.makedirs(tmp_dir, exist_ok=True)
    temp = []
    by = {o.name: o for o in objs}
    for n, places in PLACE.items():
        if n in by:
            for loc, F in places:
                temp.append(K.instance(by[n], loc, K.prop_yaw(F)))
            by[n].hide_render = True
    for n, (hexc, st) in GLOW_REVIEW.items():
        if n in by:
            K.glow_tint(by[n], hexc, st)
            by[n].visible_shadow = False
    cd = bpy.data.cameras.new('rv_cam')
    cam = core.link(bpy.data.objects.new('rv_cam', cd))
    cd.clip_start = 0.02
    cd.clip_end = 300.0
    sc.camera = cam
    temp.append(cam)
    out = {}
    try:
        for mode in ('night', 'day'):
            jobs = [(sheet, sh) for sheet, shots in SHEETS.items() if sheet.split('_')[-1] in which for sh in shots if sh[4] == mode]
            if not jobs:
                continue
            ctx = []
            _context(ctx, mode == 'day')
            _lights(ctx, mode == 'day')
            for sheet, (label, loc, tgt, lens, _) in jobs:
                out[(sheet, label)] = K.rv_shot(cam, os.path.join(tmp_dir, f'{label}.png'), loc, tgt, lens, 960, 640, 128)
            for o in ctx:
                bpy.data.objects.remove(o)
        for sheet, shots in SHEETS.items():
            paths = [out[(sheet, sh[0])] for sh in shots if (sheet, sh[0]) in out]
            if paths:
                K.rv_compose(sheet, paths, 2, LOG)
    finally:
        for o in temp:
            try:
                bpy.data.objects.remove(o)
            except ReferenceError:
                pass
        for n in PLACE:
            if n in by:
                by[n].hide_render = False
        sc.camera = None
        sc.world.color = (0.006, 0.0065, 0.008)


def pose_for_review(objs):
    """Renders ns_diner_room.jpg and ns_diner_detail.jpg, then lines the PROPs up for the standard sheet."""
    global REVIEW_VIEWS
    which = {s.strip() for s in os.environ.get('NSD_REVIEW', 'room,detail').split(',') if s.strip()}
    for p in K.PROXIES:
        p.hide_render = True
    if which:
        room_review(objs, which)
    props = [o for o in objs if o.name in PLACE]
    if props:
        x = 0.0
        for o in objs:
            o.hide_render = o not in props
        for o in props:
            lo, hi = core.bounds([o])
            o.location = (x - lo.x, -(lo.y + hi.y) / 2, -lo.z)
            x += max(hi.x - lo.x, hi.y - lo.y) + 0.4
        bpy.context.view_layer.update()   # refresh matrix_world, or the sheet frames the row at the origin
        objs[:] = props
    REVIEW_VIEWS = None

