# ShortStackCore (Unreal plugin)

The SHORT STACK poker engine in C++. It is a port of `web/src/core/` from the browser prototype and covers:

- No-Limit Hold'em rules with side pots
- the hand evaluator and equity
- the nine opponent archetypes
- decision grading
- the multi-table tournament with payouts and ICM

The port is **bit-exact**: given the same seed, it deals the same cards, makes the same bot decisions and produces the same tournament as the TypeScript build. `Tests/golden_vectors.txt` proves this with 4,201 records exported from TypeScript and replayed here.

```
ShortStackCore/
  ShortStackCore.uplugin
  Source/ShortStackCore/
    ShortStackCore.Build.cs
    Public/ShortStack/      engine headers (plain C++17, no Unreal types)
    Private/ShortStack/     engine sources
    Public/ShortStackTypes.h, ShortStackTournamentSubsystem.h   Blueprint API
    Private/Tests/          Unreal automation test (golden vectors)
  Standalone/               CMake build of the engine, no Unreal needed
  Tests/golden_vectors.txt  exported by web/scripts/gen-golden.ts
```

## Add it to a UE5 project

The plugin is written for **UE 5.1 or later** and has not been compiled inside Unreal yet. The engine sources are verified with GCC and Clang (C++17 and C++20, strict warnings, one unity translation unit), but the Unreal wrapper (`Build.cs`, subsystem, automation test) gets its first real build on your machine. If it fails, paste the Output Log error into Claude.

1. The project must be a C++ project. For a Blueprint-only project, use **Tools > New C++ Class** once to convert it.
2. Copy `unreal/Plugins/ShortStackCore` into `<YourProject>/Plugins/ShortStackCore`, or link it so both stay in sync:
   - Windows, from an administrator prompt: `mklink /J <YourProject>\Plugins\ShortStackCore <repo>\unreal\Plugins\ShortStackCore`
   - macOS or Linux: `ln -s <repo>/unreal/Plugins/ShortStackCore <YourProject>/Plugins/`
3. Right-click the `.uproject` file, choose **Generate Visual Studio project files**, then build the `Development Editor` target in Visual Studio or Rider.
4. Open the editor. The plugin is enabled by default. You can confirm under **Edit > Plugins > Gameplay > Short Stack Core**.

On an engine older than 5.1, delete the `FPSemantics` line in `ShortStackCore.Build.cs`. MSVC's default `/fp:precise` already gives the behavior that line sets.

## Run the golden-vector test in Unreal

- **In the editor:** open **Tools > Test Automation** (on older versions, **Window > Developer Tools > Session Frontend > Automation**). Filter for `ShortStack` and run **ShortStack.Core.GoldenVectors**. It should pass with *4201 passed, 0 failed*.
- **From the command line:**
  ```
  UnrealEditor-Cmd.exe <YourProject>.uproject -ExecCmds="Automation RunTests ShortStack.Core; Quit" -unattended -nullrhi -log
  ```

A failure lists the first mismatching records. A mismatch in the `rng*` or `detpow` records points to a compiler floating-point setting. A mismatch only in `hand`, `bothand` or `tick` records points to a logic difference between the two builds.

## Blueprint API

Use **Get Game Instance Subsystem > Short Stack Tournament Subsystem**, then:

| Node | What it does |
|---|---|
| `Start Tournament (Settings)` | Seats the field. Settings include buy-in, fee, guarantee, entrants, stack, level length, field (Freeroll, Micro, Low, High), table size, hero name, optional rival and seed. |
| `Simulate Round` / `Simulate Rounds (Count)` | Plays one hand at every table with the hero on autopilot. Returns events: Bust, Level, Moved, TableBroken, HandForHand, Bubble, FinalTable, Finished. |
| `Get Standings (Max Count)` | Chip leaders first: rank, name, stack, table, and whether the row is the hero. |
| `Get Blinds`, `Get Level Number`, `Get Level Seconds Left`, `Get Clock Minutes` | Tournament clock. |
| `Get Players Remaining`, `Get Paid Places`, `Get Prize Pool Cents`, `Get Prize For Place`, `Is In The Money`, `Is Hand For Hand` | Lobby and bubble UI. |
| `Get Hero Rank`, `Get Hero Stack`, `Is Hero Busted`, `Is Finished` | Hero status. |

This is enough to build the lobby, the tournament clock and the standings screen now. Interactive hands, where the player acts at the table, come with milestone 3 ("playable table"). Until then, they are available from C++.

## C++ API

```cpp
#include "ShortStack/Tournament.h"

ss::Tournament T(Spec, "Hero", "seed-123", {{"gh0stfold", ss::Archetype::Crusher}});
std::vector<ss::TEvent> Events;
std::unique_ptr<ss::Hand> H = T.StartTick(Events);   // the hero's hand, or nullptr
while (H && !H->bComplete && H->Seats[H->ToAct].Id != ss::HeroId)
{
	H->Act(T.BotDecisionFor(*H, /*Fast=*/false).Action);  // opponents act (ThinkMs is the timing tell)
}
// Hero to act: H->GetLegalActions(), then H->Act(ss::PlayerAction::RaiseTo(2500)) etc.
// Grade it with ss::AnalyzeDecision / ss::GradeDecision (ShortStack/AI/Grading.h).
// Keep dealing until H->bComplete; H->Events drives animation (blinds, deals, bets, showdown, pots).
std::vector<ss::TEvent> More = T.FinishTick(H.get());  // plays every other table, busts, clock
```

`UShortStackTournamentSubsystem::GetTournament()` returns the `ss::Tournament` behind the Blueprint API.

## Build and test without Unreal

```
cd Standalone
cmake -S . -B build && cmake --build build -j
ctest --test-dir build --output-on-failure
```

On Windows, run this from a *Developer Command Prompt for VS*. The Standalone build runs two tests:

- `golden_test`: the 4,201 golden vectors.
- `unit_test`: 5,000 fuzzed hands checking chip conservation, illegal-action rejection, and a 1,000-player tournament played to the end, about 0.7 s.

## Keeping the TypeScript and C++ builds in sync

The prototype remains the fastest place to try out rules and AI changes. After changing anything in `web/src/core/`:

1. Run `npm run export:cpp` in `web/`. It rewrites `Private/ShortStack/PreflopRanking.cpp` and `Tests/golden_vectors.txt`.
2. Make the same change in C++. Each file names its TypeScript source.
3. Run the Standalone tests until `golden_test` reports 0 failed, then commit both sides together.

Rules that keep the two builds identical (also listed at the top of `ShortStack/Common.h`):

- Use doubles only, and never allow fused multiply-add. Every engine `.cpp` includes `StrictFloat.h`, which turns contraction off for Clang and MSVC. The builds also set `-ffp-contract=off` on GCC and Clang, `/fp:precise` on MSVC, and `FPSemantics = Precise` in Unreal. Never enable fast-math for this module.
- Don't call `pow`, `exp` or `log` in simulation code. Use `PowInt`/`DetPow`, which use only multiplication and square roots.
- Make at most one RNG call per expression. C++ does not fix the order in which operands are evaluated.
- Use stable sorts wherever TypeScript's `Array.sort` ordering matters.
- Match JavaScript rounding and formatting with `JsRound`, `JsToFixed1` and `JsNumber`.
- Use no exceptions, no RTTI, and no anonymous namespaces. Put file-local helpers in a uniquely named namespace so Unreal's unity build can merge files.
