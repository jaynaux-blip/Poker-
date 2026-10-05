"""
SHORT STACK: the Embercrest's exterior (/Game/Maps/Embercrest; design spec ec_exterior_spec.md).

Builds what AEmbercrestStage uses that the Blender pipeline doesn't make, all procedural HLSL like M_Surface, each
taking the exterior's texture sets (unreal/Art/Textures/ECX, imported by shortstack_setup.import_textures) where they
are:
  M_ECX_Ground    the plaza's surfaces by kind (pavers, setts, asphalt, curbs, concrete, wall stone, paint), always wet
                  on the heated stone, puddles and slush from the ground's vertex color (R puddle, G slush, B heated)
  M_ECX_Snow      fresh snow, blue in its shade (subsurface), wind ripples, glints stable in the world
  M_ECX_Terrain   the snow terrain round the site with its rock (vertex color R), forest litter (G), wind crust (B)
  M_ECX_Glass     the entry's and the shelter's glass (Lumen's front-layer reflections), condensation at the sill;
  M_ECX_GlassTint the bus's tinted side windows
  M_ECX_Water     the cascades' black, rippling pools
  M_ECX_Falls     the sheets falling over the weirs
  M_ECX_Flame     the fire bowls' gas flames (flickered by the stage)
  M_ECX_Mist      the valley's mist sheets and the steam off the heated pools
  M_ECX_Conifer   the spruces' and pines' needle cards (masked, two-sided foliage), M_ECX_Bark their bark
  M_ECX_Sky       the stars, the Milky Way, the moon's face and the city's glow low in the west (additive, at night)
  M_ECX_Screen    the LED screens (the marquee, the monument's strip, the bus's signs): images and a render target's text
  M_ECX_Sign      lit lettering drawn into a render target (the stand-ins' EMBERCREST, EMBER GRILL)
and /Game/Maps/Embercrest with the AEmbercrestStage actor and the AEmbercrestGameMode override (the stage builds the
world itself when play begins). It also fixes up the SM_ECX_ imports (fix_mesh: Nanite off for glass, screens, water,
flames and mist; complex collision on the ground and the terrain; the importer's hulls off the rest; no distance fields
on the kilometer-scale meshes) and the ECX textures' formats (normal maps, masks).

shortstack_setup.run() calls run() when the editor opens; a material built by an older version of this file, or before
a texture set it uses was imported, is rebuilt; anything current is left alone.

    import embercrest_setup; embercrest_setup.run(force=True)
"""
import unreal

import shortstack_setup as ss

MAP_PATH = "/Game/Maps/Embercrest"
TEX_DIR = "/Game/ShortStack/Textures/ECX"
# Bump when a material's graph changes (each carries it, with the texture sets it was built with, as metadata).
VERSION = "5"
VERSION_TAG = "ECXVersion"
# Bump when the level itself must change (the stage builds the world at runtime, so new art needs no rebuild).
MAP_VERSION = "1"
MAP_TAG = "ECXMapVersion"
# Bump when fix_mesh's rules change (each mesh carries the version and its source's hash once fixed).
FIX_VERSION = "1"
FIX_TAG = "ECXFix"

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

COLOR = unreal.MaterialSamplerType.SAMPLERTYPE_COLOR
NORMAL = unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL
MASKS = unreal.MaterialSamplerType.SAMPLERTYPE_MASKS
DEFAULTS = {
    COLOR: "/Engine/EngineMaterials/DefaultDiffuse.DefaultDiffuse",
    NORMAL: "/Engine/EngineMaterials/DefaultNormal.DefaultNormal",
    MASKS: "/Engine/EngineMaterials/DefaultDiffuse_TC_Masks.DefaultDiffuse_TC_Masks",
}
BLACK = "/Engine/EngineResources/Black.Black"

# ---------------------------------------------------------------- HLSL

SCREEN_UV = """
// The image's UVs: the mesh's, or on a stand-in box its local position (each face reads from its own side).
float2 uv = UV;
if (LocalUV > 0.5)
{
    float s = LP.x >= 0.0 ? 1.0 : -1.0;
    uv = float2(0.5 - s * LP.y / 100.0, 0.5 - LP.z / 100.0);
}
// The text's: scrolled along a ticker (one period of the line, wrapping).
float2 tuv = uv;
if (Ticker > 0.5)
{
    tuv.x = frac(uv.x * ScrollScale + Scroll);
}
return float4(uv, tuv);
"""

SCREEN = """
// Each page: its image (or black) under its text (premultiplied, as Slate draws into the target).
float3 a = IA * HasImageA;
float3 b = IB * HasImageB;
a = a * (1.0 - TAa) + TA;
b = b * (1.0 - TBa) + TB;
float3 col = lerp(a, b, saturate(Mix));
// The LED cells, seen up close.
if (Cells > 0.5)
{
    float2 cell = frac(UV0 * float2(Cells, Cells * CellAspect));
    float2 d = abs(cell - 0.5);
    col *= 1.0 - Grid * smoothstep(0.32, 0.5, max(d.x, d.y));
}
return col * Tint * Strength;
"""

GLASS = ss.NOISE + """
// Clear and dark, more reflective at a glancing look; a breath of condensation along the sill where the warm room meets
// the cold (the bottom 15 cm over SillZ).
float3 v = normalize(Cam - P);
float fres = pow(1.0 - saturate(abs(dot(v, normalize(N)))), 5.0);
float h = P.z - SillZ;
float band = saturate(1.0 - h / 15.0) * step(-2.0, h);
float mottle = S.Fbm(P.xy / 7.0 + P.z / 9.0);
float fog = band * saturate(0.4 + 0.8 * mottle);
float opacity = saturate(BaseOpacity + 0.55 * fres + 0.35 * fog);
float rough = lerp(0.02, 0.35, fog);
return float2(opacity, rough);
"""

GROUND_UV = """
float2 uv = WorldUV > 0.5 ? P.xy / 100.0 : UV0;
return uv / max(Tile, 0.01);
"""

