# SHORT STACK: The open world

Scope and status for the third-person, walk-around side of the game. The poker career stays the heart of
it. The open world is where the player's life happens between sessions: the walk to the store, the bus to
the casino, the laundromat where Dee's game runs.

## Step 1 (built): you, your door, and the corner store

### Who you are: the character creator

**New Game** now opens a four-page creator: **Who**, **Background**, **Look** and **Review**. A live vector portrait of the character stands in a lit booth beside every page.

- **Who.** First and last name, screen name, age (18 to 65) and country, picked from a grid of 28 with flags. The country sets the home city on the Players Club card.
- **Background.** Six stories, each with a small, permanent perk (`ss::hero::Perks`):

  | Background | Perk |
  |---|---|
  | Line cook | Shifts pay 20% more; meals fill 25% more |
  | Dealer's kid | At Dee's table, a tell is learned the first time showdown confirms it, not the second |
  | Dropout | Bounty events unlocked from the start |
  | Bouncer | Tilt builds 30% slower |
  | Hustler | Burner runs are 5 points safer; heat cools 1.5x faster |
  | Newcomer | Starts with $40 saved |

- **Look.** Fourteen choices in four tabs:
  - **Face:** body type, face, skin tone, eyes, brows.
  - **Hair:** hair, hair color, facial hair.
  - **Body:** build and height (152 to 203 cm, shown against the door frame).
  - **Style:** jacket, jacket color, glasses, hat.
- **Review.** The Embercrest Players Club ID, the bio, and **Begin**.

Q/E switch tabs and R randomizes. The character is saved with the career (`hero` lines in the save) and shows up as **PLAYING AS** on the main menu.

### Needs and the Lucky Penny #212

- **Hunger and thirst** rise with the clock: 3 and 4.2 points an hour awake, a third of that asleep. Above 70, each drains energy faster. At 75 hunger, Mom texts.
- **The store.** The Lucky Penny #212, on the corner of Fifth and Market, sells 14 items from four shelves: drinks, snacks, hot food and coffee. Every item is a fictional brand, from Cascade water at $1.49 to an egg salad sandwich at $4.49. Tax is 7.25%.
- **The counter.** The store UI is a receipt that prints as you add items and gets stamped PAID or DECLINED. Benny, the night clerk, has a line for the hour, your shifts at his counter, and how hungry or tired you look.
- **The bag.** What you buy goes in the bag. **F** eats or drinks the best item for whichever need is higher.
- **At home: Penny Drop.** The store's own delivery app is on the laptop's taskbar.
  - **Ordering.** The same shelves cost 15% more, plus a $3.99 fee and tax, with a $5 minimum. An order arrives at the door 25 to 45 minutes later, a little longer between 2 and 6 AM. It comes with a text and goes into the bag.
  - **Tracking.** Up to three orders can be on the way at once.
  - **Walking is cheaper.** The cart shows what the same food costs at the counter.
  - **The At home card** shows hunger, thirst and energy and lets you eat or drink from the bag.
  - **The kitchen tap** is free. Each glass takes 15 off thirst, once every 45 minutes, and never below 40. It keeps you going; it doesn't replace a cold drink.
  - **The taskbar badge** turns orange when you're getting hungry or thirsty, red when it's bad, and gold while an order is on the way.
- **Night one** starts with two noodle cups in the cupboard.
- **Pacing.** The session test simulates a week of sensible play: eating and drinking when needs climb, walking to the store when the bag is empty, sleeping eight hours. It takes about five store runs at about $6.50 a day. Needs peak at 66, below the 70 where energy drain starts.

### The street, in Unreal

| Piece | What it is |
|---|---|
| `AShortStackCharacter` | You, as you made yourself: a MetaHuman, `Hero<A or B><skin band>` from `hero_cast.py`, falling back to the Back Room's "Hero" and then the archetype body. It wears the jacket's color on the shirt (Flannel brings its plaid), shorts that go with it, sneakers made at run time to fit its feet, your hair color, facial hair, hat and glasses (fitted to the face, carried by the head), and is scaled to your height and build. Benny uses the same class. |
| `UStreetBodyAnim` | A procedural gait: legs, pelvis bob and twist, counter-swinging arms, the head following the look, a sip animation. It needs no animation assets. |
| `AStreetStage` | Fifth Street from door 1812 to the Lucky Penny: sidewalks, the wet road, building fronts both sides, the Wash & Fold's neon across the street, streetlights, rain, fog, and the store inside and out (sliding doors, coolers, aisles, counter, roller grill, coffee bar). Signs are drawn by the core's `PropArt`. Blender props replace the engine shapes where they're imported. |
| `AStreetGameMode` | The session runs outside as it does at the desk: the clock, needs, texts, and saves to the same slot. It also runs the HUD, the store counter and the pause menu. |

**Controls**

| Input | Keyboard and mouse | Gamepad |
|---|---|---|
| Walk | WASD | Left stick |
| Run | Shift | Left bumper or left stick click |
| Look | Mouse | Right stick |
| Switch first/third person | V | Y |
| Use (counter, cooler, aisle, coffee, the door home) | E | A |
| Eat or drink from the bag | F | X |
| Camera distance (third person) | Wheel | |
| Pause | Esc or P | Start |

