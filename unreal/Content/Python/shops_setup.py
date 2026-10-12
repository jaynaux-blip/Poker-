"""
SHORT STACK: the shops' own materials (world_realism_spec.md 0.5, 3.4, 4.5, 4.6, 7.5.5), ue-frame's. All procedural
where they can be, the art's own texture carried into them at play:

  M_NS_Mirror      [mirror] slots (the barber's backbar, the thrift's three-way, the gym's wall): opaque, metallic 1,
                   roughness 0.02, the backing gone at the edges by the art's mask (EdgeMask: the slot's own base color,
                   white silver, dark where it's desilvered). With UseCapture 1 it shows the SceneCapture fallback's
                   picture instead (Capture, read at the pixel's place on the screen, flipped: spec 3.4's fallback, while
                   the hero is in the barber's chair or at the fitting mirror).
  M_NS_BarberPole  the pole's striped insert (UV0: u once round it, v meters up, 0..0.62): red, white and blue stripes
                   climbing it as Phase grows (the game turns it while the shop's open), lit from inside by Lit.
  M_NS_Belt        the treadmills' belts: their tread (BaseColorTexture, T_NS_GymBelt tiling along V) run along V by Pan
                   (meters, the game runs it while someone's on it), times PanScale (V per meter).
  M_NS_ClothTint   [tint] cloth (the thrift's racks and folded stacks): the slot's light neutral grey bake
                   (BaseColorTexture) times the instance's color (per-instance custom data 0..2), or Tint on a single mesh.

AShopStage gives an imported mesh's slots these at play by their slot names (ns_mirror, ns_barberpole, ns_belt; the
racks' cloth by the thrift's stage), carrying the art's texture over: nothing is reassigned at import, and a mesh imported
before this ran picks them up the next time the street builds.

shortstack_setup.run() runs this with every setup (run_realism). By hand:

    import shops_setup; shops_setup.run(force=True)
"""
import unreal

import shortstack_setup as ss

# Bump when a material's graph changes: each built material carries it (metadata) and is rebuilt when it differs.
VERSION = "1"
VERSION_TAG = "ShopsVersion"

mel = unreal.MaterialEditingLibrary

WHITE = "/Engine/EngineResources/WhiteSquareTexture"
BLACK = "/Engine/EngineResources/Black"

MIRROR = """
// Mask: the art's desilvering (1 silver .. 0 gone); Cap: the capture's picture; Use: 1 shows the capture.
float silver = saturate(Mask);
float3 col = Silver * lerp(0.28, 1.0, silver);
float rough = lerp(0.42, Rough, silver);
return float4(lerp(col, float3(0.0, 0.0, 0.0), Use), lerp(rough, 1.0, Use));
"""

FLIP = """
// The capture is rendered from the eye reflected in the glass: its picture is the mirror's, flipped side to side.
return float2(1.0 - UV.x, UV.y);
"""

POLE = """
// The insert's stripes: a helix (one and a half turns over its 62 cm), red, white, blue, white, twice round.
float f = UV.x + UV.y / 0.62 * 1.5 + Phase;
float k = frac(f * 2.0) * 4.0;
float band = floor(k);
float e = frac(k);
float3 red = float3(0.62, 0.035, 0.045);
float3 white = float3(0.86, 0.85, 0.81);
float3 blue = float3(0.035, 0.09, 0.42);
float3 col = band < 1.0 ? red : (band < 2.0 ? white : (band < 3.0 ? blue : white));
float3 next = band < 1.0 ? white : (band < 2.0 ? blue : (band < 3.0 ? white : red));
// Painted edges, a hair soft.
col = lerp(col, next, smoothstep(0.94, 1.0, e));
return col;
"""


def _material(name, force):
    """The material to build: new, forced, or built by an older version of this file (rebuilt in place, so whatever holds
    it stays valid). None when it's current."""
    path = f"{ss.MAT_DIR}/{name}"
    if ss.eal.does_asset_exist(path) and not force:
        existing = ss.eal.load_asset(path)
        if existing is not None and ss.eal.get_metadata_tag(existing, VERSION_TAG) == VERSION:
            return None
    return ss._new_material(name, True)


def _finish(mat):
    ss.eal.set_metadata_tag(mat, VERSION_TAG, VERSION)
    return ss._finish(mat)


def _texture(mat, name, default, x, y):
    """A texture parameter (Color sampler) with its default."""
    e = ss._expr(mat, unreal.MaterialExpressionTextureSampleParameter2D, x, y)
    e.set_editor_property("parameter_name", name)
    tex = unreal.load_asset(default)
    if tex:
        e.set_editor_property("texture", tex)
    return e


def _connect(src, out, dst, inp):
    if not mel.connect_material_expressions(src, out, dst, inp):
        unreal.log_warning(f"ShortStack: could not connect {out or 'out'} to {inp}")


