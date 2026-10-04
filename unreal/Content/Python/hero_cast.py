"""
SHORT STACK: the player's MetaHumans for the open world, one per body type and skin-tone band.

AShortStackCharacter dresses the player as Hero<Body><Band>: HeroA0..HeroA2 and HeroB0..HeroB2 (body type
A or B from the character creator; band 0 for skin tones 1-3, 1 for 4-6, 2 for 7-10), falling back to the
Back Room's "Hero" and then the archetype body. Each is built like the Back Room's cast (backroom_cast.py:
open, rig, texture, save, assemble, close, one at a time) from its own preset: HERO_PRESETS names one per
hero, and None takes the one in DEFAULTS, chosen by body type and skin tone. A hero's face is nobody else's:
a preset the cast wears (Benny behind the Lucky Penny's counter, the regulars at the Embercrest) is refused,
so the player never meets their twin. The Back Room's Hero is the player too, so its preset may be reused.

Hair color, facial hair (as far as the hero's grooms go), height, build and clothes come from the creator at
run time. A hairstyle can't be swapped there, so STYLE gives each hero hair that lies close to the head or is
tied back, which sits under the creator's hats (the game takes fuller hair off under a hat), and gives the
type A heroes stubble, which the game shows or takes off as the creator says.

    import hero_cast as h
    h.presets()          # what the Creator offers
    h.step()             # repeat until "done"
    h.status()
    h.rebuild("HeroA0")  # throws a hero away (to change its preset or style); step() makes it again
"""
import shutil

import unreal

import backroom_cast as cast

L = unreal.ShortStackMetaHumanLibrary

HERO_PRESETS = {
    "HeroA0": None,
    "HeroA1": None,
    "HeroA2": None,
    "HeroB0": None,
    "HeroB1": None,
    "HeroB2": None,
}
# Creator presets nobody in the cast wears, by look: type A light (Aoi), medium (Dominic), deep (Cameron);
# type B light (Vivian), medium (Celeste), deep (Asha).
DEFAULTS = {"HeroA0": "Aoi", "HeroA1": "Dominic", "HeroA2": "Cameron", "HeroB0": "Vivian", "HeroB1": "Celeste", "HeroB2": "Asha"}
# Worn over the preset's grooms (the Creator's wardrobe items; None clears the slot).
_STUBBLE = {"Beard": cast._wi("Beards", "Beard_S_Stubble"), "Mustache": cast._wi("Mustaches", "Mustache_S_Stubble")}
_CLEAN = {"Beard": None, "Mustache": None}
STYLE = {
    "HeroA0": {"Hair": cast._wi("Hair", "Hair_S_BrushCut"), **_STUBBLE},
    "HeroA1": {"Hair": cast._wi("Hair", "Hair_S_CurlyFade"), **_STUBBLE},
    "HeroA2": {"Hair": cast._wi("Hair", "Hair_S_CoilBuzzCut"), **_STUBBLE},
    "HeroB0": {"Hair": cast._wi("Hair", "Hair_S_LowPonytail"), **_CLEAN},
    "HeroB1": {"Hair": cast._wi("Hair", "Hair_S_LowPonytail"), **_CLEAN},
    "HeroB2": {"Hair": cast._wi("Hair", "Hair_S_Cornrows"), **_CLEAN},
}
# Open-world characters are seen whole and from a distance: the "high" build (hair cards, fewer LODs). The game
# holds their grooms on their cards (there's no LOD sync without the Blueprint).
QUALITY = 2


def presets():
    return list(L.list_presets())


def _faces_taken():
    """{preset: who} for the faces the player meets: the cast's, but for the Back Room's Hero (that's the player)."""
    return {preset: name for name, preset, _ in cast.CAST if name != "Hero"}


