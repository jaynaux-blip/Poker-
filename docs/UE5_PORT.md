# Moving SHORT STACK to Unreal Engine 5 and Blender

The browser prototype in `web/` proves the systems and the look of Night One. Production moves to **Unreal Engine 5** (gameplay, rendering) with **Blender** (modelling, UVs, texturing hand-off). This doc maps what exists to where it goes.

## 1. The engine: ported and verified

**Status: done.** `web/src/core/` is now also a C++ Unreal plugin, `unreal/Plugins/ShortStackCore`. It is plain C++17 with no engine types, so it builds inside Unreal and as a standalone CMake library. See the [plugin README](../unreal/Plugins/ShortStackCore/README.md) for install, test and API details.

| Prototype | C++ (`ShortStackCore`) | Notes |
|---|---|---|
| `rng.ts` | `ss::Rng` (sfc32) | Same algorithm, so a seed deals the same cards in both builds. |
| `cards.ts`, `evaluator.ts`, `equity.ts` | `ShortStack/Cards.h`, `Evaluator.h`, `Equity.h` | Same bitmask evaluator. The preflop tables are generated from the TypeScript source. |
| `hand.ts` | `ss::Hand` | Keeps the event list. Blueprints will subscribe to it to drive animation. |
| `tournament.ts`, `structure.ts`, `names.ts` | `ss::Tournament`, `Structure.h`, `Names.h` | Wrapped by `UShortStackTournamentSubsystem` (a `UGameInstanceSubsystem`) for Blueprints. |
| `ai/*` | `ShortStack/AI/*` (`ss::Decide`, `ss::MakeProfile`) | Archetype numbers can move into a `UDataTable` later so designers can tune them. |
| `ai/grading.ts` | `ss::AnalyzeDecision`, `ss::GradeDecision` | Feeds the results screen and the XP system. |

**Proof the port is correct:** `web/scripts/gen-golden.ts` runs the TypeScript engine on seeded inputs and writes 4,201 records to `Tests/golden_vectors.txt`. The records cover:

- RNG streams, evaluator results and equities, stored as exact double bit patterns
- 400 full hands and 160 bot-played hands with think times
- 129 graded decisions
- payouts and ICM
- every round of three tournaments, including a 1,000-player field

The C++ build replays all of them and must match bit for bit.

- **Standalone:** passes under GCC and Clang, C++17 and C++20, Debug and Release. `ctest` also runs a chip-conservation fuzz over 5,000 hands and a full 1,000-player tournament (about 0.7 s, roughly 4x faster than TypeScript).
- **Unreal:** the automation test `ShortStack.Core.GoldenVectors` runs the same vectors. The wrapper still needs its first compile inside Unreal on the desktop.

After any engine change in `web/`, run `npm run export:cpp` and bring the C++ side back to 0 mismatches.

## 2. Presentation

| Prototype | UE5 approach |
|---|---|
| RiverLine client on a canvas texture | UMG widget rendered with `UWidgetComponent` on the laptop screen mesh (world-space, interactive via `WidgetInteractionComponent`). |
| Apartment built from primitives | Blender set dressing exported as FBX/USD. Nanite for the room and props. Quixel Megascans for plaster, wood and fabric. |
| Rain-on-glass shader | Material with a droplet normal map flipbook plus a Niagara system for runner drops. Refraction comes from the scene color. Lumen handles the reflections. |
| Neon rain-shadow cookie | Rect light with a light function material fed by the same droplet texture. |
| Laptop as the key light | Emissive screen material plus a rect light. Lumen gives the soft bounce onto the desk and hands. |
| Post chain (bloom, grain, tilt vignette) | Post Process Volume plus a post-process material for tilt and heartbeat pulses, driven from gameplay. |
| Procedural audio | MetaSounds: rain bed, chip clacks, heartbeat. Swap in recorded Foley later. |

## 3. Blender pipeline

1. **Scale and orientation:**
   - Model in meters.
   - Keep the Blender default orientation (-Y forward).
   - Export FBX with "Apply Transform" on.
2. **Topology:**
   - Hero props (laptop, cans, mug, phone, chips, cards) get clean quad topology with bevels.
   - Nanite is fine for static set dressing.
3. **UVs and textures:**
   - Give every asset a second UV channel for lightmaps, in case you ever bake.
   - Texture in Substance or Blender. Pack ORM textures (occlusion, roughness, metallic) into one map.
4. **Naming:**
   - Use `SM_` for static meshes, `SK_` for skeletal meshes, `M_` and `MI_` for materials, `T_` for textures.
   - Match the prop list in `src/scene/props.ts` so the scene can be rebuilt one to one.
5. **Hands and characters:**
   - First-person arms can be a MetaHuman-based arm rig or custom arms from Blender.
   - Live-table opponents are MetaHumans, dressed per archetype.

## 4. Milestones in UE5

1. **Core port** (done outside Unreal): `ShortStackCore` passes the golden vectors. What remains is the first build inside UE5 and a green `ShortStack.Core.GoldenVectors` run.
2. **Night One scene** (written, needs its first build in UE5): `unreal/ShortStack.uproject`.
   - The apartment is built in C++ (`ANightOneStage`) and lit by Lumen: the laptop as key light, pink neon through the window, city fill and a blue sign.
   - The window shows rain-streaked glass, the skyline, the laundromat's neon and falling rain.
   - The lens has manual exposure, bloom, grain and a tilt treatment, with a heartbeat pulse during all-ins.
   - Materials are generated by `Content/Python/shortstack_setup.py`.
3. **Playable table** (written; the engine-agnostic parts are tested):
   - The RiverLine client runs on the laptop: lobby, a full tournament, graded decisions, sprint mode and the results screen.
   - The phone lights up with story texts, and progress is saved.
   - The session, the UI and the sound synthesis are plain C++ in the plugin. Standalone tests play whole tournaments through them, and the UI screens were rendered and compared against the prototype.
4. **Vertical slice 2, "The Back Room":** the laundromat live cash game. This is where UE5 earns its keep: MetaHuman opponents, physical chips and cards, tells and composure.

See `unreal/README.md` for opening, building and playing.
