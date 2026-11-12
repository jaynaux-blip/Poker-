"""The eight festival trophies for the apartment's shelf (Update 4 G1, F-look FL-8b): SPEC §2.7.5's trophies as Blender
pieces (§9.2: "every trophy is a Blender piece"), on fest_shelf.py's shared finishes, slots and checks.

- SM_FS_Cup, the RCOP Cup: a tall silver cup on a stepped violet-enamel foot, a violet band under its lip, two loop handles.
  UV1 ("Decal", the glb's TEXCOORD_1) maps the bowl's front for the engraved lines the game draws, one per edition.
- SM_FS_Mirror, The Funhouse Mirror (FEIGN): a gold hand mirror with a scalloped frame, standing in an ebonized foot; its
  glass is gently warped (a funhouse mirror) and UV1 maps it for the champion's avatar colours the game draws.
- SM_FS_Button, The Big Red Button (PUSH): a chrome bezel and a glossy red dome on a pink-and-mint pedestal, a chrome ring
  between the two tones.
- SM_FS_Seal, The Seal (WRIT): a chrome wax-seal stamp with a red-lacquer collar on an onyx block; a chrome seal face on the
  block's front, which UV1 maps for the red RIT monogram the game draws (SPEC §9.2: "a decal drawn by our code").
- SM_FS_Grindstone, The Grindstone (GRIT): a chrome grinding wheel with a red-lacquer rim band on an onyx trestle (two
  A-frames on an onyx base), a chrome axle and a crank with a red grip.
- SM_FS_SlamPlate, The Slam Plate (Summer Slam): a silver salver with a sunburst of gilded rays round a boss, standing
  tilted back in a brass cradle on a turned brass column and a walnut foot.
- SM_FS_Pennant, The Gold Pennant (PENNANT, v1.6): a gold-felt pennant with a little cloth curl on a brass staff with a
  spade finial, standing in a pier-plank base of walnut; no bulbs. UV1 maps the pennant's front for the champion's
  handle the game stitches in coral and Outfield Green.
- SM_FS_Sceptre, The Sceptre (REIGN, v1.6): a chrome sceptre with an onyx shaft, a red-lacquer collar and a crossed
  spade finial, upright in a chrome socket on a stepped onyx block. No pink, mint or light tube, and no laurel.

Sizes: each stands on a plinth (its origin the middle of its footprint on the plinth's top, front toward -Y, Z up). The
festival trophies are FT_H tall; a championship's trophy (the RCOP Cup, The Gold Pennant, The Sceptre: fest::IsChampionship)
is FT_CHAMP = 1.15x its site's other festival trophies (SPEC §2.7.5, v1.6): RiverLine's Cup against The Slam Plate (the
ring and the bracelet stand in FL-8a's cases), Ignite's Pennant against the Mirror and the Button, RunItTwice's Sceptre
against the Seal and the Grindstone. Every piece fits under the shelf's CLEARANCE and inside the plinth's cap.

Colours are fixed per trophy (no game tint, no FS_Tint): Ignite's house pink and mint on the Button (SPEC §2.7.5),
RunItTwice's chrome, onyx and red on the Seal, the Grindstone and the Sceptre. No logo, text or font anywhere: the
monogram, the engraving, the avatar colours and the stitched handle are the game's decals on UV1.

Checks (each asserts before any bake or export, beside fest_shelf's FsTriangles, FsTextures and FsSlots, which count the
real trophies into the shelf's 40k and print the draw-call estimate with them):
  FsFit       every trophy stands on z = 0, is at most CLEARANCE tall and its footprint, centred on its origin, is at
              most FT_FOOT on each axis (the plinth's cap is 0.164).
  FsChampion  per site, the other festival trophies share one height and the championship's trophy is FT_CHAMP times it
              (within 0.5%).
"""
import math

import bpy  # noqa: F401,I001
import bmesh
import numpy as np
from mathutils import Matrix, Vector

from artkit import core, parts
from artkit.shade import mix_float, ramp
from assets import fest_shelf as kit

FT_H = 0.25                          # a festival trophy (not a championship's)
FT_CHAMP = 1.15                      # a championship's trophy, against its site's others (SPEC §2.7.5, v1.6)
FT_CHAMP_H = FT_H * FT_CHAMP         # 0.2875
FT_FOOT = 0.16                       # inside the plinth's cap (0.164)
_FT_DRAWN, _FT_DRAWN_CHAMP = 0.25, 0.2875   # the heights each builder draws at, stretched to FT_HEIGHT's (_ft_scale_z)
FT_HEIGHT = {'SM_FS_Cup': FT_CHAMP_H, 'SM_FS_Mirror': FT_H, 'SM_FS_Button': FT_H, 'SM_FS_Seal': FT_H, 'SM_FS_Grindstone': FT_H,
             'SM_FS_SlamPlate': FT_H, 'SM_FS_Pennant': FT_CHAMP_H, 'SM_FS_Sceptre': FT_CHAMP_H}
