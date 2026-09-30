"""SK_Arms: the player's first-person arms, a pair of hands in the sleeves of a charcoal hoodie.

The hands are sculpted as signed distance fields on an anatomical skeleton:
- metacarpals, phalanges and the thumb's saddle joint;
- the thenar and hypothenar pads, the web of the thumb;
- the tendons over the back of the hand, knuckles, and fingertip pads.

Bones use the Unreal Mannequin names (clavicle_r ... pinky_03_r), so standard animations retarget.
The same primitives that shape the hand weight it to those bones.

Coordinates (meters): origin between the shoulders, +Y ahead toward the desk, +Z up, the player's
right toward +X. The bind pose reaches forward, hands palm-down and relaxed, fingers apart.
"""
import math

import numpy as np
from mathutils import Matrix, Vector

from artkit import core, sdf, texmaps
from artkit.texmaps import polyline_distance, smoothstep, value_noise

VOXEL = 0.00045

# Right hand in hand space: wrist crease at the origin, +Y to the fingertips, +Z the back of the hand,
# +X toward the little finger. Per finger: MCP center, phalanx lengths, radii at MCP, PIP, DIP and tip,
# splay (deg, + toward the little finger), and the relaxed flexion at MCP, PIP, DIP (deg).
FINGERS = {
    'index': ((-0.0285, 0.0915, 0.0005), (0.0425, 0.0250, 0.0180), (0.0093, 0.0086, 0.0078, 0.0066), -7.0, (6, 14, 8)),
    'middle': ((-0.0090, 0.0965, 0.0015), (0.0465, 0.0285, 0.0190), (0.0096, 0.0089, 0.0080, 0.0068), 0.0, (7, 16, 9)),
    'ring': ((0.0110, 0.0925, 0.0000), (0.0440, 0.0270, 0.0188), (0.0090, 0.0083, 0.0075, 0.0064), 7.0, (8, 17, 9)),
    'pinky': ((0.0295, 0.0830, -0.0030), (0.0330, 0.0205, 0.0170), (0.0079, 0.0073, 0.0066, 0.0058), 15.0, (10, 18, 10)),
}
THUMB = [(-0.0165, 0.0120, -0.0055), (-0.0435, 0.0460, -0.0105), (-0.0575, 0.0745, -0.0125), (-0.0645, 0.0950, -0.0135)]
THUMB_R = (0.0128, 0.0106, 0.0099, 0.0086)
THUMB_UP = (-0.62, 0.05, 0.78)  # the direction the thumbnail faces


def finger_joints(mcp, lengths, splay, flex, ups=False):
    """MCP, PIP, DIP and tip positions for a finger, and the direction the nail faces (with ups=True,
    also each phalanx's own "back of the finger" direction)."""
    mcp = Vector(mcp)
    d = Vector((math.sin(math.radians(splay)), math.cos(math.radians(splay)), 0.0))
    side = Vector((0.0, 0.0, 1.0)).cross(d).normalized()  # flexion axis: positive angles curl toward the palm
    up = Vector((0.0, 0.0, 1.0))
    joints = [mcp]
    seg_ups = []
    bend = 0.0
    for length, f in zip(lengths, flex):
        bend += f
        rot = Matrix.Rotation(math.radians(bend), 3, side)
        joints.append(joints[-1] + (rot @ d) * length)
        seg_ups.append(rot @ up)
    nail_up = seg_ups[-1]
    return (joints, nail_up, seg_ups) if ups else (joints, nail_up)


