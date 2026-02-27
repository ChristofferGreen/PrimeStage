#include "PrimeStage/PrimeStage.h"

#include "PrimeStageCollectionInternals.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace PrimeStage {
namespace {

constexpr float DefaultParagraphWrapWidth = 360.0f;

std::vector<std::string> wrapTextLines(PrimeFrame::Frame& frame,
                                       PrimeFrame::TextStyleToken token,
                                       std::string_view text,
                                       float maxWidth,
                                       PrimeFrame::WrapMode wrap) {
  std::vector<std::string> lines;
  if (text.empty()) {
    return lines;
  }

  if (maxWidth <= 0.0f || wrap == PrimeFrame::WrapMode::None) {
    std::string current;
    for (char ch : text) {
      if (ch == '\n') {
        lines.push_back(current);
        current.clear();
        continue;
      }
      current.push_back(ch);
    }
    if (!current.empty() || (!text.empty() && text.back() == '\n')) {
      lines.push_back(current);
    }
    return lines;
  }

  float spaceWidth = Internal::estimateTextWidth(frame, token, " ");
  float lineWidth = 0.0f;
  std::string current;
  std::string word;
  bool wrapByChar = wrap == PrimeFrame::WrapMode::Character;

  auto flushWord = [&]() {
    if (word.empty()) {
      return;
    }
    float wordWidth = Internal::estimateTextWidth(frame, token, word);
    if (!current.empty() && lineWidth + spaceWidth + wordWidth > maxWidth) {
      lines.push_back(current);
      current.clear();
      lineWidth = 0.0f;
    }
    if (!current.empty()) {
      current.push_back(' ');
      lineWidth += spaceWidth;
    }
    current += word;
    lineWidth += wordWidth;
    word.clear();
  };

  for (char ch : text) {
    if (ch == '\n') {
      flushWord();
      lines.push_back(current);
      current.clear();
      lineWidth = 0.0f;
      continue;
    }
    if (std::isspace(static_cast<unsigned char>(ch))) {
      flushWord();
      continue;
    }
    word.push_back(ch);
    if (wrapByChar) {
      flushWord();
    }
  }
  flushWord();
  if (!current.empty()) {
    lines.push_back(current);
  }

  return lines;
}

} // namespace

float measureTextWidth(PrimeFrame::Frame& frame,
                       PrimeFrame::TextStyleToken token,
                       std::string_view text);

UiNode UiNode::createParagraph(ParagraphSpec const& specInput) {
  ParagraphSpec spec = Internal::normalizeParagraphSpec(specInput);

  Internal::InternalRect bounds = Internal::resolveRect(spec.size);
  PrimeFrame::TextStyleToken token = spec.textStyle;
  float maxWidth = spec.maxWidth > 0.0f ? spec.maxWidth : bounds.width;
  if (maxWidth <= 0.0f && spec.size.maxWidth.has_value()) {
    maxWidth = std::max(0.0f, *spec.size.maxWidth);
  }
  if (maxWidth <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f &&
      !spec.text.empty()) {
    maxWidth = DefaultParagraphWrapWidth;
  }
  if (bounds.width <= 0.0f &&
      maxWidth > 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f) {
    bounds.width = maxWidth;
  }
  std::vector<std::string> lines = wrapTextLines(frame(), token, spec.text, maxWidth, spec.wrap);
  if (bounds.width <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f &&
      !lines.empty()) {
    float inferredWidth = 0.0f;
    for (auto const& line : lines) {
      inferredWidth = std::max(inferredWidth, measureTextWidth(frame(), token, line));
    }
    if (maxWidth > 0.0f) {
      inferredWidth = std::min(inferredWidth, maxWidth);
    }
    bounds.width = inferredWidth;
  }
  if (maxWidth <= 0.0f && bounds.width > 0.0f) {
    maxWidth = bounds.width;
  }

  float lineHeight = Internal::resolveLineHeight(frame(), token);
  if (spec.autoHeight &&
      bounds.height <= 0.0f &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchY <= 0.0f) {
    bounds.height = std::max(0.0f, lineHeight * static_cast<float>(lines.size()));
  }

  PrimeFrame::NodeId paragraphId = Internal::createNode(frame(),
                                                        id_,
                                                        bounds,
                                                        &spec.size,
                                                        PrimeFrame::LayoutType::None,
                                                        PrimeFrame::Insets{},
                                                        0.0f,
                                                        false,
                                                        spec.visible);
  if (PrimeFrame::Node* node = frame().getNode(paragraphId)) {
    node->hitTestVisible = false;
  }

  for (size_t i = 0; i < lines.size(); ++i) {
    float width = maxWidth > 0.0f ? maxWidth : bounds.width;
    Internal::createTextNode(frame(),
                             paragraphId,
                             Internal::InternalRect{0.0f,
                                                    spec.textOffsetY + static_cast<float>(i) * lineHeight,
                                                    width,
                                                    lineHeight},
                             lines[i],
                             token,
                             spec.textStyleOverride,
                             spec.align,
                             PrimeFrame::WrapMode::None,
                             maxWidth,
                             spec.visible);
  }

  return UiNode(frame(), paragraphId, allowAbsolute_);
}

UiNode UiNode::createParagraph(std::string_view text,
                               PrimeFrame::TextStyleToken textStyle,
                               SizeSpec const& size) {
  ParagraphSpec spec;
  spec.text = text;
  spec.textStyle = textStyle;
  spec.size = size;
  return createParagraph(spec);
}

} // namespace PrimeStage
