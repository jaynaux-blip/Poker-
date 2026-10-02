"""SM_Headphones: GearDrop's HUSH noise-cancelling headphones, hung on a desk stand.

The stand: a weighted base, a stem and a padded saddle. The headphones: a padded headband over the saddle,
brushed sliders into the yokes, oval cups in slate with protein-leather cushions facing each other,
the noise-cancelling mics as small grilles and a power LED.

Coordinates (meters), front toward -Y, Z up: origin on the desk under the stand's center.
"""
import math

import bpy  # noqa: F401,I001
from mathutils import Matrix, Vector

from artkit import core, parts
from artkit.shade import noise, object_coords, ramp

MESHES = ['SM_Headphones']
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {'hp_metal': 256, 'hp_led': 64, 'hp_cloth': 256}
AO_DISTANCE = 0.01
REVIEW_VIEWS = [('front', -25, 15, 0.8), ('side', 75, 15, 0.8), ('detail', -40, 10, 0.35, (0.08, 0.0, 0.11))]

SADDLE_Z = 0.268
BAND_R = 0.09
BAND_C = Vector((0.0, 0.0, SADDLE_Z + 0.012 + 0.006 + 0.004 - BAND_R))
CUP_X = 0.088
CUP_Z = 0.112
OVAL = 1.28  # cups are taller than wide


