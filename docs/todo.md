# PrimeStage Implementation Plan (Prioritized)

Legend:
☐ Not started
◐ Started
☑ Complete

Priority:
- `P0` = blockers for API quality/correctness
- `P1` = important architecture/documentation alignment
- `P2` = lower-priority foundational cleanup

Release Exit Criteria (for API-quality milestone):
- no direct `PrimeFrame` node callback mutation required in app code for standard widgets
- focus behavior is default-correct and visibly rendered for all interactive widgets
- keyboard and pointer behavior are consistent across widgets and platforms
- CI covers build/test matrix, interaction regressions, and visual state regressions
- docs match shipped API and canonical build/test workflow

## P0 (Do First)

- ☑ [22] Define PrimeStage API ergonomics guidelines.
  - document what belongs in PrimeStage vs what user code should never have to implement
  - include examples for state ownership, interaction wiring, and focus semantics
  - delivered in `docs/api-ergonomics-guidelines.md` with regression coverage in `tests/unit/test_api_ergonomics.cpp`

- ☑ [11] Add a uniform interactive-control contract across widgets.
  - include first-class state + callbacks for toggle/checkbox/tabs/dropdown (`checked`/`selectedIndex`/`onChanged`)
  - ensure keyboard behavior is built-in for all interactive controls
  - define click + keyboard activation semantics (`Enter`/`Space`) consistently for button-like controls
  - acceptance: demo widgets no longer manually mutate app state from raw `PrimeFrame::Event` handlers
  - delivered by widget callback/state API updates in `include/PrimeStage/Ui.h`, built-in interaction handling in `src/PrimeStage.cpp`, and demo migration in `examples/primestage_widgets.cpp`

- ☑ [18] Make tabs and dropdown first-class interactive widgets.
  - add explicit callbacks (`onTabChanged`, `onOpened`, `onSelected`) and keyboard behavior
  - avoid attaching external pointer handlers to tab/dropdown child nodes
  - delivered by explicit callback APIs in `include/PrimeStage/Ui.h`, built-in interaction in `src/PrimeStage.cpp`, and demo/test migrations in `examples/primestage_widgets.cpp` and `tests/unit/test_tabs_dropdown.cpp`

- ☑ [12] Remove `PrimeFrame` callback plumbing from app-level widget usage.
  - provide PrimeStage-level event binding APIs so user code does not mutate node callbacks directly
  - acceptance: no `node->callbacks = ...` / callback-composition helpers needed in `examples/primestage_widgets.cpp`
  - delivered via widget-spec callbacks in `examples/primestage_widgets.cpp` with regression guard in `tests/unit/test_api_ergonomics.cpp`

- ☑ [19] Add core callback-composition utilities in PrimeStage.
  - expose safe helper APIs for extending existing widget callbacks (`onEvent`, `onFocus`, `onBlur`) without clobbering previous behavior
  - avoid ad-hoc callback composition in app code
  - delivered via `appendNodeOnEvent`, `appendNodeOnFocus`, and `appendNodeOnBlur` in `include/PrimeStage/Ui.h`/`src/PrimeStage.cpp` with regression coverage in `tests/unit/test_api_ergonomics.cpp`

- ☑ [13] Add stable widget identity and reconciliation support.
  - preserve focus/selection/caret across rebuilds without manual restore bookkeeping
  - acceptance: remove manual focus-target restore state from `examples/primestage_widgets.cpp`
  - delivered via `WidgetIdentityReconciler` in `include/PrimeStage/Ui.h`/`src/PrimeStage.cpp`, demo migration in `examples/primestage_widgets.cpp`, and regression coverage in `tests/unit/test_widget_identity.cpp` + `tests/unit/test_api_ergonomics.cpp`

- ☑ [17] Guarantee focus-ring compositing order.
  - ensure focus visuals render last/on top of selection/highlight primitives for all widgets
  - add regression tests that fail if selection highlight can draw above focus ring
  - delivered by explicit overlay top-order preservation in `src/PrimeStage.cpp` and flatten-order regression tests in `tests/unit/test_focus_compositing.cpp`

