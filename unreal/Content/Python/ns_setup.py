"""
SHORT STACK: Northside's materials and the fix-ups of its imports (world realism spec 0.5-0.8, 3.4, 3.5, 8.2 ue-world).

What the street's art (art/blender/assets/ns_*.py) leaves for the engine, all procedural HLSL like M_Surface, each reading
the tiling sets (unreal/Art/Textures/NS, imported by shortstack_setup.import_textures) where they are:

  MPC_NS            the hour and the weather to every M_NS_ material (AStreetStage sets it each frame, spec 0.7): Hour,
                    Weekday, Daylight, SunElev, Rain, Wetness, Puddle, Lightning, WindX, WindY, Seed, Night, Cloud
  M_NS_Ground       the ground's slots (ns_sidewalk .. ns_busbay, spec 4.1, 3.5): the tiling set, grime and the shelter from
                    the vertex color, wet by MPC_NS.Wetness, puddles where R x Puddle fills them (mirror-flat, rain rings
                    in them), the markings' paint worn by their alpha; MI_NS_Ground_<Slot> per slot
  M_NS_Masonry      the buildings' tile slots (ns_brick_red .. ns_roof, spec 4.4): soot, edge wear and rain exposure from
                    the vertex color, the painted brick's paint lost toward the sidewalk, wet and streaked in rain;
                    MI_NS_Masonry_<Set> per slot
  M_NS_WindowRoom   the kit's windows (ns_window, spec 3.4): an interior-mapped room from T_NS_Rooms_*, its front layer
                    (T_NS_RoomFront), lit by the hour (Occupancy by kind and weekday), a TV's flicker, the glass's grime
  M_NS_Glass        shop and display glass (ns_glass, spec 3.4): translucent with Lumen's front-layer reflections, beads
                    and runners on the street side in rain, condensation inside on cold wet nights
  M_NS_Glow         the glow meshes (_GlowWarm, _GlowCool, _GlowNeon, _GlowSignal): a color and a level the stage sets by
                    the hour; the Signal grid's cells lit by LitA/LitB (spec 0.5) with the countdown's digits; the
                    Rialto's chaser bulbs. Lit: unlit by day a tube, a bulb or a lens reads as its glass
  M_NS_GlowAdd      the same light added over what's under it (unlit, additive): the cars' _GlowLamps films, which lie
                    over the baked lenses and glass (ns_cars.py), so an off lamp adds nothing
  M_NS_Screen       a [screen] slot (the plates, the 14's timetable and ad, the Rialto's marquee, the awnings'
                    valances): core's picture drawn into a render target at play (Ns::FScreens) over the slot's own
                    color where it's clear, wet in the rain, shining when it's backlit
  M_NS_Leaf         the trees' leaves (ns_leaf): masked two-sided foliage on Nanite, swaying in MPC_NS's wind by the
                    leaves' wind weight (COLOR_0.R); M_NS_Bark the trees' baked bark, swaying with them (MI_NS_Bark_<Tree>)
  M_NS_SkyGlow      the city's sodium glow on the low cloud at night (an additive sphere past everything, spec 3.2)
  M_NS_Decal        the street's decals (DBuffer, a cell of T_NS_Decals: MI_NS_Decal_00..15, spec 3.5)
  M_NS_RainFx       the rain's hits on the ground round the camera (Mode 0: a flat card, a hit, a ring and specks thrown
                    off) and the drips off the awnings and the shed (Mode 1: a thin upright line, drops gathering at its top
                    and falling its length by world Z, its top and length in the instance's custom data); masked, lit, by
                    MPC_NS.Rain and Wetness (spec 3.3)

and the imports' fix-ups (fix_mesh, after every import of an SM_NS_ mesh):
  - every slot named for a runtime material (ns_sidewalk, ns_brick_red, ns_window, ns_glass, ns_leaf...) gets it on the
    asset itself, so the editor's preview, the cook and play all draw it, with no material made at run time;
  - Nanite off for [glass], [screen], plates, steam, the cars' lamp films and the hand props (spec 3.8);
  - the ground's own triangles are its collision (complex as simple, the Nanite fallback at full detail); every other
    mesh's imported collision goes (the stages place boxes, spec 0.5);
  - the trees: Nanite on (masked foliage), their leaves out of the ray-traced scene (AStreetStage.ns_section_flags);
and the NS textures' formats (normal maps green-flipped from Blender's OpenGL, masks linear).

street_setup.build_map() runs it before it checks the Street level (after shortstack_setup.run()'s imports), and
shortstack_setup may call run() as an optional import. A material built by an older version of this file, or before a
texture set it reads was imported, is rebuilt; anything current is left alone.

    import ns_setup; ns_setup.run(force=True)
"""
import unreal

import shortstack_setup as ss

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

TEX_DIR = "/Game/ShortStack/Textures/NS"
MI_DIR = "/Game/ShortStack/Materials/NS"
MPC_PATH = f"{ss.MAT_DIR}/MPC_NS"
# Bump when a material's graph changes (each carries it, with the texture sets it was built with, as metadata). "2": M_NS_Glow
# lit (its glass by day), M_NS_GlowAdd, M_NS_Screen.
VERSION = "2"
VERSION_TAG = "NSVersion"
# Bump when fix_mesh's rules change (each mesh carries the version and its source's hash once fixed). "2": the cars' lamp films
# (_GlowLamps) off Nanite (M_NS_GlowAdd is additive).
FIX_VERSION = "2"
FIX_TAG = "NSFix"

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
BLACK = "/Engine/EngineResources/Black.Black"
WHITE = "/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"

# MPC_NS's parameters and their defaults (a clear, dry night until the stage sets them).
MPC_SCALARS = (("Hour", 22.0), ("Weekday", 1.0), ("Daylight", 0.0), ("SunElev", -30.0), ("Rain", 0.0), ("Wetness", 0.0), ("Puddle", 0.0),
               ("Lightning", 0.0), ("WindX", 0.2), ("WindY", 0.1), ("Seed", 0.37), ("Night", 1.0), ("Cloud", 0.2))

# The ground's slots (ns_ground.py SLOTS), each its kind in M_NS_Ground, its texture set, its tile (m).
GROUND_SLOTS = (
    ("ns_sidewalk", "Sidewalk", 0, "T_NS_Sidewalk_A", "T_NS_Sidewalk_B", 3.0),
    ("ns_asphalt", "Asphalt", 1, "T_NS_Asphalt", None, 4.0),
    ("ns_patch", "Patch", 2, "T_NS_Patch", None, 4.0),
    ("ns_curb", "Curb", 3, "T_NS_Curb", None, 2.0),
    ("ns_gutter", "Gutter", 4, "T_NS_Gutter", None, 2.0),
    ("ns_concrete", "Concrete", 5, "T_NS_Concrete", None, 2.0),
    ("ns_tactile", "Tactile", 6, "T_NS_Tactile", None, 0.6),
    ("ns_marking", "Marking", 7, "T_NS_Thermo", None, 2.0),
    ("ns_busbay", "BusBay", 8, "T_NS_BusBay", None, 4.0),
)
# The buildings' tile slots (ns_fronts.py), each its kind in M_NS_Masonry and its set.
MASONRY_SLOTS = (
    ("ns_brick_red", "BrickRed", 0, "T_NS_Brick_Red"),
    ("ns_brick_buff", "BrickBuff", 0, "T_NS_Brick_Buff"),
    ("ns_brick_dark", "BrickDark", 0, "T_NS_Brick_Dark"),
    ("ns_brick_painted", "BrickPainted", 1, "T_NS_Brick_Painted"),
    ("ns_limestone", "Limestone", 0, "T_NS_Limestone"),
    ("ns_caststone", "CastStone", 0, "T_NS_CastStone"),
    ("ns_stucco", "Stucco", 0, "T_NS_Stucco"),
    ("ns_roof", "Roof", 2, "T_NS_Roof"),
)
TREES = ("SM_NS_Tree_A", "SM_NS_Tree_B")

# ---------------------------------------------------------------- HLSL

# The puddle mask, the same in the color and the normal (VC.r puddle-prone, Fill how full the puddles are, H the tile's
# height 0..1: joints and recesses fill first; level ground only).
PUDDLE = """
struct SSPuddle
{
    float Of(float prone, float fill, float h, float n, float up)
    {
        float p = saturate(((prone * 1.25 + (n - 0.5) * 0.35) - (1.0 - fill) - h * 0.25) * 5.0);
        return p * step(0.01, fill) * saturate((up - 0.9) * 10.0);
    }
};
SSPuddle Pd;
"""

