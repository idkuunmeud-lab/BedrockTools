# External client research — Apollon

Date: 2026-11-25

## Repositories inspected

- `Miraniskoo/Apollon-Client-Minecraft-Bedrock-Edition`: a one-file README mirror; repository metadata reports no declared licence and no source tree.
- `TheChuan1503/apollon-client-for-minecraft-pe`: contains an Android application and native C++/JNI sources, including an ImGui Android/OpenGL UI, embedded image assets, configuration code, hook utilities, and version-specific offsets.
- `flarialmc/dll-oss`: a substantial public Bedrock client codebase under AGPL-3.0. Its README describes a contribution model where some important portions remain private and version signatures/offsets may deliberately lag current Minecraft releases. It is primarily a Windows Bedrock client, not a drop-in Android/LeviLauncher mod.
- `AtlasClientMC`: the official organisation is public, but its visible repositories are translations, a retired website, and a Dobby hook-library fork; no full Atlas client implementation was found in the public organisation repositories.

The Apollon source repository's README explicitly says the repository creator did not write the code and attributes the source to ZEFF. Its root has no LICENSE file, and GitHub reports no detected licence. Public visibility is not permission to copy, modify, redistribute, or bundle the code/assets.

## What is useful to study

Apollon's source demonstrates broad architectural patterns worth evaluating independently:

- A native in-game UI rendered through an Android/OpenGL backend.
- A central menu with categories, reusable controls, icons, and resolution scaling.
- JSON-backed user configuration.
- Native module state separated from UI rendering.
- Embedded image/font assets and lazy GPU texture creation.

Flarial is useful as a larger reference for feature organisation, documentation, and a public contribution workflow. Its AGPL-3.0 terms must be reviewed before any code reuse; its Windows-specific platform integration is not directly portable to Android. Atlas is useful as a product reference for a clearly QoL-focused feature set, but no reusable client source was located.

These are architectural observations only. Do not copy Apollon source files, compiled assets, icons, fonts, branding, or distinctive UI artwork into AetherPvP unless the original rights holder provides a compatible licence or explicit written permission. Do not copy Flarial code unless its AGPL obligations and compatibility with the combined distribution are deliberately accepted.

## Risks and incompatibilities

- Apollon's source repository has no declared project licence and disclaims authorship.
- Apollon contains hard-coded, build-specific ARM64/ARMv7 offsets and native hooks. These are not portable compatibility data for the current Minecraft build.
- Its Android application package and platform integration are not the LeviLauncher native-mod interface.
- Its source includes modules intended to automate combat, alter movement/reach, reveal hidden entities/blocks, or interfere with servers. Those are excluded from AetherPvP's legitimate public-server feature scope.
- A second independent ImGui/OpenGL overlay would risk duplicate contexts, input conflicts, texture lifetime issues, and rendering instability alongside LeviLauncher/BedrockTools.
- Flarial's AGPL-3.0 licence and Windows-first codebase create separate legal and technical integration costs.

## AetherPvP integration decision

Keep LeviLauncher as the launcher and its existing mod manager/preloader as the loading path. Keep BedrockTools as the native module/runtime base. Recreate any desired menu layout independently inside the existing UI integration rather than importing Apollon's UI implementation.

Prioritise independently implemented, read-only or practice-oriented features: equipment/effect HUD, CPS and keystroke display, inventory counters, coordinates, ping display, combo/hit statistics, and local practice tools. Each native feature must have explicit version/dependency checks and be verified in an offline test world before release.

## Licensing item still open

BedrockTools' README currently identifies GPL-3.0, while its checked-in LICENSE text is MIT. Resolve this inconsistency with the actual upstream rights holder before publishing a combined redistribution. Keep all third-party notices and dependency licences intact.
