# SHORT STACK: Game Design Document

Draft v0.1, for review.

## 1. Vision

A first-person poker RPG about grinding from nothing to the top of the tournament world. You start with $0, a cracked laptop and a freeroll. You finish at a $1,000,000 buy-in table with the lights down and the whole poker world watching.

**Pillars**

1. **Real poker, real skill.** A complete, fair No-Limit Hold'em and tournament engine. The game rewards good decisions, not lucky ones.
2. **The grind is the story.** Bankroll swings, bad beats and deep runs at 4 a.m. The climb from micro stakes to high rollers should feel earned.
3. **Two ways to read people.** Online, you read numbers, timing and bet sizes. Live, you read hands, breathing and eyes, and opponents read you.
4. **Photoreal where it counts.** The table, cards, chips, rooms and light get AAA-level attention, and the camera stays where realism is strongest.

## 2. Core loop

**Session loop:** pick a tournament from the schedule → buy in (own bankroll or a stake) → play → cash or bust → review hands → earn XP and skills → manage money → next session.

**Career loop:** build bankroll and skill → climb the buy-in ladder → satellite into bigger events → go live → build fame, backers and rivals → qualify for the Championship Main Event and The Summit.

Progress comes in two deliberately separate forms:

- **Money and fame come from results.** Results mix luck and skill, as in real poker.
- **Skill XP comes from decision quality, graded by expected value (EV).** If you get all-in as an 80% favorite and lose, your bankroll takes the hit, but you still earn full XP. Luck can slow your wallet, never your growth.

## 3. Career structure

| Act | Title | Bankroll | Where you play | Buy-ins | Home base |
|---|---|---|---|---|---|
| I | Zero | $0 → $500 | Freerolls, micro MTTs, the laundromat back-room cash game | Free to $3.30 | Studio apartment, cracked laptop |
| II | The Grind | $500 → $25K | Online MTTs, PKOs, turbos, satellites, the Sunday Showdown | $5.50 to $215 | Same studio, real desk, second monitor |
| III | Going Live | $25K → $150K | Local casino, regional series main event via satellite, first TV table | $150 to $1,700 live, up to $530 online | Loft, triple monitors |
| IV | The Circuit | $150K → $2M | International festival stops, $25K to $100K high rollers, online high rollers | $5,300 to $100K | Penthouse, hotel suites on the road |
| V | The Summit | n/a | Championship Main Event (10,000 entrants, multi-day), then The Summit | $10K and $1M | Wherever you've earned |

- The bankroll figures are guidance, not hard gates. You can buy into anything you can afford. Dee, your mentor, pushes bankroll discipline (about 1 to 2% of your roll per MTT buy-in), and ignoring it is the classic way to go broke.
- **Satellites connect the tiers.** The "$11 satellite to the Main Event" dream is playable end to end. Online step satellites win tickets to bigger satellites, and those lead to a live seat package.
- **Going broke doesn't end the game.** You drop back a tier and choose how to recover: take a stake from a backer (with makeup), grind the back-room cash game or sell action.

## 4. The hub: your apartment

The whole online career is played from a first-person desk. The apartment is the hub, the menu and the most important graphics showcase.

- **Computer:** runs the RiverLine poker client (tournament lobby and tables), study tools, a poker forum, the staking marketplace and, later, streaming software. All UI is rendered on the in-world monitors, with no floating menus.
- **Phone:** messages from Dee, backers, friends and the rival, plus tournament alerts, your bank balance and rent reminders.
- **Wall calendar:** the tournament schedule and live series dates.
- **Bed:** sleeping ends the day and restores energy.
- **Door:** travel to the laundromat, the casino and the airport.

**How the apartment changes**

- It moves through three tiers: studio → loft → penthouse.
- The desk upgrades continuously: cracked laptop → one monitor → multi-monitor battle station.
- The grind leaves evidence during a session. Energy-drink cans and cold noodles pile up, sticky notes of player reads collect on the monitor bezel, and the sky outside moves from sunset to 4 a.m. to sunrise during a deep run.

