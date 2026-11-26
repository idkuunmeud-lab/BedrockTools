# External Fork Feature Porting Matrix

Date: 2026-11-26

## Target constraints
- Keep LeviLauncher as the existing host; do not create a replacement launcher.
- Keep BedrockTools' Module, ModuleRegistry, event bus, config schema, and HUD draw-command APIs as the integration boundary.
- Target Android ARM64 and the exact installed Minecraft Bedrock build.
- A successful compile proves source/build compatibility only, not runtime compatibility of signatures, offsets, vtable slots, hooks, or render paths.
- New modules remain opt-in and must fail closed when dependencies are unavailable.

## Fork review and decisions
| Fork | Inspected area | Compatibility | Decision |
|---|---|---|---|
| CaramelClient | Android ARM64 frame-rate readout/history, config and overlay code | Strongest Android reference, but its launcher/activity/overlay host is separate from LeviLauncher. | Adapt ideas: reduce per-frame allocations, separate sampling from drawing, clamp HUD widgets to safe screen bounds. No launcher replacement. |
| Xelo-Client | Android overlay, FPS and zoom code | Android-oriented, but hooks and JNI lifecycle are tied to its own app. | Keep BedrockTools FPS and Zoom implementations; use UI ideas only. |
| Selaura | Event system, renderer abstractions and hooks | Different SDK and hook ABI; GPL-3.0 obligations apply to source reuse. | Architecture reference only; no hook transplant. |
| Baritone-bedrock | Pathfinder, path renderer and movement/mining execution | Separate automation engine, not a HUD module or drop-in Android integration. | Keep isolated; no autonomous movement or mining in this PvP feature batch. |
| OpenSunstead | HUD editor, render manager, target HUD and pathfinder | Windows/DirectX-oriented dependencies and different SDK. | High-level HUD/editor ideas only; no renderer or hook transplant. |
| Flarial (dll-oss) | Win32/DirectX event and render codebase | Windows-only architecture; AGPL-3.0. | No source transplant. |

## Gold Aether PvP integration batch
The hub links existing BedrockTools modules; it does not duplicate implementations or silently enable them.

| Feature | Module ID | Category |
|---|---|---|
| FPS Display | bedrocktools.FPS Display | PvP HUD |
| Ping Counter | bedrocktools.Ping Counter | PvP HUD |
| Inventory Supply | bedrocktools.Inventory Supply | PvP HUD |
| ArmorHUD | bedrocktools.ArmorHUD | PvP HUD |
| PotionHUD | bedrocktools.PotionHUD | PvP HUD |
| Totem Counter | bedrocktools.Totem Counter | PvP HUD |
| Keystrokes | bedrocktools.Keystrokes | PvP HUD |
| Third Person Nametag | bedrocktools.Third Person Nametag | PvP HUD |
| Arrow Counter | bedrocktools.Arrow Counter | PvP HUD |
| Reach Counter | bedrocktools.Reach Counter | PvP HUD |
| Combo Display | bedrocktools.Combo Display | PvP HUD |
| Break Indicator | bedrocktools.Break Indicator | PvP HUD |
| Tablist | bedrocktools.Tablist | PvP HUD |
| Compass | bedrocktools.Compass | Utility |
| Player Coords | bedrocktools.Player Coords | Utility |
| Speed Display | bedrocktools.Speed Display | Utility |
| Session Timer | bedrocktools.Session Timer | Utility |
| World Time | bedrocktools.World Time | Utility |
| Chat Timestamps | bedrocktools.Chat Timestamps | Utility |
| Zoom | bedrocktools.Zoom | Camera |
| Freelook | bedrocktools.Freelook | Camera |
| Chunk Border | bedrocktools.Chunk Border | Visual |
| Low Graphics Profile | bedrocktools.Low Graphics Profile | Performance |
| Projectile Trajectory Preview | bedrocktools.Projectile Trajectory Preview | PvP HUD (planned; disabled until shared world-render integration is implemented) |

## Compatibility notes
- Zoom uses BedrockTools' own resolved FOV/turn-delta signatures. Xelo's implementation is not copied.
- Freelook resolves and installs its own camera hooks and checks required handles before reporting itself available.
- Breadcrumbs and Chunk Border share the RenderLevel/tessellator path and are version-sensitive. They are independent opt-in modules.
- Chat Heads is not bundled into this batch: it hooks several UI/player-list paths and performs player/message cache work. It needs a separate performance and signature audit first.
- Baritone-bedrock is not integrated as autonomous movement/mining. Its assumptions do not map to the current HUD-first task.

## Runtime validation checklist
1. Confirm each entry appears in Gold Aether PvP.
2. Toggle each feature independently; verify it changes only its own module state.
3. Confirm new features remain disabled until explicitly enabled.
4. Test on the exact Minecraft version and Android ARM64 device.
5. Check logs after enable/disable and after leaving/rejoining a world.
6. If a signature or dependency is unavailable, the feature must remain inert rather than guessing an address or offset.

## Licensing and attribution
No external implementation code is copied in this batch. The forks remain separate repositories. Any future source reuse must record the exact upstream file, license, required notices/source-offer obligations, and compatibility impact before code is brought across.


## Projectile trajectory preview status

The Gold Aether PvP menu now reserves a dedicated, disabled entry for Projectile Trajectory Preview. It is intentionally unavailable until a shared world-render hook is implemented. The current Breadcrumbs and Chunk Border modules independently hook RenderLevel; installing a third independent hook at the same target risks hook chaining conflicts and instability. The preview must use one coordinated render path, verified projectile launch parameters and collision handling before it can be enabled. The placeholder does not install hooks, read projectile state, or affect frame performance.


## Breadcrumbs removal

Breadcrumbs has been removed from the Gold Aether PvP feature hub at the user's request. The standalone BedrockTools Breadcrumbs module source and registry registration are retained unchanged; this removes it from the Gold hub without deleting the independent module or changing existing user configurations.