- ☑ [15] Add theme-level semantic defaults for interaction visuals.
  - define default focus ring styling for all focusable controls
  - reduce per-widget token opt-in for focus visuals
  - acceptance: focus ring appears for all focusable widgets without setting `focusStyle` per widget
  - delivered by semantic focus-style resolution in `src/PrimeStage.cpp` across focusable widget builders with regression coverage in `tests/unit/test_focus_widgets.cpp`

- ☑ [14] Add a PrimeStage input bridge layer for host integrations.
  - map host/platform events to widget events centrally (avoid per-app low-level event translation and keycode wiring)
  - replace raw numeric keycode handling with symbolic key enums in app-facing APIs
  - acceptance: app event loop no longer translates `PrimeHost` events to `PrimeFrame::Event` inline
  - delivered via `include/PrimeStage/InputBridge.h`, example migration in `examples/primestage_widgets.cpp`, and regression coverage in `tests/unit/test_input_bridge.cpp` + `tests/unit/test_api_ergonomics.cpp`

- ☑ [16] Add integration tests for real interaction flows.
  - cover click/type/rebuild/focus-retention scenarios in example-like app compositions
  - include focus-visual assertions (not only focused-node id assertions)
  - include explicit focus contract matrix: interaction-enabled widgets focusable by default; selectable text not focusable
  - delivered via `tests/unit/test_integration_flows.cpp` (end-to-end interaction/rebuild focus retention and focus-contract matrix coverage) and `CMakeLists.txt` test target updates

- ☑ [20] Add app-level interaction regression suite for the widgets demo.
  - scenario coverage: click-to-focus, tab traversal, value changes, text editing, tab/page switches, rebuild stability
  - include wheel + scrollbar interaction scenarios for overflow/scrollable regions
  - run in CI via `scripts/compile.sh --test`
  - delivered via `tests/unit/test_widgets_demo_regression.cpp` (demo-like page-flow/rebuild regression and wheel+scrollbar interaction coverage) and `CMakeLists.txt` test target updates

## P1 (Do Next)

- ☑ [23] Add a PrimeStage app runtime helper for frame lifecycle.
  - centralize rebuild/layout/render scheduling and dirty-flag handling
  - avoid ad-hoc `needsRebuild` / `needsLayout` / `needsFrame` orchestration in example apps
  - delivered via `include/PrimeStage/AppRuntime.h` (`FrameLifecycle` helper), migration in `examples/primestage_widgets.cpp`, and regression coverage in `tests/unit/test_app_runtime.cpp` + `tests/unit/test_api_ergonomics.cpp`

- ☑ [24] Resolve and record design decisions from open questions.
  - decide table vs specialized list strategy
  - decide window-chrome ownership (auto-generated vs composed widgets)
  - decide and document patch-operation safety whitelist
  - delivered via `docs/design-decisions.md`, `docs/prime-stage-design.md` alignment updates, and regression checks in `tests/unit/test_api_ergonomics.cpp`

- ☑ [21] Sync docs with current build/test workflow and API.
  - update `README.md` to promote `scripts/compile.sh` and `scripts/compile.sh --test`
  - reconcile `docs/prime-stage-design.md` with current API names/signatures (`TextField` vs draft `EditBox`, callback model, focus behavior)
  - delivered via README workflow updates, design-doc API/focus reconciliation in `docs/prime-stage-design.md`, and regression checks in `tests/unit/test_api_ergonomics.cpp`

- ☑ [25] Add visual regression coverage for interaction states.
  - add snapshot tests for focus/hover/pressed/selection layering
  - ensure regressions are catchable without manual runtime inspection
  - delivered via `tests/unit/test_visual_regression.cpp` and golden snapshot baseline `tests/snapshots/interaction_visuals.snap` with CMake wiring

