"""SM_NS_*: Northside's car kit (world realism spec §4.3): a hatchback, a crossover, a cab, a delivery van and a box truck; each with
and its lamp glows, so it can drive.

CAR frame, as sedan.py: meters, Z up, the FRONT toward Blender -Y, the origin on the road under the middle of the car.

Each car is three meshes:
- SM_NS_<Car> (baked): body, glass and cabin, lamps, trim, the brake calipers. Slots in this order (sedan.py's order with the
  tire and rim moved to the wheel): 0 paint [tint] (baked light neutral; the Cab's is its own yellow with its livery, not
  tinted), 1 trim, 2 glass (opaque, the cabin baked in), 3 chrome, 4 headlamp, 5 amber, 6 red, 7 ns_plate [screen] (unbaked:
  each plate's face UV 0..1 as seen, (0, 0) bottom left; front and rear share it; a default plate is embedded), 8 brake.
- SM_NS_<Car>_Wheel (baked): one wheel, origin at its hub, its outer face toward +X. Slots: tire, rim, brake (the rotor). The
  game places four: at (+-TRACK, axle, WZ) below; on the -X side turned 180 deg about Z (a turn, not a mirror, so the
  sidewall's lettering still reads clockwise from outside).
- SM_NS_<Car>_GlowLamps (unbaked, one slot, the Signal grid's 4 x 2 cells): 0 head-low, 1 head-high, 2 tail, 3 brake,
  4 reverse, 5 turn-L, 6 turn-R (the car's left is Blender +X: it faces -Y), 7 dome (a film just outside the side glass and the
  windshield: the cabin lit from inside; on the Cab, the TAXI roof light). Each face fills its cell planar from outside.
  The film lies over the baked lenses and glass, so its material must add light and nothing else: an additive (or
  translucent, unlit) material whose off cells are invisible. M_NS_Glow as built today (ns_setup.py: opaque unlit, an off
  lamp drawn as hue x 0.004) would paint every car's tail lamps, head lamps and windows flat black by day; the cars' glow
  meshes want an additive parent (Mode 2's code, BLEND_ADDITIVE). The review renders them that way.

Wheel positions (axles y, track x, hub z), m:
- Hatchback: -1.30 / 1.25, 0.755, 0.312.   SUV: -1.42 / 1.36, 0.82, 0.37.   Cab: -1.42 / 1.38, 0.785, 0.326 (as SM_Sedan).
- Van: -1.86 / 1.80, 0.865, 0.345.

The cars (sizes L x W x H):
- SM_NS_Hatchback (4.20 x 1.76 x 1.49): a compact five-door: short sloping hood, a fast windshield, a long roof falling to a
  steep hatch with a spoiler lip over its glass, tall tail lamps up the D pillars, black B pillars, a third brake lamp. Steel
  wheels under 8-slot covers on 195/65R15.
- SM_NS_SUV (4.80 x 1.90 x 1.71): a mid-size crossover: high hood and belt, black cladding round the arches and along the
  sills, roof rails, a D pillar with a black applique, tail lamps wrapping onto the quarters; 6 split-spoke alloys on
  235/60R18.
- SM_NS_Cab (4.85 x 1.83 x 1.70 with its light): SM_Sedan's body in taxi yellow (0xf2c112), NORTHSIDE TAXI and the cab
  number 4T17 on the front doors, a checker band along the waist, the medallion number on the rear doors and the TAXI
  roof light (its lit faces are cell 7). Steel wheels with full covers.
- SM_NS_Van (5.90 x 2.02 x 2.55): a high-roof delivery van, white: a short sloping hood, a tall raked windshield, the cab's
  doors and windows, a sliding side door, twin rear doors with windows, a step bumper; lettered for a fictional plumber
  (KOWALCZYK PLUMBING & HEATING · since 1972 · 555-0186 · NO JOB TOO SMALL) on both sides and the rear doors.

- SM_NS_BoxTruck (P1; 7.50 x 2.34 x 3.32, 2.68 over the mirrors): a 16 ft box on a cutaway chassis, white: the van's front (its
  hood, windshield and cab doors; the cab's roof 2.25, its back wall right behind the doors) and an FRP box (floor 1.0, top
  3.32) on aluminum corner posts and rails with a rub rail, an eight-panel roll-up door in the rear frame, the lamps on the
  box (tail/stop, turn and reverse in a stack on each rear jamb's foot; three ID and two clearance lamps on the header; amber
  clearance at the front's top corners; side markers on the bottom rails), the floor's crossmembers, the frame rails, the
  rear axle on its springs, mud flaps, the battery box, an underride bumper with a step and the plate. Lettered NORTHSIDE
  PRODUCE SUPPLY (green and red, with a crate of produce) on both sides, on the door (HOW'S MY DRIVING? 555-0147, UNIT 12)
  and over the cab; UNIT 12 on the cab doors. Wheels: -2.66 / 1.94 (axles), hub z 0.362; the front SM_NS_BoxTruck_Wheel
  (a single, LT225/75R16 on an eight-hole steel disc) at x +-0.865; the rear is SM_NS_BoxTruck_WheelDual (an addition: the
  pair, 0.25 m apart, origin at the hub between them, the outer wheel's face +X) at x +-0.80, turned 180 deg on the -X
  side like the singles. Its _GlowLamps: the cab's head lamps and front markers, the box's tail lamps (cell 2 also lights
  the ID, clearance and red side lamps: they're running lamps), stop, turn, reverse, the dome on the cab's glass. Built in
  the van's frame (TruckBuild) and moved by TRUCK_DY = -0.8 after the bake.

Triangles (body / one wheel / glows; a car on its four wheels is body + 4 wheels): Hatchback 62.7k / 7.7k / 6.7k (94k),
SUV 71.2k / 7.5k / 7.9k (101k), Cab 66.9k / 7.0k / 2.7k (95k), Van 57.0k / 6.3k / 4.2k (82k). The bodies are lofted at twice
sedan.py's samples per section span and 3.5 cm stations (the van 3 cm), so the shoulders and arches stay round at 1-5 m.

NS_CARS_ONLY=hatchback,suv,cab,van,boxtruck builds a subset.
"""
import bisect
import math
import os
import random
import sys
import time
import traceback

import bmesh
import bpy  # noqa: F401
import numpy as np
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

from artkit import core, parts, review, street
from artkit.shade import image_surface, mix_float, noise, object_coords, ramp, scaled, smooth_less, vec
from artkit.sheet import Sheet

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sedan as SD  # noqa: E402
from ns_ground import H, compose_grid, set_attr, tri_count  # noqa: E402

TAG = 'ns_cars'
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {}
AO_DISTANCE = 0.15
REVIEW_VIEWS = None
REVIEW_SCREEN_LIGHT = False
_ONLY = [s.strip() for s in os.environ.get('NS_CARS_ONLY', '').split(',') if s.strip()]
UNBAKED_PREFIX = ('ns_plate', 'ns_glass')
GLOW_CELLS = {'head_low': 0, 'head_high': 1, 'tail': 2, 'brake': 3, 'reverse': 4, 'turn_l': 5, 'turn_r': 6, 'dome': 7}

pchip = SD.pchip
smoothstep = SD.smoothstep
hit = SD.hit
ribbon = SD.ribbon
strip = SD.strip
resample = SD.resample
closed_lathe = SD.closed_lathe
drop_faces = SD.drop_faces
LENS_STEPS = SD.LENS_STEPS


def log(*a):
    print(f'[{TAG}]', *a, flush=True)


def line_y(line, z):
    (y0, z0), (y1, z1) = line
    return y0 + (z - z0) * (y1 - y0) / (z1 - z0)


# ================================================================== the specs

class Spec:
    """A car's numbers. Defaults are SM_Sedan's (sedan.py)."""
    name = 'SM_NS_Sedanlike'
    tag = 'nsx'
    Y_F, Y_R = -2.425, 2.425
    AXLES = (-1.42, 1.38)
    TRACK = 0.785
    WZ = 0.326
    TIRE_R = 0.334
    TIRE_W = 0.225
    ARCH_R = 0.386
    TUMBLE = 0.43
    ZB, WS, ZS, WSH, ZT, WT, ZC = SD.ZB, SD.WS, SD.ZS, SD.WSH, SD.ZT, SD.WT, SD.ZC
    BELT, GH_XB, DRIP, TOP, BOW = SD.BELT, SD.GH_XB, SD.DRIP, SD.TOP, SD.BOW
    GH_Y0, GH_Y1 = SD.GH_Y0, SD.GH_Y1
    Y_HDR, Y_RR = SD.Y_HDR, SD.Y_RR
    SIDE = [(SD.SAIL, 'glass'), (SD.B_FRONT, 'black'), (SD.B_REAR, 'glass'), (SD.DIV_FRONT, 'black'), (SD.DIV_REAR, 'glass'), (SD.C_EDGE, 'paint')]
    SAIL_ZMAX = 1.13
    GRILLE = SD.GRILLE
    GRILLE_Z = 0.712
    GRILLE_BARS = (0.76, 0.712, 0.664)
    GRILLE_HW = 0.385
    INTAKE = SD.INTAKE
    INTAKE_SLATS = (0.327, 0.364)
    INTAKE_HW = 0.41
    FOG = SD.FOG
    FOG_R = SD.FOG_R
    PLATE_F = SD.PLATE_F
    PLATE_R = SD.PLATE_R
    HEAD_AXIS = SD.HEAD_AXIS
    HEAD = SD.HEAD
    HEAD_PJ = (50, 0.743, 0.053)      # projector (phi, z, bowl radius)
    HEAD_HB = (27, 0.733, 0.041)      # high beam bowl
    HEAD_AMBER = 100.5                # phi past which the lamp is the amber turn lamp
    HEAD_POS = 64.0
    HEAD_RANGE = (8.0, 114.0, 0.655, 0.825)
    TAIL_AXIS = SD.TAIL_AXIS
    TAIL = SD.TAIL
    TAIL_E = ((46, 0.905, 0.042), (72, 0.928, 0.036))
    TAIL_REV = 19.6
    TAIL_LOW = 0.72
    TAIL_SMOKE = 0.998
    TAIL_RANGE = (6.0, 106.0, 0.775, 1.035)
    MARK_F = (-2.215, 0.585, -2.125, 0.612)
    MARK_R = (2.14, 0.64, 2.235, 0.665)
    REFLECT = (0.63, 0.479, 0.075, 0.011)
    EXHAUST = (-0.52, 0.305)
    MIRROR = (-0.82, 1.02, -0.772, 1.060)   # (ray y, ray z, housing back y, housing z)
    HANDLES = (0.03, 0.83)
    TRIM_CHROME = True
    GARNISH_Z = 0.87
    EMBLEM_R_Z = 0.94
    SHUT_SIDE = [
        [(-0.938, 0.965), (-0.94, 0.80), (-0.946, 0.55), (-0.952, 0.268)],
        [(-0.952, 0.268), (0.924, 0.268)],
        [(0.246, 0.985), (0.246, 0.268)],
        'REAR_DOOR',
        [(-1.875, 0.758), (-1.86, 0.66), (-1.79, 0.535)],
        [(2.135, 0.885), (2.02, 0.70), (1.75, 0.53)],
    ]
    FUEL = (1.62, 0.745, 1.79, 0.865)
    HOOD_FRONT = [(-0.40, 0.80), (-0.2, 0.804), (0.0, 0.805), (0.2, 0.804), (0.40, 0.80)]
    HOOD_SIDES = [(0.70, None), (0.682, -1.5), (0.652, -1.95), (0.615, -2.15), (0.55, -2.245), (0.42, -2.29), (0.22, -2.312), (0.0, -2.318)]
    REAR_LID = [(-0.40, 0.99), (-0.405, 0.80), (0.405, 0.80), (0.40, 0.99), (0.42, 1.035)]
    TRUNK = True
    CABIN = dict(seat_f=-0.22, seat_r=0.66, floor=0.30, dash=-0.78, rear_shelf=1.38, roof=1.38, width=0.74, hatch=False)
    PAINT = 0xd8dadc
    PAINT2 = 0xe6e8ea
    WHEEL = 'twin5'
    TIRE_TEXT = ('NORTHLINE', 'TOURING A/S', '225/50R17  94V')
    PLATE_TEXT = 'NSD 0412'
    TEX = dict(paint=2048, trim=1024, glass=1024, chrome=512, head=(1024, 512), amber=128, red=(1024, 512), brake=512, tire=1024, rim=1024,
               wbrake=512)


# --- the hatchback
class Hatchback(Spec):
    name = 'SM_NS_Hatchback'
    tag = 'nch'
    Y_F, Y_R = -2.10, 2.10
    AXLES = (-1.30, 1.25)
    TRACK = 0.755
    WZ = 0.312
    TIRE_R = 0.318
    TIRE_W = 0.195
    ARCH_R = 0.362
    TUMBLE = 0.40
    ZB = pchip([(-2.10, 0.40), (-2.093, 0.33), (-2.075, 0.28), (-2.04, 0.25), (-1.95, 0.225), (-1.7, 0.205), (-1.0, 0.175), (0.6, 0.17),
                (1.2, 0.185), (1.65, 0.24), (1.95, 0.29), (2.06, 0.33), (2.095, 0.38), (2.10, 0.42)])
    WS = pchip([(-2.10, 0.36), (-2.096, 0.46), (-2.088, 0.55), (-2.075, 0.63), (-2.055, 0.70), (-2.025, 0.76), (-1.985, 0.80), (-1.92, 0.84),
                (-1.82, 0.865), (-1.65, 0.876), (-1.30, 0.88), (0.5, 0.882), (1.25, 0.882), (1.65, 0.876), (1.88, 0.86), (2.0, 0.835),
                (2.06, 0.79), (2.09, 0.72), (2.098, 0.62), (2.10, 0.50)])
    ZS = pchip([(-2.10, 0.58), (-2.07, 0.66), (-2.0, 0.72), (-1.85, 0.76), (-1.5, 0.79), (-0.9, 0.82), (0.4, 0.86), (1.4, 0.90), (1.85, 0.925),
                (2.02, 0.93), (2.08, 0.91), (2.10, 0.86)])
    WSH = pchip([(-2.10, 0.30), (-2.092, 0.47), (-2.078, 0.58), (-2.055, 0.67), (-2.02, 0.74), (-1.97, 0.79), (-1.9, 0.83), (-1.78, 0.855),
                 (-1.5, 0.868), (0.5, 0.872), (1.25, 0.872), (1.65, 0.866), (1.88, 0.85), (2.0, 0.82), (2.06, 0.77), (2.09, 0.70), (2.10, 0.55)])
    ZT = pchip([(-2.10, 0.64), (-2.085, 0.70), (-2.06, 0.75), (-2.02, 0.79), (-1.95, 0.82), (-1.8, 0.85), (-1.5, 0.90), (-1.2, 0.95), (-1.0, 0.985),
                (-0.92, 0.995), (0.0, 1.005), (1.0, 1.02), (1.6, 1.035), (1.85, 1.04), (1.97, 1.04), (2.04, 1.02), (2.08, 0.98), (2.10, 0.92)])
    WT = pchip([(-2.10, 0.26), (-2.09, 0.42), (-2.07, 0.53), (-2.03, 0.61), (-1.95, 0.67), (-1.8, 0.71), (-1.5, 0.735), (-1.0, 0.755), (-0.9, 0.765),
                (0.0, 0.79), (1.0, 0.79), (1.6, 0.775), (1.85, 0.76), (1.97, 0.73), (2.04, 0.68), (2.08, 0.6), (2.10, 0.48)])
    ZC = pchip([(-2.10, 0.66), (-2.085, 0.72), (-2.06, 0.775), (-2.02, 0.815), (-1.95, 0.845), (-1.8, 0.875), (-1.5, 0.925), (-1.2, 0.975),
                (-1.0, 1.0), (-0.92, 1.005), (0.0, 1.015), (1.0, 1.03), (1.6, 1.045), (1.85, 1.05), (1.97, 1.05), (2.04, 1.03), (2.08, 0.99),
                (2.10, 0.93)])
    BELT = pchip([(-0.92, 0.995), (0.0, 1.005), (0.8, 1.02), (1.5, 1.04), (1.86, 1.05)])
    GH_XB = pchip([(-0.92, 0.755), (-0.5, 0.775), (0.2, 0.785), (1.0, 0.775), (1.5, 0.755), (1.86, 0.70)])
    DRIP = pchip([(-0.92, 0.995), (-0.75, 1.07), (-0.45, 1.22), (-0.15, 1.38), (0.05, 1.43), (0.4, 1.445), (0.9, 1.44), (1.3, 1.42), (1.6, 1.36),
                  (1.78, 1.2), (1.86, 1.05)])
    TOP = pchip([(-0.92, 1.0), (-0.75, 1.12), (-0.45, 1.28), (-0.15, 1.42), (0.05, 1.465), (0.4, 1.485), (0.9, 1.485), (1.3, 1.475), (1.55, 1.455),
                 (1.62, 1.43), (1.70, 1.33), (1.78, 1.2), (1.86, 1.06)])
    BOW = pchip([(-0.92, -0.10), (-0.45, -0.08), (-0.1, -0.04), (0.4, 0.0), (1.2, 0.02), (1.62, 0.04), (1.86, 0.05)])
    GH_Y0, GH_Y1 = -0.92, 1.86
    Y_HDR, Y_RR = -0.08, 1.60
    SIDE = [(((-0.70, 0.995), (-0.80, 1.13)), 'glass'), (((0.12, 1.0), (0.09, 1.44)), 'black'), (((0.22, 1.0), (0.19, 1.445)), 'glass'),
            (((1.10, 1.02), (0.98, 1.43)), 'black'), (((1.16, 1.022), (1.04, 1.435)), 'glass'), (((1.56, 1.035), (1.36, 1.41)), 'paint')]
    # (a fixed quarter glass behind the rear door, 0.40 m at the belt: without it the D pillar ran 0.76 m in paint)
    SAIL_ZMAX = 1.13
    GRILLE = SD.rrect(0.0, 0.66, 0.33, 0.045, 0.02, taper=0.1)
    GRILLE_Z = 0.66
    GRILLE_BARS = (0.672, 0.648)
    GRILLE_HW = 0.33
    INTAKE = SD.rrect(0.0, 0.36, 0.42, 0.075, 0.035, taper=-0.08)
    INTAKE_SLATS = (0.33, 0.37, 0.41)
    INTAKE_HW = 0.40
    FOG = [(0.64, 0.355)]
    FOG_R = 0.036
    PLATE_F = (0.0, 0.50)
    PLATE_R = (0.0, 0.66)
    HEAD_AXIS = (0.32, -1.82, 0.66)
    HEAD = [(16, 0.685), (18, 0.735), (26, 0.76), (38, 0.775), (52, 0.785), (66, 0.79), (80, 0.79), (92, 0.785), (100, 0.775), (104, 0.76),
            (103, 0.735), (96, 0.71), (84, 0.695), (70, 0.685), (56, 0.678), (42, 0.672), (30, 0.668), (21, 0.672)]
    HEAD_PJ = (48, 0.73, 0.045)
    HEAD_HB = (27, 0.722, 0.036)
    HEAD_AMBER = 92.0
    HEAD_POS = 66.0
    HEAD_RANGE = (10.0, 110.0, 0.655, 0.80)
    TAIL_AXIS = (0.62, 1.82, 1.0)
    TAIL = [(52, 0.86), (56, 1.20), (64, 1.23), (74, 1.22), (82, 1.19), (86, 1.14), (88, 1.04), (87, 0.94), (84, 0.88), (76, 0.85), (64, 0.85)]
    TAIL_E = ((76, 1.06, 0.034), (74, 0.95, 0.03))
    TAIL_REV = 58.0
    TAIL_LOW = 0.885
    TAIL_SMOKE = 1.19
    TAIL_RANGE = (48.0, 92.0, 0.83, 1.25)
    MARK_F = (-1.93, 0.56, -1.85, 0.585)
    MARK_R = (1.88, 0.60, 1.96, 0.622)
    REFLECT = (0.62, 0.46, 0.06, 0.011)
    EXHAUST = (-0.5, 0.29)
    MIRROR = (-0.80, 1.04, -0.752, 1.075)
    HANDLES = (0.0, 0.92)
    TRIM_CHROME = False
    GARNISH_Z = None
    EMBLEM_R_Z = 0.98
    SHUT_SIDE = [
        [(-0.905, 0.99), (-0.91, 0.80), (-0.917, 0.55), (-0.922, 0.255)],
        [(-0.922, 0.255), (0.82, 0.255)],
        [(0.17, 1.0), (0.17, 0.255)],
        [(1.08, 1.02), (1.06, 0.77), (1.0, 0.62), (0.82, 0.255)],
        [(-1.79, 0.72), (-1.77, 0.62), (-1.70, 0.50)],
        [(1.98, 0.90), (1.9, 0.72), (1.66, 0.52)],
    ]
    FUEL = (1.42, 0.79, 1.58, 0.905)
    HOOD_FRONT = [(-0.40, 0.765), (-0.2, 0.768), (0.0, 0.77), (0.2, 0.768), (0.40, 0.765)]
    HOOD_SIDES = [(0.69, None), (0.67, -1.5), (0.64, -1.8), (0.58, -1.95), (0.45, -2.02), (0.22, -2.05), (0.0, -2.055)]
    REAR_LID = [(-0.52, 1.05), (-0.53, 0.82), (0.53, 0.82), (0.52, 1.05)]
    TRUNK = False
    CABIN = dict(seat_f=-0.18, seat_r=0.62, floor=0.28, dash=-0.74, rear_shelf=1.55, roof=1.40, width=0.72, hatch=True)
    WHEEL = 'cover8'
    TIRE_TEXT = ('NORTHLINE', 'ECO A/S', '195/65R15  91H')
    PLATE_TEXT = 'KVR 2219'


