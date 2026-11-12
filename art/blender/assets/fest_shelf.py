"""The festival shelf kit (Update 4 G1, F-look FL-8a): the apartment's trophy shelf, its plinths and brass name plates, and
the glass cases for the ring and the bracelet. fest_belt.py and fest_pinboard.py build the rest of FL-8a (the belt, the cork
pin board and its pins); fest_trophies.py (FL-8b) builds the eight trophies. All of them share this module's material set,
its slots and its checks (SPEC §9.2, §9.4).

- SM_FS_Shelf: a floor-standing walnut display shelf, 1.20 x 0.28 x 1.04 m: two open tiers of five plinths, a top board
  for the belts, a back panel with vertical grain, a recessed kick base, a brass inlay line on each board's front edge.
- SM_FS_Plinth (instanced ten times): a 0.17 m square riser, walnut base and cap on an ebonized body, 0.07 m tall.
- SM_FS_Plate (instanced, one per plinth): a brushed brass plate, 0.11 x 0.026 m, blank: the engraving is drawn by the
  game's code (no text, no font here). UV1 ("Decal", the glb's TEXCOORD_1) maps its front face onto 0..1, upright as an
  image in Unreal (u left to right, v top to bottom); every other face sits outside 0..1 (at u = -1). See fs_decal_uv.
- SM_FS_CaseRing, SM_FS_CaseBracelet: a walnut base, a brass-framed glass box (the unbaked FS_Glass slot), and inside a
  gold ring with a garnet stone on a garnet velvet cushion (Ring Rush), or a gold link bracelet with a medallion round a
  royal-blue velvet bolster (the Championship Online). The existing 2D ring and bracelet stay the trophies' art on the
  laptop; these are their shelf pieces.

Coordinates (meters), front toward -Y, Z up. Origins: the shelf, a plinth and a case at the middle of their footprint on
the floor or shelf (z = 0 their bottom); the plate at the middle of its back face (it mounts on a plinth's front, PLATE_AT).
The shelf's back face is at y = +SHELF_D / 2 (flush to the wall). SLOTS are the ten plinth origins in the shelf's frame
(bottom tier left to right, then the middle tier); PLINTH_H is a plinth's top; CLEARANCE the room above a plinth for a
trophy (FL-8b's tallest piece fits under it); BELT_SLOTS three belt origins on the top board (the shelf's budget counts
three belts). A case or a trophy stands at a slot + (0, 0, PLINTH_H).

Materials: every mesh bakes to ONE texture set (one draw), however many finishes it has: its parts carry a face attribute
fs_part that picks the part's finish inside a single shader (fs_material). The cases add FS_Glass, an unbaked, blended
slot (split off before the bake and joined back after it, as wash_fold does). The finishes are the kit's shared set:
walnut (grain along X or Z), ebonized walnut, brass, gold, a neutral metal, velvet, leather, cork, enamel, garnet; FL-8b
adds chrome (silver, chrome and platinum: polished, never textured, SPEC §9.2's "no texture"), onyx and the gold felt
(v1.6's one new material, §9.4: Velour Velvet tinted gold in the CC0 build).

Two builds of the same meshes:
  default: procedural finishes only. Committed: unreal/Art/Meshes/SM_FS_*.glb and art/review/fest_*.jpg (our own art with
    no download inside: SPEC §9.1 rule 4, the lead's R-20). This is the committed fallback (rule 7).
  SHORTSTACK_FS_CC0=<the G1 textures dir> [SHORTSTACK_FS_OUT=<dir>]: the CC0 maps (ambientCG Metal 042 A, Leather
    026 / 027, Cork 003, Onyx 013; Poly Haven Velour Velvet, American Walnut Veneer) baked in instead. Written to SHORTSTACK_FS_OUT
    (Meshes/, review/, build/; default art/build/fest_cc0, git-ignored), never into unreal/Art or art/review: R-20 lands a
    CC0 bake only as its imported .uasset (LFS). The colours stay the kit's (each map gives its luminance as detail, its
    roughness and its height); the colour maps' own hue is used only for cork and onyx. Only the maps/ images are read: never
    a .blend.

Checks (each asserts; a failure stops the build before anything is baked or exported, and Blender run with
--python-exit-code 1 exits 1):
  FsTriangles  every mesh within its FS_BUDGET row; the shelf with all ten (FS_ON_SHELF: the actual count of every SM_FS
               mesh already built, from the tally in the build dir, else its budget as a reserve) <= 40,000; the pin board
               with its 40 pins <= 5,000 (SPEC §9.4).
  FsTextures   every texture declared for the bake and every image the materials load <= 2048 px (§9.4), and again on the
               baked images before export.
  FsSlots      every mesh has exactly its FS_SLOTS slots (one baked; the cases' second is FS_Glass), before and after the
               bake.
It also prints the draw-call estimate (one per material section of each distinct mesh on the shelf; instanced plinths,
plates and belts once; FL-8b's trophies one each until built) against §9.4's cap of 12, unasserted: the lead measures the
draw calls in PIE (§9.4: "printed, never assumed").
"""
import json
import math
import os

import bpy  # noqa: F401,I001
import bmesh
import numpy as np
from mathutils import Matrix, Vector

from artkit import core, parts
from artkit.shade import mix_float, noise, object_coords, ramp, scaled, vec

# ================================================================== the build (procedural, or the CC0 maps baked in)

FS_CC0 = os.environ.get('SHORTSTACK_FS_CC0', '')
if FS_CC0:
    _fs_out = os.path.abspath(os.environ.get('SHORTSTACK_FS_OUT') or os.path.join(core.BUILD_DIR, 'fest_cc0'))
    _fs_root = os.path.normcase(os.path.abspath(core.ROOT)) + os.sep
    _fs_ignored = os.path.normcase(os.path.abspath(os.path.join(core.ROOT, 'art', 'build'))) + os.sep
    if os.path.normcase(_fs_out + os.sep).startswith(_fs_root) and not os.path.normcase(_fs_out + os.sep).startswith(_fs_ignored):
        raise SystemExit(f'fest kit: a CC0 bake never goes into the repo outside art/build (R-20): {_fs_out}')
    core.EXPORT_DIR = os.path.join(_fs_out, 'Meshes')
    core.REVIEW_DIR = os.path.join(_fs_out, 'review')
    core.BUILD_DIR = os.path.join(_fs_out, 'build')

