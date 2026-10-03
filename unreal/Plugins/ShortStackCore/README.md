# ShortStackCore (Unreal plugin)

The SHORT STACK poker engine in C++. It is a port of `web/src/core/` from the browser prototype and covers:

- No-Limit Hold'em rules with side pots
- the hand evaluator and equity
- the nine opponent archetypes
- decision grading
- the multi-table tournament with payouts and ICM

On top of the engine, the plugin also carries the rest of Night One's logic as plain C++:

- `ShortStack/Game`: the session (lobby, pacing, hero turns, sprint, story texts, results, saves, up to four tables at once) and the RiverLine network (`Network.h`: the schedule, series, regulars, simulated results, leaderboards and news, on a world clock)
- `ShortStack/Game/Life.h`: life away from the tables: shifts, hustles, energy, police heat, rent, the ledger, and the clock skipping ahead while they happen
- `ShortStack/Game/Handles.h`: RiverLine screen names in each country's style, for the regulars and table fields (display only: the engine's `Names.h` stays as it is for parity with TypeScript)
- `ShortStack/UI`: a vector canvas, the RiverLine client (the table in `RiverLine.cpp`, the lobby pages in `RiverLineNet.cpp`, the laptop's other apps in `RiverLineApps.cpp`, multi-tabling's table tabs and tile view in `RiverLineTables.cpp`), profile pictures (`Avatars.h`: 35 icons picked by screen name, with frames), event art (`EventArt.h`: a tile for every tournament on the schedule, a crest for every series and series event), card art, the phone and printed props
- `ShortStack/Audio`: sound synthesis

The Unreal project in `unreal/` (see `unreal/README.md`) is a thin host around them.

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

## Gear and streaming hooks

`ss::SessionHooks` has two optional overrides for the apartment:

- `GearChanged(ItemId, Owned)`: something from GearDrop arrived (or a subscription ended). The desk can show it: a second monitor, a ring light, a mic on an arm, a plant. Item ids are listed in `ShortStack/Game/Gear.h` (`gear::Catalog()`).
- `OnAir(Live)`: the Kast stream went live or ended. Good for an ON AIR sign or the ring light coming on.

Both default to doing nothing, so existing hosts compile unchanged.

The LED room kit is polled rather than pushed: call `Session::RoomGlow(Now)` every frame (with the time passed to `Update`). It returns whether the lights are on, the colour (0xRRGGBB, sRGB) and a level (1 is steady). With sync on and the stream live, the level and colour follow the hype, alerts and big hands. `ANightOneStage::SetRoomLights` turns it into the desk strip, the ceiling cove and the room fill.

## The living world

`ShortStack/Game/World.h` holds everyone else's careers: about 1,600 regulars, the cast and Kast's streamers. They're plain structs, not Actors, and the session owns the world (`Session::Living()`).

**How it runs**

- The world advances with the session clock (`World::AdvanceTo`), an hour at a time. Each day it plans who plays what (from bankroll, comfort, schedule and identity) and resolves events as they finish.
- Weekly, monthly and yearly passes handle:
  - living costs, deposits and cash-outs;
  - stake moves (with hysteresis);
  - skill growth and decline;
  - breaks, retirements, newcomers, staking, streaming and ties between people;
  - Player of the Year and honors.
- **Finishing places come from a formula, never from dealt cards:** the place is u^(1/a), with a = exp(-0.8 · luck · edge). The edge is skill minus the tier's field strength, clamped to ±0.35. Re-entries keep their best bullet.
- The cards are never touched. The world never decides a hand, and the AI at the player's tables plays as before.
- **Determinism:** every random draw comes from a named stream (`world/<seed>/<what>/<key>`). The same seed and the same player inputs give the same world, and a saved world resumes exactly.

**Where it shows up**

- `net::Network::Attach(const world::World*)` makes the lobby's players, results, leaderboards (including `Board::Live`) and news read from the world. Before the world starts, the old deterministic network still answers.
- `Session::NameField` seats up to three registered regulars at the player's table, matched by style. The player's results, pots, knockouts and greetings become memories (`World::Bonds()`).
- `World::ProfileOf(Npc)` is the public player card RiverLine draws (`RiverLineCard.cpp`); it never exposes hidden numbers.
- `World::Headline` turns world events into news.

**The series calendar** (`NetworkSeries.cpp`)

