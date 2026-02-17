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

UiNode::UiNode(PrimeFrame::Frame& frame, PrimeFrame::NodeId id)
    : frame_(frame), id_(id) {}

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
  return UiNode(frame(), nodeId);
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
  return UiNode(frame(), nodeId);
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
  return UiNode(frame(), nodeId);
}

UiNode UiNode::createPanel(PanelSpec const& spec) {
  PrimeFrame::NodeId nodeId = create_node(frame(), id_, spec.bounds,
                                          &spec.size,
                                          spec.layout,
                                          spec.padding,
                                          spec.gap,
                                          spec.clipChildren,
                                          spec.visible);
  add_rect_primitive(frame(), nodeId, spec.rectStyle, spec.rectStyleOverride);
  return UiNode(frame(), nodeId);
}

UiNode UiNode::createPanel(PrimeFrame::RectStyleToken rectStyle, Bounds const& bounds) {
  PanelSpec spec;
  spec.bounds = bounds;
  spec.rectStyle = rectStyle;
  return createPanel(spec);
}

UiNode UiNode::createPanel(PrimeFrame::RectStyleToken rectStyle, SizeSpec const& size) {
  PanelSpec spec;
  spec.rectStyle = rectStyle;
  spec.size = size;
  return createPanel(spec);
}

UiNode UiNode::createPanel(RectRole role, Bounds const& bounds) {
  return createPanel(rectToken(role), bounds);
}

UiNode UiNode::createPanel(RectRole role, SizeSpec const& size) {
  return createPanel(rectToken(role), size);
}

UiNode UiNode::createLabel(LabelSpec const& spec) {
  PrimeFrame::NodeId nodeId = create_node(frame(), id_, spec.bounds,
                                          &spec.size,
                                          PrimeFrame::LayoutType::None,
                                          PrimeFrame::Insets{},
                                          0.0f,
                                          false,
                                          spec.visible);
  add_text_primitive(frame(), nodeId, spec);
  return UiNode(frame(), nodeId);
}

UiNode UiNode::createLabel(std::string_view text,
                           PrimeFrame::TextStyleToken textStyle,
                           Bounds const& bounds) {
  LabelSpec spec;
  spec.bounds = bounds;
  spec.text = std::string(text);
  spec.textStyle = textStyle;
  return createLabel(spec);
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
  return createLabel(text, textToken(role), bounds);
}

UiNode UiNode::createLabel(std::string_view text, TextRole role, SizeSpec const& size) {
  return createLabel(text, textToken(role), size);
}

UiNode UiNode::createParagraph(ParagraphSpec const& spec) {
  Bounds bounds = resolve_bounds(spec.bounds, spec.size);
  PrimeFrame::TextStyleToken token = spec.textStyle;
  float maxWidth = spec.maxWidth > 0.0f ? spec.maxWidth : bounds.width;
  std::vector<std::string> lines = wrap_text_lines(frame(), token, spec.text, maxWidth, spec.wrap);

  float lineHeight = resolve_line_height(frame(), token);
  if (spec.autoHeight && bounds.height <= 0.0f) {
    bounds.height = std::max(0.0f, lineHeight * static_cast<float>(lines.size()));
  }

  PrimeFrame::NodeId paragraphId = create_node(frame(), id_, bounds,
                                               &spec.size,
                                               PrimeFrame::LayoutType::None,
                                               PrimeFrame::Insets{},
                                               0.0f,
                                               false,
                                               spec.visible);
  UiNode paragraph(frame(), paragraphId);

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
    paragraph.createLabel(label);
  }

  return paragraph;
}

UiNode UiNode::createParagraph(Bounds const& bounds,
                               std::string_view text,
                               PrimeFrame::TextStyleToken textStyle) {
  ParagraphSpec spec;
  spec.bounds = bounds;
  spec.text = std::string(text);
  spec.textStyle = textStyle;
  return createParagraph(spec);
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
  return createParagraph(bounds, text, textToken(role));
}

UiNode UiNode::createParagraph(std::string_view text,
                               TextRole role,
                               SizeSpec const& size) {
  return createParagraph(text, textToken(role), size);
}

