# ArmorHUD and PotionHUD dependency audit

Date: 2026-11-23
Target runtime: Minecraft Bedrock Android ARM64 1.26.66.1 (user-reported; not yet verified against binary metadata).

This is a source inspection only. No native signatures, offsets, or runtime behavior have been validated against the target `libminecraftpe.so`.

## ArmorHUD

### Signature dependencies
- `BaseActorRenderContextCtor`
- `NbtTreeFind`
- `ItemStackBaseGetRawNameId`
- `ItemRendererRenderGuiItemNew`
- RTTI/vtable lookup: `17HudCameraRenderer`, slot `HudCameraRendererRender`

The four signature-resolved function pointers are assigned in `ArmorHudModule::onInit()`. The code does not fail module initialization if any is null. The HUD renderer hook is installed separately if the RTTI/vtable lookup succeeds.

### Layout and vtable dependencies
- Actor entity-context pointer: `Actor::mEntityContext`
- Player inventory/container layout: `Inventory::PlayerInventory`, `PlayerInventoryContainer`, `FillingContainerItems`, `ItemStackSize`
- Item stack/item-counter layout: `ShulkerPreview::ItemStackBaseItem`, `SharedCounterPointer`
- Client/game/context layouts: `ClientInstanceMinecraftGame`, `MinecraftUIRenderContextScreenContext`
- NBT node/tree layout: `ItemStackBaseUserData`, `CompoundTagTreeRoot`, `CompoundTagTreeEnd`, `NbtNodeType`, `NbtNodeNumericValue`
- Vtable calls: local-player lookup, carried-item lookup, game lookup, item max damage, item animation frame, UI clipping rectangle, texture lookup, image draw and image flush.
- Additional fixed-size assumptions: `BaseActorRenderContextStorageSize` and `BaseActorRenderContextItemRenderer`.

## PotionHUD

### Signature and renderer dependencies
- RTTI/vtable lookup: `21HudMobEffectsRenderer`, slot `HudMobEffectsRendererRender`.
- No `SignatureId` function pattern is directly resolved in the inspected PotionHUD source.
- The renderer hook is installed in `PotionHudModule::onInit()` if the RTTI/vtable lookup succeeds.

### Layout dependencies
- Actor entity-context pointer: `Actor::mEntityContext`.
- `MobEffectsComponent`: three pointers (`begin`, `end`, `capacity`).
- `MobEffectInstance`: asserted size `0xA0`, with fields at offsets `0x00`, `0x04`, `0x20`, and `0x40`–`0x44`.
- UI vtable calls: local-player lookup, clipping rectangle, texture lookup, image drawing and image flush.
- UI context offsets: `MinecraftUIRenderContext::mClient` and `ShulkerPreview::MinecraftUIRenderContextScreenContext`.

## Shared runtime risks

1. `ModuleRegistry::initialize()` calls `onInit()` for every registered module, regardless of whether the user has enabled it. Both HUD modules attempt to install their renderer hooks during this initialization.
2. The runtime's global signature resolver returns true if at least one signature resolves. This is not a module-specific dependency check.
3. `GameHooks::install()` ignores individual hook failures and considers the hook set installed when only the tick and client-update hooks are present.
4. The inspected HUD initialization paths do not gate hook installation on the captured Minecraft version or a verified binary fingerprint.
5. Null checks and vector-range checks are useful defensive measures, but they cannot validate the correctness of a raw object offset, vtable slot, C++ object layout, or function calling convention.

## Safe next implementation sequence

1. Keep both HUD modules disabled by default and avoid loading their native hooks on an unverified game build.
2. Add a verified runtime build identity check before any module-specific native hook is installed.
3. Represent each module's dependencies explicitly and require every dependency to resolve uniquely before enabling that module.
4. Add diagnostics that report which dependency failed without dereferencing unknown addresses.
5. Validate the exact 1.26.66.1 binary offline, then run the unchanged baseline build and a local-world smoke test.

No public-server automation, anti-cheat bypass, or gameplay manipulation is part of this audit.
