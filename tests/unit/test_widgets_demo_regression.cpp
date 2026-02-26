#include "PrimeStage/Ui.h"

#include "PrimeFrame/Events.h"
#include "PrimeFrame/Focus.h"
#include "PrimeFrame/Frame.h"
#include "PrimeFrame/Layout.h"

#include "third_party/doctest.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

constexpr float RootWidth = 760.0f;
constexpr float RootHeight = 520.0f;

constexpr PrimeFrame::ColorToken ColorBackground = 1u;
constexpr PrimeFrame::ColorToken ColorSurface = 2u;
constexpr PrimeFrame::ColorToken ColorAccent = 3u;
constexpr PrimeFrame::ColorToken ColorFocus = 4u;
constexpr PrimeFrame::ColorToken ColorText = 5u;

constexpr PrimeFrame::RectStyleToken StyleBackground = 1u;
constexpr PrimeFrame::RectStyleToken StyleSurface = 2u;
constexpr PrimeFrame::RectStyleToken StyleAccent = 3u;
constexpr PrimeFrame::RectStyleToken StyleFocus = 4u;

constexpr int KeyRight = 0x4F;

PrimeFrame::Color makeColor(float r, float g, float b) {
  PrimeFrame::Color color;
  color.r = r;
  color.g = g;
  color.b = b;
  color.a = 1.0f;
  return color;
}

void configureTheme(PrimeFrame::Frame& frame) {
  PrimeFrame::Theme* theme = frame.getTheme(PrimeFrame::DefaultThemeId);
  REQUIRE(theme != nullptr);

  theme->palette.assign(8u, PrimeFrame::Color{});
  theme->palette[ColorBackground] = makeColor(0.10f, 0.12f, 0.16f);
  theme->palette[ColorSurface] = makeColor(0.18f, 0.22f, 0.29f);
  theme->palette[ColorAccent] = makeColor(0.24f, 0.68f, 0.94f);
  theme->palette[ColorFocus] = makeColor(0.90f, 0.28f, 0.12f);
  theme->palette[ColorText] = makeColor(0.95f, 0.96f, 0.98f);

  theme->rectStyles.assign(8u, PrimeFrame::RectStyle{});
  theme->rectStyles[StyleBackground].fill = ColorBackground;
  theme->rectStyles[StyleSurface].fill = ColorSurface;
  theme->rectStyles[StyleAccent].fill = ColorAccent;
  theme->rectStyles[StyleFocus].fill = ColorFocus;

  theme->textStyles.assign(1u, PrimeFrame::TextStyle{});
  theme->textStyles[0].color = ColorText;
}

PrimeStage::UiNode createRoot(PrimeFrame::Frame& frame) {
  PrimeFrame::NodeId rootId = frame.createNode();
  frame.addRoot(rootId);
  PrimeFrame::Node* root = frame.getNode(rootId);
  REQUIRE(root != nullptr);
  root->layout = PrimeFrame::LayoutType::Overlay;
  root->sizeHint.width.preferred = RootWidth;
  root->sizeHint.height.preferred = RootHeight;
  return PrimeStage::UiNode(frame, rootId, true);
}

PrimeFrame::LayoutOutput layoutFrame(PrimeFrame::Frame& frame) {
  PrimeFrame::LayoutOutput output;
  PrimeFrame::LayoutEngine engine;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = RootWidth;
  options.rootHeight = RootHeight;
  engine.layout(frame, output, options);
  return output;
}

PrimeFrame::Event makePointerEvent(PrimeFrame::EventType type, int pointerId, float x, float y) {
  PrimeFrame::Event event;
  event.type = type;
  event.pointerId = pointerId;
  event.x = x;
  event.y = y;
  return event;
}

PrimeFrame::Event makePointerScrollEvent(float x, float y, float scrollY) {
  PrimeFrame::Event event;
  event.type = PrimeFrame::EventType::PointerScroll;
  event.x = x;
  event.y = y;
  event.scrollY = scrollY;
  return event;
}

PrimeFrame::Event makeTextInputEvent(std::string_view text) {
  PrimeFrame::Event event;
  event.type = PrimeFrame::EventType::TextInput;
  event.text = std::string(text);
  return event;
}

PrimeFrame::Event makeKeyDownEvent(int key) {
  PrimeFrame::Event event;
  event.type = PrimeFrame::EventType::KeyDown;
  event.key = key;
  return event;
}

