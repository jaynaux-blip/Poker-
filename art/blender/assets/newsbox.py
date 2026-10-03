"""SM_Newsbox: a coin-op newspaper box for the Riverside Ledger, on the curb by the corner store.

A sheet-steel box in the Ledger's blue on a square post with a base plate: a sloped lid, the door with its
window (the morning's front page behind the glass), a pull handle, the coin mechanism on top with its
slot, price window and return lever, the masthead across the top of the door. Chipped and streaked.

Coordinates (meters), front toward -Y (the door), Z up: origin on the sidewalk under the post.
"""
import math

import bpy  # noqa: F401,I001
from mathutils import Matrix, Vector

from artkit import core, parts, street
from artkit.shade import image_surface, object_coords, planar
from artkit.sheet import Sheet

MESHES = ['SM_Newsbox']
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {'news_page': 1024, 'news_header': 512, 'news_coin': 256, 'news_post': 512}
AO_DISTANCE = 0.03
REVIEW_VIEWS = [('front', -28, 12, 2.5), ('back', 140, 18, 2.5), ('detail', -12, 6, 0.95, (0.0, -0.2, 0.86))]

W, D = 0.44, 0.40
BOX0, BOX1 = 0.42, 1.02
WIN = (0.32, 0.30)  # window width, height
WIN_Z = 0.62
HEADER = (0.40, 0.07)
HEADER_Z = 0.935


def page_sheet():
    """The front page as seen through the window."""
    w, h = WIN[0] * 1000, WIN[1] * 1000
    s = Sheet(w, h)
    ink = 0x1b1b1d
    s.rect(0, 0, w, h, 0xe6e1d3, rough=0.85)
    s.text('THE RIVERSIDE LEDGER', w / 2, h - 30, 22, ink, face='Black', align='CENTER', tracking=1.05, rough=0.85)
    s.rect(10, h - 40, w - 20, 1.2, ink, rough=0.85)
    s.text('SATURDAY  ·  LATE CITY EDITION  ·  $1.50', w / 2, h - 50, 6.5, ink, face='Bold', align='CENTER', tracking=1.2, rough=0.85)
    s.rect(10, h - 56, w - 20, 0.8, ink, rough=0.85)
    s.text('CARD ROOMS PACKED', w / 2, h - 86, 24, ink, face='Black', align='CENTER', rough=0.85)
    s.text('AS SERIES NEARS', w / 2, h - 112, 24, ink, face='Black', align='CENTER', rough=0.85)
    # A photo and columns of body text.
    s.rect(12, 70, 150, 95, 0x58585a, rough=0.85)
    s.rect(18, 76, 138, 50, 0x8a8a88, rough=0.85)
    s.circle(60, 120, 16, 0x3a3a3c, rough=0.85)
    s.circle(115, 116, 13, 0x48484a, rough=0.85)
    s.text('Regulars line up outside the Riverside at dusk.', 87, 60, 5.0, ink, face='Regular', align='CENTER', rough=0.85)
    for col in range(2):
        x0 = 172 + col * 72
        for k in range(22):
            y = 165 - k * 7.2
            if y < 12:
                break
            wl = 64 if (k + col) % 7 != 6 else 38
            s.rect(x0, y, wl, 2.6, 0x56565a, rough=0.85)
    for k in range(6):
        s.rect(12, 46 - k * 7.0, 150 if k != 5 else 90, 2.6, 0x56565a, rough=0.85)
    s.text('RIVERSIDE REZONES FIFTH ST.', w / 2, 5, 7.5, ink, face='Bold', align='CENTER', rough=0.85)
    return s.render('news_page', 1024)


def header_sheet():
    w, h = HEADER[0] * 1000, HEADER[1] * 1000
    s = Sheet(w, h)
    s.rect(0, 0, w, h, 0xf2efe6, rough=0.4)
    s.text('The Riverside', w / 2, h * 0.5, 28, 0x13254a, face='Black', align='CENTER', rough=0.4)
    s.text('LEDGER', w / 2, h * 0.1, 19, 0xb5262f, face='Black', align='CENTER', tracking=1.6, rough=0.4)
    return s.render('news_header', 1024)


def coin_sheet():
    s = Sheet(100, 60)
    s.rect(0, 0, 100, 60, 0x101012, rough=0.3)
    s.text('$1.50', 50, 30, 22, 0xf2efe6, face='Black', align='CENTER', rough=0.3)
    s.text('QUARTERS · DOLLAR COINS', 50, 10, 6.5, 0xd8d2c0, face='Bold', align='CENTER', rough=0.3)
    return s.render('news_coin', 512)