def hand_prims(bone_suffix='_r'):
    """The right hand's primitives, in hand space."""
    B = lambda name: name + bone_suffix  # noqa: E731
    P = []
    # Wrist, running up into the sleeve.
    P.append(sdf.round_cone((0.0005, 0.006, -0.0005), (0.0, -0.085, -0.002), 0.0275, 0.0290, k=0.012, bone=B('lowerarm'), flat=0.66))
    # Carpals and the body of the palm.
    P.append(sdf.ellipsoid((0.001, 0.018, -0.0015), (0.0285, 0.022, 0.0135), k=0.014, bone=B('hand')))
    P.append(sdf.ellipsoid((0.0005, 0.056, -0.0015), (0.0355, 0.040, 0.0112), k=0.014, bone=B('hand')))
    # Metacarpals fan out from the carpals to the knuckles.
    for name, (mcp, lengths, radii, splay, flex) in FINGERS.items():
        base = (mcp[0] * 0.45, 0.018, -0.0005)
        P.append(sdf.round_cone(base, mcp, radii[0] * 0.78, radii[0] * 0.95, k=0.010, bone=B('hand'), flat=0.8))
    # Thenar and hypothenar pads, and the pad under the knuckles.
    P.append(sdf.ellipsoid((-0.0215, 0.033, -0.0095), (0.0165, 0.0265, 0.0115), k=0.012, bone=B('thumb_01'),
                           axes=_rot_z(-22)))
    P.append(sdf.ellipsoid((0.0255, 0.042, -0.0080), (0.0115, 0.034, 0.0100), k=0.012, bone=B('hand')))
    P.append(sdf.ellipsoid((0.0005, 0.0835, -0.0070), (0.0345, 0.0110, 0.0072), k=0.009, bone=B('hand')))
    # Tendons across the back of the hand, just under the skin.
    for name, (mcp, lengths, radii, splay, flex) in FINGERS.items():
        P.append(sdf.round_cone((mcp[0] * 0.35, 0.012, 0.0095), (mcp[0], mcp[1] - 0.007, mcp[2] + radii[0] * 0.7), 0.0018, 0.0016,
                                k=0.005, bone=B('hand')))
    # Fingers: phalanges, knuckle heads, fingertip pads, and the webs between them.
    for name, (mcp, lengths, radii, splay, flex) in FINGERS.items():
        j, nail_up = finger_joints(mcp, lengths, splay, flex)
        for s in range(3):
            bone = B(f'{name}_0{s + 1}')
            # Consecutive phalanges share the joint sphere, so they join cleanly with almost no blend;
            # a wide blend would swell a ring around every joint.
            P.append(sdf.round_cone(j[s], j[s + 1], radii[s], radii[s + 1], k=0.00001 if s else 0.006, bone=bone,
                                    flat=0.84, up=tuple(nail_up)))
        # Knuckle heads on the back of each joint.
        P.append(sdf.ellipsoid(tuple(Vector(mcp) + Vector((0.0, -0.002, radii[0] * 0.45))), (radii[0] * 0.85, radii[0] * 0.8, radii[0] * 0.62),
                               k=0.005, bone=B(f'{name}_01')))
        # PIP and DIP knuckles: low bumps on the back of the joint only.
        for s in (1, 2):
            P.append(sdf.ellipsoid(tuple(j[s] + nail_up * radii[s] * 0.42), (radii[s] * 0.72, radii[s] * 0.5, radii[s] * 0.42),
                                   k=0.0015, bone=B(f'{name}_0{s + 1}'), axes=_axes_along(j[s + 1] - j[s], nail_up)))
        # The fingertip pad, fuller underneath.
        tip_dir = (j[3] - j[2]).normalized()
        P.append(sdf.ellipsoid(tuple(j[2] + tip_dir * lengths[2] * 0.55 - nail_up * radii[3] * 0.35),
                               (radii[3] * 0.95, radii[3] * 1.25, radii[3] * 0.7), k=0.004, bone=B(f'{name}_03'),
                               axes=_axes_along(tip_dir, nail_up)))
    for a, b in (('index', 'middle'), ('middle', 'ring'), ('ring', 'pinky')):
        ma, mb = Vector(FINGERS[a][0]), Vector(FINGERS[b][0])
        c = (ma + mb) / 2 + Vector((0.0, 0.011, -0.004))
        P.append(sdf.ellipsoid(tuple(c), (0.0075, 0.0085, 0.0045), k=0.006, bone=B('hand')))
    # Thumb: metacarpal (fleshy, carrying the thenar muscle), phalanges, pad, and the web to the index finger.
    for s in range(3):
        P.append(sdf.round_cone(THUMB[s], THUMB[s + 1], THUMB_R[s], THUMB_R[s + 1], k=0.009 if s == 0 else 0.004,
                                bone=B(f'thumb_0{s + 1}'), flat=0.88, up=THUMB_UP))
    P.append(sdf.ellipsoid(tuple(Vector(THUMB[1]) + Vector((0.0, 0.0, 0.0035))), (0.0105, 0.0095, 0.0088), k=0.005, bone=B('thumb_02')))
    tdir = (Vector(THUMB[3]) - Vector(THUMB[2])).normalized()
    P.append(sdf.ellipsoid(tuple(Vector(THUMB[2]) + tdir * 0.011 - Vector(THUMB_UP).normalized() * 0.003), (0.0082, 0.0115, 0.0062),
                           k=0.004, bone=B('thumb_03'), axes=_axes_along(tdir, Vector(THUMB_UP).normalized())))
    P.append(sdf.ellipsoid((-0.0305, 0.0555, -0.0035), (0.0105, 0.0205, 0.0068), k=0.011, bone=B('hand'), axes=_rot_z(-32)))
    return P


def _rot_z(deg):
    return np.array(Matrix.Rotation(math.radians(deg), 3, 'Z').transposed())


def _axes_along(y_dir, z_dir):
    """Rows (x, y, z) of a frame with y along y_dir and z toward z_dir."""
    y = Vector(y_dir).normalized()
    z = Vector(z_dir)
    z = (z - y * z.dot(y)).normalized()
    x = y.cross(z)
    return np.array([tuple(x), tuple(y), tuple(z)])


# ------------------------------------------------------------------ arm skeleton (seat space)

SHOULDER = Vector((0.17, 0.0, 0.0))
ELBOW = Vector((0.215, 0.135, -0.25))
WRIST = Vector((0.152, 0.385, -0.238))
CLAVICLE = Vector((0.025, -0.005, 0.01))


def hand_to_seat(mirror=False):
    """Matrix placing hand space at the wrist, the fingers continuing the forearm, the back of the hand up."""
    y = (WRIST - ELBOW).normalized()
    up = Vector((0.0, 0.0, 1.0))
    z = (up - y * up.dot(y)).normalized()
    x = y.cross(z)
    m = Matrix((x, y, z)).transposed().to_4x4()
    m.translation = WRIST
    if mirror:
        m = Matrix.Scale(-1.0, 4, Vector((1.0, 0.0, 0.0))) @ m
    return m


def skeleton(side):
    """Bone name -> (head, tail, up) in seat space for one side ('r' or 'l')."""
    s = 1.0 if side == 'r' else -1.0
    mirror = side == 'l'

    def mx(v):
        return Vector((v.x * s, v.y, v.z))

    H = hand_to_seat(mirror)
    to_seat = lambda v: H @ Vector(v)  # noqa: E731
    dorsal = (H.to_3x3() @ Vector((0.0, 0.0, 1.0))).normalized()
    bones = {
        f'clavicle_{side}': (mx(CLAVICLE), mx(SHOULDER), Vector((0.0, 0.0, 1.0))),
        f'upperarm_{side}': (mx(SHOULDER), mx(ELBOW), Vector((0.0, -0.3, 1.0)).normalized()),
        f'lowerarm_{side}': (mx(ELBOW), mx(WRIST), Vector((0.0, 0.0, 1.0))),
        f'hand_{side}': (mx(WRIST), to_seat(FINGERS['middle'][0]), dorsal),
    }
    for name, (mcp, lengths, radii, splay, flex) in FINGERS.items():
        j, nail_up = finger_joints(mcp, lengths, splay, flex)
        up = (H.to_3x3() @ nail_up).normalized()
        for k in range(3):
            bones[f'{name}_0{k + 1}_{side}'] = (to_seat(j[k]), to_seat(j[k + 1]), up)
    thumb_up = (H.to_3x3() @ Vector(THUMB_UP)).normalized()
    for k in range(3):
        bones[f'thumb_0{k + 1}_{side}'] = (to_seat(THUMB[k]), to_seat(THUMB[k + 1]), thumb_up)
    return bones


