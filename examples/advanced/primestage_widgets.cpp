// Advanced PrimeFrame integration (documented exception): this sample intentionally demonstrates
// advanced host/runtime interop through PrimeHost and PrimeFrame integration points.
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
constexpr PrimeHost::KeyModifierMask ControlModifier =
    static_cast<PrimeHost::KeyModifierMask>(PrimeHost::KeyModifier::Control);
constexpr std::string_view ActionNextTab = "demo.next_tab";
constexpr std::string_view ActionToggleCheckbox = "demo.toggle_checkbox";

struct AssetRow {
  std::string name;
  std::string type;
  std::string size;
};

struct AssetTreeNode {
  std::string label;
  std::vector<AssetTreeNode> children;
  bool expanded = true;
  bool selected = false;
};

struct DemoState {
  PrimeStage::State<bool> toggle{};
  PrimeStage::State<bool> checkbox{};
  PrimeStage::State<int> tabs{};
  PrimeStage::State<int> dropdown{};
  PrimeStage::State<float> sliderValue{};
  PrimeStage::State<float> progressValue{};
  std::vector<AssetRow> tableRows;
  std::vector<AssetTreeNode> tree;
  std::string displayName;
  std::string selectableTextContent;
  std::vector<std::string> listItems;
  int actionCount = 0;
  std::string lastAction;
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

void initializeState(DemoState& state) {
  state.displayName = "Editable text field";
  state.toggle.value = true;
  state.checkbox.value = false;
  state.sliderValue.value = 0.35f;
  state.progressValue.value = state.sliderValue.value;
  state.tabs.value = 0;
  state.dropdown.value = 0;
  state.selectableTextContent =
      "Selectable text supports drag selection, keyboard movement, and clipboard shortcuts.";

  state.listItems = {"Alpha", "Beta", "Gamma", "Delta"};
  state.lastAction = "none";
  state.tableRows = {
      {"icons.png", "Texture", "512 KB"},
      {"theme.ogg", "Audio", "3.1 MB"},
      {"ui.vert", "Shader", "14 KB"},
  };

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

int cycleIndex(int value, size_t itemCount, int delta) {
  if (itemCount == 0u) {
    return 0;
  }
  int span = static_cast<int>(itemCount);
  int next = value + delta;
  next %= span;
  if (next < 0) {
    next += span;
  }
  return next;
}

void registerActions(DemoApp& app) {
  (void)app.ui.registerAction(ActionNextTab, [&app](PrimeStage::AppActionInvocation const&) {
    app.state.tabs.value = cycleIndex(app.state.tabs.value, 3u, 1);
    app.state.dropdown.value = cycleIndex(app.state.dropdown.value, 4u, 1);
    app.state.actionCount += 1;
    app.state.lastAction = std::string(ActionNextTab);
    // Advanced lifecycle orchestration (documented exception): this demo mirrors action state
    // into summary text labels that require rebuild to update.
    app.ui.lifecycle().requestRebuild();
  });
  (void)app.ui.registerAction(ActionToggleCheckbox, [&app](PrimeStage::AppActionInvocation const&) {
    app.state.checkbox.value = !app.state.checkbox.value;
    app.state.actionCount += 1;
    app.state.lastAction = std::string(ActionToggleCheckbox);
    app.ui.lifecycle().requestRebuild();
  });

  PrimeStage::AppShortcut nextTabShortcut;
  nextTabShortcut.key = PrimeStage::HostKey::Enter;
  nextTabShortcut.modifiers = ControlModifier;
  (void)app.ui.bindShortcut(nextTabShortcut, ActionNextTab);

  PrimeStage::AppShortcut toggleShortcut;
  toggleShortcut.key = PrimeStage::HostKey::Space;
  toggleShortcut.modifiers = ControlModifier;
  (void)app.ui.bindShortcut(toggleShortcut, ActionToggleCheckbox);
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
    // Advanced PrimeFrame integration (documented exception): this advanced sample demonstrates
    // selective low-level layout interop through PrimeFrame layout enums.
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
    row.button("Next Tab", app.ui.makeActionCallback(std::string(ActionNextTab)));
    row.button("Toggle Check", app.ui.makeActionCallback(std::string(ActionToggleCheckbox)));
    row.toggle(PrimeStage::bind(app.state.toggle));
    row.checkbox("Checkbox", PrimeStage::bind(app.state.checkbox));
    actions.textLine("Shortcuts: Ctrl+Enter (next tab), Ctrl+Space (toggle check)");
    actions.textLine(std::string("Last action: ") + app.state.lastAction);
  }

  PrimeStage::UiNode settings = createSection(leftColumn, "Settings Form + Selectable Text");
  {
    PrimeStage::FormSpec formSpec;
    formSpec.rowGap = 10.0f;
    settings.form(formSpec, [&](PrimeStage::UiNode& form) {
      PrimeStage::FormFieldSpec nameField;
      nameField.label = "Display name";
      nameField.helpText = "Used by project-level labels and command previews.";
      nameField.invalid = app.state.displayName.empty();
      nameField.errorText = "Display name cannot be empty.";
      form.formField(nameField, [&](PrimeStage::UiNode& fieldSlot) {
        PrimeStage::TextFieldSpec field;
        field.text = app.state.displayName;
        field.placeholder = "Type here";
        field.callbacks.onChange = [&app](std::string_view text) {
          app.state.displayName = std::string(text);
        };
        app.ui.applyPlatformServices(field);
        fieldSlot.createTextField(field);
      });

      form.formField("Release channel",
                     [&](PrimeStage::UiNode& fieldSlot) {
                       fieldSlot.dropdown({"Preview", "Edit", "Export", "Publish"},
                                          PrimeStage::bind(app.state.dropdown));
                     },
                     "Shortcuts and actions keep this selection synchronized.");

      form.formField("Selectable notes",
                     [&](PrimeStage::UiNode& fieldSlot) {
                       PrimeStage::SelectableTextSpec selectable;
                       selectable.text = app.state.selectableTextContent;
                       app.ui.applyPlatformServices(selectable);
                       fieldSlot.createSelectableText(selectable);
                     },
                     "Supports drag selection, keyboard movement, and clipboard shortcuts.");
    });
  }

  PrimeStage::UiNode range = createSection(rightColumn, "Slider + Progress");
  {
    range.slider(PrimeStage::bind(app.state.sliderValue));
    range.progressBar(PrimeStage::bind(app.state.progressValue));
  }

  PrimeStage::UiNode choice = createSection(rightColumn, "Tabs, Dropdown, List");
  {
    choice.tabs({"Overview", "Assets", "Settings"}, PrimeStage::bind(app.state.tabs));
    choice.dropdown({"Preview", "Edit", "Export", "Publish"},
                    PrimeStage::bind(app.state.dropdown));

    PrimeStage::ListSpec list;
    PrimeStage::ListModelAdapter listModel = PrimeStage::makeListModel(
        app.state.listItems,
        [](std::string const& item) -> std::string_view { return item; },
        [](std::string const& item) -> PrimeStage::WidgetIdentityId { return PrimeStage::widgetIdentityId(item); });
    listModel.bind(list);
    choice.createList(list);
  }

  PrimeStage::UiNode data = createSection(rightColumn, "Table + Tree View");
  {
    PrimeStage::TableSpec table;
    table.columns = {
        {"Asset"},
        {"Type"},
        {"Size"},
    };
    PrimeStage::TableModelAdapter tableModel = PrimeStage::makeTableModel(
        app.state.tableRows,
        table.columns.size(),
        [](AssetRow const& row, size_t columnIndex) -> std::string_view {
          switch (columnIndex) {
            case 0:
              return row.name;
            case 1:
              return row.type;
            case 2:
              return row.size;
            default:
              return {};
          }
        },
        [](AssetRow const& row) -> PrimeStage::WidgetIdentityId { return PrimeStage::widgetIdentityId(row.name); });
    tableModel.bindRows(table);
    data.createTable(table);

    PrimeStage::TreeViewSpec tree;
    PrimeStage::TreeModelAdapter treeModel = PrimeStage::makeTreeModel(
        app.state.tree,
        [](AssetTreeNode const& node) -> std::string_view { return node.label; },
        [](AssetTreeNode const& node) -> std::vector<AssetTreeNode> const& { return node.children; },
        [](AssetTreeNode const& node) { return node.expanded; },
        [](AssetTreeNode const& node) { return node.selected; },
        [](AssetTreeNode const& node) -> PrimeStage::WidgetIdentityId {
          return PrimeStage::widgetIdentityId(node.label);
        });
    treeModel.bind(tree);
    data.createTreeView(tree);
  }

  PrimeStage::UiNode containers = createSection(rightColumn, "Scroll View + Window");
  {
    PrimeStage::ScrollViewSpec scrollSpec;
    PrimeStage::ScrollView scrollView = containers.createScrollView(scrollSpec);

    PrimeStage::PanelSpec contentPanel;
    contentPanel.size.preferredWidth = 520.0f;
    contentPanel.size.preferredHeight = 200.0f;
    // Advanced PrimeFrame integration (documented exception): this advanced sample demonstrates
    // selective low-level layout interop through PrimeFrame layout enums.
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
    registerActions(app);
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
  registerActions(app);
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
  // Advanced lifecycle orchestration (documented exception): initial frame presentation is
  // scheduled explicitly before entering the host event loop.
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
