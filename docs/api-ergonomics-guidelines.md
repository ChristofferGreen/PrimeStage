# PrimeStage API Ergonomics Guidelines

## Goal
PrimeStage should let apps build interactive UI without mutating `PrimeFrame` internals directly.
App code should describe state and callbacks at the widget-spec level, while PrimeStage owns node setup and callback plumbing.

## Responsibility Split
PrimeStage responsibilities:
- Build `PrimeFrame` nodes/primitives from widget specs.
- Install widget event handlers from spec callbacks.
- Apply widget-local interaction visuals (hover, pressed, focus ring).
- Expose focusable behavior on interactive widgets by default.

Application responsibilities:
- Own durable domain state and widget state objects.
- Provide callback implementations that update app state.
- Trigger rebuild/layout scheduling in app runtime loops.
- Bridge host/platform events into `PrimeFrame::Event`.

What app code should avoid:
- Writing `node->callbacks = ...` directly for standard widgets.
- Composing low-level `PrimeFrame::Callback` chains in feature code.
- Re-implementing focus visuals per widget by patching primitives.
- Translating raw key constants inside each feature/widget usage site.

## State Ownership Rules
- Keep authoritative UI state in app-owned structs.
- Pass stable state pointers to stateful widgets (for example `TextFieldState*`).
- Treat non-stateful widget specs as declarative input for one build pass.
- Rebuild safely at any time; state must survive scene disposal.

Example (`TextField` with app-owned state):

```cpp
struct AppState {
  PrimeStage::TextFieldState nameField;
  std::string status;
};

void buildUi(PrimeStage::UiNode root, AppState& state) {
  PrimeStage::TextFieldSpec field;
  field.state = &state.nameField;
  field.placeholder = "Name";
  field.size.preferredWidth = 220.0f;
  field.size.preferredHeight = 28.0f;
  field.callbacks.onTextChanged = [&](std::string_view text) {
    state.status = std::string(text);
  };
  root.createTextField(field);
}
```

## Interaction Wiring Rules
- Prefer widget spec callbacks (`onClick`, `onValueChanged`, `onSelectionChanged`) over node event hooks.
- Keep callbacks side-effect-light: update state and mark the app for rebuild/layout.
- Use widget-provided callback payloads (`TableRowInfo`, `TreeViewRowInfo`) instead of hit-test math in app code.
- Reserve direct `PrimeFrame` callback mutation for explicitly documented advanced/internal scenarios.

Example (`Button` callback wiring):

```cpp
PrimeStage::ButtonSpec apply;
apply.label = "Apply";
apply.size.preferredWidth = 120.0f;
apply.size.preferredHeight = 28.0f;
apply.callbacks.onClick = [&] {
  appState.pendingApply = true;
};
root.createButton(apply);
```

## Focus Semantics Contract
Default focus behavior expected by app code:
- Focusable by default: `Button`, `TextField` (with state), `Toggle`, `Checkbox`, `Slider`,
  `ProgressBar`, `Table`, `TreeView`.
- Not focusable by default: `SelectableText`.
- Focus visuals should be rendered by widget APIs via `focusStyle`/fallback style tokens.
- App code should drive focus through `PrimeFrame::FocusManager`, not by patching focus primitives manually.

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
- Removal of app-level direct `PrimeFrame` callback plumbing in examples (`[12]`/`[19]`).
- Host input bridge with symbolic key APIs (`[14]`).