- **What's in it:**
  - Nine online series a year through 2040 (and December 2026's).
  - Twelve named weekly tournaments, appended after the original schedule. Template indices never move, because saves keep them for the events they've planned.
  - `EventTemplate` gains `Main`, `Bracelet`, `Ring` and `Ticket`.
- **Seats:** every year's RCOP Main takes the `rcop-main` seat that the step satellites award. Use `Network::TicketOf(TemplateId)` for the ticket an event takes and `NextFor(Ticket, From, Out)` for the event it enters next.
- **Lookups:** `Network::Index()` builds the lookups by id, day and ticket, so a day's schedule doesn't scan all 8,000-odd templates.
- **Dates:** `net::DateLabel` now uses the world's calendar (`world::CivilDate`, `world::DayOn`), so leap days fall where they should.

**Newcomers and journeys**

- **Arrivals:**
  - Rookies arrive one of eight ways (`world::Arrival`); see the design doc for what each is like.
  - `Npc::Came`, `Arrived` and `CameWith` say how and when. `CameWith` is the friend from the home game, or the streamer or champion they watched.
- **Journeys:**
  - `Npc::Path` keeps the steps of a career (`world::StepKind`): the first six steps and the latest ten.
  - `Sim::Mark` adds them, from the firsts in `Apply` and from career events in `Post`.
- **Profiles:** `World::ProfileOf` adds `Came` (the arrival in a sentence), `Arrived`, `New` (joined in the last 30 days) and `Journey` (dated lines).
- **Debug:** `World::Describe`, behind `ss.World.Npc`, prints the arrival, the journey and every bracelet and ring; `World::Report`, behind `ss.World.Report`, counts arrivals.
- **Stats pages:**
  - `Npc::Stats` (and `Profile::Stats`) is a `world::Tracker`: totals, ROI and ITM, breakdowns by buy-in (`TrackStake`) and format (`TrackFormat`), finishes, records (peak, downswing, biggest score, dry runs, average finish), and the profit graph.
  - The graph is at most 64 points; when it fills, every other point goes and the stride doubles.
  - `Tracker::Spend` keeps the buy-ins at each point, for the ABI graph. `Tracker::StretchAbi(K)` is the ABI over the K-th stretch.
  - `RiverLine::ShowStatsGraph(1)` shows the ABI view.
  - `Sim::Apply` adds every online and live tournament (every bullet; satellite seats at their value). `World::HeroFinished` adds the player's to `World::HeroStats`.
  - `Sim::SeedStats` draws a regular's history before Night One from their lifetime ledgers, with an RNG seeded by their name (the world's own dice never move). It runs when someone joins, and on loading a save from before stats pages.
  - `World::RoiRank` places an ROI among the regulars.
  - `World::GrantHeroResults` plays preview tournaments for the player.
  - The dashboard is `RiverLineStats.cpp`: `StatsKpis`, `StatsBoard`, and `HeroStatsCard` (`ShowPlayer(RiverLine::HeroCard)`).
- **Trophy cases:**
  - `Npc::Awards` (and `Profile::Awards`) keeps every bracelet and ring (`world::Award`: day, ring or bracelet, online or live, Main Event, series, event, prize, entries).
  - `World::HeroAwards` keeps the player's.
  - `Sim::AwardOf` builds an award when someone wins; `World::GrantAward` gives one for debugging.
  - The art is in `EventArt.h`:
    - `eventart::Trophy` draws a bracelet or a ring in its series' design;
    - `eventart::Champion` dresses an `AvatarSpec` in a champion's frame (`AvatarFrame::Bracelet` or `Gem`);
    - `rlnet_detail::NetChampion` looks the champion up for any picture by name.
- **The UI:**
  - `RiverLine::ShowCardTab(1)` opens a card on its Journey tab.
  - A world player's seat at the table opens their card.
  - A NEW tag follows recent arrivals' names.

**Save**

- The world writes its own `world\t...` lines after the session's lines. The first line carries `world::Version`. Hosts that edit a save pass these lines through untouched (`SaveData::WorldText`).
- A save without world lines gets a new world, starting tonight.
- **Newcomer data:**
  - Each `npc` line ends with the arrival fields, and a `path` line holds the journey.
  - Saves from before newcomers read as old hands with empty journeys.
- **Stats data:**
  - A `track` line holds each person's stats page, and `herotrack` holds the player's.
  - The graph's points (net, then buy-ins) are written in dollars, as steps from the previous point.
  - Pages saved before the ABI graph spread their buy-ins evenly.
- **Trophy data:**
  - An `awards` line holds a person's bracelets and rings, and a `heroawards` line holds the player's.
  - Saves from before trophy cases fill them from the titles in history; anything left is a plain bracelet or ring.
- **Size and cost:**
  - Recent results are kept for 8 days (majors for 400 days), and long-gone retirees fold into `ghost` lines.
  - A save is about 2.9 MB after a month, 4.1 MB after a year and 6.7 MB after ten years (about a third of it is the stats pages).
  - The session rewrites the world text only when something involving the player changed, or once an in-game hour has passed.

**Nights away from the desk**

