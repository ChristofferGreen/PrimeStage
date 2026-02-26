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
Widgets store `CallbackId` only. The owner (PrimeStage or application layer) keeps a move-only RAII handle for cleanup.
No callback copies are made by widgets.

Recommended handle shape (pointer-free):

```cpp
struct CallbackHandle {
  std::optional<CallbackId> id;
  std::optional<std::reference_wrapper<CallbackTable>> table;

  ~CallbackHandle() { reset(); }
  void reset();
  CallbackHandle(CallbackHandle const&) = delete;
  CallbackHandle& operator=(CallbackHandle const&) = delete;
  CallbackHandle(CallbackHandle&&) noexcept;
  CallbackHandle& operator=(CallbackHandle&&) noexcept;
};
```

## Widget Authoring API
Widgets are exposed as free-standing functions that return a fluent `UiNode` value type.
This keeps authoring terse while avoiding pointers or heap allocation.
Core primitives live in `PrimeStage/Ui.h`; the Studio kit (roles, defaults, composite widgets) lives in the PrimeStudio repo (headers still under `PrimeStage/StudioUi.h` in namespace `PrimeStage::Studio`).

Example:

```cpp
UiNode root = createPanel(frame, rootId, panelSpec);

root.createButton(primarySpec, primaryCallbacks);
root.createButton(secondarySpec);

root.createVerticalLayout(layoutSpec, [](UiNode& col) {
  col.createButton(aSpec);
  col.createButton(bSpec);
});
```

`UiNode` stores a `Frame&` via `std::reference_wrapper` and a `NodeId`.
All widget specs use `std::optional` for optional fields.
Container sizing and layout metadata are unified via `ContainerSpec`, which is shared by
`StackSpec` and `PanelSpec` to keep padding/gap/clip/visibility consistent across containers.
`ScrollView` returns a small struct that exposes both the root node and a dedicated content node
so callers can attach children and apply scroll offsets without mixing in scrollbar primitives.

### UiNode Shape (Draft)

```cpp
struct UiNode {
  std::reference_wrapper<Frame> frame;
  NodeId id;

  UiNode createPanel(PanelSpec const& spec);
  UiNode createLabel(LabelSpec const& spec);
  UiNode createDivider(DividerSpec const& spec);
  UiNode createSpacer(SpacerSpec const& spec);

  UiNode createButton(ButtonSpec const& spec, ButtonCallbacks const& callbacks = {});
  UiNode createEditBox(EditBoxSpec const& spec, EditBoxCallbacks const& callbacks = {});
  UiNode createImage(ImageSpec const& spec);

  ScrollView createScrollView(ScrollViewSpec const& spec);
  UiNode createListView(ListViewSpec const& spec);
  UiNode createTable(TableSpec const& spec);
  UiNode createTreeView(TreeViewSpec const& spec);

  UiNode createToggle(ToggleSpec const& spec, ToggleCallbacks const& callbacks = {});
  UiNode createCheckbox(CheckboxSpec const& spec, CheckboxCallbacks const& callbacks = {});
  UiNode createSlider(SliderSpec const& spec, SliderCallbacks const& callbacks = {});
  UiNode createTabs(TabsSpec const& spec, TabsCallbacks const& callbacks = {});
  UiNode createDropdown(DropdownSpec const& spec, DropdownCallbacks const& callbacks = {});
  UiNode createProgressBar(ProgressBarSpec const& spec);
  UiNode createTooltip(TooltipSpec const& spec);
  UiNode createPopover(PopoverSpec const& spec);

  template <typename Fn>
  UiNode createVerticalLayout(VerticalLayoutSpec const& spec, Fn&& fn) {
    UiNode child = createVerticalLayout(spec);
    fn(child);
    return child;
  }
};
```

## Widget Set
Base widgets:
- Panel, Label, Divider, Spacer

Interactive widgets:
- Button, EditBox (with input constraints), Image
- Toggle, Checkbox, Slider
- Tabs, Dropdown, ProgressBar
- Tooltip, Popover

Collection widgets:
- ScrollView
- ListView
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
