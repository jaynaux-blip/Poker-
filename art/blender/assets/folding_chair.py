"""SM_FoldingChair: a steel folding chair from the laundromat's back room, one of a mismatched set.

- A putty-colored powder-coated tube frame, chipped to bare steel at the leg fronts and the seat
  corners, with a little rust in the worst chips.
- A padded oxblood vinyl seat, its edge cracked from years of being sat on; one split mended with
  silver duct tape.
- A curved pressed-steel backrest, scuffed where it has knocked the wall.
- Black rubber foot caps, worn flat on the floor side.

Coordinates (meters): origin on the floor under the middle of the seat, Z up, the front toward +X
(the player sits facing +X, as ABackRoomPlayer seats them).
"""
import math

import bmesh

from artkit import core
from artkit.shade import convex_edges, mix_float, noise, object_coords, powder_coat, ramp

MESHES = ['SM_FoldingChair']
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {'fchair_vinyl': 2048, 'fchair_steel': 2048}
AO_DISTANCE = 0.05

SEAT_TOP = 0.465
SEAT_HALF_W = 0.205   # across (Y)
SEAT_HALF_D = 0.195   # front to back (X)
TUBE_R = 0.0115
STEEL = 0xb3a78f      # putty
VINYL = 0x4a1c19      # oxblood

REVIEW_VIEWS = [('front', 30, 16, 2.4), ('back', 210, 24, 2.4), ('detail', 20, 38, 0.8, (0.14, -0.1, 0.44))]


def tube(name, ctrl, steps=10, r=TUBE_R):
    """A round steel tube through control points (smooth bends)."""
    path = core.catmull_rom(ctrl, steps)
    ring = [(r * math.cos(2 * math.pi * k / 18), r * math.sin(2 * math.pi * k / 18)) for k in range(18)]
    return core.sweep(name, path, ring)


def steel_material():
    m = core.Mat('fchair_steel')
    tc, sep = object_coords(m)
    color, rough, metal, bump = powder_coat(m, tc, STEEL, rough=0.48, chip_hex=0x7d7f82, chips=1.6)
    # The worst chips have rusted: brown in the middle of the bare patches, low on the legs mostly.
    edge = convex_edges(m, tc, distance=0.0016, breakup_scale=60.0)
    low = ramp(m, m.math('MULTIPLY', sep.outputs['Z'], -1.0), -0.25, -0.02)
    rust = m.math('MULTIPLY', ramp(m, edge, 0.7, 0.9), m.math('ADD', 0.35, m.math('MULTIPLY', low, 0.65)))
    rust = m.math('MULTIPLY', rust, ramp(m, noise(m, tc.outputs['Object'], scale=180.0, detail=4.0), 0.45, 0.6))
    color = m.mix(rust, color, m.mix(noise(m, tc.outputs['Object'], scale=900.0, detail=2.0), core.hex_linear(0x5c3317), core.hex_linear(0x8a4f22)))
    rough = mix_float(m, rust, rough, 0.85)
    metal = mix_float(m, rust, metal, 0.0)
    # Grime: darker where hands and shoes have been (the seat's front corners, the low crossbars).
    grime = m.math('MULTIPLY', ramp(m, noise(m, tc.outputs['Object'], scale=14.0, detail=3.0), 0.5, 0.75), 0.35)
    color = m.mix(grime, color, core.hex_linear(0x5e5546))
    m.set('Base Color', color)
    m.set('Roughness', rough)
    m.set('Metallic', metal)
    m.set('Normal', bump.outputs['Normal'])
    return m