- The host records Dee's game and the Riverside with `SaveData::NoteBackRoom` and `NoteRiverside`.
- These write `worldnote` lines, and the next session plays them into the world once.

**Debug console commands** (non-shipping builds, `NightOneGameMode.cpp`)

| Command | What it does |
|---|---|
| `ss.World.Report` | Population, stakes, bankrolls, form, identities and reputation leaders |
| `ss.World.Npc <name>` | Everything about one person: bankroll, skills, traits, results, schedule, ties, memories of the player, history |
| `ss.World.Leaders` | The 20 best-known players and what they're known for |
| `ss.World.Simulate <days>` | Sleep through 1, 7, 30 or 365 days (up to 3,650); the clock and the world move on |
| `ss.World.Preview <days>` | How the world would look then (a copy is played forward; nothing changes) |
| `ss.World.PlayerResults <count>` | Plays that many small tournaments onto your stats page (Career, Your stats), to preview it |
| `ss.World.Award <bracelet\|ring> [main] [name]` | A bracelet or a ring for the player (or someone by name), from the latest Championship Online or Ring Rush: see the trophy case and the champion's frame |

**Performance**

- About 15 to 19 ms per simulated day (spread over the hour ticks) and 5 to 7 s per simulated year. The yearly series calendar added about a third.
- `world_test years 10 11` runs a ten-year check with seed 11. Over ten years:
  - The active population follows its slowly growing target (about 1,640 to 1,880).
  - The number of players at each stake stays steady, and per-stake median bankrolls stay flat.
  - There are about 100 to 140 pros, and 130 to 170 players with skill of 0.75 or more. Experienced newcomers move a few percent of players from micro to low stakes over ten years.
  - The run fails if any finished Summit or Championship Main has no champion in the history.

## Build and test without Unreal

```
cd Standalone
cmake -S . -B build && cmake --build build -j
ctest --test-dir build --output-on-failure
```

On Windows, run this from a *Developer Command Prompt for VS*. The Visual Studio generator builds Debug by default, so pass the configuration to ctest: `ctest --test-dir build -C Debug --output-on-failure`. The Standalone build runs seven tests:

- `golden_test`: the 4,201 golden vectors.
- `unit_test`: 5,000 fuzzed hands checking chip conservation, illegal-action rejection, and a 1,000-player tournament played to the end, about 0.7 s.
- `session_test`: whole tournaments played through the Night One session, with random and best-EV heroes. It covers the bubble, the money, the final table, sprint mode and save round trips. It also checks the network: tonight's schedule at 2:07 AM, fees, locked formats, the Night Shift board, the player's results landing in final tables and the news, the lobby clock, and registering for a scheduled event. The life checks cover shifts, Marcus's runs, sleep, rent collection and eviction, the Night Shift payout, unlocks, bounty and satellite specs, tickets, a satellite played on a ticket, and a progressive knockout played out. The streaming checks cover GearDrop (screens add tables, the laptop can't stream until the PC upgrade, side-grades are blocked), a whole tournament on stream on Kast, the payout, a shift ending the stream, subscriptions renewing and lapsing, and the gear and channel in the save. They also check Dee and Mei on night one, a first stream that draws a handful of people, a posted schedule, a stream ended with a raid, and the community in the save. A 50-stream grind checks that growth is slow but real: tens of followers after ten streams, Affiliate only once the 30-day rules are met, more regulars and more people coming back, the community drifting after three weeks away, and a schedule and a streamer who talks to chat building a bigger community. The LED checks cover the room kit: no light without it, each colour's perk (and only that one), Aurora's drift, the power switch, the save (and old saves without it), a sub flashing the room, a won all-in sweeping it gold, a bad beat dimming it, the room breathing with hype, and sync off holding the colour.
- `ui_test`: clicks drive the session (log in, filter the schedule, select an event, register, open a page). It also draws every screen, including each lobby page before and after a big night, the laptop apps, the time-lapse, a bounty table and the seat and bounty result screens, GearDrop (the store, an order, the delivery) and Kast (the locked studio, the studio live at a table, the Community page offline and live, the channel, the directory, the end-of-stream card). On Kast it also clicks the schedule's day toggles and start time, and ends a stream with Raid & end. For the LED room kit it buys the kit, picks a colour from Your setup, opens the Room lights card and draws it in each of the seven colours and switched off, toggles power and sync, and opens it from the Kast studio while live (the facecam lit, and swept gold by a won all-in). It ends with a gallery of every product picture, emote and facecam mood. For the living world it draws the boards (including Live), the news, player cards (the rival, Mei, the world's best-known player) and the regulars registered for an event, then sleeps 420 days and draws them again. It also draws:
  - a newcomer's card (overview and journey);
  - December's series, the next one's highlights, and the home page's series slide;
  - The Championship Online in June, and a bracelet winner's journey;
  - a card opened over the table;
  - four event-art sheets: every glyph, every schedule tile, the series' crests, and series events in each metal, including Main Events, bracelets, rings and high rollers;
  - two award sheets: every bracelet and ring design, and champions' frames at card, seat and board sizes;
  - the champions: the biggest winner's card and TROPHIES tab, the boards with champions' frames, a bracelet and a ring winner at the player's table, and the player's trophy case on the Career page;
  - RiverLine Stats: a champion's STATS tab (hovering the graph, then the ABI view, also hovered), the network's biggest winner's, and the player's own from the Career page.