GROUND = ss.NOISE + """
// The plaza's surfaces by kind (0 pavers, 1 setts, 2 asphalt, 3 curb, 4 concrete, 5 wall stone, 7 paint); the tiling
// texture set where it's imported, else the kind's own pattern. Returns base color and roughness.
float3 m = P / 100.0;
float2 uv = WorldUV > 0.5 ? m.xy : UV0;
int k = (int)(Kind + 0.5);
float3 col = Tint;
float r = 0.7;
if (UseTex > 0.5 && k != 7)
{
    col = TexBase;
    r = TexOrm.g;
}
else if (k == 0)
{
    // Flamed granite pavers 60 x 30 cm in a running bond, 5 mm joints, each its own tone; a basalt band every 3.6 m.
    float2 g = float2(uv.x / 0.6, uv.y / 0.3);
    float row = floor(g.y);
    g.x += fmod(abs(row), 2.0) * 0.5;
    float2 f = frac(g);
    float joint = (f.x < 0.008 || f.y < 0.017) ? 1.0 : 0.0;
    float tone = 0.94 + 0.12 * S.Hash(floor(g));
    float speck = S.Value(uv * 260.0);
    col = Tint * tone * (0.9 + 0.2 * speck);
    float band = step(frac(uv.x / 3.6), 0.05);
    col = lerp(col, float3(0.03, 0.032, 0.036), band);
    col = lerp(col, col * 0.35, joint);
    r = lerp(0.72, 0.9, joint);
}
else if (k == 1)
{
    // Basalt setts 10 x 20 cm, rounded tops, dark joints.
    float2 g = float2(uv.x / 0.2, uv.y / 0.1);
    float row = floor(g.y);
    g.x += fmod(abs(row), 2.0) * 0.5;
    float2 f = frac(g);
    float2 e = min(f, 1.0 - f) * float2(20.0, 10.0);
    float joint = 1.0 - smoothstep(0.2, 0.9, min(e.x, e.y));
    col = Tint * (0.85 + 0.3 * S.Hash(floor(g)));
    col = lerp(col, float3(0.012, 0.012, 0.013), joint);
    r = lerp(0.6, 0.85, joint);
}
else if (k == 2)
{
    // Asphalt: dark aggregate, sealed cracks, a faint bloom of salt.
    float agg = S.Value(uv * 140.0);
    float seam = saturate(1.0 - abs(S.Fbm(uv * 0.9) - 0.5) * 70.0);
    col = Tint * (0.8 + 0.35 * agg);
    col = lerp(col, float3(0.015, 0.015, 0.016), seam * 0.7);
    float salt = saturate((S.Fbm(uv * 0.5 + 4.0) - 0.6) * 3.0);
    col = lerp(col, float3(0.32, 0.31, 0.3), salt * 0.25);
    r = 0.75 + 0.15 * agg;
}
else if (k == 3)
{
    col = Tint * (0.9 + 0.15 * S.Value(uv * 200.0));
    r = 0.75;
}
else if (k == 4)
{
    col = Tint * (0.85 + 0.25 * S.Fbm(uv * 3.0) + 0.05 * S.Value(uv * 120.0));
    r = 0.8;
}
else if (k == 5)
{
    // Basalt ashlar, coursed, recessed joints (seen from the side).
    float3 an = abs(N);
    float2 w = an.x > an.y ? m.yz : m.xz;
    float2 g = float2(w.x / 0.6, w.y / 0.3);
    g.x += fmod(abs(floor(g.y)), 2.0) * 0.5;
    float2 f = frac(g);
    float joint = (f.x < 0.02 || f.y < 0.04) ? 1.0 : 0.0;
    col = Tint * (0.85 + 0.3 * S.Hash(floor(g)));
    col = lerp(col, col * 0.3, joint);
    r = 0.6;
}
else if (k == 7)
{
    col = UseVertexColor > 0.5 ? VC.rgb : Tint;
    r = 0.45;
}
// The heated stone is always wet; puddles where the ground holds them; grey slush toward the banks.
float heated = UseVertexColor > 0.5 ? VC.b : 1.0;
float prone = UseVertexColor > 0.5 ? VC.r : saturate((S.Fbm(m.xy * 0.45 + 3.0) - 0.58) * 6.0);
float slush = UseVertexColor > 0.5 ? VC.g : 0.0;
if (k == 7)
{
    heated = 1.0;
    prone = 0.0;
    slush = 0.0;
}
float level = saturate((N.z - 0.85) * 8.0);
float wet = saturate(max(heated, slush) * Wet) * (k == 5 ? 0.4 : 1.0);
float puddle = saturate(prone * level * saturate((S.Fbm(m.xy * 0.8 + 11.0) - 0.35) * 3.0));
col = lerp(col, float3(0.36, 0.33, 0.3), slush * 0.55);
r = lerp(r, 0.6, slush * 0.5);
col *= lerp(1.0, 0.55, wet);
r = lerp(r, r * 0.4, wet);
col = lerp(col, col * 0.5, puddle);
r = lerp(r, 0.03, puddle);
return float4(col, saturate(r));
"""

GROUND_NORMAL = ss.NOISE + """
// World space: the tile's normal laid on the ground, mirror-flat in the puddles; walls keep their own. The maps are OpenGL
// (green toward the image's top). Laid by world position (v grows with +Y) the image's top is -Y; on the imported ground's
// UV0 (map meters, v flipped by glTF on the way in) it is +Y.
float3 n = normalize(N);
int k = (int)(Kind + 0.5);
if (UseTex > 0.5 && k != 7 && n.z > 0.6)
{
    float3 t = TexNormal;
    float up = WorldUV > 0.5 ? -1.0 : 1.0;
    n = normalize(float3(t.x * 0.8, up * t.y * 0.8, max(t.z, 0.2)));
}
float3 m = P / 100.0;
float prone = UseVertexColor > 0.5 ? VC.r : saturate((S.Fbm(m.xy * 0.45 + 3.0) - 0.58) * 6.0);
float puddle = prone * saturate((S.Fbm(m.xy * 0.8 + 11.0) - 0.35) * 3.0) * saturate((N.z - 0.85) * 8.0);
return normalize(lerp(n, float3(0.0, 0.0, 1.0), saturate(puddle)));
"""

PROJECT_UV = """
// The dominant axis's plane, Tile cm to a repeat (snow and rock on anything, whatever its own UVs).
float3 an = abs(N);
float2 uv = an.z > max(an.x, an.y) ? P.xy : (an.x > an.y ? P.yz : P.xz);
return uv / max(Tile, 1.0);
"""

SNOW = ss.NOISE + """
// Fresh wind-crusted snow: macro variation every 20-40 m, fine grains, crust a touch smoother; the glints' cells polished.
// Returns base color and roughness.
float3 m = P / 100.0;
float macro = S.Fbm(m.xy / 28.0);
float grain = S.Value(m.xy * 9.0);
float3 col = float3(0.84, 0.87, 0.92) * (0.95 + 0.08 * macro) * (0.97 + 0.04 * grain);
float r = 0.62 - 0.1 * saturate((S.Fbm(m.xy / 6.0 + 5.0) - 0.55) * 4.0);
if (UseTex > 0.5)
{
    col = TexBase * (0.95 + 0.08 * macro);
    r = TexOrm.g;
}
float3 id = floor(P / 0.9);
float h = S.Hash(id.xy + id.z * 17.31);
float close = 1.0 - saturate((length(Cam - P) - 900.0) / 600.0);
r = lerp(r, 0.05, step(0.985, h) * close);
return float4(col, r);
"""

SNOW_NORMAL = ss.NOISE + """
// World space: ripples in the crust, the texture's grain where it's imported, and the glints: a few cells in every
// hundred tilted like ice facets, fixed in the world (the same every frame), gone past 15 m.
float3 n = normalize(N);
float3 m = P / 100.0;
float2 g = float2(S.Fbm(m.xy * 3.0) - 0.5, S.Fbm(m.xy * 3.0 + 7.0) - 0.5) * 0.25;
g += float2(S.Fbm(float2(m.x * 1.6 + m.y * 0.4, m.y * 0.5) * 2.0) - 0.5, 0.0) * 0.15;
if (UseTex > 0.5 && n.z > 0.5)
{
    g += float2(TexNormal.x, -TexNormal.y) * 0.5;
}
n = normalize(n + float3(g, 0.0) * saturate(n.z));
float3 id = floor(P / 0.9);
float h = S.Hash(id.xy + id.z * 17.31);
float close = 1.0 - saturate((length(Cam - P) - 900.0) / 600.0);
if (h > 0.985 && close > 0.0)
{
    float2 tilt = S.H22(id.xy + 3.1) - 0.5;
    n = normalize(n + float3(tilt * 1.4, 0.0) * close);
}
return n;
"""

TERRAIN = ss.NOISE + """
// The snow terrain with its rock (vertex color R: slope and noise), forest litter (G) and wind crust (B). Returns base
// color and roughness; SNOW_AMOUNT (the subsurface) is Terrain's second node.
float3 m = P / 100.0;
float rock = saturate(VC.r);
float litter = saturate(VC.g) * (1.0 - rock);
float crust = saturate(VC.b) * (1.0 - rock);
float macro = S.Fbm(m.xy / 28.0);
float3 snow = float3(0.84, 0.87, 0.92) * (0.95 + 0.08 * macro);
float sr = 0.62;
if (UseSnowTex > 0.5)
{
    snow = SnowBase * (0.95 + 0.08 * macro);
    sr = SnowOrm.g;
}
float3 stone = float3(0.31, 0.3, 0.29) * (0.7 + 0.5 * S.Fbm(m.xy * 0.7 + m.z * 0.3));
stone = lerp(stone, float3(0.35, 0.38, 0.22), saturate((S.Fbm(m.xy * 2.0 + 9.0) - 0.62) * 4.0) * 0.5);
float rr = 0.8;
if (UseRockTex > 0.5)
{
    stone = RockBase;
    rr = RockOrm.g;
}
// Frost in the rock's cracks.
stone = lerp(stone, float3(0.75, 0.8, 0.86), saturate((S.Fbm(m.xy * 5.0) - 0.7) * 3.0) * 0.4);
float3 col = lerp(snow, stone, rock);
col = lerp(col, float3(0.1, 0.11, 0.08), litter * 0.55);
float r = lerp(sr, rr, rock);
r = lerp(r, 0.45, crust);
return float4(col, r);
"""

