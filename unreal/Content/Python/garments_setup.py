"""
SHORT STACK: the street hero's real clothes (art/blender/assets/garments.py), imported onto the hero MetaHumans' skeletons.

For each body type (A, B) the Blender build writes unreal/Art/Garments/SK_Garment_Jeans_<A|B>.fbx and
SK_Garment_Jacket_<A|B>.fbx (skeletal meshes on the hero body's own armature: every bone, its names and bind pose) and their
baked maps in unreal/Art/Garments/Textures. This imports each as a SkeletalMesh whose Skeleton is that hero body's
skeleton, so AShortStackCharacter can run it with the body as its leader pose:

    /Game/ShortStack/Garments/SK_Garment_Jeans_A          (slots: 0 denim, 1 the button's metal)
    /Game/ShortStack/Garments/SK_Garment_Jacket_A         (slots: 0 shell, 1 rib knit, 2 zip)
    /Game/ShortStack/Garments/Textures/T_SK_Garment_*      BaseColor (sRGB), ORM (linear), Normal (flipped green: Blender's
                                                           maps are OpenGL), and the tiling weave details
    /Game/ShortStack/Garments/Materials/M_Garment          the master: BaseColor x Tint, ORM, Normal + a tiling weave
    /Game/ShortStack/Garments/Materials/MI_<mesh>_<slot>   one instance per slot

The jacket's shell bakes light neutral grey: the game sets Tint on slot 0 to the creator's outfit color (and a darker
shade of it on the rib). Re-running is cheap: a garment is imported again only when its FBX or one of its maps changed
(an md5 of them all is kept on the mesh as SourceHash metadata), as shortstack_setup does for the props.

    import garments_setup
    garments_setup.run()                 # what changed
    garments_setup.run(force=True)       # everything again
    garments_setup.export_sources("C:/somewhere")   # the hero bodies and their default outfits, for the Blender build

The FBX goes through the legacy FBX importer (its options set the target Skeleton); Interchange is switched off for the
import and restored after. Needs the hero MetaHumans built (hero_cast.py): the skeleton is read from MHC_HeroA1 / MHC_HeroB1.
"""
import hashlib
import json
import os

import unreal

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary

GARMENT_DIR = "/Game/ShortStack/Garments"
TEX_DIR = f"{GARMENT_DIR}/Textures"
MAT_DIR = f"{GARMENT_DIR}/Materials"
MASTER = f"{MAT_DIR}/M_Garment"
# The hero each body type's garments were fitted to; the others of that type share its skeleton's bone names.
HEROES = {"A": ("HeroA1", "HeroA0", "HeroA2"), "B": ("HeroB1", "HeroB0", "HeroB2")}
GARMENTS = ("Jeans", "Jacket")
# Slots: (suffix, what it is, which tiling detail, its strength).
SLOTS = {
    "Jeans": ((0, "denim", "T_Garment_Detail_Twill_N", 0.55), (1, "metal", None, 0.0)),
    "Jacket": ((0, "shell", "T_Garment_Detail_Canvas_N", 0.45), (1, "rib", None, 0.0), (2, "zip", None, 0.0)),
}


def _src_dir():
    return os.path.join(unreal.Paths.project_dir(), "Art", "Garments")


def _md5(paths):
    h = hashlib.md5()
    for p in paths:
        with open(p, "rb") as f:
            h.update(f.read())
    return h.hexdigest()


def _meta():
    """The Blender build's notes (garments.json): UV scale per slot, for the weave detail's tiling."""
    p = os.path.join(_src_dir(), "garments.json")
    if not os.path.isfile(p):
        return {}
    with open(p, "r", encoding="utf-8") as f:
        return json.load(f)


def hero_skeleton(body_type):
    """The skeleton of the hero body the garments were fitted to (or another hero of the same body type)."""
    for hero in HEROES[body_type]:
        path = f"/Game/ShortStack/Cast/Built/MHC_{hero}/Body/SKM_MHC_{hero}_BodyMesh"
        if eal.does_asset_exist(path):
            mesh = eal.load_asset(path)
            skel = mesh.get_editor_property("skeleton") if mesh else None
            if skel:
                return skel, path
    return None, None


# ---------------------------------------------------------------- textures


