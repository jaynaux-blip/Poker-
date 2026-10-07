"""
SHORT STACK: the hero faces' groom libraries (the realism update's spec, 7.5.4), made by assembly.

The barber changes the hero's hair, beard and brows in 3D, so each hero face (HeroA0..HeroB2, hero_cast.py) needs a card
groom for every style the creator offers, with a binding baked against that face. The engine's grooms live in the
MetaHumanCharacter plugin's Optional content (2.6 GB with strands, Beta): nothing the game loads may reference them. So the
copies are made the way the heroes themselves were: for each style the face lacks, a working copy of the hero's character
wears it (SetWardrobe) and is assembled (Assemble, at the hero's own quality, hero_cast.QUALITY: cards LOD0-4 and helmets);
only Grooms/ is kept, its bindings retargeted to the face of the hero's own build, and moved to

    /Game/ShortStack/Cast/Grooms/<Hero>/<style>/        the groom, its binding, cards meshes, helmets, materials, atlases
    /Game/ShortStack/Cast/Grooms/<Hero>/HGL_<Hero>      the library (UHeroGroomLibrary): every copy and the build's own
                                                        grooms, by style; HeroGrooms::Resolve reads it at run time

One assembly per style of hair the face lacks (11 hairstyles in the map, less the one the face was built with: 10 per hero,
60 in all, about 4 minutes each); the beards, mustaches and brows the face lacks ride along in those (Beard, Mustache,
Eyebrows set in the same run). The map below mirrors HeroGrooms.h's (change both together); the face's skin band picks the
coily variants (band 2). Each run happens in a fresh editor (consecutive assemblies ran the editor out of memory), so this is
a state machine like backroom_cast.step(): call step() until it says "done". The scratchpad's groom_loop.sh relaunches the
editor after every "assembled" (the next editor's first step collects that build). Disk: about 10-40 MB a style, 0.5-1 GB
in all; nothing is assembled with less than backroom_cast.MIN_FREE free.

    import groom_setup as g
    g.status()          # per hero: the styles it has (its own, the copies), the runs still to go, the library
    g.plan("HeroA0")    # the runs a hero still needs: [(tag, {slot: style or None})]
    g.step()            # one step; repeat until "done"
    g.run()             # steps until one assembly is made (then restart the editor), "done" or a stop
                        # (shortstack_setup.run_grooms() calls it)
    g.refresh("HeroA0") # the library's list again from what's on disk (after copying or deleting styles by hand)

The heroes' own characters (/Game/ShortStack/Cast/MHC_Hero*) are never changed: each run's wardrobe goes on a working copy
(/Game/ShortStack/Cast/GroomWork/MHC_<Hero>, the same name so the assembly names its output the same), deleted when the
hero's library is done.
"""
import json
import os
import shutil

import unreal

import backroom_cast as cast
import hero_cast

L = unreal.ShortStackMetaHumanLibrary
eal = unreal.EditorAssetLibrary

GROOM_DIR = "/Game/ShortStack/Cast/Grooms"
WORK_DIR = "/Game/ShortStack/Cast/GroomWork"
BUILD_DIR = "/Game/ShortStack/Cast/GroomBuild"
HEROES = ("HeroA0", "HeroA1", "HeroA2", "HeroB0", "HeroB1", "HeroB2")

# The creator's hairstyles (hero::Look::Hair 1..11; 0 is shaved) as the engine wardrobe's grooms: (bands 0-1, band 2).
HAIR = [
    ("Hair_S_BuzzCut", "Hair_S_CoilBuzzCut"),          # 1 Buzz Cut
    ("Hair_S_BrushCut", "Hair_S_BrushCut"),            # 2 Crew Cut
    ("Hair_S_Messy", "Hair_S_Messy"),                  # 3 Textured Crop
    ("Hair_S_SideSweptFringe", "Hair_S_SideSweptFringe"),  # 4 Side Part
    ("Hair_S_SweptUp", "Hair_S_SweptUp"),              # 5 Undercut
    ("Hair_S_CurlyFade", "Hair_S_Coil"),               # 6 Curls
    ("Hair_S_AfroFade", "Hair_L_AfroCurly"),           # 7 Afro
    ("Hair_L_Straight", "Hair_L_Straight"),            # 8 Shoulder Length
    ("Hair_S_LowPonytail", "Hair_S_LowPonytail"),      # 9 Ponytail
    ("Hair_M_TwistedBraids", "Hair_S_Cornrows"),       # 10 Braids
    ("Hair_M_BobStraight", "Hair_M_BobStraight"),      # 11 Bob
]
# Facial hair (hero::Look::FacialHair 1..5): the beards and the mustaches they wear.
BEARDS = ["Beard_S_Stubble", "Goatee_M_Pointed", "Beard_S_Full", "Beard_L_Full"]
MUSTACHES = ["Mustache_S_Stubble", "Mustache_M_Gunslinger", "Mustache_S_Full", "Mustache_L_Full"]
# Brows (hero::Look::Brows): Thin and Thick per body type; Natural is the face's own; Arched wears Thin's.
BROWS = {"A": ["Eyebrows_S_FlatThin", "Eyebrows_M_Thick"], "B": ["Eyebrows_M_Thin", "Eyebrows_M_Full"]}
# The engine wardrobe's folders for each slot (backroom_cast._wi).
FOLDER = {"Hair": "Hair", "Beard": "Beards", "Mustache": "Mustaches", "Eyebrows": "Eyebrows"}
# The kinds the library holds (the lashes and the peach fuzz stay the build's own).
KINDS = ("Hair_", "Eyebrows_", "Beard_", "Goatee_", "Mustache_")