UiNode UiNode::createTextLine(TextLineSpec const& spec) {
  PrimeFrame::TextStyleToken token = spec.textStyle;
  float lineHeight = resolve_line_height(frame(), token);
  Bounds bounds = resolve_bounds(spec.bounds, spec.size);
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

  return createLabel(label);
}

UiNode UiNode::createTextLine(Bounds const& bounds,
                              std::string_view text,
                              PrimeFrame::TextStyleToken textStyle,
                              PrimeFrame::TextAlign align) {
  TextLineSpec spec;
  spec.bounds = bounds;
  spec.text = std::string(text);
  spec.textStyle = textStyle;
  spec.align = align;
  return createTextLine(spec);
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
  return createTextLine(bounds, text, textToken(role), align);
}

UiNode UiNode::createTextLine(std::string_view text,
                              TextRole role,
                              SizeSpec const& size,
                              PrimeFrame::TextAlign align) {
  return createTextLine(text, textToken(role), size, align);
}

UiNode UiNode::createDivider(DividerSpec const& spec) {
  PrimeFrame::NodeId nodeId = create_node(frame(), id_, spec.bounds,
                                          &spec.size,
                                          PrimeFrame::LayoutType::None,
                                          PrimeFrame::Insets{},
                                          0.0f,
                                          false,
                                          spec.visible);
  add_rect_primitive(frame(), nodeId, spec.rectStyle, spec.rectStyleOverride);
  return UiNode(frame(), nodeId);
}

UiNode UiNode::createDivider(PrimeFrame::RectStyleToken rectStyle, SizeSpec const& size) {
  DividerSpec spec;
  spec.rectStyle = rectStyle;
  spec.size = size;
  return createDivider(spec);
}

UiNode UiNode::createSpacer(SpacerSpec const& spec) {
  PrimeFrame::NodeId nodeId = create_node(frame(), id_, spec.bounds,
                                          &spec.size,
                                          PrimeFrame::LayoutType::None,
                                          PrimeFrame::Insets{},
                                          0.0f,
                                          false,
                                          spec.visible);
  return UiNode(frame(), nodeId);
}

UiNode UiNode::createSpacer(SizeSpec const& size) {
  SpacerSpec spec;
  spec.size = size;
  return createSpacer(spec);
}
UiNode UiNode::createTable(TableSpec const& spec) {
  Bounds tableBounds = resolve_bounds(spec.bounds, spec.size);
  size_t rowCount = spec.rows.size();
  float rowsHeight = 0.0f;
  if (rowCount > 0) {
    rowsHeight = static_cast<float>(rowCount) * spec.rowHeight +
                 static_cast<float>(rowCount - 1) * spec.rowGap;
  }
  float headerBlock = spec.headerHeight > 0.0f ? spec.headerInset + spec.headerHeight : 0.0f;
  if (tableBounds.height <= 0.0f) {
    tableBounds.height = headerBlock + rowsHeight;
  }

  PrimeFrame::NodeId tableId = create_node(frame(), id_, tableBounds,
                                           &spec.size,
                                           PrimeFrame::LayoutType::None,
                                           PrimeFrame::Insets{},
                                           0.0f,
                                           spec.clipChildren,
                                           true);
  UiNode tableNode(frame(), tableId);

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
    tableNode.createPanel(spec.headerRole, Bounds{0.0f, headerY, tableWidth, spec.headerHeight});
  }

  if (spec.showHeaderDividers) {
    DividerSpec divider;
    divider.rectStyle = rectToken(spec.dividerRole);
    divider.bounds = Bounds{0.0f, 0.0f, tableWidth, 1.0f};
    tableNode.createDivider(divider);
    divider.bounds = Bounds{0.0f, headerY + spec.headerHeight, tableWidth, 1.0f};
    tableNode.createDivider(divider);
  }

  float cursorX = 0.0f;
  if (spec.headerHeight > 0.0f) {
    for (size_t i = 0; i < spec.columns.size(); ++i) {
      TableColumn const& col = spec.columns[i];
      float colWidth = i < columnWidths.size() ? columnWidths[i] : 0.0f;
      float textWidth = std::max(0.0f, colWidth - spec.headerPaddingX);
      float headerTextHeight = resolve_line_height(frame(), textToken(col.headerRole));
      float headerTextY = headerY + (spec.headerHeight - headerTextHeight) * 0.5f;
      tableNode.createLabel(col.label,
                            col.headerRole,
                            Bounds{cursorX + spec.headerPaddingX,
                                   headerTextY,
                                   textWidth,
                                   headerTextHeight});
      cursorX += colWidth;
    }
  }

  float rowsY = headerY + spec.headerHeight;
  for (size_t rowIndex = 0; rowIndex < rowCount; ++rowIndex) {
    float rowY = rowsY + static_cast<float>(rowIndex) * (spec.rowHeight + spec.rowGap);
    RectRole rowRole = (rowIndex % 2 == 0) ? spec.rowAltRole : spec.rowRole;
    tableNode.createPanel(rowRole, Bounds{0.0f, rowY, tableWidth, spec.rowHeight});

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
      tableNode.createLabel(cellText,
                            col.cellRole,
                            Bounds{cursorX + spec.cellPaddingX,
                                   cellTextY,
                                   textWidth,
                                   cellTextHeight});
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
        tableNode.createDivider(divider);
      }
      if (rowsHeight > 0.0f) {
        divider.bounds = Bounds{cursor, rowsY, 1.0f, rowsHeight};
        tableNode.createDivider(divider);
      }
    }
  }

  return tableNode;
}

