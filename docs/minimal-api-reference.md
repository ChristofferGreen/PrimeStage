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

## UiNode Widget Builders

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

## Patch-First TextField Path

- State-backed `createTextField(...)` edits (text input, caret movement, selection updates) patch
  existing field visuals in place.
- Typical runtime wiring for those high-frequency callbacks uses `FrameLifecycle::requestFrame()`
  instead of full rebuild requests when no structural widgets change.

## Focus And Identity Helpers

- `PrimeStage::WidgetIdentityReconciler`
  - `beginRebuild(...)`
  - `registerNode(...)`
  - `findNode(...)`
  - `restoreFocus(...)`

## Render Surface APIs

- `PrimeStage::RenderStatusCode`
- `PrimeStage::RenderStatus`
- `PrimeStage::renderStatusMessage(...)`
- `PrimeStage::renderFrameToTarget(...)`
- `PrimeStage::renderFrameToPng(...)`
