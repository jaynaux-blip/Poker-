"""
SHORT STACK: the Street level (Fifth Street, the Lucky Penny #212).

Builds M_Street (brick, wet asphalt with puddles, sidewalk slabs, the store's floor tiles, concrete; all
procedural HLSL like M_Surface) and /Game/Maps/Street with the AStreetStage actor and the
AStreetGameMode override. shortstack_setup.run() calls this when the editor opens; anything that exists
is left alone.

    import street_setup; street_setup.build_materials(force=True); street_setup.build_map(force=True)
"""
import unreal

import shortstack_setup as ss

MAP_PATH = "/Game/Maps/Street"

STREET = ss.NOISE + """
// P: world position (cm), N: world normal. Returns base color (rgb) and roughness (a).
float3 m = P / 100.0;
float3 an = abs(N);
bool wall = an.z < 0.5;
float2 uv = an.z > max(an.x, an.y) ? m.xy : (an.x > an.y ? m.yz : m.xz);
float3 col = BaseIn;
float r = RoughIn;
int pat = (int)(PatternIn + 0.5);
float wet = saturate(WetIn);
float puddle = 0.0;
if (pat == 1)
{
    // Brick, running bond: 21 x 6.5 cm with 1 cm of mortar; each brick its own shade; soot and rain streaks.
    float2 b = float2(uv.x / 0.22, uv.y / 0.075);
    float row = floor(b.y);
    b.x += fmod(row, 2.0) * 0.5;
    float2 cell = floor(b);
    float2 f = frac(b);
    float mortar = (f.x < 0.045 || f.y < 0.13) ? 1.0 : 0.0;
    float tone = 0.78 + 0.32 * S.Hash(cell);
    float burnt = S.Hash(cell + 17.0) > 0.88 ? 0.7 : 1.0;
    col = lerp(col * tone * burnt, float3(0.42, 0.40, 0.37), mortar);
    float streak = S.Fbm(float2(uv.x * 6.0, uv.y * 0.6)) * saturate(1.0 - m.z / 9.0);
    col *= 1.0 - 0.35 * streak;
    r = lerp(0.85, 0.95, mortar);
    // Rain darkens the bottom of the wall most.
    float damp = wet * saturate(1.0 - m.z / 3.0);
    col *= 1.0 - 0.3 * damp;
    r -= 0.35 * damp;
}
else if (pat == 2)
{
    // Asphalt: aggregate, tar seams, oil stains, and puddles where the road dips.
    float agg = S.Value(uv * 140.0);
    float seam = saturate(1.0 - abs(S.Fbm(uv * 0.9) - 0.5) * 70.0);
    float oil = saturate((S.Fbm(uv * 0.7 + 30.0) - 0.62) * 5.0);
    col *= 0.8 + 0.35 * agg;
    col = lerp(col, float3(0.02, 0.02, 0.022), seam * 0.8 + oil * 0.4);
    r = 0.75 + 0.15 * agg;
    puddle = saturate((S.Fbm(uv * 0.35 + 7.0) - (0.62 - 0.22 * wet)) * 9.0) * step(0.01, wet);
}
else if (pat == 3)
{
    // Sidewalk: 1.5 m slabs with joints, old gum, the odd cracked slab.
    float2 s = uv / 1.5;
    float2 f = frac(s);
    float joint = (f.x < 0.008 || f.y < 0.008) ? 1.0 : 0.0;
    float slab = S.Hash(floor(s));
    float gum = step(0.985, S.Hash(floor(uv * 12.0) + 3.0));
    float crack = slab > 0.85 ? saturate(1.0 - abs(S.Fbm(uv * 3.0) - 0.5) * 90.0) : 0.0;
    col *= 0.86 + 0.18 * slab + 0.1 * S.Fbm(uv * 8.0);
    col = lerp(col, float3(0.08, 0.08, 0.08), max(joint, crack) * 0.7);
    col = lerp(col, float3(0.2, 0.2, 0.2), gum * 0.6);
    r = 0.8;
    puddle = saturate((S.Fbm(uv * 0.5 + 11.0) - (0.72 - 0.15 * wet)) * 7.0) * step(0.01, wet);
}
else if (pat == 4)
{
    // The store's floor: 30 cm vinyl tiles, two tones, scuffed, freshly mopped.
    float2 t = floor(uv / 0.3);
    float checker = fmod(t.x + t.y, 2.0);
    float2 f = frac(uv / 0.3);
    float grout = (f.x < 0.02 || f.y < 0.02) ? 1.0 : 0.0;
    col *= lerp(1.0, 0.82, checker);
    float scuff = saturate((S.Fbm(uv * 4.0) - 0.6) * 4.0);
    col = lerp(col, col * 0.7, scuff * 0.5 + grout * 0.5);
    r = 0.22 + 0.25 * scuff + 0.3 * grout;
}
else if (pat == 5)
{
    // Concrete: blotchy, a little rough.
    col *= 0.85 + 0.25 * S.Fbm(uv * 3.0) + 0.05 * S.Value(uv * 80.0);
    r = 0.8;
}
if (pat == 2 || pat == 3 || pat == 5)
{
    // Wet: darker and glossier everywhere, mirror-flat in the puddles.
    col *= lerp(1.0, 0.62, wet);
    r = lerp(r, r * 0.4, wet);
    col = lerp(col, col * 0.45, puddle);
    r = lerp(r, 0.03, puddle);
}
return float4(col, saturate(r));
"""


