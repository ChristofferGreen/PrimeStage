#include "PrimeStage/Ui.h"

#include "PrimeFrame/Events.h"
#include "PrimeFrame/Flatten.h"
#include "PrimeFrame/Focus.h"
#include "PrimeFrame/Frame.h"
#include "PrimeFrame/Layout.h"

#include "tests/unit/visual_test_harness.h"
#include "third_party/doctest.h"

#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <optional>
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

constexpr float MinDefaultTextContrastRatio = 4.5f;
constexpr float MinDefaultFocusContrastRatio = 3.0f;

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

float srgbToLinear(float value) {
  value = std::clamp(value, 0.0f, 1.0f);
  if (value <= 0.04045f) {
    return value / 12.92f;
  }
  return std::pow((value + 0.055f) / 1.055f, 2.4f);
}

float contrastRatio(PrimeFrame::Color const& lhs, PrimeFrame::Color const& rhs) {
  auto luminance = [](PrimeFrame::Color const& color) -> float {
    float r = srgbToLinear(color.r);
    float g = srgbToLinear(color.g);
    float b = srgbToLinear(color.b);
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
  };

  float lhsLum = luminance(lhs);
  float rhsLum = luminance(rhs);
  float hi = std::max(lhsLum, rhsLum);
  float lo = std::min(lhsLum, rhsLum);
  return (hi + 0.05f) / (lo + 0.05f);
}

PrimeFrame::Color resolveThemeSurfaceColor(PrimeFrame::Theme const& theme) {
  if (theme.palette.empty()) {
    return PrimeFrame::Color{0.16f, 0.19f, 0.24f, 1.0f};
  }
  if (!theme.rectStyles.empty()) {
    size_t fillIndex = theme.rectStyles[0].fill;
    if (fillIndex < theme.palette.size()) {
      return theme.palette[fillIndex];
    }
  }
  return theme.palette[0];
}

std::string colorHex(PrimeFrame::Color const& color) {
  auto toHex = [](float channel) -> int {
    float clamped = std::clamp(channel, 0.0f, 1.0f);
    return static_cast<int>(std::lround(clamped * 255.0f));
  };

  std::ostringstream out;
  out << '#'
      << std::uppercase
      << std::hex
      << std::setfill('0')
      << std::setw(2) << toHex(color.r)
      << std::setw(2) << toHex(color.g)
      << std::setw(2) << toHex(color.b)
      << std::setw(2) << toHex(color.a);
  return out.str();
}

std::string rectCommandSnapshotWithColor(PrimeFrame::RenderBatch const& batch) {
  std::ostringstream out;
  out << std::fixed << std::setprecision(2);
  for (PrimeFrame::DrawCommand const& command : batch.commands) {
    if (command.type != PrimeFrame::CommandType::Rect) {
      continue;
    }
    if (command.rectStyle.opacity <= 0.0f) {
      continue;
    }

    int opacity = static_cast<int>(std::lround(command.rectStyle.opacity * 1000.0f));
    out << "R "
        << command.x0 << ' '
        << command.y0 << ' '
        << (command.x1 - command.x0) << ' '
        << (command.y1 - command.y0) << ' '
        << colorHex(command.rectStyle.fill) << ' '
        << opacity << '\n';
  }
  return out.str();
}

