#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "third_party/stb_image_write.h"

#include "PrimeStage/PrimeStage.h"

#include "PrimeFrame/Flatten.h"
#include "PrimeFrame/Layout.h"

#include "PrimeManifest/renderer/Optimizer2D.hpp"
#include "PrimeManifest/renderer/Renderer2D.hpp"
#include "PrimeManifest/text/FontRegistry.hpp"
#include "PrimeManifest/text/TextBake.hpp"
#include "PrimeManifest/text/Typography.hpp"
#include "PrimeManifest/util/BitmapFont.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

using namespace PrimeFrame;
using namespace PrimeStage;

namespace {
uint8_t to_u8(float value) {
  float clamped = std::clamp(value, 0.0f, 1.0f);
  return static_cast<uint8_t>(std::lround(clamped * 255.0f));
}

uint32_t pack_color(PrimeFrame::Color const& color, float opacity) {
  float alpha = color.a * opacity;
  PrimeManifest::Color pmColor{to_u8(color.r), to_u8(color.g), to_u8(color.b), to_u8(alpha)};
  return PrimeManifest::PackRGBA8(pmColor);
}

uint8_t palette_index(PrimeManifest::RenderBatch& batch, uint32_t color) {
  if (!batch.palette.enabled) {
    batch.palette.enabled = true;
    batch.palette.size = 0;
    batch.palette.colorRGBA8.fill(0u);
  }
  for (uint16_t i = 0; i < batch.palette.size; ++i) {
    if (batch.palette.colorRGBA8[i] == color) {
      return static_cast<uint8_t>(i);
    }
  }
  if (batch.palette.size >= batch.palette.colorRGBA8.size()) {
    return 0;
  }
  uint8_t idx = static_cast<uint8_t>(batch.palette.size++);
  batch.palette.colorRGBA8[idx] = color;
  return idx;
}

void add_clear(PrimeManifest::RenderBatch& batch, uint32_t color) {
  uint32_t idx = static_cast<uint32_t>(batch.clear.colorIndex.size());
  batch.clear.colorIndex.push_back(palette_index(batch, color));
  batch.commands.push_back(PrimeManifest::RenderCommand{PrimeManifest::CommandType::Clear, idx});
}

void add_rect(PrimeManifest::RenderBatch& batch, DrawCommand const& cmd, float radiusPx) {
  uint32_t idx = static_cast<uint32_t>(batch.rects.x0.size());
  batch.rects.x0.push_back(cmd.x0);
  batch.rects.y0.push_back(cmd.y0);
  batch.rects.x1.push_back(cmd.x1);
  batch.rects.y1.push_back(cmd.y1);

  uint32_t color = pack_color(cmd.rectStyle.fill, cmd.rectStyle.opacity);
  uint8_t colorIndex = palette_index(batch, color);
  batch.rects.colorIndex.push_back(colorIndex);
  float clamped = std::clamp(radiusPx, 0.0f, 255.0f);
  uint16_t radiusQ8_8 = static_cast<uint16_t>(std::lround(clamped * 256.0f));
  batch.rects.radiusQ8_8.push_back(radiusQ8_8);
  batch.rects.rotationQ8_8.push_back(0);
  batch.rects.zQ8_8.push_back(0);
  batch.rects.opacity.push_back(255);

  uint8_t flags = 0;
  if (cmd.clipEnabled) {
    flags |= PrimeManifest::RectFlagClip;
  }
  batch.rects.flags.push_back(flags);
  batch.rects.gradientColor1Index.push_back(colorIndex);
  batch.rects.gradientDirX.push_back(0);
  batch.rects.gradientDirY.push_back(0);

  if (cmd.clipEnabled) {
    batch.rects.clipX0.push_back(cmd.clip.x0);
    batch.rects.clipY0.push_back(cmd.clip.y0);
    batch.rects.clipX1.push_back(cmd.clip.x1);
    batch.rects.clipY1.push_back(cmd.clip.y1);
  } else {
    batch.rects.clipX0.push_back(0);
    batch.rects.clipY0.push_back(0);
    batch.rects.clipX1.push_back(0);
    batch.rects.clipY1.push_back(0);
  }

  batch.commands.push_back(PrimeManifest::RenderCommand{PrimeManifest::CommandType::Rect, idx});
}

void add_rect_raw(PrimeManifest::RenderBatch& batch,
                  int32_t x0,
                  int32_t y0,
                  int32_t x1,
                  int32_t y1,
                  uint8_t colorIndex) {
  uint32_t idx = static_cast<uint32_t>(batch.rects.x0.size());
  batch.rects.x0.push_back(static_cast<int16_t>(x0));
  batch.rects.y0.push_back(static_cast<int16_t>(y0));
  batch.rects.x1.push_back(static_cast<int16_t>(x1));
  batch.rects.y1.push_back(static_cast<int16_t>(y1));
  batch.rects.colorIndex.push_back(colorIndex);
  batch.rects.radiusQ8_8.push_back(0);
  batch.rects.rotationQ8_8.push_back(0);
  batch.rects.zQ8_8.push_back(0);
  batch.rects.opacity.push_back(255);
  batch.rects.flags.push_back(0);
  batch.rects.gradientColor1Index.push_back(colorIndex);
  batch.rects.gradientDirX.push_back(0);
  batch.rects.gradientDirY.push_back(0);
  batch.rects.clipX0.push_back(0);
  batch.rects.clipY0.push_back(0);
  batch.rects.clipX1.push_back(0);
  batch.rects.clipY1.push_back(0);
  batch.commands.push_back(PrimeManifest::RenderCommand{PrimeManifest::CommandType::Rect, idx});
}

struct ClipRect {
  int32_t x0 = 0;
  int32_t y0 = 0;
  int32_t x1 = 0;
  int32_t y1 = 0;
  bool enabled = false;
};

ClipRect make_clip(float x, float y, float w, float h) {
  ClipRect clip;
  clip.x0 = static_cast<int32_t>(std::lround(x));
  clip.y0 = static_cast<int32_t>(std::lround(y));
  clip.x1 = static_cast<int32_t>(std::lround(x + w));
  clip.y1 = static_cast<int32_t>(std::lround(y + h));
  clip.enabled = true;
  return clip;
}

void apply_text_clip(PrimeManifest::RenderBatch& batch, uint32_t textIndex, ClipRect clip) {
  if (!clip.enabled) {
    return;
  }
  if (textIndex >= batch.text.flags.size()) {
    return;
  }
  batch.text.flags[textIndex] |= PrimeManifest::TextFlagClip;
  batch.text.clipX0[textIndex] = static_cast<int16_t>(clip.x0);
  batch.text.clipY0[textIndex] = static_cast<int16_t>(clip.y0);
  batch.text.clipX1[textIndex] = static_cast<int16_t>(clip.x1);
  batch.text.clipY1[textIndex] = static_cast<int16_t>(clip.y1);
}

void add_bitmap_text(PrimeManifest::RenderBatch& batch,
                     std::string_view text,
                     int32_t x,
                     int32_t y,
                     float sizePixels,
                     uint8_t colorIndex,
                     ClipRect clip) {
  if (text.empty()) return;
  float scale = sizePixels / static_cast<float>(PrimeManifest::UiFontHeight);
  int pixel = std::max(1, static_cast<int>(std::lround(scale)));
  int advance = static_cast<int>(std::lround(static_cast<float>(PrimeManifest::UiFontAdvance) * scale));

  int penX = x;
  for (char c : text) {
    if (c == '\n') {
      penX = x;
      y += static_cast<int32_t>(std::lround(static_cast<float>(PrimeManifest::UiFontHeight + 2) * scale));
      continue;
    }
    for (int py = 0; py < PrimeManifest::UiFontHeight; ++py) {
      for (int px = 0; px < PrimeManifest::UiFontWidth; ++px) {
        if (!PrimeManifest::UiFontPixel(c, px, py)) continue;
        int32_t x0 = penX + px * pixel;
        int32_t y0 = y + py * pixel;
        int32_t x1 = x0 + pixel;
        int32_t y1 = y0 + pixel;
        if (clip.enabled) {
          if (x1 <= clip.x0 || x0 >= clip.x1 || y1 <= clip.y0 || y0 >= clip.y1) {
            continue;
          }
        }
        add_rect_raw(batch, x0, y0, x1, y1, colorIndex);
      }
    }
    penX += advance;
  }
}

PrimeManifest::Typography make_typography(ResolvedTextStyle const& style) {
  PrimeManifest::Typography type;
  type.size = style.size;
  type.weight = static_cast<int>(std::lround(style.weight));
#if defined(PRIMESTAGE_HAS_BUNDLED_FONT) && PRIMESTAGE_HAS_BUNDLED_FONT
  type.fallback = PrimeManifest::FontFallbackPolicy::BundleOnly;
#else
  type.fallback = PrimeManifest::FontFallbackPolicy::BundleThenOS;
#endif
  type.lineHeight = style.lineHeight > 0.0f ? style.lineHeight : style.size * 1.2f;
  return type;
}

bool write_png(std::string const& path, PrimeManifest::RenderTarget const& target) {
  if (target.width == 0 || target.height == 0 || target.data.empty()) {
    return false;
  }
  return stbi_write_png(path.c_str(),
                        static_cast<int>(target.width),
                        static_cast<int>(target.height),
                        4,
                        target.data.data(),
                        static_cast<int>(target.width * 4)) != 0;
}

} // namespace

