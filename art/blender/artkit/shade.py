"""Shader-graph building blocks shared by the assets: masks, projections and worn finishes.

Everything works in object space (meters), so masks line up with the geometry exactly and
bake without seams; soft edges keep them free of jaggies at bake resolution.
"""
from . import core


def object_coords(m):
    """(Texture Coordinate node, Separate XYZ of its Object output)."""
    tc = m.node('ShaderNodeTexCoord')
    sep = m.node('ShaderNodeSeparateXYZ')
    m.link(tc.outputs['Object'], sep.inputs['Vector'])
    return tc, sep


def vec(m, x, y, z):
    """Combine three sockets or constants into a vector socket."""
    n = m.node('ShaderNodeCombineXYZ')
    for sock, v in zip(('X', 'Y', 'Z'), (x, y, z)):
        if isinstance(v, (int, float)):
            n.inputs[sock].default_value = v
        else:
            m.link(v, n.inputs[sock])
    return n.outputs['Vector']


def scaled(m, sep, sx, sy, sz, offset=(0.0, 0.0, 0.0)):
    """Object coordinates scaled per axis (for stretched noise: grain, brushing, fibers)."""
    return vec(m, m.math('MULTIPLY', m.math('ADD', sep.outputs['X'], offset[0]), sx),
               m.math('MULTIPLY', m.math('ADD', sep.outputs['Y'], offset[1]), sy),
               m.math('MULTIPLY', m.math('ADD', sep.outputs['Z'], offset[2]), sz))


def noise(m, vector, scale=1.0, detail=2.0, roughness=0.5, distortion=0.0):
    n = m.node('ShaderNodeTexNoise', Scale=scale, Detail=detail, Roughness=roughness, Distortion=distortion)
    m.link(vector, n.inputs['Vector'])
    return n.outputs['Fac']


def smooth_less(m, d, edge, width):
    """About 1 where d < edge, fading to 0 across edge +/- width: a soft step that bakes without jaggies."""
    n = m.node('ShaderNodeMapRange')
    n.interpolation_type = 'SMOOTHSTEP'
    n.inputs['From Min'].default_value = edge - width
    n.inputs['From Max'].default_value = edge + width
    n.inputs['To Min'].default_value = 1.0
    n.inputs['To Max'].default_value = 0.0
    m.link(d, n.inputs['Value'])
    return n.outputs['Result']


def ramp(m, x, lo, hi):
    """0 below lo, 1 above hi, smooth in between."""
    n = m.node('ShaderNodeMapRange')
    n.interpolation_type = 'SMOOTHSTEP'
    n.inputs['From Min'].default_value = lo
    n.inputs['From Max'].default_value = hi
    m.link(x, n.inputs['Value'])
    return n.outputs['Result']


def rrect_mask(m, sep, cx, cy, hw, hh, r, width=0.00008):
    """1 inside a rounded rectangle in object XY, 0 outside."""
    dx = m.math('MAXIMUM', m.math('SUBTRACT', m.math('ABSOLUTE', m.math('SUBTRACT', sep.outputs['X'], cx)), hw - r), 0.0)
    dy = m.math('MAXIMUM', m.math('SUBTRACT', m.math('ABSOLUTE', m.math('SUBTRACT', sep.outputs['Y'], cy)), hh - r), 0.0)
    d = m.math('SQRT', m.math('ADD', m.math('MULTIPLY', dx, dx), m.math('MULTIPLY', dy, dy)))
    return smooth_less(m, d, r, width)


def circle_dist(m, sep, cx, cy):
    """Distance from (cx, cy) in object XY."""
    dx = m.math('SUBTRACT', sep.outputs['X'], cx)
    dy = m.math('SUBTRACT', sep.outputs['Y'], cy)
    return m.math('SQRT', m.math('ADD', m.math('MULTIPLY', dx, dx), m.math('MULTIPLY', dy, dy)))


def planar(m, sep, a, b, a0, a1, b0, b1, mirror_a=False):
    """Object-space planar coordinates mapping [a0, a1] x [b0, b1] onto [0, 1]^2."""
    uv = m.node('ShaderNodeCombineXYZ')
    src = m.math('MULTIPLY', sep.outputs[a], -1.0) if mirror_a else sep.outputs[a]
    m.link(m.math('DIVIDE', m.math('SUBTRACT', src, a0), a1 - a0), uv.inputs['X'])
    m.link(m.math('DIVIDE', m.math('SUBTRACT', sep.outputs[b], b0), b1 - b0), uv.inputs['Y'])
    return uv.outputs['Vector']


def mix_float(m, factor, a, b):
    n = m.node('ShaderNodeMix')
    n.data_type = 'FLOAT'
    for sock, v in ((n.inputs['Factor'], factor), (n.inputs['A'], a), (n.inputs['B'], b)):
        if isinstance(v, (int, float)):
            sock.default_value = v
        else:
            m.link(v, sock)
    return n.outputs['Result']


def convex_edges(m, tc, distance=0.0007, samples=16, breakup_scale=90.0, gain=4.0):
    """Wear mask for convex edges only, broken up by noise.

    Occlusion traced inside the solid finds edges whatever the topology (Pointiness smears across
    the long triangles that booleans and large faces leave).
    """
    inside = m.node('ShaderNodeAmbientOcclusion', Distance=distance)
    inside.inside = True
    inside.only_local = True
    inside.samples = samples
    breakup = m.node('ShaderNodeTexNoise', Scale=breakup_scale, Detail=5.0)
    m.link(tc.outputs['Object'], breakup.inputs['Vector'])
    convex = m.math('MULTIPLY', m.math('SUBTRACT', 0.93, inside.outputs['AO']), gain, clamp=True)
    return m.math('MULTIPLY', convex, m.math('MULTIPLY', m.math('SUBTRACT', breakup.outputs['Fac'], 0.3), 2.2, clamp=True))


def image_surface(m, paths, vector, extension='EXTEND'):
    """A Sheet's color and surface images sampled at vector: (color socket, alpha, metal, rough)."""
    col = m.image(paths[0], vector=vector, extension=extension)
    surf = m.image(paths[1], non_color=True, vector=vector, extension=extension)
    sep = m.node('ShaderNodeSeparateColor')
    m.link(surf.outputs['Color'], sep.inputs['Color'])
    return col.outputs['Color'], col.outputs['Alpha'], sep.outputs['Red'], sep.outputs['Green']


def powder_coat(m, tc, color_hex, rough=0.55, chip_hex=0x8b8e93, chips=1.0):
    """Powder-coated steel: satin paint with an orange-peel texture, chipped to bare steel on edges.

    Returns (color, roughness, metallic, bump node).
    """
    peel = m.node('ShaderNodeTexNoise', Scale=900.0, Detail=1.0)
    m.link(tc.outputs['Object'], peel.inputs['Vector'])
    edge = m.math('MULTIPLY', convex_edges(m, tc, distance=0.0012, breakup_scale=140.0), chips)
    bare = ramp(m, edge, 0.45, 0.6)
    color = m.mix(bare, core.hex_linear(color_hex), core.hex_linear(chip_hex))
    roughness = mix_float(m, bare, m.math('ADD', rough, m.math('MULTIPLY', m.math('SUBTRACT', peel.outputs['Fac'], 0.5), 0.08)), 0.32)
    metallic = mix_float(m, bare, 0.0, 1.0)
    bump = m.node('ShaderNodeBump', Strength=0.08, Distance=0.00004)
    m.link(peel.outputs['Fac'], bump.inputs['Height'])
    return color, roughness, metallic, bump
