# PrimeStage API Ergonomics Guidelines

## Goal
PrimeStage should let apps build interactive UI without mutating `PrimeFrame` internals directly.
App code should describe state and callbacks at the widget-spec level, while PrimeStage owns node setup and callback plumbing.
Public API compatibility and migration policy is tracked in `docs/api-evolution-policy.md`.
Ergonomics quality budgets and regression gates are tracked in `docs/api-ergonomics-scorecard.md`.
Structured spec-field defaults audit is tracked in `docs/widget-spec-defaults-audit.md`.
Widget/control API merge requirements are tracked in `docs/widget-api-review-checklist.md`.
End-to-end high-level input ergonomics regressions are gated in
`tests/unit/test_end_to_end_ergonomics.cpp`.
Onboarding split is documented in `docs/5-minute-app.md` (recommended canonical path) and
`docs/advanced-escape-hatches.md` (advanced-only escape hatches).

## Responsibility Split
PrimeStage responsibilities:
- Build `PrimeFrame` nodes/primitives from widget specs.
- Install widget event handlers from spec callbacks.
- Apply widget-local interaction visuals (hover, pressed, focus ring).
- Expose focusable behavior on interactive widgets by default.
- Provide a high-level app shell (`PrimeStage::App`) that owns frame/layout/router/focus wiring.

Application responsibilities:
- Own durable domain state and widget state objects.
- Provide callback implementations that update app state.
- Trigger rebuild/layout/frame intent via `PrimeStage::App::lifecycle()`.
- Feed host input through `PrimeStage::App::bridgeHostInputEvent(...)`.
- Configure app-level platform services once (`AppPlatformServices` or `connectHostServices(...)`)
  and apply them to text specs via `applyPlatformServices(...)`.

What app code should avoid:
- Writing `node->callbacks = ...` directly for standard widgets.
- Composing low-level `PrimeFrame::Callback` chains in feature code.
- Storing/querying raw `NodeId` in canonical app code when typed widget handles are sufficient.
- Rebuilding `std::vector<std::string_view>` item/row mirrors manually for list/table/tree on every frame.
- Re-implementing focus visuals per widget by patching primitives.
- Translating raw key constants inside each feature/widget usage site.
- Owning `PrimeFrame::Frame`/`LayoutEngine`/`EventRouter`/`FocusManager` in canonical app paths.

## State Ownership Rules
- PrimeStage owns transient interaction state (`pressed`, `hovered`, drag/session flags) during one built scene.
- Application code owns durable domain state and any widget state that must survive rebuilds.
- Controlled mode:
  - pass value fields in specs (`on`, `checked`, `selectedIndex`) and handle callbacks to update app state.
  - use this when your app already has canonical domain state.
- Binding mode (preferred for value widgets):
  - pass `State<T>` storage through `bind(...)` (for example `toggle.binding = bind(state.flag)`).
  - supported on `Toggle`, `Checkbox`, `Slider`, `Tabs`, `Dropdown`, and `ProgressBar`.
  - PrimeStage reads/writes bound values directly during interaction; callbacks are optional.
  - use this as the default modern C++ value-ownership path for interactive controls.
- Owned-default mode (text widgets):
  - omit raw `state` pointers on `TextFieldSpec` and `SelectableTextSpec`; PrimeStage provisions
    owned interaction state so controls remain interactive by default.
  - for rebuild-persistent text interaction state without raw pointers, store
    `std::shared_ptr<TextFieldState>` / `std::shared_ptr<SelectableTextState>` in app state and
    assign `spec.ownedState`.
  - use this for minimal default widget usage; migrate to explicit app-owned/raw state only when
    you need custom lifetime/control semantics.
- State-backed mode (legacy convenience for compatibility):
  - pass widget state pointers (`ToggleState*`, `CheckboxState*`, `SliderState*`, `TabsState*`,
    `DropdownState*`, `ProgressBarState*`, `TextFieldState*`, `SelectableTextState*`).
  - PrimeStage reads/writes those state objects directly during interaction; callbacks are optional.
  - use this to reduce callback-driven bookkeeping when the widget value does not need separate
    app-level mirroring.
