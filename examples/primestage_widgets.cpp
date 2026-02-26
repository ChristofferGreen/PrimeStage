#include "PrimeHost/PrimeHost.h"
#include "PrimeStage/PrimeStage.h"
#include "PrimeStage/Render.h"
#include "PrimeStage/TextSelection.h"

#include "PrimeFrame/Events.h"
#include "PrimeFrame/Focus.h"
#include "PrimeFrame/Layout.h"

#include <algorithm>
#include <array>
#include <chrono>
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

constexpr uint32_t KeyEscape = 0x29u;
constexpr float ScrollLinePixels = 32.0f;

PrimeFrame::Color makeColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255u) {
  auto toFloat = [](uint8_t value) -> float {
    return static_cast<float>(value) / 255.0f;
  };
  return PrimeFrame::Color{toFloat(r), toFloat(g), toFloat(b), toFloat(a)};
}

PrimeStage::Rgba8 toRgba8(PrimeFrame::Color color) {
  auto toByte = [](float value) -> uint8_t {
    float clamped = std::clamp(value, 0.0f, 1.0f);
    return static_cast<uint8_t>(std::lround(clamped * 255.0f));
  };
  return PrimeStage::Rgba8{toByte(color.r), toByte(color.g), toByte(color.b), toByte(color.a)};
}

enum RectStyleId : uint16_t {
  RectPanel = 0,
  RectPanelAlt = 1,
  RectDivider = 2,
  RectButtonBase = 3,
  RectButtonHover = 4,
  RectButtonPressed = 5,
  RectButtonPrimary = 6,
  RectButtonPrimaryHover = 7,
  RectButtonPrimaryPressed = 8,
  RectField = 9,
  RectFieldFocus = 10,
  RectSelection = 11,
  RectToggleOff = 12,
  RectToggleOn = 13,
  RectToggleKnob = 14,
  RectCheckboxBox = 15,
  RectCheckboxCheck = 16,
  RectSliderTrack = 17,
  RectSliderFill = 18,
  RectSliderThumb = 19,
  RectTab = 20,
  RectTabActive = 21,
  RectDropdown = 22,
  RectProgressTrack = 23,
  RectProgressFill = 24,
  RectTableHeader = 25,
  RectTableRow = 26,
  RectTableRowAlt = 27,
  RectTableDivider = 28,
  RectTreeRow = 29,
  RectTreeRowAlt = 30,
  RectTreeHover = 31,
  RectTreeSelection = 32,
  RectTreeSelectionAccent = 33,
  RectTreeCaretBackground = 34,
  RectTreeCaretLine = 35,
  RectScrollTrack = 36,
  RectScrollThumb = 37,
  RectTabBar = 38,
  RectTableRowSelected = 39,
};

enum TextStyleId : uint16_t {
  TextBody = 0,
  TextMuted = 1,
  TextHeading = 2,
  TextSmall = 3,
  TextAccent = 4,
  TextMono = 5,
};

struct DemoState {
  PrimeStage::TextFieldState textField{};
  PrimeStage::SelectableTextState selectableText{};
  bool toggleOn = true;
  bool checkboxChecked = false;
  float sliderValue = 0.35f;
  float progressValue = 0.35f;
  int tabIndex = 0;
  int dropdownIndex = 0;
  int clickCount = 0;
  int tableSelectedRow = -1;
  std::vector<PrimeStage::TreeNode> tree;
  std::string selectableTextContent;
  std::string overlayText;
  std::vector<std::string> dropdownItems;
  std::vector<std::string> tabLabels;
};

enum class RestoreFocusTarget : uint8_t {
  None,
  TextField,
  TreeView,
  Toggle,
  Checkbox,
  Slider,
  Progress,
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
  bool needsRebuild = true;
  bool needsLayout = true;
  bool needsFrame = true;
  RestoreFocusTarget restoreFocusTarget = RestoreFocusTarget::None;
  uint32_t surfaceWidth = 1280u;
  uint32_t surfaceHeight = 720u;
  float surfaceScale = 1.0f;
  uint32_t renderWidth = 0u;
  uint32_t renderHeight = 0u;
  float renderScale = 1.0f;
  PrimeStage::RenderOptions renderOptions{};
  float lastPointerX = 0.0f;
  float lastPointerY = 0.0f;
  PrimeFrame::NodeId textFieldNode{};
  PrimeFrame::NodeId selectableTextNode{};
  PrimeFrame::NodeId treeViewNode{};
  PrimeFrame::NodeId toggleNode{};
  PrimeFrame::NodeId checkboxNode{};
  PrimeFrame::NodeId sliderNode{};
  PrimeFrame::NodeId progressNode{};
};

