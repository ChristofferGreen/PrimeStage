# PrimeStage Design (Draft)

## Scope
PrimeStage is the UI authoring layer that owns ground-truth widget state and emits rect hierarchies for PrimeFrame.
It builds render-ready UI scenes, wires callbacks, and provides a terse authoring API.
Contributor ergonomics and app-facing ownership contracts are documented in `docs/api-ergonomics-guidelines.md`.
Resolved architecture choices are documented in `docs/design-decisions.md`.
Public API evolution and compatibility expectations are documented in
`docs/api-evolution-policy.md`.
Render failure/status diagnostics for `PrimeStage/Render.h` APIs are documented in
`docs/render-diagnostics.md`.
Renderer rounded-corner behavior is controlled by explicit `RenderOptions::cornerStyle`
metadata (not theme-color index matching heuristics).
Dependency pin/update expectations for fetched third-party components are documented in
`docs/dependency-resolution-policy.md`.
A compact symbol-level API lookup is provided in `docs/minimal-api-reference.md`.

PrimeStage does not render. PrimeFrame handles layout, hit testing, and render batch emission.
PrimeHost handles OS integration and input delivery.

## Core Principles
- **Ground truth lives above**: PrimeStage mirrors state into a disposable scene.
- **Ephemeral scenes**: PrimeStage builds `PrimeScene` snapshots that can be discarded and rebuilt.
- **Patchable**: transient interactions (hover, drag, focus) can patch the scene without a full rebuild.
- **Rect-first**: UI output is just rect/text primitives with thin UI semantics.
- **No raw pointers**: use `std::optional`, `std::reference_wrapper`, and RAII.

## Layering
- `PrimeHost`: windowing + input dispatch.
- `PrimeStage`: ground-truth UI state, widget composition, callback wiring, window management,
  and high-level app-shell orchestration (`PrimeStage::App`).
- `PrimeFrame`: layout + rect hierarchy + render batch output.
- `PrimeManifest`: renderer backend.

### Input Normalization
- PrimeStage uses symbolic key names (`KeyCode`/`HostKey`) for input translation.
- PrimeHost input is normalized through `bridgeHostInputEvent(...)` so widget code sees consistent
  `PrimeFrame::Event` semantics.
- For canonical host loops, `PrimeStage::App::bridgeHostInputEvent(...)` wires this bridge to the
  app-owned frame/layout/router/focus pipeline.
- `PrimeStage::App` also supports app-level command routing (`registerAction`, `bindShortcut`,
  `invokeAction`) so shortcut handling does not require per-widget key plumbing.
- `PrimeStage::App` automatically requests frame presentation when dispatched input/frame events are
  handled, reducing manual lifecycle scheduling in canonical interaction callbacks.
- `PrimeStage::App::connectHostServices(...)` wires clipboard/cursor/IME host services once at the
  runtime layer.
- Scroll normalization is explicit:
  - `scrollLinePixels` converts line-based host deltas to pixels.
  - `scrollDirectionSign` aligns backend sign conventions so `PointerScroll.scrollY` direction is
    consistent across host integrations.

## Scene Lifecycle
PrimeStage builds an ephemeral `PrimeScene` (rect graph) from ground truth.

`PrimeStage::App` is the canonical app shell that owns frame root bootstrap, rebuild/layout
scheduling, input dispatch routing, and render helpers so application code does not directly own
`PrimeFrame::Frame`/`LayoutEngine`/`EventRouter`/`FocusManager` for standard usage.

- **Rebuild**: structural changes (new windows, tree shape changes, large layout changes).
- **Patch**: transient changes (hover, focus, drag) via callbacks that directly mutate the current `Frame`.

The ground truth remains authoritative. `PrimeScene` is a disposable snapshot that can be rebuilt at any time.
For high-frequency `TextField` interactions and interactive value widgets (`Toggle`, `Checkbox`,
`Slider`, and `ProgressBar` in binding-backed or legacy state-backed mode), PrimeStage patches
visuals in place on the built scene so app runtimes can request frame-only updates when structure is
unchanged.

### Patch Safety
Patches should only touch fields that do not restructure the graph:
- Position, size, opacity, visibility
- Style tokens (colors, borders, text styles)
- Scroll offsets

