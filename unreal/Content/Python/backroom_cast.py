"""
SHORT STACK: the Back Room's cast, made with MetaHuman Creator (UShortStackMetaHumanLibrary).

Each character starts from one of the Creator's presets. The face rig and the high-resolution skin
come from Epic's MetaHuman cloud service, so the first request opens Epic's sign-in page in the browser.

The Creator holds a lot in memory for a character open for editing (preview meshes, 2K-8K textures),
and the editor has crashed with all five open at once, so the cast is made one character at a time:
open, rig, texture, save, assemble, close. The cloud steps finish on later editor ticks, so this is a
state machine: call step() every few seconds (from the editor's Python console or remote execution)
until it returns "done".

    import backroom_cast as c
    c.step()        # repeat until "done"
    c.status()
"""
import unreal

L = unreal.ShortStackMetaHumanLibrary
CAST_DIR = "/Game/ShortStack/Cast"
BUILT_DIR = "/Game/ShortStack/Cast/Built"
COMMON_DIR = "/Game/ShortStack/Cast/Common"
# The opponents sit within arm's reach under one hard light: the cinematic build (every LOD, strand hair).
QUALITY = 3
TEXTURES = 2048

# Who sits where (seats counterclockwise from the player; 4 deals), and the preset each starts from.
CAST = [
    # name, preset, seat
    ("Dee", "Zuri", 4),       # runs the Tuesday game and deals; an old-school pro
    ("Sal", "Walter", 3),     # the old-timer regular; tight, patient
    ("BigLou", "Jorge", 5),   # loud, calls everything
    ("Twitch", "Victor", 2),  # young, wired, bets too much
    ("Mei", "Tuya", 6),       # quiet, sharp, hard to read
    ("Hero", "Mateo", 0),     # you: only your hands, arms and chest are ever seen
]


def _path(name):
    return f"{CAST_DIR}/MHC_{name}"


def built_blueprint(name):
    """The assembled actor Blueprint's path, or None."""
    folder = f"{BUILT_DIR}/MHC_{name}"
    if not unreal.EditorAssetLibrary.does_directory_exist(folder):
        return None
    for path in unreal.EditorAssetLibrary.list_assets(folder, recursive=False):
        if path.split("/")[-1].split(".")[0].startswith("BP_"):
            return path.split(".")[0]
    return None


def status():
    out = {}
    for name, _, _ in CAST:
        c = L.find(_path(name))
        out[name] = ("built " if built_blueprint(name) else "") + (L.status(c) if c else "missing")
    return out


def step():
    """Advances the first unfinished character by one step and says what it did."""
    for name, preset, _ in CAST:
        if built_blueprint(name):
            continue
        c = L.find(_path(name))
        if not c or "open=0" in L.status(c):
            # Close anyone else first: one character in memory at a time.
            for other, _, _ in CAST:
                o = L.find(_path(other))
                if o and other != name:
                    L.close(o)
            c = L.create_from_preset(_path(name), preset, False)
            L.save(c)
            return f"{name}: opened"
        s = L.status(c)
        if "rigging=1" in s or "requesting=1" in s:
            return f"{name}: waiting ({s})"
        if "rigged=0" in s:
            L.auto_rig(c, True)
            return f"{name}: rigging"
        if "textures=0" in s:
            L.request_textures(c, TEXTURES)
            return f"{name}: requesting textures"
        dirty = {p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()}
        if c.get_outermost().get_name() in dirty:
            L.save(c)
            return f"{name}: saved"
        if "buildable=1" not in s:
            return f"{name}: cannot build ({s})"
        if not L.assemble(c, BUILT_DIR, COMMON_DIR, QUALITY):
            return f"{name}: assembly failed"
        L.save(c)
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
        L.close(c)
        return f"{name}: assembled ({built_blueprint(name)})"
    return "done"