GROUND = ss.NOISE + """
// The ground's slots by kind (0 sidewalk, 1 asphalt, 2 patch, 3 curb, 4 gutter, 5 concrete, 6 tactile, 7 marking,
// 8 bus bay). Returns base color and roughness. VC: R puddle-prone, G grime, B shelter (A the markings' wear).
int k = (int)(Kind + 0.5);
float3 col = BaseA;
float rough = OrmA.g;
float h = HeightA;
if (k == 0)
{
    // Two sets of flags, blended across the block by a slow noise (no tile repeats in view).
    float w = smoothstep(0.42, 0.58, S.Fbm(P.xy / 600.0 + Seed * 13.0));
    col = lerp(BaseA, BaseB, w);
    rough = lerp(OrmA.g, OrmB.g, w);
}
float grime = VC.g;
float shelter = VC.b;
float prone = VC.r;
if (k == 7)
{
    // Thermoplastic: the paint (the vertex color) over the beaded white, worn through to the asphalt where the wear
    // pattern is under the face's wear (alpha).
    float gone = smoothstep(Wear - 0.08, Wear + 0.08, VCA);
    col = lerp(BaseA * VC.rgb * 1.2, float3(0.035, 0.036, 0.04), gone);
    rough = lerp(0.55, 0.75, gone);
    grime = 0.0;
    shelter = 0.0;
    prone = 0.0;
}
col *= lerp(1.0, 0.72, grime);
float wet = saturate(Wetness * (1.0 - 0.85 * shelter));
if (k == 4)
{
    // The gutters run: wet before anything else, last to dry.
    wet = saturate(wet * 1.4 + 0.15 * Wetness);
}
col *= lerp(1.0, k == 7 ? 0.8 : 0.58, wet);
rough = k == 7 ? lerp(0.55, 0.25, wet) : lerp(rough, rough * 0.35, wet);
float puddle = Pd.Of(prone, Puddle * (1.0 - 0.6 * shelter), h, S.Fbm(P.xy / 90.0 + 31.0), N.z);
col = lerp(col, col * 0.55, puddle);
rough = lerp(rough, 0.03, puddle);
return float4(col, saturate(rough));
"""

GROUND_NORMAL = ss.NOISE + """
// Tangent space: the tile's normal, flat in the puddles, the rain's rings spreading in them (and a fine stir of drops
// on wet ground).
int k = (int)(Kind + 0.5);
float3 t = NrmA;
if (k == 0)
{
    float w = smoothstep(0.42, 0.58, S.Fbm(P.xy / 600.0 + Seed * 13.0));
    t = normalize(lerp(NrmA, NrmB, w));
}
float prone = k == 7 ? 0.0 : VC.r;
float shelter = k == 7 ? 0.0 : VC.b;
float puddle = Pd.Of(prone, Puddle * (1.0 - 0.6 * shelter), HeightA, S.Fbm(P.xy / 90.0 + 31.0), N.z);
float3 n = normalize(lerp(t, float3(0.0, 0.0, 1.0), puddle));
if (Rain > 0.02 && N.z > 0.7)
{
    float2 uv = P.xy / 100.0;
    float2 g = float2(0.0, 0.0);
    float2 rp = uv * 3.0;
    float2 id = floor(rp);
    for (int j = -1; j <= 1; j++)
    {
        for (int i = -1; i <= 1; i++)
        {
            // A drop lands somewhere in each cell on its own clock; its ring spreads and fades.
            float2 c = id + float2(i, j);
            float2 hh = S.H22(c);
            float tt = frac(T * (0.7 + 0.6 * hh.x) * (0.5 + Rain) + hh.y);
            float2 d = rp - (c + 0.15 + 0.7 * hh);
            float dist = length(d);
            float x = (dist - tt * 1.1) * 14.0;
            float wave = sin(x * 3.14159) * saturate(1.0 - abs(x) * 0.5) * (1.0 - tt) * (1.0 - tt);
            g += d / max(dist, 1e-3) * wave;
        }
    }
    float kk = lerp(0.04 * Wetness, 0.35, puddle) * saturate(Rain * 1.5);
    n = normalize(n + float3(g * kk, 0.0));
}
return n;
"""

MASONRY = ss.NOISE + """
// The buildings' tile slots by kind (0 masonry, 1 painted brick, 2 roof). Returns base color and roughness. VC: R soot
// and streaks, G edge wear, B rain exposure. Z: cm over the sidewalk (the SITE mesh's origin is the stage's).
int k = (int)(Kind + 0.5);
float3 col = BaseA;
float rough = OrmA.g;
if (k == 1)
{
    // Paint holds where its mask is over the loss, which rises toward the sidewalk: red brick shows through low down.
    float loss = saturate(1.0 - Z / 120.0) * 0.85 + 0.15 * S.Fbm(P.xy / 80.0 + P.z / 60.0);
    float bare = 1.0 - smoothstep(loss - 0.06, loss + 0.06, Mask);
    col = lerp(BaseA, BaseB, bare);
    rough = lerp(OrmA.g, OrmB.g, bare);
}
// Soot and grime: darker and greyer; the arrises worn lighter (hands, carts, weather).
float soot = VC.r;
float lum = dot(col, float3(0.3, 0.59, 0.11));
col = lerp(col, lum * float3(0.62, 0.6, 0.58), soot * 0.5) * (1.0 - 0.45 * soot);
col = lerp(col, col * 1.18 + 0.015, VC.g * 0.45);
rough = saturate(rough + 0.08 * VC.g);
// Rain: the exposed faces darken and gloss, streaks run down under the sills while it falls.
float expo = k == 2 ? 1.0 : VC.b;
float wet = saturate(Wetness * expo);
float streak = saturate((S.Fbm(float2((P.x + P.y) / 35.0, P.z / 400.0)) - 0.45) * 3.0) * saturate(Rain * 1.5) * expo;
wet = saturate(wet + streak * 0.5);
col *= lerp(1.0, 0.68, wet);
rough = lerp(rough, rough * 0.4, wet);
return float4(col, saturate(rough));
"""

WINDOW = ss.NOISE + """
// A kit window (spec 3.4): an interior-mapped room behind opaque glass. UV: the pane's 0..1 (v runs down, glTF's),
// V: the view ray in the pane's tangent space (x across, y along v, z out of the glass), VC: R the window's hash, G its
// floor over the building's height, B the room's kind (0 apartment, 0.33 office or storage, 0.66 stairwell, 1 a shop's
// back room). Returns emissive (rgb) and the glass's roughness (a).
float hash = VC.r;
float kindB = VC.b;
float h2 = frac(hash * 7.31 + Seed * 3.1);
float h3 = frac(hash * 13.17 + Seed * 5.7);
float h4 = frac(hash * 29.3 + 0.17);
float up = 1.0 - UV.y;
// The pane, taken as a 1.2 x 1.8 m opening (ns_rooms' frame), the room 3.2 m deep x 0.8..1.2.
float W = 1.2;
float H = 1.8;
float D = 3.2 * (0.8 + 0.4 * h2);
float3 p = float3((UV.x - 0.5) * W, (up - 0.5) * H, 0.0);
float3 r = float3(V.x, -V.y, V.z);
r = -r; // from the eye into the room
r.z = min(r.z, -1e-3);
float tx = ((r.x > 0.0 ? 0.5 : -0.5) * W - p.x) / (abs(r.x) > 1e-4 ? r.x : 1e-4);
float ty = ((r.y > 0.0 ? 0.5 : -0.5) * H - p.y) / (abs(r.y) > 1e-4 ? r.y : 1e-4);
float tz = -D / r.z;
float t = min(min(abs(tx), abs(ty)), tz);
float3 hit = p + r * t;
float z = -hit.z;
// Back through the pinhole the cell was rendered from (a window-width in front of the opening).
float d = W;
float u = 0.5 + hit.x / W * d / (d + z);
float v = 0.5 + hit.y / H * d / (d + z);
// The room by the kind and the hash.
float cell = 0.0;
int kind = kindB < 0.15 ? 0 : (kindB < 0.5 ? 1 : (kindB < 0.85 ? 2 : 3));
if (kind == 0) { cell = floor(h2 * 11.999); }
else if (kind == 1) { cell = h2 < 0.75 ? 12.0 : 13.0; }
else if (kind == 2) { cell = 14.0; }
else { cell = 15.0; }
float col = fmod(cell, 4.0);
float row = floor(cell / 4.0);
float2 cuv = float2((col + saturate(u)) / 4.0, (row + 1.0 - saturate(v)) / 4.0);
// No derivatives here (Nanite's compute shading, ray tracing): the mip from how far the window is.
float mip = clamp(log2(max(Dist, 1.0) / 250.0), 0.0, 7.0);
float3 albedo = Rooms.SampleLevel(RoomsSampler, cuv, mip).rgb;
float3 lamp = Lamps.SampleLevel(LampsSampler, cuv, mip).rgb;
// Occupancy by the hour and the weekday (spec 3.4), each window switching at its own minute.
float hr = fmod(Hour + (h4 - 0.5) * 0.8 + 24.0, 24.0);
float occ = 0.15;
if (kind == 0)
{
    occ = hr >= 19.0 && hr < 23.0 ? 0.55 : (hr >= 23.0 || hr < 1.0 ? 0.35 : (hr < 2.0 ? 0.25 : (hr < 5.0 ? 0.08 : (hr < 6.0 ? 0.15 : (hr < 8.0 ? 0.3 : (hr < 17.5 ? 0.15 : 0.35))))));
}
else if (kind == 1)
{
    occ = Weekday < 4.5 && hr >= 9.0 && hr < 18.0 ? 0.7 : 0.04;
}
else if (kind == 2)
{
    occ = 1.0;
}
else
{
    occ = hr >= 10.0 && hr < 20.0 && Weekday < 5.5 ? 0.92 : 0.06;
}
float lit = step(h3, occ);
// The lamp's color: warm mostly, some cool, some a TV's flicker.
float3 warm = lerp(float3(1.0, 0.62, 0.32), float3(1.0, 0.78, 0.5), frac(hash * 3.7));
float3 cool = float3(0.75, 0.86, 1.0);
float tvRate = 0.5 + 1.5 * frac(hash * 5.1);
float3 tvc = lerp(float3(0.45, 0.6, 1.0), float3(0.8, 0.85, 1.0), 0.5 + 0.5 * sin(T * 0.7 + hash * 30.0));
float tvk = 0.55 + 0.45 * sin(T * tvRate * 6.283 + hash * 40.0) * sin(T * tvRate * 2.1 + hash * 9.0);
float pick = frac(hash * 17.3 + 0.41);
float3 tint = kind == 2 ? float3(1.0, 0.9, 0.75) : (kind == 1 ? cool : (pick < 0.7 ? warm : (pick < 0.85 ? cool : tvc * tvk)));
// By day the room reads by its own window (the albedo atlas, lit by the daylight); at night by its lamp.
float3 room = albedo * DayNits * Daylight + lamp * tint * LampNits * lit;
// The front layer in the glass's own plane: curtains, blinds, plants (the lamp shows through, warm).
float fcell = kind == 0 ? floor(frac(hash * 3.3 + 0.5) * 15.999) : (kind == 1 ? (h4 < 0.5 ? 4.0 : 12.0) : 15.0);
float2 fuv = float2((fmod(fcell, 4.0) + UV.x) / 4.0, (floor(fcell / 4.0) + 1.0 - up) / 4.0);
float4 front = Fronts.SampleLevel(FrontsSampler, fuv, mip);
float3 frontLit = front.rgb * (DayNits * 0.8 * Daylight + LampNits * 0.35 * lit * tint);
room = lerp(room, frontLit, front.a);
// The glass: a film of grime by the hash, darker toward the frame.
float2 e = min(UV, 1.0 - UV);
float edge = smoothstep(0.0, 0.05, min(e.x, e.y));
float grime = 0.15 + 0.5 * frac(hash * 23.7);
room *= lerp(0.6, 1.0, edge) * (1.0 - 0.25 * grime);
return float4(room, 0.04 + 0.12 * grime);
"""

