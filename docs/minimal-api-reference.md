# PrimeStage Minimal API Reference

This document is a compact reference for the main public API entry points used by app/runtime code.
It complements the design and ergonomics docs with symbol-level quick lookup.

## Core Headers

- `include/PrimeStage/PrimeStage.h`
- `include/PrimeStage/App.h`
- `include/PrimeStage/Ui.h`
- `include/PrimeStage/AppRuntime.h`
- `include/PrimeStage/InputBridge.h`
- `include/PrimeStage/Render.h`

## Runtime And Lifecycle

- `PrimeStage::getVersion()`
- `PrimeStage::getVersionString()`
- `PrimeStage::App`
  - `runRebuildIfNeeded(...)`
  - `runLayoutIfNeeded()`
  - `dispatchFrameEvent(...)`
  - `bridgeHostInputEvent(...)`
  - `setSurfaceMetrics(...)`
  - `setRenderMetrics(...)`
  - `renderToTarget(...)`
  - `renderToPng(...)`
  - `markFramePresented()`
  - `lifecycle()`
- `PrimeStage::FrameLifecycle`
  - `requestRebuild()`
  - `requestLayout()`
  - `requestFrame()`
  - `runRebuildIfNeeded(...)`
  - `runLayoutIfNeeded(...)`
  - `framePending()`
  - `markFramePresented()`

## Input Bridge

- `PrimeStage::InputBridgeState`
  - `scrollLinePixels`
  - `scrollDirectionSign`
- `PrimeStage::InputBridgeResult`
  - `requestFrame`
  - `bypassFrameCap`
  - `requestExit`
- `PrimeStage::bridgeHostInputEvent(...)`

## Core Ids And Shared Specs

- `PrimeStage::WidgetKind`
- `PrimeStage::widgetKindName(...)`
- `PrimeStage::WidgetIdentityId`
- `PrimeStage::InvalidWidgetIdentityId`
- `PrimeStage::widgetIdentityId(...)`
- `PrimeStage::WidgetSpec`
- `PrimeStage::EnableableWidgetSpec`
- `PrimeStage::FocusableWidgetSpec`
- `PrimeStage::State<T>`
- `PrimeStage::Binding<T>`
- `PrimeStage::bind(...)`
- `PrimeStage::SliderState`
- `PrimeStage::ProgressBarState`

## UiNode Widget Builders

Fluent helpers:
- `with(lambda)` for inline post-create node configuration.
- `createX(spec, lambda)` overloads for nested composition across container, widget, `ScrollView`, and `Window` builders.

Declarative helpers:
- `column(...)`
- `row(...)`
- `overlay(...)`
- `panel(...)`
- `label(text)`
- `paragraph(text, maxWidth)`
- `textLine(text)`
- `divider(height)`
- `spacer(height)`
- `button(text, onActivate)`
- `window(spec, lambda)`

Container/base:
- `createVerticalStack(...)`
- `createHorizontalStack(...)`
- `createOverlay(...)`
- `createPanel(...)`
- `createPanel(rectStyle, size)`
- `createLabel(...)`
- `createLabel(text, textStyle, size)`
- `createParagraph(...)`
- `createTextLine(...)`
- `createDivider(...)`
- `createDivider(rectStyle, size)`
- `createSpacer(...)`
- `createSpacer(size)`

Interactive:
- `createButton(...)`
- `createButton(label, backgroundStyle, textStyle, size)`
- `createTextField(...)`
- `createTextField(state, placeholder, backgroundStyle, textStyle, size)`
- `createSelectableText(...)`
- `createToggle(...)`
- `createToggle(on, trackStyle, knobStyle, size)`
- `createToggle(binding)`
- `createCheckbox(...)`
- `createCheckbox(label, checked, boxStyle, checkStyle, textStyle, size)`
- `createCheckbox(label, binding)`
- `createSlider(...)`
- `createSlider(value, vertical, trackStyle, fillStyle, thumbStyle, size)`
- `createSlider(binding, vertical)`
- `createTabs(...)`
- `createTabs(labels, binding)`
- `createDropdown(...)`
- `createDropdown(options, binding)`
- `createProgressBar(...)`
- `createProgressBar(binding)`

Collection/windowing:
- `createTable(...)`
- `createTable(columns, rows, selectedRow, size)`
- `createList(...)`
- `createTreeView(...)`
- `createTreeView(nodes, size)`
- `createScrollView(...)`
- `createScrollView(size, showVertical, showHorizontal)`
- `createWindow(...)`

## Semantic Callback Surface

Preferred widget callback names:
- `onActivate` for activation actions (for example `Button`).
- `onChange` for value mutations (for example `Toggle`, `Checkbox`, `Slider`, `ProgressBar`, `TextField` text changes).
- `onOpen` for open/expand intent (for example `Dropdown` open intent).
- `onSelect` for selection changes (for example `Tabs`, `Dropdown`, `List`, `Table`, `TreeView`).

Legacy callback names remain accepted as compatibility aliases.

## Window Builder (Stateless)

- `PrimeStage::WindowSpec`
- `PrimeStage::WindowCallbacks`
  - `onFocusRequested`
  - `onFocusChanged`
  - `onMoveStarted` / `onMoved` / `onMoveEnded`
  - `onResizeStarted` / `onResized` / `onResizeEnded`
- `PrimeStage::Window`
  - `root`
  - `titleBar`
  - `content`
  - `resizeHandleId`

Callback semantics:
- move/resize callbacks report pointer deltas, not persisted geometry.
- app/runtime code owns durable window position/size state and decides rebuild/layout scheduling.

## Patch-First Widget Interaction Paths

- State-backed `createTextField(...)` edits (text input, caret movement, selection updates) patch
  existing field visuals in place.
- `createToggle(...)`, `createCheckbox(...)`, and `createSlider(...)` interactions patch their
  value visuals in place in both binding-backed and legacy state-backed modes.
- `createProgressBar(...)` value changes patch fill geometry in place in both binding-backed and
  legacy state-backed modes.
- Typical runtime wiring for these high-frequency updates uses `FrameLifecycle::requestFrame()`
  instead of full rebuild requests when no structural widgets change.

## Focus And Identity Helpers

- `PrimeStage::WidgetIdentityReconciler`
  - `beginRebuild(...)`
  - `registerNode(...)`
  - `findNode(...)`
  - `restoreFocus(...)`
- `PrimeStage::NodeCallbackTable`
- `PrimeStage::NodeCallbackHandle`
  - `bind(...)`
  - `reset()`

## Render Surface APIs

- `PrimeStage::RenderStatusCode`
- `PrimeStage::RenderStatus`
- `PrimeStage::renderStatusMessage(...)`
- `PrimeStage::renderFrameToTarget(...)`
- `PrimeStage::renderFrameToPng(...)`
