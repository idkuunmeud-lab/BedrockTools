# AetherPvP integration audit — BedrockTools foundation

Date: 2026-11-22
Target device/build context: Android ARM64; Minecraft Bedrock 1.26.66.1 (user-reported installed version).

## Initial assessment

The repository is a native C++20 Android mod for LeviLauncher/Preloader, not a standalone launcher. Its build produces `libBedrockTools.so` and a `.levipack` package. AetherPvP should therefore integrate it as a native mod/package first; merging it into the launcher APK is a separate packaging/integration task.

## Confirmed source-level capabilities

- 51 modules are documented and registered in `src/modules/ModuleRegistry.cpp`.
- Armor HUD reads the local player, carried item and equipment/container data, and draws item icons.
- Potion HUD obtains `MobEffectsComponent` from the local player's entity context and copies effect records with basic pointer/range checks.
- InventoryAccess reads inventory container memory and resolves an offhand accessor through a signature.
- The public SDK exposes event, player/world, memory/signature and offset headers.
- The build workflow uses Android NDK r28c and uploads a `.levipack` artifact.

These are source-code findings only. They do not prove the current release works on Minecraft 1.26.66.1.

## Compatibility gate — do not skip

The manifest currently declares `1.26.6X.X`, which appears intended to cover 1.26.66.1. That declaration is not proof that native signatures, offsets, vtable indices, component layouts, or hook calling conventions match the installed binary.

Before enabling gameplay modules on the user's device:
1. Identify every signature/offset used by ArmorHUD, PotionHUD, InventoryAccess and their dependencies.
2. Compare those definitions with the exact 1.26.66.1 ARM64 library and its runtime build.
3. Add explicit version/build gating and fail-closed behavior when a required signature is missing or ambiguous.
4. Build the unchanged upstream fork first and inspect the resulting CI artifact.
5. Test in a local/offline world before considering any multiplayer use.

## Initial module priorities

1. Armor/equipment HUD — read-only display.
2. Potion/effect HUD — read-only display.
3. Inventory status and item counters — read-only display.
4. Original AetherPvP modules should use the existing module registry and SDK/event interfaces rather than duplicate low-level hooks.

No automated attacks, anti-cheat bypasses, reach manipulation, or other public-server cheating functionality is in scope.

## Licensing discrepancy to resolve

The README's License section says GPL-3.0, while the checked-in `LICENSE` file contains the MIT License. Do not redistribute a combined build or change licensing metadata until the upstream author/source licensing is clarified. Preserve upstream copyright and license notices.

## Next implementation step

Establish a reproducible baseline build and produce a compatibility matrix for the signatures and offsets used by the first two read-only HUD modules. Keep this audit separate from user-facing feature work until the compatibility gate passes.
