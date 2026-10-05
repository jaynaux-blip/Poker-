"""SM_ECX_*: the Embercrest's arrival plaza, the bus stop and the ridge road (design spec ec_exterior_spec.md §2.3,
§2.5, §3.2).

Every mesh follows the facade convention (front toward Blender -X, Z up, meters). SITE meshes (the ground, the
cascades and their water) sit at Blender (0, 0, 0) = the map origin (the entrance doors' threshold) and are placed
at (0,0,0) yaw 180, so a map point (X, Y, Z) in UE cm is Blender (-X/100, Y/100, Z/100). PROP meshes are built
about their own origin as §3.2 states; a PROP placed at yaw Y turns by (180 - Y) degrees about Blender Z.

What is built, and the choices made inside the spec's ranges:

SM_ECX_Ground (unbaked, 8 slots named ecx_paver, ecx_sett, ecx_asphalt, ecx_curb, ecx_concrete, ecx_wallstone,
  ecx_snowground, ecx_marking). Built from convex region pieces gridded at 0.33-1.0 m, then welded:
  - Levels (§2.2): apron, top landing, wing sidewalks 0; lanes -0.12 (a 1.2 cm gutter dip within 0.35 m of each curb)
    with the raised table (|Y| <= 3.5 m at 0, setts ramps to |Y| 5.0); the drive curves -0.12; landing B -0.75,
    landing A -1.50, the platform and its extension -2.25; the bay -2.40; the overlook -2.25.
  - The arms: Z(X) = -0.12 - z(s), s = X - 21.25 m in [0, 34.75], a trapezoid grade (0 -> 7.18 % over the first
    3 m, 7.18 %, back to 0 over the last 3 m) that drops exactly 2.28 m: z = g s^2/6 (s < 3), g (s - 1.5) (3 <= s <=
    31.75), 2.28 - g (34.75 - s)^2/6 (s > 31.75), g = 2.28 / 31.75. arm_z() below is the formula.
  - The ridge road: -2.40 from Y -42 to +60 m, 4 % down to -8.72 at Y -200, 2 % up to -1.20 at Y +120, 10 m
    parabolic vertical curves at both breaks; crowned 2.5 cm on the center line (X 59.5 m), edges at the profile.
  - Curbs (ecx_curb): granite, 18 cm wide, a 12 mm chamfered arris, 12-15 cm reveals; they lie on the upper side of
    their face line (the face line is the drive's edge, so the drive regions stop exactly at it). One continuous
    outer curb runs from the east shoulder at Y -200 round the south arm, the curve, along the lanes' west edge,
    the north curve and arm to the shoulder at Y +120; one closed loop curbs the island (lanes' east edge, the
    inner curves, the arms' inner edges, the platform's bay edge and its tapers). Low granite edgings (4 cm) part
    paving from snow where no curb does.
  - Retaining walls (ecx_wallstone, 35 cm thick) at X 24.0, 35.6 and 47.2 m, their faces on those lines, from the
    steps' cheeks (|Y| 4.0) to the arms' curbs. Their tops stand 45 cm over the upper side (so they meet the
    cascade copings, 45 cm over their landings, flush), easing to a 12 cm upstand where the beds rise to meet the
    arm. The wing planter strips' 25 cm wallstone curbs at X 5.0 m.
  - Island beds: snow over the terrace levels (0 / -0.75 / -1.5 / -2.25), blended toward the arm's curb top near
    the arms (w = smoothstep(20.5, 26.32, |Y|)), mounded 10-35 cm, flattened (to the terrace level) under the
    monument, the island trees, the rock and the drifts the engineer places there. Verges: the arm's curb top
    (arm_z + 0.15) plus mounds; concrete lamp pads (0.7 m) at the verge lamp posts, top at arm_z(X) + 0.16.
  - Markings (ecx_marking, 2 mm proud): the double yellow center line, white edge lines (dashed along the bay), the
    zebra to the overlook (Y 12..15 m), a stop bar across the north arm, the BUS stencil (reading for a driver
    heading north), a yellow tactile band along the platform curb.
  - UV0 = map meters on horizontal faces (u = X = -x, v = Y = y); on curbs, walls and skirts u runs along the face
    (meters) and v = Z. COLOR_0 (linear floats): R puddle-prone, G slush (unheated edges, bank feet), B heated
    (snowmelt zone: the plaza's pavers and setts); on markings RGB is the paint (sRGB hex converted to linear).
  - Skirts 0.6 m deep hang from the outline wherever the ground meets the terrain.
  - Ground texture sets written to unreal/Art/Textures/ECX (seamless; BaseColor sRGB, Normal OpenGL +Y, ORM linear):
    T_ECX_Paver (3.6 m tile; Height 0..1 = -8..+2 mm), T_ECX_Sett (2.0 m herringbone; Height 0..1 = -14..+2 mm),
    T_ECX_Asphalt (4.0 m), T_ECX_Concrete (2.0 m, 1024, a saw cut at u = 0), T_ECX_Curb (2.0 m, 1024, stone joints
    every 1.0 m along u), T_ECX_WallStone (3.0 m coursed ashlar). The 2 x 2 tiling check is art/review/ecx_plaza_textures.jpg.
SM_ECX_Steps: granite slabs 1.2 m long (3 mm joints), a 25 mm nosing with a bronze angle and three anti-slip
  grooves behind it, an LED channel under each nosing (SM_ECX_Steps_GlowWarm), basalt cheeks whose tops run 11 cm
  over the nosing line. The fifth nosing is a 0.3 m granite edge stone lying 3 mm proud on the upper landing, so the
  mesh is 1.93 deep and the cheeks reach 0.86.
SM_ECX_Cascade: two raised basins of polished black granite with basalt coping stones (1.2 m, 4 mm joints), a
  bronze-clad spout wall at X 25.2..25.6 m rising to +0.45 with its lip at +0.10, the bronze weir lip at X 35.6 m
  (top -0.375), basalt footings under the fire bowls (tops 6 cm over the basin floors), a limescale line at each
  water level, snow on the outer copings. Its walls run down to -1.6 / -1.9 (hidden below the landings and beds).
  SM_ECX_Cascade_Water: the planes at -0.37 / -1.12 (UV0 = map meters), the weir and source sheets (UV u across
  0..1, v along the flow: Blender v = 0 at the lip, 1 at the foot, so after the glTF V flip v runs 1 -> 0 down the fall).
SM_ECX_FireBowl: a hammered bowl on a basalt plinth, lava rock and ember glass; embers glow between the rocks.
SM_ECX_MonumentSign: the slab floats over a granite plinth recessed 5 cm (a shadow gap that hides the ember base
  line), the lockup on both faces: the mark 2.5 m wide in bronze relief over its lit ember disc, EMBERCREST in bronze
  channel letters with lit ash faces, RESORT · CASINO · POKER SERIES in shallow bronze channel letters with lit faces
  in a softer gold (half the letters' level; polished bronze alone mirrored the black sky and went unreadable at
  night); a bronze bezel round the LED strip; snow on the split-face top and drifts at the foot (they reach 0.25 m
  past the plinth: the mesh is 1.67 deep).
SM_ECX_BusShelter: posts at the back on anchor-bolted base plates, a cantilevered roof with larch slats under it, glass
  in the side bays, the POKER SERIES lightbox in the middle bay (bezelled), a long larch bench with two radiant
  heaters over it (their quartz tubes glow in the prints slot), the route-map case at the south end (bezelled; print
  facing north, into the shelter; a route 14 badge outside), the timetable at the north end (printed on both faces),
  a downpipe down the back of the north middle post.
SM_ECX_BusStopSign: the blade hangs off the pole's -y side, so at the spec's yaw 90 it projects over the platform
  (map -X), not over the bay (on the +y side it reached 26 cm into the bay at 2.35-3.15 m, where a bus pulling out
  of the stop cuts through it).
SM_ECX_Planter: snow heaped in the box (a wind-smoothed loaf to about 1.03 m, 8 cm proud of the cap), meeting the
  walls just under the cap's inner lip; a tree well at the trunk, where a ragged ring of soil shows; the cap is clear.
SM_ECX_SnowBank_A: each row's samples are spaced so a row edge lies exactly on the slush line (both feet), so the
  grime/snow boundary follows a ragged line instead of stepping face row by face row.
SM_ECX_SnowBank_B: built on a polar grid whose rings follow the heap's contours: a finite toe, the slush line a
  ragged ring, a 1-3 cm slush apron (grime) that sinks under the ground inside 6 x 5 m.
SM_ECX_SnowBank_C: tapers to points at both tips (sinking 2 cm under the ground there: open ends, no end faces).
T_ECX_Asphalt: sealed cracks are low-contrast tar bands that keep their course (two along, one across), plus faint
  hairlines; the oxidation and salt bloom blend two lattice sizes.
Other props as §3.2. Placement notes: the verge lamp posts stand on concrete pads (top arm_z(X) + 0.16); the plow face
  of SM_ECX_SnowBank_A is its -X side, so on the south verge yaw 270 turns it to the drive (the spec's yaw 90 turns it
  to the trees there); the bollard ring's glow sits at 0.822..0.838 m.

Emissive levels baked into [lit] slots (glTF emissive strength): monument letter faces and ember disc 12 (the subline
faces 12 x half-level gold), shelter map case, lightbox and heater tubes 12, valet plaque, key box and lamp 10,
fire-bowl embers 5. The engineer scales them with DimImportedGlow (the review sheets show them at 0.2 by day).

ECX_ONLY=steps,cascade,... (environment) builds just those meshes (keys of BUILDERS); ECX_TEX=0 skips the
ground texture sets; ECX_REVIEW=site,props,signs,tex picks the extra review sheets (art/review/ecx_plaza_*.jpg).
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
from mathutils.bvhtree import BVHTree

from artkit import core, parts, review, street
from artkit.shade import convex_edges, mix_float, noise, object_coords, ramp, scaled, smooth_less, vec
from artkit.sheet import Sheet

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import brand  # noqa: E402

MESHES = ['SM_ECX_Ground', 'SM_ECX_Steps', 'SM_ECX_Steps_GlowWarm', 'SM_ECX_Cascade', 'SM_ECX_Cascade_Water', 'SM_ECX_FireBowl',
          'SM_ECX_FireBowl_Flame', 'SM_ECX_MonumentSign', 'SM_ECX_MonumentSign_GlowEmber', 'SM_ECX_MonumentSign_Screen', 'SM_ECX_BusShelter',
          'SM_ECX_BusShelter_GlowCool', 'SM_ECX_BusStopSign', 'SM_ECX_LampPost', 'SM_ECX_LampPost_GlowWarm', 'SM_ECX_Bollard',
          'SM_ECX_Bollard_GlowEmber', 'SM_ECX_Planter', 'SM_ECX_Bench', 'SM_ECX_ValetPodium', 'SM_ECX_LuggageCart', 'SM_ECX_AshUrn',
          'SM_ECX_SnowBank_A', 'SM_ECX_SnowBank_B', 'SM_ECX_SnowBank_C', 'SM_ECX_ViewRail', 'SM_ECX_Viewer']
UNBAKED = {'SM_ECX_Ground', 'SM_ECX_Cascade_Water', 'SM_ECX_FireBowl_Flame', 'SM_ECX_MonumentSign_Screen'}
_ONLY = [s.strip() for s in os.environ.get('ECX_ONLY', '').split(',') if s.strip()]
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {
    'ecxs_granite': 2048, 'ecxs_bronze': 512, 'ecxs_basalt': 1024,
    'ecxc_basalt': 2048, 'ecxc_granite': 2048, 'ecxc_bronze': 2048, 'ecxc_snow': 2048,
    'ecxf_copper': 2048, 'ecxf_basalt': 1024, 'ecxf_media': 1024,
    'ecxm_basalt': 2048, 'ecxm_bronze': 2048, 'ecxm_granite': 1024, 'ecxm_lit': 2048, 'ecxm_snow': 2048,
    'ecxh_steel': 1024, 'ecxh_timber': 1024, 'ecxh_glass': 1024, 'ecxh_prints': 2048, 'ecxh_snow': 1024, 'ecxh_concrete': 1024,
    'ecxt_steel': 512, 'ecxt_prints': 1024,
    'ecxl_bronze': 1024, 'ecxl_basalt': 1024, 'ecxl_frame': 1024,
    'ecxb_bronze': 512, 'ecxb_frame': 512,
    'ecxp_basalt': 2048, 'ecxp_bronze': 1024, 'ecxp_snow': 1024, 'ecxp_soil': 1024,
    'ecxn_timber': 1024, 'ecxn_basalt': 1024, 'ecxn_bronze': 1024,
    'ecxv_timber': 1024, 'ecxv_bronze': 1024, 'ecxv_leather': 1024, 'ecxv_prints': 1024,
    'ecxk_brass': 1024, 'ecxk_carpet': 1024, 'ecxk_rubber': 1024, 'ecxk_luggage': 2048,
    'ecxu_bronze': 512, 'ecxu_sand': 512,
    'ecxa_snow': 2048, 'ecxa_grime': 1024, 'ecxbb_snow': 2048, 'ecxbb_grime': 1024, 'ecxcc_snow': 2048, 'ecxcc_grime': 512,
    'ecxr_basalt': 2048, 'ecxr_bronze': 1024, 'ecxr_snow': 1024,
    'ecxw_bronze': 1024, 'ecxw_frame': 1024, 'ecxw_glass': 1024,
}
AO_DISTANCE = 0.25
REVIEW_VIEWS = None   # set by pose_for_review
REVIEW_SCREEN_LIGHT = False
TEX_DIR = os.path.join(core.ROOT, 'unreal', 'Art', 'Textures', 'ECX')
RND = random.Random(2100)

# Palette (sRGB).
EMBER_LED = 0xff6a24
WARM_GLOW = 0xffc98a
COOL_GLOW = 0xe2ecff
TRANSIT_GREEN = 0x0f6b3a
TRANSIT_GOLD = 0xf2b705
TRANSIT_WHITE = 0xeceeed
PAINT_WHITE = 0xe9e6dc
PAINT_YELLOW = 0xe0b21e


def H(h):
    return core.hex_linear(h)


# ------------------------------------------------------------------ shader helpers

def attr(m, name, out='Fac'):
    n = m.node('ShaderNodeAttribute')
    n.attribute_type = 'GEOMETRY'
    n.attribute_name = name
    return n.outputs[out]


def geo_normal(m):
    g = m.node('ShaderNodeNewGeometry')
    s = m.node('ShaderNodeSeparateXYZ')
    m.link(g.outputs['Normal'], s.inputs['Vector'])
    return s


def white(m, v):
    n = m.node('ShaderNodeTexWhiteNoise')
    n.noise_dimensions = '3D'
    m.link(v, n.inputs['Vector'])
    return n.outputs['Value']


def is_k(m, s, k):
    return m.math('LESS_THAN', m.math('ABSOLUTE', m.math('SUBTRACT', s, float(k))), 0.5)


def cmul(m, col, s):
    if isinstance(s, (int, float)):
        return m.mix(1.0, col, (s, s, s, 1.0), blend='MULTIPLY')
    return m.mix(1.0, col, vec(m, s, s, s), blend='MULTIPLY')


def set_bump(m, height, strength, distance):
    b = m.node('ShaderNodeBump', Strength=strength, Distance=distance)
    m.link(height, b.inputs['Height'])
    m.set('Normal', b.outputs['Normal'])
    return b


def ao_cavity(m, distance=0.03, gain=1.5, samples=12):
    """1 in hollows (outside occlusion), 0 on open surfaces."""
    ao = m.node('ShaderNodeAmbientOcclusion', Distance=distance)
    ao.samples = samples
    ao.only_local = True
    return m.math('MULTIPLY', m.math('SUBTRACT', 1.0, ao.outputs['AO']), gain, clamp=True)


def ground_rise(m, sep, ground, height, ragged=0.4):
    """1 at the ground (a z, or a socket), fading to 0 by `height` above it, the top edge ragged."""
    if callable(ground):
        ground = ground(m, sep)
    g = ground if not isinstance(ground, (int, float)) else None
    hz = m.math('SUBTRACT', sep.outputs['Z'], g if g is not None else float(ground))
    edge = m.math('ADD', height, m.math('MULTIPLY', m.math('SUBTRACT', noise(m, scaled(m, sep, 9.0, 9.0, 2.0), 1.0, 3.0), 0.5), height * ragged * 2.0))
    rise = m.math('SUBTRACT', 1.0, m.math('DIVIDE', hz, m.math('MAXIMUM', edge, 0.01)), clamp=True)
    # nothing below the ground line (a parapet's face running on down the slope)
    return m.math('MULTIPLY', rise, ramp(m, hz, -0.1, -0.02))


# ------------------------------------------------------------------ shared materials (§0.5)

def basalt(name, joints=None, ground=0.0, salt=0.5, streaks=0.5, wet_foot=0.5, split_above=None, polish=0.0, rough_base=0.55):
    """Honed dark basalt: fine vesicles, a slight warm-grey cloud; optional staggered panel joints (w, h) on its
    vertical faces; salt bloom and a wet band at its foot (ground: a z or a socket), snowmelt streaks down it.
    split_above: z over which the face is split-face (rough, lighter, deep bump)."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    cloud = noise(m, o, 0.9, 4.0)
    fine = noise(m, o, 45.0, 3.0)
    ves = ramp(m, noise(m, o, 260.0, 1.0), 0.71, 0.75)
    col = m.mix(ramp(m, cloud, 0.3, 0.7), H(0x23252a), H(0x2e3036))
    col = m.mix(m.math('MULTIPLY', ramp(m, noise(m, o, 0.35, 3.0), 0.55, 0.8), 0.35), col, H(0x3b3631))
    col = m.mix(m.math('MULTIPLY', fine, 0.22), col, H(0x3a3c42))
    col = m.mix(m.math('MULTIPLY', ves, 0.85), col, H(0x0e0f11))
    rough = m.math('ADD', rough_base, m.math('MULTIPLY', m.math('SUBTRACT', fine, 0.5), 0.1))
    rough = m.math('ADD', rough, m.math('MULTIPLY', ves, 0.25))
    if polish > 0.0:
        band = ramp(m, noise(m, scaled(m, sep, 2.0, 2.0, 0.6), 1.0, 2.0), 0.55, 0.62)
        rough = m.math('SUBTRACT', rough, m.math('MULTIPLY', band, 0.2 * polish))
    height = m.math('MULTIPLY', ves, -0.6)
    n = geo_normal(m)
    vert = ramp(m, m.math('ABSOLUTE', n.outputs['Z']), 0.6, 0.3)
    if joints:
        jw, jh = joints
        fx = m.math('GREATER_THAN', m.math('ABSOLUTE', n.outputs['X']), m.math('ABSOLUTE', n.outputs['Y']))
        hcoord = mix_float(m, fx, sep.outputs['X'], sep.outputs['Y'])
        row = m.math('FLOOR', m.math('DIVIDE', sep.outputs['Z'], jh))
        off = m.math('MULTIPLY', m.math('FLOORED_MODULO', row, 2.0), 0.5)
        hu = m.math('ADD', m.math('DIVIDE', hcoord, jw), off)
        dh = m.math('ABSOLUTE', m.math('SUBTRACT', m.math('FRACT', hu), 0.5))
        dz = m.math('ABSOLUTE', m.math('SUBTRACT', m.math('FRACT', m.math('DIVIDE', sep.outputs['Z'], jh)), 0.5))
        jx = ramp(m, dh, 0.5 - 0.0034 / jw, 0.5 - 0.0024 / jw)
        jz = ramp(m, dz, 0.5 - 0.0034 / jh, 0.5 - 0.0024 / jh)
        joint = m.math('MULTIPLY', m.math('MAXIMUM', jx, jz), vert)
        pid = white(m, vec(m, m.math('FLOOR', hu), row, 3.0))
        col = cmul(m, col, m.math('ADD', 0.93, m.math('MULTIPLY', pid, 0.14)))
        col = m.mix(joint, col, H(0x0b0b0c))
        rough = mix_float(m, joint, rough, 0.85)
        height = m.math('SUBTRACT', height, m.math('MULTIPLY', joint, 2.0))
    if split_above is not None:
        sp = ramp(m, sep.outputs['Z'], split_above - 0.005, split_above + 0.005)
        rock = noise(m, o, 18.0, 6.0, 0.65, 0.4)
        col = m.mix(sp, col, m.mix(ramp(m, rock, 0.3, 0.75), H(0x2c2e33), H(0x4a4b4f)))
        rough = mix_float(m, sp, rough, m.math('ADD', 0.72, m.math('MULTIPLY', rock, 0.15)))
        height = m.math('ADD', height, m.math('MULTIPLY', sp, m.math('MULTIPLY', rock, 6.0)))
    # Weather: snowmelt streaks running down the faces, salt bloom and a wet band at the foot.
    if streaks > 0.0:
        st = m.math('MULTIPLY', m.math('MULTIPLY', ramp(m, noise(m, scaled(m, sep, 26.0, 26.0, 0.9), 1.0, 3.0), 0.56, 0.76), streaks), vert)
        col = m.mix(st, col, cmul(m, col, 0.62))
        rough = mix_float(m, st, rough, 0.3)
    if wet_foot > 0.0:
        wf = m.math('MULTIPLY', ground_rise(m, sep, ground, 0.12, 0.6), wet_foot)
        col = m.mix(wf, col, cmul(m, col, 0.6))
        rough = mix_float(m, wf, rough, 0.22)
    if salt > 0.0:
        sa = m.math('MULTIPLY', m.math('MULTIPLY', ground_rise(m, sep, ground, 0.3, 0.5), ramp(m, noise(m, o, 7.0, 4.0), 0.48, 0.72)), salt * 0.5)
        sa = m.math('MULTIPLY', sa, vert)
        col = m.mix(sa, col, H(0x8d8b85))
        rough = mix_float(m, sa, rough, 0.88)
    m.set('Base Color', col)
    m.set('Roughness', rough)
    set_bump(m, height, 0.35, 0.002)
    return m


def bronze(name, rime=0.0, scuff=0.6, patina=0.5, edge=0.005, seams=None, darker=0.0, facets=False):
    """Dark statuary bronze, brushed vertically, edges rubbed lighter, patina in the hollows; rime: hoarfrost on the
    upward faces. seams: vertical panel seams every n meters along Y (cladding)."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    brush = noise(m, scaled(m, sep, 700.0, 700.0, 9.0), 1.0, 4.0)
    mott = noise(m, o, 6.0, 4.0)
    broad = noise(m, o, 2.2, 3.0)
    # the color varies broadly (cast and patinated unevenly); the brushing lives mostly in the roughness
    col = m.mix(ramp(m, broad, 0.3, 0.7), H(0x4a3423), H(0x6b4a30))
    col = m.mix(m.math('MULTIPLY', ramp(m, brush, 0.3, 0.7), 0.08), col, H(0x7a5838))
    col = m.mix(m.math('MULTIPLY', ramp(m, mott, 0.5, 0.85), 0.3 + darker), col, H(0x33251a))
    ed = m.math('MULTIPLY', convex_edges(m, tc, distance=edge, samples=12, breakup_scale=40.0, gain=4.0), scuff)
    col = m.mix(ed, col, H(0xa07c54))
    cav = ao_cavity(m, 0.03, 1.4 * patina)
    col = m.mix(cav, col, H(0x26261b))
    smudge = ramp(m, noise(m, o, 14.0, 3.0), 0.55, 0.8)
    rough = m.math('ADD', 0.38, m.math('MULTIPLY', m.math('SUBTRACT', brush, 0.5), 0.12))
    rough = m.math('SUBTRACT', rough, m.math('MULTIPLY', ed, 0.13))
    rough = m.math('ADD', rough, m.math('ADD', m.math('MULTIPLY', cav, 0.3), m.math('MULTIPLY', smudge, 0.1)))
    metal = m.math('SUBTRACT', 1.0, m.math('MULTIPLY', cav, 0.45))
    height = m.math('MULTIPLY', brush, 0.2)
    if seams:
        dy = m.math('ABSOLUTE', m.math('SUBTRACT', m.math('FRACT', m.math('DIVIDE', sep.outputs['Y'], seams)), 0.5))
        sm = ramp(m, dy, 0.5 - 0.004 / seams, 0.5 - 0.0015 / seams)
        col = m.mix(sm, col, H(0x1a130c))
        height = m.math('SUBTRACT', height, m.math('MULTIPLY', sm, 2.0))
    if facets:
        # ecx_kind 1: patinated darker (the crest's shaded faces), 2: polished bright (the snow facet, a coin plate)
        k = attr(m, 'ecx_kind')
        k1, k2 = is_k(m, k, 1), is_k(m, k, 2)
        col = m.mix(k1, col, cmul(m, col, 0.45))
        col = m.mix(k2, col, H(0xc49a6c))
        rough = mix_float(m, k1, rough, 0.55)
        rough = mix_float(m, k2, rough, 0.2)
    if rime > 0.0:
        n = geo_normal(m)
        up = m.math('MULTIPLY', ramp(m, n.outputs['Z'], 0.1, 0.55), ramp(m, noise(m, o, 70.0, 4.0), 0.3, 0.55))
        up = m.math('MULTIPLY', up, rime)
        frost = noise(m, o, 1800.0, 2.0)
        col = m.mix(up, col, m.mix(frost, H(0xc9d6df), H(0xf2f6f8)))
        rough = mix_float(m, up, rough, 0.72)
        metal = mix_float(m, up, metal, 0.0)
        height = m.math('ADD', height, m.math('MULTIPLY', up, m.math('ADD', 1.0, frost)))
    m.set('Base Color', col)
    m.set('Roughness', rough)
    m.set('Metallic', metal)
    set_bump(m, height, 0.15, 0.0008)
    return m


def granite(name, wet=False, ground=None, salt=0.0, wear=0.0, grooves=None, rough_wet=0.28):
    """Flamed light-grey granite, black and white speckle; wet: the snowmelt look baked in (base 0x5e5c59, rough
    0.22-0.35). grooves(m, sep) -> (mask 0..1 of groove floors) adds anti-slip grooves to the normal."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    mid = ramp(m, noise(m, o, 110.0, 2.0), 0.35, 0.65)
    dark = ramp(m, noise(m, o, 380.0, 1.0), 0.66, 0.70)
    lite = ramp(m, noise(m, vec(m, m.math('ADD', sep.outputs['X'], 7.3), sep.outputs['Y'], m.math('ADD', sep.outputs['Z'], 3.1)), 330.0, 1.0), 0.68, 0.72)
    col = m.mix(m.math('MULTIPLY', mid, 0.5), H(0x8c8a86), H(0x7a7874))
    col = m.mix(dark, col, H(0x2a2a2c))
    col = m.mix(m.math('MULTIPLY', lite, 0.8), col, H(0xd5d3ce))
    flame = noise(m, o, 900.0, 2.0)
    height = m.math('ADD', m.math('MULTIPLY', flame, 0.6), m.math('MULTIPLY', dark, -0.3))
    if wet:
        # treads soaked by the snowmelt, risers drying in streaks
        n = geo_normal(m)
        film = noise(m, o, 3.0, 3.0)
        wetness = m.math('MAXIMUM', ramp(m, n.outputs['Z'], 0.3, 0.8),
                         m.math('MULTIPLY', ramp(m, noise(m, scaled(m, sep, 30.0, 30.0, 2.0), 1.0, 3.0), 0.35, 0.7), 0.7))
        col = m.mix(wetness, cmul(m, col, 0.7), cmul(m, col, 0.42))
        rough = mix_float(m, wetness, 0.6, m.math('ADD', rough_wet, m.math('MULTIPLY', m.math('SUBTRACT', film, 0.5), 0.14)))
    else:
        rough = m.math('ADD', 0.75, m.math('MULTIPLY', m.math('SUBTRACT', flame, 0.5), 0.1))
    if wear > 0.0:
        # Polished paths where feet go, darker grime where they don't.
        wv = ramp(m, noise(m, scaled(m, sep, 0.8, 2.5, 1.0), 1.0, 2.0), 0.4, 0.7)
        rough = m.math('SUBTRACT', rough, m.math('MULTIPLY', wv, 0.06 * wear))
    if salt > 0.0 and ground is not None:
        sa = m.math('MULTIPLY', m.math('MULTIPLY', ground_rise(m, sep, ground, 0.08, 0.5), ramp(m, noise(m, o, 9.0, 4.0), 0.45, 0.7)), salt)
        col = m.mix(sa, col, H(0x9d9a92))
        rough = mix_float(m, sa, rough, 0.8)
    if grooves is not None:
        g = grooves(m, sep)
        col = m.mix(m.math('MULTIPLY', g, 0.6), col, cmul(m, col, 0.55))
        height = m.math('SUBTRACT', height, m.math('MULTIPLY', g, 6.0))
    m.set('Base Color', col)
    m.set('Roughness', rough)
    set_bump(m, height, 0.3, 0.0007)
    return m


def granite_black(name, lines=(), line_axis='Z'):
    """Polished black granite (basins): crystals, a few mica flecks; lines: z of limescale lines (water levels)."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    cry = ramp(m, noise(m, o, 160.0, 2.0), 0.45, 0.75)
    fleck = ramp(m, noise(m, o, 900.0, 1.0), 0.74, 0.77)
    col = m.mix(m.math('MULTIPLY', cry, 0.6), H(0x0c0d0f), H(0x18191c))
    col = m.mix(fleck, col, H(0x3a3b40))
    rough = m.math('ADD', 0.08, m.math('MULTIPLY', noise(m, o, 4.0, 3.0), 0.05))
    for z in lines:
        d = m.math('ABSOLUTE', m.math('SUBTRACT', sep.outputs['Z'], z + 0.004))
        lw = m.math('ADD', 0.012, m.math('MULTIPLY', noise(m, o, 12.0, 3.0), 0.02))
        lm = m.math('MULTIPLY', smooth_less(m, d, 0.0, 1.0), 0.0)  # placeholder (kept simple below)
        band = m.math('SUBTRACT', 1.0, m.math('DIVIDE', d, lw), clamp=True)
        band = m.math('MULTIPLY', band, m.math('ADD', 0.5, m.math('MULTIPLY', noise(m, o, 30.0, 3.0), 0.6)))
        band = m.math('ADD', band, lm)
        col = m.mix(m.math('MULTIPLY', band, 0.85), col, H(0x8e8b84))
        rough = mix_float(m, m.math('MULTIPLY', band, 0.9), rough, 0.55)
    m.set('Base Color', col)
    m.set('Roughness', rough)
    m.set('Specular IOR Level', 0.55)
    return m


def timber(name, axis='Y', board=None, cross='X', weather=0.0, dark=0.0):
    """Larch/cedar, oiled honey: grain along `axis`, +-8 % per board (boards of `board` m across along `cross`)."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    s = {'X': (1.4, 55.0, 55.0), 'Y': (55.0, 1.4, 55.0), 'Z': (55.0, 55.0, 1.4)}[axis]
    grain = noise(m, scaled(m, sep, *s), 1.0, 6.0, 0.55, 1.4)
    fig = noise(m, scaled(m, sep, s[0] * 0.15, s[1] * 0.15, s[2] * 0.15), 1.0, 3.0)
    p = {'X': (30.0, 800.0, 800.0), 'Y': (800.0, 30.0, 800.0), 'Z': (800.0, 800.0, 30.0)}[axis]
    pores = ramp(m, noise(m, scaled(m, sep, *p), 1.0, 2.0), 0.62, 0.7)
    col = m.mix(ramp(m, grain, 0.3, 0.75), H(0x8a5a33), H(0xa8703f))
    col = m.mix(m.math('MULTIPLY', ramp(m, fig, 0.45, 0.8), 0.4), col, H(0x6e4426))
    col = m.mix(m.math('MULTIPLY', pores, 0.4), col, H(0x4a2c16))
    if board:
        bid = white(m, vec(m, m.math('FLOOR', m.math('DIVIDE', sep.outputs[cross], board)), 1.7, 0.0))
        col = cmul(m, col, m.math('ADD', 0.92, m.math('MULTIPLY', bid, 0.16)))
    if dark > 0.0:
        col = cmul(m, col, 1.0 - dark)
    rough = m.math('ADD', 0.45, m.math('MULTIPLY', pores, 0.15))
    if weather > 0.0:
        ed = m.math('MULTIPLY', convex_edges(m, tc, distance=0.006, samples=10, breakup_scale=30.0, gain=4.0), weather)
        col = m.mix(ed, col, H(0x9a8f80))
        rough = mix_float(m, ed, rough, 0.6)
    m.set('Base Color', col)
    m.set('Roughness', rough)
    m.set('Specular IOR Level', 0.35)
    set_bump(m, m.math('SUBTRACT', grain, m.math('MULTIPLY', pores, 0.6)), 0.18, 0.0006)
    return m


def frame_dark(name):
    return parts.metal(name, 0x15161a, 0.45, anodized=True)


def glass_dark(name):
    return street.glass(name, 0x0b0e12, 0.04)


def snow(name, dirty=0.0, ground=None):
    """Fresh snow (the fallback the runtime M_ECX_Snow replaces): white-blue, blue in the hollows, wind ripples and
    sparkle grains in the normal; dirty: grey road spray toward the ground."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    mac = noise(m, o, 2.5, 3.0)
    col = m.mix(ramp(m, mac, 0.35, 0.7), H(0xe8edf5), H(0xdde4ef))
    cav = ao_cavity(m, 0.12, 1.3)
    col = m.mix(m.math('MULTIPLY', cav, 0.8), col, H(0xa9bcd9))
    if dirty > 0.0 and ground is not None:
        d = m.math('MULTIPLY', m.math('MULTIPLY', ground_rise(m, sep, ground, 0.35, 0.6), ramp(m, noise(m, o, 5.0, 4.0), 0.3, 0.7)), dirty)
        col = m.mix(d, col, H(0x8d8a86))
    sparkle = ramp(m, noise(m, o, 2600.0, 1.0), 0.74, 0.78)
    ripple = noise(m, scaled(m, sep, 3.5, 9.0, 3.5), 1.0, 3.0, 0.5, 0.6)
    m.set('Base Color', col)
    m.set('Roughness', m.math('SUBTRACT', m.math('ADD', 0.62, m.math('MULTIPLY', m.math('SUBTRACT', mac, 0.5), 0.08)), m.math('MULTIPLY', sparkle, 0.4)))
    set_bump(m, m.math('ADD', m.math('MULTIPLY', ripple, 1.0), m.math('MULTIPLY', sparkle, 0.4)), 0.25, 0.004)
    return m


def steel_black(name, ground=0.0):
    """Black powder-coated steel: satin, orange peel, a few chips, salt splash and grime at the foot."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    from artkit.shade import powder_coat
    col, rough, metal, bump = powder_coat(m, tc, 0x16181b, rough=0.5, chip_hex=0x6d7076, chips=0.5)
    sa = m.math('MULTIPLY', ground_rise(m, sep, ground, 0.25, 0.6), ramp(m, noise(m, tc.outputs['Object'], 8.0, 4.0), 0.35, 0.7))
    col = m.mix(m.math('MULTIPLY', sa, 0.7), col, H(0x77736c))
    m.set('Base Color', col)
    m.set('Roughness', mix_float(m, sa, rough, 0.8))
    m.set('Metallic', mix_float(m, sa, metal, 0.0))
    m.set('Normal', bump.outputs['Normal'])
    return m


def concrete(name, hex_color=0x9a968e):
    return street.concrete(name, hex_color, 0.86)


def soil(name):
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    clod = noise(m, o, 30.0, 5.0)
    bark = ramp(m, noise(m, o, 9.0, 3.0, 0.6, 1.0), 0.55, 0.62)
    col = m.mix(ramp(m, clod, 0.3, 0.7), H(0x1a130e), H(0x2a2018))
    col = m.mix(m.math('MULTIPLY', bark, 0.7), col, H(0x4a2e1c))
    m.set('Base Color', col)
    m.set('Roughness', m.math('ADD', 0.78, m.math('MULTIPLY', clod, 0.15)))
    set_bump(m, m.math('ADD', clod, bark), 0.6, 0.006)
    return m


def grime(name, ground=0.0):
    """Grey-brown icy slush, wet and gritty (a plowed bank's foot)."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    lump = noise(m, o, 9.0, 5.0)
    grit = ramp(m, noise(m, o, 700.0, 1.0), 0.66, 0.7)
    rust = ramp(m, noise(m, o, 3.0, 3.0), 0.55, 0.75)
    col = m.mix(ramp(m, lump, 0.25, 0.75), H(0x4a4743), H(0x6e6a64))
    col = m.mix(m.math('MULTIPLY', rust, 0.5), col, H(0x5a4a3a))
    col = m.mix(grit, col, H(0x1e1c1a))
    ice = ramp(m, noise(m, o, 4.0, 3.0), 0.6, 0.72)
    col = m.mix(m.math('MULTIPLY', ice, 0.4), col, H(0x9aa4ad))
    m.set('Base Color', col)
    m.set('Roughness', m.math('ADD', 0.22, m.math('MULTIPLY', m.math('SUBTRACT', 1.0, ice), m.math('MULTIPLY', lump, 0.3))))
    set_bump(m, m.math('ADD', lump, m.math('MULTIPLY', grit, 0.5)), 0.5, 0.004)
    return m


# ------------------------------------------------------------------ geometry helpers

def box(x0, x1, y0, y1, z0, z1, mat, r=0.0, seg=2, name='box'):
    o = parts.rbox(name, ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), (abs(x1 - x0), abs(y1 - y0), abs(z1 - z0)), r, seg)
    if mat is not None:
        core.assign(o, mat)
    return o


def mbox(X0, X1, Y0, Y1, Z0, Z1, mat, r=0.0, seg=2):
    """A box given in map meters (X toward the valley), built in Blender's SITE frame (x = -X)."""
    return box(-X1, -X0, Y0, Y1, Z0, Z1, mat, r, seg)


def set_attr(obj, name, value):
    me = obj.data
    n = len(me.polygons)
    a = me.attributes.get(name) or me.attributes.new(name, 'FLOAT', 'FACE')
    a.data.foreach_set('value', [float(value)] * n)
    return obj


def tag(obj, kind):
    return set_attr(obj, 'ecx_kind', kind)