int main(int argc, char** argv) {
  std::string outPath = argc > 1 ? argv[1] : "screenshots/primeframe_ui.png";
  std::filesystem::path outFile(outPath);
  if (outFile.has_parent_path()) {
    std::filesystem::create_directories(outFile.parent_path());
  }

  constexpr float kWidth = 1100.0f;
  constexpr float kHeight = 700.0f;
  constexpr float topbarH = 56.0f;
  constexpr float statusH = 24.0f;
  constexpr float sidebarW = 240.0f;
  constexpr float inspectorW = 260.0f;
  constexpr float opacityBarH = 22.0f;
  constexpr float opacityBarInset = 24.0f;
  constexpr float opacityBarW = inspectorW - (opacityBarInset * 2.0f);
  constexpr float contentW = kWidth - sidebarW - inspectorW;
  constexpr float contentH = kHeight - topbarH - statusH;

  Frame frame;
  applyStudioTheme(frame);
  Theme* defaultTheme = frame.getTheme(DefaultThemeId);

  enum ColorId : ColorToken {
    ColorBackground = 0,
    ColorTopbar,
    ColorStatusbar,
    ColorSidebar,
    ColorContent,
    ColorPanel,
    ColorPanelAlt,
    ColorPanelStrong,
    ColorAccent,
    ColorSelection,
    ColorScrollTrack,
    ColorScrollThumb,
    ColorDivider,
    ColorTextBright,
    ColorTextMuted,
    ColorCount
  };

  auto& registry = PrimeManifest::GetFontRegistry();
#if defined(PRIMESTAGE_HAS_BUNDLED_FONT) && PRIMESTAGE_HAS_BUNDLED_FONT
  registry.addBundleDir(PRIMESTAGE_BUNDLED_FONT_DIR);
#endif
  registry.loadBundledFonts();
  registry.loadOsFallbackFonts();

  auto typography_for = [&](TextStyleToken token) {
    ResolvedTextStyle resolved = resolveTextStyle(*defaultTheme, token, {});
    return make_typography(resolved);
  };

  PrimeManifest::Typography titleType = typography_for(textToken(TextRole::TitleBright));
  PrimeManifest::Typography bodyType = typography_for(textToken(TextRole::BodyBright));
  PrimeManifest::Typography bodyMutedType = typography_for(textToken(TextRole::BodyMuted));
  PrimeManifest::Typography smallType = typography_for(textToken(TextRole::SmallBright));
  PrimeManifest::Typography smallMutedType = typography_for(textToken(TextRole::SmallMuted));

  ShellSpec shellSpec;
  shellSpec.bounds = Bounds{0.0f, 0.0f, kWidth, kHeight};
  shellSpec.topbarHeight = topbarH;
  shellSpec.statusHeight = statusH;
  shellSpec.sidebarWidth = sidebarW;
  shellSpec.inspectorWidth = inspectorW;
  ShellLayout shell = createShell(frame, shellSpec);
  UiNode root = shell.root;
  UiNode topbarNode = shell.topbar;
  UiNode sidebarNode = shell.sidebar;
  UiNode contentNode = shell.content;
  UiNode inspectorNode = shell.inspector;

  auto add_panel = [&](UiNode& parent, float x, float y, float w, float h, RectRole role) -> UiNode {
    return parent.createPanel(role, Bounds{x, y, w, h});
  };

  auto add_divider = [&](UiNode& parent, float x, float y, float w, float h) -> UiNode {
    DividerSpec spec;
    spec.bounds = Bounds{x, y, w, h};
    spec.rectStyle = rectToken(RectRole::Divider);
    return parent.createDivider(spec);
  };

  auto create_topbar = [&]() {
    add_divider(topbarNode, 232.0f, 12.0f, 1.0f, 32.0f);
    TextFieldSpec searchSpec;
    searchSpec.bounds = Bounds{248.0f, 12.0f, 360.0f, 32.0f};
    searchSpec.placeholder = "Search...";
    topbarNode.createTextField(searchSpec);
    ButtonSpec runSpec;
    runSpec.bounds = Bounds{kWidth - 220.0f, 12.0f, 88.0f, 32.0f};
    runSpec.label = "Run";
    runSpec.backgroundRole = RectRole::Accent;
    runSpec.textRole = TextRole::BodyBright;
    UiNode runNode = topbarNode.createButton(runSpec);

    ButtonSpec shareSpec;
    shareSpec.bounds = Bounds{kWidth - 120.0f, 12.0f, 88.0f, 32.0f};
    shareSpec.label = "Share";
    shareSpec.backgroundRole = RectRole::Panel;
    shareSpec.textRole = TextRole::BodyBright;
    UiNode shareNode = topbarNode.createButton(shareSpec);

    topbarNode.createTextLine(Bounds{0.0f, 14.0f, 232.0f, 28.0f},
                              "PrimeFrame Studio",
                              TextRole::TitleBright,
                              PrimeFrame::TextAlign::Center);
  };

  auto create_sidebar = [&]() {
    UiNode treePanel = add_panel(sidebarNode, 12.0f, 44.0f, sidebarW - 24.0f, contentH - 64.0f, RectRole::Panel);
    add_panel(sidebarNode, 12.0f, 12.0f, sidebarW - 24.0f, 20.0f, RectRole::PanelStrong);
    add_panel(sidebarNode, 12.0f, 12.0f, 3.0f, 20.0f, RectRole::Accent);

    sidebarNode.createTextLine(Bounds{40.0f, 12.0f, 120.0f, 20.0f},
                               "Scene",
                               TextRole::BodyBright);
    sidebarNode.createTextLine(Bounds{40.0f, 52.0f, 140.0f, 20.0f},
                               "Hierarchy",
                               TextRole::SmallMuted);

    TreeViewSpec treeSpec;
    treeSpec.bounds = Bounds{0.0f, 0.0f, sidebarW - 24.0f, contentH - 64.0f};
    treeSpec.showHeaderDivider = true;
    treeSpec.headerDividerY = 30.0f;
    treeSpec.scrollBar.thumbFraction = 90.0f / (contentH - 80.0f);
    float treeTrackSpan = std::max(1.0f, (contentH - 80.0f) - 90.0f);
    treeSpec.scrollBar.thumbProgress = (60.0f - 8.0f) / treeTrackSpan;

    TreeNode treeRoot{
        "Root",
        {
            TreeNode{"World",
                     {TreeNode{"Camera"}, TreeNode{"Lights"}, TreeNode{"Environment"}},
                     true,
                     false},
            TreeNode{"UI",
                     {
                         TreeNode{"Sidebar"},
                         TreeNode{"Toolbar", {TreeNode{"Buttons"}}, false, false},
                         TreeNode{"Panels",
                                  {TreeNode{"TreeView", {}, true, true}, TreeNode{"Rows"}},
                                  true,
                                  false}
                     },
                     true,
                     false}
        },
        true,
        false};

    treeSpec.nodes = {treeRoot};
    treePanel.createTreeView(treeSpec);
  };

  auto create_content = [&]() {
    UiNode boardPanel = add_panel(contentNode, 16.0f, 60.0f, contentW - 32.0f, 110.0f, RectRole::Panel);
    float boardButtonW = 120.0f;
    float boardButtonH = 32.0f;
    float boardButtonX = contentW - 148.0f;
    float boardButtonY = 60.0f + 110.0f - boardButtonH - 12.0f;
    ButtonSpec primarySpec;
    primarySpec.bounds = Bounds{boardButtonX, boardButtonY, boardButtonW, boardButtonH};
    primarySpec.label = "Primary Action";
    primarySpec.backgroundRole = RectRole::Accent;
    primarySpec.textRole = TextRole::BodyBright;
    UiNode primaryButton = contentNode.createButton(primarySpec);
    float highlightHeaderY = 170.0f;
    float highlightHeaderH = 20.0f;

    contentNode.createSectionHeader(Bounds{16.0f, 14.0f, contentW - 32.0f, 32.0f},
                                    "Overview",
                                    TextRole::TitleBright);

    contentNode.createSectionHeader(Bounds{16.0f, highlightHeaderY, contentW - 32.0f, highlightHeaderH},
                                    "Highlights",
                                    TextRole::SmallBright,
                                    true,
                                    2.0f);


    contentNode.createCardGrid(Bounds{16.0f, 196.0f, contentW - 16.0f, 120.0f},
                               {
                                   {"Card", "Detail"},
                                   {"Card", "Detail"},
                                   {"Card", "Detail"}
                               });

    boardPanel.createTextLine(Bounds{16.0f, 12.0f, 200.0f, 30.0f},
                              "Active Board",
                              TextRole::SmallMuted);
    boardPanel.createParagraph(Bounds{16.0f, 36.0f, 460.0f, 0.0f},
                               "Lorem ipsum dolor sit amet, consectetur adipiscing elit.\n"
                               "Sed do eiusmod tempor incididunt ut labore et dolore.\n"
                               "Ut enim ad minim veniam, quis nostrud exercitation.",
                               TextRole::SmallMuted);

    float listY = 340.0f;
    float listHeaderY = listY - 20.0f;
    float tableWidth = contentW - 64.0f;
    float firstColWidth = contentW - 200.0f;
    float secondColWidth = tableWidth - firstColWidth;

    contentNode.createTable(Bounds{16.0f, listHeaderY - 6.0f, tableWidth, 0.0f},
                            {
                                TableColumn{"Item", firstColWidth, TextRole::SmallBright, TextRole::SmallBright},
                                TableColumn{"Status", secondColWidth, TextRole::SmallBright, TextRole::SmallMuted}
                            },
                            {
                                {"Item Row", "Ready"},
                                {"Item Row", "Ready"},
                                {"Item Row", "Ready"},
                                {"Item Row", "Ready"},
                                {"Item Row", "Ready"},
                                {"Item Row", "Ready"}
                            });

    ScrollViewSpec scrollSpec;
    scrollSpec.bounds = Bounds{0.0f, 0.0f, contentW, contentH};
    scrollSpec.vertical.inset = 12.0f;
    scrollSpec.vertical.startPadding = 12.0f;
    scrollSpec.vertical.endPadding = 12.0f;
    scrollSpec.vertical.thickness = 6.0f;
    scrollSpec.vertical.thumbLength = 120.0f;
    scrollSpec.vertical.thumbOffset = 108.0f;
    scrollSpec.horizontal.inset = 12.0f;
    scrollSpec.horizontal.startPadding = 16.0f;
    scrollSpec.horizontal.endPadding = 48.0f;
    scrollSpec.horizontal.thickness = 6.0f;
    scrollSpec.horizontal.thumbLength = 120.0f;
    scrollSpec.horizontal.thumbOffset = 124.0f;
    contentNode.createScrollView(scrollSpec);
  };

  auto create_inspector = [&]() {
    inspectorNode.createSectionHeader(Bounds{16.0f, 14.0f, inspectorW - 32.0f, 32.0f},
                                      "Inspector",
                                      TextRole::BodyBright);

    SectionPanelSpec propsPanelSpec;
    propsPanelSpec.bounds = Bounds{16.0f, 56.0f, inspectorW - 32.0f, 90.0f};
    propsPanelSpec.title = "Properties";
    propsPanelSpec.textRole = TextRole::SmallBright;
    propsPanelSpec.headerInsetRight = 24.0f;
    SectionPanel propsPanel = inspectorNode.createSectionPanel(propsPanelSpec);

    SectionPanelSpec transformPanelSpec;
    transformPanelSpec.bounds = Bounds{16.0f, 164.0f, inspectorW - 32.0f, 140.0f};
    transformPanelSpec.title = "Transform";
    transformPanelSpec.textRole = TextRole::SmallBright;
    transformPanelSpec.headerInsetRight = 24.0f;
    SectionPanel transformPanel = inspectorNode.createSectionPanel(transformPanelSpec);

    float opacityRowY = transformPanel.contentBounds.y + 44.0f;
    float opacityBarX = transformPanel.contentBounds.x;
    ProgressBarSpec opacityBar;
    opacityBar.bounds = Bounds{opacityBarX, opacityRowY, transformPanel.contentBounds.width, opacityBarH};
    opacityBar.value = 0.85f;
    opacityBar.trackRole = RectRole::PanelStrong;
    opacityBar.fillRole = RectRole::Accent;
    transformPanel.panel.createProgressBar(opacityBar);

    ButtonSpec publishSpec;
    publishSpec.bounds = Bounds{16.0f, contentH - 56.0f, inspectorW - 32.0f, 32.0f};
    publishSpec.label = "Publish";
    publishSpec.backgroundRole = RectRole::Accent;
    publishSpec.textRole = TextRole::BodyBright;
    UiNode publishButton = inspectorNode.createButton(publishSpec);

    PropertyListSpec propsList;
    propsList.bounds = propsPanel.contentBounds;
    propsList.rowHeight = 12.0f;
    propsList.rowGap = 12.0f;
    propsList.labelRole = TextRole::SmallMuted;
    propsList.valueRole = TextRole::SmallBright;
    propsList.valueAlignRight = true;
    propsList.valuePaddingRight = 0.0f;
    propsList.rows = {
        {"Name", "SceneRoot"},
        {"Tag", "Environment"}
    };
    propsPanel.panel.createPropertyList(propsList);

    PropertyListSpec transformList;
    transformList.bounds = transformPanel.contentBounds;
    transformList.rowHeight = 12.0f;
    transformList.rowGap = 12.0f;
    transformList.labelRole = TextRole::SmallMuted;
    transformList.valueRole = TextRole::SmallBright;
    transformList.valueAlignRight = true;
    transformList.valuePaddingRight = 0.0f;
    transformList.rows = {
        {"Position", "0, 0, 0"},
        {"Scale", "1, 1, 1"}
    };
    transformPanel.panel.createPropertyList(transformList);

    PropertyListSpec opacityList;
    opacityList.bounds = transformPanel.contentBounds;
    opacityList.bounds.y = opacityRowY;
    opacityList.rowHeight = opacityBarH;
    opacityList.rowGap = 0.0f;
    opacityList.labelRole = TextRole::SmallBright;
    opacityList.valueRole = TextRole::SmallBright;
    opacityList.valueAlignRight = true;
    opacityList.valuePaddingRight = 0.0f;
    opacityList.rows = {
        {"Opacity", "85%"}
    };
    transformPanel.panel.createPropertyList(opacityList);
  };

  auto create_status = [&]() {
    StatusBarSpec status;
    status.bounds = shell.statusBounds;
    status.leftText = "Ready";
    status.rightText = "PrimeFrame Demo";
    root.createStatusBar(status);
  };

  create_topbar();
  create_sidebar();
  create_content();
  create_inspector();
  create_status();

  LayoutEngine engine;
  LayoutOutput layout;
  engine.layout(frame, layout);

  PrimeFrame::RenderBatch pfBatch;
  flattenToRenderBatch(frame, layout, pfBatch);

  PrimeManifest::RenderBatch batch;
  batch.assumeFrontToBack = false;
  add_clear(batch, PrimeManifest::PackRGBA8(PrimeManifest::Color{10, 14, 20, 255}));

  auto colors_close = [](Color const& a, Color const& b) -> bool {
    float eps = 0.02f;
    return std::abs(a.r - b.r) < eps && std::abs(a.g - b.g) < eps && std::abs(a.b - b.b) < eps &&
           std::abs(a.a - b.a) < eps;
  };

  auto theme_color_pf = [&](ColorToken token) -> Color {
    if (!defaultTheme || defaultTheme->palette.empty()) {
      return Color{0.0f, 0.0f, 0.0f, 1.0f};
    }
    return defaultTheme->palette[static_cast<size_t>(token)];
  };

  for (DrawCommand const& cmd : pfBatch.commands) {
    if (cmd.type == PrimeFrame::CommandType::Rect ||
        cmd.type == PrimeFrame::CommandType::ImagePlaceholder) {
      float rectW = static_cast<float>(cmd.x1 - cmd.x0);
      float rectH = static_cast<float>(cmd.y1 - cmd.y0);
      float radius = 0.0f;
      if (rectH <= 6.0f && colors_close(cmd.rectStyle.fill, theme_color_pf(ColorScrollThumb))) {
        radius = 4.0f;
      } else if (rectH <= 6.0f && colors_close(cmd.rectStyle.fill, theme_color_pf(ColorScrollTrack))) {
        radius = 3.0f;
      } else if (rectW <= 12.0f && rectH <= 12.0f &&
                 colors_close(cmd.rectStyle.fill, theme_color_pf(ColorPanelStrong))) {
        radius = 2.0f;
      } else if (rectH >= 30.0f && rectH <= 34.0f) {
        if (colors_close(cmd.rectStyle.fill, theme_color_pf(ColorAccent))) {
          radius = 6.0f;
        } else if (colors_close(cmd.rectStyle.fill, theme_color_pf(ColorPanel))) {
          if (rectW <= 140.0f || rectW >= 300.0f) {
            radius = 6.0f;
          }
        }
      } else if (rectH >= 110.0f && rectH <= 130.0f &&
                 colors_close(cmd.rectStyle.fill, theme_color_pf(ColorPanelAlt))) {
        radius = 4.0f;
      }
      add_rect(batch, cmd, radius);
    }
  }

  for (DrawCommand const& cmd : pfBatch.commands) {
    if (cmd.type != PrimeFrame::CommandType::Text) {
      continue;
    }
    PrimeManifest::Typography type = make_typography(cmd.textStyle);
    uint32_t packed = pack_color(cmd.textStyle.color, 1.0f);
    uint8_t colorIndex = palette_index(batch, packed);
    ClipRect clip;
    if (cmd.clipEnabled) {
      clip.x0 = cmd.clip.x0;
      clip.y0 = cmd.clip.y0;
      clip.x1 = cmd.clip.x1;
      clip.y1 = cmd.clip.y1;
      clip.enabled = true;
    }
    uint8_t flags = clip.enabled ? PrimeManifest::TextFlagClip : 0u;
    auto result = PrimeManifest::AppendText(batch,
                                            cmd.text,
                                            type,
                                            1.0f,
                                            cmd.x0,
                                            cmd.y0,
                                            colorIndex,
                                            255,
                                            flags);
    if (result) {
      apply_text_clip(batch, result->textIndex, clip);
    } else {
      float fallbackSize = std::max(10.0f, type.size * 0.9f);
      add_bitmap_text(batch, cmd.text, cmd.x0, cmd.y0, fallbackSize, colorIndex, clip);
    }
  }

  uint32_t widthPx = static_cast<uint32_t>(kWidth);
  uint32_t heightPx = static_cast<uint32_t>(kHeight);
  std::vector<uint8_t> buffer(widthPx * heightPx * 4, 0);
  PrimeManifest::RenderTarget target{std::span<uint8_t>(buffer), widthPx, heightPx, widthPx * 4};

  PrimeManifest::OptimizedBatch optimized;
  PrimeManifest::OptimizeRenderBatch(target, batch, optimized);
  PrimeManifest::RenderOptimized(target, batch, optimized);

  return write_png(outPath, target) ? 0 : 1;
}