- Rebuild safely at any time; durable state must survive scene disposal.
- Use `WidgetIdentityReconciler` for focus reconciliation across rebuilds instead of ad-hoc
  node-id tracking enums.
- For stable identity keys, use `widgetIdentityId("feature.widget")` when you need a compact
  numeric id in app/runtime state while keeping string identities at API edges.
- For text composition/IME planning, see `docs/ime-composition-plan.md`.
- For accessibility role/state planning, see `docs/accessibility-semantics-roadmap.md`.
- For `std::string_view`/callback-capture lifetime rules, see `docs/data-ownership-lifetime.md`.

Example (`TextField` with owned defaults + optional retained state):

```cpp
struct AppState {
  std::shared_ptr<PrimeStage::TextFieldState> nameField;
  std::string status;
};

void buildUi(PrimeStage::UiNode root, AppState& state) {
  if (!state.nameField) {
    state.nameField = std::make_shared<PrimeStage::TextFieldState>();
  }
  PrimeStage::TextFieldSpec field;
  field.ownedState = state.nameField;
  field.placeholder = "Name";
  field.size.preferredWidth = 220.0f;
  field.size.preferredHeight = 28.0f;
  field.callbacks.onChange = [&](std::string_view text) {
    state.status = std::string(text);
  };
  root.createTextField(field);
}
```

Example (`Toggle` in binding mode):

```cpp
struct AppState {
  PrimeStage::State<bool> autoSave;
};

void buildUi(PrimeStage::UiNode root, AppState& state) {
  PrimeStage::ToggleSpec toggle;
  toggle.binding = PrimeStage::bind(state.autoSave);
  root.createToggle(toggle);
}
```

Example (`WidgetIdentityReconciler` around rebuilds):

```cpp
PrimeStage::WidgetIdentityReconciler widgetIdentity;

void rebuild(App& app) {
  widgetIdentity.beginRebuild(app.focus.focusedNode());
  app.frame = PrimeFrame::Frame();

  PrimeStage::UiNode root(app.frame, app.frame.createNode(), true);
  PrimeStage::UiNode field = root.createTextField(fieldSpec);
  widgetIdentity.registerNode("settings.field", field.lowLevelNodeId());
}

void afterLayout(App& app) {
  app.focus.updateAfterRebuild(app.frame, app.layout);
  widgetIdentity.restoreFocus(app.focus, app.frame, app.layout);
}
```

## Interaction Wiring Rules
- Prefer semantic widget spec callbacks (`onActivate`, `onChange`, `onOpen`, `onSelect`) over node event hooks.
- Legacy callback names are still accepted as compatibility aliases, but new code should use semantic names.
- For callback alias migration mapping and before/after examples, see
  `docs/semantic-callback-migration.md`.
- Keep callbacks side-effect-light: update state and mark the app for rebuild/layout.
- Use widget-provided callback payloads (`TableRowInfo`, `TreeViewRowInfo`) instead of hit-test math in app code.
- Prefer app-level action routing for cross-widget commands and shortcuts:
  `registerAction(...)`, `bindShortcut(...)`, and `invokeAction(...)`.
- Reuse action ids across widget callbacks and shortcuts using `makeActionCallback(...)` instead of
  duplicating command behavior in multiple callback sites.
- `AppActionInvocation::actionId` is owned payload data; callbacks may safely store/copy the
  invocation for post-dispatch processing.
- For advanced/internal extension points, use `PrimeStage::LowLevel` callback-composition helpers (`LowLevel::appendNodeOnEvent`, `LowLevel::appendNodeOnFocus`, `LowLevel::appendNodeOnBlur`) instead of mutating callback tables ad hoc.
- When installing temporary/replaced low-level node callbacks, prefer `PrimeStage::LowLevel::NodeCallbackHandle` with `PrimeStage::LowLevel::NodeCallbackTable` so previous callback wiring restores automatically on scope exit.
- For window composition via `createWindow(WindowSpec)`, treat move/resize callbacks as stateless
  delta signals; update app-owned window geometry and request rebuild/layout from your runtime.

