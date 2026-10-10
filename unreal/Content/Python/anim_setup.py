"""
SHORT STACK: the street's mocap (design spec 6.6), baked from Epic's Game Animation Sample onto the MetaHuman body skeleton.

The sample's clips live in /Game/Characters/UEFN_Mannequin (migrated from the GameAnimationSample project, 911 assets, about
1.1 GB, with the motion-matching databases the walks drag in) on the UEFN mannequin. They are editor-side sources only
(spec 0.8): nothing the game loads may reference them. This bakes the clips the street uses, retargeted with the sample's
own retargeters (RTG_UEFN_to_Metahuman_nrw for the standard bodies, _ovw for the heavy build; IK_Metahuman_Retarget), onto
the cast's MetaHuman body skeleton (/Game/ShortStack/Cast/Common/.../metahuman_base_skel: every cast and hero body shares
it), as new sequences the game loads by path:

    /Game/ShortStack/Anim/Mocap/AS_NS_<Clip>          the standard set (RTG_UEFN_to_Metahuman_nrw)
    /Game/ShortStack/Anim/Mocap/AS_NS_<Clip>_Ovw      the heavy set (RTG_UEFN_to_Metahuman_ovw), only when a heavy body is built

<Clip> is StandIdle, IdleBreak1..6, Turn045L/R, Turn090L/R, Turn135L/R, Turn180L/R, WalkLoop, WalkStartL/R,
WalkStopL/R, RunLoop, RunStartL/R and RunStopL/R (StreetAnim.cpp's EStreetClip, in the same order). UStreetMocap loads
them (StreetAnim.h); what isn't baked is simply missing, and a body without its stand idle and walk loop walks on the
procedural gait as before. The game keeps each body's own bone lengths and scales the clips' pelvis to its hip height, so
one bake serves every height; the heavy set is for bodies deep for their height (ss.Anim.HeavyDepth, 0.225 of it).

Each baked clip is cleaned so it references nothing outside /Game/ShortStack (the cook would drag the sample in): no
notifies, no curves, no asset user data, no retarget-source mesh, no preview mesh but the cast body it was baked on.
verify() lists every dependency and the root motion the game reads (the turns must turn their root, the loops must travel).

Run it in the editor's Python (the lead's session, spec 0.9), after the cast is built (backroom_cast.py, hero_cast.py):

    import anim_setup
    anim_setup.status()               # the sources, the retargeters, the target bodies, what's baked
    anim_setup.run()                  # bake what's missing (the standard set; the heavy one too if a heavy body is built)
    anim_setup.run(force=True)        # bake everything again
    anim_setup.run(sets=("std",))     # only the standard set
    anim_setup.verify()               # root motion and references of every baked clip
    anim_setup.prune_report()         # what in the sample's folders the game doesn't use (Phase C prunes; nothing is deleted)

Nothing is committed (the storage decision is pending, spec 11.6): the baked clips are about 0.5-1 MB each.
"""
import os

import unreal

eal = unreal.EditorAssetLibrary

SRC = "/Game/Characters/UEFN_Mannequin"
SRC_ANIM = f"{SRC}/Animations"
SRC_MESH = f"{SRC}/Meshes/SKM_UEFN_Mannequin"
RIGS = "/Game/MetaHumans/Common/Common/Rigs"
RTG = {"std": f"{RIGS}/RTG_UEFN_to_Metahuman_nrw", "heavy": f"{RIGS}/RTG_UEFN_to_Metahuman_ovw"}
SUFFIX = {"std": "", "heavy": "_Ovw"}
OUT_DIR = "/Game/ShortStack/Anim/Mocap"
BUILT = "/Game/ShortStack/Cast/Built"
# The batch retarget writes its duplicates to /Game (its name rule's folder, from Python): they're moved from there at once.
TMP_DIR = "/Game"
TMP_PREFIX = "TMPNS_"