## 5. Online play

**The client.** RiverLine is a fictional poker site with a full lobby. You can filter by buy-in, format, speed, field size and guarantee, and you see registration counts, late-registration timers and your results graph.

**Formats:** freerolls, standard MTTs, turbos, hypers, rebuy/add-on events, progressive knockout (PKO) bounties, step satellites, 9-max sit & gos, and the weekly Sunday Showdown ($215 buy-in, $1M guaranteed).

**The network (built).** RiverLine runs like a big real site, all day, every day. It is simulated from fixed seeds and the world clock, so it is the same on every run and moves on as the night does.

- **Schedule.** About 270 recurring events: dailies across the stakes ($0.25 hypers to a $525 High Roller), PKOs, mystery bounties, flip & gos, six-max, Omaha, deepstacks, step satellites, and weekly majors (Monday Madness, Thursday Heater, the $1,050 RiverLine Millions with $3M guaranteed, the $215 Sunday Showdown with $1M guaranteed). Every event has a registration curve, late registration, a field that shrinks, a final table and an overlay when the guarantee isn't met.
- **Series.** Micro Madness (Oct 1 to 14: 70 events, $5M guaranteed, an $11 Main Event for $1M) is running on Night One. RCOP 2026 (Oct 18 to Nov 8: 154 events, $100M guaranteed, a $5,250 Main Event with $25M guaranteed) is announced, with a step-satellite ladder from $2.20. Summer Slam is history.
- **Players.** 1,600 named regulars from 28 countries, with skill, volume, stakes, lifetime records and weekly form, including Team RiverLine pros and the rival, gh0stfold. Events the player isn't in are resolved statistically: the regulars who make each final table are drawn by skill, volume and stakes.
- **Screen names.** Handles read like the ones people really pick: slang compounds ("VelvetRiver", "lazy_owl"), real names in each country's style ("kenji.k", "pablo_ortega", "BramvdBerg"), poker words in the players' own languages ("Kartenhai", "ElTiburon", "ReiDoRio"), grind jokes ("OneMoreTable", "LandlordHatesMe") and the odd gamer tag ("n00bflop", "BlindsTTV"). High-stakes pros favor understated names ("YMorozov", "mbouchard"). Table fields are named the same way and seeded with real regulars from the right stakes (about 3% of the field, 4 to 40 players), so the names on the leaderboards turn up at your tables.
- **Avatars.** Every account has a profile picture: one of 35 vector icons (sharks, owls, foxes, wolves, crowns, rockets, robots, pizza, eight balls and more) on a colored disc. Names pick fitting icons in any language: "ElTiburon" and "C0ldSh4rk" get sharks, "CoolerKing" a crown, "CoffeeAndCards" a mug. Everyone else draws one from the set by name. Frames mark status: a chip edge or colored ring for flair, gold with a star for Team RiverLine, neon for you and the rival, whose ghost no one else may wear. Seats show the player's country flag on the avatar.
- **Leaderboards.** The Night Shift (tonight's micro-stakes race, $1,000 to the top 20), Player of the Year, the all-time Money List, Titles, Final Tables and the running series. Rows show rank movement and eight-week form. The player's own rank is estimated across the whole 412,000-player network.
- **News.** Big wins, series updates, schedule changes and records, plus the player's own results ("grinder_3c wins MM #26").
- **Career.** Lifetime stats, a rank by winnings (Rookie to Legend), the rent bar ($1,225 by Friday), a career path from first cash to the RCOP Main Event, and a head-to-head card against the rival.
- **What you can play.** Tonight's story events and any scheduled Hold'em freezeout or re-entry with a full-ring field of up to 3,000 can be played, an hour before the start through late registration. Bounties, satellites, Omaha, six-max and huge fields are locked "until later in your career."

**Multi-tabling (built).** You can play several tournaments at once. It is the online game's signature skill: managing your attention.

- **Up to four tables,** two when you're exhausted (energy under 20). Every open table keeps playing whether you're looking at it, browsing the lobby or in another app; nothing pauses for you.
- **Adding a table:** the top bar's pages stay a click away while you play, and the + button opens the lobby. Registering opens the new table in front. You can't take two seats in the same event; a seated event's button reads "Open table".
- **Table tabs** in the top bar show each table's stack in big blinds and your rank. A table where it's your turn turns orange and shows a shrinking clock ring and the seconds left, and its turn chime plays even when it's behind. Chips and cards from tables behind stay quiet.
- **One in front (default):** when the table in front doesn't need you and another does, that one comes forward on its own (the longest-waiting first). It waits a moment after you act or switch, and holds while your own all-in runs out unless another clock is nearly gone.
- **Tile view:** all tables at once in a 2×2 grid (two side by side), each with its own Fold, Check/Call, Raise and All-in buttons, its clock bar, time bank, pace and banners. The keyboard's F, C and R act at the highlighted table.
- **Each table is its own tournament:** its own clock, levels, time bank, pace (one can Sprint while the others play), bounties and grades. Tilt is yours across all of them.
- **When a table finishes while others run,** it closes with a toast (place, prize, accuracy) and its result goes into your history and bankroll. The results screen waits for the last table and adds up the sitting: how many tournaments and the net across every buy-in.
- Later: the Focus stat and a second monitor raise the limit; the HUD and notes carry across tables.

**Reading opponents online (digital tells)**

- **HUD (unlockable software):** VPIP, PFR, 3-bet %, aggression and fold-to-c-bet. The stats are computed from hands you have actually observed, so small samples really are unreliable.
- **Timing tells:** how long an AI player thinks depends on how hard the decision is and how strong the hand is, plus personality noise. Instant checks, long tanks and time-bank use all mean something.
- **Bet sizing patterns** for each player.
- **Notes and color tags** you write on players, which carry over between sessions.
- **Chat:** needling, bragging and tilted players typing in all caps.

**Pacing (critical for MTTs).** Real tournaments last for hours, so the game has three speeds:

- **Full:** you play every hand.
- **Smart (default):** junk hands auto-fold and fast-forward. The game stops for any hand that needs a decision: playable holdings, blind defense, facing a raise, bubble and ICM spots, or a tell worth watching.
- **Sprint (optional):** skips ahead to the next milestone (a break, the bubble or the final table). Hands resolve using a play-style profile you pick plus your stats, so you trade control for time.

## 6. Tournament engine

- **Structures:**
  - Blind levels with a big-blind ante, level clocks and breaks.
  - Late registration, re-entries, rebuys and add-ons.
  - PKO bounties, where half of each bounty you collect is added to the bounty on your own head.
  - Top-heavy payouts with about 15% of the field paid, plus guarantees and overlays.
- **Simulating a huge field:** your own table (or tables) plays out hand by hand. The rest of a 10,000-player field runs on a fast statistical "shadow sim." It keeps the total chip count constant and keeps stack sizes and bust-outs per level realistic. The lobby (players left, average stack, chip leader, your rank) always looks believable.
- **Table balancing and breaking** follows real procedure. You get moved, new faces arrive and your reads start over.
- **ICM:** once you've learned the skill, the game can show ICM pressure on your decisions from the bubble onward. Final table payouts use a proper ICM model.
- **Deals:** at final tables, NPCs may propose a chop. You negotiate between an ICM chop, a chip chop or a custom split. NPC personalities bluff about how willing they are.
- **Multi-day events:**
  - At the end of each day you bag and tag your chips (an animated ritual).
  - You check overnight chip counts on your phone and sleep in the hotel.
  - Next day you come back to a redraw at new tables.
- **Determinism:** seeded, reproducible sessions power hand histories and replays.

## 7. Live play (first-person table)

**Physical handling**

- **Cards:**
  - Hold a key to lift the corners and peek. Peek carelessly and a neighbor might see your cards.
  - Leave your cards unprotected and the dealer can muck them, so use a card protector.
- **Chips:** grab stacks by denomination, count them with the scroll wheel and push them forward. Declare your action out loud (a key press), or the physical motion counts.
- **Chip tricks** (riffle, thumb flip, knuckle roll) are idle skills that slightly affect your table image.

**Floor rules as mechanics**

- Real rules apply:
  - String bets are called.
  - A single oversized chip without a declaration is a call.
  - Verbal declarations are binding.
  - Acting out of turn has consequences.
- Angle-shooting NPCs try tricks on you: hiding big chips behind small ones, fake call motions and "Is that a raise?"
- You can call the floor, and the floor rules on it.

**Tells**

- Each NPC has 2 to 4 physical tells tied to how strong their hand is, each with a reliability value. Examples: fiddling with chips, swallowing, glancing at their stack after the flop, freezing, acting strong when weak.
- Pros have reverse tells.
- Hold Focus to study a player. Time slows and details sharpen while Focus drains.
- Stare too long and they notice.

**Your own tells (Composure)**

- A heart-rate model reacts to pot size, bluffing, bad beats and rivals.
- As it climbs, you hear your breathing and heartbeat, your vision narrows, and your first-person hands visibly tremble when you bet. NPCs read that.
- A breathing mini-game (hold and release to a rhythm) brings your heart rate down.
- Gear matters. Sunglasses, hoodies and headphones hide your tells but make table talk less effective.

**Table talk.** During hands, a dialogue wheel lets you probe ("You've got a pair?"), needle or project confidence. Responses depend on the opponent's personality and hand, and skilled players lie.

**The TV table.** Feature tables have hole-card cameras. After big hands, a broadcast-style replay plays with win-% bars, on-screen graphics and commentators reacting to your play.

## 8. Opponent AI

- **Decision engine:**
  - **Preflop:** ranges (opens, 3-bets, calls) that depend on position and stack depth, switching to push/fold play under about 15 big blinds.
  - **Postflop:** decisions weigh hand strength, draws, board texture and win-odds (equity) simulated against the range the AI puts you on.
  - **Style:** bet sizing and bluff frequency come from the player's archetype.
  - **Payouts:** the AI adjusts its risk-taking near the money (ICM awareness).
- **Archetypes:** Fish, Calling Station, Nit, TAG, LAG, Maniac, Short-Stack Specialist, Reg, and Crusher (near-optimal play with rare weaknesses).
- **Opponent mix by stakes:** micro-stakes fields are soft and wild, and high-roller fields are mostly Regs and Crushers. Difficulty comes from who you face, never from the AI cheating.
- **Adaptation:** NPCs track your stats and adjust to you. Named rivals remember you across events.
- **Tilt:** NPCs tilt after bad beats and needling, and it shows in their play and their tells.
- **Fairness guarantee:** the AI never sees hidden cards. The shuffle is a uniform Fisher–Yates shuffle driven by a strong random number generator, and every hand history can be replayed exactly.

## 9. RPG systems

**Skill tree: five branches**

| Branch | Unlocks |
|---|---|
| Math | Pot odds readout → equity estimate (precision improves) → push/fold charts → ICM calculator |
| Reads | Longer Focus, clearer tells, deeper HUD stats, automatic player tagging |
| Composure | Tilt resistance, stronger breathing control, tell suppression |
| Stamina | Longer sessions, more tables, multi-day endurance |
| Presence | Table talk success, control of your table image, leverage in deal negotiation, sponsor appeal |

Skills give you information. They never make decisions for you.

**Decision grading.** Every meaningful decision is graded against an EV estimate as Best, Good, Inaccuracy, Mistake or Blunder, and the grades roll up into an Accuracy score for the session. XP comes from Accuracy plus milestones (first cash, first final table, first title).

**Study**

- **Hand replayer:** step through any hand on a 3D table and see the EV of the actions you didn't take.
- **Leak finder:** spots patterns across your hands, such as "You fold the big blind too often against button min-raises."
- **Rewards:** reviewing your mistakes earns bonus XP.
- **Coach:** a paid coach unlocks targeted drills.

**Condition**

- **Tilt:**
  - Rises with bad beats, coolers, needling and fatigue.
  - Its effects are sensory and pressure-based: a heartbeat, a red vignette, a shorter time bank, only aggressive dialogue options.
  - It pressures you, but it never plays for you.
- **Fatigue:**
  - Long sessions blur your vision slightly and slow Focus recovery, which cuts how many tables you can run.
  - Caffeine gives a boost followed by a crash.
  - Sleep matters during multi-day events.

## 10. Economy

- **Bankroll:** your life. Rent, food, travel, hotels and gear all come out of it.
- **Staking:** backers pay your buy-ins for a share of the profit, with makeup (you must win back their losses before you profit). Deal terms vary by backer personality.
- **Selling action:** for live events, you sell pieces of your action at a markup your reputation justifies.
- **Sponsorship:** unlocked by fame. A sponsor covers buy-ins and asks for patches, appearances and streaming hours.
- **Fame:** grows with results and memorable hands. It changes how tables play against you and unlocks invitations, including The Summit.

**Getting on your feet (built).** Early on, the bankroll is $2.37 and the rent is $1,225 by Friday midnight. The laptop has apps beside RiverLine for the other ways to get there; each one fast-forwards the clock while the room goes from night to day and back:

- **ShiftLink:** minimum-wage gig shifts. Night cashier at the Quik Stop ($7.25/hr), attendant at the Wash & Fold across the street ($8), delivery driver ($6 plus tips), warehouse loader ($9.50, early mornings). Safe, slow, exhausting.
- **Burner:** Marcus pays $120 to $200 a drop-off (and $380 to $600 for the long run once you've proven yourself). Every run adds police heat, and heat raises the chance of getting picked up: a fine, a night in holding, and a debt to Marcus for the lost bag. Sam, a rich player who notices you once you cash, pays you to play his RiverLine account ("ghosting"). If site security catches it, your account is restricted for 24 hours.
- **Bank:** the balance, the rent countdown and payment, and where every dollar came from and went.
- **Sleep:** a nap or a full night. Energy drains while you're awake and with every shift; under 20% your time bank at the table is halved.
- **Rent day:** the landlord collects at Friday midnight if the money is there. If not, it's a final notice with a $150 late fee and three more days, then eviction (Dee's couch). After that, $1,075 on the 1st of every month.
- **Career unlocks:** your first cash opens bounty events (progressive knockouts and mystery bounties), your first final table opens satellites (the steps to the RCOP Main Event pay tickets), and your first title opens six-max.
- **The Night Shift pays:** at 6 AM the top 20 on the night's micro-stakes leaderboard are paid into your balance.

## 11. Narrative

The tone is grounded drama.

- **Prologue:** you get your last $50 in as an 80% favorite and lose. There's an eviction notice on the door and a freeroll on a cracked laptop. The game's first lesson: you played it right.
- **Dee:** an old-school pro who runs the Tuesday game in the back of a laundromat. Dee is your mentor, teaches you tells and pushes bankroll discipline.
- **gh0stfold:** an online rival who keeps turning up at your tables and taunting you in chat. You find out who they are when you finally meet across a live table.
- **The backer:** offers a stake that comes with strings attached.
- **Climax:** the Championship Main Event final table under TV lights, then The Summit.

## 12. Visual showcase

**Locations**

| Location | What it shows off |
|---|---|
| Studio apartment at night | The monitor as the main light source, rain on the window, neon from the street, dust in the air, a sky that moves through the night during a deep run |
| Laundromat back room | Flickering fluorescents, steam from the dryers, cigarette smoke hanging in the light, worn felt |
| Local casino card room | Rows of tables, carpet, background crowd noise, the sound of chips clattering |
| Festival tournament hall | Hundreds of tables, tournament clock screens, end-of-day bagging |
| TV feature table | Broadcast lighting rig, LED table rail, hole-card cams, the crowd on the rail |
| Final table stage | Stage lighting and haze, bricks of cash and the trophy on the felt, confetti |
| The Summit | A private glass-walled salon above a harbor city at night |

**Rendering targets**

- Realistic, fine-grained materials: felt fibers, chip edge inserts and wear, the sheen and bend of card stock.
- Lighting: soft-edged lights and shadows, haze and smoke that light passes through, reflections on glossy surfaces, and time of day through the windows.
- Camera effects: depth of field when you focus, slow motion with motion blur for all-ins, glow on bright lights and subtle film grain.

**First-person animation**

- **Your actions:** peeking at cards, grabbing, counting and pushing chips, chip tricks, mouse and keyboard in online mode, checking your phone, drinking coffee, bagging chips.
- **Your hands show your progress:** the watch, rings and sleeves change as you climb from hoodie to blazer.

**Table animation**

- **The dealer:** pitches cards, pushes pots and runs the shuffle machine.
- **Opponents:** idle loops that carry their tells.
- **Big moments:** slow-motion all-in reveals, the winner's reaction and the crowd on the rail standing up.

**Audio:** chip riffles, card snaps, the shuffle machine, crowd murmur positioned around you in 3D, rain, and a heartbeat that belongs to you.

**Where realism is hardest: human faces.**

- The online half of the game has none.
- At live tables, these keep faces believable:
  - a natural camera distance
  - strong lighting
  - poker's own dress code (sunglasses, caps and hoodies)

## 13. Optional extras (for review)

1. **Streaming:** viewers, reactive chat, clips and donations. A stream delay setting: no delay means more hype, but stream snipers join your tables.
2. **Integrity choices:** offers to collude, get real-time help or run multiple accounts. The site's security team can ban you, with story consequences.
3. **Underground risk:** back-room games can get raided or robbed, so you have to know when to leave.
4. **Host your own home game:** a late-game side business with rake, guest lists and security.
5. **Player of the Year race:** a season leaderboard against named NPC pros.
6. **Coaching:** take on NPC students for income. They later show up at your tables.
7. **Poker news feed:** articles on your monitor about your results, your rivals and scandals.
8. **Collectibles:** card protectors and lucky charms (cosmetic, with a superstition theme).
9. **Photo mode** and exporting hand clips.
10. **Multiplayer tables** (future, outside the current scope).

## 14. Build plan

**Tech decision:** production is built in **Unreal Engine 5** with **Blender** for assets. A browser prototype of Night One (TypeScript, Three.js, Vite) is in `web/`; it proves the systems and the look first, and its engine code is written to port to C++. See `docs/UE5_PORT.md`.

**Vertical slices**

1. **Night One:**
   - Setting: the studio apartment at night, with the RiverLine client on the laptop.
   - The tournament: a complete $1 micro MTT with 1,000 entrants, the shadow-simmed field, table moves, the bubble and payouts.
   - Systems: AI archetypes, Smart pacing and decision grades after the session.
2. **The Back Room:** the laundromat live cash game, with first-person card and chip handling, tells, Composure and table talk.
3. **The Grind:** multi-tabling, the HUD, satellites, the bankroll, the career hub and schedule, the skill tree and the study replayer.
4. **Going Live:** casino and festival MTTs, multi-day events, the TV table with broadcast replays, backers and selling action.
5. **The Circuit and The Summit:** international stops, high rollers, the story, the rival reveal and the endgame.

## 15. Open questions

- Which optional extras make the cut? (Decided when production starts in UE5.)

**Decided:**
- Engine: Unreal Engine 5 with Blender for production; browser prototype first.
- First slice: Night One. The browser prototype is playable.