GLASS = ss.DROPS + """
// Shop glass (spec 3.4): clear, beads and runners on the street side while it rains, condensation inside on a cold wet
// night (along the sill and into the corners), a little smudged low where hands push it. Returns opacity, roughness.
float3 an = abs(N);
float3 o = P - ObjPos;
float2 uv = (an.x >= an.y ? o.yz : o.xz) / 100.0;
float outside = Side > 0.0 ? 1.0 : 0.0;
float wet = saturate(Rain * 1.4) * outside;
float3 b1 = D.Beads(uv, T, 14.0) * wet;
float3 b2 = D.Beads(uv + 3.7, T * 0.8, 30.0) * (0.6 * wet);
float water = saturate(b1.z + b2.z);
float cold = (1.0 - Outdoor) + Outdoor * (1.0 - Daylight) * saturate((Rain - 0.5) * 2.0);
float fog = (1.0 - outside) * cold * saturate(1.0 - (P.z - ObjPos.z) / 90.0) * (0.4 + 0.6 * S.Fbm(uv * 3.0));
float smudge = saturate((S.Fbm(uv * 4.0 + 5.0) - 0.55) * 3.0) * saturate(1.3 - (P.z - ObjPos.z) / 120.0);
float3 v = normalize(Cam - P);
float fres = pow(1.0 - saturate(abs(dot(v, normalize(N)))), 5.0);
float opacity = saturate(0.05 + 0.22 * water + 0.08 * smudge + 0.5 * fres + 0.45 * fog);
float rough = lerp(0.02, 0.2, max(max(water * 0.6, smudge * 0.5), fog));
return float2(opacity, rough);
"""

GLOW = """
// A glow mesh: Mode 0 its color, 1 the traffic signals' grid (cells 0 red, 1 amber, 2 green, 3 walk, 4 hand, 5 the
// countdown, 6-7 spare; LitA cells 0-3, LitB 4-7), 2 a car's lamps (0 head-low, 1 head-high, 2 tail, 3 brake, 4 reverse,
// 5 turn-L, 6 turn-R, 7 dome), 3 chaser bulbs (each bulb's phase in u). UV's v runs down (glTF's).
int mode = (int)(Mode + 0.5);
float3 c = Color.rgb * Level;
float4 la = float4(LitA.rgb, LitAw);
float4 lb = float4(LitB.rgb, LitBw);
if (mode == 1 || mode == 2)
{
    float up = 1.0 - UV.y;
    int colI = (int)clamp(floor(UV.x * 4.0), 0.0, 3.0);
    int rowI = up < 0.5 ? 0 : 1;
    int cell = rowI * 4 + colI;
    float lu = frac(UV.x * 4.0);
    float lv = frac(up * 2.0);
    float lit = cell < 4 ? la[cell] : lb[cell - 4];
    float3 hue = float3(1.0, 1.0, 1.0);
    if (mode == 1)
    {
        hue = cell == 0 ? float3(1.0, 0.08, 0.03) : (cell == 1 ? float3(1.0, 0.45, 0.04) : (cell == 2 ? float3(0.1, 1.0, 0.55) : (cell == 3 ? float3(0.92, 0.95, 1.0) : float3(1.0, 0.42, 0.08))));
        if (cell == 5)
        {
            // The countdown's two digits, seven segments each, in the face's middle.
            int n = (int)clamp(Count, 0.0, 99.0);
            float on = 0.0;
            for (int dI = 0; dI < 2; dI++)
            {
                int dig = dI == 0 ? n / 10 : n - (n / 10) * 10;
                if (dI == 0 && dig == 0) { continue; }
                float x0 = dI == 0 ? 0.14 : 0.54;
                float2 q = float2((lu - x0) / 0.32, (lv - 0.12) / 0.76);
                if (q.x < 0.0 || q.x > 1.0 || q.y < 0.0 || q.y > 1.0) { continue; }
                // Segments a..g as bits of the digit's mask.
                int masks[10] = {63, 6, 91, 79, 102, 109, 125, 7, 127, 111};
                int m = masks[dig];
                float th = 0.16;
                float sa = step(1.0 - th, q.y) * step(th, q.x) * step(q.x, 1.0 - th);
                float sg = step(abs(q.y - 0.5), th * 0.5) * step(th, q.x) * step(q.x, 1.0 - th);
                float sd = step(q.y, th) * step(th, q.x) * step(q.x, 1.0 - th);
                float sb = step(1.0 - th, q.x) * step(0.5, q.y);
                float sc = step(1.0 - th, q.x) * step(q.y, 0.5);
                float sf = step(q.x, th) * step(0.5, q.y);
                float se = step(q.x, th) * step(q.y, 0.5);
                on = max(on, sa * ((m >> 0) & 1) + sb * ((m >> 1) & 1) + sc * ((m >> 2) & 1) + sd * ((m >> 3) & 1) + se * ((m >> 4) & 1) + sf * ((m >> 5) & 1) + sg * ((m >> 6) & 1));
            }
            lit *= saturate(on);
        }
    }
    else
    {
        hue = cell == 0 || cell == 1 || cell == 4 ? float3(1.0, 0.95, 0.86) : (cell == 2 ? float3(1.0, 0.04, 0.02) : (cell == 3 ? float3(1.0, 0.03, 0.02) : (cell == 7 ? float3(1.0, 0.82, 0.6) : float3(1.0, 0.5, 0.05))));
    }
    // A lamp off adds nothing: M_NS_Glow's lit glass (GLOW_BASE) shows it, M_NS_GlowAdd lets the baked lens show.
    c = hue * Level * max(lit, 0.0);
}
else if (mode == 3)
{
    float on = step(frac(UV.x * Groups - T * ChaseRate), 0.5);
    c *= lerp(0.15, 1.0, ChaseRate > 0.0 ? on : 1.0);
}
// Neon's breath: a little flicker on old tubes.
c *= 1.0 - Flicker * (0.5 + 0.5 * sin(T * 43.0 + UV.x * 9.0)) * (0.5 + 0.5 * sin(T * 7.3));
return c;
"""

GLOW_BASE = """
// The glass of a glow mesh as it looks unlit (M_NS_Glow is lit: by day a tube, a bulb or a lens reads as glass, not as a
// black cut-out): the tube's own tint, darkened; on the signals' grid each lens tinted by its lamp's color.
int mode = (int)(Mode + 0.5);
float3 b = Color.rgb * 0.05 + 0.02;
if (mode == 1)
{
    float up = 1.0 - UV.y;
    int colI = (int)clamp(floor(UV.x * 4.0), 0.0, 3.0);
    int cell = (up < 0.5 ? 0 : 4) + colI;
    float3 hue = cell == 0 ? float3(0.5, 0.03, 0.02) : (cell == 1 ? float3(0.45, 0.2, 0.02) : (cell == 2 ? float3(0.03, 0.3, 0.16) : (cell == 3 ? float3(0.3, 0.3, 0.3) : float3(0.4, 0.15, 0.04))));
    b = hue * 0.12 + 0.01;
}
return b;
"""

