#include "PrimeHost/PrimeHost.h"
#include "PrimeStage/App.h"
#include "PrimeStage/PrimeStage.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr float ScrollLinePixels = 32.0f;

struct DemoState {
  PrimeStage::TextFieldState textField{};
  PrimeStage::SelectableTextState selectableText{};
  PrimeStage::State<bool> toggle{};
  PrimeStage::State<bool> checkbox{};
  PrimeStage::State<int> tabs{};
  PrimeStage::State<int> dropdown{};
  PrimeStage::State<float> sliderValue{};
  PrimeStage::State<float> progressValue{};
  int clickCount = 0;
  int listSelectedIndex = 1;
  int tableSelectedRow = -1;
  std::vector<PrimeStage::TreeNode> tree;
  std::string selectableTextContent;
  std::vector<std::string> dropdownItems;
  std::vector<std::string> tabLabels;
  std::vector<std::string> listItems;
};

struct DemoApp {
  PrimeHost::Host* host = nullptr;
  PrimeHost::SurfaceId surfaceId{};
  PrimeStage::App ui;
  DemoState state;
};

PrimeStage::UiNode createSection(PrimeStage::UiNode parent, std::string_view title) {
  PrimeStage::StackSpec sectionSpec;
  sectionSpec.size.stretchX = 1.0f;
  sectionSpec.gap = 8.0f;
  return parent.column(sectionSpec, [&](PrimeStage::UiNode& section) {
    section.label(title);
    section.divider();
  });
}

void clearTreeSelection(std::vector<PrimeStage::TreeNode>& nodes) {
  for (auto& node : nodes) {
    node.selected = false;
    clearTreeSelection(node.children);
  }
}

PrimeStage::TreeNode* findTreeNode(std::vector<PrimeStage::TreeNode>& nodes,
                                   std::span<const uint32_t> path) {
  std::vector<PrimeStage::TreeNode>* current = &nodes;
  PrimeStage::TreeNode* node = nullptr;
  for (uint32_t index : path) {
    if (index >= current->size()) {
      return nullptr;
    }
    node = &(*current)[index];
    current = &node->children;
  }
  return node;
}

void initializeState(DemoState& state) {
  state.textField.text = "Editable text field";
  state.toggle.value = true;
  state.checkbox.value = false;
  state.sliderValue.value = 0.35f;
  state.progressValue.value = state.sliderValue.value;
  state.tabs.value = 0;
  state.dropdown.value = 0;
  state.selectableTextContent =
      "Selectable text supports drag selection, keyboard movement, and clipboard shortcuts.";

  state.dropdownItems = {"Preview", "Edit", "Export", "Publish"};
  state.tabLabels = {"Overview", "Assets", "Settings"};
  state.listItems = {"Alpha", "Beta", "Gamma", "Delta"};

  state.tree = {
      {"Assets",
       {
           {"Textures", {{"ui.png"}, {"icons.png"}}, true, false},
           {"Audio", {{"theme.ogg"}, {"click.wav"}}, true, false},
       },
       true,
       false},
      {"Scripts",
       {
           {"main.cpp"},
           {"ui.cpp"},
           {"widgets.cpp"},
       },
       true,
       false},
      {"Shaders",
       {
           {"ui.vert"},
           {"ui.frag"},
           {"post.fx"},
       },
       false,
       false},
  };
}