- `monkey_test`: random clicks and keys across every screen while it sits down at random events (up to four tables at once) and winds the sitting down to its results, about 15 s. It also shops on GearDrop, goes live on Kast and works the studio (ads, answers, timeouts, mods), changes the schedule, sometimes ends a stream with a raid, and changes the LED kit's colour, power and sync. Every frame it checks that the bankroll only moves through the ledger, tournament chips are conserved, the clock never runs backwards, no table stalls, saves round-trip, nobody streams without the PC upgrade, the stream's numbers stay in range, the community stays in its ranges (loyalty, affinity, stage, schedule), and the room's lights match the kit and its settings. `./build/monkey_test 40 30000` runs a longer sweep.
- `living_world` (`world_test`): first the online calendar:
  - nine series a year from 2027 to 2040, none overlapping;
  - every series has its Main Event, its events and its counts of rings and bracelets;
  - every series event name and every template id is used once;
  - RCOP seats carry over to every year's Main Event, and leap days are on the calendar.

  Then a world is created and played for a month, about 1.5 s. It checks that newcomers arrive in different ways, with journeys that start the day they joined and cards that tell them. It also checks every person (no negative bankrolls, skills and stakes in range, sane ledgers, at most seven ties, a trophy case that matches their bracelet and ring counts, a stats page whose tournaments, buy-ins and prizes match their ledgers, an ABI graph whose buy-ins only rise), that tonight's events have the world's regulars registered, that final tables mix regulars and unknowns, that the rival keeps the network's index, that a night at Dee's leaves memories, that a saved world writes the same save (the trophy cases included) and plays on exactly as the original, and that an older save without trophy cases fills them in. `world_test years <n> [seed]` runs the long check above (it also fails if any Championship Online bracelet has no winner in the history), and `world_test npc <name> [days]` prints one person's career.
- `audio_test`: every synthesized sound is audible, finite and in range.

To look at the UI without Unreal:

```
./build/ui_test /tmp/ui
cd ../../../../web && node scripts/render-drawlists.mjs /tmp/ui
```

This writes PNGs of each RiverLine screen, the phone and the props. `audio_test /tmp/wav` writes every sound as a WAV file.

## Keeping the TypeScript and C++ builds in sync

The prototype remains the fastest place to try out rules and AI changes. After changing anything in `web/src/core/`:

1. Run `npm run export:cpp` in `web/`. It rewrites `Private/ShortStack/PreflopRanking.cpp` and `Tests/golden_vectors.txt`.
2. Make the same change in C++. Each file names its TypeScript source.
3. Run the Standalone tests until `golden_test` reports 0 failed, then commit both sides together.

The C++ build adds formats the prototype doesn't have: bounty prize pools, satellite seats and knockout tracking (`TournamentSpec::BountyCents`, `SeatValueCents`, `TEvent::EliminatedBy`). They default to off, so the golden vectors still match.

Rules that keep the two builds identical (also listed at the top of `ShortStack/Common.h`):

- Use doubles only, and never allow fused multiply-add. Every engine `.cpp` includes `StrictFloat.h`, which turns contraction off for Clang and MSVC. The builds also set `-ffp-contract=off` on GCC and Clang, `/fp:precise` on MSVC, and `FPSemantics = Precise` in Unreal. Never enable fast-math for this module.
- Don't call `pow`, `exp` or `log` in simulation code. Use `PowInt`/`DetPow`, which use only multiplication and square roots.
- Make at most one RNG call per expression. C++ does not fix the order in which operands are evaluated.
- Use stable sorts wherever TypeScript's `Array.sort` ordering matters.
- Match JavaScript rounding and formatting with `JsRound`, `JsToFixed1` and `JsNumber`.
- Use no exceptions, no RTTI, and no anonymous namespaces. Put file-local helpers in a uniquely named namespace so Unreal's unity build can merge files.
- Mark every non-inline function that code outside the plugin calls with `SHORTSTACKCORE_API`. In editor builds the plugin is its own DLL, and an unmarked function fails the game module's link with `LNK2019: unresolved external symbol`.