# --- the crossover
class SUV(Spec):
    name = 'SM_NS_SUV'
    tag = 'ncs'
    Y_F, Y_R = -2.40, 2.40
    AXLES = (-1.42, 1.36)
    TRACK = 0.82
    WZ = 0.37
    TIRE_R = 0.372
    TIRE_W = 0.235
    ARCH_R = 0.43
    TUMBLE = 0.36
    ZB = pchip([(-2.40, 0.48), (-2.39, 0.41), (-2.37, 0.36), (-2.33, 0.33), (-2.2, 0.31), (-1.9, 0.29), (-1.0, 0.25), (0.6, 0.245), (1.2, 0.26),
                (1.85, 0.31), (2.2, 0.34), (2.33, 0.37), (2.38, 0.41), (2.40, 0.47)])
    WS = pchip([(-2.40, 0.40), (-2.396, 0.52), (-2.388, 0.62), (-2.374, 0.71), (-2.35, 0.78), (-2.31, 0.84), (-2.25, 0.88), (-2.15, 0.91),
                (-1.95, 0.935), (-1.42, 0.945), (0.5, 0.948), (1.36, 0.948), (1.9, 0.94), (2.15, 0.92), (2.28, 0.89), (2.35, 0.84), (2.385, 0.76),
                (2.397, 0.66), (2.40, 0.54)])
    ZS = pchip([(-2.40, 0.70), (-2.36, 0.80), (-2.28, 0.86), (-2.1, 0.90), (-1.6, 0.93), (-0.9, 0.96), (0.4, 0.99), (1.4, 1.01), (2.0, 1.02),
                (2.28, 1.02), (2.36, 1.0), (2.40, 0.95)])
    WSH = pchip([(-2.40, 0.32), (-2.392, 0.52), (-2.378, 0.64), (-2.355, 0.73), (-2.32, 0.80), (-2.27, 0.85), (-2.2, 0.885), (-2.05, 0.91),
                 (-1.8, 0.925), (0.5, 0.932), (1.36, 0.932), (1.9, 0.925), (2.15, 0.905), (2.28, 0.875), (2.35, 0.83), (2.385, 0.75), (2.40, 0.60)])
    ZT = pchip([(-2.40, 0.78), (-2.385, 0.85), (-2.36, 0.91), (-2.32, 0.95), (-2.24, 0.98), (-2.0, 1.01), (-1.6, 1.05), (-1.25, 1.09),
                (-1.1, 1.11), (0.0, 1.125), (1.0, 1.14), (1.8, 1.15), (2.1, 1.15), (2.30, 1.14), (2.36, 1.10), (2.39, 1.04), (2.40, 0.98)])
    WT = pchip([(-2.40, 0.28), (-2.39, 0.46), (-2.37, 0.58), (-2.33, 0.67), (-2.25, 0.73), (-2.1, 0.77), (-1.8, 0.80), (-1.25, 0.82), (-1.1, 0.83),
                (0.0, 0.855), (1.0, 0.855), (1.8, 0.84), (2.1, 0.82), (2.30, 0.78), (2.36, 0.72), (2.39, 0.62), (2.40, 0.5)])
    ZC = pchip([(-2.40, 0.80), (-2.385, 0.87), (-2.36, 0.935), (-2.32, 0.975), (-2.24, 1.005), (-2.0, 1.035), (-1.6, 1.075), (-1.25, 1.115),
                (-1.1, 1.13), (0.0, 1.14), (1.0, 1.155), (1.8, 1.165), (2.1, 1.165), (2.30, 1.155), (2.36, 1.115), (2.39, 1.05), (2.40, 0.99)])
    BELT = pchip([(-1.1, 1.115), (0.0, 1.125), (1.0, 1.14), (1.8, 1.15), (2.17, 1.155)])
    GH_XB = pchip([(-1.1, 0.81), (-0.6, 0.83), (0.2, 0.84), (1.2, 0.835), (1.9, 0.81), (2.17, 0.76)])
    DRIP = pchip([(-1.1, 1.115), (-0.92, 1.20), (-0.65, 1.35), (-0.35, 1.53), (-0.15, 1.6), (0.3, 1.625), (1.0, 1.625), (1.6, 1.615), (1.95, 1.58),
                  (2.08, 1.45), (2.17, 1.156)])
    TOP = pchip([(-1.1, 1.12), (-0.92, 1.23), (-0.65, 1.39), (-0.35, 1.57), (-0.15, 1.64), (0.3, 1.665), (1.0, 1.668), (1.6, 1.66), (1.92, 1.645),
                 (2.0, 1.62), (2.08, 1.48), (2.17, 1.17)])
    BOW = pchip([(-1.1, -0.10), (-0.65, -0.08), (-0.25, -0.04), (0.4, 0.0), (1.6, 0.02), (2.0, 0.04), (2.17, 0.05)])
    GH_Y0, GH_Y1 = -1.10, 2.17
    Y_HDR, Y_RR = -0.18, 1.98
    SIDE = [(((-0.88, 1.12), (-0.98, 1.25)), 'glass'), (((0.10, 1.125), (0.07, 1.62)), 'black'), (((0.22, 1.13), (0.19, 1.625)), 'glass'),
            (((1.18, 1.14), (1.14, 1.62)), 'black'), (((1.30, 1.145), (1.27, 1.62)), 'glass'), (((1.92, 1.15), (1.86, 1.6)), 'paint')]
    SAIL_ZMAX = 1.25
    GRILLE = SD.rrect(0.0, 0.84, 0.46, 0.115, 0.03, taper=0.08)
    GRILLE_Z = 0.84
    GRILLE_BARS = (0.92, 0.88, 0.84, 0.80, 0.76)
    GRILLE_HW = 0.46
    INTAKE = SD.rrect(0.0, 0.46, 0.44, 0.06, 0.04, taper=-0.05)
    INTAKE_SLATS = (0.44, 0.48)
    INTAKE_HW = 0.43
    FOG = [(0.70, 0.47)]
    FOG_R = 0.042
    PLATE_F = (0.0, 0.62)
    PLATE_R = (0.0, 0.74)
    HEAD_AXIS = (0.32, -2.10, 0.86)
    HEAD = [(16, 0.90), (17, 0.95), (26, 0.975), (40, 0.99), (55, 0.995), (70, 0.992), (84, 0.985), (96, 0.975), (104, 0.96), (106, 0.94),
            (102, 0.92), (92, 0.905), (78, 0.893), (62, 0.886), (46, 0.882), (32, 0.882), (22, 0.887)]
    HEAD_PJ = (52, 0.94, 0.048)
    HEAD_HB = (30, 0.935, 0.038)
    HEAD_AMBER = 96.0
    HEAD_POS = 70.0
    HEAD_RANGE = (10.0, 112.0, 0.87, 1.01)
    TAIL_AXIS = (0.45, 2.05, 1.0)
    TAIL = [(30, 0.93), (30, 1.10), (44, 1.13), (60, 1.14), (76, 1.135), (90, 1.12), (100, 1.10), (104, 1.06), (102, 1.0), (94, 0.97), (78, 0.95),
            (60, 0.94), (44, 0.935)]
    TAIL_E = ((60, 1.04, 0.04), (88, 1.05, 0.034))
    TAIL_REV = 36.0
    TAIL_LOW = 0.96
    TAIL_SMOKE = 1.12
    TAIL_RANGE = (24.0, 110.0, 0.91, 1.16)
    MARK_F = (-2.2, 0.75, -2.11, 0.775)
    MARK_R = (2.16, 0.78, 2.25, 0.80)
    REFLECT = (0.68, 0.54, 0.07, 0.012)
    EXHAUST = (-0.56, 0.36)
    MIRROR = (-0.98, 1.18, -0.93, 1.23)
    HANDLES = (0.0, 1.0)
    TRIM_CHROME = True
    GARNISH_Z = 0.98
    EMBLEM_R_Z = 1.08
    SHUT_SIDE = [
        [(-1.07, 1.12), (-1.08, 0.90), (-1.09, 0.62), (-1.095, 0.335)],
        [(-1.095, 0.335), (0.95, 0.335)],
        [(0.16, 1.125), (0.16, 0.335)],
        [(1.22, 1.14), (1.2, 0.85), (1.1, 0.70), (0.95, 0.335)],
        [(-2.12, 0.92), (-2.1, 0.80), (-2.0, 0.64)],
        [(2.25, 1.02), (2.18, 0.84), (1.9, 0.62)],
    ]
    FUEL = (1.56, 0.92, 1.73, 1.04)
    HOOD_FRONT = [(-0.5, 0.995), (-0.25, 1.0), (0.0, 1.002), (0.25, 1.0), (0.5, 0.995)]
    HOOD_SIDES = [(0.76, None), (0.75, -1.6), (0.72, -2.1), (0.66, -2.24), (0.52, -2.32), (0.26, -2.36), (0.0, -2.365)]
    REAR_LID = [(-0.62, 1.16), (-0.63, 0.86), (0.63, 0.86), (0.62, 1.16)]
    TRUNK = False
    CABIN = dict(seat_f=-0.28, seat_r=0.72, floor=0.40, dash=-0.86, rear_shelf=1.75, roof=1.60, width=0.78, hatch=True)
    WHEEL = 'split6'
    TIRE_TEXT = ('NORTHLINE', 'TRAIL H/T', '235/60R18  103H')
    PLATE_TEXT = 'JMW 7731'
    ROOF_RAILS = True
    CLADDING = True       # black cladding round the arches and along the sills (the docstring's crossover)


class Cab(Spec):
    name = 'SM_NS_Cab'
    tag = 'ncb'
    PAINT = 0xf2c112
    PAINT2 = 0xf6c82a
    WHEEL = 'cover_full'
    TIRE_TEXT = ('NORTHLINE', 'FLEET A/S', '225/60R16  97S')
    PLATE_TEXT = 'T 4417 NS'
    TAXI = True


# --- the pickup (P1)
class Pickup(Spec):
    """A mid-size crew-cab pickup with a short bed: the car body lofted whole, the cab's greenhouse ending at its back
    window, the bed cut out of the loft behind it (pickup_bed_cut) with a black liner, rail caps, floor ribs; a chrome
    bumper at each end, the plate on the rear one, black flares (CLADDING)."""
    name = 'SM_NS_Pickup'
    tag = 'ncp'
    PICKUP = True
    Y_F, Y_R = -2.65, 2.65
    AXLES = (-1.66, 1.62)
    TRACK = 0.83
    WZ = 0.385
    TIRE_R = 0.395
    TIRE_W = 0.255
    ARCH_R = 0.455
    TUMBLE = 0.30
    ZB = pchip([(-2.65, 0.56), (-2.64, 0.50), (-2.61, 0.46), (-2.55, 0.43), (-2.4, 0.41), (-2.0, 0.40), (-1.0, 0.38), (1.0, 0.38), (1.9, 0.43),
                (2.3, 0.50), (2.55, 0.56), (2.65, 0.60)])
    WS = pchip([(-2.65, 0.80), (-2.648, 0.85), (-2.643, 0.885), (-2.632, 0.91), (-2.61, 0.93), (-2.57, 0.945), (-2.5, 0.955), (-2.3, 0.96),
                (-1.66, 0.962), (0.0, 0.962), (1.62, 0.962), (2.4, 0.962), (2.56, 0.958), (2.62, 0.95), (2.645, 0.935), (2.65, 0.915)])
    ZS = pchip([(-2.65, 0.90), (-2.6, 0.95), (-2.5, 0.98), (-2.2, 1.0), (-1.5, 1.01), (0.5, 1.03), (2.0, 1.05), (2.6, 1.05), (2.65, 1.04)])
    WSH = pchip([(-2.65, 0.76), (-2.646, 0.83), (-2.64, 0.865), (-2.628, 0.893), (-2.605, 0.915), (-2.56, 0.93), (-2.48, 0.94), (-2.2, 0.946),
                 (0.0, 0.947), (2.4, 0.947), (2.56, 0.943), (2.62, 0.935), (2.645, 0.92), (2.65, 0.90)])
    ZT = pchip([(-2.65, 1.02), (-2.64, 1.05), (-2.62, 1.075), (-2.58, 1.09), (-2.45, 1.10), (-2.0, 1.115), (-1.5, 1.13), (-1.15, 1.14), (0.0, 1.16),
                (0.66, 1.17), (1.0, 1.18), (2.0, 1.19), (2.6, 1.19), (2.64, 1.18), (2.65, 1.16)])
    WT = pchip([(-2.65, 0.74), (-2.64, 0.79), (-2.62, 0.82), (-2.58, 0.84), (-2.5, 0.85), (-2.2, 0.86), (-1.6, 0.87), (-1.15, 0.875), (0.0, 0.89),
                (0.7, 0.90), (2.4, 0.90), (2.58, 0.895), (2.63, 0.88), (2.65, 0.85)])
    ZC = pchip([(-2.65, 1.035), (-2.64, 1.065), (-2.62, 1.09), (-2.58, 1.11), (-2.45, 1.125), (-2.0, 1.145), (-1.5, 1.165), (-1.15, 1.175),
                (0.0, 1.19), (0.66, 1.20), (2.6, 1.21), (2.64, 1.20), (2.65, 1.18)])
    BELT = pchip([(-1.15, 1.145), (-0.5, 1.152), (0.0, 1.16), (0.62, 1.172)])
    GH_XB = pchip([(-1.15, 0.82), (-0.6, 0.845), (0.0, 0.86), (0.45, 0.86), (0.62, 0.85)])
    DRIP = pchip([(-1.15, 1.147), (-0.95, 1.27), (-0.7, 1.44), (-0.45, 1.61), (-0.25, 1.72), (0.0, 1.765), (0.3, 1.775), (0.48, 1.77), (0.55, 1.73),
                  (0.59, 1.55), (0.62, 1.174)])
    TOP = pchip([(-1.15, 1.15), (-0.95, 1.29), (-0.7, 1.47), (-0.45, 1.65), (-0.25, 1.765), (0.0, 1.81), (0.3, 1.82), (0.46, 1.815), (0.53, 1.79),
                 (0.58, 1.62), (0.61, 1.35), (0.62, 1.18)])
    BOW = pchip([(-1.15, -0.10), (-0.7, -0.08), (-0.3, -0.04), (0.2, 0.0), (0.5, 0.01), (0.62, 0.012)])
    GH_Y0, GH_Y1 = -1.15, 0.62
    Y_HDR, Y_RR = -0.27, 0.50
    SIDE = [(((-0.95, 1.15), (-1.04, 1.27)), 'glass'), (((-0.03, 1.16), (-0.06, 1.72)), 'black'), (((0.08, 1.165), (0.05, 1.735)), 'glass'),
            (((0.47, 1.17), (0.44, 1.75)), 'paint')]
    SAIL_ZMAX = 1.27
    GRILLE = SD.rrect(0.0, 0.88, 0.56, 0.155, 0.035)
    GRILLE_Z = 0.88
    GRILLE_BARS = (0.98, 0.93, 0.88, 0.83, 0.78)
    GRILLE_HW = 0.56
    INTAKE = SD.rrect(0.0, 0.665, 0.48, 0.045, 0.02)
    INTAKE_SLATS = (0.665,)
    INTAKE_HW = 0.46
    FOG = []
    FOG_R = 0.04
    PLATE_ON_BUMPER = (-2.772, 0.50)
    PLATE_F = (0.0, 0.50)
    PLATE_R = (0.0, 5.0)              # (no recess in the tailgate: the plate is on the rear bumper)
    # the head lamps from beside the grille (x 0.60) round the corner, wrapping 0.1 m onto the fender
    HEAD_AXIS = (0.55, -2.35, 0.95)
    HEAD = [(10, 0.925), (10, 1.035), (20, 1.05), (35, 1.058), (50, 1.055), (62, 1.045), (69, 1.02), (70, 0.98), (66, 0.94), (55, 0.92), (40, 0.912),
            (25, 0.912), (15, 0.915)]
    HEAD_PJ = (36, 0.985, 0.045)
    HEAD_HB = (20, 0.985, 0.036)
    HEAD_AMBER = 60.0
    HEAD_POS = 48.0
    HEAD_RANGE = (6.0, 74.0, 0.90, 1.07)
    # the tail lamps tall on the bed's corners, outboard of the tailgate, wrapping onto the bed's sides
    TAIL_AXIS = (0.70, 2.45, 1.0)
    TAIL = [(36, 0.80), (36, 1.14), (44, 1.16), (55, 1.158), (63, 1.13), (68, 1.08), (68, 0.86), (63, 0.80), (52, 0.785), (42, 0.785)]
    TAIL_E = ((48, 1.06, 0.032), (48, 0.93, 0.028))
    TAIL_REV = 40.0
    TAIL_LOW = 0.86
    TAIL_SMOKE = 1.25
    TAIL_RANGE = (30.0, 72.0, 0.77, 1.18)
    MARK_F = (-2.52, 0.95, -2.44, 0.975)
    MARK_R = (2.50, 0.95, 2.58, 0.975)
    REFLECT = (0.74, 0.70, 0.05, 0.012)
    EXHAUST = (-0.62, 0.40)
    MIRROR = (-1.0, 1.26, -0.95, 1.32)
    HANDLES = (-0.28, 0.40)
    TRIM_CHROME = False
    BLACK_MIRRORS = True
    CLADDING = True
    GARNISH_Z = None
    EMBLEM_R_Z = 1.02
    SHUT_SIDE = [
        [(-1.12, 1.14), (-1.13, 0.90), (-1.14, 0.62), (-1.145, 0.42)],      # the front door's front edge
        [(-1.145, 0.42), (0.60, 0.42)],                                        # the doors' feet
        [(-0.02, 1.16), (-0.02, 0.42)],                                        # front door / rear door
        [(0.60, 1.172), (0.60, 0.42)],                                         # the rear door's back edge
        [(0.692, 1.18), (0.692, 0.45)], [(0.698, 1.18), (0.698, 0.45)], [(0.704, 1.18), (0.704, 0.45)],   # the cab-to-bed gap (2 cm)
        [(0.71, 1.18), (0.71, 0.45)], [(0.716, 1.18), (0.716, 0.45)],
        [(-2.58, 1.07), (-2.57, 0.90), (-2.55, 0.70)],                        # the fender's front edge
    ]
    FUEL = (0.95, 0.88, 1.10, 1.02)
    HOOD_FRONT = [(-0.72, 1.06), (-0.36, 1.07), (0.0, 1.072), (0.36, 1.07), (0.72, 1.06)]
    HOOD_SIDES = [(0.80, None), (0.81, -1.6), (0.80, -2.2), (0.78, -2.5), (0.70, -2.6), (0.40, -2.635), (0.0, -2.64)]
    REAR_LID = [(-0.82, 1.18), (-0.83, 0.64), (0.83, 0.64), (0.82, 1.18)]
    TRUNK = False
    CABIN = dict(seat_f=-0.45, seat_r=0.28, floor=0.52, dash=-1.05, rear_shelf=0.52, roof=1.76, width=0.80, hatch=False)
    PAINT = 0xd8dadc
    PAINT2 = 0xe6e8ea
    WHEEL = 'split6'
    TIRE_TEXT = ('NORTHLINE', 'TRAIL A/T', '265/70R17  115T')
    PLATE_TEXT = 'PKT 5530'
    BED = (0.84, 0.76, 2.595, 0.92)   # the bed's inner half-width, its front and back (the tailgate's inside), its floor


def pickup_bed_cut(S, body, mats):
    """The bed: a box cut out of the loft behind the cab (the cut's faces black: the liner)."""
    hw, y0, y1, zf = S.BED
    cut = parts.rbox('bed_cut', (0.0, (y0 + y1) / 2, zf + 1.5), (2 * hw, y1 - y0, 3.0), 0.012, 2)
    core.assign(cut, mat_of(mats['trim']))
    core.boolean(body, cut)


def pickup_parts(S, bvh, mats):
    """The bed's rail caps and the tailgate's cap, the floor's ribs, tie-down cleats; the tailgate's handle and emblem; the
    chrome bumpers (a step bumper with the plate and its lamps at the back; a valance under the front one), the exhaust."""
    hw, y0, y1, zf = S.BED
    out = []
    chrome, blk = mats['chrome'], mats['trim']

    def rb(name, c, size, r, m, seg=2):
        out.append((parts.rbox(name, c, size, r, seg), m))
    for sx in (-1, 1):
        # the rail caps along the bed's sides (black plastic over the wall's top), stake pockets in them
        xa = hw - 0.004
        p, _ = hit(bvh, (sx * (hw + 0.03), (y0 + y1) / 2, 3.0), (0, 0, -1))
        zt = p.z if p is not None else S.ZT(1.6)
        rb('rail_cap', (sx * (xa + 0.035), (y0 + y1) / 2 + 0.01, zt + 0.006), (0.074, y1 - y0 + 0.07, 0.022), 0.008, blk)
        for yy in (y0 + 0.25, y1 - 0.25):
            rb('stake_pocket', (sx * (xa + 0.035), yy, zt + 0.0175), (0.045, 0.07, 0.002), 0.001, mats['brake'])
        # tie-down cleats on the inner walls
        for yy in (y0 + 0.12, y1 - 0.12):
            rb('cleat', (sx * (hw - 0.012), yy, zt - 0.09), (0.024, 0.09, 0.03), 0.008, chrome)
    rb('tailgate_cap', (0.0, S.Y_R - 0.026, 1.2), (2 * hw + 0.04, 0.06, 0.022), 0.008, blk)
    for k in range(7):
        rb('floor_rib', (-0.54 + 0.18 * k, (y0 + y1) / 2, zf + 0.006), (0.05, y1 - y0 - 0.04, 0.014), 0.005, blk)
    # the tailgate: its latch handle in a black bezel, the emblem
    rb('tg_bezel', (0.0, S.Y_R + 0.003, 1.11), (0.26, 0.012, 0.075), 0.012, blk)
    rb('tg_handle', (0.0, S.Y_R + 0.006, 1.105), (0.2, 0.012, 0.045), 0.01, blk)
    ring = closed_lathe('emblem', [(0.034, 0.0), (0.05, 0.0), (0.05, 0.005), (0.046, 0.009), (0.038, 0.009), (0.034, 0.005)],
                        Matrix.Translation((0, S.Y_R + 0.001, S.EMBLEM_R_Z)) @ Matrix.Rotation(math.radians(-90), 4, 'X'), 40)
    out.append((ring, chrome))
    # the rear step bumper: chrome, a black step pad on it, the plate in its middle with two lamps over it
    rb('rbumper', (0.0, S.Y_R + 0.06, 0.52), (1.86, 0.18, 0.20), 0.035, chrome, 3)
    rb('rbumper_step', (0.0, S.Y_R + 0.07, 0.623), (0.56, 0.12, 0.01), 0.004, blk)
    rb('rbumper_valance', (0.0, S.Y_R - 0.02, 0.40), (1.7, 0.12, 0.06), 0.02, blk)
    rb('plate_back', (0.0, S.Y_R + 0.151, 0.51), (0.31, 0.004, 0.16), 0.002, chrome)
    out.append((plate_quad('plate_r', (0.0, S.Y_R + 0.1535, 0.51), 0.30, 0.15, 1, mats['plate']), None))
    for sx in (-1, 1):
        rb('plate_lamp', (sx * 0.07, S.Y_R + 0.14, 0.6), (0.04, 0.02, 0.012), 0.004, mats['head'])
    # the front bumper: chrome, its black valance under it
    rb('fbumper', (0.0, S.Y_F - 0.02, 0.51), (1.92, 0.20, 0.22), 0.04, chrome, 3)
    rb('fvalance', (0.0, S.Y_F + 0.03, 0.38), (1.7, 0.16, 0.08), 0.02, blk)
    ex, ez = S.EXHAUST
    out.append((closed_lathe('exhaust', [(0.03, -0.12), (0.038, -0.12), (0.04, 0.0), (0.034, 0.004), (0.03, 0.0)],
                             Matrix.Translation((ex, S.Y_R - 0.08, ez)) @ Matrix.Rotation(math.radians(-90), 4, 'X'), 24), chrome))
    out.append((parts.disc('exhaust_in', (ex, S.Y_R - 0.1, ez), (0, 1, 0), 0.031, 0.01, 24), blk))
    return out


SPECS = {'hatchback': Hatchback, 'suv': SUV, 'cab': Cab, 'pickup': Pickup}


# ================================================================== materials (names per car)

def mname(S, k):
    return f'{S.tag}_{k}'


def tex(S, k, default=None):
    name = mname(S, k)
    TEXTURE_SIZES[name] = S.TEX.get(k, default or 1024)
    return name


def line_sheets(S):
    """The shut lines in four views (white on black), projected onto the paint as sedan.py does."""
    LW = SD.LINE_W

    def stroke(s, pts, scale, off, w=LW):
        q = [((a + off[0]) * scale, (b + off[1]) * scale) for a, b in pts]
        hw = w * scale / 2
        for (x0, y0), (x1, y1) in zip(q, q[1:]):
            L = math.hypot(x1 - x0, y1 - y0)
            if L < 1e-9:
                continue
            nx, ny = -(y1 - y0) / L * hw, (x1 - x0) / L * hw
            s.poly([(x0 + nx, y0 + ny), (x1 + nx, y1 + ny), (x1 - nx, y1 - ny), (x0 - nx, y0 - ny)], 0xffffff)
        for x, y in q:
            s.circle(x, y, hw, 0xffffff, n=12)

    def arc(cy, cz, r, a0, a1, n=24):
        return [(cy + r * math.cos(math.radians(a0 + (a1 - a0) * k / n)), cz + r * math.sin(math.radians(a0 + (a1 - a0) * k / n))) for k in range(n + 1)]

    paths = {}
    lines = []
    for ln in S.SHUT_SIDE:
        if ln == 'REAR_DOOR':
            lines.append([(1.296, 1.002), (1.283, 0.77)] + arc(S.AXLES[1], S.WZ, S.ARCH_R + 0.07, 102.7, 180.0) + [(0.924, 0.268)])
        else:
            lines.append(ln)
    for key, fuel in (('side_l', False), ('side_r', True)):
        s = Sheet(5000, 2600)
        s.rect(0, 0, 5000, 2600, 0x000000)
        for ln in lines:
            stroke(s, ln, 1000, (2.5, 0.0))
        if fuel:
            fd = core.rounded_rect(*S.FUEL, 0.022, 4)
            stroke(s, fd + [fd[0]], 1000, (2.5, 0.0))
        paths[key] = s.render(f'{S.tag}_lines_{key}', 4096)[0]
    s = Sheet(2000, 5000)
    s.rect(0, 0, 2000, 5000, 0x000000)

    def ws_base(x):
        return S.GH_Y0 + S.BOW(S.GH_Y0) * max(0.0, 1.0 - (x / S.GH_XB(S.GH_Y0)) ** 2)

    hx = S.HOOD_SIDES[0][0]
    hood_rear = [(x, ws_base(x) - 0.095) for x in [-hx + 2 * hx * k / 28 for k in range(29)]]
    stroke(s, hood_rear, 1000, (1.0, 2.5))
    for sx in (-1, 1):
        pts = [(sx * hx, ws_base(hx) - 0.095)] + [(sx * x, y) for x, y in S.HOOD_SIDES[1:]]
        stroke(s, pts, 1000, (1.0, 2.5))
    if S.TRUNK:
        def bl_base(x):
            return S.GH_Y1 + S.BOW(S.GH_Y1) * max(0.0, 1.0 - (x / S.GH_XB(S.GH_Y1)) ** 2)
        trunk = [(x, bl_base(x) + 0.05) for x in [-0.655 + 1.31 * k / 26 for k in range(27)]]
        stroke(s, trunk, 1000, (1.0, 2.5))
        for sx in (-1, 1):
            stroke(s, [(sx * 0.655, bl_base(0.655) + 0.05), (sx * 0.655, 2.0), (sx * 0.64, 2.25), (sx * 0.60, 2.36)], 1000, (1.0, 2.5))
    paths['top'] = s.render(f'{S.tag}_lines_top', 1640)[0]
    s = Sheet(2000, 2600)
    s.rect(0, 0, 2000, 2600, 0x000000)
    stroke(s, S.HOOD_FRONT, 1000, (1.0, 0.0))
    paths['front'] = s.render(f'{S.tag}_lines_front', 1640)[0]
    s = Sheet(2000, 2600)
    s.rect(0, 0, 2000, 2600, 0x000000)
    stroke(s, S.REAR_LID, 1000, (1.0, 0.0))
    paths['rear'] = s.render(f'{S.tag}_lines_rear', 1640)[0]
    return paths


def livery_sheets(S):
    """The Cab's livery in side view (y from -2.5, z from 0; 1 unit = 1 mm): NORTHSIDE TAXI and the cab number on the front
    doors, the checker band, the medallion number on the rear doors. Right side reads front to back, the left mirrored."""
    out = {}
    for key in ('r', 'l'):
        s = Sheet(5000, 2600)
        s.rect(0, 0, 5000, 2600, 0x000000)

        def X(y):
            return (y + 2.5) * 1000 if key == 'r' else (2.5 - y) * 1000
        # the checker band along the waist (two rows of 50 mm squares) from the front fender to the rear quarter
        z0 = 760
        y0, y1 = -1.95, 2.05
        n = int((y1 - y0) / 0.05)
        for k in range(n):
            for row in range(2):
                if (k + row) % 2 == 0:
                    ya, yb = y0 + 0.05 * k, y0 + 0.05 * (k + 1)
                    xa, xb = sorted((X(ya), X(yb)))
                    s.rect(xa, z0 + 50 * row, xb - xa, 50, 0xffffff)
        # front door: NORTHSIDE over a big TAXI (read from the far curb), a rule under it
        cx = X(-0.36)
        s.text('NORTHSIDE', cx, 625, 92, 0x808080, 'Black', 'CENTER', tracking=1.08)
        s.text('TAXI', cx, 445, 165, 0x808080, 'Black', 'CENTER', tracking=1.18)
        s.rect(cx - 330, 405, 660, 16, 0x808080)
        # rear door: the medallion number big, the fare line under it
        cx = X(0.62)
        s.text('4T17', cx, 520, 150, 0xffffff, 'Black', 'CENTER', tracking=1.05)
        s.text('RATES POSTED INSIDE  ·  555-0124', cx, 440, 40, 0x808080, 'Bold', 'CENTER', tracking=1.05)
        out[key] = s.render(f'{S.tag}_livery_{key}', 4096)[0]
    return out


