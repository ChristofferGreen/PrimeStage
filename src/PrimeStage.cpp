#include "PrimeStage/StudioUi.h"
#include "PrimeStage/PrimeStage.h"
#include "PrimeFrame/Events.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <memory>
#include <utility>

#if defined(PRIMESTAGE_HAS_PRIMEMANIFEST)
#include "PrimeManifest/text/FontRegistry.hpp"
#include "PrimeManifest/text/Typography.hpp"
#endif

namespace PrimeStage {
namespace {

struct Rect {
  float x = 0.0f;
  float y = 0.0f;
  float width = 0.0f;
  float height = 0.0f;
};

void apply_rect(PrimeFrame::Node& node, Rect const& rect) {
  node.localX = rect.x;
  node.localY = rect.y;
  if (rect.width > 0.0f) {
    node.sizeHint.width.preferred = rect.width;
  } else {
    node.sizeHint.width.preferred.reset();
  }
  if (rect.height > 0.0f) {
    node.sizeHint.height.preferred = rect.height;
  } else {
    node.sizeHint.height.preferred.reset();
  }
}

void apply_size_spec(PrimeFrame::Node& node, SizeSpec const& size) {
  node.sizeHint.width.min = size.minWidth;
  node.sizeHint.width.max = size.maxWidth;
  if (!node.sizeHint.width.preferred.has_value() && size.preferredWidth.has_value()) {
    node.sizeHint.width.preferred = size.preferredWidth;
  }
  node.sizeHint.width.stretch = size.stretchX;

  node.sizeHint.height.min = size.minHeight;
  node.sizeHint.height.max = size.maxHeight;
  if (!node.sizeHint.height.preferred.has_value() && size.preferredHeight.has_value()) {
    node.sizeHint.height.preferred = size.preferredHeight;
  }
  node.sizeHint.height.stretch = size.stretchY;
}

Rect resolve_rect(SizeSpec const& size) {
  Rect resolved;
  if (size.preferredWidth.has_value()) {
    resolved.width = *size.preferredWidth;
  }
  if (size.preferredHeight.has_value()) {
    resolved.height = *size.preferredHeight;
  }
  return resolved;
}

float slider_value_from_event(PrimeFrame::Event const& event, bool vertical, float thumbSize) {
  float width = std::max(0.0f, event.targetW);
  float height = std::max(0.0f, event.targetH);
  float thumb = std::max(0.0f, thumbSize);
  float clampedThumb = std::min(thumb, std::min(width, height));
  if (vertical) {
    float range = std::max(0.0f, height - clampedThumb);
    if (range <= 0.0f) {
      return 0.0f;
    }
    float pos = std::clamp(event.localY - clampedThumb * 0.5f, 0.0f, range);
    return std::clamp(1.0f - (pos / range), 0.0f, 1.0f);
  }
  float range = std::max(0.0f, width - clampedThumb);
  if (range <= 0.0f) {
    return 0.0f;
  }
  float pos = std::clamp(event.localX - clampedThumb * 0.5f, 0.0f, range);
  return std::clamp(pos / range, 0.0f, 1.0f);
}

PrimeFrame::NodeId create_node(PrimeFrame::Frame& frame,
                               PrimeFrame::NodeId parent,
                               Rect const& rect,
                               SizeSpec const* size,
                               PrimeFrame::LayoutType layout,
                               PrimeFrame::Insets const& padding,
                               float gap,
                               bool clipChildren,
                               bool visible) {
  PrimeFrame::NodeId id = frame.createNode();
  PrimeFrame::Node* node = frame.getNode(id);
  if (!node) {
    return id;
  }
  apply_rect(*node, rect);
  if (size) {
    apply_size_spec(*node, *size);
  }
  node->layout = layout;
  node->padding = padding;
  node->gap = gap;
  node->clipChildren = clipChildren;
  node->visible = visible;
  if (parent.isValid()) {
    frame.addChild(parent, id);
  } else {
    frame.addRoot(id);
  }
  return id;
}

void add_rect_primitive(PrimeFrame::Frame& frame,
                        PrimeFrame::NodeId nodeId,
                        PrimeFrame::RectStyleToken token,
                        PrimeFrame::RectStyleOverride const& overrideStyle) {
  PrimeFrame::Primitive prim;
  prim.type = PrimeFrame::PrimitiveType::Rect;
  prim.rect.token = token;
  prim.rect.overrideStyle = overrideStyle;
  PrimeFrame::PrimitiveId pid = frame.addPrimitive(prim);
  if (PrimeFrame::Node* node = frame.getNode(nodeId)) {
    node->primitives.push_back(pid);
  }
}

PrimeFrame::PrimitiveId add_rect_primitive_with_rect(PrimeFrame::Frame& frame,
                                                     PrimeFrame::NodeId nodeId,
                                                     Rect const& rect,
                                                     PrimeFrame::RectStyleToken token,
                                                     PrimeFrame::RectStyleOverride const& overrideStyle) {
  PrimeFrame::Primitive prim;
  prim.type = PrimeFrame::PrimitiveType::Rect;
  prim.offsetX = rect.x;
  prim.offsetY = rect.y;
  prim.width = rect.width;
  prim.height = rect.height;
  prim.rect.token = token;
  prim.rect.overrideStyle = overrideStyle;
  PrimeFrame::PrimitiveId pid = frame.addPrimitive(prim);
  if (PrimeFrame::Node* node = frame.getNode(nodeId)) {
    node->primitives.push_back(pid);
  }
  return pid;
}

void add_text_primitive(PrimeFrame::Frame& frame,
                        PrimeFrame::NodeId nodeId,
                        std::string_view text,
                        PrimeFrame::TextStyleToken textStyle,
                        PrimeFrame::TextStyleOverride const& overrideStyle,
                        PrimeFrame::TextAlign align,
                        PrimeFrame::WrapMode wrap,
                        float maxWidth,
                        float width,
                        float height) {
  PrimeFrame::Primitive prim;
  prim.type = PrimeFrame::PrimitiveType::Text;
  prim.width = width;
  prim.height = height;
  prim.textBlock.text = std::string(text);
  prim.textBlock.align = align;
  prim.textBlock.wrap = wrap;
  prim.textBlock.maxWidth = maxWidth;
  prim.textStyle.token = textStyle;
  prim.textStyle.overrideStyle = overrideStyle;
  PrimeFrame::PrimitiveId pid = frame.addPrimitive(prim);
  if (PrimeFrame::Node* node = frame.getNode(nodeId)) {
    node->primitives.push_back(pid);
  }
}

PrimeFrame::NodeId create_rect_node(PrimeFrame::Frame& frame,
                                    PrimeFrame::NodeId parent,
                                    Rect const& rect,
                                    PrimeFrame::RectStyleToken token,
                                    PrimeFrame::RectStyleOverride const& overrideStyle,
                                    bool clipChildren,
                                    bool visible) {
  PrimeFrame::NodeId id = create_node(frame,
                                      parent,
                                      rect,
                                      nullptr,
                                      PrimeFrame::LayoutType::None,
                                      PrimeFrame::Insets{},
                                      0.0f,
                                      clipChildren,
                                      visible);
  PrimeFrame::Node* node = frame.getNode(id);
  if (node) {
    node->hitTestVisible = false;
  }
  add_rect_primitive(frame, id, token, overrideStyle);
  return id;
}

PrimeFrame::NodeId create_text_node(PrimeFrame::Frame& frame,
                                    PrimeFrame::NodeId parent,
                                    Rect const& rect,
                                    std::string_view text,
                                    PrimeFrame::TextStyleToken textStyle,
                                    PrimeFrame::TextStyleOverride const& overrideStyle,
                                    PrimeFrame::TextAlign align,
                                    PrimeFrame::WrapMode wrap,
                                    float maxWidth,
                                    bool visible) {
  PrimeFrame::NodeId id = create_node(frame,
                                      parent,
                                      rect,
                                      nullptr,
                                      PrimeFrame::LayoutType::None,
                                      PrimeFrame::Insets{},
                                      0.0f,
                                      false,
                                      visible);
  PrimeFrame::Node* node = frame.getNode(id);
  if (node) {
    node->hitTestVisible = false;
  }
  add_text_primitive(frame,
                     id,
                     text,
                     textStyle,
                     overrideStyle,
                     align,
                     wrap,
                     maxWidth,
                     rect.width,
                     rect.height);
  return id;
}

float resolve_line_height(PrimeFrame::Frame& frame, PrimeFrame::TextStyleToken token) {
  PrimeFrame::Theme const* theme = frame.getTheme(PrimeFrame::DefaultThemeId);
  if (!theme) {
    return 0.0f;
  }
  PrimeFrame::ResolvedTextStyle resolved = PrimeFrame::resolveTextStyle(*theme, token, {});
  float lineHeight = resolved.lineHeight > 0.0f ? resolved.lineHeight : resolved.size * 1.2f;
  return lineHeight;
}

float estimate_text_width(PrimeFrame::Frame& frame,
                          PrimeFrame::TextStyleToken token,
                          std::string_view text) {
  PrimeFrame::Theme const* theme = frame.getTheme(PrimeFrame::DefaultThemeId);
  if (!theme) {
    return 0.0f;
  }
  PrimeFrame::ResolvedTextStyle resolved = PrimeFrame::resolveTextStyle(*theme, token, {});
  float advance = resolved.size * 0.6f + resolved.tracking;
  float lineWidth = 0.0f;
  float maxWidth = 0.0f;
  for (char ch : text) {
    if (ch == '\n') {
      maxWidth = std::max(maxWidth, lineWidth);
      lineWidth = 0.0f;
      continue;
    }
    lineWidth += advance;
  }
  maxWidth = std::max(maxWidth, lineWidth);
  return maxWidth;
}

int estimate_wrapped_lines(PrimeFrame::Frame& frame,
                           PrimeFrame::TextStyleToken token,
                           std::string_view text,
                           float maxWidth,
                           PrimeFrame::WrapMode wrap) {
  if (text.empty()) {
    return 0;
  }

  if (maxWidth <= 0.0f || wrap == PrimeFrame::WrapMode::None) {
    int lines = 1;
    for (char ch : text) {
      if (ch == '\n') {
        ++lines;
      }
    }
    return lines;
  }

  float spaceWidth = measureTextWidth(frame, token, " ");
  float lineWidth = 0.0f;
  int lines = 1;
  std::string word;

  auto flush_word = [&]() {
    if (word.empty()) {
      return;
    }
    float wordWidth = measureTextWidth(frame, token, word);
    if (lineWidth > 0.0f && lineWidth + spaceWidth + wordWidth > maxWidth) {
      ++lines;
      lineWidth = wordWidth;
    } else {
      if (lineWidth > 0.0f) {
        lineWidth += spaceWidth;
      }
      lineWidth += wordWidth;
    }
    word.clear();
  };

  for (char ch : text) {
    if (ch == '\n') {
      flush_word();
      ++lines;
      lineWidth = 0.0f;
      continue;
    }
    if (std::isspace(static_cast<unsigned char>(ch))) {
      flush_word();
      continue;
    }
    word.push_back(ch);
  }
  flush_word();

  return lines;
}

std::vector<std::string> wrap_text_lines(PrimeFrame::Frame& frame,
                                         PrimeFrame::TextStyleToken token,
                                         std::string_view text,
                                         float maxWidth,
                                         PrimeFrame::WrapMode wrap) {
  std::vector<std::string> lines;
  if (text.empty()) {
    return lines;
  }

  if (maxWidth <= 0.0f || wrap == PrimeFrame::WrapMode::None) {
    std::string current;
    for (char ch : text) {
      if (ch == '\n') {
        lines.push_back(current);
        current.clear();
        continue;
      }
      current.push_back(ch);
    }
    if (!current.empty() || (!text.empty() && text.back() == '\n')) {
      lines.push_back(current);
    }
    return lines;
  }

  float spaceWidth = estimate_text_width(frame, token, " ");
  float lineWidth = 0.0f;
  std::string current;
  std::string word;
  bool wrapByChar = wrap == PrimeFrame::WrapMode::Character;

  auto flush_word = [&]() {
    if (word.empty()) {
      return;
    }
    float wordWidth = estimate_text_width(frame, token, word);
    if (!current.empty() && lineWidth + spaceWidth + wordWidth > maxWidth) {
      lines.push_back(current);
      current.clear();
      lineWidth = 0.0f;
    }
    if (!current.empty()) {
      current.push_back(' ');
      lineWidth += spaceWidth;
    }
    current += word;
    lineWidth += wordWidth;
    word.clear();
  };

  for (char ch : text) {
    if (ch == '\n') {
      flush_word();
      lines.push_back(current);
      current.clear();
      lineWidth = 0.0f;
      continue;
    }
    if (std::isspace(static_cast<unsigned char>(ch))) {
      flush_word();
      continue;
    }
    word.push_back(ch);
    if (wrapByChar) {
      flush_word();
    }
  }
  flush_word();
  if (!current.empty()) {
    lines.push_back(current);
  }

  return lines;
}

struct FlatTreeRow {
  std::string_view label;
  int depth = 0;
  int parentIndex = -1;
  bool hasChildren = false;
  bool expanded = true;
  bool selected = false;
  std::vector<int> ancestors;
};

void flatten_tree(std::vector<TreeNode> const& nodes,
                  int depth,
                  std::vector<int>& depthStack,
                  std::vector<FlatTreeRow>& out) {
  for (TreeNode const& node : nodes) {
    int parentIndex = depth > 0 && depth - 1 < static_cast<int>(depthStack.size())
                          ? depthStack[static_cast<size_t>(depth - 1)]
                          : -1;
    FlatTreeRow row;
    row.label = node.label;
    row.depth = depth;
    row.parentIndex = parentIndex;
    row.hasChildren = !node.children.empty();
    row.expanded = node.expanded;
    row.selected = node.selected;
    if (depth > 0 && depth <= static_cast<int>(depthStack.size())) {
      row.ancestors.assign(depthStack.begin(), depthStack.begin() + depth);
    }
    int index = static_cast<int>(out.size());
    out.push_back(std::move(row));

    if (depth >= static_cast<int>(depthStack.size())) {
      depthStack.resize(static_cast<size_t>(depth) + 1, -1);
    }
    depthStack[static_cast<size_t>(depth)] = index;

    if (node.expanded && !node.children.empty()) {
      flatten_tree(node.children, depth + 1, depthStack, out);
    }
  }
}

void add_divider_rect(PrimeFrame::Frame& frame,
                      PrimeFrame::NodeId nodeId,
                      Rect const& bounds,
                      PrimeFrame::RectStyleToken token) {
  PrimeFrame::NodeId id = create_node(frame, nodeId, bounds,
                                      nullptr,
                                      PrimeFrame::LayoutType::None,
                                      PrimeFrame::Insets{},
                                      0.0f,
                                      false,
                                      true);
  add_rect_primitive(frame, id, token, {});
}

} // namespace

void setScrollBarThumbPixels(ScrollBarSpec& spec,
                             float trackHeight,
                             float thumbHeight,
                             float thumbOffset) {
  spec.autoThumb = false;
  float track = std::max(1.0f, trackHeight);
  float thumb = std::max(0.0f, std::min(thumbHeight, track));
  float maxOffset = std::max(1.0f, track - thumb);
  spec.thumbFraction = std::clamp(thumb / track, 0.0f, 1.0f);
  spec.thumbProgress = std::clamp(thumbOffset / maxOffset, 0.0f, 1.0f);
}

float measureTextWidth(PrimeFrame::Frame& frame,
                       PrimeFrame::TextStyleToken token,
                       std::string_view text) {
  if (text.empty()) {
    return 0.0f;
  }
  PrimeFrame::Theme const* theme = frame.getTheme(PrimeFrame::DefaultThemeId);
  if (!theme) {
    return 0.0f;
  }
  PrimeFrame::ResolvedTextStyle resolved = PrimeFrame::resolveTextStyle(*theme, token, {});
#if defined(PRIMESTAGE_HAS_PRIMEMANIFEST)
  static bool fontsLoaded = false;
  auto& registry = PrimeManifest::GetFontRegistry();
  if (!fontsLoaded) {
#if defined(PRIMESTAGE_HAS_BUNDLED_FONT) && PRIMESTAGE_HAS_BUNDLED_FONT
    registry.addBundleDir(PRIMESTAGE_BUNDLED_FONT_DIR);
#endif
    registry.loadBundledFonts();
    registry.loadOsFallbackFonts();
    fontsLoaded = true;
  }
  PrimeManifest::Typography typography;
  typography.size = resolved.size;
  typography.weight = static_cast<int>(std::lround(resolved.weight));
  typography.lineHeight = resolved.lineHeight > 0.0f ? resolved.lineHeight : resolved.size * 1.2f;
  typography.letterSpacing = resolved.tracking;
  if (resolved.slant != 0.0f) {
    typography.slant = PrimeManifest::FontSlant::Italic;
  }
#if defined(PRIMESTAGE_HAS_BUNDLED_FONT) && PRIMESTAGE_HAS_BUNDLED_FONT
  typography.fallback = PrimeManifest::FontFallbackPolicy::BundleOnly;
#else
  typography.fallback = PrimeManifest::FontFallbackPolicy::BundleThenOS;
#endif
  auto measured = registry.measureText(text, typography);
  return static_cast<float>(measured.first);
#else
  float advance = resolved.size * 0.6f + resolved.tracking;
  float lineWidth = 0.0f;
  float maxWidth = 0.0f;
  for (char ch : text) {
    if (ch == '\n') {
      maxWidth = std::max(maxWidth, lineWidth);
      lineWidth = 0.0f;
      continue;
    }
    lineWidth += advance;
  }
  maxWidth = std::max(maxWidth, lineWidth);
  return maxWidth;
#endif
}

namespace Studio {

PrimeFrame::RectStyleToken rectToken(RectRole role) {
  return static_cast<PrimeFrame::RectStyleToken>(role);
}

PrimeFrame::TextStyleToken textToken(TextRole role) {
  return static_cast<PrimeFrame::TextStyleToken>(role);
}

void applyStudioTheme(PrimeFrame::Frame& frame) {
  PrimeFrame::Theme* theme = frame.getTheme(PrimeFrame::DefaultThemeId);
  if (!theme) {
    return;
  }
  theme->name = "Studio";
  theme->palette = {
      PrimeFrame::Color{0.07f, 0.09f, 0.13f, 1.0f}, // Background
      PrimeFrame::Color{0.12f, 0.15f, 0.22f, 1.0f}, // Topbar
      PrimeFrame::Color{0.10f, 0.12f, 0.18f, 1.0f}, // Statusbar
      PrimeFrame::Color{0.11f, 0.13f, 0.19f, 1.0f}, // Sidebar/Inspector
      PrimeFrame::Color{0.13f, 0.16f, 0.22f, 1.0f}, // Content
      PrimeFrame::Color{0.16f, 0.19f, 0.26f, 1.0f}, // Panel
      PrimeFrame::Color{0.18f, 0.21f, 0.30f, 1.0f}, // Panel Alt
      PrimeFrame::Color{0.22f, 0.26f, 0.36f, 1.0f}, // Panel Strong
      PrimeFrame::Color{0.35f, 0.64f, 1.0f, 1.0f},  // Accent
      PrimeFrame::Color{0.25f, 0.38f, 0.60f, 1.0f}, // Selection
      PrimeFrame::Color{0.10f, 0.12f, 0.16f, 1.0f}, // Scroll Track
      PrimeFrame::Color{0.24f, 0.28f, 0.39f, 1.0f}, // Scroll Thumb
      PrimeFrame::Color{0.08f, 0.10f, 0.15f, 1.0f}, // Divider
      PrimeFrame::Color{0.925f, 0.937f, 0.957f, 1.0f}, // Text Bright
      PrimeFrame::Color{0.588f, 0.620f, 0.667f, 1.0f}, // Text Muted
      PrimeFrame::Color{0.20f, 0.24f, 0.34f, 1.0f}, // Button Base
      PrimeFrame::Color{0.24f, 0.29f, 0.41f, 1.0f}, // Button Hover
      PrimeFrame::Color{0.16f, 0.20f, 0.30f, 1.0f}, // Button Pressed
      PrimeFrame::Color{0.35f, 0.64f, 1.0f, 1.0f},  // Button Primary
      PrimeFrame::Color{0.42f, 0.72f, 1.0f, 1.0f},  // Button Primary Hover
      PrimeFrame::Color{0.28f, 0.55f, 0.92f, 1.0f}  // Button Primary Pressed
  };
  theme->rectStyles = {
      PrimeFrame::RectStyle{0, 1.0f},
      PrimeFrame::RectStyle{1, 1.0f},
      PrimeFrame::RectStyle{2, 1.0f},
      PrimeFrame::RectStyle{3, 1.0f},
      PrimeFrame::RectStyle{4, 1.0f},
      PrimeFrame::RectStyle{3, 1.0f},
      PrimeFrame::RectStyle{5, 1.0f},
      PrimeFrame::RectStyle{6, 1.0f},
      PrimeFrame::RectStyle{7, 1.0f},
      PrimeFrame::RectStyle{8, 1.0f},
      PrimeFrame::RectStyle{9, 0.55f},
      PrimeFrame::RectStyle{10, 1.0f},
      PrimeFrame::RectStyle{11, 1.0f},
      PrimeFrame::RectStyle{12, 1.0f},
      PrimeFrame::RectStyle{15, 1.0f},
      PrimeFrame::RectStyle{16, 1.0f},
      PrimeFrame::RectStyle{17, 1.0f},
      PrimeFrame::RectStyle{18, 1.0f},
      PrimeFrame::RectStyle{19, 1.0f},
      PrimeFrame::RectStyle{20, 1.0f}
  };

  PrimeFrame::TextStyle titleText;
  titleText.size = 18.0f;
  titleText.weight = 600.0f;
  titleText.lineHeight = titleText.size * 1.2f;
  titleText.color = 13;
  PrimeFrame::TextStyle bodyText;
  bodyText.size = 14.0f;
  bodyText.weight = 500.0f;
  bodyText.lineHeight = bodyText.size * 1.4f;
  bodyText.color = 13;
  PrimeFrame::TextStyle bodyMuted = bodyText;
  bodyMuted.color = 14;
  PrimeFrame::TextStyle smallText;
  smallText.size = 12.0f;
  smallText.weight = 500.0f;
  smallText.lineHeight = smallText.size * 1.3f;
  smallText.color = 13;
  PrimeFrame::TextStyle smallMuted = smallText;
  smallMuted.color = 14;

  theme->textStyles = {titleText, bodyText, bodyMuted, smallText, smallMuted};
}

bool is_default_theme(PrimeFrame::Theme const& theme) {
  if (theme.name != "Default") {
    return false;
  }
  if (theme.palette.size() != 1 || theme.rectStyles.size() != 1 || theme.textStyles.size() != 1) {
    return false;
  }
  PrimeFrame::Color const& color = theme.palette[0];
  if (color.r != 0.0f || color.g != 0.0f || color.b != 0.0f || color.a != 1.0f) {
    return false;
  }
  PrimeFrame::RectStyle const& rect = theme.rectStyles[0];
  if (rect.fill != 0 || rect.opacity != 1.0f) {
    return false;
  }
  PrimeFrame::TextStyle const& text = theme.textStyles[0];
  if (text.font != 0 || text.size != 14.0f || text.weight != 400.0f || text.slant != 0.0f ||
      text.lineHeight != 0.0f || text.tracking != 0.0f || text.color != 0) {
    return false;
  }
  return true;
}

static void ensure_studio_theme(PrimeFrame::Frame& frame) {
  PrimeFrame::Theme* theme = frame.getTheme(PrimeFrame::DefaultThemeId);
  if (!theme) {
    return;
  }
  if (theme->palette.empty() || theme->rectStyles.empty() || theme->textStyles.empty() ||
      is_default_theme(*theme)) {
    applyStudioTheme(frame);
  }
}

} // namespace Studio

UiNode::UiNode(PrimeFrame::Frame& frame, PrimeFrame::NodeId id, bool allowAbsolute)
    : frame_(frame), id_(id), allowAbsolute_(allowAbsolute) {}

UiNode& UiNode::setVisible(bool visible) {
  PrimeFrame::Node* node = frame().getNode(id_);
  if (!node) {
    return *this;
  }
  node->visible = visible;
  return *this;
}

UiNode& UiNode::setSize(SizeSpec const& size) {
  PrimeFrame::Node* node = frame().getNode(id_);
  if (!node) {
    return *this;
  }
  apply_size_spec(*node, size);
  return *this;
}

UiNode& UiNode::setHitTestVisible(bool visible) {
  PrimeFrame::Node* node = frame().getNode(id_);
  if (!node) {
    return *this;
  }
  node->hitTestVisible = visible;
  return *this;
}

UiNode UiNode::createVerticalStack(StackSpec const& spec) {
  PrimeFrame::NodeId nodeId = create_node(frame(), id_, Rect{},
                                          &spec.size,
                                          PrimeFrame::LayoutType::VerticalStack,
                                          spec.padding,
                                          spec.gap,
                                          spec.clipChildren,
                                          spec.visible);
  PrimeFrame::Node* node = frame().getNode(nodeId);
  if (node) {
    node->hitTestVisible = false;
  }
  return UiNode(frame(), nodeId, allowAbsolute_);
}

UiNode UiNode::createHorizontalStack(StackSpec const& spec) {
  PrimeFrame::NodeId nodeId = create_node(frame(), id_, Rect{},
                                          &spec.size,
                                          PrimeFrame::LayoutType::HorizontalStack,
                                          spec.padding,
                                          spec.gap,
                                          spec.clipChildren,
                                          spec.visible);
  PrimeFrame::Node* node = frame().getNode(nodeId);
  if (node) {
    node->hitTestVisible = false;
  }
  return UiNode(frame(), nodeId, allowAbsolute_);
}

UiNode UiNode::createOverlay(StackSpec const& spec) {
  PrimeFrame::NodeId nodeId = create_node(frame(), id_, Rect{},
                                          &spec.size,
                                          PrimeFrame::LayoutType::Overlay,
                                          spec.padding,
                                          spec.gap,
                                          spec.clipChildren,
                                          spec.visible);
  PrimeFrame::Node* node = frame().getNode(nodeId);
  if (node) {
    node->hitTestVisible = false;
  }
  return UiNode(frame(), nodeId, allowAbsolute_);
}

UiNode UiNode::createPanel(PanelSpec const& spec) {
  PrimeFrame::NodeId nodeId = create_node(frame(), id_, Rect{},
                                          &spec.size,
                                          spec.layout,
                                          spec.padding,
                                          spec.gap,
                                          spec.clipChildren,
                                          spec.visible);
  add_rect_primitive(frame(), nodeId, spec.rectStyle, spec.rectStyleOverride);
  return UiNode(frame(), nodeId, allowAbsolute_);
}

UiNode UiNode::createPanel(PrimeFrame::RectStyleToken rectStyle, SizeSpec const& size) {
  PanelSpec spec;
  spec.rectStyle = rectStyle;
  spec.size = size;
  return createPanel(spec);
}

namespace Studio {

UiNode createPanel(UiNode& parent, RectRole role, SizeSpec const& size) {
  return parent.createPanel(rectToken(role), size);
}

} // namespace Studio

UiNode UiNode::createLabel(LabelSpec const& spec) {
  Rect rect = resolve_rect(spec.size);
  if ((rect.width <= 0.0f || rect.height <= 0.0f) &&
      !spec.text.empty() &&
      !spec.size.preferredWidth.has_value() &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchX <= 0.0f &&
      spec.size.stretchY <= 0.0f) {
    float lineHeight = resolve_line_height(frame(), spec.textStyle);
    float textWidth = estimate_text_width(frame(), spec.textStyle, spec.text);
    if (rect.width <= 0.0f) {
      if (spec.maxWidth > 0.0f) {
        rect.width = std::min(textWidth, spec.maxWidth);
      } else {
        rect.width = textWidth;
      }
    }
    if (rect.height <= 0.0f) {
      float wrapWidth = rect.width;
      if (spec.maxWidth > 0.0f) {
        wrapWidth = spec.maxWidth;
      }
      float height = lineHeight;
      if (spec.wrap != PrimeFrame::WrapMode::None && wrapWidth > 0.0f) {
        std::vector<std::string> lines = wrap_text_lines(frame(), spec.textStyle, spec.text, wrapWidth, spec.wrap);
        height = lineHeight * std::max<size_t>(1, lines.size());
      }
      rect.height = height;
    }
  }
  PrimeFrame::NodeId nodeId = create_node(frame(), id_, rect,
                                          &spec.size,
                                          PrimeFrame::LayoutType::None,
                                          PrimeFrame::Insets{},
                                          0.0f,
                                          false,
                                          spec.visible);
  PrimeFrame::Node* node = frame().getNode(nodeId);
  if (node) {
    node->hitTestVisible = false;
  }
  add_text_primitive(frame(),
                     nodeId,
                     spec.text,
                     spec.textStyle,
                     spec.textStyleOverride,
                     spec.align,
                     spec.wrap,
                     spec.maxWidth,
                     rect.width,
                     rect.height);
  return UiNode(frame(), nodeId, allowAbsolute_);
}

UiNode UiNode::createLabel(std::string_view text,
                           PrimeFrame::TextStyleToken textStyle,
                           SizeSpec const& size) {
  LabelSpec spec;
  spec.text = text;
  spec.textStyle = textStyle;
  spec.size = size;
  return createLabel(spec);
}

namespace Studio {

UiNode createLabel(UiNode& parent, std::string_view text, TextRole role, SizeSpec const& size) {
  return parent.createLabel(text, textToken(role), size);
}

} // namespace Studio

UiNode UiNode::createParagraph(ParagraphSpec const& spec) {
  Rect bounds = resolve_rect(spec.size);
  PrimeFrame::TextStyleToken token = spec.textStyle;
  float maxWidth = spec.maxWidth > 0.0f ? spec.maxWidth : bounds.width;
  if (bounds.width <= 0.0f &&
      spec.maxWidth > 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f) {
    bounds.width = maxWidth;
  }
  std::vector<std::string> lines = wrap_text_lines(frame(), token, spec.text, maxWidth, spec.wrap);
  if (bounds.width <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f &&
      !lines.empty()) {
    float inferredWidth = 0.0f;
    for (auto const& line : lines) {
      inferredWidth = std::max(inferredWidth, measureTextWidth(frame(), token, line));
    }
    bounds.width = inferredWidth;
  }
  if (maxWidth <= 0.0f && bounds.width > 0.0f) {
    maxWidth = bounds.width;
  }

  float lineHeight = resolve_line_height(frame(), token);
  if (spec.autoHeight &&
      bounds.height <= 0.0f &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchY <= 0.0f) {
    bounds.height = std::max(0.0f, lineHeight * static_cast<float>(lines.size()));
  }

  PrimeFrame::NodeId paragraphId = create_node(frame(), id_, bounds,
                                               &spec.size,
                                               PrimeFrame::LayoutType::None,
                                               PrimeFrame::Insets{},
                                               0.0f,
                                               false,
                                               spec.visible);
  PrimeFrame::Node* node = frame().getNode(paragraphId);
  if (node) {
    node->hitTestVisible = false;
  }

  for (size_t i = 0; i < lines.size(); ++i) {
    Rect lineRect;
    lineRect.x = 0.0f;
    lineRect.y = spec.textOffsetY + static_cast<float>(i) * lineHeight;
    lineRect.width = maxWidth > 0.0f ? maxWidth : bounds.width;
    lineRect.height = lineHeight;
    create_text_node(frame(),
                     paragraphId,
                     lineRect,
                     lines[i],
                     token,
                     spec.textStyleOverride,
                     spec.align,
                     PrimeFrame::WrapMode::None,
                     maxWidth,
                     spec.visible);
  }

  return UiNode(frame(), paragraphId, allowAbsolute_);
}

UiNode UiNode::createParagraph(std::string_view text,
                               PrimeFrame::TextStyleToken textStyle,
                               SizeSpec const& size) {
  ParagraphSpec spec;
  spec.text = text;
  spec.textStyle = textStyle;
  spec.size = size;
  return createParagraph(spec);
}

namespace Studio {

UiNode createParagraph(UiNode& parent,
                       std::string_view text,
                       TextRole role,
                       SizeSpec const& size) {
  return parent.createParagraph(text, textToken(role), size);
}

} // namespace Studio

UiNode UiNode::createTextLine(TextLineSpec const& spec) {
  PrimeFrame::TextStyleToken token = spec.textStyle;
  float lineHeight = resolve_line_height(frame(), token);
  Rect bounds = resolve_rect(spec.size);
  if ((bounds.width <= 0.0f || bounds.height <= 0.0f) &&
      !spec.size.preferredWidth.has_value() &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchX <= 0.0f &&
      spec.size.stretchY <= 0.0f &&
      !spec.text.empty()) {
    float textWidth = estimate_text_width(frame(), token, spec.text);
    if (bounds.width <= 0.0f) {
      bounds.width = textWidth;
    }
    if (bounds.height <= 0.0f) {
      bounds.height = lineHeight;
    }
  }
  float containerHeight = bounds.height > 0.0f ? bounds.height : lineHeight;
  float textY = (containerHeight - lineHeight) * 0.5f + spec.textOffsetY;

  float textWidth = estimate_text_width(frame(), token, spec.text);
  float containerWidth = bounds.width;
  bool manualAlign = spec.align != PrimeFrame::TextAlign::Start &&
                     containerWidth > 0.0f &&
                     textWidth > 0.0f;

  PrimeFrame::NodeId lineId;
  if (manualAlign) {
    float offset = 0.0f;
    if (spec.align == PrimeFrame::TextAlign::Center) {
      offset = (containerWidth - textWidth) * 0.5f;
    } else if (spec.align == PrimeFrame::TextAlign::End) {
      offset = containerWidth - textWidth;
    }
    float x = offset;
    if (x < 0.0f) {
      x = 0.0f;
    }
    Rect textRect{x, textY, textWidth, lineHeight};
    lineId = create_text_node(frame(),
                              id_,
                              textRect,
                              spec.text,
                              token,
                              spec.textStyleOverride,
                              PrimeFrame::TextAlign::Start,
                              PrimeFrame::WrapMode::None,
                              textWidth,
                              spec.visible);
  } else {
    float width = containerWidth > 0.0f ? containerWidth : textWidth;
    Rect textRect{0.0f, textY, width, lineHeight};
    lineId = create_text_node(frame(),
                              id_,
                              textRect,
                              spec.text,
                              token,
                              spec.textStyleOverride,
                              spec.align,
                              PrimeFrame::WrapMode::None,
                              width,
                              spec.visible);
  }
  return UiNode(frame(), lineId, allowAbsolute_);
}

UiNode UiNode::createTextLine(std::string_view text,
                              PrimeFrame::TextStyleToken textStyle,
                              SizeSpec const& size,
                              PrimeFrame::TextAlign align) {
  TextLineSpec spec;
  spec.text = text;
  spec.textStyle = textStyle;
  spec.align = align;
  spec.size = size;
  return createTextLine(spec);
}

namespace Studio {

UiNode createTextLine(UiNode& parent,
                      std::string_view text,
                      TextRole role,
                      SizeSpec const& size,
                      PrimeFrame::TextAlign align) {
  return parent.createTextLine(text, textToken(role), size, align);
}

} // namespace Studio

namespace Studio {

UiNode createTable(UiNode& parent, TableSpec const& spec) {
  PrimeStage::TableSpec tableSpec = spec.base;
  tableSpec.headerStyle = rectToken(spec.headerRole);
  tableSpec.rowStyle = rectToken(spec.rowRole);
  tableSpec.rowAltStyle = rectToken(spec.rowAltRole);
  tableSpec.dividerStyle = rectToken(spec.dividerRole);

  if (!spec.columns.empty()) {
    tableSpec.columns.clear();
    tableSpec.columns.reserve(spec.columns.size());
    for (TableColumn const& col : spec.columns) {
      PrimeStage::TableColumn core;
      core.label = col.label;
      core.width = col.width;
      core.headerStyle = textToken(col.headerRole);
      core.cellStyle = textToken(col.cellRole);
      tableSpec.columns.push_back(core);
    }
  }

  return parent.createTable(tableSpec);
}

UiNode createTreeView(UiNode& parent, TreeViewSpec const& spec) {
  PrimeStage::TreeViewSpec treeSpec = spec.base;
  treeSpec.rowStyle = rectToken(spec.rowRole);
  treeSpec.rowAltStyle = rectToken(spec.rowAltRole);
  treeSpec.selectionStyle = rectToken(spec.selectionRole);
  treeSpec.selectionAccentStyle = rectToken(spec.selectionAccentRole);
  treeSpec.caretBackgroundStyle = rectToken(spec.caretBackgroundRole);
  treeSpec.caretLineStyle = rectToken(spec.caretLineRole);
  treeSpec.connectorStyle = rectToken(spec.connectorRole);
  treeSpec.textStyle = textToken(spec.textRole);
  treeSpec.selectedTextStyle = textToken(spec.selectedTextRole);

  if (treeSpec.scrollBar.trackStyle == 0) {
    treeSpec.scrollBar.trackStyle = rectToken(RectRole::ScrollTrack);
  }
  if (treeSpec.scrollBar.thumbStyle == 0) {
    treeSpec.scrollBar.thumbStyle = rectToken(RectRole::ScrollThumb);
  }

  return parent.createTreeView(treeSpec);
}

} // namespace Studio

UiNode UiNode::createDivider(DividerSpec const& spec) {
  Rect rect = resolve_rect(spec.size);
  PrimeFrame::NodeId nodeId = create_node(frame(), id_, rect,
                                          &spec.size,
                                          PrimeFrame::LayoutType::None,
                                          PrimeFrame::Insets{},
                                          0.0f,
                                          false,
                                          spec.visible);
  PrimeFrame::Node* node = frame().getNode(nodeId);
  if (node) {
    node->hitTestVisible = false;
  }
  add_rect_primitive(frame(), nodeId, spec.rectStyle, spec.rectStyleOverride);
  return UiNode(frame(), nodeId, allowAbsolute_);
}

UiNode UiNode::createDivider(PrimeFrame::RectStyleToken rectStyle, SizeSpec const& size) {
  DividerSpec spec;
  spec.rectStyle = rectStyle;
  spec.size = size;
  return createDivider(spec);
}

UiNode UiNode::createSpacer(SpacerSpec const& spec) {
  Rect rect = resolve_rect(spec.size);
  PrimeFrame::NodeId nodeId = create_node(frame(), id_, rect,
                                          &spec.size,
                                          PrimeFrame::LayoutType::None,
                                          PrimeFrame::Insets{},
                                          0.0f,
                                          false,
                                          spec.visible);
  PrimeFrame::Node* node = frame().getNode(nodeId);
  if (node) {
    node->hitTestVisible = false;
  }
  return UiNode(frame(), nodeId, allowAbsolute_);
}

UiNode UiNode::createSpacer(SizeSpec const& size) {
  SpacerSpec spec;
  spec.size = size;
  return createSpacer(spec);
}

UiNode UiNode::createButton(ButtonSpec const& spec) {
  Rect bounds = resolve_rect(spec.size);
  float lineHeight = resolve_line_height(frame(), spec.textStyle);
  if (bounds.height <= 0.0f &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchY <= 0.0f) {
    if (lineHeight > 0.0f) {
      bounds.height = lineHeight;
    }
  }
  if (bounds.width <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f &&
      !spec.label.empty()) {
    float textWidth = estimate_text_width(frame(), spec.textStyle, spec.label);
    bounds.width = std::max(bounds.width, textWidth + spec.textInsetX * 2.0f);
  }
  if (bounds.width <= 0.0f &&
      bounds.height <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchX <= 0.0f &&
      spec.size.stretchY <= 0.0f) {
    return UiNode(frame(), id_, allowAbsolute_);
  }
  PrimeFrame::RectStyleToken baseToken = spec.backgroundStyle;
  PrimeFrame::RectStyleToken hoverToken = spec.hoverStyle != 0 ? spec.hoverStyle : baseToken;
  PrimeFrame::RectStyleToken pressedToken = spec.pressedStyle != 0 ? spec.pressedStyle : baseToken;
  PrimeFrame::RectStyleOverride baseOverride = spec.backgroundStyleOverride;
  baseOverride.opacity = spec.baseOpacity;
  PrimeFrame::RectStyleOverride hoverOverride = spec.hoverStyleOverride;
  hoverOverride.opacity = spec.hoverOpacity;
  PrimeFrame::RectStyleOverride pressedOverride = spec.pressedStyleOverride;
  pressedOverride.opacity = spec.pressedOpacity;

  PanelSpec panel;
  panel.size = spec.size;
  if (!panel.size.preferredWidth.has_value() && bounds.width > 0.0f) {
    panel.size.preferredWidth = bounds.width;
  }
  if (!panel.size.preferredHeight.has_value() && bounds.height > 0.0f) {
    panel.size.preferredHeight = bounds.height;
  }
  panel.rectStyle = baseToken;
  panel.rectStyleOverride = baseOverride;
  panel.visible = spec.visible;
  UiNode button = createPanel(panel);
  if (spec.label.empty() || !spec.visible) {
    return UiNode(frame(), button.nodeId(), allowAbsolute_);
  }
  float textWidth = estimate_text_width(frame(), spec.textStyle, spec.label);
  float textY = (bounds.height - lineHeight) * 0.5f + spec.textOffsetY;
  float textX = spec.textInsetX;
  float labelWidth = std::max(0.0f, bounds.width - spec.textInsetX);
  PrimeFrame::TextAlign align = PrimeFrame::TextAlign::Start;
  if (spec.centerText) {
    textX = (bounds.width - textWidth) * 0.5f;
    if (textX < 0.0f) {
      textX = 0.0f;
    }
    labelWidth = std::max(0.0f, textWidth);
  }

  if (!spec.centerText && textWidth > 0.0f) {
    labelWidth = std::max(labelWidth, textWidth);
  }

  Rect textRect{textX, textY, labelWidth, lineHeight};
  create_text_node(frame(),
                   button.nodeId(),
                   textRect,
                   spec.label,
                   spec.textStyle,
                   spec.textStyleOverride,
                   align,
                   PrimeFrame::WrapMode::None,
                   labelWidth,
                   spec.visible);

  bool needsInteraction = spec.callbacks.onClick ||
                          spec.callbacks.onHoverChanged ||
                          spec.callbacks.onPressedChanged ||
                          hoverToken != baseToken ||
                          pressedToken != baseToken ||
                          std::abs(spec.hoverOpacity - spec.baseOpacity) > 0.001f ||
                          std::abs(spec.pressedOpacity - spec.baseOpacity) > 0.001f;
  if (needsInteraction) {
    PrimeFrame::Node* node = frame().getNode(button.nodeId());
    if (node && !node->primitives.empty()) {
      PrimeFrame::PrimitiveId background = node->primitives.front();
      PrimeFrame::Frame* framePtr = &frame();
      auto applyStyle = [framePtr,
                         background,
                         baseToken,
                         hoverToken,
                         pressedToken,
                         baseOverride,
                         hoverOverride,
                         pressedOverride](bool pressed, bool hovered) {
        PrimeFrame::Primitive* prim = framePtr->getPrimitive(background);
        if (!prim || prim->type != PrimeFrame::PrimitiveType::Rect) {
          return;
        }
        if (pressed) {
          prim->rect.token = pressedToken;
          prim->rect.overrideStyle = pressedOverride;
        } else if (hovered) {
          prim->rect.token = hoverToken;
          prim->rect.overrideStyle = hoverOverride;
        } else {
          prim->rect.token = baseToken;
          prim->rect.overrideStyle = baseOverride;
        }
      };
      struct ButtonState {
        bool hovered = false;
        bool pressed = false;
      };
      auto state = std::make_shared<ButtonState>();
      PrimeFrame::Callback callback;
      callback.onEvent = [callbacks = spec.callbacks,
                          applyStyle = std::move(applyStyle),
                          state](PrimeFrame::Event const& event) mutable -> bool {
        auto is_inside = [&event]() {
          return event.localX >= 0.0f && event.localX <= event.targetW &&
                 event.localY >= 0.0f && event.localY <= event.targetH;
        };
        auto update = [&](bool nextPressed, bool nextHovered) {
          bool hoverChanged = (nextHovered != state->hovered);
          bool pressChanged = (nextPressed != state->pressed);
          state->hovered = nextHovered;
          state->pressed = nextPressed;
          applyStyle(state->pressed, state->hovered);
          if (hoverChanged && callbacks.onHoverChanged) {
            callbacks.onHoverChanged(state->hovered);
          }
          if (pressChanged && callbacks.onPressedChanged) {
            callbacks.onPressedChanged(state->pressed);
          }
        };
        switch (event.type) {
          case PrimeFrame::EventType::PointerEnter:
            update(state->pressed, true);
            return true;
          case PrimeFrame::EventType::PointerLeave:
            update(false, false);
            return true;
          case PrimeFrame::EventType::PointerDown:
            update(true, true);
            return true;
          case PrimeFrame::EventType::PointerDrag: {
            bool inside = is_inside();
            update(inside, inside);
            return true;
          }
          case PrimeFrame::EventType::PointerUp: {
            bool inside = is_inside();
            bool fire = state->pressed && inside;
            update(false, inside);
            if (fire && callbacks.onClick) {
              callbacks.onClick();
            }
            return true;
          }
          case PrimeFrame::EventType::PointerCancel:
            update(false, false);
            return true;
          case PrimeFrame::EventType::PointerMove: {
            bool inside = is_inside();
            update(state->pressed && inside, inside);
            return true;
          }
          default:
            break;
        }
        return false;
      };
      PrimeFrame::CallbackId callbackId = frame().addCallback(std::move(callback));
      node->callbacks = callbackId;
      applyStyle(false, false);
    }
  }

  return UiNode(frame(), button.nodeId(), allowAbsolute_);
}

UiNode UiNode::createTextField(TextFieldSpec const& spec) {
  Rect bounds = resolve_rect(spec.size);
  std::string_view previewText = spec.text;
  PrimeFrame::TextStyleToken previewStyle = spec.textStyle;
  if (previewText.empty() && spec.showPlaceholderWhenEmpty) {
    previewText = spec.placeholder;
    previewStyle = spec.placeholderStyle;
  }
  float lineHeight = resolve_line_height(frame(), spec.textStyle);
  if (lineHeight <= 0.0f && previewStyle != spec.textStyle) {
    lineHeight = resolve_line_height(frame(), previewStyle);
  }
  if (bounds.height <= 0.0f &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchY <= 0.0f) {
    if (lineHeight > 0.0f) {
      bounds.height = lineHeight;
    }
  }
  if (bounds.width <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f &&
      !previewText.empty()) {
    float previewWidth = estimate_text_width(frame(), previewStyle, previewText);
    bounds.width = std::max(bounds.width, previewWidth + spec.paddingX * 2.0f);
  }
  if (bounds.width <= 0.0f &&
      bounds.height <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchX <= 0.0f &&
      spec.size.stretchY <= 0.0f) {
    return UiNode(frame(), id_, allowAbsolute_);
  }
  PanelSpec panel;
  panel.size = spec.size;
  if (!panel.size.preferredWidth.has_value() && bounds.width > 0.0f) {
    panel.size.preferredWidth = bounds.width;
  }
  if (!panel.size.preferredHeight.has_value() && bounds.height > 0.0f) {
    panel.size.preferredHeight = bounds.height;
  }
  panel.rectStyle = spec.backgroundStyle;
  panel.rectStyleOverride = spec.backgroundStyleOverride;
  panel.visible = spec.visible;
  UiNode field = createPanel(panel);

  std::string_view content = spec.text;
  PrimeFrame::TextStyleToken style = spec.textStyle;
  PrimeFrame::TextStyleOverride overrideStyle = spec.textStyleOverride;
  if (content.empty() && spec.showPlaceholderWhenEmpty) {
    content = spec.placeholder;
    style = spec.placeholderStyle;
    overrideStyle = spec.placeholderStyleOverride;
  }
  if (!spec.visible) {
    return UiNode(frame(), field.nodeId(), allowAbsolute_);
  }

  lineHeight = resolve_line_height(frame(), style);
  if (lineHeight <= 0.0f && style != spec.textStyle) {
    lineHeight = resolve_line_height(frame(), spec.textStyle);
  }
  float textY = (bounds.height - lineHeight) * 0.5f + spec.textOffsetY;
  float textWidth = std::max(0.0f, bounds.width - spec.paddingX * 2.0f);

  if (!content.empty()) {
    uint32_t selStart = std::min(spec.selectionStart, spec.selectionEnd);
    uint32_t selEnd = std::max(spec.selectionStart, spec.selectionEnd);
    uint32_t textSize = static_cast<uint32_t>(spec.text.size());
    selStart = std::min(selStart, textSize);
    selEnd = std::min(selEnd, textSize);
    if (selStart < selEnd && spec.selectionStyle != 0 && !spec.text.empty()) {
      float startAdvance = 0.0f;
      if (selStart > 0u) {
        startAdvance = measureTextWidth(frame(), spec.textStyle, spec.text.substr(0, selStart));
      }
      float endAdvance = startAdvance;
      if (selEnd > selStart) {
        endAdvance = measureTextWidth(frame(), spec.textStyle, spec.text.substr(0, selEnd));
      }
      float startX = spec.paddingX + startAdvance;
      float endX = spec.paddingX + endAdvance;
      float maxX = bounds.width - spec.paddingX;
      if (maxX < spec.paddingX) {
        maxX = spec.paddingX;
      }
      startX = std::clamp(startX, spec.paddingX, maxX);
      endX = std::clamp(endX, spec.paddingX, maxX);
      if (endX > startX) {
        Rect selectionRect{startX, textY, endX - startX, lineHeight};
        create_rect_node(frame(),
                         field.nodeId(),
                         selectionRect,
                         spec.selectionStyle,
                         spec.selectionStyleOverride,
                         false,
                         spec.visible);
      }
    }
    Rect textRect{spec.paddingX, textY, textWidth, lineHeight};
    create_text_node(frame(),
                     field.nodeId(),
                     textRect,
                     content,
                     style,
                     overrideStyle,
                     PrimeFrame::TextAlign::Start,
                     PrimeFrame::WrapMode::None,
                     textWidth,
                     spec.visible);
  }

  if (spec.showCursor && spec.cursorStyle != 0) {
    uint32_t cursorIndex = std::min(spec.cursorIndex, static_cast<uint32_t>(spec.text.size()));
    float cursorAdvance = 0.0f;
    if (cursorIndex > 0u && !spec.text.empty()) {
      cursorAdvance = measureTextWidth(frame(), spec.textStyle, spec.text.substr(0, cursorIndex));
    }
    float cursorX = spec.paddingX + cursorAdvance;
    float maxX = bounds.width - spec.paddingX - spec.cursorWidth;
    if (maxX < spec.paddingX) {
      maxX = spec.paddingX;
    }
    if (cursorX > maxX) {
      cursorX = maxX;
    }
    float cursorHeight = lineHeight > 0.0f ? lineHeight : bounds.height;
    if (cursorHeight < 0.0f) {
      cursorHeight = 0.0f;
    }
    Rect cursorRect{cursorX, textY, spec.cursorWidth, cursorHeight};
    create_rect_node(frame(),
                     field.nodeId(),
                     cursorRect,
                     spec.cursorStyle,
                     spec.cursorStyleOverride,
                     false,
                     spec.visible);
  }

  return UiNode(frame(), field.nodeId(), allowAbsolute_);
}

UiNode UiNode::createToggle(ToggleSpec const& spec) {
  Rect bounds = resolve_rect(spec.size);
  if (bounds.width <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f) {
    bounds.width = 40.0f;
  }
  if (bounds.height <= 0.0f &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchY <= 0.0f) {
    bounds.height = 20.0f;
  }
  PanelSpec panel;
  panel.size = spec.size;
  if (!panel.size.preferredWidth.has_value() && bounds.width > 0.0f) {
    panel.size.preferredWidth = bounds.width;
  }
  if (!panel.size.preferredHeight.has_value() && bounds.height > 0.0f) {
    panel.size.preferredHeight = bounds.height;
  }
  panel.rectStyle = spec.trackStyle;
  panel.rectStyleOverride = spec.trackStyleOverride;
  panel.visible = spec.visible;
  UiNode toggle = createPanel(panel);
  if (!spec.visible) {
    return UiNode(frame(), toggle.nodeId(), allowAbsolute_);
  }

  float inset = std::max(0.0f, spec.knobInset);
  float knobSize = std::max(0.0f, bounds.height - inset * 2.0f);
  float maxX = std::max(0.0f, bounds.width - knobSize);
  float knobX = spec.on ? maxX - inset : inset;
  knobX = std::clamp(knobX, 0.0f, maxX);
  Rect knobRect{knobX, inset, knobSize, knobSize};
  create_rect_node(frame(),
                   toggle.nodeId(),
                   knobRect,
                   spec.knobStyle,
                   spec.knobStyleOverride,
                   false,
                   spec.visible);
  return UiNode(frame(), toggle.nodeId(), allowAbsolute_);
}

UiNode UiNode::createCheckbox(CheckboxSpec const& spec) {
  Rect bounds = resolve_rect(spec.size);
  float lineHeight = resolve_line_height(frame(), spec.textStyle);
  float contentHeight = std::max(spec.boxSize, lineHeight);
  if (bounds.height <= 0.0f &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchY <= 0.0f) {
    bounds.height = contentHeight;
  }
  if (bounds.width <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f) {
    float labelWidth = spec.label.empty()
                           ? 0.0f
                           : estimate_text_width(frame(), spec.textStyle, spec.label);
    float gap = spec.label.empty() ? 0.0f : spec.gap;
    bounds.width = spec.boxSize + gap + labelWidth;
  }
  StackSpec rowSpec;
  rowSpec.size = spec.size;
  if (!rowSpec.size.preferredWidth.has_value() && bounds.width > 0.0f) {
    rowSpec.size.preferredWidth = bounds.width;
  }
  if (!rowSpec.size.preferredHeight.has_value() && bounds.height > 0.0f) {
    rowSpec.size.preferredHeight = bounds.height;
  }
  rowSpec.gap = spec.gap;
  rowSpec.clipChildren = false;
  rowSpec.visible = spec.visible;
  UiNode row = createHorizontalStack(rowSpec);

  PanelSpec box;
  box.size.preferredWidth = spec.boxSize;
  box.size.preferredHeight = spec.boxSize;
  box.rectStyle = spec.boxStyle;
  box.rectStyleOverride = spec.boxStyleOverride;
  box.visible = spec.visible;
  UiNode boxNode = row.createPanel(box);
  if (spec.checked && spec.visible) {
    float inset = std::max(0.0f, spec.checkInset);
    float checkSize = std::max(0.0f, spec.boxSize - inset * 2.0f);
    Rect checkRect{inset, inset, checkSize, checkSize};
    create_rect_node(frame(),
                     boxNode.nodeId(),
                     checkRect,
                     spec.checkStyle,
                     spec.checkStyleOverride,
                     false,
                     spec.visible);
  }

  if (!spec.label.empty()) {
    TextLineSpec text;
    text.text = spec.label;
    text.textStyle = spec.textStyle;
    text.textStyleOverride = spec.textStyleOverride;
    text.size.stretchX = 1.0f;
    text.size.preferredHeight = bounds.height;
    text.visible = spec.visible;
    row.createTextLine(text);
  }

  return UiNode(frame(), row.nodeId(), allowAbsolute_);
}

UiNode UiNode::createSlider(SliderSpec const& spec) {
  Rect bounds = resolve_rect(spec.size);
  if (bounds.width <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f) {
    bounds.width = spec.vertical ? 20.0f : 160.0f;
  }
  if (bounds.height <= 0.0f &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchY <= 0.0f) {
    bounds.height = spec.vertical ? 160.0f : 20.0f;
  }
  PanelSpec panel;
  panel.size = spec.size;
  if (!panel.size.preferredWidth.has_value() && bounds.width > 0.0f) {
    panel.size.preferredWidth = bounds.width;
  }
  if (!panel.size.preferredHeight.has_value() && bounds.height > 0.0f) {
    panel.size.preferredHeight = bounds.height;
  }
  panel.rectStyle = spec.trackStyle;
  panel.rectStyleOverride = spec.trackStyleOverride;
  panel.visible = spec.visible;
  UiNode slider = createPanel(panel);
  if (!spec.visible) {
    return UiNode(frame(), slider.nodeId(), allowAbsolute_);
  }

  float t = std::clamp(spec.value, 0.0f, 1.0f);
  auto apply_geometry = [vertical = spec.vertical,
                         trackThickness = spec.trackThickness,
                         thumbSize = spec.thumbSize](PrimeFrame::Frame& frame,
                                                     PrimeFrame::PrimitiveId fillPrim,
                                                     PrimeFrame::PrimitiveId thumbPrim,
                                                     float value,
                                                     float width,
                                                     float height,
                                                     PrimeFrame::RectStyleOverride const& fillOverride,
                                                     PrimeFrame::RectStyleOverride const& thumbOverride) {
    float clamped = std::clamp(value, 0.0f, 1.0f);
    float track = std::max(0.0f, trackThickness);
    float thumb = std::max(0.0f, thumbSize);
    auto apply_rect = [&](PrimeFrame::PrimitiveId primId,
                          Rect const& rect,
                          PrimeFrame::RectStyleOverride const& baseOverride) {
      if (PrimeFrame::Primitive* prim = frame.getPrimitive(primId)) {
        prim->offsetX = rect.x;
        prim->offsetY = rect.y;
        prim->width = rect.width;
        prim->height = rect.height;
        prim->rect.overrideStyle = baseOverride;
        if (rect.width <= 0.0f || rect.height <= 0.0f) {
          prim->rect.overrideStyle.opacity = 0.0f;
        }
      }
    };
    if (vertical) {
      float trackW = std::min(width, track);
      float trackX = (width - trackW) * 0.5f;
      float fillH = height * clamped;
      Rect fillRect{trackX, height - fillH, trackW, fillH};
      apply_rect(fillPrim, fillRect, fillOverride);
      float clampedThumb = std::min(thumb, std::min(width, height));
      Rect thumbRect{0.0f, 0.0f, 0.0f, 0.0f};
      if (clampedThumb > 0.0f) {
        float thumbY = (1.0f - clamped) * (height - clampedThumb);
        thumbRect = Rect{(width - clampedThumb) * 0.5f, thumbY, clampedThumb, clampedThumb};
      }
      apply_rect(thumbPrim, thumbRect, thumbOverride);
    } else {
      float trackH = std::min(height, track);
      float trackY = (height - trackH) * 0.5f;
      float fillW = width * clamped;
      Rect fillRect{0.0f, trackY, fillW, trackH};
      apply_rect(fillPrim, fillRect, fillOverride);
      float clampedThumb = std::min(thumb, std::min(width, height));
      Rect thumbRect{0.0f, 0.0f, 0.0f, 0.0f};
      if (clampedThumb > 0.0f) {
        float thumbX = clamped * (width - clampedThumb);
        thumbRect = Rect{thumbX, (height - clampedThumb) * 0.5f, clampedThumb, clampedThumb};
      }
      apply_rect(thumbPrim, thumbRect, thumbOverride);
    }
  };

  PrimeFrame::PrimitiveId fillPrim =
      add_rect_primitive_with_rect(frame(),
                                   slider.nodeId(),
                                   Rect{0.0f, 0.0f, 0.0f, 0.0f},
                                   spec.fillStyle,
                                   spec.fillStyleOverride);
  PrimeFrame::PrimitiveId thumbPrim =
      add_rect_primitive_with_rect(frame(),
                                   slider.nodeId(),
                                   Rect{0.0f, 0.0f, 0.0f, 0.0f},
                                   spec.thumbStyle,
                                   spec.thumbStyleOverride);
  PrimeFrame::PrimitiveId trackPrim = 0;
  bool trackPrimValid = false;
  if (PrimeFrame::Node* node = frame().getNode(slider.nodeId())) {
    if (!node->primitives.empty()) {
      trackPrim = node->primitives.front();
      trackPrimValid = true;
    }
  }
  apply_geometry(frame(),
                 fillPrim,
                 thumbPrim,
                 t,
                 bounds.width,
                 bounds.height,
                 spec.fillStyleOverride,
                 spec.thumbStyleOverride);
  if (trackPrimValid) {
    if (PrimeFrame::Primitive* prim = frame().getPrimitive(trackPrim)) {
      prim->rect.overrideStyle = spec.trackStyleOverride;
    }
  }

  if (spec.callbacks.onValueChanged || spec.callbacks.onDragStart || spec.callbacks.onDragEnd) {
    struct SliderState {
      bool active = false;
      bool hovered = false;
      bool trackPrimValid = false;
      PrimeFrame::PrimitiveId trackPrim = 0;
      PrimeFrame::PrimitiveId fillPrim = 0;
      PrimeFrame::PrimitiveId thumbPrim = 0;
      float targetW = 0.0f;
      float targetH = 0.0f;
      float value = 0.0f;
    };
    auto state = std::make_shared<SliderState>();
    state->trackPrimValid = trackPrimValid;
    state->trackPrim = trackPrim;
    state->fillPrim = fillPrim;
    state->thumbPrim = thumbPrim;
    state->targetW = bounds.width;
    state->targetH = bounds.height;
    state->value = t;
    auto update_from_event = [state,
                              vertical = spec.vertical,
                              thumbSize = spec.thumbSize](PrimeFrame::Event const& event) {
      if (event.targetW > 0.0f) {
        state->targetW = event.targetW;
      }
      if (event.targetH > 0.0f) {
        state->targetH = event.targetH;
      }
      float next = slider_value_from_event(event, vertical, thumbSize);
      state->value = std::clamp(next, 0.0f, 1.0f);
    };
    auto build_thumb_override = [state,
                                 base = spec.thumbStyleOverride,
                                 hoverOpacity = spec.thumbHoverOpacity,
                                 pressedOpacity = spec.thumbPressedOpacity]() {
      PrimeFrame::RectStyleOverride overrideStyle = base;
      if (state->active && pressedOpacity.has_value()) {
        overrideStyle.opacity = pressedOpacity;
      } else if (state->hovered && hoverOpacity.has_value()) {
        overrideStyle.opacity = hoverOpacity;
      }
      return overrideStyle;
    };
    auto build_fill_override = [state,
                                base = spec.fillStyleOverride,
                                hoverOpacity = spec.fillHoverOpacity,
                                pressedOpacity = spec.fillPressedOpacity]() {
      PrimeFrame::RectStyleOverride overrideStyle = base;
      if (state->active && pressedOpacity.has_value()) {
        overrideStyle.opacity = pressedOpacity;
      } else if (state->hovered && hoverOpacity.has_value()) {
        overrideStyle.opacity = hoverOpacity;
      }
      return overrideStyle;
    };
    auto build_track_override = [state,
                                 base = spec.trackStyleOverride,
                                 hoverOpacity = spec.trackHoverOpacity,
                                 pressedOpacity = spec.trackPressedOpacity]() {
      PrimeFrame::RectStyleOverride overrideStyle = base;
      if (state->active && pressedOpacity.has_value()) {
        overrideStyle.opacity = pressedOpacity;
      } else if (state->hovered && hoverOpacity.has_value()) {
        overrideStyle.opacity = hoverOpacity;
      }
      return overrideStyle;
    };
    auto apply_track_override = [framePtr = &frame(), state, build_track_override]() {
      if (!state->trackPrimValid) {
        return;
      }
      if (PrimeFrame::Primitive* prim = framePtr->getPrimitive(state->trackPrim)) {
        prim->rect.overrideStyle = build_track_override();
      }
    };
    PrimeFrame::Callback callback;
    callback.onEvent = [callbacks = spec.callbacks,
                        vertical = spec.vertical,
                        thumbSize = spec.thumbSize,
                        framePtr = &frame(),
                        state,
                        apply_geometry,
                        update_from_event,
                        build_fill_override,
                        build_thumb_override,
                        apply_track_override](PrimeFrame::Event const& event) -> bool {
      switch (event.type) {
        case PrimeFrame::EventType::PointerEnter: {
          state->hovered = true;
          apply_track_override();
          apply_geometry(*framePtr,
                         state->fillPrim,
                         state->thumbPrim,
                         state->value,
                         state->targetW,
                         state->targetH,
                         build_fill_override(),
                         build_thumb_override());
          return true;
        }
        case PrimeFrame::EventType::PointerLeave: {
          state->hovered = false;
          apply_track_override();
          apply_geometry(*framePtr,
                         state->fillPrim,
                         state->thumbPrim,
                         state->value,
                         state->targetW,
                         state->targetH,
                         build_fill_override(),
                         build_thumb_override());
          return true;
        }
        case PrimeFrame::EventType::PointerDown: {
          state->active = true;
          apply_track_override();
          update_from_event(event);
          apply_geometry(*framePtr,
                         state->fillPrim,
                         state->thumbPrim,
                         state->value,
                         state->targetW,
                         state->targetH,
                         build_fill_override(),
                         build_thumb_override());
          if (callbacks.onDragStart) {
            callbacks.onDragStart();
          }
          if (callbacks.onValueChanged) {
            callbacks.onValueChanged(state->value);
          }
          return true;
        }
        case PrimeFrame::EventType::PointerDrag:
        case PrimeFrame::EventType::PointerMove: {
          if (!state->active) {
            return false;
          }
          update_from_event(event);
          apply_geometry(*framePtr,
                         state->fillPrim,
                         state->thumbPrim,
                         state->value,
                         state->targetW,
                         state->targetH,
                         build_fill_override(),
                         build_thumb_override());
          if (callbacks.onValueChanged) {
            callbacks.onValueChanged(state->value);
          }
          return true;
        }
        case PrimeFrame::EventType::PointerUp:
        case PrimeFrame::EventType::PointerCancel: {
          if (!state->active) {
            return false;
          }
          update_from_event(event);
          apply_geometry(*framePtr,
                         state->fillPrim,
                         state->thumbPrim,
                         state->value,
                         state->targetW,
                         state->targetH,
                         build_fill_override(),
                         build_thumb_override());
          if (callbacks.onValueChanged) {
            callbacks.onValueChanged(state->value);
          }
          if (callbacks.onDragEnd) {
            callbacks.onDragEnd();
          }
          state->active = false;
          apply_track_override();
          return true;
        }
        default:
          break;
      }
      return false;
    };
    PrimeFrame::CallbackId callbackId = frame().addCallback(std::move(callback));
    if (PrimeFrame::Node* node = frame().getNode(slider.nodeId())) {
      node->callbacks = callbackId;
    }
  }

  return UiNode(frame(), slider.nodeId(), allowAbsolute_);
}

UiNode UiNode::createTabs(TabsSpec const& spec) {
  Rect bounds = resolve_rect(spec.size);
  float lineHeight = resolve_line_height(frame(), spec.textStyle);
  float activeLineHeight = resolve_line_height(frame(), spec.activeTextStyle);
  float tabLine = std::max(lineHeight, activeLineHeight);
  if (bounds.height <= 0.0f &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchY <= 0.0f) {
    bounds.height = tabLine + spec.tabPaddingY * 2.0f;
  }
  if (bounds.width <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f &&
      !spec.labels.empty()) {
    float total = 0.0f;
    for (size_t i = 0; i < spec.labels.size(); ++i) {
      PrimeFrame::TextStyleToken token =
          (static_cast<int>(i) == spec.selectedIndex) ? spec.activeTextStyle : spec.textStyle;
      float textWidth = estimate_text_width(frame(), token, spec.labels[i]);
      total += textWidth + spec.tabPaddingX * 2.0f;
      if (i + 1 < spec.labels.size()) {
        total += spec.gap;
      }
    }
    bounds.width = total;
  }

  StackSpec rowSpec;
  rowSpec.size = spec.size;
  if (!rowSpec.size.preferredWidth.has_value() && bounds.width > 0.0f) {
    rowSpec.size.preferredWidth = bounds.width;
  }
  if (!rowSpec.size.preferredHeight.has_value() && bounds.height > 0.0f) {
    rowSpec.size.preferredHeight = bounds.height;
  }
  rowSpec.gap = spec.gap;
  rowSpec.clipChildren = false;
  rowSpec.visible = spec.visible;
  UiNode row = createHorizontalStack(rowSpec);

  for (size_t i = 0; i < spec.labels.size(); ++i) {
    bool active = static_cast<int>(i) == spec.selectedIndex;
    PrimeFrame::RectStyleToken rectStyle = active ? spec.activeTabStyle : spec.tabStyle;
    PrimeFrame::RectStyleOverride rectOverride =
        active ? spec.activeTabStyleOverride : spec.tabStyleOverride;
    PrimeFrame::TextStyleToken textToken = active ? spec.activeTextStyle : spec.textStyle;
    PrimeFrame::TextStyleOverride textOverride =
        active ? spec.activeTextStyleOverride : spec.textStyleOverride;

    float textWidth = estimate_text_width(frame(), textToken, spec.labels[i]);
    PanelSpec tabPanel;
    tabPanel.rectStyle = rectStyle;
    tabPanel.rectStyleOverride = rectOverride;
    tabPanel.size.preferredWidth = textWidth + spec.tabPaddingX * 2.0f;
    tabPanel.size.preferredHeight = bounds.height;
    tabPanel.visible = spec.visible;
    UiNode tab = row.createPanel(tabPanel);

    TextLineSpec textSpec;
    textSpec.text = spec.labels[i];
    textSpec.textStyle = textToken;
    textSpec.textStyleOverride = textOverride;
    textSpec.align = PrimeFrame::TextAlign::Center;
    textSpec.size.stretchX = 1.0f;
    textSpec.size.preferredHeight = bounds.height;
    textSpec.visible = spec.visible;
    tab.createTextLine(textSpec);
  }

  return UiNode(frame(), row.nodeId(), allowAbsolute_);
}

UiNode UiNode::createDropdown(DropdownSpec const& spec) {
  Rect bounds = resolve_rect(spec.size);
  float lineHeight = resolve_line_height(frame(), spec.textStyle);
  if (bounds.height <= 0.0f &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchY <= 0.0f) {
    bounds.height = lineHeight + spec.paddingX;
  }
  if (bounds.width <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f) {
    float labelWidth = spec.label.empty()
                           ? 0.0f
                           : estimate_text_width(frame(), spec.textStyle, spec.label);
    float indicatorWidth = estimate_text_width(frame(), spec.indicatorStyle, spec.indicator);
    float gap = spec.label.empty() ? 0.0f : spec.indicatorGap;
    bounds.width = spec.paddingX * 2.0f + labelWidth + gap + indicatorWidth;
  }

  PanelSpec panel;
  panel.size = spec.size;
  if (!panel.size.preferredWidth.has_value() && bounds.width > 0.0f) {
    panel.size.preferredWidth = bounds.width;
  }
  if (!panel.size.preferredHeight.has_value() && bounds.height > 0.0f) {
    panel.size.preferredHeight = bounds.height;
  }
  panel.rectStyle = spec.backgroundStyle;
  panel.rectStyleOverride = spec.backgroundStyleOverride;
  panel.layout = PrimeFrame::LayoutType::HorizontalStack;
  panel.padding.left = spec.paddingX;
  panel.padding.right = spec.paddingX;
  panel.gap = spec.indicatorGap;
  panel.visible = spec.visible;
  UiNode dropdown = createPanel(panel);

  if (!spec.label.empty()) {
    TextLineSpec labelText;
    labelText.text = spec.label;
    labelText.textStyle = spec.textStyle;
    labelText.textStyleOverride = spec.textStyleOverride;
    labelText.align = PrimeFrame::TextAlign::Start;
    labelText.size.stretchX = 1.0f;
    labelText.size.preferredHeight = bounds.height;
    labelText.visible = spec.visible;
    dropdown.createTextLine(labelText);
  } else {
    SizeSpec spacer;
    spacer.stretchX = 1.0f;
    spacer.preferredHeight = bounds.height;
    dropdown.createSpacer(spacer);
  }

  TextLineSpec indicatorText;
  indicatorText.text = spec.indicator;
  indicatorText.textStyle = spec.indicatorStyle;
  indicatorText.textStyleOverride = spec.indicatorStyleOverride;
  indicatorText.align = PrimeFrame::TextAlign::Center;
  indicatorText.size.preferredHeight = bounds.height;
  indicatorText.visible = spec.visible;
  dropdown.createTextLine(indicatorText);

  return UiNode(frame(), dropdown.nodeId(), allowAbsolute_);
}

UiNode UiNode::createProgressBar(ProgressBarSpec const& spec) {
  Rect bounds = resolve_rect(spec.size);
  if (bounds.width <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f) {
    bounds.width = 140.0f;
  }
  if (bounds.height <= 0.0f &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchY <= 0.0f) {
    bounds.height = 12.0f;
  }
  PanelSpec panel;
  panel.size = spec.size;
  if (!panel.size.preferredWidth.has_value() && bounds.width > 0.0f) {
    panel.size.preferredWidth = bounds.width;
  }
  if (!panel.size.preferredHeight.has_value() && bounds.height > 0.0f) {
    panel.size.preferredHeight = bounds.height;
  }
  panel.rectStyle = spec.trackStyle;
  panel.rectStyleOverride = spec.trackStyleOverride;
  panel.visible = spec.visible;
  UiNode bar = createPanel(panel);
  if (!spec.visible) {
    return UiNode(frame(), bar.nodeId(), allowAbsolute_);
  }

  float t = std::clamp(spec.value, 0.0f, 1.0f);
  float fillW = bounds.width * t;
  if (spec.minFillWidth > 0.0f) {
    fillW = std::max(fillW, spec.minFillWidth);
  }
  fillW = std::min(fillW, bounds.width);
  if (fillW > 0.0f) {
    Rect fillRect{0.0f, 0.0f, fillW, bounds.height};
    create_rect_node(frame(),
                     bar.nodeId(),
                     fillRect,
                     spec.fillStyle,
                     spec.fillStyleOverride,
                     false,
                     spec.visible);
  }

  return UiNode(frame(), bar.nodeId(), allowAbsolute_);
}

UiNode UiNode::createTable(TableSpec const& spec) {
  PrimeFrame::NodeId id_ = nodeId();
  bool allowAbsolute_ = allowAbsolute();
  Rect tableBounds = resolve_rect(spec.size);
  size_t rowCount = spec.rows.size();
  float rowsHeight = 0.0f;
  if (rowCount > 0) {
    rowsHeight = static_cast<float>(rowCount) * spec.rowHeight +
                 static_cast<float>(rowCount - 1) * spec.rowGap;
  }
  float headerBlock = spec.headerHeight > 0.0f ? spec.headerInset + spec.headerHeight : 0.0f;
  if (tableBounds.height <= 0.0f && !spec.size.preferredHeight.has_value() && spec.size.stretchY <= 0.0f) {
    tableBounds.height = headerBlock + rowsHeight;
  }
  if (tableBounds.width <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f &&
      !spec.columns.empty()) {
    float inferredWidth = 0.0f;
    float paddingX = std::max(spec.headerPaddingX, spec.cellPaddingX);
    for (size_t colIndex = 0; colIndex < spec.columns.size(); ++colIndex) {
      TableColumn const& col = spec.columns[colIndex];
      if (col.width > 0.0f) {
        inferredWidth += col.width;
        continue;
      }
      float maxText = estimate_text_width(frame(), col.headerStyle, col.label);
      for (auto const& row : spec.rows) {
        if (colIndex < row.size()) {
          std::string_view cell = row[colIndex];
          float cellWidth = estimate_text_width(frame(), col.cellStyle, cell);
          if (cellWidth > maxText) {
            maxText = cellWidth;
          }
        }
      }
      inferredWidth += maxText + paddingX;
    }
    tableBounds.width = inferredWidth;
  }

  SizeSpec tableSize = spec.size;
  if (!tableSize.preferredWidth.has_value() && tableBounds.width > 0.0f) {
    tableSize.preferredWidth = tableBounds.width;
  }
  if (!tableSize.preferredHeight.has_value() && tableBounds.height > 0.0f) {
    tableSize.preferredHeight = tableBounds.height;
  }

  StackSpec tableSpec;
  tableSpec.size = tableSize;
  tableSpec.gap = 0.0f;
  tableSpec.clipChildren = spec.clipChildren;
  tableSpec.visible = spec.visible;
  UiNode parentNode(frame(), id_, allowAbsolute_);
  UiNode tableNode = parentNode.createVerticalStack(tableSpec);

  float tableWidth = tableBounds.width > 0.0f ? tableBounds.width
                                              : tableSize.preferredWidth.value_or(0.0f);
  float dividerWidth = spec.showColumnDividers ? 1.0f : 0.0f;
  size_t dividerCount = spec.columns.size() > 1 ? spec.columns.size() - 1 : 0;
  float dividerTotal = dividerWidth * static_cast<float>(dividerCount);

  auto compute_auto_width = [&](size_t colIndex, TableColumn const& col) {
    float paddingX = std::max(spec.headerPaddingX, spec.cellPaddingX);
    float maxText = estimate_text_width(frame(), col.headerStyle, col.label);
    for (auto const& row : spec.rows) {
      if (colIndex < row.size()) {
        std::string_view cell = row[colIndex];
        float cellWidth = estimate_text_width(frame(), col.cellStyle, cell);
        if (cellWidth > maxText) {
          maxText = cellWidth;
        }
      }
    }
    return maxText + paddingX;
  };

  std::vector<float> columnWidths;
  columnWidths.reserve(spec.columns.size());
  float fixedWidth = 0.0f;
  size_t autoCount = 0;
  for (size_t colIndex = 0; colIndex < spec.columns.size(); ++colIndex) {
    TableColumn const& col = spec.columns[colIndex];
    if (col.width > 0.0f) {
      columnWidths.push_back(col.width);
      fixedWidth += col.width;
    } else {
      columnWidths.push_back(0.0f);
      ++autoCount;
    }
  }
  float availableWidth = std::max(0.0f, tableWidth - dividerTotal);
  if (autoCount > 0 && availableWidth > fixedWidth) {
    float remaining = availableWidth - fixedWidth;
    float autoWidth = remaining / static_cast<float>(autoCount);
    for (float& width : columnWidths) {
      if (width == 0.0f) {
        width = autoWidth;
      }
    }
  }
  if (autoCount > 0 && (availableWidth <= fixedWidth || tableWidth <= 0.0f)) {
    for (size_t colIndex = 0; colIndex < columnWidths.size(); ++colIndex) {
      if (columnWidths[colIndex] <= 0.0f) {
        columnWidths[colIndex] = compute_auto_width(colIndex, spec.columns[colIndex]);
      }
    }
  }
  if (autoCount == 0 && availableWidth > 0.0f && fixedWidth > availableWidth && !columnWidths.empty()) {
    float overflow = fixedWidth - availableWidth;
    columnWidths.back() = std::max(0.0f, columnWidths.back() - overflow);
  }

  auto create_cell = [&](UiNode const& rowNode,
                         float width,
                         float height,
                         float paddingX,
                         std::string_view text,
                         PrimeFrame::TextStyleToken role) {
    SizeSpec cellSize;
    if (width > 0.0f) {
      cellSize.preferredWidth = width;
    }
    if (height > 0.0f) {
      cellSize.preferredHeight = height;
    }
    PrimeFrame::Insets padding{};
    padding.left = paddingX;
    padding.right = paddingX;
    PrimeFrame::NodeId cellId = create_node(frame(), rowNode.nodeId(), Rect{},
                                            &cellSize,
                                            PrimeFrame::LayoutType::Overlay,
                                            padding,
                                            0.0f,
                                            false,
                                            spec.visible);
    UiNode cell(frame(), cellId, rowNode.allowAbsolute());
    SizeSpec textSize;
    textSize.stretchX = 1.0f;
    if (height > 0.0f) {
      textSize.preferredHeight = height;
    }
    TextLineSpec textSpec;
    textSpec.text = text;
    textSpec.textStyle = role;
    textSpec.size = textSize;
    textSpec.visible = spec.visible;
    cell.createTextLine(textSpec);
  };

  if (spec.showHeaderDividers) {
    DividerSpec divider;
    divider.rectStyle = spec.dividerStyle;
    divider.visible = spec.visible;
    divider.size.stretchX = 1.0f;
    divider.size.preferredHeight = 1.0f;
    tableNode.createDivider(divider);
  }

  if (spec.headerInset > 0.0f) {
    SizeSpec headerInset;
    headerInset.preferredHeight = spec.headerInset;
    tableNode.createSpacer(headerInset);
  }

  if (spec.headerHeight > 0.0f && !spec.columns.empty()) {
    PanelSpec headerPanel;
    headerPanel.rectStyle = spec.headerStyle;
    headerPanel.layout = PrimeFrame::LayoutType::HorizontalStack;
    headerPanel.size.preferredHeight = spec.headerHeight;
    headerPanel.size.stretchX = 1.0f;
    headerPanel.visible = spec.visible;
    UiNode headerRow = tableNode.createPanel(headerPanel);

    for (size_t colIndex = 0; colIndex < spec.columns.size(); ++colIndex) {
      TableColumn const& col = spec.columns[colIndex];
      float colWidth = colIndex < columnWidths.size() ? columnWidths[colIndex] : 0.0f;
      create_cell(headerRow,
                  colWidth,
                  spec.headerHeight,
                  spec.headerPaddingX,
                  col.label,
                  col.headerStyle);
      if (spec.showColumnDividers && colIndex + 1 < spec.columns.size()) {
        DividerSpec divider;
        divider.rectStyle = spec.dividerStyle;
        divider.visible = spec.visible;
        divider.size.preferredWidth = dividerWidth;
        divider.size.preferredHeight = spec.headerHeight;
        headerRow.createDivider(divider);
      }
    }
  }

  if (spec.showHeaderDividers) {
    DividerSpec divider;
    divider.rectStyle = spec.dividerStyle;
    divider.visible = spec.visible;
    divider.size.stretchX = 1.0f;
    divider.size.preferredHeight = 1.0f;
    tableNode.createDivider(divider);
  }

  StackSpec rowsSpec;
  rowsSpec.size.stretchX = 1.0f;
  rowsSpec.size.stretchY = spec.size.stretchY;
  rowsSpec.gap = spec.rowGap;
  rowsSpec.clipChildren = spec.clipChildren;
  rowsSpec.visible = spec.visible;
  UiNode rowsNode = tableNode.createVerticalStack(rowsSpec);

  for (size_t rowIndex = 0; rowIndex < rowCount; ++rowIndex) {
    PrimeFrame::RectStyleToken rowRole = (rowIndex % 2 == 0) ? spec.rowAltStyle : spec.rowStyle;
    PanelSpec rowPanel;
    rowPanel.rectStyle = rowRole;
    rowPanel.layout = PrimeFrame::LayoutType::HorizontalStack;
    rowPanel.size.preferredHeight = spec.rowHeight;
    rowPanel.size.stretchX = 1.0f;
    rowPanel.visible = spec.visible;
    UiNode rowNode = rowsNode.createPanel(rowPanel);

    for (size_t colIndex = 0; colIndex < spec.columns.size(); ++colIndex) {
      TableColumn const& col = spec.columns[colIndex];
      float colWidth = colIndex < columnWidths.size() ? columnWidths[colIndex] : 0.0f;
      std::string_view cellText;
      if (rowIndex < spec.rows.size() && colIndex < spec.rows[rowIndex].size()) {
        cellText = spec.rows[rowIndex][colIndex];
      }
      create_cell(rowNode,
                  colWidth,
                  spec.rowHeight,
                  spec.cellPaddingX,
                  cellText,
                  col.cellStyle);
      if (spec.showColumnDividers && colIndex + 1 < spec.columns.size()) {
        DividerSpec divider;
        divider.rectStyle = spec.dividerStyle;
        divider.visible = spec.visible;
        divider.size.preferredWidth = dividerWidth;
        divider.size.preferredHeight = spec.rowHeight;
        rowNode.createDivider(divider);
      }
    }
  }

  return UiNode(frame(), tableNode.nodeId(), allowAbsolute_);
}

namespace Studio {

UiNode createSectionHeader(UiNode& parent, SectionHeaderSpec const& spec) {
  auto frame = [&]() -> PrimeFrame::Frame& { return parent.frame(); };
  bool allowAbsolute_ = parent.allowAbsolute();
  Rect bounds = resolve_rect(spec.size);
  if (bounds.height <= 0.0f &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchY <= 0.0f) {
    bounds.height = StudioDefaults::SectionHeaderHeight;
  }
  if (bounds.width <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f &&
      !spec.title.empty()) {
    float textWidth = estimate_text_width(frame(), textToken(spec.textRole), spec.title);
    bounds.width = std::max(bounds.width, spec.textInsetX + textWidth);
  }
  SizeSpec headerSize = spec.size;
  if (!headerSize.preferredWidth.has_value() && bounds.width > 0.0f) {
    headerSize.preferredWidth = bounds.width;
  }
  if (!headerSize.preferredHeight.has_value() && bounds.height > 0.0f) {
    headerSize.preferredHeight = bounds.height;
  }

  PanelSpec panel;
  panel.size = headerSize;
  panel.rectStyle = rectToken(spec.backgroundRole);
  panel.layout = PrimeFrame::LayoutType::HorizontalStack;
  panel.gap = 0.0f;
  UiNode header = parent.createPanel(panel);

  if (spec.accentWidth > 0.0f) {
    PanelSpec accentPanel;
    accentPanel.rectStyle = rectToken(spec.accentRole);
    accentPanel.size.preferredWidth = spec.accentWidth;
    accentPanel.size.stretchY = 1.0f;
    header.createPanel(accentPanel);
  }

  if (spec.textInsetX > 0.0f) {
    SizeSpec spacer;
    spacer.preferredWidth = spec.textInsetX;
    spacer.preferredHeight = bounds.height;
    header.createSpacer(spacer);
  }

  if (!spec.title.empty()) {
    SizeSpec textSize;
    textSize.stretchX = 1.0f;
    textSize.preferredHeight = bounds.height;
    header.createTextLine(spec.title, textToken(spec.textRole), textSize, PrimeFrame::TextAlign::Start);
  }

  if (spec.textInsetX > 0.0f) {
    SizeSpec spacer;
    spacer.preferredWidth = spec.textInsetX;
    spacer.preferredHeight = bounds.height;
    header.createSpacer(spacer);
  }

  if (spec.addDivider) {
    if (spec.dividerOffsetY > 0.0f) {
      SizeSpec spacer;
      spacer.preferredHeight = spec.dividerOffsetY;
      parent.createSpacer(spacer);
    }
    DividerSpec divider;
    divider.rectStyle = rectToken(spec.dividerRole);
    divider.size.stretchX = 1.0f;
    divider.size.preferredHeight = 1.0f;
    parent.createDivider(divider);
  }

  return UiNode(frame(), header.nodeId(), allowAbsolute_);
}

UiNode createSectionHeader(UiNode& parent,
                           SizeSpec const& size,
                           std::string_view title,
                           TextRole role) {
  SectionHeaderSpec spec;
  spec.size = size;
  spec.title = title;
  spec.textRole = role;
  return createSectionHeader(parent, spec);
}

UiNode createSectionHeader(UiNode& parent,
                           SizeSpec const& size,
                           std::string_view title,
                           TextRole role,
                           bool addDivider,
                           float dividerOffsetY) {
  SectionHeaderSpec spec;
  spec.size = size;
  spec.title = title;
  spec.textRole = role;
  spec.addDivider = addDivider;
  spec.dividerOffsetY = dividerOffsetY;
  return createSectionHeader(parent, spec);
}

SectionPanel createSectionPanel(UiNode& parent, SectionPanelSpec const& spec) {
  auto frame = [&]() -> PrimeFrame::Frame& { return parent.frame(); };
  bool allowAbsolute_ = parent.allowAbsolute();
  Rect bounds = resolve_rect(spec.size);
  if (bounds.width <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f &&
      !spec.title.empty()) {
    float textWidth = estimate_text_width(frame(), textToken(spec.textRole), spec.title);
    bounds.width = spec.headerInsetX + spec.headerInsetRight + spec.headerPaddingX * 2.0f + textWidth;
  }
  if (bounds.height <= 0.0f &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchY <= 0.0f) {
    bounds.height = spec.headerInsetY + spec.headerHeight + spec.contentInsetY + spec.contentInsetBottom;
  }
  SizeSpec panelSize = spec.size;
  if (!panelSize.preferredWidth.has_value() && bounds.width > 0.0f) {
    panelSize.preferredWidth = bounds.width;
  }
  if (!panelSize.preferredHeight.has_value() && bounds.height > 0.0f) {
    panelSize.preferredHeight = bounds.height;
  }

  PanelSpec panel;
  panel.size = panelSize;
  panel.rectStyle = rectToken(spec.panelRole);
  panel.layout = PrimeFrame::LayoutType::VerticalStack;
  panel.gap = 0.0f;
  UiNode panelNode = parent.createPanel(panel);
  SectionPanel out{UiNode(frame(), panelNode.nodeId(), allowAbsolute_),
                   UiNode(frame(), PrimeFrame::NodeId{}, allowAbsolute_)};

  float headerWidth = std::max(0.0f, bounds.width - spec.headerInsetX - spec.headerInsetRight);

  if (spec.headerInsetY > 0.0f) {
    SizeSpec spacer;
    spacer.preferredHeight = spec.headerInsetY;
    panelNode.createSpacer(spacer);
  }

  if (spec.headerHeight > 0.0f) {
    StackSpec headerInset;
    headerInset.size.stretchX = 1.0f;
    headerInset.size.preferredHeight = spec.headerHeight;
    headerInset.clipChildren = false;
    headerInset.padding.left = spec.headerInsetX;
    headerInset.padding.right = spec.headerInsetRight;
    UiNode headerInsetNode = panelNode.createOverlay(headerInset);

    PanelSpec headerPanel;
    headerPanel.rectStyle = rectToken(spec.headerRole);
    headerPanel.layout = PrimeFrame::LayoutType::HorizontalStack;
    headerPanel.size.stretchX = 1.0f;
    headerPanel.size.stretchY = 1.0f;
    if (headerWidth > 0.0f) {
      headerPanel.size.preferredWidth = headerWidth;
    }
    headerPanel.gap = 0.0f;
    headerPanel.clipChildren = false;
    UiNode headerRow = headerInsetNode.createPanel(headerPanel);

    if (spec.showAccent && spec.accentWidth > 0.0f) {
      PanelSpec accentPanel;
      accentPanel.rectStyle = rectToken(spec.accentRole);
      accentPanel.size.preferredWidth = spec.accentWidth;
      accentPanel.size.stretchY = 1.0f;
      headerRow.createPanel(accentPanel);
    }

    if (spec.headerPaddingX > 0.0f) {
      SizeSpec spacer;
      spacer.preferredWidth = spec.headerPaddingX;
      spacer.preferredHeight = spec.headerHeight;
      headerRow.createSpacer(spacer);
    }

    if (!spec.title.empty()) {
      SizeSpec titleSize;
      titleSize.stretchX = 1.0f;
      titleSize.preferredHeight = spec.headerHeight;
      headerRow.createTextLine(spec.title, textToken(spec.textRole), titleSize, PrimeFrame::TextAlign::Start);
    }

    if (spec.headerPaddingX > 0.0f) {
      SizeSpec spacer;
      spacer.preferredWidth = spec.headerPaddingX;
      spacer.preferredHeight = spec.headerHeight;
      headerRow.createSpacer(spacer);
    }
  }

  if (spec.contentInsetY > 0.0f) {
    SizeSpec spacer;
    spacer.preferredHeight = spec.contentInsetY;
    panelNode.createSpacer(spacer);
  }

  float contentY = spec.headerInsetY + spec.headerHeight + spec.contentInsetY;
  float contentW = std::max(0.0f, bounds.width - spec.contentInsetX - spec.contentInsetRight);
  float contentH = std::max(0.0f, bounds.height - (contentY + spec.contentInsetBottom));

  StackSpec contentInset;
  contentInset.size.stretchX = 1.0f;
  contentInset.size.stretchY = panelSize.stretchY > 0.0f ? 1.0f : 0.0f;
  if (contentH > 0.0f) {
    contentInset.size.preferredHeight = contentH;
  }
  contentInset.clipChildren = false;
  contentInset.padding.left = spec.contentInsetX;
  contentInset.padding.right = spec.contentInsetRight;
  UiNode contentInsetNode = panelNode.createOverlay(contentInset);

  SizeSpec contentSize;
  contentSize.stretchX = 1.0f;
  contentSize.stretchY = contentInset.size.stretchY;
  if (contentW > 0.0f) {
    contentSize.preferredWidth = contentW;
  }
  if (contentH > 0.0f) {
    contentSize.preferredHeight = contentH;
  }
  PrimeFrame::NodeId contentId = create_node(frame(), contentInsetNode.nodeId(), Rect{},
                                             &contentSize,
                                             PrimeFrame::LayoutType::None,
                                             PrimeFrame::Insets{},
                                             0.0f,
                                             false,
                                             true);
  out.content = UiNode(frame(), contentId, allowAbsolute_);

  if (spec.contentInsetBottom > 0.0f) {
    SizeSpec spacer;
    spacer.preferredHeight = spec.contentInsetBottom;
    panelNode.createSpacer(spacer);
  }

  return out;
}

SectionPanel createSectionPanel(UiNode& parent, SizeSpec const& size, std::string_view title) {
  SectionPanelSpec spec;
  spec.size = size;
  spec.title = title;
  return createSectionPanel(parent, spec);
}

UiNode createPropertyList(UiNode const& parent, PropertyListSpec const& spec) {
  auto frame = [&]() -> PrimeFrame::Frame& { return parent.frame(); };
  bool allowAbsolute_ = parent.allowAbsolute();
  Rect bounds = resolve_rect(spec.size);
  if (bounds.width <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f) {
    float maxWidth = 0.0f;
    float labelInset = spec.labelInsetX;
    float valueInset = spec.valueInsetX;
    float valuePadding = spec.valuePaddingRight;
    for (PropertyRow const& row : spec.rows) {
      float labelWidth = estimate_text_width(frame(), textToken(spec.labelRole), row.label);
      float valueWidth = estimate_text_width(frame(), textToken(spec.valueRole), row.value);
      if (spec.valueAlignRight) {
        maxWidth = std::max(maxWidth, labelInset + labelWidth + valuePadding + valueWidth);
      } else {
        maxWidth = std::max(maxWidth, std::max(labelInset + labelWidth, valueInset + valueWidth));
      }
    }
    bounds.width = maxWidth;
  }
  if (bounds.height <= 0.0f &&
      !spec.rows.empty() &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchY <= 0.0f) {
    bounds.height = static_cast<float>(spec.rows.size()) * spec.rowHeight +
                    static_cast<float>(spec.rows.size() - 1) * spec.rowGap;
  }
  SizeSpec listSize = spec.size;
  if (!listSize.preferredWidth.has_value() && bounds.width > 0.0f) {
    listSize.preferredWidth = bounds.width;
  }
  if (!listSize.preferredHeight.has_value() && bounds.height > 0.0f) {
    listSize.preferredHeight = bounds.height;
  }

  StackSpec listSpec;
  listSpec.size = listSize;
  listSpec.gap = spec.rowGap;
  listSpec.clipChildren = false;
  UiNode parentNode(frame(), parent.nodeId(), allowAbsolute_);
  UiNode listNode = parentNode.createVerticalStack(listSpec);

  float maxLabelWidth = 0.0f;
  float maxValueWidth = 0.0f;
  for (PropertyRow const& row : spec.rows) {
    maxLabelWidth = std::max(maxLabelWidth,
                             estimate_text_width(frame(), textToken(spec.labelRole), row.label));
    maxValueWidth = std::max(maxValueWidth,
                             estimate_text_width(frame(), textToken(spec.valueRole), row.value));
  }

  float rowWidth = bounds.width > 0.0f ? bounds.width : listSize.preferredWidth.value_or(0.0f);
  float labelCellWidth = std::max(0.0f, spec.labelInsetX + maxLabelWidth);
  float valueCellWidth = std::max(0.0f, spec.valueInsetX + maxValueWidth + spec.valuePaddingRight);
  if (rowWidth > 0.0f && spec.valueAlignRight) {
    valueCellWidth = std::max(valueCellWidth, rowWidth - labelCellWidth);
  }

  auto create_cell = [&](UiNode const& rowNode,
                         float width,
                         float height,
                         float paddingLeft,
                         float paddingRight,
                         std::string_view text,
                         TextRole role,
                         PrimeFrame::TextAlign align) {
    SizeSpec cellSize;
    if (width > 0.0f) {
      cellSize.preferredWidth = width;
    }
    if (height > 0.0f) {
      cellSize.preferredHeight = height;
    }
    PrimeFrame::Insets padding{};
    padding.left = paddingLeft;
    padding.right = paddingRight;
    PrimeFrame::NodeId cellId = create_node(frame(), rowNode.nodeId(), Rect{},
                                            &cellSize,
                                            PrimeFrame::LayoutType::Overlay,
                                            padding,
                                            0.0f,
                                            false,
                                            true);
    if (PrimeFrame::Node* cellNode = frame().getNode(cellId)) {
      cellNode->hitTestVisible = false;
    }
    UiNode cell(frame(), cellId, rowNode.allowAbsolute());
    SizeSpec textSize;
    float innerWidth = width - paddingLeft - paddingRight;
    if (innerWidth > 0.0f) {
      textSize.preferredWidth = innerWidth;
    }
    if (height > 0.0f) {
      textSize.preferredHeight = height;
    }
    cell.createTextLine(text, textToken(role), textSize, align);
  };

  for (PropertyRow const& row : spec.rows) {
    StackSpec rowSpec;
    rowSpec.size.preferredHeight = spec.rowHeight;
    rowSpec.size.stretchX = 1.0f;
    rowSpec.gap = 0.0f;
    rowSpec.clipChildren = false;
    UiNode rowNode = listNode.createHorizontalStack(rowSpec);

    create_cell(rowNode,
                labelCellWidth,
                spec.rowHeight,
                spec.labelInsetX,
                0.0f,
                row.label,
                spec.labelRole,
                PrimeFrame::TextAlign::Start);

    float valueWidth = estimate_text_width(frame(), textToken(spec.valueRole), row.value);
    SizeSpec valueSize;
    valueSize.preferredWidth = valueCellWidth;
    valueSize.preferredHeight = spec.rowHeight;
    if (spec.valueAlignRight) {
      valueSize.stretchX = 1.0f;
    }

    PrimeFrame::Insets valuePadding{};
    valuePadding.left = spec.valueInsetX;
    valuePadding.right = spec.valuePaddingRight;
    PrimeFrame::NodeId valueId = create_node(frame(), rowNode.nodeId(), Rect{},
                                             &valueSize,
                                             PrimeFrame::LayoutType::HorizontalStack,
                                             valuePadding,
                                             0.0f,
                                             false,
                                             true);
    if (PrimeFrame::Node* valueNode = frame().getNode(valueId)) {
      valueNode->hitTestVisible = false;
    }
    UiNode valueCell(frame(), valueId, rowNode.allowAbsolute());

    if (spec.valueAlignRight) {
      SizeSpec spacer;
      spacer.stretchX = 1.0f;
      valueCell.createSpacer(spacer);
    }

    SizeSpec textSize;
    textSize.preferredWidth = valueWidth;
    textSize.preferredHeight = spec.rowHeight;
    valueCell.createTextLine(row.value, textToken(spec.valueRole), textSize, PrimeFrame::TextAlign::Start);
  }

  return UiNode(frame(), listNode.nodeId(), allowAbsolute_);
}

UiNode createPropertyList(UiNode const& parent, SizeSpec const& size, std::vector<PropertyRow> rows) {
  PropertyListSpec spec;
  spec.size = size;
  spec.rows = std::move(rows);
  return createPropertyList(parent, spec);
}

UiNode createPropertyRow(UiNode const& parent,
                         SizeSpec const& size,
                         std::string_view label,
                         std::string_view value,
                         TextRole role) {
  PropertyListSpec spec;
  spec.size = size;
  spec.labelRole = role;
  spec.valueRole = role;
  spec.rows = {{label, value}};
  return createPropertyList(parent, spec);
}

UiNode createProgressBar(UiNode& parent, ProgressBarSpec const& spec) {
  auto frame = [&]() -> PrimeFrame::Frame& { return parent.frame(); };
  Rect bounds = resolve_rect(spec.size);
  if ((bounds.width <= 0.0f || bounds.height <= 0.0f) &&
      !spec.size.preferredWidth.has_value() &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchX <= 0.0f &&
      spec.size.stretchY <= 0.0f) {
    if (bounds.width <= 0.0f) {
      bounds.width = StudioDefaults::ControlWidthL;
    }
    if (bounds.height <= 0.0f) {
      bounds.height = StudioDefaults::OpacityBarHeight;
    }
  }
  PanelSpec panel;
  panel.size = spec.size;
  if (!panel.size.preferredWidth.has_value() && bounds.width > 0.0f) {
    panel.size.preferredWidth = bounds.width;
  }
  if (!panel.size.preferredHeight.has_value() && bounds.height > 0.0f) {
    panel.size.preferredHeight = bounds.height;
  }
  panel.rectStyle = rectToken(spec.trackRole);
  UiNode bar = parent.createPanel(panel);
  float clamped = std::clamp(spec.value, 0.0f, 1.0f);
  float fillWidth = bounds.width * clamped;
  if (spec.minFillWidth > 0.0f && clamped > 0.0f) {
    fillWidth = std::max(fillWidth, spec.minFillWidth);
  }
  if (fillWidth > 0.0f) {
    create_rect_node(frame(),
                     bar.nodeId(),
                     Rect{0.0f, 0.0f, fillWidth, bounds.height},
                     rectToken(spec.fillRole),
                     {},
                     false,
                     true);
  }
  return bar;
}

UiNode createProgressBar(UiNode& parent, SizeSpec const& size, float value) {
  ProgressBarSpec spec;
  spec.size = size;
  spec.value = value;
  return createProgressBar(parent, spec);
}

UiNode createCardGrid(UiNode& parent, CardGridSpec const& spec) {
  auto frame = [&]() -> PrimeFrame::Frame& { return parent.frame(); };
  bool allowAbsolute_ = parent.allowAbsolute();
  Rect bounds = resolve_rect(spec.size);
  if ((bounds.width <= 0.0f || bounds.height <= 0.0f) &&
      !spec.size.preferredWidth.has_value() &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchX <= 0.0f &&
      spec.size.stretchY <= 0.0f &&
      !spec.cards.empty()) {
    if (bounds.width <= 0.0f) {
      bounds.width = spec.cardWidth;
    }
    if (bounds.height <= 0.0f) {
      size_t rows = spec.cards.size();
      if (rows > 1 && spec.gapY > 0.0f) {
        bounds.height = static_cast<float>(rows) * spec.cardHeight +
                        static_cast<float>(rows - 1) * spec.gapY;
      } else {
        bounds.height = static_cast<float>(rows) * spec.cardHeight;
      }
    }
  }
  SizeSpec gridSize = spec.size;
  if (!gridSize.preferredWidth.has_value() && bounds.width > 0.0f) {
    gridSize.preferredWidth = bounds.width;
  }
  if (!gridSize.preferredHeight.has_value() && bounds.height > 0.0f) {
    gridSize.preferredHeight = bounds.height;
  }

  StackSpec gridSpec;
  gridSpec.size = gridSize;
  gridSpec.gap = spec.gapY;
  gridSpec.clipChildren = true;
  UiNode gridNode = parent.createVerticalStack(gridSpec);

  if (spec.cards.empty()) {
    return UiNode(frame(), gridNode.nodeId(), allowAbsolute_);
  }

  size_t columns = 1;
  float gridWidth = bounds.width > 0.0f ? bounds.width : gridSize.preferredWidth.value_or(0.0f);
  if (spec.cardWidth + spec.gapX > 0.0f && gridWidth > 0.0f) {
    columns = std::max<size_t>(1, static_cast<size_t>((gridWidth + spec.gapX) /
                                                      (spec.cardWidth + spec.gapX)));
  }

  float titleLine = resolve_line_height(frame(), textToken(spec.titleRole));
  float subtitleLine = resolve_line_height(frame(), textToken(spec.subtitleRole));

  size_t rowCount = (spec.cards.size() + columns - 1) / columns;
  for (size_t row = 0; row < rowCount; ++row) {
    StackSpec rowSpec;
    rowSpec.size.preferredHeight = spec.cardHeight;
    rowSpec.size.stretchX = 1.0f;
    rowSpec.gap = spec.gapX;
    rowSpec.clipChildren = false;
    UiNode rowNode = gridNode.createHorizontalStack(rowSpec);

    for (size_t col = 0; col < columns; ++col) {
      size_t index = row * columns + col;
      if (index >= spec.cards.size()) {
        break;
      }
      PanelSpec cardPanel;
      cardPanel.rectStyle = rectToken(spec.cardRole);
      cardPanel.size.preferredWidth = spec.cardWidth;
      cardPanel.size.preferredHeight = spec.cardHeight;
      UiNode cardNode = rowNode.createPanel(cardPanel);
      UiNode cardInternal(frame(), cardNode.nodeId(), true);

      CardSpec const& card = spec.cards[index];
      if (!card.title.empty()) {
        Rect titleRect;
        titleRect.x = spec.paddingX;
        titleRect.y = spec.titleOffsetY;
        titleRect.width = std::max(0.0f, spec.cardWidth - spec.paddingX * 2.0f);
        titleRect.height = titleLine;
        create_text_node(frame(),
                         cardInternal.nodeId(),
                         titleRect,
                         card.title,
                         textToken(spec.titleRole),
                         {},
                         PrimeFrame::TextAlign::Start,
                         PrimeFrame::WrapMode::None,
                         titleRect.width,
                         true);
      }
      if (!card.subtitle.empty()) {
        Rect subtitleRect;
        subtitleRect.x = spec.paddingX;
        subtitleRect.y = spec.subtitleOffsetY;
        subtitleRect.width = std::max(0.0f, spec.cardWidth - spec.paddingX * 2.0f);
        subtitleRect.height = subtitleLine;
        create_text_node(frame(),
                         cardInternal.nodeId(),
                         subtitleRect,
                         card.subtitle,
                         textToken(spec.subtitleRole),
                         {},
                         PrimeFrame::TextAlign::Start,
                         PrimeFrame::WrapMode::None,
                         subtitleRect.width,
                         true);
      }
    }
  }

  return UiNode(frame(), gridNode.nodeId(), allowAbsolute_);
}

} // namespace Studio

namespace Studio {

UiNode createButton(UiNode& parent,
                    std::string_view label,
                    ButtonVariant variant,
                    SizeSpec const& size,
                    ButtonCallbacks const& callbacks) {
  ButtonSpec spec;
  spec.label = label;
  spec.size = size;
  if (!spec.size.preferredHeight.has_value() && spec.size.stretchY <= 0.0f) {
    spec.size.preferredHeight = StudioDefaults::ControlHeight;
  }
  if (spec.label.empty() &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f) {
    spec.size.preferredWidth = StudioDefaults::ControlWidthM;
  }
  if (variant == ButtonVariant::Primary) {
    spec.backgroundStyle = rectToken(RectRole::ButtonPrimary);
    spec.hoverStyle = rectToken(RectRole::ButtonPrimaryHover);
    spec.pressedStyle = rectToken(RectRole::ButtonPrimaryPressed);
    spec.baseOpacity = 1.0f;
    spec.hoverOpacity = 1.0f;
    spec.pressedOpacity = 1.0f;
    spec.textStyle = textToken(TextRole::BodyBright);
  } else {
    spec.backgroundStyle = rectToken(RectRole::ButtonBase);
    spec.hoverStyle = rectToken(RectRole::ButtonHover);
    spec.pressedStyle = rectToken(RectRole::ButtonPressed);
    spec.baseOpacity = 1.0f;
    spec.hoverOpacity = 1.0f;
    spec.pressedOpacity = 1.0f;
    spec.textStyle = textToken(TextRole::BodyBright);
  }
  spec.callbacks = callbacks;
  return parent.createButton(spec);
}

} // namespace Studio

namespace Studio {

UiNode createTextField(UiNode& parent, std::string_view placeholder, SizeSpec const& size) {
  TextFieldSpec spec;
  spec.placeholder = placeholder;
  spec.size = size;
  if (!spec.size.preferredHeight.has_value() && spec.size.stretchY <= 0.0f) {
    spec.size.preferredHeight = StudioDefaults::ControlHeight;
  }
  if (!spec.size.minWidth.has_value() && spec.size.stretchX <= 0.0f) {
    spec.size.minWidth = StudioDefaults::FieldWidthL;
  }
  spec.backgroundStyle = rectToken(RectRole::Panel);
  spec.textStyle = textToken(TextRole::BodyBright);
  spec.placeholderStyle = textToken(TextRole::BodyMuted);
  return parent.createTextField(spec);
}

UiNode createStatusBar(UiNode& parent, StatusBarSpec const& spec) {
  auto frame = [&]() -> PrimeFrame::Frame& { return parent.frame(); };
  PrimeFrame::NodeId id_ = parent.nodeId();
  bool allowAbsolute_ = parent.allowAbsolute();
  Rect bounds = resolve_rect(spec.size);
  if (bounds.height <= 0.0f &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchY <= 0.0f) {
    bounds.height = StudioDefaults::StatusHeight;
  }
  if (bounds.width <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f) {
    float leftWidth = spec.leftText.empty()
                          ? 0.0f
                          : estimate_text_width(frame(), textToken(spec.leftRole), spec.leftText);
    float rightWidth = spec.rightText.empty()
                           ? 0.0f
                           : estimate_text_width(frame(), textToken(spec.rightRole), spec.rightText);
    float spacing = spec.rightText.empty() ? 0.0f : spec.paddingX;
    bounds.width = spec.paddingX * 2.0f + leftWidth + spacing + rightWidth;
  }
  if (bounds.width <= 0.0f || bounds.height <= 0.0f) {
    return UiNode(frame(), id_, allowAbsolute_);
  }

  PanelSpec panelSpec;
  panelSpec.rectStyle = rectToken(spec.backgroundRole);
  panelSpec.size = spec.size;
  if (!panelSpec.size.preferredWidth.has_value() && bounds.width > 0.0f) {
    panelSpec.size.preferredWidth = bounds.width;
  }
  if (!panelSpec.size.preferredHeight.has_value() && bounds.height > 0.0f) {
    panelSpec.size.preferredHeight = bounds.height;
  }
  UiNode bar = parent.createPanel(panelSpec);

  float leftLineHeight = resolve_line_height(frame(), textToken(spec.leftRole));
  float rightLineHeight = resolve_line_height(frame(), textToken(spec.rightRole));

  float leftY = (bounds.height - leftLineHeight) * 0.5f;
  float rightY = (bounds.height - rightLineHeight) * 0.5f;

  float rightWidth = 0.0f;
  if (!spec.rightText.empty()) {
    rightWidth = estimate_text_width(frame(), textToken(spec.rightRole), spec.rightText);
    float maxRightWidth = std::max(0.0f, bounds.width - spec.paddingX);
    rightWidth = std::min(rightWidth, maxRightWidth);
  }

  float rightX = bounds.width - spec.paddingX - rightWidth;
  if (rightX < 0.0f) {
    rightX = 0.0f;
  }

  float leftMaxWidth = std::max(0.0f, bounds.width - spec.paddingX * 2.0f);
  if (!spec.rightText.empty()) {
    leftMaxWidth = std::min(leftMaxWidth, std::max(0.0f, rightX - spec.paddingX));
  }

  if (!spec.leftText.empty()) {
    Rect leftRect{spec.paddingX, leftY, leftMaxWidth, leftLineHeight};
    create_text_node(frame(),
                     bar.nodeId(),
                     leftRect,
                     spec.leftText,
                     textToken(spec.leftRole),
                     {},
                     PrimeFrame::TextAlign::Start,
                     PrimeFrame::WrapMode::None,
                     leftMaxWidth,
                     true);
  }

  if (!spec.rightText.empty()) {
    Rect rightRect{rightX, rightY, rightWidth, rightLineHeight};
    create_text_node(frame(),
                     bar.nodeId(),
                     rightRect,
                     spec.rightText,
                     textToken(spec.rightRole),
                     {},
                     PrimeFrame::TextAlign::Start,
                     PrimeFrame::WrapMode::None,
                     rightWidth,
                     true);
  }

  return UiNode(frame(), bar.nodeId(), allowAbsolute_);
}

UiNode createStatusBar(UiNode& parent,
                       SizeSpec const& size,
                       std::string_view leftText,
                       std::string_view rightText) {
  StatusBarSpec spec;
  spec.size = size;
  spec.leftText = leftText;
  spec.rightText = rightText;
  return createStatusBar(parent, spec);
}

} // namespace Studio

ScrollView UiNode::createScrollView(ScrollViewSpec const& spec) {
  Rect bounds = resolve_rect(spec.size);
  if (bounds.width <= 0.0f || bounds.height <= 0.0f) {
    return ScrollView{UiNode(frame(), id_, allowAbsolute_),
                      UiNode(frame(), PrimeFrame::NodeId{}, allowAbsolute_)};
  }

  SizeSpec scrollSize = spec.size;
  if (!scrollSize.preferredWidth.has_value() && bounds.width > 0.0f) {
    scrollSize.preferredWidth = bounds.width;
  }
  if (!scrollSize.preferredHeight.has_value() && bounds.height > 0.0f) {
    scrollSize.preferredHeight = bounds.height;
  }
  PrimeFrame::NodeId scrollId = create_node(frame(), id_, bounds,
                                            &scrollSize,
                                            PrimeFrame::LayoutType::None,
                                            PrimeFrame::Insets{},
                                            0.0f,
                                            spec.clipChildren,
                                            spec.visible);
  SizeSpec contentSize;
  contentSize.stretchX = 1.0f;
  contentSize.stretchY = 1.0f;
  PrimeFrame::NodeId contentId = create_node(frame(), scrollId, Rect{},
                                             &contentSize,
                                             PrimeFrame::LayoutType::Overlay,
                                             PrimeFrame::Insets{},
                                             0.0f,
                                             false,
                                             spec.visible);

  if (spec.showVertical && spec.vertical.enabled) {
    float trackW = spec.vertical.thickness;
    float trackH = std::max(0.0f, bounds.height - spec.vertical.startPadding - spec.vertical.endPadding);
    float trackX = bounds.width - spec.vertical.inset;
    float trackY = spec.vertical.startPadding;
    create_rect_node(frame(),
                     scrollId,
                     Rect{trackX, trackY, trackW, trackH},
                     spec.vertical.trackStyle,
                     {},
                     false,
                     spec.visible);

    float thumbH = std::min(trackH, spec.vertical.thumbLength);
    float maxOffset = std::max(0.0f, trackH - thumbH);
    float thumbOffset = std::clamp(spec.vertical.thumbOffset, 0.0f, maxOffset);
    float thumbY = trackY + thumbOffset;
    create_rect_node(frame(),
                     scrollId,
                     Rect{trackX, thumbY, trackW, thumbH},
                     spec.vertical.thumbStyle,
                     {},
                     false,
                     spec.visible);
  }

  if (spec.showHorizontal && spec.horizontal.enabled) {
    float trackH = spec.horizontal.thickness;
    float trackW = std::max(0.0f, bounds.width - spec.horizontal.startPadding - spec.horizontal.endPadding);
    float trackX = spec.horizontal.startPadding;
    float trackY = bounds.height - spec.horizontal.inset;
    create_rect_node(frame(),
                     scrollId,
                     Rect{trackX, trackY, trackW, trackH},
                     spec.horizontal.trackStyle,
                     {},
                     false,
                     spec.visible);

    float thumbW = std::min(trackW, spec.horizontal.thumbLength);
    float maxOffset = std::max(0.0f, trackW - thumbW);
    float thumbOffset = std::clamp(spec.horizontal.thumbOffset, 0.0f, maxOffset);
    float thumbX = trackX + thumbOffset;
    create_rect_node(frame(),
                     scrollId,
                     Rect{thumbX, trackY, thumbW, trackH},
                     spec.horizontal.thumbStyle,
                     {},
                     false,
                     spec.visible);
  }

  return ScrollView{UiNode(frame(), scrollId, allowAbsolute_),
                    UiNode(frame(), contentId, allowAbsolute_)};
}

namespace Studio {

UiNode createScrollHints(UiNode& parent, ScrollHintsSpec const& spec) {
  ScrollViewSpec view;
  view.size = spec.size;
  if (!view.size.preferredWidth.has_value() &&
      !view.size.preferredHeight.has_value() &&
      view.size.stretchX <= 0.0f &&
      view.size.stretchY <= 0.0f) {
    view.size.preferredWidth = StudioDefaults::FieldWidthL;
    view.size.preferredHeight = StudioDefaults::PanelHeightM;
  }
  view.showVertical = spec.showVertical;
  view.showHorizontal = spec.showHorizontal;
  view.vertical.trackStyle = rectToken(RectRole::ScrollTrack);
  view.vertical.thumbStyle = rectToken(RectRole::ScrollThumb);
  view.vertical.thumbLength = spec.verticalThumbLength;
  view.vertical.thumbOffset = spec.verticalThumbOffset;
  view.horizontal.trackStyle = rectToken(RectRole::ScrollTrack);
  view.horizontal.thumbStyle = rectToken(RectRole::ScrollThumb);
  view.horizontal.thumbLength = spec.horizontalThumbLength;
  view.horizontal.thumbOffset = spec.horizontalThumbOffset;
  view.horizontal.startPadding = spec.horizontalStartPadding;
  view.horizontal.endPadding = spec.horizontalEndPadding;
  return parent.createScrollView(view).root;
}

} // namespace Studio

UiNode UiNode::createTreeView(TreeViewSpec const& spec) {
  PrimeFrame::NodeId id_ = nodeId();
  bool allowAbsolute_ = allowAbsolute();

  std::vector<FlatTreeRow> rows;
  std::vector<int> depthStack;
  flatten_tree(spec.nodes, 0, depthStack, rows);

  float rowsHeight = rows.empty()
                         ? spec.rowHeight
                         : static_cast<float>(rows.size()) * spec.rowHeight +
                               static_cast<float>(rows.size() - 1) * spec.rowGap;

  std::vector<int> firstChild(rows.size(), -1);
  std::vector<int> lastChild(rows.size(), -1);
  for (size_t i = 0; i < rows.size(); ++i) {
    if (rows[i].parentIndex >= 0) {
      size_t parent = static_cast<size_t>(rows[i].parentIndex);
      if (firstChild[parent] < 0) {
        firstChild[parent] = static_cast<int>(i);
      }
      lastChild[parent] = static_cast<int>(i);
    }
  }

  Rect bounds = resolve_rect(spec.size);
  if (bounds.width <= 0.0f || bounds.height <= 0.0f) {
    float maxLabelWidth = 0.0f;
    for (FlatTreeRow const& row : rows) {
      PrimeFrame::TextStyleToken role = row.selected ? spec.selectedTextStyle : spec.textStyle;
      float textWidth = estimate_text_width(frame(), role, row.label);
      float indent = row.depth > 0 ? spec.indent * static_cast<float>(row.depth) : 0.0f;
      float contentWidth = spec.rowWidthInset + 20.0f + indent + textWidth;
      if (contentWidth > maxLabelWidth) {
        maxLabelWidth = contentWidth;
      }
    }
    if (bounds.width <= 0.0f) {
      bounds.width = maxLabelWidth;
    }
    if (bounds.height <= 0.0f) {
      bounds.height = spec.rowStartY + rowsHeight;
    }
  }

  if (bounds.width <= 0.0f || bounds.height <= 0.0f) {
    return UiNode(frame(), id_, allowAbsolute_);
  }

  SizeSpec treeSize = spec.size;
  if (!treeSize.preferredWidth.has_value() && bounds.width > 0.0f && treeSize.stretchX <= 0.0f) {
    treeSize.preferredWidth = bounds.width;
  }
  if (!treeSize.preferredHeight.has_value() && bounds.height > 0.0f) {
    treeSize.preferredHeight = bounds.height;
  }

  StackSpec treeSpec;
  treeSpec.size = treeSize;
  treeSpec.gap = 0.0f;
  treeSpec.clipChildren = spec.clipChildren;
  treeSpec.padding.left = 0.0f;
  treeSpec.padding.top = spec.rowStartY;
  treeSpec.padding.right = 0.0f;
  treeSpec.visible = spec.visible;
  UiNode parentNode(frame(), id_, allowAbsolute_);
  UiNode treeNode = parentNode.createOverlay(treeSpec);

  float rowWidth = std::max(0.0f, bounds.width);
  float rowTextHeight = resolve_line_height(frame(), spec.textStyle);
  float selectedTextHeight = resolve_line_height(frame(), spec.selectedTextStyle);
  float caretBaseX = std::max(0.0f, spec.caretBaseX);

  StackSpec rowsSpec;
  rowsSpec.size.stretchX = 1.0f;
  rowsSpec.size.stretchY = spec.size.stretchY;
  rowsSpec.gap = spec.rowGap;
  rowsSpec.clipChildren = false;
  rowsSpec.visible = spec.visible;

  if (spec.showHeaderDivider && spec.visible) {
    float dividerY = spec.headerDividerY;
    add_divider_rect(frame(), treeNode.nodeId(),
                     Rect{0.0f, dividerY, rowWidth, spec.connectorThickness},
                     spec.connectorStyle);
  }

  UiNode rowsNode = treeNode.createVerticalStack(rowsSpec);

  for (size_t i = 0; i < rows.size(); ++i) {
    FlatTreeRow const& row = rows[i];
    PrimeFrame::RectStyleToken rowRole = row.selected ? spec.selectionStyle
                                                      : (i % 2 == 0 ? spec.rowAltStyle : spec.rowStyle);

    PanelSpec rowPanel;
    rowPanel.rectStyle = rowRole;
    rowPanel.layout = PrimeFrame::LayoutType::Overlay;
    rowPanel.size.preferredHeight = spec.rowHeight;
    rowPanel.size.stretchX = 1.0f;
    rowPanel.clipChildren = false;
    rowPanel.visible = spec.visible;
    UiNode rowNode = rowsNode.createPanel(rowPanel);
    PrimeFrame::NodeId rowId = rowNode.nodeId();

    if (spec.showConnectors && row.depth > 0 && spec.visible) {
      float halfThickness = spec.connectorThickness * 0.5f;
      float rowCenterY = spec.rowHeight * 0.5f;
      float rowTop = -spec.rowGap * 0.5f;
      float rowBottom = spec.rowHeight + spec.rowGap * 0.5f;

      auto draw_trunk_segment = [&](size_t depthIndex, int ancestorIndex) {
        if (ancestorIndex < 0) {
          return;
        }
        FlatTreeRow const& ancestor = rows[static_cast<size_t>(ancestorIndex)];
        if (!ancestor.hasChildren || !ancestor.expanded) {
          return;
        }
        int first = firstChild[static_cast<size_t>(ancestorIndex)];
        int last = lastChild[static_cast<size_t>(ancestorIndex)];
        if (first < 0) {
          return;
        }
        if (static_cast<int>(i) != ancestorIndex &&
            (static_cast<int>(i) < first || static_cast<int>(i) > last)) {
          return;
        }
        float trunkX = caretBaseX + static_cast<float>(depthIndex) * spec.indent +
                       spec.caretSize * 0.5f;
        float segmentTop = rowTop;
        float segmentBottom = rowBottom;
        if (static_cast<int>(i) == ancestorIndex) {
          segmentTop = rowCenterY;
        }
        if (static_cast<int>(i) == last) {
          segmentBottom = rowCenterY;
        }
        if (segmentBottom > segmentTop + 0.5f) {
          add_divider_rect(frame(), rowNode.nodeId(),
                           Rect{trunkX - halfThickness, segmentTop - halfThickness,
                                  spec.connectorThickness,
                                  (segmentBottom - segmentTop) + spec.connectorThickness},
                           spec.connectorStyle);
        }
      };

      for (size_t depthIndex = 0; depthIndex < row.ancestors.size(); ++depthIndex) {
        draw_trunk_segment(depthIndex, row.ancestors[depthIndex]);
      }
      if (row.hasChildren && row.expanded) {
        draw_trunk_segment(static_cast<size_t>(row.depth), static_cast<int>(i));
      }

      int parentIndex = row.parentIndex;
      if (parentIndex >= 0) {
        float trunkX = caretBaseX + static_cast<float>(row.depth - 1) * spec.indent +
                       spec.caretSize * 0.5f;
        float childTrunkX = caretBaseX + static_cast<float>(row.depth) * spec.indent +
                            spec.caretSize * 0.5f;
        float linkStartX = trunkX - halfThickness;
        float linkEndX = childTrunkX + halfThickness;
        float linkW = linkEndX - linkStartX;
        if (linkW > 0.5f) {
          add_divider_rect(frame(), rowNode.nodeId(),
                           Rect{linkStartX, rowCenterY - halfThickness,
                                  linkW, spec.connectorThickness},
                           spec.connectorStyle);
        }
      }
    }

    if (row.selected && spec.selectionAccentWidth > 0.0f && spec.visible) {
      create_rect_node(frame(),
                       rowId,
                       Rect{0.0f, 0.0f, spec.selectionAccentWidth, spec.rowHeight},
                       spec.selectionAccentStyle,
                       {},
                       false,
                       spec.visible);
    }

    float indent = (row.depth > 0) ? spec.indent * static_cast<float>(row.depth) : 0.0f;
    float glyphX = caretBaseX + indent;
    float glyphY = (spec.rowHeight - spec.caretSize) * 0.5f;

    if (spec.showCaretMasks && row.depth > 0 && spec.visible) {
      float maskPad = spec.caretMaskPad;
      create_rect_node(frame(),
                       rowId,
                       Rect{glyphX - maskPad,
                            glyphY - maskPad,
                            spec.caretSize + maskPad * 2.0f,
                            spec.caretSize + maskPad * 2.0f},
                       rowRole,
                       {},
                       false,
                       spec.visible);
    }

    if (row.hasChildren) {
      create_rect_node(frame(),
                       rowId,
                       Rect{glyphX, glyphY, spec.caretSize, spec.caretSize},
                       spec.caretBackgroundStyle,
                       {},
                       false,
                       spec.visible);

      create_rect_node(frame(),
                       rowId,
                       Rect{glyphX + spec.caretInset,
                            glyphY + spec.caretSize * 0.5f - spec.caretThickness * 0.5f,
                            spec.caretSize - spec.caretInset * 2.0f,
                            spec.caretThickness},
                       spec.caretLineStyle,
                       {},
                       false,
                       spec.visible);
      if (!row.expanded) {
        create_rect_node(frame(),
                         rowId,
                         Rect{glyphX + spec.caretSize * 0.5f - spec.caretThickness * 0.5f,
                              glyphY + spec.caretInset,
                              spec.caretThickness,
                              spec.caretSize - spec.caretInset * 2.0f},
                         spec.caretLineStyle,
                         {},
                         false,
                         spec.visible);
      }
    } else {
      create_rect_node(frame(),
                       rowId,
                       Rect{glyphX, glyphY, spec.caretSize, spec.caretSize},
                       spec.caretBackgroundStyle,
                       {},
                       false,
                       spec.visible);

      float dot = std::max(2.0f, spec.caretThickness);
      create_rect_node(frame(),
                       rowId,
                       Rect{glyphX + spec.caretSize * 0.5f - dot * 0.5f,
                            glyphY + spec.caretSize * 0.5f - dot * 0.5f,
                            dot,
                            dot},
                       spec.caretLineStyle,
                       {},
                       false,
                       spec.visible);
    }

    float textX = spec.rowStartX + 20.0f + indent;
    PrimeFrame::TextStyleToken textRole = row.selected ? spec.selectedTextStyle : spec.textStyle;
    float lineHeight = row.selected ? selectedTextHeight : rowTextHeight;
    float textY = (spec.rowHeight - lineHeight) * 0.5f;
    float labelWidth = std::max(0.0f, rowWidth - spec.rowWidthInset - textX);
    create_text_node(frame(),
                     rowId,
                     Rect{textX, textY, labelWidth, lineHeight},
                     row.label,
                     textRole,
                     {},
                     PrimeFrame::TextAlign::Start,
                     PrimeFrame::WrapMode::None,
                     labelWidth,
                     spec.visible);
  }

  if (spec.showScrollBar && spec.scrollBar.enabled && spec.visible) {
    float trackX = bounds.width - spec.scrollBar.inset;
    float trackY = spec.scrollBar.padding;
    float trackH = std::max(0.0f, bounds.height - spec.scrollBar.padding * 2.0f);
    float trackW = spec.scrollBar.width;
    create_rect_node(frame(),
                     treeNode.nodeId(),
                     Rect{trackX, trackY, trackW, trackH},
                     spec.scrollBar.trackStyle,
                     {},
                     false,
                     spec.visible);

    float contentHeight = spec.rowStartY + rowsHeight;
    float thumbFraction = spec.scrollBar.thumbFraction;
    float thumbProgress = spec.scrollBar.thumbProgress;
    if (spec.scrollBar.autoThumb) {
      if (contentHeight > 0.0f && bounds.height > 0.0f) {
        thumbFraction = std::clamp(bounds.height / contentHeight, 0.0f, 1.0f);
      } else {
        thumbFraction = 1.0f;
      }
      thumbProgress = std::clamp(thumbProgress, 0.0f, 1.0f);
    }

    float thumbH = trackH * thumbFraction;
    thumbH = std::max(thumbH, spec.scrollBar.minThumbHeight);
    if (thumbH > trackH) {
      thumbH = trackH;
    }
    float maxOffset = std::max(0.0f, trackH - thumbH);
    float progress = std::clamp(thumbProgress, 0.0f, 1.0f);
    float thumbY = trackY + maxOffset * progress;
    create_rect_node(frame(),
                     treeNode.nodeId(),
                     Rect{trackX, thumbY, trackW, thumbH},
                     spec.scrollBar.thumbStyle,
                     {},
                     false,
                     spec.visible);
  }

  return UiNode(frame(), treeNode.nodeId(), allowAbsolute_);
}

namespace Studio {

UiNode createRoot(PrimeFrame::Frame& frame, SizeSpec const& size) {
  Rect rootBounds = resolve_rect(size);
  PrimeFrame::NodeId id = create_node(frame, PrimeFrame::NodeId{}, rootBounds,
                                      nullptr,
                                      PrimeFrame::LayoutType::None,
                                      PrimeFrame::Insets{},
                                      0.0f,
                                      false,
                                      true);
  return UiNode(frame, id);
}

UiNode createStudioRoot(PrimeFrame::Frame& frame, SizeSpec const& size) {
  ensure_studio_theme(frame);
  return createRoot(frame, size);
}

ShellLayout createShell(PrimeFrame::Frame& frame, ShellSpec const& spec) {
  ensure_studio_theme(frame);
  Rect shellBounds = resolve_rect(spec.size);
  float width = shellBounds.width;
  float height = shellBounds.height;
  float contentH = std::max(0.0f, height - spec.topbarHeight - spec.statusHeight);
  float contentW = std::max(0.0f, width - spec.sidebarWidth - spec.inspectorWidth);
  SizeSpec shellSize = spec.size;
  if (!shellSize.preferredWidth.has_value() && width > 0.0f) {
    shellSize.preferredWidth = width;
  }
  if (!shellSize.preferredHeight.has_value() && height > 0.0f) {
    shellSize.preferredHeight = height;
  }
  UiNode root = createRoot(frame, shellSize);
  StackSpec overlaySpec;
  overlaySpec.size.preferredWidth = width;
  overlaySpec.size.preferredHeight = height;
  overlaySpec.clipChildren = false;
  UiNode overlay = root.createOverlay(overlaySpec);

  SizeSpec fullSize;
  fullSize.preferredWidth = width;
  fullSize.preferredHeight = height;

  UiNode background = overlay.createPanel(rectToken(spec.backgroundRole), fullSize);

  StackSpec shellSpec;
  shellSpec.size = fullSize;
  shellSpec.gap = 0.0f;
  shellSpec.clipChildren = false;
  UiNode shellStack = overlay.createVerticalStack(shellSpec);

  PanelSpec topbarSpec;
  topbarSpec.rectStyle = rectToken(spec.topbarRole);
  topbarSpec.size.preferredHeight = spec.topbarHeight;
  UiNode topbar = shellStack.createPanel(topbarSpec);

  StackSpec bodySpec;
  bodySpec.size.stretchY = 1.0f;
  bodySpec.size.stretchX = 1.0f;
  bodySpec.gap = 0.0f;
  bodySpec.clipChildren = false;
  UiNode bodyRow = shellStack.createHorizontalStack(bodySpec);

  PanelSpec sidebarSpec;
  sidebarSpec.rectStyle = rectToken(spec.sidebarRole);
  sidebarSpec.size.preferredWidth = spec.sidebarWidth;
  UiNode sidebar = bodyRow.createPanel(sidebarSpec);

  PanelSpec contentSpec;
  contentSpec.rectStyle = rectToken(spec.contentRole);
  contentSpec.size.stretchX = 1.0f;
  UiNode content = bodyRow.createPanel(contentSpec);

  PanelSpec inspectorSpec;
  inspectorSpec.rectStyle = rectToken(spec.inspectorRole);
  inspectorSpec.size.preferredWidth = spec.inspectorWidth;
  UiNode inspector = bodyRow.createPanel(inspectorSpec);

  PanelSpec statusSpec;
  statusSpec.rectStyle = rectToken(spec.statusRole);
  statusSpec.size.preferredHeight = spec.statusHeight;
  UiNode status = shellStack.createPanel(statusSpec);

  if (spec.drawDividers) {
    create_rect_node(frame,
                     root.nodeId(),
                     Rect{0.0f, spec.topbarHeight, width, 1.0f},
                     rectToken(spec.dividerRole),
                     {},
                     false,
                     true);
    create_rect_node(frame,
                     root.nodeId(),
                     Rect{spec.sidebarWidth - 1.0f, spec.topbarHeight, 1.0f, contentH},
                     rectToken(spec.dividerRole),
                     {},
                     false,
                     true);
    create_rect_node(frame,
                     root.nodeId(),
                     Rect{width - spec.inspectorWidth, spec.topbarHeight, 1.0f, contentH},
                     rectToken(spec.dividerRole),
                     {},
                     false,
                     true);
    create_rect_node(frame,
                     root.nodeId(),
                     Rect{0.0f, height - spec.statusHeight, width, 1.0f},
                     rectToken(spec.dividerRole),
                     {},
                     false,
                     true);
  }

  return ShellLayout{
      root,
      background,
      topbar,
      status,
      sidebar,
      content,
      inspector
  };
}

ShellSpec makeShellSpec(SizeSpec const& size) {
  ShellSpec spec;
  spec.size = size;
  return spec;
}

} // namespace Studio

Version getVersion() {
  return Version{};
}

std::string_view getVersionString() {
  return "0.1.0";
}

} // namespace PrimeStage