# The CC0 maps each finish reads (under SHORTSTACK_FS_CC0), and the ledger name of each.
FS_CC0_MAPS = {
    'walnut': ('American Walnut Veneer', 'american_walnut_veneer/american_walnut_veneer_{}_2k.jpg', {'col': 'diff', 'rough': 'rough', 'disp': 'disp'}),
    'metal': ('Metal 042 A', 'Metal042A/maps/Metal042A_2K-JPG_{}.jpg', {'col': 'Color', 'rough': 'Roughness', 'disp': 'Displacement'}),
    'leather26': ('Leather 026', 'Leather026/maps/Leather026_2K-JPG_{}.jpg', {'col': 'Color', 'rough': 'Roughness', 'disp': 'Displacement'}),
    'leather27': ('Leather 027', 'Leather027/maps/Leather027_2K-JPG_{}.jpg', {'col': 'Color', 'rough': 'Roughness', 'disp': 'Displacement'}),
    'velvet': ('Velour Velvet', 'velour_velvet/velour_velvet_{}_2k.jpg', {'col': 'diff', 'rough': 'rough', 'disp': 'disp'}),
    'cork': ('Cork 003', 'Cork003/maps/Cork003_2K-JPG_{}.jpg', {'col': 'Color', 'rough': 'Roughness', 'disp': 'Displacement'}),
    'onyx': ('Onyx 013', 'Onyx013/maps/Onyx013_2K-JPG_{}.jpg', {'col': 'Color', 'rough': 'Roughness', 'disp': 'Displacement'}),
}
FS_CC0_USED = {}   # mesh material name -> set of ledger names (printed after each build)
_FS_RANGE = {}     # image path -> (low, high) luminance percentiles (linear)

# ================================================================== slots, budgets, checks

SHELF_W, SHELF_D = 1.20, 0.28
SIDE_T, BOARD_T, BACK_T = 0.022, 0.028, 0.008
KICK = 0.09
TIER = 0.43                      # board top to the next board's underside
TIER_Z = (KICK + BOARD_T, KICK + BOARD_T + TIER + BOARD_T)   # the two tiers' surfaces: 0.118, 0.576
TOP_Z = TIER_Z[1] + TIER          # the top board's underside: 1.006
TOP_T = 0.03
PLINTH_W, PLINTH_H = 0.17, 0.07
CLEARANCE = TIER - PLINTH_H       # 0.36 above a plinth's top
SLOT_Y = -0.02
SLOT_PITCH = (SHELF_W - 2 * SIDE_T) / 5
SLOTS = [(-(SHELF_W - 2 * SIDE_T) / 2 + SLOT_PITCH * (i + 0.5), SLOT_Y, z) for z in TIER_Z for i in range(5)]
BELT_SLOTS = [(x, 0.0, TOP_Z + TOP_T) for x in (-0.39, 0.0, 0.39)]
PLATE = (0.11, 0.026, 0.0025)     # width, height, thickness
PLATE_AT = (0.0, -0.075, 0.035)   # on a plinth's front (its body's face), from the plinth's origin

FS_BUDGET = {'SM_FS_Shelf': 1600, 'SM_FS_Plinth': 300, 'SM_FS_Plate': 50, 'SM_FS_CaseRing': 1500, 'SM_FS_CaseBracelet': 1800,
             'SM_FS_Belt': 1200, 'SM_FS_PinBoard': 600, 'SM_FS_Pin': 110}
# The budgets are the allocations: with every unbuilt mesh at its budget the shelf sums to 40,000 and the board to 5,000.
FS_TROPHIES = ['SM_FS_Cup', 'SM_FS_Mirror', 'SM_FS_Button', 'SM_FS_Seal', 'SM_FS_Grindstone', 'SM_FS_SlamPlate', 'SM_FS_Pennant',
               'SM_FS_Sceptre']
FS_TROPHY_RESERVE = 3500          # each of FL-8b's eight, until it is built (SPEC §9.4: about 2.5-3.5k each)
FS_ON_SHELF = {'SM_FS_Shelf': 1, 'SM_FS_Plinth': 10, 'SM_FS_Plate': 10, 'SM_FS_CaseRing': 1, 'SM_FS_CaseBracelet': 1,
               'SM_FS_Belt': len(BELT_SLOTS), **{t: 1 for t in FS_TROPHIES}}
FS_INSTANCED = {'SM_FS_Plinth', 'SM_FS_Plate', 'SM_FS_Belt'}
FS_ON_BOARD = {'SM_FS_PinBoard': 1, 'SM_FS_Pin': 40}
FS_SLOTS = {'SM_FS_CaseRing': 2, 'SM_FS_CaseBracelet': 2}   # every other mesh: 1
FS_UNBAKED = {'FS_Glass'}
SHELF_CAP, BOARD_CAP, TEX_CAP, DRAW_CAP = 40000, 5000, 2048, 12


def fs_tris(obj):
    me = obj.evaluated_get(bpy.context.evaluated_depsgraph_get()).to_mesh()
    me.calc_loop_triangles()
    n = len(me.loop_triangles)
    obj.evaluated_get(bpy.context.evaluated_depsgraph_get()).to_mesh_clear()
    return n


def _fs_tally_path():
    return os.path.join(core.BUILD_DIR, 'fest_tally.json')


def _fs_tally():
    try:
        with open(_fs_tally_path(), encoding='utf-8') as f:
            return json.load(f)
    except (OSError, ValueError):
        return {}


def _fs_images(objs):
    out = {}
    for o in objs:
        for s in o.material_slots:
            if s.material and s.material.node_tree:
                for n in s.material.node_tree.nodes:
                    if n.type == 'TEX_IMAGE' and n.image is not None:
                        out[n.image.name] = tuple(n.image.size)
    return out