### Callback Threading/Reentrancy Contract
- Callback execution is synchronous on the event-dispatch thread.
- PrimeStage does not provide cross-thread synchronization for app-owned callback state.
- Callback-composition helpers suppress direct reentrant invocation of the same callback chain to
  avoid recursive loops.
- Prefer requesting rebuild/layout work from callbacks instead of rebuilding synchronously inside
  callbacks.
- See `docs/callback-reentrancy-threading.md` for the full contract and guardrail details.

### `std::string_view` Lifetime Contract
- Spec `std::string_view` fields are build-time inputs; caller data must stay valid through the
  widget build call.
- Do not capture references to short-lived local strings in callbacks that may run after rebuild.
- For callback payloads requiring stable post-build data, PrimeStage uses internal owned storage
  (for example `TableRowInfo::row` in row-click callbacks).

Example (`Button` callback wiring):

```cpp
PrimeStage::ButtonSpec apply;
apply.label = "Apply";
apply.size.preferredWidth = 120.0f;
apply.size.preferredHeight = 28.0f;
apply.callbacks.onActivate = [&] {
  appState.pendingApply = true;
};
root.createButton(apply);
```

## Fluent Builder Authoring
- `UiNode::with(...)` applies inline node configuration and returns the same `UiNode` value.
- Use `createX(spec, fn)` overloads to author nested trees without temporary variables when that
  improves readability.
- `createScrollView(spec, fn)` and `createWindow(spec, fn)` pass typed builder payloads
  (`ScrollView`/`Window`) so composition can target `content` and title/chrome nodes directly.

## Declarative Composition Helpers
- Prefer `column(...)`/`row(...)`/`overlay(...)`/`panel(...)` for nested composition in canonical
  app code.
- Prefer `label(text)`, `paragraph(text, maxWidth)`, and `textLine(text)` for default text nodes
  when style overrides are unnecessary.
- Use `button(text, onActivate)` for default button construction when advanced styling is not needed.
- Use `form(...)` + `formField(...)` for settings/input pages so label+control+help/error text
  composition does not require ad-hoc stack boilerplate.
- Use `toggle(bind(flag))`, `checkbox("Enabled", bind(flag))`, `slider(bind(value))`,
  `tabs({"A", "B"}, bind(index))`, `dropdown({"One", "Two"}, bind(index))`, and
  `progressBar(bind(value))` for concise default value-widget composition.
- Use `window(spec, fn)` in declarative code paths to compose title/content slots without manual
  `createWindow` ceremony.

## Sizing And Responsive Width Policy
- Rely on intrinsic widget sizing defaults before adding per-widget `preferredWidth`/`preferredHeight`.
- For responsive text/layout composition, prefer `size.maxWidth` on text-bearing specs and
  container specs instead of hard-coding many child preferred widths.
- Canonical examples should keep explicit size hints to occasional demo-driven cases (for example,
  oversized scroll content), not baseline widget visibility.
- `ScrollView`, empty `Table`/`List`/`TreeView`, and text widgets without explicit width now provide
  built-in fallback sizing so default usage remains visible.
- For typed domain models, prefer `makeListModel(...)`, `makeTableModel(...)`, and `makeTreeModel(...)`
  plus adapter `bind(...)` helpers instead of ad-hoc string-view conversion loops.

## Host Input Bridge
- Use `PrimeStage::bridgeHostInputEvent(...)` to centralize translation from `PrimeHost::InputEvent`
  to `PrimeFrame::Event`.
- In canonical host loops, register shortcut bindings on `PrimeStage::App` and let
  `App::bridgeHostInputEvent(...)` dispatch them via action ids before widget-level routing.
- Use `PrimeStage::KeyCode`/`PrimeStage::HostKey` values instead of raw numeric key constants in
  app code.
- Normalize wheel/touchpad deltas in one place:
  - set `InputBridgeState::scrollLinePixels` for line-based host events.
  - set `InputBridgeState::scrollDirectionSign` to `-1.0f` if a host backend reports opposite
    vertical scroll sign.
  - PrimeStage emits `PrimeFrame::EventType::PointerScroll` with positive `scrollY` mapped to the
    same semantic direction across backends.