void applyDemoTheme(PrimeFrame::Frame& frame, PrimeStage::RenderOptions& renderOptions) {
  PrimeFrame::Theme* theme = frame.getTheme(PrimeFrame::DefaultThemeId);
  if (!theme) {
    return;
  }

  const PrimeFrame::Color background = makeColor(12, 16, 22);
  const PrimeFrame::Color panel = makeColor(19, 26, 36);
  const PrimeFrame::Color panelAlt = makeColor(24, 33, 45);
  const PrimeFrame::Color border = makeColor(37, 50, 68);
  const PrimeFrame::Color text = makeColor(230, 238, 248);
  const PrimeFrame::Color textMuted = makeColor(152, 176, 197);
  const PrimeFrame::Color accent = makeColor(77, 210, 255);
  const PrimeFrame::Color accentHover = makeColor(108, 219, 255);
  const PrimeFrame::Color accentPressed = makeColor(42, 184, 242);
  const PrimeFrame::Color success = makeColor(75, 227, 138);
  const PrimeFrame::Color warning = makeColor(243, 201, 105);
  const PrimeFrame::Color danger = makeColor(240, 102, 102);
  const PrimeFrame::Color track = makeColor(42, 58, 80);
  const PrimeFrame::Color trackDark = makeColor(30, 40, 55);
  const PrimeFrame::Color selection = makeColor(35, 66, 94);
  const PrimeFrame::Color toggleOn = makeColor(44, 143, 120);
  const PrimeFrame::Color toggleOff = makeColor(57, 73, 93);
  const PrimeFrame::Color scrollTrack = makeColor(26, 35, 48);
  const PrimeFrame::Color scrollThumb = makeColor(59, 76, 99);
  const PrimeFrame::Color tabBar = makeColor(18, 24, 33);
  const PrimeFrame::Color tabIdle = makeColor(27, 37, 52);
  const PrimeFrame::Color tabActive = makeColor(60, 82, 112);
  const PrimeFrame::Color tableRowSelected = makeColor(36, 74, 110);

  theme->palette = {
      background,
      panel,
      panelAlt,
      border,
      text,
      textMuted,
      accent,
      accentHover,
      accentPressed,
      success,
      warning,
      danger,
      track,
      trackDark,
      selection,
      toggleOn,
      toggleOff,
      scrollTrack,
      scrollThumb,
      tabBar,
      tabIdle,
      tabActive,
      tableRowSelected,
  };

  auto rectStyle = [](PrimeFrame::ColorToken token, float opacity = 1.0f) {
    return PrimeFrame::RectStyle{token, opacity};
  };

  theme->rectStyles = {
      rectStyle(1),   // RectPanel
      rectStyle(2),   // RectPanelAlt
      rectStyle(3),   // RectDivider
      rectStyle(2),   // RectButtonBase
      rectStyle(3),   // RectButtonHover
      rectStyle(0),   // RectButtonPressed
      rectStyle(6),   // RectButtonPrimary
      rectStyle(7),   // RectButtonPrimaryHover
      rectStyle(8),   // RectButtonPrimaryPressed
      rectStyle(1),   // RectField
      rectStyle(6),   // RectFieldFocus
      rectStyle(14),  // RectSelection
      rectStyle(16),  // RectToggleOff
      rectStyle(15),  // RectToggleOn
      rectStyle(4),   // RectToggleKnob
      rectStyle(2),   // RectCheckboxBox
      rectStyle(6),   // RectCheckboxCheck
      rectStyle(12),  // RectSliderTrack
      rectStyle(6),   // RectSliderFill
      rectStyle(4),   // RectSliderThumb
      rectStyle(20),  // RectTab
      rectStyle(21),  // RectTabActive
      rectStyle(2),   // RectDropdown
      rectStyle(12),  // RectProgressTrack
      rectStyle(6),   // RectProgressFill
      rectStyle(1),   // RectTableHeader
      rectStyle(0),   // RectTableRow
      rectStyle(2),   // RectTableRowAlt
      rectStyle(3),   // RectTableDivider
      rectStyle(0),   // RectTreeRow
      rectStyle(2),   // RectTreeRowAlt
      rectStyle(3),   // RectTreeHover
      rectStyle(14),  // RectTreeSelection
      rectStyle(6),   // RectTreeSelectionAccent
      rectStyle(3),   // RectTreeCaretBackground
      rectStyle(4),   // RectTreeCaretLine
      rectStyle(17),  // RectScrollTrack
      rectStyle(18),  // RectScrollThumb
      rectStyle(19),  // RectTabBar
      rectStyle(22, 0.5f),  // RectTableRowSelected
  };

  auto textStyle = [](float size, float weight, PrimeFrame::ColorToken color, float lineHeight = 0.0f) {
    PrimeFrame::TextStyle style;
    style.size = size;
    style.weight = weight;
    style.color = color;
    style.lineHeight = lineHeight;
    return style;
  };

  theme->textStyles = {
      textStyle(14.0f, 450.0f, 4),  // TextBody
      textStyle(12.0f, 400.0f, 5),  // TextMuted
      textStyle(18.0f, 600.0f, 4),  // TextHeading
      textStyle(11.0f, 400.0f, 5),  // TextSmall
      textStyle(13.0f, 600.0f, 6),  // TextAccent
      textStyle(12.0f, 500.0f, 4),  // TextMono
  };

  renderOptions.clear = true;
  renderOptions.clearColor = toRgba8(background);
  renderOptions.roundedCorners = true;
}