UiNode UiNode::createTable(Bounds const& bounds,
                           std::vector<TableColumn> columns,
                           std::vector<std::vector<std::string>> rows) {
  TableSpec spec;
  spec.bounds = bounds;
  spec.columns = std::move(columns);
  spec.rows = std::move(rows);
  return createTable(spec);
}

UiNode UiNode::createSectionHeader(SectionHeaderSpec const& spec) {
  UiNode header = createPanel(spec.backgroundRole, spec.bounds);

  if (spec.accentWidth > 0.0f) {
    header.createPanel(spec.accentRole,
                       Bounds{0.0f, 0.0f, spec.accentWidth, spec.bounds.height});
  }

  if (!spec.title.empty()) {
    float lineHeight = resolve_line_height(frame(), textToken(spec.textRole));
    float textY = (spec.bounds.height - lineHeight) * 0.5f + spec.textOffsetY;
    header.createLabel(spec.title,
                       spec.textRole,
                       Bounds{spec.textInsetX,
                              textY,
                              std::max(0.0f, spec.bounds.width - spec.textInsetX),
                              lineHeight});
  }

  if (spec.addDivider) {
    DividerSpec divider;
    divider.rectStyle = rectToken(spec.dividerRole);
    divider.bounds = Bounds{spec.bounds.x,
                            spec.bounds.y + spec.bounds.height + spec.dividerOffsetY,
                            spec.bounds.width,
                            1.0f};
    createDivider(divider);
  }

  return header;
}

UiNode UiNode::createSectionHeader(Bounds const& bounds,
                                   std::string_view title,
                                   TextRole role) {
  SectionHeaderSpec spec;
  spec.bounds = bounds;
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
  spec.bounds = bounds;
  spec.title = std::string(title);
  spec.textRole = role;
  spec.addDivider = addDivider;
  spec.dividerOffsetY = dividerOffsetY;
  return createSectionHeader(spec);
}

UiNode UiNode::createSectionHeader(SizeSpec const& size,
                                   std::string_view title,
                                   TextRole role) {
  Bounds bounds = resolve_bounds(Bounds{}, size);
  return createSectionHeader(bounds, title, role);
}

UiNode UiNode::createSectionHeader(SizeSpec const& size,
                                   std::string_view title,
                                   TextRole role,
                                   bool addDivider,
                                   float dividerOffsetY) {
  Bounds bounds = resolve_bounds(Bounds{}, size);
  return createSectionHeader(bounds, title, role, addDivider, dividerOffsetY);
}

