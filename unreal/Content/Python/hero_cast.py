"""
SHORT STACK: the player's MetaHumans for the open world, one per body type and skin-tone band.

AShortStackCharacter dresses the player as Hero<Body><Band>: HeroA0..HeroA2 and HeroB0..HeroB2 (body type
A or B from the character creator; band 0 for skin tones 1-3, 1 for 4-6, 2 for 7-10), falling back to the
Back Room's "Hero" and then the archetype body. Each is built like the Back Room's cast (backroom_cast.py:
open, rig, texture, save, assemble, close, one at a time) from the preset in HERO_PRESETS; None picks a
Creator preset the cast doesn't already use. Pick the faces you like by naming presets there
(unreal.ShortStackMetaHumanLibrary.list_presets() lists them).

    import hero_cast as h
    h.presets()     # what the Creator offers
    h.step()        # repeat until "done"
    h.status()
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
# If the Creator lists nothing new, these known presets stand in (light, medium, deep for each body type).
FALLBACK = {"HeroA0": "Bruce", "HeroA1": "Mateo", "HeroA2": "Trey", "HeroB0": "Jelena", "HeroB1": "Ada", "HeroB2": "Isaiah"}
# Open-world characters are seen whole and from a distance: the "high" build (hair cards, fewer LODs).
QUALITY = 2


def presets():
    return list(L.list_presets())


def _choose():
    taken = {preset for _, preset, _ in cast.CAST}
    fresh = [p for p in presets() if p not in taken]
    out = {}
    for i, name in enumerate(HERO_PRESETS):
        chosen = HERO_PRESETS[name]
        if not chosen:
            chosen = fresh[i] if i < len(fresh) else FALLBACK[name]
        out[name] = chosen
    return out


def status():
    out = {}
    for name in HERO_PRESETS:
        c = L.find(cast._path(name))
        out[name] = ("built " if cast.built_blueprint(name) else "") + (L.status(c) if c else "missing")
    return out


def step():
    """Advances the first unfinished hero by one step and says what it did."""
    chosen = _choose()
    for name, preset in chosen.items():
        if cast.built_blueprint(name):
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
        L.save(c)
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
        L.close(c)
        return f"{name}: assembled ({cast.built_blueprint(name)})"
    return "done"