SCREEN = """
// A [screen] face (FScreens): core's picture (Image, drawn into a render target) over the slot's own color where the
// picture is clear (an awning's fabric, Back); printed faces lit by the street, wet in the rain; backlit faces shine (Glow).
// Returns base color (rgb) and roughness (a); the emissive is Img.rgb * Img.a * Glow.
float3 col = lerp(Back, Img.rgb, saturate(Img.a));
float wet = saturate(Wetness * Exposed);
col *= lerp(1.0, 0.85, wet);
return float4(col, saturate(lerp(Rough, Rough * 0.4, wet)));
"""

LEAF = """
// Masked foliage: the leaf atlas's color, a touch of autumn by the hash of where the tree stands.
return Base;
"""

SWAY = ss.NOISE + """
// World position offset: the limbs and the leaves (VC.r, the wind weight) swaying in MPC_NS's wind, gusting, each tree
// its own phase; the leaves flutter on top.
float w = VC.r;
float2 wind = float2(WindX, WindY);
float strength = length(wind);
float phase = dot(ObjPos.xy, float2(0.0031, 0.0047));
float gust = 0.6 + 0.4 * sin(T * 0.37 + phase) * sin(T * 0.11 + phase * 1.7);
float sway = sin(T * 1.1 + phase + P.z * 0.004) * 0.6 + sin(T * 2.3 + phase * 2.0) * 0.25;
float3 off = float3(wind * (sway + 0.6) * gust * 14.0 * w * w, 0.0);
float flutter = sin(T * 9.0 + dot(P, float3(0.13, 0.17, 0.11))) * 1.2 * w * w * w * (0.3 + strength);
off += float3(flutter, flutter * 0.6, flutter * 0.4);
return off * Enabled;
"""

SKYGLOW = """
// The city's light on the low cloud at night: sodium orange strongest at the horizon, a little up to the zenith.
float3 dir = normalize(P - Cam);
float h = saturate(dir.z);
float g = pow(1.0 - h, 4.0) * 0.8 + 0.2 * pow(1.0 - h, 1.5);
return Tint * Strength * g;
"""

RAINFX = """
// Mode 0: a raindrop's hit on the ground, a flat card (the hit, a ring spreading and thinning, specks thrown off), a
// fraction of the instances by the rain, each at its own rate; Mode 1: a drip line under an edge (world Z: its top and
// length per instance), a bead swelling at the edge and letting go, falling at g, stretched as it falls. R: the
// instance's random. Out: x the mask, y the roughness.
float m = 0.0;
if (Mode < 0.5)
{
    float density = Rain > 0.01 ? 0.08 + 0.92 * Rain : 0.0;
    if (frac(R * 7.31) > density) { return float3(0.0, 0.05, 0.0); }
    float life = frac(T * (1.5 + 1.5 * frac(R * 3.7)) + R * 13.7);
    float a = life / 0.28;
    if (a > 1.0) { return float3(0.0, 0.05, 0.0); }
    float2 q = (UV - 0.5) * 2.0;
    float r = length(q);
    float ring = saturate(1.0 - abs(r - 0.15 - a * 0.8) / (0.05 * (1.0 - a) + 0.02)) * (1.0 - a);
    float hit = saturate(1.0 - r / (0.3 * (1.0 - a) + 0.01)) * step(a, 0.3);
    float ang = atan2(q.y, q.x) / 6.2831853 + 0.5;
    float seg = frac(ang * 7.0 + R * 5.0);
    float speck = saturate(1.0 - abs(seg - 0.5) / 0.12) * saturate(1.0 - abs(r - a) / 0.08) * (1.0 - a) * step(0.15, a);
    m = max(max(ring, hit), speck);
}
else
{
    float wet = saturate(Rain * 1.6 + saturate(Wetness - 0.4));
    if (frac(R * 5.17) > wet) { return float3(0.0, 0.05, 0.0); }
    float len = max(Length, 1.0);
    float y = saturate((Top - P.z) / len);
    float tf = sqrt(2.0 * len / 980.0);
    float period = tf + 0.15 + 1.2 * frac(R * 9.13) / max(0.3, wet);
    float tt = frac(T / period + R * 17.0) * period;
    float hang = period - tf;
    float fallen = tt - hang;
    float dropY = fallen <= 0.0 ? 0.0 : saturate(0.5 * 980.0 * fallen * fallen / len);
    float stretch = 0.012 + 0.06 * dropY;
    m = saturate(1.0 - abs(y - dropY) / stretch) * step(0.0, fallen) * step(dropY, 0.995);
    m = max(m, saturate(1.0 - y / 0.015) * saturate(tt / max(hang, 0.01)));
}
return float3(m, 0.05, 0.0);
"""

DECAL = """
// One cell of the 4 x 4 decal atlas (row-major from the top left), its alpha the decal's opacity.
float c = floor(Cell + 0.5);
return float2((fmod(c, 4.0) + frac(UV.x)) / 4.0, (floor(c / 4.0) + frac(UV.y)) / 4.0);
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
    """A texture parameter sampled at uv: the NS texture when it's imported, else the engine's default for its kind."""
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


def _tex_object(mat, param, texture, x, y, default=None):
    """A texture object parameter (sampled inside a Custom node), the NS texture or a default."""
    e = ss._expr(mat, unreal.MaterialExpressionTextureObjectParameter, x, y)
    e.set_editor_property("parameter_name", param)
    asset = eal.load_asset(_tex_path(texture)) if _has_tex(texture) else unreal.load_object(None, default or BLACK)
    if asset is not None:
        e.set_editor_property("texture", asset)
    return e


def _const(mat, value, x, y):
    e = ss._expr(mat, unreal.MaterialExpressionConstant, x, y)
    e.set_editor_property("r", value)
    return e


def _prop(src, prop, pin=""):
    mel.connect_material_property(src, pin, prop)


def _on_nanite(mat):
    """A material the street's Nanite meshes draw with: its Nanite shaders compiled now (unflagged, the editor flags it on
    first use and dirties it, and a cooked game draws the default material instead)."""
    _set(mat, "used_with_nanite", True)


_MPC = None


def _mpc():
    """MPC_NS, made if it's missing (or missing a parameter: rebuilt, so every material reading it recompiles)."""
    global _MPC
    if _MPC is not None:
        return _MPC
    mpc = eal.load_asset(MPC_PATH) if eal.does_asset_exist(MPC_PATH) else None
    have = set()
    if mpc is not None:
        try:
            have = {str(p.get_editor_property("parameter_name")) for p in mpc.get_editor_property("scalar_parameters")}
        except Exception:
            have = set()
    want = [n for n, _ in MPC_SCALARS]
    if mpc is None:
        tools = unreal.AssetToolsHelpers.get_asset_tools()
        mpc = tools.create_asset("MPC_NS", ss.MAT_DIR, unreal.MaterialParameterCollection, unreal.MaterialParameterCollectionFactoryNew())
    if mpc is not None and any(n not in have for n in want):
        params = list(mpc.get_editor_property("scalar_parameters")) if have else []
        for name, value in MPC_SCALARS:
            if name in have:
                continue
            p = unreal.CollectionScalarParameter()
            p.set_editor_property("parameter_name", name)
            p.set_editor_property("default_value", value)
            params.append(p)
        mpc.set_editor_property("scalar_parameters", params)
        eal.save_loaded_asset(mpc)
        unreal.log(f"ShortStack: MPC_NS holds {len(params)} parameters")
    _MPC = mpc
    return mpc


def _p(mat, name, x, y):
    """One of MPC_NS's parameters in a material."""
    e = ss._expr(mat, unreal.MaterialExpressionCollectionParameter, x, y)
    e.set_editor_property("collection", _mpc())
    e.set_editor_property("parameter_name", name)
    return e


