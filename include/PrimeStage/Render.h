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

struct RenderOptions {
  bool clear = true;
  Rgba8 clearColor{};
  bool roundedCorners = true;
};

struct RenderTarget {
  std::span<uint8_t> pixels;
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t stride = 0;
  float scale = 1.0f;
};

bool renderFrameToTarget(PrimeFrame::Frame& frame,
                         PrimeFrame::LayoutOutput const& layout,
                         RenderTarget const& target,
                         RenderOptions const& options = {});

bool renderFrameToTarget(PrimeFrame::Frame& frame,
                         RenderTarget const& target,
                         RenderOptions const& options = {});

bool renderFrameToPng(PrimeFrame::Frame& frame,
                      PrimeFrame::LayoutOutput const& layout,
                      std::string_view path,
                      RenderOptions const& options = {});

bool renderFrameToPng(PrimeFrame::Frame& frame,
                      std::string_view path,
                      RenderOptions const& options = {});

} // namespace PrimeStage
