"""Finishes for things that live outdoors or in a shop (the street and the Lucky Penny): paint that has
seen weather, galvanized and cast metal, car paint, tires, shop plastics.

The desk props' finishes (parts.py) are scaled for things held in the hand; these are scaled for things a
meter or several tall, seen from a sidewalk: chips and rust a few centimeters across, grime that rises
from the ground, streaks the rain leaves running down.
"""
from . import core
from .shade import convex_edges, mix_float, noise, object_coords, ramp, scaled


def _grime_mask(m, sep, height):
    """1 at the ground fading out by height (meters), broken up: road dirt and splash."""
    rise = m.math('SUBTRACT', 1.0, m.math('DIVIDE', sep.outputs['Z'], height), clamp=True)
    return rise


def painted(name, hex_color, rough=0.42, chips=0.6, rust=0.4, grime=0.5, grime_height=0.6, streaks=0.4, primer_hex=0x6b6f72):
    """Enamel on steel or iron, outdoors for years: chipped on the edges to primer and rust, rain streaks
    running down, road grime at the foot."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    mottle = noise(m, o, scale=6.0, detail=4.0)
    edge = m.math('MULTIPLY', convex_edges(m, tc, distance=0.004, samples=12, breakup_scale=24.0, gain=5.0), chips)
    blotch = ramp(m, noise(m, o, scale=14.0, detail=6.0, roughness=0.6), 0.66, 0.74)
    chipped = m.math('MAXIMUM', ramp(m, edge, 0.35, 0.55), m.math('MULTIPLY', blotch, chips * 0.6))
    rusty = m.math('MULTIPLY', chipped, ramp(m, noise(m, o, scale=30.0, detail=3.0), 0.45 - 0.3 * rust, 0.6 - 0.3 * rust))
    streak = m.math('MULTIPLY', ramp(m, noise(m, scaled(m, sep, 40.0, 40.0, 1.2), detail=3.0), 0.55, 0.75), streaks)
    dirt = m.math('MULTIPLY', m.math('MULTIPLY', _grime_mask(m, sep, grime_height), ramp(m, noise(m, o, scale=9.0, detail=4.0), 0.25, 0.7)), grime)

    paint = m.mix(m.math('MULTIPLY', mottle, 0.35), core.hex_linear(hex_color), core.hex_linear(_shade(hex_color, 0.8)))
    col = m.mix(chipped, paint, core.hex_linear(primer_hex))
    col = m.mix(rusty, col, core.hex_linear(0x5a3018))
    col = m.mix(m.math('MULTIPLY', streak, 0.5), col, core.hex_linear(0x3a3226))
    col = m.mix(m.math('MULTIPLY', dirt, 0.8), col, core.hex_linear(0x2a2620))
    m.set('Base Color', col)
    r = m.math('ADD', rough, m.math('MULTIPLY', m.math('SUBTRACT', mottle, 0.5), 0.12))
    r = mix_float(m, chipped, r, 0.7)
    r = mix_float(m, rusty, r, 0.88)
    r = mix_float(m, dirt, r, 0.85)
    m.set('Roughness', r)
    bump = m.node('ShaderNodeBump', Strength=0.25, Distance=0.0015)
    m.link(m.math('ADD', m.math('MULTIPLY', rusty, 0.6), m.math('MULTIPLY', chipped, -0.4)), bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def galvanized(name, rough=0.48, grime=0.4, grime_height=0.8):
    """Hot-dip galvanized steel: a dull zinc with its spangle, darker where hands and weather have been."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    spangle = noise(m, o, scale=60.0, detail=1.0, distortion=1.5)
    patina = noise(m, o, scale=5.0, detail=4.0)
    dirt = m.math('MULTIPLY', _grime_mask(m, sep, grime_height), grime)
    col = m.mix(m.math('MULTIPLY', spangle, 0.25), core.hex_linear(0x9ea3a6), core.hex_linear(0xc3c7c9))
    col = m.mix(m.math('MULTIPLY', patina, 0.5), col, core.hex_linear(0x7d8184))
    col = m.mix(dirt, col, core.hex_linear(0x3b3a36))
    m.set('Base Color', col)
    m.set('Metallic', mix_float(m, dirt, 0.9, 0.4))
    m.set('Roughness', m.math('ADD', rough, m.math('MULTIPLY', m.math('SUBTRACT', spangle, 0.5), 0.15)))
    return m