def _import_texture(path, kind, force):
    name = os.path.splitext(os.path.basename(path))[0]
    asset_path = f"{TEX_DIR}/{name}"
    digest = _md5([path])
    if not force and eal.does_asset_exist(asset_path):
        existing = eal.load_asset(asset_path)
        if existing and eal.get_metadata_tag(existing, "SourceHash") == digest:
            return existing
    task = unreal.AssetImportTask()
    task.filename = path
    task.destination_path = TEX_DIR
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    tex = eal.load_asset(asset_path)
    if not isinstance(tex, unreal.Texture2D):
        unreal.log_error(f"ShortStack: importing {path} produced no texture")
        return None
    if kind == "normal":
        tex.set_editor_property("srgb", False)
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        tex.set_editor_property("flip_green_channel", True)   # Blender bakes OpenGL normals; Unreal's are DirectX
    elif kind == "linear":
        tex.set_editor_property("srgb", False)
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
    eal.set_metadata_tag(tex, "SourceHash", digest)
    eal.save_loaded_asset(tex)
    return tex


def _texture_kind(file):
    if file.endswith("_Normal.png") or file.endswith("_N.png"):
        return "normal"
    if file.endswith("_ORM.png"):
        return "linear"
    return "color"


# ---------------------------------------------------------------- material


def _expr(mat, cls, x, y):
    return mel.create_material_expression(mat, cls, x, y)