def paint_material(S, lines, livery=None):
    """sedan.py's paint (the clear coat in its one layer, road film and spray, crevice grime, stone chips, the shut lines),
    its color from the spec; the Cab's livery from its side sheets (white = black ink, grey = black, see livery_sheets)."""
    m = core.Mat(tex(S, 'paint'))
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    nx, ny, nz = SD.normal_parts(m)
    flakes = noise(m, o, scale=900.0, detail=1.0)
    col = m.mix(m.math('MULTIPLY', flakes, 0.3), core.hex_linear(S.PAINT), core.hex_linear(S.PAINT2))
    u_side, v_side = m.math('DIVIDE', m.math('ADD', y, 2.5), 5.0), m.math('DIVIDE', z, 2.6)
    left = SD.sample_mask(m, lines['side_l'], u_side, v_side)
    right = SD.sample_mask(m, lines['side_r'], u_side, v_side)
    side = mix_float(m, m.math('GREATER_THAN', x, 0.0), right, left)
    side = m.math('MULTIPLY', side, ramp(m, m.math('ABSOLUTE', nx), 0.4, 0.62))
    top = m.math('MULTIPLY', SD.sample_mask(m, lines['top'], m.math('DIVIDE', m.math('ADD', x, 1.0), 2.0), m.math('DIVIDE', m.math('ADD', y, 2.5), 5.0)),
                 ramp(m, nz, 0.4, 0.62))
    ux = m.math('DIVIDE', m.math('ADD', x, 1.0), 2.0)
    front = m.math('MULTIPLY', SD.sample_mask(m, lines['front'], ux, v_side), ramp(m, m.math('MULTIPLY', ny, -1.0), 0.35, 0.6))
    rear = m.math('MULTIPLY', SD.sample_mask(m, lines['rear'], ux, v_side), ramp(m, ny, 0.35, 0.6))
    lines_m = m.math('MAXIMUM', m.math('MAXIMUM', side, top), m.math('MAXIMUM', front, rear))
    if livery is not None:
        u_r = u_side
        u_l = m.math('SUBTRACT', 1.0, u_side)
        lr = SD.sample_mask(m, livery['r'], u_r, v_side)
        ll = SD.sample_mask(m, livery['l'], u_l, v_side)
        liv = mix_float(m, m.math('GREATER_THAN', x, 0.0), ll, lr)
        liv = m.math('MULTIPLY', liv, ramp(m, m.math('ABSOLUTE', nx), 0.5, 0.7))
        ink = ramp(m, liv, 0.3, 0.45)
        col = m.mix(ink, col, core.hex_linear(0x101010))
    blot = noise(m, o, scale=7.0, detail=4.0)
    streak = noise(m, scaled(m, sep, 3.0, 9.0, 40.0), detail=3.0)
    film = m.math('MULTIPLY', ramp(m, z, 0.5, 0.17), ramp(m, blot, 0.3, 0.75))
    spray = None
    for ay in S.AXLES:
        d = m.math('SUBTRACT', y, ay)
        behind = m.math('MULTIPLY', ramp(m, d, 0.25, 0.42), ramp(m, d, 1.0, 0.55))
        sp = m.math('MULTIPLY', m.math('MULTIPLY', behind, ramp(m, z, S.WZ + 0.25, S.WZ - 0.13)), ramp(m, streak, 0.35, 0.7))
        spray = sp if spray is None else m.math('MAXIMUM', spray, sp)
    dirt = m.math('MINIMUM', m.math('ADD', m.math('MULTIPLY', film, 0.55), m.math('MULTIPLY', spray, 0.6)), 1.0)
    ao = m.node('ShaderNodeAmbientOcclusion', Distance=0.05)
    ao.samples = 16
    ao.only_local = True
    crevice = m.math('MULTIPLY', ramp(m, m.math('SUBTRACT', 1.0, ao.outputs['AO']), 0.2, 0.65), ramp(m, noise(m, o, scale=40.0, detail=3.0), 0.3, 0.65))
    vor = m.node('ShaderNodeTexVoronoi', Scale=70.0)
    m.link(o, vor.inputs['Vector'])
    pick = m.node('ShaderNodeSeparateColor')
    m.link(vor.outputs['Color'], pick.inputs['Color'])
    d_front = m.math('SUBTRACT', y, S.AXLES[0])
    zone = m.math('MAXIMUM', m.math('MULTIPLY', ramp(m, m.math('MULTIPLY', ny, -1.0), 0.45, 0.7), ramp(m, z, S.ZT(S.Y_F + 0.3) + 0.15, S.ZT(S.Y_F + 0.3))),
                  m.math('MULTIPLY', ramp(m, y, S.Y_F + 0.3, S.Y_F + 0.16), ramp(m, nz, 0.3, 0.55)))
    zone = m.math('MAXIMUM', zone, m.math('MULTIPLY', m.math('MULTIPLY', ramp(m, d_front, 0.36, 0.46), ramp(m, d_front, 1.0, 0.8)),
                                          m.math('MULTIPLY', ramp(m, z, 0.42, 0.3), ramp(m, m.math('ABSOLUTE', nx), 0.5, 0.7))))
    chips = m.math('MULTIPLY', m.math('MULTIPLY', smooth_less(m, vor.outputs['Distance'], 0.16, 0.04), ramp(m, pick.outputs['Red'], 0.9, 0.92)), zone)
    smudge = ramp(m, noise(m, o, scale=4.0, detail=5.0), 0.4, 0.75)
    col = m.mix(m.math('MULTIPLY', crevice, 0.5), col, core.hex_linear(0x4a443b))
    col = m.mix(m.math('MULTIPLY', dirt, 0.85), col, core.hex_linear(0x4d4840))
    col = m.mix(chips, col, core.hex_linear(0x5d5b57))
    col = m.mix(lines_m, col, core.hex_linear(0x0a0a0b))
    m.set('Base Color', col)
    rough = m.math('ADD', 0.07, m.math('ADD', m.math('MULTIPLY', flakes, 0.02), m.math('MULTIPLY', smudge, 0.05)))
    rough = mix_float(m, m.math('MULTIPLY', crevice, 0.7), rough, 0.45)
    rough = mix_float(m, dirt, rough, 0.6)
    rough = mix_float(m, chips, rough, 0.55)
    m.set('Roughness', rough)
    bare = m.math('MAXIMUM', m.math('MAXIMUM', dirt, lines_m), chips)
    m.set('Metallic', mix_float(m, bare, m.math('ADD', 0.2, m.math('MULTIPLY', flakes, 0.1)), 0.03))
    bump = m.node('ShaderNodeBump', Strength=0.7, Distance=0.002)
    m.link(m.math('SUBTRACT', m.math('SUBTRACT', 1.0, lines_m), m.math('MULTIPLY', chips, 0.35)), bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def lens_frame(S, bvh, axis, toward, phi, z, lift=0.0035):
    p, n = corner_spot(S, bvh, 1, *axis, toward)((phi, z))
    if p is None:
        return Vector((0, 0, -10)), Vector((1, 0, 0)), Vector((0, 0, 1))
    n = n.normalized()
    t1 = Vector((0.0, 0.0, 1.0)).cross(n).normalized()
    t2 = n.cross(t1).normalized()
    return p + n * lift, t1, t2


def corner_spot(S, bvh, sx, cx, cy, z0, toward):
    return SD.corner_spot(bvh, sx, cx, cy, z0, toward)


def headlamp_material(S, bvh):
    """A generic headlamp behind a clear lens: silver housing, a projector in its fluted bowl, a high-beam bowl, a pillowed
    position lamp, the amber turn lamp at the outer end, all falling into shadow toward the opening's edge."""
    m = core.Mat(tex(S, 'head'))
    sep, ax, P = SD.lens_point(m)
    y, z = sep.outputs['Y'], sep.outputs['Z']
    A = S.HEAD_AXIS
    phi = m.math('MULTIPLY', m.math('ARCTAN2', m.math('SUBTRACT', ax, A[0]), m.math('SUBTRACT', A[1], y)), 180.0 / math.pi)
    r1, th1 = SD.polar(m, P, lens_frame(S, bvh, A, -1, S.HEAD_HB[0], S.HEAD_HB[1]))
    r2, th2 = SD.polar(m, P, lens_frame(S, bvh, A, -1, S.HEAD_PJ[0], S.HEAD_PJ[1]))
    soft = 0.0008
    R1, R2 = S.HEAD_HB[2], S.HEAD_PJ[2]
    hb = smooth_less(m, r1, R1, soft)
    cap = smooth_less(m, r1, R1 * 0.27, soft)
    pj_bowl = smooth_less(m, r2, R2, soft)
    pj_shroud = smooth_less(m, r2, R2 * 0.74, soft)
    pj_ring = smooth_less(m, r2, R2 * 0.58, soft)
    pj_lens = smooth_less(m, r2, R2 * 0.46, soft)
    amber = ramp(m, phi, S.HEAD_AMBER, S.HEAD_AMBER + 1.5)
    pos = m.math('MULTIPLY', ramp(m, phi, S.HEAD_POS, S.HEAD_POS + 1.5), m.math('SUBTRACT', 1.0, amber))
    rear = m.math('GREATER_THAN', y, 0.0)
    fog0 = S.FOG[0] if S.FOG else (9.0, 9.0)
    fx = m.math('SUBTRACT', ax, fog0[0])
    fz = m.math('SUBTRACT', z, fog0[1])
    df = m.math('SQRT', m.math('ADD', m.math('MULTIPLY', fx, fx), m.math('MULTIPLY', fz, fz)))
    fog = m.math('MAXIMUM', m.math('MULTIPLY', ramp(m, z, fog0[1] + 0.08, fog0[1] + 0.06), ramp(m, df, 0.08, 0.07)), rear)
    col, met, rou = SD.layered(m, (0x8d9298, 0.45, 0.2), [
        (pos, 0xb9bec4, 0.8, 0.24), (amber, 0xc8761a, 0.55, 0.16), (pj_bowl, 0xd3d7db, 1.0, 0.07), (pj_shroud, 0x17181b, 0.0, 0.42),
        (pj_ring, 0xe2e5e8, 1.0, 0.1), (pj_lens, 0x0a0d12, 0.0, 0.03), (hb, 0xd3d7db, 1.0, 0.07), (cap, 0xc4c8cc, 1.0, 0.12),
        (fog, 0xc9cdd1, 0.9, 0.1), (rear, 0xd6d8da, 0.25, 0.22)])
    p0, p1, z0, z1 = S.HEAD_RANGE
    edge = SD.edge_depth(m, phi, z, S.HEAD, p0, p1, z0, z1, 0.45, f'T_{S.tag}_head_edge')
    shadow = m.math('MULTIPLY', m.math('SUBTRACT', 1.0, ramp(m, edge, 0.0, 0.55)), m.math('SUBTRACT', 1.0, fog))
    col = m.mix(m.math('MULTIPLY', shadow, 0.7), col, core.hex_linear(0x0d0e10))
    m.set('Base Color', col)
    m.set('Metallic', met)
    m.set('Roughness', rou)
    h = SD.sum_of(m, [
        SD.masked(m, m.math('MULTIPLY', hb, m.math('SUBTRACT', 1.0, cap)), m.math('ADD', SD.dish(m, r1, R1, 0.014), SD.flutes(m, r1, th1, R1, 10, 0.003))),
        SD.masked(m, cap, m.math('ADD', SD.dish(m, r1, R1, 0.014), SD.dish(m, r1, R1 * 0.27, -0.004))),
        SD.masked(m, m.math('SUBTRACT', pj_bowl, pj_shroud), m.math('ADD', SD.dish(m, r2, R2, 0.012), SD.flutes(m, r2, th2, R2, 12, 0.0025))),
        SD.masked(m, m.math('SUBTRACT', pj_ring, pj_lens), m.math('ADD', -0.004, SD.dish(m, r2, R2 * 0.58, -0.002))),
        SD.masked(m, pj_lens, m.math('ADD', -0.006, SD.dish(m, r2, R2 * 0.46, -0.007))),
        SD.masked(m, m.math('SUBTRACT', pj_shroud, pj_ring), -0.007),
        SD.masked(m, m.math('MAXIMUM', pos, amber), SD.pillows(m, P, 0.006, 0.0005)),
    ])
    h = m.math('MULTIPLY', h, m.math('SUBTRACT', 1.0, fog))
    h = m.math('ADD', h, SD.masked(m, m.math('SUBTRACT', fog, rear), m.math('ADD', SD.dish(m, df, 0.039, 0.008), SD.ripples(m, fx, 0.005, 0.0004))))
    SD.bumped(m, h)
    return m


def taillamp_material(S, bvh):
    m = core.Mat(tex(S, 'red'))
    sep, ax, P = SD.lens_point(m)
    y, z = sep.outputs['Y'], sep.outputs['Z']
    A = S.TAIL_AXIS
    phi = m.math('MULTIPLY', m.math('ARCTAN2', m.math('SUBTRACT', ax, A[0]), m.math('SUBTRACT', y, A[1])), 180.0 / math.pi)
    (pa, za, Ra), (pb, zb, Rb) = S.TAIL_E
    ra, tha = SD.polar(m, P, lens_frame(S, bvh, A, 1, pa, za))
    rb, thb = SD.polar(m, P, lens_frame(S, bvh, A, 1, pb, zb))
    soft = 0.0008
    e1, e2 = smooth_less(m, ra, Ra, soft), smooth_less(m, rb, Rb, soft)
    o1, o2 = smooth_less(m, ra, Ra * 1.33, soft), smooth_less(m, rb, Rb * 1.36, soft)
    c1, c2 = smooth_less(m, ra, Ra * 0.21, soft), smooth_less(m, rb, Rb * 0.22, soft)
    elem = m.math('MAXIMUM', e1, e2)
    bezel = m.math('MAXIMUM', m.math('SUBTRACT', o1, e1), m.math('SUBTRACT', o2, e2))
    caps = m.math('MAXIMUM', c1, c2)
    low = ramp(m, z, S.TAIL_LOW + 0.01, S.TAIL_LOW - 0.01)
    reverse = m.math('MULTIPLY', ramp(m, phi, S.TAIL_REV, S.TAIL_REV - 1.0), m.math('SUBTRACT', 1.0, low))
    smoke = m.math('MULTIPLY', ramp(m, z, S.TAIL_SMOKE, S.TAIL_SMOKE + 0.014), 0.75)
    col, met, rou = SD.layered(m, (0x7a070c, 0.35, 0.1), [
        (elem, 0xb3121a, 0.75, 0.1), (caps, 0xd2262c, 0.6, 0.14), (bezel, 0x2a0305, 0.1, 0.25), (reverse, 0xd3d6d9, 0.5, 0.1), (low, 0xa3121a, 0.4, 0.08)])
    col = m.mix(smoke, col, core.hex_linear(0x170203))
    p0, p1, z0, z1 = S.TAIL_RANGE
    edge = SD.edge_depth(m, phi, z, S.TAIL, p0, p1, z0, z1, 0.42, f'T_{S.tag}_red_edge')
    shadow = m.math('MULTIPLY', m.math('SUBTRACT', 1.0, ramp(m, edge, 0.0, 0.5)), m.math('SUBTRACT', 1.0, low))
    col = m.mix(m.math('MULTIPLY', shadow, 0.6), col, core.hex_linear(0x1a0204))
    m.set('Base Color', col)
    m.set('Metallic', m.math('MULTIPLY', met, m.math('SUBTRACT', 1.0, m.math('MULTIPLY', smoke, 0.6))))
    m.set('Roughness', rou)
    field = m.math('SUBTRACT', 1.0, m.math('MAXIMUM', m.math('MAXIMUM', o1, o2), m.math('MAXIMUM', reverse, low)))
    h = SD.sum_of(m, [
        SD.masked(m, e1, m.math('ADD', SD.dish(m, ra, Ra, 0.008), SD.ripples(m, ra, 0.0045, 0.0003))),
        SD.masked(m, e2, m.math('ADD', SD.dish(m, rb, Rb, 0.007), SD.ripples(m, rb, 0.0045, 0.0003))),
        SD.masked(m, c1, SD.dish(m, ra, Ra * 0.21, -0.003)), SD.masked(m, c2, SD.dish(m, rb, Rb * 0.22, -0.003)),
        SD.masked(m, bezel, 0.0015), SD.masked(m, field, SD.pillows(m, P, 0.009, 0.0005)),
        SD.masked(m, reverse, SD.ripples(m, z, 0.006, 0.0004)), SD.masked(m, low, SD.pillows(m, P, 0.0035, 0.00025))])
    SD.bumped(m, h)
    return m


def plate_image(S):
    s = Sheet(300, 150)
    s.rect(0, 0, 300, 150, 0xf2f1ec)
    s.rect(4, 4, 292, 142, 0x1e3a6e)
    s.rect(8, 8, 284, 134, 0xf2f1ec)
    s.text('NORTHSIDE', 150, 118, 22, 0xb5262f, face='Black', align='CENTER', tracking=1.3)
    s.text(S.PLATE_TEXT, 150, 40, 58 if len(S.PLATE_TEXT) <= 8 else 50, 0x1e3a6e, face='Black', align='CENTER')
    s.text('OPEN ALL NIGHT', 150, 14, 13, 0x1e3a6e, face='Bold', align='CENTER', tracking=1.3)
    for bx in (24, 276):
        s.circle(bx, 131, 5.5, 0xb4b7ba)
    s.rect(238, 11, 42, 21, 0xe0b422)
    return s.render(f'{S.tag}_plate', 1024, samples=8)[0]


def plate_slot(S):
    """The [screen] plate slot, shared by name across the cars (the game draws each plate; a default is embedded)."""
    m = bpy.data.materials.get('ns_plate')
    if m is not None:
        return m
    mm = core.Mat('ns_plate')
    uvn = mm.node('ShaderNodeUVMap')
    uvn.uv_map = 'UVMap'
    img = mm.image(plate_image(S), vector=uvn.outputs['UV'], extension='EXTEND')
    mm.link(img.outputs['Color'], mm.bsdf.inputs['Base Color'])
    mm.set('Roughness', 0.35)
    mm.set('Metallic', 0.3)
    return mm.m


def wheel_materials(S, text, dual_off=0.0):
    """tire, rim, rotor for a wheel built at its hub (the axle along X, its outer face +X). dual_off: a dual pair's tires sit
    at x +-dual_off (the lettering on the outer one's outer wall; the tread on both)."""
    def tire():
        m = core.Mat(tex(S, 'tire'))
        tc, sep = object_coords(m)
        o = tc.outputs['Object']
        x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
        xl = x
        if dual_off:
            x = m.math('SUBTRACT', m.math('ABSOLUTE', x), dual_off)     # across each tire's own tread
        r = m.math('SQRT', m.math('ADD', m.math('MULTIPLY', y, y), m.math('MULTIPLY', z, z)))
        ang = m.math('ARCTAN2', z, y)
        R = S.TIRE_R
        u = m.math('FRACT', m.math('MULTIPLY', ang, -2.0 / (2 * math.pi)))
        rim_r = R * 0.6
        v = m.math('DIVIDE', m.math('SUBTRACT', r, R * 0.763), R * 0.138)
        letters = m.math('MULTIPLY', SD.sample_mask(m, text, u, v), ramp(m, xl, dual_off + S.TIRE_W * 0.31, dual_off + S.TIRE_W * 0.4))
        tread = ramp(m, r, R * 0.964, R * 0.985)
        hw = S.TIRE_W / 2
        circ = None
        for c in (-0.55, -0.2, 0.2, 0.55):
            g = smooth_less(m, m.math('ABSOLUTE', m.math('SUBTRACT', x, c * hw)), 0.0045, 0.0012)
            circ = g if circ is None else m.math('MAXIMUM', circ, g)
        sipe = ramp(m, m.math('ABSOLUTE', m.math('SINE', m.math('ADD', m.math('MULTIPLY', ang, 30.0), m.math('MULTIPLY', x, 9.0)))), 0.93, 0.97)
        grooves = m.math('MULTIPLY', m.math('MAXIMUM', circ, m.math('MULTIPLY', sipe, ramp(m, m.math('ABSOLUTE', x), hw * 0.3, hw * 0.4))), tread)
        rib = m.math('MULTIPLY', smooth_less(m, m.math('ABSOLUTE', m.math('SUBTRACT', r, R * 0.707)), 0.0035, 0.001), ramp(m, x, hw * 0.53, hw * 0.71))
        dust = m.math('MULTIPLY', ramp(m, noise(m, o, scale=12.0, detail=4.0), 0.45, 0.85), ramp(m, z, 0.12, -R * 0.7))
        col = m.mix(m.math('MULTIPLY', dust, 0.35), core.hex_linear(0x121212), core.hex_linear(0x2e2a25))
        col = m.mix(letters, col, core.hex_linear(0x262626))
        col = m.mix(grooves, col, core.hex_linear(0x060606))
        m.set('Base Color', col)
        m.set('Roughness', mix_float(m, dust, mix_float(m, tread, 0.78, 0.66), 0.9))
        bump = m.node('ShaderNodeBump', Strength=0.5, Distance=0.002)
        h = m.math('ADD', m.math('ADD', m.math('MULTIPLY', letters, 0.6), m.math('MULTIPLY', rib, 0.5)), m.math('MULTIPLY', grooves, -1.0))
        m.link(m.math('ADD', h, m.math('MULTIPLY', noise(m, o, scale=300.0, detail=1.0), 0.05)), bump.inputs['Height'])
        m.set('Normal', bump.outputs['Normal'])
        del rim_r
        return m

    def rim():
        m = core.Mat(tex(S, 'rim'))
        tc, sep = object_coords(m)
        o = tc.outputs['Object']
        lx = sep.outputs['X']
        blot = noise(m, o, scale=28.0, detail=4.0)
        dust = m.math('MAXIMUM', m.math('MULTIPLY', ramp(m, lx, 0.07, 0.0), 0.75), m.math('MULTIPLY', ramp(m, blot, 0.55, 0.78), 0.45))
        flake = noise(m, o, scale=1200.0, detail=1.0)
        if S.WHEEL in ('cover8', 'cover_full'):
            # plastic wheel covers: silver-painted plastic, scuffed on the rim by curbs
            base, hi = 0xa9adb2, 0xbfc3c7
        elif S.WHEEL == 'steel':
            base, hi = 0x1b1c1e, 0x2a2b2e
        elif S.WHEEL == 'truck':
            base, hi = 0x9a9ea2, 0xaeb2b6          # silver-painted steel, brake dust in the dish
        else:
            base, hi = 0xb4b8bd, 0xc8ccd0
        col = m.mix(m.math('MULTIPLY', flake, 0.25), core.hex_linear(base), core.hex_linear(hi))
        scuff = m.math('MULTIPLY', ramp(m, m.math('SQRT', m.math('ADD', m.math('MULTIPLY', sep.outputs['Y'], sep.outputs['Y']),
                                                                    m.math('MULTIPLY', sep.outputs['Z'], sep.outputs['Z']))), S.TIRE_R * 0.56, S.TIRE_R * 0.6),
                       ramp(m, noise(m, o, 30.0, 3.0), 0.55, 0.7))
        col = m.mix(scuff, col, core.hex_linear(0x6b6a66))
        m.set('Base Color', m.mix(dust, col, core.hex_linear(0x3b342d)))
        metal = 0.0 if S.WHEEL in ('cover8', 'cover_full', 'steel', 'truck') else 0.8
        m.set('Metallic', mix_float(m, dust, metal, 0.25))
        m.set('Roughness', mix_float(m, dust, 0.3 if metal else 0.38, 0.72))
        return m

    def rotor():
        m = core.Mat(tex(S, 'wbrake'))
        tc, sep = object_coords(m)
        o = tc.outputs['Object']
        y, z = sep.outputs['Y'], sep.outputs['Z']
        r = m.math('SQRT', m.math('ADD', m.math('MULTIPLY', y, y), m.math('MULTIPLY', z, z)))
        score = noise(m, vec(m, m.math('MULTIPLY', r, 400.0), 0.0, 0.0), detail=2.0)
        rust = m.math('MULTIPLY', ramp(m, noise(m, o, scale=30.0, detail=5.0), 0.5, 0.75), 0.75)
        col = m.mix(m.math('MULTIPLY', score, 0.4), core.hex_linear(0x55585c), core.hex_linear(0x75787c))
        col = m.mix(rust, col, core.hex_linear(0x5e3720))
        m.set('Base Color', col)
        m.set('Metallic', mix_float(m, rust, 0.9, 0.3))
        m.set('Roughness', mix_float(m, rust, m.math('ADD', 0.38, m.math('MULTIPLY', score, 0.12)), 0.82))
        return m
    return tire(), rim(), rotor()


def body_materials(S):
    lines = line_sheets(S)
    livery = livery_sheets(S) if getattr(S, 'TAXI', False) else None
    paint = paint_material(S, lines, livery)
    trim = street.shop_plastic(tex(S, 'trim'), 0x111214, rough=0.6, grime=0.35)
    glass = street.glass(tex(S, 'glass'), 0x0b0e12, rough=0.03)
    chrome = street.bare_metal(tex(S, 'chrome'), 0xd2d6da, rough=0.07, grime=0.25)
    head = None   # filled once the body exists (its shading is measured on it)
    amber = street.lens(tex(S, 'amber'), 0xd98a24, rough=0.08)
    plate = plate_slot(S)
    brake = core.Mat(tex(S, 'brake'))
    tc, sep = object_coords(brake)
    brake.set('Base Color', brake.mix(ramp(brake, noise(brake, tc.outputs['Object'], 30.0, 4.0), 0.4, 0.7), H(0x3f4245), H(0x55585c)))
    brake.set('Roughness', 0.6)
    brake.set('Metallic', 0.6)
    mats = {'paint': paint, 'trim': trim, 'glass': glass, 'chrome': chrome, 'amber': amber, 'plate': plate, 'brake': brake}
    return mats


def order_list(mats):
    return [mats['paint'], mats['trim'], mats['glass'], mats['chrome'], mats['head'], mats['amber'], mats['red'], mats['plate'], mats['brake']]


def mat_of(m):
    return m.m if isinstance(m, core.Mat) else m


def index_of(mats):
    return {mat_of(m).name: i for i, m in enumerate(order_list(mats))}


def slotted_object(name, bm, mats):
    me = bpy.data.meshes.new(name)
    for m in order_list(mats):
        me.materials.append(mat_of(m))
    bm.to_mesh(me)
    bm.free()
    return core.link(bpy.data.objects.new(name, me))


# ================================================================== the body

BODY_SPANS = [2 * c for c in SD.BODY_SPANS]     # twice sedan.py's samples per span: smoother sections
LOW_SPANS = SD.LOW_SPANS
GH_SPANS = [2 * c for c in SD.GH_SPANS]


def body_half(S, y):
    zb, ws, zs = S.ZB(y), S.WS(y), S.ZS(y)
    wsh = min(S.WSH(y), ws - 0.002)
    zt = S.ZT(y)
    wt = min(S.WT(y), wsh - 0.032)
    zc = max(S.ZC(y), zt + 0.004)
    zw = zb + 0.48 * (zs - zb)
    zl = zb + 0.065 + 0.55 * (zw - zb - 0.065)
    crown = zc - zt
    return [(0.0, zb), (0.55 * ws, zb), (ws - 0.07, zb + 0.006), (ws - 0.03, zb + 0.065), (ws - 0.0115, zl - 0.007), (ws - 0.0035, zl + 0.007),
            (ws, zw), (ws - 0.003, zs - 0.07), (wsh, zs), (wsh - 0.006, zs + 0.014), (wt + 0.022, zt - 0.02), (wt - 0.01, zt + 0.003),
            (0.5 * wt, zt + 0.75 * crown), (0.0, zc)]


def body_stations(S):
    end = [0.0, 0.0015, 0.004, 0.0075, 0.012, 0.018, 0.025, 0.033, 0.042, 0.052, 0.063, 0.075, 0.088, 0.102, 0.117, 0.135, 0.155, 0.18,
           0.21, 0.245, 0.285, 0.33, 0.38, 0.435, 0.495]
    front = [S.Y_F + d for d in end]
    rear = [S.Y_R - d for d in end]
    a, b = front[-1], rear[-1]
    n = int(round((b - a) / 0.035))
    mid = [a + (b - a) * i / n for i in range(1, n)]
    return front + mid + list(reversed(rear))


def loft_spans(name, rings, counts, mats):
    bm = bmesh.new()
    span_of = []
    for j, c in enumerate(list(counts) + list(reversed(counts))):
        span_of += [j + 1] * c
    layer = bm.faces.layers.int.new('span')
    verts = [[bm.verts.new(p) for p in ring] for ring in rings]
    m = len(rings[0])
    for a, b in zip(verts, verts[1:]):
        for k in range(m):
            f = bm.faces.new((a[k], a[(k + 1) % m], b[(k + 1) % m], b[k]))
            f[layer] = span_of[k]
    bm.faces.new(list(reversed(verts[0])))
    bm.faces.new(verts[-1])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    return slotted_object(name, bm, mats)


def body_shell(S, mats):
    rings = []
    for y in body_stations(S):
        rings.append([Vector((x, y, z)) for x, z in SD.mirrored_loop(body_half(S, y), BODY_SPANS)])
    body = loft_spans('body', rings, BODY_SPANS, mats)
    idx = index_of(mats)
    front_bvh = BVHTree.FromObject(body, bpy.context.evaluated_depsgraph_get())
    cuts = []
    for ay in S.AXLES:
        for sx in (-1, 1):
            cuts.append(parts.rod('arch_cut', (sx * 0.56, ay, S.WZ + 0.004), (sx * 1.3, ay, S.WZ + 0.004), S.ARCH_R, 72))
    cuts.append(SD.prism_y('grille_cut', S.GRILLE, S.Y_F - 0.2, S.Y_F + 0.125))
    cuts.append(SD.prism_y('intake_cut', S.INTAKE, S.Y_F - 0.2, S.Y_F + 0.06))
    for fx, fz in S.FOG:
        for sx in (-1, 1):
            cuts.append(parts.rod('fog_cut', (sx * fx, S.Y_F - 0.2, fz), (sx * fx, S.Y_F + 0.075, fz), S.FOG_R, 32))
    cuts.append(SD.prism_y('plate_cut', SD.rrect(S.PLATE_R[0], S.PLATE_R[1], 0.172, 0.088, 0.012), S.Y_R - 0.016, S.Y_R + 0.2))
    for c in cuts:
        core.assign(c, mat_of(mats['trim']))
        core.boolean(body, c)
    span = body.data.attributes['span']
    clad = getattr(S, 'CLADDING', False)
    for p in body.data.polygons:
        sp = span.data[p.index].value
        c = p.center
        low = sp in LOW_SPANS
        n = p.normal
        if (n.z < -0.9 and c.z < S.ZB(c.y) + 0.012) or (low and (c.y < S.AXLES[0] - S.ARCH_R or c.y > S.AXLES[1] + S.ARCH_R)):
            p.material_index = idx[mat_of(mats['trim']).name]
        if clad and (low or (abs(n.x) > 0.5 and c.z < S.ZB(c.y) + 0.12)):
            p.material_index = idx[mat_of(mats['trim']).name]
    return body, front_bvh


def band_f(S, y):
    return max(0.02, smoothstep(S.GH_Y0, S.GH_Y0 + 0.11, y) * smoothstep(S.GH_Y1, S.GH_Y1 - 0.13, y))


def gh_half(S, y):
    zb, xb = S.BELT(y), S.GH_XB(y)
    f = band_f(S, y)
    zd = max(S.DRIP(y), zb + 0.002)
    xd = xb - (zd - zb) * S.TUMBLE
    zr, xr = zd + 0.045 * f, xd - 0.05 * f
    zc = max(S.TOP(y), zr + 0.003)
    return [(0.0, zb - 0.06), (xb - 0.04, zb - 0.06), (xb + 0.003, zb - 0.025), (xb, zb), (xd, zd), (xd - 0.012 * f, zd + 0.03 * f), (xr, zr),
            (0.55 * xr, zc - 0.3 * (zc - zr)), (0.0, zc)]


def gh_stations(S):
    d = [0.0, 0.005, 0.013, 0.024, 0.038, 0.055, 0.075, 0.10, 0.13, 0.165]
    ys = [S.GH_Y0 + v for v in d] + [S.GH_Y1 - v for v in d]
    a, b = S.GH_Y0 + 0.2, S.GH_Y1 - 0.2
    n = int(round((b - a) / 0.035))
    ys += [a + (b - a) * i / n for i in range(n + 1)]
    ys += [S.Y_HDR, S.Y_RR]
    return sorted(set(round(v, 4) for v in ys))


def greenhouse(S, mats):
    idx = index_of(mats)
    I = {k: idx[mat_of(mats[k]).name] for k in ('trim', 'glass', 'paint')}
    stations = gh_stations(S)
    half_n = sum(GH_SPANS)
    ring_n = 2 * half_n
    k_belt = sum(GH_SPANS[:3])
    k_drip = sum(GH_SPANS[:4])
    k_roof = sum(GH_SPANS[:6])
    bm = bmesh.new()
    band = bm.faces.layers.int.new('band')
    stn = bm.faces.layers.int.new('station')
    verts = []
    drip_pts = []
    for i, y in enumerate(stations):
        loop = SD.mirrored_loop(gh_half(S, y), GH_SPANS)
        xr = gh_half(S, y)[6][0]
        bow = S.BOW(y)
        row = []
        for k, (x, z) in enumerate(loop):
            yy = y
            kk = k if k <= half_n else ring_n - k
            if kk > k_roof:
                yy = y + bow * max(0.0, 1.0 - (x / max(xr, 1e-3)) ** 2)
            row.append(bm.verts.new((x, yy, z)))
        verts.append(row)
        drip_pts.append((Vector((loop[k_drip][0], y, loop[k_drip][1])), Vector((loop[ring_n - k_drip][0], y, loop[ring_n - k_drip][1]))))
    for i in range(len(stations) - 1):
        a, b = verts[i], verts[i + 1]
        for k in range(ring_n):
            f = bm.faces.new((a[k], a[(k + 1) % ring_n], b[(k + 1) % ring_n], b[k]))
            kk = k if k < half_n else ring_n - 1 - k
            f[band] = 0 if kk < k_belt else 1 if kk < k_drip else 2 if kk < k_roof else 3
            f[stn] = i
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if f[band] == 0 and min(v.co.z for v in f.verts) < S.BELT(f.calc_center_median().y) - 0.03],
                     context='FACES')
    for line, _ in S.SIDE:
        (y0, z0), (y1, z1) = line
        d = Vector((0.0, y1 - y0, z1 - z0)).normalized()
        nrm = Vector((0.0, -d.z, d.y))
        geom = bm.verts[:] + bm.edges[:] + bm.faces[:]
        bmesh.ops.bisect_plane(bm, geom=geom, dist=1e-5, plane_co=(0.0, y0, z0), plane_no=nrm)
    i_hdr = stations.index(round(S.Y_HDR, 4))
    i_rr = stations.index(round(S.Y_RR, 4))
    glass_faces = []
    for f in bm.faces:
        c = f.calc_center_median()
        b = f[band]
        mask = 0.0
        if b == 0:
            mi = I['trim']
        elif b == 1:
            kind = 'trim'
            for line, k in S.SIDE:
                if c.y > line_y(line, c.z):
                    kind = k
            if kind == 'trim' and c.z >= S.SAIL_ZMAX:
                kind = 'glass'
            if kind == 'glass':
                mi, mask = I['glass'], 1.0
            elif kind == 'black':
                mi = I['glass']
            elif kind == 'paint':
                mi = I['paint']
            else:
                mi = I['trim']
        elif b == 2:
            mi = I['paint']
        else:
            if f[stn] < i_hdr or f[stn] >= i_rr:
                mi, mask = I['glass'], 1.0
            else:
                mi = I['paint']
        f.material_index = mi
        if mask > 0.5:
            glass_faces.append(f)
    res = bmesh.ops.inset_region(bm, faces=glass_faces, thickness=0.006, depth=-0.007, use_even_offset=True, use_boundary=True)
    for f in res['faces']:
        f.material_index = I['trim']
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=1e-5)
    return slotted_object('greenhouse', bm, mats), drip_pts, stations


