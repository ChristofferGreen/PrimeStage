#include "PrimeStage/PrimeStage.h"

#include "PrimeStageCollectionInternals.h"

#include <algorithm>

namespace PrimeStage {

UiNode UiNode::createTextLine(TextLineSpec const& specInput) {
  TextLineSpec spec = Internal::normalizeTextLineSpec(specInput);
  PrimeFrame::TextStyleToken token = spec.textStyle;
  float lineHeight = Internal::resolveLineHeight(frame(), token);
  Internal::InternalRect bounds = Internal::resolveRect(spec.size);
  if ((bounds.width <= 0.0f || bounds.height <= 0.0f) &&
      !spec.size.preferredWidth.has_value() &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchX <= 0.0f &&
      spec.size.stretchY <= 0.0f &&
      !spec.text.empty()) {
    float textWidth = Internal::estimateTextWidth(frame(), token, spec.text);
    if (bounds.width <= 0.0f) {
      bounds.width = textWidth;
    }
    if (bounds.height <= 0.0f) {
      bounds.height = lineHeight;
    }
  }
  float containerHeight = bounds.height > 0.0f ? bounds.height : lineHeight;
  float textY = (containerHeight - lineHeight) * 0.5f + spec.textOffsetY;

  float textWidth = Internal::estimateTextWidth(frame(), token, spec.text);
  float containerWidth = bounds.width;
  bool manualAlign = spec.align != PrimeFrame::TextAlign::Start &&
                     containerWidth > 0.0f &&
                     textWidth > 0.0f;

  PrimeFrame::NodeId lineId;
  if (manualAlign) {
    float offset = 0.0f;
    if (spec.align == PrimeFrame::TextAlign::Center) {
      offset = (containerWidth - textWidth) * 0.5f;
    } else if (spec.align == PrimeFrame::TextAlign::End) {
      offset = containerWidth - textWidth;
    }
    float x = std::max(0.0f, offset);
    lineId = Internal::createTextNode(frame(),
                                      id_,
                                      Internal::InternalRect{x, textY, textWidth, lineHeight},
                                      spec.text,
                                      token,
                                      spec.textStyleOverride,
                                      PrimeFrame::TextAlign::Start,
                                      PrimeFrame::WrapMode::None,
                                      textWidth,
                                      spec.visible);
  } else {
    float width = containerWidth > 0.0f ? containerWidth : textWidth;
    lineId = Internal::createTextNode(frame(),
                                      id_,
                                      Internal::InternalRect{0.0f, textY, width, lineHeight},
                                      spec.text,
                                      token,
                                      spec.textStyleOverride,
                                      spec.align,
                                      PrimeFrame::WrapMode::None,
                                      width,
                                      spec.visible);
  }
  return UiNode(frame(), lineId, allowAbsolute_);
}

UiNode UiNode::createTextLine(std::string_view text,
                              PrimeFrame::TextStyleToken textStyle,
                              SizeSpec const& size,
                              PrimeFrame::TextAlign align) {
  TextLineSpec spec;
  spec.text = text;
  spec.textStyle = textStyle;
  spec.align = align;
  spec.size = size;
  return createTextLine(spec);
}

} // namespace PrimeStage
