# External Fork Feature Porting Matrix

This document records how the AetherPvP/BedrockTools work uses the user's forked reference projects. It is an integration plan, not a claim that foreign code has been copied or runtime-validated.

## Target constraints

- Keep LeviLauncher as the existing host; do not create a replacement launcher.
- Keep BedrockTools' Module, ModuleRegistry, event bus, config schema, and HUD draw-command APIs as the integration boundary.
- Target Android ARM64 and the exact installed Minecraft Bedrock build.
- A successful compile proves source/build compatibility only. It does not prove that a signature, offset, vtable slot, hook, or render path works in-game.
- Enable new modules only on explicit user action. Resolve required signatures before installing hooks and fail closed when a dependency is unavailable.

## Fork-by-fork findings

| Fork | Useful reference area | Compatibility with BedrockTools | Decision |
|---|---|---|---|
| Xelo-Client | Android overlay layout, FPS presentation, zoom UX | Android/ARM64-oriented, but native JNI entry points are tied to Xelo's launcher package and overlay lifecycle. Its zoom implementation scans RTTI/vtables and writes a vtable slot directly; this is not a safe drop-in for the BedrockTools signature/hook system. | Reuse design ideas only. Keep BedrockTools' own FPS Display and signature-resolved Zoom implementation; do not transplant Xelo's hook code. |
| Selaura | Android ARM64 support, event/module architecture, feature concepts | Different SDK, event model, hook layer, and module ABI. GPL-3.0 obligations apply to any derivative code reuse. | Study architecture and feature ideas. No source transplant without a deliberate GPL-compatible licensing decision and an adapter review. |
| CaramelClient | Loader diagnostics, module capability reporting, safe-mode UX | Android ARM64 native mod loader, but its SweetLoader and launcher integration are a separate runtime. Apache-2.0. | Borrow operational ideas (clear failure reasons, capability reporting, safe-mode diagnostics) conceptually. Do not replace LeviLauncher or import its loader. |
| Baritone-bedrock | Navigation/pathfinding algorithms | Separate navigation engine with movement, mining, and path execution assumptions; not a BedrockTools HUD module and not a direct Android module port. | Keep isolated for a future, separate review. No autonomous movement/mining is being added to this PvP HUD batch. |
| OpenSunstead | UI/rendering concepts | Desktop/Windows-oriented rendering and UI dependencies do not map directly to the Android in-game render lifecycle. MIT. | UI inspiration only; no DirectX/render-hook transplant. |

## Current integration batch

The following existing BedrockTools modules are being surfaced through Gold Aether PvP. These are native modules already registered in this build; the hub only exposes their existing enable/disable state.

| Feature | Module ID | Category | Notes |
|---|---|---|---|
| World Time | bedrocktools.World Time | Utility | Reads world time through the existing Time Changer dependency; verify in a loaded world. |
| Chat Timestamps | bedrocktools.Chat Timestamps | Utility | Adds local timestamps to supported chat/system messages; depends on the current GUI message signature and offsets. |

Existing BedrockTools implementations of FPS Display and Zoom are preferred over Xelo's platform-specific copies. Existing visual modules such as Chunk Border and Breadcrumbs remain separate opt-in modules because they depend on render hooks and version-sensitive structures; they are not being silently enabled by the hub.

## Runtime validation checklist

1. Confirm the module appears in the Gold Aether PvP settings page.
2. Confirm toggling it changes only that module's enabled state.
3. Confirm the module remains disabled after a clean install unless the user enables it.
4. Test on the exact Minecraft version and Android ARM64 device.
5. Check logcat/crash logs after enabling and disabling.
6. If the required signature or dependency is missing, the module must remain inert rather than guessing an address or offset.

## Licensing and attribution

The forks remain separate repositories. This integration batch does not copy their implementation code. Any future source reuse must first record the exact upstream file, license, required notices/source-offer obligations, and the resulting compatibility impact. GPL/LGPL code must not be mixed into the BedrockTools distribution casually.