# ------------------------------------------------------------------ sleeves

CUFF_END = 0.022   # the cuff's edge, this far up the forearm from the wrist crease
CUFF_LEN = 0.052


def sleeve_prims(side):
    """A charcoal hoodie sleeve over the right or left arm, in seat space."""
    s = 1.0 if side == 'r' else -1.0
    sh, el, wr = [Vector((v.x * s, v.y, v.z)) for v in (SHOULDER, ELBOW, WRIST)]
    fwd = (wr - el).normalized()
    P = []
    B = lambda n: f'{n}_{side}'  # noqa: E731
    cuff_edge = wr - fwd * CUFF_END
    cuff_top = cuff_edge - fwd * CUFF_LEN
    # Shoulder cap and upper arm.
    P.append(sdf.ellipsoid(tuple(sh + Vector((-0.012 * s, -0.008, 0.004))), (0.052, 0.050, 0.054), k=0.03, bone=B('clavicle')))
    P.append(sdf.round_cone(tuple(sh), tuple(el), 0.056, 0.053, k=0.03, bone=B('upperarm'), flat=0.92))
    # Forearm: loose, gathering into the cuff.
    gather = cuff_top - fwd * 0.03
    P.append(sdf.round_cone(tuple(el), tuple(gather), 0.053, 0.050, k=0.02, bone=B('lowerarm'), flat=0.9))
    P.append(sdf.round_cone(tuple(gather), tuple(cuff_top + fwd * 0.006), 0.050, 0.039, k=0.012, bone=B('lowerarm'), flat=0.88))
    # The ribbed cuff hugs the wrist and ends in a soft rolled edge.
    P.append(sdf.rounded_cylinder(tuple(cuff_top), tuple(cuff_edge), 0.0345, 0.0045, k=0.006, bone=B('lowerarm'), flat=0.8))
    # Compression folds where the sleeve bunches above the cuff: tilted, uneven rings.
    rnd = np.random.default_rng(3 if side == 'r' else 7)
    for i, t in enumerate((0.012, 0.030, 0.050, 0.074)):
        c = cuff_top - fwd * t
        tilt = Matrix.Rotation(math.radians(rnd.uniform(-22, 22)), 3, Vector((0.0, 0.0, 1.0)).cross(fwd).normalized())
        tilt = Matrix.Rotation(math.radians(rnd.uniform(-15, 15)), 3, Vector((0.0, 0.0, 1.0))) @ tilt
        axis = tilt @ fwd
        R = 0.040 + i * 0.0035
        P.append(sdf.torus(tuple(c), tuple(axis), R, 0.0075, k=0.012, bone=B('lowerarm'), squash=0.75))
    # Folds in the crook of the elbow, on its inner side.
    inner = (sh - el).normalized() + fwd
    inner = (inner - fwd * inner.dot(fwd)).normalized()
    for i, t in enumerate((-0.03, 0.0, 0.03)):
        c = el + fwd * t + inner * 0.045
        P.append(sdf.ellipsoid(tuple(c), (0.028, 0.009, 0.010), k=0.015, bone=B('lowerarm' if t >= 0 else 'upperarm'),
                               axes=np.array([tuple(inner.cross(fwd).normalized()), tuple(fwd), tuple(inner)])))
    return P


# ------------------------------------------------------------------ assembly

def transformed(prims, M):
    """Wraps hand-space primitives so they evaluate at seat-space points (M: hand space to seat)."""
    Minv = np.array(M.inverted())
    scale = abs(np.linalg.det(np.array(M.to_3x3()))) ** (1.0 / 3.0)
    out = []
    for p in prims:
        def fn(q, p=p):
            h = np.c_[q, np.ones(len(q))] @ Minv.T
            return p.fn(h[:, :3]) * scale
        corners = [Vector((x, y, z)) for x in (p.lo[0], p.hi[0]) for y in (p.lo[1], p.hi[1]) for z in (p.lo[2], p.hi[2])]
        pts = np.array([tuple(M @ c) for c in corners])
        out.append(sdf.Prim(fn, pts.min(axis=0), pts.max(axis=0), p.k, p.bone, p.sub))
    return out


def arm_prims(side):
    """(skin primitives, sleeve primitives) for one arm, in seat space."""
    hand = hand_prims('_' + side)
    return transformed(hand, hand_to_seat(side == 'l')), sleeve_prims(side)


# ------------------------------------------------------------------ rig

SKIN_TRIS = 11000   # per hand
SLEEVE_TRIS = 6000  # per sleeve


def decimate(obj, tris):
    import bpy
    mod = obj.modifiers.new('decimate', 'DECIMATE')
    mod.ratio = min(1.0, tris / max(len(obj.data.polygons), 1))
    core.select_only([obj])
    bpy.ops.object.modifier_apply(modifier=mod.name)