std::pair<float, float> centerOf(PrimeFrame::LayoutOutput const& layout, PrimeFrame::NodeId nodeId) {
  PrimeFrame::LayoutOut const* out = layout.get(nodeId);
  REQUIRE(out != nullptr);
  return {out->absX + out->absW * 0.5f, out->absY + out->absH * 0.5f};
}

void clickCenter(PrimeFrame::Frame& frame,
                 PrimeFrame::LayoutOutput const& layout,
                 PrimeFrame::EventRouter& router,
                 PrimeFrame::FocusManager& focus,
                 PrimeFrame::NodeId nodeId,
                 int pointerId = 1) {
  auto [x, y] = centerOf(layout, nodeId);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, pointerId, x, y),
                  frame,
                  layout,
                  &focus);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, pointerId, x, y),
                  frame,
                  layout,
                  &focus);
}

struct DemoState {
  PrimeStage::TextFieldState textField{};
  int tabIndex = 0;
  int clickCount = 0;
  float sliderValue = 0.35f;
  float progressValue = 0.35f;
  int dropdownIndex = 0;
  std::vector<std::string> tabLabels = {"Overview", "Assets", "Settings"};
  std::vector<std::string> dropdownItems = {"Preview", "Edit", "Export"};
  std::vector<PrimeStage::TreeNode> treeNodes;
  int scrollEvents = 0;
  PrimeStage::TreeViewScrollInfo lastScroll{};
};

struct DemoRegressionApp {
  PrimeFrame::Frame frame;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;
  PrimeStage::WidgetIdentityReconciler widgetIdentity;
  DemoState state;
  bool needsRebuild = true;
  PrimeFrame::NodeId tabsNode{};
  PrimeFrame::NodeId buttonNode{};
  PrimeFrame::NodeId treeNode{};
  PrimeFrame::NodeId textFieldNode{};
  PrimeFrame::NodeId dropdownNode{};
  PrimeFrame::NodeId sliderNode{};
  PrimeFrame::NodeId progressNode{};
};

constexpr std::string_view WidgetTabs = "demo.tabs";
constexpr std::string_view WidgetButton = "demo.button";
constexpr std::string_view WidgetTree = "demo.tree";
constexpr std::string_view WidgetTextField = "demo.textField";
constexpr std::string_view WidgetDropdown = "demo.dropdown";
constexpr std::string_view WidgetSlider = "demo.slider";
constexpr std::string_view WidgetProgress = "demo.progress";

void initializeDemoState(DemoState& state) {
  state.textField.text = "Demo";
  state.textField.cursor = static_cast<uint32_t>(state.textField.text.size());
  state.treeNodes.clear();
  state.treeNodes.reserve(28u);
  for (int i = 0; i < 28; ++i) {
    state.treeNodes.push_back(PrimeStage::TreeNode{
        "Asset " + std::to_string(i),
        {},
        true,
        false,
    });
  }
}

