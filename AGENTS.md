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
- Local artifact cleanup: use `scripts/clean.sh` (`--dry-run` to preview, `--all` to also remove `.cache/` and `compile_commands.json`).
- Debug: `cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug` then `cmake --build build-debug`.
- Release: `cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release` then `cmake --build build-release`.
- Install/package: `cmake --install <build-dir> --prefix <install-prefix>`; use `find_package(PrimeStage CONFIG REQUIRED)` from consumer builds.
- Options: `-DPRIMESTAGE_BUILD_TESTS=ON/OFF`, `-DPRIMESTAGE_BUILD_BENCHMARKS=ON/OFF`, `-DPRIMESTAGE_BUILD_EXAMPLES=ON/OFF`, `-DPRIMESTAGE_WARNINGS_AS_ERRORS=ON/OFF`, `-DPRIMESTAGE_ENABLE_ASAN=ON/OFF`, `-DPRIMESTAGE_ENABLE_UBSAN=ON/OFF`.
- Tests: from a build dir, run `ctest --output-on-failure` or execute `./PrimeStage_tests` directly.

## Tests
- Unit tests live in `tests/unit` and use doctest.
- Visual snapshot updates use `PRIMESTAGE_UPDATE_SNAPSHOTS=1 ./scripts/compile.sh --test`; follow `docs/visual-test-harness.md` for deterministic harness inputs and triage workflow.
- End-to-end high-level API ergonomics coverage lives in `tests/unit/test_end_to_end_ergonomics.cpp`; keep this suite high-level-only (no `PrimeStage::LowLevel` usage) and ensure it stays wired in CI smoke gates.
- When writing collection-interaction regression helpers, prefer descendant row callbacks over
  container callbacks because `createTable`/`createList` may compose keyboard handlers at parent
  nodes.

## Code guidelines
- Target C++23; prefer value semantics, RAII, `std::span`, and `std::optional`/`std::expected`.
- Use `std::chrono` types for durations/timeouts.
- Keep hot paths allocation-free; reuse buffers and caches where possible.
- Avoid raw `new`; use `std::unique_ptr` or `std::shared_ptr` with clear ownership intent.
- For public widget specs, clamp invalid inputs to safe runtime fallbacks and emit debug diagnostics; add unit tests for new validation paths.
- For new widget APIs, derive spec structs from shared bases (`WidgetSpec`, `EnableableWidgetSpec`, `FocusableWidgetSpec`) instead of duplicating accessibility/visibility/enablement fields.
- For new/changed widget APIs, satisfy `docs/widget-api-review-checklist.md` and complete the widget checklist in `.github/pull_request_template.md`.
- For public API/spec/callback changes, follow `docs/api-evolution-policy.md` (semver impact classification, staged deprecation, and migration notes).
- For callback composition/reentrancy/threading behavior, follow `docs/callback-reentrancy-threading.md` and keep callback state changes single-thread-safe.
- For low-level node callback overrides, use `PrimeStage::LowLevel::NodeCallbackTable` + `PrimeStage::LowLevel::NodeCallbackHandle` so previous callback tables are restored automatically via RAII.
- Keep low-level callback composition quarantined under `PrimeStage::LowLevel` (`LowLevel::appendNodeOnEvent`, `LowLevel::NodeCallbackHandle`, etc.); canonical/high-level app code should not use these APIs.
- For `std::string_view` and callback-capture lifetime safety, follow `docs/data-ownership-lifetime.md` and prefer owned captures/state for post-build callback use.
- For render-path failures, use `RenderStatus`/`RenderStatusCode` diagnostics (`docs/render-diagnostics.md`) instead of introducing new opaque `bool` failure paths.
- For renderer corner rounding behavior, use explicit `RenderOptions::cornerStyle` metadata; do not reintroduce theme-color index heuristics.
- Keep runtime version reporting (`getVersion()` / `getVersionString()`) derived from CMake project metadata via generated version headers; avoid hard-coded version literals in source/tests.
- Keep FetchContent dependency defaults pinned to immutable commits and update `docs/dependency-resolution-policy.md` whenever `*_GIT_TAG` values change.
- Keep input/focus fuzz/property tests deterministic (fixed RNG seeds + explicit regression corpus) so failures are reproducible.
- Keep examples split by tier: `examples/canonical` must stay high-level PrimeStage-only, while
  host/frame internals live in `examples/advanced`; any direct PrimeFrame integration must be
  explicitly tagged inline with `Advanced PrimeFrame integration (documented exception):`; this
  tag also covers direct PrimeHost runtime integration in advanced examples, and
  aligned with `docs/example-app-consumer-checklist.md`.