If a patch affects layout or structure, trigger a layout pass or rebuild.
See `docs/design-decisions.md` for the strict patch-operation whitelist used by PrimeStage runtime flows.

## Callback Model
PrimeStage widget callbacks are configured through widget specs.
Examples:
- `ButtonSpec::callbacks.onActivate`
- `TextFieldSpec::callbacks.onChange`
- `TabsSpec::callbacks.onSelect`
- `DropdownSpec::callbacks.onOpen` / `DropdownSpec::callbacks.onSelect`
- `ToggleSpec::callbacks.onChange`

Application code should provide callback behavior at spec level and avoid direct `PrimeFrame::Callback`
table mutation for standard widget usage.
Legacy callback names remain accepted for compatibility, but new app code should use semantic names.
Canonical example-app usage and documented advanced exceptions are tracked in
`docs/example-app-consumer-checklist.md`.
For advanced extension points, use PrimeStage callback composition helpers:
- `appendNodeOnEvent(...)`
- `appendNodeOnFocus(...)`
- `appendNodeOnBlur(...)`
For scoped callback-table replacement on a node, use `NodeCallbackTable` with
`NodeCallbackHandle` to restore previous callback wiring automatically.
Callback reentrancy/threading guarantees are documented in
`docs/callback-reentrancy-threading.md`.
Data ownership/lifetime guarantees for `std::string_view` specs and callback captures are
documented in `docs/data-ownership-lifetime.md`.

For text controls, app/runtime platform services are applied through:
- `AppPlatformServices` for clipboard, cursor-hint, and IME composition-rect callbacks.
- `App::applyPlatformServices(TextFieldSpec&)`
- `App::applyPlatformServices(SelectableTextSpec&)`

For app-level commands:
- register commands with `App::registerAction(...)`
- bind keyboard shortcuts with `App::bindShortcut(...)`
- invoke from widget callbacks via `App::makeActionCallback(...)` or `App::invokeAction(...)`
- this unifies shortcut/button/programmatic entrypoints on one command id

## Controlled, Binding, and State-Backed Widgets
PrimeStage supports three widget value models for interactive controls:
- Controlled:
  - widget value comes from spec fields (`on`, `checked`, `selectedIndex`).
  - app callbacks update canonical app state, then rebuild.
- Binding-backed:
  - widget value comes from first-class bindings (`State<T>`, `Binding<T>`, `bind(...)`).
  - supported on `Toggle`, `Checkbox`, `Slider`, `Tabs`, `Dropdown`, and `ProgressBar`.
  - PrimeStage mutates bound state values during interaction; callbacks are optional.
  - this is the preferred default for concise app-side value ownership in modern API usage.
- Legacy state-backed:
  - widget value comes from state pointers (`ToggleState*`, `CheckboxState*`, `SliderState*`,
    `TabsState*`, `DropdownState*`, `TextFieldState*`, `SelectableTextState*`).
  - PrimeStage mutates those state objects during interaction; callbacks are optional.
  - kept for backward compatibility and advanced interoperability.

PrimeStage always owns transient per-frame interaction details (hover/pressed/drag bookkeeping).

## Widget Authoring API
Widgets are exposed as free-standing functions that return a fluent `UiNode` value type.
This keeps authoring terse while avoiding pointers or heap allocation.
Core primitives live in `PrimeStage/Ui.h`; the Studio kit (roles, defaults, composite widgets) lives in the PrimeStudio repo (headers still under `PrimeStage/StudioUi.h` in namespace `PrimeStage::Studio`).

Example:

```cpp
UiNode root(frame, rootId, true);

ButtonSpec primary;
primary.label = "Apply";
primary.callbacks.onActivate = [&] { appState.pendingApply = true; };
root.createButton(primary);

TextFieldSpec field;
field.state = &appState.nameField;
field.placeholder = "Name";
field.callbacks.onChange = [&](std::string_view text) {
  appState.name = std::string(text);
};
root.createTextField(field);

root.createVerticalStack(layoutSpec, [](UiNode& col) {
  col.createButton(primary).with([](UiNode& button) {
    button.setVisible(true);
  });
  col.createTextField(field);
});
```

