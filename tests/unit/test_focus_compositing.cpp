#include "PrimeStage/Ui.h"

#include "PrimeFrame/Flatten.h"
#include "PrimeFrame/Focus.h"
#include "PrimeFrame/Frame.h"
#include "PrimeFrame/Layout.h"

#include "third_party/doctest.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

constexpr float RootWidth = 480.0f;
constexpr float RootHeight = 280.0f;

constexpr PrimeFrame::ColorToken ColorBase = 1u;
constexpr PrimeFrame::ColorToken ColorSelection = 2u;
constexpr PrimeFrame::ColorToken ColorFocus = 3u;
constexpr PrimeFrame::ColorToken ColorAlt = 4u;
constexpr PrimeFrame::ColorToken ColorHeader = 5u;
constexpr PrimeFrame::ColorToken ColorDivider = 6u;
constexpr PrimeFrame::ColorToken ColorHover = 7u;
constexpr PrimeFrame::ColorToken ColorAccent = 8u;
constexpr PrimeFrame::ColorToken ColorText = 9u;

constexpr PrimeFrame::RectStyleToken StyleBase = 1u;
constexpr PrimeFrame::RectStyleToken StyleSelection = 2u;
constexpr PrimeFrame::RectStyleToken StyleFocus = 3u;
constexpr PrimeFrame::RectStyleToken StyleAlt = 4u;
constexpr PrimeFrame::RectStyleToken StyleHeader = 5u;
constexpr PrimeFrame::RectStyleToken StyleDivider = 6u;
constexpr PrimeFrame::RectStyleToken StyleHover = 7u;
constexpr PrimeFrame::RectStyleToken StyleAccent = 8u;

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

  theme->palette.assign(16u, PrimeFrame::Color{});
  theme->palette[ColorBase] = makeColor(0.20f, 0.22f, 0.26f);
  theme->palette[ColorSelection] = makeColor(0.05f, 0.74f, 0.16f);
  theme->palette[ColorFocus] = makeColor(0.92f, 0.17f, 0.11f);
  theme->palette[ColorAlt] = makeColor(0.32f, 0.35f, 0.40f);
  theme->palette[ColorHeader] = makeColor(0.42f, 0.38f, 0.20f);
  theme->palette[ColorDivider] = makeColor(0.60f, 0.62f, 0.66f);
  theme->palette[ColorHover] = makeColor(0.12f, 0.28f, 0.74f);
  theme->palette[ColorAccent] = makeColor(0.95f, 0.78f, 0.12f);
  theme->palette[ColorText] = makeColor(0.98f, 0.98f, 0.98f);

  theme->rectStyles.assign(16u, PrimeFrame::RectStyle{});
  theme->rectStyles[StyleBase].fill = ColorBase;
  theme->rectStyles[StyleAlt].fill = ColorAlt;
  theme->rectStyles[StyleSelection].fill = ColorSelection;
  theme->rectStyles[StyleFocus].fill = ColorFocus;
  theme->rectStyles[StyleHeader].fill = ColorHeader;
  theme->rectStyles[StyleDivider].fill = ColorDivider;
  theme->rectStyles[StyleHover].fill = ColorHover;
  theme->rectStyles[StyleAccent].fill = ColorAccent;

  theme->textStyles.assign(1u, PrimeFrame::TextStyle{});
  theme->textStyles[0].color = ColorText;
}

PrimeStage::UiNode createRoot(PrimeFrame::Frame& frame) {
  PrimeFrame::NodeId rootId = frame.createNode();
  frame.addRoot(rootId);
  if (PrimeFrame::Node* root = frame.getNode(rootId)) {
    root->layout = PrimeFrame::LayoutType::Overlay;
    root->sizeHint.width.preferred = RootWidth;
    root->sizeHint.height.preferred = RootHeight;
  }
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

PrimeFrame::RenderBatch flattenBatch(PrimeFrame::Frame const& frame,
                                     PrimeFrame::LayoutOutput const& layout) {
  PrimeFrame::RenderBatch batch;
  PrimeFrame::flattenToRenderBatch(frame, layout, batch);
  return batch;
}

std::vector<size_t> findRectCommandIndices(PrimeFrame::RenderBatch const& batch,
                                           PrimeFrame::Color const& color) {
  std::vector<size_t> indices;
  for (size_t i = 0; i < batch.commands.size(); ++i) {
    PrimeFrame::DrawCommand const& command = batch.commands[i];
    if (command.type != PrimeFrame::CommandType::Rect) {
      continue;
    }
    if (command.rectStyle.opacity <= 0.0f) {
      continue;
    }
    if (colorClose(command.rectStyle.fill, color)) {
      indices.push_back(i);
    }
  }
  return indices;
}

void checkFocusAfterSelection(PrimeFrame::RenderBatch const& batch) {
  PrimeFrame::Color selectionColor = makeColor(0.05f, 0.74f, 0.16f);
  PrimeFrame::Color focusColor = makeColor(0.92f, 0.17f, 0.11f);

  std::vector<size_t> selectionIndices = findRectCommandIndices(batch, selectionColor);
  std::vector<size_t> focusIndices = findRectCommandIndices(batch, focusColor);
  REQUIRE_FALSE(selectionIndices.empty());
  REQUIRE_FALSE(focusIndices.empty());

  size_t latestSelection = *std::max_element(selectionIndices.begin(), selectionIndices.end());
  size_t earliestFocus = *std::min_element(focusIndices.begin(), focusIndices.end());
  CHECK(earliestFocus > latestSelection);
}

} // namespace