def leather_material(name, hex_color):
    m = core.Mat(name)
    tc, _ = object_coords(m)
    grain = noise(m, tc.outputs['Object'], scale=2800.0, detail=2.0)
    creases = ramp(m, noise(m, tc.outputs['Object'], scale=180.0, detail=3.0), 0.55, 0.62)
    m.set('Base Color', m.mix(m.math('MULTIPLY', grain, 0.35), core.hex_linear(hex_color), core.hex_linear(0x3a3f47)))
    m.set('Roughness', m.math('ADD', 0.45, m.math('MULTIPLY', creases, 0.2)))
    bump = m.node('ShaderNodeBump', Strength=0.25, Distance=0.0002)
    m.link(m.math('ADD', m.math('MULTIPLY', grain, 0.3), creases), bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def cup(side, mats):
    """One ear cup, its cushion facing the other cup (side -1: left, +1: right)."""
    shell = core.lathe('shell', [(0.0, 0.034), (0.012, 0.0335), (0.024, 0.031), (0.033, 0.025), (0.0385, 0.016), (0.04, 0.006), (0.04, 0.0)], segments=64)
    core.orient_normals(shell)
    core.assign(shell, mats['shell'])
    ring = core.lathe('trim', [(0.0405, 0.0), (0.0405, 0.004), (0.039, 0.0045)], segments=64)
    core.orient_normals(ring)
    core.assign(ring, mats['metal'])
    sec = core.rounded_rect(0.02, -0.022, 0.041, 0.0, 0.008, steps=5)
    cushion = core.lathe('cushion', sec + [sec[0]], segments=64)
    core.orient_normals(cushion)
    core.assign(cushion, mats['leather'])
    cloth = parts.disc('cloth', (0, 0, -0.012), (0, 0, 1), 0.021, 0.002, 40)
    core.assign(cloth, mats['cloth'])
    mic = [parts.disc('mic', (0.024 * math.cos(math.radians(a)), 0.024 * math.sin(math.radians(a)), 0.0302), (0, 0, 1), 0.0022, 0.002, 12) for a in (40, 140)]
    parts.assign_all(mic, mats['cloth'])
    objs = [shell, ring, cushion, cloth] + mic
    if side > 0:
        led = parts.disc('led', (0.0, -0.03, 0.019), (0.0, -0.6, 0.8), 0.0012, 0.001, 10)
        core.assign(led, mats['led'])
        objs.append(led)
    # Oval, then turned so the cushion (-Z) faces the middle, and placed.
    scale = Matrix.Diagonal((1.0, OVAL, 1.0, 1.0))
    turn = parts.rot_y(-90 * side) @ parts.rot_z(90)
    parts.xform(objs, Matrix.Translation((side * (CUP_X + 0.01), 0.0, CUP_Z)) @ turn @ scale)
    return objs


def build():
    core.reset()
    mats = {'shell': parts.plastic('hp_shell', 0x2b3038, rough=0.38, scuffs=0.3), 'metal': parts.metal('hp_metal', 0xbfc3c9, rough=0.2, brushed_axis='Z'),
            'leather': leather_material('hp_leather', 0x1c1f24), 'cloth': parts.fabric('hp_cloth', 0x0d0e10, period=0.0008),
            'led': parts.emissive('hp_led', 0x7dd3fc, 1.0), 'stand': parts.metal('hp_stand', 0x1d1e21, rough=0.4, anodized=True),
            'felt': parts.fabric('hp_felt', 0x2a2b2e, period=0.0012), 'rubber': parts.rubber('hp_rubber')}
    out = []
    # The stand.
    base = core.lathe('base', [(0.0, 0.0), (0.056, 0.0), (0.058, 0.002), (0.058, 0.009), (0.054, 0.012), (0.012, 0.0135), (0.0, 0.0135)], segments=64)
    core.orient_normals(base)
    core.assign(base, mats['stand'])
    pad = core.lathe('pad', [(0.0, -0.0003), (0.05, -0.0003), (0.05, 0.0015), (0.0, 0.0015)], segments=48)
    core.orient_normals(pad)
    core.assign(pad, mats['rubber'])
    stem = parts.rod('stem', (0, 0, 0.012), (0, 0, SADDLE_Z - 0.008), 0.0075, 24)
    core.assign(stem, mats['stand'])
    arch = [(x, 0.0, SADDLE_Z + 0.008 * (1.0 - (x / 0.06) ** 2)) for x in (-0.06, -0.03, 0.0, 0.03, 0.06)]
    saddle = core.sweep('saddle', core.catmull_rom(arch, 6), core.rounded_rect(-0.006, -0.018, 0.006, 0.018, 0.0055, steps=4))
    core.assign(saddle, mats['felt'])
    out += [base, pad, stem, saddle]
    # The headband: the outer band and its padding underneath, over the saddle.
    arc = [(BAND_C.x + BAND_R * math.cos(math.radians(a)), 0.0, BAND_C.z + BAND_R * math.sin(math.radians(a))) for a in range(-8, 189, 6)]
    band = core.sweep('band', arc, core.rounded_rect(-0.004, -0.017, 0.004, 0.017, 0.0035, steps=3))
    core.assign(band, mats['shell'])
    inner = [(BAND_C.x + (BAND_R - 0.008) * math.cos(math.radians(a)), 0.0, BAND_C.z + (BAND_R - 0.008) * math.sin(math.radians(a))) for a in range(28, 153, 6)]
    padding = core.sweep('padding', inner, core.rounded_rect(-0.0045, -0.014, 0.0045, 0.014, 0.004, steps=3))
    core.assign(padding, mats['leather'])
    out += [band, padding]
    # Sliders down from the band's ends into the yokes, the yokes around the cups.
    for s in (-1, 1):
        top = Vector((s * BAND_R * math.cos(math.radians(-8)), 0.0, BAND_C.z + BAND_R * math.sin(math.radians(-8))))
        yoke_top = Vector((s * (CUP_X + 0.012), 0.0, CUP_Z + 0.057))
        slider = core.sweep('slider', [top + Vector((0, 0, 0.004)), yoke_top], core.rounded_rect(-0.0015, -0.006, 0.0015, 0.006, 0.0012, steps=2))
        core.assign(slider, mats['metal'])
        fork = core.sweep('yoke', core.catmull_rom([(s * (CUP_X + 0.012), -0.046, CUP_Z), (s * (CUP_X + 0.014), -0.042, CUP_Z + 0.04), yoke_top,
                                                    (s * (CUP_X + 0.014), 0.042, CUP_Z + 0.04), (s * (CUP_X + 0.012), 0.046, CUP_Z)], 6),
                          core.rounded_rect(-0.003, -0.0035, 0.003, 0.0035, 0.002, steps=2))
        core.assign(fork, mats['shell'])
        out += [slider, fork] + cup(s, mats)
    return [parts.finish(out, 'SM_Headphones', 40)]