def _instance(name, parent, scalars=None, textures=None, vectors=None):
    """MI_NS_<...> under Materials/NS: the parent, its scalars, textures (by NS name; a default when it isn't imported)
    and vectors. Made again each time the parent is built (cheap), so a texture set that arrives later is picked up."""
    path = f"{MI_DIR}/{name}"
    if eal.does_asset_exist(path):
        mi = eal.load_asset(path)
    else:
        tools = unreal.AssetToolsHelpers.get_asset_tools()
        mi = tools.create_asset(name, MI_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    if mi is None:
        unreal.log_error(f"ShortStack: couldn't make {path}")
        return None
    mel.set_material_instance_parent(mi, parent)
    for key, value in (scalars or {}).items():
        mel.set_material_instance_scalar_parameter_value(mi, key, float(value))
    for key, value in (vectors or {}).items():
        mel.set_material_instance_vector_parameter_value(mi, key, unreal.LinearColor(*value))
    for key, (texture, path_or_none) in (textures or {}).items():
        asset = None
        if path_or_none:
            asset = eal.load_asset(path_or_none) if eal.does_asset_exist(path_or_none.split(".")[0]) else None
        elif _has_tex(texture):
            asset = eal.load_asset(_tex_path(texture))
        if asset is not None:
            mel.set_material_instance_texture_parameter_value(mi, key, asset)
    mel.update_material_instance(mi)
    eal.save_loaded_asset(mi)
    return mi


# ---------------------------------------------------------------- materials

def build_ground(force):
    sets = []
    for _, _, _, a, b, _ in GROUND_SLOTS:
        sets += [f"{a}_BaseColor", f"{a}_Normal", f"{a}_ORM"]
        if b:
            sets += [f"{b}_BaseColor", f"{b}_Normal", f"{b}_ORM"]
    sets.append("T_NS_Thermo_Wear")
    stamp = _stamp(sets)
    mat = _material("M_NS_Ground", force, stamp)
    if mat:
        _on_nanite(mat)
        uv0 = ss._expr(mat, unreal.MaterialExpressionTextureCoordinate, -1900, -300)
        tile = ss._scalar(mat, "Tile", 3.0, -1900, -200)
        uv = _custom(mat, "return UV / max(Tile, 0.01);", [("UV", uv0), ("Tile", tile)], ss.F2, -1650, -250, "Tile UVs")
        base_a = _tex(mat, "BaseA", "T_NS_Sidewalk_A_BaseColor", COLOR, -1350, -700, uv)
        nrm_a = _tex(mat, "NormalA", "T_NS_Sidewalk_A_Normal", NORMAL, -1350, -500, uv)
        orm_a = _tex(mat, "OrmA", "T_NS_Sidewalk_A_ORM", MASKS, -1350, -300, uv)
        hgt_a = _tex(mat, "HeightA", "T_NS_Sidewalk_A_Height", MASKS, -1350, -100, uv)
        base_b = _tex(mat, "BaseB", "T_NS_Sidewalk_B_BaseColor", COLOR, -1350, 100, uv)
        nrm_b = _tex(mat, "NormalB", "T_NS_Sidewalk_B_Normal", NORMAL, -1350, 300, uv)
        orm_b = _tex(mat, "OrmB", "T_NS_Sidewalk_B_ORM", MASKS, -1350, 500, uv)
        wear = _tex(mat, "WearTex", "T_NS_Thermo_Wear", MASKS, -1350, 700, uv)
        wp = ss._world_pos(mat, -1900, 0)
        n = ss._expr(mat, unreal.MaterialExpressionVertexNormalWS, -1900, 80)
        vc = ss._expr(mat, unreal.MaterialExpressionVertexColor, -1900, 160)
        kind = ss._scalar(mat, "Kind", 0.0, -1900, 240)
        t = ss._time(mat, -1900, 320)
        common = [("P", wp), ("N", n), ("VC", vc), ("Kind", kind), ("Seed", _p(mat, "Seed", -1900, 400)), ("Wetness", _p(mat, "Wetness", -1900, 480)),
                  ("Puddle", _p(mat, "Puddle", -1900, 560)), ("HeightA", (hgt_a, "R"))]
        c = _custom(mat, PUDDLE + GROUND, common + [("BaseA", (base_a, "RGB")), ("BaseB", (base_b, "RGB")), ("OrmA", (orm_a, "RGB")), ("OrmB", (orm_b, "RGB")),
                                                    ("VCA", (vc, "A")), ("Wear", (wear, "R"))], ss.F4, -800, -300, "Northside's ground")
        _prop(ss._mask(mat, c, "rgb", -500, -350), unreal.MaterialProperty.MP_BASE_COLOR)
        _prop(ss._mask(mat, c, "a", -500, -250), unreal.MaterialProperty.MP_ROUGHNESS)
        nn = _custom(mat, PUDDLE + GROUND_NORMAL, common + [("NrmA", (nrm_a, "RGB")), ("NrmB", (nrm_b, "RGB")), ("Rain", _p(mat, "Rain", -1900, 640)), ("T", t)],
                     ss.F3, -800, 200, "The ground's normal: puddles and rings")
        _prop(nn, unreal.MaterialProperty.MP_NORMAL)
        _prop(orm_a, unreal.MaterialProperty.MP_AMBIENT_OCCLUSION, "R")
        _finish(mat, stamp)
    parent = eal.load_asset(f"{ss.MAT_DIR}/M_NS_Ground")
    made = 1 if mat else 0
    if parent is not None and (mat or force or not eal.does_asset_exist(f"{MI_DIR}/MI_NS_Ground_Sidewalk")):
        for slot, short, kind, a, b, tile in GROUND_SLOTS:
            tex = {"BaseA": (f"{a}_BaseColor", None), "NormalA": (f"{a}_Normal", None), "OrmA": (f"{a}_ORM", None), "HeightA": (f"{a}_Height", None),
                   "BaseB": (f"{b or a}_BaseColor", None), "NormalB": (f"{b or a}_Normal", None), "OrmB": (f"{b or a}_ORM", None)}
            _instance(f"MI_NS_Ground_{short}", parent, {"Kind": kind, "Tile": tile}, tex)
    return made


def build_masonry(force):
    sets = []
    for _, _, _, a in MASONRY_SLOTS:
        sets += [f"{a}_BaseColor", f"{a}_Normal", f"{a}_ORM"]
    sets.append("T_NS_Brick_Painted_Mask")
    stamp = _stamp(sets)
    mat = _material("M_NS_Masonry", force, stamp)
    if mat:
        _on_nanite(mat)
        uv0 = ss._expr(mat, unreal.MaterialExpressionTextureCoordinate, -1900, -300)
        tile = ss._scalar(mat, "Tile", 2.0, -1900, -200)
        uv = _custom(mat, "return UV / max(Tile, 0.01);", [("UV", uv0), ("Tile", tile)], ss.F2, -1650, -250, "Tile UVs")
        base_a = _tex(mat, "BaseA", "T_NS_Brick_Red_BaseColor", COLOR, -1350, -600, uv)
        nrm_a = _tex(mat, "NormalA", "T_NS_Brick_Red_Normal", NORMAL, -1350, -400, uv)
        orm_a = _tex(mat, "OrmA", "T_NS_Brick_Red_ORM", MASKS, -1350, -200, uv)
        base_b = _tex(mat, "BaseB", "T_NS_Brick_Red_BaseColor", COLOR, -1350, 0, uv)
        orm_b = _tex(mat, "OrmB", "T_NS_Brick_Red_ORM", MASKS, -1350, 200, uv)
        mask = _tex(mat, "PaintMask", "T_NS_Brick_Painted_Mask", MASKS, -1350, 400, uv, WHITE)
        wp = ss._world_pos(mat, -1900, 0)
        op = ss._expr(mat, unreal.MaterialExpressionObjectPositionWS, -1900, 80)
        z = _custom(mat, "return P.z - O.z;", [("P", wp), ("O", op)], ss.F1, -1650, 60, "Height over the sidewalk")
        vc = ss._expr(mat, unreal.MaterialExpressionVertexColor, -1900, 160)
        c = _custom(mat, MASONRY, [("P", wp), ("Z", z), ("VC", vc), ("Kind", ss._scalar(mat, "Kind", 0.0, -1900, 240)), ("BaseA", (base_a, "RGB")),
                                   ("BaseB", (base_b, "RGB")), ("OrmA", (orm_a, "RGB")), ("OrmB", (orm_b, "RGB")), ("Mask", (mask, "R")),
                                   ("Wetness", _p(mat, "Wetness", -1900, 320)), ("Rain", _p(mat, "Rain", -1900, 400))], ss.F4, -800, -200, "Northside's masonry")
        _prop(ss._mask(mat, c, "rgb", -500, -250), unreal.MaterialProperty.MP_BASE_COLOR)
        _prop(ss._mask(mat, c, "a", -500, -150), unreal.MaterialProperty.MP_ROUGHNESS)
        _prop(nrm_a, unreal.MaterialProperty.MP_NORMAL, "RGB")
        _prop(orm_a, unreal.MaterialProperty.MP_AMBIENT_OCCLUSION, "R")
        _finish(mat, stamp)
    parent = eal.load_asset(f"{ss.MAT_DIR}/M_NS_Masonry")
    if parent is not None and (mat or force or not eal.does_asset_exist(f"{MI_DIR}/MI_NS_Masonry_BrickRed")):
        for slot, short, kind, a in MASONRY_SLOTS:
            tex = {"BaseA": (f"{a}_BaseColor", None), "NormalA": (f"{a}_Normal", None), "OrmA": (f"{a}_ORM", None),
                   "BaseB": ("T_NS_Brick_Red_BaseColor", None), "OrmB": ("T_NS_Brick_Red_ORM", None)}
            if kind == 1:
                tex["PaintMask"] = ("T_NS_Brick_Painted_Mask", None)
            _instance(f"MI_NS_Masonry_{short}", parent, {"Kind": kind, "Tile": 2.0}, tex)
    return 1 if mat else 0


def build_window(force):
    sets = ("T_NS_Rooms_Albedo", "T_NS_Rooms_Lamp", "T_NS_RoomFront")
    stamp = _stamp(sets)
    mat = _material("M_NS_WindowRoom", force, stamp)
    if not mat:
        return 0
    _on_nanite(mat)
    uv0 = ss._expr(mat, unreal.MaterialExpressionTextureCoordinate, -1700, -400)
    vc = ss._expr(mat, unreal.MaterialExpressionVertexColor, -1700, -300)
    cam = ss._expr(mat, unreal.MaterialExpressionCameraVectorWS, -1900, -200)
    view = ss._expr(mat, unreal.MaterialExpressionTransform, -1700, -200)
    try:
        view.set_editor_property("transform_source_type", unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_WORLD)
        view.set_editor_property("transform_type", unreal.MaterialVectorCoordTransform.TRANSFORM_TANGENT)
    except Exception as exc:
        unreal.log_warning(f"ShortStack: M_NS_WindowRoom's view transform: {exc}")
    mel.connect_material_expressions(cam, "", view, "Input")
    rooms = _tex_object(mat, "Rooms", sets[0], -1700, 0)
    lamps = _tex_object(mat, "Lamps", sets[1], -1700, 100)
    fronts = _tex_object(mat, "Fronts", sets[2], -1700, 200)
    dist = _custom(mat, "return length(C - P);", [("C", ss._expr(mat, unreal.MaterialExpressionCameraPositionWS, -1900, 860)), ("P", ss._world_pos(mat, -1900, 940))],
                   ss.F1, -1700, 860, "How far the window is")
    c = _custom(mat, WINDOW, [("UV", uv0), ("Dist", dist), ("VC", vc), ("V", view), ("Rooms", rooms), ("Lamps", lamps), ("Fronts", fronts),
                              ("Hour", _p(mat, "Hour", -1700, 300)), ("Weekday", _p(mat, "Weekday", -1700, 380)), ("Daylight", _p(mat, "Daylight", -1700, 460)),
                              ("Seed", _p(mat, "Seed", -1700, 540)), ("T", ss._time(mat, -1700, 620)), ("DayNits", ss._scalar(mat, "DayNits", 45.0, -1700, 700)),
                              ("LampNits", ss._scalar(mat, "LampNits", 2.2, -1700, 780))], ss.F4, -900, -100, "A room behind the window")
    _prop(ss._mask(mat, c, "rgb", -500, -150), unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    _prop(ss._mask(mat, c, "a", -500, -50), unreal.MaterialProperty.MP_ROUGHNESS)
    _prop(ss._vector(mat, "Glass", (0.012, 0.014, 0.018), -500, 100), unreal.MaterialProperty.MP_BASE_COLOR)
    _prop(_const(mat, 0.5, -500, 200), unreal.MaterialProperty.MP_SPECULAR)
    return 1 if _finish(mat, stamp) else 0


def build_glass(force):
    stamp = _stamp()
    mat = _material("M_NS_Glass", force, stamp)
    if not mat:
        return 0
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("two_sided", True)
    mode = getattr(getattr(unreal, "TranslucencyLightingMode", None), "TLM_SURFACE_PER_PIXEL_LIGHTING", None)
    if mode is not None:
        _set(mat, "translucency_lighting_mode", mode)
    wp = ss._world_pos(mat, -900, -400)
    n = ss._expr(mat, unreal.MaterialExpressionVertexNormalWS, -900, -320)
    cam = ss._expr(mat, unreal.MaterialExpressionCameraPositionWS, -900, -240)
    pivot = ss._expr(mat, unreal.MaterialExpressionObjectPositionWS, -900, -160)
    side = ss._expr(mat, unreal.MaterialExpressionTwoSidedSign, -900, -80)
    c = _custom(mat, GLASS, [("P", wp), ("N", n), ("Cam", cam), ("ObjPos", pivot), ("Side", side), ("T", ss._time(mat, -900, 0)),
                             ("Rain", _p(mat, "Rain", -900, 80)), ("Daylight", _p(mat, "Daylight", -900, 160)), ("Outdoor", ss._scalar(mat, "Outdoor", 1.0, -900, 240))],
                ss.F2, -450, -200, "Shop glass in the rain")
    _prop(ss._mask(mat, c, "r", -200, -250), unreal.MaterialProperty.MP_OPACITY)
    _prop(ss._mask(mat, c, "g", -200, -150), unreal.MaterialProperty.MP_ROUGHNESS)
    _prop(ss._vector(mat, "Tint", (0.02, 0.025, 0.03), -450, 100), unreal.MaterialProperty.MP_BASE_COLOR)
    _prop(_const(mat, 0.5, -450, 200), unreal.MaterialProperty.MP_SPECULAR)
    return 1 if _finish(mat, stamp) else 0


def _glow_graph(mat, base):
    """M_NS_Glow's graph: the hour's glow as emissive (GLOW), and with base, the unlit glass's color (GLOW_BASE)."""
    uv = ss._expr(mat, unreal.MaterialExpressionTextureCoordinate, -900, -400)
    mode = ss._scalar(mat, "Mode", 0.0, -900, -240)
    color = ss._vector(mat, "Color", (1.0, 0.75, 0.45), -900, -160)
    lit_a = ss._vector(mat, "LitA", (0.0, 0.0, 0.0), -1100, 0)
    lit_b = ss._vector(mat, "LitB", (0.0, 0.0, 0.0), -1100, 80)
    c = _custom(mat, GLOW, [("UV", uv), ("T", ss._time(mat, -900, -320)), ("Mode", mode), ("Color", color),
                            ("Level", ss._scalar(mat, "Level", 20.0, -900, -80)), ("LitA", lit_a), ("LitAw", (lit_a, "A")),
                            ("LitB", lit_b), ("LitBw", (lit_b, "A")), ("Count", ss._scalar(mat, "Count", 0.0, -900, 160)),
                            ("Groups", ss._scalar(mat, "Groups", 6.0, -900, 240)), ("ChaseRate", ss._scalar(mat, "ChaseRate", 0.0, -900, 320)),
                            ("Flicker", ss._scalar(mat, "Flicker", 0.0, -900, 400))], ss.F3, -450, 0, "A glow by the hour")
    # LitA and LitB carry four cells each: RGB the first three, alpha (its own pin) the fourth.
    _prop(c, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if base:
        b = _custom(mat, GLOW_BASE, [("UV", uv), ("Mode", mode), ("Color", color)], ss.F3, -450, 300, "The glass unlit")
        _prop(b, unreal.MaterialProperty.MP_BASE_COLOR)
        _prop(ss._scalar(mat, "Rough", 0.22, -450, 420), unreal.MaterialProperty.MP_ROUGHNESS)
        _prop(_const(mat, 0.5, -450, 500), unreal.MaterialProperty.MP_SPECULAR)


def build_glow(force):
    """M_NS_Glow: the kit's glow meshes (tubes, bulbs, floods, the signals' lenses): lit glass by day, their light by the
    hour as emissive. M_NS_GlowAdd: the same light added over what's under it (unlit, additive) for the cars' lamp films,
    which lie over the baked lenses and glass (ns_cars.py): an off cell adds nothing."""
    built = 0
    stamp = _stamp()
    mat = _material("M_NS_Glow", force, stamp)
    if mat:
        _on_nanite(mat)
        mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
        _glow_graph(mat, True)
        built += 1 if _finish(mat, stamp) else 0
    add = _material("M_NS_GlowAdd", force, stamp)
    if add:
        add.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
        add.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
        _set(add, "use_translucency_vertex_fog", True)
        _glow_graph(add, False)
        built += 1 if _finish(add, stamp) else 0
    return built


def build_screen(force):
    """M_NS_Screen: a [screen] slot showing core's picture (FScreens draws it into a render target: Image), its clear parts the
    slot's own color (Back), wet in the rain (MPC_NS.Wetness x Exposed), shining by itself when it's backlit (Glow, the
    hour's nits)."""
    stamp = _stamp()
    mat = _material("M_NS_Screen", force, stamp)
    if not mat:
        return 0
    _on_nanite(mat)
    img = _tex(mat, "Image", None, COLOR, -900, -300, ss._expr(mat, unreal.MaterialExpressionTextureCoordinate, -1150, -300), BLACK)
    back = ss._vector(mat, "Back", (0.5, 0.5, 0.5), -900, -80)
    c = _custom(mat, SCREEN, [("Img", (img, "RGBA")), ("Back", back), ("Rough", ss._scalar(mat, "Rough", 0.45, -900, 0)),
                              ("Exposed", ss._scalar(mat, "Exposed", 1.0, -900, 80)), ("Wetness", _p(mat, "Wetness", -900, 160))],
                ss.F4, -450, -200, "Core's picture on the face")
    _prop(ss._mask(mat, c, "rgb", -200, -250), unreal.MaterialProperty.MP_BASE_COLOR)
    _prop(ss._mask(mat, c, "a", -200, -150), unreal.MaterialProperty.MP_ROUGHNESS)
    glow = ss._scalar(mat, "Glow", 0.0, -900, 260)
    e = _custom(mat, "return Img.rgb * saturate(Img.a) * Glow;", [("Img", (img, "RGBA")), ("Glow", glow)], ss.F3, -450, 150, "Backlit")
    _prop(e, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    _prop(_const(mat, 0.5, -200, 300), unreal.MaterialProperty.MP_SPECULAR)
    return 1 if _finish(mat, stamp) else 0


def _sway(mat, x, y):
    """The trees' world position offset (MPC_NS's wind), Enabled 1 (the stage's distance disables it past ~40 m)."""
    return _custom(mat, SWAY, [("P", ss._world_pos(mat, x - 300, y)), ("ObjPos", ss._expr(mat, unreal.MaterialExpressionObjectPositionWS, x - 300, y + 80)),
                               ("VC", ss._expr(mat, unreal.MaterialExpressionVertexColor, x - 300, y + 160)), ("T", ss._time(mat, x - 300, y + 240)),
                               ("WindX", _p(mat, "WindX", x - 300, y + 320)), ("WindY", _p(mat, "WindY", x - 300, y + 400)),
                               ("Enabled", ss._scalar(mat, "Sway", 1.0, x - 300, y + 480))], ss.F3, x, y, "Swaying in the wind")


def build_leaf(force):
    sets = ("T_NS_Leaf_BaseColor", "T_NS_Leaf_Normal", "T_NS_Leaf_ORM")
    stamp = _stamp(sets)
    mat = _material("M_NS_Leaf", force, stamp)
    if mat:
        _on_nanite(mat)
        mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
        mat.set_editor_property("two_sided", True)
        mat.set_editor_property("opacity_mask_clip_value", 0.4)
        mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
        uv = ss._expr(mat, unreal.MaterialExpressionTextureCoordinate, -1300, 0)
        base = _tex(mat, "LeafBase", sets[0], COLOR, -1000, -200, uv)
        nrm = _tex(mat, "LeafNormal", sets[1], NORMAL, -1000, 0, uv)
        orm = _tex(mat, "LeafOrm", sets[2], MASKS, -1000, 200, uv)
        _prop(base, unreal.MaterialProperty.MP_BASE_COLOR, "RGB")
        _prop(base, unreal.MaterialProperty.MP_OPACITY_MASK, "A")
        _prop(nrm, unreal.MaterialProperty.MP_NORMAL, "RGB")
        _prop(orm, unreal.MaterialProperty.MP_ROUGHNESS, "G")
        _prop(orm, unreal.MaterialProperty.MP_AMBIENT_OCCLUSION, "R")
        sss = _custom(mat, "return Base * float3(0.55, 0.5, 0.25);", [("Base", (base, "RGB"))], ss.F3, -600, 300, "Light through the leaves")
        _prop(sss, unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
        _prop(_sway(mat, -600, 500), unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
        _finish(mat, stamp)
    parent = eal.load_asset(f"{ss.MAT_DIR}/M_NS_Leaf")
    if parent is not None and (mat or force or not eal.does_asset_exist(f"{MI_DIR}/MI_NS_Leaf")):
        _instance("MI_NS_Leaf", parent, {}, {"LeafBase": (sets[0], None), "LeafNormal": (sets[1], None), "LeafOrm": (sets[2], None)})
    made = 1 if mat else 0
    # The bark, baked per tree (the tree's own textures), swaying with its leaves.
    stamp_b = _stamp()
    bark = _material("M_NS_Bark", force, stamp_b)
    if bark:
        _on_nanite(bark)
        uv = ss._expr(bark, unreal.MaterialExpressionTextureCoordinate, -1300, 0)
        base = _tex(bark, "BarkBase", None, COLOR, -1000, -200, uv)
        nrm = _tex(bark, "BarkNormal", None, NORMAL, -1000, 0, uv)
        orm = _tex(bark, "BarkOrm", None, MASKS, -1000, 200, uv)
        _prop(base, unreal.MaterialProperty.MP_BASE_COLOR, "RGB")
        _prop(nrm, unreal.MaterialProperty.MP_NORMAL, "RGB")
        _prop(orm, unreal.MaterialProperty.MP_ROUGHNESS, "G")
        _prop(orm, unreal.MaterialProperty.MP_AMBIENT_OCCLUSION, "R")
        _prop(_sway(bark, -600, 400), unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
        _finish(bark, stamp_b)
        made += 1
    bark_parent = eal.load_asset(f"{ss.MAT_DIR}/M_NS_Bark")
    if bark_parent is not None:
        for tree in TREES:
            folder = f"{ss.MESH_DIR}/{tree}/{tree}/Textures"
            if not eal.does_directory_exist(folder):
                continue
            name = f"MI_NS_Bark_{tree.replace('SM_NS_', '')}"
            if not (bark or force) and eal.does_asset_exist(f"{MI_DIR}/{name}"):
                continue
            tex = {"BarkBase": (None, f"{folder}/T_{tree}_BaseColor"), "BarkNormal": (None, f"{folder}/T_{tree}_Normal"),
                   "BarkOrm": (None, f"{folder}/T_{tree}_ORM")}
            _instance(name, bark_parent, {}, tex)
    return made


def build_sky_glow(force):
    stamp = _stamp()
    mat = _material("M_NS_SkyGlow", force, stamp)
    if not mat:
        return 0
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    mat.set_editor_property("two_sided", True)
    _set(mat, "use_translucency_vertex_fog", False)
    c = _custom(mat, SKYGLOW, [("P", ss._world_pos(mat, -900, -200)), ("Cam", ss._expr(mat, unreal.MaterialExpressionCameraPositionWS, -900, -100)),
                               ("Tint", ss._vector(mat, "Tint", (1.0, 0.61, 0.32), -900, 0)), ("Strength", ss._scalar(mat, "Strength", 0.0, -900, 100))],
                ss.F3, -450, 0, "The city's glow on the cloud")
    _prop(c, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    return 1 if _finish(mat, stamp) else 0


def build_decal(force):
    sets = ("T_NS_Decals_BaseColor", "T_NS_Decals_Normal", "T_NS_Decals_ORM")
    stamp = _stamp(sets)
    mat = _material("M_NS_Decal", force, stamp)
    if mat:
        _set(mat, "material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL)
        mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
        dbuffer = getattr(getattr(unreal, "DecalBlendMode", None), "DBM_D_BUFFER_COLOR_NORMAL_ROUGHNESS", None)
        if dbuffer is not None:
            _set(mat, "decal_blend_mode", dbuffer)
        uv0 = ss._expr(mat, unreal.MaterialExpressionTextureCoordinate, -1300, -200)
        uv = _custom(mat, DECAL, [("UV", uv0), ("Cell", ss._scalar(mat, "Cell", 0.0, -1300, -100))], ss.F2, -1050, -150, "The atlas's cell")
        base = _tex(mat, "DecalBase", sets[0], COLOR, -800, -300, uv)
        nrm = _tex(mat, "DecalNormal", sets[1], NORMAL, -800, -100, uv)
        orm = _tex(mat, "DecalOrm", sets[2], MASKS, -800, 100, uv)
        wet = _custom(mat, "return float4(Base * lerp(1.0, 0.65, Wetness), saturate(lerp(Orm.g, Orm.g * 0.4, Wetness)));",
                      [("Base", (base, "RGB")), ("Orm", (orm, "RGB")), ("Wetness", _p(mat, "Wetness", -800, 300))], ss.F4, -450, -200, "Wet decal")
        _prop(ss._mask(mat, wet, "rgb", -200, -250), unreal.MaterialProperty.MP_BASE_COLOR)
        _prop(ss._mask(mat, wet, "a", -200, -150), unreal.MaterialProperty.MP_ROUGHNESS)
        _prop(nrm, unreal.MaterialProperty.MP_NORMAL, "RGB")
        _prop(base, unreal.MaterialProperty.MP_OPACITY, "A")
        _finish(mat, stamp)
    parent = eal.load_asset(f"{ss.MAT_DIR}/M_NS_Decal")
    if parent is not None and (mat or force or not eal.does_asset_exist(f"{MI_DIR}/MI_NS_Decal_00")):
        for cell in range(16):
            _instance(f"MI_NS_Decal_{cell:02d}", parent, {"Cell": cell})
    return 1 if mat else 0


def build_rain_fx(force):
    """M_NS_RainFx: the splashes round the camera and the drip lines (AStreetStage::BuildRainFx, AMarketStage's drips)."""
    stamp = _stamp()
    mat = _material("M_NS_RainFx", force, stamp)
    if not mat:
        return 0
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    mat.set_editor_property("two_sided", True)
    _set(mat, "used_with_instanced_static_meshes", True)
    _set(mat, "opacity_mask_clip_value", 0.4)
    top = ss._expr(mat, unreal.MaterialExpressionPerInstanceCustomData, -900, 240)
    _set(top, "data_index", 0)
    length = ss._expr(mat, unreal.MaterialExpressionPerInstanceCustomData, -900, 320)
    _set(length, "data_index", 1)
    _set(length, "const_default_value", 100.0)
    c = _custom(mat, RAINFX, [("UV", ss._expr(mat, unreal.MaterialExpressionTextureCoordinate, -900, -400)), ("T", ss._time(mat, -900, -320)),
                              ("R", ss._expr(mat, unreal.MaterialExpressionPerInstanceRandom, -900, -240)), ("Mode", ss._scalar(mat, "Mode", 0.0, -900, -160)),
                              ("P", ss._world_pos(mat, -900, -80)), ("Rain", _p(mat, "Rain", -900, 0)), ("Wetness", _p(mat, "Wetness", -900, 80)),
                              ("Top", top), ("Length", length)],
                ss.F3, -450, 0, "The rain's hits and drips")
    _prop(ss._mask(mat, c, "r", -200, -100), unreal.MaterialProperty.MP_OPACITY_MASK)
    _prop(ss._mask(mat, c, "g", -200, 0), unreal.MaterialProperty.MP_ROUGHNESS)
    _prop(ss._vector(mat, "Water", (0.5, 0.54, 0.58), -450, 150), unreal.MaterialProperty.MP_BASE_COLOR)
    _prop(_const(mat, 0.6, -450, 250), unreal.MaterialProperty.MP_SPECULAR)
    return 1 if _finish(mat, stamp) else 0


def build_materials(force=False):
    """Northside's materials that are missing or out of date (all of them with force). Returns how many were built."""
    if not eal.does_directory_exist(MI_DIR):
        eal.make_directory(MI_DIR)
    built = 0
    _mpc()
    for build in (build_ground, build_masonry, build_window, build_glass, build_glow, build_screen, build_leaf, build_sky_glow, build_decal, build_rain_fx):
        try:
            built += build(force)
        except Exception as exc:  # the street keeps the imported materials for each
            unreal.log_error(f"ShortStack: {build.__name__} failed: {exc}")
    return built


# ---------------------------------------------------------------- the imports' fix-ups

def fix_textures():
    """The NS textures' formats: normal maps as normal maps with Blender's OpenGL green flipped, masks (ORM, heights, the
    paint's mask, the wear pattern) linear. shortstack_setup.import_textures brings every group but Clothing in as color."""
    if not eal.does_directory_exist(TEX_DIR):
        return 0
    fixed = 0
    for asset_path in eal.list_assets(TEX_DIR, recursive=True, include_folder=False):
        tex = eal.load_asset(asset_path)
        if not isinstance(tex, unreal.Texture2D):
            continue
        name = tex.get_name()
        flip = False
        if name.endswith("_Normal"):
            want = (False, unreal.TextureCompressionSettings.TC_NORMALMAP)
            flip = True
        elif name.endswith(("_ORM", "_Mask", "_Height", "_Wear")):
            want = (False, unreal.TextureCompressionSettings.TC_MASKS)
        else:
            continue
        current_flip = False
        try:
            current_flip = bool(tex.get_editor_property("flip_green_channel"))
        except Exception:
            current_flip = flip
        if tex.get_editor_property("srgb") == want[0] and tex.get_editor_property("compression_settings") == want[1] and current_flip == flip:
            continue
        tex.set_editor_property("srgb", want[0])
        tex.set_editor_property("compression_settings", want[1])
        if flip:
            _set(tex, "flip_green_channel", True)
        eal.save_loaded_asset(tex)
        fixed += 1
    if fixed:
        unreal.log(f"ShortStack: {fixed} Northside textures set to their formats")
    return fixed


def _runtime_material(slot_name):
    """The runtime material for an unbaked slot, by its Blender material's name (None: keep the imported one)."""
    s = slot_name.lower()
    for slot, short, _, _, _, _ in GROUND_SLOTS:
        if s == slot:
            return f"{MI_DIR}/MI_NS_Ground_{short}"
    for slot, short, _, _ in MASONRY_SLOTS:
        if s == slot:
            return f"{MI_DIR}/MI_NS_Masonry_{short}"
    if s == "ns_window":
        return f"{ss.MAT_DIR}/M_NS_WindowRoom"
    if s == "ns_glass":
        return f"{ss.MAT_DIR}/M_NS_Glass"
    if s == "ns_leaf":
        return f"{MI_DIR}/MI_NS_Leaf"
    return None


def _slot_names(mesh):
    """Each slot's name (the importer's slot name, or its material's)."""
    out = []
    for slot in mesh.get_editor_property("static_materials"):
        name = str(slot.get_editor_property("material_slot_name"))
        mi = slot.get_editor_property("material_interface")
        if (not name or name == "None") and mi is not None:
            name = mi.get_name()
        out.append(name)
    return out


def _wants_no_nanite(name, slots):
    # The hand props (small, skinned to hands), and the cars' lamp films (drawn additive over the baked lenses: Nanite draws
    # no translucency).
    if name.startswith("SM_NS_Prop_") or name.endswith("_GlowLamps"):
        return True
    for s in slots:
        low = s.lower()
        if low == "ns_glass" or low.startswith("ns_screen") or low == "ns_plate" or low == "ns_steam":
            return True
    return False


def fix_mesh(name, mesh, force=False):
    """The rules for one SM_NS_ mesh: its runtime materials, Nanite, collision, the trees' settings. True if it changed."""
    if not isinstance(mesh, unreal.StaticMesh):
        return False
    tag = FIX_VERSION + ":" + (eal.get_metadata_tag(mesh, "SourceHash") or "") + ":" + VERSION
    if not force and eal.get_metadata_tag(mesh, FIX_TAG) == tag:
        return False
    sms = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    slots = _slot_names(mesh)
    # The runtime materials on the asset, slot by slot (by name: the glb carries only the slots it uses).
    materials = list(mesh.get_editor_property("static_materials"))
    changed = False
    for i, slot in enumerate(materials):
        path = _runtime_material(slots[i])
        if name in TREES and i == 0 and slots[i].lower() != "ns_leaf":
            path = f"{MI_DIR}/MI_NS_Bark_{name.replace('SM_NS_', '')}"
        if not path or not eal.does_asset_exist(path):
            continue
        mi = eal.load_asset(path)
        if mi is not None and slot.get_editor_property("material_interface") != mi:
            slot.set_editor_property("material_interface", mi)
            materials[i] = slot
            changed = True
    if changed:
        mesh.set_editor_property("static_materials", materials)
    # Nanite: off for glass, screens, plates, steam and the hand props; on for the trees' masked foliage.
    nanite = mesh.get_editor_property("nanite_settings")
    if _wants_no_nanite(name, slots):
        ss._without_nanite(mesh)
    elif name in TREES and not nanite.enabled:
        nanite.enabled = True
        sms.set_nanite_settings(mesh, nanite, True)
    if name.startswith("SM_NS_Ground_"):
        # Its collision (and its ray tracing) are the Nanite fallback's triangles: kept at full detail, every curb.
        try:
            nanite = mesh.get_editor_property("nanite_settings")
            if nanite.enabled:
                nanite.set_editor_property("fallback_target", unreal.NaniteFallbackTarget.RELATIVE_ERROR)
                nanite.set_editor_property("fallback_relative_error", 0.0)
                sms.set_nanite_settings(mesh, nanite, True)
        except Exception as exc:
            unreal.log_warning(f"ShortStack: couldn't keep {name}'s fallback at full detail: {exc}")
    # The stages place their own collision (spec 0.5): the importer's hulls go; the ground's triangles are its own.
    try:
        sms.remove_collisions(mesh)
    except Exception as exc:
        unreal.log_warning(f"ShortStack: couldn't remove {name}'s collision: {exc}")
    body = mesh.get_editor_property("body_setup")
    if body is not None:
        flag = unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE if name.startswith("SM_NS_Ground_") else unreal.CollisionTraceFlag.CTF_USE_DEFAULT
        _set(body, "collision_trace_flag", flag)
    # The trees' leaves out of the ray-traced scene (any-hit on masked foliage costs; spec 3.8). The bark stays in.
    if name in TREES:
        stage = getattr(unreal, "StreetStage", None)
        if stage is not None and hasattr(stage, "ns_section_flags"):
            try:
                stage.ns_section_flags(mesh, "ns_leaf", False, True)
            except Exception as exc:
                unreal.log_warning(f"ShortStack: couldn't take {name}'s leaves out of ray tracing: {exc}")
    eal.set_metadata_tag(mesh, FIX_TAG, tag)
    eal.save_loaded_asset(mesh)
    unreal.log(f"ShortStack: {name} fixed up for Northside")
    return True


def fix_imports(force=False):
    """The rules on every SM_NS_ mesh imported (one imported before them, or not changed since, is fixed once)."""
    fixed = 0
    if not eal.does_directory_exist(ss.MESH_DIR):
        return 0
    for folder in eal.list_assets(ss.MESH_DIR, recursive=False, include_folder=True):
        name = folder.rstrip("/").split("/")[-1]
        if not name.startswith("SM_NS_"):
            continue
        path = f"{ss.MESH_DIR}/{name}/{name}"
        if eal.does_asset_exist(path):
            try:
                if fix_mesh(name, eal.load_asset(path), force):
                    fixed += 1
            except Exception as exc:
                unreal.log_error(f"ShortStack: fixing {name} failed: {exc}")
    return fixed


def run(force=False):
    """Northside's part of the editor's setup: the textures' formats, MPC_NS and the materials, the meshes' rules.
    Returns how many things it changed."""
    changed = fix_textures()
    changed += build_materials(force)
    changed += fix_imports(force)
    return changed