def fs_prebake(module, meshes, objs, size, sizes):
    """The three checks on the built meshes, before the bake. Prints every line, then raises on any failure."""
    fails = []
    by = {o.name: o for o in objs}
    missing = [m for m in meshes if m not in by]
    if missing:
        fails.append(f'not built: {missing}')
    counts = {}
    for name in meshes:
        o = by.get(name)
        if o is None:
            continue
        n = fs_tris(o)
        counts[name] = n
        cap = FS_BUDGET.get(name, FS_TROPHY_RESERVE)
        ok = n <= cap
        print(f'[{module}] FsTriangles {name} {n} <= {cap} {"OK" if ok else "FAIL"}')
        if not ok:
            fails.append(f'{name} {n} triangles > {cap}')
    tally = _fs_tally()
    slots_now = {o.name: len(o.material_slots) for o in objs}

    def count_of(name):
        if name in counts:
            return counts[name], 'built'
        if name in tally:
            return tally[name]['tris'], 'tally'
        return FS_BUDGET.get(name, FS_TROPHY_RESERVE), 'reserve'

    for label, table, cap in (('shelf with all ten', FS_ON_SHELF, SHELF_CAP), ('pin board with its pins', FS_ON_BOARD, BOARD_CAP)):
        total = 0
        terms = []
        for name, k in table.items():
            n, src = count_of(name)
            total += n * k
            terms.append(f'{name} {n}x{k}{"" if src == "built" else " (" + src + ")"}')
        ok = total <= cap
        print(f'[{module}] FsTriangles {label}: {total} <= {cap} {"OK" if ok else "FAIL"} = ' + ', '.join(terms))
        if not ok:
            fails.append(f'{label}: {total} > {cap}')
    # FsSlots
    for name in meshes:
        o = by.get(name)
        if o is None:
            continue
        want = FS_SLOTS.get(name, 1)
        names = [s.material.name if s.material else None for s in o.material_slots]
        baked = [x for x in names if x not in FS_UNBAKED]
        ok = len(names) == want and len(baked) == 1 and all(x in FS_UNBAKED for x in names if x not in baked)
        print(f'[{module}] FsSlots {name} {len(names)} == {want} ({", ".join(str(x) for x in names)}) {"OK" if ok else "FAIL"}')
        if not ok:
            fails.append(f'{name} slots {names} (want {want}: one baked{", plus FS_Glass" if want == 2 else ""})')
    # FsTextures: the declared bake sizes and every image the materials load (the CC0 maps)
    for name in meshes:
        o = by.get(name)
        if o is None:
            continue
        for s in o.material_slots:
            if s.material is None or s.material.name in FS_UNBAKED:
                continue
            px = sizes.get(s.material.name, size)
            w, h = px if isinstance(px, (tuple, list)) else (px, px)
            ok = max(w, h) <= TEX_CAP
            print(f'[{module}] FsTextures {name} bake {s.material.name} {w}x{h} <= {TEX_CAP} {"OK" if ok else "FAIL"}')
            if not ok:
                fails.append(f'{name} bakes {s.material.name} at {w}x{h}')
    for img, (w, h) in sorted(_fs_images(objs).items()):
        ok = max(w, h) <= TEX_CAP
        print(f'[{module}] FsTextures source {img} {w}x{h} <= {TEX_CAP} {"OK" if ok else "FAIL"}')
        if not ok:
            fails.append(f'source image {img} {w}x{h}')
    # The draw-call estimate (printed against the cap, never asserted: PIE measures it).
    draws = 0
    terms = []
    for name in FS_ON_SHELF:
        if name in slots_now:
            k, src = slots_now[name], ''
        elif name in tally:
            k, src = tally[name]['slots'], ' (tally)'
        else:
            k, src = FS_SLOTS.get(name, 1), ' (reserve)'
        draws += k
        terms.append(f'{name} {k}{src}')
    verdict = 'within the cap' if draws <= DRAW_CAP else f'OVER the cap by {draws - DRAW_CAP}: ten distinct trophy pieces take ten sections alone; the lead measures it in PIE'
    print(f'[{module}] draw calls (estimate, one per section of each distinct mesh; plinths, plates and belts instanced once): '
          f'{draws} against {DRAW_CAP}, {verdict} = ' + ', '.join(terms))
    if FS_CC0:
        for mat, used in sorted(FS_CC0_USED.items()):
            print(f'[{module}] CC0 maps in {mat}: {", ".join(sorted(used)) or "none"}')
    if fails:
        raise AssertionError(f'[{module}] fest kit checks FAILED: ' + '; '.join(fails))
    tally.update({name: {'tris': counts[name], 'slots': slots_now[name]} for name in counts})
    os.makedirs(core.BUILD_DIR, exist_ok=True)
    with open(_fs_tally_path(), 'w', encoding='utf-8', newline='\n') as f:
        json.dump(tally, f, indent=1, sort_keys=True)
        f.write('\n')
    print(f'[{module}] fest kit checks passed (FsTriangles, FsSlots, FsTextures)')


def fs_postbake(module, meshes, objs):
    """FsTextures and FsSlots again on the baked meshes, before export; drops the fs_part attribute."""
    fails = []
    by = {o.name: o for o in objs}
    for name in meshes:
        o = by.get(name)
        if o is None:
            fails.append(f'{name} missing after the bake')
            continue
        want = FS_SLOTS.get(name, 1)
        if len(o.material_slots) != want:
            fails.append(f'{name} has {len(o.material_slots)} slots after the bake (want {want})')
        a = o.data.attributes.get('fs_part')
        if a is not None:
            o.data.attributes.remove(a)
    for img, (w, h) in sorted(_fs_images(objs).items()):
        ok = max(w, h) <= TEX_CAP
        print(f'[{module}] FsTextures baked {img} {w}x{h} <= {TEX_CAP} {"OK" if ok else "FAIL"}')
        if not ok:
            fails.append(f'baked image {img} {w}x{h}')
    if fails:
        raise AssertionError(f'[{module}] fest kit checks FAILED after the bake: ' + '; '.join(fails))
    print(f'[{module}] fest kit checks passed after the bake ({len(objs)} meshes)')


# ================================================================== the finishes (one shader per mesh, picked by fs_part)

def _fs_srgb_to_linear(a):
    return np.where(a <= 0.04045, a / 12.92, ((a + 0.055) / 1.055) ** 2.4)


def _fs_cc0(key, which):
    """Path of a CC0 map, checked to exist (only maps/ images: never a .blend)."""
    ledger, pattern, names = FS_CC0_MAPS[key]
    path = os.path.join(FS_CC0, pattern.format(names[which]))
    if not os.path.isfile(path):
        raise SystemExit(f'fest kit: missing CC0 map {path}')
    return path, ledger


def _fs_range(path, color):
    """2nd and 98th percentiles of a map's luminance (linear for a colour map, raw for a data map)."""
    if path not in _FS_RANGE:
        img = bpy.data.images.load(path, check_existing=True)
        w, h = img.size
        a = np.empty(w * h * 4, dtype=np.float32)
        img.pixels.foreach_get(a)
        a = a.reshape(-1, 4)[::5, :3]
        if color:
            a = _fs_srgb_to_linear(a)
        lum = 0.2126 * a[:, 0] + 0.7152 * a[:, 1] + 0.0722 * a[:, 2]
        lo, hi = float(np.percentile(lum, 2)), float(np.percentile(lum, 98))
        _FS_RANGE[path] = (lo, max(hi, lo + 1e-3))
    return _FS_RANGE[path]


def _fs_map_range(m, x, lo, hi, to_lo=0.0, to_hi=1.0):
    n = m.node('ShaderNodeMapRange')
    n.clamp = True
    n.inputs['From Min'].default_value = lo
    n.inputs['From Max'].default_value = hi
    n.inputs['To Min'].default_value = to_lo
    n.inputs['To Max'].default_value = to_hi
    m.link(x, n.inputs['Value'])
    return n.outputs['Result']


