# SHORT STACK art pipeline

Props are built in Blender from Python, so every asset is reproducible and can be changed by editing code. Each build runs four steps:

1. **Model.** Real-world dimensions in meters, with clean topology.
2. **Bake.** The procedural materials are baked to 2K PBR textures:
   - base color;
   - ORM (occlusion, roughness, metallic);
   - normal.
3. **Export.** The asset is written as glTF (`unreal/Art/Meshes/<SM_Name>.glb`). The Unreal editor imports it on its next launch.
4. **Review.** The asset is rendered to a review sheet (`art/review/<name>.jpg`) for sign-off before it goes in the game.

## Build

```
python3 art/blender/build.py energy_can        # bpy from PyPI (pip install bpy)
blender -b -P art/blender/build.py -- energy_can  # or any Blender 4.2+
```

- With no asset name, the script builds all of them.
- `--no-review` skips the review renders, which take several minutes on a CPU.

Intermediate textures and review tiles go to `art/build/`, which git ignores.

## Review sheets

Every sheet shows six views:

- **Top row:** front three-quarter, top and a close-up, in a neutral studio.
- **Bottom row, left:** wireframe over clay, to check topology and density.
- **Bottom row, right:** two views under the game's lighting: a dark desk, the laptop's cool glow and the laundromat's pink neon.

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

## Fonts

`art/fonts` holds Roboto (Apache License 2.0, see `LICENSE-Roboto.txt`), the family the game's interface uses, for printed artwork.
