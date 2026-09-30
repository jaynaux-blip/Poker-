# SHORT STACK in Unreal Engine 5

This folder is the Unreal project for Night One. It contains:

- the seated first-person view in the rainy apartment
- the RiverLine poker client on the laptop, playable from the lobby through a full tournament to the results screen
- phone texts from Dee and the landlord
- the composure and tilt effects
- procedural sound
- saved progress

Everything is text in git: C++, config, and a Python script that generates the materials and the map the first time the editor opens.

**Status:** everything that can run outside Unreal has been tested here. That covers the game session, the laptop UI, the audio synthesis and the poker engine. The Unreal-specific code is written for UE 5.3 or later but has **not been compiled inside Unreal yet**. Expect the first build to report a few API mismatches. Paste the errors to Claude and they get fixed.

## Get the project onto your computer

```
git clone https://github.com/jaynaux-blip/Poker-.git
cd Poker-
git checkout claude/poker-rpg-brainstorm-026wk4
```

To update later, run `git pull`.

## Open it

You need Unreal Engine 5.3 or later from the Epic Games Launcher. You also need Visual Studio 2022 with the **Game development with C++** workload. Rider also works.

1. Right-click `unreal/ShortStack.uproject` and choose **Switch Unreal Engine version**, then pick your installed version. This also generates the Visual Studio solution.
2. Double-click `ShortStack.uproject`. When asked to rebuild the missing `ShortStack` and `ShortStackCore` modules, click **Yes**.
   - If the rebuild fails, open `ShortStack.sln` and build **Development Editor / Win64** to see the errors.
3. On the first launch, the Output Log shows `ShortStack: built ...` for each material, then `ShortStack: created /Game/Maps/NightOne`. Shader compilation takes a few minutes the first time.
4. Press **Play** (Alt+P). Type a screen name and press **Begin**.

**Controls:**

| Input | Action |
|---|---|
| Mouse | Play on the laptop screen |
| Space | Lean back and look around by pointing; click the laptop to lean back in |
| F · C · R · A | Fold, call or check, raise, all-in |
| Up / Down | Bet size |
| M | Mute |

The game builds the apartment at runtime, so it also runs in any other level. If the map was not created, the set is spawned at the origin.

## If something goes wrong

- **Build errors:** copy the first errors from Visual Studio's Error List or the Output Log and give them to Claude.
  - On UE 5.0, delete the `FPSemantics` line in `Plugins/ShortStackCore/Source/ShortStackCore/ShortStackCore.Build.cs`.
- **Materials or map missing:** make sure **Edit > Plugins > Python Editor Script Plugin** is enabled. Then run this in the Output Log's Python console:
  ```
  import shortstack_setup; shortstack_setup.run(force=True)
  ```
- **Engine tests:** open **Tools > Test Automation** and run `ShortStack.Core.GoldenVectors`. It should report 4,201 passed. It proves the C++ poker engine matches the browser prototype bit for bit.

## How it fits together

```
ShortStack.uproject
Config/                     renderer (Lumen, virtual shadow maps), maps and game mode, input, packaging
Content/Python/             init_unreal.py + shortstack_setup.py: materials and the NightOne map
Source/ShortStack/          the game module (Unreal side)
  NightOneGameMode          boots everything, runs the session each frame, routes input (port of web/src/main.ts)
  NightOneGame              the session's hooks into the world: sounds, phone texts, cans, saves
  NightOneStage             the apartment, props, window, city, neon, lights and lens, built from engine shapes
  NightOnePawn              seated camera: leaned in on the laptop, or sitting back looking around
  NightOnePlayerController  mouse ray onto the laptop screen, clicks, wheel, hotkeys
  SlateDrawList             draws ShortStackCore draw lists with Slate (laptop, phone, printed props, neon sign)
  NightOneAudio             plays the synthesized sounds and streams the room ambience
  SNightOneOverlay          title card, phone notifications and the controls hint
  NightOneSaveGame          bankroll, results and story flags
Plugins/ShortStackCore/     everything engine-agnostic, tested without Unreal (see its README)
  ShortStack/*              poker engine, bots, grading, tournaments
  ShortStack/Game           the Night One session, lobby, chat
  ShortStack/UI             vector canvas, RiverLine client, card art, phone, printed props
  ShortStack/Audio          sound synthesis
```

The laptop client is not UMG. The C++ UI draws each frame into a draw list of anti-aliased triangles and text runs. A Slate widget replays that list on a widget component on the laptop screen. The browser test tool replays the same lists, which is how the UI was checked without Unreal.

## Tuning

Select the `NightOneStage` actor in the NightOne map to change:

- `ExposureBias`
- `ScreenLightCandela` (the laptop as key light)
- `NeonCandela`
- `RoomFillCandela`
- `ScreenResolution` (laptop client sharpness)

On the game mode, `TimeScale` speeds up the game clock for testing.

## Replacing placeholder geometry with Blender assets

Each piece of the set is a component created in `NightOneStage.cpp`: `BuildShell`, `BuildWindow`, `BuildDesk`, `BuildProps` and `BuildOutside`. Positions are written in the prototype's meters through `Web(x, y, z)`, which maps to Unreal centimeters as X = -z, Y = x, Z = y.

To swap in a Blender model:

1. Export FBX in meters with **Apply Transform** on.
2. Import it under `/Game/ShortStack/Meshes`.
3. Replace the matching `BoxWeb`, `CylinderWeb` or `AddMesh` call with `AddMesh(YourMesh, Material, Location, Scale, Rotation)`.

Naming follows `docs/UE5_PORT.md`: `SM_`, `M_`, `MI_`, `T_`.

## Differences from the browser prototype

- The window glass is a fogged, rain-streaked translucent layer with clear trails. It does not refract the city yet.
- Steam from the mug and the dust motes are not ported yet. Niagara is the natural fit for both.
- The can and cup labels are plain colors, and the eviction notice's stamp is square to the page.
- Text uses Unreal's Roboto instead of the browser's Inter or Arial, so some widths differ slightly.