bool rebuildDemoUi(DemoRegressionApp& app) {
  app.widgetIdentity.beginRebuild(app.focus.focusedNode());
  app.frame = PrimeFrame::Frame();
  configureTheme(app.frame);

  PrimeStage::UiNode root = createRoot(app.frame);
  PrimeStage::PanelSpec background;
  background.size.stretchX = 1.0f;
  background.size.stretchY = 1.0f;
  background.rectStyle = StyleBackground;
  PrimeStage::UiNode backgroundNode = root.createPanel(background);
  backgroundNode.setHitTestVisible(false);

  PrimeStage::StackSpec contentSpec;
  contentSpec.size.stretchX = 1.0f;
  contentSpec.size.stretchY = 1.0f;
  contentSpec.padding.left = 16.0f;
  contentSpec.padding.top = 16.0f;
  contentSpec.padding.right = 16.0f;
  contentSpec.padding.bottom = 16.0f;
  contentSpec.gap = 12.0f;
  PrimeStage::UiNode content = root.createVerticalStack(contentSpec);

  int tabCount = static_cast<int>(app.state.tabLabels.size());
  app.state.tabIndex = tabCount > 0 ? std::clamp(app.state.tabIndex, 0, tabCount - 1) : 0;
  std::vector<std::string_view> labels;
  labels.reserve(app.state.tabLabels.size());
  for (std::string const& label : app.state.tabLabels) {
    labels.push_back(label);
  }

  PrimeStage::TabsSpec tabsSpec;
  tabsSpec.labels = labels;
  tabsSpec.selectedIndex = app.state.tabIndex;
  tabsSpec.tabStyle = StyleSurface;
  tabsSpec.activeTabStyle = StyleAccent;
  tabsSpec.textStyle = 0u;
  tabsSpec.activeTextStyle = 0u;
  tabsSpec.size.preferredWidth = 420.0f;
  tabsSpec.size.preferredHeight = 30.0f;
  tabsSpec.callbacks.onTabChanged = [&app](int nextIndex) {
    app.state.tabIndex = nextIndex;
    app.needsRebuild = true;
  };
  PrimeStage::UiNode tabsNode = content.createTabs(tabsSpec);
  app.tabsNode = tabsNode.nodeId();
  app.widgetIdentity.registerNode(WidgetTabs, app.tabsNode);

  PrimeStage::PanelSpec pageSpec;
  pageSpec.size.stretchX = 1.0f;
  pageSpec.size.stretchY = 1.0f;
  pageSpec.layout = PrimeFrame::LayoutType::VerticalStack;
  pageSpec.padding.left = 12.0f;
  pageSpec.padding.top = 12.0f;
  pageSpec.padding.right = 12.0f;
  pageSpec.padding.bottom = 12.0f;
  pageSpec.gap = 10.0f;
  pageSpec.rectStyle = StyleSurface;
  PrimeStage::UiNode page = content.createPanel(pageSpec);

  app.buttonNode = {};
  app.treeNode = {};
  app.textFieldNode = {};
  app.dropdownNode = {};
  app.sliderNode = {};
  app.progressNode = {};

  if (app.state.tabIndex == 0) {
    PrimeStage::ButtonSpec buttonSpec;
    buttonSpec.label = "Primary";
    buttonSpec.backgroundStyle = StyleSurface;
    buttonSpec.hoverStyle = StyleAccent;
    buttonSpec.pressedStyle = StyleAccent;
    buttonSpec.focusStyle = StyleFocus;
    buttonSpec.textStyle = 0u;
    buttonSpec.size.preferredWidth = 180.0f;
    buttonSpec.size.preferredHeight = 32.0f;
    buttonSpec.callbacks.onClick = [&app]() {
      app.state.clickCount += 1;
      app.needsRebuild = true;
    };
    PrimeStage::UiNode button = page.createButton(buttonSpec);
    app.buttonNode = button.nodeId();
    app.widgetIdentity.registerNode(WidgetButton, app.buttonNode);
  } else if (app.state.tabIndex == 1) {
    PrimeStage::TreeViewSpec treeSpec;
    treeSpec.nodes = app.state.treeNodes;
    treeSpec.rowStyle = StyleSurface;
    treeSpec.rowAltStyle = StyleBackground;
    treeSpec.hoverStyle = StyleAccent;
    treeSpec.selectionStyle = StyleAccent;
    treeSpec.selectionAccentStyle = StyleAccent;
    treeSpec.caretBackgroundStyle = StyleSurface;
    treeSpec.caretLineStyle = StyleAccent;
    treeSpec.connectorStyle = StyleSurface;
    treeSpec.focusStyle = StyleFocus;
    treeSpec.textStyle = 0u;
    treeSpec.selectedTextStyle = 0u;
    treeSpec.size.preferredWidth = 500.0f;
    treeSpec.size.preferredHeight = 220.0f;
    treeSpec.scrollBar.enabled = true;
    treeSpec.scrollBar.autoThumb = true;
    treeSpec.scrollBar.inset = 8.0f;
    treeSpec.scrollBar.width = 8.0f;
    treeSpec.scrollBar.padding = 6.0f;
    treeSpec.scrollBar.trackStyle = StyleSurface;
    treeSpec.scrollBar.thumbStyle = StyleAccent;
    treeSpec.callbacks.onScrollChanged = [&app](PrimeStage::TreeViewScrollInfo const& info) {
      app.state.scrollEvents += 1;
      app.state.lastScroll = info;
    };
    PrimeStage::UiNode tree = page.createTreeView(treeSpec);
    app.treeNode = tree.nodeId();
    app.widgetIdentity.registerNode(WidgetTree, app.treeNode);
  } else {
    PrimeStage::TextFieldSpec fieldSpec;
    fieldSpec.state = &app.state.textField;
    fieldSpec.backgroundStyle = StyleSurface;
    fieldSpec.focusStyle = StyleFocus;
    fieldSpec.selectionStyle = StyleAccent;
    fieldSpec.textStyle = 0u;
    fieldSpec.placeholderStyle = 0u;
    fieldSpec.cursorStyle = StyleAccent;
    fieldSpec.size.preferredWidth = 300.0f;
    fieldSpec.size.preferredHeight = 30.0f;
    fieldSpec.callbacks.onStateChanged = [&app]() { app.needsRebuild = true; };
    fieldSpec.callbacks.onTextChanged = [&app](std::string_view) { app.needsRebuild = true; };
    PrimeStage::UiNode field = page.createTextField(fieldSpec);
    app.textFieldNode = field.nodeId();
    app.widgetIdentity.registerNode(WidgetTextField, app.textFieldNode);

    PrimeStage::DropdownSpec dropdownSpec;
    dropdownSpec.options.reserve(app.state.dropdownItems.size());
    for (std::string const& option : app.state.dropdownItems) {
      dropdownSpec.options.push_back(option);
    }
    dropdownSpec.selectedIndex = app.state.dropdownIndex;
    dropdownSpec.backgroundStyle = StyleSurface;
    dropdownSpec.textStyle = 0u;
    dropdownSpec.indicatorStyle = 0u;
    dropdownSpec.focusStyle = StyleFocus;
    dropdownSpec.size.preferredWidth = 220.0f;
    dropdownSpec.size.preferredHeight = 28.0f;
    dropdownSpec.callbacks.onSelected = [&app](int nextIndex) {
      app.state.dropdownIndex = nextIndex;
      app.needsRebuild = true;
    };
    PrimeStage::UiNode dropdown = page.createDropdown(dropdownSpec);
    app.dropdownNode = dropdown.nodeId();
    app.widgetIdentity.registerNode(WidgetDropdown, app.dropdownNode);

    PrimeStage::SliderSpec sliderSpec;
    sliderSpec.value = app.state.sliderValue;
    sliderSpec.trackStyle = StyleSurface;
    sliderSpec.fillStyle = StyleAccent;
    sliderSpec.thumbStyle = StyleAccent;
    sliderSpec.focusStyle = StyleFocus;
    sliderSpec.size.preferredWidth = 280.0f;
    sliderSpec.size.preferredHeight = 18.0f;
    sliderSpec.callbacks.onValueChanged = [&app](float value) {
      app.state.sliderValue = value;
      app.state.progressValue = value;
      app.needsRebuild = true;
    };
    PrimeStage::UiNode slider = page.createSlider(sliderSpec);
    app.sliderNode = slider.nodeId();
    app.widgetIdentity.registerNode(WidgetSlider, app.sliderNode);

    PrimeStage::ProgressBarSpec progressSpec;
    progressSpec.value = app.state.progressValue;
    progressSpec.trackStyle = StyleSurface;
    progressSpec.fillStyle = StyleAccent;
    progressSpec.focusStyle = StyleFocus;
    progressSpec.size.preferredWidth = 280.0f;
    progressSpec.size.preferredHeight = 12.0f;
    PrimeStage::UiNode progress = page.createProgressBar(progressSpec);
    app.progressNode = progress.nodeId();
    app.widgetIdentity.registerNode(WidgetProgress, app.progressNode);
  }

  app.layout = layoutFrame(app.frame);
  app.focus.updateAfterRebuild(app.frame, app.layout);
  bool restoredFocus = app.widgetIdentity.restoreFocus(app.focus, app.frame, app.layout);
  if (!restoredFocus && app.state.tabIndex == 1) {
    PrimeFrame::NodeId tree = app.widgetIdentity.findNode(WidgetTree);
    if (tree.isValid()) {
      app.focus.setFocus(app.frame, app.layout, tree);
    }
  }
  app.needsRebuild = false;
  return restoredFocus;
}

