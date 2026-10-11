"""
SHORT STACK: the next segment's art in the editor (next_segment_spec 3.10 and 6.2 B; track 13, art). Made for the meshes
art/blender builds (wear.py's patches, chips.py's tournament set):

  T_PatchAtlas_BaseColor / _Normal / _ORT   /Game/ShortStack/Textures/Patches, from unreal/Art/Patches (sponsors.py): every
                    brand's patch face, embroidered (satin, tatami and twill in the normal map), in cells by the brand's
                    index (sponsor::Brand::Mark: the catalog's order, patch_atlas.json). BaseColor sRGB; Normal a normal map
                    with Blender's OpenGL green flipped; ORT linear masks (R occlusion, G roughness, B thread).
  M_Patch           a sponsor's patch on a garment: opaque, a few hundred triangles of mesh with the merrowed edge as
                    real geometry (so no alpha test). Slot 0, the face: its brand's cell of the atlas (the embroidery's
                    colour, relief and sheen: thread specular over the twill's). Slot 1, the merrowed edge: the brand's
                    thread colour (the cell's swatch) overcast at a slant, Wraps a centimetre.
                    The brand, per patch, either way:
                      - Custom Primitive Data 0 = the brand's index + 1 (UPrimitiveComponent::SetCustomPrimitiveDataFloat
                        (0, Brand.Mark + 1)): no material instance per patch; 0 (unset) falls back to
                      - the BrandIndex parameter (a dynamic instance's SetScalarParameterValue("BrandIndex", Mark)).
                    The runtime sheet instead (sponsorart::Patch drawn into a render target, spec 3.10): a dynamic
                    instance with UseMap 1 and PatchMap the target; the face shows the sheet (the round patch its mark's
                    box), the edge its drawn merrow's colour, over a procedural twill.
                    Art direction in engine: Tint, Rough (added), Strength (the relief), Wraps.
  MI_Patch_Wide_Face, _Wide_Thread, _Round_Face, _Round_Thread    its instances (Shape 0 wide / 1 round, Part 0 face /
                    1 edge), set on the meshes' slots here: SM_Wear_Patch and SM_Wear_PatchCap wide, SM_Wear_PatchRound
                    round (slot "patch_face" the face, "patch_thread" the edge).
  M_Chip            the Embercrest's tournament chips (SM_Chip_T100, _T500, _T1K, _T5K, _T25K) and SM_ChipStacks_EC: their
                    own baked maps (the glb's), Tint, Rough (added), Specular (clay's, 0.42), Strength; instanced and on
                    Nanite. MI_<mesh>_<slot> beside each mesh (in its folder, so a re-import's refresh of the mesh's
                    textures replaces them with it; this rebuilds them after). Dee's four chips keep the art's own.

shortstack_setup.run() runs this with the realism setups (REALISM_SETUPS), after the meshes and textures are imported.
By hand:

    import segment_art_setup; segment_art_setup.run(force=True)
"""
import json
import os
import re

import unreal

import shortstack_setup as ss

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

# Bump when a material's graph changes: each built material carries it (metadata) and is rebuilt when it differs.
VERSION = "1"
VERSION_TAG = "SegmentArtVersion"

TEX_DIR = "/Game/ShortStack/Textures/Patches"
PATCH_MESHES = {"SM_Wear_Patch": "Wide", "SM_Wear_PatchCap": "Wide", "SM_Wear_PatchRound": "Round"}
CHIP_MESHES = ("SM_Chip_T100", "SM_Chip_T500", "SM_Chip_T1K", "SM_Chip_T5K", "SM_Chip_T25K", "SM_ChipStacks_EC")

COLOR = unreal.MaterialSamplerType.SAMPLERTYPE_COLOR
NORMAL = unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL
MASKS = unreal.MaterialSamplerType.SAMPLERTYPE_MASKS
LINEAR = unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR
DEFAULTS = {
    COLOR: "/Engine/EngineMaterials/DefaultDiffuse.DefaultDiffuse",
    NORMAL: "/Engine/EngineMaterials/DefaultNormal.DefaultNormal",
    MASKS: "/Engine/EngineMaterials/DefaultDiffuse_TC_Masks.DefaultDiffuse_TC_Masks",
    LINEAR: "/Engine/EngineMaterials/DefaultDiffuse_TC_Masks.DefaultDiffuse_TC_Masks",
}

