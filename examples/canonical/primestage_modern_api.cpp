#include "PrimeStage/App.h"
#include "PrimeStage/PrimeStage.h"

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct TaskRow {
  std::string task;
  std::string status;
};

struct AssetNode {
  std::string label;
  std::vector<AssetNode> children;
  bool expanded = true;
  bool selected = false;
};

struct DemoState {
  PrimeStage::TextFieldState search{};
  PrimeStage::State<bool> notifications{true};
  PrimeStage::State<int> tabIndex{0};
  PrimeStage::State<float> progress{0.64f};
  std::vector<std::string> recentItems = {
      "Alpha", "Beta", "Gamma", "Delta",
  };
  std::vector<TaskRow> tasks = {
      {"Load icons", "Done"},
      {"Compile shaders", "Running"},
      {"Upload bundle", "Queued"},
  };
  std::vector<AssetNode> tree = {
      {"Assets", {{"Textures"}, {"Audio"}}, true, false},
      {"Scripts", {{"main.cpp"}, {"widgets.cpp"}}, true, false},
  };
};

void buildUi(PrimeStage::UiNode root, DemoState& state) {
  PrimeStage::StackSpec page;
  page.size.stretchX = 1.0f;
  page.size.stretchY = 1.0f;
  page.padding.left = 14.0f;
  page.padding.top = 12.0f;
  page.padding.right = 14.0f;
  page.padding.bottom = 12.0f;
  page.gap = 10.0f;

  root.column(page, [&](PrimeStage::UiNode& screen) {
    screen.label("PrimeStage Modern API");
    screen.paragraph("Strict high-level API usage with no low-level escape hatches.", 700.0f);

    screen.row([&](PrimeStage::UiNode& actions) {
      actions.button("Build");
      actions.createToggle(PrimeStage::bind(state.notifications));
      actions.createTabs({"Overview", "Assets", "Settings"}, PrimeStage::bind(state.tabIndex));
    });

    PrimeStage::TextFieldSpec search;
    search.state = &state.search;
    search.placeholder = "Search assets";
    screen.createTextField(search);

    PrimeStage::ListSpec list;
    PrimeStage::ListModelAdapter listModel = PrimeStage::makeListModel(
        state.recentItems,
        [](std::string const& item) -> std::string_view { return item; },
        [](std::string const& item) -> PrimeStage::WidgetIdentityId {
          return PrimeStage::widgetIdentityId(item);
        });
    listModel.bind(list);
    screen.createList(list);

    PrimeStage::TableSpec table;
    table.columns = {{"Task"}, {"Status"}};
    PrimeStage::TableModelAdapter tableModel = PrimeStage::makeTableModel(
        state.tasks,
        table.columns.size(),
        [](TaskRow const& row, size_t columnIndex) -> std::string_view {
          if (columnIndex == 0u) {
            return row.task;
          }
          return row.status;
        },
        [](TaskRow const& row) -> PrimeStage::WidgetIdentityId {
          return PrimeStage::widgetIdentityId(row.task);
        });
    tableModel.bindRows(table);
    screen.createTable(table);

    PrimeStage::TreeViewSpec tree;
    PrimeStage::TreeModelAdapter treeModel = PrimeStage::makeTreeModel(
        state.tree,
        [](AssetNode const& node) -> std::string_view { return node.label; },
        [](AssetNode const& node) -> std::vector<AssetNode> const& { return node.children; },
        [](AssetNode const& node) { return node.expanded; },
        [](AssetNode const& node) { return node.selected; },
        [](AssetNode const& node) -> PrimeStage::WidgetIdentityId {
          return PrimeStage::widgetIdentityId(node.label);
        });
    treeModel.bind(tree);
    screen.createTreeView(tree);

    PrimeStage::ProgressBarSpec progress;
    progress.binding = PrimeStage::bind(state.progress);
    screen.createProgressBar(progress);
  });
}

} // namespace

int main(int argc, char** argv) {
  std::string outputPath = argc > 1 ? std::string(argv[1]) : "primestage_modern_api.png";

  PrimeStage::App app;
  DemoState state;
  state.search.text = "PrimeStage";
  state.search.cursor = static_cast<uint32_t>(state.search.text.size());

  app.setSurfaceMetrics(1024u, 640u, 1.0f);
  app.setRenderMetrics(1024u, 640u, 1.0f);
  (void)app.runRebuildIfNeeded([&](PrimeStage::UiNode root) { buildUi(root, state); });

  PrimeStage::RenderStatus status = app.renderToPng(outputPath);
  if (!status.ok()) {
    std::cerr << "Failed to render modern API example: "
              << PrimeStage::renderStatusMessage(status.code);
    if (!status.detail.empty()) {
      std::cerr << " (" << status.detail << ")";
    }
    std::cerr << "\n";
    return 1;
  }

  std::cout << "Wrote modern API snapshot to " << outputPath << "\n";
  return 0;
}
