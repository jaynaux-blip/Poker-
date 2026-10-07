"""
SHORT STACK: the block's six staff MetaHumans (spec 6.1): Ray, Lonnie, Bea, Rosa, Ruby and Marlene, each a face nobody
else wears.

The Creator ships 29 presets, and the cast, the extras and the heroes wear 28 of them (only Grace is free), so each of
the staff is made the way the Creator's Blend tool makes a new face: a weighted blend of three presets
(ShortStackMetaHumanLibrary.CreateFromBlend). The first preset also gives the body, so it is picked for the build (an
older body for the older staff); the weights alone decide how much of each face is kept. Then what reads their age and
background:
  - the skin tone, and a face texture whose scan has the wrinkles, stubble and marks of their years (SetSkinColor,
    FaceTextures): the scans are the Creator's 153 (texture_attributes.json). A woman's is one of the women presets' own
    scans where one has the wrinkles asked for (the marks may differ), else an unworn one with no stubble; a scan worn by
    one of the person's donors, or by the staff made before, is never used;
  - the eyes (one of the Creator's 12 eye presets), worn teeth and greying lashes (SetEyesPreset, SetAging);
  - hair, brows and facial hair from the installed wardrobe, in colors greyed for their age (SetWardrobe,
    SetGroomColor). Only styles with hair cards: at quality 1 a card-less one (Hair_S_BuzzCut, _CoilBuzzCut, _360Waves,
    _BaldingStubble and the stubble beards and mustaches) is only painted on, and ExtraJ's 360 waves never showed.
Every new face is measured against the 29 presets, so against every face in the cast and the heroes, and against the
staff made before it (FaceDistances): one nearer than TWIN of the typical distance between two faces of its sex to a face
someone wears is a twin, and step() stops (Grace, which nobody wears, is listed but can't make a twin). A face someone
on the street keeps (the player's six hero faces, Benny's ExtraB, Augie's ExtraH, Inez's ExtraK) gives a blend at most
DONOR_CAP, and Benny's and Augie's faces none at all.

Each is built like the Back Room's cast (backroom_cast.py): open and shape, rig, texture, save, assemble, close, one
character per fresh editor (the scratchpad's staff_loop.sh), at quality 1 ("medium": hair cards, about 316 MB each),
as /Game/ShortStack/Cast/MHC_<Name> and /Game/ShortStack/Cast/Built/MHC_<Name>/BP_MHC_<Name>, where CastLooks finds them
(CastLooks.cpp IsBuilt). Until one lands, its role borrows a reserved extra (CastLooks.h). What each was made from is
recorded on its character asset and its Blueprint (metadata ShortStackRecipe, ShortStackPreset, ShortStackQuality,
ShortStackSkin, ShortStackFaceCheck): change an entry of STAFF before it's built and step() makes it again; after, it
asks for rebuild().

    import staff_cast as s
    s.shape()           # repeat until "shaped all": every face blended, shaped and checked, nothing sent to the cloud
    s.status()          # each one's state and face check
    s.step()            # repeat until "done" (staff_loop.sh does, with a fresh editor per assembly)
    s.check("Ray")      # opens one and says how near the nearest faces are
    s.survey()          # every preset: who wears it, its skin (with any character open: its scan's wrinkles etc.)
    s.rebuild("Ray")    # throws one away (its build and its character); step() makes it again
"""
import re
import shutil
import zlib

import unreal

import backroom_cast as cast
import hero_cast

L = unreal.ShortStackMetaHumanLibrary
EAL = unreal.EditorAssetLibrary
PRESETS_DIR = "/MetaHumanCharacter/Optional/Presets"
GROOM_ASSETS = "/MetaHumanCharacter/Optional/Grooms/GroomAssets"
# The street's staff are seen whole and from a few meters (and at Near, talking): "medium" (hair cards, no strands).
QUALITY = 1
# A face nearer than this fraction of the typical distance between two faces of its sex (the median distance from it to
# the presets of its sex) is a twin. Three-way blends that keep at most ~55% of one face land at 0.4-0.6.
TWIN = 0.35
# The most a face someone on the street keeps may give a blend.
DONOR_CAP = 0.30

