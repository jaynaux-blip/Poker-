# SHORT STACK

A first-person poker RPG about grinding from nothing to the top of the tournament world: from freerolls on a cracked laptop to a $1,000,000 buy-in final table.

- **Design:** [`docs/GAME_DESIGN.md`](docs/GAME_DESIGN.md)
- **Playable prototype (Night One):** [`web/`](web/), a browser build with the full poker engine, a 1,000-player tournament, graded decisions, and the rainy 2 a.m. apartment
- **Production plan:** [`docs/UE5_PORT.md`](docs/UE5_PORT.md), for Unreal Engine 5 and Blender
- **Unreal project:** [`unreal/`](unreal/README.md), Night One in Unreal Engine 5. Open `unreal/ShortStack.uproject`.
- **Unreal plugin:** [`unreal/Plugins/ShortStackCore`](unreal/Plugins/ShortStackCore/README.md), the engine, game session, laptop UI and sound synthesis in engine-agnostic C++. The poker engine matches the prototype bit for bit.
