#pragma once

#include "PrimeStage/Ui.h"

namespace PrimeStage::Internal {

struct InternalRect {
  float x = 0.0f;
  float y = 0.0f;
  float width = 0.0f;
  float height = 0.0f;
};

ListSpec normalizeListSpec(ListSpec const& specInput);
ScrollViewSpec normalizeScrollViewSpec(ScrollViewSpec const& specInput);
InternalRect resolveRect(SizeSpec const& size);
float defaultScrollViewWidth();
float defaultScrollViewHeight();
PrimeFrame::NodeId createNode(PrimeFrame::Frame& frame,
                              PrimeFrame::NodeId parent,
                              InternalRect const& rect,
                              SizeSpec const* size,
                              PrimeFrame::LayoutType layout,
                              PrimeFrame::Insets const& padding,
                              float gap,
                              bool clipChildren,
                              bool visible,
                              char const* context = "UiNode");
PrimeFrame::NodeId createRectNode(PrimeFrame::Frame& frame,
                                  PrimeFrame::NodeId parent,
                                  InternalRect const& rect,
                                  PrimeFrame::RectStyleToken token,
                                  PrimeFrame::RectStyleOverride const& overrideStyle,
                                  bool clipChildren,
                                  bool visible);

} // namespace PrimeStage::Internal
