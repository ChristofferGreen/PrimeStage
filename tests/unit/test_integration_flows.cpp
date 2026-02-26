#include "PrimeStage/Ui.h"

#include "PrimeFrame/Events.h"
#include "PrimeFrame/Flatten.h"
#include "PrimeFrame/Focus.h"
#include "PrimeFrame/Frame.h"
#include "PrimeFrame/Layout.h"

#include "third_party/doctest.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr float RootWidth = 420.0f;
constexpr float RootHeight = 560.0f;

constexpr PrimeFrame::ColorToken ColorBackground = 1u;
constexpr PrimeFrame::ColorToken ColorSurface = 2u;
constexpr PrimeFrame::ColorToken ColorFocus = 3u;
constexpr PrimeFrame::ColorToken ColorAccent = 4u;
constexpr PrimeFrame::ColorToken ColorText = 5u;

constexpr PrimeFrame::RectStyleToken StyleBackground = 1u;
constexpr PrimeFrame::RectStyleToken StyleSurface = 2u;
constexpr PrimeFrame::RectStyleToken StyleFocus = 3u;
constexpr PrimeFrame::RectStyleToken StyleAccent = 4u;

constexpr int KeyEnter = 0x28;

PrimeFrame::Color makeColor(float r, float g, float b) {
  PrimeFrame::Color color;
  color.r = r;
  color.g = g;
  color.b = b;
  color.a = 1.0f;
  return color;
}

bool colorClose(PrimeFrame::Color const& lhs, PrimeFrame::Color const& rhs) {
  constexpr float Epsilon = 0.001f;
  return std::abs(lhs.r - rhs.r) <= Epsilon &&
         std::abs(lhs.g - rhs.g) <= Epsilon &&
         std::abs(lhs.b - rhs.b) <= Epsilon &&
         std::abs(lhs.a - rhs.a) <= Epsilon;
}