# The Creator's women presets (the rest are men's; the extras' were checked by face, backroom-wardrobe notes).
WOMEN = {"Ada", "Aera", "Asha", "Celeste", "Etta", "Grace", "Jelena", "Lani", "Sook-ja", "Sunita", "Tuya", "Vivian", "Zuri"}
# Faces kept for someone on the Street map (CastLooks.h): Benny (ExtraB, Isaiah), Augie (ExtraH, Bo), Inez (ExtraK, Lani).
STREET_KEPT = {"Isaiah": "Benny", "Bo": "Augie", "Lani": "Inez"}
# Never blended in at all: a reserved face walks the same street as the staff.
NO_DONOR = {"Isaiah", "Bo"}

# Groom slots and their wardrobe folders under /MetaHumanCharacter/Optional/Grooms/Bindings (and GroomAssets).
SLOTS = {"Hair": "Hair", "Eyebrows": "Eyebrows", "Beard": "Beards", "Mustache": "Mustaches"}

# Who they are (spec 6.1), and how each is made:
#   blend     (preset, weight) x 3, normalized; the first gives the body, the skin and eyes until they're set, and the
#             wardrobe until it's replaced
#   skin      the skin color asked for (sRGB hex): the Creator's skin model takes the nearest tone it can make
#   wrinkles  the face texture's scan: 0 low, 1 medium, 2 high; stubble 0 none..3 high (women: none); marks 0..2, -1 any
#   eyes      the Creator's eye preset 1..12 (1 dark brown, 2 brown, 3 amber, 6 pale blue, 7 green, 9 hazel...)
#   teeth     wear 0..1 (worn down, gums back, ivory); lashes: grey 0..1
#   groom     slot: (style, melanin, redness, whiteness) or None for a clean slot; melanin 0 fair..1 black,
#             whiteness is grey
STAFF = {
    # Ray Molina, 50s: owns the Lucky Penny and works its days. Mexican-American, a shop polo; short and tidy, salt and
    # pepper, a full mustache. The body is Lorenzo's (a man of fifty, average build).
    "Ray": {
        "woman": False, "age": 56,
        "blend": [("Lorenzo", 0.40), ("Jorge", 0.35), ("Bruce", 0.25)],
        "skin": "b07a5a", "wrinkles": 1, "stubble": 1, "marks": -1,
        "eyes": 2, "teeth": 0.35, "lashes": 0.25,
        "groom": {
            "Hair": ("Hair_S_Clean", 0.85, 0.12, 0.35),
            "Eyebrows": ("Eyebrows_M_Thick", 0.85, 0.10, 0.20),
            "Mustache": ("Mustache_S_Full", 0.85, 0.12, 0.45),
            "Beard": None,
        },
    },
    # Lonnie Pruitt, 64: the barber, and the shop is his. Black, old school: thinning grey hair kept short, a grey pencil
    # mustache, the white smock over a shirt and tie. Walter's body (an older man's, a little round).
    "Lonnie": {
        "woman": False, "age": 64,
        "blend": [("Walter", 0.35), ("Omari", 0.35), ("Trey", 0.30)],
        "skin": "5e3e2e", "wrinkles": 2, "stubble": 1, "marks": 1,
        "eyes": 1, "teeth": 0.60, "lashes": 0.55,
        "groom": {
            "Hair": ("Hair_S_HairLoss", 0.95, 0.05, 0.75),
            "Eyebrows": ("Eyebrows_M_Unruly", 0.95, 0.05, 0.60),
            "Mustache": ("Mustache_S_PencilThin", 0.95, 0.05, 0.70),
            "Beard": None,
        },
    },
    # Bea Kowalski, 71: the thrift's volunteer. Polish-American, fair and pink, pale blue eyes, a short white layered
    # cut, a cardigan and a lanyard of keys. Sook-ja's body (a slight woman in her seventies).
    "Bea": {
        "woman": True, "age": 71,
        "blend": [("Sook-ja", 0.30), ("Jelena", 0.45), ("Vivian", 0.25)],
        "skin": "e8c8b5", "wrinkles": 2, "stubble": 0, "marks": 2,
        "eyes": 6, "teeth": 0.75, "lashes": 0.70,
        "groom": {
            "Hair": ("Hair_S_BobLayered", 0.30, 0.08, 0.92),
            "Eyebrows": ("Eyebrows_M_Thin", 0.35, 0.10, 0.75),
            "Beard": None,
            "Mustache": None,
        },
    },
    # Rosa Delgado, 46: trains people and owns the gym. Latina, fit, dark hair pulled back for work, a track jacket.
    # Etta's body (an athletic one).
    "Rosa": {
        "woman": True, "age": 46,
        "blend": [("Etta", 0.40), ("Celeste", 0.30), ("Sunita", 0.30)],
        "skin": "c08f6e", "wrinkles": 1, "stubble": 0, "marks": 0,
        "eyes": 3, "teeth": 0.15, "lashes": 0.10,
        "groom": {
            "Hair": ("Hair_S_PulledBack", 0.92, 0.22, 0.06),
            "Eyebrows": ("Eyebrows_M_Natural", 0.92, 0.20, 0.03),
            "Beard": None,
            "Mustache": None,
        },
    },
    # Ruby Okafor, 63: owns the diner and works its days. Nigerian-American, deep brown, short natural coils going grey,
    # a cardigan. Lani's body (a fuller woman near sixty; Lani's face is Inez's, so it gives 30% and no more).
    "Ruby": {
        "woman": True, "age": 63,
        "blend": [("Lani", 0.30), ("Ada", 0.40), ("Asha", 0.30)],
        "skin": "4e3226", "wrinkles": 2, "stubble": 0, "marks": 1,
        "eyes": 1, "teeth": 0.55, "lashes": 0.40,
        "groom": {
            "Hair": ("Hair_S_Coil", 1.00, 0.04, 0.50),
            "Eyebrows": ("Eyebrows_M_Fine", 1.00, 0.04, 0.30),
            "Beard": None,
            "Mustache": None,
        },
    },
    # Marlene, 54: the night waitress, the pink uniform and apron. Asian-American, from Grace, the one preset nobody
    # wears (her body too); her hair in an updo, dyed auburn over brows she never dyed.
    "Marlene": {
        "woman": True, "age": 54,
        "blend": [("Grace", 0.50), ("Aera", 0.25), ("Tuya", 0.25)],
        "skin": "d9b497", "wrinkles": 1, "stubble": 0, "marks": 1,
        "eyes": 2, "teeth": 0.35, "lashes": 0.20,
        "groom": {
            "Hair": ("Hair_S_Updo", 0.50, 0.75, 0.00),
            "Eyebrows": ("Eyebrows_M_SlightArch", 0.85, 0.15, 0.05),
            "Beard": None,
            "Mustache": None,
        },
    },
}


