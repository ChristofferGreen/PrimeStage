#include "PrimeStage/PrimeStage.h"

#include "PrimeStageCollectionInternals.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace PrimeStage {
namespace {

void addTextPrimitive(PrimeFrame::Frame& frame,
                      PrimeFrame::NodeId nodeId,
                      std::string_view text,
                      PrimeFrame::TextStyleToken textStyle,
                      PrimeFrame::TextStyleOverride const& overrideStyle,
                      PrimeFrame::TextAlign align,
                      PrimeFrame::WrapMode wrap,
                      float maxWidth,
                      float width,
                      float height) {
  PrimeFrame::Primitive prim;
  prim.type = PrimeFrame::PrimitiveType::Text;
  prim.width = width;
  prim.height = height;
  prim.textBlock.text = std::string(text);
  prim.textBlock.align = align;
  prim.textBlock.wrap = wrap;
  prim.textBlock.maxWidth = maxWidth;
  prim.textStyle.token = textStyle;
  prim.textStyle.overrideStyle = overrideStyle;
  PrimeFrame::PrimitiveId pid = frame.addPrimitive(prim);
  if (PrimeFrame::Node* node = frame.getNode(nodeId)) {
    node->primitives.push_back(pid);
  }
}

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

UiNode UiNode::createLabel(LabelSpec const& specInput) {
  LabelSpec spec = Internal::normalizeLabelSpec(specInput);

  Internal::InternalRect rect = Internal::resolveRect(spec.size);
  if ((rect.width <= 0.0f || rect.height <= 0.0f) &&
      !spec.text.empty() &&
      !spec.size.preferredWidth.has_value() &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchX <= 0.0f &&
      spec.size.stretchY <= 0.0f) {
    float lineHeight = Internal::resolveLineHeight(frame(), spec.textStyle);
    float textWidth = Internal::estimateTextWidth(frame(), spec.textStyle, spec.text);
    if (rect.width <= 0.0f) {
      rect.width = spec.maxWidth > 0.0f ? std::min(textWidth, spec.maxWidth) : textWidth;
    }
    if (rect.height <= 0.0f) {
      float wrapWidth = spec.maxWidth > 0.0f ? spec.maxWidth : rect.width;
      float height = lineHeight;
      if (spec.wrap != PrimeFrame::WrapMode::None && wrapWidth > 0.0f) {
        std::vector<std::string> lines =
            wrapTextLines(frame(), spec.textStyle, spec.text, wrapWidth, spec.wrap);
        height = lineHeight * static_cast<float>(std::max<size_t>(1, lines.size()));
      }
      rect.height = height;
    }
  }
  PrimeFrame::NodeId nodeId = Internal::createNode(frame(),
                                                   id_,
                                                   rect,
                                                   &spec.size,
                                                   PrimeFrame::LayoutType::None,
                                                   PrimeFrame::Insets{},
                                                   0.0f,
                                                   false,
                                                   spec.visible);
  if (PrimeFrame::Node* node = frame().getNode(nodeId)) {
    node->hitTestVisible = false;
  }
  addTextPrimitive(frame(),
                   nodeId,
                   spec.text,
                   spec.textStyle,
                   spec.textStyleOverride,
                   spec.align,
                   spec.wrap,
                   spec.maxWidth,
                   rect.width,
                   rect.height);
  return UiNode(frame(), nodeId, allowAbsolute_);
}

UiNode UiNode::createLabel(std::string_view text,
                           PrimeFrame::TextStyleToken textStyle,
                           SizeSpec const& size) {
  LabelSpec spec;
  spec.text = text;
  spec.textStyle = textStyle;
  spec.size = size;
  return createLabel(spec);
}

} // namespace PrimeStage
