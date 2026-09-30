"""Texture maps computed per texel in numpy, for detail too intricate for shader nodes (skin, nails).

1. position_maps() bakes every texel's position and normal, in any space given as vertex attributes.
2. The asset computes color, roughness and height from those, with the helpers below.
3. height_to_normal() turns the height into a tangent-space normal map on the mesh's own UVs.
4. save() writes images that an ordinary material uses, and the usual bake finishes the job.
"""
import os

import bpy
import numpy as np

from . import core


# ------------------------------------------------------------------ baking positions

def position_maps(obj, size, positions, normals):
    """Bakes per-texel positions and normals (N x 3 arrays per vertex, in any space) at size x size.

    Returns (P, N, valid): (H, W, 3), (H, W, 3) and a (H, W) coverage mask, rows bottom-up like
    Blender images.
    """
    me = obj.data
    for name, data, offset in (('_bake_p', positions, 0.5), ('_bake_n', normals, 0.0)):
        attr = me.color_attributes.get(name) or me.color_attributes.new(name, 'FLOAT_COLOR', 'POINT')
        rgba = np.ones((len(me.vertices), 4), dtype=np.float32)
        rgba[:, :3] = np.asarray(data, dtype=np.float32) * (1.0 if name == '_bake_p' else 0.5) + (offset if name == '_bake_p' else 0.5)
        attr.data.foreach_set('color', rgba.ravel())
    sc = bpy.context.scene
    added_slot = not obj.material_slots
    if added_slot:
        me.materials.append(None)
    saved = [s.material for s in obj.material_slots]
    out = {}
    for key, attr_name in (('P', '_bake_p'), ('N', '_bake_n'), ('valid', None)):
        m = core.Mat(f'_bake_{key}')
        em = m.node('ShaderNodeEmission')
        if attr_name:
            a = m.node('ShaderNodeAttribute')
            a.attribute_name = attr_name
            m.link(a.outputs['Color'], em.inputs['Color'])
        else:
            em.inputs['Color'].default_value = (1.0, 1.0, 1.0, 1.0)
        m.link(em.outputs['Emission'], m.out.inputs['Surface'])
        img = bpy.data.images.new(f'_bake_{key}', size, size, alpha=False, float_buffer=True)
        img.colorspace_settings.name = 'Non-Color'
        tex = m.nt.nodes.new('ShaderNodeTexImage')
        tex.image = img
        m.nt.nodes.active = tex
        for slot in obj.material_slots:
            slot.material = m.m
        core.select_only([obj])
        sc.render.bake.margin = 0
        sc.render.bake.use_clear = True
        sc.cycles.samples = 1
        bpy.ops.object.bake(type='EMIT')
        a = np.empty(size * size * 4, dtype=np.float32)
        img.pixels.foreach_get(a)
        out[key] = a.reshape(size, size, 4)[:, :, :3]
        bpy.data.images.remove(img)
    for slot, mat in zip(obj.material_slots, saved):
        slot.material = mat
    if added_slot:
        me.materials.clear()
    valid = out['valid'][:, :, 0] > 0.5
    P = out['P'] - 0.5
    N = out['N'] * 2.0 - 1.0
    n = np.linalg.norm(N, axis=2, keepdims=True)
    N = np.where(n > 1e-6, N / np.maximum(n, 1e-6), 0.0)
    return P, N, valid


# ------------------------------------------------------------------ noise and shapes

def _hash(ix, iy, iz, seed):
    h = (ix * np.int64(73856093)) ^ (iy * np.int64(19349663)) ^ (iz * np.int64(83492791)) ^ np.int64(seed * 2654435761)
    h = (h ^ (h >> 13)) * np.int64(1274126177)
    h = h ^ (h >> 16)
    return (h & 0xFFFFFF).astype(np.float32) / float(0xFFFFFF)


def value_noise(P, scale, seed=0, octaves=1, gain=0.5, chunk=1 << 20):
    """Smooth 3D value noise in 0..1 at points P (N x 3, meters); scale = feature size in meters."""
    P = np.asarray(P, dtype=np.float64)
    out = np.zeros(len(P), dtype=np.float32)
    amp, total, s = 1.0, 0.0, scale
    for o in range(octaves):
        for c0 in range(0, len(P), chunk):
            q = P[c0:c0 + chunk] / s
            i = np.floor(q).astype(np.int64)
            f = (q - i).astype(np.float32)
            f = f * f * (3.0 - 2.0 * f)
            acc = np.zeros(len(q), dtype=np.float32)
            for dx in (0, 1):
                wx = f[:, 0] if dx else 1.0 - f[:, 0]
                for dy in (0, 1):
                    wy = f[:, 1] if dy else 1.0 - f[:, 1]
                    for dz in (0, 1):
                        wz = f[:, 2] if dz else 1.0 - f[:, 2]
                        acc += wx * wy * wz * _hash(i[:, 0] + dx, i[:, 1] + dy, i[:, 2] + dz, seed + o * 17)
            out[c0:c0 + chunk] += amp * acc
        total += amp
        amp *= gain
        s *= 0.5
    return out / total