std::optional<PrimeFrame::Color> findFocusRingColor(PrimeFrame::RenderBatch const& batch,
                                                     PrimeFrame::LayoutOutput const& layout,
                                                     PrimeFrame::NodeId nodeId) {
  PrimeFrame::LayoutOut const* out = layout.get(nodeId);
  REQUIRE(out != nullptr);

  constexpr float EdgeEpsilon = 0.01f;
  constexpr float RingThickness = 2.0f;
  for (PrimeFrame::DrawCommand const& command : batch.commands) {
    if (command.type != PrimeFrame::CommandType::Rect || command.rectStyle.opacity <= 0.0f) {
      continue;
    }
    float width = command.x1 - command.x0;
    float height = command.y1 - command.y0;
    bool isTopEdge = std::abs(command.x0 - out->absX) <= EdgeEpsilon &&
                     std::abs(command.y0 - out->absY) <= EdgeEpsilon &&
                     std::abs(width - out->absW) <= EdgeEpsilon &&
                     std::abs(height - RingThickness) <= EdgeEpsilon;
    if (isTopEdge) {
      return command.rectStyle.fill;
    }
  }
  return std::nullopt;
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

std::string buildDefaultThemeReadabilitySnapshot(PrimeStageTest::VisualHarnessConfig const& config = {}) {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = PrimeStageTest::createDeterministicRoot(frame, config);

  PrimeStage::ButtonSpec buttonSpec;
  buttonSpec.label = "Readable";
  buttonSpec.size.preferredWidth = 180.0f;
  buttonSpec.size.preferredHeight = 36.0f;
  PrimeStage::UiNode button = root.createButton(buttonSpec);

  PrimeFrame::LayoutOutput layout = PrimeStageTest::layoutDeterministicFrame(frame, config);
  PrimeFrame::FocusManager focus;
  CHECK(focus.setFocus(frame, layout, button.nodeId()));

  PrimeFrame::RenderBatch batch = flattenBatch(frame, layout);

  PrimeFrame::Theme const* theme = frame.getTheme(PrimeFrame::DefaultThemeId);
  REQUIRE(theme != nullptr);
  REQUIRE(!theme->palette.empty());
  REQUIRE(!theme->rectStyles.empty());
  REQUIRE(!theme->textStyles.empty());
  REQUIRE(theme->rectStyles[0].fill < theme->palette.size());
  REQUIRE(theme->textStyles[0].color < theme->palette.size());

  PrimeFrame::Color surfaceColor = resolveThemeSurfaceColor(*theme);
  PrimeFrame::Color textColor = theme->palette[theme->textStyles[0].color];

  std::optional<PrimeFrame::Color> focusColor = findFocusRingColor(batch, layout, button.nodeId());
  REQUIRE(focusColor.has_value());

  float textContrast = contrastRatio(textColor, surfaceColor);
  float focusContrast = contrastRatio(*focusColor, surfaceColor);
  CHECK(textContrast >= MinDefaultTextContrastRatio);
  CHECK(focusContrast >= MinDefaultFocusContrastRatio);

  std::ostringstream out;
  out << "[harness]\n";
  out << "version=default_theme_readability_v1\n";
  out << "theme=primestage_default_semantic_v1\n";
  out << "font_policy=command_batch_no_raster\n";
  out << "layout_scale=" << std::fixed << std::setprecision(2) << config.layoutScale << '\n';
  out << "root_size=" << static_cast<int>(std::lround(config.rootWidth)) << 'x'
      << static_cast<int>(std::lround(config.rootHeight)) << '\n';
  out << "[metrics]\n";
  out << std::fixed << std::setprecision(2);
  out << "min_text_contrast=" << MinDefaultTextContrastRatio << '\n';
  out << "min_focus_contrast=" << MinDefaultFocusContrastRatio << '\n';
  out << "text_contrast=" << textContrast << '\n';
  out << "focus_contrast=" << focusContrast << '\n';
  out << "surface_color=" << colorHex(surfaceColor) << '\n';
  out << "text_color=" << colorHex(textColor) << '\n';
  out << "focus_color=" << colorHex(*focusColor) << '\n';
  out << "[button_focused]\n";
  out << rectCommandSnapshotWithColor(batch);
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

TEST_CASE("PrimeStage default-theme visual snapshot enforces readability thresholds") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path snapshotPath =
      repoRoot / "tests" / "snapshots" / "default_theme_readability.snap";
  std::string actual = buildDefaultThemeReadabilitySnapshot();

  bool updateSnapshots = std::getenv("PRIMESTAGE_UPDATE_SNAPSHOTS") != nullptr;
  if (updateSnapshots) {
    std::filesystem::create_directories(snapshotPath.parent_path());
    std::ofstream output(snapshotPath, std::ios::binary | std::ios::trunc);
    REQUIRE(output.good());
    output << actual;
    REQUIRE(output.good());
    INFO("Updated default-theme readability snapshots");
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
