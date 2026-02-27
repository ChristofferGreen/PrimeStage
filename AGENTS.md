# AGENTS.md for PrimeStage

## Scope
Defines naming and coding rules plus build/test entrypoints for contributors working in this repo.

## Naming rules
- **Namespaces:** PascalCase for public namespaces (e.g., `PrimeStage`).
- **Types/structs/classes/enums/aliases:** PascalCase.
- **Functions (free/member/static):** lowerCamelCase.
- **Struct/class fields:** lowerCamelCase.
- **Local/file-only helpers:** lower_snake_case is acceptable when marked `static` in a `.cpp`.
- **Constants (`constexpr`/`static`):** PascalCase; avoid `k`-prefixes.
- **Macros:** avoid new ones; if unavoidable, use `PS_` prefix and ALL_CAPS.

## Build/test workflow
- Preferred build: `scripts/compile.sh [--debug|--release|--relwithdebinfo|--minsizerel] [--test] [--warnings-as-errors] [--asan] [--ubsan] [--perf|--perf-budget]` (uses clang, builds examples by default, tests/benchmarks built by default; sanitizer mode disables examples and `--test` runs tests).
- Debug: `cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug` then `cmake --build build-debug`.
- Release: `cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release` then `cmake --build build-release`.
- Install/package: `cmake --install <build-dir> --prefix <install-prefix>`; use `find_package(PrimeStage CONFIG REQUIRED)` from consumer builds.
- Options: `-DPRIMESTAGE_BUILD_TESTS=ON/OFF`, `-DPRIMESTAGE_BUILD_BENCHMARKS=ON/OFF`, `-DPRIMESTAGE_BUILD_EXAMPLES=ON/OFF`, `-DPRIMESTAGE_WARNINGS_AS_ERRORS=ON/OFF`, `-DPRIMESTAGE_ENABLE_ASAN=ON/OFF`, `-DPRIMESTAGE_ENABLE_UBSAN=ON/OFF`.
- Tests: from a build dir, run `ctest --output-on-failure` or execute `./PrimeStage_tests` directly.

## Tests
- Unit tests live in `tests/unit` and use doctest.
- Visual snapshot updates use `PRIMESTAGE_UPDATE_SNAPSHOTS=1 ./scripts/compile.sh --test`; follow `docs/visual-test-harness.md` for deterministic harness inputs and triage workflow.

## Code guidelines
- Target C++23; prefer value semantics, RAII, `std::span`, and `std::optional`/`std::expected`.
- Use `std::chrono` types for durations/timeouts.
- Keep hot paths allocation-free; reuse buffers and caches where possible.
- Avoid raw `new`; use `std::unique_ptr` or `std::shared_ptr` with clear ownership intent.
- For public widget specs, clamp invalid inputs to safe runtime fallbacks and emit debug diagnostics; add unit tests for new validation paths.
- For public API/spec/callback changes, follow `docs/api-evolution-policy.md` (semver impact classification, staged deprecation, and migration notes).
- For callback composition/reentrancy/threading behavior, follow `docs/callback-reentrancy-threading.md` and keep callback state changes single-thread-safe.
- For `std::string_view` and callback-capture lifetime safety, follow `docs/data-ownership-lifetime.md` and prefer owned captures/state for post-build callback use.
- For render-path failures, use `RenderStatus`/`RenderStatusCode` diagnostics (`docs/render-diagnostics.md`) instead of introducing new opaque `bool` failure paths.
- For renderer corner rounding behavior, use explicit `RenderOptions::cornerStyle` metadata; do not reintroduce theme-color index heuristics.
- Keep runtime version reporting (`getVersion()` / `getVersionString()`) derived from CMake project metadata via generated version headers; avoid hard-coded version literals in source/tests.
- Keep FetchContent dependency defaults pinned to immutable commits and update `docs/dependency-resolution-policy.md` whenever `*_GIT_TAG` values change.