- ☑ [31] Add CI/presubmit build matrix.
  - run `scripts/compile.sh --test` in at least `RelWithDebInfo` and `Release`
  - add coverage for `PRIMESTAGE_HEADLESS_ONLY=ON` and `PRIMESTAGE_ENABLE_PRIMEMANIFEST=OFF` compatibility path
  - fail fast on test regressions in the interaction/focus suites
  - delivered via `.github/workflows/presubmit.yml` (release/relwithdebinfo matrix + headless compatibility job) and workflow regression checks in `tests/unit/test_api_ergonomics.cpp`

- ☑ [32] Add toolchain quality gates.
  - add sanitizer configuration (`ASan`/`UBSan`) for tests
  - add static checks for warnings in project-owned code paths
  - keep third-party warnings isolated (do not block local code quality gate)
  - delivered via CMake quality-gate options in `CMakeLists.txt`, CLI wiring in `scripts/compile.sh`, presubmit `toolchain-quality` job in `.github/workflows/presubmit.yml`, and regression checks in `tests/unit/test_api_ergonomics.cpp`

- ☑ [37] Add API validation and diagnostics for widget specs.
  - detect invalid/inconsistent spec inputs (negative sizes, impossible ranges, invalid selection indices)
  - provide debug diagnostics/assertions and safe runtime fallbacks
  - add unit tests for validation behavior
  - delivered via spec sanitizers/index clamps in `src/PrimeStage.cpp`, runtime validation coverage in `tests/unit/test_spec_validation.cpp`, and source/docs guard checks in `tests/unit/test_api_ergonomics.cpp`

- ☑ [27] Define and implement disabled/read-only widget semantics.
  - add common `enabled`/`readOnly` behavior contract for interactive widgets where applicable
  - ensure disabled controls are not focusable/clickable and have consistent visuals
  - delivered via widget-spec API updates in `include/PrimeStage/Ui.h`, disabled/read-only interaction + scrim semantics in `src/PrimeStage.cpp`, and regression coverage in `tests/unit/test_interaction.cpp` + `tests/unit/test_api_ergonomics.cpp`

- ☑ [28] Expose focus-order API at widget level.
  - add user-facing `tabIndex`/focus-order controls without requiring direct `PrimeFrame` node access
  - include tests for deterministic tab order across mixed widget sets
  - delivered via `tabIndex` fields in focusable specs in `include/PrimeStage/Ui.h`, focus-order/clamp wiring in `src/PrimeStage.cpp`, and mixed-widget tab-order regression coverage in `tests/unit/test_focus_widgets.cpp` + `tests/unit/test_api_ergonomics.cpp`

- ☑ [29] Define controlled vs uncontrolled widget state model.
  - document when PrimeStage owns transient widget state vs when user code provides it
  - reduce full-app state bookkeeping for common controls (toggle/checkbox/dropdown/tabs)
  - delivered via state-backed widget APIs (`ToggleState`/`CheckboxState`/`TabsState`/`DropdownState`) in `include/PrimeStage/Ui.h`, runtime state ownership wiring in `src/PrimeStage.cpp`, and coverage in `tests/unit/test_interaction.cpp`, `tests/unit/test_tabs_dropdown.cpp`, and `tests/unit/test_api_ergonomics.cpp` with docs updates in `docs/api-ergonomics-guidelines.md` and `docs/prime-stage-design.md`

- ☑ [33] Normalize cross-platform input semantics in PrimeStage.
  - standardize key naming/translation (no hard-coded numeric key constants in examples)
  - document and test pointer scroll direction/units consistently across host backends
  - delivered via shared `KeyCode` semantics in `include/PrimeStage/Ui.h`, input normalization updates in `include/PrimeStage/InputBridge.h`, internal/test migration away from raw key literals in `src/PrimeStage.cpp` + input tests, and docs/test guards in `docs/api-ergonomics-guidelines.md`, `docs/prime-stage-design.md`, and `tests/unit/test_api_ergonomics.cpp`