# The atlas's layout when patch_atlas.json isn't there (sponsors.py writes the same numbers).
LAYOUT = {"size": 2048, "wide": {"cell": [512, 344], "cols": 4, "y0": 0}, "round": {"cell": 292, "cols": 7, "y0": 1376}, "swatch": 12,
          "brands": ["luckypenny", "barber", "ironhouse", "voltrush", "overclock", "embercrest", "halo", "tunnelrat", "stacked",
                     "rangewise", "riverline", "sitwell", "turnstone"]}

# ---------------------------------------------------------------- HLSL

PATCH_UV = """
// The atlas cell a patch reads (sponsors.py's layout, pixels from the top left), or the runtime sheet. UV: the mesh's own
// (the face 0..1 with v down; the edge's centimetres, which the edge never uses here: it reads its swatch).
float i = Cpd > 0.5 ? Cpd - 1.0 : Index;
i = clamp(floor(i + 0.5), 0.0, {last});
float2 uv = saturate(UV);
if (UseMap > 0.5)
{
    // sponsorart::Patch's 512 x 340 sheet: the round face shows its mark's box; the edge, the merrow drawn round it.
    if (Part > 0.5) return float2(0.5, 16.0 / 340.0);
    if (Shape > 0.5) return float2((48.0 + uv.x * 190.0) / 512.0, (74.0 + uv.y * 190.0) / 340.0);
    return uv;
}
float2 o = Shape > 0.5 ? float2(fmod(i, {rcols}) * {rs}, {ry0} + floor(i / {rcols}) * {rs}) : float2(fmod(i, {wcols}) * {ww}, {wy0} + floor(i / {wcols}) * {wh});
float2 s = Shape > 0.5 ? float2({rs}, {rs}) : float2({ww}, {wh});
// The edge reads its thread's swatch in the cell's corner: one texel for the whole edge, so always the sharpest mip.
float2 px = Part > 0.5 ? o + {half_swatch} : o + uv * s;
return px / {size};
"""

PATCH_SURFACE = """
// Base: the atlas's colour; Map: the runtime sheet's; Ort: occlusion, roughness, thread. UV: the mesh's own.
float3 col = UseMap > 0.5 ? Map : Base;
float rough = UseMap > 0.5 ? 0.78 : Ort.g;
if (Part > 0.5)
{
    // The merrowed edge: the thread overcast at a slant (u centimetres along the edge, v across it), each wrap rounded.
    float f = frac(UV.x * Wraps + UV.y * 0.6);
    float h = sin(f * 3.14159265);
    col *= lerp(0.55, 1.0, h);
    rough = lerp(0.62, 0.40, h);
}
return float4(col * Tint.rgb, saturate(rough + Rough));
"""

PATCH_NORMAL = """
// The face's relief: the atlas's embroidery; the edge's wraps; a plain twill under a runtime sheet.
float3 n = N;
if (Part > 0.5)
{
    float f = frac(UV.x * Wraps + UV.y * 0.6);
    float d = cos(f * 3.14159265);
    n = float3(-d * 0.55, -d * 0.2, 1.0);
}
else if (UseMap > 0.5)
{
    // Twill: diagonal wales 0.62 mm apart across the 76 x 51 mm face.
    float t = (UV.x * 76.0 * 0.454 - UV.y * 51.0 * 0.891) / 0.62;
    float d = cos(t * 6.2831853) * 0.18;
    n = float3(-d * 0.454, d * 0.891, 1.0);
}
n.xy *= Strength;
return normalize(n);
"""

PATCH_EXTRA = """
// x: occlusion; y: specular (thread shines over the twill).
if (Part > 0.5)
{
    float h = sin(frac(UV.x * Wraps + UV.y * 0.6) * 3.14159265);
    return float2(lerp(0.7, 1.0, h), 0.5);
}
if (UseMap > 0.5) return float2(1.0, 0.35);
return float2(Ort.r, lerp(0.3, 0.55, Ort.b));
"""