def _choose():
    """{hero: preset} for every hero; raises ValueError naming the first whose preset can't be used."""
    known = set(presets())
    taken = _faces_taken()
    out = {}
    for name in HERO_PRESETS:
        preset = HERO_PRESETS[name] or DEFAULTS[name]
        if preset not in known:
            raise ValueError(f"{name}: the Creator has no preset {preset!r} (h.presets() lists them)")
        if preset in taken:
            raise ValueError(f"{name}: {preset} is {taken[preset]}'s face already; name another in HERO_PRESETS")
        twin = next((other for other, used in out.items() if used == preset), None)
        if twin:
            raise ValueError(f"{name}: {preset} is {twin} already")
        out[name] = preset
    return out


def _built_preset(name):
    """The preset a built hero was made from (recorded on its Blueprint), or None."""
    bp = cast.built_blueprint(name)
    if not bp:
        return None
    return unreal.EditorAssetLibrary.get_metadata_tag(unreal.EditorAssetLibrary.load_asset(bp), "ShortStackPreset") or None


def status():
    out = {}
    try:
        chosen = _choose()
    except ValueError as exc:
        chosen = {}
        out["stopped"] = str(exc)
    for name in HERO_PRESETS:
        c = L.find(cast._path(name))
        line = ("built " if cast.built_blueprint(name) else "") + (L.status(c) if c else "missing")
        made = _built_preset(name)
        if made and name in chosen and made != chosen[name]:
            line += f" (made from {made}, wants {chosen[name]}: h.rebuild({name!r}))"
        elif cast.built_blueprint(name) and not made:
            line += " (preset not recorded: h.rebuild() it if the face is wrong)"
        out[name] = line
    return out


def rebuild(name):
    """Throws a hero away, its built Blueprint and its character asset (the cloud's rig and textures with it), so that
    step() makes it again from HERO_PRESETS and STYLE. Only the heroes: the Back Room's cast is backroom_cast's."""
    if name not in HERO_PRESETS:
        raise ValueError(f"{name!r} isn't one of the heroes ({', '.join(HERO_PRESETS)})")
    eal = unreal.EditorAssetLibrary
    c = L.find(cast._path(name))
    if c:
        L.close(c)
    folder = f"{cast.BUILT_DIR}/MHC_{name}"
    if eal.does_directory_exist(folder) and not eal.delete_directory(folder):
        return f"{name}: couldn't delete {folder} (still in use?); nothing else was removed"
    if eal.does_asset_exist(cast._path(name)) and not eal.delete_asset(cast._path(name)):
        return f"{name}: deleted the build, but not {cast._path(name)} (still in use?); delete it before step()"
    return f"{name}: removed; h.step() makes it again"


def step():
    """Advances the first unfinished hero by one step and says what it did."""
    try:
        chosen = _choose()
    except ValueError as exc:
        return f"stopped: {exc}"
    for name, preset in chosen.items():
        if cast.built_blueprint(name):
            made = _built_preset(name)
            if made and made != preset:
                return f"{name}: made from {made}, but wants {preset}: h.rebuild({name!r}) first"
            continue
        c = L.find(cast._path(name))
        if not c or "open=0" in L.status(c):
            free = shutil.disk_usage(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).free
            if free < cast.MIN_FREE:
                return f"{name}: stopped, only {free / 1024 ** 3:.1f} GB free"
            for other in chosen:
                o = L.find(cast._path(other))
                if o and other != name:
                    L.close(o)
            c = L.create_from_preset(cast._path(name), preset, False)
            for slot, item in STYLE.get(name, {}).items():
                L.set_wardrobe(c, slot, item or "")
            L.save(c)
            return f"{name}: opened ({preset})"
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
            # What it was made from, so a later change of preset is caught (status, step).
            asset = unreal.EditorAssetLibrary.load_asset(bp)
            unreal.EditorAssetLibrary.set_metadata_tag(asset, "ShortStackPreset", preset)
            unreal.EditorAssetLibrary.set_metadata_tag(asset, "ShortStackQuality", str(QUALITY))
            unreal.EditorAssetLibrary.save_loaded_asset(asset)
        L.save(c)
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
        L.close(c)
        return f"{name}: assembled ({cast.built_blueprint(name)})"
    return "done"
