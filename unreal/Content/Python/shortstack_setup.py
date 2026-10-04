"""
SHORT STACK editor setup.

Builds the project's materials (all procedural, written as HLSL in Custom
nodes), imports the Blender props from unreal/Art/Meshes, and creates the
NightOne map with the apartment placed. init_unreal.py runs this when the editor opens; anything that
already exists is left alone.

Rebuild everything from the editor's Python console (Output Log > Python):

    import shortstack_setup; shortstack_setup.run(force=True)
"""
import unreal

MAT_DIR = "/Game/ShortStack/Materials"
MAP_PATH = "/Game/Maps/NightOne"

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

# ---------------------------------------------------------------- HLSL

NOISE = """
struct SSNoise
{
    float Hash(float2 p) { float3 p3 = frac(float3(p.xyx) * 0.1031); p3 += dot(p3, p3.yzx + 33.33); return frac((p3.x + p3.y) * p3.z); }
    float Value(float2 p)
    {
        float2 i = floor(p); float2 f = frac(p); float2 u = f * f * (3.0 - 2.0 * f);
        return lerp(lerp(Hash(i), Hash(i + float2(1, 0)), u.x), lerp(Hash(i + float2(0, 1)), Hash(i + float2(1, 1)), u.x), u.y);
    }
    float Fbm(float2 p) { float s = 0.0; float a = 0.5; for (int k = 0; k < 5; k++) { s += a * Value(p); p *= 2.03; a *= 0.5; } return s; }
    float H11(float p) { p = frac(p * 0.1031); p *= p + 33.33; p *= p + p; return frac(p); }
    float2 H22(float2 p) { float3 p3 = frac(float3(p.xyx) * float3(0.1031, 0.1030, 0.0973)); p3 += dot(p3, p3.yzx + 33.33); return frac((p3.xx + p3.yz) * p3.zy); }
};
SSNoise S;
"""

SURFACE = NOISE + """
// P: world position (cm), N: world normal. Returns base color (rgb) and roughness (a).
float3 m = P / 100.0;
float3 an = abs(N);
float2 uv = an.z > max(an.x, an.y) ? m.xy : (an.x > an.y ? m.yz : m.xz);
float3 col = BaseIn;
float r = RoughIn;
int pat = (int)(PatternIn + 0.5);
if (pat == 1)
{
    // Old plaster: paint over plaster, grime, peeling patches, water stains near the ceiling.
    float plaster = S.Fbm(uv * 18.0);
    float grime = S.Fbm(uv * 1.7 + 10.0);
    float peel = S.Fbm(uv * 3.4 + 40.0);
    float stain = S.Fbm(uv * 1.1 + 3.0) * saturate((m.z - 1.5) * 1.3);
    float peeled = saturate((peel - 0.62) * 9.0);
    float edge = saturate(1.0 - abs(peel - 0.62) * 60.0) * 0.6;
    col = lerp(col, col * float3(0.87, 0.78, 0.72), peeled);
    col *= 0.82 + 0.18 * grime - 0.06 * plaster;
    float sk = saturate((stain - 0.45) * 3.0) * 0.45;
    col = col * (1.0 - sk * float3(0.35, 0.42, 0.55)) + sk * float3(0.06, 0.04, 0.01);
    col -= edge * 0.05;
    r = lerp(0.8, 0.95, peeled) + (grime - 0.5) * 0.08;
}
else if (pat == 2)
{
    // Walnut veneer: rings warped by noise, fine grain, pores, sun-faded wear.
    float warp = S.Fbm(float2(uv.x * 0.8, uv.y * 3.0)) * 9.0;
    float rings = sin((uv.y * 9.0 + warp) * 3.14159);
    float grain = pow(0.5 + 0.5 * rings, 2.2);
    float fine = S.Fbm(float2(uv.x * 2.0, uv.y * 90.0));
    float wear = saturate((S.Fbm(uv * 2.5 + 9.0) - 0.55) * 4.0);
    col *= 0.7 + 0.25 * grain + 0.2 * fine;
    col = lerp(col, col * 1.25 + 0.02, wear * 0.5);
    r = 0.45 + 0.16 * (1.0 - grain) + wear * 0.2;
}
else if (pat == 3)
{
    // Floorboards running toward the window, with gaps and end joints.
    float pw = 0.16;
    float plank = floor(uv.y / pw);
    float pu = frac(uv.y / pw);
    float offset = S.Hash(float2(plank, 3.1)) * 3.0;
    float seg = floor(uv.x * 0.9 + offset);
    float tone = 0.75 + 0.35 * S.Hash(float2(plank * 7.3, seg * 3.1));
    float grain = S.Fbm(float2(uv.x * 3.0, uv.y * 60.0 + plank * 5.0));
    float gap = (pu < 0.02 || pu > 0.98) ? 1.0 : 0.0;
    float joint = abs(frac(uv.x * 0.9 + offset) - 0.5) > 0.495 ? 1.0 : 0.0;
    col *= tone * (0.6 + 0.4 * grain) * ((gap + joint) > 0.0 ? 0.3 : 1.0);
    r = 0.55 + grain * 0.25 + gap * 0.2;
}
else if (pat == 4)
{
    // Brushed aluminium.
    float b = S.Fbm(float2(uv.x * 400.0, uv.y * 6.0));
    col *= 0.9 + 0.2 * b;
    r = RoughIn + (b - 0.5) * 0.12;
}
else if (pat == 5)
{
    // Woven fabric.
    float w = sin(uv.x * 900.0) * sin(uv.y * 900.0);
    float blotch = S.Fbm(uv * 6.0);
    col *= 0.85 + 0.1 * w + 0.15 * blotch;
}
return float4(col, saturate(r));
"""