void configureTheme(PrimeFrame::Frame& frame) {
  PrimeFrame::Theme* theme = frame.getTheme(PrimeFrame::DefaultThemeId);
  REQUIRE(theme != nullptr);

  theme->palette.assign(8u, PrimeFrame::Color{});
  theme->palette[ColorBackground] = makeColor(0.09f, 0.11f, 0.14f);
  theme->palette[ColorSurface] = makeColor(0.18f, 0.22f, 0.28f);
  theme->palette[ColorFocus] = makeColor(0.92f, 0.24f, 0.16f);
  theme->palette[ColorAccent] = makeColor(0.20f, 0.67f, 0.95f);
  theme->palette[ColorText] = makeColor(0.94f, 0.96f, 0.98f);

  theme->rectStyles.assign(8u, PrimeFrame::RectStyle{});
  theme->rectStyles[StyleBackground].fill = ColorBackground;
  theme->rectStyles[StyleSurface].fill = ColorSurface;
  theme->rectStyles[StyleFocus].fill = ColorFocus;
  theme->rectStyles[StyleAccent].fill = ColorAccent;

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

PrimeFrame::Event makePointerEvent(PrimeFrame::EventType type, float x, float y) {
  PrimeFrame::Event event;
  event.type = type;
  event.pointerId = 1;
  event.x = x;
  event.y = y;
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

PrimeFrame::Event makePointerDownAtCenter(PrimeFrame::LayoutOutput const& layout, PrimeFrame::NodeId nodeId) {
  PrimeFrame::LayoutOut const* out = layout.get(nodeId);
  REQUIRE(out != nullptr);
  float centerX = out->absX + out->absW * 0.5f;
  float centerY = out->absY + out->absH * 0.5f;
  return makePointerEvent(PrimeFrame::EventType::PointerDown, centerX, centerY);
}

PrimeFrame::Event makePointerUpAtCenter(PrimeFrame::LayoutOutput const& layout, PrimeFrame::NodeId nodeId) {
  PrimeFrame::LayoutOut const* out = layout.get(nodeId);
  REQUIRE(out != nullptr);
  float centerX = out->absX + out->absW * 0.5f;
  float centerY = out->absY + out->absH * 0.5f;
  return makePointerEvent(PrimeFrame::EventType::PointerUp, centerX, centerY);
}

void clickNodeCenter(PrimeFrame::Frame& frame,
                     PrimeFrame::LayoutOutput const& layout,
                     PrimeFrame::EventRouter& router,
                     PrimeFrame::FocusManager& focus,
                     PrimeFrame::NodeId nodeId) {
  router.dispatch(makePointerDownAtCenter(layout, nodeId), frame, layout, &focus);
  router.dispatch(makePointerUpAtCenter(layout, nodeId), frame, layout, &focus);
}

PrimeFrame::RenderBatch flattenBatch(PrimeFrame::Frame const& frame,
                                     PrimeFrame::LayoutOutput const& layout) {
  PrimeFrame::RenderBatch batch;
  PrimeFrame::flattenToRenderBatch(frame, layout, batch);
  return batch;
}

size_t countVisibleRectColor(PrimeFrame::RenderBatch const& batch, PrimeFrame::Color const& color) {
  size_t count = 0u;
  for (PrimeFrame::DrawCommand const& command : batch.commands) {
    if (command.type != PrimeFrame::CommandType::Rect) {
      continue;
    }
    if (command.rectStyle.opacity <= 0.0f) {
      continue;
    }
    if (colorClose(command.rectStyle.fill, color)) {
      ++count;
    }
  }
  return count;
}

struct PrimitiveSnapshot {
  PrimeFrame::PrimitiveType type = PrimeFrame::PrimitiveType::Rect;
  PrimeFrame::RectStyleToken rectToken = 0;
  std::optional<float> rectOpacity;
};

void collectVisualSnapshot(PrimeFrame::Frame const& frame,
                           PrimeFrame::NodeId nodeId,
                           std::vector<PrimitiveSnapshot>& out) {
  PrimeFrame::Node const* node = frame.getNode(nodeId);
  if (!node) {
    return;
  }
  for (PrimeFrame::PrimitiveId primitiveId : node->primitives) {
    PrimeFrame::Primitive const* primitive = frame.getPrimitive(primitiveId);
    if (!primitive) {
      continue;
    }
    PrimitiveSnapshot snapshot;
    snapshot.type = primitive->type;
    if (primitive->type == PrimeFrame::PrimitiveType::Rect) {
      snapshot.rectToken = primitive->rect.token;
      snapshot.rectOpacity = primitive->rect.overrideStyle.opacity;
    }
    out.push_back(snapshot);
  }
  for (PrimeFrame::NodeId childId : node->children) {
    collectVisualSnapshot(frame, childId, out);
  }
}

std::vector<PrimitiveSnapshot> captureVisualSnapshot(PrimeFrame::Frame const& frame,
                                                     PrimeFrame::NodeId nodeId) {
  std::vector<PrimitiveSnapshot> snapshot;
  collectVisualSnapshot(frame, nodeId, snapshot);
  return snapshot;
}

bool hasVisualDifference(std::vector<PrimitiveSnapshot> const& lhs,
                         std::vector<PrimitiveSnapshot> const& rhs) {
  if (lhs.size() != rhs.size()) {
    return true;
  }
  for (size_t i = 0; i < lhs.size(); ++i) {
    if (lhs[i].type != rhs[i].type) {
      return true;
    }
    if (lhs[i].rectToken != rhs[i].rectToken) {
      return true;
    }
    if (lhs[i].rectOpacity != rhs[i].rectOpacity) {
      return true;
    }
  }
  return false;
}

struct IntegrationApp {
  PrimeFrame::Frame frame;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;
  PrimeStage::WidgetIdentityReconciler widgetIdentity;
  PrimeStage::TextFieldState textFieldState;
  PrimeStage::SelectableTextState selectableState;
  int buttonClicks = 0;
  PrimeFrame::NodeId buttonNode{};
  PrimeFrame::NodeId fieldNode{};
  PrimeFrame::NodeId selectableNode{};
};

constexpr std::string_view IdentityButton = "integration.button";
constexpr std::string_view IdentityField = "integration.field";
constexpr std::string_view IdentitySelectable = "integration.selectable";

bool rebuildIntegrationScene(IntegrationApp& app, bool insertLeadingSpacer) {
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
  contentSpec.gap = 10.0f;
  PrimeStage::UiNode content = root.createVerticalStack(contentSpec);

  if (insertLeadingSpacer) {
    PrimeStage::LabelSpec spacerLabel;
    spacerLabel.text = "Rebuild shift node ids";
    spacerLabel.textStyle = 0u;
    content.createLabel(spacerLabel);
  }

  PrimeStage::ButtonSpec buttonSpec;
  buttonSpec.label = "Save";
  buttonSpec.backgroundStyle = StyleSurface;
  buttonSpec.hoverStyle = StyleAccent;
  buttonSpec.pressedStyle = StyleAccent;
  buttonSpec.focusStyle = StyleFocus;
  buttonSpec.textStyle = 0u;
  buttonSpec.size.preferredWidth = 180.0f;
  buttonSpec.size.preferredHeight = 30.0f;
  buttonSpec.callbacks.onClick = [&app]() { app.buttonClicks += 1; };
  PrimeStage::UiNode button = content.createButton(buttonSpec);
  app.buttonNode = button.nodeId();
  app.widgetIdentity.registerNode(IdentityButton, app.buttonNode);

  PrimeStage::TextFieldSpec fieldSpec;
  fieldSpec.state = &app.textFieldState;
  fieldSpec.backgroundStyle = StyleSurface;
  fieldSpec.selectionStyle = StyleAccent;
  fieldSpec.focusStyle = StyleFocus;
  fieldSpec.textStyle = 0u;
  fieldSpec.size.preferredWidth = 260.0f;
  fieldSpec.size.preferredHeight = 30.0f;
  PrimeStage::UiNode field = content.createTextField(fieldSpec);
  app.fieldNode = field.nodeId();
  app.widgetIdentity.registerNode(IdentityField, app.fieldNode);

  PrimeStage::SelectableTextSpec selectableSpec;
  selectableSpec.state = &app.selectableState;
  selectableSpec.text = "Selectable text should not receive default focus.";
  selectableSpec.textStyle = 0u;
  selectableSpec.selectionStyle = StyleAccent;
  selectableSpec.size.preferredWidth = 260.0f;
  selectableSpec.size.preferredHeight = 34.0f;
  PrimeStage::UiNode selectable = content.createSelectableText(selectableSpec);
  app.selectableNode = selectable.nodeId();
  app.widgetIdentity.registerNode(IdentitySelectable, app.selectableNode);

  app.layout = layoutFrame(app.frame);
  app.focus.updateAfterRebuild(app.frame, app.layout);
  return app.widgetIdentity.restoreFocus(app.focus, app.frame, app.layout);
}

} // namespace

TEST_CASE("PrimeStage integration flow covers click type rebuild and focus retention") {
  IntegrationApp app;
  app.textFieldState.text = "Prime";
  app.textFieldState.cursor = 5u;

  bool restoredInitial = rebuildIntegrationScene(app, false);
  CHECK_FALSE(restoredInitial);

  PrimeFrame::Node const* buttonNode = app.frame.getNode(app.buttonNode);
  PrimeFrame::Node const* fieldNode = app.frame.getNode(app.fieldNode);
  PrimeFrame::Node const* selectableNode = app.frame.getNode(app.selectableNode);
  REQUIRE(buttonNode != nullptr);
  REQUIRE(fieldNode != nullptr);
  REQUIRE(selectableNode != nullptr);
  CHECK(buttonNode->focusable);
  CHECK(fieldNode->focusable);
  CHECK_FALSE(selectableNode->focusable);

  clickNodeCenter(app.frame, app.layout, app.router, app.focus, app.buttonNode);
  CHECK(app.buttonClicks == 1);
  CHECK(app.focus.focusedNode() == app.buttonNode);

  PrimeFrame::Color const focusColor = makeColor(0.92f, 0.24f, 0.16f);
  PrimeFrame::RenderBatch beforeFocusBatch = flattenBatch(app.frame, app.layout);
  CHECK(countVisibleRectColor(beforeFocusBatch, focusColor) > 0u);

  std::vector<PrimitiveSnapshot> beforeFieldFocus = captureVisualSnapshot(app.frame, app.fieldNode);
  clickNodeCenter(app.frame, app.layout, app.router, app.focus, app.fieldNode);
  CHECK(app.focus.focusedNode() == app.fieldNode);

  std::vector<PrimitiveSnapshot> afterFieldFocus = captureVisualSnapshot(app.frame, app.fieldNode);
  CHECK(hasVisualDifference(beforeFieldFocus, afterFieldFocus));

  PrimeFrame::RenderBatch afterFocusBatch = flattenBatch(app.frame, app.layout);
  CHECK(countVisibleRectColor(afterFocusBatch, focusColor) > 0u);

  app.router.dispatch(makeTextInputEvent(" Stage"), app.frame, app.layout, &app.focus);
  CHECK(app.textFieldState.text == "Prime Stage");
  CHECK(app.textFieldState.cursor == 11u);

  PrimeFrame::NodeId previousFieldNode = app.fieldNode;
  bool restoredAfterRebuild = rebuildIntegrationScene(app, true);
  CHECK(restoredAfterRebuild);
  CHECK(app.fieldNode != previousFieldNode);
  CHECK(app.focus.focusedNode() == app.fieldNode);
  CHECK(app.textFieldState.text == "Prime Stage");

  PrimeFrame::RenderBatch rebuiltBatch = flattenBatch(app.frame, app.layout);
  CHECK(countVisibleRectColor(rebuiltBatch, focusColor) > 0u);
}

TEST_CASE("PrimeStage focus contract matrix is explicit for integration widgets") {
  PrimeFrame::Frame frame;
  configureTheme(frame);
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::StackSpec contentSpec;
  contentSpec.size.stretchX = 1.0f;
  contentSpec.size.stretchY = 1.0f;
  contentSpec.padding.left = 12.0f;
  contentSpec.padding.top = 12.0f;
  contentSpec.padding.right = 12.0f;
  contentSpec.padding.bottom = 12.0f;
  contentSpec.gap = 8.0f;
  PrimeStage::UiNode content = root.createVerticalStack(contentSpec);

  PrimeStage::TextFieldState textFieldState;
  textFieldState.text = "matrix";
  PrimeStage::SelectableTextState selectableState;

  PrimeStage::ButtonSpec buttonSpec;
  buttonSpec.label = "Button";
  buttonSpec.backgroundStyle = StyleSurface;
  buttonSpec.focusStyle = StyleFocus;
  buttonSpec.size.preferredWidth = 200.0f;
  buttonSpec.size.preferredHeight = 28.0f;
  PrimeStage::UiNode button = content.createButton(buttonSpec);

  PrimeStage::TextFieldSpec fieldSpec;
  fieldSpec.state = &textFieldState;
  fieldSpec.backgroundStyle = StyleSurface;
  fieldSpec.focusStyle = StyleFocus;
  fieldSpec.textStyle = 0u;
  fieldSpec.size.preferredWidth = 240.0f;
  fieldSpec.size.preferredHeight = 28.0f;
  PrimeStage::UiNode textField = content.createTextField(fieldSpec);

  PrimeStage::ToggleSpec toggleSpec;
  toggleSpec.trackStyle = StyleSurface;
  toggleSpec.knobStyle = StyleAccent;
  toggleSpec.focusStyle = StyleFocus;
  toggleSpec.size.preferredWidth = 56.0f;
  toggleSpec.size.preferredHeight = 24.0f;
  PrimeStage::UiNode toggle = content.createToggle(toggleSpec);

  PrimeStage::CheckboxSpec checkboxSpec;
  checkboxSpec.label = "Checkbox";
  checkboxSpec.boxStyle = StyleSurface;
  checkboxSpec.checkStyle = StyleAccent;
  checkboxSpec.focusStyle = StyleFocus;
  checkboxSpec.textStyle = 0u;
  PrimeStage::UiNode checkbox = content.createCheckbox(checkboxSpec);

  PrimeStage::SliderSpec sliderSpec;
  sliderSpec.trackStyle = StyleSurface;
  sliderSpec.fillStyle = StyleAccent;
  sliderSpec.thumbStyle = StyleAccent;
  sliderSpec.focusStyle = StyleFocus;
  sliderSpec.size.preferredWidth = 220.0f;
  sliderSpec.size.preferredHeight = 18.0f;
  PrimeStage::UiNode slider = content.createSlider(sliderSpec);

  PrimeStage::ProgressBarSpec progressSpec;
  progressSpec.value = 0.5f;
  progressSpec.trackStyle = StyleSurface;
  progressSpec.fillStyle = StyleAccent;
  progressSpec.focusStyle = StyleFocus;
  progressSpec.size.preferredWidth = 220.0f;
  progressSpec.size.preferredHeight = 12.0f;
  PrimeStage::UiNode progress = content.createProgressBar(progressSpec);

  PrimeStage::TableSpec tableSpec;
  tableSpec.size.preferredWidth = 280.0f;
  tableSpec.size.preferredHeight = 96.0f;
  tableSpec.headerHeight = 18.0f;
  tableSpec.headerStyle = StyleSurface;
  tableSpec.rowStyle = StyleBackground;
  tableSpec.rowAltStyle = StyleSurface;
  tableSpec.selectionStyle = StyleAccent;
  tableSpec.dividerStyle = StyleSurface;
  tableSpec.focusStyle = StyleFocus;
  tableSpec.columns = {{"A", 120.0f, 0u, 0u}, {"B", 120.0f, 0u, 0u}};
  tableSpec.rows = {{"1", "2"}, {"3", "4"}};
  PrimeStage::UiNode table = content.createTable(tableSpec);

  PrimeStage::TreeViewSpec treeSpec;
  treeSpec.size.preferredWidth = 280.0f;
  treeSpec.size.preferredHeight = 96.0f;
  treeSpec.rowStyle = StyleBackground;
  treeSpec.rowAltStyle = StyleSurface;
  treeSpec.hoverStyle = StyleAccent;
  treeSpec.selectionStyle = StyleAccent;
  treeSpec.selectionAccentStyle = StyleAccent;
  treeSpec.caretBackgroundStyle = StyleSurface;
  treeSpec.caretLineStyle = StyleSurface;
  treeSpec.connectorStyle = StyleSurface;
  treeSpec.focusStyle = StyleFocus;
  treeSpec.textStyle = 0u;
  treeSpec.selectedTextStyle = 0u;
  treeSpec.nodes = {PrimeStage::TreeNode{"Root"}};
  PrimeStage::UiNode treeView = content.createTreeView(treeSpec);

  PrimeStage::SelectableTextSpec selectableSpec;
  selectableSpec.state = &selectableState;
  selectableSpec.text = "Selectable text is not focusable by default.";
  selectableSpec.textStyle = 0u;
  selectableSpec.selectionStyle = StyleAccent;
  selectableSpec.size.preferredWidth = 260.0f;
  selectableSpec.size.preferredHeight = 30.0f;
  PrimeStage::UiNode selectable = content.createSelectableText(selectableSpec);

  struct FocusMatrixEntry {
    std::string_view name;
    PrimeFrame::NodeId nodeId;
    bool expectFocusable;
    bool expectVisualFocus;
  };

  std::array<FocusMatrixEntry, 9> matrix{{
      {"button", button.nodeId(), true, true},
      {"text_field", textField.nodeId(), true, true},
      {"toggle", toggle.nodeId(), true, true},
      {"checkbox", checkbox.nodeId(), true, true},
      {"slider", slider.nodeId(), true, true},
      {"progress_bar", progress.nodeId(), true, true},
      {"table", table.nodeId(), true, true},
      {"tree_view", treeView.nodeId(), true, true},
      {"selectable_text", selectable.nodeId(), false, false},
  }};

  PrimeFrame::LayoutOutput layout = layoutFrame(frame);
  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;

  for (FocusMatrixEntry const& entry : matrix) {
    INFO(entry.name);
    PrimeFrame::Node const* node = frame.getNode(entry.nodeId);
    REQUIRE(node != nullptr);
    CHECK(node->focusable == entry.expectFocusable);

    focus.clearFocus(frame);
    std::vector<PrimitiveSnapshot> beforeFocus = captureVisualSnapshot(frame, entry.nodeId);
    clickNodeCenter(frame, layout, router, focus, entry.nodeId);

    if (entry.expectFocusable) {
      if (focus.focusedNode() != entry.nodeId) {
        CHECK(focus.focusedNode().isValid());
        CHECK(focus.setFocus(frame, layout, entry.nodeId));
      }
      CHECK(focus.focusedNode() == entry.nodeId);
      if (entry.expectVisualFocus) {
        std::vector<PrimitiveSnapshot> afterFocus = captureVisualSnapshot(frame, entry.nodeId);
        CHECK(hasVisualDifference(beforeFocus, afterFocus));
      }
    } else {
      CHECK(focus.focusedNode() != entry.nodeId);
    }
  }

  focus.clearFocus(frame);
  std::vector<PrimeFrame::NodeId> visitedByTab;
  for (size_t i = 0; i < matrix.size() * 3u; ++i) {
    bool handled = focus.handleTab(frame, layout, true);
    if (!handled) {
      break;
    }
    PrimeFrame::NodeId focused = focus.focusedNode();
    if (!focused.isValid()) {
      continue;
    }
    auto alreadyVisited = std::find(visitedByTab.begin(), visitedByTab.end(), focused);
    if (alreadyVisited == visitedByTab.end()) {
      visitedByTab.push_back(focused);
    }
  }

  std::vector<PrimeFrame::NodeId> expectedFocusable;
  for (FocusMatrixEntry const& entry : matrix) {
    if (entry.expectFocusable) {
      expectedFocusable.push_back(entry.nodeId);
    }
  }

  CHECK(visitedByTab.size() == expectedFocusable.size());
  for (PrimeFrame::NodeId expected : expectedFocusable) {
    auto found = std::find(visitedByTab.begin(), visitedByTab.end(), expected);
    CHECK(found != visitedByTab.end());
  }
  auto selectableFound = std::find(visitedByTab.begin(), visitedByTab.end(), selectable.nodeId());
  CHECK(selectableFound == visitedByTab.end());

  focus.clearFocus(frame);
  clickNodeCenter(frame, layout, router, focus, button.nodeId());
  REQUIRE(focus.focusedNode() == button.nodeId());
  router.dispatch(makeKeyDownEvent(KeyEnter), frame, layout, &focus);
}
