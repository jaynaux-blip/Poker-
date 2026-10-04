"""SM_Trophy and SM_ChipStand: mementos the apartment collects as the career goes.

- SM_Trophy: the Embercrest Card Room's Sunday tournament cup, a gold cup with loop handles on a stepped
  black base, an engraved brass plaque on the front.
- SM_ChipStand: a little walnut stand with a slot and a brass plate, for the $100 chip from Dee's back
  room (the game sets SM_Chip_100 into the slot, upright).

Coordinates (meters), front toward -Y, Z up: each origin on the surface under the piece's middle.
SM_ChipStand's slot runs along X, centered at SLOT (the chip stands in it, tilted back SLOT_TILT degrees).
"""
import math

import bpy  # noqa: F401,I001
from mathutils import Matrix, Vector

from artkit import core, parts
from artkit.sheet import Sheet
from artkit.shade import image_surface, object_coords, planar, ramp

MESHES = ['SM_Trophy', 'SM_ChipStand']
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {'trophy_plaque': 1024, 'stand_plate': 512}
AO_DISTANCE = 0.008
REVIEW_VIEWS = [('front', -20, 12, 0.7), ('side', 60, 20, 0.7), ('detail', -10, 5, 0.25, (0.0, -0.05, 0.03))]

PLAQUE = (0.074, 0.018)  # width, height
PLATE = (0.05, 0.011)
SLOT = Vector((0.0, 0.004, 0.018))
SLOT_TILT = 12.0


def plaque_sheet():
    w, h = PLAQUE[0] * 1000, PLAQUE[1] * 1000
    s = Sheet(w, h)
    s.rect(0, 0, w, h, 0xb08d4a, metal=1.0, rough=0.25)
    s.rect(1.0, 1.0, w - 2.0, 0.35, 0x5a4422, metal=0.6, rough=0.5)
    s.rect(1.0, h - 1.35, w - 2.0, 0.35, 0x5a4422, metal=0.6, rough=0.5)
    s.text('EMBERCREST CARD ROOM', w / 2, h * 0.5, 4.4, 0x3b2c15, face='Black', align='CENTER', tracking=1.2, metal=0.4, rough=0.6)
    s.text('SUNDAY TOURNAMENT  ·  CHAMPION', w / 2, h * 0.2, 2.4, 0x3b2c15, face='Bold', align='CENTER', tracking=1.3, metal=0.4, rough=0.6)
    return s.render('trophy_plaque', 1024)


def plate_sheet():
    w, h = PLATE[0] * 1000, PLATE[1] * 1000
    s = Sheet(w, h)
    s.rect(0, 0, w, h, 0xb08d4a, metal=1.0, rough=0.25)
    s.text("DEE'S GAME", w / 2, h * 0.46, 3.6, 0x3b2c15, face='Black', align='CENTER', tracking=1.2, metal=0.4, rough=0.6)
    s.text('THE BACK ROOM', w / 2, h * 0.14, 2.0, 0x3b2c15, face='Bold', align='CENTER', tracking=1.4, metal=0.4, rough=0.6)
    return s.render('stand_plate', 512)