void rebuildIfNeeded(DemoRegressionApp& app) {
  if (app.needsRebuild) {
    rebuildDemoUi(app);
  }
}

std::vector<PrimeFrame::NodeId> tabChildren(PrimeFrame::Frame const& frame, PrimeFrame::NodeId tabsRow) {
  PrimeFrame::Node const* node = frame.getNode(tabsRow);
  if (!node) {
    return {};
  }
  return node->children;
}

struct ScrollbarNodes {
  PrimeFrame::NodeId track{};
  PrimeFrame::NodeId thumb{};
};

ScrollbarNodes findTreeScrollbarNodes(PrimeFrame::Frame const& frame,
                                      PrimeFrame::LayoutOutput const& layout,
                                      PrimeFrame::NodeId treeNodeId) {
  ScrollbarNodes nodes;
  PrimeFrame::Node const* treeNode = frame.getNode(treeNodeId);
  if (!treeNode) {
    return nodes;
  }

  float trackHeight = -1.0f;
  for (PrimeFrame::NodeId childId : treeNode->children) {
    PrimeFrame::LayoutOut const* out = layout.get(childId);
    if (!out) {
      continue;
    }
    if (out->absW > 12.0f || out->absH <= 0.0f) {
      continue;
    }
    if (out->absH > trackHeight) {
      trackHeight = out->absH;
      nodes.track = childId;
    }
  }
  if (!nodes.track.isValid()) {
    return nodes;
  }

  PrimeFrame::LayoutOut const* trackOut = layout.get(nodes.track);
  REQUIRE(trackOut != nullptr);
  float thumbHeight = -1.0f;
  for (PrimeFrame::NodeId childId : treeNode->children) {
    if (childId == nodes.track) {
      continue;
    }
    PrimeFrame::LayoutOut const* out = layout.get(childId);
    if (!out) {
      continue;
    }
    if (out->absW > 12.0f || out->absH <= 0.0f || out->absH >= trackOut->absH) {
      continue;
    }
    if (out->absX + 0.5f < trackOut->absX || out->absX > trackOut->absX + trackOut->absW + 0.5f) {
      continue;
    }
    if (out->absH > thumbHeight) {
      thumbHeight = out->absH;
      nodes.thumb = childId;
    }
  }
  return nodes;
}

} // namespace