SectionPanel UiNode::createSectionPanel(SectionPanelSpec const& spec) {
  SectionPanel out{createPanel(spec.panelRole, spec.bounds), {}, {}};

  float headerWidth = std::max(0.0f, spec.bounds.width - spec.headerInsetX - spec.headerInsetRight);
  Bounds headerBounds{spec.headerInsetX, spec.headerInsetY, headerWidth, spec.headerHeight};
  out.headerBounds = headerBounds;

  if (spec.headerHeight > 0.0f) {
    out.panel.createPanel(spec.headerRole, headerBounds);
  }

  if (spec.showAccent && spec.accentWidth > 0.0f && spec.headerHeight > 0.0f) {
    out.panel.createPanel(spec.accentRole,
                          Bounds{headerBounds.x, headerBounds.y, spec.accentWidth, headerBounds.height});
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
    out.panel.createTextLine(title);
  }

  float contentX = spec.contentInsetX;
  float contentY = headerBounds.y + headerBounds.height + spec.contentInsetY;
  float contentW = std::max(0.0f, spec.bounds.width - spec.contentInsetX - spec.contentInsetRight);
  float contentH = std::max(0.0f,
                            spec.bounds.height - (contentY + spec.contentInsetBottom));
  out.contentBounds = Bounds{contentX, contentY, contentW, contentH};

  return out;
}

SectionPanel UiNode::createSectionPanel(Bounds const& bounds, std::string_view title) {
  SectionPanelSpec spec;
  spec.bounds = bounds;
  spec.title = std::string(title);
  return createSectionPanel(spec);
}

SectionPanel UiNode::createSectionPanel(SizeSpec const& size, std::string_view title) {
  SectionPanelSpec spec;
  spec.bounds = resolve_bounds(Bounds{}, size);
  spec.title = std::string(title);
  return createSectionPanel(spec);
}

UiNode UiNode::createPropertyList(PropertyListSpec const& spec) {
  Bounds bounds = spec.bounds;
  if (bounds.height <= 0.0f && !spec.rows.empty()) {
    bounds.height = static_cast<float>(spec.rows.size()) * spec.rowHeight +
                    static_cast<float>(spec.rows.size() - 1) * spec.rowGap;
  }
  PrimeFrame::NodeId listId = create_node(frame(), id_, bounds,
                                          nullptr,
                                          PrimeFrame::LayoutType::None,
                                          PrimeFrame::Insets{},
                                          0.0f,
                                          false,
                                          true);
  UiNode listNode(frame(), listId);

  float labelLineHeight = resolve_line_height(frame(), textToken(spec.labelRole));
  float valueLineHeight = resolve_line_height(frame(), textToken(spec.valueRole));

  for (size_t i = 0; i < spec.rows.size(); ++i) {
    PropertyRow const& row = spec.rows[i];
    float rowY = static_cast<float>(i) * (spec.rowHeight + spec.rowGap);
    float labelY = rowY + (spec.rowHeight - labelLineHeight) * 0.5f;
    float valueY = rowY + (spec.rowHeight - valueLineHeight) * 0.5f;

    if (!row.label.empty()) {
      listNode.createLabel(row.label,
                           spec.labelRole,
                           Bounds{spec.labelInsetX,
                                  labelY,
                                  std::max(0.0f, bounds.width - spec.labelInsetX),
                                  labelLineHeight});
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
      listNode.createLabel(row.value,
                           spec.valueRole,
                           Bounds{valueX,
                                  valueY,
                                  std::max(0.0f, valueWidth),
                                  valueLineHeight});
    }
  }

  return listNode;
}

UiNode UiNode::createPropertyList(Bounds const& bounds, std::vector<PropertyRow> rows) {
  PropertyListSpec spec;
  spec.bounds = bounds;
  spec.rows = std::move(rows);
  return createPropertyList(spec);
}

UiNode UiNode::createPropertyRow(Bounds const& bounds,
                                 std::string_view label,
                                 std::string_view value,
                                 TextRole role) {
  PropertyListSpec spec;
  spec.bounds = bounds;
  if (bounds.height > 0.0f) {
    spec.rowHeight = bounds.height;
  }
  spec.rowGap = 0.0f;
  spec.labelRole = role;
  spec.valueRole = role;
  spec.rows = {{std::string(label), std::string(value)}};
  return createPropertyList(spec);
}

