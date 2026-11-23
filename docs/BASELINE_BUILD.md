# Baseline build request

This documentation-only commit intentionally triggers the existing push workflow on `main`.

Purpose: build the unchanged BedrockTools source for Android ARM64 using the repository's configured Android NDK, then inspect the build result and `.levipack` artifact before runtime changes.

No gameplay source, signatures, offsets, or runtime behavior are modified by this commit.
