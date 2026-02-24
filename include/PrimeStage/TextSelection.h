#pragma once

#include "PrimeFrame/Frame.h"

#include <cstdint>
#include <string_view>
#include <vector>

namespace PrimeStage {

struct TextSelectionLine {
  uint32_t start = 0u;
  uint32_t end = 0u;
  float width = 0.0f;
};

uint32_t utf8Prev(std::string_view text, uint32_t index);
uint32_t utf8Next(std::string_view text, uint32_t index);

uint32_t caretIndexForClick(PrimeFrame::Frame& frame,
                            PrimeFrame::TextStyleToken token,
                            std::string_view text,
                            float paddingX,
                            float localX);

std::vector<TextSelectionLine> wrapTextLineRanges(PrimeFrame::Frame& frame,
                                                  PrimeFrame::TextStyleToken token,
                                                  std::string_view text,
                                                  float maxWidth,
                                                  PrimeFrame::WrapMode wrap);

} // namespace PrimeStage
