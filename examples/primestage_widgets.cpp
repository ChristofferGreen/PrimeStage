#include "PrimeHost/PrimeHost.h"
#include "PrimeStage/InputBridge.h"
#include "PrimeStage/PrimeStage.h"
#include "PrimeStage/Render.h"

#include "PrimeFrame/Events.h"
#include "PrimeFrame/Focus.h"
#include "PrimeFrame/Layout.h"

#include <algorithm>
#include <array>
#include <cmath>
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
  PrimeStage::ToggleState toggle{};
  PrimeStage::CheckboxState checkbox{};
  PrimeStage::TabsState tabs{};
  PrimeStage::DropdownState dropdown{};
  PrimeStage::SliderState slider{};
  PrimeStage::ProgressBarState progress{};
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
  PrimeFrame::Frame frame;
  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;
  DemoState state;
  PrimeStage::FrameLifecycle runtime{};
  uint32_t surfaceWidth = 1280u;
  uint32_t surfaceHeight = 720u;
  float surfaceScale = 1.0f;
  uint32_t renderWidth = 0u;
  uint32_t renderHeight = 0u;
  float renderScale = 1.0f;
  PrimeStage::RenderOptions renderOptions{};
  PrimeStage::InputBridgeState inputBridge{};
};

PrimeStage::UiNode createSection(PrimeStage::UiNode parent, std::string_view title) {
  PrimeStage::StackSpec sectionSpec;
  sectionSpec.size.stretchX = 1.0f;
  sectionSpec.gap = 8.0f;
  PrimeStage::UiNode section = parent.createVerticalStack(sectionSpec);

  PrimeStage::LabelSpec heading;
  heading.text = title;
  section.createLabel(heading);

  PrimeStage::DividerSpec divider;
  divider.size.preferredHeight = 1.0f;
  divider.size.stretchX = 1.0f;
  section.createDivider(divider);
  return section;
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

PrimeHost::CursorShape cursorShapeForHint(PrimeStage::CursorHint hint) {
  switch (hint) {
    case PrimeStage::CursorHint::IBeam:
      return PrimeHost::CursorShape::IBeam;
    case PrimeStage::CursorHint::Arrow:
    default:
      return PrimeHost::CursorShape::Arrow;
  }
}

void initializeState(DemoState& state) {
  state.textField.text = "Editable text field";
  state.toggle.on = true;
  state.checkbox.checked = false;
  state.slider.value = 0.35f;
  state.progress.value = state.slider.value;
  state.tabs.selectedIndex = 0;
  state.dropdown.selectedIndex = 0;
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

void rebuildUi(DemoApp& app) {
  app.frame = PrimeFrame::Frame();
  app.router.clearAllCaptures();

  app.renderOptions = PrimeStage::RenderOptions{};

  PrimeFrame::NodeId rootId = app.frame.createNode();
  app.frame.addRoot(rootId);
  if (PrimeFrame::Node* rootNode = app.frame.getNode(rootId)) {
    rootNode->layout = PrimeFrame::LayoutType::Overlay;
    rootNode->visible = true;
    rootNode->clipChildren = true;
    rootNode->hitTestVisible = false;
  }

  PrimeStage::UiNode root(app.frame, rootId);

  PrimeStage::StackSpec pageSpec;
  pageSpec.size.stretchX = 1.0f;
  pageSpec.size.stretchY = 1.0f;
  pageSpec.padding.left = 14.0f;
  pageSpec.padding.top = 12.0f;
  pageSpec.padding.right = 14.0f;
  pageSpec.padding.bottom = 12.0f;
  pageSpec.gap = 10.0f;
  PrimeStage::UiNode page = root.createVerticalStack(pageSpec);

  PrimeStage::LabelSpec title;
  title.text = "PrimeStage Widgets";
  page.createLabel(title);

  PrimeStage::ParagraphSpec intro;
  intro.text = "A compact gallery showing each widget with mostly default API usage.";
  intro.maxWidth = 780.0f;
  page.createParagraph(intro);

  PrimeStage::StackSpec columnsSpec;
  columnsSpec.size.stretchX = 1.0f;
  columnsSpec.size.stretchY = 1.0f;
  columnsSpec.gap = 14.0f;
  PrimeStage::UiNode columns = page.createHorizontalStack(columnsSpec);

  PrimeStage::StackSpec columnSpec;
  columnSpec.size.stretchX = 1.0f;
  columnSpec.gap = 10.0f;
  PrimeStage::UiNode leftColumn = columns.createVerticalStack(columnSpec);
  PrimeStage::UiNode rightColumn = columns.createVerticalStack(columnSpec);

  PrimeStage::UiNode basic = createSection(leftColumn, "Basic");
  {
    PrimeStage::TextLineSpec line;
    line.text = "TextLine";
    basic.createTextLine(line);

    PrimeStage::LabelSpec label;
    label.text = "Label widget";
    basic.createLabel(label);

    PrimeStage::ParagraphSpec paragraph;
    paragraph.text =
        "Paragraph widget wraps text naturally based on width constraints provided by layout.";
    paragraph.maxWidth = 360.0f;
    basic.createParagraph(paragraph);

    PrimeStage::PanelSpec panel;
    panel.size.preferredWidth = 220.0f;
    panel.size.preferredHeight = 36.0f;
    panel.layout = PrimeFrame::LayoutType::VerticalStack;
    panel.padding.left = 6.0f;
    panel.padding.top = 6.0f;
    PrimeStage::UiNode panelNode = basic.createPanel(panel);
    PrimeStage::LabelSpec panelLabel;
    panelLabel.text = "Panel widget";
    panelNode.createLabel(panelLabel);

    PrimeStage::DividerSpec divider;
    divider.size.preferredHeight = 1.0f;
    divider.size.stretchX = 1.0f;
    basic.createDivider(divider);

    PrimeStage::SpacerSpec spacer;
    spacer.size.preferredHeight = 4.0f;
    basic.createSpacer(spacer);
  }

  PrimeStage::UiNode actions = createSection(leftColumn, "Buttons, Toggle, Checkbox");
  {
    PrimeStage::StackSpec rowSpec;
    rowSpec.gap = 12.0f;
    PrimeStage::UiNode row = actions.createHorizontalStack(rowSpec);

    PrimeStage::ButtonSpec button;
    button.label = "Button";
    button.callbacks.onClick = [&app]() {
      app.state.clickCount += 1;
      app.runtime.requestRebuild();
    };
    row.createButton(button);

    PrimeStage::ToggleSpec toggle;
    toggle.state = &app.state.toggle;
    toggle.callbacks.onChanged = [&app](bool) {
      app.runtime.requestRebuild();
    };
    row.createToggle(toggle);

    PrimeStage::CheckboxSpec checkbox;
    checkbox.state = &app.state.checkbox;
    checkbox.label = "Checkbox";
    checkbox.callbacks.onChanged = [&app](bool) {
      app.runtime.requestRebuild();
    };
    row.createCheckbox(checkbox);

    PrimeStage::TextLineSpec summary;
    std::string summaryText = "Clicks: " + std::to_string(app.state.clickCount);
    summary.text = summaryText;
    actions.createTextLine(summary);
  }

  PrimeStage::UiNode textInput = createSection(leftColumn, "Text Field + Selectable Text");
  {
    PrimeStage::TextFieldClipboard clipboard;
    clipboard.setText = [&app](std::string_view text) {
      if (app.host) {
        app.host->setClipboardText(text);
      }
    };
    clipboard.getText = [&app]() -> std::string {
      if (!app.host) {
        return {};
      }
      auto size = app.host->clipboardTextSize();
      if (!size || size.value() == 0u) {
        return {};
      }
      std::string buffer(size.value(), '\0');
      auto text = app.host->clipboardText(std::span<char>(buffer));
      if (!text) {
        return {};
      }
      return std::string(text->data(), text->size());
    };

    PrimeStage::TextFieldSpec field;
    field.state = &app.state.textField;
    field.placeholder = "Type here";
    field.clipboard = clipboard;
    field.size.preferredWidth = 320.0f;
    field.callbacks.onStateChanged = [&app]() {
      app.runtime.requestFrame();
    };
    field.callbacks.onCursorHintChanged = [&app](PrimeStage::CursorHint hint) {
      if (app.host && app.surfaceId.isValid()) {
        app.host->setCursorShape(app.surfaceId, cursorShapeForHint(hint));
      }
    };
    textInput.createTextField(field);

    PrimeStage::SelectableTextClipboard selectableClipboard;
    selectableClipboard.setText = clipboard.setText;

    PrimeStage::SelectableTextSpec selectable;
    selectable.state = &app.state.selectableText;
    selectable.text = app.state.selectableTextContent;
    selectable.clipboard = selectableClipboard;
    selectable.maxWidth = 320.0f;
    selectable.size.preferredWidth = 320.0f;
    selectable.callbacks.onStateChanged = [&app]() {
      app.runtime.requestFrame();
    };
    selectable.callbacks.onCursorHintChanged = [&app](PrimeStage::CursorHint hint) {
      if (app.host && app.surfaceId.isValid()) {
        app.host->setCursorShape(app.surfaceId, cursorShapeForHint(hint));
      }
    };
    textInput.createSelectableText(selectable);
  }

  PrimeStage::UiNode range = createSection(rightColumn, "Slider + Progress");
  {
    PrimeStage::SliderSpec slider;
    slider.state = &app.state.slider;
    slider.size.preferredWidth = 280.0f;
    range.createSlider(slider);

    PrimeStage::ProgressBarSpec progress;
    progress.state = &app.state.progress;
    progress.size.preferredWidth = 280.0f;
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
    tabs.state = &app.state.tabs;
    tabs.labels = tabViews;
    tabs.callbacks.onTabChanged = [&app](int) {
      app.runtime.requestRebuild();
    };
    choice.createTabs(tabs);

    std::vector<std::string_view> dropdownViews;
    dropdownViews.reserve(app.state.dropdownItems.size());
    for (const std::string& item : app.state.dropdownItems) {
      dropdownViews.push_back(item);
    }

    PrimeStage::DropdownSpec dropdown;
    dropdown.state = &app.state.dropdown;
    dropdown.options = dropdownViews;
    dropdown.callbacks.onSelected = [&app](int) {
      app.runtime.requestRebuild();
    };
    choice.createDropdown(dropdown);

    std::vector<std::string_view> listViews;
    listViews.reserve(app.state.listItems.size());
    for (const std::string& item : app.state.listItems) {
      listViews.push_back(item);
    }

    PrimeStage::ListSpec list;
    list.items = listViews;
    list.selectedIndex = app.state.listSelectedIndex;
    list.size.preferredWidth = 280.0f;
    list.size.preferredHeight = 116.0f;
    list.callbacks.onSelected = [&app](PrimeStage::ListRowInfo const& info) {
      app.state.listSelectedIndex = info.rowIndex;
      app.runtime.requestRebuild();
    };
    choice.createList(list);
  }

  PrimeStage::UiNode data = createSection(rightColumn, "Table + Tree View");
  {
    PrimeStage::TableSpec table;
    table.selectedRow = app.state.tableSelectedRow;
    table.size.preferredWidth = 360.0f;
    table.size.preferredHeight = 116.0f;
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
    table.callbacks.onRowClicked = [&app](PrimeStage::TableRowInfo const& info) {
      app.state.tableSelectedRow = info.rowIndex;
      app.runtime.requestRebuild();
    };
    data.createTable(table);

    PrimeStage::TreeViewSpec tree;
    tree.nodes = app.state.tree;
    tree.size.preferredWidth = 360.0f;
    tree.size.preferredHeight = 120.0f;
    tree.callbacks.onSelectionChanged = [&app](PrimeStage::TreeViewRowInfo const& info) {
      clearTreeSelection(app.state.tree);
      if (PrimeStage::TreeNode* node = findTreeNode(app.state.tree, info.path)) {
        node->selected = true;
      }
      app.runtime.requestRebuild();
    };
    tree.callbacks.onExpandedChanged = [&app](PrimeStage::TreeViewRowInfo const& info, bool expanded) {
      if (PrimeStage::TreeNode* node = findTreeNode(app.state.tree, info.path)) {
        node->expanded = expanded;
      }
      app.runtime.requestRebuild();
    };
    data.createTreeView(tree);
  }

  PrimeStage::UiNode containers = createSection(rightColumn, "Scroll View + Window");
  {
    PrimeStage::ScrollViewSpec scrollSpec;
    scrollSpec.size.preferredWidth = 360.0f;
    scrollSpec.size.preferredHeight = 92.0f;
    PrimeStage::ScrollView scrollView = containers.createScrollView(scrollSpec);

    PrimeStage::PanelSpec contentPanel;
    contentPanel.size.preferredWidth = 520.0f;
    contentPanel.size.preferredHeight = 200.0f;
    contentPanel.layout = PrimeFrame::LayoutType::VerticalStack;
    contentPanel.padding.left = 12.0f;
    contentPanel.padding.top = 10.0f;
    PrimeStage::UiNode scrollContent = scrollView.content.createPanel(contentPanel);

    PrimeStage::LabelSpec scrollLabel;
    scrollLabel.text = "ScrollView content area";
    scrollContent.createLabel(scrollLabel);

    PrimeStage::SpacerSpec tallSpacer;
    tallSpacer.size.preferredHeight = 120.0f;
    scrollContent.createSpacer(tallSpacer);
  }

  PrimeStage::WindowSpec windowSpec;
  windowSpec.title = "Window";
  windowSpec.positionX = 900.0f;
  windowSpec.positionY = 450.0f;
  windowSpec.width = 220.0f;
  windowSpec.height = 150.0f;
  PrimeStage::Window window = root.createWindow(windowSpec);

  PrimeStage::LabelSpec windowLabel;
  windowLabel.text = "Window content";
  window.content.createLabel(windowLabel);
}

void runRebuildIfNeeded(DemoApp& app) {
  app.runtime.runRebuildIfNeeded([&app]() { rebuildUi(app); });
}

void updateLayoutIfNeeded(DemoApp& app) {
  app.runtime.runLayoutIfNeeded([&app]() {
    PrimeFrame::LayoutOptions options;
    float scale = app.renderScale > 0.0f ? app.renderScale
                                         : (app.surfaceScale > 0.0f ? app.surfaceScale : 1.0f);
    uint32_t widthPx = app.renderWidth > 0u ? app.renderWidth : app.surfaceWidth;
    uint32_t heightPx = app.renderHeight > 0u ? app.renderHeight : app.surfaceHeight;
    options.rootWidth = static_cast<float>(widthPx) / scale;
    options.rootHeight = static_cast<float>(heightPx) / scale;
    app.layoutEngine.layout(app.frame, app.layout, options);
    app.focus.updateAfterRebuild(app.frame, app.layout);
  });
}

bool dispatchFrameEvent(DemoApp& app, PrimeFrame::Event const& event) {
  updateLayoutIfNeeded(app);
  return app.router.dispatch(event, app.frame, app.layout, &app.focus);
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
    app.surfaceWidth = snapshotWidth;
    app.surfaceHeight = snapshotHeight;
    app.surfaceScale = snapshotScale > 0.0f ? snapshotScale : 1.0f;
    app.renderWidth = snapshotWidth;
    app.renderHeight = snapshotHeight;
    app.renderScale = app.surfaceScale;
    runRebuildIfNeeded(app);
    updateLayoutIfNeeded(app);
    PrimeStage::RenderStatus status =
        PrimeStage::renderFrameToPng(app.frame, app.layout, *snapshotPath, app.renderOptions);
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
  app.inputBridge.scrollLinePixels = ScrollLinePixels;
  app.inputBridge.scrollDirectionSign = 1.0f;

  PrimeHost::SurfaceConfig config{};
  config.width = app.surfaceWidth;
  config.height = app.surfaceHeight;
  config.resizable = true;
  config.title = std::string("PrimeStage Widgets");

  auto surfaceResult = app.host->createSurface(config);
  if (!surfaceResult) {
    std::cerr << "Failed to create surface (" << static_cast<int>(surfaceResult.error().code) << ")\n";
    return 1;
  }
  PrimeHost::SurfaceId surfaceId = surfaceResult.value();
  app.surfaceId = surfaceId;

  if (auto size = app.host->surfaceSize(surfaceId)) {
    app.surfaceWidth = size->width;
    app.surfaceHeight = size->height;
  }
  if (auto scale = app.host->surfaceScale(surfaceId)) {
    app.surfaceScale = scale.value();
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
    uint32_t previousRenderWidth = app.renderWidth;
    uint32_t previousRenderHeight = app.renderHeight;
    float previousRenderScale = app.renderScale;
    float nextScale = buffer.scale > 0.0f ? buffer.scale : app.renderScale;
    app.renderWidth = buffer.size.width;
    app.renderHeight = buffer.size.height;
    app.renderScale = nextScale;
    app.surfaceScale = app.renderScale;
    bool renderTargetChanged = previousRenderWidth != app.renderWidth ||
                               previousRenderHeight != app.renderHeight ||
                               std::abs(previousRenderScale - app.renderScale) > 0.0001f;
    if (renderTargetChanged) {
      app.runtime.requestLayout();
    }
    updateLayoutIfNeeded(app);
    PrimeStage::RenderTarget targetBuffer;
    targetBuffer.pixels = buffer.pixels;
    targetBuffer.width = buffer.size.width;
    targetBuffer.height = buffer.size.height;
    targetBuffer.stride = buffer.stride;
    targetBuffer.scale = buffer.scale;
    PrimeStage::RenderStatus status =
        PrimeStage::renderFrameToTarget(app.frame, app.layout, targetBuffer, app.renderOptions);
    if (!status.ok()) {
      std::cerr << "Frame render failed: " << PrimeStage::renderStatusMessage(status.code);
      if (!status.detail.empty()) {
        std::cerr << " (" << status.detail << ")";
      }
      std::cerr << "\n";
    }
    app.host->presentFrameBuffer(target, buffer);
    app.runtime.markFramePresented();
  };
  app.host->setCallbacks(callbacks);

  runRebuildIfNeeded(app);
  app.runtime.requestFrame();
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
        PrimeStage::InputBridgeResult bridgeResult = PrimeStage::bridgeHostInputEvent(
            *input,
            batch,
            app.inputBridge,
            [&app](PrimeFrame::Event const& frameEvent) { return dispatchFrameEvent(app, frameEvent); },
            PrimeStage::HostKey::Escape);
        if (bridgeResult.requestExit) {
          running = false;
          continue;
        }
        if (bridgeResult.requestFrame) {
          app.runtime.requestFrame();
        }
        if (bridgeResult.bypassFrameCap) {
          bypassCap = true;
        }
      } else if (auto* resize = std::get_if<PrimeHost::ResizeEvent>(&event.payload)) {
        app.surfaceWidth = resize->width;
        app.surfaceHeight = resize->height;
        app.surfaceScale = resize->scale > 0.0f ? resize->scale : 1.0f;
        app.runtime.requestRebuild();
        bypassCap = true;
      } else if (auto* lifecycle = std::get_if<PrimeHost::LifecycleEvent>(&event.payload)) {
        if (lifecycle->phase == PrimeHost::LifecyclePhase::Destroyed) {
          running = false;
        }
      }
    }

    runRebuildIfNeeded(app);

    if (app.runtime.framePending()) {
      app.host->requestFrame(app.surfaceId, bypassCap);
    }
  }

  app.host->destroySurface(app.surfaceId);
  return 0;
}