def smooth_weights(obj, W, iterations, keep=4):
    """Relaxes weights across the surface (like an artist's weight smoothing): joint transitions
    soften, and nothing leaks between fingers, which touch only at the webs."""
    me = obj.data
    edges = np.array([tuple(e.vertices) for e in me.edges])
    n = len(me.vertices)
    deg = np.bincount(edges.ravel(), minlength=n).astype(float)
    for _ in range(iterations):
        acc = np.zeros_like(W)
        np.add.at(acc, edges[:, 0], W[edges[:, 1]])
        np.add.at(acc, edges[:, 1], W[edges[:, 0]])
        W = 0.5 * W + 0.5 * acc / np.maximum(deg, 1.0)[:, None]
    if W.shape[1] > keep:
        cut = np.sort(W, axis=1)[:, -keep][:, None]
        W = np.where(W >= cut, W, 0.0)
    return W / np.maximum(W.sum(axis=1, keepdims=True), 1e-12)


def skin_to(obj, prims, sigma, smooth=0):
    """Vertex groups from the primitives' bones."""
    pts = np.array([tuple(v.co) for v in obj.data.vertices])
    bones, W = sdf.bone_weights(prims, pts, sigma=sigma, limit=0)
    if smooth:
        W = smooth_weights(obj, W, smooth)
    for j, b in enumerate(bones):
        g = obj.vertex_groups.new(name=b)
        for i in np.nonzero(W[:, j] > 0.002)[0]:
            g.add([int(i)], float(W[i, j]), 'REPLACE')


def armature():
    """The arms' armature: a root between the shoulders and both arms (Mannequin bone names)."""
    import bpy
    data = bpy.data.armatures.new('SK_Arms_rig')
    rig = core.link(bpy.data.objects.new('SK_Arms_rig', data))
    core.select_only([rig])
    bpy.ops.object.mode_set(mode='EDIT')
    eb = data.edit_bones
    root = eb.new('arms_root')
    root.head, root.tail = (0.0, 0.0, 0.0), (0.0, 0.0, 0.05)
    for side in ('r', 'l'):
        bones = skeleton(side)
        for name, (head, tail, up) in bones.items():
            b = eb.new(name)
            b.head, b.tail = head, tail
            b.align_roll(up)
        for name in bones:
            b = eb[name]
            kind = name[:-2]
            if kind == 'clavicle':
                b.parent = root
            elif kind == 'upperarm':
                b.parent = eb[f'clavicle_{side}']
            elif kind == 'lowerarm':
                b.parent = eb[f'upperarm_{side}']
            elif kind == 'hand':
                b.parent = eb[f'lowerarm_{side}']
            elif kind.endswith('_01'):
                b.parent = eb[f'hand_{side}']
            else:
                b.parent = eb[f'{kind[:-3]}_0{int(kind[-1]) - 1}_{side}']
    bpy.ops.object.mode_set(mode='OBJECT')
    return rig


def build_geometry():
    """Skin and sleeve meshes for both arms, skinned (vertex groups), in seat space. The left hand is
    the right one mirrored, so both share one skin texture; the sleeves are sculpted per side, so
    their folds differ."""
    skin_p, _ = arm_prims('r')
    skin_r = sdf.mesh(skin_p, 0.0006, 'skin_r')
    decimate(skin_r, SKIN_TRIS)
    skin_to(skin_r, skin_p, 0.0018, smooth=6)
    core.smart_uv(skin_r, margin=0.004)
    skin_l = mirror_copy(skin_r, 'skin_l')
    parts = [skin_r, skin_l]
    for side in ('r', 'l'):
        sleeve_p = sleeve_prims(side)
        sleeve = sdf.mesh(sleeve_p, 0.0012, f'sleeve_{side}')
        decimate(sleeve, SLEEVE_TRIS)
        skin_to(sleeve, sleeve_p, 0.02, smooth=10)
        parts.append(sleeve)
    torso_p = torso_prims()
    torso = sdf.mesh(torso_p, 0.002, 'torso')
    decimate(torso, 3000)
    skin_to(torso, torso_p, 0.03, smooth=6)
    parts.append(torso)
    return parts


def mirror_copy(obj, name):
    """A mirrored duplicate (x -> -x), its vertex groups renamed from the right side to the left."""
    import bpy
    dup = obj.copy()
    dup.data = obj.data.copy()
    dup.name = dup.data.name = name
    core.link(dup)
    dup.data.transform(Matrix.Scale(-1.0, 4, Vector((1.0, 0.0, 0.0))))
    dup.data.flip_normals()
    for g in dup.vertex_groups:
        if g.name.endswith('_r'):
            g.name = g.name[:-2] + '_l'
    return dup


def torso_prims():
    """The hoodie's chest and shoulders, joining the two sleeves: seen when looking down or far to the side."""
    # A slim chest: its front sits a little ahead of the shoulders and falls away below them.
    P = [sdf.ellipsoid((0.0, -0.06, -0.2), (0.155, 0.08, 0.22), k=0.06, bone='arms_root'),
         sdf.round_cone((-0.125, -0.03, -0.012), (0.125, -0.03, -0.012), 0.05, 0.05, k=0.05, bone='arms_root')]
    for s in (1.0, -1.0):
        P.append(sdf.ellipsoid((0.155 * s, -0.012, -0.012), (0.055, 0.058, 0.058), k=0.04, bone='clavicle_r' if s > 0 else 'clavicle_l'))
    return P


