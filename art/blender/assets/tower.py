"""SM_Tower and SM_Tower_Glow: GearDrop's FORGE desktop PC, a mid-tower with its side panel off.

- SM_Tower: a black steel case (perforated front, a dust-filtered top, rubber feet) and, through the open
  side, a motherboard with an AIO cooler (pump block, tubes, a top radiator with two fans), two RAM sticks,
  a graphics card, a PSU shroud with a sleeved cable, three front intake fans and a rear exhaust.
- SM_Tower_Glow: every part that lights up (fan rings and hubs, the pump's ring, the RAM bars, the
  card's strip, the front strips), exported unbaked: the game colors it with the room's LEDs.
The two-PC setup stands two of them side by side.

Coordinates (meters), front toward -Y, Z up: the origin is on the floor under the middle of the case; the
open side faces -X (toward the desk, which the tower stands to the right of).
"""
import math

import bpy  # noqa: F401,I001
import bmesh
from mathutils import Matrix, Vector

from artkit import core, parts
from artkit.shade import noise, object_coords, powder_coat, ramp

MESHES = ['SM_Tower', 'SM_Tower_Glow']
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {'tower_case': 2048, 'tower_front': 1024, 'tower_rubber': 128, 'tower_cable': 256}
AO_DISTANCE = 0.03
REVIEW_VIEWS = [('front', -55, 15, 1.5), ('side', -95, 10, 1.4), ('detail', -80, 20, 0.7, (0.0, 0.0, 0.3))]

W, D, H = 0.215, 0.44, 0.46
FEET = 0.012
X0, X1 = -W / 2, W / 2
Y0, Y1 = -D / 2, D / 2
Z0, Z1 = FEET, FEET + H
FAN = 0.12
FAN_D = 0.025


# ------------------------------------------------------------------ materials

def case_material():
    m = core.Mat('tower_case')
    tc, _ = object_coords(m)
    color, rough, metal, bump = powder_coat(m, tc, 0x111214, rough=0.5, chips=0.3)
    m.set('Base Color', color)
    m.set('Roughness', rough)
    m.set('Metallic', metal)
    m.set('Normal', bump.outputs['Normal'])
    return m