FT_SITES = [('RiverLine', 'SM_FS_Cup', ['SM_FS_SlamPlate']), ('Ignite', 'SM_FS_Pennant', ['SM_FS_Mirror', 'SM_FS_Button']),
            ('RunItTwice', 'SM_FS_Sceptre', ['SM_FS_Seal', 'SM_FS_Grindstone'])]
FT_TOL = 0.005                       # FsChampion's tolerance on the ratio (0.5%) and on the others' heights (1%: twice it)

FT_VIOLET = 0x8a6ee6                 # RCOP's Floodlight Violet, as an enamel
FT_PINK, FT_MINT = 0xff4fa3, 0x5ff2c9   # Ignite's house pair (The Big Red Button's pedestal)
FT_BUTTON_RED = 0xe3141c
FT_LACQUER = 0xc8121a                # RunItTwice's Signal Red as a lacquer

CUP_SEGMENTS = 24
SPADE_N = 32


# ================================================================== helpers

def _ft_scale_z(objs, s):
    """Stretches built parts along Z about the plinth's top (a piece drawn at its nominal height, made h tall)."""
    if abs(s - 1.0) > 1e-9:
        for o in objs:
            o.data.transform(Matrix.Scale(s, 4, (0.0, 0.0, 1.0)))
    return objs


def _ft_outer(r_min):
    """A test for a lathe's outer faces (away from the axis, beyond r_min)."""
    def test(p, me):
        rad = math.hypot(p.center.x, p.center.y)
        return rad > r_min and (p.normal.x * p.center.x + p.normal.y * p.center.y) > 0.0
    return test


def _ft_heart():
    pts = []
    for k in range(SPADE_N):
        t = 2 * math.pi * k / SPADE_N
        pts.append((16 * math.sin(t) ** 3, -(13 * math.cos(t) - 5 * math.cos(2 * t) - 2 * math.cos(3 * t) - math.cos(4 * t))))
    return pts


def ft_spade(name, w, h, z0, t, at=(0.0, 0.0), turn=False):
    """A spade finial: an upturned heart (its point at the top, its lobes below) on a short flared stem, w wide and h tall
    from z0, t thick, facing -Y (turn: facing +X). The stem is thinner, so its faces stay behind the body's."""
    body = _ft_heart()
    ylo, yhi = min(p[1] for p in body), max(p[1] for p in body)
    zb = z0 + 0.22 * h
    sx, sz = w / 32.0, (z0 + h - zb) / (yhi - ylo)
    outer = [(x * sx, zb + (y - ylo) * sz) for x, y in body]
    notch = zb + (-5.0 - ylo) * sz
    stem = [(-0.18 * w, z0), (0.18 * w, z0), (0.06 * w, notch + 0.1 * h), (-0.06 * w, notch + 0.1 * h)]
    m = Matrix.Translation((at[0], at[1], 0.0)) @ (parts.rot_z(90) if turn else Matrix()) @ parts.rot_x(90)
    return [parts.outline_prism(name, outer, [], -t / 2, t / 2, m), parts.outline_prism(name + '_stem', stem, [], -t * 0.35, t * 0.35, m)]