# ================================================================== the wheel (its own mesh, at its hub)

def wheel_mesh(S, mats_w):
    """One wheel at the origin, the axle along X, its outer face toward +X. The tire is sedan.py's section scaled to the
    spec; the face by style: twin5 / split6 alloys, cover8 / cover_full plastic covers, steel."""
    tire_m, rim_m, rot_m = mats_w
    R = S.TIRE_R
    sr = R / 0.334
    sw = S.TIRE_W / 0.225
    axle = Matrix.Rotation(math.radians(90), 4, 'Y')

    def at(r, ang, w):
        return Vector((w, r * math.cos(ang), r * math.sin(ang)))
    out = []
    tire = closed_lathe('tire', [(r * sr, a * sw) for r, a in SD.TIRE], axle, 72)
    for v in tire.data.vertices:
        if v.co.z < -S.WZ:
            v.co.z = -S.WZ
    core.assign(tire, tire_m)
    out.append(tire)
    rim = closed_lathe('rim', [(r * sr, a * sw) for r, a in SD.RIM], axle, 64)
    core.assign(rim, rim_m)
    out.append(rim)
    rr = 0.197 * sr                     # the rim's inner lip
    if S.WHEEL in ('twin5', 'split6'):
        n_sp = 5 if S.WHEEL == 'twin5' else 6
        out.append(closed_lathe('hub', [(0.0, 0.05 * sw), (0.082 * sr, 0.05 * sw), (0.09 * sr, 0.044 * sw), (0.09 * sr, 0.02 * sw), (0.0, 0.02 * sw)],
                                axle, 40))
        core.assign(out[-1], rim_m)
        out.append(closed_lathe('cap', [(0.0, 0.069 * sw), (0.022, 0.0685 * sw), (0.031, 0.065 * sw), (0.034, 0.058 * sw), (0.034, 0.05 * sw),
                                        (0.0, 0.05 * sw)], axle, 32))
        core.assign(out[-1], rim_m)
        for k in range(n_sp):
            a = 2 * math.pi * k / n_sp + math.pi / 10
            out.append(parts.rod('lug', at(0.0572 * sr, a, 0.048 * sw), at(0.0572 * sr, a, 0.064 * sw), 0.0105, 6))
            core.assign(out[-1], rim_m)
        for k in range(n_sp):
            base = 2 * math.pi * k / n_sp + math.pi / 2
            for sgn in (-1, 1):
                rings = []
                for j in range(9):
                    r = 0.072 * sr + (rr - 0.072 * sr) * j / 8
                    t = j / 8
                    spread = (0.135 if n_sp == 5 else 0.11) - 0.04 * t
                    ang = base + sgn * spread * (0.33 + 0.67 * min(1.0, t * 2.0))
                    hw = (0.0148 - 0.0018 * t) * (1.0 if n_sp == 5 else 0.9) * sr
                    tt = (r - 0.07 * sr) / (rr - 0.07 * sr)
                    wf = (0.056 + 0.03 * tt + 0.004 * math.sin(math.pi * tt)) * sw
                    wb = wf - 0.032 * sw
                    tdir = Vector((0.0, -math.sin(ang), math.cos(ang)))
                    c = at(r, ang, 0.0)
                    sec = [(-0.75 * hw, wf), (0.75 * hw, wf), (hw, wf - 0.006), (0.9 * hw, wb), (-0.9 * hw, wb), (-hw, wf - 0.006)]
                    rings.append([c + tdir * t_ + Vector((w, 0.0, 0.0)) for t_, w in sec])
                sp = core.loft('spoke', rings)
                core.assign(sp, rim_m)
                out.append(sp)
    else:
        # a pressed steel wheel's face (the dish and its round vent holes behind a cover, or bare)
        disk = closed_lathe('disk', [(0.0, 0.02 * sw), (0.10 * sr, 0.02 * sw), (0.15 * sr, 0.045 * sw), (rr, 0.07 * sw), (rr, 0.06 * sw), (0.15 * sr, 0.035 * sw),
                                     (0.10 * sr, 0.01 * sw), (0.0, 0.01 * sw)], axle, 48)
        core.assign(disk, rim_m)
        out.append(disk)
        if S.WHEEL in ('cover8', 'cover_full'):
            # the cover: a shallow plastic dish over the face with radial slots (cover8) or nearly closed (cover_full)
            prof = [(0.0, 0.078 * sw), (0.04 * sr, 0.078 * sw), (0.12 * sr, 0.084 * sw), (0.18 * sr, 0.09 * sw), (0.205 * sr, 0.094 * sw),
                    (0.212 * sr, 0.088 * sw), (0.205 * sr, 0.076 * sw), (0.0, 0.07 * sw)]
            cov = closed_lathe('cover', prof, axle, 64)
            if S.WHEEL == 'cover8':
                for k in range(8):
                    a = 2 * math.pi * k / 8
                    cut = parts.rbox('slot', (0, 0, 0), (0.06, 0.045 * sr, 0.022 * sr), 0.008)
                    cut.data.transform(Matrix.Translation(at(0.155 * sr, a, 0.088 * sw)) @ Matrix.Rotation(a + math.pi / 2, 4, 'X'))
                    core.boolean(cov, cut)
            core.assign(cov, rim_m)
            out.append(cov)
        else:
            for k in range(5):
                a = 2 * math.pi * k / 5
                out.append(parts.rod('lug', at(0.0572 * sr, a, 0.02 * sw), at(0.0572 * sr, a, 0.04 * sw), 0.0105, 6))
                core.assign(out[-1], rim_m)
    rot = closed_lathe('rotor', [(0.0, 0.03), (0.08, 0.03), (0.082, 0.016), (0.150 * sr, 0.016), (0.152 * sr, 0.012), (0.152 * sr, -0.010),
                                 (0.150 * sr, -0.014), (0.07, -0.014), (0.0, -0.014)], axle, 40)
    core.assign(rot, rot_m)
    out.append(rot)
    obj = core.join(out, f'{S.name}_Wheel')
    obj.name = obj.data.name = f'{S.name}_Wheel'
    core.finish_hard_surface(obj, 38.0)
    ti = next(i for i, s in enumerate(obj.material_slots) if s.material == mat_of(tire_m))

    def outer_wall(f):
        c = f.calc_center_median()
        return f.material_index == ti and c.x > 0.05 and f.normal.x > 0.5

    groups = [(outer_wall, 'box', None, (0.0, 0.36, 1.0, 1.0)), (lambda f: f.material_index == ti, 'box', None, (0.0, 0.0, 1.0, 0.35))]
    groups += [(lambda f, i=i: f.material_index == i, 'box', None, (0.0, 0.0, 1.0, 1.0)) for i in range(len(obj.material_slots))]
    core.uv_layout(obj, groups)
    from ns_ground import order_slots
    order_slots(obj, [mat_of(tire_m), mat_of(rim_m), mat_of(rot_m)], TAG)
    log(f'{obj.name} tris {tri_count(obj)}')
    return obj


def calipers(S, mats):
    out = []
    ca = math.radians(18.0)
    for ay in S.AXLES:
        for sx in (-1, 1):
            cal = parts.rbox('caliper', (0, 0, 0), (0.05, 0.11, 0.055), 0.014)
            pos = Vector((sx * (S.TRACK - 0.004), ay + 0.135 * (S.TIRE_R / 0.334) * math.cos(ca), S.WZ + 0.135 * (S.TIRE_R / 0.334) * math.sin(ca)))
            cal.data.transform(Matrix.Translation(pos) @ Matrix.Rotation(ca, 4, 'X'))
            out.append((cal, mats['brake']))
    return out


# ================================================================== lamps, front, rear, sides, trims (sedan.py's, from the spec)

def rear_ray(S):
    return lambda p: ((p[0], S.Y_R + 1.0, p[1]), (0.0, -1.0, 0.0))


def side_ray(sx):
    return lambda p: ((sx * 2.0, p[0], p[1]), (-sx, 0.0, 0.0))


def lamps(S, bvh, bvh0, mats):
    out = []
    seals = []
    for sx in (-1, 1):
        out.append((SD.lens_patch('headlamp', bvh, resample(S.HEAD, 72, 0.0075), corner_spot(S, bvh, sx, *S.HEAD_AXIS, -1), lift=0.0035,
                                  steps=LENS_STEPS, seal=seals), mats['head']))
        out.append((SD.lens_patch('taillamp', bvh, resample(S.TAIL, 72, 0.0075), corner_spot(S, bvh, sx, *S.TAIL_AXIS, 1), lift=0.0035,
                                  steps=LENS_STEPS, seal=seals), mats['red']))
        y0, z0, y1, z1 = S.MARK_F
        mk = [(y, z) for y, z in core.rounded_rect(y0, z0, y1, z1, 0.012, 3)]
        out.append((SD.lens_patch('marker', bvh, mk, side_ray(sx), steps=(1.0, 0.5), seal=seals), mats['amber']))
        y0, z0, y1, z1 = S.MARK_R
        mk = [(y, z) for y, z in core.rounded_rect(y0, z0, y1, z1, 0.011, 3)]
        out.append((SD.lens_patch('marker', bvh, mk, side_ray(sx), steps=(1.0, 0.5), seal=seals), mats['red']))
        for fx, fz in S.FOG:
            p, nr = hit(bvh0, (sx * fx, S.Y_F - 1.0, fz), (0, 1, 0))
            if p is None:
                continue
            out.append((parts.disc('fog', (sx * fx, p.y + 0.016, fz), (0, 1, 0), S.FOG_R - 0.004, 0.008, 32), mats['head']))
            out.append((closed_lathe('fog_ring', [(S.FOG_R - 0.008, -0.006), (S.FOG_R + 0.001, -0.006), (S.FOG_R + 0.001, 0.012), (S.FOG_R - 0.008, 0.012)],
                                     Matrix.Translation((sx * fx, p.y + 0.004, fz)) @ Matrix.Rotation(math.radians(90), 4, 'X'), 32), mats['chrome']))
    out += [(s, mats['trim']) for s in seals]
    return out


def plate_quad(name, center, w, h, facing, mat):
    """A plate's face (the [screen]): UV 0..1 as seen from outside. facing -1: the front (seen from -Y), +1: the rear."""
    cx, cy, cz = center
    bm = bmesh.new()
    if facing < 0:
        pts = [(cx - w / 2, cy, cz - h / 2), (cx + w / 2, cy, cz - h / 2), (cx + w / 2, cy, cz + h / 2), (cx - w / 2, cy, cz + h / 2)]
        uvs = [(0, 0), (1, 0), (1, 1), (0, 1)]
    else:
        pts = [(cx + w / 2, cy, cz - h / 2), (cx - w / 2, cy, cz - h / 2), (cx - w / 2, cy, cz + h / 2), (cx + w / 2, cy, cz + h / 2)]
        uvs = [(0, 0), (1, 0), (1, 1), (0, 1)]
    uvl = bm.loops.layers.uv.new('UVMap')
    f = bm.faces.new([bm.verts.new(p) for p in pts])
    for lp, t in zip(f.loops, uvs):
        lp[uvl].uv = t
    o = core.mesh_object(name, bm)
    core.orient_normals(o, (0, facing, 0))
    core.assign(o, mat)
    set_attr(o, 'ns_uvlock', 1.0)
    return o


def front_end(S, bvh0, mats):
    out = []

    def surface_y(x, z):
        p, _ = hit(bvh0, (x, S.Y_F - 1.0, z), (0, 1, 0))
        return p.y if p is not None else S.Y_F

    pts, nrms = [], []
    for x, z in S.GRILLE:
        p, nr = hit(bvh0, (x, S.Y_F - 1.0, z), (0, 1, 0))
        if p is None:
            continue
        pts.append(p)
        nrms.append(nr)
    if len(pts) > 4:
        out.append((ribbon('grille_ring', pts, nrms, strip(0.016, 0.006, 0.01), closed=True), mats['chrome'] if S.TRIM_CHROME else mats['trim']))

    def bar(name, z, half, back, prof, m):
        pts, nrms = [], []
        for k in range(17):
            x = -half + 2 * half * k / 16
            p, nr = hit(bvh0, (x, S.Y_F - 1.0, z), (0, 1, 0))
            if p is None:
                continue
            pts.append(p + Vector((0, back, 0)))
            nrms.append(nr)
        if len(pts) > 2:
            out.append((ribbon(name, pts, nrms, prof, sides=[(0, 0, 1)] * len(pts)), m))

    slat = [(-0.011, -0.006), (0.011, -0.006), (0.011, 0.003), (0.008, 0.006), (-0.008, 0.006), (-0.011, 0.003)]
    for z in S.GRILLE_BARS:
        half = S.GRILLE_HW * (1.0 - 0.15 * max(0.0, S.GRILLE_Z - z) / 0.12) - 0.012
        bar('grille_bar', z, half, 0.016, slat, mats['chrome'] if S.TRIM_CHROME else mats['trim'])
    out.append((parts.disc('emblem', (0, surface_y(0, S.GRILLE_Z) + 0.004, S.GRILLE_Z), (0, 1, 0), 0.045, 0.012, 32), mats['chrome']))
    out[-1][0].data.transform(Matrix.Translation((0, 0, S.GRILLE_Z)) @ Matrix.Diagonal((1.0, 1.0, 0.62, 1.0)) @ Matrix.Translation((0, 0, -S.GRILLE_Z)))
    for z in S.INTAKE_SLATS:
        bar('intake_slat', z, S.INTAKE_HW, 0.022, [(-0.006, -0.02), (0.006, -0.02), (0.006, 0.004), (-0.006, 0.004)], mats['trim'])
    bump = getattr(S, 'PLATE_ON_BUMPER', None)
    if bump is not None:
        y, pz = bump
        out.append((parts.rbox('plate_back', (0, y - 0.002, pz), (0.31, 0.004, 0.16), 0.002), mats['chrome']))
        out.append((plate_quad('plate_f', (0, y - 0.0045, pz), 0.30, 0.15, -1, mats['plate']), None))
        return out
    y = surface_y(0.0, S.PLATE_F[1])
    out.append((parts.rbox('plate_bracket', (0, y - 0.003, S.PLATE_F[1]), (0.33, 0.008, 0.17), 0.004), mats['trim']))
    out.append((parts.rbox('plate_back', (0, y - 0.008, S.PLATE_F[1]), (0.30, 0.004, 0.15), 0.002), mats['chrome']))
    out.append((plate_quad('plate_f', (0, y - 0.0105, S.PLATE_F[1]), 0.30, 0.15, -1, mats['plate']), None))
    return out


def rear_end(S, bvh, mats):
    out = []
    out.append((parts.rbox('plate_back', (0, S.Y_R - 0.012, S.PLATE_R[1]), (0.30, 0.004, 0.15), 0.002), mats['chrome']))
    p, _ = hit(bvh, (0, S.Y_R + 1.0, S.PLATE_R[1]), (0, -1, 0))
    yb = (p.y if p is not None else S.Y_R) - 0.0
    out.append((plate_quad('plate_r', (0, S.Y_R - 0.0095, S.PLATE_R[1]), 0.30, 0.15, 1, mats['plate']), None))
    for sx in (-1, 1):
        out.append((parts.rbox('plate_lamp', (sx * 0.075, S.Y_R - 0.012, S.PLATE_R[1] + 0.082), (0.04, 0.012, 0.008), 0.003), mats['head']))
    if S.GARNISH_Z:
        pts, nrms = [], []
        for k in range(25):
            x = -0.40 + 0.80 * k / 24
            q, nr = hit(bvh, (x, S.Y_R + 1.0, S.GARNISH_Z), (0, -1, 0))
            if q is not None:
                pts.append(q)
                nrms.append(nr)
        if len(pts) > 3:
            out.append((ribbon('garnish', pts, nrms, [(-0.016, -0.004), (-0.016, 0.002), (-0.012, 0.004), (0.012, 0.004), (0.016, 0.002), (0.016, -0.004)],
                               sides=[(0, 0, 1)] * len(pts)), mats['chrome']))
    q, nr = hit(bvh, (0, S.Y_R + 1.0, S.EMBLEM_R_Z), (0, -1, 0))
    if q is not None:
        squash = Matrix.Translation((0, 0, S.EMBLEM_R_Z)) @ Matrix.Diagonal((1.0, 1.0, 0.62, 1.0)) @ Matrix.Translation((0, 0, -S.EMBLEM_R_Z))
        ring = closed_lathe('emblem', [(0.026, 0.0), (0.037, 0.0), (0.037, 0.004), (0.034, 0.007), (0.029, 0.007), (0.026, 0.004)],
                            Matrix.Translation((0, q.y - 0.001, S.EMBLEM_R_Z)) @ Matrix.Rotation(math.radians(90), 4, 'X'), 40)
        ring.data.transform(squash)
        out.append((ring, mats['chrome']))
    seals = []
    rx, rz, rw, rh = S.REFLECT
    for sx in (-1, 1):
        rf = core.rounded_rect(sx * rx - rw, rz, sx * rx + rw, rz + rh * 2, 0.008, 3)
        out.append((SD.lens_patch('reflector', bvh, rf, rear_ray(S), steps=(1.0, 0.5), seal=seals), mats['red']))
    out += [(s, mats['trim']) for s in seals]
    ex, ez = S.EXHAUST
    out.append((closed_lathe('exhaust', [(0.026, -0.08), (0.034, -0.08), (0.036, 0.0), (0.03, 0.004), (0.026, 0.0)],
                             Matrix.Translation((ex, S.Y_R - 0.045, ez)) @ Matrix.Rotation(math.radians(-90), 4, 'X'), 24), mats['chrome']))
    out.append((parts.disc('exhaust_in', (ex, S.Y_R - 0.06, ez), (0, 1, 0), 0.027, 0.01, 24), mats['trim']))
    return out