class _FsMaps:
    """A CC0 asset's colour luminance (0..1 between its percentiles), roughness (0..1) and height (-0.5..0.5) at a vector."""

    def __init__(self, m, key, vector, box=True, tag=None):
        self.m = m
        out = {}
        for which in ('col', 'rough', 'disp'):
            path, ledger = _fs_cc0(key, which)
            n = m.image(path, non_color=which != 'col', vector=vector, extension='REPEAT')
            if box:
                n.projection = 'BOX'
                n.projection_blend = 0.3
            out[which] = (n, path)
        if tag:
            FS_CC0_USED.setdefault(tag, set()).add(ledger)
        col, cpath = out['col']
        bw = m.node('ShaderNodeRGBToBW')
        m.link(col.outputs['Color'], bw.inputs['Color'])
        lo, hi = _fs_range(cpath, True)
        self.color = col.outputs['Color']
        self.lum = _fs_map_range(m, bw.outputs['Val'], lo, hi)
        r, rpath = out['rough']
        rlo, rhi = _fs_range(rpath, False)
        self.rough = _fs_map_range(m, r.outputs['Color'], rlo, rhi)
        d, dpath = out['disp']
        dlo, dhi = _fs_range(dpath, False)
        self.disp = m.math('SUBTRACT', _fs_map_range(m, d.outputs['Color'], dlo, dhi), 0.5)


def _fs_obj_vec(m, sep, tile):
    return vec(m, m.math('DIVIDE', sep.outputs['X'], tile), m.math('DIVIDE', sep.outputs['Y'], tile), m.math('DIVIDE', sep.outputs['Z'], tile))


def _fs_grain_uv(m, sep, along, tile):
    """Planar coordinates for a board whose grain runs along one axis: u along it, v across (the sum of the other two),
    so the grain reads right on its faces and edges alike."""
    axes = ['X', 'Y', 'Z']
    others = [a for a in axes if a != along]
    u = m.math('DIVIDE', sep.outputs[along], tile)
    v = m.math('DIVIDE', m.math('ADD', sep.outputs[others[0]], sep.outputs[others[1]]), tile)
    return vec(m, u, v, 0.0)


def fs_walnut(along='X', ebonized=False):
    """Walnut with a satin oil finish, its grain along an axis; ebonized: the same wood stained near-black."""
    dark, light = (0x0b0806, 0x241a13) if ebonized else (0x2e1c10, 0x66422a)

    def surface(m, sep, tag):
        if FS_CC0:
            mp = _FsMaps(m, 'walnut', _fs_grain_uv(m, sep, along, 0.8), box=False, tag=tag)
            color = m.mix(mp.lum, core.hex_linear(dark), core.hex_linear(light))
            rough = m.math('ADD', 0.24 if ebonized else 0.32, m.math('MULTIPLY', mp.rough, 0.22))
            height = m.math('MULTIPLY', mp.disp, 0.00012)
            return {'color': color, 'rough': rough, 'metal': 0.0, 'height': height}
        s = {'X': (8.0, 520.0, 520.0), 'Y': (520.0, 8.0, 520.0), 'Z': (520.0, 520.0, 8.0)}[along]
        b = {'X': (1.2, 22.0, 22.0), 'Y': (22.0, 1.2, 22.0), 'Z': (22.0, 22.0, 1.2)}[along]
        p = {'X': (30.0, 2600.0, 2600.0), 'Y': (2600.0, 30.0, 2600.0), 'Z': (2600.0, 2600.0, 30.0)}[along]
        grain = noise(m, scaled(m, sep, *s), detail=6.0, distortion=1.4)
        figure = noise(m, scaled(m, sep, *b), detail=3.0)
        pores = ramp(m, noise(m, scaled(m, sep, *p), detail=2.0), 0.62, 0.7)
        tone = m.mix(grain, core.hex_linear(dark), core.hex_linear(light))
        tone = m.mix(m.math('MULTIPLY', figure, 0.45), tone, core.hex_linear(dark))
        color = m.mix(m.math('MULTIPLY', pores, 0.5), tone, core.hex_linear(dark))
        rough = m.math('ADD', 0.3 if ebonized else 0.36, m.math('ADD', m.math('MULTIPLY', grain, 0.1), m.math('MULTIPLY', pores, 0.12)))
        height = m.math('SUBTRACT', m.math('MULTIPLY', grain, 0.00006), m.math('MULTIPLY', pores, 0.00005))
        return {'color': color, 'rough': rough, 'metal': 0.0, 'height': height}
    return surface


_FS_METALS = {'brass': (0x8a6a32, 0xc9a55a), 'gold': (0xb9893a, 0xe8bd62), 'neutral': (0xa9a9ab, 0xe0e0e2)}


def fs_metal(kind='brass', rough=0.22, brushed=None):
    """Brass, gold or a neutral (tintable) metal: polished, or brushed along an axis ('X', 'Y' or 'Z')."""
    lo, hi = _FS_METALS[kind]

    def surface(m, sep, tag):
        if FS_CC0:
            mp = _FsMaps(m, 'metal', _fs_obj_vec(m, sep, 0.25), tag=tag)
            # Metal 042 A is worn and mottled: on the kit's polished pieces it only shades the top of the range and breaks
            # up the roughness a little.
            color = m.mix(m.math('ADD', 0.78, m.math('MULTIPLY', mp.lum, 0.22)), core.hex_linear(lo), core.hex_linear(hi))
            r = m.math('ADD', rough - 0.03, m.math('MULTIPLY', mp.rough, 0.08))
            height = m.math('MULTIPLY', mp.disp, 0.000008)
            return {'color': color, 'rough': r, 'metal': 1.0, 'height': height}
        if brushed:
            st = {'X': (8.0, 700.0, 700.0), 'Y': (700.0, 8.0, 700.0), 'Z': (700.0, 700.0, 8.0)}[brushed]
            streak = noise(m, scaled(m, sep, *st), detail=4.0)
        else:
            tc = m.node('ShaderNodeTexCoord')
            streak = noise(m, tc.outputs['Object'], scale=300.0, detail=3.0)
        color = m.mix(m.math('MULTIPLY', streak, 0.35), core.hex_linear(hi), core.hex_linear(lo))
        r = m.math('ADD', rough, m.math('MULTIPLY', m.math('SUBTRACT', streak, 0.5), 0.12))
        height = m.math('MULTIPLY', streak, 0.000006 if brushed else 0.0)
        return {'color': color, 'rough': r, 'metal': 1.0, 'height': height}
    return surface


def fs_velvet(hex_color):
    """Velvet in a colour: deep, matte, a fine pile."""
    def surface(m, sep, tag):
        base = core.hex_linear(hex_color)
        deep = tuple(c * 0.45 for c in base[:3]) + (1.0,)
        if FS_CC0:
            mp = _FsMaps(m, 'velvet', _fs_obj_vec(m, sep, 0.25), tag=tag)
            return {'color': m.mix(mp.lum, deep, base), 'rough': m.math('ADD', 0.8, m.math('MULTIPLY', mp.rough, 0.18)), 'metal': 0.0,
                    'height': m.math('MULTIPLY', mp.disp, 0.0001)}
        tc = m.node('ShaderNodeTexCoord')
        pile = noise(m, tc.outputs['Object'], scale=2600.0, detail=2.0)
        crush = noise(m, tc.outputs['Object'], scale=60.0, detail=3.0)
        color = m.mix(m.math('ADD', m.math('MULTIPLY', pile, 0.4), m.math('MULTIPLY', crush, 0.5)), deep, base)
        return {'color': color, 'rough': 0.92, 'metal': 0.0, 'height': m.math('MULTIPLY', pile, 0.00005)}
    return surface


