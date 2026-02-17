#pragma once

#include "PrimeFrame/Frame.h"
#include "PrimeFrame/Layout.h"

#include <cstdint>
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

bool renderFrameToPng(PrimeFrame::Frame& frame,
                      PrimeFrame::LayoutOutput const& layout,
                      std::string_view path,
                      RenderOptions const& options = {});

bool renderFrameToPng(PrimeFrame::Frame& frame,
                      std::string_view path,
                      RenderOptions const& options = {});

} // namespace PrimeStage
