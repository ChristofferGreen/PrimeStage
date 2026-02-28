# Semantic Callback Migration

## Goal

Migrate app code from legacy callback aliases to the preferred semantic callback names without
breaking existing integrations.

## Legacy To Preferred Mapping

| Legacy Alias | Preferred Callback |
| --- | --- |
| `ButtonCallbacks::onClick` | `ButtonCallbacks::onActivate` |
| `TextFieldCallbacks::onTextChanged` | `TextFieldCallbacks::onChange` |
| `ToggleCallbacks::onChanged` | `ToggleCallbacks::onChange` |
| `CheckboxCallbacks::onChanged` | `CheckboxCallbacks::onChange` |
| `SliderCallbacks::onValueChanged` | `SliderCallbacks::onChange` |
| `ProgressBarCallbacks::onValueChanged` | `ProgressBarCallbacks::onChange` |
| `TabsCallbacks::onTabChanged` | `TabsCallbacks::onSelect` |
| `DropdownCallbacks::onOpened` | `DropdownCallbacks::onOpen` |
| `DropdownCallbacks::onSelected` | `DropdownCallbacks::onSelect` |
| `ListCallbacks::onSelected` | `ListCallbacks::onSelect` |
| `TableCallbacks::onRowClicked` | `TableCallbacks::onSelect` |
| `TreeViewCallbacks::onSelectionChanged` | `TreeViewCallbacks::onSelect` |
| `TreeViewCallbacks::onActivated` | `TreeViewCallbacks::onActivate` |

## Before/After Examples

### Button activation

Before:

```cpp
PrimeStage::ButtonSpec save;
save.label = "Save";
save.callbacks.onClick = [&]() { submit(); };
```

After:

```cpp
PrimeStage::ButtonSpec save;
save.label = "Save";
save.callbacks.onActivate = [&]() { submit(); };
```

### Value-change widgets

Before:

```cpp
PrimeStage::ToggleSpec darkMode;
darkMode.callbacks.onChanged = [&](bool value) { state.darkMode = value; };
```

After:

```cpp
PrimeStage::ToggleSpec darkMode;
darkMode.callbacks.onChange = [&](bool value) { state.darkMode = value; };
```

### Selection widgets

Before:

```cpp
PrimeStage::TabsSpec tabs;
tabs.callbacks.onTabChanged = [&](int index) { state.tabIndex = index; };
```

After:

```cpp
PrimeStage::TabsSpec tabs;
tabs.callbacks.onSelect = [&](int index) { state.tabIndex = index; };
```

### Tree activation

Before:

```cpp
PrimeStage::TreeViewSpec tree;
tree.callbacks.onActivated = [&](PrimeStage::TreeViewRowInfo const& row) { openNode(row); };
```

After:

```cpp
PrimeStage::TreeViewSpec tree;
tree.callbacks.onActivate = [&](PrimeStage::TreeViewRowInfo const& row) { openNode(row); };
```

## Compatibility Contract

- Legacy aliases remain supported for compatibility.
- New code should use preferred semantic callback names.
- When both alias and preferred callback are set, PrimeStage invokes the preferred callback.

## Verification

- Regression tests in `tests/unit/test_interaction.cpp` and
  `tests/unit/test_tabs_dropdown.cpp` cover:
  - legacy alias support
  - preferred callback behavior
  - preferred-callback precedence when both are present