PrimeStage::UiNode createSection(PrimeStage::UiNode parent,
                                 std::string_view title) {
  PrimeStage::PanelSpec panel;
  panel.size.stretchX = 1.0f;
  panel.layout = PrimeFrame::LayoutType::VerticalStack;
  panel.padding.left = 12.0f;
  panel.padding.top = 10.0f;
  panel.padding.right = 12.0f;
  panel.padding.bottom = 12.0f;
  panel.gap = 8.0f;
  panel.rectStyle = RectPanelAlt;
  panel.visible = true;
  PrimeStage::UiNode section = parent.createPanel(panel);

  PrimeStage::TextLineSpec header;
  header.text = title;
  header.textStyle = TextHeading;
  header.align = PrimeFrame::TextAlign::Start;
  header.visible = true;
  section.createTextLine(header);

  PrimeStage::DividerSpec divider;
  divider.rectStyle = RectDivider;
  divider.size.preferredHeight = 1.0f;
  divider.size.stretchX = 1.0f;
  divider.visible = true;
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

void appendNodeEventCallback(PrimeFrame::Frame& frame,
                             PrimeFrame::NodeId nodeId,
                             std::function<bool(PrimeFrame::Event const&)> onEvent) {
  PrimeFrame::Node* node = frame.getNode(nodeId);
  if (!node) {
    return;
  }
  if (node->callbacks != PrimeFrame::InvalidCallbackId) {
    PrimeFrame::Callback* callback = frame.getCallback(node->callbacks);
    if (!callback) {
      return;
    }
    auto previous = callback->onEvent;
    callback->onEvent = [handler = std::move(onEvent), previous = std::move(previous)](
                            PrimeFrame::Event const& event) -> bool {
      if (handler && handler(event)) {
        return true;
      }
      if (previous) {
        return previous(event);
      }
      return false;
    };
    return;
  }

  PrimeFrame::Callback callback;
  callback.onEvent = std::move(onEvent);
  node->callbacks = frame.addCallback(std::move(callback));
}

std::optional<std::string_view> textFromSpan(const PrimeHost::EventBatch& batch,
                                             PrimeHost::TextSpan span) {
  if (span.length == 0u) {
    return std::string_view{};
  }
  if (span.offset + span.length > batch.textBytes.size()) {
    return std::nullopt;
  }
  return std::string_view(batch.textBytes.data() + span.offset, span.length);
}

void initializeState(DemoState& state) {
  state.textField.text = "Editable text field";
  state.selectableTextContent =
      "Selectable text supports drag selection, arrow keys, and clipboard shortcuts.";
  state.overlayText =
      "This overlay draws selection rectangles based on precomputed text layout.";
  state.dropdownItems = {"Preview", "Edit", "Export", "Publish"};
  state.tabLabels = {"Overview", "Assets", "Settings"};
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
  PrimeFrame::NodeId focusedNode = app.focus.focusedNode();
  app.restoreFocusTarget = RestoreFocusTarget::None;
  if (focusedNode == app.textFieldNode) {
    app.restoreFocusTarget = RestoreFocusTarget::TextField;
  } else if (focusedNode == app.treeViewNode) {
    app.restoreFocusTarget = RestoreFocusTarget::TreeView;
  } else if (focusedNode == app.toggleNode) {
    app.restoreFocusTarget = RestoreFocusTarget::Toggle;
  } else if (focusedNode == app.checkboxNode) {
    app.restoreFocusTarget = RestoreFocusTarget::Checkbox;
  } else if (focusedNode == app.sliderNode) {
    app.restoreFocusTarget = RestoreFocusTarget::Slider;
  } else if (focusedNode == app.progressNode) {
    app.restoreFocusTarget = RestoreFocusTarget::Progress;
  }

  app.frame = PrimeFrame::Frame();
  app.router.clearAllCaptures();
  applyDemoTheme(app.frame, app.renderOptions);
  app.textFieldNode = PrimeFrame::NodeId{};
  app.selectableTextNode = PrimeFrame::NodeId{};
  app.treeViewNode = PrimeFrame::NodeId{};
  app.toggleNode = PrimeFrame::NodeId{};
  app.checkboxNode = PrimeFrame::NodeId{};
  app.sliderNode = PrimeFrame::NodeId{};
  app.progressNode = PrimeFrame::NodeId{};

  PrimeFrame::NodeId rootId = app.frame.createNode();
  app.frame.addRoot(rootId);
  if (PrimeFrame::Node* rootNode = app.frame.getNode(rootId)) {
    rootNode->layout = PrimeFrame::LayoutType::Overlay;
    rootNode->visible = true;
    rootNode->clipChildren = true;
    rootNode->hitTestVisible = false;
  }

  PrimeStage::UiNode root(app.frame, rootId);

  PrimeStage::PanelSpec background;
  background.size.stretchX = 1.0f;
  background.size.stretchY = 1.0f;
  background.rectStyle = RectPanel;
  background.visible = true;
  PrimeStage::UiNode backgroundNode = root.createPanel(background);
  backgroundNode.setHitTestVisible(false);

  PrimeStage::StackSpec contentSpec;
  contentSpec.size.stretchX = 1.0f;
  contentSpec.size.stretchY = 0.0f;
  contentSpec.padding.left = 20.0f;
  contentSpec.padding.top = 18.0f;
  contentSpec.padding.right = 20.0f;
  contentSpec.padding.bottom = 18.0f;
  contentSpec.gap = 14.0f;
  contentSpec.clipChildren = true;
  contentSpec.visible = true;

  PrimeStage::UiNode content = root.createVerticalStack(contentSpec);

  PrimeStage::LabelSpec title;
  title.text = "PrimeStage Widget Gallery";
  title.textStyle = TextHeading;
  title.size.stretchX = 1.0f;
  title.visible = true;
  content.createLabel(title);

  PrimeStage::ParagraphSpec intro;
  intro.text =
      "This sample renders every PrimeStage widget in a PrimeHost window and pipes full "
      "input events through PrimeFrame's router.";
  intro.textStyle = TextMuted;
  intro.wrap = PrimeFrame::WrapMode::Word;
  intro.maxWidth = 720.0f;
  intro.size.stretchX = 1.0f;
  intro.visible = true;
  content.createParagraph(intro);

  PrimeStage::DividerSpec divider;
  divider.rectStyle = RectDivider;
  divider.size.preferredHeight = 1.0f;
  divider.size.stretchX = 1.0f;
  divider.visible = true;
  content.createDivider(divider);

  int tabCount = static_cast<int>(app.state.tabLabels.size());
  int tabIndex = tabCount > 0 ? std::clamp(app.state.tabIndex, 0, tabCount - 1) : 0;
  app.state.tabIndex = tabIndex;

  std::vector<std::string_view> tabViews;
  tabViews.reserve(app.state.tabLabels.size());
  for (auto const& labelText : app.state.tabLabels) {
    tabViews.push_back(labelText);
  }

  PrimeStage::PanelSpec tabsBarSpec;
  tabsBarSpec.layout = PrimeFrame::LayoutType::Overlay;
  tabsBarSpec.size.stretchX = 1.0f;
  tabsBarSpec.size.preferredHeight = 40.0f;
  tabsBarSpec.padding.left = 8.0f;
  tabsBarSpec.padding.right = 8.0f;
  tabsBarSpec.padding.top = 8.0f;
  tabsBarSpec.padding.bottom = 8.0f;
  tabsBarSpec.rectStyle = RectTabBar;
  tabsBarSpec.visible = true;
  PrimeStage::UiNode tabsBar = content.createPanel(tabsBarSpec);

  PrimeStage::TabsSpec tabsSpec;
  tabsSpec.labels = tabViews;
  tabsSpec.selectedIndex = tabIndex;
  tabsSpec.tabPaddingX = 12.0f;
  tabsSpec.tabPaddingY = 5.0f;
  tabsSpec.gap = 6.0f;
  tabsSpec.tabStyle = RectTab;
  tabsSpec.activeTabStyle = RectTabActive;
  tabsSpec.textStyle = TextMuted;
  tabsSpec.activeTextStyle = TextAccent;
  tabsSpec.activeTextStyleOverride.weight = 600.0f;
  tabsSpec.size.stretchX = 1.0f;
  tabsSpec.size.preferredHeight = 28.0f;
  tabsSpec.visible = true;
  PrimeStage::UiNode tabsNode = tabsBar.createTabs(tabsSpec);
  if (PrimeFrame::Node* rowNode = app.frame.getNode(tabsNode.nodeId())) {
    for (size_t i = 0; i < rowNode->children.size(); ++i) {
      PrimeFrame::NodeId tabId = rowNode->children[i];
      appendNodeEventCallback(
          app.frame,
          tabId,
          [&app, index = static_cast<int>(i)](PrimeFrame::Event const& event) -> bool {
            if (event.type == PrimeFrame::EventType::PointerDown) {
              app.state.tabIndex = index;
              app.needsRebuild = true;
              app.needsFrame = true;
              return true;
            }
            return false;
          });
    }
  }

  PrimeStage::LabelSpec pageTitle;
  pageTitle.text = tabCount > 0 ? tabViews[static_cast<size_t>(tabIndex)] : "Page";
  pageTitle.textStyle = TextHeading;
  pageTitle.visible = true;
  content.createLabel(pageTitle);

  PrimeStage::PanelSpec pagePanel;
  pagePanel.layout = PrimeFrame::LayoutType::VerticalStack;
  pagePanel.size.stretchX = 1.0f;
  pagePanel.size.stretchY = 1.0f;
  pagePanel.padding.left = 18.0f;
  pagePanel.padding.top = 18.0f;
  pagePanel.padding.right = 18.0f;
  pagePanel.padding.bottom = 18.0f;
  pagePanel.gap = 10.0f;
  pagePanel.rectStyle = RectPanelAlt;
  pagePanel.visible = true;
  PrimeStage::UiNode pageBody = content.createPanel(pagePanel);

  PrimeStage::ParagraphSpec pageCopy;
  pageCopy.textStyle = TextBody;
  pageCopy.wrap = PrimeFrame::WrapMode::Word;
  pageCopy.maxWidth = 520.0f;
  pageCopy.visible = true;
  switch (tabIndex) {
    case 0:
      pageCopy.text = "Overview page. Click a tab to switch to a different page.";
      break;
    case 1:
      pageCopy.text = "Assets page. Project tables and tree views live here.";
      break;
    case 2:
      pageCopy.text = "Settings page. Editable inputs and sliders.";
      break;
    default:
      pageCopy.text = "Page content.";
      break;
  }
  pageBody.createParagraph(pageCopy);

  switch (tabIndex) {
    case 0: {
      PrimeStage::UiNode basics = createSection(pageBody, "Basics");

      PrimeStage::TextLineSpec line;
      line.text = "TextLine: single-line aligned text";
      line.textStyle = TextBody;
      line.visible = true;
      basics.createTextLine(line);

      PrimeStage::LabelSpec label;
      label.text = "Label: lightweight label widget";
      label.textStyle = TextBody;
      label.size.stretchX = 1.0f;
      label.visible = true;
      basics.createLabel(label);

      PrimeStage::ParagraphSpec paragraph;
      paragraph.text =
          "Paragraph: multi-line text with wrapping. The layout engine measures the text and "
          "allocates height accordingly.";
      paragraph.textStyle = TextMuted;
      paragraph.wrap = PrimeFrame::WrapMode::Word;
      paragraph.maxWidth = 520.0f;
      paragraph.visible = true;
      basics.createParagraph(paragraph);

      PrimeStage::SpacerSpec spacer;
      spacer.size.preferredHeight = 6.0f;
      spacer.visible = true;
      basics.createSpacer(spacer);

      PrimeStage::UiNode actions = createSection(pageBody, "Buttons + Toggles");

      PrimeStage::StackSpec buttonRowSpec;
      buttonRowSpec.gap = 12.0f;
      buttonRowSpec.visible = true;
      PrimeStage::UiNode buttonRow = actions.createHorizontalStack(buttonRowSpec);

      PrimeStage::ButtonSpec primaryButton;
      primaryButton.label = "Primary Action";
      primaryButton.backgroundStyle = RectButtonPrimary;
      primaryButton.hoverStyle = RectButtonPrimaryHover;
      primaryButton.pressedStyle = RectButtonPrimaryPressed;
      primaryButton.focusStyle = RectFieldFocus;
      primaryButton.textStyle = TextBody;
      primaryButton.size.preferredWidth = 160.0f;
      primaryButton.size.preferredHeight = 32.0f;
      primaryButton.callbacks.onClick = [&app]() {
        app.state.clickCount += 1;
        app.needsRebuild = true;
        app.needsFrame = true;
      };
      buttonRow.createButton(primaryButton);

      PrimeStage::ButtonSpec secondaryButton;
      secondaryButton.label = "Secondary";
      secondaryButton.backgroundStyle = RectButtonBase;
      secondaryButton.hoverStyle = RectButtonHover;
      secondaryButton.pressedStyle = RectButtonPressed;
      secondaryButton.focusStyle = RectFieldFocus;
      secondaryButton.textStyle = TextBody;
      secondaryButton.size.preferredWidth = 120.0f;
      secondaryButton.size.preferredHeight = 32.0f;
      secondaryButton.callbacks.onClick = [&app]() {
        app.state.toggleOn = !app.state.toggleOn;
        app.needsRebuild = true;
        app.needsFrame = true;
      };
      buttonRow.createButton(secondaryButton);

      PrimeStage::TextLineSpec clickLabel;
      std::string clickText = "Clicks: " + std::to_string(app.state.clickCount);
      clickLabel.text = clickText;
      clickLabel.textStyle = TextMuted;
      clickLabel.visible = true;
      actions.createTextLine(clickLabel);

      PrimeStage::StackSpec toggleRowSpec;
      toggleRowSpec.gap = 16.0f;
      toggleRowSpec.visible = true;
      PrimeStage::UiNode toggleRow = actions.createHorizontalStack(toggleRowSpec);

      PrimeStage::ToggleSpec toggleSpec;
      toggleSpec.on = app.state.toggleOn;
      toggleSpec.trackStyle = app.state.toggleOn ? RectToggleOn : RectToggleOff;
      toggleSpec.knobStyle = RectToggleKnob;
      toggleSpec.focusStyle = RectFieldFocus;
      toggleSpec.size.preferredWidth = 46.0f;
      toggleSpec.size.preferredHeight = 22.0f;
      PrimeStage::UiNode toggleNode = toggleRow.createToggle(toggleSpec);
      app.toggleNode = toggleNode.nodeId();
      appendNodeEventCallback(app.frame, toggleNode.nodeId(), [&app](PrimeFrame::Event const& event) -> bool {
        if (event.type == PrimeFrame::EventType::PointerDown) {
          app.state.toggleOn = !app.state.toggleOn;
          app.needsRebuild = true;
          app.needsFrame = true;
          return true;
        }
        return false;
      });

      PrimeStage::CheckboxSpec checkboxSpec;
      checkboxSpec.label = "Enable notifications";
      checkboxSpec.checked = app.state.checkboxChecked;
      checkboxSpec.boxStyle = RectCheckboxBox;
      checkboxSpec.checkStyle = RectCheckboxCheck;
      checkboxSpec.focusStyle = RectFieldFocus;
      checkboxSpec.textStyle = TextBody;
      PrimeStage::UiNode checkboxNode = toggleRow.createCheckbox(checkboxSpec);
      app.checkboxNode = checkboxNode.nodeId();
      appendNodeEventCallback(app.frame, checkboxNode.nodeId(), [&app](PrimeFrame::Event const& event) -> bool {
        if (event.type == PrimeFrame::EventType::PointerDown) {
          app.state.checkboxChecked = !app.state.checkboxChecked;
          app.needsRebuild = true;
          app.needsFrame = true;
          return true;
        }
        return false;
      });
      break;
    }
    case 1: {
      PrimeStage::UiNode tableSection = createSection(pageBody, "Table");

      PrimeStage::TableSpec table;
      table.headerStyle = RectTableHeader;
      table.rowStyle = RectTableRow;
      table.rowAltStyle = RectTableRowAlt;
      table.selectionStyle = RectTableRowSelected;
      table.dividerStyle = RectTableDivider;
      table.focusStyle = RectFieldFocus;
      table.selectedRow = app.state.tableSelectedRow;
      table.headerPaddingX = 12.0f;
      table.cellPaddingX = 12.0f;
      table.rowHeight = 24.0f;
      table.columns = {
          {"Asset", 160.0f, TextMuted, TextBody},
          {"Type", 110.0f, TextMuted, TextBody},
          {"Size", 90.0f, TextMuted, TextBody},
      };
      table.rows = {
          {"icons.png", "Texture", "512 KB"},
          {"theme.ogg", "Audio", "3.1 MB"},
          {"ui.vert", "Shader", "14 KB"},
      };
      table.callbacks.onRowClicked = [&app](PrimeStage::TableRowInfo const& info) {
        app.state.tableSelectedRow = info.rowIndex;
        app.needsFrame = true;
      };
      tableSection.createTable(table);

      PrimeStage::UiNode treeSection = createSection(pageBody, "Tree View");

      PrimeStage::TreeViewSpec treeSpec;
      treeSpec.nodes = app.state.tree;
      treeSpec.rowStyle = RectTreeRow;
      treeSpec.rowAltStyle = RectTreeRowAlt;
      treeSpec.hoverStyle = RectTreeHover;
      treeSpec.selectionStyle = RectTreeSelection;
      treeSpec.selectionAccentStyle = RectTreeSelectionAccent;
      treeSpec.caretBackgroundStyle = RectTreeCaretBackground;
      treeSpec.caretLineStyle = RectTreeCaretLine;
      treeSpec.connectorStyle = RectDivider;
      treeSpec.focusStyle = RectFieldFocus;
      treeSpec.textStyle = TextBody;
      treeSpec.selectedTextStyle = TextAccent;
      treeSpec.scrollBar.trackStyle = RectScrollTrack;
      treeSpec.scrollBar.thumbStyle = RectScrollThumb;
      treeSpec.scrollBar.trackHoverOpacity = 0.85f;
      treeSpec.scrollBar.thumbHoverOpacity = 0.9f;
      treeSpec.keyboardNavigation = true;
      treeSpec.size.preferredWidth = 440.0f;
      treeSpec.size.preferredHeight = 180.0f;
      treeSpec.callbacks.onSelectionChanged = [&app](PrimeStage::TreeViewRowInfo const& info) {
        clearTreeSelection(app.state.tree);
        if (PrimeStage::TreeNode* node = findTreeNode(app.state.tree, info.path)) {
          node->selected = true;
        }
        app.needsRebuild = true;
        app.needsFrame = true;
      };
      treeSpec.callbacks.onExpandedChanged = [&app](PrimeStage::TreeViewRowInfo const& info, bool expanded) {
        if (PrimeStage::TreeNode* node = findTreeNode(app.state.tree, info.path)) {
          node->expanded = expanded;
        }
        app.needsRebuild = true;
        app.needsFrame = true;
      };
      PrimeStage::UiNode treeNode = treeSection.createTreeView(treeSpec);
      app.treeViewNode = treeNode.nodeId();
      break;
    }
    case 2: {
      PrimeStage::UiNode textInputs = createSection(pageBody, "Text Inputs");

      PrimeStage::TextFieldClipboard textClipboard;
      textClipboard.setText = [&app](std::string_view text) {
        if (app.host) {
          app.host->setClipboardText(text);
        }
      };
      textClipboard.getText = [&app]() -> std::string {
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

      PrimeStage::TextFieldSpec fieldSpec;
      fieldSpec.state = &app.state.textField;
      fieldSpec.placeholder = "Type to edit";
      fieldSpec.backgroundStyle = RectField;
      fieldSpec.focusStyle = RectFieldFocus;
      fieldSpec.textStyle = TextBody;
      fieldSpec.placeholderStyle = TextMuted;
      fieldSpec.cursorStyle = RectButtonPrimary;
      fieldSpec.selectionStyle = RectSelection;
      fieldSpec.clipboard = textClipboard;
      fieldSpec.size.preferredWidth = 360.0f;
      fieldSpec.size.preferredHeight = 28.0f;
      fieldSpec.callbacks.onStateChanged = [&app]() {
        app.needsRebuild = true;
        app.needsFrame = true;
      };
      fieldSpec.callbacks.onTextChanged = [&app](std::string_view) {
        app.needsRebuild = true;
        app.needsFrame = true;
      };
      fieldSpec.callbacks.onCursorHintChanged = [&app](PrimeStage::CursorHint hint) {
        if (app.host && app.surfaceId.isValid()) {
          app.host->setCursorShape(app.surfaceId, cursorShapeForHint(hint));
        }
      };
      fieldSpec.callbacks.onRequestBlur = [&app]() {
        app.focus.clearFocus(app.frame);
        app.needsRebuild = true;
        app.needsFrame = true;
      };
      PrimeStage::UiNode fieldNode = textInputs.createTextField(fieldSpec);
      app.textFieldNode = fieldNode.nodeId();

      PrimeStage::SelectableTextClipboard selectableClipboard;
      selectableClipboard.setText = [&app](std::string_view text) {
        if (app.host) {
          app.host->setClipboardText(text);
        }
      };

      PrimeStage::SelectableTextSpec selectableSpec;
      selectableSpec.state = &app.state.selectableText;
      selectableSpec.text = app.state.selectableTextContent;
      selectableSpec.textStyle = TextBody;
      selectableSpec.selectionStyle = RectSelection;
      selectableSpec.focusStyle = RectFieldFocus;
      selectableSpec.clipboard = selectableClipboard;
      selectableSpec.maxWidth = 420.0f;
      selectableSpec.size.preferredWidth = 420.0f;
      selectableSpec.visible = true;
      selectableSpec.callbacks.onStateChanged = [&app]() {
        app.needsRebuild = true;
        app.needsFrame = true;
      };
      selectableSpec.callbacks.onCursorHintChanged = [&app](PrimeStage::CursorHint hint) {
        if (app.host && app.surfaceId.isValid()) {
          app.host->setCursorShape(app.surfaceId, cursorShapeForHint(hint));
        }
      };
      PrimeStage::UiNode selectableNode = textInputs.createSelectableText(selectableSpec);
      app.selectableTextNode = selectableNode.nodeId();

      PrimeStage::UiNode navigation = createSection(pageBody, "Dropdown");

      std::string_view dropdownLabel = app.state.dropdownItems.empty()
                                           ? std::string_view("(empty)")
                                           : std::string_view(app.state.dropdownItems[app.state.dropdownIndex]);
      PrimeStage::DropdownSpec dropdownSpec;
      dropdownSpec.label = dropdownLabel;
      dropdownSpec.indicator = "v";
      dropdownSpec.backgroundStyle = RectDropdown;
      dropdownSpec.textStyle = TextBody;
      dropdownSpec.indicatorStyle = TextMuted;
      dropdownSpec.size.preferredWidth = 200.0f;
      dropdownSpec.size.preferredHeight = 28.0f;
      PrimeStage::UiNode dropdownNode = navigation.createDropdown(dropdownSpec);
      appendNodeEventCallback(app.frame, dropdownNode.nodeId(), [&app](PrimeFrame::Event const& event) -> bool {
        if (event.type == PrimeFrame::EventType::PointerDown) {
          if (!app.state.dropdownItems.empty()) {
            app.state.dropdownIndex = (app.state.dropdownIndex + 1) %
                                      static_cast<int>(app.state.dropdownItems.size());
            app.needsRebuild = true;
            app.needsFrame = true;
          }
          return true;
        }
        return false;
      });

      PrimeStage::UiNode ranges = createSection(pageBody, "Sliders + Progress");

      PrimeStage::SliderSpec sliderSpec;
      sliderSpec.value = app.state.sliderValue;
      sliderSpec.trackStyle = RectSliderTrack;
      sliderSpec.fillStyle = RectSliderFill;
      sliderSpec.thumbStyle = RectSliderThumb;
      sliderSpec.focusStyle = RectFieldFocus;
      sliderSpec.size.preferredWidth = 260.0f;
      sliderSpec.callbacks.onValueChanged = [&app](float value) {
        app.state.sliderValue = value;
        app.state.progressValue = value;
        app.needsRebuild = true;
        app.needsFrame = true;
      };
      PrimeStage::UiNode sliderNode = ranges.createSlider(sliderSpec);
      app.sliderNode = sliderNode.nodeId();

      PrimeStage::ProgressBarSpec progressSpec;
      progressSpec.value = app.state.progressValue;
      progressSpec.trackStyle = RectProgressTrack;
      progressSpec.fillStyle = RectProgressFill;
      progressSpec.focusStyle = RectFieldFocus;
      progressSpec.size.preferredWidth = 260.0f;
      progressSpec.size.preferredHeight = 12.0f;
      PrimeStage::UiNode progressNode = ranges.createProgressBar(progressSpec);
      app.progressNode = progressNode.nodeId();
      break;
    }
    default: {
      PrimeStage::ParagraphSpec emptyCopy;
      emptyCopy.text = "No widgets for this tab.";
      emptyCopy.textStyle = TextBody;
      emptyCopy.visible = true;
      pageBody.createParagraph(emptyCopy);
      break;
    }
  }

  app.needsRebuild = false;
  app.needsLayout = true;
}

void updateLayoutIfNeeded(DemoApp& app) {
  if (!app.needsLayout) {
    return;
  }
  PrimeFrame::LayoutOptions options;
  float scale = app.renderScale > 0.0f ? app.renderScale
                                       : (app.surfaceScale > 0.0f ? app.surfaceScale : 1.0f);
  uint32_t widthPx = app.renderWidth > 0u ? app.renderWidth : app.surfaceWidth;
  uint32_t heightPx = app.renderHeight > 0u ? app.renderHeight : app.surfaceHeight;
  options.rootWidth = static_cast<float>(widthPx) / scale;
  options.rootHeight = static_cast<float>(heightPx) / scale;
  app.layoutEngine.layout(app.frame, app.layout, options);
  app.focus.updateAfterRebuild(app.frame, app.layout);

  PrimeFrame::NodeId restoreNode{};
  switch (app.restoreFocusTarget) {
    case RestoreFocusTarget::TextField:
      restoreNode = app.textFieldNode;
      break;
    case RestoreFocusTarget::TreeView:
      restoreNode = app.treeViewNode;
      break;
    case RestoreFocusTarget::Toggle:
      restoreNode = app.toggleNode;
      break;
    case RestoreFocusTarget::Checkbox:
      restoreNode = app.checkboxNode;
      break;
    case RestoreFocusTarget::Slider:
      restoreNode = app.sliderNode;
      break;
    case RestoreFocusTarget::Progress:
      restoreNode = app.progressNode;
      break;
    case RestoreFocusTarget::None:
      break;
  }

  if (restoreNode.isValid()) {
    app.focus.setFocus(app.frame, app.layout, restoreNode);
  } else if (app.state.tabIndex == 1 && app.treeViewNode.isValid()) {
    app.focus.setFocus(app.frame, app.layout, app.treeViewNode);
  }
  app.restoreFocusTarget = RestoreFocusTarget::None;
  app.needsLayout = false;
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
    rebuildUi(app);
    updateLayoutIfNeeded(app);
    bool ok = PrimeStage::renderFrameToPng(app.frame, app.layout, *snapshotPath, app.renderOptions);
    if (!ok) {
      std::cerr << "Failed to render snapshot to " << *snapshotPath << "\n";
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
    if (app.needsRebuild) {
      rebuildUi(app);
    }
    auto bufferResult = app.host->acquireFrameBuffer(target);
    if (!bufferResult) {
      return;
    }
    PrimeHost::FrameBuffer buffer = bufferResult.value();
    app.renderWidth = buffer.size.width;
    app.renderHeight = buffer.size.height;
    app.renderScale = buffer.scale > 0.0f ? buffer.scale : app.renderScale;
    app.surfaceScale = app.renderScale;
    updateLayoutIfNeeded(app);
    PrimeStage::RenderTarget targetBuffer;
    targetBuffer.pixels = buffer.pixels;
    targetBuffer.width = buffer.size.width;
    targetBuffer.height = buffer.size.height;
    targetBuffer.stride = buffer.stride;
    targetBuffer.scale = buffer.scale;
    PrimeStage::renderFrameToTarget(app.frame, app.layout, targetBuffer, app.renderOptions);
    app.host->presentFrameBuffer(target, buffer);
    app.needsFrame = false;
  };
  app.host->setCallbacks(callbacks);

  rebuildUi(app);
  app.needsFrame = true;
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
        if (auto* pointer = std::get_if<PrimeHost::PointerEvent>(input)) {
          PrimeFrame::Event frameEvent;
          frameEvent.pointerId = static_cast<int>(pointer->pointerId);
          frameEvent.x = static_cast<float>(pointer->x);
          frameEvent.y = static_cast<float>(pointer->y);
          app.lastPointerX = frameEvent.x;
          app.lastPointerY = frameEvent.y;
          switch (pointer->phase) {
            case PrimeHost::PointerPhase::Down:
              frameEvent.type = PrimeFrame::EventType::PointerDown;
              break;
            case PrimeHost::PointerPhase::Move:
              frameEvent.type = PrimeFrame::EventType::PointerMove;
              break;
            case PrimeHost::PointerPhase::Up:
              frameEvent.type = PrimeFrame::EventType::PointerUp;
              break;
            case PrimeHost::PointerPhase::Cancel:
              frameEvent.type = PrimeFrame::EventType::PointerCancel;
              break;
          }
          if (dispatchFrameEvent(app, frameEvent)) {
            app.needsFrame = true;
          }
          bypassCap = true;
        } else if (auto* key = std::get_if<PrimeHost::KeyEvent>(input)) {
          if (key->pressed && key->keyCode == KeyEscape) {
            running = false;
            continue;
          }
          PrimeFrame::Event frameEvent;
          frameEvent.type = key->pressed ? PrimeFrame::EventType::KeyDown
                                         : PrimeFrame::EventType::KeyUp;
          frameEvent.key = static_cast<int>(key->keyCode);
          frameEvent.modifiers = key->modifiers;
          if (dispatchFrameEvent(app, frameEvent)) {
            app.needsFrame = true;
          }
        } else if (auto* text = std::get_if<PrimeHost::TextEvent>(input)) {
          auto view = textFromSpan(batch, text->text);
          if (view) {
            PrimeFrame::Event frameEvent;
            frameEvent.type = PrimeFrame::EventType::TextInput;
            frameEvent.text = std::string(*view);
            if (dispatchFrameEvent(app, frameEvent)) {
              app.needsFrame = true;
            }
          }
        } else if (auto* scroll = std::get_if<PrimeHost::ScrollEvent>(input)) {
          PrimeFrame::Event frameEvent;
          frameEvent.type = PrimeFrame::EventType::PointerScroll;
          frameEvent.x = app.lastPointerX;
          frameEvent.y = app.lastPointerY;
          float deltaScale = scroll->isLines ? ScrollLinePixels : 1.0f;
          frameEvent.scrollX = scroll->deltaX * deltaScale;
          frameEvent.scrollY = scroll->deltaY * deltaScale;
          if (dispatchFrameEvent(app, frameEvent)) {
            app.needsFrame = true;
          }
          bypassCap = true;
        }
      } else if (auto* resize = std::get_if<PrimeHost::ResizeEvent>(&event.payload)) {
        app.surfaceWidth = resize->width;
        app.surfaceHeight = resize->height;
        app.surfaceScale = resize->scale > 0.0f ? resize->scale : 1.0f;
        app.needsRebuild = true;
        app.needsFrame = true;
        bypassCap = true;
      } else if (auto* lifecycle = std::get_if<PrimeHost::LifecycleEvent>(&event.payload)) {
        if (lifecycle->phase == PrimeHost::LifecyclePhase::Destroyed) {
          running = false;
        }
      }
    }

    if (app.needsRebuild) {
      rebuildUi(app);
    }

    if (app.needsFrame) {
      app.host->requestFrame(app.surfaceId, bypassCap);
    }
  }

  app.host->destroySurface(app.surfaceId);
  return 0;
}
