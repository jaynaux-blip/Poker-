"""The Riverside's crowd at the far tables: people sculpted as signed distance fields, light enough to fill a room.

The card room's near tables seat MetaHumans; everyone farther away is one of these, instanced (the room's
fidelity tiers, docs/LIVE_TOURNAMENTS.md section 4). Seen across a dim room under the pendants they need the right
silhouettes, postures and colors, not faces:
- SM_Crowd_Seated_A..F: players in six postures (forearms on the rail, leaning back with hands in the lap, chin
  on a fist, arms crossed on the rail, hunched over their cards, one arm hooked over the chair), each with its own
  skin, hair and clothes.
- SM_Crowd_Dealer: a dealer in a black vest over a white shirt, hands on the felt.
- SM_Crowd_Standing_A/B: railbirds, hands in pockets or holding a drink.

Coordinates (meters): a seated figure's frame is the seat's (ABackRoomStage::SeatTransform): +X toward the table,
origin on the floor 10 cm outside the rail, the chair seat 20 cm behind it at 46 cm; the rail is at x = 0.1, the
felt 76 cm up. A standing figure stands on its origin facing +X.
"""
import math

import numpy as np

import bpy  # noqa: F401

from artkit import core, parts, sdf

MESHES = ['SM_Crowd_Seated_A', 'SM_Crowd_Seated_B', 'SM_Crowd_Seated_C', 'SM_Crowd_Seated_D', 'SM_Crowd_Seated_E', 'SM_Crowd_Seated_F',
          'SM_Crowd_Dealer', 'SM_Crowd_Standing_A', 'SM_Crowd_Standing_B']
DOUBLE_SIDED = False
# Seen from across the room they need colors, not textures: plain materials, exported unbaked.
NO_BAKE = True
TEXTURE_SIZE = 512
AO_DISTANCE = 0.12
REVIEW_VIEWS = [('front', -150, 10, 2.2, (0.0, 0.0, 0.8)), ('side', -90, 8, 2.2, (0.0, 0.0, 0.8))]
REVIEW_SCREEN_LIGHT = False

VOXEL = 0.012
TRIS = 5000


def plain(name, hex_color, rough):
    m = core.Mat(name)
    m.set('Base Color', core.hex_linear(hex_color))
    m.set('Roughness', rough)
    return m


# Skin tones, hair colors and clothes, mixed across the figures.
LOOKS = {
    'A': dict(skin=0xc58c6a, hair=0x1d140e, top=0x2f3b52, pants=0x2a3550, shoes=0x1a1612, cut='short', sleeves=True),
    'B': dict(skin=0x6e4630, hair=0x0f0c0a, top=0x7a2a2a, pants=0x1f1f22, shoes=0x111111, cut='buzz', sleeves=False),
    'C': dict(skin=0xe0b394, hair=0x9a8a74, top=0x3a5a40, pants=0x5a4a3a, shoes=0x3a2a1a, cut='bun', sleeves=True),
    'D': dict(skin=0x8e5c40, hair=0x231812, top=0x8a7a5a, pants=0x23304a, shoes=0x2a2522, cut='cap', sleeves=True),
    'E': dict(skin=0xd9a07a, hair=0x5b3a1e, top=0x24464f, pants=0x2e2e33, shoes=0x151515, cut='long', sleeves=True),
    'F': dict(skin=0xb07a58, hair=0xc8c4bc, top=0x5a4a6e, pants=0x3a3a40, shoes=0x2a1a12, cut='short', sleeves=False),
}


