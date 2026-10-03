"""SM_Hydrant: the fire hydrant on Fifth Street's sidewalk, in front of the apartment building.

A dry-barrel hydrant: a bolted ground flange, a barrel in chrome-yellow enamel, the nozzle section with its
big pumper nozzle to the street and a hose nozzle each side, a bolted bonnet flange and the domed bonnet
with its pentagon operating nut. Bonnet and caps are in the light blue that marks the high-flow hydrants.
Years of weather: chipped to primer and rust on every edge, rain streaks, road grime splashed up the foot.
A chain keeps the pumper cap from wandering off.

Coordinates (meters), front toward -Y (the pumper nozzle, which faces the street), Z up: origin on the
sidewalk under the barrel's axis.
"""
import math

import bpy  # noqa: F401,I001
from mathutils import Matrix, Vector

from artkit import core, parts, street

MESHES = ['SM_Hydrant']
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {'hydrant_chain': 256, 'hydrant_bolts': 256}
AO_DISTANCE = 0.03
REVIEW_VIEWS = [('front', -30, 14, 2.5), ('side', 70, 20, 2.5), ('detail', -15, 10, 1.1, (0.0, -0.12, 0.47))]

PUMPER_Z = 0.47
HOSE_Z = 0.45


def nozzle(axis_deg, z, r, length, cap_r, body_m, cap_m, bolt_m):
    """A nozzle out of the barrel along azimuth axis_deg (0 is -Y), its cap and the cap's nut."""
    a = math.radians(axis_deg)
    d = Vector((math.sin(a), -math.cos(a), 0.0))
    base = Vector((0.0, 0.0, z))
    out = []
    out.append((parts.rod('nozzle', base + d * 0.06, base + d * (0.10 + length), r, 32), body_m))
    out.append((parts.disc('collar', base + d * (0.10 + length * 0.55), d, r + 0.008, 0.016, 32), body_m))
    cap_c = base + d * (0.10 + length + 0.022)
    out.append((parts.disc('cap', cap_c, d, cap_r, 0.044, 40), cap_m))
    # Lugs on the cap's rim, and the pentagon nut on its face.
    for k in range(2):
        ang = math.pi * k
        side = d.cross(Vector((0, 0, 1))).normalized()
        lug_c = cap_c + (side * math.cos(ang) + Vector((0, 0, 1)) * math.sin(ang)) * (cap_r + 0.006)
        out.append((parts.rbox('lug', lug_c, (0.014, 0.014, 0.014), 0.003), cap_m))
    out.append((parts.disc('nut', cap_c + d * 0.033, d, cap_r * 0.42, 0.024, 5), cap_m))
    return out


def build():
    core.reset()
    body_m = street.painted('hydrant_body', 0xcf9a17, rough=0.5, chips=1.3, rust=0.7, grime=1.0, grime_height=0.4, streaks=0.7)
    cap_m = street.painted('hydrant_cap', 0x4f93c4, rough=0.5, chips=1.4, rust=0.8, grime=0.5, grime_height=0.35, streaks=0.6)
    bolt_m = street.painted('hydrant_bolts', 0x6d6a5f, rough=0.6, chips=1.0, rust=0.9, grime=0.6)
    chain_m = street.bare_metal('hydrant_chain', 0x6e6a63, rough=0.55, grime=0.6)
    out = []
    # The barrel, revolved: ground flange, the shaft, the nozzle section's swell, the bonnet flange.
    barrel = [(0.0, 0.0), (0.165, 0.0), (0.168, 0.012), (0.165, 0.028), (0.128, 0.034), (0.118, 0.06),
              (0.112, 0.34), (0.128, 0.38), (0.134, 0.42), (0.134, 0.52), (0.124, 0.555), (0.112, 0.585),
              (0.146, 0.594), (0.148, 0.618), (0.12, 0.626), (0.118, 0.632)]
    out.append((parts.lathe_at('barrel', barrel, Matrix(), segments=64), body_m))
    bonnet = [(0.118, 0.632), (0.116, 0.66), (0.104, 0.69), (0.084, 0.712), (0.056, 0.728), (0.03, 0.735), (0.0, 0.737)]
    out.append((parts.lathe_at('bonnet', bonnet, Matrix(), segments=64), cap_m))
    out.append((parts.disc('op_nut', (0.0, 0.0, 0.752), (0, 0, 1), 0.024, 0.032, 5), cap_m))
    out.append((parts.disc('op_seat', (0.0, 0.0, 0.738), (0, 0, 1), 0.032, 0.006, 32), cap_m))
    # Bolts: eight on the ground flange, eight on the bonnet flange.
    for k in range(8):
        a = 2 * math.pi * (k + 0.5) / 8
        for r, z in ((0.145, 0.03), (0.135, 0.622)):
            c = Vector((r * math.cos(a), r * math.sin(a), z + 0.006))
            out.append((parts.disc('bolt', c, (0, 0, 1), 0.011, 0.012, 6), bolt_m))
    # Nozzles: the pumper to the street, a hose nozzle each side.
    out += nozzle(0.0, PUMPER_Z, 0.062, 0.05, 0.072, body_m, cap_m, bolt_m)
    out += nozzle(90.0, HOSE_Z, 0.038, 0.04, 0.046, body_m, cap_m, bolt_m)
    out += nozzle(-90.0, HOSE_Z, 0.038, 0.04, 0.046, body_m, cap_m, bolt_m)
    # The cap chain: an eye on the barrel, a slack run of links down to the cap's lug.
    eye = Vector((0.07, -0.105, PUMPER_Z + 0.075))
    lug = Vector((0.0, -0.236, PUMPER_Z + 0.078))
    out.append((parts.disc('chain_eye', eye, (1, 0, 0), 0.012, 0.012, 16), bolt_m))
    # Links of 3 mm wire, 2 cm long, each turned a quarter from the last, sagging between the two.
    pts = [eye.lerp(lug, t / 40) + Vector((0.0, 0.0, -0.05 * math.sin(math.pi * t / 40))) for t in range(41)]
    lengths = [0.0]
    for p0, p1 in zip(pts, pts[1:]):
        lengths.append(lengths[-1] + (p1 - p0).length)
    pitch = 0.013
    count = int(lengths[-1] / pitch)
    for i in range(count):
        d = (i + 0.5) * pitch
        k = next(j for j in range(1, len(lengths)) if lengths[j] >= d)
        t = (d - lengths[k - 1]) / (lengths[k] - lengths[k - 1])
        p = pts[k - 1].lerp(pts[k], t)
        tangent = (pts[k] - pts[k - 1]).normalized()
        side = tangent.cross(Vector((0, 0, 1))).normalized()
        up = side.cross(tangent).normalized()
        across = side if i % 2 == 0 else up
        oval = [(0.0065 * math.cos(2 * math.pi * j / 16) + (0.003 if math.cos(2 * math.pi * j / 16) > 0 else -0.003), 0.0042 * math.sin(2 * math.pi * j / 16)) for j in range(16)]
        path = [p + tangent * a_ + across * b_ for a_, b_ in oval]
        out.append((core.sweep('link', path, parts.circle(0.0012, 6), closed=True), chain_m))
    for o, m in out:
        core.assign(o, m)
    return [parts.finish([o for o, _ in out], 'SM_Hydrant', 40)]