def _band(hero):
    return int(hero[-1])


def _type(hero):
    return "B" if hero.startswith("HeroB") else "A"


def _wanted(hero):
    """Every style the map asks for on this face, by slot."""
    band2 = _band(hero) >= 2
    hairs = []
    for pair in HAIR:
        style = pair[1] if band2 else pair[0]
        if style not in hairs:
            hairs.append(style)
    return {"Hair": hairs, "Beard": list(BEARDS), "Mustache": list(MUSTACHES), "Eyebrows": list(BROWS[_type(hero)])}


def _registry():
    return unreal.AssetRegistryHelpers.get_asset_registry()


def _bindings(folder, recursive=True):
    """Every groom binding under a folder: [(style, object path)], the style the binding's name less _Binding."""
    if not eal.does_directory_exist(folder):
        return []
    out = []
    for path in eal.list_assets(folder, recursive=recursive, include_folder=False):
        data = eal.find_asset_data(path)
        try:
            cls = str(data.asset_class_path.asset_name)
        except Exception:
            cls = str(data.asset_class)
        if cls != "GroomBindingAsset":
            continue
        name = str(data.asset_name)
        style = name[:-len("_Binding")] if name.endswith("_Binding") else name
        out.append((style, path))
    return out


def own_styles(hero):
    """The styles the hero's own build wears (its Grooms folder's bindings; GroomAsset_N names skipped)."""
    return [s for s, _ in _bindings(f"{cast.BUILT_DIR}/MHC_{hero}/Grooms", recursive=False) if not s.startswith("GroomAsset")]


def copied_styles(hero):
    return [s for s, _ in _bindings(f"{GROOM_DIR}/{hero}")]


def plan(hero):
    """The assemblies a hero still needs: [(tag, {slot: style or None})]. One per hairstyle it lacks; the beards, the
    mustaches and the brows it lacks ride along, one of each per run."""
    have = set(own_styles(hero)) | set(copied_styles(hero))
    need = {slot: [s for s in styles if s not in have] for slot, styles in _wanted(hero).items()}
    runs = []
    count = max(len(v) for v in need.values())
    for i in range(count):
        slots = {slot: (v[i] if i < len(v) else None) for slot, v in need.items()}
        tag = next(s for s in (slots["Hair"], slots["Beard"], slots["Mustache"], slots["Eyebrows"]) if s)
        runs.append((tag, slots))
    return runs


def _face(hero):
    """The face mesh of the hero's own build (every copy's binding is retargeted to it)."""
    folder = f"{cast.BUILT_DIR}/MHC_{hero}/Face"
    if not eal.does_directory_exist(folder):
        return None
    for path in eal.list_assets(folder, recursive=False, include_folder=False):
        if path.split("/")[-1].split(".")[0].endswith("_FaceMesh"):
            return eal.load_asset(path)
    for path in eal.list_assets(folder, recursive=False, include_folder=False):
        obj = eal.load_asset(path)
        if isinstance(obj, unreal.SkeletalMesh):
            return obj
    return None


def _manifest_path():
    return os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "ShortStack", "groom_build.json")


def _read_manifest():
    p = _manifest_path()
    if not os.path.isfile(p):
        return None
    with open(p, "r", encoding="utf-8") as f:
        return json.load(f)