- ☑ [35] Add IME/composition support plan for text input controls.
  - define API for composition start/update/commit/cancel
  - add tests for non-ASCII and composition-heavy input workflows
  - delivered via composition API planning types in `include/PrimeStage/Ui.h`, implementation plan in `docs/ime-composition-plan.md` with design/guideline links, and UTF-8/composition-like workflow regression coverage in `tests/unit/test_text_field.cpp` + source/docs guards in `tests/unit/test_api_ergonomics.cpp`

- ☑ [36] Add accessibility semantics roadmap.
  - define widget role/state metadata model (for future HTML/backend accessibility integration)
  - ensure focus order and activation semantics map cleanly to accessibility expectations
  - delivered via accessibility metadata model types in `include/PrimeStage/Ui.h`, roadmap doc `docs/accessibility-semantics-roadmap.md`, design/guideline links in docs, and behavior/source guards in `tests/unit/test_interaction.cpp` + `tests/unit/test_api_ergonomics.cpp`

- ☑ [38] Add performance benchmarks and budgets.
  - benchmark rebuild/layout/render cost for representative widget scenes
  - set target budgets for interaction-heavy flows (typing, drag, wheel scroll)
  - gate regressions in CI with trend/budget checks where feasible
  - delivered via benchmark harness `tests/perf/PrimeStage_benchmarks.cpp`, budget thresholds in `tests/perf/perf_budgets.txt`, CLI workflow wiring in `scripts/compile.sh`, CI budget gate in `.github/workflows/presubmit.yml`, documentation in `docs/performance-benchmarks.md`, and source/docs regression guards in `tests/unit/test_api_ergonomics.cpp`

- ☑ [39] Add deterministic visual-test harness.
  - stabilize snapshot rendering inputs (fonts/theme/scale) to reduce platform-dependent drift
  - define golden update workflow and failure triage guidance
  - delivered via deterministic harness helpers in `tests/unit/visual_test_harness.h`, visual regression migration in `tests/unit/test_visual_regression.cpp` with harness metadata block in `tests/snapshots/interaction_visuals.snap`, workflow/triage doc in `docs/visual-test-harness.md`, and source/docs guard coverage in `tests/unit/test_api_ergonomics.cpp`

- ☑ [40] Define API evolution and compatibility policy.
  - document semver expectations and deprecation process for spec/callback changes
  - provide migration notes when replacing draft APIs (for example `EditBox` naming in design docs)
  - delivered via compatibility policy doc `docs/api-evolution-policy.md` (semver/deprecation/checklist + `EditBox` -> `TextField` migration notes), design/guideline/readme links in docs, and regression guards in `tests/unit/test_api_ergonomics.cpp`

- ☑ [41] Add install/export/package support for PrimeStage.
  - add `install()` rules and CMake package exports for library + public headers
  - verify consumer integration with `find_package(PrimeStage)` style workflow
  - delivered via install/export/package config wiring in `CMakeLists.txt` + `cmake/PrimeStageConfig.cmake.in`, consumer smoke assets in `tests/cmake/find_package_smoke/*` with ctest driver `tests/cmake/run_find_package_smoke.cmake`, packaging docs in `docs/cmake-packaging.md`, and regression guards in `tests/unit/test_api_ergonomics.cpp`

- ☑ [42] Define callback reentrancy/threading guarantees.
  - document which callbacks may trigger rebuild/layout and in what execution context
  - add guardrails/tests for callback-triggered state changes to avoid reentrancy bugs
  - delivered via callback contract doc `docs/callback-reentrancy-threading.md`, docs alignment in `docs/api-ergonomics-guidelines.md` + `docs/prime-stage-design.md`, runtime reentrancy suppression guardrails in `src/PrimeStage.cpp`/`include/PrimeStage/Ui.h` for callback composition helpers, and regression coverage in `tests/unit/test_api_ergonomics.cpp`

