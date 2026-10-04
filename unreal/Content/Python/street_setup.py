"""
SHORT STACK: the Street level (Fifth Street, the Lucky Penny #212).

Builds the street's materials, all procedural HLSL like M_Surface:
  M_Street        brick, wet asphalt with puddles (rain rings in them), sidewalk slabs, the store's floor tiles,
                  concrete, rolled-down shop gates
  M_StreetWindow  the facades' windows, one instanced mesh: each window's room, curtains or blinds, lit or dark
  M_StreetRain    rain streaks around the eye, lit by the nearest lamps, never inside the store
  M_StreetGlass   the store's glass: clean, rain beaded on it
  M_StreetSky     the apartment's night sky and skyline with a strength (the street sets it for the hour)
  M_StreetCity
and /Game/Maps/Street with the AStreetStage actor and the AStreetGameMode override. shortstack_setup.run() calls
this when the editor opens. A material built by an older version of this file is rebuilt (VERSION), and so is the
saved level when the stage has changed (MAP_VERSION); anything current is left alone.

    import street_setup; street_setup.build_materials(force=True); street_setup.build_map(force=True)
"""
import unreal

import shortstack_setup as ss

MAP_PATH = "/Game/Maps/Street"
# Bump when a material's graph changes: each built material carries it (metadata) and is rebuilt when it differs.
VERSION = "4"
# Bump when AStreetStage builds the set differently: the saved level (what Play on the map and a cooked build use,
# since neither runs the stage's construction again) is rebuilt once.
MAP_VERSION = "4"
VERSION_TAG = "StreetVersion"

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
else if (pat == 6)
{
    // A rolled-down shop gate: corrugated steel slats, rust bleeding from the bottom, a tag or two.
    float slat = frac(m.z / 0.075);
    float rib = 0.5 + 0.5 * cos(slat * 6.28318);
    col *= 0.68 + 0.32 * rib;
    float rust = saturate((S.Fbm(float2(uv.x * 2.0, m.z * 0.6) + 21.0) - 0.55) * 3.0) * saturate(1.2 - m.z / 3.0);
    col = lerp(col, float3(0.24, 0.11, 0.05), rust * 0.6);
    float tag = saturate((S.Fbm(uv * 0.8 + 50.0) - 0.68) * 8.0) * step(m.z, 2.2);
    float3 ink = lerp(float3(0.75, 0.12, 0.35), float3(0.15, 0.45, 0.85), S.Hash(floor(uv * 0.5) + 9.0));
    col = lerp(col, ink * 0.6, tag * 0.7);
    r = lerp(0.35, 0.7, rust) + 0.1 * (1.0 - rib) - 0.2 * wet * saturate(1.0 - m.z / 3.0);
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

# The street's normal (world space): rain rings spreading in the puddles, and a fine stir of drops on wet ground.
STREET_NORMAL = ss.NOISE + """
float3 n = normalize(N);
int pat = (int)(PatternIn + 0.5);
float wet = saturate(WetIn);
if (n.z < 0.7 || wet < 0.01 || (pat != 2 && pat != 3 && pat != 5))
{
    return n;
}
float2 uv = P.xy / 100.0;
// The same puddles as the color (M_Street's pattern code).
float puddle = pat == 2 ? saturate((S.Fbm(uv * 0.35 + 7.0) - (0.62 - 0.22 * wet)) * 9.0) : (pat == 3 ? saturate((S.Fbm(uv * 0.5 + 11.0) - (0.72 - 0.15 * wet)) * 7.0) : 0.0);
float2 g = float2(0.0, 0.0);
float2 rp = uv * 3.0;
float2 id = floor(rp);
for (int j = -1; j <= 1; j++)
{
    for (int i = -1; i <= 1; i++)
    {
        // A drop lands somewhere in each cell on its own clock; its ring spreads and fades.
        float2 c = id + float2(i, j);
        float2 h = S.H22(c);
        float t = frac(T * (0.7 + 0.6 * h.x) + h.y);
        float2 d = rp - (c + 0.15 + 0.7 * h);
        float dist = length(d);
        float x = (dist - t * 1.1) * 14.0;
        float wave = sin(x * 3.14159) * saturate(1.0 - abs(x) * 0.5) * (1.0 - t) * (1.0 - t);
        g += d / max(dist, 1e-3) * wave;
    }
}
float k = lerp(0.04, 0.35, puddle) * wet;
return normalize(n + float3(g * k, 0.0));
"""

# Rain around the eye. Each sheet is one layer at its own distance (Depth); the streaks are laid out by their angle
# from the eye, so turning doesn't drag them and walking doesn't strobe them; each column falls at its own speed.
# The drops catch the nearest lamps' light (brightest in a lamp's cone, and when it's behind them), and the lit
# fronts of the store and the laundromat; nothing falls inside the store (StoreMin..StoreMax).
RAIN = ss.NOISE + """
struct SSRain
{
    float3 Lamp(float3 P, float3 V, float3 Pos, float3 Col)
    {
        float3 L = Pos - P;
        float d = length(L);
        float3 Ld = L / max(d, 1.0);
        float cone = smoothstep(0.35, 0.85, Ld.z);
        float fall = 1.0 / (1.0 + d * d / (520.0 * 520.0));
        float fwd = 1.0 + 3.0 * pow(saturate(dot(V, Ld)), 6.0);
        return Col * cone * fall * fwd;
    }
    float3 Glow(float3 P, float3 V, float3 Pos, float3 Col)
    {
        float3 L = Pos - P;
        float d = length(L);
        float fall = 1.0 / (1.0 + d * d / (450.0 * 450.0));
        float fwd = 1.0 + 2.0 * pow(saturate(dot(V, L / max(d, 1.0))), 4.0);
        return Col * fall * fwd;
    }
};
SSRain Rain;
float3 d = P - Cam;
// A whole number of columns round the circle, each named by its place in it: where atan2 wraps (looking along -X,
// at the store's windows from the sidewalk) the columns carry on unbroken instead of a streak cut in two.
float ring = max(1.0, floor(6.2831853 * Depth / Spacing + 0.5));
float perRad = ring / 6.2831853;
float x = atan2(d.y, d.x) * perRad + d.z * Slant / Spacing;
float colId = floor(x);
colId -= ring * floor(colId / ring);
float rr = S.H11(colId * 3.7 + Depth * 0.0137);
float on = step(0.48, rr);
float speed = 650.0 + 350.0 * frac(rr * 7.3);
float period = LengthCm * (4.0 + 5.0 * frac(rr * 13.1));
float y = frac((P.z + T * speed) / period + rr * 7.0);
float s = y * period / LengthCm;
float streak = s < 1.0 ? pow(saturate(1.0 - abs(2.0 * s - 1.0)), 0.6) : 0.0;
// A streak a pixel or two wide wherever it is: thinner than a pixel it dims instead of widening. A pixel's width in
// columns comes from the view (PixelAngle: radians per pixel, set by the stage), not from derivatives, which the
// ray-traced passes this material is also compiled for don't have.
float u = frac(x) - 0.5 - (frac(rr * 31.0) - 0.5) * 0.6;
float fw = max(PixelAngle * perRad, 1e-4);
float hw = 0.5 * WidthCm / Spacing;
float cover = saturate((hw + 0.5 * fw - abs(u)) / fw) * min(1.0, 2.0 * hw / fw);
float3 V = normalize(d);
float3 lit = Ambient + Rain.Lamp(P, V, LampPos0, LampCol0) + Rain.Lamp(P, V, LampPos1, LampCol1) + Rain.Lamp(P, V, LampPos2, LampCol2) + Rain.Lamp(P, V, LampPos3, LampCol3)
    + Rain.Glow(P, V, GlowPos0, GlowCol0) + Rain.Glow(P, V, GlowPos1, GlowCol1);
// Nothing falls indoors: the store, and the laundromat's room seen through its glass across the street.
bool inStore = all(P > StoreMin) && all(P < StoreMax);
bool inWash = all(P > WashMin) && all(P < WashMax);
float outside = (inStore || inWash) ? 0.0 : 1.0;
// The sheet thins out toward its own edges: looking far up, or wide, the rain fades rather than stopping on a line.
float2 edgeUV = min(UV, 1.0 - UV);
float edge = saturate(min(edgeUV.x, edgeUV.y) * 12.0);
return (lit * Strength + FlashIn * 0.5) * streak * cover * on * outside * edge * (0.6 + 0.4 * frac(colId * 13.7));
"""

# One window on a facade (the engine cube, 5 cm deep; its faces along X are the glass), from its instance's data:
# Kind 0 dark, 1 a warm lamp, 2 a cool screen or tube, 3 a TV; Seed picks the room and what hangs in the window;
# Level how bright. Glow is the street's (lower by day).
WINDOW = """
float front = step(49.0, abs(LP.x));
float2 uv = float2(LP.y / 100.0 + 0.5, LP.z / 100.0 + 0.5);
float k = floor(Kind + 0.5);
if (k < 0.5 || front < 0.5)
{
    return float3(0.0, 0.0, 0.0);
}
float sd = Seed;
float3 warm = lerp(float3(1.0, 0.52, 0.22), float3(1.0, 0.72, 0.42), frac(sd * 3.7));
float3 cool = lerp(float3(0.55, 0.7, 1.0), float3(0.82, 0.9, 1.0), frac(sd * 5.3));
float3 tint = k < 1.5 ? warm : (k < 2.5 ? cool : float3(0.35, 0.55, 1.0));
// The room: brightest near its lamp, darker to the corners and the sill; a sofa back or a shelf low across it.
float2 lamp = float2(0.3 + 0.4 * frac(sd * 11.0), 0.85);
float room = 0.45 + 0.55 * saturate(1.0 - length((uv - lamp) * float2(1.3, 1.6)));
room *= 0.75 + 0.25 * saturate(uv.y * 2.0);
float furniture = step(uv.y, 0.18 + 0.1 * step(0.5, frac(sd * 17.0))) * step(abs(uv.x - frac(sd * 23.0)), 0.25);
room *= 1.0 - 0.6 * furniture;
float treat = frac(sd * 7.13);
float cover = 1.0;
if (treat < 0.35)
{
    // Curtains drawn back to the sides, folds catching the light; the light through them takes their color.
    float open = 0.22 + 0.3 * frac(sd * 13.0);
    float drape = smoothstep(open, open + 0.05, abs(uv.x - 0.5) * 2.0);
    float folds = 0.7 + 0.3 * sin(uv.x * 70.0 + sd * 9.0);
    float3 cloth = lerp(float3(0.95, 0.4, 0.3), float3(0.4, 0.55, 0.95), frac(sd * 29.0));
    tint = lerp(tint, tint * cloth, drape);
    cover = lerp(1.0, 0.4 * folds, drape);
}
else if (treat < 0.6)
{
    // Blinds: light between the slats, the top part pulled up.
    float raised = frac(sd * 19.0) * 0.55;
    float slat = frac(uv.y * 24.0);
    float gap = smoothstep(0.55, 0.75, slat) * (1.0 - smoothstep(0.85, 0.98, slat));
    cover = uv.y > 1.0 - raised ? 1.0 : 0.15 + 0.5 * gap;
}
else if (treat < 0.75)
{
    // A sheer: the whole room soft behind it.
    cover = 0.55;
    room = lerp(room, 0.85, 0.6);
}
// The sash: its frame, the meeting rail, a center bar on some.
float2 e = min(uv, 1.0 - uv);
float sash = step(0.045, e.x) * step(0.035, e.y) * (1.0 - step(abs(uv.y - 0.52), 0.012));
sash *= 1.0 - step(abs(uv.x - 0.5), 0.008) * step(0.5, frac(sd * 31.0));
float tv = k > 2.5 ? (0.55 + 0.45 * sin(T * 6.3 + sd * 40.0) * sin(T * 1.7 + sd * 9.0)) : 1.0;
return tint * room * cover * tv * sash * Level * GlowIn;
"""

# The store's glass: clean, the rain beaded on it, a little smudged low where hands push it, more reflective at a
# glancing look. The beads are laid out in the pane's own plane from its pivot (ObjPos), so they ride with a sliding
# door, whichever way it faces (the windows along X, the counter's acrylic along Y or up: laid out in the wrong plane
# they'd smear into stripes). Wet 0 is a dry pane indoors. Returns opacity and roughness.
GLASS = ss.DROPS + """
float3 an = abs(N);
float3 o = P - ObjPos;
float2 uv = (an.z > max(an.x, an.y) ? o.xy : (an.x >= an.y ? o.yz : o.xz)) / 100.0;
float wet = saturate(WetIn);
float3 b1 = D.Beads(uv, T, 14.0) * wet;
float3 b2 = D.Beads(uv + 3.7, T * 0.8, 30.0) * (0.6 * wet);
float water = saturate(b1.z + b2.z);
float smudge = saturate((S.Fbm(uv * 4.0 + 5.0) - 0.55) * 3.0) * saturate(1.3 - P.z / 120.0);
float3 v = normalize(Cam - P);
float fres = pow(1.0 - saturate(abs(dot(v, normalize(N)))), 5.0);
float opacity = saturate(0.06 + 0.22 * water + 0.1 * smudge + 0.5 * fres);
float rough = lerp(0.03, 0.18, max(water * 0.6, smudge));
return float2(opacity, rough);
"""

GLASS_NORMAL = ss.DROPS + """
float3 an = abs(N);
float3 o = P - ObjPos;
bool up = an.z > max(an.x, an.y);
bool facesX = an.x >= an.y;
float2 uv = (up ? o.xy : (facesX ? o.yz : o.xz)) / 100.0;
float3 b1 = D.Beads(uv, T, 14.0);
float3 b2 = D.Beads(uv + 3.7, T * 0.8, 30.0) * 0.6;
// The beads bend what the pane reflects (their offsets tilt it across and up the pane, in its own plane).
float2 b = (b1.xy + b2.xy) * (0.35 * saturate(WetIn));
float3 tilt = up ? float3(b.x, b.y, 0.0) : (facesX ? float3(0.0, b.x, b.y) : float3(b.x, 0.0, b.y));
return normalize(normalize(N) + tilt);
"""


def _set(obj, prop, value):
    """Sets an editor property, logging (not failing) when this engine names it differently."""
    try:
        obj.set_editor_property(prop, value)
        return True
    except Exception as exc:
        unreal.log_warning(f"ShortStack: couldn't set {prop}: {exc}")
        return False


def _material(name, force):
    """The material to build: new, forced, or built by an older version of this file (rebuilt in place, so the
    instances the saved Street level holds stay valid). None when it's current."""
    path = f"{ss.MAT_DIR}/{name}"
    if ss.eal.does_asset_exist(path) and not force:
        existing = ss.eal.load_asset(path)
        if existing is not None and ss.eal.get_metadata_tag(existing, VERSION_TAG) == VERSION:
            return None
    return ss._new_material(name, True)


def _finish(mat):
    """Marks a material built by this version (only once its graph is complete: one that failed halfway is built again
    next time), compiles and saves it."""
    ss.eal.set_metadata_tag(mat, VERSION_TAG, VERSION)
    return ss._finish(mat)


def build_street(force):
    mat = _material("M_Street", force)
    if not mat:
        return 0
    mat.set_editor_property("used_with_instanced_static_meshes", True)
    # The normal below is in world space (the ripples are laid on the ground, whatever the box's UVs).
    _set(mat, "tangent_space_normal", False)
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
    nrm = ss._custom(mat, STREET_NORMAL, ["P", "N", "PatternIn", "WetIn", "T"], ss.F3, -500, 500, "Rain on the wet street")
    ss._link(wp, nrm, "P")
    ss._link(n, nrm, "N")
    ss._link(pattern, nrm, "PatternIn")
    ss._link(wet, nrm, "WetIn")
    ss._link(ss._time(mat, -900, 600), nrm, "T")
    unreal.MaterialEditingLibrary.connect_material_property(nrm, "", unreal.MaterialProperty.MP_NORMAL)
    _finish(mat)
    return 1


def build_rain(force):
    mat = _material("M_StreetRain", force)
    if not mat:
        return 0
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    mat.set_editor_property("two_sided", True)
    inputs = ["P", "Cam", "T", "Depth", "Spacing", "WidthCm", "LengthCm", "Slant", "Strength", "PixelAngle", "Ambient",
              "LampPos0", "LampCol0", "LampPos1", "LampCol1", "LampPos2", "LampCol2", "LampPos3", "LampCol3",
              "GlowPos0", "GlowCol0", "GlowPos1", "GlowCol1", "StoreMin", "StoreMax", "WashMin", "WashMax", "FlashIn", "UV"]
    c = ss._custom(mat, RAIN, inputs, ss.F3, -450, 0, "Rain around the eye")
    ss._link(ss._world_pos(mat, -900, -600), c, "P")
    # Where on the sheet (the engine plane's 0..1): for the fade at its edges.
    ss._link(ss._expr(mat, unreal.MaterialExpressionTextureCoordinate, -900, -700), c, "UV")
    ss._link(ss._expr(mat, unreal.MaterialExpressionCameraPositionWS, -900, -500), c, "Cam")
    ss._link(ss._time(mat, -900, -400), c, "T")
    ss._link(ss._scalar(mat, "Flash", 0.0, -900, -300), c, "FlashIn")
    y = -200
    for name, value in (("Depth", 500.0), ("Spacing", 10.0), ("WidthCm", 0.4), ("LengthCm", 45.0), ("Slant", 0.06), ("Strength", 1.0), ("PixelAngle", 0.0006)):
        ss._link(ss._scalar(mat, name, value, -900, y), c, name)
        y += 80
    ss._link(ss._vector(mat, "Ambient", (0.05, 0.055, 0.07), -900, y), c, "Ambient")
    y += 80
    for k in range(4):
        ss._link(ss._vector(mat, f"LampPos{k}", (0.0, 0.0, -100000.0), -900, y), c, f"LampPos{k}")
        ss._link(ss._vector(mat, f"LampCol{k}", (0.0, 0.0, 0.0), -900, y + 80), c, f"LampCol{k}")
        y += 160
    for k in range(2):
        ss._link(ss._vector(mat, f"GlowPos{k}", (0.0, 0.0, -100000.0), -900, y), c, f"GlowPos{k}")
        ss._link(ss._vector(mat, f"GlowCol{k}", (0.0, 0.0, 0.0), -900, y + 80), c, f"GlowCol{k}")
        y += 160
    # An empty box masks nothing until the stage says where the store (and the laundromat's room) is.
    ss._link(ss._vector(mat, "StoreMin", (0.0, 0.0, 0.0), -900, y), c, "StoreMin")
    ss._link(ss._vector(mat, "StoreMax", (0.0, 0.0, 0.0), -900, y + 80), c, "StoreMax")
    ss._link(ss._vector(mat, "WashMin", (0.0, 0.0, 0.0), -900, y + 160), c, "WashMin")
    ss._link(ss._vector(mat, "WashMax", (0.0, 0.0, 0.0), -900, y + 240), c, "WashMax")
    unreal.MaterialEditingLibrary.connect_material_property(c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    _finish(mat)
    return 1


def build_window(force):
    mat = _material("M_StreetWindow", force)
    if not mat:
        return 0
    mat.set_editor_property("used_with_instanced_static_meshes", True)
    c = ss._custom(mat, WINDOW, ["LP", "Kind", "Seed", "Level", "T", "GlowIn"], ss.F3, -450, 0, "A lit window")
    # The instance's own space (the cube's +-50), read per pixel: Local Position (Instance) works in the pixel shader,
    # where the emissive is; Pre-Skinned Position is vertex-shader only and would fail to compile there.
    ss._link(ss._expr(mat, unreal.MaterialExpressionLocalPosition, -900, -300), c, "LP")
    for index, name in enumerate(("Kind", "Seed", "Level")):
        data = ss._expr(mat, unreal.MaterialExpressionPerInstanceCustomData, -900, -200 + index * 90)
        data.set_editor_property("data_index", index)
        data.set_editor_property("const_default_value", 0.0)
        ss._link(data, c, name)
    ss._link(ss._time(mat, -900, 100), c, "T")
    ss._link(ss._scalar(mat, "Glow", 1.0, -900, 200), c, "GlowIn")
    unreal.MaterialEditingLibrary.connect_material_property(c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    # The glass itself: dark and glossy, so the street's lamps show in it.
    unreal.MaterialEditingLibrary.connect_material_property(ss._vector(mat, "Glass", (0.012, 0.014, 0.018), -900, 320), "", unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.connect_material_property(ss._scalar(mat, "Roughness", 0.1, -900, 420), "", unreal.MaterialProperty.MP_ROUGHNESS)
    _finish(mat)
    return 1


def build_glass(force):
    mat = _material("M_StreetGlass", force)
    if not mat:
        return 0
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    # Lit per pixel, so the street's lamps glint in it.
    per_pixel = getattr(getattr(unreal, "TranslucencyLightingMode", None), "TLM_SURFACE_PER_PIXEL_LIGHTING", None)
    if per_pixel is not None:
        _set(mat, "translucency_lighting_mode", per_pixel)
    _set(mat, "tangent_space_normal", False)
    wp = ss._world_pos(mat, -900, -300)
    n = ss._expr(mat, unreal.MaterialExpressionVertexNormalWS, -900, -200)
    cam = ss._expr(mat, unreal.MaterialExpressionCameraPositionWS, -900, -100)
    pivot = ss._expr(mat, unreal.MaterialExpressionObjectPositionWS, -900, 50)
    t = ss._time(mat, -900, 0)
    # 1 out on the street, 0 for the counter's acrylic indoors (AStreetStage sets it).
    wet = ss._scalar(mat, "Wet", 1.0, -900, 250)
    c = ss._custom(mat, GLASS, ["P", "N", "Cam", "ObjPos", "T", "WetIn"], ss.F2, -450, -100, "Rain-beaded shop glass")
    for src, name in ((wp, "P"), (n, "N"), (cam, "Cam"), (pivot, "ObjPos"), (t, "T"), (wet, "WetIn")):
        ss._link(src, c, name)
    nrm = ss._custom(mat, GLASS_NORMAL, ["P", "N", "ObjPos", "T", "WetIn"], ss.F3, -450, 250, "The beads' lenses")
    for src, name in ((wp, "P"), (n, "N"), (pivot, "ObjPos"), (t, "T"), (wet, "WetIn")):
        ss._link(src, nrm, name)
    unreal.MaterialEditingLibrary.connect_material_property(ss._mask(mat, c, "r", -200, -150), "", unreal.MaterialProperty.MP_OPACITY)
    unreal.MaterialEditingLibrary.connect_material_property(ss._mask(mat, c, "g", -200, -50), "", unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.connect_material_property(nrm, "", unreal.MaterialProperty.MP_NORMAL)
    unreal.MaterialEditingLibrary.connect_material_property(ss._vector(mat, "Tint", (0.02, 0.025, 0.03), -900, 150), "", unreal.MaterialProperty.MP_BASE_COLOR)
    _finish(mat)
    return 1


def _strengthened(name, code, inputs, description, instanced, force):
    """The apartment's sky or skyline, times a Strength the street sets for the hour (by day the exposure is far darker)."""
    mat = _material(name, force)
    if not mat:
        return 0
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    if instanced:
        mat.set_editor_property("used_with_instanced_static_meshes", True)
    else:
        mat.set_editor_property("two_sided", True)
    c = ss._custom(mat, code, inputs, ss.F3, -450, 0, description)
    ss._link(ss._world_pos(mat, -900, -300), c, "P")
    ss._link(ss._expr(mat, unreal.MaterialExpressionCameraPositionWS, -900, -100), c, "Cam")
    if "N" in inputs:
        ss._link(ss._expr(mat, unreal.MaterialExpressionVertexNormalWS, -900, -200), c, "N")
    if "Rand" in inputs:
        ss._link(ss._expr(mat, unreal.MaterialExpressionPerInstanceRandom, -900, 0), c, "Rand")
    ss._timed_inputs(mat, c, inputs)
    glow = ss._expr(mat, unreal.MaterialExpressionMultiply, -200, 0)
    ss._link(c, glow, "A")
    ss._link(ss._scalar(mat, "Strength", 1.0, -450, 250), glow, "B")
    unreal.MaterialEditingLibrary.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    _finish(mat)
    return 1


def build_sky(force):
    return _strengthened("M_StreetSky", ss.SKY, ["P", "Cam", "T", "DawnIn", "FlashIn"], "Night sky over the street", False, force)


def build_city(force):
    return _strengthened("M_StreetCity", ss.CITY, ["P", "N", "Rand", "Cam", "T", "DawnIn", "FlashIn"], "The skyline past the street", True, force)


def build_materials(force=False):
    """Builds the street's materials that are missing or out of date (all of them with force). Returns how many."""
    built = 0
    for build in (build_street, build_rain, build_window, build_glass, build_sky, build_city):
        try:
            built += build(force)
        except Exception as exc:  # the stage falls back to the apartment's materials for each
            unreal.log_error(f"ShortStack: {build.__name__} failed: {exc}")
    return built


def _stamp_open_level():
    """Marks the open (Street) level as built by this version of the stage, and sets its floor: a pawn that ever
    gets below the street is removed at 20 m down (the game mode puts the player back at the door long before)."""
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    if not world:
        return
    _set(world.get_world_settings(), "kill_z", -2000.0)
    try:
        ss.eal.set_metadata_tag(world, VERSION_TAG, MAP_VERSION)
    except Exception as exc:
        unreal.log_warning(f"ShortStack: couldn't mark the Street level's version: {exc}")


def _map_current():
    world = unreal.load_asset(MAP_PATH)
    return world is not None and ss.eal.get_metadata_tag(world, VERSION_TAG) == MAP_VERSION


def build_map(force=False):
    """/Game/Maps/Street: the stage actor and the game mode override (the stage builds everything else). A level
    saved by an older stage is rebuilt (refresh_map). True when it was created or rebuilt."""
    if ss.eal.does_asset_exist(MAP_PATH) and not force:
        try:
            if _map_current():
                return False
            unreal.log("ShortStack: the Street level was built by an older stage; rebuilding it")
            return refresh_map()
        except Exception as exc:
            unreal.log_error(f"ShortStack: checking the Street level failed: {exc}")
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
    _stamp_open_level()
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
    _stamp_open_level()
    levels.save_current_level()
    if previous and previous != MAP_PATH and ss.eal.does_asset_exist(previous):
        levels.load_level(previous)
    unreal.log("ShortStack: rebuilt the Street level")
    return True