def _write_manifest(data):
    p = _manifest_path()
    os.makedirs(os.path.dirname(p), exist_ok=True)
    if data is None:
        if os.path.isfile(p):
            os.remove(p)
        return
    with open(p, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=1)


def _work_path(hero):
    return f"{WORK_DIR}/MHC_{hero}"


def _close_all():
    """One character open for editing at a time (the editor has crashed with several)."""
    for name, _, _ in cast.CAST:
        c = L.find(cast._path(name))
        if c:
            L.close(c)
    for hero in HEROES:
        for path in (cast._path(hero), _work_path(hero)):
            c = L.find(path)
            if c:
                L.close(c)


def _free_bytes():
    return shutil.disk_usage(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).free


def _save_dirty():
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)


def _collect(m):
    """The last assembly's grooms into the library's folder: bindings retargeted to the hero's own face, the build's actor,
    body, face and clothes deleted, Grooms/ moved to Grooms/<Hero>/<tag>, the lashes and fuzz that came along dropped."""
    hero, tag = m["hero"], m["tag"]
    src = f"{BUILD_DIR}/MHC_{hero}"
    grooms = f"{src}/Grooms"
    dest = f"{GROOM_DIR}/{hero}/{tag}"
    if not eal.does_directory_exist(grooms):
        if eal.does_directory_exist(src):
            eal.delete_directory(src)
        _write_manifest(None)
        return f"{hero}: the {tag} build left no grooms (assembly failed?); it will be tried again"
    face = _face(hero)
    if face is None:
        return f"{hero}: stopped, its own build's face isn't there ({cast.BUILT_DIR}/MHC_{hero}/Face)"
    retargeted = 0
    for _, path in _bindings(grooms):
        binding = eal.load_asset(path)
        if binding is None:
            continue
        binding.set_editor_property("target_skeletal_mesh", face)
        try:
            source = binding.get_editor_property("source_skeletal_mesh")
            if source and source.get_path_name().startswith(BUILD_DIR):
                binding.set_editor_property("source_skeletal_mesh", face)
        except Exception:
            pass
        eal.save_loaded_asset(binding)
        retargeted += 1
    # Nothing may point at the temporary face once it goes: the actor first (it holds everything), then its parts.
    for path in eal.list_assets(src, recursive=False, include_folder=False):
        if path.split("/")[-1].startswith("BP_"):
            eal.delete_asset(path.split(".")[0])
    for part in ("Body", "Face", "Clothing"):
        if eal.does_directory_exist(f"{src}/{part}"):
            eal.delete_directory(f"{src}/{part}")
    if eal.does_directory_exist(dest):
        eal.delete_directory(dest)
    if not eal.rename_directory(grooms, dest):
        return f"{hero}: stopped, couldn't move {grooms} to {dest}"
    # The lashes and the peach fuzz the assembly brought along (the build's own stay the library's).
    for path in eal.list_assets(dest, recursive=True, include_folder=False):
        name = path.split("/")[-1]
        if name.startswith("Eyelashes_") or name.startswith("Peachfuzz_"):
            eal.delete_asset(path.split(".")[0])
    eal.delete_directory(src)
    _save_dirty()
    _write_manifest(None)
    # The library grows with each copy (the game wears what's there; a style still to come reads PORTRAIT ONLY).
    line = refresh(hero)
    return f"{hero}: collected {tag} ({retargeted} bindings on {face.get_name()}) into {dest}; {line}"


def library_path(hero):
    return f"{GROOM_DIR}/{hero}/HGL_{hero}"


def _signature(hero):
    """What the library was made from: every binding it could list (the copies and the build's own), hashed."""
    paths = sorted(p for _, p in _bindings(f"{GROOM_DIR}/{hero}")) + sorted(p for _, p in _bindings(f"{cast.BUILT_DIR}/MHC_{hero}/Grooms", recursive=False))
    import hashlib
    return hashlib.md5("\n".join(paths).encode("utf-8")).hexdigest()


def refresh(hero):
    """Makes (or updates) the hero's library from what's on disk (UHeroGroomLibrary.Refresh) and saves it."""
    path = library_path(hero)
    if eal.does_asset_exist(path):
        lib = eal.load_asset(path)
    else:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.HeroGroomLibrary)
        lib = unreal.AssetToolsHelpers.get_asset_tools().create_asset(f"HGL_{hero}", f"{GROOM_DIR}/{hero}", unreal.HeroGroomLibrary, factory)
    if lib is None:
        return f"{hero}: couldn't make {path}"
    lib.set_editor_property("face", hero)
    line = lib.refresh()
    eal.set_metadata_tag(lib, "GroomSignature", _signature(hero))
    eal.save_loaded_asset(lib)
    return line


