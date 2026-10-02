"""SM_Router: the NORTHLINE fiber router that comes with Fiber 500.

A low black box with a gloss top and vent ribs, four antenna paddles hinged along the back, a row of status
lights along the front lip (power, internet, Wi-Fi bands, the ports), rubber feet, and the fiber and power
cables plugged into an outlet beside it.

Coordinates (meters), front toward -Y, Z up: origin under the router's middle; its cables plug into an outlet on
the window recess's side wall, WALL_X to the right.
"""
import math

import bpy  # noqa: F401,I001
from mathutils import Matrix, Vector

from artkit import core, parts

MESHES = ['SM_Router']
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {'router_green': 64, 'router_white': 64, 'router_rubber': 128, 'router_cable': 256}
AO_DISTANCE = 0.01
REVIEW_VIEWS = [('front', -25, 18, 0.7), ('back', 150, 25, 0.7), ('detail', -10, 10, 0.3, (0.0, -0.06, 0.03))]

W, D, H = 0.21, 0.135, 0.032
FEET = 0.004
WALL_X = 0.16  # the window recess's right-hand wall, from the router's middle (as the stage sets it on the sill)


def build():
    core.reset()
    body_m = parts.plastic('router_body', 0x111214, rough=0.45, scuffs=0.25)
    top_m = parts.gloss('router_top', 0x0b0b0d, rough=0.07)
    green = parts.emissive('router_green', 0x34d399, 1.0, base_hex=0x0d2a1e)
    white = parts.emissive('router_white', 0xe0f2fe, 1.0, base_hex=0x2a2f33)
    rubber = parts.rubber('router_rubber')
    cable_m = parts.rubber('router_cable', 0x0e0e0e, rough=0.55)
    fiber_m = parts.plastic('router_fiber', 0xe5e1d8, rough=0.4, scuffs=0.0)
    out = []
    c = Vector((0.0, 0.0, FEET + H / 2))
    out.append((parts.rbox('body', c, (W, D, H), 0.009, 4), body_m))
    out.append((parts.rbox('top', c + Vector((0.0, 0.006, H / 2)), (W - 0.03, D - 0.04, 0.0016), 0.006, 3), top_m))
    for k in range(9):
        x = -0.05 + k * 0.0125
        out.append((parts.rbox('vent', (x, 0.035, FEET + H + 0.0009), (0.0035, 0.03, 0.0018), 0.0008), body_m))
    for sx in (-1, 1):
        for sy in (-1, 1):
            out.append((parts.rbox('foot', (sx * 0.085, sy * 0.05, FEET / 2), (0.016, 0.016, FEET), 0.003), rubber))
    # Status lights along the front lip.
    for k in range(7):
        x = -0.06 + k * 0.02
        mat = white if k in (0, 3) else green
        out.append((parts.rbox('led', (x, -D / 2 - 0.0004, FEET + H * 0.62), (0.006, 0.0012, 0.0022), 0.0006), mat))
    # Antennas: paddles on hinges along the back, splayed a little.
    for k, (x, lean) in enumerate(((-0.08, -10), (-0.027, -3), (0.027, 3), (0.08, 10))):
        hinge_c = Vector((x, D / 2 - 0.004, FEET + H - 0.004))
        hinge = parts.disc('hinge', hinge_c, (1, 0, 0), 0.007, 0.018, 20)
        paddle = parts.rbox('antenna', (0.0, 0.0, 0.085), (0.017, 0.008, 0.16), 0.004, 4)
        paddle.data.transform(Matrix.Translation(hinge_c) @ parts.rot_y(lean) @ parts.rot_x(-6))
        out += [(hinge, body_m), (paddle, body_m)]
    # Ports on the back and the cables: the fiber (pale, thin) and the power lead.
    out.append((parts.rbox('ports', (0.0, D / 2 + 0.0004, FEET + H * 0.45), (0.12, 0.0012, 0.012), 0.001), top_m))
    # Both run behind the router to the window recess's side wall (WALL_X to its right) and plug in there:
    # the window is only a few centimeters behind, so nothing may trail backward.
    fiber = parts.tube('fiber', [(-0.04, D / 2 + 0.002, FEET + H * 0.45), (-0.04, D / 2 + 0.014, FEET + H * 0.35), (-0.02, D / 2 + 0.02, 0.003),
                                 (0.06, D / 2 + 0.022, 0.002), (WALL_X - 0.03, D / 2 + 0.018, 0.004), (WALL_X - 0.012, D / 2 + 0.012, 0.03)], 0.0015, 8, 8)
    power = parts.tube('power', [(0.05, D / 2 + 0.002, FEET + H * 0.45), (0.05, D / 2 + 0.012, FEET + H * 0.3), (0.07, D / 2 + 0.01, 0.003),
                                 (WALL_X - 0.04, D / 2 + 0.006, 0.0025), (WALL_X - 0.016, D / 2 - 0.004, 0.012), (WALL_X - 0.012, D / 2 - 0.012, 0.02)], 0.0024, 8, 8)
    plate = parts.rbox('outlet', (WALL_X - 0.002, D / 2 + 0.002, 0.03), (0.004, 0.07, 0.05), 0.002)
    plugs = [parts.rbox('plug', (WALL_X - 0.009, D / 2 + 0.012, 0.03), (0.012, 0.012, 0.01), 0.002),
             parts.rbox('plug', (WALL_X - 0.01, D / 2 - 0.012, 0.021), (0.014, 0.016, 0.018), 0.003)]
    out += [(fiber, fiber_m), (power, cable_m), (plate, fiber_m)] + [(p, body_m) for p in plugs]
    for o, m in out:
        core.assign(o, m)
    return [parts.finish([o for o, _ in out], 'SM_Router', 40)]