Example:

```cpp
PrimeStage::InputBridgeState inputBridge;
inputBridge.scrollLinePixels = 32.0f;
inputBridge.scrollDirectionSign = 1.0f;

PrimeStage::InputBridgeResult result = PrimeStage::bridgeHostInputEvent(
    input,
    batch,
    inputBridge,
    [&](PrimeFrame::Event const& event) { return dispatchFrameEvent(event); });

if (result.requestExit) {
  running = false;
}
if (result.requestFrame) {
  requestFrame();
}
```

## Platform Services Bridge
- Prefer app-level platform service configuration over per-widget host lambdas for clipboard/cursor/IME.
- Use `app.connectHostServices(host, surfaceId)` in canonical host loops.
- Call `app.applyPlatformServices(fieldSpec)` and `app.applyPlatformServices(selectableSpec)` before
  creating text widgets.

Example:

```cpp
PrimeStage::App app;
app.connectHostServices(*host, surfaceId);

PrimeStage::TextFieldSpec field;
field.state = &state.nameField;
app.applyPlatformServices(field);
root.createTextField(field);
```

## Frame Lifecycle Helper
- Prefer `PrimeStage::App` as the top-level runtime shell and use `PrimeStage::FrameLifecycle`
  through `app.lifecycle()` for rebuild/layout/frame scheduling.
- `App::dispatchFrameEvent(...)` and `App::bridgeHostInputEvent(...)` automatically request a frame
  when events are handled, so canonical interaction callbacks do not need manual frame scheduling.
- Use `requestRebuild()` only when callback logic intentionally changes scene structure or other
  build-time inputs that require full rebuild.
- Use `requestLayout()` when only layout inputs (for example render size/scale) changed.
- Use `requestFrame()` for explicit non-event redraw triggers (for example animation ticks), not as
  boilerplate inside ordinary widget interaction callbacks.
- `TextField` state-backed edits (typing, caret moves, selection updates) and value-widget
  interactions (`Toggle`, `Checkbox`, `Slider`, and `ProgressBar` in binding-backed or legacy
  state-backed mode) are patch-first in the built scene, so high-frequency callbacks can request
  only a frame in typical app loops.
- In frame loops, consume work with `runRebuildIfNeeded(...)` and `runLayoutIfNeeded(...)`, then call
  `markFramePresented()` after presenting.

## Focus Semantics Contract
Default focus behavior expected by app code:
- Focusable by default: `Button`, `TextField` (with state), `Toggle`, `Checkbox`, `Slider`,
  `ProgressBar`, `Table`, `TreeView`.
- Not focusable by default: `SelectableText`.
- Per-widget focus/keyboard/pointer/accessibility defaults are defined in
  `docs/default-widget-behavior-matrix.md`.
- Focus visuals should render by default for focusable widgets using semantic focus styling;
  `focusStyle` remains an opt-in override, not a requirement.
- App code should drive focus through `PrimeStage::App::focusWidget(...)` with
  `WidgetFocusHandle` in canonical paths, not by patching focus primitives manually.
- Raw `PrimeFrame::FocusManager` and `NodeId` usage is reserved for advanced/low-level interop.

## Default Styling Readability Contract
- Default theme text on default surface must maintain at least `4.5:1` contrast.
- Default semantic focus ring on default surface must maintain at least `3.0:1` contrast.
- Apps should not need to set custom palette/style tokens for baseline legibility.
- Regression coverage for this contract lives in `tests/unit/test_visual_regression.cpp`
  with golden snapshots under `tests/snapshots/default_theme_readability.snap`.

Example (`FocusManager` with widget defaults):

```cpp
PrimeFrame::EventRouter router;
PrimeFrame::FocusManager focus;

PrimeFrame::Event down;
down.type = PrimeFrame::EventType::PointerDown;
down.pointerId = 1;
down.x = buttonCenterX;
down.y = buttonCenterY;

router.dispatch(down, frame, layout, &focus);
```

## Current Gaps (Tracked Separately)
These ergonomics are still tracked in `docs/todo.md` and are not complete yet:
- none.