- Keep canonical examples free of manual lifecycle scheduling calls (`requestRebuild`,
  `requestLayout`, `requestFrame`) for ordinary interactions; advanced examples may use them only
  when tagged inline as `Advanced lifecycle orchestration (documented exception):`.
- Keep onboarding docs split and aligned with shipped examples: update
  `docs/5-minute-app.md` for canonical-first guidance and
  `docs/advanced-escape-hatches.md` for low-level paths whenever tier guidance changes.
- Keep canonical API-surface linting current in `scripts/lint_canonical_api_surface.sh` and run it
  when changing canonical examples/docs snippets or low-level API naming.
- Keep `docs/widget-spec-defaults-audit.md` synchronized with `include/PrimeStage/Ui.h` whenever
  spec fields are added/renamed/reclassified.
- For canonical example composition, prefer intrinsic widget defaults plus `size.maxWidth` container/text constraints over repeated per-widget `preferredWidth` micromanagement.
- For settings-like canonical UI flows, prefer `UiNode::form(...)` + `UiNode::formField(...)` over manual row/column label-control-help assembly.
- Keep canonical API ergonomics within `docs/api-ergonomics-scorecard.md` thresholds; update scorecard docs and enforcing tests together when intentionally changing budgets.
- Keep widget interaction defaults aligned with `docs/default-widget-behavior-matrix.md`; when default focus/keyboard/pointer/accessibility behavior changes, update the matrix and its enforcing tests in the same change.
- Keep `ProgressBar` patch-first visuals stable: at zero value with `minFillWidth == 0`, keep the
  fill node allocated but hidden (do not remove/recreate it ad hoc).
- For `TextFieldSpec` / `SelectableTextSpec`, prefer owned-default state (`spec.ownedState`) in canonical examples and reserve raw `state` pointers for explicit compatibility/advanced control paths.
- For `PrimeStage::App`-based host loops, prefer `connectHostServices(...)` + `applyPlatformServices(...)` for text widgets instead of per-widget clipboard/cursor host lambdas.
- For keyboard shortcuts and cross-widget commands in canonical host loops, prefer `PrimeStage::App` action routing (`registerAction`, `bindShortcut`, `makeActionCallback`) over ad-hoc raw key event branching.
- Keep `AppActionInvocation` callback payload lifetimes safe: `actionId` should remain owned data, not a borrowed transient view.
- For `createWindow(WindowSpec)` usage, keep window geometry in app-owned durable state and treat move/resize callbacks as stateless deltas that drive runtime rebuild/layout requests.
- Keep `docs/minimal-api-reference.md` aligned with shipped public headers (`PrimeStage.h`, `Ui.h`, `AppRuntime.h`, `InputBridge.h`, `Render.h`) whenever API symbols change.
- For templated ergonomic entry points (`bind(...)`, `makeListModel(...)`, `makeTableModel(...)`, `makeTreeModel(...)`), keep compile-time validators and static-assert diagnostics concise, actionable, and linked to `docs/minimal-api-reference.md`.
- Prefer patch-first frame updates for high-frequency `TextField` edits plus non-structural `Toggle`/`Checkbox`/`Slider`/state-backed `ProgressBar` interaction visuals, and reserve full rebuild requests for structural UI changes.