def front_material():
    """The perforated front and top: a dense grid of round holes, dark behind."""
    m = core.Mat('tower_front')
    tc, sep = object_coords(m)
    pitch = 0.0042
    # Holes on the planes the panels lie in (front: XZ, top: XY): take whichever two axes vary.
    def cell(a, b):
        fa = m.math('SUBTRACT', m.math('FRACT', m.math('DIVIDE', a, pitch)), 0.5)
        fb = m.math('SUBTRACT', m.math('FRACT', m.math('DIVIDE', b, pitch)), 0.5)
        return m.math('SQRT', m.math('ADD', m.math('MULTIPLY', fa, fa), m.math('MULTIPLY', fb, fb)))
    geo = m.node('ShaderNodeNewGeometry')
    n = m.node('ShaderNodeSeparateXYZ')
    m.link(geo.outputs['Normal'], n.inputs['Vector'])
    facing_y = ramp(m, m.math('ABSOLUTE', n.outputs['Y']), 0.6, 0.8)
    facing_z = ramp(m, m.math('ABSOLUTE', n.outputs['Z']), 0.6, 0.8)
    d_front = cell(sep.outputs['X'], sep.outputs['Z'])
    d_top = cell(sep.outputs['X'], sep.outputs['Y'])
    hole_f = m.math('MULTIPLY', ramp(m, m.math('SUBTRACT', 0.32, d_front), 0.0, 0.05), facing_y)
    hole_t = m.math('MULTIPLY', ramp(m, m.math('SUBTRACT', 0.32, d_top), 0.0, 0.05), facing_z)
    hole = m.math('MAXIMUM', hole_f, hole_t)
    m.set('Base Color', m.mix(hole, core.hex_linear(0x15161a), core.hex_linear(0x020203)))
    m.set('Roughness', m.math('ADD', 0.5, m.math('MULTIPLY', hole, 0.4)))
    bump = m.node('ShaderNodeBump', Strength=0.5, Distance=0.0006)
    m.link(m.math('SUBTRACT', 1.0, hole), bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def pcb_material():
    m = core.Mat('tower_pcb')
    tc, sep = object_coords(m)
    # Faint traces: thin lines along Y and Z broken up by noise.
    ly = ramp(m, m.math('SUBTRACT', 0.08, m.math('ABSOLUTE', m.math('SUBTRACT', m.math('FRACT', m.math('DIVIDE', sep.outputs['Y'], 0.0035)), 0.5))), 0.0, 0.02)
    lz = ramp(m, m.math('SUBTRACT', 0.08, m.math('ABSOLUTE', m.math('SUBTRACT', m.math('FRACT', m.math('DIVIDE', sep.outputs['Z'], 0.0042)), 0.5))), 0.0, 0.02)
    mask = ramp(m, noise(m, tc.outputs['Object'], scale=60.0, detail=2.0), 0.5, 0.55)
    trace = m.math('MULTIPLY', m.math('MAXIMUM', m.math('MULTIPLY', ly, mask), m.math('MULTIPLY', lz, m.math('SUBTRACT', 1.0, mask))), 0.8)
    m.set('Base Color', m.mix(trace, core.hex_linear(0x101215), core.hex_linear(0x22262c)))
    m.set('Roughness', m.math('SUBTRACT', 0.55, m.math('MULTIPLY', trace, 0.2)))
    return m


# ------------------------------------------------------------------ parts

def fan(name, at, axis, mats, glow):
    """A 120 mm RGB fan: square frame, hub and nine pitched blades; its bore ring and hub ring go to glow.
    Built around Z, then turned so its intake faces along -axis."""
    frame_m, blade_m = mats
    s, d = FAN, FAN_D
    sq = core.rounded_rect(-s / 2, -s / 2, s / 2, s / 2, 0.008, steps=3)
    bore = parts.circle(0.0585, 56)
    frame = parts.outline_prism('fan_frame', sq, [bore], -d / 2, d / 2)
    # Screw bosses at the corners.
    bosses = [parts.disc('boss', (sx * 0.0525, sy * 0.0525, 0.0), (0, 0, 1), 0.0045, d + 0.0006, 12) for sx in (-1, 1) for sy in (-1, 1)]
    hub = parts.disc('hub', (0, 0, 0.002), (0, 0, 1), 0.021, d * 0.78, 40)
    struts = [parts.rbox('strut', (0.0, 0.0, 0.0), (0.004, 0.04, 0.003), 0.001) for _ in range(4)]
    for k, st in enumerate(struts):
        a = math.radians(45 + 90 * k)
        st.data.transform(Matrix.Translation((0.04 * math.cos(a), 0.04 * math.sin(a), d / 2 - 0.002)) @ Matrix.Rotation(a - math.pi / 2, 4, 'Z'))
    parts.assign_all([frame, hub] + bosses + struts, frame_m)
    blades = []
    for k in range(9):
        bm = bmesh.new()
        a0 = 2 * math.pi * k / 9
        grid = []
        for i in range(5):
            r = 0.021 + (0.055 - 0.021) * i / 4
            row = []
            for j in range(7):
                t = j / 6
                a = a0 + (t - 0.5) * math.radians(30 + 8 * i / 4)
                z = (t - 0.5) * 0.016 * (1.0 - 0.25 * i / 4)
                row.append(bm.verts.new((r * math.cos(a), r * math.sin(a), z)))
            grid.append(row)
        for i in range(4):
            for j in range(6):
                bm.faces.new((grid[i][j], grid[i][j + 1], grid[i + 1][j + 1], grid[i + 1][j]))
        blade = core.mesh_object('blade', bm)
        mod = blade.modifiers.new('solid', 'SOLIDIFY')
        mod.thickness = 0.0014
        mod.offset = 0.0
        core.select_only([blade])
        bpy.ops.object.modifier_apply(modifier=mod.name)
        core.assign(blade, blade_m)
        blades.append(blade)
    ring = core.lathe('fan_ring', [(0.0582, d * 0.36), (0.0582, -d * 0.36)], segments=56)
    front = parts.annulus('fan_face', 0.054, 0.0585, d / 2, d / 2 + 0.0008, 56)
    hub_ring = parts.annulus('hub_ring', 0.012, 0.0175, d / 2 - 0.002, d / 2 - 0.0012, 32)
    rot = Vector((0, 0, 1)).rotation_difference(Vector(axis).normalized()).to_matrix().to_4x4()
    place = Matrix.Translation(at) @ rot
    body = [frame, hub] + bosses + struts + blades
    parts.xform(body, place)
    lit = [ring, front, hub_ring]
    parts.xform(lit, place)
    glow.extend(lit)
    return body


def build():
    core.reset()
    case = case_material()
    front_m = front_material()
    gloss_black = parts.gloss('tower_trim', 0x0b0b0d, rough=0.12)
    pcb = pcb_material()
    sink = parts.metal('tower_sink', 0x3b3e44, rough=0.38, brushed_axis='Y', anodized=True)
    fan_m = parts.plastic('tower_fan', 0x0e0e10, rough=0.45, scuffs=0.1)
    blade_m = parts.plastic('tower_blade', 0x2a2c31, rough=0.35, scuffs=0.0)
    gpu_m = parts.plastic('tower_gpu', 0x1a1b1f, rough=0.42, scuffs=0.2)
    plate_m = parts.metal('tower_plate', 0x26282d, rough=0.3, brushed_axis='Y', anodized=True)
    rubber = parts.rubber('tower_rubber')
    cable_m = parts.braided('tower_cable', 0x141414)
    glow = []
    body = []

    # The case: right side, top, bottom, back, front, the open side's frame and feet.
    t = 0.004
    shell = [parts.rbox('side_r', (X1 - t / 2, 0, (Z0 + Z1) / 2), (t, D, H), 0.0015),
             parts.rbox('bottom', (0, 0, Z0 + t / 2), (W, D, t), 0.0015),
             parts.rbox('back', (0, Y1 - t / 2, (Z0 + Z1) / 2), (W, t, H), 0.0015)]
    rim = [parts.rbox('rim_top', (X0 + 0.003, 0, Z1 - 0.007), (0.006, D, 0.014), 0.002),
           parts.rbox('rim_bot', (X0 + 0.003, 0, Z0 + 0.007), (0.006, D, 0.014), 0.002),
           parts.rbox('rim_back', (X0 + 0.003, Y1 - 0.007, (Z0 + Z1) / 2), (0.006, 0.014, H), 0.002)]
    parts.assign_all(shell + rim, case)
    face = parts.rbox('front', (0, Y0 + 0.006, (Z0 + Z1) / 2), (W, 0.012, H), 0.004)
    top = parts.rbox('top', (0, 0.006, Z1 - t / 2), (W, D - 0.012, t), 0.0015)
    parts.assign_all([face, top], front_m)
    # A gloss trim around the front and the top's I/O: power button, two ports.
    trim = [parts.rbox('trim_l', (X0 + 0.004, Y0 + 0.001, (Z0 + Z1) / 2), (0.008, 0.004, H), 0.0015),
            parts.rbox('trim_r', (X1 - 0.004, Y0 + 0.001, (Z0 + Z1) / 2), (0.008, 0.004, H), 0.0015),
            parts.rbox('io', (0, Y0 + 0.04, Z1 + 0.0004), (0.09, 0.03, 0.002), 0.001),
            parts.disc('power', (0.03, Y0 + 0.04, Z1 + 0.0018), (0, 0, 1), 0.0065, 0.0025, 24)]
    trim += [parts.rbox('usb', (x, Y0 + 0.04, Z1 + 0.0014), (0.013, 0.006, 0.001), 0.0004) for x in (-0.03, -0.012)]
    parts.assign_all(trim, gloss_black)
    feet = [parts.rbox('foot', (sx * 0.08, sy * 0.18, FEET / 2), (0.03, 0.06, FEET), 0.003) for sx in (-1, 1) for sy in (-1, 1)]
    parts.assign_all(feet, rubber)
    glow += [parts.rbox('strip', (sx * 0.093, Y0 - 0.0005, (Z0 + Z1) / 2), (0.003, 0.002, H * 0.86), 0.0008) for sx in (-1, 1)]
    glow.append(parts.annulus('power_ring', 0.0066, 0.0078, Z1 + 0.0004, Z1 + 0.0026, 24, Matrix.Translation((0.03, Y0 + 0.04, 0))))

    # Motherboard on the right wall, its chipset and VRM heatsinks, the rear I/O shield.
    bx = X1 - t - 0.008
    board = parts.rbox('board', (bx, 0.03, 0.29), (0.003, 0.244, 0.3), 0.001)
    core.assign(board, pcb)
    sinks = [parts.rbox('vrm_top', (bx - 0.012, 0.075, 0.415), (0.022, 0.08, 0.02), 0.002),
             parts.rbox('vrm_side', (bx - 0.012, 0.13, 0.36), (0.022, 0.022, 0.09), 0.002),
             parts.rbox('chipset', (bx - 0.006, -0.03, 0.17), (0.01, 0.06, 0.05), 0.002),
             parts.rbox('m2', (bx - 0.004, 0.03, 0.245), (0.006, 0.09, 0.022), 0.0015)]
    parts.assign_all(sinks, sink)
    body += [board] + sinks

    # The AIO: a pump block on the CPU, tubes up to a radiator under the top with two fans pulling through it.
    pump_c = Vector((bx - 0.015, 0.07, 0.355))
    pump = parts.disc('pump', pump_c, (1, 0, 0), 0.033, 0.026, 48)
    cap = parts.disc('pump_cap', pump_c - Vector((0.0135, 0, 0)), (1, 0, 0), 0.027, 0.002, 48)
    core.assign(pump, gpu_m)
    core.assign(cap, gloss_black)
    glow.append(parts.annulus('pump_ring', 0.027, 0.0315, 0.0, 0.0012, 48,
                              Matrix.Translation(pump_c - Vector((0.0136, 0, 0))) @ parts.rot_y(-90)))
    glow.append(parts.annulus('pump_logo', 0.0, 0.006, 0.0, 0.0012, 24, Matrix.Translation(pump_c - Vector((0.0146, 0, 0))) @ parts.rot_y(-90)))
    rad = parts.rbox('radiator', (-0.012, -0.015, Z1 - t - 0.0145), (0.122, 0.29, 0.027), 0.002)
    core.assign(rad, sink)
    body += [pump, cap, rad]
    for k, dy in enumerate((-0.008, 0.008)):
        a = pump_c + Vector((-0.004, dy, 0.03))
        tube_pts = [a, a + Vector((-0.006, 0.0, 0.03)), Vector((-0.03 + k * 0.02, -0.145 + dy, Z1 - 0.05)), Vector((-0.03 + k * 0.02, -0.145 + dy, Z1 - t - 0.028))]
        tb = parts.tube('aio_tube', tube_pts, 0.0055, 12, 8)
        core.assign(tb, cable_m)
        body.append(tb)
    for y in (-0.055, 0.065):
        body += fan('rad_fan', (-0.012, y, Z1 - t - 0.028 - FAN_D / 2), (0, 0, -1), (fan_m, blade_m), glow)

    # RAM: two sticks in the slots right of the CPU, lit along their tops.
    for k in range(2):
        y = -0.012 - k * 0.0095
        stick = parts.rbox('ram', (bx - 0.02, y, 0.355), (0.036, 0.0065, 0.133), 0.0012)
        core.assign(stick, plate_m)
        body.append(stick)
        glow.append(parts.rbox('ram_bar', (bx - 0.0395, y, 0.355), (0.004, 0.0055, 0.125), 0.0012))

    # Graphics card: a long shroud below the CPU, backplate up, a lit strip along the edge facing out.
    gpu_c = Vector((bx - 0.065, 0.065, 0.205))
    gpu = parts.rbox('gpu', gpu_c, (0.124, 0.30, 0.048), 0.004)
    core.assign(gpu, gpu_m)
    backplate = parts.rbox('backplate', gpu_c + Vector((0.0, 0.0, 0.0255)), (0.12, 0.29, 0.003), 0.001)
    bracket = parts.rbox('bracket', (gpu_c.x, Y1 - 0.012, gpu_c.z), (0.12, 0.003, 0.06), 0.001)
    core.assign(backplate, plate_m)
    core.assign(bracket, sink)
    body += [gpu, backplate, bracket]
    glow.append(parts.rbox('gpu_strip', (gpu_c.x - 0.0625, gpu_c.y - 0.02, gpu_c.z + 0.008), (0.002, 0.2, 0.006), 0.0008))
    glow.append(parts.rbox('gpu_logo', (gpu_c.x - 0.0625, gpu_c.y + 0.11, gpu_c.z - 0.008), (0.002, 0.04, 0.008), 0.0008))

    # PSU shroud, and the card's power cable rising out of it.
    shroud = parts.rbox('shroud', (0.0, 0.02, Z0 + t + 0.05), (W - 2 * t, D - 0.06 - 0.012, 0.1), 0.003)
    core.assign(shroud, case)
    badge = parts.rbox('badge', (X0 + 0.0005, 0.06, Z0 + t + 0.05), (0.002, 0.08, 0.018), 0.001)
    core.assign(badge, gloss_black)
    body += [shroud, badge]
    edge = gpu_c.x - 0.062  # the card's outer face, where its power plugs in
    pc = parts.tube('gpu_power', [(edge - 0.022, 0.12, Z0 + t + 0.101), (edge - 0.028, 0.12, 0.17), (edge - 0.018, 0.10, gpu_c.z - 0.002),
                                  (edge - 0.002, 0.10, gpu_c.z)], 0.005, 10, 8)
    core.assign(pc, cable_m)
    body.append(pc)

    # Intake fans behind the front, the exhaust at the back.
    for z in (0.11, 0.235, 0.36):
        body += fan('front_fan', (0.0, Y0 + 0.012 + FAN_D / 2 + 0.002, z), (0, 1, 0), (fan_m, blade_m), glow)
    body += fan('rear_fan', (-0.01, Y1 - t - FAN_D / 2 - 0.002, 0.36), (0, -1, 0), (fan_m, blade_m), glow)

    tower = parts.finish(shell + rim + [face, top] + trim + feet + body, 'SM_Tower', 40)
    lit = parts.glow_object(glow, 'SM_Tower_Glow')
    return [tower, lit]


def bake_parts(objs):
    return [objs[0]]
