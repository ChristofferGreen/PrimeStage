#pragma once

#include "PrimeFrame/Frame.h"
#include "PrimeFrame/Layout.h"

#include <cstdint>
#include <span>
#include <string_view>

namespace PrimeStage {

struct Rgba8 {
  uint8_t r = 10;
  uint8_t g = 14;
  uint8_t b = 20;
  uint8_t a = 255;
};

struct CornerStyleMetadata {
  float thinBandMaxHeight = 6.0f;
  float thumbMaxWidth = 12.0f;
  float thumbMaxHeight = 12.0f;
  float controlMinHeight = 30.0f;
  float controlMaxHeight = 34.0f;
  float panelMinHeight = 110.0f;
  float panelMaxHeight = 130.0f;
  float thinBandRadius = 0.0f;
  float thumbRadius = 2.0f;
  float controlRadius = 6.0f;
  float panelRadius = 4.0f;
  float fallbackRadius = 0.0f;
};

struct RenderOptions {
  bool clear = true;
  Rgba8 clearColor{};
  bool roundedCorners = true;
  CornerStyleMetadata cornerStyle{};
};

struct RenderTarget {
  std::span<uint8_t> pixels;
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t stride = 0;
  float scale = 1.0f;
};

enum class RenderStatusCode : uint8_t {
  Success = 0,
  BackendUnavailable,
  InvalidTargetDimensions,
  InvalidTargetStride,
  InvalidTargetBuffer,
  LayoutHasNoRoots,
  LayoutMissingRootMetrics,
  LayoutZeroExtent,
  PngPathEmpty,
  PngWriteFailed,
};

struct RenderStatus {
  RenderStatusCode code = RenderStatusCode::Success;
  uint32_t targetWidth = 0;
  uint32_t targetHeight = 0;
  uint32_t targetStride = 0;
  uint32_t requiredStride = 0;
  std::string_view detail{};

  [[nodiscard]] bool ok() const {
    return code == RenderStatusCode::Success;
  }

  [[nodiscard]] explicit operator bool() const {
    return ok();
  }
};

[[nodiscard]] std::string_view renderStatusMessage(RenderStatusCode code);

[[nodiscard]] RenderStatus renderFrameToTarget(PrimeFrame::Frame& frame,
                                               PrimeFrame::LayoutOutput const& layout,
                                               RenderTarget const& target,
                                               RenderOptions const& options = {});

[[nodiscard]] RenderStatus renderFrameToTarget(PrimeFrame::Frame& frame,
                                               RenderTarget const& target,
                                               RenderOptions const& options = {});

[[nodiscard]] RenderStatus renderFrameToPng(PrimeFrame::Frame& frame,
                                            PrimeFrame::LayoutOutput const& layout,
                                            std::string_view path,
                                            RenderOptions const& options = {});

[[nodiscard]] RenderStatus renderFrameToPng(PrimeFrame::Frame& frame,
                                            std::string_view path,
                                            RenderOptions const& options = {});

} // namespace PrimeStage
