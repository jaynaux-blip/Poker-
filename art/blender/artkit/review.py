"""Review sheets: every asset is rendered from several angles, in a neutral studio and under
the game's neon lighting, plus a wireframe view, and composed into one image for sign-off."""
import math
import os

import bpy
import numpy as np
from mathutils import Vector

from . import core


def _look_at(obj, target):
    d = Vector(target) - obj.location
    obj.rotation_euler = d.to_track_quat('-Z', 'Y').to_euler()


def _area(name, loc, target, size, power, color=(1, 1, 1)):
    data = bpy.data.lights.new(name, 'AREA')
    data.shape = 'DISK'
    data.size = size
    data.energy = power
    data.color = color
    obj = core.link(bpy.data.objects.new(name, data))
    obj.visible_camera = False  # light the asset without appearing in frame
    obj.location = loc
    _look_at(obj, target)
    return obj


def _orbit(center, radius, azimuth_deg, elevation_deg):
    a = math.radians(azimuth_deg)
    e = math.radians(elevation_deg)
    return Vector((center.x + radius * math.cos(e) * math.sin(a), center.y - radius * math.cos(e) * math.cos(a), center.z + radius * math.sin(e)))


def _floor(z, radius, color, rough):
    bpy.ops.mesh.primitive_circle_add(vertices=96, radius=radius, fill_type='NGON', location=(0, 0, z))
    fl = bpy.context.view_layer.objects.active
    fl.name = 'review_floor'
    m = core.Mat('review_floor')
    m.set('Base Color', color)
    m.set('Roughness', rough)
    core.assign(fl, m)
    return fl


def _wood_desk(z, radius):
    """A dark varnished desk top for the in-game lighting view."""
    bpy.ops.mesh.primitive_plane_add(size=radius * 2, location=(0, 0, z))
    fl = bpy.context.view_layer.objects.active
    fl.name = 'review_desk'
    m = core.Mat('review_desk')
    tc = m.node('ShaderNodeTexCoord')
    wave = m.node('ShaderNodeTexWave', Scale=6.0 / radius, Distortion=6.0, **{'Detail': 4.0})
    m.link(tc.outputs['Object'], wave.inputs['Vector'])
    ramp = m.mix(wave.outputs['Fac'], core.hex_linear(0x1c0f08), core.hex_linear(0x3a2214))
    m.set('Base Color', ramp)
    m.set('Roughness', 0.32)
    core.assign(fl, m)
    return fl


def _render(path, w, h, samples):
    sc = bpy.context.scene
    sc.render.resolution_x = w
    sc.render.resolution_y = h
    sc.render.resolution_percentage = 100
    sc.cycles.samples = samples
    sc.cycles.adaptive_threshold = 0.02
    sc.cycles.use_denoising = True
    sc.render.filepath = path
    sc.render.image_settings.file_format = 'PNG'
    sc.render.image_settings.color_mode = 'RGB'
    bpy.ops.render.render(write_still=True)
    return path


def _pixels(path):
    img = bpy.data.images.load(path, check_existing=False)
    w, h = img.size
    a = np.empty(w * h * 4, dtype=np.float32)
    img.pixels.foreach_get(a)
    bpy.data.images.remove(img)
    return a.reshape(h, w, 4)