GLOW = """
float2 d = UV - 0.5;
float f = saturate(1.0 - length(d) * 2.0);
return ColorIn * StrengthIn * f * f;
"""

SKY = NOISE + """
float3 dir = normalize(P - Cam);
float h = clamp(dir.z, -0.1, 1.0);
// Night: sodium-orange light pollution at the horizon, deep blue overhead.
float3 horizon = lerp(float3(0.23, 0.13, 0.10), float3(0.62, 0.42, 0.44), DawnIn);
float3 zenith = lerp(float3(0.012, 0.018, 0.04), float3(0.10, 0.16, 0.30), DawnIn);
float3 col = lerp(horizon, zenith, pow(max(h, 0.0), 0.45));
// Low rain clouds drifting, lit from below by the city.
float2 cp = dir.xy / max(dir.z, 0.05) * 1.2 + float2(T * 0.012, T * 0.004);
float cloud = smoothstep(0.35, 0.8, S.Fbm(cp));
float3 cloudCol = lerp(float3(0.16, 0.10, 0.09), float3(0.45, 0.40, 0.45), DawnIn);
col = lerp(col, cloudCol * (0.6 + 0.8 * (1.0 - h)), cloud * 0.75);
col += float3(0.55, 0.6, 0.75) * FlashIn * (0.4 + cloud);
return col;
"""

CITY = NOISE + """
float seed = floor(Rand * 100.0);
float2 fc = abs(N.x) > 0.5 ? P.yz / 100.0 : P.xz / 100.0;
bool roof = N.z > 0.5;
float2 cell = float2(3.2, 3.4);
float2 id = floor(fc / cell);
float2 f = frac(fc / cell);
float win = step(0.18, f.x) * step(f.x, 0.82) * step(0.25, f.y) * step(f.y, 0.8);
float r = S.Hash(id + seed * 17.0);
float lit = step(0.62 - DawnIn * 0.25, r);
// Some windows flicker (TVs) and a few switch over time.
float tv = step(0.97, S.Hash(id * 1.7 + seed)) * (0.6 + 0.4 * sin(T * 7.0 + r * 40.0) * sin(T * 2.3 + r * 9.0));
float slow = step(0.5, frac(r * 13.0 + T * 0.002));
float3 warm = lerp(float3(1.0, 0.62, 0.3), float3(1.0, 0.82, 0.55), S.Hash(id + 3.0));
float3 wc = lerp(warm, float3(0.55, 0.75, 1.0), step(0.8, S.Hash(id + 5.0)));
float3 col = lerp(float3(0.025, 0.027, 0.035), float3(0.16, 0.17, 0.2), DawnIn) * (0.7 + 0.3 * S.Hash(float2(seed, seed)));
if (!roof) { col += win * (lit * slow + tv) * wc * lerp(1.1, 0.35, DawnIn); }
col += float3(0.5, 0.55, 0.7) * FlashIn * 0.25;
float dist = length((P - Cam).xy) / 100.0;
float fog = 1.0 - exp(-dist * 0.0055);
float3 fogCol = float3(0.2 + 0.35 * DawnIn, 0.12 + 0.3 * DawnIn, 0.1 + 0.33 * DawnIn);
return lerp(col, fogCol, clamp(fog, 0.0, 0.92));
"""

RAIN = NOISE + """
// Streaks in world space: x across (cm), z up (cm); falls at 7-11 m/s.
float x = P.y / 100.0 * 60.0 + P.x * 0.013;
float colId = floor(x);
float rr = S.H11(colId * 3.7 + floor(P.x * 0.01));
float speed = 7.0 + rr * 4.0;
float y = frac((P.z / 100.0 + T * speed) / 1.4 + rr * 7.0);
float streak = smoothstep(0.35, 0.0, y) * smoothstep(0.0, 0.02, y);
float thin = smoothstep(0.18, 0.0, abs(frac(x) - 0.5));
float on = step(0.45, rr);
return float3(0.75, 0.7, 0.85) * streak * thin * on * 0.35 * (0.35 + 0.65 * frac(colId * 13.7)) + float3(0.5, 0.5, 0.6) * FlashIn * 0.02;
"""