All `UiNode` builders support a fluent nested form `createX(spec, fn)` that returns the created node
after invoking `fn(createdNode)`. For composed return types, `createScrollView(spec, fn)` passes a
`ScrollView` (`root` + `content`), and `createWindow(spec, fn)` passes a `Window` (`root`,
`titleBar`, `content`, `resizeHandleId`).

For low-ceremony composition, `UiNode` also exposes declarative helpers:
- `column(...)`, `row(...)`, `overlay(...)`, `panel(...)`
- `label(text)`, `paragraph(text, maxWidth)`, `textLine(text)`
- `divider(height)`, `spacer(height)`, `button(text, onActivate)`
- `toggle(binding)`, `checkbox(label, binding)`, `slider(binding, vertical)`
- `tabs(labels, binding)`, `dropdown(options, binding)`, `progressBar(binding)`
- `window(spec, fn)`

`UiNode` stores a `Frame&` via `std::reference_wrapper` and a `NodeId`, but app-facing focus,
visibility, and imperative flows should use typed handles (`WidgetFocusHandle`,
`WidgetVisibilityHandle`, `WidgetActionHandle`) exposed from `UiNode`.
Raw `NodeId` access is explicitly low-level via `lowLevelNodeId()`.
All widget specs use `std::optional` for optional fields.
Shared spec bases (`WidgetSpec`, `EnableableWidgetSpec`, `FocusableWidgetSpec`) centralize
common accessibility/visibility/enablement/focus-order fields across widget families.
Core widget metadata types (`WidgetKind`, `WidgetIdentityId`) provide canonical enum/id primitives
for app/runtime identity bookkeeping.
Collection adapters (`ListModelAdapter`, `TableModelAdapter`, `TreeModelAdapter`) convert typed
domain containers to widget spec payloads with optional key extraction, so canonical app code does
not maintain ad-hoc `std::vector<std::string_view>` mirrors.
Container sizing and layout metadata are unified via `ContainerSpec`, which is shared by
`StackSpec` and `PanelSpec` to keep padding/gap/clip/visibility consistent across containers.
`ScrollView` returns a small struct that exposes both the root node and a dedicated content node
so callers can attach children and apply scroll offsets without mixing in scrollbar primitives.

### UiNode Shape (Current)

```cpp
struct UiNode {
  std::reference_wrapper<Frame> frame;
  NodeId id;

  WidgetFocusHandle focusHandle() const;
  WidgetVisibilityHandle visibilityHandle() const;
  WidgetActionHandle actionHandle() const;
  NodeId lowLevelNodeId() const;

  template <typename Fn>
  UiNode with(Fn&& fn);

  UiNode createPanel(PanelSpec const& spec);
  template <typename Fn>
  UiNode createPanel(PanelSpec const& spec, Fn&& fn);
  UiNode createLabel(LabelSpec const& spec);
  template <typename Fn>
  UiNode createLabel(LabelSpec const& spec, Fn&& fn);
  UiNode createParagraph(ParagraphSpec const& spec);
  UiNode createTextLine(TextLineSpec const& spec);
  UiNode createDivider(DividerSpec const& spec);
  UiNode createSpacer(SpacerSpec const& spec);

  UiNode createButton(ButtonSpec const& spec);
  UiNode createTextField(TextFieldSpec const& spec);
  UiNode createSelectableText(SelectableTextSpec const& spec);
  UiNode toggle(Binding<bool> binding);
  UiNode checkbox(std::string_view label, Binding<bool> binding);
  UiNode slider(Binding<float> binding, bool vertical = false);
  UiNode tabs(std::vector<std::string_view> labels, Binding<int> binding);
  UiNode dropdown(std::vector<std::string_view> options, Binding<int> binding);
  UiNode progressBar(Binding<float> binding);
  UiNode createToggle(ToggleSpec const& spec);
  UiNode createCheckbox(CheckboxSpec const& spec);
  UiNode createSlider(SliderSpec const& spec);
  UiNode createTabs(TabsSpec const& spec);
  UiNode createDropdown(DropdownSpec const& spec);
  UiNode createProgressBar(ProgressBarSpec const& spec);
  UiNode createList(ListSpec const& spec);
  UiNode createTable(TableSpec const& spec);
  UiNode createTable(std::vector<TableColumn> columns,
                     std::vector<std::vector<std::string_view>> rows,
                     int selectedRow,
                     SizeSpec const& size);
  UiNode createTreeView(TreeViewSpec const& spec);
  UiNode createTreeView(std::vector<TreeNode> nodes, SizeSpec const& size);
  ScrollView createScrollView(ScrollViewSpec const& spec);
  ScrollView createScrollView(SizeSpec const& size, bool showVertical, bool showHorizontal);
  template <typename Fn>
  ScrollView createScrollView(ScrollViewSpec const& spec, Fn&& fn);
  Window createWindow(WindowSpec const& spec);
  template <typename Fn>
  Window createWindow(WindowSpec const& spec, Fn&& fn);

  template <typename Fn>
  UiNode createVerticalStack(StackSpec const& spec, Fn&& fn) {
    UiNode child = createVerticalStack(spec);
    fn(child);
    return child;
  }
};
```

