"""
SHORT STACK: materials and map for The Back Room (the Spin Cycle Club behind the laundromat).

Called by shortstack_setup.run(); like it, it only builds what is missing unless forced.

    import backroom_setup; backroom_setup.run(force=True)
"""
import unreal

import shortstack_setup as ss

MAP_PATH = "/Game/Maps/BackRoom"
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary

# ---------------------------------------------------------------- HLSL

# World-space surfaces for the room shell. Heights are in meters; Normal() turns the height field into
# a bumped world normal with screen-space derivatives (Mikkelsen's surface gradient), so mortar
# joints, saw cuts and tile grids catch the light without any textures.
ROOM_LIB = """
struct SSRoom
{
    float Hash(float2 p) { float3 p3 = frac(float3(p.xyx) * 0.1031); p3 += dot(p3, p3.yzx + 33.33); return frac((p3.x + p3.y) * p3.z); }
    float Value(float2 p)
    {
        float2 i = floor(p); float2 f = frac(p); float2 u = f * f * (3.0 - 2.0 * f);
        return lerp(lerp(Hash(i), Hash(i + float2(1, 0)), u.x), lerp(Hash(i + float2(0, 1)), Hash(i + float2(1, 1)), u.x), u.y);
    }
    float Fbm(float2 p) { float s = 0.0; float a = 0.5; for (int k = 0; k < 5; k++) { s += a * Value(p); p = p * 2.03 + 11.7; a *= 0.5; } return s; }
    float2 H22(float2 p) { float3 p3 = frac(float3(p.xyx) * float3(0.1031, 0.1030, 0.0973)); p3 += dot(p3, p3.yzx + 33.33); return frac((p3.xx + p3.yz) * p3.zy); }
    // Distance to the nearest Voronoi edge (cracks).
    float VoronoiEdge(float2 p)
    {
        float2 n = floor(p); float2 f = frac(p); float2 mr = 0; float md = 8.0;
        for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++)
        {
            float2 g = float2(i, j); float2 r = g + H22(n + g) - f; float d = dot(r, r);
            if (d < md) { md = d; mr = r; }
        }
        md = 8.0;
        for (int j2 = -1; j2 <= 1; j2++) for (int i2 = -1; i2 <= 1; i2++)
        {
            float2 g = float2(i2, j2); float2 r = g + H22(n + g) - f;
            if (dot(mr - r, mr - r) > 0.00001) md = min(md, dot(0.5 * (mr + r), normalize(r - mr)));
        }
        return md;
    }
    float2 Plane(float3 m, float3 N)
    {
        float3 an = abs(N);
        return an.z > max(an.x, an.y) ? m.xy : (an.x > an.y ? m.yz : m.xz);
    }

    // col, rough and height (m) for pattern pat at world position P (cm).
    void Eval(float3 P, float3 N, float pat, float3 base, float3 base2, float split, out float3 col, out float rough, out float h)
    {
        float3 m = P / 100.0;
        float2 uv = Plane(m, N);
        col = base; rough = 0.8; h = 0.0;
        int k = (int)(pat + 0.5);
        if (k == 1)
        {
            // Painted cinderblock, running bond: 8 x 16 in blocks, 3/8 in mortar joints set back 4 mm.
            float bw = 0.4064, bh = 0.2032;
            float row = floor(uv.y / bh);
            float2 q = float2(uv.x + fmod(abs(row), 2.0) * bw * 0.5, uv.y);
            float2 cell = floor(q / float2(bw, bh));
            float2 f = q - cell * float2(bw, bh);
            float d = min(min(f.x, bw - f.x), min(f.y, bh - f.y));
            float face = smoothstep(0.0035, 0.0085, d);
            float br = Hash(cell + 7.1);
            float pores = Fbm(uv * 160.0);
            float pits = step(0.94, Hash(floor(uv * 300.0)));
            // Two-tone institutional paint, glossier below the line.
            float lower = 1.0 - smoothstep(split - 0.004, split + 0.004, m.z);
            float3 paint = lerp(base, base2, lower);
            float peel = Fbm(uv * 3.4 + 17.0);
            float peeled = smoothstep(0.735, 0.75, peel);
            float peelEdge = saturate(1.0 - abs(peel - 0.742) * 90.0);
            col = paint * (0.95 + 0.07 * br) * (0.97 + 0.05 * pores);
            col = lerp(col, float3(0.47, 0.45, 0.41) * (0.8 + 0.4 * pores), peeled);
            // Grime rising from the floor, water streaks running down from the ceiling.
            float low = 1.0 - smoothstep(0.03, 0.55, m.z);
            col *= 1.0 - 0.38 * low * (0.55 + 0.45 * Fbm(uv * 5.0));
            float streak = smoothstep(0.6, 0.76, Fbm(float2(uv.x * 11.0, uv.y * 0.7 + 3.0))) * smoothstep(1.3, 2.6, m.z);
            col = lerp(col, col * float3(0.74, 0.68, 0.55), streak * 0.65);
            col = lerp(col * float3(0.62, 0.61, 0.58), col, face);
            rough = lerp(0.88, lerp(0.58, 0.46, lower), face) + peeled * 0.25 + streak * 0.08;
            h = face * (0.004 + 0.0014 * pores - 0.0012 * pits) + peelEdge * 0.0005 - peeled * 0.0004;
        }
        else if (k == 2)
        {
            // Poured concrete: trowel marks, aggregate, saw cuts every 3 m, sparse cracks, old stains.
            float mott = Fbm(uv * 1.2);
            float trowel = Fbm(uv * float2(8.0, 3.0) + mott * 2.0);
            float agg = step(0.86, Hash(floor(uv * 700.0)));
            col = base * (0.82 + 0.3 * mott) * (0.95 + 0.07 * trowel);
            col = lerp(col, col * 1.3, agg * 0.35);
            float st = Fbm(uv * 0.8 + 3.0);
            float stain = smoothstep(0.58, 0.76, st);
            col *= 1.0 - 0.38 * stain;
            float2 jf = abs(frac(uv / 3.0 + 0.5) - 0.5) * 3.0;
            float joint = 1.0 - smoothstep(0.0025, 0.005, min(jf.x, jf.y));
            float crack = (1.0 - smoothstep(0.0, 0.006, VoronoiEdge(uv * 1.6))) * smoothstep(0.58, 0.7, Fbm(uv * 0.6 + 9.0));
            float cut = max(joint, crack);
            col *= 1.0 - 0.65 * cut;
            // Scuffed smooth where chairs and feet move around the table.
            float worn = 1.0 - smoothstep(1.2, 2.0, length(m.xy));
            rough = 0.84 - 0.1 * mott - 0.12 * stain - 0.12 * worn;
            col = lerp(col, col * 1.08, worn * 0.5);
            h = -0.003 * cut + 0.0005 * trowel + 0.0002 * agg;
        }
        else if (k == 3)
        {
            // Drop ceiling: 2 x 2 ft mineral tiles in a white T-bar grid, some water-stained or sagging.
            float t = 0.6096;
            float2 tile = floor(uv / t);
            float2 f = uv - tile * t;
            float d = min(min(f.x, t - f.x), min(f.y, t - f.y));
            float bar = 1.0 - smoothstep(0.0115, 0.0125, d);
            float tr = Hash(tile + 3.3);
            float fis = Fbm(uv * 70.0);
            col = lerp(base * (0.88 + 0.12 * tr) * (0.92 + 0.1 * fis), float3(0.72, 0.71, 0.68), bar);
            float2 c = f / t - 0.5 + (H22(tile) - 0.5) * 0.3;
            float stainR = length(c) + 0.05 * Fbm(uv * 12.0);
            float stained = step(0.78, tr) * (1.0 - bar);
            float ring = smoothstep(0.02, 0.0, abs(stainR - 0.3)) * 0.6 + smoothstep(0.32, 0.1, stainR) * 0.35;
            col = lerp(col, col * float3(0.66, 0.55, 0.38), ring * stained);
            col *= lerp(1.0, 0.8, step(0.93, tr) * (1.0 - bar));  // dirtier, sagging tiles
            rough = lerp(0.96, 0.42, bar);
            h = bar * 0.006 - 0.0012 * fis * (1.0 - bar);
        }
        else if (k == 4)
        {
            // Painted steel (doors, machine cabinets, pipes): orange peel, chips down to rust.
            float chips = Fbm(uv * 38.0 + 5.0);
            float chip = smoothstep(0.79, 0.805, chips);
            float rust = Fbm(uv * 60.0);
            col = lerp(base, lerp(float3(0.23, 0.12, 0.06), float3(0.4, 0.22, 0.1), rust), chip);
            float grime = smoothstep(0.55, 0.8, Fbm(uv * 4.0 + 2.0));
            col *= 1.0 - 0.3 * grime;
            rough = lerp(0.42, 0.85, chip) + grime * 0.2;
            h = -chip * 0.0004 + (Fbm(uv * 500.0) - 0.5) * 0.00008;
        }
    }

    float3 Normal(float3 P, float3 N, float h)
    {
        float hc = h * 100.0; // cm, like P
        float3 dpdx = ddx(P), dpdy = ddy(P);
        float dhx = ddx(hc), dhy = ddy(hc);
        float3 r1 = cross(dpdy, N), r2 = cross(N, dpdx);
        float det = dot(dpdx, r1);
        float3 grad = sign(det) * (dhx * r1 + dhy * r2);
        return normalize(abs(det) * N - grad);
    }
};
SSRoom R;
float3 col; float rough; float h;
R.Eval(P, normalize(N), PatternIn, BaseIn, Base2In, SplitIn, col, rough, h);
"""