UiNode UiNode::createProgressBar(ProgressBarSpec const& spec) {
  Bounds bounds = spec.bounds;
  UiNode bar = createPanel(spec.trackRole, bounds);
  float clamped = std::clamp(spec.value, 0.0f, 1.0f);
  float fillWidth = bounds.width * clamped;
  if (spec.minFillWidth > 0.0f && clamped > 0.0f) {
    fillWidth = std::max(fillWidth, spec.minFillWidth);
  }
  if (fillWidth > 0.0f) {
    bar.createPanel(spec.fillRole, Bounds{0.0f, 0.0f, fillWidth, bounds.height});
  }
  return bar;
}

UiNode UiNode::createProgressBar(Bounds const& bounds, float value) {
  ProgressBarSpec spec;
  spec.bounds = bounds;
  spec.value = value;
  return createProgressBar(spec);
}

UiNode UiNode::createCardGrid(CardGridSpec const& spec) {
  Bounds bounds = resolve_bounds(spec.bounds, spec.size);
  PrimeFrame::NodeId gridId = create_node(frame(), id_, bounds,
                                          &spec.size,
                                          PrimeFrame::LayoutType::None,
                                          PrimeFrame::Insets{},
                                          0.0f,
                                          true,
                                          true);
  UiNode gridNode(frame(), gridId);

  if (spec.cards.empty()) {
    return gridNode;
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
    UiNode cardNode = gridNode.createPanel(spec.cardRole,
                                           Bounds{cardX, cardY, spec.cardWidth, spec.cardHeight});

    CardSpec const& card = spec.cards[i];
    if (!card.title.empty()) {
      cardNode.createLabel(card.title,
                           spec.titleRole,
                           Bounds{spec.paddingX,
                                  spec.titleOffsetY,
                                  std::max(0.0f, spec.cardWidth - spec.paddingX * 2.0f),
                                  titleLine});
    }
    if (!card.subtitle.empty()) {
      cardNode.createLabel(card.subtitle,
                           spec.subtitleRole,
                           Bounds{spec.paddingX,
                                  spec.subtitleOffsetY,
                                  std::max(0.0f, spec.cardWidth - spec.paddingX * 2.0f),
                                  subtitleLine});
    }
  }

  return gridNode;
}

UiNode UiNode::createCardGrid(Bounds const& bounds, std::vector<CardSpec> cards) {
  CardGridSpec spec;
  spec.bounds = bounds;
  spec.cards = std::move(cards);
  return createCardGrid(spec);
}

UiNode UiNode::createButton(ButtonSpec const& spec) {
  Bounds bounds = resolve_bounds(spec.bounds, spec.size);
  PanelSpec panel;
  panel.bounds = bounds;
  panel.size = spec.size;
  panel.rectStyle = spec.backgroundStyle;
  panel.rectStyleOverride = spec.backgroundStyleOverride;
  UiNode button = createPanel(panel);
  if (spec.label.empty()) {
    return button;
  }
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

  button.createLabel(label);
  return button;
}

UiNode UiNode::createButton(Bounds const& bounds,
                            std::string_view label,
                            ButtonVariant variant) {
  ButtonSpec spec;
  spec.bounds = bounds;
  spec.label = std::string(label);
  if (variant == ButtonVariant::Primary) {
    spec.backgroundStyle = rectToken(RectRole::Accent);
    spec.textStyle = textToken(TextRole::BodyBright);
  } else {
    spec.backgroundStyle = rectToken(RectRole::Panel);
    spec.textStyle = textToken(TextRole::BodyBright);
  }
  return createButton(spec);
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
  Bounds bounds = resolve_bounds(spec.bounds, spec.size);
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
    return field;
  }

  float lineHeight = resolve_line_height(frame(), style);
  float textY = (bounds.height - lineHeight) * 0.5f + spec.textOffsetY;
  float textWidth = std::max(0.0f, bounds.width - spec.paddingX * 2.0f);

  LabelSpec label;
  label.text = std::string(content);
  label.textStyle = style;
  label.textStyleOverride = overrideStyle;
  label.bounds = Bounds{spec.paddingX, textY, textWidth, lineHeight};
  field.createLabel(label);

  return field;
}