TERRAIN_NORMAL = ss.NOISE + """
float3 n = normalize(N);
float3 m = P / 100.0;
float rock = saturate(VC.r);
float2 g = float2(S.Fbm(m.xy * 3.0) - 0.5, S.Fbm(m.xy * 3.0 + 7.0) - 0.5) * lerp(0.2, 0.6, rock);
if (UseRockTex > 0.5)
{
    g = lerp(g, float2(RockNormal.x, -RockNormal.y), rock);
}
return normalize(n + float3(g, 0.0) * saturate(n.z + 0.3));
"""

WATER_NORMAL = ss.NOISE + """
// Ripples drifting toward the weirs (+X), finer ones over them.
float2 uv = P.xy / 100.0;
float2 flow = float2(T * 0.18, 0.0);
float2 g = float2(S.Fbm(uv * 1.3 - flow) - 0.5, S.Fbm(uv * 1.3 - flow + 9.0) - 0.5) * 0.12;
g += float2(S.Fbm(uv * 4.0 - flow * 2.5) - 0.5, S.Fbm(uv * 4.0 - flow * 2.5 + 3.0) - 0.5) * 0.05;
return normalize(float3(g, 1.0));
"""

FALLS = ss.NOISE + """
// A sheet falling over a weir: streaks running down it (the mesh's v along the flow, or a stand-in's height).
float s = 1.0 - UV.y;
float u = UV.x;
if (LocalUV > 0.5)
{
    s = 0.5 - LP.z / 100.0;
    u = 0.5 + LP.y / 100.0;
}
float streak = S.Fbm(float2(u * 60.0, s * 3.0 - T * 2.2));
float fine = S.Value(float2(u * 220.0, s * 8.0 - T * 4.0));
float sheet = 0.35 + 0.45 * streak + 0.2 * fine;
float lip = smoothstep(0.0, 0.08, s);
return saturate(sheet * lip * 0.75);
"""

FLAME = ss.NOISE + """
// A gas flame on a quad (v up): blue at its root, an orange body, a pale core, tongues breaking off at the top; it
// flickers with the bowl's light (Flicker) and is pale by day.
float2 uv = float2(UV.x, 1.0 - UV.y);
if (LocalUV > 0.5)
{
    uv = float2(0.5 + LP.y / 100.0, 0.5 + LP.z / 100.0);
}
float v = saturate(uv.y);
float t = T * (1.0 + 0.3 * Seed) + Seed * 31.0;
float n1 = S.Fbm(float2(uv.x * 3.0 + Seed * 7.0, v * 2.2 - t * 1.6));
float n2 = S.Fbm(float2(uv.x * 7.0 - Seed * 3.0, v * 5.0 - t * 2.9));
float x = uv.x - 0.5 + (n1 - 0.5) * 0.25 * v + (n2 - 0.5) * 0.08;
float width = 0.36 * pow(saturate(1.0 - v), 0.65) * (0.85 + 0.3 * Flicker) + 0.02;
float d = abs(x) / width;
float body = saturate(1.0 - d) * saturate(1.0 - v * (0.9 + 0.4 * (1.0 - Flicker)));
body = pow(body, 1.3) * smoothstep(0.0, 0.05, v);
float tongue = smoothstep(0.45, 0.75, n2) * saturate(1.0 - d * 0.8) * smoothstep(0.35, 0.9, v) * (1.0 - smoothstep(0.85, 1.0, v));
float heat = saturate(body * 1.4 + tongue * 0.6);
float3 col = lerp(float3(1.0, 0.32, 0.05), float3(1.0, 0.72, 0.32), saturate(heat * 1.3));
col = lerp(col, float3(1.0, 0.92, 0.75), saturate(heat - 0.75) * 2.0);
col = lerp(col, float3(0.25, 0.4, 1.0), smoothstep(0.08, 0.0, v) * saturate(1.0 - d * 1.5));
float strength = lerp(16.0, 2200.0, Day) * Flicker;
col = lerp(col, float3(1.0, 0.85, 0.65), Day * 0.5);
return col * (body + tongue * 0.5) * strength;
"""

MIST = ss.NOISE + """
// Steam: a soft puff over a heated pool, fading in and out over its rise. Mist: big sheets drifting 2 m/s over the
// valley, soft at their edges (their UVs), with gaps where the city shows through. Both fade into what's behind them.
float o;
if (Steam > 0.5)
{
    float2 c = UV - 0.5;
    float r = length(c) * 2.0;
    float puff = saturate(1.0 - r);
    puff *= puff;
    float n = S.Fbm(UV * 3.0 + float2(Seed * 13.0, -T * 0.25));
    float life = smoothstep(0.0, 0.15, Life) * (1.0 - smoothstep(0.55, 1.0, Life));
    o = puff * (0.4 + 0.6 * n) * life * 0.16 * (1.0 - 0.7 * Day);
    // Never a wall of steam over the lens: gone within 1.2 m of the camera, full from 4.5 m.
    o *= saturate((PixelZ - 120.0) / 330.0);
}
else
{
    float2 e = min(UV, 1.0 - UV);
    float edge = saturate(min(e.x, e.y) * 6.0);
    float2 w = P.xy / 100000.0 + float2(T * 0.002, T * 0.0008);
    float n = S.Fbm(w * 3.0) * 0.65 + S.Fbm(w * 11.0 + 4.0) * 0.35;
    o = edge * saturate((n - 0.32) * 2.2) * 0.55;
}
float fade = saturate((SceneZ - PixelZ) / max(DepthFade, 1.0));
return o * fade;
"""

CONIFER = """
// Needles from the atlas, snow on their upper sides (the mask's blue, where the card faces up).
float snow = saturate(Mask.b * saturate(N.z * 1.5 + 0.2));
float3 col = lerp(Base, float3(0.86, 0.89, 0.94), snow);
return float4(col, lerp(Mask.g, 0.6, snow));
"""

SKY_UV = """
// The star map's two wraps (each seam on the opposite side of the sky), as equirectangular UVs.
float3 d = normalize(P - Cam);
float az = atan2(d.y, d.x);
float azB = atan2(-d.y, -d.x);
float v = acos(clamp(d.z, -1.0, 1.0)) / 3.14159265;
return float4(az / 6.2831853 + 0.5, v, azB / 6.2831853 + 0.5, v);
"""

MOON_UV = """
// Where on the moon's face (0.9 degrees across) this direction falls.
float3 d = normalize(P - Cam);
float3 m = normalize(MoonDir);
float3 right = normalize(cross(float3(0.0, 0.0, 1.0), m));
float3 up = cross(m, right);
float2 q = float2(dot(d, right), dot(d, up)) / 0.0079;
return float2(q.x, -q.y) * 0.5 + 0.5;
"""

