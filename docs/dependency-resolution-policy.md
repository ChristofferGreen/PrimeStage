# PrimeStage Dependency Resolution Policy

## Scope

PrimeStage uses FetchContent for PrimeFrame, PrimeHost, and PrimeManifest. To keep release-grade
builds reproducible, default dependency refs must be pinned to immutable commit hashes.

## Default Pins

Current default refs in `CMakeLists.txt`:

- `PRIMEFRAME_GIT_TAG=180a3c2ec0af4b56eba1f5e74b4e74ba90efdecc`
- `PRIMEHOST_GIT_TAG=762dbfbc77cd46a009e8a9b352404ffe7b81e66e`
- `PRIMESTAGE_PRIMEMANIFEST_GIT_TAG=4e65e2b393b63ec798f35fdd89f6f32d2205675c`

Do not use floating defaults such as `master`, `main`, or unqualified tags for release workflows.

## Update Policy

When updating dependency revisions:

1. Pick explicit commit hashes for each dependency.
2. Update the corresponding `*_GIT_TAG` values in `CMakeLists.txt`.
3. Run `./scripts/compile.sh --test` and `./scripts/compile.sh`.
4. Update this document's pin list in the same change.
5. Document behavior changes from dependency bumps in release notes/changelog when applicable.

## Override Policy

Local experimentation may override refs via CMake cache flags, for example:

```bash
cmake -S . -B build-debug \
  -DPRIMEFRAME_GIT_TAG=<commit> \
  -DPRIMEHOST_GIT_TAG=<commit> \
  -DPRIMESTAGE_PRIMEMANIFEST_GIT_TAG=<commit>
```

Such overrides are not a substitute for updating pinned defaults in source control.