UiNode UiNode::createTextField(Bounds const& bounds, std::string_view placeholder) {
  TextFieldSpec spec;
  spec.bounds = bounds;
  spec.placeholder = std::string(placeholder);
  spec.backgroundStyle = rectToken(RectRole::Panel);
  spec.textStyle = textToken(TextRole::BodyBright);
  spec.placeholderStyle = textToken(TextRole::BodyMuted);
  return createTextField(spec);
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
  UiNode bar = createPanel(spec.backgroundRole, spec.bounds);

  float leftLineHeight = resolve_line_height(frame(), textToken(spec.leftRole));
  float rightLineHeight = resolve_line_height(frame(), textToken(spec.rightRole));

  float leftY = (spec.bounds.height - leftLineHeight) * 0.5f;
  float rightY = (spec.bounds.height - rightLineHeight) * 0.5f;

  float rightWidth = 0.0f;
  if (!spec.rightText.empty()) {
    rightWidth = estimate_text_width(frame(), textToken(spec.rightRole), spec.rightText);
    float maxRightWidth = std::max(0.0f, spec.bounds.width - spec.paddingX);
    rightWidth = std::min(rightWidth, maxRightWidth);
  }

  float rightX = spec.bounds.width - spec.paddingX - rightWidth;
  if (rightX < 0.0f) {
    rightX = 0.0f;
  }

  float leftMaxWidth = std::max(0.0f, spec.bounds.width - spec.paddingX * 2.0f);
  if (!spec.rightText.empty()) {
    leftMaxWidth = std::min(leftMaxWidth, std::max(0.0f, rightX - spec.paddingX));
  }

  if (!spec.leftText.empty()) {
    bar.createLabel(spec.leftText,
                    spec.leftRole,
                    Bounds{spec.paddingX, leftY, leftMaxWidth, leftLineHeight});
  }

  if (!spec.rightText.empty()) {
    bar.createLabel(spec.rightText,
                    spec.rightRole,
                    Bounds{rightX, rightY, rightWidth, rightLineHeight});
  }

  return bar;
}

UiNode UiNode::createStatusBar(Bounds const& bounds,
                               std::string_view leftText,
                               std::string_view rightText) {
  StatusBarSpec spec;
  spec.bounds = bounds;
  spec.leftText = std::string(leftText);
  spec.rightText = std::string(rightText);
  return createStatusBar(spec);
}

UiNode UiNode::createScrollView(ScrollViewSpec const& spec) {
  Bounds bounds = resolve_bounds(spec.bounds, spec.size);
  if (bounds.width <= 0.0f || bounds.height <= 0.0f) {
    return UiNode(frame(), id_);
  }

  PrimeFrame::NodeId scrollId = create_node(frame(), id_, bounds,
                                            &spec.size,
                                            PrimeFrame::LayoutType::None,
                                            PrimeFrame::Insets{},
                                            0.0f,
                                            spec.clipChildren,
                                            true);
  UiNode scrollNode(frame(), scrollId);

  if (spec.showVertical && spec.vertical.enabled) {
    float trackW = spec.vertical.thickness;
    float trackH = std::max(0.0f, bounds.height - spec.vertical.startPadding - spec.vertical.endPadding);
    float trackX = bounds.width - spec.vertical.inset;
    float trackY = spec.vertical.startPadding;
    scrollNode.createPanel(spec.vertical.trackStyle, Bounds{trackX, trackY, trackW, trackH});

    float thumbH = std::min(trackH, spec.vertical.thumbLength);
    float maxOffset = std::max(0.0f, trackH - thumbH);
    float thumbOffset = std::clamp(spec.vertical.thumbOffset, 0.0f, maxOffset);
    float thumbY = trackY + thumbOffset;
    scrollNode.createPanel(spec.vertical.thumbStyle, Bounds{trackX, thumbY, trackW, thumbH});
  }

  if (spec.showHorizontal && spec.horizontal.enabled) {
    float trackH = spec.horizontal.thickness;
    float trackW = std::max(0.0f, bounds.width - spec.horizontal.startPadding - spec.horizontal.endPadding);
    float trackX = spec.horizontal.startPadding;
    float trackY = bounds.height - spec.horizontal.inset;
    scrollNode.createPanel(spec.horizontal.trackStyle, Bounds{trackX, trackY, trackW, trackH});

    float thumbW = std::min(trackW, spec.horizontal.thumbLength);
    float maxOffset = std::max(0.0f, trackW - thumbW);
    float thumbOffset = std::clamp(spec.horizontal.thumbOffset, 0.0f, maxOffset);
    float thumbX = trackX + thumbOffset;
    scrollNode.createPanel(spec.horizontal.thumbStyle, Bounds{thumbX, trackY, thumbW, trackH});
  }

  return scrollNode;
}