def build_mirror(force):
    mat = _material("M_NS_Mirror", force)
    if not mat:
        return 0
    mat.set_editor_property("used_with_nanite", True)
    edge = _texture(mat, "EdgeMask", WHITE, -1000, -300)
    # The capture: its picture at the pixel's place on the screen, flipped.
    screen = ss._expr(mat, unreal.MaterialExpressionScreenPosition, -1300, 150)
    flip = ss._custom(mat, FLIP, ["UV"], ss.F2, -1100, 150, "The mirror's picture, flipped")
    _connect(screen, "ViewportUV", flip, "UV")
    cap = _texture(mat, "Capture", BLACK, -850, 150)
    _connect(flip, "", cap, "UVs")
    use = ss._scalar(mat, "UseCapture", 0.0, -1000, 400)
    c = ss._custom(mat, MIRROR, ["Mask", "Silver", "Rough", "Use"], ss.F4, -500, -200, "Silvered glass, its edges desilvered")
    _connect(edge, "R", c, "Mask")
    ss._link(ss._vector(mat, "Silver", (0.93, 0.935, 0.94), -1000, -100), c, "Silver")
    ss._link(ss._scalar(mat, "Roughness", 0.02, -1000, 0), c, "Rough")
    ss._link(use, c, "Use")
    mel.connect_material_property(ss._mask(mat, c, "rgb", -250, -250), "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(ss._mask(mat, c, "a", -250, -100), "", unreal.MaterialProperty.MP_ROUGHNESS)
    metal = ss._expr(mat, unreal.MaterialExpressionOneMinus, -250, 0)
    ss._link(use, metal, "")
    mel.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)
    glow = ss._expr(mat, unreal.MaterialExpressionMultiply, -250, 150)
    _connect(cap, "RGB", glow, "A")
    ss._link(use, glow, "B")
    mel.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    _finish(mat)
    return 1


def build_pole(force):
    mat = _material("M_NS_BarberPole", force)
    if not mat:
        return 0
    c = ss._custom(mat, POLE, ["UV", "Phase"], ss.F3, -500, -100, "The barber pole's stripes")
    ss._link(ss._expr(mat, unreal.MaterialExpressionTextureCoordinate, -900, -200), c, "UV")
    ss._link(ss._scalar(mat, "Phase", 0.0, -900, -100), c, "Phase")
    mel.connect_material_property(c, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(ss._scalar(mat, "Roughness", 0.32, -900, 50), "", unreal.MaterialProperty.MP_ROUGHNESS)
    # Lit from inside while it's open (the game sets Lit 1 open, 0 shut).
    glow = ss._expr(mat, unreal.MaterialExpressionMultiply, -250, 150)
    ss._link(c, glow, "A")
    ss._link(ss._scalar(mat, "Lit", 1.0, -900, 200), glow, "B")
    lit = ss._expr(mat, unreal.MaterialExpressionMultiply, -100, 150)
    ss._link(glow, lit, "A")
    ss._link(ss._scalar(mat, "Glow", 0.8, -900, 300), lit, "B")
    mel.connect_material_property(lit, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    _finish(mat)
    return 1


def build_belt(force):
    mat = _material("M_NS_Belt", force)
    if not mat:
        return 0
    uv = ss._expr(mat, unreal.MaterialExpressionTextureCoordinate, -1100, -200)
    pan = ss._expr(mat, unreal.MaterialExpressionMultiply, -1100, 0)
    ss._link(ss._scalar(mat, "Pan", 0.0, -1300, -50), pan, "A")
    ss._link(ss._scalar(mat, "PanScale", 1.0, -1300, 50), pan, "B")
    shift = ss._expr(mat, unreal.MaterialExpressionAppendVector, -900, 0)
    ss._link(ss._expr(mat, unreal.MaterialExpressionConstant, -1100, 100), shift, "A")
    ss._link(pan, shift, "B")
    moved = ss._expr(mat, unreal.MaterialExpressionAdd, -750, -100)
    ss._link(uv, moved, "A")
    ss._link(shift, moved, "B")
    tread = _texture(mat, "BaseColorTexture", WHITE, -550, -100)
    _connect(moved, "", tread, "UVs")
    tint = ss._expr(mat, unreal.MaterialExpressionMultiply, -250, -100)
    _connect(tread, "RGB", tint, "A")
    ss._link(ss._vector(mat, "Tint", (1.0, 1.0, 1.0), -550, 150), tint, "B")
    mel.connect_material_property(tint, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(ss._scalar(mat, "Roughness", 0.85, -550, 300), "", unreal.MaterialProperty.MP_ROUGHNESS)
    _finish(mat)
    return 1


def build_cloth(force):
    mat = _material("M_NS_ClothTint", force)
    if not mat:
        return 0
    mat.set_editor_property("used_with_instanced_static_meshes", True)
    base = _texture(mat, "BaseColorTexture", WHITE, -800, -200)
    # Each instance's color (custom data 0..2), or the Tint parameter on a mesh that isn't instanced.
    color = ss._expr(mat, unreal.MaterialExpressionPerInstanceCustomData3Vector, -800, 100)
    color.set_editor_property("data_index", 0)
    ss._link(ss._vector(mat, "Tint", (1.0, 1.0, 1.0), -1100, 100), color, "DefaultValue")
    dyed = ss._expr(mat, unreal.MaterialExpressionMultiply, -400, -100)
    _connect(base, "RGB", dyed, "A")
    ss._link(color, dyed, "B")
    mel.connect_material_property(dyed, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(ss._scalar(mat, "Roughness", 0.86, -800, 300), "", unreal.MaterialProperty.MP_ROUGHNESS)
    _finish(mat)
    return 1


def run(force=False):
    """Builds the shops' materials that are missing or out of date (all of them with force). Returns how many."""
    if not ss.eal.does_directory_exist(ss.MAT_DIR):
        ss.eal.make_directory(ss.MAT_DIR)
    built = 0
    for build in (build_mirror, build_pole, build_belt, build_cloth):
        try:
            built += build(force)
        except Exception as exc:  # each shop keeps the art's own material for that slot without it
            unreal.log_error(f"ShortStack: shops_setup.{build.__name__} failed: {exc}")
    if built:
        unreal.log(f"ShortStack: shops_setup built {built} materials")
    return built