def _path(name):
    return f"{cast.CAST_DIR}/MHC_{name}"


def _preset_path(preset):
    return f"{PRESETS_DIR}/{preset}"


def _wi(slot, style):
    return cast._wi(SLOTS[slot], style)


def presets():
    return list(L.list_presets())


def _wearers():
    """{preset: [who wears it]}: the Back Room's and the Embercrest's cast, and the heroes."""
    out = {}
    for who, preset, _ in cast.CAST:
        out.setdefault(preset, []).append(who)
    for hero in hero_cast.HERO_PRESETS:
        out.setdefault(hero_cast.HERO_PRESETS[hero] or hero_cast.DEFAULTS[hero], []).append(hero)
    return out


def _heroes():
    return {hero_cast.HERO_PRESETS[h] or hero_cast.DEFAULTS[h] for h in hero_cast.HERO_PRESETS}


def _blend(name):
    """(presets, normalized weights) for one of the staff."""
    pairs = STAFF[name]["blend"]
    total = sum(max(0.0, w) for _, w in pairs)
    return [p for p, _ in pairs], [max(0.0, w) / total for _, w in pairs]


def _blend_text(name):
    names, weights = _blend(name)
    return " + ".join(f"{p} {w:.2f}" for p, w in zip(names, weights))


def _recipe(name):
    """Everything a build is made from, as one line: a change to any of it makes the character again."""
    s = STAFF[name]
    groom = " ".join(f"{slot}={g[0]}/{g[1]:.2f}/{g[2]:.2f}/{g[3]:.2f}" if g else f"{slot}=-" for slot, g in sorted(s["groom"].items()))
    return (f"{_blend_text(name)} | skin {s['skin']} w{s['wrinkles']} s{s['stubble']} m{s['marks']} | eyes {s['eyes']} "
            f"| teeth {s['teeth']:.2f} lashes {s['lashes']:.2f} | {groom}")


