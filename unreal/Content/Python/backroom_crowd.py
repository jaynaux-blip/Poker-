"""
SHORT STACK: the card room's crowd, baked from the room's MetaHumans.

Run in Play In Editor at the Embercrest, once the near tables are seated:

    import backroom_crowd; backroom_crowd.bake()

The far tables' people are instanced static figures (ABackRoomStage's crowd kinds). bake() takes the extras seated at
the near tables (real MetaHumans, ABackRoomPlayer), each as it sits this frame, and bakes it into
/Game/ShortStack/Cast/Built/Crowd/SM_Crowd_MH_<kind> with UShortStackMetaHumanLibrary.bake_figure: its pose, its
clothes as dyed, what it wears, its hair cards. The game uses them where they exist and the crowd kit
(art/blender/assets/crowd.py) where they don't. Git-ignored, like the cast they're made from.
"""
import unreal

CROWD_DIR = "/Game/ShortStack/Cast/Built/Crowd"
INVISIBLE = "/Game/ShortStack/Materials/M_Invisible.M_Invisible"
HAIR_CARDS = "/Game/ShortStack/Materials/M_HairCardsStatic.M_HairCardsStatic"
# Crowd kind -> the extra who sits for it: the twelve seated kinds (0-5, 9-14) from the two near tables' players,
# the dealer (6) from the second near table's dealer.
SITTERS = {0: "Extra1", 1: "Extra2", 2: "Extra3", 3: "Extra4", 4: "Extra5", 5: "Extra6", 6: "Extra7",
           9: "Extra8", 10: "Extra9", 11: "Extra10", 12: "Extra11", 13: "Extra12", 14: "Extra13"}


def _extras(world):
    found = {}
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.BackRoomPlayer):
        found[actor.get_editor_property("Persona").get_editor_property("Name")] = actor
    return found


def _seated(actor):
    return actor is not None and not actor.get_editor_property("bHidden")


def _is_dealer(actor):
    """The house dresses its dealers: a plain black shirt (0x141418), nothing on the head."""
    p = actor.get_editor_property("Persona")
    shirt = p.get_editor_property("Shirt")
    return p.get_editor_property("Headwear") == 0 and max(shirt.r, shirt.g, shirt.b) < 0.012


def bake(lod=0, kinds=None):
    """Bakes each crowd kind from its sitter (all of them, or the kinds listed). Returns {kind: asset path}.

    SITTERS assumes both near tables seat six around their dealer (Extra0 and Extra7 dealing, Extra1-6 and Extra8-13
    playing, nobody past Extra13): bake() checks that and refuses otherwise, rather than bake a dealer as a player."""
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    if not world:
        unreal.log_error("ShortStack: bake the crowd in Play In Editor, at the Embercrest")
        return {}
    invisible = unreal.load_object(None, INVISIBLE)
    hair_cards = unreal.load_object(None, HAIR_CARDS)
    extras = _extras(world)
    if (not all(_seated(extras.get(f"Extra{i}")) for i in range(14)) or any(_seated(extras.get(f"Extra{i}")) for i in (14, 15))
            or not _is_dealer(extras["Extra0"]) or not _is_dealer(extras["Extra7"])):
        unreal.log_error("ShortStack: the near tables aren't two full six-seat tables with their dealers at Extra0 and Extra7; "
                         "bake the crowd at the start of a big field")
        return {}
    made = {}
    for kind, name in sorted(SITTERS.items()):
        if kinds is not None and kind not in kinds:
            continue
        actor = extras.get(name)
        if not actor or actor.is_hidden_ed() or actor.get_editor_property("bHidden"):
            unreal.log_warning(f"ShortStack: no {name} seated to bake crowd kind {kind}")
            continue
        mesh = unreal.ShortStackMetaHumanLibrary.bake_figure(actor, f"{CROWD_DIR}/Crowd_MH_{kind}", lod, invisible, hair_cards)
        if mesh:
            made[kind] = mesh.get_path_name()
            unreal.log(f"ShortStack: crowd kind {kind} baked from {name}: {mesh.get_path_name()}")
        else:
            unreal.log_error(f"ShortStack: baking crowd kind {kind} from {name} failed")
    return made