def pose(rig, curls, wrist=0.0, thumb=0.0):
    """Poses the rig for reviews: curls = {finger: (mcp, pip, dip) degrees}; wrist tilts the hands up."""
    for side in ('r', 'l'):
        for name, angles in curls.items():
            for k, a in enumerate(angles):
                pb = rig.pose.bones[f'{name}_0{k + 1}_{side}']
                pb.rotation_mode = 'XYZ'
                pb.rotation_euler = (math.radians(-a), 0.0, 0.0)
        hb = rig.pose.bones[f'hand_{side}']
        hb.rotation_mode = 'XYZ'
        hb.rotation_euler = (math.radians(wrist), 0.0, 0.0)
        for k in range(3):
            tb = rig.pose.bones[f'thumb_0{k + 1}_{side}']
            tb.rotation_mode = 'XYZ'
            tb.rotation_euler = (math.radians(-thumb * (0.5 if k == 0 else 1.0)), 0.0, 0.0)


# ------------------------------------------------------------------ skin maps (hand space)

SKIN_TEX = 4096
SKIN = 0xcf987e       # back of the hand
PALM = 0xdcaa98
FLUSH = 0xc7746a      # knuckles, fingertips, the palm's blush
NAIL_BED = 0xd9a79c
NAIL_EDGE = 0xeee6da

# Palm lines in hand XY (the palm side): heart, head, life; then the two wrist creases.
PALM_LINES = [
    ([(0.040, 0.066), (0.026, 0.0715), (0.010, 0.0775), (-0.004, 0.0825), (-0.0135, 0.0865)], 0.00030, 0.00020),
    ([(-0.0365, 0.0655), (-0.020, 0.0615), (0.000, 0.0575), (0.018, 0.0520), (0.0285, 0.0470)], 0.00028, 0.00018),
    ([(-0.0355, 0.0660), (-0.0245, 0.0520), (-0.0165, 0.0360), (-0.0120, 0.0200), (-0.0110, 0.0060)], 0.00030, 0.00020),
    ([(-0.024, 0.0015), (-0.008, 0.0005), (0.008, 0.0010), (0.024, 0.0020)], 0.00022, 0.00014),
    ([(-0.022, -0.0065), (0.000, -0.0075), (0.022, -0.0065)], 0.00018, 0.00010),
]
# Veins over the back of the hand, in hand XY.
VEINS = [
    [(-0.020, -0.035), (-0.018, 0.000), (-0.021, 0.030), (-0.024, 0.050), (-0.021, 0.072)],
    [(0.012, -0.035), (0.010, 0.000), (0.006, 0.025), (0.003, 0.050), (0.000, 0.074)],
    [(-0.021, 0.030), (-0.008, 0.038), (0.006, 0.031), (0.020, 0.026), (0.029, 0.035)],
    [(0.020, 0.026), (0.022, 0.050), (0.019, 0.071)],
    [(-0.022, 0.018), (-0.032, 0.035), (-0.041, 0.050)],
]


def _lin(hexv):
    return np.array(core.hex_linear(hexv)[:3], dtype=np.float32)


def _frame_np(a, b, up):
    a, b, up = np.asarray(a, float), np.asarray(b, float), np.asarray(up, float)
    ax = (b - a) / np.linalg.norm(b - a)
    up = up - ax * (up @ ax)
    up /= np.linalg.norm(up)
    side = np.cross(ax, up)
    return a, ax, side, up


def _local(p, frame):
    o, ax, side, up = frame
    q = p - o
    return q @ ax, q @ side, q @ up