DROPS = NOISE + """
struct SSDrops
{
    SSNoise S;
    // Static beads: (offset.xy, mask).
    float3 Beads(float2 uv, float t, float density)
    {
        float2 g = uv * density; float2 id = floor(g); float3 acc = float3(0, 0, 0);
        for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++)
        {
            float2 cid = id + float2(i, j); float2 r = S.H22(cid);
            float2 c = cid + 0.5 + (r - 0.5) * 0.8;
            float life = frac(t * (0.05 + 0.08 * r.x) + r.y);
            float size = (0.06 + 0.3 * r.x * r.x) * smoothstep(0.0, 0.15, life) * (1.0 - smoothstep(0.85, 1.0, life)) * step(0.35, r.y);
            if (size < 0.01) continue;
            float2 d = g - c; float m = smoothstep(size, size * 0.55, length(d));
            acc.xy += d / max(size, 1e-3) * m; acc.z = max(acc.z, m);
        }
        return acc;
    }
    // Runners: one drop per column sliding in jerks, clear trail above (w).
    float4 Runners(float2 uv, float t, float columns, float aspect)
    {
        float colW = 1.0 / columns; float ci = floor(uv.x / colW); float4 acc = float4(0, 0, 0, 0);
        for (int k = -1; k <= 1; k++)
        {
            float c = ci + k; float r1 = S.H11(c * 3.7 + 1.3); float r2 = S.H11(c * 7.1 + 4.2);
            float period = 6.0 + 10.0 * r1; float p = frac(t / period + r2);
            float steps = 5.0 + floor(r1 * 5.0); float sp = p * steps;
            float stepped = (floor(sp) + smoothstep(0.55, 1.0, frac(sp))) / steps;
            float y = 1.15 - 1.4 * stepped;
            float x = (c + 0.5 + (r2 - 0.5) * 0.5) * colW + 0.004 * sin(y * 40.0 + c);
            float rad = 0.008 + 0.01 * r2;
            float2 d = float2((uv.x - x) * aspect, uv.y - y); d.y *= 0.8 + 0.4 * step(0.0, d.y);
            float m = smoothstep(rad, rad * 0.5, length(d));
            acc.xy += d / rad * m; acc.z = max(acc.z, m);
            float above = uv.y - y; float trailW = rad * 0.45 / aspect;
            float inTrail = smoothstep(trailW, trailW * 0.3, abs(uv.x - x)) * step(0.0, above) * (1.0 - smoothstep(0.0, 0.35, above));
            acc.w = max(acc.w, inTrail);
        }
        return acc;
    }
};
SSDrops D;
"""

GLASS = DROPS + """
// uv over the pane from world position relative to the pane's pivot (y across, z up).
float3 rel = P - ObjPos;
float2 uv = float2(rel.y / WidthCm + 0.5, rel.z / HeightCm + 0.5);
float3 b1 = D.Beads(float2(uv.x * AspectIn, uv.y), T, 20.0);
float3 b2 = D.Beads(float2(uv.x * AspectIn, uv.y) + 7.3, T * 0.7, 48.0) * 0.5;
float4 r1 = D.Runners(uv, T, 22.0, AspectIn);
float4 r2 = D.Runners(uv + float2(0.013, 0.0), T * 0.83 + 11.0, 37.0, AspectIn);
float water = saturate(b1.z + b2.z + r1.z + r2.z);
float clear = saturate(water + max(r1.w, r2.w));
float rim = smoothstep(0.2, 0.6, water) * (1.0 - smoothstep(0.6, 1.0, water));
float2 e = min(uv, 1.0 - uv);
float edgeDist = min(e.x * AspectIn, e.y);
float edge = smoothstep(0.0, 0.06, edgeDist);
// Condensation gathers where the glass is coldest: thick along the sill and into the corners, thinning to a
// faint bloom across the middle so the street stays readable; mottled, never a flat sheet.
float mottle = S.Fbm(float2(uv.x * AspectIn, uv.y) * 5.0 + 3.1);
float fogAmt = 0.1 + 0.5 * (1.0 - smoothstep(0.0, 0.3, uv.y)) + 0.22 * (1.0 - smoothstep(0.0, 0.16, edgeDist));
fogAmt = saturate(fogAmt * lerp(0.55, 1.3, mottle));
float fog = fogAmt * (1.0 - clear);
// What the haze glows with: the laundromat's pink low on the right, the city's cold grey everywhere else.
float2 dn = (uv - float2(0.72, 0.12)) * float2(AspectIn, 1.0);
float neon = exp(-dot(dn, dn) * 3.5);
float3 haze = lerp(float3(0.05, 0.055, 0.07), float3(0.22, 0.06, 0.13), neon);
// Drops are lenses (the refraction carries the street through them); only their lit edge shows, faintly.
float3 col = haze * fog + rim * lerp(float3(0.07, 0.08, 0.1), float3(0.2, 0.08, 0.14), neon) + float3(0.7, 0.75, 0.9) * FlashIn * 0.3;
col *= lerp(0.55, 1.0, edge);
float opacity = saturate(fog * 0.75 + rim * 0.18 + (1.0 - edge) * 0.3);
return float4(col, opacity);
"""