## Widget Set
Base widgets:
- Panel, Label, Divider, Spacer

Interactive widgets:
- Button, TextField, SelectableText
- Toggle, Checkbox, Slider
- Tabs, Dropdown, ProgressBar

Collection widgets:
- ScrollView
- List (table-backed convenience API)
- Table
- TreeView

`Table` remains a first-class widget API, with collection internals reusable behind the scenes.

## Windows
Windowing is built in PrimeStage as a thin layer that emits window roots into PrimeFrame.
The window manager is stateless: it builds windows from ground truth and wires callbacks for move/resize/focus.
Transient interactions (drag, hover) are applied as patches inside callbacks.

Window roots are just top-level nodes with a title bar and content slot.
Z-order is determined by input order or explicit `zOrder` fields from ground truth.
Window chrome is composed explicitly via widgets or helper builders, not auto-generated implicitly.

Current helper API:
- `UiNode::createWindow(WindowSpec const& spec)` returns a `Window` with `root`, `titleBar`,
  and `content` nodes.
- Pointer interactions are callback-driven and stateless:
  - `WindowCallbacks::onMoved(deltaX, deltaY)`
  - `WindowCallbacks::onResized(deltaWidth, deltaHeight)`
  - `WindowCallbacks::onFocusRequested()` / `onFocusChanged(bool)`
- PrimeStage reports interaction deltas; app/runtime code owns durable window geometry state and
  requests rebuild/layout via frame lifecycle helpers.

## Focus Animation
Focus is represented as a dedicated rect node that can be patched each frame.
When focus changes, the rect morphs to the new target bounds via simple interpolation.
This keeps animation transient and does not require a rebuild.

## Focus Behavior (Current)
- Focusable by default: `Button`, `TextField` (with state), `Toggle`, `Checkbox`, `Slider`,
  `ProgressBar`, `Table`, `TreeView`.
- Not focusable by default: `SelectableText`.
- Focus visuals are semantic-by-default for focusable controls; `focusStyle` is an optional override.

## HTML Stack
PrimeStage can target an HTML backend as a separate surface.
The HTML stack uses the same widget specs but maps them to native HTML widgets instead of rect primitives.
The HTML surface typically uses a single window root.

## Non-Goals (for now)
- Persistent scene storage or incremental diffing between rebuilds.
- Rich animation system beyond simple rect morphing.
- Complex widget theming beyond token mapping to PrimeFrame styles.

## Design Decisions
Previously open architecture questions are resolved and tracked in `docs/design-decisions.md`.

## IME/Composition Planning
IME/composition API planning for text controls is tracked in `docs/ime-composition-plan.md`.
That plan defines `TextField` lifecycle semantics for composition start/update/commit/cancel and
stages the host/frame integration steps.

## Accessibility Planning
Accessibility role/state planning is tracked in `docs/accessibility-semantics-roadmap.md`.
That roadmap defines metadata model, focus-order mapping, and keyboard-activation expectations for
future backend accessibility adapters.

## API Evolution Policy
Public API compatibility rules and migration notes are tracked in
`docs/api-evolution-policy.md`.
Use that policy when changing widget specs, callbacks, and public naming.