TEST_CASE("Widgets demo regression covers page flow and rebuild stability") {
  DemoRegressionApp app;
  initializeDemoState(app.state);
  rebuildDemoUi(app);

  REQUIRE(app.state.tabIndex == 0);
  REQUIRE(app.buttonNode.isValid());

  clickCenter(app.frame, app.layout, app.router, app.focus, app.buttonNode);
  CHECK(app.state.clickCount == 1);
  CHECK(app.focus.focusedNode() == app.buttonNode);
  CHECK(app.needsRebuild);
  rebuildIfNeeded(app);
  CHECK(app.state.clickCount == 1);

  std::vector<PrimeFrame::NodeId> tabs = tabChildren(app.frame, app.tabsNode);
  REQUIRE(tabs.size() >= 3u);
  clickCenter(app.frame, app.layout, app.router, app.focus, tabs[2]);
  CHECK(app.state.tabIndex == 2);
  CHECK(app.needsRebuild);
  rebuildIfNeeded(app);

  REQUIRE(app.textFieldNode.isValid());
  REQUIRE(app.dropdownNode.isValid());
  REQUIRE(app.sliderNode.isValid());
  REQUIRE(app.progressNode.isValid());

  clickCenter(app.frame, app.layout, app.router, app.focus, app.textFieldNode);
  CHECK(app.focus.focusedNode() == app.textFieldNode);
  app.router.dispatch(makeTextInputEvent(" stage"), app.frame, app.layout, &app.focus);
  CHECK(app.state.textField.text == "Demo stage");
  CHECK(app.needsRebuild);

  PrimeFrame::NodeId preRebuildField = app.textFieldNode;
  rebuildIfNeeded(app);
  CHECK(app.textFieldNode.isValid());
  CHECK(preRebuildField.isValid());
  CHECK(app.focus.focusedNode() == app.textFieldNode);

  int previousDropdown = app.state.dropdownIndex;
  clickCenter(app.frame, app.layout, app.router, app.focus, app.dropdownNode);
  CHECK(app.state.dropdownIndex != previousDropdown);

  PrimeFrame::LayoutOut const* sliderOut = app.layout.get(app.sliderNode);
  REQUIRE(sliderOut != nullptr);
  float sliderX = sliderOut->absX + sliderOut->absW * 0.78f;
  float sliderY = sliderOut->absY + sliderOut->absH * 0.5f;
  app.router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 2, sliderX, sliderY),
                      app.frame,
                      app.layout,
                      &app.focus);
  app.router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 2, sliderX, sliderY),
                      app.frame,
                      app.layout,
                      &app.focus);
  CHECK(app.state.sliderValue > 0.60f);
  CHECK(app.state.progressValue == doctest::Approx(app.state.sliderValue));
  CHECK(app.needsRebuild);
  rebuildIfNeeded(app);

  app.focus.clearFocus(app.frame);
  std::vector<PrimeFrame::NodeId> visited;
  for (size_t i = 0; i < 16u; ++i) {
    if (!app.focus.handleTab(app.frame, app.layout, true)) {
      break;
    }
    PrimeFrame::NodeId focused = app.focus.focusedNode();
    if (!focused.isValid()) {
      continue;
    }
    if (std::find(visited.begin(), visited.end(), focused) == visited.end()) {
      visited.push_back(focused);
    }
  }
  CHECK(std::find(visited.begin(), visited.end(), app.textFieldNode) != visited.end());
  CHECK(std::find(visited.begin(), visited.end(), app.dropdownNode) != visited.end());
  CHECK(std::find(visited.begin(), visited.end(), app.sliderNode) != visited.end());
  CHECK(std::find(visited.begin(), visited.end(), app.progressNode) != visited.end());

  tabs = tabChildren(app.frame, app.tabsNode);
  REQUIRE_FALSE(tabs.empty());
  clickCenter(app.frame, app.layout, app.router, app.focus, tabs[0]);
  REQUIRE(app.focus.focusedNode() == tabs[0]);
  app.router.dispatch(makeKeyDownEvent(KeyRight), app.frame, app.layout, &app.focus);
  CHECK(app.state.tabIndex == 1);
}