def bare_metal(name, hex_color=0x8e9296, rough=0.35, metal=1.0, grime=0.2):
    """Stainless, chrome or machined steel with a light film of handling."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    smudge = noise(m, o, scale=20.0, detail=4.0)
    m.set('Base Color', m.mix(m.math('MULTIPLY', smudge, grime), core.hex_linear(hex_color), core.hex_linear(_shade(hex_color, 0.7))))
    m.set('Metallic', metal)
    m.set('Roughness', m.math('ADD', rough, m.math('MULTIPLY', ramp(m, smudge, 0.5, 0.7), 0.15)))
    return m


def concrete(name, hex_color=0x8f8b84, rough=0.88):
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    blot = noise(m, o, scale=4.0, detail=5.0)
    pits = ramp(m, noise(m, o, scale=220.0, detail=1.0), 0.68, 0.72)
    col = m.mix(m.math('MULTIPLY', blot, 0.5), core.hex_linear(hex_color), core.hex_linear(_shade(hex_color, 0.75)))
    m.set('Base Color', m.mix(m.math('MULTIPLY', pits, 0.5), col, core.hex_linear(0x3a3936)))
    m.set('Roughness', rough)
    bump = m.node('ShaderNodeBump', Strength=0.3, Distance=0.001)
    m.link(m.math('SUBTRACT', blot, pits), bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def shop_plastic(name, hex_color, rough=0.45, grime=0.25):
    """Molded plastic that's been wiped down a thousand times: even, a little scuffed."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    g = noise(m, o, scale=400.0, detail=1.0)
    scuff = m.math('MULTIPLY', ramp(m, noise(m, o, scale=18.0, detail=5.0), 0.6, 0.75), grime)
    m.set('Base Color', m.mix(scuff, core.hex_linear(hex_color), core.hex_linear(_shade(hex_color, 0.85))))
    m.set('Roughness', m.math('ADD', rough, m.math('MULTIPLY', m.math('SUBTRACT', g, 0.5), 0.08)))
    bump = m.node('ShaderNodeBump', Strength=0.04, Distance=0.0002)
    m.link(g, bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def car_paint(name, hex_color, rough=0.18, flake=0.25, dirt=0.35):
    """Two-stage paint with metallic flake under clear coat; road film along the sills and wheel arches."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    flakes = noise(m, o, scale=900.0, detail=1.0)
    film = m.math('MULTIPLY', m.math('MULTIPLY', _grime_mask(m, sep, 0.55), ramp(m, noise(m, o, scale=7.0, detail=4.0), 0.3, 0.8)), dirt)
    col = m.mix(m.math('MULTIPLY', flakes, flake), core.hex_linear(hex_color), core.hex_linear(_shade(hex_color, 1.25)))
    m.set('Base Color', m.mix(film, col, core.hex_linear(0x4a463f)))
    m.set('Metallic', mix_float(m, film, 0.55, 0.1))
    m.set('Roughness', mix_float(m, film, m.math('ADD', rough, m.math('MULTIPLY', flakes, 0.05)), 0.7))
    m.set('Coat Weight', 1.0)
    m.set('Coat Roughness', 0.04)
    return m


def tire(name):
    """Tire rubber: matte, browned and dusty on the sidewall, the tread a little shinier."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    dust = ramp(m, noise(m, o, scale=12.0, detail=4.0), 0.4, 0.8)
    m.set('Base Color', m.mix(m.math('MULTIPLY', dust, 0.6), core.hex_linear(0x161616), core.hex_linear(0x3a352e)))
    m.set('Roughness', mix_float(m, dust, 0.75, 0.92))
    bump = m.node('ShaderNodeBump', Strength=0.2, Distance=0.0008)
    m.link(noise(m, o, scale=300.0, detail=1.0), bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def glass(name, hex_color=0x0d1014, rough=0.04):
    """Glass seen at night: no transmission in the bake, so a dark mirror with a faint film of grime."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    film = ramp(m, noise(m, tc.outputs['Object'], scale=8.0, detail=4.0), 0.5, 0.8)
    m.set('Base Color', core.hex_linear(hex_color))
    m.set('Roughness', m.math('ADD', rough, m.math('MULTIPLY', film, 0.12)))
    m.set('Specular IOR Level', 0.6)
    return m


def lens(name, hex_color, strength=0.0, rough=0.15):
    """A light's plastic lens: colored, glossy; glowing when strength > 0."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    prism = noise(m, scaled(m, sep, 300.0, 40.0, 300.0), detail=1.0)
    m.set('Base Color', core.hex_linear(hex_color))
    m.set('Roughness', m.math('ADD', rough, m.math('MULTIPLY', prism, 0.1)))
    if strength > 0.0:
        m.set('Emission Color', core.hex_linear(hex_color))
        m.set('Emission Strength', strength)
    return m


def _shade(h, f):
    r, g, b = (h >> 16) & 255, (h >> 8) & 255, h & 255
    r, g, b = (max(0, min(255, int(c * f))) for c in (r, g, b))
    return (r << 16) | (g << 8) | b