def _library_count(hero):
    path = library_path(hero)
    if not eal.does_asset_exist(path):
        return -1
    return len(eal.load_asset(path).get_editor_property("entries"))


def _library_current(hero):
    """The library lists what's on disk now (made after the last copy came in)."""
    path = library_path(hero)
    if not eal.does_asset_exist(path):
        return False
    return eal.get_metadata_tag(eal.load_asset(path), "GroomSignature") == _signature(hero)


def status():
    out = {}
    m = _read_manifest()
    if m:
        out["pending"] = f"{m['hero']}: {m['tag']} assembled, not collected yet (step() collects it)"
    for hero in HEROES:
        if not cast.built_blueprint(hero):
            out[hero] = "not built (hero_cast.py)"
            continue
        runs = plan(hero)
        lib = _library_count(hero)
        out[hero] = (f"own {own_styles(hero)}, {len(copied_styles(hero))} copies, {len(runs)} runs to go"
                     + (f" (next {runs[0][0]})" if runs else "") + f", library {'missing' if lib < 0 else str(lib) + ' styles'}")
    return out


def step():
    """Advances by one step and says what it did: collect the last assembly, assemble the next style, write a finished
    hero's library, tidy its working copy; "done" when every hero is complete."""
    m = _read_manifest()
    if m:
        return _collect(m)
    for hero in HEROES:
        if not cast.built_blueprint(hero):
            continue
        runs = plan(hero)
        if not runs:
            if not _library_current(hero):
                return f"{hero}: library {refresh(hero)}"
            if eal.does_asset_exist(_work_path(hero)):
                c = L.find(_work_path(hero))
                if c:
                    L.close(c)
                eal.delete_asset(_work_path(hero))
                return f"{hero}: library complete; working copy removed"
            continue
        tag, slots = runs[0]
        free = _free_bytes()
        if free < cast.MIN_FREE:
            return f"{hero}: stopped, only {free / 1024 ** 3:.1f} GB free"
        if eal.does_directory_exist(f"{BUILD_DIR}/MHC_{hero}"):
            # A build without a manifest (an editor that died mid-assembly): out with it.
            eal.delete_directory(f"{BUILD_DIR}/MHC_{hero}")
        _close_all()
        if not eal.does_asset_exist(_work_path(hero)):
            if not eal.duplicate_asset(cast._path(hero), _work_path(hero)):
                return f"{hero}: stopped, couldn't copy {cast._path(hero)} to {_work_path(hero)}"
            _save_dirty()
            return f"{hero}: working copy made ({_work_path(hero)})"
        c = L.create_from_preset(_work_path(hero), hero_cast.DEFAULTS.get(hero, ""), False)
        if not c:
            return f"{hero}: stopped, couldn't open {_work_path(hero)}"
        s = L.status(c)
        if "rigged=0" in s or "textures=0" in s:
            L.close(c)
            return f"{hero}: stopped, the working copy isn't rigged and textured ({s}); delete {_work_path(hero)} and step() again"
        for slot, style in slots.items():
            if not L.set_wardrobe(c, slot, cast._wi(FOLDER[slot], style) if style else ""):
                L.close(c)
                return f"{hero}: stopped, {slot} = {style} refused (see the log)"
        if "buildable=1" not in L.status(c):
            L.close(c)
            return f"{hero}: cannot build ({L.status(c)})"
        _write_manifest({"hero": hero, "tag": tag, "slots": slots})
        if not L.assemble(c, BUILD_DIR, cast.COMMON_DIR, hero_cast.QUALITY):
            _write_manifest(None)
            L.close(c)
            return f"{hero}: assembly failed ({tag})"
        L.save(c)
        _save_dirty()
        L.close(c)
        return f"{hero}: assembled {tag} ({', '.join(f'{k}={v}' for k, v in slots.items() if v)})"
    return "done"


def run(max_steps=40):
    """One editor's worth of steps (shortstack_setup.run_grooms): step() until an assembly is made (the editor must be
    restarted before the next: consecutive assemblies ran it out of memory), the work is done, or a step stops. Returns
    the steps' lines. groom_loop.sh restarts the editor and calls step() round after round, unattended."""
    lines = []
    for _ in range(max_steps):
        line = step()
        unreal.log(f"ShortStack groom_setup: {line}")
        lines.append(line)
        if line == "done" or "assembled" in line or "stopped" in line or "failed" in line or "cannot build" in line:
            break
    return lines