def side_details(S, bvh, gh_bvh, mats):
    out = []
    ry, rz, yb0, zc0 = S.MIRROR
    for sx in (-1, 1):
        p, nr = hit(gh_bvh, (sx * 2.0, ry, rz), (-sx, 0, 0))
        x0 = abs(p.x) if p is not None else S.GH_XB(S.GH_Y0) + 0.02
        rings, glass = [], []
        for t in (0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.78, 0.84, 0.89, 0.93, 0.96, 0.98, 0.995, 1.0):
            x = x0 + 0.03 + 0.205 * t
            k = max(0.08, (1.0 - ((t - 0.76) / 0.24) ** 4) ** 0.25) if t > 0.76 else 1.0
            hz, d, rc = (0.044 + 0.026 * t) * k, (0.046 + 0.018 * t) * k, 0.012 * k
            zc, yb = zc0 + 0.014 * t, yb0 + 0.01 * t
            sec = [(yb - rc + d * math.cos(math.pi / 2 + math.pi * q / 12), zc + hz * math.sin(math.pi / 2 + math.pi * q / 12)) for q in range(13)]
            sec += [(yb - rc + rc * math.cos(a_), zc - hz + rc + rc * math.sin(a_)) for a_ in (-math.pi / 3, -math.pi / 6, 0.0)]
            sec += [(yb - rc + rc * math.cos(a_), zc + hz - rc + rc * math.sin(a_)) for a_ in (0.0, math.pi / 6, math.pi / 3)]
            rings.append([Vector((sx * x, y_, z_)) for y_, z_ in sec])
            g = (hz - 0.011) * (0.82 if t in (0.1, 0.96) else 1.0)
            if 0.03 < t < 0.97:
                glass.append((Vector((sx * x, yb + 0.0012, zc + g)), Vector((sx * x, yb + 0.0012, zc - g))))
        out.append((core.loft('mirror', rings), mats['paint'] if not getattr(S, 'BLACK_MIRRORS', False) else mats['trim']))
        bm = bmesh.new()
        vs = [(bm.verts.new(a), bm.verts.new(b)) for a, b in glass]
        for (a0, b0), (a1, b1) in zip(vs, vs[1:]):
            bm.faces.new((b0, b1, a1, a0))
        gl = core.mesh_object('mirror_glass', bm)
        core.orient_normals(gl, toward=(0, 1, 0))
        out.append((gl, mats['chrome']))
        out.append((parts.rbox('mirror_arm', (sx * (x0 + 0.018), yb0 - 0.02, zc0 - 0.025), (0.05, 0.045, 0.028), 0.01), mats['trim']))
        out.append((parts.rbox('mirror_base', (sx * (x0 - 0.004), yb0 - 0.028, zc0 - 0.03), (0.025, 0.11, 0.065), 0.01), mats['trim']))
        for hy in S.HANDLES:
            z = S.ZS(hy) - 0.045
            p, nr = hit(bvh, (sx * 2.0, hy, z), (-sx, 0, 0))
            if p is None:
                continue
            cup = [(hy + 0.085 * math.cos(2 * math.pi * q / 24), z - 0.004 + 0.024 * math.sin(2 * math.pi * q / 24)) for q in range(24)]
            out.append((SD.lens_patch('handle_cup', bvh, cup, side_ray(sx), lift=0.0008, skirt=0.006, steps=(1.0, 0.5)), mats['trim']))
            rot = Vector((1, 0, 0)).rotation_difference(nr if sx > 0 else -nr).to_matrix().to_4x4()
            h = parts.rbox('handle', (0, 0, 0), (0.02, 0.19, 0.03), 0.0095)
            h.data.transform(Matrix.Translation(p + nr * 0.012) @ rot)
            out.append((h, mats['chrome'] if S.TRIM_CHROME else mats['paint']))
        for ay in S.AXLES:
            pts, nrms, sides = [], [], []
            for q in range(49):
                th = math.radians(-12 + 204 * q / 48)
                y, z = ay + S.ARCH_R * math.cos(th), S.WZ + 0.004 + S.ARCH_R * math.sin(th)
                if z < S.ZB(y) + 0.035:
                    continue
                pr = Vector((0.0, math.cos(th), math.sin(th)))
                p, nr = hit(bvh, (sx * 2.0, y + pr.y * 0.02, z + pr.z * 0.02), (-sx, 0, 0))
                if p is None:
                    continue
                pts.append(Vector((p.x, y, z)))
                nrms.append(Vector((nr.x, 0.0, nr.z * 0.5)))
                sides.append(pr)
            if len(pts) > 3:
                if getattr(S, 'CLADDING', False):
                    prof = [(-0.008, -0.014), (-0.006, 0.0), (0.0, 0.012), (0.03, 0.016), (0.06, 0.012), (0.075, -0.002), (0.07, -0.014)]
                    out.append((ribbon('arch_clad', pts, nrms, prof, sides=sides), mats['trim']))
                else:
                    prof = [(-0.008, -0.014), (-0.006, 0.0), (0.0, 0.006), (0.012, 0.0075), (0.024, 0.0045), (0.034, -0.002), (0.03, -0.014)]
                    out.append((ribbon('arch_lip', pts, nrms, prof, sides=sides), mats['paint']))
    return out


def window_trim(S, gh_bvh, body_bvh, drip, stations, mats):
    out = []
    tm = mats['chrome'] if S.TRIM_CHROME else mats['trim']
    c_edge = S.SIDE[-1][0]
    for sx in (1, -1):
        pts, nrms = [], []
        for (pr, pl), y in zip(drip, stations):
            p = pr if sx > 0 else pl
            if y < S.GH_Y0 + 0.015 or y > line_y(c_edge, p.z) + 0.004:
                continue
            q, nr = gh_bvh.find_nearest(p)[:2]
            up = Vector((0, 0, 1)) - nr * nr.z
            pts.append(q + up.normalized() * 0.006)
            nrms.append(nr)
        if len(pts) > 2:
            out.append((ribbon('drip_trim', pts, nrms, strip(0.011, 0.0035)), tm))
        pts, nrms = [], []
        yb = line_y(c_edge, S.BELT(c_edge[0][0]))
        for k in range(40):
            y = S.GH_Y0 + 0.02 + (yb - 0.02 - S.GH_Y0 - 0.02) * k / 39
            p, nr = body_bvh.find_nearest(Vector((sx * (S.GH_XB(y) + 0.007), y, S.BELT(y) - 0.002)))[:2]
            pts.append(p)
            nrms.append(nr)
        out.append((ribbon('belt_trim', pts, nrms, strip(0.013, 0.005)), tm))
    if getattr(S, 'ROOF_RAILS', False):
        for sx in (-1, 1):
            pts, nrms = [], []
            for k in range(30):
                y = -0.25 + 1.95 * k / 29
                p, nr = hit(gh_bvh, (sx * 0.62, y, 3.0), (0, 0, -1))
                if p is not None:
                    pts.append(p + Vector((0, 0, 0.055 if 2 < k < 27 else 0.012 + 0.043 * min(k, 29 - k) / 2.5)))
                    nrms.append(Vector((0, 0, 1)))
            if len(pts) > 3:
                out.append((ribbon('rail', pts, nrms, [(-0.015, -0.012), (0.015, -0.012), (0.017, 0.006), (0.0, 0.016), (-0.017, 0.006)],
                                   sides=[(1, 0, 0)] * len(pts)), mats['trim']))
                for k in (2, 15, 27):
                    if k < len(pts):
                        out.append((parts.rbox('rail_foot', pts[k] - Vector((0, 0, 0.03)), (0.03, 0.08, 0.06), 0.01), mats['trim']))
    return out


def cowl_and_wipers(S, bvh, gh_bvh, mats):
    out = []

    def ws_base(x):
        return S.GH_Y0 + S.BOW(S.GH_Y0) * max(0.0, 1.0 - (x / S.GH_XB(S.GH_Y0)) ** 2)

    hx = S.GH_XB(S.GH_Y0) - 0.06
    outline = []
    for k in range(13):
        x = -hx + 2 * hx * k / 12
        outline.append((x, ws_base(x) - 0.095))
    for k in range(13):
        x = hx - 2 * hx * k / 12
        outline.append((x, ws_base(x) - 0.012))
    out.append((SD.lens_patch('cowl', bvh, outline, lambda p: ((p[0], p[1], 4.0), (0, 0, -1)), lift=0.004, skirt=0.01, steps=(1.0, 0.5)), mats['trim']))
    for x0, x1 in ((-0.06, hx - 0.1), (-hx + 0.04, -0.12)):
        pts, nrms = [], []
        for k in range(12):
            x = x0 + (x1 - x0) * k / 11
            y = ws_base(x) + 0.05
            p, nr = hit(gh_bvh, (x, y, 4.0), (0, 0, -1))
            if p is not None:
                pts.append(p + nr * 0.012)
                nrms.append(nr)
        if len(pts) > 2:
            out.append((ribbon('wiper', pts, nrms, [(-0.008, -0.008), (0.008, -0.008), (0.006, 0.006), (-0.006, 0.006)]), mats['trim']))
            piv = Vector((x0 + 0.04 if x0 > -0.3 else x1 - 0.04, ws_base(x0) - 0.04, 0.0))
            p, nr = hit(bvh, (piv.x, piv.y, 4.0), (0, 0, -1))
            if p is not None:
                mid = pts[len(pts) // 2]
                out.append((parts.rod('wiper_arm', p + Vector((0, 0, 0.012)), mid + nr * 0.008, 0.007, 8), mats['trim']))
                out.append((parts.disc('wiper_pivot', p + Vector((0, 0, 0.01)), (0, 0, 1), 0.016, 0.02, 12), mats['trim']))
    return out


def cabin(S):
    C = S.CABIN

    def mat(name, hex_color, rough=0.85, glow=0.0):
        m = core.Mat(name)
        m.set('Base Color', core.hex_linear(hex_color))
        m.set('Roughness', rough)
        if glow:
            m.set('Emission Color', core.hex_linear(hex_color))
            m.set('Emission Strength', glow)
        return m
    cloth = mat('cab_cloth', 0x3f4144)
    dark = mat('cab_dark', 0x1d1e20, 0.6)
    card = mat('cab_card', 0x333437, 0.7)
    liner = mat('cab_liner', 0x5e5c58)
    floor_m = mat('cab_floor', 0x232324)
    out = []

    def bx(name, c, size, r, m, tilt=0.0):
        o = parts.rbox(name, (0, 0, 0), size, r)
        o.data.transform(Matrix.Translation(c) @ Matrix.Rotation(math.radians(tilt), 4, 'X'))
        core.assign(o, m)
        out.append(o)
    fz = C['floor']
    sf, sr = C['seat_f'], C['seat_r']
    W = C['width']
    for sx in (-1, 1):
        x = sx * 0.37 * W / 0.74
        bx('seat', (x, sf, fz + 0.2), (0.50, 0.52, 0.13), 0.05, cloth, -6)
        bx('seat_back', (x, sf + 0.24, fz + 0.52), (0.50, 0.13, 0.62), 0.06, cloth, -14)
        bx('headrest', (x, sf + 0.32, fz + 0.9), (0.27, 0.11, 0.19), 0.05, cloth, -10)
        bx('door_card', (sx * (W - 0.005), sf - 0.16, fz + 0.46), (0.05, 1.05, 0.48), 0.02, card)
        bx('door_card', (sx * (W - 0.01), sr + 0.12, fz + 0.48), (0.05, 0.86, 0.46), 0.02, card)
        bx('rear_headrest', (sx * 0.46, sr + 0.44, fz + 0.76), (0.25, 0.10, 0.15), 0.045, cloth, -18)
    bx('rear_seat', (0, sr, fz + 0.19), (1.34, 0.50, 0.15), 0.06, cloth, -5)
    bx('rear_back', (0, sr + 0.32, fz + 0.46), (1.34, 0.15, 0.58), 0.06, cloth, -22)
    bx('console', (0, sf - 0.08, fz + 0.26), (0.24, 0.85, 0.26), 0.04, dark)
    bx('dash', (0, C['dash'], fz + 0.54), (1.46, 0.42, 0.26), 0.07, dark)
    bx('dash_top', (0, C['dash'] - 0.08, fz + 0.65), (1.40, 0.30, 0.06), 0.03, dark, 12)
    bx('floor', (0, (sf + sr) / 2, fz), (1.50, 2.6, 0.04), 0.01, floor_m)
    if C['hatch']:
        bx('cargo', (0, C['rear_shelf'] - 0.2, fz + 0.25), (1.30, 0.6, 0.03), 0.01, floor_m)
        bx('shelf', (0, C['rear_shelf'] - 0.25, fz + 0.62), (1.30, 0.5, 0.02), 0.01, card)
    else:
        bx('parcel_shelf', (0, C['rear_shelf'], fz + 0.685), (1.40, 0.50, 0.03), 0.01, card)
    bx('rv_mirror', (0, S.Y_HDR - 0.13, C['roof'] - 0.095), (0.25, 0.035, 0.068), 0.02, dark, 8)
    for sx in (-1, 1):
        bx('visor', (sx * 0.36, S.Y_HDR + 0.03, C['roof'] - 0.03), (0.38, 0.17, 0.024), 0.01, liner, 6)
    ring = [Vector((0.37 + 0.185 * math.cos(2 * math.pi * k / 32), 0.0, 0.185 * math.sin(2 * math.pi * k / 32))) for k in range(32)]
    wheel_o = core.sweep('steering', ring, parts.circle(0.017, 8), closed=True)
    wheel_o.data.transform(Matrix.Translation((0.0, sf - 0.28, fz + 0.62)) @ Matrix.Rotation(math.radians(25), 4, 'X'))
    core.assign(wheel_o, dark)
    out.append(wheel_o)
    return out


def bake_cabin(S, obj, mats):
    """sedan.py's cabin bake: what each pane looks onto, baked into an image the glass shows faintly under its tint."""
    sc = bpy.context.scene
    gi = index_of(mats)[mat_of(mats['glass']).name]
    stuff = cabin(S)
    me = obj.data.copy()
    bm = bmesh.new()
    bm.from_mesh(me)
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if f.material_index != gi], context='FACES')
    bm.to_mesh(me)
    bm.free()
    dup = core.link(bpy.data.objects.new('glass_bake', me))
    size = TEXTURE_SIZES[mname(S, 'glass')]
    img = bpy.data.images.new(f'T_{S.tag}_cabin_view', size, size, alpha=False, float_buffer=True)
    see = core.Mat('cabin_see_' + S.tag)
    tr = see.node('ShaderNodeBsdfTransparent')
    see.link(tr.outputs[0], see.out.inputs['Surface'])
    tx = see.nt.nodes.new('ShaderNodeTexImage')
    tx.image = img
    see.nt.nodes.active = tx
    me.materials.clear()
    me.materials.append(see.m)
    for p in me.polygons:
        p.material_index = 0
    clear = core.Mat('cabin_clear_' + S.tag)
    tr2 = clear.node('ShaderNodeBsdfTransparent')
    clear.link(tr2.outputs[0], clear.out.inputs['Surface'])
    keep = obj.data.materials[gi]
    obj.data.materials[gi] = clear.m
    world = sc.world
    world_color = tuple(world.color)
    world.color = (0.55, 0.57, 0.6)
    sc.cycles.samples = 96
    sc.render.bake.margin = 6
    sc.render.bake.use_clear = True
    sc.cycles.use_auto_tile = False
    bk = sc.render.bake
    bk.use_selected_to_active = True
    bk.use_cage = False
    bk.cage_extrusion = 0.02
    bk.max_ray_distance = 2.5
    core.select_only(stuff + [dup], dup)
    try:
        bpy.ops.object.bake(type='COMBINED')
    finally:
        bk.use_selected_to_active = False
        bk.cage_extrusion = 0.0
        bk.max_ray_distance = 0.0
    obj.data.materials[gi] = keep
    world.color = world_color
    bpy.data.objects.remove(dup)
    for o in stuff:
        bpy.data.objects.remove(o)
    w, h = img.size
    a = np.empty(w * h * 4, dtype=np.float32)
    img.pixels.foreach_get(a)
    a = a.reshape(h, w, 4)
    for _ in range(2):
        a = (a + np.roll(a, 1, 0) + np.roll(a, -1, 0)) / 3.0
        a = (a + np.roll(a, 1, 1) + np.roll(a, -1, 1)) / 3.0
    img.pixels.foreach_set(a.ravel())
    img.update()
    return img


def show_cabin(S, glass, img):
    m = glass
    tx = m.nt.nodes.new('ShaderNodeTexImage')
    tx.image = img
    k = 0.3
    m.set('Base Color', m.mix(k, core.hex_linear(0x0b0e12), tx.outputs['Color'], blend='ADD'))


# ================================================================== the taxi's roof light

