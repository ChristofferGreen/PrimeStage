#include "PrimeStage/PrimeStage.h"

#include "PrimeStageCollectionInternals.h"
#include "PrimeFrame/Events.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <optional>
#include <utility>

namespace PrimeStage {

namespace {

struct Rect {
  float x = 0.0f;
  float y = 0.0f;
  float width = 0.0f;
  float height = 0.0f;
};

struct FlatTreeRow {
  std::string_view label;
  int depth = 0;
  int parentIndex = -1;
  bool hasChildren = false;
  bool expanded = true;
  bool selected = false;
  std::vector<int> ancestors;
  std::vector<uint32_t> path;
};

struct ResolvedFocusStyle {
  PrimeFrame::RectStyleToken token = 0;
  PrimeFrame::RectStyleOverride overrideStyle{};
};

struct FocusOverlay {};

Rect resolve_rect(SizeSpec const& size) {
  Internal::InternalRect resolved = Internal::resolveRect(size);
  return Rect{resolved.x, resolved.y, resolved.width, resolved.height};
}

float estimate_text_width(PrimeFrame::Frame& frame,
                          PrimeFrame::TextStyleToken token,
                          std::string_view text) {
  return Internal::estimateTextWidth(frame, token, text);
}

float resolve_line_height(PrimeFrame::Frame& frame, PrimeFrame::TextStyleToken token) {
  return Internal::resolveLineHeight(frame, token);
}

PrimeFrame::NodeId create_rect_node(PrimeFrame::Frame& frame,
                                    PrimeFrame::NodeId parent,
                                    Rect const& rect,
                                    PrimeFrame::RectStyleToken token,
                                    PrimeFrame::RectStyleOverride const& overrideStyle,
                                    bool clipChildren,
                                    bool visible) {
  return Internal::createRectNode(frame,
                                  parent,
                                  Internal::InternalRect{rect.x, rect.y, rect.width, rect.height},
                                  token,
                                  overrideStyle,
                                  clipChildren,
                                  visible);
}

PrimeFrame::NodeId create_text_node(PrimeFrame::Frame& frame,
                                    PrimeFrame::NodeId parent,
                                    Rect const& rect,
                                    std::string_view text,
                                    PrimeFrame::TextStyleToken textStyle,
                                    PrimeFrame::TextStyleOverride const& overrideStyle,
                                    PrimeFrame::TextAlign align,
                                    PrimeFrame::WrapMode wrap,
                                    float maxWidth,
                                    bool visible) {
  return Internal::createTextNode(frame,
                                  parent,
                                  Internal::InternalRect{rect.x, rect.y, rect.width, rect.height},
                                  text,
                                  textStyle,
                                  overrideStyle,
                                  align,
                                  wrap,
                                  maxWidth,
                                  visible);
}

void flatten_tree(std::vector<TreeNode> const& nodes,
                  int depth,
                  std::vector<int>& depthStack,
                  std::vector<uint32_t>& pathStack,
                  std::vector<FlatTreeRow>& out) {
  for (size_t i = 0; i < nodes.size(); ++i) {
    TreeNode const& node = nodes[i];
    int parentIndex = depth > 0 && depth - 1 < static_cast<int>(depthStack.size())
                          ? depthStack[static_cast<size_t>(depth - 1)]
                          : -1;
    FlatTreeRow row;
    row.label = node.label;
    row.depth = depth;
    row.parentIndex = parentIndex;
    row.hasChildren = !node.children.empty();
    row.expanded = node.expanded;
    row.selected = node.selected;
    if (depth > 0 && depth <= static_cast<int>(depthStack.size())) {
      row.ancestors.assign(depthStack.begin(), depthStack.begin() + depth);
    }
    pathStack.push_back(static_cast<uint32_t>(i));
    row.path = pathStack;
    int index = static_cast<int>(out.size());
    out.push_back(std::move(row));

    if (depth >= static_cast<int>(depthStack.size())) {
      depthStack.resize(static_cast<size_t>(depth) + 1, -1);
    }
    depthStack[static_cast<size_t>(depth)] = index;

    if (node.expanded && !node.children.empty()) {
      flatten_tree(node.children, depth + 1, depthStack, pathStack, out);
    }
    pathStack.pop_back();
  }
}

void add_divider_rect(PrimeFrame::Frame& frame,
                      PrimeFrame::NodeId nodeId,
                      Rect const& bounds,
                      PrimeFrame::RectStyleToken token) {
  (void)create_rect_node(frame, nodeId, bounds, token, {}, false, true);
}

ResolvedFocusStyle resolve_focus_style(PrimeFrame::Frame& frame,
                                       PrimeFrame::RectStyleToken focusStyle,
                                       PrimeFrame::RectStyleOverride const& focusStyleOverride,
                                       std::initializer_list<PrimeFrame::RectStyleToken> fallbacks) {
  PrimeFrame::RectStyleToken fallbackA = 0;
  PrimeFrame::RectStyleToken fallbackB = 0;
  PrimeFrame::RectStyleToken fallbackC = 0;
  PrimeFrame::RectStyleToken fallbackD = 0;
  PrimeFrame::RectStyleToken fallbackE = 0;
  auto it = fallbacks.begin();
  if (it != fallbacks.end()) {
    fallbackA = *it++;
  }
  if (it != fallbacks.end()) {
    fallbackB = *it++;
  }
  if (it != fallbacks.end()) {
    fallbackC = *it++;
  }
  if (it != fallbacks.end()) {
    fallbackD = *it++;
  }
  if (it != fallbacks.end()) {
    fallbackE = *it++;
  }
  Internal::InternalFocusStyle resolved = Internal::resolveFocusStyle(frame,
                                                                      focusStyle,
                                                                      focusStyleOverride,
                                                                      fallbackA,
                                                                      fallbackB,
                                                                      fallbackC,
                                                                      fallbackD,
                                                                      fallbackE);
  return ResolvedFocusStyle{resolved.token, resolved.overrideStyle};
}

std::optional<FocusOverlay> add_focus_overlay_node(PrimeFrame::Frame& frame,
                                                   PrimeFrame::NodeId nodeId,
                                                   Rect const& rect,
                                                   PrimeFrame::RectStyleToken token,
                                                   PrimeFrame::RectStyleOverride const& overrideStyle,
                                                   bool visible) {
  Internal::attachFocusOverlay(frame,
                               nodeId,
                               Internal::InternalRect{rect.x, rect.y, rect.width, rect.height},
                               Internal::InternalFocusStyle{token, overrideStyle},
                               visible);
  return std::nullopt;
}

void attach_focus_callbacks(PrimeFrame::Frame& frame,
                            PrimeFrame::NodeId nodeId,
                            FocusOverlay const&) {
  (void)frame;
  (void)nodeId;
}

void add_state_scrim_overlay(PrimeFrame::Frame& frame,
                             PrimeFrame::NodeId nodeId,
                             Rect const& rect,
                             float opacity,
                             bool visible) {
  (void)opacity;
  Internal::addDisabledScrimOverlay(frame,
                                    nodeId,
                                    Internal::InternalRect{rect.x, rect.y, rect.width, rect.height},
                                    visible);
}

} // namespace

constexpr float DisabledScrimOpacity = 0.38f;
UiNode UiNode::createTreeView(TreeViewSpec const& spec) {
  TreeViewSpec normalized = Internal::normalizeTreeViewSpec(spec);
  bool enabled = normalized.enabled;
  PrimeFrame::NodeId id_ = nodeId();
  bool allowAbsolute_ = allowAbsolute();

  std::vector<FlatTreeRow> rows;
  std::vector<int> depthStack;
  std::vector<uint32_t> pathStack;
  flatten_tree(normalized.nodes, 0, depthStack, pathStack, rows);

  float rowsHeight = rows.empty()
                         ? normalized.rowHeight
                         : static_cast<float>(rows.size()) * spec.rowHeight +
                               static_cast<float>(rows.size() - 1) * spec.rowGap;

  std::vector<int> firstChild(rows.size(), -1);
  std::vector<int> lastChild(rows.size(), -1);
  for (size_t i = 0; i < rows.size(); ++i) {
    if (rows[i].parentIndex >= 0) {
      size_t parent = static_cast<size_t>(rows[i].parentIndex);
      if (firstChild[parent] < 0) {
        firstChild[parent] = static_cast<int>(i);
      }
      lastChild[parent] = static_cast<int>(i);
    }
  }

  Rect bounds = resolve_rect(normalized.size);
  if (bounds.width <= 0.0f || bounds.height <= 0.0f) {
    float maxLabelWidth = 0.0f;
    for (FlatTreeRow const& row : rows) {
      PrimeFrame::TextStyleToken role = row.selected ? normalized.selectedTextStyle
                                                     : normalized.textStyle;
      float textWidth = estimate_text_width(frame(), role, row.label);
      float indent = row.depth > 0 ? spec.indent * static_cast<float>(row.depth) : 0.0f;
      float contentWidth = normalized.rowWidthInset + 20.0f + indent + textWidth;
      if (contentWidth > maxLabelWidth) {
        maxLabelWidth = contentWidth;
      }
    }
    if (bounds.width <= 0.0f) {
      bounds.width = maxLabelWidth;
    }
    if (bounds.height <= 0.0f) {
      bounds.height = normalized.rowStartY + rowsHeight;
    }
  }
  if (bounds.width <= 0.0f &&
      !normalized.size.preferredWidth.has_value() &&
      normalized.size.stretchX <= 0.0f) {
    bounds.width = Internal::defaultCollectionWidth();
  }
  if (bounds.height <= 0.0f &&
      !normalized.size.preferredHeight.has_value() &&
      normalized.size.stretchY <= 0.0f) {
    bounds.height = Internal::defaultCollectionHeight();
  }

  if (bounds.width <= 0.0f || bounds.height <= 0.0f) {
    return UiNode(frame(), id_, allowAbsolute_);
  }

  SizeSpec treeSize = normalized.size;
  if (!treeSize.preferredWidth.has_value() && bounds.width > 0.0f && treeSize.stretchX <= 0.0f) {
    treeSize.preferredWidth = bounds.width;
  }
  if (!treeSize.preferredHeight.has_value() && bounds.height > 0.0f) {
    treeSize.preferredHeight = bounds.height;
  }

  StackSpec treeSpec;
  treeSpec.size = treeSize;
  treeSpec.gap = 0.0f;
  treeSpec.clipChildren = normalized.clipChildren;
  treeSpec.padding.left = 0.0f;
  treeSpec.padding.top = normalized.rowStartY;
  treeSpec.padding.right = 0.0f;
  treeSpec.visible = normalized.visible;
  UiNode parentNode(frame(), id_, allowAbsolute_);
  UiNode treeNode = parentNode.createOverlay(treeSpec);

  float rowWidth = std::max(0.0f, bounds.width);
  float rowTextHeight = resolve_line_height(frame(), normalized.textStyle);
  float selectedTextHeight = resolve_line_height(frame(), normalized.selectedTextStyle);
  float caretBaseX = std::max(0.0f, normalized.caretBaseX);
  float viewportHeight = std::max(0.0f, bounds.height - normalized.rowStartY);

  StackSpec rowsSpec;
  rowsSpec.size.stretchX = 1.0f;
  rowsSpec.size.stretchY = normalized.size.stretchY;
  rowsSpec.size.preferredWidth = rowWidth;
  rowsSpec.size.preferredHeight = viewportHeight;
  rowsSpec.gap = spec.rowGap;
  rowsSpec.clipChildren = normalized.clipChildren;
  rowsSpec.visible = normalized.visible;

  if (normalized.showHeaderDivider && normalized.visible) {
    float dividerY = normalized.headerDividerY;
    add_divider_rect(frame(), treeNode.nodeId(),
                     Rect{0.0f, dividerY, rowWidth, normalized.connectorThickness},
                     normalized.connectorStyle);
  }

  struct TreeViewRowVisual {
    PrimeFrame::PrimitiveId background = 0;
    PrimeFrame::PrimitiveId accent = 0;
    PrimeFrame::PrimitiveId mask = 0;
    PrimeFrame::PrimitiveId label = 0;
    PrimeFrame::RectStyleToken baseStyle = 0;
    PrimeFrame::RectStyleToken hoverStyle = 0;
    PrimeFrame::RectStyleToken selectionStyle = 0;
    PrimeFrame::TextStyleToken textStyle = 0;
    PrimeFrame::TextStyleToken selectedTextStyle = 0;
    bool hasAccent = false;
    bool hasMask = false;
    bool hasChildren = false;
    bool expanded = false;
    int depth = 0;
    int parentIndex = -1;
    std::vector<uint32_t> path;
  };

  struct TreeViewInteractionState {
    PrimeFrame::Frame* frame = nullptr;
    std::vector<TreeViewRowVisual> rows;
    TreeViewCallbacks callbacks;
    int hoveredRow = -1;
    int selectedRow = -1;
    int lastClickRow = -1;
    std::chrono::steady_clock::time_point lastClickTime{};
    std::chrono::duration<double, std::milli> doubleClickThreshold{0.0};
    PrimeFrame::NodeId viewportNode{};
    PrimeFrame::PrimitiveId scrollTrackPrim = 0;
    PrimeFrame::NodeId scrollThumbNode{};
    PrimeFrame::PrimitiveId scrollThumbPrim = 0;
    float viewportHeight = 0.0f;
    float contentHeight = 0.0f;
    float maxScroll = 0.0f;
    float scrollOffset = 0.0f;
    float trackY = 0.0f;
    float trackH = 0.0f;
    float thumbH = 0.0f;
    bool scrollEnabled = false;
    bool scrollDragging = false;
    int scrollPointerId = -1;
    float scrollDragStartY = 0.0f;
    float scrollDragStartOffset = 0.0f;
    int scrollHoverCount = 0;
    PrimeFrame::RectStyleOverride scrollTrackBaseOverride{};
    PrimeFrame::RectStyleOverride scrollThumbBaseOverride{};
    std::optional<float> scrollTrackHoverOpacity;
    std::optional<float> scrollTrackPressedOpacity;
    std::optional<float> scrollThumbHoverOpacity;
    std::optional<float> scrollThumbPressedOpacity;
  };

  auto interaction = std::make_shared<TreeViewInteractionState>();
  interaction->frame = &frame();
  interaction->callbacks = normalized.callbacks;
  interaction->doubleClickThreshold =
      std::chrono::duration<double, std::milli>(std::max(0.0f, normalized.doubleClickMs));
  interaction->rows.reserve(rows.size());
  interaction->viewportHeight = viewportHeight;
  interaction->contentHeight = rowsHeight;
  interaction->maxScroll = std::max(0.0f, rowsHeight - viewportHeight);
  interaction->scrollEnabled = interaction->maxScroll > 0.0f;
  float initialProgress = std::clamp(normalized.scrollBar.thumbProgress, 0.0f, 1.0f);
  if (!interaction->scrollEnabled) {
    initialProgress = 0.0f;
  }
  interaction->scrollOffset = initialProgress * interaction->maxScroll;
  interaction->scrollTrackBaseOverride = normalized.scrollBar.trackStyleOverride;
  interaction->scrollThumbBaseOverride = normalized.scrollBar.thumbStyleOverride;
  interaction->scrollTrackHoverOpacity = normalized.scrollBar.trackHoverOpacity;
  interaction->scrollTrackPressedOpacity = normalized.scrollBar.trackPressedOpacity;
  interaction->scrollThumbHoverOpacity = normalized.scrollBar.thumbHoverOpacity;
  interaction->scrollThumbPressedOpacity = normalized.scrollBar.thumbPressedOpacity;

  UiNode rowsNode = treeNode.createVerticalStack(rowsSpec);
  interaction->viewportNode = rowsNode.nodeId();
  if (PrimeFrame::Node* rowsNodePtr = frame().getNode(rowsNode.nodeId())) {
    rowsNodePtr->isViewport = true;
    rowsNodePtr->scrollY = interaction->scrollOffset;
    rowsNodePtr->hitTestVisible = enabled;
  }

  auto makeRowInfo = [interaction](int rowIndex) {
    TreeViewRowInfo info;
    info.rowIndex = rowIndex;
    if (rowIndex >= 0 && rowIndex < static_cast<int>(interaction->rows.size())) {
      const auto& row = interaction->rows[static_cast<size_t>(rowIndex)];
      info.path = row.path;
      info.hasChildren = row.hasChildren;
      info.expanded = row.expanded;
    }
    return info;
  };

  auto updateRowVisual = [interaction](int rowIndex) {
    if (rowIndex < 0 || rowIndex >= static_cast<int>(interaction->rows.size())) {
      return;
    }
    auto& row = interaction->rows[static_cast<size_t>(rowIndex)];
    bool selected = (rowIndex == interaction->selectedRow);
    bool hovered = (rowIndex == interaction->hoveredRow);
    PrimeFrame::RectStyleToken style = row.baseStyle;
    if (selected) {
      style = row.selectionStyle;
    } else if (hovered && row.hoverStyle != 0) {
      style = row.hoverStyle;
    }
    if (PrimeFrame::Primitive* prim = interaction->frame->getPrimitive(row.background)) {
      if (prim->type == PrimeFrame::PrimitiveType::Rect) {
        prim->rect.token = style;
      }
    }
    if (row.hasMask) {
      if (PrimeFrame::Primitive* prim = interaction->frame->getPrimitive(row.mask)) {
        if (prim->type == PrimeFrame::PrimitiveType::Rect) {
          prim->rect.token = style;
        }
      }
    }
    if (PrimeFrame::Primitive* prim = interaction->frame->getPrimitive(row.label)) {
      if (prim->type == PrimeFrame::PrimitiveType::Text) {
        prim->textStyle.token = selected ? row.selectedTextStyle : row.textStyle;
      }
    }
    if (row.hasAccent) {
      if (PrimeFrame::Primitive* prim = interaction->frame->getPrimitive(row.accent)) {
        if (prim->type == PrimeFrame::PrimitiveType::Rect) {
          if (selected) {
            prim->rect.overrideStyle.opacity.reset();
          } else {
            prim->rect.overrideStyle.opacity = 0.0f;
          }
        }
      }
    }
  };

  auto setHovered = [interaction, updateRowVisual](int rowIndex) {
    if (rowIndex == interaction->hoveredRow) {
      return;
    }
    int previous = interaction->hoveredRow;
    interaction->hoveredRow = rowIndex;
    if (previous >= 0) {
      updateRowVisual(previous);
    }
    if (rowIndex >= 0) {
      updateRowVisual(rowIndex);
    }
    if (interaction->callbacks.onHoverChanged) {
      interaction->callbacks.onHoverChanged(rowIndex);
    }
  };

  auto requestToggle = [interaction, makeRowInfo](int rowIndex, bool expanded) {
    if (rowIndex < 0 || rowIndex >= static_cast<int>(interaction->rows.size())) {
      return;
    }
    auto& row = interaction->rows[static_cast<size_t>(rowIndex)];
    if (!row.hasChildren) {
      return;
    }
    row.expanded = expanded;
    if (interaction->callbacks.onExpandedChanged) {
      TreeViewRowInfo info = makeRowInfo(rowIndex);
      interaction->callbacks.onExpandedChanged(info, expanded);
    }
  };

  auto applyScroll = [interaction](float offset, bool notify, bool force = false) {
    float clamped = offset;
    if (interaction->maxScroll <= 0.0f) {
      clamped = 0.0f;
    } else {
      clamped = std::clamp(offset, 0.0f, interaction->maxScroll);
    }
    if (!force && clamped == interaction->scrollOffset) {
      return;
    }
    interaction->scrollOffset = clamped;
    if (PrimeFrame::Node* viewport = interaction->frame->getNode(interaction->viewportNode)) {
      viewport->scrollY = clamped;
    }
    if (interaction->scrollThumbNode.isValid() && interaction->trackH > 0.0f) {
      float travel = std::max(0.0f, interaction->trackH - interaction->thumbH);
      float progress = (interaction->maxScroll > 0.0f) ? (clamped / interaction->maxScroll) : 0.0f;
      float thumbY = interaction->trackY + travel * progress;
      if (PrimeFrame::Node* thumbNode = interaction->frame->getNode(interaction->scrollThumbNode)) {
        thumbNode->localY = thumbY;
      }
    }
    if (notify && interaction->callbacks.onScrollChanged) {
      TreeViewScrollInfo info;
      info.offset = clamped;
      info.maxOffset = interaction->maxScroll;
      info.progress = (interaction->maxScroll > 0.0f) ? (clamped / interaction->maxScroll) : 0.0f;
      info.viewportHeight = interaction->viewportHeight;
      info.contentHeight = interaction->contentHeight;
      interaction->callbacks.onScrollChanged(info);
    }
  };

  auto ensureRowVisible = [interaction,
                           applyScroll,
                           rowHeight = normalized.rowHeight,
                           rowGap = normalized.rowGap](int rowIndex) {
    if (!interaction->scrollEnabled) {
      return;
    }
    if (rowIndex < 0 || rowIndex >= static_cast<int>(interaction->rows.size())) {
      return;
    }
    float rowPitch = std::max(1.0f, rowHeight + rowGap);
    float rowTop = rowPitch * static_cast<float>(rowIndex);
    float rowBottom = rowTop + rowHeight;
    float viewTop = interaction->scrollOffset;
    float viewBottom = viewTop + interaction->viewportHeight;
    if (rowTop < viewTop) {
      applyScroll(rowTop, true);
    } else if (rowBottom > viewBottom) {
      float next = rowBottom - interaction->viewportHeight;
      applyScroll(next, true);
    }
  };

  auto setSelected = [interaction, updateRowVisual, makeRowInfo, ensureRowVisible](int rowIndex) {
    if (rowIndex < 0 || rowIndex >= static_cast<int>(interaction->rows.size())) {
      return false;
    }
    if (interaction->selectedRow == rowIndex) {
      return false;
    }
    int previous = interaction->selectedRow;
    interaction->selectedRow = rowIndex;
    if (previous >= 0) {
      updateRowVisual(previous);
    }
    updateRowVisual(rowIndex);
    if (interaction->callbacks.onSelect) {
      TreeViewRowInfo info = makeRowInfo(rowIndex);
      interaction->callbacks.onSelect(info);
    } else if (interaction->callbacks.onSelectionChanged) {
      TreeViewRowInfo info = makeRowInfo(rowIndex);
      interaction->callbacks.onSelectionChanged(info);
    }
    ensureRowVisible(rowIndex);
    return true;
  };

  auto scrollBy = [interaction, applyScroll](float delta) {
    if (!interaction->scrollEnabled) {
      return false;
    }
    applyScroll(interaction->scrollOffset + delta, true);
    return true;
  };

  auto applyScrollHover = [interaction]() {
    bool hovered = interaction->scrollHoverCount > 0;
    bool pressed = interaction->scrollDragging;
    auto apply_override = [&](PrimeFrame::PrimitiveId primId,
                              PrimeFrame::RectStyleOverride const& base,
                              std::optional<float> hoverOpacity,
                              std::optional<float> pressedOpacity) {
      if (primId == 0) {
        return;
      }
      PrimeFrame::Primitive* prim = interaction->frame->getPrimitive(primId);
      if (!prim || prim->type != PrimeFrame::PrimitiveType::Rect) {
        return;
      }
      PrimeFrame::RectStyleOverride overrideStyle = base;
      if (pressed && pressedOpacity.has_value()) {
        overrideStyle.opacity = pressedOpacity.value();
      } else if (hovered && hoverOpacity.has_value()) {
        overrideStyle.opacity = hoverOpacity.value();
      }
      prim->rect.overrideStyle = overrideStyle;
    };
    apply_override(interaction->scrollTrackPrim,
                   interaction->scrollTrackBaseOverride,
                   interaction->scrollTrackHoverOpacity,
                   interaction->scrollTrackPressedOpacity);
    apply_override(interaction->scrollThumbPrim,
                   interaction->scrollThumbBaseOverride,
                   interaction->scrollThumbHoverOpacity,
                   interaction->scrollThumbPressedOpacity);
  };

  constexpr int KeyEnter = keyCodeInt(KeyCode::Enter);
  constexpr int KeyRight = keyCodeInt(KeyCode::Right);
  constexpr int KeyLeft = keyCodeInt(KeyCode::Left);
  constexpr int KeyDown = keyCodeInt(KeyCode::Down);
  constexpr int KeyUp = keyCodeInt(KeyCode::Up);
  constexpr int KeyHome = keyCodeInt(KeyCode::Home);
  constexpr int KeyEnd = keyCodeInt(KeyCode::End);
  constexpr int KeyPageUp = keyCodeInt(KeyCode::PageUp);
  constexpr int KeyPageDown = keyCodeInt(KeyCode::PageDown);

  for (size_t i = 0; i < rows.size(); ++i) {
    FlatTreeRow const& row = rows[i];
    PrimeFrame::RectStyleToken baseRole =
        (i % 2 == 0 ? normalized.rowAltStyle : normalized.rowStyle);
    PrimeFrame::RectStyleToken rowRole = row.selected ? normalized.selectionStyle : baseRole;

    PanelSpec rowPanel;
    rowPanel.rectStyle = rowRole;
    rowPanel.layout = PrimeFrame::LayoutType::Overlay;
    rowPanel.size.preferredHeight = normalized.rowHeight;
    rowPanel.size.preferredWidth = rowWidth;
    rowPanel.size.stretchX = 1.0f;
    rowPanel.clipChildren = false;
    rowPanel.visible = normalized.visible;
    UiNode rowNode = rowsNode.createPanel(rowPanel);
    PrimeFrame::NodeId rowId = rowNode.nodeId();
    PrimeFrame::PrimitiveId backgroundPrim = 0;
    if (PrimeFrame::Node* rowNodePtr = frame().getNode(rowId)) {
      if (!rowNodePtr->primitives.empty()) {
        backgroundPrim = rowNodePtr->primitives.front();
      }
    }

    if (normalized.showConnectors && row.depth > 0 && normalized.visible) {
      float halfThickness = normalized.connectorThickness * 0.5f;
      float rowCenterY = normalized.rowHeight * 0.5f;
      float rowTop = -normalized.rowGap * 0.5f;
      float rowBottom = normalized.rowHeight + normalized.rowGap * 0.5f;

      auto draw_trunk_segment = [&](size_t depthIndex, int ancestorIndex) {
        if (ancestorIndex < 0) {
          return;
        }
        FlatTreeRow const& ancestor = rows[static_cast<size_t>(ancestorIndex)];
        if (!ancestor.hasChildren || !ancestor.expanded) {
          return;
        }
        int first = firstChild[static_cast<size_t>(ancestorIndex)];
        int last = lastChild[static_cast<size_t>(ancestorIndex)];
        if (first < 0) {
          return;
        }
        if (static_cast<int>(i) != ancestorIndex &&
            (static_cast<int>(i) < first || static_cast<int>(i) > last)) {
          return;
        }
        float trunkX = caretBaseX + static_cast<float>(depthIndex) * normalized.indent +
                       normalized.caretSize * 0.5f;
        float segmentTop = rowTop;
        float segmentBottom = rowBottom;
        if (static_cast<int>(i) == ancestorIndex) {
          segmentTop = rowCenterY;
        }
        if (static_cast<int>(i) == last) {
          segmentBottom = rowCenterY;
        }
        if (segmentBottom > segmentTop + 0.5f) {
          add_divider_rect(frame(), rowNode.nodeId(),
                           Rect{trunkX - halfThickness,
                                segmentTop - halfThickness,
                                normalized.connectorThickness,
                                (segmentBottom - segmentTop) + normalized.connectorThickness},
                           normalized.connectorStyle);
        }
      };

      for (size_t depthIndex = 0; depthIndex < row.ancestors.size(); ++depthIndex) {
        draw_trunk_segment(depthIndex, row.ancestors[depthIndex]);
      }
      if (row.hasChildren && row.expanded) {
        draw_trunk_segment(static_cast<size_t>(row.depth), static_cast<int>(i));
      }

      int parentIndex = row.parentIndex;
      if (parentIndex >= 0) {
        float trunkX = caretBaseX + static_cast<float>(row.depth - 1) * normalized.indent +
                       normalized.caretSize * 0.5f;
        float childTrunkX = caretBaseX + static_cast<float>(row.depth) * normalized.indent +
                            normalized.caretSize * 0.5f;
        float linkStartX = trunkX - halfThickness;
        float linkEndX = childTrunkX + halfThickness;
        float linkW = linkEndX - linkStartX;
        if (linkW > 0.5f) {
          add_divider_rect(frame(), rowNode.nodeId(),
                           Rect{linkStartX,
                                rowCenterY - halfThickness,
                                linkW,
                                normalized.connectorThickness},
                           normalized.connectorStyle);
        }
      }
    }

    float indent = (row.depth > 0) ? normalized.indent * static_cast<float>(row.depth) : 0.0f;
    float glyphX = caretBaseX + indent;
    float glyphY = (normalized.rowHeight - normalized.caretSize) * 0.5f;

    PrimeFrame::PrimitiveId maskPrim = 0;
    bool hasMask = false;
    if (normalized.showCaretMasks && row.depth > 0 && normalized.visible) {
      float maskPad = normalized.caretMaskPad;
      PrimeFrame::NodeId maskId = create_rect_node(frame(),
                                                   rowId,
                                                   Rect{glyphX - maskPad,
                                                        glyphY - maskPad,
                                                        normalized.caretSize + maskPad * 2.0f,
                                                        normalized.caretSize + maskPad * 2.0f},
                                                   rowRole,
                                                   {},
                                                   false,
                                                   normalized.visible);
      if (PrimeFrame::Node* maskNode = frame().getNode(maskId)) {
        if (!maskNode->primitives.empty()) {
          maskPrim = maskNode->primitives.front();
          hasMask = true;
        }
      }
    }

    if (row.hasChildren) {
      create_rect_node(frame(),
                       rowId,
                       Rect{glyphX, glyphY, normalized.caretSize, normalized.caretSize},
                       normalized.caretBackgroundStyle,
                       {},
                       false,
                       normalized.visible);

      create_rect_node(frame(),
                       rowId,
                       Rect{glyphX + normalized.caretInset,
                            glyphY + normalized.caretSize * 0.5f - normalized.caretThickness * 0.5f,
                            normalized.caretSize - normalized.caretInset * 2.0f,
                            normalized.caretThickness},
                       normalized.caretLineStyle,
                       {},
                       false,
                       normalized.visible);
      if (!row.expanded) {
        create_rect_node(frame(),
                         rowId,
                         Rect{glyphX + normalized.caretSize * 0.5f -
                                  normalized.caretThickness * 0.5f,
                              glyphY + normalized.caretInset,
                              normalized.caretThickness,
                              normalized.caretSize - normalized.caretInset * 2.0f},
                         normalized.caretLineStyle,
                         {},
                         false,
                         normalized.visible);
      }
    } else {
      create_rect_node(frame(),
                       rowId,
                       Rect{glyphX, glyphY, normalized.caretSize, normalized.caretSize},
                       normalized.caretBackgroundStyle,
                       {},
                       false,
                       normalized.visible);

      float dot = std::max(2.0f, normalized.caretThickness);
      create_rect_node(frame(),
                       rowId,
                       Rect{glyphX + normalized.caretSize * 0.5f - dot * 0.5f,
                            glyphY + normalized.caretSize * 0.5f - dot * 0.5f,
                            dot,
                            dot},
                       normalized.caretLineStyle,
                       {},
                       false,
                       normalized.visible);
    }

    float textX = normalized.rowStartX + 20.0f + indent;
    PrimeFrame::TextStyleToken textRole =
        row.selected ? normalized.selectedTextStyle : normalized.textStyle;
    float lineHeight = row.selected ? selectedTextHeight : rowTextHeight;
    float textY = (normalized.rowHeight - lineHeight) * 0.5f;
    float labelWidth = std::max(0.0f, rowWidth - normalized.rowWidthInset - textX);
    PrimeFrame::NodeId labelId = create_text_node(frame(),
                                                  rowId,
                                                  Rect{textX, textY, labelWidth, lineHeight},
                                                  row.label,
                                                  textRole,
                                                  {},
                                                  PrimeFrame::TextAlign::Start,
                                                  PrimeFrame::WrapMode::None,
                                                  labelWidth,
                                                  normalized.visible);
    PrimeFrame::PrimitiveId labelPrim = 0;
    if (PrimeFrame::Node* labelNode = frame().getNode(labelId)) {
      if (!labelNode->primitives.empty()) {
        labelPrim = labelNode->primitives.front();
      }
    }

    PrimeFrame::PrimitiveId accentPrim = 0;
    bool hasAccent = false;
    if (normalized.selectionAccentWidth > 0.0f &&
        normalized.selectionAccentStyle != 0 &&
        normalized.visible) {
      PrimeFrame::RectStyleOverride accentOverride;
      if (!row.selected) {
        accentOverride.opacity = 0.0f;
      }
      PrimeFrame::NodeId accentId = create_rect_node(frame(),
                                                     rowId,
                                                     Rect{0.0f,
                                                          0.0f,
                                                          normalized.selectionAccentWidth,
                                                          normalized.rowHeight},
                                                     normalized.selectionAccentStyle,
                                                     accentOverride,
                                                     false,
                                                     normalized.visible);
      if (PrimeFrame::Node* accentNode = frame().getNode(accentId)) {
        if (!accentNode->primitives.empty()) {
          accentPrim = accentNode->primitives.front();
          hasAccent = true;
        }
      }
    }

    TreeViewRowVisual visual;
    visual.background = backgroundPrim;
    visual.accent = accentPrim;
    visual.mask = maskPrim;
    visual.label = labelPrim;
    visual.baseStyle = baseRole;
    visual.hoverStyle = normalized.hoverStyle;
    visual.selectionStyle = normalized.selectionStyle;
    visual.textStyle = normalized.textStyle;
    visual.selectedTextStyle = normalized.selectedTextStyle;
    visual.hasAccent = hasAccent;
    visual.hasMask = hasMask;
    visual.hasChildren = row.hasChildren;
    visual.expanded = row.expanded;
    visual.depth = row.depth;
    visual.parentIndex = row.parentIndex;
    visual.path = row.path;

    int rowIndex = static_cast<int>(interaction->rows.size());
    interaction->rows.push_back(std::move(visual));
    if (row.selected && interaction->selectedRow < 0) {
      interaction->selectedRow = rowIndex;
    }

    if (enabled) {
      PrimeFrame::Callback rowCallback;
      rowCallback.onEvent = [interaction,
                             rowIndex,
                             glyphX,
                             glyphY,
                             caretSize = normalized.caretSize,
                             updateRowVisual,
                             setHovered,
                             setSelected,
                             requestToggle,
                             makeRowInfo](PrimeFrame::Event const& event) mutable -> bool {
      auto onCaret = [&]() {
        if (rowIndex < 0 || rowIndex >= static_cast<int>(interaction->rows.size())) {
          return false;
        }
        const auto& row = interaction->rows[static_cast<size_t>(rowIndex)];
        if (!row.hasChildren) {
          return false;
        }
        return event.localX >= glyphX && event.localX <= glyphX + caretSize &&
               event.localY >= glyphY && event.localY <= glyphY + caretSize;
      };

      switch (event.type) {
        case PrimeFrame::EventType::PointerEnter:
          setHovered(rowIndex);
          return true;
        case PrimeFrame::EventType::PointerLeave:
          if (interaction->hoveredRow == rowIndex) {
            setHovered(-1);
          }
          return true;
        case PrimeFrame::EventType::PointerDown: {
          setSelected(rowIndex);
          bool toggled = false;
          if (onCaret()) {
            const auto& row = interaction->rows[static_cast<size_t>(rowIndex)];
            requestToggle(rowIndex, !row.expanded);
            toggled = true;
          }
          auto now = std::chrono::steady_clock::now();
          if (!toggled &&
              interaction->doubleClickThreshold.count() > 0.0 &&
              interaction->lastClickRow == rowIndex &&
              interaction->lastClickTime.time_since_epoch().count() != 0) {
            if (now - interaction->lastClickTime <= interaction->doubleClickThreshold) {
              const auto& row = interaction->rows[static_cast<size_t>(rowIndex)];
              if (row.hasChildren) {
                requestToggle(rowIndex, !row.expanded);
              } else if (interaction->callbacks.onActivate) {
                TreeViewRowInfo info = makeRowInfo(rowIndex);
                interaction->callbacks.onActivate(info);
              } else if (interaction->callbacks.onActivated) {
                TreeViewRowInfo info = makeRowInfo(rowIndex);
                interaction->callbacks.onActivated(info);
              }
            }
          }
          interaction->lastClickRow = rowIndex;
          interaction->lastClickTime = now;
          return true;
        }
        default:
          break;
      }
      return false;
      };
      PrimeFrame::CallbackId rowCallbackId = frame().addCallback(std::move(rowCallback));
      if (PrimeFrame::Node* rowNodePtr = frame().getNode(rowId)) {
        rowNodePtr->callbacks = rowCallbackId;
      }
    }
  }

  bool wantsKeyboard = enabled && normalized.keyboardNavigation && !interaction->rows.empty();
  bool wantsPointerScroll = enabled && interaction->scrollEnabled;
  bool wantsScrollBar = wantsPointerScroll && normalized.scrollBar.enabled;
  bool treeFocusable = enabled && (!interaction->rows.empty() || wantsKeyboard);
  if (normalized.visible) {
    PrimeFrame::Node* treeNodePtr = frame().getNode(treeNode.nodeId());
    if (treeNodePtr) {
      treeNodePtr->focusable = treeFocusable;
      treeNodePtr->hitTestVisible = enabled;
      treeNodePtr->tabIndex = treeFocusable ? normalized.tabIndex : -1;
      if (wantsKeyboard || wantsPointerScroll) {
        PrimeFrame::Callback keyCallback;
        keyCallback.onEvent = [interaction,
                               setSelected,
                               requestToggle,
                               makeRowInfo,
                               scrollBy,
                               lastChild,
                               rowHeight = normalized.rowHeight,
                               rowGap = normalized.rowGap,
                               wantsKeyboard,
                               wantsPointerScroll](PrimeFrame::Event const& event) {
        if (wantsPointerScroll && event.type == PrimeFrame::EventType::PointerScroll) {
          if (event.scrollY != 0.0f) {
            return scrollBy(event.scrollY);
          }
          return false;
        }
        if (!wantsKeyboard || event.type != PrimeFrame::EventType::KeyDown) {
          return false;
        }
        int rowCount = static_cast<int>(interaction->rows.size());
        if (rowCount <= 0) {
          return false;
        }
        switch (event.key) {
          case KeyUp:
          case KeyDown: {
            int current = interaction->selectedRow;
            if (current < 0) {
              current = (event.key == KeyDown) ? -1 : rowCount;
            }
            int delta = (event.key == KeyDown) ? 1 : -1;
            int next = std::clamp(current + delta, 0, rowCount - 1);
            if (next != current) {
              setSelected(next);
            }
            return true;
          }
          case KeyPageUp:
          case KeyPageDown: {
            int current = interaction->selectedRow;
            if (current < 0) {
              current = (event.key == KeyPageDown) ? -1 : rowCount;
            }
            float rowPitch = std::max(1.0f, rowHeight + rowGap);
            int pageStep = static_cast<int>(std::floor(interaction->viewportHeight / rowPitch));
            if (pageStep < 1) {
              pageStep = 1;
            }
            int delta = (event.key == KeyPageDown) ? pageStep : -pageStep;
            int next = std::clamp(current + delta, 0, rowCount - 1);
            if (next != current) {
              setSelected(next);
            }
            return true;
          }
          case KeyHome: {
            setSelected(0);
            return true;
          }
          case KeyEnd: {
            setSelected(rowCount - 1);
            return true;
          }
          case KeyLeft:
          case KeyRight: {
            int index = interaction->selectedRow;
            if (index >= 0 && index < rowCount) {
              const auto& row = interaction->rows[static_cast<size_t>(index)];
              if (row.hasChildren) {
                bool wasExpanded = row.expanded;
                bool wantExpanded = (event.key == KeyRight);
                if (row.expanded != wantExpanded) {
                  requestToggle(index, wantExpanded);
                }
                if (event.key == KeyLeft) {
                  if (wasExpanded) {
                    return true;
                  }
                  if (row.parentIndex >= 0) {
                    setSelected(row.parentIndex);
                  }
                } else if (event.key == KeyRight && row.expanded) {
                  int childIndex = (index >= 0 && index < static_cast<int>(lastChild.size()))
                                       ? lastChild[static_cast<size_t>(index)]
                                       : -1;
                  if (childIndex >= 0) {
                    setSelected(childIndex);
                  }
                }
              } else if (event.key == KeyLeft && row.parentIndex >= 0) {
                setSelected(row.parentIndex);
              }
            }
            return true;
          }
          case KeyEnter: {
            int index = interaction->selectedRow;
            if (index >= 0 && index < rowCount) {
              const auto& row = interaction->rows[static_cast<size_t>(index)];
              if (row.hasChildren) {
                requestToggle(index, !row.expanded);
              } else if (interaction->callbacks.onActivate) {
                TreeViewRowInfo info = makeRowInfo(index);
                interaction->callbacks.onActivate(info);
              } else if (interaction->callbacks.onActivated) {
                TreeViewRowInfo info = makeRowInfo(index);
                interaction->callbacks.onActivated(info);
              }
            }
            return true;
          }
          default:
            break;
        }
          return false;
        };
        PrimeFrame::CallbackId keyCallbackId = frame().addCallback(std::move(keyCallback));
        treeNodePtr->callbacks = keyCallbackId;
      }
    }
  }

  if (normalized.showScrollBar && wantsScrollBar && normalized.visible) {
    float trackX = bounds.width - normalized.scrollBar.inset;
    float trackY = normalized.scrollBar.padding;
    float trackH = std::max(0.0f, bounds.height - normalized.scrollBar.padding * 2.0f);
    float trackW = normalized.scrollBar.width;
    PrimeFrame::NodeId trackId = create_rect_node(frame(),
                                                  treeNode.nodeId(),
                                                  Rect{trackX, trackY, trackW, trackH},
                                                  normalized.scrollBar.trackStyle,
                                                  normalized.scrollBar.trackStyleOverride,
                                                  false,
                                                  normalized.visible);
    if (PrimeFrame::Node* trackNode = frame().getNode(trackId)) {
      trackNode->hitTestVisible = true;
      if (!trackNode->primitives.empty()) {
        interaction->scrollTrackPrim = trackNode->primitives.front();
      }
    }

    float thumbFraction = normalized.scrollBar.thumbFraction;
    if (normalized.scrollBar.autoThumb) {
      if (interaction->contentHeight > 0.0f && viewportHeight > 0.0f) {
        thumbFraction = std::clamp(viewportHeight / interaction->contentHeight, 0.0f, 1.0f);
      } else {
        thumbFraction = 1.0f;
      }
    }

    float thumbH = trackH * thumbFraction;
    thumbH = std::max(thumbH, normalized.scrollBar.minThumbHeight);
    if (thumbH > trackH) {
      thumbH = trackH;
    }
    float maxOffset = std::max(0.0f, trackH - thumbH);
    float progress = (interaction->maxScroll > 0.0f)
                         ? std::clamp(interaction->scrollOffset / interaction->maxScroll, 0.0f, 1.0f)
                         : 0.0f;
    float thumbY = trackY + maxOffset * progress;
    PrimeFrame::NodeId thumbId = create_rect_node(frame(),
                                                  treeNode.nodeId(),
                                                  Rect{trackX, thumbY, trackW, thumbH},
                                                  normalized.scrollBar.thumbStyle,
                                                  normalized.scrollBar.thumbStyleOverride,
                                                  false,
                                                  normalized.visible);
    if (PrimeFrame::Node* thumbNode = frame().getNode(thumbId)) {
      thumbNode->hitTestVisible = true;
      if (!thumbNode->primitives.empty()) {
        interaction->scrollThumbPrim = thumbNode->primitives.front();
      }
    }

    interaction->trackY = trackY;
    interaction->trackH = trackH;
    interaction->thumbH = thumbH;
    interaction->scrollThumbNode = thumbId;

    PrimeFrame::Callback trackCallback;
    trackCallback.onEvent = [interaction, applyScroll, applyScrollHover](PrimeFrame::Event const& event) -> bool {
      switch (event.type) {
        case PrimeFrame::EventType::PointerEnter:
          interaction->scrollHoverCount += 1;
          applyScrollHover();
          return true;
        case PrimeFrame::EventType::PointerLeave:
          interaction->scrollHoverCount = std::max(0, interaction->scrollHoverCount - 1);
          applyScrollHover();
          return true;
        case PrimeFrame::EventType::PointerDown: {
          if (!interaction->scrollEnabled) {
            return false;
          }
          float travel = std::max(0.0f, interaction->trackH - interaction->thumbH);
          if (travel <= 0.0f) {
            return false;
          }
          float pos = std::clamp(event.localY - interaction->thumbH * 0.5f, 0.0f, travel);
          float progress = pos / travel;
          applyScroll(progress * interaction->maxScroll, true);
          return true;
        }
        default:
          break;
      }
      return false;
    };
    PrimeFrame::CallbackId trackCallbackId = frame().addCallback(std::move(trackCallback));
    if (PrimeFrame::Node* trackNode = frame().getNode(trackId)) {
      trackNode->callbacks = trackCallbackId;
    }

    PrimeFrame::Callback thumbCallback;
    thumbCallback.onEvent = [interaction, applyScroll, applyScrollHover](PrimeFrame::Event const& event) -> bool {
      switch (event.type) {
        case PrimeFrame::EventType::PointerEnter:
          interaction->scrollHoverCount += 1;
          applyScrollHover();
          return true;
        case PrimeFrame::EventType::PointerLeave:
          interaction->scrollHoverCount = std::max(0, interaction->scrollHoverCount - 1);
          applyScrollHover();
          return true;
        case PrimeFrame::EventType::PointerDown: {
          if (!interaction->scrollEnabled) {
            return false;
          }
          interaction->scrollDragging = true;
          interaction->scrollPointerId = event.pointerId;
          interaction->scrollDragStartY = event.y;
          interaction->scrollDragStartOffset = interaction->scrollOffset;
          applyScrollHover();
          return true;
        }
        case PrimeFrame::EventType::PointerDrag:
        case PrimeFrame::EventType::PointerMove: {
          if (!interaction->scrollDragging || interaction->scrollPointerId != event.pointerId) {
            return false;
          }
          float travel = std::max(0.0f, interaction->trackH - interaction->thumbH);
          if (travel <= 0.0f) {
            return true;
          }
          float delta = event.y - interaction->scrollDragStartY;
          float next = interaction->scrollDragStartOffset + delta * (interaction->maxScroll / travel);
          applyScroll(next, true);
          return true;
        }
        case PrimeFrame::EventType::PointerUp:
        case PrimeFrame::EventType::PointerCancel: {
          if (interaction->scrollPointerId == event.pointerId) {
            interaction->scrollDragging = false;
            interaction->scrollPointerId = -1;
            applyScrollHover();
            return true;
          }
          return false;
        }
        default:
          break;
      }
      return false;
    };
    PrimeFrame::CallbackId thumbCallbackId = frame().addCallback(std::move(thumbCallback));
    if (PrimeFrame::Node* thumbNode = frame().getNode(thumbId)) {
      thumbNode->callbacks = thumbCallbackId;
    }

    applyScroll(interaction->scrollOffset, false, true);
  }

  std::optional<FocusOverlay> focusOverlay;
  if (normalized.visible && treeFocusable) {
    ResolvedFocusStyle focusStyle = resolve_focus_style(
        frame(),
        normalized.focusStyle,
        normalized.focusStyleOverride,
        {normalized.selectionAccentStyle,
         normalized.selectionStyle,
         normalized.hoverStyle,
         normalized.rowStyle,
         normalized.rowAltStyle});
    Rect overlayRect{0.0f, 0.0f, bounds.width, bounds.height};
    focusOverlay = add_focus_overlay_node(frame(),
                                          treeNode.nodeId(),
                                          overlayRect,
                                          focusStyle.token,
                                          focusStyle.overrideStyle,
                                          normalized.visible);
    if (PrimeFrame::Node* treeNodePtr = frame().getNode(treeNode.nodeId())) {
      treeNodePtr->focusable = true;
      treeNodePtr->tabIndex = normalized.tabIndex;
    }
  }
  if (focusOverlay.has_value()) {
    attach_focus_callbacks(frame(), treeNode.nodeId(), *focusOverlay);
  }

  if (!enabled) {
    add_state_scrim_overlay(frame(),
                            treeNode.nodeId(),
                            Rect{0.0f, 0.0f, bounds.width, bounds.height},
                            DisabledScrimOpacity,
                            normalized.visible);
  }

  return UiNode(frame(), treeNode.nodeId(), allowAbsolute_);
}

UiNode UiNode::createTreeView(std::vector<TreeNode> nodes, SizeSpec const& size) {
  TreeViewSpec spec;
  spec.nodes = std::move(nodes);
  spec.size = size;
  return createTreeView(spec);
}

} // namespace PrimeStage