def engraved(name, printed, x0, x1, z0, z1):
    """Brass with an engraved print, mapped onto the XZ plane (a plate facing -Y)."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    uv = planar(m, sep, 'X', 'Z', x0, x1, z0, z1)
    ink, alpha, metal, rough = image_surface(m, printed, uv, extension='EXTEND')
    m.set('Base Color', ink)
    m.set('Metallic', metal)
    m.set('Roughness', rough)
    bump = m.node('ShaderNodeBump', Strength=0.3, Distance=0.0002)
    m.link(m.math('SUBTRACT', 1.0, metal), bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def gold_material():
    m = core.Mat('trophy_gold')
    tc, sep = object_coords(m)
    from artkit.shade import noise
    smudge = noise(m, tc.outputs['Object'], scale=300.0, detail=3.0)
    m.set('Base Color', m.mix(m.math('MULTIPLY', smudge, 0.25), core.hex_linear(0xe3b55a), core.hex_linear(0xb9893a)))
    m.set('Metallic', 1.0)
    m.set('Roughness', m.math('ADD', 0.12, m.math('MULTIPLY', ramp(m, smudge, 0.55, 0.75), 0.18)))
    return m


def lacquer_material():
    return parts.gloss('trophy_base', 0x0c0c0d, rough=0.1, coat=0.6)


def walnut_material():
    m = core.Mat('stand_walnut')
    tc, sep = object_coords(m)
    from artkit.shade import noise, scaled
    grain = noise(m, scaled(m, sep, 900.0, 60.0, 60.0), detail=6.0, distortion=1.2)
    m.set('Base Color', m.mix(grain, core.hex_linear(0x3a2414), core.hex_linear(0x5c3b22)))
    m.set('Roughness', m.math('ADD', 0.35, m.math('MULTIPLY', grain, 0.15)))
    m.set('Coat Weight', 0.3)
    return m


def trophy(printed):
    gold, lacquer = gold_material(), lacquer_material()
    plaque_m = engraved('trophy_plaque', printed, -PLAQUE[0] / 2, PLAQUE[0] / 2, 0.006, 0.006 + PLAQUE[1])
    out = [(parts.rbox('base_lo', (0, 0, 0.016), (0.11, 0.11, 0.032), 0.003, 3), lacquer),
           (parts.rbox('base_hi', (0, 0, 0.032 + 0.017), (0.084, 0.084, 0.034), 0.003, 3), lacquer),
           (parts.rbox('plaque', (0, -0.0555, 0.006 + PLAQUE[1] / 2), (PLAQUE[0], 0.0012, PLAQUE[1]), 0.0006), plaque_m)]
    z0 = 0.066
    prof = [(0.0, z0), (0.03, z0), (0.031, z0 + 0.003), (0.024, z0 + 0.008), (0.012, z0 + 0.016), (0.0085, z0 + 0.03), (0.0085, z0 + 0.044),
            (0.013, z0 + 0.05), (0.0085, z0 + 0.056), (0.011, z0 + 0.062), (0.02, z0 + 0.07), (0.034, z0 + 0.086), (0.046, z0 + 0.11),
            (0.052, z0 + 0.135), (0.0545, z0 + 0.152), (0.056, z0 + 0.155), (0.0525, z0 + 0.155), (0.049, z0 + 0.138), (0.042, z0 + 0.11),
            (0.0, z0 + 0.1)]
    cup = core.lathe('cup', prof, segments=96)
    core.orient_normals(cup)
    out.append((cup, gold))
    for s in (-1, 1):
        loop = [(s * 0.04, 0.0, z0 + 0.125), (s * 0.068, 0.0, z0 + 0.13), (s * 0.08, 0.0, z0 + 0.11), (s * 0.07, 0.0, z0 + 0.085), (s * 0.034, 0.0, z0 + 0.082)]
        handle = core.sweep('handle', core.catmull_rom(loop, 8), core.rounded_rect(-0.0035, -0.0055, 0.0035, 0.0055, 0.003, steps=3))
        out.append((handle, gold))
    # A spade on the cup's front: two lobes and a point, embossed.
    sp = Vector((0.0, -0.0495, z0 + 0.12))
    for dx in (-0.007, 0.007):
        out.append((parts.disc('lobe', sp + Vector((dx, 0.0, -0.002)), (0, 1, 0), 0.0075, 0.004, 24), gold))
    tip = parts.outline_prism('tip', [(-0.0135, -0.001), (0.0135, -0.001), (0.0, 0.016)], [], -0.002, 0.002, Matrix.Translation(sp) @ parts.rot_x(90))
    stem = parts.outline_prism('stem', [(-0.004, -0.012), (0.004, -0.012), (0.0015, -0.002), (-0.0015, -0.002)], [], -0.002, 0.002, Matrix.Translation(sp) @ parts.rot_x(90))
    out += [(tip, gold), (stem, gold)]
    for o, m in out:
        core.assign(o, m)
    return parts.finish([o for o, _ in out], 'SM_Trophy', 40)


def chip_stand(printed):
    walnut = walnut_material()
    plate_m = engraved('stand_plate', printed, -PLATE[0] / 2, PLATE[0] / 2, 0.003, 0.003 + PLATE[1])
    felt = parts.fabric('stand_felt', 0x1d3b2a, period=0.0008)
    out = []
    # A block with a sloped front, a slot cut along X where the chip stands.
    block = core.prism_x('block', [(-0.03, 0.0), (0.025, 0.0), (0.025, 0.02), (-0.004, 0.024), (-0.03, 0.017)], -0.032, 0.032)
    core.orient_normals(block)
    out.append((block, walnut))
    slot = parts.rbox('slot', SLOT + Vector((0, 0, 0.002)), (0.044, 0.0042, 0.012), 0.0005, rot=parts.rot_x(-SLOT_TILT).to_3x3())  # leaning back
    core.boolean(block, slot)
    out.append((parts.rbox('felt', (0.0, 0.0, -0.0006), (0.062, 0.053, 0.0012), 0.0004), felt))
    # The plate on the block's upright front.
    plate = parts.rbox('plate', (0.0, -0.0305, 0.003 + PLATE[1] / 2), (PLATE[0], 0.001, PLATE[1]), 0.0004)
    out.append((plate, plate_m))
    for o, m in out:
        core.assign(o, m)
    stand = parts.finish([o for o, _ in out], 'SM_ChipStand', 40)
    return stand


def build():
    core.reset()
    plaque = plaque_sheet()
    plate = plate_sheet()
    core.reset()
    return [trophy(plaque), chip_stand(plate)]


def pose_for_review(objs):
    objs[1].location.x = 0.12
