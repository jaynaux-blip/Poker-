"""Organic modeling with signed distance fields: shapes built from smooth-blended primitives, meshed
with marching cubes.

Skin over bone and muscle is smooth unions of tapered capsules and ellipsoids. The blend radius sets
how softly neighbors merge (tight at a knuckle, broad at the heel of the hand). Every primitive
belongs to a bone, so the same field that shapes the mesh also skins it (bone_weights).

Needs numpy and scikit-image (pip install scikit-image).
"""
import math

import numpy as np

from . import core


def _frame(axis, up):
    """Orthonormal rows (x, y, z) with z along axis and y toward up."""
    z = np.asarray(axis, float)
    z = z / np.linalg.norm(z)
    up = np.asarray(up, float)
    x = np.cross(up, z)
    if np.linalg.norm(x) < 1e-9:
        x = np.cross((1.0, 0.0, 0.0), z)
    x /= np.linalg.norm(x)
    y = np.cross(z, x)
    return np.stack([x, y, z])


class Prim:
    """A primitive: a distance function over points (N x 3), its bounds, blend radius and owning bone."""

    def __init__(self, fn, lo, hi, k, bone, sub=False):
        self.fn, self.lo, self.hi, self.k, self.bone, self.sub = fn, np.asarray(lo, float), np.asarray(hi, float), k, bone, sub


def round_cone(a, b, ra, rb, k=0.004, bone=None, flat=1.0, up=(0.0, 0.0, 1.0)):
    """Tapered capsule from a (radius ra) to b (radius rb). flat < 1 squashes the section toward up
    (fingers are wider than they are thick)."""
    a, b = np.asarray(a, float), np.asarray(b, float)
    R = _frame(b - a, up)
    ba = b - a
    L = float(np.linalg.norm(ba))
    rr = ra - rb
    a2 = L * L - rr * rr

    def fn(p):
        # Exact round-cone distance (Quilez) in the local frame: a at the origin, b at (0, 0, L).
        q = (p - a) @ R.T
        q[:, 1] /= flat
        l2 = L * L
        y = q[:, 2] * L
        z = y - l2
        x2 = (q[:, 0] ** 2 + q[:, 1] ** 2) * l2 * l2
        y2 = y * y * l2
        z2 = z * z * l2
        kk = math.copysign(1.0, rr) * rr * rr * x2
        d_b = np.sqrt(x2 + z2) / l2 - rb
        d_a = np.sqrt(x2 + y2) / l2 - ra
        d_s = (np.sqrt(np.maximum(x2 * a2 / l2, 0.0)) + y * rr) / l2 - ra
        return np.where(np.sign(z) * a2 * z2 > kk, d_b, np.where(np.sign(y) * a2 * y2 < kk, d_a, d_s))

    m = max(ra, rb) + k
    return Prim(fn, np.minimum(a, b) - m, np.maximum(a, b) + m, k, bone)


def ellipsoid(c, r, k=0.004, bone=None, axes=None):
    """Ellipsoid centered at c with semi-axes r along axes (rows; default the world axes)."""
    c = np.asarray(c, float)
    r = np.asarray(r, float)
    Rm = np.eye(3) if axes is None else np.asarray(axes, float)

    def fn(p):
        q = (p - c) @ Rm.T
        k0 = np.linalg.norm(q / r, axis=1)
        k1 = np.linalg.norm(q / (r * r), axis=1)
        return k0 * (k0 - 1.0) / np.maximum(k1, 1e-12)

    m = float(r.max()) + k
    return Prim(fn, c - m, c + m, k, bone)


def rounded_box(c, half, radius, k=0.004, bone=None, axes=None):
    c = np.asarray(c, float)
    half = np.asarray(half, float)
    Rm = np.eye(3) if axes is None else np.asarray(axes, float)

    def fn(p):
        q = np.abs((p - c) @ Rm.T) - (half - radius)
        return np.linalg.norm(np.maximum(q, 0.0), axis=1) + np.minimum(q.max(axis=1), 0.0) - radius

    m = float(np.linalg.norm(half)) + k
    return Prim(fn, c - m, c + m, k, bone)


def torus(c, axis, R, r, k=0.004, bone=None, squash=1.0):
    """Ring of major radius R and tube radius r around axis through c; squash < 1 flattens the tube
    along the axis (a fabric fold is wider than it is tall)."""
    c = np.asarray(c, float)
    Rm = _frame(axis, (0.0, 0.0, 1.0) if abs(np.asarray(axis, float)[2]) < 0.9 * np.linalg.norm(axis) else (1.0, 0.0, 0.0))

    def fn(p):
        q = (p - c) @ Rm.T
        radial = np.sqrt(q[:, 0] ** 2 + q[:, 1] ** 2) - R
        return np.sqrt(radial ** 2 + (q[:, 2] / squash) ** 2) - r

    m = R + r / min(squash, 1.0) + k
    return Prim(fn, c - m, c + m, k, bone)


def carve(prim):
    """Marks a primitive as a smooth subtraction (a crease, a hollow)."""
    prim.sub = True
    return prim


def _smin(a, b, k):
    h = np.maximum(k - np.abs(a - b), 0.0) / k
    return np.minimum(a, b) - h * h * k * 0.25


def _smax_sub(a, b, k):
    """Smooth subtraction of b from a."""
    h = np.maximum(k - np.abs(-b - a), 0.0) / k
    return np.maximum(a, -b) + h * h * k * 0.25