UiNode UiNode::createScrollHints(ScrollHintsSpec const& spec) {
  ScrollViewSpec view;
  view.bounds = resolve_bounds(spec.bounds, spec.size);
  view.size = spec.size;
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
  spec.bounds = bounds;
  return createScrollHints(spec);
}

UiNode UiNode::createTreeView(TreeViewSpec const& spec) {
  Bounds bounds = resolve_bounds(spec.bounds, spec.size);
  if (bounds.width <= 0.0f || bounds.height <= 0.0f) {
    return UiNode(frame(), id_);
  }

  PrimeFrame::NodeId treeId = create_node(frame(), id_, bounds,
                                          nullptr,
                                          PrimeFrame::LayoutType::None,
                                          PrimeFrame::Insets{},
                                          0.0f,
                                          spec.clipChildren,
                                          true);
  UiNode treeNode(frame(), treeId);

  std::vector<FlatTreeRow> rows;
  std::vector<int> depthStack;
  flatten_tree(spec.nodes, 0, depthStack, rows);

  float rowWidth = bounds.width - spec.rowWidthInset;
  float rowTextHeight = resolve_line_height(frame(), textToken(spec.textRole));
  float selectedTextHeight = resolve_line_height(frame(), textToken(spec.selectedTextRole));

  for (size_t i = 0; i < rows.size(); ++i) {
    FlatTreeRow const& row = rows[i];
    float rowY = spec.rowStartY + static_cast<float>(i) * (spec.rowHeight + spec.rowGap);
    RectRole rowRole = row.selected ? spec.selectionRole
                                    : (i % 2 == 0 ? spec.rowAltRole : spec.rowRole);

    treeNode.createPanel(rowRole, Bounds{spec.rowStartX, rowY, rowWidth, spec.rowHeight});

    if (row.selected && spec.selectionAccentWidth > 0.0f) {
      treeNode.createPanel(spec.selectionAccentRole,
                           Bounds{spec.rowStartX, rowY, spec.selectionAccentWidth, spec.rowHeight});
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
      treeNode.createPanel(rowRole,
                           Bounds{glyphX - maskPad,
                                  glyphY - maskPad,
                                  spec.caretSize + maskPad * 2.0f,
                                  spec.caretSize + maskPad * 2.0f});
    }

    if (row.hasChildren) {
      treeNode.createPanel(spec.caretBackgroundRole,
                           Bounds{glyphX, glyphY, spec.caretSize, spec.caretSize});
      treeNode.createPanel(spec.caretLineRole,
                           Bounds{glyphX + spec.caretInset,
                                  glyphY + spec.caretSize * 0.5f - spec.caretThickness * 0.5f,
                                  spec.caretSize - spec.caretInset * 2.0f,
                                  spec.caretThickness});
      if (!row.expanded) {
        treeNode.createPanel(spec.caretLineRole,
                             Bounds{glyphX + spec.caretSize * 0.5f - spec.caretThickness * 0.5f,
                                    glyphY + spec.caretInset,
                                    spec.caretThickness,
                                    spec.caretSize - spec.caretInset * 2.0f});
      }
    } else {
      treeNode.createPanel(spec.caretLineRole,
                           Bounds{glyphX + spec.caretSize * 0.5f - 1.0f,
                                  glyphY + spec.caretSize * 0.5f - 1.0f,
                                  2.0f,
                                  2.0f});
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
    treeNode.createLabel(row.label,
                         textRole,
                         Bounds{textX, textY, rowWidth - (textX - spec.rowStartX), lineHeight});
  }

  if (spec.showScrollBar && spec.scrollBar.enabled) {
    float trackX = bounds.width - spec.scrollBar.inset;
    float trackY = spec.scrollBar.padding;
    float trackH = std::max(0.0f, bounds.height - spec.scrollBar.padding * 2.0f);
    float trackW = spec.scrollBar.width;
    treeNode.createPanel(spec.scrollBar.trackStyle, Bounds{trackX, trackY, trackW, trackH});

    float thumbH = trackH * spec.scrollBar.thumbFraction;
    thumbH = std::max(thumbH, spec.scrollBar.minThumbHeight);
    if (thumbH > trackH) {
      thumbH = trackH;
    }
    float maxOffset = std::max(0.0f, trackH - thumbH);
    float progress = std::clamp(spec.scrollBar.thumbProgress, 0.0f, 1.0f);
    float thumbY = trackY + maxOffset * progress;
    treeNode.createPanel(spec.scrollBar.thumbStyle, Bounds{trackX, thumbY, trackW, thumbH});
  }

  return treeNode;
}

UiNode createRoot(PrimeFrame::Frame& frame, Bounds const& bounds) {
  PrimeFrame::Theme* theme = frame.getTheme(PrimeFrame::DefaultThemeId);
  if (theme && (theme->palette.empty() || theme->rectStyles.empty() || theme->textStyles.empty() ||
                is_default_theme(*theme))) {
    applyStudioTheme(frame);
  }
  PrimeFrame::NodeId id = create_node(frame, PrimeFrame::NodeId{}, bounds,
                                      nullptr,
                                      PrimeFrame::LayoutType::None,
                                      PrimeFrame::Insets{},
                                      0.0f,
                                      false,
                                      true);
  return UiNode(frame, id);
}

ShellLayout createShell(PrimeFrame::Frame& frame, ShellSpec const& spec) {
  float width = spec.bounds.width;
  float height = spec.bounds.height;
  float contentH = std::max(0.0f, height - spec.topbarHeight - spec.statusHeight);
  float contentW = std::max(0.0f, width - spec.sidebarWidth - spec.inspectorWidth);

  Bounds topbarBounds{0.0f, 0.0f, width, spec.topbarHeight};
  Bounds statusBounds{0.0f, height - spec.statusHeight, width, spec.statusHeight};
  Bounds sidebarBounds{0.0f, spec.topbarHeight, spec.sidebarWidth, contentH};
  Bounds contentBounds{spec.sidebarWidth, spec.topbarHeight, contentW, contentH};
  Bounds inspectorBounds{width - spec.inspectorWidth, spec.topbarHeight, spec.inspectorWidth, contentH};

  UiNode root = createRoot(frame, spec.bounds);
  UiNode background = root.createPanel(spec.backgroundRole, spec.bounds);
  UiNode topbar = root.createPanel(spec.topbarRole, topbarBounds);
  UiNode sidebar = root.createPanel(spec.sidebarRole, sidebarBounds);
  UiNode content = root.createPanel(spec.contentRole, contentBounds);
  UiNode inspector = root.createPanel(spec.inspectorRole, inspectorBounds);

  if (spec.drawDividers) {
    DividerSpec divider;
    divider.rectStyle = rectToken(spec.dividerRole);
    divider.bounds = Bounds{0.0f, spec.topbarHeight, width, 1.0f};
    root.createDivider(divider);
    divider.bounds = Bounds{spec.sidebarWidth - 1.0f, spec.topbarHeight, 1.0f, contentH};
    root.createDivider(divider);
    divider.bounds = Bounds{width - spec.inspectorWidth, spec.topbarHeight, 1.0f, contentH};
    root.createDivider(divider);
    divider.bounds = Bounds{0.0f, height - spec.statusHeight, width, 1.0f};
    root.createDivider(divider);
  }

  return ShellLayout{
      root,
      background,
      topbar,
      sidebar,
      content,
      inspector,
      spec.bounds,
      topbarBounds,
      statusBounds,
      sidebarBounds,
      contentBounds,
      inspectorBounds
  };
}

ShellSpec makeShellSpec(Bounds const& bounds) {
  ShellSpec spec;
  spec.bounds = bounds;
  return spec;
}

Version getVersion() {
  return Version{};
}

std::string_view getVersionString() {
  return "0.1.0";
}

} // namespace PrimeStage
