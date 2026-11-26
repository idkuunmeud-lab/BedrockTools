# AetherPvP External Client Reuse Audit

Date: 2026-11-26

## Scope

BedrockTools remains the native client foundation and LeviLauncher remains the existing loader/host. This audit identifies reusable *design patterns and safe utility/HUD ideas* from the candidate repositories. It is not a plan to build or replace a launcher.

No source or assets are copied by this document. Any later code reuse must retain the source project's license notices and satisfy its license terms.

## Findings

### 1. CaramelClient — strongest Android-side reference

Repository: https://github.com/ChimeraAnt-DEV/CaramelClient  
Fork: https://github.com/idkuunmeud-lab/CaramelClient  
License reported by repository: Apache-2.0  
Platform: Android, Kotlin + C++, arm64-v8a.

Useful patterns to adapt into the existing BedrockTools/LeviLauncher arrangement:

- HUD layout positions stored as normalized screen fractions, rather than raw pixels. This survives resolution and orientation changes.
- HUD safe-area clamping and grid snapping, including display cutouts and system bars.
- Multiple named HUD layouts that store positions independently from module enabled states.
- Frame-time history/graph calculations separated into pure, testable code.
- Cached draw commands, fonts and images; avoid allocations and string rebuilding on every frame.
- Explicit unavailable states when a data feed is missing; never display fabricated readings.
- Config backups and temporary-file replacement before committing settings.

Important boundary: CaramelClient's Android launcher, activity, WindowManager overlay host and SweetLoader are not drop-in replacements for LeviLauncher or BedrockTools. Keep LeviLauncher. Reimplement only compatible concepts inside BedrockTools' existing module/menu/render architecture.

### 2. Selaura — useful event-system reference, not a safe direct port

Repository: https://github.com/selauraclient/selaura  
Fork: https://github.com/idkuunmeud-lab/selaura  
License: GPL-3.0.

The source has a compact typed event manager and pre/post render event examples. These are useful references for event naming and separation of render phases.

Compatibility audit found important blockers:

- The README says Android ARM64 is an intended target, but the checked-in CMake configuration only defines Windows and generic UNIX branches.
- The checked-in platform abstraction currently exposes only a Windows platform type.
- The inspected hook implementation uses Windows x64-style byte signatures and calling conventions.
- Its event bus copies the listener map for every dispatch and has no visible synchronization around subscribe/unsubscribe/post.

Conclusion: do not copy its hook implementation or assume its current source builds for Android ARM64. Treat it as an architectural reference only unless a separate Android-specific branch and implementation are verified.

### 3. Xelo Client — settings and visual reference only

Repository: https://github.com/Xelo-Client/Xelo-Client  
Fork: https://github.com/idkuunmeud-lab/Xelo-Client  
License: GPL-3.0.

The repository is an Android app with an integrated launcher. Its useful material for this project is limited to settings persistence, themes, module-list presentation and general visual organization. Do not import its launcher or duplicate LeviLauncher functionality. Its README describes several features, but those descriptions are not proof that the native features work with our Minecraft build.

### 4. Flarial — Windows-only design reference

Repository: https://github.com/flarialmc/dll-oss  
Fork: https://github.com/idkuunmeud-lab/dll-oss  
License: AGPL-3.0.

The inspected tree contains a large DirectX/Win32 ImGui and event/rendering codebase. It is not an Android ARM64 implementation. Use only high-level UI/architecture ideas after checking each dependency and license; do not transplant the DirectX/Win32 backends.

### 5. OpenSunstead — Windows-oriented reference

Repository: https://github.com/idkuunmeud-lab/OpenSunstead  
License: MIT (repository metadata).

The README describes a Bedrock client for 26.60. Its CMake project and much of its client integration are Windows-oriented. The bundled ImGui tree includes multiple platform backends, but that alone does not make the game hooks or client build Android-compatible. Treat it as a visual/architecture reference, not a direct Android module source.

### 6. Latite website repository

The fork currently named `latiteclient.github.io` is the website repository, not the Latite Client source. It is not a source candidate for native modules.

## Existing BedrockTools overlap

BedrockTools already registers HUD/utilities including armor, potion, coordinates, compass, keystrokes, tablist, speed, CPS/combo/reach counters, ping and other visual modules. Avoid reimplementing an existing feature before auditing its current implementation and runtime dependencies.

The current module lifecycle uses lazy one-time initialization when a module is first enabled. Preserve this behavior when adding modules.

## Recommended implementation order

1. **HUD layout presets** — multiple named position profiles, with module enabled states kept separate.
2. **Safe-area and grid snapping** — clamp draggable widgets to visible screen space and make alignment easier.
3. **HUD render efficiency** — cache text/assets and keep per-frame paths allocation-light.
4. **Frame-time graph** — only show game FPS when the game frame source is actually available; otherwise label any fallback cadence accurately.
5. **Config reliability** — validate settings, preserve a backup, and report parse/write failures instead of silently swallowing them.

## Compatibility and release gates

- Keep the existing LeviLauncher loader. No launcher rebuild is part of this work.
- Match every native signature, structure layout and hook to the exact Minecraft ARM64 build before enabling it.
- A successful Android CI build proves compilation/packaging only; it does not prove the module works in-game.
- Test each module independently and fail closed when required game data or hooks are unavailable.
- Do not import platform-specific code just because the upstream repository advertises Android support.