# Each drop bends the street behind it like a small lens: an upside-down, shrunken view through the bead and a
# sideways smear through the runners. A screen-space offset (the material's 2D-offset refraction).
GLASS_REFRACT = DROPS + """
float3 rel = P - ObjPos;
float2 uv = float2(rel.y / WidthCm + 0.5, rel.z / HeightCm + 0.5);
float3 b1 = D.Beads(float2(uv.x * AspectIn, uv.y), T, 20.0);
float3 b2 = D.Beads(float2(uv.x * AspectIn, uv.y) + 7.3, T * 0.7, 48.0) * 0.5;
float4 r1 = D.Runners(uv, T, 22.0, AspectIn);
float4 r2 = D.Runners(uv + float2(0.013, 0.0), T * 0.83 + 11.0, 37.0, AspectIn);
float2 lens = b1.xy * 0.05 + b2.xy * 0.02 + (r1.xy + r2.xy) * 0.018;
// Pane y runs up, the screen's v runs down.
return float2(lens.x, -lens.y) * Strength;
"""

COOKIE = DROPS + """
// Light function: the droplet field and the window frame projected into the room by the neon spot.
float2 uv = frac(UV);
float3 b = D.Beads(uv * float2(1.2, 1.0), T, 22.0);
float4 r = D.Runners(uv, T, 18.0, 1.2);
float water = saturate(b.z + r.z);
float caustic = smoothstep(0.6, 1.0, water) * 0.9;
float rimShadow = smoothstep(0.05, 0.4, water) * (1.0 - smoothstep(0.5, 0.9, water));
float v = 0.8 - rimShadow * 0.55 + caustic * 0.6 - r.w * 0.1;
float2 e = min(uv, 1.0 - uv);
float frame = step(0.035, e.x) * step(0.035, e.y) * step(0.012, abs(uv.x - 0.5));
return clamp(v, 0.0, 1.2) * frame;
"""

# ---------------------------------------------------------------- helpers


def _new_material(name, force):
    path = f"{MAT_DIR}/{name}"
    if eal.does_asset_exist(path):
        if not force:
            return None
        # Rebuilt in place: the open map's stage holds instances of it, so deleting it would fail.
        mat = unreal.load_asset(path)
        if mat:
            mel.delete_all_material_expressions(mat)
            return mat
        eal.delete_asset(path)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    return tools.create_asset(name, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())


def _expr(mat, cls, x, y):
    return mel.create_material_expression(mat, cls, x, y)