def skin_fields(p, n):
    """Color (linear RGB), roughness and height (m) for hand-space points p with normals n."""
    M = len(p)
    rng = np.random.default_rng(11)
    dorsal = smoothstep(0.05, 0.45, n[:, 2])
    palmar = smoothstep(0.05, 0.45, -n[:, 2])
    red = np.zeros(M, np.float32)
    height = np.zeros(M, np.float32)
    crease = np.zeros(M, np.float32)
    rough = np.full(M, 0.46, np.float32)
    nail = np.zeros(M, np.float32)
    nail_col = np.zeros((M, 3), np.float32)
    nail_rough = np.full(M, 0.2, np.float32)
    wob = value_noise(p, 0.0016, seed=5, octaves=2) - 0.5  # hand-drawn wobble for lines

    def groove(dist, width, depth, env):
        g = np.exp(-(dist / width) ** 2) * env
        return g * depth, g

    fingers = dict(FINGERS)
    fingers['thumb'] = None
    for name, spec in fingers.items():
        if name == 'thumb':
            j = [Vector(t) for t in THUMB]
            radii = THUMB_R
            ups = [Vector(THUMB_UP).normalized()] * 3
            lengths = [(j[k + 1] - j[k]).length for k in range(3)]
        else:
            mcp, lengths, radii, splay, flex = spec
            j, _, ups = finger_joints(mcp, lengths, splay, flex, ups=True)
        center = np.array(tuple((j[0] + j[3]) / 2))
        near = np.nonzero(np.sum((p - center) ** 2, axis=1) < (sum(lengths) * 0.75 + 0.02) ** 2)[0]
        if not len(near):
            continue
        q, nq = p[near], n[near]
        # Frames: distal phalanx (nail), and each joint (wrinkles, creases).
        dist = _frame_np(tuple(j[2]), tuple(j[3]), tuple(ups[2]))
        t, u, v = _local(q, dist)
        L3, rd, rt = lengths[2], radii[2], radii[3]
        facing = nq @ dist[3]
        w = 0.74 * rd
        t0 = 0.26 * L3 + 0.0015 * (u / w) ** 2
        in_w = smoothstep(0.0003, -0.0002, np.abs(u) - w)
        plate = in_w * smoothstep(-0.0002, 0.0002, t - t0) * smoothstep(0.25, 0.45, facing) * (v > 0)
        lunula = plate * (t < t0 + 0.0026 * np.sqrt(np.clip(1 - (u / (0.72 * w)) ** 2, 0, 1))) * (1.0 if name in ('thumb', 'index') else 0.55)
        free = plate * smoothstep(L3 + 0.35 * rt, L3 + 0.55 * rt, t)
        col = np.tile(_lin(NAIL_BED), (len(q), 1))
        col = col * (1 - lunula[:, None] * 0.5) + _lin(0xefe0da) * lunula[:, None] * 0.5
        col = col * (1 - free[:, None]) + _lin(NAIL_EDGE) * free[:, None]
        ridges = value_noise(np.c_[t * 0.08, u, v], 0.00035, seed=9)
        nail[near] = np.maximum(nail[near], plate)
        nail_col[near] = np.where(plate[:, None] > 0, col, nail_col[near])
        nail_rough[near] = np.where(plate > 0, 0.16 + ridges * 0.1 + free * 0.12, nail_rough[near])
        # The plate stands proud of the skin; the cuticle folds over its base.
        height[near] += plate * 0.00018 + ridges * plate * 0.00001
        cuticle = smoothstep(0.0012, 0.0, np.abs(t - t0 + 0.0004)) * in_w * smoothstep(0.2, 0.4, facing) * (v > 0)
        height[near] += cuticle * 0.00008
        red[near] = np.maximum(red[near], cuticle * 0.8)
        # Around the nail and the fingertip, the skin flushes.
        tipness = smoothstep(0.2 * L3, L3 + 0.3 * rt, t) * (1 - plate)
        red[near] = np.maximum(red[near], tipness * 0.55)
        # Fingerprints: concentric ridges on the pad.
        tc = 0.5 * L3
        pad = smoothstep(0.1, 0.4, -facing) * smoothstep(-0.004, 0.002, t) * smoothstep(L3 + 0.8 * rt, L3, t)
        e = np.sqrt(((t - tc) / 1.0) ** 2 + (u / 0.8) ** 2)
        prints = 0.5 + 0.5 * np.sin(2 * np.pi * e / 0.00047 + wob[near] * 4.0)
        height[near] -= pad * prints * 0.000014
        # Knuckle wrinkles on the back of the joints (PIP, DIP; the thumb's IP), and creases underneath.
        joints = [(1, 6 if name != 'thumb' else 0, 2), (2, 4 if name != 'thumb' else 6, 1)]
        for k, n_lines, n_palmar in joints:
            ax = np.asarray(tuple((j[k + 1] - j[k - 1]).normalized()))
            fr = _frame_np(tuple(j[k]), tuple(j[k] + Vector(tuple(ax))), tuple(ups[k]))
            tt, uu, vv = _local(q, fr)
            r = radii[k]
            back = smoothstep(0.1, 0.4, nq @ fr[3])
            under = smoothstep(0.1, 0.4, -(nq @ fr[3]))
            env = np.exp(-(uu / (0.62 * r)) ** 2) * back
            for i in range(n_lines):
                tk = (-0.0034 + 0.0058 * i / max(n_lines - 1, 1)) + rng.uniform(-0.0003, 0.0003)
                bow = 0.0011 * np.sign(tk) * rng.uniform(0.6, 1.2)
                line = tt - (tk + bow * (uu / r) ** 2) + wob[near] * 0.0005
                strength = rng.uniform(0.5, 1.0) * np.exp(-(tk / 0.004) ** 2)
                dh, g = groove(line, 0.00011, 0.00007 * strength, env)
                height[near] -= dh
                crease[near] += g * 0.35 * strength
            for i in range(n_palmar):
                tk = (-0.0008 + 0.0014 * i) if n_palmar > 1 else 0.0003
                line = tt - tk + wob[near] * 0.0003
                env2 = smoothstep(-0.9 * r, -0.6 * r, -np.abs(uu)) * under
                dh, g = groove(line, 0.00016, 0.00016, env2)
                height[near] -= dh
                crease[near] += g
            red[near] = np.maximum(red[near], np.exp(-(tt ** 2 + uu ** 2) / 0.005 ** 2) * back * 0.55)
        if name != 'thumb':
            # The creases where the finger meets the palm, and the flush over the big knuckle.
            prox = _frame_np(tuple(j[0]), tuple(j[1]), tuple(ups[0]))
            tt, uu, vv = _local(q, prox)
            under = smoothstep(0.1, 0.4, -(nq @ prox[3]))
            for tk in (0.0128, 0.0152):
                dh, g = groove(tt - tk + wob[near] * 0.0004, 0.00018, 0.00014, under * smoothstep(-radii[0], -0.6 * radii[0], -np.abs(uu)))
                height[near] -= dh
                crease[near] += g
            back = smoothstep(0.1, 0.4, nq @ prox[3])
            red[near] = np.maximum(red[near], np.exp(-(tt ** 2 + uu ** 2) / 0.0065 ** 2) * back * 0.5)
            for i in range(3):
                tk = -0.004 + 0.003 * i
                line = tt - (tk + 0.0012 * (uu / radii[0]) ** 2) + wob[near] * 0.0006
                dh, g = groove(line, 0.0001, 0.00004, np.exp(-(uu / (0.6 * radii[0])) ** 2) * back * np.exp(-(tt / 0.005) ** 2))
                height[near] -= dh
                crease[near] += g * 0.3

    # Palm lines.
    pal = palmar > 0.05
    idx = np.nonzero(pal & (p[:, 1] > -0.012) & (p[:, 1] < 0.095))[0]
    for pts, width, depth in PALM_LINES:
        d, along = polyline_distance(p[idx, :2] + wob[idx, None] * 0.0008, pts)
        taper = smoothstep(0.0, 0.12, along) * smoothstep(1.0, 0.8, along)
        dh, g = groove(d, width, depth, taper * palmar[idx])
        height[idx] -= dh
        crease[idx] += g
    # Veins over the back of the hand: raised, faintly blue.
    vein = np.zeros(M, np.float32)
    idx = np.nonzero((dorsal > 0.05) & (p[:, 1] > -0.05) & (p[:, 1] < 0.08))[0]
    vw = 0.5 + value_noise(p[idx], 0.006, seed=31)  # veins swell and sink along their length
    for k, pts in enumerate(VEINS):
        d, along = polyline_distance(p[idx, :2] + (wob[idx, None] * 0.0015), pts)
        width = 0.0019 * (0.8 + 0.4 * vw)
        prof = np.exp(-(d / width) ** 2) * smoothstep(0.0, 0.2, along) * smoothstep(1.0, 0.7, along) * dorsal[idx] * vw
        vein[idx] = np.maximum(vein[idx], prof * (0.75 if k >= 2 else 1.0))
    height += vein * 0.00007

    # Micro detail: pores, and the fine diamond pattern of skin on the back of the hand.
    pores = value_noise(p, 0.00028, seed=21)
    a1 = np.sin(2 * np.pi * (0.8 * p[:, 0] + 0.6 * p[:, 1]) / 0.0004 + wob * 5.0)
    a2 = np.sin(2 * np.pi * (-0.8 * p[:, 0] + 0.6 * p[:, 1]) / 0.0004 + wob * 5.0)
    diamond = np.maximum(a1, a2)
    height += (pores - 0.5) * 0.000008 - dorsal * smoothstep(0.6, 1.0, diamond) * 0.000006

    # Color.
    tone = value_noise(p, 0.008, seed=3, octaves=3)
    mottle = value_noise(p, 0.0025, seed=4, octaves=2)
    red = np.maximum(red, palmar * (0.25 + 0.3 * smoothstep(0.45, 0.7, mottle)))
    col = np.tile(_lin(SKIN), (M, 1))
    col = col * (1 - palmar[:, None]) + _lin(PALM) * palmar[:, None]
    col = col * (1 - red[:, None] * 0.42) + _lin(FLUSH) * (red[:, None] * 0.42)
    col *= (0.94 + 0.12 * tone)[:, None] * (0.97 + 0.06 * mottle)[:, None]
    col = col * (1 - vein[:, None] * 0.2) + col * np.array([0.86, 0.9, 1.04], np.float32) * vein[:, None] * 0.2
    col *= (1 - np.clip(crease, 0, 1) * 0.16)[:, None]
    col = col * (1 - nail[:, None]) + nail_col * nail[:, None]
    # A few small moles on the back of the hand.
    for c in ((0.012, 0.043), (-0.006, 0.018), (0.024, 0.061)):
        d = np.hypot(p[:, 0] - c[0], p[:, 1] - c[1])
        mole = smoothstep(0.0006, 0.0003, d) * dorsal
        col = col * (1 - mole[:, None] * 0.6) + _lin(0x6e4a3a) * mole[:, None] * 0.6

    # Roughness: drier over knuckles and wrinkles, a little oilier in the palm and at the fingertips.
    rough += np.clip(crease, 0, 1) * 0.05 + (tone - 0.5) * 0.06
    rough -= palmar * 0.04
    rough = rough * (1 - nail) + nail_rough * nail
    return col, np.clip(rough, 0.1, 0.9), height


