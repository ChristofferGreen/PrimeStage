#include "PrimeStage/Ui.h"

#include "PrimeFrame/Events.h"
#include "PrimeFrame/Flatten.h"
#include "PrimeFrame/Focus.h"
#include "PrimeFrame/Frame.h"
#include "PrimeFrame/Layout.h"

#include "third_party/doctest.h"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <utility>

namespace {

constexpr float RootWidth = 480.0f;
constexpr float RootHeight = 280.0f;

constexpr PrimeFrame::ColorToken ColorBase = 1u;
constexpr PrimeFrame::ColorToken ColorHover = 2u;
constexpr PrimeFrame::ColorToken ColorPressed = 3u;
constexpr PrimeFrame::ColorToken ColorFocus = 4u;
constexpr PrimeFrame::ColorToken ColorSelection = 5u;
constexpr PrimeFrame::ColorToken ColorText = 6u;

constexpr PrimeFrame::RectStyleToken StyleBase = 1u;
constexpr PrimeFrame::RectStyleToken StyleHover = 2u;
constexpr PrimeFrame::RectStyleToken StylePressed = 3u;
constexpr PrimeFrame::RectStyleToken StyleFocus = 4u;
constexpr PrimeFrame::RectStyleToken StyleSelection = 5u;

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
  theme->palette[ColorBase] = makeColor(0.24f, 0.26f, 0.30f);
  theme->palette[ColorHover] = makeColor(0.18f, 0.46f, 0.80f);
  theme->palette[ColorPressed] = makeColor(0.13f, 0.30f, 0.56f);
  theme->palette[ColorFocus] = makeColor(0.90f, 0.23f, 0.15f);
  theme->palette[ColorSelection] = makeColor(0.09f, 0.65f, 0.24f);
  theme->palette[ColorText] = makeColor(0.96f, 0.97f, 0.98f);

  theme->rectStyles.assign(8u, PrimeFrame::RectStyle{});
  theme->rectStyles[StyleBase].fill = ColorBase;
  theme->rectStyles[StyleHover].fill = ColorHover;
  theme->rectStyles[StylePressed].fill = ColorPressed;
  theme->rectStyles[StyleFocus].fill = ColorFocus;
  theme->rectStyles[StyleSelection].fill = ColorSelection;

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

PrimeFrame::RenderBatch flattenBatch(PrimeFrame::Frame const& frame,
                                     PrimeFrame::LayoutOutput const& layout) {
  PrimeFrame::RenderBatch batch;
  PrimeFrame::flattenToRenderBatch(frame, layout, batch);
  return batch;
}

char colorTag(PrimeFrame::Color const& color) {
  if (colorClose(color, makeColor(0.24f, 0.26f, 0.30f))) {
    return 'B'; // base
  }
  if (colorClose(color, makeColor(0.18f, 0.46f, 0.80f))) {
    return 'H'; // hover
  }
  if (colorClose(color, makeColor(0.13f, 0.30f, 0.56f))) {
    return 'P'; // pressed
  }
  if (colorClose(color, makeColor(0.90f, 0.23f, 0.15f))) {
    return 'F'; // focus
  }
  if (colorClose(color, makeColor(0.09f, 0.65f, 0.24f))) {
    return 'S'; // selection
  }
  return '?';
}

int quantizeOpacity(float value) {
  return static_cast<int>(std::lround(value * 1000.0f));
}

std::string rectSnapshot(PrimeFrame::RenderBatch const& batch) {
  std::ostringstream out;
  for (PrimeFrame::DrawCommand const& command : batch.commands) {
    if (command.type != PrimeFrame::CommandType::Rect) {
      continue;
    }
    if (command.rectStyle.opacity <= 0.0f) {
      continue;
    }
    out << "R "
        << command.x0 << ' '
        << command.y0 << ' '
        << (command.x1 - command.x0) << ' '
        << (command.y1 - command.y0) << ' '
        << colorTag(command.rectStyle.fill) << ' '
        << quantizeOpacity(command.rectStyle.opacity) << '\n';
  }
  return out.str();
}

PrimeFrame::Event makePointerEvent(PrimeFrame::EventType type, float x, float y) {
  PrimeFrame::Event event;
  event.type = type;
  event.pointerId = 1;
  event.x = x;
  event.y = y;
  return event;
}

std::pair<float, float> nodeCenter(PrimeFrame::LayoutOutput const& layout, PrimeFrame::NodeId nodeId) {
  PrimeFrame::LayoutOut const* out = layout.get(nodeId);
  REQUIRE(out != nullptr);
  return {out->absX + out->absW * 0.5f, out->absY + out->absH * 0.5f};
}

enum class ButtonVisualState {
  Idle,
  Hover,
  Pressed,
  Focused,
};

std::string snapshotButtonState(ButtonVisualState state) {
  PrimeFrame::Frame frame;
  configureTheme(frame);
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::ButtonSpec buttonSpec;
  buttonSpec.label = "Visual";
  buttonSpec.backgroundStyle = StyleBase;
  buttonSpec.hoverStyle = StyleHover;
  buttonSpec.pressedStyle = StylePressed;
  buttonSpec.focusStyle = StyleFocus;
  buttonSpec.textStyle = 0u;
  buttonSpec.size.preferredWidth = 140.0f;
  buttonSpec.size.preferredHeight = 30.0f;
  PrimeStage::UiNode button = root.createButton(buttonSpec);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame);
  auto [x, y] = nodeCenter(layout, button.nodeId());
  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;

