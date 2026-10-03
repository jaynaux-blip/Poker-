"""Builds SHORT STACK props: model, bake textures, export glTF to unreal/Art/Meshes, render review sheets to art/review.

    python3 art/blender/build.py [asset ...]          (bpy from PyPI)
    blender -b -P art/blender/build.py -- [asset ...]  (any Blender 4.2+)

With no asset names, builds everything in ASSETS. --no-review skips the review renders.
--review-only renders the review from the exported .glb files without rebuilding, which also checks
that the export carries everything the asset needs.
"""
import importlib
import os
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import bpy  # noqa: E402
from artkit import core, review  # noqa: E402

ASSETS = ['energy_can', 'laptop', 'desk', 'mug', 'phone', 'chips', 'lamp', 'chair', 'mouse', 'arms', 'table', 'cards', 'folding_chair', 'dryer',
          # GearDrop's gear in the apartment, and the career's mementos.
          'monitor', 'tower', 'mic', 'webcam', 'lights', 'macro_pad', 'headphones', 'plant', 'curtains', 'router', 'trophy',
          # What the apartment's window looks out on.
          'tenement']


def args():
    a = sys.argv
    return a[a.index('--') + 1:] if '--' in a else a[1:]


def review_only(name):
    """Re-renders the review sheet from the exported meshes (asset module MESHES lists their names)."""
    mod = importlib.import_module(f'assets.{name}')
    t0 = time.time()
    core.reset()
    objs = []
    for mesh in mod.MESHES:
        bpy.ops.import_scene.gltf(filepath=os.path.join(core.EXPORT_DIR, f'{mesh}.glb'))
        obj = next(o for o in bpy.context.selected_objects if o.type == 'MESH')
        obj.name = mesh
        objs.append(obj)
    finish_review(mod, name, objs)
    print(f'[{name}] review done in {time.time() - t0:.0f}s')


def finish_review(mod, name, objs):
    if hasattr(mod, 'pose_for_review'):
        mod.pose_for_review(objs)
    path = review.sheet(objs, name, views=getattr(mod, 'REVIEW_VIEWS', None), screen_light=getattr(mod, 'REVIEW_SCREEN_LIGHT', True))
    print(f'[{name}] review sheet {os.path.relpath(path, core.ROOT)}')


def run(name, do_review=True):
    mod = importlib.import_module(f'assets.{name}')
    t0 = time.time()
    objs = mod.build()
    tris, dims = core.stats(objs)
    print(f'[{name}] built {len(objs)} object(s), {tris} triangles, {dims.x * 100:.1f} x {dims.y * 100:.1f} x {dims.z * 100:.1f} cm')
    # An asset can bake parts once and assemble the exported meshes from them (poker chips in stacks).
    # Assets that the game shades itself (cards) skip baking and export their plain materials.
    bake_objs = [] if getattr(mod, 'NO_BAKE', False) else getattr(mod, 'bake_parts', lambda o: o)(objs)
    for o in bake_objs:
        # Each mesh bakes alone: parts built at a shared origin (a laptop's base and lid) must not
        # shadow each other's occlusion.
        for other in bake_objs:
            other.hide_render = other is not o
        baked = core.bake(o, o.name, size=getattr(mod, 'TEXTURE_SIZE', 2048), ao_distance=getattr(mod, 'AO_DISTANCE', 0.01),
                          sizes=getattr(mod, 'TEXTURE_SIZES', None))
        for m in baked:
            m.use_backface_culling = not getattr(mod, 'DOUBLE_SIDED', True)  # exported as glTF doubleSided
    for o in bake_objs:
        o.hide_render = False
    if hasattr(mod, 'assemble'):
        objs = mod.assemble(bake_objs)
    for o in objs:
        path = core.export_glb([o], o.name)
        print(f'[{name}] exported {os.path.relpath(path, core.ROOT)}')
    if do_review:
        finish_review(mod, name, objs)
    print(f'[{name}] done in {time.time() - t0:.0f}s')


def main():
    a = args()
    do_review = '--no-review' not in a
    names = [x for x in a if not x.startswith('--')] or ASSETS
    for n in names:
        if '--review-only' in a:
            review_only(n)
        else:
            run(n, do_review)


main()
