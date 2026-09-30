"""SM_EnergyCan: an opened 12 oz can of GRIND energy drink, the grinder's fuel.

Standard 211/202 can proportions (66.2 mm body, 122 mm tall). The label is laid out flat,
rendered, and wrapped around the body; the lid, rim and base are bare aluminum.
"""
import math

from artkit import core
from artkit.sheet import Sheet

NAME = 'SM_EnergyCan'
MESHES = [NAME]
BODY_R = 0.0331
LABEL_Z0 = 0.0135
LABEL_Z1 = 0.1058
LIME = 0xb8ff2e
INK = 0x0b120d


def label():
    """The wrap-around print, in millimeters: 208 wide (the circumference) by 92 tall."""
    w = 2 * math.pi * BODY_R * 1000
    h = (LABEL_Z1 - LABEL_Z0) * 1000
    s = Sheet(w, h)
    s.rect(0, 0, w, h, INK, metal=0.12, rough=0.33)
    # Diagonal hairlines give the dark field some texture under the varnish.
    for k in range(-20, 60):
        x = k * 4.2
        s.poly([(x, 0), (x + 0.5, 0), (x + 0.5 + h * 0.55, h), (x + h * 0.55, h)], 0x142219, metal=0.15, rough=0.3)
    # Lime bands top and bottom.
    s.rect(0, 1.2, w, 1.6, LIME, metal=0.3, rough=0.28)
    s.rect(0, h - 2.8, w, 1.6, LIME, metal=0.3, rough=0.28)
    # A lightning slash across the front.
    cx = 52.0
    s.poly([(cx - 30, 10), (cx + 4, 48), (cx - 4, 50), (cx + 34, 84), (cx + 22, 84), (cx - 12, 54), (cx - 3, 52), (cx - 38, 10)], 0x2c4a12, metal=0.25, rough=0.3)
    # The wordmark runs up the can.
    s.text('GRIND', cx + 13, 9, 30.5, LIME, face='Black', rot=math.pi / 2, tracking=1.02, metal=0.35, rough=0.25)
    s.text('ENERGY', cx + 19.5, 10, 6.2, 0xf2f5ee, face='Bold', rot=math.pi / 2, tracking=1.55, metal=0.0, rough=0.36)
    s.text('FOR THE LONG NIGHT', cx - 22.5, 10, 3.6, 0x9fb38a, face='Medium', rot=math.pi / 2, tracking=1.35, metal=0.2, rough=0.4)
    # Flavor and claims on the side.
    sx = 104.0
    s.rect(sx - 13, 60, 26, 9, LIME, metal=0.3, rough=0.28)
    s.text('ZERO SUGAR', sx, 63, 3.9, INK, face='Black', align='CENTER', tracking=1.2)
    s.text('CITRUS', sx, 49, 8.0, 0xf2f5ee, face='Black', align='CENTER', tracking=1.05)
    s.text('VOLT', sx, 40, 8.0, 0xf2f5ee, face='Black', align='CENTER', tracking=1.05)
    s.text('160 MG CAFFEINE', sx, 31, 3.2, 0x9fb38a, face='Bold', align='CENTER', tracking=1.3)
    s.text('12 FL OZ (355 mL)', sx, 16, 3.4, 0xf2f5ee, face='Bold', align='CENTER', tracking=1.2)
    # Nutrition panel and barcode on the back.
    px, py, pw, ph = 138.0, 12.0, 40.0, 68.0
    s.rect(px, py, pw, ph, 0xe9ede4, metal=0.0, rough=0.42)
    s.text('Nutrition Facts', px + 2, py + ph - 7, 4.6, 0x0b0b0b, face='Black')
    s.rect(px + 2, py + ph - 9.5, pw - 4, 0.9, 0x0b0b0b)
    rows = [('Serving size', '1 can'), ('Calories', '10'), ('Total Fat', '0g'), ('Sodium', '200mg'), ('Total Carb.', '3g'),
            ('Total Sugars', '0g'), ('Protein', '0g'), ('Niacin', '100%'), ('Vitamin B6', '250%'), ('Vitamin B12', '250%')]
    y = py + ph - 14
    for name, val in rows:
        s.text(name, px + 2, y, 2.6, 0x151515, face='Bold' if name in ('Calories', 'Total Carb.') else 'Regular')
        s.text(val, px + pw - 2, y, 2.6, 0x151515, face='Bold', align='RIGHT')
        s.rect(px + 2, y - 1.2, pw - 4, 0.25, 0x333333)
        y -= 4.3
    s.text('CAFFEINE: 160mg / can. Not for children,', px + 2, py + 6.5, 1.55, 0x333333, face='Regular')
    s.text('pregnant women or people sensitive to caffeine.', px + 2, py + 4.3, 1.55, 0x333333, face='Regular')
    bx = 184.0
    s.rect(bx, 20, 18, 12, 0xf4f4f0)
    import random
    rnd = random.Random(7)
    x = bx + 1.5
    while x < bx + 16.5:
        bw = rnd.choice((0.25, 0.25, 0.5, 0.75))
        s.rect(x, 23, bw, 8, 0x080808)
        x += bw + rnd.choice((0.25, 0.5, 0.5))
    s.text('0 12345 67890 5', bx + 9, 20.8, 1.7, 0x080808, face='Regular', align='CENTER')
    s.text('CA CASH REFUND  ME HI VT 5\xa2  MI 10\xa2', bx + 9, 44, 1.6, 0x9fb38a, face='Bold', align='CENTER')
    s.text('RECYCLE ME', bx + 9, 48, 2.2, 0x9fb38a, face='Black', align='CENTER', tracking=1.3)
    return s.render('can_label', 2048)


