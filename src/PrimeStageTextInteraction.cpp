#include "PrimeStage/PrimeStage.h"
#include "PrimeStage/TextSelection.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <limits>

#if defined(PRIMESTAGE_HAS_PRIMEMANIFEST)
#include "PrimeManifest/text/FontRegistry.hpp"
#include "PrimeManifest/text/Typography.hpp"
#endif

namespace PrimeStage {
namespace {

bool is_utf8_continuation(uint8_t value) {
  return (value & 0xC0u) == 0x80u;
}

float resolve_line_height(PrimeFrame::Frame& frame, PrimeFrame::TextStyleToken token) {
  PrimeFrame::Theme const* theme = frame.getTheme(PrimeFrame::DefaultThemeId);
  if (!theme) {
    return 0.0f;
  }
  PrimeFrame::ResolvedTextStyle resolved = PrimeFrame::resolveTextStyle(*theme, token, {});
  float lineHeight = resolved.lineHeight > 0.0f ? resolved.lineHeight : resolved.size * 1.2f;
  return lineHeight;
}

#if defined(PRIMESTAGE_HAS_PRIMEMANIFEST)
PrimeManifest::Typography make_typography(PrimeFrame::Frame& frame, PrimeFrame::TextStyleToken token) {
  PrimeManifest::Typography typography;
  PrimeFrame::Theme const* theme = frame.getTheme(PrimeFrame::DefaultThemeId);
  if (!theme) {
    return typography;
  }
  PrimeFrame::ResolvedTextStyle resolved = PrimeFrame::resolveTextStyle(*theme, token, {});
  typography.size = resolved.size;
  typography.weight = static_cast<int>(std::lround(resolved.weight));
  typography.lineHeight = resolved.lineHeight > 0.0f ? resolved.lineHeight : resolved.size * 1.2f;
  typography.letterSpacing = resolved.tracking;
  if (resolved.slant != 0.0f) {
    typography.slant = PrimeManifest::FontSlant::Italic;
  }
#if defined(PRIMESTAGE_HAS_BUNDLED_FONT) && PRIMESTAGE_HAS_BUNDLED_FONT
  typography.fallback = PrimeManifest::FontFallbackPolicy::BundleOnly;
#else
  typography.fallback = PrimeManifest::FontFallbackPolicy::BundleThenOS;
#endif
  return typography;
}

void ensure_text_fonts_loaded() {
  static bool fontsLoaded = false;
  if (fontsLoaded) {
    return;
  }
  auto& registry = PrimeManifest::GetFontRegistry();
#if defined(PRIMESTAGE_HAS_BUNDLED_FONT) && PRIMESTAGE_HAS_BUNDLED_FONT
  registry.addBundleDir(PRIMESTAGE_BUNDLED_FONT_DIR);
#endif
  registry.loadBundledFonts();
  registry.loadOsFallbackFonts();
  fontsLoaded = true;
}
#endif

} // namespace

float measureTextWidth(PrimeFrame::Frame& frame,
                       PrimeFrame::TextStyleToken token,
                       std::string_view text) {
  if (text.empty()) {
    return 0.0f;
  }
  PrimeFrame::Theme const* theme = frame.getTheme(PrimeFrame::DefaultThemeId);
  if (!theme) {
    return 0.0f;
  }
  PrimeFrame::ResolvedTextStyle resolved = PrimeFrame::resolveTextStyle(*theme, token, {});
#if defined(PRIMESTAGE_HAS_PRIMEMANIFEST)
  ensure_text_fonts_loaded();
  auto& registry = PrimeManifest::GetFontRegistry();
  PrimeManifest::Typography typography = make_typography(frame, token);
  typography.lineHeight = resolved.lineHeight > 0.0f ? resolved.lineHeight : typography.lineHeight;
  auto measured = registry.measureText(text, typography);
  return static_cast<float>(measured.first);
#else
  float advance = resolved.size * 0.6f + resolved.tracking;
  float lineWidth = 0.0f;
  float maxWidth = 0.0f;
  for (char ch : text) {
    if (ch == '\n') {
      maxWidth = std::max(maxWidth, lineWidth);
      lineWidth = 0.0f;
      continue;
    }
    lineWidth += advance;
  }
  maxWidth = std::max(maxWidth, lineWidth);
  return maxWidth;
#endif
}

float textLineHeight(PrimeFrame::Frame& frame, PrimeFrame::TextStyleToken token) {
  return resolve_line_height(frame, token);
}

uint32_t utf8Prev(std::string_view text, uint32_t index) {
  if (index == 0u) {
    return 0u;
  }
  uint32_t size = static_cast<uint32_t>(text.size());
  uint32_t i = std::min(index, size);
  if (i > 0u) {
    --i;
  }
  while (i > 0u && is_utf8_continuation(static_cast<uint8_t>(text[i]))) {
    --i;
  }
  return i;
}

uint32_t utf8Next(std::string_view text, uint32_t index) {
  uint32_t size = static_cast<uint32_t>(text.size());
  if (index >= size) {
    return size;
  }
  uint32_t i = index + 1u;
  while (i < size && is_utf8_continuation(static_cast<uint8_t>(text[i]))) {
    ++i;
  }
  return i;
}

bool textFieldHasSelection(TextFieldState const& state,
                           uint32_t& start,
                           uint32_t& end) {
  start = std::min(state.selectionStart, state.selectionEnd);
  end = std::max(state.selectionStart, state.selectionEnd);
  return start != end;
}

void clearTextFieldSelection(TextFieldState& state, uint32_t cursor) {
  state.selectionAnchor = cursor;
  state.selectionStart = cursor;
  state.selectionEnd = cursor;
  state.selecting = false;
  state.pointerId = -1;
}

bool updateTextFieldBlink(TextFieldState& state,
                          std::chrono::steady_clock::time_point now,
                          std::chrono::milliseconds interval) {
  bool changed = false;
  if (state.focused) {
    if (state.nextBlink.time_since_epoch().count() == 0) {
      state.cursorVisible = true;
      state.nextBlink = now + interval;
      changed = true;
    } else if (now >= state.nextBlink) {
      state.cursorVisible = !state.cursorVisible;
      state.nextBlink = now + interval;
      changed = true;
    }
  } else if (state.cursorVisible || state.nextBlink.time_since_epoch().count() != 0) {
    state.cursorVisible = false;
    state.nextBlink = {};
    changed = true;
  }
  return changed;
}

bool selectableTextHasSelection(SelectableTextState const& state,
                                uint32_t& start,
                                uint32_t& end) {
  start = std::min(state.selectionStart, state.selectionEnd);
  end = std::max(state.selectionStart, state.selectionEnd);
  return start != end;
}

void clearSelectableTextSelection(SelectableTextState& state, uint32_t anchor) {
  state.selectionAnchor = anchor;
  state.selectionStart = anchor;
  state.selectionEnd = anchor;
  state.selecting = false;
  state.pointerId = -1;
}

std::vector<float> buildCaretPositions(PrimeFrame::Frame& frame,
                                       PrimeFrame::TextStyleToken token,
                                       std::string_view text) {
  std::vector<float> positions;
  if (text.empty()) {
    positions.resize(1, 0.0f);
    return positions;
  }

  positions.assign(text.size() + 1u, std::numeric_limits<float>::quiet_NaN());
  positions[0] = 0.0f;

#if defined(PRIMESTAGE_HAS_PRIMEMANIFEST)
  bool usedLayout = false;
  ensure_text_fonts_loaded();
  PrimeManifest::Typography typography = make_typography(frame, token);
  auto run = PrimeManifest::LayoutText(text, typography, 1.0f, false);
  if (run) {
    float penX = 0.0f;
    for (auto const& glyph : run->glyphs) {
      uint32_t cluster = std::min<uint32_t>(glyph.cluster, static_cast<uint32_t>(text.size()));
      if (!std::isfinite(positions[cluster])) {
        positions[cluster] = penX;
      }
      penX += glyph.advance;
    }
    positions[text.size()] = penX;
    usedLayout = true;
  }
#endif

  uint32_t index = utf8Next(text, 0u);
  while (index <= text.size()) {
#if defined(PRIMESTAGE_HAS_PRIMEMANIFEST)
    if (!usedLayout || !std::isfinite(positions[index])) {
      positions[index] = measureTextWidth(frame, token, text.substr(0, index));
    }
#else
    positions[index] = measureTextWidth(frame, token, text.substr(0, index));
#endif
    if (index == text.size()) {
      break;
    }
    index = utf8Next(text, index);
  }

  float last = positions[0];
  for (uint32_t i = 1u; i <= text.size(); ++i) {
    if (!std::isfinite(positions[i])) {
      positions[i] = last;
    } else {
      last = positions[i];
    }
  }

  return positions;
}

uint32_t caretIndexForClick(PrimeFrame::Frame& frame,
                            PrimeFrame::TextStyleToken token,
                            std::string_view text,
                            float paddingX,
                            float localX) {
  if (text.empty()) {
    return 0u;
  }
  float targetX = localX - paddingX;
  if (targetX <= 0.0f) {
    return 0u;
  }
  auto positions = buildCaretPositions(frame, token, text);
  float totalWidth = positions.back();
  if (targetX >= totalWidth) {
    return static_cast<uint32_t>(text.size());
  }
  uint32_t prevIndex = 0u;
  float prevWidth = positions[0];
  uint32_t index = utf8Next(text, 0u);
  while (index <= text.size()) {
    float width = positions[index];
    if (width >= targetX) {
      float prevDist = targetX - prevWidth;
      float nextDist = width - targetX;
      return (prevDist <= nextDist) ? prevIndex : index;
    }
    prevIndex = index;
    prevWidth = width;
    index = utf8Next(text, index);
  }
  return static_cast<uint32_t>(text.size());
}

std::vector<TextSelectionLine> wrapTextLineRanges(PrimeFrame::Frame& frame,
                                                  PrimeFrame::TextStyleToken token,
                                                  std::string_view text,
                                                  float maxWidth,
                                                  PrimeFrame::WrapMode wrap) {
  std::vector<TextSelectionLine> lines;
  if (text.empty()) {
    lines.push_back({0u, 0u, 0.0f});
    return lines;
  }
  if (maxWidth <= 0.0f || wrap == PrimeFrame::WrapMode::None) {
    uint32_t lineStart = 0u;
    for (uint32_t i = 0u; i < text.size(); ++i) {
      if (text[i] == '\n') {
        float width = measureTextWidth(frame, token, text.substr(lineStart, i - lineStart));
        lines.push_back({lineStart, i, width});
        lineStart = i + 1u;
      }
    }
    float width = measureTextWidth(frame, token, text.substr(lineStart, text.size() - lineStart));
    lines.push_back({lineStart, static_cast<uint32_t>(text.size()), width});
    return lines;
  }

  float spaceWidth = measureTextWidth(frame, token, " ");
  bool wrapByChar = wrap == PrimeFrame::WrapMode::Character;
  uint32_t i = 0u;
  uint32_t lineStart = 0u;
  uint32_t lineEnd = 0u;
  float lineWidth = 0.0f;
  bool lineHasWord = false;

  auto push_line = [&](uint32_t endIndex, float width) {
    lines.push_back({lineStart, endIndex, width});
    lineStart = endIndex;
    lineEnd = endIndex;
    lineWidth = 0.0f;
    lineHasWord = false;
  };

  while (i < text.size()) {
    char ch = text[i];
    if (ch == '\n') {
      push_line(lineHasWord ? lineEnd : i, lineWidth);
      ++i;
      lineStart = i;
      continue;
    }
    if (std::isspace(static_cast<unsigned char>(ch))) {
      ++i;
      continue;
    }
    uint32_t wordStart = i;
    if (wrapByChar) {
      i = utf8Next(text, i);
    } else {
      while (i < text.size()) {
        char wordCh = text[i];
        if (wordCh == '\n' || std::isspace(static_cast<unsigned char>(wordCh))) {
          break;
        }
        ++i;
      }
    }
    uint32_t wordEnd = i;
    if (wordEnd <= wordStart) {
      ++i;
      continue;
    }
    float wordWidth = measureTextWidth(frame, token, text.substr(wordStart, wordEnd - wordStart));
    if (lineHasWord && lineWidth + spaceWidth + wordWidth > maxWidth) {
      push_line(lineEnd, lineWidth);
    }
    if (!lineHasWord) {
      lineStart = wordStart;
      lineEnd = wordEnd;
      lineWidth = wordWidth;
      lineHasWord = true;
    } else {
      lineEnd = wordEnd;
      lineWidth += spaceWidth + wordWidth;
    }
  }
  if (lineHasWord) {
    push_line(lineEnd, lineWidth);
  } else if (lineStart < text.size()) {
    lines.push_back({lineStart, static_cast<uint32_t>(text.size()), 0.0f});
  }
  if (lines.empty()) {
    lines.push_back({0u, static_cast<uint32_t>(text.size()), 0.0f});
  }
  return lines;
}

TextSelectionLayout buildTextSelectionLayout(PrimeFrame::Frame& frame,
                                             PrimeFrame::TextStyleToken token,
                                             std::string_view text,
                                             float maxWidth,
                                             PrimeFrame::WrapMode wrap) {
  TextSelectionLayout layout;
  layout.lines = wrapTextLineRanges(frame, token, text, maxWidth, wrap);
  layout.lineHeight = textLineHeight(frame, token);
  if (layout.lineHeight <= 0.0f) {
    layout.lineHeight = 1.0f;
  }
  return layout;
}

std::vector<TextSelectionRect> buildSelectionRects(PrimeFrame::Frame& frame,
                                                   PrimeFrame::TextStyleToken token,
                                                   std::string_view text,
                                                   TextSelectionLayout const& layout,
                                                   uint32_t selectionStart,
                                                   uint32_t selectionEnd,
                                                   float paddingX) {
  std::vector<TextSelectionRect> rects;
  if (text.empty() || layout.lines.empty() || selectionStart == selectionEnd) {
    return rects;
  }
  uint32_t textSize = static_cast<uint32_t>(text.size());
  uint32_t selStart = std::min(selectionStart, selectionEnd);
  uint32_t selEnd = std::max(selectionStart, selectionEnd);
  selStart = std::min(selStart, textSize);
  selEnd = std::min(selEnd, textSize);
  if (selStart >= selEnd) {
    return rects;
  }
  for (size_t lineIndex = 0; lineIndex < layout.lines.size(); ++lineIndex) {
    const auto& line = layout.lines[lineIndex];
    if (selEnd <= line.start || selStart >= line.end) {
      continue;
    }
    uint32_t localStart = std::max(selStart, line.start) - line.start;
    uint32_t localEnd = std::min(selEnd, line.end) - line.start;
    std::string_view lineText(text.data() + line.start, line.end - line.start);
    auto caretPositions = buildCaretPositions(frame, token, lineText);
    uint32_t maxIndex = static_cast<uint32_t>(lineText.size());
    localStart = std::min(localStart, maxIndex);
    localEnd = std::min(localEnd, maxIndex);
    float leftWidth = caretPositions[localStart];
    float rightWidth = caretPositions[localEnd];
    float width = rightWidth - leftWidth;
    if (width <= 0.0f) {
      continue;
    }
    TextSelectionRect rect;
    rect.x = paddingX + leftWidth;
    rect.y = static_cast<float>(lineIndex) * layout.lineHeight;
    rect.width = width;
    rect.height = layout.lineHeight;
    rects.push_back(rect);
  }
  return rects;
}

uint32_t caretIndexForClickInLayout(PrimeFrame::Frame& frame,
                                    PrimeFrame::TextStyleToken token,
                                    std::string_view text,
                                    TextSelectionLayout const& layout,
                                    float paddingX,
                                    float localX,
                                    float localY) {
  if (layout.lines.empty() || layout.lineHeight <= 0.0f) {
    return caretIndexForClick(frame, token, text, paddingX, localX);
  }
  float lineHeight = layout.lineHeight;
  int lineIndex = static_cast<int>(localY / lineHeight);
  lineIndex = std::clamp(lineIndex, 0, static_cast<int>(layout.lines.size() - 1));
  const auto& line = layout.lines[static_cast<size_t>(lineIndex)];
  std::string_view lineText(text.data() + line.start, line.end - line.start);
  uint32_t localIndex = caretIndexForClick(frame, token, lineText, paddingX, localX);
  return line.start + localIndex;
}


} // namespace PrimeStage