CHIP_NORMAL = """
return normalize(float3(N.xy * Strength, N.z));
"""


# ---------------------------------------------------------------- helpers


def _material(name, force, stamp):
    """The material to build: new, forced, or built by an older version of this file (or from other inputs). None when
    it's current."""
    path = f"{ss.MAT_DIR}/{name}"
    if eal.does_asset_exist(path) and not force:
        existing = eal.load_asset(path)
        if existing is not None and eal.get_metadata_tag(existing, VERSION_TAG) == stamp:
            return None
    return ss._new_material(name, True)


def _finish(mat, stamp):
    eal.set_metadata_tag(mat, VERSION_TAG, stamp)
    return ss._finish(mat)


def _set(obj, prop, value):
    try:
        obj.set_editor_property(prop, value)
        return True
    except Exception as exc:
        unreal.log_warning(f"ShortStack: couldn't set {prop}: {exc}")
        return False


def _custom(mat, code, inputs, out_type, x, y, description):
    """A Custom node with its inputs linked: (name, expression) or (name, (expression, output pin))."""
    c = ss._custom(mat, code, [name for name, _ in inputs], out_type, x, y, description)
    for name, src in inputs:
        expr, pin = src if isinstance(src, tuple) else (src, "")
        if not mel.connect_material_expressions(expr, pin, c, name):
            unreal.log_warning(f"ShortStack: {mat.get_name()}: couldn't connect {name}")
    return c


def _tex(mat, param, asset, sampler, x, y, uv=None):
    """A texture parameter sampled at uv: asset, or the engine's default for its sampler."""
    e = ss._expr(mat, unreal.MaterialExpressionTextureSampleParameter2D, x, y)
    e.set_editor_property("parameter_name", param)
    if asset is None:
        asset = unreal.load_object(None, DEFAULTS[sampler])
    if asset is not None:
        e.set_editor_property("texture", asset)
    e.set_editor_property("sampler_type", sampler)
    if uv is not None:
        mel.connect_material_expressions(uv, "", e, "UVs")
    return e


def _prop(src, prop, pin=""):
    mel.connect_material_property(src, pin, prop)


def _layout():
    path = os.path.join(unreal.Paths.project_dir(), "Art", "Patches", "patch_atlas.json")
    if os.path.isfile(path):
        try:
            with open(path, "r", encoding="utf-8") as f:
                return json.load(f)
        except Exception as exc:
            unreal.log_warning(f"ShortStack: patch_atlas.json: {exc}")
    return LAYOUT


def _instance(path, parent, scalars=None, textures=None):
    """A material instance at path (made if missing) on parent with these scalars and textures. True if it changed."""
    folder, name = path.rsplit("/", 1)
    mi = eal.load_asset(path) if eal.does_asset_exist(path) else None
    made = mi is None
    if made:
        mi = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.MaterialInstanceConstant,
                                                                    unreal.MaterialInstanceConstantFactoryNew())
        if mi is None:
            unreal.log_error(f"ShortStack: couldn't make {path}")
            return None, False
    changed = made or mi.get_editor_property("parent") != parent
    if changed:
        mel.set_material_instance_parent(mi, parent)
    for key, value in (scalars or {}).items():
        if made or abs(mel.get_material_instance_scalar_parameter_value(mi, key) - float(value)) > 1e-6:
            mel.set_material_instance_scalar_parameter_value(mi, key, float(value))
            changed = True
    for key, tex in (textures or {}).items():
        if tex is not None and (made or mel.get_material_instance_texture_parameter_value(mi, key) != tex):
            mel.set_material_instance_texture_parameter_value(mi, key, tex)
            changed = True
    if changed:
        mel.update_material_instance(mi)
        eal.save_loaded_asset(mi)
    return mi, changed


def _mesh(name):
    path = f"{ss.MESH_DIR}/{name}/{name}"
    if not eal.does_asset_exist(path):
        return None
    mesh = eal.load_asset(path)
    return mesh if isinstance(mesh, unreal.StaticMesh) else None