def profile():
    """Body profile (radius, z) in meters, bottom center to lid center."""
    pts = [
        (0.0, 0.0108), (0.0060, 0.0106), (0.0120, 0.0099), (0.0170, 0.0086), (0.0205, 0.0068), (0.0228, 0.0042), (0.0241, 0.0017),
        (0.0250, 0.0004), (0.0257, 0.0), (0.0266, 0.0002), (0.0278, 0.0012), (0.0296, 0.0032), (0.0314, 0.0062), (0.0326, 0.0096),
        (0.0331, 0.0130), (0.0331, 0.0400), (0.0331, 0.0800), (0.0331, 0.1058), (0.0328, 0.1085), (0.0318, 0.1118), (0.0302, 0.1146),
        (0.0288, 0.1168), (0.0280, 0.1185), (0.0279, 0.1200), (0.0280, 0.1210), (0.0278, 0.1219), (0.0273, 0.1224), (0.0267, 0.1222),
        (0.0263, 0.1212), (0.0262, 0.1198), (0.0260, 0.1190), (0.0256, 0.1187), (0.0252, 0.1190), (0.0249, 0.1198), (0.0244, 0.1203),
        (0.0180, 0.1205), (0.0100, 0.1206), (0.0, 0.1207),
    ]
    return pts


def tab_outline():
    """The stay-on tab, rivet at the origin, nose toward +X (over the opening)."""
    outer = core.rounded_profile(
        [(-0.0160, 0.0), (-0.0160, 0.0068), (0.0035, 0.0068), (0.0070, 0.0035), (0.0070, -0.0035), (0.0035, -0.0068), (-0.0160, -0.0068), (-0.0160, 0.0)],
        [0, 0.0048, 0.0012, 0.0018, 0.0018, 0.0012, 0.0048, 0], steps=8)[1:-1]
    finger = core.rounded_profile(
        [(-0.0138, 0.0), (-0.0138, 0.0046), (-0.0052, 0.0046), (-0.0045, 0.0), (-0.0052, -0.0046), (-0.0138, -0.0046), (-0.0138, 0.0)],
        [0, 0.0034, 0.0022, 0.0, 0.0022, 0.0034, 0], steps=8)[1:-1]
    rivet_hole = core.circle_points(0.0, 0.0, 0.0011, 24)
    return outer, [list(reversed(finger)), list(reversed(rivet_hole))]


def aluminum(m, rough=0.2):
    """Bare aluminum with faint machining streaks and wear."""
    tc = m.node('ShaderNodeTexCoord')
    noise = m.node('ShaderNodeTexNoise', Scale=320.0, Detail=4.0, Roughness=0.5)
    m.link(tc.outputs['Object'], noise.inputs['Vector'])
    streak = m.node('ShaderNodeTexNoise', Scale=40.0, Detail=2.0)
    m.link(tc.outputs['Object'], streak.inputs['Vector'])
    r = m.math('ADD', m.math('MULTIPLY', noise.outputs['Fac'], 0.04), rough - 0.02)
    r = m.math('ADD', r, m.math('MULTIPLY', streak.outputs['Fac'], 0.03))
    return r