SKY = ss.NOISE + """
// Added over the atmosphere's night sky: the stars (the map, or the sky's own), the Milky Way, the moon's face and a
// soft halo, the city's glow low in the west. All of it fades in with the night (Night: the sun below -4..-12 degrees).
float3 d = normalize(P - Cam);
float el = d.z;
float3 col = float3(0.0, 0.0, 0.0);
float az = atan2(d.y, d.x);
float3 stars;
if (HasStarTex > 0.5)
{
    stars = lerp(StarB, StarA, step(abs(az), 1.5708)) * 2.0;
}
else
{
    float3 p = d * 420.0;
    float3 id = floor(p);
    float3 f = frac(p) - 0.5;
    float h = S.Hash(id.xy * 1.37 + id.z * 7.13);
    float on = step(0.9955, h);
    float2 jit = S.H22(id.xy + id.z) - 0.5;
    float dist = length(f.xy - jit * 0.5);
    float mag = pow(S.Hash(id.yz + 3.7), 6.0);
    float star = on * smoothstep(0.32, 0.0, dist) * (0.6 + 30.0 * mag);
    float tw = mag > 0.5 ? 0.75 + 0.25 * sin(T * (3.0 + 9.0 * h) + h * 50.0) : 1.0;
    float hue = S.Hash(id.xz + 1.9);
    float3 tint = hue > 0.9 ? float3(0.7, 0.8, 1.0) : (hue < 0.06 ? float3(1.0, 0.75, 0.55) : float3(1.0, 0.96, 0.92));
    float3 axis = normalize(float3(-0.35, 0.25, 0.9));
    float across = dot(d, axis) / 0.22;
    float band = exp(-across * across);
    float dust = S.Fbm(float2(az * 6.0, d.z * 8.0));
    stars = tint * star * tw * 1.5 + float3(0.55, 0.6, 0.75) * band * (0.25 + 0.5 * dust) * 0.35;
}
col += stars * smoothstep(-0.02, 0.12, el);
float3 m = normalize(MoonDir);
if (dot(d, m) > 0.99)
{
    float3 right = normalize(cross(float3(0.0, 0.0, 1.0), m));
    float3 up = cross(m, right);
    float2 q = float2(dot(d, right), dot(d, up)) / 0.0079;
    float lq = length(q);
    float disc = smoothstep(1.0, 0.96, lq);
    float3 face;
    if (HasMoonTex > 0.5)
    {
        face = MoonRGB * MoonA * disc;
    }
    else
    {
        float mare = S.Fbm(q * 2.2 + 5.0);
        float phase = smoothstep(-0.55, -0.35, q.x + 0.15 * q.y * q.y);
        face = float3(0.92, 0.9, 0.86) * (0.72 + 0.35 * mare) * phase * disc;
    }
    col += face * 400.0;
    col += float3(0.6, 0.68, 0.85) * exp(-max(lq - 1.0, 0.0) * 0.9) * 0.6 * step(1.0, lq);
}
float yaw = degrees(az);
float west = smoothstep(-30.0, -15.0, yaw) * (1.0 - smoothstep(25.0, 40.0, yaw));
float glow = west * exp(-max(el, 0.0) / 0.05) * step(-0.05, el);
col += float3(1.0, 0.62, 0.32) * glow * 0.9;
return col * Night;
"""


# ---------------------------------------------------------------- helpers

def _tex_path(name):
    return f"{TEX_DIR}/{name}"


def _has_tex(name):
    return bool(name) and eal.does_asset_exist(_tex_path(name))


def _stamp(textures=()):
    """This version, and the texture sets a material was built with (built again when one arrives)."""
    return VERSION + "|" + ",".join(t for t in textures if _has_tex(t))


def _material(name, force, stamp):
    """The material to build: new, forced, or built by an older version or with fewer textures. None when current."""
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


def _tex(mat, param, texture, sampler, x, y, uv=None, default=None):
    """A texture parameter sampled at uv: the imported texture when it's there, else the engine's default for its kind."""
    e = ss._expr(mat, unreal.MaterialExpressionTextureSampleParameter2D, x, y)
    e.set_editor_property("parameter_name", param)
    asset = eal.load_asset(_tex_path(texture)) if _has_tex(texture) else None
    if asset is None:
        asset = unreal.load_object(None, default or DEFAULTS[sampler])
    if asset is not None:
        e.set_editor_property("texture", asset)
    e.set_editor_property("sampler_type", sampler)
    if uv is not None:
        mel.connect_material_expressions(uv, "", e, "UVs")
    return e


def _green_flipped(mat, normal_sample, x, y):
    """A Blender-baked (OpenGL) tangent-space normal map read the engine's (DirectX) way: green flipped. The landscape's
    and the plaza's maps are OpenGL (ecx_landscape.py: "the setup flips green for DirectX"); the world-projected ones
    (ground, snow, rock) flip it in their own code."""
    return _custom(mat, "return float3(Nm.x, -Nm.y, Nm.z);", [("Nm", (normal_sample, "RGB"))], ss.F3, x, y, "OpenGL normal to DirectX")


def _const(mat, value, x, y):
    e = ss._expr(mat, unreal.MaterialExpressionConstant, x, y)
    e.set_editor_property("r", value)
    return e


def _prop(src, prop, pin=""):
    mel.connect_material_property(src, pin, prop)


def _on_nanite(mat):
    """A material the stage puts on Nanite meshes at runtime (the ground, the snow slots, the terrain, the trees): its Nanite
    shaders compiled now. Unflagged, the editor flags it on first use and dirties it, and a cooked game draws the default
    material on those meshes instead."""
    _set(mat, "used_with_nanite", True)


def _translucent(mat, lighting=None, two_sided=False):
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    if two_sided:
        mat.set_editor_property("two_sided", True)
    if lighting:
        mode = getattr(getattr(unreal, "TranslucencyLightingMode", None), lighting, None)
        if mode is not None:
            _set(mat, "translucency_lighting_mode", mode)


# ---------------------------------------------------------------- materials

