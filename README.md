# PrimeStage

PrimeStage is the UI authoring layer that owns ground-truth widget state and emits rect hierarchies for PrimeFrame. The Studio UI kit now lives in the PrimeStudio repo.

## Build And Test (Preferred)

Use the project build script as the canonical workflow:

```sh
./scripts/compile.sh
```

Run full tests:

```sh
./scripts/compile.sh --test
```

Build-type options:
- `./scripts/compile.sh --debug`
- `./scripts/compile.sh --release`
- `./scripts/compile.sh --relwithdebinfo`
- `./scripts/compile.sh --minsizerel`

Quality-gate options:
- `./scripts/compile.sh --warnings-as-errors`
- `./scripts/compile.sh --asan --ubsan --test` (sanitizer mode builds tests with examples disabled)
- `./scripts/compile.sh --release --perf-budget` (runs benchmark suite and enforces p95 budgets)
- `./scripts/lint_canonical_api_surface.sh` (forbidden low-level API usage guard in canonical examples/docs snippets)

Clean local generated artifacts:
- `./scripts/clean.sh --dry-run`
- `./scripts/clean.sh`
- `./scripts/clean.sh --all`

## Direct CMake (Optional)

```sh
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug
```

```sh
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
```

Optional configuration:
- `-DPRIMESTAGE_BUILD_TESTS=ON/OFF` toggles tests.
- `-DPRIMESTAGE_BUILD_EXAMPLES=ON/OFF` toggles example binaries.
- `-DPRIMESTAGE_HEADLESS_ONLY=ON/OFF` skips PrimeManifest (no rendering). Defaults to `OFF`.
- `-DPRIMESTAGE_ENABLE_PRIMEMANIFEST=ON/OFF` toggles PrimeManifest integration. Required unless headless mode is on.
- `-DPRIMESTAGE_WARNINGS_AS_ERRORS=ON/OFF` enables warning gates on PrimeStage-owned targets.
- `-DPRIMESTAGE_ENABLE_ASAN=ON/OFF` enables AddressSanitizer for test builds.
- `-DPRIMESTAGE_ENABLE_UBSAN=ON/OFF` enables UndefinedBehaviorSanitizer for test builds.
- `-DPRIMESTAGE_BUILD_BENCHMARKS=ON/OFF` toggles the `PrimeStage_benchmarks` executable.

## Example Binaries

With examples enabled (`-DPRIMESTAGE_BUILD_EXAMPLES=ON`), the build provides:

Canonical tier (start here):
- `primestage_modern_api` (default strict high-level canonical example; writes a PNG snapshot)
- `primestage_example` (version smoke example)

Advanced tier (host/frame integration samples):
- `primestage_widgets` (full host/render interactive demo)
- `primestage_scene` (window + widgets scene composition example with direct frame/layout wiring)

Default quick start:

```sh
./build-debug/primestage_modern_api
```

Onboarding guides:
- `docs/5-minute-app.md` (recommended canonical path)
- `docs/advanced-escape-hatches.md` (advanced host/frame escape hatches)

## Tests

From a build dir:

```sh
ctest --output-on-failure
```

Or run the test binary directly:

```sh
./PrimeStage_tests
```

## Install And Package

Install PrimeStage and exported CMake package files:

```sh
cmake --install build-release --prefix /tmp/primestage-install
```

Downstream consumers can use:

```cmake
find_package(PrimeStage CONFIG REQUIRED)
target_link_libraries(app PRIVATE PrimeStage::PrimeStage)
```

## Spec Validation

PrimeStage clamps invalid widget-spec inputs to safe values at runtime (for example negative sizes,
inverted min/max bounds, and out-of-range selection indices). Debug builds also emit validation
diagnostics.

## Docs
- `docs/prime-stage-design.md`
- `docs/api-ergonomics-guidelines.md`
- `docs/callback-reentrancy-threading.md`
- `docs/data-ownership-lifetime.md`
- `docs/render-diagnostics.md`
- `docs/dependency-resolution-policy.md`
- `docs/api-evolution-policy.md`
- `docs/design-decisions.md`
- `docs/performance-benchmarks.md`
- `docs/visual-test-harness.md`
- `docs/cmake-packaging.md`
- `docs/build-artifact-hygiene.md`
- `docs/5-minute-app.md`
- `docs/advanced-escape-hatches.md`
- `docs/example-app-consumer-checklist.md`
- `docs/widget-api-review-checklist.md`
- `docs/minimal-api-reference.md`
