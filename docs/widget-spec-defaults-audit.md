# PrimeStage Widget-Spec Defaults Audit

## Goal

Inventory standard widget spec fields and classify them as:
- required
- optional
- advanced

This keeps default-first API usage explicit and prevents noisy configuration from becoming required.

## Classification Rules

- `required`: no safe runtime fallback exists and app code must set the field.
- `optional`: field has a safe default and is part of normal high-level customization.
- `advanced`: field has a safe default but is intended for low-level tuning, compatibility, or deep
  visual/input control.

Current policy target: required fields are zero for standard controls.

## Shared Base Spec Fields

These base fields apply to all widget specs through inheritance and are not required by default:

- `SizeSpec`: `minWidth`, `maxWidth`, `preferredWidth`, `stretchX`, `minHeight`, `maxHeight`,
  `preferredHeight`, `stretchY` (`optional`)
- `WidgetSpec`: `accessibility`, `visible`, `size` (`optional`)
- `EnableableWidgetSpec`: `enabled` (`optional`)
- `FocusableWidgetSpec`: `tabIndex` (`advanced`; default focus order is automatic)

## Standard Widget Audit

### `ButtonSpec`

- Required fields: none.
- Optional fields: `label`, `callbacks.onActivate`.
- Advanced fields:
  `backgroundStyle`, `backgroundStyleOverride`, `hoverStyle`, `hoverStyleOverride`, `pressedStyle`,
  `pressedStyleOverride`, `focusStyle`, `focusStyleOverride`, `textStyle`, `textStyleOverride`,
  `textInsetX`, `textOffsetY`, `centerText`, `baseOpacity`, `hoverOpacity`, `pressedOpacity`,
  `callbacks.onHoverChanged`, `callbacks.onPressedChanged`, legacy `callbacks.onClick`.

### `TextFieldSpec`

- Required fields: none.
- Optional fields: `text`, `placeholder`, `callbacks.onChange`.
- Advanced fields:
  `state`, `ownedState`, `compositionState`, `callbacks`, `compositionCallbacks`, `clipboard`,
  `paddingX`, `textOffsetY`, `backgroundStyle`, `backgroundStyleOverride`, `focusStyle`,
  `focusStyleOverride`, `textStyle`, `textStyleOverride`, `placeholderStyle`,
  `placeholderStyleOverride`, `showPlaceholderWhenEmpty`, `showCursor`, `cursorIndex`,
  `cursorWidth`, `cursorStyle`, `cursorStyleOverride`, `selectionStart`, `selectionEnd`,
  `selectionStyle`, `selectionStyleOverride`, `cursorBlinkInterval`, `allowNewlines`,
  `handleClipboardShortcuts`, `setCursorToEndOnFocus`, `readOnly`, legacy `callbacks.onTextChanged`.

### `SelectableTextSpec`

- Required fields: none.
- Optional fields: `text`.
- Advanced fields:
  `state`, `ownedState`, `callbacks`, `clipboard`, `textStyle`, `textStyleOverride`, `wrap`,
  `maxWidth`, `paddingX`, `selectionStart`, `selectionEnd`, `selectionStyle`,
  `selectionStyleOverride`, `focusStyle`, `focusStyleOverride`, `handleClipboardShortcuts`.

### `ToggleSpec`

- Required fields: none.
- Optional fields: `binding`, `on`, `callbacks.onChange`.
- Advanced fields:
  `state`, `knobInset`, `trackStyle`, `trackStyleOverride`, `knobStyle`, `knobStyleOverride`,
  `focusStyle`, `focusStyleOverride`, legacy `callbacks.onChanged`.

### `CheckboxSpec`

- Required fields: none.
- Optional fields: `binding`, `label`, `checked`, `callbacks.onChange`.
- Advanced fields:
  `state`, `boxSize`, `checkInset`, `gap`, `boxStyle`, `boxStyleOverride`, `checkStyle`,
  `checkStyleOverride`, `focusStyle`, `focusStyleOverride`, `textStyle`, `textStyleOverride`,
  legacy `callbacks.onChanged`.

### `SliderSpec`

- Required fields: none.
- Optional fields: `binding`, `value`, `vertical`, `callbacks.onChange`.
- Advanced fields:
  `state`, `trackThickness`, `thumbSize`, `trackStyle`, `trackStyleOverride`, `fillStyle`,
  `fillStyleOverride`, `fillHoverOpacity`, `fillPressedOpacity`, `thumbStyle`, `thumbStyleOverride`,
  `focusStyle`, `focusStyleOverride`, `trackHoverOpacity`, `trackPressedOpacity`,
  `thumbHoverOpacity`, `thumbPressedOpacity`, `callbacks.onDragStart`, `callbacks.onDragEnd`,
  legacy `callbacks.onValueChanged`.

### `TabsSpec`

- Required fields: none.
- Optional fields: `labels`, `binding`, `selectedIndex`, `callbacks.onSelect`.
- Advanced fields:
  `state`, `tabPaddingX`, `tabPaddingY`, `gap`, `tabStyle`, `tabStyleOverride`, `activeTabStyle`,
  `activeTabStyleOverride`, `textStyle`, `textStyleOverride`, `activeTextStyle`,
  `activeTextStyleOverride`, legacy `callbacks.onTabChanged`.

### `DropdownSpec`

- Required fields: none.
- Optional fields: `options`, `binding`, `selectedIndex`, `callbacks.onOpen`, `callbacks.onSelect`.
- Advanced fields:
  `state`, `label`, `indicator`, `paddingX`, `indicatorGap`, `backgroundStyle`,
  `backgroundStyleOverride`, `textStyle`, `textStyleOverride`, `indicatorStyle`,
  `indicatorStyleOverride`, `focusStyle`, `focusStyleOverride`, legacy `callbacks.onOpened`,
  legacy `callbacks.onSelected`.