def printed_face(name, printed, x0, x1, z0, z1, gloss=None):
    """A print on a face looking -Y, mapped in object XZ; gloss puts glass over it."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    uv = planar(m, sep, 'X', 'Z', x0, x1, z0, z1)
    ink, alpha, metal, rough = image_surface(m, printed, uv)
    if gloss is not None:
        # Through scratched glass: a touch darker, and the glass's sheen.
        m.set('Base Color', m.mix(0.18, ink, core.hex_linear(0x101418)))
        m.set('Roughness', gloss)
        m.set('Coat Weight', 1.0)
        m.set('Coat Roughness', 0.08)
    else:
        m.set('Base Color', ink)
        m.set('Roughness', rough)
    return m


def build():
    core.reset()
    body_m = street.painted('news_body', 0x1d3f7a, rough=0.38, chips=0.6, rust=0.35, grime=0.4, grime_height=0.7, streaks=0.5)
    post_m = street.painted('news_post', 0x2a2c2f, rough=0.55, chips=0.8, rust=0.7, grime=0.9, grime_height=0.5)
    trim_m = street.bare_metal('news_trim', 0xa8acb0, rough=0.32, grime=0.4)
    page_m = printed_face('news_page', page_sheet(), -WIN[0] / 2, WIN[0] / 2, WIN_Z - WIN[1] / 2, WIN_Z + WIN[1] / 2, gloss=0.08)
    header_m = printed_face('news_header', header_sheet(), -HEADER[0] / 2, HEADER[0] / 2, HEADER_Z - HEADER[1] / 2, HEADER_Z + HEADER[1] / 2)
    out = []
    # Post and base plate.
    out.append((parts.rbox('base', (0, 0, 0.006), (0.30, 0.30, 0.012), 0.004), post_m))
    out.append((parts.rbox('post', (0, 0, (0.012 + BOX0) / 2), (0.06, 0.06, BOX0 - 0.012), 0.004), post_m))
    for sx in (-1, 1):
        for sy in (-1, 1):
            out.append((parts.disc('anchor', (sx * 0.11, sy * 0.11, 0.016), (0, 0, 1), 0.012, 0.01, 6), trim_m))
    # The box: a body, a lid sloping down to the front, a lip under the door.
    out.append((parts.rbox('body', (0, 0, (BOX0 + BOX1 - 0.03) / 2), (W, D, BOX1 - BOX0 - 0.03), 0.012), body_m))
    lid = parts.rbox('lid', (0, 0, BOX1 - 0.02), (W + 0.02, D + 0.02, 0.03), 0.008, rot=Matrix.Rotation(math.radians(5), 3, 'X'))
    out.append((lid, body_m))
    # Door: a raised frame round the window, the window, the masthead, a handle.
    fy = -D / 2 - 0.006
    frame_w, frame_h = WIN[0] + 0.05, WIN[1] + 0.05
    for (cx, cz, sx, sz) in ((0, WIN_Z + frame_h / 2 - 0.0125, frame_w, 0.025), (0, WIN_Z - frame_h / 2 + 0.0125, frame_w, 0.025),
                             (-frame_w / 2 + 0.0125, WIN_Z, 0.025, frame_h), (frame_w / 2 - 0.0125, WIN_Z, 0.025, frame_h)):
        out.append((parts.rbox('frame', (cx, fy, cz), (sx, 0.012, sz), 0.004), body_m))
    out.append((parts.rbox('window', (0, fy + 0.003, WIN_Z), (WIN[0], 0.004, WIN[1]), 0.0), page_m))
    out.append((parts.rbox('header', (0, -D / 2 - 0.002, HEADER_Z), (HEADER[0], 0.004, HEADER[1]), 0.002), header_m))
    out.append((parts.rbox('door_lip', (0, -D / 2 - 0.004, BOX0 + 0.04), (W - 0.04, 0.008, 0.03), 0.003), body_m))
    # The pull handle across the top of the door, under the masthead.
    handle_z = WIN_Z + frame_h / 2 + 0.035
    out.append((parts.rbox('handle', (0, fy - 0.022, handle_z), (0.12, 0.012, 0.02), 0.005), trim_m))
    for sx in (-1, 1):
        out.append((parts.rbox('handle_leg', (sx * 0.05, fy - 0.011, handle_z), (0.012, 0.02, 0.016), 0.004), trim_m))
    # The coin mechanism on the right of the lid: a steel housing, the price window, a slot and a return lever.
    coin_m = printed_face('news_coin', coin_sheet(), 0.1 - 0.05, 0.1 + 0.05, BOX1 + 0.035 - 0.03, BOX1 + 0.035 + 0.03, gloss=0.1)
    out.append((parts.rbox('coin_box', (0.1, -0.06, BOX1 + 0.04), (0.14, 0.13, 0.09), 0.01), trim_m))
    out.append((parts.rbox('coin_window', (0.1, -0.126, BOX1 + 0.035), (0.1, 0.004, 0.06), 0.002), coin_m))
    out.append((parts.rbox('coin_slot', (0.1, -0.05, BOX1 + 0.086), (0.03, 0.004, 0.003), 0.001), post_m))
    out.append((parts.rod('coin_lever', (0.172, -0.08, BOX1 + 0.05), (0.2, -0.08, BOX1 + 0.05), 0.006, 12), trim_m))
    out.append((parts.disc('coin_knob', (0.205, -0.08, BOX1 + 0.05), (1, 0, 0), 0.012, 0.012, 16), post_m))
    for o, m in out:
        core.assign(o, m)
    return [parts.finish([o for o, _ in out], 'SM_Newsbox', 40)]
