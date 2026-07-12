# TODO - Depth sorting + collision refactor

- [ ] Inspect current `src/screens/main_level.cpp` render + collision logic (full relevant sections).
- [ ] Fix/verify rock init list fields in `main_level.cpp` so every rock has the required members used by depth + collision.
- [ ] Verify `src/screens/main_level.h` structs `Rock` and `Tree` (`GetDepthY`, `CollisionBounds`, `TrunkBounds`, `Draw`).
- [x] Implement trunk-only tree collision and rock collision in `GameplayScreen::ResolveWorldCollision(Character* c)`.
- [x] Update `GameplayScreen::Update()` to call `ResolveWorldCollision()` instead of legacy `ResolveRockCollisions()`.
- [x] Refactor `GameplayScreen::Draw()` to build one render list for: player, remote players, bots, trees, rocks.

- [ ] Sort render list every frame by `GetDepthY()` and draw in sorted order.
- [ ] Remove hardcoded draw order for trees/rocks that would break depth sorting.
- [ ] Compile + run and verify:
  - [ ] Player behind/in front of trees based on feet/trunk depth.
  - [ ] Leaves are non-solid; only trunk blocks.
  - [ ] Player cannot walk through rocks.
  - [ ] Rocks/trees render depth correctly.

