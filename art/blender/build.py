"""Builds SHORT STACK props: model, bake textures, export glTF to unreal/Art/Meshes, render review sheets to art/review.

    python3 art/blender/build.py [asset ...]          (bpy from PyPI)
    blender -b -P art/blender/build.py -- [asset ...]  (any Blender 4.2+)

With no asset names, builds everything in ASSETS. --no-review skips the review renders.
"""
import importlib
import os
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

from artkit import core, review  # noqa: E402

ASSETS = ['energy_can']


def args():
    a = sys.argv
    return a[a.index('--') + 1:] if '--' in a else a[1:]


def run(name, do_review=True):
    mod = importlib.import_module(f'assets.{name}')
    t0 = time.time()
    objs = mod.build()
    tris, dims = core.stats(objs)
    print(f'[{name}] built {len(objs)} object(s), {tris} triangles, {dims.x * 100:.1f} x {dims.y * 100:.1f} x {dims.z * 100:.1f} cm')
    for o in objs:
        core.bake(o, o.name, size=getattr(mod, 'TEXTURE_SIZE', 2048), ao_distance=getattr(mod, 'AO_DISTANCE', 0.01))
        path = core.export_glb([o], o.name)
        print(f'[{name}] exported {os.path.relpath(path, core.ROOT)}')
    if do_review:
        path = review.sheet(objs, name, views=getattr(mod, 'REVIEW_VIEWS', None))
        print(f'[{name}] review sheet {os.path.relpath(path, core.ROOT)}')
    print(f'[{name}] done in {time.time() - t0:.0f}s')


def main():
    a = args()
    do_review = '--no-review' not in a
    names = [x for x in a if not x.startswith('--')] or ASSETS
    for n in names:
        run(n, do_review)


main()