void rebuildUi(PrimeStage::UiNode root, DemoApp& app) {
  PrimeStage::StackSpec pageSpec;
  pageSpec.size.stretchX = 1.0f;
  pageSpec.size.stretchY = 1.0f;
  pageSpec.padding.left = 14.0f;
  pageSpec.padding.top = 12.0f;
  pageSpec.padding.right = 14.0f;
  pageSpec.padding.bottom = 12.0f;
  pageSpec.gap = 10.0f;
  PrimeStage::UiNode page = root.column(pageSpec);
  page.label("PrimeStage Widgets");
  page.paragraph("A compact gallery showing each widget with mostly default API usage.", 780.0f);

  PrimeStage::StackSpec columnsSpec;
  columnsSpec.size.stretchX = 1.0f;
  columnsSpec.size.stretchY = 1.0f;
  columnsSpec.gap = 14.0f;
  PrimeStage::UiNode columns = page.row(columnsSpec);

  PrimeStage::StackSpec columnSpec;
  columnSpec.size.stretchX = 1.0f;
  columnSpec.size.maxWidth = 460.0f;
  columnSpec.gap = 10.0f;
  PrimeStage::UiNode leftColumn = columns.column(columnSpec);
  PrimeStage::UiNode rightColumn = columns.column(columnSpec);

  PrimeStage::UiNode basic = createSection(leftColumn, "Basic");
  {
    basic.textLine("TextLine");
    basic.label("Label widget");
    basic.paragraph(
        "Paragraph widget wraps text naturally based on width constraints provided by layout.");

    PrimeStage::PanelSpec panel;
    panel.layout = PrimeFrame::LayoutType::VerticalStack;
    panel.padding.left = 6.0f;
    panel.padding.top = 6.0f;
    basic.panel(panel, [](PrimeStage::UiNode& panelNode) { panelNode.label("Panel widget"); });
    basic.divider();
    basic.spacer(4.0f);
  }

  PrimeStage::UiNode actions = createSection(leftColumn, "Buttons, Toggle, Checkbox");
  {
    PrimeStage::StackSpec rowSpec;
    rowSpec.gap = 12.0f;
    PrimeStage::UiNode row = actions.row(rowSpec);
    row.button("Button", [&app]() {
      app.state.clickCount += 1;
      app.ui.lifecycle().requestRebuild();
    });

    PrimeStage::ToggleSpec toggle;
    toggle.binding = PrimeStage::bind(app.state.toggle);
    row.createToggle(toggle);

    PrimeStage::CheckboxSpec checkbox;
    checkbox.binding = PrimeStage::bind(app.state.checkbox);
    checkbox.label = "Checkbox";
    row.createCheckbox(checkbox);

    std::string summaryText = "Clicks: " + std::to_string(app.state.clickCount);
    actions.textLine(summaryText);
  }

  PrimeStage::UiNode textInput = createSection(leftColumn, "Text Field + Selectable Text");
  {
    PrimeStage::TextFieldSpec field;
    field.state = &app.state.textField;
    field.placeholder = "Type here";
    field.callbacks.onStateChanged = [&app]() {
      app.ui.lifecycle().requestFrame();
    };
    app.ui.applyPlatformServices(field);
    textInput.createTextField(field);

    PrimeStage::SelectableTextSpec selectable;
    selectable.state = &app.state.selectableText;
    selectable.text = app.state.selectableTextContent;
    selectable.callbacks.onStateChanged = [&app]() {
      app.ui.lifecycle().requestFrame();
    };
    app.ui.applyPlatformServices(selectable);
    textInput.createSelectableText(selectable);
  }

  PrimeStage::UiNode range = createSection(rightColumn, "Slider + Progress");
  {
    PrimeStage::SliderSpec slider;
    slider.binding = PrimeStage::bind(app.state.sliderValue);
    range.createSlider(slider);

    PrimeStage::ProgressBarSpec progress;
    progress.binding = PrimeStage::bind(app.state.progressValue);
    range.createProgressBar(progress);
  }

  PrimeStage::UiNode choice = createSection(rightColumn, "Tabs, Dropdown, List");
  {
    std::vector<std::string_view> tabViews;
    tabViews.reserve(app.state.tabLabels.size());
    for (const std::string& label : app.state.tabLabels) {
      tabViews.push_back(label);
    }

    PrimeStage::TabsSpec tabs;
    tabs.binding = PrimeStage::bind(app.state.tabs);
    tabs.labels = tabViews;
    choice.createTabs(tabs);

    std::vector<std::string_view> dropdownViews;
    dropdownViews.reserve(app.state.dropdownItems.size());
    for (const std::string& item : app.state.dropdownItems) {
      dropdownViews.push_back(item);
    }

    PrimeStage::DropdownSpec dropdown;
    dropdown.binding = PrimeStage::bind(app.state.dropdown);
    dropdown.options = dropdownViews;
    choice.createDropdown(dropdown);

    std::vector<std::string_view> listViews;
    listViews.reserve(app.state.listItems.size());
    for (const std::string& item : app.state.listItems) {
      listViews.push_back(item);
    }

    PrimeStage::ListSpec list;
    list.items = listViews;
    list.selectedIndex = app.state.listSelectedIndex;
    list.callbacks.onSelect = [&app](PrimeStage::ListRowInfo const& info) {
      app.state.listSelectedIndex = info.rowIndex;
      app.ui.lifecycle().requestRebuild();
    };
    choice.createList(list);
  }

  PrimeStage::UiNode data = createSection(rightColumn, "Table + Tree View");
  {
    PrimeStage::TableSpec table;
    table.selectedRow = app.state.tableSelectedRow;
    table.columns = {
        {"Asset"},
        {"Type"},
        {"Size"},
    };
    table.rows = {
        {"icons.png", "Texture", "512 KB"},
        {"theme.ogg", "Audio", "3.1 MB"},
        {"ui.vert", "Shader", "14 KB"},
    };
    table.callbacks.onSelect = [&app](PrimeStage::TableRowInfo const& info) {
      app.state.tableSelectedRow = info.rowIndex;
      app.ui.lifecycle().requestRebuild();
    };
    data.createTable(table);

    PrimeStage::TreeViewSpec tree;
    tree.nodes = app.state.tree;
    tree.callbacks.onSelect = [&app](PrimeStage::TreeViewRowInfo const& info) {
      clearTreeSelection(app.state.tree);
      if (PrimeStage::TreeNode* node = findTreeNode(app.state.tree, info.path)) {
        node->selected = true;
      }
      app.ui.lifecycle().requestRebuild();
    };
    tree.callbacks.onExpandedChanged = [&app](PrimeStage::TreeViewRowInfo const& info, bool expanded) {
      if (PrimeStage::TreeNode* node = findTreeNode(app.state.tree, info.path)) {
        node->expanded = expanded;
      }
      app.ui.lifecycle().requestRebuild();
    };
    data.createTreeView(tree);
  }

  PrimeStage::UiNode containers = createSection(rightColumn, "Scroll View + Window");
  {
    PrimeStage::ScrollViewSpec scrollSpec;
    PrimeStage::ScrollView scrollView = containers.createScrollView(scrollSpec);

    PrimeStage::PanelSpec contentPanel;
    contentPanel.size.preferredWidth = 520.0f;
    contentPanel.size.preferredHeight = 200.0f;
    contentPanel.layout = PrimeFrame::LayoutType::VerticalStack;
    contentPanel.padding.left = 12.0f;
    contentPanel.padding.top = 10.0f;
    PrimeStage::UiNode scrollContent = scrollView.content.panel(contentPanel);
    scrollContent.label("ScrollView content area");
    scrollContent.spacer(120.0f);
  }

  PrimeStage::WindowSpec windowSpec;
  windowSpec.title = "Window";
  windowSpec.positionX = 900.0f;
  windowSpec.positionY = 450.0f;
  windowSpec.width = 220.0f;
  windowSpec.height = 150.0f;
  root.window(windowSpec,
              [](PrimeStage::Window& window) { window.content.label("Window content"); });
}