def taxi_sign(S, mats, gh_bvh, glows):
    """A TAXI roof light: a 0.86 m box in the cab's yellow on a black base and two feet, a chrome cap rail, TAXI raised 4 mm
    in black on its front and back faces (reading from outside); the lit faces in the glow mesh (cell 7) 2 mm out, behind
    the letters, so at night the box glows with the word dark across it."""
    from ns_ground import text_mesh, text_bounds
    out = []
    p, nr = hit(gh_bvh, (0.0, 0.25, 4.0), (0, 0, -1))
    z0 = p.z if p is not None else 1.45
    L, W, Hh = 0.86, 0.17, 0.21
    yc = 0.25
    for fx in (-0.3, 0.3):
        out.append((parts.rbox('taxi_foot', (fx, yc, z0 + 0.008), (0.12, 0.2, 0.024), 0.008), mats['trim']))
    out.append((parts.rbox('taxi_base', (0, yc, z0 + 0.028), (L + 0.03, W + 0.03, 0.028), 0.01), mats['trim']))
    zc = z0 + 0.042 + Hh / 2
    out.append((parts.rbox('taxi_box', (0, yc, zc), (L, W, Hh), 0.035), mats['paint']))
    out.append((parts.rbox('taxi_cap', (0, yc, zc + Hh / 2 - 0.004), (L - 0.06, W - 0.03, 0.014), 0.006), mats['chrome']))
    for sgn in (-1, 1):
        # the lit faces (front and back of the box), a hair proud of it
        bm = bmesh.new()
        uvl = bm.loops.layers.uv.new('UVMap')
        yy = yc + sgn * (W / 2 + 0.002)
        za, zb = zc - Hh / 2 + 0.03, zc + Hh / 2 - 0.035
        pts = [(-L / 2 + 0.04, yy, za), (L / 2 - 0.04, yy, za), (L / 2 - 0.04, yy, zb), (-L / 2 + 0.04, yy, zb)]
        if sgn < 0:
            pts = [pts[1], pts[0], pts[3], pts[2]]
        f = bm.faces.new([bm.verts.new(q) for q in pts])
        cell = GLOW_CELLS['dome']
        u0, v0 = (cell % 4) * 0.25, (cell // 4) * 0.5
        for lp, (a, b) in zip(f.loops, ((0, 0), (1, 0), (1, 1), (0, 1))):
            lp[uvl].uv = (u0 + 0.25 * (0.02 + 0.96 * a), v0 + 0.5 * (0.02 + 0.96 * b))
        g = core.mesh_object('taxi_glow', bm)
        core.orient_normals(g, (0, sgn, 0))
        glows.append(g)
        # TAXI in raised black letters (in front of the glow film), reading from outside the face: the text's +X runs to the
        # reader's right (-X seen from behind the car, +X from in front), its +Y up, its +Z out of the face
        t = text_mesh('TAXI', 0.15, 'Black', 'CENTER', tracking=1.12, extrude=0.0016)
        x0, x1, y0, y1 = text_bounds(t)
        t.data.transform(Matrix.Translation((-(x0 + x1) / 2, -(y0 + y1) / 2, 0.0)))
        if sgn > 0:
            rot = Matrix(((-1, 0, 0, 0), (0, 0, 1, 0), (0, 1, 0, 0), (0, 0, 0, 1)))
        else:
            rot = Matrix(((1, 0, 0, 0), (0, 0, -1, 0), (0, 1, 0, 0), (0, 0, 0, 1)))
        t.data.transform(Matrix.Translation((0.0, yc + sgn * (W / 2 + 0.0042), zc - 0.002)) @ rot)
        out.append((t, mats['trim']))
    return out


# ================================================================== glow lamps

def glow_patch(bvh, outline, ray, lift, steps, cell, inner=None):
    """A thin patch over a lamp (outline fitted like a lens), UV'd into its Signal-grid cell planar in the outline's own
    parameters. inner: (lo, hi) of the shrink steps to keep (an annulus or the middle)."""
    n = len(outline)
    cu = sum(p[0] for p in outline) / n
    cv = sum(p[1] for p in outline) / n
    us = [p[0] for p in outline]
    vs_ = [p[1] for p in outline]
    u_lo, u_hi, v_lo, v_hi = min(us), max(us), min(vs_), max(vs_)
    rows = []
    for s in steps:
        row = []
        for u, v in outline:
            pu, pv = cu + (u - cu) * s, cv + (v - cv) * s
            r = ray((pu, pv))
            p, nr = r if isinstance(r[0], Vector) or r[0] is None else hit(bvh, *r)
            if p is None:
                if row:
                    p, nr = row[-1][0], row[-1][1]
                else:
                    continue
            row.append((p, nr, (pu, pv)))
        if len(row) == n:
            rows.append(row)
    if len(rows) < 2:
        return None
    col, rw = cell % 4, cell // 4
    u0, v0 = col * 0.25, rw * 0.5

    def uv(pu, pv):
        a = (pu - u_lo) / max(u_hi - u_lo, 1e-6)
        b = (pv - v_lo) / max(v_hi - v_lo, 1e-6)
        return (u0 + 0.25 * (0.02 + 0.96 * a), v0 + 0.5 * (0.02 + 0.96 * b))
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new('UVMap')
    vrows = [[(bm.verts.new(p + nr * lift), q) for p, nr, q in row] for row in rows]
    for a, b in zip(vrows, vrows[1:]):
        for k in range(n):
            f = bm.faces.new((a[k][0], a[(k + 1) % n][0], b[(k + 1) % n][0], b[k][0]))
            for lp, (vv, q) in zip(f.loops, (a[k], a[(k + 1) % n], b[(k + 1) % n], b[k])):
                lp[uvl].uv = uv(*q)
    if inner is None or inner == 'fill':
        r = ray((cu, cv))
        pc, nc = r if isinstance(r[0], Vector) else hit(bvh, *r)
        if pc is not None:
            vc = bm.verts.new(pc + nc * lift)
            last = vrows[-1]
            for k in range(n):
                f = bm.faces.new((last[k][0], last[(k + 1) % n][0], vc))
                for lp, q in zip(f.loops, (last[k][1], last[(k + 1) % n][1], (cu, cv))):
                    lp[uvl].uv = uv(*q)
    o = core.mesh_object('glow', bm)
    return o


def circle_outline(cphi, cz, r_phi, r_z, n=24):
    return [(cphi + r_phi * math.cos(2 * math.pi * k / n), cz + r_z * math.sin(2 * math.pi * k / n)) for k in range(n)]


def glow_lamps(S, bvh, gh, glows_extra):
    """The lamps' glow faces, 1.2 mm over their lenses: the low beam's projector and the high beam's bowl, the tail lamp
    (its outer band) and the brake lamp (its middle), the reverse lamp (the inner end), the turn lamps (the headlamp's amber
    end, the front side marker, the tail lamp's low strip), the dome (a film on the side glass and the windshield)."""
    out = []
    for sx in (-1, 1):
        hs = corner_spot(S, bvh, sx, *S.HEAD_AXIS, -1)
        ts = corner_spot(S, bvh, sx, *S.TAIL_AXIS, 1)
        # circles in (phi, z): phi degrees ~ arc at the axis' reach (0.45 m): 1 deg ~ 7.9 mm
        k = 180.0 / (math.pi * 0.45)
        p, z, R = S.HEAD_PJ
        g = glow_patch(bvh, circle_outline(p, z, R * 0.9 * k, R * 0.9), hs, 0.0047, (1.0, 0.6, 0.3), GLOW_CELLS['head_low'])
        if g:
            out.append(g)
        p, z, R = S.HEAD_HB
        g = glow_patch(bvh, circle_outline(p, z, R * 0.9 * k, R * 0.9), hs, 0.0047, (1.0, 0.6, 0.3), GLOW_CELLS['head_high'])
        if g:
            out.append(g)
        # the car faces -Y, so its left side is +X
        turn = 'turn_l' if sx > 0 else 'turn_r'
        amb = [(ph, zz) for ph, zz in S.HEAD if ph >= S.HEAD_AMBER - 2]
        if len(amb) >= 3:
            amb = resample(amb, 24, 0.0075)
            g = glow_patch(bvh, amb, hs, 0.0047, (1.0, 0.7, 0.4), GLOW_CELLS[turn])
            if g:
                out.append(g)
        # tail: the outer band (tail), the middle (brake); reverse and turn as strips of the same outline (the box truck's
        # rear lamps are on its box: their glows come in glows_extra)
        if getattr(S, 'TAIL_GLOWS', True):
            tl = resample(S.TAIL, 72, 0.0075)
            g = glow_patch(bvh, tl, ts, 0.0047, (1.0, 0.86, 0.72), GLOW_CELLS['tail'], inner='ring')
            if g:
                out.append(g)
            g = glow_patch(bvh, tl, ts, 0.0047, (0.72, 0.5, 0.3, 0.12), GLOW_CELLS['brake'])
            if g:
                out.append(g)
            rev = [(ph, zz) for ph, zz in S.TAIL if ph <= S.TAIL_REV + 2]
            if len(rev) >= 3:
                rev = resample(rev, 24, 0.0075)
                g = glow_patch(bvh, rev, ts, 0.0055, (1.0, 0.7, 0.4), GLOW_CELLS['reverse'])
                if g:
                    out.append(g)
            y0, z0, y1, z1 = S.MARK_R
            mk = [(y, z) for y, z in core.rounded_rect(y0, z0, y1, z1, 0.011, 3)]
            g = glow_patch(bvh, mk, side_ray(sx), 0.0012, (1.0, 0.5), GLOW_CELLS[turn])
            if g:
                out.append(g)
        y0, z0, y1, z1 = S.MARK_F
        mk = [(y, z) for y, z in core.rounded_rect(y0, z0, y1, z1, 0.012, 3)]
        g = glow_patch(bvh, mk, side_ray(sx), 0.0012, (1.0, 0.5), GLOW_CELLS[turn])
        if g:
            out.append(g)
    if getattr(S, 'TAXI', False):
        # the Cab's cell 7 is its TAXI roof light: no dome film
        out += glows_extra
        obj = parts.glow_object(out, f'{S.name}_GlowLamps')
        obj.data.materials[0].use_backface_culling = True
        log(f'{obj.name} tris {tri_count(obj)}')
        return obj
    # the dome: the side glass and the windshield, a film 1.2 mm outside
    me = gh.data
    gi = next(i for i, s in enumerate(gh.material_slots) if s.material and s.material.name.endswith('_glass'))
    bm = bmesh.new()
    bm.from_mesh(me)
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if f.material_index != gi or f.calc_area() < 1e-5], context='FACES')
    for v in bm.verts:
        v.co += v.normal * 0.0012
    uvl = bm.loops.layers.uv.get('UVMap') or bm.loops.layers.uv.new('UVMap')
    cell = GLOW_CELLS['dome']
    u0, v0 = (cell % 4) * 0.25, (cell // 4) * 0.5
    for f in bm.faces:
        for lp in f.loops:
            co = lp.vert.co
            lp[uvl].uv = (u0 + 0.25 * (0.02 + 0.96 * min(max((co.y - S.GH_Y0) / (S.GH_Y1 - S.GH_Y0), 0), 1)), v0 + 0.5 * (0.02 + 0.96 * min(max((co.z - 0.8) / 0.9, 0), 1)))
    dome = core.mesh_object('dome', bm)
    out.append(dome)
    out += glows_extra
    obj = parts.glow_object(out, f'{S.name}_GlowLamps')
    obj.data.materials[0].use_backface_culling = True
    log(f'{obj.name} tris {tri_count(obj)}')
    return obj


# ================================================================== assembly

LAMP_REST = SD.LAMP_REST
LAMP_STRIPS = SD.LAMP_STRIPS


def lamp_lens(S, mats, side=0):
    idx = index_of(mats)
    head_i, red_i = idx[mat_of(mats['head']).name], idx[mat_of(mats['red']).name]
    hz = S.HEAD_RANGE[2] - 0.05
    tz = S.TAIL_RANGE[2] - 0.05

    def test(f):
        c = f.calc_center_median()
        if side and c.x * side < 0.2:
            return False
        return (f.material_index == head_i and c.z > hz and c.y < S.Y_F + 0.6) or (f.material_index == red_i and c.z > tz and c.y > S.Y_R - 0.8)
    return test


def lamp_uvs(S, obj, mats):
    me = obj.data
    for key, front in (('head', True), ('red', False)):
        size = TEXTURE_SIZES[mname(S, key)]
        for side, rect in LAMP_STRIPS.items():
            test = lamp_lens(S, mats, side)
            core.select_only([obj])
            bpy.ops.object.mode_set(mode='EDIT')
            bpy.ops.mesh.select_mode(type='FACE')
            bm = bmesh.from_edit_mesh(me)
            for f in bm.faces:
                c = f.calc_center_median()
                f.select_set(test(f) and (c.y < 0) == front)
            bmesh.update_edit_mesh(me)
            bpy.ops.uv.unwrap(method='ANGLE_BASED', margin=0.0, correct_aspect=False)
            bm = bmesh.from_edit_mesh(me)
            uv = bm.loops.layers.uv.verify()
            loops = [lp for f in bm.faces if f.select for lp in f.loops]
            if loops:
                pts = [lp[uv].uv.copy() for lp in loops]
                cx = sum(p.x for p in pts) / len(pts)
                cy = sum(p.y for p in pts) / len(pts)
                sxx = sum((p.x - cx) ** 2 for p in pts)
                syy = sum((p.y - cy) ** 2 for p in pts)
                sxy = sum((p.x - cx) * (p.y - cy) for p in pts)
                ang = -0.5 * math.atan2(2 * sxy, sxx - syy)
                ca, sa = math.cos(ang), math.sin(ang)
                rot = [((p.x - cx) * ca - (p.y - cy) * sa, (p.x - cx) * sa + (p.y - cy) * ca) for p in pts]
                a0, a1 = min(p[0] for p in rot), max(p[0] for p in rot)
                b0, b1 = min(p[1] for p in rot), max(p[1] for p in rot)
                W, Hh = size
                u0, v0, u1, v1 = rect
                s = min((u1 - u0) * W / max(a1 - a0, 1e-9), (v1 - v0) * Hh / max(b1 - b0, 1e-9))
                ox = u0 * W + ((u1 - u0) * W - (a1 - a0) * s) / 2
                oy = v0 * Hh + ((v1 - v0) * Hh - (b1 - b0) * s) / 2
                for lp, (a, b) in zip(loops, rot):
                    lp[uv].uv = ((ox + (a - a0) * s) / W, (oy + (b - b0) * s) / Hh)
                bmesh.update_edit_mesh(me)
            bpy.ops.object.mode_set(mode='OBJECT')


def finish_body(S, objs, mats):
    for o in objs:
        if not o.data.uv_layers:
            o.data.uv_layers.new(name='UVMap')
    obj = core.join(objs, S.name)
    obj.name = obj.data.name = S.name
    from ns_ground import order_slots
    order_slots(obj, [mat_of(m) for m in order_list(mats)], TAG)
    core.finish_hard_surface(obj, 38.0)
    idx = index_of(mats)
    lock = obj.data.attributes.get('ns_uvlock')
    locked = set()
    if lock is not None:
        vals = np.zeros(len(obj.data.polygons), dtype=np.float32)
        lock.data.foreach_get('value', vals)
        locked = set(np.nonzero(vals > 0.5)[0].tolist())
    groups = []
    if locked:
        groups.append((lambda f: f.index in locked, 'keep', None, (0.0, 0.0, 1.0, 1.0)))
    groups += [(lamp_lens(S, mats), 'keep', None, (0.0, 0.0, 0.01, 0.01)),
               (lambda f: f.material_index in (idx[mat_of(mats['head']).name], idx[mat_of(mats['red']).name]), 'box', None, (0.0, 0.0, 1.0, LAMP_REST))]
    groups += [(lambda f, i=i: f.material_index == i, 'box', None, (0.0, 0.0, 1.0, 1.0)) for i in range(len(obj.material_slots))]
    groups.append((lambda f: True, 'box', None, (0.0, 0.0, 1.0, 1.0)))
    core.uv_layout(obj, groups)
    lamp_uvs(S, obj, mats)
    for name in ('span', 'band', 'station', 'ns_uvlock'):
        if name in obj.data.attributes:
            obj.data.attributes.remove(obj.data.attributes[name])
    return obj


def build_car(S):
    t0 = time.time()
    mats = body_materials(S)
    mats['head'] = core.Mat(tex(S, 'head'))      # placeholders: replaced once the body is measured
    mats['red'] = core.Mat(tex(S, 'red'))
    out = []
    body, bvh0 = body_shell(S, mats)
    pickup = getattr(S, 'PICKUP', False)
    out.append((body, None))
    gh, drip, stations = greenhouse(S, mats)
    out.append((gh, None))
    dg = bpy.context.evaluated_depsgraph_get()
    bvh = BVHTree.FromObject(body, dg)
    gh_bvh = BVHTree.FromObject(gh, dg)
    if pickup:
        # the bed is cut after the body is measured: the tail lamps are found by rays from TAIL_AXIS, which lies inside
        # the bed, so measured on the cut body they landed on the bed's inner walls (no tail lamps from outside)
        pickup_bed_cut(S, body, mats)
    # the lamps' shading, measured on the body: build the real materials and swap them into the slots
    head = headlamp_material(S, bvh)
    red = taillamp_material(S, bvh)
    for o in (body, gh):
        for i, s in enumerate(o.material_slots):
            if s.material == mats['head'].m:
                o.material_slots[i].material = head.m
            if s.material == mats['red'].m:
                o.material_slots[i].material = red.m
    bpy.data.materials.remove(mats['head'].m)
    bpy.data.materials.remove(mats['red'].m)
    mats['head'], mats['red'] = head, red
    head.m.name = mname(S, 'head')
    red.m.name = mname(S, 'red')
    out += lamps(S, bvh, bvh0, mats)
    out += front_end(S, bvh0, mats)
    out += pickup_parts(S, bvh, mats) if pickup else rear_end(S, bvh, mats)
    out += side_details(S, bvh, gh_bvh, mats)
    out += window_trim(S, gh_bvh, bvh, drip, stations, mats)
    out += cowl_and_wipers(S, bvh, gh_bvh, mats)
    out += calipers(S, mats)
    extra_glows = []
    if getattr(S, 'TAXI', False):
        out += taxi_sign(S, mats, gh_bvh, extra_glows)
    glow_obj = glow_lamps(S, bvh, gh, extra_glows)
    for o, m in out:
        if m is not None:
            core.assign(o, m)
    obj = finish_body(S, [o for o, _ in out], mats)
    show_cabin(S, mats['glass'], bake_cabin(S, obj, mats))
    wm = wheel_materials(S, tire_text(S))
    wheel = wheel_mesh(S, wm)
    log(f'{S.name}: tris {tri_count(obj)}, built in {time.time() - t0:.0f}s; slots {[s.material.name for s in obj.material_slots]}')
    return [obj, wheel, glow_obj]


def tire_text(S):
    s = Sheet(1000, 46)
    s.rect(0, 0, 1000, 46, 0x000000)
    a, b, c = S.TIRE_TEXT
    s.text(a, 170, 14, 21, 0xffffff, face='Black', align='CENTER', tracking=1.25)
    s.text(b, 470, 17, 13, 0xffffff, face='Bold', align='CENTER', tracking=1.15)
    s.text(c, 760, 17, 13, 0xffffff, face='Bold', align='CENTER', tracking=1.1)
    s.rect(330, 21, 20, 2.5, 0xffffff)
    s.rect(605, 21, 20, 2.5, 0xffffff)
    s.text('M+S', 920, 18, 10, 0xffffff, face='Bold', align='CENTER')
    return s.render(f'{S.tag}_tire_text', 4096)[0]


# ================================================================== the van (one loft: a high-roof box on a short-nosed cab)

class Van(Spec):
    name = 'SM_NS_Van'
    tag = 'ncv'
    Y_F, Y_R = -2.95, 2.95
    AXLES = (-1.86, 1.80)
    TRACK = 0.865
    WZ = 0.345
    TIRE_R = 0.352
    TIRE_W = 0.235
    ARCH_R = 0.41
    ZB = pchip([(-2.95, 0.36), (-2.7, 0.33), (-2.2, 0.30), (2.2, 0.30), (2.7, 0.38), (2.95, 0.42)])
    ZTOP = pchip([(-2.95, 1.0), (-2.86, 1.05), (-2.6, 1.10), (-2.3, 1.16), (-2.12, 1.21), (-1.95, 1.42), (-1.75, 1.68), (-1.55, 1.92),
                  (-1.42, 2.02), (-1.25, 2.25), (-1.05, 2.44), (-0.8, 2.52), (0.0, 2.55), (2.8, 2.55), (2.95, 2.52)])
    WS = pchip([(-2.95, 0.93), (-2.85, 0.975), (-2.6, 0.995), (-2.0, 1.005), (2.8, 1.005), (2.95, 0.995)])
    WT = pchip([(-2.95, 0.80), (-2.6, 0.86), (-2.15, 0.90), (-1.5, 0.93), (-0.9, 0.95), (2.95, 0.95)])
    RT = pchip([(-2.95, 0.10), (-2.15, 0.12), (-1.5, 0.1), (-0.9, 0.16), (2.95, 0.16)])
    BELT_Z = 1.22
    ZS = pchip([(-2.95, 1.0), (-2.2, 1.1), (-1.0, 1.15), (2.95, 1.15)])
    GH_Y0, GH_Y1 = -2.12, -1.15
    Y_HDR = -1.5
    GRILLE = SD.rrect(0.0, 0.82, 0.42, 0.11, 0.03)
    GRILLE_Z = 0.82
    GRILLE_BARS = (0.9, 0.855, 0.81, 0.765)
    GRILLE_HW = 0.42
    INTAKE = SD.rrect(0.0, 0.48, 0.5, 0.05, 0.03)
    INTAKE_SLATS = ()
    PLATE_ON_BUMPER = (-3.081, 0.5)
    INTAKE_HW = 0.48
    FOG = []
    FOG_R = 0.035
    PLATE_F = (0.0, 0.62)
    PLATE_R = (0.0, 0.55)
    HEAD_AXIS = (0.5, -2.62, 0.92)
    HEAD = [(14, 0.83), (14, 0.99), (24, 1.0), (36, 1.0), (48, 0.995), (60, 0.985), (66, 0.965), (66, 0.88), (58, 0.85), (44, 0.835), (28, 0.828)]
    HEAD_PJ = (24, 0.925, 0.042)
    HEAD_HB = (38, 0.925, 0.04)
    HEAD_AMBER = 56.0
    HEAD_POS = 48.0
    HEAD_RANGE = (10.0, 70.0, 0.8, 1.02)
    TAIL_AXIS = (0.88, 2.75, 1.2)
    # on the rear pillars outboard of the doors (x 0.91 to the corner, phi 32: past it the side runs along the rays and a lens tears)
    TAIL = [(9, 0.80), (9, 1.62), (16, 1.64), (25, 1.62), (30, 1.57), (32, 1.2), (30, 0.84), (22, 0.8), (14, 0.79)]
    TAIL_E = ((18, 1.40, 0.04), (18, 1.18, 0.04))
    TAIL_REV = 12.0
    TAIL_LOW = 0.98
    TAIL_SMOKE = 1.58
    TAIL_RANGE = (5.0, 36.0, 0.75, 1.68)
    MARK_F = (-2.62, 0.78, -2.54, 0.805)
    MARK_R = (2.6, 0.78, 2.68, 0.805)
    REFLECT = (0.6, 0.47, 0.07, 0.011)
    EXHAUST = (-0.58, 0.33)
    HANDLES = (-1.25, 0.1)
    TRIM_CHROME = False
    GARNISH_Z = None
    EMBLEM_R_Z = 1.95
    SHUT_SIDE = [
        [(-1.18, 2.30), (-1.20, 1.25), (-1.22, 0.62), (-1.24, 0.36)],               # cab door's rear edge (B pillar)
        [(-2.06, 1.30), (-2.07, 0.95), (-2.10, 0.62), (-2.15, 0.40)],               # cab door's front edge
        [(-2.15, 0.40), (-1.24, 0.36)],
        [(-0.95, 2.22), (0.55, 2.22), (0.56, 0.36), (-0.96, 0.36), (-0.95, 2.22)],  # the sliding door
        [(0.0, 2.29), (1.2, 2.29)],                                                  # its track over the window line
        [(2.86, 2.4), (2.88, 0.45)],                                                 # the rear doors' hinge side
    ]
    FUEL = (-0.6, 0.86, -0.45, 0.98)
    HOOD_FRONT = [(-0.8, 1.04), (-0.4, 1.05), (0.0, 1.052), (0.4, 1.05), (0.8, 1.04)]
    HOOD_SIDES = [(0.82, None), (0.86, -2.4), (0.84, -2.8), (0.72, -2.92), (0.0, -2.95)]
    REAR_LID = [(0.0, 0.48), (0.0, 2.42), (-0.9, 2.42), (-0.9, 0.48), (0.0, 0.48), (0.9, 0.48), (0.9, 2.42), (0.0, 2.42)]
    TRUNK = False
    CABIN = dict(seat_f=-1.55, seat_r=-0.9, floor=0.55, dash=-2.05, rear_shelf=-0.8, roof=2.2, width=0.86, hatch=False)
    PAINT = 0xeceeee
    PAINT2 = 0xf2f3f3
    WHEEL = 'steel'
    TIRE_TEXT = ('NORTHLINE', 'CARGO LT', 'LT235/65R16  121R')
    PLATE_TEXT = 'NC 51807'
    VAN = True


def van_half(S, y):
    """The right half of the van's section at y, bottom center to top center."""
    zb, zt, ws, wt, rt = S.ZB(y), S.ZTOP(y), S.WS(y), S.WT(y), S.RT(y)
    zt = max(zt, zb + 0.3)
    crown = 0.025
    zbelt = min(S.BELT_Z, zt - rt - 0.05)
    return [(0.0, zb), (0.6 * ws, zb), (ws - 0.05, zb + 0.008), (ws - 0.006, zb + 0.06), (ws, zb + 0.3 * (zbelt - zb)), (ws - 0.004, zbelt),
            (wt + (ws - wt) * 0.25, zt - rt), (wt - rt * 0.55, zt - 0.02), (0.5 * wt, zt + crown * 0.75), (0.0, zt + crown)]


VAN_SPANS = [4, 4, 4, 6, 6, 10, 6, 4, 6]       # samples per span of the half section (smooth corners at 1-5 m)


def van_stations(S):
    end = [0.0, 0.004, 0.012, 0.025, 0.045, 0.07, 0.10]
    front = [S.Y_F + d for d in end]
    rear = [S.Y_R - d for d in end]
    a, b = front[-1], rear[-1]
    n = int(round((b - a) / 0.03))
    return front + [a + (b - a) * i / n for i in range(1, n)] + list(reversed(rear))


def van_body(S, mats):
    rings = []
    for y in van_stations(S):
        rings.append([Vector((x, y, z)) for x, z in SD.mirrored_loop(van_half(S, y), VAN_SPANS)])
    body = loft_spans('van_body', rings, VAN_SPANS, mats)
    idx = index_of(mats)
    I = {k: idx[mat_of(mats[k]).name] for k in ('trim', 'glass', 'paint')}
    bvh0 = BVHTree.FromObject(body, bpy.context.evaluated_depsgraph_get())
    cuts = []
    for ay in S.AXLES:
        for sx in (-1, 1):
            cuts.append(parts.rod('arch_cut', (sx * 0.6, ay, S.WZ + 0.004), (sx * 1.3, ay, S.WZ + 0.004), S.ARCH_R, 72))
    cuts.append(SD.prism_y('grille_cut', S.GRILLE, S.Y_F - 0.2, S.Y_F + 0.06))
    for c in cuts:
        core.assign(c, mat_of(mats['trim']))
        core.boolean(body, c)
    # the cab's door windows get clean edges: the sides are cut along the window's outline before faces are assigned by center
    bm = bmesh.new()
    bm.from_mesh(body.data)
    sides = [fc for fc in bm.faces if abs(fc.normal.x) > 0.5 and -2.2 < fc.calc_center_median().y < -1.0 and fc.calc_center_median().z > 1.1]
    geom = list(set(sides) | {e for fc in sides for e in fc.edges} | {v for fc in sides for v in fc.verts})
    for co, no in (((0.0, 0.0, 1.26), (0.0, 0.0, 1.0)), ((0.0, 0.0, 1.97), (0.0, 0.0, 1.0)), ((0.0, -1.26, 0.0), (0.0, 1.0, 0.0)),
                   ((0.0, -2.0, 0.0), (0.0, 1.0, 0.0)), ((0.0, -1.72, 1.97), (0.0, -0.55, 1.0))):
        res = bmesh.ops.bisect_plane(bm, geom=geom, plane_co=co, plane_no=no)
        geom = res['geom']
    bm.to_mesh(body.data)
    bm.free()
    # glass: the windshield between the A pillars, the cab's door windows; black underneath
    for p in body.data.polygons:
        c, n = p.center, p.normal
        if n.z < -0.8 and c.z < 0.5:
            p.material_index = I['trim']
            continue
        if -2.10 < c.y < -1.49 and n.z > 0.25 and n.y < -0.2 and abs(c.x) < S.WT(c.y) - 0.09:
            p.material_index = I['glass']
        elif abs(n.x) > 0.75 and -2.0 < c.y < -1.26 and 1.26 < c.z < 1.97 - 0.55 * max(0.0, (-1.72 - c.y)):
            p.material_index = I['glass']
    # the glass sits back in black seals
    bm = bmesh.new()
    bm.from_mesh(body.data)
    gf = [f for f in bm.faces if f.material_index == I['glass']]
    res = bmesh.ops.inset_region(bm, faces=gf, thickness=0.012, depth=-0.008, use_even_offset=True, use_boundary=True)
    for f in res['faces']:
        f.material_index = I['trim']
    bm.to_mesh(body.data)
    bm.free()
    return body, bvh0


def van_parts(S, bvh, mats):
    """Bumpers, the side rub strip, arch flares, the rear doors' windows and hinges, the sliding door's handle and track,
    mirrors on arms, the roof's marker lamps, the step."""
    out = []
    # front bumper: a black plastic wrap round the nose's foot
    fb = parts.rbox('fbumper', (0, S.Y_F - 0.02, 0.5), (2.0, 0.22, 0.32), 0.07, 4)
    out.append((fb, mats['trim']))
    cutaway = getattr(S, 'CUTAWAY', False)        # the box truck's cab: no rear doors, bumper or roof markers of its own
    # rear step bumper: a black bumper with a ribbed steel step in the middle
    if not cutaway:
        out.append((parts.rbox('rbumper', (0, S.Y_R + 0.03, 0.48), (2.0, 0.2, 0.2), 0.05, 3), mats['trim']))
        out.append((parts.rbox('step', (0, S.Y_R + 0.12, 0.52), (0.7, 0.18, 0.04), 0.01, 2), mats['chrome']))
    # the rub strip along each side, at z 0.78 (cut short of the wheels' arches)
    for sx in (-1, 1):
        runs = ((S.AXLES[0] + S.ARCH_R + 0.05, S.AXLES[1] - S.ARCH_R - 0.05), (S.AXLES[1] + S.ARCH_R + 0.05, S.Y_R - 0.08),
                (S.Y_F + 0.25, S.AXLES[0] - S.ARCH_R - 0.05))
        if cutaway:
            runs = ((S.AXLES[0] + S.ARCH_R + 0.05, S.Y_R - 0.06), (S.Y_F + 0.25, S.AXLES[0] - S.ARCH_R - 0.05))
        for (y0, y1) in runs:
            pts, nrms = [], []
            for k in range(20):
                y = y0 + (y1 - y0) * k / 19
                p, nr = hit(bvh, (sx * 2.0, y, 0.78), (-sx, 0, 0))
                if p is not None:
                    pts.append(p)
                    nrms.append(nr)
            if len(pts) > 2:
                out.append((ribbon('rub', pts, nrms, [(-0.035, -0.004), (-0.035, 0.008), (-0.03, 0.012), (0.03, 0.012), (0.035, 0.008), (0.035, -0.004)],
                                   sides=[(0, 0, 1)] * len(pts)), mats['trim']))
        # arch flares
        for ay in S.AXLES:
            pts, nrms, sides = [], [], []
            for q in range(49):
                th = math.radians(-8 + 196 * q / 48)
                y, z = ay + S.ARCH_R * math.cos(th), S.WZ + 0.004 + S.ARCH_R * math.sin(th)
                if z < S.ZB(y) + 0.03:
                    continue
                pr = Vector((0.0, math.cos(th), math.sin(th)))
                p, nr = hit(bvh, (sx * 2.0, y + pr.y * 0.03, z + pr.z * 0.03), (-sx, 0, 0))
                if p is None:
                    continue
                pts.append(Vector((p.x, y, z)))
                nrms.append(Vector((nr.x, 0.0, nr.z * 0.5)))
                sides.append(pr)
            if len(pts) > 3:
                out.append((ribbon('flare', pts, nrms, [(-0.008, -0.012), (-0.006, 0.0), (0.0, 0.01), (0.03, 0.013), (0.05, 0.008), (0.058, -0.012)],
                                   sides=sides), mats['trim']))
        # the mirror: a tall black housing on two tubes off the A pillar's foot
        base = Vector((sx * 0.99, -2.0, 1.25))
        head_c = Vector((sx * getattr(S, 'MIRROR_X', 1.17), -2.05, 1.38))
        out.append((parts.rod('m_arm', base, head_c + Vector((-sx * 0.04, 0, -0.1)), 0.012, 10), mats['trim']))
        out.append((parts.rod('m_arm2', base + Vector((0, 0.02, 0.18)), head_c + Vector((-sx * 0.04, 0, 0.1)), 0.012, 10), mats['trim']))
        out.append((parts.rbox('m_head', head_c, (0.06, 0.12, 0.34), 0.025, 3), mats['trim']))
        mg = parts.rbox('m_glass', head_c + Vector((0, 0.061, 0.0)), (0.05, 0.004, 0.31), 0.01, 2)
        out.append((mg, mats['chrome']))
        # the cab door's handle and the sliding door's
        for hy, hz in ((-1.36, 1.12), (0.42, 1.08)):
            p, nr = hit(bvh, (sx * 2.0, hy, hz), (-sx, 0, 0))
            if p is not None:
                h = parts.rbox('handle', p + nr * 0.012, (0.03, 0.17, 0.035), 0.012)
                out.append((h, mats['trim']))
        # amber side markers along the box at the roofline and the waist
        for my in (-0.4, 1.0, 2.4):
            p, nr = hit(bvh, (sx * 2.0, my, 2.2), (-sx, 0, 0))
            if p is not None:
                out.append((parts.rbox('marker', p + nr * 0.008, (0.016, 0.08, 0.03), 0.006), mats['amber']))
    if cutaway:
        return out + van_wipers_cowl(S, bvh, mats)
    # the rear doors: two windows framed in black rubber, hinges, a handle
    yr = S.Y_R + 0.002
    for sx in (-1, 1):
        x0, x1 = sorted((sx * 0.1, sx * 0.82))
        out.append((parts.rbox('rw_seal', ((x0 + x1) / 2, yr + 0.004, 1.82), (x1 - x0 + 0.04, 0.012, 0.66), 0.03, 3), mats['trim']))
        gl = parts.rbox('rw_glass', ((x0 + x1) / 2, yr + 0.009, 1.82), (x1 - x0 - 0.01, 0.006, 0.61), 0.025, 2)
        out.append((gl, mats['glass']))
        for hz in (0.75, 1.5, 2.2):
            out.append((parts.rbox('hinge', (sx * 0.855, yr + 0.01, hz), (0.07, 0.03, 0.09), 0.008), mats['trim']))
    out.append((parts.rbox('r_handle', (0.18, yr + 0.02, 1.1), (0.16, 0.03, 0.035), 0.012), mats['trim']))
    # the third brake lamp at the top middle
    out.append((parts.rbox('cbl', (0, yr + 0.01, 2.47), (0.4, 0.03, 0.05), 0.012), mats['red']))
    return out + van_wipers_cowl(S, bvh, mats)


def van_wipers_cowl(S, bvh, mats):
    out = []
    # wipers at the foot of the windshield
    for x0, x1 in ((-0.05, 0.75), (-0.82, -0.12)):
        pts, nrms = [], []
        for k in range(12):
            x = x0 + (x1 - x0) * k / 11
            p, nr = hit(bvh, (x, -2.06, 4.0), (0, 0, -1))
            if p is not None:
                pts.append(p + nr * 0.015)
                nrms.append(nr)
        if len(pts) > 2:
            out.append((ribbon('wiper', pts, nrms, [(-0.008, -0.008), (0.008, -0.008), (0.006, 0.006), (-0.006, 0.006)]), mats['trim']))
    # the cowl: a black strip across the hood's back
    p, nr = hit(bvh, (0.0, -2.16, 4.0), (0, 0, -1))
    if p is not None:
        out.append((parts.rbox('cowl', (0, -2.16, p.z + 0.01), (1.62, 0.1, 0.03), 0.01), mats['trim']))
    return out


def van_livery(S):
    """KOWALCZYK PLUMBING & HEATING on the box's sides (white = ink: navy and red are told apart by grey levels), and on the
    rear doors."""
    out = {}
    for key in ('r', 'l'):
        s = Sheet(5000, 2600)
        s.rect(0, 0, 5000, 2600, 0x000000)

        def X(y):
            return (y + 2.5) * 1000 if key == 'r' else (2.5 - y) * 1000
        cx = X(1.35)
        s.text('KOWALCZYK', cx, 1750, 230, 0xffffff, 'Black', 'CENTER', tracking=1.02)
        s.text('PLUMBING & HEATING', cx, 1530, 120, 0x9a9a9a, 'Black', 'CENTER', tracking=1.05)
        s.rect(cx - 1050, 1470, 2100, 18, 0x9a9a9a)
        s.text('since 1972  ·  licensed & insured', cx, 1350, 70, 0xffffff, 'Bold', 'CENTER')
        s.text('555-0186', cx, 1130, 150, 0x9a9a9a, 'Black', 'CENTER')
        s.text('NO JOB TOO SMALL', cx, 980, 70, 0xffffff, 'Bold', 'CENTER', tracking=1.2)
        # a drop of water in a circle on the cab door
        dx = X(-1.62)
        s.ring(dx, 1000, 120, 140, 0xffffff)
        s.circle(dx, 980, 60, 0x9a9a9a)
        s.poly([(dx - 52, 1010), (dx + 52, 1010), (dx, 1110)], 0x9a9a9a)
        s.text('KOWALCZYK', dx, 800, 55, 0xffffff, 'Black', 'CENTER')
        out[key] = s.render(f'{S.tag}_livery_{key}', 4096)[0]
    s = Sheet(2000, 2600)
    s.rect(0, 0, 2000, 2600, 0x000000)
    # the rear doors: under the handle (z 1.08..1.12) and over the step bumper (its top z 0.58), across the doors' split
    s.text('KOWALCZYK', 1000, 935, 115, 0xffffff, 'Black', 'CENTER')
    s.text('PLUMBING & HEATING', 1000, 845, 58, 0x9a9a9a, 'Black', 'CENTER', tracking=1.04)
    s.text('555-0186', 1000, 700, 100, 0x9a9a9a, 'Black', 'CENTER')
    out['rear'] = s.render(f'{S.tag}_livery_rear', 2048)[0]
    return out


def van_paint(S, lines, livery):
    """The paint with the van's livery: navy (0x1d3157) for the white ink, red (0xb12a2a) for the grey."""
    m = paint_material(S, lines, None)
    nt = m.nt
    bsdf = m.bsdf
    src = bsdf.inputs['Base Color'].links[0].from_socket
    tc, sep = object_coords(m)
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    nx, ny, nz = SD.normal_parts(m)
    u_side, v_side = m.math('DIVIDE', m.math('ADD', y, 2.5), 5.0), m.math('DIVIDE', z, 2.6)
    lr = SD.sample_mask(m, livery['r'], u_side, v_side)
    ll = SD.sample_mask(m, livery['l'], m.math('SUBTRACT', 1.0, u_side), v_side)
    side = mix_float(m, m.math('GREATER_THAN', x, 0.0), ll, lr)
    side = m.math('MULTIPLY', side, ramp(m, m.math('ABSOLUTE', nx), 0.5, 0.7))
    rear = m.math('MULTIPLY', SD.sample_mask(m, livery['rear'], m.math('DIVIDE', m.math('SUBTRACT', 1.0, x), 2.0), v_side), ramp(m, ny, 0.5, 0.7))
    liv = m.math('MAXIMUM', side, rear)
    navy = ramp(m, liv, 0.8, 0.9)
    red = m.math('MULTIPLY', ramp(m, liv, 0.3, 0.45), m.math('SUBTRACT', 1.0, navy))
    col = m.mix(navy, src, H(0x1d3157))
    col = m.mix(red, col, H(0xb12a2a))
    m.link(col, bsdf.inputs['Base Color'])
    return m


def build_van():
    S = Van
    t0 = time.time()
    lines = line_sheets(S)
    mats = body_materials(S)
    bpy.data.materials.remove(mats['paint'].m)
    mats['paint'] = van_paint(S, lines, van_livery(S))
    mats['head'] = core.Mat(tex(S, 'head'))
    mats['red'] = core.Mat(tex(S, 'red'))
    out = []
    body, bvh0 = van_body(S, mats)
    out.append((body, None))
    bvh = BVHTree.FromObject(body, bpy.context.evaluated_depsgraph_get())
    head = headlamp_material(S, bvh)
    red = taillamp_material(S, bvh)
    for i, s in enumerate(body.material_slots):
        if s.material == mats['head'].m:
            body.material_slots[i].material = head.m
        if s.material == mats['red'].m:
            body.material_slots[i].material = red.m
    bpy.data.materials.remove(mats['head'].m)
    bpy.data.materials.remove(mats['red'].m)
    mats['head'], mats['red'] = head, red
    head.m.name = mname(S, 'head')
    red.m.name = mname(S, 'red')
    out += lamps(S, bvh, bvh0, mats)
    out += front_end(S, bvh0, mats)
    # the rear plate on the step bumper
    out.append((plate_quad('plate_r', (0, S.Y_R + 0.135, 0.48), 0.30, 0.15, 1, mats['plate']), None))
    out.append((parts.rbox('plate_back', (0, S.Y_R + 0.13, 0.48), (0.31, 0.006, 0.16), 0.002), mats['chrome']))
    out += van_parts(S, bvh, mats)
    out += calipers(S, mats)
    glow_obj = glow_lamps(S, bvh, body, [])
    for o, m in out:
        if m is not None:
            core.assign(o, m)
    obj = finish_body(S, [o for o, _ in out], mats)
    show_cabin(S, mats['glass'], bake_cabin(S, obj, mats))
    wm = wheel_materials(S, tire_text(S))
    wheel = wheel_mesh(S, wm)
    log(f'{S.name}: tris {tri_count(obj)}, built in {time.time() - t0:.0f}s; slots {[s.material.name for s in obj.material_slots]}')
    return [obj, wheel, glow_obj]


# ================================================================== the box truck (P1)

TRUCK_DY = -0.8     # the truck is built in the van's frame (its cab is the van's front, the nose at y -2.95) and moved by this


class BoxTruck(Spec):
    """The box truck in its final frame (the review, the docstring, the game)."""
    name = 'SM_NS_BoxTruck'
    tag = 'nct'
    Y_F, Y_R = -3.75, 3.75
    AXLES = (-2.66, 1.94)
    TRACK = 0.865          # the front wheels
    TRACK_R = 0.80         # the rear duals' center
    WZ = 0.362
    TIRE_R = 0.372
    TIRE_W = 0.225
    DUAL = True
    REVIEW_K = 1.75


class TruckBuild(Van):
    """The truck's numbers in the build frame (the van's): the cab is the van's front loft cut off behind its doors with a
    lower roof, the 16 ft box behind it on the cutaway chassis."""
    name = 'SM_NS_BoxTruck'
    tag = 'nct'
    Y_F, Y_R = -2.95, -0.75                    # the cab's loft: its back wall at -0.75
    BOX_Y0, BOX_Y1 = -0.62, 4.42               # the box (its rear frame's face); the rear bumper reaches 4.55
    BOX_X = 1.17
    BOX_Z0, BOX_Z1 = 1.0, 3.32
    AXLES = (-1.86, 2.74)
    TRACK = 0.865
    TRACK_R = 0.80
    WZ = 0.362
    TIRE_R = 0.372
    TIRE_W = 0.225
    ARCH_R = 0.43
    ZTOP = pchip([(-2.95, 1.0), (-2.86, 1.05), (-2.6, 1.10), (-2.3, 1.16), (-2.12, 1.21), (-1.95, 1.42), (-1.75, 1.68), (-1.55, 1.92),
                  (-1.42, 2.02), (-1.25, 2.14), (-1.1, 2.2), (-0.95, 2.235), (-0.75, 2.245)])
    TAIL_RANGE = (0.0, 46.0, 99.0, 99.5)       # no tail lenses on the cab: the truck's are on the box
    TAIL_GLOWS = False
    CUTAWAY = True
    MIRROR_X = 1.34                            # the mirrors reach past the box (2.34 m wide)
    SHUT_SIDE = Van.SHUT_SIDE[:3]
    REAR_LID = []
    FUEL = (-1.08, 0.70, -0.95, 0.80)
    CABIN = dict(seat_f=-1.55, seat_r=-1.0, floor=0.55, dash=-2.05, rear_shelf=-0.85, roof=2.15, width=0.86, hatch=False)
    PAINT = 0xeceeee
    PAINT2 = 0xf2f3f3
    WHEEL = 'truck'
    TIRE_TEXT = ('NORTHLINE', 'LT HAUL', 'LT225/75R16  115R')
    PLATE_TEXT = 'NT 40912'
    TEX = dict(Spec.TEX, chrome=1024)
    VAN = True


class TruckDual(TruckBuild):
    tag = 'nctd'


def truck_sheets(S):
    """The truck's prints in color on black (black = no ink): the box's sides (1 unit = 1 mm; y from the box's front, z from
    its floor), its roll-up door and its front over the cab (x across, seen from outside), and the cab doors' small lettering
    (in the Cab's side-sheet format: white = black ink)."""
    GREEN, RED, DARK, WHITE = 0x1f6b35, 0xb8322a, 0x3a3d40, 0xf4f4f0
    W = int((S.BOX_Y1 - S.BOX_Y0) * 1000)
    Hh = int((S.BOX_Z1 - S.BOX_Z0) * 1000)
    out = {}
    s = Sheet(W, Hh)
    s.rect(0, 0, W, Hh, 0x000000)
    # a green band along the foot with a red rule over it
    s.rect(0, 175, W, 85, GREEN)
    s.rect(0, 285, W, 22, RED)
    cx = W * 0.6
    s.text('NORTHSIDE', cx, 1230, 600, GREEN, 'Black', 'CENTER', tracking=1.02)
    s.text('PRODUCE SUPPLY', cx, 900, 245, RED, 'Black', 'CENTER', tracking=1.12)
    s.text('Fresh fruit & vegetables to the trade since 1958', cx, 680, 105, DARK, 'Bold', 'CENTER')
    s.text('555-0147   ·   212 CARDINAL AVE', cx, 480, 125, GREEN, 'Black', 'CENTER', tracking=1.05)
    # the produce: a slatted crate heaped with tomatoes, oranges and green apples, a bunch of carrots against it
    bx = 820
    s.rect(bx - 520, 560, 1040, 430, 0x8a5a2a)
    for zz in (690, 830):
        s.rect(bx - 520, zz, 1040, 18, 0x6a4220)
    s.rect(bx - 520, 960, 1040, 30, 0x6a4220)
    fruit = [(-360, 1060, 125, RED), (-130, 1085, 135, 0xe8862a), (110, 1070, 128, 0x7cab3a), (340, 1055, 122, RED),
             (-250, 1250, 120, 0x7cab3a), (0, 1275, 130, RED), (240, 1245, 118, 0xe8862a), (-110, 1440, 112, RED), (130, 1430, 110, 0x7cab3a)]
    for fx, fz, fr, fc in fruit:
        s.circle(bx + fx, fz, fr, fc, n=48)
        s.circle(bx + fx - fr * 0.35, fz + fr * 0.35, fr * 0.22, 0xf6f2e8 if fc != 0x7cab3a else 0xd8ecb0, n=20)   # a highlight
        if fc == RED:
            s.poly([(bx + fx - 40, fz + fr - 10), (bx + fx, fz + fr + 45), (bx + fx + 40, fz + fr - 10), (bx + fx, fz + fr + 10)], 0x2f7a2c)
    for k, ang in enumerate((-0.35, -0.1, 0.18)):
        tx, tz = bx + 560 + 60 * k, 600
        c_, s_ = math.cos(ang), math.sin(ang)
        tip = (tx + 620 * s_, tz + 620 * c_)
        s.poly([(tx - 45 * c_, tz + 45 * s_), (tx + 45 * c_, tz - 45 * s_), tip], 0xe07a1e)
        for lf in (-0.5, 0.0, 0.5):
            s.poly([(tip[0] - 15, tip[1]), (tip[0] + 15, tip[1]), (tip[0] + 260 * math.sin(ang + lf), tip[1] + 260 * math.cos(ang + lf))], 0x3f8a3a)
    out['side'] = s.render(f'{S.tag}_box_side', 4096)[0]
    X2 = int(2 * S.BOX_X * 1000)
    s = Sheet(X2, Hh)
    s.rect(0, 0, X2, Hh, 0x000000)
    c2 = X2 / 2
    s.text('NORTHSIDE', c2, 1420, 300, GREEN, 'Black', 'CENTER', tracking=1.02)
    s.text('PRODUCE SUPPLY', c2, 1250, 125, RED, 'Black', 'CENTER', tracking=1.1)
    s.text("HOW'S MY DRIVING?  555-0147", c2, 640, 78, DARK, 'Black', 'CENTER')
    s.text('UNIT 12', c2 + 760, 1820, 90, DARK, 'Black', 'CENTER')
    s.text('THIS VEHICLE MAKES FREQUENT STOPS', c2, 500, 52, DARK, 'Bold', 'CENTER', tracking=1.05)
    out['rear'] = s.render(f'{S.tag}_box_rear', 2048)[0]
    s = Sheet(X2, Hh)
    s.rect(0, 0, X2, Hh, 0x000000)
    s.text('NORTHSIDE PRODUCE', c2, 1680, 210, GREEN, 'Black', 'CENTER', tracking=1.04)
    s.rect(c2 - 900, 1610, 1800, 20, RED)
    out['front'] = s.render(f'{S.tag}_box_front', 2048)[0]
    for key in ('r', 'l'):
        s = Sheet(5000, 2600)
        s.rect(0, 0, 5000, 2600, 0x000000)

        def X(y):
            return (y + 2.5) * 1000 if key == 'r' else (2.5 - y) * 1000
        dx = X(-1.62)
        s.text('NORTHSIDE PRODUCE', dx, 905, 62, 0xffffff, 'Black', 'CENTER', tracking=1.05)
        s.text('UNIT 12', dx, 820, 52, 0xffffff, 'Black', 'CENTER')
        out[key] = s.render(f'{S.tag}_door_{key}', 4096)[0]
    return out


def truck_paint(S, lines, sheets):
    """The van's paint (white) with the cab doors' lettering, and the box's prints in their own colors over it."""
    m = paint_material(S, lines, {'r': sheets['r'], 'l': sheets['l']})
    bsdf = m.bsdf
    src = bsdf.inputs['Base Color'].links[0].from_socket
    tc, sep = object_coords(m)
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    nx, ny, nz = SD.normal_parts(m)
    X, y0, y1, z0, z1 = S.BOX_X, S.BOX_Y0, S.BOX_Y1, S.BOX_Z0, S.BOX_Z1
    v = m.math('DIVIDE', m.math('SUBTRACT', z, z0), z1 - z0)
    u_side = m.math('DIVIDE', m.math('SUBTRACT', y, y0), y1 - y0)
    # +X (the truck's left) reads front to back, -X back to front: each reads left to right from outside
    u_on = mix_float(m, m.math('GREATER_THAN', x, 0.0), m.math('SUBTRACT', 1.0, u_side), u_side)
    side_img = m.image(sheets['side'], vector=vec(m, u_on, v, 0.0), extension='CLIP')
    rear_img = m.image(sheets['rear'], vector=vec(m, m.math('DIVIDE', m.math('SUBTRACT', X, x), 2 * X), v, 0.0), extension='CLIP')
    front_img = m.image(sheets['front'], vector=vec(m, m.math('DIVIDE', m.math('ADD', x, X), 2 * X), v, 0.0), extension='CLIP')
    in_box = m.math('MULTIPLY', ramp(m, y, y0 - 0.02, y0 - 0.01), ramp(m, z, z0 + 0.16, z0 + 0.17))
    w_side = m.math('MULTIPLY', ramp(m, m.math('ABSOLUTE', nx), 0.6, 0.75), in_box)
    w_rear = m.math('MULTIPLY', ramp(m, ny, 0.6, 0.75), ramp(m, y, y1 - 0.09, y1 - 0.08))
    w_front = m.math('MULTIPLY', ramp(m, m.math('MULTIPLY', ny, -1.0), 0.6, 0.75),
                     m.math('MULTIPLY', ramp(m, y, y0 + 0.05, y0 + 0.04), ramp(m, z, 2.3, 2.32)))
    col = src
    for img, w in ((side_img, w_side), (rear_img, w_rear), (front_img, w_front)):
        sp = m.node('ShaderNodeSeparateColor')
        m.link(img.outputs['Color'], sp.inputs['Color'])
        mx = m.math('MAXIMUM', m.math('MAXIMUM', sp.outputs['Red'], sp.outputs['Green']), sp.outputs['Blue'])
        col = m.mix(m.math('MULTIPLY', ramp(m, mx, 0.004, 0.012), w), col, img.outputs['Color'])
    # road film creeping up the box's foot and a grey tide under the roof's edge
    film = m.math('MULTIPLY', m.math('MULTIPLY', ramp(m, z, z0 + 0.6, z0 + 0.15), ramp(m, noise(m, tc.outputs['Object'], 6.0, 4.0), 0.3, 0.75)), 0.5)
    film = m.math('MULTIPLY', film, ramp(m, y, y0 - 0.02, y0 - 0.01))     # the box's foot only: it had blotched the cab below 1.6 m
    col = m.mix(film, col, H(0x6a645a))
    m.link(col, bsdf.inputs['Base Color'])
    return m


def truck_wheel_obj(S, mats_w, name, dual):
    """A truck wheel at its hub, the axle along X, its outer face +X: a pressed steel disc with eight hand holes and eight lug
    nuts round a bearing cap on the 16 in. rim. dual: the rear pair, the outer wheel's mounting face at its inboard edge (a
    deep dish) and the inner wheel turned to face in, back to back 0.25 m apart; the hub's flange stands out of the dish."""
    tire_m, rim_m, rot_m = mats_w
    R = S.TIRE_R
    sr = R / 0.334
    sw = S.TIRE_W / 0.225
    rr = 0.197 * sr
    axle = Matrix.Rotation(math.radians(90), 4, 'Y')

    def at(r, ang, w):
        return Vector((w, r * math.cos(ang), r * math.sin(ang)))

    def one(mount):
        o = []
        tire = closed_lathe('tire', [(r * sr, a * sw) for r, a in SD.TIRE], axle, 72)
        core.assign(tire, tire_m)
        o.append(tire)
        rim = closed_lathe('rim', [(r * sr, a * sw) for r, a in SD.RIM], axle, 64)
        core.assign(rim, rim_m)
        o.append(rim)
        d = 0.01
        prof = [(0.0, mount + d), (0.105 * sr, mount + d), (0.145 * sr, mount + d + 0.022), (0.172 * sr, mount + d + 0.05), (rr - 0.004, mount + d + 0.066),
                (rr - 0.004, mount + 0.056), (0.168 * sr, mount + 0.04), (0.14 * sr, mount + 0.012), (0.10 * sr, mount), (0.0, mount)]
        disk = closed_lathe('disk', prof, axle, 64)
        for k in range(8):
            a = 2 * math.pi * (k + 0.5) / 8
            cut = parts.rbox('hole', (0, 0, 0), (0.3, 0.05 * sr, 0.034 * sr), 0.012)
            cut.data.transform(Matrix.Translation(at(0.158 * sr, a, mount + 0.03)) @ Matrix.Rotation(a, 4, 'X'))
            core.boolean(disk, cut)
        core.assign(disk, rim_m)
        o.append(disk)
        for k in range(8):
            a = 2 * math.pi * k / 8
            o.append(parts.rod('lug', at(0.0855, a, mount + d), at(0.0855, a, mount + d + 0.026), 0.0135, 6))
            core.assign(o[-1], rim_m)
            o.append(parts.rod('stud', at(0.0855, a, mount + d + 0.026), at(0.0855, a, mount + d + 0.036), 0.0085, 10))
            core.assign(o[-1], rim_m)
        return o
    out = []
    if not dual:
        out += one(-0.02)
        cap = closed_lathe('cap', [(0.0, 0.075), (0.05, 0.075), (0.058, 0.068), (0.064, 0.0), (0.0, 0.0)], axle, 32)
        core.assign(cap, rim_m)
        out.append(cap)
        rot = closed_lathe('rotor', [(0.0, 0.03), (0.08, 0.03), (0.082, 0.016), (0.150 * sr, 0.016), (0.152 * sr, 0.012), (0.152 * sr, -0.010),
                                     (0.150 * sr, -0.014), (0.07, -0.014), (0.0, -0.014)], axle, 40)
        core.assign(rot, rot_m)
        out.append(rot)
    else:
        off = 0.125
        outer = one(-0.105)
        for p in outer:
            p.data.transform(Matrix.Translation((off, 0, 0)))
        inner = one(-0.105)
        for p in inner:
            p.data.transform(Matrix.Translation((-off, 0, 0)) @ Matrix.Rotation(math.pi, 4, 'Z'))
        out += outer + inner
        # the hub: the axle's flange and the bearing cap standing out of the outer dish; the brake drum between the wheels
        hub = closed_lathe('hub', [(0.0, 0.11), (0.052, 0.11), (0.06, 0.1), (0.066, 0.05), (0.095, 0.045), (0.098, 0.03), (0.0, 0.03)], axle, 40)
        core.assign(hub, rim_m)
        out.append(hub)
        drum = closed_lathe('drum', [(0.0, -0.06), (0.17, -0.06), (0.18, -0.05), (0.18, 0.012), (0.17, 0.018), (0.0, 0.018)], axle, 48)
        core.assign(drum, rot_m)
        out.append(drum)
    for p in out:
        for v in p.data.vertices:
            if v.co.z < -S.WZ:
                v.co.z = -S.WZ                       # the tires' contact patch, flat on the road
    obj = core.join(out, name)
    obj.name = obj.data.name = name
    core.finish_hard_surface(obj, 38.0)
    ti = next(i for i, s in enumerate(obj.material_slots) if s.material == mat_of(tire_m))
    xo = 0.125 if dual else 0.0

    def outer_wall(f):
        c = f.calc_center_median()
        return f.material_index == ti and c.x > xo + 0.05 and f.normal.x > 0.5

    groups = [(outer_wall, 'box', None, (0.0, 0.36, 1.0, 1.0)), (lambda f: f.material_index == ti, 'box', None, (0.0, 0.0, 1.0, 0.35))]
    groups += [(lambda f, i=i: f.material_index == i, 'box', None, (0.0, 0.0, 1.0, 1.0)) for i in range(len(obj.material_slots))]
    core.uv_layout(obj, groups)
    from ns_ground import order_slots
    order_slots(obj, [mat_of(tire_m), mat_of(rim_m), mat_of(rot_m)], TAG)
    log(f'{obj.name} tris {tri_count(obj)}')
    return obj


def glow_quad(c, w, h, normal, cell, lift=0.002):
    """A flat glow face w x h centered at c facing normal ((nx, ny): one of +-X, +-Y), filling its Signal-grid cell as seen."""
    nx, ny = normal
    R = Vector((-ny, 0.0, 0.0)) if abs(ny) > 0.5 else Vector((0.0, nx, 0.0))      # the viewer's right
    U = Vector((0.0, 0.0, 1.0))
    N = Vector((nx, ny, 0.0))
    c = Vector(c) + N * lift
    pts = [c - R * w / 2 - U * h / 2, c + R * w / 2 - U * h / 2, c + R * w / 2 + U * h / 2, c - R * w / 2 + U * h / 2]
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new('UVMap')
    f = bm.faces.new([bm.verts.new(p) for p in pts])
    u0, v0 = (cell % 4) * 0.25, (cell // 4) * 0.5
    for lp, (a, b) in zip(f.loops, ((0, 0), (1, 0), (1, 1), (0, 1))):
        lp[uvl].uv = (u0 + 0.25 * (0.02 + 0.96 * a), v0 + 0.5 * (0.02 + 0.96 * b))
    o = core.mesh_object('glow', bm)
    core.orient_normals(o, tuple(N))
    return o


def truck_box(S, mats, glows):
    """The 16 ft box: FRP panels (the paint) on aluminum corner posts and top and bottom rails, a rub rail; the rear frame
    (header, posts, sill) round an eight-panel roll-up door with its hinges, handle, hasp and pull strap; the lamps (tail,
    stop, turn and reverse on the rear posts, three ID lamps and two clearance lamps on the header, amber clearance at the
    front's top corners, side markers on the bottom rails); under it the floor's crossmembers, the cutaway's frame rails, the
    rear axle and springs, mud flaps, the battery box, the exhaust; the rear bumper with its step and the plate."""
    X, y0, y1, z0, z1 = S.BOX_X, S.BOX_Y0, S.BOX_Y1, S.BOX_Z0, S.BOX_Z1
    alu, blk, paint = mats['chrome'], mats['trim'], mats['paint']
    red, amber, white = mats['red'], mats['amber'], mats['head']
    out = []

    def rb(name, c, size, r, m, seg=2):
        out.append((parts.rbox(name, c, size, r, seg), m))
    zc, H_ = (z0 + z1) / 2, z1 - z0
    yd = y1 - 0.045                                         # the door's face plane (the frame stands 4.5 cm proud of it)
    rb('box', (0, (y0 + yd) / 2, zc), (2 * X - 0.004, yd - y0, H_ - 0.004), 0.02, paint, 3)
    for sx in (-1, 1):
        # corner posts: the front ones square, the rear ones the door frame's jambs
        rb('post_f', (sx * (X - 0.032), y0 + 0.032, zc), (0.072, 0.072, H_ + 0.008), 0.012, alu)
        rb('post_r', (sx * (X - 0.075), y1 - 0.033, zc), (0.158, 0.068, H_ + 0.008), 0.012, alu)
        # the side rails
        rb('rail_top', (sx * (X + 0.004), (y0 + y1) / 2, z1 - 0.05), (0.03, y1 - y0 - 0.06, 0.1), 0.008, alu)
        rb('rail_bot', (sx * (X + 0.006), (y0 + y1) / 2, z0 + 0.08), (0.036, y1 - y0 - 0.06, 0.16), 0.008, alu)
        rb('rub_rail', (sx * (X + 0.026), (y0 + y1) / 2, z0 + 0.075), (0.022, y1 - y0 - 0.3, 0.05), 0.009, alu)
        # side markers: amber at the front, red at the rear (on the bottom rail)
        rb('marker_f', (sx * (X + 0.026), y0 + 0.2, z0 + 0.135), (0.016, 0.1, 0.035), 0.008, amber)
        rb('marker_r', (sx * (X + 0.026), y1 - 0.25, z0 + 0.135), (0.016, 0.1, 0.035), 0.008, red)
        glows.append(glow_quad((sx * (X + 0.034), y1 - 0.25, z0 + 0.135), 0.1, 0.035, (sx, 0), GLOW_CELLS['tail']))
        # amber clearance lamps at the front top corners, facing forward
        rb('clear_f', (sx * (X - 0.05), y0 - 0.006, z1 - 0.1), (0.07, 0.02, 0.05), 0.01, amber)
    rb('rail_top_f', (0, y0 - 0.002, z1 - 0.05), (2 * X - 0.06, 0.03, 0.1), 0.008, alu)
    rb('rail_bot_f', (0, y0 - 0.004, z0 + 0.08), (2 * X - 0.06, 0.036, 0.16), 0.008, alu)
    # the rear frame: the header and the sill
    rb('header', (0, y1 - 0.033, z1 - 0.16), (2 * X - 0.2, 0.068, 0.32), 0.01, alu)
    rb('sill', (0, y1 - 0.035, z0 + 0.08), (2 * X - 0.2, 0.072, 0.16), 0.01, alu)
    # the roll-up door: eight panels with gaps between, hinges across each joint, the handle, the hasp, the pull strap
    dz0, dz1 = z0 + 0.16, z1 - 0.32
    dx = X - 0.155
    n = 8
    ph = (dz1 - dz0) / n
    for k in range(n):
        za = dz0 + k * ph
        rb('door_panel', (0, yd + 0.012, za + ph / 2), (2 * dx, 0.024, ph - 0.006), 0.006, paint)
        if k:
            for hx in (-dx + 0.12, 0.0, dx - 0.12):
                rb('hinge', (hx, yd + 0.027, za), (0.07, 0.008, 0.05), 0.003, alu)
    rb('door_handle', (0, yd + 0.05, dz0 + 0.12), (0.34, 0.02, 0.025), 0.009, alu)
    for hx in (-0.15, 0.15):
        rb('handle_post', (hx, yd + 0.035, dz0 + 0.12), (0.025, 0.03, 0.035), 0.006, alu)
    rb('hasp', (0.42, yd + 0.03, dz0 + 0.07), (0.05, 0.012, 0.09), 0.004, alu)
    rb('strap', (-0.25, yd + 0.03, dz0 - 0.07), (0.045, 0.006, 0.34), 0.002, blk)
    # the rear lamps on the jambs' feet: red (tail and stop) over amber (turn) over clear (reverse) in a black housing
    for sx in (-1, 1):
        lx = sx * (X - 0.075)
        rb('lamp_housing', (lx, y1 + 0.008, z0 + 0.43), (0.12, 0.022, 0.5), 0.012, blk)
        rb('lamp_red', (lx, y1 + 0.02, z0 + 0.56), (0.1, 0.012, 0.21), 0.01, red)
        rb('lamp_amber', (lx, y1 + 0.02, z0 + 0.39), (0.1, 0.012, 0.11), 0.01, amber)
        rb('lamp_rev', (lx, y1 + 0.02, z0 + 0.27), (0.1, 0.012, 0.1), 0.01, white)
        glows.append(glow_quad((lx, y1 + 0.026, z0 + 0.56), 0.1, 0.21, (0, 1), GLOW_CELLS['tail']))
        glows.append(glow_quad((lx, y1 + 0.026, z0 + 0.56), 0.1, 0.21, (0, 1), GLOW_CELLS['brake'], lift=0.003))
        glows.append(glow_quad((lx, y1 + 0.026, z0 + 0.39), 0.1, 0.11, (0, 1), GLOW_CELLS['turn_l' if sx > 0 else 'turn_r']))
        glows.append(glow_quad((lx, y1 + 0.026, z0 + 0.27), 0.1, 0.1, (0, 1), GLOW_CELLS['reverse']))
        # red clearance lamps at the header's corners
        rb('clear_r', (sx * (X - 0.075), y1 + 0.008, z1 - 0.1), (0.08, 0.02, 0.05), 0.01, red)
        glows.append(glow_quad((sx * (X - 0.075), y1 + 0.018, z1 - 0.1), 0.08, 0.05, (0, 1), GLOW_CELLS['tail']))
    for ix in (-0.3, 0.0, 0.3):
        rb('id_lamp', (ix, y1 + 0.008, z1 - 0.1), (0.1, 0.02, 0.04), 0.01, red)
        glows.append(glow_quad((ix, y1 + 0.018, z1 - 0.1), 0.1, 0.04, (0, 1), GLOW_CELLS['tail']))
    # under the box: the floor's crossmembers, the frame rails from under the cab, the rear axle on its springs
    k = 0
    yy = y0 + 0.12
    while yy < y1 - 0.08:
        rb('crossmember', (0, yy, z0 - 0.045), (2 * X - 0.06, 0.05, 0.09), 0.006, blk)
        yy += 0.4
        k += 1
    for sx in (-1, 1):
        rb('frame_rail', (sx * 0.47, (-0.86 + y1 - 0.2) / 2, 0.75), (0.07, y1 - 0.2 + 0.86, 0.2), 0.006, blk)
        rb('spring', (sx * 0.47, S.AXLES[1], 0.56), (0.07, 1.2, 0.06), 0.02, blk)
        rb('spring_hanger', (sx * 0.47, S.AXLES[1] - 0.6, 0.62), (0.05, 0.05, 0.1), 0.006, blk)
        rb('spring_hanger', (sx * 0.47, S.AXLES[1] + 0.6, 0.62), (0.05, 0.05, 0.1), 0.006, blk)
        # mud flaps behind the duals on hangers
        rb('mudflap', (sx * S.TRACK_R, S.AXLES[1] + 0.52, 0.47), (0.5, 0.012, 0.52), 0.01, blk)
        out.append((parts.rod('flap_hanger', (sx * (S.TRACK_R - 0.22), S.AXLES[1] + 0.52, 0.72), (sx * (S.TRACK_R - 0.22), S.AXLES[1] + 0.52, z0 - 0.09), 0.012, 10), blk))
    out.append((parts.rod('axle', (-0.62, S.AXLES[1], S.WZ), (0.62, S.AXLES[1], S.WZ), 0.055, 20), blk))
    out.append((closed_lathe('diff', [(0.0, -0.12), (0.12, -0.1), (0.16, 0.0), (0.12, 0.1), (0.0, 0.12)],
                             Matrix.Translation((0.0, S.AXLES[1], S.WZ)) @ Matrix.Rotation(math.radians(90), 4, 'X'), 24), blk))
    # the battery box on the +X side between the axles, the exhaust out under the -X side ahead of the duals
    rb('battery_box', (0.86, 0.55, 0.74), (0.5, 0.62, 0.36), 0.015, blk)
    rb('battery_lid', (0.86, 0.55, 0.93), (0.52, 0.64, 0.02), 0.006, alu)
    out.append((parts.rod('exhaust', (-0.3, 1.2, 0.52), (-0.95, 1.95, 0.48), 0.035, 14), alu))
    # the rear bumper: an underride bar on two struts with a ribbed step on it, the plate on its bracket and its lamp
    rb('bumper', (0, y1 + 0.07, 0.52), (2.12, 0.1, 0.1), 0.012, blk)
    for sx in (-1, 1):
        rb('bumper_strut', (sx * 0.6, y1 + 0.01, 0.76), (0.06, 0.12, 0.44), 0.008, blk)
    rb('step', (0, y1 + 0.07, 0.585), (0.62, 0.11, 0.03), 0.008, alu)
    rb('plate_bracket', (0.62, y1 + 0.06, 0.69), (0.34, 0.012, 0.19), 0.004, blk)
    out.append((plate_quad('plate_r', (0.62, y1 + 0.0705, 0.69), 0.30, 0.15, 1, mats['plate']), None))
    rb('plate_lamp', (0.62, y1 + 0.075, 0.8), (0.08, 0.02, 0.025), 0.008, white)
    return out


def build_truck():
    S = TruckBuild
    t0 = time.time()
    lines = line_sheets(S)
    sheets = truck_sheets(S)
    mats = body_materials(S)
    bpy.data.materials.remove(mats['paint'].m)
    mats['paint'] = truck_paint(S, lines, sheets)
    bpy.data.materials.remove(mat_of(mats['chrome']))
    mats['chrome'] = street.bare_metal(tex(S, 'chrome'), 0xb7bcc1, rough=0.3, grime=0.5)      # satin aluminum (the box's extrusions)
    mats['head'] = core.Mat(tex(S, 'head'))
    mats['red'] = street.lens(tex(S, 'red'), 0x8c0d12, rough=0.1)
    out = []
    body, bvh0 = van_body(S, mats)
    out.append((body, None))
    bvh = BVHTree.FromObject(body, bpy.context.evaluated_depsgraph_get())
    head = headlamp_material(S, bvh)
    for i, s in enumerate(body.material_slots):
        if s.material == mats['head'].m:
            body.material_slots[i].material = head.m
    bpy.data.materials.remove(mats['head'].m)
    mats['head'] = head
    head.m.name = mname(S, 'head')
    # the cab's lamps: the van's headlamps and front markers (its tails are on the box)
    seals = []
    for sx in (-1, 1):
        out.append((SD.lens_patch('headlamp', bvh, resample(S.HEAD, 72, 0.0075), corner_spot(S, bvh, sx, *S.HEAD_AXIS, -1), lift=0.0035,
                                  steps=LENS_STEPS, seal=seals), mats['head']))
        y0, z0, y1, z1 = S.MARK_F
        mk = [(y, z) for y, z in core.rounded_rect(y0, z0, y1, z1, 0.012, 3)]
        out.append((SD.lens_patch('marker', bvh, mk, side_ray(sx), steps=(1.0, 0.5), seal=seals), mats['amber']))
    out += [(s_, mats['trim']) for s_ in seals]
    out += front_end(S, bvh0, mats)
    out += van_parts(S, bvh, mats)

    class _Front:
        AXLES = (S.AXLES[0],)
        TRACK, TIRE_R, WZ = S.TRACK, S.TIRE_R, S.WZ
    out += calipers(_Front, mats)
    box_glows = []
    out += truck_box(S, mats, box_glows)
    glow_obj = glow_lamps(S, bvh, body, box_glows)
    for o, m in out:
        if m is not None:
            core.assign(o, m)
    obj = finish_body(S, [o for o, _ in out], mats)
    show_cabin(S, mats['glass'], bake_cabin(S, obj, mats))
    text = tire_text(S)
    wheel = truck_wheel_obj(S, wheel_materials(S, text), f'{S.name}_Wheel', False)
    dual = truck_wheel_obj(TruckDual, wheel_materials(TruckDual, text, dual_off=0.125), f'{S.name}_WheelDual', True)
    log(f'{S.name}: tris {tri_count(obj)}, built in {time.time() - t0:.0f}s; slots {[s.material.name for s in obj.material_slots]}')
    return [obj, wheel, dual, glow_obj]


EXTRA_SPECS = [Van, BoxTruck]


# ================================================================== build / bake / review

BUILDERS = {'hatchback': lambda: build_car(Hatchback), 'suv': lambda: build_car(SUV), 'cab': lambda: build_car(Cab), 'pickup': lambda: build_car(Pickup), 'van': build_van,
            'boxtruck': build_truck}
MESHES = []
for _n in ('SM_NS_Hatchback', 'SM_NS_SUV', 'SM_NS_Cab', 'SM_NS_Pickup', 'SM_NS_Van'):
    MESHES += [_n, _n + '_Wheel', _n + '_GlowLamps']
MESHES += ['SM_NS_BoxTruck', 'SM_NS_BoxTruck_Wheel', 'SM_NS_BoxTruck_WheelDual', 'SM_NS_BoxTruck_GlowLamps']


def build():
    core.reset()
    random.seed(4417)
    objs = []
    for key, fn in BUILDERS.items():
        if _ONLY and key not in _ONLY:
            continue
        try:
            made = fn()
        except Exception:
            traceback.print_exc()
            log(f'FAILED {key}')
            continue
        objs += made
    for o in objs:
        o.data.name = o.name
    return objs


_SPLIT = {}
_ORDER = {}


def _is_unbaked(m):
    return m is not None and m.name.startswith(UNBAKED_PREFIX)


def bake_parts(objs):
    import ns_kit
    _SPLIT.clear()
    _ORDER.clear()
    bake = []
    for o in objs:
        if '_Glow' in o.name:
            o.hide_render = True
            continue
        _ORDER[o.name] = [s.material.name for s in o.material_slots]
        idx = [i for i, s in enumerate(o.material_slots) if _is_unbaked(s.material)]
        if idx:
            dup = o.copy()
            dup.data = o.data.copy()
            core.link(dup)
            dup.name = o.name + '__unbaked'
            ns_kit._delete_faces(o, lambda f: f.material_index in idx)
            ns_kit._delete_faces(dup, lambda f: f.material_index not in idx)
            ns_kit._prune_slots(o)
            ns_kit._prune_slots(dup)
            dup.hide_render = True
            _SPLIT[o.name] = dup
        bake.append(o)
    # wheels bake alone (they sit at the origin, inside the bodies)
    return bake


def assemble(objs):
    from ns_ground import order_slots
    out = []
    for o in objs:
        name = o.name
        dup = _SPLIT.get(name)
        if dup is not None:
            dup.hide_render = False
            o = core.join([o, dup], name)
            o.name = o.data.name = name
        order = _ORDER.get(name)
        if order:
            mats = [s.material for s in o.material_slots]
            baked = [m for m in mats if not _is_unbaked(m)]
            want, bi = [], 0
            for nm in order:
                if nm.startswith(UNBAKED_PREFIX):
                    want.append(next(m for m in mats if m.name == nm))
                else:
                    want.append(baked[bi])
                    bi += 1
            order_slots(o, want, TAG)
            for i, s in enumerate(o.material_slots):
                if not _is_unbaked(s.material):
                    s.material.name = f'M_{name}_{i}'
            log(f'{name} slots: {[s.material.name for s in o.material_slots]}')
        out.append(o)
    for ob in list(bpy.data.objects):
        if '_GlowLamps' in ob.name and ob not in out:
            ob.hide_render = False
            out.append(ob)
    # the box truck was built in the van's frame: now it's baked, its body and glows move to its own (the middle at 0)
    for o in out:
        if o.name in ('SM_NS_BoxTruck', 'SM_NS_BoxTruck_GlowLamps'):
            o.data.transform(Matrix.Translation((0.0, TRUCK_DY, 0.0)))
    order = {n: i for i, n in enumerate(MESHES)}
    out.sort(key=lambda o: order.get(o.name, 999))
    return out


def _tint_paint(body, hexc):
    """The review's stand-in for the game's [tint]: the paint slot's base color multiplied by a street color."""
    m = body.material_slots[0].material if body.material_slots else None
    if m is None or not m.use_nodes:
        return
    nt = m.node_tree
    bsdf = next((n for n in nt.nodes if n.type == 'BSDF_PRINCIPLED'), None)
    if bsdf is None or not bsdf.inputs['Base Color'].is_linked:
        return
    src = bsdf.inputs['Base Color'].links[0].from_socket
    mx = nt.nodes.new('ShaderNodeMix')
    mx.data_type = 'RGBA'
    mx.blend_type = 'MULTIPLY'
    mx.inputs[0].default_value = 1.0
    nt.links.new(src, mx.inputs[6])
    mx.inputs[7].default_value = H(hexc)
    nt.links.new(mx.outputs[2], bsdf.inputs['Base Color'])


def _place_wheels(S, body, wheel, temp, dual=None):
    """Four wheel instances at the car's hubs (the -X side turned 180 deg) for the review; a dual pair at each rear hub
    (S.TRACK_R) when the vehicle has one."""
    ws = []
    for ay in S.AXLES:
        rear_dual = dual is not None and ay == S.AXLES[1]
        src = dual if rear_dual else wheel
        for sx in (-1, 1):
            first = sx > 0 and (ay == S.AXLES[0] or rear_dual)
            w = src if first else src.copy()
            if w is not src:
                bpy.context.scene.collection.objects.link(w)
                temp.append(w)
            w.location = (sx * (S.TRACK_R if rear_dual else S.TRACK), ay, S.WZ)
            w.rotation_mode = 'XYZ'          # (a glb-imported wheel comes in quaternion mode)
            w.rotation_euler = (0.0, 0.0, 0.0 if sx > 0 else math.pi)
            ws.append(w)
    return ws


def pose_for_review(objs):
    global REVIEW_VIEWS
    by = {o.name: o for o in objs}
    sc = bpy.context.scene
    temp = []
    d = os.path.join(core.BUILD_DIR, 'review', 'ns_cars')
    os.makedirs(d, exist_ok=True)
    specs = [S for S in (Hatchback, SUV, Cab, Pickup) if S.name in by] + [S for S in getattr(sys.modules[__name__], 'EXTRA_SPECS', []) if S.name in by]
    try:
        from ns_kit import _world, _cam, _mat
        from ns_ground import box as gbox
        floor = gbox(-30, 30, -30, 30, -0.3, 0.0, _mat('rv_floor', 0x4b4a48, 0.6))
        temp.append(floor)
        d_ = bpy.data.lights.new('rv_sun', 'SUN')
        d_.energy = 3.0
        d_.angle = math.radians(5.0)
        sun = core.link(bpy.data.objects.new('rv_sun', d_))
        sun.rotation_euler = Vector((-0.5, -0.6, 0.65)).normalized().to_track_quat('Z', 'Y').to_euler()
        temp.append(sun)
        # The glow film is shaded additively here (a lamp that's off adds nothing, so the baked lens and glass show through, as
        # they must in the game: see the module docstring's note on M_NS_Glow). Night lights the low beams, the tails and the
        # Cab's roof light.
        glow_mats = []
        for o in objs:
            if o.name.endswith('_GlowLamps'):
                m = core.Mat('rv_glow_' + o.name)
                uv = m.node('ShaderNodeUVMap')
                sp = m.node('ShaderNodeSeparateXYZ')
                m.link(uv.outputs['UV'], sp.inputs['Vector'])
                cell = m.math('ADD', m.math('FLOOR', m.math('MULTIPLY', sp.outputs['X'], 4.0)), m.math('MULTIPLY', m.math('FLOOR', m.math('MULTIPLY', sp.outputs['Y'], 2.0)), 4.0))

                def is_c(k):
                    return m.math('LESS_THAN', m.math('ABSOLUTE', m.math('SUBTRACT', cell, float(k))), 0.5)
                col = m.mix(is_c(0), H(0x000000), H(0xfff4e0))
                col = m.mix(is_c(2), col, H(0xff1a0a))
                col = m.mix(is_c(7), col, H(0xfff2c0))
                lit = m.math('MAXIMUM', m.math('MAXIMUM', is_c(0), m.math('MULTIPLY', is_c(2), 0.35)), m.math('MULTIPLY', is_c(7), 1.0 if 'Cab' in o.name else 0.0))
                e = m.node('ShaderNodeValue')
                e.outputs[0].default_value = 0.0
                em = m.node('ShaderNodeEmission')
                m.link(col, em.inputs['Color'])
                m.link(m.math('MULTIPLY', lit, e.outputs[0]), em.inputs['Strength'])
                tr = m.node('ShaderNodeBsdfTransparent')
                add = m.node('ShaderNodeAddShader')
                m.link(tr.outputs[0], add.inputs[0])
                m.link(em.outputs[0], add.inputs[1])
                m.link(add.outputs[0], m.out.inputs['Surface'])
                o.data.materials[0] = m.m
                glow_mats.append((e, lit))
        # A parked car shows its paint: the tinted bodies take a street color for the sheet (the game sets its own).
        tints = {'SM_NS_Hatchback': 0x23395a, 'SM_NS_SUV': 0x4a4d52}
        for S in specs:
            if S.name in tints and S.name in by:
                _tint_paint(by[S.name], tints[S.name])
        # a street lamp over the cars at night (warm LED from above and to the front), and a little cool sky
        lamp_d = bpy.data.lights.new('rv_lamp', 'AREA')
        lamp_d.energy = 900.0
        lamp_d.size = 1.2
        lamp_d.color = (1.0, 0.78, 0.55)
        lamp = core.link(bpy.data.objects.new('rv_lamp', lamp_d))
        lamp.location = (-3.0, -2.0, 6.5)
        lamp.rotation_euler = Vector((3.0, 2.0, -6.5)).to_track_quat('-Z', 'Y').to_euler()
        temp.append(lamp)
        paths = {False: [], True: []}
        for night in (False, True):
            _world(sc, night)
            sun.hide_render = night
            lamp.hide_render = not night
            for e, _ in glow_mats:
                e.outputs[0].default_value = 12.0 if night else 0.0
            sc.view_settings.exposure = 0.6 if night else -0.2
            for S in specs:
                body, wheel, glow = by[S.name], by.get(S.name + '_Wheel'), by.get(S.name + '_GlowLamps')
                dual = by.get(S.name + '_WheelDual')
                for o in objs:
                    o.hide_render = o.name not in (S.name, S.name + '_Wheel', S.name + '_WheelDual', S.name + '_GlowLamps')
                body.location = glow.location = (0, 0, 0)
                ws = _place_wheels(S, body, wheel, temp, dual) if wheel is not None else []
                L = S.Y_R - S.Y_F
                k = getattr(S, 'REVIEW_K', 1.0)
                shots = [('front34', (-3.4 * k, -L * 0.95, 1.35 * k), (0, -0.2, 0.7 * k), 45), ('rear34', (3.0 * k, L * 0.9, 1.5 * k), (0, 0.3, 0.75 * k), 45),
                         ('side', (-6.2 * k, 0.0, 0.9 * k), (0, 0, 0.75 * k), 42), ('wheel', (-1.9, S.AXLES[0] - 0.9, 0.55), (-S.TRACK, S.AXLES[0], 0.4), 40)]
                if dual is not None:
                    shots[3] = ('dual', (-2.4, S.AXLES[1] + 1.3, 0.75), (-S.TRACK_R, S.AXLES[1], 0.45), 40)      # (four to a row: the duals for the wheel)
                if night:
                    shots = shots[:2]
                for label, cp, tp, fov in shots:
                    paths[night].append(_cam(sc, cp, tp, fov, os.path.join(d, f'{S.tag}_{"n" if night else "d"}_{label}.png'), 900, 640, 40))
                # the wheel copies go (left behind they'd sit at the origin, half sunk, under the next car)
                for w in ws:
                    if w is wheel or w is dual:
                        w.location = (0, 0, 0)
                        w.rotation_euler = (0, 0, 0)
                    else:
                        if w in temp:
                            temp.remove(w)
                        bpy.data.objects.remove(w)
        if paths[False]:
            compose_grid('ns_cars_day', paths[False], 4, TAG)
        if paths[True]:
            compose_grid('ns_cars_night', paths[True], 4, TAG)
    except Exception:
        traceback.print_exc()
    finally:
        for o in temp:
            try:
                bpy.data.objects.remove(o)
            except ReferenceError:
                pass
        sc.view_settings.exposure = 0.0
    # the standard sheet: cars in a row, each with its wheels set back on
    x = 0.0
    for S in specs:
        o = by[S.name]
        o.location = (x, 0, 0)
        o.hide_render = False
        if S.name + '_GlowLamps' in by:
            by[S.name + '_GlowLamps'].location = (x, 0, 0)
            by[S.name + '_GlowLamps'].hide_render = False
        if S.name + '_Wheel' in by:
            w = by[S.name + '_Wheel']
            wd = by.get(S.name + '_WheelDual')
            w.hide_render = False
            if wd is not None:
                wd.hide_render = False
            used = set()
            for ay in S.AXLES:
                rear_dual = wd is not None and ay == S.AXLES[1]
                src = wd if rear_dual else w
                for sx in (-1, 1):
                    inst = src if src.name not in used else src.copy()
                    if src.name in used:
                        sc.collection.objects.link(inst)
                    used.add(src.name)
                    inst.location = (x + sx * (S.TRACK_R if rear_dual else S.TRACK), ay, S.WZ)
                    inst.rotation_mode = 'XYZ'
                    inst.rotation_euler = (0.0, 0.0, 0.0 if sx > 0 else math.pi)
        x += 3.1 if getattr(S, 'DUAL', False) else 2.6
    REVIEW_VIEWS = [('front', -35, 14, 1.4), ('rear', 150, 14, 1.2), ('top', 0, 60, 1.2)]