def _has_cards(slot, style):
    """Whether an installed groom has hair cards (its GroomAssets folder holds *_CardsMesh_* or *_CardMesh_* meshes).
    True when its folder can't be read: the build then shows it."""
    folder = f"{GROOM_ASSETS}/{SLOTS[slot]}/{style}"
    if not EAL.does_directory_exist(folder):
        return True
    return any(re.search(r"_Cards?Mesh_", p.split("/")[-1]) for p in EAL.list_assets(folder, recursive=False))


def _validate():
    """Raises ValueError naming the first entry of STAFF that can't be made as written."""
    known = set(presets())
    heroes = _heroes()
    for name, s in STAFF.items():
        names, weights = _blend(name)
        if not 2 <= len(names) <= 3:
            raise ValueError(f"{name}: blend two or three presets, not {len(names)}")
        if len(set(names)) != len(names):
            raise ValueError(f"{name}: a preset twice in the blend")
        for preset, weight in zip(names, weights):
            if preset not in known:
                raise ValueError(f"{name}: the Creator has no preset {preset!r} (s.presets() lists them)")
            if (preset in WOMEN) != s["woman"]:
                raise ValueError(f"{name}: {preset} is a {'woman' if preset in WOMEN else 'man'}'s face")
            if preset in NO_DONOR:
                raise ValueError(f"{name}: {preset} is {STREET_KEPT[preset]}'s face on the street; blend another")
            kept = "the player's hero face" if preset in heroes else STREET_KEPT.get(preset)
            if kept and weight > DONOR_CAP + 1e-6:
                raise ValueError(f"{name}: {preset} is {kept}; it may give at most {DONOR_CAP:.0%}, not {weight:.0%}")
        for slot, g in s["groom"].items():
            if slot not in SLOTS:
                raise ValueError(f"{name}: no groom slot {slot!r} ({', '.join(SLOTS)})")
            if g and not EAL.does_asset_exist(_wi(slot, g[0]).split(".")[0]):
                raise ValueError(f"{name}: no wardrobe item {g[0]} in {SLOTS[slot]}")
            if g and not _has_cards(slot, g[0]):
                raise ValueError(f"{name}: {g[0]} has no hair cards, so at quality {QUALITY} it's only painted on; pick a style with cards")
        if s["woman"] and s["stubble"] not in (0, -1):
            raise ValueError(f"{name}: a woman's face texture has no stubble")
        if not 1 <= s["eyes"] <= 12:
            raise ValueError(f"{name}: eye presets are 1..12")


def _meta(asset, tag):
    return EAL.get_metadata_tag(asset, tag) or "" if asset else ""


def _texture_of(asset):
    m = re.search(r"texture=(\d+)", L.describe_skin(asset)) if asset else None
    return int(m.group(1)) if m else None


def _preset_textures():
    """{preset: its face texture}."""
    out = {}
    for preset in presets():
        t = _texture_of(L.find(_preset_path(preset)))
        if t is not None:
            out[preset] = t
    return out