TEST_CASE("TextField focus ring renders after selection highlight") {
  PrimeFrame::Frame frame;
  configureTheme(frame);
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::TextFieldState state;
  state.text = "focus order";
  state.cursor = 6u;
  state.selectionAnchor = 1u;
  state.selectionStart = 1u;
  state.selectionEnd = 6u;

  PrimeStage::TextFieldSpec spec;
  spec.state = &state;
  spec.backgroundStyle = StyleBase;
  spec.selectionStyle = StyleSelection;
  spec.focusStyle = StyleFocus;
  spec.size.preferredWidth = 220.0f;
  spec.size.preferredHeight = 30.0f;

  PrimeStage::UiNode textField = root.createTextField(spec);
  PrimeFrame::LayoutOutput layout = layoutFrame(frame);

  PrimeFrame::FocusManager focus;
  REQUIRE(focus.setFocus(frame, layout, textField.nodeId()));

  PrimeFrame::RenderBatch batch = flattenBatch(frame, layout);
  checkFocusAfterSelection(batch);
}

TEST_CASE("Table focus ring renders after selected row highlight") {
  PrimeFrame::Frame frame;
  configureTheme(frame);
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::TableSpec spec;
  spec.size.preferredWidth = 320.0f;
  spec.size.preferredHeight = 140.0f;
  spec.headerStyle = StyleHeader;
  spec.rowStyle = StyleBase;
  spec.rowAltStyle = StyleAlt;
  spec.selectionStyle = StyleSelection;
  spec.dividerStyle = StyleDivider;
  spec.focusStyle = StyleFocus;
  spec.selectedRow = 1;
  spec.columns = {
      {"Name", 150.0f, 0u, 0u},
      {"Value", 150.0f, 0u, 0u},
  };
  spec.rows = {
      {"alpha", "10"},
      {"beta", "20"},
      {"gamma", "30"},
  };

  PrimeStage::UiNode table = root.createTable(spec);
  PrimeFrame::LayoutOutput layout = layoutFrame(frame);

  PrimeFrame::FocusManager focus;
  REQUIRE(focus.setFocus(frame, layout, table.nodeId()));

  PrimeFrame::RenderBatch batch = flattenBatch(frame, layout);
  checkFocusAfterSelection(batch);
}

TEST_CASE("TreeView focus ring renders after selected row highlight") {
  PrimeFrame::Frame frame;
  configureTheme(frame);
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::TreeViewSpec spec;
  spec.size.preferredWidth = 320.0f;
  spec.size.preferredHeight = 180.0f;
  spec.rowStyle = StyleBase;
  spec.rowAltStyle = StyleAlt;
  spec.hoverStyle = StyleHover;
  spec.selectionStyle = StyleSelection;
  spec.selectionAccentStyle = StyleAccent;
  spec.caretBackgroundStyle = StyleHeader;
  spec.caretLineStyle = StyleDivider;
  spec.connectorStyle = StyleDivider;
  spec.focusStyle = StyleFocus;
  spec.textStyle = 0u;
  spec.selectedTextStyle = 0u;
  spec.nodes = {
      {"Root", {{"Child A"}, {"Child B"}}, true, true},
      {"Other", {}, false, false},
  };

  PrimeStage::UiNode tree = root.createTreeView(spec);
  PrimeFrame::LayoutOutput layout = layoutFrame(frame);

  PrimeFrame::FocusManager focus;
  REQUIRE(focus.setFocus(frame, layout, tree.nodeId()));

  PrimeFrame::RenderBatch batch = flattenBatch(frame, layout);
  checkFocusAfterSelection(batch);
}
