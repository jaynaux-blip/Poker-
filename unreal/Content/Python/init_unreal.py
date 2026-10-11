"""Runs when the Unreal Editor opens this project: builds SHORT STACK's materials and map if missing."""
import os

import unreal

import shortstack_setup

_state = {"handle": None, "ticks": 0}


def _tick(_delta):
    # Wait until the editor has settled and the asset registry has finished its initial scan.
    _state["ticks"] += 1
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    if _state["ticks"] < 30 or registry.is_loading_assets():
        return
    unreal.unregister_slate_post_tick_callback(_state["handle"])
    try:
        shortstack_setup.run()
    except Exception as exc:  # never block the editor
        unreal.log_error(f"ShortStack setup failed: {exc}")


# A replay sandbox (E:/ShortStack/durable/segment/tools/replay/make_trees.sh: its own Binaries and Saved, the project's Content
# shared through a junction) never builds or imports anything: the Content it sees is this project's.
if os.path.exists(os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "REPLAY_SANDBOX.txt")):
    unreal.log("ShortStack setup skipped: a replay sandbox (REPLAY_SANDBOX.txt)")
else:
    _state["handle"] = unreal.register_slate_post_tick_callback(_tick)
