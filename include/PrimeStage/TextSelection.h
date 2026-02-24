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

struct TextSelectionLayout {
  std::vector<TextSelectionLine> lines;
  float lineHeight = 0.0f;
};

struct TextSelectionRect {
  float x = 0.0f;
  float y = 0.0f;
  float width = 0.0f;
  float height = 0.0f;
};

uint32_t utf8Prev(std::string_view text, uint32_t index);
uint32_t utf8Next(std::string_view text, uint32_t index);

float textLineHeight(PrimeFrame::Frame& frame, PrimeFrame::TextStyleToken token);

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

TextSelectionLayout buildTextSelectionLayout(PrimeFrame::Frame& frame,
                                             PrimeFrame::TextStyleToken token,
                                             std::string_view text,
                                             float maxWidth,
                                             PrimeFrame::WrapMode wrap);

std::vector<TextSelectionRect> buildSelectionRects(PrimeFrame::Frame& frame,
                                                   PrimeFrame::TextStyleToken token,
                                                   std::string_view text,
                                                   TextSelectionLayout const& layout,
                                                   uint32_t selectionStart,
                                                   uint32_t selectionEnd,
                                                   float paddingX);

uint32_t caretIndexForClickInLayout(PrimeFrame::Frame& frame,
                                    PrimeFrame::TextStyleToken token,
                                    std::string_view text,
                                    TextSelectionLayout const& layout,
                                    float paddingX,
                                    float localX,
                                    float localY);

} // namespace PrimeStage
