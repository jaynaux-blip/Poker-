# SHORT STACK art pipeline

Props are built in Blender from Python, so every asset is reproducible and can be changed by editing code. Each build runs four steps:

1. **Model.** Real-world dimensions in meters, with clean topology. Flat faces next to bevels get face-weighted normals, so they shade flat.
2. **Bake.** The procedural materials are baked to PBR textures, 2K by default and 4K for hero surfaces:
   - base color;
   - ORM (occlusion, roughness, metallic);
   - normal;
   - emissive, for anything that glows.

   Parts that move separately, like the laptop's lid, bake one at a time so they don't shadow each other.
3. **Export.** The asset is written as glTF (`unreal/Art/Meshes/<SM_Name>.glb`). The Unreal editor imports it on its next launch.
4. **Review.** The asset is rendered to a review sheet (`art/review/<name>.jpg`) for sign-off before it goes in the game.

## Build

```
python3 art/blender/build.py energy_can        # bpy from PyPI (pip install bpy)
blender -b -P art/blender/build.py -- energy_can  # or any Blender 4.2+
```

- With no asset name, the script builds all of them.
- `--no-review` skips the review renders, which take several minutes on a CPU.
- `--review-only` renders the review from the exported `.glb` files without rebuilding. It is also the check that the export carries everything: the sheet shows exactly what Unreal imports.
- Without a GPU, environment variables trade time for quality: `SHORTSTACK_AO_SAMPLES` (the occlusion bake, 64 by default), `SHORTSTACK_REVIEW_SAMPLES` (96) and `SHORTSTACK_REVIEW_TILE` (900 px). The street's props were built in the cloud with 24, 40 and 720.

Intermediate textures and review tiles go to `art/build/`, which git ignores.

## Review sheets

