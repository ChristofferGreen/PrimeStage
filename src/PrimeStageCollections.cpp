#include "PrimeStage/PrimeStage.h"

#include "PrimeStageCollectionInternals.h"

namespace PrimeStage {

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

UiNode UiNode::createTreeView(std::vector<TreeNode> nodes, SizeSpec const& size) {
  TreeViewSpec spec;
  spec.nodes = std::move(nodes);
  spec.size = size;
  return createTreeView(spec);
}

} // namespace PrimeStage
