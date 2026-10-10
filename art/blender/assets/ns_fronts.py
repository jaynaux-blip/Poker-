"""ns_fronts: Northside's building kit, its plots, the landmarks and the skyline (world realism spec 4.4, art-fronts).

Builds the buildings that replace every engine box on Fifth St, every plot on Market St, the three landmarks closing
the street ends, and the proxies that line the cross streets (world realism spec v2: 0.10 priorities, 2.3 plots, 2.4
shells, 4.4 contract), with the masonry texture sets they tile. Its sister module ns_rooms renders the rooms the kit's
windows look into (T_NS_Rooms_*), and its sister edits are facades.py (the barber bay) and tenement.py (the laundromat
split).

FRAMES (spec 0.4). Plots and landmarks are SITE meshes: Blender (0,0,0) is the stage origin, placed at the stage's
transform with yaw 180, stage (X, Y, Z) cm = (-100 x, 100 y, 100 z). The proxies are PROP: front toward Blender -X, origin
at the front's middle on the sidewalk line (Z 0), placed at L with yaw F + 180. The bank's clock hands and the grilles
are SITE meshes whose origin is moved to their pivot (see below). Printed text reads correctly in Blender from outside
its face, so it reads in the game (spec 0.4); every print is checked on the review sheets.

MESHES PER PLOT (spec 4.4), all SITE:
- SM_NS_Bldg_<Plot> (unbaked): walls, every opening's masonry reveals (17-30 cm), the sills and lintels in stone, the
  roof, the parapets and their coping. Slots: the masonry it uses in the order ns_brick_red, ns_brick_buff,
  ns_brick_dark, ns_brick_painted, ns_limestone, ns_caststone, ns_stucco, ns_roof, then ns_window [window]. Each tile
  slot's exported material is a constant near its set's mean times COLOR_0 (so the glTF carries COLOR_0; M_NS_Masonry
  replaces it by name). UV0 in meters: on a vertical face u = the position along the face as face_t runs (left to right
  seen from outside), v = Z; on a level face (u, v) = (x, y). Window and shop openings are snapped so their jambs land on
  half-bricks (0.1 m) and sills and heads on whole courses (1/15 m: 30 courses to the 2 m tile). COLOR_0 (linear): R soot
  and streaks (under every sill, AC unit and fire escape, under the cornice, splashed up from the sidewalk), G edge wear
  (the openings' arrises, the corners, hand height), B rain exposure (0.1 in heads, 0.35 in jambs, 0.4 under sign bands
  and cornices, 0.85 on open wall, 0.95 on sills, copings and roofs). The panes of ns_window: UV0 0..1 over the opening
  (0,0 its bottom left seen from outside), COLOR_0 R a per-window hash, G the window's height over the building's, B the
  room kind (0 apartment, 0.33 office or storage, 0.66 stairwell, 1 shop back room).
- SM_NS_Bldg_<Plot>_Trim (baked), slots in order: 0 stone (keystones, belt courses, cornices in stone, columns,
  surrounds), 1 metal (pressed-metal cornices, fire escapes, AC units, downspouts, grille hoods and guides, steel sash,
  lamps' bodies, brackets, water tanks), 2 storefront (sashes and frames, bulkheads, doors, transoms, sign boards'
  edges), 3 lit [lit] (the displays behind the shop glass, 1.5-2.3 m deep and dressed, the shops past the doors, the
  vestibules and lobbies; baked emission for DimImportedGlow), 4 prints (sign bands, lightbox faces (their emission
  baked), house numbers, buzzer panels, ghost signs, carved lettering). A slot a plot doesn't use holds one 1 cm stub on
  its roof, so every trim keeps the five slots in order. Texture sizes per slot from each slot's own area at a target
  density (prints 330 px/m, storefront 260, metal 220, stone 180, lit 110), the prints first, within the spec's budget
  (2048^2 per 10 m of frontage, +30%); the build log prints every slot's size.
- SM_NS_Bldg_<Plot>_Glass: the shop, door and transom glass (material ns_glass, M_NS_Glass replaces it; Nanite off).
- SM_NS_Bldg_<Plot>_Glow<Family>: Warm (sconces, gooseneck lamps, lobby canopy lights, floodlights, chaser bulbs), Cool
  (the pharmacy's cross), Neon (tube lettering; its design color is also in COLOR_0, linear: see the log).
- SM_NS_Bldg_<Plot>_Grille (baked, 1 slot): the roll-down gate of the plot's main shop opening, closed. Its origin is
  the top middle of that opening (the build log prints it in stage cm), so the game scales it in Z into its hood (the
  hood is in _Trim).

THE ENTERABLE SHOPS (spec 2.4): their openings are left clear (no glass, no lit room, no grille), the masonry reveal
returns 30 cm, and art-fronts builds the body and the band over them.
- Barber: facades.py's walk-up (see there).
- Thrift (MS2): Y 4950 plane, X -2440..-3660, Z 0..400. Over it a glazed terracotta frieze: SM_NS_Thrift_Front
  (4.6) carries the painted SECOND CHANCE THRIFT sign, so the text isn't doubled. (If the thrift's front stops short
  of a sign, build MS2 with NSF_THRIFT_BAND=1 to paint SECOND CHANCE THRIFT / St. Agnes Outreach on it.)
- Pawn (MS3): Y 4950 plane, X -3760..-4440, Z 0..380; its band reads NORTHSIDE PAWN & LOAN (gilt on black, two
  gooseneck lamps). The gold balls, the blue tube and the bars are ns_pawn's.
- Gym (MS4): Y 4950 plane, X -4560..-6540, Z 0..450; a plain steel fascia over it (IRONHOUSE's channel letters are
  SM_NS_Gym_Front's).
- Ruby's (Ruby): the whole ground storey on both faces and round the R 2.0 m corner, Z 0..430, is SM_NS_Ruby_Front's
  (its red fascia runs to Z 430, ns_diner's FZ); the body starts at Z 430 with a 30 cm soffit and a stone band. Pick:
  Ruby's ground storey is 4.3 m high.

THE PLOTS (stage cm; body type; floors; picks):
Fifth St, home side (fronts on X 0 facing west); full bar:
  - HHollis: Y -6200..-3600 and Hollis X 0..-1800, footprint to X -1800; B4, buff brick over a rusticated cast-stone
    ground storey, the corner chamfered 1.6 m with HOLLIS HARDWARE's door and a double-faced HARDWARE blade sign, a
    residents' door (1800) on Fifth, a fire escape on Hollis; 4 floors (ground 4.5 m, then 3.25 m); pressed-metal
    cornice round all three faces.
  - HKim: Y -3600..-2150, to X -1500; B1 red brick, keystone lintels, fire escape over bays 5-6; KIM'S PRODUCE with
    stepped fruit crates in the window; 5 floors (4.5 + 3.15 m).
  - HCash: Y 800..2300, to X -1500; B1 variant in painted brick, 3 floors (4.5 + 3.2 m); QUIKCASH lightbox band, the
    inset window behind bars with the blue tube round it (_GlowNeon, 0x2a6bff), a lit CHECKS CASHED · MONEY ORDERS ·
    BILL PAY lightbox in it; the residents' door 1816.
  - Ruby: Fifth Y 6950..8000 and Market X 0..-1500, to X -1500 / Y 8000; B4 buff brick, the corner rounded R 2.0 m
    all the way up (six facets), the ground storey left clear (above), stone cornice; 4 floors (4.3 + 3.2 m).
  - HWexler: Y 8000..9200 and Wexler X 0..-1500; B1 corner, red brick, hood lintels; NORTHSIDE WIRELESS (blue
    lightbox) on Fifth, the residents' door (9) and a fire escape on Wexler; 5 floors.
  Fifth St, far side (fronts on X 2040 facing east); full bar:
  - FHollis: Y -6200..-4300 and Hollis X 2040..3840; B4 dark glazed brick, chamfered corner with GOLDEN DRAGON's door,
    red lightbox bands (lit) on both faces, neon OPEN and TAKE OUT (_GlowNeon, 0xff3020), the residents' door 1801;
    4 floors.
  - FLobby: Y -4300..-2300, to X 3840; B2: two floors of rusticated cast stone, buff brick over, the lobby (1805) under
    a bronze canopy with three downlights, a stone cornice, a water tank; 6 floors (4.4 + 3.1 m).
  - FBotanica: Y -2300..-880, to X 3600; B1 red brick, keystones, a fire escape; ORTIZ BOTÁNICA (candles, saints);
    the door 1809; 5 floors.
  - FRecords: Y 4745..6300, to X 3540; B3, 2 floors (4.7 + 3.7 m), NORTHSIDE RECORDS band, neon RECORDS
    (0xff3a8a) in the window, the BRANDT BLDG 1926 parapet panel, offices in triple windows; the door 1829.
  - FWexler: Y 6300..9200 and Wexler X 2040..3840; B2 corner, 7 floors (4.4 + 3.05 m), two floors of rusticated
    limestone, NORTHSIDE DENTAL (gilt on dark green), the lobby (1901) under its canopy on Fifth, a steel garage door
    on Wexler, stone cornice, a water tank.
  Market St, south side (fronts on Y 4950 facing north, to Y 2400):
  - MS1 (full bar): X -1400..-2400; B1 red brick, 4 floors; NORTHSIDE PHARMACY (green lightbox), a lit green cross
    (_GlowCool), the door 202.
  - MS2 (full bar): X -2400..-3700; B3 former five-and-dime, 2 floors (5.0 + 3.8 m), buff brick, terracotta parapet
    panel C. W. MARSH & CO. · 5¢ · 10¢ · 25¢ STORE · 1928; the thrift opening left clear (above).
  - MS3 (full bar): X -3700..-4500; B1 painted brick, 4 floors, a fire escape; the pawn's opening left clear, its band
    NORTHSIDE PAWN & LOAN with two gooseneck lamps; the ghost sign MARKET ST. BOTTLING WORKS on its east party wall
    over the gym's roof (Z 11.7..14.9).
  - MS4 (full bar): X -4500..-6600; B5 brick warehouse, 2 tall floors (5.2 + 4.6 m), steel multi-light sash, brick
    pilasters, a corbelled band, a parapet; the gym's opening left clear under a plain steel fascia.
  - MS5 (proxy detail): X -6600..-7700; B1 red brick, 5 floors, hood lintels, fire escape; GINO'S SLICE (neon SLICE,
    0xff2a1a); the door 254.
  - MS6 (proxy detail): X -7700..-9600 and Cardinal Y 4950..3400; B5, 3 floors (4.8 + 4.2 m), steel sash; vacant:
    papered glass, FOR LEASE 555-0142, wheat-pasted posters and tags, a night light; the roll-up loading door on
    Cardinal (Y 3600..4200); the ghost sign CARDINAL STORAGE & MOVING on the Cardinal wall.
  Market St, north side (fronts on Y 6950 facing south, to Y 8600):
  - MN2 (full bar): X -1500..-2800; B1 red brick, 5 floors, hood lintels, 2-over-2 sash, a fire escape; HALDEN & SONS
    CLOTHIERS (gilt), mannequins in the window; the door 201.
  - MN3 (full bar): X -2800..-3600; B1 painted brick, 4 floors; LILY NAILS (pink lightbox) over a polish wall of small
    glossy bottles, manicure tables with their lamps and the clients' chairs; the door 205.
  - MN4 (full bar): X -3600..-4300; B1 buff brick, soldier-course heads, 4 floors; MARKET WINE & LIQUOR (lightbox),
    neon LIQUORS (0x2a8aff) in front of a back wall shelved with bottles (wine, spirits, longnecks: lathes) and stacked
    cases topped with wine in the window; the door 209.
  - MN5 (proxy detail): X -4300..-6300; B2, 6 floors, rusticated cast-stone base, THE MARLOWE on its canopy, the lobby,
    a stone cornice, a water tank.
  - MN6 (proxy detail): X -6300..-7600; B3 dark brick, 2 floors; THE ANCHOR in red neon (0xff2a1a) over a glass-block
    front with a lit bar room behind, a black door, COCKTAILS · BEER plaque.
  - MN7 (proxy detail): X -7600..-9600 and Cardinal Y 6950..8350; B4 red brick, 4 floors, chamfered corner;
    CARDINAL FURNITURE band, STORE CLOSING · EVERYTHING MUST GO · 70% OFF banners on both faces, the door 300 on
    Cardinal. (Its sidewalk shed is ns_kit's.)
  The Market proxy-detail set (MS5, MS6, MN5..MN7) shares about one 2048^2 of trim texture between its five plots.

LANDMARKS (B6, SITE):
- SM_NS_Vista_Bank (P0, full bar): Hollis Savings Bank across Hollis, facing north, X -180..2220 on Y -8040: a granite
  plinth (1.0 m) with five steps up to the portico (they reach 2.25 m out over Hollis's south sidewalk) between sloped
  granite cheek blocks, two bronze handrails down them, four engaged fluted Ionic columns (Ø 0.96 m, 10.5 m, entasis) on
  Attic bases, their capitals with scrolled volutes and egg-and-dart, two-storey bronze window walls between them, the
  bronze doors under a grilled transom and the 1911 cartouche, bronze plaques (the charter, the banking hours), bronze
  lanterns, rusticated wings with tall arched windows behind bronze grilles, the lit ATM vestibule in the east wing
  (24 h), an entablature (dentils, modillions) with HOLLIS SAVINGS BANK cut in its frieze, the attic with the clock in a
  laurel wreath and festooned panels, a balustrade over it (vase balusters between pedestals over the columns; top at
  stage Z 1857), four floodlights on the plinth (_GlowWarm). About 93k triangles (pick: the spec's ~400k is a target for
  a fully carved front; this one carries its carving in the 2048 stone bake and the tiling limestone, and every
  silhouette the street sees is modelled). The clock: separate hand meshes SM_NS_Vista_Bank_Hour and _Minute (baked,
  1 slot), each with its origin at the pivot, built pointing at 12 in the dial's plane (facing north); place them at the
  stage point the log prints and turn them clockwise as seen from the north. The dial is baked into _Trim's prints
  (enamel, Roman numerals, its emission lit a little at night). Meshes: _Trim, _Glass, _GlowWarm, _Hour, _Minute.
- SM_NS_Vista_Rialto (P0 at proxy detail first): the Rialto across Wexler, facing south, X -80..2120 on Y 11040:
  terracotta piers with stepped crowns, the stair-hall window over the marquee, chevron spandrels, a stepped parapet;
  the V-shaped marquee with chaser bulbs on both faces and its soffit; the blade RIALTO (porcelain enamel, 15 m) with
  neon round each letter and chasers on its edge; six closed doors, the boarded octagonal booth. Chaser bulbs are in
  _GlowWarm, each bulb its own island with its chase phase in UV0 (u = phase 0..1, v = 0.25 marquee faces, 0.5 soffit,
  0.75 blade). [screen] meshes: SM_NS_Vista_Rialto_Marquee (the letter boards on both faces of the V, each UV0 0..1:
  the same text both sides) and SM_NS_Vista_Rialto_Posters (four poster cases, case i at u i/4..(i+1)/4).
- SM_NS_Vista_Library (P0 at proxy detail first): the Northside Branch Library on Cardinal, facing west down Market,
  X -11440 face, Y 4750..7150: a rusticated raised base with barred basement windows, six arched reading-room windows
  (their panes ns_window of kind 1: T_NS_Rooms cell 15), the entrance pavilion with paired pilasters, seven granite
  steps (2.9 m out), two cast-iron lamp standards (_GlowWarm globes), the bronze book drop, NORTHSIDE BRANCH · PUBLIC
  LIBRARY cut in the frieze, a balustrade.
The Rialto's and the library's trims together stay about one 2048^2 each (proxy detail, spec 4.4).

PROXIES (P0, PROP): SM_NS_Proxy_A..H, kit fronts 20-30 m wide of 4-7 floors at the lowest detail (no chamfers, a
frame and a meeting rail per window, light fire escapes), 12 m deep with a roof. One mesh each: slot 0 the baked atlas
(2048, everything: masonry, trim, storefront), slot 1 ns_window (the windows and the shop windows, the latter kind 1).
They carry no lettering, so they can be mirrored. A: 24 m, 5 floors, red brick, two shops, a fire escape. B: 28 m, 7
floors, buff over cast stone, a lobby canopy, a water tank. C: 20 m, 4 floors, painted brick, one wide shop. D: 26 m, 6
floors, dark brick on a limestone base. E: 22 m, 4 floors, red brick, two shops. F: 30 m, 5 floors, stucco, three
shops. G: 25 m, 6 floors, red brick, a fire escape. H: 21 m, 5 floors, buff brick, hood lintels, a lobby.

SM_NS_Penny_Front (P1, SITE): the Lucky Penny's storefront (X 0 plane, Y 3700..4950), fitted to the store StreetStage
builds: its fixed glass (X -12..-10) and sliding doors (opening Y 4650..4850) stay code-built; this replaces the code's
low wall, mullions, transom bar, header and door head with a brick knee wall (Z 0..45) and an aluminum sill cap,
clear-anodized mullions (Y 4170, 4650, 4850) and jambs, the red transom bar, a painted header fascia (Z 330..480) and
its cap, the door surround and the operator's housing, and the box sign: an aluminum cabinet with returns on a
raceway, its face SM_NS_Penny_Front_Sign ([screen], UV0 0..1, 7.2 x 1.35 m centered on Y 4300, Z 405, its face at X 34.6:
PropArt's sign art goes there instead of the flat widget at X 4). Window decals LOTTERY, ATM INSIDE, COLD BEER,
HOT COFFEE on the glass's inner face; the hours sticker SM_NS_Penny_Front_Hours ([screen], UV0 0..1, Y 4880..4930,
Z 140..170). Slots: 0 metal (aluminum, enamel, paint), 1 brick, 2 prints (baked).

SKYLINE (P1, SITE): SM_NS_Skyline_N, _E, _S, _W: each a quarter of a ring of about 80 buildings 160-780 m from the set's
middle (stage X 1000, Y 1500), turned to face it: brick, limestone and stucco boxes 18-140 m tall (setbacks on the tall
ones), water tanks on the low ones, lit crowns (_GlowWarm / _GlowCool) on some towers, a window grid on every face the
set can see (ns_window panes 1.2 x 1.6 m on 2.6 x 3.2 m bays, each with its hash: they light by the hour). _N carries an
unnamed limestone clock tower (its dials _GlowWarm) 520 m north; _W a river bridge 620 m west with its lamps (_GlowWarm)
and the towers' red lights (_GlowNeon, 0xff2a1a). 48-55k triangles each. Their masonry and windows are the tile and
window slots of the plots, so M_NS_Masonry and M_NS_WindowRoom shade them too.
SM_NS_Ridge (P1, SITE): the ridge 3-5 km east (Blender +x 3000..5000 m) rising to 720 +/- 270 m, one slot ns_ridge (a
plain night forest; its look is the glows), the Embercrest on its shoulder over Market's axis (Y 5950) at about 520 m:
160 warm points (_GlowWarm) and the ember crown's three peaks (_GlowNeon, COLOR_0 EmberHex 0xff6a24). 16k triangles.

TEXTURES written to unreal/Art/Textures/NS (seamless 2 m tiles, 2048^2; BaseColor sRGB, Normal OpenGL +Y, ORM linear
(R cavity occlusion, G roughness, B metallic 0), Height linear 0..1 = -8..+2 mm): T_NS_Brick_Red, _Buff, _Dark
(glazed), _Painted (and T_NS_Brick_Painted_Mask: R the paint's hold, 0..1, low lets go first; M_NS_Masonry shows
T_NS_Brick_Red where it is under a loss level rising toward the ground, the spec's paint loss on the lower 1.2 m),
T_NS_Limestone, T_NS_CastStone, T_NS_Stucco, T_NS_Roof. Brick: common bond on a 20 x 6.67 cm module (10 bricks and
30 courses to the tile), 10 mm joints recessed 6 mm, a header course every sixth, a tone per brick, clinkers, washed
and burnt headers. The stones and the stucco have no joints (the geometry carries rustication and ashlar).

REVIEW: art/review/ns_fronts_textures.jpg (the sets, 2 x 2 and a 0.5 m detail), ns_fronts_plots_day/night.jpg (every
plot from across the street and at its shop), ns_fronts_w_day/night.jpg (the spec's W1, W2, W3, W5, W6, W8 with the
legacy fronts, art-street's ground where built and these plots, under an overcast sky or at night with the lamps,
the glows and the windows lit by the spec's interior mapping (preview_window: the pinhole projection of 3.4, the
room by the hash and kind, the front layer, occupancy 0.5)), ns_fronts.jpg (the standard sheet).

ENVIRONMENT: NSF_ONLY=HKim,MS3,VistaBank,ProxyA,tex builds a subset ('tex' the texture sets); NSF_TEX=0 skips the
sets; NSF_TEX_N=1024 makes them smaller for iterating; NSF_REVIEW=plots,w,tex picks the review renders; NSF_SHOTS=all
adds two more views per plot.
"""
import math
import os
import random
import struct
import sys
import time
import traceback
import zlib

import bmesh
import bpy  # noqa: F401
import numpy as np
from mathutils import Matrix, Vector

from artkit import core, parts, review
from artkit.shade import convex_edges, image_surface, mix_float, noise, object_coords, ramp, scaled, smooth_less, vec
from artkit.sheet import Sheet

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from assets import facades as fac  # noqa: E402  (the fronts' surface recipes, prints and lit rooms: same owner)

TEX_DIR = os.path.join(core.ROOT, 'unreal', 'Art', 'Textures', 'NS')
_ONLY = [s.strip() for s in os.environ.get('NSF_ONLY', '').split(',') if s.strip()]
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {}       # filled per plot (its trim slots by frontage)
AO_DISTANCE = 0.45
REVIEW_SCREEN_LIGHT = False
REVIEW_VIEWS = None      # set by pose_for_review

Z = Vector((0.0, 0.0, 1.0))


def hx(h, a=1.0):
    return core.hex_linear(h, a)


def _shade(h, f):
    r, g, b = (h >> 16) & 255, (h >> 8) & 255, h & 255
    return (max(0, min(255, int(r * f))) << 16) | (max(0, min(255, int(g * f))) << 8) | max(0, min(255, int(b * f)))


def _tint(h, f):
    r, g, b = (h >> 16) & 255, (h >> 8) & 255, h & 255
    return (int(r + (255 - r) * f) << 16) | (int(g + (255 - g) * f) << 8) | int(b + (255 - b) * f)


def S(X, Y, Z_=0.0):
    """Stage centimeters -> Blender meters (the SITE frame: x = -X/100)."""
    return Vector((-X / 100.0, Y / 100.0, Z_ / 100.0))


def stage_of(p):
    """Blender meters -> stage centimeters."""
    return (-p[0] * 100.0, p[1] * 100.0, p[2] * 100.0)


# ================================================================== the masonry texture sets (T_NS_*, 2 m tiles)

def _png(path, arr):
    """Writes an H x W (x C) float array (row 0 at the top) as an 8-bit PNG exactly as given."""
    a = np.asarray(arr, dtype=np.float32)
    if a.ndim == 2:
        a = a[:, :, None]
    h, w, c = a.shape
    data = (np.clip(a, 0.0, 1.0) * 255.0 + 0.5).astype(np.uint8)
    raw = b''.join(b'\x00' + data[y].tobytes() for y in range(h))

    def chunk(tag, body):
        return struct.pack('>I', len(body)) + tag + body + struct.pack('>I', zlib.crc32(tag + body) & 0xffffffff)
    kind = {1: 0, 3: 2, 4: 6}[c]
    png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, kind, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(raw, 6)) + chunk(b'IEND', b'')
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'wb') as f:
        f.write(png)


def lin2srgb(x):
    x = np.clip(x, 0.0, 1.0)
    return np.where(x <= 0.0031308, x * 12.92, 1.055 * np.power(np.maximum(x, 0.0031308), 1 / 2.4) - 0.055)


def lin(h):
    return np.array(hx(h)[:3], dtype=np.float32)


def tnoise(n, cells, seed, octaves=4, gain=0.5, cells_y=None):
    """Tileable fractal value noise on an n x n tile."""
    out = np.zeros((n, n), np.float32)
    amp, tot = 1.0, 0.0
    cy0 = cells_y if cells_y is not None else cells
    for o in range(octaves):
        px, py = int(cells * 2 ** o), int(cy0 * 2 ** o)
        if px > n or py > n:
            break
        rng = np.random.default_rng(seed * 1000 + o)
        lat = rng.random((py, px), dtype=np.float32)
        cx = np.arange(n, dtype=np.float32) * px / n
        cyy = np.arange(n, dtype=np.float32) * py / n
        ix0 = np.floor(cx).astype(np.int64)
        iy0 = np.floor(cyy).astype(np.int64)
        fx = cx - ix0
        fy = cyy - iy0
        fx = fx * fx * fx * (fx * (fx * 6 - 15) + 10)
        fy = fy * fy * fy * (fy * (fy * 6 - 15) + 10)
        ix1, iy1 = (ix0 + 1) % px, (iy0 + 1) % py
        ix0, iy0 = ix0 % px, iy0 % py
        a = lat[np.ix_(iy0, ix0)]
        b = lat[np.ix_(iy0, ix1)]
        c = lat[np.ix_(iy1, ix0)]
        d = lat[np.ix_(iy1, ix1)]
        fxx = fx[None, :]
        fyy = fy[:, None]
        out += amp * ((a * (1 - fxx) + b * fxx) * (1 - fyy) + (c * (1 - fxx) + d * fxx) * fyy)
        tot += amp
        amp *= gain
    return out / tot


def blur(a, passes=1):
    for _ in range(passes):
        a = (a * 2.0 + np.roll(a, 1, 0) + np.roll(a, -1, 0) + np.roll(a, 1, 1) + np.roll(a, -1, 1)) / 6.0
    return a


def grains(n, seed, soft=1):
    rng = np.random.default_rng(seed)
    return blur(rng.random((n, n), dtype=np.float32), soft)


def normal_from_height(h, texel, strength=1.0):
    """OpenGL (+Y up the image) tangent normal of a tileable height field (rows top-down, meters)."""
    gx = (np.roll(h, -1, 1) - np.roll(h, 1, 1)) / (2 * texel) * strength
    gup = (np.roll(h, 1, 0) - np.roll(h, -1, 0)) / (2 * texel) * strength      # up the image = row - 1
    nrm = np.stack([-gx, -gup, np.ones_like(h)], 2)
    nrm /= np.linalg.norm(nrm, axis=2, keepdims=True)
    return nrm * 0.5 + 0.5


def cavity_ao(h, scale_px=(2, 6, 16), gain=180.0):
    ao = np.ones_like(h)
    for s in scale_px:
        b = h.copy()
        for _ in range(s):
            b = (b + np.roll(b, 1, 0) + np.roll(b, -1, 0) + np.roll(b, 1, 1) + np.roll(b, -1, 1)) / 5.0
        ao -= np.clip((b - h) * gain, 0.0, 0.35)
    return np.clip(ao, 0.25, 1.0)


TILE_M = 2.0
TEX_N = int(os.environ.get('NSF_TEX_N', 2048))
# Height textures: 0..1 maps to HEIGHT_RANGE meters (below and above the wall's plane).
HEIGHT_RANGE = (-0.008, 0.002)


def _write_set(name, color_lin, rough, h, extra=None, metal=None, ao_gain=180.0):
    n = color_lin.shape[0]
    texel = TILE_M / n
    base = os.path.join(TEX_DIR, f'T_NS_{name}')
    _png(base + '_BaseColor.png', lin2srgb(color_lin))
    _png(base + '_Normal.png', normal_from_height(h, texel))
    ao = cavity_ao(h, gain=ao_gain)
    _png(base + '_ORM.png', np.stack([ao, np.clip(rough, 0.02, 1.0), np.zeros_like(rough) if metal is None else metal], 2))
    lo, hi = HEIGHT_RANGE
    _png(base + '_Height.png', np.clip((h - lo) / (hi - lo), 0.0, 1.0))
    for suffix, arr in (extra or {}).items():
        _png(base + f'_{suffix}.png', arr)
    print(f'[ns_fronts] wrote T_NS_{name}_* ({n}^2, {TILE_M:.0f} m)')


def _grid(n):
    c = (np.arange(n, dtype=np.float64) + 0.5) * TILE_M / n
    U, Vtop = np.meshgrid(c, c)
    return U, TILE_M - Vtop          # U across, V up (row 0 = the top of the tile)


def _brick_layout(n, seed, module=0.2, course=TILE_M / 30, joint=0.010, header_every=6):
    """Common bond: stretchers 19 x 5.7 cm on a 20 x 6.67 cm module (10 mm joints), a header course every sixth. Returns
    per-texel brick ids, a brick mask with rounded arrises, the distance to the nearest joint and the header flag."""
    U, Vv = _grid(n)
    ci = np.floor(Vv / course).astype(np.int64)
    vy = Vv - ci * course
    header = (ci % header_every) == 0
    mod = np.where(header, module / 2, module)
    off = np.where(header, module / 4, np.where(ci % 2 == 1, module / 2, 0.0))
    # Joints wander a millimeter or two (hand-laid).
    wob = (tnoise(n, 12, seed + 5, 2) - 0.5) * 0.0016
    uu = (U + off + wob) % TILE_M
    bi = np.floor(uu / mod).astype(np.int64)
    fu = uu - bi * mod
    du = np.minimum(fu, mod - fu) - joint / 2
    dv = np.minimum(vy, course - vy) - joint / 2 + (tnoise(n, 10, seed + 6, 2) - 0.5) * 0.001
    e = np.minimum(du, dv)
    bid = ci * 64 + bi + np.where(header, 1 << 20, 0)
    return bid, e, header, U, Vv


def _per_brick(bid, seed):
    """A random 0..1 per brick id (vectorized hash)."""
    h = (bid.astype(np.int64) * np.int64(73856093)) ^ np.int64(seed * 2654435761 & 0x7fffffff)
    h = (h ^ (h >> 13)) * np.int64(1274126177)
    h = h ^ (h >> 16)
    return ((h & 0xFFFFFF).astype(np.float32) / float(0xFFFFFF))


def brick_set(name, c1, c2, mortar, seed, clinker=0x3c1f18, washed=0xa86a50, burnt=0x3a1c14, rough=0.85, glaze=False, specks=None):
    n = TEX_N
    bid, e, header, U, Vv = _brick_layout(n, seed)
    r0, r1, r2, r3 = (_per_brick(bid, seed + k) for k in range(4))
    brick = e > 0.0
    # Arris: rounded and chipped over the outer 2.5 mm; the faces a little dished and pitted.
    arr = np.clip(e / 0.0025, 0.0, 1.0)
    chip_n = tnoise(n, 120, seed + 11, 3)
    chips = brick & (e < 0.006) & (chip_n > 0.68)
    sand = grains(n, seed + 12, 1)
    pits = (grains(n, seed + 13, 0) > 0.985).astype(np.float32)
    bulge = (tnoise(n, 10, seed + 14, 3) - 0.5) * 0.0012
    h_brick = -0.0016 * (1.0 - arr) ** 2 + bulge + (sand - 0.5) * 0.0003 - pits * 0.0009 - chips * 0.0022
    mortar_n = tnoise(n, 90, seed + 15, 3)
    h_mortar = -0.006 + (mortar_n - 0.5) * 0.0016 + (sand - 0.5) * 0.0004
    h = np.where(brick, h_brick, h_mortar).astype(np.float32)
    h = blur(h, 1)
    # Color: a tone per brick between the two ends of the range, a fire flash across some, clinkers and washed ones,
    # burnt headers.
    tone = r0[..., None]
    col = lin(c1) * (1 - tone) + lin(c2) * tone
    # Each brick its own brightness and a little hue (browner, redder), and fired unevenly along its length.
    col = col * (0.78 + 0.4 * r3[..., None])
    hue = (r1[..., None] - 0.5) * np.array([0.05, -0.02, -0.04], np.float32)
    col = np.clip(col * (1.0 + hue * 4.0), 0, 1)
    along = tnoise(n, 25, seed + 18, 2)[..., None]
    col = col * (0.9 + 0.2 * along)
    flash = np.clip((tnoise(n, 14, seed + 16, 3) - 0.45) * 2.0, 0, 1)[..., None] * (r1[..., None] > 0.7)
    col = col * (1.0 - 0.3 * flash)
    col = np.where((r2 > 0.975)[..., None], lin(clinker) * (0.85 + 0.3 * r3[..., None]), col)
    col = np.where(((r2 < 0.025))[..., None], lin(washed) * (0.9 + 0.2 * r3[..., None]), col)
    burnt_hdr = header & (r1 > 0.86)
    col = np.where(burnt_hdr[..., None], lin(burnt) * (0.8 + 0.5 * r3[..., None]), col)
    speck = grains(n, seed + 19, 0)
    col = col * (0.84 + 0.3 * sand[..., None]) * (1.0 - 0.25 * (speck > 0.93)[..., None])
    if specks is not None:
        sp = (grains(n, seed + 17, 0) > 0.975)[..., None]
        col = np.where(sp, lin(specks), col)
    col = col * (0.82 + 0.18 * arr[..., None])
    mcol = lin(mortar) * (0.7 + 0.35 * mortar_n[..., None]) * (0.85 + 0.3 * grains(n, seed + 20, 0)[..., None])
    col = np.where(brick[..., None], col, mcol)
    r = np.where(brick, rough + (r3 - 0.5) * 0.1 - (burnt_hdr * 0.2), 0.92).astype(np.float32)
    if glaze:
        r = np.where(brick, 0.35 + (r3 - 0.5) * 0.12 + (1 - arr) * 0.3, 0.9).astype(np.float32)
    _write_set(name, col.astype(np.float32), r, h)
    return col, h, brick


def painted_brick_set(seed=41):
    """Painted brick: an old coat of 0xb9b2a3 over red common bond, filling the joints half way, worn through on the
    arrises in flecks. T_NS_Brick_Painted_Mask (R) is the paint's hold, 0..1 (low lets go first): M_NS_Masonry shows
    T_NS_Brick_Red where R < a loss level that rises toward the ground (the spec's paint loss on the lower 1.2 m)."""
    n = TEX_N
    bid, e, header, U, Vv = _brick_layout(n, seed)
    brick = e > 0.0
    arr = np.clip(e / 0.0025, 0.0, 1.0)
    sand = grains(n, seed + 12, 1)
    h = np.where(brick, -0.0014 * (1.0 - arr) ** 2 + (tnoise(n, 10, seed + 14, 3) - 0.5) * 0.001, -0.0035 + (tnoise(n, 90, seed + 15, 3) - 0.5) * 0.001)
    brush = tnoise(n, 6, seed + 20, 4, cells_y=60)
    h = (h + (brush - 0.5) * 0.0002).astype(np.float32)
    h = blur(h, 2)
    paint = lin(0xb9b2a3)
    col = paint * (0.9 + 0.1 * brush[..., None]) * (0.95 + 0.06 * sand[..., None])
    # Touched up over the years: broad patches a shade apart.
    patch = tnoise(n, 3, seed + 21, 3)
    col = col * (0.94 + 0.1 * patch[..., None])
    col = np.where(brick[..., None], col, col * 0.8)
    # Flecks off the arrises showing the red under it.
    fleck = brick & (e < 0.004) & (tnoise(n, 140, seed + 22, 3) > 0.7)
    col = np.where(fleck[..., None], lin(0x7e3d2c) * (0.9 + 0.2 * sand[..., None]), col)
    r = np.where(fleck, 0.85, 0.7 - 0.05 * brush).astype(np.float32)
    hold = tnoise(n, 5, seed + 23, 5, 0.55)
    hold = np.clip((hold - hold.min()) / (hold.max() - hold.min()), 0, 1)
    _write_set('Brick_Painted', col.astype(np.float32), r, h, extra={'Mask': hold})


def stone_set(name, base, seed, rough=0.8, veins=0.0, fossils=0.0, tooling=0.0, pores=0.4, var=0.08, dark=None):
    """Honed or tooled stone with no joints (the geometry carries them): a fine grain, bedding, pores, fossils."""
    n = TEX_N
    U, Vv = _grid(n)
    g1 = grains(n, seed, 1)
    g2 = grains(n, seed + 1, 0)
    cloud = tnoise(n, 4, seed + 2, 5, 0.55)
    bed = tnoise(n, 3, seed + 3, 4, cells_y=40)
    col = lin(base) * (1.0 - var + 2 * var * cloud[..., None]) * (0.95 + 0.1 * g1[..., None])
    col = col * (0.96 + 0.08 * bed[..., None])
    por = (g2 > 1.0 - 0.02 * pores).astype(np.float32)
    col = col * (1.0 - 0.45 * por[..., None])
    # Aggregate: dark and light grains a millimeter or two across, and a mid-scale weathering mottle.
    g3 = grains(n, seed + 9, 1)
    col = col * (1.0 - 0.22 * np.clip((g3 - 0.62) * 8, 0, 1)[..., None]) * (1.0 + 0.12 * np.clip((0.38 - g3) * 8, 0, 1)[..., None])
    mott = tnoise(n, 12, seed + 10, 4, 0.6)
    col = col * (0.9 + 0.2 * mott[..., None])
    h = (g1 - 0.5) * 0.0003 - por * 0.0006 + (cloud - 0.5) * 0.0006
    if fossils > 0:
        fz = tnoise(n, 60, seed + 4, 2)
        fos = np.clip((fz - (1.0 - 0.12 * fossils)) * 25, 0, 1)
        col = col * (1.0 - 0.18 * fos[..., None]) + lin(_tint(base, 0.2)) * 0.1 * fos[..., None]
        h = h - fos * 0.0004
    if veins > 0:
        w = tnoise(n, 3, seed + 5, 4)
        vv = tnoise(n, 2, seed + 6, 6, 0.6)
        vein = np.clip(1.0 - np.abs(vv + 0.3 * w - 0.5) / 0.012, 0, 1) * veins
        col = col * (1.0 - 0.35 * vein[..., None])
    if tooling > 0:
        # Fine parallel tooling (drag-comb) on cast stone.
        t = 0.5 + 0.5 * np.sin(Vv * 2 * math.pi / 0.004 + tnoise(n, 20, seed + 7, 2) * 6)
        h = h + (t - 0.5) * 0.00025 * tooling
    if dark is not None:
        col = col * (1 - 0.3 * np.clip((tnoise(n, 8, seed + 8, 4) - 0.55) * 4, 0, 1))[..., None]
    r = (rough + (cloud - 0.5) * 0.08 + por * 0.1).astype(np.float32)
    _write_set(name, col.astype(np.float32), r, h.astype(np.float32), ao_gain=120.0)


def stucco_set(seed=61):
    n = TEX_N
    U, Vv = _grid(n)
    g = grains(n, seed, 1)
    sweep = tnoise(n, 6, seed + 1, 4, 0.6)
    arcs = 0.5 + 0.5 * np.sin((U * 1.7 + np.sin(Vv * 2.3) * 0.6 + sweep * 3.0) * 2 * math.pi / 0.35)
    h = (g - 0.5) * 0.0005 + (arcs - 0.5) * 0.0004 * sweep + (sweep - 0.5) * 0.001
    # Hairline cracks: ridges of a warped noise.
    cn = tnoise(n, 3, seed + 2, 5, 0.6)
    crack = np.clip(1.0 - np.abs(cn - 0.5) / 0.003, 0, 1) * (tnoise(n, 2, seed + 3, 3) > 0.62) * 0.6
    h = h - crack * 0.0008
    col = lin(0xb8ae9c) * (0.9 + 0.12 * sweep[..., None]) * (0.95 + 0.08 * g[..., None])
    patch = tnoise(n, 2, seed + 4, 3) > 0.7
    col = np.where(patch[..., None], col * 1.06, col)
    col = col * (1.0 - 0.35 * crack[..., None])
    r = (0.86 + (g - 0.5) * 0.06).astype(np.float32)
    _write_set('Stucco', col.astype(np.float32), r, h.astype(np.float32))


def roof_set(seed=71):
    """Modified-bitumen roofing: granulated cap sheet in 1 m rolls with 10 cm laps, a patch, tar along the seams."""
    n = TEX_N
    U, Vv = _grid(n)
    g = grains(n, seed, 0)
    g2 = grains(n, seed + 1, 1)
    roll = np.floor(Vv / 1.0)
    fy = Vv - roll
    lap = np.clip(1.0 - np.abs(fy - 0.1) / 0.01, 0, 1)
    seam_tar = np.clip(1.0 - np.abs(fy - 0.1) / 0.03, 0, 1) * (tnoise(n, 30, seed + 2, 2) > 0.45)
    h = (g - 0.5) * 0.0012 + np.where(fy < 0.1, 0.002, 0.0) - lap * 0.001
    col = lin(0x4e4d4a) * (0.75 + 0.5 * g[..., None]) * (0.9 + 0.2 * g2[..., None])
    col = col * (0.85 + 0.25 * tnoise(n, 4, seed + 3, 4)[..., None])
    col = np.where(seam_tar[..., None] > 0.5, lin(0x161616), col)
    ponding = np.clip((tnoise(n, 3, seed + 4, 4) - 0.6) * 5, 0, 1)
    col = col * (1.0 - 0.25 * ponding[..., None])
    r = np.where(seam_tar > 0.5, 0.45, 0.9 - 0.2 * ponding).astype(np.float32)
    _write_set('Roof', col.astype(np.float32), r, h.astype(np.float32))


def make_textures():
    t0 = time.time()
    brick_set('Brick_Red', 0x7a3a2a, 0x8f4a35, 0x8d877c, 11, clinker=0x3a1e18, burnt=0x4a2418)
    brick_set('Brick_Buff', 0x9c8160, 0xb39874, 0x9a9488, 21, clinker=0x6a5038, washed=0xc2aa88, burnt=0x6e5a44, specks=0x3a2a1a)
    brick_set('Brick_Dark', 0x2a1f1b, 0x3a2c25, 0x4f4b46, 31, clinker=0x1a1412, washed=0x4a3a30, burnt=0x1e1714, rough=0.45, glaze=True)
    painted_brick_set()
    stone_set('Limestone', 0xbdb4a2, 81, rough=0.8, fossils=1.0, pores=0.8, var=0.09, dark=True)
    stone_set('CastStone', 0x8a6f5a, 91, rough=0.82, tooling=1.0, pores=1.5, var=0.1, dark=True)
    stucco_set()
    roof_set()
    print(f'[ns_fronts] masonry sets in {time.time() - t0:.0f}s')


MASONRY_SETS = ['Brick_Red', 'Brick_Buff', 'Brick_Dark', 'Brick_Painted', 'Limestone', 'CastStone', 'Stucco', 'Roof']


def _load_px(path):
    """An image's pixels, rows bottom-up, as float RGBA."""
    img = bpy.data.images.load(path, check_existing=False)
    w, h = img.size
    a = np.empty(w * h * 4, dtype=np.float32)
    img.pixels.foreach_get(a)
    bpy.data.images.remove(img)
    return a.reshape(h, w, 4)


def _save_jpg(name, out):
    """out: H x W x 3 rows bottom-up -> art/review/<name>.jpg."""
    Hh, Ww = out.shape[:2]
    os.makedirs(core.REVIEW_DIR, exist_ok=True)
    path = os.path.join(core.REVIEW_DIR, f'{name}.jpg')
    img = bpy.data.images.new(f'rv_{name}', Ww, Hh, alpha=False)
    img.pixels.foreach_set(np.dstack([out, np.ones((Hh, Ww))]).astype(np.float32).ravel())
    img.filepath_raw = path
    img.file_format = 'JPEG'
    img.save(quality=90)
    bpy.data.images.remove(img)
    print(f'[ns_fronts] review sheet {os.path.relpath(path, core.ROOT)}')
    return path


def texture_sheet():
    """art/review/ns_fronts_textures.jpg: every masonry set tiled 2 x 2 (4 m square), raked by a light from the upper
    left through its normal map, so seams, joints and relief show; a 0.5 m detail of each under it."""
    tiles = []
    S_ = 640
    for nm in MASONRY_SETS:
        p = os.path.join(TEX_DIR, f'T_NS_{nm}_BaseColor.png')
        q = os.path.join(TEX_DIR, f'T_NS_{nm}_Normal.png')
        if not os.path.exists(p):
            continue
        a = _load_px(p)[:, :, :3]
        nrm = _load_px(q)[:, :, :3] * 2.0 - 1.0
        L = np.array([-0.5, 0.5, 0.7])
        L /= np.linalg.norm(L)
        lit = np.clip((nrm @ L) * 1.25, 0.15, 1.5)[..., None]
        shaded = np.clip(a * lit, 0, 1)
        t = np.tile(shaded, (2, 2, 1))
        f = t.shape[0] // S_
        t = t[:f * S_, :f * S_].reshape(S_, f, S_, f, 3).mean(axis=(1, 3))
        n = a.shape[0]
        crop = shaded[n - n // 4:, :n // 4]          # the top-left half meter
        ys = (np.arange(S_) * crop.shape[0] / S_).astype(int)
        xs = (np.arange(S_) * crop.shape[1] / S_).astype(int)
        tiles.append((t, crop[ys][:, xs]))
    if not tiles:
        return None
    gap = 6
    cols = len(tiles)
    Wt = cols * S_ + (cols - 1) * gap
    out = np.full((2 * S_ + gap, Wt, 3), 0.05, np.float32)
    for i, (t, c) in enumerate(tiles):
        x0 = i * (S_ + gap)
        out[S_ + gap:, x0:x0 + S_] = t
        out[:S_, x0:x0 + S_] = c
    return _save_jpg('ns_fronts_textures', out)


# ================================================================== the mesh builder

class MB:
    """Builds a mesh fast from Python lists: vertices, faces, per-face attributes, optional per-face UVs and colors."""

    def __init__(self):
        self.verts = []
        self.faces = []
        self.fattr = {}
        self.cur = {}
        self.fuv = {}
        self.fcol = {}

    def __len__(self):
        return len(self.faces)

    def set(self, **kw):
        self.cur.update(kw)
        return self

    def v(self, p):
        self.verts.append((float(p[0]), float(p[1]), float(p[2])))
        return len(self.verts) - 1

    def f(self, idx, center=None, want=None, uv=None, col=None, uvall=None, **attrs):
        if uvall is not None:
            uv = [tuple(uvall)] * len(idx)
        if center is not None or want is not None:
            pts = [Vector(self.verts[i]) for i in idx]
            n = Vector((0.0, 0.0, 0.0))
            for a, b in zip(pts, pts[1:] + pts[:1]):
                n.x += (a.y - b.y) * (a.z + b.z)
                n.y += (a.z - b.z) * (a.x + b.x)
                n.z += (a.x - b.x) * (a.y + b.y)
            d = (sum(pts, Vector()) / len(pts) - Vector(center)) if center is not None else Vector(want)
            if n.dot(d) < 0.0:
                idx = list(reversed(idx))
                if uv is not None:
                    uv = list(reversed(uv))
                if col is not None:
                    col = list(reversed(col))
        k = len(self.faces)
        self.faces.append(list(idx))
        vals = dict(self.cur)
        vals.update(attrs)
        for name, val in vals.items():
            lst = self.fattr.get(name)
            if lst is None:
                lst = self.fattr[name] = [None] * k
            elif len(lst) < k:
                lst += [None] * (k - len(lst))
            lst.append(val)
        if uv is not None:
            self.fuv[k] = uv
        if col is not None:
            self.fcol[k] = col
        return k

    def poly(self, pts, want=None, center=None, uv=None, col=None, **attrs):
        return self.f([self.v(p) for p in pts], center=center, want=want, uv=uv, col=col, **attrs)

    def obox(self, center, axes, half, cham=0.0, skip=(), seg=None, **attrs):
        """An oriented box: center, three unit axes, half sizes, every edge chamfered (or rounded, seg >= 2) by cham.
        skip: sides to leave out, as (axis index, sign)."""
        c = Vector(center)
        ax = [Vector(a) for a in axes]
        h = list(half)
        cm = min(cham, min(h) * 0.45)

        def P(s, o):
            return c + ax[0] * (s[0] * o[0]) + ax[1] * (s[1] * o[1]) + ax[2] * (s[2] * o[2])
        if cm <= 1e-6:
            idx = {}
            for sx in (-1, 1):
                for sy in (-1, 1):
                    for sz in (-1, 1):
                        idx[(sx, sy, sz)] = self.v(P((sx, sy, sz), h))
            for a in range(3):
                for s in (-1, 1):
                    if (a, s) in skip:
                        continue
                    b, d = (a + 1) % 3, (a + 2) % 3
                    ring = []
                    for (u, w) in ((-1, -1), (1, -1), (1, 1), (-1, 1)):
                        key = [0, 0, 0]
                        key[a], key[b], key[d] = s, u, w
                        ring.append(idx[tuple(key)])
                    self.f(ring, center=c, **attrs)
            return
        if seg is None:
            seg = 2 if cm >= 0.006 else 1
        if seg >= 2:
            self._rounded(c, ax, h, cm, seg, skip, attrs)
            return
        vi = {}
        for sx in (-1, 1):
            for sy in (-1, 1):
                for sz in (-1, 1):
                    s = (sx, sy, sz)
                    vi[(s, 0)] = self.v(P(s, (h[0], h[1] - cm, h[2] - cm)))
                    vi[(s, 1)] = self.v(P(s, (h[0] - cm, h[1], h[2] - cm)))
                    vi[(s, 2)] = self.v(P(s, (h[0] - cm, h[1] - cm, h[2])))
        for a in range(3):
            for sgn in (-1, 1):
                if (a, sgn) in skip:
                    continue
                b, d = (a + 1) % 3, (a + 2) % 3
                ring = []
                for (u, w) in ((-1, -1), (1, -1), (1, 1), (-1, 1)):
                    key = [0, 0, 0]
                    key[a], key[b], key[d] = sgn, u, w
                    ring.append(vi[(tuple(key), a)])
                self.f(ring, center=c, **attrs)
        for a in range(3):
            b, d = (a + 1) % 3, (a + 2) % 3
            for sb in (-1, 1):
                for sd in (-1, 1):
                    k0 = [0, 0, 0]
                    k1 = [0, 0, 0]
                    k0[a], k0[b], k0[d] = -1, sb, sd
                    k1[a], k1[b], k1[d] = 1, sb, sd
                    self.f([vi[(tuple(k0), b)], vi[(tuple(k0), d)], vi[(tuple(k1), d)], vi[(tuple(k1), b)]], center=c, **attrs)
        for sx in (-1, 1):
            for sy in (-1, 1):
                for sz in (-1, 1):
                    s = (sx, sy, sz)
                    self.f([vi[(s, 0)], vi[(s, 1)], vi[(s, 2)]], center=c, **attrs)

    def _rounded(self, c, ax, h, r, s, skip, attrs):
        hi = [hh - r for hh in h]
        th = [math.tan((math.pi / 4) * i / s) for i in range(1, s + 1)]

        def samples(k):
            pos = [hi[k] + r * t for t in th]
            return [-p for p in reversed(pos)] + [-hi[k], hi[k]] + pos
        cache = {}

        def vert(p):
            key = (round(p[0], 7), round(p[1], 7), round(p[2], 7))
            if key in cache:
                return cache[key]
            q = [min(max(p[i], -hi[i]), hi[i]) for i in range(3)]
            d = [p[i] - q[i] for i in range(3)]
            L = math.sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2])
            if L > 1e-12:
                q = [q[i] + d[i] * r / L for i in range(3)]
            idx = self.v(c + ax[0] * q[0] + ax[1] * q[1] + ax[2] * q[2])
            cache[key] = idx
            return idx
        for a in range(3):
            for sgn in (-1, 1):
                if (a, sgn) in skip:
                    continue
                b, d = (a + 1) % 3, (a + 2) % 3
                Sb, Sd = samples(b), samples(d)
                grid = []
                for yb in Sb:
                    row = []
                    for zd in Sd:
                        p = [0.0, 0.0, 0.0]
                        p[a], p[b], p[d] = sgn * h[a], yb, zd
                        row.append(vert(p))
                    grid.append(row)
                for i in range(len(Sb) - 1):
                    for j in range(len(Sd) - 1):
                        self.f([grid[i][j], grid[i + 1][j], grid[i + 1][j + 1], grid[i][j + 1]], center=c, **attrs)

    def box(self, x0, x1, y0, y1, z0, z1, cham=0.0, skip=(), seg=None, **attrs):
        self.obox(((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), ((1, 0, 0), (0, 1, 0), (0, 0, 1)),
                  (abs(x1 - x0) / 2, abs(y1 - y0) / 2, abs(z1 - z0) / 2), cham, skip, seg, **attrs)

    def beam(self, a, b, w, h, up=(0.0, 0.0, 1.0), cham=0.0, seg=None, **attrs):
        a, b = Vector(a), Vector(b)
        d = b - a
        L = d.length
        if L < 1e-6:
            return
        t = d / L
        u = Vector(up)
        if abs(t.dot(u)) > 0.98:
            u = Vector((1.0, 0.0, 0.0)) if abs(t.x) < 0.9 else Vector((0.0, 1.0, 0.0))
        s = t.cross(u).normalized()
        n = s.cross(t).normalized()
        self.obox((a + b) / 2, (t, s, n), (L / 2, w / 2, h / 2), cham, (), seg, **attrs)

    def tube(self, a, b, r0, r1=None, n=12, caps=True, phase=0.0, **attrs):
        r1 = r0 if r1 is None else r1
        a, b = Vector(a), Vector(b)
        if (b - a).length < 1e-6:
            return
        t = (b - a).normalized()
        ref = Vector((0, 0, 1)) if abs(t.z) < 0.9 else Vector((1, 0, 0))
        u = t.cross(ref).normalized()
        w = t.cross(u)
        ra, rb = [], []
        for i in range(n):
            ang = phase + 2 * math.pi * i / n
            d = u * math.cos(ang) + w * math.sin(ang)
            ra.append(self.v(a + d * r0))
            rb.append(self.v(b + d * r1))
        for i in range(n):
            j = (i + 1) % n
            mid = (a + b) / 2
            self.f([ra[i], ra[j], rb[j], rb[i]], center=mid, **attrs)
        if caps:
            self.f(ra, want=-t, **attrs)
            self.f(rb, want=t, **attrs)

    def path_tube(self, pts, r, n=8, closed=False, caps=True, **attrs):
        """A round tube through a polyline (parallel-transported rings): neon, conduit, railings."""
        pts = [Vector(p) for p in pts]
        m = len(pts)
        if m < 2:
            return
        tans = []
        for i in range(m):
            a = pts[i - 1] if (closed or i > 0) else pts[i]
            b = pts[(i + 1) % m] if (closed or i < m - 1) else pts[i]
            d = b - a
            tans.append(d.normalized() if d.length > 1e-9 else Vector((1, 0, 0)))
        ref = Vector((0, 0, 1)) if abs(tans[0].z) < 0.9 else Vector((1, 0, 0))
        nrm = (ref - tans[0] * ref.dot(tans[0])).normalized()
        rings = []
        for i in range(m):
            if i > 0:
                nrm = tans[i - 1].rotation_difference(tans[i]) @ nrm
                nrm = (nrm - tans[i] * nrm.dot(tans[i])).normalized()
            bn = tans[i].cross(nrm)
            rings.append([self.v(pts[i] + (nrm * math.cos(2 * math.pi * k / n) + bn * math.sin(2 * math.pi * k / n)) * r) for k in range(n)])
        for i in range(m if closed else m - 1):
            A, B = rings[i], rings[(i + 1) % m]
            mid = (pts[i] + pts[(i + 1) % m]) / 2
            for k in range(n):
                self.f([A[k], A[(k + 1) % n], B[(k + 1) % n], B[k]], center=mid, **attrs)
        if caps and not closed:
            self.f(rings[0], want=-tans[0], **attrs)
            self.f(rings[-1], want=tans[-1], **attrs)

    def lathe(self, profile, center=(0.0, 0.0, 0.0), n=32, a0=0.0, a1=2 * math.pi, axis=None, **attrs):
        """Revolves (r, z) points (bottom to top) about a vertical axis through center (or about axis: a 3x3 Matrix
        whose third column is the axis); outward normals for a profile climbing the outside."""
        cx, cy, cz = center
        R = axis if axis is not None else Matrix.Identity(3)
        full = abs((a1 - a0) - 2 * math.pi) < 1e-6
        steps = n if full else n + 1
        rings = []
        for r, z in profile:
            if r <= 1e-7:
                rings.append([self.v(Vector((cx, cy, cz)) + R @ Vector((0.0, 0.0, z)))])
            else:
                rings.append([self.v(Vector((cx, cy, cz)) + R @ Vector((r * math.cos(a0 + (a1 - a0) * i / n), r * math.sin(a0 + (a1 - a0) * i / n), z)))
                              for i in range(steps)])
        for j in range(len(rings) - 1):
            A, B = rings[j], rings[j + 1]
            dr = profile[j + 1][0] - profile[j][0]
            dz = profile[j + 1][1] - profile[j][1]
            for i in range(n):
                i1 = (i + 1) % steps if full else i + 1
                ang = a0 + (a1 - a0) * (i + 0.5) / n
                out = R @ Vector((math.cos(ang) * dz, math.sin(ang) * dz, -dr))
                if len(A) == 1 and len(B) == 1:
                    continue
                if len(A) == 1:
                    self.f([A[0], B[i1], B[i]], want=out, **attrs)
                elif len(B) == 1:
                    self.f([A[i], A[i1], B[0]], want=out, **attrs)
                else:
                    self.f([A[i], A[i1], B[i1], B[i]], want=out, **attrs)

    def molding(self, path, prof, z0=0.0, caps=True, **attrs):
        """A profile prof [(o, z)] (o out from the wall, z up; a closed outline) run along a horizontal polyline path
        [(x, y)] whose outward side is on the RIGHT of travel (a CCW footprint's walls), mitered at the corners."""
        pts = [Vector((p[0], p[1], 0.0)) for p in path]
        if len(pts) < 2:
            return
        area = sum(a[0] * b[1] - b[0] * a[1] for a, b in zip(prof, prof[1:] + prof[:1]))
        if area < 0:
            prof = list(reversed(prof))
        ns = []
        for a, b in zip(pts, pts[1:]):
            d = (b - a).normalized()
            ns.append(Vector((d.y, -d.x, 0.0)))
        miters = []
        for i in range(len(pts)):
            if i == 0:
                mv = ns[0]
            elif i == len(pts) - 1:
                mv = ns[-1]
            else:
                s = (ns[i - 1] + ns[i])
                s = s.normalized() if s.length > 1e-6 else ns[i]
                mv = s / max(s.dot(ns[i]), 0.25)
            miters.append(mv)
        rings = [[self.v(pts[i] + miters[i] * o + Vector((0.0, 0.0, z0 + z))) for o, z in prof] for i in range(len(pts))]
        k = len(prof)
        for i in range(len(pts) - 1):
            nseg = ns[i]
            for j in range(k):
                (o0, z0_), (o1, z1_) = prof[j], prof[(j + 1) % k]
                do, dz = o1 - o0, z1_ - z0_
                want = nseg * dz + Vector((0.0, 0.0, -do))
                self.f([rings[i][j], rings[i][(j + 1) % k], rings[i + 1][(j + 1) % k], rings[i + 1][j]], want=want, **attrs)
        if caps:
            d0 = (pts[1] - pts[0]).normalized()
            d1 = (pts[-1] - pts[-2]).normalized()
            self.f(rings[0], want=-d0, **attrs)
            self.f(rings[-1], want=d1, **attrs)

    def merge(self, other):
        off = len(self.verts)
        self.verts += other.verts
        k0 = len(self.faces)
        for fc in other.faces:
            self.faces.append([i + off for i in fc])
        names = set(self.fattr) | set(other.fattr)
        for name in names:
            mine = self.fattr.setdefault(name, [None] * k0)
            if len(mine) < k0:
                mine += [None] * (k0 - len(mine))
            theirs = other.fattr.get(name, [])
            theirs = theirs + [None] * (len(other.faces) - len(theirs))
            mine += theirs
        for k, v in other.fuv.items():
            self.fuv[k + k0] = v
        for k, v in other.fcol.items():
            self.fcol[k + k0] = v

    def obj(self, name, uv_meters=False):
        """The object. Faces with explicit UVs get them; with uv_meters, the rest get UV0 in meters by their normal (a
        vertical face: u along it as face_t runs, v = z; a level face: u = x, v = y). Explicit colors go to a corner color
        attribute 'Color' (COLOR_0)."""
        me = bpy.data.meshes.new(name)
        me.from_pydata(self.verts, [], self.faces)
        me.update()
        n = len(self.faces)
        for aname, vals in self.fattr.items():
            vals = vals + [None] * (n - len(vals))
            if all(v is None for v in vals):
                continue
            is_vec = any(isinstance(v, (tuple, list, Vector)) for v in vals if v is not None)
            if is_vec:
                a = me.attributes.new(aname, 'FLOAT_VECTOR', 'FACE')
                flat = []
                for v in vals:
                    flat += list(v) if v is not None else [0.0, 0.0, 0.0]
                a.data.foreach_set('vector', flat)
            else:
                a = me.attributes.new(aname, 'FLOAT', 'FACE')
                a.data.foreach_set('value', [float(v) if v is not None else 0.0 for v in vals])
        if self.fuv or uv_meters:
            uvl = me.uv_layers.new(name='UVMap')
            uvs = np.zeros((len(me.loops), 2), np.float32)
            co = np.array(self.verts, np.float64)
            for fi, poly in enumerate(me.polygons):
                ls = poly.loop_start
                if fi in self.fuv:
                    for k, (u, v) in enumerate(self.fuv[fi]):
                        uvs[ls + k] = (u, v)
                elif uv_meters:
                    nn = poly.normal
                    vids = poly.vertices
                    if abs(nn.z) > 0.7:
                        for k, vi in enumerate(vids):
                            uvs[ls + k] = (co[vi][0], co[vi][1])
                    else:
                        t = fac.face_t(nn)
                        for k, vi in enumerate(vids):
                            p = co[vi]
                            uvs[ls + k] = (p[0] * t.x + p[1] * t.y, p[2])
            uvl.data.foreach_set('uv', uvs.ravel())
        if self.fcol:
            ca = me.color_attributes.new('Color', 'FLOAT_COLOR', 'CORNER')
            cols = np.zeros((len(me.loops), 4), np.float32)
            cols[:, 3] = 1.0
            for fi, poly in enumerate(me.polygons):
                c = self.fcol.get(fi)
                if c is None:
                    continue
                ls = poly.loop_start
                for k in range(poly.loop_total):
                    cols[ls + k] = c[k] if isinstance(c[0], (tuple, list)) else c
            ca.data.foreach_set('color', cols.ravel())
            me.color_attributes.active_color = ca
            me.color_attributes.render_color_index = me.color_attributes.find('Color')
        o = bpy.data.objects.new(name, me)
        core.link(o)
        return o


# ================================================================== a wall face's frame

_LOWPOLY = [False]


class Face:
    """One wall of a footprint (CCW seen from above): from a to b along t, which runs left to right for someone
    outside looking at it; n is its outward normal. Local (u, z, w): u along it from a, z up, w into the building."""

    def __init__(self, a, b, kind='street', key=''):
        self.a = Vector((a[0], a[1], 0.0))
        self.b = Vector((b[0], b[1], 0.0))
        d = self.b - self.a
        self.L = d.length
        self.t = d / self.L
        self.n = Vector((self.t.y, -self.t.x, 0.0))
        self.kind = kind
        self.key = key

    def P(self, u, z, w=0.0):
        return self.a + self.t * u + Z * z - self.n * w

    def axes(self):
        return (self.t, Z, -self.n)

    def box(self, mb, u0, u1, z0, z1, w0, w1, cham=0.0, skip=(), seg=None, **attrs):
        """A box in the face's frame (skip sides as (axis, sign) in (u, z, w)). Proxies build without chamfers."""
        if _LOWPOLY[0]:
            cham = 0.0
        c = self.P((u0 + u1) / 2, (z0 + z1) / 2, (w0 + w1) / 2)
        mb.obox(c, self.axes(), (abs(u1 - u0) / 2, abs(z1 - z0) / 2, abs(w1 - w0) / 2), cham, skip, seg, **attrs)

    def quad(self, mb, u0, u1, z0, z1, w, facing=-1, uv=None, col=None, **attrs):
        """A quad in the plane w (facing out of the wall, facing=-1, or into it)."""
        pts = [self.P(u0, z0, w), self.P(u1, z0, w), self.P(u1, z1, w), self.P(u0, z1, w)]
        return mb.poly(pts, want=self.n * (-facing), uv=uv, col=col, **attrs)

    def prism(self, mb, prof, u0, u1, **attrs):
        """A closed (w, z) outline run along the face from u0 to u1 (sills, hoods, brackets, steps)."""
        a = [mb.v(self.P(u0, z, w)) for w, z in prof]
        b = [mb.v(self.P(u1, z, w)) for w, z in prof]
        k = len(prof)
        cw = sum(w for w, z in prof) / k
        cz = sum(z for w, z in prof) / k
        c = self.P((u0 + u1) / 2, cz, cw)
        mb.f(a, want=-self.t, **attrs)
        mb.f(list(reversed(b)), want=self.t, **attrs)
        for i in range(k):
            mb.f([a[i], a[(i + 1) % k], b[(i + 1) % k], b[i]], center=c, **attrs)

    def uworld(self, u):
        """The meters-UV u of a point at u on this face (the brick coursing's u)."""
        p = self.P(u, 0.0)
        t = fac.face_t(self.n)
        return p.x * t.x + p.y * t.y


def snap_u(F, u, step=0.1):
    """u moved so the brick coursing lands a whole half-brick there."""
    w = F.uworld(u)
    return u + (round(w / step) * step - w)


COURSE = TILE_M / 30


def snap_z(z):
    return round(z / COURSE) * COURSE


def offset_poly(poly, d):
    """A CCW polygon moved inward by d (each edge shifted along its inward normal, neighbors intersected)."""
    pts = [Vector((p[0], p[1])) for p in poly]
    k = len(pts)
    lines = []
    for i in range(k):
        a, b = pts[i], pts[(i + 1) % k]
        t = (b - a).normalized()
        nin = Vector((-t.y, t.x))
        lines.append((a + nin * d, t))
    out = []
    for i in range(k):
        (p0, t0), (p1, t1) = lines[i - 1], lines[i]
        den = t0.x * t1.y - t0.y * t1.x
        if abs(den) < 1e-9:
            out.append((p1.x, p1.y))
            continue
        s = ((p1.x - p0.x) * t1.y - (p1.y - p0.y) * t1.x) / den
        q = p0 + t0 * s
        out.append((q.x, q.y))
    return out


def wall(mb, F, u0, u1, z0, z1, openings, slot_of, sub=0.6, ends=(None, None), top=False, region=None, **attrs):
    """A masonry face in F's plane from (u0, z0) to (u1, z1) with rectangular openings [dict(u0, u1, z0, z1, depth,
    slot)] cut through it, reveals depth deep. Built on the openings' grid and subdivided to sub, so every face is a clean
    quad and COLOR_0 has vertices to vary on. slot_of(z) picks the face's masonry by height. ends: how deep the returns
    at u0 / u1 run (None: none). Faces carry uvm=1 (meters UVs) and reg (region tag for the colors)."""
    base_reg = attrs.pop('reg', 0)
    us = {round(u0, 5), round(u1, 5)}
    zs = {round(z0, 5), round(z1, 5)}
    for o in openings:
        for v in (o['u0'], o['u1']):
            if u0 - 1e-6 <= v <= u1 + 1e-6:
                us.add(round(v, 5))
        for v in (o['z0'], o['z1']):
            if z0 - 1e-6 <= v <= z1 + 1e-6:
                zs.add(round(v, 5))
    for zz in getattr(slot_of, 'splits', ()):
        if z0 < zz < z1:
            zs.add(round(zz, 5))

    def refine(vals):
        vals = sorted(vals)
        out = [vals[0]]
        for a, b in zip(vals, vals[1:]):
            n = max(1, int(math.ceil((b - a) / sub - 1e-6)))
            for i in range(1, n + 1):
                out.append(round(a + (b - a) * i / n, 5))
        return out
    us, zs = refine(us), refine(zs)
    cache = {}

    def V(u, z, w=0.0):
        k = (round(u, 5), round(z, 5), round(w, 5))
        if k not in cache:
            cache[k] = mb.v(F.P(u, z, w))
        return cache[k]

    def inside(cu, cz):
        return any(o['u0'] < cu < o['u1'] and o['z0'] < cz < o['z1'] for o in openings)
    out_n = F.n
    for a, b in zip(us, us[1:]):
        for c, d in zip(zs, zs[1:]):
            if not inside((a + b) / 2, (c + d) / 2):
                mb.f([V(a, c), V(b, c), V(b, d), V(a, d)], want=out_n, slot=slot_of((c + d) / 2), uvm=1, reg=base_reg, **attrs)
    for o in openings:
        dep = o.get('depth', 0.3)
        sl = o.get('slot', None)
        uu = [v for v in us if o['u0'] - 1e-6 <= v <= o['u1'] + 1e-6]
        zz = [v for v in zs if o['z0'] - 1e-6 <= v <= o['z1'] + 1e-6]
        if o.get('head', True) and o['z1'] < z1 - 1e-6:
            for a, b in zip(uu, uu[1:]):
                mb.f([V(a, o['z1']), V(b, o['z1']), V(b, o['z1'], dep), V(a, o['z1'], dep)], want=-Z, slot=sl if sl is not None else slot_of(o['z1'] + 0.01),
                     uvm=1, reg=2, **attrs)
        if o.get('sill', True) and o['z0'] > z0 + 1e-6:
            for a, b in zip(uu, uu[1:]):
                mb.f([V(a, o['z0']), V(b, o['z0']), V(b, o['z0'], dep), V(a, o['z0'], dep)], want=Z, slot=sl if sl is not None else slot_of(o['z0'] - 0.01),
                     uvm=1, reg=3, **attrs)
        for c, d in zip(zz, zz[1:]):
            s_ = sl if sl is not None else slot_of((c + d) / 2)
            if o.get('jambs', True) and o['u0'] > u0 + 1e-6:
                mb.f([V(o['u0'], c), V(o['u0'], d), V(o['u0'], d, dep), V(o['u0'], c, dep)], want=F.t, slot=s_, uvm=1, reg=4, **attrs)
            if o.get('jambs', True) and o['u1'] < u1 - 1e-6:
                mb.f([V(o['u1'], c), V(o['u1'], d), V(o['u1'], d, dep), V(o['u1'], c, dep)], want=-F.t, slot=s_, uvm=1, reg=4, **attrs)
    for side, uu_, dep in ((0, u0, ends[0]), (1, u1, ends[1])):
        if dep is None:
            continue
        for c, d in zip(zs, zs[1:]):
            if inside(uu_ + (1e-4 if side == 0 else -1e-4), (c + d) / 2):
                continue
            mb.f([V(uu_, c), V(uu_, d), V(uu_, d, dep), V(uu_, c, dep)], want=-F.t if side == 0 else F.t, slot=slot_of((c + d) / 2), uvm=1, reg=base_reg, **attrs)
    if top:
        dep = top if isinstance(top, float) else 0.3
        for a, b in zip(us, us[1:]):
            mb.f([V(a, z1), V(b, z1), V(b, z1, dep), V(a, z1, dep)], want=Z, slot=slot_of(z1 - 0.01), uvm=1, reg=6, **attrs)


def slots_by_height(splits, names):
    """slot_of(z): names[i] between splits[i-1] and splits[i] (names has one more entry than splits)."""
    def f(z):
        for s, nm in zip(splits, names):
            if z < s:
                return BODY_INDEX[nm]
        return BODY_INDEX[names[-1]]
    f.splits = tuple(splits)
    return f


# Body slots in the spec's order (4.4): the masonry a plot uses, then its windows.
BODY_ORDER = ['ns_brick_red', 'ns_brick_buff', 'ns_brick_dark', 'ns_brick_painted', 'ns_limestone', 'ns_caststone', 'ns_stucco', 'ns_roof', 'ns_window']
BODY_INDEX = {n: i for i, n in enumerate(BODY_ORDER)}
TRIM_SLOTS = ['stone', 'metal', 'front', 'lit', 'print']


# ================================================================== materials

# The body's tile slots as exported: a constant near the set's mean times COLOR_0 (so the glTF carries COLOR_0; the
# runtime M_NS_Masonry and M_NS_WindowRoom replace them by slot name).
BODY_BASE = {'ns_brick_red': (0x7e3f2e, 0.85), 'ns_brick_buff': (0xa48a68, 0.85), 'ns_brick_dark': (0x30241f, 0.45),
             'ns_brick_painted': (0xb9b2a3, 0.7), 'ns_limestone': (0xbdb4a2, 0.8), 'ns_caststone': (0x8a6f5a, 0.8),
             'ns_stucco': (0xb8ae9c, 0.85), 'ns_roof': (0x4a4946, 0.9), 'ns_window': (0x0b0e12, 0.04)}
_BODY_MATS = {}


def body_material(name):
    if name in _BODY_MATS:
        return _BODY_MATS[name]
    h, r = BODY_BASE[name]
    m = core.Mat(name)
    va = m.node('ShaderNodeVertexColor')
    va.layer_name = 'Color'
    mx = m.nt.nodes.new('ShaderNodeMix')
    mx.data_type = 'RGBA'
    mx.blend_type = 'MULTIPLY'
    mx.inputs[0].default_value = 1.0
    mx.inputs[6].default_value = hx(h)
    m.link(va.outputs['Color'], mx.inputs[7])
    m.link(mx.outputs[2], m.bsdf.inputs['Base Color'])
    m.set('Roughness', r)
    m.m.use_backface_culling = True
    if m.m.name != name:
        raise RuntimeError(f'material name clash: {m.m.name}')
    _BODY_MATS[name] = m
    return m


def glass_material():
    """[glass] as exported: a clear, slightly dark glass (M_NS_Glass replaces it)."""
    m = bpy.data.materials.get('ns_glass')
    if m is not None:
        return m
    mt = core.Mat('ns_glass')
    mt.set('Base Color', hx(0x1a2024))
    mt.set('Roughness', 0.03)
    mt.set('Alpha', 0.25)
    mt.set('Specular IOR Level', 0.5)
    return mt.m


def screen_material():
    """[screen] as exported: a dark plain face (the game draws it, UV0 0..1)."""
    m = bpy.data.materials.get('ns_screen')
    if m is not None:
        return m
    mt = core.Mat('ns_screen')
    mt.set('Base Color', hx(0x101010))
    mt.set('Roughness', 0.3)
    return mt.m


def glow_material(name):
    m = core.Mat(name)
    m.set('Base Color', (1.0, 1.0, 1.0, 1.0))
    m.set('Emission Color', (1.0, 1.0, 1.0, 1.0))
    m.set('Emission Strength', 1.0)
    m.set('Roughness', 0.5)
    return m


# Trim kinds (the trim slots' ss_kind).
NK = {n: i + 1 for i, n in enumerate(['stone', 'sash', 'front', 'door', 'iron', 'galv', 'brass', 'alu', 'ac', 'tar', 'tile', 'gate', 'granite', 'wood',
                                      'black', 'brown', 'cornice', 'concrete', 'plywood', 'grate', 'casing', 'steel', 'marble', 'pot', 'leaf', 'enamel',
                                      'cedar', 'bronze', 'terracotta', 'glassblock', 'chrome', 'board', 'rubber', 'lens', 'copper', 'canvas', 'bulb',
                                      'limestone', 'caststone', 'paintsteel', 'hood', 'brick', 'gold', 'porcelain'])}


def s_bronze(m, c, sp):
    """Statuary bronze outdoors: dark brown-black, brighter where hands rub, verdigris weeping from the joints."""
    h = sp.get('hex', 0x3a2a1c)
    patina = ramp(m, fac.noise(m, c.P, 4.0, 5.0), 0.5, 0.8)
    green = m.math('MULTIPLY', m.math('MULTIPLY', ramp(m, c.streak, 0.5, 0.75), patina), sp.get('verdigris', 0.6))
    col = m.mix(m.math('MULTIPLY', c.mottle, 0.5), fac.hexc(h), fac.hexc(_shade(h, 0.7)))
    col = m.mix(m.math('MULTIPLY', c.edges, 0.7), col, fac.hexc(0x9a7a48))
    col = m.mix(green, col, fac.hexc(0x4a7a66))
    metal = mix_float(m, green, 0.9, 0.1)
    rough = mix_float(m, green, m.math('SUBTRACT', 0.42, m.math('MULTIPLY', c.edges, 0.2)), 0.8)
    return col, rough, metal, m.math('MULTIPLY', green, 0.3)


def s_enamel(m, c, sp):
    """Porcelain enamel or a fresh gloss paint: color holds, chips are rare and black."""
    h = sp['hex']
    chip = m.math('MULTIPLY', ramp(m, c.edges, 0.55, 0.7), sp.get('chips', 0.3))
    col = m.mix(m.math('MULTIPLY', c.mottle, 0.15), fac.hexc(h), fac.hexc(_shade(h, 0.9)))
    col = m.mix(chip, col, fac.hexc(0x151515))
    col = fac.weather(m, c, col, sp.get('soot', 0.25), 0.4, 0.25)
    return col, mix_float(m, chip, sp.get('rough', 0.22), 0.6), 0.0, 0.0


def _block_joints(m, c, bw, bh, joint=0.006, stagger=True):
    """0..1 on the mortar joints of blocks bw x bh (m) in the face's (u, v), running bond when stagger; also a per-block
    random 0..1 (for each block's own tone)."""
    row = m.math('FLOOR', m.math('DIVIDE', c.v, bh))
    us = m.math('DIVIDE', c.u, bw)
    if stagger:
        us = m.math('ADD', us, m.math('MULTIPLY', m.math('MODULO', row, 2.0), 0.5))
    fu = m.math('FRACT', us)
    fv = m.math('FRACT', m.math('DIVIDE', c.v, bh))
    du = m.math('MULTIPLY', m.math('MINIMUM', fu, m.math('SUBTRACT', 1.0, fu)), bw)
    dv = m.math('MULTIPLY', m.math('MINIMUM', fv, m.math('SUBTRACT', 1.0, fv)), bh)
    j = m.math('MAXIMUM', ramp(m, du, joint, joint * 0.4), ramp(m, dv, joint, joint * 0.4))
    cell = vec(m, m.math('FLOOR', us), row, 0.0)
    wn = m.node('ShaderNodeTexWhiteNoise')
    wn.noise_dimensions = '3D'
    m.link(cell, wn.inputs['Vector'])
    return j, wn.outputs['Value']


def s_terracotta(m, c, sp):
    """Glazed architectural terracotta: blocks (60 x 45 cm, raked joints), each its own tone of the glaze, a satin glaze
    crazed fine; it sheds the soot, so only faint streaks."""
    h = sp.get('hex', 0xcdb88e)
    craze = ramp(m, fac.noise(m, c.P, 60.0, 2.0, 0.6, 2.0), 0.62, 0.66)
    j, cell = _block_joints(m, c, 0.6, 0.45, 0.005)
    col = m.mix(m.math('MULTIPLY', cell, 0.5), fac.hexc(_shade(h, 1.04)), fac.hexc(_shade(h, 0.9)))
    col = m.mix(m.math('MULTIPLY', c.mottle, 0.25), col, fac.hexc(_shade(h, 0.85)))
    col = m.mix(m.math('MULTIPLY', craze, 0.2), col, fac.hexc(_shade(h, 0.6)))
    col = m.mix(j, col, fac.hexc(_shade(h, 0.55)))
    col = fac.weather(m, c, col, sp.get('soot', 0.22), 0.45, 0.3)
    rough = m.math('ADD', m.math('ADD', 0.28, m.math('MULTIPLY', craze, 0.15)), m.math('MULTIPLY', j, 0.5))
    return col, rough, 0.0, m.math('SUBTRACT', m.math('MULTIPLY', craze, -0.15), m.math('MULTIPLY', j, 0.8))


def glassblock_pattern(m, c):
    """Glass block (20 cm, 1 cm mortar): (joint 0..1, the block's glow 0..1: its fluting and pillowed middle)."""
    u = m.math('FRACT', m.math('DIVIDE', c.u, 0.2))
    v = m.math('FRACT', m.math('DIVIDE', c.v, 0.2))
    du = m.math('MINIMUM', u, m.math('SUBTRACT', 1.0, u))
    dv = m.math('MINIMUM', v, m.math('SUBTRACT', 1.0, v))
    edge = m.math('MAXIMUM', ramp(m, du, 0.045, 0.025), ramp(m, dv, 0.045, 0.025))
    pillow = ramp(m, m.math('MINIMUM', du, dv), 0.03, 0.22)
    flute = m.math('MULTIPLY', m.math('ADD', m.math('SINE', m.math('MULTIPLY', c.u, 150.0)), 1.0), 0.5)
    wave = fac.noise(m, scaled(m, c.sep, 22.0, 22.0, 22.0), 1.0, 2.0)
    glow = m.math('MULTIPLY', m.math('ADD', m.math('MULTIPLY', pillow, 0.55), m.math('ADD', m.math('MULTIPLY', flute, 0.2), m.math('MULTIPLY', wave, 0.35))),
                  m.math('SUBTRACT', 1.0, edge))
    return edge, glow


def s_glassblock(m, c, sp):
    edge, glow = glassblock_pattern(m, c)
    col = m.mix(edge, m.mix(glow, fac.hexc(0x6e7c78), fac.hexc(0xa9b6b0)), fac.hexc(0x8c8982))
    return col, mix_float(m, edge, 0.05, 0.8), 0.0, m.math('SUBTRACT', m.math('MULTIPLY', glow, 0.3), edge)


def s_chrome(m, c, sp):
    col = m.mix(m.math('MULTIPLY', c.mottle, 0.3), fac.hexc(0xd8dcdf), fac.hexc(0x9aa0a4))
    return col, m.math('ADD', 0.08, m.math('MULTIPLY', c.mottle, 0.1)), 1.0, 0.0


def s_rubber(m, c, sp):
    col = m.mix(m.math('MULTIPLY', c.mottle, 0.4), fac.hexc(0x121212), fac.hexc(0x2a2826))
    return col, 0.85, 0.0, 0.0


def s_lens(m, c, sp):
    return fac.hexc(sp.get('hex', 0xf0e8d8)), 0.15, 0.0, 0.0


def s_copper(m, c, sp):
    """Old copper flashing and leaders: brown with green streaks."""
    g = ramp(m, c.streak, 0.45, 0.7)
    col = m.mix(g, m.mix(c.mottle, fac.hexc(0x5a3a22), fac.hexc(0x7a4a2a)), fac.hexc(0x5a8a74))
    return col, mix_float(m, g, 0.45, 0.8), mix_float(m, g, 0.7, 0.0), 0.0


def s_brick(m, c, sp):
    """Baked brick (where a trim piece is brick: a ghost sign's field, a chimney): the fronts' brick recipe's look."""
    h = sp.get('hex', 0x7e3f2e)
    row = m.math('FLOOR', m.math('DIVIDE', c.v, COURSE))
    off = m.math('MULTIPLY', m.math('FLOORED_MODULO', row, 2.0), 0.1)
    fu = m.math('FRACT', m.math('DIVIDE', m.math('ADD', c.u, off), 0.2))
    fv = m.math('FRACT', m.math('DIVIDE', c.v, COURSE))
    joint = m.math('MAXIMUM', ramp(m, m.math('MINIMUM', fu, m.math('SUBTRACT', 1.0, fu)), 0.035, 0.02),
                   ramp(m, m.math('MINIMUM', fv, m.math('SUBTRACT', 1.0, fv)), 0.1, 0.06))
    rnd, _ = fac.white_noise(m, vec(m, m.math('FLOOR', m.math('DIVIDE', m.math('ADD', c.u, off), 0.2)), row, 1.0))
    col = m.mix(rnd, fac.hexc(_shade(h, 0.85)), fac.hexc(_shade(h, 1.12)))
    col = m.mix(joint, col, fac.hexc(0x8d877c))
    col = fac.weather(m, c, col, 0.5, 0.5, 0.3)
    return col, mix_float(m, joint, 0.82, 0.92), 0.0, m.math('MULTIPLY', joint, -1.0)


def s_gold(m, c, sp):
    col = m.mix(m.math('MULTIPLY', c.mottle, 0.3), fac.hexc(0xd8a84a), fac.hexc(0xb08030))
    return col, 0.25, 1.0, 0.0


SURF = dict(fac.SURF)
SURF.update({'bronze': s_bronze, 'enamel': s_enamel, 'terracotta': s_terracotta, 'glassblock': s_glassblock, 'chrome': s_chrome,
             'rubber': s_rubber, 'lens': s_lens, 'copper': s_copper, 'brick': s_brick, 'gold': s_gold})


def trim_material(name, palette, kinds):
    """One trim slot: each face's ss_kind picks its surface from palette ({kind: {'type': surface, ...}})."""
    m = core.Mat(name)
    c = fac.Ctx(m, edges=True)
    kind = fac.attr(m, 'ss_kind')
    col, rough, metal, height = hx(0x808080), 0.5, 0.0, 0.0
    for k in sorted(kinds, key=lambda k: NK[k]):
        sp = palette[k]
        kc, kr, km, kh = SURF[sp['type']](m, c, sp)
        mk = fac.is_k(m, kind, NK[k])
        col = m.mix(mk, col, kc)
        rough = mix_float(m, mk, rough, kr)
        metal = mix_float(m, mk, metal, km)
        height = mix_float(m, mk, height, kh)
    m.set('Base Color', col)
    m.set('Roughness', rough)
    m.set('Metallic', metal)
    fac.bump(m, height, 0.45, 0.003)
    return m


# Lit kinds (the lit slot's ss_kind): facades' set plus the shops' goods.
IK = dict(fac.IK)
for _k in ('goods', 'produce', 'bottle', 'cloth', 'terrazzo', 'chalk', 'gilt', 'velvet', 'paper', 'tvwall', 'shopwall', 'glassblock', 'polish'):
    IK[_k] = len(IK) + 1


def lit_material(name, strength=5.0):
    """Everything seen lit through shop glass and lobby doors: facades' lit recipe (each surface lit by its room's lamp,
    ss_lamp / ss_room) plus the goods: packages, fruit, bottles, garments, each face its own color by ss_rnd."""
    m = fac.lit_material(name, strength)
    c = fac.Ctx(m)
    kind = fac.attr(m, 'ss_kind')
    lamp = fac.attr(m, 'ss_lamp', 'Vector')
    look, seed, power = fac.split(m, fac.attr(m, 'ss_room', 'Vector'))
    rnd = fac.attr(m, 'ss_rnd')
    L = fac.vmath(m, 'SUBTRACT', lamp, c.P)
    d2 = fac.vmath(m, 'DOT_PRODUCT', L, L)
    ndl = fac.vmath(m, 'DOT_PRODUCT', c.N, fac.vmath(m, 'NORMALIZE', L))
    wrap = m.math('DIVIDE', m.math('ADD', ndl, 0.25), 1.25, clamp=True)
    fall = m.math('DIVIDE', power, m.math('ADD', 1.0, m.math('DIVIDE', d2, 0.8)))
    light = m.math('ADD', m.math('MULTIPLY', wrap, fall), m.math('MULTIPLY', power, 0.025))
    lampcol = fac.hexc(fac.LOOK_HEX[1])
    for k, h in fac.LOOK_HEX.items():
        lampcol = m.mix(fac.is_k(m, look, k), lampcol, fac.hexc(h))

    def lit(albedo, gain=1.0):
        return fac.cmul(m, fac.ctimes(m, albedo, lampcol), m.math('MULTIPLY', light, gain))

    def pick(colors, r):
        col = fac.hexc(colors[0])
        for i, h in enumerate(colors[1:]):
            col = m.mix(m.math('GREATER_THAN', r, (i + 1) / len(colors)), col, fac.hexc(h))
        return col
    # A package's face: its color, a label band, the print's speckle.
    pk = pick([0xd8342a, 0xf2c230, 0x2a5aa8, 0xf0ece0, 0x2a8a4a, 0xe86a2a, 0x7a2a8a, 0x1a1a1a, 0x4ab0d8, 0xc8a070], rnd)
    band = ramp(m, m.math('ABSOLUTE', m.math('SUBTRACT', m.math('FRACT', m.math('MULTIPLY', c.v, 9.0)), 0.5)), 0.2, 0.1)
    goods = m.mix(m.math('MULTIPLY', band, 0.6), pk, fac.hexc(0xf4f0e6))
    produce = pick([0xc8281e, 0xe8a020, 0x6aa02a, 0xe8d040, 0x7a1a2a, 0xe86a1a, 0x3a7a2a], rnd)
    produce = m.mix(m.math('MULTIPLY', fac.noise(m, c.P, 80.0, 2.0), 0.4), produce, fac.cmul(m, produce, 0.6))
    bottle = pick([0x2a4a1a, 0x5a2a0a, 0xc8c0a0, 0x1a2a1a, 0x8a5a1a, 0x3a1a1a], rnd)
    gl = m.math('POWER', m.math('MAXIMUM', ndl, 0.0), 12.0)
    bottle = m.mix(m.math('MULTIPLY', gl, 0.8), bottle, fac.hexc(0xffffff))
    # Nail polish: a salon's wall of reds, pinks, corals, nudes and darks, each bottle glossy; its black cap above.
    polish = pick([0xb0121e, 0xd8487a, 0xf08a8a, 0xe8b8a0, 0x7a1a3a, 0xc83a2a, 0xf2d0d8, 0x5a2a6a, 0xe8e2dc, 0x2a2a3a, 0xd86a3a, 0x8a1a2a], rnd)
    cap = m.math('GREATER_THAN', m.math('FRACT', c.v), 0.62)
    polish = m.mix(cap, polish, fac.hexc(0x141414))
    polish = m.mix(m.math('MULTIPLY', gl, 0.7), polish, fac.hexc(0xffffff))
    cloth = pick([0x1f2a3a, 0x8a2a22, 0xd8d0c0, 0x3a4a2a, 0x2a2a2a, 0x6a5a4a, 0x8a7a5a, 0x2a3a6a], rnd)
    terr = m.mix(ramp(m, fac.noise(m, c.P, 120.0, 1.0), 0.6, 0.65), fac.hexc(0xc9c2b4), fac.hexc(0x6a645a))
    chalk = m.mix(ramp(m, fac.noise(m, scaled(m, c.sep, 3.0, 3.0, 40.0), 1.0, 2.0), 0.55, 0.7), fac.hexc(0x1a1e1c), fac.hexc(0xc8c8c0))
    em = m.bsdf.inputs['Emission Color'].links[0].from_socket
    # Glass block lit from the room behind it: its own day color (base color: what shows when the glow is dimmed by
    # day), its blocks glowing with the room's light at night.
    gb_edge, gb_glow = glassblock_pattern(m, c)
    gb_day = m.mix(gb_edge, m.mix(gb_glow, fac.hexc(0x6e7c78), fac.hexc(0xa9b6b0)), fac.hexc(0x8c8982))
    gb_em = fac.cmul(m, lampcol, m.math('MULTIPLY', m.math('ADD', 0.12, m.math('MULTIPLY', gb_glow, 0.9)), m.math('MULTIPLY', power, 0.45)))
    bc = m.bsdf.inputs['Base Color']
    bc_src = bc.links[0].from_socket if bc.is_linked else fac.hexc(0x0e0d0c)
    m.link(m.mix(fac.is_k(m, kind, IK['glassblock']), bc_src, gb_day), bc)
    for k, col in (('glassblock', gb_em), ('goods', lit(goods, 1.1)), ('produce', lit(produce, 1.2)), ('bottle', lit(bottle, 1.2)), ('cloth', lit(cloth)),
                   ('terrazzo', lit(terr)), ('chalk', lit(chalk, 0.8)), ('gilt', lit(fac.hexc(0xd8b060), 1.6)),
                   ('velvet', lit(fac.hexc(0x5a1418))), ('paper', lit(fac.hexc(0xe8e2d0))), ('tvwall', fac.hexc(0x5a80d0)), ('polish', lit(polish, 1.25)),
                   ('shopwall', lit(m.mix(m.math('MULTIPLY', fac.noise(m, c.P, 1.5, 3.0), 0.6), fac.hexc(0xe2dccd), fac.hexc(0xc9c0ac))))):
        em = m.mix(fac.is_k(m, kind, IK[k]), em, col)
    m.link(em, m.bsdf.inputs['Emission Color'])
    return m


# ================================================================== a plot under construction

GLOW_FAMILIES = ('Warm', 'Cool', 'Neon')
LOOK = {'warm': 1, 'cool': 2, 'tv': 3, 'red': 5, 'shop': 6, 'hall': 7}
SF_W = 0.16        # a storefront's frame plane, back from the face
WIN_D = 0.30       # a window's pane, back from the face (the masonry reveal runs to 0.17, the frame covers the rest)


def render_sheet(sheet, name, px, samples=12):
    """A flat printed sheet rendered on the CPU (it's a few emissive shapes: the GPU is better left to the bakes, and
    a shared GPU makes each sheet wait), the scene's device put back after."""
    sc = bpy.context.scene
    dev = sc.cycles.device
    os.environ['SHORTSTACK_CPU'] = '1'
    try:
        return sheet.render(name, px, samples=samples)
    finally:
        os.environ.pop('SHORTSTACK_CPU', None)
        sc.cycles.device = dev


class Plot:
    """One building: its footprint (Blender meters, CCW from above) and the five meshes it becomes."""

    def __init__(self, pid, poly, streets, floors, gf=4.2, fh=3.15, frieze=0.75, parapet=1.0, wall='ns_brick_red', base=None,
                 base_top=None, lintel='ns_caststone', detail='full', seed=1, palette=None, neon=None, room_kind=0.0, ac=0.18,
                 body='B1', depth_back=None, heights=None):
        self.id = pid
        self.name = f'SM_NS_Bldg_{pid}'
        self.p = pid.lower()
        self.poly = [tuple(p) for p in poly]
        k = len(self.poly)
        self.faces = [Face(self.poly[i], self.poly[(i + 1) % k], 'street' if i in streets else 'party', key=f'{pid}{i}') for i in range(k)]
        self.streets = streets
        self.floors = floors
        self.gf, self.fh = gf, fh
        self.top = snap_z(gf + (floors - 1) * fh + frieze)       # the main wall's top (the cornice's crown at street side)
        self.parapet = parapet
        self.wall_m, self.base_m, self.base_top, self.lintel_m = wall, base, base_top, lintel
        self.detail = detail
        self.rnd = random.Random(seed)
        self.seed = seed
        self.palette = dict(DEFAULT_PALETTE)
        self.palette.update(palette or {})
        self.neon = neon
        self.room_kind = room_kind
        self.ac = ac
        self.bodytype = body
        self.heights = heights or {}
        self.body = MB()
        self.trim = MB()
        self.glass = MB()
        self.glow = {f: MB() for f in GLOW_FAMILIES}
        self.grille = MB()
        self.grille_origin = None
        self.signs = []
        self.used = {i: set() for i in range(5)}
        self.opens = {i: [] for i in range(k)}
        self.soot = []
        self.lit_strength = 4.0
        self.notes = []

    # Floors.
    def fz(self, k):
        """The floor of storey k (1 = the ground floor)."""
        return 0.0 if k <= 1 else self.gf + (k - 2) * self.fh

    def height_of(self, fi):
        return self.heights.get(fi, self.top)

    # Attribute dictionaries for trim faces.
    def T(self, slot, kind):
        assert kind in self.palette, f'{self.id}: no {kind} in the palette'
        self.used[slot].add(kind)
        return dict(slot=slot, ss_kind=NK[kind])

    def L(self, ikind, lamp, room, r=None):
        self.used[3].add(ikind)
        return dict(slot=3, ss_kind=IK[ikind], ss_lamp=tuple(lamp), ss_room=tuple(room), ss_rnd=self.rnd.random() if r is None else r)

    def body_slot(self, name):
        return BODY_INDEX[name]

    # Signs (the prints slot).
    def sign(self, label, w, h, draw, center, normal, depth=0.015, glow=0.0, backlit=False, px=None, board=True, edge='board'):
        """A printed face w x h (m) centered at center facing normal; draw(sheet, W, H) lays it out in millimeters. Its
        front is cut into pieces no wider than 2.4 m (each its own UV island, so a long band packs into the prints slot
        at a readable density); a board behind it (edge kind) unless board is False."""
        s = Sheet(w * 1000.0, h * 1000.0)
        draw(s, w * 1000.0, h * 1000.0)
        if px is None:
            px = int(min(4096, max(256, 380 * w, 240 * w / max(h, 0.01))))
        paths = render_sheet(s, f'nsf_{self.p}_{label}', px)
        sid = len(self.signs) + 1
        n = Vector(normal).normalized()
        t = fac.face_t(n)
        cv = Vector(center)
        ac = cv.x * t.x + cv.y * t.y
        self.signs.append(dict(id=sid, paths=paths, a0=ac - w / 2, a1=ac + w / 2, z0=cv.z - h / 2, z1=cv.z + h / 2, glow=glow, backlit=backlit,
                               c=tuple(cv), n=tuple(n), w=w, h=h))
        self.used[4].add('sign')
        tt = Vector((t.x, t.y, 0.0))
        p0 = cv - tt * (w / 2) - Z * (h / 2)
        k = max(1, int(math.ceil(w / 2.4)))
        for i in range(k):
            a, b = p0 + tt * (w * i / k), p0 + tt * (w * (i + 1) / k)
            self.trim.poly([a, b, b + Z * h, a + Z * h], want=n, slot=4, ss_sign=sid)
        if board:
            # The board behind the face: its edges and back in the storefront's paint.
            self.trim.obox(cv - n * (depth / 2 + 0.001), (tt, Z, n), (w / 2, h / 2, depth / 2), 0.0, ((2, 1),), None, **self.T(2, edge))
        return sid

    def note(self, s):
        self.notes.append(s)


DEFAULT_PALETTE = {
    'stone': dict(type='stone', hex=0xb2a68c, soot=0.75),
    'limestone': dict(type='stone', hex=0xbdb4a2, soot=0.8),
    'caststone': dict(type='stone', hex=0x8f735c, soot=0.7),
    'granite': dict(type='granite'),
    'marble': dict(type='marble'),
    'concrete': dict(type='concrete'),
    'sash': dict(type='paint', hex=0xd8d0bc, under=0x6a5a48, chips=0.85, soot=0.5),
    'casing': dict(type='paint', hex=0xcac2ae, under=0x6a5a48, chips=0.8, soot=0.5),
    'front': dict(type='paint', hex=0x1d3a2c, under=0x4a3a2c, chips=0.85, under_metal=0.3, soot=0.5),
    'door': dict(type='paint', hex=0x4e1c16, under=0x6b4a30, chips=0.7),
    'cornice': dict(type='paint', hex=0xb8b09c, under=0x5d6266, chips=0.75, soot=0.6, under_metal=0.6),
    'board': dict(type='paint', hex=0x1a1c1e, under=0x5a5a58, chips=0.6),
    'iron': dict(type='iron'),
    'grate': dict(type='grate'),
    'galv': dict(type='galv'),
    'hood': dict(type='galv'),
    'steel': dict(type='steel'),
    'alu': dict(type='alu'),
    'brass': dict(type='brass'),
    'bronze': dict(type='bronze'),
    'black': dict(type='black'),
    'ac': dict(type='plastic', hex=0xc6bfae),
    'tar': dict(type='tar'),
    'tile': dict(type='tile', hex=0x2c5a46, alt=0x214637, size=0.024),
    'wood': dict(type='wood', hex=0x4a2c18),
    'cedar': dict(type='wood', hex=0x5a4632),
    'plywood': dict(type='plywood'),
    'enamel': dict(type='enamel', hex=0xb02a24),
    'chrome': dict(type='chrome'),
    'rubber': dict(type='rubber'),
    'lens': dict(type='lens', hex=0xf0e8d8),
    'copper': dict(type='copper'),
    'brick': dict(type='brick', hex=0x7e3f2e),
    'gold': dict(type='gold'),
    'terracotta': dict(type='terracotta', hex=0xcdb88e),
    'glassblock': dict(type='glassblock'),
    'porcelain': dict(type='enamel', hex=0xe8e4da, chips=0.2),
    'paintsteel': dict(type='paint', hex=0x2a2e30, under=0x5a3a22, chips=0.7, under_metal=0.8),
    'gate': dict(type='gate'),
}


# ------------------------------------------------------------------ windows

def window_unit(P, F, uc, z0, w, h, look='apt', panes=(1, 1), raise_=0.0, ac=False, lintel='key', sill='stone', guard=False, boarded=False,
                kind=0.0, steel=False):
    """One window in F at u = uc, sill z0, w x h: the opening and its masonry reveals (body), the stone sill and lintel
    (body), the frame and two sashes (trim), the pane the room is seen through (body, ns_window), maybe an AC unit or a
    guard. Steel: a multi-light steel sash (warehouses), panes = (cols, rows) of lights."""
    u0, u1, z1 = uc - w / 2, uc + w / 2, z0 + h
    P.opens[P.faces.index(F)].append(dict(u0=u0, u1=u1, z0=z0, z1=z1, depth=WIN_D - 0.12 if not steel else WIN_D - 0.02))
    tb = P.trim
    full = P.detail == 'full'
    # The pane (UV 0..1 over the opening, (0, 0) at its bottom left seen from outside).
    hsh = P.rnd.random()
    gfrac = min(1.0, (z0 + h / 2) / max(P.top, 1.0))
    if boarded:
        F.box(tb, u0 + 0.01, u1 - 0.01, z0 + 0.01, z1 - 0.01, WIN_D - 0.13, WIN_D - 0.11, **P.T(2, 'plywood'))
    else:
        F.quad(P.body, u0, u1, z0, z1, WIN_D, uv=[(0, 0), (1, 0), (1, 1), (0, 1)], col=(hsh, gfrac, kind, 1.0), slot=BODY_INDEX['ns_window'], reg=7,
               pane=(u1 - u0, z1 - z0, 0.0))
    fr = 0.06
    wf0, wf1 = WIN_D - 0.13, WIN_D - 0.005
    if steel:
        # A steel sash: a frame and a grid of T-bars, one hopper light pushed open.
        fk = P.T(1, 'paintsteel')
        F.box(tb, u0, u0 + 0.05, z0, z1, wf0, wf1, **fk)
        F.box(tb, u1 - 0.05, u1, z0, z1, wf0, wf1, **fk)
        F.box(tb, u0, u1, z0, z0 + 0.06, wf0, wf1, **fk)
        F.box(tb, u0, u1, z1 - 0.05, z1, wf0, wf1, **fk)
        cols, rows = panes
        for i in range(1, cols):
            uu = u0 + (u1 - u0) * i / cols
            F.box(tb, uu - 0.012, uu + 0.012, z0 + 0.06, z1 - 0.05, wf1 - 0.05, wf1, **fk)
        for j in range(1, rows):
            zz = z0 + 0.06 + (h - 0.11) * j / rows
            F.box(tb, u0 + 0.05, u1 - 0.05, zz - 0.012, zz + 0.012, wf1 - 0.05, wf1, **fk)
    elif getattr(P, 'is_proxy', False):
        sk = P.T(2, 'sash')
        F.box(tb, u0, u0 + fr, z0, z1, wf0, wf1, **sk)
        F.box(tb, u1 - fr, u1, z0, z1, wf0, wf1, **sk)
        F.box(tb, u0 + fr, u1 - fr, z1 - 0.07, z1, wf0, wf1, **sk)
        F.box(tb, u0 + fr, u1 - fr, z0, z0 + 0.07, wf0 - 0.03, wf1, **sk)
        zm = z0 + h * 0.5
        F.box(tb, u0 + fr, u1 - fr, zm - 0.035, zm + 0.035, wf0 + 0.04, wf1 - 0.02, **sk)
    else:
        sk = P.T(2, 'sash')
        # Sashes and frames are square-edged (their 3-4 mm arrises are below a texel at the distances windows are seen
        # from; the bake's wear still finds the edges).
        F.box(tb, u0, u0 + fr, z0, z1, wf0, wf1, **sk)
        F.box(tb, u1 - fr, u1, z0, z1, wf0, wf1, **sk)
        F.box(tb, u0 + fr, u1 - fr, z1 - 0.07, z1, wf0, wf1, **sk)
        F.prism(tb, [(wf0 - 0.035, z0), (wf1, z0), (wf1, z0 + 0.045), (wf0 - 0.035, z0 + 0.018)], u0 + fr, u1 - fr, **sk)
        ya, yb, za, zb = u0 + fr, u1 - fr, z0 + 0.045, z1 - 0.07
        zm = (za + zb) / 2 + 0.03
        if ac:
            raise_ = 0.42
        raise_ = min(raise_, zb - zm - 0.06)
        cols, rows = panes if full else (1, 1)

        def sash(wa, wb, sz0, sz1, top, bot):
            F.box(tb, ya, ya + 0.045, sz0, sz1, wa, wb, **sk)
            F.box(tb, yb - 0.045, yb, sz0, sz1, wa, wb, **sk)
            F.box(tb, ya + 0.045, yb - 0.045, sz1 - top, sz1, wa, wb, **sk)
            F.box(tb, ya + 0.045, yb - 0.045, sz0, sz0 + bot, wa, wb, **sk)
            py0, py1, pz0, pz1 = ya + 0.045, yb - 0.045, sz0 + bot, sz1 - top
            for i in range(1, cols):
                yy = py0 + (py1 - py0) * i / cols
                F.box(tb, yy - 0.011, yy + 0.011, pz0, pz1, wa + 0.008, wb - 0.008, **sk)
            for j in range(1, rows):
                zz = pz0 + (pz1 - pz0) * j / rows
                F.box(tb, py0, py1, zz - 0.011, zz + 0.011, wa + 0.008, wb - 0.008, **sk)
        sash(wf0 + 0.02, wf0 + 0.06, zm - 0.02, zb, 0.05, 0.035)
        sash(wf0 + 0.07, wf0 + 0.11, za + raise_, zm + 0.02 + raise_, 0.035, 0.075)
        if ac and full:
            aw = min(0.66, w - 0.2)
            ak = P.T(1, 'ac')
            F.box(tb, uc - aw / 2, uc + aw / 2, za + 0.01, za + 0.4, -0.36, wf0 + 0.1, 0.012, seg=2, **ak)
            F.box(tb, uc - aw / 2 + 0.04, uc + aw / 2 - 0.04, za + 0.05, za + 0.35, -0.365, -0.355, **ak)
            for s in (-1, 1):
                F.box(tb, (ya if s < 0 else uc + aw / 2), (uc - aw / 2 if s < 0 else yb), za, za + 0.38, wf0 + 0.07, wf0 + 0.1, **P.T(1, 'ac'))
                a = F.P(uc + s * (aw / 2 - 0.05), z0 - 0.08, -0.3)
                b = F.P(uc + s * (aw / 2 - 0.05), z0 - 0.42, -0.02)
                tb.beam(a, b, 0.02, 0.02, **P.T(1, 'iron'))
            # Its drip stains the wall under it.
            P.soot.append((F, uc - 0.2, uc + 0.2, z0 - 0.1, 1.8, 0.8))
    # The stone sill and lintel (body masonry), the keystone (trim).
    lm = BODY_INDEX[P.lintel_m]
    if sill == 'stone':
        F.prism(P.body, [(-0.065, z0 - 0.1), (0.0, z0 - 0.1), (0.0, z0), (-0.065, z0 - 0.025), (-0.075, z0 - 0.03), (-0.075, z0 - 0.09)],
                u0 - 0.1, u1 + 0.1, slot=lm, uvm=1, reg=8)
    if lintel in ('flat', 'key'):
        F.box(P.body, u0 - 0.12, u1 + 0.12, z1, z1 + 0.24, -0.025, 0.0, 0.008, seg=1, skip=((2, 1),), slot=lm, uvm=1, reg=8)
        if lintel == 'key' and full:
            F.prism(tb, [(-0.05, z1 - 0.03), (0.0, z1 - 0.03), (0.0, z1 + 0.3), (-0.05, z1 + 0.3)], uc - 0.075, uc + 0.075, **P.T(0, 'stone'))
            F.prism(tb, [(-0.045, z1 + 0.3), (0.0, z1 + 0.3), (0.0, z1 + 0.32), (-0.045, z1 + 0.32)], uc - 0.095, uc + 0.095, **P.T(0, 'stone'))
    elif lintel == 'hood':
        F.box(P.body, u0 - 0.12, u1 + 0.12, z1, z1 + 0.2, -0.03, 0.0, 0.008, seg=1, skip=((2, 1),), slot=lm, uvm=1, reg=8)
        if full:
            hood = [(0.0, 0.0), (0.06, 0.0), (0.06, 0.02), (0.09, 0.05), (0.09, 0.1), (0.13, 0.12), (0.16, 0.17), (0.17, 0.2), (0.17, 0.24), (0.0, 0.24)]
            pts = [F.P(u0 - 0.22, 0, -0.03), F.P(u1 + 0.22, 0, -0.03)]
            P.trim.molding([(pts[0].x, pts[0].y), (pts[1].x, pts[1].y)], hood, z1 + 0.2, **P.T(0, 'stone'))
    elif lintel == 'soldier':
        # A soldier course of brick over the head: in the wall's own brick, the masonry shader shows it as a band.
        pass
    if guard:
        n = max(3, int(w / 0.11))
        gk = P.T(1, 'iron')
        for i in range(n + 1):
            uu = u0 + 0.03 + (w - 0.06) * i / n
            F.box(tb, uu - 0.007, uu + 0.007, z0 + 0.02, z1 - 0.02, -0.075, -0.06, **gk)
        for zz in (z0 + 0.06, (z0 + z1) / 2, z1 - 0.06):
            F.box(tb, u0 + 0.01, u1 - 0.01, zz - 0.02, zz + 0.02, -0.08, -0.055, **gk)
    # Soot runs down from the sill.
    P.soot.append((F, u0 - 0.05, u1 + 0.05, z0 - 0.1, 2.4, 0.75))


# ------------------------------------------------------------------ storefronts, doors, lobbies

def display_room(P, F, ua, ub, w0, w1, z0, z1, lamp_look='shop', power=1.2, floor='floor', wall='shopwall', lamp_at=None, back=True):
    """An open box of lit surfaces behind glass (w0 at the glass side, open there): floor, ceiling, back, sides."""
    lamp = lamp_at or F.P((ua + ub) / 2, z1 - 0.35, (w0 + w1) / 2)
    rm = (LOOK[lamp_look], P.rnd.random(), power)
    tb = P.trim
    F.quad(tb, ua, ub, z0, z1, w1, facing=-1, **P.L(wall, lamp, rm)) if back else None
    tb.poly([F.P(ua, z0, w0), F.P(ub, z0, w0), F.P(ub, z0, w1), F.P(ua, z0, w1)], want=Z, **P.L(floor, lamp, rm))
    tb.poly([F.P(ua, z1, w0), F.P(ub, z1, w0), F.P(ub, z1, w1), F.P(ua, z1, w1)], want=-Z, **P.L('ceil', lamp, rm))
    tb.poly([F.P(ua, z0, w0), F.P(ua, z1, w0), F.P(ua, z1, w1), F.P(ua, z0, w1)], want=F.t, **P.L(wall, lamp, rm))
    tb.poly([F.P(ub, z0, w0), F.P(ub, z1, w0), F.P(ub, z1, w1), F.P(ub, z0, w1)], want=-F.t, **P.L(wall, lamp, rm))
    return lamp, rm


def lbox(P, F, u0, u1, z0, z1, w0, w1, ikind, lamp, rm, cham=0.0, r=None):
    F.box(P.trim, u0, u1, z0, z1, w0, w1, cham, seg=1, **P.L(ikind, lamp, rm, r))


def storefront(P, F, u0, u1, sh, door='center', recess=1.0, door_w=1.0, bulk='tile', frame='front', dress=None, lamp_look='shop',
               transom=True, grille=True, double=False, deep=1.9, bulk_h=0.5, mullion=1.8, lit=True, door_kind='door'):
    """A shop opening from u0 to u1, head sh: the masonry reveal (body), a bulkhead, display glass in a frame with a
    transom, a recessed entry with its door, the lit display behind the glass (dressed by dress), the grille's guides and
    hood. Glass goes to _Glass; the closed grille to _Grille (the plot's first one)."""
    fi = P.faces.index(F)
    P.opens[fi].append(dict(u0=u0, u1=u1, z0=0.0, z1=sh, depth=SF_W, sill=False))
    tb = P.trim
    fk = P.T(2, frame)
    full = P.detail == 'full'
    dw = door_w * (2 if double else 1) + 0.12
    if door == 'center':
        d0 = (u0 + u1) / 2 - dw / 2
    elif door == 'left':
        d0 = u0 + 0.35
    elif door == 'right':
        d0 = u1 - 0.35 - dw
    else:
        d0 = None
    d1 = d0 + dw if d0 is not None else None
    th = sh - 0.62 if transom else sh - 0.08    # transom bar's underside
    runs = [(u0, d0), (d1, u1)] if d0 is not None else [(u0, u1)]
    # Bulkheads and display glass on each run, glazed returns into the recess.
    for a, b in runs:
        if b - a < 0.15:
            continue
        if bulk == 'tile':
            F.box(tb, a, b, 0.0, bulk_h, SF_W, SF_W + 0.18, **P.T(2, 'tile'))
            F.box(tb, a, b, bulk_h - 0.04, bulk_h + 0.06, SF_W - 0.04, SF_W + 0.2, 0.006, **fk)
        else:
            bk = P.T(2, bulk)
            F.box(tb, a, b, 0.0, bulk_h + 0.06, SF_W, SF_W + 0.18, 0.006, **bk)
            if full and b - a > 0.6:
                n = max(1, int((b - a) / 0.9))
                for i in range(n):
                    pa = a + 0.08 + (b - a - 0.16) * i / n
                    pb = a + 0.08 + (b - a - 0.16) * (i + 1) / n - 0.08
                    F.box(tb, pa, pb, 0.1, bulk_h - 0.06, SF_W - 0.015, SF_W, 0.006, **bk)
            F.box(tb, a, b, bulk_h + 0.02, bulk_h + 0.08, SF_W - 0.04, SF_W + 0.2, 0.006, **fk)
        n = max(1, int(math.ceil((b - a) / mullion)))
        for i in range(n + 1):
            uu = a + (b - a) * i / n
            uu = min(max(uu, a + 0.04), b - 0.04)
            F.box(tb, uu - 0.04, uu + 0.04, bulk_h + 0.06, sh, SF_W - 0.02, SF_W + 0.1, 0.005, **fk)
        for i in range(n):
            ga = a + (b - a) * i / n + 0.04
            gb = a + (b - a) * (i + 1) / n - 0.04
            F.quad(P.glass, ga, gb, bulk_h + 0.08, th, SF_W + 0.04, slot=0)
        if transom:
            F.box(tb, a, b, th, th + 0.08, SF_W - 0.03, SF_W + 0.1, 0.005, **fk)
            F.quad(P.glass, a + 0.04, b - 0.04, th + 0.08, sh - 0.06, SF_W + 0.04, slot=0)
        # The display behind this run.
        if lit:
            lamp, rm = display_room(P, F, a + 0.02, b - 0.02, SF_W + 0.1, SF_W + 0.1 + deep, 0.0, th + (0.5 if transom else 0.0), lamp_look,
                                    floor='floor', wall='shopwall')
            if dress:
                dress(P, F, a + 0.04, b - 0.04, SF_W + 0.12, SF_W + 0.1 + deep, bulk_h + 0.06, th, lamp, rm)
        else:
            F.quad(tb, a, b, bulk_h + 0.08, sh, SF_W + 0.3, **P.L('wall', F.P((a + b) / 2, sh - 0.5, 1.0), (LOOK['shop'], 0.3, 0.3)))
    F.box(tb, u0, u1, sh - 0.08, sh, SF_W - 0.04, SF_W + 0.12, 0.006, **fk)
    # The recessed entry: its glazed returns, the floor, the soffit, the door and its transom.
    if d0 is not None:
        rw = SF_W + recess
        F.box(tb, d0, d1, -0.005, 0.015, SF_W, rw + 0.05, **P.T(2, 'tile'))
        for uu, s in ((d0, 1), (d1, -1)):
            F.box(tb, uu - 0.04, uu + 0.04, 0.0, sh, SF_W - 0.02, rw, 0.005, **fk)
            F.box(tb, uu + s * 0.0 - 0.03, uu + 0.03, 0.0, bulk_h, SF_W, rw, **P.T(2, 'tile' if bulk == 'tile' else bulk))
            pts = [F.P(uu, bulk_h + 0.06, SF_W + 0.08), F.P(uu, bulk_h + 0.06, rw - 0.06), F.P(uu, th, rw - 0.06), F.P(uu, th, SF_W + 0.08)]
            P.glass.poly(pts, want=F.t * s, slot=0)
        F.box(tb, d0, d1, th, th + 0.08, SF_W, rw, **fk)
        F.box(tb, d0, d1, sh - 0.08, sh, SF_W, rw, **fk)
        # The soffit over the recess (under the transom's level) and the door.
        tb.poly([F.P(d0, th, SF_W), F.P(d1, th, SF_W), F.P(d1, th, rw), F.P(d0, th, rw)], want=-Z, **fk)
        nl = 2 if double else 1
        lw = (dw - 0.12) / nl
        for i in range(nl):
            la = d0 + 0.06 + i * lw
            lb = la + lw
            dk = P.T(2, door_kind)
            F.box(tb, la, la + 0.11, 0.02, th - 0.02, rw - 0.05, rw, 0.004, **dk)
            F.box(tb, lb - 0.11, lb, 0.02, th - 0.02, rw - 0.05, rw, 0.004, **dk)
            F.box(tb, la + 0.11, lb - 0.11, 0.02, 0.3, rw - 0.05, rw, 0.004, **dk)
            F.box(tb, la + 0.11, lb - 0.11, th - 0.14, th - 0.02, rw - 0.05, rw, 0.004, **dk)
            F.quad(P.glass, la + 0.11, lb - 0.11, 0.3, th - 0.14, rw - 0.025, slot=0)
            hx_ = lb - 0.2 if (i == 0 and nl == 2) or (nl == 1 and door != 'right') else la + 0.2
            tb.tube(F.P(hx_, 0.95, rw - 0.05), F.P(hx_, 0.95, rw - 0.12), 0.012, n=8, **P.T(2, 'brass'))
            F.box(tb, hx_ - 0.03, hx_ + 0.03, 0.9, 1.2, rw - 0.06, rw - 0.05, **P.T(2, 'brass'))
            F.box(tb, la + 0.11, lb - 0.11, 0.35, 0.43, rw - 0.07, rw - 0.05, 0.003, **P.T(2, 'brass'))
        F.box(tb, d0, d1, th - 0.02, th + 0.02, rw - 0.06, rw + 0.02, **fk)
        if transom:
            F.quad(P.glass, d0 + 0.04, d1 - 0.04, th + 0.08, sh - 0.08, SF_W + 0.04, slot=0)
        # The shop inside, past the door.
        lamp = F.P((d0 + d1) / 2, sh - 0.3, rw + 2.5)
        rm = (LOOK[lamp_look], P.rnd.random(), 1.0)
        F.quad(tb, d0 - 0.6, d1 + 0.6, 0.0, sh, rw + 4.0, **P.L('shopwall', lamp, rm))
        tb.poly([F.P(d0 - 0.6, 0.0, rw), F.P(d1 + 0.6, 0.0, rw), F.P(d1 + 0.6, 0.0, rw + 4.0), F.P(d0 - 0.6, 0.0, rw + 4.0)], want=Z, **P.L('floor', lamp, rm))
        tb.poly([F.P(d0 - 0.6, sh, rw), F.P(d1 + 0.6, sh, rw), F.P(d1 + 0.6, sh, rw + 4.0), F.P(d0 - 0.6, sh, rw + 4.0)], want=-Z, **P.L('ceil', lamp, rm))
        for uu, s in ((d0 - 0.6, 1), (d1 + 0.6, -1)):
            tb.poly([F.P(uu, 0.0, rw), F.P(uu, sh, rw), F.P(uu, sh, rw + 4.0), F.P(uu, 0.0, rw + 4.0)], want=F.t * s, **P.L('shopwall', lamp, rm))
        if dress:
            dress(P, F, d0 - 0.5, d1 + 0.5, rw + 1.0, rw + 3.9, 0.0, sh - 0.2, lamp, rm)
    # Grille guides and hood (trim); the grille itself (its own mesh), closed.
    if grille:
        gk = P.T(1, 'hood')
        for uu in (u0, u1):
            F.box(tb, uu - 0.04, uu + 0.04, 0.0, sh, -0.07, 0.0, 0.004, **gk)
        F.box(tb, u0 - 0.04, u1 + 0.04, sh, sh + 0.32, -0.32, 0.0, 0.012, seg=2, **gk)
        if P.grille_origin is None:
            grille_curtain(P, F, u0 + 0.02, u1 - 0.02, sh)
    return d0, d1


def grille_curtain(P, F, u0, u1, sh):
    """The roll-down grille, closed: interlocking slats in its guides, a bottom bar with the lock. Its own mesh, its
    origin at the top middle of its opening (the game scales it in Z to roll it up)."""
    g = P.grille
    w = -0.035
    z = 0.0
    pitch = 0.076
    while z < sh - 0.01:
        za, zb = z, min(z + pitch, sh)
        # A curved slat: three facets.
        pts = [(w, za), (w - 0.012, za + (zb - za) * 0.3), (w - 0.012, za + (zb - za) * 0.7), (w, zb)]
        for (wa, zza), (wb, zzb) in zip(pts, pts[1:]):
            g.poly([F.P(u0, zza, wa), F.P(u1, zza, wa), F.P(u1, zzb, wb), F.P(u0, zzb, wb)], want=F.n, slot=0)
        z += pitch
    F.box(g, u0, u1, 0.0, 0.07, w - 0.03, w + 0.01, 0.008, slot=0)
    uc = (u0 + u1) / 2
    for s in (-0.35, 0.35):
        F.box(g, uc + s - 0.05, uc + s + 0.05, 0.015, 0.06, w - 0.05, w - 0.03, slot=0)
    P.grille_origin = F.P(uc, sh, 0.0)
    P.grille_face = F


def res_door(P, F, uc, width=1.35, height=2.95, recess=0.55, step=True, surround='lintel', number=None, lamp=True, buzzer=True,
             door_kind='door', lobby=False, canopy=False):
    """A residents' door: the opening (body reveal), a stone step, the door and its lit transom with the number in gilt,
    the vestibule lit behind, a light over it, the buzzer panel on the jamb."""
    fi = P.faces.index(F)
    u0, u1 = uc - width / 2, uc + width / 2
    P.opens[fi].append(dict(u0=u0, u1=u1, z0=0.0, z1=height, depth=recess, sill=False))
    tb = P.trim
    if step:
        F.box(P.body, u0 - 0.15, u1 + 0.15, 0.0, 0.17, -0.36, recess, 0.01, seg=1, slot=BODY_INDEX['ns_caststone'], uvm=1, reg=9)
    z0 = 0.17 if step else 0.0
    dk = P.T(2, door_kind)
    tz = height - 0.62
    F.box(tb, u0, u0 + 0.09, z0, height, recess - 0.02, recess + 0.06, 0.004, **dk)
    F.box(tb, u1 - 0.09, u1, z0, height, recess - 0.02, recess + 0.06, 0.004, **dk)
    F.box(tb, u0, u1, tz, tz + 0.09, recess - 0.02, recess + 0.06, 0.004, **dk)
    F.box(tb, u0, u1, height - 0.06, height, recess - 0.02, recess + 0.06, 0.004, **dk)
    # The leaf: stiles and rails round its glass, a kick panel, the handle.
    la, lb = u0 + 0.09, u1 - 0.09
    lw = recess + 0.03
    F.box(tb, la, la + 0.12, z0, tz, lw - 0.02, lw + 0.03, 0.004, **dk)
    F.box(tb, lb - 0.12, lb, z0, tz, lw - 0.02, lw + 0.03, 0.004, **dk)
    F.box(tb, la + 0.12, lb - 0.12, z0, z0 + 0.75, lw - 0.02, lw + 0.03, 0.004, **dk)
    F.box(tb, la + 0.12, lb - 0.12, tz - 0.16, tz, lw - 0.02, lw + 0.03, 0.004, **dk)
    if P.detail == 'full':
        F.box(tb, la + 0.18, lb - 0.18, z0 + 0.12, z0 + 0.63, lw - 0.035, lw - 0.02, 0.006, **dk)
    F.quad(P.glass, la + 0.12, lb - 0.12, z0 + 0.75, tz - 0.16, lw, slot=0)
    F.box(tb, lb - 0.22, lb - 0.16, z0 + 0.9, z0 + 1.15, lw - 0.03, lw - 0.02, **P.T(2, 'brass'))
    tb.tube(F.P(lb - 0.19, z0 + 1.0, lw - 0.03), F.P(lb - 0.19, z0 + 1.0, lw - 0.1), 0.012, n=8, **P.T(2, 'brass'))
    F.box(tb, la + 0.12, lb - 0.12, z0 + 0.02, z0 + 0.22, lw - 0.03, lw - 0.02, **P.T(2, 'brass'))
    # The transom: glass with the number in gilt on its inside face.
    F.quad(P.glass, u0 + 0.09, u1 - 0.09, tz + 0.09, height - 0.06, recess + 0.02, slot=0)
    if number:
        def draw(s, Wm, Hm):
            fac.outlined(s, number, Wm / 2, Hm * 0.18, Hm * 0.62, 0xd8b060, 0x3a2408, face='Bold', metal=1.0, rough=0.25)
        P.sign(f'num{uc:.0f}', min(0.9, width - 0.3), 0.32, draw, F.P(uc, (tz + 0.09 + height - 0.06) / 2, recess + 0.025), F.n, depth=0.002,
               glow=0.6, backlit=True, px=512, board=False, )
    # The vestibule.
    hl = F.P(uc, height - 0.4, recess + 1.6)
    rm = (LOOK['hall'], P.rnd.random(), 1.1 if not lobby else 1.5)
    hw = max(width / 2 + 0.4, 1.2)
    vd = 3.2 if not lobby else 5.5
    F.quad(tb, uc - hw, uc + hw, 0.0, height + 0.3, recess + vd, **P.L('wall', hl, rm))
    tb.poly([F.P(uc - hw, z0, recess + 0.05), F.P(uc + hw, z0, recess + 0.05), F.P(uc + hw, z0, recess + vd), F.P(uc - hw, z0, recess + vd)], want=Z,
            **P.L('hex' if not lobby else 'terrazzo', hl, rm))
    tb.poly([F.P(uc - hw, height + 0.3, recess + 0.05), F.P(uc + hw, height + 0.3, recess + 0.05), F.P(uc + hw, height + 0.3, recess + vd),
             F.P(uc - hw, height + 0.3, recess + vd)], want=-Z, **P.L('ceil', hl, rm))
    for uu, s in ((uc - hw, 1), (uc + hw, -1)):
        tb.poly([F.P(uu, 0.0, recess + 0.05), F.P(uu, height + 0.3, recess + 0.05), F.P(uu, height + 0.3, recess + vd), F.P(uu, 0.0, recess + vd)],
                want=F.t * s, **P.L('wall' if not lobby else 'marble', hl, rm))
    # Mailboxes on one side, a stair or the elevator at the back.
    lbox(P, F, uc - hw, uc - hw + 0.04, 1.0, 1.7, recess + 0.6, recess + 1.4, 'mailbox', hl, rm)
    if lobby:
        lbox(P, F, uc - 0.55, uc + 0.55, z0, z0 + 2.2, recess + vd - 0.05, recess + vd - 0.02, 'chrome', hl, rm)
        lbox(P, F, uc - 0.01, uc + 0.01, z0, z0 + 2.2, recess + vd - 0.06, recess + vd - 0.05, 'furn', hl, rm)
        lbox(P, F, uc - 0.2, uc + 0.2, z0 + 2.35, z0 + 2.45, recess + vd - 0.06, recess + vd - 0.05, 'gilt', hl, rm)
        P.trim.lathe([(0.0, -0.2), (0.25, -0.1), (0.28, 0.0), (0.0, 0.0)], tuple(F.P(uc, height + 0.3, recess + 2.2)), 16, **P.L('fixture', hl, rm))
    else:
        for i in range(7):
            lbox(P, F, uc + 0.1, uc + hw, z0, z0 + 0.19 * (i + 1), recess + 1.6 + i * 0.25, recess + vd, 'stair', hl, rm)
        P.trim.lathe([(0.0, -0.12), (0.12, -0.06), (0.14, 0.0), (0.0, 0.0)], tuple(F.P(uc, height + 0.3, recess + 1.0)), 12, **P.L('fixture', hl, rm))
    if lamp:
        # A wall light over the door: a cast box with a frosted lens.
        F.box(tb, uc - 0.09, uc + 0.09, height + 0.25, height + 0.5, -0.12, 0.0, 0.01, seg=2, **P.T(1, 'black'))
        F.box(P.glow['Warm'], uc - 0.07, uc + 0.07, height + 0.27, height + 0.48, -0.125, -0.115, slot=0)
    if buzzer:
        def draw(s, Wm, Hm):
            s.rect(0, 0, Wm, Hm, 0x9ea3a8, rough=0.35, metal=1.0)
            s.rect(5, 5, Wm - 10, Hm - 10, 0xb7bcc1, rough=0.3, metal=1.0)
            for i in range(10):
                s.rect(16, Hm - 30 - i * 5, Wm - 32, 2, 0x2a2c2e, rough=0.5)
            r = random.Random(P.seed + int(uc * 10))
            names = ['RIVERA', 'OSEI', 'NOWAK', 'LIU', 'PETROV', 'HASSAN', 'COHEN', 'DIAZ', 'KOVAC', 'PARK', 'SINGH', 'BRENNAN', 'OKAFOR', 'MORENO']
            r.shuffle(names)
            for i in range(min(P.floors * 2, 10)):
                y = Hm - 110 - i * 20
                s.rect(10, y - 3, Wm - 44, 15, 0xf0ece2, rough=0.6)
                s.text(f'{i // 2 + 1}{"AB"[i % 2]} {names[i]}', 13, y, 10, 0x1a1a1a, face='Bold', rough=0.6)
                s.circle(Wm - 20, y + 4, 6, 0x3a3020, rough=0.3, metal=1.0)
        jamb = F.P(u1 + 0.18, 1.45, -0.012)
        P.sign(f'buzz{uc:.0f}', 0.15, 0.34, draw, jamb, F.n, depth=0.012, px=256)
    if canopy:
        cz = height + 0.35
        ck = P.T(1, 'bronze' if canopy == 'bronze' else 'paintsteel')
        F.box(tb, u0 - 0.6, u1 + 0.6, cz, cz + 0.28, -2.6, 0.0, 0.01, seg=2, **ck)
        for s in (-1, 1):
            a = F.P(uc + s * (width / 2 + 0.3), cz + 0.25, -2.45)
            b = F.P(uc + s * (width / 2 + 0.3), cz + 1.4, -0.02)
            tb.beam(a, b, 0.03, 0.03, **P.T(1, 'paintsteel'))
        for k in range(3):
            pp = F.P(uc - 0.5 + k * 0.5, cz - 0.005, -1.4)
            P.glow['Warm'].lathe([(0.0, 0.0), (0.06, 0.0), (0.0, 0.005)], tuple(pp), 12, slot=0)
    return u0, u1


# ------------------------------------------------------------------ ironwork, cornices, roofs

def fire_escape(P, F, ua, ub, zs, deep=1.15, ladder=True):
    """An iron fire escape across ua..ub of F: grated platforms at each z in zs, railings of square bar, stairs between,
    brackets back to the wall, the drop ladder hooked up under the lowest; rust weeping down the brick beneath."""
    tb = P.trim
    ik, gk = P.T(1, 'iron'), P.T(1, 'grate')
    for k, z in enumerate(zs):
        F.box(tb, ua + 0.05, ub - 0.05, z - 0.03, z - 0.006, -deep + 0.05, -0.03, **gk)
        F.box(tb, ua, ub, z - 0.11, z, -deep, -deep + 0.05, **ik)
        F.box(tb, ua, ua + 0.05, z - 0.11, z, -deep, -0.03, **ik)
        F.box(tb, ub - 0.05, ub, z - 0.11, z, -deep, -0.03, **ik)
        for uu in (ua + 0.08, (ua + ub) / 2, ub - 0.08):
            tb.beam(F.P(uu, z - 0.08, -deep + 0.08), F.P(uu, z - 0.62, -0.02), 0.03, 0.03, **ik)
        rz = z + 0.95
        for zz, hh in ((rz, 0.04), (z + 0.48, 0.03)):
            F.box(tb, ua, ub, zz - hh / 2, zz + hh / 2, -deep, -deep + 0.03, **ik)
            F.box(tb, ua, ua + 0.03, zz - hh / 2, zz + hh / 2, -deep, -0.03, **ik)
            F.box(tb, ub - 0.03, ub, zz - hh / 2, zz + hh / 2, -deep, -0.03, **ik)
        n = int((ub - ua) / (0.115 if not getattr(P, 'is_proxy', False) else 0.45))
        for j in range(n + 1):
            uu = ua + 0.015 + (ub - ua - 0.03) * j / n
            F.box(tb, uu - 0.007, uu + 0.007, z, rz, -deep + 0.008, -deep + 0.022, **ik)
        for j in (range(1, 9) if not getattr(P, 'is_proxy', False) else (4,)):
            ww = -deep + (deep - 0.05) * j / 9
            for uu in (ua + 0.015, ub - 0.015):
                F.box(tb, uu - 0.007, uu + 0.007, z, rz, ww - 0.007, ww + 0.007, **ik)
        if k + 1 < len(zs):
            z2 = zs[k + 1]
            s0, s1 = (ua + 0.3, ub - 1.25) if k % 2 == 0 else (ub - 0.3, ua + 1.25)
            ws = -deep + 0.36
            for sd in (-0.27, 0.27):
                tb.beam(F.P(s0, z, ws + sd), F.P(s1, z2, ws + sd), 0.04, 0.04, **ik)
            steps = 11 if not getattr(P, 'is_proxy', False) else 4
            for j in range(1, steps):
                t = j / steps
                uu, zz = s0 + (s1 - s0) * t, z + (z2 - z) * t
                F.box(tb, uu - 0.085, uu + 0.085, zz - 0.02, zz, ws - 0.27, ws + 0.27, **gk)
            tb.beam(F.P(s0, z + 0.88, ws - 0.3), F.P(s1, z2 + 0.88, ws - 0.3), 0.025, 0.025, **ik)
        P.soot.append((F, ua, ub, z - 0.1, 2.6, 0.55))
    if P.detail == 'full' and not getattr(P, 'is_proxy', False) and len(zs) >= 2 and P.rnd.random() < 0.6:
        k = P.rnd.randrange(1, len(zs))
        uu = ua + 0.45 if P.rnd.random() < 0.5 else ub - 0.45
        out = Vector((F.n.x, F.n.y, 0.0))
        tt = Vector((F.t.x, F.t.y, 0.0))
        yaw = P.rnd.uniform(-0.6, 0.6)
        el = P.rnd.uniform(0.5, 0.7)
        facing = (out * math.cos(yaw) + tt * math.sin(yaw)) * math.cos(el) + Z * math.sin(el)
        sat_dish(P, F.P(uu, zs[k] + 0.78, -deep - 0.02), facing, r=P.rnd.choice((0.23, 0.28, 0.3)))
    if ladder:
        z1 = zs[0]
        lu = ub - 0.62
        # The drop ladder hangs 2.5 m below the lowest platform, or (P.ladder_floor) is hooked up clear of a sign band
        # and storefront under it.
        zb = z1 - 2.5
        if getattr(P, 'ladder_floor', None) is not None:
            zb = max(zb, P.ladder_floor)
        for sd in (-0.21, 0.21):
            F.box(tb, lu + sd - 0.012, lu + sd + 0.012, zb, z1 + 0.9, -deep + 0.1, -deep + 0.16, **ik)
        for j in range(int((z1 + 0.75 - zb) / 0.28) + 1):
            zz = zb + 0.15 + j * 0.28
            tb.tube(F.P(lu - 0.21, zz, -deep + 0.13), F.P(lu + 0.21, zz, -deep + 0.13), 0.01, n=6, **ik)


def console_profile(depth, height, scroll=True):
    return fac.console_profile(depth, height, scroll)


def cornice(P, path, z, height, project, slot=1, kind='cornice', modillions=True, dentils=True, consoles=True):
    """A cornice (pressed metal, slot 1, or stone, slot 0) along path [(x, y)] (outward on the right of travel) with its
    underside at z: frieze fascia, bed molding, dentils, the soffit on modillion brackets, the corona and a cyma crown,
    mitered at the corners; big scrolled consoles at its ends."""
    H, Pj = height, project
    k = Pj / 0.7
    prof = [(0.0, 0.0), (0.05, 0.0), (0.05, 0.08 * H), (0.08, 0.1 * H), (0.08, 0.36 * H), (0.12 * k, 0.38 * H), (0.14 * k, 0.43 * H),
            (0.2 * k, 0.46 * H), (0.22 * k, 0.5 * H), (0.24 * k, 0.56 * H), (0.8 * Pj, 0.58 * H), (0.84 * Pj, 0.62 * H),
            (0.84 * Pj, 0.74 * H), (0.88 * Pj, 0.76 * H), (0.93 * Pj, 0.81 * H), (0.98 * Pj, 0.88 * H), (Pj, 0.92 * H), (Pj, H), (0.0, H)]
    at = P.T(slot, kind)
    P.trim.molding(path, prof, z, **at)
    segs = list(zip(path, path[1:]))
    full = P.detail == 'full'
    for i, (a, b) in enumerate(segs):
        F = Face(a, b)
        L = F.L
        lo = 0.06 if i == 0 else 0.2 * k + 0.08
        hi = L - (0.06 if i == len(segs) - 1 else 0.2 * k + 0.08)
        if dentils and full:
            uu = lo
            while uu < hi:
                F.box(P.trim, uu - 0.025, uu + 0.025, z + 0.38 * H, z + 0.46 * H, -0.2 * k, -0.05, **at)
                uu += 0.1
        if modillions:
            lo_m = 0.35 if i == 0 else 0.8 * Pj + 0.1
            hi_m = L - (0.3 if i == len(segs) - 1 else 0.8 * Pj + 0.1)
            span = hi_m - lo_m
            n = int(span / 0.62)
            spots = [lo_m + span * j / n for j in range(n + 1)] if n >= 1 else ([(lo_m + hi_m) / 2] if span > -0.2 and L > 0.5 else [])
            for uu in spots:
                prof_m = [(x - 0.24 * k, zz + z + 0.58 * H) for x, zz in console_profile(0.8 * Pj - 0.24 * k - 0.02, 0.12 * H, scroll=False)]
                F.prism(P.trim, prof_m, uu - 0.05, uu + 0.05, **at)
        if consoles:
            ch = 0.95 * H
            ends = ([0.11] if i == 0 else []) + ([L - 0.11] if i == len(segs) - 1 else [])
            for uu in ends:
                prof_c = [(x - 0.06, zz + z + 0.58 * H) for x, zz in console_profile(0.8 * Pj - 0.06, ch)]
                F.prism(P.trim, [(w_, z_) for w_, z_ in prof_c], uu - 0.09, uu + 0.09, **at)


def downspout(P, F, uu, z_top, w_off=-0.09, kind='galv'):
    """A leader from a gutter head under the cornice down to the sidewalk, strapped every couple of meters."""
    tb = P.trim
    gk = P.T(1, kind)
    F.box(tb, uu - 0.14, uu + 0.14, z_top - 0.3, z_top, w_off - 0.13, w_off + 0.06, 0.01, seg=2, **gk)
    tb.tube(F.P(uu, z_top - 0.3, w_off), F.P(uu, 0.15, w_off), 0.045, n=12, **gk)
    tb.tube(F.P(uu, 0.15, w_off), F.P(uu, 0.03, w_off - 0.16), 0.045, n=12, **gk)
    zz = 0.6
    while zz < z_top - 0.6:
        F.box(tb, uu - 0.055, uu + 0.055, zz, zz + 0.03, w_off - 0.05, 0.0, **gk)
        zz += 2.1
    P.soot.append((F, uu - 0.25, uu + 0.25, z_top, z_top, 0.5))


def roof(P, z_roof, z_par, coping='ns_caststone', bulkhead=True, tank=False, extras=True):
    """The roof inside the parapets (ns_roof), the parapets' inner faces and their coping (body), a stair bulkhead, a
    water tank on its steel stand, vents."""
    inner = offset_poly(P.poly, 0.32)
    P.body.poly([Vector((x, y, z_roof)) for x, y in inner], want=Z, slot=BODY_INDEX['ns_roof'], uvm=1, reg=5)
    k = len(inner)
    wm = BODY_INDEX[P.wall_m]
    for i in range(k):
        a, b = inner[i], inner[(i + 1) % k]
        F = Face(a, b)
        # The inner face of the parapet faces into the roof (away from F's outward).
        P.body.poly([Vector((a[0], a[1], z_roof)), Vector((b[0], b[1], z_roof)), Vector((b[0], b[1], z_par)), Vector((a[0], a[1], z_par))],
                    want=-F.n, slot=wm, uvm=1, reg=6)
    # The coping: a cap over the parapet, its edge a little proud both sides.
    cm = BODY_INDEX[coping]
    outer = offset_poly(P.poly, -0.03)
    inn = offset_poly(P.poly, 0.35)
    for i in range(k):
        a0, a1 = outer[i], outer[(i + 1) % k]
        b0, b1 = inn[i], inn[(i + 1) % k]
        P.body.poly([Vector((a0[0], a0[1], z_par + 0.08)), Vector((a1[0], a1[1], z_par + 0.08)), Vector((b1[0], b1[1], z_par + 0.08)),
                     Vector((b0[0], b0[1], z_par + 0.08))], want=Z, slot=cm, uvm=1, reg=6)
        P.body.poly([Vector((a0[0], a0[1], z_par - 0.02)), Vector((a1[0], a1[1], z_par - 0.02)), Vector((a1[0], a1[1], z_par + 0.08)),
                     Vector((a0[0], a0[1], z_par + 0.08))], want=Face(a0, a1).n, slot=cm, uvm=1, reg=6)
        P.body.poly([Vector((b0[0], b0[1], z_par - 0.02)), Vector((b1[0], b1[1], z_par - 0.02)), Vector((b1[0], b1[1], z_par + 0.08)),
                     Vector((b0[0], b0[1], z_par + 0.08))], want=-Face(b0, b1).n, slot=cm, uvm=1, reg=6)
        P.body.poly([Vector((a0[0], a0[1], z_par - 0.02)), Vector((a1[0], a1[1], z_par - 0.02)), Vector((b1[0], b1[1], z_par - 0.02)),
                     Vector((b0[0], b0[1], z_par - 0.02))], want=-Z, slot=cm, uvm=1, reg=6)
    xs = [p[0] for p in inner]
    ys = [p[1] for p in inner]
    cx, cy = sum(xs) / k, sum(ys) / k
    rnd = P.rnd
    if bulkhead:
        # Toward the back of the roof (away from the street faces).
        sf = P.faces[P.streets[0]] if P.streets else P.faces[0]
        bx, by = cx + sf.n.x * -2.0, cy + sf.n.y * -2.0
        P.body.box(bx - 1.4, bx + 1.4, by - 1.4, by + 1.4, z_roof, z_roof + 2.7, slot=wm, uvm=1, reg=0)
        P.body.box(bx - 1.5, bx + 1.5, by - 1.5, by + 1.5, z_roof + 2.7, z_roof + 2.82, slot=BODY_INDEX['ns_roof'], uvm=1, reg=5)
        P.trim.box(bx - 0.45, bx + 0.45, by - 1.42, by - 1.38, z_roof, z_roof + 2.1, **P.T(1, 'paintsteel'))
    if tank:
        tx, ty = cx + (rnd.random() - 0.5) * 3.0, cy + (rnd.random() - 0.5) * 3.0
        tz = z_roof + 3.0
        sk = P.T(1, 'paintsteel')
        for dx in (-1.1, 1.1):
            for dy in (-1.1, 1.1):
                P.trim.box(tx + dx - 0.08, tx + dx + 0.08, ty + dy - 0.08, ty + dy + 0.08, z_roof, tz, **sk)
        P.trim.box(tx - 1.5, tx + 1.5, ty - 1.5, ty + 1.5, tz, tz + 0.18, **sk)
        P.trim.lathe([(0.0, 0.0), (1.35, 0.0), (1.38, 0.1), (1.42, 3.4), (1.45, 3.45), (1.3, 3.5), (0.15, 4.3), (0.08, 4.5), (0.0, 4.5)],
                     (tx, ty, tz + 0.18), 32, **P.T(1, 'cedar'))
        for hz in (0.4, 1.2, 2.0, 2.8):
            P.trim.lathe([(1.37 + hz * 0.01, 0.0), (1.4 + hz * 0.01, 0.0), (1.4 + hz * 0.01, 0.05), (1.37 + hz * 0.01, 0.05)],
                         (tx, ty, tz + 0.18 + hz), 32, **P.T(1, 'iron'))
    if extras:
        for i in range(rnd.randint(2, 4)):
            vx = cx + (rnd.random() - 0.5) * (max(xs) - min(xs) - 2.0)
            vy = cy + (rnd.random() - 0.5) * (max(ys) - min(ys) - 2.0)
            P.trim.lathe([(0.0, 0.0), (0.09, 0.0), (0.09, 0.6), (0.18, 0.62), (0.16, 0.72), (0.0, 0.74)], (vx, vy, z_roof), 12, **P.T(1, 'galv'))


# ------------------------------------------------------------------ neon and lettering

def text_outline_loops(text, size, face='Bold', tracking=1.0):
    """The outlines of a word set in Roboto, as closed 2D loops (meters, baseline at y 0, centered on x)."""
    cu = bpy.data.curves.new('neon_txt', 'FONT')
    cu.body = text
    cu.font = bpy.data.fonts.load(os.path.join(core.FONTS, f'Roboto-{face}.ttf'), check_existing=True)
    cu.size = size
    cu.align_x = 'CENTER'
    cu.space_character = tracking
    cu.resolution_u = 4
    o = core.link(bpy.data.objects.new('neon_txt', cu))
    core.select_only([o])
    bpy.ops.object.convert(target='MESH')
    o = bpy.context.view_layer.objects.active
    bm = bmesh.new()
    bm.from_mesh(o.data)
    edges = [e for e in bm.edges if e.is_boundary]
    loops = []
    seen = set()
    for e0 in edges:
        if e0.index in seen:
            continue
        loop = []
        e = e0
        v = e.verts[0]
        while True:
            seen.add(e.index)
            loop.append(v.co.copy())
            v = e.other_vert(v)
            nxt = [x for x in v.link_edges if x.is_boundary and x.index not in seen]
            if not nxt:
                break
            e = nxt[0]
        if len(loop) > 2:
            loops.append([(p.x, p.y) for p in loop])
    bm.free()
    bpy.data.objects.remove(o, do_unlink=True)
    return loops


def neon(P, F, text, uc, zc, size, family='Neon', face='Bold', tracking=1.05, tube=0.008, w=-0.08, backer=True):
    """Neon lettering in front of F: tubes along the letters' outlines (the glow mesh), a raceway behind them (trim)."""
    loops = text_outline_loops(text, size, face, tracking)
    xs = [x for lp in loops for x, y in lp]
    ys = [y for lp in loops for x, y in lp]
    if not xs:
        return
    cx, cy = (min(xs) + max(xs)) / 2, (min(ys) + max(ys)) / 2
    g = P.glow[family]
    for lp in loops:
        pts = [F.P(uc + (x - cx), zc + (y - cy), w) for x, y in lp]
        # Simplify: drop points closer than half the tube's diameter.
        keep = [pts[0]]
        for p in pts[1:]:
            if (p - keep[-1]).length > tube * 1.5:
                keep.append(p)
        if len(keep) > 3:
            g.path_tube(keep, tube, n=6, closed=True, slot=0)
    if backer:
        wd, ht = max(xs) - min(xs), max(ys) - min(ys)
        F.box(P.trim, uc - wd / 2 - 0.1, uc + wd / 2 + 0.1, zc - 0.06, zc + 0.06, w + 0.02, 0.0, 0.006, **P.T(1, 'black'))
        for s in (-1, 1):
            F.box(P.trim, uc + s * wd * 0.35 - 0.015, uc + s * wd * 0.35 + 0.015, zc - ht / 2, zc + ht / 2, w + 0.025, 0.0, **P.T(1, 'black'))


def gooseneck(P, F, uc, z, reach=0.55):
    """A gooseneck sign lamp: a curved arm out from the wall, an enamel shade, its lamp."""
    tb = P.trim
    pts = [F.P(uc, z, 0.0), F.P(uc, z + 0.12, -0.15), F.P(uc, z + 0.18, -0.35), F.P(uc, z + 0.15, -reach + 0.05), F.P(uc, z + 0.05, -reach)]
    tb.path_tube(pts, 0.014, n=8, **P.T(1, 'paintsteel'))
    at = tuple(F.P(uc, z + 0.05, -reach))
    tb.lathe([(0.0, 0.0), (0.03, 0.0), (0.16, -0.12), (0.17, -0.14)], at, 20, **P.T(1, 'paintsteel'))
    tb.lathe([(0.165, -0.138), (0.155, -0.12), (0.028, -0.005), (0.0, -0.005)], at, 20, **P.T(1, 'porcelain'))
    P.glow['Warm'].lathe([(0.0, 0.0), (0.12, 0.0), (0.0, 0.004)], tuple(F.P(uc, z - 0.085, -reach)), 16, a0=0.0, slot=0)


# ================================================================== the shops' displays (the lit slot)

def bottle(P, F, uc, z, wc, r, h, ikind, lamp, rm, kind='wine'):
    """A bottle standing at (uc, z, wc): wine (round shoulders, a long neck), spirits (a squat neck) or a beer longneck."""
    prof = {'wine': [(0.0, 0.0), (r, 0.0), (r, 0.62 * h), (0.75 * r, 0.7 * h), (0.36 * r, 0.76 * h), (0.3 * r, h), (0.0, h)],
            'spirit': [(0.0, 0.0), (r, 0.0), (r, 0.72 * h), (0.85 * r, 0.78 * h), (0.42 * r, 0.82 * h), (0.42 * r, h), (0.0, h)],
            'beer': [(0.0, 0.0), (r, 0.0), (r, 0.55 * h), (0.5 * r, 0.75 * h), (0.38 * r, h), (0.0, h)]}[kind]
    P.trim.lathe(prof, tuple(F.P(uc, z, wc)), 8, **P.L(ikind, lamp, rm))


def shelves(P, F, ua, ub, wb, z0, z1, rows, depth, lamp, rm, goods='goods', board='furn', fill=0.85, item=(0.06, 0.16), face_out=True,
            shape='box', item_h=None):
    """A run of shelving against the back of a display (its back at wb), rows of goods on it: boxes, or bottles
    (shape 'bottle': lathes of wine, spirits and longnecks); item_h an absolute height range instead of the row's."""
    rnd = P.rnd
    lbox(P, F, ua, ub, z0, z1, wb - 0.02, wb, board, lamp, rm)
    for s in (ua, ub - 0.03):
        lbox(P, F, s, s + 0.03, z0, z1, wb - depth, wb, board, lamp, rm)
    for i in range(rows):
        z = z0 + 0.05 + (z1 - z0 - 0.1) * i / rows
        zt = z0 + 0.05 + (z1 - z0 - 0.1) * (i + 1) / rows
        lbox(P, F, ua, ub, z - 0.025, z, wb - depth, wb, board, lamp, rm)
        uu = ua + 0.04
        while uu < ub - 0.08:
            wd = rnd.uniform(*item)
            if rnd.random() < fill:
                room = zt - z - 0.04
                hh = min(room, rnd.uniform(*item_h)) if item_h else min(room, rnd.uniform(0.6, 1.0) * room)
                if shape == 'bottle':
                    kind = rnd.choice(('wine', 'wine', 'spirit', 'spirit', 'beer'))
                    hh = min(room, {'wine': 0.31, 'spirit': 0.27, 'beer': 0.23}[kind] * rnd.uniform(0.92, 1.05))
                    bottle(P, F, uu + wd / 2, z, wb - depth * 0.55, wd * 0.46, hh, goods, lamp, rm, kind)
                else:
                    lbox(P, F, uu, min(uu + wd, ub - 0.04), z, z + hh, wb - depth + rnd.uniform(0.0, 0.05), wb - 0.03, goods, lamp, rm)
            uu += wd + 0.01


def dress_produce(P, F, ua, ub, w0, w1, z0, z1, lamp, rm):
    """Kim's: stepped crates of fruit behind the glass, buckets of flowers, a scale, the counter beyond."""
    rnd = P.rnd
    for step in range(3):
        wa = w0 + 0.1 + step * 0.45
        za = z0 - 0.15 + step * 0.32
        lbox(P, F, ua, ub, za - 0.3, za, wa, wa + 0.45, 'oak', lamp, rm)
        uu = ua + 0.05
        while uu < ub - 0.3:
            cw = rnd.uniform(0.35, 0.55)
            lbox(P, F, uu, uu + cw, za, za + 0.12, wa + 0.03, wa + 0.42, 'oak', lamp, rm)
            r = rnd.random()
            # A heaped crate: the mound, and the front row of fruit on it.
            lbox(P, F, uu + 0.03, uu + cw - 0.03, za + 0.08, za + 0.17, wa + 0.06, wa + 0.4, 'produce', lamp, rm, cham=0.04, r=r)
            for j in range(int(cw / 0.1)):
                cu = uu + 0.06 + j * 0.1
                P.trim.obox(F.P(cu, za + 0.17, wa + 0.1 + (j % 2) * 0.06), F.axes(), (0.045, 0.04, 0.045), 0.03, seg=1, **P.L('produce', lamp, rm, r))
            uu += cw + 0.03
    shelves(P, F, ua + 0.1, ub - 0.1, w1, z0 + 0.5, z1 + 0.2, 3, 0.3, lamp, rm, goods='goods', board='oak', fill=0.7, item=(0.15, 0.3))


def dress_pharmacy(P, F, ua, ub, w0, w1, z0, z1, lamp, rm):
    shelves(P, F, ua + 0.1, ub - 0.1, w1, z0 - 0.4, z1 - 0.1, 6, 0.35, lamp, rm, board='counter', fill=0.95)
    lbox(P, F, ua + 0.3, ua + 1.1, z0, z0 + 1.0, w0 + 0.2, w0 + 0.4, 'goods', lamp, rm)
    # A hanging promo card (white with a red band, generic).
    lbox(P, F, (ua + ub) / 2 - 0.4, (ua + ub) / 2 + 0.4, z1 - 0.7, z1 - 0.15, w0 + 0.15, w0 + 0.16, 'paper', lamp, rm)
    lbox(P, F, (ua + ub) / 2 - 0.4, (ua + ub) / 2 + 0.4, z1 - 0.32, z1 - 0.2, w0 + 0.145, w0 + 0.15, 'vinyl', lamp, rm)


def dress_slice(P, F, ua, ub, w0, w1, z0, z1, lamp, rm):
    """Gino's: a counter along the glass with a heated case of pies on it, a menu board behind, the oven's glow."""
    lbox(P, F, ua, ub, z0 - 0.5, z0 + 0.45, w0 + 0.3, w0 + 0.95, 'counter', lamp, rm)
    lbox(P, F, ua + 0.2, ub - 0.2, z0 + 0.45, z0 + 0.5, w0 + 0.35, w0 + 0.9, 'chrome', lamp, rm)
    rnd = P.rnd
    uu = ua + 0.25
    while uu < ub - 0.6:
        P.trim.lathe([(0.0, 0.0), (0.22, 0.0), (0.23, 0.03), (0.0, 0.035)], tuple(F.P(uu + 0.25, z0 + 0.5, w0 + 0.62)), 20,
                     **P.L('produce', lamp, rm, r=rnd.choice((0.05, 0.2, 0.55))))
        uu += 0.55
    lbox(P, F, ua + 0.2, ub - 0.2, z1 - 0.9, z1 - 0.2, w1 - 0.03, w1 - 0.01, 'chalk', lamp, rm)
    lbox(P, F, ub - 1.2, ub - 0.1, z0 - 0.5, z0 + 0.9, w1 - 0.6, w1 - 0.02, 'chrome', lamp, rm)
    lbox(P, F, ub - 1.1, ub - 0.2, z0 + 0.2, z0 + 0.5, w1 - 0.61, w1 - 0.6, 'glow', lamp, rm)


def dress_checkcash(P, F, ua, ub, w0, w1, z0, z1, lamp, rm):
    """QuikCash: bright and bare: posters on the glass's back, a teller wall with windows beyond."""
    for i in range(int((ub - ua) / 0.8)):
        uu = ua + 0.1 + i * 0.8
        lbox(P, F, uu, uu + 0.6, z0 + 0.6, z0 + 1.5, w0 + 0.05, w0 + 0.06, 'poster', lamp, rm)
    lbox(P, F, ua, ub, z0 - 0.5, z0 + 0.6, w1 - 0.5, w1, 'cabinet', lamp, rm)
    lbox(P, F, ua, ub, z0 + 0.6, z1, w1 - 0.06, w1 - 0.02, 'mirror', lamp, rm)


def mannequin(P, F, uc, wc, z0, lamp, rm, r, turn=0.0):
    """A headless men's form on a chrome stand, dressed: a tailored jacket (a torso of elliptical section, its shoulders
    sloped, a collar, the sleeves hanging a little forward), trousers on two tapered legs, a wooden neck cap. Turned
    by turn (rad) about its stand. Lit slot (the display)."""
    tb = P.trim
    side = Vector((F.t.x, F.t.y, 0.0))
    out = Vector((F.n.x, F.n.y, 0.0))
    side, out = side * math.cos(turn) + out * math.sin(turn), out * math.cos(turn) - side * math.sin(turn)
    ell = Matrix((side, out * 0.66, Z)).transposed()
    c = F.P(uc, z0, wc)
    jacket = P.L('cloth', lamp, rm, r)
    trousers = P.L('cloth', lamp, rm, (r + 0.37) % 1.0)
    tb.lathe([(0.0, 0.0), (0.17, 0.0), (0.17, 0.018), (0.12, 0.03), (0.016, 0.04), (0.016, 0.8), (0.0, 0.8)], tuple(c), 20, **P.L('chrome', lamp, rm))
    # The jacket: hip, waist, chest, the shoulder's slope, the collar.
    tb.lathe([(0.0, 0.76), (0.165, 0.76), (0.172, 0.8), (0.158, 0.92), (0.178, 1.04), (0.198, 1.16), (0.205, 1.26), (0.19, 1.33), (0.13, 1.38),
              (0.075, 1.41), (0.068, 1.43), (0.0, 1.43)], tuple(c), 18, axis=ell, **jacket)
    # A lapel line and buttons: a thin darker strip down the front.
    fr = c + out * 0.66 * 0.168
    for k in range(3):
        bz = z0 + 0.88 + k * 0.1
        tb.lathe([(0.0, 0.0), (0.011, 0.0), (0.011, 0.006), (0.0, 0.008)], tuple(fr + Z * bz + out * 0.004), 8,
                 axis=Matrix((side, Z, out)).transposed(), **P.L('furn', lamp, rm))
    # Sleeves hanging from the shoulders, a little forward and in, to the cuffs.
    for s in (-1, 1):
        sh = c + side * (s * 0.2) + Z * 1.28
        el = c + side * (s * 0.225) + out * 0.03 + Z * 1.02
        cu = c + side * (s * 0.215) + out * 0.07 + Z * 0.8
        tb.path_tube([sh, el, cu], 0.052, n=10, **jacket)
        tb.lathe([(0.0, 0.0), (0.034, 0.0), (0.03, 0.05), (0.0, 0.06)], tuple(cu - Z * 0.055 + out * 0.01), 8, **P.L('counter', lamp, rm))
    # Trousers: two legs from the hip to the cuff, a little apart.
    for s in (-1, 1):
        hip = c + side * (s * 0.085)
        tb.lathe([(0.0, 0.05), (0.06, 0.05), (0.064, 0.1), (0.07, 0.42), (0.083, 0.62), (0.092, 0.78), (0.0, 0.8)], tuple(hip), 12,
                 axis=Matrix((side, out * 0.9, Z)).transposed(), **trousers)
        F.box(tb, uc + s * 0.085 - 0.045, uc + s * 0.085 + 0.045, z0, z0 + 0.06, wc - 0.13, wc + 0.06, 0.02, seg=1, **P.L('furn', lamp, rm))
    # The neck and its wooden cap.
    tb.lathe([(0.0, 1.41), (0.055, 1.41), (0.05, 1.5), (0.058, 1.52), (0.04, 1.555), (0.0, 1.56)], tuple(c), 14, **P.L('counter', lamp, rm))


def dress_clothier(P, F, ua, ub, w0, w1, z0, z1, lamp, rm):
    """Halden & Sons: two dressed mannequins on a platform, a folded stack, velvet drape behind."""
    lbox(P, F, ua, ub, z0, z1 + 0.3, w1 - 0.04, w1 - 0.02, 'velvet', lamp, rm)
    n = max(1, int((ub - ua) / 1.2))
    for i in range(n):
        uc = ua + (ub - ua) * (i + 0.5) / n
        wc = (w0 + w1) / 2
        mannequin(P, F, uc, wc, z0, lamp, rm, P.rnd.random(), turn=P.rnd.uniform(-0.35, 0.35))
    lbox(P, F, ua + 0.1, ua + 0.6, z0, z0 + 0.25, w0 + 0.1, w0 + 0.45, 'cloth', lamp, rm)


def dress_nails(P, F, ua, ub, w0, w1, z0, z1, lamp, rm):
    """Lily Nails: the polish wall (rows of small glossy bottles on narrow shelves), manicure tables each with its
    lamp and a client's chair, a plant by the glass."""
    shelves(P, F, ua + 0.2, ub - 0.2, w1, z0 + 0.1, z1 - 0.1, 11, 0.09, lamp, rm, goods='polish', board='counter', item=(0.028, 0.034), fill=0.97,
            item_h=(0.065, 0.085))
    tb = P.trim
    for i in range(int((ub - ua) / 1.4)):
        uu = ua + 0.3 + i * 1.4
        lbox(P, F, uu, uu + 0.9, z0 + 0.2, z0 + 0.25, w0 + 0.6, w0 + 1.1, 'counter', lamp, rm)
        lbox(P, F, uu + 0.05, uu + 0.85, z0 - 0.5, z0 + 0.2, w0 + 0.95, w0 + 1.08, 'counter', lamp, rm)
        # The table lamp: a weighted base, an arm, a shade over the hands.
        tb.lathe([(0.0, 0.0), (0.07, 0.0), (0.07, 0.025), (0.0, 0.03)], tuple(F.P(uu + 0.75, z0 + 0.25, w0 + 1.0)), 12, **P.L('chrome', lamp, rm))
        tb.path_tube([F.P(uu + 0.75, z0 + 0.27, w0 + 1.0), F.P(uu + 0.72, z0 + 0.62, w0 + 0.98), F.P(uu + 0.6, z0 + 0.66, w0 + 0.85)], 0.008, n=6,
                     **P.L('chrome', lamp, rm))
        tb.lathe([(0.0, 0.07), (0.03, 0.07), (0.075, 0.0), (0.0, 0.0)], tuple(F.P(uu + 0.6, z0 + 0.57, w0 + 0.85)), 12, **P.L('glow', lamp, rm))
        # Polish on the table, a towel, the client's chair on the glass side.
        for k in range(5):
            lbox(P, F, uu + 0.1 + k * 0.05, uu + 0.13 + k * 0.05, z0 + 0.25, z0 + 0.32, w0 + 0.66, w0 + 0.69, 'polish', lamp, rm)
        lbox(P, F, uu + 0.35, uu + 0.6, z0 + 0.25, z0 + 0.27, w0 + 0.65, w0 + 0.85, 'paper', lamp, rm)
        lbox(P, F, uu + 0.2, uu + 0.7, z0 - 0.08, z0 + 0.0, w0 + 0.1, w0 + 0.55, 'vinyl', lamp, rm, cham=0.03)
        lbox(P, F, uu + 0.2, uu + 0.7, z0 + 0.0, z0 + 0.5, w0 + 0.08, w0 + 0.16, 'vinyl', lamp, rm, cham=0.03)
        tb.lathe([(0.0, -0.5), (0.025, -0.5), (0.025, -0.08), (0.0, -0.08)], tuple(F.P(uu + 0.45, z0, w0 + 0.32)), 8, **P.L('chrome', lamp, rm))
    lbox(P, F, ua + 0.05, ua + 0.35, z0 - 0.5, z0 + 0.05, w0 + 0.05, w0 + 0.35, 'counter', lamp, rm)
    lbox(P, F, ua + 0.08, ua + 0.32, z0 + 0.05, z0 + 0.6, w0 + 0.08, w0 + 0.32, 'plant', lamp, rm, cham=0.1)


def dress_liquor(P, F, ua, ub, w0, w1, z0, z1, lamp, rm):
    """Market Wine & Liquor: the back wall shelved with bottles (wine, spirits, longnecks), stacked beer cases on the
    window's floor, a row of wine standing on them."""
    shelves(P, F, ua + 0.05, ub - 0.05, w1, z0 - 0.4, z1, 6, 0.3, lamp, rm, goods='bottle', board='furn', item=(0.075, 0.09), fill=0.95,
            shape='bottle')
    lbox(P, F, ua + 0.2, ub - 0.2, z0 - 0.02, z0 + 0.0, w0 + 0.15, w0 + 0.65, 'furn', lamp, rm)
    uu = ua + 0.25
    rnd = P.rnd
    while uu < ub - 0.65:
        n = rnd.choice((1, 2, 2, 3))
        r = rnd.random()
        for k in range(n):
            lbox(P, F, uu, uu + 0.4, z0 + k * 0.27, z0 + (k + 1) * 0.27 - 0.005, w0 + 0.22, w0 + 0.6, 'goods', lamp, rm, r=r)
        top = z0 + n * 0.27
        for j in range(4):
            bottle(P, F, uu + 0.06 + j * 0.095, top, w0 + 0.4, 0.037, 0.3, 'bottle', lamp, rm, 'wine')
        uu += 0.48


def dress_records(P, F, ua, ub, w0, w1, z0, z1, lamp, rm):
    """Northside Records: sleeves faced out on a wall of shelves, bins of records, a turntable on a crate."""
    for row in range(3):
        for i in range(int((ub - ua) / 0.34)):
            uu = ua + 0.05 + i * 0.34
            lbox(P, F, uu, uu + 0.31, z0 + 0.2 + row * 0.4, z0 + 0.51 + row * 0.4, w1 - 0.05, w1 - 0.02, 'poster', lamp, rm)
    for i in range(int((ub - ua) / 0.8)):
        uu = ua + 0.1 + i * 0.8
        lbox(P, F, uu, uu + 0.7, z0 - 0.5, z0 + 0.25, w0 + 0.4, w0 + 0.75, 'furn', lamp, rm)
        lbox(P, F, uu + 0.02, uu + 0.68, z0 + 0.25, z0 + 0.55, w0 + 0.45, w0 + 0.7, 'goods', lamp, rm)
    lbox(P, F, ua + 0.2, ua + 0.6, z0, z0 + 0.35, w0 + 0.05, w0 + 0.4, 'oak', lamp, rm)
    P.trim.lathe([(0.0, 0.0), (0.15, 0.0), (0.15, 0.02), (0.0, 0.02)], tuple(F.P(ua + 0.4, z0 + 0.35, w0 + 0.22)), 20, **P.L('furn', lamp, rm))


def dress_takeout(P, F, ua, ub, w0, w1, z0, z1, lamp, rm):
    """Golden Dragon: the counter, a lit menu over it, a tank in the window, a cat with a raised paw."""
    lbox(P, F, ua, ub, z0 - 0.5, z0 + 0.5, w1 - 0.9, w1 - 0.4, 'counter', lamp, rm)
    lbox(P, F, ua + 0.2, ub - 0.2, z1 - 0.8, z1 - 0.1, w1 - 0.04, w1 - 0.02, 'glow', lamp, rm)
    lbox(P, F, ua + 0.15, ua + 0.8, z0, z0 + 0.45, w0 + 0.1, w0 + 0.45, 'glassblock' if False else 'mirror', lamp, rm)
    lbox(P, F, ub - 0.5, ub - 0.3, z0, z0 + 0.25, w0 + 0.1, w0 + 0.25, 'gilt', lamp, rm, cham=0.05)


def dress_botanica(P, F, ua, ub, w0, w1, z0, z1, lamp, rm):
    """Ortiz Botánica: shelves of tall glass candles, plaster saints, herbs hung in bunches, a lace cloth."""
    shelves(P, F, ua + 0.05, ub - 0.05, w1, z0 - 0.3, z1, 5, 0.3, lamp, rm, goods='jar', board='furn', item=(0.06, 0.07), fill=0.9)
    rnd = P.rnd
    for i in range(int((ub - ua) / 0.5)):
        uu = ua + 0.2 + i * 0.5
        h = rnd.uniform(0.3, 0.55)
        P.trim.lathe([(0.0, 0.0), (0.07, 0.0), (0.08, 0.05), (0.05, h * 0.6), (0.06, h * 0.75), (0.04, h * 0.95), (0.0, h)],
                     tuple(F.P(uu, z0, w0 + 0.3)), 10, **P.L('counter', lamp, rm))
    lbox(P, F, ua, ub, z0 - 0.02, z0, w0, w0 + 0.6, 'paper', lamp, rm)
    for i in range(int((ub - ua) / 0.35)):
        uu = ua + 0.15 + i * 0.35
        lbox(P, F, uu, uu + 0.12, z1 - 0.5, z1 - 0.1, w0 + 0.08, w0 + 0.18, 'plant', lamp, rm, cham=0.04)


def dress_hardware(P, F, ua, ub, w0, w1, z0, z1, lamp, rm):
    shelves(P, F, ua, ub, w1, z0 - 0.4, z1, 5, 0.35, lamp, rm, goods='goods', board='cabinet', item=(0.12, 0.25), fill=0.9)
    rnd = P.rnd
    for i in range(int((ub - ua) / 0.3)):
        uu = ua + 0.1 + i * 0.3
        P.trim.lathe([(0.0, 0.0), (0.09, 0.0), (0.09, 0.22), (0.0, 0.22)], tuple(F.P(uu, z0, w0 + 0.25)), 12, **P.L('goods', lamp, rm))
    lbox(P, F, ua + 0.2, ua + 0.3, z0, z0 + 1.4, w0 + 0.5, w0 + 0.6, 'oak', lamp, rm)
    lbox(P, F, ua + 0.5, ua + 0.6, z0, z0 + 1.2, w0 + 0.55, w0 + 0.65, 'oak', lamp, rm)


def dress_wireless(P, F, ua, ub, w0, w1, z0, z1, lamp, rm):
    """Northside Wireless: a slatwall of cases and chargers, a glass counter, the repair bench's lamp."""
    lbox(P, F, ua, ub, z0 - 0.4, z1, w1 - 0.03, w1, 'counter', lamp, rm)
    for row in range(5):
        for i in range(int((ub - ua) / 0.22)):
            uu = ua + 0.05 + i * 0.22
            lbox(P, F, uu, uu + 0.12, z0 + row * 0.33, z0 + 0.22 + row * 0.33, w1 - 0.06, w1 - 0.03, 'goods', lamp, rm)
    lbox(P, F, ua + 0.3, ub - 0.3, z0 - 0.5, z0 + 0.45, w0 + 0.3, w0 + 0.75, 'glow', lamp, rm)
    lbox(P, F, ua + 0.3, ub - 0.3, z0 + 0.45, z0 + 0.5, w0 + 0.3, w0 + 0.75, 'chrome', lamp, rm)


def dress_furniture(P, F, ua, ub, w0, w1, z0, z1, lamp, rm):
    """Cardinal Furniture, closing: sofas with tags, a lamp, STORE CLOSING cards on the glass's back."""
    rnd = P.rnd
    uu = ua + 0.2
    while uu < ub - 1.6:
        r = rnd.random()
        lbox(P, F, uu, uu + 1.5, z0 - 0.4, z0 + 0.0, w0 + 0.4, w0 + 1.3, 'cloth', lamp, rm, cham=0.06, r=r)
        lbox(P, F, uu, uu + 1.5, z0 + 0.0, z0 + 0.45, w0 + 1.05, w0 + 1.3, 'cloth', lamp, rm, cham=0.06, r=r)
        for s in (0.0, 1.35):
            lbox(P, F, uu + s, uu + s + 0.15, z0 - 0.4, z0 + 0.15, w0 + 0.4, w0 + 1.3, 'cloth', lamp, rm, cham=0.05, r=r)
        lbox(P, F, uu + 0.6, uu + 0.9, z0 + 0.15, z0 + 0.3, w0 + 0.42, w0 + 0.44, 'paper', lamp, rm)
        uu += 1.9
    for i in range(int((ub - ua) / 1.5)):
        u_ = ua + 0.3 + i * 1.5
        lbox(P, F, u_, u_ + 0.8, z1 - 0.6, z1 - 0.1, w0 + 0.03, w0 + 0.04, 'paper', lamp, rm)


def dress_dental(P, F, ua, ub, w0, w1, z0, z1, lamp, rm):
    lbox(P, F, ua, ub, z0 - 0.5, z1, w1 - 0.03, w1, 'wall', lamp, rm)
    lbox(P, F, ua + 0.3, ub - 0.3, z0 - 0.5, z0 + 0.6, w1 - 0.8, w1 - 0.4, 'counter', lamp, rm)
    for i in range(int((ub - ua) / 0.7)):
        uu = ua + 0.2 + i * 0.7
        lbox(P, F, uu, uu + 0.5, z0 - 0.5, z0 - 0.05, w0 + 0.15, w0 + 0.6, 'vinyl', lamp, rm, cham=0.04)
    lbox(P, F, ua + 0.2, ua + 0.6, z0, z0 + 0.6, w1 - 0.4, w1 - 0.1, 'plant', lamp, rm, cham=0.1)


def dress_pawn(P, F, ua, ub, w0, w1, z0, z1, lamp, rm):
    """Fifth St. Loan & Pawn: a guitar wall, cased watches and rings, tools, a stacked TV, all behind the bars."""
    rnd = P.rnd
    for i in range(int((ub - ua) / 0.45)):
        uu = ua + 0.15 + i * 0.45
        r = rnd.random()
        lbox(P, F, uu, uu + 0.34, z0 + 0.4, z0 + 0.82, w1 - 0.06, w1 - 0.02, 'oak', lamp, rm, cham=0.08, r=r)
        lbox(P, F, uu + 0.13, uu + 0.21, z0 + 0.82, z0 + 1.45, w1 - 0.05, w1 - 0.03, 'furn', lamp, rm)
    lbox(P, F, ua + 0.1, ub - 0.1, z0 - 0.5, z0 + 0.2, w0 + 0.15, w0 + 0.75, 'cabinet', lamp, rm)
    lbox(P, F, ua + 0.15, ub - 0.15, z0 + 0.2, z0 + 0.22, w0 + 0.2, w0 + 0.7, 'velvet', lamp, rm)
    for i in range(int((ub - ua) / 0.12)):
        uu = ua + 0.2 + i * 0.12
        lbox(P, F, uu, uu + 0.05, z0 + 0.22, z0 + 0.25, w0 + 0.3 + (i % 3) * 0.12, w0 + 0.34 + (i % 3) * 0.12, 'gilt', lamp, rm)
    lbox(P, F, ub - 1.0, ub - 0.3, z0 + 0.25, z0 + 0.7, w0 + 0.4, w0 + 0.75, 'furn', lamp, rm)
    lbox(P, F, ub - 0.95, ub - 0.35, z0 + 0.3, z0 + 0.65, w0 + 0.39, w0 + 0.4, 'screen', lamp, rm)


def dress_vacant(P, F, ua, ub, w0, w1, z0, z1, lamp, rm):
    lbox(P, F, ua, ub, z0, z1, w0 + 0.01, w0 + 0.02, 'paper', lamp, rm)


def dress_bar(P, F, ua, ub, w0, w1, z0, z1, lamp, rm):
    """The Anchor: the room past the glass block: a backbar of bottles under a mirror, stools, beer signs' glow."""
    shelves(P, F, ua + 0.1, ub - 0.1, w1, z0 + 0.4, z0 + 1.4, 3, 0.25, lamp, rm, goods='bottle', board='furn', item=(0.07, 0.08), fill=0.97)
    lbox(P, F, ua + 0.1, ub - 0.1, z0 + 1.4, z1, w1 - 0.03, w1 - 0.01, 'mirror', lamp, rm)
    lbox(P, F, ua, ub, z0 - 0.5, z0 + 0.55, w1 - 1.4, w1 - 0.8, 'oak', lamp, rm)
    for i in range(int((ub - ua) / 0.6)):
        uu = ua + 0.3 + i * 0.6
        P.trim.lathe([(0.0, 0.0), (0.18, 0.0), (0.18, 0.03), (0.0, 0.03)], tuple(F.P(uu, z0 + 0.25, w1 - 1.65)), 12, **P.L('vinyl', lamp, rm))


def dress_lobby(P, F, ua, ub, w0, w1, z0, z1, lamp, rm):
    lbox(P, F, ua, ub, z0 - 0.5, z1, w1 - 0.03, w1, 'marble', lamp, rm)
    lbox(P, F, ua + 0.4, ua + 1.4, z0 - 0.5, z0 - 0.05, w0 + 0.6, w0 + 1.2, 'velvet', lamp, rm, cham=0.04)
    lbox(P, F, ub - 0.8, ub - 0.3, z0 - 0.5, z0 + 0.4, w0 + 0.4, w0 + 0.9, 'plant', lamp, rm, cham=0.1)


DRESS = {'produce': dress_produce, 'pharmacy': dress_pharmacy, 'slice': dress_slice, 'checkcash': dress_checkcash, 'clothier': dress_clothier,
         'nails': dress_nails, 'liquor': dress_liquor, 'records': dress_records, 'takeout': dress_takeout, 'botanica': dress_botanica,
         'hardware': dress_hardware, 'wireless': dress_wireless, 'furniture': dress_furniture, 'dental': dress_dental, 'pawn': dress_pawn,
         'vacant': dress_vacant, 'bar': dress_bar, 'lobby': dress_lobby}


# ================================================================== signs' artwork

def band_art(text, sub=None, fg=0xf0e8d0, bg=0x1a1c1e, accent=None, face='Black', squeeze=0.9, sub_color=None, frame=True, wear=1.0,
             gilt=False, seed=1, extra=None):
    """A painted sign band: a field, a rule, the name large and squeezed to fit, a line under it; worn."""
    def draw(s, Wm, Hm):
        s.rect(0, 0, Wm, Hm, bg, rough=0.45)
        if frame:
            s.rect(Hm * 0.06, Hm * 0.06, Wm - Hm * 0.12, Hm * 0.025, accent or fg, rough=0.4, metal=1.0 if gilt else 0.0)
            s.rect(Hm * 0.06, Hm * 0.915, Wm - Hm * 0.12, Hm * 0.025, accent or fg, rough=0.4, metal=1.0 if gilt else 0.0)
        size = Hm * (0.52 if sub else 0.62)
        # Fit the name in 92% of the width.
        est = len(text) * size * 0.62 * squeeze
        if est > Wm * 0.92:
            size *= Wm * 0.92 / est
        y = Hm * (0.38 if sub else 0.22)
        if gilt:
            fac.outlined(s, text, Wm / 2, y, size, 0xd6ac58, 0x2a1a08, face=face, metal=1.0, rough=0.25, squeeze=squeeze, shadow=(0x000000, size * 0.04, -size * 0.04))
        else:
            fac.outlined(s, text, Wm / 2, y, size, fg, _shade(bg, 0.5), face=face, squeeze=squeeze, width=size * 0.02)
        if sub:
            s.text(sub, Wm / 2, Hm * 0.12, Hm * 0.15, sub_color or fg, face='Bold', align='CENTER', tracking=1.25, rough=0.45)
        if extra:
            extra(s, Wm, Hm)
        if wear > 0:
            fac.worn(s, Wm, Hm, int(160 * wear), _shade(bg, 1.6) if bg < 0x404040 else _shade(bg, 0.6), 0.35, seed)
    return draw


def lightbox_art(text, sub=None, fg=0xffffff, bg=0x1f5aa8, face='Black', squeeze=0.88, stripe=None, seed=2):
    """A backlit plastic sign face: flat color, white letters, a stripe."""
    def draw(s, Wm, Hm):
        s.rect(0, 0, Wm, Hm, bg, rough=0.3)
        if stripe:
            s.rect(0, 0, Wm, Hm * 0.12, stripe, rough=0.3)
            s.rect(0, Hm * 0.88, Wm, Hm * 0.12, stripe, rough=0.3)
        size = Hm * (0.48 if sub else 0.58)
        est = len(text) * size * 0.62 * squeeze
        if est > Wm * 0.9:
            size *= Wm * 0.9 / est
        o = s.text(text, Wm / 2, Hm * (0.4 if sub else 0.24), size, fg, face=face, align='CENTER', tracking=1.02, rough=0.3)
        o.scale.x = squeeze
        if sub:
            s.text(sub, Wm / 2, Hm * 0.16, Hm * 0.14, fg, face='Bold', align='CENTER', tracking=1.2, rough=0.3)
        fac.worn(s, Wm, Hm, 60, 0x000000, 0.15, seed)
    return draw


def plain_card(lines, bg=0xf0ece0, fg=0x1a1a1a, accent=0xc8202c):
    def draw(s, Wm, Hm):
        s.rect(0, 0, Wm, Hm, bg, rough=0.8)
        s.rect(0, Hm * 0.8, Wm, Hm * 0.2, accent, rough=0.8)
        s.text(lines[0], Wm / 2, Hm * 0.84, Hm * 0.13, 0xffffff, face='Black', align='CENTER', rough=0.8)
        for i, ln in enumerate(lines[1:]):
            s.text(ln, Wm / 2, Hm * (0.55 - i * 0.2), Hm * 0.13, fg, face='Black', align='CENTER', rough=0.8)
    return draw


# ================================================================== the body types

def bays_for(L, target=1.9, margin=0.0):
    n = max(1, int(round((L - 2 * margin) / target)))
    w = (L - 2 * margin) / n
    return [margin + w * (i + 0.5) for i in range(n)], w


def walls_and_windows(P, fi, ground_opens=(), win=None, skip=(), floors_from=2, lintel='key', panes=(1, 1), sill='stone', kind=None,
                      bays=None, ac_ok=True, look_plan=None, steel=False, win_h=None, z_from=None):
    """The street face fi's windows on every upper floor at its bays (skip: {(floor, bay)}), its wall (body) with every
    opening cut, including the ground floor's (already registered in P.opens by storefront / res_door)."""
    F = P.faces[fi]
    bay_us, bw = bays if bays else bays_for(F.L)
    ww = win or min(1.12, bw - 0.6)
    wh = win_h or 1.733
    kind = P.room_kind if kind is None else kind
    for k in range(floors_from, P.floors + 1):
        for i, uc in enumerate(bay_us):
            if (k, i) in skip or (None, i) in skip:
                continue
            uc_ = snap_u(F, uc - ww / 2) + ww / 2
            z0 = snap_z(P.fz(k) + 0.85)
            ac = ac_ok and P.detail == 'full' and P.rnd.random() < P.ac and k > 1
            window_unit(P, F, uc_, z0, ww, wh, panes=panes, ac=ac, lintel=lintel, sill=sill, kind=kind, steel=steel,
                        raise_=0.25 if P.rnd.random() < 0.12 else 0.0)
    return bay_us


def finish_walls(P, slot_of=None, top_extra=None):
    """Every wall of the footprint, street faces with their openings, to the parapet's top."""
    z_par = P.top + P.parapet
    slot_of = slot_of or slots_by_height([], [P.wall_m])
    for fi, F in enumerate(P.faces):
        zt = P.heights.get(fi, z_par)
        wall(P.body, F, 0.0, F.L, 0.0, zt, P.opens[fi], slot_of, reg=0)
    return z_par


def corner_wear(P):
    pass


# ================================================================== finishing a plot: its meshes

def _attr_array(me, name, n, vec=False):
    a = me.attributes.get(name)
    if a is None:
        return np.zeros((n, 3) if vec else n, np.float32)
    if vec:
        out = np.empty(n * 3, np.float32)
        a.data.foreach_get('vector', out)
        return out.reshape(n, 3)
    out = np.empty(n, np.float32)
    a.data.foreach_get('value', out)
    return out


def _vnoise(P, scale, seed):
    from artkit import texmaps
    return texmaps.value_noise(P, scale, seed=seed, octaves=2)


def body_colors(P, obj):
    """COLOR_0 on the masonry: R soot and streaks (under sills, AC units, fire escapes and the cornice, splashed up from
    the sidewalk), G edge wear (the openings' arrises, the building's corners, hand height), B rain exposure (low under
    the cornice, in heads and jambs, under the sign band; high on sills, copings and the roof). The panes keep theirs."""
    me = obj.data
    nf = len(me.polygons)
    reg = _attr_array(me, 'reg', nf)
    ca = me.color_attributes.get('Color') or me.color_attributes.new('Color', 'FLOAT_COLOR', 'CORNER')
    me.color_attributes.active_color = ca
    me.color_attributes.render_color_index = me.color_attributes.find('Color')
    nl = len(me.loops)
    cols = np.empty(nl * 4, np.float32)
    ca.data.foreach_get('color', cols)
    cols = cols.reshape(nl, 4)
    loop_v = np.empty(nl, np.int64)
    me.loops.foreach_get('vertex_index', loop_v)
    co = np.empty(len(me.vertices) * 3, np.float64)
    me.vertices.foreach_get('co', co)
    co = co.reshape(-1, 3)
    pos = co[loop_v]
    loop_f = np.empty(nl, np.int64)
    starts = np.empty(nf, np.int64)
    totals = np.empty(nf, np.int64)
    me.polygons.foreach_get('loop_start', starts)
    me.polygons.foreach_get('loop_total', totals)
    loop_f = np.repeat(np.arange(nf), totals)
    fn = np.empty(nf * 3, np.float64)
    me.polygons.foreach_get('normal', fn)
    fn = fn.reshape(nf, 3)[loop_f]
    lr = reg[loop_f]
    x, y, z = pos[:, 0], pos[:, 1], pos[:, 2]
    R = np.zeros(nl, np.float32)
    for F, ua, ub, ztop, length, strength in P.soot:
        d = (pos[:, :2] - np.array([F.a.x, F.a.y])) @ np.array([F.n.x, F.n.y])
        on = (np.abs(d) < 0.45) & ((fn[:, :2] @ np.array([F.n.x, F.n.y])) > 0.7)
        u = (pos[:, :2] - np.array([F.a.x, F.a.y])) @ np.array([F.t.x, F.t.y])
        edge = np.clip(np.minimum(u - ua, ub - u) / 0.15 + 0.5, 0, 1)
        dz = ztop - z
        s = np.where(on & (dz > 0) & (dz < length), strength * np.clip(1.0 - dz / length, 0, 1) ** 1.3 * edge, 0.0)
        R = np.maximum(R, s.astype(np.float32))
    R = np.maximum(R, np.clip((z - (P.top - 1.6)) / 1.6, 0, 1) * 0.45 * (lr == 0))
    R = np.maximum(R, np.clip(1.0 - z / 0.55, 0, 1) * 0.4)
    nz = _vnoise(pos, 1.7, P.seed)
    R = np.clip(R * (0.55 + 0.9 * nz) + (nz - 0.5) * 0.12, 0, 1)
    G = np.zeros(nl, np.float32)
    # Arrises: reveal corners on the wall's plane.
    for fi, F in enumerate(P.faces):
        d = (pos[:, :2] - np.array([F.a.x, F.a.y])) @ np.array([F.n.x, F.n.y])
        on_plane = np.abs(d) < 0.01
        G = np.where(on_plane & np.isin(lr, (2, 3, 4)), 0.9, G)
    for px, py in P.poly:
        dc = np.hypot(x - px, y - py)
        G = np.maximum(G, np.where(dc < 0.05, 0.75, 0.0))
    G = np.maximum(G, np.clip(1.0 - z / 1.3, 0, 1) * 0.45 * np.isin(lr, (0, 4)))
    G = np.where(lr == 8, np.maximum(G, 0.5), G)
    G = np.clip(G * (0.7 + 0.6 * _vnoise(pos, 0.9, P.seed + 3)), 0, 1)
    B = np.full(nl, 0.85, np.float32)
    B = np.where((lr == 0) & (z < P.gf - 0.05), 0.4, B)
    B = np.where((lr == 0) & (z > P.top - 1.2) & (z < P.top + 0.05), 0.45, B)
    B = np.where(lr == 2, 0.1, B)
    B = np.where(lr == 3, 0.9, B)
    B = np.where(lr == 4, 0.35, B)
    B = np.where(np.isin(lr, (5, 6, 8)), 0.95, B)
    B = np.where(lr == 9, 0.6, B)
    keep = lr == 7
    new = np.stack([R, G, B, np.ones(nl, np.float32)], 1)
    cols = np.where(keep[:, None], cols, new)
    ca.data.foreach_set('color', cols.ravel())


def _px_for(budget, share):
    s = math.sqrt(budget * share)
    p = 2 ** int(round(math.log2(max(s, 256))))
    return int(min(2048, max(256, p)))


SLOT_DENSITY_FULL = (180.0, 220.0, 260.0, 110.0, 330.0)     # px per meter: stone, metal, storefront, lit, prints
SLOT_DENSITY_PROXY = (90.0, 110.0, 130.0, 60.0, 260.0)


def sat_dish(P, mount, facing, r=0.28, mast=0.2):
    """A small satellite dish on a clamp at mount, aimed along facing: the reflector (a shallow paraboloid, front and
    back), the feed arm up from its lower rim to the LNB at the focus, the mast back to the clamp. Trim, metal slot."""
    tb = P.trim
    dk, mk, bk = P.T(1, 'ac'), P.T(1, 'galv'), P.T(1, 'black')
    f = Vector(facing).normalized()
    side = Z.cross(f)
    side = side.normalized() if side.length > 1e-3 else Vector((1.0, 0.0, 0.0))
    up = f.cross(side).normalized()
    m = Vector(mount)
    c = m + f * mast
    ax = Matrix((side, up, f)).transposed()
    fl = r * 1.1
    zr = r * r / (4 * fl)
    back = [(0.0, -0.014), (r * 0.5, (r * 0.5) ** 2 / (4 * fl) - 0.014), (r, zr - 0.012)]
    front = [(r, zr + 0.003), (r * 0.94, (r * 0.94) ** 2 / (4 * fl)), (r * 0.7, (r * 0.7) ** 2 / (4 * fl)), (r * 0.45, (r * 0.45) ** 2 / (4 * fl)),
             (r * 0.2, (r * 0.2) ** 2 / (4 * fl)), (0.0, 0.0)]
    tb.lathe(back + front, tuple(c), 20, axis=ax, **dk)
    # The mast and clamp, the feed arm and the LNB.
    tb.tube(m, c - f * 0.014, 0.022, n=8, **mk)
    tb.obox(m, (f, side, up), (0.05, 0.05, 0.07), 0.008, seg=1, **mk)
    rim = c + f * zr - up * (r * 0.96)
    foc = c + f * fl
    tb.tube(rim, foc - f * 0.04, 0.011, n=6, **mk)
    tb.obox(foc, (side, up, f), (0.026, 0.026, 0.05), 0.008, seg=1, **bk)
    tb.tube(foc + f * 0.05, foc + f * 0.075, 0.03, 0.022, n=10, **dk)
    # The coax: from the LNB along the arm and down the mast.
    tb.path_tube([foc - f * 0.05, rim - f * 0.01, c - f * 0.03 - up * 0.02, m - up * 0.06, m - up * 0.4 - f * 0.02], 0.004, n=4, **bk)


def _face_keepouts(P, fi):
    """Rectangles (u0, u1, z0, z1) on face fi that services must stay clear of: its openings, and the signs on it."""
    F = P.faces[fi]
    out = Vector((F.n.x, F.n.y, 0.0))
    tt = Vector((F.t.x, F.t.y, 0.0))
    ko = [(o['u0'], o['u1'], o['z0'], o['z1']) for o in P.opens[fi]]
    ko += [k[1:] for k in getattr(P, 'svc_keep', []) if k[0] == fi]
    for sg in P.signs:
        if 'c' not in sg:
            continue
        c, n = Vector(sg['c']), Vector(sg['n'])
        rel = c - F.a
        d = rel.dot(out)
        if abs(n.dot(out)) < 0.9 and not (abs(n.dot(tt)) > 0.9 and -0.1 < d < 1.0):
            continue
        if not (-1.2 < d < 0.4):
            continue
        u = rel.dot(tt)
        half = sg['w'] / 2 if abs(n.dot(out)) >= 0.9 else 0.1
        ko.append((u - half, u + half, c.z - sg['h'] / 2, c.z + sg['h'] / 2))
    return ko


def _free_u(P, fi, want, half, z0, z1, margin=0.25, end=0.6, step=0.05):
    """The u nearest want on face fi where a strip half wide, z0..z1, clears every keep-out by margin; None if none."""
    F = P.faces[fi]
    ko = _face_keepouts(P, fi)
    n = int(F.L / step)
    for u in sorted((i * step for i in range(n + 1)), key=lambda u: abs(u - want)):
        if u - half < end or u + half > F.L - end:
            continue
        if all(not (u + half + margin > a and u - half - margin < b and z1 > c and z0 < d) for a, b, c, d in ko):
            return u
    return None


def _wall_out(P, fi, u, z):
    """How far the wall's face stands out of the face plane at (u, z): 5 cm on a rusticated base, else 0."""
    return 0.05 if any(r[0] == fi and r[1] - 1e-3 <= u <= r[2] + 1e-3 and r[3] - 1e-3 <= z <= r[4] + 1e-3 for r in getattr(P, 'rust', [])) else 0.0


def _keep(P, fi, u0, u1, z0, z1):
    if not hasattr(P, 'svc_keep'):
        P.svc_keep = []
    P.svc_keep.append((fi, u0, u1, z0, z1))


def services(P):
    """What a city adds to a front (full-detail plots): conduit up to the lit sign bands, with its straps and junction box;
    a fire department connection (standpipe or sprinkler siamese, brass, its red plate) on the elevator buildings, the
    commercial blocks and the warehouse; a dish on a mast behind some parapets (fire escapes get theirs in fire_escape)."""
    if P.detail != 'full' or getattr(P, 'is_proxy', False) or P.bodytype == 'B6':
        return
    gk, bk = P.T(1, 'galv'), P.T(1, 'black')
    for fi in P.streets:
        F = P.faces[fi]
        out = Vector((F.n.x, F.n.y, 0.0))
        tt = Vector((F.t.x, F.t.y, 0.0))
        # Conduit to the widest band over the shop on this face (its lightbox or its gooseneck lamps).
        bands = []
        for sg in P.signs:
            if 'c' not in sg or Vector(sg['n']).dot(out) < 0.9:
                continue
            c = Vector(sg['c'])
            if 2.4 < c.z < P.gf + 1.2 and sg['w'] > 1.2 and abs((c - F.a).dot(out)) < 0.6:
                bands.append(sg)
        if bands:
            sg = max(bands, key=lambda q: q['w'])
            c = Vector(sg['c'])
            uc = (c - F.a).dot(tt)
            zb = c.z
            best = None
            for want in (uc + sg['w'] / 2 + 0.35, uc - sg['w'] / 2 - 0.35):
                u = _free_u(P, fi, want, 0.05, 0.35, zb - sg['h'] / 2 - 0.02, margin=0.12, end=0.35)
                if u is not None and abs(u - want) < 0.9 and (best is None or abs(u - want) < abs(best[0] - best[1])):
                    best = (u, want)
            if best and _wall_out(P, fi, best[0], 0.4) != _wall_out(P, fi, best[0], zb - 0.3):
                best = None
            if best:
                u = best[0]
                edge = uc + (sg['w'] / 2 if u > uc else -sg['w'] / 2)
                wo = _wall_out(P, fi, u, 0.4)
                w_c = -0.03 - wo
                zt = zb - 0.05
                _keep(P, fi, u - 0.1, u + 0.1, 0.3, zt)
                # The LB fitting where it leaves the wall, the run up, the elbow and the run into the band's end.
                F.box(P.trim, u - 0.035, u + 0.035, 0.32, 0.46, w_c - 0.035, -wo, 0.006, seg=1, **gk)
                P.trim.tube(F.P(u, 0.46, w_c), F.P(u, zt - 0.06, w_c), 0.012, n=8, **gk)
                P.trim.path_tube([F.P(u, zt - 0.08, w_c), F.P(u, zt - 0.01, w_c), F.P(u + (0.06 if edge > u else -0.06), zt, w_c), F.P(edge, zt, w_c)],
                                 0.012, n=8, **gk)
                F.box(P.trim, u - 0.07, u + 0.07, zt - 0.3, zt - 0.16, w_c - 0.04, -wo, 0.006, seg=1, **gk)
                zz = 0.9
                while zz < zt - 0.4:
                    F.box(P.trim, u - 0.02, u + 0.02, zz - 0.012, zz + 0.012, w_c - 0.016, -wo, **gk)
                    zz += 0.95
                P.soot.append((F, u - 0.08, u + 0.08, zt - 0.3, 1.2, 0.2))
                P.note(f'services: conduit on face {fi} at u {u:.2f} up to the band at z {zt:.2f}')
    # The fire department connection on the first street face, by the middle.
    if P.bodytype in ('B2', 'B3', 'B5'):
        fi = P.streets[0]
        F = P.faces[fi]
        u = _free_u(P, fi, F.L * 0.5, 0.25, 0.3, 1.32, margin=0.2)
        if u is not None and _wall_out(P, fi, u - 0.2, 1.2) == _wall_out(P, fi, u + 0.2, 0.4):
            _keep(P, fi, u - 0.3, u + 0.3, 0.3, 1.3)
            ax = Matrix((Vector((F.t.x, F.t.y, 0.0)), Z, Vector((F.n.x, F.n.y, 0.0)))).transposed()
            brass, chrome = P.T(1, 'brass'), P.T(1, 'chrome')
            zc = 0.78
            wo = _wall_out(P, fi, u, zc)
            P.trim.lathe([(0.0, 0.0), (0.17, 0.0), (0.17, 0.012), (0.15, 0.022), (0.0, 0.024)], tuple(F.P(u, zc, -wo)), 24, axis=ax, **brass)
            P.trim.tube(F.P(u, zc, -wo), F.P(u, zc, -0.13 - wo), 0.062, n=14, **brass)
            for sd in (-1, 1):
                a = F.P(u, zc, -0.12 - wo)
                b = F.P(u + sd * 0.14, zc - 0.03, -0.25 - wo)
                P.trim.tube(a, b, 0.046, n=12, **brass)
                d = (b - a).normalized()
                P.trim.tube(b, b + d * 0.045, 0.06, n=6, **chrome)
                P.trim.tube(b + d * 0.045, b + d * 0.055, 0.05, n=6, **chrome)
                # The cap's chain, looped down to the body.
                P.trim.path_tube([b + d * 0.02 - Z * 0.06, F.P(u + sd * 0.09, zc - 0.13, -0.17 - wo), F.P(u + sd * 0.03, zc - 0.07, -0.11 - wo)], 0.004, n=4,
                                 **chrome)
            label = 'STANDPIPE' if P.bodytype == 'B2' else 'AUTO SPKR'

            def plate(s, Wm, Hm, label=label):
                s.rect(0, 0, Wm, Hm, 0xb01e1a, rough=0.35)
                s.rect(Hm * 0.08, Hm * 0.08, Wm - Hm * 0.16, Hm - Hm * 0.16, 0xc22a22, rough=0.3)
                o = s.text(label, Wm / 2, Hm * 0.27, Hm * 0.5, 0xf2eee4, face='Bold', align='CENTER', tracking=1.1, rough=0.4)
                fac.worn(s, Wm, Hm, 40, 0x3a1a10, 0.35, len(label))
            P.sign('fdc', 0.34, 0.1, plate, F.P(u, zc + 0.36, -0.008 - wo), F.n, depth=0.007, px=512, edge='enamel')
            P.note(f'services: {label} connection on face {fi} at u {u:.2f}')
    # A dish on a mast behind the parapet, peeking over it (walk-ups and corners, about half).
    if P.bodytype in ('B1', 'B4') and P.rnd.random() < 0.5:
        fi = P.streets[0]
        F = P.faces[fi]
        u = P.rnd.uniform(1.2, max(1.3, F.L - 1.2))
        base = F.P(u, P.top - 0.2, 0.9)
        top = F.P(u, P.top + P.parapet + 0.45, 0.9)
        P.trim.tube(base, top, 0.024, n=8, **gk)
        P.trim.obox(base + Z * 0.03, (Vector((F.t.x, F.t.y, 0.0)), Vector((F.n.x, F.n.y, 0.0)), Z), (0.22, 0.22, 0.03), 0.0, seg=1, **gk)
        out = Vector((F.n.x, F.n.y, 0.0))
        tt = Vector((F.t.x, F.t.y, 0.0))
        yaw = P.rnd.uniform(-0.7, 0.7)
        el = P.rnd.uniform(0.55, 0.75)
        sat_dish(P, top - Z * 0.05, (out * math.cos(yaw) + tt * math.sin(yaw)) * math.cos(el) + Z * math.sin(el), r=P.rnd.choice((0.3, 0.38, 0.45)), mast=0.1)
        P.note(f'services: roof dish on face {fi} at u {u:.2f}')


def finish_plot(P):
    """The plot's meshes: SM_NS_Bldg_<Plot> (body), _Trim, _Glass, _Glow<Family>, _Grille."""
    services(P)
    out = []
    # Every trim slot gets faces (the slot order is the contract): an unused slot holds one 1 cm quad on the roof.
    rz = P.top + P.parapet - 0.6
    cx = sum(p[0] for p in P.poly) / len(P.poly)
    cy = sum(p[1] for p in P.poly) / len(P.poly)
    for s in range(4):
        if not P.used[s]:
            attrs = P.L('wall', (cx, cy, rz + 1.0), (6, 0.5, 0.5)) if s == 3 else P.T(s, 'tar')
            P.trim.box(cx, cx + 0.01, cy, cy + 0.01, rz, rz + 0.01, **attrs)
            P.note(f'trim slot {s} unused: holds a 1 cm stub on the roof')
    if not P.signs:
        def blank(s, Wm, Hm):
            s.rect(0, 0, Wm, Hm, 0x202020)
        P.sign('blank', 0.01, 0.01, blank, (cx, cy, rz + 0.05), (0.0, 0.0, 1.0), board=False, px=16)
    # Body.
    body = P.body.obj(P.name, uv_meters=True)
    slots = sorted({int(v) for v in P.body.fattr['slot'] if v is not None})
    for sidx in slots:
        body.data.materials.append(body_material(BODY_ORDER[sidx]).m)
    remap = {s: i for i, s in enumerate(slots)}
    mi = [remap[int(v)] for v in P.body.fattr['slot']]
    body.data.polygons.foreach_set('material_index', mi)
    if 'Color' not in body.data.color_attributes:
        body.data.color_attributes.new('Color', 'FLOAT_COLOR', 'CORNER')
    body_colors(P, body)
    body.data.shade_flat()
    out.append(body)
    # Trim.
    trim = P.trim.obj(P.name + '_Trim')
    front = sum(P.faces[i].L for i in P.streets)
    budget = 2048 * 2048 * max(front, 6.0) / 10.0
    mats = []
    for s, nm in enumerate(TRIM_SLOTS):
        mname = f'{P.p}_{nm}'
        if nm == 'lit':
            m = lit_material(mname, P.lit_strength)
        elif nm == 'print':
            m = core.Mat(mname)
            fac.print_material(m, P.signs)
        else:
            m = trim_material(mname, P.palette, P.used[s])
        mats.append(m)
        trim.data.materials.append(m.m)
    trim.data.polygons.foreach_set('material_index', [int(v) for v in P.trim.fattr['slot']])
    # Each slot's texture from its own area at a target density (px/m), within the spec's budget (2048^2 per 10 m of
    # frontage) give or take 30%.
    areas = np.zeros(5)
    pa = np.empty(len(trim.data.polygons), np.float32)
    trim.data.polygons.foreach_get('area', pa)
    pi = np.array([int(v) for v in P.trim.fattr['slot']])
    for s_ in range(5):
        areas[s_] = float(pa[pi == s_].sum())
    dens = SLOT_DENSITY_FULL if P.detail == 'full' else SLOT_DENSITY_PROXY
    want = [math.sqrt(max(a, 0.01) * d * d / 0.5) for a, d in zip(areas, dens)]

    def p2(x, lo, hi):
        return int(min(hi, max(lo, 2 ** int(round(math.log2(max(x, 1.0)) - 0.2)))))
    # The prints, the lit displays and the stone first (what's read and looked into); the metal and the storefront
    # share what's left.
    sz = [0] * 5
    sz[4] = p2(want[4], 512 if len(P.signs) > 1 else 128, 2048)
    sz[3] = p2(want[3], 256, 1024)
    sz[0] = p2(want[0], 128, 2048 if (P.bodytype == 'B6' and P.detail == 'full') else 1024)
    rest = max(budget * 1.3 - sz[4] ** 2 - sz[3] ** 2 - sz[0] ** 2, 2 * 512 * 512)
    k = min(1.0, math.sqrt(rest / max(want[1] ** 2 + want[2] ** 2, 1.0)))
    lo_mf = 1024 if P.detail == 'full' else 512
    sz[1] = p2(want[1] * k, lo_mf if areas[1] > 4.0 else 128, 2048)
    sz[2] = p2(want[2] * k, lo_mf if areas[2] > 4.0 else 128, 2048)
    if P.detail == 'proxy':
        # Proxy detail (spec 0.10, 4.4): the Market proxy-detail set shares about one 2048^2 between its five plots;
        # the Rialto and the library one 2048^2 each.
        cap = 2048 * 2048 * (1.0 if P.bodytype == 'B6' else 0.22)
        while sum(x * x for x in sz) > cap * 1.05:
            k_ = max(range(5), key=lambda i: sz[i])
            if sz[k_] <= 128:
                break
            sz[k_] //= 2
    for s_, m in enumerate(mats):
        TEXTURE_SIZES[m.m.name] = sz[s_]
    core.finish_hard_surface(trim, 32.0)
    groups = [(core.material_is(trim, m.m.name), 'box', None, (0.0, 0.0, 1.0, 1.0)) for m in mats]
    groups.append((lambda f: True, 'box', None, (0.0, 0.0, 1.0, 1.0)))
    core.uv_layout(trim, groups)
    out.append(trim)
    # Glass.
    if len(P.glass):
        g = P.glass.obj(P.name + '_Glass')
        g.data.materials.append(glass_material())
        core.uv_layout(g, [(lambda f: True, 'box', None, (0.0, 0.0, 1.0, 1.0))])
        out.append(g)
    # Glows.
    for fam, mb in P.glow.items():
        if not len(mb):
            continue
        g = mb.obj(f'{P.name}_Glow{fam}')
        gm = glow_material(f'glow_{P.p}_{fam.lower()}')
        g.data.materials.append(gm.m)
        g.data.shade_smooth()
        if fam == 'Neon' and P.neon:
            # COLOR_0: the design color (linear), so the runtime material can take it from the mesh.
            ca = g.data.color_attributes.get('Color') or g.data.color_attributes.new('Color', 'FLOAT_COLOR', 'CORNER')
            c = hx(P.neon)
            ca.data.foreach_set('color', list(c) * len(g.data.loops))
            g.data.color_attributes.active_color = ca
            va = gm.node('ShaderNodeVertexColor')
            va.layer_name = 'Color'
            gm.link(va.outputs['Color'], gm.bsdf.inputs['Emission Color'])
        out.append(g)
    # The grille, its origin at the top middle of its opening.
    if len(P.grille):
        g = P.grille.obj(P.name + '_Grille')
        o = P.grille_origin
        g.data.transform(Matrix.Translation(-o))
        m = trim_material(f'{P.p}_grille', P.palette, {'gate'})
        for f_ in g.data.polygons:
            pass
        a = g.data.attributes.new('ss_kind', 'FLOAT', 'FACE')
        a.data.foreach_set('value', [float(NK['gate'])] * len(g.data.polygons))
        g.data.materials.append(m.m)
        TEXTURE_SIZES[m.m.name] = 1024 if P.detail == 'full' else 512
        core.finish_hard_surface(g, 30.0)
        core.uv_layout(g, [(lambda f: True, 'box', None, (0.0, 0.0, 1.0, 1.0))])
        g['grille_at'] = tuple(o)
        g['place_at'] = tuple(o)
        out.append(g)
        so = stage_of(o)
        P.note(f'_Grille origin at stage ({so[0]:.0f}, {so[1]:.0f}, {so[2]:.0f}) cm')
    for nm, mb in getattr(P, 'extra_meshes', []):
        o = mb.obj(nm)
        o.data.materials.append(screen_material())
        out.append(o)
    for o in getattr(P, 'hands', []):
        o['place_at'] = o['pivot']
        out.append(o)
    tris = 0
    for o in out:
        tris += sum(len(p.vertices) - 2 for p in o.data.polygons)
    slots_s = ', '.join(f'{i} {s.material.name}' for i, s in enumerate(body.material_slots))
    print(f'[ns_fronts] {P.name}: {len(out)} meshes, {tris} triangles; body slots {slots_s}; trim slots ' +
          ', '.join(f'{i} {m.m.name} {TEXTURE_SIZES.get(m.m.name)}' for i, m in enumerate(mats)))
    for n_ in P.notes:
        print(f'[ns_fronts] {P.name}: {n_}')
    return out


# ================================================================== the plots (spec 2.3)

def rect_poly(X0, X1, Y0, Y1):
    """A footprint from stage cm extents, CCW in Blender."""
    xs = sorted((-X0 / 100.0, -X1 / 100.0))
    ys = sorted((Y0 / 100.0, Y1 / 100.0))
    return [(xs[0], ys[0]), (xs[1], ys[0]), (xs[1], ys[1]), (xs[0], ys[1])]


def street_edge(poly, side):
    """The index of the footprint edge on a side: 'w' (stage +X, Blender -x), 'e', 'n' (+y), 's'."""
    k = len(poly)
    best, bi = -1e9, 0
    want = {'w': (-1, 0), 'e': (1, 0), 'n': (0, 1), 's': (0, -1)}[side]
    for i in range(k):
        F = Face(poly[i], poly[(i + 1) % k])
        d = F.n.x * want[0] + F.n.y * want[1]
        if d > best + 1e-6:
            best, bi = d, i
    return bi


def b1_front(P, fi, door_u=None, shop=None, sign=None, number=None, fe=None, lintel='key', panes=(1, 1), sh=3.3, band_h=0.55,
             goosenecks=True, skip=(), bays=None, kind=None):
    """A B1 walk-up's street face: the tenants' door, a shop (u0, u1, dress, opts), its sign band and storefront
    cornice, windows over every bay, a fire escape over bays fe = (i0, i1)."""
    F = P.faces[fi]
    if door_u is not None:
        res_door(P, F, door_u, number=number)
    if shop:
        u0, u1, dress, opts = shop
        storefront(P, F, u0, u1, sh, dress=DRESS.get(dress), **opts)
        if sign:
            bz = sh + 0.38 + band_h / 2
            P.sign('band', u1 - u0 + 0.1, band_h, sign, F.P((u0 + u1) / 2, bz, -0.065), F.n, depth=0.05)
            if goosenecks and P.detail == 'full':
                for gu in (u0 + (u1 - u0) * 0.2, u0 + (u1 - u0) * 0.8):
                    gooseneck(P, F, gu, bz + band_h / 2 + 0.12)
    # The storefront cornice over the shop storey.
    sc = [(0.0, 0.0), (0.05, 0.0), (0.05, 0.04), (0.1, 0.06), (0.1, 0.1), (0.2, 0.14), (0.22, 0.18), (0.28, 0.22), (0.28, 0.27), (0.3, 0.29), (0.3, 0.32), (0.0, 0.32)]
    a, b = F.P(0.0, 0, 0), F.P(F.L, 0, 0)
    P.trim.molding([(a.x, a.y), (b.x, b.y)], sc, max(P.gf - 0.34, sh + 0.38 + band_h + 0.02), **P.T(1, 'cornice'))
    if bays is None:
        bays = bays_for(F.L, 2.05)
    walls_and_windows(P, fi, lintel=lintel, panes=panes, skip=skip, bays=bays, kind=kind)
    if fe:
        bu, bw = bays
        i0, i1 = fe
        ua, ub = bu[i0] - bw / 2 + 0.1, bu[i1] + bw / 2 - 0.1
        fire_escape(P, F, ua, ub, [snap_z(P.fz(k) + 0.85) - 0.17 for k in range(2, P.floors + 1)])
    return F


def main_cornice(P, fis, kind='metal', height=0.95, project=0.7):
    """The cornice along the street faces fis (in footprint order, a continuous run round corners)."""
    pts = []
    for fi in fis:
        F = P.faces[fi]
        if not pts:
            pts.append((F.a.x, F.a.y))
        pts.append((F.b.x, F.b.y))
    if kind == 'metal':
        cornice(P, pts, P.top - 0.35, height, project, slot=1, kind='cornice')
    else:
        cornice(P, pts, P.top - 0.35, height, project, slot=0, kind='stone')


def plot_hkim():
    P = Plot('HKim', rect_poly(0, -1500, -3600, -2150), [3], floors=5, gf=4.5, fh=3.15, seed=2875,
             palette={'front': dict(type='paint', hex=0x1f4a2e, under=0x5a4a34, chips=0.9, under_metal=0.3, soot=0.5),
                      'cornice': dict(type='paint', hex=0xc4b9a0, under=0x5d6266, chips=0.8, soot=0.7, under_metal=0.6),
                      'tile': dict(type='tile', hex=0x2e5c40, alt=0x244a34, size=0.024)})
    fi = street_edge(P.poly, 'w')
    P.streets = [fi]
    P.faces[fi].kind = 'street'
    sign = band_art("KIM'S PRODUCE", 'FRUIT · VEGETABLES · FLOWERS · DELI · 555-0174', fg=0xf4efe0, bg=0x1f4a2e, accent=0xd8b04a, seed=3)
    b1_front(P, fi, door_u=1.25, number='1806', shop=(2.4, 14.05, 'produce', dict(door='center', double=True, bulk='tile')), sign=sign,
             fe=(4, 5))
    main_cornice(P, [fi])
    F = P.faces[fi]
    downspout(P, F, F.L - 0.25, P.top - 0.35)
    finish_walls(P)
    roof(P, P.top - 0.2, P.top + P.parapet)
    return P


PLOTS = {'HKim': plot_hkim}


# ================================================================== build

_ALL = []
_PLOTS = []


def build():
    core.reset()
    _ALL.clear()
    _PLOTS.clear()
    if os.environ.get('NSF_TEX', '1') != '0' and (not _ONLY or 'tex' in _ONLY):
        make_textures()
    for pid, fn in list(PLOTS.items()) + list(LANDMARKS.items()) + list(PROXIES.items()) + [('Penny', plot_penny)] + list(SKY.items()):
        if _ONLY and pid not in _ONLY:
            continue
        t0 = time.time()
        P = fn()
        if pid == 'Penny':
            objs = finish_penny(P)
        elif getattr(P, 'is_sky', False):
            objs = finish_sky(P)
        else:
            objs = finish_proxy(P) if getattr(P, 'is_proxy', False) else finish_plot(P)
        _PLOTS.append((P, objs))
        _ALL.extend(objs)
        print(f'[ns_fronts] built {pid} in {time.time() - t0:.1f}s')
    for o in _ALL:
        o.data.name = o.name
    return list(_ALL)


_REUSED = []


def _bake_sig(o):
    """What a baked mesh's textures depend on: its vertices, faces' slots and UVs, its materials' names, sizes and
    unlinked node values, and a coarse digest of every image they read (the sign sheets). A rebuild whose mesh hashes
    the same reuses the textures on disk (art/build/textures/<mesh>/bake_sig.txt) instead of baking again."""
    import hashlib
    me = o.data
    h = hashlib.sha1()
    co = np.empty(len(me.vertices) * 3, np.float32)
    me.vertices.foreach_get('co', co)
    h.update(np.round(co, 4).tobytes())
    mi = np.empty(len(me.polygons), np.int32)
    me.polygons.foreach_get('material_index', mi)
    h.update(mi.tobytes())
    for ul in me.uv_layers[:1]:
        uv = np.empty(len(me.loops) * 2, np.float32)
        ul.data.foreach_get('uv', uv)
        h.update(np.round(uv, 5).tobytes())
    seen = set()

    def tree(nt):
        for n in sorted(nt.nodes, key=lambda n: n.name):
            h.update(n.bl_idname.encode())
            for k in ('operation', 'blend_type', 'interpolation', 'data_type'):
                if hasattr(n, k):
                    h.update(str(getattr(n, k)).encode())
            for i in n.inputs:
                if i.is_linked or not hasattr(i, 'default_value'):
                    continue
                v = i.default_value
                if hasattr(v, '__len__') and not isinstance(v, str):
                    v = tuple(round(float(x), 4) for x in v)
                elif isinstance(v, float):
                    v = round(v, 4)
                h.update(repr(v).encode())
            if getattr(n, 'node_tree', None) is not None and n.node_tree.name not in seen:
                seen.add(n.node_tree.name)
                tree(n.node_tree)
            im = getattr(n, 'image', None)
            if im is not None:
                w, hh = im.size
                h.update(f'{w}x{hh}'.encode())
                if w * hh > 0:
                    px = np.empty(w * hh * 4, np.float32)
                    im.pixels.foreach_get(px)
                    a = px.reshape(hh, w, 4)
                    gy, gx = max(1, hh // 16), max(1, w // 16)
                    g = a[:hh // gy * gy, :w // gx * gx].reshape(hh // gy, gy, w // gx, gx, 4).mean(axis=(1, 3))
                    h.update(np.round(g * 40.0).astype(np.int16).tobytes())
        for l in nt.links:
            h.update(f'{l.from_node.name}.{l.from_socket.identifier}>{l.to_node.name}.{l.to_socket.identifier}'.encode())
    for s_ in o.material_slots:
        m = s_.material
        h.update(f'{m.name}|{TEXTURE_SIZES.get(m.name)}|{TEXTURE_SIZE}|{AO_DISTANCE}'.encode())
        tree(m.node_tree)
    return h.hexdigest()


def _try_reuse(o, trust):
    """Points o's slots at the textures a previous build baked for it, as core.bake would have, when its signature
    matches (or the plot is listed in NSF_REUSE_TRUST: textures baked before signatures were written). False if not."""
    d = os.path.join(core.BUILD_DIR, 'textures', o.name)
    sig_p = os.path.join(d, 'bake_sig.txt')
    if not trust:
        if not os.path.exists(sig_p) or open(sig_p).read().strip() != o['bake_sig']:
            return False
    n = len(o.material_slots)
    plan = []
    for i, s_ in enumerate(o.material_slots):
        suf = f'_{i}' if n > 1 else ''
        paths = {k: os.path.join(d, f'T_{o.name}{suf}_{k}.png') for k in ('BaseColor', 'ORM', 'Normal', 'Emissive')}
        if not all(os.path.exists(paths[k]) for k in ('BaseColor', 'ORM', 'Normal')):
            return False
        bsdf = next(nd for nd in s_.material.node_tree.nodes if nd.type == 'BSDF_PRINCIPLED')
        spec_in = bsdf.inputs['Specular IOR Level']
        plan.append((suf, paths, bsdf.inputs['Emission Strength'].default_value, 0.5 if spec_in.is_linked else spec_in.default_value))
    for i, (suf, paths, strength, spec) in enumerate(plan):
        m = core.gltf_material(f'M_{o.name}{suf}', paths['BaseColor'], paths['ORM'], paths['Normal'],
                               paths['Emissive'] if os.path.exists(paths['Emissive']) else None, strength, spec)
        m.use_backface_culling = not DOUBLE_SIDED
        o.material_slots[i].material = m
    return True


def bake_parts(objs):
    """Bakes the trims and grilles. The bodies stay visible (their walls occlude the trim's bake, as in the street);
    glass and glows are hidden. A grille bakes in its place (its mesh's origin is its opening's top middle). A mesh
    whose signature matches its last bake reuses those textures (NSF_REUSE=0 bakes everything again)."""
    bake = []
    _REUSED.clear()
    reuse = os.environ.get('NSF_REUSE', '1') != '0'
    trust = {t.strip() for t in os.environ.get('NSF_REUSE_TRUST', '').split(',') if t.strip()}
    for o in objs:
        if o.name.endswith('_PaneTmp'):
            o.hide_render = True
        elif o.name.endswith(('_Trim', '_Grille', '_Hour', '_Minute')) or o.name.startswith('SM_NS_Proxy_') or o.name == 'SM_NS_Penny_Front':
            if 'place_at' in o:
                o.location = Vector(o['place_at'])
            o['bake_sig'] = _bake_sig(o)
            if reuse and _try_reuse(o, o.name in trust):
                print(f'[ns_fronts] {o.name}: textures reused from the last bake (signature {o["bake_sig"][:10]})')
                _REUSED.append(o)
                o.hide_render = True
            else:
                bake.append(o)
        elif '_Glow' in o.name or o.name.endswith(('_Glass', '_Marquee', '_Posters', '_Sign', '_Hours')):
            o.hide_render = True
    return bake


def _stem(img):
    return os.path.splitext(os.path.basename(img.filepath or img.name))[0]


def assemble(objs):
    """After the bake: grilles back to the origin; a baked texture that came out one flat value goes out as 4x4
    (facades' rule). Returns every mesh (the unbaked ones too) for export."""
    # Where the pivoted meshes go (grilles, the clock's hands), for the review script (art/build/ns_fronts_places.json).
    import json
    jp = os.path.join(core.BUILD_DIR, 'ns_fronts_places.json')
    places = {}
    if os.path.exists(jp):
        try:
            places = json.load(open(jp))
        except Exception:
            places = {}
    for o in _ALL:
        if 'place_at' in o:
            places[o.name] = list(o['place_at'])
            o.location = (0.0, 0.0, 0.0)
    os.makedirs(core.BUILD_DIR, exist_ok=True)
    json.dump(places, open(jp, 'w'), indent=1)
    for o in objs + _REUSED:
        if 'bake_sig' in o:
            d = os.path.join(core.BUILD_DIR, 'textures', o.name)
            os.makedirs(d, exist_ok=True)
            open(os.path.join(d, 'bake_sig.txt'), 'w').write(o['bake_sig'])
    for o in _REUSED:
        o.hide_render = False
    fac.assemble(objs + _REUSED)
    # The proxies: their panes join the baked atlas as slot 1 (ns_window).
    for tmp in [o for o in _ALL if o.name.endswith('_PaneTmp')]:
        target = bpy.data.objects[tmp['joins']]
        tmp.hide_render = False
        j = core.join([target, tmp], target.name)
        j.name = j.data.name = target['proxy_name'] if 'proxy_name' in target else j.name
        _ALL.remove(tmp)
        print(f'[ns_fronts] {j.name}: slots ' + ', '.join(f'{i} {s_.material.name}' for i, s_ in enumerate(j.material_slots)))
    return [o for o in _ALL if o.name in bpy.data.objects]




# ================================================================== review: the plots in the street, by day and by night

SET_OF = {'ns_brick_red': 'Brick_Red', 'ns_brick_buff': 'Brick_Buff', 'ns_brick_dark': 'Brick_Dark', 'ns_brick_painted': 'Brick_Painted',
          'ns_limestone': 'Limestone', 'ns_caststone': 'CastStone', 'ns_stucco': 'Stucco', 'ns_roof': 'Roof'}
GLOW_HEX = {'Warm': 0xffc489, 'Cool': 0xe6efff, 'Neon': 0xff4a6a}


def _vmath(m, op, a, b=None):
    n = m.node('ShaderNodeVectorMath')
    n.operation = op
    for i, v in enumerate((a, b)):
        if v is None:
            continue
        if isinstance(v, (tuple, list)):
            n.inputs[i].default_value = v
        else:
            m.link(v, n.inputs[i])
    return n.outputs['Value'] if op in ('DOT_PRODUCT', 'LENGTH', 'DISTANCE') else n.outputs['Vector']


def _sep(m, v):
    n = m.node('ShaderNodeSeparateXYZ')
    m.link(v, n.inputs['Vector'])
    return n.outputs['X'], n.outputs['Y'], n.outputs['Z']


def _uvmap(m):
    n = m.node('ShaderNodeUVMap')
    n.uv_map = 'UVMap'
    return n.outputs['UV']


def preview_masonry(slot, night):
    """A masonry slot as M_NS_Masonry would shade it: the tiling set in meters (2 m tile), soot streaks from COLOR_0.R,
    worn arrises from G, and, at night in the rain, wet where B says the rain reaches."""
    st = SET_OF[slot]
    m = core.Mat(f'pv_{slot}_{"n" if night else "d"}')
    uv = _vmath(m, 'MULTIPLY', _uvmap(m), (0.5, 0.5, 1.0))
    base = m.image(os.path.join(TEX_DIR, f'T_NS_{st}_BaseColor.png'), vector=uv)
    orm = m.image(os.path.join(TEX_DIR, f'T_NS_{st}_ORM.png'), non_color=True, vector=uv)
    nrm = m.image(os.path.join(TEX_DIR, f'T_NS_{st}_Normal.png'), non_color=True, vector=uv)
    _, rough_t, _ = _sep(m, orm.outputs['Color'])
    vc = m.node('ShaderNodeVertexColor')
    vc.layer_name = 'Color'
    soot, wear, rain = _sep(m, vc.outputs['Color'])
    tc, sep = object_coords(m)
    streak = noise(m, scaled(m, sep, 5.0, 5.0, 0.25), 1.0, 3.0)
    s_amt = m.math('MULTIPLY', soot, m.math('ADD', 0.35, m.math('MULTIPLY', ramp(m, streak, 0.4, 0.7), 0.65)))
    col = m.mix(m.math('MULTIPLY', s_amt, 0.75), base.outputs['Color'], hx(0x15100c))
    col = m.mix(m.math('MULTIPLY', wear, 0.25), col, fac.cmul(m, col, 1.35))
    rough = rough_t
    if night:
        wet = m.math('MULTIPLY', rain, 0.85)
        col = m.mix(m.math('MULTIPLY', wet, 0.45), col, fac.cmul(m, col, 0.45))
        rough = m.math('MULTIPLY', rough_t, m.math('SUBTRACT', 1.0, m.math('MULTIPLY', wet, 0.6)))
    m.set('Base Color', col)
    m.set('Roughness', rough)
    nm = m.node('ShaderNodeNormalMap', Strength=1.0)
    nm.uv_map = 'UVMap'
    m.link(nrm.outputs['Color'], nm.inputs['Color'])
    m.set('Normal', nm.outputs['Normal'])
    return m


def preview_window(night):
    """[window] as M_NS_WindowRoom (spec 3.4) shades it: the view ray into a box the pane's size and 3.2 m x a hash deep,
    projected back through a pinhole one window-width out (u = 0.5 + x / (1 + z)), the room picked by the hash and the
    kind, its front layer, its lamp by occupancy; the glass over it reflects."""
    m = core.Mat(f'pv_window_{"n" if night else "d"}')
    geo = m.node('ShaderNodeNewGeometry')
    r = _vmath(m, 'SCALE', geo.outputs['Incoming'])
    r.node.inputs['Scale'].default_value = -1.0
    N = geo.outputs['True Normal']
    nx, ny, nz = _sep(m, N)
    t = _vmath(m, 'NORMALIZE', vec(m, m.math('MULTIPLY', ny, -1.0), nx, 0.0))
    rx = _vmath(m, 'DOT_PRODUCT', r, t)
    _, _, ry = _sep(m, r)
    rz = m.math('MAXIMUM', m.math('MULTIPLY', _vmath(m, 'DOT_PRODUCT', r, N), -1.0), 0.002)
    uu, vv, _ = _sep(m, _uvmap(m))
    pane = fac.attr(m, 'pane', 'Vector')
    pw0, ph0, _ = _sep(m, pane)
    pw = mix_float(m, m.math('GREATER_THAN', pw0, 0.01), 1.12, pw0)
    ph = mix_float(m, m.math('GREATER_THAN', ph0, 0.01), 1.73, ph0)
    vc = m.node('ShaderNodeVertexColor')
    vc.layer_name = 'Color'
    hsh, gfl, kind = _sep(m, vc.outputs['Color'])
    asp = m.math('DIVIDE', ph, m.math('MAXIMUM', pw, 0.1))
    h2 = m.math('FRACT', m.math('MULTIPLY', hsh, 7.137))
    h3 = m.math('FRACT', m.math('MULTIPLY', hsh, 13.71))
    h4 = m.math('FRACT', m.math('MULTIPLY', hsh, 29.3))
    dn = m.math('DIVIDE', m.math('MULTIPLY', 3.2, m.math('ADD', 0.8, m.math('MULTIPLY', h2, 0.4))), m.math('MAXIMUM', pw, 0.1))
    x = m.math('SUBTRACT', uu, 0.5)
    y = m.math('MULTIPLY', m.math('SUBTRACT', vv, 0.5), asp)
    rxs = m.math('ADD', rx, m.math('MULTIPLY', m.math('SIGN', rx), 1e-4))
    rys = m.math('ADD', ry, m.math('MULTIPLY', m.math('SIGN', ry), 1e-4))
    tx = m.math('DIVIDE', m.math('SUBTRACT', m.math('MULTIPLY', m.math('SIGN', rxs), 0.5), x), rxs)
    ty = m.math('DIVIDE', m.math('SUBTRACT', m.math('MULTIPLY', m.math('SIGN', rys), m.math('MULTIPLY', asp, 0.5)), y), rys)
    tz = m.math('DIVIDE', dn, rz)
    tt = m.math('MINIMUM', m.math('MINIMUM', tx, ty), tz)
    hxx = m.math('ADD', x, m.math('MULTIPLY', rx, tt))
    hyy = m.math('ADD', y, m.math('MULTIPLY', ry, tt))
    hzz = m.math('MULTIPLY', rz, tt)
    den = m.math('ADD', 1.0, hzz)
    u2 = m.math('ADD', 0.5, m.math('DIVIDE', hxx, den), clamp=True)
    v2 = m.math('ADD', 0.5, m.math('DIVIDE', m.math('DIVIDE', hyy, asp), den), clamp=True)
    # The room: apartments 0-11 by the hash, offices and storage 12-13, a stairwell 14, a shop's back room 15.
    apt = m.math('FLOOR', m.math('MULTIPLY', h3, 11.999))
    off = m.math('ADD', 12.0, m.math('GREATER_THAN', h3, 0.6))
    cell = mix_float(m, m.math('GREATER_THAN', kind, 0.2), apt, off)
    cell = mix_float(m, m.math('GREATER_THAN', kind, 0.5), cell, 14.0)
    cell = mix_float(m, m.math('GREATER_THAN', kind, 0.85), cell, 15.0)
    col_ = m.math('FLOORED_MODULO', cell, 4.0)
    row = m.math('FLOOR', m.math('DIVIDE', cell, 4.0))
    au = m.math('DIVIDE', m.math('ADD', col_, m.math('ADD', 0.002, m.math('MULTIPLY', u2, 0.996))), 4.0)
    av = m.math('SUBTRACT', 1.0, m.math('DIVIDE', m.math('SUBTRACT', m.math('ADD', row, 1.0), m.math('ADD', 0.002, m.math('MULTIPLY', v2, 0.996))), 4.0))
    auv = vec(m, au, av, 0.0)
    alb = m.image(os.path.join(TEX_DIR, 'T_NS_Rooms_Albedo.png'), vector=auv, extension='EXTEND')
    lmp = m.image(os.path.join(TEX_DIR, 'T_NS_Rooms_Lamp.png'), vector=auv, extension='EXTEND')
    fcell = m.math('FLOOR', m.math('MULTIPLY', h4, 15.999))
    fcol = m.math('FLOORED_MODULO', fcell, 4.0)
    frow = m.math('FLOOR', m.math('DIVIDE', fcell, 4.0))
    fu = m.math('DIVIDE', m.math('ADD', fcol, m.math('ADD', 0.002, m.math('MULTIPLY', uu, 0.996))), 4.0)
    fv = m.math('SUBTRACT', 1.0, m.math('DIVIDE', m.math('SUBTRACT', m.math('ADD', frow, 1.0), m.math('ADD', 0.002, m.math('MULTIPLY', vv, 0.996))), 4.0))
    front = m.image(os.path.join(TEX_DIR, 'T_NS_RoomFront.png'), vector=vec(m, fu, fv, 0.0), extension='EXTEND')
    fa = m.math('MULTIPLY', front.outputs['Alpha'], m.math('GREATER_THAN', h2, 0.25))
    if night:
        occ = m.math('ADD', 0.5, m.math('MULTIPLY', m.math('GREATER_THAN', kind, 0.5), 0.5))
        lit = m.math('LESS_THAN', h3, occ) if False else m.math('LESS_THAN', h4, occ)
        tint = m.mix(m.math('GREATER_THAN', hsh, 0.7), (1.0, 0.86, 0.68, 1.0), (0.82, 0.9, 1.0, 1.0))
        tint = m.mix(m.math('GREATER_THAN', hsh, 0.85), tint, (0.55, 0.68, 1.0, 1.0))
        room = m.mix(1.0, lmp.outputs['Color'], tint, blend='MULTIPLY')
        room = fac.cmul(m, room, m.math('MULTIPLY', lit, 2.2))
        room = m.mix(0.5, room, fac.cmul(m, alb.outputs['Color'], 0.012), blend='ADD')
        fr = fac.cmul(m, front.outputs['Color'], m.math('ADD', 0.01, m.math('MULTIPLY', lit, 0.5)))
    else:
        room = fac.cmul(m, alb.outputs['Color'], 0.55)
        fr = fac.cmul(m, front.outputs['Color'], 0.6)
    em = m.mix(fa, room, fr)
    m.set('Base Color', hx(0x05070a))
    m.set('Roughness', 0.04)
    m.set('Specular IOR Level', 0.5)
    m.set('Emission Color', em)
    m.set('Emission Strength', 1.0)
    return m


def preview_glass():
    m = core.Mat('pv_glass')
    nt = m.nt
    for n in list(nt.nodes):
        nt.nodes.remove(n)
    out = nt.nodes.new('ShaderNodeOutputMaterial')
    tr = nt.nodes.new('ShaderNodeBsdfTransparent')
    gl = nt.nodes.new('ShaderNodeBsdfGlossy')
    gl.inputs['Roughness'].default_value = 0.03
    fr = nt.nodes.new('ShaderNodeFresnel')
    fr.inputs['IOR'].default_value = 1.5
    mx = nt.nodes.new('ShaderNodeMixShader')
    nt.links.new(fr.outputs['Fac'], mx.inputs['Fac'])
    nt.links.new(tr.outputs['BSDF'], mx.inputs[1])
    nt.links.new(gl.outputs['BSDF'], mx.inputs[2])
    nt.links.new(mx.outputs['Shader'], out.inputs['Surface'])
    return m


def preview_glow(color_hex, strength):
    m = core.Mat(f'pv_glow_{color_hex:06x}_{strength}')
    m.set('Base Color', hx(_shade(color_hex, 0.6)))
    m.set('Emission Color', hx(color_hex))
    m.set('Emission Strength', strength)
    return m


def _flat(name, hexc, rough):
    m = core.Mat(name)
    tc, sep = object_coords(m)
    n = noise(m, tc.outputs['Object'], 0.6, 4.0)
    m.set('Base Color', m.mix(m.math('MULTIPLY', n, 0.4), hx(hexc), hx(_shade(hexc, 0.75))))
    m.set('Roughness', rough)
    return m


class _Review:
    """The street around the plots: ground, the legacy fronts, art-street's ground where it exists, lamps and sky."""

    def __init__(self, objs):
        self.objs = objs
        self.extra = []
        self.lights = []
        self.saved = {}

    def import_glb(self, name, matrix):
        p = os.path.join(core.EXPORT_DIR, f'{name}.glb')
        if not os.path.exists(p):
            return None
        before = set(bpy.data.objects)
        bpy.ops.import_scene.gltf(filepath=p)
        new = [o for o in bpy.data.objects if o not in before]
        for o in new:
            if o.type == 'MESH' and o.parent is None:
                o.matrix_world = matrix @ o.matrix_world
        self.extra += new
        return new

    def context(self):
        # The importer reuses a node group named 'glTF Material Output' and expects its full set of sockets; artkit's
        # bake makes one with Occlusion only (the export is done by now, so it can step aside).
        g = bpy.data.node_groups.get('glTF Material Output')
        if g is not None:
            g.name = 'glTF Material Output (artkit)'
        # The legacy fronts (StreetStage's Facade(): yaw 180 home side, 0 far side; their origin on their face).
        home = lambda Y: Matrix.Translation((0.0, Y / 100.0, 0.0))  # noqa: E731
        far = lambda Y: Matrix.Translation((-20.4, Y / 100.0, 0.0)) @ Matrix.Rotation(math.pi, 4, 'Z')  # noqa: E731
        self.import_glb('SM_Facade_1812', home(0.0))
        self.import_glb('SM_Facade_Walkup', home(2990.0))
        self.import_glb('SM_Facade_StoreUpper', home(4325.0))
        for Y in (-1812.5, -1137.5):
            self.import_glb('SM_Facade_Brownstone', home(Y))
        for Y in (3057.5, 3732.5, 4407.5):
            self.import_glb('SM_Facade_Brownstone', far(Y))
        self.import_glb('SM_Tenement', far(920.0))
        # The Lucky Penny's storey under its upper floors.
        self.box('penny', 0.0, 14.0, 37.0, 49.5, 0.0, 4.8, _flat('pv_penny', 0x2a2a2c, 0.5))
        # The enterable shops' fronts and rooms where their tracks have exported them (SITE meshes, so in place): the
        # openings this module leaves clear show what fills them. Their glass and glows get the preview materials.
        self.shop_glows = []
        try:
            names = sorted(f[:-4] for f in os.listdir(core.EXPORT_DIR) if f.endswith('.glb'))
        except OSError:
            names = []
        # (their rooms, the _Shell meshes, only with NSF_RV_SHELLS=1: they are heavy on a shared GPU)
        keep = ('_Front', '_Sign', '_Pole', '_Neon', '_Door', '_Banners') + (('_Shell',) if os.environ.get('NSF_RV_SHELLS') == '1' else ())
        for nm in names:
            if not nm.startswith(('SM_NS_Barber_', 'SM_NS_Gym_', 'SM_NS_Pawn_', 'SM_NS_Thrift_', 'SM_NS_Ruby_')):
                continue
            stem = nm.split('_Glow')[0]
            if not any(k in stem for k in keep):
                continue
            new = self.import_glb(nm, Matrix.Identity(4)) or []
            for o in new:
                if o.type != 'MESH':
                    continue
                if nm.endswith('_Glass'):
                    for sl in o.material_slots:
                        sl.material = preview_glass().m
                elif '_Glow' in nm or '_Neon' in nm:
                    self.shop_glows.append((o, nm))
        # The ground: art-street's meshes when built, else sidewalks, curbs and roads.
        got = False
        walk = _flat('pv_walk', 0x8a867e, 0.8)
        road = _flat('pv_road', 0x2a2b2e, 0.7)
        paint = _flat('pv_paint', 0xd8d4c8, 0.55)
        yellow = _flat('pv_yellow', 0xd9a520, 0.55)
        for g in ('SM_NS_Ground_FifthS', 'SM_NS_Ground_FifthN', 'SM_NS_Ground_Market', 'SM_NS_Ground_Cardinal'):
            new = self.import_glb(g, Matrix.Identity(4))
            if not new:
                continue
            got = True
            for o in new:
                if o.type != 'MESH':
                    continue
                for sl in o.material_slots:
                    nm = sl.material.name if sl.material else ''
                    if any(k in nm for k in ('asphalt', 'patch', 'busbay', 'gutter')):
                        sl.material = road.m
                    elif 'marking' in nm:
                        sl.material = paint.m
                    elif 'tactile' in nm:
                        sl.material = yellow.m
                    else:
                        sl.material = walk.m
        if not got:
            self.box('walk_h', -4.2, 0.0, -90.0, 115.0, -0.3, 0.0, walk)
            self.box('walk_f', -20.4, -16.2, -90.0, 115.0, -0.3, 0.0, walk)
            self.box('road_5', -16.2, -4.2, -90.0, 115.0, -0.4, -0.15, road)
            self.box('walk_ms', 0.0, 100.0, 49.5, 54.0, -0.3, 0.0, walk)
            self.box('walk_mn', 0.0, 100.0, 65.0, 69.5, -0.3, 0.0, walk)
            self.box('road_m', 0.0, 100.0, 54.0, 65.0, -0.4, -0.15, road)
            self.box('road_h', -60.0, 60.0, -76.2, -66.2, -0.4, -0.15, road)
            self.box('walk_hn', -60.0, 60.0, -66.2, -62.0, -0.3, 0.0, walk)
            self.box('road_w', -60.0, 60.0, 96.2, 106.2, -0.4, -0.15, road)
            self.box('walk_ws', -60.0, 60.0, 92.0, 96.2, -0.3, 0.0, walk)
            self.box('road_c', 100.2, 110.2, 20.0, 100.0, -0.4, -0.15, road)
        self.ground_mats = [walk, road]
        # Streetlights: art's SM_Streetlight where imported, and their light.
        for (X, Y) in [(360, y) for y in (-5400, -2400, 600, 3600)] + [(1680, y) for y in (-3900, -900, 2100, 5100, 8100)] + \
                [(X_, 5340) for X_ in (-2900, -5650, -8400)] + [(X_, 6560) for X_ in (-1450, -4250, -7050)]:
            self.lamp(S(X, Y, 850.0))

    def box(self, name, x0, x1, y0, y1, z0, z1, mat):
        o = parts.rbox(name, ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), (abs(x1 - x0), abs(y1 - y0), abs(z1 - z0)))
        core.assign(o, mat)
        self.extra.append(o)
        return o

    def lamp(self, at):
        d = bpy.data.lights.new('street_lamp', 'SPOT')
        d.spot_size = math.radians(130)
        d.spot_blend = 0.6
        d.shadow_soft_size = 0.25
        d.energy = 9000.0
        d.color = (1.0, 0.62, 0.3)
        o = core.link(bpy.data.objects.new('street_lamp', d))
        o.location = at
        o.visible_camera = False
        self.lights.append(o)

    def materials(self, night):
        """Preview materials on the plots' unbaked meshes (bodies, glass, glows)."""
        for o in self.objs:
            if o.name.endswith('_Glass'):
                o.data.materials[0] = preview_glass().m
            elif '_Glow' in o.name:
                fam = o.name.split('_Glow')[-1]
                hexc = GLOW_HEX.get(fam, 0xffffff)
                if fam == 'Neon':
                    P = next((p for p, ob in _PLOTS if o in ob), None)
                    hexc = (P.neon if P and P.neon else hexc)
                    ca = o.data.color_attributes.get('Color')
                    if P is None and ca is not None and len(ca.data):
                        c = ca.data[0].color
                        hexc = (int(core.srgb_to_linear(1) and min(255, (c[0] ** (1 / 2.2)) * 255)) << 16) | (int(min(255, (c[1] ** (1 / 2.2)) * 255)) << 8) | int(min(255, (c[2] ** (1 / 2.2)) * 255))
                o.data.materials[0] = preview_glow(hexc, 40.0 if night else 0.0).m
            elif o.name.startswith('SM_NS_') and not o.name.endswith(('_Trim', '_Grille')):
                for i, s in enumerate(o.material_slots):
                    nm = self.saved.setdefault((o.name, i), s.material.name)
                    if nm in SET_OF:
                        s.material = preview_masonry(nm, night).m
                    elif nm == 'ns_window':
                        s.material = preview_window(night).m
        for m in getattr(self, 'ground_mats', []):
            m.set('Roughness', 0.25 if night else 0.8)
        # The shops' glows: their design color from COLOR_0 where they carry one, else by family.
        for o, nm in getattr(self, 'shop_glows', []):
            hexc = 0xffffff
            if 'Warm' in nm:
                hexc = GLOW_HEX['Warm']
            elif 'Cool' in nm:
                hexc = GLOW_HEX['Cool']
            ca = o.data.color_attributes[0] if len(o.data.color_attributes) else None
            if ca is not None and len(ca.data):
                c = ca.data[0].color
                hexc = sum(int(min(255, max(0.0, c[i]) ** (1 / 2.2) * 255)) << (16 - 8 * i) for i in range(3))
            elif 'Neon' in nm:
                hexc = 0xff3a2a if 'Ruby' in nm else 0x4a8aff
            for sl in o.material_slots:
                sl.material = preview_glow(hexc, 40.0 if night else 0.0).m

    def set_time(self, night):
        sc = bpy.context.scene
        world = sc.world
        world.use_nodes = True
        bg = world.node_tree.nodes.get('Background') or world.node_tree.nodes.new('ShaderNodeBackground')
        if not bg.outputs['Background'].is_linked:
            world.node_tree.links.new(bg.outputs['Background'], world.node_tree.nodes.get('World Output').inputs['Surface'])
        if night:
            bg.inputs['Color'].default_value = (0.012, 0.016, 0.03, 1.0)
            bg.inputs['Strength'].default_value = 1.0
        else:
            bg.inputs['Color'].default_value = (0.55, 0.6, 0.68, 1.0)
            bg.inputs['Strength'].default_value = 1.6
        for o in self.lights:
            o.hide_render = not night
        sun = getattr(self, 'sun', None)
        if sun is None:
            d = bpy.data.lights.new('sun', 'SUN')
            d.angle = math.radians(25)
            d.energy = 1.2
            d.color = (1.0, 0.97, 0.92)
            sun = self.sun = core.link(bpy.data.objects.new('sun', d))
            sun.rotation_euler = (math.radians(55), 0.0, math.radians(-35))
        sun.hide_render = night
        # The lit slots and prints glow at night only (DimImportedGlow by day).
        for o in self.objs:
            if o.name.endswith('_Trim'):
                for s in o.material_slots:
                    b = next((n for n in s.material.node_tree.nodes if n.type == 'BSDF_PRINCIPLED'), None)
                    if b is None:
                        continue
                    key = (s.material.name, 'em')
                    if key not in self.saved:
                        self.saved[key] = b.inputs['Emission Strength'].default_value
                    b.inputs['Emission Strength'].default_value = self.saved[key] * (1.0 if night else (0.5 if s.material.name.endswith('_3') else 0.15))
        self.materials(night)
        sc.view_settings.view_transform = 'AgX'
        sc.view_settings.exposure = 1.0 if night else -0.2

    def grilles(self, up):
        for o in self.objs:
            if o.name.endswith('_Grille'):
                o.scale = (1.0, 1.0, 0.03 if up else 1.0)

    def shot(self, label, cam_p, look_at, fov, w=1280, h=720, samples=64):
        sc = bpy.context.scene
        cam = bpy.data.objects.get('nsf_cam')
        if cam is None:
            cd = bpy.data.cameras.new('nsf_cam')
            cam = core.link(bpy.data.objects.new('nsf_cam', cd))
        cam.data.angle = math.radians(fov)
        cam.data.clip_start = 0.05
        cam.data.clip_end = 3000.0
        cam.location = Vector(cam_p)
        d = Vector(look_at) - Vector(cam_p)
        cam.rotation_euler = d.to_track_quat('-Z', 'Y').to_euler()
        sc.camera = cam
        sc.render.resolution_x, sc.render.resolution_y = w, h
        sc.render.resolution_percentage = 100
        sc.cycles.samples = samples
        # A shared GPU: cap the textures Cycles uploads (NSF_RV_TEXLIMIT, default 2048; OFF for none).
        sc.render.use_simplify = True
        sc.render.simplify_subdivision_render = 6
        sc.cycles.texture_limit_render = os.environ.get('NSF_RV_TEXLIMIT', '2048')
        sc.cycles.use_denoising = True
        sc.render.film_transparent = False
        d_ = os.path.join(core.BUILD_DIR, 'review', 'ns_fronts')
        os.makedirs(d_, exist_ok=True)
        p = os.path.join(d_, f'{label}.png')
        sc.render.filepath = p
        sc.render.image_settings.file_format = 'PNG'
        sc.render.image_settings.color_mode = 'RGB'
        bpy.ops.render.render(write_still=True)
        return p

    def cleanup(self):
        # The standard sheet that follows shows the plots in their day previews, without the street.
        self.set_time(False)
        sun = getattr(self, 'sun', None)
        for o in self.extra + self.lights + ([sun] if sun else []):
            try:
                bpy.data.objects.remove(o, do_unlink=True)
            except ReferenceError:
                pass
        self.lights = []
        self.sun = None


def compose(name, paths, cols):
    tag = os.environ.get('NSF_TAG', '').strip()
    if tag:
        name = f'{name}_{tag}'      # a partial build's sheets (NSF_TAG=a) don't overwrite another's
    imgs = [_load_px(p)[:, :, :3] for p in paths]
    h, w = imgs[0].shape[:2]
    rows = (len(imgs) + cols - 1) // cols
    gap = 6
    out = np.full((rows * h + (rows - 1) * gap, cols * w + (cols - 1) * gap, 3), 0.05, np.float32)
    for k, im in enumerate(imgs):
        r, c = k // cols, k % cols
        y0 = out.shape[0] - (r + 1) * h - r * gap
        out[y0:y0 + h, c * (w + gap):c * (w + gap) + w] = im
    return _save_jpg(name, out)


# Where the review stands the proxies (Blender x, y, yaw): HOLLIS-S either side of the bank, WEXLER-N either side of the
# Rialto, C-S and C-N either side of the library, two more down Hollis and Wexler. ue-world places them in the game.
REVIEW_PROXY_AT = {'A': (14.0, -80.4, -90.0), 'B': (-36.4, -80.4, -90.0), 'C': (10.9, 110.4, 90.0), 'D': (-34.4, 110.4, 90.0),
                   'E': (114.4, 36.0, 0.0), 'F': (114.4, 84.0, 0.0), 'G': (-50.0, -62.0, 90.0), 'H': (34.0, 92.0, -90.0)}

# The spec's acceptance shots (9), stage cm: camera, look-at, FOV.
W_SHOTS = {
    'W1': ((240, 60, 170), (700, 9000, 650), 70.0),
    'W2': ((1900, 4600, 170), (200, 6200, 300), 70.0),
    'W3': ((-300, 5200, 170), (-11440, 5950, 600), 60.0),
    'W5': ((300, -2000, 170), (1020, -8040, 800), 55.0),
    'W6': ((1800, 5400, 170), (1020, 11040, 900), 55.0),
    'W8': ((-9900, 6200, 170), (-7800, 4950, 250), 60.0),
}


def plot_shots(P):
    """Per plot: the front from across the street, along its sidewalk at a walker's eye, close at its shop."""
    F = P.faces[P.streets[0]]
    across = 13.0 if F.n.x != 0 else 12.5
    c = F.P(F.L / 2, 0.0, 0.0)
    out = [(f'{P.id}_front', c + F.n * across + Z * 1.7, F.P(F.L / 2, P.top * 0.62, 0.0), 82.0),
           (f'{P.id}_close', F.P(F.L * 0.62, 1.6, -5.2), F.P(F.L * 0.55, 3.0, 0.0), 58.0)]
    if os.environ.get('NSF_SHOTS', '') == 'all':
        out += [(f'{P.id}_upper', c + F.n * (across + 4.0) + F.t * (F.L * 0.3) + Z * 8.0, F.P(F.L / 2, P.top * 0.6, 0.0), 62.0),
                (f'{P.id}_walk', F.P(-1.5, 1.65, -2.6), F.P(F.L * 0.65, 2.6, 0.0), 68.0)]
    return out


def site_review(objs):
    which = {s.strip() for s in os.environ.get('NSF_REVIEW', 'plots,w,tex').split(',') if s.strip()}
    if 'tex' in which:
        try:
            texture_sheet()
        except Exception:
            traceback.print_exc()
    if not ({'plots', 'w'} & which):
        return
    R = _Review(objs)
    try:
        R.context()
        R.grilles(up=True)
        shots = []
        if 'plots' in which:
            for P, _ in _PLOTS:
                shots += plot_shots(P)
        if 'w' in which and len(_PLOTS) > 3:
            for k, (cp, lp, fov) in W_SHOTS.items():
                shots.append((k, S(*cp), S(*lp), fov))
        small = len(_PLOTS) > 4
        for night in (False, True):
            R.set_time(night)
            tag = 'night' if night else 'day'
            pp = [R.shot(f'{lab}_{tag}', cp, lp, fov, *((960, 540, 40) if small else (1280, 720, 64)))
                  for lab, cp, lp, fov in shots if not lab.startswith('W')]
            ww = [R.shot(f'{lab}_{tag}', cp, lp, fov, 1600, 900, 64) for lab, cp, lp, fov in shots if lab.startswith('W')]
            if pp:
                compose(f'ns_fronts_plots_{tag}', pp, 4 if small else 3)
            if ww:
                compose(f'ns_fronts_w_{tag}', ww, 2)
    except Exception:
        traceback.print_exc()
    finally:
        R.cleanup()


def pose_for_review(objs):
    global REVIEW_VIEWS
    for o in objs:
        o.hide_render = False
        if 'place_at' in o:
            o.location = Vector(o['place_at'])
        if o.name.startswith('SM_NS_Proxy_') and o.name[-1] in REVIEW_PROXY_AT:
            x, y, yaw = REVIEW_PROXY_AT[o.name[-1]]
            o.location = (x, y, 0.0)
            o.rotation_euler = (0.0, 0.0, math.radians(yaw))
    # The clock's hands at 10:10 for the review.
    for o in objs:
        if o.name.endswith('_Hour'):
            o.rotation_euler = (0.0, math.radians(-305.0), 0.0)
        elif o.name.endswith('_Minute'):
            o.rotation_euler = (0.0, math.radians(-60.0), 0.0)
    bpy.context.view_layer.update()
    t0 = time.time()
    site_review(objs)
    print(f'[ns_fronts] site review in {time.time() - t0:.0f}s')
    lo, hi = core.bounds(objs)
    size = (hi - lo).length
    P = _PLOTS[0][0] if _PLOTS else None
    if P is not None:
        F = P.faces[P.streets[0]]
        c = F.P(F.L / 2, P.top / 2, 0.0)
        az = math.degrees(math.atan2(F.n.x, -F.n.y))
        c2 = F.P(F.L * 0.5, 2.0, 0.0)
        REVIEW_VIEWS = [('front', az, 4, max(F.L, P.top) * 2.4 / size, tuple(c)),
                        ('angle', az - 35, 10, max(F.L, P.top) * 2.2 / size, tuple(c)),
                        ('street', az + 25, 6, 9.0 / size, tuple(c2))]


# ================================================================== more of the kit: bases, B2 lobbies, B3 commercial, B5 warehouse

def rustication(P, F, u0, u1, z0, z1, course=0.5, block=1.2, slot='ns_caststone', openings=None):
    """A rusticated base: courses of blocks with V-joints (each block chamfered 2 cm), stopping at the openings (whose
    reveals the wall makes). Body masonry (slot), so the tiling set dresses it."""
    sl = BODY_INDEX[slot]
    opens = openings if openings is not None else P.opens[P.faces.index(F)]
    if not hasattr(P, 'rust'):
        P.rust = []
    P.rust.append((P.faces.index(F), u0, u1, z0, z1))
    z = z0
    ci = 0
    while z < z1 - 0.05:
        za, zb = z, min(z + course, z1)
        cuts = sorted((o['u0'], o['u1']) for o in opens if o['z0'] < zb - 1e-4 and o['z1'] > za + 1e-4)
        spans, a = [], u0
        for c0, c1 in cuts:
            if c0 > a + 1e-4:
                spans.append((a, c0))
            a = max(a, c1)
        if a < u1 - 1e-4:
            spans.append((a, u1))
        off = 0.5 * block if ci % 2 else 0.0
        for a, b in spans:
            edges = [a]
            t = u0 - off
            while t < b:
                t += block
                if a + 0.3 < t < b - 0.3:
                    edges.append(t)
            edges.append(b)
            for e0, e1 in zip(edges, edges[1:]):
                F.box(P.body, e0, e1, za, zb, -0.05, 0.0, 0.02, seg=1, skip=((2, 1),), slot=sl, uvm=1, reg=0)
        z = zb
        ci += 1


def b3_front(P, fi, shops, band=None, band_text=None, upper='triple', sh=3.6, band_h=0.75, piers=None, door=None, number=None,
             parapet_panel=None):
    """A B3 two-floor commercial face: storefronts under one continuous sign band (its own text or each shop's), offices
    over them in wide windows, a parapet with coping and a raised name panel."""
    F = P.faces[fi]
    for u0, u1, dress, opts in shops:
        storefront(P, F, u0, u1, sh, dress=DRESS.get(dress), **opts)
    if door is not None:
        res_door(P, F, door, width=1.2, height=2.9, number=number, recess=0.45)
    bz0 = sh + 0.38
    a = min(s[0] for s in shops) - 0.2
    b = max(s[1] for s in shops) + 0.2
    if band:
        P.sign('band', b - a, band_h, band, F.P((a + b) / 2, bz0 + band_h / 2, -0.07), F.n, depth=0.06)
    else:
        F.box(P.trim, a, b, bz0, bz0 + band_h, -0.07, 0.0, 0.006, **P.T(2, 'front'))
    sc = [(0.0, 0.0), (0.06, 0.0), (0.06, 0.05), (0.12, 0.08), (0.12, 0.12), (0.24, 0.16), (0.26, 0.2), (0.32, 0.24), (0.32, 0.3), (0.0, 0.3)]
    pa, pb = F.P(0.0, 0, 0), F.P(F.L, 0, 0)
    P.trim.molding([(pa.x, pa.y), (pb.x, pb.y)], sc, bz0 + band_h + 0.02, **P.T(1, 'cornice'))
    # Offices over: wide triple windows (or pairs), steel or wood.
    bays, bw = bays_for(F.L, 3.2 if upper == 'triple' else 2.2)
    for i, uc in enumerate(bays):
        ww = min(bw - 0.7, 2.6 if upper == 'triple' else 1.2)
        z0 = snap_z(P.fz(2) + 0.8)
        window_unit(P, F, uc, z0, ww, 1.8, panes=(3, 1) if upper == 'triple' else (1, 1), lintel='flat', kind=0.33, ac=P.rnd.random() < 0.15)
    if parapet_panel:
        w, h, draw = parapet_panel
        F.box(P.trim, F.L / 2 - w / 2 - 0.15, F.L / 2 + w / 2 + 0.15, P.top + P.parapet - 0.1, P.top + P.parapet + 0.55, -0.08, 0.3, 0.01, **P.T(0, 'stone'))
        P.sign('panel', w, h, draw, F.P(F.L / 2, P.top + P.parapet + 0.2, -0.081), F.n, depth=0.004, board=False)
    return F


def b2_front(P, fi, door_u, number=None, name=None, base_floors=2, shop=None, canopy='bronze', bays=None, lintel='flat'):
    """A B2 prewar elevator building's face: a rusticated cast-stone base two floors high, the lobby with its canopy, its
    doors and lit hall; brick over it; windows in pairs."""
    F = P.faces[fi]
    res_door(P, F, door_u, width=2.0, height=3.3, recess=0.6, number=number, lobby=True, canopy=canopy, door_kind='bronze')
    if shop:
        u0, u1, dress, opts = shop
        storefront(P, F, u0, u1, 3.4, dress=DRESS.get(dress), **opts)
    if name and canopy:
        cz = 3.3 + 0.35
        P.sign('canopy', 2.0 + 1.2 + 0.2, 0.26, name, F.P(door_u, cz + 0.14, -2.62), F.n, depth=0.004, board=False)
    bays = bays or bays_for(F.L, 1.75)
    walls_and_windows(P, fi, lintel=lintel, panes=(1, 1), bays=bays, floors_from=2, skip={(2, i) for i, u in enumerate(bays[0]) if abs(u - door_u) < 1.6})
    return F


# ================================================================== the plots

def _plot(pid, X0, X1, Y0, Y1, side, **kw):
    poly = rect_poly(X0, X1, Y0, Y1)
    P = Plot(pid, poly, [], **kw)
    fi = street_edge(P.poly, side)
    P.streets = [fi]
    P.faces[fi].kind = 'street'
    return P, fi


def _corner_poly(x0, x1, y0, y1, corner, chamfer=0.0, round_r=0.0, segs=6):
    """A CCW rectangle with one corner ('sw', 'se', 'nw', 'ne' in Blender's x/y) chamfered or rounded."""
    pts = [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]
    idx = {'sw': 0, 'se': 1, 'ne': 2, 'nw': 3}[corner]
    p = Vector(pts[idx])
    a = Vector(pts[idx - 1])
    b = Vector(pts[(idx + 1) % 4])
    da, db = (a - p).normalized(), (b - p).normalized()
    if round_r > 0:
        c = p + da * round_r + db * round_r
        rep = []
        for k in range(segs + 1):
            t = k / segs
            v = (p + da * round_r - c).lerp(p + db * round_r - c, t)
            v = v.normalized() * round_r
            rep.append(tuple(c + v))
    elif chamfer > 0:
        rep = [tuple(p + da * chamfer), tuple(p + db * chamfer)]
    else:
        rep = [tuple(p)]
    out = pts[:idx] + rep + pts[idx + 1:]
    return [(q[0], q[1]) for q in out]


def _faces_facing(P, side, min_len=0.5):
    want = {'w': (-1, 0), 'e': (1, 0), 'n': (0, 1), 's': (0, -1)}[side]
    return [i for i, F in enumerate(P.faces) if F.n.x * want[0] + F.n.y * want[1] > 0.95 and F.L > min_len]


def _finish(P, base=None, cornice_kind='metal', tank=False, bulkhead=True, cornice_fis=None, cornice_h=0.95, cornice_p=0.7):
    fis = cornice_fis if cornice_fis is not None else P.streets
    if cornice_kind:
        main_cornice(P, fis, kind=cornice_kind, height=cornice_h, project=cornice_p)
    if base:
        slot_of = slots_by_height([P.gf if base[1] is None else base[1]], [base[0], P.wall_m])
    else:
        slot_of = None
    finish_walls(P, slot_of)
    roof(P, P.top - 0.2, P.top + P.parapet, tank=tank, bulkhead=bulkhead)
    return P


# ------------------------------------------------------------------ Fifth St, home side

def plot_hhollis():
    """H-HOLLIS: a B4 corner on Hollis: buff brick over a cast-stone base, the corner chamfered for Hollis Hardware's door."""
    poly = _corner_poly(0.0, 18.0, -62.0, -36.0, 'sw', chamfer=1.6)
    P = Plot('HHollis', poly, [], floors=4, gf=4.5, fh=3.25, wall='ns_brick_buff', seed=6100, body='B4', lintel='ns_caststone',
             palette={'front': dict(type='paint', hex=0x7a1a14, under=0x3a2a20, chips=0.9, under_metal=0.3, soot=0.5),
                      'cornice': dict(type='paint', hex=0x5a5446, under=0x3d4246, chips=0.8, soot=0.75, under_metal=0.6)})
    fifth = _faces_facing(P, 'w')[0]
    hollis = _faces_facing(P, 's')[0]
    cham = next(i for i, F in enumerate(P.faces) if abs(F.n.x) > 0.5 and abs(F.n.y) > 0.5)
    P.streets = [fifth, cham, hollis]
    for i in P.streets:
        P.faces[i].kind = 'street'
    Ff, Fc, Fh = P.faces[fifth], P.faces[cham], P.faces[hollis]
    sign = band_art('HOLLIS HARDWARE', 'PAINT · KEYS CUT · PLUMBING · ELECTRICAL · GLASS CUT · SINCE 1946', fg=0xf2e6c8, bg=0x7a1a14, accent=0xe8c060, seed=7)
    res_door(P, Ff, 2.2, number='1800')
    storefront(P, Ff, 3.9, Ff.L - 0.35, 3.4, door=None, dress=dress_hardware, bulk='front', frame='front')
    storefront(P, Fc, 0.12, Fc.L - 0.12, 3.4, door='center', recess=0.6, door_w=0.7, double=True, dress=None, bulk='front', grille=False)
    storefront(P, Fh, 0.35, 9.0, 3.4, door=None, dress=dress_hardware, bulk='front', frame='front', grille=False)
    res_door(P, Fh, 11.5, width=1.1, height=2.6, step=False, number=None, buzzer=False, lamp=True, recess=0.3)
    bz = 3.4 + 0.38 + 0.3
    P.sign('band', Ff.L - 3.9 + 0.1, 0.6, sign, Ff.P((3.9 + Ff.L - 0.35) / 2, bz, -0.07), Ff.n, depth=0.05)
    P.sign('band2', 8.8, 0.6, band_art('HOLLIS HARDWARE', 'HOUSEWARES · LOCKSMITH · 555-0162', fg=0xf2e6c8, bg=0x7a1a14, accent=0xe8c060, seed=8),
           Fh.P(4.7, bz, -0.07), Fh.n, depth=0.05)
    # A double-faced blade sign on the chamfer's corner over the door.
    def blade(s, Wm, Hm):
        s.rect(0, 0, Wm, Hm, 0xe8e0c8, rough=0.4)
        s.rect(20, 20, Wm - 40, Hm - 40, 0x7a1a14, rough=0.4)
        for i, ch in enumerate('HARDWARE'):
            s.text(ch, Wm / 2, Hm - 170 - i * (Hm - 200) / 8, 150, 0xf2e6c8, face='Black', align='CENTER', rough=0.4)
        fac.worn(s, Wm, Hm, 50, 0x3a1a10, 0.4, 3)
    bp = Fc.P(Fc.L / 2, 6.6, -0.48)
    P.sign('blade', 0.62, 2.6, blade, bp + Fc.t * 0.06, Fc.t, depth=0.12)
    P.sign('blade_b', 0.62, 2.6, blade, bp - Fc.t * 0.061, -Fc.t, depth=0.0, board=False)
    for z in (5.4, 7.8):
        P.trim.beam(Fc.P(Fc.L / 2, z, 0.0), Fc.P(Fc.L / 2, z, -0.45), 0.04, 0.04, **P.T(1, 'paintsteel'))
    for fi in (fifth, cham, hollis):
        F = P.faces[fi]
        rustication(P, F, 0.0, F.L, 0.0, P.gf - 0.1, course=0.45, block=1.1)
    walls_and_windows(P, fifth, lintel='flat', panes=(2, 2), bays=bays_for(Ff.L, 2.05))
    walls_and_windows(P, cham, lintel='flat', panes=(1, 1), bays=([Fc.L / 2], Fc.L), win=0.9)
    walls_and_windows(P, hollis, lintel='flat', panes=(2, 2), bays=bays_for(Fh.L, 2.05))
    fire_escape(P, Fh, 11.0, 15.4, [snap_z(P.fz(k) + 0.85) - 0.17 for k in range(2, P.floors + 1)])
    downspout(P, Ff, 0.3, P.top - 0.35)
    # A belt course between the base and the brick.
    pts = [(Ff.a.x, Ff.a.y), (Ff.b.x, Ff.b.y), (Fc.b.x, Fc.b.y), (Fh.b.x, Fh.b.y)]
    P.trim.molding(pts, [(0.0, 0.0), (0.09, 0.0), (0.09, 0.06), (0.06, 0.09), (0.06, 0.16), (0.0, 0.16)], P.gf - 0.1, **P.T(0, 'caststone'))
    return _finish(P, base=('ns_caststone', P.gf - 0.1), cornice_fis=[fifth, cham, hollis])


def plot_hcash():
    """H-CASH: a three-floor walk-up in painted brick (QuikCash Check Cashing moved here from Market): an inset display
    window behind bars with the blue tube round it (on all night), a lit CHECKS CASHED · MONEY ORDERS · BILL PAY lightbox."""
    P, fi = _plot('HCash', 0, -1500, 800, 2300, 'w', floors=3, gf=4.5, fh=3.2, wall='ns_brick_painted', seed=1550, neon=0x2a6bff,
                  palette={'front': dict(type='paint', hex=0x14171c, under=0x4a4038, chips=0.85, under_metal=0.4, soot=0.5),
                           'cornice': dict(type='paint', hex=0x2c3a33, under=0x5d6266, chips=0.85, soot=0.6, under_metal=0.6),
                           'sash': dict(type='paint', hex=0x2c3a33, under=0x6a5a48, chips=0.8, soot=0.5)})
    F = P.faces[fi]
    sign = lightbox_art('QUIKCASH', 'CHECK CASHING', fg=0xc8101a, bg=0xf6d200, stripe=0xc8101a)
    b1_front(P, fi, door_u=13.6, number='1816', shop=(0.45, 12.2, 'checkcash', dict(door='right', bulk='front', recess=1.2)), sign=sign,
             lintel='flat', bays=bays_for(F.L, 2.1), goosenecks=False)
    # The inset window: bars inside the glass, the blue tube round it.
    a, b = 0.5, 10.3
    uu = a + 0.1
    while uu < b:
        F.box(P.trim, uu - 0.009, uu + 0.009, 0.55, 2.75, SF_W + 0.07, SF_W + 0.09, **P.T(1, 'iron'))
        uu += 0.12
    for zz in (0.62, 1.7, 2.7):
        F.box(P.trim, a, b, zz - 0.015, zz + 0.015, SF_W + 0.065, SF_W + 0.095, **P.T(1, 'iron'))
    pts = [F.P(a + 0.1, 0.72, SF_W + 0.05), F.P(b - 0.1, 0.72, SF_W + 0.05), F.P(b - 0.1, 2.62, SF_W + 0.05), F.P(a + 0.1, 2.62, SF_W + 0.05)]
    P.glow['Neon'].path_tube(pts, 0.008, n=6, closed=True, slot=0)
    # The lightbox in the window, its face baked lit.
    P.sign('lightbox', 3.4, 0.55, lightbox_art('CHECKS CASHED', 'MONEY ORDERS · BILL PAY', fg=0xffffff, bg=0x1a4fa8, stripe=0xf6d200),
           F.P(5.4, 2.25, SF_W + 0.22), F.n, depth=0.1, glow=1.3)
    downspout(P, F, F.L - 0.25, P.top - 0.35, kind='copper')
    return _finish(P, cornice_h=0.8, cornice_p=0.55)


def plot_ruby():
    """RUBY: the diner's building above its ground storey (SM_NS_Ruby_Front fills Z 0..430 on both faces and round the
    quarter-round corner, R 2.0): buff brick, the corner rounded all the way up, a stone cornice."""
    poly = _corner_poly(0.0, 15.0, 69.5, 80.0, 'sw', round_r=2.0, segs=6)
    P = Plot('Ruby', poly, [], floors=4, gf=4.3, fh=3.2, wall='ns_brick_buff', seed=7300, body='B4',
             palette={'cornice': dict(type='stone', hex=0xc8bca2, soot=0.8)})
    fifth = _faces_facing(P, 'w')[0]
    market = _faces_facing(P, 's')[0]
    arc = [i for i, F in enumerate(P.faces) if F.L < 1.0]
    order = [fifth] + sorted(arc, key=lambda i: P.faces[i].a.y, reverse=True) + [market]
    P.streets = order
    for i in order:
        P.faces[i].kind = 'street'
        F = P.faces[i]
        # The ground storey is Ruby's own (art-shops-a): clear, its head closed by a soffit 30 cm deep.
        P.opens[i].append(dict(u0=0.0, u1=F.L, z0=0.0, z1=P.gf, depth=0.3, sill=False, jambs=False))
    Ff, Fm = P.faces[fifth], P.faces[market]
    walls_and_windows(P, fifth, lintel='flat', panes=(1, 1), bays=bays_for(Ff.L, 2.1, margin=0.3))
    walls_and_windows(P, market, lintel='flat', panes=(1, 1), bays=bays_for(Fm.L, 2.1, margin=0.3))
    # Brick piers between the corner's arc and the windows, a stone band at the ground storey's head.
    pts = [(P.faces[order[0]].a.x, P.faces[order[0]].a.y)] + [(P.faces[i].b.x, P.faces[i].b.y) for i in order]
    P.trim.molding(pts, [(0.0, 0.0), (0.1, 0.0), (0.1, 0.07), (0.07, 0.1), (0.07, 0.3), (0.0, 0.3)], P.gf, **P.T(0, 'stone'))
    fire_escape(P, Ff, 4.2, 7.9, [snap_z(P.fz(k) + 0.85) - 0.17 for k in range(2, P.floors + 1)], ladder=False)
    P.note('the ground storey (Z 0..430 on both faces and the corner) is left clear for SM_NS_Ruby_Front')
    return _finish(P, cornice_kind='stone', cornice_fis=order, cornice_h=0.8, cornice_p=0.6)


def plot_hwexler():
    """H-WEXLER: a B1 tenement on the corner of Wexler: red brick, Northside Wireless on Fifth, the tenants' door on
    Wexler, a fire escape down the Wexler side."""
    P = Plot('HWexler', rect_poly(0, -1500, 8000, 9200), [], floors=5, gf=4.5, fh=3.15, wall='ns_brick_red', seed=8600,
             palette={'front': dict(type='alu', hex=0x2b2622, metal=0.9),
                      'cornice': dict(type='paint', hex=0x6a2a20, under=0x5d6266, chips=0.8, soot=0.7, under_metal=0.6)})
    fifth = _faces_facing(P, 'w')[0]
    wex = _faces_facing(P, 'n')[0]
    P.streets = [fifth, wex]
    for i in P.streets:
        P.faces[i].kind = 'street'
    Ff, Fw = P.faces[fifth], P.faces[wex]
    sign = lightbox_art('NORTHSIDE WIRELESS', 'PHONE REPAIR · CASES · UNLOCKING · PREPAID · 555-0151', fg=0xffffff, bg=0x1f4fa8, stripe=0xe8e8e8)
    b1_front(P, fifth, shop=(0.6, Ff.L - 0.5, 'wireless', dict(door='left', bulk='front', frame='front', recess=0.9)), sign=sign,
             lintel='hood', bays=bays_for(Ff.L, 2.05), goosenecks=False)
    b1_front(P, wex, door_u=Fw.L - 2.2, number='9', lintel='hood', bays=bays_for(Fw.L, 2.05), fe=(1, 2))
    downspout(P, Fw, 0.3, P.top - 0.35)
    return _finish(P, cornice_fis=[fifth, wex])


# ------------------------------------------------------------------ Fifth St, far side

def plot_fhollis():
    """F-HOLLIS: a B4 corner in dark brick on Hollis; Golden Dragon at the chamfered corner."""
    poly = _corner_poly(-38.4, -20.4, -62.0, -43.0, 'se', chamfer=1.6)
    P = Plot('FHollis', poly, [], floors=4, gf=4.5, fh=3.2, wall='ns_brick_dark', seed=6150, body='B4', neon=0xff3020,
             palette={'front': dict(type='paint', hex=0x8a1410, under=0x3a2a20, chips=0.8, under_metal=0.3, soot=0.4),
                      'cornice': dict(type='paint', hex=0x2a2a28, under=0x5d6266, chips=0.7, soot=0.6, under_metal=0.6),
                      'sash': dict(type='paint', hex=0xb8ae96, under=0x6a5a48, chips=0.8, soot=0.6)})
    fifth = _faces_facing(P, 'e')[0]
    hollis = _faces_facing(P, 's')[0]
    cham = next(i for i, F in enumerate(P.faces) if abs(F.n.x) > 0.5 and abs(F.n.y) > 0.5)
    P.streets = [hollis, cham, fifth]
    for i in P.streets:
        P.faces[i].kind = 'street'
    Fh, Fc, Ff = P.faces[hollis], P.faces[cham], P.faces[fifth]
    storefront(P, Fh, Fh.L - 7.0, Fh.L - 0.3, 3.4, door=None, dress=dress_takeout, bulk='tile', frame='front')
    storefront(P, Fc, 0.12, Fc.L - 0.12, 3.4, door='center', recess=0.5, door_w=0.95, dress=None, bulk='tile', grille=False)
    storefront(P, Ff, 0.3, 6.0, 3.4, door=None, dress=dress_takeout, bulk='tile', frame='front', grille=False)
    res_door(P, Ff, 9.0, number='1801')
    sign = lightbox_art('GOLDEN DRAGON', 'CHINESE FOOD · TAKE OUT · FREE DELIVERY · 555-0128', fg=0xf6d24a, bg=0xb01a12, stripe=0xf6d24a)
    bz = 3.4 + 0.38 + 0.35
    P.sign('band', 6.9, 0.7, sign, Fh.P(Fh.L - 3.6, bz, -0.12), Fh.n, depth=0.12, glow=1.1)
    P.sign('band2', 5.9, 0.7, sign, Ff.P(3.1, bz, -0.12), Ff.n, depth=0.12, glow=1.1)
    neon(P, Ff, 'OPEN', 3.1, 2.3, 0.32, w=SF_W + 0.12, backer=False)
    neon(P, Fh, 'TAKE OUT', Fh.L - 3.6, 2.3, 0.26, w=SF_W + 0.12, backer=False)
    walls_and_windows(P, hollis, lintel='flat', bays=bays_for(Fh.L, 2.1))
    walls_and_windows(P, cham, lintel='flat', bays=([Fc.L / 2], Fc.L), win=0.9)
    walls_and_windows(P, fifth, lintel='flat', bays=bays_for(Ff.L, 2.1))
    fire_escape(P, Ff, 11.4, 15.6, [snap_z(P.fz(k) + 0.85) - 0.17 for k in range(2, P.floors + 1)])
    downspout(P, Ff, Ff.L - 0.3, P.top - 0.35)
    return _finish(P, cornice_fis=[hollis, cham, fifth], cornice_h=0.85, cornice_p=0.6)


def plot_flobby():
    """F-LOBBY: a B2 prewar elevator building: six floors of buff brick on a rusticated base, the lobby lit all night
    under a bronze canopy, a water tank on the roof."""
    P, fi = _plot('FLobby', 2040, 3840, -4300, -2300, 'e', floors=6, gf=4.4, fh=3.1, wall='ns_brick_buff', seed=4300, body='B2',
                  palette={'cornice': dict(type='stone', hex=0xc4b8a0, soot=0.85)})
    F = P.faces[fi]

    def canopy(s, Wm, Hm):
        s.rect(0, 0, Wm, Hm, 0x2a2016, rough=0.4, metal=1.0)
        s.text('1805', Wm / 2, Hm * 0.22, Hm * 0.6, 0xd8b060, face='Medium', align='CENTER', tracking=1.4, metal=1.0, rough=0.3)
    b2_front(P, fi, F.L / 2, number='1805', name=canopy)
    for k in range(2):
        rustication(P, F, 0.0, F.L, 0.0, P.gf + P.fh - 0.1, course=0.48, block=1.25)
    P.trim.molding([(F.a.x, F.a.y), (F.b.x, F.b.y)], [(0.0, 0.0), (0.12, 0.0), (0.12, 0.08), (0.08, 0.12), (0.08, 0.22), (0.0, 0.22)],
                   P.gf + P.fh - 0.1, **P.T(0, 'caststone'))
    downspout(P, F, 0.3, P.top - 0.35)
    return _finish(P, base=('ns_caststone', P.gf + P.fh - 0.1), cornice_kind='stone', tank=True, cornice_h=1.1, cornice_p=0.8)


def plot_fbotanica():
    P, fi = _plot('FBotanica', 2040, 3600, -2300, -880, 'e', floors=5, gf=4.5, fh=3.1, wall='ns_brick_red', seed=1590,
                  palette={'front': dict(type='paint', hex=0x1d3f7a, under=0x4a3a2c, chips=0.9, under_metal=0.3, soot=0.5),
                           'tile': dict(type='tile', hex=0xc8b878, alt=0xa89858, size=0.03),
                           'cornice': dict(type='paint', hex=0x8a7a5a, under=0x5d6266, chips=0.85, soot=0.7, under_metal=0.6)})
    F = P.faces[fi]
    sign = band_art('ORTIZ BOTÁNICA', 'VELAS · HIERBAS · INCIENSO · CONSULTAS · 555-0133', fg=0xf6d24a, bg=0x1d3f7a, accent=0xe8e0c8, seed=21)
    b1_front(P, fi, door_u=F.L - 1.3, number='1809', shop=(0.45, F.L - 2.45, 'botanica', dict(door='center', bulk='tile', recess=0.9)),
             sign=sign, lintel='key', fe=(1, 2), panes=(1, 1), bays=bays_for(F.L, 2.0))
    downspout(P, F, 0.3, P.top - 0.35)
    return _finish(P, cornice_h=0.9)


def plot_frecords():
    """F-RECORDS: a B3 two-floor commercial building: Northside Records under a band, offices over, a parapet."""
    P, fi = _plot('FRecords', 2040, 3540, 4745, 6300, 'e', floors=2, gf=4.7, fh=3.7, frieze=0.9, parapet=1.2, wall='ns_brick_red', seed=5500,
                  body='B3', neon=0xff3a8a, palette={'front': dict(type='paint', hex=0x111111, under=0x5a4a3a, chips=0.85, under_metal=0.4)})
    F = P.faces[fi]
    band = band_art('NORTHSIDE RECORDS', 'NEW & USED VINYL · CDs · TAPES · TURNTABLE REPAIR · WE BUY COLLECTIONS', fg=0xf28a2a, bg=0x111111, accent=0xf28a2a, seed=23)

    def panel(s, Wm, Hm):
        s.rect(0, 0, Wm, Hm, 0xb2a68c, rough=0.8)
        s.text('19   BRANDT BLDG   26', Wm / 2, Hm * 0.25, Hm * 0.5, 0x5a5040, face='Medium', align='CENTER', tracking=1.3, rough=0.8)
    b3_front(P, fi, [(0.4, 9.6, 'records', dict(door='right', bulk='front', recess=1.0))], band=band, door=12.4, number='1829',
             parapet_panel=(3.2, 0.5, panel))
    neon(P, F, 'RECORDS', 4.0, 2.2, 0.36, w=SF_W + 0.14, backer=True)
    downspout(P, F, F.L - 0.25, P.top - 0.35)
    return _finish(P, cornice_kind=None)


def plot_fwexler():
    """F-WEXLER: a B2 prewar corner of seven floors on Wexler: Northside Dental in the base, the lobby under its canopy,
    the garage door on Wexler."""
    P = Plot('FWexler', rect_poly(2040, 3840, 6300, 9200), [], floors=7, gf=4.4, fh=3.1, wall='ns_brick_red', seed=9100, body='B2',
             palette={'cornice': dict(type='stone', hex=0xbab09a, soot=0.85),
                      'front': dict(type='bronze', hex=0x3a2a1c)})
    fifth = _faces_facing(P, 'e')[0]
    wex = _faces_facing(P, 'n')[0]
    P.streets = [fifth, wex]
    for i in P.streets:
        P.faces[i].kind = 'street'
    Ff, Fw = P.faces[fifth], P.faces[wex]

    def canopy(s, Wm, Hm):
        s.rect(0, 0, Wm, Hm, 0x2a2016, rough=0.4, metal=1.0)
        s.text('1901', Wm / 2, Hm * 0.22, Hm * 0.6, 0xd8b060, face='Medium', align='CENTER', tracking=1.4, metal=1.0, rough=0.3)
    b2_front(P, fifth, Ff.L - 7.5, number='1901', name=canopy, shop=(0.45, 12.0, 'dental', dict(door='center', bulk='granite', frame='front')))

    def dental(s, Wm, Hm):
        s.rect(0, 0, Wm, Hm, 0x1a2a2a, rough=0.4)
        s.text('NORTHSIDE DENTAL', Wm / 2, Hm * 0.45, Hm * 0.32, 0xd8b060, face='Medium', align='CENTER', tracking=1.3, metal=1.0, rough=0.3)
        s.text('GENERAL & FAMILY DENTISTRY · 555-0157', Wm / 2, Hm * 0.15, Hm * 0.14, 0xe0dcd0, face='Bold', align='CENTER', tracking=1.2)
    P.sign('band', 11.0, 0.7, dental, Ff.P(6.2, 3.4 + 0.38 + 0.35, -0.07), Ff.n, depth=0.05)
    # The garage on Wexler: a steel roll-up in a stone surround.
    gu0, gu1 = 6.0, 9.2
    P.opens[wex].append(dict(u0=gu0, u1=gu1, z0=0.0, z1=3.2, depth=0.2, sill=False))
    gk = P.T(1, 'paintsteel')
    zz = 0.0
    while zz < 3.2:
        Fw.box(P.trim, gu0, gu1, zz, zz + 0.1, 0.17, 0.2, **gk)
        zz += 0.1
    Fw.box(P.trim, gu0 - 0.3, gu1 + 0.3, 3.2, 3.5, -0.05, 0.0, 0.01, **P.T(0, 'limestone'))
    walls_and_windows(P, wex, lintel='flat', bays=bays_for(Fw.L, 1.8), skip={(None, i) for i in ()})
    for fi in (fifth, wex):
        F = P.faces[fi]
        rustication(P, F, 0.0, F.L, 0.0, P.gf + P.fh - 0.1, course=0.5, block=1.3, slot='ns_limestone')
    downspout(P, Fw, Fw.L - 0.3, P.top - 0.35)
    return _finish(P, base=('ns_limestone', P.gf + P.fh - 0.1), cornice_kind='stone', tank=True, cornice_fis=[fifth, wex], cornice_h=1.15, cornice_p=0.85)


# ------------------------------------------------------------------ Market St, south side (fronts on Y 4950 facing north)

def plot_ms1():
    P, fi = _plot('MS1', -1400, -2400, 2400, 4950, 'n', floors=4, gf=4.5, fh=3.15, wall='ns_brick_red', seed=1900,
                  palette={'front': dict(type='alu', hex=0x9aa0a6, metal=0.9),
                           'cornice': dict(type='paint', hex=0x3a4a3f, under=0x5d6266, chips=0.8, soot=0.7, under_metal=0.6)})
    F = P.faces[fi]
    sign = lightbox_art('NORTHSIDE PHARMACY', 'PRESCRIPTIONS · VITAMINS · PHOTO · OPEN 8 AM – 10 PM', fg=0xffffff, bg=0x1a7a4a, stripe=0xe8e8e8)
    b1_front(P, fi, door_u=F.L - 1.15, number='202', shop=(0.45, F.L - 2.3, 'pharmacy', dict(door='center', double=True, bulk='front', frame='front', recess=0.6)),
             sign=sign, lintel='key', bays=bays_for(F.L, 2.0), goosenecks=False)
    # The green cross over the door, lit.
    cu = F.L / 2
    F.box(P.trim, cu - 0.33, cu + 0.33, 4.85, 5.55, -0.4, -0.05, 0.01, **P.T(1, 'paintsteel'))
    g = P.glow['Cool']
    for a, b, c, d in ((cu - 0.1, cu + 0.1, 4.9, 5.5), (cu - 0.3, cu + 0.3, 5.1, 5.3)):
        F.quad(g, a, b, c, d, -0.405, slot=0)
    downspout(P, F, 0.25, P.top - 0.35)
    return _finish(P)


def plot_ms2():
    """M-S2: a former five-and-dime, two floors under a parapet with the old firm's name in terracotta. The thrift's
    opening (X -2440..-3660, Z 0..400) is left clear for SM_NS_Thrift_Front; the band over it is plain terracotta (the
    thrift's own front carries its painted sign)."""
    P, fi = _plot('MS2', -2400, -3700, 2400, 4950, 'n', floors=2, gf=5.0, fh=3.8, frieze=0.9, parapet=1.4, wall='ns_brick_buff', seed=3050,
                  body='B3', palette={'front': dict(type='enamel', hex=0x2a3a5a, chips=0.4), 'stone': dict(type='terracotta', hex=0xd8c49a)})
    F = P.faces[fi]
    # The opening: stage X -2440..-3660 -> u from the face's start (its east end, X -3700).
    u0, u1 = 0.40, 12.60
    P.opens[fi].append(dict(u0=u0, u1=u1, z0=0.0, z1=4.0, depth=0.3, sill=False))
    # Over the opening a terracotta frieze (the five-and-dime's, in the parapet panel's glaze): the thrift's own front
    # carries its painted sign band (Z 332..400) under it, so the text isn't doubled.
    F.box(P.trim, u0 - 0.2, u1 + 0.2, 4.05, 4.75, -0.07, 0.0, 0.006, **P.T(0, 'stone'))
    F.box(P.trim, u0 - 0.26, u1 + 0.26, 4.69, 4.77, -0.1, 0.0, 0.006, **P.T(0, 'stone'))
    if os.environ.get('NSF_THRIFT_BAND') == '1':
        P.sign('band', u1 - u0 + 0.3, 0.66, band_art('SECOND CHANCE THRIFT', 'St. Agnes Outreach', fg=0xf4efe0, bg=0x2a3a5a, accent=0xe8c060, seed=31),
               F.P((u0 + u1) / 2, 4.4, -0.071), F.n, depth=0.002, board=False)
    sc = [(0.0, 0.0), (0.06, 0.0), (0.06, 0.05), (0.12, 0.08), (0.12, 0.12), (0.24, 0.16), (0.26, 0.2), (0.32, 0.24), (0.32, 0.3), (0.0, 0.3)]
    P.trim.molding([(F.a.x, F.a.y), (F.b.x, F.b.y)], sc, 4.77, **P.T(1, 'cornice'))
    for uc in bays_for(F.L, 3.2)[0]:
        window_unit(P, F, uc, snap_z(P.fz(2) + 0.75), 2.6, 2.0, panes=(3, 2), lintel='flat', kind=0.33)

    def panel(s, Wm, Hm):
        s.rect(0, 0, Wm, Hm, 0xd8c49a, rough=0.5)
        s.rect(30, 30, Wm - 60, Hm - 60, 0xc8b48a, rough=0.5)
        s.text('C. W. MARSH & CO.', Wm / 2, Hm * 0.5, Hm * 0.32, 0x6a3a22, face='Bold', align='CENTER', tracking=1.25, rough=0.5)
        s.text('5¢ · 10¢ · 25¢  STORE  ·  1928', Wm / 2, Hm * 0.17, Hm * 0.17, 0x6a3a22, face='Bold', align='CENTER', tracking=1.3, rough=0.5)
        fac.worn(s, Wm, Hm, 90, 0x6a5a40, 0.35, 31)
    F.box(P.trim, F.L / 2 - 2.3, F.L / 2 + 2.3, P.top + P.parapet - 0.2, P.top + P.parapet + 0.9, -0.06, 0.3, 0.01, **P.T(0, 'stone'))
    P.sign('panel', 4.2, 0.9, panel, F.P(F.L / 2, P.top + P.parapet + 0.35, -0.061), F.n, depth=0.004, board=False)
    P.note('the thrift opening (Y 4950 plane, X -2440..-3660, Z 0..400) is left clear, its band plain for the thrift front')
    downspout(P, F, F.L - 0.2, P.top - 0.3)
    return _finish(P, cornice_kind=None)


def plot_ms3():
    """M-S3: Northside Pawn & Loan's building: four floors of painted brick and a fire escape; the pawn's opening (Y 4950
    plane, X -3760..-4440, Z 0..380) is left clear for SM_NS_Pawn_Front, its band reads NORTHSIDE PAWN & LOAN (the gold
    balls, the tube and the bars are ns_pawn's). Its east party wall, bare over the gym's roof, carries the ghost sign
    MARKET ST. BOTTLING WORKS."""
    P, fi = _plot('MS3', -3700, -4500, 2400, 4950, 'n', floors=4, gf=4.5, fh=3.1, wall='ns_brick_painted', seed=4100,
                  palette={'front': dict(type='paint', hex=0x121316, under=0x4a3a2c, chips=0.85, under_metal=0.4, soot=0.6),
                           'cornice': dict(type='paint', hex=0x2a2e33, under=0x5d6266, chips=0.85, soot=0.7, under_metal=0.6),
                           'sash': dict(type='paint', hex=0x2c3a33, under=0x6a5a48, chips=0.85, soot=0.5)})
    F = P.faces[fi]
    # The opening: stage X -3760..-4440 -> u from the face's start (its east end, X -4500).
    u0, u1, sh = 0.6, 7.4, 3.8
    P.opens[fi].append(dict(u0=u0, u1=u1, z0=0.0, z1=sh, depth=0.3, sill=False))
    band = band_art('NORTHSIDE PAWN & LOAN', 'LOANS · WE BUY GOLD · GUITARS · ELECTRONICS', fg=0xe8c060, bg=0x111214, accent=0xe8c060, gilt=True, seed=12)
    bz = sh + 0.08 + 0.29
    P.sign('band', u1 - u0 + 0.3, 0.58, band, F.P((u0 + u1) / 2, bz, -0.07), F.n, depth=0.06)
    sc = [(0.0, 0.0), (0.05, 0.0), (0.05, 0.04), (0.1, 0.06), (0.1, 0.1), (0.2, 0.14), (0.22, 0.18), (0.28, 0.22), (0.28, 0.27), (0.3, 0.29), (0.3, 0.32), (0.0, 0.32)]
    P.trim.molding([(F.a.x, F.a.y), (F.b.x, F.b.y)], sc, bz + 0.31, **P.T(1, 'cornice'))
    for gu in (u0 + 1.4, u1 - 1.4):
        gooseneck(P, F, gu, bz + 0.42)
    bays = bays_for(F.L, 2.0)
    walls_and_windows(P, fi, lintel='flat', bays=bays)
    P.ladder_floor = bz + 0.75      # the drop ladder hooked up over the band and its cornice
    fire_escape(P, F, bays[0][1] - bays[1] / 2 + 0.1, bays[0][2] + bays[1] / 2 - 0.1, [snap_z(P.fz(k) + 0.85) - 0.17 for k in range(2, P.floors + 1)])
    # The ghost sign on the east party wall, over the gym's roof (M-S4's parapet top is about Z 11.6).
    east = next(i for i, Fx in enumerate(P.faces) if Fx.n.x > 0.9)
    Fe = P.faces[east]

    def ghost(s, Wm, Hm):
        s.rect(0, 0, Wm, Hm, 0x33402e, rough=0.85)
        s.text('MARKET ST.', Wm / 2, Hm * 0.62, Hm * 0.26, 0xdcd2b8, face='Black', align='CENTER', tracking=1.15, rough=0.85)
        s.text('BOTTLING WORKS', Wm / 2, Hm * 0.32, Hm * 0.2, 0xdcd2b8, face='Black', align='CENTER', tracking=1.12, rough=0.85)
        s.text('SODA · SELTZER · GINGER ALE · TEL. 555-0108', Wm / 2, Hm * 0.12, Hm * 0.065, 0xc8b070, face='Bold', align='CENTER', rough=0.85)
        fac.worn(s, Wm, Hm, 700, 0xb9b2a3, 0.75, 52)
    P.sign('ghost', 9.0, 3.1, ghost, Fe.P(Fe.L - 6.0, 13.3, -0.003), Fe.n, depth=0.002, board=False)
    P.note('the pawn opening (Y 4950 plane, X -3760..-4440, Z 0..380) is left clear for SM_NS_Pawn_Front; no grille (ns_pawn has its own)')
    return _finish(P, cornice_h=0.8, cornice_p=0.55)


def plot_ms4():
    """M-S4: IronHouse Gym's building, a B5 brick warehouse of two tall floors: steel multi-light sash over the gym's
    opening (Y 4950 plane, X -4560..-6540, Z 0..450, left clear for SM_NS_Gym_Front, whose channel letters say IRONHOUSE),
    brick pilasters, a plain steel fascia over the opening, a parapet with terracotta coping."""
    P, fi = _plot('MS4', -4500, -6600, 2400, 4950, 'n', floors=2, gf=5.2, fh=4.6, frieze=1.0, parapet=1.0, wall='ns_brick_red', seed=5600,
                  body='B5', palette={'front': dict(type='paint', hex=0x22262a, under=0x5a3a22, chips=0.7, under_metal=0.8)})
    F = P.faces[fi]
    u0, u1 = 0.6, 20.4
    P.opens[fi].append(dict(u0=u0, u1=u1, z0=0.0, z1=4.5, depth=0.3, sill=False))
    F.box(P.trim, u0 - 0.1, u1 + 0.1, 4.55, 5.05, -0.08, 0.0, 0.006, **P.T(2, 'front'))
    bays, bw = bays_for(F.L, 3.5)
    for uc in bays:
        window_unit(P, F, uc, snap_z(P.gf + 0.75), bw - 1.0, 2.9, panes=(4, 4), lintel='flat', steel=True, kind=0.33, sill='stone')
    for i in range(len(bays) + 1):
        uu = i * bw
        F.box(P.body, max(uu - 0.3, 0.0), min(uu + 0.3, F.L), 5.1, P.top, -0.12, 0.0, skip=((2, 1),), slot=BODY_INDEX[P.wall_m], uvm=1, reg=0)
    # A corbelled brick band under the parapet.
    for k in range(int(F.L / 0.4)):
        uu = 0.2 + k * 0.4
        F.box(P.body, uu - 0.1, uu + 0.1, P.top - 0.9, P.top - 0.6, -0.08, 0.0, skip=((2, 1),), slot=BODY_INDEX[P.wall_m], uvm=1, reg=0)
    downspout(P, F, F.L - 0.3, P.top - 0.2)
    P.note('the gym opening (Y 4950 plane, X -4560..-6540, Z 0..450) is left clear for SM_NS_Gym_Front; no grille')
    return _finish(P, cornice_kind=None)


def plot_ms5():
    P, fi = _plot('MS5', -6600, -7700, 2400, 4950, 'n', floors=5, gf=4.5, fh=3.1, wall='ns_brick_red', seed=5000, detail='proxy', neon=0xff2a1a,
                  palette={'front': dict(type='paint', hex=0x1a5a2a, under=0x4a3a2c, chips=0.85, under_metal=0.3)})
    F = P.faces[fi]
    sign = band_art("GINO'S SLICE", 'PIZZA · CALZONES · HEROS · 555-0110', fg=0xffffff, bg=0xb01a14, accent=0x1a7a3a, seed=41)
    b1_front(P, fi, door_u=F.L - 1.0, number='254', shop=(0.4, F.L - 2.1, 'slice', dict(door='right', bulk='tile', recess=0.6)), sign=sign,
             lintel='hood', bays=bays_for(F.L, 2.0), fe=(1, 2))
    neon(P, F, 'SLICE', 3.0, 2.3, 0.34, w=SF_W + 0.14, backer=True)
    return _finish(P)


def plot_ms6():
    """M-S6: a three-floor B5 brick warehouse on the corner of Cardinal, vacant: papered glass, FOR LEASE, posters and
    tags, a night light; a roll-up loading door on Cardinal (Y 3600..4200, its apron in the ground); the ghost sign
    CARDINAL STORAGE & MOVING on the Cardinal wall."""
    poly = rect_poly(-7700, -9600, 3400, 4950)
    P = Plot('MS6', poly, [], floors=3, gf=4.8, fh=4.2, frieze=1.0, parapet=1.0, wall='ns_brick_red', seed=8100, body='B5', detail='proxy',
             palette={'front': dict(type='paint', hex=0x3a3e40, under=0x5a3a22, chips=0.9, under_metal=0.8, soot=0.8)})
    mk = _faces_facing(P, 'n')[0]
    cd = _faces_facing(P, 'e')[0]
    P.streets = [mk, cd]
    for i in P.streets:
        P.faces[i].kind = 'street'
    Fm, Fc = P.faces[mk], P.faces[cd]
    # Market: two papered shop windows and a door; Cardinal: the loading door, a service door.
    storefront(P, Fm, 0.5, 9.0, 3.6, door='right', dress=dress_vacant, bulk='front', lit=False)
    storefront(P, Fm, 10.0, 18.5, 3.6, door=None, dress=dress_vacant, bulk='front', lit=False, grille=False)
    night = (6, 0.4, 0.3)
    for a, b in ((0.55, 6.7), (10.05, 18.45)):
        Fm.quad(P.trim, a, b, 0.58, 3.0, SF_W + 0.08, **P.L('paper', Fm.P((a + b) / 2, 3.2, 1.0), night))
    P.sign('lease', 1.0, 0.7, plain_card(['FOR LEASE', 'WAREHOUSE · RETAIL', '555-0142']), Fm.P(4.0, 1.7, SF_W + 0.06), Fm.n, depth=0.005, board=False)
    gu0, gu1 = 2.0, 8.0
    P.opens[cd].append(dict(u0=gu0, u1=gu1, z0=0.0, z1=4.0, depth=0.25, sill=False))
    zz = 0.0
    while zz < 4.0:
        Fc.box(P.trim, gu0, gu1, zz, zz + 0.1, 0.2, 0.25, **P.T(1, 'galv'))
        zz += 0.1
    Fc.box(P.trim, gu0 - 0.1, gu1 + 0.1, 4.0, 4.45, -0.4, 0.0, 0.01, **P.T(1, 'hood'))
    for F_, fi in ((Fm, mk), (Fc, cd)):
        bays, bw = bays_for(F_.L, 3.6)
        for k in (2, 3):
            for uc in bays:
                window_unit(P, F_, uc, snap_z(P.fz(k) + 0.8), bw - 1.0, 2.6, panes=(4, 3), lintel='flat', steel=True, kind=0.33, sill='stone')
        for i in range(len(bays) + 1):
            uu = i * bw
            F_.box(P.body, max(uu - 0.3, 0.0), min(uu + 0.3, F_.L), P.gf - 0.3, P.top, -0.12, 0.0, skip=((2, 1),), slot=BODY_INDEX[P.wall_m], uvm=1, reg=0)
    # Wheat-pasted posters and tags over the ground floor's piers and the papered glass.

    def posters(s, Wm, Hm):
        r = random.Random(61)
        s.rect(0, 0, Wm, Hm, 0xb8ae96, rough=0.9)
        for i in range(9):
            x = 60 + i * (Wm - 120) / 9
            col = r.choice((0xe8e2d0, 0xf2c230, 0x1a1a1a, 0xc8202c))
            s.rect(x, 200 + r.uniform(0, 300), (Wm - 120) / 9 - 30, 900, col, rough=0.8)
            s.text(r.choice(('FIGHT NIGHT', 'LOST CAT', 'LIVE · SAT', 'ROOM 4 RENT', 'DJ NOCTURNE')), x + (Wm - 120) / 18 - 15, 700, 60,
                   0x111111 if col != 0x1a1a1a else 0xe8e2d0, face='Black', align='CENTER', rough=0.8)
        for text, col, x, y in (('REKS', 0x3fa9f5, Wm * 0.2, 150), ('NSD', 0xf2c230, Wm * 0.6, 120), ('MOSE', 0xe8e8e8, Wm * 0.85, 1300)):
            fac.outlined(s, text, x, y, 220, col, 0x111111, face='Black', width=14, rough=0.55)
    P.sign('posters', 8.0, 1.6, posters, Fm.P(14.25, 1.5, SF_W + 0.02), Fm.n, depth=0.004, board=False)

    def ghost(s, Wm, Hm):
        s.rect(0, 0, Wm, Hm, 0x4a2a20, rough=0.85)
        s.text('CARDINAL', Wm / 2, Hm * 0.6, Hm * 0.28, 0xd8ccb0, face='Black', align='CENTER', tracking=1.1, rough=0.85)
        s.text('STORAGE & MOVING', Wm / 2, Hm * 0.32, Hm * 0.18, 0xd8ccb0, face='Black', align='CENTER', tracking=1.15, rough=0.85)
        s.text('FIREPROOF WAREHOUSE · LOCAL & LONG DISTANCE', Wm / 2, Hm * 0.1, Hm * 0.07, 0xd8ccb0, face='Bold', align='CENTER', rough=0.85)
        fac.worn(s, Wm, Hm, 700, 0x7e3f2e, 0.75, 51)
    P.sign('ghost', 11.0, 3.2, ghost, Fc.P(Fc.L / 2, P.fz(3) + 2.2, -0.003), Fc.n, depth=0.002, board=False)
    P.note('vacant: a night light behind the papered glass')
    return _finish(P, cornice_kind=None, cornice_fis=[])


def plot_mn2():
    P, fi = _plot('MN2', -1500, -2800, 6950, 8600, 's', floors=5, gf=4.5, fh=3.15, wall='ns_brick_red', seed=2150,
                  palette={'front': dict(type='paint', hex=0x163a2a, under=0x4a3a2c, chips=0.8, under_metal=0.3),
                           'cornice': dict(type='paint', hex=0x9a8e74, under=0x5d6266, chips=0.85, soot=0.7, under_metal=0.6)})
    F = P.faces[fi]
    sign = band_art('HALDEN & SONS', 'CLOTHIERS · TAILORING & ALTERATIONS · EST. 1952', fg=0xe8c060, bg=0x163a2a, accent=0xe8c060, gilt=True, seed=61)
    P.ladder_floor = P.gf + 0.05    # the drop ladder hooked up over the storefront cornice, clear of the window
    b1_front(P, fi, door_u=1.2, number='201', shop=(2.4, F.L - 0.45, 'clothier', dict(door='center', bulk='front', frame='front', recess=1.1)),
             sign=sign, lintel='hood', panes=(2, 2), bays=bays_for(F.L, 2.1), fe=(3, 4))
    downspout(P, F, F.L - 0.25, P.top - 0.35)
    return _finish(P)


def plot_mn3():
    P, fi = _plot('MN3', -2800, -3600, 6950, 8600, 's', floors=4, gf=4.5, fh=3.1, wall='ns_brick_painted', seed=3200,
                  palette={'front': dict(type='alu', hex=0xc0c4c8, metal=0.9),
                           'cornice': dict(type='paint', hex=0xe0dcd0, under=0x5d6266, chips=0.8, soot=0.75, under_metal=0.6)})
    F = P.faces[fi]
    sign = lightbox_art('LILY NAILS', 'MANICURE · PEDICURE · WAXING · WALK-INS WELCOME', fg=0xffffff, bg=0xd8487a, stripe=0xf6c8d8)
    b1_front(P, fi, door_u=F.L - 1.0, number='205', shop=(0.4, F.L - 2.1, 'nails', dict(door='left', bulk='front', frame='front', recess=0.5)),
             sign=sign, lintel='flat', bays=bays_for(F.L, 2.0), goosenecks=False)
    return _finish(P, cornice_h=0.75)


def plot_mn4():
    P, fi = _plot('MN4', -3600, -4300, 6950, 8600, 's', floors=4, gf=4.5, fh=3.1, wall='ns_brick_buff', seed=3950, neon=0x2a8aff,
                  palette={'front': dict(type='paint', hex=0x6a1414, under=0x4a3a2c, chips=0.85, under_metal=0.3)})
    F = P.faces[fi]
    sign = lightbox_art('MARKET WINE & LIQUOR', 'BEER · WINE · SPIRITS · LOTTERY · ICE', fg=0xc8101a, bg=0xf4f0e6, stripe=0xc8101a)
    b1_front(P, fi, door_u=0.95, number='209', shop=(2.0, F.L - 0.4, 'liquor', dict(door='right', bulk='front', frame='front', recess=0.6)),
             sign=sign, lintel='soldier', bays=bays_for(F.L, 2.05), goosenecks=False)
    neon(P, F, 'LIQUORS', 3.6, 2.3, 0.25, w=SF_W + 0.14, backer=False)
    return _finish(P, cornice_kind='metal', cornice_h=0.7, cornice_p=0.5)


def plot_mn5():
    P, fi = _plot('MN5', -4300, -6300, 6950, 8600, 's', floors=6, gf=4.4, fh=3.1, wall='ns_brick_buff', seed=5300, body='B2', detail='proxy',
                  palette={'cornice': dict(type='stone', hex=0xc4b8a0, soot=0.85)})
    F = P.faces[fi]

    def canopy(s, Wm, Hm):
        s.rect(0, 0, Wm, Hm, 0x1a1f2a, rough=0.4)
        s.text('THE MARLOWE', Wm / 2, Hm * 0.22, Hm * 0.55, 0xd8b060, face='Medium', align='CENTER', tracking=1.5, metal=1.0, rough=0.3)
    b2_front(P, fi, F.L / 2, number='221', name=canopy, canopy='paint')
    rustication(P, F, 0.0, F.L, 0.0, P.gf + P.fh - 0.1, course=0.5, block=1.3)
    return _finish(P, base=('ns_caststone', P.gf + P.fh - 0.1), cornice_kind='stone', tank=True, cornice_h=1.0, cornice_p=0.75)


def plot_mn6():
    """M-N6: The Anchor: a two-floor bar front in glass block, red neon, a black door."""
    P, fi = _plot('MN6', -6300, -7600, 6950, 8600, 's', floors=2, gf=4.4, fh=3.6, frieze=0.9, parapet=1.0, wall='ns_brick_dark', seed=6950,
                  body='B3', detail='proxy', neon=0xff2a1a, palette={'front': dict(type='paint', hex=0x0e0e10, under=0x3a3a3a, chips=0.6)})
    F = P.faces[fi]
    gb0, gb1 = 0.6, 9.4
    P.opens[fi].append(dict(u0=gb0, u1=gb1, z0=0.6, z1=3.2, depth=0.15))
    lamp = F.P((gb0 + gb1) / 2, 2.5, 1.5)
    F.box(P.trim, gb0, gb1, 0.6, 3.2, 0.1, 0.18, skip=((2, -1),), **P.T(2, 'front'))
    F.quad(P.trim, gb0, gb1, 0.6, 3.2, 0.1, **P.L('glassblock', lamp, (5, 0.5, 0.6)))
    F.box(P.trim, gb0 - 0.04, gb1 + 0.04, 0.55, 0.6, -0.03, 0.15, 0.004, **P.T(0, 'stone'))
    storefront(P, F, 10.2, 12.4, 3.0, door='center', door_w=1.0, recess=0.4, dress=dress_bar, bulk='front', transom=False, grille=False, lit=True, lamp_look='red')
    neon(P, F, 'THE ANCHOR', 5.0, 3.85, 0.42, w=-0.1, backer=True)
    P.sign('plaque', 1.6, 0.3, band_art('COCKTAILS · BEER', None, fg=0xe8e0c8, bg=0x0e0e10, seed=71, wear=0.5), F.P(11.3, 3.35, -0.03), F.n, depth=0.02)
    for uc in bays_for(F.L, 2.6)[0]:
        window_unit(P, F, uc, snap_z(P.fz(2) + 0.8), 1.4, 1.8, lintel='flat', kind=0.0)
    return _finish(P, cornice_kind=None)


def plot_mn7():
    """M-N7: Cardinal Furniture, closing: a B4 corner of four floors on Cardinal; STORE CLOSING banners."""
    poly = _corner_poly(76.0, 96.0, 69.5, 83.5, 'se', chamfer=1.2)
    P = Plot('MN7', poly, [], floors=4, gf=4.5, fh=3.2, wall='ns_brick_red', seed=8700, body='B4', detail='proxy',
             palette={'front': dict(type='paint', hex=0x5a1a14, under=0x3a2a20, chips=0.85, under_metal=0.3)})
    mk = _faces_facing(P, 's')[0]
    cd = _faces_facing(P, 'e')[0]
    cham = next(i for i, F in enumerate(P.faces) if abs(F.n.x) > 0.5 and abs(F.n.y) > 0.5)
    P.streets = [mk, cham, cd]
    for i in P.streets:
        P.faces[i].kind = 'street'
    Fm, Fc, Fd = P.faces[mk], P.faces[cham], P.faces[cd]
    storefront(P, Fm, 0.4, Fm.L - 0.3, 3.6, door=None, dress=dress_furniture, bulk='front')
    storefront(P, Fc, 0.1, Fc.L - 0.1, 3.6, door='center', door_w=0.8, recess=0.4, grille=False, bulk='front')
    storefront(P, Fd, 0.3, 8.0, 3.6, door=None, dress=dress_furniture, bulk='front', grille=False)
    res_door(P, Fd, 10.4, number='300')
    band = band_art('CARDINAL FURNITURE', 'SOFAS · BEDROOM · DINING · SINCE 1961', fg=0xf4efe0, bg=0x5a1a14, accent=0xe8c060, seed=81)
    P.sign('band', Fm.L - 0.5, 0.7, band, Fm.P(Fm.L / 2, 3.6 + 0.38 + 0.35, -0.07), Fm.n, depth=0.05)

    def banner(s, Wm, Hm):
        s.rect(0, 0, Wm, Hm, 0xf6d200, rough=0.8)
        s.text('STORE CLOSING', Wm / 2, Hm * 0.55, Hm * 0.36, 0xc8101a, face='Black', align='CENTER', rough=0.8)
        s.text('EVERYTHING MUST GO · 70% OFF', Wm / 2, Hm * 0.15, Hm * 0.2, 0x111111, face='Black', align='CENTER', rough=0.8)
    for k, uc in enumerate((Fm.L * 0.3, Fm.L * 0.72)):
        P.sign(f'banner{k}', 4.5, 1.1, banner, Fm.P(uc, P.gf + 1.6, -0.03), Fm.n, depth=0.01)
    P.sign('banner2', 4.5, 1.1, banner, Fd.P(5.0, P.gf + 1.6, -0.03), Fd.n, depth=0.01)
    walls_and_windows(P, mk, lintel='key', bays=bays_for(Fm.L, 2.05))
    walls_and_windows(P, cham, lintel='key', bays=([Fc.L / 2], Fc.L), win=0.8)
    walls_and_windows(P, cd, lintel='key', bays=bays_for(Fd.L, 2.05))
    P.note('a sidewalk shed (SM_NS_SidewalkShed, ns_kit) runs along its Market face')
    return _finish(P, cornice_fis=[mk, cham, cd])


PLOTS = {'HHollis': plot_hhollis, 'HKim': plot_hkim, 'HCash': plot_hcash, 'Ruby': plot_ruby, 'HWexler': plot_hwexler,
         'FHollis': plot_fhollis, 'FLobby': plot_flobby, 'FBotanica': plot_fbotanica, 'FRecords': plot_frecords, 'FWexler': plot_fwexler,
         'MS1': plot_ms1, 'MS2': plot_ms2, 'MS3': plot_ms3, 'MS4': plot_ms4, 'MS5': plot_ms5, 'MS6': plot_ms6,
         'MN2': plot_mn2, 'MN3': plot_mn3, 'MN4': plot_mn4, 'MN5': plot_mn5, 'MN6': plot_mn6, 'MN7': plot_mn7}


# ================================================================== the landmarks (B6)

def arch_window(P, F, uc, z0, w, h_rect, kind=0.33, frame='bronze', mullions=2, transom=True, depth=0.45, wall_slot=None, keystone=True,
                grille=False, seg=10):
    """A tall round-headed window: the opening (body), its spandrels and the arch's soffit (body), the pane (ns_window)
    over the whole of it, a frame with mullions and a transom (trim), a keystone; optionally a bronze grille."""
    fi = P.faces.index(F)
    r = w / 2
    u0, u1 = uc - r, uc + r
    zs = z0 + h_rect
    P.opens[fi].append(dict(u0=u0, u1=u1, z0=z0, z1=zs + r, depth=depth))
    sl = BODY_INDEX[wall_slot or P.wall_m]
    pts = [(uc + r * math.cos(math.pi * (1 - k / seg)), zs + r * math.sin(math.pi * (1 - k / seg))) for k in range(seg + 1)]
    # Spandrels: fan quads from the arc to the opening's top corners, in the wall's plane.
    for (ua, za), (ub, zb) in zip(pts, pts[1:]):
        corner_u = u0 if (ua + ub) / 2 < uc else u1
        P.body.poly([F.P(ua, za, 0.0), F.P(ub, zb, 0.0), F.P(corner_u, zs + r, 0.0)], want=F.n, slot=sl, uvm=1, reg=0)
        P.body.poly([F.P(ua, za, 0.0), F.P(ub, zb, 0.0), F.P(ub, zb, depth), F.P(ua, za, depth)],
                    center=F.P(uc, zs, depth / 2), slot=sl, uvm=1, reg=2)
    hsh = P.rnd.random()
    F.quad(P.body, u0, u1, z0, zs + r, depth, uv=[(0, 0), (1, 0), (1, 1), (0, 1)], col=(hsh, (z0 + h_rect / 2) / max(P.top, 1.0), kind, 1.0),
           slot=BODY_INDEX['ns_window'], reg=7, pane=(w, h_rect + r, 0.0))
    fk = P.T(2 if frame != 'bronze' else 1, frame)
    wf = depth - 0.1
    F.box(P.trim, u0, u0 + 0.08, z0, zs, wf, depth - 0.01, **fk)
    F.box(P.trim, u1 - 0.08, u1, z0, zs, wf, depth - 0.01, **fk)
    F.box(P.trim, u0, u1, z0, z0 + 0.1, wf, depth - 0.01, **fk)
    P.trim.path_tube([F.P(u, z, wf + 0.04) for u, z in [(uc + (r - 0.05) * math.cos(math.pi * (1 - k / (seg * 2))), zs + (r - 0.05) * math.sin(math.pi * (1 - k / (seg * 2))))
                                                         for k in range(seg * 2 + 1)]], 0.05, n=6, **fk)
    for i in range(1, mullions + 1):
        uu = u0 + w * i / (mullions + 1)
        F.box(P.trim, uu - 0.03, uu + 0.03, z0, zs + math.sqrt(max(r * r - (uu - uc) ** 2, 0.0)) - 0.04, wf + 0.02, depth - 0.02, **fk)
    if transom:
        F.box(P.trim, u0, u1, zs - 0.06, zs + 0.04, wf + 0.01, depth - 0.02, **fk)
    for j in range(1, int(h_rect / 0.9)):
        zz = z0 + j * 0.9
        F.box(P.trim, u0, u1, zz - 0.015, zz + 0.015, wf + 0.03, depth - 0.02, **fk)
    if keystone:
        F.prism(P.trim, [(-0.06, zs + r - 0.05), (0.0, zs + r - 0.05), (0.0, zs + r + 0.45), (-0.06, zs + r + 0.45)], uc - 0.16, uc + 0.16,
                **P.T(0, 'limestone'))
    if grille:
        gk = P.T(1, 'bronze')
        n = int(w / 0.14)
        for i in range(1, n):
            uu = u0 + w * i / n
            top = zs + math.sqrt(max(r * r - (uu - uc) ** 2, 0.0)) - 0.05
            F.box(P.trim, uu - 0.012, uu + 0.012, z0 + 0.05, min(z0 + 2.4, top), 0.06, 0.09, **gk)
        for zz in (z0 + 0.1, z0 + 1.2, z0 + 2.3):
            F.box(P.trim, u0 + 0.05, u1 - 0.05, zz - 0.02, zz + 0.02, 0.055, 0.095, **gk)
    P.soot.append((F, u0, u1, z0 - 0.1, 2.5, 0.5))


def nn_dir(F):
    return Vector((F.n.x, F.n.y, 0.0))


def tt_dir(F):
    return Vector((F.t.x, F.t.y, 0.0))


def column(P, F, uc, z0, z1, r0, r1=None, flutes=20, engaged=True, base=True, capital='ionic', kind='limestone', w=0.0):
    """A column (engaged half-round on F, or free) from z0 to z1: an Attic base, a fluted shaft with entasis, an Ionic or
    Doric capital. Trim stone."""
    r1 = r1 or r0 * 0.86
    at = P.T(0, kind)
    c = F.P(uc, 0.0, w - (0.0 if engaged else r0 + 0.05))
    cx, cy = c.x, c.y
    out = Vector((F.n.x, F.n.y, 0.0))
    side = Vector((F.t.x, F.t.y, 0.0))
    bh = 0.0
    if base:
        bh = r0 * 0.9
        F.box(P.trim, uc - r0 * 1.35, uc + r0 * 1.35, z0, z0 + r0 * 0.3, -r0 * 1.35 if engaged else -r0 * 2.4, 0.0 if engaged else 0.1, **at)
        prof = [(r0 * 1.25, 0.0), (r0 * 1.3, r0 * 0.12), (r0 * 1.2, r0 * 0.25), (r0 * 1.05, r0 * 0.32), (r0 * 1.12, r0 * 0.45), (r0 * 1.08, r0 * 0.55),
                (r0 * 1.0, r0 * 0.6)]
        a0, a1 = (-math.pi * 0.55, math.pi * 0.55) if engaged else (0.0, 2 * math.pi)
        ang0 = math.atan2(out.y, out.x)
        P.trim.lathe(prof, (cx, cy, z0 + r0 * 0.3), 28, a0=ang0 + a0, a1=ang0 + a1, **at)
    zs0 = z0 + (r0 * 0.9 if base else 0.0)
    zs1 = z1 - (r0 * 1.0 if capital else 0.0)
    rings = 16 if P.detail == 'full' else 8
    nseg = flutes * 3
    ang0 = math.atan2(out.y, out.x)
    span = math.pi * 1.1 if engaged else 2 * math.pi
    grid = []
    for j in range(rings + 1):
        t = j / rings
        z = zs0 + (zs1 - zs0) * t
        rr = r0 + (r1 - r0) * (t ** 1.5)
        row = []
        for k in range(nseg + (1 if engaged else 0)):
            a = ang0 - span / 2 + span * k / nseg
            fl = (k % 3 == 1) * rr * 0.06
            row.append(P.trim.v((cx + (rr - fl) * math.cos(a), cy + (rr - fl) * math.sin(a), z)))
        grid.append(row)
    for j in range(rings):
        for k in range(nseg if not engaged else nseg):
            k1 = (k + 1) % len(grid[j])
            if engaged and k + 1 >= len(grid[j]):
                break
            mid = Vector((cx, cy, (zs0 + zs1) / 2))
            P.trim.f([grid[j][k], grid[j][k1], grid[j + 1][k1], grid[j + 1][k]], center=mid, **at)
    if capital == 'ionic':
        zc = zs1
        P.trim.lathe([(r1, 0.0), (r1 * 1.15, r0 * 0.15), (r1 * 1.2, r0 * 0.3), (r1 * 1.05, r0 * 0.42)], (cx, cy, zc), 28,
                     a0=ang0 - span / 2, a1=ang0 + span / 2, **at)
        F.box(P.trim, uc - r0 * 1.4, uc + r0 * 1.4, zc + r0 * 0.42, zc + r0 * 0.62, -r0 * 1.35 if engaged else -r0 * 2.4, 0.0, 0.01, **at)
        F.box(P.trim, uc - r0 * 1.25, uc + r0 * 1.25, zc + r0 * 0.62, zc + r0 * 1.0, -r0 * 1.25 if engaged else -r0 * 2.3, 0.0, 0.01, **at)
        # The volutes: a scrolled disc each side, faced to the street.
        ax = Matrix((side, Z, out)).transposed()
        for s in (-1, 1):
            vc = c + side * (s * r0 * 1.15) + Z * (zc + r0 * 0.5) + out * (r0 * 0.55 if engaged else 0.0)
            P.trim.lathe([(0.0, -r0 * 0.25), (r0 * 0.36, -r0 * 0.25), (r0 * 0.4, -r0 * 0.18), (r0 * 0.4, r0 * 0.2), (r0 * 0.3, r0 * 0.26), (r0 * 0.12, r0 * 0.2),
                          (0.0, r0 * 0.22)], tuple(vc), 24, axis=ax, **at)
            if P.detail == 'full':
                # The scroll: a fillet spiralling in two and a half turns from the disc's rim to its eye, standing proud
                # of the face; it turns down and out from the canalis (clockwise on the right volute, mirrored left).
                face = vc + out * (r0 * 0.235)
                pts_ = []
                n_ = 64
                for k_ in range(n_ + 1):
                    t_ = k_ / n_
                    th = math.pi / 2 + s * (2 * math.pi * 2.5 * t_)
                    rr_ = r0 * (0.37 - 0.29 * t_)
                    pts_.append(face + side * (s * -rr_ * math.cos(th) * -1.0) + Z * (rr_ * math.sin(th)))
                P.trim.path_tube(pts_, r0 * 0.028, n=6, **at)
                P.trim.lathe([(0.0, 0.0), (r0 * 0.07, 0.0), (r0 * 0.07, r0 * 0.03), (0.0, r0 * 0.05)], tuple(face), 12, axis=ax, **at)
        # Egg-and-dart round the echinus's front half: eggs in their shells, a dart between each.
        if P.detail == 'full':
            ne = 13
            for k_ in range(ne):
                a = ang0 - span * 0.42 + span * 0.84 * (k_ + 0.5) / ne
                rad = Vector((math.cos(a), math.sin(a), 0.0))
                tan_ = Vector((-math.sin(a), math.cos(a), 0.0))
                ec = Vector((cx, cy, zc + r0 * 0.24)) + rad * (r1 * 1.16)
                P.trim.obox(ec, (tan_, Z, rad), (r0 * 0.075, r0 * 0.11, r0 * 0.05), r0 * 0.04, seg=2, **at)
                dc = Vector((cx, cy, zc + r0 * 0.2)) + rad * (r1 * 1.17)
                a2 = a + span * 0.42 / ne
                rad2 = Vector((math.cos(a2), math.sin(a2), 0.0))
                dc = Vector((cx, cy, zc + r0 * 0.2)) + rad2 * (r1 * 1.17)
                P.trim.obox(dc, (Vector((-math.sin(a2), math.cos(a2), 0.0)), Z, rad2), (r0 * 0.012, r0 * 0.09, r0 * 0.03), 0.0, seg=1, **at)
    elif capital == 'doric':
        zc = zs1
        P.trim.lathe([(r1, 0.0), (r1 * 1.2, r0 * 0.3), (r1 * 1.2, r0 * 0.45)], (cx, cy, zc), 28, a0=ang0 - span / 2, a1=ang0 + span / 2, **at)
        F.box(P.trim, uc - r0 * 1.3, uc + r0 * 1.3, zc + r0 * 0.45, zc + r0 * 0.8, -r0 * 1.3 if engaged else -r0 * 2.3, 0.0, 0.01, **at)


def entablature(P, fis, z, frieze_h=0.75, arch_h=0.75, corn_h=0.85, project=0.9, kind='limestone', path=None):
    """Architrave (three fasciae), the frieze (its face left for the lettering), a modillioned cornice, along fis (or
    along path [(x, y)], outward on the right: a break-forward round a pavilion)."""
    pts = [tuple(q) for q in path] if path else []
    for fi in ([] if path else fis):
        F = P.faces[fi]
        if not pts:
            pts.append((F.a.x, F.a.y))
        pts.append((F.b.x, F.b.y))
    at = P.T(0, kind)
    a = arch_h
    P.trim.molding(pts, [(0.0, 0.0), (0.06, 0.0), (0.06, a * 0.3), (0.09, a * 0.32), (0.09, a * 0.62), (0.12, a * 0.64), (0.12, a * 0.88),
                         (0.18, a * 0.94), (0.18, a), (0.0, a)], z, **at)
    P.trim.molding(pts, [(0.0, 0.0), (0.1, 0.0), (0.1, frieze_h), (0.0, frieze_h)], z + a, **at)
    cornice(P, pts, z + a + frieze_h, corn_h, project, slot=0, kind=kind)
    return z + a, z + a + frieze_h


def carved_lettering(text, sub=None, stone=0xbdb4a2, size=0.62, tracking=1.45, seed=91):
    """Letters cut into stone: a stone ground (mottled), each letter its shadowed upper-left wall, lit lower-right, dark
    floor: a V-cut seen in daylight."""
    def draw(s, Wm, Hm):
        r = random.Random(seed)
        s.rect(0, 0, Wm, Hm, stone, rough=0.8)
        for _ in range(int(Wm * Hm / 9000)):
            x, y = r.uniform(0, Wm), r.uniform(0, Hm)
            rr = r.uniform(4, 40)
            s.circle(x, y, rr, _shade(stone, r.uniform(0.9, 1.06)), rough=0.8, n=12, alpha=0.5)
        sz = Hm * size
        y = Hm * (0.42 if sub else 0.24)
        for dx, dy, col in ((-sz * 0.025, sz * 0.025, _shade(stone, 0.55)), (sz * 0.02, -sz * 0.02, _tint(stone, 0.35)), (0.0, 0.0, _shade(stone, 0.72))):
            s.text(text, Wm / 2 + dx, y + dy, sz, col, face='Medium', align='CENTER', tracking=tracking, rough=0.85)
        if sub:
            sz2 = Hm * 0.2
            for dx, dy, col in ((-sz2 * 0.03, sz2 * 0.03, _shade(stone, 0.55)), (0.0, 0.0, _shade(stone, 0.72))):
                s.text(sub, Wm / 2 + dx, Hm * 0.1 + dy, sz2, col, face='Medium', align='CENTER', tracking=tracking + 0.1, rough=0.85)
        fac.worn(s, Wm, Hm, 120, 0x3a342a, 0.25, seed)
    return draw


def plot_bank():
    """SM_NS_Vista_Bank: Hollis Savings Bank, 1911, closing the south end of Fifth across Hollis: a limestone temple front
    of four engaged Ionic columns on a granite plinth, the entablature with the name cut in its frieze, an attic carrying
    the clock (its hands SM_NS_Vista_Bank_Hour / _Minute), bronze doors and window grilles, the lit ATM vestibule in the
    east wing, four floodlights."""
    poly = rect_poly(-180, 2220, -11040, -8040)
    P = Plot('VistaBank', poly, [], floors=3, gf=5.0, fh=4.5, frieze=0.0, parapet=0.0, wall='ns_limestone', seed=1911, body='B6',
             lintel='ns_limestone', palette={'stone': dict(type='stone', hex=0xc4bba8, soot=0.45), 'limestone': dict(type='stone', hex=0xc2b8a4, soot=0.5),
                                             'front': dict(type='bronze', hex=0x3a2a1c), 'door': dict(type='bronze', hex=0x34261a)})
    P.name = 'SM_NS_Vista_Bank'
    P.p = 'bank'
    fi = street_edge(P.poly, 'n')
    P.streets = [fi]
    F = P.faces[fi]
    F.kind = 'street'
    P.top = 17.4
    P.parapet = 0.3
    L = F.L
    plinth = 1.0
    cols = [5.25, 9.75, 14.25, 18.75]
    # The plinth and the steps up to the portico (between the outer columns).
    F.box(P.trim, 0.0, L, 0.0, plinth, -0.75, 0.0, 0.02, seg=1, skip=((2, 1),), **P.T(0, 'granite'))
    for k in range(5):
        h = plinth * (k + 1) / 5
        F.box(P.trim, cols[0] - 0.7, cols[-1] + 0.7, 0.0, h, -0.75 - (5 - k) * 0.3, -0.74, 0.01, seg=1, **P.T(0, 'granite'))
    # The wings: rusticated, a tall arched window each with its grille; the east wing's ground has the ATM vestibule.
    arch_window(P, F, 21.4, 2.6, 2.2, 5.4, kind=0.33, frame='bronze', grille=True)
    arch_window(P, F, 2.6, 4.8, 2.2, 3.4, kind=0.33, frame='bronze', grille=False)
    # ATM vestibule (east wing, at the sidewalk): a bronze frame, glass, the lit room behind.
    au0, au1 = 1.4, 3.8
    P.opens[fi].append(dict(u0=au0, u1=au1, z0=0.0, z1=3.3, depth=0.45, sill=False))
    fk = P.T(1, 'bronze')
    for uu in (au0 + 0.05, (au0 + au1) / 2, au1 - 0.05):
        F.box(P.trim, uu - 0.05, uu + 0.05, 0.0, 3.3, 0.3, 0.42, **fk)
    F.box(P.trim, au0, au1, 2.55, 2.65, 0.3, 0.42, **fk)
    F.box(P.trim, au0, au1, 3.2, 3.3, 0.3, 0.42, **fk)
    F.quad(P.glass, au0 + 0.1, au1 - 0.1, 0.02, 3.2, 0.36, slot=0)
    lamp, rm = display_room(P, F, au0, au1, 0.45, 3.2, 0.0, 3.3, 'cool', power=1.4, floor='terrazzo', wall='marble')
    for k, uu in enumerate((au0 + 0.6, au1 - 0.6)):
        lbox(P, F, uu - 0.35, uu + 0.35, 0.0, 1.7, 2.75, 3.2, 'chrome', lamp, rm)
        lbox(P, F, uu - 0.22, uu + 0.22, 1.1, 1.45, 2.73, 2.75, 'tvwall', lamp, rm)
    P.trim.lathe([(0.0, -0.05), (0.3, -0.03), (0.32, 0.0), (0.0, 0.0)], tuple(F.P((au0 + au1) / 2, 3.3, 1.8)), 16, **P.L('fixture', lamp, rm))

    def atm(s, Wm, Hm):
        s.rect(0, 0, Wm, Hm, 0x1a2a3a, rough=0.4)
        s.text('ATM · 24 HOURS', Wm / 2, Hm * 0.3, Hm * 0.42, 0xf0ece0, face='Bold', align='CENTER', tracking=1.3, rough=0.4)
    P.sign('atm', 1.6, 0.26, atm, F.P((au0 + au1) / 2, 2.93, 0.25), F.n, depth=0.02, glow=1.5)
    # The portico: three bays between the columns: two-storey windows with bronze spandrels either side of the doors.
    for a, b in ((cols[0], cols[1]), (cols[2], cols[3])):
        uc = (a + b) / 2
        wu0, wu1 = uc - 1.3, uc + 1.3
        P.opens[fi].append(dict(u0=wu0, u1=wu1, z0=plinth + 1.2, z1=10.0, depth=0.5))
        hsh = P.rnd.random()
        F.quad(P.body, wu0, wu1, plinth + 1.2, 10.0, 0.5, uv=[(0, 0), (1, 0), (1, 1), (0, 1)], col=(hsh, 0.35, 0.33, 1.0), slot=BODY_INDEX['ns_window'], reg=7,
               pane=(2.6, 10.0 - plinth - 1.2, 0.0))
        for uu in (wu0 + 0.06, uc, wu1 - 0.06):
            F.box(P.trim, uu - 0.06, uu + 0.06, plinth + 1.2, 10.0, 0.32, 0.48, **fk)
        for zz in (plinth + 1.25, 5.6, 6.4, 9.95):
            F.box(P.trim, wu0, wu1, zz - 0.05, zz + 0.05, 0.32, 0.48, **fk)
        F.box(P.trim, wu0, wu1, 5.6, 6.4, 0.34, 0.47, **P.T(1, 'bronze'))
        for k in range(4):
            F.box(P.trim, wu0 + 0.2 + k * 0.58, wu0 + 0.6 + k * 0.58, 5.7, 6.3, 0.33, 0.34, 0.01, **P.T(1, 'bronze'))
    # Bronze lanterns on scrolled brackets either side of the doors.
    for uu in (10.675, 13.325):
        lc = F.P(uu, 4.6, -0.45)
        P.trim.path_tube([F.P(uu, 4.25, 0.0), F.P(uu, 4.3, -0.25), F.P(uu, 4.55, -0.42)], 0.02, n=8, **P.T(1, 'bronze'))
        P.trim.lathe([(0.0, -0.3), (0.06, -0.3), (0.13, -0.18), (0.15, 0.12), (0.18, 0.16), (0.12, 0.26), (0.03, 0.34), (0.0, 0.42)], tuple(lc + Z * 0.3), 8,
                     **P.T(1, 'bronze'))
        P.glow['Warm'].lathe([(0.0, -0.16), (0.11, -0.14), (0.125, 0.1), (0.0, 0.12)], tuple(lc + Z * 0.3), 8, slot=0)
    # The doors: bronze, panelled, a grilled transom over them, a cartouche with 1911 above.
    du0, du1 = 10.9, 13.1
    P.opens[fi].append(dict(u0=du0, u1=du1, z0=plinth, z1=8.6, depth=0.6, sill=False))
    dk = P.T(2, 'door')
    for k in range(2):
        la = du0 + k * 1.1
        F.box(P.trim, la + 0.02, la + 1.08, plinth, plinth + 4.6, 0.5, 0.58, 0.004, **dk)
        for j in range(3):
            F.box(P.trim, la + 0.15, la + 0.95, plinth + 0.3 + j * 1.45, plinth + 1.5 + j * 1.45, 0.48, 0.5, 0.01, **dk)
        F.box(P.trim, la + 0.45, la + 0.65, plinth + 1.9, plinth + 2.2, 0.45, 0.48, 0.01, **P.T(2, 'door'))
        for j in range(3):
            P.trim.lathe([(0.0, 0.0), (0.09, 0.0), (0.1, 0.02), (0.06, 0.05), (0.0, 0.06)], tuple(F.P(la + 0.55, plinth + 0.9 + j * 1.45, 0.48)), 12,
                         axis=Matrix((Vector((F.t.x, F.t.y, 0)), Z, Vector((F.n.x, F.n.y, 0)))).transposed(), **P.T(2, 'door'))
    F.box(P.trim, du0, du1, plinth + 4.6, plinth + 4.75, 0.45, 0.6, **dk)
    for k in range(9):
        uu = du0 + 0.15 + k * 0.237
        F.box(P.trim, uu - 0.02, uu + 0.02, plinth + 4.75, 8.55, 0.5, 0.54, **fk)
    F.quad(P.glass, du0, du1, plinth + 4.75, 8.55, 0.57, slot=0)
    F.quad(P.trim, du0, du1, plinth + 4.75, 8.55, 0.7, **P.L('wall', F.P(12.0, 7.5, 3.0), (7, 0.4, 0.5)))
    lk = P.T(0, 'limestone')
    F.box(P.trim, du0 - 0.45, du0, plinth, 9.0, -0.12, 0.0, 0.01, skip=((2, 1),), **lk)
    F.box(P.trim, du1, du1 + 0.45, plinth, 9.0, -0.12, 0.0, 0.01, skip=((2, 1),), **lk)
    F.box(P.trim, du0 - 0.45, du1 + 0.45, 8.6, 9.0, -0.12, 0.0, 0.01, skip=((2, 1),), **lk)

    def plaque(lines, seed):
        def draw(s, Wm, Hm):
            s.rect(0, 0, Wm, Hm, 0x4a3420, rough=0.35, metal=1.0)
            s.rect(Wm * 0.04, Hm * 0.04, Wm * 0.92, Hm * 0.92, 0x5c4228, rough=0.3, metal=1.0)
            s.rect(Wm * 0.07, Hm * 0.07, Wm * 0.86, Hm * 0.86, 0x3a2816, rough=0.5, metal=1.0)
            n = len(lines)
            for i, (t, sz, face) in enumerate(lines):
                y = Hm * (0.86 - (i + 0.75) * 0.78 / n)
                s.text(t, Wm / 2 + Hm * 0.004, y - Hm * 0.004, Hm * sz, 0x22160c, face=face, align='CENTER', tracking=1.15, rough=0.5, metal=1.0)
                s.text(t, Wm / 2, y, Hm * sz, 0xc8a46a, face=face, align='CENTER', tracking=1.15, rough=0.25, metal=1.0)
            fac.worn(s, Wm, Hm, 60, 0x2a5a4a, 0.22, seed)
        return draw
    P.sign('charter', 0.34, 0.46, plaque([('HOLLIS', 0.1, 'Bold'), ('SAVINGS', 0.1, 'Bold'), ('BANK', 0.1, 'Bold'), ('CHARTERED', 0.055, 'Medium'),
                                          ('MCMXI', 0.075, 'Medium')], 7), F.P(10.675, plinth + 1.55, -0.125), F.n, depth=0.012, board=False)
    P.sign('hours', 0.34, 0.46, plaque([('BANKING', 0.085, 'Bold'), ('HOURS', 0.085, 'Bold'), ('MON - FRI', 0.06, 'Medium'), ('9 AM - 4 PM', 0.06, 'Medium'),
                                        ('SAT 9 - 12', 0.06, 'Medium'), ('ATM 24 HRS', 0.055, 'Medium')], 8), F.P(13.325, plinth + 1.55, -0.125), F.n,
           depth=0.012, board=False)
    cz = 9.6
    P.trim.lathe([(0.0, 0.0), (0.55, 0.0), (0.6, 0.06), (0.6, 0.12), (0.0, 0.14)], tuple(F.P(12.0, cz, -0.02)), 24,
                 axis=Matrix((Vector((F.t.x, F.t.y, 0)), Z, Vector((F.n.x, F.n.y, 0)))).transposed(), **P.T(0, 'limestone'))

    def cartouche(s, Wm, Hm):
        s.circle(Wm / 2, Hm / 2, Wm / 2, 0xc2b8a4, rough=0.8)
        for dx, dy, col in ((-6, 6, 0x7a7262), (0, 0, 0x9a9282)):
            s.text('1911', Wm / 2 + dx, Hm * 0.32 + dy, Hm * 0.36, col, face='Medium', align='CENTER', tracking=1.2, rough=0.8)
    P.sign('cartouche', 1.0, 1.0, cartouche, F.P(12.0, cz, -0.165), F.n, depth=0.002, board=False)
    # Rustication on the wings (to the entablature) and plain ashlar faces behind the columns.
    rustication(P, F, 0.0, 4.2, plinth, 11.5, course=0.6, block=1.4, slot='ns_limestone')
    rustication(P, F, 19.8, L, plinth, 11.5, course=0.6, block=1.4, slot='ns_limestone')
    for uc in cols:
        column(P, F, uc, plinth, 11.5, 0.48, flutes=20, engaged=True, capital='ionic')
    # Pilasters at the wings' outer corners.
    for uc in (0.35, L - 0.35):
        F.box(P.trim, uc - 0.35, uc + 0.35, plinth, 11.5, -0.18, 0.0, 0.01, seg=1, **P.T(0, 'limestone'))
    fz0, fz1 = entablature(P, [fi], 11.5, frieze_h=0.8, arch_h=0.7, corn_h=0.9, project=1.0)
    P.sign('frieze', 18.0, 0.78, carved_lettering('HOLLIS SAVINGS BANK', None, stone=0xc2b8a4, size=0.74, tracking=1.5),
           F.P(L / 2, (fz0 + fz1) / 2, -0.101), F.n, depth=0.002, board=False)
    # The attic: panels, the clock in its wreathed surround over the doors, a balustraded parapet.
    az0 = fz1 + 0.9
    clock_c = F.P(12.0, az0 + 1.55, -0.08)
    P.trim.lathe([(1.1, 0.0), (1.24, 0.0), (1.24, 0.14), (1.18, 0.2), (1.1, 0.2), (1.1, 0.0)], tuple(F.P(12.0, az0 + 1.55, 0.0)), 48,
                 axis=Matrix((Vector((F.t.x, F.t.y, 0)), Z, Vector((F.n.x, F.n.y, 0)))).transposed(), **P.T(1, 'bronze'))

    def dial(s, Wm, Hm):
        cx, cy, R = Wm / 2, Hm / 2, Wm / 2
        s.circle(cx, cy, R, 0x2a2016, rough=0.4, metal=1.0)
        s.circle(cx, cy, R * 0.94, 0xece6d4, rough=0.35)
        s.ring(cx, cy, R * 0.78, R * 0.8, 0x1a1a1a, rough=0.4)
        s.ring(cx, cy, R * 0.86, R * 0.88, 0x1a1a1a, rough=0.4)
        for m_ in range(60):
            a = math.pi / 2 - 2 * math.pi * m_ / 60
            l0 = R * (0.8 if m_ % 5 == 0 else 0.84)
            s.rect(cx + math.cos(a) * l0, cy + math.sin(a) * l0, R * (0.08 if m_ % 5 == 0 else 0.03), R * (0.018 if m_ % 5 == 0 else 0.01), 0x1a1a1a,
                   rot=a, rough=0.4)
        romans = ['XII', 'I', 'II', 'III', 'IIII', 'V', 'VI', 'VII', 'VIII', 'IX', 'X', 'XI']
        for h, rn in enumerate(romans):
            # Radial numerals, their feet toward the center, each centered on its radius.
            a = math.pi / 2 - 2 * math.pi * h / 12
            cap = R * 0.15 * 0.71
            rr = R * 0.66 - cap / 2
            s.text(rn, cx + math.cos(a) * rr, cy + math.sin(a) * rr, R * 0.15, 0x1a1a1a, face='Medium', align='CENTER', rot=a - math.pi / 2,
                   rough=0.4)
        s.text('HOLLIS', cx, cy + R * 0.28, R * 0.08, 0x3a3a3a, face='Medium', align='CENTER', tracking=1.4, rough=0.4)
    P.sign('dial', 2.24, 2.24, dial, clock_c, F.n, depth=0.002, board=False, glow=0.35, px=1536)
    for s_ in (-1, 1):
        P.trim.lathe([(0.0, -0.18), (0.32, -0.12), (0.4, 0.0), (0.32, 0.12), (0.0, 0.18)], tuple(F.P(12.0 + s_ * 1.95, az0 + 1.55, -0.1)), 16,
                     axis=Matrix((Vector((F.t.x, F.t.y, 0)), Z, Vector((F.n.x, F.n.y, 0)))).transposed(), **P.T(0, 'limestone'))
    # A carved laurel wreath round the dial: overlapping leaves in pairs, ribbons tied at the bottom.
    ax_d = Matrix((Vector((F.t.x, F.t.y, 0)), Z, Vector((F.n.x, F.n.y, 0)))).transposed()
    lk_ = P.T(0, 'limestone')
    for k in range(44):
        a = math.pi / 2 + 2 * math.pi * k / 44
        if abs(math.sin(a) + 1.0) < 0.08:
            continue
        for side in (-1, 1):
            r_ = 1.38 + side * 0.06
            c = F.P(12.0 + math.cos(a) * r_, az0 + 1.55 + math.sin(a) * r_, -0.03)
            leaf_ax = (Vector((F.t.x, F.t.y, 0)) * -math.sin(a + side * 0.5) + Z * math.cos(a + side * 0.5)).normalized()
            P.trim.obox(c, (leaf_ax, Vector((F.n.x, F.n.y, 0)).cross(leaf_ax).normalized(), Vector((F.n.x, F.n.y, 0))), (0.11, 0.035, 0.03), 0.015, seg=1, **lk_)
    for s_ in (-1, 1):
        P.trim.path_tube([F.P(12.0, az0 + 0.16, -0.025), F.P(12.0 + s_ * 0.2, az0 + 0.06, -0.025), F.P(12.0 + s_ * 0.32, az0 + 0.03, -0.02)], 0.028, n=6, **lk_)
    for k in range(4):
        uu = [2.0, 6.5, 17.5, 22.0][k]
        F.box(P.trim, uu - 1.2, uu + 1.2, az0 + 0.5, az0 + 2.4, -0.06, 0.0, 0.01, seg=1, **P.T(0, 'limestone'))
    P.trim.molding([(F.a.x, F.a.y), (F.b.x, F.b.y)], [(0.0, 0.0), (0.12, 0.0), (0.12, 0.08), (0.08, 0.12), (0.08, 0.25), (0.15, 0.3), (0.15, 0.4), (0.0, 0.4)],
                   P.top - 0.4, **P.T(0, 'limestone'))
    # A balustrade over the attic: pedestals at the ends and over the columns, a base and a rail, vase balusters.
    bz0 = P.top
    ped = [0.35, cols[0], cols[1], 12.0, cols[2], cols[3], L - 0.35]
    lkb = P.T(0, 'limestone')
    for uu in ped:
        F.box(P.trim, uu - 0.32, uu + 0.32, bz0, bz0 + 1.05, -0.06, 0.5, 0.012, seg=1, **lkb)
        F.box(P.trim, uu - 0.38, uu + 0.38, bz0 + 1.05, bz0 + 1.17, -0.12, 0.56, 0.015, seg=1, **lkb)
    bal_prof = [(0.0, 0.0), (0.07, 0.0), (0.07, 0.05), (0.05, 0.08), (0.055, 0.12), (0.09, 0.25), (0.085, 0.33), (0.05, 0.46), (0.04, 0.5),
                (0.055, 0.53), (0.06, 0.56), (0.07, 0.6), (0.0, 0.62)]
    for a_, b_ in zip(ped, ped[1:]):
        ua_, ub_ = a_ + 0.32, b_ - 0.32
        if ub_ - ua_ < 0.4:
            continue
        F.box(P.trim, ua_, ub_, bz0, bz0 + 0.2, 0.0, 0.42, 0.01, seg=1, **lkb)
        F.box(P.trim, ua_, ub_, bz0 + 0.82, bz0 + 0.98, -0.03, 0.45, 0.012, seg=1, **lkb)
        n_b = max(1, int((ub_ - ua_) / 0.3))
        for k in range(n_b):
            uu = ua_ + (ub_ - ua_) * (k + 0.5) / n_b
            P.trim.lathe(bal_prof, tuple(F.P(uu, bz0 + 0.2, 0.21)), 12, **lkb)
    # Festoons in the attic's panels: a swag of laurel hung from two rosettes.
    for k in range(4):
        uu = [2.0, 6.5, 17.5, 22.0][k]
        for s_ in (-1, 1):
            P.trim.lathe([(0.0, 0.0), (0.09, 0.0), (0.1, 0.03), (0.06, 0.06), (0.0, 0.065)], tuple(F.P(uu + s_ * 0.85, az0 + 2.05, -0.06)), 12,
                         axis=ax_d, **lkb)
        sw = [F.P(uu + 0.85 * math.cos(math.pi * t_ / 10.0), az0 + 2.05 - 0.55 * math.sin(math.pi * t_ / 10.0), -0.07) for t_ in range(11)]
        P.trim.path_tube(sw, 0.055, n=8, **lkb)
        for t_ in range(1, 10):
            c_ = sw[t_]
            P.trim.obox(c_ + nn_dir(F) * 0.035, (tt_dir(F), Z, nn_dir(F)), (0.07, 0.035, 0.03), 0.015, seg=1, **lkb)
    # The steps: granite cheek blocks at their ends, and two bronze handrails down them.
    for uu in (cols[0] - 0.7, cols[-1] + 0.7):
        F.prism(P.trim, [(-0.74, 0.0), (-0.74, plinth + 0.15), (-2.25, 0.35), (-2.25, 0.0)], uu - 0.3, uu + 0.3, **P.T(0, 'granite'))
    for uu in (10.2, 13.8):
        top = F.P(uu, plinth + 0.95, -0.95)
        bot = F.P(uu, 0.95, -0.75 - 5 * 0.3 + 0.1)
        P.trim.path_tube([F.P(uu, plinth + 0.95, -0.6), top, bot, F.P(uu, 0.95, -0.75 - 5 * 0.3 - 0.25), F.P(uu, 0.75, -0.75 - 5 * 0.3 - 0.33)],
                         0.022, n=8, **P.T(1, 'bronze'))
        for t_ in (0.0, 0.5, 1.0):
            pp = top.lerp(bot, t_)
            zf = plinth * (1.0 - t_) + 0.0 * t_
            P.trim.tube(pp, Vector((pp.x, pp.y, zf + (0.0 if t_ < 1.0 else 0.0))), 0.018, n=8, **P.T(1, 'bronze'))
    # Floodlights on the plinth, aimed up at the front.
    nn_ = Vector((F.n.x, F.n.y, 0.0))
    tt_ = Vector((F.t.x, F.t.y, 0.0))
    aim = (Z * 1.0 - nn_ * 0.55).normalized()
    side_ = aim.cross(tt_).normalized()
    for uu in (3.0, 7.0, 17.0, 21.0):
        F.box(P.trim, uu - 0.18, uu + 0.18, plinth, plinth + 0.06, -0.7, -0.4, 0.01, **P.T(1, 'paintsteel'))
        hc = F.P(uu, plinth + 0.25, -0.55)
        P.trim.obox(hc, (tt_, side_, aim), (0.2, 0.16, 0.13), 0.02, seg=1, **P.T(1, 'paintsteel'))
        P.trim.beam(F.P(uu, plinth + 0.05, -0.55), hc, 0.04, 0.04, **P.T(1, 'paintsteel'))
        lc = hc + aim * 0.131
        P.glow['Warm'].poly([lc - tt_ * 0.17 - side_ * 0.13, lc + tt_ * 0.17 - side_ * 0.13, lc + tt_ * 0.17 + side_ * 0.13, lc - tt_ * 0.17 + side_ * 0.13],
                            want=aim, slot=0)
    # The side and rear walls: limestone for the first 2.5 m round each corner, brick behind.
    P.heights = {i: P.top for i in range(len(P.faces))}
    slot_lime = slots_by_height([], ['ns_limestone'])
    slot_brick = slots_by_height([], ['ns_brick_buff'])
    for i, Fs in enumerate(P.faces):
        if i == fi:
            wall(P.body, Fs, 0.0, Fs.L, 0.0, P.top, P.opens[i], slot_lime, reg=0)
            continue
        # Which end touches the front?
        near_b = (Fs.b - F.a).length < 0.01 or (Fs.b - F.b).length < 0.01
        near_a = (Fs.a - F.a).length < 0.01 or (Fs.a - F.b).length < 0.01
        if near_b:
            wall(P.body, Fs, Fs.L - 2.5, Fs.L, 0.0, P.top, [], slot_lime, reg=0)
            wall(P.body, Fs, 0.0, Fs.L - 2.5, 0.0, P.top - 1.5, [], slot_brick, reg=0)
        elif near_a:
            wall(P.body, Fs, 0.0, 2.5, 0.0, P.top, [], slot_lime, reg=0)
            wall(P.body, Fs, 2.5, Fs.L, 0.0, P.top - 1.5, [], slot_brick, reg=0)
        else:
            wall(P.body, Fs, 0.0, Fs.L, 0.0, P.top - 1.5, [], slot_brick, reg=0)
    P.body.poly([Vector((x, y, P.top - 1.6)) for x, y in offset_poly(P.poly, 0.3)], want=Z, slot=BODY_INDEX['ns_roof'], uvm=1, reg=5)
    # The clock's hands (their own meshes, origin at the pivot; built pointing to 12).
    P.hands = []
    for nm, length, width, tail in (('Hour', 0.62, 0.075, 0.16), ('Minute', 0.92, 0.05, 0.2)):
        mb = MB()
        tt, up, nn = Vector((F.t.x, F.t.y, 0.0)), Z, Vector((F.n.x, F.n.y, 0.0))
        pts = [(-width * 0.5, -tail), (width * 0.5, -tail), (width * 0.7, 0.0), (width * 0.3, length * 0.82), (width * 0.9, length * 0.86),
               (0.0, length), (-width * 0.9, length * 0.86), (-width * 0.3, length * 0.82), (-width * 0.7, 0.0)]
        th = 0.02
        front = [mb.v(tt * x + up * z + nn * th) for x, z in pts]
        back = [mb.v(tt * x + up * z) for x, z in pts]
        mb.f(front, want=nn)
        mb.f(list(reversed(back)), want=-nn)
        k = len(pts)
        for i in range(k):
            mb.f([back[i], back[(i + 1) % k], front[(i + 1) % k], front[i]], center=up * (length / 2) + nn * (th / 2))
        mb.lathe([(0.0, 0.0), (0.06, 0.0), (0.06, 0.03), (0.0, 0.035)], (0.0, 0.0, 0.0), 16,
                 axis=Matrix((tt, up, nn)).transposed())
        o = mb.obj(f'SM_NS_Vista_Bank_{nm}')
        m = core.Mat(f'bank_{nm.lower()}')
        tc, sep = object_coords(m)
        m.set('Base Color', hx(0x141210))
        m.set('Metallic', 0.5)
        m.set('Roughness', 0.35)
        o.data.materials.append(m.m)
        core.uv_layout(o, [(lambda f: True, 'box', None, (0.0, 0.0, 1.0, 1.0))])
        TEXTURE_SIZES[m.m.name] = 128
        # In front of the dial face, 2 and 4 cm out (the minute hand over the hour hand).
        o['pivot'] = tuple(clock_c + nn * (0.01 if nm == 'Hour' else 0.035))
        P.hands.append(o)
    pv = stage_of(clock_c)
    P.note(f'clock: hands SM_NS_Vista_Bank_Hour / _Minute, origin at the pivot, built pointing at 12 and facing north; place at stage '
           f'({pv[0]:.0f}, {pv[1] + 1:.0f}/{pv[1] + 3.5:.0f}, {pv[2]:.0f}) cm and turn clockwise as seen from the north (about stage +Y)')
    return P


def plot_rialto():
    """SM_NS_Vista_Rialto: the Rialto, an Art Deco movie house closing the north end of Fifth across Wexler: buff brick
    and terracotta piers, the vertical blade sign RIALTO (porcelain enamel, neon, chaser bulbs), a V-shaped marquee with
    chasers and letter boards (_Marquee [screen]), the boxed-in ticket booth, poster cases (_Posters [screen]). Proxy
    detail first (spec 13.9)."""
    poly = rect_poly(-80, 2120, 11040, 14040)
    P = Plot('VistaRialto', poly, [], floors=4, gf=5.6, fh=3.6, frieze=0.0, parapet=0.0, wall='ns_brick_buff', seed=1929, body='B6',
             detail='proxy', neon=0xff3020, palette={'stone': dict(type='terracotta', hex=0xd8c49a, soot=0.3),
                                                     'front': dict(type='enamel', hex=0x8a1a14, chips=0.4),
                                                     'porcelain': dict(type='enamel', hex=0xe8dfc8, chips=0.3)})
    P.name = 'SM_NS_Vista_Rialto'
    P.p = 'rialto'
    fi = street_edge(P.poly, 's')
    P.streets = [fi]
    F = P.faces[fi]
    F.kind = 'street'
    L = F.L
    P.top = 17.0
    P.parapet = 0.0
    tk = P.T(0, 'stone')
    # Ground: six doors under the marquee (closed, posters behind their glass), the ticket booth in the middle, the
    # poster cases on the outer piers.
    P.opens[fi].append(dict(u0=4.0, u1=18.0, z0=0.0, z1=3.6, depth=0.9, sill=False))
    dk = P.T(2, 'front')
    for a in (4.4, 6.3, 8.2, 12.8, 14.7, 16.6):
        F.box(P.trim, a, a + 1.0, 0.0, 2.5, 0.78, 0.86, 0.004, **dk)
        F.quad(P.glass, a + 0.12, a + 0.88, 0.9, 2.3, 0.77, slot=0)
        F.quad(P.trim, a + 0.12, a + 0.88, 0.9, 2.3, 0.88, **P.L('poster', F.P(a + 0.5, 2.4, 1.5), (6, 0.5, 0.6)))
    F.box(P.trim, 4.0, 18.0, 2.5, 3.6, 0.8, 0.9, **P.T(2, 'front'))
    # The booth: an octagon of enamel and glass, boarded.
    bc = F.P(11.0, 0.0, -0.4)
    P.trim.lathe([(0.0, 0.0), (1.0, 0.0), (1.0, 1.0), (0.95, 1.05), (0.95, 2.3), (1.05, 2.4), (1.05, 2.6), (0.0, 2.6)], tuple(bc), 8, **P.T(2, 'front'))
    P.trim.lathe([(0.96, 1.1), (0.96, 2.25)], tuple(bc), 8, **P.T(2, 'plywood'))
    # Terracotta piers (fluted, stepped crowns) between the bays, up to the parapet.
    piers = [0.0, 4.0, 18.0, L - 0.0]
    for k, uu in enumerate((0.6, 3.6, 18.4, L - 0.6)):
        F.box(P.trim, uu - 0.6, uu + 0.6, 0.0, P.top + 1.2, -0.25, 0.0, 0.01, seg=1, **tk)
        for j in range(3):
            F.box(P.trim, uu - 0.6 + j * 0.42, uu - 0.6 + j * 0.42 + 0.24, 0.6, P.top + 0.8, -0.3, -0.25, **tk)
        F.box(P.trim, uu - 0.7, uu + 0.7, P.top + 1.2, P.top + 1.6, -0.3, 0.0, 0.01, **tk)
        F.box(P.trim, uu - 0.45, uu + 0.45, P.top + 1.6, P.top + 2.2, -0.2, 0.0, 0.01, **tk)
    # Over the marquee: a great window to the lobby stair, chevron spandrels, a stepped parapet.
    P.opens[fi].append(dict(u0=6.0, u1=16.0, z0=7.0, z1=14.4, depth=0.5))
    F.quad(P.body, 6.0, 16.0, 7.0, 14.4, 0.5, uv=[(0, 0), (1, 0), (1, 1), (0, 1)], col=(0.31, 0.6, 0.66, 1.0), slot=BODY_INDEX['ns_window'], reg=7,
           pane=(10.0, 7.4, 0.0))
    fk = P.T(1, 'paintsteel')
    for k in range(1, 8):
        uu = 6.0 + k * 1.25
        F.box(P.trim, uu - 0.07, uu + 0.07, 7.0, 14.4, 0.3, 0.48, **fk)
    for zz in (7.0, 9.4, 11.8, 14.4):
        F.box(P.trim, 6.0, 16.0, zz - 0.06, zz + 0.06, 0.3, 0.48, **fk)
    for k in range(10):
        uu = 6.0 + k
        F.prism(P.trim, [(-0.12, 14.6), (0.0, 14.6), (0.0, 15.4), (-0.12, 15.4)], uu + 0.05, uu + 0.95, **tk)
        F.prism(P.trim, [(-0.18, 15.0), (-0.12, 15.0), (-0.12, 15.2), (-0.18, 15.2)], uu + 0.35, uu + 0.65, **tk)
    for zz, w0 in ((P.top, 6.5), (P.top + 0.7, 4.5), (P.top + 1.4, 2.5)):
        F.box(P.trim, 11.0 - w0, 11.0 + w0, zz, zz + 0.7, -0.15, 0.3, 0.01, seg=1, **tk)
    # The marquee: a V in plan out over the sidewalk, its letter boards on both faces (_Marquee, [screen]), chaser bulbs.
    mz0, mz1 = 3.9, 5.5
    tip = F.P(11.0, 0.0, -4.2)
    left, right = F.P(4.6, 0.0, 0.0), F.P(17.4, 0.0, 0.0)
    mk = P.T(2, 'front')
    screen = MB()
    bulbs = P.glow['Warm']
    phase = 0
    for a, b in ((left, tip), (tip, right)):
        d = (b - a)
        Lm = d.length
        t = d.normalized()
        nrm = Vector((t.y, -t.x, 0.0))
        if nrm.dot(Vector((F.n.x, F.n.y, 0))) < 0:
            nrm = -nrm
        Fm = Face((a.x, a.y), (b.x, b.y)) if nrm.dot(Face((a.x, a.y), (b.x, b.y)).n) > 0 else None
        Fm = Fm or Face((b.x, b.y), (a.x, a.y))
        Fm.box(P.trim, 0.0, Fm.L, mz0, mz1, 0.0, 0.25, 0.01, **mk)
        Fm.quad(screen, 0.25, Fm.L - 0.25, mz0 + 0.18, mz1 - 0.18, -0.005, uv=[(0, 0), (1, 0), (1, 1), (0, 1)], slot=0)
        for zz in (mz0 + 0.08, mz1 - 0.08):
            n = int((Fm.L - 0.3) / 0.25)
            for k in range(n + 1):
                uu = 0.15 + (Fm.L - 0.3) * k / n
                c = Fm.P(uu, zz, -0.03)
                bulbs.lathe([(0.0, -0.04), (0.035, -0.02), (0.035, 0.02), (0.0, 0.04)], tuple(c), 6, axis=Matrix((Fm.t, Z.cross(Fm.t), Z)).transposed(),
                            slot=0, uvall=((phase % 64) / 64.0, 0.25))
                phase += 1
    # The soffit with its grid of bulbs, the marquee's top.
    tri = [left, tip, right]
    P.trim.poly([p + Z * mz0 for p in tri], want=-Z, **mk)
    P.trim.poly([p + Z * mz1 for p in tri], want=Z, **mk)
    for k in range(5):
        for j in range(int(12 - k * 2.2)):
            t1 = (k + 0.5) / 5.5
            p = left.lerp(right, 0.5) + (tip - left.lerp(right, 0.5)) * t1
            sp = (right - left).normalized()
            q = p + sp * ((j - (12 - k * 2.2) / 2) * 0.55) + Z * (mz0 - 0.03)
            bulbs.lathe([(0.0, 0.0), (0.05, 0.02), (0.0, 0.05)], tuple(q - Z * 0.05), 6, slot=0, uvall=(((j + k) % 8) / 8.0, 0.5))
    # The blade: a tall double-faced enamel sign on brackets, RIALTO down both faces, neon round each letter, chasers on
    # its outer edge.
    bz0, bz1 = 6.4, 21.5
    bw = 2.2
    bu = 11.0
    blade = Face((F.P(bu, 0, 0).x, F.P(bu, 0, 0).y), (F.P(bu, 0, -bw).x, F.P(bu, 0, -bw).y))
    # blade's faces: its own normal is along F.t
    bc0 = F.P(bu, 0.0, -0.15)
    bt = -Vector((F.n.x, F.n.y, 0.0))
    bn = Vector((F.t.x, F.t.y, 0.0))
    P.trim.obox(bc0 + Vector((F.n.x, F.n.y, 0)) * (bw / 2) + Z * ((bz0 + bz1) / 2), (bt, Z, bn), (bw / 2, (bz1 - bz0) / 2, 0.22), 0.04, seg=1,
                **P.T(2, 'porcelain'))

    def blade_art(s, Wm, Hm):
        s.rect(0, 0, Wm, Hm, 0x8a1a14, rough=0.3)
        s.rect(Wm * 0.06, Wm * 0.06, Wm * 0.88, Hm - Wm * 0.12, 0xe8dfc8, rough=0.3)
        s.rect(Wm * 0.1, Wm * 0.1, Wm * 0.8, Hm - Wm * 0.2, 0x8a1a14, rough=0.3)
        for i, ch in enumerate('RIALTO'):
            y = Hm - Wm * 0.2 - (i + 0.85) * (Hm - Wm * 0.4) / 6
            s.text(ch, Wm / 2, y, Wm * 0.62, 0xf2e6c8, face='Black', align='CENTER', rough=0.3)
        fac.worn(s, Wm, Hm, 80, 0x3a0a08, 0.3, 4)
    Wb, Hb = bw - 0.1, bz1 - bz0 - 0.1
    for sgn in (1, -1):
        c = bc0 + Vector((F.n.x, F.n.y, 0)) * (bw / 2) + Z * ((bz0 + bz1) / 2) + bn * (0.221 * sgn)
        P.sign(f'blade{sgn}', Wb, Hb, blade_art, c, bn * sgn, depth=0.0, board=False, px=1024)
        # Neon round each letter, centered on the printed one.
        for i, ch in enumerate('RIALTO'):
            sz = Wb * 0.62
            base_y = Hb - Wb * 0.2 - (i + 0.85) * (Hb - Wb * 0.4) / 6
            zc = (bz0 + bz1) / 2 - Hb / 2 + base_y + 0.711 * sz / 2
            cc = bc0 + Vector((F.n.x, F.n.y, 0)) * (bw / 2) + Z * zc + bn * (0.27 * sgn)
            loops = text_outline_loops(ch, sz, 'Black')
            xs = [x for lp in loops for x, y in lp]
            ys = [y for lp in loops for x, y in lp]
            mx, my = (min(xs) + max(xs)) / 2, (min(ys) + max(ys)) / 2
            hor = bt if sgn > 0 else -bt
            for lp in loops:
                pts = [cc + hor * (x - mx) + Z * (y - my) for x, y in lp]
                keep = [pts[0]]
                for p in pts[1:]:
                    if (p - keep[-1]).length > 0.03:
                        keep.append(p)
                if len(keep) > 3:
                    P.glow['Neon'].path_tube(keep, 0.012, n=6, closed=True, slot=0)
    edge = bc0 + Vector((F.n.x, F.n.y, 0)) * (bw + 0.05)
    n = int((bz1 - bz0) / 0.22)
    for k in range(n):
        c = edge + Z * (bz0 + 0.1 + k * 0.22)
        bulbs.lathe([(0.0, -0.045), (0.04, -0.02), (0.04, 0.02), (0.0, 0.045)], tuple(c), 6, axis=Matrix((bt, Z, bn)).transposed(),
                    slot=0, uvall=((k % 32) / 32.0, 0.75))
    for zz in (7.4, 12.0, 17.0, 20.8):
        P.trim.beam(F.P(bu, zz, 0.0), F.P(bu, zz, -0.2), 0.08, 0.08, **P.T(1, 'paintsteel'))
    # Poster cases on the outer piers (_Posters, [screen]: case i at u i/4..(i+1)/4).
    posters = MB()
    for i, uu in enumerate((1.4, 2.8, 19.2, 20.6)):
        F.box(P.trim, uu - 0.6, uu + 0.6, 0.6, 2.6, -0.12, 0.0, 0.01, **P.T(2, 'front'))
        F.quad(posters, uu - 0.5, uu + 0.5, 0.7, 2.5, -0.121, uv=[(i / 4, 0), ((i + 1) / 4, 0), ((i + 1) / 4, 1), (i / 4, 1)], slot=0)
        F.quad(P.glass, uu - 0.52, uu + 0.52, 0.68, 2.52, -0.13, slot=0)
    P.extra_meshes = [('SM_NS_Vista_Rialto_Marquee', screen), ('SM_NS_Vista_Rialto_Posters', posters)]
    P.heights = {i: P.top + (0.0 if i == fi else -0.5) for i in range(len(P.faces))}
    finish_walls(P)
    P.body.poly([Vector((x, y, P.top - 0.6)) for x, y in offset_poly(P.poly, 0.3)], want=Z, slot=BODY_INDEX['ns_roof'], uvm=1, reg=5)
    P.note('chaser bulbs (_GlowWarm): the marquee\'s edge and soffit bulbs and the blade\'s outer edge; neon (_GlowNeon) round RIALTO\'s letters')
    return P


def plot_library():
    """SM_NS_Vista_Library: the Northside Branch Library on Cardinal, closing the view down Market: a Carnegie-style
    limestone branch on a raised base, seven granite steps to its doors, two lamp standards, tall arched reading-room
    windows (their panes [window], kind 1: T_NS_Rooms cell 15), NORTHSIDE BRANCH · PUBLIC LIBRARY in the frieze, a
    bronze book drop. Proxy detail first (spec 13.9)."""
    poly = rect_poly(-11440, -13440, 4750, 7150)
    P = Plot('VistaLibrary', poly, [], floors=2, gf=1.3, fh=6.4, frieze=0.0, parapet=0.0, wall='ns_limestone', seed=1904, body='B6',
             detail='proxy', palette={'stone': dict(type='stone', hex=0xc4bba8, soot=0.5), 'limestone': dict(type='stone', hex=0xc2b8a4, soot=0.5),
                                      'door': dict(type='bronze', hex=0x34261a)})
    P.name = 'SM_NS_Vista_Library'
    P.p = 'library'
    fi = street_edge(P.poly, 'w')
    P.streets = [fi]
    F = P.faces[fi]
    F.kind = 'street'
    L = F.L
    P.top = 11.2
    base_h = 1.3
    # The raised base, rusticated, with small basement windows.
    for uu in (3.0, 6.0, 18.0, 21.0):
        window_unit(P, F, uu, 0.3, 0.9, 0.7, lintel='flat', sill='none', guard=True, kind=0.33)
    rustication(P, F, 0.0, L, 0.0, base_h, course=0.43, block=1.2, slot='ns_limestone')
    # Reading-room windows: three each side of the entrance pavilion.
    for uu in (2.3, 5.3, 8.3, 15.7, 18.7, 21.7):
        arch_window(P, F, uu, base_h + 1.0, 2.0, 4.0, kind=1.0, frame='front', mullions=1, keystone=True)
    # The entrance: a projecting pavilion with paired pilasters, the doors, the steps, the lamps.
    pu0, pu1 = 9.9, 14.1
    du0, du1 = 11.1, 12.9
    lk = P.T(0, 'limestone')
    ze = P.top - 2.6
    F.box(P.trim, pu0, du0 - 0.1, base_h, ze, -0.6, 0.0, 0.01, seg=1, skip=((2, 1),), **lk)
    F.box(P.trim, du1 + 0.1, pu1, base_h, ze, -0.6, 0.0, 0.01, seg=1, skip=((2, 1),), **lk)
    F.box(P.trim, du0 - 0.1, du1 + 0.1, base_h + 4.4, ze, -0.6, 0.0, 0.01, seg=1, skip=((2, 1),), **lk)
    door_open = dict(u0=du0, u1=du1, z0=base_h, z1=base_h + 4.2, depth=0.8, sill=False)
    P.opens[fi].append(door_open)
    F.box(P.trim, du0, du1, base_h, base_h + 3.1, -0.05 + 0.6 + 0.0, 0.63, **P.T(2, 'door'))
    F.quad(P.glass, du0 + 0.1, du1 - 0.1, base_h + 3.2, base_h + 4.1, 0.55, slot=0)
    F.quad(P.trim, du0, du1, base_h + 3.1, base_h + 4.2, 0.7, **P.L('glow', F.P(12.0, base_h + 3.6, 1.5), (1, 0.5, 0.7)))
    for uu in (pu0 + 0.35, pu0 + 1.0, pu1 - 1.0, pu1 - 0.35):
        F.box(P.trim, uu - 0.25, uu + 0.25, base_h, ze, -0.78, -0.6, 0.01, seg=1, **P.T(0, 'limestone'))
        # Their capitals: a necking, an echinus and an abacus under the architrave.
        F.box(P.trim, uu - 0.27, uu + 0.27, ze - 0.42, ze - 0.36, -0.8, -0.6, 0.008, seg=1, **P.T(0, 'limestone'))
        F.box(P.trim, uu - 0.3, uu + 0.3, ze - 0.2, ze - 0.08, -0.84, -0.6, 0.01, seg=1, **P.T(0, 'limestone'))
        F.box(P.trim, uu - 0.32, uu + 0.32, ze - 0.08, ze, -0.86, -0.6, 0.008, seg=1, **P.T(0, 'limestone'))
    for k in range(7):
        h = base_h * (k + 1) / 7
        F.box(P.trim, pu0 - 0.6, pu1 + 0.6, 0.0, h, -0.6 - (7 - k) * 0.33, -0.6, 0.01, seg=1, **P.T(0, 'granite'))
    for s_ in (-1, 1):
        uu = (pu0 - 1.0) if s_ < 0 else (pu1 + 1.0)
        F.box(P.trim, uu - 0.35, uu + 0.35, 0.0, base_h + 0.3, -3.2, -0.6, 0.01, seg=1, **P.T(0, 'granite'))
        c = F.P(uu, base_h + 0.3, -2.4)
        P.trim.lathe([(0.0, 0.0), (0.2, 0.0), (0.16, 0.1), (0.06, 0.3), (0.05, 2.8), (0.09, 2.9), (0.04, 3.0), (0.0, 3.0)], tuple(c), 12, **P.T(1, 'paintsteel'))
        P.glow['Warm'].lathe([(0.0, 0.0), (0.18, 0.08), (0.22, 0.25), (0.15, 0.42), (0.0, 0.46)], tuple(c + Z * 3.0), 16, slot=0)
    F.box(P.trim, pu1 + 0.15, pu1 + 0.75, base_h, base_h + 1.0, -1.2, -0.6, 0.02, seg=1, **P.T(1, 'bronze'))
    path = [F.P(0.0, 0.0, 0.0), F.P(pu0, 0.0, 0.0), F.P(pu0, 0.0, -0.6), F.P(pu1, 0.0, -0.6), F.P(pu1, 0.0, 0.0), F.P(L, 0.0, 0.0)]
    fz0, fz1 = entablature(P, [fi], ze, frieze_h=0.75, arch_h=0.55, corn_h=0.8, project=0.75, path=[(q.x, q.y) for q in path])
    # The name in the frieze either side of the pavilion, the year on the pavilion's own.
    for lab, text, u0_, u1_, w_ in (('frieze_l', 'NORTHSIDE BRANCH', 0.0, pu0, 0.0), ('frieze_r', 'PUBLIC LIBRARY', pu1, L, 0.0),
                                    ('frieze_c', 'MCMIV', pu0, pu1, -0.6)):
        wd = (u1_ - u0_) - (1.2 if w_ == 0.0 else 0.5)
        P.sign(lab, wd, 0.74, carved_lettering(text, None, stone=0xc2b8a4, size=0.6, tracking=1.35, seed=91 + len(text)),
               F.P((u0_ + u1_) / 2, (fz0 + fz1) / 2, w_ - 0.101), F.n, depth=0.002, board=False)
    # A low balustrade over the cornice.
    zb = P.top - 0.4
    k = int(L / 0.32)
    for i in range(k):
        uu = 0.2 + i * (L - 0.4) / (k - 1)
        P.trim.lathe([(0.0, 0.0), (0.07, 0.0), (0.05, 0.08), (0.09, 0.3), (0.04, 0.5), (0.07, 0.56), (0.0, 0.56)], tuple(F.P(uu, zb, -0.15)), 8, **P.T(0, 'limestone'))
    F.box(P.trim, 0.0, L, zb + 0.56, zb + 0.72, -0.3, 0.0, 0.01, seg=1, **P.T(0, 'limestone'))
    P.heights = {i: P.top for i in range(len(P.faces))}
    finish_walls(P)
    P.body.poly([Vector((x, y, P.top - 1.0)) for x, y in offset_poly(P.poly, 0.3)], want=Z, slot=BODY_INDEX['ns_roof'], uvm=1, reg=5)
    return P


LANDMARKS = {'VistaBank': plot_bank, 'VistaRialto': plot_rialto, 'VistaLibrary': plot_library}


# ================================================================== the proxies (SM_NS_Proxy_A..H)

PROXY_MASONRY = {'ns_brick_red': ('brick', 0x7e3f2e), 'ns_brick_buff': ('brick', 0xa88c68), 'ns_brick_dark': ('brick', 0x33261f),
                 'ns_brick_painted': ('brick', 0xb4ad9e), 'ns_limestone': ('limestone', 0xbdb4a2), 'ns_caststone': ('caststone', 0x8a6f5a),
                 'ns_stucco': ('concrete', 0xb8ae9c), 'ns_roof': ('tar', 0x3a3936)}


def proxy_shop(P, F, u0, u1, sh, band=0.6, color=0x2a3a4a):
    """A proxy's shop: the opening, a frame and mullions, its window a [window] pane of kind 1 (a shop's back room, lit
    with the hour), a sign band with no lettering (proxies may be mirrored), a grille hood."""
    fi = P.faces.index(F)
    P.opens[fi].append(dict(u0=u0, u1=u1, z0=0.0, z1=sh, depth=0.25, sill=False))
    fk = P.T(2, 'front')
    F.box(P.trim, u0, u1, 0.0, 0.55, 0.12, 0.25, **fk)
    n = max(1, int((u1 - u0) / 1.8))
    for i in range(n + 1):
        uu = min(max(u0 + (u1 - u0) * i / n, u0 + 0.04), u1 - 0.04)
        F.box(P.trim, uu - 0.04, uu + 0.04, 0.55, sh, 0.12, 0.24, **fk)
    F.box(P.trim, u0, u1, sh - 0.6, sh - 0.52, 0.12, 0.24, **fk)
    F.box(P.trim, u0, u1, sh - 0.08, sh, 0.12, 0.24, **fk)
    F.quad(P.body, u0, u1, 0.55, sh, 0.25, uv=[(0, 0), (1, 0), (1, 1), (0, 1)], col=(P.rnd.random(), 0.05, 1.0, 1.0),
           slot=BODY_INDEX['ns_window'], reg=7, pane=(u1 - u0, sh - 0.55, 0.0))
    F.box(P.trim, u0 - 0.1, u1 + 0.1, sh + 0.35, sh + 0.35 + band, -0.08, 0.0, **P.T(2, 'board'))
    F.box(P.trim, u0, u1, sh, sh + 0.3, -0.3, 0.0, **P.T(1, 'hood'))


def proxy(pid, W, floors, masonry, seed, shops, door=None, cornice='metal', fe=None, base=None, lintel='flat', tank=False, lobby=None,
          palette=None, bay=2.0):
    """A stand-in front (PROP frame: front toward Blender -X, origin at the front's middle on the sidewalk line), the kit at
    its lowest detail."""
    poly = [(0.0, -W / 2), (12.0, -W / 2), (12.0, W / 2), (0.0, W / 2)]
    pal = {'board': dict(type='enamel', hex=0x2a3a4a, chips=0.6)}
    pal.update(palette or {})
    _LOWPOLY[0] = True
    P = Plot(pid, poly, [], floors=floors, gf=4.4, fh=3.1, wall=masonry, seed=seed, detail="proxy", palette=pal)
    P.name = f'SM_NS_Proxy_{pid[-1]}'
    P.p = f'proxy{pid[-1].lower()}'
    P.is_proxy = True
    fi = street_edge(P.poly, 'w')
    P.streets = [fi]
    F = P.faces[fi]
    F.kind = 'street'
    for u0, u1 in shops:
        proxy_shop(P, F, u0, u1, 3.4)
    if door is not None:
        if lobby:
            res_door(P, F, door, width=2.0, height=3.2, recess=0.5, number=None, lobby=False, canopy=lobby, buzzer=False, door_kind='bronze')
        else:
            res_door(P, F, door, number=None, buzzer=False)
    bays = bays_for(F.L, bay)
    walls_and_windows(P, fi, lintel=lintel, bays=bays, ac_ok=False)
    if fe:
        i0, i1 = fe
        fire_escape(P, F, bays[0][i0] - bays[1] / 2 + 0.1, bays[0][i1] + bays[1] / 2 - 0.1, [snap_z(P.fz(k) + 0.85) - 0.17 for k in range(2, P.floors + 1)])
    if cornice:
        main_cornice(P, [fi], kind=cornice, height=0.9, project=0.65)
    slot_of = slots_by_height([base[1]], [base[0], P.wall_m]) if base else slots_by_height([], [P.wall_m])
    for i, Fx in enumerate(P.faces):
        wall(P.body, Fx, 0.0, Fx.L, 0.0, P.top + P.parapet, P.opens[i], slot_of, sub=50.0, reg=0)
    roof(P, P.top - 0.2, P.top + P.parapet, tank=tank, bulkhead=True, extras=False)
    _LOWPOLY[0] = False
    return P


def build_proxy(fn):
    _LOWPOLY[0] = True
    try:
        return fn()
    finally:
        _LOWPOLY[0] = False


def finish_proxy(P):
    """One mesh: slot 0 the baked atlas (masonry, trim, everything), slot 1 ns_window (the panes, unbaked). Built as two
    objects (the atlas bakes alone) and joined after the bake (assemble)."""
    # The body's masonry moves into the trim as atlas kinds; its panes stay apart.
    win = MB()
    k = len(P.body.faces)
    for fi_ in range(k):
        slot = int(P.body.fattr['slot'][fi_])
        idx = P.body.faces[fi_]
        pts = [P.body.verts[i] for i in idx]
        if BODY_ORDER[slot] == 'ns_window':
            win.poly(pts, uv=P.body.fuv.get(fi_), col=P.body.fcol.get(fi_), slot=0, pane=P.body.fattr.get('pane', [None] * k)[fi_])
        else:
            kind, hexc = PROXY_MASONRY[BODY_ORDER[slot]]
            P.palette.setdefault(kind, dict(type='brick', hex=hexc) if kind == 'brick' else dict(type='stone' if kind != 'tar' else 'tar', hex=hexc))
            P.trim.poly(pts, **P.T(0, kind))
    # A proxy has no lit slot: what a door's glass shows of its hall is a dark interior in the atlas.
    sl = P.trim.fattr['slot']
    kk = P.trim.fattr['ss_kind']
    for i_ in range(len(P.trim.faces)):
        if sl[i_] == 3:
            sl[i_] = 0
            kk[i_] = NK['black']
    P.used[3] = set()
    P.used[0].add('black')
    atlas = P.trim.obj(P.name)
    kinds = set().union(*[P.used[s] for s in range(5)]) - {'sign'}
    m = trim_material(f'{P.p}_atlas', P.palette, kinds)
    atlas.data.materials.append(m.m)
    core.finish_hard_surface(atlas, 32.0)
    core.uv_layout(atlas, [(lambda f: True, 'box', None, (0.0, 0.0, 1.0, 1.0))])
    TEXTURE_SIZES[m.m.name] = 2048
    w = win.obj(P.name + '_PaneTmp')
    w.data.materials.append(body_material('ns_window').m)
    w['joins'] = P.name
    tris = sum(len(p.vertices) - 2 for p in atlas.data.polygons) + sum(len(p.vertices) - 2 for p in w.data.polygons)
    print(f'[ns_fronts] {P.name}: {tris} triangles; slots 0 {m.m.name} 2048 (baked atlas), 1 ns_window')
    return [atlas, w]


PROXIES = {
    'ProxyA': lambda: proxy('ProxyA', 24.0, 5, 'ns_brick_red', 101, [(0.5, 9.5), (13.5, 23.5)], door=11.5, fe=(7, 8),
                            palette={'board': dict(type='enamel', hex=0x7a1a14, chips=0.6)}),
    'ProxyB': lambda: proxy('ProxyB', 28.0, 7, 'ns_brick_buff', 102, [(0.5, 8.0), (20.0, 27.5)], door=14.0, cornice='stone', base=('ns_caststone', 7.4),
                            tank=True, lobby='bronze', palette={'cornice': dict(type='stone', hex=0xc4b8a0, soot=0.85)}, bay=1.8),
    'ProxyC': lambda: proxy('ProxyC', 20.0, 4, 'ns_brick_painted', 103, [(0.5, 16.5)], door=18.3, cornice='metal',
                            palette={'board': dict(type='enamel', hex=0x1a4a2a, chips=0.6)}),
    'ProxyD': lambda: proxy('ProxyD', 26.0, 6, 'ns_brick_dark', 104, [(0.5, 10.0), (16.0, 25.5)], door=13.0, cornice='stone', base=('ns_limestone', 4.4),
                            lintel='flat', palette={'cornice': dict(type='stone', hex=0xbab09a, soot=0.85)}, bay=2.05),
    'ProxyE': lambda: proxy('ProxyE', 22.0, 4, 'ns_brick_red', 105, [(0.5, 10.0), (12.0, 21.5)], door=None, cornice='metal', lintel='soldier',
                            palette={'board': dict(type='enamel', hex=0x2a2a5a, chips=0.6)}),
    'ProxyF': lambda: proxy('ProxyF', 30.0, 5, 'ns_stucco', 106, [(0.5, 9.0), (10.5, 19.5), (21.0, 29.5)], door=None, cornice='stone', lintel='flat',
                            palette={'board': dict(type='enamel', hex=0x8a6a1a, chips=0.6)}, bay=2.05),
    'ProxyG': lambda: proxy('ProxyG', 25.0, 6, 'ns_brick_red', 107, [(0.5, 11.0)], door=12.6, fe=(1, 2), cornice='metal',
                            palette={'cornice': dict(type='paint', hex=0x3a4a3f, under=0x5d6266, chips=0.8, soot=0.7, under_metal=0.6)}),
    'ProxyH': lambda: proxy('ProxyH', 21.0, 5, 'ns_brick_buff', 108, [(8.5, 20.5)], door=4.0, cornice='metal', lintel='hood', lobby='paint',
                            palette={'board': dict(type='enamel', hex=0x5a1a3a, chips=0.6)}),
}


# ================================================================== SM_NS_Penny_Front (P1): the Lucky Penny's storefront

def plot_penny():
    """SM_NS_Penny_Front (SITE): the Lucky Penny's storefront on Fifth (X 0 plane, Y 3700..4950), fitted to the store
    StreetStage builds (StreetStage.cpp BuildStore @4b8b7ae): its fixed glass (X -12..-10, Z 45..330) and the sliding
    doors (opening Y 4650..4850, on their track at X -23.5) stay code-built; this replaces the code's low wall, mullions,
    transom bar, header and door head. A brick knee wall (Z 0..45) with an aluminum sill cap; clear-anodized mullions
    and jambs; the red transom bar; a painted-aluminum header fascia (Z 330..480) carrying the box sign: an extruded
    aluminum cabinet with returns on a raceway, its face SM_NS_Penny_Front_Sign ([screen], UV0 0..1, 7.2 x 1.35 m, centered
    Y 4300 Z 405, 34 cm proud of the header: the game draws PropArt's sign art on it in place of the flat widget at
    X 4); the door surround and the operator's housing over the doors; window decals (LOTTERY, ATM INSIDE, COLD BEER,
    HOT COFFEE) on the glass's inner face; the hours sticker by the door, SM_NS_Penny_Front_Hours ([screen], UV0 0..1,
    Y 4880..4930, Z 140..170). Slots of SM_NS_Penny_Front (baked): 0 metal (aluminum, paint, steel), 1 brick, 2 prints."""
    P = Plot('Penny', [(0.0, 37.0), (14.0, 37.0), (14.0, 49.5), (0.0, 49.5)], [3], floors=1, gf=4.8, seed=212,
             palette={'alu': dict(type='alu', hex=0x9aa0a6, metal=0.9), 'enamel': dict(type='enamel', hex=0x8a1c14, chips=0.5, soot=0.4),
                      'paintsteel': dict(type='paint', hex=0x5a5e62, under=0x3a3a3a, chips=0.6, under_metal=0.8),
                      'brick': dict(type='brick', hex=0x7e3f2e)})
    P.name = 'SM_NS_Penny_Front'
    P.p = 'penny'
    F = Face((0.0, 49.5), (0.0, 37.0))
    tb = P.trim
    ak, ek, bk = P.T(0, 'alu'), P.T(0, 'enamel'), P.T(1, 'brick')
    # u along the front from its north end (Y 4950) south: Y = 4950 - 100 u.
    door0, door1 = 1.0, 3.0
    # The knee wall and its cap.
    for a, b in ((0.0, door0), (door1, 12.5)):
        F.box(tb, a, b, 0.0, 0.45, 0.0, 0.2, **bk)
        F.box(tb, a, b, 0.45, 0.48, -0.015, 0.2, 0.004, **ak)
    # Mullions (Y 4170, 4650, 4850) and the end jambs, front and back faces of the glass line.
    for u in (0.0, door0, door1, 7.8, 12.5):
        a, b = max(u - 0.06, 0.0), min(u + 0.06, 12.5)
        F.box(tb, a, b, 0.45 if u not in (door0, door1) else 0.0, 3.3, 0.06, 0.18, 0.003, **ak)
    # The transom bar (red), the header fascia and its cap and drip, the door head and the operator's housing.
    F.box(tb, 0.0, 12.5, 3.18, 3.3, -0.06, 0.02, 0.004, **ek)
    F.box(tb, 0.0, 12.5, 3.3, 4.8, -0.03, 0.0, 0.004, **ek)
    F.box(tb, -0.02, 12.52, 4.78, 4.86, -0.06, 0.05, 0.006, **P.T(0, 'paintsteel'))
    F.box(tb, -0.02, 12.52, 3.28, 3.32, -0.05, 0.0, **P.T(0, 'paintsteel'))
    F.box(tb, door0 - 0.06, door1 + 0.06, 2.6, 3.18, 0.0, 0.28, 0.004, **ak)
    F.box(tb, door0 - 0.4, door1 + 0.4, 2.605, 2.72, 0.19, 0.29, 0.004, **P.T(0, 'paintsteel'))
    for u in (door0, door1):
        F.box(tb, u - 0.06, u + 0.06, 0.0, 2.6, 0.0, 0.28, 0.003, **ak)
    # The box sign: cabinet with returns on a raceway; its face is the [screen].
    su, sz, sw, sh = 4950.0 / 100 - 43.0, 4.05, 7.2, 1.35
    F.box(tb, su - 2.8, su + 2.8, sz - 0.12, sz + 0.12, -0.12, -0.03, 0.006, **P.T(0, 'paintsteel'))
    F.box(tb, su - sw / 2 - 0.06, su + sw / 2 + 0.06, sz - sh / 2 - 0.06, sz + sh / 2 + 0.06, -0.34, -0.12, 0.01, seg=2, **ak)
    F.box(tb, su - sw / 2 - 0.04, su + sw / 2 + 0.04, sz - sh / 2 - 0.04, sz + sh / 2 + 0.04, -0.345, -0.335, **P.T(0, 'paintsteel'))
    screen = MB()
    F.quad(screen, su - sw / 2, su + sw / 2, sz - sh / 2, sz + sh / 2, -0.346, uv=[(0, 0), (1, 0), (1, 1), (0, 1)], slot=0)
    hours = MB()
    F.quad(hours, 0.2, 0.7, 1.4, 1.7, 0.122, uv=[(0, 0), (1, 0), (1, 1), (0, 1)], slot=0)
    # Window decals on the glass's inner face (read from the street).

    def decal(text, sub, fg, bg):
        def draw(s, Wm, Hm):
            s.rect(0, 0, Wm, Hm, bg, rough=0.5)
            s.rect(Hm * 0.06, Hm * 0.06, Wm - Hm * 0.12, Hm - Hm * 0.12, _shade(bg, 0.8), rough=0.5)
            o = s.text(text, Wm / 2, Hm * (0.42 if sub else 0.3), Hm * (0.42 if sub else 0.5), fg, face='Black', align='CENTER', rough=0.5)
            o.scale.x = 0.92
            if sub:
                s.text(sub, Wm / 2, Hm * 0.14, Hm * 0.18, fg, face='Bold', align='CENTER', rough=0.5)
            fac.worn(s, Wm, Hm, 30, 0xffffff, 0.15, len(text))
        return draw
    for label, text, sub, fg, bg, u, z, w, h in (('lottery', 'LOTTERY', 'PLAY HERE', 0xffffff, 0x1a7a3a, 10.6, 1.95, 1.0, 0.42),
                                                ('beer', 'COLD BEER', 'ICE · SODA', 0xffffff, 0x1f4fa8, 5.4, 2.35, 1.4, 0.45),
                                                ('coffee', 'HOT COFFEE', 'FRESH 24 HRS', 0xf6d200, 0x5a2a14, 9.0, 2.65, 1.3, 0.42),
                                                ('atm', 'ATM INSIDE', None, 0xffffff, 0xc8202c, 0.5, 2.1, 0.7, 0.2)):
        P.sign(label, w, h, decal(text, sub, fg, bg), F.P(u, z, 0.123), F.n, depth=0.002, board=False)
    P.extra_meshes = [('SM_NS_Penny_Front_Sign', screen), ('SM_NS_Penny_Front_Hours', hours)]
    P.faces = [F]
    return P


def finish_penny(P):
    o = P.trim.obj(P.name)
    mats = [trim_material(f'{P.p}_metal', P.palette, P.used[0]), trim_material(f'{P.p}_brick', P.palette, P.used[1])]
    pm = core.Mat(f'{P.p}_print')
    fac.print_material(pm, P.signs)
    mats.append(pm)
    for m in mats:
        o.data.materials.append(m.m)
    sl = [int(v) for v in P.trim.fattr['slot']]
    o.data.polygons.foreach_set('material_index', [0 if v == 0 else (1 if v == 1 else 2) for v in sl])
    core.finish_hard_surface(o, 32.0)
    core.uv_layout(o, [(core.material_is(o, m.m.name), 'box', None, (0.0, 0.0, 1.0, 1.0)) for m in mats] + [(lambda f: True, 'box', None, (0.0, 0.0, 1.0, 1.0))])
    TEXTURE_SIZES[mats[0].m.name] = 1024
    TEXTURE_SIZES[mats[1].m.name] = 512
    TEXTURE_SIZES[mats[2].m.name] = 1024
    o.name = o.data.name = P.name
    out = [o]
    for nm, mb in P.extra_meshes:
        e = mb.obj(nm)
        e.data.materials.append(screen_material())
        out.append(e)
    print(f'[ns_fronts] {P.name}: slots 0 penny_metal 1024, 1 penny_brick 512, 2 penny_print 1024; [screen] SM_NS_Penny_Front_Sign, _Hours')
    return out


# ================================================================== the skyline (P1) and the ridge (P1)

SET_CENTER = (-10.0, 15.0)     # Blender meters: the middle of the set (stage X 1000, Y 1500)
SKY_SECTORS = {'N': 90.0, 'W': 180.0, 'S': 270.0, 'E': 0.0}   # the direction each sheet faces from the set (Blender angle)
SKY_MASONRY = ['ns_brick_red', 'ns_brick_buff', 'ns_brick_dark', 'ns_limestone', 'ns_stucco', 'ns_brick_buff', 'ns_brick_red']


def sky_building(mb, glow, rnd, cx, cy, fw, fd, h, ang, masonry, kind, setback=False, tank=False, crown=None, near=True):
    """One skyline building: a box (with a setback for tall ones) turned to face the set, windows (ns_window panes)
    on the faces the set can see, a water tank or a lit crown on top."""
    def boxes():
        out = [(fw, fd, 0.0, h)]
        if setback and h > 60:
            out = [(fw, fd, 0.0, h * 0.62), (fw * 0.7, fd * 0.7, h * 0.62, h * 0.85), (fw * 0.45, fd * 0.45, h * 0.85, h)]
        return out
    ca, sa = math.cos(ang), math.sin(ang)
    to_set = Vector((SET_CENTER[0] - cx, SET_CENTER[1] - cy, 0.0)).normalized()
    sl = BODY_INDEX[masonry]
    for w_, d_, z0, z1 in boxes():
        corners = [(-w_ / 2, -d_ / 2), (w_ / 2, -d_ / 2), (w_ / 2, d_ / 2), (-w_ / 2, d_ / 2)]
        pts = [(cx + x * ca - y * sa, cy + x * sa + y * ca) for x, y in corners]
        for i in range(4):
            a, b = pts[i], pts[(i + 1) % 4]
            F = Face(a, b)
            mb.poly([F.P(0, z0), F.P(F.L, z0), F.P(F.L, z1), F.P(0, z1)], want=F.n, slot=sl, uvm=1, reg=0)
            if F.n.dot(to_set) > -0.25:
                # Windows on a coarse grid: 1.2 x 1.6 m, 2.6 x 3.2 m bays.
                nb = max(1, int(F.L / 2.6))
                nf = max(1, int((z1 - z0 - 3.0) / 3.2))
                bw = F.L / nb
                for k in range(nf):
                    z = z0 + 3.0 + k * 3.2 + 0.9
                    for j in range(nb):
                        if rnd.random() < 0.06:
                            continue
                        uc = bw * (j + 0.5)
                        mb.poly([F.P(uc - 0.6, z, -0.02), F.P(uc + 0.6, z, -0.02), F.P(uc + 0.6, z + 1.6, -0.02), F.P(uc - 0.6, z + 1.6, -0.02)],
                                want=F.n, uv=[(0, 0), (1, 0), (1, 1), (0, 1)], col=(rnd.random(), z / max(h, 1.0), kind, 1.0),
                                slot=BODY_INDEX['ns_window'], reg=7, pane=(1.2, 1.6, 0.0))
        mb.poly([Vector((x, y, z1)) for x, y in pts], want=Z, slot=BODY_INDEX['ns_roof'], uvm=1, reg=5)
    top = boxes()[-1]
    if tank:
        tx, ty = cx + rnd.uniform(-2, 2), cy + rnd.uniform(-2, 2)
        mb.lathe([(0.0, 0.0), (2.2, 0.0), (2.2, 3.5), (1.4, 4.6), (0.0, 4.9)], (tx, ty, h + 2.5), 10, slot=BODY_INDEX['ns_roof'], uvm=1, reg=5)
        for dx in (-1.6, 1.6):
            for dy in (-1.6, 1.6):
                mb.box(tx + dx - 0.15, tx + dx + 0.15, ty + dy - 0.15, ty + dy + 0.15, h, h + 2.5, slot=BODY_INDEX['ns_roof'], uvm=1, reg=5)
    if crown:
        # A lit crown: a band of glowing strips round the top of the tower.
        w_, d_ = top[0], top[1]
        corners = [(-w_ / 2, -d_ / 2), (w_ / 2, -d_ / 2), (w_ / 2, d_ / 2), (-w_ / 2, d_ / 2)]
        pts = [(cx + x * ca - y * sa, cy + x * sa + y * ca) for x, y in corners]
        g = glow[crown]
        for i in range(4):
            F = Face(pts[i], pts[(i + 1) % 4])
            for zz in (h - 1.2, h - 4.2, h - 7.2):
                g.poly([F.P(0.5, zz, -0.05), F.P(F.L - 0.5, zz, -0.05), F.P(F.L - 0.5, zz + 0.5, -0.05), F.P(0.5, zz + 0.5, -0.05)], want=F.n, slot=0)
        mb.lathe([(min(w_, d_) * 0.3, 0.0), (0.4, 14.0), (0.0, 18.0)], (cx, cy, h), 8, slot=BODY_INDEX['ns_roof'], uvm=1, reg=5)


def plot_skyline(key):
    """SM_NS_Skyline_<key> (SITE, P1): the quarter of the ring of buildings 150-800 m out that this sheet faces."""
    P = Plot(f'Skyline{key}', [(0.0, 0.0), (1.0, 0.0), (1.0, 1.0), (0.0, 1.0)], [], floors=1, seed=7000 + ord(key))
    P.name = f'SM_NS_Skyline_{key}'
    P.p = f'sky{key.lower()}'
    P.is_sky = True
    rnd = random.Random(7000 + ord(key))
    base = math.radians(SKY_SECTORS[key])
    n = 78
    for i in range(n):
        a = base + math.radians(-44.0 + 88.0 * (i + rnd.random() * 0.8) / n)
        r = rnd.uniform(160.0, 360.0) if i % 3 else rnd.uniform(380.0, 780.0)
        cx, cy = SET_CENTER[0] + r * math.cos(a), SET_CENTER[1] + r * math.sin(a)
        far = r > 380.0
        h = rnd.uniform(18.0, 40.0) if not far else rnd.uniform(35.0, 140.0)
        fw, fd = rnd.uniform(16.0, 34.0), rnd.uniform(14.0, 30.0)
        kind = 0.0 if h < 45 else (0.33 if rnd.random() < 0.6 else 0.0)
        crown = ('Warm' if rnd.random() < 0.5 else 'Cool') if (far and h > 90 and rnd.random() < 0.5) else None
        sky_building(P.body, P.glow, rnd, cx, cy, fw, fd, h, a + rnd.uniform(-0.2, 0.2), rnd.choice(SKY_MASONRY), kind,
                     setback=h > 60, tank=(h < 50 and rnd.random() < 0.55), crown=crown)
    if key == 'N':
        # An unnamed clock tower to the north: a lit dial on each face toward the set, a copper spire.
        cx, cy = SET_CENTER[0] - 40.0, SET_CENTER[1] + 520.0
        sky_building(P.body, P.glow, rnd, cx, cy, 14.0, 14.0, 72.0, math.radians(90), 'ns_limestone', 0.33)
        P.body.box(cx - 6.0, cx + 6.0, cy - 6.0, cy + 6.0, 72.0, 82.0, slot=BODY_INDEX['ns_limestone'], uvm=1, reg=0)
        P.body.lathe([(6.5, 0.0), (0.6, 22.0), (0.0, 25.0)], (cx, cy, 82.0), 4, slot=BODY_INDEX['ns_roof'], uvm=1, reg=5)
        for nx_, ny_ in ((0, -1), (1, 0), (-1, 0)):
            c = Vector((cx + nx_ * 6.02, cy + ny_ * 6.02, 77.0))
            ax = Matrix((Vector((-ny_, nx_, 0.0)), Z, Vector((nx_, ny_, 0.0)))).transposed()
            P.glow['Warm'].lathe([(0.0, 0.0), (3.6, 0.0), (0.0, 0.01)], tuple(c), 24, axis=ax, slot=0)
    if key == 'W':
        # A river bridge's lights to the west (+X stage, Blender -x): a deck on two towers, lamps strung along it.
        y0, y1, x = SET_CENTER[1] - 600.0, SET_CENTER[1] + 420.0, SET_CENTER[0] - 620.0
        P.body.box(x - 8.0, x + 8.0, y0, y1, 24.0, 27.0, slot=BODY_INDEX['ns_caststone'], uvm=1, reg=0)
        for ty in (y0 + 300.0, y1 - 300.0):
            P.body.box(x - 9.0, x + 9.0, ty - 4.0, ty + 4.0, 0.0, 95.0, slot=BODY_INDEX['ns_caststone'], uvm=1, reg=0)
            P.glow['Neon'].lathe([(0.0, 0.0), (1.2, 0.0), (0.0, 1.2)], (x, ty, 95.0), 6, slot=0)
        k = int((y1 - y0) / 24.0)
        for i in range(k + 1):
            yy = y0 + (y1 - y0) * i / k
            for sx in (-7.5, 7.5):
                P.glow['Warm'].lathe([(0.0, 0.0), (0.7, 0.0), (0.0, 0.9)], (x + sx, yy, 28.0), 6, slot=0)
        P.neon = 0xff2a1a
    return P


def finish_sky(P):
    body = P.body.obj(P.name, uv_meters=True)
    slots = sorted({int(v) for v in P.body.fattr['slot'] if v is not None})
    for sidx in slots:
        body.data.materials.append(body_material(BODY_ORDER[sidx]).m)
    remap = {s: i for i, s in enumerate(slots)}
    body.data.polygons.foreach_set('material_index', [remap[int(v)] for v in P.body.fattr['slot']])
    if 'Color' not in body.data.color_attributes:
        body.data.color_attributes.new('Color', 'FLOAT_COLOR', 'CORNER')
    me = body.data
    nf = len(me.polygons)
    reg = _attr_array(me, 'reg', nf)
    ca = me.color_attributes['Color']
    cols = np.empty(len(me.loops) * 4, np.float32)
    ca.data.foreach_get('color', cols)
    cols = cols.reshape(-1, 4)
    totals = np.empty(nf, np.int64)
    me.polygons.foreach_get('loop_total', totals)
    lr = np.repeat(reg, totals)
    cols = np.where((lr == 7)[:, None], cols, np.array([0.35, 0.1, 0.8, 1.0], np.float32))
    ca.data.foreach_set('color', cols.ravel())
    body.data.shade_flat()
    if P.name == 'SM_NS_Ridge':
        # The ridge is one plain slot, ns_ridge (night forest and rock; its lights are the glows).
        rm = bpy.data.materials.get('ns_ridge')
        if rm is None:
            m_ = core.Mat('ns_ridge')
            m_.set('Base Color', hx(0x1c211d))
            m_.set('Roughness', 0.95)
            rm = m_.m
        body.data.materials.clear()
        body.data.materials.append(rm)
        body.data.polygons.foreach_set('material_index', [0] * len(body.data.polygons))
        body.data.shade_smooth()
    out = [body]
    for fam, mb in P.glow.items():
        if not len(mb):
            continue
        g = mb.obj(f'{P.name}_Glow{fam}')
        gm = glow_material(f'glow_{P.p}_{fam.lower()}')
        g.data.materials.append(gm.m)
        if fam == 'Neon' and P.neon:
            ca = g.data.color_attributes.new('Color', 'FLOAT_COLOR', 'CORNER')
            ca.data.foreach_set('color', list(hx(P.neon)) * len(g.data.loops))
            va = gm.node('ShaderNodeVertexColor')
            va.layer_name = 'Color'
            gm.link(va.outputs['Color'], gm.bsdf.inputs['Emission Color'])
        out.append(g)
    tris = sum(sum(len(p.vertices) - 2 for p in o.data.polygons) for o in out)
    print(f'[ns_fronts] {P.name}: {tris} triangles; slots ' + ', '.join(f'{i} {s.material.name}' for i, s in enumerate(body.material_slots)))
    return out


def plot_ridge():
    """SM_NS_Ridge (SITE, P1): the ridge 3-5 km east (stage -X, Blender +x) with the Embercrest on its shoulder over
    Market's axis: a cluster of warm points (_GlowWarm) and the ember crown (_GlowNeon, EmberHex 0xff6a24)."""
    P = Plot('Ridge', [(0.0, 0.0), (1.0, 0.0), (1.0, 1.0), (0.0, 1.0)], [], floors=1, seed=4242)
    P.name = 'SM_NS_Ridge'
    P.p = 'ridge'
    P.is_sky = True
    P.neon = 0xff6a24
    mb = P.body
    ny, nx = 240, 30
    ys = [SET_CENTER[1] - 3200.0 + 6400.0 * j / ny for j in range(ny + 1)]
    xs = [3000.0 + 2000.0 * i / nx for i in range(nx + 1)]
    rnd = random.Random(4242)
    grid = []
    for i, x in enumerate(xs):
        row = []
        for j, y in enumerate(ys):
            t = (x - 3000.0) / 2000.0
            crest = 720.0 + 180.0 * math.sin(y / 700.0) + 90.0 * math.sin(y / 230.0 + 1.3) + 40.0 * math.sin(y / 90.0)
            zz = crest * (math.sin(min(t * 1.2, 1.0) * math.pi / 2) ** 1.4) + rnd.uniform(-8, 8)
            row.append(mb.v((x, y, max(zz, 0.0))))
        grid.append(row)
    for i in range(nx):
        for j in range(ny):
            mb.f([grid[i][j], grid[i][j + 1], grid[i + 1][j + 1], grid[i + 1][j]], want=(-1.0, 0.0, 0.3), slot=BODY_INDEX['ns_roof'], uvm=1, reg=5)
    # The resort on the shoulder facing Northside, centered on Market's axis.
    cx, cy, cz = 3700.0, 59.5, 520.0
    for k in range(160):
        px = cx + rnd.uniform(-30.0, 60.0)
        py = cy + rnd.gauss(0.0, 55.0)
        pz = cz + rnd.uniform(-25.0, 60.0) + max(0.0, 40.0 - abs(py - cy) * 0.5)
        P.glow['Warm'].lathe([(0.0, 0.0), (1.6, 0.0), (0.0, 2.2)], (px, py, pz), 5, slot=0)
    # The ember crown: three lit peaks over the tower.
    for k, (dy, hh) in enumerate(((-14.0, 18.0), (0.0, 26.0), (14.0, 18.0))):
        base = Vector((cx - 2.0, cy + dy, cz + 95.0))
        P.glow['Neon'].poly([base + Vector((0, -8.0, 0)), base + Vector((0, 8.0, 0)), base + Vector((0, 0, hh))], want=(-1.0, 0.0, 0.0), slot=0)
    mb.box(cx - 6.0, cx + 6.0, cy - 16.0, cy + 16.0, cz - 40.0, cz + 95.0, slot=BODY_INDEX['ns_brick_dark'], uvm=1, reg=0)
    return P


SKY = {'SkylineN': lambda: plot_skyline('N'), 'SkylineE': lambda: plot_skyline('E'), 'SkylineS': lambda: plot_skyline('S'),
       'SkylineW': lambda: plot_skyline('W'), 'Ridge': plot_ridge}


# ================================================================== review from the exported meshes

REVIEW_EXTRA = {
    # Close looks at the golden 50 m, stage cm: camera, look-at, FOV.
    'Corner': ((1500, 4300, 165), (200, 5600, 350), 62.0),
    'MarketS': ((-900, 5250, 165), (-4800, 4900, 400), 58.0),
    'MarketN': ((-1200, 6650, 165), (-3600, 7000, 500), 60.0),
    'FifthHome': ((350, -1500, 165), (0, -4300, 600), 62.0),
    'FifthFar': ((1700, 3000, 165), (2040, 6400, 600), 62.0),
    'Bank': ((1020, -6400, 165), (1020, -8040, 900), 55.0),
    'BankDoor': ((1330, -7450, 165), (1020, -8040, 330), 52.0),
    'Barber': ((800, 2450, 165), (0, 3250, 240), 60.0),
    'MarketMid': ((-3300, 6150, 165), (-4300, 4950, 350), 60.0),
}


def _plot_shots():
    """Every plot's front from the road at a walker's eye (stage cm): camera, look-at, FOV."""
    out = {}
    for pid, yc in (('HHollis', -4900), ('HKim', -2875), ('HCash', 1550), ('Ruby', 7575), ('HWexler', 8600)):
        out[pid] = ((950, yc + 500, 165), (0, yc, 420), 62.0)
    for pid, yc in (('FHollis', -5250), ('FLobby', -3300), ('FBotanica', -1590), ('FRecords', 5520), ('FWexler', 7750)):
        out[pid] = ((1100, yc - 500, 165), (2040, yc, 420), 62.0)
    for pid, xc in (('MS1', -1900), ('MS2', -3050), ('MS3', -4100), ('MS4', -5550), ('MS5', -7150), ('MS6', -8650)):
        out[pid] = ((xc + 600, 5900, 165), (xc, 4950, 420), 62.0)
    for pid, xc in (('RubyM', -750), ('MN2', -2150), ('MN3', -3200), ('MN4', -3950), ('MN5', -5300), ('MN6', -6950), ('MN7', -8600)):
        out[pid] = ((xc + 600, 6000, 165), (xc, 6950, 420), 62.0)
    out['Bank'] = ((1020, -6500, 165), (1020, -8040, 850), 58.0)
    out['Rialto'] = ((1020, 9500, 165), (1020, 11040, 900), 62.0)
    out['Library'] = ((-9900, 5950, 165), (-11440, 5950, 600), 60.0)
    return out


# Review render sizes and samples (NSF_RV=1600,900,64 for the final sheets; smaller while iterating on a busy GPU).
RV_BIG = tuple(int(v) for v in os.environ.get('NSF_RV', '1280,720,40').split(','))
RV_SMALL = (RV_BIG[0] * 3 // 5, RV_BIG[1] * 3 // 5, max(16, RV_BIG[2] // 2))


def review_exported(which=None):
    """Imports every mesh this module exported (unreal/Art/Meshes) and renders the street review sheets from them:
    ns_fronts_w_day/night.jpg (the spec's W shots) and ns_fronts_close_day/night.jpg (REVIEW_EXTRA)."""
    import json
    which = which or os.environ.get('NSF_REVIEW', 'w,close')
    core.reset()
    g = bpy.data.node_groups.get('glTF Material Output')
    if g is not None:
        g.name = 'glTF Material Output (artkit)'
    names = sorted(f[:-4] for f in os.listdir(core.EXPORT_DIR) if f.endswith('.glb') and
                   f.startswith(('SM_NS_Bldg_', 'SM_NS_Vista_', 'SM_NS_Proxy_', 'SM_NS_Penny_', 'SM_NS_Skyline_', 'SM_NS_Ridge')))
    objs = []
    for nm in names:
        before = set(bpy.data.objects)
        bpy.ops.import_scene.gltf(filepath=os.path.join(core.EXPORT_DIR, f'{nm}.glb'))
        new = [o for o in bpy.data.objects if o not in before and o.type == 'MESH']
        for o in new:
            o.name = nm
            objs.append(o)
    jp = os.path.join(core.BUILD_DIR, 'ns_fronts_places.json')
    places = json.load(open(jp)) if os.path.exists(jp) else {}
    for o in objs:
        for s_ in o.material_slots:
            if s_.material and '.' in s_.material.name and s_.material.name.split('.')[0] in BODY_BASE:
                s_.material = bpy.data.materials.get(s_.material.name.split('.')[0]) or s_.material
        if o.name in places:
            o['place_at'] = places[o.name]
            o.location = Vector(places[o.name])
        if o.name.startswith('SM_NS_Proxy_') and o.name[-1] in REVIEW_PROXY_AT:
            x, y, yaw = REVIEW_PROXY_AT[o.name[-1]]
            o.location = (x, y, 0.0)
            o.rotation_euler = (0.0, 0.0, math.radians(yaw))
        if o.name.endswith('_Hour'):
            o.rotation_euler = (0.0, math.radians(-305.0), 0.0)
        elif o.name.endswith('_Minute'):
            o.rotation_euler = (0.0, math.radians(-60.0), 0.0)
    print(f'[ns_fronts] review: {len(objs)} meshes imported')
    R = _Review(objs)
    try:
        R.context()
        R.grilles(up=True)
        shots = []
        if 'w' in which:
            shots += [(k, S(*cp), S(*lp), fov) for k, (cp, lp, fov) in W_SHOTS.items()]
        if 'close' in which:
            csel = [t for t in os.environ.get('NSF_CLOSE', '').split(',') if t]
            shots += [(f'X{k}', S(*cp), S(*lp), fov) for k, (cp, lp, fov) in REVIEW_EXTRA.items() if not csel or k in csel]
        if 'plots' in which:
            sel = [t for t in os.environ.get('NSF_PLOTS', '').split(',') if t]
            shots += [(f'P{k}', S(*cp), S(*lp), fov) for k, (cp, lp, fov) in _plot_shots().items() if not sel or k in sel]
        for night in (False, True):
            R.set_time(night)
            tag = 'night' if night else 'day'
            ww = [R.shot(f'{lab}_{tag}', cp, lp, fov, *RV_BIG) for lab, cp, lp, fov in shots if lab.startswith('W')]
            xx = [R.shot(f'{lab}_{tag}', cp, lp, fov, *RV_BIG) for lab, cp, lp, fov in shots if lab.startswith('X')]
            if ww:
                compose(f'ns_fronts_w_{tag}', ww, 2)
            if xx:
                compose(f'ns_fronts_close_{tag}', xx, 2)
            pp = [R.shot(f'{lab}_{tag}', cp, lp, fov, *RV_SMALL) for lab, cp, lp, fov in shots if lab.startswith('P')]
            if pp:
                compose(f'ns_fronts_plots_{tag}', pp, 4)
    finally:
        R.cleanup()