  switch (state) {
    case ButtonVisualState::Idle:
      break;
    case ButtonVisualState::Hover:
      router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerMove, x, y), frame, layout, &focus);
      break;
    case ButtonVisualState::Pressed:
      router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerMove, x, y), frame, layout, &focus);
      router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, x, y), frame, layout, &focus);
      break;
    case ButtonVisualState::Focused:
      CHECK(focus.setFocus(frame, layout, button.nodeId()));
      break;
  }

  return rectSnapshot(flattenBatch(frame, layout));
}

std::string snapshotTextFieldSelectionFocus() {
  PrimeFrame::Frame frame;
  configureTheme(frame);
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::TextFieldState state;
  state.text = "snapshot";
  state.cursor = 6u;
  state.selectionAnchor = 1u;
  state.selectionStart = 1u;
  state.selectionEnd = 6u;

  PrimeStage::TextFieldSpec fieldSpec;
  fieldSpec.state = &state;
  fieldSpec.backgroundStyle = StyleBase;
  fieldSpec.selectionStyle = StyleSelection;
  fieldSpec.focusStyle = StyleFocus;
  fieldSpec.textStyle = 0u;
  fieldSpec.size.preferredWidth = 220.0f;
  fieldSpec.size.preferredHeight = 30.0f;
  PrimeStage::UiNode field = root.createTextField(fieldSpec);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame);
  PrimeFrame::FocusManager focus;
  CHECK(focus.setFocus(frame, layout, field.nodeId()));
  return rectSnapshot(flattenBatch(frame, layout));
}

std::string snapshotTableSelectionFocus() {
  PrimeFrame::Frame frame;
  configureTheme(frame);
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::TableSpec tableSpec;
  tableSpec.size.preferredWidth = 300.0f;
  tableSpec.size.preferredHeight = 140.0f;
  tableSpec.headerHeight = 18.0f;
  tableSpec.headerStyle = StyleBase;
  tableSpec.rowStyle = StyleBase;
  tableSpec.rowAltStyle = StyleHover;
  tableSpec.selectionStyle = StyleSelection;
  tableSpec.dividerStyle = StyleBase;
  tableSpec.focusStyle = StyleFocus;
  tableSpec.selectedRow = 1;
  tableSpec.columns = {{"A", 120.0f, 0u, 0u}, {"B", 120.0f, 0u, 0u}};
  tableSpec.rows = {{"1", "2"}, {"3", "4"}, {"5", "6"}};
  PrimeStage::UiNode table = root.createTable(tableSpec);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame);
  PrimeFrame::FocusManager focus;
  CHECK(focus.setFocus(frame, layout, table.nodeId()));
  return rectSnapshot(flattenBatch(frame, layout));
}

std::string buildVisualSnapshotBundle() {
  std::string buttonIdle = snapshotButtonState(ButtonVisualState::Idle);
  std::string buttonHover = snapshotButtonState(ButtonVisualState::Hover);
  std::string buttonPressed = snapshotButtonState(ButtonVisualState::Pressed);
  std::string buttonFocused = snapshotButtonState(ButtonVisualState::Focused);
  std::string textSelectionFocus = snapshotTextFieldSelectionFocus();
  std::string tableSelectionFocus = snapshotTableSelectionFocus();

  CHECK_FALSE(buttonIdle.empty());
  CHECK_FALSE(buttonHover.empty());
  CHECK_FALSE(buttonPressed.empty());
  CHECK_FALSE(buttonFocused.empty());
  CHECK_FALSE(textSelectionFocus.empty());
  CHECK_FALSE(tableSelectionFocus.empty());

  CHECK(buttonIdle != buttonHover);
  CHECK(buttonHover != buttonPressed);
  CHECK(buttonIdle != buttonFocused);
  CHECK(textSelectionFocus.find(" S ") != std::string::npos);
  CHECK(textSelectionFocus.find(" F ") != std::string::npos);
  CHECK(tableSelectionFocus.find(" S ") != std::string::npos);
  CHECK(tableSelectionFocus.find(" F ") != std::string::npos);

  std::ostringstream out;
  out << "[button_idle]\n" << buttonIdle;
  out << "[button_hover]\n" << buttonHover;
  out << "[button_pressed]\n" << buttonPressed;
  out << "[button_focused]\n" << buttonFocused;
  out << "[text_field_selection_focus]\n" << textSelectionFocus;
  out << "[table_selection_focus]\n" << tableSelectionFocus;
  return out.str();
}

} // namespace

TEST_CASE("PrimeStage visual snapshots cover interaction and focus layering") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path snapshotPath = repoRoot / "tests" / "snapshots" / "interaction_visuals.snap";
  std::string actual = buildVisualSnapshotBundle();

  bool updateSnapshots = std::getenv("PRIMESTAGE_UPDATE_SNAPSHOTS") != nullptr;
  if (updateSnapshots) {
    std::filesystem::create_directories(snapshotPath.parent_path());
    std::ofstream output(snapshotPath, std::ios::binary | std::ios::trunc);
    REQUIRE(output.good());
    output << actual;
    REQUIRE(output.good());
    INFO("Updated interaction visual snapshots");
    CHECK(true);
    return;
  }

  REQUIRE(std::filesystem::exists(snapshotPath));
  std::ifstream input(snapshotPath, std::ios::binary);
  REQUIRE(input.good());
  std::string expected((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
  REQUIRE(!expected.empty());
  CHECK(actual == expected);
}