def build():
    core.reset()
    color_path, surface_path = label()
    core.reset()

    body = core.lathe('can_body', profile(), segments=144)
    # The opening, punched through the lid in front of the rivet (+X), is a pocket into the dark can.
    opening = core.rounded_profile(
        [(0.0048, 0.0), (0.0048, 0.0058), (0.0120, 0.0078), (0.0205, 0.0052), (0.0205, -0.0052), (0.0120, -0.0078), (0.0048, -0.0058), (0.0048, 0.0)],
        [0, 0.002, 0.004, 0.003, 0.003, 0.004, 0.002, 0], steps=8)[1:-1]
    cutter = core.extrude_outline('cutter', opening, depth=0.11, z=0.02)
    inside = core.Mat('can_inside')
    inside.set('Base Color', core.hex_linear(0x1b1a18))
    inside.set('Roughness', 0.55)
    inside.set('Metallic', 0.6)
    core.assign(cutter, inside)
    core.smart_uv(cutter)

    can = core.Mat('can_body')
    tc = can.node('ShaderNodeTexCoord')
    sep = can.node('ShaderNodeSeparateXYZ')
    can.link(tc.outputs['Object'], sep.inputs['Vector'])
    ang = can.math('ARCTAN2', sep.outputs['Y'], sep.outputs['X'])
    u = can.math('ADD', can.math('DIVIDE', ang, 2 * math.pi), 0.5)
    v = can.math('DIVIDE', can.math('SUBTRACT', sep.outputs['Z'], LABEL_Z0), LABEL_Z1 - LABEL_Z0)
    uv = can.node('ShaderNodeCombineXYZ')
    can.link(u, uv.inputs['X'])
    can.link(v, uv.inputs['Y'])
    col = can.image(color_path, vector=uv.outputs['Vector'], extension='EXTEND')
    surf = can.image(surface_path, non_color=True, vector=uv.outputs['Vector'], extension='EXTEND')
    ssep = can.node('ShaderNodeSeparateColor')
    can.link(surf.outputs['Color'], ssep.inputs['Color'])
    in_label = can.math('MULTIPLY', can.math('GREATER_THAN', sep.outputs['Z'], LABEL_Z0 - 0.0002), can.math('LESS_THAN', sep.outputs['Z'], LABEL_Z1 + 0.0002))
    alu_rough = aluminum(can, 0.15)
    can.set('Base Color', can.mix(in_label, core.hex_linear(0xd9dadd), col.outputs['Color']))
    metal = can.node('ShaderNodeMix')
    metal.data_type = 'FLOAT'
    can.link(in_label, metal.inputs['Factor'])
    metal.inputs['A'].default_value = 1.0
    can.link(ssep.outputs['Red'], metal.inputs['B'])
    can.set('Metallic', metal.outputs['Result'])
    rough = can.node('ShaderNodeMix')
    rough.data_type = 'FLOAT'
    can.link(in_label, rough.inputs['Factor'])
    can.link(alu_rough, rough.inputs['A'])
    can.link(ssep.outputs['Green'], rough.inputs['B'])
    can.set('Roughness', rough.outputs['Result'])
    # Faint dents and handling marks.
    dn = can.node('ShaderNodeTexNoise', Scale=18.0, Detail=3.0)
    can.link(tc.outputs['Object'], dn.inputs['Vector'])
    bump = can.node('ShaderNodeBump', Strength=0.008, Distance=0.0001)
    can.link(dn.outputs['Fac'], bump.inputs['Height'])
    can.set('Normal', bump.outputs['Normal'])
    core.assign(body, can)

    select_body = body
    boolean = select_body.modifiers.new('opening', 'BOOLEAN')
    boolean.operation = 'DIFFERENCE'
    boolean.solver = 'EXACT'
    boolean.object = cutter
    boolean.material_mode = 'TRANSFER'
    core.select_only([body])
    import bpy
    bpy.ops.object.modifier_apply(modifier='opening')
    bpy.data.objects.remove(cutter)

    outer, holes = tab_outline()
    tab = core.extrude_outline('can_tab', outer, holes, depth=0.00045, z=0.12075, bevel=0.00012)
    tab.rotation_euler.y = math.radians(8)  # opened: the ring end lifts and the nose dips into the opening
    rivet = core.lathe('can_rivet', [(0.0, 0.1204), (0.0021, 0.1204), (0.0021, 0.12085), (0.0016, 0.1212), (0.0, 0.12135)], segments=48)
    tab_mat = core.Mat('can_tab')
    tab_mat.set('Base Color', core.hex_linear(0xc9cbd0))
    tab_mat.set('Metallic', 1.0)
    tab_mat.set('Roughness', aluminum(tab_mat, 0.28))
    core.assign(tab, tab_mat)
    core.assign(rivet, tab_mat)
    core.apply_transforms(tab)
    core.smart_uv(tab)
    core.smart_uv(rivet)
    small = core.join([tab, rivet], 'can_tab_parts')
    core.smart_uv(small, margin=0.03)

    obj = core.join([body, small], NAME)
    return [obj]
