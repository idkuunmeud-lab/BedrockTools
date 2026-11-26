# AetherPvP / BedrockTools Compatibility Audit

Updated: 2026-11-26

## Target and boundary

Target is the existing BedrockTools Android ARM64 native module host loaded by LeviLauncher. This audit does not introduce or replace a launcher. Runtime compatibility must be verified against the exact installed Minecraft Bedrock build; a successful Android compile is not proof that a signature, offset, vtable slot, or patch is valid in-game.

## Current BedrockTools module inventory

The current registry already contains a broad set of native modules. The Gold Aether PvP hub is intended to expose existing modules rather than duplicate their implementations.

| Module group | Existing modules | Integration assessment |
|---|---|---|
| Combat / PvP HUD | FPS Display, Ping Counter, Inventory Supply, ArmorHUD, PotionHUD, Totem Counter, Keystrokes, Third Person Nametag, Arrow Counter, Reach Counter, Combo Display | Already present in the BedrockTools registry. HUD-only modules are structurally compatible with the current host; inventory identifiers, ping source, attack event semantics, and native patch signatures still need live validation. |
| Utility HUD | Player Coords, Speed Display, Session Timer, Compass, Tablist, Break Indicator | Already present in the registry. Mostly event/HUD integrations; validate data sources and menu visibility in-game. |
| Visual / performance | Low Graphics Profile, Fullbright, Motion Blur, No Fog, Colored Lights, Light Overlay, Map Preview, Chunk Border, Connected Glass, ViewModel, Zoom, Freelook, Cinematic Camera | Already present. Renderer and camera modules are more version-sensitive than simple HUD overlays. Do not enable unverified renderer hooks by default. |
| Other QoL | Chat Timestamps, No Disconnect, No Touch Border, Quick Loot, Auto Sprint, Weather/Time Changer, Nick, AutoGG, Hive utilities, Shulker Preview, etc. | Already present, but each needs an individual behavior and server-compatibility review before being promoted into the Aether hub. |

## Concrete fixes from this audit

- FPS Unlocker previously forced `eglSwapInterval(display, 0)` and slept inside the frame callback. That is not a display-synchronized frame limiter and can cause tearing, irregular pacing, and large apparent FPS drops. It has been changed to fail closed: it no longer overrides the swap interval or sleeps in the callback. A proper frame-pacing implementation remains future work.
- Gold Aether PvP now includes the existing Third Person Nametag and additional registered HUD modules. The feature array count must match its entries (18).
- Third Person Nametag currently applies a four-byte AArch64 NOP patch at a resolved signature. This is inherently build-sensitive. Do not treat it as verified until the signature and restore path are checked on the target game version.

## External candidate assessment

| Candidate | Public source findings | BedrockTools compatibility |
|---|---|---|
| Xelo-Client | Android ARM64 native loader and JNI modules; GPL-3.0. Its FPS counter hooks `eglSwapBuffers` and measures a one-second window. Its nametag wrapper calls a native patch/unpatch pair. | Platform is relevant, but its loader, hook backend, module ABI, and game signatures are not drop-in compatible. GPL-3.0 obligations must be resolved before any code reuse. Use only as a research reference for now. |
| Selaura | README explicitly targets Android 9+ ARM64 as well as Windows; GPL-3.0; has a separate event, hook, and game SDK architecture. | Potentially useful source of ideas, but not a drop-in module. GPL-3.0 is not compatible with silently copying into this repository; a deliberate licensing decision would be required. |
| CaramelClient | Android 9+, ARM64, C++ native modding backend and SweetLoader integration; Apache-2.0. It is a launcher/backend project rather than a BedrockTools feature module. | Useful for studying runtime capability reporting and defensive backend checks. Not a direct module import; preserve Apache notices if any code is ever reused. |
| Baritone-bedrock | C++ navigation/pathfinding implementation, Windows-focused build and hooks; LGPL-3.0. | Core algorithms may be portable, but game access, movement integration, platform hooks, and license obligations require a separate port. Not safe to drop into Android BedrockTools as-is. |
| OpenSunstead | MIT-licensed, Windows/DirectX-oriented client/UI source. | UI/logic concepts may be portable, but DirectX and Windows hooks are not Android Bedrock runtime modules. |
| AetherPvP-v3.1 | Private repository currently contains only README.md and workflow metadata; no importable module source was found. | No modules can be ported from this repository until source files are added. |

## Compatibility rules for future integrations

1. Prefer modules using BedrockTools' existing `Module`, event bus, HUD draw commands, config manager, and resolved signature catalog.
2. Do not import another project's launcher, hook engine, or native ABI wholesale.
3. Require exact Minecraft-version signature/offset validation before enabling native patches or renderer hooks.
4. Fail closed when required symbols, objects, or runtime capabilities are unavailable.
5. Separate build compatibility from runtime compatibility and in-game testing.
6. Respect each upstream project's license and retain required attribution/notices.
7. Avoid automation or exploit behavior; prioritize HUD, QoL, navigation, and user-controlled visual/performance settings.

## Current status

This document records source-level compatibility findings. It is not a claim that every listed module has been tested in Minecraft. The next validation stage is an Android build of the current changes, followed by in-game checks on the user's exact Bedrock version and device.