def _pick_texture(name, c):
    """(face texture, the attributes it matched) for one of the staff; (-1, None) keeps the first preset's."""
    s = STAFF[name]
    woman = s["woman"]
    by_preset = _preset_textures()
    worn = set(by_preset.values())
    donors = {by_preset[p] for p in _blend(name)[0] if p in by_preset}
    staff = {_texture_of(L.find(_path(o))) for o in STAFF if o != name and EAL.does_asset_exist(_path(o))}
    women_scans = {t for p, t in by_preset.items() if p in WOMEN}
    w, st, m = s["wrinkles"], 0 if woman else s["stubble"], s["marks"]
    tries = [(w, st, m), (w, st, -1)] + ([] if woman else [(w, -1, -1)])
    in_women, unworn, anything = (lambda t: t in women_scans), (lambda t: t not in worn), (lambda t: True)
    if woman:
        # A scan a woman preset wears, with the marks asked for or any, before one nobody wears: a scan with no stubble
        # may be a clean-shaven man's.
        order = [(attrs, keep) for keep in (in_women, unworn) for attrs in tries]
    else:
        # The attributes first (a mustache over a heavy-stubble scan reads wrong); one nobody wears where there is one.
        order = [(attrs, keep) for attrs in tries for keep in (unworn, anything)]
    for attrs, keep in order:
        ranked = [t for t in L.face_textures(c, *attrs) if t not in donors and t not in staff and keep(t)]
        if ranked:
            return ranked[zlib.crc32(name.encode()) % len(ranked)], attrs
    return -1, None


def _srgb(hex6):
    return unreal.LinearColor(int(hex6[0:2], 16) / 255.0, int(hex6[2:4], 16) / 255.0, int(hex6[4:6], 16) / 255.0, 1.0)


def _shape(name, c):
    """Skin, eyes, age and grooms on a character fresh from its blend. Returns what it did, briefly."""
    s = STAFF[name]
    texture, matched = _pick_texture(name, c)
    if not L.set_skin_color(c, _srgb(s["skin"]), texture, -1.0):
        raise RuntimeError(f"{name}: the skin wasn't set (see the log)")
    L.set_eyes_preset(c, s["eyes"])
    L.set_aging(c, s["teeth"], s["lashes"])
    for slot in SLOTS:
        g = s["groom"].get(slot)
        L.set_wardrobe(c, slot, _wi(slot, g[0]) if g else "")
        if g:
            L.set_groom_color(c, slot, g[1], g[2], g[3], -1.0)
    wanted = (s["wrinkles"], 0 if s["woman"] else s["stubble"], s["marks"])
    note = "" if matched == wanted else f" (no free scan {wanted}: {'kept the first preset' if matched is None else matched})"
    return f"skin {L.describe_skin(c)}{note}"


