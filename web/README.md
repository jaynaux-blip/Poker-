# SHORT STACK: Night One (browser prototype)

A playable vertical slice of the first-person poker RPG described in [`docs/GAME_DESIGN.md`](../docs/GAME_DESIGN.md). You sit in a studio apartment at 2 a.m. with $2.37 in your account and grind an online multi-table tournament on your laptop.

This prototype exists to prove the game systems and the look before production moves to Unreal Engine 5 and Blender (see [`docs/UE5_PORT.md`](../docs/UE5_PORT.md)).

## Run it

```bash
cd web
npm install
npm run dev        # http://localhost:5173
npm test           # engine, AI, tournament and session tests
npm run build      # dist/short-stack.html: one self-contained file
npm run export:cpp # after engine changes: refresh the C++ port's golden vectors and tables
```

Useful URL parameters for development: `?shot=lobby`, `?shot=table&event=1`, `?pace=full`, `?speed=4` (game time multiplier), `?debug=outside` or `?debug=nopost` (render passes), `?hq=1` (full pixel ratio).

## Controls

| Input | Action |
|---|---|
| Mouse / tap | Use the laptop (the cursor is drawn on the in-world screen) |
| Space | Lean back and look around the room; again to lean in |
| F · C · R · A | Fold · check/call · raise to the selected size · all-in |
| ↑ ↓ or mouse wheel | Adjust bet size |
| M | Mute |

## What's in the slice

- **Real poker.** Complete No-Limit Hold'em rules: big-blind ante, min-raises, incomplete all-in raises that don't reopen action, side pots, uncalled bets, odd chips.
- **A 1,000-player tournament.** Every table in the field plays real hands every round. Your table uses full-strength bots; the rest of the field uses fast bots, so a round takes about 10 ms. Tables break and balance as players bust. The game plays hand-for-hand on the bubble, has payout bands, and forms a final table.
- **Opponents with personalities:**
  - Nine archetypes: Fish, Station, Nit, TAG, LAG, Maniac, Reg, Crusher, plus your rival gh0stfold.
  - Each has position-aware ranges, push/fold play, and Monte Carlo equity against the ranges their opponents' actions imply.
  - They tilt, chat, and give off timing tells.
  - Bots only ever see a public view of the table plus their own two cards.
- **Decision grading.** Every decision you make is graded from Best to Blunder by estimated chip EV, and you get a decision-accuracy score at the end. Skill XP comes from decision quality, not results.
- **Pacing:**
  - **Smart** auto-folds junk and fast-forwards once you're out of a hand.
  - **Full** shows every hand.
  - **Sprint** plays your hands in a solid TAG style until the bubble or the final table.
- **Composure.** Bad beats tilt you: red tunnel vision and a shorter action timer. You can sit out a hand to breathe.
- **The room:**
  - A rain-streaked window refracts the city.
  - Neon from the laundromat across the street throws moving rain shadows through the glass.
  - The sky moves toward dawn during a deep run.
  - An empty energy-drink can piles up on the desk every 50 minutes of game time.
  - Texts from Dee and the landlord arrive on your phone.
- **No external assets.** Every texture, model and sound is generated in code (procedural PBR textures, Web Audio synthesis).

## Code map

```
src/core/          engine, no DOM (portable to C++ for UE5)
  rng.ts           seeded sfc32 RNG, Fisher-Yates shuffle
  cards.ts         card encoding, hand classes
  evaluator.ts     5-7 card evaluator (cross-checked vs brute force)
  equity.ts        Monte Carlo and exact equity
  hand.ts          one hand of NLHE as an event-emitting state machine
  structure.ts     blind levels, payout tables, ICM
  tournament.ts    MTT: seating, balancing, eliminations, clock
  ai/              public view, archetypes, opponent modelling, bot, grading
src/game/          session flow, lobby events, chat, phone texts
src/client/        RiverLine poker client drawn to a canvas (the laptop screen)
src/scene/         Three.js apartment, window/rain shaders, outside city, post chain
src/audio/         synthesized ambience and sound effects
tests/             Vitest suites
```
