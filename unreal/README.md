# SHORT STACK in Unreal Engine 5

This folder is the Unreal project for Night One. It contains:

- the seated first-person view in the rainy apartment
- the RiverLine poker client on the laptop, playable from the lobby through a full tournament to the results screen
- a living poker network behind the lobby: a round-the-clock schedule, series, 1,600 regulars, leaderboards, news and your career page
- phone texts from Dee and the landlord
- the composure and tilt effects
- procedural sound
- saved progress

Everything is text in git: C++, config, and a Python script that generates the materials and the map the first time the editor opens.

**Status:** everything that can run outside Unreal has been tested here. That covers the game session, the laptop UI, the audio synthesis and the poker engine. The Unreal code compiles with UE 5.6 and Visual Studio 2022; the first full build and play-through are in progress. If a build fails, run `BuildLog.bat` and paste the errors to Claude.

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
   - If Unreal says "ShortStack could not be compiled", double-click `BuildLog.bat` in this folder. It builds the project, lists the first errors and saves everything to `build_log.txt`.
3. On the first launch, the Output Log shows `ShortStack: built ...` for each material, then `ShortStack: created /Game/Maps/NightOne`. Shader compilation takes a while the first time, longer with hardware ray tracing on (see Rendering below).
4. Press **Play** (Alt+P). The title screen appears over the room. Press any key, then choose **Continue** or **New Game**.
   - Play-In-Editor keeps Escape for stopping the session, so use **P** to pause. In a standalone game (**Play > Standalone Game**) or a packaged build, Escape pauses too.

**Controls:**

| Input | Action |
|---|---|
| Mouse | Play on the laptop screen |
| Space | Lean back and look around by pointing; click the laptop to lean back in |
| F · C · R · A | Fold, call or check, raise, all-in |
| Up / Down | Bet size |
| M | Mute |
| P · Esc · gamepad Start | Pause menu |

The menus work with the mouse, the keyboard (arrows, Enter, Escape, Q/E for settings tabs) or a gamepad (D-pad or left stick, A, B, LB/RB).

## Menus and settings

- **Title screen:** the logo over a slow establishing shot of the room; press any key.
- **Main menu:** Continue (your career: bankroll, events played, best finish), New Game (pick a screen name; replaces the current career), Settings, Credits, Quit.
- **Pause menu:** Resume, Settings, Quit to Main Menu, Quit to Desktop, with the night so far (bankroll, the event, players left, your rank and stack).
- **Settings** apply as you change them and are saved to the `Settings` save slot:
  - Graphics: quality preset (Low to Cinematic), ray-traced lighting, resolution scale, frame rate limit, V-Sync, motion blur, film grain, chromatic aberration.
  - Display: window mode, resolution, brightness, field of view.
  - Audio: master, effects, ambience.
  - Controls: look sensitivity, invert look, control hints.

The menus are drawn by `ss::ui::FrontEnd` in ShortStackCore (like the laptop client), so every page can be rendered and checked without Unreal: `ui_test <dir>` writes them as `menu_*.json`, and `BG=<image> node web/scripts/render-drawlists.mjs <dir>` turns them into PNGs over a backdrop.

## Rendering

The project targets DirectX 12 with Shader Model 6 and turns on hardware ray tracing (`r.RayTracing`). On an RTX-class GPU, Lumen then traces the real scene geometry for global illumination and reflections, so the neon and the laptop light the room exactly. Virtual shadow maps, Nanite and temporal super resolution are on as well. The defaults are Epic quality with ray-traced lighting, which suits an RTX 4070-class GPU at 1440p. The Cinematic preset adds ray-traced hit lighting for reflections.

The first editor launch after ray tracing is enabled recompiles the engine's shaders. It can take 10 to 30 minutes, once.

The game builds the apartment at runtime, so it also runs in any other level. If the map was not created, the set is spawned at the origin.

## If something goes wrong