def fs_leather(which='26'):
    """Leather in a light neutral, so the game's tint gives each festival's strap its colour: a pebbled grain."""
    lo, hi = (0x9b948c, 0xcdc7bf) if which == '26' else (0x8a847d, 0xb7b1a9)

    def surface(m, sep, tag):
        if FS_CC0:
            mp = _FsMaps(m, 'leather' + which, _fs_obj_vec(m, sep, 0.3), tag=tag)
            return {'color': m.mix(mp.lum, core.hex_linear(lo), core.hex_linear(hi)),
                    'rough': m.math('ADD', 0.38, m.math('MULTIPLY', mp.rough, 0.3)), 'metal': 0.0,
                    'height': m.math('MULTIPLY', mp.disp, 0.00006)}
        tc = m.node('ShaderNodeTexCoord')
        vor = m.node('ShaderNodeTexVoronoi', Scale=700.0 if which == '26' else 1100.0)
        vor.feature = 'DISTANCE_TO_EDGE'
        m.link(tc.outputs['Object'], vor.inputs['Vector'])
        crease = ramp(m, vor.outputs['Distance'], 0.02, 0.12)
        mott = noise(m, tc.outputs['Object'], scale=80.0, detail=3.0)
        color = m.mix(m.math('ADD', m.math('MULTIPLY', crease, 0.2), m.math('MULTIPLY', mott, 0.6)), core.hex_linear(lo), core.hex_linear(hi))
        rough = m.math('ADD', 0.46, m.math('MULTIPLY', m.math('SUBTRACT', 1.0, crease), 0.08))
        return {'color': color, 'rough': rough, 'metal': 0.0, 'height': m.math('MULTIPLY', crease, 0.00004)}
    return surface


def fs_cork():
    """Cork board: warm granules with dark specks."""
    def surface(m, sep, tag):
        if FS_CC0:
            mp = _FsMaps(m, 'cork', _fs_obj_vec(m, sep, 0.4), tag=tag)
            return {'color': mp.color, 'rough': m.math('ADD', 0.72, m.math('MULTIPLY', mp.rough, 0.24)), 'metal': 0.0,
                    'height': m.math('MULTIPLY', mp.disp, 0.0005)}
        tc = m.node('ShaderNodeTexCoord')
        gran = noise(m, tc.outputs['Object'], scale=900.0, detail=4.0)
        speck = ramp(m, noise(m, tc.outputs['Object'], scale=3200.0, detail=2.0), 0.64, 0.7)
        tone = m.mix(gran, core.hex_linear(0x8f6238), core.hex_linear(0xc2935c))
        color = m.mix(m.math('MULTIPLY', speck, 0.8), tone, core.hex_linear(0x4a2e18))
        return {'color': color, 'rough': 0.88, 'metal': 0.0, 'height': m.math('SUBTRACT', m.math('MULTIPLY', gran, 0.0003), m.math('MULTIPLY', speck, 0.0002))}
    return surface


def fs_flat(hex_color, rough=0.1, metal=0.0):
    """A plain finish: enamel, a cut stone, a backing board."""
    def surface(m, sep, tag):
        return {'color': core.hex_linear(hex_color), 'rough': rough, 'metal': metal, 'height': 0.0}
    return surface


def fs_chrome(rough=0.07, hex_color=0xdcdee3):
    """Chrome, silver or platinum (FL-8b): one polished metal, never textured, in either build (SPEC §9.2: "metallic 1,
    roughness 0.05-0.15, no texture")."""
    return fs_flat(hex_color, rough, 1.0)


def fs_onyx():
    """Polished black onyx with pale banded veins (FL-8b: The Seal's block, The Grindstone's trestle, The Sceptre's shaft
    and block). The CC0 build keeps ambientCG Onyx 013's own colour, a shade deeper (its veins are the point, as cork keeps
    its colour)."""
    def surface(m, sep, tag):
        if FS_CC0:
            mp = _FsMaps(m, 'onyx', _fs_obj_vec(m, sep, 0.18), tag=tag)
            color = m.mix(0.35, mp.color, (0.0, 0.0, 0.0, 1.0))   # a shade deeper: black onyx on the shelf
            return {'color': color, 'rough': m.math('ADD', 0.06, m.math('MULTIPLY', mp.rough, 0.14)), 'metal': 0.0,
                    'height': m.math('MULTIPLY', mp.disp, 0.00004)}
        tc = m.node('ShaderNodeTexCoord')
        wave = m.node('ShaderNodeTexWave', Scale=20.0, Distortion=14.0, Detail=6.0)
        wave.wave_type = 'BANDS'
        wave.bands_direction = 'Z'
        m.link(tc.outputs['Object'], wave.inputs['Vector'])
        veins = ramp(m, wave.outputs['Fac'], 0.94, 0.995)
        vein = m.math('MULTIPLY', veins, ramp(m, noise(m, tc.outputs['Object'], scale=25.0, detail=2.0), 0.35, 0.6))   # broken up
        cloud = noise(m, tc.outputs['Object'], scale=40.0, detail=3.0)
        tone = m.mix(m.math('MULTIPLY', cloud, 0.6), core.hex_linear(0x060607), core.hex_linear(0x18161b))
        color = m.mix(m.math('MULTIPLY', vein, 0.6), tone, core.hex_linear(0x6e665b))
        return {'color': color, 'rough': m.math('ADD', 0.07, m.math('MULTIPLY', cloud, 0.06)), 'metal': 0.0, 'height': 0.0}
    return surface


def fs_gold_felt():
    """The gold felt (FL-8b; v1.6's one new material, SPEC §9.4): a dense matte nap in gold for The Gold Pennant. The CC0
    build shades it with Poly Haven's Velour Velvet tinted gold (PLAN_FLOOK Corrections item 10: no new download)."""
    lo, hi = 0x9a6c18, 0xe6b84a

    def surface(m, sep, tag):
        if FS_CC0:
            mp = _FsMaps(m, 'velvet', _fs_obj_vec(m, sep, 0.2), tag=tag)
            return {'color': m.mix(mp.lum, core.hex_linear(lo), core.hex_linear(hi)),
                    'rough': m.math('ADD', 0.78, m.math('MULTIPLY', mp.rough, 0.18)), 'metal': 0.0,
                    'height': m.math('MULTIPLY', mp.disp, 0.00008)}
        tc = m.node('ShaderNodeTexCoord')
        fibre = noise(m, tc.outputs['Object'], scale=4200.0, detail=2.0)
        mott = noise(m, tc.outputs['Object'], scale=90.0, detail=3.0)
        color = m.mix(m.math('ADD', m.math('MULTIPLY', fibre, 0.35), m.math('MULTIPLY', mott, 0.55)), core.hex_linear(lo), core.hex_linear(hi))
        return {'color': color, 'rough': 0.9, 'metal': 0.0, 'height': m.math('MULTIPLY', fibre, 0.00004)}
    return surface