ROOM_COLOR = ROOM_LIB + "return float4(col, saturate(rough));\n"
ROOM_NORMAL = ROOM_LIB + "return R.Normal(P, normalize(N), h);\n"

F3 = unreal.CustomMaterialOutputType.CMOT_FLOAT3
F4 = unreal.CustomMaterialOutputType.CMOT_FLOAT4


def build_room(force):
    """M_Room: the Back Room's walls, floor, ceiling and painted steel (Pattern 1-4), with bump normals."""
    mat = ss._new_material("M_Room", force)
    if not mat:
        return False
    mat.set_editor_property("tangent_space_normal", False)
    base = ss._vector(mat, "BaseColor", (0.6, 0.6, 0.6), -1100, -300)
    base2 = ss._vector(mat, "BaseColor2", (0.3, 0.4, 0.33), -1100, -150)
    split = ss._scalar(mat, "Split", 1.2, -1100, 0)
    pattern = ss._scalar(mat, "Pattern", 1.0, -1100, 100)
    metal = ss._scalar(mat, "Metallic", 0.0, -1100, 200)
    inputs = ["P", "N", "BaseIn", "Base2In", "SplitIn", "PatternIn"]
    nodes = []
    for code, out, y, desc in ((ROOM_COLOR, F4, -250, "Room surfaces"), (ROOM_NORMAL, F3, 150, "Room surface normal")):
        c = ss._custom(mat, code, inputs, out, -600, y, desc)
        ss._link(ss._world_pos(mat, -1100, -500), c, "P")
        ss._link(ss._expr(mat, unreal.MaterialExpressionVertexNormalWS, -1100, -420), c, "N")
        ss._link(base, c, "BaseIn")
        ss._link(base2, c, "Base2In")
        ss._link(split, c, "SplitIn")
        ss._link(pattern, c, "PatternIn")
        nodes.append(c)
    rgb = ss._mask(mat, nodes[0], "rgb", -300, -300)
    a = ss._mask(mat, nodes[0], "a", -300, -150)
    mel.connect_material_property(rgb, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(a, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)
    mel.connect_material_property(nodes[1], "", unreal.MaterialProperty.MP_NORMAL)
    return ss._finish(mat)


def build_materials(force=False):
    built = import_cards(force)
    for build in (build_room, build_card):
        try:
            if build(force):
                built += 1
        except Exception as exc:
            unreal.log_error(f"ShortStack: {build.__name__} failed: {exc}")
    return built


def build_map(force=False):
    """/Game/Maps/BackRoom: the stage actor and a game mode override (the stage builds everything else)."""
    if eal.does_asset_exist(MAP_PATH) and not force:
        return
    stage_class = unreal.load_class(None, "/Script/ShortStack.BackRoomStage")
    if not stage_class:
        unreal.log_error("ShortStack: BackRoomStage class not found (is the C++ module built?)")
        return
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if eal.does_asset_exist(MAP_PATH):
        levels.load_level(MAP_PATH)
    else:
        levels.new_level(MAP_PATH)
    if not any(a.get_class() == stage_class for a in actors.get_all_level_actors()):
        actors.spawn_actor_from_class(stage_class, unreal.Vector(0.0, 0.0, 0.0), unreal.Rotator(0.0, 0.0, 0.0))
    mode_class = unreal.load_class(None, "/Script/ShortStack.BackRoomGameMode")
    if mode_class:
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        world.get_world_settings().set_editor_property("default_game_mode", mode_class)
    levels.save_current_level()
    unreal.log(f"ShortStack: created {MAP_PATH}")


# ---------------------------------------------------------------- cards

# art/blender/assets/cards.py: the card spans u in [CARD_U0, CARD_U1] of its 1024 square face texture.
CARD_W, CARD_H, CARD_R = 6.35, 8.89, 0.32
CARD_U0 = (CARD_H - CARD_W) / 2 / CARD_H
CARDS_SRC = "Art/Cards"
CARDS_DIR = "/Game/ShortStack/Cards"

# The peek: the card curls around a hinge line across it. Points past the hinge (t < Hinge along the
# bend direction D) wrap onto a cylinder of Radius until they reach the Lift angle, then run straight.
# Up is +1 when the face is up and -1 when the card lies face down (so the lift is toward the ceiling).
CARD_BEND = """
float2 D = float2(cos(BendAngle), sin(BendAngle));
float t = dot(LP.xy, D);
float s = Hinge - t;
float3 outP = LP;
float phi = 0.0;
if (s > 0.0 && Lift > 0.0001)
{
    float R = max(Radius, 0.2);
    float arc = Lift * R;
    float u, z;
    if (s <= arc) { phi = s / R; u = Hinge - R * sin(phi); z = R * (1.0 - cos(phi)); }
    else { phi = Lift; u = Hinge - R * sin(Lift) - (s - arc) * cos(Lift); z = R * (1.0 - cos(Lift)) + (s - arc) * sin(Lift); }
    float2 perp = LP.xy - t * D;
    outP = float3(perp + u * D, LP.z + z * Up);
}
"""
CARD_WPO = CARD_BEND + "return outP - LP;\n"
CARD_NORMAL = CARD_BEND + """
float3 nTop = float3(Up * D * sin(phi), cos(phi));
return (Side > 0.5 && Side < 1.5) ? -nTop : nTop;
"""
# Face and back: the print, with the rounded corners masked. The edge is plain card stock.
CARD_SURFACE = """
float x = (UV.x - U0) / (1.0 - 2.0 * U0) * W;
float y = UV.y * H;
float2 q = abs(float2(x, y) - float2(W, H) * 0.5) - (float2(W, H) * 0.5 - Rc);
float d = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - Rc;
float mask = Side > 1.5 ? 1.0 : step(d, 0.0);
float3 col = Side > 1.5 ? float3(0.86, 0.84, 0.8) : Tex;
return float4(col, mask);
"""
CARD_PICK = "return Side > 1.5 ? VN : BN;\n"


def _local_to_world(mat, x, y):
    t = ss._expr(mat, unreal.MaterialExpressionTransform, x, y)
    t.set_editor_property("transform_source_type", unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_LOCAL)
    t.set_editor_property("transform_type", unreal.MaterialVectorCoordTransform.TRANSFORM_WORLD)
    return t


def build_card(force):
    """M_Card: one material for a card's face (Side 0), back (1) and edge (2), bent by the peek."""
    mat = ss._new_material("M_Card", force)
    if not mat:
        return False
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    mat.set_editor_property("tangent_space_normal", False)
    side = ss._scalar(mat, "Side", 0.0, -1400, -500)
    params = {}
    for i, (n, v) in enumerate((("Lift", 0.0), ("Radius", 2.5), ("Hinge", 0.0), ("BendAngle", 1.5708), ("Up", 1.0))):
        params[n] = ss._scalar(mat, n, v, -1400, -400 + i * 80)
    lp = ss._expr(mat, unreal.MaterialExpressionLocalPosition, -1400, 100)

    def bend_node(code, y, desc):
        c = ss._custom(mat, code, ["LP", "Lift", "Radius", "Hinge", "BendAngle", "Up", "Side"], F3, -900, y, desc)
        ss._link(lp, c, "LP")
        for n, e in params.items():
            ss._link(e, c, n)
        ss._link(side, c, "Side")
        return c

    to_world = _local_to_world(mat, -600, -300)
    ss._link(bend_node(CARD_WPO, -300, "Card bend (local offset)"), to_world, "")
    mel.connect_material_property(to_world, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)

    n_world = _local_to_world(mat, -600, 0)
    ss._link(bend_node(CARD_NORMAL, 0, "Card bend (local normal)"), n_world, "")
    pick = ss._custom(mat, CARD_PICK, ["BN", "VN", "Side"], F3, -350, 50, "Edge keeps its normal")
    ss._link(n_world, pick, "BN")
    ss._link(ss._expr(mat, unreal.MaterialExpressionVertexNormalWS, -600, 120), pick, "VN")
    ss._link(side, pick, "Side")
    mel.connect_material_property(pick, "", unreal.MaterialProperty.MP_NORMAL)

    tex = ss._expr(mat, unreal.MaterialExpressionTextureSampleParameter2D, -1100, 400)
    tex.set_editor_property("parameter_name", "Face")
    if eal.does_asset_exist(f"{CARDS_DIR}/T_Card_back"):
        tex.set_editor_property("texture", eal.load_asset(f"{CARDS_DIR}/T_Card_back"))
    uv = ss._expr(mat, unreal.MaterialExpressionTextureCoordinate, -1400, 400)
    surf = ss._custom(mat, CARD_SURFACE, ["UV", "Tex", "Side", "U0", "W", "H", "Rc"], F4, -700, 400, "Card print and corners")
    ss._link(uv, surf, "UV")
    ss._link(tex, surf, "Tex")
    ss._link(side, surf, "Side")
    for n, v, y in (("U0", CARD_U0, 500), ("W", CARD_W, 560), ("H", CARD_H, 620), ("Rc", CARD_R, 680)):
        c = ss._expr(mat, unreal.MaterialExpressionConstant, -1000, y)
        c.set_editor_property("r", v)
        ss._link(c, surf, n)
    mel.connect_material_property(ss._mask(mat, surf, "rgb", -400, 380), "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(ss._mask(mat, surf, "a", -400, 460), "", unreal.MaterialProperty.MP_OPACITY_MASK)
    mel.connect_material_property(ss._scalar(mat, "Roughness", 0.3, -400, 540), "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(ss._scalar(mat, "Specular", 0.45, -400, 600), "", unreal.MaterialProperty.MP_SPECULAR)
    return ss._finish(mat)


def import_cards(force=False):
    """Imports the deck's faces (unreal/Art/Cards/T_Card_*.png) into /Game/ShortStack/Cards when they change."""
    import os
    src = os.path.join(unreal.Paths.project_dir(), CARDS_SRC)
    if not os.path.isdir(src):
        return 0
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    imported = 0
    for file in sorted(os.listdir(src)):
        if not file.lower().endswith(".png"):
            continue
        name = os.path.splitext(file)[0]
        path = os.path.join(src, file)
        digest = ss._file_hash(path)
        asset_path = f"{CARDS_DIR}/{name}"
        if not force and eal.does_asset_exist(asset_path):
            existing = eal.load_asset(asset_path)
            if existing and eal.get_metadata_tag(existing, "SourceHash") == digest:
                continue
        task = unreal.AssetImportTask()
        task.filename = path
        task.destination_path = CARDS_DIR
        task.destination_name = name
        task.automated = True
        task.replace_existing = True
        task.save = True
        tools.import_asset_tasks([task])
        tex = eal.load_asset(asset_path)
        if not tex:
            unreal.log_error(f"ShortStack: importing {file} failed")
            continue
        tex.set_editor_property("srgb", True)
        eal.set_metadata_tag(tex, "SourceHash", digest)
        eal.save_loaded_asset(tex)
        imported += 1
    if imported:
        unreal.log(f"ShortStack: imported {imported} card faces into {CARDS_DIR}")
    return imported


def fix_meshes():
    """Import settings the glTF importer can't take: the card bends in its material, so no Nanite."""
    path = "/Game/ShortStack/Meshes/SM_Card/SM_Card"
    if not eal.does_asset_exist(path):
        return 0
    card = eal.load_asset(path)
    settings = card.get_editor_property("nanite_settings")
    if not settings.enabled:
        return 0
    settings.enabled = False
    card.set_editor_property("nanite_settings", settings)
    eal.save_loaded_asset(card)
    return 1
