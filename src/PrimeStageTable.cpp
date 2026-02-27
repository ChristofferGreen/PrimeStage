#include "PrimeStage/PrimeStage.h"

#include "PrimeStageCollectionInternals.h"
#include "PrimeFrame/Events.h"

#include <algorithm>
#include <memory>
#include <utility>

namespace PrimeStage {

constexpr int TableKeyEnter = keyCodeInt(KeyCode::Enter);
constexpr int TableKeyDown = keyCodeInt(KeyCode::Down);
constexpr int TableKeyUp = keyCodeInt(KeyCode::Up);
constexpr int TableKeyHome = keyCodeInt(KeyCode::Home);
constexpr int TableKeyEnd = keyCodeInt(KeyCode::End);

UiNode UiNode::createTable(TableSpec const& specInput) {
  TableSpec spec = Internal::normalizeTableSpec(specInput);
  bool enabled = spec.enabled;
  Internal::WidgetRuntimeContext runtime = Internal::makeWidgetRuntimeContext(frame(),
                                                                              nodeId(),
                                                                              allowAbsolute(),
                                                                              enabled,
                                                                              spec.visible,
                                                                              spec.tabIndex);
  PrimeFrame::Frame& runtimeFrame = Internal::runtimeFrame(runtime);

  Internal::InternalRect tableBounds = Internal::resolveRect(spec.size);
  size_t rowCount = spec.rows.size();
  float rowsHeight = 0.0f;
  if (rowCount > 0) {
    rowsHeight = static_cast<float>(rowCount) * spec.rowHeight +
                 static_cast<float>(rowCount - 1) * spec.rowGap;
  }
  float headerBlock = spec.headerHeight > 0.0f ? spec.headerInset + spec.headerHeight : 0.0f;
  if (tableBounds.height <= 0.0f && !spec.size.preferredHeight.has_value() && spec.size.stretchY <= 0.0f) {
    tableBounds.height = headerBlock + rowsHeight;
  }
  if (tableBounds.width <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f &&
      !spec.columns.empty()) {
    float inferredWidth = 0.0f;
    float paddingX = std::max(spec.headerPaddingX, spec.cellPaddingX);
    for (size_t colIndex = 0; colIndex < spec.columns.size(); ++colIndex) {
      TableColumn const& col = spec.columns[colIndex];
      if (col.width > 0.0f) {
        inferredWidth += col.width;
        continue;
      }
      float maxText = Internal::estimateTextWidth(frame(), col.headerStyle, col.label);
      for (auto const& row : spec.rows) {
        if (colIndex < row.size()) {
          std::string_view cell = row[colIndex];
          float cellWidth = Internal::estimateTextWidth(frame(), col.cellStyle, cell);
          if (cellWidth > maxText) {
            maxText = cellWidth;
          }
        }
      }
      inferredWidth += maxText + paddingX;
    }
    tableBounds.width = inferredWidth;
  }
  if (tableBounds.width <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f) {
    tableBounds.width = Internal::defaultCollectionWidth();
  }
  if (tableBounds.height <= 0.0f &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchY <= 0.0f) {
    tableBounds.height = Internal::defaultCollectionHeight();
  }

  SizeSpec tableSize = spec.size;
  if (!tableSize.preferredWidth.has_value() && tableBounds.width > 0.0f) {
    tableSize.preferredWidth = tableBounds.width;
  }
  if (!tableSize.preferredHeight.has_value() && tableBounds.height > 0.0f) {
    tableSize.preferredHeight = tableBounds.height;
  }

  StackSpec tableRootSpec;
  tableRootSpec.size = tableSize;
  tableRootSpec.gap = 0.0f;
  tableRootSpec.clipChildren = spec.clipChildren;
  tableRootSpec.visible = spec.visible;
  UiNode parentNode = Internal::makeParentNode(runtime);
  UiNode tableRoot = parentNode.createOverlay(tableRootSpec);
  Internal::configureInteractiveRoot(runtime, tableRoot.nodeId());

  StackSpec tableSpec;
  tableSpec.size = tableSize;
  tableSpec.gap = 0.0f;
  tableSpec.clipChildren = spec.clipChildren;
  tableSpec.visible = spec.visible;
  UiNode tableNode = tableRoot.createVerticalStack(tableSpec);

  float tableWidth = tableBounds.width > 0.0f ? tableBounds.width
                                              : tableSize.preferredWidth.value_or(0.0f);
  float dividerWidth = spec.showColumnDividers ? 1.0f : 0.0f;
  size_t dividerCount = spec.columns.size() > 1 ? spec.columns.size() - 1 : 0;
  float dividerTotal = dividerWidth * static_cast<float>(dividerCount);

  auto compute_auto_width = [&](size_t colIndex, TableColumn const& col) {
    float paddingX = std::max(spec.headerPaddingX, spec.cellPaddingX);
    float maxText = Internal::estimateTextWidth(frame(), col.headerStyle, col.label);
    for (auto const& row : spec.rows) {
      if (colIndex < row.size()) {
        std::string_view cell = row[colIndex];
        float cellWidth = Internal::estimateTextWidth(frame(), col.cellStyle, cell);
        if (cellWidth > maxText) {
          maxText = cellWidth;
        }
      }
    }
    return maxText + paddingX;
  };

  std::vector<float> columnWidths;
  columnWidths.reserve(spec.columns.size());
  float fixedWidth = 0.0f;
  size_t autoCount = 0;
  for (size_t colIndex = 0; colIndex < spec.columns.size(); ++colIndex) {
    TableColumn const& col = spec.columns[colIndex];
    if (col.width > 0.0f) {
      columnWidths.push_back(col.width);
      fixedWidth += col.width;
    } else {
      columnWidths.push_back(0.0f);
      ++autoCount;
    }
  }
  float availableWidth = std::max(0.0f, tableWidth - dividerTotal);
  if (autoCount > 0 && availableWidth > fixedWidth) {
    float remaining = availableWidth - fixedWidth;
    float autoWidth = remaining / static_cast<float>(autoCount);
    for (float& width : columnWidths) {
      if (width == 0.0f) {
        width = autoWidth;
      }
    }
  }
  if (autoCount > 0 && (availableWidth <= fixedWidth || tableWidth <= 0.0f)) {
    for (size_t colIndex = 0; colIndex < columnWidths.size(); ++colIndex) {
      if (columnWidths[colIndex] <= 0.0f) {
        columnWidths[colIndex] = compute_auto_width(colIndex, spec.columns[colIndex]);
      }
    }
  }
  if (autoCount == 0 && availableWidth > 0.0f && fixedWidth > availableWidth && !columnWidths.empty()) {
    float overflow = fixedWidth - availableWidth;
    columnWidths.back() = std::max(0.0f, columnWidths.back() - overflow);
  }

  auto create_cell = [&](UiNode const& rowNode,
                         float width,
                         float height,
                         float paddingX,
                         std::string_view text,
                         PrimeFrame::TextStyleToken role) {
    SizeSpec cellSize;
    if (width > 0.0f) {
      cellSize.preferredWidth = width;
    }
    if (height > 0.0f) {
      cellSize.preferredHeight = height;
    }
    PrimeFrame::Insets padding{};
    padding.left = paddingX;
    padding.right = paddingX;
    PrimeFrame::NodeId cellId = Internal::createNode(frame(),
                                                     rowNode.nodeId(),
                                                     Internal::InternalRect{},
                                                     &cellSize,
                                                     PrimeFrame::LayoutType::Overlay,
                                                     padding,
                                                     0.0f,
                                                     false,
                                                     spec.visible);
    UiNode cell(frame(), cellId, rowNode.allowAbsolute());
    SizeSpec textSize;
    textSize.stretchX = 1.0f;
    if (height > 0.0f) {
      textSize.preferredHeight = height;
    }
    TextLineSpec textSpec;
    textSpec.text = text;
    textSpec.textStyle = role;
    textSpec.size = textSize;
    textSpec.visible = spec.visible;
    cell.createTextLine(textSpec);
  };

  if (spec.showHeaderDividers) {
    DividerSpec divider;
    divider.rectStyle = spec.dividerStyle;
    divider.visible = spec.visible;
    divider.size.stretchX = 1.0f;
    divider.size.preferredHeight = 1.0f;
    tableNode.createDivider(divider);
  }

  if (spec.headerInset > 0.0f) {
    SizeSpec headerInset;
    headerInset.preferredHeight = spec.headerInset;
    tableNode.createSpacer(headerInset);
  }

  if (spec.headerHeight > 0.0f && !spec.columns.empty()) {
    PanelSpec headerPanel;
    headerPanel.rectStyle = spec.headerStyle;
    headerPanel.layout = PrimeFrame::LayoutType::HorizontalStack;
    headerPanel.size.preferredHeight = spec.headerHeight;
    headerPanel.size.stretchX = 1.0f;
    headerPanel.visible = spec.visible;
    UiNode headerRow = tableNode.createPanel(headerPanel);

    for (size_t colIndex = 0; colIndex < spec.columns.size(); ++colIndex) {
      TableColumn const& col = spec.columns[colIndex];
      float colWidth = colIndex < columnWidths.size() ? columnWidths[colIndex] : 0.0f;
      create_cell(headerRow,
                  colWidth,
                  spec.headerHeight,
                  spec.headerPaddingX,
                  col.label,
                  col.headerStyle);
      if (spec.showColumnDividers && colIndex + 1 < spec.columns.size()) {
        DividerSpec divider;
        divider.rectStyle = spec.dividerStyle;
        divider.visible = spec.visible;
        divider.size.preferredWidth = dividerWidth;
        divider.size.preferredHeight = spec.headerHeight;
        headerRow.createDivider(divider);
      }
    }
  }

  if (spec.showHeaderDividers) {
    DividerSpec divider;
    divider.rectStyle = spec.dividerStyle;
    divider.visible = spec.visible;
    divider.size.stretchX = 1.0f;
    divider.size.preferredHeight = 1.0f;
    tableNode.createDivider(divider);
  }

  StackSpec rowsSpec;
  rowsSpec.size.stretchX = 1.0f;
  rowsSpec.size.stretchY = spec.size.stretchY;
  rowsSpec.gap = spec.rowGap;
  rowsSpec.clipChildren = spec.clipChildren;
  rowsSpec.visible = spec.visible;
  UiNode rowsNode = tableNode.createVerticalStack(rowsSpec);
  if (PrimeFrame::Node* rowsNodePtr = runtimeFrame.getNode(rowsNode.nodeId())) {
    rowsNodePtr->hitTestVisible = enabled;
  }

  struct TableInteractionState {
    PrimeFrame::Frame* frame = nullptr;
    std::vector<PrimeFrame::PrimitiveId> backgrounds;
    std::vector<PrimeFrame::RectStyleToken> baseStyles;
    PrimeFrame::RectStyleToken selectionStyle = 0;
    TableCallbacks callbacks{};
    std::vector<std::vector<std::string>> ownedRows;
    std::vector<std::string_view> rowViewScratch;
    int selectedRow = -1;
    float rowHeight = 0.0f;
    float rowGap = 0.0f;
  };

  auto interaction = std::make_shared<TableInteractionState>();
  interaction->frame = &runtimeFrame;
  interaction->selectionStyle = spec.selectionStyle;
  interaction->callbacks = spec.callbacks;
  interaction->ownedRows.reserve(spec.rows.size());
  for (auto const& sourceRow : spec.rows) {
    std::vector<std::string> ownedRow;
    ownedRow.reserve(sourceRow.size());
    for (std::string_view cell : sourceRow) {
      ownedRow.emplace_back(cell);
    }
    interaction->ownedRows.push_back(std::move(ownedRow));
  }
  interaction->selectedRow = spec.selectedRow;
  interaction->rowHeight = spec.rowHeight;
  interaction->rowGap = spec.rowGap;
  interaction->backgrounds.reserve(rowCount);
  interaction->baseStyles.reserve(rowCount);
  std::vector<PrimeFrame::NodeId> rowNodeIds;
  rowNodeIds.reserve(rowCount);

  for (size_t rowIndex = 0; rowIndex < rowCount; ++rowIndex) {
    PrimeFrame::RectStyleToken rowRole = (rowIndex % 2 == 0) ? spec.rowAltStyle : spec.rowStyle;
    if (spec.selectionStyle != 0 && static_cast<int>(rowIndex) == spec.selectedRow) {
      rowRole = spec.selectionStyle;
    }
    PanelSpec rowPanel;
    rowPanel.rectStyle = rowRole;
    rowPanel.layout = PrimeFrame::LayoutType::HorizontalStack;
    rowPanel.size.preferredHeight = spec.rowHeight;
    rowPanel.size.stretchX = 1.0f;
    rowPanel.visible = spec.visible;
    UiNode rowNode = rowsNode.createPanel(rowPanel);
    rowNodeIds.push_back(rowNode.nodeId());
    if (PrimeFrame::Node* rowNodePtr = runtimeFrame.getNode(rowNode.nodeId())) {
      if (!rowNodePtr->primitives.empty()) {
        interaction->backgrounds.push_back(rowNodePtr->primitives.front());
      } else {
        interaction->backgrounds.push_back(0);
      }
    } else {
      interaction->backgrounds.push_back(0);
    }
    interaction->baseStyles.push_back((rowIndex % 2 == 0) ? spec.rowAltStyle : spec.rowStyle);

    for (size_t colIndex = 0; colIndex < spec.columns.size(); ++colIndex) {
      TableColumn const& col = spec.columns[colIndex];
      float colWidth = colIndex < columnWidths.size() ? columnWidths[colIndex] : 0.0f;
      std::string_view cellText;
      if (rowIndex < spec.rows.size() && colIndex < spec.rows[rowIndex].size()) {
        cellText = spec.rows[rowIndex][colIndex];
      }
      create_cell(rowNode,
                  colWidth,
                  spec.rowHeight,
                  spec.cellPaddingX,
                  cellText,
                  col.cellStyle);
      if (spec.showColumnDividers && colIndex + 1 < spec.columns.size()) {
        DividerSpec divider;
        divider.rectStyle = spec.dividerStyle;
        divider.visible = spec.visible;
        divider.size.preferredWidth = dividerWidth;
        divider.size.preferredHeight = spec.rowHeight;
        rowNode.createDivider(divider);
      }
    }
  }

  bool hasRowSelectCallback = static_cast<bool>(interaction->callbacks.onSelect) ||
                              static_cast<bool>(interaction->callbacks.onRowClicked);
  if (enabled && spec.visible && (hasRowSelectCallback || spec.selectionStyle != 0)) {
    auto updateRowStyle = [interaction](int rowIndex, bool selected) {
      if (rowIndex < 0 || rowIndex >= static_cast<int>(interaction->backgrounds.size())) {
        return;
      }
      PrimeFrame::PrimitiveId primId = interaction->backgrounds[static_cast<size_t>(rowIndex)];
      if (primId == 0) {
        return;
      }
      PrimeFrame::Primitive* prim = interaction->frame->getPrimitive(primId);
      if (!prim || prim->type != PrimeFrame::PrimitiveType::Rect) {
        return;
      }
      if (selected && interaction->selectionStyle != 0) {
        prim->rect.token = interaction->selectionStyle;
      } else if (rowIndex >= 0 && rowIndex < static_cast<int>(interaction->baseStyles.size())) {
        prim->rect.token = interaction->baseStyles[static_cast<size_t>(rowIndex)];
      }
    };

    auto notifyRowSelect = [interaction](int index) {
      if (!interaction->callbacks.onSelect && !interaction->callbacks.onRowClicked) {
        return;
      }
      TableRowInfo info;
      info.rowIndex = index;
      if (index >= 0 && index < static_cast<int>(interaction->ownedRows.size())) {
        auto const& row = interaction->ownedRows[static_cast<size_t>(index)];
        interaction->rowViewScratch.clear();
        interaction->rowViewScratch.reserve(row.size());
        for (std::string const& cell : row) {
          interaction->rowViewScratch.push_back(cell);
        }
        info.row = std::span<const std::string_view>(interaction->rowViewScratch);
      }
      if (interaction->callbacks.onSelect) {
        interaction->callbacks.onSelect(info);
      } else if (interaction->callbacks.onRowClicked) {
        interaction->callbacks.onRowClicked(info);
      }
    };

    auto selectRow = [interaction, updateRowStyle, notifyRowSelect](int index,
                                                                     bool notifyWhenUnchanged) {
      if (index < 0 || index >= static_cast<int>(interaction->backgrounds.size())) {
        return false;
      }
      if (interaction->selectedRow != index) {
        int previous = interaction->selectedRow;
        interaction->selectedRow = index;
        updateRowStyle(previous, false);
        updateRowStyle(index, true);
        notifyRowSelect(index);
        return true;
      }
      if (notifyWhenUnchanged) {
        notifyRowSelect(index);
        return true;
      }
      return false;
    };

    for (size_t rowIndex = 0; rowIndex < rowNodeIds.size(); ++rowIndex) {
      PrimeFrame::Callback rowCallback;
      rowCallback.onEvent = [selectRow, rowIndex](PrimeFrame::Event const& event) -> bool {
        if (event.type != PrimeFrame::EventType::PointerDown) {
          return false;
        }
        return selectRow(static_cast<int>(rowIndex), true);
      };
      if (PrimeFrame::Node* rowNodePtr = runtimeFrame.getNode(rowNodeIds[rowIndex])) {
        rowNodePtr->callbacks = runtimeFrame.addCallback(std::move(rowCallback));
      }
    }

    (void)Internal::appendNodeOnEvent(runtime,
                                      tableRoot.nodeId(),
                                      [interaction, selectRow](PrimeFrame::Event const& event) {
                              if (event.type != PrimeFrame::EventType::KeyDown) {
                                return false;
                              }
                              if (interaction->backgrounds.empty()) {
                                return false;
                              }
                              int lastIndex = static_cast<int>(interaction->backgrounds.size()) - 1;
                              int current = interaction->selectedRow;
                              if (current < 0) {
                                current = 0;
                              }

                              int target = current;
                              if (event.key == TableKeyUp) {
                                target = std::max(0, current - 1);
                              } else if (event.key == TableKeyDown) {
                                target = std::min(lastIndex, current + 1);
                              } else if (event.key == TableKeyHome) {
                                target = 0;
                              } else if (event.key == TableKeyEnd) {
                                target = lastIndex;
                              } else if (event.key == TableKeyEnter) {
                                return selectRow(current, true);
                              } else {
                                return false;
                              }
                              return selectRow(target, false) || target == current;
                            });
  }

  if (spec.visible && enabled) {
    Internal::InternalFocusStyle focusStyle = Internal::resolveFocusStyle(
        frame(),
        spec.focusStyle,
        spec.focusStyleOverride,
        spec.selectionStyle,
        spec.rowStyle,
        spec.rowAltStyle,
        spec.headerStyle,
        spec.dividerStyle);
    float focusWidth = tableBounds.width > 0.0f ? tableBounds.width : tableSize.preferredWidth.value_or(0.0f);
    float focusHeight = tableBounds.height > 0.0f ? tableBounds.height : tableSize.preferredHeight.value_or(0.0f);
    Internal::attachFocusOverlay(runtime,
                                 tableRoot.nodeId(),
                                 Internal::InternalRect{0.0f,
                                                        0.0f,
                                                        std::max(0.0f, focusWidth),
                                                        std::max(0.0f, focusHeight)},
                                 focusStyle);
  }

  if (!enabled) {
    Internal::addDisabledScrimOverlay(runtime,
                                      tableRoot.nodeId(),
                                      Internal::InternalRect{
                                          0.0f, 0.0f, tableBounds.width, tableBounds.height});
  }

  return UiNode(runtimeFrame, tableRoot.nodeId(), runtime.allowAbsolute);
}

UiNode UiNode::createTable(std::vector<TableColumn> columns,
                           std::vector<std::vector<std::string_view>> rows,
                           int selectedRow,
                           SizeSpec const& size) {
  TableSpec spec;
  spec.columns = std::move(columns);
  spec.rows = std::move(rows);
  spec.selectedRow = selectedRow;
  spec.size = size;
  return createTable(spec);
}

} // namespace PrimeStage