void runRebuildIfNeeded(DemoApp& app) {
  (void)app.ui.runRebuildIfNeeded([&app](PrimeStage::UiNode root) { rebuildUi(root, app); });
}

} // namespace

int main(int argc, char** argv) {
  std::optional<std::string> snapshotPath;
  uint32_t snapshotWidth = 1280u;
  uint32_t snapshotHeight = 720u;
  float snapshotScale = 1.0f;

  for (int i = 1; i < argc; ++i) {
    std::string_view arg = argv[i];
    if (arg == "--snapshot" && i + 1 < argc) {
      snapshotPath = std::string(argv[++i]);
    } else if (arg == "--width" && i + 1 < argc) {
      snapshotWidth = static_cast<uint32_t>(std::strtoul(argv[++i], nullptr, 10));
    } else if (arg == "--height" && i + 1 < argc) {
      snapshotHeight = static_cast<uint32_t>(std::strtoul(argv[++i], nullptr, 10));
    } else if (arg == "--scale" && i + 1 < argc) {
      snapshotScale = std::strtof(argv[++i], nullptr);
    }
  }

  if (snapshotPath.has_value()) {
    DemoApp app;
    initializeState(app.state);
    float resolvedScale = snapshotScale > 0.0f ? snapshotScale : 1.0f;
    app.ui.setSurfaceMetrics(snapshotWidth, snapshotHeight, resolvedScale);
    app.ui.setRenderMetrics(snapshotWidth, snapshotHeight, resolvedScale);
    runRebuildIfNeeded(app);
    PrimeStage::RenderStatus status = app.ui.renderToPng(*snapshotPath);
    if (!status.ok()) {
      std::cerr << "Failed to render snapshot to " << *snapshotPath << ": "
                << PrimeStage::renderStatusMessage(status.code);
      if (!status.detail.empty()) {
        std::cerr << " (" << status.detail << ")";
      }
      std::cerr << "\n";
      return 1;
    }
    std::cout << "Wrote snapshot to " << *snapshotPath << "\n";
    return 0;
  }

  std::cout << "PrimeStage widgets demo" << std::endl;

  auto hostResult = PrimeHost::createHost();
  if (!hostResult) {
    std::cerr << "PrimeHost unavailable (" << static_cast<int>(hostResult.error().code) << ")\n";
    return 1;
  }

  DemoApp app;
  app.host = hostResult.value().get();
  initializeState(app.state);
  app.ui.inputBridge().scrollLinePixels = ScrollLinePixels;
  app.ui.inputBridge().scrollDirectionSign = 1.0f;

  PrimeHost::SurfaceConfig config{};
  config.width = 1280u;
  config.height = 720u;
  config.resizable = true;
  config.title = std::string("PrimeStage Widgets");

  auto surfaceResult = app.host->createSurface(config);
  if (!surfaceResult) {
    std::cerr << "Failed to create surface (" << static_cast<int>(surfaceResult.error().code) << ")\n";
    return 1;
  }
  PrimeHost::SurfaceId surfaceId = surfaceResult.value();
  app.surfaceId = surfaceId;
  app.ui.connectHostServices(*app.host, app.surfaceId);

  if (auto size = app.host->surfaceSize(surfaceId)) {
    float scale = 1.0f;
    if (auto hostScale = app.host->surfaceScale(surfaceId)) {
      scale = hostScale.value();
    }
    app.ui.setSurfaceMetrics(size->width, size->height, scale);
  }

  PrimeHost::Callbacks callbacks;
  callbacks.onFrame = [&app](PrimeHost::SurfaceId target,
                             PrimeHost::FrameTiming const&,
                             PrimeHost::FrameDiagnostics const&) {
    if (target != app.surfaceId) {
      return;
    }
    runRebuildIfNeeded(app);
    auto bufferResult = app.host->acquireFrameBuffer(target);
    if (!bufferResult) {
      return;
    }
    PrimeHost::FrameBuffer buffer = bufferResult.value();
    float renderScale = buffer.scale > 0.0f ? buffer.scale : 1.0f;
    app.ui.setRenderMetrics(buffer.size.width, buffer.size.height, renderScale);
    PrimeStage::RenderTarget targetBuffer;
    targetBuffer.pixels = buffer.pixels;
    targetBuffer.width = buffer.size.width;
    targetBuffer.height = buffer.size.height;
    targetBuffer.stride = buffer.stride;
    targetBuffer.scale = buffer.scale;
    PrimeStage::RenderStatus status = app.ui.renderToTarget(targetBuffer);
    if (!status.ok()) {
      std::cerr << "Frame render failed: " << PrimeStage::renderStatusMessage(status.code);
      if (!status.detail.empty()) {
        std::cerr << " (" << status.detail << ")";
      }
      std::cerr << "\n";
    }
    app.host->presentFrameBuffer(target, buffer);
    app.ui.markFramePresented();
  };
  app.host->setCallbacks(callbacks);

  runRebuildIfNeeded(app);
  app.ui.lifecycle().requestFrame();
  app.host->requestFrame(app.surfaceId, true);

  std::array<PrimeHost::Event, 256> events{};
  std::array<char, 8192> textBytes{};
  PrimeHost::EventBuffer buffer{
      std::span<PrimeHost::Event>(events.data(), events.size()),
      std::span<char>(textBytes.data(), textBytes.size()),
  };

  bool running = true;

  while (running) {
    app.host->waitEvents();

    auto batchResult = app.host->pollEvents(buffer);
    if (!batchResult) {
      std::cerr << "pollEvents failed (" << static_cast<int>(batchResult.error().code) << ")\n";
      continue;
    }

    const auto& batch = batchResult.value();
    bool bypassCap = false;

    for (const auto& event : batch.events) {
      if (auto* input = std::get_if<PrimeHost::InputEvent>(&event.payload)) {
        PrimeStage::InputBridgeResult bridgeResult =
            app.ui.bridgeHostInputEvent(*input, batch, PrimeStage::HostKey::Escape);
        if (bridgeResult.requestExit) {
          running = false;
          continue;
        }
        if (bridgeResult.requestFrame) {
          app.ui.lifecycle().requestFrame();
        }
        if (bridgeResult.bypassFrameCap) {
          bypassCap = true;
        }
      } else if (auto* resize = std::get_if<PrimeHost::ResizeEvent>(&event.payload)) {
        app.ui.setSurfaceMetrics(resize->width, resize->height, resize->scale);
        bypassCap = true;
      } else if (auto* lifecycle = std::get_if<PrimeHost::LifecycleEvent>(&event.payload)) {
        if (lifecycle->phase == PrimeHost::LifecyclePhase::Destroyed) {
          running = false;
        }
      }
    }

    runRebuildIfNeeded(app);

    if (app.ui.lifecycle().framePending()) {
      app.host->requestFrame(app.surfaceId, bypassCap);
    }
  }

  app.host->destroySurface(app.surfaceId);
  return 0;
}
