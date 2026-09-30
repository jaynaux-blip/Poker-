"""Runs when the Unreal Editor opens this project: builds SHORT STACK's materials and map if missing."""
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


_state["handle"] = unreal.register_slate_post_tick_callback(_tick)