def smoothstep(e0, e1, x):
    t = np.clip((x - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def polyline_distance(P2, pts):
    """Distance from 2D points (N x 2) to a polyline, and the parameter along it (0..1)."""
    pts = np.asarray(pts, dtype=np.float64)
    best = np.full(len(P2), np.inf)
    along = np.zeros(len(P2))
    lengths = np.linalg.norm(np.diff(pts, axis=0), axis=1)
    total = lengths.sum()
    start = 0.0
    for (a, b), L in zip(zip(pts[:-1], pts[1:]), lengths):
        ab = b - a
        t = np.clip(((P2 - a) @ ab) / max(ab @ ab, 1e-12), 0.0, 1.0)
        d = np.linalg.norm(P2 - (a + t[:, None] * ab), axis=1)
        better = d < best
        best = np.where(better, d, best)
        along = np.where(better, (start + t * L) / total, along)
        start += L
    return best, along


# ------------------------------------------------------------------ normals and output

def height_to_normal(height, P, valid, strength=1.0):
    """Tangent-space normal map (rows bottom-up, +X = +U, +Y = +V) from a height field in meters, using
    each texel's size in meters from the position map. Differences never reach across an island's edge."""
    H, W = height.shape

    def diff(axis):
        fwd = np.roll(height, -1, axis=axis) - height
        bwd = height - np.roll(height, 1, axis=axis)
        pf = np.linalg.norm(np.roll(P, -1, axis=axis) - P, axis=2)
        pb = np.linalg.norm(P - np.roll(P, 1, axis=axis), axis=2)
        vf = np.roll(valid, -1, axis=axis) & valid
        vb = np.roll(valid, 1, axis=axis) & valid
        texel = np.median(np.concatenate([pf[vf], pb[vb]])) if (vf.any() or vb.any()) else 1e-4
        vf &= pf < texel * 4
        vb &= pb < texel * 4
        num = np.where(vf, fwd, 0.0) + np.where(vb, bwd, 0.0)
        den = np.where(vf, pf, 0.0) + np.where(vb, pb, 0.0)
        return np.where(den > 0, num / np.maximum(den, 1e-12), 0.0)

    dx = diff(1) * strength
    dy = diff(0) * strength
    n = np.stack([-dx, -dy, np.ones_like(dx)], axis=2)
    n /= np.linalg.norm(n, axis=2, keepdims=True)
    return n


def dilate(img, valid, steps=8):
    """Grows island colors outward so mipmaps and filtering never pull in the empty background."""
    img = img.copy()
    v = valid.copy()
    for _ in range(steps):
        acc = np.zeros_like(img)
        cnt = np.zeros(v.shape)
        for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            vs = np.roll(v, (dy, dx), axis=(0, 1))
            acc += np.where(vs[..., None], np.roll(img, (dy, dx), axis=(0, 1)), 0.0)
            cnt += vs
        grow = (~v) & (cnt > 0)
        img[grow] = acc[grow] / cnt[grow][:, None]
        v = v | grow
    return img


def save(name, rgb, colorspace='sRGB'):
    """Writes an (H, W, 3) array (rows bottom-up) as an 8-bit PNG in art/build/textures; returns its path.

    colorspace 'sRGB': rgb is linear color, encoded to sRGB on the way out. 'Non-Color': written as is.
    """
    H, W = rgb.shape[:2]
    img = bpy.data.images.new(name, W, H, alpha=False)
    img.colorspace_settings.name = colorspace
    px = np.ones((H, W, 4), dtype=np.float32)
    if colorspace == 'sRGB':
        lin = np.clip(rgb, 0.0, 1.0)
        px[:, :, :3] = np.where(lin <= 0.0031308, lin * 12.92, 1.055 * np.power(np.maximum(lin, 0.0031308), 1 / 2.4) - 0.055)
    else:
        px[:, :, :3] = np.clip(rgb, 0.0, 1.0)
    img.pixels.foreach_set(px.ravel())  # a byte image stores exactly these (encoded) values
    path = os.path.join(core.BUILD_DIR, 'textures', f'{name}.png')
    os.makedirs(os.path.dirname(path), exist_ok=True)
    img.filepath_raw = path
    img.file_format = 'PNG'
    img.save()
    bpy.data.images.remove(img)
    return path