# (the game's clip name, the sample's clip under Animations/), in StreetAnim.cpp's EStreetClip order.
CLIPS = [
    ("StandIdle", "Idle/M_Neutral_Stand_Idle_Loop"),
    ("IdleBreak1", "Idle/M_Neutral_Stand_Idle_Break_v01"),
    ("IdleBreak2", "Idle/M_Neutral_Stand_Idle_Break_v02"),
    ("IdleBreak3", "Idle/M_Neutral_Stand_Idle_Break_v03"),
    ("IdleBreak4", "Idle/M_Neutral_Stand_Idle_Break_v04"),
    ("IdleBreak5", "Idle/M_Neutral_Stand_Idle_Break_v05"),
    ("IdleBreak6", "Idle/M_Neutral_Stand_Idle_Break_v06"),
    ("Turn045L", "Idle/M_Neutral_Stand_Turn_045_L"),
    ("Turn045R", "Idle/M_Neutral_Stand_Turn_045_R"),
    ("Turn090L", "Idle/M_Neutral_Stand_Turn_090_L"),
    ("Turn090R", "Idle/M_Neutral_Stand_Turn_090_R"),
    ("Turn135L", "Idle/M_Neutral_Stand_Turn_135_L"),
    ("Turn135R", "Idle/M_Neutral_Stand_Turn_135_R"),
    ("Turn180L", "Idle/M_Neutral_Stand_Turn_180_L"),
    ("Turn180R", "Idle/M_Neutral_Stand_Turn_180_R"),
    ("WalkLoop", "Walk/M_Neutral_Walk_Loop_F"),
    ("WalkStartL", "Walk/M_Neutral_Walk_Start_F_Lfoot"),
    ("WalkStartR", "Walk/M_Neutral_Walk_Start_F_Rfoot"),
    ("WalkStopL", "Walk/M_Neutral_Walk_Stop_F_Lfoot"),
    ("WalkStopR", "Walk/M_Neutral_Walk_Stop_F_Rfoot"),
    ("RunLoop", "Run/M_Neutral_Run_Loop_F"),
    ("RunStartL", "Run/M_Neutral_Run_Start_F_Lfoot"),
    ("RunStartR", "Run/M_Neutral_Run_Start_F_Rfoot"),
    ("RunStopL", "Run/M_Neutral_Run_Stop_F_Lfoot"),
    ("RunStopR", "Run/M_Neutral_Run_Stop_F_Rfoot"),
]
# The bodies the standard set is baked onto, in order of preference (standard men's proportions; any cast body would do:
# the game keeps each body's own bone lengths).
STANDARD_TARGETS = ("HeroA1", "HeroA0", "HeroA2", "ExtraD", "ExtraB", "Hero")
# A body this deep (front to back) for its height or deeper is the heavy build's (StreetAnim.cpp's ss.Anim.HeavyDepth).
# Off (1.0) for now: measured on the built bodies (2026-10-07) every one reads 0.255-0.281, Big Lou and the slim heroes
# alike, so the bounds can't tell an overweight body; the heavy set waits for a real body-type test.
HEAVY_DEPTH = 1.0
# The nominal turns, for verify().
TURNS = {"Turn045L": -45, "Turn045R": 45, "Turn090L": -90, "Turn090R": 90, "Turn135L": -135, "Turn135R": 135, "Turn180L": -180, "Turn180R": 180}


def _log(msg):
    unreal.log(f"anim_setup: {msg}")


def _warn(msg):
    unreal.log_warning(f"anim_setup: {msg}")


def _name(clip, which):
    return f"AS_NS_{clip}{SUFFIX[which]}"


def _dest(clip, which):
    return f"{OUT_DIR}/{_name(clip, which)}"


def _src(rel):
    return f"{SRC_ANIM}/{rel}"


def _body_mesh(cast):
    return f"{BUILT}/MHC_{cast}/Body/SKM_MHC_{cast}_BodyMesh"


def _built_bodies():
    """Every cast body mesh that's built: {name: path}."""
    out = {}
    for path in eal.list_assets(BUILT, recursive=True, include_folder=False):
        base = path.split(".")[0]
        leaf = base.rsplit("/", 1)[-1]
        if leaf.startswith("SKM_MHC_") and leaf.endswith("_BodyMesh"):
            out[leaf[len("SKM_MHC_"):-len("_BodyMesh")]] = base
    return out