### `ProgressBarSpec`

- Required fields: none.
- Optional fields: `binding`, `value`, `callbacks.onChange`.
- Advanced fields:
  `state`, `minFillWidth`, `trackStyle`, `trackStyleOverride`, `fillStyle`, `fillStyleOverride`,
  `focusStyle`, `focusStyleOverride`, legacy `callbacks.onValueChanged`.

### `ListSpec`

- Required fields: none.
- Optional fields: `items`, `selectedIndex`, `callbacks.onSelect`.
- Advanced fields:
  `textStyle`, `rowHeight`, `rowGap`, `rowPaddingX`, `rowStyle`, `rowAltStyle`, `selectionStyle`,
  `dividerStyle`, `focusStyle`, `focusStyleOverride`, `clipChildren`, legacy `callbacks.onSelected`.

### `TableSpec`

- Required fields: none.
- Optional fields: `columns`, `rows`, `selectedRow`, `callbacks.onSelect`.
- Advanced fields:
  `headerInset`, `headerHeight`, `rowHeight`, `rowGap`, `headerPaddingX`, `cellPaddingX`,
  `headerStyle`, `rowStyle`, `rowAltStyle`, `selectionStyle`, `dividerStyle`, `focusStyle`,
  `focusStyleOverride`, `showHeaderDividers`, `showColumnDividers`, `clipChildren`,
  legacy `callbacks.onRowClicked`.

### `TreeViewSpec`

- Required fields: none.
- Optional fields: `nodes`, `callbacks.onSelect`, `callbacks.onActivate`.
- Advanced fields:
  `rowStartX`, `rowStartY`, `rowWidthInset`, `rowHeight`, `rowGap`, `indent`, `caretBaseX`,
  `caretSize`, `caretInset`, `caretThickness`, `caretMaskPad`, `connectorThickness`, `linkEndInset`,
  `selectionAccentWidth`, `doubleClickMs`, `keyboardNavigation`, `showHeaderDivider`,
  `headerDividerY`, `showConnectors`, `showCaretMasks`, `showScrollBar`, `clipChildren`, `visible`,
  `rowStyle`, `rowAltStyle`, `hoverStyle`, `selectionStyle`, `selectionAccentStyle`,
  `caretBackgroundStyle`, `caretLineStyle`, `connectorStyle`, `focusStyle`, `focusStyleOverride`,
  `textStyle`, `selectedTextStyle`, `scrollBar`, `callbacks.onSelectionChanged`,
  `callbacks.onExpandedChanged`, `callbacks.onActivated`, `callbacks.onHoverChanged`,
  `callbacks.onScrollChanged`.

### `ScrollViewSpec`

- Required fields: none.
- Optional fields: `showVertical`, `showHorizontal`.
- Advanced fields: `clipChildren`, `visible`, `size`, `vertical`, `horizontal`.

Supporting advanced tuning specs:
- `ScrollAxisSpec`: `enabled`, `thickness`, `inset`, `startPadding`, `endPadding`, `thumbLength`,
  `thumbOffset`, `trackStyle`, `thumbStyle`
- `ScrollBarSpec`: `enabled`, `autoThumb`, `inset`, `padding`, `width`, `minThumbHeight`,
  `thumbFraction`, `thumbProgress`, `trackHoverOpacity`, `trackPressedOpacity`,
  `thumbHoverOpacity`, `thumbPressedOpacity`, `trackStyle`, `trackStyleOverride`, `thumbStyle`,
  `thumbStyleOverride`

### `WindowSpec`

- Required fields: none.
- Optional fields: `title`.
- Advanced fields:
  `accessibility`, `positionX`, `positionY`, `width`, `height`, `minWidth`, `minHeight`,
  `titleBarHeight`, `contentPadding`, `resizeHandleSize`, `movable`, `resizable`, `focusable`,
  `tabIndex`, `visible`, `callbacks`, `frameStyle`, `frameStyleOverride`, `titleBarStyle`,
  `titleBarStyleOverride`, `titleTextStyle`, `titleTextStyleOverride`, `contentStyle`,
  `contentStyleOverride`, `resizeHandleStyle`, `resizeHandleStyleOverride`.

## Minimal Instantiation Paths (1 Line Each)

```cpp
root.createButton(PrimeStage::ButtonSpec{});
root.createTextField(PrimeStage::TextFieldSpec{});
root.createSelectableText(PrimeStage::SelectableTextSpec{});
root.createToggle(PrimeStage::ToggleSpec{});
root.createCheckbox(PrimeStage::CheckboxSpec{});
root.createSlider(PrimeStage::SliderSpec{});
root.createTabs(PrimeStage::TabsSpec{});
root.createDropdown(PrimeStage::DropdownSpec{});
root.createProgressBar(PrimeStage::ProgressBarSpec{});
root.createList(PrimeStage::ListSpec{});
root.createTable(PrimeStage::TableSpec{});
root.createTreeView(PrimeStage::TreeViewSpec{});
root.createScrollView(PrimeStage::ScrollViewSpec{});
PrimeStage::Window window = root.createWindow(PrimeStage::WindowSpec{});
```

## Noisy Defaults Policy

Fields considered noisy in normal app code are classified as advanced:
- raw state pointers (`state`, `compositionState`) when binding/owned defaults suffice
- compatibility callback aliases (`onClick`, `onTextChanged`, `onChanged`, `onValueChanged`,
  `onTabChanged`, `onOpened`, `onSelected`, `onRowClicked`)
- deep style/opacity/timing knobs not required for baseline behavior

New public API additions should preserve this contract: no new required fields for standard
controls unless a runtime-safe default is impossible.