def evaluate(prims, pts):
    """The combined field at arbitrary points (N x 3)."""
    pts = np.asarray(pts, float)
    F = np.full(len(pts), 1.0)
    for p in prims:
        d = p.fn(pts)
        F = _smax_sub(F, d, p.k) if p.sub else _smin(F, d, p.k)
    return F


def surface(prims, origin, direction, reach=0.1, steps=2000):
    """The first point where a ray from origin (outside the shape) meets the surface."""
    o = np.asarray(origin, float)
    d = np.asarray(direction, float)
    d = d / np.linalg.norm(d)
    ts = np.linspace(0.0, reach, steps)
    F = evaluate(prims, o + ts[:, None] * d)
    inside = np.nonzero(F < 0.0)[0]
    if not len(inside):
        return None
    i = inside[0]
    t0, t1, f0, f1 = ts[max(i - 1, 0)], ts[i], F[max(i - 1, 0)], F[i]
    t = t0 + (t1 - t0) * f0 / max(f0 - f1, 1e-12)
    return o + t * d


def field(prims, voxel, pad=0.004):
    """Evaluates the smooth union of prims on a grid; returns (values, origin)."""
    adds = [p for p in prims if not p.sub]
    lo = np.min([p.lo for p in adds], axis=0) - pad
    hi = np.max([p.hi for p in adds], axis=0) + pad
    shape = np.ceil((hi - lo) / voxel).astype(int) + 1
    F = np.full(shape, 1.0, dtype=np.float32)
    budget = 1 << 20  # points per evaluation, to keep memory flat whatever the resolution
    for p in prims:
        i0 = np.clip(np.floor((p.lo - lo) / voxel).astype(int), 0, shape - 1)
        i1 = np.clip(np.ceil((p.hi - lo) / voxel).astype(int) + 1, 1, shape)
        ys = lo[1] + voxel * np.arange(i0[1], i1[1])
        zs = lo[2] + voxel * np.arange(i0[2], i1[2])
        per_slab = max(1, budget // max(len(ys) * len(zs), 1))
        for a0 in range(i0[0], i1[0], per_slab):
            a1 = min(a0 + per_slab, i1[0])
            xs = lo[0] + voxel * np.arange(a0, a1)
            X, Y, Z = np.meshgrid(xs, ys, zs, indexing='ij')
            pts = np.stack([X.ravel(), Y.ravel(), Z.ravel()], axis=1)
            d = p.fn(pts).reshape(X.shape).astype(np.float32)
            block = F[a0:a1, i0[1]:i1[1], i0[2]:i1[2]]
            F[a0:a1, i0[1]:i1[1], i0[2]:i1[2]] = _smax_sub(block, d, p.k) if p.sub else _smin(block, d, p.k)
    return F, lo


def mesh(prims, voxel, name):
    """Marching-cubes mesh of the field, as a Blender object (outward-facing, watertight)."""
    from skimage.measure import marching_cubes
    import bpy
    F, lo = field(prims, voxel)
    verts, faces, _, _ = marching_cubes(F, level=0.0, spacing=(voxel, voxel, voxel), gradient_direction='ascent',
                                        allow_degenerate=False)
    verts += lo
    me = bpy.data.meshes.new(name)
    me.from_pydata(verts.tolist(), [], faces.tolist())
    me.validate()
    me.update()
    obj = core.link(bpy.data.objects.new(name, me))
    core.orient_normals(obj)  # outward, whatever winding marching cubes chose
    return obj


def rounded_cylinder(a, b, r, edge, k=0.004, bone=None, flat=1.0, up=(0.0, 0.0, 1.0)):
    """A cylinder from a to b with flat ends and rounded rims (a cuff, a band)."""
    a, b = np.asarray(a, float), np.asarray(b, float)
    R = _frame(b - a, up)
    half = float(np.linalg.norm(b - a)) / 2
    mid = (a + b) / 2

    def fn(p):
        q = (p - mid) @ R.T
        q[:, 1] /= flat
        dr = np.sqrt(q[:, 0] ** 2 + q[:, 1] ** 2) - (r - edge)
        dz = np.abs(q[:, 2]) - (half - edge)
        return np.minimum(np.maximum(dr, dz), 0.0) + np.sqrt(np.maximum(dr, 0.0) ** 2 + np.maximum(dz, 0.0) ** 2) - edge

    m = max(r, half) + k
    return Prim(fn, np.minimum(a, b) - m, np.maximum(a, b) + m, k, bone)


def bone_weights(prims, points, sigma=0.0015, limit=4):
    """Skin weights from the primitives: each point leans toward the bones whose primitives it lies on.

    Returns (bone names, weights array N x bones) with at most `limit` influences per point.
    """
    adds = [p for p in prims if not p.sub and p.bone]
    bones = sorted({p.bone for p in adds})
    index = {b: i for i, b in enumerate(bones)}
    P = np.asarray(points, float)
    D = np.stack([p.fn(P) for p in adds], axis=1)  # N x prims
    D = np.maximum(D, 0.0)
    dmin = D.min(axis=1, keepdims=True)
    W = np.exp(-((D - dmin) / sigma) ** 2)
    out = np.zeros((len(P), len(bones)))
    for j, p in enumerate(adds):
        out[:, index[p.bone]] = np.maximum(out[:, index[p.bone]], W[:, j])
    if limit and out.shape[1] > limit:
        cut = np.sort(out, axis=1)[:, -limit][:, None]
        out = np.where(out >= cut, out, 0.0)
    out /= np.maximum(out.sum(axis=1, keepdims=True), 1e-12)
    return bones, out
