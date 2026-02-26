#pragma once

#include "PrimeStage/Ui.h"

#include "PrimeFrame/Flatten.h"
#include "PrimeFrame/Frame.h"
#include "PrimeFrame/Layout.h"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

namespace PrimeStageTest {

constexpr float VisualRootWidth = 480.0f;
constexpr float VisualRootHeight = 280.0f;

constexpr PrimeFrame::ColorToken VisualColorBase = 1u;
constexpr PrimeFrame::ColorToken VisualColorHover = 2u;
constexpr PrimeFrame::ColorToken VisualColorPressed = 3u;
constexpr PrimeFrame::ColorToken VisualColorFocus = 4u;
constexpr PrimeFrame::ColorToken VisualColorSelection = 5u;
constexpr PrimeFrame::ColorToken VisualColorText = 6u;

constexpr PrimeFrame::RectStyleToken VisualStyleBase = 1u;
constexpr PrimeFrame::RectStyleToken VisualStyleHover = 2u;
constexpr PrimeFrame::RectStyleToken VisualStylePressed = 3u;
constexpr PrimeFrame::RectStyleToken VisualStyleFocus = 4u;
constexpr PrimeFrame::RectStyleToken VisualStyleSelection = 5u;

struct VisualHarnessConfig {
  float rootWidth = VisualRootWidth;
  float rootHeight = VisualRootHeight;
  float layoutScale = 1.0f;
  std::string_view snapshotVersion = "interaction_v2";
  std::string_view fontPolicy = "command_batch_no_raster";
};

inline PrimeFrame::Color makeHarnessColor(float r, float g, float b) {
  PrimeFrame::Color color;
  color.r = r;
  color.g = g;
  color.b = b;
  color.a = 1.0f;
  return color;
}

inline bool harnessColorClose(PrimeFrame::Color const& lhs, PrimeFrame::Color const& rhs) {
  constexpr float Epsilon = 0.001f;
  return std::abs(lhs.r - rhs.r) <= Epsilon &&
         std::abs(lhs.g - rhs.g) <= Epsilon &&
         std::abs(lhs.b - rhs.b) <= Epsilon &&
         std::abs(lhs.a - rhs.a) <= Epsilon;
}

inline char harnessColorTag(PrimeFrame::Color const& color) {
  if (harnessColorClose(color, makeHarnessColor(0.24f, 0.26f, 0.30f))) {
    return 'B';
  }
  if (harnessColorClose(color, makeHarnessColor(0.18f, 0.46f, 0.80f))) {
    return 'H';
  }
  if (harnessColorClose(color, makeHarnessColor(0.13f, 0.30f, 0.56f))) {
    return 'P';
  }
  if (harnessColorClose(color, makeHarnessColor(0.90f, 0.23f, 0.15f))) {
    return 'F';
  }
  if (harnessColorClose(color, makeHarnessColor(0.09f, 0.65f, 0.24f))) {
    return 'S';
  }
  return '?';
}

inline int quantizeHarnessOpacity(float value) {
  return static_cast<int>(std::lround(value * 1000.0f));
}

inline void configureDeterministicTheme(PrimeFrame::Frame& frame) {
  PrimeFrame::Theme* theme = frame.getTheme(PrimeFrame::DefaultThemeId);
  if (!theme) {
    return;
  }

  theme->palette.assign(8u, PrimeFrame::Color{});
  theme->palette[VisualColorBase] = makeHarnessColor(0.24f, 0.26f, 0.30f);
  theme->palette[VisualColorHover] = makeHarnessColor(0.18f, 0.46f, 0.80f);
  theme->palette[VisualColorPressed] = makeHarnessColor(0.13f, 0.30f, 0.56f);
  theme->palette[VisualColorFocus] = makeHarnessColor(0.90f, 0.23f, 0.15f);
  theme->palette[VisualColorSelection] = makeHarnessColor(0.09f, 0.65f, 0.24f);
  theme->palette[VisualColorText] = makeHarnessColor(0.96f, 0.97f, 0.98f);

  theme->rectStyles.assign(8u, PrimeFrame::RectStyle{});
  theme->rectStyles[VisualStyleBase].fill = VisualColorBase;
  theme->rectStyles[VisualStyleHover].fill = VisualColorHover;
  theme->rectStyles[VisualStylePressed].fill = VisualColorPressed;
  theme->rectStyles[VisualStyleFocus].fill = VisualColorFocus;
  theme->rectStyles[VisualStyleSelection].fill = VisualColorSelection;

  theme->textStyles.assign(1u, PrimeFrame::TextStyle{});
  theme->textStyles[0].color = VisualColorText;
}

inline PrimeStage::UiNode createDeterministicRoot(PrimeFrame::Frame& frame,
                                                  VisualHarnessConfig const& config = {}) {
  PrimeFrame::NodeId rootId = frame.createNode();
  frame.addRoot(rootId);
  PrimeFrame::Node* root = frame.getNode(rootId);
  if (root) {
    root->layout = PrimeFrame::LayoutType::Overlay;
    root->sizeHint.width.preferred = config.rootWidth;
    root->sizeHint.height.preferred = config.rootHeight;
  }
  return PrimeStage::UiNode(frame, rootId, true);
}

inline PrimeFrame::LayoutOutput layoutDeterministicFrame(PrimeFrame::Frame& frame,
                                                         VisualHarnessConfig const& config = {}) {
  PrimeFrame::LayoutOutput output;
  PrimeFrame::LayoutEngine engine;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = config.rootWidth * config.layoutScale;
  options.rootHeight = config.rootHeight * config.layoutScale;
  engine.layout(frame, output, options);
  return output;
}

inline std::string rectCommandSnapshot(PrimeFrame::RenderBatch const& batch) {
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
        << harnessColorTag(command.rectStyle.fill) << ' '
        << quantizeHarnessOpacity(command.rectStyle.opacity) << '\n';
  }
  return out.str();
}

inline std::string deterministicSnapshotHeader(VisualHarnessConfig const& config = {}) {
  std::ostringstream out;
  out << "[harness]\n";
  out << "version=" << config.snapshotVersion << '\n';
  out << "theme=interaction_palette_v1\n";
  out << "font_policy=" << config.fontPolicy << '\n';
  out << "layout_scale=" << std::fixed << std::setprecision(2) << config.layoutScale << '\n';
  out << "root_size=" << static_cast<int>(std::lround(config.rootWidth)) << 'x'
      << static_cast<int>(std::lround(config.rootHeight)) << '\n';
  return out.str();
}

} // namespace PrimeStageTest