def vinyl_material():
    m = core.Mat('fchair_vinyl')
    tc, sep = object_coords(m)
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    # Vinyl grain, and creases radiating from where people sit.
    grain = noise(m, tc.outputs['Object'], scale=2600.0, detail=2.0)
    crease = noise(m, m.math('MULTIPLY', tc.outputs['Object'], 1.0), scale=55.0, detail=6.0, distortion=0.6)
    # Worn shiny and lighter at the front edge where legs rub; cracked along the top edge's roll.
    rim = ramp(m, m.math('SUBTRACT', SEAT_TOP, z), 0.004, 0.0)
    front = ramp(m, x, 0.12, 0.19)
    cracks = m.math('MULTIPLY', ramp(m, noise(m, tc.outputs['Object'], scale=320.0, detail=8.0, distortion=1.2), 0.62, 0.66), m.math('MAXIMUM', rim, front))
    color = m.mix(m.math('MULTIPLY', front, 0.35), core.hex_linear(VINYL), core.hex_linear(0x6b3329))
    color = m.mix(cracks, color, core.hex_linear(0x1d0d0b))
    rough = m.math('ADD', 0.42, m.math('MULTIPLY', grain, 0.12))
    rough = mix_float(m, m.math('MULTIPLY', front, 0.6), rough, 0.28)
    rough = mix_float(m, cracks, rough, 0.75)
    # Duct tape over a split near the front right corner: matte silver cloth, its ends lifting.
    tx = m.math('SUBTRACT', x, 0.11)
    ty = m.math('SUBTRACT', y, -0.09)
    ca, sa = math.cos(0.5), math.sin(0.5)
    u = m.math('ADD', m.math('MULTIPLY', tx, ca), m.math('MULTIPLY', ty, sa))
    v = m.math('SUBTRACT', m.math('MULTIPLY', ty, ca), m.math('MULTIPLY', tx, sa))
    # Torn ends, not cut: the ragged edge wanders.
    ragged = m.math('MULTIPLY', m.math('SUBTRACT', noise(m, tc.outputs['Object'], scale=420.0, detail=4.0), 0.5), 0.01)
    tape = m.math('MULTIPLY', ramp(m, m.math('SUBTRACT', 0.019, m.math('ABSOLUTE', v)), 0.0, 0.0008),
                  ramp(m, m.math('SUBTRACT', m.math('ADD', 0.055, ragged), m.math('ABSOLUTE', u)), 0.0, 0.0008))
    tape = m.math('MULTIPLY', tape, ramp(m, z, SEAT_TOP - 0.02, SEAT_TOP - 0.012))
    weave = noise(m, tc.outputs['Object'], scale=4000.0, detail=1.0)
    # Years of grime worked into the cloth, darkest along the edges where it's lifting.
    lift = ramp(m, m.math('ABSOLUTE', v), 0.012, 0.019)
    dirt = m.math('MAXIMUM', lift, ramp(m, noise(m, tc.outputs['Object'], scale=90.0, detail=4.0), 0.4, 0.75))
    tape_col = m.mix(weave, core.hex_linear(0x7f8082), core.hex_linear(0x96979a))
    tape_col = m.mix(m.math('MULTIPLY', dirt, 0.75), tape_col, core.hex_linear(0x4e4a43))
    color = m.mix(tape, color, tape_col)
    rough = mix_float(m, tape, rough, 0.58)
    m.set('Base Color', color)
    m.set('Roughness', rough)
    m.set('Metallic', mix_float(m, tape, 0.0, 0.35))
    height = m.math('ADD', m.math('MULTIPLY', grain, 0.15), m.math('MULTIPLY', crease, 0.6))
    height = m.math('SUBTRACT', height, m.math('MULTIPLY', cracks, 1.5))
    # The tape stands proud of the vinyl, wrinkled across its width.
    wrinkle = noise(m, m.math('MULTIPLY', tc.outputs['Object'], 1.0), scale=160.0, detail=3.0, distortion=1.5)
    height = m.math('ADD', height, m.math('MULTIPLY', tape, m.math('ADD', 0.6, m.math('MULTIPLY', wrinkle, 0.8))))
    bump = m.node('ShaderNodeBump', Strength=0.12, Distance=0.0008)
    m.link(height, bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def rubber_material():
    m = core.Mat('fchair_rubber')
    tc, sep = object_coords(m)
    scuff = ramp(m, noise(m, tc.outputs['Object'], scale=400.0, detail=3.0), 0.5, 0.7)
    m.set('Base Color', m.mix(scuff, core.hex_linear(0x101010), core.hex_linear(0x2a2a28)))
    m.set('Roughness', mix_float(m, scuff, 0.72, 0.9))
    m.set('Metallic', 0.0)
    return m


def build():
    core.reset()
    steel, vinyl, rubber = steel_material(), vinyl_material(), rubber_material()
    parts_steel = []

    # The back legs run up into the backrest, leaning back; the front legs come up under the seat's
    # sides and run back to the hinge at the back legs.
    for s in (-1, 1):
        back = tube(f'back_leg_{s}', [(-0.215, s * 0.188, 0.014), (-0.2, s * 0.188, 0.24), (-0.188, s * 0.188, 0.44),
                                      (-0.205, s * 0.186, 0.62), (-0.236, s * 0.183, 0.80), (-0.244, s * 0.178, 0.835)])
        front = tube(f'front_leg_{s}', [(0.205, s * 0.212, 0.014), (0.19, s * 0.212, 0.22), (0.17, s * 0.212, 0.385),
                                        (0.125, s * 0.212, 0.425), (-0.05, s * 0.21, 0.428), (-0.19, s * 0.2, 0.43)])
        parts_steel += [back, front]
    # Crossbars: low at the front and back, and the seat's rear rail.
    parts_steel.append(tube('front_bar', [(0.2, -0.212, 0.075), (0.2, 0.212, 0.075)], steps=2, r=0.0085))
    parts_steel.append(tube('back_bar', [(-0.206, -0.188, 0.11), (-0.206, 0.188, 0.11)], steps=2, r=0.0085))
    parts_steel.append(tube('rear_rail', [(-0.19, -0.2, 0.43), (-0.19, 0.2, 0.43)], steps=2, r=0.009))
    # Hinge bolts where the legs cross.
    for s in (-1, 1):
        bolt = core.lathe(f'bolt_{s}', [(0.0, 0.0), (0.009, 0.0), (0.009, 0.004), (0.006, 0.0065), (0.0, 0.007)], segments=24)
        bolt.rotation_euler = (s * -math.pi / 2, 0.0, 0.0)
        bolt.location = (-0.19, s * (0.2 + 0.012), 0.43)
        core.apply_transforms(bolt)
        parts_steel.append(bolt)

    # The backrest: a curved steel panel between the back legs, its middle bowed back toward the sitter's spine.
    path = []
    for i in range(25):
        yy = -0.192 + 0.384 * i / 24
        bow = 0.026 * (1.0 - (yy / 0.192) ** 2)
        path.append((-0.226 - bow, yy, 0.73))
    # Its section: 15 cm tall, 1.1 cm thick, the edges rolled.
    hh, tt = 0.075, 0.0055
    panel = core.sweep('backrest', path, core.rounded_rect(-hh, -tt, hh, tt, tt * 0.95, steps=4))
    # Lean the panel with the legs (about 10 degrees back), about its middle.
    panel.location = (0.226, 0.0, -0.73)
    core.apply_transforms(panel)
    panel.rotation_euler = (0.0, math.radians(-10), 0.0)
    panel.location = (-0.226, 0.0, 0.73)
    core.apply_transforms(panel)
    parts_steel.append(panel)

    # The seat: a steel pan with a padded vinyl cushion on top, edges rolled.
    outline = core.rounded_rect(-SEAT_HALF_D, -SEAT_HALF_W, SEAT_HALF_D, SEAT_HALF_W, 0.045, steps=8)
    pan_bm = core.slab(outline, 0.428, 0.44)
    pan = core.mesh_object('pan', pan_bm)
    parts_steel.append(pan)
    cushion_bm = core.slab([(x * 0.985, y * 0.985) for x, y in outline], 0.438, SEAT_TOP)
    core.bevel(cushion_bm, lambda e: all(v.co.z > SEAT_TOP - 0.001 for v in e.verts), 0.012, segments=5, profile=0.6)
    cushion = core.mesh_object('cushion', cushion_bm)
    # A slight dish where everyone sits.
    for v in cushion.data.vertices:
        if v.co.z > SEAT_TOP - 0.004:
            d = (v.co.x / SEAT_HALF_D) ** 2 + (v.co.y / SEAT_HALF_W) ** 2
            v.co.z -= 0.006 * max(0.0, 1.0 - d)

    # Rubber caps on the four feet.
    caps = []
    for (fx, fy) in ((-0.215, -0.188), (-0.215, 0.188), (0.205, -0.212), (0.205, 0.212)):
        cap = core.lathe('cap', [(0.0, 0.0), (0.0135, 0.0), (0.0145, 0.003), (0.0145, 0.026), (0.0125, 0.028), (0.0, 0.028)], segments=24)
        cap.location = (fx, fy, 0.0)
        core.apply_transforms(cap)
        caps.append(cap)

    for p in parts_steel:
        core.assign(p, steel)
        core.finish_hard_surface(p, 50)
    core.assign(cushion, vinyl)
    core.finish_hard_surface(cushion, 70)
    for c in caps:
        core.assign(c, rubber)
        core.finish_hard_surface(c, 50)
    steel_obj = core.join(parts_steel, 'steel')
    caps_obj = core.join(caps, 'caps')
    core.smart_uv(steel_obj)
    core.smart_uv(cushion)
    core.smart_uv(caps_obj)
    return [core.join([steel_obj, cushion, caps_obj], 'SM_FoldingChair')]