def seated(pose, look):
    """Prims by region for a seated figure in one of six postures."""
    R = {k: [] for k in ('skin', 'hair', 'top', 'pants', 'shoes', 'eyes')}
    lean = {'rail': 0.0, 'back': -0.1, 'chin': 0.02, 'crossed': 0.03, 'hunch': 0.08, 'hook': -0.06}[pose]
    hip = np.array([-0.22, 0.0, 0.53])
    chest = np.array([-0.13 + lean, 0.0, 0.95 - abs(lean) * 0.25])
    head = chest + np.array([0.05 + lean * 0.4, 0.0, 0.33])
    # Hips and legs under the table, feet on the floor.
    R['pants'].append(sdf.ellipsoid(hip, (0.15, 0.19, 0.11), k=0.03))
    for s in (-1, 1):
        knee = np.array([0.2, s * 0.12, 0.55])
        ankle = np.array([0.17, s * 0.13, 0.09])
        R['pants'].append(sdf.round_cone(hip + (0.04, s * 0.1, 0.0), knee, 0.085, 0.06, k=0.02))
        R['pants'].append(sdf.round_cone(knee, ankle, 0.058, 0.045, k=0.02))
        R['shoes'].append(sdf.round_cone(ankle + (-0.02, 0, -0.04), ankle + (0.12, s * 0.01, -0.06), 0.045, 0.04, k=0.015))
    # The trunk: belly to chest, broad shoulders.
    R['top'].append(sdf.round_cone(hip + (0.0, 0.0, 0.04), chest, 0.15, 0.16, k=0.04, flat=0.75, up=(1.0, 0.0, 0.0)))
    R['top'].append(sdf.ellipsoid(chest + (0.0, 0.0, 0.04), (0.12, 0.2, 0.13), k=0.04))
    shoulder = [chest + (0.0, s * 0.2, 0.08) for s in (-1, 1)]
    R['skin'].append(sdf.round_cone(chest + (0.02, 0, 0.14), head + (-0.01, 0, -0.1), 0.055, 0.05, k=0.02))
    R['skin'].append(sdf.ellipsoid(head, (0.1, 0.085, 0.12), k=0.02))
    R['skin'].append(sdf.ellipsoid(head + (0.095, 0.0, -0.01), (0.02, 0.015, 0.03), k=0.01))  # the nose
    for s in (-1, 1):
        R['skin'].append(sdf.ellipsoid(head + (-0.005, s * 0.085, 0.0), (0.02, 0.012, 0.03), k=0.008))  # ears
        # Across a room a face is its eyes and brows: shadowed sockets, a brow ridge in the hair's color.
        R['eyes'].append(sdf.ellipsoid(head + (0.088, s * 0.033, 0.018), (0.012, 0.016, 0.009), k=0.004))
        R['hair'].append(sdf.ellipsoid(head + (0.09, s * 0.034, 0.042), (0.008, 0.022, 0.006), k=0.004))
    cut = look['cut']
    if cut != 'bald':
        top = {'short': (0.105, 0.092, 0.085), 'buzz': (0.102, 0.088, 0.075), 'bun': (0.105, 0.09, 0.09), 'cap': (0.11, 0.095, 0.07), 'long': (0.11, 0.095, 0.09)}[cut]
        R['hair'].append(sdf.ellipsoid(head + (-0.015, 0.0, 0.035), top, k=0.01))
        # The back of the head and the nape covered too, not a cap on an egg.
        R['hair'].append(sdf.ellipsoid(head + (-0.045, 0.0, -0.01), (top[0] * 0.85, top[1] * 0.98, top[2] * 0.95), k=0.012))
        if cut == 'bun':
            R['hair'].append(sdf.ellipsoid(head + (-0.11, 0.0, 0.06), (0.045, 0.045, 0.045), k=0.015))
        if cut == 'long':
            R['hair'].append(sdf.round_cone(head + (-0.06, 0.0, 0.0), head + (-0.1, 0.0, -0.28), 0.1, 0.07, k=0.03, flat=0.6, up=(1.0, 0.0, 0.0)))
        if cut == 'cap':
            R['hair'].append(sdf.ellipsoid(head + (0.08, 0.0, 0.05), (0.08, 0.08, 0.012), k=0.005))  # the bill
    # Arms, by posture.
    def arm(s, elbow, wrist, hand, hand_r=(0.055, 0.04, 0.028)):
        sleeve = 'top' if look['sleeves'] else 'skin'
        R['top'].append(sdf.round_cone(shoulder[(s + 1) // 2], elbow, 0.058, 0.046, k=0.02))
        R['top' if look['sleeves'] else 'skin'].append(sdf.round_cone(elbow, wrist, 0.045, 0.034, k=0.015))
        R['skin'].append(sdf.ellipsoid(hand, hand_r, k=0.015))
        _ = sleeve
    rail_z = 0.81
    for s in (-1, 1):
        if pose == 'rail' or (pose == 'chin' and s > 0) or (pose == 'hook' and s < 0):
            arm(s, np.array([0.01, s * 0.25, rail_z]), np.array([0.24, s * 0.12, rail_z]), np.array([0.3, s * 0.09, rail_z]))
        elif pose == 'back':
            arm(s, np.array([-0.18, s * 0.24, 0.72]), np.array([-0.02, s * 0.12, 0.62]), np.array([0.03, s * 0.08, 0.62]))
        elif pose == 'chin':
            arm(s, np.array([0.12, s * 0.17, rail_z]), np.array([0.02, s * 0.05, 1.1]), head + (0.07, s * 0.02, -0.12), (0.045, 0.045, 0.05))
        elif pose == 'crossed':
            arm(s, np.array([0.04, s * 0.26, rail_z]), np.array([0.2, -s * 0.06, rail_z + 0.02 * s]), np.array([0.2, -s * 0.15, rail_z + 0.02 * s]))
        elif pose == 'hunch':
            arm(s, np.array([0.06, s * 0.2, rail_z]), np.array([0.3, s * 0.07, rail_z - 0.01]), np.array([0.36, s * 0.05, rail_z - 0.01]))
        elif pose == 'hook':
            arm(s, np.array([-0.3, s * 0.32, 0.86]), np.array([-0.36, s * 0.3, 0.62]), np.array([-0.36, s * 0.3, 0.55]))
    return R


def standing(drink):
    R = {k: [] for k in ('skin', 'hair', 'top', 'pants', 'shoes', 'eyes')}
    hip = np.array([0.0, 0.0, 0.95])
    chest = np.array([0.02, 0.0, 1.35])
    head = chest + (0.03, 0.0, 0.33)
    R['pants'].append(sdf.ellipsoid(hip, (0.13, 0.18, 0.12), k=0.03))
    for s in (-1, 1):
        R['pants'].append(sdf.round_cone(hip + (0.0, s * 0.09, -0.05), (0.02, s * 0.11, 0.5), 0.085, 0.06, k=0.02))
        R['pants'].append(sdf.round_cone((0.02, s * 0.11, 0.5), (0.0, s * 0.12, 0.09), 0.058, 0.045, k=0.02))
        R['shoes'].append(sdf.round_cone((-0.02, s * 0.12, 0.05), (0.12, s * 0.13, 0.03), 0.045, 0.04, k=0.015))
    R['top'].append(sdf.round_cone(hip + (0, 0, 0.05), chest, 0.15, 0.17, k=0.04, flat=0.75, up=(1.0, 0.0, 0.0)))
    R['top'].append(sdf.ellipsoid(chest + (0, 0, 0.04), (0.12, 0.21, 0.13), k=0.04))
    R['skin'].append(sdf.round_cone(chest + (0, 0, 0.14), head + (-0.01, 0, -0.1), 0.055, 0.05, k=0.02))
    R['skin'].append(sdf.ellipsoid(head, (0.1, 0.085, 0.12), k=0.02))
    R['skin'].append(sdf.ellipsoid(head + (0.095, 0, -0.01), (0.02, 0.015, 0.03), k=0.01))
    R['hair'].append(sdf.ellipsoid(head + (-0.015, 0, 0.035), (0.105, 0.092, 0.085), k=0.01))
    R['hair'].append(sdf.ellipsoid(head + (-0.045, 0, -0.01), (0.09, 0.09, 0.08), k=0.012))
    for s in (-1, 1):
        R['eyes'].append(sdf.ellipsoid(head + (0.088, s * 0.033, 0.018), (0.012, 0.016, 0.009), k=0.004))
        R['hair'].append(sdf.ellipsoid(head + (0.09, s * 0.034, 0.042), (0.008, 0.022, 0.006), k=0.004))
    for s in (-1, 1):
        sh = chest + (0.0, s * 0.21, 0.08)
        if drink and s > 0:
            elbow = np.array([0.02, s * 0.25, 1.08])
            hand = np.array([0.22, s * 0.16, 1.2])
            R['top'].append(sdf.round_cone(sh, elbow, 0.058, 0.046, k=0.02))
            R['top'].append(sdf.round_cone(elbow, hand - (0.04, 0, 0.0), 0.045, 0.034, k=0.015))
            R['skin'].append(sdf.ellipsoid(hand, (0.045, 0.04, 0.05), k=0.015))
            R['shoes'].append(sdf.round_cone(hand + (0.02, 0, -0.04), hand + (0.02, 0, 0.08), 0.035, 0.04, k=0.004))  # the glass
        else:
            elbow = np.array([-0.04, s * 0.25, 1.08])
            hand = np.array([0.02, s * 0.2, 0.86])
            R['top'].append(sdf.round_cone(sh, elbow, 0.058, 0.046, k=0.02))
            R['top'].append(sdf.round_cone(elbow, hand, 0.045, 0.038, k=0.015))
    return R


def figure(name, regions, look, dealer=False):
    prims = [p for ps in regions.values() for p in ps]
    obj = sdf.mesh(prims, VOXEL, name)
    mod = obj.modifiers.new('decimate', 'DECIMATE')
    mod.ratio = min(1.0, TRIS / max(len(obj.data.polygons), 1))
    core.select_only([obj])
    bpy.ops.object.modifier_apply(modifier=mod.name)
    # Each face takes the material of the region it's nearest.
    me = obj.data
    centers = np.array([p.center[:] for p in me.polygons])
    names = [k for k in regions if regions[k]]
    dist = np.stack([sdf.evaluate(regions[k], centers) for k in names], axis=1)
    pick = np.argmin(dist, axis=1)
    tag = name.lower().replace('sm_', '')
    top_hex = 0x101010 if dealer else look['top']
    mats = {
        'skin': plain(f'{tag}_skin', look['skin'], 0.68),
        'eyes': plain(f'{tag}_eyes', 0x140d0a, 0.4),
        'hair': plain(f'{tag}_hair', look['hair'], 0.5),
        'top': plain(f'{tag}_top', top_hex, 0.85),
        'pants': plain(f'{tag}_pants', look['pants'], 0.9),
        'shoes': plain(f'{tag}_shoes', look['shoes'], 0.45),
    }
    for k in names:
        me.materials.append(mats[k].m)
    for p, i in zip(me.polygons, pick):
        p.material_index = int(i)
    if dealer:
        # The white shirt's sleeves and collar: faces of the arms and the neck's base go white.
        shirt = plain(f'{tag}_shirt', 0xece8e0, 0.8)
        me.materials.append(shirt.m)
        si = len(me.materials) - 1
        ti = names.index('top')
        for p in me.polygons:
            c = p.center
            if p.material_index == ti and (abs(c.y) > 0.19 or (c.z > 1.02 and c.x > -0.1)):
                p.material_index = si
    me.shade_smooth()
    obj.name = obj.data.name = name
    return obj


def build():
    core.reset()
    objs = []
    for key, pose in zip('ABCDEF', ('rail', 'back', 'chin', 'crossed', 'hunch', 'hook')):
        objs.append(figure(f'SM_Crowd_Seated_{key}', seated(pose, LOOKS[key]), LOOKS[key]))
    dealer_look = dict(LOOKS['A'], skin=0xa8704e, hair=0x15100c, pants=0x111114, cut='bun', sleeves=True)
    objs.append(figure('SM_Crowd_Dealer', seated('rail', dealer_look), dealer_look, dealer=True))
    objs.append(figure('SM_Crowd_Standing_A', standing(False), LOOKS['D']))
    objs.append(figure('SM_Crowd_Standing_B', standing(True), LOOKS['E']))
    return objs