def _slot_names(mesh):
    names = []
    for slot in mesh.get_editor_property("static_materials"):
        n = str(slot.get_editor_property("material_slot_name"))
        if not n or n == "None":
            try:
                n = str(slot.get_editor_property("imported_material_slot_name"))
            except Exception:
                n = ""
        names.append(n)
    return names


# ---------------------------------------------------------------- the patch atlas


def _texture_format(tex, kind):
    """Sets the atlas's maps to their formats. True if anything changed."""
    want_srgb = kind == "color"
    want_cs = {"color": unreal.TextureCompressionSettings.TC_DEFAULT, "normal": unreal.TextureCompressionSettings.TC_NORMALMAP,
               "linear": unreal.TextureCompressionSettings.TC_MASKS}[kind]
    changed = False
    if tex.get_editor_property("srgb") != want_srgb:
        tex.set_editor_property("srgb", want_srgb)
        changed = True
    if tex.get_editor_property("compression_settings") != want_cs:
        tex.set_editor_property("compression_settings", want_cs)
        changed = True
    if kind == "normal":
        try:
            if not tex.get_editor_property("flip_green_channel"):
                tex.set_editor_property("flip_green_channel", True)  # Blender's (and the atlas's) normals are OpenGL
                changed = True
        except Exception as exc:
            unreal.log_warning(f"ShortStack: {tex.get_name()}: flip_green_channel: {exc}")
    return changed


def import_patch_textures(force=False):
    """The atlas's maps from unreal/Art/Patches into /Game/ShortStack/Textures/Patches, imported again only when a file
    changed (its md5 kept as SourceHash), each in its format. Returns how many it imported or fixed."""
    src = os.path.join(unreal.Paths.project_dir(), "Art", "Patches")
    if not os.path.isdir(src):
        return 0
    if not eal.does_directory_exist(TEX_DIR):
        eal.make_directory(TEX_DIR)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    done = 0
    for file in sorted(os.listdir(src)):
        if not file.lower().endswith(".png"):
            continue
        name = os.path.splitext(file)[0]
        path = os.path.join(src, file)
        asset_path = f"{TEX_DIR}/{name}"
        kind = "normal" if name.endswith("_Normal") else ("linear" if name.endswith("_ORT") else "color")
        digest = ss._file_hash(path)
        tex = eal.load_asset(asset_path) if eal.does_asset_exist(asset_path) else None
        imported = False
        if tex is None or force or eal.get_metadata_tag(tex, "SourceHash") != digest:
            task = unreal.AssetImportTask()
            task.filename = path
            task.destination_path = TEX_DIR
            task.destination_name = name
            task.automated = True
            task.replace_existing = True
            task.save = False
            tools.import_asset_tasks([task])
            tex = eal.load_asset(asset_path)
            imported = True
        if not isinstance(tex, unreal.Texture2D):
            unreal.log_error(f"ShortStack: importing {file} produced no texture")
            continue
        fixed = _texture_format(tex, kind)
        if imported or fixed:
            eal.set_metadata_tag(tex, "SourceHash", digest)
            eal.save_loaded_asset(tex)
            unreal.log(f"ShortStack: {'imported' if imported else 'fixed'} Patches/{name}")
            done += 1
    return done


def _atlas(key):
    path = f"{TEX_DIR}/T_PatchAtlas_{key}"
    return eal.load_asset(path) if eal.does_asset_exist(path) else None


# ---------------------------------------------------------------- M_Patch


