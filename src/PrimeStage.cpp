#include "PrimeStage/StudioUi.h"
#include "PrimeStage/PrimeStage.h"

#include <algorithm>
#include <cctype>
#include <utility>

namespace PrimeStage {
namespace {

Bounds inset_bounds(Bounds const& bounds, float left, float top, float right, float bottom) {
  float width = std::max(0.0f, bounds.width - left - right);
  float height = std::max(0.0f, bounds.height - top - bottom);
  return Bounds{bounds.x + left, bounds.y + top, width, height};
}

void apply_bounds(PrimeFrame::Node& node, Bounds const& bounds) {
  node.localX = bounds.x;
  node.localY = bounds.y;
  if (bounds.width > 0.0f) {
    node.sizeHint.width.preferred = bounds.width;
  } else {
    node.sizeHint.width.preferred.reset();
  }
  if (bounds.height > 0.0f) {
    node.sizeHint.height.preferred = bounds.height;
  } else {
    node.sizeHint.height.preferred.reset();
  }
}

void apply_size_spec(PrimeFrame::Node& node, Bounds const& bounds, SizeSpec const& size) {
  node.sizeHint.width.min = size.minWidth;
  node.sizeHint.width.max = size.maxWidth;
  if (bounds.width <= 0.0f && size.preferredWidth.has_value()) {
    node.sizeHint.width.preferred = size.preferredWidth;
  }
  node.sizeHint.width.stretch = size.stretchX;

  node.sizeHint.height.min = size.minHeight;
  node.sizeHint.height.max = size.maxHeight;
  if (bounds.height <= 0.0f && size.preferredHeight.has_value()) {
    node.sizeHint.height.preferred = size.preferredHeight;
  }
  node.sizeHint.height.stretch = size.stretchY;
}

Bounds resolve_bounds(Bounds const& bounds, SizeSpec const& size) {
  Bounds resolved = bounds;
  if (resolved.width <= 0.0f && size.preferredWidth.has_value()) {
    resolved.width = *size.preferredWidth;
  }
  if (resolved.height <= 0.0f && size.preferredHeight.has_value()) {
    resolved.height = *size.preferredHeight;
  }
  return resolved;
}

SizeSpec size_from_bounds(Bounds const& bounds) {
  SizeSpec size;
  if (bounds.width > 0.0f) {
    size.preferredWidth = bounds.width;
  }
  if (bounds.height > 0.0f) {
    size.preferredHeight = bounds.height;
  }
  return size;
}

PrimeFrame::NodeId create_node(PrimeFrame::Frame& frame,
                               PrimeFrame::NodeId parent,
                               Bounds const& bounds,
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
  apply_bounds(*node, bounds);
  if (size) {
    apply_size_spec(*node, bounds, *size);
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

void add_text_primitive(PrimeFrame::Frame& frame,
                        PrimeFrame::NodeId nodeId,
                        LabelSpec const& spec) {
  PrimeFrame::Primitive prim;
  prim.type = PrimeFrame::PrimitiveType::Text;
  prim.width = spec.bounds.width;
  prim.height = spec.bounds.height;
  prim.textBlock.text = spec.text;
  prim.textBlock.align = spec.align;
  prim.textBlock.wrap = spec.wrap;
  prim.textBlock.maxWidth = spec.maxWidth;
  prim.textStyle.token = spec.textStyle;
  prim.textStyle.overrideStyle = spec.textStyleOverride;
  PrimeFrame::PrimitiveId pid = frame.addPrimitive(prim);
  if (PrimeFrame::Node* node = frame.getNode(nodeId)) {
    node->primitives.push_back(pid);
  }
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

  float spaceWidth = estimate_text_width(frame, token, " ");
  float lineWidth = 0.0f;
  int lines = 1;
  std::string word;

  auto flush_word = [&]() {
    if (word.empty()) {
      return;
    }
    float wordWidth = estimate_text_width(frame, token, word);
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
  std::string label;
  int depth = 0;
  int parentIndex = -1;
  bool hasChildren = false;
  bool expanded = true;
  bool selected = false;
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
                      Bounds const& bounds,
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

Bounds insetBounds(Bounds const& bounds, float inset) {
  return inset_bounds(bounds, inset, inset, inset, inset);
}

Bounds insetBounds(Bounds const& bounds, float insetX, float insetY) {
  return inset_bounds(bounds, insetX, insetY, insetX, insetY);
}

Bounds insetBounds(Bounds const& bounds, float left, float top, float right, float bottom) {
  return inset_bounds(bounds, left, top, right, bottom);
}

Bounds alignBottomRight(Bounds const& bounds, float width, float height, float insetX, float insetY) {
  float x = bounds.x + std::max(0.0f, bounds.width - width - insetX);
  float y = bounds.y + std::max(0.0f, bounds.height - height - insetY);
  return Bounds{x, y, width, height};
}

Bounds alignCenterY(Bounds const& bounds, float height) {
  float y = bounds.y + (bounds.height - height) * 0.5f;
  return Bounds{bounds.x, y, bounds.width, height};
}

void setScrollBarThumbPixels(ScrollBarSpec& spec,
                             float trackHeight,
                             float thumbHeight,
                             float thumbOffset) {
  float track = std::max(1.0f, trackHeight);
  float thumb = std::max(0.0f, std::min(thumbHeight, track));
  float maxOffset = std::max(1.0f, track - thumb);
  spec.thumbFraction = std::clamp(thumb / track, 0.0f, 1.0f);
  spec.thumbProgress = std::clamp(thumbOffset / maxOffset, 0.0f, 1.0f);
}

RowLayout::RowLayout(Bounds const& bounds, float gap)
    : bounds(bounds),
      gap(gap),
      cursorLeft(bounds.x),
      cursorRight(bounds.x + bounds.width) {}

Bounds RowLayout::takeLeft(float width, float height) {
  float h = height > 0.0f ? height : bounds.height;
  Bounds out{cursorLeft, bounds.y, width, h};
  cursorLeft += width + gap;
  return out;
}

Bounds RowLayout::takeRight(float width, float height) {
  float h = height > 0.0f ? height : bounds.height;
  float x = cursorRight - width;
  if (x < cursorLeft) {
    x = cursorLeft;
  }
  Bounds out{x, bounds.y, width, h};
  cursorRight = x - gap;
  return out;
}

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
      PrimeFrame::Color{0.588f, 0.620f, 0.667f, 1.0f}  // Text Muted
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
      PrimeFrame::RectStyle{12, 1.0f}
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

UiNode::UiNode(PrimeFrame::Frame& frame, PrimeFrame::NodeId id, bool allowAbsolute)
    : frame_(frame), id_(id), allowAbsolute_(allowAbsolute) {}

Bounds UiNode::sanitizeBounds(Bounds bounds) const {
  if (!allowAbsolute_) {
    bounds.x = 0.0f;
    bounds.y = 0.0f;
  }
  return bounds;
}

Bounds UiNode::resolveLayoutBounds(Bounds const& bounds, SizeSpec const& size) const {
  return sanitizeBounds(resolve_bounds(bounds, size));
}

UiNode& UiNode::setSize(SizeSpec const& size) {
  PrimeFrame::Node* node = frame().getNode(id_);
  if (!node) {
    return *this;
  }
  apply_size_spec(*node, Bounds{}, size);
  return *this;
}

UiNode UiNode::createVerticalStack(StackSpec const& spec) {
  Bounds bounds;
  PrimeFrame::NodeId nodeId = create_node(frame(), id_, bounds,
                                          &spec.size,
                                          PrimeFrame::LayoutType::VerticalStack,
                                          spec.padding,
                                          spec.gap,
                                          spec.clipChildren,
                                          spec.visible);
  return UiNode(frame(), nodeId, allowAbsolute_);
}

UiNode UiNode::createHorizontalStack(StackSpec const& spec) {
  Bounds bounds;
  PrimeFrame::NodeId nodeId = create_node(frame(), id_, bounds,
                                          &spec.size,
                                          PrimeFrame::LayoutType::HorizontalStack,
                                          spec.padding,
                                          spec.gap,
                                          spec.clipChildren,
                                          spec.visible);
  return UiNode(frame(), nodeId, allowAbsolute_);
}

UiNode UiNode::createOverlay(StackSpec const& spec) {
  Bounds bounds;
  PrimeFrame::NodeId nodeId = create_node(frame(), id_, bounds,
                                          &spec.size,
                                          PrimeFrame::LayoutType::Overlay,
                                          spec.padding,
                                          spec.gap,
                                          spec.clipChildren,
                                          spec.visible);
  return UiNode(frame(), nodeId, allowAbsolute_);
}

UiNode UiNode::createPanel(PanelSpec const& spec) {
  Bounds bounds = resolveLayoutBounds(spec.bounds, spec.size);
  PrimeFrame::NodeId nodeId = create_node(frame(), id_, bounds,
                                          &spec.size,
                                          spec.layout,
                                          spec.padding,
                                          spec.gap,
                                          spec.clipChildren,
                                          spec.visible);
  add_rect_primitive(frame(), nodeId, spec.rectStyle, spec.rectStyleOverride);
  return UiNode(frame(), nodeId, allowAbsolute_);
}

UiNode UiNode::createPanel(PrimeFrame::RectStyleToken rectStyle, Bounds const& bounds) {
  return createPanel(rectStyle, size_from_bounds(bounds));
}

UiNode UiNode::createPanel(PrimeFrame::RectStyleToken rectStyle, SizeSpec const& size) {
  PanelSpec spec;
  spec.rectStyle = rectStyle;
  spec.size = size;
  return createPanel(spec);
}

UiNode UiNode::createPanel(RectRole role, Bounds const& bounds) {
  return createPanel(role, size_from_bounds(bounds));
}

UiNode UiNode::createPanel(RectRole role, SizeSpec const& size) {
  return createPanel(rectToken(role), size);
}

UiNode UiNode::createLabel(LabelSpec const& spec) {
  LabelSpec resolved = spec;
  resolved.bounds = resolveLayoutBounds(spec.bounds, spec.size);
  if ((resolved.bounds.width <= 0.0f || resolved.bounds.height <= 0.0f) &&
      !spec.text.empty() &&
      !spec.size.preferredWidth.has_value() &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchX <= 0.0f &&
      spec.size.stretchY <= 0.0f) {
    float lineHeight = resolve_line_height(frame(), spec.textStyle);
    float textWidth = estimate_text_width(frame(), spec.textStyle, spec.text);
    if (resolved.bounds.width <= 0.0f) {
      if (spec.maxWidth > 0.0f) {
        resolved.bounds.width = std::min(textWidth, spec.maxWidth);
      } else {
        resolved.bounds.width = textWidth;
      }
    }
    if (resolved.bounds.height <= 0.0f) {
      float wrapWidth = resolved.bounds.width;
      if (spec.maxWidth > 0.0f) {
        wrapWidth = spec.maxWidth;
      }
      float height = lineHeight;
      if (spec.wrap != PrimeFrame::WrapMode::None && wrapWidth > 0.0f) {
        std::vector<std::string> lines = wrap_text_lines(frame(), spec.textStyle, spec.text, wrapWidth, spec.wrap);
        height = lineHeight * std::max<size_t>(1, lines.size());
      }
      resolved.bounds.height = height;
    }
  }
  PrimeFrame::NodeId nodeId = create_node(frame(), id_, resolved.bounds,
                                          &spec.size,
                                          PrimeFrame::LayoutType::None,
                                          PrimeFrame::Insets{},
                                          0.0f,
                                          false,
                                          spec.visible);
  add_text_primitive(frame(), nodeId, resolved);
  return UiNode(frame(), nodeId, allowAbsolute_);
}

UiNode UiNode::createLabel(std::string_view text,
                           PrimeFrame::TextStyleToken textStyle,
                           Bounds const& bounds) {
  return createLabel(text, textStyle, size_from_bounds(bounds));
}

UiNode UiNode::createLabel(std::string_view text,
                           PrimeFrame::TextStyleToken textStyle,
                           SizeSpec const& size) {
  LabelSpec spec;
  spec.text = std::string(text);
  spec.textStyle = textStyle;
  spec.size = size;
  return createLabel(spec);
}

UiNode UiNode::createLabel(std::string_view text, TextRole role, Bounds const& bounds) {
  return createLabel(text, role, size_from_bounds(bounds));
}

UiNode UiNode::createLabel(std::string_view text, TextRole role, SizeSpec const& size) {
  return createLabel(text, textToken(role), size);
}

UiNode UiNode::createParagraph(ParagraphSpec const& spec) {
  Bounds bounds = resolveLayoutBounds(spec.bounds, spec.size);
  PrimeFrame::TextStyleToken token = spec.textStyle;
  float maxWidth = spec.maxWidth > 0.0f ? spec.maxWidth : bounds.width;
  std::vector<std::string> lines = wrap_text_lines(frame(), token, spec.text, maxWidth, spec.wrap);

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
  UiNode paragraphInternal(frame(), paragraphId, true);

  for (size_t i = 0; i < lines.size(); ++i) {
    LabelSpec label;
    label.bounds = Bounds{0.0f,
                          spec.textOffsetY + static_cast<float>(i) * lineHeight,
                          maxWidth > 0.0f ? maxWidth : bounds.width,
                          lineHeight};
    label.text = lines[i];
    label.textStyle = token;
    label.textStyleOverride = spec.textStyleOverride;
    label.align = spec.align;
    label.wrap = PrimeFrame::WrapMode::None;
    label.visible = spec.visible;
    paragraphInternal.createLabel(label);
  }

  return UiNode(frame(), paragraphId, allowAbsolute_);
}

UiNode UiNode::createParagraph(Bounds const& bounds,
                               std::string_view text,
                               PrimeFrame::TextStyleToken textStyle) {
  return createParagraph(text, textStyle, size_from_bounds(bounds));
}

UiNode UiNode::createParagraph(std::string_view text,
                               PrimeFrame::TextStyleToken textStyle,
                               SizeSpec const& size) {
  ParagraphSpec spec;
  spec.text = std::string(text);
  spec.textStyle = textStyle;
  spec.size = size;
  return createParagraph(spec);
}

UiNode UiNode::createParagraph(Bounds const& bounds,
                               std::string_view text,
                               TextRole role) {
  return createParagraph(text, role, size_from_bounds(bounds));
}

UiNode UiNode::createParagraph(std::string_view text,
                               TextRole role,
                               SizeSpec const& size) {
  return createParagraph(text, textToken(role), size);
}

UiNode UiNode::createTextLine(TextLineSpec const& spec) {
  PrimeFrame::TextStyleToken token = spec.textStyle;
  float lineHeight = resolve_line_height(frame(), token);
  Bounds bounds = resolveLayoutBounds(spec.bounds, spec.size);
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
  float textY = bounds.y + (containerHeight - lineHeight) * 0.5f + spec.textOffsetY;

  LabelSpec label;
  label.text = spec.text;
  label.textStyle = token;
  label.textStyleOverride = spec.textStyleOverride;
  label.wrap = PrimeFrame::WrapMode::None;
  label.visible = spec.visible;

  float textWidth = estimate_text_width(frame(), token, spec.text);
  float containerWidth = bounds.width;
  bool manualAlign = spec.align != PrimeFrame::TextAlign::Start &&
                     containerWidth > 0.0f &&
                     textWidth > 0.0f;

  if (manualAlign) {
    float offset = 0.0f;
    if (spec.align == PrimeFrame::TextAlign::Center) {
      offset = (containerWidth - textWidth) * 0.5f;
    } else if (spec.align == PrimeFrame::TextAlign::End) {
      offset = containerWidth - textWidth;
    }
    float x = bounds.x + offset;
    if (x < bounds.x) {
      x = bounds.x;
    }
    label.bounds = Bounds{x, textY, textWidth, lineHeight};
    label.align = PrimeFrame::TextAlign::Start;
    label.maxWidth = textWidth;
  } else {
    float width = containerWidth > 0.0f ? containerWidth : textWidth;
    label.bounds = Bounds{bounds.x, textY, width, lineHeight};
    label.align = spec.align;
    label.maxWidth = width;
  }

  UiNode internal(frame(), id_, true);
  UiNode line = internal.createLabel(label);
  return UiNode(frame(), line.nodeId(), allowAbsolute_);
}

UiNode UiNode::createTextLine(Bounds const& bounds,
                              std::string_view text,
                              PrimeFrame::TextStyleToken textStyle,
                              PrimeFrame::TextAlign align) {
  return createTextLine(text, textStyle, size_from_bounds(bounds), align);
}

UiNode UiNode::createTextLine(std::string_view text,
                              PrimeFrame::TextStyleToken textStyle,
                              SizeSpec const& size,
                              PrimeFrame::TextAlign align) {
  TextLineSpec spec;
  spec.text = std::string(text);
  spec.textStyle = textStyle;
  spec.align = align;
  spec.size = size;
  return createTextLine(spec);
}

UiNode UiNode::createTextLine(Bounds const& bounds,
                              std::string_view text,
                              TextRole role,
                              PrimeFrame::TextAlign align) {
  return createTextLine(text, role, size_from_bounds(bounds), align);
}

UiNode UiNode::createTextLine(std::string_view text,
                              TextRole role,
                              SizeSpec const& size,
                              PrimeFrame::TextAlign align) {
  return createTextLine(text, textToken(role), size, align);
}

UiNode UiNode::createDivider(DividerSpec const& spec) {
  DividerSpec resolved = spec;
  resolved.bounds = resolveLayoutBounds(spec.bounds, spec.size);
  if ((resolved.bounds.width <= 0.0f || resolved.bounds.height <= 0.0f) &&
      !spec.size.preferredWidth.has_value() &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchX <= 0.0f &&
      spec.size.stretchY <= 0.0f) {
    if (resolved.bounds.width <= 0.0f) {
      resolved.bounds.width = UiDefaults::DividerThickness;
    }
    if (resolved.bounds.height <= 0.0f) {
      resolved.bounds.height = UiDefaults::DividerThickness;
    }
  }
  PrimeFrame::NodeId nodeId = create_node(frame(), id_, resolved.bounds,
                                          &spec.size,
                                          PrimeFrame::LayoutType::None,
                                          PrimeFrame::Insets{},
                                          0.0f,
                                          false,
                                          spec.visible);
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
  Bounds bounds = resolveLayoutBounds(spec.bounds, spec.size);
  if ((bounds.width <= 0.0f || bounds.height <= 0.0f) &&
      !spec.size.preferredWidth.has_value() &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchX <= 0.0f &&
      spec.size.stretchY <= 0.0f) {
    if (bounds.width <= 0.0f) {
      bounds.width = UiDefaults::PanelInset;
    }
    if (bounds.height <= 0.0f) {
      bounds.height = UiDefaults::PanelInset;
    }
  }
  PrimeFrame::NodeId nodeId = create_node(frame(), id_, bounds,
                                          &spec.size,
                                          PrimeFrame::LayoutType::None,
                                          PrimeFrame::Insets{},
                                          0.0f,
                                          false,
                                          spec.visible);
  return UiNode(frame(), nodeId, allowAbsolute_);
}

UiNode UiNode::createSpacer(SizeSpec const& size) {
  SpacerSpec spec;
  spec.size = size;
  return createSpacer(spec);
}
UiNode UiNode::createTable(TableSpec const& spec) {
  Bounds tableBounds = resolveLayoutBounds(spec.bounds, spec.size);
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

  PrimeFrame::NodeId tableId = create_node(frame(), id_, tableBounds,
                                           &spec.size,
                                           PrimeFrame::LayoutType::None,
                                           PrimeFrame::Insets{},
                                           0.0f,
                                           spec.clipChildren,
                                           true);
  UiNode tableNodeInternal(frame(), tableId, true);

  float tableWidth = tableBounds.width;
  std::vector<float> columnWidths;
  columnWidths.reserve(spec.columns.size());
  float fixedWidth = 0.0f;
  size_t autoCount = 0;
  for (TableColumn const& col : spec.columns) {
    if (col.width > 0.0f) {
      columnWidths.push_back(col.width);
      fixedWidth += col.width;
    } else {
      columnWidths.push_back(0.0f);
      ++autoCount;
    }
  }
  if (autoCount > 0 && tableWidth > fixedWidth) {
    float remaining = tableWidth - fixedWidth;
    float autoWidth = remaining / static_cast<float>(autoCount);
    for (float& width : columnWidths) {
      if (width == 0.0f) {
        width = autoWidth;
      }
    }
  }

  float headerY = spec.headerInset;
  if (spec.headerHeight > 0.0f) {
    PanelSpec headerPanel;
    headerPanel.rectStyle = rectToken(spec.headerRole);
    headerPanel.bounds = Bounds{0.0f, headerY, tableWidth, spec.headerHeight};
    tableNodeInternal.createPanel(headerPanel);
  }

  if (spec.showHeaderDividers) {
    DividerSpec divider;
    divider.rectStyle = rectToken(spec.dividerRole);
    divider.bounds = Bounds{0.0f, 0.0f, tableWidth, 1.0f};
    tableNodeInternal.createDivider(divider);
    divider.bounds = Bounds{0.0f, headerY + spec.headerHeight, tableWidth, 1.0f};
    tableNodeInternal.createDivider(divider);
  }

  float cursorX = 0.0f;
  if (spec.headerHeight > 0.0f) {
    for (size_t i = 0; i < spec.columns.size(); ++i) {
      TableColumn const& col = spec.columns[i];
      float colWidth = i < columnWidths.size() ? columnWidths[i] : 0.0f;
      float textWidth = std::max(0.0f, colWidth - spec.headerPaddingX);
      float headerTextHeight = resolve_line_height(frame(), textToken(col.headerRole));
      float headerTextY = headerY + (spec.headerHeight - headerTextHeight) * 0.5f;
      LabelSpec headerLabel;
      headerLabel.text = col.label;
      headerLabel.textStyle = textToken(col.headerRole);
      headerLabel.bounds = Bounds{cursorX + spec.headerPaddingX,
                                  headerTextY,
                                  textWidth,
                                  headerTextHeight};
      tableNodeInternal.createLabel(headerLabel);
      cursorX += colWidth;
    }
  }

  float rowsY = headerY + spec.headerHeight;
  for (size_t rowIndex = 0; rowIndex < rowCount; ++rowIndex) {
    float rowY = rowsY + static_cast<float>(rowIndex) * (spec.rowHeight + spec.rowGap);
    RectRole rowRole = (rowIndex % 2 == 0) ? spec.rowAltRole : spec.rowRole;
    PanelSpec rowPanel;
    rowPanel.rectStyle = rectToken(rowRole);
    rowPanel.bounds = Bounds{0.0f, rowY, tableWidth, spec.rowHeight};
    tableNodeInternal.createPanel(rowPanel);

    cursorX = 0.0f;
    for (size_t colIndex = 0; colIndex < spec.columns.size(); ++colIndex) {
      TableColumn const& col = spec.columns[colIndex];
      float colWidth = colIndex < columnWidths.size() ? columnWidths[colIndex] : 0.0f;
      float textWidth = std::max(0.0f, colWidth - spec.cellPaddingX);
      std::string cellText;
      if (rowIndex < spec.rows.size() && colIndex < spec.rows[rowIndex].size()) {
        cellText = spec.rows[rowIndex][colIndex];
      }
      float cellTextHeight = resolve_line_height(frame(), textToken(col.cellRole));
      float cellTextY = rowY + (spec.rowHeight - cellTextHeight) * 0.5f;
      LabelSpec cellLabel;
      cellLabel.text = cellText;
      cellLabel.textStyle = textToken(col.cellRole);
      cellLabel.bounds = Bounds{cursorX + spec.cellPaddingX,
                                cellTextY,
                                textWidth,
                                cellTextHeight};
      tableNodeInternal.createLabel(cellLabel);
      cursorX += colWidth;
    }
  }

  if (spec.showColumnDividers && spec.columns.size() > 1) {
    DividerSpec divider;
    divider.rectStyle = rectToken(spec.dividerRole);
    float cursor = 0.0f;
    for (size_t colIndex = 0; colIndex + 1 < spec.columns.size(); ++colIndex) {
      float colWidth = colIndex < columnWidths.size() ? columnWidths[colIndex] : 0.0f;
      cursor += colWidth;
      if (spec.headerHeight > 0.0f) {
        divider.bounds = Bounds{cursor, headerY, 1.0f, spec.headerHeight};
        tableNodeInternal.createDivider(divider);
      }
      if (rowsHeight > 0.0f) {
        divider.bounds = Bounds{cursor, rowsY, 1.0f, rowsHeight};
        tableNodeInternal.createDivider(divider);
      }
    }
  }

  return UiNode(frame(), tableId, allowAbsolute_);
}

UiNode UiNode::createTable(Bounds const& bounds,
                           std::vector<TableColumn> columns,
                           std::vector<std::vector<std::string>> rows) {
  TableSpec spec;
  spec.size = size_from_bounds(bounds);
  spec.columns = std::move(columns);
  spec.rows = std::move(rows);
  return createTable(spec);
}

UiNode UiNode::createSectionHeader(SectionHeaderSpec const& spec) {
  Bounds bounds = resolveLayoutBounds(spec.bounds, spec.size);
  if (bounds.height <= 0.0f &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchY <= 0.0f) {
    bounds.height = UiDefaults::SectionHeaderHeight;
  }
  if (bounds.width <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f &&
      !spec.title.empty()) {
    float textWidth = estimate_text_width(frame(), textToken(spec.textRole), spec.title);
    bounds.width = std::max(bounds.width, spec.textInsetX + textWidth);
  }
  PanelSpec panel;
  panel.bounds = bounds;
  panel.size = spec.size;
  panel.rectStyle = rectToken(spec.backgroundRole);
  UiNode header = createPanel(panel);
  UiNode headerInternal(frame(), header.nodeId(), true);

  if (spec.accentWidth > 0.0f) {
    PanelSpec accentPanel;
    accentPanel.rectStyle = rectToken(spec.accentRole);
    accentPanel.bounds = Bounds{0.0f, 0.0f, spec.accentWidth, bounds.height};
    headerInternal.createPanel(accentPanel);
  }

  if (!spec.title.empty()) {
    float lineHeight = resolve_line_height(frame(), textToken(spec.textRole));
    float textY = (bounds.height - lineHeight) * 0.5f + spec.textOffsetY;
    LabelSpec titleLabel;
    titleLabel.text = spec.title;
    titleLabel.textStyle = textToken(spec.textRole);
    titleLabel.bounds = Bounds{spec.textInsetX,
                               textY,
                               std::max(0.0f, bounds.width - spec.textInsetX),
                               lineHeight};
    headerInternal.createLabel(titleLabel);
  }

  if (spec.addDivider) {
    DividerSpec divider;
    divider.rectStyle = rectToken(spec.dividerRole);
    divider.bounds = Bounds{bounds.x,
                            bounds.y + bounds.height + spec.dividerOffsetY,
                            bounds.width,
                            1.0f};
    UiNode parentInternal(frame(), id_, true);
    parentInternal.createDivider(divider);
  }

  return UiNode(frame(), header.nodeId(), allowAbsolute_);
}

UiNode UiNode::createSectionHeader(Bounds const& bounds,
                                   std::string_view title,
                                   TextRole role) {
  SectionHeaderSpec spec;
  spec.size = size_from_bounds(bounds);
  spec.title = std::string(title);
  spec.textRole = role;
  return createSectionHeader(spec);
}

UiNode UiNode::createSectionHeader(Bounds const& bounds,
                                   std::string_view title,
                                   TextRole role,
                                   bool addDivider,
                                   float dividerOffsetY) {
  SectionHeaderSpec spec;
  spec.size = size_from_bounds(bounds);
  spec.title = std::string(title);
  spec.textRole = role;
  spec.addDivider = addDivider;
  spec.dividerOffsetY = dividerOffsetY;
  return createSectionHeader(spec);
}

UiNode UiNode::createSectionHeader(SizeSpec const& size,
                                   std::string_view title,
                                   TextRole role) {
  SectionHeaderSpec spec;
  spec.size = size;
  spec.title = std::string(title);
  spec.textRole = role;
  return createSectionHeader(spec);
}

UiNode UiNode::createSectionHeader(SizeSpec const& size,
                                   std::string_view title,
                                   TextRole role,
                                   bool addDivider,
                                   float dividerOffsetY) {
  SectionHeaderSpec spec;
  spec.size = size;
  spec.title = std::string(title);
  spec.textRole = role;
  spec.addDivider = addDivider;
  spec.dividerOffsetY = dividerOffsetY;
  return createSectionHeader(spec);
}

SectionPanel UiNode::createSectionPanel(SectionPanelSpec const& spec) {
  Bounds bounds = resolveLayoutBounds(spec.bounds, spec.size);
  PanelSpec panel;
  panel.bounds = bounds;
  panel.size = spec.size;
  panel.rectStyle = rectToken(spec.panelRole);
  UiNode panelNode = createPanel(panel);
  UiNode panelInternal(frame(), panelNode.nodeId(), true);
  SectionPanel out{UiNode(frame(), panelNode.nodeId(), allowAbsolute_),
                   UiNode(frame(), PrimeFrame::NodeId{}, allowAbsolute_),
                   {},
                   {}};

  float headerWidth = std::max(0.0f, bounds.width - spec.headerInsetX - spec.headerInsetRight);
  Bounds headerBounds{spec.headerInsetX, spec.headerInsetY, headerWidth, spec.headerHeight};
  out.headerBounds = headerBounds;

  if (spec.headerHeight > 0.0f) {
    PanelSpec headerPanel;
    headerPanel.rectStyle = rectToken(spec.headerRole);
    headerPanel.bounds = headerBounds;
    panelInternal.createPanel(headerPanel);
  }

  if (spec.showAccent && spec.accentWidth > 0.0f && spec.headerHeight > 0.0f) {
    PanelSpec accentPanel;
    accentPanel.rectStyle = rectToken(spec.accentRole);
    accentPanel.bounds = Bounds{headerBounds.x, headerBounds.y, spec.accentWidth, headerBounds.height};
    panelInternal.createPanel(accentPanel);
  }

  if (!spec.title.empty() && spec.headerHeight > 0.0f) {
    TextLineSpec title;
    title.bounds = Bounds{headerBounds.x + spec.headerPaddingX,
                          headerBounds.y,
                          std::max(0.0f, headerBounds.width - spec.headerPaddingX * 2.0f),
                          headerBounds.height};
    title.text = spec.title;
    title.textStyle = textToken(spec.textRole);
    title.align = PrimeFrame::TextAlign::Start;
    panelInternal.createTextLine(title);
  }

  float contentX = spec.contentInsetX;
  float contentY = headerBounds.y + headerBounds.height + spec.contentInsetY;
  float contentW = std::max(0.0f, bounds.width - spec.contentInsetX - spec.contentInsetRight);
  float contentH = std::max(0.0f,
                            bounds.height - (contentY + spec.contentInsetBottom));
  out.contentBounds = Bounds{contentX, contentY, contentW, contentH};
  PrimeFrame::NodeId contentId = create_node(frame(), panelNode.nodeId(), out.contentBounds,
                                             nullptr,
                                             PrimeFrame::LayoutType::None,
                                             PrimeFrame::Insets{},
                                             0.0f,
                                             false,
                                             true);
  out.content = UiNode(frame(), contentId, allowAbsolute_);

  return out;
}

SectionPanel UiNode::createSectionPanel(Bounds const& bounds, std::string_view title) {
  SectionPanelSpec spec;
  spec.size = size_from_bounds(bounds);
  spec.title = std::string(title);
  return createSectionPanel(spec);
}

SectionPanel UiNode::createSectionPanel(SizeSpec const& size, std::string_view title) {
  SectionPanelSpec spec;
  spec.size = size;
  spec.title = std::string(title);
  return createSectionPanel(spec);
}

UiNode UiNode::createPropertyList(PropertyListSpec const& spec) {
  Bounds bounds = resolveLayoutBounds(spec.bounds, spec.size);
  if (bounds.height <= 0.0f &&
      !spec.rows.empty() &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchY <= 0.0f) {
    bounds.height = static_cast<float>(spec.rows.size()) * spec.rowHeight +
                    static_cast<float>(spec.rows.size() - 1) * spec.rowGap;
  }
  PrimeFrame::NodeId listId = create_node(frame(), id_, bounds,
                                          &spec.size,
                                          PrimeFrame::LayoutType::None,
                                          PrimeFrame::Insets{},
                                          0.0f,
                                          false,
                                          true);
  UiNode listNodeInternal(frame(), listId, true);

  float labelLineHeight = resolve_line_height(frame(), textToken(spec.labelRole));
  float valueLineHeight = resolve_line_height(frame(), textToken(spec.valueRole));

  for (size_t i = 0; i < spec.rows.size(); ++i) {
    PropertyRow const& row = spec.rows[i];
    float rowY = static_cast<float>(i) * (spec.rowHeight + spec.rowGap);
    float labelY = rowY + (spec.rowHeight - labelLineHeight) * 0.5f;
    float valueY = rowY + (spec.rowHeight - valueLineHeight) * 0.5f;

    if (!row.label.empty()) {
      LabelSpec label;
      label.text = row.label;
      label.textStyle = textToken(spec.labelRole);
      label.bounds = Bounds{spec.labelInsetX,
                            labelY,
                            std::max(0.0f, bounds.width - spec.labelInsetX),
                            labelLineHeight};
      listNodeInternal.createLabel(label);
    }

    if (!row.value.empty()) {
      float valueWidth = estimate_text_width(frame(), textToken(spec.valueRole), row.value);
      float valueX = spec.valueInsetX;
      if (spec.valueAlignRight) {
        valueX = bounds.width - spec.valuePaddingRight - valueWidth;
      }
      if (valueX < 0.0f) {
        valueX = 0.0f;
      }
      LabelSpec valueLabel;
      valueLabel.text = row.value;
      valueLabel.textStyle = textToken(spec.valueRole);
      valueLabel.bounds = Bounds{valueX,
                                 valueY,
                                 std::max(0.0f, valueWidth),
                                 valueLineHeight};
      listNodeInternal.createLabel(valueLabel);
    }
  }

  return UiNode(frame(), listId, allowAbsolute_);
}

UiNode UiNode::createPropertyList(Bounds const& bounds, std::vector<PropertyRow> rows) {
  PropertyListSpec spec;
  spec.size = size_from_bounds(bounds);
  spec.rows = std::move(rows);
  return createPropertyList(spec);
}

UiNode UiNode::createPropertyList(SizeSpec const& size, std::vector<PropertyRow> rows) {
  PropertyListSpec spec;
  spec.size = size;
  spec.rows = std::move(rows);
  return createPropertyList(spec);
}

UiNode UiNode::createPropertyRow(Bounds const& bounds,
                                 std::string_view label,
                                 std::string_view value,
                                 TextRole role) {
  PropertyListSpec spec;
  spec.size = size_from_bounds(bounds);
  if (bounds.height > 0.0f) {
    spec.rowHeight = bounds.height;
  }
  spec.rowGap = 0.0f;
  spec.labelRole = role;
  spec.valueRole = role;
  spec.rows = {{std::string(label), std::string(value)}};
  return createPropertyList(spec);
}

UiNode UiNode::createPropertyRow(SizeSpec const& size,
                                 std::string_view label,
                                 std::string_view value,
                                 TextRole role) {
  PropertyListSpec spec;
  spec.size = size;
  spec.labelRole = role;
  spec.valueRole = role;
  spec.rows = {{std::string(label), std::string(value)}};
  return createPropertyList(spec);
}

UiNode UiNode::createProgressBar(ProgressBarSpec const& spec) {
  Bounds bounds = resolveLayoutBounds(spec.bounds, spec.size);
  if ((bounds.width <= 0.0f || bounds.height <= 0.0f) &&
      !spec.size.preferredWidth.has_value() &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchX <= 0.0f &&
      spec.size.stretchY <= 0.0f) {
    if (bounds.width <= 0.0f) {
      bounds.width = UiDefaults::ControlWidthL;
    }
    if (bounds.height <= 0.0f) {
      bounds.height = UiDefaults::OpacityBarHeight;
    }
  }
  PanelSpec panel;
  panel.bounds = bounds;
  panel.size = spec.size;
  panel.rectStyle = rectToken(spec.trackRole);
  UiNode bar = createPanel(panel);
  float clamped = std::clamp(spec.value, 0.0f, 1.0f);
  float fillWidth = bounds.width * clamped;
  if (spec.minFillWidth > 0.0f && clamped > 0.0f) {
    fillWidth = std::max(fillWidth, spec.minFillWidth);
  }
  if (fillWidth > 0.0f) {
    PanelSpec fillPanel;
    fillPanel.rectStyle = rectToken(spec.fillRole);
    fillPanel.bounds = Bounds{0.0f, 0.0f, fillWidth, bounds.height};
    UiNode barInternal(frame(), bar.nodeId(), true);
    barInternal.createPanel(fillPanel);
  }
  return bar;
}

UiNode UiNode::createProgressBar(Bounds const& bounds, float value) {
  return createProgressBar(size_from_bounds(bounds), value);
}

UiNode UiNode::createProgressBar(SizeSpec const& size, float value) {
  ProgressBarSpec spec;
  spec.size = size;
  spec.value = value;
  return createProgressBar(spec);
}

UiNode UiNode::createCardGrid(CardGridSpec const& spec) {
  Bounds bounds = resolveLayoutBounds(spec.bounds, spec.size);
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
  PrimeFrame::NodeId gridId = create_node(frame(), id_, bounds,
                                          &spec.size,
                                          PrimeFrame::LayoutType::None,
                                          PrimeFrame::Insets{},
                                          0.0f,
                                          true,
                                          true);
  UiNode gridNodeInternal(frame(), gridId, true);

  if (spec.cards.empty()) {
    return UiNode(frame(), gridId, allowAbsolute_);
  }

  size_t columns = 1;
  if (spec.cardWidth + spec.gapX > 0.0f && bounds.width > 0.0f) {
    columns = std::max<size_t>(1, static_cast<size_t>((bounds.width + spec.gapX) /
                                                      (spec.cardWidth + spec.gapX)));
  }

  float titleLine = resolve_line_height(frame(), textToken(spec.titleRole));
  float subtitleLine = resolve_line_height(frame(), textToken(spec.subtitleRole));

  for (size_t i = 0; i < spec.cards.size(); ++i) {
    size_t row = i / columns;
    size_t col = i % columns;
    float cardX = static_cast<float>(col) * (spec.cardWidth + spec.gapX);
    float cardY = static_cast<float>(row) * (spec.cardHeight + spec.gapY);
    PanelSpec cardPanel;
    cardPanel.rectStyle = rectToken(spec.cardRole);
    cardPanel.bounds = Bounds{cardX, cardY, spec.cardWidth, spec.cardHeight};
    UiNode cardNode = gridNodeInternal.createPanel(cardPanel);
    UiNode cardInternal(frame(), cardNode.nodeId(), true);

    CardSpec const& card = spec.cards[i];
    if (!card.title.empty()) {
      LabelSpec titleLabel;
      titleLabel.text = card.title;
      titleLabel.textStyle = textToken(spec.titleRole);
      titleLabel.bounds = Bounds{spec.paddingX,
                                 spec.titleOffsetY,
                                 std::max(0.0f, spec.cardWidth - spec.paddingX * 2.0f),
                                 titleLine};
      cardInternal.createLabel(titleLabel);
    }
    if (!card.subtitle.empty()) {
      LabelSpec subtitleLabel;
      subtitleLabel.text = card.subtitle;
      subtitleLabel.textStyle = textToken(spec.subtitleRole);
      subtitleLabel.bounds = Bounds{spec.paddingX,
                                    spec.subtitleOffsetY,
                                    std::max(0.0f, spec.cardWidth - spec.paddingX * 2.0f),
                                    subtitleLine};
      cardInternal.createLabel(subtitleLabel);
    }
  }

  return UiNode(frame(), gridId, allowAbsolute_);
}

UiNode UiNode::createCardGrid(Bounds const& bounds, std::vector<CardSpec> cards) {
  CardGridSpec spec;
  spec.size = size_from_bounds(bounds);
  spec.cards = std::move(cards);
  return createCardGrid(spec);
}

UiNode UiNode::createButton(ButtonSpec const& spec) {
  Bounds bounds = resolveLayoutBounds(spec.bounds, spec.size);
  if (bounds.height <= 0.0f &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchY <= 0.0f) {
    bounds.height = UiDefaults::ControlHeight;
  }
  if (bounds.width <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f &&
      !spec.label.empty()) {
    float textWidth = estimate_text_width(frame(), spec.textStyle, spec.label);
    bounds.width = std::max(bounds.width, textWidth + spec.textInsetX * 2.0f);
  }
  PanelSpec panel;
  panel.bounds = bounds;
  panel.size = spec.size;
  panel.rectStyle = spec.backgroundStyle;
  panel.rectStyleOverride = spec.backgroundStyleOverride;
  UiNode button = createPanel(panel);
  if (spec.label.empty()) {
    return UiNode(frame(), button.nodeId(), allowAbsolute_);
  }
  UiNode buttonInternal(frame(), button.nodeId(), true);
  float lineHeight = resolve_line_height(frame(), spec.textStyle);
  float textWidth = estimate_text_width(frame(), spec.textStyle, spec.label);
  float textY = (bounds.height - lineHeight) * 0.5f + spec.textOffsetY;

  LabelSpec label;
  label.text = spec.label;
  label.textStyle = spec.textStyle;
  label.textStyleOverride = spec.textStyleOverride;
  label.bounds.height = lineHeight;
  label.bounds.y = textY;

  if (spec.centerText) {
    float textX = (bounds.width - textWidth) * 0.5f;
    if (textX < 0.0f) {
      textX = 0.0f;
    }
    label.align = PrimeFrame::TextAlign::Start;
    label.bounds.x = textX;
    label.bounds.width = std::max(0.0f, textWidth);
    label.maxWidth = label.bounds.width;
  } else {
    label.bounds.x = spec.textInsetX;
    label.bounds.width = std::max(0.0f, bounds.width - spec.textInsetX);
    label.maxWidth = label.bounds.width;
  }

  if (!spec.centerText && textWidth > 0.0f) {
    label.bounds.width = std::max(label.bounds.width, textWidth);
    label.maxWidth = label.bounds.width;
  }

  buttonInternal.createLabel(label);
  return UiNode(frame(), button.nodeId(), allowAbsolute_);
}

UiNode UiNode::createButton(Bounds const& bounds,
                            std::string_view label,
                            ButtonVariant variant) {
  return createButton(label, variant, size_from_bounds(bounds));
}

UiNode UiNode::createButton(std::string_view label,
                            ButtonVariant variant,
                            SizeSpec const& size) {
  ButtonSpec spec;
  spec.label = std::string(label);
  spec.size = size;
  if (variant == ButtonVariant::Primary) {
    spec.backgroundStyle = rectToken(RectRole::Accent);
    spec.textStyle = textToken(TextRole::BodyBright);
  } else {
    spec.backgroundStyle = rectToken(RectRole::Panel);
    spec.textStyle = textToken(TextRole::BodyBright);
  }
  return createButton(spec);
}

UiNode UiNode::createTextField(TextFieldSpec const& spec) {
  Bounds bounds = resolveLayoutBounds(spec.bounds, spec.size);
  if (bounds.height <= 0.0f &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchY <= 0.0f) {
    bounds.height = UiDefaults::ControlHeight;
  }
  if (bounds.width <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f) {
    std::string_view preview = spec.text;
    PrimeFrame::TextStyleToken previewStyle = spec.textStyle;
    if (preview.empty() && spec.showPlaceholderWhenEmpty) {
      preview = spec.placeholder;
      previewStyle = spec.placeholderStyle;
    }
    if (!preview.empty()) {
      float previewWidth = estimate_text_width(frame(), previewStyle, preview);
      bounds.width = std::max(bounds.width, previewWidth + spec.paddingX * 2.0f);
    }
  }
  PanelSpec panel;
  panel.bounds = bounds;
  panel.size = spec.size;
  panel.rectStyle = spec.backgroundStyle;
  panel.rectStyleOverride = spec.backgroundStyleOverride;
  UiNode field = createPanel(panel);

  std::string_view content = spec.text;
  PrimeFrame::TextStyleToken style = spec.textStyle;
  PrimeFrame::TextStyleOverride overrideStyle = spec.textStyleOverride;
  if (content.empty() && spec.showPlaceholderWhenEmpty) {
    content = spec.placeholder;
    style = spec.placeholderStyle;
    overrideStyle = spec.placeholderStyleOverride;
  }
  if (content.empty()) {
    return UiNode(frame(), field.nodeId(), allowAbsolute_);
  }

  float lineHeight = resolve_line_height(frame(), style);
  float textY = (bounds.height - lineHeight) * 0.5f + spec.textOffsetY;
  float textWidth = std::max(0.0f, bounds.width - spec.paddingX * 2.0f);

  LabelSpec label;
  label.text = std::string(content);
  label.textStyle = style;
  label.textStyleOverride = overrideStyle;
  label.bounds = Bounds{spec.paddingX, textY, textWidth, lineHeight};
  UiNode fieldInternal(frame(), field.nodeId(), true);
  fieldInternal.createLabel(label);

  return UiNode(frame(), field.nodeId(), allowAbsolute_);
}

UiNode UiNode::createTextField(Bounds const& bounds, std::string_view placeholder) {
  return createTextField(placeholder, size_from_bounds(bounds));
}

UiNode UiNode::createTextField(std::string_view placeholder, SizeSpec const& size) {
  TextFieldSpec spec;
  spec.placeholder = std::string(placeholder);
  spec.size = size;
  spec.backgroundStyle = rectToken(RectRole::Panel);
  spec.textStyle = textToken(TextRole::BodyBright);
  spec.placeholderStyle = textToken(TextRole::BodyMuted);
  return createTextField(spec);
}

UiNode UiNode::createStatusBar(StatusBarSpec const& spec) {
  Bounds bounds = resolveLayoutBounds(spec.bounds, spec.size);
  if (bounds.height <= 0.0f &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchY <= 0.0f) {
    bounds.height = UiDefaults::StatusHeight;
  }
  if (bounds.width <= 0.0f || bounds.height <= 0.0f) {
    return UiNode(frame(), id_, allowAbsolute_);
  }

  PanelSpec panelSpec;
  panelSpec.rectStyle = rectToken(spec.backgroundRole);
  panelSpec.bounds = bounds;
  panelSpec.size = spec.size;
  UiNode bar = createPanel(panelSpec);
  UiNode barInternal(frame(), bar.nodeId(), true);

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
    LabelSpec leftLabel;
    leftLabel.text = spec.leftText;
    leftLabel.textStyle = textToken(spec.leftRole);
    leftLabel.bounds = Bounds{spec.paddingX, leftY, leftMaxWidth, leftLineHeight};
    barInternal.createLabel(leftLabel);
  }

  if (!spec.rightText.empty()) {
    LabelSpec rightLabel;
    rightLabel.text = spec.rightText;
    rightLabel.textStyle = textToken(spec.rightRole);
    rightLabel.bounds = Bounds{rightX, rightY, rightWidth, rightLineHeight};
    barInternal.createLabel(rightLabel);
  }

  return UiNode(frame(), bar.nodeId(), allowAbsolute_);
}

UiNode UiNode::createStatusBar(Bounds const& bounds,
                               std::string_view leftText,
                               std::string_view rightText) {
  StatusBarSpec spec;
  spec.size = size_from_bounds(bounds);
  spec.leftText = std::string(leftText);
  spec.rightText = std::string(rightText);
  return createStatusBar(spec);
}

UiNode UiNode::createStatusBar(SizeSpec const& size,
                               std::string_view leftText,
                               std::string_view rightText) {
  StatusBarSpec spec;
  spec.size = size;
  spec.leftText = std::string(leftText);
  spec.rightText = std::string(rightText);
  return createStatusBar(spec);
}

UiNode UiNode::createScrollView(ScrollViewSpec const& spec) {
  Bounds bounds = resolveLayoutBounds(spec.bounds, spec.size);
  if ((bounds.width <= 0.0f || bounds.height <= 0.0f) &&
      !spec.size.preferredWidth.has_value() &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchX <= 0.0f &&
      spec.size.stretchY <= 0.0f) {
    if (bounds.width <= 0.0f) {
      bounds.width = UiDefaults::FieldWidthL;
    }
    if (bounds.height <= 0.0f) {
      bounds.height = UiDefaults::PanelHeightM;
    }
  }
  if (bounds.width <= 0.0f || bounds.height <= 0.0f) {
    return UiNode(frame(), id_, allowAbsolute_);
  }

  PrimeFrame::NodeId scrollId = create_node(frame(), id_, bounds,
                                            &spec.size,
                                            PrimeFrame::LayoutType::None,
                                            PrimeFrame::Insets{},
                                            0.0f,
                                            spec.clipChildren,
                                            true);
  UiNode scrollNodeInternal(frame(), scrollId, true);

  if (spec.showVertical && spec.vertical.enabled) {
    float trackW = spec.vertical.thickness;
    float trackH = std::max(0.0f, bounds.height - spec.vertical.startPadding - spec.vertical.endPadding);
    float trackX = bounds.width - spec.vertical.inset;
    float trackY = spec.vertical.startPadding;
    PanelSpec trackPanel;
    trackPanel.rectStyle = spec.vertical.trackStyle;
    trackPanel.bounds = Bounds{trackX, trackY, trackW, trackH};
    scrollNodeInternal.createPanel(trackPanel);

    float thumbH = std::min(trackH, spec.vertical.thumbLength);
    float maxOffset = std::max(0.0f, trackH - thumbH);
    float thumbOffset = std::clamp(spec.vertical.thumbOffset, 0.0f, maxOffset);
    float thumbY = trackY + thumbOffset;
    PanelSpec thumbPanel;
    thumbPanel.rectStyle = spec.vertical.thumbStyle;
    thumbPanel.bounds = Bounds{trackX, thumbY, trackW, thumbH};
    scrollNodeInternal.createPanel(thumbPanel);
  }

  if (spec.showHorizontal && spec.horizontal.enabled) {
    float trackH = spec.horizontal.thickness;
    float trackW = std::max(0.0f, bounds.width - spec.horizontal.startPadding - spec.horizontal.endPadding);
    float trackX = spec.horizontal.startPadding;
    float trackY = bounds.height - spec.horizontal.inset;
    PanelSpec trackPanel;
    trackPanel.rectStyle = spec.horizontal.trackStyle;
    trackPanel.bounds = Bounds{trackX, trackY, trackW, trackH};
    scrollNodeInternal.createPanel(trackPanel);

    float thumbW = std::min(trackW, spec.horizontal.thumbLength);
    float maxOffset = std::max(0.0f, trackW - thumbW);
    float thumbOffset = std::clamp(spec.horizontal.thumbOffset, 0.0f, maxOffset);
    float thumbX = trackX + thumbOffset;
    PanelSpec thumbPanel;
    thumbPanel.rectStyle = spec.horizontal.thumbStyle;
    thumbPanel.bounds = Bounds{thumbX, trackY, thumbW, trackH};
    scrollNodeInternal.createPanel(thumbPanel);
  }

  return UiNode(frame(), scrollId, allowAbsolute_);
}

UiNode UiNode::createScrollHints(ScrollHintsSpec const& spec) {
  ScrollViewSpec view;
  view.bounds = spec.bounds;
  view.size = spec.size;
  if (!view.size.preferredWidth.has_value() &&
      !view.size.preferredHeight.has_value() &&
      view.size.stretchX <= 0.0f &&
      view.size.stretchY <= 0.0f) {
    view.size.preferredWidth = UiDefaults::FieldWidthL;
    view.size.preferredHeight = UiDefaults::PanelHeightM;
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
  return createScrollView(view);
}

UiNode UiNode::createScrollHints(Bounds const& bounds) {
  ScrollHintsSpec spec;
  spec.size = size_from_bounds(bounds);
  return createScrollHints(spec);
}

UiNode UiNode::createTreeView(TreeViewSpec const& spec) {
  std::vector<FlatTreeRow> rows;
  std::vector<int> depthStack;
  flatten_tree(spec.nodes, 0, depthStack, rows);

  Bounds bounds = resolveLayoutBounds(spec.bounds, spec.size);
  if (bounds.width <= 0.0f || bounds.height <= 0.0f) {
    float maxLabelWidth = 0.0f;
    for (FlatTreeRow const& row : rows) {
      TextRole role = row.selected ? spec.selectedTextRole : spec.textRole;
      float textWidth = estimate_text_width(frame(), textToken(role), row.label);
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
      float rowsHeight = rows.empty()
                             ? spec.rowHeight
                             : static_cast<float>(rows.size()) * spec.rowHeight +
                                   static_cast<float>(rows.size() - 1) * spec.rowGap;
      bounds.height = spec.rowStartY + rowsHeight;
    }
  }

  if (bounds.width <= 0.0f || bounds.height <= 0.0f) {
    return UiNode(frame(), id_, allowAbsolute_);
  }

  PrimeFrame::NodeId treeId = create_node(frame(), id_, bounds,
                                          &spec.size,
                                          PrimeFrame::LayoutType::None,
                                          PrimeFrame::Insets{},
                                          0.0f,
                                          spec.clipChildren,
                                          true);
  UiNode treeNodeInternal(frame(), treeId, true);

  float rowWidth = bounds.width - spec.rowWidthInset;
  float rowTextHeight = resolve_line_height(frame(), textToken(spec.textRole));
  float selectedTextHeight = resolve_line_height(frame(), textToken(spec.selectedTextRole));

  for (size_t i = 0; i < rows.size(); ++i) {
    FlatTreeRow const& row = rows[i];
    float rowY = spec.rowStartY + static_cast<float>(i) * (spec.rowHeight + spec.rowGap);
    RectRole rowRole = row.selected ? spec.selectionRole
                                    : (i % 2 == 0 ? spec.rowAltRole : spec.rowRole);

    PanelSpec rowPanel;
    rowPanel.rectStyle = rectToken(rowRole);
    rowPanel.bounds = Bounds{spec.rowStartX, rowY, rowWidth, spec.rowHeight};
    treeNodeInternal.createPanel(rowPanel);

    if (row.selected && spec.selectionAccentWidth > 0.0f) {
      PanelSpec accentPanel;
      accentPanel.rectStyle = rectToken(spec.selectionAccentRole);
      accentPanel.bounds = Bounds{spec.rowStartX, rowY, spec.selectionAccentWidth, spec.rowHeight};
      treeNodeInternal.createPanel(accentPanel);
    }
  }

  if (spec.showHeaderDivider) {
    float dividerY = spec.headerDividerY;
    add_divider_rect(frame(), treeId,
                     Bounds{spec.rowStartX, dividerY, rowWidth, spec.connectorThickness},
                     rectToken(spec.connectorRole));
  }

  if (spec.showConnectors) {
    for (size_t i = 0; i < rows.size(); ++i) {
      FlatTreeRow const& row = rows[i];
      if (row.depth <= 0 || row.parentIndex < 0) {
        continue;
      }
      float rowY = spec.rowStartY + static_cast<float>(i) * (spec.rowHeight + spec.rowGap);
      float glyphCenterY = rowY + spec.rowHeight * 0.5f;
      float glyphX = spec.caretBaseX + static_cast<float>(row.depth) * spec.indent;

      int parentIndex = row.parentIndex;
      float parentY = spec.rowStartY + static_cast<float>(parentIndex) * (spec.rowHeight + spec.rowGap);
      float parentCenterY = parentY + spec.rowHeight * 0.5f;
      float trunkX = spec.caretBaseX + static_cast<float>(row.depth - 1) * spec.indent + spec.caretSize * 0.5f;
      float linkEndX = glyphX + spec.caretSize * 0.5f;
      if (row.hasChildren) {
        linkEndX = glyphX - spec.linkEndInset;
      }
      float y0 = std::min(parentCenterY, glyphCenterY);
      float y1 = std::max(parentCenterY, glyphCenterY);
      float lineH = std::max(spec.connectorThickness, y1 - y0);
      add_divider_rect(frame(), treeId,
                       Bounds{trunkX, y0, spec.connectorThickness, lineH},
                       rectToken(spec.connectorRole));
      float linkW = linkEndX - trunkX;
      if (linkW > 0.5f) {
        add_divider_rect(frame(), treeId,
                         Bounds{trunkX, glyphCenterY, linkW, spec.connectorThickness},
                         rectToken(spec.connectorRole));
      }
    }
  }

  for (size_t i = 0; i < rows.size(); ++i) {
    FlatTreeRow const& row = rows[i];
    float rowY = spec.rowStartY + static_cast<float>(i) * (spec.rowHeight + spec.rowGap);
    float glyphX = spec.caretBaseX + static_cast<float>(row.depth) * spec.indent;
    float glyphY = rowY + (spec.rowHeight - spec.caretSize) * 0.5f;
    RectRole rowRole = row.selected ? spec.selectionRole
                                    : (i % 2 == 0 ? spec.rowAltRole : spec.rowRole);

    if (spec.showCaretMasks && row.depth > 0) {
      float maskPad = spec.caretMaskPad;
      PanelSpec maskPanel;
      maskPanel.rectStyle = rectToken(rowRole);
      maskPanel.bounds = Bounds{glyphX - maskPad,
                                glyphY - maskPad,
                                spec.caretSize + maskPad * 2.0f,
                                spec.caretSize + maskPad * 2.0f};
      treeNodeInternal.createPanel(maskPanel);
    }

    if (row.hasChildren) {
      PanelSpec caretPanel;
      caretPanel.rectStyle = rectToken(spec.caretBackgroundRole);
      caretPanel.bounds = Bounds{glyphX, glyphY, spec.caretSize, spec.caretSize};
      treeNodeInternal.createPanel(caretPanel);

      PanelSpec caretLine;
      caretLine.rectStyle = rectToken(spec.caretLineRole);
      caretLine.bounds = Bounds{glyphX + spec.caretInset,
                                glyphY + spec.caretSize * 0.5f - spec.caretThickness * 0.5f,
                                spec.caretSize - spec.caretInset * 2.0f,
                                spec.caretThickness};
      treeNodeInternal.createPanel(caretLine);
      if (!row.expanded) {
        PanelSpec caretVertical;
        caretVertical.rectStyle = rectToken(spec.caretLineRole);
        caretVertical.bounds = Bounds{glyphX + spec.caretSize * 0.5f - spec.caretThickness * 0.5f,
                                      glyphY + spec.caretInset,
                                      spec.caretThickness,
                                      spec.caretSize - spec.caretInset * 2.0f};
        treeNodeInternal.createPanel(caretVertical);
      }
    } else {
      PanelSpec caretDot;
      caretDot.rectStyle = rectToken(spec.caretLineRole);
      caretDot.bounds = Bounds{glyphX + spec.caretSize * 0.5f - 1.0f,
                               glyphY + spec.caretSize * 0.5f - 1.0f,
                               2.0f,
                               2.0f};
      treeNodeInternal.createPanel(caretDot);
    }
  }

  for (size_t i = 0; i < rows.size(); ++i) {
    FlatTreeRow const& row = rows[i];
    float rowY = spec.rowStartY + static_cast<float>(i) * (spec.rowHeight + spec.rowGap);
    float indent = (row.depth > 0) ? spec.indent * static_cast<float>(row.depth) : 0.0f;
    float textX = spec.rowStartX + 20.0f + indent;
    TextRole textRole = row.selected ? spec.selectedTextRole : spec.textRole;
    float lineHeight = row.selected ? selectedTextHeight : rowTextHeight;
    float textY = rowY + (spec.rowHeight - lineHeight) * 0.5f;
    LabelSpec rowLabel;
    rowLabel.text = row.label;
    rowLabel.textStyle = textToken(textRole);
    rowLabel.bounds = Bounds{textX, textY, rowWidth - (textX - spec.rowStartX), lineHeight};
    treeNodeInternal.createLabel(rowLabel);
  }

  if (spec.showScrollBar && spec.scrollBar.enabled) {
    float trackX = bounds.width - spec.scrollBar.inset;
    float trackY = spec.scrollBar.padding;
    float trackH = std::max(0.0f, bounds.height - spec.scrollBar.padding * 2.0f);
    float trackW = spec.scrollBar.width;
    PanelSpec trackPanel;
    trackPanel.rectStyle = spec.scrollBar.trackStyle;
    trackPanel.bounds = Bounds{trackX, trackY, trackW, trackH};
    treeNodeInternal.createPanel(trackPanel);

    float thumbH = trackH * spec.scrollBar.thumbFraction;
    thumbH = std::max(thumbH, spec.scrollBar.minThumbHeight);
    if (thumbH > trackH) {
      thumbH = trackH;
    }
    float maxOffset = std::max(0.0f, trackH - thumbH);
    float progress = std::clamp(spec.scrollBar.thumbProgress, 0.0f, 1.0f);
    float thumbY = trackY + maxOffset * progress;
    PanelSpec thumbPanel;
    thumbPanel.rectStyle = spec.scrollBar.thumbStyle;
    thumbPanel.bounds = Bounds{trackX, thumbY, trackW, thumbH};
    treeNodeInternal.createPanel(thumbPanel);
  }

  return UiNode(frame(), treeId, allowAbsolute_);
}

UiNode createRoot(PrimeFrame::Frame& frame, Bounds const& bounds) {
  return createRoot(frame, size_from_bounds(bounds));
}

UiNode createRoot(PrimeFrame::Frame& frame, SizeSpec const& size) {
  PrimeFrame::Theme* theme = frame.getTheme(PrimeFrame::DefaultThemeId);
  if (theme && (theme->palette.empty() || theme->rectStyles.empty() || theme->textStyles.empty() ||
                is_default_theme(*theme))) {
    applyStudioTheme(frame);
  }
  Bounds rootBounds = resolve_bounds(Bounds{}, size);
  rootBounds.x = 0.0f;
  rootBounds.y = 0.0f;
  PrimeFrame::NodeId id = create_node(frame, PrimeFrame::NodeId{}, rootBounds,
                                      nullptr,
                                      PrimeFrame::LayoutType::None,
                                      PrimeFrame::Insets{},
                                      0.0f,
                                      false,
                                      true);
  return UiNode(frame, id);
}

ShellLayout createShell(PrimeFrame::Frame& frame, ShellSpec const& spec) {
  Bounds shellBounds = resolve_bounds(spec.bounds, spec.size);
  shellBounds.x = 0.0f;
  shellBounds.y = 0.0f;
  float width = shellBounds.width;
  float height = shellBounds.height;
  float contentH = std::max(0.0f, height - spec.topbarHeight - spec.statusHeight);
  float contentW = std::max(0.0f, width - spec.sidebarWidth - spec.inspectorWidth);

  Bounds topbarBounds{0.0f, 0.0f, width, spec.topbarHeight};
  Bounds statusBounds{0.0f, height - spec.statusHeight, width, spec.statusHeight};
  Bounds sidebarBounds{0.0f, spec.topbarHeight, spec.sidebarWidth, contentH};
  Bounds contentBounds{spec.sidebarWidth, spec.topbarHeight, contentW, contentH};
  Bounds inspectorBounds{width - spec.inspectorWidth, spec.topbarHeight, spec.inspectorWidth, contentH};

  UiNode root = createRoot(frame, shellBounds);
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
    UiNode rootInternal(frame, root.nodeId(), true);
    DividerSpec divider;
    divider.rectStyle = rectToken(spec.dividerRole);
    divider.bounds = Bounds{0.0f, spec.topbarHeight, width, 1.0f};
    rootInternal.createDivider(divider);
    divider.bounds = Bounds{spec.sidebarWidth - 1.0f, spec.topbarHeight, 1.0f, contentH};
    rootInternal.createDivider(divider);
    divider.bounds = Bounds{width - spec.inspectorWidth, spec.topbarHeight, 1.0f, contentH};
    rootInternal.createDivider(divider);
    divider.bounds = Bounds{0.0f, height - spec.statusHeight, width, 1.0f};
    rootInternal.createDivider(divider);
  }

  return ShellLayout{
      root,
      background,
      topbar,
      status,
      sidebar,
      content,
      inspector,
      shellBounds,
      topbarBounds,
      statusBounds,
      sidebarBounds,
      contentBounds,
      inspectorBounds
  };
}

ShellSpec makeShellSpec(Bounds const& bounds) {
  ShellSpec spec;
  spec.size = size_from_bounds(bounds);
  return spec;
}

ShellSpec makeShellSpec(SizeSpec const& size) {
  ShellSpec spec;
  spec.size = size;
  return spec;
}

Version getVersion() {
  return Version{};
}

std::string_view getVersionString() {
  return "0.1.0";
}

} // namespace PrimeStage
