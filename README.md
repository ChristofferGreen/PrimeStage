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

## Tests

From a build dir:

```sh
ctest --output-on-failure
```

Or run the test binary directly:

```sh
./PrimeStage_tests
```

## Spec Validation

PrimeStage clamps invalid widget-spec inputs to safe values at runtime (for example negative sizes,
inverted min/max bounds, and out-of-range selection indices). Debug builds also emit validation
diagnostics.

## Docs
- `docs/prime-stage-design.md`
- `docs/api-ergonomics-guidelines.md`
- `docs/design-decisions.md`
- `docs/performance-benchmarks.md`
- `docs/visual-test-harness.md`