def build_patch(force=False):
    """M_Patch and its four instances. Returns how many assets it built or changed."""
    lay = _layout()
    wide, rnd = lay["wide"], lay["round"]
    code_uv = PATCH_UV
    for key, value in (("last", len(lay["brands"]) - 1), ("rcols", rnd["cols"]), ("rs", rnd["cell"]), ("ry0", rnd["y0"]), ("wcols", wide["cols"]),
                       ("ww", wide["cell"][0]), ("wh", wide["cell"][1]), ("wy0", wide.get("y0", 0)), ("half_swatch", lay["swatch"] * 0.5),
                       ("size", lay["size"])):
        code_uv = code_uv.replace("{" + key + "}", f"{float(value):.1f}")
    textures = {k: _atlas(k) for k in ("BaseColor", "Normal", "ORT")}
    stamp = VERSION + "|" + json.dumps([wide, rnd, lay["swatch"], lay["size"], len(lay["brands"])], sort_keys=True) + "|" + \
        ",".join(k for k, t in textures.items() if t is not None)
    made = 0
    mat = _material("M_Patch", force, stamp)
    if mat:
        _set(mat, "used_with_instanced_static_meshes", True)
        uv0 = ss._expr(mat, unreal.MaterialExpressionTextureCoordinate, -1700, -200)
        cpd = ss._scalar(mat, "BrandIndexCPD", 0.0, -1700, -100)
        # The brand by Custom Primitive Data 0 (index + 1) when the component sets it: no instance per patch.
        _set(cpd, "use_custom_primitive_data", True)
        _set(cpd, "primitive_data_index", 0)
        index = ss._scalar(mat, "BrandIndex", 0.0, -1700, 0)
        shape = ss._scalar(mat, "Shape", 0.0, -1700, 100)
        part = ss._scalar(mat, "Part", 0.0, -1700, 200)
        use_map = ss._scalar(mat, "UseMap", 0.0, -1700, 300)
        uv = _custom(mat, code_uv, [("UV", uv0), ("Cpd", cpd), ("Index", index), ("Shape", shape), ("Part", part), ("UseMap", use_map)],
                     ss.F2, -1400, 0, "The patch's cell in the atlas")
        base = _tex(mat, "PatchBaseColor", textures["BaseColor"], COLOR, -1100, -400, uv)
        nrm = _tex(mat, "PatchNormal", textures["Normal"], NORMAL, -1100, -150, uv)
        ort = _tex(mat, "PatchORT", textures["ORT"], MASKS, -1100, 100, uv)
        pmap = _tex(mat, "PatchMap", None, COLOR, -1100, 350, uv)
        tint = ss._vector(mat, "Tint", (1.0, 1.0, 1.0), -1100, 600)
        rough = ss._scalar(mat, "Rough", 0.0, -1100, 700)
        strength = ss._scalar(mat, "Strength", 1.0, -1100, 800)
        wraps = ss._scalar(mat, "Wraps", 14.0, -1100, 900)
        common = [("UV", uv0), ("Part", part), ("UseMap", use_map), ("Wraps", wraps)]
        surf = _custom(mat, PATCH_SURFACE, [("Base", (base, "RGB")), ("Map", (pmap, "RGB")), ("Ort", (ort, "RGB")), ("Tint", tint), ("Rough", rough)] + common,
                       ss.F4, -700, -300, "The patch's colour and roughness")
        n = _custom(mat, PATCH_NORMAL, [("N", (nrm, "RGB")), ("Strength", strength)] + common, ss.F3, -700, 0, "The patch's relief")
        extra = _custom(mat, PATCH_EXTRA, [("Ort", (ort, "RGB"))] + common, ss.F2, -700, 300, "Occlusion and specular")
        _prop(ss._mask(mat, surf, "rgb", -400, -350), unreal.MaterialProperty.MP_BASE_COLOR)
        _prop(ss._mask(mat, surf, "a", -400, -250), unreal.MaterialProperty.MP_ROUGHNESS)
        _prop(n, unreal.MaterialProperty.MP_NORMAL)
        _prop(ss._mask(mat, extra, "r", -400, 250), unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
        _prop(ss._mask(mat, extra, "g", -400, 350), unreal.MaterialProperty.MP_SPECULAR)
        _finish(mat, stamp)
        made += 1
    parent = eal.load_asset(f"{ss.MAT_DIR}/M_Patch")
    if parent is None:
        return made
    for shape_name, shape in (("Wide", 0.0), ("Round", 1.0)):
        for part_name, part_value in (("Face", 0.0), ("Thread", 1.0)):
            _, changed = _instance(f"{ss.MAT_DIR}/MI_Patch_{shape_name}_{part_name}", parent, {"Shape": shape, "Part": part_value})
            made += 1 if changed else 0
    return made


def assign_patches(force=False):
    """The patch meshes' slots get M_Patch's instances (the face, the merrowed edge), whatever the import gave them."""
    changed = 0
    for name, shape in PATCH_MESHES.items():
        mesh = _mesh(name)
        if mesh is None:
            continue
        dirty = False
        for i, slot in enumerate(_slot_names(mesh)):
            part = "Thread" if "thread" in slot.lower() else "Face"
            mi = eal.load_asset(f"{ss.MAT_DIR}/MI_Patch_{shape}_{part}")
            if mi is not None and mesh.get_material(i) != mi:
                mesh.set_material(i, mi)
                dirty = True
        if dirty:
            eal.save_loaded_asset(mesh)
            unreal.log(f"ShortStack: {name} wears M_Patch")
            changed += 1
    return changed


# ---------------------------------------------------------------- M_Chip


def _chip_textures(name):
    """The baked maps a chip mesh's import brought (T_chip_<denomination>_BaseColor, _ORM, _Normal), by name."""
    found = {}
    folder = f"{ss.MESH_DIR}/{name}"
    for asset_path in eal.list_assets(folder, recursive=True, include_folder=False):
        data = eal.find_asset_data(asset_path)
        if str(data.asset_class_path.asset_name) != "Texture2D":
            continue
        found[str(data.asset_name)] = asset_path.split(".")[0]
    return found


def _sampler_for(tex):
    """The sampler type a texture needs, from its own settings (the glTF importer's, which this doesn't change)."""
    cs = tex.get_editor_property("compression_settings")
    if cs == unreal.TextureCompressionSettings.TC_NORMALMAP:
        return NORMAL
    if cs == unreal.TextureCompressionSettings.TC_MASKS:
        return MASKS
    return COLOR if tex.get_editor_property("srgb") else LINEAR


def _chip_sets():
    """Per chip mesh, per slot: (slot index, {BaseColor, ORM, Normal: texture})."""
    sets = {}
    for name in CHIP_MESHES:
        mesh = _mesh(name)
        if mesh is None:
            continue
        textures = _chip_textures(name)
        slots = []
        for i, slot in enumerate(_slot_names(mesh)):
            m = re.search(r"chip_(T\w+?)(?:_\d+)?$", slot) or re.search(r"chip_(T[0-9]+K?)", slot)
            key = m.group(1) if m else name.replace("SM_Chip_", "")
            maps = {}
            for kind in ("BaseColor", "ORM", "Normal"):
                # T_chip_<key>_<kind>, as chips.py's bake names them (the importer may add to the name).
                path = textures.get(f"T_chip_{key}_{kind}") or next((p for n, p in sorted(textures.items())
                                                                     if f"chip_{key}_" in n and n.endswith(kind)), None)
                maps[kind] = eal.load_asset(path) if path else None
            slots.append((i, key, maps))
        sets[name] = (mesh, slots)
    return sets


def build_chip(force=False, sets=None):
    """M_Chip, its sampler types read off the chips' own maps. Returns how many it built."""
    sets = sets if sets is not None else _chip_sets()
    sample = None
    for _mesh_obj, slots in sets.values():
        for _i, _key, maps in slots:
            if all(maps.values()):
                sample = maps
                break
        if sample:
            break
    defaults = {"BaseColor": COLOR, "ORM": MASKS, "Normal": NORMAL}
    samplers = {k: (_sampler_for(sample[k]) if sample else d) for k, d in defaults.items()}
    stamp = VERSION + "|" + ",".join(f"{k}:{str(v).split('.')[-1]}" for k, v in samplers.items()) + ("|maps" if sample else "|nomaps")
    mat = _material("M_Chip", force, stamp)
    if not mat:
        return 0
    _set(mat, "used_with_instanced_static_meshes", True)
    _set(mat, "used_with_nanite", True)
    uv = ss._expr(mat, unreal.MaterialExpressionTextureCoordinate, -1100, 0)
    base = _tex(mat, "BaseColorTexture", sample["BaseColor"] if sample else None, samplers["BaseColor"], -800, -300, uv)
    orm = _tex(mat, "ORMTexture", sample["ORM"] if sample else None, samplers["ORM"], -800, 0, uv)
    nrm = _tex(mat, "NormalTexture", sample["Normal"] if sample else None, samplers["Normal"], -800, 300, uv)
    tint = ss._vector(mat, "Tint", (1.0, 1.0, 1.0), -800, -500)
    col = ss._expr(mat, unreal.MaterialExpressionMultiply, -450, -350)
    mel.connect_material_expressions(base, "RGB", col, "A")
    ss._link(tint, col, "B")
    _prop(col, unreal.MaterialProperty.MP_BASE_COLOR)
    rough = _custom(mat, "return saturate(R + Rough);", [("R", (orm, "G")), ("Rough", ss._scalar(mat, "Rough", 0.0, -800, 150))], ss.F1, -450, 0,
                    "Clay's roughness")
    _prop(rough, unreal.MaterialProperty.MP_ROUGHNESS)
    _prop(orm, unreal.MaterialProperty.MP_AMBIENT_OCCLUSION, "R")
    _prop(orm, unreal.MaterialProperty.MP_METALLIC, "B")
    _prop(ss._scalar(mat, "Specular", 0.42, -450, 150), unreal.MaterialProperty.MP_SPECULAR)
    n = _custom(mat, CHIP_NORMAL, [("N", (nrm, "RGB")), ("Strength", ss._scalar(mat, "Strength", 1.0, -800, 450))], ss.F3, -450, 300,
                "The chip's relief")
    _prop(n, unreal.MaterialProperty.MP_NORMAL)
    _finish(mat, stamp)
    return 1


def assign_chips(force=False, sets=None):
    """Each tournament chip's slots get an instance of M_Chip with that slot's own maps (MI_<mesh>_<slot>, in its folder)."""
    sets = sets if sets is not None else _chip_sets()
    parent = eal.load_asset(f"{ss.MAT_DIR}/M_Chip")
    if parent is None:
        return 0
    changed = 0
    for name, (mesh, slots) in sets.items():
        dirty = False
        for i, key, maps in slots:
            if not all(maps.values()):
                unreal.log_warning(f"ShortStack: {name} slot {i} ({key}): its maps aren't all imported; it keeps the art's material")
                continue
            mi, made = _instance(f"{ss.MESH_DIR}/{name}/MI_{name}_{key}", parent,
                                 textures={"BaseColorTexture": maps["BaseColor"], "ORMTexture": maps["ORM"], "NormalTexture": maps["Normal"]})
            changed += 1 if made else 0
            if mi is not None and mesh.get_material(i) != mi:
                mesh.set_material(i, mi)
                dirty = True
        if dirty:
            eal.save_loaded_asset(mesh)
            unreal.log(f"ShortStack: {name} wears M_Chip")
            changed += 1
    return changed


# ---------------------------------------------------------------- run


def run(force=False):
    """The segment's art: the patch atlas's maps, M_Patch on the patches, M_Chip on the tournament chips. Returns how many
    assets it changed."""
    if not eal.does_directory_exist(ss.MAT_DIR):
        eal.make_directory(ss.MAT_DIR)
    changed = 0
    for step in (import_patch_textures, build_patch, assign_patches):
        try:
            changed += int(step(force) or 0)
        except Exception as exc:  # the patches keep the art's plain materials without it
            unreal.log_error(f"ShortStack: segment_art_setup.{step.__name__} failed: {exc}")
    try:
        sets = _chip_sets()
        changed += build_chip(force, sets)
        changed += assign_chips(force, sets)
    except Exception as exc:  # the chips keep the art's own (glTF) materials without it
        unreal.log_error(f"ShortStack: segment_art_setup chips failed: {exc}")
    if changed:
        unreal.log(f"ShortStack: segment_art_setup changed {changed} assets")
    return changed