def build_screen(force):
    stamp = _stamp()
    mat = _material("M_ECX_Screen", force, stamp)
    if not mat:
        return 0
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    uv = ss._expr(mat, unreal.MaterialExpressionTextureCoordinate, -1600, -200)
    lp = ss._expr(mat, unreal.MaterialExpressionLocalPosition, -1600, -100)
    uvs = _custom(mat, SCREEN_UV, [("UV", uv), ("LP", lp), ("LocalUV", ss._scalar(mat, "LocalUV", 0.0, -1600, 0)),
                                   ("Ticker", ss._scalar(mat, "Ticker", 0.0, -1600, 80)), ("Scroll", ss._scalar(mat, "Scroll", 0.0, -1600, 160)),
                                   ("ScrollScale", ss._scalar(mat, "ScrollScale", 1.0, -1600, 240))], ss.F4, -1300, 0, "Screen UVs")
    img_uv = ss._mask(mat, uvs, "rg", -1100, -100)
    txt_uv = ss._mask(mat, uvs, "ba", -1100, 100)
    ia = _tex(mat, "ImageA", None, COLOR, -900, -400, img_uv, BLACK)
    ib = _tex(mat, "ImageB", None, COLOR, -900, -200, img_uv, BLACK)
    ta = _tex(mat, "TextA", None, COLOR, -900, 0, txt_uv, BLACK)
    tb = _tex(mat, "TextB", None, COLOR, -900, 200, txt_uv, BLACK)
    c = _custom(mat, SCREEN, [("IA", (ia, "RGB")), ("IB", (ib, "RGB")), ("TA", (ta, "RGB")), ("TB", (tb, "RGB")), ("TAa", (ta, "A")), ("TBa", (tb, "A")),
                              ("UV0", img_uv), ("Mix", ss._scalar(mat, "Mix", 0.0, -900, 400)), ("Strength", ss._scalar(mat, "Strength", 300.0, -900, 480)),
                              ("Cells", ss._scalar(mat, "Cells", 0.0, -900, 560)), ("Grid", ss._scalar(mat, "Grid", 0.25, -900, 640)),
                              ("CellAspect", ss._scalar(mat, "CellAspect", 0.375, -900, 720)), ("HasImageA", ss._scalar(mat, "HasImageA", 0.0, -900, 800)),
                              ("HasImageB", ss._scalar(mat, "HasImageB", 0.0, -900, 880)), ("Tint", ss._vector(mat, "Tint", (1.0, 1.0, 1.0), -900, 960))],
                ss.F3, -500, 0, "LED screen")
    _prop(c, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    return 1 if _finish(mat, stamp) else 0


def build_sign(force):
    stamp = _stamp()
    mat = _material("M_ECX_Sign", force, stamp)
    if not mat:
        return 0
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    mat.set_editor_property("opacity_mask_clip_value", 0.3)
    uv = ss._expr(mat, unreal.MaterialExpressionTextureCoordinate, -1600, -200)
    lp = ss._expr(mat, unreal.MaterialExpressionLocalPosition, -1600, -100)
    zero = _const(mat, 0.0, -1600, 100)
    one = _const(mat, 1.0, -1600, 200)
    uvs = _custom(mat, SCREEN_UV, [("UV", uv), ("LP", lp), ("LocalUV", ss._scalar(mat, "LocalUV", 1.0, -1600, 0)), ("Ticker", zero), ("Scroll", zero),
                                   ("ScrollScale", one)], ss.F4, -1300, 0, "Sign UVs")
    ta = _tex(mat, "TextA", None, COLOR, -900, 0, ss._mask(mat, uvs, "rg", -1100, 0), BLACK)
    glow = _custom(mat, "return TA * Tint * Strength;", [("TA", (ta, "RGB")), ("Tint", ss._vector(mat, "Tint", (1.0, 1.0, 1.0), -900, 300)),
                                                        ("Strength", ss._scalar(mat, "Strength", 300.0, -900, 400))], ss.F3, -500, 0, "Lit letters")
    _prop(glow, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    _prop(ta, unreal.MaterialProperty.MP_OPACITY_MASK, "A")
    return 1 if _finish(mat, stamp) else 0


def _build_glass(name, tint, base_opacity, force):
    stamp = _stamp()
    mat = _material(name, force, stamp)
    if not mat:
        return 0
    _translucent(mat, "TLM_SURFACE_PER_PIXEL_LIGHTING")
    wp = ss._world_pos(mat, -900, -300)
    n = ss._expr(mat, unreal.MaterialExpressionVertexNormalWS, -900, -200)
    cam = ss._expr(mat, unreal.MaterialExpressionCameraPositionWS, -900, -100)
    c = _custom(mat, GLASS, [("P", wp), ("N", n), ("Cam", cam), ("SillZ", ss._scalar(mat, "SillZ", 0.0, -900, 0)),
                             ("BaseOpacity", ss._scalar(mat, "BaseOpacity", base_opacity, -900, 80))], ss.F2, -500, -100, "Clear glass, a breath at the sill")
    _prop(ss._mask(mat, c, "r", -250, -150), unreal.MaterialProperty.MP_OPACITY)
    _prop(ss._mask(mat, c, "g", -250, -50), unreal.MaterialProperty.MP_ROUGHNESS)
    _prop(ss._vector(mat, "Tint", tint, -500, 150), unreal.MaterialProperty.MP_BASE_COLOR)
    _prop(_const(mat, 0.5, -500, 250), unreal.MaterialProperty.MP_SPECULAR)
    return 1 if _finish(mat, stamp) else 0


def build_glass(force):
    return _build_glass("M_ECX_Glass", (0.018, 0.022, 0.026), 0.08, force) + _build_glass("M_ECX_GlassTint", (0.008, 0.009, 0.01), 0.35, force)


def build_ground(force):
    stamp = _stamp()
    mat = _material("M_ECX_Ground", force, stamp)
    if not mat:
        return 0
    mat.set_editor_property("used_with_instanced_static_meshes", True)
    _on_nanite(mat)
    _set(mat, "tangent_space_normal", False)
    wp = ss._world_pos(mat, -1700, -400)
    uv0 = ss._expr(mat, unreal.MaterialExpressionTextureCoordinate, -1700, -300)
    n = ss._expr(mat, unreal.MaterialExpressionVertexNormalWS, -1700, -200)
    vc = ss._expr(mat, unreal.MaterialExpressionVertexColor, -1700, -100)
    kind = ss._scalar(mat, "Kind", 0.0, -1700, 0)
    world_uv = ss._scalar(mat, "WorldUV", 0.0, -1700, 80)
    use_vc = ss._scalar(mat, "UseVertexColor", 1.0, -1700, 160)
    use_tex = ss._scalar(mat, "UseTex", 0.0, -1700, 240)
    tile = ss._scalar(mat, "Tile", 3.6, -1700, 320)
    wet = ss._scalar(mat, "Wet", 0.75, -1700, 400)
    tint = ss._vector(mat, "Tint", (0.26, 0.25, 0.24), -1700, 480)
    uvs = _custom(mat, GROUND_UV, [("UV0", uv0), ("P", wp), ("WorldUV", world_uv), ("Tile", tile)], ss.F2, -1400, 600, "Tile UVs")
    base = _tex(mat, "BaseTex", None, COLOR, -1100, 500, uvs)
    nrm = _tex(mat, "NormalTex", None, NORMAL, -1100, 700, uvs)
    orm = _tex(mat, "OrmTex", None, MASKS, -1100, 900, uvs)
    c = _custom(mat, GROUND, [("P", wp), ("N", n), ("UV0", uv0), ("VC", vc), ("Kind", kind), ("WorldUV", world_uv), ("UseVertexColor", use_vc),
                              ("UseTex", use_tex), ("Tint", tint), ("Wet", wet), ("TexBase", (base, "RGB")), ("TexOrm", (orm, "RGB"))],
                ss.F4, -600, -200, "The plaza's surfaces")
    _prop(ss._mask(mat, c, "rgb", -300, -250), unreal.MaterialProperty.MP_BASE_COLOR)
    _prop(ss._mask(mat, c, "a", -300, -150), unreal.MaterialProperty.MP_ROUGHNESS)
    nn = _custom(mat, GROUND_NORMAL, [("P", wp), ("N", n), ("UseTex", use_tex), ("TexNormal", (nrm, "RGB")), ("Kind", kind), ("VC", vc),
                                      ("UseVertexColor", use_vc), ("WorldUV", world_uv)], ss.F3, -600, 300, "Tile normal, flat puddles")
    _prop(nn, unreal.MaterialProperty.MP_NORMAL)
    ao = _custom(mat, "return lerp(1.0, Orm.r, UseTex);", [("Orm", (orm, "RGB")), ("UseTex", use_tex)], ss.F1, -600, 600, "Occlusion")
    _prop(ao, unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    return 1 if _finish(mat, stamp) else 0


def build_snow(force):
    sets = ("T_ECX_Snow_BaseColor", "T_ECX_Snow_Normal", "T_ECX_Snow_ORM")
    stamp = _stamp(sets)
    mat = _material("M_ECX_Snow", force, stamp)
    if not mat:
        return 0
    use_tex = 1.0 if all(_has_tex(t) for t in sets) else 0.0
    mat.set_editor_property("used_with_instanced_static_meshes", True)
    _on_nanite(mat)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_SUBSURFACE)
    _set(mat, "tangent_space_normal", False)
    wp = ss._world_pos(mat, -1600, -300)
    n = ss._expr(mat, unreal.MaterialExpressionVertexNormalWS, -1600, -200)
    cam = ss._expr(mat, unreal.MaterialExpressionCameraPositionWS, -1600, -100)
    uvs = _custom(mat, PROJECT_UV, [("P", wp), ("N", n), ("Tile", _const(mat, 400.0, -1600, 0))], ss.F2, -1300, 300, "Snow's projection")
    base = _tex(mat, "SnowBase", sets[0], COLOR, -1000, 200, uvs)
    nrm = _tex(mat, "SnowNormal", sets[1], NORMAL, -1000, 400, uvs)
    orm = _tex(mat, "SnowOrm", sets[2], MASKS, -1000, 600, uvs)
    flag = _const(mat, use_tex, -1300, 600)
    c = _custom(mat, SNOW, [("P", wp), ("N", n), ("Cam", cam), ("UseTex", flag), ("TexBase", (base, "RGB")), ("TexOrm", (orm, "RGB"))], ss.F4, -600, -200, "Snow")
    _prop(ss._mask(mat, c, "rgb", -300, -250), unreal.MaterialProperty.MP_BASE_COLOR)
    _prop(ss._mask(mat, c, "a", -300, -150), unreal.MaterialProperty.MP_ROUGHNESS)
    nn = _custom(mat, SNOW_NORMAL, [("P", wp), ("N", n), ("Cam", cam), ("UseTex", flag), ("TexNormal", (nrm, "RGB"))], ss.F3, -600, 200, "Snow's ripples and glints")
    _prop(nn, unreal.MaterialProperty.MP_NORMAL)
    # Blue in its shade: light scattering through it.
    _prop(ss._vector(mat, "Scatter", (0.35, 0.48, 0.75), -600, 450), unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
    _prop(_const(mat, 0.6, -600, 550), unreal.MaterialProperty.MP_OPACITY)
    return 1 if _finish(mat, stamp) else 0


def build_terrain(force):
    snow_sets = ("T_ECX_Snow_BaseColor", "T_ECX_Snow_Normal", "T_ECX_Snow_ORM")
    rock_sets = ("T_ECX_Rock_BaseColor", "T_ECX_Rock_Normal", "T_ECX_Rock_ORM")
    stamp = _stamp(snow_sets + rock_sets)
    mat = _material("M_ECX_Terrain", force, stamp)
    if not mat:
        return 0
    _on_nanite(mat)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_SUBSURFACE)
    _set(mat, "tangent_space_normal", False)
    wp = ss._world_pos(mat, -1600, -300)
    n = ss._expr(mat, unreal.MaterialExpressionVertexNormalWS, -1600, -200)
    vc = ss._expr(mat, unreal.MaterialExpressionVertexColor, -1600, -100)
    snow_uv = _custom(mat, PROJECT_UV, [("P", wp), ("N", n), ("Tile", _const(mat, 400.0, -1600, 0))], ss.F2, -1300, 300, "Snow's projection")
    rock_uv = _custom(mat, PROJECT_UV, [("P", wp), ("N", n), ("Tile", _const(mat, 600.0, -1600, 80))], ss.F2, -1300, 700, "Rock's projection")
    sb = _tex(mat, "SnowBase", snow_sets[0], COLOR, -1000, 200, snow_uv)
    so = _tex(mat, "SnowOrm", snow_sets[2], MASKS, -1000, 400, snow_uv)
    rb = _tex(mat, "RockBase", rock_sets[0], COLOR, -1000, 600, rock_uv)
    rn = _tex(mat, "RockNormal", rock_sets[1], NORMAL, -1000, 800, rock_uv)
    ro = _tex(mat, "RockOrm", rock_sets[2], MASKS, -1000, 1000, rock_uv)
    use_snow = _const(mat, 1.0 if all(_has_tex(t) for t in snow_sets) else 0.0, -1300, 900)
    use_rock = _const(mat, 1.0 if all(_has_tex(t) for t in rock_sets) else 0.0, -1300, 1000)
    c = _custom(mat, TERRAIN, [("P", wp), ("VC", vc), ("UseSnowTex", use_snow), ("SnowBase", (sb, "RGB")), ("SnowOrm", (so, "RGB")), ("UseRockTex", use_rock),
                               ("RockBase", (rb, "RGB")), ("RockOrm", (ro, "RGB"))], ss.F4, -600, -200, "Snow, rock, litter, crust")
    _prop(ss._mask(mat, c, "rgb", -300, -250), unreal.MaterialProperty.MP_BASE_COLOR)
    _prop(ss._mask(mat, c, "a", -300, -150), unreal.MaterialProperty.MP_ROUGHNESS)
    nn = _custom(mat, TERRAIN_NORMAL, [("P", wp), ("N", n), ("VC", vc), ("UseRockTex", use_rock), ("RockNormal", (rn, "RGB"))], ss.F3, -600, 200, "Terrain normal")
    _prop(nn, unreal.MaterialProperty.MP_NORMAL)
    _prop(ss._vector(mat, "Scatter", (0.35, 0.48, 0.75), -600, 450), unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
    amount = _custom(mat, "return lerp(0.6, 0.0, saturate(VC.r));", [("VC", vc)], ss.F1, -600, 550, "Scattering where there's snow")
    _prop(amount, unreal.MaterialProperty.MP_OPACITY)
    return 1 if _finish(mat, stamp) else 0


def build_water(force):
    stamp = _stamp()
    mat = _material("M_ECX_Water", force, stamp)
    if not mat:
        return 0
    _set(mat, "tangent_space_normal", False)
    nn = _custom(mat, WATER_NORMAL, [("P", ss._world_pos(mat, -900, -100)), ("T", ss._time(mat, -900, 0))], ss.F3, -500, 0, "Ripples toward the weir")
    _prop(nn, unreal.MaterialProperty.MP_NORMAL)
    _prop(ss._vector(mat, "Color", (0.008, 0.009, 0.01), -500, -250), unreal.MaterialProperty.MP_BASE_COLOR)
    _prop(ss._scalar(mat, "Roughness", 0.03, -500, 200), unreal.MaterialProperty.MP_ROUGHNESS)
    _prop(_const(mat, 0.35, -500, 300), unreal.MaterialProperty.MP_SPECULAR)
    return 1 if _finish(mat, stamp) else 0


def build_falls(force):
    stamp = _stamp()
    mat = _material("M_ECX_Falls", force, stamp)
    if not mat:
        return 0
    _translucent(mat, "TLM_SURFACE_PER_PIXEL_LIGHTING", two_sided=True)
    c = _custom(mat, FALLS, [("UV", ss._expr(mat, unreal.MaterialExpressionTextureCoordinate, -900, -200)),
                             ("LP", ss._expr(mat, unreal.MaterialExpressionLocalPosition, -900, -100)), ("LocalUV", ss._scalar(mat, "LocalUV", 0.0, -900, 0)),
                             ("T", ss._time(mat, -900, 100))], ss.F1, -500, 0, "The falling sheet")
    _prop(c, unreal.MaterialProperty.MP_OPACITY)
    _prop(ss._vector(mat, "Color", (0.1, 0.115, 0.13), -500, -250), unreal.MaterialProperty.MP_BASE_COLOR)
    _prop(_const(mat, 0.08, -500, 200), unreal.MaterialProperty.MP_ROUGHNESS)
    return 1 if _finish(mat, stamp) else 0


def build_flame(force):
    stamp = _stamp()
    mat = _material("M_ECX_Flame", force, stamp)
    if not mat:
        return 0
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    mat.set_editor_property("two_sided", True)
    c = _custom(mat, FLAME, [("UV", ss._expr(mat, unreal.MaterialExpressionTextureCoordinate, -900, -300)),
                             ("LP", ss._expr(mat, unreal.MaterialExpressionLocalPosition, -900, -200)), ("LocalUV", ss._scalar(mat, "LocalUV", 0.0, -900, -100)),
                             ("T", ss._time(mat, -900, 0)), ("Flicker", ss._scalar(mat, "Flicker", 1.0, -900, 100)), ("Seed", ss._scalar(mat, "Seed", 0.0, -900, 200)),
                             ("Day", ss._scalar(mat, "Day", 0.0, -900, 300))], ss.F3, -500, 0, "Gas flame")
    _prop(c, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    return 1 if _finish(mat, stamp) else 0


def build_mist(force):
    stamp = _stamp()
    mat = _material("M_ECX_Mist", force, stamp)
    if not mat:
        return 0
    _translucent(mat, None, two_sided=True)
    c = _custom(mat, MIST, [("UV", ss._expr(mat, unreal.MaterialExpressionTextureCoordinate, -900, -400)), ("P", ss._world_pos(mat, -900, -300)),
                            ("T", ss._time(mat, -900, -200)), ("Steam", ss._scalar(mat, "Steam", 0.0, -900, -100)), ("Life", ss._scalar(mat, "Life", 0.5, -900, 0)),
                            ("Seed", ss._scalar(mat, "Seed", 0.0, -900, 80)), ("Day", ss._scalar(mat, "Day", 0.0, -900, 160)),
                            ("SceneZ", ss._expr(mat, unreal.MaterialExpressionSceneDepth, -900, 240)), ("PixelZ", ss._expr(mat, unreal.MaterialExpressionPixelDepth, -900, 320)),
                            ("DepthFade", ss._scalar(mat, "DepthFade", 50000.0, -900, 400))], ss.F1, -500, 0, "Mist and steam")
    _prop(c, unreal.MaterialProperty.MP_OPACITY)
    _prop(ss._vector(mat, "Color", (0.78, 0.8, 0.84), -500, -250), unreal.MaterialProperty.MP_BASE_COLOR)
    # At night its underside takes the city's color (the stage sets it for the hour).
    _prop(ss._vector(mat, "UnderGlow", (0.0, 0.0, 0.0), -500, 250), unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    return 1 if _finish(mat, stamp) else 0


def build_conifer(force):
    sets = ("T_ECX_Needles_BaseColor", "T_ECX_Needles_Normal", "T_ECX_Needles_Mask")
    stamp = _stamp(sets)
    mat = _material("M_ECX_Conifer", force, stamp)
    if not mat:
        return 0
    mat.set_editor_property("used_with_instanced_static_meshes", True)
    _on_nanite(mat)  # masked Nanite foliage (spec 4.5)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    mat.set_editor_property("two_sided", True)
    mat.set_editor_property("opacity_mask_clip_value", 0.33)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
    uv = ss._expr(mat, unreal.MaterialExpressionTextureCoordinate, -1300, 0)
    base = _tex(mat, "NeedleBase", sets[0], COLOR, -1000, -200, uv)
    nrm = _tex(mat, "NeedleNormal", sets[1], NORMAL, -1000, 0, uv)
    mask = _tex(mat, "NeedleMask", sets[2], MASKS, -1000, 200, uv)
    c = _custom(mat, CONIFER, [("Base", (base, "RGB")), ("Mask", (mask, "RGBA")), ("N", ss._expr(mat, unreal.MaterialExpressionVertexNormalWS, -1000, 400))],
                ss.F4, -600, -100, "Needles and snow")
    _prop(ss._mask(mat, c, "rgb", -300, -150), unreal.MaterialProperty.MP_BASE_COLOR)
    _prop(ss._mask(mat, c, "a", -300, -50), unreal.MaterialProperty.MP_ROUGHNESS)
    _prop(base, unreal.MaterialProperty.MP_OPACITY_MASK, "A")
    _prop(_green_flipped(mat, nrm, -600, 100), unreal.MaterialProperty.MP_NORMAL)
    _prop(mask, unreal.MaterialProperty.MP_AMBIENT_OCCLUSION, "R")
    sss = _custom(mat, "return float3(0.05, 0.09, 0.04) * (0.4 + 0.6 * Mask.a);", [("Mask", (mask, "RGBA"))], ss.F3, -600, 300, "Light through the needles")
    _prop(sss, unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
    return 1 if _finish(mat, stamp) else 0


def build_bark(force):
    sets = ("T_ECX_Bark_BaseColor", "T_ECX_Bark_Normal", "T_ECX_Bark_ORM")
    stamp = _stamp(sets)
    mat = _material("M_ECX_Bark", force, stamp)
    if not mat:
        return 0
    mat.set_editor_property("used_with_instanced_static_meshes", True)
    _on_nanite(mat)
    uv = ss._expr(mat, unreal.MaterialExpressionTextureCoordinate, -1000, 0)
    has = all(_has_tex(t) for t in sets)
    base = _tex(mat, "BarkBase", sets[0], COLOR, -700, -200, uv)
    nrm = _tex(mat, "BarkNormal", sets[1], NORMAL, -700, 0, uv)
    orm = _tex(mat, "BarkOrm", sets[2], MASKS, -700, 200, uv)
    if has:
        _prop(base, unreal.MaterialProperty.MP_BASE_COLOR, "RGB")
        _prop(_green_flipped(mat, nrm, -400, 0), unreal.MaterialProperty.MP_NORMAL)
        _prop(orm, unreal.MaterialProperty.MP_ROUGHNESS, "G")
        _prop(orm, unreal.MaterialProperty.MP_AMBIENT_OCCLUSION, "R")
    else:
        _prop(ss._vector(mat, "Color", (0.07, 0.05, 0.04), -400, -200), unreal.MaterialProperty.MP_BASE_COLOR)
        _prop(_const(mat, 0.9, -400, 0), unreal.MaterialProperty.MP_ROUGHNESS)
    return 1 if _finish(mat, stamp) else 0


def build_sky(force):
    sets = ("T_ECX_StarField", "T_ECX_Moon")
    stamp = _stamp(sets)
    mat = _material("M_ECX_Sky", force, stamp)
    if not mat:
        return 0
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    mat.set_editor_property("two_sided", True)
    # Past the fog and the atmosphere's haze: the stars stay sharp (the night's own sky is the atmosphere's).
    _set(mat, "use_translucency_vertex_fog", False)
    wp = ss._world_pos(mat, -1600, -300)
    cam = ss._expr(mat, unreal.MaterialExpressionCameraPositionWS, -1600, -200)
    moon_dir = ss._vector(mat, "MoonDir", (0.79, -0.55, 0.37), -1600, -100)
    sky_uv = _custom(mat, SKY_UV, [("P", wp), ("Cam", cam)], ss.F4, -1300, 200, "Star map UVs")
    moon_uv = _custom(mat, MOON_UV, [("P", wp), ("Cam", cam), ("MoonDir", moon_dir)], ss.F2, -1300, 500, "Moon UVs")
    star_a = _tex(mat, "StarTex", sets[0], COLOR, -1000, 100, ss._mask(mat, sky_uv, "rg", -1150, 100), BLACK)
    star_b = _tex(mat, "StarTexB", sets[0], COLOR, -1000, 300, ss._mask(mat, sky_uv, "ba", -1150, 300), BLACK)
    moon = _tex(mat, "MoonTex", sets[1], COLOR, -1000, 500, moon_uv, BLACK)
    c = _custom(mat, SKY, [("P", wp), ("Cam", cam), ("T", ss._time(mat, -1600, 0)), ("Night", ss._scalar(mat, "Night", 1.0, -1600, 100)), ("MoonDir", moon_dir),
                           ("HasStarTex", ss._scalar(mat, "HasStarTex", 1.0 if _has_tex(sets[0]) else 0.0, -1600, 400)), ("StarA", (star_a, "RGB")),
                           ("StarB", (star_b, "RGB")), ("HasMoonTex", ss._scalar(mat, "HasMoonTex", 1.0 if _has_tex(sets[1]) else 0.0, -1600, 500)),
                           ("MoonRGB", (moon, "RGB")), ("MoonA", (moon, "A"))], ss.F3, -600, 0, "The night sky")
    _prop(c, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    return 1 if _finish(mat, stamp) else 0


def build_materials(force=False):
    """Builds the exterior's materials that are missing or out of date (all of them with force). Returns how many."""
    built = 0
    for build in (build_screen, build_sign, build_glass, build_ground, build_snow, build_terrain, build_water, build_falls, build_flame, build_mist,
                  build_conifer, build_bark, build_sky):
        try:
            built += build(force)
        except Exception as exc:  # the stage falls back to plain surfaces for each
            unreal.log_error(f"ShortStack: {build.__name__} failed: {exc}")
    return built


# ---------------------------------------------------------------- the imports' fix-ups

# Every mesh the spec marks "Nanite OFF": the glazing, the screens, the water, the flames, the mist (their materials are
# translucent or need their own triangles).
NANITE_OFF = {
    "SM_ECX_Podium_Glass", "SM_ECX_RevolvingDoor", "SM_ECX_RevolvingDoor_Wings", "SM_ECX_SwingDoor", "SM_ECX_Marquee_Screen",
    "SM_ECX_Cascade_Water", "SM_ECX_FireBowl_Flame", "SM_ECX_MonumentSign_Screen", "SM_ECX_BusShelter", "SM_ECX_Bus_Glass",
    "SM_ECX_Bus_DoorLeaf", "SM_ECX_Bus_Signs", "SM_ECX_ValleyMist",
}
# What the hero walks on: its own triangles are its collision.
COMPLEX = {"SM_ECX_Ground", "SM_ECX_SiteTerrain"}
# Kilometers across: Lumen traces them with hardware ray tracing (far field); their distance fields would be huge and useless.
NO_DISTANCE_FIELD = {"SM_ECX_Ridge_Near", "SM_ECX_Ridge_Mid", "SM_ECX_Ridge_Far", "SM_ECX_Valley", "SM_ECX_ValleyCity", "SM_ECX_ValleyMist"}


def fix_mesh(name, mesh, force=False):
    """The import rules for one SM_ECX_ mesh (shortstack_setup.import_meshes calls it after each import). True if it changed."""
    if not isinstance(mesh, unreal.StaticMesh):
        return False
    tag = FIX_VERSION + ":" + (eal.get_metadata_tag(mesh, "SourceHash") or "")
    if not force and eal.get_metadata_tag(mesh, FIX_TAG) == tag:
        return False
    sms = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    if name in NANITE_OFF:
        ss._without_nanite(mesh)
    if name == "SM_ECX_Ground":
        # Its collision (and ray tracing) are the Nanite fallback's triangles: kept at full detail, curbs and falls and all.
        try:
            nanite = mesh.get_editor_property("nanite_settings")
            if nanite.enabled:
                nanite.set_editor_property("fallback_target", unreal.NaniteFallbackTarget.RELATIVE_ERROR)
                nanite.set_editor_property("fallback_relative_error", 0.0)
                sms.set_nanite_settings(mesh, nanite, True)
        except Exception as exc:
            unreal.log_warning(f"ShortStack: couldn't keep {name}'s fallback at full detail: {exc}")
    # The stage places its own collision (spec 2.8): the importer's hull round each mesh would block the plaza.
    try:
        sms.remove_collisions(mesh)
    except Exception as exc:
        unreal.log_warning(f"ShortStack: couldn't remove {name}'s collision: {exc}")
    body = mesh.get_editor_property("body_setup")
    if body is not None:
        flag = unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE if name in COMPLEX else unreal.CollisionTraceFlag.CTF_USE_DEFAULT
        _set(body, "collision_trace_flag", flag)
    if name in NO_DISTANCE_FIELD:
        try:
            settings = sms.get_lod_build_settings(mesh, 0)
            settings.set_editor_property("distance_field_resolution_scale", 0.0)
            sms.set_lod_build_settings(mesh, 0, settings)
        except Exception as exc:
            unreal.log_warning(f"ShortStack: couldn't turn off {name}'s distance field: {exc}")
    eal.set_metadata_tag(mesh, FIX_TAG, tag)
    eal.save_loaded_asset(mesh)
    unreal.log(f"ShortStack: {name} fixed up for the Embercrest")
    return True


def fix_imports(force=False):
    """The rules on every SM_ECX_ mesh already imported (one imported before them, or not changed since, is fixed once)."""
    fixed = 0
    for folder in eal.list_assets(ss.MESH_DIR, recursive=False, include_folder=True):
        name = folder.rstrip("/").split("/")[-1]
        if not name.startswith("SM_ECX_"):
            continue
        path = f"{ss.MESH_DIR}/{name}/{name}"
        if eal.does_asset_exist(path):
            try:
                if fix_mesh(name, eal.load_asset(path), force):
                    fixed += 1
            except Exception as exc:
                unreal.log_error(f"ShortStack: fixing {name} failed: {exc}")
    return fixed


def fix_textures():
    """The ECX textures' formats: normal maps as normal maps, masks (ORM, the needles' mask, heights) linear.
    shortstack_setup.import_textures brings every group but Clothing in as color."""
    if not eal.does_directory_exist(TEX_DIR):
        return 0
    fixed = 0
    for asset_path in eal.list_assets(TEX_DIR, recursive=True, include_folder=False):
        tex = eal.load_asset(asset_path)
        if not isinstance(tex, unreal.Texture2D):
            continue
        name = tex.get_name()
        if name.endswith("_Normal"):
            want = (False, unreal.TextureCompressionSettings.TC_NORMALMAP)
        elif name.endswith(("_ORM", "_Mask", "_Height")):
            want = (False, unreal.TextureCompressionSettings.TC_MASKS)
        else:
            continue
        if tex.get_editor_property("srgb") == want[0] and tex.get_editor_property("compression_settings") == want[1]:
            continue
        tex.set_editor_property("srgb", want[0])
        tex.set_editor_property("compression_settings", want[1])
        eal.save_loaded_asset(tex)
        fixed += 1
    if fixed:
        unreal.log(f"ShortStack: {fixed} Embercrest textures set to their formats")
    return fixed


# ---------------------------------------------------------------- the level

def _map_current():
    world = unreal.load_asset(MAP_PATH)
    return world is not None and eal.get_metadata_tag(world, MAP_TAG) == MAP_VERSION


def build_map(force=False):
    """/Game/Maps/Embercrest: the stage actor and the game mode override (the stage builds the world when play begins).
    True when it was made or remade."""
    if eal.does_asset_exist(MAP_PATH) and not force:
        try:
            if _map_current():
                return False
        except Exception as exc:
            unreal.log_error(f"ShortStack: checking the Embercrest level failed: {exc}")
            return False
    stage_class = unreal.load_class(None, "/Script/ShortStack.EmbercrestStage")
    mode_class = unreal.load_class(None, "/Script/ShortStack.EmbercrestGameMode")
    if not stage_class or not mode_class:
        unreal.log_error("ShortStack: EmbercrestStage or EmbercrestGameMode not found (is the C++ module built?)")
        return False
    if unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages():
        unreal.log_warning("ShortStack: the open level has unsaved changes; run embercrest_setup.build_map(True) to make the Embercrest level")
        return False
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    world = editor.get_editor_world()
    previous = world.get_outermost().get_name() if world else ""
    if eal.does_asset_exist(MAP_PATH):
        levels.load_level(MAP_PATH)
    else:
        levels.new_level(MAP_PATH)
    stages = [a for a in actors.get_all_level_actors() if a.get_class() == stage_class]
    for extra in stages[1:]:
        actors.destroy_actor(extra)
    if not stages:
        actors.spawn_actor_from_class(stage_class, unreal.Vector(0.0, 0.0, 0.0), unreal.Rotator(0.0, 0.0, 0.0))
    world = editor.get_editor_world()
    settings = world.get_world_settings()
    settings.set_editor_property("default_game_mode", mode_class)
    # A pawn that ever falls below the plaza is removed 200 m down (the game mode puts the player back long before).
    _set(settings, "kill_z", -20000.0)
    try:
        eal.set_metadata_tag(world, MAP_TAG, MAP_VERSION)
    except Exception as exc:
        unreal.log_warning(f"ShortStack: couldn't mark the Embercrest level's version: {exc}")
    levels.save_current_level()
    unreal.log(f"ShortStack: made {MAP_PATH}")
    if previous and previous != MAP_PATH and eal.does_asset_exist(previous):
        levels.load_level(previous)
    return True


def run(force=False):
    """The exterior's part of the editor's setup (shortstack_setup.run calls it last): the textures' formats, the
    materials, the meshes' rules, the level."""
    fix_textures()
    built = build_materials(force)
    fix_imports(force)
    build_map(force)
    return built
