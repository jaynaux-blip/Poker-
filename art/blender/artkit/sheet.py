"""Printed artwork (labels, chip inlays, logos) laid out in 2D and rendered by Cycles.

Each element carries a color and surface attributes. render() writes two
images of the same layout: the color (sRGB) and a surface map (R metallic,
G roughness, B coverage) that materials use to vary finish across the print.
"""
import math
import os

import bpy

from . import core


class Sheet:
    def __init__(self, width, height):
        """Layout units are arbitrary (millimeters work well); the image keeps the aspect ratio."""
        self.w = width
        self.h = height
        self.items = []  # (object, color hex, metallic, roughness)
        self.z = 0.0
        self.fonts = {}

    def _font(self, face):
        if face not in self.fonts:
            self.fonts[face] = bpy.data.fonts.load(os.path.join(core.FONTS, f'Roboto-{face}.ttf'), check_existing=True)
        return self.fonts[face]

    def _add(self, obj, color, metal, rough, alpha=1.0):
        self.z += 0.001
        obj.location.z = self.z
        core.link(obj)
        self.items.append((obj, color, metal, rough, alpha))
        return obj

    def rect(self, x, y, w, h, color, metal=0.0, rough=0.4, rot=0.0, alpha=1.0):
        me = bpy.data.meshes.new('rect')
        me.from_pydata([(0, 0, 0), (w, 0, 0), (w, h, 0), (0, h, 0)], [], [(0, 1, 2, 3)])
        obj = bpy.data.objects.new('rect', me)
        obj.location = (x, y, 0)
        obj.rotation_euler.z = rot
        return self._add(obj, color, metal, rough, alpha)

    def poly(self, points, color, metal=0.0, rough=0.4, alpha=1.0):
        me = bpy.data.meshes.new('poly')
        me.from_pydata([(x, y, 0) for x, y in points], [], [tuple(range(len(points)))])
        return self._add(bpy.data.objects.new('poly', me), color, metal, rough, alpha)

    def circle(self, cx, cy, r, color, metal=0.0, rough=0.4, n=96, alpha=1.0):
        return self.poly(core.circle_points(cx, cy, r, n), color, metal, rough, alpha)

    def ring(self, cx, cy, r0, r1, color, metal=0.0, rough=0.4, n=128, a0=0.0, a1=2 * math.pi, alpha=1.0):
        pts = [(cx + r1 * math.cos(a0 + (a1 - a0) * i / n), cy + r1 * math.sin(a0 + (a1 - a0) * i / n)) for i in range(n + 1)]
        pts += [(cx + r0 * math.cos(a1 - (a1 - a0) * i / n), cy + r0 * math.sin(a1 - (a1 - a0) * i / n)) for i in range(n + 1)]
        return self.poly(pts, color, metal, rough, alpha)

    def text(self, s, x, y, size, color, face='Black', align='LEFT', rot=0.0, tracking=1.0, metal=0.0, rough=0.4, alpha=1.0):
        cu = bpy.data.curves.new('text', 'FONT')
        cu.body = s
        cu.font = self._font(face)
        cu.size = size
        cu.align_x = align
        cu.space_character = tracking
        obj = bpy.data.objects.new('text', cu)
        obj.location = (x, y, 0)
        obj.rotation_euler.z = rot
        return self._add(obj, color, metal, rough, alpha)

    def text_on_arc(self, s, cx, cy, radius, size, color, face='Bold', center_angle=math.pi / 2, tracking=1.0, metal=0.0, rough=0.4, inward=False):
        """Characters set along a circle, centered at center_angle, reading clockwise on top."""
        font = self._font(face)
        widths = []
        for ch in s:
            cu = bpy.data.curves.new('m', 'FONT')
            cu.body = ch if ch != ' ' else 'n'
            cu.font = font
            cu.size = size
            tmp = bpy.data.objects.new('m', cu)
            core.link(tmp)
            bpy.context.view_layer.update()
            w = tmp.dimensions.x if ch != ' ' else size * 0.28
            bpy.data.objects.remove(tmp)
            widths.append(w * tracking)
        total = sum(widths)
        ang = center_angle + (total / 2) / radius
        for ch, w in zip(s, widths):
            a = ang - (w / 2) / radius
            if ch != ' ':
                px = cx + radius * math.cos(a)
                py = cy + radius * math.sin(a)
                self.text(ch, px, py, size, color, face=face, align='CENTER', rot=a - math.pi / 2, metal=metal, rough=rough)
            ang -= w / radius

    def render(self, name, px_width, samples=24):
        """Renders into art/build/textures/<name>_color.png and <name>_surface.png; returns both paths."""
        sc = bpy.context.scene
        sc.render.engine = 'CYCLES'  # EEVEE needs a GPU; Cycles renders anywhere
        sc.cycles.device = 'CPU'
        out_dir = os.path.join(core.BUILD_DIR, 'textures')
        os.makedirs(out_dir, exist_ok=True)
        cam_data = bpy.data.cameras.new('sheet_cam')
        cam_data.type = 'ORTHO'
        cam_data.ortho_scale = max(self.w, self.h)
        cam = core.link(bpy.data.objects.new('sheet_cam', cam_data))
        cam.location = (self.w / 2, self.h / 2, 10.0)
        cam_data.clip_end = 100.0
        sc.camera = cam
        sc.render.resolution_x = px_width
        sc.render.resolution_y = max(1, round(px_width * self.h / self.w))
        sc.render.resolution_percentage = 100
        sc.render.film_transparent = True
        sc.cycles.samples = samples
        sc.cycles.use_denoising = False
        sc.render.filter_size = 0.9
        sc.world.color = (0, 0, 0)
        paths = {}
        for pass_name, view in (('color', 'Standard'), ('surface', 'Raw')):
            sc.view_settings.view_transform = view
            for obj, color, metal, rough, alpha in self.items:
                if pass_name == 'color':
                    rgba = core.hex_linear(color)
                else:
                    rgba = (metal, rough, 1.0, 1.0)
                mat = bpy.data.materials.new('sheet')
                if not mat.node_tree:
                    mat.use_nodes = True
                nt = mat.node_tree
                for n in list(nt.nodes):
                    nt.nodes.remove(n)
                em = nt.nodes.new('ShaderNodeEmission')
                em.inputs['Color'].default_value = rgba
                out = nt.nodes.new('ShaderNodeOutputMaterial')
                if alpha < 1.0:
                    tr = nt.nodes.new('ShaderNodeBsdfTransparent')
                    mix = nt.nodes.new('ShaderNodeMixShader')
                    mix.inputs['Fac'].default_value = alpha
                    nt.links.new(tr.outputs[0], mix.inputs[1])
                    nt.links.new(em.outputs[0], mix.inputs[2])
                    nt.links.new(mix.outputs[0], out.inputs['Surface'])
                else:
                    nt.links.new(em.outputs[0], out.inputs['Surface'])
                if obj.type == 'MESH':
                    obj.data.materials.clear()
                    obj.data.materials.append(mat)
                else:
                    obj.data.materials.clear()
                    obj.data.materials.append(mat)
            path = os.path.join(out_dir, f'{name}_{pass_name}.png')
            sc.render.filepath = path
            sc.render.image_settings.file_format = 'PNG'
            sc.render.image_settings.color_mode = 'RGBA'
            bpy.ops.render.render(write_still=True)
            paths[pass_name] = path
        sc.render.film_transparent = False
        sc.view_settings.view_transform = 'AgX'
        return paths['color'], paths['surface']