def fs_material(name, surfaces):
    """One material whose finish is picked per face by the fs_part attribute (0, 1, 2 ... in surfaces' order), so the
    whole mesh bakes to one texture set."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    part = m.node('ShaderNodeAttribute')
    part.attribute_type = 'GEOMETRY'
    part.attribute_name = 'fs_part'
    color = rough = metal = height = None
    for k, surf in enumerate(surfaces):
        s = surf(m, sep, name)
        if k == 0:
            color, rough, metal, height = s['color'], s['rough'], s['metal'], s['height']
            continue
        cmp = m.node('ShaderNodeMath', _operation='COMPARE')
        m.link(part.outputs['Fac'], cmp.inputs[0])
        cmp.inputs[1].default_value = float(k)
        cmp.inputs[2].default_value = 0.25
        w = cmp.outputs[0]
        color = m.mix(w, color, s['color'])
        rough = mix_float(m, w, rough, s['rough'])
        metal = mix_float(m, w, metal, s['metal'])
        height = mix_float(m, w, height, s['height'])
    m.set('Base Color', color)
    m.set('Roughness', rough)
    m.set('Metallic', metal)
    bump = m.node('ShaderNodeBump', Strength=1.0, Distance=1.0)
    if isinstance(height, (int, float)):
        bump.inputs['Height'].default_value = height
    else:
        m.link(height, bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    FS_CC0_USED.setdefault(name, FS_CC0_USED.get(name, set()))
    return m


def fs_glass():
    """FS_Glass: clear, faintly cool, blended (unbaked, so the game sees through it)."""
    g = core.Mat('FS_Glass')
    g.set('Base Color', core.hex_linear(0xe4ecee))
    g.set('Roughness', 0.03)
    g.set('Metallic', 0.0)
    g.set('Alpha', 0.12)
    g.set('Specular IOR Level', 0.6)
    g.m.use_backface_culling = True
    try:
        g.m.surface_render_method = 'BLENDED'
    except (AttributeError, TypeError):
        pass
    return g


# ================================================================== geometry helpers

def fs_tag(obj, part, tint=None):
    """Marks every face of obj with its finish (fs_part) and, for a tinted mesh, the tint mask (FS_Tint: RGB white,
    alpha 1 for the tinted zone, 0 for the metal zone). Returns obj."""
    me = obj.data
    a = me.attributes.get('fs_part') or me.attributes.new('fs_part', 'FLOAT', 'FACE')
    a.data.foreach_set('value', np.full(len(me.polygons), float(part), dtype=np.float32))
    if tint is not None:
        c = me.color_attributes.get('FS_Tint') or me.color_attributes.new('FS_Tint', 'BYTE_COLOR', 'CORNER')
        c.data.foreach_set('color', np.tile(np.array([1.0, 1.0, 1.0, float(tint)], dtype=np.float32), len(me.loops)))
    return obj


def fs_tag_faces(obj, test, part, tint=None):
    """Re-marks the faces passing test(polygon, mesh) with another finish (and tint value)."""
    me = obj.data
    a = me.attributes['fs_part']
    vals = np.empty(len(me.polygons), dtype=np.float32)
    a.data.foreach_get('value', vals)
    hit = [p.index for p in me.polygons if test(p, me)]
    for i in hit:
        vals[i] = float(part)
    a.data.foreach_set('value', vals)
    if tint is not None and 'FS_Tint' in me.color_attributes:
        c = me.color_attributes['FS_Tint']
        for i in hit:
            for li in me.polygons[i].loop_indices:
                c.data[li].color = (1.0, 1.0, 1.0, float(tint))
    return len(hit)


def fs_finish(objs, name, mat, angle=35.0, tint=False):
    """Joins tagged parts into one mesh on one material (plus FS_Glass where a part carries it), hard-surface shading,
    one box-packed UV atlas for the baked faces."""
    for o in objs:
        if not o.data.materials:
            core.assign(o, mat)
    obj = parts.finish(objs, name, angle)
    obj.data.uv_layers[0].name = 'UVMap'
    if tint:
        me = obj.data
        me.color_attributes.active_color_name = 'FS_Tint'
        try:
            me.color_attributes.render_color_index = me.color_attributes.active_color_index
        except AttributeError:
            pass
    return obj


def fs_decal_uv(obj, test):
    """UV1 "Decal": the faces passing test(polygon, mesh) map planar over their joint X-Z bounds onto 0..1 as seen from
    the front (-Y): in Blender u left to right, v bottom to top; the glTF export flips v, so in the glb and in Unreal the
    region reads as an upright image (v top to bottom). Every other face sits at Blender (-1, -1), glTF and Unreal (-1, 2):
    outside 0..1. UV0 stays the active (baked) map."""
    me = obj.data
    base = me.uv_layers[0]
    layer = me.uv_layers.new(name='Decal')
    me.uv_layers.active = base
    base.active_render = True
    faces = [p for p in me.polygons if test(p, me)]
    vs = [me.vertices[me.loops[li].vertex_index].co for p in faces for li in p.loop_indices]
    x0, x1 = min(v.x for v in vs), max(v.x for v in vs)
    z0, z1 = min(v.z for v in vs), max(v.z for v in vs)
    uv = np.full(len(me.loops) * 2, -1.0, dtype=np.float32)
    for p in faces:
        for li in p.loop_indices:
            v = me.vertices[me.loops[li].vertex_index].co
            uv[2 * li] = (v.x - x0) / (x1 - x0)
            uv[2 * li + 1] = (v.z - z0) / (z1 - z0)
    layer.data.foreach_set('uv', uv)
    return len(faces)


def fs_lathe(name, profile, matrix=None, segments=16, toward=None):
    """A lathe (core.lathe: (radius, z) bottom to top round Z) with consistent normals, outward on a closed profile; an
    open one (a dome on a face) is turned toward the given side first (+Z: its top). Then moved by matrix."""
    obj = core.lathe(name, profile, segments=segments)
    core.orient_normals(obj, toward)
    if matrix is not None:
        obj.data.transform(matrix)
    return obj


def fs_front(y_max, nz=0.9):
    """A test for faces facing -Y whose centre lies in front of y_max."""
    return lambda p, me: p.normal.y < -nz and p.center.y < y_max


def fs_rect_frame(name, w, d, h, bar, z0):
    """A square brass frame (a ring w x d outside, bar wide) from z0 up h."""
    outer = [(-w / 2, -d / 2), (w / 2, -d / 2), (w / 2, d / 2), (-w / 2, d / 2)]
    inner = [(-w / 2 + bar, -d / 2 + bar), (-w / 2 + bar, d / 2 - bar), (w / 2 - bar, d / 2 - bar), (w / 2 - bar, -d / 2 + bar)]
    return parts.outline_prism(name, outer, [inner], z0, z0 + h)


def fs_split_unbaked(objs, store):
    """Each mesh's unbaked slots (FS_Glass) are split off and hidden from the bake; fs_join_unbaked joins them back."""
    store.clear()
    for o in objs:
        idx = [i for i, s in enumerate(o.material_slots) if s.material and s.material.name in FS_UNBAKED]
        if not idx:
            continue
        dup = o.copy()
        dup.data = o.data.copy()
        core.link(dup)
        dup.name = o.name + '__unbaked'
        _fs_delete_faces(o, lambda f: f.material_index in idx)
        _fs_delete_faces(dup, lambda f: f.material_index not in idx)
        _fs_prune_slots(o)
        _fs_prune_slots(dup)
        dup.hide_render = True
        store[o.name] = dup
    return list(objs)