def ft_flag(name, x0, z_lo, z_hi, length, tip_z, t, nu=20, nv=6, curl=0.007):
    """A felt pennant with thickness t: its hoist at x0 from z_lo to z_hi, flying length along +X to a 3 mm tip at tip_z,
    with a little cloth curl (a wave across Y growing toward the tip)."""
    bm = bmesh.new()

    def mid(u, v):
        top = z_hi + (tip_z + 0.0015 - z_hi) * u
        bot = z_lo + (tip_z - 0.0015 - z_lo) * u
        y = -curl * math.sin(math.pi * 1.5 * u) * u ** 0.8 - 0.0015 * math.sin(math.pi * v) * u
        return Vector((x0 + length * u, y, bot + (top - bot) * v))
    front = [[bm.verts.new(mid(i / nu, j / nv) + Vector((0.0, -t / 2, 0.0))) for j in range(nv + 1)] for i in range(nu + 1)]
    back = [[bm.verts.new(mid(i / nu, j / nv) + Vector((0.0, t / 2, 0.0))) for j in range(nv + 1)] for i in range(nu + 1)]
    for i in range(nu):
        for j in range(nv):
            bm.faces.new((front[i][j], front[i + 1][j], front[i + 1][j + 1], front[i][j + 1]))
            bm.faces.new((back[i][j], back[i][j + 1], back[i + 1][j + 1], back[i + 1][j]))
        bm.faces.new((front[i][0], back[i][0], back[i + 1][0], front[i + 1][0]))
        bm.faces.new((front[i][nv], front[i + 1][nv], back[i + 1][nv], back[i][nv]))
    for j in range(nv):
        bm.faces.new((front[0][j], front[0][j + 1], back[0][j + 1], back[0][j]))
        bm.faces.new((front[nu][j], back[nu][j], back[nu][j + 1], front[nu][j + 1]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    return core.mesh_object(name, bm)


def ft_glass(name, a, b, zc, y, rings=5, n=40, warp=0.0024):
    """A mirror's glass, an ellipse (a across, b up) at y facing -Y, gently warped like a funhouse mirror: bulged toward the
    viewer with a slow ripple, flat at its rim (which tucks under the frame)."""
    bm = bmesh.new()

    def pt(rr, th):
        x, zr = a * rr * math.cos(th), b * rr * math.sin(th)
        dy = -warp * (1.0 - rr * rr) * (0.65 + 0.35 * math.sin(3.2 * zr / b + 1.3 * x / a))
        return (x, y + dy, zc + zr)
    centre = bm.verts.new(pt(0.0, 0.0))
    ring = [[bm.verts.new(pt(k / rings, 2 * math.pi * i / n)) for i in range(n)] for k in range(1, rings + 1)]
    for i in range(n):
        bm.faces.new((centre, ring[0][i], ring[0][(i + 1) % n]))
    for k in range(rings - 1):
        for i in range(n):
            bm.faces.new((ring[k][i], ring[k + 1][i], ring[k + 1][(i + 1) % n], ring[k][(i + 1) % n]))
    obj = core.mesh_object(name, bm)
    core.orient_normals(obj, (0.0, -1.0, 0.0))
    return obj


def ft_sunburst(cz, tilt, rays=16, r0=0.013, r1=0.06):
    """The salver's face: a silver field with a sunburst of gilded rays narrowing outward from a gilded boss, in the plate's
    plane (centred at x = 0, z = cz, tilted back tilt degrees), the rays raised a touch."""
    silver, gold = kit.fs_chrome(0.12, 0xd4d7dc), kit.fs_metal('gold', 0.16)
    k = 1.0 / math.cos(math.radians(tilt))

    def surface(m, sep, tag):
        a, g = silver(m, sep, tag), gold(m, sep, tag)
        u = sep.outputs['X']
        v = m.math('MULTIPLY', m.math('SUBTRACT', sep.outputs['Z'], cz), k)
        ang = m.math('ARCTAN2', v, u)
        rad = m.math('SQRT', m.math('ADD', m.math('MULTIPLY', u, u), m.math('MULTIPLY', v, v)))
        thr = m.math('ADD', -0.6, m.math('MULTIPLY', m.math('SUBTRACT', rad, r0), 1.58 / (r1 - r0)))   # wide at the boss, to points
        ray = ramp(m, m.math('SUBTRACT', m.math('SINE', m.math('MULTIPLY', ang, float(rays))), thr), 0.0, 0.06)
        band = m.math('MULTIPLY', ramp(m, rad, r0 - 0.003, r0 - 0.001), m.math('SUBTRACT', 1.0, ramp(m, rad, r1 - 0.003, r1)))
        w = m.math('MAXIMUM', m.math('MULTIPLY', ray, band), m.math('SUBTRACT', 1.0, ramp(m, rad, r0 - 0.002, r0)))
        return {'color': m.mix(w, a['color'], g['color']), 'rough': mix_float(m, w, a['rough'], g['rough']), 'metal': 1.0,
                'height': m.math('MULTIPLY', w, 0.00015)}
    return surface


# ================================================================== the trophies

def cup(h):
    """The RCOP Cup (drawn 0.2875 tall)."""
    mat = kit.fs_material('fs_cup', [kit.fs_chrome(0.11, 0xd6d9df), kit.fs_flat(FT_VIOLET, 0.16)])
    seg = CUP_SEGMENTS
    out = [kit.fs_tag(kit.fs_lathe('foot', [(0.0, 0.0), (0.060, 0.0), (0.060, 0.016), (0.056, 0.020), (0.050, 0.020), (0.0, 0.020)],
                                   segments=seg), 1),
           kit.fs_tag(kit.fs_lathe('ring', [(0.0, 0.019), (0.0515, 0.019), (0.0525, 0.0215), (0.0515, 0.024), (0.0, 0.024)], segments=seg), 0),
           kit.fs_tag(kit.fs_lathe('step', [(0.0, 0.023), (0.046, 0.023), (0.046, 0.036), (0.042, 0.040), (0.0, 0.040)], segments=seg), 1),
           kit.fs_tag(kit.fs_lathe('stem', [(0.0, 0.039), (0.030, 0.039), (0.026, 0.044), (0.012, 0.050), (0.0085, 0.062), (0.0085, 0.080),
                                            (0.016, 0.086), (0.0085, 0.092), (0.0085, 0.104), (0.014, 0.112), (0.022, 0.118), (0.0, 0.118)],
                                   segments=seg), 0)]
    bowl = kit.fs_lathe('bowl', [(0.0, 0.114), (0.024, 0.116), (0.034, 0.124), (0.042, 0.138), (0.047, 0.158), (0.049, 0.180),
                                 (0.050, 0.205), (0.051, 0.230), (0.0525, 0.248), (0.0535, 0.256), (0.0552, 0.264), (0.0570, 0.270),
                                 (0.0585, 0.2825), (0.0595, 0.2875), (0.0565, 0.2875), (0.054, 0.272), (0.050, 0.250), (0.047, 0.210),
                                 (0.044, 0.170), (0.0, 0.150)], segments=seg)
    kit.fs_tag(bowl, 0)
    outer = _ft_outer(0.0)
    kit.fs_tag_faces(bowl, lambda p, me: 0.2555 < p.center.z < 0.2705 and outer(p, me), 1)   # the violet band under the lip
    out.append(bowl)
    for sx in (-1, 1):   # the loop handles
        loop = [(sx * 0.0485, 0.0, 0.236), (sx * 0.066, 0.0, 0.232), (sx * 0.072, 0.0, 0.212), (sx * 0.066, 0.0, 0.186), (sx * 0.0455, 0.0, 0.168)]
        out.append(kit.fs_tag(core.sweep('handle', core.catmull_rom(loop, 5), core.rounded_rect(-0.0035, -0.0028, 0.0035, 0.0028, 0.0012, steps=1)), 0))
    _ft_scale_z(out, h / _FT_DRAWN_CHAMP)
    obj = kit.fs_finish(out, 'SM_FS_Cup', mat, angle=40.0)
    s = h / _FT_DRAWN_CHAMP
    kit.fs_decal_uv(obj, lambda p, me: 0.150 * s < p.center.z < 0.250 * s and p.center.y < -0.01 and p.normal.y < -0.3
                    and math.hypot(p.center.x, p.center.y) > 0.045)
    return obj


def mirror(h):
    """The Funhouse Mirror (drawn 0.25 tall)."""
    mat = kit.fs_material('fs_mirror', [kit.fs_metal('gold', 0.16), kit.fs_chrome(0.03, 0xe6ecef), kit.fs_walnut('X', ebonized=True)])
    out = [kit.fs_tag(kit.fs_lathe('foot', [(0.0, 0.0), (0.050, 0.0), (0.050, 0.012), (0.046, 0.018), (0.030, 0.022), (0.0, 0.022)], segments=24), 2),
           kit.fs_tag(kit.fs_lathe('socket', [(0.0, 0.020), (0.016, 0.020), (0.017, 0.026), (0.012, 0.034), (0.009, 0.040), (0.0, 0.040)], segments=16), 0),
           kit.fs_tag(kit.fs_lathe('handle', [(0.0, 0.038), (0.0075, 0.038), (0.0068, 0.060), (0.0085, 0.066), (0.0068, 0.072), (0.0062, 0.098),
                                              (0.0085, 0.104), (0.011, 0.112), (0.0085, 0.122), (0.0, 0.123)], segments=16), 0)]
    a, b, wav, lobes = 0.047, 0.058, 0.035, 12
    zc = 0.25 - b * (1 + wav) - 0.006
    path = []
    for k in range(60):
        th = 2 * math.pi * k / 60
        kk = 1.0 + wav * math.cos(lobes * th)
        path.append((a * kk * math.cos(th), 0.0, zc + b * kk * math.sin(th)))
    out.append(kit.fs_tag(core.sweep('frame', path, core.rounded_rect(-0.006, -0.008, 0.006, 0.008, 0.003, steps=1), closed=True), 0))
    back = [(a * math.cos(2 * math.pi * k / 40), b * math.sin(2 * math.pi * k / 40)) for k in range(40)]
    out.append(kit.fs_tag(parts.outline_prism('back', back, [], -0.0065, -0.0005, Matrix.Translation((0.0, 0.0, zc)) @ parts.rot_x(90)), 0))
    out.append(kit.fs_tag(ft_glass('glass', a - 0.003, b - 0.003, zc, -0.002), 1))
    _ft_scale_z(out, h / _FT_DRAWN)
    obj = kit.fs_finish(out, 'SM_FS_Mirror', mat, angle=40.0)
    s = h / _FT_DRAWN
    kit.fs_decal_uv(obj, lambda p, me: p.normal.y < -0.5 and -0.0050 < p.center.y < -0.0015 and math.hypot(p.center.x / a, (p.center.z - zc * s) / (b * s)) < 0.97)
    return obj


def button(h):
    """The Big Red Button (drawn 0.25 tall)."""
    mat = kit.fs_material('fs_button', [kit.fs_flat(FT_PINK, 0.24), kit.fs_flat(FT_MINT, 0.24), kit.fs_chrome(0.06), kit.fs_flat(FT_BUTTON_RED, 0.05)])
    out = [kit.fs_tag(kit.fs_lathe('pink', [(0.0, 0.0), (0.062, 0.0), (0.062, 0.012), (0.057, 0.018), (0.044, 0.024), (0.036, 0.030), (0.034, 0.034),
                                            (0.0335, 0.095), (0.0, 0.095)], segments=24), 0),
           kit.fs_tag(kit.fs_lathe('ring', [(0.0, 0.093), (0.037, 0.093), (0.0385, 0.0985), (0.037, 0.104), (0.0, 0.104)], segments=24), 2),
           kit.fs_tag(kit.fs_lathe('mint', [(0.0, 0.102), (0.0335, 0.102), (0.0335, 0.160), (0.036, 0.166), (0.046, 0.174), (0.052, 0.182),
                                            (0.052, 0.192), (0.0, 0.192)], segments=24), 1),
           kit.fs_tag(kit.fs_lathe('bezel', [(0.0, 0.190), (0.050, 0.190), (0.050, 0.198), (0.048, 0.206), (0.044, 0.211), (0.039, 0.212),
                                             (0.036, 0.209), (0.0, 0.209)], segments=32), 2),
           kit.fs_tag(kit.fs_lathe('dome', [(0.0, 0.207), (0.035, 0.207), (0.0352, 0.214), (0.034, 0.222), (0.030, 0.232), (0.024, 0.240),
                                            (0.016, 0.246), (0.008, 0.249), (0.0, 0.250)], segments=32), 3)]
    _ft_scale_z(out, h / _FT_DRAWN)
    return kit.fs_finish(out, 'SM_FS_Button', mat, angle=40.0)


def seal(h):
    """The Seal (drawn 0.25 tall)."""
    mat = kit.fs_material('fs_seal', [kit.fs_onyx(), kit.fs_chrome(0.07), kit.fs_flat(FT_LACQUER, 0.1)])
    bh = 0.058
    out = [kit.fs_tag(parts.rbox('block', (0.0, 0.0, bh / 2), (0.12, 0.09, bh), 0.003, 2), 0),
           kit.fs_tag(parts.disc('face', (0.0, -0.045 - 0.002, bh / 2), (0.0, 1.0, 0.0), 0.021, 0.004, 32), 1)]
    z0 = bh
    stamp = kit.fs_lathe('stamp', [(0.0, z0), (0.025, z0), (0.025, z0 + 0.007), (0.022, z0 + 0.010), (0.011, z0 + 0.014), (0.0075, z0 + 0.022),
                                   (0.0075, z0 + 0.036), (0.012, z0 + 0.040), (0.012, z0 + 0.046), (0.0085, z0 + 0.050), (0.010, z0 + 0.064),
                                   (0.0145, z0 + 0.090), (0.0165, z0 + 0.112), (0.0155, z0 + 0.130), (0.0115, z0 + 0.150), (0.0095, z0 + 0.160),
                                   (0.0125, z0 + 0.170), (0.0135, z0 + 0.178), (0.0115, z0 + 0.186), (0.006, z0 + 0.191), (0.0, 0.25)], segments=32)
    kit.fs_tag(stamp, 1)
    kit.fs_tag_faces(stamp, lambda p, me: z0 + 0.0395 < p.center.z < z0 + 0.0465, 2)   # the red-lacquer collar
    out.append(stamp)
    _ft_scale_z(out, h / _FT_DRAWN)
    obj = kit.fs_finish(out, 'SM_FS_Seal', mat, angle=40.0)
    kit.fs_decal_uv(obj, lambda p, me: p.normal.y < -0.9 and p.center.y < -0.0485 and abs(p.center.x) < 0.022)
    return obj


def grindstone(h):
    """The Grindstone (drawn 0.25 tall)."""
    mat = kit.fs_material('fs_grindstone', [kit.fs_onyx(), kit.fs_chrome(0.08), kit.fs_flat(FT_LACQUER, 0.1)])
    R, axle_z = 0.062, 0.25 - 0.062
    out = [kit.fs_tag(parts.rbox('base', (0.0, 0.0, 0.01), (0.15, 0.12, 0.02), 0.003, 2), 0)]
    yz = Matrix(((0.0, 0.0, 1.0, 0.0), (1.0, 0.0, 0.0, 0.0), (0.0, 1.0, 0.0, 0.0), (0.0, 0.0, 0.0, 1.0)))   # an outline in (y, z), along X
    a_frame = [(-0.05, 0.02), (0.05, 0.02), (0.012, axle_z + 0.006), (-0.012, axle_z + 0.006)]
    hole = [(-0.03, 0.034), (0.0, axle_z - 0.04), (0.03, 0.034)]
    for sx in (-1, 1):
        x0 = sx * 0.042 - 0.006
        out.append(kit.fs_tag(parts.outline_prism('upright', a_frame, [hole], x0, x0 + 0.012, yz), 0))
        out.append(kit.fs_tag(parts.rbox('bearing', (sx * 0.042, 0.0, axle_z), (0.016, 0.026, 0.02), 0.003, 2), 1))
    out.append(kit.fs_tag(parts.rod('axle', (-0.058, 0.0, axle_z), (0.058, 0.0, axle_z), 0.0055, 12), 1))
    wheel = kit.fs_lathe('wheel', [(0.0, -0.020), (0.012, -0.020), (0.014, -0.017), (0.016, -0.012), (0.056, -0.013), (0.060, -0.0125),
                                   (R, -0.010), (R, 0.010), (0.060, 0.0125), (0.056, 0.013), (0.016, 0.012), (0.014, 0.017), (0.012, 0.020),
                                   (0.0, 0.020)], Matrix.Translation((0.0, 0.0, axle_z)) @ parts.rot_y(90), segments=40)
    kit.fs_tag(wheel, 1)
    kit.fs_tag_faces(wheel, lambda p, me: abs(p.normal.x) < 0.5 and abs(p.center.x) < 0.0105 and math.hypot(p.center.y, p.center.z - axle_z) > R - 0.004, 2)
    out.append(wheel)
    out.append(kit.fs_tag(parts.rbox('crank', (0.059, -0.015, axle_z), (0.006, 0.042, 0.010), 0.002, 1), 1))
    out.append(kit.fs_tag(parts.rod('grip', (0.062, -0.033, axle_z), (0.074, -0.033, axle_z), 0.0042, 12), 2))
    _ft_scale_z(out, h / _FT_DRAWN)
    return kit.fs_finish(out, 'SM_FS_Grindstone', mat, angle=40.0)


def slam_plate(h):
    """The Slam Plate: the salver stands tilted back in a groove on a stepped walnut stand, a brass ledge before it and a
    brass strut behind; the stand is as tall as it takes for the plate's top to be h."""
    tilt = 12.0
    rot = parts.rot_x(90 - tilt)                  # the plate's axis (its face) toward -Y, tilted back
    prof = [(0.0, 0.001), (0.040, 0.001), (0.042, 0.0), (0.046, 0.0), (0.048, 0.002), (0.066, 0.004), (0.0725, 0.008), (0.0735, 0.011),
            (0.072, 0.0125), (0.068, 0.011), (0.062, 0.0065), (0.052, 0.0055), (0.016, 0.0055), (0.012, 0.0075), (0.008, 0.0090),
            (0.004, 0.0097), (0.0, 0.0099)]
    plate = kit.fs_lathe('plate', prof, rot, segments=32)
    vs = [v.co.copy() for v in plate.data.vertices]
    lowest = min(vs, key=lambda v: v.z)
    span = max(v.z for v in vs) - lowest.z
    bt = h - span + 0.004                         # the stand's top: the plate sits 4 mm deep in its groove
    move = Vector((0.0, -0.006 - lowest.y, bt - 0.004 - lowest.z))
    plate.data.transform(Matrix.Translation(move))
    cz = (rot @ Vector((0.0, 0.0, 0.0099)) + move).z
    mat = kit.fs_material('fs_slam_plate', [kit.fs_chrome(0.12, 0xd4d7dc), ft_sunburst(cz, tilt), kit.fs_walnut('X'), kit.fs_metal('brass', 0.2)])
    kit.fs_tag(plate, 0)
    front = rot @ Vector((0.0, 0.0, 1.0))
    kit.fs_tag_faces(plate, lambda p, me: p.normal.dot(front) > 0.6 and (p.center - move).length < 0.0625, 1)
    out = [plate,
           kit.fs_tag(parts.rbox('slab', (0.0, 0.0, 0.009), (0.13, 0.085, 0.018), 0.003, 2), 2),
           kit.fs_tag(parts.rbox('body', (0.0, 0.0, (0.018 + bt - 0.012) / 2), (0.094, 0.056, bt - 0.030), 0.002, 2), 2),
           kit.fs_tag(parts.rbox('cap', (0.0, 0.0, bt - 0.006), (0.104, 0.066, 0.012), 0.003, 2), 2),
           kit.fs_tag(parts.rbox('inlay', (0.0, -0.028 - 0.0004, (0.018 + bt - 0.012) / 2), (0.074, 0.0012, 0.003), 0.0), 3),
           kit.fs_tag(parts.rbox('ledge', (0.0, -0.006 - 0.0095, bt + 0.004), (0.05, 0.006, 0.008), 0.0015, 1), 3)]
    back_pt = rot @ Vector((0.0, -0.025, -0.002)) + move
    out.append(kit.fs_tag(parts.rod('strut', (0.0, 0.026, bt - 0.002), tuple(back_pt), 0.0025, 8), 3))
    return kit.fs_finish(out, 'SM_FS_SlamPlate', mat, angle=40.0)


def pennant(h):
    """The Gold Pennant (drawn 0.2875 tall)."""
    mat = kit.fs_material('fs_pennant', [kit.fs_gold_felt(), kit.fs_metal('brass', 0.2), kit.fs_walnut('X'), kit.fs_walnut('Y')])
    out = []
    for i, y in enumerate((-0.039, -0.013, 0.013, 0.039)):   # the pier planks on two runners
        dz = (0.0, 0.0008, -0.0006, 0.0004)[i]
        out.append(kit.fs_tag(parts.rbox('plank', (0.0, y, 0.020 + dz), (0.13, 0.0245, 0.016), 0.0015, 1), 2))
    for sx in (-1, 1):
        out.append(kit.fs_tag(parts.rbox('runner', (sx * 0.045, 0.0, 0.006), (0.012, 0.10, 0.012), 0.001, 1), 3))
    xs = -0.042
    staff_top = 0.2555
    at = Matrix.Translation((xs, 0.0, 0.0))
    out.append(kit.fs_tag(kit.fs_lathe('socket', [(0.0, 0.027), (0.011, 0.027), (0.011, 0.031), (0.008, 0.035), (0.0065, 0.046), (0.0, 0.046)], at, segments=16), 1))
    out.append(kit.fs_tag(parts.rod('staff', (xs, 0.0, 0.044), (xs, 0.0, staff_top + 0.002), 0.0035, 12), 1))
    out.append(kit.fs_tag(kit.fs_lathe('ball', [(0.0, 0.2545), (0.004, 0.2555), (0.0055, 0.2595), (0.004, 0.2635), (0.0015, 0.2648), (0.0, 0.2650)],
                                       at, segments=16), 1))
    out += [kit.fs_tag(o, 1) for o in ft_spade('spade', 0.018, 0.0235, 0.264, 0.005, (xs, 0.0))]
    z_lo, z_hi = 0.168, 0.250
    flag = ft_flag('flag', xs + 0.0038, z_lo, z_hi, 0.107, (z_lo + z_hi) / 2 - 0.006, 0.0026)
    out.append(kit.fs_tag(flag, 0))
    for z in (z_lo + 0.003, z_hi - 0.003):        # the rings holding the hoist to the staff
        out.append(kit.fs_tag(kit.fs_lathe('ring', [(0.0034, z - 0.0022), (0.0056, z - 0.0016), (0.0056, z + 0.0016), (0.0034, z + 0.0022)],
                                           at, segments=12), 1))
    _ft_scale_z(out, h / _FT_DRAWN_CHAMP)
    obj = kit.fs_finish(out, 'SM_FS_Pennant', mat, angle=40.0)
    s = h / _FT_DRAWN_CHAMP
    kit.fs_decal_uv(obj, lambda p, me: p.normal.y < -0.5 and p.center.x > xs + 0.0045 and z_lo * s - 0.001 < p.center.z < z_hi * s + 0.001
                    and abs(p.center.y) < 0.02)
    return obj


def sceptre(h):
    """The Sceptre (drawn 0.2875 tall)."""
    mat = kit.fs_material('fs_sceptre', [kit.fs_onyx(), kit.fs_chrome(0.06), kit.fs_flat(FT_LACQUER, 0.1)])
    out = [kit.fs_tag(parts.rbox('block', (0.0, 0.0, 0.0125), (0.11, 0.085, 0.025), 0.003, 2), 0),
           kit.fs_tag(parts.rbox('step', (0.0, 0.0, 0.0375), (0.08, 0.062, 0.025), 0.003, 2), 0),
           kit.fs_tag(kit.fs_lathe('socket', [(0.0, 0.049), (0.016, 0.049), (0.016, 0.053), (0.013, 0.057), (0.0095, 0.066), (0.0, 0.066)], segments=20), 1)]
    rod = kit.fs_lathe('sceptre', [(0.0, 0.064), (0.0085, 0.065), (0.0095, 0.070), (0.0080, 0.076), (0.0062, 0.078), (0.0062, 0.120),
                                   (0.0075, 0.121), (0.0075, 0.127), (0.0062, 0.128), (0.0062, 0.198), (0.0085, 0.200), (0.012, 0.205),
                                   (0.0125, 0.214), (0.010, 0.220), (0.0075, 0.222), (0.012, 0.228), (0.0145, 0.236), (0.0135, 0.244),
                                   (0.009, 0.250), (0.005, 0.2535), (0.0, 0.2540)], segments=20)
    kit.fs_tag(rod, 1)
    kit.fs_tag_faces(rod, lambda p, me: (0.077 < p.center.z < 0.1205 or 0.1275 < p.center.z < 0.1990), 0)   # the onyx shaft
    kit.fs_tag_faces(rod, lambda p, me: 0.1990 < p.center.z < 0.2205, 2)                                   # the red-lacquer collar
    out.append(rod)
    for turn in (False, True):
        out += [kit.fs_tag(o, 1) for o in ft_spade('spade', 0.024, 0.0345, 0.253, 0.007, turn=turn)]
    _ft_scale_z(out, h / _FT_DRAWN_CHAMP)
    return kit.fs_finish(out, 'SM_FS_Sceptre', mat, angle=40.0)


# ================================================================== the checks FL-8b adds

def _ft_box(obj):
    v = np.empty(len(obj.data.vertices) * 3, dtype=np.float64)
    obj.data.vertices.foreach_get('co', v)
    v = v.reshape(-1, 3)
    return v.min(axis=0), v.max(axis=0)


def ft_checks(module, objs):
    """FsFit and FsChampion on the built trophies. Prints every line, then raises on any failure."""
    fails = []
    heights = {}
    for o in objs:
        lo, hi = _ft_box(o)
        ht = float(hi[2] - lo[2])
        heights[o.name] = ht
        reach = 2.0 * float(max(-lo[0], hi[0], -lo[1], hi[1]))
        ok = abs(lo[2]) <= 0.0005 and hi[2] <= kit.CLEARANCE and reach <= FT_FOOT
        print(f'[{module}] FsFit {o.name}: base z {lo[2] * 1000:+.2f} mm, height {ht * 1000:.1f} <= {kit.CLEARANCE * 1000:.0f} mm, '
              f'footprint {(hi[0] - lo[0]) * 1000:.1f} x {(hi[1] - lo[1]) * 1000:.1f} mm (2 x its reach from the origin {reach * 1000:.1f} '
              f'<= {FT_FOOT * 1000:.0f}) {"OK" if ok else "FAIL"}')
        if not ok:
            fails.append(f'{o.name} does not fit its plinth (base {lo[2]:.4f}, height {ht:.4f}, reach {reach:.4f})')
    for site, champ, others in FT_SITES:
        if champ not in heights or any(x not in heights for x in others):
            fails.append(f'{site}: a trophy is missing')
            continue
        hs = [heights[x] for x in others]
        ratio = heights[champ] / max(hs)
        same = max(hs) - min(hs) <= 2 * FT_TOL * max(hs)
        ok = same and abs(ratio - FT_CHAMP) <= FT_TOL * FT_CHAMP
        print(f'[{module}] FsChampion {site}: {champ} {heights[champ] * 1000:.1f} mm = {ratio:.4f} x '
              f'{", ".join(f"{x} {heights[x] * 1000:.1f}" for x in others)} (want {FT_CHAMP} +/- {FT_TOL * 100:.1f}%'
              f'{"" if same else "; the others differ"}) {"OK" if ok else "FAIL"}')
        if not ok:
            fails.append(f'{site}: {champ} is {ratio:.4f}x its site\'s others (want {FT_CHAMP})')
    if fails:
        raise AssertionError(f'[{module}] fest trophy checks FAILED: ' + '; '.join(fails))
    print(f'[{module}] fest trophy checks passed (FsFit, FsChampion)')


# ================================================================== the build

MESHES = list(kit.FS_TROPHIES)
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {'fs_cup': 1024, 'fs_mirror': 1024, 'fs_button': 1024, 'fs_seal': 1024, 'fs_grindstone': 1024, 'fs_slam_plate': 1024,
                 'fs_pennant': 1024, 'fs_sceptre': 1024}
AO_DISTANCE = 0.01
# Review: the eight in two rows (each site's championship trophy beside its others), the pennant close.
REVIEW_LAYOUT = {'SM_FS_Mirror': (-0.3, -0.12), 'SM_FS_Button': (-0.1, -0.12), 'SM_FS_Pennant': (0.1, -0.12), 'SM_FS_SlamPlate': (0.3, -0.12),
                 'SM_FS_Seal': (-0.3, 0.12), 'SM_FS_Grindstone': (-0.1, 0.12), 'SM_FS_Sceptre': (0.1, 0.12), 'SM_FS_Cup': (0.3, 0.12)}
REVIEW_VIEWS = [('front', 8, 9, 2.6), ('side', 40, 20, 2.4), ('detail', -18, 8, 0.36, (0.115, -0.12, 0.2))]


def build():
    core.reset()
    kit.FS_CC0_USED.clear()
    makers = {'SM_FS_Cup': cup, 'SM_FS_Mirror': mirror, 'SM_FS_Button': button, 'SM_FS_Seal': seal, 'SM_FS_Grindstone': grindstone,
              'SM_FS_SlamPlate': slam_plate, 'SM_FS_Pennant': pennant, 'SM_FS_Sceptre': sceptre}
    return [makers[name](FT_HEIGHT[name]) for name in MESHES]


def bake_parts(objs):
    ft_checks('fest_trophies', objs)
    kit.fs_prebake('fest_trophies', MESHES, objs, TEXTURE_SIZE, TEXTURE_SIZES)
    return objs


def assemble(objs):
    kit.fs_postbake('fest_trophies', MESHES, objs)
    return objs


def pose_for_review(objs):
    for o in objs:
        if o.name in REVIEW_LAYOUT:
            o.location = (REVIEW_LAYOUT[o.name][0], REVIEW_LAYOUT[o.name][1], 0.0)
    bpy.context.view_layer.update()   # the review frames all eight