def sheet(objs, name, views=None, tile=900, samples=96, context_scale=1.0, screen_light=True):
    """Renders the review sheet art/review/<name>.jpg (3 x 2 tiles) and returns its path.

    views: optional list of (label, azimuth, elevation, distance factor[, target]) for the first three
    tiles; the camera aims at target (world meters) when given, else at the middle of the asset.
    screen_light: light the in-game views with a stand-in for the laptop's glow. Off for an asset that
    lights itself (the laptop), whose glossy screen would otherwise mirror the stand-in.
    """
    sc = bpy.context.scene
    lo, hi = core.bounds(objs)
    center = (lo + hi) / 2
    size = max((hi - lo).length, 1e-4)
    r = size / 2
    tmp_dir = os.path.join(core.BUILD_DIR, 'review', name)
    os.makedirs(tmp_dir, exist_ok=True)
    views = views or [('front', -30, 18, 2.6), ('top', 20, 62, 2.4), ('detail', -60, 32, 1.35)]

    cam_data = bpy.data.cameras.new('review_cam')
    cam_data.lens = 85
    cam_data.clip_start = size * 0.01
    cam_data.clip_end = size * 200
    cam = core.link(bpy.data.objects.new('review_cam', cam_data))
    sc.camera = cam

    # Studio: soft key, cool fill, strong rim, dark gradient world.
    world = sc.world
    world.color = (0.006, 0.0065, 0.008)
    floor = _floor(lo.z, size * 20, core.hex_linear(0x1a1b1f), 0.38)
    lights = [
        # Powers scale with size squared, so every asset gets the same exposure.
        _area('key', _orbit(center, size * 3, -45, 40), center, size * 2.2, 520 * size ** 2, (1.0, 0.95, 0.88)),
        _area('fill', _orbit(center, size * 3, 70, 15), center, size * 3, 130 * size ** 2, (0.8, 0.88, 1.0)),
        _area('rim', _orbit(center, size * 3, 170, 35), center, size * 1.2, 900 * size ** 2, (1.0, 1.0, 1.0)),
    ]
    tiles = []

    def aim(view):
        target = Vector(view[4]) if len(view) > 4 else center
        cam.location = _orbit(target, size * view[3], view[1], view[2])
        _look_at(cam, target)

    for view in views:
        aim(view)
        tiles.append(_render(os.path.join(tmp_dir, f'{view[0]}.png'), tile, tile, samples))

    # Wireframe over clay, to check topology and density.
    wire = core.Mat('review_wire')
    wf = wire.node('ShaderNodeWireframe', Size=0.9)
    wf.use_pixel_size = True
    col = wire.mix(wf.outputs['Fac'], core.hex_linear(0x7d828b), core.hex_linear(0x0c0d10))
    wire.set('Base Color', col)
    wire.set('Roughness', 0.6)
    sc.view_layers[0].material_override = wire.m
    aim(views[0])
    tiles.append(_render(os.path.join(tmp_dir, 'wire.png'), tile, tile, 32))
    sc.view_layers[0].material_override = None

    # In game: a dark desk, the laptop's cool glow in front, the laundromat's pink neon from the window.
    for l in lights:
        l.hide_render = True
    floor.hide_render = True
    desk = _wood_desk(lo.z, size * 20)
    world.color = (0.002, 0.002, 0.004)
    scale = context_scale
    game = [
        _area('neon', _orbit(center, size * 3, 150, 28), center, size * 1.0, 700 * size ** 2 * scale, (1.0, 0.18, 0.55)),
        _area('city', _orbit(center, size * 3, -150, 45), center, size * 2.0, 90 * size ** 2 * scale, (0.35, 0.47, 0.78)),
    ]
    if screen_light:
        game.append(_area('screen', _orbit(center, size * 2.2, -15, 12), center, size * 0.9, 160 * size ** 2 * scale, (0.72, 0.84, 1.0)))
    for label, az, el, dist in (('game', -25, 16, 2.4), ('game_close', 35, 24, 1.5)):
        cam.location = _orbit(center, size * dist, az, el)
        _look_at(cam, center)
        tiles.append(_render(os.path.join(tmp_dir, f'{label}.png'), tile, tile, samples))
    for l in game:
        bpy.data.objects.remove(l)
    bpy.data.objects.remove(desk)
    for l in lights:
        l.hide_render = False
    floor.hide_render = False

    path = compose(name, tiles, tile)
    bpy.data.objects.remove(floor)
    for l in lights:
        bpy.data.objects.remove(l)
    bpy.data.objects.remove(cam)
    return path


def compose(name, tiles, tile=900):
    """Lays six renders out 3 x 2 (studio views on top, wireframe and in-game views below) as art/review/<name>.jpg."""
    imgs = [_pixels(p) for p in tiles]
    gap = 6
    W = tile * 3 + gap * 2
    H = tile * 2 + gap
    out = np.zeros((H, W, 4), dtype=np.float32)
    out[:, :, 3] = 1.0
    out[:, :, :3] = 0.05
    for k, im in enumerate(imgs[:6]):
        row = 0 if k < 3 else 1
        colm = k % 3
        y0 = H - (row + 1) * tile - row * gap  # image rows run bottom-up
        x0 = colm * (tile + gap)
        out[y0:y0 + tile, x0:x0 + tile] = im
    os.makedirs(core.REVIEW_DIR, exist_ok=True)
    path = os.path.join(core.REVIEW_DIR, f'{name}.jpg')
    img = bpy.data.images.new(f'review_{name}', W, H, alpha=False)
    img.pixels.foreach_set(out.ravel())
    img.filepath_raw = path
    img.file_format = 'JPEG'
    img.save(quality=90)
    bpy.data.images.remove(img)
    return path