def fs_join_unbaked(objs, store):
    out = []
    for o in objs:
        dup = store.get(o.name)
        name = o.name
        if dup is not None:
            dup.hide_render = False
            a = dup.data.attributes.get('fs_part')
            if a is not None:
                dup.data.attributes.remove(a)
            o = core.join([o, dup], name)
            o.name = name
            o.data.name = name
        out.append(o)
    return out


def _fs_delete_faces(obj, test):
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if test(f)], context='FACES')
    bm.to_mesh(obj.data)
    bm.free()


def _fs_prune_slots(obj):
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


def fs_copy(obj, name=None):
    c = obj.copy()
    core.link(c)
    if name:
        c.name = name
    return c


def fs_review_tint(obj, tinted_hex, metal_hex):
    """Review only: a copy of obj's material multiplied by the game's tint (FS_Tint's alpha picks the strap or enamel
    colour, else the metal's), as FL-9's material will. Returns obj."""
    me = obj.data
    if not me.color_attributes:
        return obj
    layer = me.color_attributes[0].name
    for i, slot in enumerate(obj.material_slots):
        mat = me.materials[i] if i < len(me.materials) else None   # the mesh's own, never a copy's earlier tint
        if mat is None or mat.name in FS_UNBAKED or not mat.node_tree:
            continue
        mt = mat.copy()
        nt = mt.node_tree
        bsdf = next(n for n in nt.nodes if n.type == 'BSDF_PRINCIPLED')
        src = bsdf.inputs['Base Color'].links[0].from_socket if bsdf.inputs['Base Color'].is_linked else None
        if src is None:
            continue
        vc = nt.nodes.new('ShaderNodeVertexColor')
        vc.layer_name = layer
        pick = nt.nodes.new('ShaderNodeMix')
        pick.data_type = 'RGBA'
        nt.links.new(vc.outputs['Alpha'], pick.inputs[0])
        pick.inputs[6].default_value = core.hex_linear(metal_hex)
        pick.inputs[7].default_value = core.hex_linear(tinted_hex)
        mul = nt.nodes.new('ShaderNodeMix')
        mul.data_type = 'RGBA'
        mul.blend_type = 'MULTIPLY'
        mul.inputs[0].default_value = 1.0
        nt.links.new(src, mul.inputs[6])
        nt.links.new(pick.outputs[2], mul.inputs[7])
        nt.links.new(mul.outputs[2], bsdf.inputs['Base Color'])
        obj.material_slots[i].link = 'OBJECT'
        obj.material_slots[i].material = mt
    return obj


# ================================================================== the shelf

def shelf():
    mat = fs_material('fs_shelf', [fs_walnut('X'), fs_walnut('Z'), fs_metal('brass', 0.26, 'X')])
    W, D = SHELF_W, SHELF_D
    inner = W - 2 * SIDE_T
    out = []
    for s in (-1, 1):   # the sides, floor to the top board, grain upright
        out.append(fs_tag(parts.rbox('side', (s * (W / 2 - SIDE_T / 2), 0.0, TOP_Z / 2), (SIDE_T, D, TOP_Z), 0.003, 2), 1))
    for z in (KICK, TIER_Z[0] + TIER):   # the two tier boards between the sides
        out.append(fs_tag(parts.rbox('board', (0.0, -BACK_T / 2, z + BOARD_T / 2), (inner, D - BACK_T, BOARD_T), 0.003, 2), 0))
        out.append(fs_tag(parts.rbox('inlay', (0.0, -D / 2 + 0.0002, z + BOARD_T / 2), (inner - 0.03, 0.0012, 0.003), 0.0), 2))
    out.append(fs_tag(parts.rbox('top', (0.0, -0.006, TOP_Z + TOP_T / 2), (W + 0.03, D + 0.012, TOP_T), 0.004, 2), 0))
    out.append(fs_tag(parts.rbox('inlay_top', (0.0, -(D + 0.012) / 2 - 0.006 + 0.0002, TOP_Z + TOP_T / 2), (W, 0.0012, 0.0035), 0.0), 2))
    out.append(fs_tag(parts.rbox('back', (0.0, D / 2 - BACK_T / 2, (KICK + TOP_Z) / 2), (inner, BACK_T, TOP_Z - KICK), 0.0), 1))
    out.append(fs_tag(parts.rbox('kick', (0.0, -D / 2 + 0.03, KICK / 2), (inner, 0.018, KICK), 0.002, 1), 0))
    return fs_finish(out, 'SM_FS_Shelf', mat)


def plinth():
    mat = fs_material('fs_plinth', [fs_walnut('X'), fs_walnut('X', ebonized=True)])
    w = PLINTH_W
    out = [fs_tag(parts.rbox('base', (0.0, 0.0, 0.006), (w, w, 0.012), 0.002, 2), 0),
           fs_tag(parts.rbox('body', (0.0, 0.0, 0.012 + 0.023), (w - 0.02, w - 0.02, 0.046), 0.0015, 1), 1),
           fs_tag(parts.rbox('cap', (0.0, 0.0, PLINTH_H - 0.006), (w - 0.006, w - 0.006, 0.012), 0.002, 2), 0)]
    return fs_finish(out, 'SM_FS_Plinth', mat)


def plate():
    mat = fs_material('fs_plate', [fs_metal('brass', 0.24, 'X')])
    w, h, t = PLATE
    obj = fs_finish([fs_tag(parts.rbox('plate', (0.0, -t / 2, 0.0), (w, t, h), 0.0007, 1), 0)], 'SM_FS_Plate', mat)
    fs_decal_uv(obj, fs_front(-t + 0.0001, 0.99))
    return obj