def _scalar(mat, name, value, x, y):
    e = _expr(mat, unreal.MaterialExpressionScalarParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", value)
    return e


def _vector(mat, name, rgb, x, y):
    e = _expr(mat, unreal.MaterialExpressionVectorParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    return e


def _custom(mat, code, inputs, output_type, x, y, description):
    e = _expr(mat, unreal.MaterialExpressionCustom, x, y)
    e.set_editor_property("code", code)
    e.set_editor_property("description", description)
    e.set_editor_property("output_type", output_type)
    ins = []
    for name in inputs:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", name)
        ins.append(ci)
    e.set_editor_property("inputs", ins)
    return e


def _mask(mat, source, channels, x, y):
    e = _expr(mat, unreal.MaterialExpressionComponentMask, x, y)
    for c in "rgba":
        e.set_editor_property(c, c in channels)
    mel.connect_material_expressions(source, "", e, "")
    return e


def _link(src, dst, input_name):
    if not mel.connect_material_expressions(src, "", dst, input_name):
        unreal.log_warning(f"ShortStack: could not connect to {input_name}")


def _finish(mat):
    mel.layout_material_expressions(mat)
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat)
    unreal.log(f"ShortStack: built {mat.get_path_name()}")
    return True


def _world_pos(mat, x, y):
    return _expr(mat, unreal.MaterialExpressionWorldPosition, x, y)


def _time(mat, x, y):
    return _expr(mat, unreal.MaterialExpressionTime, x, y)


F3 = unreal.CustomMaterialOutputType.CMOT_FLOAT3
F4 = unreal.CustomMaterialOutputType.CMOT_FLOAT4
F1 = unreal.CustomMaterialOutputType.CMOT_FLOAT1
F2 = unreal.CustomMaterialOutputType.CMOT_FLOAT2

# ---------------------------------------------------------------- materials


def build_surface(force):
    mat = _new_material("M_Surface", force)
    if not mat:
        return
    mat.set_editor_property("used_with_instanced_static_meshes", True)  # the city's rooftop beacons
    base = _vector(mat, "BaseColor", (0.5, 0.5, 0.5), -900, -200)
    rough = _scalar(mat, "Roughness", 0.6, -900, 0)
    metal = _scalar(mat, "Metallic", 0.0, -900, 100)
    pattern = _scalar(mat, "Pattern", 0.0, -900, 200)
    emissive = _scalar(mat, "Emissive", 0.0, -900, 300)
    wp = _world_pos(mat, -900, -400)
    n = _expr(mat, unreal.MaterialExpressionVertexNormalWS, -900, -300)
    c = _custom(mat, SURFACE, ["P", "N", "BaseIn", "RoughIn", "PatternIn"], F4, -500, -200, "ShortStack surface patterns")
    _link(wp, c, "P")
    _link(n, c, "N")
    _link(base, c, "BaseIn")
    _link(rough, c, "RoughIn")
    _link(pattern, c, "PatternIn")
    rgb = _mask(mat, c, "rgb", -250, -250)
    a = _mask(mat, c, "a", -250, -100)
    mel.connect_material_property(rgb, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(a, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)
    glow = _expr(mat, unreal.MaterialExpressionMultiply, -250, 250)
    _link(base, glow, "A")
    _link(emissive, glow, "B")
    mel.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    return _finish(mat)


def build_glow(force):
    mat = _new_material("M_Glow", force)
    if not mat:
        return
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    mat.set_editor_property("two_sided", True)
    uv = _expr(mat, unreal.MaterialExpressionTextureCoordinate, -700, -200)
    color = _vector(mat, "Color", (1.0, 0.2, 0.5), -700, -50)
    strength = _scalar(mat, "Strength", 1.0, -700, 100)
    c = _custom(mat, GLOW, ["UV", "ColorIn", "StrengthIn"], F3, -350, -100, "Radial glow")
    _link(uv, c, "UV")
    _link(color, c, "ColorIn")
    _link(strength, c, "StrengthIn")
    mel.connect_material_property(c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    return _finish(mat)


def _timed_inputs(mat, c, names):
    """Wire Time / Dawn / Flash into a custom node (Dawn and Flash are set by ANightOneStage)."""
    if "T" in names:
        _link(_time(mat, -900, 200), c, "T")
    if "DawnIn" in names:
        _link(_scalar(mat, "Dawn", 0.0, -900, 300), c, "DawnIn")
    if "FlashIn" in names:
        _link(_scalar(mat, "Flash", 0.0, -900, 400), c, "FlashIn")


def build_sky(force):
    mat = _new_material("M_Sky", force)
    if not mat:
        return
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("two_sided", True)
    names = ["P", "Cam", "T", "DawnIn", "FlashIn"]
    c = _custom(mat, SKY, names, F3, -450, 0, "Night sky over the city")
    _link(_world_pos(mat, -900, -100), c, "P")
    _link(_expr(mat, unreal.MaterialExpressionCameraPositionWS, -900, 0), c, "Cam")
    _timed_inputs(mat, c, names)
    mel.connect_material_property(c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    return _finish(mat)


def build_city(force):
    mat = _new_material("M_City", force)
    if not mat:
        return
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("used_with_instanced_static_meshes", True)  # the buildings are instanced
    names = ["P", "N", "Rand", "Cam", "T", "DawnIn", "FlashIn"]
    c = _custom(mat, CITY, names, F3, -450, 0, "Skyline facades with lit windows")
    _link(_world_pos(mat, -900, -300), c, "P")
    _link(_expr(mat, unreal.MaterialExpressionVertexNormalWS, -900, -200), c, "N")
    _link(_expr(mat, unreal.MaterialExpressionPerInstanceRandom, -900, -100), c, "Rand")
    _link(_expr(mat, unreal.MaterialExpressionCameraPositionWS, -900, 0), c, "Cam")
    _timed_inputs(mat, c, names)
    mel.connect_material_property(c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    return _finish(mat)


def build_rain(force):
    mat = _new_material("M_Rain", force)
    if not mat:
        return
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    mat.set_editor_property("two_sided", True)
    names = ["P", "T", "FlashIn"]
    c = _custom(mat, RAIN, names, F3, -450, 0, "Falling rain streaks")
    _link(_world_pos(mat, -900, -100), c, "P")
    _timed_inputs(mat, c, names)
    mel.connect_material_property(c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    return _finish(mat)


def build_glass(force):
    mat = _new_material("M_RainGlass", force)
    if not mat:
        return
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("two_sided", True)
    names = ["P", "ObjPos", "WidthCm", "HeightCm", "AspectIn", "T", "FlashIn"]
    c = _custom(mat, GLASS, names, F4, -450, 0, "Rain-streaked, fogged window glass")
    pos = _world_pos(mat, -900, -300)
    pivot = _expr(mat, unreal.MaterialExpressionObjectPositionWS, -900, -200)
    width = _scalar(mat, "WidthCm", 138.0, -900, -100)
    height = _scalar(mat, "HeightCm", 108.0, -900, 0)
    aspect = _scalar(mat, "Aspect", 1.28, -900, 100)
    _link(pos, c, "P")
    _link(pivot, c, "ObjPos")
    _link(width, c, "WidthCm")
    _link(height, c, "HeightCm")
    _link(aspect, c, "AspectIn")
    _timed_inputs(mat, c, names)
    rgb = _mask(mat, c, "rgb", -200, -50)
    a = _mask(mat, c, "a", -200, 100)
    mel.connect_material_property(rgb, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.connect_material_property(a, "", unreal.MaterialProperty.MP_OPACITY)
    # The drops refract what is behind the glass (a screen-space offset, so the building and the neon swim in them).
    mat.set_editor_property("refraction_method", unreal.RefractionMode.RM_2D_OFFSET)
    rnames = ["P", "ObjPos", "WidthCm", "HeightCm", "AspectIn", "T", "Strength"]
    r = _custom(mat, GLASS_REFRACT, rnames, F2, -450, 400, "Rain drops as lenses")
    _link(pos, r, "P")
    _link(pivot, r, "ObjPos")
    _link(width, r, "WidthCm")
    _link(height, r, "HeightCm")
    _link(aspect, r, "AspectIn")
    _link(_time(mat, -900, 700), r, "T")
    _link(_scalar(mat, "Refraction", 1.0, -900, 780), r, "Strength")
    mel.connect_material_property(r, "", unreal.MaterialProperty.MP_REFRACTION)
    return _finish(mat)


def build_cookie(force):
    mat = _new_material("M_RainCookie", force)
    if not mat:
        return
    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_LIGHT_FUNCTION)
    names = ["UV", "T"]
    c = _custom(mat, COOKIE, names, F1, -450, 0, "Rain on the window, projected by the neon")
    _link(_expr(mat, unreal.MaterialExpressionTextureCoordinate, -900, -100), c, "UV")
    _timed_inputs(mat, c, names)
    mel.connect_material_property(c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    return _finish(mat)


def build_widget_lit(force):
    """Lit material for printed props shown through widget components (the component fills in SlateUI)."""
    mat = _new_material("M_WidgetLit", force)
    if not mat:
        return
    tex = _expr(mat, unreal.MaterialExpressionTextureSampleParameter2D, -600, 0)
    tex.set_editor_property("parameter_name", "SlateUI")
    default_tex = unreal.load_asset("/Engine/EngineResources/DefaultTexture")
    if default_tex:
        tex.set_editor_property("texture", default_tex)
    mel.connect_material_property(tex, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(_scalar(mat, "Roughness", 0.85, -600, 250), "", unreal.MaterialProperty.MP_ROUGHNESS)
    return _finish(mat)


def build_materials(force=False):
    """Builds the materials that are missing (all of them with force). Returns how many were built."""
    if not eal.does_directory_exist(MAT_DIR):
        eal.make_directory(MAT_DIR)
    built = 0
    for build in (build_surface, build_glow, build_sky, build_city, build_rain, build_glass, build_cookie, build_widget_lit):
        try:
            if build(force):
                built += 1
        except Exception as exc:  # keep going: the game has fallbacks for every material
            unreal.log_error(f"ShortStack: {build.__name__} failed: {exc}")
    return built


def build_map(force=False):
    if eal.does_asset_exist(MAP_PATH) and not force:
        return
    stage_class = unreal.load_class(None, "/Script/ShortStack.NightOneStage")
    if not stage_class:
        unreal.log_error("ShortStack: NightOneStage class not found (is the C++ module built?)")
        return
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if eal.does_asset_exist(MAP_PATH):
        levels.load_level(MAP_PATH)
    else:
        levels.new_level(MAP_PATH)
    if not any(a.get_class() == stage_class for a in actors.get_all_level_actors()):
        actors.spawn_actor_from_class(stage_class, unreal.Vector(0.0, 0.0, 0.0), unreal.Rotator(0.0, 0.0, 0.0))
    levels.save_current_level()
    unreal.log(f"ShortStack: created {MAP_PATH}")


MESH_DIR = "/Game/ShortStack/Meshes"


def _file_hash(path):
    import hashlib
    with open(path, "rb") as f:
        return hashlib.md5(f.read()).hexdigest()


LOOK_CLASSES = ("Texture2D", "Material", "MaterialInstanceConstant")


def _looks(folder):
    """The textures and materials under a folder, by package path."""
    looks = []
    for asset_path in eal.list_assets(folder, recursive=True, include_folder=False):
        if str(eal.find_asset_data(asset_path).asset_class_path.asset_name) in LOOK_CLASSES:
            looks.append(asset_path.split(".")[0])
    return looks


def _fresh_looks(folder, path, mesh):
    """Brings a changed glb's textures and materials to its existing mesh. Importing over an existing mesh updates only
    its geometry (the textures and materials already there stay, so a rebaked print never arrives): the file is
    imported again into a scratch folder, its textures and materials take the old ones' places, and the mesh's slots
    take them; the mesh itself stays the same object, so whatever uses it still does. Skipped, with a warning, if
    something outside the folder uses one of the old ones."""
    old = _looks(folder)
    for look in old:
        for user in eal.find_package_referencers_for_asset(look, False):
            if not str(user).startswith(folder + "/"):
                unreal.log_warning(f"ShortStack: {look} is used by {user}; keeping {folder}'s old textures")
                return False
    scratch = folder + "_Fresh"
    if eal.does_directory_exist(scratch):
        eal.delete_directory(scratch)
    task = unreal.AssetImportTask()
    task.filename = path
    task.destination_path = scratch
    task.automated = True
    task.replace_existing = True
    task.save = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    fresh_mesh = None
    for asset_path in eal.list_assets(scratch, recursive=True, include_folder=False):
        asset = eal.load_asset(asset_path)
        if isinstance(asset, type(mesh)):
            fresh_mesh = asset
            break
    if fresh_mesh is None:
        unreal.log_error(f"ShortStack: importing {path} again produced no mesh")
        eal.delete_directory(scratch)
        return False
    for look in old:
        eal.delete_asset(look)
    for look in _looks(scratch):
        eal.rename_asset(look, folder + look[len(scratch):])
    if isinstance(mesh, unreal.StaticMesh):
        for i, slot in enumerate(fresh_mesh.static_materials):
            if i < len(mesh.static_materials):
                mesh.set_material(i, slot.material_interface)
    else:
        mats = mesh.get_editor_property("materials")
        for i, slot in enumerate(fresh_mesh.get_editor_property("materials")):
            if i < len(mats):
                mats[i].material_interface = slot.material_interface
        mesh.set_editor_property("materials", mats)
    eal.delete_directory(scratch)
    eal.save_directory(folder)
    return True


def import_meshes(force=False, only=None):
    """Imports the props exported by art/blender/build.py (unreal/Art/Meshes/*.glb).

    Each file lands in /Game/ShortStack/Meshes/<Name>/ with its materials and textures, and
    its mesh (static, or skeletal for a rigged file like SK_Arms) is named <Name>, which is where
    NightOneStage looks for it. A file is
    reimported only when it changed (its hash is kept as metadata on the mesh), textures and materials included (see
    _fresh_looks). `only` limits it to those names (with force, to reimport a few files).
    Returns how many files were imported.
    """
    import os
    src_dir = os.path.join(unreal.Paths.project_dir(), "Art", "Meshes")
    if not os.path.isdir(src_dir):
        return 0
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    imported = 0
    for file in sorted(os.listdir(src_dir)):
        if not file.lower().endswith(".glb"):
            continue
        name = os.path.splitext(file)[0]
        if only is not None and name not in only:
            continue
        path = os.path.join(src_dir, file)
        folder = f"{MESH_DIR}/{name}"
        mesh_path = f"{folder}/{name}"
        digest = _file_hash(path)
        reimport = eal.does_asset_exist(mesh_path)
        if reimport:
            existing = eal.load_asset(mesh_path)
            if not force and existing and eal.get_metadata_tag(existing, "SourceHash") == digest:
                continue
        task = unreal.AssetImportTask()
        task.filename = path
        task.destination_path = folder
        task.automated = True
        task.replace_existing = True
        task.save = True
        tools.import_asset_tasks([task])
        mesh = None
        for asset_path in eal.list_assets(folder, recursive=True):
            asset = eal.load_asset(asset_path)
            if isinstance(asset, (unreal.StaticMesh, unreal.SkeletalMesh)):
                mesh = asset
                if asset.get_path_name().split(".")[0] != mesh_path:
                    eal.rename_asset(asset.get_path_name().split(".")[0], mesh_path)
                    mesh = eal.load_asset(mesh_path)
                break
        if mesh is None:
            unreal.log_error(f"ShortStack: importing {file} produced no mesh")
            continue
        if reimport:
            _fresh_looks(folder, path, mesh)
        if name.startswith("SM_Wear_") and isinstance(mesh, unreal.StaticMesh):
            # Glasses and hats are small and seen up close: their own triangles, not Nanite (whose fallback for a mesh
            # this small cuts the frames' rounded corners down to chamfers).
            nanite = mesh.get_editor_property("nanite_settings")
            nanite.enabled = False
            # Through the subsystem, which rebuilds the mesh (setting the property alone keeps the fallback's triangles).
            unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem).set_nanite_settings(mesh, nanite, True)
        if isinstance(mesh, unreal.SkeletalMesh):
            try:
                _skin_material(folder, mesh)
            except Exception as exc:  # the imported glTF material still works, just without subsurface
                unreal.log_warning(f"ShortStack: skin material for {name} failed: {exc}")
        eal.set_metadata_tag(mesh, "SourceHash", digest)
        eal.save_loaded_asset(mesh)
        unreal.log(f"ShortStack: imported {file} as {mesh_path}")
        imported += 1
    return imported


TEXTURE_DIR = "/Game/ShortStack/Textures"


def import_textures(force=False):
    """Imports the textures made by art/blender (unreal/Art/Textures/<Group>/*.png): the shirt prints from prints.py
    (Clothing, linear masks) and the Embercrest's brand art from brand.py (Brand, color).

    Each lands in /Game/ShortStack/Textures/<Group>/<Name>, reimported only when the file changed (its hash kept as
    metadata). Returns how many were imported.
    """
    import os
    src_root = os.path.join(unreal.Paths.project_dir(), "Art", "Textures")
    if not os.path.isdir(src_root):
        return 0
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    imported = 0
    for group in sorted(os.listdir(src_root)):
        group_dir = os.path.join(src_root, group)
        if not os.path.isdir(group_dir):
            continue
        for file in sorted(os.listdir(group_dir)):
            if not file.lower().endswith(".png"):
                continue
            name = os.path.splitext(file)[0]
            path = os.path.join(group_dir, file)
            folder = f"{TEXTURE_DIR}/{group}"
            asset_path = f"{folder}/{name}"
            digest = _file_hash(path)
            if not force and eal.does_asset_exist(asset_path):
                existing = eal.load_asset(asset_path)
                if existing and eal.get_metadata_tag(existing, "SourceHash") == digest:
                    continue
            task = unreal.AssetImportTask()
            task.filename = path
            task.destination_path = folder
            task.destination_name = name
            task.automated = True
            task.replace_existing = True
            task.save = False
            tools.import_asset_tasks([task])
            tex = eal.load_asset(asset_path)
            if not isinstance(tex, unreal.Texture2D):
                unreal.log_error(f"ShortStack: importing {file} produced no texture")
                continue
            masks = group.lower() == "clothing"
            tex.set_editor_property("srgb", not masks)
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS if masks else unreal.TextureCompressionSettings.TC_DEFAULT)
            eal.set_metadata_tag(tex, "SourceHash", digest)
            eal.save_loaded_asset(tex)
            unreal.log(f"ShortStack: imported {group}/{file} as {asset_path}")
            imported += 1
    return imported


def rebuild_stages():
    """Rebuilds each stage (NightOneStage, BackRoomStage) in the open level.

    The editor loads the level before this script runs, so a stage already in it was built
    without the materials and props made since, and Play-In-Editor copies it as it is.
    """
    stage_classes = [c for c in (unreal.load_class(None, "/Script/ShortStack.NightOneStage"),
                                 unreal.load_class(None, "/Script/ShortStack.BackRoomStage")) if c]
    if not stage_classes:
        return
    # Rebuilding marks the level unsaved; save it again unless it already had unsaved edits.
    was_dirty = bool(unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages())
    rebuilt = False
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for actor in actors.get_all_level_actors():
        if actor.get_class() in stage_classes:
            actor.rebuild_set()
            rebuilt = True
            unreal.log(f"ShortStack: rebuilt {actor.get_actor_label()} with the new assets")
    if rebuilt and not was_dirty:
        unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()


def _skin_material(folder, mesh):
    """Skin needs subsurface scattering, which glTF can't carry: M_<mesh>Skin rebuilds material slot 0
    from the imported textures with the Subsurface Profile shading model."""
    tex = {}
    for asset_path in eal.list_assets(folder, recursive=True):
        asset = eal.load_asset(asset_path)
        if isinstance(asset, unreal.Texture2D):
            for key in ("BaseColor", "ORM", "Normal"):
                if asset.get_name().endswith(f"_0_{key}"):
                    tex[key] = asset
    if len(tex) < 3:
        unreal.log_warning(f"ShortStack: skin textures not found in {folder} (have {sorted(tex)})")
        return
    tex["ORM"].set_editor_property("srgb", False)
    tex["Normal"].set_editor_property("srgb", False)
    tex["Normal"].set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
    for t in tex.values():
        eal.save_loaded_asset(t)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    profile_path = f"{MAT_DIR}/SP_Skin"
    if eal.does_asset_exist(profile_path):
        profile = eal.load_asset(profile_path)
    else:
        profile = tools.create_asset("SP_Skin", MAT_DIR, unreal.SubsurfaceProfile, None)  # the defaults are skin
    mat = _new_material(f"M_{mesh.get_name()}Skin", True)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_SUBSURFACE_PROFILE)
    mat.set_editor_property("subsurface_profile", profile)
    samplers = {}
    for i, (key, kind) in enumerate((("BaseColor", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR),
                                     ("ORM", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR),
                                     ("Normal", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL))):
        e = _expr(mat, unreal.MaterialExpressionTextureSample, -700, i * 260)
        e.set_editor_property("texture", tex[key])
        e.set_editor_property("sampler_type", kind)
        samplers[key] = e
    mel.connect_material_property(samplers["BaseColor"], "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(samplers["ORM"], "G", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(samplers["ORM"], "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    mel.connect_material_property(samplers["Normal"], "RGB", unreal.MaterialProperty.MP_NORMAL)
    spec = _expr(mat, unreal.MaterialExpressionConstant, -300, 600)
    spec.set_editor_property("r", 0.35)  # skin reflects less than the default 0.5
    mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
    _finish(mat)
    materials = list(mesh.get_editor_property("materials"))
    if materials:
        slot = materials[0]
        slot.set_editor_property("material_interface", mat)
        materials[0] = slot
        mesh.set_editor_property("materials", materials)


def run(force=False):
    import backroom_setup
    import street_setup
    changed = build_materials(force)
    changed += backroom_setup.build_materials(force)
    try:
        changed += street_setup.build_materials(force)
    except Exception as exc:  # the street falls back to M_Surface
        unreal.log_error(f"ShortStack: street materials failed: {exc}")
    imported = 0
    try:
        imported = import_meshes(force)
        changed += imported
        changed += backroom_setup.fix_meshes()
        import_textures(force)
    except Exception as exc:  # the stage falls back to engine shapes
        unreal.log_error(f"ShortStack: importing meshes failed: {exc}")
    if changed:
        rebuild_stages()
    build_map(False)
    backroom_setup.build_map(False)
    if not street_setup.build_map(False) and imported:
        # New props (the street's, the hats and glasses) go into the saved Street level.
        try:
            street_setup.refresh_map()
        except Exception as exc:
            unreal.log_error(f"ShortStack: rebuilding the Street level failed: {exc}")
