#pragma once

#include "PrimeStage/Ui.h"

namespace PrimeStage::Internal {

struct InternalRect {
  float x = 0.0f;
  float y = 0.0f;
  float width = 0.0f;
  float height = 0.0f;
};

struct InternalFocusStyle {
  PrimeFrame::RectStyleToken token = 0;
  PrimeFrame::RectStyleOverride overrideStyle{};
};

ListSpec normalizeListSpec(ListSpec const& specInput);
TableSpec normalizeTableSpec(TableSpec const& specInput);
TreeViewSpec normalizeTreeViewSpec(TreeViewSpec const& specInput);
ScrollViewSpec normalizeScrollViewSpec(ScrollViewSpec const& specInput);
InternalRect resolveRect(SizeSpec const& size);
float defaultScrollViewWidth();
float defaultScrollViewHeight();
float defaultCollectionWidth();
float defaultCollectionHeight();
float estimateTextWidth(PrimeFrame::Frame& frame,
                        PrimeFrame::TextStyleToken token,
                        std::string_view text);
float resolveLineHeight(PrimeFrame::Frame& frame, PrimeFrame::TextStyleToken token);
InternalFocusStyle resolveFocusStyle(PrimeFrame::Frame& frame,
                                     PrimeFrame::RectStyleToken focusStyle,
                                     PrimeFrame::RectStyleOverride const& focusStyleOverride,
                                     PrimeFrame::RectStyleToken fallbackA,
                                     PrimeFrame::RectStyleToken fallbackB,
                                     PrimeFrame::RectStyleToken fallbackC,
                                     PrimeFrame::RectStyleToken fallbackD,
                                     PrimeFrame::RectStyleToken fallbackE);
void attachFocusOverlay(PrimeFrame::Frame& frame,
                        PrimeFrame::NodeId nodeId,
                        InternalRect const& rect,
                        InternalFocusStyle const& focusStyle,
                        bool visible);
void addDisabledScrimOverlay(PrimeFrame::Frame& frame,
                             PrimeFrame::NodeId nodeId,
                             InternalRect const& rect,
                             bool visible);
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
PrimeFrame::NodeId createTextNode(PrimeFrame::Frame& frame,
                                  PrimeFrame::NodeId parent,
                                  InternalRect const& rect,
                                  std::string_view text,
                                  PrimeFrame::TextStyleToken textStyle,
                                  PrimeFrame::TextStyleOverride const& overrideStyle,
                                  PrimeFrame::TextAlign align,
                                  PrimeFrame::WrapMode wrap,
                                  float maxWidth,
                                  bool visible);

} // namespace PrimeStage::Internal
