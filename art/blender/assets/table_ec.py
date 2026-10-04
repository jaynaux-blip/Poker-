"""SM_PokerTable_EC: the Embercrest's tournament table.

The same table as SM_PokerTable (its shape, seats, rail and cup holders, so the players sit to it the same way),
dressed for the card room instead of the laundromat: new navy speed cloth with the Embercrest's mark and a copper
betting line, a black leather rail, clean stainless cups.
"""
import os
import sys

from artkit import core
from artkit.shade import image_surface, noise, object_coords, planar, ramp, scaled, smooth_less
from artkit.sheet import Sheet

from assets import table as T

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import brand  # noqa: E402

MESHES = ['SM_PokerTable_EC']
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = dict(T.TEXTURE_SIZES)
AO_DISTANCE = T.AO_DISTANCE

CLOTH = 0x14213d
INK = 0x23345a


def felt_print():
    """The mark and the name in the middle of the cloth (millimeters, 700 x 360), printed a shade off the cloth."""
    s = Sheet(700, 360)
    brand.mark(s, 350, 222, 150, mono=INK, ground=CLOTH)
    s.text('EMBERCREST', 350, 82, 44, INK, face='Black', align='CENTER', tracking=1.22, rough=0.9)
    s.text('POKER SERIES', 350, 52, 16, 0x5a3f2c, face='Bold', align='CENTER', tracking=1.9, rough=0.9)
    return s.render('table_ec_print', 2048)


def felt_material():
    m = core.Mat('table_felt')
    tc, sep = object_coords(m)
    obj = tc.outputs['Object']
    fiber = noise(m, obj, 5200.0, 2.0)
    fuzz = noise(m, scaled(m, sep, 900.0, 900.0, 1.0), 1.0, 3.0)
    mottle = noise(m, obj, 5.0, 4.0)
    paths = felt_print()
    uv = planar(m, sep, 'X', 'Y', -0.35, 0.35, -0.18, 0.18)
    pcol, palpha, _, _ = image_surface(m, paths, uv, extension='CLIP')
    navy = m.mix(m.math('MULTIPLY', mottle, 0.4), core.hex_linear(0x111c35), core.hex_linear(CLOTH))
    color = m.mix(m.math('MULTIPLY', fuzz, 0.2), navy, core.hex_linear(0x1a2848))
    color = m.mix(m.math('MULTIPLY', palpha, 0.9), color, pcol)
    # The betting line: a thin copper oval 13.5 cm in from the rail.
    d = T.stadium_dist(m, sep)
    line = smooth_less(m, m.math('ABSOLUTE', m.math('SUBTRACT', d, T.FELT_R - 0.135)), 0.003, 0.0007)
    color = m.mix(m.math('MULTIPLY', line, 0.92), color, core.hex_linear(0xb8743f))
    m.set('Base Color', color)
    m.set('Roughness', m.math('ADD', 0.86, m.math('MULTIPLY', fiber, 0.08)))
    m.set('Metallic', 0.0)
    # Speed cloth hardly reflects: at the default specular the navy reads slate grey under the pendant.
    m.set('Specular IOR Level', 0.08)
    bump = m.node('ShaderNodeBump', Strength=0.25, Distance=0.00025)
    m.link(fiber, bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def rail_material(perimeter):
    """Black pebble-grain leather with a soft sheen."""
    m = core.Mat('table_rail')
    tc, _ = object_coords(m)
    pebble = m.node('ShaderNodeTexVoronoi', Scale=2600.0)
    m.link(tc.outputs['Object'], pebble.inputs['Vector'])
    grain = ramp(m, pebble.outputs['Distance'], 0.1, 0.5)
    m.set('Base Color', m.mix(m.math('MULTIPLY', grain, 0.35), core.hex_linear(0x0b0c0e), core.hex_linear(0x17181c)))
    m.set('Roughness', m.math('ADD', 0.4, m.math('MULTIPLY', grain, 0.16)))
    m.set('Coat Weight', 0.15)
    bump = m.node('ShaderNodeBump', Strength=0.35, Distance=0.0004)
    m.link(grain, bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def steel_material():
    m = core.Mat('table_steel')
    tc, sep = object_coords(m)
    brushed = noise(m, scaled(m, sep, 3000.0, 3000.0, 20.0), 1.0, 2.0)
    m.set('Base Color', core.hex_linear(0xc0c1c2))
    m.set('Roughness', m.math('ADD', 0.24, m.math('MULTIPLY', brushed, 0.1)))
    m.set('Metallic', 1.0)
    return m


def base_material():
    """The pedestals: clean black powder coat."""
    from artkit.shade import powder_coat
    m = core.Mat('table_base')
    tc, _ = object_coords(m)
    color, rough, metal, bump = powder_coat(m, tc, 0x0f1013, rough=0.45, chips=0.0)
    m.set('Base Color', color)
    m.set('Roughness', rough)
    m.set('Metallic', metal)
    m.set('Normal', bump.outputs['Normal'])
    return m


def build():
    # The base table's own materials come back afterwards, so a 'table' build in the same run stays the laundromat's.
    saved = (T.base_material, T.felt_material, T.rail_material, T.steel_material)
    T.base_material, T.felt_material, T.rail_material, T.steel_material = base_material, felt_material, rail_material, steel_material
    try:
        objs = T.build()
    finally:
        T.base_material, T.felt_material, T.rail_material, T.steel_material = saved
    objs[0].name = 'SM_PokerTable_EC'
    objs[0].data.name = 'SM_PokerTable_EC'
    return objs