def _case(name, mat, glass, base_w, glass_w, glass_h):
    """A case's walnut base and its brass-framed glass box; returns (parts, the base's top)."""
    bt = 0.03
    out = [fs_tag(parts.rbox('base', (0.0, 0.0, bt / 2), (base_w, base_w, bt), 0.003, 2), 0)]
    fw = glass_w + 0.006
    out.append(fs_tag(fs_rect_frame('foot', fw, fw, 0.008, 0.005, bt), 1))
    out.append(fs_tag(fs_rect_frame('rim', fw, fw, 0.005, 0.005, bt + glass_h - 0.004), 1))
    for sx in (-1, 1):
        for sy in (-1, 1):
            out.append(fs_tag(parts.rbox('post', (sx * (fw / 2 - 0.0025), sy * (fw / 2 - 0.0025), bt + glass_h / 2), (0.004, 0.004, glass_h), 0.0), 1))
    pane = fs_tag(parts.rbox('glass', (0.0, 0.0, bt + 0.0005 + glass_h / 2), (glass_w, glass_w, glass_h - 0.001), 0.0), 0)
    core.assign(pane, glass)
    out.append(pane)
    return out, bt


def case_ring(glass):
    mat = fs_material('fs_case_ring', [fs_walnut('X'), fs_metal('brass', 0.2), fs_velvet(0x5e1424), fs_metal('gold', 0.14),
                                       fs_flat(0x7a0f22, 0.04)])
    out, top = _case('SM_FS_CaseRing', mat, glass, 0.12, 0.10, 0.10)
    cushion_h = 0.026
    out.append(fs_tag(parts.rbox('cushion', (0.0, 0.0, top + cushion_h / 2), (0.08, 0.064, cushion_h), 0.009, 3), 2))
    # The ring stands in the cushion facing the front: a band round the Y axis, its head on top.
    R, band_w, band_t = 0.0115, 0.0072, 0.0028
    zc = top + cushion_h - 0.005 + R
    path = [(R * math.sin(2 * math.pi * k / 28), 0.0, zc + R * math.cos(2 * math.pi * k / 28)) for k in range(28)]
    prof = [(-band_t / 2, -band_w / 2), (band_t / 2, -band_w / 2), (band_t / 2, band_w / 2), (-band_t / 2, band_w / 2)]
    band = core.sweep('band', path, prof, closed=True)
    out.append(fs_tag(band, 3))
    head_z = zc + R + band_t / 2 - 0.0006
    bezel = fs_lathe('bezel', [(0.0, 0.0), (0.0062, 0.0), (0.0068, 0.0016), (0.0064, 0.0034), (0.0052, 0.0036), (0.0046, 0.0024),
                               (0.0, 0.0024)], Matrix.Translation((0.0, 0.0, head_z)), segments=12)
    out.append(fs_tag(bezel, 3))
    stone = fs_lathe('stone', [(0.0, 0.0), (0.0052, 0.0), (0.0052, 0.0012), (0.0036, 0.0034), (0.0, 0.0044)],
                     Matrix.Translation((0.0, 0.0, head_z + 0.0024)), segments=8)
    stone.data.shade_flat()
    out.append(fs_tag(stone, 4))
    return fs_finish(out, 'SM_FS_CaseRing', mat, angle=40.0)


def case_bracelet(glass):
    mat = fs_material('fs_case_bracelet', [fs_walnut('X'), fs_metal('brass', 0.2), fs_velvet(0x1d3577), fs_metal('gold', 0.14)])
    out, top = _case('SM_FS_CaseBracelet', mat, glass, 0.15, 0.13, 0.12)
    br = 0.022   # the bolster: a velvet pillow along X, the bracelet wrapped round it facing the front
    zb = top + br
    out.append(fs_tag(parts.rbox('bolster', (0.0, 0.0, zb), (0.086, 2 * br, 2 * br), 0.02, 4), 2))
    R = br + 0.0016
    n = 18
    for k in range(n):
        th = 2 * math.pi * k / n
        if math.cos(th) < -0.55:   # the links under the bolster are hidden in the base
            continue
        r_hat = Vector((0.0, -math.cos(th), math.sin(th)))
        t_hat = Vector((0.0, math.sin(th), math.cos(th)))
        rot = Matrix((Vector((1.0, 0.0, 0.0)), r_hat, -t_hat)).transposed()
        wide = 0.017 if k % 2 == 0 else 0.015
        c = Vector((0.0, 0.0, zb)) + r_hat * R
        out.append(fs_tag(parts.rbox('link', c, (wide, 0.0032, 2 * math.pi * R / n * 0.92), 0.0011, 1, rot=rot), 3))
    medal = fs_lathe('medal', [(0.0092, 0.0), (0.0085, 0.0012), (0.0055, 0.0024), (0.0, 0.0028)],
                     Matrix.Translation((0.0, -(R + 0.0016), zb)) @ parts.rot_x(90), segments=16, toward=(0.0, 0.0, 1.0))
    out.append(fs_tag(medal, 3))
    return fs_finish(out, 'SM_FS_CaseBracelet', mat, angle=40.0)


# ================================================================== the build

MESHES = ['SM_FS_Shelf', 'SM_FS_Plinth', 'SM_FS_Plate', 'SM_FS_CaseRing', 'SM_FS_CaseBracelet']
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {'fs_shelf': 2048, 'fs_plinth': 1024, 'fs_plate': 256, 'fs_case_ring': 1024, 'fs_case_bracelet': 1024}
AO_DISTANCE = 0.02
REVIEW_VIEWS = [('front', -14, 9, 2.0), ('side', 48, 22, 2.0), ('detail', -20, 12, 0.42, (SLOTS[8][0], SLOT_Y, TIER_Z[1] + 0.1))]
_SPLIT = {}


def build():
    core.reset()
    FS_CC0_USED.clear()
    glass = fs_glass()
    return [shelf(), plinth(), plate(), case_ring(glass.m), case_bracelet(glass.m)]


def bake_parts(objs):
    fs_prebake('fest_shelf', MESHES, objs, TEXTURE_SIZE, TEXTURE_SIZES)
    return fs_split_unbaked(objs, _SPLIT)


def assemble(objs):
    out = fs_join_unbaked(objs, _SPLIT)
    fs_postbake('fest_shelf', MESHES, out)
    return out


def pose_for_review(objs):
    """The shelf dressed: ten plinths with their plates, the two cases on the middle tier's right."""
    by = {o.name: o for o in objs}
    pl, pt = by['SM_FS_Plinth'], by['SM_FS_Plate']
    extra = []
    for i, (x, y, z) in enumerate(SLOTS):
        p = pl if i == 0 else fs_copy(pl)
        p.location = (x, y, z)
        q = pt if i == 0 else fs_copy(pt)
        q.location = (x + PLATE_AT[0], y + PLATE_AT[1], z + PLATE_AT[2])
        if i:
            extra += [p, q]
    for name, i in (('SM_FS_CaseRing', 7), ('SM_FS_CaseBracelet', 8)):
        x, y, z = SLOTS[i]
        by[name].location = (x, y, z + PLINTH_H)
    objs.extend(extra)
    bpy.context.view_layer.update()   # the review frames the dressed shelf
