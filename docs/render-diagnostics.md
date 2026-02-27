# PrimeStage Render Diagnostics

## Scope

PrimeStage render APIs return `RenderStatus` so failures include structured diagnostics instead of
opaque `bool` values.

## API Contract

- `renderFrameToTarget(...)` returns `RenderStatus`.
- `renderFrameToPng(...)` returns `RenderStatus`.
- `RenderStatus::ok()` is `true` only when `code == RenderStatusCode::Success`.
- `renderStatusMessage(code)` maps status codes to stable human-readable messages.
- Rounded-corner policy uses explicit `RenderOptions::cornerStyle` metadata (`CornerStyleMetadata`)
  and does not depend on theme palette index/color matching heuristics.

## Status Codes

- `Success`: render succeeded.
- `BackendUnavailable`: build was configured without PrimeManifest (`PRIMESTAGE_ENABLE_PRIMEMANIFEST=OFF`).
- `InvalidTargetDimensions`: render target width/height is zero.
- `InvalidTargetStride`: target stride is smaller than `width * 4`.
- `InvalidTargetBuffer`: target pixel span is empty or smaller than `stride * height`.
- `LayoutHasNoRoots`: frame has no roots to render.
- `LayoutMissingRootMetrics`: layout output does not contain metrics for any frame root.
- `LayoutZeroExtent`: layout resolved to zero-sized render bounds.
- `PngPathEmpty`: PNG output path is empty.
- `PngWriteFailed`: image encoding/write failed.

## Actionable Context Fields

`RenderStatus` includes context fields for diagnostics and logs:

- `targetWidth`
- `targetHeight`
- `targetStride`
- `requiredStride`
- `detail`

## Corner Style Metadata

`CornerStyleMetadata` defines explicit radius buckets and dimension thresholds used when
`RenderOptions::roundedCorners` is enabled:

- `thinBand*` for thin horizontal strips.
- `thumb*` for compact square-ish indicators.
- `control*` for standard control heights.
- `panel*` for larger panel-like regions.
- `fallbackRadius` for unmatched geometry.

Because this policy is geometry + metadata based (not theme-color matching), rounded-corner output
is deterministic under theme palette changes unless the metadata itself changes.

## Test Coverage

`tests/unit/test_render.cpp` covers:

- `renderFrameToTarget` success/failure paths for both layout-explicit and layout-derived overloads.
- `renderFrameToPng` success/failure paths, including PNG write failures.
- Headless behavior (`PRIMESTAGE_ENABLE_PRIMEMANIFEST=OFF`) expectations where render APIs return
  `RenderStatusCode::BackendUnavailable`.

## Example

```cpp
PrimeStage::RenderStatus status =
    PrimeStage::renderFrameToTarget(frame, layout, target, PrimeStage::RenderOptions{});
if (!status.ok()) {
  std::fprintf(stderr,
               "Render failed: %s (detail=%.*s, w=%u h=%u stride=%u requiredStride=%u)\n",
               PrimeStage::renderStatusMessage(status.code).data(),
               static_cast<int>(status.detail.size()),
               status.detail.data(),
               status.targetWidth,
               status.targetHeight,
               status.targetStride,
               status.requiredStride);
}
```