TEST_CASE("Widgets demo regression covers wheel and scrollbar interactions") {
  DemoRegressionApp app;
  initializeDemoState(app.state);
  app.state.tabIndex = 1;
  rebuildDemoUi(app);

  REQUIRE(app.treeNode.isValid());
  PrimeFrame::LayoutOut const* treeOut = app.layout.get(app.treeNode);
  REQUIRE(treeOut != nullptr);

  float centerX = treeOut->absX + treeOut->absW * 0.5f;
  float centerY = treeOut->absY + treeOut->absH * 0.5f;
  app.router.dispatch(makePointerScrollEvent(centerX, centerY, 42.0f),
                      app.frame,
                      app.layout,
                      &app.focus);
  CHECK(app.state.scrollEvents >= 1);
  CHECK(app.state.lastScroll.offset > 0.0f);

  ScrollbarNodes scrollbar = findTreeScrollbarNodes(app.frame, app.layout, app.treeNode);
  REQUIRE(scrollbar.track.isValid());
  REQUIRE(scrollbar.thumb.isValid());

  PrimeFrame::LayoutOut const* trackOut = app.layout.get(scrollbar.track);
  PrimeFrame::LayoutOut const* thumbOut = app.layout.get(scrollbar.thumb);
  REQUIRE(trackOut != nullptr);
  REQUIRE(thumbOut != nullptr);

  float trackX = trackOut->absX + trackOut->absW * 0.5f;
  float trackClickY = trackOut->absY + trackOut->absH * 0.85f;
  float offsetAfterWheel = app.state.lastScroll.offset;
  app.router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 3, trackX, trackClickY),
                      app.frame,
                      app.layout,
                      &app.focus);
  CHECK(app.state.scrollEvents >= 2);
  CHECK(app.state.lastScroll.offset >= offsetAfterWheel);

  float thumbX = thumbOut->absX + thumbOut->absW * 0.5f;
  float thumbY = thumbOut->absY + thumbOut->absH * 0.5f;
  float offsetBeforeDrag = app.state.lastScroll.offset;
  app.router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 4, thumbX, thumbY),
                      app.frame,
                      app.layout,
                      &app.focus);
  app.router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDrag, 4, thumbX, thumbY + 26.0f),
                      app.frame,
                      app.layout,
                      &app.focus);
  app.router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 4, thumbX, thumbY + 26.0f),
                      app.frame,
                      app.layout,
                      &app.focus);
  CHECK(app.state.lastScroll.offset != doctest::Approx(offsetBeforeDrag));
}