Every sheet shows six views (an asset can choose its own studio angles; the laptop's third is a close-up of its keyboard):

- **Top row:** front three-quarter, top and a close-up, in a neutral studio.
- **Bottom row, left:** wireframe over clay, to check topology and density.
- **Bottom row, right:** two views under the game's lighting: a dark desk, the laptop's cool glow and the laundromat's pink neon.

The studio is calibrated so an 18% gray card reads as middle gray. The in-game views are exposed for the dark room, as the game is.

An asset goes in the game once all six views hold up. Check for:

- correct proportions and silhouette;
- crisp print;
- believable materials: no cloudy metal, no plastic-looking paint;
- no shading artifacts;
- good behavior under the in-game lighting.

## In Unreal

The editor setup script (`unreal/Content/Python/shortstack_setup.py`) imports each `.glb` into `/Game/ShortStack/Meshes/<SM_Name>/`. It reimports only when the file changes. `NightOneStage` uses the mesh when it exists and falls back to engine shapes otherwise.

## Assets

| Asset | File | Notes |
|---|---|---|
| GRIND energy drink, opened 12 oz can | `assets/energy_can.py` | Wrap label with nutrition panel and barcode, stay-on tab and rivet, punched opening. 13k triangles. One more appears on the desk every hour of play. |
| KESTREL 15 laptop, base and lid | `assets/laptop.py` | Gunmetal anodized unibody with edge wear. Every keycap is modeled, with printed legends and a white backlight. Also: glass trackpad, speaker grilles, side ports, hinge, rubber feet, webcam and palm-rest stickers. Years of poker left the F, C, R, A, arrow and space keys worn shiny. 14k triangles. Maps are 4K for the chassis and keyboard. |
| Desk | `assets/desk.py` | Solid walnut plank top: flat-sawn grain, lacquer worn matte where forearms rest, coffee and soda rings where the props stand in the stage, scratches, dust toward the wall. Black powder-coated steel frame with chipped edges, plastic feet. Top baked at 4096 x 2048. |
| RiverLine mug | `assets/mug.py` | 11 oz stoneware: glazed, with an unglazed foot. The logo faces the player. Cold coffee with a crema ring, tide lines inside, a dried drip and a rim chip. |
| Phone | `assets/phone.py` | Face up in a worn silicone case: button covers, USB-C port. Cracked glass: hairlines from a corner impact that glint in the light, and thumb smudges. The stage's lock screen lies on its display. |
| Poker chips | `assets/chips.py` | Clay chips from the "Spin Cycle Club" in four denominations: edge inserts, molded ring, printed inlays, grime. Baked once per denomination, then stacked with jitter. Faces hidden in a stack are removed. |
| Desk lamp | `assets/lamp.py` | Balanced-arm lamp, switched off. Enamel base and twin-rod arms, balance springs, knobbed joints. Bell shade with a white interior and a frosted bulb. A cloth cable drops off the desk's rear edge. |
| Office chair | `assets/chair.py` | Mid-back mesh chair: five-star base on twin-wheel casters, gas lift, pilled fabric seat, curved mesh back, T-armrests peeling at the front. |
| Mouse and pad | `assets/mouse.py` | Matte black gaming mouse, sculpted as a signed distance field: palm hump, split buttons, ribbed wheel, thumb buttons, knurled rubber grips, polished where the hand rests. Cloth pad with a stitched edge, a RiverLine print and a rubbed-smooth patch. |
| Monitors on arms | `assets/monitor.py` | The VANTAGE 24" and 27" (`SM_Monitor`, `SM_MonitorWide`), each with its desk clamp, pole, two-link arm, tilt head and VESA plate. Thin bezels, a tapered back housing with vents, a power LED, and a cable routed along the arm and down the pole. The game draws on the display through a widget, sized from the asset's `SCREENS` table. |
| Desktop PC | `assets/tower.py` | The FORGE mid-tower with its side panel off. It has a perforated front and top, a motherboard with heatsinks, an AIO cooler (pump, tubes, a radiator with two fans), RAM, a graphics card, a PSU shroud with a sleeved cable, and six fans. Everything that lights up is in `SM_Tower_Glow`, which the game colors with the LED kit. |
| Microphones | `assets/mic.py` | The CORVID USB condenser (weighted base, yoke, perforated grille, amber mute ring) and the broadcast kit: a dynamic mic with a foam windscreen in a yoke, on a spring-balanced boom arm clamped to the desk's left edge, its XLR cable strapped along the arm. |
| Cameras | `assets/webcam.py` | 720p and 1080p webcams clipped on the laptop lid's top edge (modeled in the lid's frame, so they tilt with it), and a mirrorless camera with a kit lens on a mini tripod with a ball head. |
| Streaming lights | `assets/lights.py` | A 12" ring light on a desk tripod, and a pair of LED key lights on telescoping poles clamped to the desk's edges, aimed at the player. The diffusers are separate `_Glow` meshes that the game lights while the stream is live. |
| Macro pad | `assets/macro_pad.py` | KEYSTRIP: 15 LCD keys on a wedge stand. The key icons (live, BRB, scenes, mute, clip, fold, call, raise, all-in, the suits) are printed by a Sheet into an emissive texture. |
| Headphones | `assets/headphones.py` | Noise-cancelling headphones on a weighted stand: padded band, brushed sliders, yokes, oval cups with leather cushions, NC mics and an LED. |
| Desk plant | `assets/plant.py` | A golden pothos in a glazed pot on its saucer, on the windowsill. Vines stand up, drape along the sill and hang over its edge. Heart-shaped, variegated leaves are folded at the midrib and shrink toward the tips. |
| Blackout curtains | `assets/curtains.py` | A rod on wall brackets with ball finials, and two heavy panels on rings, pinch-pleated at the top and flaring toward the hem. |
| Fiber router | `assets/router.py` | A gloss-topped router with four antenna paddles and status lights, its cables plugged into an outlet on the window recess's wall. |
| Mementos | `assets/trophy.py` | The Riverside's Sunday cup: gold, with loop handles and an embossed spade, on a stepped lacquer base with an engraved brass plaque. Also a walnut stand for the $100 chip from Dee's back room. |
| The player's arms | `assets/arms.py` | Both hands in the sleeves of a charcoal hoodie, rigged with Unreal Mannequin bone names (`SK_Arms`). The hands are sculpted as signed distance fields on an anatomical skeleton. Skin detail is computed per texel at 4K: knuckle wrinkles, nails with lunulae and cuticles, fingerprints, palm lines, veins, moles and flushed fingertips. The primitives that shape the mesh also weight it to the bones. |

