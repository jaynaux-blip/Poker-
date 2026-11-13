"""SM_FS_PinBoard and SM_FS_Pin (Update 4 G1, F-look FL-8a): the cork pin board by the desk and the enamel festival pins
it collects (SPEC §2.6.4, §2.8.7, §9.2: the board <= 5k triangles with its pins).

- SM_FS_PinBoard: a cork board in a walnut frame, 0.62 x 0.46 m, 22 mm deep, the cork 8 mm below the frame's face. It
  hangs on the wall: the origin is the middle of its back face, the front toward -Y. PIN_SLOTS are forty pin origins on the
  cork (four rows of ten: a festival year a row, in the year's calendar order like the Career page's pin wall), each on
  the cork's face.
- SM_FS_Pin: a 24 mm enamel pin, its origin the middle of its back (it sits on the cork at a PIN_SLOTS point, facing -Y): a
  raised metal rim round a flat enamel field. The enamel is a neutral white and the rim a neutral metal; the game tints
  both (the field in the festival's Accent, the rim by grade: gold, silver, or an outline), through FS_Tint (vertex colour,
  COLOR_0): white with alpha 1 on the enamel, 0 on the metal. UV1 ("Decal", TEXCOORD_1) maps the enamel field onto 0..1
  (an upright image in Unreal) for the festival's glyph drawn by the game's code (no logo, text or font here); every other
  face sits outside 0..1 (fest_shelf.fs_decal_uv).

Coordinates in meters, Z up.
"""
import bpy  # noqa: F401,I001
from mathutils import Vector

from artkit import core, parts
from assets import fest_shelf as kit

BOARD_W, BOARD_H, BOARD_D = 0.62, 0.46, 0.022
FRAME_W = 0.028
CORK_Y = -0.014                       # the cork's face
PIN_R = 0.012
PIN_SEGMENTS = 12
PIN_SLOTS = [(-0.2475 + 0.055 * i, CORK_Y, 0.141 - 0.094 * j) for j in range(4) for i in range(10)]


def board():
    mat = kit.fs_material('fs_pinboard', [kit.fs_cork(), kit.fs_walnut('X'), kit.fs_walnut('Z')])
    W, H, D, F = BOARD_W, BOARD_H, BOARD_D, FRAME_W
    out = [kit.fs_tag(parts.rbox('cork', (0.0, CORK_Y / 2, 0.0), (W - 2 * F + 0.004, -CORK_Y, H - 2 * F + 0.004), 0.0), 0)]
    for s in (-1, 1):
        out.append(kit.fs_tag(parts.rbox('rail', (0.0, -D / 2, s * (H / 2 - F / 2)), (W, D, F), 0.003, 2), 1))
        out.append(kit.fs_tag(parts.rbox('stile', (s * (W / 2 - F / 2), -D / 2, 0.0), (F, D, H - 2 * F), 0.003, 2), 2))
    return kit.fs_finish(out, 'SM_FS_PinBoard', mat)


def pin():
    mat = kit.fs_material('fs_pin', [kit.fs_flat(0xf2f2f2, 0.07), kit.fs_metal('neutral', 0.14)])
    prof = [(PIN_R, 0.0), (PIN_R, 0.0028), (0.0115, 0.0034), (0.0095, 0.0034), (0.0092, 0.0026), (0.0, 0.0026)]
    body = kit.fs_lathe('pin', prof, parts.rot_x(90), segments=PIN_SEGMENTS, toward=(0.0, 0.0, 1.0))   # its face toward -Y
    kit.fs_tag(body, 1, tint=0.0)

    def enamel(p, me):
        return p.normal.y < -0.99 and Vector((p.center.x, p.center.z)).length < 0.0093
    kit.fs_tag_faces(body, enamel, 0, tint=1.0)
    obj = kit.fs_finish([body], 'SM_FS_Pin', mat, angle=50.0, tint=True)
    kit.fs_decal_uv(obj, enamel)
    return obj


MESHES = ['SM_FS_PinBoard', 'SM_FS_Pin']
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {'fs_pinboard': 2048, 'fs_pin': 256}
AO_DISTANCE = 0.01
GLTF_OPTIONS = {'export_vertex_color': 'ACTIVE'}
REVIEW_VIEWS = [('front', -12, 6, 2.2), ('side', 40, 18, 2.2), ('detail', -15, 10, 0.3, (-0.165, CORK_Y, 0.141))]
# Review only: the ten festivals' pins on the first row (enamel Accent, rim by grade), four more on the second (SPEC §2.7.2).
REVIEW_PINS = [(0xd2141a, 0xc9d3e6), (0xe0566e, 0x6a6a6e), (0x21d958, 0xf2c14e), (0xd2141a, 0xd9b25f), (0x7b96ff, 0xf2c14e),
               (0xd2141a, 0xe8b4a0), (0xff8a3d, 0xc9d3e6), (0xf2a93b, 0xc9d3e6), (0xa78bfa, 0xc9d3e6), (0xff4fa3, 0xf2c14e),
               (0xd2141a, 0xc9d3e6), (0x21d958, 0xc9d3e6), (0xa78bfa, 0xf2c14e), (0xff4fa3, 0x6a6a6e)]


def build():
    core.reset()
    kit.FS_CC0_USED.clear()
    return [board(), pin()]


def bake_parts(objs):
    kit.fs_prebake('fest_pinboard', MESHES, objs, TEXTURE_SIZE, TEXTURE_SIZES)
    return objs


def assemble(objs):
    kit.fs_postbake('fest_pinboard', MESHES, objs)
    return objs


def pose_for_review(objs):
    """The board with fourteen pins in their slots, each in a festival's tint."""
    by = {o.name: o for o in objs}
    first = by['SM_FS_Pin']
    for i, (enamel_hex, rim_hex) in enumerate(REVIEW_PINS):
        o = first if i == 0 else kit.fs_copy(first)
        o.location = PIN_SLOTS[i]
        kit.fs_review_tint(o, enamel_hex, rim_hex)
        if i:
            objs.append(o)
    bpy.context.view_layer.update()   # the review frames the board with its pins