def _tex_param(mat, name, default, sampler, x, y):
    e = _expr(mat, unreal.MaterialExpressionTextureSampleParameter2D, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("sampler_type", sampler)
    if default:
        e.set_editor_property("texture", default)
    return e


def build_master(force=False):
    """M_Garment: BaseColor x Tint (the outfit color; white leaves the bake as it is), ORM to occlusion, roughness and
    metallic, the baked normal with a tiling weave normal folded in (UVs x DetailTiling, DetailStrength). Two-sided: the
    inside shows at the cuffs, the collar and the hems."""
    if eal.does_asset_exist(MASTER) and not force:
        return eal.load_asset(MASTER)
    if eal.does_asset_exist(MASTER):
        mat = eal.load_asset(MASTER)
        mel.delete_all_material_expressions(mat)
    else:
        mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset("M_Garment", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("two_sided", True)
    white = eal.load_asset("/Engine/EngineResources/WhiteSquareTexture")
    flat = eal.load_asset("/Engine/EngineMaterials/DefaultNormal")
    S = unreal.MaterialSamplerType
    base = _tex_param(mat, "BaseColor", white, S.SAMPLERTYPE_COLOR, -900, -300)
    orm = _tex_param(mat, "ORM", white, S.SAMPLERTYPE_LINEAR_COLOR, -900, 0)
    nrm = _tex_param(mat, "Normal", flat, S.SAMPLERTYPE_NORMAL, -900, 300)
    tint = _expr(mat, unreal.MaterialExpressionVectorParameter, -600, -450)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    mul = _expr(mat, unreal.MaterialExpressionMultiply, -400, -350)
    mel.connect_material_expressions(base, "RGB", mul, "A")
    mel.connect_material_expressions(tint, "", mul, "B")
    mel.connect_material_property(mul, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(orm, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    mel.connect_material_property(orm, "G", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(orm, "B", unreal.MaterialProperty.MP_METALLIC)
    # The weave: a tiling normal at the cloth's own scale, whiteout-blended over the bake.
    uv = _expr(mat, unreal.MaterialExpressionTextureCoordinate, -1300, 600)
    tiling = _expr(mat, unreal.MaterialExpressionScalarParameter, -1300, 700)
    tiling.set_editor_property("parameter_name", "DetailTiling")
    tiling.set_editor_property("default_value", 100.0)
    uvm = _expr(mat, unreal.MaterialExpressionMultiply, -1100, 650)
    mel.connect_material_expressions(uv, "", uvm, "A")
    mel.connect_material_expressions(tiling, "", uvm, "B")
    det = _tex_param(mat, "Detail", flat, S.SAMPLERTYPE_NORMAL, -900, 600)
    mel.connect_material_expressions(uvm, "", det, "UVs")
    strength = _expr(mat, unreal.MaterialExpressionScalarParameter, -900, 850)
    strength.set_editor_property("parameter_name", "DetailStrength")
    strength.set_editor_property("default_value", 0.0)
    blend = _expr(mat, unreal.MaterialExpressionCustom, -500, 400)
    blend.set_editor_property("code", "float3 d = lerp(float3(0, 0, 1), D, saturate(S));\n"
                                      "return normalize(float3(B.xy + d.xy, B.z * d.z));")
    blend.set_editor_property("description", "Weave over the bake")
    blend.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    ins = []
    for n in ("B", "D", "S"):
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        ins.append(ci)
    blend.set_editor_property("inputs", ins)
    mel.connect_material_expressions(nrm, "RGB", blend, "B")
    mel.connect_material_expressions(det, "RGB", blend, "D")
    mel.connect_material_expressions(strength, "", blend, "S")
    mel.connect_material_property(blend, "", unreal.MaterialProperty.MP_NORMAL)
    spec = _expr(mat, unreal.MaterialExpressionConstant, -400, 150)
    spec.set_editor_property("r", 0.3)   # cloth reflects less than the default 0.5
    mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
    mat.set_editor_property("used_with_skeletal_mesh", True)
    mel.layout_material_expressions(mat)
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat)
    unreal.log(f"ShortStack: built {MASTER}")
    return mat


def _instance(name, master, textures, detail, strength, tiling):
    path = f"{MAT_DIR}/{name}"
    if eal.does_asset_exist(path):
        mi = eal.load_asset(path)
    else:
        mi = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, MAT_DIR, unreal.MaterialInstanceConstant,
                                                                    unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, master)
    for param, tex in textures.items():
        if tex:
            mel.set_material_instance_texture_parameter_value(mi, param, tex)
    if detail:
        mel.set_material_instance_texture_parameter_value(mi, "Detail", detail)
    mel.set_material_instance_scalar_parameter_value(mi, "DetailStrength", strength if detail else 0.0)
    mel.set_material_instance_scalar_parameter_value(mi, "DetailTiling", tiling)
    eal.save_loaded_asset(mi)
    return mi


# ---------------------------------------------------------------- meshes


def _fbx_options(skeleton):
    ui = unreal.FbxImportUI()
    sets = [("import_mesh", True), ("import_as_skeletal", True), ("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH),
            ("skeleton", skeleton), ("import_materials", False), ("import_textures", False), ("import_animations", False),
            ("create_physics_asset", False), ("automated_import_should_detect_type", False)]
    for k, v in sets:
        try:
            ui.set_editor_property(k, v)
        except Exception as exc:
            unreal.log_warning(f"ShortStack: FBX option {k}: {exc}")
    data = ui.get_editor_property("skeletal_mesh_import_data")
    for k, v in (("import_morph_targets", False), ("use_t0_as_ref_pose", False), ("update_skeleton_reference_pose", False),
                 ("convert_scene", True), ("force_front_x_axis", False),
                 ("normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS),
                 ("preserve_smoothing_groups", True)):
        try:
            data.set_editor_property(k, v)
        except Exception as exc:
            unreal.log_warning(f"ShortStack: FBX skeletal option {k}: {exc}")
    return ui


INTERCHANGE_FBX = "Interchange.FeatureFlags.Import.FBX"


def _interchange_fbx(on):
    """Interchange takes FBX by default in 5.6; the legacy importer is the one whose options name the Skeleton. Sets the
    flag and returns what it was."""
    try:
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    except Exception:
        world = None
    try:
        was = bool(unreal.SystemLibrary.get_console_variable_bool_value(INTERCHANGE_FBX))
    except Exception:
        was = True
    unreal.SystemLibrary.execute_console_command(world, f"{INTERCHANGE_FBX} {1 if on else 0}")
    return was


def import_garment(name, body_type, force=False):
    """Imports one garment's FBX onto the hero skeleton, its maps, its materials. Returns the mesh, or None."""
    src = _src_dir()
    fbx = os.path.join(src, f"{name}.fbx")
    if not os.path.isfile(fbx):
        return None
    tex_dir = os.path.join(src, "Textures")
    maps = sorted(f for f in os.listdir(tex_dir) if f.startswith(f"T_{name}_") and f.endswith(".png")) if os.path.isdir(tex_dir) else []
    details = sorted(f for f in os.listdir(tex_dir) if f.startswith("T_Garment_Detail_")) if os.path.isdir(tex_dir) else []
    digest = _md5([fbx] + [os.path.join(tex_dir, f) for f in maps + details])
    mesh_path = f"{GARMENT_DIR}/{name}"
    if not force and eal.does_asset_exist(mesh_path):
        existing = eal.load_asset(mesh_path)
        if existing and eal.get_metadata_tag(existing, "SourceHash") == digest:
            return existing
    skeleton, body = hero_skeleton(body_type)
    if skeleton is None:
        unreal.log_error(f"ShortStack: no hero body of type {body_type} is built (hero_cast.py): can't import {name}")
        return None
    task = unreal.AssetImportTask()
    task.filename = fbx
    task.destination_path = GARMENT_DIR
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = False
    task.options = _fbx_options(skeleton)
    was = _interchange_fbx(False)
    try:
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    finally:
        _interchange_fbx(was)
    mesh = eal.load_asset(mesh_path)
    if not isinstance(mesh, unreal.SkeletalMesh):
        unreal.log_error(f"ShortStack: importing {fbx} produced no skeletal mesh at {mesh_path}")
        return None
    if mesh.get_editor_property("skeleton") != skeleton:
        unreal.log_error(f"ShortStack: {name} came in on {mesh.get_editor_property('skeleton')}, not {body}'s skeleton")
    # Its maps and materials.
    tex = {f: _import_texture(os.path.join(tex_dir, f), _texture_kind(f), force) for f in maps + details}
    master = build_master()
    meta = _meta().get(name, {})
    mats = list(mesh.get_editor_property("materials"))
    garment = "Jacket" if "Jacket" in name else "Jeans"
    for index, what, detail_name, strength in SLOTS[garment]:
        mi = _instance(f"MI_{name}_{what}", master,
                       {"BaseColor": tex.get(f"T_{name}_{index}_BaseColor.png"), "ORM": tex.get(f"T_{name}_{index}_ORM.png"),
                        "Normal": tex.get(f"T_{name}_{index}_Normal.png")},
                       tex.get(f"{detail_name}.png") if detail_name else None, strength,
                       float(meta.get("detail_tiling", {}).get(str(index), 100.0)))
        slot = _slot_of(mats, index)
        if slot is None:
            unreal.log_warning(f"ShortStack: {name} has no material slot for {what} (_{index})")
            continue
        mats[slot].set_editor_property("material_interface", mi)
    mesh.set_editor_property("materials", mats)
    eal.set_metadata_tag(mesh, "SourceHash", digest)
    eal.set_metadata_tag(mesh, "ShortStackBody", body)
    eal.save_loaded_asset(mesh)
    unreal.log(f"ShortStack: imported {name} onto {body}'s skeleton")
    return mesh


def _slot_of(mats, index):
    """The slot whose name (the Blender material, M_<mesh>_<index>) ends in _<index>; index itself when none does."""
    for i, m in enumerate(mats):
        n = str(m.get_editor_property("material_slot_name"))
        if n.endswith(f"_{index}"):
            return i
    return index if index < len(mats) else None


def run(force=False):
    """Imports every garment the Blender build has written. Returns how many came in (or were already current)."""
    if not os.path.isdir(_src_dir()):
        return 0
    # The master once (rebuilt with force, so a change to its graph reaches every instance).
    build_master(force)
    done = 0
    for body_type in HEROES:
        for g in GARMENTS:
            if import_garment(f"SK_Garment_{g}_{body_type}", body_type, force):
                done += 1
    return done


# ---------------------------------------------------------------- sources for the Blender build


def export_sources(out_dir, heroes=("HeroA1", "HeroB1")):
    """Writes each hero's body mesh and default outfit as FBX (<Hero>_SKM_MHC_<Hero>_BodyMesh.fbx, <Hero>_MHC_<Hero>_Outfits.fbx),
    what garments.py fits the clothes to (its --wardrobe directory)."""
    os.makedirs(out_dir, exist_ok=True)
    written = []
    for hero in heroes:
        for asset, file in ((f"/Game/ShortStack/Cast/Built/MHC_{hero}/Body/SKM_MHC_{hero}_BodyMesh", f"{hero}_SKM_MHC_{hero}_BodyMesh.fbx"),
                            (f"/Game/ShortStack/Cast/Built/MHC_{hero}/Clothing/MHC_{hero}_Outfits", f"{hero}_MHC_{hero}_Outfits.fbx")):
            obj = eal.load_asset(asset)
            if obj is None:
                unreal.log_warning(f"ShortStack: {asset} isn't built")
                continue
            task = unreal.AssetExportTask()
            task.object = obj
            task.filename = os.path.join(out_dir, file)
            task.automated = True
            task.replace_identical = True
            task.prompt = False
            opts = unreal.FbxExportOption()
            opts.set_editor_property("collision", False)
            opts.set_editor_property("level_of_detail", False)
            task.options = opts
            if unreal.Exporter.run_asset_export_task(task):
                written.append(task.filename)
    return written
