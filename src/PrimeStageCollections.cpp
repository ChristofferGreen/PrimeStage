#include "PrimeStage/PrimeStage.h"

#include "PrimeStageCollectionInternals.h"

#include <algorithm>
#include <utility>

namespace PrimeStage {

UiNode UiNode::createList(ListSpec const& specInput) {
  ListSpec spec = Internal::normalizeListSpec(specInput);

  TableSpec table;
  table.accessibility = spec.accessibility;
  table.visible = spec.visible;
  table.enabled = spec.enabled;
  table.tabIndex = spec.tabIndex;
  table.size = spec.size;
  table.headerInset = 0.0f;
  table.headerHeight = 0.0f;
  table.rowHeight = spec.rowHeight;
  table.rowGap = spec.rowGap;
  table.headerPaddingX = spec.rowPaddingX;
  table.cellPaddingX = spec.rowPaddingX;
  table.rowStyle = spec.rowStyle;
  table.rowAltStyle = spec.rowAltStyle;
  table.selectionStyle = spec.selectionStyle;
  table.dividerStyle = spec.dividerStyle;
  table.focusStyle = spec.focusStyle;
  table.focusStyleOverride = spec.focusStyleOverride;
  table.selectedRow = spec.selectedIndex;
  table.showHeaderDividers = false;
  table.showColumnDividers = false;
  table.clipChildren = spec.clipChildren;
  table.columns.push_back(TableColumn{"", 0.0f, spec.textStyle, spec.textStyle});
  table.rows.reserve(spec.items.size());
  for (std::string_view item : spec.items) {
    table.rows.push_back({item});
  }
  auto onListSelect = spec.callbacks.onSelect ? spec.callbacks.onSelect : spec.callbacks.onSelected;
  if (onListSelect) {
    table.callbacks.onSelect =
        [callback = std::move(onListSelect)](TableRowInfo const& rowInfo) {
          ListRowInfo listInfo;
          listInfo.rowIndex = rowInfo.rowIndex;
          if (!rowInfo.row.empty()) {
            listInfo.item = rowInfo.row.front();
          }
          callback(listInfo);
        };
  }
  return createTable(table);
}

ScrollView UiNode::createScrollView(ScrollViewSpec const& specInput) {
  ScrollViewSpec spec = Internal::normalizeScrollViewSpec(specInput);
  Internal::WidgetRuntimeContext runtime = Internal::makeWidgetRuntimeContext(frame(),
                                                                              nodeId(),
                                                                              allowAbsolute(),
                                                                              true,
                                                                              spec.visible,
                                                                              -1);
  PrimeFrame::Frame& runtimeFrame = Internal::runtimeFrame(runtime);
  Internal::InternalRect bounds = Internal::resolveRect(spec.size);
  if (bounds.width <= 0.0f && !spec.size.preferredWidth.has_value()) {
    bounds.width = Internal::defaultScrollViewWidth();
  }
  if (bounds.height <= 0.0f && !spec.size.preferredHeight.has_value()) {
    bounds.height = Internal::defaultScrollViewHeight();
  }
  if (bounds.width <= 0.0f || bounds.height <= 0.0f) {
    return ScrollView{UiNode(runtimeFrame, runtime.parentId, runtime.allowAbsolute),
                      UiNode(runtimeFrame, PrimeFrame::NodeId{}, runtime.allowAbsolute)};
  }

  SizeSpec scrollSize = spec.size;
  if (!scrollSize.preferredWidth.has_value() && bounds.width > 0.0f) {
    scrollSize.preferredWidth = bounds.width;
  }
  if (!scrollSize.preferredHeight.has_value() && bounds.height > 0.0f) {
    scrollSize.preferredHeight = bounds.height;
  }
  PrimeFrame::NodeId scrollId = Internal::createNode(runtimeFrame,
                                                     runtime.parentId,
                                                     bounds,
                                                     &scrollSize,
                                                     PrimeFrame::LayoutType::None,
                                                     PrimeFrame::Insets{},
                                                     0.0f,
                                                     spec.clipChildren,
                                                     spec.visible);
  SizeSpec contentSize;
  contentSize.stretchX = 1.0f;
  contentSize.stretchY = 1.0f;
  PrimeFrame::NodeId contentId = Internal::createNode(runtimeFrame,
                                                      scrollId,
                                                      Internal::InternalRect{},
                                                      &contentSize,
                                                      PrimeFrame::LayoutType::Overlay,
                                                      PrimeFrame::Insets{},
                                                      0.0f,
                                                      false,
                                                      spec.visible);

  if (spec.showVertical && spec.vertical.enabled) {
    float trackW = spec.vertical.thickness;
    float trackH = std::max(0.0f, bounds.height - spec.vertical.startPadding - spec.vertical.endPadding);
    float trackX = bounds.width - spec.vertical.inset;
    float trackY = spec.vertical.startPadding;
    Internal::createRectNode(runtimeFrame,
                             scrollId,
                             Internal::InternalRect{trackX, trackY, trackW, trackH},
                             spec.vertical.trackStyle,
                             {},
                             false,
                             spec.visible);

    float thumbH = std::min(trackH, spec.vertical.thumbLength);
    float maxOffset = std::max(0.0f, trackH - thumbH);
    float thumbOffset = std::clamp(spec.vertical.thumbOffset, 0.0f, maxOffset);
    float thumbY = trackY + thumbOffset;
    Internal::createRectNode(runtimeFrame,
                             scrollId,
                             Internal::InternalRect{trackX, thumbY, trackW, thumbH},
                             spec.vertical.thumbStyle,
                             {},
                             false,
                             spec.visible);
  }

  if (spec.showHorizontal && spec.horizontal.enabled) {
    float trackH = spec.horizontal.thickness;
    float trackW = std::max(0.0f, bounds.width - spec.horizontal.startPadding - spec.horizontal.endPadding);
    float trackX = spec.horizontal.startPadding;
    float trackY = bounds.height - spec.horizontal.inset;
    Internal::createRectNode(runtimeFrame,
                             scrollId,
                             Internal::InternalRect{trackX, trackY, trackW, trackH},
                             spec.horizontal.trackStyle,
                             {},
                             false,
                             spec.visible);

    float thumbW = std::min(trackW, spec.horizontal.thumbLength);
    float maxOffset = std::max(0.0f, trackW - thumbW);
    float thumbOffset = std::clamp(spec.horizontal.thumbOffset, 0.0f, maxOffset);
    float thumbX = trackX + thumbOffset;
    Internal::createRectNode(runtimeFrame,
                             scrollId,
                             Internal::InternalRect{thumbX, trackY, thumbW, trackH},
                             spec.horizontal.thumbStyle,
                             {},
                             false,
                             spec.visible);
  }

  return ScrollView{UiNode(runtimeFrame, scrollId, runtime.allowAbsolute),
                    UiNode(runtimeFrame, contentId, runtime.allowAbsolute)};
}

ScrollView UiNode::createScrollView(SizeSpec const& size,
                                    bool showVertical,
                                    bool showHorizontal) {
  ScrollViewSpec spec;
  spec.size = size;
  spec.showVertical = showVertical;
  spec.showHorizontal = showHorizontal;
  return createScrollView(spec);
}

} // namespace PrimeStage
