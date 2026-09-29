# Moving SHORT STACK to Unreal Engine 5 and Blender

The browser prototype in `web/` proves the systems and the look of Night One. Production moves to **Unreal Engine 5** (gameplay, rendering) with **Blender** (modelling, UVs, texturing hand-off). This doc maps what exists to where it goes.

## 1. The engine code ports almost line for line

Everything in `web/src/core/` is plain logic with no rendering or DOM. It becomes a C++ runtime module, `ShortStackCore`. It uses no engine types, so it stays unit-testable outside the editor.

| Prototype | UE5 target | Notes |
|---|---|---|
| `rng.ts` | `FSSRng` (sfc32) | Keep the algorithm identical so seeds reproduce the same hands across both builds. |
| `evaluator.ts`, `equity.ts` | `FHandEvaluator`, `FEquity` | Same bit-mask evaluator. Run Monte Carlo on a worker thread (`UE::Tasks`). |
| `hand.ts` | `FHoldemHand` | Keep the event list. Blueprints subscribe to events to drive animation. |
| `tournament.ts`, `structure.ts` | `FTournament` in a `UGameInstanceSubsystem` | Simulate off-screen tables on a background task. |
| `ai/*` | `FBotBrain` and archetype data | Move archetype numbers into a `UDataTable` so designers can tune them. |
| `ai/grading.ts` | `FDecisionGrader` | Feeds the results screen and the XP system. |

**Porting safety net:** export golden test vectors from the TypeScript tests (seeded hands, evaluator results, side-pot outcomes) as JSON. Run the same vectors against the C++ build with UE's Automation Tests. When both agree, the port is correct.

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

## 4. First milestones in UE5

1. **Core port:** `ShortStackCore` compiles and passes the golden vectors.
2. **Night One scene:** the apartment in UE5 with Lumen, the rain window and the laptop widget running the lobby.
3. **Playable table:** a full tournament through the UMG client, graded decisions and the results screen.
4. **Vertical slice 2, "The Back Room":** the laundromat live cash game. This is where UE5 earns its keep: MetaHuman opponents, physical chips and cards, tells and composure.
