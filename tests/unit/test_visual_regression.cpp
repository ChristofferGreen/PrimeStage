#include "PrimeStage/Ui.h"

#include "PrimeFrame/Events.h"
#include "PrimeFrame/Flatten.h"
#include "PrimeFrame/Focus.h"
#include "PrimeFrame/Frame.h"
#include "PrimeFrame/Layout.h"

#include "tests/unit/visual_test_harness.h"
#include "third_party/doctest.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <utility>

namespace {

enum class ButtonVisualState {
  Idle,
  Hover,
  Pressed,
  Focused,
};

PrimeFrame::RenderBatch flattenBatch(PrimeFrame::Frame const& frame,
                                     PrimeFrame::LayoutOutput const& layout) {
  PrimeFrame::RenderBatch batch;
  PrimeFrame::flattenToRenderBatch(frame, layout, batch);
  return batch;
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

std::string snapshotButtonState(ButtonVisualState state,
                                PrimeStageTest::VisualHarnessConfig const& config) {
  PrimeFrame::Frame frame;
  PrimeStageTest::configureDeterministicTheme(frame);
  PrimeStage::UiNode root = PrimeStageTest::createDeterministicRoot(frame, config);

  PrimeStage::ButtonSpec buttonSpec;
  buttonSpec.label = "Visual";
  buttonSpec.backgroundStyle = PrimeStageTest::VisualStyleBase;
  buttonSpec.hoverStyle = PrimeStageTest::VisualStyleHover;
  buttonSpec.pressedStyle = PrimeStageTest::VisualStylePressed;
  buttonSpec.focusStyle = PrimeStageTest::VisualStyleFocus;
  buttonSpec.textStyle = 0u;
  buttonSpec.size.preferredWidth = 140.0f;
  buttonSpec.size.preferredHeight = 30.0f;
  PrimeStage::UiNode button = root.createButton(buttonSpec);

  PrimeFrame::LayoutOutput layout = PrimeStageTest::layoutDeterministicFrame(frame, config);
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

  return PrimeStageTest::rectCommandSnapshot(flattenBatch(frame, layout));
}

std::string snapshotTextFieldSelectionFocus(PrimeStageTest::VisualHarnessConfig const& config) {
  PrimeFrame::Frame frame;
  PrimeStageTest::configureDeterministicTheme(frame);
  PrimeStage::UiNode root = PrimeStageTest::createDeterministicRoot(frame, config);

  PrimeStage::TextFieldState state;
  state.text = "snapshot";
  state.cursor = 6u;
  state.selectionAnchor = 1u;
  state.selectionStart = 1u;
  state.selectionEnd = 6u;

  PrimeStage::TextFieldSpec fieldSpec;
  fieldSpec.state = &state;
  fieldSpec.backgroundStyle = PrimeStageTest::VisualStyleBase;
  fieldSpec.selectionStyle = PrimeStageTest::VisualStyleSelection;
  fieldSpec.focusStyle = PrimeStageTest::VisualStyleFocus;
  fieldSpec.textStyle = 0u;
  fieldSpec.size.preferredWidth = 220.0f;
  fieldSpec.size.preferredHeight = 30.0f;
  PrimeStage::UiNode field = root.createTextField(fieldSpec);

  PrimeFrame::LayoutOutput layout = PrimeStageTest::layoutDeterministicFrame(frame, config);
  PrimeFrame::FocusManager focus;
  CHECK(focus.setFocus(frame, layout, field.nodeId()));
  return PrimeStageTest::rectCommandSnapshot(flattenBatch(frame, layout));
}

std::string snapshotTableSelectionFocus(PrimeStageTest::VisualHarnessConfig const& config) {
  PrimeFrame::Frame frame;
  PrimeStageTest::configureDeterministicTheme(frame);
  PrimeStage::UiNode root = PrimeStageTest::createDeterministicRoot(frame, config);

  PrimeStage::TableSpec tableSpec;
  tableSpec.size.preferredWidth = 300.0f;
  tableSpec.size.preferredHeight = 140.0f;
  tableSpec.headerHeight = 18.0f;
  tableSpec.headerStyle = PrimeStageTest::VisualStyleBase;
  tableSpec.rowStyle = PrimeStageTest::VisualStyleBase;
  tableSpec.rowAltStyle = PrimeStageTest::VisualStyleHover;
  tableSpec.selectionStyle = PrimeStageTest::VisualStyleSelection;
  tableSpec.dividerStyle = PrimeStageTest::VisualStyleBase;
  tableSpec.focusStyle = PrimeStageTest::VisualStyleFocus;
  tableSpec.selectedRow = 1;
  tableSpec.columns = {{"A", 120.0f, 0u, 0u}, {"B", 120.0f, 0u, 0u}};
  tableSpec.rows = {{"1", "2"}, {"3", "4"}, {"5", "6"}};
  PrimeStage::UiNode table = root.createTable(tableSpec);

  PrimeFrame::LayoutOutput layout = PrimeStageTest::layoutDeterministicFrame(frame, config);
  PrimeFrame::FocusManager focus;
  CHECK(focus.setFocus(frame, layout, table.nodeId()));
  return PrimeStageTest::rectCommandSnapshot(flattenBatch(frame, layout));
}

std::string buildVisualSnapshotBundle(PrimeStageTest::VisualHarnessConfig const& config = {}) {
  std::string buttonIdle = snapshotButtonState(ButtonVisualState::Idle, config);
  std::string buttonHover = snapshotButtonState(ButtonVisualState::Hover, config);
  std::string buttonPressed = snapshotButtonState(ButtonVisualState::Pressed, config);
  std::string buttonFocused = snapshotButtonState(ButtonVisualState::Focused, config);
  std::string textSelectionFocus = snapshotTextFieldSelectionFocus(config);
  std::string tableSelectionFocus = snapshotTableSelectionFocus(config);

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
  out << PrimeStageTest::deterministicSnapshotHeader(config);
  out << "[button_idle]\n" << buttonIdle;
  out << "[button_hover]\n" << buttonHover;
  out << "[button_pressed]\n" << buttonPressed;
  out << "[button_focused]\n" << buttonFocused;
  out << "[text_field_selection_focus]\n" << textSelectionFocus;
  out << "[table_selection_focus]\n" << tableSelectionFocus;
  return out.str();
}

} // namespace

TEST_CASE("PrimeStage visual harness metadata pins deterministic inputs") {
  PrimeStageTest::VisualHarnessConfig defaultConfig;
  std::string baseline = buildVisualSnapshotBundle(defaultConfig);
  CHECK(baseline.find("[harness]\n") != std::string::npos);
  CHECK(baseline.find("version=interaction_v2") != std::string::npos);
  CHECK(baseline.find("theme=interaction_palette_v1") != std::string::npos);
  CHECK(baseline.find("font_policy=command_batch_no_raster") != std::string::npos);
  CHECK(baseline.find("layout_scale=1.00") != std::string::npos);
  CHECK(baseline.find("root_size=480x280") != std::string::npos);

  PrimeStageTest::VisualHarnessConfig scaledConfig;
  scaledConfig.layoutScale = 1.25f;
  std::string scaled = buildVisualSnapshotBundle(scaledConfig);
  CHECK(scaled.find("layout_scale=1.25") != std::string::npos);
  CHECK(scaled != baseline);
}

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