### The street, the store, and what the player wears

These use `artkit/street.py`: finishes scaled for things a meter or more tall that live outdoors or in a shop (weathered enamel chipped to primer and rust, rain streaks, road grime rising from the ground; galvanized and bare metal; car paint, tires, shop plastics, lenses). `AStreetStage` places them with `BlenderFacing` (`BlenderProps.h`), so each one's front (Blender -Y) faces where the stage says.

| Asset | File | Notes |
|---|---|---|
| Fire hydrant | `assets/hydrant.py` | Dry-barrel hydrant in chrome yellow: a bolted ground flange, the nozzle section with the pumper nozzle to the street and a hose nozzle each side, the bonnet with its pentagon nut. Bonnet and caps in high-flow light blue, a chain on the pumper cap. Chipped, streaked and splashed with road grime. |
| Litter basket | `assets/trash_can.py` | Slatted steel basket in municipal green on three feet, a rain hood on struts, a black liner bunched over the rim, a RIVERSIDE SANITATION plate. |
| Newspaper box | `assets/newsbox.py` | The Riverside Ledger's coin-op box on a post: the front page behind the door's glass, the masthead, a pull handle, the coin mechanism with its price window and return lever. |
| Streetlight | `assets/streetlight.py` | 7.6 m tapered pole on a fluted base cover, an access hatch, a davit arm 1.5 m out over the road and a cobra head with its warm lens and photocell. |
| Parked sedan | `assets/sedan.py` | A four-door with a lofted body, arches cut and lined, alloys with brake discs, glass and pillars, lights, mirrors, shut lines, RIVERSIDE plates. Baked light grey: the stage paints each car (navy, oxblood, silver, black). |
| Cooler wall | `assets/cooler.py` | The Lucky Penny's ten-door cooler: a lit header (COLD DRINKS · ENERGY · WATER · JUICE · ICE · DAIRY), chrome pulls, LED mullions, wire shelves with price rails. The glass is left out; the stage stocks the shelves. |
| Gondola shelf | `assets/store_shelf.py` | A 6 m double-sided run: slotted spine and uprights, three shelves a side on brackets, price channels with tags, kick plates, end panels. |
| Register | `assets/register.py` | The point of sale on the counter: cash drawer, the clerk's touchscreen mid-sale, receipt printer, the customer's pole display and PIN pad, a scanner in its cradle. |
| Hats and glasses | `assets/wearables.py` | `SM_Hat_Beanie` (cuffed rib knit), `SM_Hat_Cap` (six panels, a stitched spade, pre-curved bill), `SM_Hat_Bucket`, and `SM_Glasses_Round`, `_Square`, `_Wire` and `_Shades`. Fabric and frames are baked light grey, and the character tints them. A hat's origin is the top of the head; the glasses' origin is the bridge of the nose. |

## Organic shapes

`artkit/sdf.py` models smooth, organic forms (hands, a mouse, fabric folds) as signed distance fields:
- tapered capsules, ellipsoids, rounded boxes and tori, joined with smooth unions whose blend radius sets how softly neighbors merge, or carved;
- meshed with marching cubes, then decimated.

Each primitive can belong to a bone, so the same field that shapes a character also skins it.

`artkit/texmaps.py` computes texture maps per texel in numpy, for detail too fine for shader nodes: it bakes every texel's position, fills color, roughness and height from it, and converts the height to a normal map.

## Fonts

`art/fonts` holds Roboto (Apache License 2.0, see `LICENSE-Roboto.txt`), the family the game's interface uses, for printed artwork.