**Travel.** **G** at the desk saves and walks out the door: `OpenLevel(Street, From=<world minutes>)`. **E** at door 1812 goes home: `OpenLevel(NightOne, Home?Street?From=…)`. The clock carries over both ways. You can't leave with tables open or during a time skip.

**Third person at live events.** **V** at the Back Room table moves the camera behind the hero's right shoulder, with their head in the shot. Peeking and Focus pull it back to first person, and V switches back.

### Setting it up on the desktop

1. **Build** the C++ module. The editor runs `shortstack_setup.run()`, which builds `M_Street` and the `/Game/Maps/Street` level (`street_setup.py`) and imports the new props from `unreal/Art/Meshes`.
2. **Hero MetaHumans (optional).** In the editor's Python console, run `import hero_cast as h; h.step()` until it returns `done`. This builds six MetaHumans (two body types times three skin bands). Without them, the player wears the Back Room's hero. Each hero's preset is in `hero_cast.DEFAULTS` (override per hero in `HERO_PRESETS`; a face the cast already wears is refused), with hair from `STYLE` that sits under the creator's hats; `h.rebuild("HeroA0")` makes one again.
3. **Play.** Press **G** in the apartment, or open `/Game/Maps/Street` and Play.

### Tuning on the desktop (written blind)

Unreal can't run in the cloud, so the gait and the wearable offsets were set by reasoning, not by eye. These console variables tune them live:

| Variable | Default | What it does |
|---|---|---|
| `ss.Walk.Sign` | 1 | Set it to -1 if the legs swing backward |
| `ss.Walk.Stride`, `ss.Walk.Arms`, `ss.Walk.Bob` | 1 | Scale the leg swing, the arm swing, and the pelvis bob and twist |
| `ss.Wear.HatUp`, `ss.Wear.HatForward` | fitted (about 22, 0.4 cm) | Hat crown from the head bone; fitted to each face (eye height and spacing: the cuff 3.5 cm over the eyes) until set, then fixed for all |
| `ss.Wear.GlassesUp`, `ss.Wear.GlassesForward` | fitted (the eyes) | Glasses' bridge from the head bone; fitted to the eyes until set |
| `ss.Wear.Refit` | | Puts the hats and glasses on again after changing the above |
| `ss.Walk.Curl` | 1 | Scales how far the hands curl |
| `ss.Walk.SipSide`, `ss.Walk.SipAhead`, `ss.Walk.SipUp` | 3, 8, -11 cm | Where a sip brings the right wrist, from the mouth |

Props face the way the stage intends through `BlenderFacing()` (`BlenderProps.h`), which works out the importer's yaw from the laptop mesh, as the apartment does.

The imported glTF materials are tinted by whatever base-color parameter they expose (`TintImported`). This covers the parked cars' paint, the hats and the frames. If the hats or cars come out light grey, the importer named that parameter something else. Clear lenses use M_Lens (built by shortstack_setup); hats and glasses import without Nanite.

## What comes next

Each step keeps the walk-around world small, dense and tied to the career. There are no empty streets.

1. **The block.** Evicted, you wake on Dee's couch, and door 1812 is locked to you. The Wash & Fold is enterable, and Dee's Tuesday game moves from a menu into the back room you walk into. It reuses the Back Room stage with the third-person toggle. Pedestrians walk the sidewalks: a handful of MetaHumans on the procedural gait, following waypoint loops. Shift work at the Lucky Penny becomes a playable night behind Benny's counter.
2. **Time and weather on the street.** The street follows the session clock: dusk, the dead hours, sunrise. Rain comes and goes (the materials already carry a Wet parameter). The store's stock and Benny's lines follow the hour.
3. **Getting around.** A bus stop on Market St with a timetable takes you to the Embercrest's card room for live events and to the bank. Each stop is a small, dense scene, not an open map. Later, rideshare costs money and saves time.
4. **Clothes and looks that move with the career.** A thrift store and then better shops sell jackets, hats and glasses (more Blender wearables), and the creator's choices become things you own. Fame changes how the street reacts: a double take, someone asking for a photo.
5. **Live events in third person.** The casino floor is walkable between levels: the cage, the rail, the bathroom break. You walk to your table and sit, and the table switches to the first-person camera with V for third person.
6. **The apartment tiers outside.** The loft and the penthouse each come with a neighborhood: different shops, a different walk, different neighbors.

## Status checklist

- [x] Character creator, perks, save, main-menu card
- [x] Hunger and thirst, the store's catalog, checkout, the bag, Benny
- [x] Penny Drop on the laptop: delivery, eating at home, the kitchen tap, a week's pacing check
- [x] Rent with teeth: monthly reminders, a late fee without drift, and eviction to Dee's couch. While you're out the gear is in storage, the desk is empty, and an unpaid unit goes to auction. **Move back in** gets you home (GAME_DESIGN §10)
- [ ] Dee's couch as a place: wake up in her front room, not the apartment, while evicted. Door 1812's lock refuses your key
- [x] The street level, the character, procedural gait, first and third person, the HUD, the counter UI, the pause menu
- [x] Apartment ↔ street travel with the clock carried over
- [x] Third person at the Back Room table
- [x] Blender props: hydrant, litter basket, newsbox, streetlight, sedan, cooler wall, gondola, register, hats and glasses
- [ ] First play on the desktop: gait, wearable offsets, prop scale and facing, lighting balance
- [ ] Step 2 onward, as above