def prism_y(name, prof, y0, y1, mat=None):
    """A prism along Y from a closed (x, z) profile."""
    bm = bmesh.new()
    a = [bm.verts.new((x, y0, z)) for x, z in prof]
    b = [bm.verts.new((x, y1, z)) for x, z in prof]
    n = len(prof)
    bm.faces.new(a)
    bm.faces.new(list(reversed(b)))
    for i in range(n):
        bm.faces.new((a[i], a[(i + 1) % n], b[(i + 1) % n], b[i]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    o = core.mesh_object(name, bm)
    if mat is not None:
        core.assign(o, mat)
    return o


def prism_x(name, prof, x0, x1, mat=None):
    """A prism along X from a closed (y, z) profile."""
    o = core.prism_x(name, prof, x0, x1)
    if mat is not None:
        core.assign(o, mat)
    return o


def lathe(name, profile, mat=None, seg=48, matrix=None):
    o = parts.lathe_at(name, profile, matrix if matrix is not None else Matrix(), seg)
    if mat is not None:
        core.assign(o, mat)
    return o


def rod(a, b, r, mat, n=16):
    o = parts.rod('rod', a, b, r, n)
    core.assign(o, mat)
    return o


def tube(points, r, mat, n=12, steps=8):
    o = parts.tube('tube', points, r, n, steps)
    core.assign(o, mat)
    return o


def grid_mesh(name, xs, ys, zf, mat=None, flip=False):
    """A height-field sheet over the grid xs x ys (z = zf(x, y)), normals up."""
    bm = bmesh.new()
    vs = [[bm.verts.new((x, y, zf(x, y))) for x in xs] for y in ys]
    for j in range(len(ys) - 1):
        for i in range(len(xs) - 1):
            q = (vs[j][i], vs[j][i + 1], vs[j + 1][i + 1], vs[j + 1][i])
            bm.faces.new(tuple(reversed(q)) if flip else q)
    o = core.mesh_object(name, bm)
    if mat is not None:
        core.assign(o, mat)
    return o


def text_mesh(body, size, face='Black', align='CENTER', tracking=1.0, extrude=0.0, offset=0.0, bevel=0.0, res=3):
    """Text converted to a mesh in its own XY plane (reads +X, up +Y; extruded along Z by +-extrude)."""
    cu = bpy.data.curves.new('txt', 'FONT')
    cu.body = body
    cu.font = bpy.data.fonts.load(os.path.join(core.FONTS, f'Roboto-{face}.ttf'), check_existing=True)
    cu.size = size
    cu.align_x = align
    cu.space_character = tracking
    cu.extrude = extrude
    cu.offset = offset
    cu.bevel_depth = bevel
    cu.bevel_resolution = 1 if bevel > 0 else 0
    cu.resolution_u = res
    obj = core.link(bpy.data.objects.new('txt', cu))
    core.select_only([obj])
    bpy.ops.object.convert(target='MESH')
    obj = bpy.context.view_layer.objects.active
    obj.name = 'txt'
    me = obj.data
    bm = bmesh.new()
    bm.from_mesh(me)
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=1e-6)
    bm.to_mesh(me)
    bm.free()
    return obj


def text_bounds(obj):
    xs = [v.co.x for v in obj.data.vertices]
    ys = [v.co.y for v in obj.data.vertices]
    return min(xs), max(xs), min(ys), max(ys)


def face_matrix(front, origin_u, origin_v, depth):
    """Maps a text's XY (u right, v up as a viewer outside the face sees it) onto a face of a PROP: front=True is
    the -X face (u runs toward -Y), False the +X face (u toward +Y); depth = x of the text's z = 0 plane."""
    if front:
        rot = Matrix(((0.0, 0.0, -1.0), (-1.0, 0.0, 0.0), (0.0, 1.0, 0.0)))
        t = Vector((depth, -origin_u, origin_v))
    else:
        rot = Matrix(((0.0, 0.0, 1.0), (1.0, 0.0, 0.0), (0.0, 1.0, 0.0)))
        t = Vector((depth, origin_u, origin_v))
    m = rot.to_4x4()
    m.translation = t
    return m


def order_slots(obj, mats):
    """Puts obj's material slots in the order of mats (§0.4) and remaps the faces."""
    me = obj.data
    old = [s.material for s in obj.material_slots]
    remap = []
    for mt in old:
        if mt not in mats:
            raise RuntimeError(f'{obj.name}: material {mt.name if mt else None} not in its slot list')
        remap.append(mats.index(mt))
    fi = np.zeros(len(me.polygons), dtype=np.int32)
    me.polygons.foreach_get('material_index', fi)
    new = np.array(remap, dtype=np.int32)[fi] if len(remap) else fi
    me.materials.clear()
    for mt in mats:
        me.materials.append(mt)
    me.polygons.foreach_set('material_index', new)
    used = set(new.tolist())
    for i, mt in enumerate(mats):
        if i not in used:
            print(f'[ecx_plaza] WARNING {obj.name}: slot {i} ({mt.name}) has no faces')


def finish_mesh(objs, name, mats, angle=35.0, rects=None, weighted=True):
    """Joins parts, orders the slots as mats, shades hard-surface, lays out UVs: faces flagged ecx_uvlock keep
    the UVs they were given (prints), every other face is box-projected per slot (into rects[slot name] if given)."""
    for o in objs:
        if not o.data.uv_layers:
            o.data.uv_layers.new(name='UVMap')
    obj = core.join(objs, name) if len(objs) > 1 else objs[0]
    obj.name = name
    obj.data.name = name
    order_slots(obj, [m.m if isinstance(m, core.Mat) else m for m in mats])
    core.finish_hard_surface(obj, angle, weighted)
    lock = obj.data.attributes.get('ecx_uvlock')
    locked = set()
    if lock is not None:
        vals = np.zeros(len(obj.data.polygons), dtype=np.float32)
        lock.data.foreach_get('value', vals)
        locked = set(np.nonzero(vals > 0.5)[0].tolist())
    rects = rects or {}
    groups = []
    if locked:
        groups.append((lambda f: f.index in locked, 'keep', None, (0.0, 0.0, 1.0, 1.0)))
    for s in obj.material_slots:
        groups.append((core.material_is(obj, s.material.name), 'box', None, rects.get(s.material.name, (0.0, 0.0, 1.0, 1.0))))
    groups.append((lambda f: True, 'box', None, (0.0, 0.0, 1.0, 1.0)))
    core.uv_layout(obj, groups)
    print(f'[ecx_plaza] {name} slots: {[s.material.name for s in obj.material_slots]}  tris {tri_count(obj)}')
    return obj


def tri_count(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


def glow(objs, name):
    o = parts.glow_object(objs, name)
    o.data.materials[0].use_backface_culling = True
    print(f'[ecx_plaza] {name} tris {tri_count(o)}')
    return o


def plain_material(name, hex_color, rough=0.5, metal=0.0, cull=True, emit=None, emit_strength=1.0):
    """An unbaked material named exactly `name` (Unreal finds the slot by it)."""
    m = core.Mat(name)
    m.set('Base Color', H(hex_color))
    m.set('Roughness', rough)
    m.set('Metallic', metal)
    if emit is not None:
        m.set('Emission Color', H(emit))
        m.set('Emission Strength', emit_strength)
    m.m.use_backface_culling = cull
    if m.m.name != name:
        raise RuntimeError(f'material name clash: wanted {name}, got {m.m.name}')
    return m


def lock_uvs(obj, uvfun):
    """Sets obj's UVs per loop from uvfun(co) and flags its faces to keep them through finish_mesh."""
    me = obj.data
    uv = me.uv_layers.get('UVMap') or me.uv_layers.new(name='UVMap')
    for poly in me.polygons:
        for li in poly.loop_indices:
            co = me.vertices[me.loops[li].vertex_index].co
            uv.data[li].uv = uvfun(co)
    set_attr(obj, 'ecx_uvlock', 1.0)
    return obj


def place_text(obj, matrix):
    obj.data.transform(matrix)
    return obj


# ------------------------------------------------------------------ noise for geometry (numpy)

def _hash2(ix, iy, seed):
    h = (ix.astype(np.int64) * np.int64(73856093)) ^ (iy.astype(np.int64) * np.int64(19349663)) ^ np.int64(seed * 83492791 + 12345)
    h = (h ^ (h >> 13)) * np.int64(1274126177)
    h = h ^ (h >> 16)
    return (h & 0xFFFFFF).astype(np.float64) / float(0xFFFFFF)


def vnoise2(X, Y, scale, seed=0, octaves=3, gain=0.5):
    """Smooth 2D value noise in 0..1 at arrays X, Y (meters); scale = feature size."""
    X = np.asarray(X, dtype=np.float64)
    Y = np.asarray(Y, dtype=np.float64)
    out = np.zeros_like(X)
    amp, tot, s = 1.0, 0.0, scale
    for o in range(octaves):
        qx, qy = X / s, Y / s
        ix, iy = np.floor(qx), np.floor(qy)
        fx, fy = qx - ix, qy - iy
        fx = fx * fx * fx * (fx * (fx * 6 - 15) + 10)
        fy = fy * fy * fy * (fy * (fy * 6 - 15) + 10)
        sd = seed + o * 31
        a = _hash2(ix, iy, sd)
        b = _hash2(ix + 1, iy, sd)
        c = _hash2(ix, iy + 1, sd)
        d = _hash2(ix + 1, iy + 1, sd)
        out += amp * ((a * (1 - fx) + b * fx) * (1 - fy) + (c * (1 - fx) + d * fx) * fy)
        tot += amp
        amp *= gain
        s *= 0.5
    return out / tot


def smoothstep(e0, e1, x):
    t = np.clip((np.asarray(x, dtype=np.float64) - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def seg_distance(P, segs):
    """Distance from points P (N x 2) to a list of segments ((x0, y0), (x1, y1))."""
    P = np.asarray(P, dtype=np.float64)
    best = np.full(len(P), 1e9)
    if not segs:
        return best
    A = np.array([s[0] for s in segs], dtype=np.float64)
    B = np.array([s[1] for s in segs], dtype=np.float64)
    for c0 in range(0, len(A), 256):
        a = A[c0:c0 + 256][None, :, :]
        b = B[c0:c0 + 256][None, :, :]
        ab = b - a
        pa = P[:, None, :] - a
        t = np.clip((pa * ab).sum(2) / np.maximum((ab * ab).sum(2), 1e-12), 0.0, 1.0)
        d = np.linalg.norm(pa - t[..., None] * ab, axis=2)
        best = np.minimum(best, d.min(axis=1))
    return best


def path_segs(pts):
    return [(tuple(pts[i][:2]), tuple(pts[i + 1][:2])) for i in range(len(pts) - 1)]


# ================================================================== SM_ECX_Ground

# Plan (map meters). See §2.3. Curb face lines are the drive's edges; curb bodies (CW wide) lie on the upper side.
CW = 0.18
APRON_X = 8.0
LANE_X = (8.0, 16.5)
DIV_X = (11.875, 12.125)
TABLE_Y = 3.5
RAMP_Y = 5.0
TOP_X1 = 23.2
WALLS = (24.0, 35.6, 47.2)
WALL_T = 0.35
FLIGHTS = ((23.2, 24.8), (34.8, 36.4), (46.4, 48.0))
LAND_B = (24.8, 34.8)
LAND_A = (36.4, 46.4)
CAS_X = (25.2, 46.4)
CAS_Y = (4.6, 14.9)
STEP_Y = 4.0
PLAT_X = (48.0, 52.5)
PLAT_Y = 22.0
BAY_X = (52.5, 56.0)
ROAD_X = (56.0, 63.0)
ROAD_C = 59.5
SHOULDER_E = 55.0
PARAPET_X = 64.0
OUT_X = 64.45
OVER = ((64.0, 70.45), (5.55, 20.45))
ARM_Y = (26.5, 33.5)
CURVE_C = (21.25, 21.0)
OUT_Y = 42.0
ROAD_Y = (-200.0, 120.0)
LEVEL_B, LEVEL_A, LEVEL_P, LEVEL_BAY = -0.75, -1.5, -2.25, -2.40
FILLET_OUT = 3.0     # the arms' outer corners at the road
FILLET_IN = 1.0      # the north arm's inner corner
NOSE = 0.4           # the south island nose between the arm and the bay taper
# Where the island curb runs along the bay: south taper foot, the straight, the north taper's end (§2.3 bay).
BAY_S0 = (54.8, -26.5)
BAY_S1 = (52.5, -18.0)
BAY_N0 = (52.5, 8.0)
BAY_N1 = (55.0, 18.0)


def arm_z(X):
    X = np.asarray(X, dtype=np.float64)
    L, rmp = 34.75, 3.0
    g = 2.28 / (L - rmp)
    s = np.clip(X - CURVE_C[0], 0.0, L)
    z = np.where(s < rmp, g * s * s / (2 * rmp),
                 np.where(s <= L - rmp, g * (s - rmp / 2), 2.28 - g * (L - s) ** 2 / (2 * rmp)))
    return -0.12 - z


def road_profile(Y):
    Y = np.asarray(Y, dtype=np.float64)
    Lc = 10.0
    d = Y - (-42.0)
    fs = np.where(d < -Lc / 2, 0.04 * d, np.where(d > Lc / 2, 0.0, 0.04 * (d - (d + Lc / 2) ** 2 / (2 * Lc))))
    d = Y - 60.0
    fn = np.where(d < -Lc / 2, 0.0, np.where(d > Lc / 2, 0.02 * d, 0.02 * (d + Lc / 2) ** 2 / (2 * Lc)))
    return LEVEL_BAY + fs + fn


def road_z(X, Y):
    X = np.asarray(X, dtype=np.float64)
    crown = 0.025 * np.clip(1.0 - np.abs(X - ROAD_C) / 3.5, 0.0, 1.0)
    return road_profile(Y) + np.where((X >= ROAD_X[0]) & (X <= ROAD_X[1]), crown, 0.0)


def lane_z(X, Y):
    X = np.asarray(X, dtype=np.float64)
    ay = np.abs(np.asarray(Y, dtype=np.float64))
    gut = smoothstep(0.35, 0.0, np.minimum(np.abs(X - LANE_X[0]), np.abs(X - LANE_X[1])))
    lane = -0.12 - 0.012 * gut
    t = np.clip((ay - TABLE_Y) / (RAMP_Y - TABLE_Y), 0.0, 1.0)
    return np.where(ay <= TABLE_Y, 0.0, np.where(ay < RAMP_Y, lane * t, lane))


def bed_w(ay):
    return smoothstep(20.5, ARM_Y[0] - CW, ay)


def terrace(X):
    X = np.asarray(X, dtype=np.float64)
    return np.where(X < WALLS[0], 0.0, np.where(X < WALLS[1], LEVEL_B, np.where(X < WALLS[2], LEVEL_A, LEVEL_P)))


def r_in(theta_deg):
    """The curves' inner edge radius at theta (deg, 180 at the lanes, 90 at the arm): 4.75 -> 5.5 m."""
    return 4.75 + 0.75 * float(np.clip((180.0 - theta_deg) / 90.0, 0.0, 1.0))


def r_out(theta_deg):
    """The curves' outer edge radius: 13.25 -> 12.5 m."""
    return 13.25 - 0.75 * float(np.clip((180.0 - theta_deg) / 90.0, 0.0, 1.0))


def curve_pt(sgn, theta, r):
    """A point at angle theta (deg, 90..180 on the north curve) and radius r about the curve's center; sgn mirrors."""
    a = math.radians(theta)
    return (CURVE_C[0] + r * math.cos(a), sgn * (CURVE_C[1] + r * math.sin(a)))


def arc_pts(c, r, a0, a1, n):
    return [(c[0] + r * math.cos(math.radians(a0 + (a1 - a0) * i / n)), c[1] + r * math.sin(math.radians(a0 + (a1 - a0) * i / n))) for i in range(n + 1)]


def fillet_pts(p0, corner, p1, r, n=10):
    """An arc of radius r rounding the corner between the lines p0-corner and corner-p1 (its tangent points
    included), from the p0 side to the p1 side."""
    c = np.array(corner, float)
    d0 = np.array(p0, float) - c
    d1 = np.array(p1, float) - c
    d0 /= np.linalg.norm(d0)
    d1 /= np.linalg.norm(d1)
    ang = math.acos(max(-1.0, min(1.0, float(d0 @ d1))))
    t = r / math.tan(ang / 2)
    a = c + d0 * t
    b = c + d1 * t
    bis = d0 + d1
    bis /= np.linalg.norm(bis)
    center = c + bis * (r / math.sin(ang / 2))
    a0 = math.atan2(a[1] - center[1], a[0] - center[0])
    a1 = math.atan2(b[1] - center[1], b[0] - center[0])
    da = (a1 - a0 + math.pi) % (2 * math.pi) - math.pi
    return [(center[0] + r * math.cos(a0 + da * i / n), center[1] + r * math.sin(a0 + da * i / n)) for i in range(n + 1)], tuple(center)


def offset_line(p0, p1, d):
    """The segment p0-p1 moved by d to its left."""
    t = np.array(p1, float) - np.array(p0, float)
    t /= np.linalg.norm(t)
    n = np.array((-t[1], t[0])) * d
    return tuple(np.array(p0, float) + n), tuple(np.array(p1, float) + n)


def line_x_at(p0, p1, y):
    t = (y - p0[1]) / (p1[1] - p0[1])
    return p0[0] + t * (p1[0] - p0[0])


# ---------------------------------------------------------------- polygon tools

def poly_area(p):
    a = 0.0
    for i in range(len(p)):
        x0, y0 = p[i]
        x1, y1 = p[(i + 1) % len(p)]
        a += x0 * y1 - x1 * y0
    return a / 2


def ccw(p):
    return list(p) if poly_area(p) >= 0 else list(reversed(p))


def clip_convex(subject, clip):
    """Sutherland-Hodgman: subject polygon clipped by a convex CCW polygon."""
    out = list(subject)
    n = len(clip)
    for i in range(n):
        if not out:
            break
        ax, ay = clip[i]
        bx, by = clip[(i + 1) % n]
        ex, ey = bx - ax, by - ay
        inp, out = out, []
        s = inp[-1]
        ss = ex * (s[1] - ay) - ey * (s[0] - ax)
        for p in inp:
            sp = ex * (p[1] - ay) - ey * (p[0] - ax)
            if sp >= -1e-12:
                if ss < -1e-12:
                    t = ss / (ss - sp)
                    out.append((s[0] + t * (p[0] - s[0]), s[1] + t * (p[1] - s[1])))
                out.append(p)
            elif ss >= -1e-12:
                t = ss / (ss - sp)
                out.append((s[0] + t * (p[0] - s[0]), s[1] + t * (p[1] - s[1])))
            s, ss = p, sp
    return out


def clean_poly(p, eps=1e-6):
    out = []
    for q in p:
        if not out or abs(q[0] - out[-1][0]) > eps or abs(q[1] - out[-1][1]) > eps:
            out.append(q)
    while len(out) > 1 and abs(out[0][0] - out[-1][0]) <= eps and abs(out[0][1] - out[-1][1]) <= eps:
        out.pop()
    # drop collinear points
    res = []
    n = len(out)
    for i in range(n):
        a, b, c = out[i - 1], out[i], out[(i + 1) % n]
        cr = (b[0] - a[0]) * (c[1] - b[1]) - (b[1] - a[1]) * (c[0] - b[0])
        if abs(cr) > 1e-10:
            res.append(b)
    return res


def rect(x0, x1, y0, y1):
    return [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]


def mirror_y(p):
    return [(x, -y) for x, y in reversed(p)]


def point_in_poly(x, y, poly):
    inside = False
    n = len(poly)
    for i in range(n):
        x0, y0 = poly[i]
        x1, y1 = poly[(i + 1) % n]
        if (y0 > y) != (y1 > y):
            xi = x0 + (y - y0) * (x1 - x0) / (y1 - y0)
            if x < xi:
                inside = not inside
    return inside


OUTLINE = [(0, -18.5), (0, 18.5), (3, 18.5), (3, 42), (55, 42), (55, 120), (64.45, 120), (64.45, 20.45), (70.45, 20.45),
           (70.45, 5.55), (64.45, 5.55), (64.45, -200), (55, -200), (55, -42), (3, -42), (3, -18.5)]


# ---------------------------------------------------------------- the builder

class GroundBuilder:
    """Collects the ground's faces in map meters: horizontal regions (convex pieces gridded and welded),
    swept curbs and walls, markings; then builds the Blender mesh (x = -X)."""

    MATS = ['ecx_paver', 'ecx_sett', 'ecx_asphalt', 'ecx_curb', 'ecx_concrete', 'ecx_wallstone', 'ecx_snowground', 'ecx_marking']

    def __init__(self):
        self.faces = []        # (mat, [(X, Y, Z)], [(u, v)], [(r, g, b)])
        self.edge_segs = []    # boundaries snow tapers to (curb faces, walls, edgings, the cascades)
        self.curb_segs = []    # curb face lines (for gutters and slush)
        self.snow_segs = []    # where snow meets paving or drive (slush on the unheated side)

    # ---- regions
    def region(self, pieces, mat, zf, cell, colf, uvf=None):
        polys = []
        cx, cy = cell if isinstance(cell, tuple) else (cell, cell)
        for piece in pieces:
            piece = ccw(clean_poly(piece))
            if len(piece) < 3 or abs(poly_area(piece)) < 1e-6:
                continue
            xs = [p[0] for p in piece]
            ys = [p[1] for p in piece]
            for i in range(int(math.floor(min(xs) / cx)), int(math.ceil(max(xs) / cx))):
                for j in range(int(math.floor(min(ys) / cy)), int(math.ceil(max(ys) / cy))):
                    cellp = [(i * cx, j * cy), ((i + 1) * cx, j * cy), ((i + 1) * cx, (j + 1) * cy), (i * cx, (j + 1) * cy)]
                    q = clean_poly(clip_convex(cellp, piece))
                    if len(q) >= 3 and abs(poly_area(q)) > 2e-5:
                        polys.append(ccw(q))
        if not polys:
            return
        pts = np.array([p for q in polys for p in q], dtype=np.float64)
        Z = zf(pts[:, 0], pts[:, 1])
        C = colf(pts[:, 0], pts[:, 1], Z)
        k = 0
        for q in polys:
            n = len(q)
            vs = [(pts[k + i, 0], pts[k + i, 1], float(Z[k + i])) for i in range(n)]
            uvs = [(pts[k + i, 0], pts[k + i, 1]) for i in range(n)] if uvf is None else [uvf(pts[k + i, 0], pts[k + i, 1]) for i in range(n)]
            cs = [tuple(C[k + i]) for i in range(n)]
            self.faces.append((mat, vs, uvs, cs))
            k += n

    # ---- swept profiles (curbs, walls, edgings)
    def sweep(self, path, mat, prof_fn, upper_left=True, colf=None, caps=True, step=0.5, closed=False):
        """path: list of (X, Y) points (dense on arcs). prof_fn(X, Y, s) -> (list of (lat, z) points of a closed
        profile, lat measured toward the upper side; the set of profile edge indices that are the top). Faces get
        u = s (meters along), v = z; top edges get map UVs. closed: the path loops (no caps)."""
        pts = []
        for p in path:
            p = np.array(p[:2], float)
            if not pts or np.linalg.norm(p - pts[-1]) > 1e-5:
                pts.append(p)
        if closed:
            if np.linalg.norm(pts[0] - pts[-1]) < 1e-5:
                pts.pop()
            pts = pts + [pts[0]]
        dense = [pts[0]]
        for a, b in zip(pts[:-1], pts[1:]):
            L = np.linalg.norm(b - a)
            n = max(1, int(math.ceil(L / step)))
            for i in range(1, n + 1):
                dense.append(a + (b - a) * i / n)
        if closed:
            dense.pop()
        pts = dense
        n = len(pts)
        s_acc = [0.0]
        for i in range(1, n + (1 if closed else 0)):
            s_acc.append(s_acc[-1] + float(np.linalg.norm(pts[i % n] - pts[i - 1])))
        rings = []
        for i in range(n):
            if closed:
                a, b = pts[i - 1], pts[(i + 1) % n]
            else:
                a, b = pts[max(i - 1, 0)], pts[min(i + 1, n - 1)]
            t = b - a
            t /= max(np.linalg.norm(t), 1e-9)
            nl = np.array((-t[1], t[0])) if upper_left else np.array((t[1], -t[0]))
            prof, top = prof_fn(pts[i][0], pts[i][1], s_acc[i])
            ring = [(pts[i][0] + nl[0] * lat, pts[i][1] + nl[1] * lat, z) for lat, z in prof]
            rings.append((ring, top))
        m = len(rings[0][0])
        for i in range(n if closed else n - 1):
            r0, top = rings[i]
            r1, _ = rings[(i + 1) % n]
            s0, s1 = s_acc[i], s_acc[i + 1]
            for k in range(m):
                k1 = (k + 1) % m
                quad = [r0[k], r0[k1], r1[k1], r1[k]]
                if k in top:
                    uvs = [(q[0], q[1]) for q in quad]
                else:
                    uvs = [(s0, r0[k][2]), (s0, r0[k1][2]), (s1, r1[k1][2]), (s1, r1[k][2])]
                cols = [colf(q[0], q[1], q[2]) if colf else (0.0, 0.0, 0.0) for q in quad]
                self.faces.append((mat, quad, uvs, cols))
        if caps and not closed:
            for idx, rev in ((0, True), (n - 1, False)):
                ring = rings[idx][0]
                poly = list(reversed(ring)) if rev else list(ring)
                uvs = [(q[0] + q[1], q[2]) for q in poly]
                cols = [colf(q[0], q[1], q[2]) if colf else (0.0, 0.0, 0.0) for q in poly]
                self.faces.append((mat, poly, uvs, cols))
        return pts

    def curb(self, path, top_fn, low_fn, back_fn=None, upper_left=True, mat=3, width=CW, chamfer=0.012, colf=None, closed=False,
             record=True):
        def prof(X, Y, s):
            top = float(top_fn(X, Y))
            low = float(low_fn(X, Y))
            back = float(back_fn(X, Y)) if back_fn else top - 0.003
            ch = min(chamfer, max(top - low, 0.0) * 0.5 + 0.002)
            return ([(0.0, min(low, top) - 0.06), (0.0, top - ch), (ch, top), (width - 0.006, top), (width, top - 0.006),
                     (width, min(back, top) - 0.08)], {2})
        pts = self.sweep(path, mat, prof, upper_left, colf, closed=closed)
        if record:
            segs = path_segs(list(path) + ([path[0]] if closed else []))
            self.curb_segs += segs
            self.edge_segs += segs
        return pts

    def wall(self, path, top_fn, low_fn, back_fn, upper_left=True, thick=WALL_T, colf=None):
        def prof(X, Y, s):
            top = float(top_fn(X, Y))
            low = float(low_fn(X, Y))
            back = float(back_fn(X, Y))
            return ([(-0.012, low - 0.12), (-0.012, top - 0.07), (-0.03, top - 0.06), (-0.03, top - 0.012), (-0.018, top), (thick + 0.006, top),
                     (thick + 0.018, top - 0.012), (thick + 0.018, top - 0.06), (thick, top - 0.07), (thick, min(back, top) - 0.2)], {4})
        pts = self.sweep(path, 5, prof, upper_left, colf, step=0.4)
        self.edge_segs += path_segs(list(path))
        return pts

    def solid_box(self, X0, X1, Y0, Y1, Z0, Z1, mat, col=(0.0, 0.0, 0.0)):
        """An axis-aligned box (map frame), faces counter-clockwise seen from outside."""
        c = [(X0, Y0, Z0), (X1, Y0, Z0), (X1, Y1, Z0), (X0, Y1, Z0), (X0, Y0, Z1), (X1, Y0, Z1), (X1, Y1, Z1), (X0, Y1, Z1)]
        quads = [(4, 5, 6, 7), (3, 2, 1, 0), (0, 1, 5, 4), (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)]
        for q in quads:
            vs = [c[i] for i in q]
            uvs = [(v[0], v[1]) if q == (4, 5, 6, 7) else (v[0] + v[1], v[2]) for v in vs]
            self.faces.append((mat, vs, uvs, [col] * 4))

    # ---- markings
    def marking(self, poly, paint, zf, lift=0.002):
        col = tuple(H(paint)[:3])
        q = ccw(poly)
        vs = [(x, y, float(zf(np.array([x]), np.array([y]))[0]) + lift) for x, y in q]
        self.faces.append((7, vs, [(x, y) for x, y in q], [col] * len(q)))

    def strip(self, x0, x1, y0, y1, paint, zf, seg=1.0):
        n = max(1, int(math.ceil((y1 - y0) / seg)))
        ys = [y0 + (y1 - y0) * i / n for i in range(n + 1)]
        for a, b in zip(ys[:-1], ys[1:]):
            self.marking(rect(x0, x1, a, b), paint, zf)

    # ---- the mesh
    def build(self, name):
        bm = bmesh.new()
        uvl = bm.loops.layers.uv.new('UVMap')
        col = bm.loops.layers.float_color.new('Color')
        vmap = {}

        def V(p):
            key = (round(p[0] * 4000), round(p[1] * 4000), round(p[2] * 4000))
            v = vmap.get(key)
            if v is None:
                v = bm.verts.new((-p[0], p[1], p[2]))
                vmap[key] = v
            return v
        mats = []
        for f in self.faces:
            mat, vs, uvs, cs = f[:4]
            verts = [V(p) for p in vs]
            if len(set(verts)) < 3:
                continue
            # map frame -> Blender mirrors X: reverse the winding so normals stay as built
            order = list(range(len(verts)))[::-1]
            try:
                face = bm.faces.new([verts[i] for i in order])
            except ValueError:
                continue
            face.material_index = mat
            for loop, i in zip(face.loops, order):
                loop[uvl].uv = uvs[i]
                c = cs[i]
                loop[col] = (float(c[0]), float(c[1]), float(c[2]), 1.0)
            mats.append(face)
        bm.normal_update()
        # Swept solids: make each closed piece's normals point out (their winding depends on the side they face).
        obj = core.mesh_object(name, bm)
        me = obj.data
        ca = me.color_attributes.get('Color')
        if ca is not None:
            me.color_attributes.active_color = ca
            me.color_attributes.render_color_index = me.color_attributes.find('Color')
        return obj


def _orient_solids(obj):
    """Recalculates normals on the closed swept pieces (curbs, walls: their winding depends on the side they face)."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.faces.ensure_lookup_table()
    solid = [f for f in bm.faces if f.material_index in (3, 5)]
    if solid:
        bmesh.ops.recalc_face_normals(bm, faces=solid)
    bm.to_mesh(obj.data)
    bm.free()


def _skirts(obj, tests, depth_fn):
    """Hangs a vertical skirt from every open edge of the ground whose two ends pass one of `tests` (map-frame
    predicates on (X, Y)), so the terrain's edge never shows a gap under it."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    uvl = bm.loops.layers.uv.active
    col = bm.loops.layers.float_color.get('Color')
    edges = [e for e in bm.edges if e.is_boundary]
    made = 0
    for e in edges:
        a, b = e.verts
        pa, pb = (-a.co.x, a.co.y), (-b.co.x, b.co.y)
        if not any(t(*pa) and t(*pb) for t in tests):
            continue
        f = e.link_faces[0]
        if f.material_index in (3, 5, 7):
            continue
        # keep the face's winding: the edge runs a->b in the face, so the skirt runs b->a on top
        loops = {l.vert: l for l in f.loops}
        la = loops[a]
        if la.link_loop_next.vert is not b:
            a, b = b, a
            pa, pb = pb, pa
        d = depth_fn(*pa)
        a2 = bm.verts.new((a.co.x, a.co.y, a.co.z - d))
        b2 = bm.verts.new((b.co.x, b.co.y, b.co.z - d))
        try:
            nf = bm.faces.new((b, a, a2, b2))
        except ValueError:
            continue
        nf.material_index = f.material_index
        L = (Vector(pa) - Vector(pb)).length
        for loop, uv in zip(nf.loops, ((0.0, b.co.z), (L, a.co.z), (L, a2.co.z), (0.0, b2.co.z))):
            loop[uvl].uv = uv
            if col is not None:
                loop[col] = loops[a][col] if loop.vert in (a, a2) else loops[b if b in loops else a][col]
        made += 1
    bm.to_mesh(obj.data)
    bm.free()
    return made


def ground_material(name, hex_color, rough):
    """An unbaked ground slot (named for Unreal) whose base color reads the vertex colors, so the glTF carries
    them as COLOR_0."""
    m = core.Mat(name)
    va = m.node('ShaderNodeVertexColor')
    va.layer_name = 'Color'
    mx = m.nt.nodes.new('ShaderNodeMix')
    mx.data_type = 'RGBA'
    mx.blend_type = 'MULTIPLY'
    mx.inputs[0].default_value = 1.0
    mx.inputs[6].default_value = H(hex_color)
    m.link(va.outputs['Color'], mx.inputs[7])
    m.link(mx.outputs[2], m.bsdf.inputs['Base Color'])
    m.set('Roughness', rough)
    m.m.use_backface_culling = True
    if m.m.name != name:
        raise RuntimeError(f'material name clash: {m.m.name}')
    return m


def drive_z(X, Y):
    """The drive surface (lanes, table, curves, arms, bay, shoulders, road) at map points."""
    X = np.asarray(X, dtype=np.float64)
    Y = np.asarray(Y, dtype=np.float64)
    ay = np.abs(Y)
    z = np.where((X >= SHOULDER_E - 1e-6), road_z(X, Y), 0.0)
    bay = (X >= BAY_X[0] - 0.3) & (X < SHOULDER_E) & (Y > -26.6) & (Y < 26.0)
    arm = (X >= CURVE_C[0] - 0.01) & (X < SHOULDER_E) & (ay >= 26.0)
    curve = (X < CURVE_C[0]) & (ay > 21.0)
    lane = (X >= LANE_X[0] - 0.3) & (X <= LANE_X[1] + 0.3) & (ay <= 21.0)
    z = np.where(lane, lane_z(X, Y), z)
    z = np.where(curve, -0.12, z)
    z = np.where(arm, arm_z(X), z)
    z = np.where(bay, LEVEL_BAY, z)
    return z


def build_ground():
    G = GroundBuilder()
    t0 = time.time()

    # ------------------------------------------------ curbs, walls, edgings (their lines feed the colors below)
    def curve_arc(sgn, rad, a0, a1, n=36):
        return [curve_pt(sgn, a0 + (a1 - a0) * i / n, rad(a0 + (a1 - a0) * i / n)) for i in range(n + 1)]

    r_in_f = r_in
    r_out_f = r_out
    # The island loop: lanes' east edge, north inner curve, north arm, the bay edge, south arm, south inner curve.
    fil_n, cen_fn = fillet_pts((CURVE_C[0], ARM_Y[0]), (SHOULDER_E, ARM_Y[0]), (SHOULDER_E, BAY_N1[1]), FILLET_IN, 8)
    fil_s, cen_fs = fillet_pts(BAY_S1, BAY_S0, (CURVE_C[0], -ARM_Y[0]), NOSE, 8)
    loop = [(LANE_X[1], -21.0), (LANE_X[1], 21.0)]
    loop += curve_arc(1, r_in_f, 180.0, 90.0)[1:]
    loop += fil_n
    loop += [BAY_N1, BAY_N0, BAY_S1]
    loop += fil_s
    loop += curve_arc(-1, r_in_f, 90.0, 180.0)
    loop = loop[:-1] if abs(loop[-1][0] - loop[0][0]) < 1e-6 and abs(loop[-1][1] - loop[0][1]) < 1e-6 else loop

    def top_island(X, Y):
        ay = abs(Y)
        if X <= LANE_X[1] + 0.3 or (X < CURVE_C[0] + 0.05 and ay > 20.9):
            return 0.003
        arm = float(arm_z(X)) + 0.15
        t = min(1.0, max(0.0, (X - (WALLS[0] - WALL_T)) / WALL_T))
        arm_top = 0.003 + (arm - 0.003) * t
        if X > 52.0 and ay < 26.4:
            w = float(smoothstep(25.4, 26.4, ay)) if Y > 0 else float(smoothstep(-25.9, -26.45, Y))
            return LEVEL_P + (arm_top - LEVEL_P) * w
        return arm_top

    def heated_col(X, Y, Z):
        h = 1.0 if (X < 44.5 and abs(Y) < 34.0) or (X < 55.1 and abs(Y) < 26.6 and X > 46.0) else 0.0
        g = 0.0 if h > 0.5 else 0.5
        return (0.0, g, h)
    G.curb(loop, top_island, drive_z, upper_left=False, colf=heated_col, closed=True)

    # The outer curb: east shoulder (south) -> south arm -> south curve -> lanes' west edge -> north curve -> north arm
    # -> east shoulder (north). The upper side (verge, apron) is on the left.
    fo_s, cen_os = fillet_pts((SHOULDER_E, ROAD_Y[0]), (SHOULDER_E, -ARM_Y[1]), (CURVE_C[0], -ARM_Y[1]), FILLET_OUT, 10)
    fo_n, cen_on = fillet_pts((CURVE_C[0], ARM_Y[1]), (SHOULDER_E, ARM_Y[1]), (SHOULDER_E, ROAD_Y[1]), FILLET_OUT, 10)
    outer = [(SHOULDER_E, ROAD_Y[0])] + fo_s + curve_arc(-1, r_out_f, 90.0, 180.0) + [(LANE_X[0], 0.0)] + curve_arc(1, r_out_f, 180.0, 90.0) + fo_n + \
        [(SHOULDER_E, ROAD_Y[1])]

    def top_outer(X, Y):
        ay = abs(Y)
        if ay <= 21.0 and X < LANE_X[0] + 0.3:
            return 0.003
        if X < CURVE_C[0]:
            return 0.003 + 0.027 * float(smoothstep(21.0, 22.6, ay))
        arm = float(arm_z(X)) + 0.15
        road = float(road_profile(Y)) + 0.15
        if X < 52.0:
            return arm
        t = min(1.0, (X - 52.0) / 3.0) if ay < 37.0 else 1.0
        return arm + (road - arm) * t

    def back_outer(X, Y):
        return top_outer(X, Y) - (0.35 if abs(Y) > OUT_Y else 0.003)

    def outer_col(X, Y, Z):
        h = 1.0 if (X < 44.5 and abs(Y) < 34.0) else 0.0
        return (0.0, 0.2 if h > 0.5 else 0.6, h)
    G.curb(outer, top_outer, drive_z, back_outer, upper_left=True, colf=outer_col)

    # The curves' back lines (where the curbs' bodies end).
    def theta_where_back_x(target):
        lo, hi = 150.0, 180.0
        for _ in range(60):
            mid = (lo + hi) / 2
            if curve_pt(1, mid, r_out(mid) + CW)[0] > target:
                lo = mid
            else:
                hi = mid
        return (lo + hi) / 2
    th_c = theta_where_back_x(LANE_X[0])
    y_c = curve_pt(1, th_c, r_out(th_c) + CW)[1]

    # Sidewalk edging (granite, 4 cm) between the wing sidewalk and the snow outside the curve.
    for sgn in (1, -1):
        p = [(LANE_X[0], y_c), (LANE_X[0], OUT_Y)] if sgn > 0 else [(LANE_X[0], -OUT_Y), (LANE_X[0], -y_c)]
        G.curb(p, lambda X, Y: 0.04, lambda X, Y: 0.0, upper_left=False, width=0.15, chamfer=0.008, colf=lambda X, Y, Z: (0.0, 0.2, 1.0))
        G.snow_segs += path_segs(p)
    # Wing planter strips' wallstone curbs (25 cm).
    for sgn in (1, -1):
        p = [(3.0, 22.0), (4.85, 22.0)] + fillet_pts((3.0, 22.0), (5.0, 22.0), (5.0, OUT_Y), 0.15, 4)[0] + [(5.0, OUT_Y)]
        if sgn < 0:
            p = [(x, -y) for x, y in reversed(p)]
        G.curb(p, lambda X, Y: 0.25, lambda X, Y: 0.0, lambda X, Y: 0.18, upper_left=True, mat=5, width=0.2, chamfer=0.015,
               colf=lambda X, Y, Z: (0.0, 0.2, 1.0))
        G.snow_segs += path_segs(p)
    # Overlook curb.
    G.curb([(PARAPET_X, OVER[1][0]), (PARAPET_X, OVER[1][1])], lambda X, Y: LEVEL_P + 0.003, drive_z, upper_left=False,
           colf=lambda X, Y, Z: (0.0, 0.5, 0.0))
    # Platform ends and landing strips: granite edgings where paving meets the beds.
    (sb0, sb1) = offset_line(BAY_S1, BAY_S0, -CW)   # the south taper's back line (island side)
    x_back_s22 = line_x_at(sb0, sb1, -PLAT_Y)
    for p, lvl in (([(WALLS[2], PLAT_Y), (SHOULDER_E - CW, PLAT_Y)], LEVEL_P), ([(x_back_s22, -PLAT_Y), (WALLS[2], -PLAT_Y)], LEVEL_P),
                   ([(WALLS[0], CAS_Y[1]), (CAS_X[0], CAS_Y[1])], LEVEL_B), ([(CAS_X[0], -CAS_Y[1]), (WALLS[0], -CAS_Y[1])], LEVEL_B),
                   ([(CAS_X[1], CAS_Y[1]), (WALLS[2] - WALL_T, CAS_Y[1])], LEVEL_A), ([(WALLS[2] - WALL_T, -CAS_Y[1]), (CAS_X[1], -CAS_Y[1])], LEVEL_A)):
        G.curb(p, (lambda lv: (lambda X, Y: lv + 0.04))(lvl), (lambda lv: (lambda X, Y: lv))(lvl), upper_left=True, width=0.15, chamfer=0.008,
               colf=lambda X, Y, Z: (0.0, 0.2, 1.0))
        G.snow_segs += path_segs(p)

    # Retaining walls.
    def bed_base(X, ay, lvl):
        w = float(bed_w(ay))
        return lvl + (float(arm_z(X)) + 0.15 - lvl) * w

    def wall_fns(Xw, up_lvl, lo_lvl, strip_y, lo_paved_y, up_paved=False):
        def top(X, Y):
            ay = abs(Y)
            if ay <= strip_y + 1e-6 or up_paved:
                return up_lvl + 0.45
            w = float(bed_w(ay))
            return bed_base(Xw, ay, up_lvl) + 0.45 * (1 - w) + 0.12 * w

        def low(X, Y):
            ay = abs(Y)
            if ay <= lo_paved_y + 1e-6:
                return lo_lvl
            return min(bed_base(Xw, ay, lo_lvl), lo_lvl + 0.0) if ay < strip_y else bed_base(Xw, ay, lo_lvl)

        def back(X, Y):
            return up_lvl if (abs(Y) <= strip_y or up_paved) else bed_base(Xw, abs(Y), up_lvl)
        return top, low, back
    wcol = lambda X, Y, Z: (0.0, 0.15, 0.0)  # noqa: E731
    end_y = ARM_Y[0] - CW
    for sgn in (1, -1):
        def P(a, b):
            return [(WALLS[0], sgn * a), (WALLS[0], sgn * b)] if sgn > 0 else [(WALLS[0], sgn * b), (WALLS[0], sgn * a)]
        t, lo, bk = wall_fns(WALLS[0], 0.0, LEVEL_B, CAS_Y[1], CAS_Y[1], up_paved=True)
        G.wall(P(STEP_Y, end_y), t, lo, bk, upper_left=True, colf=wcol)
        for (a, b) in ((STEP_Y, CAS_Y[0]), (CAS_Y[1], end_y)):
            pts = [(WALLS[1], sgn * a), (WALLS[1], sgn * b)] if sgn > 0 else [(WALLS[1], sgn * b), (WALLS[1], sgn * a)]
            t, lo, bk = wall_fns(WALLS[1], LEVEL_B, LEVEL_A, CAS_Y[0], CAS_Y[0])
            G.wall(pts, t, lo, bk, upper_left=True, colf=wcol)
        pts = [(WALLS[2], sgn * STEP_Y), (WALLS[2], sgn * end_y)] if sgn > 0 else [(WALLS[2], sgn * end_y), (WALLS[2], sgn * STEP_Y)]
        t, lo, bk = wall_fns(WALLS[2], LEVEL_A, LEVEL_P, CAS_Y[1], PLAT_Y)
        G.wall(pts, t, lo, bk, upper_left=True, colf=wcol)
        # The cascades' footprints: the beds and the paving taper to their walls.
        G.edge_segs += path_segs([(CAS_X[0], sgn * CAS_Y[0]), (CAS_X[1], sgn * CAS_Y[0]), (CAS_X[1], sgn * CAS_Y[1]), (CAS_X[0], sgn * CAS_Y[1]),
                                  (CAS_X[0], sgn * CAS_Y[0])])
    print(f'[ecx_plaza] ground: curbs and walls in {time.time() - t0:.1f}s')

    # ------------------------------------------------ colors
    def pud(X, Y, scale=3.2, lo=0.56, hi=0.74, seed=11):
        return smoothstep(lo, hi, vnoise2(X, Y, scale, seed, 3))

    def gutters(X, Y, reach=0.4):
        return smoothstep(reach, 0.05, seg_distance(np.stack([X, Y], 1), G.curb_segs))

    def col_paver(X, Y, Z, heated=1.0):
        P = np.stack([X, Y], 1)
        R = np.clip(0.7 * pud(X, Y) + 0.35 * pud(X, Y, 0.9, 0.62, 0.8, 5), 0, 1)
        Gc = 0.3 * smoothstep(0.4, 0.0, seg_distance(P, G.snow_segs)) if heated else np.full(len(X), 0.35)
        return np.stack([R, Gc, np.full(len(X), heated)], 1)

    def col_overlook(X, Y, Z):
        R = 0.6 * pud(X, Y, 2.5)
        edge = np.minimum.reduce([np.abs(X - OVER[0][1]), np.abs(Y - OVER[1][0]), np.abs(Y - OVER[1][1])])
        Gc = 0.3 + 0.55 * smoothstep(1.2, 0.1, edge)
        return np.stack([R, Gc, np.zeros(len(X))], 1)

    def col_sett(X, Y, Z):
        R = np.clip(0.6 * pud(X, Y, 2.6) + 0.7 * gutters(X, Y), 0, 1)
        return np.stack([R, 0.08 * np.ones(len(X)), np.ones(len(X))], 1)

    def col_asphalt(X, Y, Z):
        P = np.stack([X, Y], 1)
        R = np.clip(0.5 * pud(X, Y, 3.5) + 0.75 * gutters(X, Y, 0.5), 0, 1)
        dc = seg_distance(P, G.curb_segs)
        on_road = (X >= ROAD_X[0]) & (X <= ROAD_X[1])
        edge = np.where(on_road, np.minimum(np.abs(X - ROAD_X[0]), np.abs(X - ROAD_X[1])), dc)
        lanes = np.minimum(np.abs(np.abs(X - 57.75) - 0.85), np.abs(np.abs(X - 61.25) - 0.85))
        tracks = np.where(on_road, smoothstep(0.35, 0.0, lanes), 0.0)
        Gc = np.clip(0.18 + 0.7 * smoothstep(1.5, 0.0, edge) + 0.15 * vnoise2(X, Y, 1.5, 7, 2) - 0.2 * tracks, 0, 1)
        R = np.clip(R + 0.25 * tracks * vnoise2(X, Y, 4.0, 9, 2), 0, 1)
        B = np.where((X < 46.0) & (np.abs(Y) > 26.0), smoothstep(46.0, 43.8, X), 0.0)
        return np.stack([R, Gc, B], 1)

    def col_concrete(X, Y, Z):
        return np.stack([0.4 * pud(X, Y, 2.0), np.full(len(X), 0.8), np.zeros(len(X))], 1)

    def col_snow(X, Y, Z):
        P = np.stack([X, Y], 1)
        dirt = 0.06 + 0.55 * smoothstep(1.3, 0.0, seg_distance(P, G.curb_segs)) * np.where(X > 44.0, 1.0, 0.45)
        return np.stack([np.zeros(len(X)), np.clip(dirt, 0, 1), np.zeros(len(X))], 1)

    # ------------------------------------------------ snow heights
    flat = []   # (X, Y, radius, level or None = keep the base)
    for k in np.arange(-4.5, 4.51, 1.0):
        flat.append((45.5 + 0.259 * k, -20.5 + 0.966 * k, 1.15, LEVEL_A))
    for sgn in (1, -1):
        flat += [(29.5, sgn * 20.0, 1.2, LEVEL_B), (42.5, sgn * 22.5, 1.2, LEVEL_A)]
        flat += [(30.0 + k, sgn * 21.0, 1.4, LEVEL_B) for k in range(-4, 5)]
        flat += [(40.0 + k, sgn * 22.0, 1.4, LEVEL_A) for k in range(-4, 5)]
        flat += [(x + k, sgn * 35.5, 0.9, None) for x in (26.0, 36.0, 46.0) for k in range(-5, 6)]
        flat += [(x, sgn * 34.3, 0.6, None) for x in (24.0, 32.0, 40.0, 48.0)]
        flat += [(50.0, sgn * 24.3, 2.4, None), (22.0, sgn * 37.0, 3.0, None)]
    flat += [(39.5, 18.5, 2.3, LEVEL_A), (44.5, 17.5, 0.9, LEVEL_A), (38.0, 24.0, 0.9, LEVEL_A)]
    FL = np.array([(f[0], f[1], f[2]) for f in flat])
    FLV = [f[3] for f in flat]

    def snow_z(base_fn, amp=1.0, outline_taper=False):
        def zf(X, Y):
            base = base_fn(X, Y)
            P = np.stack([X, Y], 1)
            de = seg_distance(P, G.edge_segs)
            te = smoothstep(0.04, 0.95, de)
            if outline_taper:
                te = te * (0.4 + 0.6 * smoothstep(0.0, 1.2, OUT_Y - np.abs(Y)))
            A = (0.10 + 0.25 * vnoise2(X, Y, 3.2, 21, 3)) * amp + 0.035 * vnoise2(X, Y, 0.45, 23, 2)
            keep = np.ones(len(X))
            lvl = base.copy()
            for (fx, fy, fr), fv in zip(FL, FLV):
                d = np.hypot(X - fx, Y - fy)
                inside = smoothstep(fr + 0.9, fr, d)
                if inside.max() <= 0:
                    continue
                keep = keep * (1.0 - inside)
                if fv is not None:
                    lvl = lvl + (fv - lvl) * inside
            z = lvl + te * A * keep - 0.01 * (1.0 - te)
            return z
        return zf

    def verge_base(X, Y):
        return arm_z(np.maximum(X, CURVE_C[0])) + 0.15

    def bed_base_fn(lvl):
        def f(X, Y):
            w = bed_w(np.abs(Y))
            return lvl + (arm_z(X) + 0.15 - lvl) * w
        return f

    # ------------------------------------------------ regions
    t1 = time.time()
    both = lambda ps: ps + [mirror_y(p) for p in ps]  # noqa: E731
    flatz = lambda v: (lambda X, Y: np.full(len(X), v))  # noqa: E731

    # Pavers (heated).
    back_arc = [curve_pt(1, th, r_out(th) + CW) for th in np.linspace(180.0, th_c, 8)]
    sidewalk = []
    for a, b in zip(back_arc[:-1], back_arc[1:]):
        trap = ccw([(3.0, a[1]), (a[0], a[1]), (b[0], b[1]), (3.0, b[1])])
        for clipr in (rect(3.0, 9.0, 21.0, 22.0), rect(5.0, 9.0, 22.0, y_c + 1e-9)):
            q = clean_poly(clip_convex(trap, ccw(clipr)))
            if len(q) >= 3 and abs(poly_area(q)) > 1e-5:
                sidewalk.append(q)
    sidewalk.append(rect(5.0, LANE_X[0], y_c, OUT_Y))
    apron = [rect(0.0, LANE_X[0] - CW, -18.5, 18.5)] + both([rect(3.0, LANE_X[0] - CW, 18.5, 21.0)] + sidewalk)
    G.region(apron, 0, flatz(0.0), 0.5, col_paver)
    G.region([rect(LANE_X[0], LANE_X[1], -TABLE_Y, TABLE_Y)] + both([rect(DIV_X[0], DIV_X[1], RAMP_Y, 21.0)]), 0, lane_z, 0.5, col_paver)
    disc = [CURVE_C] + [curve_pt(1, th, r_in(th) - CW) for th in np.linspace(180.0, 90.0, 19)]
    top = [rect(LANE_X[1] + CW, TOP_X1, -21.0, 21.0)] + both([rect(TOP_X1, WALLS[0] - WALL_T, STEP_Y, 21.0), disc,
                                                            rect(CURVE_C[0], WALLS[0] - WALL_T, 21.0, ARM_Y[0] - CW)])
    G.region(top, 0, flatz(0.0), 0.5, col_paver)
    landb = [rect(LAND_B[0], LAND_B[1], -CAS_Y[0], CAS_Y[0])] + both([rect(LAND_B[1], WALLS[1] - WALL_T, STEP_Y, CAS_Y[0]), rect(WALLS[0], LAND_B[0], STEP_Y, CAS_Y[1]),
                                                                      rect(LAND_B[0], CAS_X[0], CAS_Y[0], CAS_Y[1])])
    G.region(landb, 0, flatz(LEVEL_B), 0.5, col_paver)
    landa = [rect(LAND_A[0], LAND_A[1], -CAS_Y[0], CAS_Y[0])] + both([rect(WALLS[1], LAND_A[0], STEP_Y, CAS_Y[0]), rect(CAS_X[1], WALLS[2] - WALL_T, STEP_Y, CAS_Y[1])])
    G.region(landa, 0, flatz(LEVEL_A), 0.5, col_paver)
    # The platform: its bay edge's back line, the tapers stepping it out at both ends.
    (nb0, nb1) = offset_line(BAY_N1, BAY_N0, -CW)
    xb = PLAT_X[1] - CW

    def isect(p0, p1, x):
        return (x, p0[1] + (x - p0[0]) * (p1[1] - p0[1]) / (p1[0] - p0[0]))
    yN = isect(nb0, nb1, xb)[1]
    yS = isect(sb0, sb1, xb)[1]
    y_top = isect(nb0, nb1, SHOULDER_E - CW)[1]
    plat = [rect(PLAT_X[0], xb, yS, yN),
            ccw([(PLAT_X[0], yN), (xb, yN), (SHOULDER_E - CW, y_top), (PLAT_X[0], y_top)]),
            rect(PLAT_X[0], SHOULDER_E - CW, y_top, PLAT_Y),
            ccw([(PLAT_X[0], -PLAT_Y), (x_back_s22, -PLAT_Y), (xb, yS), (PLAT_X[0], yS)])]
    plat += both([rect(WALLS[2], PLAT_X[0], STEP_Y, PLAT_Y)])
    G.region(plat, 0, flatz(LEVEL_P), 0.5, col_paver)
    G.region([rect(PARAPET_X + CW, OVER[0][1], OVER[1][0], OVER[1][1])], 0, flatz(LEVEL_P), 0.5, col_overlook)

    # Setts (heated): lanes and ramps, the curves, the arms to X 44.
    lanes = both([rect(LANE_X[0], DIV_X[0], RAMP_Y, 21.0), rect(DIV_X[1], LANE_X[1], RAMP_Y, 21.0), rect(LANE_X[0], LANE_X[1], TABLE_Y, RAMP_Y)])
    G.region(lanes, 1, lane_z, 0.5, col_sett)
    curves = []
    ths = np.linspace(180.0, 90.0, 25)
    for a, b in zip(ths[:-1], ths[1:]):
        curves.append(ccw([curve_pt(1, a, r_in(a)), curve_pt(1, a, r_out(a)), curve_pt(1, b, r_out(b)), curve_pt(1, b, r_in(b))]))
    G.region(both(curves), 1, lambda X, Y: np.full(len(X), -0.12), 0.5, col_sett)
    G.region(both([rect(CURVE_C[0], 44.0, ARM_Y[0], ARM_Y[1])]), 1, lambda X, Y: arm_z(X), 0.5, col_sett)

    # Asphalt: the arms beyond X 44, the corner fills, the bay, the road.
    def fan(apex, arc):
        return [ccw([apex, a, b]) for a, b in zip(arc[:-1], arc[1:])]
    asph = both([rect(44.0, BAY_X[1], ARM_Y[0], ARM_Y[1])] + fan((SHOULDER_E, ARM_Y[1]), fo_n))
    asph += fan((SHOULDER_E, ARM_Y[0]), fil_n) + fan(BAY_S0, fil_s)
    asph += [rect(BAY_X[0], BAY_X[1], BAY_S1[1], BAY_N0[1]), ccw([BAY_N0, (BAY_X[1], BAY_N0[1]), (BAY_X[1], BAY_N1[1]), BAY_N1]),
             ccw([BAY_S0, (BAY_X[1], BAY_S0[1]), (BAY_X[1], BAY_S1[1]), BAY_S1])]
    G.region(asph, 2, drive_z, 0.5, col_asphalt)
    G.region([rect(ROAD_X[0], ROAD_X[1], -60.0, 70.0)], 2, road_z, (0.875, 1.0), col_asphalt)
    G.region([rect(ROAD_X[0], ROAD_X[1], ROAD_Y[0], -60.0), rect(ROAD_X[0], ROAD_X[1], 70.0, ROAD_Y[1])], 2, road_z, (0.875, 2.0), col_asphalt)

    # Concrete shoulders.
    rp = lambda X, Y: road_profile(Y)  # noqa: E731
    shoulders = [rect(SHOULDER_E, ROAD_X[0], ROAD_Y[0], -ARM_Y[1]), rect(SHOULDER_E, ROAD_X[0], ARM_Y[1], ROAD_Y[1]),
                 rect(SHOULDER_E, ROAD_X[0], BAY_N1[1], ARM_Y[0]),
                 rect(ROAD_X[1], OUT_X, ROAD_Y[0], OVER[1][0]), rect(ROAD_X[1], PARAPET_X, OVER[1][0], OVER[1][1]), rect(ROAD_X[1], OUT_X, OVER[1][1], ROAD_Y[1])]
    G.region(shoulders, 4, rp, (0.5, 1.0), col_concrete)
    print(f'[ecx_plaza] ground: hard regions in {time.time() - t1:.1f}s')

    # Snow: wing planter strips, the verges outside the curves and along the arms, the island beds.
    G.region(both([rect(3.0, 4.8, 22.2, OUT_Y)]), 6, snow_z(lambda X, Y: np.full(len(X), 0.2), 0.45, True), 0.3, col_snow)
    verge = []
    ths = np.linspace(90.0, 180.0, 31)
    box_r = (LANE_X[0] + 0.15, CURVE_C[0], CURVE_C[1], OUT_Y)

    def exit_pt(th):
        a = math.radians(th)
        dx, dy = math.cos(a), math.sin(a)
        ts = []
        if dy > 1e-9:
            ts.append((box_r[3] - CURVE_C[1]) / dy)
        if dx < -1e-9:
            ts.append((box_r[0] - CURVE_C[0]) / dx)
        t = min(ts)
        return (CURVE_C[0] + dx * t, CURVE_C[1] + dy * t)
    corner_th = math.degrees(math.atan2(box_r[3] - CURVE_C[1], box_r[0] - CURVE_C[0]))
    for a, b in zip(ths[:-1], ths[1:]):
        pts = [curve_pt(1, a, r_out(a) + CW), exit_pt(a)]
        if a < corner_th < b:
            pts.append((box_r[0], box_r[3]))
        pts += [exit_pt(b), curve_pt(1, b, r_out(b) + CW)]
        q = clean_poly(clip_convex(ccw(pts), ccw(rect(box_r[0], CURVE_C[0] + 1e-9, 0.0, OUT_Y))))
        if len(q) >= 3 and abs(poly_area(q)) > 1e-5:
            verge.append(q)
    sector = [(52.0, ARM_Y[1] + FILLET_OUT)] + arc_pts((52.0, ARM_Y[1] + FILLET_OUT), FILLET_OUT - CW, 270.0, 360.0, 10)
    verge += [rect(CURVE_C[0], 52.0, ARM_Y[1] + CW, OUT_Y), ccw(sector), rect(52.0, SHOULDER_E - CW, ARM_Y[1] + FILLET_OUT, OUT_Y)]
    G.region(both(verge), 6, snow_z(verge_base, 1.0, True), 0.3, col_snow)
    G.region(both([rect(WALLS[0], WALLS[1] - WALL_T, CAS_Y[1], end_y)]), 6, snow_z(bed_base_fn(LEVEL_B)), 0.3, col_snow)
    G.region(both([rect(WALLS[1], WALLS[2] - WALL_T, CAS_Y[1], end_y)]), 6, snow_z(bed_base_fn(LEVEL_A)), 0.3, col_snow)
    # Platform-level snow beyond the platform's ends, up to the arms (the north one rounds the island's corner).
    n_end = [rect(WALLS[2], cen_fn[0], PLAT_Y + 0.15, end_y), rect(cen_fn[0], SHOULDER_E - CW, PLAT_Y + 0.15, cen_fn[1]),
             ccw([cen_fn] + arc_pts(cen_fn, FILLET_IN - CW, 0.0, 90.0, 6))]
    # the south one: round the nose (the fillet's back arc) up the taper's back line
    a0 = math.degrees(math.atan2(-ARM_Y[0] - cen_fs[1], 0.0))
    dvec = np.array(sb1) - np.array(sb0)
    nrm = np.array((-dvec[1], dvec[0]))
    nrm /= np.linalg.norm(nrm)
    a1 = math.degrees(math.atan2(nrm[1], nrm[0]))

    nose_back = arc_pts(cen_fs, NOSE - CW, -90.0, a1, 6)
    x22 = line_x_at(sb0, sb1, -PLAT_Y - 0.15)
    s_end = [ccw([(WALLS[2], -end_y), (cen_fs[0], -end_y)] + nose_back + [(x22, -PLAT_Y - 0.15), (WALLS[2], -PLAT_Y - 0.15)])]
    G.region(n_end + s_end, 6, snow_z(bed_base_fn(LEVEL_P), 0.8), 0.3, col_snow)

    # Concrete pads under the verge lamp posts.
    for sgn in (1, -1):
        for x in (24.0, 32.0, 40.0, 48.0):
            z = float(arm_z(x)) + 0.16
            G.solid_box(x - 0.35, x + 0.35, sgn * 34.3 - 0.35, sgn * 34.3 + 0.35, z - 0.25, z, 4, (0.1, 0.4, 0.0))
    print(f'[ecx_plaza] ground: snow in {time.time() - t1:.1f}s')

    # ------------------------------------------------ markings
    rz = lambda X, Y: road_z(X, Y)  # noqa: E731
    zebra = (11.5, 15.5)
    for y0, y1 in ((ROAD_Y[0], zebra[0]), (zebra[1], ROAD_Y[1])):
        for x0 in (59.35, 59.55):
            G.strip(x0, x0 + 0.1, y0, y1, PAINT_YELLOW, rz)
        G.strip(62.75, 62.9, y0, y1, PAINT_WHITE, rz)
    for y0, y1 in ((ROAD_Y[0], -ARM_Y[1]), (BAY_N1[1], ARM_Y[0]), (ARM_Y[1], ROAD_Y[1])):
        G.strip(56.1, 56.25, y0, y1, PAINT_WHITE, rz)
    y = -26.0
    while y < BAY_N1[1] - 0.5:
        if not (zebra[0] - 1.5 < y < zebra[1]):
            G.strip(55.95, 56.1, y, min(y + 1.5, BAY_N1[1]), PAINT_WHITE, drive_z)
        y += 3.0
    for x in np.arange(54.5, 62.6, 1.0):
        for xa, xb2 in ((x, min(x + 0.5, 63.0)),):
            for k in range(3):
                G.marking(rect(xa, xb2, 12.0 + k, 13.0 + k), PAINT_WHITE, drive_z)
    G.strip(54.2, 54.6, 26.8, 33.2, PAINT_WHITE, lambda X, Y: arm_z(X))
    G.strip(xb - 0.6, xb, BAY_S1[1] + 0.2, BAY_N0[1] - 0.2, PAINT_YELLOW, flatz(LEVEL_P))
    # BUS, reading for a driver heading north (Blender's top view: text +x right, +y up; the map frame mirrors X).
    t = text_mesh('BUS', 1.0, 'Bold', 'CENTER', 1.15)
    bm = bmesh.new()
    bm.from_mesh(t.data)
    bmesh.ops.triangulate(bm, faces=bm.faces[:])
    x0, x1, y0b, y1b = text_bounds(t)
    sx = 2.2 / (x1 - x0)
    sy = 1.7 / (y1b - y0b)
    cxm, cym = 54.3, -13.0
    for f in bm.faces:
        poly = [(cxm - (v.co.x - (x0 + x1) / 2) * sx, cym + (v.co.y - (y0b + y1b) / 2) * sy) for v in f.verts]
        G.marking(poly, PAINT_WHITE, flatz(LEVEL_BAY))
    bm.free()
    bpy.data.objects.remove(t)

    # ------------------------------------------------ mesh
    t2 = time.time()
    obj = G.build('SM_ECX_Ground')
    _orient_solids(obj)
    tests = [lambda X, Y: abs(abs(Y) - OUT_Y) < 0.004 and 2.99 < X < SHOULDER_E + 0.01,
             lambda X, Y: X < 3.004 and 18.4 < abs(Y) <= OUT_Y + 0.01,
             lambda X, Y: X < 0.004 and abs(Y) <= 18.51,
             lambda X, Y: abs(Y - ROAD_Y[0]) < 0.004 or abs(Y - ROAD_Y[1]) < 0.004]
    n = _skirts(obj, tests, lambda X, Y: 1.0 if (abs(Y - ROAD_Y[0]) < 0.01 or abs(Y - ROAD_Y[1]) < 0.01) else 0.6)
    mats = [ground_material('ecx_paver', 0x8c8a86, 0.7), ground_material('ecx_sett', 0x2e3034, 0.75), ground_material('ecx_asphalt', 0x2b2b2c, 0.85),
            ground_material('ecx_curb', 0x8e8c88, 0.75), ground_material('ecx_concrete', 0x9a968e, 0.85), ground_material('ecx_wallstone', 0x2a2c30, 0.7),
            ground_material('ecx_snowground', 0xe8edf5, 0.62), ground_material('ecx_marking', 0xffffff, 0.6)]
    for mt in mats:
        obj.data.materials.append(mt.m)
    obj.data.shade_smooth()
    obj.data.set_sharp_from_angle(angle=math.radians(32.0))
    print(f'[ecx_plaza] ground: mesh in {time.time() - t2:.1f}s, {len(obj.data.polygons)} faces, {tri_count(obj)} tris, {n} skirt quads')
    return obj


# ================================================================== ground texture sets (unreal/Art/Textures/ECX)

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


def lin(hexc):
    return np.array(H(hexc)[:3], dtype=np.float32)


def tnoise(n, cells, seed, octaves=4, gain=0.5, cells_y=None):
    """Tileable fractal value noise on an n x n tile (cells_y: a different lattice count down the rows)."""
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
    gx = (np.roll(h, -1, 1) - np.roll(h, 1, 1)) / (2 * texel) * strength
    gr = (np.roll(h, -1, 0) - np.roll(h, 1, 0)) / (2 * texel) * strength
    nrm = np.stack([-gx, gr, np.ones_like(h)], 2)
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


def _write_set(name, color_lin, rough, h, texel, ao_gain=180.0, height=None, nstrength=1.0):
    base = os.path.join(TEX_DIR, f'T_ECX_{name}')
    _png(base + '_BaseColor.png', lin2srgb(color_lin))
    _png(base + '_Normal.png', normal_from_height(h, texel, nstrength))
    ao = cavity_ao(h, gain=ao_gain)
    _png(base + '_ORM.png', np.stack([ao, np.clip(rough, 0.02, 1.0), np.zeros_like(rough)], 2))
    if height is not None:
        lo, hi = height
        _png(base + '_Height.png', (h - lo) / (hi - lo))
    print(f'[ecx_plaza] wrote T_ECX_{name}_* ({color_lin.shape[1]}^2)')


def _grid_m(n, L):
    c = (np.arange(n, dtype=np.float64) + 0.5) * L / n
    return np.meshgrid(c, c)   # U (columns), V (rows)


def _granite_surface(n, seed, base_hex=0x8c8a86, grain_scale=1):
    rng_tone = tnoise(n, 24, seed + 1, 4)
    gr1 = grains(n, seed + 2, grain_scale)
    gr2 = grains(n, seed + 3, grain_scale)
    col = np.broadcast_to(lin(base_hex), (n, n, 3)) * (0.94 + 0.12 * rng_tone[..., None])
    dark = np.clip((gr1 - 0.70) * 9.0, 0, 1)[..., None]
    white = np.clip((gr2 - 0.71) * 9.0, 0, 1)[..., None]
    col = col * (1 - dark) + lin(0x2a2a2c) * dark
    col = col * (1 - white * 0.8) + lin(0xd8d6d1) * white * 0.8
    micro = grains(n, seed + 4, 1)
    return col, micro, dark[..., 0]


def tex_paver(n=2048):
    L = 3.6
    U, V = _grid_m(n, L)
    row = np.floor(V / 0.30).astype(int)
    off = (row % 2) * 0.30
    uu = (U + off) % L
    col_i = np.floor(uu / 0.60).astype(int)
    du = uu % 0.60
    dv = V % 0.30
    edge = np.minimum.reduce([du, 0.60 - du, dv, 0.30 - dv])
    rng = np.random.default_rng(31)
    tone = rng.uniform(0.94, 1.06, (12, 6))
    warm = rng.uniform(-0.02, 0.02, (12, 6))
    tx = rng.uniform(-0.0012, 0.0012, (12, 6)) / 0.3
    ty = rng.uniform(-0.0012, 0.0012, (12, 6)) / 0.15
    t = tone[row, col_i][..., None]
    wv = warm[row, col_i][..., None]
    col, micro, dark = _granite_surface(n, 7)
    col = col * t * (1 + np.concatenate([wv, wv * 0.3, -wv], 2))
    band = (row == 11)[..., None]
    ves = np.clip((grains(n, 41, 1) - 0.80) * 8, 0, 1)
    basalt = np.broadcast_to(lin(0x2b2d31), (n, n, 3)) * (0.9 + 0.2 * tnoise(n, 30, 43, 3))[..., None] * (1 - 0.6 * ves[..., None])
    col = np.where(band, basalt, col)
    joint = smoothstep(0.0030, 0.0018, edge)[..., None]
    jcol = lin(0x3b3835) * (0.7 + 0.5 * grains(n, 51, 1))[..., None]
    col = col * (1 - joint) + jcol * joint
    h = (micro - 0.5) * 0.0005 - 0.0006 * dark
    h += tx[row, col_i] * (du - 0.3) + ty[row, col_i] * (dv - 0.15)
    h -= 0.0022 * smoothstep(0.007, 0.0025, edge)
    h = h * (1 - joint[..., 0]) + (-0.006 + 0.001 * grains(n, 52, 1)) * joint[..., 0]
    rough = np.where(band[..., 0], 0.58 + 0.06 * micro, 0.76 + 0.08 * (micro - 0.5))
    rough = rough * (1 - joint[..., 0]) + 0.9 * joint[..., 0]
    st = smoothstep(0.78, 0.86, tnoise(n, 9, 61, 4))[..., None]
    col = col * (1 - 0.1 * st)
    _write_set('Paver', col, rough, h, L / n, height=(-0.008, 0.002))


def _herringbone(x, y):
    """Brick-local coordinates in the 1 x 2 herringbone (units of the short side): (lx, ly, W, Hh, anchor_x,
    anchor_y, is_horizontal). Horizontal bricks H(n) = [n, n+2] x [n, n+1], vertical V(n) = [n, n+1] x [n+1, n+3],
    the staircase repeated every (2, -2)."""
    a = np.floor(x)
    b = np.floor(y)
    d = b - a
    k = -np.floor((d + 1) / 4)
    xa = x - 2 * k
    ya = y + 2 * k
    a2 = a - 2 * k
    b2 = b + 2 * k
    d0 = b2 - a2
    isH = (d0 <= 0)
    n_ = np.where(isH, np.where(d0 == 0, a2, b2), a2)
    lx = xa - n_
    ly = np.where(isH, ya - n_, ya - (n_ + 1))
    W = np.where(isH, 2.0, 1.0)
    Hh = np.where(isH, 1.0, 2.0)
    ax = n_ + 2 * k
    ay = np.where(isH, n_ - 2 * k, n_ + 1 - 2 * k)
    return lx, ly, W, Hh, ax, ay, isH


def tex_sett(n=2048):
    L = 2.0
    U, V = _grid_m(n, L)
    x, y = U / 0.1, V / 0.1
    lx, ly, W, Hh, ax, ay, isH = _herringbone(x, y)
    e = np.minimum.reduce([lx, W - lx, ly, Hh - ly]) * 0.1
    period = 20
    gx = np.mod(ax, period)
    gy = np.mod(ay, period) + 40 * isH
    t = (0.84 + 0.32 * _hash2(gx, gy, 701))[..., None]
    kd = _hash2(gx, gy, 703)[..., None]
    tx_ = (_hash2(gx, gy, 705) - 0.5) * 0.002
    ty_ = (_hash2(gx, gy, 707) - 0.5) * 0.002
    base = np.where(kd < 0.12, lin(0x35302b), np.where(kd < 0.22, lin(0x3a3c40), lin(0x2b2d31)))
    surf = tnoise(n, 160, 81, 3)
    col = base * t * (0.88 + 0.24 * surf[..., None])
    dome = 1.0 - (1.0 - np.clip(e / 0.02, 0, 1)) ** 2
    wear = np.clip((dome - 0.75) * 4, 0, 1)
    col = col * (1 + 0.12 * wear[..., None])
    ves = np.clip((grains(n, 83, 1) - 0.82) * 8, 0, 1)
    col = col * (1 - 0.5 * ves[..., None])
    joint = smoothstep(0.0034, 0.002, e)
    sand = grains(n, 85, 1)
    jcol = lin(0x1d1c1a) * (0.6 + 0.9 * sand)[..., None]
    col = col * (1 - joint[..., None]) + jcol * joint[..., None]
    h = 0.006 * dome - 0.006 + (surf - 0.5) * 0.0012 + tx_ * (lx - W / 2) * 2.0 + ty_ * (ly - Hh / 2) * 2.0
    h = h * (1 - joint) + (-0.011 + 0.002 * sand) * joint
    rough = (0.58 - 0.12 * wear + 0.05 * (surf - 0.5)) * (1 - joint) + 0.92 * joint
    _write_set('Sett', col, rough, h, L / n, height=(-0.014, 0.002))


def _stamp_paths(n, L, paths, width_m):
    """A mask of polylines (meters, wrapped on the tile) of the given width."""
    mask = np.zeros((n, n), np.float32)
    px = n / L
    r = max(1, int(round(width_m * px / 2)))
    yy, xx = np.mgrid[-r - 1:r + 2, -r - 1:r + 2]
    disc = np.clip(r + 0.5 - np.hypot(xx, yy), 0.0, 1.0).astype(np.float32)
    for path in paths:
        for (x0, y0), (x1, y1) in zip(path[:-1], path[1:]):
            steps = int(max(abs(x1 - x0), abs(y1 - y0)) * px) + 1
            for i in range(steps + 1):
                cx = int(round((x0 + (x1 - x0) * i / steps) * px)) % n
                cyy = int(round((y0 + (y1 - y0) * i / steps) * px)) % n
                rows = (np.arange(cyy - r - 1, cyy + r + 2)) % n
                cols = (np.arange(cx - r - 1, cx + r + 2)) % n
                mask[np.ix_(rows, cols)] = np.maximum(mask[np.ix_(rows, cols)], disc)
    return mask


def _crack_paths(L, count, seed, curl=0.35, steps=(30, 90), step=(0.02, 0.05), angles=None, hold=0.0):
    """Random-walk crack lines (meters on the tile). angles: headings to start from (else random); hold pulls the
    heading back toward its start each step (0..1), so a crack wanders but keeps its course."""
    rng = np.random.default_rng(seed)
    paths = []
    for i in range(count):
        x, y = rng.uniform(0, L, 2)
        a0 = angles[i % len(angles)] + rng.normal(0, 0.12) if angles else rng.uniform(0, 2 * math.pi)
        ang = a0
        pts = [(x, y)]
        for _ in range(int(rng.uniform(*steps))):
            ang += rng.normal(0, curl) + (a0 - ang) * hold
            s = rng.uniform(*step)
            x += math.cos(ang) * s
            y += math.sin(ang) * s
            pts.append((x, y))
        paths.append(pts)
    return paths


def tex_asphalt(n=2048):
    L = 4.0
    tone = tnoise(n, 6, 101, 4)
    stones = grains(n, 103, 1)
    stones2 = tnoise(n, 900, 105, 1)
    big = np.clip((stones2 - 0.68) * 7, 0, 1)
    small = np.clip((stones - 0.74) * 8, 0, 1)
    col = np.broadcast_to(lin(0x232324), (n, n, 3)) * (0.9 + 0.2 * tone[..., None])
    stone_col = lin(0x77736d) * (0.7 + 0.6 * grains(n, 107, 0))[..., None]
    sfac = np.clip(big * 0.75 + small * 0.6, 0, 1)[..., None]
    col = col * (1 - sfac) + stone_col * sfac
    # oxidation and salt bloom from two lattice sizes blended (one lattice thresholded drew axis-aligned streaks)
    ox = smoothstep(0.54, 0.72, 0.5 * tnoise(n, 7, 109, 4) + 0.5 * tnoise(n, 5, 110, 4))[..., None]
    col = col * (1 + 0.08 * ox)
    salt = smoothstep(0.56, 0.74, 0.6 * tnoise(n, 14, 111, 4) + 0.4 * tnoise(n, 9, 112, 4))[..., None] * 0.06
    col = col * (1 - salt) + lin(0x8f8e8a) * salt
    # Sealed cracks: glossy tar bands that keep their course (two along the road, one across), low contrast so the
    # 4 m repeat doesn't announce itself; and faint hairlines nobody sealed. (Curly black worms read as a pattern.)
    sealed = _crack_paths(L, 3, 113, curl=0.06, steps=(40, 95), step=(0.025, 0.04), angles=[math.pi / 2, -math.pi / 2, 0.0], hold=0.08)
    hair = _crack_paths(L, 5, 117, curl=0.3, steps=(15, 40), step=(0.012, 0.025))
    cracks = _stamp_paths(n, L, sealed, 0.03) * 0.55
    seal = _stamp_paths(n, L, sealed, 0.008)
    hl = _stamp_paths(n, L, hair, 0.005) * 0.45
    col = col * (1 - cracks[..., None]) + lin(0x111112) * cracks[..., None]
    col = col * (1 - hl[..., None]) + lin(0x0d0d0e) * hl[..., None]
    h = (big * 0.0016 + small * 0.0008) + (tone - 0.5) * 0.002 + cracks * 0.0006 - seal * 0.0008 - hl * 0.0015
    rough = 0.84 - 0.12 * big + 0.04 * (tone - 0.5)
    rough = rough * (1 - cracks / 0.55) + 0.4 * cracks / 0.55
    _write_set('Asphalt', col, rough, h, L / n, ao_gain=120.0)


def tex_concrete(n=1024):
    L = 2.0
    U, V = _grid_m(n, L)
    tone = tnoise(n, 5, 121, 4)
    broom = 0.6 * tnoise(n, 5, 123, 3, cells_y=200) + 0.4 * tnoise(n, 40, 124, 3)
    grit = grains(n, 125, 0)
    col = np.broadcast_to(lin(0x908c85), (n, n, 3)) * (0.9 + 0.2 * tone[..., None]) * (0.95 + 0.1 * broom[..., None])
    pits = np.clip((grit - 0.86) * 8, 0, 1)
    col = col * (1 - 0.5 * pits[..., None])
    jd = np.minimum(U, L - U)
    joint = smoothstep(0.004, 0.0015, jd)
    col = col * (1 - joint[..., None] * 0.7)
    h = (broom - 0.5) * 0.0008 - pits * 0.0006 - joint * 0.004
    rough = 0.86 + 0.06 * (tone - 0.5)
    _write_set('Concrete', col, rough, h, L / n)


def tex_curb(n=1024):
    L = 2.0
    U, V = _grid_m(n, L)
    col, micro, dark = _granite_surface(n, 131, 0x8e8c88, 1)
    stone = (U > 1.0).astype(np.float32)
    col = col * (0.97 + 0.06 * stone)[..., None]
    jd = np.minimum.reduce([U, np.abs(U - 1.0), L - U])
    joint = smoothstep(0.004, 0.0015, jd)
    col = col * (1 - joint[..., None]) + lin(0x2c2a28) * joint[..., None]
    h = (micro - 0.5) * 0.0006 - 0.0005 * dark - 0.005 * joint
    rough = 0.76 + 0.08 * (micro - 0.5)
    _write_set('Curb', col, rough, h, L / n)


def tex_wallstone(n=2048):
    L = 3.0
    U, V = _grid_m(n, L)
    courses = [0.30, 0.40, 0.25, 0.35, 0.30, 0.40, 0.25, 0.35, 0.40]
    edges_v = np.cumsum([0.0] + courses)
    ci = np.clip(np.searchsorted(edges_v, V, side='right') - 1, 0, len(courses) - 1)
    rng = np.random.default_rng(141)
    stone_id = np.zeros_like(U, dtype=np.int64)
    eu = np.zeros_like(U)
    for c in range(len(courses)):
        lens = []
        while sum(lens) < L - 1.2:
            lens.append(rng.uniform(0.6, 1.2))
        rest = L - sum(lens)
        if rest > 1.2:
            lens += [rest / 2, rest / 2]
        else:
            lens.append(rest)
        start = rng.uniform(0, L)
        bounds = np.cumsum([0.0] + lens)
        sel = ci == c
        uu = (U[sel] - start) % L
        k = np.clip(np.searchsorted(bounds, uu, side='right') - 1, 0, len(lens) - 1)
        stone_id[sel] = c * 50 + k
        eu[sel] = np.minimum(uu - bounds[k], bounds[k + 1] - uu)
    ev = np.minimum(V - edges_v[ci], edges_v[ci + 1] - V)
    e = np.minimum(eu, ev)
    tone = rng.uniform(0.85, 1.15, 1000)
    warm = rng.uniform(-0.03, 0.03, 1000)
    t = tone[stone_id][..., None]
    wv = warm[stone_id][..., None]
    split = tnoise(n, 50, 143, 5)
    split2 = tnoise(n, 160, 145, 3)
    col = np.broadcast_to(lin(0x2c2e33), (n, n, 3)) * t * (0.82 + 0.36 * split[..., None]) * (1 + np.concatenate([wv, wv * 0.4, -wv], 2))
    ves = np.clip((grains(n, 147, 1) - 0.8) * 8, 0, 1)
    col = col * (1 - 0.55 * ves[..., None])
    joint = smoothstep(0.0062, 0.0042, e)
    col = col * (1 - joint[..., None]) + lin(0x34322f) * (0.7 + 0.5 * grains(n, 149, 1))[..., None] * joint[..., None]
    pillow = -0.006 * (1 - smoothstep(0.0, 0.035, e))
    h = pillow + (split - 0.5) * 0.004 + (split2 - 0.5) * 0.0015 - 0.0005 * ves
    h = h * (1 - joint) + (-0.016 + 0.002 * grains(n, 151, 1)) * joint
    rough = 0.68 + 0.12 * (split - 0.5)
    rough = rough * (1 - joint) + 0.88 * joint
    _write_set('WallStone', col, rough, h, L / n, ao_gain=90.0)


def make_textures():
    t0 = time.time()
    for f in (tex_paver, tex_sett, tex_asphalt, tex_concrete, tex_curb, tex_wallstone):
        f()
    print(f'[ecx_plaza] ground textures in {time.time() - t0:.0f}s')


# ================================================================== SM_ECX_Steps (+ _GlowWarm)

def steps_grooves(m, sep):
    """Three anti-slip grooves behind every nosing (the treads' nosings sit at x = 0.4 k - 0.025)."""
    xr = m.math('FLOORED_MODULO', sep.outputs['X'], 0.4)
    g = None
    for c in (0.032, 0.050, 0.068):
        p = smooth_less(m, m.math('ABSOLUTE', m.math('SUBTRACT', xr, c)), 0.0028, 0.0006)
        g = p if g is None else m.math('MAXIMUM', g, p)
    n = geo_normal(m)
    return m.math('MULTIPLY', g, ramp(m, n.outputs['Z'], 0.5, 0.9))


def build_steps():
    gran = granite('ecxs_granite', wet=True, ground=0.0, salt=0.5, wear=1.0, grooves=steps_grooves)
    brz = bronze('ecxs_bronze', scuff=1.0, patina=0.35, edge=0.002)
    bas = basalt('ecxs_basalt', ground=0.0, salt=0.6, streaks=0.5, wet_foot=0.6)
    out, lit = [], []
    slabs = [(-3.6 + 1.2 * j, -3.6 + 1.2 * (j + 1)) for j in range(6)]
    for i in range(5):
        x0, z0 = 0.4 * i, 0.15 * i
        zt0 = z0 + 0.10
        top = 0.15 * (i + 1) if i < 4 else 0.753
        back = 0.4 * (i + 1) + 0.03 if i < 4 else 1.9
        for (y0, y1) in slabs:
            out.append(box(x0, x0 + 0.03, y0 + 0.0015, y1 - 0.0015, z0, zt0 - 0.0005, gran, 0.002, 1))
            out.append(box(x0 - 0.025, back, y0 + 0.0015, y1 - 0.0015, zt0, top, gran, 0.003, 2))
        out.append(box(x0 - 0.028, x0 - 0.0248, -3.6, 3.6, zt0 + 0.004, top + 0.0028, brz, 0.0008, 1))
        out.append(box(x0 - 0.028, x0 + 0.02, -3.6, 3.6, top, top + 0.003, brz, 0.0008, 1))
        for k in range(24):
            s = parts.disc('screw', (x0 - 0.004, -3.45 + k * 0.3, top + 0.0038), (0, 0, 1), 0.0035, 0.0018, 8)
            core.assign(s, brz)
            out.append(s)
        lit.append(box(x0 - 0.02, x0 - 0.001, -3.58, 3.58, zt0 - 0.009, zt0 - 0.001, None))
    prof = [(-0.02, 0.0), (1.9, 0.0), (1.9, 0.86), (1.6, 0.86), (0.0, 0.26), (-0.02, 0.26)]
    for y0, y1 in ((3.6, 4.0), (-4.0, -3.6)):
        c = prism_y('cheek', prof, y0, y1, bas)
        bm = bmesh.new()
        bm.from_mesh(c.data)
        bmesh.ops.bevel(bm, geom=bm.edges[:], offset=0.008, segments=2, profile=0.5, affect='EDGES', clamp_overlap=True)
        bm.to_mesh(c.data)
        bm.free()
        out.append(c)
    return [finish_mesh(out, 'SM_ECX_Steps', [gran, brz, bas], 30.0), glow(lit, 'SM_ECX_Steps_GlowWarm')]


# ================================================================== SM_ECX_Cascade (+ _Water)

def build_cascade():
    gnd = lambda m, sep: m.math('ADD', LEVEL_A, m.math('MULTIPLY', m.math('GREATER_THAN', sep.outputs['X'], -35.6), 0.75))  # noqa: E731
    bas = basalt('ecxc_basalt', joints=(1.2, 0.6), ground=gnd, salt=0.4, streaks=0.5)
    blk = granite_black('ecxc_granite', lines=(-0.37, -1.12))
    brz = bronze('ecxc_bronze', scuff=0.5, patina=0.7, seams=1.2)
    sn = snow('ecxc_snow')
    out = []

    def coping(X0, X1, Y0, Y1, ztop, along='X'):
        L = (X1 - X0) if along == 'X' else (Y1 - Y0)
        n = max(1, int(round(L / 1.2)))
        for k in range(n):
            a = (X0 if along == 'X' else Y0) + L * k / n + 0.002
            b = (X0 if along == 'X' else Y0) + L * (k + 1) / n - 0.002
            if along == 'X':
                out.append(mbox(a, b, Y0, Y1, ztop - 0.10, ztop, bas, 0.008, 4))
            else:
                out.append(mbox(X0, X1, a, b, ztop - 0.10, ztop, bas, 0.008, 4))
    TB, TA = LEVEL_B + 0.45, LEVEL_A + 0.45
    # Walls under the copings (down below the ground they stand on).
    out += [mbox(25.6, 35.6, 4.6, 5.0, -1.6, TB - 0.10, bas), mbox(25.6, 35.6, 14.5, 14.9, -1.9, TB - 0.10, bas),
            mbox(35.6, 46.0, 4.6, 5.0, -1.6, TA - 0.10, bas), mbox(35.6, 46.0, 14.5, 14.9, -1.9, TA - 0.10, bas),
            mbox(46.0, 46.4, 4.6, 14.9, -1.6, TA - 0.10, bas)]
    coping(25.6, 35.6, 4.58, 5.02, TB)
    coping(25.6, 35.6, 14.48, 14.92, TB)
    coping(35.602, 45.998, 4.58, 5.02, TA)
    coping(35.602, 45.998, 14.48, 14.92, TA)
    coping(46.0, 46.42, 4.58, 14.92, TA, along='Y')
    # The weir: a black granite divider under a bronze lip the upper basin spills over.
    out.append(mbox(35.25, 35.6, 5.0, 14.5, -1.6, -0.393, blk))
    out.append(mbox(35.22, 35.635, 4.99, 14.51, -0.393, -0.376, brz, 0.005, 3))
    # The spout wall: bronze-clad, its lip at +0.10 the source sheet falls from.
    out.append(mbox(25.2, 25.6, 4.6, 14.9, -1.6, 0.45, brz, 0.006, 3))
    out.append(mbox(25.18, 25.622, 4.58, 14.92, 0.43, 0.455, brz, 0.004, 2))
    out.append(mbox(25.6, 25.69, 5.0, 14.5, 0.082, 0.10, brz, 0.004, 3))
    out.append(mbox(25.6, 25.615, 5.0, 14.5, LEVEL_B + 0.03, 0.08, blk))
    # Black granite linings and floors.
    out += [mbox(25.6, 35.25, 5.0, 14.5, LEVEL_B - 0.01, LEVEL_B + 0.03, blk),
            mbox(25.615, 35.235, 5.0, 5.015, LEVEL_B + 0.03, TB - 0.10, blk), mbox(25.615, 35.235, 14.485, 14.5, LEVEL_B + 0.03, TB - 0.10, blk),
            mbox(35.6, 46.0, 5.0, 14.5, LEVEL_A - 0.01, LEVEL_A + 0.03, blk),
            mbox(35.615, 45.985, 5.0, 5.015, LEVEL_A + 0.03, TA - 0.10, blk), mbox(35.615, 45.985, 14.485, 14.5, LEVEL_A + 0.03, TA - 0.10, blk),
            mbox(45.985, 46.0, 5.015, 14.485, LEVEL_A + 0.03, TA - 0.10, blk)]
    # Basalt footings under the fire bowls.
    out += [mbox(30.02, 31.18, 9.17, 10.33, LEVEL_B + 0.03, LEVEL_B + 0.06, bas, 0.01, 2),
            mbox(40.42, 41.58, 9.17, 10.33, LEVEL_A + 0.03, LEVEL_A + 0.06, bas, 0.01, 2)]
    # Snow along the outer copings: thin at the water side, a soft lip over the outer edge.
    rng = np.random.default_rng(5)
    for X0, X1, ztop in ((25.63, 35.57, TB), (35.63, 46.38, TA)):
        xs = np.arange(X0, X1 + 1e-6, 0.035)
        prof = [(14.60, 0.0), (14.66, 0.35), (14.72, 0.7), (14.78, 0.92), (14.84, 1.0), (14.89, 0.9), (14.925, 0.55), (14.945, 0.0),
                (14.94, -0.45), (14.915, -0.6), (14.90, -0.15)]
        bm = bmesh.new()
        rows = []
        for X in xs:
            e = float(smoothstep(0.0, 0.35, min(X - X0, X1 - X)))
            th = (0.035 + 0.05 * float(vnoise2(np.array([X]), np.array([3.0]), 0.9, 7, 3)[0])) * e
            row = []
            for Y, f in prof:
                z = ztop + (th * f if f >= 0 else -0.01 + f * 0.05 * e)
                row.append(bm.verts.new((-X, Y + (rng.uniform(-0.004, 0.004) if 0.1 < f < 0.95 else 0.0), z)))
            rows.append(row)
        for a, b in zip(rows[:-1], rows[1:]):
            for k in range(len(prof) - 1):
                bm.faces.new((a[k], b[k], b[k + 1], a[k + 1]))
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
        o = core.mesh_object('snow', bm)
        core.orient_normals(o, (0, 0, 1))
        core.assign(o, sn)
        out.append(o)
    return [finish_mesh(out, 'SM_ECX_Cascade', [bas, blk, brz, sn], 30.0)]


def build_cascade_water():
    wat = plain_material('ecx_water', 0x0a1418, 0.03)
    fal = plain_material('ecx_falls', 0xbfd2da, 0.1, cull=False)
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new('UVMap')
    faces = []

    def plane(X0, X1, Y0, Y1, Z, nx=24, ny=12):
        vs = [[bm.verts.new((-(X0 + (X1 - X0) * i / nx), Y0 + (Y1 - Y0) * j / ny, Z)) for i in range(nx + 1)] for j in range(ny + 1)]
        for j in range(ny):
            for i in range(nx):
                f = bm.faces.new((vs[j][i], vs[j + 1][i], vs[j + 1][i + 1], vs[j][i + 1]))
                f.material_index = 0
                for l in f.loops:
                    l[uv].uv = (-l.vert.co.x, l.vert.co.y)
                faces.append(f)

    def sheet(Xlip, Ztop, Zbot, reach, Y0, Y1, ny=24, nz=12):
        rows = []
        for k in range(nz + 1):
            s = k / nz
            X = Xlip + reach * math.sqrt(s)
            Z = Ztop + (Zbot - Ztop) * s
            rows.append([bm.verts.new((-X, Y0 + (Y1 - Y0) * j / ny, Z)) for j in range(ny + 1)])
        for k in range(nz):
            for j in range(ny):
                f = bm.faces.new((rows[k][j], rows[k + 1][j], rows[k + 1][j + 1], rows[k][j + 1]))
                f.material_index = 1
                for l, (kk, jj) in zip(f.loops, ((k, j), (k + 1, j), (k + 1, j + 1), (k, j + 1))):
                    l[uv].uv = (jj / ny, kk / nz)
                faces.append(f)
    plane(25.615, 35.235, 5.015, 14.485, -0.37)
    plane(35.615, 45.985, 5.015, 14.485, -1.12)
    sheet(35.636, -0.376, -1.13, 0.13, 5.0, 14.5)
    sheet(25.69, 0.084, -0.38, 0.10, 5.0, 14.5)
    o = core.mesh_object('SM_ECX_Cascade_Water', bm)
    core.orient_normals(o)
    # planes face up, sheets face the walk (+X map = -x)
    bm2 = bmesh.new()
    bm2.from_mesh(o.data)
    bm2.normal_update()
    flip = [f for f in bm2.faces if (f.material_index == 0 and f.normal.z < 0) or (f.material_index == 1 and f.normal.x > 0)]
    bmesh.ops.reverse_faces(bm2, faces=flip)
    bm2.to_mesh(o.data)
    bm2.free()
    o.data.materials.append(wat.m)
    o.data.materials.append(fal.m)
    o.data.shade_smooth()
    print(f'[ecx_plaza] SM_ECX_Cascade_Water tris {tri_count(o)}')
    return [o]


# ================================================================== SM_ECX_FireBowl (+ _Flame)

BOWL_PROFILE = [(0.0, 0.9), (0.26, 0.9), (0.285, 0.93), (0.30, 0.962), (0.40, 0.99), (0.52, 1.05), (0.62, 1.12), (0.70, 1.19), (0.76, 1.25),
                (0.79, 1.285), (0.80, 1.30), (0.796, 1.311), (0.785, 1.309), (0.776, 1.292), (0.74, 1.243), (0.66, 1.163), (0.56, 1.093),
                (0.44, 1.033), (0.30, 0.998), (0.0, 0.985)]


def fire_copper(name):
    """Hammered bronze-copper: dimpled, lighter where the hammer struck, verdigris in the hollows outside, heat-blackened
    at the rim and inside, a faint heat tint below the rim."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    vor = m.node('ShaderNodeTexVoronoi', Scale=24.0)
    m.link(o, vor.inputs['Vector'])
    hd = vor.outputs['Distance']
    col = m.mix(ramp(m, noise(m, o, 8.0, 4.0), 0.3, 0.7), H(0x84502f), H(0xb06c3c))
    col = m.mix(m.math('MULTIPLY', ramp(m, hd, 0.45, 0.15), 0.3), col, H(0xcf9260))
    n = geo_normal(m)
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    r = m.math('MAXIMUM', m.math('SQRT', m.math('ADD', m.math('MULTIPLY', x, x), m.math('MULTIPLY', y, y))), 0.01)
    rd = m.math('DIVIDE', m.math('ADD', m.math('MULTIPLY', n.outputs['X'], x), m.math('MULTIPLY', n.outputs['Y'], y)), r)
    outside = ramp(m, rd, -0.02, 0.2)
    inside = m.math('SUBTRACT', 1.0, outside)
    heat = m.math('MAXIMUM', m.math('MULTIPLY', ramp(m, z, 1.24, 1.30), 0.9), m.math('MULTIPLY', inside, 0.95))
    heat = m.math('MULTIPLY', heat, m.math('ADD', 0.6, m.math('MULTIPLY', noise(m, o, 14.0, 3.0), 0.5)))
    tint = m.math('MULTIPLY', m.math('MULTIPLY', ramp(m, z, 1.12, 1.19), m.math('SUBTRACT', 1.0, ramp(m, z, 1.2, 1.26))), outside)
    col = m.mix(m.math('MULTIPLY', tint, 0.45), col, H(0x4a3550))
    col = m.mix(m.math('MINIMUM', heat, 1.0), col, H(0x1a1410))
    cav = ao_cavity(m, 0.025, 1.8)
    verd = m.math('MULTIPLY', outside, m.math('ADD', m.math('MULTIPLY', cav, 0.8), m.math('MULTIPLY', ramp(m, noise(m, o, 5.0, 5.0), 0.56, 0.7), 0.7)))
    verd = m.math('MULTIPLY', m.math('MINIMUM', verd, 1.0), m.math('SUBTRACT', 1.0, ramp(m, z, 1.17, 1.26)))
    verd = m.math('MAXIMUM', verd, m.math('MULTIPLY', ramp(m, hd, 0.32, 0.5), m.math('MULTIPLY', outside, 0.25)))
    vcol = m.mix(ramp(m, noise(m, o, 40.0, 3.0), 0.3, 0.7), H(0x3f7a68), H(0x86b8a2))
    col = m.mix(verd, col, vcol)
    ed = convex_edges(m, tc, distance=0.004, samples=10, breakup_scale=30.0, gain=4.0)
    col = m.mix(m.math('MULTIPLY', ed, m.math('SUBTRACT', 1.0, heat)), col, H(0xd49a68))
    m.set('Base Color', col)
    m.set('Roughness', mix_float(m, verd, m.math('ADD', 0.3, m.math('MULTIPLY', heat, 0.3)), 0.88))
    m.set('Metallic', m.math('SUBTRACT', 1.0, m.math('ADD', m.math('MULTIPLY', verd, 0.9), m.math('MULTIPLY', heat, 0.35)), clamp=True))
    set_bump(m, m.math('ADD', hd, m.math('MULTIPLY', verd, m.math('MULTIPLY', noise(m, o, 300.0, 2.0), 0.6))), 0.45, 0.003)
    return m


def fire_media(name):
    """Lava rock and ember glass on a bed of grit (ecx_kind 0 bed, 1 rock, 2 glass); faint embers glow in the
    gaps between the rocks (baked emissive)."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    kind = attr(m, 'ecx_kind')
    rock_t = noise(m, o, 14.0, 3.0)
    pores = ramp(m, noise(m, o, 260.0, 2.0), 0.62, 0.7)
    rock = m.mix(ramp(m, rock_t, 0.25, 0.75), H(0x1c1614), H(0x45302a))
    rock = m.mix(m.math('MULTIPLY', ramp(m, noise(m, o, 5.0, 2.0), 0.45, 0.8), 0.7), rock, H(0x5c2a1e))
    rock = m.mix(m.math('MULTIPLY', pores, 0.8), rock, H(0x0d0908))
    bed = m.mix(ramp(m, noise(m, o, 120.0, 2.0), 0.3, 0.7), H(0x18130f), H(0x2e241c))
    glass = m.mix(ramp(m, noise(m, o, 30.0, 2.0), 0.3, 0.7), H(0x6a2408), H(0xa04a10))
    is_r = is_k(m, kind, 1)
    is_g = is_k(m, kind, 2)
    col = m.mix(is_r, bed, rock)
    col = m.mix(is_g, col, glass)
    cav = ramp(m, ao_cavity(m, 0.04, 1.6), 0.55, 0.95)
    patch = ramp(m, noise(m, o, 16.0, 3.0), 0.5, 0.75)
    # embers only deep between the rocks, in patches, never at the bowl's rim
    inner = ramp(m, m.math('SQRT', m.math('ADD', m.math('MULTIPLY', sep.outputs['X'], sep.outputs['X']),
                                         m.math('MULTIPLY', sep.outputs['Y'], sep.outputs['Y']))), 0.62, 0.45)
    ember = m.math('ADD', m.math('MULTIPLY', m.math('MULTIPLY', cav, patch), m.math('SUBTRACT', 1.0, is_g)), m.math('MULTIPLY', is_g, 0.25))
    ember = m.math('MULTIPLY', ember, inner)
    ecol = m.mix(ramp(m, noise(m, o, 9.0, 2.0), 0.3, 0.8), H(0xff3c0a), H(0xff8a30))
    m.set('Base Color', col)
    m.set('Roughness', mix_float(m, is_g, mix_float(m, is_r, 0.92, 0.82), 0.06))
    m.set('Specular IOR Level', mix_float(m, is_g, 0.4, 0.7))
    m.set('Emission Color', cmul(m, ecol, m.math('MINIMUM', ember, 1.0)))
    m.set('Emission Strength', 5.0)
    set_bump(m, m.math('ADD', rock_t, m.math('MULTIPLY', pores, -0.8)), 0.5, 0.004)
    return m


def build_firebowl():
    cop = fire_copper('ecxf_copper')
    bas = basalt('ecxf_basalt', ground=0.0, salt=0.4, streaks=0.4, wet_foot=0.7)
    med = fire_media('ecxf_media')
    out = [box(-0.5, 0.5, -0.5, 0.5, 0.0, 0.9, bas, 0.012, 3)]
    # a shadow reveal under the plinth's cap
    out.append(box(-0.49, 0.49, -0.49, 0.49, 0.835, 0.848, bas, 0.0, 1))
    bowl = lathe('bowl', BOWL_PROFILE, cop, 128)
    # hammer the bowl a little out of round
    rng = np.random.default_rng(3)
    for v in bowl.data.vertices:
        r = math.hypot(v.co.x, v.co.y)
        if r > 0.32:
            k = 1.0 + 0.004 * float(vnoise2(np.array([v.co.x]), np.array([v.co.y + v.co.z]), 0.06, 3, 2)[0] - 0.5)
            v.co.x *= k
            v.co.y *= k
    out.append(bowl)
    bed = lathe('bed', [(0.0, 1.212), (0.715, 1.212), (0.722, 1.205)], None, 64)
    core.orient_normals(bed, (0, 0, 1))
    core.assign(tag(bed, 0), med)
    bm = bmesh.new()
    for i in range(150):
        rr = 0.68 * math.sqrt(rng.random())
        a = rng.random() * 2 * math.pi
        s = rng.uniform(0.038, 0.075) * (1.0 - 0.2 * rr / 0.68)
        res = bmesh.ops.create_icosphere(bm, subdivisions=2, radius=1.0)
        sx, sy, sz = s * rng.uniform(0.8, 1.25), s * rng.uniform(0.8, 1.25), s * rng.uniform(0.55, 0.9)
        rot = Matrix.Rotation(rng.uniform(0, 6.3), 4, 'Z') @ Matrix.Rotation(rng.uniform(-0.4, 0.4), 4, 'X')
        cz = 1.212 + sz * rng.uniform(0.15, 0.6) + 0.02 * (1 - rr / 0.68)
        # broken lava rock, not pebbles: three or four fracture planes shear the lump into flat-faced chunks
        cuts = []
        for _ in range(int(rng.integers(3, 5))):
            nv = Vector((rng.normal(), rng.normal(), rng.normal() * 0.6)).normalized()
            cuts.append((nv, rng.uniform(0.5, 0.78)))
        for v in res['verts']:
            p = Vector(v.co) * rng.uniform(0.86, 1.1)
            for nv, off in cuts:
                k = p.dot(nv) - off
                if k > 0.0:
                    p -= nv * (k * 0.92)
            d = Vector((p.x * sx, p.y * sy, p.z * sz))
            v.co = (rot @ d.to_4d()).to_3d() + Vector((rr * math.cos(a), rr * math.sin(a), cz))
    rocks = core.mesh_object('rocks', bm)
    core.assign(tag(rocks, 1), med)
    bm = bmesh.new()
    for i in range(46):
        rr = 0.62 * math.sqrt(rng.random())
        a = rng.random() * 2 * math.pi
        s = rng.uniform(0.02, 0.045)
        res = bmesh.ops.create_icosphere(bm, subdivisions=0, radius=1.0)
        rot = Matrix.Rotation(rng.uniform(0, 6.3), 4, 'Z') @ Matrix.Rotation(rng.uniform(-0.9, 0.9), 4, 'X')
        for v in res['verts']:
            d = Vector((v.co.x * s * rng.uniform(0.6, 1.4), v.co.y * s * rng.uniform(0.6, 1.4), v.co.z * s * rng.uniform(0.5, 1.0)))
            v.co = (rot @ d.to_4d()).to_3d() + Vector((rr * math.cos(a), rr * math.sin(a), 1.22 + s * 0.5))
    gl = core.mesh_object('glass', bm)
    core.assign(tag(gl, 2), med)
    out += [bed, rocks, gl]
    obj = finish_mesh(out, 'SM_ECX_FireBowl', [cop, bas, med], 40.0)
    # flame quads
    fl = plain_material('ecx_flame', 0xffa040, 0.5, cull=False, emit=0xff8a30, emit_strength=4.0)
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new('UVMap')
    for k in range(3):
        a = math.radians(60.0 * k)
        dx, dy = 0.7 * math.cos(a), 0.7 * math.sin(a)
        vs = [bm.verts.new((-dx, -dy, 1.05)), bm.verts.new((dx, dy, 1.05)), bm.verts.new((dx, dy, 2.35)), bm.verts.new((-dx, -dy, 2.35))]
        f = bm.faces.new(vs)
        for l, t in zip(f.loops, ((0, 0), (1, 0), (1, 1), (0, 1))):
            l[uv].uv = t
    flame = core.mesh_object('SM_ECX_FireBowl_Flame', bm)
    flame.data.materials.append(fl.m)
    return [obj, flame]


# ================================================================== SM_ECX_MonumentSign (+ _GlowEmber, _Screen)

def shape(name, poly, w0, w1, mat, front, face_x, bevel=0.0, holes=()):
    """A flat face-coordinate shape (u right, v up as seen from outside the face) extruded outward from w0 to w1 and
    put on the PROP's front (-X) or back (+X) face whose plane is x = face_x."""
    o = core.extrude_outline(name, [tuple(p) for p in poly], holes=[[tuple(p) for p in h] for h in holes], depth=w1 - w0, z=w0, bevel=bevel)
    core.apply_transforms(o)
    core.orient_normals(o)
    o.data.transform(face_matrix(front, 0.0, 0.0, face_x))
    if mat is not None:
        core.assign(o, mat)
    return o


def circle_poly(c, r, n=96):
    return [(c[0] + r * math.cos(2 * math.pi * k / n), c[1] + r * math.sin(2 * math.pi * k / n)) for k in range(n)]


def mark_shapes(cu, cv, size):
    """The brand mark's shapes in face coordinates (brand.mark's construction, meters)."""
    u = size / 100.0
    base = cv - 28 * u

    def P(x, y):
        return (cu + x * u, base + y * u)
    return {'disc': (P(0, 42), 34 * u),
            'crest': [P(-48, 0), P(-26, 34), P(-15, 22), P(0, 60), P(13, 29), P(25, 40), P(48, 0)],
            'shade': [[P(-26, 34), P(-15, 22), P(-13, 0), P(-21, 0)], [P(0, 60), P(13, 29), P(15, 0), P(3, 0)], [P(25, 40), P(48, 0), P(31, 0)]],
            'snow': [P(0, 60), P(-8, 43), P(-4.5, 46.5), P(-1.5, 42), P(1.6, 49)],
            'bars': [[P(-48, -6.5), P(-3, -6.5), P(-3, -4.1), P(-48, -4.1)], [P(3, -6.5), P(48, -6.5), P(48, -4.1), P(3, -4.1)]],
            'u': u}


def channel_letters(body, face, width, u_left, v_bottom, w0, w1, front, face_x, mat_face, mat_side, tracking=1.22, offset=0.0, flat=False):
    """Text fitted to `width`, extruded from w0 to w1 off the face: its front faces take mat_face, the returns
    mat_side (flat: a single sheet at w0, offset grows the glyphs)."""
    ref = text_mesh(body, 1.0, face, 'LEFT', tracking, extrude=0.0)
    x0, x1, y0, y1 = text_bounds(ref)
    bpy.data.objects.remove(ref)
    k = width / (x1 - x0)
    t = text_mesh(body, 1.0, face, 'LEFT', tracking, extrude=0.0 if flat else 0.5, offset=offset / k, bevel=0.0 if flat else 0.006, res=4)
    me = t.data
    half = 0.5 if not flat else 0.0
    for v in me.vertices:
        v.co.x = (v.co.x - x0) * k + u_left
        v.co.y = (v.co.y - y0) * k + v_bottom
        v.co.z = w0 + (v.co.z + half) / (2 * half) * (w1 - w0) if not flat else w0
    me.materials.clear()
    me.materials.append(mat_face.m if isinstance(mat_face, core.Mat) else mat_face)
    if mat_side is not None:
        me.materials.append(mat_side.m if isinstance(mat_side, core.Mat) else mat_side)
        me.update()
        for p in me.polygons:
            p.material_index = 0 if p.normal.z > 0.9 else 1
    core.orient_normals(t) if not flat else None
    if flat:
        for p in me.polygons:
            if p.normal.z < 0:
                p.flip()
    me.transform(face_matrix(front, 0.0, 0.0, face_x))
    return t, k * 0.711


def rough_cap(x0, x1, y0, y1, zb, ztop, step, amp_top, amp_side, seed, name='cap'):
    """A split-face cap: a rough top over (x0..x1, y0..y1) at about ztop, its sides bulging irregularly, closing down
    to the exact outline at zb. Returns (object, height function of the top)."""
    def top_h(X, Y):
        X = np.asarray(X, float)
        Y = np.asarray(Y, float)
        n1 = vnoise2(X * 1.0 + 13.0, Y, 0.22, seed, 4)
        n2 = vnoise2(X, Y * 1.0 + 7.0, 0.07, seed + 1, 2)
        return ztop + amp_top * ((n1 - 0.5) * 2.0 + (n2 - 0.5) * 0.5)
    nx = max(2, int(round((x1 - x0) / step)))
    ny = max(2, int(round((y1 - y0) / step)))
    xs = np.linspace(x0, x1, nx + 1)
    ys = np.linspace(y0, y1, ny + 1)
    bm = bmesh.new()
    grid = {}
    for j, y in enumerate(ys):
        for i, x in enumerate(xs):
            z = float(top_h(x, y))
            ox = oy = 0.0
            if i in (0, nx) or j in (0, ny):
                push = amp_side * (0.4 + 0.8 * float(vnoise2(np.array([x * 3.0]), np.array([y * 3.0 + 5.0]), 0.12, seed + 3, 2)[0]))
                ox = -push if i == 0 else (push if i == nx else 0.0)
                oy = -push if j == 0 else (push if j == ny else 0.0)
            grid[(i, j)] = bm.verts.new((x + ox, y + oy, z))
    for j in range(ny):
        for i in range(nx):
            bm.faces.new((grid[(i, j)], grid[(i + 1, j)], grid[(i + 1, j + 1)], grid[(i, j + 1)]))
    loop = [(i, 0) for i in range(nx)] + [(nx, j) for j in range(ny)] + [(i, ny) for i in range(nx, 0, -1)] + [(0, j) for j in range(ny, 0, -1)]
    ring = [grid[k] for k in loop]
    rows = [ring]
    ns = 4
    for s in range(1, ns + 1):
        f = s / ns
        new = []
        for k, v in zip(loop, ring):
            bx = xs[k[0]]
            by = ys[k[1]]
            z = v.co.z + (zb - v.co.z) * f
            px = v.co.x + (bx - v.co.x) * (f ** 0.6)
            py = v.co.y + (by - v.co.y) * (f ** 0.6)
            if s < ns:
                jit = amp_side * 0.5 * (float(vnoise2(np.array([bx * 4.0 + z * 9.0]), np.array([by * 4.0]), 0.1, seed + 5, 2)[0]) - 0.5)
                px += (jit if k[0] == nx else (-jit if k[0] == 0 else 0.0))
                py += (jit if k[1] == ny else (-jit if k[1] == 0 else 0.0))
            new.append(bm.verts.new((px, py, z)))
        rows.append(new)
    n = len(loop)
    for a, b in zip(rows[:-1], rows[1:]):
        for k in range(n):
            bm.faces.new((a[k], a[(k + 1) % n], b[(k + 1) % n], b[k]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    o = core.mesh_object(name, bm)
    core.orient_normals(o, (0, 0, 1))
    return o, top_h


def snow_cap(x0, x1, y0, y1, top_h, step, thick, seed, over=0.025, name='snowcap', ridge_axis='Y'):
    """A snow layer on a top (top_h(x, y)), rounding over its edges by `over` and tucked back under them."""
    nx = max(2, int(round((x1 - x0 + 2 * over) / step)))
    ny = max(2, int(round((y1 - y0 + 2 * over) / step)))
    xs = np.linspace(x0 - over, x1 + over, nx + 1)
    ys = np.linspace(y0 - over, y1 + over, ny + 1)
    bm = bmesh.new()
    grid = {}
    for j, y in enumerate(ys):
        for i, x in enumerate(xs):
            cx, cy = min(max(x, x0), x1), min(max(y, y0), y1)
            out = max(abs(x - cx), abs(y - cy))
            inner = min(cx - x0, x1 - cx, cy - y0, y1 - cy)
            body = float(smoothstep(0.0, 0.12, inner))
            n = float(vnoise2(np.array([x]), np.array([y]), 0.35, seed, 3)[0])
            th = thick * (0.55 + 0.7 * n) * (0.35 + 0.65 * body)
            z = float(top_h(cx, cy)) + th - (out / over) ** 1.5 * (th + 0.012) if over > 0 else float(top_h(cx, cy)) + th
            grid[(i, j)] = bm.verts.new((x, y, z))
    for j in range(ny):
        for i in range(nx):
            bm.faces.new((grid[(i, j)], grid[(i + 1, j)], grid[(i + 1, j + 1)], grid[(i, j + 1)]))
    loop = [(i, 0) for i in range(nx)] + [(nx, j) for j in range(ny)] + [(i, ny) for i in range(nx, 0, -1)] + [(0, j) for j in range(ny, 0, -1)]
    under = []
    for k in loop:
        v = grid[k]
        cx, cy = min(max(v.co.x, x0 + 0.015), x1 - 0.015), min(max(v.co.y, y0 + 0.015), y1 - 0.015)
        under.append(bm.verts.new((cx, cy, float(top_h(cx, cy)) - 0.03)))
    n = len(loop)
    for k in range(n):
        a, b = grid[loop[k]], grid[loop[(k + 1) % n]]
        bm.faces.new((a, b, under[(k + 1) % n], under[k]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    o = core.mesh_object(name, bm)
    core.orient_normals(o, (0, 0, 1))
    return o


def lit_sign(name, disc_centers, disc_r, strength=12.0):
    """The monument's lit slot: the letter faces in ash (ecx_kind 0), the ember disc (ecx_kind 1) running from deep
    ember at its rim to flame at its heart, hotter low; disc_centers: (y, z) on the front (x < 0) and back."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    kind = attr(m, 'ecx_kind')
    front = m.math('LESS_THAN', sep.outputs['X'], 0.0)
    (yf, zf), (yb, zb) = disc_centers
    cy = mix_float(m, front, yb, yf)
    cz = m.math('SUBTRACT', mix_float(m, front, zb, zf), 0.2 * disc_r)
    dy = m.math('SUBTRACT', sep.outputs['Y'], cy)
    dz = m.math('SUBTRACT', sep.outputs['Z'], cz)
    t = m.math('DIVIDE', m.math('SQRT', m.math('ADD', m.math('MULTIPLY', dy, dy), m.math('MULTIPLY', dz, dz))), disc_r * 1.15)
    stops = [(0.0, brand.FLAME), (0.18, brand.GOLD), (0.42, brand.EMBER_HOT), (0.7, brand.EMBER), (0.95, brand.EMBER_DEEP)]
    col = H(stops[0][1])
    for (t0, c0), (t1, c1) in zip(stops[:-1], stops[1:]):
        col = m.mix(ramp(m, t, t0, t1), col, H(c1))
    letters = H(brand.ASH)
    c = m.mix(is_k(m, kind, 1), letters, col)
    # ecx_kind 3: the subline's faces, a softer brand gold (about half the letters' level)
    gold = H(brand.GOLD)
    c = m.mix(is_k(m, kind, 3), c, (gold[0] * 0.5, gold[1] * 0.5, gold[2] * 0.5, 1.0))
    m.set('Base Color', c)
    m.set('Roughness', 0.35)
    m.set('Emission Color', c)
    m.set('Emission Strength', strength)
    return m


def build_monument():
    L2, T2 = 4.5, 0.6
    bas = basalt('ecxm_basalt', ground=0.5, salt=0.3, streaks=0.8, wet_foot=0.3, split_above=4.0, rough_base=0.52)
    brz = bronze('ecxm_bronze', scuff=0.5, patina=0.6, edge=0.004, facets=True)
    gra = granite('ecxm_granite', wet=False, ground=0.0, salt=0.8)
    sn = snow('ecxm_snow')
    size = 2.5
    cu, cv = -L2 + 0.35 + 1.275, 2.45
    ms = mark_shapes(cu, cv, size)
    (dcu, dcv), dr = ms['disc']
    lit = lit_sign('ecxm_lit', ((-dcu, dcv), (dcu, dcv)), dr)
    out, glows = [], []
    out.append(box(-0.55, 0.55, -L2 + 0.05, L2 - 0.05, 0.0, 0.5, gra, 0.008, 2))
    out.append(box(-T2, T2, -L2, L2, 0.5, 4.0, bas, 0.006, 2))
    cap, top_h = rough_cap(-T2, T2, -L2, L2, 3.995, 4.24, 0.05, 0.035, 0.014, 17, 'split')
    core.assign(cap, bas)
    out.append(cap)
    sc = snow_cap(-T2, T2, -L2, L2, lambda x, y: top_h(x, y) + 0.004, 0.06, 0.09, 23, 0.03)
    core.assign(sc, sn)
    out.append(sc)
    word_l = cu + 1.275 + 0.40
    word_w = (L2 - 0.35) - word_l
    for front, fx in ((True, -T2), (False, T2)):
        sgn = -1.0 if front else 1.0
        # the ember disc, its bronze rim, the crest in relief over it, the shaded and snow facets, the ground line
        out.append(tag(shape('disc', circle_poly((dcu, dcv), dr), 0.0, 0.012, lit, front, fx), 1))
        ring = shape('ring', circle_poly((dcu, dcv), dr + 0.035), 0.0, 0.02, brz, front, fx, holes=[list(reversed(circle_poly((dcu, dcv), dr)))])
        out.append(ring)
        out.append(shape('crest', ms['crest'], 0.012, 0.058, brz, front, fx, bevel=0.003))
        for f in ms['shade']:
            out.append(tag(shape('shade', f, 0.058, 0.0605, brz, front, fx), 1))
        out.append(tag(shape('snowf', ms['snow'], 0.058, 0.0612, brz, front, fx), 2))
        for b in ms['bars']:
            out.append(shape('bar', b, 0.0, 0.03, brz, front, fx, bevel=0.002))
        # EMBERCREST: bronze channel letters, their faces lit ash; the halo behind; the subline in polished bronze
        let, cap_h = channel_letters('EMBERCREST', 'Black', word_w, word_l, 2.62, 0.025, 0.085, front, fx, lit, brz)
        out.append(tag(let, 0))
        halo, _ = channel_letters('EMBERCREST', 'Black', word_w, word_l, 2.62, 0.003, 0.003, front, fx, H(0xffffff) and lit, None, offset=0.014, flat=True)
        glows.append(halo)
        # (lit faces in a softer gold, bronze returns: polished bronze alone mirrored the black sky and went
        # unreadable at night)
        sub, _ = channel_letters('RESORT  ·  CASINO  ·  POKER SERIES', 'Bold', word_w, word_l, 2.16, 0.0, 0.015, front, fx, lit, brz, tracking=1.9)
        out.append(tag(sub, 3))
        # the LED strip's bezel (the screen sits inside it at 1 cm off the stone)
        for (u0, u1, v0, v1) in ((-4.05, 4.05, 1.15, 1.20), (-4.05, 4.05, 0.60, 0.65), (-4.05, -4.0, 0.65, 1.15), (4.0, 4.05, 0.65, 1.15)):
            out.append(shape('bezel', rect(u0, u1, v0, v1), 0.0, 0.025, brz, front, fx, bevel=0.002))
        # the base line: an ember LED under the slab's overhang
        glows.append(box(sgn * 0.578, sgn * 0.556, -L2 + 0.1, L2 - 0.1, 0.484, 0.496, None))
    # drifts against the plinth, both faces, and a little at the ends
    rng_seed = 31
    for sgn in (-1.0, 1.0):
        ys = np.arange(-L2 + 0.05, L2 - 0.05 + 1e-6, 0.06)
        ts = np.linspace(0.0, 1.0, 9)
        bm = bmesh.new()
        rows = []
        for y in ys:
            end = float(smoothstep(0.0, 0.5, min(y + L2 - 0.05, L2 - 0.05 - y)))
            h = (0.07 + 0.2 * float(vnoise2(np.array([y]), np.array([sgn * 3.0]), 0.9, rng_seed, 3)[0])) * end
            reach = 0.18 + 0.12 * float(vnoise2(np.array([y]), np.array([sgn * 5.0]), 0.6, rng_seed + 1, 2)[0])
            rows.append([bm.verts.new((sgn * (0.551 + reach * t), y, h * (1 - t) ** 1.7 - 0.012 * t)) for t in ts])
        for a, b in zip(rows[:-1], rows[1:]):
            for k in range(len(ts) - 1):
                bm.faces.new((a[k], b[k], b[k + 1], a[k + 1]))
        d = core.mesh_object('drift', bm)
        core.orient_normals(d, (0, 0, 1))
        core.assign(d, sn)
        out.append(d)
    obj = finish_mesh(out, 'SM_ECX_MonumentSign', [bas, brz, gra, lit, sn], 30.0)
    g = glow(glows, 'SM_ECX_MonumentSign_GlowEmber')
    # the LED message strip: two quads, each reading left to right from its own side
    scr = plain_material('ecx_screen', 0x050608, 0.3, emit=0x101010, emit_strength=1.0)
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new('UVMap')
    for front in (True, False):
        x = -0.61 if front else 0.61
        ys = (4.0, -4.0) if front else (-4.0, 4.0)
        vs = [bm.verts.new((x, ys[0], 0.65)), bm.verts.new((x, ys[1], 0.65)), bm.verts.new((x, ys[1], 1.15)), bm.verts.new((x, ys[0], 1.15))]
        f = bm.faces.new(vs)
        for l, t in zip(f.loops, ((0, 0), (1, 0), (1, 1), (0, 1))):
            l[uv].uv = t
    bm.normal_update()
    for f in bm.faces:
        want = -1.0 if f.calc_center_median().x < 0 else 1.0
        if f.normal.x * want < 0:
            f.normal_flip()
    screen = core.mesh_object('SM_ECX_MonumentSign_Screen', bm)
    screen.data.materials.append(scr.m)
    return [obj, g, screen]


# ================================================================== prints (sheets rendered into atlases)

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
    """Panels rendered one by one (Sheet) and pasted into rects (u0, v0, u1, v1) of one N x N image."""

    def __init__(self, name, n):
        self.name, self.n = name, n
        self.img = np.zeros((n, n, 4), np.float32)
        self.img[..., 3] = 1.0
        self.img[..., :3] = 0.1

    def put(self, rect_uv, panel_fn, w, h, samples=16):
        u0, v0, u1, v1 = rect_uv
        pw = int(round((u1 - u0) * self.n))
        ph = int(round((v1 - v0) * self.n))
        s = Sheet(w, h)
        panel_fn(s, w, h)
        a = sheet_px(s, f'{self.name}_{len(self.__dict__)}_{u0:.2f}_{v0:.2f}', pw, samples)
        a = a[:ph, :pw] if a.shape[0] >= ph else np.pad(a, ((0, ph - a.shape[0]), (0, 0), (0, 0)), mode='edge')
        r0 = self.n - int(round(v1 * self.n))
        self.img[r0:r0 + a.shape[0], int(round(u0 * self.n)):int(round(u0 * self.n)) + a.shape[1], :3] = a[..., :3] * a[..., 3:4] + 0.1 * (1 - a[..., 3:4])
        return self

    def save(self):
        path = os.path.join(core.BUILD_DIR, 'textures', f'{self.name}.png')
        _png(path, self.img[..., :3])
        return path


def print_material(name, path, lit_rects=(), strength=10.0, rough=0.32, base_rects_rough=None):
    """A prints slot: the atlas on the UVs; inside lit_rects it glows (a backlit panel)."""
    m = core.Mat(name)
    uvn = m.node('ShaderNodeUVMap')
    uvn.uv_map = 'UVMap'
    img = m.image(path, vector=uvn.outputs['UV'], extension='EXTEND')
    tc, sep = object_coords(m)
    us = m.node('ShaderNodeSeparateXYZ')
    m.link(uvn.outputs['UV'], us.inputs['Vector'])
    u, v = us.outputs['X'], us.outputs['Y']
    mask = None
    for (u0, v0, u1, v1) in lit_rects:
        r = m.math('MULTIPLY', m.math('MULTIPLY', m.math('GREATER_THAN', u, u0), m.math('LESS_THAN', u, u1)),
                   m.math('MULTIPLY', m.math('GREATER_THAN', v, v0), m.math('LESS_THAN', v, v1)))
        mask = r if mask is None else m.math('MAXIMUM', mask, r)
    m.set('Base Color', img.outputs['Color'])
    m.set('Roughness', m.math('ADD', rough, m.math('MULTIPLY', noise(m, tc.outputs['Object'], 9.0, 3.0), 0.1)))
    m.set('Specular IOR Level', 0.45)
    if mask is not None:
        m.set('Emission Color', cmul(m, img.outputs['Color'], mask))
        m.set('Emission Strength', strength)
    return m


def print_quad(name, corners, uv_rect, u_axis, v_axis, origin, size, mat):
    """One printed quad: corners (4 Blender points, outward winding), its atlas rect; u = (p - origin) . u_axis / size[0]."""
    bm = bmesh.new()
    vs = [bm.verts.new(c) for c in corners]
    bm.faces.new(vs)
    o = core.mesh_object(name, bm)
    core.assign(o, mat)
    u0, v0, u1, v1 = uv_rect
    O, U, V = Vector(origin), Vector(u_axis), Vector(v_axis)
    lock_uvs(o, lambda co: (u0 + (u1 - u0) * ((Vector(co) - O).dot(U) / size[0]), v0 + (v1 - v0) * ((Vector(co) - O).dot(V) / size[1])))
    return o


# ---- the shelter's prints (§3.6)

STOPS = ['NORTHSIDE · FIFTH ST & MARKET', 'MARKET & 9TH', 'CIVIC CENTER', 'UNION STATION', 'RIVERFRONT', 'OLD MILL RD', 'PINE FLATS',
         'SWITCHBACK LOOKOUT', 'EMBERCREST RESORT']
TRANSFER = {0, 3}
INK = 0x1b1d20


def route_badge(s, cx, cy, r, number='14'):
    s.circle(cx, cy, r, TRANSIT_GREEN, n=96)
    s.ring(cx, cy, r * 0.86, r * 0.92, 0xffffff, n=96)
    s.text(number, cx, cy - r * 0.36, r * 1.05, 0xffffff, 'Black', 'CENTER')


def transit_header(s, w, y0, h, title):
    s.rect(0, y0, w, h, TRANSIT_GREEN)
    s.rect(0, y0 - h * 0.07, w, h * 0.05, TRANSIT_GOLD)
    s.text(title, w * 0.05, y0 + h * 0.3, h * 0.42, 0xffffff, 'Black', 'LEFT', tracking=1.05)


def diamond(s, cx, cy, r, color):
    s.poly([(cx, cy - r), (cx + r, cy), (cx, cy + r), (cx - r, cy)], color)


def panel_route_map(s, w, h):
    s.rect(0, 0, w, h, 0xf2f1ec)
    transit_header(s, w, h * 0.88, h * 0.12, 'CITY TRANSIT')
    route_badge(s, w * 0.15, h * 0.775, w * 0.1)
    s.text('EMBERCREST · RIDGE ROAD', w * 0.29, h * 0.79, w * 0.052, INK, 'Bold', 'LEFT')
    s.text('Northside · Fifth St  to  Embercrest Resort', w * 0.29, h * 0.755, w * 0.032, 0x4a4d52, 'Regular', 'LEFT')
    # A faint ridge behind the upper stops.
    pts = [(0, h * 0.58)]
    for k in range(14):
        pts.append((w * (k + 0.5) / 14, h * (0.6 + 0.06 * abs(math.sin(k * 1.7)) + (0.05 if k in (5, 7) else 0.0) + (0.09 if k == 6 else 0.0))))
    pts += [(w, h * 0.6), (w, h * 0.55), (0, h * 0.55)]
    s.poly(pts, 0xe4e6e8)
    x = w * 0.17
    yb, yt = h * 0.12, h * 0.68
    s.rect(x - w * 0.012, yb, w * 0.024, yt - yb, TRANSIT_GREEN)
    for i, name in enumerate(STOPS):
        y = yb + (yt - yb) * i / (len(STOPS) - 1)
        term = i in (0, len(STOPS) - 1)
        r = w * (0.032 if term else 0.022)
        s.circle(x, y, r, TRANSIT_GREEN, n=48)
        s.circle(x, y, r * 0.6, 0xffffff, n=48)
        if i in TRANSFER:
            diamond(s, x + w * 0.065, y, w * 0.018, INK)
        s.text(name, x + w * (0.11 if i in TRANSFER else 0.07), y - w * 0.014, w * (0.042 if term else 0.034), INK, 'Bold' if term else 'Regular', 'LEFT')
    yt_ = yt
    brand.mark(s, w * 0.86, yt_ + w * 0.005, w * 0.13)
    s.rect(x + w * 0.07, yt_ - w * 0.075, w * 0.36, w * 0.042, EMBER_LED)
    s.text('YOU ARE HERE', x + w * 0.085, yt_ - w * 0.066, w * 0.026, 0xffffff, 'Bold', 'LEFT', tracking=1.2)
    diamond(s, w * 0.07, h * 0.062, w * 0.012, INK)
    s.text('Transfer', w * 0.095, h * 0.055, w * 0.026, INK, 'Regular', 'LEFT')
    s.rect(w * 0.05, h * 0.045, w * 0.9, h * 0.002, 0x9a9da2)
    s.text('Fare $2.90 · Exact fare or pass · 20 min to Fifth & Market', w * 0.05, h * 0.018, w * 0.029, INK, 'Regular', 'LEFT')


def panel_ad(s, w, h):
    brand.banner_art(s, w, h, tagline=False)
    s.text('NIGHTLY AT 7', w * 0.5, h * 0.33, w * 0.115, brand.ASH, 'Black', 'CENTER', tracking=1.1)
    s.text('NOON DEEPSTACK  ·  10:30 TURBO', w * 0.5, h * 0.285, w * 0.042, brand.GOLD, 'Bold', 'CENTER', tracking=1.3)
    s.text('THE EMBERCREST  ·  RESORT  ·  CASINO', w * 0.5, h * 0.215, w * 0.036, brand.ICE, 'Bold', 'CENTER', tracking=1.4)
    s.text('Take the 14 to the top of the ridge', w * 0.5, h * 0.185, w * 0.03, brand.ICE, 'Regular', 'CENTER')


def panel_timetable(s, w, h):
    s.rect(0, 0, w, h, 0xf4f3ef)
    transit_header(s, w, h * 0.87, h * 0.13, 'CITY TRANSIT')
    route_badge(s, w * 0.16, h * 0.77, w * 0.1)
    s.text('DEPARTURES TO', w * 0.32, h * 0.795, w * 0.05, 0x4a4d52, 'Bold', 'LEFT', tracking=1.1)
    s.text('NORTHSIDE', w * 0.32, h * 0.745, w * 0.085, INK, 'Black', 'LEFT')
    s.text('Daily 6:15 AM – 11:15 PM', w * 0.07, h * 0.665, w * 0.05, INK, 'Bold', 'LEFT')
    s.text('every 30 min (:15 and :45)', w * 0.07, h * 0.63, w * 0.042, 0x4a4d52, 'Regular', 'LEFT')
    cols = [('AM', [6, 7, 8, 9, 10, 11]), ('PM', [12, 1, 2, 3, 4, 5]), ('PM', [6, 7, 8, 9, 10, 11])]
    for c, (ap, hrs) in enumerate(cols):
        x = w * (0.07 + 0.31 * c)
        s.text(ap, x, h * 0.585, w * 0.034, TRANSIT_GREEN, 'Bold', 'LEFT')
        for r, hr in enumerate(hrs):
            y = h * (0.545 - 0.036 * r)
            s.text(f'{hr}', x, y, w * 0.038, INK, 'Bold', 'LEFT')
            s.text(':15  :45', x + w * 0.07, y, w * 0.034, 0x3a3d42, 'Regular', 'LEFT')
    s.rect(w * 0.05, h * 0.2, w * 0.9, h * 0.12, brand.NAVY)
    s.circle(w * 0.13, h * 0.26, w * 0.04, 0xf0e6c8, n=48)
    s.circle(w * 0.145, h * 0.27, w * 0.036, brand.NAVY, n=48)
    s.text('OWL SERVICE', w * 0.21, h * 0.275, w * 0.045, 0xffffff, 'Black', 'LEFT', tracking=1.1)
    s.text('12:15 · 1:15 · 2:15 · 3:15 AM', w * 0.21, h * 0.225, w * 0.04, brand.GOLD, 'Bold', 'LEFT')
    s.text('Buses wait at the Embercrest stop', w * 0.07, h * 0.14, w * 0.036, INK, 'Regular', 'LEFT')
    s.text('up to 10 min after events end.', w * 0.07, h * 0.105, w * 0.036, INK, 'Regular', 'LEFT')
    s.rect(w * 0.05, h * 0.07, w * 0.9, h * 0.003, 0x9a9da2)
    s.text('ROUTE 14  ·  EMBERCREST STOP', w * 0.07, h * 0.035, w * 0.03, 0x6a6d72, 'Bold', 'LEFT', tracking=1.2)


def panel_badge(s, w, h):
    s.rect(0, 0, w, h, TRANSIT_GREEN)
    s.rect(0, h * 0.82, w, h * 0.02, TRANSIT_GOLD)
    s.text('CITY TRANSIT', w * 0.5, h * 0.88, w * 0.12, 0xffffff, 'Black', 'CENTER')
    s.circle(w * 0.5, h * 0.55, w * 0.3, 0xffffff, n=96)
    s.text('14', w * 0.5, h * 0.45, w * 0.32, TRANSIT_GREEN, 'Black', 'CENTER')
    s.text('EMBERCREST', w * 0.5, h * 0.16, w * 0.11, 0xffffff, 'Bold', 'CENTER')
    s.text('RESORT', w * 0.5, h * 0.07, w * 0.09, 0xffffff, 'Regular', 'CENTER', tracking=1.3)


def panel_stop_blade(s, w, h):
    s.rect(0, 0, w, h, TRANSIT_WHITE)
    s.rect(0, h * 0.8, w, h * 0.2, TRANSIT_GREEN)
    s.rect(0, h * 0.78, w, h * 0.018, TRANSIT_GOLD)
    s.text('CITY TRANSIT', w * 0.5, h * 0.86, w * 0.105, 0xffffff, 'Black', 'CENTER', tracking=1.05)
    # the 14 route badge and a bus pictogram
    s.circle(w * 0.3, h * 0.6, w * 0.15, TRANSIT_GREEN, n=96)
    s.text('14', w * 0.3, h * 0.555, w * 0.16, 0xffffff, 'Black', 'CENTER')
    bx, by, bw, bh = w * 0.53, h * 0.53, w * 0.36, h * 0.14
    s.rect(bx, by + bh * 0.15, bw, bh * 0.85, INK)
    s.rect(bx + bw * 0.06, by + bh * 0.52, bw * 0.88, bh * 0.3, TRANSIT_WHITE)
    for k in range(4):
        s.rect(bx + bw * (0.08 + 0.22 * k), by + bh * 0.55, bw * 0.18, bh * 0.24, INK)
    s.circle(bx + bw * 0.22, by + bh * 0.15, bh * 0.15, INK, n=32)
    s.circle(bx + bw * 0.78, by + bh * 0.15, bh * 0.15, INK, n=32)
    s.text('EMBERCREST', w * 0.5, h * 0.36, w * 0.115, INK, 'Black', 'CENTER')
    s.text('RESORT', w * 0.5, h * 0.29, w * 0.085, INK, 'Bold', 'CENTER', tracking=1.4)
    s.rect(0, 0, w, h * 0.2, brand.NAVY)
    s.circle(w * 0.25, h * 0.1, h * 0.06, 0xf0e6c8, n=48)
    s.circle(w * 0.272, h * 0.112, h * 0.055, brand.NAVY, n=48)
    s.text('OWL', w * 0.58, h * 0.055, w * 0.13, 0xffffff, 'Black', 'CENTER', tracking=1.2)


def panel_mini_timetable(s, w, h):
    s.rect(0, 0, w, h, 0xf4f3ef)
    s.rect(0, h * 0.82, w, h * 0.18, TRANSIT_GREEN)
    s.text('14  TO NORTHSIDE', w * 0.5, h * 0.87, w * 0.1, 0xffffff, 'Black', 'CENTER')
    s.text('Daily :15 and :45', w * 0.5, h * 0.7, w * 0.085, INK, 'Bold', 'CENTER')
    s.text('6:15 AM – 11:15 PM', w * 0.5, h * 0.6, w * 0.075, INK, 'Regular', 'CENTER')
    s.rect(w * 0.08, h * 0.32, w * 0.84, h * 0.2, brand.NAVY)
    s.text('OWL  12:15  1:15', w * 0.5, h * 0.43, w * 0.07, 0xffffff, 'Bold', 'CENTER')
    s.text('2:15  3:15 AM', w * 0.5, h * 0.36, w * 0.07, brand.GOLD, 'Bold', 'CENTER')
    s.text('Fare $2.90', w * 0.5, h * 0.16, w * 0.08, INK, 'Bold', 'CENTER')


# ================================================================== SM_ECX_BusShelter (+ _GlowCool)

SH_MAP = (0.0, 0.0, 0.42, 0.63)
SH_AD = (0.42, 0.0, 0.84, 0.63)
SH_TT_IN = (0.0, 0.64, 0.246, 1.0)
SH_TT_OUT = (0.25, 0.64, 0.496, 1.0)
SH_BADGE = (0.50, 0.64, 0.62, 0.82)
SH_HEAT = (0.63, 0.84, 0.99, 0.905)


def panel_heater(s, w, h):
    """A radiant heater's underside: a dark reflector, two quartz tubes glowing ember, end caps, a wire guard."""
    s.rect(0, 0, w, h, 0x121110)
    for yc in (h * 0.32, h * 0.68):
        s.rect(w * 0.05, yc - h * 0.13, w * 0.9, h * 0.26, 0x4a1c08)
        s.rect(w * 0.05, yc - h * 0.075, w * 0.9, h * 0.15, 0xc8461a)
        s.rect(w * 0.05, yc - h * 0.035, w * 0.9, h * 0.07, 0xff9a52)
        s.rect(w * 0.05, yc - h * 0.012, w * 0.9, h * 0.024, 0xffd9a8)
    for x0 in (0.0, w * 0.95):
        s.rect(x0, 0, w * 0.05, h, 0x2e2c2a)
    for k in range(1, 24):
        s.rect(w * k / 24.0 - w * 0.002, 0, w * 0.004, h, 0x0c0c0c)
    s.rect(0, h * 0.49, w, h * 0.02, 0x0c0c0c)


def build_shelter():
    at = Atlas('ecx_shelter_prints', 2048)
    at.put(SH_MAP, panel_route_map, 1200.0, 1800.0)
    at.put(SH_AD, panel_ad, 1200.0, 1800.0)
    at.put(SH_TT_IN, panel_timetable, 800.0, 1200.0)
    at.put(SH_TT_OUT, panel_timetable, 800.0, 1200.0)
    at.put(SH_BADGE, panel_badge, 400.0, 600.0)
    at.put(SH_HEAT, panel_heater, 940.0, 180.0)
    path = at.save()
    st = steel_black('ecxh_steel', 0.0)
    tim = timber('ecxh_timber', axis='Y', board=0.09, cross='X')
    gla = street.glass('ecxh_glass', 0x0d1215, 0.03)
    prn = print_material('ecxh_prints', path, lit_rects=(SH_MAP, SH_AD, SH_HEAT), strength=12.0)
    sn = snow('ecxh_snow')
    con = concrete('ecxh_concrete', 0x9a968e)
    out, lit = [], []
    out.append(box(-0.9, 0.9, -4.0, 4.0, 0.0, 0.025, con, 0.008, 2))
    out.append(box(0.56, 0.88, -3.95, 3.95, 0.025, 0.14, con, 0.01, 2))
    posts = (-3.9, -1.3, 1.3, 3.9)
    for y in posts:
        out.append(box(0.67, 0.77, y - 0.05, y + 0.05, 0.025, 2.62, st, 0.006, 2))
        # the base plate, held down by four anchor nuts on studs
        out.append(box(0.63, 0.81, y - 0.09, y + 0.09, 0.14, 0.152, st, 0.003, 1))
        for bx in (0.65, 0.79):
            for by in (y - 0.07, y + 0.07):
                out.append(lathe('nut', [(0.0, 0.152), (0.011, 0.152), (0.011, 0.162), (0.009, 0.165), (0.0, 0.165)], st, 6,
                                 Matrix.Translation((bx, by, 0.0))))
                out.append(lathe('stud', [(0.0, 0.165), (0.006, 0.165), (0.006, 0.172), (0.004, 0.175), (0.0, 0.175)], st, 12,
                                 Matrix.Translation((bx, by, 0.0))))
        out.append(prism_y('beam', [(0.80, 2.58), (0.80, 2.78), (-0.86, 2.78), (-0.86, 2.70)], y - 0.04, y + 0.04, st))
        for bz in (0.36,):
            out.append(box(0.25, 0.67, y - 0.03, y + 0.03, bz, 0.42, st, 0.004, 1))
    out.append(box(-0.88, 0.88, -4.03, 4.03, 2.78, 2.86, st, 0.01, 2))
    out.append(box(-0.90, -0.86, -4.05, 4.05, 2.66, 2.88, st, 0.006, 3))
    out.append(box(0.86, 0.90, -4.05, 4.05, 2.70, 2.88, st, 0.006, 3))
    for sy in (-1, 1):
        out.append(box(-0.86, 0.86, sy * 4.03 - 0.02, sy * 4.03 + 0.02, 2.70, 2.88, st, 0.004, 2))
    # the larch soffit, a steel channel for the LED down its middle
    for k in range(20):
        x = -0.855 + 0.09 * k
        if abs(x) < 0.05:
            continue
        out.append(box(x - 0.0325, x + 0.0325, -3.98, 3.98, 2.735, 2.765, tim, 0.004, 2))
    out.append(box(-0.05, 0.05, -3.6, 3.6, 2.74, 2.78, st, 0.003, 1))
    lit.append(box(-0.04, 0.04, -3.5, 3.5, 2.734, 2.742, None))
    # glass in the side bays, in steel glazing channels
    for y0, y1 in ((-3.85, -1.35), (1.35, 3.85)):
        out.append(box(0.714, 0.726, y0, y1, 0.17, 2.50, gla, 0.0, 1))
        for z0, z1 in ((0.14, 0.17), (2.50, 2.53)):
            out.append(box(0.70, 0.74, y0 - 0.05, y1 + 0.05, z0, z1, st, 0.003, 1))
    # the lightbox in the middle bay
    out.append(box(0.66, 0.80, -1.25, 1.25, 0.55, 2.53, st, 0.008, 2))
    out.append(print_quad('ad', [(0.657, 0.6, 0.65), (0.657, -0.6, 0.65), (0.657, -0.6, 2.45), (0.657, 0.6, 2.45)], SH_AD,
                          (0, -1, 0), (0, 0, 1), (0.657, 0.6, 0.65), (1.2, 1.8), prn))
    # its bezel, 2.5 cm proud of the lightbox
    for (y0, y1, z0, z1) in ((-0.64, 0.64, 2.45, 2.49), (-0.64, 0.64, 0.61, 0.65), (-0.64, -0.6, 0.65, 2.45), (0.6, 0.64, 0.65, 2.45)):
        out.append(box(0.632, 0.661, y0, y1, z0, z1, st, 0.004, 2))
    # two radiant heaters hung under the soffit over the bench halves (their quartz tubes glow in the prints slot)
    for c in (-1.9, 1.9):
        out.append(box(0.30, 0.52, c - 0.5, c + 0.5, 2.632, 2.712, st, 0.012, 2))
        for (x0, x1, y0, y1) in ((0.30, 0.52, c - 0.5, c - 0.47), (0.30, 0.52, c + 0.47, c + 0.5), (0.30, 0.32, c - 0.47, c + 0.47),
                                 (0.50, 0.52, c - 0.47, c + 0.47)):
            out.append(box(x0, x1, y0, y1, 2.620, 2.633, st, 0.002, 1))
        for yy in (c - 0.38, c + 0.38):
            out.append(rod((0.41, yy, 2.71), (0.41, yy, 2.736), 0.008, st, 12))
        out.append(print_quad('heat', [(0.32, c - 0.47, 2.629), (0.32, c + 0.47, 2.629), (0.50, c + 0.47, 2.629), (0.50, c - 0.47, 2.629)],
                              SH_HEAT, (0, 1, 0), (1, 0, 0), (0.32, c - 0.47, 2.629), (0.94, 0.18), prn))
    # the bench: three larch boards on the brackets
    for k in range(3):
        x0 = 0.25 + k * 0.137
        out.append(box(x0, x0 + 0.13, -3.75, 3.75, 0.42, 0.46, tim, 0.006, 2))
    # south end: the route-map case (printed inside, a route badge outside)
    out.append(box(-0.52, 0.84, -4.02, -3.88, 0.06, 2.40, st, 0.01, 2))
    out.append(print_quad('map', [(0.75, -3.877, 0.45), (-0.45, -3.877, 0.45), (-0.45, -3.877, 2.25), (0.75, -3.877, 2.25)], SH_MAP,
                          (-1, 0, 0), (0, 0, 1), (0.75, -3.877, 0.45), (1.2, 1.8), prn))
    # the case's bezel, 3 cm proud
    for (x0, x1, z0, z1) in ((-0.49, 0.79, 2.25, 2.29), (-0.49, 0.79, 0.41, 0.45), (-0.49, -0.45, 0.45, 2.25), (0.75, 0.79, 0.45, 2.25)):
        out.append(box(x0, x1, -3.881, -3.85, z0, z1, st, 0.004, 2))
    # a downpipe from the roof's back fascia down the back of a middle post, in three brackets, to a shoe on the plinth
    py = 1.3
    out.append(tube([(0.885, py, 2.72), (0.87, py, 2.64), (0.83, py, 2.56), (0.80, py, 2.48)], 0.028, st, 16, 6))
    out.append(rod((0.80, py, 2.48), (0.80, py, 0.26), 0.028, st, 16))
    out.append(tube([(0.80, py, 0.26), (0.80, py, 0.2), (0.825, py, 0.17), (0.85, py, 0.16)], 0.028, st, 16, 6))
    for bz in (0.6, 1.4, 2.2):
        out.append(box(0.77, 0.832, py - 0.034, py + 0.034, bz - 0.02, bz + 0.02, st, 0.004, 1))
    out.append(print_quad('badge', [(0.1, -4.023, 1.6), (0.5, -4.023, 1.6), (0.5, -4.023, 2.2), (0.1, -4.023, 2.2)], SH_BADGE,
                          (1, 0, 0), (0, 0, 1), (0.1, -4.023, 1.6), (0.4, 0.6), prn))
    # north end: the timetable, printed both sides
    out.append(box(-0.25, 0.84, 3.90, 3.98, 0.85, 2.30, st, 0.006, 2))
    out.append(box(-0.28, -0.22, 3.91, 3.97, 0.025, 0.85, st, 0.004, 1))
    out.append(print_quad('tt_in', [(-0.2, 3.897, 0.95), (0.6, 3.897, 0.95), (0.6, 3.897, 2.15), (-0.2, 3.897, 2.15)], SH_TT_IN,
                          (1, 0, 0), (0, 0, 1), (-0.2, 3.897, 0.95), (0.8, 1.2), prn))
    out.append(print_quad('tt_out', [(0.6, 3.983, 0.95), (-0.2, 3.983, 0.95), (-0.2, 3.983, 2.15), (0.6, 3.983, 2.15)], SH_TT_OUT,
                          (-1, 0, 0), (0, 0, 1), (0.6, 3.983, 0.95), (0.8, 1.2), prn))
    # snow on the roof, a soft lip over the front edge
    sc = snow_cap(-0.9, 0.9, -4.05, 4.05, lambda x, y: 2.88 + 0.0 * x, 0.045, 0.12, 41, 0.022)
    core.assign(sc, sn)
    out.append(sc)
    obj = finish_mesh(out, 'SM_ECX_BusShelter', [st, tim, gla, prn, sn, con], 30.0)
    return [obj, glow(lit, 'SM_ECX_BusShelter_GlowCool')]


# ================================================================== SM_ECX_BusStopSign

ST_FRONT = (0.0, 0.0, 0.5, 0.667)
ST_BACK = (0.5, 0.0, 1.0, 0.667)
ST_HOLDER = (0.0, 0.68, 0.24, 1.0)


def build_stopsign():
    at = Atlas('ecx_stop_prints', 1024)
    at.put(ST_FRONT, panel_stop_blade, 600.0, 800.0)
    at.put(ST_BACK, panel_stop_blade, 600.0, 800.0)
    at.put(ST_HOLDER, panel_mini_timetable, 250.0, 333.0)
    path = at.save()
    st = steel_black('ecxt_steel', 0.0)
    prn = print_material('ecxt_prints', path, rough=0.3)
    out = []
    out.append(lathe('pole', [(0.0, 0.0), (0.038, 0.0), (0.038, 3.17), (0.034, 3.19), (0.026, 3.2), (0.0, 3.205)], st, 64))
    out.append(lathe('collar', [(0.0, 0.0), (0.055, 0.0), (0.055, 0.02), (0.05, 0.035), (0.038, 0.06), (0.0, 0.06)], st, 64))
    # the blade: an aluminium panel with rolled edges, clamped to the pole. It hangs off the pole's -y side, which
    # the spec's yaw 90 turns over the platform (map -X): on the +y side it reached 26 cm over the bay at 2.35-3.15 m,
    # where a bus pulling out of the stop (its body to 3.05 m over the platform, 10 cm inside that) would cut through it.
    out.append(box(-0.003, 0.003, -0.66, -0.06, 2.35, 3.15, st, 0.0025, 2))
    for z in (2.45, 3.05):
        out.append(lathe('clamp', [(0.0, z - 0.025), (0.042, z - 0.025), (0.044, z - 0.02), (0.044, z + 0.02), (0.042, z + 0.025), (0.0, z + 0.025)], st, 48))
        for yb in (-0.045, -0.06):
            out.append(parts.disc('bolt', (-0.013, yb, z), (1, 0, 0), 0.005, 0.004, 12))
            core.assign(out[-1], st)
        out.append(box(-0.012, 0.012, -0.075, -0.03, z - 0.02, z + 0.02, st, 0.003, 1))
    # front (-X) reads toward -y: from the pole outward; back (+X) reads toward +y: toward the pole
    out.append(print_quad('blade_f', [(-0.0055, -0.06, 2.35), (-0.0055, -0.66, 2.35), (-0.0055, -0.66, 3.15), (-0.0055, -0.06, 3.15)], ST_FRONT,
                          (0, -1, 0), (0, 0, 1), (-0.0055, -0.06, 2.35), (0.6, 0.8), prn))
    out.append(print_quad('blade_b', [(0.0055, -0.66, 2.35), (0.0055, -0.06, 2.35), (0.0055, -0.06, 3.15), (0.0055, -0.66, 3.15)], ST_BACK,
                          (0, 1, 0), (0, 0, 1), (0.0055, -0.66, 2.35), (0.6, 0.8), prn))
    # the timetable holder on the pole's front
    out.append(box(-0.072, -0.04, -0.13, 0.13, 1.33, 1.71, st, 0.006, 2))
    out.append(box(-0.045, -0.035, -0.02, 0.02, 1.4, 1.64, st, 0.002, 1))
    out.append(print_quad('holder', [(-0.0745, 0.125, 1.345), (-0.0745, -0.125, 1.345), (-0.0745, -0.125, 1.695), (-0.0745, 0.125, 1.695)], ST_HOLDER,
                          (0, -1, 0), (0, 0, 1), (-0.0745, 0.125, 1.345), (0.25, 0.35), prn))
    return [finish_mesh(out, 'SM_ECX_BusStopSign', [st, prn], 35.0)]


# ================================================================== small-prop materials

def leather(name, hex_color, worn=0.4):
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    vor = m.node('ShaderNodeTexVoronoi', Scale=700.0)
    m.link(o, vor.inputs['Vector'])
    peb = vor.outputs['Distance']
    mott = noise(m, o, 12.0, 4.0)
    col = m.mix(m.math('MULTIPLY', ramp(m, mott, 0.3, 0.8), 0.4), H(hex_color), cmul(m, H(hex_color), 0.7))
    ed = m.math('MULTIPLY', convex_edges(m, tc, distance=0.004, samples=10, breakup_scale=40.0, gain=4.0), worn)
    col = m.mix(ed, col, cmul(m, H(hex_color), 1.6))
    m.set('Base Color', col)
    m.set('Roughness', m.math('ADD', 0.42, m.math('MULTIPLY', peb, 0.25)))
    m.set('Sheen Weight', 0.25)
    m.set('Specular IOR Level', 0.4)
    set_bump(m, peb, 0.25, 0.0005)
    return m


def brass(name, rough=0.18):
    """Polished brass that hands keep bright and the weather tarnishes in the corners."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    mott = noise(m, o, 9.0, 4.0)
    prints = ramp(m, noise(m, o, 35.0, 3.0), 0.55, 0.75)
    col = m.mix(ramp(m, mott, 0.3, 0.7), H(0xc6a056), H(0xad8640))
    cav = ao_cavity(m, 0.02, 1.6)
    col = m.mix(cav, col, H(0x4e3f22))
    m.set('Base Color', col)
    m.set('Metallic', m.math('SUBTRACT', 1.0, m.math('MULTIPLY', cav, 0.4)))
    m.set('Roughness', m.math('ADD', rough, m.math('ADD', m.math('MULTIPLY', prints, 0.12), m.math('MULTIPLY', cav, 0.3))))
    return m


def rubber(name, hex_color=0x141414):
    return parts.rubber(name, hex_color, 0.8)


def carpet_png():
    """The card room's carpet (M_CardRoom pattern 8) at a 0.3 m tile: an ember four-lobed star ringed in gold,
    steel-blue scallops from the corners, a faint ember lattice, confetti, on navy cut pile."""
    n = 512
    c = (np.arange(n) + 0.5) / n
    U, V = np.meshgrid(c, c)
    f = np.stack([U - 0.5, V - 0.5], 2)
    r = np.linalg.norm(f, axis=2)
    ang = np.arctan2(f[..., 1], f[..., 0])
    lobe = 0.065 + 0.035 * np.cos(ang * 4.0)
    star = 1.0 - smoothstep(-0.006, 0.006, r - lobe)
    halo = 1.0 - smoothstep(0.004, 0.012, np.abs(r - lobe - 0.04))
    scallop = np.zeros_like(r)
    for qx in (-0.5, 0.5):
        for qy in (-0.5, 0.5):
            rc = np.hypot(f[..., 0] - qx, f[..., 1] - qy)
            scallop = np.maximum(scallop, 1.0 - smoothstep(0.008, 0.02, np.abs(rc - 0.36)))
            scallop = np.maximum(scallop, (1.0 - smoothstep(0.006, 0.014, np.abs(rc - 0.30))) * 0.7)
    g = np.abs(np.mod(np.stack([U, V], 2) * 2.0 + 0.25, 1.0) - 0.5)
    lattice = 1.0 - smoothstep(0.006, 0.018, np.abs(g[..., 0] - g[..., 1]))
    col = np.broadcast_to(lin(0x131d34), (n, n, 3)).copy()
    col = col * (1 - 0.6 * lattice[..., None]) + lin(0x9a3a14) * 0.6 * lattice[..., None]
    col = col * (1 - scallop[..., None]) + lin(0x3a5378) * scallop[..., None]
    col = col * (1 - halo[..., None]) + lin(0xc89a48) * halo[..., None]
    col = col * (1 - star[..., None]) + lin(0xd9531e) * star[..., None]
    pile = tnoise(n, 120, 7, 2)
    col = col * (0.82 + 0.3 * pile[..., None])
    path = os.path.join(core.BUILD_DIR, 'textures', 'ecx_cart_carpet.png')
    _png(path, lin2srgb(col))
    return path


def carpet(name):
    m = core.Mat(name)
    tc, sep = object_coords(m)
    mp = m.node('ShaderNodeMapping')
    mp.inputs['Scale'].default_value = (1 / 0.3, 1 / 0.3, 1.0)
    m.link(tc.outputs['Object'], mp.inputs['Vector'])
    img = m.image(carpet_png(), vector=mp.outputs['Vector'])
    fz = noise(m, tc.outputs['Object'], 2200.0, 2.0)
    m.set('Base Color', img.outputs['Color'])
    m.set('Roughness', 0.95)
    m.set('Sheen Weight', 0.6)
    m.set('Specular IOR Level', 0.2)
    set_bump(m, fz, 0.3, 0.0008)
    return m


def luggage(name):
    """The cart's load (ecx_kind): 1 an ember polycarbonate case, 2 a navy one, 3 brown leather, 4 black nylon,
    5 black plastic and rubber (handles, wheels, corners)."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    k = attr(m, 'ecx_kind')
    scuff = ramp(m, noise(m, o, 40.0, 4.0), 0.6, 0.78)
    weave = noise(m, scaled(m, sep, 1600.0, 1600.0, 300.0), 1.0, 2.0)
    mott = noise(m, o, 10.0, 4.0)
    col = H(0x1d2947)
    rough = 0.22
    for kk, c, r in ((1, 0xd9531e, 0.22), (2, 0x1d2947, 0.24), (3, 0x5a3520, 0.5), (4, 0x16171a, 0.72), (5, 0x111214, 0.6)):
        msk = is_k(m, k, kk)
        col = m.mix(msk, col, H(c))
        rough = mix_float(m, msk, rough, r)
    lea = is_k(m, k, 3)
    col = m.mix(m.math('MULTIPLY', lea, m.math('MULTIPLY', ramp(m, mott, 0.3, 0.8), 0.5)), col, H(0x3a2010))
    col = m.mix(m.math('MULTIPLY', scuff, 0.35), col, cmul(m, col, 1.4))
    m.set('Base Color', col)
    m.set('Roughness', m.math('ADD', rough, m.math('MULTIPLY', scuff, 0.15)))
    m.set('Coat Weight', mix_float(m, m.math('ADD', is_k(m, k, 1), is_k(m, k, 2)), 0.0, 0.6))
    set_bump(m, m.math('ADD', m.math('MULTIPLY', weave, is_k(m, k, 4)), m.math('MULTIPLY', mott, 0.2)), 0.2, 0.0006)
    return m


def sand(name):
    """Ash-urn sand (ecx_kind 0), cigarette filters (1) and their paper (2)."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    k = attr(m, 'ecx_kind')
    gr = noise(m, o, 900.0, 2.0)
    col = m.mix(ramp(m, gr, 0.3, 0.7), H(0xa69a83), H(0xc8bea8))
    col = m.mix(m.math('MULTIPLY', ramp(m, noise(m, o, 60.0, 2.0), 0.6, 0.75), 0.6), col, H(0x5a5248))
    col = m.mix(is_k(m, k, 1), col, m.mix(ramp(m, noise(m, o, 2000.0, 1.0), 0.5, 0.7), H(0xc9874a), H(0xa86a34)))
    col = m.mix(is_k(m, k, 2), col, H(0xe6e1d6))
    m.set('Base Color', col)
    m.set('Roughness', 0.85)
    set_bump(m, gr, 0.4, 0.0006)
    return m


# ================================================================== SM_ECX_LampPost (+ _GlowWarm)

def medallion(x_face, z, r, front, mats):
    """A small bronze disc with the mark in relief, on a face of the pole."""
    brz = mats
    out = []
    sgn = -1.0 if front else 1.0
    d = parts.disc('med', (x_face + sgn * 0.0125, 0.0, z), (1, 0, 0), r, 0.025, 48)
    core.assign(d, brz)
    out.append(d)
    ms = mark_shapes(0.0, z - 0.005, r * 1.35)
    fx = x_face + sgn * 0.025
    out.append(shape('crest', ms['crest'], 0.0, 0.004, brz, front, fx))
    for f in ms['shade']:
        out.append(tag(shape('shade', f, 0.004, 0.0048, brz, front, fx), 1))
    out.append(tag(shape('snowf', ms['snow'], 0.004, 0.005, brz, front, fx), 2))
    ring = shape('ring', circle_poly((0.0, z), r * 0.96, 48), 0.0, 0.003, brz, front, fx, holes=[list(reversed(circle_poly((0.0, z), r * 0.88, 48)))])
    out.append(tag(ring, 2))
    return out


def build_lamppost():
    brz = bronze('ecxl_bronze', scuff=0.5, patina=0.6, edge=0.004, darker=0.25, facets=True)
    bas = basalt('ecxl_basalt', ground=0.0, salt=0.7, streaks=0.5, wet_foot=0.5)
    frm = frame_dark('ecxl_frame')
    out, lit = [], []
    out.append(box(-0.2, 0.2, -0.2, 0.2, 0.0, 0.9, bas, 0.012, 5))
    out.append(box(-0.19, 0.19, -0.19, 0.19, 0.035, 0.05, bas, 0.0, 1))
    out.append(box(-0.202, -0.199, -0.06, 0.06, 0.25, 0.45, brz, 0.002, 1))
    out.append(lathe('collar', [(0.0, 0.9), (0.1, 0.9), (0.1, 0.93), (0.075, 0.96), (0.068, 1.0), (0.0, 1.0)], brz, 96))
    out.append(lathe('pole', [(0.0, 0.99), (0.06, 0.99), (0.058, 1.5), (0.05, 3.2), (0.045, 4.15), (0.0, 4.15)], brz, 96))
    out.append(lathe('ring', [(0.0, 1.55), (0.064, 1.55), (0.066, 1.58), (0.064, 1.61), (0.0, 1.61)], brz, 96))
    out.append(lathe('cap', [(0.0, 4.12), (0.06, 4.12), (0.09, 4.17), (0.12, 4.2), (0.0, 4.2)], brz, 96))
    for front in (True, False):
        out += medallion(-0.052 if front else 0.052, 2.75, 0.1, front, brz)
    # the lantern: a tray, four corner posts with rails, the panes (glow), a cap, a hipped roof and a finial
    out.append(box(-0.23, 0.23, -0.23, 0.23, 4.2, 4.28, brz, 0.008, 4))
    out.append(box(-0.15, 0.15, -0.15, 0.15, 4.195, 4.205, frm, 0.002, 1))
    for sx in (-1, 1):
        for sy in (-1, 1):
            out.append(box(sx * 0.215 - 0.016, sx * 0.215 + 0.016, sy * 0.215 - 0.016, sy * 0.215 + 0.016, 4.28, 4.88, frm, 0.004, 3))
    for z0, z1 in ((4.28, 4.30), (4.85, 4.88)):
        for s in (-1, 1):
            out.append(box(s * 0.21 - 0.012, s * 0.21 + 0.012, -0.2, 0.2, z0, z1, frm, 0.002, 1))
            out.append(box(-0.2, 0.2, s * 0.21 - 0.012, s * 0.21 + 0.012, z0, z1, frm, 0.002, 1))
    for s in (-1, 1):
        out.append(box(s * 0.206 - 0.004, s * 0.206 + 0.004, -0.004, 0.004, 4.30, 4.85, frm, 0.001, 1))
        out.append(box(-0.004, 0.004, s * 0.206 - 0.004, s * 0.206 + 0.004, 4.30, 4.85, frm, 0.001, 1))
    out.append(box(-0.25, 0.25, -0.25, 0.25, 4.88, 4.92, brz, 0.008, 4))
    roof = core.loft('roof', [[(-0.25, -0.25, 4.92), (0.25, -0.25, 4.92), (0.25, 0.25, 4.92), (-0.25, 0.25, 4.92)],
                              [(-0.06, -0.06, 5.08), (0.06, -0.06, 5.08), (0.06, 0.06, 5.08), (-0.06, 0.06, 5.08)]])
    core.assign(roof, brz)
    out.append(roof)
    out.append(lathe('finial', [(0.0, 5.075), (0.03, 5.075), (0.02, 5.11), (0.018, 5.13), (0.032, 5.155), (0.028, 5.18), (0.012, 5.198), (0.0, 5.2)], brz, 32))
    for sx, sy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
        x0, x1 = (-0.198, 0.198) if sx == 0 else (sx * 0.198 - 0.001, sx * 0.198 + 0.001)
        y0, y1 = (-0.198, 0.198) if sy == 0 else (sy * 0.198 - 0.001, sy * 0.198 + 0.001)
        lit.append(box(min(x0, x1), max(x0, x1), min(y0, y1), max(y0, y1), 4.30, 4.85, None))
    return [finish_mesh(out, 'SM_ECX_LampPost', [brz, bas, frm], 35.0), glow(lit, 'SM_ECX_LampPost_GlowWarm')]


# ================================================================== SM_ECX_Bollard (+ _GlowEmber)

def build_bollard():
    brz = bronze('ecxb_bronze', scuff=0.9, patina=0.5, edge=0.003)
    frm = core.Mat('ecxb_frame')
    tc, sep = object_coords(frm)
    frost = ramp(frm, sep.outputs['Z'], 0.925, 0.935)
    frm.set('Base Color', frm.mix(frost, H(0x15161a), H(0xcfd2d4)))
    frm.set('Metallic', mix_float(frm, frost, 0.85, 0.0))
    frm.set('Roughness', mix_float(frm, frost, 0.45, 0.55))
    out, lit = [], []
    out.append(lathe('base', [(0.0, 0.0), (0.11, 0.0), (0.11, 0.02), (0.106, 0.035), (0.105, 0.05), (0.105, 0.815), (0.0, 0.815)], brz, 64))
    out.append(lathe('groove', [(0.0, 0.815), (0.105, 0.815), (0.096, 0.82), (0.096, 0.84), (0.105, 0.845), (0.0, 0.845)], frm, 64))
    out.append(lathe('upper', [(0.0, 0.845), (0.105, 0.845), (0.105, 0.9), (0.0, 0.9)], brz, 64))
    out.append(lathe('cap', [(0.0, 0.9), (0.106, 0.9), (0.106, 0.925), (0.098, 0.93), (0.09, 0.935), (0.07, 0.944), (0.04, 0.949), (0.0, 0.95)], frm, 64))
    ring = lathe('ring', [(0.0975, 0.822), (0.0975, 0.838)], None, 64)
    core.orient_normals(ring)
    bm = bmesh.new()
    bm.from_mesh(ring.data)
    bm.normal_update()
    if sum(f.normal.dot(Vector((f.calc_center_median().x, f.calc_center_median().y, 0))) for f in bm.faces) < 0:
        bmesh.ops.reverse_faces(bm, faces=bm.faces[:])
    bm.to_mesh(ring.data)
    bm.free()
    lit.append(ring)
    return [finish_mesh(out, 'SM_ECX_Bollard', [brz, frm], 35.0), glow(lit, 'SM_ECX_Bollard_GlowEmber')]


# ================================================================== SM_ECX_Planter

def build_planter():
    bas = basalt('ecxp_basalt', ground=0.0, salt=0.7, streaks=0.6, wet_foot=0.5)
    brz = bronze('ecxp_bronze', scuff=0.5, patina=0.6)
    sn = snow('ecxp_snow')
    so = soil('ecxp_soil')
    out = []
    a, b = 1.2, 1.02
    out.append(box(-a, a, -a, -b, 0.04, 0.9, bas, 0.01, 2))
    out.append(box(-a, a, b, a, 0.04, 0.9, bas, 0.01, 2))
    out.append(box(-a, -b, -b, b, 0.04, 0.9, bas, 0.01, 2))
    out.append(box(b, a, -b, b, 0.04, 0.9, bas, 0.01, 2))
    out.append(box(-a + 0.02, a - 0.02, -a + 0.02, a - 0.02, 0.0, 0.045, bas, 0.004, 1))
    for (x0, x1, y0, y1) in ((-a - 0.012, a + 0.012, -a - 0.012, -b + 0.012), (-a - 0.012, a + 0.012, b - 0.012, a + 0.012),
                             (-a - 0.012, -b + 0.012, -b + 0.012, b - 0.012), (b - 0.012, a + 0.012, -b + 0.012, b - 0.012)):
        out.append(box(x0, x1, y0, y1, 0.9, 0.95, brz, 0.005, 2))
    # The soil only shows in a ragged ring round the trunk (the tree is placed at the center, Z + 0.75).
    soil_o = grid_mesh('soil', list(np.linspace(-0.36, 0.36, 19)), list(np.linspace(-0.36, 0.36, 19)),
                       lambda x, y: 0.75 + 0.012 * float(vnoise2(np.array([x]), np.array([y]), 0.12, 5, 2)[0]), so)
    out.append(soil_o)

    # The snow is heaped in the box (what was shovelled off the plaza): a wind-smoothed loaf about 8 cm proud of the
    # cap, sloping down to meet the walls just under the cap's inner lip (so no edge shows); a tree well round the
    # trunk, where a ring of soil shows through. The cap stays clear (the staff brush it).
    def snow_h(x, y):
        r = math.hypot(x, y)
        edge = max(abs(x), abs(y))
        n = float(vnoise2(np.array([x]), np.array([y]), 0.55, 9, 3)[0])
        fine = float(vnoise2(np.array([x * 1.3]), np.array([y]), 0.14, 10, 2)[0])
        loaf = (0.12 + 0.03 * n) * (1.0 - float(smoothstep(0.45, 0.97, edge / b)))
        rag = 0.04 * (float(vnoise2(np.array([math.atan2(y, x) * 0.6]), np.array([r]), 0.08, 12, 2)[0]) - 0.5)
        well = float(smoothstep(0.075 + rag, 0.42 + rag, r))
        z = 0.896 + loaf + 0.008 * (n - 0.5) + 0.004 * (fine - 0.5)
        return 0.735 + (z - 0.735) * well
    k = b - 0.002
    xs = list(np.linspace(-k, k, 49))
    sm = grid_mesh('snow', xs, xs, snow_h, sn)
    out.append(sm)
    return [finish_mesh(out, 'SM_ECX_Planter', [bas, brz, sn, so], 35.0)]


# ================================================================== SM_ECX_Bench

def build_bench():
    tim = timber('ecxn_timber', axis='Y', board=0.1375, cross='X', weather=0.5)
    bas = basalt('ecxn_basalt', ground=0.0, salt=0.6, streaks=0.3, wet_foot=0.6)
    brz = bronze('ecxn_bronze', scuff=0.9, patina=0.4, edge=0.003)
    out = []
    for sy in (-1, 1):
        out.append(box(-0.22, 0.22, min(sy * 0.52, sy * 0.88), max(sy * 0.52, sy * 0.88), 0.0, 0.37, bas, 0.012, 5))
    for k in range(4):
        x0 = -0.275 + k * 0.1375
        out.append(box(x0 + 0.003, x0 + 0.1375 - 0.003, -1.094, 1.094, 0.37, 0.452, tim, 0.006, 5))
    for sy in (-1, 1):
        out.append(box(-0.278, 0.278, sy * 1.094 - (0.0 if sy > 0 else 0.008), sy * 1.094 + (0.008 if sy > 0 else 0.0), 0.365, 0.458, brz, 0.003, 2))
    for sx in (-1, 1):
        out.append(box(sx * 0.276 - 0.004, sx * 0.276 + 0.004, -1.1, 1.1, 0.44, 0.46, brz, 0.002, 2))
    return [finish_mesh(out, 'SM_ECX_Bench', [tim, bas, brz], 35.0)]


# ================================================================== SM_ECX_AshUrn

def build_ashurn():
    brz = bronze('ecxu_bronze', scuff=0.7, patina=0.6, edge=0.003)
    sd = sand('ecxu_sand')
    out = []
    out.append(lathe('urn', [(0.0, 0.0), (0.185, 0.0), (0.19, 0.012), (0.19, 0.05), (0.186, 0.06), (0.186, 0.62), (0.19, 0.63), (0.19, 0.735),
                             (0.186, 0.748), (0.17, 0.75), (0.165, 0.74), (0.165, 0.705), (0.0, 0.705)], brz, 64))
    sgrid = grid_mesh('sand', list(np.linspace(-0.166, 0.166, 15)), list(np.linspace(-0.166, 0.166, 15)),
                      lambda x, y: 0.708 + 0.006 * float(vnoise2(np.array([x]), np.array([y]), 0.05, 4, 2)[0]), sd)
    bm = bmesh.new()
    bm.from_mesh(sgrid.data)
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if Vector((f.calc_center_median().x, f.calc_center_median().y)).length > 0.163], context='FACES')
    bm.to_mesh(sgrid.data)
    bm.free()
    out.append(tag(sgrid, 0))
    rng = np.random.default_rng(12)
    for i in range(6):
        a = rng.uniform(0, 6.28)
        r = rng.uniform(0.02, 0.12)
        c = Vector((r * math.cos(a), r * math.sin(a), 0.716))
        d = Vector((math.cos(a + rng.uniform(-1.2, 1.2)), math.sin(a + rng.uniform(-1.2, 1.2)), rng.uniform(-0.05, 0.25))).normalized()
        L = rng.uniform(0.022, 0.04)
        f = parts.rod('filter', c, c + d * 0.014, 0.0039, 12)
        core.assign(tag(f, 1), sd)
        p = parts.rod('paper', c + d * 0.014, c + d * L, 0.0038, 12)
        core.assign(tag(p, 2), sd)
        out += [f, p]
    return [finish_mesh(out, 'SM_ECX_AshUrn', [brz, sd], 35.0)]


# ================================================================== SM_ECX_ValetPodium

VP_PLAQUE = (0.0, 0.0, 1.0, 0.36)
VP_KEYS = (0.0, 0.40, 0.6, 0.79)
VP_LAMP = (0.7, 0.45, 0.9, 0.65)


def panel_plaque(s, w, h):
    s.rect(0, 0, w, h, 0x0b0b0d)
    brand.mark(s, h * 0.5, h * 0.52, h * 0.66, ground=0x0b0b0d)
    s.text('VALET', w * 0.62, h * 0.38, h * 0.42, brand.ASH, 'Black', 'CENTER', tracking=1.3)
    s.text('THE EMBERCREST', w * 0.62, h * 0.15, h * 0.12, brand.GOLD, 'Bold', 'CENTER', tracking=1.8)


def panel_keys(s, w, h):
    s.rect(0, 0, w, h, 0xf1dcb6)
    rng = random.Random(9)
    cols, rows = 8, 4
    for r in range(rows):
        for c in range(cols):
            if rng.random() < 0.22:
                continue
            cx = w * (0.08 + 0.84 * c / (cols - 1))
            cy = h * (0.82 - 0.22 * r)
            s.circle(cx, cy + h * 0.06, w * 0.006, 0x3a2a18)
            s.rect(cx - w * 0.001, cy - h * 0.02, w * 0.002, h * 0.08, 0x6a5a40)
            col = rng.choice([brand.EMBER, brand.NAVY, 0x2b5d3a, 0x6a1a14])
            s.rect(cx - w * 0.028, cy - h * 0.12, w * 0.056, h * 0.11, col)
            s.text(str(100 + r * cols + c), cx, cy - h * 0.09, w * 0.024, 0xffffff, 'Bold', 'CENTER')


def panel_lamp(s, w, h):
    s.rect(0, 0, w, h, 0xffd9a0)


def build_valet():
    at = Atlas('ecx_valet_prints', 1024)
    at.put(VP_PLAQUE, panel_plaque, 500.0, 180.0)
    at.put(VP_KEYS, panel_keys, 600.0, 400.0)
    at.put(VP_LAMP, panel_lamp, 100.0, 100.0)
    path = at.save()
    tim = timber('ecxv_timber', axis='Z', board=0.1, cross='Y')
    brz = bronze('ecxv_bronze', scuff=0.8, patina=0.5, edge=0.003)
    lea = leather('ecxv_leather', 0x1d2947)
    prn = print_material('ecxv_prints', path, lit_rects=(VP_PLAQUE, VP_KEYS, VP_LAMP), strength=10.0)
    out = []
    out.append(box(-0.27, 0.27, -0.42, 0.42, 0.0, 0.06, brz, 0.006, 3))
    out.append(box(-0.235, 0.235, -0.385, 0.385, 0.06, 0.98, tim, 0.002, 1))
    # vertical larch boards with V-grooves between them on all four faces
    for k in range(8):
        y0 = -0.385 + 0.77 * k / 8
        for sx in (-1, 1):
            out.append(box(sx * 0.235, sx * 0.25, y0 + 0.002, y0 + 0.77 / 8 - 0.002, 0.065, 0.975, tim, 0.003, 2))
    for k in range(5):
        x0 = -0.235 + 0.47 * k / 5
        for sy in (-1, 1):
            out.append(box(x0 + 0.002, x0 + 0.47 / 5 - 0.002, sy * 0.385, sy * 0.40, 0.065, 0.975, tim, 0.003, 2))
    for sx in (-1, 1):
        for sy in (-1, 1):
            out.append(box(sx * 0.25 - 0.012, sx * 0.25 + 0.012, sy * 0.40 - 0.012, sy * 0.40 + 0.012, 0.06, 0.98, brz, 0.003, 2))
    out.append(box(-0.252, 0.252, -0.402, 0.402, 0.06, 0.10, brz, 0.003, 2))

    def ztop(x):
        return 1.12 - (x + 0.3) / 0.6 * 0.10
    out.append(prism_y('top', [(-0.3, 0.98), (0.3, 0.98), (0.3, ztop(0.3)), (-0.3, ztop(-0.3))], -0.45, 0.45, tim))
    for y0, y1 in ((-0.455, -0.45), (0.45, 0.455)):
        out.append(prism_y('band', [(-0.305, 0.975), (0.305, 0.975), (0.305, ztop(0.3) + 0.004), (-0.305, ztop(-0.3) + 0.004)], y0, y1, brz))
    pad = prism_y('pad', [(-0.24, ztop(-0.24) - 0.004), (0.22, ztop(0.22) - 0.004), (0.22, ztop(0.22) + 0.009), (-0.24, ztop(-0.24) + 0.009)], -0.36, 0.36, lea)
    out.append(pad)
    out.append(rod((0.0, -0.12, ztop(0.0) + 0.016), (0.0, 0.05, ztop(0.0) + 0.016), 0.005, brz, 12))
    # the plaque on the guest side, the key box on the attendant's side
    out.append(box(-0.262, -0.25, -0.26, 0.26, 0.71, 0.91, brz, 0.003, 2))
    out.append(print_quad('plaque', [(-0.265, 0.25, 0.72), (-0.265, -0.25, 0.72), (-0.265, -0.25, 0.90), (-0.265, 0.25, 0.90)], VP_PLAQUE,
                          (0, -1, 0), (0, 0, 1), (-0.265, 0.25, 0.72), (0.5, 0.18), prn))
    out.append(box(0.25, 0.31, -0.3, 0.3, 0.5, 0.92, brz, 0.004, 2))
    out.append(print_quad('keys', [(0.313, -0.27, 0.53), (0.313, 0.27, 0.53), (0.313, 0.27, 0.89), (0.313, -0.27, 0.89)], VP_KEYS,
                          (0, 1, 0), (0, 0, 1), (0.313, -0.27, 0.53), (0.54, 0.36), prn))
    for k in range(5):
        y = -0.2 + 0.1 * k
        out.append(box(0.316, 0.322, y - 0.012, y + 0.012, 0.6 + 0.07 * (k % 2), 0.64 + 0.07 * (k % 2), lea, 0.001, 1))
    # the brass lamp on a ledge at the back of the top
    out.append(box(0.24, 0.305, 0.18, 0.44, ztop(0.27), ztop(0.27) + 0.012, brz, 0.002, 1))
    zb = ztop(0.27) + 0.012
    out.append(lathe('lampbase', [(0.0, 0.0), (0.035, 0.0), (0.03, 0.012), (0.008, 0.02), (0.0, 0.02)], brz, 32, Matrix.Translation((0.27, 0.36, zb))))
    out.append(rod((0.27, 0.36, zb + 0.015), (0.27, 0.36, 1.10), 0.005, brz, 12))
    out.append(rod((0.27, 0.36, 1.10), (0.22, 0.30, 1.12), 0.005, brz, 12))
    shade = parts.lathe_at('shade', [(0.0, 0.0), (0.04, 0.0), (0.036, 0.03), (0.012, 0.045), (0.0, 0.047)], Matrix.Translation((0.21, 0.29, 1.085)), 32)
    core.assign(shade, brz)
    out.append(shade)
    dif = parts.disc('diffuser', (0.21, 0.29, 1.084), (0, 0, 1), 0.034, 0.002, 24)
    core.assign(dif, prn)
    lock_uvs(dif, lambda co: (0.8, 0.55))
    out.append(dif)
    return [finish_mesh(out, 'SM_ECX_ValetPodium', [tim, brz, lea, prn], 35.0)]


# ================================================================== SM_ECX_LuggageCart

def build_cart():
    bra = brass('ecxk_brass')
    crp = carpet('ecxk_carpet')
    rub = rubber('ecxk_rubber')
    lug = luggage('ecxk_luggage')
    out = []
    hx, hy = 0.93, 0.43
    out.append(box(-hx, hx, -hy, hy, 0.2, 0.275, crp, 0.012, 2))
    out.append(box(-hx + 0.01, hx - 0.01, -hy + 0.01, hy - 0.01, 0.16, 0.2, bra, 0.004, 1))
    rail = [(-hx - 0.012, -hy - 0.012, 0.24), (hx + 0.012, -hy - 0.012, 0.24), (hx + 0.012, hy + 0.012, 0.24), (-hx - 0.012, hy + 0.012, 0.24)]
    for a, b in zip(rail, rail[1:] + rail[:1]):
        out.append(rod(a, b, 0.016, bra, 24))
    for p in rail:
        out.append(parts.disc('corner', p, (0, 0, 1), 0.024, 0.06, 20))
        core.assign(out[-1], bra)
    # casters
    for sx in (-1, 1):
        for sy in (-1, 1):
            cx, cy = sx * 0.78, sy * 0.33
            out.append(parts.disc('plate', (cx, cy, 0.155), (0, 0, 1), 0.05, 0.01, 24))
            core.assign(out[-1], bra)
            out.append(box(cx - 0.035, cx + 0.035, cy - 0.028, cy + 0.028, 0.10, 0.15, bra, 0.004, 1))
            for s in (-1, 1):
                out.append(box(cx - 0.012, cx + 0.03, cy + s * 0.026 - 0.004, cy + s * 0.026 + 0.004, 0.07, 0.115, bra, 0.002, 1))
            tire = lathe('tire', [(0.03, -0.02), (0.066, -0.02), (0.072, -0.016), (0.075, -0.008), (0.075, 0.008), (0.072, 0.016), (0.066, 0.02), (0.03, 0.02)], rub, 48,
                         Matrix.Translation((cx + 0.01, cy, 0.075)) @ Matrix.Rotation(math.pi / 2, 4, 'X'))
            out.append(tire)
            out.append(parts.disc('hub', (cx + 0.01, cy, 0.075), (0, 1, 0), 0.032, 0.044, 20))
            core.assign(out[-1], bra)
    # the birdcage: an arch at each end, crown rails between them, bars across the ends, finials
    for sx in (-1, 1):
        x = sx * 0.9
        arch = [(x, -0.40, 0.27), (x, -0.40, 1.0), (x, -0.40, 1.5), (x, -0.33, 1.75), (x, -0.18, 1.89), (x, 0.0, 1.93), (x, 0.18, 1.89),
                (x, 0.33, 1.75), (x, 0.40, 1.5), (x, 0.40, 1.0), (x, 0.40, 0.27)]
        out.append(tube(arch, 0.018, bra, 24, 14))
        for z in (0.75, 1.15):
            out.append(rod((x, -0.40, z), (x, 0.40, z), 0.012, bra, 20))
        out.append(parts.disc('ball', (x, 0.0, 1.935), (0, 0, 1), 0.03, 0.05, 16))
        core.assign(out[-1], bra)
        for sy in (-1, 1):
            out.append(parts.disc('foot', (x, sy * 0.4, 0.28), (0, 0, 1), 0.03, 0.02, 16))
            core.assign(out[-1], bra)
    out.append(rod((-0.9, 0.0, 1.905), (0.9, 0.0, 1.905), 0.016, bra, 24))
    for sy in (-1, 1):
        out.append(rod((-0.9, sy * 0.33, 1.75), (0.9, sy * 0.33, 1.75), 0.012, bra, 20))
    # the load
    case = parts.rbox('case', (0.55, -0.12, 0.28 + 0.34), (0.28, 0.45, 0.68), 0.035, 3)
    core.assign(tag(case, 1), lug)
    out.append(case)
    for z in (0.42, 0.52, 0.72, 0.82):
        rib = parts.rbox('rib', (0.55, -0.12, z), (0.29, 0.46, 0.012), 0.005, 1)
        core.assign(tag(rib, 1), lug)
        out.append(rib)
    for sy in (-1, 1):
        w = parts.disc('wheel', (0.45, -0.12 + sy * 0.18, 0.295), (0, 1, 0), 0.025, 0.02, 16)
        core.assign(tag(w, 5), lug)
        out.append(w)
    out.append(tag(box(0.53, 0.57, -0.2, -0.04, 0.96, 0.98, None), 5))
    core.assign(out[-1], lug)
    navy = parts.rbox('navy', (-0.38, 0.0, 0.28 + 0.13), (0.72, 0.48, 0.26), 0.03, 3)
    core.assign(tag(navy, 2), lug)
    out.append(navy)
    out.append(tag(box(-0.42, -0.34, 0.24, 0.26, 0.33, 0.47, None), 5))
    core.assign(out[-1], lug)
    rings = []
    for k in range(13):
        t = k / 12
        x = -0.68 + 0.6 * t
        bulge = math.sin(math.pi * min(max(t, 0.02), 0.98)) ** 0.35
        ry, rz = 0.15 * bulge + 0.01, 0.13 * bulge + 0.01
        ring = []
        for j in range(20):
            a = 2 * math.pi * j / 20
            z = rz * math.sin(a)
            z = max(z, -rz * 0.6)
            ring.append((x, ry * math.cos(a) * (1.0 + 0.04 * math.sin(3 * a + t * 6)), 0.54 + 0.13 + z))
        rings.append(ring)
    duf = core.loft('duffel', rings)
    core.assign(tag(duf, 3), lug)
    out.append(duf)
    for xs in (-0.55, -0.25):
        out.append(tag(tube([(xs, -0.06, 0.8), (xs - 0.02, 0.0, 0.9), (xs, 0.06, 0.8)], 0.012, lug, 10, 6), 3))
    gb = []
    for k in range(14):
        t = k / 13
        z = 1.84 - 1.12 * t
        w2 = (0.035 + 0.255 * float(smoothstep(0.0, 0.14, t))) * (1.0 - 0.06 * t)
        d2 = 0.012 + 0.048 * float(smoothstep(0.0, 0.2, t)) * (1.0 - 0.25 * t)
        ring = []
        for a in np.linspace(0, 2 * math.pi, 24, endpoint=False):
            c, s = math.cos(a), math.sin(a)
            ring.append((0.1 + w2 * math.copysign(abs(c) ** 0.45, c), d2 * math.copysign(abs(s) ** 0.6, s) + 0.006 * math.sin(3 * a + 7 * t), z))
        gb.append(ring)
    bag = core.loft('garment', gb)
    core.assign(tag(bag, 4), lug)
    out.append(bag)
    out.append(tag(tube([(0.1, 0.0, 1.82), (0.1, 0.0, 1.87), (0.12, 0.0, 1.92), (0.1, 0.0, 1.935), (0.08, 0.0, 1.92)], 0.004, lug, 8, 6), 5))
    return [finish_mesh(out, 'SM_ECX_LuggageCart', [bra, crp, rub, lug], 35.0)]


# ================================================================== snow banks

def profile_sweep(name, ys, prof_fn, mats_fn=None, caps=True, col_mats=None):
    """A surface swept along Y: prof_fn(y) -> list of (x, z) across (the same count every time); returns the object
    (normals up/out). mats_fn(x, y, z) -> material index per face center; col_mats(k) -> material index by the
    face's column k across the profile instead. caps=False leaves the ends open (a sweep whose end rows lie flat
    under the ground)."""
    bm = bmesh.new()
    rows = [[bm.verts.new((x, y, z)) for x, z in prof_fn(y)] for y in ys]
    n = len(rows[0])
    for a, b in zip(rows[:-1], rows[1:]):
        for k in range(n - 1):
            f = bm.faces.new((a[k], a[k + 1], b[k + 1], b[k]))
            if col_mats is not None:
                f.material_index = col_mats(k)
    for row, rev in ((rows[0], True), (rows[-1], False)):
        if caps and len(row) >= 3:
            try:
                bm.faces.new(list(reversed(row)) if rev else row)
            except ValueError:
                pass
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    o = core.mesh_object(name, bm)
    core.orient_normals(o, (0, 0, 1))
    if mats_fn is not None:
        for p in o.data.polygons:
            c = p.center
            p.material_index = mats_fn(c.x, c.y, c.z)
    return o


def _nz(x, y, s, seed, o=3):
    return float(vnoise2(np.array([x]), np.array([y]), s, seed, o)[0])


def build_snowbank_a():
    sn = snow('ecxa_snow', dirty=0.5, ground=0.0)
    gr = grime('ecxa_grime', 0.0)
    base = [(-0.80, -0.03), (-0.76, 0.06), (-0.69, 0.22), (-0.6, 0.42), (-0.49, 0.6), (-0.36, 0.74), (-0.2, 0.84), (-0.04, 0.88), (0.12, 0.85),
            (0.28, 0.76), (0.42, 0.6), (0.55, 0.4), (0.66, 0.2), (0.74, 0.06), (0.80, -0.03)]
    ts = np.linspace(0.0, 1.0, 72)
    arc = np.concatenate([[0.0], np.cumsum([math.hypot(b[0] - a[0], b[1] - a[1]) for a, b in zip(base[:-1], base[1:])])])
    arc /= arc[-1]
    base_x = np.array([p[0] for p in base])
    base_z = np.array([p[1] for p in base])
    bx = np.interp(ts, arc, base_x)
    bz = np.interp(ts, arc, base_z)

    def shape(y):
        end = float(smoothstep(0.0, 1.0, 5.0 - abs(y)))
        sh = (0.62 + 0.42 * _nz(0.0, y, 1.1, 3)) * (0.15 + 0.85 * end)
        return end, sh

    def row(y, tt):
        """The bank's cross-section at y, sampled at arc fractions tt (0 = the plow face's foot, 1 = the back's)."""
        end, sh = shape(y)
        sw = 0.4 + 0.6 * end
        xs_ = np.interp(tt, arc, base_x)
        zs_ = np.interp(tt, arc, base_z)
        yy = np.full_like(xs_, y)
        chunky = smoothstep(0.36, 0.55, tt)
        d = (0.11 * chunky + 0.006) * (vnoise2(xs_ * 2, yy, 0.32, 7, 3) - 0.5) * 2 + 0.05 * chunky * (vnoise2(xs_ * 5, yy * 2, 0.11, 8, 2) - 0.5)
        zz = zs_ * sh + np.where(zs_ > 0.0, d, 0.0)
        jit = 0.012 * (vnoise2(tt * 4, yy * 0.5, 0.25, 11, 3) - 0.5)
        return xs_ * sw + np.where(tt < 0.36, jit, d * 0.6), zz

    # The slush line on each side: higher on the plow face (the blade's spray), wandering along the bank, ragged at
    # the scale of a hand. Each row's samples are spaced so that one row edge lies exactly on it (KA samples up to
    # the plow face's line, KB down from the back's): the grime/snow boundary follows the line instead of stepping
    # from one face row to the next (a per-face test drew it as a crenellated staircase).
    KA, KM, KB = 6, 60, 5
    fine = np.linspace(0.0, 1.0, 321)

    def lines(y):
        front = 0.12 + 0.07 * _nz(0.0, y, 0.9, 13, 2) + 0.05 * (_nz(0.0, y, 0.16, 14, 2) - 0.5) + 0.03 * (_nz(0.0, y * 1.7, 0.045, 15, 1) - 0.5)
        back = 0.05 + 0.06 * _nz(9.0, y, 0.9, 16, 2) + 0.04 * (_nz(9.0, y, 0.16, 17, 2) - 0.5) + 0.02 * (_nz(9.0, y * 1.7, 0.045, 18, 1) - 0.5)
        return front, back

    def crossing(zf, level, from_end):
        z = zf[::-1] if from_end else zf
        t = fine[::-1] if from_end else fine
        i = int(np.argmax(z >= level))
        if i == 0:
            return float(t[0])
        z0, z1 = z[i - 1], z[i]
        return float(t[i - 1] + (t[i] - t[i - 1]) * (level - z0) / max(z1 - z0, 1e-6))

    def prof(y):
        _, zf = row(y, fine)
        la, lb = lines(y)
        top = float(zf.max())
        la, lb = min(la, 0.5 * top), min(lb, 0.5 * top)
        ta = min(max(crossing(zf, la, False), 0.01), 0.4)
        tb = max(min(crossing(zf, lb, True), 0.99), 0.6)
        tt = np.concatenate([np.linspace(0.0, ta, KA + 1), np.linspace(ta, tb, KM + 1)[1:], np.linspace(tb, 1.0, KB + 1)[1:]])
        xx, zz = row(y, tt)
        return list(zip(xx.tolist(), zz.tolist()))
    ys = np.arange(-5.0, 5.0001, 0.05)
    o = profile_sweep('bank', ys, prof, col_mats=lambda k: 1 if (k < KA or k >= KA + KM) else 0)
    o.data.materials.append(sn.m)
    o.data.materials.append(gr.m)
    # Chunks the plow threw up onto the crest and the back.
    rng = np.random.default_rng(17)
    bm = bmesh.new()
    for i in range(44):
        y = rng.uniform(-4.4, 4.4)
        end, sh = shape(y)
        t = rng.uniform(0.42, 0.85)
        x = float(np.interp(t, ts, bx)) * (0.4 + 0.6 * end)
        z = float(np.interp(t, ts, bz)) * sh
        r = rng.uniform(0.07, 0.2) * (0.5 + 0.5 * end)
        res = bmesh.ops.create_icosphere(bm, subdivisions=2, radius=1.0)
        rot = Matrix.Rotation(rng.uniform(0, 6.3), 4, 'Z') @ Matrix.Rotation(rng.uniform(-0.5, 0.5), 4, 'X')
        for v in res['verts']:
            dv = Vector((v.co.x * r * rng.uniform(0.85, 1.3), v.co.y * r * rng.uniform(0.85, 1.3), v.co.z * r * rng.uniform(0.55, 0.85)))
            dv *= 0.85 + 0.3 * _nz(v.co.x * 3 + i, v.co.y * 3, 0.6, 19, 2)
            v.co = (rot @ dv.to_4d()).to_3d() + Vector((x, y, z - r * 0.25))
    chunks = core.mesh_object('chunks', bm)
    chunks.data.materials.append(sn.m)
    chunks.data.materials.append(gr.m)
    return [finish_mesh([o, chunks], 'SM_ECX_SnowBank_A', [sn, gr], 60.0, weighted=False)]


def snow_boulders(bm, rng, n, place, rmin, rmax, seed=0):
    """Lumpy snow chunks: place(i) -> (x, y, z) on the surface they sit on (half sunk into it)."""
    for i in range(n):
        x, y, z = place(i)
        r = rng.uniform(rmin, rmax)
        res = bmesh.ops.create_icosphere(bm, subdivisions=2, radius=1.0)
        rot = Matrix.Rotation(rng.uniform(0, 6.3), 4, 'Z') @ Matrix.Rotation(rng.uniform(-0.6, 0.6), 4, 'X')
        sx, sy, sz = rng.uniform(0.85, 1.35), rng.uniform(0.85, 1.35), rng.uniform(0.5, 0.85)
        for v in res['verts']:
            dv = Vector((v.co.x * r * sx, v.co.y * r * sy, v.co.z * r * sz))
            dv *= 0.82 + 0.36 * _nz(v.co.x * 3 + i * 1.7, v.co.y * 3 + seed, 0.55, 19 + seed, 2)
            v.co = (rot @ dv.to_4d()).to_3d() + Vector((x, y, z - r * 0.3))


def build_snowbank_b():
    sn = snow('ecxbb_snow', dirty=0.4, ground=0.0)
    gr = grime('ecxbb_grime', 0.0)
    # A pushed heap: steep where the blade stopped (-x), a long tail behind, lobes and ledges, a 1-3 cm apron of
    # slush and grit round its foot. It is built on a polar grid whose rings follow the heap's own contours (a square
    # height-field grid stood its steep foot up in slivers and tore the slush line into teeth): the slush line is a
    # ring, ragged by its radius wandering with the angle, and the apron sinks under the ground inside 6 x 5 m.
    SHEAR = 0.25

    def side(th):
        """0 on the blade face (-x), 1 on the tail (+x), blended across the y axis."""
        return float(smoothstep(-0.35, 0.35, math.cos(th)))

    def r_edge(th):
        """The heap's foot radius toward th (before the shear): an ellipse, shorter on the blade side, lobed."""
        w = side(th)
        A = 2.35 * (0.8 + 0.2 * w)
        B = 1.95
        c, s = math.cos(th), math.sin(th)
        lobe = 0.88 + 0.24 * _nz(c * 1.3 + 5.0, s * 1.3, 0.45, 21, 3)
        return lobe / math.sqrt((c / A) ** 2 + (s / B) ** 2)

    def body_z(rho, th, x, y):
        u = max(0.0, 1.0 - rho * rho)
        w = side(th)
        p, e = 0.55 + 0.35 * w, 0.07 - 0.04 * w
        dome = ((u + e) ** p - e ** p) / ((1.0 + e) ** p - e ** p)
        ledge = 0.08 * math.sin(dome * 9.0 + 2.0 * _nz(x, y, 0.8, 25)) * dome
        lumps = (_nz(x, y, 0.5, 23, 3) - 0.5) * 0.38 + (_nz(x, y, 0.16, 24, 2) - 0.5) * 0.09
        return max(1.45 * dome + ledge + lumps * min(1.0, dome * 3.0), apron0(x, y))

    def apron0(x, y):
        return 0.016 + 0.014 * _nz(x, y, 0.3, 26)

    def apron_z(dr, x, y):
        return apron0(x, y) * (1.0 - float(smoothstep(0.1, 0.24, dr))) - 0.045 * float(smoothstep(0.18, 0.3, dr))

    def xy(r, th):
        x0, y = r * math.cos(th), r * math.sin(th)
        return x0 - SHEAR * math.sin(0.9 * y), y

    def h(x, y):
        """The surface's height at (x, y) (to seat the chunks and slabs)."""
        x0 = x + SHEAR * math.sin(0.9 * y)
        th = math.atan2(y, x0)
        r = math.hypot(x0, y)
        re = r_edge(th)
        return body_z(r / re, th, x, y) if r <= re else apron_z(r - re, x, y)

    NT, K1, K2, K3 = 208, 56, 8, 8
    bm = bmesh.new()
    gl = bm.faces.layers.float.new('ecx_grime')
    center = bm.verts.new((0.0, 0.0, body_z(0.0, 0.0, 0.0, 0.0)))
    cols = []
    for j in range(NT):
        th = 2.0 * math.pi * j / NT
        c, s = math.cos(th), math.sin(th)
        # where the slush line crosses this ray (a fraction of the foot radius), ragged with the angle
        # (about 14 and 38 wobbles round the heap: finer, and neighbouring rays alternate into teeth)
        rho_g = 0.972 + 0.036 * (_nz(c * 1.3 + 9.0, s * 1.3, 0.6, 34, 2) - 0.5) + 0.012 * (_nz(c * 3.0, s * 3.0 + 3.0, 0.5, 35, 1) - 0.5)
        rho_g = min(rho_g, 0.992)
        re = r_edge(th)
        col = []
        for k in range(1, K1 + 1):
            rho = rho_g * (1.0 - (1.0 - k / K1) ** 1.35)
            x, y = xy(rho * re, th)
            col.append(bm.verts.new((x, y, body_z(rho, th, x, y))))
        for k in range(1, K2 + 1):
            rho = rho_g + (1.0 - rho_g) * k / K2
            x, y = xy(rho * re, th)
            col.append(bm.verts.new((x, y, body_z(rho, th, x, y))))
        for k in range(1, K3 + 1):
            dr = 0.3 * k / K3
            x, y = xy(re + dr, th)
            col.append(bm.verts.new((x, y, apron_z(dr, x, y))))
        cols.append(col)
    for j in range(NT):
        a, b = cols[j], cols[(j + 1) % NT]
        f = bm.faces.new((center, a[0], b[0]))
        f[gl] = 0.0
        for k in range(len(a) - 1):
            f = bm.faces.new((a[k], a[k + 1], b[k + 1], b[k]))
            f[gl] = 1.0 if k >= K1 - 1 else 0.0
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    pile = core.mesh_object('pile', bm)
    core.orient_normals(pile, (0, 0, 1))
    out = [pile]
    rng = np.random.default_rng(29)
    bm = bmesh.new()

    def spot(i):
        for _ in range(50):
            a = rng.uniform(0, 6.28)
            rr = rng.uniform(0.3, 2.2)
            px, py = rr * math.cos(a) * 1.15, rr * math.sin(a) * 0.95
            pz = h(px, py)
            if 0.3 < pz < 1.25:
                return px, py, pz
        return 0.0, 0.0, h(0.0, 0.0) - 0.2
    snow_boulders(bm, rng, 30, spot, 0.1, 0.32, 3)
    out.append(core.mesh_object('chunks', bm))
    slabs = []
    for i in range(9):
        a = rng.uniform(0, 6.28)
        rr = rng.uniform(0.5, 2.0)
        px, py = rr * math.cos(a) * 1.1, rr * math.sin(a) * 0.9
        pz = h(px, py)
        sx, sy, sz = rng.uniform(0.35, 0.8), rng.uniform(0.25, 0.55), rng.uniform(0.07, 0.15)
        s = parts.rbox('slab', (0, 0, 0), (sx, sy, sz), 0.02, 2)
        me = s.data
        for v in me.vertices:
            v.co += Vector((rng.uniform(-0.02, 0.02), rng.uniform(-0.02, 0.02), rng.uniform(-0.015, 0.015)))
        rot = Matrix.Rotation(rng.uniform(0, 6.28), 4, 'Z') @ Matrix.Rotation(rng.uniform(0.3, 1.0), 4, 'X')
        me.transform(Matrix.Translation((px, py, pz - sz * 0.2)) @ rot)
        set_attr(s, 'ecx_slab', 1.0)
        slabs.append(s)
    out += slabs
    obj = core.join(out, 'SM_ECX_SnowBank_B')
    obj.data.materials.clear()
    obj.data.materials.append(sn.m)
    obj.data.materials.append(gr.m)
    me = obj.data
    me.update()
    n = len(me.polygons)
    grime_f = np.zeros(n, dtype=np.float32)
    slab_f = np.zeros(n, dtype=np.float32)
    if me.attributes.get('ecx_grime') is not None:
        me.attributes['ecx_grime'].data.foreach_get('value', grime_f)
    if me.attributes.get('ecx_slab') is not None:
        me.attributes['ecx_slab'].data.foreach_get('value', slab_f)
    for p in me.polygons:
        c = p.center
        # the slush ring (the pile's rings beyond the slush line), and the broken slabs' dirty undersides
        dirty_slab = slab_f[p.index] > 0.5 and p.normal.z < 0.2 and c.z > 0.15 and _nz(c.x * 5, c.y * 5, 0.2, 33) > 0.6
        p.material_index = 1 if (grime_f[p.index] > 0.5 or dirty_slab) else 0
    return [finish_mesh([obj], 'SM_ECX_SnowBank_B', [sn, gr], 60.0, weighted=False)]


def build_snowbank_c():
    sn = snow('ecxcc_snow')
    gr = grime('ecxcc_grime', 0.0)
    base = [(-1.25, -0.02), (-1.05, 0.03), (-0.85, 0.07), (-0.6, 0.13), (-0.35, 0.2), (-0.1, 0.27), (0.15, 0.33), (0.38, 0.39), (0.55, 0.43),
            (0.66, 0.45), (0.74, 0.44), (0.79, 0.415), (0.78, 0.39), (0.74, 0.37), (0.71, 0.34), (0.72, 0.28), (0.76, 0.2), (0.83, 0.11),
            (0.93, 0.04), (1.02, -0.005), (1.06, -0.02)]
    ts = np.linspace(0.0, 1.0, 52)
    arc = np.concatenate([[0.0], np.cumsum([math.hypot(b[0] - a[0], b[1] - a[1]) for a, b in zip(base[:-1], base[1:])])])
    arc /= arc[-1]
    bx = np.interp(ts, arc, [p[0] for p in base])
    bz = np.interp(ts, arc, [p[1] for p in base])

    def prof(y):
        # The drift tapers to a point at both tips (lower and narrower over the last 1.6 m, the north tip longer,
        # as wind tails are) and sinks just under the ground there, so no blunt end face shows; its crest meanders.
        ay = abs(y)
        tip = 1.9 if y > 0 else 1.4
        end = float(smoothstep(0.0, tip, 4.0 - ay))
        sh = (0.8 + 0.3 * _nz(0.0, y, 1.3, 41)) * end ** 0.75
        wid = 0.3 + 0.7 * end ** 0.6
        shift = 0.18 * (_nz(0.0, y, 1.6, 44) - 0.5) * 2.0 * end
        lip = 0.05 * (_nz(0.0, y, 0.5, 42) - 0.5) + 0.03 * (_nz(0.0, y, 0.18, 45) - 0.5)
        sink = 0.022 * (1.0 - float(smoothstep(0.0, 0.35, end)))
        pts = []
        for t, x, z in zip(ts, bx, bz):
            rip = 0.014 * math.sin(2 * math.pi * x / (0.15 + 0.05 * _nz(0.0, y, 0.7, 46)) + 3.0 * _nz(x, y, 0.6, 43))
            rip *= float(smoothstep(-1.1, -0.9, x)) * float(smoothstep(0.55, 0.3, x))
            xx = x + (lip * end if x > 0.6 else 0.0)
            zz = max(z * sh + rip * end, -0.02) if z > -0.01 else z
            pts.append((xx * wid + shift, zz - sink))
        return pts
    ys = np.arange(-4.0, 4.0001, 0.04)
    o = profile_sweep('drift', ys, prof, lambda x, y, z: 1 if z < 0.004 else 0, caps=False)
    o.data.materials.append(sn.m)
    o.data.materials.append(gr.m)
    return [finish_mesh([o], 'SM_ECX_SnowBank_C', [sn, gr], 60.0, weighted=False)]


# ================================================================== SM_ECX_ViewRail

def build_viewrail():
    bas = basalt('ecxr_basalt', joints=(1.2, 0.6), ground=0.0, salt=0.6, streaks=0.8, wet_foot=0.4)
    brz = bronze('ecxr_bronze', rime=1.0, scuff=0.4, patina=0.6)
    sn = snow('ecxr_snow')
    out = []
    out.append(box(0.0, 0.45, -3.0, 3.0, -2.5, 0.68, bas, 0.004, 1))
    out.append(box(-0.025, 0.475, -2.998, 2.998, 0.68, 0.75, bas, 0.006, 2))
    out.append(box(-0.027, 0.477, -2.996, 2.996, 0.664, 0.672, bas, 0.0, 1))
    for y in (-2.25, -0.75, 0.75, 2.25):
        out.append(lathe('flange', [(0.0, 0.75), (0.05, 0.75), (0.05, 0.758), (0.03, 0.766), (0.0, 0.766)], brz, 32, Matrix.Translation((0.2, y, 0.0))))
        out.append(lathe('post', [(0.0, 0.75), (0.024, 0.75), (0.024, 1.05), (0.0, 1.05)], brz, 24, Matrix.Translation((0.2, y, 0.0))))
    out.append(rod((0.2, -3.0, 1.07), (0.2, 3.0, 1.07), 0.03, brz, 32))
    for y in (-2.25, -0.75, 0.75, 2.25):
        out.append(lathe('saddle', [(0.0, 1.035), (0.03, 1.035), (0.036, 1.05), (0.0, 1.05)], brz, 24, Matrix.Translation((0.2, y, 0.0))))

    def th(y):
        a = 2 * math.pi * y / 6.0
        return 0.045 + 0.018 * math.sin(a + 0.7) + 0.012 * math.sin(3 * a + 1.9) + 0.006 * math.sin(7 * a + 0.4)
    prof_x = [(-0.03, -0.02), (-0.02, 0.2), (0.05, 0.75), (0.15, 0.95), (0.25, 1.0), (0.35, 0.95), (0.42, 0.75), (0.47, 0.3), (0.475, -0.02),
              (0.44, -0.03)]

    def prof(y):
        t = th(y)
        return [(x, 0.75 + (t * f if f >= 0 else f)) for x, f in prof_x]
    s = profile_sweep('snow', np.linspace(-3.0, 3.0, 121), prof)
    core.assign(s, sn)
    out.append(s)
    return [finish_mesh(out, 'SM_ECX_ViewRail', [bas, brz, sn], 35.0)]


# ================================================================== SM_ECX_Viewer

def build_viewer():
    brz = bronze('ecxw_bronze', rime=0.6, scuff=0.8, patina=0.5, edge=0.003, facets=True)
    frm = frame_dark('ecxw_frame')
    gla = glass_dark('ecxw_glass')
    out = []
    dz = 0.18   # the head rides 0.18 higher than a desk unit: eyepieces at 1.50 m
    out.append(lathe('base', [(0.0, 0.0), (0.2, 0.0), (0.2, 0.02), (0.185, 0.03), (0.17, 0.035), (0.09, 0.046), (0.075, 0.05), (0.0, 0.05)], brz, 72))
    for a in range(4):
        ang = math.pi / 4 + a * math.pi / 2
        out.append(lathe('bolt', [(0.0, 0.02), (0.014, 0.02), (0.014, 0.032), (0.008, 0.038), (0.0, 0.038)], frm, 16,
                         Matrix.Translation((0.15 * math.cos(ang), 0.15 * math.sin(ang), 0.0))))
    out.append(lathe('column', [(0.0, 0.04), (0.07, 0.04), (0.068, 0.12), (0.066, 0.3), (0.056, 0.95 + dz), (0.06, 0.97 + dz), (0.06, 1.0 + dz),
                                (0.0, 1.0 + dz)], brz, 72))
    out.append(lathe('band', [(0.0, 0.55), (0.066, 0.55), (0.068, 0.565), (0.066, 0.58), (0.0, 0.58)], brz, 72))
    out.append(lathe('swivel', [(0.0, 1.0 + dz), (0.09, 1.0 + dz), (0.095, 1.02 + dz), (0.09, 1.04 + dz), (0.0, 1.04 + dz)], frm, 72))
    # the yoke: a cast bridge on the swivel, its two arms rising beside the head to the tilt pivots
    out.append(box(-0.045, 0.045, -0.196, 0.196, 1.04 + dz, 1.085 + dz, brz, 0.012, 3))
    out.append(lathe('hub', [(0.0, 1.085 + dz), (0.06, 1.085 + dz), (0.05, 1.10 + dz), (0.0, 1.10 + dz)], brz, 48))
    for sy in (-1, 1):
        out.append(box(-0.04, 0.04, sy * 0.18 - 0.015, sy * 0.18 + 0.015, 1.08 + dz, 1.30 + dz, brz, 0.008, 3))
        out.append(parts.disc('pivot', (0.0, sy * 0.198, 1.27 + dz), (0, 1, 0), 0.035, 0.012, 32))
        core.assign(out[-1], frm)
        out.append(rod((-0.04, sy * 0.2, 1.2 + dz), (-0.04, sy * 0.25, 1.2 + dz), 0.016, frm, 24))
        out.append(parts.disc('grip_end', (-0.04, sy * 0.255, 1.2 + dz), (0, 1, 0), 0.02, 0.01, 24))
        core.assign(out[-1], frm)
    out.append(box(-0.15, 0.15, -0.165, 0.165, 1.12 + dz, 1.42 + dz, brz, 0.05, 5))
    out.append(box(-0.205, -0.12, -0.16, 0.16, 1.37 + dz, 1.40 + dz, frm, 0.012, 3))
    for sy in (-1, 1):
        y = sy * 0.065
        out.append(rod((-0.15, y, 1.32 + dz), (-0.2, y, 1.32 + dz), 0.024, frm, 32))
        out.append(lathe('cup', [(0.016, 0.0), (0.03, 0.0), (0.034, 0.01), (0.034, 0.02), (0.03, 0.03), (0.02, 0.028)], frm, 32,
                         Matrix.Translation((-0.2, y, 1.32 + dz)) @ Matrix.Rotation(-math.pi / 2, 4, 'Y')))
        out.append(parts.disc('ocular', (-0.205, y, 1.32 + dz), (1, 0, 0), 0.015, 0.004, 24))
        core.assign(out[-1], gla)
        y = sy * 0.085
        out.append(rod((0.15, y, 1.28 + dz), (0.28, y, 1.28 + dz), 0.052, brz, 48))
        for xb in (0.19, 0.24):
            out.append(lathe('ring', [(0.0, -0.006), (0.055, -0.006), (0.055, 0.006), (0.0, 0.006)], brz, 48,
                             Matrix.Translation((xb, y, 1.28 + dz)) @ Matrix.Rotation(math.pi / 2, 4, 'Y')))
        out.append(lathe('hood', [(0.044, 0.0), (0.058, 0.0), (0.058, 0.03), (0.05, 0.03)], frm, 48,
                         Matrix.Translation((0.28, y, 1.28 + dz)) @ Matrix.Rotation(math.pi / 2, 4, 'Y')))
        out.append(parts.disc('lens', (0.29, y, 1.28 + dz), (1, 0, 0), 0.046, 0.006, 48))
        core.assign(out[-1], gla)
    # the coin plate, facing the user: 25¢ in polished relief, the slot beside it
    out.append(tag(box(-0.158, -0.15, -0.07, 0.07, 1.165 + dz, 1.245 + dz, brz, 0.003, 2), 2))
    t, _ = channel_letters('25¢', 'Black', 0.07, -0.06, 1.19 + dz, 0.0, 0.004, True, -0.158, brz, brz, tracking=1.1)
    out.append(tag(t, 0))
    out.append(box(-0.1595, -0.157, 0.03, 0.06, 1.18 + dz, 1.23 + dz, frm, 0.0, 1))
    return [finish_mesh(out, 'SM_ECX_Viewer', [brz, frm, gla], 35.0)]


# ================================================================== build

BUILDERS = {'ground': lambda: [build_ground()], 'steps': build_steps, 'cascade': lambda: build_cascade() + build_cascade_water(),
            'firebowl': build_firebowl, 'monument': build_monument, 'shelter': build_shelter, 'stopsign': build_stopsign,
            'lamppost': build_lamppost, 'bollard': build_bollard, 'planter': build_planter, 'bench': build_bench, 'valet': build_valet,
            'cart': build_cart, 'ashurn': build_ashurn, 'snowbank_a': build_snowbank_a, 'snowbank_b': build_snowbank_b,
            'snowbank_c': build_snowbank_c, 'viewrail': build_viewrail, 'viewer': build_viewer}


def build():
    core.reset()
    if os.environ.get('ECX_TEX', '1') != '0' and (not _ONLY or 'tex' in _ONLY or 'ground' in _ONLY):
        make_textures()
    objs = []
    for key, fn in BUILDERS.items():
        if _ONLY and key not in _ONLY:
            continue
        t0 = time.time()
        made = fn()
        objs += made
        print(f'[ecx_plaza] built {key} ({", ".join(o.name for o in made)}) in {time.time() - t0:.1f}s')
    for o in objs:
        o.data.name = o.name
    return objs


def bake_parts(objs):
    """Bakes every mesh but the unbaked ones (ground, water, flame, screen) and the glows. Those are hidden from
    the bake so they don't occlude the props that share their origin (pose_for_review shows them again)."""
    bake = []
    for o in objs:
        if o.name in UNBAKED or '_Glow' in o.name:
            o.hide_render = True
        else:
            bake.append(o)
    return bake


# ================================================================== review

# §2.5 placements in map meters: (X, Y, Z or None for the ground under it, yaw, scale_y). Glow, screen, flame and
# water meshes follow their parents.
def _placements():
    P = {
        'SM_ECX_Ground': [(0.0, 0.0, 0.0, 180.0, 1.0)],
        'SM_ECX_Cascade': [(0.0, 0.0, 0.0, 180.0, 1.0), (0.0, 0.0, 0.0, 180.0, -1.0)],
        'SM_ECX_Steps': [(48.0, 0.0, LEVEL_P, 180.0, 1.0), (36.4, 0.0, LEVEL_A, 180.0, 1.0), (24.8, 0.0, LEVEL_B, 180.0, 1.0)],
        'SM_ECX_FireBowl': [(49.3, 5.3, LEVEL_P, 180, 1), (49.3, -5.3, LEVEL_P, 180, 1), (41.0, 9.75, LEVEL_A, 180, 1), (41.0, -9.75, LEVEL_A, 180, 1),
                            (30.6, 9.75, LEVEL_B, 180, 1), (30.6, -9.75, LEVEL_B, 180, 1)],
        'SM_ECX_MonumentSign': [(45.5, -20.5, LEVEL_A, 165.0, 1.0)],
        'SM_ECX_BusShelter': [(50.6, -12.0, LEVEL_P, 180.0, 1.0)],
        'SM_ECX_BusStopSign': [(52.1, 3.0, LEVEL_P, 90.0, 1.0)],
        'SM_ECX_LampPost': [(x, s * 34.3, float(arm_z(x)) + 0.16, 180, 1) for x in (24.0, 32.0, 40.0, 48.0) for s in (1, -1)] +
                           [(21.0, 22.5, 0.0, 180, 1), (21.0, -22.5, 0.0, 180, 1), (51.6, -17.5, LEVEL_P, 180, 1), (51.6, 10.5, LEVEL_P, 180, 1),
                            (69.6, 6.6, LEVEL_P, 180, 1), (69.6, 19.4, LEVEL_P, 180, 1), (63.7, -30.0, None, 180, 1), (63.7, 40.0, None, 180, 1)],
        'SM_ECX_Bollard': [(7.75, s * y, 0.0, 180, 1) for y in (4.2, 6.6, 9.0, 11.4, 13.8, 16.2) for s in (1, -1)] +
                          [(16.75, 4.2, 0.0, 180, 1), (16.75, -4.2, 0.0, 180, 1)],
        'SM_ECX_Planter': [(49.9, 13.0, LEVEL_P, 180, 1), (49.9, 19.0, LEVEL_P, 180, 1), (49.9, -20.0, LEVEL_P, 180, 1), (4.6, 20.0, 0.0, 180, 1),
                           (4.6, -20.0, 0.0, 180, 1), (20.0, 18.6, 0.0, 180, 1), (20.0, -18.6, 0.0, 180, 1)],
        'SM_ECX_Bench': [(41.4, 3.05, LEVEL_A, 90, 1), (41.4, -3.05, LEVEL_A, 90, 1), (29.8, 3.05, LEVEL_B, 90, 1), (29.8, -3.05, LEVEL_B, 90, 1),
                         (49.6, 8.5, LEVEL_P, 180, 1), (66.0, 13.0, LEVEL_P, 180, 1)],
        'SM_ECX_ValetPodium': [(6.4, -11.0, 0.0, 180, 1)],
        'SM_ECX_LuggageCart': [(2.4, 7.2, 0.0, 90, 1)],
        'SM_ECX_AshUrn': [(22.4, 19.6, 0.0, 180, 1)],
        'SM_ECX_SnowBank_A': [(x, s * 35.5, None, 90 if s > 0 else 270, 1) for x in (26.0, 36.0, 46.0) for s in (1, -1)] +
                             [(54.2, -44.0, None, 180, 1), (54.2, 44.0, None, 180, 1)],
        'SM_ECX_SnowBank_B': [(50.0, 24.3, None, 30, 0.8), (50.0, -24.3, None, 200, 0.8), (22.0, 37.0, None, 110, 1), (22.0, -37.0, None, 300, 1)],
        'SM_ECX_SnowBank_C': [(63.4, y, None, 180, 1) for y in (-40.0, -25.0, -10.0, 25.0, 38.0)] +
                             [(30.0, 21.0, None, 90, 1), (30.0, -21.0, None, 90, 1), (40.0, 22.0, None, 90, 1), (40.0, -22.0, None, 90, 1),
                              (4.0, 30.0, None, 180, 1), (4.0, -30.0, None, 180, 1)],
        'SM_ECX_Viewer': [(69.3, 9.0, LEVEL_P, 0, 1)],
    }
    rail = []
    for yc in list(np.arange(-45.0, 5.0, 6.0)) + list(np.arange(23.5, 48.0, 6.0)):
        rail.append((PARAPET_X, float(yc), float(road_profile(yc)), 0, 1))
    rail += [(PARAPET_X + 3.2, OVER[1][0] + 0.45, LEVEL_P, 270, 1.07), (PARAPET_X + 3.2, OVER[1][1] - 0.45, LEVEL_P, 90, 1.07),
             (OVER[0][1] - 0.45, 13.0, LEVEL_P, 0, 1.0), (OVER[0][1] - 0.45, 7.5, LEVEL_P, 0, 0.5), (OVER[0][1] - 0.45, 18.5, LEVEL_P, 0, 0.5)]
    P['SM_ECX_ViewRail'] = rail
    follow = {'SM_ECX_Steps_GlowWarm': 'SM_ECX_Steps', 'SM_ECX_Cascade_Water': 'SM_ECX_Cascade', 'SM_ECX_FireBowl_Flame': 'SM_ECX_FireBowl',
              'SM_ECX_MonumentSign_GlowEmber': 'SM_ECX_MonumentSign', 'SM_ECX_MonumentSign_Screen': 'SM_ECX_MonumentSign',
              'SM_ECX_BusShelter_GlowCool': 'SM_ECX_BusShelter', 'SM_ECX_LampPost_GlowWarm': 'SM_ECX_LampPost',
              'SM_ECX_Bollard_GlowEmber': 'SM_ECX_Bollard'}
    for k, v in follow.items():
        P[k] = P[v]
    return P


def _bl(p):
    return Vector((-p[0], p[1], p[2]))


class _Review:
    """The site renders: every mesh placed as §2.5 says on the ground, stand-ins for the building, the terrain and the
    cars, a day sun and a night of moon and lamps."""

    def __init__(self, objs):
        self.sc = bpy.context.scene
        self.objs = {o.name: o for o in objs}
        self.temp = []          # objects to delete afterwards
        self.night_lights = []
        self.day_lights = []
        self.glows = []
        self.dir = os.path.join(core.BUILD_DIR, 'review', 'ecx_plaza')
        os.makedirs(self.dir, exist_ok=True)
        self.saved = {o.name: (o.location.copy(), o.rotation_euler.copy(), o.scale.copy()) for o in objs}
        self.ground = self.objs.get('SM_ECX_Ground')
        self.bvh = None

    # ---- placing
    def ground_z(self, X, Y):
        if self.ground is not None:
            if self.bvh is None:
                self.bvh = BVHTree.FromObject(self.ground, bpy.context.evaluated_depsgraph_get())
            hit = self.bvh.ray_cast(Vector((-X, Y, 50.0)), Vector((0, 0, -1)))
            if hit[0] is not None:
                return hit[0].z
        if abs(Y) > 26.0 and X < 56:
            return float(arm_z(X)) + 0.15
        return float(drive_z(np.array([X]), np.array([Y]))[0])

    def place_all(self):
        P = _placements()
        for name, o in self.objs.items():
            spots = P.get(name)
            if not spots:
                continue
            for i, (X, Y, Z, yaw, sy) in enumerate(spots):
                inst = o if i == 0 else self._dup(o)
                z = self.ground_z(X, Y) if Z is None else Z
                if name in ('SM_ECX_Ground', 'SM_ECX_Cascade', 'SM_ECX_Cascade_Water'):
                    inst.location = (0.0, 0.0, 0.0)
                else:
                    inst.location = _bl((X, Y, z))
                inst.rotation_euler = (0.0, 0.0, math.radians(180.0 - yaw))
                inst.scale = (1.0, sy, 1.0) if name in ('SM_ECX_Cascade', 'SM_ECX_Cascade_Water') else (sy if name == 'SM_ECX_SnowBank_B' else 1.0,
                                                                                                         sy, sy if name == 'SM_ECX_SnowBank_B' else 1.0)
        bpy.context.view_layer.update()

    def _dup(self, o):
        d = o.copy()
        self.sc.collection.objects.link(d)
        self.temp.append(d)
        return d

    # ---- context stand-ins
    def _mat(self, name, hexc, rough=0.5, emit=None, strength=0.0, metal=0.0):
        m = core.Mat(name)
        m.set('Base Color', H(hexc))
        m.set('Roughness', rough)
        m.set('Metallic', metal)
        if emit is not None:
            m.set('Emission Color', H(emit))
            m.set('Emission Strength', strength)
        return m

    def _box(self, X0, X1, Y0, Y1, Z0, Z1, mat):
        o = mbox(X0, X1, Y0, Y1, Z0, Z1, mat)
        self.temp.append(o)
        return o

    def _import(self, name):
        """Imports another author's exported mesh (unreal/Art/Meshes/<name>.glb) for context, or None."""
        path = os.path.join(core.EXPORT_DIR, f'{name}.glb')
        if not os.path.exists(path):
            return None
        before = set(bpy.data.objects)
        grp = bpy.data.node_groups.get('glTF Material Output')
        if grp is not None:
            grp.name = 'glTF Material Output (bake)'
        try:
            bpy.ops.import_scene.gltf(filepath=path)
        except Exception:
            traceback.print_exc()
            return None
        finally:
            if grp is not None:
                imp = bpy.data.node_groups.get('glTF Material Output')
                if imp is not None:
                    imp.name = 'glTF Material Output (import)'
                grp.name = 'glTF Material Output'
        new = [o for o in bpy.data.objects if o not in before]
        mesh = next((o for o in new if o.type == 'MESH'), None)
        for o in new:
            if o is not mesh:
                bpy.data.objects.remove(o)
        if mesh is None:
            return None
        mesh.parent = None
        mesh.matrix_world = Matrix()
        mesh.name = 'rv_' + name
        self.temp.append(mesh)
        self.context_objs.append(mesh)
        if '_Glow' in name:
            colr = EMBER_LED if 'Ember' in name else (COOL_GLOW if 'Cool' in name else (0xebe7df if 'Ash' in name else WARM_GLOW))
            m = self._mat('rv_' + name + '_m', colr, 0.5, colr, 3.0)
            for i in range(len(mesh.data.materials)):
                mesh.data.materials[i] = m.m
            self.glows.append(m)
        elif name.endswith('_Glass'):
            g = core.Mat('rv_glass_' + name)
            g.set('Base Color', H(0xdfe8ee))
            g.set('Roughness', 0.02)
            g.set('Transmission Weight', 1.0)
            for i in range(len(mesh.data.materials)):
                mesh.data.materials[i] = g.m
        return mesh

    def context(self):
        self.context_objs = []
        dark = self._mat('rv_basalt', 0x26282d, 0.55)
        self.glass = self._mat('rv_lobby', 0x0b0e12, 0.05, 0xffb070, 0.0)
        self.soffit = self._mat('rv_soffit', 0x9a6a3c, 0.5, 0xffc98a, 0.0)
        self.crown = self._mat('rv_crown', 0x4a3423, 0.4, EMBER_LED, 0.0, 0.8)
        self.tower_win = self._mat('rv_tower', 0x10141a, 0.1, 0xffb070, 0.0)
        bronze_m = self._mat('rv_bronze', 0x5a3e28, 0.35, metal=1.0)
        have = {n: self._import(n) for n in ('SM_ECX_Entry', 'SM_ECX_Canopy', 'SM_ECX_WingN', 'SM_ECX_WingS', 'SM_ECX_Lobby', 'SM_ECX_Lobby_GlowWarm',
                                             'SM_ECX_Podium_Glass', 'SM_ECX_Podium_GlowEmber', 'SM_ECX_Podium_GlowWarm', 'SM_ECX_Podium_GlowAsh',
                                             'SM_ECX_Tower', 'SM_ECX_Tower_GlowEmber', 'SM_ECX_Crown', 'SM_ECX_Crown_GlowEmber', 'SM_ECX_CrownCrest',
                                             'SM_ECX_Marquee')}
        if not have['SM_ECX_WingN']:
            self._box(-30.0, 3.0, 18.5, 56.0, 0.0, 16.6, dark)
        if not have['SM_ECX_WingS']:
            self._box(-30.0, 3.0, -58.0, -18.5, 0.0, 16.6, dark)
        if not have['SM_ECX_Entry']:
            self._box(-18.0, 0.0, -16.0, 16.0, 0.0, 12.8, self.glass)
            self._box(-2.0, 3.0, 16.0, 18.5, 0.0, 12.8, dark)
            self._box(-2.0, 3.0, -18.5, -16.0, 0.0, 12.8, dark)
            self._box(-18.0, 0.0, -16.0, 16.0, 12.0, 12.8, bronze_m)
        elif not have['SM_ECX_Lobby']:
            self._box(-18.0, -4.6, -16.0, 16.0, 0.0, 12.0, self.glass)
        if not have['SM_ECX_Canopy']:
            self._box(0.0, 24.3, -18.2, 18.2, 7.4, 9.0, dark)
            self._box(0.05, 24.25, -18.15, 18.15, 7.0, 7.4, self.soffit)
            for y in (-15.0, -6.0, 6.0, 15.0):
                self._box(18.2, 18.85, y - 0.3, y + 0.3, 0.0, 7.0, bronze_m)
        if not have['SM_ECX_Tower']:
            self._box(-56.0, -36.0, 10.0, 52.0, 16.0, 62.0, self.tower_win)
        if not have['SM_ECX_Crown']:
            self._box(-56.0, -36.0, 10.0, 52.0, 62.0, 66.0, self.crown)
        marq = self._mat('rv_marquee', 0x101418, 0.2, 0xff8a3a, 0.0)
        self._box(3.0 + (0.58 if have['SM_ECX_Marquee'] else 0.0), 3.05 + (0.58 if have['SM_ECX_Marquee'] else 0.0), -46.0, -30.0, 5.0, 11.0, marq)
        self.marquee = marq
        # The 14 at the stop (bus frame: front -X, curb side +Y), its wheels.
        body = self._import('SM_ECX_Bus')
        if body is not None:
            rig = core.link(bpy.data.objects.new('rv_bus', None))
            self.temp.append(rig)
            rig.location = _bl((53.95, -4.28, -2.40))
            rig.rotation_euler = (0.0, 0.0, math.radians(180.0 - 270.0))
            parts_ = [body, self._import('SM_ECX_Bus_Glass')]
            for nm, spots in (('SM_ECX_Bus_Wheel', ((-3.80, 1.06), (-3.80, -1.06))), ('SM_ECX_Bus_WheelDual', ((3.30, 0.93), (3.30, -0.93)))):
                w = self._import(nm)
                if w is None:
                    continue
                for i, (x, y) in enumerate(spots):
                    inst = w if i == 0 else self._dup(w)
                    inst.location = (x, y, 0.51)
                    inst.scale = (1.0, 1.0 if y > 0 else -1.0, 1.0)
                    parts_.append(inst)
            for p in parts_:
                if p is not None:
                    p.parent = rig
        # Terrain stand-in: snow beyond the outline, the drop past the parapet.
        snow_m = snow('rv_snow')
        xs = np.arange(-60.0, 140.0, 2.0)
        ys = np.arange(-210.0, 132.0, 2.0)

        def tz(X, Y):
            if X < 3.0:
                return 0.6 + max(0.0, (abs(Y) - 58.0) * 0.1) if abs(Y) > 58 else -6.0
            if X > OUT_X - 0.4:
                if OVER[0][0] < X < OVER[0][1] + 0.4 and OVER[1][0] - 0.4 < Y < OVER[1][1] + 0.4:
                    return -8.0
                return float(road_profile(Y)) - 0.3 - (X - OUT_X) * 0.8
            if point_in_poly(X, Y, OUTLINE):
                return -9.0
            if X < 55.0:
                return float(arm_z(max(min(X, 56.0), CURVE_C[0]))) + 0.35 + (abs(Y) - 42.0) * 0.12
            return float(road_profile(Y)) + 0.2
        t = grid_mesh('rv_terrain', list(-xs), list(ys), lambda x, y: tz(-x, y), snow_m, flip=True)
        t.data.shade_smooth()
        self.temp.append(t)
        valley = self._mat('rv_valley', 0x1c2430, 0.9)
        v = grid_mesh('rv_valley', [-2000.0, -150.0], [-2000.0, 2000.0], lambda x, y: -160.0, valley, flip=True)
        self.temp.append(v)
        # The cars.
        path = os.path.join(core.EXPORT_DIR, 'SM_Sedan.glb')
        if os.path.exists(path):
            before = set(bpy.data.objects)
            # the bake's occlusion group lacks the sockets the importer's own group has: keep it out of the way
            grp = bpy.data.node_groups.get('glTF Material Output')
            if grp is not None:
                grp.name = 'glTF Material Output (bake)'
            try:
                bpy.ops.import_scene.gltf(filepath=path)
            finally:
                if grp is not None:
                    imp = bpy.data.node_groups.get('glTF Material Output')
                    if imp is not None:
                        imp.name = 'glTF Material Output (import)'
                    grp.name = 'glTF Material Output'
            car = next(o for o in bpy.data.objects if o not in before and o.type == 'MESH')
            for o in set(bpy.data.objects) - before:
                if o is not car:
                    bpy.data.objects.remove(o)
            car.parent = None
            self.temp.append(car)
            spots = [((10.4, -7.6, -0.12), 90), ((10.4, 9.0, -0.12), 90), ((40.0, -30.0, float(arm_z(40.0))), 180), ((31.0, -30.0, float(arm_z(31.0))), 180)]
            for i, (p, F) in enumerate(spots):
                c = car if i == 0 else self._dup(car)
                c.location = _bl(p)
                c.rotation_euler = (0.0, 0.0, math.radians(270.0 - F))

    # ---- preview materials (after export: the exported files keep their own)
    def preview_materials(self):
        g = self.ground
        if g is not None:
            names = ['Paver', 'Sett', 'Asphalt', 'Curb', 'Concrete', 'WallStone', None, None]
            tiles = [3.6, 2.0, 4.0, 2.0, 2.0, 3.0, 1, 1]
            for i, (nm, tl) in enumerate(zip(names, tiles)):
                if i >= len(g.material_slots):
                    break
                if nm is None:
                    m = snow('rv_snowground', 0.6, -9.0) if i == 6 else self._marking_mat()
                else:
                    m = self._ground_preview(nm, tl)
                g.material_slots[i].material = m.m if isinstance(m, core.Mat) else m
        for name, o in self.objs.items():
            if '_Glow' in name:
                colr = EMBER_LED if 'Ember' in name else (COOL_GLOW if 'Cool' in name else WARM_GLOW)
                m = self._mat('rv_' + name, colr, 0.5, colr, 3.0)
                o.data.materials[0] = m.m
                self.glows.append(m)
            elif name == 'SM_ECX_FireBowl_Flame':
                o.data.materials[0] = self._flame_mat()
            elif name == 'SM_ECX_Cascade_Water':
                water = self._mat('rv_water', 0x050b0e, 0.02)
                water.set('Specular IOR Level', 0.7)
                falls = core.Mat('rv_falls')
                falls.set('Base Color', H(0xa9c2cc))
                falls.set('Roughness', 0.15)
                falls.set('Transmission Weight', 0.6)
                o.data.materials[0] = water.m
                if len(o.data.materials) > 1:
                    o.data.materials[1] = falls.m
            elif name == 'SM_ECX_MonumentSign_Screen':
                o.data.materials[0] = self._screen_mat()

    def _ground_preview(self, nm, tile):
        base = os.path.join(TEX_DIR, f'T_ECX_{nm}')
        m = core.Mat('rv_' + nm)
        if not os.path.exists(base + '_BaseColor.png'):
            m.set('Base Color', H(0x808080))
            return m
        uv = m.node('ShaderNodeUVMap')
        uv.uv_map = 'UVMap'
        mp = m.node('ShaderNodeMapping')
        mp.inputs['Scale'].default_value = (1.0 / tile, 1.0 / tile, 1.0)
        m.link(uv.outputs['UV'], mp.inputs['Vector'])
        col = m.image(base + '_BaseColor.png', vector=mp.outputs['Vector'])
        orm = m.image(base + '_ORM.png', non_color=True, vector=mp.outputs['Vector'])
        nrm = m.image(base + '_Normal.png', non_color=True, vector=mp.outputs['Vector'])
        sep = m.node('ShaderNodeSeparateColor')
        m.link(orm.outputs['Color'], sep.inputs['Color'])
        va = m.node('ShaderNodeVertexColor')
        va.layer_name = 'Color'
        vs = m.node('ShaderNodeSeparateColor')
        m.link(va.outputs['Color'], vs.inputs['Color'])
        wet = m.math('MINIMUM', m.math('ADD', m.math('MULTIPLY', vs.outputs['Blue'], 0.8), m.math('MULTIPLY', vs.outputs['Red'], 0.6)), 1.0)
        puddle = ramp(m, vs.outputs['Red'], 0.55, 0.78)
        c = m.mix(m.math('MULTIPLY', wet, 0.55), col.outputs['Color'], cmul(m, col.outputs['Color'], 0.45))
        c = m.mix(m.math('MULTIPLY', vs.outputs['Green'], 0.6), c, H(0x6f6b66))
        m.set('Base Color', c)
        r = m.math('MULTIPLY', sep.outputs['Green'], m.math('SUBTRACT', 1.0, m.math('MULTIPLY', wet, 0.65)))
        m.set('Roughness', mix_float(m, puddle, r, 0.03))
        nm_n = m.node('ShaderNodeNormalMap')
        m.link(nrm.outputs['Color'], nm_n.inputs['Color'])
        m.link(m.math('SUBTRACT', 1.0, puddle), nm_n.inputs['Strength'])
        m.set('Normal', nm_n.outputs['Normal'])
        return m

    def _marking_mat(self):
        m = core.Mat('rv_marking')
        va = m.node('ShaderNodeVertexColor')
        va.layer_name = 'Color'
        m.link(va.outputs['Color'], m.bsdf.inputs['Base Color'])
        m.set('Roughness', 0.45)
        return m

    def _flame_mat(self):
        m = core.Mat('rv_flame')
        nt = m.nt
        uv = m.node('ShaderNodeUVMap')
        sep = m.node('ShaderNodeSeparateXYZ')
        m.link(uv.outputs['UV'], sep.inputs['Vector'])
        u, v = sep.outputs['X'], sep.outputs['Y']
        r = m.math('ABSOLUTE', m.math('MULTIPLY', m.math('SUBTRACT', u, 0.5), 2.0))
        tc = m.node('ShaderNodeTexCoord')
        wob = noise(m, tc.outputs['Object'], 3.0, 3.0)
        width = m.math('MULTIPLY', m.math('POWER', m.math('SUBTRACT', 1.0, v, clamp=True), 0.7), m.math('ADD', 0.55, m.math('MULTIPLY', wob, 0.5)))
        width = m.math('MULTIPLY', width, m.math('MINIMUM', m.math('MULTIPLY', v, 6.0), 1.0))
        shape = ramp(m, m.math('SUBTRACT', width, r), 0.0, 0.25)
        col = m.mix(ramp(m, v, 0.05, 0.7), H(0xffd27a), H(0xff4a10))
        em = m.node('ShaderNodeEmission')
        m.link(col, em.inputs['Color'])
        m.link(m.math('MULTIPLY', shape, 14.0), em.inputs['Strength'])
        tr = m.node('ShaderNodeBsdfTransparent')
        add = m.node('ShaderNodeAddShader')
        m.link(em.outputs['Emission'], add.inputs[0])
        mixs = m.node('ShaderNodeMixShader')
        m.link(shape, mixs.inputs['Fac'])
        m.link(tr.outputs['BSDF'], mixs.inputs[1])
        m.link(em.outputs['Emission'], mixs.inputs[2])
        m.link(mixs.outputs['Shader'], m.out.inputs['Surface'])
        self.flame = m
        return m.m

    def _screen_mat(self):
        s = Sheet(2048.0, 128.0)
        s.rect(0, 0, 2048, 128, 0x05070c)
        s.text('TONIGHT  ·  EMBERCREST NIGHTLY  ·  CARDS AT 7:00 PM  ·  $150  ·  20,000 CHIPS', 30, 40, 54, brand.ASH, 'Bold', 'LEFT', tracking=1.1)
        s.rect(0, 0, 18, 128, EMBER_LED)
        color, _ = s.render('ecx_review_ticker', 2048, samples=8)
        m = core.Mat('rv_screen')
        img = m.image(color)
        em = m.node('ShaderNodeEmission')
        m.link(img.outputs['Color'], em.inputs['Color'])
        em.inputs['Strength'].default_value = 3.0
        m.link(em.outputs['Emission'], m.out.inputs['Surface'])
        self.screen = m
        return m.m

    # ---- lights
    def _sun(self, name, to_sun, energy, color, angle_deg):
        d = bpy.data.lights.new(name, 'SUN')
        d.energy = energy
        d.color = color
        d.angle = math.radians(angle_deg)
        o = core.link(bpy.data.objects.new(name, d))
        o.rotation_euler = Vector(to_sun).normalized().to_track_quat('Z', 'Y').to_euler()
        self.temp.append(o)
        return o

    def _point(self, name, p, energy, color, radius=0.1):
        d = bpy.data.lights.new(name, 'POINT')
        d.energy = energy
        d.color = color
        d.shadow_soft_size = radius
        o = core.link(bpy.data.objects.new(name, d))
        o.location = _bl(p)
        self.temp.append(o)
        return o

    def _area(self, name, p, target, size, energy, color):
        d = bpy.data.lights.new(name, 'AREA')
        d.shape = 'RECTANGLE'
        d.size, d.size_y = size
        d.energy = energy
        d.color = color
        o = core.link(bpy.data.objects.new(name, d))
        o.location = _bl(p)
        o.rotation_euler = (_bl(target) - _bl(p)).to_track_quat('-Z', 'Y').to_euler()
        self.temp.append(o)
        return o

    def lights(self):
        el = math.radians(40.0)
        self.day_lights.append(self._sun('rv_sun', (0.25 * math.cos(el), -math.cos(el), math.sin(el)), 5.0, (1.0, 0.97, 0.92), 0.53))
        mz = math.radians(22.0)
        moon = (-0.819 * math.cos(mz), -0.574 * math.cos(mz), math.sin(mz))
        self.night_lights.append(self._sun('rv_moon', moon, 0.06, (0.62, 0.71, 0.88), 1.0))
        P = _placements()
        for (X, Y, Z, yaw, sy) in P['SM_ECX_LampPost']:
            z = self.ground_z(X, Y) if Z is None else Z
            self.night_lights.append(self._point('rv_lamp', (X, Y, z + 4.55), 45.0, (1.0, 0.72, 0.45), 0.12))
        for (X, Y, Z, yaw, sy) in P['SM_ECX_FireBowl']:
            self.night_lights.append(self._point('rv_fire', (X, Y, Z + 1.75), 120.0, (1.0, 0.45, 0.16), 0.3))
        for x in (4.5, 10.0, 15.5, 21.0):
            for y in (-15.0, -9.0, -3.0, 3.0, 9.0, 15.0):
                self.night_lights.append(self._area('rv_down', (x, y, 6.95), (x, y, 0.0), (0.3, 0.3), 18.0, (1.0, 0.8, 0.6)))
        self.night_lights.append(self._area('rv_lobby', (0.5, 0.0, 4.5), (20.0, 0.0, 0.0), (30.0, 9.0), 500.0, (1.0, 0.7, 0.45)))
        self.night_lights.append(self._area('rv_shelter', (50.6, -12.0, 2.55 + LEVEL_P), (50.6, -12.0, LEVEL_P), (0.2, 7.0), 25.0, (0.88, 0.92, 1.0)))
        self.night_lights.append(self._area('rv_marq', (5.0, -38.0, 8.0), (30.0, -38.0, 0.0), (16.0, 6.0), 300.0, (1.0, 0.55, 0.3)))

    def world(self, night):
        w = self.sc.world
        w.use_nodes = True
        nt = w.node_tree
        nt.nodes.clear()
        out = nt.nodes.new('ShaderNodeOutputWorld')
        bg = nt.nodes.new('ShaderNodeBackground')
        tc = nt.nodes.new('ShaderNodeTexCoord')
        sep = nt.nodes.new('ShaderNodeSeparateXYZ')
        nt.links.new(tc.outputs['Generated'], sep.inputs['Vector'])
        mr = nt.nodes.new('ShaderNodeMapRange')
        mr.inputs['From Min'].default_value = -0.02
        mr.inputs['From Max'].default_value = 0.55
        nt.links.new(sep.outputs['Z'], mr.inputs['Value'])
        mx = nt.nodes.new('ShaderNodeMix')
        mx.data_type = 'RGBA'
        nt.links.new(mr.outputs['Result'], mx.inputs[0])
        if night:
            mx.inputs[6].default_value = H(0x1a2238)
            mx.inputs[7].default_value = H(0x03050b)
            bg.inputs['Strength'].default_value = 0.15
        else:
            mx.inputs[6].default_value = H(0xc9dcf0)
            mx.inputs[7].default_value = H(0x2f6cc4)
            bg.inputs['Strength'].default_value = 1.0
        nt.links.new(mx.outputs[2], bg.inputs['Color'])
        nt.links.new(bg.outputs['Background'], out.inputs['Surface'])

    def set_time(self, night):
        self.world(night)
        for l in self.day_lights:
            l.hide_render = night
        for l in self.night_lights:
            l.hide_render = not night
        k = 4.0 if night else 1.2
        for m in self.glows:
            m.bsdf.inputs['Emission Strength'].default_value = k
        for m, day, nite in ((self.glass, 0.6, 0.7), (self.soffit, 0.0, 0.35), (self.crown, 0.4, 3.0), (self.tower_win, 0.05, 0.15),
                             (self.marquee, 2.0, 1.2)):
            m.bsdf.inputs['Emission Strength'].default_value = nite if night else day
        self.sc.view_settings.view_transform = 'AgX'
        try:
            self.sc.view_settings.look = 'AgX - Punchy' if not night else 'None'
        except TypeError:
            pass
        self.sc.view_settings.exposure = 2.2 if night else -0.1
        self.scale_lit(1.0 if night else 0.2)

    def scale_lit(self, factor):
        """Scales every baked [lit] slot as DimImportedGlow will (0.2 by day, 1 at night)."""
        if not hasattr(self, '_lit0'):
            self._lit0 = {}
            for o in list(self.objs.values()) + list(getattr(self, 'context_objs', [])):
                name = o.name
                if name in UNBAKED or '_Glow' in name:
                    continue
                for s in o.material_slots:
                    m = s.material
                    if m is None or not m.node_tree:
                        continue
                    b = next((n for n in m.node_tree.nodes if n.type == 'BSDF_PRINCIPLED'), None)
                    if b is not None and b.inputs['Emission Color'].is_linked and b.inputs['Emission Strength'].default_value > 0:
                        self._lit0[m.name] = (b, b.inputs['Emission Strength'].default_value)
        for b, s0 in self._lit0.values():
            b.inputs['Emission Strength'].default_value = s0 * factor

    # ---- rendering
    def shot(self, name, cam_p, look_p, fov, w=1280, h=720, samples=80):
        cam = self.sc.camera
        if cam is None or cam.name != 'rv_cam':
            cd = bpy.data.cameras.new('rv_cam')
            cam = core.link(bpy.data.objects.new('rv_cam', cd))
            self.temp.append(cam)
            self.sc.camera = cam
        cam.data.sensor_fit = 'HORIZONTAL'
        cam.data.angle = math.radians(fov)
        cam.data.clip_start = 0.05
        cam.data.clip_end = 5000.0
        cam.location = _bl(cam_p)
        cam.rotation_euler = (_bl(look_p) - _bl(cam_p)).to_track_quat('-Z', 'Y').to_euler()
        sc = self.sc
        sc.render.resolution_x = w
        sc.render.resolution_y = h
        sc.render.resolution_percentage = 100
        sc.cycles.samples = int(os.environ.get('ECX_REVIEW_SAMPLES', samples))
        sc.cycles.use_denoising = True
        sc.cycles.adaptive_threshold = 0.03
        sc.render.film_transparent = False
        path = os.path.join(self.dir, f'{name}.png')
        sc.render.filepath = path
        sc.render.image_settings.file_format = 'PNG'
        sc.render.image_settings.color_mode = 'RGB'
        bpy.ops.render.render(write_still=True)
        return path

    def cleanup(self):
        for o in self.temp:
            try:
                bpy.data.objects.remove(o)
            except ReferenceError:
                pass
        self.temp = []
        for name, (loc, rot, scl) in self.saved.items():
            o = self.objs[name]
            o.location, o.rotation_euler, o.scale = loc, rot, scl
        self.sc.view_settings.exposure = 0.0
        try:
            self.sc.view_settings.look = 'None'
        except TypeError:
            pass
        # back to the plain studio world the standard sheet sets by color
        w = self.sc.world
        if w.node_tree is not None:
            nt = w.node_tree
            nt.nodes.clear()
            out = nt.nodes.new('ShaderNodeOutputWorld')
            bg = nt.nodes.new('ShaderNodeBackground')
            bg.inputs['Color'].default_value = (0.006, 0.0065, 0.008, 1.0)
            nt.links.new(bg.outputs['Background'], out.inputs['Surface'])


def compose_grid(name, paths, cols, label=None):
    """Lays renders (all the same size) out in a grid as art/review/<name>.jpg."""
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
        out[y0:y0 + h, x0:x0 + w] = im[:, :, :4] if im.shape[2] == 4 else np.dstack([im, np.ones((h, w))])
    os.makedirs(core.REVIEW_DIR, exist_ok=True)
    path = os.path.join(core.REVIEW_DIR, f'{name}.jpg')
    img = bpy.data.images.new(f'rv_{name}', W, Hh, alpha=False)
    img.pixels.foreach_set(out.ravel())
    img.filepath_raw = path
    img.file_format = 'JPEG'
    img.save(quality=90)
    bpy.data.images.remove(img)
    print(f'[ecx_plaza] review sheet {os.path.relpath(path, core.ROOT)}')
    return path


def texture_sheet():
    """BaseColor of every ground set tiled 2 x 2 (seams would show as lines across the middle)."""
    names = ['Paver', 'Sett', 'Asphalt', 'Concrete', 'Curb', 'WallStone']
    tiles = []
    d = os.path.join(core.BUILD_DIR, 'review', 'ecx_plaza')
    os.makedirs(d, exist_ok=True)
    for nm in names:
        p = os.path.join(TEX_DIR, f'T_ECX_{nm}_BaseColor.png')
        q = os.path.join(TEX_DIR, f'T_ECX_{nm}_Normal.png')
        if not os.path.exists(p):
            return None
        a = review._pixels(p)[:, :, :3]
        nrm = review._pixels(q)[:, :, :3]
        lit = np.clip((nrm[..., 0] - 0.5) * 1.2 + (nrm[..., 1] - 0.5) * 1.2 + 0.85, 0.3, 1.4)[..., None]
        a = np.clip(a * lit, 0, 1)
        t = np.tile(a, (2, 2, 1))
        f = t.shape[0] // 768
        t = t[:f * 768, :f * 768].reshape(768, f, 768, f, 3).mean(axis=(1, 3))
        img = bpy.data.images.new('rv_tex', 768, 768, alpha=False)
        img.pixels.foreach_set(np.dstack([t, np.ones((768, 768))]).astype(np.float32).ravel())
        path = os.path.join(d, f'tex_{nm}.png')
        img.filepath_raw = path
        img.file_format = 'PNG'
        img.save()
        bpy.data.images.remove(img)
        tiles.append(path)
    return compose_grid('ecx_plaza_textures', tiles, 3)


def _which_reviews():
    v = os.environ.get('ECX_REVIEW', 'site,props,signs,tex')
    return {s.strip() for s in v.split(',') if s.strip()}


SITE_SHOTS = [
    # name, camera (map m), look-at, fov, night?
    ('reveal', (54.2, 3.4, -0.45), (0.0, 1.0, 7.4), 75.0),
    ('door', (2.6, 0.0, 1.75), (60.0, 0.0, -1.6), 75.0),
    ('crane', (92.0, -46.0, 24.0), (26.0, -2.0, -1.5), 50.0),
    ('midwalk', (30.0, -2.5, 0.9), (0.0, 0.0, 9.0), 72.0),
    ('lowfire', (43.0, 6.5, -1.1), (41.0, 9.75, -0.2), 50.0),
    ('monument', (54.0, -23.6, -0.35), (45.5, -20.5, 0.7), 46.0),
    ('shelter', (52.1, -4.6, -0.6), (50.3, -12.6, -1.05), 60.0),
    ('overlook', (58.5, 4.0, -0.5), (67.0, 13.0, -1.6), 62.0),
    ('drive', (14.0, -36.0, 1.5), (30.0, -8.0, -1.2), 70.0),
    ('cascade', (33.6, 3.0, 0.15), (38.5, 10.0, -1.0), 62.0),
]


def site_review(objs):
    which = _which_reviews()
    if 'tex' in which:
        try:
            texture_sheet()
        except Exception:
            traceback.print_exc()
    if 'site' not in which:
        return
    R = _Review(objs)
    try:
        R.place_all()
        R.context()
        R.preview_materials()
        R.lights()
        shots = {s[0]: s for s in SITE_SHOTS}
        day = ['reveal', 'crane', 'cascade', 'midwalk', 'monument', 'shelter', 'drive', 'overlook', 'lowfire']
        night = ['reveal', 'door', 'crane', 'cascade', 'lowfire', 'monument', 'shelter', 'overlook', 'drive']
        for label, names, nite in (('day', day, False), ('night', night, True)):
            R.set_time(nite)
            paths = []
            for nm in names:
                _, cp, lp, fov = shots[nm]
                paths.append(R.shot(f'{label}_{nm}', cp, lp, fov, 1120, 630, 48))
            compose_grid(f'ecx_plaza_site_{label}', paths, 3)
    except Exception:
        traceback.print_exc()
    finally:
        R.cleanup()


def pose_for_review(objs):
    """Renders the plaza sheets (art/review/ecx_plaza_site_day/night, _props, _signs, _textures), then lines the meshes
    up for the standard sheet."""
    global REVIEW_VIEWS
    for o in objs:
        o.hide_render = False
    t0 = time.time()
    site_review(objs)
    if 'props' in _which_reviews() or 'signs' in _which_reviews():
        try:
            props_review(objs)
        except Exception:
            traceback.print_exc()
    print(f'[ecx_plaza] review renders in {time.time() - t0:.0f}s')
    # The standard sheet: the meshes in rows, small props in front, the big ones behind, the SITE meshes shrunk at the
    # back (the plaza sheets above are the real review).
    rows = [['SM_ECX_Bollard', 'SM_ECX_AshUrn', 'SM_ECX_Viewer', 'SM_ECX_ValetPodium', 'SM_ECX_BusStopSign', 'SM_ECX_LampPost', 'SM_ECX_FireBowl',
             'SM_ECX_Bench', 'SM_ECX_LuggageCart', 'SM_ECX_Planter'],
            ['SM_ECX_Steps', 'SM_ECX_BusShelter', 'SM_ECX_ViewRail'],
            ['SM_ECX_MonumentSign', 'SM_ECX_SnowBank_B', 'SM_ECX_SnowBank_C'],
            ['SM_ECX_SnowBank_A', 'SM_ECX_Cascade', 'SM_ECX_Ground']]
    shrink = {'SM_ECX_Cascade': 0.35, 'SM_ECX_Ground': 0.05}
    follow = {'SM_ECX_Steps_GlowWarm': 'SM_ECX_Steps', 'SM_ECX_FireBowl_Flame': 'SM_ECX_FireBowl', 'SM_ECX_MonumentSign_GlowEmber': 'SM_ECX_MonumentSign',
              'SM_ECX_MonumentSign_Screen': 'SM_ECX_MonumentSign', 'SM_ECX_BusShelter_GlowCool': 'SM_ECX_BusShelter',
              'SM_ECX_LampPost_GlowWarm': 'SM_ECX_LampPost', 'SM_ECX_Bollard_GlowEmber': 'SM_ECX_Bollard', 'SM_ECX_Cascade_Water': 'SM_ECX_Cascade'}
    by = {o.name: o for o in objs}
    x_row = 0.0
    spots = {}
    for row in rows:
        names = [n for n in row if n in by]
        if not names:
            continue
        for n in names:
            o = by[n]
            s = shrink.get(n, 1.0)
            o.scale = (s, s, s)
            o.location = (0.0, 0.0, 0.0)
        bpy.context.view_layer.update()
        ext = [core.bounds([by[n]]) for n in names]
        width = sum(hi.y - lo.y for lo, hi in ext) + 0.8 * (len(names) - 1)
        depth = max(hi.x - lo.x for lo, hi in ext)
        y = -width / 2
        for n, (lo, hi) in zip(names, ext):
            o = by[n]
            o.location = (x_row - lo.x, y - lo.y, -lo.z if n in shrink else 0.0)
            spots[n] = (x_row, y + (hi.y - lo.y) / 2)
            y += (hi.y - lo.y) + 0.8
        x_row += depth + 1.5
    for k, v in follow.items():
        if k in by and v in by:
            by[k].location = by[v].location
            by[k].scale = by[v].scale
    bpy.context.view_layer.update()
    lo, hi = core.bounds(objs)
    size = max((hi - lo).length, 1.0)
    first = [n for n in rows[0] if n in by]
    w0 = 0.0
    if first:
        a = core.bounds([by[n] for n in first])
        w0 = a[1].y - a[0].y
    my = spots.get('SM_ECX_MonumentSign', (0.0, 0.0))
    REVIEW_VIEWS = [('front', -78, 16, max(hi.y - lo.y, 8.0) * 2.6 / size, ((lo.x + hi.x) / 2, (lo.y + hi.y) / 2, 1.5)),
                    ('props', -80, 10, max(w0, 4.0) * 2.5 / size, (0.6, 0.0, 0.9)),
                    ('monument', -88, 6, 22.0 / size, (my[0] + 0.6, my[1], 2.2))]


# Close looks: (label, meshes shown, azimuth from the front (deg, + toward +y), elevation, distance factor, target offset, fov)
PROP_SHOTS = [
    ('steps', ['SM_ECX_Steps', 'SM_ECX_Steps_GlowWarm'], 35, 22, 0.9, (0.8, 1.5, 0.3), 50),
    ('steps_close', ['SM_ECX_Steps', 'SM_ECX_Steps_GlowWarm'], 20, 12, 0.22, (0.4, 2.6, 0.35), 50),
    ('firebowl', ['SM_ECX_FireBowl', 'SM_ECX_FireBowl_Flame'], 30, 25, 1.25, (0.0, 0.0, 0.9), 45),
    ('firebowl_top', ['SM_ECX_FireBowl'], 20, 55, 0.75, (0.0, 0.0, 1.2), 45),
    ('monument', ['SM_ECX_MonumentSign', 'SM_ECX_MonumentSign_GlowEmber', 'SM_ECX_MonumentSign_Screen'], 25, 8, 1.15, (0.0, 0.0, 2.1), 45),
    ('monument_back', ['SM_ECX_MonumentSign', 'SM_ECX_MonumentSign_GlowEmber', 'SM_ECX_MonumentSign_Screen'], 200, 8, 1.15, (0.0, 0.0, 2.1), 45),
    ('shelter', ['SM_ECX_BusShelter', 'SM_ECX_BusShelter_GlowCool'], 35, 12, 1.2, (0.0, 0.0, 1.4), 50),
    ('shelter_in', ['SM_ECX_BusShelter', 'SM_ECX_BusShelter_GlowCool'], 70, 5, 0.6, (0.3, -2.0, 1.3), 60),
    ('stopsign', ['SM_ECX_BusStopSign'], 25, 8, 1.3, (0.0, -0.35, 2.0), 40),
    ('lamppost', ['SM_ECX_LampPost', 'SM_ECX_LampPost_GlowWarm'], 30, 8, 1.2, (0.0, 0.0, 2.6), 45),
    ('lantern', ['SM_ECX_LampPost', 'SM_ECX_LampPost_GlowWarm'], 30, 10, 0.35, (0.0, 0.0, 4.55), 45),
    ('bollard', ['SM_ECX_Bollard', 'SM_ECX_Bollard_GlowEmber'], 30, 18, 2.2, (0.0, 0.0, 0.5), 40),
    ('planter', ['SM_ECX_Planter'], 35, 30, 1.3, (0.0, 0.0, 0.6), 45),
    ('bench', ['SM_ECX_Bench'], 40, 25, 1.4, (0.0, 0.0, 0.3), 45),
    ('valet', ['SM_ECX_ValetPodium'], 30, 15, 1.6, (0.0, 0.0, 0.7), 45),
    ('valet_back', ['SM_ECX_ValetPodium'], 160, 15, 1.6, (0.0, 0.0, 0.7), 45),
    ('cart', ['SM_ECX_LuggageCart'], 35, 15, 1.3, (0.0, 0.0, 1.0), 45),
    ('ashurn', ['SM_ECX_AshUrn'], 30, 35, 2.4, (0.0, 0.0, 0.45), 40),
    ('snowbank_a', ['SM_ECX_SnowBank_A'], 35, 22, 0.8, (0.0, 0.0, 0.3), 50),
    ('snowbank_b', ['SM_ECX_SnowBank_B'], 35, 25, 1.0, (0.0, 0.0, 0.5), 50),
    ('snowbank_c', ['SM_ECX_SnowBank_C'], 35, 25, 0.9, (0.0, 0.0, 0.2), 50),
    ('viewrail', ['SM_ECX_ViewRail'], 30, 10, 0.9, (0.0, 0.0, 0.2), 50),
    ('viewrail_out', ['SM_ECX_ViewRail'], 150, 0, 0.9, (0.0, 0.0, -0.8), 50),
    ('viewer', ['SM_ECX_Viewer'], 35, 12, 1.5, (0.0, 0.0, 1.0), 45),
]

# Straight-on looks at the prints: (label, mesh, camera (Blender, in the mesh's frame), target, fov)
SIGN_SHOTS = [
    ('mon_front', 'SM_ECX_MonumentSign', (-11.0, 0.0, 2.2), (0.0, 0.0, 2.2), 46),
    ('mon_back', 'SM_ECX_MonumentSign', (11.0, 0.0, 2.2), (0.0, 0.0, 2.2), 46),
    ('mon_strip', 'SM_ECX_MonumentSign', (-4.0, 0.0, 0.9), (0.0, 0.0, 0.9), 90),
    ('shelter_map', 'SM_ECX_BusShelter', (0.15, 0.5, 1.45), (0.15, -4.0, 1.45), 45),
    ('shelter_ad', 'SM_ECX_BusShelter', (-3.0, 0.0, 1.55), (0.7, 0.0, 1.55), 40),
    ('shelter_time', 'SM_ECX_BusShelter', (0.2, 6.5, 1.5), (0.2, 4.0, 1.5), 40),
    ('stop_front', 'SM_ECX_BusStopSign', (-3.5, -0.36, 2.75), (0.0, -0.36, 2.75), 22),
    ('stop_back', 'SM_ECX_BusStopSign', (3.5, -0.36, 2.75), (0.0, -0.36, 2.75), 22),
    ('valet_plaque', 'SM_ECX_ValetPodium', (-2.2, 0.0, 0.85), (0.0, 0.0, 0.85), 30),
    ('bus_stencil', 'SM_ECX_Ground', (-54.3, -13.0, 9.0), (-54.3, -12.99, -2.4), 30),
]


def props_review(objs):
    """Close looks at the props (a sunny studio, and the same at night with their glows on) and the prints read
    straight on (art/review/ecx_plaza_props_day, _props_night, _signs)."""
    which = _which_reviews()
    sc = bpy.context.scene
    by = {o.name: o for o in objs}
    saved = {o.name: o.hide_render for o in objs}
    R = _Review(objs)
    temp = []
    try:
        R.preview_materials()
        floor = grid_mesh('rv_floor', [-40.0, 40.0], [-40.0, 40.0], lambda x, y: -0.002, R._mat('rv_floor_m', 0x5c5d60, 0.6))
        temp.append(floor)
        if 'SM_ECX_Ground' in by:
            by['SM_ECX_Ground'].hide_render = True
        sun = R._sun('rv_psun', (-0.55, -0.45, 0.7), 3.5, (1.0, 0.97, 0.92), 1.0)
        fill = R._area('rv_pfill', (-6.0, 6.0, 3.0), (0.0, 0.0, 1.0), (6.0, 6.0), 400.0, (0.75, 0.82, 1.0))
        warm = R._area('rv_pwarm', (-3.0, -3.0, 4.0), (0.0, 0.0, 0.8), (1.0, 1.0), 260.0, (1.0, 0.7, 0.42))
        rim = R._sun('rv_prim', (0.7, 0.3, 0.35), 0.05, (0.62, 0.71, 0.88), 2.0)
        R.temp += []
        temp += [sun, fill, warm, rim]
        R.glass = R._mat('rv_lobby_p', 0, 0.5)
        R.soffit = R.crown = R.tower_win = R.marquee = R.glass
        if 'props' in which:
            for night in (False, True):
                R.world(night)
                sun.hide_render = fill.hide_render = night
                warm.hide_render = rim.hide_render = not night
                for m in R.glows:
                    m.bsdf.inputs['Emission Strength'].default_value = 5.0 if night else 1.0
                sc.view_settings.exposure = 1.6 if night else -0.3
                R.scale_lit(1.0 if night else 0.2)
                paths = []
                for label, names, az, el, dist, tgt, fov in PROP_SHOTS:
                    show = [by[n] for n in names if n in by]
                    if not show:
                        continue
                    for o in objs:
                        o.hide_render = o not in show
                    lo, hi = core.bounds([show[0]])
                    size = max((hi - lo).length, 0.5)
                    t = Vector(tgt)
                    a, e = math.radians(az), math.radians(el)
                    camp = t + Vector((-math.cos(a) * math.cos(e), math.sin(a) * math.cos(e), math.sin(e))) * size * dist
                    paths.append(R.shot(f'p_{"n" if night else "d"}_{label}', (-camp.x, camp.y, camp.z), (-t.x, t.y, t.z), fov, 900, 900, 48))
                if paths:
                    compose_grid('ecx_plaza_props_' + ('night' if night else 'day'), paths, 4)
        if 'signs' in which:
            R.world(False)
            sun.hide_render = fill.hide_render = False
            warm.hide_render = rim.hide_render = True
            sc.view_settings.exposure = -0.3
            R.scale_lit(0.25)
            paths = []
            for label, name, cam, tgt, fov in SIGN_SHOTS:
                if name not in by:
                    continue
                for o in objs:
                    o.hide_render = o.name not in (name, name + '_Screen', name + '_GlowEmber')
                floor.hide_render = name == 'SM_ECX_Ground'
                c = Vector(cam)
                t = Vector(tgt)
                paths.append(R.shot(f's_{label}', (-c.x, c.y, c.z), (-t.x, t.y, t.z), fov, 1200, 675, 32))
            if paths:
                compose_grid('ecx_plaza_signs', paths, 2)
    finally:
        for o in temp:
            try:
                bpy.data.objects.remove(o)
            except ReferenceError:
                pass
        R.cleanup()
        for n, h in saved.items():
            by[n].hide_render = h
        sc.view_settings.exposure = 0.0
