#include "PrimeStage/Render.h"
#include "PrimeStage/Ui.h"

#if defined(PRIMESTAGE_HAS_PRIMEMANIFEST)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "third_party/stb_image_write.h"

#include "PrimeFrame/Flatten.h"

#include "PrimeManifest/renderer/Optimizer2D.hpp"
#include "PrimeManifest/renderer/Renderer2D.hpp"
#include "PrimeManifest/text/FontRegistry.hpp"
#include "PrimeManifest/text/TextBake.hpp"
#include "PrimeManifest/text/Typography.hpp"
#include "PrimeManifest/util/BitmapFont.hpp"
#endif

#include <algorithm>
#include <cmath>
#include <span>
#include <string>
#include <vector>

namespace PrimeStage {

#if defined(PRIMESTAGE_HAS_PRIMEMANIFEST)
namespace {

struct ClipRect {
  int32_t x0 = 0;
  int32_t y0 = 0;
  int32_t x1 = 0;
  int32_t y1 = 0;
  bool enabled = false;
};

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

void add_rect(PrimeManifest::RenderBatch& batch, PrimeFrame::DrawCommand const& cmd, float radiusPx) {
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

void apply_text_clip(PrimeManifest::RenderBatch& batch, uint32_t textIndex, ClipRect clip) {
  if (!clip.enabled || textIndex >= batch.text.clipX0.size()) {
    return;
  }
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
  if (text.empty()) {
    return;
  }
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
        if (!PrimeManifest::UiFontPixel(c, px, py)) {
          continue;
        }
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

PrimeManifest::Typography make_typography(PrimeFrame::ResolvedTextStyle const& style) {
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

bool write_png(std::string_view path, PrimeManifest::RenderTarget const& target) {
  if (target.width == 0 || target.height == 0 || target.data.empty()) {
    return false;
  }
  return stbi_write_png(std::string(path).c_str(),
                        static_cast<int>(target.width),
                        static_cast<int>(target.height),
                        4,
                        target.data.data(),
                        static_cast<int>(target.width * 4)) != 0;
}

bool colors_close(PrimeFrame::Color const& a, PrimeFrame::Color const& b) {
  float eps = 0.02f;
  return std::abs(a.r - b.r) < eps && std::abs(a.g - b.g) < eps &&
         std::abs(a.b - b.b) < eps && std::abs(a.a - b.a) < eps;
}

PrimeFrame::Color theme_color(PrimeFrame::Theme const* theme, size_t index) {
  if (!theme || theme->palette.empty() || index >= theme->palette.size()) {
    return PrimeFrame::Color{0.0f, 0.0f, 0.0f, 1.0f};
  }
  return theme->palette[index];
}

void ensure_fonts_loaded() {
  static bool loaded = false;
  if (loaded) {
    return;
  }
  auto& registry = PrimeManifest::GetFontRegistry();
#if defined(PRIMESTAGE_HAS_BUNDLED_FONT) && PRIMESTAGE_HAS_BUNDLED_FONT
  registry.addBundleDir(PRIMESTAGE_BUNDLED_FONT_DIR);
#endif
  registry.loadBundledFonts();
  registry.loadOsFallbackFonts();
  loaded = true;
}

bool compute_target_size(PrimeFrame::Frame const& frame,
                         PrimeFrame::LayoutOutput const& layout,
                         uint32_t& outW,
                         uint32_t& outH) {
  auto const& roots = frame.roots();
  if (roots.empty()) {
    return false;
  }
  float maxX = 0.0f;
  float maxY = 0.0f;
  for (PrimeFrame::NodeId rootId : roots) {
    PrimeFrame::LayoutOut const* out = layout.get(rootId);
    if (!out) {
      continue;
    }
    maxX = std::max(maxX, out->absX + out->absW);
    maxY = std::max(maxY, out->absY + out->absH);
  }
  outW = static_cast<uint32_t>(std::lround(maxX));
  outH = static_cast<uint32_t>(std::lround(maxY));
  return outW > 0 && outH > 0;
}

} // namespace

bool renderFrameToTarget(PrimeFrame::Frame& frame,
                         PrimeFrame::LayoutOutput const& layout,
                         RenderTarget const& target,
                         RenderOptions const& options) {
  if (target.width == 0 || target.height == 0 || target.pixels.empty()) {
    return false;
  }
  if (target.stride < target.width * 4) {
    return false;
  }

  ensure_fonts_loaded();

  float scale = target.scale > 0.0f ? target.scale : 1.0f;

  PrimeFrame::RenderBatch pfBatch;
  PrimeFrame::flattenToRenderBatch(frame, layout, pfBatch);

  PrimeManifest::RenderBatch batch;
  batch.assumeFrontToBack = false;
  if (options.clear) {
    PrimeManifest::Color clear{options.clearColor.r, options.clearColor.g,
                               options.clearColor.b, options.clearColor.a};
    add_clear(batch, PrimeManifest::PackRGBA8(clear));
  }

  PrimeFrame::Theme const* theme = frame.getTheme(PrimeFrame::DefaultThemeId);

  for (PrimeFrame::DrawCommand const& cmd : pfBatch.commands) {
    if (cmd.type == PrimeFrame::CommandType::Rect ||
        cmd.type == PrimeFrame::CommandType::ImagePlaceholder) {
      float logicalW = static_cast<float>(cmd.x1 - cmd.x0);
      float logicalH = static_cast<float>(cmd.y1 - cmd.y0);
      float radius = 0.0f;
      if (options.roundedCorners) {
        if (logicalH <= 6.0f && colors_close(cmd.rectStyle.fill, theme_color(theme, 11))) {
          radius = 4.0f;
        } else if (logicalH <= 6.0f && colors_close(cmd.rectStyle.fill, theme_color(theme, 10))) {
          radius = 3.0f;
        } else if (logicalW <= 12.0f && logicalH <= 12.0f &&
                   colors_close(cmd.rectStyle.fill, theme_color(theme, 7))) {
          radius = 2.0f;
        } else if (logicalH >= 30.0f && logicalH <= 34.0f) {
          if (colors_close(cmd.rectStyle.fill, theme_color(theme, 8))) {
            radius = 6.0f;
          } else if (colors_close(cmd.rectStyle.fill, theme_color(theme, 5))) {
            if (logicalW <= 140.0f || logicalW >= 300.0f) {
              radius = 6.0f;
            }
          }
        } else if (logicalH >= 110.0f && logicalH <= 130.0f &&
                   colors_close(cmd.rectStyle.fill, theme_color(theme, 6))) {
          radius = 4.0f;
        }
      }
      PrimeFrame::DrawCommand scaled = cmd;
      scaled.x0 = static_cast<int>(std::lround(static_cast<float>(cmd.x0) * scale));
      scaled.y0 = static_cast<int>(std::lround(static_cast<float>(cmd.y0) * scale));
      scaled.x1 = static_cast<int>(std::lround(static_cast<float>(cmd.x1) * scale));
      scaled.y1 = static_cast<int>(std::lround(static_cast<float>(cmd.y1) * scale));
      if (cmd.clipEnabled) {
        scaled.clip.x0 = static_cast<int>(std::lround(static_cast<float>(cmd.clip.x0) * scale));
        scaled.clip.y0 = static_cast<int>(std::lround(static_cast<float>(cmd.clip.y0) * scale));
        scaled.clip.x1 = static_cast<int>(std::lround(static_cast<float>(cmd.clip.x1) * scale));
        scaled.clip.y1 = static_cast<int>(std::lround(static_cast<float>(cmd.clip.y1) * scale));
      }
      add_rect(batch, scaled, radius * scale);
    }
  }

  for (PrimeFrame::DrawCommand const& cmd : pfBatch.commands) {
    if (cmd.type != PrimeFrame::CommandType::Text) {
      continue;
    }
    PrimeManifest::Typography type = make_typography(cmd.textStyle);
    type.size *= scale;
    type.lineHeight *= scale;
    type.tracking *= scale;
    uint32_t packed = pack_color(cmd.textStyle.color, 1.0f);
    uint8_t colorIndex = palette_index(batch, packed);
    ClipRect clip;
    if (cmd.clipEnabled) {
      clip.x0 = static_cast<int32_t>(std::lround(static_cast<float>(cmd.clip.x0) * scale));
      clip.y0 = static_cast<int32_t>(std::lround(static_cast<float>(cmd.clip.y0) * scale));
      clip.x1 = static_cast<int32_t>(std::lround(static_cast<float>(cmd.clip.x1) * scale));
      clip.y1 = static_cast<int32_t>(std::lround(static_cast<float>(cmd.clip.y1) * scale));
      clip.enabled = true;
    }
    int32_t textX = static_cast<int32_t>(std::lround(static_cast<float>(cmd.x0) * scale));
    int32_t textY = static_cast<int32_t>(std::lround(static_cast<float>(cmd.y0) * scale));

    uint8_t flags = clip.enabled ? PrimeManifest::TextFlagClip : 0u;
    auto result = PrimeManifest::AppendText(batch,
                                            cmd.text,
                                            type,
                                            1.0f,
                                            textX,
                                            textY,
                                            colorIndex,
                                            255,
                                            flags);
    if (result) {
      apply_text_clip(batch, result->textIndex, clip);
    } else {
      float fallbackSize = std::max(10.0f * scale, type.size * 0.9f);
      add_bitmap_text(batch, cmd.text, textX, textY, fallbackSize, colorIndex, clip);
    }
  }

  PrimeManifest::RenderTarget pmTarget{std::span<uint8_t>(target.pixels),
                                       target.width,
                                       target.height,
                                       target.stride};
  PrimeManifest::OptimizedBatch optimized;
  PrimeManifest::OptimizeRenderBatch(pmTarget, batch, optimized);
  PrimeManifest::RenderOptimized(pmTarget, batch, optimized);
  return true;
}

bool renderFrameToTarget(PrimeFrame::Frame& frame,
                         RenderTarget const& target,
                         RenderOptions const& options) {
  PrimeFrame::LayoutEngine engine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions layoutOptions;
  float scale = target.scale > 0.0f ? target.scale : 1.0f;
  if (target.width > 0 && target.height > 0) {
    layoutOptions.rootWidth = static_cast<float>(target.width) / scale;
    layoutOptions.rootHeight = static_cast<float>(target.height) / scale;
  }
  engine.layout(frame, layout, layoutOptions);
  return renderFrameToTarget(frame, layout, target, options);
}

bool renderFrameToPng(PrimeFrame::Frame& frame,
                      PrimeFrame::LayoutOutput const& layout,
                      std::string_view path,
                      RenderOptions const& options) {
  uint32_t widthPx = 0;
  uint32_t heightPx = 0;
  if (!compute_target_size(frame, layout, widthPx, heightPx)) {
    return false;
  }
  std::vector<uint8_t> buffer(widthPx * heightPx * 4, 0);
  RenderTarget target;
  target.pixels = std::span<uint8_t>(buffer);
  target.width = widthPx;
  target.height = heightPx;
  target.stride = widthPx * 4;
  target.scale = 1.0f;

  if (!renderFrameToTarget(frame, layout, target, options)) {
    return false;
  }
  PrimeManifest::RenderTarget pmTarget{std::span<uint8_t>(buffer), widthPx, heightPx, widthPx * 4};
  return write_png(path, pmTarget);
}

bool renderFrameToPng(PrimeFrame::Frame& frame,
                      std::string_view path,
                      RenderOptions const& options) {
  PrimeFrame::LayoutEngine engine;
  PrimeFrame::LayoutOutput layout;
  engine.layout(frame, layout);
  return renderFrameToPng(frame, layout, path, options);
}

#else

bool renderFrameToTarget(PrimeFrame::Frame&, PrimeFrame::LayoutOutput const&, RenderTarget const&,
                         RenderOptions const&) {
  return false;
}

bool renderFrameToTarget(PrimeFrame::Frame&, RenderTarget const&, RenderOptions const&) {
  return false;
}

bool renderFrameToPng(PrimeFrame::Frame&, PrimeFrame::LayoutOutput const&, std::string_view,
                      RenderOptions const&) {
  return false;
}

bool renderFrameToPng(PrimeFrame::Frame&, std::string_view, RenderOptions const&) {
  return false;
}

#endif

} // namespace PrimeStage