def build_street(force):
    mat = ss._new_material("M_Street", force)
    if not mat:
        return 0
    mat.set_editor_property("used_with_instanced_static_meshes", True)
    base = ss._vector(mat, "BaseColor", (0.5, 0.5, 0.5), -900, -200)
    rough = ss._scalar(mat, "Roughness", 0.6, -900, 0)
    metal = ss._scalar(mat, "Metallic", 0.0, -900, 100)
    pattern = ss._scalar(mat, "Pattern", 0.0, -900, 200)
    emissive = ss._scalar(mat, "Emissive", 0.0, -900, 300)
    wet = ss._scalar(mat, "Wet", 0.85, -900, 400)
    wp = ss._world_pos(mat, -900, -400)
    n = ss._expr(mat, unreal.MaterialExpressionVertexNormalWS, -900, -300)
    c = ss._custom(mat, STREET, ["P", "N", "BaseIn", "RoughIn", "PatternIn", "WetIn"], ss.F4, -500, -200, "ShortStack street patterns")
    ss._link(wp, c, "P")
    ss._link(n, c, "N")
    ss._link(base, c, "BaseIn")
    ss._link(rough, c, "RoughIn")
    ss._link(pattern, c, "PatternIn")
    ss._link(wet, c, "WetIn")
    rgb = ss._mask(mat, c, "rgb", -250, -250)
    a = ss._mask(mat, c, "a", -250, -100)
    unreal.MaterialEditingLibrary.connect_material_property(rgb, "", unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.connect_material_property(a, "", unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)
    glow = ss._expr(mat, unreal.MaterialExpressionMultiply, -250, 250)
    ss._link(base, glow, "A")
    ss._link(emissive, glow, "B")
    unreal.MaterialEditingLibrary.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    ss._finish(mat)
    return 1


def build_materials(force=False):
    return build_street(force)


def build_map(force=False):
    """/Game/Maps/Street: the stage actor and the game mode override (the stage builds everything else)."""
    if ss.eal.does_asset_exist(MAP_PATH) and not force:
        return False
    stage_class = unreal.load_class(None, "/Script/ShortStack.StreetStage")
    if not stage_class:
        unreal.log_error("ShortStack: StreetStage class not found (is the C++ module built?)")
        return False
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if ss.eal.does_asset_exist(MAP_PATH):
        levels.load_level(MAP_PATH)
    else:
        levels.new_level(MAP_PATH)
    if not any(a.get_class() == stage_class for a in actors.get_all_level_actors()):
        actors.spawn_actor_from_class(stage_class, unreal.Vector(0.0, 0.0, 0.0), unreal.Rotator(0.0, 0.0, 0.0))
    mode_class = unreal.load_class(None, "/Script/ShortStack.StreetGameMode")
    if mode_class:
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        world.get_world_settings().set_editor_property("default_game_mode", mode_class)
    levels.save_current_level()
    unreal.log(f"ShortStack: created {MAP_PATH}")
    return True


def rebuild_stage():
    """After new props or materials: rebuild the street's stage actor in the open level."""
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    stage_class = unreal.load_class(None, "/Script/ShortStack.StreetStage")
    for a in actors.get_all_level_actors():
        if stage_class and a.get_class() == stage_class:
            a.rebuild_set()


def refresh_map():
    """Rebuilds the street's stage in /Game/Maps/Street and saves it, then goes back to the level that was open.

    The stage builds its props when the level is built in the editor, and Play travels into the saved level,
    so props imported after the map was saved only show up once it's rebuilt. shortstack_setup.run() calls
    this when it imported meshes; it does nothing if the open level has unsaved edits (run it by hand then).
    """
    if not ss.eal.does_asset_exist(MAP_PATH):
        return False
    if unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages():
        unreal.log_warning("ShortStack: the open level has unsaved changes; run street_setup.refresh_map() to rebuild the Street level")
        return False
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    previous = world.get_outermost().get_name() if world else ""
    levels.load_level(MAP_PATH)
    rebuild_stage()
    levels.save_current_level()
    if previous and previous != MAP_PATH and ss.eal.does_asset_exist(previous):
        levels.load_level(previous)
    unreal.log("ShortStack: rebuilt the Street level with the imported props")
    return True