def _face_check(name, c):
    """(not a twin, "nearest ...") for an open character, against every preset and the staff made before it. A preset
    nobody wears (Grace) is listed but can't make a twin: nobody in the game has that face."""
    wearers = _wearers()
    preset_names = presets()
    staff = [o for o in STAFF if o != name and EAL.does_asset_exist(_path(o))]
    others = preset_names + [_path(o) for o in staff]
    labels = [f"{p} ({', '.join(wearers.get(p, ['nobody']))})" for p in preset_names] + [f"{o} (staff)" for o in staff]
    worn = [p in wearers for p in preset_names] + [True] * len(staff)
    distances = list(L.face_distances(c, others))
    if len(distances) != len(others):
        return True, "face check unavailable"
    woman = STAFF[name]["woman"]
    same = sorted(d for p, d in zip(preset_names, distances) if d > 0 and (p in WOMEN) == woman)
    if not same:
        return True, "face check unavailable (no preset could be read)"
    typical = same[len(same) // 2]
    ranked = sorted((d / typical, label, w) for label, d, w in zip(labels, distances, worn) if d >= 0)
    nearest = ", ".join(f"{label} {r:.2f}" for r, label, _ in ranked[:3])
    closest_worn = next((r for r, _, w in ranked if w), 1.0)
    return closest_worn >= TWIN, f"nearest {nearest} (x the typical {typical:.2f} cm; a twin of a worn face is under {TWIN})"


def _close_others(name):
    for other in STAFF:
        o = L.find(_path(other)) if other != name and EAL.does_asset_exist(_path(other)) else None
        if o:
            L.close(o)


def _built_recipe(name):
    bp = cast.built_blueprint(name)
    return _meta(EAL.load_asset(bp), "ShortStackRecipe") if bp else ""


def _record(name, c, bp):
    """Writes what a build was made from on its Blueprint (so a later change of STAFF is caught: status, step)."""
    asset = EAL.load_asset(bp)
    if not asset:
        return
    EAL.set_metadata_tag(asset, "ShortStackRecipe", _recipe(name))
    EAL.set_metadata_tag(asset, "ShortStackPreset", _blend_text(name))
    EAL.set_metadata_tag(asset, "ShortStackQuality", str(QUALITY))
    EAL.set_metadata_tag(asset, "ShortStackSkin", L.describe_skin(c))
    EAL.set_metadata_tag(asset, "ShortStackFaceCheck", _meta(c, "ShortStackFaceCheck"))
    EAL.save_loaded_asset(asset)


def status():
    out = {}
    try:
        _validate()
    except ValueError as exc:
        out["stopped"] = str(exc)
    for name in STAFF:
        c = L.find(_path(name)) if EAL.does_asset_exist(_path(name)) else None
        built = cast.built_blueprint(name)
        line = ("built " if built else "") + (L.status(c) if c else "missing")
        if built and _built_recipe(name) != _recipe(name):
            line += f" (built from another recipe: s.rebuild({name!r}) to make it as STAFF says)"
        elif c and not built and _meta(c, "ShortStackRecipe") != _recipe(name):
            line += " (STAFF changed: step() makes it again)"
        check = _meta(c, "ShortStackFaceCheck")
        if check:
            line += f" | {check}"
        out[name] = line
    return out


def survey():
    """{preset: "who wears it | its skin"}: the faces there are to blend (the scans' attributes once a character is open)."""
    wearers = _wearers()
    return {p: f"{', '.join(wearers.get(p, ['nobody']))} | {'woman' if p in WOMEN else 'man'} | {L.describe_skin(L.find(_preset_path(p)))}"
            for p in presets()}


def check(name):
    """Opens one of the staff (closing the others) and says how near the nearest faces are."""
    if name not in STAFF:
        raise ValueError(f"{name!r} isn't one of the staff ({', '.join(STAFF)})")
    if not EAL.does_asset_exist(_path(name)):
        return f"{name}: not made yet (s.shape() or s.step())"
    _close_others(name)
    names, weights = _blend(name)
    c = L.create_from_blend(_path(name), names, weights, False)
    ok, text = _face_check(name, c)
    return f"{name}: {'ok' if ok else 'TWIN'}: {text}"


def rebuild(name):
    """Throws one of the staff away, its build and its character asset (the cloud's rig and textures with it), so that
    step() makes it again from STAFF."""
    if name not in STAFF:
        raise ValueError(f"{name!r} isn't one of the staff ({', '.join(STAFF)})")
    c = L.find(_path(name)) if EAL.does_asset_exist(_path(name)) else None
    if c:
        L.close(c)
    folder = f"{cast.BUILT_DIR}/MHC_{name}"
    if EAL.does_directory_exist(folder) and not EAL.delete_directory(folder):
        return f"{name}: couldn't delete {folder} (still in use?); nothing else was removed"
    if EAL.does_asset_exist(_path(name)) and not EAL.delete_asset(_path(name)):
        return f"{name}: deleted the build, but not {_path(name)} (still in use?); delete it before step()"
    return f"{name}: removed; s.step() makes it again"


def _make(name, remake):
    """Opens a new character from its blend (or makes an existing one again), shapes it, checks its face, saves it."""
    free = shutil.disk_usage(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).free
    if free < cast.MIN_FREE:
        return f"{name}: stopped, only {free / 1024 ** 3:.1f} GB free"
    _close_others(name)
    names, weights = _blend(name)
    c = L.create_from_blend(_path(name), names, weights, remake)
    if not c:
        return f"{name}: stopped, the blend failed (the log says why)"
    shaped = _shape(name, c)
    ok, text = _face_check(name, c)
    EAL.set_metadata_tag(c, "ShortStackRecipe", _recipe(name))
    EAL.set_metadata_tag(c, "ShortStackFaceCheck", ("ok: " if ok else "TWIN: ") + text)
    L.save(c)
    if not ok:
        return (f"{name}: stopped, a twin: {text}. Change STAFF[{name!r}]['blend'] (more of the others, less of the nearest) "
                f"and step() makes it again")
    return f"{name}: {'made again' if remake else 'opened'} ({_blend_text(name)}); {shaped}; {text}"


def _made(name):
    """The character asset if it's made as STAFF says now, None if it must be made (again); built ones are skipped by the
    callers."""
    c = L.find(_path(name)) if EAL.does_asset_exist(_path(name)) else None
    return c if c and _meta(c, "ShortStackRecipe") == _recipe(name) else None


def shape():
    """Blends, shapes and checks the first of the staff not built and not made as STAFF says, saves it and closes it:
    nothing goes to the cloud, so all six faces can be looked at (status(), or each MHC_<Name> opened in the Creator)
    before any is rigged. Repeat until "shaped all"; step() rigs and builds from there."""
    try:
        _validate()
    except ValueError as exc:
        return f"stopped: {exc}"
    for name in STAFF:
        if cast.built_blueprint(name) or _made(name):
            continue
        remake = EAL.does_asset_exist(_path(name))
        said = _make(name, remake)
        c = L.find(_path(name)) if EAL.does_asset_exist(_path(name)) else None
        if c:
            L.close(c)
        # A twin is shaped and recorded like the rest (status() shows it); only a lack of disk or a failed blend stops.
        return said.replace("stopped, a twin", "shaped, a TWIN") if "a twin" in said else said
    checks = []
    for name in STAFF:
        c = L.find(_path(name)) if EAL.does_asset_exist(_path(name)) else None
        verdict = "built" if cast.built_blueprint(name) else _meta(c, "ShortStackFaceCheck").split(":")[0] or "?"
        checks.append(f"{name} {verdict}")
    return "shaped all: " + ", ".join(checks)


def step():
    """Advances the first unfinished one of the staff by one step and says what it did."""
    try:
        _validate()
    except ValueError as exc:
        return f"stopped: {exc}"
    for name in STAFF:
        recipe = _recipe(name)
        bp = cast.built_blueprint(name)
        if bp:
            made = _built_recipe(name)
            if not made:
                # Assembled, but the editor went before the recipe was written on the Blueprint: the character asset
                # says what it was made from (its rig and textures are the build's).
                c = L.find(_path(name)) if EAL.does_asset_exist(_path(name)) else None
                if c and _meta(c, "ShortStackRecipe") == recipe:
                    _record(name, c, bp)
                    made = recipe
            if made != recipe:
                return f"stopped: {name} was built from another recipe; s.rebuild({name!r}) to make it as STAFF says"
            continue
        c = _made(name)
        if not c:
            return _make(name, EAL.does_asset_exist(_path(name)))
        if _meta(c, "ShortStackFaceCheck").startswith("TWIN"):
            return f"{name}: stopped, a twin ({_meta(c, 'ShortStackFaceCheck')}); change its blend in STAFF"
        if "open=0" in L.status(c):
            # A fresh editor: open it again as it was made (no blend, no shaping).
            _close_others(name)
            names, weights = _blend(name)
            c = L.create_from_blend(_path(name), names, weights, False)
            return f"{name}: reopened"
        s = L.status(c)
        if "rigging=1" in s or "requesting=1" in s:
            return f"{name}: waiting ({s})"
        if "rigged=0" in s:
            L.auto_rig(c, True)
            return f"{name}: rigging"
        if "textures=0" in s:
            L.request_textures(c, cast.TEXTURES)
            return f"{name}: requesting textures"
        dirty = {p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()}
        if c.get_outermost().get_name() in dirty:
            L.save(c)
            return f"{name}: saved"
        if "buildable=1" not in s:
            return f"{name}: cannot build ({s})"
        if not L.assemble(c, cast.BUILT_DIR, cast.COMMON_DIR, QUALITY):
            return f"{name}: assembly failed"
        bp = cast.built_blueprint(name)
        if bp:
            _record(name, c, bp)
        L.save(c)
        # Content only (the build's packages): the editor opens on NightOne, and a map dirtied by its construction
        # scripts is never written from here.
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(False, True)
        L.close(c)
        return f"{name}: assembled ({cast.built_blueprint(name)})"
    return "done"
