# CI activation check

GitHub Actions was enabled for this fork after the initial unchanged-source baseline-build trigger commit. This follow-up commit exists solely to create a fresh push event after Actions was enabled.

The expected workflow is `.github/workflows/build.yml`, which configures an Android ARM64 release build using NDK r28c and uploads `BedrockTools.levipack`.

A commit is not evidence of a successful build. Check the Actions run and artifact before treating the baseline build as validated.