def skin_textures(skin, size=SKIN_TEX):
    """Bakes positions, computes the skin maps in hand space, and returns (base color, roughness, normal) paths."""
    Hi = hand_to_seat(False).inverted()
    R = Hi.to_3x3()
    pos = np.array([tuple(Hi @ v.co) for v in skin.data.vertices])
    nor = np.array([tuple((R @ v.normal).normalized()) for v in skin.data.vertices])
    P, N, valid = texmaps.position_maps(skin, size, pos, nor)
    col, rough, height = skin_fields(P[valid].astype(np.float64), N[valid].astype(np.float64))
    C = np.zeros((size, size, 3), np.float32)
    Rg = np.zeros((size, size, 3), np.float32)
    Hh = np.zeros((size, size), np.float32)
    C[valid] = col
    Rg[valid] = rough[:, None]
    Hh[valid] = height
    normal = texmaps.height_to_normal(Hh, P, valid) * 0.5 + 0.5
    return (texmaps.save('arms_skin_color', texmaps.dilate(C, valid)),
            texmaps.save('arms_skin_rough', texmaps.dilate(Rg, valid), 'Non-Color'),
            texmaps.save('arms_skin_normal', texmaps.dilate(normal.astype(np.float32), valid), 'Non-Color'))


# ------------------------------------------------------------------ hoodie fabric (shader, baked)

