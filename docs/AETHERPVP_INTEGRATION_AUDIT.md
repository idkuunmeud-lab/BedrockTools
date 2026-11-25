# AetherPvP integration audit — BedrockTools foundation

Date: 2026-11-23
Target device/build context: Android ARM64; Minecraft Bedrock 1.26.66.1 (user-reported installed version).

## Initial assessment

The repository is a native C++ Android mod for the LeviLauncher/Preloader environment, not a standalone launcher. Its build workflow targets Android ARM64 and packages a `libBedrockTools.so` native library into a `.levipack`. Integrating this mod into the AetherPvP launcher is a separate packaging/loading task.

## Confirmed source-level capabilities

- The module registry includes read-only HUD candidates such as ArmorHUD, PotionHUD, CPS/keystroke displays, counters and player-coordinate displays.
- ArmorHUD reads player/equipment/container data and renders equipment icons and durability information.
- PotionHUD reads a `MobEffectsComponent` from the local player's entity context and copies effect records after basic pointer/range checks.
- InventoryAccess reads inventory container memory and resolves an offhand accessor through a signature.
- The project contains a signature resolver, hard-coded offsets, vtable indices and ABI/layout assertions.
- The GitHub Actions build workflow uses Android NDK r28c and uploads a `.levipack` artifact.

These are source-code findings only. They do not prove the current release works on Minecraft 1.26.66.1.

## Compatibility findings — critical

1. `levimod.json` declares Minecraft version `1.26.6X.X`. This is a broad manifest pattern, not proof of compatibility with 1.26.66.1.
2. `Runtime::resolveSignatures()` treats `resolveAll()` as successful when any signature resolves. It does not require the signatures needed by a particular module or hook set to resolve.
3. `GameHooks::install()` attempts many hooks but ignores individual hook failures. It marks the hook set installed when only the tick and client-update originals are non-null. Other hooks can therefore be absent while installation reports success.
4. The version-string hook captures the game's version in `gameVersion`, but the inspected runtime path does not use that value to gate installation.
5. ArmorHUD, PotionHUD and InventoryAccess use fixed offsets/layouts and native function signatures. The source contains basic sanity checks, but these cannot prove that pointers or function signatures match the installed binary.
6. The inspected signature patterns are ARM64 instruction-byte patterns. They must be verified against the exact 1.26.66.1 `libminecraftpe.so`; a successful partial signature scan is not enough.

**Conclusion:** do not enable these modules on the user's live game yet. A mismatched hook or object layout can crash Minecraft or corrupt process state.

## Required compatibility gate

Before enabling gameplay modules on the user's device:

1. Record the exact Minecraft package version/build and ARM64 library identity (for example, cryptographic hash and ELF build metadata).
2. Build a dependency map for ArmorHUD and PotionHUD: every signature, offset, vtable index, object layout, and function prototype they rely on.
3. Verify each required signature uniquely against the exact target library; reject zero matches and ambiguous/multiple matches.
4. Add explicit version/build gating and per-module capability checks. Missing dependencies must disable only the affected module and must never be treated as a successful full initialization.
5. Build the unchanged fork and inspect the CI artifact before making code changes that affect runtime behavior.
6. Test in a local/offline world first, with a clean rollback path.

## First module priorities

1. Armor/equipment HUD — read-only display.
2. Potion/effect HUD — read-only display.
3. Inventory status and item counters — read-only display.
4. Original AetherPvP modules should use the existing registry and event interfaces where appropriate rather than duplicating low-level hooks.

No automated attacks, anti-cheat bypasses, reach manipulation, or other public-server cheating functionality is in scope.

## Licensing discrepancy to resolve

The README's License section says GPL-3.0, while the checked-in `LICENSE` file contains the MIT License. Do not redistribute a combined build or change licensing metadata until the upstream author/source licensing is clarified. Preserve upstream copyright and license notices.

## Next steps

- Run the baseline Android ARM64 CI build from the unchanged source.
- Inspect the artifact contents and build logs.
- Produce the first compatibility matrix for ArmorHUD and PotionHUD.
- Resolve the license discrepancy before redistribution.


## Expanded whole-project review — 2026-11-25

### Startup and configuration flow

The current startup sequence in `Runtime::install()` is:

1. Resolve signatures.
2. Install core game hooks.
3. Register all modules.
4. Wire runtime events.
5. Load saved configuration.
6. Initialize modules that are enabled by the loaded configuration.
7. Register modules with the launcher menu.

This ordering means module `onInit()` work triggered by saved settings occurs after core hooks are installed. The lazy initialization change also avoids initializing every disabled module. The current `Module::initializeOnce()` sets its initialized flag before calling `onInit()`; this prevents re-entrant duplicate initialization, but there is no explicit success/failure result or retry path if an initializer cannot establish its dependencies.

### Confirmed project-wide blockers

- The module registry advertises 51 modules, but source presence and successful compilation do not prove each module works on the current Minecraft binary.
- The signature resolver still reports success when at least one address resolves. There is no per-module dependency readiness result.
- The game-hook installer attempts a broad set of hooks but only treats the tick and client-update hooks as essential. Other missing hooks are not surfaced as a structured capability report.
- Runtime captures a Minecraft version string in the hook layer, but does not use it as an explicit compatibility gate before enabling version-sensitive code.
- ArmorHUD and PotionHUD both rely on native ABI/layout assumptions, offsets, vtable indices and render-context behavior. Neither is safe to certify from source inspection alone.
- Configuration loading catches all exceptions without exposing a parse/load error to the user. A malformed config can therefore silently leave defaults active.
- README claims GPL-3.0 while the checked-in LICENSE text says MIT. This must be resolved with the upstream author before redistribution.

### Whole-project release gates

1. Identify the exact target Minecraft package version and hash/build identity of its ARM64 `libminecraftpe.so`.
2. Build a dependency inventory for every module: signatures, offsets, RTTI/vtable slots, native prototypes, hooks, and runtime prerequisites.
3. Add explicit global compatibility and per-module readiness gates; missing optional dependencies must disable only the affected capability.
4. Test each read-only HUD in a local/offline world, then test settings persistence, module toggling, reload/unload, and clean failure behavior.
5. Integrate the verified BedrockTools package into the AetherPvP launcher and confirm the launcher actually loads the intended package/library.
6. Implement and test original AetherPvP features only after the shared runtime and module lifecycle are stable.
7. Resolve licensing and produce reproducible release artifacts with a rollback build.

No new gameplay feature is certified by this review. Build success remains compile/package evidence only.
