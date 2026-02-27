# PrimeStage Minimal API Reference

This document is a compact reference for the main public API entry points used by app/runtime code.
It complements the design and ergonomics docs with symbol-level quick lookup.

## Core Headers

- `include/PrimeStage/PrimeStage.h`
- `include/PrimeStage/Ui.h`
- `include/PrimeStage/AppRuntime.h`
- `include/PrimeStage/InputBridge.h`
- `include/PrimeStage/Render.h`

## Runtime And Lifecycle

- `PrimeStage::getVersion()`
- `PrimeStage::getVersionString()`
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

## UiNode Widget Builders

Fluent helpers:
- `with(lambda)` for inline post-create node configuration.
- `createX(spec, lambda)` overloads for nested composition across container, widget, `ScrollView`, and `Window` builders.

Container/base:
- `createVerticalStack(...)`
- `createHorizontalStack(...)`
- `createOverlay(...)`
- `createPanel(...)`
- `createLabel(...)`
- `createParagraph(...)`
- `createTextLine(...)`
- `createDivider(...)`
- `createSpacer(...)`

Interactive:
- `createButton(...)`
- `createTextField(...)`
- `createSelectableText(...)`
- `createToggle(...)`
- `createCheckbox(...)`
- `createSlider(...)`
- `createTabs(...)`
- `createDropdown(...)`
- `createProgressBar(...)`

Collection/windowing:
- `createTable(...)`
- `createTreeView(...)`
- `createScrollView(...)`
- `createWindow(...)`

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
  value visuals in place.
- State-backed `createProgressBar(...)` value changes patch fill geometry in place.
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