def sleeve_material():
    """Charcoal cotton jersey: a fine knit, vertical ribs on the cuff, pilling and a sheen where it rubs."""
    from artkit.shade import noise, object_coords, ramp
    m = core.Mat('arms_sleeve')
    tc, sep = object_coords(m)
    # Knit: fine loops along the sleeve, from the attributes baked into object space.
    knit_u = m.math('SINE', m.math('MULTIPLY', m.math('ADD', sep.outputs['X'], m.math('MULTIPLY', sep.outputs['Z'], 0.7)), 2 * math.pi / 0.0011))
    knit_v = m.math('SINE', m.math('MULTIPLY', sep.outputs['Y'], 2 * math.pi / 0.0009))
    knit = m.math('MULTIPLY', m.math('ADD', m.math('MULTIPLY', knit_u, knit_v), 1.0), 0.5)
    heather = noise(m, tc.outputs['Object'], scale=900.0, detail=2.0)
    pills = ramp(m, noise(m, tc.outputs['Object'], scale=2400.0, detail=1.0), 0.72, 0.8)
    color = m.mix(m.math('MULTIPLY', heather, 0.5), core.hex_linear(0x26272a), core.hex_linear(0x35363a))
    color = m.mix(m.math('MULTIPLY', pills, 0.4), color, core.hex_linear(0x4a4b50))
    # The cuffs: 1x1 ribbing running along the arm, about 2.4 mm apart (sides handled by the sign of x).
    ribs = None
    for s in (1.0, -1.0):
        el = Vector((ELBOW.x * s, ELBOW.y, ELBOW.z))
        wr = Vector((WRIST.x * s, WRIST.y, WRIST.z))
        fwd = (wr - el).normalized()
        edge = wr - fwd * CUFF_END
        x = Vector((0.0, 0.0, 1.0)).cross(fwd).normalized()
        y = fwd.cross(x)
        rel = m.node('ShaderNodeVectorMath')
        rel.operation = 'SUBTRACT'
        m.link(tc.outputs['Object'], rel.inputs[0])
        rel.inputs[1].default_value = tuple(edge)

        def dot(axis, rel=rel):
            d = m.node('ShaderNodeVectorMath')
            d.operation = 'DOT_PRODUCT'
            m.link(rel.outputs['Vector'], d.inputs[0])
            d.inputs[1].default_value = tuple(axis)
            return d.outputs['Value']
        t = m.math('MULTIPLY', dot(fwd), -1.0)
        ang = m.math('ARCTAN2', dot(y), dot(x))
        count = round(2 * math.pi * 0.034 / 0.0024)
        rib = m.math('MULTIPLY', m.math('ADD', m.math('SINE', m.math('MULTIPLY', ang, float(count))), 1.0), 0.5)
        inside = m.math('MULTIPLY', ramp(m, t, 0.0015, 0.003), m.math('SUBTRACT', 1.0, ramp(m, t, CUFF_LEN - 0.003, CUFF_LEN - 0.0015)))
        this_side = m.math('GREATER_THAN', m.math('MULTIPLY', sep.outputs['X'], s), 0.0)
        r = m.math('MULTIPLY', m.math('MULTIPLY', rib, inside), this_side)
        ribs = r if ribs is None else m.math('MAXIMUM', ribs, r)
    color = m.mix(m.math('MULTIPLY', ribs, 0.12), color, core.hex_linear(0x1f2023))
    m.set('Base Color', color)
    m.set('Roughness', m.math('ADD', 0.84, m.math('MULTIPLY', heather, 0.06)))
    m.set('Sheen Weight', 0.6)
    m.set('Sheen Roughness', 0.35)
    bump = m.node('ShaderNodeBump', Strength=0.45, Distance=0.00025)
    height = m.math('ADD', m.math('ADD', knit, m.math('MULTIPLY', pills, 0.6)), m.math('MULTIPLY', ribs, 2.5))
    m.link(height, bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m



# ------------------------------------------------------------------ the asset

MESHES = ['SK_Arms']
DOUBLE_SIDED = False
TEXTURE_SIZE = 2048
TEXTURE_SIZES = {'arms_skin': 4096, 'arms_sleeve': 4096}
AO_DISTANCE = 0.02
TYPING = dict(index=(26, 40, 18), middle=(24, 44, 20), ring=(28, 46, 20), pinky=(32, 46, 18))


def skin_material(maps):
    col, rough, nrm = maps
    m = core.Mat('arms_skin')
    c = m.image(col)
    m.link(c.outputs['Color'], m.bsdf.inputs['Base Color'])
    r = m.image(rough, non_color=True)
    m.link(r.outputs['Color'], m.bsdf.inputs['Roughness'])
    n = m.image(nrm, non_color=True)
    nmap = m.node('ShaderNodeNormalMap')
    m.link(n.outputs['Color'], nmap.inputs['Color'])
    m.link(nmap.outputs['Normal'], m.bsdf.inputs['Normal'])
    m.set('Subsurface Weight', 1.0)
    m.set('Subsurface Radius', (1.0, 0.35, 0.15))
    m.set('Subsurface Scale', 0.0035)
    return m


def build():
    import bpy
    core.reset()
    parts = build_geometry()
    maps = skin_textures(parts[0])
    skin, sleeve = skin_material(maps), sleeve_material()
    for p in parts:
        core.assign(p, skin if p.name.startswith('skin') else sleeve)
    obj = core.join(parts, 'SK_Arms')  # the skins come first: material slot 0 is the skin
    obj.data.shade_smooth()
    is_skin = core.material_is(obj, 'arms_skin')
    core.uv_layout(obj, [
        (is_skin, 'keep', None, (0.0, 0.0, 1.0, 1.0)),
        (lambda f: True, 'box', None, (0.0, 0.0, 1.0, 1.0)),
    ])
    rig = armature()
    obj.parent = rig
    mod = obj.modifiers.new('rig', 'ARMATURE')
    mod.object = rig
    return [obj]


def pose_for_review(objs):
    """Typing pose, and a slab of desk under the hands."""
    import bpy
    rig = objs[0].parent
    pose(rig, TYPING, wrist=10, thumb=10)
    bpy.ops.mesh.primitive_cube_add(size=1.0)
    desk = bpy.context.view_layer.objects.active
    desk.name = 'review_desk'
    desk.scale = (0.9, 0.5, 0.03)
    desk.location = (0.0, 0.5, -0.285)
    m = core.Mat('review_desk')
    m.set('Base Color', core.hex_linear(0x3b2616))
    m.set('Roughness', 0.45)
    core.assign(desk, m)
    bpy.context.view_layer.update()


REVIEW_VIEWS = [('player', 0, 38, 1.3, (0.0, 0.42, -0.23)), ('three', -35, 25, 1.5), ('detail', -25, 38, 0.32, (0.13, 0.47, -0.235))]