- **"ShortStack could not be compiled":** close the editor and double-click `BuildLog.bat`. Give Claude the errors it prints, or attach `build_log.txt`.
  - If it reports that Visual Studio's C++ compiler was not found, install Visual Studio 2022 Community with the **Game development with C++** workload. In the workload's details, include **Unreal Engine installer** and a **Windows 11 SDK**. Then run `BuildLog.bat` again.
  - If the engine is not found automatically, pass its folder: `BuildLog.bat "D:\Epic Games\UE_5.5"`.
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
  SFrontEndWidget           the title screen and menus (draws ss::ui::FrontEnd, takes keyboard, mouse and gamepad input)
  SNightOneOverlay          phone notifications and the controls hint
  NightOneSaveGame          bankroll, results and story flags
Plugins/ShortStackCore/     everything engine-agnostic, tested without Unreal (see its README)
  ShortStack/*              poker engine, bots, grading, tournaments
  ShortStack/Game           the Night One session, lobby, chat, the RiverLine network (schedule, series, players, boards, news)
  ShortStack/UI             vector canvas, RiverLine client, menus (FrontEnd), card art, phone, printed props
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

## Blender assets

Props are modeled, textured and reviewed in Blender by the scripts in `art/blender` (see `art/README.md`). They are exported to `unreal/Art/Meshes/*.glb`. On editor launch, `shortstack_setup.py` imports each file into `/Game/ShortStack/Meshes/<SM_Name>/`, and `NightOneStage` uses the imported mesh in place of its engine-shape stand-in. So far that covers:

- the energy drink cans (`SM_EnergyCan`);
- the laptop (`SM_Laptop_Base` and `SM_Laptop_Lid`). The stage hinges the lid at 18.5 mm and tilts it back 18.3°, and it mounts the RiverLine screen just in front of the lid's glass. Its keyboard, legends and backlight are part of the mesh.
- the desk, chair, desk lamp, mug, phone and poker chips (`SM_Desk`, `SM_Chair`, `SM_DeskLamp`, `SM_Mug`, `SM_Phone`, `SM_ChipStacks`). The phone's lock screen lies on its cracked glass.

- the mouse and mousepad (`SM_Mouse`, `SM_MousePad`). The mouse slides across the pad as you move the pointer;
- the player's arms (`SK_Arms`, a skeletal mesh). The setup script rebuilds their skin material with Unreal's Subsurface Profile shading (`M_SK_ArmsSkin`), which glTF can't carry.

**First-person arms** (`FirstPersonArms.cpp`, driven from `NightOneStage::UpdateArms`):
- The left hand rests on the laptop's palm rest. On each hotkey it reaches over and taps the key with the finger a touch typist would use (F and R index, C middle, X ring, A pinky, Space thumb).
- The right hand holds the mouse, which follows the pointer. It clicks with the index finger, and reaches over for the arrows and M.
- Two-bone IK places the wrists; the hands and fingers pose from the mesh's reference pose. The shoulders follow the head when you lean in.
- The arms hide during the title screen's establishing shot.

Blender props are modeled facing -Y. The stage takes the importer's orientation from the laptop base's bounds and turns every prop by that same yaw.

To swap in a hand-made model instead:

1. Export FBX or glTF in meters with **Apply Transform** on.
2. Import it under `/Game/ShortStack/Meshes`.
3. Replace the matching `BoxWeb`, `CylinderWeb` or `AddMesh` call in `NightOneStage.cpp` with `AddMesh(YourMesh, Material, Location, Scale, Rotation)`.

Positions in `NightOneStage.cpp` are written in the prototype's meters through `Web(x, y, z)`, which maps to Unreal centimeters as X = -z, Y = x, Z = y. Naming follows `docs/UE5_PORT.md`: `SM_`, `M_`, `MI_`, `T_`.

## Differences from the browser prototype

- The window glass is a fogged, rain-streaked translucent layer with clear trails. It does not refract the city yet.
- Steam from the mug and the dust motes are not ported yet. Niagara is the natural fit for both.
- The can and cup labels are plain colors, and the eviction notice's stamp is square to the page.
- Text uses Unreal's Roboto instead of the browser's Inter or Arial, so some widths differ slightly.