def _depth_ratio(mesh):
    """How deep the body is for its height (front to back: MetaHuman bodies face +Y), from its bounds."""
    bounds = None
    for getter in ("get_bounds", "get_imported_bounds"):
        try:
            bounds = getattr(mesh, getter)()
            break
        except Exception:
            continue
    if bounds is None:
        return 0.0
    ext = bounds.box_extent
    return (ext.y * 2.0) / max(1.0, ext.z * 2.0)


def targets():
    """The body each set is baked onto: {"std": path, "heavy": path or None}, and every built body's depth ratio."""
    bodies = _built_bodies()
    ratios = {}
    for name, path in bodies.items():
        mesh = unreal.load_asset(path)
        if mesh:
            ratios[name] = _depth_ratio(mesh)
    std = None
    for name in STANDARD_TARGETS:
        if name in bodies:
            std = bodies[name]
            break
    if std is None and bodies:
        # The most ordinary body there is: the median depth.
        ordered = sorted(ratios, key=lambda n: ratios[n])
        std = bodies[ordered[len(ordered) // 2]]
    heavy = None
    deep = [n for n in ratios if ratios[n] >= HEAVY_DEPTH]
    if deep:
        heavy = bodies[max(deep, key=lambda n: ratios[n])]
    return {"std": std, "heavy": heavy}, ratios


def _clean(seq):
    """Nothing in a baked clip may point outside /Game/ShortStack: notifies, curves, user data, the source mesh go. The
    sync markers stay (names, not references: the game reads the feet's to pick a stop's foot, UStreetMocap::Load), put
    back on the one notify track remove_all_animation_notify_tracks leaves (it empties the markers with the notifies)."""
    lib = unreal.AnimationLibrary
    markers = []
    try:
        markers = [(m.get_editor_property("marker_name"), m.get_editor_property("time")) for m in (lib.get_animation_sync_markers(seq) or [])]
    except Exception as exc:
        _warn(f"{seq.get_name()}: couldn't read its sync markers ({exc})")
    for call in ("remove_all_animation_notify_tracks", "remove_all_curve_data"):
        try:
            getattr(lib, call)(seq)
        except Exception as exc:
            _warn(f"{seq.get_name()}: {call} failed ({exc})")
    if markers:
        try:
            tracks = list(lib.get_animation_notify_track_names(seq) or [])
            if not tracks:
                lib.add_animation_notify_track(seq, unreal.Name("Sync"), unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
                tracks = [unreal.Name("Sync")]
            for name, time in markers:
                lib.add_animation_sync_marker(seq, name, time, tracks[0])
        except Exception as exc:
            _warn(f"{seq.get_name()}: couldn't put its {len(markers)} sync markers back ({exc})")
    for prop, value in (("asset_user_data", []), ("retarget_source_asset", None), ("retarget_source", "None"), ("enable_root_motion", False),
                        ("force_root_lock", False)):
        try:
            if prop == "retarget_source":
                seq.set_editor_property(prop, unreal.Name("None"))
            else:
                seq.set_editor_property(prop, value)
        except Exception:
            pass  # not every version has every property


def _delete_temporaries():
    for path in eal.list_assets(TMP_DIR, recursive=False, include_folder=False):
        leaf = path.split(".")[0].rsplit("/", 1)[-1]
        if leaf.startswith(TMP_PREFIX):
            eal.delete_asset(path.split(".")[0])


def bake_set(which, target_path, force=False):
    """Retargets and bakes one set onto target_path's skeleton; returns the clips baked."""
    rtg = unreal.load_asset(RTG[which])
    src_mesh = unreal.load_asset(SRC_MESH)
    target = unreal.load_asset(target_path) if target_path else None
    if not rtg or not src_mesh or not target:
        _warn(f"{which}: missing {'the retargeter ' + RTG[which] if not rtg else ''}{' the mannequin ' + SRC_MESH if not src_mesh else ''}"
              f"{' the target body ' + str(target_path) if not target else ''}: nothing baked")
        return []
    todo = []
    for clip, rel in CLIPS:
        dest = _dest(clip, which)
        if eal.does_asset_exist(dest) and not force:
            continue
        src = _src(rel)
        if not eal.does_asset_exist(src):
            _warn(f"{clip}: the sample's {src} isn't in the project (migrate it from the GameAnimationSample project)")
            continue
        todo.append((clip, src))
    if not todo:
        _log(f"{which}: every clip is baked already")
        return []
    _delete_temporaries()
    prefix = f"{TMP_PREFIX}{which}_"
    datas = [eal.find_asset_data(src) for _, src in todo]
    _log(f"{which}: retargeting {len(todo)} clips with {rtg.get_name()} onto {target.get_name()} ({target.get_editor_property('skeleton').get_path_name()})")
    made = unreal.IKRetargetBatchOperation.duplicate_and_retarget(datas, src_mesh, target, rtg, "", "", prefix, "", False) or []
    # Where each duplicate went (the batch's name rule: /Game/<prefix><leaf>, with a number added if that was taken), by the
    # source clip's leaf name.
    made_by_leaf = {}
    for data in made:
        path = str(data.package_name)
        leaf = path.rsplit("/", 1)[-1]
        if leaf.startswith(prefix):
            made_by_leaf.setdefault(leaf[len(prefix):], path)
    eal.make_directory(OUT_DIR)
    baked = []
    for clip, src in todo:
        leaf = src.rsplit("/", 1)[-1]
        tmp = made_by_leaf.get(leaf, f"{TMP_DIR}/{prefix}{leaf}")
        if tmp not in made_by_leaf.values():
            # A number on the end (a leftover of a cancelled run took the plain name): the closest duplicate of this leaf.
            for name, path in made_by_leaf.items():
                if name.startswith(leaf):
                    tmp = path
                    break
        dest = _dest(clip, which)
        if not eal.does_asset_exist(tmp):
            _warn(f"{clip}: the retarget didn't make {tmp}")
            continue
        if eal.does_asset_exist(dest):
            eal.delete_asset(dest)
        if not eal.rename_asset(tmp, dest):
            _warn(f"{clip}: couldn't move {tmp} to {dest}")
            continue
        seq = unreal.load_asset(dest)
        if seq:
            _clean(seq)
            eal.save_asset(dest, only_if_is_dirty=False)
            baked.append(clip)
    _delete_temporaries()
    _log(f"{which}: baked {len(baked)} of {len(todo)}: {', '.join(baked)}")
    return baked


def run(force=False, sets=("std", "heavy")):
    """Bakes what's missing (or everything with force). The heavy set only when a heavy body is built."""
    picks, ratios = targets()
    _log("the cast's bodies, deep for their height: " + ", ".join(f"{n} {r:.3f}" for n, r in sorted(ratios.items(), key=lambda kv: kv[1])))
    baked = {}
    for which in sets:
        target = picks.get(which)
        if which == "heavy" and not target:
            _log(f"no heavy body is built (none at {HEAVY_DEPTH} of its height or deeper): the heavy set isn't baked; every body takes the standard one")
            continue
        if not target:
            _warn("no cast body is built to bake onto (backroom_cast.py, hero_cast.py first)")
            continue
        baked[which] = bake_set(which, target, force)
    verify()
    return baked


def _bone_pose(seq, bone, t):
    try:
        return unreal.AnimationLibrary.get_bone_pose_for_time(seq, bone, t, False)
    except Exception:
        return None


def verify():
    """Every baked clip: its length, its root's travel and turn (what the game reads), and anything it references outside
    /Game/ShortStack (there must be nothing: the cook would bring it along)."""
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    opts = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True,
                                                 include_searchable_names=False, include_soft_management_references=False,
                                                 include_hard_management_references=False)
    problems = 0
    total = 0
    for which in ("std", "heavy"):
        for clip, _ in CLIPS:
            dest = _dest(clip, which)
            if not eal.does_asset_exist(dest):
                continue
            seq = unreal.load_asset(dest)
            length = seq.get_play_length()
            line = f"{_name(clip, which)}: {length:.2f} s"
            first = _bone_pose(seq, "root", 0.0)
            last = _bone_pose(seq, "root", length)
            if first and last:
                travel = (last.translation - first.translation).length()
                # The turn as the sum of small steps (a 180 reads back across the seam otherwise).
                yaw, prev, steps = 0.0, first.rotation.rotator().yaw, 16
                for k in range(1, steps + 1):
                    now = _bone_pose(seq, "root", length * k / steps)
                    if now:
                        d = (now.rotation.rotator().yaw - prev + 540.0) % 360.0 - 180.0
                        yaw += d
                        prev = now.rotation.rotator().yaw
                line += f", root {travel:.0f} cm, {yaw:+.0f} deg"
                if clip in TURNS and abs(yaw) < abs(TURNS[clip]) * 0.5:
                    line += "  <- the turn isn't in the root: the game won't play the turns (ss.Anim.Turns)"
                    problems += 1
                if clip in ("WalkLoop", "RunLoop") and travel < 20.0:
                    line += "  <- the loop's root doesn't travel: the game matches its rate to the sample's pace instead"
            if clip in ("WalkLoop", "RunLoop"):
                try:
                    names = [str(m.get_editor_property("marker_name")) for m in (unreal.AnimationLibrary.get_animation_sync_markers(seq) or [])]
                    line += f", sync markers: {', '.join(names) if names else 'none (stops start on the left foot at the cycle start)'}"
                except Exception:
                    pass
            deps = registry.get_dependencies(dest.rsplit(".", 1)[0], opts) or []
            outside = [str(d) for d in deps if not (str(d).startswith("/Game/ShortStack") or str(d).startswith("/Script") or str(d).startswith("/Engine"))]
            if outside:
                line += f"  <- references outside /Game/ShortStack: {', '.join(outside)}"
                problems += 1
            _log(line)
            total += 1
    size = 0
    folder = os.path.join(unreal.Paths.project_content_dir(), "ShortStack", "Anim", "Mocap")
    if os.path.isdir(folder):
        for f in os.listdir(folder):
            size += os.path.getsize(os.path.join(folder, f))
    _log(f"{total} clips baked ({size / 1024.0 / 1024.0:.1f} MB), {problems} problem(s)")
    return problems


def status():
    """The sources, the retargeters, the bodies they'd bake onto, and what's baked."""
    missing_src = [rel for _, rel in CLIPS if not eal.does_asset_exist(_src(rel))]
    _log(f"the sample: {SRC} {'found' if eal.does_directory_exist(SRC) else 'MISSING'}; {len(CLIPS) - len(missing_src)}/{len(CLIPS)} clips"
         + (f" (missing: {', '.join(missing_src)})" if missing_src else ""))
    for which, path in RTG.items():
        _log(f"{which}: {path} {'found' if eal.does_asset_exist(path) else 'MISSING'}")
    picks, ratios = targets()
    _log(f"targets: standard {picks['std']}, heavy {picks['heavy'] or '(none built)'}")
    # Each body's depth for its height: the game gives the heavy set to those at HEAVY_DEPTH or over (ss.Anim.HeavyDepth,
    # the same number): check the split falls between the normal-weight bodies and the heavy ones.
    _log(f"depth for height (heavy at {HEAVY_DEPTH}): " + ", ".join(f"{n} {r:.3f}{' HEAVY' if r >= HEAVY_DEPTH else ''}"
                                                                    for n, r in sorted(ratios.items(), key=lambda kv: kv[1])))
    for which in ("std", "heavy"):
        done = [clip for clip, _ in CLIPS if eal.does_asset_exist(_dest(clip, which))]
        _log(f"{which}: {len(done)}/{len(CLIPS)} baked")


def prune_report():
    """What in the sample's migrated folders the game doesn't use (for Phase C's prune; nothing is deleted here)."""
    used = {_src(rel) for _, rel in CLIPS}
    unused = 0
    size = 0
    root = os.path.join(unreal.Paths.project_content_dir(), "Characters", "UEFN_Mannequin")
    for path in eal.list_assets(SRC, recursive=True, include_folder=False):
        base = path.split(".")[0]
        if base in used:
            continue
        unused += 1
        rel = base[len(SRC) + 1:] + ".uasset"
        full = os.path.join(root, *rel.split("/"))
        if os.path.isfile(full):
            size += os.path.getsize(full)
    _log(f"{unused} assets under {SRC} aren't baked from ({size / 1024.0 / 1024.0:.0f} MB); once the bake is verified, the whole "
         f"folder (and /Game/MetaHumans/Common) can go in Phase C: the game references neither")