- ☑ [48] Define data ownership/lifetime contracts in public specs.
  - document safe usage rules for `std::string_view` fields and callback captures
  - eliminate dangling-view hazards by copying where required or introducing owned-string alternatives
  - add tests that exercise rebuilds/callbacks with short-lived source strings
  - delivered via ownership contract doc `docs/data-ownership-lifetime.md`, guideline/design/readme/agents alignment, owned-row backing for table callback payloads in `src/PrimeStage.cpp`, and short-lived source + docs/source guard coverage in `tests/unit/test_interaction.cpp` and `tests/unit/test_api_ergonomics.cpp`

- ☑ [43] Improve render API diagnostics and failure reporting.
  - replace opaque `bool` failures with structured error/status information
  - expose actionable error context for PNG write/layout/render failures
  - delivered via structured render diagnostics API (`RenderStatusCode`/`RenderStatus` + `renderStatusMessage`) in `include/PrimeStage/Render.h` and `src/Render.cpp`, benchmark-callsite migration in `tests/perf/PrimeStage_benchmarks.cpp`, render status regression tests in `tests/unit/test_render.cpp`, docs in `docs/render-diagnostics.md` with README/design/agents alignment, and source/docs guard coverage in `tests/unit/test_api_ergonomics.cpp`

- ☑ [44] Remove renderer style heuristics tied to theme color indices.
  - replace color-index-based rounded-corner inference with explicit style metadata
  - ensure renderer output is deterministic under theme changes
  - delivered via explicit `CornerStyleMetadata` in `include/PrimeStage/Render.h`, geometry/metadata-driven radius resolution in `src/Render.cpp` (theme-color heuristics removed), deterministic theme-change regression coverage in `tests/unit/test_render.cpp`, and docs/guidance alignment in `docs/render-diagnostics.md`, `docs/prime-stage-design.md`, and `AGENTS.md`

- ☐ [45] Add render-path test coverage.
  - add tests for `renderFrameToTarget` and `renderFrameToPng` success/failure paths
  - include headless (`PRIMESTAGE_ENABLE_PRIMEMANIFEST=OFF`) behavior expectations

- ☐ [46] Establish single-source versioning.
  - derive runtime version string from project version metadata
  - add checks to prevent drift between CMake version, `getVersionString()`, and tests

- ☐ [47] Make dependency resolution reproducible.
  - avoid floating `master` defaults for fetched dependencies in release-grade workflows
  - document pin/update policy for PrimeFrame/PrimeHost/PrimeManifest revisions

- ☐ [49] Add property/fuzz testing for input and focus state machines.
  - generate mixed pointer/keyboard/text event sequences and assert invariants (no crashes, valid focus, stable selection bounds)
  - include regression corpus for previously observed focus/callback bugs

- ☐ [50] Make example apps canonical API consumers.
  - ensure examples avoid direct `PrimeFrame` internals except where explicitly documented as advanced
  - add review checklist/tests to prevent examples from reintroducing callback/focus workarounds

- ☐ [7] Implement window builder (stateless) + callback wiring.

- ☐ [10] Final pass: validate naming rules, add minimal API docs, and run tests.

## P2 (Foundational Cleanup / Backlog)

- ☐ [26] Add patch-first update path for high-frequency text/selection edits.
  - avoid full-scene rebuild on every text/caret update where structural changes are not required

- ☐ [30] Expand patch-first updates beyond text fields.
  - support non-structural interaction updates (toggle/checkbox/slider/progress visual state) without full rebuilds

- ☐ [34] Keep generated/build artifacts out of source control workflows.
  - document expected ignored paths for build outputs and snapshots
  - add cleanup guidance/script for local dev loops

- ☐ [1] Establish core ids, enums, and shared widget specs.
- ☐ [2] Define callback table + RAII callback handle.
- ☐ [3] Implement `UiNode` fluent builder for `Frame` authoring.
- ☐ [4] Implement widget creation helpers (panel, label, divider, spacer).
- ☐ [5] Implement interactive widgets (button, edit box, toggle, checkbox, slider).
- ☐ [6] Implement collection widgets (scroll view, list, table, tree).
- ☐ [8] Add minimal unit tests for builder API.
- ☐ [9] Add example scene build demonstrating window + widgets.
