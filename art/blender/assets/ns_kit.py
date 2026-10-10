"""SM_NS_*: Northside's street furniture (world realism spec §4.2): lamps, signals, signs, the bus shelter, awnings, the
sidewalk's dressing, the work zone, trees and litter.

Every mesh is PROP frame: meters, Z up, the FRONT toward Blender -X, the origin on the ground under the middle of the piece
unless an entry says otherwise. Placed to face stage yaw F, a mesh turns by -F about Blender Z in the SITE frame (ue-world:
yaw F + 180). Printed text reads correctly from outside its face in Blender, so it reads correctly in the game.

Slots are listed per mesh in the order the build log prints. Baked slots become M_<Mesh>_<i>. Unbaked slots, named for the
runtime materials: ns_glass ([glass], M_NS_Glass), ns_screen_<what> ([screen]: UV0 0..1 across the face as seen, (0, 0) at
its bottom left; a default picture is embedded so the mesh reads before the game draws it), ns_leaf ([tile:ns_leaf]). A mesh
with an unbaked slot is baked without it and joined back (bake_parts / assemble), its slot order kept. [tint] slots bake light
and neutral (the game multiplies the color in through custom primitive data 0..2).

Glow meshes (unbaked, one slot each, parts.glow_object): _GlowCool (4000-5000 K), _GlowWarm (2700-3000 K), _GlowSignal: each
lamp face is its own UV island in a 4 x 2 grid (columns u 0..1 in quarters, rows v 0..0.5 / 0.5..1, row 0 at v 0..0.5): cells
0 red, 1 amber, 2 green, 3 walk, 4 hand, 5 countdown, 6-7 spare; each face fills its cell (inset 2 %) as seen from the front, so a
material can draw on it. The walk and hand faces are the symbols' own shapes; the countdown face is a plain rectangle.

The meshes (sizes W x D x H, m; what was picked inside the spec's ranges):
- SM_NS_StreetlightLED (pole base origin; the arm reaches -X): Fifth's pole and fluted base (SM_Streetlight's shape, 8.4 m to
  the arm's sleeve) carrying an LED cobra head: a flat die-cast housing with 13 cooling fins and a parting lip, a NEMA
  photocell, the optic panel under it (a bezel round 4 x 12 LED optics, 37 x 35 mm grid), two nose latches; two banner arms
  (4.6 and 6.4 m, toward +Y), the hand hole and an ID tag. Lens center (-2.15, 0, 8.92) (its light points down). Slots:
  nsk_led_pole, nsk_led_head. + _GlowCool: the optic panel (a sheet just under the optics).
- SM_NS_SignalMast (pole base origin; the arm along -X): a 7 m tapered pole on an anchor-bolted base plate, a 6.5 m mast arm
  at 6.0 m rising to 6.25 at its tip, two 12-inch 3-section heads hung from it on rigid mounts (a pipe from the head's top
  boss to a saddle with two U-bolts round the arm, the cable's drip loop beside it), each a stack of die-cast sections with
  hinged doors (knuckles, wing-screw latches), shallow LED lenses in gaskets, tunnel visors, a black backplate with a 50 mm
  yellow retroreflective border, riveted; housings' bottoms 4.88 m, backplates' 4.72 m over the pole base. The pole: a
  hand hole on +X, an ID tag, a ground lug; the arm's double flange plate bolted on, a gusset. _A (SM_NS_SignalMast):
  the tip head (x -6.2) faces +Y, the mid head (x -4.5) faces -Y: on Fifth's far side facing east (stage yaw 180) the tip head
  faces south (northbound) and the mid one north (southbound). SM_NS_SignalMast_B (an addition: Ruby's corner): both heads
  face -Y (placed with the arm reaching south over Market, yaw -90, they face east). Slots: nsk_sig_pole (painted
  galvanized), nsk_sig_heads. + _GlowSignal (red/amber/green per head).
- SM_NS_SignalPedestal: a 3.5 m pole on a cast transformer base (rounded, a door with two screws, a collar with set screws),
  one 3-section head (no backplate) on a slipfitter on top, facing -X, the head centered over the pole. Slots: pole, heads.
  + _GlowSignal.
- SM_NS_PedSignal: a 3 m pole; at 2.45 m a two-section pedestrian head facing -X: the hand (left) and the walking person
  (right) side by side in the top section, the countdown below, an egg-crate louver in each section's hood; clamshell
  brackets to the pole; a push-button station at 1.07 m on the pole's -X side (strapped on: the big button with its raised
  arrow, a speaker grille) with its sign on a rounded blank (PUSH BUTTON FOR WALK SIGNAL, an arrow). Slots: pole, heads,
  prints. + _GlowSignal (walk, hand, countdown).
- SM_NS_CornerBlades_A..D: a 3.6 m galvanized pole (a cast foot collar) with a cast cap holding two double-faced street-name
  blades (0.9 x 0.15 m with 25 mm rounded corners and rolled rims, transit green, white letters, block numbers) in bolted
  saddles: the upper blade lies along Blender X (read from +-Y), the lower along Y (read from +-X). A FIFTH ST 1800-1900 / MARKET ST 200; B FIFTH ST 1700-1800 / HOLLIS ST; C FIFTH ST 1900-2000 / WEXLER ST;
  D MARKET ST 200-300 / CARDINAL AVE. Slots: pole, prints.
- SM_NS_SignPost_NoParking / _NoStanding / _Stop / _BusFlag: a 2 in. square perforated post (rounded corners, its holes in
  the bake), 3 m, in an anchor sleeve, capped; the sign a rounded aluminum blank bolted on its -X face (washers in front, nuts
  behind; the flag double-faced on two bolted brackets). Slots: post, prints.
- SM_NS_BikeRack: a galvanized 48 mm pipe inverted U, 0.75 x 0.9 m, on two bolted flanges (along Y). Slot: steel.
- SM_NS_IceChest: 1.5 x 0.8 x 1.2 m outdoor ice merchandiser, two front doors (ICE, PACKAGED ICE · SELF SERVE), frost at the
  door seals, a padlocked hasp, the condenser louvers, rust at the foot. Slots: cabinet, prints.
- SM_NS_CellarDoors: two diamond-plate leaves 1.5 x 1.2 m (strap hinges along the long sides, carriage bolts), a steel angle
  frame, a lift ring in a plate on each leaf, the hasp and a padlock; the plate stands 4 mm proud of the sidewalk (origin:
  center, on the sidewalk's surface). Slot: steel.
- SM_NS_Awning_S / _M / _L (3 / 6 / 9 m wide, 1.6 m projection): origin on the wall plane at the top middle (x 0 = the wall,
  the awning out toward -X, z 0 = its top, everything below). A roller hood on the wall, the fabric sloping 0.55 m down to the
  front bar, folding arms under it (2 / 2 / 3 pairs: shoulder brackets bolted to the wall, elbow knuckles, end fittings on the
  bar), the bar's end caps, the hood's end plates and the crank gearbox with its eye at the +Y end, a 0.25 m valance. Slots: nsk_awn_fabric [tint] (UV0: u 0..1 left to
  right seen from the street, v 0 at the front bar .. 1 at the wall, so a stripe = u x width / stripe width), nsk_awn_metal,
  ns_screen_valance (the valance's front: u 0..1 left to right, v 0..1 up).
- SM_NS_Barricade (1.8 m Type I sawhorse, two orange/white boards, rubber feet, a flasher on a battery box), SM_NS_Cone
  (0.71 m, two collars), SM_NS_RoadPlate (2.4 x 1.5 x 0.025 m steel, along Y, origin center top; locating pins, lifting
  holes, a lumpy cold-patch wedge feathered round it in nsk_plate_patch), SM_NS_JerseyBarrier (3.05 x 0.61 x 0.81 m F-shape
  precast, rounded arrises, chamfered ends, lifting slots, two drainage scuppers, connecting loops).
- SM_NS_SidewalkClosure: a 2.4 m galvanized mesh fence panel pinned into two weighted feet, couplers on its posts, a bracing
  stay each side, with SIDEWALK CLOSED / USE OTHER SIDE on a rounded blank, cable-tied at its corners.
- SM_NS_IceChest additions: three hinge barrels per door, riveted aluminum corner trims, the top's rim, the condenser grille
  (slanted louvers) on the +Y end, the power cord to a wall box behind (+X).
- SM_NS_BusShelter (4.6 x 1.6 x 2.6 m, floor middle origin, the open side toward -X): galvanized 100 mm posts (rounded) on
  anchor-bolted base plates under cast foot covers; a curved aluminum roof deck on seven T-ribs, a rolled drip edge front and
  ends, fascia beams (bolted) carrying the CITY TRANSIT header (green and gold, the 14's badge) outside front and back; the
  glass back and the glass -Y end (ns_glass) in channels with black gaskets, kick plates, a mid rail, three rows of white
  frit dots at eye level; the lightbox in the -Y end (ns_screen_ad, both faces: default RiverLine · the night shift never
  closes) in a rounded black housing with framed doors, a lock, hinges, a vented top; the route-map case inside the back
  wall (baked, a glass front, a lock); the timetable screen on the +Y back bay facing the inside (ns_screen_timetable,
  default the 14's times) in a rounded housing with a bezel, a help button and a speaker grille; a tilted perch rail on
  three cantilever brackets; a downspout down the +Y back post; the downlight strip under the front beam. Slots:
  nsk_sh_frame, nsk_sh_trim (black powder coat), nsk_sh_prints, ns_glass, ns_screen_ad, ns_screen_timetable. + _GlowCool
  (the roof's downlight strip).
- SM_NS_Tree_A / _B (trunk base origin): a London plane 9.2 m (crown 6.2 x 6.6 m, 14,000 leaves) and a honey locust 7.5 m
  (crown 4.9 x 7.5 m, 10,500 compound leaves), October: grown limbs to twigs (bark: plane's camouflage patches, locust's
  furrows) and leaves as bent 8-triangle cards (folded on the midrib, drooping, twisted) in the leaf atlas, about 30 %
  turned yellow-brown (the rest green or just turning). Slots: nsk_tr<a|b>_bark (baked), ns_leaf ([tile:ns_leaf]: T_NS_Leaf_BaseColor (RGBA, masked), _Normal, _ORM,
  4096, 4 x 4 cells: 0-7 plane leaves, green to brown, 8-15 locust leaflets, row 0 at the top). COLOR_0.R: wind weight (0 at
  the trunk, rising along the limbs, 0.88..1 on the leaves, more at their tips). Foliage normals bent outward from the crown.
- SM_NS_SignalCabinet (P1; 1.2 x 0.6 x 1.4 m on a 0.8 x 1.4 x 0.12 pad, base origin, doors to -X): two doors with louvers,
  hinges, a handle and lock, a vent grille on -Y, the police panel on +Y, anchor bolts, a conduit; stickers and the
  TRAFFIC SIGNAL CONTROL plate (CAB 212-F, 555-0110). Slots: nsk_scb_cab, nsk_scb_pad, nsk_scb_prints.
- SM_NS_MuniMeter (P1; 1.65 m, screen to -X): the pay-and-display head on a post, the solar cap tilted to +Y (its cells a
  grid), keypad, card, coin and receipt slots, a lock, the instructions print. Slots: nsk_mm_body, nsk_mm_steel, nsk_mm_cells,
  nsk_mm_prints, ns_screen_meter ([screen]: PAY HERE / rates by default).
- SM_NS_Mailbox (P1; 1.22 m, chute to -X): the blue collection box on four bolted legs, a domed top, the pull-down chute
  with its hinge and handle, the access door and lock, side ribs; MAIL on both sides (no agency mark), the LAST PICKUP
  plate on the door. Slots: nsk_mb_blue, nsk_mb_prints.
- SM_NS_CallBox (P1; 2.58 m): a fluted red post on a cast base, the box (door, hinge, rivets, the pull hook, FIRE ALARM
  label to -X), a lamp globe in a wire guard on top. Slots: nsk_cb_red, nsk_cbx_prints. + _GlowWarm (the globe).
- SM_NS_Payphone (P1; origin the wall plane's middle at the shell's foot; the shell 0.6 wide, 1.1 tall, standing 0.33 off
  the wall toward -X): a dead phone: no handset, the armored cord's ragged stub, blanked keys, the coin slot, the hook
  lever, PHONE on the hood, stickers. Slots: nsk_pp_steel, nsk_pp_black, nsk_pp_prints.
- SM_NS_PropaneCage (P1; 1.2 x 0.75 x 1.2 m, door to -X): a galvanized wire cage (hinges, hasp, padlock), twelve 20 lb
  tanks (foot rings, collars, valves) on the floor and a shelf, the PROPANE EXCHANGE sign. Slots: nsk_pc_cage, nsk_pc_tank,
  nsk_pc_prints.
- SM_NS_SidewalkShed (P1; one 6 m module, 2.6 deep, the deck at 3.1 m, the parapet to 3.84; origin at the base on the curb
  side, x 0 = the parapet's face, the shed reaching +X to the building): W6 posts on bolted base plates and timber pads,
  headers and knee braces, runners, timber joists, a 4 x 8 plywood deck with curb boards, a green plywood parapet on studs
  with a cap rail, the posted permit (Permit No. 2026-0412-SH) and a FIGHT NIGHT poster, four caged bulkhead lights on a
  conduit. Slots: nsk_shed_steel, nsk_shed_ply, nsk_shed_deck, nsk_shed_prints. + _GlowCool (the four lenses).
- SM_NS_SteamStack (P1; 3 m, base origin over the vent manhole): an orange-and-white striped chimney with two section
  joints on a flared base held by four stays and weights, and a crossed pair of 2 x 2.45 m cards rising from its mouth
  (z 2.95..5.4) in ns_steam (UV 0..1 per card): the runtime M_NS_Steam plays T_NS_Steam (4 x 4 flipbook, 2048, A = density,
  frames row-major from the top left, the plume rising from each cell's bottom middle; the glb's ns_steam carries frame 0
  with its alpha as a default). Slots: nsk_ss_stack, ns_steam.
- SM_NS_GarbageBags_A / _B / _C (P1; base origin): piles of black bags, glossy where wet, knotted: 5 + 1, 8 + 2, 3 + 1 bags.
  Slot: nsk_gb<a|b|c>_bag. SM_NS_Cardboard: a twine-bundled stack of 11 flattened boxes (warped, creased, scored) with two
  leaned against it. Slots: nsk_cbd_card, nsk_cbd_twine.
- SM_NS_SandwichBoard (P1; 0.97 m A-frame, its feet 0.48 m apart, the faces to +-X leaning back 14 deg): ns_screen_chalk on
  both faces (default: TODAY Coffee + Pie $5).
  Slots: nsk_sb_frame, ns_screen_chalk. SM_NS_Bench (1.8 m steel slats, black, bolted feet): nsk_bn_steel.
  SM_NS_TreeGuard (1.3 m square hoop fence, origin the pit's center): nsk_tg_steel.
- SM_NS_Litter_Cup / _Can / _Bottle / _Butts / _Flyer / _Wrapper / _Leaves / _Cap (P1; 3-50 cm, origin on the ground under
  their middle): one baked slot each, nsk_lt_<name>.
- SM_NS_LockedBike / SM_NS_LockedBike_B (P1; 1.75 m, origin on the ground under its middle, the bike along Y with its front
  wheel at -Y and its drive side to -X): a commuter bike (teal steel diamond frame, flat bar, fenders, a rear rack, a bell
  and a lamp, 700c wheels laced 3-cross) U-locked through its rear triangle. Place it with SM_NS_BikeRack's yaw, 6 cm to the
  rack's -X side (bike origin = rack origin + (-0.06, 0, 0) in the rack's frame): the lock then rings the rack's +Y leg. _B:
  its front wheel stolen, the bike pitched forward onto its fork tips (pivoting on the rear tire). Slots: nsk_bk_frame
  (nsk_bkb_ for _B), nsk_bk_rubber, nsk_bk_metal, nsk_bk_lock.

NS_KIT_ONLY=led,mast,... (keys of BUILDERS) builds a subset; NS_KIT_REVIEW=props,signs picks the extra sheets.
"""
import math
import os
import random
import sys
import time
import traceback

import bmesh
import bpy  # noqa: F401
import numpy as np
from mathutils import Matrix, Vector

from artkit import core, parts, review, street
from artkit.shade import convex_edges, mix_float, noise, object_coords, ramp, scaled, smooth_less, vec
from artkit.sheet import Sheet

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ns_ground import (H, lin, _png, attr, compose_grid, finish_mesh, import_glb, lock_uvs, plain_material, prism_x,  # noqa: E402
                       prism_y, set_attr, set_bump, text_bounds, text_mesh, tri_count, ao_cavity)
import ns_ground as _G  # noqa: E402


# ------------------------------------------------------------------ the detail layer
# Seen from 1-5 m nothing manufactured has a knife edge or a faceted round: every lathed profile's corners get a small
# fillet, every rounded box at least 2-3 bevel segments, and rods, tubes and lathes enough sides for their radius
# (their facets within 0.25 mm of the true round). Hex nuts and bolt heads (6 sides) stay hexagonal.

FILLET = 0.003


def _round_profile(profile, rf=FILLET):
    pts = [tuple(p) for p in profile]
    out = []
    n = len(pts)
    for i, p in enumerate(pts):
        if i == 0 or i == n - 1 or p[0] <= 1e-6:
            out.append(p)
            continue
        a, c = pts[i - 1], pts[i + 1]
        la = math.hypot(a[0] - p[0], a[1] - p[1])
        lc = math.hypot(c[0] - p[0], c[1] - p[1])
        if la < 1e-6 or lc < 1e-6:
            out.append(p)
            continue
        ua = ((a[0] - p[0]) / la, (a[1] - p[1]) / la)
        uc = ((c[0] - p[0]) / lc, (c[1] - p[1]) / lc)
        if ua[0] * uc[0] + ua[1] * uc[1] < -0.985:          # nearly straight: nothing to round
            out.append(p)
            continue
        fr = min(rf, 0.3 * la, 0.3 * lc, 0.45 * p[0])
        pa = (p[0] + ua[0] * fr, p[1] + ua[1] * fr)
        pc = (p[0] + uc[0] * fr, p[1] + uc[1] * fr)
        for t in (0.0, 0.5, 1.0):                           # a quadratic Bezier through the corner: 2 segments
            out.append(((1 - t) ** 2 * pa[0] + 2 * (1 - t) * t * p[0] + t * t * pc[0],
                        (1 - t) ** 2 * pa[1] + 2 * (1 - t) * t * p[1] + t * t * pc[1]))
    return out


SAGITTA = 0.00025


def _sides(r, n, lo=8, hi=64):
    """Enough sides that a round's facets stray no more than SAGITTA from the true circle (never fewer than asked for)."""
    if r <= SAGITTA * 2:
        return max(n, lo)
    need = int(math.ceil(math.pi / math.acos(1.0 - SAGITTA / r)))
    return max(n, min(hi, max(lo, need)))


def lathe(name, profile, mat=None, seg=48, matrix=None, fillet=FILLET):
    if seg != 6:
        rmax = max((p[0] for p in profile), default=0.0)
        seg = _sides(rmax, seg, 12, 96)
    return _G.lathe(name, _round_profile(profile, fillet) if fillet > 0 else profile, mat, seg, matrix)


def box(x0, x1, y0, y1, z0, z1, mat, r=0.0, seg=2, name='box'):
    if r > 0.0:
        seg = max(seg, 3 if r >= 0.004 else 2)
    return _G.box(x0, x1, y0, y1, z0, z1, mat, r, seg, name)


def rod(a, b, r, mat, n=16):
    return _G.rod(a, b, r, mat, _sides(r, n, 6 if r < 0.008 else 8, 48))


def tube(points, r, mat, n=12, steps=8):
    return _G.tube(points, r, mat, _sides(r, n, 6 if r < 0.008 else 8, 40), steps)

TAG = 'ns_kit'
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {}          # filled by the builders (per material)
AO_DISTANCE = 0.05
REVIEW_VIEWS = None
REVIEW_SCREEN_LIGHT = False
_ONLY = [s.strip() for s in os.environ.get('NS_KIT_ONLY', '').split(',') if s.strip()]
UNBAKED_PREFIX = ('ns_glass', 'ns_screen', 'ns_leaf', 'ns_steam')

TRANSIT_GREEN = 0x0f6b3a
TRANSIT_GOLD = 0xf2b705
TRANSIT_WHITE = 0xeceeed
SIGN_GREEN = 0x0f6b3a
SIGN_RED = 0xb3121b
SIGN_WHITE = 0xf1f0ea
INK = 0x1b1d20
SIG_YELLOW = 0xe8b416
COOL_GLOW = 0xe2ecff
WARM_GLOW = 0xffc98a


def log(*a):
    print(f'[{TAG}]', *a, flush=True)


def tex(name, size):
    TEXTURE_SIZES[name] = size
    return name


# ================================================================== materials

def painted(name, hexc, size=1024, **kw):
    """Enamel on steel outdoors for years (street.painted, pushed further: more chips, rust, grime and streaks)."""
    tex(name, size)
    a = dict(rough=0.45, chips=0.8, rust=0.6, grime=0.8, grime_height=0.9, streaks=0.7)
    a.update(kw)
    return street.painted(name, hexc, **a)


def galv(name, size=1024, grime=0.6, grime_height=0.9):
    tex(name, size)
    return street.galvanized(name, grime=grime, grime_height=grime_height)


def plastic(name, hexc, size=512, rough=0.45, grime=0.4):
    tex(name, size)
    return street.shop_plastic(name, hexc, rough=rough, grime=grime)


def concrete_mat(name, hexc=0x9a968e, size=1024, grime=0.6):
    tex(name, size)
    m = street.concrete(name, hexc, 0.88)
    return m


def black_powder(name, size=1024, ground=0.25):
    """Black powder-coated steel: satin, orange peel, chips to steel, a splash of grime at the foot."""
    tex(name, size)
    m = core.Mat(name)
    tc, sep = object_coords(m)
    from artkit.shade import powder_coat
    col, rough, metal, bump = powder_coat(m, tc, 0x17191c, rough=0.5, chip_hex=0x6d7076, chips=0.7)
    gr = m.math('MULTIPLY', m.math('SUBTRACT', 1.0, m.math('DIVIDE', sep.outputs['Z'], ground), clamp=True),
                ramp(m, noise(m, tc.outputs['Object'], 8.0, 4.0), 0.3, 0.7))
    col = m.mix(m.math('MULTIPLY', gr, 0.7), col, H(0x5d5a54))
    m.set('Base Color', col)
    m.set('Roughness', mix_float(m, gr, rough, 0.8))
    m.set('Metallic', mix_float(m, gr, metal, 0.0))
    m.set('Normal', bump.outputs['Normal'])
    return m


def screen_material(name, image_path=None, hexc=0x10141a, emit=0.0):
    """An unbaked [screen] slot (a bpy material, shared by name): its default picture embedded (the game draws over it)."""
    ex = bpy.data.materials.get(name)
    if ex is not None:
        return ex
    m = core.Mat(name)
    if image_path:
        uvn = m.node('ShaderNodeUVMap')
        uvn.uv_map = 'UVMap'
        img = m.image(image_path, vector=uvn.outputs['UV'], extension='EXTEND')
        m.link(img.outputs['Color'], m.bsdf.inputs['Base Color'])
        if emit > 0:
            m.link(img.outputs['Color'], m.bsdf.inputs['Emission Color'])
            m.set('Emission Strength', emit)
    else:
        m.set('Base Color', H(hexc))
    m.set('Roughness', 0.3)
    m.m.use_backface_culling = True
    if m.m.name != name:
        raise RuntimeError(f'material name clash {m.m.name}')
    return m.m


def glass_material(name='ns_glass'):
    m = bpy.data.materials.get(name)
    if m is not None:
        return m
    m = core.Mat(name)
    m.set('Base Color', H(0xd8e2e6))
    m.set('Roughness', 0.03)
    m.set('Transmission Weight', 1.0)
    m.set('Alpha', 0.25)
    m.m.use_backface_culling = False
    return m.m


# ================================================================== prints

def sheet_px(s, name, px_w, samples=16):
    color, _ = s.render(name, px_w, samples=samples)
    img = bpy.data.images.load(color)
    w, h = img.size
    a = np.array(img.pixels[:], dtype=np.float32).reshape(h, w, 4)[::-1].copy()
    bpy.data.images.remove(img)
    for o in [o for o in bpy.data.objects if o.name.startswith(('rect', 'poly', 'text', 'sheet_cam'))]:
        bpy.data.objects.remove(o)
    return a


class Atlas:
    """Panels drawn one by one (Sheet) and pasted into rects (u0, v0, u1, v1) of one W x H image."""

    def __init__(self, name, w, h=None, ground=0.1):
        self.name, self.w, self.h = name, w, h or w
        self.img = np.zeros((self.h, self.w, 4), np.float32)
        self.img[..., 3] = 1.0
        self.img[..., :3] = ground
        self.k = 0

    def put(self, rect_uv, panel_fn, w, h, samples=16):
        u0, v0, u1, v1 = rect_uv
        pw = int(round((u1 - u0) * self.w))
        ph = int(round((v1 - v0) * self.h))
        s = Sheet(w, h)
        panel_fn(s, w, h)
        self.k += 1
        a = sheet_px(s, f'{self.name}_{self.k}', pw, samples)[:, :pw]
        if abs(a.shape[0] - ph) > 1:
            # the panel fills its rect whatever its aspect (the quad's UVs stretch it back): resample its rows, never crop
            t = np.linspace(0.0, a.shape[0] - 1.0, ph)
            i0 = np.floor(t).astype(np.int64)
            i1 = np.minimum(i0 + 1, a.shape[0] - 1)
            f = (t - i0)[:, None, None].astype(np.float32)
            a = a[i0] * (1.0 - f) + a[i1] * f
        a = a[:ph, :pw] if a.shape[0] >= ph else np.pad(a, ((0, ph - a.shape[0]), (0, 0), (0, 0)), mode='edge')
        r0 = self.h - int(round(v1 * self.h))
        c0 = int(round(u0 * self.w))
        bg = self.img[r0:r0 + a.shape[0], c0:c0 + a.shape[1], :3]
        self.img[r0:r0 + a.shape[0], c0:c0 + a.shape[1], :3] = a[..., :3] * a[..., 3:4] + bg * (1 - a[..., 3:4])
        return self

    def save(self):
        path = os.path.join(core.BUILD_DIR, 'textures', f'{self.name}.png')
        from ns_ground import lin2srgb  # noqa
        _png(path, self.img[..., :3])
        return path


def print_material(name, path, size=1024, lit_rects=(), strength=6.0, rough=0.35, reflective_rects=(), weather=0.5):
    """A prints slot: the atlas on the UVs, weathered (faded, grimy at the bottom, a sheen of dirt); inside lit_rects it
    glows (a backlit panel); retroreflective sheeting is smoother."""
    tex(name, size)
    m = core.Mat(name)
    uvn = m.node('ShaderNodeUVMap')
    uvn.uv_map = 'UVMap'
    img = m.image(path, vector=uvn.outputs['UV'], extension='EXTEND')
    tc, sep = object_coords(m)
    us = m.node('ShaderNodeSeparateXYZ')
    m.link(uvn.outputs['UV'], us.inputs['Vector'])
    u, v = us.outputs['X'], us.outputs['Y']

    def rect_mask(rects):
        mask = None
        for (u0, v0, u1, v1) in rects:
            r = m.math('MULTIPLY', m.math('MULTIPLY', m.math('GREATER_THAN', u, u0), m.math('LESS_THAN', u, u1)),
                       m.math('MULTIPLY', m.math('GREATER_THAN', v, v0), m.math('LESS_THAN', v, v1)))
            mask = r if mask is None else m.math('MAXIMUM', mask, r)
        return mask
    o = tc.outputs['Object']
    dirt = m.math('MULTIPLY', ramp(m, noise(m, o, 14.0, 4.0), 0.45, 0.8), weather)
    scratch = m.math('MULTIPLY', ramp(m, noise(m, scaled(m, sep, 300.0, 300.0, 6.0), 1.0, 2.0), 0.7, 0.75), weather * 0.6)
    col = img.outputs['Color']
    col = m.mix(m.math('MULTIPLY', dirt, 0.35), col, m.mix(1.0, col, H(0x8a8478), blend='MULTIPLY'))
    col = m.mix(scratch, col, H(0xcfcac0))
    m.set('Base Color', col)
    m.set('Roughness', m.math('ADD', rough, m.math('MULTIPLY', dirt, 0.25)))
    m.set('Specular IOR Level', 0.45)
    lm = rect_mask(lit_rects)
    if lm is not None:
        m.set('Emission Color', m.mix(1.0, img.outputs['Color'], vec(m, lm, lm, lm), blend='MULTIPLY'))
        m.set('Emission Strength', strength)
    return m


def print_quad(name, corners, uv_rect, u_axis, v_axis, origin, size, mat):
    """One printed quad: corners (4 points, outward winding), its atlas rect; u = (p - origin) . u_axis / size[0]."""
    bm = bmesh.new()
    vs = [bm.verts.new(c) for c in corners]
    bm.faces.new(vs)
    o = core.mesh_object(name, bm)
    core.assign(o, mat)
    u0, v0, u1, v1 = uv_rect
    O, U, V = Vector(origin), Vector(u_axis), Vector(v_axis)
    lock_uvs(o, lambda co: (u0 + (u1 - u0) * ((Vector(co) - O).dot(U) / size[0]), v0 + (v1 - v0) * ((Vector(co) - O).dot(V) / size[1])))
    return o


def face_quad(name, cx, cy, cz, w, h, normal, uv_rect, mat):
    """A printed rectangle w x h centered at (cx, cy, cz) facing `normal` (one of +-x, +-y), its text upright and reading
    left to right for a viewer in front of it."""
    nx, ny = normal
    # the viewer looks along -normal; their right = (-ny, nx) ... for normal -X (viewer at -X looking +X), right is -Y
    rx, ry = (ny, -nx)
    rx, ry = -rx, -ry
    hw, hh = w / 2, h / 2
    c = Vector((cx, cy, cz))
    R = Vector((rx, ry, 0.0))
    U = Vector((0.0, 0.0, 1.0))
    corners = [c - R * hw - U * hh, c + R * hw - U * hh, c + R * hw + U * hh, c - R * hw + U * hh]
    return print_quad(name, [tuple(p) for p in corners], uv_rect, tuple(R), (0, 0, 1), tuple(c - R * hw - U * hh), (w, h), mat)


def glow(objs, name):
    o = parts.glow_object(objs, name)
    o.data.materials[0].use_backface_culling = True
    log(f'{name} tris {tri_count(o)}')
    return o


SIGNAL_CELLS = {'red': 0, 'amber': 1, 'green': 2, 'walk': 3, 'hand': 4, 'countdown': 5}


def signal_uvs(obj, cell, center, right, up, half_w, half_h):
    """Maps obj's faces into the Signal grid's cell (4 x 2), planar from the front: u along `right`, v along `up`."""
    col, row = cell % 4, cell // 4
    u0, v0 = col * 0.25, row * 0.5
    du, dv = 0.25, 0.5
    m = 0.02
    C, R, U = Vector(center), Vector(right), Vector(up)
    me = obj.data
    uv = me.uv_layers.get('UVMap') or me.uv_layers.new(name='UVMap')
    for poly in me.polygons:
        for li in poly.loop_indices:
            p = me.vertices[me.loops[li].vertex_index].co - C
            a = 0.5 + p.dot(R) / (2 * half_w)
            b = 0.5 + p.dot(U) / (2 * half_h)
            uv.data[li].uv = (u0 + du * (m + (1 - 2 * m) * a), v0 + dv * (m + (1 - 2 * m) * b))
    return obj


# ================================================================== geometry helpers

def xf(objs, M):
    for o in (objs if isinstance(objs, (list, tuple)) else [objs]):
        o.data.transform(M)
    return objs


def flat_poly(name, pts, x=0.0, mat=None, frame=None):
    """A flat polygon given in (a, b) = (right, up) coordinates, placed by the 4x4 frame (columns: right, up, normal, origin)."""
    bm = bmesh.new()
    vs = [bm.verts.new((a, b, 0.0)) for a, b in pts]
    bm.faces.new(vs)
    o = core.mesh_object(name, bm)
    if frame is not None:
        o.data.transform(frame)
    if mat is not None:
        core.assign(o, mat)
    return o


def frame_facing(normal, origin):
    """A matrix taking (right, up, out) local coordinates to a face looking along `normal` (a horizontal unit vector)."""
    n = Vector(normal).normalized()
    up = Vector((0, 0, 1))
    right = up.cross(n).normalized() * -1.0   # for n = -X: right = -Y (the viewer's right)
    right = n.cross(up).normalized() * -1.0 if False else Vector((-n.y, n.x, 0.0)) * -1.0
    M = Matrix((right, up, n)).transposed().to_4x4()
    M.translation = Vector(origin)
    return M


def thick_line(a, b, w):
    """A quad (4 points) for a stroke of width w from a to b (2D)."""
    a = np.array(a, float)
    b = np.array(b, float)
    d = b - a
    d /= max(np.linalg.norm(d), 1e-9)
    n = np.array((-d[1], d[0])) * w / 2
    return [tuple(a - n), tuple(b - n), tuple(b + n), tuple(a + n)]


def circle2(cx, cy, r, n=20):
    return [(cx + r * math.cos(2 * math.pi * k / n), cy + r * math.sin(2 * math.pi * k / n)) for k in range(n)]


# ================================================================== SM_NS_StreetlightLED (+ _GlowCool)

LED_POLE_H = 8.4
LED_REACH = 2.15
LED_Z = 8.93


def led_head_material(name):
    """Die-cast aluminum in a grey powder coat; underneath, the optic panel: 4 x 12 LEDs in silver reflector cups."""
    tex(name, 1024)
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    from artkit.shade import powder_coat
    col, rough, metal, bump = powder_coat(m, tc, 0x6f747a, rough=0.42, chip_hex=0x9a9ea3, chips=0.4)
    streak = m.math('MULTIPLY', ramp(m, noise(m, scaled(m, sep, 40.0, 40.0, 3.0), 1.0, 3.0), 0.55, 0.8), 0.5)
    col = m.mix(streak, col, H(0x3d3c38))
    # the optic panel (the bottom face, z < LED_Z + 0.004): cups on a 37 x 35 mm grid
    under = m.math('LESS_THAN', z, LED_Z + 0.004)
    lx = m.math('SUBTRACT', x, -LED_REACH)
    gx = m.math('SUBTRACT', m.math('FRACT', m.math('DIVIDE', m.math('ADD', lx, 0.5), 0.037)), 0.5)
    gy = m.math('SUBTRACT', m.math('FRACT', m.math('DIVIDE', m.math('ADD', y, 0.5), 0.035)), 0.5)
    r = m.math('SQRT', m.math('ADD', m.math('MULTIPLY', gx, gx), m.math('MULTIPLY', gy, gy)))
    inpanel = m.math('MULTIPLY', smooth_less(m, m.math('ABSOLUTE', lx), 0.222, 0.002), smooth_less(m, m.math('ABSOLUTE', y), 0.072, 0.002))
    cup = m.math('MULTIPLY', smooth_less(m, r, 0.42, 0.04), inpanel)
    led = m.math('MULTIPLY', smooth_less(m, r, 0.14, 0.03), inpanel)
    pan = m.math('MULTIPLY', under, inpanel)
    col = m.mix(pan, col, H(0x1a1c1f))
    col = m.mix(m.math('MULTIPLY', cup, under), col, H(0xa9adb2))
    col = m.mix(m.math('MULTIPLY', led, under), col, H(0xe9e4c8))
    m.set('Base Color', col)
    m.set('Metallic', mix_float(m, m.math('MULTIPLY', cup, under), metal, 0.9))
    m.set('Roughness', mix_float(m, m.math('MULTIPLY', cup, under), rough, 0.15))
    m.set('Normal', bump.outputs['Normal'])
    return m


def pole_base(mat, r=0.2, h=0.62, flutes=16):
    base = core.lathe('base', [(0.0, 0.0), (r, 0.0), (r, 0.05), (r * 0.875, 0.09), (r * 0.75, h * 0.68), (r * 0.65, h * 0.8), (r * 0.6, h), (0.0, h)],
                      segments=64)
    for v in base.data.vertices:
        if 0.08 < v.co.z < h * 0.72:
            rr = v.co.xy.length
            if rr > 0.05:
                k = 1.0 + 0.045 * math.cos(math.atan2(v.co.y, v.co.x) * flutes)
                v.co.x *= k
                v.co.y *= k
    core.orient_normals(base)
    core.assign(base, mat)
    return base


def build_streetlight_led():
    pole = painted('nsk_led_pole', 0x2c3033, 2048, rough=0.5, chips=0.7, rust=0.7, grime=0.9, grime_height=0.9, streaks=0.8)
    head = led_head_material('nsk_led_head')
    out, lit = [], []
    pl = [(0.0, 0.0), (0.095, 0.0), (0.093, 1.0), (0.08, 3.8), (0.066, 6.6), (0.058, LED_POLE_H), (0.0, LED_POLE_H)]
    out.append(_G.lathe('pole', _round_profile(pl), pole, 40))
    out.append(pole_base(pole))
    out.append(lathe('collar', [(0.098, 0.6), (0.112, 0.61), (0.112, 0.66), (0.098, 0.67)], pole, 40))
    hatch = parts.rbox('hatch', (0.0, 0.0, 0.0), (0.012, 0.075, 0.16), 0.006)
    hatch.data.transform(Matrix.Translation((0.088, 0.0, 0.95)))
    core.assign(hatch, pole)
    out.append(hatch)
    for zz in (0.89, 1.01):
        out.append(parts.disc('screw', (0.096, 0.0, zz), (1, 0, 0), 0.006, 0.004, 6))
        core.assign(out[-1], pole)
    # the davit arm out of the pole top over toward -X, levelling out at the head
    arm = [(0.0, 0.0, LED_POLE_H - 0.3), (0.0, 0.0, LED_POLE_H + 0.05), (-0.08, 0.0, LED_POLE_H + 0.3), (-0.35, 0.0, LED_POLE_H + 0.47),
           (-0.9, 0.0, LED_Z + 0.06), (-1.55, 0.0, LED_Z + 0.07), (-1.80, 0.0, LED_Z + 0.07)]
    out.append(tube(arm, 0.032, pole, 16, 10))
    out.append(lathe('sleeve', [(0.06, LED_POLE_H - 0.02), (0.064, LED_POLE_H), (0.064, LED_POLE_H + 0.12), (0.036, LED_POLE_H + 0.18)], pole, 32))
    # the head: a flat cast housing, back (at the arm) to the nose, with a slipfitter clamp
    zb = LED_Z
    rings = []
    secs = [(-1.78, 0.13, 0.095), (-1.82, 0.16, 0.105), (-1.95, 0.17, 0.105), (-2.15, 0.17, 0.095), (-2.35, 0.155, 0.08), (-2.48, 0.12, 0.06),
            (-2.53, 0.08, 0.045), (-2.55, 0.04, 0.03)]
    # resample the sections along x (a smooth plan and profile), 64 points round each
    fine = []
    for i in range(len(secs) - 1):
        a, b = secs[i], secs[i + 1]
        for j in range(3):
            t = j / 3
            t2 = t * t * (3 - 2 * t)
            fine.append(tuple(a[q] + (b[q] - a[q]) * (t if q == 0 else t2) for q in range(3)))
    fine.append(secs[-1])
    for x, hw, hh in fine:
        ring = []
        for k in range(36):
            t = 2 * math.pi * k / 36
            cy, sy_ = math.cos(t), math.sin(t)
            yy = hw * math.copysign(abs(cy) ** 0.55, cy)
            zz = zb + (hh * (abs(sy_) ** 0.7) if sy_ > 0 else -0.004 * abs(sy_))
            ring.append((x, yy, zz))
        rings.append(ring)
    housing = core.loft('housing', rings)
    core.orient_normals(housing)
    core.assign(housing, head)
    out.append(housing)
    # the parting line: a slim lip where the hinged lower door meets the upper casting
    for sy in (-1, 1):
        pts = [(x, sy * (hw * 1.0 + 0.002), zb + 0.006) for x, hw, hh in fine[1:-1]]
        out.append(tube(pts, 0.004, head, 8, 2))
    # cooling fins on the top, thin and tapered at the ends
    for k in range(13):
        yy = -0.108 + 0.018 * k
        top = zb + 0.105 - 0.012 * abs(k - 6) / 6
        fin = prism_y('fin', [(-2.3, zb + 0.06), (-1.88, zb + 0.06), (-1.9, top), (-2.27, top)], yy - 0.0022, yy + 0.0022, head)
        out.append(fin)
    # the optic panel: a bezel round 4 x 12 LED optics (small silicone domes) under the flat bottom
    px0, px1, py0, py1 = -LED_REACH - 0.225, -LED_REACH + 0.225, -0.075, 0.075
    for (a0, a1, b0, b1) in ((px0 - 0.012, px1 + 0.012, py0 - 0.012, py0), (px0 - 0.012, px1 + 0.012, py1, py1 + 0.012),
                             (px0 - 0.012, px0, py0, py1), (px1, px1 + 0.012, py0, py1)):
        out.append(box(a0, a1, b0, b1, zb - 0.008, zb - 0.003, head, 0.002, 2))
    for i in range(12):
        for j in range(4):
            cx = -LED_REACH + 0.037 * (i - 5.5)
            cy = 0.035 * (j - 1.5)
            out.append(_G.lathe('optic', [(0.0, 0.0), (0.012, 0.0), (0.009, -0.004), (0.0, -0.0052)], head, 8,
                                Matrix.Translation((cx, cy, zb - 0.0035))))
    # two latches at the nose's underside, the door's hinge pin at the back
    for yy in (-0.06, 0.06):
        out.append(box(-2.47, -2.43, yy - 0.012, yy + 0.012, zb - 0.012, zb - 0.002, head, 0.003, 2))
    out.append(rod((-1.86, -0.12, zb - 0.002), (-1.86, 0.12, zb - 0.002), 0.006, head, 16))
    # the slipfitter: a clamp barrel round the arm's end, two bolts
    out.append(rod((-1.70, 0.0, zb + 0.07), (-1.84, 0.0, zb + 0.07), 0.042, head, 20))
    for xx in (-1.73, -1.80):
        out.append(rod((xx, -0.05, zb + 0.07), (xx, 0.05, zb + 0.07), 0.008, head, 8))
    # the NEMA photocell on top, toward the back
    out.append(lathe('cell_base', [(0.0, zb + 0.095), (0.05, zb + 0.095), (0.05, zb + 0.105), (0.0, zb + 0.105)], head, 24,
                     Matrix.Translation((-2.02, 0.0, 0.0))))
    out.append(lathe('cell', [(0.0, zb + 0.105), (0.042, zb + 0.105), (0.042, zb + 0.16), (0.036, zb + 0.175), (0.0, zb + 0.18)], head, 24,
                     Matrix.Translation((-2.02, 0.0, 0.0))))
    # banner arms (two, for the block association's banners) on the pole's +-Y sides, ball ends
    for zz in (4.6, 6.4):
        r_at = 0.093 - 0.0049 * (zz - 1.0)
        out.append(lathe('band', [(r_at, zz - 0.04), (r_at + 0.012, zz - 0.04), (r_at + 0.012, zz + 0.04), (r_at, zz + 0.04)], pole, 48))
        out.append(rod((0.0, r_at + 0.01, zz), (0.0, 0.62, zz), 0.016, pole, 24))
        out.append(lathe('ball', [(0.0, -0.025), (0.018, -0.02), (0.025, 0.0), (0.018, 0.02), (0.0, 0.025)], pole, 24,
                         Matrix.Translation((0.0, 0.64, zz))))
    # the pole's ID tag and the hand hole's frame
    out.append(box(0.088, 0.096, -0.035, 0.035, 2.1, 2.16, pole, 0.002, 2))
    out.append(box(0.084, 0.094, -0.048, 0.048, 0.85, 1.05, pole, 0.004, 3))
    # the optic panel's glow, just under the optics
    lit.append(box(-LED_REACH - 0.22, -LED_REACH + 0.22, -0.07, 0.07, zb - 0.0105, zb - 0.0088, None))
    return [finish_mesh(out, 'SM_NS_StreetlightLED', [pole, head], 35.0, tag=TAG), glow(lit, 'SM_NS_StreetlightLED_GlowCool')]


# ================================================================== signals

def heads_material(name):
    """Signal and ped heads: black polycarbonate housings and visors, black aluminum backplates with a yellow retroreflective
    border; lenses (when dark) smoked glass with LED dimples; dust on the tops, bird marks."""
    tex(name, 2048)
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    kind = attr(m, 'nsk_part')   # 0 housing, 1 backplate border, 2 lens red, 3 amber, 4 green, 5 ped lens, 6 button, 7 white
    g = noise(m, o, 900.0, 1.0)
    dust = m.math('MULTIPLY', ramp(m, noise(m, o, 18.0, 4.0), 0.45, 0.8), 0.45)
    base = m.mix(m.math('MULTIPLY', g, 0.3), H(0x131416), H(0x1c1d20))
    col = m.mix(dust, base, H(0x4a4740))

    def is_k(k):
        return m.math('LESS_THAN', m.math('ABSOLUTE', m.math('SUBTRACT', kind, float(k))), 0.5)
    border = is_k(1)
    # retroreflective sheeting: a fine prismatic grid in the roughness, a little worn and dirty
    prism = ramp(m, noise(m, o, 600.0, 1.0), 0.4, 0.6)
    ycol = m.mix(m.math('MULTIPLY', dust, 0.6), H(SIG_YELLOW), H(0x8a7a3a))
    col = m.mix(border, col, ycol)
    lens_k = [(2, 0x2a0907), (3, 0x2c1905), (4, 0x06231a), (5, 0x1f1b16)]
    lensm = None
    for k, hx in lens_k:
        mk = is_k(k)
        col = m.mix(mk, col, H(hx))
        lensm = mk if lensm is None else m.math('MAXIMUM', lensm, mk)
    btn = is_k(6)
    col = m.mix(btn, col, H(0x8d9196))
    white = is_k(7)
    col = m.mix(white, col, H(0xe9e7e1))
    m.set('Base Color', col)
    r = m.math('ADD', 0.5, m.math('MULTIPLY', g, 0.1))
    r = mix_float(m, border, r, m.math('ADD', 0.35, m.math('MULTIPLY', prism, 0.1)))
    r = mix_float(m, lensm, r, 0.08)
    r = mix_float(m, btn, r, 0.3)
    m.set('Roughness', r)
    m.set('Metallic', mix_float(m, btn, 0.0, 0.9))
    set_bump(m, m.math('ADD', m.math('MULTIPLY', g, 0.3), m.math('MULTIPLY', prism, m.math('MULTIPLY', border, 0.4))), 0.2, 0.0005)
    return m


def tagged(o, k):
    set_attr(o, 'nsk_part', k)
    return o


def shell_loft(name, rings, n_outer):
    """A loft through rings that are a thin open section's outline (n_outer points out, the same points back in reverse):
    the ends closed with quads between the two walls (no concave n-gon caps)."""
    bm = bmesh.new()
    verts = [[bm.verts.new(p) for p in ring] for ring in rings]
    m = len(rings[0])
    for a, b in zip(verts, verts[1:]):
        for k in range(m):
            bm.faces.new((a[k], a[(k + 1) % m], b[(k + 1) % m], b[k]))
    for ring, flip in ((verts[0], True), (verts[-1], False)):
        for k in range(n_outer - 1):
            q = (ring[k], ring[k + 1], ring[m - 2 - k], ring[m - 1 - k])
            bm.faces.new(tuple(reversed(q)) if flip else q)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    return core.mesh_object(name, bm)


def rivet(mat, at, axis_m, r=0.006, k=0):
    """A round-head rivet seated at `at`, its axis +Z turned by axis_m (a 4x4 rotation)."""
    return tagged(_G.lathe('rivet', [(0.0, 0.0), (r, 0.0), (r * 0.7, r * 0.6), (0.0, r * 0.8)], mat, 8, Matrix.Translation(Vector(at)) @ axis_m), k)


def vehicle_head(mat, lit, glow_list, cells=('red', 'amber', 'green'), backplate=True):
    """A 12-inch 3-section head built facing -X, its origin at the top of the housing's back center. Returns parts; appends
    the lens glow discs (UV'd into the Signal grid) to glow_list. Each section is its own die-cast housing (a seam between
    them), with a hinged door (two hinge knuckles on the -Y side, a wing-screw latch on the +Y side), a shallow LED lens in a
    rubber gasket, and a tunnel visor drooping a little at its mouth; tri-stud bosses top and bottom; a backplate with a 50
    mm yellow retroreflective border, riveted, on two stiffeners."""
    out = []
    W, D, Hs = 0.36, 0.25, 0.355
    n = len(cells)
    Ht = Hs * n
    face = rot_y(-90)           # +Z -> -X: lathed parts that face the street
    for i, cname in enumerate(cells):
        zc = -Hs * (i + 0.5)
        hous = parts.rbox('housing', (-D / 2, 0.0, zc), (D, W, Hs - 0.005), 0.03, 3)
        core.assign(hous, mat)
        out.append(tagged(hous, 0))
        # the door's raised frame, its hinge knuckles and latch
        door = parts.rbox('door', (-D - 0.008, 0.0, zc), (0.016, W - 0.02, Hs - 0.02), 0.012, 2)
        core.assign(door, mat)
        out.append(tagged(door, 0))
        for dz in (-0.11, 0.11):
            out.append(tagged(rod((-D - 0.004, -W / 2 + 0.004, zc + dz - 0.03), (-D - 0.004, -W / 2 + 0.004, zc + dz + 0.03), 0.009, mat, 16), 0))
        out.append(tagged(box(-D - 0.022, -D - 0.006, W / 2 - 0.03, W / 2 - 0.004, zc - 0.025, zc + 0.025, mat, 0.004, 3), 0))
        out.append(tagged(lathe('wing', [(0.0, 0.0), (0.008, 0.0), (0.008, 0.012), (0.0, 0.014)], mat, 16,
                                Matrix.Translation((-D - 0.022, W / 2 - 0.017, zc)) @ face), 0))
        out.append(tagged(box(-D - 0.04, -D - 0.034, W / 2 - 0.02, W / 2 - 0.014, zc - 0.018, zc + 0.018, mat, 0.002, 2), 0))
        # the lens: a shallow dome in a rubber gasket
        lens = _G.lathe('lens', [(0.0, 0.0), (0.15, 0.0), (0.15, 0.0025), (0.12, 0.0055), (0.0, 0.008)], mat, 32,
                        Matrix.Translation((-D - 0.016, 0.0, zc)) @ face)
        out.append(tagged(lens, {'red': 2, 'amber': 3, 'green': 4}[cname]))
        out.append(tagged(_G.lathe('gasket', [(0.15, 0.0), (0.162, 0.0), (0.158, 0.005), (0.15, 0.004)], mat, 32,
                                   Matrix.Translation((-D - 0.016, 0.0, zc)) @ face), 0))
        # tunnel visor: 3/4 of a tube, open at the bottom, drooping 1 cm at the mouth
        prof = []
        for k in range(29):
            a = math.radians(-45 + 270 * k / 28)
            prof.append((0.17 * math.cos(a), 0.17 * math.sin(a)))
        inner = [(0.164 * math.cos(math.atan2(b, a)), 0.164 * math.sin(math.atan2(b, a))) for a, b in reversed(prof)]
        sec = prof + inner
        rings = []
        for t in (0.0, 0.5, 1.0):
            xx = -D - 0.02 - 0.24 * t
            rings.append([(xx, a, zc + b - 0.012 * t * t * (b > 0)) for a, b in sec])
        vis = shell_loft('visor', rings, len(prof))
        core.orient_normals(vis)
        core.assign(vis, mat)
        out.append(tagged(vis, 0))
        # the visor's mounting tabs (four screws round its root)
        for a in (30, 150, 210, 330):
            ra = math.radians(a)
            out.append(tagged(_G.lathe('vscrew', [(0.0, 0.0), (0.006, 0.0), (0.0, 0.004)], mat, 6,
                                       Matrix.Translation((-D - 0.016, 0.172 * math.cos(ra), zc + 0.172 * math.sin(ra))) @ face), 0))
        gl = parts.disc('glow', (-D - 0.026, 0.0, zc), (1, 0, 0), 0.142, 0.002, 48)
        signal_uvs(gl, SIGNAL_CELLS[cname], (-D - 0.027, 0.0, zc), (0, -1, 0), (0, 0, 1), 0.142, 0.142)
        glow_list.append(gl)
    # the tri-stud bosses and the hanger plates top and bottom
    for zz, sgn in ((0.0, 1), (-Ht, -1)):
        out.append(tagged(_G.lathe('boss', [(0.0, 0.0), (0.07, 0.0), (0.065, 0.02), (0.05, 0.03), (0.0, 0.03)], mat, 24,
                                Matrix.Translation((-D / 2, 0.0, zz)) @ (Matrix() if sgn > 0 else rot_y(180))), 0))
        for a in (90, 210, 330):
            ra = math.radians(a)
            out.append(tagged(lathe('stud', [(0.0, 0.0), (0.008, 0.0), (0.008, 0.022), (0.0, 0.024)], mat, 6,
                                    Matrix.Translation((-D / 2 + 0.045 * math.cos(ra), 0.045 * math.sin(ra), zz + sgn * 0.028))
                                    @ (Matrix() if sgn > 0 else rot_y(180))), 0))
    if backplate:
        bw, bh, bd = 0.72, Ht + 0.32, 0.004
        bp = box(0.004, 0.004 + bd, -bw / 2, bw / 2, -Ht - 0.16, 0.16, mat, 0.0015, 2)
        out.append(tagged(bp, 0))
        # the yellow border on its front, 50 mm, riveted
        for (y0, y1, z0, z1) in ((-bw / 2, bw / 2, 0.11, 0.16), (-bw / 2, bw / 2, -Ht - 0.16, -Ht - 0.11), (-bw / 2, -bw / 2 + 0.05, -Ht - 0.11, 0.11),
                                 (bw / 2 - 0.05, bw / 2, -Ht - 0.11, 0.11)):
            out.append(tagged(box(-0.0005, 0.004, y0, y1, z0, z1, mat), 1))
        for k in range(5):
            z = 0.135 - (Ht + 0.27) * k / 4
            for y in (-bw / 2 + 0.025, bw / 2 - 0.025):
                out.append(rivet(mat, (-0.0005, y, z), face, 0.005, 1))
        for k in range(1, 4):
            y = -bw / 2 + bw * k / 4
            for z in (0.135, -Ht - 0.135):
                out.append(rivet(mat, (-0.0005, y, z), face, 0.005, 1))
        # the backplate's stiffening brackets to the housing, bolted
        for zz in (-0.05, -Ht + 0.05):
            out.append(tagged(box(0.0, 0.03, -0.2, 0.2, zz - 0.01, zz + 0.01, mat, 0.003, 2), 0))
            for yy in (-0.17, 0.17):
                out.append(tagged(_G.lathe('bolt', [(0.0, 0.0), (0.009, 0.0), (0.009, 0.007), (0.0, 0.008)], mat, 6,
                                           Matrix.Translation((-0.0005, yy, zz)) @ face), 0))
    return out


def rot_z(deg):
    return Matrix.Rotation(math.radians(deg), 4, 'Z')


def pole_shaft(mat, h, r0, r1, seg=40):
    return lathe('pole', [(0.0, 0.0), (r0, 0.0), (r0 * 0.99, 0.5), (r1, h), (0.0, h)], mat, seg)


def base_plate(mat, r_pole, plate=0.42):
    out = [box(-plate / 2, plate / 2, -plate / 2, plate / 2, 0.0, 0.03, mat, 0.004, 1)]
    for sx in (-1, 1):
        for sy in (-1, 1):
            c = (sx * plate * 0.36, sy * plate * 0.36)
            out.append(lathe('nut', [(0.0, 0.03), (0.022, 0.03), (0.022, 0.055), (0.018, 0.06), (0.0, 0.06)], mat, 6, Matrix.Translation((c[0], c[1], 0))))
            out.append(lathe('stud', [(0.0, 0.06), (0.012, 0.06), (0.012, 0.085), (0.0, 0.088)], mat, 10, Matrix.Translation((c[0], c[1], 0))))
    # a skirt casting round the pole's foot
    out.append(lathe('skirt', [(0.0, 0.03), (r_pole + 0.05, 0.03), (r_pole + 0.02, 0.12), (r_pole + 0.005, 0.16), (0.0, 0.16)], mat, 40))
    return out


def pole_details(mat, r, z_hand=0.55, facing=1.0):
    """A hand hole (a raised frame and a screwed cover) on the pole's +X (facing=1) or -X side, an ID tag, a ground lug."""
    out = []
    sx = facing
    x0 = sx * (r - 0.004)
    out.append(box(min(x0, x0 + sx * 0.018), max(x0, x0 + sx * 0.018), -0.06, 0.06, z_hand - 0.11, z_hand + 0.11, mat, 0.006, 3))
    out.append(box(min(x0, x0 + sx * 0.024), max(x0, x0 + sx * 0.024), -0.048, 0.048, z_hand - 0.095, z_hand + 0.095, mat, 0.004, 3))
    rm = rot_y(90 * sx)
    for dz in (-0.075, 0.075):
        out.append(lathe('screw', [(0.0, 0.0), (0.007, 0.0), (0.0065, 0.003), (0.0, 0.004)], mat, 12,
                         Matrix.Translation((x0 + sx * 0.024, 0.0, z_hand + dz)) @ rm, fillet=0.0))
    out.append(box(min(x0, x0 + sx * 0.008), max(x0, x0 + sx * 0.008), -0.04, 0.04, 1.9, 1.96, mat, 0.002, 2))
    out.append(lathe('lug', [(0.0, 0.0), (0.012, 0.0), (0.012, 0.02), (0.0, 0.022)], mat, 6,
                     Matrix.Translation((x0, 0.0, 0.22)) @ rm))
    return out


def build_signal_mast(variant='A'):
    name = 'SM_NS_SignalMast' if variant == 'A' else 'SM_NS_SignalMast_B'
    tg = 'nsk_sig' if variant == 'A' else 'nsk_sigb'
    pole = painted(f'{tg}_pole', 0x2c3330, 2048, rough=0.45, chips=0.9, rust=0.4, grime=0.8, grime_height=1.0, streaks=0.8, primer_hex=0x9ea3a6)
    hm = heads_material(f'{tg}_heads')
    out, lit = [], []
    out.append(pole_shaft(pole, 7.0, 0.165, 0.11, 96))
    out += base_plate(pole, 0.165, 0.5)
    out += pole_details(pole, 0.163, 0.55, 1.0)
    out.append(lathe('cap', [(0.0, 7.0), (0.12, 7.0), (0.12, 7.03), (0.08, 7.07), (0.0, 7.09)], pole, 64))
    # the arm: a tapered tube from the pole at 6.0 m out 6.5 m along -X, rising (camber) to 6.25
    rings = []
    NR = 13
    for i in range(NR):
        t = i / (NR - 1)
        x = -0.1 - 6.5 * t
        z = 6.0 + 0.25 * (1 - (1 - t) ** 1.6)
        r = 0.105 - 0.045 * t
        rings.append([(x, r * math.cos(2 * math.pi * k / 48), z + r * math.sin(2 * math.pi * k / 48)) for k in range(48)])
    arm = core.loft('arm', rings)
    core.orient_normals(arm)
    core.assign(arm, pole)
    out.append(arm)
    out.append(lathe('armcap', [(0.0, 0.0), (0.064, 0.0), (0.064, 0.014), (0.03, 0.024), (0.0, 0.026)], pole, 48,
                     Matrix.Translation((-6.6, 0, 6.25)) @ rot_y(-90)))
    # the arm's flange plate (two plates bolted face to face) and gussets on the pole
    out.append(box(-0.2, -0.17, -0.17, 0.17, 5.78, 6.22, pole, 0.006, 3))
    out.append(box(-0.23, -0.2, -0.16, 0.16, 5.79, 6.21, pole, 0.006, 3))
    for zz in (5.84, 6.16):
        for yy in (-0.12, 0.12):
            out.append(lathe('bolt', [(0.0, 0.0), (0.02, 0.0), (0.02, 0.022), (0.0, 0.026)], pole, 6, Matrix.Translation((-0.23, yy, zz)) @ rot_y(-90)))
            out.append(lathe('washer', [(0.0, -0.004), (0.03, -0.004), (0.03, 0.0), (0.0, 0.0)], pole, 24,
                             Matrix.Translation((-0.23, yy, zz)) @ rot_y(-90)))
    out.append(prism_y('gusset', [(-0.23, 5.72), (-0.55, 6.0), (-0.23, 6.0)], -0.009, 0.009, pole))
    # the heads
    spots = [(-6.2, 1 if variant == 'A' else -1), (-4.5, -1)]
    for x, face in spots:
        t = (-0.1 - x) / 6.5
        z_arm = 6.0 + 0.25 * (1 - (1 - t) ** 1.6)
        r_arm = 0.105 - 0.045 * t
        hl = []
        hp = vehicle_head(hm, hl, hl, ('red', 'amber', 'green'))
        # face +Y: turn -90 deg from -X; face -Y: +90
        M = Matrix.Translation((x, -0.125 * face, z_arm - 0.3)) @ rot_z(-90.0 if face > 0 else 90.0)
        xf(hp + hl, M)
        out += hp
        lit += hl
        # a rigid mount: a pipe from the head's top boss to a saddle clamp on the arm, two U-bolts round the arm
        out.append(tagged(rod((x, 0.0, z_arm - 0.27), (x, 0.0, z_arm - r_arm + 0.01), 0.032, hm, 32), 0))
        out.append(tagged(box(x - 0.09, x + 0.09, -0.06, 0.06, z_arm - r_arm - 0.02, z_arm - r_arm + 0.012, hm, 0.004, 3), 0))
        for dx in (-0.065, 0.065):
            ub = [(x + dx, -0.05, z_arm - r_arm - 0.02)]
            for k in range(13):
                a = math.pi * k / 12
                ub.append((x + dx, -(r_arm + 0.008) * math.cos(a), z_arm + (r_arm + 0.008) * math.sin(a) * 0.98 - 0.0))
            ub.append((x + dx, 0.05, z_arm - r_arm - 0.02))
            out.append(tagged(tube(ub, 0.006, hm, 12, 2), 0))
            for yy in (-0.05, 0.05):
                out.append(tagged(lathe('nut', [(0.0, 0.0), (0.011, 0.0), (0.011, 0.01), (0.0, 0.01)], hm, 6,
                                        Matrix.Translation((x + dx, yy, z_arm - r_arm - 0.03))), 0))
        # the drip loop: the head's cable out of the arm, sagging, into the top boss
        out.append(tagged(tube([(x + 0.25, 0.0, z_arm - r_arm * 0.6), (x + 0.22, 0.03, z_arm - 0.2), (x + 0.12, 0.05, z_arm - 0.3),
                                (x + 0.04, 0.03, z_arm - 0.26), (x + 0.015, 0.0, z_arm - 0.25)], 0.009, hm, 12, 8), 0))
    # the transform: glow discs placed with their heads (they were built at the origin and moved with them)
    gl = glow(lit, f'{name}_GlowSignal')
    return [finish_mesh(out, name, [pole, hm], 35.0, tag=TAG), gl]


def rot_y(deg):
    return Matrix.Rotation(math.radians(deg), 4, 'Y')


def build_signal_pedestal():
    name = 'SM_NS_SignalPedestal'
    pole = painted('nsk_ped_pole', 0x2c3330, 1024, rough=0.45, chips=0.9, rust=0.4, grime=0.9, grime_height=0.8, streaks=0.7, primer_hex=0x9ea3a6)
    hm = heads_material('nsk_ped_heads')
    out, lit = [], []
    # a cast aluminum transformer base: a plinth, a tapered body with rounded corners, a door with two screws, the collar
    out.append(box(-0.23, 0.23, -0.23, 0.23, 0.0, 0.05, pole, 0.012, 3))

    def rsq(h, z, rc=0.03, k=5):
        pts = []
        for cx, cy, a0 in ((h, h, 0), (-h, h, 90), (-h, -h, 180), (h, -h, 270)):
            for j in range(k):
                a = math.radians(a0 + 90 * j / (k - 1))
                pts.append((cx - math.copysign(rc, cx) + rc * math.cos(a), cy - math.copysign(rc, cy) + rc * math.sin(a), z))
        return pts
    base = core.loft('tbase', [rsq(0.205, 0.05), rsq(0.2, 0.09), rsq(0.17, 0.55), rsq(0.165, 0.6)])
    core.orient_normals(base)
    core.assign(base, pole)
    out.append(base)
    out.append(box(-0.2, -0.17, -0.1, 0.1, 0.15, 0.46, pole, 0.008, 3))
    for dz in (0.19, 0.42):
        out.append(lathe('screw', [(0.0, 0.0), (0.008, 0.0), (0.0075, 0.004), (0.0, 0.005)], pole, 12,
                         Matrix.Translation((-0.2, 0.0, dz)) @ rot_y(-90), fillet=0.0))
    out.append(_G.lathe('collar', [(0.0, 0.6), (0.11, 0.6), (0.11, 0.63), (0.09, 0.66), (0.08, 0.7), (0.0, 0.7)], pole, 40))
    for a in range(0, 360, 90):
        ra = math.radians(a + 45)
        out.append(lathe('setscrew', [(0.0, 0.0), (0.01, 0.0), (0.01, 0.012), (0.0, 0.014)], pole, 6,
                         Matrix.Translation((0.1 * math.cos(ra), 0.1 * math.sin(ra), 0.66)) @ Matrix.Rotation(ra, 4, 'Z') @ rot_y(90)))
    out.append(_G.lathe('pole', [(0.0, 0.6), (0.075, 0.6), (0.06, 3.5), (0.0, 3.5)], pole, 40))
    out.append(box(-0.075, -0.068, -0.04, 0.04, 1.9, 1.96, pole, 0.002, 2))
    hl = []
    hp = vehicle_head(hm, hl, hl, ('red', 'amber', 'green'), backplate=False)
    xf(hp + hl, Matrix.Translation((0.125, 0.0, 3.5 + 0.16 + 1.065 + 0.05)))
    out += hp
    lit += hl
    # the pole-top mount: a slipfitter over the pole's top up into the head's bottom boss, its set screws
    out.append(tagged(_G.lathe('slip', [(0.0, 3.45), (0.075, 3.45), (0.075, 3.6), (0.06, 3.64), (0.05, 3.66), (0.05, 3.69), (0.0, 3.69)], hm, 32), 0))
    for a in (0, 120, 240):
        ra = math.radians(a)
        out.append(tagged(lathe('set', [(0.0, 0.0), (0.009, 0.0), (0.009, 0.014), (0.0, 0.016)], hm, 6,
                                Matrix.Translation((0.075 * math.cos(ra), 0.075 * math.sin(ra), 3.52)) @ Matrix.Rotation(ra, 4, 'Z') @ rot_y(90)), 0))
    gl = glow(lit, f'{name}_GlowSignal')
    return [finish_mesh(out, name, [pole, hm], 35.0, tag=TAG), gl]


def hand_shape():
    """The raised hand (left half of the ped head): palm, four fingers, thumb, in (right, up) meters about its center."""
    polys = [[(-0.06, -0.09), (0.06, -0.09), (0.065, 0.02), (-0.06, 0.02)]]
    for k, (x, ln) in enumerate(((-0.048, 0.085), (-0.016, 0.1), (0.016, 0.095), (0.048, 0.075))):
        polys.append([(x - 0.013, 0.0), (x + 0.013, 0.0), (x + 0.013, ln), (x, ln + 0.012), (x - 0.013, ln)])
    polys.append(thick_line((0.055, -0.04), (0.105, 0.02), 0.026))
    return polys


def walk_shape():
    """The walking person (right half), in (right, up) meters about its center."""
    polys = [circle2(0.0, 0.085, 0.022, 16)]
    polys.append(thick_line((0.0, 0.055), (-0.01, -0.02), 0.032))
    polys.append(thick_line((-0.01, -0.02), (-0.045, -0.1), 0.022))
    polys.append(thick_line((-0.01, -0.02), (0.035, -0.1), 0.022))
    polys.append(thick_line((0.0, 0.045), (-0.04, 0.0), 0.018))
    polys.append(thick_line((0.0, 0.045), (0.045, 0.01), 0.018))
    return polys


def ped_head(mat, glow_list):
    """A two-section ped head built facing -X: top section the hand (left) and the walking person (right) side by side,
    bottom section the countdown; origin at the housing's top back center. 0.46 x 0.24 x 0.9 m."""
    out = []
    W, D, Hs = 0.46, 0.24, 0.45
    face = rot_y(-90)
    for i in range(2):
        zc = -Hs * (i + 0.5)
        hous = parts.rbox('housing', (-D / 2, 0.0, zc), (D, W, Hs - 0.005), 0.025, 3)
        core.assign(hous, mat)
        out.append(tagged(hous, 0))
        door = parts.rbox('door', (-D - 0.006, 0.0, zc), (0.012, W - 0.02, Hs - 0.02), 0.01, 2)
        core.assign(door, mat)
        out.append(tagged(door, 0))
        for dz in (-0.13, 0.13):
            out.append(tagged(rod((-D - 0.003, -W / 2 + 0.004, zc + dz - 0.03), (-D - 0.003, -W / 2 + 0.004, zc + dz + 0.03), 0.008, mat, 16), 0))
        out.append(tagged(box(-D - 0.02, -D - 0.006, W / 2 - 0.03, W / 2 - 0.004, zc - 0.025, zc + 0.025, mat, 0.004, 3), 0))
        out.append(tagged(lathe('wing', [(0.0, 0.0), (0.008, 0.0), (0.008, 0.012), (0.0, 0.014)], mat, 16,
                                Matrix.Translation((-D - 0.02, W / 2 - 0.017, zc)) @ face), 0))
        lens = _G.box(-D - 0.016, -D - 0.008, -0.19, 0.19, zc - 0.19, zc + 0.19, mat, 0.004, 2)
        out.append(tagged(lens, 5))
        # the hood round the lens (top and sides) and the egg-crate louver inside it: thin slats both ways
        for (y0, y1, z0, z1) in ((-0.205, 0.205, zc + 0.19, zc + 0.205), (-0.205, -0.19, zc - 0.19, zc + 0.19), (0.19, 0.205, zc - 0.19, zc + 0.19)):
            out.append(tagged(_G.box(-D - 0.14, -D - 0.006, y0, y1, z0, z1, mat, 0.002, 1), 0))
        for k in range(1, 8):
            yy = -0.19 + 0.38 * k / 8
            out.append(tagged(box(-D - 0.065, -D - 0.022, yy - 0.0012, yy + 0.0012, zc - 0.19, zc + 0.19, mat), 0))
            zz = zc - 0.19 + 0.38 * k / 8
            out.append(tagged(box(-D - 0.065, -D - 0.022, -0.19, 0.19, zz - 0.0012, zz + 0.0012, mat), 0))
        for (y0, y1, z0, z1) in ((-0.19, 0.19, zc - 0.19, zc - 0.186), (-0.19, 0.19, zc + 0.186, zc + 0.19)):
            out.append(tagged(box(-D - 0.065, -D - 0.022, y0, y1, z0, z1, mat), 0))
    # glow: hand on the viewer's left (+Y for a face looking -X), walk on the right, countdown below
    R = Vector((0, -1, 0))
    Up = Vector((0, 0, 1))
    xg = -D - 0.018
    for shape, cell, cy in ((hand_shape(), 'hand', 0.095), (walk_shape(), 'walk', -0.095)):
        objs = []
        for p in shape:
            frame = Matrix(((0, 0, 1, xg), (-1, 0, 0, cy), (0, 1, 0, -Hs * 0.5), (0, 0, 0, 1)))
            o = flat_poly('sym', [(a * 0.85, b * 0.85) for a, b in p], frame=frame)
            core.orient_normals(o, (-1, 0, 0))
            objs.append(o)
        sym = core.join(objs, 'sym_' + cell) if len(objs) > 1 else objs[0]
        signal_uvs(sym, SIGNAL_CELLS[cell], (xg, cy, -Hs * 0.5), R, Up, 0.11, 0.11)
        glow_list.append(sym)
    cd = box(xg - 0.001, xg, -0.15, 0.15, -1.5 * Hs - 0.12, -1.5 * Hs + 0.12, None)
    signal_uvs(cd, SIGNAL_CELLS['countdown'], (xg, 0.0, -1.5 * Hs), R, Up, 0.15, 0.12)
    glow_list.append(cd)
    return out


def ped_prints():
    at = Atlas('nsk_pb_prints', 512, 1024, ground=0.95)

    def push(s, w, h):
        s.rect(0, 0, w, h, SIGN_WHITE)
        s.rect(6, 6, w - 12, h - 12, 0x1b1d20)
        s.rect(12, 12, w - 24, h - 24, SIGN_WHITE)
        s.text('PUSH', w / 2, h * 0.82, w * 0.2, INK, 'Black', 'CENTER')
        s.text('BUTTON', w / 2, h * 0.7, w * 0.15, INK, 'Black', 'CENTER')
        s.text('FOR', w / 2, h * 0.6, w * 0.12, INK, 'Bold', 'CENTER')
        # the walking person
        cx, cy = w * 0.32, h * 0.38
        s.circle(cx, cy + 62, 14, INK)
        for a, b, wd in (((0, 45), (-4, -5), 20), ((-4, -5), (-26, -58), 14), ((-4, -5), (24, -58), 14), ((0, 38), (-26, 8), 11),
                         ((0, 38), (26, 14), 11)):
            q = thick_line((cx + a[0], cy + a[1]), (cx + b[0], cy + b[1]), wd)
            s.poly(q, INK)
        s.text('WALK', w * 0.62, h * 0.35, w * 0.13, INK, 'Black', 'CENTER')
        s.text('SIGNAL', w * 0.62, h * 0.27, w * 0.11, INK, 'Black', 'CENTER')
        s.poly([(w * 0.25, h * 0.14), (w * 0.62, h * 0.14), (w * 0.62, h * 0.09), (w * 0.78, h * 0.165), (w * 0.62, h * 0.24), (w * 0.62, h * 0.19),
                (w * 0.25, h * 0.19)], INK)
    at.put((0.0, 0.0, 1.0, 1.0), push, 230.0, 460.0)
    return at.save()


def build_ped_signal():
    name = 'SM_NS_PedSignal'
    face = rot_y(-90)
    pole = painted('nsk_pds_pole', 0x2c3330, 1024, rough=0.45, chips=0.9, rust=0.4, grime=0.9, grime_height=0.8, streaks=0.7, primer_hex=0x9ea3a6)
    hm = heads_material('nsk_pds_heads')
    pr = print_material('nsk_pds_prints', ped_prints(), 512)
    out, lit = [], []
    out += base_plate(pole, 0.06, 0.3)
    out.append(_G.lathe('pole', [(0.0, 0.0), (0.06, 0.0), (0.0595, 1.0), (0.055, 3.0), (0.0, 3.0)], pole, 36))
    out.append(_G.lathe('cap', [(0.0, 3.0), (0.065, 3.0), (0.065, 3.012), (0.05, 3.035), (0.0, 3.05)], pole, 36))
    out.append(box(0.055, 0.062, -0.035, 0.035, 2.0, 2.06, pole, 0.002, 2))
    hp = ped_head(hm, lit)
    xf(hp, Matrix.Translation((-0.09, 0.0, 2.95)))
    out += hp
    for g_ in lit:
        g_.data.transform(Matrix.Translation((-0.09, 0.0, 2.95)))
    # clamshell brackets to the pole: a collar round the pole, an arm to the head's back, bolts
    for zz in (2.55, 2.85):
        out.append(tagged(_G.lathe('collar', [(0.056, zz - 0.025), (0.072, zz - 0.025), (0.072, zz + 0.025), (0.056, zz + 0.025)], hm, 32), 0))
        out.append(tagged(box(-0.1, -0.06, -0.05, 0.05, zz - 0.02, zz + 0.02, hm, 0.006, 3), 0))
        for sy in (-1, 1):
            out.append(tagged(box(-0.02, 0.02, sy * 0.072 - 0.012, sy * 0.072 + 0.012, zz - 0.02, zz + 0.02, hm, 0.004, 2), 0))
            out.append(tagged(lathe('bolt', [(0.0, 0.0), (0.008, 0.0), (0.008, 0.008), (0.0, 0.009)], hm, 6,
                                    Matrix.Translation((0.02, sy * 0.072, zz)) @ rot_y(90)), 0))
    # the push button station at 1.07 m on the -X side: a strap round the pole, a rounded housing, the big button with
    # its raised arrow, a speaker grille, the sign on a rounded blank above
    for zz in (0.99, 1.15):
        out.append(tagged(_G.lathe('strap', [(0.059, zz - 0.012), (0.064, zz - 0.012), (0.064, zz + 0.012), (0.059, zz + 0.012)], hm, 32), 0))
    pb = parts.rbox('pb', (-0.1, 0.0, 1.07), (0.08, 0.13, 0.2), 0.018, 4)
    core.assign(pb, hm)
    out.append(tagged(pb, 0))
    out.append(tagged(_G.lathe('btnring', [(0.0, 0.0), (0.034, 0.0), (0.034, 0.006), (0.03, 0.008), (0.0, 0.008)], hm, 32,
                               Matrix.Translation((-0.14, 0.0, 1.05)) @ face), 0))
    out.append(tagged(_G.lathe('btn', [(0.0, 0.0), (0.026, 0.0), (0.026, 0.006), (0.022, 0.01), (0.0, 0.011)], hm, 32,
                               Matrix.Translation((-0.146, 0.0, 1.05)) @ face), 6))
    arrow = [(-0.014, -0.004), (0.004, -0.004), (0.004, -0.01), (0.016, 0.0), (0.004, 0.01), (0.004, 0.004), (-0.014, 0.004)]
    ar = flat_poly('arrow', arrow, frame=face_frame((-0.1571, 0.0, 1.05), (-1, 0)))
    core.orient_normals(ar, (-1, 0, 0))
    core.assign(ar, hm)
    out.append(tagged(ar, 7))
    for k in range(5):
        zz = 1.12 + 0.008 * k
        out.append(tagged(box(-0.142, -0.138, -0.03, 0.03, zz - 0.002, zz + 0.002, hm), 0))
    sign_ol = rrect(0.23, 0.46, 0.02, 6)
    out.append(blank_slab('signblank', sign_ol, (-0.066, 0.0, 1.41), (-1, 0), 0.003, hm))
    out[-1] = tagged(out[-1], 7)
    out.append(print_poly('sign', sign_ol, (-0.0679, 0.0, 1.41), (-1, 0), (0.0, 0.0, 1.0, 1.0), 0.23, 0.46, pr))
    for zz in (1.2, 1.62):
        out.append(tagged(box(-0.064, 0.0, -0.025, 0.025, zz - 0.012, zz + 0.012, hm, 0.003, 2), 0))
        out.append(tagged(lathe('rivet', [(0.0, 0.0), (0.006, 0.0), (0.005, 0.003), (0.0, 0.0035)], hm, 12,
                                Matrix.Translation((-0.0679, 0.0, zz)) @ face, fillet=0.0), 0))
    gl = glow(lit, f'{name}_GlowSignal')
    return [finish_mesh(out, name, [pole, hm, pr], 35.0, tag=TAG), gl]


# ================================================================== street-name blades and sign posts

BLADES = {'A': (('FIFTH ST', '1800', '1900'), ('MARKET ST', '200', None)), 'B': (('FIFTH ST', '1700', '1800'), ('HOLLIS ST', None, None)),
          'C': (('FIFTH ST', '1900', '2000'), ('WEXLER ST', None, None)), 'D': (('MARKET ST', '200', '300'), ('CARDINAL AVE', None, None))}


def blade_panel(name, lo, hi):
    """A street-name blade (900 x 150 mm): a white border, the name in white caps centered (its suffix smaller), the block
    numbers small at the right; the bottom 25 mm at the middle left clear for the clamp."""
    def draw(s, w, h):
        s.rect(0, 0, w, h, SIGN_GREEN)
        s.rect(6, 6, w - 12, h - 12, SIGN_WHITE)
        s.rect(10, 10, w - 20, h - 20, SIGN_GREEN)
        words = name.split(' ')
        main, suf = ' '.join(words[:-1]), words[-1]
        size = h * 0.56
        w_main = len(main) * size * 0.6
        w_suf = len(suf) * size * 0.4
        x0 = w * 0.47 - (w_main + w_suf + size * 0.15) / 2
        s.text(main, x0, h * 0.27, size, SIGN_WHITE, 'Bold', 'LEFT', tracking=1.0)
        s.text(suf, x0 + w_main + size * 0.15, h * 0.27, size * 0.62, SIGN_WHITE, 'Bold', 'LEFT')
        if lo:
            txt = f'{lo}-{hi}' if hi else lo
            s.text(txt, w - 22, h * 0.33, h * 0.24, SIGN_WHITE, 'Bold', 'RIGHT')
    return draw


def rrect(w, h, r, k=8):
    """A rounded rectangle's outline (w x h, corner radius r, k points per corner), centered, counter-clockwise."""
    pts = []
    for cx, cy, a0 in ((w / 2 - r, h / 2 - r, 0), (-w / 2 + r, h / 2 - r, 90), (-w / 2 + r, -h / 2 + r, 180), (w / 2 - r, -h / 2 + r, 270)):
        for j in range(k):
            a = math.radians(a0 + 90 * j / (k - 1))
            pts.append((cx + r * math.cos(a), cy + r * math.sin(a)))
    return pts


def face_frame(center, normal):
    """(right, up, normal) columns for a vertical face looking along `normal` (+-X or +-Y), as face_quad lays its print."""
    nx, ny = normal
    R = Vector((-ny, nx, 0.0))
    U = Vector((0.0, 0.0, 1.0))
    N = Vector((nx, ny, 0.0))
    M = Matrix((R, U, N)).transposed().to_4x4()
    M.translation = Vector(center)
    return M


def print_poly(name, pts, center, normal, uv_rect, w, h, mat):
    """A printed flat polygon (pts in the face's (right, up) coordinates) on a vertical face looking along `normal`; its
    print spans w x h about the center, so a rounded outline crops the print as the real blank does."""
    M = face_frame(center, normal)
    o = flat_poly(name, pts, frame=M)
    core.orient_normals(o, (normal[0], normal[1], 0.0))
    core.assign(o, mat)
    Mi = M.inverted()
    u0, v0, u1, v1 = uv_rect

    def uvf(co):
        p = Mi @ Vector(co)
        return (u0 + (u1 - u0) * (p.x / w + 0.5), v0 + (v1 - v0) * (p.y / h + 0.5))
    lock_uvs(o, uvf)
    return o


def blank_slab(name, pts, center, normal, t, mat, r=0.0015):
    """An aluminum sign blank: the outline extruded t thick (centered on center) along the face normal, its rims rounded."""
    M = face_frame(center, normal)
    o = parts.panel(name, pts, -t / 2, t / 2, r, 2, M)
    core.orient_normals(o)
    core.assign(o, mat)
    return o


def build_corner_blades(letter):
    name = f'SM_NS_CornerBlades_{letter}'
    tg = f'nsk_cb{letter.lower()}'
    pole = galv(f'{tg}_pole', 1024, grime=0.7)
    (n1, a1, b1), (n2, a2, b2) = BLADES[letter]
    at = Atlas(f'{tg}_prints', 2048, 512)
    rects = [(0.0, 0.0, 0.5, 0.333), (0.5, 0.0, 1.0, 0.333), (0.0, 0.34, 0.5, 0.673), (0.5, 0.34, 1.0, 0.673)]
    at.put(rects[0], blade_panel(n1, a1, b1), 900.0, 150.0)
    at.put(rects[1], blade_panel(n1, a1, b1), 900.0, 150.0)
    at.put(rects[2], blade_panel(n2, a2, b2), 900.0, 150.0)
    at.put(rects[3], blade_panel(n2, a2, b2), 900.0, 150.0)
    pr = print_material(f'{tg}_prints', at.save(), 2048, rough=0.3)
    out = []
    # the pole: a 90 mm galvanized tube on a bolted flange, a band at 2.4 m
    out.append(lathe('foot', [(0.0, 0.0), (0.075, 0.0), (0.07, 0.03), (0.05, 0.07), (0.0, 0.07)], pole, 24))
    out.append(lathe('pole', [(0.0, 0.0), (0.045, 0.0), (0.0445, 1.0), (0.04, 3.27), (0.0, 3.27)], pole, 24))
    out.append(lathe('band', [(0.042, 2.38), (0.05, 2.38), (0.05, 2.42), (0.042, 2.42)], pole, 48))
    L, Hb, T = 0.9, 0.15, 0.004
    outline = rrect(L, Hb, 0.025, 8)
    # the lower blade (along Y, read from +-X) sits in a slotted cast cap on the pole top; a cross fitting on its top edge
    # carries the upper blade (along X, read from +-Y)
    out.append(lathe('cap', [(0.0, 3.2), (0.052, 3.2), (0.052, 3.28), (0.045, 3.3), (0.03, 3.33), (0.0, 3.335)], pole, 48))
    z_lo = 3.31 + Hb / 2
    z_hi = z_lo + Hb + 0.06
    for zc, along, rr in ((z_hi, 'x', (0, 1)), (z_lo, 'y', (2, 3))):
        if along == 'x':
            out.append(blank_slab('blade', outline, (0.0, 0.0, zc), (0, -1), T, pole))
            out.append(print_poly('b1', outline, (0.0, -T / 2 - 0.0004, zc), (0, -1), rects[rr[0]], L, Hb, pr))
            out.append(print_poly('b2', outline, (0.0, T / 2 + 0.0004, zc), (0, 1), rects[rr[1]], L, Hb, pr))
            # its saddle: a slotted cast fitting on the lower blade's top edge, two bolts through the blade
            out.append(box(-0.05, 0.05, -0.013, 0.013, zc - Hb / 2 - 0.025, zc - Hb / 2 + 0.03, pole, 0.004, 3))
            out.append(box(-0.013, 0.013, -0.05, 0.05, zc - Hb / 2 - 0.06, zc - Hb / 2 - 0.02, pole, 0.004, 3))
            for xx in (-0.03, 0.03):
                for sy in (-1, 1):
                    out.append(lathe('bolt', [(0.0, 0.0), (0.006, 0.0), (0.006, 0.005), (0.0, 0.006)], pole, 6,
                                     Matrix.Translation((xx, sy * 0.013, zc - Hb / 2 + 0.012)) @ Matrix.Rotation(math.radians(-90 * sy), 4, 'X')))
        else:
            out.append(blank_slab('blade', outline, (0.0, 0.0, zc), (-1, 0), T, pole))
            out.append(print_poly('b3', outline, (-T / 2 - 0.0004, 0.0, zc), (-1, 0), rects[rr[0]], L, Hb, pr))
            out.append(print_poly('b4', outline, (T / 2 + 0.0004, 0.0, zc), (1, 0), rects[rr[1]], L, Hb, pr))
            # the cap's slotted jaws round the blade's bottom edge, its bolts
            out.append(box(-0.016, 0.016, -0.055, 0.055, zc - Hb / 2 - 0.035, zc - Hb / 2 + 0.03, pole, 0.004, 3))
            for yy in (-0.035, 0.035):
                for sx in (-1, 1):
                    out.append(lathe('bolt', [(0.0, 0.0), (0.006, 0.0), (0.006, 0.005), (0.0, 0.006)], pole, 6,
                                     Matrix.Translation((sx * 0.016, yy, zc - Hb / 2 + 0.012)) @ rot_y(90 * sx)))
    # set screws round the cap
    for a in (0, 120, 240):
        ra = math.radians(a + 30)
        out.append(lathe('setscrew', [(0.0, 0.0), (0.007, 0.0), (0.007, 0.01), (0.0, 0.012)], pole, 6,
                         Matrix.Translation((0.052 * math.cos(ra), 0.052 * math.sin(ra), 3.24)) @ Matrix.Rotation(ra, 4, 'Z') @ rot_y(90)))
    return [finish_mesh(out, name, [pole, pr], 35.0, tag=TAG)]


def sign_noparking(s, w, h):
    s.rect(0, 0, w, h, SIGN_WHITE)
    s.rect(8, 8, w - 16, h - 16, SIGN_RED)
    s.rect(14, 14, w - 28, h - 28, SIGN_WHITE)
    # the P under a red ring and slash
    cx, cy, r = w / 2, h * 0.72, w * 0.2
    s.text('P', cx, cy - r * 0.55, r * 1.55, INK, 'Black', 'CENTER')
    s.ring(cx, cy, r * 0.82, r, SIGN_RED)
    s.poly(thick_line((cx - r * 0.7, cy + r * 0.7), (cx + r * 0.7, cy - r * 0.7), r * 0.2), SIGN_RED)
    s.text('8 AM - 9:30 AM', w / 2, h * 0.45, w * 0.1, SIGN_RED, 'Black', 'CENTER')
    s.text('TUES & FRI', w / 2, h * 0.36, w * 0.1, SIGN_RED, 'Black', 'CENTER')
    s.rect(30, h * 0.31, w - 60, 3, SIGN_RED)
    s.text('STREET', w * 0.42, h * 0.21, w * 0.075, SIGN_RED, 'Bold', 'CENTER')
    s.text('CLEANING', w * 0.42, h * 0.14, w * 0.075, SIGN_RED, 'Bold', 'CENTER')
    # a broom
    bx, by = w * 0.8, h * 0.18
    s.poly(thick_line((bx - 30, by + 40), (bx + 6, by - 6), 6), SIGN_RED)
    s.poly([(bx - 6, by + 2), (bx + 26, by - 30), (bx + 6, by - 46), (bx - 22, by - 14)], SIGN_RED)


def sign_nostanding(s, w, h):
    s.rect(0, 0, w, h, SIGN_WHITE)
    s.rect(8, 8, w - 16, h - 16, SIGN_RED)
    s.rect(14, 14, w - 28, h - 28, SIGN_WHITE)
    s.text('NO', w / 2, h * 0.8, w * 0.16, SIGN_RED, 'Black', 'CENTER')
    s.text('STANDING', w / 2, h * 0.67, w * 0.14, SIGN_RED, 'Black', 'CENTER')
    s.text('BUS STOP', w / 2, h * 0.48, w * 0.13, INK, 'Black', 'CENTER')
    s.text('ANYTIME', w / 2, h * 0.36, w * 0.1, INK, 'Bold', 'CENTER')
    for sgn in (-1, 1):
        cx = w / 2 + sgn * w * 0.22
        s.poly(thick_line((cx - sgn * 40, h * 0.18), (cx + sgn * 20, h * 0.18), 14), INK)
        s.poly([(cx + sgn * 20, h * 0.18 - 26), (cx + sgn * 56, h * 0.18), (cx + sgn * 20, h * 0.18 + 26)], INK)


def sign_stop(s, w, h):
    cx, cy = w / 2, h / 2
    oct_ = [(cx + w / 2 * math.cos(math.radians(22.5 + 45 * k)), cy + h / 2 * math.sin(math.radians(22.5 + 45 * k))) for k in range(8)]
    s.rect(0, 0, w, h, 0x8b8f93)
    s.poly(oct_, SIGN_WHITE)
    s.poly([(cx + (w / 2 - 16) * math.cos(math.radians(22.5 + 45 * k)), cy + (h / 2 - 16) * math.sin(math.radians(22.5 + 45 * k))) for k in range(8)],
           SIGN_RED)
    s.text('STOP', cx, cy - h * 0.12, h * 0.33, SIGN_WHITE, 'Bold', 'CENTER', tracking=1.05)


def sign_busflag(s, w, h):
    s.rect(0, 0, w, h, TRANSIT_WHITE)
    s.rect(0, h * 0.8, w, h * 0.2, TRANSIT_GREEN)
    s.rect(0, h * 0.77, w, h * 0.03, TRANSIT_GOLD)
    s.text('CITY TRANSIT', w / 2, h * 0.865, h * 0.075, 0xffffff, 'Black', 'CENTER', tracking=1.05)
    s.circle(w / 2, h * 0.6, w * 0.2, TRANSIT_GREEN, n=96)
    s.ring(w / 2, h * 0.6, w * 0.17, w * 0.185, 0xffffff, n=96)
    s.text('14', w / 2, h * 0.6 - w * 0.08, w * 0.22, 0xffffff, 'Black', 'CENTER')
    s.text('EMBERCREST', w / 2, h * 0.36, w * 0.1, INK, 'Black', 'CENTER')
    s.text('via MARKET', w / 2, h * 0.29, w * 0.07, INK, 'Bold', 'CENTER')
    s.rect(w * 0.12, h * 0.25, w * 0.76, 3, 0x9a9da0)
    s.text('BUS STOP', w / 2, h * 0.16, w * 0.08, INK, 'Black', 'CENTER')
    s.text('No standing  ·  Owl service nightly', w / 2, h * 0.09, w * 0.045, 0x55585c, 'Bold', 'CENTER')


SIGNPOSTS = {'NoParking': (sign_noparking, 0.3, 0.45, False), 'NoStanding': (sign_nostanding, 0.3, 0.45, False),
             'Stop': (sign_stop, 0.75, 0.75, False), 'BusFlag': (sign_busflag, 0.45, 0.6, True)}


def perforated_post(name):
    """A 2 in. square galvanized post: 7/16 in. holes every inch down every face (in the bake)."""
    tex(name, 1024)
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    g = galv(name + '_tmp', 64)
    bpy.data.materials.remove(g.m)
    TEXTURE_SIZES.pop(name + '_tmp', None)
    # zinc spangle, grime at the foot, the holes: on each face, centered across it, every 25.4 mm up
    spang = noise(m, o, 60.0, 1.0, distortion=1.5)
    pat = noise(m, o, 5.0, 4.0)
    across = m.math('ADD', m.math('MULTIPLY', m.math('ABSOLUTE', x), 1.0), m.math('MULTIPLY', m.math('ABSOLUTE', y), 0.0))
    ax = m.math('LESS_THAN', m.math('ABSOLUTE', x), 0.024)
    ay = m.math('LESS_THAN', m.math('ABSOLUTE', y), 0.024)
    c = mix_float(m, ax, x, y)
    c = mix_float(m, m.math('MULTIPLY', ax, ay), c, x)
    vz = m.math('SUBTRACT', m.math('FRACT', m.math('DIVIDE', z, 0.0254)), 0.5)
    d = m.math('SQRT', m.math('ADD', m.math('MULTIPLY', c, c), m.math('MULTIPLY', m.math('MULTIPLY', vz, 0.0254), m.math('MULTIPLY', vz, 0.0254))))
    hole = smooth_less(m, d, 0.0056, 0.0006)
    del across
    grime = m.math('MULTIPLY', m.math('SUBTRACT', 1.0, m.math('DIVIDE', z, 0.7), clamp=True), ramp(m, noise(m, o, 9.0, 4.0), 0.3, 0.7))
    col = m.mix(m.math('MULTIPLY', spang, 0.25), H(0x9ea3a6), H(0xc3c7c9))
    col = m.mix(m.math('MULTIPLY', pat, 0.5), col, H(0x7d8184))
    col = m.mix(grime, col, H(0x3b3a36))
    col = m.mix(hole, col, H(0x050505))
    m.set('Base Color', col)
    m.set('Metallic', mix_float(m, m.math('MAXIMUM', grime, hole), 0.9, 0.2))
    m.set('Roughness', m.math('ADD', 0.45, m.math('MULTIPLY', m.math('SUBTRACT', spang, 0.5), 0.15)))
    set_bump(m, m.math('MULTIPLY', hole, -1.0), 0.6, 0.002)
    return m


def build_signpost(kind):
    name = f'SM_NS_SignPost_{kind}'
    tg = f'nsk_sp{kind.lower()[:4]}'
    fn, w, h, both = SIGNPOSTS[kind]
    post = perforated_post(f'{tg}_post')
    at = Atlas(f'{tg}_prints', 1024 if not both else 2048, 1024)
    if both:
        rects = [(0.0, 0.0, 0.49, 1.0), (0.51, 0.0, 1.0, 1.0)]
        at.put(rects[0], fn, w * 1000, h * 1000)
        at.put(rects[1], fn, w * 1000, h * 1000)
    else:
        rects = [(0.0, 0.0, 1.0, 1.0)]
        at.put(rects[0], fn, w * 1000, h * 1000)
    pr = print_material(f'{tg}_prints', at.save(), 1024 if not both else 2048, rough=0.3)
    out = []
    P = 3.0
    # the post (rounded corners as rolled), the anchor sleeve it sits in with its corner bolt, a plastic cap
    out.append(box(-0.0254, 0.0254, -0.0254, 0.0254, -0.05, P, post, 0.004, 3))
    out.append(box(-0.032, 0.032, -0.032, 0.032, -0.05, 0.45, post, 0.005, 3))
    out.append(lathe('sleevebolt', [(0.0, 0.0), (0.009, 0.0), (0.009, 0.007), (0.0, 0.008)], post, 6,
                     Matrix.Translation((-0.032, 0.0, 0.38)) @ rot_y(-90)))
    out.append(box(-0.027, 0.027, -0.027, 0.027, P, P + 0.012, post, 0.004, 3))
    zc = P - 0.06 - h / 2
    t = 0.0025
    if kind == 'Stop':
        # an octagon blank, its print on the front
        oct_ = [(w / 2 * math.cos(math.radians(22.5 + 45 * k)), h / 2 * math.sin(math.radians(22.5 + 45 * k))) for k in range(8)]
        out.append(blank_slab('blank', oct_, (-0.0254 - t / 2, 0.0, zc), (-1, 0), t, post, 0.001))
        out.append(print_poly('face', oct_, (-0.0254 - t - 0.0004, 0.0, zc), (-1, 0), rects[0], w, h, pr))
        xb_ = -0.0254 - t
    elif both:
        # the flag: two faces on a blank, held off the post on two brackets (it sticks out over the curb, along -Y)
        y0 = -0.05 - w
        ol = rrect(w, h, 0.03, 8)
        out.append(blank_slab('blank', ol, (0.0, y0 + w / 2, zc), (-1, 0), 0.004, post))
        out.append(print_poly('f1', ol, (-0.0024, y0 + w / 2, zc), (-1, 0), rects[0], w, h, pr))
        out.append(print_poly('f2', ol, (0.0024, y0 + w / 2, zc), (1, 0), rects[1], w, h, pr))
        for zz in (zc + h / 2 - 0.06, zc - h / 2 + 0.06):
            out.append(box(-0.012, 0.012, -0.075, 0.03, zz - 0.02, zz + 0.02, post, 0.004, 3))
            for sx in (-1, 1):
                out.append(lathe('bolt', [(0.0, 0.0), (0.008, 0.0), (0.008, 0.006), (0.0, 0.007)], post, 6,
                                 Matrix.Translation((sx * 0.012, -0.065, zz)) @ rot_y(90 * sx)))
        xb_ = None
    else:
        ol = rrect(w, h, 0.038, 8)
        out.append(blank_slab('blank', ol, (-0.0254 - t / 2, 0.0, zc), (-1, 0), t, post))
        out.append(print_poly('f', ol, (-0.0254 - t - 0.0004, 0.0, zc), (-1, 0), rects[0], w, h, pr))
        xb_ = -0.0254 - t
    if xb_ is not None:
        # bolts with their washers in front, nuts on the back of the post
        for zz in (zc + h / 2 - 0.02, zc - h / 2 + 0.02):
            out.append(lathe('washer', [(0.0, 0.0), (0.016, 0.0), (0.016, 0.0015), (0.0, 0.0015)], post, 24,
                             Matrix.Translation((xb_ - 0.0004, 0.0, zz)) @ rot_y(-90), fillet=0.0005))
            out.append(lathe('bolt', [(0.0, 0.0015), (0.011, 0.0015), (0.011, 0.006), (0.008, 0.008), (0.0, 0.0085)], post, 32,
                             Matrix.Translation((xb_ - 0.0004, 0.0, zz)) @ rot_y(-90), fillet=0.0))
            out.append(lathe('nut', [(0.0, 0.0), (0.01, 0.0), (0.01, 0.008), (0.0, 0.008)], post, 6,
                             Matrix.Translation((0.0254, 0.0, zz)) @ rot_y(90)))
    return [finish_mesh(out, name, [post, pr], 35.0, tag=TAG)]


# ================================================================== small pieces: bike rack, cellar doors, work zone

def build_bike_rack():
    st = galv('nsk_bike_steel', 1024, grime=0.5, grime_height=0.4)
    out = []
    pts = [(0.0, -0.375, 0.0)] + [(0.0, -0.375 + 0.2 * (1 - math.cos(math.radians(a))), 0.7 + 0.2 * math.sin(math.radians(a))) for a in range(0, 181, 10)]
    pts = [(0.0, -0.375, -0.05), (0.0, -0.375, 0.7)] + [(0.0, -0.175 - 0.2 * math.cos(math.radians(a)), 0.7 + 0.2 * math.sin(math.radians(a)))
                                                       for a in range(10, 171, 10)] + [(0.0, 0.375, 0.7), (0.0, 0.375, -0.05)]
    pts = [(0.0, -0.375, -0.05), (0.0, -0.375, 0.7)] + [(0.0, -0.175 * math.cos(math.radians(a)) - 0.2 * math.cos(math.radians(a)), 0.7 + 0.2 * math.sin(math.radians(a)))
                                                       for a in range(10, 171, 10)] + [(0.0, 0.375, 0.7), (0.0, 0.375, -0.05)]
    path = core.catmull_rom(pts, 1)
    out.append(core.sweep('u', path, parts.circle(0.024, 14)))
    core.assign(out[-1], st)
    for sy in (-0.375, 0.375):
        out.append(box(-0.075, 0.075, sy - 0.075, sy + 0.075, 0.0, 0.008, st, 0.003, 1))
        for (dx, dy) in ((-0.05, -0.05), (0.05, -0.05), (-0.05, 0.05), (0.05, 0.05)):
            out.append(_G.lathe('bolt', [(0.0, 0.008), (0.012, 0.008), (0.012, 0.016), (0.009, 0.02), (0.0, 0.02)], st, 6, Matrix.Translation((dx, sy + dy, 0))))
    return [finish_mesh(out, 'SM_NS_BikeRack', [st], 35.0, tag=TAG)]


def diamond_plate(name, size=2048, rust=0.6):
    tex(name, size)
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    # raised lugs, alternating 45 degrees, 30 mm pitch
    p = 0.03
    u = m.math('DIVIDE', m.math('ADD', x, y), p)
    v = m.math('DIVIDE', m.math('SUBTRACT', x, y), p)
    fu = m.math('SUBTRACT', m.math('FRACT', u), 0.5)
    fv = m.math('SUBTRACT', m.math('FRACT', v), 0.5)
    alt = m.math('MODULO', m.math('ADD', m.math('FLOOR', u), m.math('FLOOR', v)), 2.0)
    a1 = m.math('ADD', m.math('MULTIPLY', m.math('ABSOLUTE', fu), 0.25), m.math('ABSOLUTE', fv))
    a2 = m.math('ADD', m.math('ABSOLUTE', fu), m.math('MULTIPLY', m.math('ABSOLUTE', fv), 0.25))
    lugd = mix_float(m, alt, a1, a2)
    lug = smooth_less(m, lugd, 0.17, 0.02)
    top = m.math('GREATER_THAN', z, -0.003)
    mott = noise(m, o, 6.0, 4.0)
    rusty = m.math('MULTIPLY', ramp(m, noise(m, o, 10.0, 5.0), 0.45, 0.7), rust)
    polish = m.math('MULTIPLY', m.math('MULTIPLY', lug, top), ramp(m, mott, 0.35, 0.7))
    col = m.mix(ramp(m, mott, 0.3, 0.7), H(0x3b3a37), H(0x55524c))
    col = m.mix(rusty, col, m.mix(ramp(m, noise(m, o, 40.0, 3.0), 0.3, 0.7), H(0x5c321a), H(0x7a4524)))
    col = m.mix(m.math('MULTIPLY', m.math('SUBTRACT', 1.0, lug), 0.45), col, H(0x2a2622))
    col = m.mix(polish, col, H(0x8e8a84))
    m.set('Base Color', col)
    m.set('Metallic', mix_float(m, rusty, mix_float(m, polish, 0.6, 0.9), 0.2))
    m.set('Roughness', mix_float(m, polish, mix_float(m, rusty, 0.55, 0.85), 0.3))
    set_bump(m, m.math('ADD', m.math('MULTIPLY', lug, top), m.math('MULTIPLY', noise(m, o, 200.0, 2.0), 0.1)), 0.8, 0.003)
    return m


def build_cellar_doors():
    dp = diamond_plate('nsk_cel_steel', 2048)
    out = []
    # the frame: a steel angle round the opening, flush (its vertical leg into the sidewalk), a concrete curb under it
    W, Dd = 1.5, 1.2          # along y, along x
    fx, fy = Dd / 2 + 0.04, W / 2 + 0.04
    for (a0, a1, b0, b1) in ((-fx, fx, -fy, -fy + 0.05), (-fx, fx, fy - 0.05, fy), (-fx, -fx + 0.05, -fy, fy), (fx - 0.05, fx, -fy, fy)):
        out.append(box(a0, a1, b0, b1, -0.006, 0.0005, dp, 0.0025, 2))
    for sx in (-1, 1):
        leaf = box(min(0.003 * sx, sx * Dd / 2), max(0.003 * sx, sx * Dd / 2), -W / 2, W / 2, 0.0005, 0.0065, dp, 0.0025, 3)
        out.append(leaf)
        # strap hinges on the outer edges: a strap welded on the leaf, a knuckle on the frame, pins, bolt heads
        for yy in (-0.5, 0.5):
            out.append(box(min(sx * (Dd / 2 - 0.28), sx * (Dd / 2 - 0.005)), max(sx * (Dd / 2 - 0.28), sx * (Dd / 2 - 0.005)), yy - 0.03, yy + 0.03,
                           0.0065, 0.0105, dp, 0.0015, 2))
            out.append(_G.rod((sx * (Dd / 2 + 0.012), yy - 0.065, 0.007), (sx * (Dd / 2 + 0.012), yy + 0.065, 0.007), 0.011, dp, 14))
            for k in range(2):
                xx = sx * (Dd / 2 - 0.08 - 0.12 * k)
                out.append(_G.lathe('carriage', [(0.0, 0.0), (0.008, 0.0), (0.006, 0.0035), (0.0, 0.0045)], dp, 8,
                                    Matrix.Translation((xx, yy, 0.0105))))
        # a recessed lift ring in a pocket
        out.append(box(sx * 0.25 - 0.07, sx * 0.25 + 0.07, -0.04, 0.04, 0.0062, 0.0072, dp, 0.002, 2))
        ring = [(sx * 0.25 + 0.05 * math.cos(math.radians(a)), 0.03 * math.sin(math.radians(a)), 0.0085) for a in range(0, 361, 30)]
        out.append(_G.tube(ring, 0.005, dp, 6, 1))
        for sgn in (-1, 1):
            out.append(_G.box(sx * 0.25 + sgn * 0.05 - 0.008, sx * 0.25 + sgn * 0.05 + 0.008, -0.008, 0.008, 0.0065, 0.012, dp, 0.002, 1))
    # the hasp across the meeting edges (a staple on one leaf, the slotted strap on the other) and a padlock
    out.append(box(-0.06, 0.0, 0.35, 0.39, 0.0065, 0.010, dp, 0.0015, 2))
    out.append(box(0.0, 0.05, 0.355, 0.385, 0.0065, 0.010, dp, 0.0015, 2))
    out.append(rod((0.03, 0.37, 0.010), (0.03, 0.37, 0.028), 0.006, dp, 12))
    out.append(parts.rbox('lock', (0.03, 0.37, 0.05), (0.034, 0.055, 0.045), 0.008, 2))
    core.assign(out[-1], dp)
    out.append(_G.tube([(0.03, 0.352, 0.072), (0.03, 0.352, 0.09), (0.03, 0.37, 0.1), (0.03, 0.388, 0.09), (0.03, 0.388, 0.072)], 0.0045, dp, 8, 3))
    out.append(rod((0.013, 0.37, 0.045), (0.011, 0.37, 0.045), 0.006, dp, 16))
    return [finish_mesh(out, 'SM_NS_CellarDoors', [dp], 40.0, tag=TAG)]


def stripes_material(name, a_hex, b_hex, width=0.15, angle=45.0, size=1024, refl=True):
    """Retroreflective sheeting in diagonal stripes (barricade boards, the cone's collars via a plain band)."""
    tex(name, size)
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    ca, sa = math.cos(math.radians(angle)), math.sin(math.radians(angle))
    t = m.math('ADD', m.math('MULTIPLY', y, ca), m.math('MULTIPLY', z, sa))
    st = ramp(m, m.math('ABSOLUTE', m.math('SUBTRACT', m.math('FRACT', m.math('DIVIDE', t, 2 * width)), 0.5)), 0.245, 0.255)
    dirt = m.math('MULTIPLY', ramp(m, noise(m, o, 9.0, 4.0), 0.4, 0.85), 0.6)
    scuff = ramp(m, noise(m, scaled(m, sep, 4.0, 60.0, 60.0), 1.0, 2.0), 0.62, 0.7)
    col = m.mix(st, H(a_hex), H(b_hex))
    col = m.mix(m.math('MULTIPLY', dirt, 0.5), col, H(0x5a554d))
    col = m.mix(m.math('MULTIPLY', scuff, 0.6), col, H(0x8a8580))
    m.set('Base Color', col)
    m.set('Roughness', m.math('ADD', 0.35 if refl else 0.6, m.math('MULTIPLY', dirt, 0.3)))
    return m


def build_barricade():
    board = stripes_material('nsk_bar_board', 0xd2531a, 0xeeeae2, 0.1, 45.0, 1024)
    frame = painted('nsk_bar_frame', 0xe2b31c, 1024, rough=0.5, chips=1.0, rust=0.8, grime=1.0, grime_height=0.6, streaks=0.5)
    out = []
    L = 1.8
    # A-frame legs at each end, two boards across
    for sy in (-1, 1):
        y = sy * (L / 2 - 0.12)
        for sx in (-1, 1):
            out.append(rod((sx * 0.32, y, 0.0), (sx * 0.02, y, 0.98), 0.018, frame, 10))
        out.append(rod((-0.27, y, 0.18), (0.27, y, 0.18), 0.012, frame, 8))
        out.append(box(-0.05, 0.05, y - 0.03, y + 0.03, 0.95, 1.0, frame, 0.005, 1))
    for zc in (0.86, 0.55):
        out.append(box(-0.04, -0.015, -L / 2, L / 2, zc - 0.1, zc + 0.1, board, 0.004, 1))
    # rubber feet on the legs, carriage bolts through the boards into the frame
    for sy in (-1, 1):
        y = sy * (L / 2 - 0.12)
        for sx in (-1, 1):
            out.append(box(sx * 0.32 - 0.05, sx * 0.32 + 0.05, y - 0.035, y + 0.035, 0.0, 0.03, frame, 0.008, 3))
        for zc in (0.86, 0.55):
            out.append(lathe('bolt', [(0.0, 0.0), (0.012, 0.0), (0.011, 0.004), (0.0, 0.006)], frame, 16,
                             Matrix.Translation((-0.04, y, zc)) @ rot_y(-90), fillet=0.0))
    # a flasher on top: a battery box, an amber lens in a ribbed bezel
    out.append(box(-0.06, 0.06, L / 2 - 0.2, L / 2 - 0.04, 1.0, 1.1, frame, 0.012, 3))
    out.append(lathe('bezel', [(0.0, 0.0), (0.085, 0.0), (0.085, 0.012), (0.07, 0.02), (0.0, 0.02)], frame, 48,
                     Matrix.Translation((-0.04, L / 2 - 0.12, 1.18)) @ rot_y(-90)))
    out.append(lathe('lens', [(0.0, 0.0), (0.07, 0.0), (0.06, 0.012), (0.035, 0.02), (0.0, 0.022)], frame, 48,
                     Matrix.Translation((-0.06, L / 2 - 0.12, 1.18)) @ rot_y(-90), fillet=0.0))
    out.append(box(-0.04, 0.04, L / 2 - 0.16, L / 2 - 0.08, 1.1, 1.26, frame, 0.01, 3))
    return [finish_mesh(out, 'SM_NS_Barricade', [board, frame], 35.0, tag=TAG)]


def build_cone():
    tex('nsk_cone_body', 1024)
    m = core.Mat('nsk_cone_body')
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    z = sep.outputs['Z']
    band = m.math('MAXIMUM', m.math('MULTIPLY', ramp(m, z, 0.43, 0.432), ramp(m, z, 0.585, 0.583)),
                  m.math('MULTIPLY', ramp(m, z, 0.28, 0.282), ramp(m, z, 0.38, 0.378)))
    dirt = m.math('MULTIPLY', ramp(m, noise(m, o, 12.0, 4.0), 0.35, 0.8), m.math('ADD', 0.3, m.math('MULTIPLY', ramp(m, z, 0.25, 0.0), 0.7)))
    base = m.math('LESS_THAN', z, 0.04)
    col = m.mix(band, H(0xe0521c), H(0xe9e7e2))
    col = m.mix(base, col, H(0x111111))
    col = m.mix(m.math('MULTIPLY', dirt, 0.55), col, H(0x4a4038))
    scuff = ramp(m, noise(m, scaled(m, sep, 80.0, 80.0, 3.0), 1.0, 2.0), 0.64, 0.7)
    col = m.mix(m.math('MULTIPLY', scuff, 0.5), col, H(0x2a2522))
    m.set('Base Color', col)
    m.set('Roughness', mix_float(m, band, 0.55, 0.35))
    out = []
    out.append(lathe('cone', [(0.0, 0.0), (0.18, 0.0), (0.18, 0.035), (0.13, 0.04), (0.125, 0.06), (0.032, 0.71), (0.0, 0.712)], m, 48))
    out.append(lathe('basehole', [(0.0, 0.035), (0.12, 0.035), (0.12, 0.04), (0.0, 0.04)], m, 48))
    # the base is square with rounded corners
    sq = parts.rbox('base', (0.0, 0.0, 0.017), (0.36, 0.36, 0.034), 0.05, 3)
    core.assign(sq, m)
    out.append(sq)
    return [finish_mesh(out, 'SM_NS_Cone', [m], 40.0, tag=TAG)]


def build_road_plate():
    st = diamond_plate('nsk_plate_steel', 2048, rust=0.8)
    out = []
    W, L, T = 1.5, 2.4, 0.025
    plate = box(-W / 2, W / 2, -L / 2, L / 2, -T, 0.0, st, 0.006, 2)
    out.append(plate)
    # cold-patch ramps on its edges (black asphalt mix), two lifting holes
    asph = core.Mat('nsk_plate_patch')
    tex('nsk_plate_patch', 1024)
    tc, sep = object_coords(asph)
    g = noise(asph, tc.outputs['Object'], 300.0, 1.0)
    asph.set('Base Color', asph.mix(ramp(asph, g, 0.6, 0.8), H(0x161718), H(0x4b4945)))
    asph.set('Roughness', 0.85)
    set_bump(asph, g, 0.5, 0.002)
    # the cold patch feathered round the plate: a lumpy wedge (grid displaced by noise) on every side, the corners mitred
    rng = random.Random(77)

    def lump(u, v):
        return 0.004 * math.sin(u * 17.0 + v * 9.0) * math.sin(u * 5.3 - v * 21.0) + rng.uniform(-0.0015, 0.0015)
    bm = bmesh.new()
    for side in range(4):
        along = W if side < 2 else L
        nu, nv = int(along / 0.04), 6
        grid = []
        for i in range(nu + 1):
            row = []
            a = -along / 2 + along * i / nu
            for j in range(nv + 1):
                t = j / nv
                w_ = 0.13 * t
                z = -T * t ** 0.8 * 0.95 + (lump(a, t) * math.sin(math.pi * t) if 0 < t < 1 else 0.0)
                aa = a * (along / 2 + w_) / (along / 2)          # mitred: the strips meet at the corners
                if side == 0:
                    p = (aa, -L / 2 - w_, z)
                elif side == 1:
                    p = (-aa, L / 2 + w_, z)
                elif side == 2:
                    p = (W / 2 + w_, aa, z)
                else:
                    p = (-W / 2 - w_, -aa, z)
                row.append(bm.verts.new(p))
            grid.append(row)
        for i in range(nu):
            for j in range(nv):
                bm.faces.new((grid[i][j], grid[i + 1][j], grid[i + 1][j + 1], grid[i][j + 1]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    rp = core.mesh_object('patch', bm)
    core.orient_normals(rp, (0, 0, 1))
    core.assign(rp, asph)
    out.append(rp)
    # locating pins (the plate is pinned against creep) and the weld beads of its lifting lugs
    for (px, py) in ((-W / 2 + 0.08, -L / 2 + 0.08), (W / 2 - 0.08, -L / 2 + 0.08), (-W / 2 + 0.08, L / 2 - 0.08), (W / 2 - 0.08, L / 2 - 0.08)):
        out.append(lathe('pin', [(0.0, 0.0), (0.016, 0.0), (0.016, 0.004), (0.012, 0.007), (0.0, 0.008)], st, 24,
                         Matrix.Translation((px, py, 0.0)), fillet=0.0))
    for yy in (-0.9, 0.9):
        cut = parts.disc('hole', (0.0, yy, -0.01), (0, 0, 1), 0.03, 0.06, 16)
        core.boolean(plate, cut)
    return [finish_mesh(out, 'SM_NS_RoadPlate', [st, asph], 40.0, tag=TAG)]


def build_jersey():
    tex('nsk_jer_concrete', 2048)
    m = core.Mat('nsk_jer_concrete')
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    z = sep.outputs['Z']
    blot = noise(m, o, 3.0, 5.0)
    pits = ramp(m, noise(m, o, 180.0, 1.0), 0.7, 0.74)
    chips = m.math('MULTIPLY', convex_edges(m, tc, distance=0.01, samples=12, breakup_scale=20.0, gain=5.0), 1.0)
    chipm = ramp(m, chips, 0.3, 0.5)
    tire = m.math('MULTIPLY', ramp(m, noise(m, scaled(m, sep, 1.0, 30.0, 30.0), 1.0, 2.0), 0.6, 0.75), ramp(m, z, 0.35, 0.1))
    col = m.mix(ramp(m, blot, 0.3, 0.7), H(0xa29d93), H(0x7f7b74))
    col = m.mix(m.math('MULTIPLY', pits, 0.5), col, H(0x3c3a36))
    col = m.mix(m.math('MULTIPLY', ramp(m, z, 0.3, 0.0), 0.6), col, H(0x4d4943))
    col = m.mix(m.math('MULTIPLY', tire, 0.7), col, H(0x1d1c1b))
    col = m.mix(chipm, col, H(0xb6b1a6))
    m.set('Base Color', col)
    m.set('Roughness', 0.88)
    set_bump(m, m.math('SUBTRACT', m.math('ADD', m.math('MULTIPLY', blot, 0.5), m.math('MULTIPLY', chipm, -0.6)), pits), 0.4, 0.003)
    # F-shape: 0.61 wide at the foot, 0.15 at the top, 0.81 tall; break points at 0.075 and 0.33
    hw = [(0.305, 0.0), (0.305, 0.075), (0.23, 0.33), (0.075, 0.79), (0.07, 0.81)]
    prof = [(-a, b) for a, b in reversed(hw)] + [(a, b) for a, b in hw]
    prof = [(x, z) for x, z in [(-0.305, 0.0)] + [(-a, b) for a, b in hw[1:]][::1]]
    prof = [(0.305, 0.0), (0.305, 0.075), (0.23, 0.33), (0.075, 0.79), (0.06, 0.81), (-0.06, 0.81), (-0.075, 0.79), (-0.23, 0.33), (-0.305, 0.075),
            (-0.305, 0.0)]
    def fillet2(poly, rf=0.012):
        # rounds a closed polygon's corners (a quadratic arc each, 3 points)
        outp = []
        n = len(poly)
        for i in range(n):
            a, p, c = poly[i - 1], poly[i], poly[(i + 1) % n]
            la = math.hypot(a[0] - p[0], a[1] - p[1])
            lc = math.hypot(c[0] - p[0], c[1] - p[1])
            if p[1] <= 1e-6:
                outp.append(p)
                continue
            f_ = min(rf, 0.3 * la, 0.3 * lc)
            pa = (p[0] + (a[0] - p[0]) / la * f_, p[1] + (a[1] - p[1]) / la * f_)
            pc = (p[0] + (c[0] - p[0]) / lc * f_, p[1] + (c[1] - p[1]) / lc * f_)
            for t in (0.0, 0.25, 0.5, 0.75, 1.0):
                outp.append(((1 - t) ** 2 * pa[0] + 2 * (1 - t) * t * p[0] + t * t * pc[0], (1 - t) ** 2 * pa[1] + 2 * (1 - t) * t * p[1] + t * t * pc[1]))
        return outp
    prof = fillet2(prof)
    inset = [(x_ * 0.94, z_ * 0.985 if z_ > 0.01 else z_) for x_, z_ in prof]
    rings = []
    for yy, pr_ in ((-1.525, inset), (-1.51, prof), (-1.2, prof), (-0.6, prof), (0.0, prof), (0.6, prof), (1.2, prof), (1.51, prof), (1.525, inset)):
        rings.append([(x_, yy, z_) for x_, z_ in pr_])
    body = core.loft('jersey', rings)
    core.orient_normals(body)
    core.assign(body, m)
    # the drainage scuppers through the foot
    for yy in (-0.4, 0.4):
        cut = box(-0.4, 0.4, yy - 0.15, yy + 0.15, -0.01, 0.08, None)
        core.boolean(body, cut)
    # the lifting slots and the connecting loops' pockets at the ends
    for yy in (-0.75, 0.75):
        cut = box(-0.4, 0.4, yy - 0.08, yy + 0.08, 0.0, 0.1, None)
        core.boolean(body, cut)
    out = [body]
    st = painted('nsk_jer_steel', 0x6a5c4e, 256, rough=0.7, chips=1.0, rust=1.0)
    for yy in (-1.5, 1.5):
        out.append(tube([(0.0, yy, 0.6), (0.0, yy + math.copysign(0.05, yy), 0.62), (0.0, yy + math.copysign(0.05, yy), 0.7), (0.0, yy, 0.72)], 0.012, st, 8, 4))
    return [finish_mesh(out, 'SM_NS_JerseyBarrier', [m, st], 35.0, tag=TAG)]


def sidewalk_closed(s, w, h):
    s.rect(0, 0, w, h, SIGN_WHITE)
    s.rect(10, 10, w - 20, h - 20, INK)
    s.rect(18, 18, w - 36, h - 36, SIGN_WHITE)
    s.text('SIDEWALK', w / 2, h * 0.66, h * 0.17, INK, 'Black', 'CENTER')
    s.text('CLOSED', w / 2, h * 0.43, h * 0.2, INK, 'Black', 'CENTER')
    s.rect(w * 0.1, h * 0.35, w * 0.8, 6, INK)
    s.text('USE OTHER SIDE', w / 2, h * 0.16, h * 0.11, INK, 'Black', 'CENTER')


def build_sidewalk_closure():
    st = galv('nsk_clo_steel', 1024, grime=0.7, grime_height=0.5)
    feet = concrete_mat('nsk_clo_feet', 0x8a857c, 512)
    at = Atlas('nsk_clo_prints', 1024, 512)
    at.put((0.0, 0.0, 1.0, 1.0), sidewalk_closed, 900.0, 450.0)
    pr = print_material('nsk_clo_prints', at.save(), 1024, rough=0.3)
    out = []
    W, Hh = 2.4, 1.8
    # a tube frame and a welded mesh (wires on a 50 x 100 grid... as a perforated sheet of wires)
    for y in (-W / 2 + 0.02, W / 2 - 0.02):
        out.append(rod((0.0, y, 0.12), (0.0, y, Hh + 0.12), 0.019, st, 12))
    for z in (0.14, Hh + 0.1):
        out.append(rod((0.0, -W / 2 + 0.02, z), (0.0, W / 2 - 0.02, z), 0.019, st, 12))
    for k in range(1, 24):
        y = -W / 2 + 0.02 + (W - 0.04) * k / 24
        out.append(rod((0.0, y, 0.15), (0.0, y, Hh + 0.09), 0.002, st, 4))
    for k in range(1, 18):
        z = 0.15 + (Hh - 0.06) * k / 18
        out.append(rod((0.006, -W / 2 + 0.03, z), (0.006, W / 2 - 0.03, z), 0.002, st, 4))
    # the weighted feet (recycled-rubber/concrete blocks), the pins down into them
    for y in (-W / 2 + 0.02, W / 2 - 0.02):
        out.append(box(-0.3, 0.3, y - 0.09, y + 0.09, 0.0, 0.13, feet, 0.02, 2))
    # the pins from the frame down into the feet, couplers on the posts (where the next panel clips on), a bracing stay
    for y in (-W / 2 + 0.02, W / 2 - 0.02):
        out.append(rod((0.0, y, 0.0), (0.0, y, 0.14), 0.016, st, 24))
        out.append(_G.lathe('ferrule', [(0.0, 0.125), (0.026, 0.125), (0.026, 0.16), (0.0, 0.16)], st, 16, Matrix.Translation((0.0, y, 0.0))))
        for zz in (0.5, Hh - 0.2):
            out.append(_G.lathe('coupler', [(0.0, -0.035), (0.03, -0.035), (0.03, 0.035), (0.0, 0.035)], st, 16, Matrix.Translation((0.0, y, zz))))
            out.append(box(-0.012, 0.012, y - 0.06 * (1 if y > 0 else -1) - 0.01, y - 0.06 * (1 if y > 0 else -1) + 0.01, zz - 0.02, zz + 0.02,
                           st, 0.004, 2))
        out.append(rod((0.28, y, 0.13), (0.03, y, 1.1), 0.012, st, 16))
    # the sign: a rounded blank, cable-tied to the mesh at its four corners
    ol = rrect(0.9, 0.45, 0.03, 8)
    out.append(blank_slab('blank', ol, (-0.0085, 0.0, 1.075), (-1, 0), 0.004, st))
    out.append(print_poly('sign', ol, (-0.011, 0.0, 1.075), (-1, 0), (0.0, 0.0, 1.0, 1.0), 0.9, 0.45, pr))
    for yy in (-0.41, 0.41):
        for zz in (0.885, 1.265):
            tie = [(-0.012, yy, zz + 0.012), (-0.014, yy, zz), (-0.012, yy, zz - 0.012), (0.008, yy, zz - 0.01), (0.008, yy, zz + 0.01),
                   (-0.012, yy, zz + 0.012)]
            out.append(tube(tie, 0.0018, st, 6, 2))
    return [finish_mesh(out, 'SM_NS_SidewalkClosure', [st, feet, pr], 35.0, tag=TAG)]


# ================================================================== SM_NS_IceChest

def ice_front(s, w, h):
    """One door's front: ICE in big blue letters over a frosty field, the small print."""
    s.rect(0, 0, w, h, 0xf2f4f5)
    s.rect(0, h * 0.82, w, h * 0.18, 0x1d5fa8)
    s.text('PACKAGED', w / 2, h * 0.875, h * 0.07, 0xffffff, 'Black', 'CENTER', tracking=1.15)
    s.text('ICE', w / 2, h * 0.38, h * 0.42, 0x1d5fa8, 'Black', 'CENTER', tracking=1.05)
    for k in range(9):
        cx, cy = w * (0.1 + 0.1 * k), h * 0.24 + 20 * math.sin(k)
        for a in range(3):
            s.poly(thick_line((cx - 14 * math.cos(math.pi * a / 3), cy - 14 * math.sin(math.pi * a / 3)),
                              (cx + 14 * math.cos(math.pi * a / 3), cy + 14 * math.sin(math.pi * a / 3)), 3), 0x7fb0dd)
    s.text('SELF SERVE  ·  PAY INSIDE', w / 2, h * 0.12, h * 0.045, 0x1d5fa8, 'Bold', 'CENTER')
    s.text('KEEP DOOR CLOSED', w / 2, h * 0.06, h * 0.035, 0x55585c, 'Bold', 'CENTER')


def build_ice_chest():
    cab = painted('nsk_ice_cab', 0xe6e8e9, 2048, rough=0.4, chips=0.6, rust=0.9, grime=1.0, grime_height=0.45, streaks=0.7)
    at = Atlas('nsk_ice_prints', 1024)
    rects = [(0.0, 0.0, 0.49, 1.0), (0.51, 0.0, 1.0, 1.0)]
    at.put(rects[0], ice_front, 600.0, 900.0)
    at.put(rects[1], ice_front, 600.0, 900.0)
    pr = print_material('nsk_ice_prints', at.save(), 1024, rough=0.3, weather=0.8)
    # frost: a white rime on the door seals (in its own baked material, a soft frosted white)
    frost = core.Mat('nsk_ice_frost')
    tex('nsk_ice_frost', 512)
    tc, sep = object_coords(frost)
    fr = noise(frost, tc.outputs['Object'], 80.0, 3.0)
    frost.set('Base Color', frost.mix(fr, H(0xdfe6ea), H(0xf6f9fb)))
    frost.set('Roughness', 0.6)
    set_bump(frost, fr, 0.5, 0.002)
    out = []
    W, D, Hh = 1.5, 0.8, 1.2
    # cabinet on a kick base with louvers (the condenser)
    out.append(box(-D / 2, D / 2, -W / 2, W / 2, 0.12, Hh, cab, 0.02, 2))
    out.append(box(-D / 2 + 0.03, D / 2 - 0.03, -W / 2 + 0.03, W / 2 - 0.03, 0.0, 0.12, cab, 0.01, 1))
    for k in range(10):
        y = -0.6 + 1.2 * k / 9
        out.append(box(-D / 2 + 0.02, -D / 2 + 0.03, y - 0.04, y + 0.04, 0.03, 0.1, cab, 0.003, 1))
    # two doors with their prints, raised 15 mm, gaskets frosted, pull handles, a hasp across them
    for i, (y0, y1) in enumerate(((-W / 2 + 0.04, -0.01), (0.01, W / 2 - 0.04))):
        out.append(box(-D / 2 - 0.015, -D / 2, y0, y1, 0.17, Hh - 0.05, cab, 0.012, 2))
        out.append(face_quad('door', -D / 2 - 0.0155, (y0 + y1) / 2, (0.17 + Hh - 0.05) / 2, (y1 - y0) - 0.06, (Hh - 0.22) - 0.06, (-1, 0),
                             rects[i], pr))
        for (a0, a1, b0, b1) in ((y0 - 0.006, y1 + 0.006, Hh - 0.05, Hh - 0.044), (y0 - 0.006, y1 + 0.006, 0.164, 0.17), (y0 - 0.006, y0, 0.17, Hh - 0.05),
                                 (y1, y1 + 0.006, 0.17, Hh - 0.05)):
            out.append(box(-D / 2 - 0.006, -D / 2 + 0.002, a0, a1, b0, b1, frost))
        hy = y1 - 0.06 if i == 0 else y0 + 0.06
        out.append(box(-D / 2 - 0.05, -D / 2 - 0.015, hy - 0.015, hy + 0.015, 0.55, 0.85, cab, 0.008, 2))
    out.append(box(-D / 2 - 0.03, -D / 2 - 0.015, -0.08, 0.08, 0.62, 0.66, cab, 0.003, 1))
    lock = parts.rbox('lock', (-D / 2 - 0.045, 0.0, 0.6), (0.03, 0.05, 0.045), 0.006)
    core.assign(lock, cab)
    out.append(lock)
    out.append(tube([(-D / 2 - 0.045, -0.015, 0.62), (-D / 2 - 0.045, -0.015, 0.64), (-D / 2 - 0.045, 0.0, 0.648), (-D / 2 - 0.045, 0.015, 0.64),
                     (-D / 2 - 0.045, 0.015, 0.62)], 0.004, cab, 8, 4))
    # the doors' hinges (three barrels each on their outer edges), aluminum corner trims riveted on, the top's rim
    for i, (y0, y1) in enumerate(((-W / 2 + 0.04, -0.01), (0.01, W / 2 - 0.04))):
        yh = y0 - 0.008 if i == 0 else y1 + 0.008
        for zz in (0.3, 0.68, 1.06):
            out.append(rod((-D / 2 - 0.012, yh, zz - 0.045), (-D / 2 - 0.012, yh, zz + 0.045), 0.009, cab, 20))
            out.append(lathe('pincap', [(0.0, 0.0), (0.009, 0.0), (0.007, 0.004), (0.0, 0.005)], cab, 16,
                             Matrix.Translation((-D / 2 - 0.012, yh, zz + 0.045)), fillet=0.0))
    for sx in (-1, 1):
        for sy in (-1, 1):
            cx, cy = sx * D / 2, sy * W / 2
            out.append(box(cx - 0.02 if sx > 0 else cx - 0.004, cx + 0.004 if sx > 0 else cx + 0.02, cy - 0.004 if sy < 0 else cy - 0.02,
                           cy + 0.02 if sy < 0 else cy + 0.004, 0.12, Hh + 0.002, cab, 0.002, 2))
            for k in range(6):
                zz = 0.2 + (Hh - 0.3) * k / 5
                out.append(lathe('rivet', [(0.0, 0.0), (0.004, 0.0), (0.0035, 0.002), (0.0, 0.0025)], cab, 8,
                                 Matrix.Translation((cx + sx * 0.004, cy - sy * 0.012, zz)) @ rot_y(90 * sx), fillet=0.0))
    out.append(box(-D / 2 - 0.01, D / 2 + 0.01, -W / 2 - 0.01, W / 2 + 0.01, Hh - 0.012, Hh + 0.012, cab, 0.006, 3))
    # the condenser grille on the +Y end: a frame and slanted louvers
    out.append(box(-0.25, 0.25, W / 2 - 0.002, W / 2 + 0.012, 0.2, 0.55, cab, 0.004, 2))
    for k in range(12):
        zz = 0.22 + 0.31 * k / 11
        out.append(prism_x('louver', [(W / 2 + 0.002, zz - 0.012), (W / 2 + 0.016, zz - 0.004), (W / 2 + 0.016, zz), (W / 2 + 0.002, zz - 0.008)],
                           -0.23, 0.23, cab))
    # the power cord out of the back to the wall
    out.append(tube([(D / 2, 0.5, 0.2), (D / 2 + 0.06, 0.52, 0.12), (D / 2 + 0.1, 0.6, 0.02), (D / 2 + 0.12, 0.66, 0.012), (D / 2 + 0.14, 0.7, 0.3),
                     (D / 2 + 0.14, 0.72, 0.42)], 0.006, cab, 10, 8))
    out.append(box(D / 2 + 0.12, D / 2 + 0.16, 0.7, 0.75, 0.4, 0.47, cab, 0.006, 3))
    # frost on the top's edge and a drip line of rust at the foot is in the paint
    return [finish_mesh(out, 'SM_NS_IceChest', [cab, pr, frost], 35.0, tag=TAG)]


# ================================================================== awnings

AWN = {'S': (3.0, 2), 'M': (6.0, 2), 'L': (9.0, 3)}
AWN_P, AWN_DROP, AWN_VAL = 1.6, 0.55, 0.25


def awning_fabric(name, size):
    """Solution-dyed acrylic canvas baked light and neutral (the game tints it): the weave, sag wrinkles between the bars,
    dirt streaked down the slope from the front bar, a darker drip line, pigeon marks near the hood."""
    tex(name, size)
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    weave = noise(m, scaled(m, sep, 900.0, 900.0, 900.0), 1.0, 1.0)
    streak = ramp(m, noise(m, scaled(m, sep, 3.0, 30.0, 3.0), 1.0, 3.0), 0.5, 0.8)
    low = ramp(m, x, -0.6, -AWN_P)            # toward the front bar
    dirt = m.math('MULTIPLY', streak, m.math('ADD', 0.25, m.math('MULTIPLY', low, 0.6)))
    blot = m.math('MULTIPLY', ramp(m, noise(m, o, 5.0, 4.0), 0.55, 0.8), 0.4)
    col = m.mix(m.math('MULTIPLY', weave, 0.15), H(0xe9e7e2), H(0xd9d6cf))
    col = m.mix(m.math('MULTIPLY', dirt, 0.45), col, H(0x8b857a))
    col = m.mix(blot, col, H(0xb4aea2))
    m.set('Base Color', col)
    m.set('Roughness', 0.86)
    m.set('Sheen Weight', 0.4)
    set_bump(m, m.math('ADD', weave, m.math('MULTIPLY', streak, 0.3)), 0.25, 0.0006)
    return m


def valance_default(name, width):
    s = Sheet(width * 1000, AWN_VAL * 1000)
    s.rect(0, 0, width * 1000, AWN_VAL * 1000, 0xe9e7e2)
    color, _ = s.render(f'{name}_default', 1024, samples=4)
    return color


def build_awning(size):
    W, npairs = AWN[size]
    name = f'SM_NS_Awning_{size}'
    tg = f'nsk_awn{size.lower()}'
    fab = awning_fabric(f'{tg}_fabric', (2048, 512) if W > 4 else (1024, 512))
    met = painted(f'{tg}_metal', 0x3a3d40, 1024, rough=0.4, chips=0.5, rust=0.5, grime=0.0, streaks=0.7)
    val = screen_material(f'ns_screen_valance', None, 0xe9e7e2)
    out = []
    # the hood on the wall (x 0): a rounded box, the roller inside
    hood = parts.rbox('hood', (-0.11, 0.0, -0.11), (0.22, W + 0.1, 0.2), 0.04, 3)
    core.assign(hood, met)
    out.append(hood)
    out.append(box(-0.012, 0.0, -W / 2 - 0.05, W / 2 + 0.05, -0.24, -0.02, met, 0.003, 1))
    # the fabric: from under the hood (x -0.16, z -0.2) to the front bar (x -1.6, z -0.75), sagging between the arms
    nx, ny = 16, int(26 + 3.5 * W)
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new('UVMap')
    verts = []
    arm_ys = [(-W / 2 + 0.25) + (W - 0.5) * k / (npairs - 1) for k in range(npairs)] if npairs > 1 else [0.0]
    for i in range(nx + 1):
        t = i / nx
        row = []
        for j in range(ny + 1):
            s_ = j / ny
            yy = -W / 2 + W * s_
            xx = -0.16 + (-AWN_P + 0.16) * t
            zz = -0.2 + (-AWN_DROP - 0.2 + 0.2) * t
            # sag between arms and toward the middle of the run, small wrinkles off the bar
            da = min(abs(yy - a) for a in arm_ys)
            gap = (W - 0.5) / max(npairs - 1, 1)
            sag = 0.03 * math.sin(math.pi * t) * min(1.0, da / max(gap / 2, 0.3)) ** 1.5
            wr = 0.004 * math.sin(yy * 23.0 + t * 3.0) * math.sin(math.pi * t)
            row.append(bm.verts.new((xx, yy, zz - sag + wr)))
        verts.append(row)
    for i in range(nx):
        for j in range(ny):
            f = bm.faces.new((verts[i][j], verts[i + 1][j], verts[i + 1][j + 1], verts[i][j + 1]))
            for lp, (a, b) in zip(f.loops, ((i, j), (i + 1, j), (i + 1, j + 1), (i, j + 1))):
                # u left -> right seen from the street (-X looking +X: left is +Y), v 0 at the bar .. 1 at the wall
                lp[uvl].uv = (1.0 - b / ny, 1.0 - a / nx)
    sheet = core.mesh_object('fabric', bm)
    core.orient_normals(sheet, (0, 0, 1))
    # a thin double-sided fabric: solidify
    mod = sheet.modifiers.new('sol', 'SOLIDIFY')
    mod.thickness = 0.003
    core.select_only([sheet])
    bpy.ops.object.modifier_apply(modifier=mod.name)
    core.assign(sheet, fab)
    set_attr(sheet, 'ns_uvlock', 1.0)
    out.append(sheet)
    # the front bar and the valance hanging from it
    bx, bz = -AWN_P, -AWN_DROP - 0.2
    out.append(box(bx - 0.03, bx + 0.03, -W / 2, W / 2, bz - 0.04, bz + 0.02, met, 0.008, 2))
    vfront = print_quad('valance', [(bx - 0.032, W / 2, bz - 0.04), (bx - 0.032, -W / 2, bz - 0.04), (bx - 0.032, -W / 2, bz - 0.04 - AWN_VAL),
                                    (bx - 0.032, W / 2, bz - 0.04 - AWN_VAL)], (0.0, 0.0, 1.0, 1.0), (0, -1, 0), (0, 0, 1),
                        (bx - 0.032, W / 2, bz - 0.04 - AWN_VAL), (W, AWN_VAL), val)
    core.orient_normals(vfront, (-1, 0, 0))
    out.append(vfront)
    vback = box(bx - 0.03, bx - 0.027, -W / 2, W / 2, bz - 0.04 - AWN_VAL, bz - 0.04, fab, 0.0, 1)
    set_attr(vback, 'ns_uvlock', 1.0)
    lock_uvs(vback, lambda co: (0.5 - co.y / W, 0.01))
    out.append(vback)
    # folding arms: shoulder on the wall bracket, elbow, forearm to the bar
    for y in arm_ys:
        sh = (-0.05, y, -0.3)
        fr_ = (bx + 0.04, y, bz)
        el = (-0.85, y + (0.18 if y < 0 else -0.18), -0.36)
        out.append(_G.rod(sh, el, 0.022, met, 14))
        out.append(_G.rod(el, fr_, 0.02, met, 14))
        out.append(_G.lathe('elbow', [(0.0, -0.028), (0.032, -0.028), (0.034, 0.0), (0.032, 0.028), (0.0, 0.028)], met, 20,
                            Matrix.Translation(el)))
        out.append(_G.lathe('pin', [(0.0, 0.028), (0.012, 0.028), (0.012, 0.036), (0.0, 0.038)], met, 10, Matrix.Translation(el)))
        out.append(box(-0.04, 0.0, y - 0.07, y + 0.07, -0.38, -0.22, met, 0.006, 3))
        out.append(box(-0.075, -0.035, y - 0.03, y + 0.03, -0.33, -0.27, met, 0.006, 3))
        for dy in (-0.045, 0.045):
            for dz in (-0.35, -0.25):
                out.append(_G.lathe('bolt', [(0.0, 0.0), (0.009, 0.0), (0.009, 0.007), (0.0, 0.008)], met, 6,
                                    Matrix.Translation((-0.04, y + dy, dz)) @ rot_y(-90)))
        # the forearm's end fitting on the front bar
        out.append(box(bx + 0.02, bx + 0.07, y - 0.025, y + 0.025, bz - 0.03, bz + 0.025, met, 0.006, 3))
    # the front bar's end caps, the crank gearbox and its eye at the +Y end of the hood
    for sy in (-1, 1):
        out.append(box(bx - 0.034, bx + 0.034, sy * W / 2 - (0.006 if sy > 0 else -0.006) * 0 - 0.006, sy * W / 2 + 0.006, bz - 0.046, bz + 0.026,
                       met, 0.006, 3))
        out.append(box(-0.23, 0.0, sy * (W / 2 + 0.05) - 0.012, sy * (W / 2 + 0.05) + 0.012, -0.215, -0.0, met, 0.008, 3))
    gy = W / 2 - 0.12
    out.append(lathe('gear', [(0.0, -0.05), (0.035, -0.05), (0.04, -0.03), (0.04, 0.03), (0.035, 0.05), (0.0, 0.05)], met, 32,
                     Matrix.Translation((-0.13, gy, -0.215)) @ Matrix.Rotation(math.radians(90), 4, 'X')))
    out.append(rod((-0.13, gy, -0.25), (-0.13, gy, -0.3), 0.008, met, 16))
    out.append(tube([(-0.13, gy, -0.3), (-0.13, gy + 0.015, -0.315), (-0.13, gy, -0.33), (-0.13, gy - 0.015, -0.315), (-0.13, gy, -0.3)], 0.004,
                    met, 8, 4))
    return [finish_mesh(out, name, [fab, met, val], 35.0, tag=TAG)]


# ================================================================== SM_NS_BusShelter (+ _GlowCool)

SH_MAP = (0.0, 0.1, 0.49, 0.84)
SH_HEAD = (0.0, 0.0, 1.0, 0.0427)
SH_HEAD2 = (0.0, 0.047, 1.0, 0.0897)
SH_BADGE = (0.5, 0.1, 0.62, 0.28)
SH_DECAL = (0.64, 0.1, 1.0, 0.208)

STOPS = ['NORTHSIDE · FIFTH ST & MARKET', 'MARKET & 3RD', 'CARDINAL AVE', 'CIVIC CENTER', 'UNION STATION', 'RIVERFRONT', 'OLD MILL RD',
         'PINE FLATS', 'SWITCHBACK LOOKOUT', 'EMBERCREST RESORT']


def panel_route_map(s, w, h):
    s.rect(0, 0, w, h, 0xf3f2ee)
    s.rect(0, h * 0.9, w, h * 0.1, TRANSIT_GREEN)
    s.rect(0, h * 0.885, w, h * 0.015, TRANSIT_GOLD)
    s.text('CITY TRANSIT', w * 0.05, h * 0.93, h * 0.04, 0xffffff, 'Black', 'LEFT', tracking=1.05)
    s.circle(w * 0.87, h * 0.95, h * 0.035, 0xffffff)
    s.text('14', w * 0.87, h * 0.935, h * 0.035, TRANSIT_GREEN, 'Black', 'CENTER')
    s.text('ROUTE 14  ·  NORTHSIDE - EMBERCREST', w * 0.05, h * 0.84, h * 0.026, INK, 'Black', 'LEFT')
    s.text('You are here: Market St at Fifth', w * 0.05, h * 0.8, h * 0.02, 0x55585c, 'Bold', 'LEFT')
    x = w * 0.18
    y0, y1 = h * 0.72, h * 0.12
    s.rect(x - 7, y1, 14, y0 - y1, TRANSIT_GREEN)
    for i, st in enumerate(STOPS):
        y = y0 - (y0 - y1) * i / (len(STOPS) - 1)
        here = i == 0
        s.circle(x, y, 18 if here else 12, TRANSIT_GOLD if here else 0xffffff)
        s.ring(x, y, 12 if here else 8, 18 if here else 12, TRANSIT_GREEN)
        s.text(st, x + 40, y - 9, h * 0.019, INK, 'Black' if here else 'Bold', 'LEFT')
    s.text('Owl service 00:35 - 05:35 hourly', w * 0.05, h * 0.05, h * 0.018, 0x55585c, 'Bold', 'LEFT')
    s.text('Info 555-0114', w * 0.95, h * 0.05, h * 0.018, 0x55585c, 'Bold', 'RIGHT')


def panel_header(s, w, h):
    s.rect(0, 0, w, h, TRANSIT_GREEN)
    s.rect(0, 0, w, h * 0.12, TRANSIT_GOLD)
    s.text('CITY TRANSIT', w * 0.04, h * 0.32, h * 0.42, 0xffffff, 'Black', 'LEFT', tracking=1.08)
    s.circle(w * 0.93, h * 0.55, h * 0.36, 0xffffff)
    s.text('14', w * 0.93, h * 0.4, h * 0.34, TRANSIT_GREEN, 'Black', 'CENTER')
    s.text('MARKET ST at FIFTH', w * 0.6, h * 0.36, h * 0.28, 0xffffff, 'Bold', 'CENTER')


def panel_badge(s, w, h):
    s.rect(0, 0, w, h, TRANSIT_WHITE)
    s.circle(w / 2, h * 0.58, w * 0.36, TRANSIT_GREEN, n=96)
    s.ring(w / 2, h * 0.58, w * 0.3, w * 0.33, 0xffffff, n=96)
    s.text('14', w / 2, h * 0.58 - w * 0.15, w * 0.4, 0xffffff, 'Black', 'CENTER')
    s.text('EMBERCREST', w / 2, h * 0.14, w * 0.11, INK, 'Black', 'CENTER')


def panel_decal(s, w, h):
    s.rect(0, 0, w, h, 0xeeeeea)
    s.text('No smoking in the shelter', w / 2, h * 0.65, h * 0.13, INK, 'Bold', 'CENTER')
    s.text('Report damage 555-0117 · Shelter 14-0212', w / 2, h * 0.35, h * 0.1, 0x55585c, 'Bold', 'CENTER')


def ad_default():
    """The lightbox's default: RiverLine · the night shift never closes (a fictional poker app)."""
    s = Sheet(1200, 1750)
    s.rect(0, 0, 1200, 1750, 0x0d1424)
    for k in range(18):
        s.rect(0, 1750 * k / 18, 1200, 1750 / 18 + 1, 0x0d1424 if k % 2 else 0x101a2e)
    # the card fan
    for k, (dx, ang) in enumerate(((-150, 0.35), (0, 0.0), (150, -0.35))):
        cx, cy = 600 + dx, 1080
        c, sn = math.cos(ang), math.sin(ang)
        pts = [(cx + a * c - b * sn, cy + a * sn + b * c) for a, b in ((-140, -200), (140, -200), (140, 200), (-140, 200))]
        s.poly(pts, 0xf4f1ea)
        s.text(['A', 'K', 'Q'][k], cx - 90 * c + 150 * sn, cy - 90 * sn - 150 * c + 0, 70, 0xb3121b if k != 1 else 0x111111, 'Black', 'CENTER',
               rot=ang)
    s.text('RiverLine', 600, 600, 150, 0xf4f1ea, 'Black', 'CENTER')
    s.text('the night shift never closes', 600, 480, 60, 0x7fc6ff, 'Bold', 'CENTER')
    s.rect(380, 300, 440, 110, 0x2b7de9)
    s.text('PLAY NOW', 600, 330, 60, 0xffffff, 'Black', 'CENTER')
    s.text('18+  ·  Play responsibly  ·  help 555-0199', 600, 120, 34, 0x9aa6bb, 'Bold', 'CENTER')
    color, _ = s.render('nsk_sh_ad_default', 1024, samples=8)
    return color


def timetable_default():
    s = Sheet(800, 1200)
    s.rect(0, 0, 800, 1200, 0x0b0f14)
    s.rect(0, 1080, 800, 120, TRANSIT_GREEN)
    s.text('14  EMBERCREST', 40, 1120, 50, 0xffffff, 'Black', 'LEFT')
    s.text('MARKET ST at FIFTH', 40, 1030, 34, 0xe0e4e8, 'Bold', 'LEFT')
    times = ['06:05', '06:35', '07:05', '07:35', '08:05', '08:35', '09:05', '09:35', '10:05', '10:35']
    for i, t in enumerate(times):
        y = 940 - i * 82
        s.text(t, 60, y, 46, 0xffd24a if i == 0 else 0xe9edf1, 'Black', 'LEFT')
        s.text('EMBERCREST via MARKET', 230, y + 4, 30, 0xc5ccd3, 'Bold', 'LEFT')
        if i == 0:
            s.text('DUE', 740, y, 40, 0xffd24a, 'Black', 'RIGHT')
    color, _ = s.render('nsk_sh_tt_default', 800, samples=8)
    return color


def frit_dots(name, centers, normal_axis, r, mat, uv, n=12):
    """Ceramic frit dots (manifestation) fired onto glass: flat n-gons at the given 3D centers, facing +-X (normal_axis 'x+',
    'x-') or +-Y ('y+', 'y-'); all UVs at one point of a white atlas area."""
    bm = bmesh.new()
    for c in centers:
        vs = []
        for k in range(n):
            a = 2 * math.pi * k / n
            da, db = r * math.cos(a), r * math.sin(a)
            if normal_axis[0] == 'x':
                p = (c[0], c[1] + da, c[2] + db)
            else:
                p = (c[0] + da, c[1], c[2] + db)
            vs.append(bm.verts.new(p))
        f = bm.faces.new(vs)
        f.normal_update()
        want = {'x+': Vector((1, 0, 0)), 'x-': Vector((-1, 0, 0)), 'y+': Vector((0, 1, 0)), 'y-': Vector((0, -1, 0))}[normal_axis]
        if f.normal.dot(want) < 0:
            f.normal_flip()
    o = core.mesh_object(name, bm)
    core.assign(o, mat)
    lock_uvs(o, lambda co: uv)
    return o


def hex_bolt(mat, at, axis, size=0.008):
    """A hex bolt head with a washer, its axis along +-X/+-Y/+-Z ('x+' ...), seated on the surface at `at`."""
    rot = {'z+': Matrix(), 'z-': rot_y(180), 'x+': rot_y(90), 'x-': rot_y(-90), 'y+': Matrix.Rotation(math.radians(-90), 4, 'X'),
           'y-': Matrix.Rotation(math.radians(90), 4, 'X')}[axis]
    M = Matrix.Translation(Vector(at)) @ rot
    return [_G.lathe('washer', [(0.0, 0.0), (size * 1.6, 0.0), (size * 1.6, size * 0.25), (0.0, size * 0.25)], mat, 10, M),
            _G.lathe('bolt', [(0.0, size * 0.25), (size, size * 0.25), (size, size * 0.85), (size * 0.8, size), (0.0, size)], mat, 6, M)]


def build_bus_shelter():
    """CITY TRANSIT's standard shelter: see the module docstring."""
    name = 'SM_NS_BusShelter'
    fr = galv('nsk_sh_frame', 2048, grime=0.6, grime_height=0.5)
    tr = black_powder('nsk_sh_trim', 2048, ground=0.3)
    at = Atlas('nsk_sh_prints', 2048)
    at.put(SH_MAP, panel_route_map, 1000.0, 1500.0)
    at.put(SH_HEAD, panel_header, 4680.0, 200.0)
    at.put(SH_HEAD2, panel_header, 4680.0, 200.0)
    at.put(SH_BADGE, panel_badge, 400.0, 600.0)
    at.put(SH_DECAL, panel_decal, 1200.0, 360.0)
    pr = print_material('nsk_sh_prints', at.save(), 2048, lit_rects=(SH_MAP,), strength=3.0, rough=0.25)
    gl = glass_material('ns_glass')
    ad = screen_material('ns_screen_ad', ad_default(), emit=2.0)
    tt = screen_material('ns_screen_timetable', timetable_default(), emit=2.0)
    white_uv = (SH_DECAL[0] + 0.004, SH_DECAL[1] + 0.004)       # the decal's plain margin: the frit's white
    out, lit = [], []
    L, Hh = 4.6, 2.6
    xb, xf = 0.72, -0.72          # back and front post lines
    yE = L / 2 - 0.06             # end post centers
    # ---- posts: 100 x 100 galvanized tubes (rounded corners), anchor-bolted base plates, a cast foot cover
    posts = [(x, y) for x in (xb, xf) for y in (-yE, yE)] + [(xb, 0.0)]
    for (x, y) in posts:
        out.append(box(x - 0.05, x + 0.05, y - 0.05, y + 0.05, 0.0, 2.45, fr, 0.012, 4))
        out.append(box(x - 0.1, x + 0.1, y - 0.1, y + 0.1, 0.0, 0.014, fr, 0.003, 2))
        out.append(_G.lathe('foot', _round_profile([(0.0, 0.014), (0.088, 0.014), (0.084, 0.04), (0.075, 0.09), (0.0, 0.09)], 0.004), fr, 36,
                            Matrix.Translation((x, y, 0.0))))
        for dx in (-0.075, 0.075):
            for dy in (-0.075, 0.075):
                out += hex_bolt(fr, (x + dx, y + dy, 0.014), 'z+', 0.011)
                out.append(_G.lathe('stud', [(0.0, 0.025), (0.006, 0.025), (0.006, 0.034), (0.0, 0.036)], fr, 8,
                                    Matrix.Translation((x + dx, y + dy, 0.0))))
    # ---- the roof: a curved aluminum deck (thick, its top falling to the back), ribs under it, the fascia beams
    xa, xc = xf - 0.1, xb + 0.12

    def ztop(x):
        t = (x - xa) / (xc - xa)
        return 2.565 + 0.06 * math.sin(math.pi * t) - 0.03 * t

    N = 28
    top = [(xa + (xc - xa) * k / N, ztop(xa + (xc - xa) * k / N)) for k in range(N + 1)]
    deck = top + [(x, z - 0.03) for x, z in reversed(top)]
    out.append(prism_y('deck', deck, -L / 2 - 0.06, L / 2 + 0.06, fr))
    # the drip edge: a rolled lip under the deck's front and both ends
    out.append(tube([(xa + 0.004, -L / 2 - 0.06, ztop(xa) - 0.032), (xa + 0.004, L / 2 + 0.06, ztop(xa) - 0.032)], 0.009, fr, 16, 2))
    for sy in (-1, 1):
        yy = sy * (L / 2 + 0.06)
        out.append(tube([(x, yy, z - 0.032) for x, z in top[::2]], 0.007, fr, 12, 2))
    # transverse ribs (T sections) every 0.76 m, following the deck's underside
    for k in range(7):
        y = -L / 2 + 0.04 + (L - 0.08) * k / 6
        web = [(x, z - 0.03) for x, z in top[1:-1]] + [(x, z - 0.03 - 0.075 * (0.55 + 0.45 * math.sin(math.pi * (x - xa) / (xc - xa))))
                                                        for x, z in reversed(top[1:-1])]
        out.append(prism_y('rib', web, y - 0.005, y + 0.005, fr))
        flange = [(x, z - 0.03 - 0.075 * (0.55 + 0.45 * math.sin(math.pi * (x - xa) / (xc - xa)))) for x, z in top[1:-1]]
        out.append(tube([(x, y, z) for x, z in flange[::2]], 0.008, fr, 10, 2))
    # the front beam (the header both sides of it), the back beam, the side beams; bolts along the beams' faces
    out.append(box(xf - 0.1, xf - 0.06, -L / 2 - 0.05, L / 2 + 0.05, 2.36, 2.585, fr, 0.006, 3))
    out.append(face_quad('head', xf - 0.1006, 0.0, 2.47, L + 0.06, 0.2, (-1, 0), SH_HEAD, pr))
    out.append(box(xb + 0.08, xb + 0.12, -L / 2 - 0.05, L / 2 + 0.05, 2.36, 2.56, fr, 0.006, 3))
    out.append(face_quad('head2', xb + 0.1206, 0.0, 2.46, L + 0.06, 0.18, (1, 0), SH_HEAD2, pr))
    for sy in (-1, 1):
        out.append(box(xf - 0.06, xb + 0.08, sy * yE - 0.04, sy * yE + 0.04, 2.40, 2.50, fr, 0.006, 3))
    for k in range(13):
        y = -L / 2 + 0.15 + (L - 0.3) * k / 12
        out += hex_bolt(fr, (xf - 0.06, y, 2.39), 'x+', 0.007)
        out += hex_bolt(fr, (xb + 0.08, y, 2.385), 'x-', 0.007)
    # ---- the back wall: two panes in black gaskets between galvanized channels, a mid rail, frit dots at eye level
    panes = [((-L / 2 + 0.11, -0.05), 'back'), ((0.05, L / 2 - 0.11), 'back')]
    dots = []
    for (y0, y1), _ in panes:
        out.append(box(xb - 0.006, xb + 0.006, y0, y1, 0.17, 2.36, gl))
        for z0, z1 in ((0.10, 0.17), (2.36, 2.40)):
            out.append(box(xb - 0.024, xb + 0.024, y0 - 0.01, y1 + 0.01, z0, z1, fr, 0.004, 3))
        for sx in (-1, 1):
            xx = xb + sx * 0.006
            for (a0, a1, b0, b1) in ((y0, y1, 0.165, 0.18), (y0, y1, 2.35, 2.365), (y0, y0 + 0.012, 0.165, 2.365), (y1 - 0.012, y1, 0.165, 2.365)):
                out.append(box(min(xx, xx + sx * 0.005), max(xx, xx + sx * 0.005), a0, a1, b0, b1, tr, 0.0015, 2))
            # the kick plate: a brushed band low on the pane
            out.append(box(min(xx, xx + sx * 0.003), max(xx, xx + sx * 0.003), y0 + 0.02, y1 - 0.02, 0.18, 0.42, fr, 0.001, 2))
        n = int((y1 - y0 - 0.1) / 0.1)
        for row, z in enumerate((1.42, 1.50, 1.58)):
            for i in range(n):
                y = y0 + 0.05 + (y1 - y0 - 0.1) * (i + 0.5 * (row % 2)) / n
                if y < y1 - 0.05:
                    dots.append((y, z))
    out.append(box(xb - 0.028, xb + 0.028, -L / 2 + 0.11, L / 2 - 0.11, 1.08, 1.13, fr, 0.005, 3))
    out.append(frit_dots('dots_o', [(xb + 0.0062, y, z) for y, z in dots], 'x+', 0.024, pr, white_uv))
    out.append(frit_dots('dots_i', [(xb - 0.0062, y, z) for y, z in dots], 'x-', 0.024, pr, white_uv))
    # ---- the -Y end: a glass pane at the front half, the lightbox at the back half (both faces the ad)
    ye = -yE
    out.append(box(xf + 0.06, -0.14, ye - 0.006, ye + 0.006, 0.17, 2.36, gl))
    for z0, z1 in ((0.10, 0.17), (2.36, 2.40)):
        out.append(box(xf + 0.05, -0.13, ye - 0.024, ye + 0.024, z0, z1, fr, 0.004, 3))
    edots = []
    for row, z in enumerate((1.42, 1.50, 1.58)):
        for i in range(5):
            edots.append((xf + 0.13 + (-0.14 - xf - 0.2) * (i + 0.5 * (row % 2)) / 5, z))
    out.append(frit_dots('edots_o', [(x, ye - 0.0062, z) for x, z in edots], 'y-', 0.024, pr, white_uv))
    out.append(frit_dots('edots_i', [(x, ye + 0.0062, z) for x, z in edots], 'y+', 0.024, pr, white_uv))
    for sy in (-1, 1):
        yy = ye + sy * 0.006
        out.append(box(xf + 0.06, -0.14, min(yy, yy + sy * 0.005), max(yy, yy + sy * 0.005), 0.165, 0.18, tr, 0.0015, 2))
        out.append(box(xf + 0.06, -0.14, min(yy, yy + sy * 0.005), max(yy, yy + sy * 0.005), 2.35, 2.365, tr, 0.0015, 2))
    # the lightbox: a deep rounded housing, the ad framed on both faces, a lock, three barrel hinges, a vented top
    lx0, lx1 = -0.13, xb - 0.06
    out.append(box(lx0, lx1, ye - 0.085, ye + 0.085, 0.08, 2.42, tr, 0.02, 4))
    aw, ah = 1.2 * 0.62, 1.75 * 0.62
    adc = ((lx0 + lx1) / 2, 1.32)
    out.append(print_quad('ad1', [(adc[0] - aw / 2, ye - 0.0865, adc[1] - ah / 2), (adc[0] + aw / 2, ye - 0.0865, adc[1] - ah / 2),
                                  (adc[0] + aw / 2, ye - 0.0865, adc[1] + ah / 2), (adc[0] - aw / 2, ye - 0.0865, adc[1] + ah / 2)],
                          (0.0, 0.0, 1.0, 1.0), (1, 0, 0), (0, 0, 1), (adc[0] - aw / 2, ye - 0.0865, adc[1] - ah / 2), (aw, ah), ad))
    out.append(print_quad('ad2', [(adc[0] + aw / 2, ye + 0.0865, adc[1] - ah / 2), (adc[0] - aw / 2, ye + 0.0865, adc[1] - ah / 2),
                                  (adc[0] - aw / 2, ye + 0.0865, adc[1] + ah / 2), (adc[0] + aw / 2, ye + 0.0865, adc[1] + ah / 2)],
                          (0.0, 0.0, 1.0, 1.0), (-1, 0, 0), (0, 0, 1), (adc[0] + aw / 2, ye + 0.0865, adc[1] - ah / 2), (aw, ah), ad))
    for sgn in (-1, 1):
        yy = ye + sgn * 0.0855
        for (a0, a1, b0, b1) in ((adc[0] - aw / 2 - 0.05, adc[0] + aw / 2 + 0.05, adc[1] + ah / 2, adc[1] + ah / 2 + 0.05),
                                 (adc[0] - aw / 2 - 0.05, adc[0] + aw / 2 + 0.05, adc[1] - ah / 2 - 0.05, adc[1] - ah / 2),
                                 (adc[0] - aw / 2 - 0.05, adc[0] - aw / 2, adc[1] - ah / 2, adc[1] + ah / 2),
                                 (adc[0] + aw / 2, adc[0] + aw / 2 + 0.05, adc[1] - ah / 2, adc[1] + ah / 2)):
            out.append(box(a0, a1, min(yy, yy + sgn * 0.014), max(yy, yy + sgn * 0.014), b0, b1, fr, 0.004, 3))
        # the lock cylinder and the hinge barrels on the door's edges
        out.append(rod((adc[0] + aw / 2 + 0.025, yy + sgn * 0.014, 1.32), (adc[0] + aw / 2 + 0.025, yy + sgn * 0.024, 1.32), 0.011, fr, 24))
        for z in (0.45, 1.32, 2.15):
            out.append(rod((lx0 + 0.012, yy + sgn * 0.006, z - 0.06), (lx0 + 0.012, yy + sgn * 0.006, z + 0.06), 0.008, fr, 16))
    for k in range(9):
        xx = lx0 + 0.08 + (lx1 - lx0 - 0.16) * k / 8
        out.append(box(xx - 0.025, xx + 0.025, ye - 0.04, ye + 0.04, 2.42, 2.432, tr, 0.004, 2))
    out.append(rod((lx1 - 0.08, ye, 0.0), (lx1 - 0.08, ye, 0.09), 0.018, fr, 24))
    # ---- the route map inside the back wall, left bay seen from inside: a framed case with a glass front and a lock
    mx0, mx1 = -1.71, -1.05
    out.append(box(xb - 0.07, xb - 0.03, mx0 - 0.05, mx1 + 0.05, 1.12, 2.23, tr, 0.008, 3))
    out.append(face_quad('map', xb - 0.0705, (mx0 + mx1) / 2, 1.675, mx1 - mx0, 1.0, (-1, 0), SH_MAP, pr))
    out.append(box(xb - 0.082, xb - 0.074, mx0 - 0.045, mx1 + 0.045, 1.125, 2.225, gl))
    for (a0, a1, b0, b1) in ((mx0 - 0.05, mx1 + 0.05, 2.18, 2.23), (mx0 - 0.05, mx1 + 0.05, 1.12, 1.17),
                             (mx0 - 0.05, mx0, 1.12, 2.23), (mx1, mx1 + 0.05, 1.12, 2.23)):
        out.append(box(xb - 0.09, xb - 0.07, a0, a1, b0, b1, tr, 0.004, 3))
    out.append(rod((xb - 0.09, mx1 + 0.025, 1.65), (xb - 0.1, mx1 + 0.025, 1.65), 0.008, fr, 16))
    # ---- the timetable screen on the +Y back bay, facing the inside (-X): housing, bezel, help button, speaker slots
    ty = L / 2 - 0.42
    out.append(box(xb - 0.13, xb - 0.03, ty - 0.27, ty + 0.27, 1.12, 1.98, tr, 0.025, 4))
    out.append(box(xb - 0.14, xb - 0.12, ty - 0.23, ty + 0.23, 1.22, 1.88, tr, 0.006, 3))
    out.append(print_quad('tt', [(xb - 0.1405, ty + 0.2, 1.25), (xb - 0.1405, ty - 0.2, 1.25), (xb - 0.1405, ty - 0.2, 1.85),
                                 (xb - 0.1405, ty + 0.2, 1.85)],
                          (0.0, 0.0, 1.0, 1.0), (0, -1, 0), (0, 0, 1), (xb - 0.1405, ty + 0.2, 1.25), (0.4, 0.6), tt))
    out.append(lathe('help', [(0.0, 0.0), (0.02, 0.0), (0.02, 0.008), (0.014, 0.012), (0.0, 0.012)], fr, 32,
                     Matrix.Translation((xb - 0.13, ty, 1.16)) @ rot_y(-90)))
    for k in range(6):
        yy = ty - 0.1 + 0.04 * k
        out.append(box(xb - 0.1302, xb - 0.128, yy - 0.004, yy + 0.004, 1.86 + 0.06, 1.86 + 0.1, None))
    # ---- the 14 badge on the +Y front post (outside) and the decal on the back post
    out.append(face_quad('badge', xf - 0.0506, yE, 2.0, 0.09, 0.135, (-1, 0), SH_BADGE, pr))
    out.append(face_quad('decal', xb - 0.0506, 0.0, 0.9, 0.3, 0.09, (-1, 0), SH_DECAL, pr))
    # ---- the perch: a rounded, tilted aluminum rail on three cantilever brackets with gussets, end caps
    prof = []
    for k in range(32):
        a = 2 * math.pi * k / 32
        px, pz = 0.125 * math.cos(a), 0.028 * math.sin(a)
        ca, sa = math.cos(math.radians(-12)), math.sin(math.radians(-12))
        prof.append((xb - 0.2 + px * ca - pz * sa, 0.74 + px * sa + pz * ca))
    out.append(prism_y('perch', prof, -2.10, 2.19, fr))
    for y in (-2.10, 2.19):
        out.append(lathe('cap', [(0.0, 0.0), (0.03, 0.0), (0.028, 0.008), (0.0, 0.01)], tr, 24,
                         Matrix.Translation((xb - 0.2, y, 0.74)) @ Matrix.Rotation(math.radians(-90 if y > 0 else 90), 4, 'X')))
    for yy in (-2.09, 0.0, 2.13):
        out.append(box(xb - 0.32, xb - 0.06, yy - 0.012, yy + 0.012, 0.66, 0.72, tr, 0.004, 3))
        out.append(prism_y('gusset', [(xb - 0.06, 0.5), (xb - 0.06, 0.68), (xb - 0.22, 0.68)], yy - 0.008, yy + 0.008, tr))
        out.append(box(xb - 0.072, xb - 0.05, yy - 0.05, yy + 0.05, 0.48, 0.74, tr, 0.004, 3))
        for z in (0.53, 0.69):
            out += hex_bolt(fr, (xb - 0.072, yy + 0.03, z), 'x-', 0.006)
    # ---- the downspout: from the back beam down the +Y back post's outside, two straps, a shoe
    sx_, sy_ = xb + 0.105, yE - 0.07
    out.append(tube([(xb + 0.1, sy_, 2.42), (sx_, sy_, 2.36), (sx_, sy_, 2.3)], 0.028, fr, 32, 6))
    out.append(rod((sx_, sy_, 2.3), (sx_, sy_, 0.16), 0.028, fr, 32))
    out.append(tube([(sx_, sy_, 0.16), (sx_, sy_, 0.08), (sx_ + 0.08, sy_, 0.05), (sx_ + 0.14, sy_, 0.045)], 0.028, fr, 32, 8))
    for z in (0.6, 1.7):
        out.append(box(sx_ - 0.036, sx_ + 0.036, sy_ - 0.036, sy_ + 0.036, z - 0.015, z + 0.015, tr, 0.004, 2))
        out.append(box(xb + 0.05, sx_ - 0.03, sy_ - 0.012, sy_ + 0.012, z - 0.015, z + 0.015, tr, 0.003, 2))
    # ---- the downlight strip under the roof's front
    out.append(box(xf + 0.02, xf + 0.1, -L / 2 + 0.2, L / 2 - 0.2, 2.42, 2.45, tr, 0.004, 3))
    lit.append(box(xf + 0.03, xf + 0.09, -L / 2 + 0.25, L / 2 - 0.25, 2.415, 2.42, None))
    out2 = []
    for o in out:
        if not o.data.materials:
            core.assign(o, tr)
        out2.append(o)
    obj = finish_mesh(out2, name, [fr, tr, pr, gl, ad, tt], 35.0, tag=TAG)
    return [obj, glow(lit, f'{name}_GlowCool')]


# ================================================================== trees: SM_NS_Tree_A (London plane) / _B (honey locust)

LEAF_N = 4096
LEAF_CELLS = 4
TEX_NS = os.path.join(core.ROOT, 'unreal', 'Art', 'Textures', 'NS')


def _pip(px, py, poly):
    """Point-in-polygon for pixel arrays (even-odd)."""
    inside = np.zeros(px.shape, bool)
    n = len(poly)
    for i in range(n):
        x0, y0 = poly[i]
        x1, y1 = poly[(i + 1) % n]
        cond = (y0 > py) != (y1 > py)
        xi = x0 + (py - y0) * (x1 - x0) / ((y1 - y0) if abs(y1 - y0) > 1e-12 else 1e-12)
        inside ^= cond & (px < xi)
    return inside


def _seg_d(px, py, a, b):
    ax, ay = a
    bx, by = b
    dx, dy = bx - ax, by - ay
    t = np.clip(((px - ax) * dx + (py - ay) * dy) / max(dx * dx + dy * dy, 1e-12), 0.0, 1.0)
    return np.hypot(px - (ax + t * dx), py - (ay + t * dy)), t


def _blur(a, r):
    for _ in range(r):
        a = (a * 2 + np.roll(a, 1, 0) + np.roll(a, -1, 0) + np.roll(a, 1, 1) + np.roll(a, -1, 1)) / 6.0
    return a


def plane_leaf_outline(rng, n=400):
    """A London plane leaf in a unit frame: the petiole's end at (0, -0.5), blade up to y ~0.5; 5 lobes (3 big), toothed,
    the sinuses cut a third deep. Returns the outline and the lobe tips (for the veins)."""
    tips = [(0.0, 0.47), (-0.36, 0.18), (0.36, 0.18), (-0.43, -0.13), (0.43, -0.13)]
    base = (0.0, -0.3)
    pts = []
    angs = sorted([math.atan2(t[1] - base[1], t[0] - base[0]) for t in tips])
    tip_r = {}
    for t in tips:
        tip_r[round(math.atan2(t[1] - base[1], t[0] - base[0]), 6)] = math.hypot(t[0] - base[0], t[1] - base[1])
    lo = math.radians(-35)
    hi = math.radians(215)
    for k in range(n):
        a = lo + (hi - lo) * k / (n - 1)
        # distance to the nearest lobe direction sets the radius: lobes are points, sinuses deep
        best = None
        for ta in angs:
            d = abs(a - ta)
            w = max(0.0, 1.0 - d / 0.62)
            r = tip_r[round(ta, 6)] * (0.42 + 0.58 * w ** 1.4)
            best = r if best is None else max(best, r)
        teeth = 0.018 * max(0.0, math.sin(a * 46.0 + rng.uniform(0, 0.3))) ** 3
        r = best * (1.0 + rng.normal(0, 0.008)) + teeth
        pts.append((base[0] + r * math.cos(a), base[1] + r * math.sin(a)))
    # the base: a shallow cordate notch to the petiole
    pts.append((0.06, -0.33))
    pts.append((0.0, -0.36))
    pts.append((-0.06, -0.33))
    return pts, tips, base


def leaf_cell_plane(S, rng, palette):
    """One plane leaf in an S x S cell: (rgba linear, normal rgb, orm)."""
    c = (np.arange(S) + 0.5) / S - 0.5
    px, py = np.meshgrid(c, -c)      # row 0 at the top (y = +0.5)
    outline, tips, base = plane_leaf_outline(rng)
    sc = 0.92
    outline = [(x * sc, y * sc + 0.02) for x, y in outline]
    tips = [(x * sc, y * sc + 0.02) for x, y in tips]
    base = (base[0] * sc, base[1] * sc + 0.02)
    inside = _pip(px, py, outline).astype(np.float32)
    alpha = np.clip(_blur(inside, 1), 0, 1)
    # the petiole down to the cell's bottom middle
    dpet, _ = _seg_d(px, py, base, (0.0, -0.49))
    pet = (dpet < 0.012).astype(np.float32)
    alpha = np.maximum(alpha, pet)
    # veins: the five main veins, secondaries off them toward the margin
    vein = np.zeros_like(px)
    for t in tips:
        d, tt = _seg_d(px, py, base, t)
        w = 0.0075 * (1.0 - 0.7 * tt)
        vein = np.maximum(vein, np.clip(1.0 - d / w, 0, 1))
        L = math.hypot(t[0] - base[0], t[1] - base[1])
        ang = math.atan2(t[1] - base[1], t[0] - base[0])
        for k in range(1, 6):
            s = k / 6.5
            p0 = (base[0] + (t[0] - base[0]) * s, base[1] + (t[1] - base[1]) * s)
            for sgn in (-1, 1):
                a2 = ang + sgn * 0.75
                p1 = (p0[0] + math.cos(a2) * L * 0.28 * (1 - s * 0.6), p0[1] + math.sin(a2) * L * 0.28 * (1 - s * 0.6))
                d2, t2 = _seg_d(px, py, p0, p1)
                vein = np.maximum(vein, np.clip(1.0 - d2 / (0.003 * (1 - 0.6 * t2)), 0, 1) * 0.7)
    tone = tnoise_np(S, 6, rng.integers(1e6), 4)
    fine = tnoise_np(S, 60, rng.integers(1e6), 2)
    g0, g1, yel, brn = palette
    col = np.broadcast_to(lin(g0), (S, S, 3)) * (1 - tone[..., None]) + np.broadcast_to(lin(g1), (S, S, 3)) * tone[..., None]
    # autumn: yellowing from the margin in, between the veins; brown spots and a dried edge
    dm = _edge_dist(inside)
    yel_amt = rng.uniform(*palette_yel_range(palette))
    turn = np.clip(yel_amt * 1.6 - dm / (S * 0.12) * (1.0 - yel_amt) + (tone - 0.5) * 0.6, 0, 1) * (1 - 0.6 * vein)
    col = col * (1 - turn[..., None]) + lin(yel) * turn[..., None]
    spots = np.clip((tnoise_np(S, 30, rng.integers(1e6), 3) - 0.72) * 6, 0, 1) * rng.uniform(0.2, 1.0)
    col = col * (1 - spots[..., None] * 0.8) + lin(brn) * spots[..., None] * 0.8
    edge = np.clip(1.0 - dm / (S * 0.012), 0, 1) * rng.uniform(0.0, 0.9)
    col = col * (1 - edge[..., None]) + lin(brn) * edge[..., None]
    col = col * (1 - 0.35 * vein[..., None]) + (col * 1.35 + 0.02) * 0.35 * vein[..., None]
    col = col * (0.92 + 0.16 * fine[..., None])
    col = np.where(pet[..., None] > 0.5, lin(0x6b6a3a) * 0.9, col)
    h = vein * 0.6 + (fine - 0.5) * 0.15 - (dm / S) * 0.0
    nrm = _normal_np(h, 3.0)
    rough = 0.55 - 0.1 * vein + 0.1 * turn
    orm = np.stack([np.ones_like(h), rough, np.zeros_like(h)], 2)
    return np.dstack([col, alpha]), nrm, orm


def palette_yel_range(p):
    return (0.0, 0.25) if p[0] in (0x3f5a26, 0x4b6a2c) else (0.35, 0.95)


def _edge_dist(inside, iters=60):
    """Approximate distance (px) from each inside pixel to the outline (a few passes of erosion)."""
    d = np.zeros(inside.shape, np.float32)
    cur = inside > 0.5
    for k in range(iters):
        er = cur & np.roll(cur, 1, 0) & np.roll(cur, -1, 0) & np.roll(cur, 1, 1) & np.roll(cur, -1, 1)
        d += cur
        cur = er
        if not cur.any():
            break
    return d * 2.0


def tnoise_np(n, cells, seed, octaves=3):
    from ns_ground import tnoise
    return tnoise(n, cells, int(seed) % 100000, octaves)


def _normal_np(h, strength):
    gx = (np.roll(h, -1, 1) - np.roll(h, 1, 1)) * strength
    gy = (np.roll(h, -1, 0) - np.roll(h, 1, 0)) * strength
    n = np.stack([-gx, gy, np.ones_like(h)], 2)
    n /= np.linalg.norm(n, axis=2, keepdims=True)
    return n * 0.5 + 0.5


def leaf_cell_locust(S, rng, palette):
    """A honey locust compound leaf: a rachis up the cell with 18-26 leaflets in pairs (pinnate), tapering at the tip."""
    c = (np.arange(S) + 0.5) / S - 0.5
    px, py = np.meshgrid(c, -c)
    alpha = np.zeros_like(px)
    col = np.zeros((S, S, 3), np.float32)
    g0, g1, yel, brn = palette
    yel_amt = rng.uniform(*palette_yel_range(palette))
    # the rachis: a gentle curve from the bottom middle to the top
    bend = rng.uniform(-0.08, 0.08)
    rach = [(bend * (t ** 2) * 1.0, -0.48 + 0.95 * t) for t in np.linspace(0, 1, 20)]
    rmask = np.zeros_like(px)
    for a, b in zip(rach[:-1], rach[1:]):
        d, _ = _seg_d(px, py, a, b)
        rmask = np.maximum(rmask, np.clip(1.0 - d / 0.006, 0, 1))
    npairs = rng.integers(9, 13)
    for k in range(npairs):
        t = 0.12 + 0.84 * k / (npairs - 1)
        i = min(int(t * 19), 18)
        ax, ay = rach[i]
        tx, ty = rach[i + 1][0] - ax, rach[i + 1][1] - ay
        L = math.hypot(tx, ty)
        tx, ty = tx / L, ty / L
        size = 0.085 * (1.0 - 0.45 * t) + rng.normal(0, 0.006)
        for sgn in (-1, 1):
            ang = math.atan2(ty, tx) + sgn * math.radians(rng.uniform(55, 75))
            cx = ax + math.cos(ang) * size * 0.95
            cy = ay + math.sin(ang) * size * 0.95
            ca, sa = math.cos(ang), math.sin(ang)
            lx = (px - cx) * ca + (py - cy) * sa
            ly = -(px - cx) * sa + (py - cy) * ca
            e = (lx / size) ** 2 + (ly / (size * 0.36)) ** 2
            m = np.clip((1.0 - e) * 8.0, 0, 1)
            tone = rng.uniform(0, 1)
            turn = np.clip(yel_amt * 1.4 + rng.normal(0, 0.25), 0, 1)
            lc = lin(g0) * (1 - tone) + lin(g1) * tone
            lc = lc * (1 - turn) + lin(yel) * turn
            if rng.random() < 0.12:
                lc = lc * 0.5 + lin(brn) * 0.5
            midrib = np.clip(1.0 - np.abs(ly) / (size * 0.03), 0, 1) * (np.abs(lx) < size)
            shade_ = (0.88 + 0.2 * (1 - np.abs(ly) / (size * 0.36)).clip(0, 1))[..., None]
            upd = m > alpha
            col = np.where(upd[..., None], lc * shade_ * (1 + 0.25 * midrib[..., None]), col)
            alpha = np.maximum(alpha, m)
    col = np.where(rmask[..., None] > alpha[..., None], lin(0x7a7448) * 0.8, col)
    alpha = np.maximum(alpha, rmask)
    h = alpha * 0.3 + rmask * 0.4
    nrm = _normal_np(_blur(h, 2), 4.0)
    orm = np.stack([np.ones_like(h), np.full_like(h, 0.55), np.zeros_like(h)], 2)
    return np.dstack([col, alpha]), nrm, orm


PLANE_PAL = [(0x3f5a26, 0x587436, 0xb59a2a, 0x6b4a26), (0x4b6a2c, 0x6a8436, 0xc2a12e, 0x704c24), (0x5c7430, 0x7c8a3a, 0xc9a33a, 0x7a5228),
             (0x6a7a34, 0x8c9440, 0xd1a838, 0x80562a)]
LOCUST_PAL = [(0x4e6a24, 0x6a8a2e, 0xd8b830, 0x8a6a2a), (0x5c7426, 0x7a9030, 0xe0c03a, 0x8a6a2a)]


def leaf_atlas():
    """T_NS_Leaf_*: 4 x 4 cells of 1024. Cells 0-7 plane leaves (0-1 summer green, 2-5 turning, 6-7 yellow-brown), 8-15 honey
    locust compound leaves (8-10 green, 11-15 yellowing). Each leaf's petiole/rachis enters its cell at the bottom middle."""
    S = LEAF_N // LEAF_CELLS
    rgba = np.zeros((LEAF_N, LEAF_N, 4), np.float32)
    nrm = np.zeros((LEAF_N, LEAF_N, 3), np.float32)
    orm = np.zeros((LEAF_N, LEAF_N, 3), np.float32)
    rng = np.random.default_rng(1883)
    for k in range(16):
        r, cc = k // 4, k % 4
        if k < 8:
            pal = PLANE_PAL[min(k // 2, 3)]
            a, n_, o_ = leaf_cell_plane(S, rng, pal)
        else:
            pal = LOCUST_PAL[0 if k < 11 else 1]
            a, n_, o_ = leaf_cell_locust(S, rng, pal)
        rgba[r * S:(r + 1) * S, cc * S:(cc + 1) * S] = a
        nrm[r * S:(r + 1) * S, cc * S:(cc + 1) * S] = n_
        orm[r * S:(r + 1) * S, cc * S:(cc + 1) * S] = o_
    # bleed color into the transparent texels so mips don't halo
    col = rgba[..., :3]
    al = rgba[..., 3]
    for _ in range(12):
        m = al > 0.5
        acc = np.zeros_like(col)
        cnt = np.zeros(al.shape, np.float32)
        for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            ms = np.roll(m, (dy, dx), (0, 1))
            acc += np.where(ms[..., None], np.roll(col, (dy, dx), (0, 1)), 0)
            cnt += ms
        grow = (~m) & (cnt > 0)
        col[grow] = acc[grow] / cnt[grow][:, None]
        al = np.where(grow, 0.51, al)
    from ns_ground import lin2srgb
    out = np.dstack([lin2srgb(col), rgba[..., 3]])
    base = os.path.join(TEX_NS, 'T_NS_Leaf')
    _png(base + '_BaseColor.png', out)
    _png(base + '_Normal.png', nrm)
    _png(base + '_ORM.png', orm)
    log('wrote T_NS_Leaf_* (4096, RGBA: A coverage)')
    return base


def leaf_material(base):
    m = bpy.data.materials.get('ns_leaf')
    if m is not None:
        return m
    mm = core.Mat('ns_leaf')
    uvn = mm.node('ShaderNodeUVMap')
    uvn.uv_map = 'UVMap'
    img = mm.image(base + '_BaseColor.png', vector=uvn.outputs['UV'], extension='REPEAT')
    mm.link(img.outputs['Color'], mm.bsdf.inputs['Base Color'])
    rnd = mm.math('ROUND', img.outputs['Alpha'])
    mm.link(rnd, mm.bsdf.inputs['Alpha'])
    nimg = mm.image(base + '_Normal.png', non_color=True, vector=uvn.outputs['UV'])
    nm = mm.node('ShaderNodeNormalMap')
    mm.link(nimg.outputs['Color'], nm.inputs['Color'])
    mm.link(nm.outputs['Normal'], mm.bsdf.inputs['Normal'])
    mm.set('Roughness', 0.55)
    mm.set('Subsurface Weight', 0.0)
    mm.m.use_backface_culling = False
    try:
        mm.m.surface_render_method = 'DITHERED'
    except Exception:
        pass
    return mm.m


def bark_material(name, kind):
    """A: London plane's bark, flaking in camouflage patches (cream, olive, grey-brown) stretched up the trunk, finer and
    greener in the crown. B: honey locust's dark grey-brown bark in long scaly plates between shallow furrows."""
    tex(name, 2048)
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    z = sep.outputs['Z']
    up = scaled(m, sep, 9.0, 9.0, 3.5)
    if kind == 'A':
        vor = m.node('ShaderNodeTexVoronoi')
        vor.feature = 'F1'
        vor.inputs['Scale'].default_value = 1.0
        m.link(up, vor.inputs['Vector'])
        cell = vor.outputs['Color']
        sp = m.node('ShaderNodeSeparateColor')
        m.link(cell, sp.inputs['Color'])
        h = sp.outputs['Red']
        vor2 = m.node('ShaderNodeTexVoronoi')
        vor2.feature = 'DISTANCE_TO_EDGE'
        vor2.inputs['Scale'].default_value = 1.0
        m.link(up, vor2.inputs['Vector'])
        edge = ramp(m, vor2.outputs['Distance'], 0.06, 0.0)
        c1 = m.mix(ramp(m, h, 0.0, 0.33), H(0xc9c1a1), H(0x8e8b6c))
        c2 = m.mix(ramp(m, h, 0.33, 0.66), c1, H(0x6f725a))
        col = m.mix(ramp(m, h, 0.7, 0.9), c2, H(0x9a8f78))
        mott = noise(m, o, 30.0, 4.0)
        col = m.mix(m.math('MULTIPLY', mott, 0.3), col, H(0x5d5a48))
        col = m.mix(m.math('MULTIPLY', edge, 0.8), col, H(0x4a4536))
        # grime and lichen at the base, the crown's twigs browner
        base_g = m.math('MULTIPLY', ramp(m, z, 0.8, 0.0), ramp(m, noise(m, o, 6.0, 4.0), 0.3, 0.7))
        col = m.mix(base_g, col, H(0x3b3a33))
        twig = ramp(m, z, 4.0, 7.0)
        col = m.mix(m.math('MULTIPLY', twig, 0.45), col, H(0x6a5f4c))
        height = m.math('ADD', m.math('MULTIPLY', edge, -0.6), m.math('MULTIPLY', mott, 0.3))
        rough = 0.82
    else:
        streak = noise(m, scaled(m, sep, 22.0, 22.0, 2.0), 1.0, 4.0, distortion=1.2)
        fur = ramp(m, streak, 0.42, 0.62)
        scale_n = noise(m, o, 45.0, 3.0)
        col = m.mix(fur, H(0x2a2520), H(0x564c42))
        col = m.mix(m.math('MULTIPLY', ramp(m, scale_n, 0.6, 0.8), 0.4), col, H(0x6e655a))
        lich = m.math('MULTIPLY', ramp(m, noise(m, o, 8.0, 4.0), 0.62, 0.72), 0.5)
        col = m.mix(lich, col, H(0x7c8064))
        base_g = m.math('MULTIPLY', ramp(m, z, 0.6, 0.0), 0.7)
        col = m.mix(base_g, col, H(0x26231f))
        height = m.math('ADD', fur, m.math('MULTIPLY', scale_n, 0.3))
        rough = 0.88
    m.set('Base Color', col)
    m.set('Roughness', rough)
    set_bump(m, height, 0.8, 0.006)
    return m


class Limb:
    def __init__(self, pts, radii, order):
        self.pts, self.radii, self.order = pts, radii, order


def _perp(d):
    a = Vector((0, 0, 1)) if abs(d.z) < 0.9 else Vector((1, 0, 0))
    return d.cross(a).normalized()


def grow_tree(kind, seed):
    """The skeleton: a list of Limbs (polylines with radii), orders 0 trunk .. 4 twigs."""
    rng = random.Random(seed)
    limbs = []
    if kind == 'A':
        H_, split, nscaf = 9.0, 2.9, 6
        r0 = 0.17
    else:
        H_, split, nscaf = 8.0, 2.3, 4
        r0 = 0.14

    def walk(p, d, length, ra, rb, order, n, up=0.2, wob=0.12, droop=0.0):
        pts, rad = [p.copy()], [ra]
        seg = length / n
        for i in range(n):
            d = (d + Vector((rng.gauss(0, wob), rng.gauss(0, wob), rng.gauss(0, wob))) + Vector((0, 0, up - droop * (i / n)))).normalized()
            p = p + d * seg
            pts.append(p.copy())
            rad.append(ra + (rb - ra) * (i + 1) / n)
        return pts, rad

    # trunk, a slight lean and sweep
    lean = Vector((rng.uniform(-0.05, 0.05), rng.uniform(-0.05, 0.05), 1.0)).normalized()
    tp, tr = walk(Vector((0, 0, -0.1)), lean, H_ * (0.92 if kind == 'A' else 0.7), r0, 0.05 if kind == 'A' else 0.06, 0, 14, up=0.05, wob=0.025)
    limbs.append(Limb(tp, tr, 0))
    trunk = limbs[0]

    def point_on(limb, t):
        f = t * (len(limb.pts) - 1)
        i = min(int(f), len(limb.pts) - 2)
        a = f - i
        p = limb.pts[i].lerp(limb.pts[i + 1], a)
        r = limb.radii[i] + (limb.radii[i + 1] - limb.radii[i]) * a
        d = (limb.pts[i + 1] - limb.pts[i]).normalized()
        return p, r, d

    def children(parent, count, t0, t1, ang0, ang1, len_fn, r_scale, order, n_seg, up, wob, droop, spin0=None):
        made = []
        spin = rng.uniform(0, 2 * math.pi) if spin0 is None else spin0
        for k in range(count):
            t = t0 + (t1 - t0) * (k + rng.uniform(0.1, 0.9)) / count
            p, r, d = point_on(parent, t)
            spin += 2.39996 + rng.gauss(0, 0.3)       # the golden angle round the parent
            side = _perp(d)
            side = Matrix.Rotation(spin, 3, d) @ side
            ang = math.radians(rng.uniform(ang0, ang1))
            nd = (d * math.cos(ang) + side * math.sin(ang)).normalized()
            L = len_fn(t)
            ra = max(r * r_scale, 0.004)
            pts, rad = walk(p - nd * r * 0.5, nd, L, ra, max(ra * 0.25, 0.003), order, n_seg, up=up, wob=wob, droop=droop)
            lb = Limb(pts, rad, order)
            limbs.append(lb)
            made.append(lb)
        return made

    if kind == 'A':
        scafs = children(trunk, nscaf, split / H_ / 0.92, 0.62, 35, 58, lambda t: rng.uniform(3.6, 4.6) * (1.15 - t * 0.4), 0.62, 1, 9, 0.12, 0.05, 0.0)
        tops = [trunk] + scafs
    else:
        # the vase: the trunk splits at 2.3 m into 3 leaders up and out, the trunk itself stops
        trunk.pts = trunk.pts[:5]
        trunk.radii = trunk.radii[:5]
        scafs = children(trunk, nscaf, 0.92, 1.0, 22, 40, lambda t: rng.uniform(4.6, 5.6), 0.75, 1, 10, 0.08, 0.06, 0.02)
        tops = scafs
    secs = []
    for s in tops:
        o = 1 if s is trunk else 2
        cnt = 8 if kind == 'A' else 9
        t0 = 0.55 if s is trunk else 0.2
        secs += children(s, cnt, t0, 0.95, 40, 70, lambda t: rng.uniform(1.4, 2.2) * (1.1 - 0.5 * t), 0.5, o if o == 2 else 2, 6, 0.05,
                         0.1, 0.12 if kind == 'B' else 0.03)
    ters = []
    for s in secs:
        ters += children(s, 5 if kind == 'A' else 6, 0.25, 0.98, 35, 65, lambda t: rng.uniform(0.6, 1.0) * (1.15 - 0.4 * t), 0.55, 3, 4, 0.04,
                         0.14, 0.2 if kind == 'B' else 0.05)
    twigs = []
    for s in ters:
        twigs += children(s, 3, 0.35, 0.95, 30, 60, lambda t: rng.uniform(0.25, 0.45), 0.6, 4, 3, 0.03, 0.18, 0.25 if kind == 'B' else 0.08)
    return limbs, ters + twigs, rng


def limb_mesh(limbs, crown_c):
    """Tubes for every limb (sides by radius), the base of each sunk into its parent. UVs left to the bake's layout;
    COLOR_0.R (wind) per vertex by the limb's order and its distance along."""
    bm = bmesh.new()
    col = bm.loops.layers.float_color.new('Color')
    wind_of = {0: 0.0, 1: 0.12, 2: 0.35, 3: 0.65, 4: 0.85}
    for lb in limbs:
        r0 = lb.radii[0]
        sides = 18 if r0 > 0.1 else 12 if r0 > 0.04 else 7 if r0 > 0.015 else 5
        n = len(lb.pts)
        rings = []
        d0 = (lb.pts[1] - lb.pts[0]).normalized()
        nrm = _perp(d0)
        for i in range(n):
            a = lb.pts[max(i - 1, 0)]
            b = lb.pts[min(i + 1, n - 1)]
            t = (b - a).normalized()
            if i > 0:
                nrm = (nrm - t * nrm.dot(t)).normalized()
            bn = t.cross(nrm)
            r = lb.radii[i]
            # a root flare at the trunk's foot
            if lb.order == 0 and i <= 2:
                r *= 1.0 + 0.35 * (1 - i / 2)
            rings.append([bm.verts.new(lb.pts[i] + (nrm * math.cos(2 * math.pi * k / sides) + bn * math.sin(2 * math.pi * k / sides)) * r *
                                       (1.0 + (0.06 * math.sin(k * 2.3 + i) if lb.order == 0 else 0.0))) for k in range(sides)])
        w0 = wind_of[lb.order]
        w1 = wind_of[min(lb.order + 1, 4)]
        for i in range(n - 1):
            for k in range(sides):
                f = bm.faces.new((rings[i][k], rings[i][(k + 1) % sides], rings[i + 1][(k + 1) % sides], rings[i + 1][k]))
                for lp in f.loops:
                    t = (i + (lp.vert in rings[i + 1])) / max(n - 1, 1)
                    w = w0 + (w1 - w0) * t
                    lp[col] = (w, 1.0, 1.0, 1.0)
        tipv = bm.verts.new(lb.pts[-1] + (lb.pts[-1] - lb.pts[-2]).normalized() * lb.radii[-1])
        for k in range(sides):
            f = bm.faces.new((rings[-1][k], rings[-1][(k + 1) % sides], tipv))
            for lp in f.loops:
                lp[col] = (w1, 1.0, 1.0, 1.0)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    return core.mesh_object('limbs', bm)


def leaves_mesh(kind, hosts, rng, crown_c, count):
    """Leaves (A: single plane leaves 16-24 cm, B: compound locust leaves 22-30 cm) on the twigs: each a quad bent along
    its midrib (4 tris) UV'd into an atlas cell, its petiole at the twig; facing outward and up, some drooping; normals
    bent outward from the crown's center for soft foliage light; COLOR_0.R wind 0.9..1 (more at the tip)."""
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new('UVMap')
    col = bm.loops.layers.float_color.new('Color')
    lens = [sum((lb.pts[i + 1] - lb.pts[i]).length for i in range(len(lb.pts) - 1)) for lb in hosts]
    total = sum(lens)
    placed = 0
    nrm_target = []
    for lb, L in zip(hosts, lens):
        k = max(1, int(round(count * L / total)))
        for j in range(k):
            t = rng.uniform(0.15, 1.0) if lb.order == 3 else rng.uniform(0.0, 1.0)
            f = t * (len(lb.pts) - 1)
            i = min(int(f), len(lb.pts) - 2)
            p = lb.pts[i].lerp(lb.pts[i + 1], f - i)
            d = (lb.pts[i + 1] - lb.pts[i]).normalized()
            out = (p - crown_c)
            out.z *= 0.6
            out = out.normalized()
            side = _perp(d)
            side = Matrix.Rotation(rng.uniform(0, 2 * math.pi), 3, d) @ side
            # the leaf's axis: off the twig, outward, a little up, with droop
            ax = (side * 0.6 + out * 0.5 + d * 0.3 + Vector((0, 0, rng.uniform(-0.4, 0.15)))).normalized()
            size = (rng.uniform(0.16, 0.24) if kind == 'A' else rng.uniform(0.22, 0.3))
            width = size * (1.0 if kind == 'A' else 0.55)
            # the blade's normal: mostly up and out
            up = (Vector((0, 0, 1)) * 0.7 + out * 0.5 + Vector((rng.gauss(0, 0.3), rng.gauss(0, 0.3), 0))).normalized()
            rt = ax.cross(up).normalized()
            up = rt.cross(ax).normalized()
            fold = rng.uniform(0.08, 0.25) * width
            base = p + ax * 0.01
            # cell: plane 0..7 (weighted to the turning ones), locust 8..15
            if kind == 'A':
                cell = rng.choices(range(8), weights=[5, 5, 3, 3, 2, 2, 2, 1])[0]      # about 30 % in the yellow-brown cells 4-7
            else:
                cell = rng.choices(range(8, 16), weights=[4, 4, 4, 2, 2, 1, 1, 1])[0]  # about a third yellowing (11-15)
            cu, cv = cell % 4, 3 - cell // 4      # row 0 of the atlas is its top (v 1)
            u0, v0 = cu / 4.0, cv / 4.0
            # 9 vertices (3 across x 3 along): folded on the midrib, drooping toward the tip, the tip's half twisted a little
            corners = [(a, b) for b in (0.0, 0.5, 1.0) for a in (-0.5, 0.0, 0.5)]
            droop = rng.uniform(0.03, 0.12) * size
            twist = rng.uniform(-0.35, 0.35)
            vs = []
            for (a, b) in corners:
                aw = a * width
                q = base + rt * (aw * math.cos(twist * b)) + ax * (b * size) + up * (fold * (1 - abs(a) * 2) + aw * math.sin(twist * b))                     - up * (droop * b * b)
                vs.append(bm.verts.new(q))
            quads = [(0, 1, 4, 3), (1, 2, 5, 4), (3, 4, 7, 6), (4, 5, 8, 7)]
            for qd in quads:
                fc = bm.faces.new([vs[x] for x in qd])
                for lp, x in zip(fc.loops, qd):
                    a, b = corners[x]
                    lp[uvl].uv = (u0 + (0.5 + a) * 0.25, v0 + b * 0.25)
                    lp[col] = (0.88 + 0.12 * b, 1.0, 1.0, 1.0)
            nrm_target.append((placed, up, out))
            placed += 1
    obj = core.mesh_object('leaves', bm)
    return obj


def bend_normals(obj, crown_c, amount=0.55):
    """Custom normals: each leaf's face normal bent toward the direction out from the crown's center."""
    me = obj.data
    me.update()
    normals = []
    for poly in me.polygons:
        fn = Vector(poly.normal)
        c = Vector(poly.center)
        out = (c - crown_c)
        out.z *= 0.7
        out = out.normalized()
        if fn.dot(out) < 0:
            fn = -fn
        n = (fn * (1 - amount) + out * amount).normalized()
        for _ in poly.loop_indices:
            normals.append(n)
    try:
        me.normals_split_custom_set(normals)
    except Exception:
        traceback.print_exc()


def build_tree(kind):
    name = f'SM_NS_Tree_{kind}'
    base = leaf_atlas() if not os.path.exists(os.path.join(TEX_NS, 'T_NS_Leaf_BaseColor.png')) or not getattr(build_tree, '_atlas', False) else \
        os.path.join(TEX_NS, 'T_NS_Leaf')
    build_tree._atlas = True
    bark = bark_material(f'nsk_tr{kind.lower()}_bark', kind)
    leaf = leaf_material(base)
    limbs, hosts, rng = grow_tree(kind, 1711 if kind == 'A' else 2203)
    crown_c = Vector((0.0, 0.0, 5.6 if kind == 'A' else 5.2))
    lm = limb_mesh(limbs, crown_c)
    core.assign(lm, bark)
    lv = leaves_mesh(kind, hosts, rng, crown_c, 14000 if kind == 'A' else 10500)
    core.assign(lv, leaf)
    # joined after the limbs get their bake UVs
    core.finish_hard_surface(lm, 60.0, weighted=False)
    core.uv_layout(lm, [(lambda f: True, 'smart', None, (0.0, 0.0, 1.0, 1.0))])
    set_attr(lm, 'ns_uvlock', 1.0)
    set_attr(lv, 'ns_uvlock', 1.0)
    obj = core.join([lm, lv], name)
    obj.name = name
    obj.data.name = name
    from ns_ground import order_slots
    order_slots(obj, [bark.m, leaf], TAG)
    lv_faces = [p.index for p in obj.data.polygons if p.material_index == 1]
    bend_normals_subset(obj, crown_c, set(lv_faces))
    if 'ns_uvlock' in obj.data.attributes:
        obj.data.attributes.remove(obj.data.attributes['ns_uvlock'])
    ca = obj.data.color_attributes.get('Color')
    if ca is not None:
        obj.data.color_attributes.active_color = ca
        obj.data.color_attributes.render_color_index = obj.data.color_attributes.find('Color')
    log(f'{name} slots: {[s.material.name for s in obj.material_slots]}  tris {tri_count(obj)}')
    return [obj]


def bend_normals_subset(obj, crown_c, leaf_faces, amount=0.55):
    me = obj.data
    normals = []
    for poly in me.polygons:
        fn = Vector(poly.normal)
        if poly.index in leaf_faces:
            c = Vector(poly.center)
            out = c - crown_c
            out.z *= 0.7
            out = out.normalized()
            n = (fn * (1 - amount) + out * amount * (1 if fn.dot(out) >= 0 else -1)).normalized()
            nl = [n] * poly.loop_total
        else:
            nl = [me.vertices[me.loops[li].vertex_index].normal.copy() for li in poly.loop_indices]
        normals += nl
    try:
        me.normals_split_custom_set(normals)
    except Exception:
        traceback.print_exc()


# ================================================================== P1: cabinets, meters, boxes, the shed, the steam, bags, litter

def sticker_panel(seed, kinds=('tag', 'flyer', 'sticker')):
    """A panel of street stickers and a tag (fictional names), for cabinets and poles."""
    rng = random.Random(seed)

    def draw(s, w, h):
        s.rect(0, 0, w, h, 0x000000)
        for k in range(9):
            x, y = rng.uniform(0, w * 0.8), rng.uniform(0, h * 0.85)
            sw, sh = rng.uniform(w * 0.08, w * 0.22), rng.uniform(h * 0.05, h * 0.14)
            c = rng.choice([0xf2f0ea, 0xe8d23a, 0x2a2a2e, 0xd8462e, 0x4a90c8, 0xf0a8c0])
            s.rect(x, y, sw, sh, c)
            s.text(rng.choice(['SKATE OR DIE', 'NO BAD DAYS', 'RIVERLINE', 'PEACE', 'WE R HERE', 'LOST CAT 555-0133', 'BAND TONITE']),
                   x + sw / 2, y + sh * 0.3, sh * 0.35, 0x111111 if c != 0x2a2a2e else 0xeeeeee, 'Black', 'CENTER')
        # a marker tag
        tx, ty = rng.uniform(w * 0.1, w * 0.4), rng.uniform(h * 0.3, h * 0.6)
        s.text(rng.choice(['KRAYO', 'DUSK', 'NEBS', 'ZORBO']), tx, ty, h * 0.12, 0xe8e8e8, 'Black', 'LEFT', rot=0.12, tracking=0.8)
    return draw


def build_signal_cabinet():
    name = 'SM_NS_SignalCabinet'
    cab = painted('nsk_scb_cab', 0x8f9488, 1024, rough=0.45, chips=0.7, rust=0.4, grime=0.9, grime_height=0.5, streaks=0.8)
    pad = concrete_mat('nsk_scb_pad', 0x9a968e, 512)
    at = Atlas('nsk_scb_prints', 1024)
    at.put((0.0, 0.0, 1.0, 0.6), sticker_panel(51), 1000.0, 600.0)

    def plate(s, w, h):
        s.rect(0, 0, w, h, 0xd9d6cc)
        s.text('TRAFFIC SIGNAL CONTROL', w / 2, h * 0.62, h * 0.16, INK, 'Black', 'CENTER')
        s.text('CITY OF NORTHSIDE  ·  CAB 212-F', w / 2, h * 0.38, h * 0.11, INK, 'Bold', 'CENTER')
        s.text('EMERGENCY 555-0110', w / 2, h * 0.15, h * 0.11, 0xa01818, 'Bold', 'CENTER')
    at.put((0.0, 0.62, 1.0, 0.9), plate, 1000.0, 280.0)
    pr = print_material('nsk_scb_prints', at.save(), 1024, rough=0.35, weather=0.8)
    out = []
    out.append(box(-0.4, 0.4, -0.7, 0.7, 0.0, 0.12, pad, 0.02, 2))
    W, D, Hh = 1.2, 0.6, 1.4
    out.append(box(-D / 2, D / 2, -W / 2, W / 2, 0.12, 0.12 + Hh, cab, 0.012, 2))
    out.append(box(-D / 2 - 0.02, D / 2 + 0.02, -W / 2 - 0.02, W / 2 + 0.02, 0.12 + Hh, 0.12 + Hh + 0.04, cab, 0.01, 2))
    # two doors on the front (-X) with their hinges, a handle, louvers
    for i, (y0, y1) in enumerate(((-0.58, -0.01), (0.01, 0.58))):
        out.append(box(-D / 2 - 0.012, -D / 2, y0, y1, 0.16, 0.12 + Hh - 0.04, cab, 0.004, 1))
        for k in range(6):
            z = 1.05 + 0.04 * k
            out.append(prism_y('louver', [(-D / 2 - 0.012, z), (-D / 2 - 0.03, z + 0.004), (-D / 2 - 0.03, z + 0.016), (-D / 2 - 0.012, z + 0.022)],
                               y0 + 0.08, y1 - 0.08, cab))
        out.append(box(-D / 2 - 0.016, -D / 2 - 0.012, y0 + 0.07, y1 - 0.07, 1.035, 1.045, cab, 0.0015, 2))
        out.append(box(-D / 2 - 0.016, -D / 2 - 0.012, y0 + 0.07, y1 - 0.07, 1.3, 1.31, cab, 0.0015, 2))
        yh = y0 - 0.006 if i == 0 else y1 + 0.006
        for zz in (0.3, 0.8, 1.3):
            out.append(rod((-D / 2 - 0.006, yh, zz - 0.05), (-D / 2 - 0.006, yh, zz + 0.05), 0.009, cab, 20))
    out.append(box(-D / 2 - 0.05, -D / 2 - 0.012, -0.05, -0.03, 0.75, 0.95, cab, 0.006, 1))
    out.append(lathe('lockcyl', [(0.0, 0.0), (0.014, 0.0), (0.014, 0.008), (0.011, 0.012), (0.0, 0.012)], cab, 24,
                     Matrix.Translation((-D / 2 - 0.012, -0.08, 0.98)) @ rot_y(-90)))
    # a vent grille low on the -Y end: a frame and slats
    out.append(box(-0.16, 0.16, -W / 2 - 0.01, -W / 2, 0.25, 0.55, cab, 0.004, 3))
    for k in range(10):
        zz = 0.27 + 0.26 * k / 9
        out.append(prism_x('slat', [(-W / 2 - 0.004, zz - 0.012), (-W / 2 - 0.018, zz - 0.004), (-W / 2 - 0.018, zz), (-W / 2 - 0.004, zz - 0.008)],
                           -0.14, 0.14, cab))
    # the police panel (a small door with its own lock) on the +Y end, the rain cap's drip edge, anchor bolts in the pad
    out.append(box(-0.12, 0.12, W / 2, W / 2 + 0.01, 0.9, 1.15, cab, 0.004, 3))
    out.append(lathe('ppanel', [(0.0, 0.0), (0.012, 0.0), (0.012, 0.008), (0.0, 0.01)], cab, 24,
                     Matrix.Translation((0.08, W / 2 + 0.01, 1.025)) @ Matrix.Rotation(math.radians(-90), 4, 'X')))
    for sy in (-1, 1):
        for sx in (-1, 1):
            out.append(lathe('anchor', [(0.0, 0.0), (0.014, 0.0), (0.014, 0.012), (0.0, 0.012)], cab, 6,
                             Matrix.Translation((sx * (D / 2 + 0.04), sy * (W / 2 - 0.08), 0.12))))
    out.append(rod((D / 2 + 0.05, 0.3, 0.0), (D / 2 + 0.05, 0.3, 0.2), 0.03, cab, 32))
    out.append(tube([(D / 2 + 0.05, 0.3, 0.2), (D / 2 + 0.05, 0.3, 0.28), (D / 2 - 0.02, 0.3, 0.32)], 0.03, cab, 32, 6))
    out.append(face_quad('stickers', -D / 2 - 0.0125, 0.29, 0.6, 0.5, 0.3, (-1, 0), (0.0, 0.0, 1.0, 0.6), pr))
    out.append(face_quad('plate', -D / 2 - 0.0125, -0.3, 1.3, 0.3, 0.084, (-1, 0), (0.0, 0.62, 1.0, 0.9), pr))
    return [finish_mesh(out, name, [cab, pad, pr], 35.0, tag=TAG)]


def meter_default():
    s = Sheet(400, 260)
    s.rect(0, 0, 400, 260, 0x0d1a12)
    s.text('PAY HERE', 200, 190, 36, 0x9ef7b0, 'Black', 'CENTER')
    s.text('$2.50 / HR  ·  2 HR MAX', 200, 130, 22, 0x9ef7b0, 'Bold', 'CENTER')
    s.text('8AM - 7PM MON-SAT', 200, 80, 22, 0x9ef7b0, 'Bold', 'CENTER')
    s.text('ZONE 212', 200, 30, 22, 0x9ef7b0, 'Bold', 'CENTER')
    return s.render('nsk_mm_default', 512, samples=4)[0]


def build_muni_meter():
    name = 'SM_NS_MuniMeter'
    body = painted('nsk_mm_body', 0x2a4a72, 1024, rough=0.4, chips=0.5, rust=0.3, grime=0.7, grime_height=0.4, streaks=0.6)
    grey = galv('nsk_mm_steel', 512, grime=0.5)
    scr = screen_material('ns_screen_meter', meter_default(), emit=1.5)
    at = Atlas('nsk_mm_prints', 1024, 512)

    def face(s, w, h):
        # laid out round the hardware on it (the face spans z 0.85..1.25): the keypad z 1.12..1.21 in the middle with the
        # green and red keys either side, the coin slot at the left z 1.05..1.11, the receipt slot z 0.92..0.95
        s.rect(0, 0, w, h, 0xe8e6df)
        s.rect(0, h * 0.925, w, h * 0.075, 0x2a4a72)
        s.text('PARKING', w / 2, h * 0.94, h * 0.045, 0xffffff, 'Black', 'CENTER', tracking=1.15)
        s.text('COINS', w * 0.1, h * 0.475, h * 0.02, INK, 'Bold', 'CENTER')
        s.text('1  Insert card or coins', w * 0.06, h * 0.43, h * 0.028, INK, 'Bold', 'LEFT')
        s.text('2  Enter your space number', w * 0.06, h * 0.39, h * 0.028, INK, 'Bold', 'LEFT')
        s.text('3  Press GREEN for receipt', w * 0.06, h * 0.35, h * 0.028, INK, 'Bold', 'LEFT')
        s.text('4  Display on dashboard', w * 0.06, h * 0.31, h * 0.028, INK, 'Bold', 'LEFT')
        s.text('RECEIPT', w * 0.65, h * 0.265, h * 0.02, INK, 'Bold', 'CENTER')
        s.text('$2.50 / HR  ·  2 HR MAX  ·  8 AM - 7 PM MON - SAT', w / 2, h * 0.13, h * 0.019, INK, 'Bold', 'CENTER')
        s.text('Out of order? 555-0172', w / 2, h * 0.07, h * 0.022, 0x55585c, 'Bold', 'CENTER')
    at.put((0.0, 0.0, 0.5, 1.0), face, 400.0, 800.0)
    pr = print_material('nsk_mm_prints', at.save(), 1024, rough=0.35)
    out = []
    out.append(box(-0.15, 0.15, -0.15, 0.15, 0.0, 0.02, grey, 0.005, 1))
    out.append(box(-0.11, 0.11, -0.13, 0.13, 0.02, 0.85, body, 0.02, 2))
    head = parts.rbox('head', (0.0, 0.0, 1.2), (0.26, 0.34, 0.7), 0.04, 3)
    core.assign(head, body)
    out.append(head)
    # the solar cap tilted south (+Y here), its cells in the steel slot's finish
    cap = box(-0.16, 0.16, -0.2, 0.2, 1.55, 1.58, grey, 0.01, 2)
    cap.data.transform(Matrix.Translation((0, 0, 1.565)) @ Matrix.Rotation(math.radians(18), 4, 'X') @ Matrix.Translation((0, 0, -1.565)))
    out.append(cap)
    out.append(box(-0.14, 0.14, -0.18, 0.18, 1.585, 1.595, plain_dark('nsk_mm_cells'), 0.003, 1))
    out[-1].data.transform(Matrix.Translation((0, 0, 1.565)) @ Matrix.Rotation(math.radians(18), 4, 'X') @ Matrix.Translation((0, 0, -1.565)))
    # the screen, keypad, card slot, coin slot, receipt slot on the front (-X)
    out.append(print_quad('screen', [(-0.131, 0.1, 1.3), (-0.131, -0.1, 1.3), (-0.131, -0.1, 1.43), (-0.131, 0.1, 1.43)], (0.0, 0.0, 1.0, 1.0),
                          (0, -1, 0), (0, 0, 1), (-0.131, 0.1, 1.3), (0.2, 0.13), scr))
    out.append(face_quad('face', -0.1305, 0.0, 1.05, 0.2, 0.4, (-1, 0), (0.0, 0.0, 0.5, 1.0), pr))
    # the keypad (12 keys and the green/red pair), the card slot's bezel, the coin slot, the receipt slot, the lock
    for r_ in range(4):
        for c_ in range(3):
            yy, zz = 0.035 * (1 - c_), 1.235 - 0.026 * r_ - 0.03
            out.append(box(-0.138, -0.13, yy - 0.013, yy + 0.013, zz - 0.009, zz + 0.009, grey, 0.003, 2))
    for yy in (0.085, -0.085):
        out.append(box(-0.14, -0.13, yy - 0.025, yy + 0.025, 1.16, 1.19, grey, 0.005, 3))
    out.append(box(-0.15, -0.13, -0.05, 0.05, 1.255, 1.27, grey, 0.004, 3))
    out.append(box(-0.1505, -0.149, -0.035, 0.035, 1.26, 1.265, plain_dark('nsk_mm_cells')))
    out.append(box(-0.145, -0.13, 0.06, 0.1, 1.05, 1.11, grey, 0.006, 3))
    out.append(box(-0.1455, -0.144, 0.075, 0.085, 1.06, 1.1, plain_dark('nsk_mm_cells')))
    out.append(box(-0.14, -0.13, -0.08, 0.02, 0.92, 0.95, grey, 0.006, 3))
    out.append(lathe('lock', [(0.0, 0.0), (0.012, 0.0), (0.012, 0.006), (0.0, 0.008)], grey, 24,
                     Matrix.Translation((-0.11, 0.1, 0.7)) @ rot_y(-90)))
    # the solar cap's cells as a grid (thin bus bars across the dark panel), the post's anchor bolts
    M_cap = Matrix.Translation((0, 0, 1.565)) @ Matrix.Rotation(math.radians(18), 4, 'X') @ Matrix.Translation((0, 0, -1.565))
    for k in range(1, 6):
        yy = -0.18 + 0.36 * k / 6
        b_ = box(-0.14, 0.14, yy - 0.002, yy + 0.002, 1.595, 1.597, grey)
        b_.data.transform(M_cap)
        out.append(b_)
    for k in range(1, 4):
        xx = -0.14 + 0.28 * k / 4
        b_ = box(xx - 0.002, xx + 0.002, -0.18, 0.18, 1.595, 1.597, grey)
        b_.data.transform(M_cap)
        out.append(b_)
    for sx in (-1, 1):
        for sy in (-1, 1):
            out.append(lathe('anchor', [(0.0, 0.0), (0.011, 0.0), (0.011, 0.01), (0.0, 0.01)], grey, 6,
                             Matrix.Translation((sx * 0.12, sy * 0.12, 0.02))))
    return [finish_mesh(out, name, [body, grey, plain_dark('nsk_mm_cells'), pr, scr], 35.0, tag=TAG)]


_PLAIN = {}


def plain_dark(name):
    if name not in _PLAIN:
        tex(name, 256)
        m = core.Mat(name)
        tc, sep = object_coords(m)
        cells = ramp(m, noise(m, scaled(m, sep, 60.0, 60.0, 60.0), 1.0, 1.0), 0.45, 0.55)
        m.set('Base Color', m.mix(cells, H(0x10162a), H(0x1d2742)))
        m.set('Roughness', 0.15)
        m.set('Metallic', 0.3)
        _PLAIN[name] = m
    return _PLAIN[name]


def build_mailbox():
    name = 'SM_NS_Mailbox'
    blue = painted('nsk_mb_blue', 0x1f3f8a, 1024, rough=0.45, chips=0.6, rust=0.5, grime=0.8, grime_height=0.35, streaks=0.6)
    at = Atlas('nsk_mb_prints', 1024)

    def side(s, w, h):
        s.rect(0, 0, w, h, 0x1f3f8a)
        s.text('MAIL', w / 2, h * 0.42, h * 0.3, 0xf2f0ea, 'Black', 'CENTER', tracking=1.1)

    def plate(s, w, h):
        s.rect(0, 0, w, h, 0xe8e6df)
        s.text('LAST PICKUP', w / 2, h * 0.75, h * 0.12, INK, 'Black', 'CENTER')
        s.text('MON-FRI  5:00 PM', w / 2, h * 0.52, h * 0.1, INK, 'Bold', 'CENTER')
        s.text('SAT  12:00 PM', w / 2, h * 0.32, h * 0.1, INK, 'Bold', 'CENTER')
        s.text('No liquids · No hazardous items', w / 2, h * 0.1, h * 0.065, 0x55585c, 'Bold', 'CENTER')
    rects = [(0.0, 0.0, 0.48, 0.48), (0.5, 0.0, 0.98, 0.48), (0.0, 0.5, 0.48, 0.98)]
    at.put(rects[0], side, 500.0, 500.0)
    at.put(rects[1], side, 500.0, 500.0)
    at.put(rects[2], plate, 500.0, 500.0)
    pr = print_material('nsk_mb_prints', at.save(), 1024, rough=0.4)
    out = []
    W, D = 0.54, 0.5
    for sx in (-1, 1):
        for sy in (-1, 1):
            out.append(box(sx * 0.2 - 0.03, sx * 0.2 + 0.03, sy * 0.22 - 0.03, sy * 0.22 + 0.03, 0.0, 0.18, blue, 0.006, 2))
    body = box(-D / 2, D / 2, -W / 2, W / 2, 0.15, 1.05, blue, 0.02, 2)
    out.append(body)
    # the domed top
    top = prism_y('top', [(-D / 2 * math.cos(math.pi * k / 24), 1.05 + 0.17 * math.sin(math.pi * k / 24) ** 0.8) for k in range(25)], -W / 2, W / 2, blue)
    out.append(top)
    # the pull-down chute on the front (-X): a curved hood, its hinge, the handle; the access door below with its lock
    hood = []
    for k in range(13):
        a = math.radians(-10 + 100 * k / 12)
        hood.append((-D / 2 - 0.06 * math.cos(a), 0.83 + 0.15 * math.sin(a) * 0.95))
    hood_prof = hood + [(x_ + 0.012 * math.cos(math.radians(-10 + 100 * (12 - j) / 12)), z_ - 0.012 * math.sin(math.radians(-10 + 100 * (12 - j) / 12)))
                        for j, (x_, z_) in enumerate(reversed(hood))]
    out.append(prism_y('chute', hood_prof, -0.2, 0.2, blue))
    for sy in (-1, 1):
        out.append(box(-D / 2 - 0.06, -D / 2, sy * 0.2 - 0.006, sy * 0.2 + 0.006, 0.8, 0.99, blue, 0.004, 3))
    out.append(rod((-D / 2 - 0.004, -0.2, 0.83), (-D / 2 - 0.004, 0.2, 0.83), 0.01, blue, 24))
    out.append(tube([(-D / 2 - 0.06, -0.08, 0.97), (-D / 2 - 0.085, -0.07, 0.975), (-D / 2 - 0.09, 0.0, 0.975), (-D / 2 - 0.085, 0.07, 0.975),
                     (-D / 2 - 0.06, 0.08, 0.97)], 0.008, blue, 16, 6))
    out.append(box(-D / 2 - 0.008, -D / 2, -0.22, 0.22, 0.22, 0.74, blue, 0.006, 3))
    out.append(lathe('lock', [(0.0, 0.0), (0.016, 0.0), (0.016, 0.008), (0.012, 0.012), (0.0, 0.012)], blue, 24,
                     Matrix.Translation((-D / 2 - 0.008, 0.17, 0.66)) @ rot_y(-90)))
    for zz in (0.3, 0.66):
        out.append(rod((-D / 2 - 0.006, -0.226, zz - 0.04), (-D / 2 - 0.006, -0.226, zz + 0.04), 0.008, blue, 16))
    # the side reinforcing ribs and the legs' bolts into the sidewalk
    for sy in (-1, 1):
        for xx in (-0.12, 0.12):
            out.append(box(xx - 0.012, xx + 0.012, sy * W / 2 - 0.004, sy * W / 2 + 0.004, 0.17, 1.03, blue, 0.004, 3))
    for sx in (-1, 1):
        for sy in (-1, 1):
            out.append(box(sx * 0.2 - 0.05, sx * 0.2 + 0.05, sy * 0.22 - 0.05, sy * 0.22 + 0.05, 0.0, 0.012, blue, 0.004, 2))
            out.append(lathe('anchor', [(0.0, 0.0), (0.011, 0.0), (0.011, 0.01), (0.0, 0.01)], blue, 6,
                             Matrix.Translation((sx * 0.2 + sx * 0.035, sy * 0.22 + sy * 0.035, 0.012))))
    out.append(face_quad('mail1', 0.0, -W / 2 - 0.0005, 0.6, 0.4, 0.4, (0, -1), rects[0], pr))
    out.append(face_quad('mail2', 0.0, W / 2 + 0.0005, 0.6, 0.4, 0.4, (0, 1), rects[1], pr))
    out.append(face_quad('plate', -D / 2 - 0.0086, 0.0, 0.55, 0.2, 0.2, (-1, 0), rects[2], pr))
    return [finish_mesh(out, name, [blue, pr], 35.0, tag=TAG)]


def build_callbox():
    name = 'SM_NS_CallBox'
    red = painted('nsk_cb_red', 0xa3191c, 1024, rough=0.4, chips=0.8, rust=0.7, grime=0.8, grime_height=0.6, streaks=0.8)
    at = Atlas('nsk_cbx_prints', 512)

    def lab(s, w, h):
        s.rect(0, 0, w, h, 0xece8dc)
        s.text('FIRE', w / 2, h * 0.68, h * 0.2, 0xa3191c, 'Black', 'CENTER')
        s.text('ALARM', w / 2, h * 0.44, h * 0.18, 0xa3191c, 'Black', 'CENTER')
        s.text('PULL HOOK', w / 2, h * 0.18, h * 0.1, INK, 'Bold', 'CENTER')
    at.put((0.0, 0.0, 1.0, 1.0), lab, 300.0, 300.0)
    pr = print_material('nsk_cbx_prints', at.save(), 512, rough=0.35)
    out, lit = [], []
    # a fluted cast post on a base, the box on top, a lamp globe over it
    out.append(lathe('base', [(0.0, 0.0), (0.16, 0.0), (0.16, 0.06), (0.12, 0.12), (0.09, 0.3), (0.0, 0.3)], red, 32))
    post = core.lathe('post', [(0.0, 0.3), (0.07, 0.3), (0.065, 1.8), (0.0, 1.8)], segments=64)
    for v in post.data.vertices:
        r = v.co.xy.length
        if r > 0.03:
            k = 1.0 + 0.06 * math.cos(math.atan2(v.co.y, v.co.x) * 12)
            v.co.x *= k
            v.co.y *= k
    core.orient_normals(post)
    core.assign(post, red)
    out.append(post)
    out.append(box(-0.13, 0.13, -0.11, 0.11, 1.8, 2.2, red, 0.02, 3))
    out.append(prism_y('roof', [(-0.15, 2.2), (0.0, 2.3), (0.15, 2.2)], -0.13, 0.13, red))
    out.append(face_quad('label', -0.1366, 0.0, 2.02, 0.16, 0.16, (-1, 0), (0.0, 0.0, 1.0, 1.0), pr))
    out.append(box(-0.16, -0.13, -0.02, 0.02, 1.88, 1.93, red, 0.006, 1))
    out.append(lathe('lampbase', [(0.0, 2.3), (0.04, 2.3), (0.05, 2.33), (0.0, 2.33)], red, 24))
    lit.append(lathe('globe', [(0.0, 2.33), (0.055, 2.34), (0.065, 2.4), (0.055, 2.46), (0.0, 2.47)], None, 32))
    out.append(lathe('lampcap', [(0.0, 2.47), (0.06, 2.47), (0.04, 2.5), (0.0, 2.52)], red, 32))
    out.append(lathe('finial', [(0.0, 2.52), (0.012, 2.52), (0.016, 2.55), (0.0, 2.58)], red, 24))
    for k in range(6):
        a = 2 * math.pi * k / 6
        out.append(tube([(0.05 * math.cos(a), 0.05 * math.sin(a), 2.33), (0.074 * math.cos(a), 0.074 * math.sin(a), 2.4),
                         (0.05 * math.cos(a), 0.05 * math.sin(a), 2.47)], 0.003, red, 6, 4))
    out.append(tube([(0.074 * math.cos(2 * math.pi * k / 24), 0.074 * math.sin(2 * math.pi * k / 24), 2.4) for k in range(25)], 0.003, red, 6, 1))
    # the door: its frame, a hinge, rivets round the label, the pull hook, the cast mouldings under the roof
    out.append(box(-0.136, -0.13, -0.1, 0.1, 1.84, 2.16, red, 0.004, 3))
    out.append(rod((-0.134, 0.104, 1.86), (-0.134, 0.104, 2.14), 0.008, red, 16))
    for (yy, zz) in ((-0.085, 1.94), (0.085, 1.94), (-0.085, 2.1), (0.085, 2.1)):
        out.append(lathe('rivet', [(0.0, 0.0), (0.006, 0.0), (0.005, 0.003), (0.0, 0.004)], red, 12,
                         Matrix.Translation((-0.136, yy, zz)) @ rot_y(-90), fillet=0.0))
    out.append(tube([(-0.16, 0.0, 1.9), (-0.19, 0.0, 1.9), (-0.2, 0.0, 1.87), (-0.19, 0.0, 1.85)], 0.008, red, 12, 6))
    for zz, dz in ((2.2, 0.0), (1.8, -0.02)):
        out.append(box(-0.15, 0.15, -0.13, 0.13, zz - 0.015 + dz, zz + 0.015 + dz, red, 0.008, 3))
    return [finish_mesh(out, name, [red, pr], 35.0, tag=TAG), glow(lit, f'{name}_GlowWarm')]


def build_payphone():
    name = 'SM_NS_Payphone'
    st = street.bare_metal('nsk_pp_steel', 0x9a9ea2, rough=0.4, grime=0.6)
    tex('nsk_pp_steel', 1024)
    blk = painted('nsk_pp_black', 0x1d1f22, 1024, rough=0.45, chips=0.7, rust=0.6, grime=0.4, grime_height=0.0, streaks=0.8)
    at = Atlas('nsk_pp_prints', 1024, 512)
    at.put((0.0, 0.0, 0.5, 1.0), sticker_panel(77), 400.0, 800.0)

    def hdr(s, w, h):
        s.rect(0, 0, w, h, 0x1d1f22)
        s.text('PHONE', w / 2, h * 0.3, h * 0.45, 0xd8d6cf, 'Black', 'CENTER', tracking=1.2)
    at.put((0.52, 0.8, 1.0, 1.0), hdr, 490.0, 102.0)          # (the panel's aspect is its rect's: 491 x 102 px)
    pr = print_material('nsk_pp_prints', at.save(), 1024, rough=0.35, weather=1.0)
    out = []
    # origin: the wall plane's middle, Z at the shell's foot (0); the shell stands off the wall toward -X
    W, Hh = 0.6, 1.1
    out.append(box(-0.03, 0.0, -W / 2, W / 2, 0.0, Hh, blk, 0.01, 2))                # back plate
    out.append(box(-0.32, -0.03, -W / 2, -W / 2 + 0.02, 0.0, Hh, blk, 0.006, 1))     # side wings
    out.append(box(-0.32, -0.03, W / 2 - 0.02, W / 2, 0.0, Hh, blk, 0.006, 1))
    out.append(box(-0.33, -0.03, -W / 2, W / 2, Hh - 0.12, Hh, blk, 0.01, 2))        # hood
    out.append(face_quad('hdr', -0.3305, 0.0, Hh - 0.06, 0.5, 0.1, (-1, 0), (0.52, 0.8, 1.0, 1.0), pr))
    # the phone body, its keypad blanked, the cord's armored stub hanging, no handset
    out.append(box(-0.14, -0.03, -0.12, 0.12, 0.35, 0.85, st, 0.015, 2))
    out.append(box(-0.145, -0.14, -0.06, 0.06, 0.45, 0.6, st, 0.004, 1))
    # the armored cord's stub: steel rings, cut off ragged
    cord = [(-0.12, 0.13, 0.5), (-0.13, 0.16, 0.42), (-0.14, 0.15, 0.32), (-0.135, 0.13, 0.25)]
    out.append(tube(cord, 0.008, st, 12, 8))
    for k in range(8):
        t = k / 7
        p = Vector(cord[0]).lerp(Vector(cord[-1]), t) + Vector((-0.012 * math.sin(math.pi * t), 0.02 * math.sin(math.pi * t), 0.0))
        out.append(_G.lathe('ring', [(0.0075, -0.004), (0.011, -0.003), (0.011, 0.003), (0.0075, 0.004)], st, 10, Matrix.Translation(p)))
    # the coin slot and return, the hook lever (no handset on it), the keypad's sixteen blanks, the shell's mount bolts
    out.append(box(-0.148, -0.14, 0.04, 0.09, 0.76, 0.8, st, 0.004, 3))
    out.append(box(-0.1485, -0.147, 0.06, 0.07, 0.765, 0.795, blk))
    out.append(box(-0.15, -0.14, -0.06, 0.06, 0.38, 0.42, st, 0.008, 3))
    out.append(box(-0.17, -0.14, 0.095, 0.115, 0.64, 0.7, st, 0.004, 3))
    out.append(box(-0.2, -0.165, 0.098, 0.112, 0.69, 0.7, st, 0.003, 2))
    for r_ in range(4):
        for c_ in range(3):
            yy, zz = -0.028 + 0.028 * c_, 0.575 - 0.03 * r_
            out.append(box(-0.15, -0.144, yy - 0.011, yy + 0.011, zz - 0.011, zz + 0.011, st, 0.003, 2))
    for yy in (-0.24, 0.24):
        for zz in (0.12, Hh - 0.2):
            out.append(lathe('bolt', [(0.0, 0.0), (0.01, 0.0), (0.01, 0.006), (0.0, 0.007)], blk, 6,
                             Matrix.Translation((-0.03, yy, zz)) @ rot_y(-90)))
    out.append(face_quad('stick', -0.0305, -0.2, 0.55, 0.15, 0.3, (-1, 0), (0.0, 0.0, 0.5, 1.0), pr))
    return [finish_mesh(out, name, [st, blk, pr], 35.0, tag=TAG)]


def build_propane_cage():
    name = 'SM_NS_PropaneCage'
    cage = galv('nsk_pc_cage', 1024, grime=0.6, grime_height=0.3)
    tank = painted('nsk_pc_tank', 0xe9e8e3, 1024, rough=0.4, chips=0.6, rust=0.6, grime=0.6, grime_height=0.2, streaks=0.5)
    at = Atlas('nsk_pc_prints', 1024, 512)

    def sign(s, w, h):
        s.rect(0, 0, w, h, 0xf2f0ea)
        s.rect(0, h * 0.7, w, h * 0.3, 0xc8241c)
        s.text('PROPANE', w / 2, h * 0.78, h * 0.15, 0xffffff, 'Black', 'CENTER')
        s.text('EXCHANGE · PAY INSIDE', w / 2, h * 0.45, h * 0.1, INK, 'Black', 'CENTER')
        s.text('NO SMOKING', w / 2, h * 0.2, h * 0.1, 0xc8241c, 'Black', 'CENTER')
    at.put((0.0, 0.0, 1.0, 1.0), sign, 1000.0, 500.0)
    pr = print_material('nsk_pc_prints', at.save(), 1024, rough=0.35)
    out = []
    W, D, Hh = 1.2, 0.75, 1.2
    for sx in (-1, 1):
        for sy in (-1, 1):
            out.append(box(sx * D / 2 - 0.02, sx * D / 2 + 0.02, sy * W / 2 - 0.02, sy * W / 2 + 0.02, 0.0, Hh, cage, 0.004, 1))
    for z in (0.08, Hh / 2, Hh - 0.02):
        for sx in (-1, 1):
            out.append(box(sx * D / 2 - 0.015, sx * D / 2 + 0.015, -W / 2, W / 2, z - 0.015, z + 0.015, cage, 0.003, 1))
        for sy in (-1, 1):
            out.append(box(-D / 2, D / 2, sy * W / 2 - 0.015, sy * W / 2 + 0.015, z - 0.015, z + 0.015, cage, 0.003, 1))
    # expanded-metal mesh as a grid of thin wires on every face
    for k in range(1, 20):
        y = -W / 2 + W * k / 20
        for sx in (-1, 1):
            out.append(_G.rod((sx * D / 2, y, 0.08), (sx * D / 2, y, Hh - 0.02), 0.003, cage, 6))
    for k in range(1, 12):
        x = -D / 2 + D * k / 12
        for sy in (-1, 1):
            out.append(_G.rod((x, sy * W / 2, 0.08), (x, sy * W / 2, Hh - 0.02), 0.003, cage, 6))
    out.append(box(-D / 2, D / 2, -W / 2, W / 2, Hh - 0.01, Hh + 0.01, cage, 0.003, 1))
    out.append(box(-D / 2, D / 2, -W / 2, W / 2, 0.06, 0.08, cage, 0.003, 1))
    # tanks: 2 rows of 3 on the floor, 2 rows of 3 on the shelf
    for z0 in (0.08, Hh / 2 + 0.015):
        for row in (-0.17, 0.17):
            for k in range(3):
                y = -0.36 + 0.36 * k
                out.append(_G.lathe('tank', [(0.0, z0), (0.13, z0), (0.13, z0 + 0.05), (0.14, z0 + 0.055), (0.155, z0 + 0.08), (0.155, z0 + 0.36),
                                             (0.145, z0 + 0.4), (0.13, z0 + 0.42), (0.09, z0 + 0.44), (0.04, z0 + 0.45), (0.0, z0 + 0.45)], tank, 18,
                                    Matrix.Translation((row, y, 0))))
                out.append(_G.lathe('collar', [(0.0, z0 + 0.45), (0.06, z0 + 0.45), (0.06, z0 + 0.53), (0.0, z0 + 0.53)], cage, 12,
                                    Matrix.Translation((row, y, 0))))
                out.append(_G.lathe('valve', [(0.0, z0 + 0.45), (0.014, z0 + 0.45), (0.014, z0 + 0.5), (0.03, z0 + 0.5), (0.03, z0 + 0.51), (0.0, z0 + 0.51)],
                                    cage, 10, Matrix.Translation((row, y, 0))))
    # the door's hinges and hasp, its padlock and the sign on the front
    for zz in (0.25, Hh - 0.25):
        out.append(rod((-D / 2 - 0.022, -W / 2 + 0.02, zz - 0.04), (-D / 2 - 0.022, -W / 2 + 0.02, zz + 0.04), 0.012, cage, 20))
    out.append(box(-D / 2 - 0.03, -D / 2 - 0.015, 0.15, 0.25, 0.86, 0.94, cage, 0.003, 2))
    out.append(tube([(-D / 2 - 0.03, 0.185, 0.925), (-D / 2 - 0.03, 0.185, 0.945), (-D / 2 - 0.03, 0.2, 0.955), (-D / 2 - 0.03, 0.215, 0.945),
                     (-D / 2 - 0.03, 0.215, 0.925)], 0.004, cage, 8, 4))
    out.append(parts.rbox('lock', (-D / 2 - 0.03, 0.2, 0.9), (0.03, 0.05, 0.045), 0.006, 3))
    core.assign(out[-1], cage)
    out.append(face_quad('sign', -D / 2 - 0.006, -0.2, 0.95, 0.5, 0.25, (-1, 0), (0.0, 0.0, 1.0, 1.0), pr))
    out.append(box(-D / 2 - 0.005, -D / 2, -0.45, 0.05, 0.82, 1.08, cage, 0.002, 1))
    return [finish_mesh(out, name, [cage, tank, pr], 35.0, tag=TAG)]


def plywood(name, hexc, size=1024):
    tex(name, size)
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    grain = noise(m, scaled(m, sep, 2.0, 30.0, 2.0), 1.0, 4.0, distortion=2.0)
    blot = noise(m, o, 4.0, 4.0)
    col = m.mix(ramp(m, blot, 0.3, 0.7), H(hexc), H(street._shade(hexc, 0.82)))
    col = m.mix(m.math('MULTIPLY', ramp(m, grain, 0.45, 0.65), 0.15), col, H(street._shade(hexc, 0.7)))
    drip = m.math('MULTIPLY', ramp(m, noise(m, scaled(m, sep, 30.0, 30.0, 2.0), 1.0, 3.0), 0.6, 0.8), 0.4)
    col = m.mix(drip, col, H(0x3b3a33))
    m.set('Base Color', col)
    m.set('Roughness', 0.75)
    set_bump(m, grain, 0.2, 0.001)
    return m


def ibeam(name, a, b, depth, flange, mat, web=0.008, tf=0.012, up=(0.0, 0.0, 1.0)):
    """A wide-flange beam from a to b: depth along `up`, flanges `flange` wide."""
    A, B = Vector(a), Vector(b)
    d = (B - A).normalized()
    u = Vector(up) - d * Vector(up).dot(d)
    u.normalize()
    s_ = d.cross(u)
    hd, hf, hw = depth / 2, flange / 2, web / 2
    prof = [(-hf, -hd), (hf, -hd), (hf, -hd + tf), (hw, -hd + tf), (hw, hd - tf), (hf, hd - tf), (hf, hd), (-hf, hd), (-hf, hd - tf), (-hw, hd - tf),
            (-hw, -hd + tf), (-hf, -hd + tf)]
    rings = [[tuple(P + s_ * x + u * y) for x, y in prof] for P in (A, B)]
    o = core.loft(name, rings)
    core.orient_normals(o)
    core.assign(o, mat)
    return o


def build_sidewalk_shed():
    name = 'SM_NS_SidewalkShed'
    st = painted('nsk_shed_steel', 0x2f5a3a, 2048, rough=0.5, chips=0.8, rust=0.7, grime=0.8, grime_height=0.6, streaks=0.8)
    ply = plywood('nsk_shed_ply', 0x2f5a3a, 2048)
    deck = plywood('nsk_shed_deck', 0x9a7e58, 1024)
    at = Atlas('nsk_shed_prints', 1024)

    def permit(s, w, h):
        s.rect(0, 0, w, h, 0xf3f0e6)
        s.text('BUILDING PERMIT', w / 2, h * 0.88, h * 0.06, INK, 'Black', 'CENTER')
        s.text('CITY OF NORTHSIDE  ·  DEPT. OF BUILDINGS', w / 2, h * 0.81, h * 0.03, INK, 'Bold', 'CENTER')
        for k, ln in enumerate(['Permit No. 2026-0412-SH', 'Sidewalk shed  ·  Market St 200 blk', 'Owner: Cardinal Furniture Co.',
                                'Contractor: Ruiz & Daughters', 'Expires 03/31/2027', 'Complaints 555-0161']):
            s.text(ln, w * 0.08, h * (0.68 - 0.08 * k), h * 0.038, INK, 'Bold', 'LEFT')
        s.rect(w * 0.6, h * 0.08, w * 0.3, w * 0.3, 0x111111)
        s.rect(w * 0.63, h * 0.11, w * 0.24, w * 0.24, 0xf3f0e6)

    def poster(s, w, h):
        s.rect(0, 0, w, h, 0xd8b23a)
        s.text('FIGHT NIGHT', w / 2, h * 0.8, h * 0.1, 0x1a1a1a, 'Black', 'CENTER')
        s.text('IRONHOUSE', w / 2, h * 0.62, h * 0.08, 0xb3121b, 'Black', 'CENTER')
        s.text('SAT · 9 PM', w / 2, h * 0.45, h * 0.08, 0x1a1a1a, 'Black', 'CENTER')
        s.text('MARKET ST', w / 2, h * 0.3, h * 0.06, 0x1a1a1a, 'Bold', 'CENTER')
    at.put((0.0, 0.0, 0.48, 0.64), permit, 480.0, 640.0)
    at.put((0.5, 0.0, 0.98, 0.64), poster, 480.0, 640.0)
    pr = print_material('nsk_shed_prints', at.save(), 1024, rough=0.4, weather=0.9)
    out, lit = [], []
    L, Dd, Hh = 6.0, 2.6, 3.2
    zt = Hh - 0.3                      # the deck's underside
    xs = (0.15, Dd - 0.1)
    ys = (-L / 2 + 0.08, 0.0, L / 2 - 0.08)
    # posts: W6 sections on base plates bolted to timber pads; cross-bracing in the depth
    for y in ys:
        for x in xs:
            out.append(ibeam('post', (x, y, 0.0), (x, y, zt - 0.15), 0.15, 0.15, st, up=(1.0, 0.0, 0.0)))
            out.append(box(x - 0.13, x + 0.13, y - 0.13, y + 0.13, 0.0, 0.016, st, 0.003, 2))
            out.append(box(x - 0.16, x + 0.16, y - 0.16, y + 0.16, -0.05, 0.0, deck, 0.008, 3))
            for dx in (-0.095, 0.095):
                for dy in (-0.095, 0.095):
                    out.append(lathe('bolt', [(0.0, 0.0), (0.012, 0.0), (0.012, 0.01), (0.0, 0.011)], st, 6, Matrix.Translation((x + dx, y + dy, 0.016))))
        # the header beam across the depth and a knee brace each side
        out.append(ibeam('header', (0.05, y, zt - 0.075), (Dd + 0.02, y, zt - 0.075), 0.15, 0.1, st, up=(0.0, 0.0, 1.0)))
        for x, sgn in ((xs[0], 1), (xs[1], -1)):
            out.append(rod((x + sgn * 0.08, y, zt - 0.7), (x + sgn * 0.6, y, zt - 0.16), 0.024, st, 24))
    # runners along the length, timber joists across them, the plywood deck (4 x 8 sheets, gaps) and its curb boards
    for x in xs:
        out.append(ibeam('runner', (x, -L / 2, zt + 0.06), (x, L / 2, zt + 0.06), 0.12, 0.1, st, up=(0.0, 0.0, 1.0)))
    for k in range(13):
        y = -L / 2 + 0.1 + (L - 0.2) * k / 12
        out.append(box(0.02, Dd + 0.03, y - 0.04, y + 0.04, zt + 0.12, zt + 0.21, deck, 0.004, 2))
    for i in range(3):
        for j in range(2):
            y0 = -L / 2 + 2.0 * i + 0.004
            x0 = 0.0 + 1.3 * j + 0.004
            out.append(box(x0, x0 + 1.292, y0, y0 + 1.992, zt + 0.21, zt + 0.229, deck, 0.003, 2))
    for x in (0.0, Dd + 0.04):
        out.append(box(x - 0.025, x + 0.025, -L / 2, L / 2, zt + 0.229, zt + 0.33, deck, 0.004, 2))
    # the parapet on the street side (-X): 2 x 4 studs behind green plywood, a cap rail; the posted permit and a poster
    for k in range(9):
        y = -L / 2 + 0.05 + (L - 0.1) * k / 8
        out.append(box(0.0, 0.09, y - 0.02, y + 0.02, zt + 0.229, Hh + 0.6, deck, 0.004, 2))
    for i in range(3):
        y0 = -L / 2 + 2.0 * i + 0.003
        out.append(box(-0.02, 0.0, y0, y0 + 1.994, zt - 0.15, Hh + 0.6, ply, 0.003, 2))
    out.append(box(-0.04, 0.11, -L / 2, L / 2, Hh + 0.6, Hh + 0.64, deck, 0.006, 3))
    out.append(face_quad('permit', -0.0205, 1.2, Hh + 0.12, 0.42, 0.56, (-1, 0), (0.0, 0.0, 0.48, 0.64), pr))
    out.append(face_quad('poster', -0.0205, -1.0, Hh + 0.12, 0.42, 0.56, (-1, 0), (0.5, 0.0, 0.98, 0.64), pr))
    # caged bulkhead lights under the deck, a conduit run between them
    for y in (-1.5, 1.5):
        for x in (0.8, 1.9):
            out.append(lathe('bulk', [(0.0, 0.0), (0.09, 0.0), (0.09, -0.02), (0.075, -0.04), (0.0, -0.045)], st, 48,
                             Matrix.Translation((x, y, zt + 0.12))))
            for k in range(6):
                a = 2 * math.pi * k / 6
                out.append(tube([(x + 0.09 * math.cos(a), y + 0.09 * math.sin(a), zt + 0.1), (x + 0.075 * math.cos(a), y + 0.075 * math.sin(a), zt + 0.03),
                                 (x, y, zt + 0.02)], 0.003, st, 6, 4))
            out.append(tube([(x + 0.085 * math.cos(2 * math.pi * k / 24), y + 0.085 * math.sin(2 * math.pi * k / 24), zt + 0.07) for k in range(25)],
                            0.003, st, 6, 1))
            lit.append(lathe('lens', [(0.0, zt + 0.1), (0.07, zt + 0.1), (0.06, zt + 0.07), (0.0, zt + 0.065)], None, 32,
                             Matrix.Translation((x, y, 0.0))))     # under its bulkhead (without it all four sat at x 0, y 0)
        out.append(tube([(0.8, y, zt + 0.12 - 0.005), (1.35, y + 0.02, zt + 0.115), (1.9, y, zt + 0.12 - 0.005)], 0.012, st, 16, 4))
    out.append(tube([(0.8, -1.5, zt + 0.115), (0.81, 0.0, zt + 0.112), (0.8, 1.5, zt + 0.115)], 0.012, st, 16, 6))
    return [finish_mesh(out, name, [st, ply, deck, pr], 35.0, tag=TAG), glow(lit, f'{name}_GlowCool')]


def steam_flipbook():
    """T_NS_Steam: a 4 x 4 flipbook (1024 per cell, 4096) of a steam plume rising off the stack, A = density."""
    N, C = 2048, 4
    S_ = N // C
    out = np.zeros((N, N, 4), np.float32)
    yy, xx = np.mgrid[0:S_, 0:S_].astype(np.float32) / S_
    from ns_ground import tnoise
    base1 = tnoise(S_, 4, 901, 5)
    base2 = tnoise(S_, 8, 903, 4)
    for k in range(16):
        t = k / 16.0
        r, c = k // C, k % C
        h = 1.0 - yy                       # 0 at the bottom of the cell (the stack's mouth), up
        sh = int(t * S_)
        n1 = np.roll(base1, -sh, 0)
        n2 = np.roll(base2, -sh * 2, 0)
        width = 0.06 + 0.32 * h ** 0.8
        drift = 0.08 * h * h + 0.03 * np.sin(h * 7 + t * 6.283)
        d = np.abs(xx - 0.5 - drift) / width
        dens = np.clip(1.0 - d, 0, 1) ** 1.5 * np.clip(1.15 - h, 0, 1) * (0.55 + 0.6 * n1) * (0.7 + 0.5 * n2)
        dens = np.clip((dens - 0.12) * 1.6, 0, 1)
        out[r * S_:(r + 1) * S_, c * S_:(c + 1) * S_, :3] = 0.92
        out[r * S_:(r + 1) * S_, c * S_:(c + 1) * S_, 3] = dens
    path = os.path.join(core.ROOT, 'unreal', 'Art', 'Textures', 'NS', 'T_NS_Steam.png')
    _png(path, out)
    log('wrote T_NS_Steam (2048, 4 x 4 frames, A density)')
    # the first frame on its own: the default the glb carries (the card reads as steam before M_NS_Steam takes it over)
    frame0 = os.path.join(core.BUILD_DIR, 'textures', 'nsk_steam_frame0.png')
    _png(frame0, out[0:S_, 0:S_])
    return path, frame0


def build_steam_stack():
    name = 'SM_NS_SteamStack'
    tex('nsk_ss_stack', 1024)
    m = core.Mat('nsk_ss_stack')
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    z = sep.outputs['Z']
    band = ramp(m, m.math('ABSOLUTE', m.math('SUBTRACT', m.math('FRACT', m.math('DIVIDE', z, 0.5)), 0.5)), 0.245, 0.255)
    dirt = m.math('MULTIPLY', ramp(m, noise(m, o, 10.0, 4.0), 0.35, 0.8), m.math('ADD', 0.25, m.math('MULTIPLY', ramp(m, z, 0.8, 0.0), 0.6)))
    soot = m.math('MULTIPLY', ramp(m, z, 2.6, 3.0), 0.7)
    col = m.mix(band, H(0xe06a1a), H(0xeeece6))
    col = m.mix(m.math('MULTIPLY', dirt, 0.5), col, H(0x4a4038))
    col = m.mix(soot, col, H(0x2a2826))
    m.set('Base Color', col)
    m.set('Roughness', 0.55)
    out = []
    # a cone base over the manhole's vent and the stack, a rain cap
    # (fixed 64-sided rounds: the stack's budget is 3k)
    out.append(_G.lathe('base', [(0.0, 0.0), (0.45, 0.0), (0.45, 0.04), (0.44, 0.05), (0.2, 0.3), (0.0, 0.3)], m, 64))
    out.append(_G.lathe('stack', [(0.17, 0.25), (0.17, 3.0), (0.168, 3.01), (0.163, 3.017), (0.15, 3.02), (0.0, 3.02)], m, 64))
    for zz in (1.0, 2.0):
        out.append(_G.lathe('joint', [(0.168, zz - 0.03), (0.18, zz - 0.027), (0.182, zz - 0.02), (0.182, zz + 0.02), (0.18, zz + 0.027),
                                      (0.168, zz + 0.03)], m, 64))
    for k in range(4):
        a = 2 * math.pi * k / 4 + math.pi / 4
        c_, s_ = math.cos(a), math.sin(a)
        out.append(rod((0.42 * c_, 0.42 * s_, 0.04), (0.2 * c_, 0.2 * s_, 0.6), 0.012, m, 12))
        out.append(_G.lathe('weight', [(0.0, 0.0), (0.05, 0.0), (0.05, 0.035), (0.0, 0.04)], m, 12, Matrix.Translation((0.42 * c_, 0.42 * s_, 0.0))))
    # the steam card: a vertical quad standing in the stack's mouth (runtime M_NS_Steam, the T_NS_Steam flipbook), UV 0..1
    _, frame0 = steam_flipbook()
    sm = bpy.data.materials.get('ns_steam')
    if sm is None:
        smm = core.Mat('ns_steam')
        uvn = smm.node('ShaderNodeUVMap')
        uvn.uv_map = 'UVMap'
        img = smm.image(frame0, vector=uvn.outputs['UV'], extension='EXTEND')
        smm.link(img.outputs['Color'], smm.bsdf.inputs['Base Color'])
        smm.link(img.outputs['Alpha'], smm.bsdf.inputs['Alpha'])
        smm.set('Roughness', 1.0)
        sm = smm.m
        for attr, val in (('surface_render_method', 'BLENDED'), ('blend_method', 'BLEND')):
            try:
                setattr(sm, attr, val)
            except (AttributeError, TypeError):
                pass
        sm.use_backface_culling = False
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new('UVMap')
    for ang in (0.0, 90.0):
        a = math.radians(ang)
        c, s_ = math.cos(a), math.sin(a)
        pts = [(-1.0 * c, -1.0 * s_, 2.95), (1.0 * c, 1.0 * s_, 2.95), (1.0 * c, 1.0 * s_, 5.4), (-1.0 * c, -1.0 * s_, 5.4)]
        f = bm.faces.new([bm.verts.new(p) for p in pts])
        for lp, t in zip(f.loops, ((0, 0), (1, 0), (1, 1), (0, 1))):
            lp[uvl].uv = t
    card = core.mesh_object('steam', bm)
    card.data.materials.append(sm)
    set_attr(card, 'ns_uvlock', 1.0)
    out.append(card)
    return [finish_mesh(out, name, [m, sm], 35.0, tag=TAG)]


def bag_material(name):
    tex(name, 1024)
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    wr = noise(m, o, 30.0, 4.0, distortion=1.5)
    col = m.mix(ramp(m, wr, 0.4, 0.7), H(0x0d0d0e), H(0x1e1f22))
    wet = ramp(m, noise(m, o, 5.0, 3.0), 0.5, 0.8)
    m.set('Base Color', col)
    m.set('Roughness', mix_float(m, wet, m.math('ADD', 0.22, m.math('MULTIPLY', wr, 0.2)), 0.08))
    m.set('Specular IOR Level', 0.7)
    set_bump(m, wr, 0.6, 0.004)
    return m


def blob_bag(name, center, size, rng, mat, knot=True):
    """A stuffed trash bag: a deformed sphere (lumpy, sagging under its weight) with a knotted neck."""
    bm = bmesh.new()
    seg = getattr(blob_bag, 'seg', (40, 32))
    bmesh.ops.create_uvsphere(bm, u_segments=seg[0], v_segments=seg[1], radius=1.0)
    sx, sy, sz = size
    seeds = [Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(-1, 1))).normalized() for _ in range(8)]
    for v in bm.verts:
        p = v.co.copy()
        lump = sum(max(0.0, p.dot(s_)) ** 6 * rng.uniform(0.05, 0.15) for s_ in seeds[:5])
        wr = 0.025 * math.sin(p.x * 13 + p.y * 7) * math.sin(p.z * 11)
        r = 1.0 + lump + wr
        q = Vector((p.x * sx * r, p.y * sy * r, p.z * sz * r))
        if q.z < -sz * 0.55:
            q.z = -sz * 0.55 - (q.z + sz * 0.55) * 0.15
            q.x *= 1.06
            q.y *= 1.06
        v.co = q + Vector(center)
    o = core.mesh_object(name, bm)
    o.data.shade_smooth()
    core.assign(o, mat)
    out = [o]
    if knot:
        top = Vector(center) + Vector((0, 0, sz * 0.95))
        k = _G.lathe('knot', [(0.0, 0.0), (0.03, 0.0), (0.045, 0.04), (0.02, 0.07), (0.035, 0.1), (0.0, 0.11)], mat, 12, Matrix.Translation(top))
        out.append(k)
        for sgn in (-1, 1):
            out.append(_G.tube([top + Vector((0, 0, 0.08)), top + Vector((sgn * 0.06, 0.02, 0.14)), top + Vector((sgn * 0.1, 0.0, 0.1))], 0.01, mat, 6, 3))
    return out


def build_garbage(kind):
    name = f'SM_NS_GarbageBags_{kind}'
    rng = random.Random({'A': 11, 'B': 23, 'C': 37}[kind])
    bag = bag_material(f'nsk_gb{kind.lower()}_bag')
    out = []
    n = {'A': 5, 'B': 8, 'C': 3}[kind]
    blob_bag.seg = {'A': (38, 30), 'B': (30, 21), 'C': (48, 37)}[kind]
    spots = []
    for k in range(n):
        for _ in range(30):
            c = Vector((rng.uniform(-0.6, 0.6), rng.uniform(-0.9, 0.9), 0.0))
            if all((c - s_).length > 0.42 for s_ in spots):
                break
        spots.append(c)
        s_ = (rng.uniform(0.24, 0.32), rng.uniform(0.24, 0.32), rng.uniform(0.3, 0.38))
        c.z = s_[2] * 0.55
        out += blob_bag('bag', c, s_, rng, bag)
    # a couple stacked on top
    for k in range(n // 3):
        a, b = spots[k], spots[(k + 1) % n]
        c = (a + b) / 2 + Vector((0, 0, 0.5))
        out += blob_bag('bag', c, (0.25, 0.25, 0.28), rng, bag)
    return [finish_mesh(out, name, [bag], 60.0, tag=TAG)]


def cardboard_material(name):
    tex(name, 1024)
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    flute = noise(m, scaled(m, sep, 2.0, 2.0, 400.0), 1.0, 1.0)
    wet = m.math('MULTIPLY', ramp(m, noise(m, o, 4.0, 4.0), 0.45, 0.75), ramp(m, sep.outputs['Z'], 0.35, 0.0))
    col = m.mix(ramp(m, noise(m, o, 8.0, 3.0), 0.3, 0.7), H(0xa07a4a), H(0xb88f5c))
    col = m.mix(wet, col, H(0x5e4126))
    tape = ramp(m, m.math('ABSOLUTE', sep.outputs['Y']), 0.03, 0.025)
    col = m.mix(m.math('MULTIPLY', tape, 0.6), col, H(0xc9b48a))
    m.set('Base Color', col)
    m.set('Roughness', mix_float(m, wet, 0.85, 0.5))
    set_bump(m, flute, 0.15, 0.001)
    return m


def card_sheet(name, w, d, t, z, rng, mat, nu=18, nv=12):
    """A flattened box: a warped sheet (a crease across its middle, flaps' score lines, the wet end sagging), t thick."""
    bm = bmesh.new()
    crease = rng.uniform(-0.1, 0.1) * w
    sag = rng.uniform(0.0, 0.03)
    tw = rng.uniform(-0.02, 0.02)

    def hgt(x_, y_):
        u, v = (y_ + w / 2) / w, (x_ + d / 2) / d
        h = 0.006 * math.exp(-((y_ - crease) / 0.02) ** 2)                      # the fold's ridge
        h += 0.003 * (math.exp(-((x_ - d * 0.3) / 0.012) ** 2) + math.exp(-((x_ + d * 0.3) / 0.012) ** 2))   # score lines
        h += tw * (u - 0.5) * (v - 0.5) * 2.0 - sag * max(0.0, u - 0.7) ** 2 * 10
        return h
    top, bot = [], []
    for i in range(nu + 1):
        rt, rb = [], []
        for j in range(nv + 1):
            y_ = -w / 2 + w * i / nu
            x_ = -d / 2 + d * j / nv
            h = hgt(x_, y_)
            rt.append(bm.verts.new((x_, y_, z + t + h)))
            rb.append(bm.verts.new((x_, y_, z + h)))
        top.append(rt)
        bot.append(rb)
    for i in range(nu):
        for j in range(nv):
            bm.faces.new((top[i][j], top[i + 1][j], top[i + 1][j + 1], top[i][j + 1]))
            bm.faces.new((bot[i][j + 1], bot[i + 1][j + 1], bot[i + 1][j], bot[i][j]))
    for i in range(nu):
        bm.faces.new((bot[i][0], bot[i + 1][0], top[i + 1][0], top[i][0]))
        bm.faces.new((top[i][nv], top[i + 1][nv], bot[i + 1][nv], bot[i][nv]))
    for j in range(nv):
        bm.faces.new((top[0][j], top[0][j + 1], bot[0][j + 1], bot[0][j]))
        bm.faces.new((bot[nu][j], bot[nu][j + 1], top[nu][j + 1], top[nu][j]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    o = core.mesh_object(name, bm)
    core.assign(o, mat)
    return o


def build_cardboard():
    name = 'SM_NS_Cardboard'
    cb = cardboard_material('nsk_cbd_card')
    twine = plastic('nsk_cbd_twine', 0xd9c9a0, 256, rough=0.8)
    rng = random.Random(81)
    out = []
    z = 0.0
    for k in range(11):
        t = rng.uniform(0.006, 0.012)
        w, d = rng.uniform(0.7, 1.05), rng.uniform(0.5, 0.75)
        o = card_sheet('sheet', w, d, t, z, rng, cb)
        o.data.transform(Matrix.Translation((rng.uniform(-0.04, 0.04), rng.uniform(-0.05, 0.05), 0)) @ Matrix.Rotation(rng.uniform(-0.08, 0.08), 4, 'Z'))
        out.append(o)
        z += t + 0.012
    # two boxes leaned against the stack, folded but not flat
    for sgn in (-1, 1):
        o = card_sheet('lean', 0.8, 0.7, 0.008, 0.0, rng, cb, 18, 14)
        o.data.transform(Matrix.Translation((sgn * 0.42, 0.0, 0.0)) @ Matrix.Rotation(sgn * -0.25, 4, 'Y') @ Matrix.Rotation(math.radians(90), 4, 'Y')
                         @ Matrix.Translation((-0.35, 0.0, 0.0)))
        out.append(o)
    for y in (-0.25, 0.25):
        out.append(tube([(-0.38, y, 0.0), (-0.38, y, z + 0.012), (-0.2, y, z + 0.02), (0.2, y, z + 0.02), (0.38, y, z + 0.012), (0.38, y, 0.0)],
                        0.003, twine, 6, 6))
        out.append(lathe('knot', [(0.0, 0.0), (0.008, 0.0), (0.01, 0.006), (0.0, 0.012)], twine, 12, Matrix.Translation((0.1, y, z + 0.02))))
    return [finish_mesh(out, name, [cb, twine], 40.0, tag=TAG)]


def chalk_default():
    s = Sheet(500, 800)
    s.rect(0, 0, 500, 800, 0x1e2422)
    s.text('TODAY', 250, 680, 70, 0xf0eee4, 'Bold', 'CENTER')
    s.text('Coffee + Pie', 250, 560, 46, 0xf0eee4, 'Bold', 'CENTER')
    s.text('$5', 250, 470, 80, 0xf6d26a, 'Black', 'CENTER')
    s.text('Come on in', 250, 300, 40, 0xf0eee4, 'Bold', 'CENTER')
    return s.render('nsk_sb_chalk', 512, samples=4)[0]


def build_sandwich_board():
    name = 'SM_NS_SandwichBoard'
    wood = plywood('nsk_sb_frame', 0x3a2a1d, 1024)
    scr = screen_material('ns_screen_chalk', chalk_default())
    out = []
    W, Hh, ang = 0.6, 1.0, math.radians(14)
    for sx in (-1, 1):
        # a frame leaning in to the top hinge, its board facing out (sx -1: the front, -X; its foot swings out to -X)
        M = Matrix.Translation((0, 0, Hh * math.cos(ang))) @ Matrix.Rotation(-sx * ang, 4, 'Y') @ Matrix.Translation((0, 0, -Hh))
        parts_ = [box(sx * 0.0 - 0.012, sx * 0.0 + 0.012, -W / 2, W / 2, 0.0, Hh, wood, 0.004, 1)]
        for (y0, y1, z0, z1) in ((-W / 2, -W / 2 + 0.04, 0.0, Hh), (W / 2 - 0.04, W / 2, 0.0, Hh), (-W / 2, W / 2, Hh - 0.04, Hh), (-W / 2, W / 2, 0.08, 0.12)):
            parts_.append(box(sx * 0.012, sx * 0.03, y0, y1, z0, z1, wood, 0.004, 1))
        q = print_quad('chalk', [(sx * 0.0125, -sx * (W / 2 - 0.04), 0.12), (sx * 0.0125, sx * (W / 2 - 0.04), 0.12),
                                 (sx * 0.0125, sx * (W / 2 - 0.04), Hh - 0.04), (sx * 0.0125, -sx * (W / 2 - 0.04), Hh - 0.04)],
                       (0.0, 0.0, 1.0, 1.0), (0, sx, 0), (0, 0, 1), (sx * 0.0125, -sx * (W / 2 - 0.04), 0.12), (W - 0.08, Hh - 0.16), scr)
        core.orient_normals(q, (sx, 0, 0))
        parts_.append(q)
        for p in parts_:
            p.data.transform(M)
        out += parts_
    for sx in (-1, 1):
        o = rod((sx * 0.02, -W / 2 + 0.05, Hh * math.cos(ang) - 0.02), (sx * 0.02, W / 2 - 0.05, Hh * math.cos(ang) - 0.02), 0.006, wood, 8)
        out.append(o)
    return [finish_mesh(out, name, [wood, scr], 35.0, tag=TAG)]


def build_bench():
    name = 'SM_NS_Bench'
    st = black_powder('nsk_bn_steel', 1024, 0.3)
    out = []
    L = 1.8
    for y in (-L / 2 + 0.15, L / 2 - 0.15):
        # cast end frames: legs, seat support, armrest
        out.append(prism_y('leg', [(-0.3, 0.0), (-0.24, 0.0), (-0.05, 0.42), (0.18, 0.0), (0.24, 0.0), (0.02, 0.46), (-0.05, 0.46)], y - 0.02, y + 0.02, st))
        out.append(prism_y('back', [(0.18, 0.44), (0.22, 0.44), (0.3, 0.85), (0.26, 0.86)], y - 0.02, y + 0.02, st))
        out.append(tube([(-0.25, y, 0.45), (-0.25, y, 0.62), (0.0, y, 0.64), (0.18, y, 0.62)], 0.015, st, 10, 6))
    for k in range(5):
        x = -0.24 + 0.09 * k
        out.append(box(x - 0.035, x + 0.035, -L / 2, L / 2, 0.44, 0.46, st, 0.004, 1))
    for k in range(3):
        t = k / 2
        x = 0.21 + 0.07 * t
        z = 0.55 + 0.25 * t
        o = box(-0.035, 0.035, -L / 2, L / 2, -0.01, 0.01, st, 0.004, 1)
        o.data.transform(Matrix.Translation((x, 0, z)) @ Matrix.Rotation(math.radians(-75), 4, 'Y'))
        out.append(o)
    # an anti-sleep divider in the middle
    out.append(tube([(-0.2, 0.0, 0.46), (-0.2, 0.0, 0.62), (0.15, 0.0, 0.62), (0.15, 0.0, 0.46)], 0.012, st, 8, 4))
    for y in (-L / 2 + 0.15, L / 2 - 0.15):
        for k in range(5):
            x = -0.24 + 0.09 * k
            out.append(lathe('bolt', [(0.0, 0.0), (0.007, 0.0), (0.0065, 0.003), (0.0, 0.0035)], st, 12,
                             Matrix.Translation((x, y, 0.46)), fillet=0.0))
        for xx in (-0.27, 0.21):
            out.append(box(xx - 0.05, xx + 0.05, y - 0.04, y + 0.04, 0.0, 0.012, st, 0.004, 3))
            out.append(lathe('anchor', [(0.0, 0.0), (0.01, 0.0), (0.01, 0.008), (0.0, 0.009)], st, 6, Matrix.Translation((xx, y, 0.012))))
    return [finish_mesh(out, name, [st], 35.0, tag=TAG)]


def build_tree_guard():
    name = 'SM_NS_TreeGuard'
    st = black_powder('nsk_tg_steel', 512, 0.2)
    out = []
    # a low hoop fence round a 1.2 m pit: 4 sides of hoops between corner posts (0.45 m high)
    h = 0.45
    for sx, sy in ((-1, -1), (1, -1), (1, 1), (-1, 1)):
        out.append(rod((sx * 0.62, sy * 0.62, -0.1), (sx * 0.62, sy * 0.62, h + 0.02), 0.016, st, 10))
        out.append(lathe('ball', [(0.0, h + 0.02), (0.02, h + 0.03), (0.022, h + 0.05), (0.0, h + 0.07)], st, 12, Matrix.Translation((sx * 0.62, sy * 0.62, 0))))
    for side in range(4):
        a = [(-0.62, -0.62), (0.62, -0.62), (0.62, 0.62), (-0.62, 0.62)][side]
        b = [(0.62, -0.62), (0.62, 0.62), (-0.62, 0.62), (-0.62, -0.62)][side]
        out.append(rod((a[0], a[1], h), (b[0], b[1], h), 0.01, st, 8))
        for k in range(4):
            t0, t1 = k / 4, (k + 1) / 4
            p0 = (a[0] + (b[0] - a[0]) * t0, a[1] + (b[1] - a[1]) * t0)
            p1 = (a[0] + (b[0] - a[0]) * t1, a[1] + (b[1] - a[1]) * t1)
            pts = []
            for q in range(7):
                t = q / 6
                pts.append((p0[0] + (p1[0] - p0[0]) * t, p0[1] + (p1[1] - p0[1]) * t, 0.05 + (h - 0.12) * math.sin(math.pi * t)))
            out.append(_G.tube(pts, 0.006, st, 6, 2))
    return [finish_mesh(out, name, [st], 35.0, tag=TAG)]


# ---- litter

def build_litter():
    """SM_NS_Litter_Cup / _Can / _Bottle / _Butts / _Flyer / _Wrapper / _Leaves / _Cap: small things, one baked slot each."""
    out = []
    rng = random.Random(1717)
    # a crushed coffee cup on its side
    cm = plastic('nsk_lt_cup', 0xe9e3d6, 256, rough=0.7)
    cup = _G.lathe('cup', [(0.0, 0.0), (0.028, 0.0), (0.04, 0.12), (0.043, 0.122), (0.0, 0.122)], cm, 24)
    for v in cup.data.vertices:
        v.co.x *= 1.0 - 0.35 * max(0.0, v.co.z - 0.05) * 8.0 * (1 if v.co.x > 0 else 0.4)
    lid = _G.lathe('lid', [(0.0, 0.122), (0.044, 0.122), (0.046, 0.128), (0.04, 0.132), (0.03, 0.136), (0.0, 0.138)], cm, 24)
    for v in lid.data.vertices:
        v.co.x *= 1.0 - 0.35 * max(0.0, v.co.z - 0.05) * 8.0 * (1 if v.co.x > 0 else 0.4)
    cup = core.join([cup, lid], 'cup')
    cup.data.transform(Matrix.Translation((0, 0, 0.035)) @ Matrix.Rotation(math.radians(90), 4, 'X'))
    out.append(finish_mesh([cup], 'SM_NS_Litter_Cup', [cm], 40.0, tag=TAG))
    # a crushed can
    cn = street.bare_metal('nsk_lt_can', 0xb52b2b, rough=0.35, grime=0.4)
    tex('nsk_lt_can', 256)
    can = lathe('can', [(0.0, 0.0), (0.026, 0.0), (0.033, 0.01), (0.033, 0.11), (0.027, 0.122), (0.0, 0.122)], cn, 20)
    for v in can.data.vertices:
        v.co.z *= 0.45 + 0.25 * math.sin(v.co.x * 90 + v.co.y * 40)
        v.co.x *= 1.0 + 0.2 * math.sin(v.co.z * 80)
    can.data.transform(Matrix.Rotation(math.radians(90), 4, 'X') @ Matrix.Translation((0, 0, -0.03)))
    can.data.transform(Matrix.Translation((0, 0, 0.033)))
    out.append(finish_mesh([can], 'SM_NS_Litter_Can', [cn], 40.0, tag=TAG))
    # a brown glass bottle on its side
    bt = core.Mat('nsk_lt_bottle')
    tex('nsk_lt_bottle', 256)
    bt.set('Base Color', H(0x3a2008))
    bt.set('Roughness', 0.1)
    bt.set('Specular IOR Level', 0.7)
    bot = lathe('bottle', [(0.0, 0.0), (0.03, 0.0), (0.031, 0.14), (0.016, 0.19), (0.013, 0.23), (0.0, 0.232)], bt, 20)
    bot.data.transform(Matrix.Translation((0, 0, 0.031)) @ Matrix.Rotation(math.radians(90), 4, 'X'))
    out.append(finish_mesh([bot], 'SM_NS_Litter_Bottle', [bt], 40.0, tag=TAG))
    # cigarette butts: a little cluster
    bu = plastic('nsk_lt_butts', 0xd9c49a, 256, rough=0.8)
    bs = []
    for k in range(9):
        a = rng.uniform(0, 2 * math.pi)
        r = rng.uniform(0.0, 0.09)
        c = Vector((r * math.cos(a), r * math.sin(a), 0.004))
        d = Vector((math.cos(rng.uniform(0, 6.3)), math.sin(rng.uniform(0, 6.3)), 0.0))
        o = rod(c - d * 0.012, c + d * 0.012, 0.004, bu, 8)
        bs.append(o)
    out.append(finish_mesh(bs, 'SM_NS_Litter_Butts', [bu], 40.0, tag=TAG))
    # a soggy flyer and a candy wrapper: crumpled sheets
    for nm, hexc, size in (('Flyer', 0xe8e2c8, (0.2, 0.28)), ('Wrapper', 0xc8302a, (0.08, 0.05))):
        mt = plastic(f'nsk_lt_{nm.lower()}', hexc, 256, rough=0.5)
        bm = bmesh.new()
        nx, ny = 8, 10
        vs = [[bm.verts.new(((i / nx - 0.5) * size[0], (j / ny - 0.5) * size[1], 0.002 + rng.uniform(0, 0.01 if nm == 'Wrapper' else 0.004))) for i in range(nx + 1)]
              for j in range(ny + 1)]
        for j in range(ny):
            for i in range(nx):
                bm.faces.new((vs[j][i], vs[j][i + 1], vs[j + 1][i + 1], vs[j + 1][i]))
        o = core.mesh_object(nm, bm)
        mod = o.modifiers.new('sol', 'SOLIDIFY')
        mod.thickness = 0.0008
        core.select_only([o])
        bpy.ops.object.modifier_apply(modifier=mod.name)
        core.assign(o, mt)
        out.append(finish_mesh([o], f'SM_NS_Litter_{nm}', [mt], 40.0, tag=TAG))
    # a drift of wet leaves (plane leaves, browning)
    # (plane leaves: broader than long, three main lobes and two small basal ones with wide sinuses, a wedge base and a stalk,
    # cupped and folded on the midrib; brown, ochre and a few still yellow-green, darker where wet)
    tex('nsk_lt_leaves', 512)
    lf = core.Mat('nsk_lt_leaves')
    ltc, lsep = object_coords(lf)
    lo_ = ltc.outputs['Object']
    hue = noise(lf, lo_, 9.0, 2.0)
    lcol = lf.mix(ramp(lf, hue, 0.35, 0.55), H(0x6b4a22), H(0x9a6c26))
    lcol = lf.mix(ramp(lf, hue, 0.62, 0.72), lcol, H(0x8d8a3a))
    lcol = lf.mix(ramp(lf, noise(lf, lo_, 40.0, 3.0), 0.55, 0.8), lcol, H(0x3e2a16))
    lf.set('Base Color', lcol)
    lf.set('Roughness', lf.math('ADD', 0.35, lf.math('MULTIPLY', ramp(lf, noise(lf, lo_, 5.0, 2.0), 0.3, 0.7), 0.35)))
    lf.set('Specular IOR Level', 0.6)
    lobes = [(math.pi / 2, 1.0, 0.42), (math.pi / 2 - 0.85, 0.86, 0.36), (math.pi / 2 + 0.85, 0.86, 0.36), (-0.35, 0.5, 0.3), (math.pi + 0.35, 0.5, 0.3)]
    ls = []
    for k in range(18):
        a = rng.uniform(0, 2 * math.pi)
        r = rng.uniform(0.0, 0.25)
        bm = bmesh.new()
        s = rng.uniform(0.07, 0.11)
        ring = []
        for t in np.linspace(0, 2 * math.pi, 21)[:-1]:
            g = max(w * math.exp(-((math.atan2(math.sin(t - th), math.cos(t - th))) / wd) ** 2) for th, w, wd in lobes)
            rr = s * 0.5 * (0.32 + 0.68 * g) * (1.0 + 0.04 * math.sin(14 * t))
            ring.append((rr * math.cos(t), rr * math.sin(t)))
        c0 = bm.verts.new((0.0, -0.08 * s, 0.0))
        mid = [bm.verts.new((0.5 * x, 0.5 * y - 0.04 * s, 0.0)) for x, y in ring]
        out_ = [bm.verts.new((x, y, 0.0)) for x, y in ring]
        n_ = len(ring)
        for i in range(n_):
            j = (i + 1) % n_
            bm.faces.new((c0, mid[i], mid[j]))
            bm.faces.new((mid[i], out_[i], out_[j], mid[j]))
        # the stalk
        st0 = bm.verts.new((-0.002, -0.1 * s, 0.0))
        st1 = bm.verts.new((0.002, -0.1 * s, 0.0))
        st2 = bm.verts.new((0.0025, -0.1 * s - 0.035, 0.0))
        st3 = bm.verts.new((-0.0015, -0.1 * s - 0.035, 0.0))
        bm.faces.new((st0, st1, st2, st3))
        cup, fold, curl = rng.uniform(0.08, 0.25), rng.uniform(0.05, 0.2), rng.uniform(-0.3, 0.3)
        for v in bm.verts:
            x, y = v.co.x, v.co.y
            d = math.hypot(x, y) / (0.5 * s)
            v.co.z = s * (cup * d * d * 0.5 + fold * abs(x) / (0.5 * s) * 0.4 + curl * (y / (0.5 * s)) ** 2 * 0.3)
        o = core.mesh_object('leaf', bm)
        o.data.transform(Matrix.Translation((r * math.cos(a), r * math.sin(a), 0.002 + 0.003 * k / 26)) @ Matrix.Rotation(rng.uniform(0, 6.3), 4, 'Z')
                         @ Matrix.Rotation(rng.uniform(-0.2, 0.2), 4, 'X'))
        ls.append(o)
    leaves = core.join(ls, 'leaves')
    core.assign(leaves, lf)
    out.append(finish_mesh([leaves], 'SM_NS_Litter_Leaves', [lf], 40.0, tag=TAG))
    # a bottle cap
    cp = street.bare_metal('nsk_lt_cap', 0x8a8e92, rough=0.4, grime=0.5)
    tex('nsk_lt_cap', 128)
    cap = lathe('cap', [(0.0, 0.0), (0.0145, 0.0), (0.0145, 0.006), (0.0, 0.006)], cp, 21)
    for v in cap.data.vertices:
        r_ = v.co.xy.length
        if r_ > 0.014:
            a = math.atan2(v.co.y, v.co.x)
            k = 1.0 + 0.06 * (1 if int(round(a / (2 * math.pi) * 21)) % 2 == 0 else 0)
            v.co.x *= k
            v.co.y *= k
    out.append(finish_mesh([cap], 'SM_NS_Litter_Cap', [cp], 40.0, tag=TAG))
    return out


# ================================================================== SM_NS_LockedBike (A) / _B (P1)

BIKE_X = 0.0          # the bike's plane (its origin is on the ground under its middle)


def _merge(o):
    bm = bmesh.new()
    bm.from_mesh(o.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=1e-6)
    bm.to_mesh(o.data)
    bm.free()
    return o


def bike_wheel(c, mats, front):
    """A 700c wheel in the YZ plane (axle along X) centered at c: tire, rim, hub, 32 spokes laced 3-cross, the valve."""
    frame, rubber, metal = mats
    out = []
    M = Matrix.Translation(Vector(c)) @ rot_y(90)       # lathe Z -> the axle (X)
    tire = [(0.322 + 0.0175 * math.cos(2 * math.pi * k / 16), 0.0175 * math.sin(2 * math.pi * k / 16) * 1.05) for k in range(17)]
    out.append(_merge(_G.lathe('tire', tire, rubber, 64, M)))
    rim = [(0.292, -0.0095), (0.305, -0.0095), (0.307, -0.008), (0.307, 0.008), (0.305, 0.0095), (0.292, 0.0095), (0.288, 0.004), (0.288, -0.004),
           (0.292, -0.0095)]
    out.append(_merge(_G.lathe('rim', rim, metal, 64, M)))
    hub = [(0.0, -0.05), (0.006, -0.05), (0.006, -0.042), (0.02, -0.04), (0.03, -0.034), (0.03, -0.03), (0.014, -0.028), (0.016, 0.0),
           (0.014, 0.028), (0.03, 0.03), (0.03, 0.034), (0.02, 0.04), (0.006, 0.042), (0.006, 0.05), (0.0, 0.05)]
    out.append(_G.lathe('hub', hub, metal, 20, M))
    C = Vector(c)
    # 3-cross: 16 spokes a side from the flange holes, leading and trailing in turn, each meeting the rim 67.5 deg on
    for side in (-1, 1):
        for j in range(16):
            a_h = 2 * math.pi * j / 16 + (math.pi / 16 if side > 0 else 0.0)
            a_r = a_h + (1 if j % 2 == 0 else -1) * math.radians(67.5)
            p0 = C + Vector((side * 0.032, 0.026 * math.cos(a_h), 0.026 * math.sin(a_h)))
            p1 = C + Vector((side * 0.003, 0.289 * math.cos(a_r), 0.289 * math.sin(a_r)))
            out.append(_G.rod(tuple(p0), tuple(p1), 0.001, metal, 4))
    out.append(_G.rod(tuple(C + Vector((0.0, 0.0, -0.288))), tuple(C + Vector((0.0, 0.0, -0.27))), 0.003, metal, 6))
    return out


def build_locked_bike(variant='A'):
    name = 'SM_NS_LockedBike' if variant == 'A' else 'SM_NS_LockedBike_B'
    tg = 'nsk_bk' if variant == 'A' else 'nsk_bkb'
    frame = painted(f'{tg}_frame', 0x2f5d62, 1024, rough=0.4, chips=0.7, rust=0.5, grime=0.6, grime_height=0.5, streaks=0.3)
    rubber = plastic(f'{tg}_rubber', 0x1c1c1e, 1024, rough=0.8, grime=0.5)
    tex(f'{tg}_metal', 1024)
    metal = street.bare_metal(f'{tg}_metal', 0x9a9ea3, rough=0.35, grime=0.5)
    lockm = plastic(f'{tg}_lock', 0xd8b020, 512, rough=0.5, grime=0.4)
    mats = (frame, rubber, metal)
    out = []
    X = BIKE_X

    def P(y, z, x=0.0):
        return (X + x, y, z)
    R, F, BB = (0.52, 0.34), (-0.53, 0.34), (0.10, 0.27)
    sa = math.radians(73.0)
    S = (BB[0] + 0.5 * math.cos(sa), BB[1] + 0.5 * math.sin(sa))
    ha = math.radians(71.0)
    H2 = (-0.43, 0.64)
    H1 = (H2[0] + 0.15 * math.cos(ha), H2[1] + 0.15 * math.sin(ha))
    # ---- the frame: tubes, lugs at the joints, the head tube's cups
    tubes = [((S[0] - 0.012, S[1] - 0.02), (H1[0] + 0.004, H1[1] - 0.015), 0.016),      # top tube
             (BB, (H2[0] + 0.006, H2[1] + 0.02), 0.019),                                 # down tube
             (BB, S, 0.015)]                                                              # seat tube
    for a, b, r in tubes:
        out.append(rod(P(*a), P(*b), r, frame, 24))
    for sx in (-1, 1):
        out.append(tube([P(BB[0] + 0.02, BB[1], sx * 0.025), P(0.3, 0.3, sx * 0.05), P(R[0] - 0.01, R[1], sx * 0.064)], 0.0095, frame, 14, 6))
        out.append(tube([P(S[0] - 0.008, S[1] - 0.03, sx * 0.018), P(0.4, 0.55, sx * 0.05), P(R[0] - 0.005, R[1] + 0.01, sx * 0.064)], 0.008, frame,
                        14, 6))
        out.append(box(X + sx * 0.064 - 0.003, X + sx * 0.064 + 0.003, R[0] - 0.02, R[0] + 0.015, R[1] - 0.025, R[1] + 0.02, frame, 0.002, 2))
    out.append(lathe('headtube', [(0.0, 0.0), (0.02, 0.0), (0.021, 0.008), (0.019, 0.02), (0.019, 0.13), (0.021, 0.142), (0.02, 0.15), (0.0, 0.15)],
                     frame, 32, Matrix.Translation(P(*H2)) @ Matrix.Rotation(-(math.pi / 2 - ha), 4, 'X')))
    out.append(lathe('bbshell', [(0.0, -0.035), (0.021, -0.035), (0.021, 0.035), (0.0, 0.035)], frame, 24, Matrix.Translation(P(*BB)) @ rot_y(90)))
    # ---- the fork: crown, two curved blades to the dropouts, the steerer and a riser stem, the flat bar, grips, levers
    crown = (H2[0] - 0.012, H2[1] - 0.03)
    out.append(box(X - 0.045, X + 0.045, crown[0] - 0.02, crown[0] + 0.02, crown[1] - 0.015, crown[1] + 0.012, frame, 0.006, 3))
    for sx in (-1, 1):
        out.append(tube([P(crown[0], crown[1] - 0.01, sx * 0.04), P(-0.49, 0.48, sx * 0.05), P(F[0] + 0.01, F[1] + 0.03, sx * 0.05),
                         P(F[0], F[1], sx * 0.05)], 0.011, frame, 14, 6))
        out.append(box(X + sx * 0.05 - 0.003, X + sx * 0.05 + 0.003, F[0] - 0.012, F[0] + 0.012, F[1] - 0.02, F[1] + 0.012, frame, 0.002, 2))
    top = (H1[0] + 0.035 * math.cos(ha), H1[1] + 0.035 * math.sin(ha))
    out.append(rod(P(*H1), P(*top), 0.0145, metal, 20))
    for k in range(3):
        c_ = (H1[0] + (0.002 + 0.009 * k) * math.cos(ha), H1[1] + (0.002 + 0.009 * k) * math.sin(ha))
        out.append(lathe('spacer', [(0.0145, 0.0), (0.017, 0.0), (0.017, 0.008), (0.0145, 0.008)], metal, 20,
                         Matrix.Translation(P(*c_)) @ Matrix.Rotation(-(math.pi / 2 - ha), 4, 'X')))
    bar = (top[0] - 0.08, top[1] + 0.07)
    out.append(rod(P(top[0], top[1] - 0.01), P(*bar), 0.016, metal, 20))
    out.append(box(X - 0.025, X + 0.025, bar[0] - 0.02, bar[0] + 0.02, bar[1] - 0.02, bar[1] + 0.02, metal, 0.006, 3))
    barp = [P(bar[0] + 0.05, bar[1] + 0.01, -0.3), P(bar[0] + 0.01, bar[1] + 0.02, -0.16), P(bar[0], bar[1], 0.0), P(bar[0] + 0.01, bar[1] + 0.02, 0.16),
            P(bar[0] + 0.05, bar[1] + 0.01, 0.3)]
    out.append(tube(barp, 0.011, metal, 16, 8))
    for sx in (-1, 1):
        g0 = Vector(P(bar[0] + 0.044, bar[1] + 0.012, sx * 0.25))
        g1 = Vector(P(bar[0] + 0.052, bar[1] + 0.01, sx * 0.31))
        d = (g1 - g0).normalized()
        q = d.to_track_quat('Z', 'Y').to_matrix().to_4x4()
        out.append(lathe('grip', [(0.0, 0.0), (0.016, 0.0), (0.017, 0.01), (0.016, 0.05), (0.017, 0.12), (0.0175, 0.125), (0.0, 0.128)], rubber, 20,
                         Matrix.Translation(g0) @ q))
        # brake lever: a clamp and a curved blade forward under the bar
        lv = Vector(P(bar[0] + 0.03, bar[1] + 0.015, sx * 0.2))
        out.append(box(lv.x - 0.015, lv.x + 0.015, lv.y - 0.018, lv.y + 0.018, lv.z - 0.018, lv.z + 0.018, metal, 0.005, 3))
        out.append(tube([tuple(lv + Vector((0.0, -0.02, -0.01))), tuple(lv + Vector((sx * 0.04, -0.05, -0.02))), tuple(lv + Vector((sx * 0.1, -0.04, -0.025)))],
                        0.005, metal, 10, 6))
        # the cable's housing looping down to the frame
        out.append(tube([tuple(lv + Vector((0.0, 0.02, 0.0))), tuple(lv + Vector((-sx * 0.06, 0.12, -0.05))), P(-0.3, 0.72, sx * 0.02),
                         P(-0.1, 0.71, sx * 0.018)], 0.0025, rubber, 6, 8))
    # ---- seat post and saddle (a lofted shell: narrow nose, broad tail)
    sp_top = (S[0] + 0.14 * math.cos(sa), S[1] + 0.14 * math.sin(sa))
    out.append(rod(P(S[0] - 0.01 * math.cos(sa), S[1] - 0.01 * math.sin(sa)), P(*sp_top), 0.0135, metal, 20))
    out.append(lathe('clamp', [(0.0145, -0.012), (0.019, -0.012), (0.019, 0.012), (0.0145, 0.012)], metal, 20,
                     Matrix.Translation(P(*S)) @ Matrix.Rotation(-(math.pi / 2 - sa), 4, 'X')))
    rings = []
    for k in range(10):
        t = k / 9
        yy = sp_top[0] - 0.13 + 0.26 * t
        u_ = min(1.0, max(0.0, (t - 0.35) / 0.6))
        hw = 0.022 + 0.055 * (u_ * u_ * (3 - 2 * u_)) ** 0.9
        zz = sp_top[1] + 0.035 + 0.012 * math.sin(math.pi * t) - 0.01 * (1 - t) ** 3
        ring = []
        for j in range(16):
            a = 2 * math.pi * j / 16
            ca, sa_ = math.cos(a), math.sin(a)
            ring.append(P(yy, zz + (0.022 if sa_ > 0 else 0.01) * sa_, hw * math.copysign(abs(ca) ** 0.7, ca)))
        rings.append(ring)
    sad = core.loft('saddle', rings)
    core.orient_normals(sad)
    core.assign(sad, rubber)
    out.append(sad)
    for sx in (-1, 1):
        out.append(tube([P(sp_top[0] - 0.08, sp_top[1] + 0.03, sx * 0.012), P(sp_top[0], sp_top[1] + 0.005, sx * 0.02), P(sp_top[0] + 0.09, sp_top[1] + 0.03, sx * 0.025)],
                        0.0035, metal, 8, 4))
    # ---- drivetrain on the -X side: chainring, cranks, pedals, the cassette, the derailleur, the chain
    cx = -0.055
    out.append(_G.lathe('ring', [(0.07, -0.0025), (0.1, -0.0025), (0.104, -0.0012), (0.104, 0.0012), (0.1, 0.0025), (0.07, 0.0025), (0.07, -0.0025)],
                        metal, 64, Matrix.Translation(P(BB[0], BB[1], cx)) @ rot_y(90)))
    for k in range(5):
        a = 2 * math.pi * k / 5
        out.append(rod(P(BB[0], BB[1], cx), P(BB[0] + 0.075 * math.cos(a), BB[1] + 0.075 * math.sin(a), cx), 0.006, metal, 8))
    ca = math.radians(-35.0)
    for sx, ang in ((-1, ca), (1, ca + math.pi)):
        cend = (BB[0] + 0.17 * math.cos(ang), BB[1] + 0.17 * math.sin(ang))
        out.append(rod(P(BB[0], BB[1], sx * 0.065), P(cend[0], cend[1], sx * 0.07), 0.011, metal, 12))
        out.append(_G.rod(P(cend[0], cend[1], sx * 0.07), P(cend[0], cend[1], sx * 0.16), 0.005, metal, 8))
        pz = X + sx * 0.115
        out.append(box(pz - 0.045, pz + 0.045, cend[0] - 0.035, cend[0] + 0.035, cend[1] - 0.012, cend[1] + 0.012, rubber, 0.006, 3))
    out.append(lathe('spindle', [(0.0, -0.065), (0.012, -0.065), (0.012, 0.065), (0.0, 0.065)], metal, 16, Matrix.Translation(P(*BB)) @ rot_y(90)))
    cog = []
    for k in range(8):
        r_ = 0.028 + 0.005 * k
        x_ = -0.012 - 0.0035 * k
        cog += [(r_, x_ - 0.001), (r_, x_ + 0.001)]
    out.append(_G.lathe('cassette', [(0.0, -0.045)] + [(0.025, -0.045)] + sorted(cog, key=lambda p: p[1]) + [(0.02, 0.0), (0.0, 0.0)], metal, 40,
                        Matrix.Translation(P(R[0], R[1], 0.0)) @ rot_y(90)))
    der = (R[0] + 0.01, R[1] - 0.09)
    out.append(box(X + cx - 0.012, X + cx + 0.012, der[0] - 0.035, der[0] + 0.02, der[1] - 0.01, der[1] + 0.07, metal, 0.006, 3))
    for zz in (der[1] - 0.005, der[1] + 0.05):
        out.append(_G.lathe('pulley', [(0.0, -0.004), (0.016, -0.004), (0.016, 0.004), (0.0, 0.004)], metal, 16,
                            Matrix.Translation(P(der[0] - 0.012, zz, cx - 0.004)) @ rot_y(90)))
    chain = [P(BB[0] + 0.102 * math.cos(math.pi / 2 + math.pi * k / 8), BB[1] + 0.102 * math.sin(math.pi / 2 + math.pi * k / 8), cx) for k in range(-1, 9)]
    chain += [P(der[0] - 0.012, der[1] - 0.02, cx), P(der[0] - 0.012 + 0.016, der[1] - 0.005, cx), P(der[0] - 0.012, der[1] + 0.03, cx),
              P(R[0] + 0.035, R[1] - 0.03, cx), P(R[0] + 0.045, R[1] + 0.01, cx), P(R[0] + 0.02, R[1] + 0.045, cx), P(R[0] - 0.01, R[1] + 0.048, cx)]
    chain += [P(BB[0] + 0.03, BB[1] + 0.1, cx), P(BB[0] - 0.005, BB[1] + 0.102, cx)]
    out.append(_G.tube(chain, 0.004, metal, 6, 6))
    # ---- fenders (curved strips with stays), a rear rack, a kickstand folded up, a bell and a front light
    for (cy, cz), a0, a1 in ((R, 15.0, 160.0), (F, 20.0, 165.0)):
        rings = []
        for k in range(25):
            a = math.radians(a0 + (a1 - a0) * k / 24)
            if (cy, cz) == F:
                a = math.pi - a
            ring = []
            for j in range(5):
                t = (j / 4 - 0.5) * 2
                rr = 0.36 + 0.006 * (1 - t * t)
                ring.append(P(cy + rr * math.cos(a), cz + rr * math.sin(a), 0.026 * t))
            rings.append(ring)
        bm = bmesh.new()
        vs = [[bm.verts.new(p) for p in ring] for ring in rings]
        for i in range(len(vs) - 1):
            for j in range(4):
                bm.faces.new((vs[i][j], vs[i + 1][j], vs[i + 1][j + 1], vs[i][j + 1]))
        fen = core.mesh_object('fender', bm)
        mod = fen.modifiers.new('sol', 'SOLIDIFY')
        mod.thickness = 0.002
        core.select_only([fen])
        bpy.ops.object.modifier_apply(modifier=mod.name)
        core.orient_normals(fen)
        core.assign(fen, frame)
        out.append(fen)
        for sx in (-1, 1):
            aa = math.radians(a0 + 25.0) if (cy, cz) == R else math.pi - math.radians(a0 + 25.0)
            out.append(_G.rod(P(cy, cz, sx * 0.06), P(cy + 0.355 * math.cos(aa), cz + 0.355 * math.sin(aa), sx * 0.024), 0.0022, metal, 6))
    for sx in (-1, 1):
        out.append(tube([P(R[0], R[1] + 0.01, sx * 0.066), P(R[0] + 0.05, 0.62, sx * 0.07), P(R[0] + 0.06, 0.75, sx * 0.07)], 0.005, metal, 10, 4))
        out.append(rod(P(S[0] + 0.02, S[1] - 0.02, sx * 0.02), P(R[0] - 0.05, 0.75, sx * 0.07), 0.004, metal, 8))
    for k in range(4):
        yy = R[0] - 0.12 + 0.07 * k
        out.append(rod(P(yy, 0.75, -0.075), P(yy, 0.75, 0.075), 0.004, metal, 8))
    out.append(rod(P(R[0] - 0.12, 0.75, -0.07), P(R[0] + 0.1, 0.75, -0.07), 0.004, metal, 8))
    out.append(rod(P(R[0] - 0.12, 0.75, 0.07), P(R[0] + 0.1, 0.75, 0.07), 0.004, metal, 8))
    out.append(rod(P(BB[0] + 0.06, BB[1] - 0.005, 0.03), P(0.36, 0.29, 0.06), 0.006, metal, 10))
    out.append(lathe('bell', [(0.0, 0.0), (0.026, 0.0), (0.024, 0.012), (0.012, 0.02), (0.0, 0.022)], metal, 24,
                     Matrix.Translation(P(bar[0] + 0.005, bar[1] + 0.018, -0.12))))
    lamp = (bar[0] - 0.03, bar[1] - 0.01)
    out.append(lathe('light', [(0.0, 0.0), (0.02, 0.0), (0.022, 0.035), (0.026, 0.05), (0.0, 0.052)], rubber, 24,
                     Matrix.Translation(P(lamp[0], lamp[1], 0.05)) @ Matrix.Rotation(math.radians(90), 4, 'X')))
    # ---- the wheels (variant B: the front one stolen, the bike pitched forward onto its fork tips)
    out += bike_wheel(P(*R), mats, False)
    if variant == 'A':
        out += bike_wheel(P(*F), mats, True)
    else:
        piv = Vector(P(R[0] + 0.0, 0.0))
        rot = Matrix.Translation(piv) @ Matrix.Rotation(math.radians(16.9), 4, 'X') @ Matrix.Translation(-piv)
        for o in out:
            o.data.transform(rot)
    # ---- the U-lock: the shackle round the rack's +Y leg (at x +0.06 from the bike's plane) and the drive-side chainstay
    zl = 0.31 if variant == 'A' else 0.30
    rx, ry = X + 0.06, 0.375
    shackle = [(rx + 0.05, ry - 0.05, zl), (rx - 0.02, ry - 0.05, zl), (rx - 0.12, ry - 0.05, zl)]
    for k in range(9):
        a = -math.pi / 2 - math.pi * k / 8
        shackle.append((rx - 0.12 + 0.05 * math.cos(a), ry + 0.05 * math.sin(a), zl))
    shackle += [(rx - 0.12, ry + 0.05, zl), (rx - 0.02, ry + 0.05, zl), (rx + 0.05, ry + 0.05, zl)]
    out.append(_G.tube(shackle, 0.0085, metal, 14, 3))
    out.append(lathe('crossbar', [(0.0, -0.085), (0.017, -0.085), (0.019, -0.075), (0.019, 0.075), (0.017, 0.085), (0.0, 0.085)], lockm, 24,
                     Matrix.Translation((rx + 0.06, ry, zl)) @ Matrix.Rotation(math.radians(90), 4, 'X')))
    out.append(lathe('keycyl', [(0.0, 0.0), (0.012, 0.0), (0.012, 0.012), (0.0, 0.014)], metal, 16,
                     Matrix.Translation((rx + 0.06, ry + 0.085, zl)) @ Matrix.Rotation(math.radians(-90), 4, 'X')))
    return [finish_mesh(out, name, [frame, rubber, metal, lockm], 40.0, tag=TAG)]


# ================================================================== build
BUILDERS = {
    'led': build_streetlight_led, 'mast': lambda: build_signal_mast('A'), 'mast_b': lambda: build_signal_mast('B'), 'pedestal': build_signal_pedestal,
    'ped': build_ped_signal, 'blades_a': lambda: build_corner_blades('A'), 'blades_b': lambda: build_corner_blades('B'),
    'blades_c': lambda: build_corner_blades('C'), 'blades_d': lambda: build_corner_blades('D'), 'noparking': lambda: build_signpost('NoParking'),
    'nostanding': lambda: build_signpost('NoStanding'), 'stop': lambda: build_signpost('Stop'), 'busflag': lambda: build_signpost('BusFlag'),
    'bikerack': build_bike_rack, 'ice': build_ice_chest, 'cellar': build_cellar_doors, 'awning_s': lambda: build_awning('S'),
    'awning_m': lambda: build_awning('M'), 'awning_l': lambda: build_awning('L'), 'barricade': build_barricade, 'cone': build_cone,
    'plate': build_road_plate, 'jersey': build_jersey, 'closure': build_sidewalk_closure, 'shelter': build_bus_shelter,
    'tree_a': lambda: build_tree('A'), 'tree_b': lambda: build_tree('B'),
    'cabinet': build_signal_cabinet, 'meter': build_muni_meter, 'mailbox': build_mailbox, 'callbox': build_callbox, 'payphone': build_payphone,
    'propane': build_propane_cage, 'shed': build_sidewalk_shed, 'steam': build_steam_stack, 'bags_a': lambda: build_garbage('A'),
    'bags_b': lambda: build_garbage('B'), 'bags_c': lambda: build_garbage('C'), 'cardboard': build_cardboard, 'sandwich': build_sandwich_board,
    'bench': build_bench, 'treeguard': build_tree_guard, 'litter': build_litter,
    'lockbike': lambda: build_locked_bike('A'), 'lockbike_b': lambda: build_locked_bike('B'),
}

MESHES = ['SM_NS_StreetlightLED', 'SM_NS_StreetlightLED_GlowCool', 'SM_NS_SignalMast', 'SM_NS_SignalMast_GlowSignal', 'SM_NS_SignalMast_B',
          'SM_NS_SignalMast_B_GlowSignal', 'SM_NS_SignalPedestal', 'SM_NS_SignalPedestal_GlowSignal', 'SM_NS_PedSignal', 'SM_NS_PedSignal_GlowSignal',
          'SM_NS_CornerBlades_A', 'SM_NS_CornerBlades_B', 'SM_NS_CornerBlades_C', 'SM_NS_CornerBlades_D', 'SM_NS_SignPost_NoParking',
          'SM_NS_SignPost_NoStanding', 'SM_NS_SignPost_Stop', 'SM_NS_SignPost_BusFlag', 'SM_NS_BikeRack', 'SM_NS_IceChest', 'SM_NS_CellarDoors',
          'SM_NS_Awning_S', 'SM_NS_Awning_M', 'SM_NS_Awning_L', 'SM_NS_Barricade', 'SM_NS_Cone', 'SM_NS_RoadPlate', 'SM_NS_JerseyBarrier',
          'SM_NS_SidewalkClosure', 'SM_NS_BusShelter', 'SM_NS_BusShelter_GlowCool', 'SM_NS_Tree_A', 'SM_NS_Tree_B',
          'SM_NS_SignalCabinet', 'SM_NS_MuniMeter', 'SM_NS_Mailbox', 'SM_NS_CallBox', 'SM_NS_CallBox_GlowWarm', 'SM_NS_Payphone', 'SM_NS_PropaneCage',
          'SM_NS_SidewalkShed', 'SM_NS_SidewalkShed_GlowCool', 'SM_NS_SteamStack', 'SM_NS_GarbageBags_A', 'SM_NS_GarbageBags_B', 'SM_NS_GarbageBags_C',
          'SM_NS_Cardboard', 'SM_NS_SandwichBoard', 'SM_NS_Bench', 'SM_NS_TreeGuard', 'SM_NS_Litter_Cup', 'SM_NS_Litter_Can', 'SM_NS_Litter_Bottle',
          'SM_NS_Litter_Butts', 'SM_NS_Litter_Flyer', 'SM_NS_Litter_Wrapper', 'SM_NS_Litter_Leaves', 'SM_NS_Litter_Cap',
          'SM_NS_LockedBike', 'SM_NS_LockedBike_B']


def build():
    core.reset()
    random.seed(1931)
    objs = []
    for key, fn in BUILDERS.items():
        if _ONLY and key not in _ONLY:
            continue
        t0 = time.time()
        try:
            made = fn()
        except Exception:
            traceback.print_exc()
            log(f'FAILED {key}')
            continue
        objs += made
        log(f'built {key} ({", ".join(o.name for o in made)}) in {time.time() - t0:.1f}s')
    for o in objs:
        o.data.name = o.name
    return objs


# ---- baking: unbaked slots split off and joined back

_SPLIT = {}
_ORDER = {}


def _is_unbaked(mat):
    return mat is not None and mat.name.startswith(UNBAKED_PREFIX)


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
    if len(fi):
        me.polygons.foreach_set('material_index', np.array([remap[i] for i in fi], dtype=np.int32))


def bake_parts(objs):
    _SPLIT.clear()
    _ORDER.clear()
    bake = []
    for o in objs:
        if '_Glow' in o.name:
            o.hide_render = True
            continue
        _ORDER[o.name] = [s.material.name for s in o.material_slots]
        idx = [i for i, s in enumerate(o.material_slots) if _is_unbaked(s.material)]
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
    return bake


def _export_glb_colors(objs, name):
    """core.export_glb, but COLOR_0 goes out from the active color attribute (the trees' wind weights on the baked bark
    as well as the leaves): the exporter's default only writes colors a material reads."""
    os.makedirs(core.EXPORT_DIR, exist_ok=True)
    path = os.path.join(core.EXPORT_DIR, f'{name}.glb')
    core.select_only(list(objs))
    kw = dict(filepath=path, export_format='GLB', use_selection=True, export_apply=True, export_yup=True, export_texcoords=True,
              export_normals=True, export_materials='EXPORT', export_image_format='AUTO', export_skins=False, export_animations=False)
    has_col = any(o.type == 'MESH' and len(o.data.color_attributes) for o in objs)
    if has_col:
        try:
            bpy.ops.export_scene.gltf(**kw, export_vertex_color='ACTIVE')
            return path
        except TypeError:
            pass
    bpy.ops.export_scene.gltf(**kw)
    return path


def assemble(objs):
    """Joins each mesh's unbaked part back, puts the slots back in their order and names the baked ones M_<Mesh>_<i>."""
    core.export_glb = _export_glb_colors
    out = []
    by_all = {o.name: o for o in bpy.data.objects}
    for o in objs:
        name = o.name
        dup = _SPLIT.get(name)
        if dup is not None:
            dup.hide_render = False
            o = core.join([o, dup], name)
            o.name = name
            o.data.name = name
        order = _ORDER.get(name)
        if order:
            mats = [s.material for s in o.material_slots]
            # baked slots come back as M_<name>_<k> in the order they were baked; unbaked keep their names
            baked = [m for m in mats if not _is_unbaked(m)]
            want = []
            bi = 0
            for nm in order:
                if nm.startswith(UNBAKED_PREFIX):
                    want.append(next(m for m in mats if m.name == nm or m.name.startswith(nm + '.')))
                else:
                    want.append(baked[bi])
                    bi += 1
            from ns_ground import order_slots
            order_slots(o, want, TAG)
            for i, m in enumerate(o.material_slots):
                if not _is_unbaked(m.material):
                    m.material.name = f'M_{name}_{i}'
            log(f'{name} slots: {[s.material.name for s in o.material_slots]}')
        out.append(o)
    # the glows (hidden from the bake) go out too
    for nm, ob in by_all.items():
        if '_Glow' in nm and ob.name in bpy.data.objects and ob not in out and not nm.endswith('__unbaked'):
            ob.hide_render = False
            out.append(ob)
    order = {n: i for i, n in enumerate(MESHES)}
    out.sort(key=lambda o: order.get(o.name, 999))
    return out


# ================================================================== review

def _which_reviews():
    v = os.environ.get('NS_KIT_REVIEW', 'props,signs')
    return {s.strip() for s in v.split(',') if s.strip()}


# (label, meshes shown, camera (Blender, in the first mesh's frame), target, fov)
PROP_SHOTS = [
    ('led', ['SM_NS_StreetlightLED', 'SM_NS_StreetlightLED_GlowCool'], (-7.0, -6.0, 3.0), (-0.9, 0.0, 4.6), 55),
    ('led_head', ['SM_NS_StreetlightLED', 'SM_NS_StreetlightLED_GlowCool'], (-3.4, -1.2, 8.3), (-2.1, 0.0, 8.95), 45),
    ('mast', ['SM_NS_SignalMast', 'SM_NS_SignalMast_GlowSignal'], (-3.0, -11.0, 2.5), (-3.2, 0.0, 4.6), 60),
    ('mast_heads', ['SM_NS_SignalMast', 'SM_NS_SignalMast_GlowSignal'], (-6.2, 3.2, 4.2), (-6.0, 0.0, 5.4), 45),
    ('pedestal', ['SM_NS_SignalPedestal', 'SM_NS_SignalPedestal_GlowSignal'], (-3.6, -1.6, 2.6), (0.0, 0.0, 2.6), 50),
    ('ped', ['SM_NS_PedSignal', 'SM_NS_PedSignal_GlowSignal'], (-2.6, -1.0, 1.8), (0.0, 0.0, 1.8), 50),
    ('blades', ['SM_NS_CornerBlades_A'], (-2.2, -2.6, 2.8), (0.0, 0.0, 3.2), 45),
    ('signs', ['SM_NS_SignPost_NoParking'], (-2.0, -0.8, 2.3), (0.0, 0.0, 2.4), 40),
    ('shelter', ['SM_NS_BusShelter', 'SM_NS_BusShelter_GlowCool'], (-5.5, -4.0, 2.2), (0.0, 0.0, 1.2), 50),
    ('shelter_in', ['SM_NS_BusShelter', 'SM_NS_BusShelter_GlowCool'], (-1.6, 1.6, 1.5), (0.6, -1.0, 1.3), 70),
    ('awning', ['SM_NS_Awning_M'], (-5.0, -3.5, -2.2), (-0.8, 0.0, -0.6), 50),
    ('awning_under', ['SM_NS_Awning_L'], (-2.2, -2.5, -2.4), (-0.6, 1.0, -0.4), 70),
    ('ice', ['SM_NS_IceChest'], (-2.6, -1.5, 1.3), (0.0, 0.0, 0.6), 45),
    ('cellar', ['SM_NS_CellarDoors'], (-1.6, -1.2, 1.2), (0.0, 0.0, 0.0), 45),
    ('bike', ['SM_NS_BikeRack'], (-1.6, -1.4, 1.0), (0.0, 0.0, 0.45), 45),
    ('lockbike', ['SM_NS_BikeRack', 'SM_NS_LockedBike'], (-2.6, -1.6, 1.3), (0.0, 0.0, 0.5), 45),
    ('lockbike_b', ['SM_NS_BikeRack', 'SM_NS_LockedBike_B'], (-2.2, 1.9, 1.2), (0.0, 0.0, 0.4), 45),
    ('work', ['SM_NS_Barricade', 'SM_NS_Cone', 'SM_NS_RoadPlate', 'SM_NS_JerseyBarrier', 'SM_NS_SidewalkClosure'], (-6.0, -5.0, 2.8), (0.0, 1.5, 0.5), 50),
    ('tree_a', ['SM_NS_Tree_A'], (-13.0, -7.0, 3.0), (0.0, 0.0, 4.6), 50),
    ('tree_a_close', ['SM_NS_Tree_A'], (-3.2, -1.6, 2.4), (0.0, 0.0, 3.6), 60),
    ('tree_b', ['SM_NS_Tree_B'], (-12.0, -7.0, 3.0), (0.0, 0.0, 4.2), 50),
    ('tree_b_close', ['SM_NS_Tree_B'], (-3.0, 1.8, 2.2), (0.0, 0.0, 3.4), 60),
    ('cabinet', ['SM_NS_SignalCabinet'], (-2.4, -1.6, 1.3), (0.0, 0.0, 0.8), 45),
    ('meter', ['SM_NS_MuniMeter'], (-1.6, -0.9, 1.4), (0.0, 0.0, 1.0), 45),
    ('mailbox', ['SM_NS_Mailbox'], (-1.8, -1.2, 1.2), (0.0, 0.0, 0.6), 45),
    ('callbox', ['SM_NS_CallBox', 'SM_NS_CallBox_GlowWarm'], (-1.8, -1.0, 1.8), (0.0, 0.0, 1.5), 45),
    ('payphone', ['SM_NS_Payphone'], (-1.6, -0.9, 1.0), (0.0, 0.0, 0.6), 45),
    ('propane', ['SM_NS_PropaneCage'], (-2.4, -1.6, 1.4), (0.0, 0.0, 0.6), 45),
    ('shed', ['SM_NS_SidewalkShed', 'SM_NS_SidewalkShed_GlowCool'], (-5.0, -6.0, 2.0), (1.0, 0.0, 2.2), 55),
    ('steam', ['SM_NS_SteamStack'], (-4.0, -2.5, 1.8), (0.0, 0.0, 2.2), 45),
    ('bags', ['SM_NS_GarbageBags_A'], (-2.4, -1.4, 1.2), (0.0, 0.0, 0.3), 45),
    ('bags_b', ['SM_NS_GarbageBags_B'], (-2.6, -1.6, 1.4), (0.0, 0.0, 0.3), 45),
    ('cardboard', ['SM_NS_Cardboard'], (-1.8, -1.2, 1.0), (0.0, 0.0, 0.2), 45),
    ('sandwich', ['SM_NS_SandwichBoard'], (-1.8, -0.8, 1.0), (0.0, 0.0, 0.5), 45),
    ('bench', ['SM_NS_Bench'], (-2.2, -1.6, 1.1), (0.0, 0.0, 0.4), 45),
    ('treeguard', ['SM_NS_TreeGuard'], (-1.8, -1.4, 1.2), (0.0, 0.0, 0.2), 45),
    ('litter', ['SM_NS_Litter_Cup', 'SM_NS_Litter_Can', 'SM_NS_Litter_Bottle', 'SM_NS_Litter_Butts', 'SM_NS_Litter_Flyer', 'SM_NS_Litter_Wrapper',
                'SM_NS_Litter_Leaves', 'SM_NS_Litter_Cap'], (-0.9, -0.7, 0.55), (0.0, 0.05, 0.0), 45),
]
WORK_LAYOUT = {'SM_NS_LockedBike': (-0.06, 0.0, 0.0), 'SM_NS_LockedBike_B': (-0.06, 0.0, 0.0), 'SM_NS_Barricade': (0.0, 0.0, 0.0), 'SM_NS_Cone': (-0.8, -1.4, 0.0), 'SM_NS_RoadPlate': (1.5, 3.2, 0.025),
               'SM_NS_JerseyBarrier': (1.8, -0.5, 0.0), 'SM_NS_SidewalkClosure': (0.6, 6.0, 0.0),
               'SM_NS_Litter_Cup': (0.0, -0.3, 0.0), 'SM_NS_Litter_Can': (0.15, -0.1, 0.0), 'SM_NS_Litter_Bottle': (-0.1, 0.15, 0.0),
               'SM_NS_Litter_Butts': (0.2, 0.25, 0.0), 'SM_NS_Litter_Flyer': (0.35, 0.0, 0.0), 'SM_NS_Litter_Wrapper': (-0.2, -0.1, 0.0),
               'SM_NS_Litter_Leaves': (0.1, 0.5, 0.0), 'SM_NS_Litter_Cap': (-0.05, -0.05, 0.0)}

SIGN_SHOTS = [
    ('blade_a1', 'SM_NS_CornerBlades_A', (0.0, -1.6, 3.5), (0.0, 0.0, 3.5), 40),
    ('blade_a2', 'SM_NS_CornerBlades_A', (-1.6, 0.0, 3.32), (0.0, 0.0, 3.32), 40),
    ('blade_d1', 'SM_NS_CornerBlades_D', (0.0, 1.6, 3.5), (0.0, 0.0, 3.5), 40),
    ('noparking', 'SM_NS_SignPost_NoParking', (-1.2, 0.0, 2.7), (0.0, 0.0, 2.7), 30),
    ('nostanding', 'SM_NS_SignPost_NoStanding', (-1.2, 0.0, 2.7), (0.0, 0.0, 2.7), 30),
    ('stop', 'SM_NS_SignPost_Stop', (-1.8, 0.0, 2.55), (0.0, 0.0, 2.55), 30),
    ('busflag', 'SM_NS_SignPost_BusFlag', (-1.4, -0.28, 2.64), (0.0, -0.28, 2.64), 34),
    ('busflag_back', 'SM_NS_SignPost_BusFlag', (1.4, -0.28, 2.64), (0.0, -0.28, 2.64), 34),
    ('shelter_head', 'SM_NS_BusShelter', (-5.0, 0.0, 2.47), (0.0, 0.0, 2.47), 52),
    ('shelter_map', 'SM_NS_BusShelter', (-0.2, -1.38, 1.67), (0.72, -1.38, 1.67), 60),
    ('ped_sign', 'SM_NS_PedSignal', (-0.8, 0.0, 1.41), (0.0, 0.0, 1.41), 40),
    ('ice_front', 'SM_NS_IceChest', (-2.6, 0.0, 0.7), (0.0, 0.0, 0.7), 40),
]


def _mat(name, hexc, rough=0.5, emit=None, strength=0.0):
    m = core.Mat(name)
    m.set('Base Color', H(hexc))
    m.set('Roughness', rough)
    if emit is not None:
        m.set('Emission Color', H(emit))
        m.set('Emission Strength', strength)
    return m


def _cam(sc, cam_p, look_p, fov, path, w=900, h=900, samples=48):
    cd = bpy.data.cameras.new('rv_cam')
    co = core.link(bpy.data.objects.new('rv_cam', cd))
    cd.sensor_fit = 'HORIZONTAL'
    cd.angle = math.radians(fov)
    cd.clip_start = 0.02
    cd.clip_end = 500.0
    co.location = Vector(cam_p)
    co.rotation_euler = (Vector(look_p) - Vector(cam_p)).to_track_quat('-Z', 'Y').to_euler()
    sc.camera = co
    sc.render.resolution_x = w
    sc.render.resolution_y = h
    sc.render.resolution_percentage = 100
    sc.cycles.samples = int(os.environ.get('NS_REVIEW_SAMPLES', samples))
    sc.cycles.use_denoising = True
    sc.render.filepath = path
    sc.render.image_settings.file_format = 'PNG'
    bpy.ops.render.render(write_still=True)
    bpy.data.objects.remove(co)
    return path


def _world(sc, night):
    w = sc.world
    w.use_nodes = True
    nt = w.node_tree
    nt.nodes.clear()
    out = nt.nodes.new('ShaderNodeOutputWorld')
    bg = nt.nodes.new('ShaderNodeBackground')
    tc = nt.nodes.new('ShaderNodeTexCoord')
    sep = nt.nodes.new('ShaderNodeSeparateXYZ')
    nt.links.new(tc.outputs['Generated'], sep.inputs['Vector'])
    mr = nt.nodes.new('ShaderNodeMapRange')
    mr.inputs['From Min'].default_value = -0.05
    mr.inputs['From Max'].default_value = 0.6
    nt.links.new(sep.outputs['Z'], mr.inputs['Value'])
    mx = nt.nodes.new('ShaderNodeMix')
    mx.data_type = 'RGBA'
    nt.links.new(mr.outputs['Result'], mx.inputs[0])
    if night:
        mx.inputs[6].default_value = H(0x2a2420)
        mx.inputs[7].default_value = H(0x070910)
        bg.inputs['Strength'].default_value = 0.1
    else:
        mx.inputs[6].default_value = H(0xb9bec4)
        mx.inputs[7].default_value = H(0x7d8a9a)
        bg.inputs['Strength'].default_value = 1.0
    nt.links.new(mx.outputs[2], bg.inputs['Color'])
    nt.links.new(bg.outputs['Background'], out.inputs['Surface'])


def props_review(objs):
    sc = bpy.context.scene
    by = {o.name: o for o in objs}
    saved = {o.name: (o.hide_render, o.location.copy(), o.rotation_euler.copy()) for o in objs}
    temp = []
    d = os.path.join(core.BUILD_DIR, 'review', 'ns_kit')
    os.makedirs(d, exist_ok=True)
    which = _which_reviews()
    try:
        # preview the unbaked slots: glass clear, glows lit at night
        glows = []
        for o in objs:
            if '_Glow' in o.name:
                colr = {'Cool': COOL_GLOW, 'Warm': WARM_GLOW}.get(o.name.split('_Glow')[-1], None)
                if o.name.endswith('GlowSignal'):
                    m = core.Mat('rv_sig_' + o.name)
                    uv = m.node('ShaderNodeUVMap')
                    sp = m.node('ShaderNodeSeparateXYZ')
                    m.link(uv.outputs['UV'], sp.inputs['Vector'])
                    col_i = m.math('FLOOR', m.math('MULTIPLY', sp.outputs['X'], 4.0))
                    row_i = m.math('FLOOR', m.math('MULTIPLY', sp.outputs['Y'], 2.0))
                    cell = m.math('ADD', col_i, m.math('MULTIPLY', row_i, 4.0))
                    # show red on the mast, walk on the ped head (a review choice)
                    red = m.math('LESS_THAN', m.math('ABSOLUTE', m.math('SUBTRACT', cell, 0.0)), 0.5)
                    walk = m.math('LESS_THAN', m.math('ABSOLUTE', m.math('SUBTRACT', cell, 3.0)), 0.5)
                    cdn = m.math('LESS_THAN', m.math('ABSOLUTE', m.math('SUBTRACT', cell, 5.0)), 0.5)
                    col = m.mix(red, H(0x101010), H(0xff2a12))
                    col = m.mix(walk, col, H(0xf2f6ff))
                    col = m.mix(cdn, col, H(0xff8a1a))
                    m.set('Base Color', H(0x0c0c0c))
                    m.set('Emission Color', col)
                    m.set('Emission Strength', 0.0)
                    glows.append(m)
                    o.data.materials[0] = m.m
                elif colr is not None:
                    m = _mat('rv_' + o.name, colr, 0.5, colr, 0.0)
                    glows.append(m)
                    o.data.materials[0] = m.m
        floor = box(-30, 30, -30, 30, -0.3, 0.0, _mat('rv_floor', 0x5a5856, 0.75))
        temp.append(floor)
        d_ = bpy.data.lights.new('rv_sun', 'SUN')
        d_.energy = 3.2
        d_.angle = math.radians(4.0)
        sun = core.link(bpy.data.objects.new('rv_sun', d_))
        sun.rotation_euler = Vector((-0.55, -0.45, 0.7)).normalized().to_track_quat('Z', 'Y').to_euler()
        temp.append(sun)
        d2 = bpy.data.lights.new('rv_pt', 'AREA')
        d2.energy = 600.0
        d2.size = 2.0
        d2.color = (1.0, 0.72, 0.45)
        pt = core.link(bpy.data.objects.new('rv_pt', d2))
        temp.append(pt)
        if 'props' in which:
            for night in (False, True):
                _world(sc, night)
                sun.hide_render = night
                pt.hide_render = not night
                for m in glows:
                    m.bsdf.inputs['Emission Strength'].default_value = 6.0 if night else 1.5
                sc.view_settings.view_transform = 'AgX'
                sc.view_settings.exposure = 1.4 if night else -0.2
                paths = []
                for label, names, cam, tgt, fov in PROP_SHOTS:
                    show = [by[n] for n in names if n in by]
                    if not show:
                        continue
                    for o in objs:
                        o.hide_render = o not in show
                    for n in names:
                        if n in by:
                            by[n].location = WORK_LAYOUT.get(n, (0.0, 0.0, 0.0)) if label in ('work', 'litter', 'lockbike', 'lockbike_b') else (0.0, 0.0, 0.0)
                            by[n].rotation_euler = (0, 0, 0)
                    # awnings hang from z 0: lift them to 3.2 m on a wall
                    lift = 0.0
                    if 'Awning' in names[0]:
                        lift = 3.2
                        for n in names:
                            by[n].location = (0.0, 0.0, 3.2)
                        wall = box(0.0, 0.5, -6, 6, 0.0, 4.5, _mat('rv_wall', 0x6a3a2c, 0.85))
                        temp.append(wall)
                    pt.location = Vector(tgt) + Vector((-2.0, -1.5, 2.5 + lift))
                    c = Vector(cam) + Vector((0, 0, lift))
                    t = Vector(tgt) + Vector((0, 0, lift))
                    paths.append(_cam(sc, c, t, fov, os.path.join(d, f'p_{"n" if night else "d"}_{label}.png'), 900, 900, 40))
                    if lift:
                        bpy.data.objects.remove(temp.pop())
                compose_grid('ns_kit_props_' + ('night' if night else 'day'), paths, 4, TAG)
        if 'signs' in which:
            _world(sc, False)
            sun.hide_render = False
            pt.hide_render = True
            sc.view_settings.exposure = -0.2
            paths = []
            for label, name, cam, tgt, fov in SIGN_SHOTS:
                if name not in by:
                    continue
                for o in objs:
                    o.hide_render = o.name != name
                by[name].location = (0, 0, 0)
                by[name].rotation_euler = (0, 0, 0)
                paths.append(_cam(sc, cam, tgt, fov, os.path.join(d, f's_{label}.png'), 1200, 675, 24))
            if paths:
                compose_grid('ns_kit_signs', paths, 3, TAG)
    except Exception:
        traceback.print_exc()
    finally:
        for o in temp:
            try:
                bpy.data.objects.remove(o)
            except ReferenceError:
                pass
        for n, (h, loc, rot) in saved.items():
            by[n].hide_render, by[n].location, by[n].rotation_euler = h, loc, rot
        sc.view_settings.exposure = 0.0
        w = sc.world
        if w.node_tree is not None:
            nt = w.node_tree
            nt.nodes.clear()
            out = nt.nodes.new('ShaderNodeOutputWorld')
            bg = nt.nodes.new('ShaderNodeBackground')
            bg.inputs['Color'].default_value = (0.006, 0.0065, 0.008, 1.0)
            nt.links.new(bg.outputs['Background'], out.inputs['Surface'])


def pose_for_review(objs):
    """Renders art/review/ns_kit_props_day/night and ns_kit_signs, then lines the meshes up for the standard sheet."""
    global REVIEW_VIEWS
    t0 = time.time()
    for o in objs:
        o.hide_render = False
    try:
        props_review(objs)
    except Exception:
        traceback.print_exc()
    log(f'review renders in {time.time() - t0:.0f}s')
    by = {o.name: o for o in objs}
    y = 0.0
    for n in MESHES:
        if n not in by or '_Glow' in n:
            continue
        o = by[n]
        lo, hi = core.bounds([o])
        w = hi.y - lo.y
        o.location = (0.0, y - lo.y, 3.2 if 'Awning' in n else 0.0)
        o.rotation_euler = (0, 0, 0)
        for k in ('_GlowCool', '_GlowSignal', '_GlowWarm'):
            if n + k in by:
                by[n + k].location = o.location
        y += w + 0.6
    REVIEW_VIEWS = [('front', -70, 12, 0.55), ('top', -20, 40, 0.5), ('detail', -60, 15, 0.25)]
