# PrimeStage Design (Draft)

## Scope
PrimeStage is the UI authoring layer that owns ground-truth widget state and emits rect hierarchies for PrimeFrame.
It builds render-ready UI scenes, wires callbacks, and provides a terse authoring API.
Contributor ergonomics and app-facing ownership contracts are documented in `docs/api-ergonomics-guidelines.md`.
Resolved architecture choices are documented in `docs/design-decisions.md`.

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
- `PrimeStage`: ground-truth UI state, widget composition, callback wiring, window management.
- `PrimeFrame`: layout + rect hierarchy + render batch output.
- `PrimeManifest`: renderer backend.

## Scene Lifecycle
PrimeStage builds an ephemeral `PrimeScene` (rect graph) from ground truth.

- **Rebuild**: structural changes (new windows, tree shape changes, large layout changes).
- **Patch**: transient changes (hover, focus, drag) via callbacks that directly mutate the current `Frame`.

The ground truth remains authoritative. `PrimeScene` is a disposable snapshot that can be rebuilt at any time.

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
- `ButtonSpec::callbacks.onClick`
- `TextFieldSpec::callbacks.onTextChanged`
- `TabsSpec::callbacks.onTabChanged`

Application code should provide callback behavior at spec level and avoid direct `PrimeFrame::Callback`
table mutation for standard widget usage.
For advanced extension points, use PrimeStage callback composition helpers:
- `appendNodeOnEvent(...)`
- `appendNodeOnFocus(...)`
- `appendNodeOnBlur(...)`

## Controlled vs State-Backed Widgets
PrimeStage supports two widget value models for interactive controls:
- Controlled:
  - widget value comes from spec fields (`on`, `checked`, `selectedIndex`).
  - app callbacks update canonical app state, then rebuild.
- State-backed:
  - widget value comes from state pointers (`ToggleState*`, `CheckboxState*`, `TabsState*`,
    `DropdownState*`, `TextFieldState*`, `SelectableTextState*`).
  - PrimeStage mutates those state objects during interaction; callbacks are optional.

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
primary.callbacks.onClick = [&] { appState.pendingApply = true; };
root.createButton(primary);

TextFieldSpec field;
field.state = &appState.nameField;
field.placeholder = "Name";
field.callbacks.onTextChanged = [&](std::string_view text) {
  appState.name = std::string(text);
};
root.createTextField(field);

root.createVerticalStack(layoutSpec, [](UiNode& col) {
  col.createButton(primary);
  col.createTextField(field);
});
```

`UiNode` stores a `Frame&` via `std::reference_wrapper` and a `NodeId`.
All widget specs use `std::optional` for optional fields.
Container sizing and layout metadata are unified via `ContainerSpec`, which is shared by
`StackSpec` and `PanelSpec` to keep padding/gap/clip/visibility consistent across containers.
`ScrollView` returns a small struct that exposes both the root node and a dedicated content node
so callers can attach children and apply scroll offsets without mixing in scrollbar primitives.

### UiNode Shape (Current)

```cpp
struct UiNode {
  std::reference_wrapper<Frame> frame;
  NodeId id;

  UiNode createPanel(PanelSpec const& spec);
  UiNode createLabel(LabelSpec const& spec);
  UiNode createParagraph(ParagraphSpec const& spec);
  UiNode createTextLine(TextLineSpec const& spec);
  UiNode createDivider(DividerSpec const& spec);
  UiNode createSpacer(SpacerSpec const& spec);

  UiNode createButton(ButtonSpec const& spec);
  UiNode createTextField(TextFieldSpec const& spec);
  UiNode createSelectableText(SelectableTextSpec const& spec);
  UiNode createToggle(ToggleSpec const& spec);
  UiNode createCheckbox(CheckboxSpec const& spec);
  UiNode createSlider(SliderSpec const& spec);
  UiNode createTabs(TabsSpec const& spec);
  UiNode createDropdown(DropdownSpec const& spec);
  UiNode createProgressBar(ProgressBarSpec const& spec);
  UiNode createTable(TableSpec const& spec);
  UiNode createTreeView(TreeViewSpec const& spec);
  ScrollView createScrollView(ScrollViewSpec const& spec);

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
