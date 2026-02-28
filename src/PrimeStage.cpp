#include "PrimeStage/PrimeStage.h"
#include "PrimeStage/GeneratedVersion.h"
#include "PrimeStage/TextSelection.h"
#include "PrimeStageCollectionInternals.h"
#include "PrimeFrame/Events.h"
#include "PrimeFrame/Focus.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <limits>
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

constexpr float DisabledScrimOpacity = 0.38f;
constexpr float ReadOnlyScrimOpacity = 0.16f;
constexpr float DefaultSelectableTextWrapWidth = 360.0f;
constexpr float DefaultScrollViewWidth = 320.0f;
constexpr float DefaultScrollViewHeight = 180.0f;
constexpr float DefaultCollectionWidth = 280.0f;
constexpr float DefaultCollectionHeight = 120.0f;
constexpr float MinDefaultTextContrastRatio = 4.5f;
constexpr float MinDefaultFocusContrastRatio = 3.0f;

void apply_default_accessibility_semantics(AccessibilitySemantics& semantics,
                                           AccessibilityRole role,
                                           bool enabled) {
  if (semantics.role == AccessibilityRole::Unspecified) {
    semantics.role = role;
  }
  semantics.state.disabled = !enabled;
}

void apply_default_checked_semantics(AccessibilitySemantics& semantics,
                                     AccessibilityRole role,
                                     bool enabled,
                                     bool checked) {
  apply_default_accessibility_semantics(semantics, role, enabled);
  semantics.state.checked = checked;
}

void apply_default_range_semantics(AccessibilitySemantics& semantics,
                                   AccessibilityRole role,
                                   bool enabled,
                                   float value) {
  apply_default_accessibility_semantics(semantics, role, enabled);
  semantics.state.valueNow = value;
  semantics.state.valueMin = 0.0f;
  semantics.state.valueMax = 1.0f;
}

void report_validation_float(char const* context,
                             char const* field,
                             float original,
                             float adjusted) {
#if !defined(NDEBUG)
  if (original != adjusted) {
    std::fprintf(stderr,
                 "PrimeStage validation: %s.%s adjusted from %.3f to %.3f\n",
                 context,
                 field,
                 static_cast<double>(original),
                 static_cast<double>(adjusted));
  }
#else
  (void)context;
  (void)field;
  (void)original;
  (void)adjusted;
#endif
}

void report_validation_int(char const* context,
                           char const* field,
                           int original,
                           int adjusted) {
#if !defined(NDEBUG)
  if (original != adjusted) {
    std::fprintf(stderr,
                 "PrimeStage validation: %s.%s adjusted from %d to %d\n",
                 context,
                 field,
                 original,
                 adjusted);
  }
#else
  (void)context;
  (void)field;
  (void)original;
  (void)adjusted;
#endif
}

void report_validation_uint(char const* context,
                            char const* field,
                            uint32_t original,
                            uint32_t adjusted) {
#if !defined(NDEBUG)
  if (original != adjusted) {
    std::fprintf(stderr,
                 "PrimeStage validation: %s.%s adjusted from %u to %u\n",
                 context,
                 field,
                 static_cast<unsigned>(original),
                 static_cast<unsigned>(adjusted));
  }
#else
  (void)context;
  (void)field;
  (void)original;
  (void)adjusted;
#endif
}

float clamp_non_negative(float value, char const* context, char const* field) {
  float adjusted = std::max(0.0f, value);
  report_validation_float(context, field, value, adjusted);
  return adjusted;
}

float clamp_unit_interval(float value, char const* context, char const* field) {
  float adjusted = std::clamp(value, 0.0f, 1.0f);
  report_validation_float(context, field, value, adjusted);
  return adjusted;
}

std::optional<float> clamp_optional_non_negative(std::optional<float> value,
                                                 char const* context,
                                                 char const* field) {
  if (!value.has_value()) {
    return value;
  }
  float adjusted = std::max(0.0f, *value);
  report_validation_float(context, field, *value, adjusted);
  return adjusted;
}

std::optional<float> clamp_optional_unit_interval(std::optional<float> value,
                                                  char const* context,
                                                  char const* field) {
  if (!value.has_value()) {
    return value;
  }
  float adjusted = std::clamp(*value, 0.0f, 1.0f);
  report_validation_float(context, field, *value, adjusted);
  return adjusted;
}

void sanitize_size_spec(SizeSpec& size, char const* context) {
  size.minWidth = clamp_optional_non_negative(size.minWidth, context, "minWidth");
  size.maxWidth = clamp_optional_non_negative(size.maxWidth, context, "maxWidth");
  size.preferredWidth =
      clamp_optional_non_negative(size.preferredWidth, context, "preferredWidth");
  size.stretchX = clamp_non_negative(size.stretchX, context, "stretchX");

  size.minHeight = clamp_optional_non_negative(size.minHeight, context, "minHeight");
  size.maxHeight = clamp_optional_non_negative(size.maxHeight, context, "maxHeight");
  size.preferredHeight =
      clamp_optional_non_negative(size.preferredHeight, context, "preferredHeight");
  size.stretchY = clamp_non_negative(size.stretchY, context, "stretchY");

  if (size.minWidth.has_value() && size.maxWidth.has_value() &&
      *size.minWidth > *size.maxWidth) {
    report_validation_float(context, "maxWidth", *size.maxWidth, *size.minWidth);
    size.maxWidth = size.minWidth;
  }
  if (size.minHeight.has_value() && size.maxHeight.has_value() &&
      *size.minHeight > *size.maxHeight) {
    report_validation_float(context, "maxHeight", *size.maxHeight, *size.minHeight);
    size.maxHeight = size.minHeight;
  }

  if (size.preferredWidth.has_value()) {
    float preferred = *size.preferredWidth;
    if (size.minWidth.has_value() && preferred < *size.minWidth) {
      report_validation_float(context, "preferredWidth", preferred, *size.minWidth);
      preferred = *size.minWidth;
    }
    if (size.maxWidth.has_value() && preferred > *size.maxWidth) {
      report_validation_float(context, "preferredWidth", preferred, *size.maxWidth);
      preferred = *size.maxWidth;
    }
    size.preferredWidth = preferred;
  }

  if (size.preferredHeight.has_value()) {
    float preferred = *size.preferredHeight;
    if (size.minHeight.has_value() && preferred < *size.minHeight) {
      report_validation_float(context, "preferredHeight", preferred, *size.minHeight);
      preferred = *size.minHeight;
    }
    if (size.maxHeight.has_value() && preferred > *size.maxHeight) {
      report_validation_float(context, "preferredHeight", preferred, *size.maxHeight);
      preferred = *size.maxHeight;
    }
    size.preferredHeight = preferred;
  }

#if !defined(NDEBUG)
  assert(!size.minWidth.has_value() || *size.minWidth >= 0.0f);
  assert(!size.maxWidth.has_value() || *size.maxWidth >= 0.0f);
  assert(!size.preferredWidth.has_value() || *size.preferredWidth >= 0.0f);
  assert(!size.minHeight.has_value() || *size.minHeight >= 0.0f);
  assert(!size.maxHeight.has_value() || *size.maxHeight >= 0.0f);
  assert(!size.preferredHeight.has_value() || *size.preferredHeight >= 0.0f);
  assert(!size.minWidth.has_value() || !size.maxWidth.has_value() ||
         *size.minWidth <= *size.maxWidth);
  assert(!size.minHeight.has_value() || !size.maxHeight.has_value() ||
         *size.minHeight <= *size.maxHeight);
#endif
}

PrimeFrame::Insets sanitize_insets(PrimeFrame::Insets insets, char const* context) {
  insets.left = clamp_non_negative(insets.left, context, "padding.left");
  insets.top = clamp_non_negative(insets.top, context, "padding.top");
  insets.right = clamp_non_negative(insets.right, context, "padding.right");
  insets.bottom = clamp_non_negative(insets.bottom, context, "padding.bottom");
  return insets;
}

int clamp_selected_index(int value, int count, char const* context, char const* field) {
  if (count <= 0) {
    int adjusted = 0;
    report_validation_int(context, field, value, adjusted);
    return adjusted;
  }
  int adjusted = std::clamp(value, 0, count - 1);
  report_validation_int(context, field, value, adjusted);
  return adjusted;
}

int clamp_selected_row_or_none(int value, int count, char const* context, char const* field) {
  if (count <= 0) {
    int adjusted = -1;
    report_validation_int(context, field, value, adjusted);
    return adjusted;
  }
  if (value < 0 || value >= count) {
    int adjusted = -1;
    report_validation_int(context, field, value, adjusted);
    return adjusted;
  }
  return value;
}

int clamp_tab_index(int value, char const* context, char const* field) {
  int adjusted = std::max(value, -1);
  report_validation_int(context, field, value, adjusted);
  return adjusted;
}

uint32_t clamp_text_index(uint32_t value,
                          uint32_t maxValue,
                          char const* context,
                          char const* field) {
  uint32_t adjusted = std::min(value, maxValue);
  report_validation_uint(context, field, value, adjusted);
  return adjusted;
}

bool text_field_state_is_pristine(TextFieldState const& state) {
  return state.text.empty() &&
         state.cursor == 0u &&
         state.selectionAnchor == 0u &&
         state.selectionStart == 0u &&
         state.selectionEnd == 0u &&
         !state.focused &&
         !state.hovered &&
         !state.selecting &&
         state.pointerId == -1 &&
         !state.cursorVisible &&
         state.nextBlink == std::chrono::steady_clock::time_point{} &&
         state.cursorHint == CursorHint::Arrow;
}

void seed_text_field_state_from_spec(TextFieldState& state, TextFieldSpec const& spec) {
  state.text = std::string(spec.text);
  uint32_t size = static_cast<uint32_t>(state.text.size());
  state.cursor = std::min(spec.cursorIndex, size);
  state.selectionAnchor = state.cursor;
  state.selectionStart = std::min(spec.selectionStart, size);
  state.selectionEnd = std::min(spec.selectionEnd, size);
  state.cursorVisible = spec.showCursor;
}


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

void apply_size_spec(PrimeFrame::Node& node,
                     SizeSpec const& size,
                     char const* context = "SizeSpec") {
  SizeSpec sanitized = size;
  sanitize_size_spec(sanitized, context);

  node.sizeHint.width.min = sanitized.minWidth;
  node.sizeHint.width.max = sanitized.maxWidth;
  if (!node.sizeHint.width.preferred.has_value() && sanitized.preferredWidth.has_value()) {
    node.sizeHint.width.preferred = sanitized.preferredWidth;
  }
  node.sizeHint.width.stretch = sanitized.stretchX;

  node.sizeHint.height.min = sanitized.minHeight;
  node.sizeHint.height.max = sanitized.maxHeight;
  if (!node.sizeHint.height.preferred.has_value() && sanitized.preferredHeight.has_value()) {
    node.sizeHint.height.preferred = sanitized.preferredHeight;
  }
  node.sizeHint.height.stretch = sanitized.stretchY;
}

Rect resolve_rect(SizeSpec const& size) {
  SizeSpec sanitized = size;
  sanitize_size_spec(sanitized, "SizeSpec");
  Rect resolved;
  if (sanitized.preferredWidth.has_value()) {
    resolved.width = *sanitized.preferredWidth;
  }
  if (sanitized.preferredHeight.has_value()) {
    resolved.height = *sanitized.preferredHeight;
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
                               bool visible,
                               char const* context = "UiNode") {
  PrimeFrame::NodeId id = frame.createNode();
  PrimeFrame::Node* node = frame.getNode(id);
  if (!node) {
    return id;
  }
  apply_rect(*node, rect);
  if (size) {
    apply_size_spec(*node, *size, context);
  }
  node->layout = layout;
  node->padding = sanitize_insets(padding, context);
  node->gap = clamp_non_negative(gap, context, "gap");
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

struct FocusOverlay {
  std::vector<PrimeFrame::PrimitiveId> primitives;
  PrimeFrame::RectStyleOverride focused{};
  PrimeFrame::RectStyleOverride blurred{};
  PrimeFrame::NodeId overlayNode{};
};

struct ResolvedFocusStyle {
  PrimeFrame::RectStyleToken token = 0;
  PrimeFrame::RectStyleOverride overrideStyle{};
};

PrimeFrame::RectStyleToken resolve_focus_style_token(
    PrimeFrame::RectStyleToken requested,
    std::initializer_list<PrimeFrame::RectStyleToken> fallbacks) {
  if (requested != 0) {
    return requested;
  }
  for (PrimeFrame::RectStyleToken token : fallbacks) {
    if (token != 0) {
      return token;
    }
  }
  return 0;
}

bool color_is_zero(PrimeFrame::Color const& color) {
  return std::abs(color.r) <= 0.0001f &&
         std::abs(color.g) <= 0.0001f &&
         std::abs(color.b) <= 0.0001f &&
         std::abs(color.a) <= 0.0001f;
}

bool color_is_near(PrimeFrame::Color const& color,
                   float r,
                   float g,
                   float b,
                   float a,
                   float epsilon = 0.0001f) {
  return std::abs(color.r - r) <= epsilon &&
         std::abs(color.g - g) <= epsilon &&
         std::abs(color.b - b) <= epsilon &&
         std::abs(color.a - a) <= epsilon;
}

float linearize_srgb(float channel) {
  channel = std::clamp(channel, 0.0f, 1.0f);
  if (channel <= 0.04045f) {
    return channel / 12.92f;
  }
  return std::pow((channel + 0.055f) / 1.055f, 2.4f);
}

float relative_luminance(PrimeFrame::Color const& color) {
  float r = linearize_srgb(color.r);
  float g = linearize_srgb(color.g);
  float b = linearize_srgb(color.b);
  return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

float color_contrast_ratio(PrimeFrame::Color const& lhs, PrimeFrame::Color const& rhs) {
  float lhsLum = relative_luminance(lhs);
  float rhsLum = relative_luminance(rhs);
  float high = std::max(lhsLum, rhsLum);
  float low = std::min(lhsLum, rhsLum);
  return (high + 0.05f) / (low + 0.05f);
}

PrimeFrame::Color make_theme_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255u) {
  auto to_float = [](uint8_t value) -> float {
    return static_cast<float>(value) / 255.0f;
  };
  return PrimeFrame::Color{
      to_float(r),
      to_float(g),
      to_float(b),
      to_float(a),
  };
}

bool is_canonical_primeframe_default_theme(PrimeFrame::Theme const& theme) {
  if (theme.palette.size() != 1u || theme.rectStyles.size() != 1u || theme.textStyles.size() != 1u) {
    return false;
  }
  PrimeFrame::RectStyle const& rect = theme.rectStyles[0];
  PrimeFrame::TextStyle const& text = theme.textStyles[0];
  return color_is_near(theme.palette[0], 0.0f, 0.0f, 0.0f, 1.0f) && rect.fill == 0u &&
         std::abs(rect.opacity - 1.0f) <= 0.0001f && text.color == 0u &&
         std::abs(text.size - 14.0f) <= 0.0001f && std::abs(text.weight - 400.0f) <= 0.0001f;
}

void install_primestage_default_theme(PrimeFrame::Theme& theme) {
  theme.name = "PrimeStage Default";
  theme.palette = {
      make_theme_color(5, 12, 26),     // 0: backdrop/disabled tint
      make_theme_color(40, 48, 62),    // 1: surface
      make_theme_color(54, 64, 80),    // 2: surface alt
      make_theme_color(78, 89, 108),   // 3: divider/muted
      make_theme_color(55, 122, 210),  // 4: accent
      make_theme_color(30, 167, 67),   // 5: selection
      make_theme_color(255, 89, 45),   // 6: focus
      make_theme_color(239, 243, 248), // 7: text primary
      make_theme_color(203, 212, 223), // 8: text muted
      make_theme_color(245, 211, 133), // 9: text accent
      make_theme_color(86, 97, 112),   // 10: track/knob base
  };

  theme.rectStyles.assign(6u, PrimeFrame::RectStyle{});
  theme.rectStyles[0] = PrimeFrame::RectStyle{1u, 1.0f};
  theme.rectStyles[1] = PrimeFrame::RectStyle{6u, 1.0f};
  theme.rectStyles[2] = PrimeFrame::RectStyle{2u, 1.0f};
  theme.rectStyles[3] = PrimeFrame::RectStyle{4u, 1.0f};
  theme.rectStyles[4] = PrimeFrame::RectStyle{5u, 1.0f};
  theme.rectStyles[5] = PrimeFrame::RectStyle{10u, 1.0f};

  theme.textStyles.assign(2u, PrimeFrame::TextStyle{});
  theme.textStyles[0].color = 7u;
  theme.textStyles[1].color = 9u;
}

PrimeFrame::Color resolve_theme_surface_color(PrimeFrame::Theme const& theme) {
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

void ensure_readable_theme_defaults(PrimeFrame::Frame& frame) {
  PrimeFrame::Theme* theme = frame.getTheme(PrimeFrame::DefaultThemeId);
  if (!theme) {
    return;
  }

  if (is_canonical_primeframe_default_theme(*theme)) {
    install_primestage_default_theme(*theme);
    return;
  }

  if (theme->palette.empty()) {
    install_primestage_default_theme(*theme);
    return;
  }
  if (theme->rectStyles.empty()) {
    theme->rectStyles.resize(1u, PrimeFrame::RectStyle{});
    theme->rectStyles[0].fill = 0u;
    theme->rectStyles[0].opacity = 1.0f;
  }
  if (theme->textStyles.empty()) {
    theme->textStyles.resize(1u, PrimeFrame::TextStyle{});
    theme->textStyles[0].color = 0u;
    theme->textStyles[0].size = 14.0f;
    theme->textStyles[0].weight = 400.0f;
  }

  PrimeFrame::RectStyleToken fillToken = theme->rectStyles[0].fill;
  if (fillToken >= theme->palette.size()) {
    fillToken = 0u;
    theme->rectStyles[0].fill = fillToken;
  }
  PrimeFrame::Color fillColor = theme->palette[fillToken];

  PrimeFrame::ColorToken textToken = theme->textStyles[0].color;
  PrimeFrame::Color textColor{};
  if (textToken < theme->palette.size()) {
    textColor = theme->palette[textToken];
  }

  bool paletteLooksZeroed = true;
  size_t sampleCount = std::min<size_t>(theme->palette.size(), 8u);
  for (size_t i = 0; i < sampleCount; ++i) {
    if (!color_is_zero(theme->palette[i])) {
      paletteLooksZeroed = false;
      break;
    }
  }

  float contrast = color_contrast_ratio(textColor, fillColor);
  bool needsReadablePatch = paletteLooksZeroed || textToken >= theme->palette.size() ||
                            contrast < MinDefaultTextContrastRatio ||
                            theme->textStyles[0].size <= 0.0f ||
                            color_is_near(fillColor, 0.0f, 0.0f, 0.0f, 1.0f);
  if (!needsReadablePatch) {
    return;
  }

  if (color_is_near(fillColor, 0.0f, 0.0f, 0.0f, 1.0f)) {
    theme->palette[fillToken] = make_theme_color(40, 48, 62);
    fillColor = theme->palette[fillToken];
  }

  size_t bestToken = 0u;
  float bestContrast = -1.0f;
  for (size_t i = 0; i < theme->palette.size(); ++i) {
    float candidate = color_contrast_ratio(theme->palette[i], fillColor);
    if (candidate > bestContrast) {
      bestContrast = candidate;
      bestToken = i;
    }
  }
  if (bestContrast < MinDefaultTextContrastRatio) {
    PrimeFrame::Color lightText = make_theme_color(236, 242, 250);
    PrimeFrame::Color darkText = make_theme_color(16, 20, 27);
    float lightContrast = color_contrast_ratio(lightText, fillColor);
    float darkContrast = color_contrast_ratio(darkText, fillColor);
    PrimeFrame::Color fallbackText = lightContrast >= darkContrast ? lightText : darkText;
    theme->palette.push_back(fallbackText);
    bestToken = theme->palette.size() - 1u;
  }
  theme->textStyles[0].color = static_cast<PrimeFrame::ColorToken>(bestToken);
  if (theme->textStyles[0].size <= 0.0f) {
    theme->textStyles[0].size = 14.0f;
  }
  if (theme->textStyles[0].weight <= 0.0f) {
    theme->textStyles[0].weight = 400.0f;
  }
}

PrimeFrame::Color resolve_semantic_focus_color(PrimeFrame::Frame& frame) {
  PrimeFrame::Theme const* theme = frame.getTheme(PrimeFrame::DefaultThemeId);
  if (theme && !theme->palette.empty()) {
    PrimeFrame::Color surface = resolve_theme_surface_color(*theme);
    float bestContrast = 0.0f;
    PrimeFrame::Color bestColor = theme->palette[0];
    constexpr std::array<size_t, 6> PreferredPaletteIndices{6u, 8u, 7u, 2u, 1u, 0u};
    for (size_t index : PreferredPaletteIndices) {
      if (index < theme->palette.size()) {
        PrimeFrame::Color candidate = theme->palette[index];
        float contrast = color_contrast_ratio(candidate, surface);
        if (contrast > bestContrast) {
          bestContrast = contrast;
          bestColor = candidate;
        }
        if (contrast >= MinDefaultFocusContrastRatio) {
          return candidate;
        }
      }
    }
    for (PrimeFrame::Color const& candidate : theme->palette) {
      float contrast = color_contrast_ratio(candidate, surface);
      if (contrast > bestContrast) {
        bestContrast = contrast;
        bestColor = candidate;
      }
    }
    if (bestContrast >= MinDefaultFocusContrastRatio) {
      return bestColor;
    }
    std::array<PrimeFrame::Color, 4> fallbackCandidates{
        make_theme_color(255, 89, 45),
        make_theme_color(55, 122, 210),
        make_theme_color(236, 242, 250),
        make_theme_color(16, 20, 27),
    };
    for (PrimeFrame::Color const& candidate : fallbackCandidates) {
      float contrast = color_contrast_ratio(candidate, surface);
      if (contrast > bestContrast) {
        bestContrast = contrast;
        bestColor = candidate;
      }
    }
    return bestColor;
  }
  return PrimeFrame::Color{0.20f, 0.56f, 0.95f, 1.0f};
}

PrimeFrame::Color resolve_semantic_disabled_color(PrimeFrame::Frame& frame) {
  PrimeFrame::Theme const* theme = frame.getTheme(PrimeFrame::DefaultThemeId);
  if (theme && !theme->palette.empty()) {
    PrimeFrame::Color color = theme->palette.front();
    color.a = 1.0f;
    return color;
  }
  return PrimeFrame::Color{0.0f, 0.0f, 0.0f, 1.0f};
}

void add_state_scrim_overlay(PrimeFrame::Frame& frame,
                             PrimeFrame::NodeId parent,
                             Rect const& bounds,
                             float opacity,
                             bool visible) {
  if (!visible || !parent.isValid() || bounds.width <= 0.0f || bounds.height <= 0.0f ||
      opacity <= 0.0f) {
    return;
  }
  PrimeFrame::RectStyleOverride overlayStyle;
  overlayStyle.fill = resolve_semantic_disabled_color(frame);
  overlayStyle.opacity = std::clamp(opacity, 0.0f, 1.0f);
  PrimeFrame::NodeId overlayId = create_node(frame,
                                             parent,
                                             bounds,
                                             nullptr,
                                             PrimeFrame::LayoutType::None,
                                             PrimeFrame::Insets{},
                                             0.0f,
                                             false,
                                             visible,
                                             "StateScrimOverlay");
  if (PrimeFrame::Node* node = frame.getNode(overlayId)) {
    node->hitTestVisible = false;
  }
  add_rect_primitive(frame, overlayId, 1u, overlayStyle);
  frame.removeChild(parent, overlayId);
  frame.addChild(parent, overlayId);
}

ResolvedFocusStyle resolve_focus_style(PrimeFrame::Frame& frame,
                                       PrimeFrame::RectStyleToken requestedToken,
                                       PrimeFrame::RectStyleOverride const& requestedOverride,
                                       std::initializer_list<PrimeFrame::RectStyleToken> fallbacks,
                                       std::optional<PrimeFrame::RectStyleOverride> fallbackOverride =
                                           std::nullopt) {
  ResolvedFocusStyle resolved;
  resolved.token = resolve_focus_style_token(requestedToken, fallbacks);

  if (requestedToken != 0) {
    resolved.overrideStyle = requestedOverride;
  } else if (fallbackOverride.has_value()) {
    resolved.overrideStyle = *fallbackOverride;
  }

  if (resolved.token == 0) {
    resolved.token = 1;
    resolved.overrideStyle.fill = resolve_semantic_focus_color(frame);
    if (!resolved.overrideStyle.opacity.has_value()) {
      resolved.overrideStyle.opacity = 1.0f;
    }
  }
  return resolved;
}

constexpr float FocusRingThickness = 2.0f;

std::vector<PrimeFrame::PrimitiveId> add_focus_ring_primitives(
    PrimeFrame::Frame& frame,
    PrimeFrame::NodeId nodeId,
    PrimeFrame::RectStyleToken token,
    PrimeFrame::RectStyleOverride const& overrideStyle,
    Rect const* bounds) {
  std::vector<PrimeFrame::PrimitiveId> prims;
  if (token == 0) {
    return prims;
  }
  if (!bounds || bounds->width <= 0.0f || bounds->height <= 0.0f) {
    prims.push_back(add_rect_primitive_with_rect(frame, nodeId, Rect{}, token, overrideStyle));
    return prims;
  }
  float maxThickness = std::min(bounds->width, bounds->height) * 0.5f;
  float thickness = std::clamp(FocusRingThickness, 1.0f, maxThickness);
  Rect top{0.0f, 0.0f, bounds->width, thickness};
  Rect bottom{0.0f,
              std::max(0.0f, bounds->height - thickness),
              bounds->width,
              thickness};
  float sideHeight = std::max(0.0f, bounds->height - thickness * 2.0f);
  Rect left{0.0f, thickness, thickness, sideHeight};
  Rect right{std::max(0.0f, bounds->width - thickness), thickness, thickness, sideHeight};
  auto add_if = [&](Rect const& rect) {
    if (rect.width <= 0.0f || rect.height <= 0.0f) {
      return;
    }
    prims.push_back(add_rect_primitive_with_rect(frame, nodeId, rect, token, overrideStyle));
  };
  add_if(top);
  add_if(bottom);
  add_if(left);
  add_if(right);
  if (prims.empty()) {
    prims.push_back(add_rect_primitive_with_rect(frame, nodeId, Rect{}, token, overrideStyle));
  }
  return prims;
}

std::optional<FocusOverlay> add_focus_overlay_node(PrimeFrame::Frame& frame,
                                                   PrimeFrame::NodeId parent,
                                                   Rect const& rect,
                                                   PrimeFrame::RectStyleToken token,
                                                   PrimeFrame::RectStyleOverride const& overrideStyle,
                                                   bool visible) {
  if (token == 0) {
    return std::nullopt;
  }
  FocusOverlay overlay;
  overlay.focused = overrideStyle;
  overlay.blurred = overrideStyle;
  overlay.blurred.opacity = 0.0f;
  PrimeFrame::NodeId overlayId =
      create_node(frame, parent, rect, nullptr, PrimeFrame::LayoutType::None,
                  PrimeFrame::Insets{}, 0.0f, false, visible);
  if (PrimeFrame::Node* node = frame.getNode(overlayId)) {
    node->hitTestVisible = false;
  }
  overlay.overlayNode = overlayId;
  overlay.primitives = add_focus_ring_primitives(frame, overlayId, token, overlay.blurred, &rect);
  if (overlay.primitives.empty()) {
    return std::nullopt;
  }
  // Keep focus overlay as the last sibling so flatten traversal renders it above content/highlight nodes.
  frame.removeChild(parent, overlayId);
  frame.addChild(parent, overlayId);
  return overlay;
}

void attach_focus_callbacks(PrimeFrame::Frame& frame,
                            PrimeFrame::NodeId nodeId,
                            FocusOverlay const& overlay) {
  if (overlay.primitives.empty()) {
    return;
  }
  auto applyFocus = [framePtr = &frame,
                     prims = overlay.primitives,
                     focused = overlay.focused,
                     blurred = overlay.blurred](bool focusedState) {
    for (PrimeFrame::PrimitiveId primId : prims) {
      PrimeFrame::Primitive* prim = framePtr->getPrimitive(primId);
      if (!prim || prim->type != PrimeFrame::PrimitiveType::Rect) {
        continue;
      }
      prim->rect.overrideStyle = focusedState ? focused : blurred;
    }
  };
  auto promoteOverlay = [framePtr = &frame, overlayId = overlay.overlayNode]() {
    if (!overlayId.isValid()) {
      return;
    }
    PrimeFrame::Node* overlayNode = framePtr->getNode(overlayId);
    if (!overlayNode) {
      return;
    }
    PrimeFrame::NodeId parent = overlayNode->parent;
    if (!parent.isValid()) {
      return;
    }
    framePtr->removeChild(parent, overlayId);
    framePtr->addChild(parent, overlayId);
  };

  PrimeFrame::Node* node = frame.getNode(nodeId);
  if (!node) {
    return;
  }
  if (node->callbacks != PrimeFrame::InvalidCallbackId) {
    PrimeFrame::Callback* callback = frame.getCallback(node->callbacks);
    if (!callback) {
      return;
    }
    auto prevFocus = callback->onFocus;
    auto prevBlur = callback->onBlur;
    callback->onFocus = [promoteOverlay, applyFocus, prevFocus]() {
      promoteOverlay();
      applyFocus(true);
      if (prevFocus) {
        prevFocus();
      }
    };
    callback->onBlur = [applyFocus, prevBlur]() {
      applyFocus(false);
      if (prevBlur) {
        prevBlur();
      }
    };
    return;
  }

  PrimeFrame::Callback callback;
  callback.onFocus = [promoteOverlay, applyFocus]() {
    promoteOverlay();
    applyFocus(true);
  };
  callback.onBlur = [applyFocus]() { applyFocus(false); };
  node->callbacks = frame.addCallback(std::move(callback));
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

} // namespace

std::vector<float> buildCaretPositions(PrimeFrame::Frame& frame,
                                       PrimeFrame::TextStyleToken token,
                                       std::string_view text);

namespace Internal {

WidgetRuntimeContext makeWidgetRuntimeContext(PrimeFrame::Frame& frame,
                                              PrimeFrame::NodeId parentId,
                                              bool allowAbsolute,
                                              bool enabled,
                                              bool visible,
                                              int tabIndex) {
  WidgetRuntimeContext runtime;
  runtime.frame = &frame;
  runtime.parentId = parentId;
  runtime.allowAbsolute = allowAbsolute;
  runtime.enabled = enabled;
  runtime.visible = visible;
  runtime.tabIndex = tabIndex;
  return runtime;
}

PrimeFrame::Frame& runtimeFrame(WidgetRuntimeContext const& runtime) {
  assert(runtime.frame != nullptr);
  return *runtime.frame;
}

UiNode makeParentNode(WidgetRuntimeContext const& runtime) {
  return UiNode(runtimeFrame(runtime), runtime.parentId, runtime.allowAbsolute);
}

UiNode createExtensionPrimitive(WidgetRuntimeContext const& runtime,
                                ExtensionPrimitiveSpec const& spec) {
  PrimeFrame::Frame& frame = runtimeFrame(runtime);
  bool interactive = runtime.visible && runtime.enabled;
  PrimeFrame::NodeId nodeId = createNode(frame,
                                         runtime.parentId,
                                         spec.rect,
                                         &spec.size,
                                         spec.layout,
                                         spec.padding,
                                         spec.gap,
                                         spec.clipChildren,
                                         runtime.visible,
                                         "ExtensionPrimitiveSpec");
  UiNode node(frame, nodeId, runtime.allowAbsolute);
  PrimeFrame::Node* builtNode = frame.getNode(nodeId);
  if (!builtNode) {
    return node;
  }

  builtNode->focusable = interactive && spec.focusable;
  builtNode->hitTestVisible = interactive && spec.hitTestVisible;
  builtNode->tabIndex = builtNode->focusable ? runtime.tabIndex : -1;

  (void)createRectNode(frame,
                       nodeId,
                       InternalRect{0.0f, 0.0f, 0.0f, 0.0f},
                       spec.rectStyle,
                       spec.rectStyleOverride,
                       false,
                       runtime.visible);

  if (interactive && spec.callbacks.onEvent) {
    (void)appendNodeOnEvent(runtime, nodeId, spec.callbacks.onEvent);
  }
  if (interactive && spec.callbacks.onFocus) {
    (void)LowLevel::appendNodeOnFocus(frame, nodeId, spec.callbacks.onFocus);
  }
  if (interactive && spec.callbacks.onBlur) {
    (void)LowLevel::appendNodeOnBlur(frame, nodeId, spec.callbacks.onBlur);
  }

  return node;
}

void configureInteractiveRoot(WidgetRuntimeContext const& runtime, PrimeFrame::NodeId nodeId) {
  if (!runtime.visible) {
    return;
  }
  PrimeFrame::Node* node = runtimeFrame(runtime).getNode(nodeId);
  if (!node) {
    return;
  }
  node->focusable = runtime.enabled;
  node->hitTestVisible = runtime.enabled;
  node->tabIndex = runtime.enabled ? runtime.tabIndex : -1;
}

bool appendNodeOnEvent(WidgetRuntimeContext const& runtime,
                       PrimeFrame::NodeId nodeId,
                       std::function<bool(PrimeFrame::Event const&)> onEvent) {
  return LowLevel::appendNodeOnEvent(runtimeFrame(runtime), nodeId, std::move(onEvent));
}

ListSpec normalizeListSpec(ListSpec const& specInput) {
  ListSpec spec = specInput;
  sanitize_size_spec(spec.size, "ListSpec.size");
  spec.rowHeight = clamp_non_negative(spec.rowHeight, "ListSpec", "rowHeight");
  spec.rowGap = clamp_non_negative(spec.rowGap, "ListSpec", "rowGap");
  spec.rowPaddingX = clamp_non_negative(spec.rowPaddingX, "ListSpec", "rowPaddingX");
  spec.selectedIndex = clamp_selected_row_or_none(spec.selectedIndex,
                                                  static_cast<int>(spec.items.size()),
                                                  "ListSpec",
                                                  "selectedIndex");
  spec.tabIndex = clamp_tab_index(spec.tabIndex, "ListSpec", "tabIndex");
  apply_default_accessibility_semantics(spec.accessibility, AccessibilityRole::Table, spec.enabled);
  return spec;
}

TableSpec normalizeTableSpec(TableSpec const& specInput) {
  TableSpec spec = specInput;
  sanitize_size_spec(spec.size, "TableSpec.size");
  spec.headerInset = clamp_non_negative(spec.headerInset, "TableSpec", "headerInset");
  spec.headerHeight = clamp_non_negative(spec.headerHeight, "TableSpec", "headerHeight");
  spec.rowHeight = clamp_non_negative(spec.rowHeight, "TableSpec", "rowHeight");
  spec.rowGap = clamp_non_negative(spec.rowGap, "TableSpec", "rowGap");
  spec.headerPaddingX = clamp_non_negative(spec.headerPaddingX, "TableSpec", "headerPaddingX");
  spec.cellPaddingX = clamp_non_negative(spec.cellPaddingX, "TableSpec", "cellPaddingX");
  spec.selectedRow = clamp_selected_row_or_none(spec.selectedRow,
                                                 static_cast<int>(spec.rows.size()),
                                                 "TableSpec",
                                                 "selectedRow");
  spec.tabIndex = clamp_tab_index(spec.tabIndex, "TableSpec", "tabIndex");
  apply_default_accessibility_semantics(spec.accessibility, AccessibilityRole::Table, spec.enabled);
  return spec;
}

TreeViewSpec normalizeTreeViewSpec(TreeViewSpec const& specInput) {
  TreeViewSpec spec = specInput;
  spec.tabIndex = clamp_tab_index(spec.tabIndex, "TreeViewSpec", "tabIndex");
  apply_default_accessibility_semantics(spec.accessibility, AccessibilityRole::Tree, spec.enabled);
  return spec;
}

ProgressBarSpec normalizeProgressBarSpec(ProgressBarSpec const& specInput) {
  ProgressBarSpec spec = specInput;
  sanitize_size_spec(spec.size, "ProgressBarSpec.size");
  spec.value = clamp_unit_interval(spec.value, "ProgressBarSpec", "value");
  spec.minFillWidth = clamp_non_negative(spec.minFillWidth, "ProgressBarSpec", "minFillWidth");
  spec.tabIndex = clamp_tab_index(spec.tabIndex, "ProgressBarSpec", "tabIndex");
  bool enabled = spec.enabled;
  if (spec.binding.state) {
    spec.binding.state->value = clamp_unit_interval(spec.binding.state->value,
                                                    "State<float>",
                                                    "value");
    spec.value = spec.binding.state->value;
  } else if (spec.state) {
    spec.state->value = clamp_unit_interval(spec.state->value, "ProgressBarState", "value");
    spec.value = spec.state->value;
  }
  apply_default_range_semantics(spec.accessibility, AccessibilityRole::ProgressBar, enabled, spec.value);
  return spec;
}

DropdownSpec normalizeDropdownSpec(DropdownSpec const& specInput) {
  DropdownSpec spec = specInput;
  sanitize_size_spec(spec.size, "DropdownSpec.size");
  spec.paddingX = clamp_non_negative(spec.paddingX, "DropdownSpec", "paddingX");
  spec.indicatorGap = clamp_non_negative(spec.indicatorGap, "DropdownSpec", "indicatorGap");
  spec.tabIndex = clamp_tab_index(spec.tabIndex, "DropdownSpec", "tabIndex");
  bool enabled = spec.enabled;

  int optionCount = static_cast<int>(spec.options.size());
  int selectedIndex =
      clamp_selected_index(spec.selectedIndex, optionCount, "DropdownSpec", "selectedIndex");
  if (spec.binding.state) {
    selectedIndex = clamp_selected_index(spec.binding.state->value,
                                         optionCount,
                                         "State<int>",
                                         "value");
    spec.binding.state->value = selectedIndex;
  } else if (spec.state) {
    selectedIndex = clamp_selected_index(spec.state->selectedIndex,
                                         optionCount,
                                         "DropdownState",
                                         "selectedIndex");
    spec.state->selectedIndex = selectedIndex;
  }
  spec.selectedIndex = selectedIndex;
  apply_default_accessibility_semantics(spec.accessibility, AccessibilityRole::ComboBox, enabled);
  if (optionCount > 0) {
    spec.accessibility.state.positionInSet = selectedIndex + 1;
    spec.accessibility.state.setSize = optionCount;
  } else {
    spec.accessibility.state.positionInSet.reset();
    spec.accessibility.state.setSize.reset();
  }
  return spec;
}

TabsSpec normalizeTabsSpec(TabsSpec const& specInput) {
  TabsSpec spec = specInput;
  sanitize_size_spec(spec.size, "TabsSpec.size");
  spec.tabPaddingX = clamp_non_negative(spec.tabPaddingX, "TabsSpec", "tabPaddingX");
  spec.tabPaddingY = clamp_non_negative(spec.tabPaddingY, "TabsSpec", "tabPaddingY");
  spec.gap = clamp_non_negative(spec.gap, "TabsSpec", "gap");
  spec.tabIndex = clamp_tab_index(spec.tabIndex, "TabsSpec", "tabIndex");
  bool enabled = spec.enabled;

  int tabCount = static_cast<int>(spec.labels.size());
  int selectedIndex = clamp_selected_index(spec.selectedIndex, tabCount, "TabsSpec", "selectedIndex");
  if (spec.binding.state) {
    selectedIndex = clamp_selected_index(spec.binding.state->value,
                                         tabCount,
                                         "State<int>",
                                         "value");
    spec.binding.state->value = selectedIndex;
  } else if (spec.state) {
    selectedIndex = clamp_selected_index(spec.state->selectedIndex,
                                         tabCount,
                                         "TabsState",
                                         "selectedIndex");
    spec.state->selectedIndex = selectedIndex;
  }
  spec.selectedIndex = selectedIndex;
  apply_default_accessibility_semantics(spec.accessibility, AccessibilityRole::TabList, enabled);
  if (tabCount > 0) {
    spec.accessibility.state.positionInSet = selectedIndex + 1;
    spec.accessibility.state.setSize = tabCount;
  } else {
    spec.accessibility.state.positionInSet.reset();
    spec.accessibility.state.setSize.reset();
  }
  return spec;
}

ToggleSpec normalizeToggleSpec(ToggleSpec const& specInput) {
  ToggleSpec spec = specInput;
  sanitize_size_spec(spec.size, "ToggleSpec.size");
  spec.knobInset = clamp_non_negative(spec.knobInset, "ToggleSpec", "knobInset");
  spec.tabIndex = clamp_tab_index(spec.tabIndex, "ToggleSpec", "tabIndex");
  bool enabled = spec.enabled;
  bool on = spec.binding.state ? spec.binding.state->value : (spec.state ? spec.state->on : spec.on);
  spec.on = on;
  apply_default_checked_semantics(spec.accessibility, AccessibilityRole::Toggle, enabled, on);
  return spec;
}

CheckboxSpec normalizeCheckboxSpec(CheckboxSpec const& specInput) {
  CheckboxSpec spec = specInput;
  sanitize_size_spec(spec.size, "CheckboxSpec.size");
  spec.boxSize = clamp_non_negative(spec.boxSize, "CheckboxSpec", "boxSize");
  spec.checkInset = clamp_non_negative(spec.checkInset, "CheckboxSpec", "checkInset");
  spec.gap = clamp_non_negative(spec.gap, "CheckboxSpec", "gap");
  spec.tabIndex = clamp_tab_index(spec.tabIndex, "CheckboxSpec", "tabIndex");
  bool enabled = spec.enabled;
  bool checked = spec.binding.state ? spec.binding.state->value
                                    : (spec.state ? spec.state->checked : spec.checked);
  spec.checked = checked;
  apply_default_checked_semantics(spec.accessibility, AccessibilityRole::Checkbox, enabled, checked);
  return spec;
}

SliderSpec normalizeSliderSpec(SliderSpec const& specInput) {
  SliderSpec spec = specInput;
  sanitize_size_spec(spec.size, "SliderSpec.size");
  spec.value = clamp_unit_interval(spec.value, "SliderSpec", "value");
  spec.trackThickness = clamp_non_negative(spec.trackThickness, "SliderSpec", "trackThickness");
  spec.thumbSize = clamp_non_negative(spec.thumbSize, "SliderSpec", "thumbSize");
  spec.fillHoverOpacity =
      clamp_optional_unit_interval(spec.fillHoverOpacity, "SliderSpec", "fillHoverOpacity");
  spec.fillPressedOpacity =
      clamp_optional_unit_interval(spec.fillPressedOpacity, "SliderSpec", "fillPressedOpacity");
  spec.trackHoverOpacity =
      clamp_optional_unit_interval(spec.trackHoverOpacity, "SliderSpec", "trackHoverOpacity");
  spec.trackPressedOpacity =
      clamp_optional_unit_interval(spec.trackPressedOpacity, "SliderSpec", "trackPressedOpacity");
  spec.thumbHoverOpacity =
      clamp_optional_unit_interval(spec.thumbHoverOpacity, "SliderSpec", "thumbHoverOpacity");
  spec.thumbPressedOpacity =
      clamp_optional_unit_interval(spec.thumbPressedOpacity, "SliderSpec", "thumbPressedOpacity");
  spec.tabIndex = clamp_tab_index(spec.tabIndex, "SliderSpec", "tabIndex");
  bool enabled = spec.enabled;
  if (spec.binding.state) {
    spec.binding.state->value = clamp_unit_interval(spec.binding.state->value,
                                                    "State<float>",
                                                    "value");
    spec.value = spec.binding.state->value;
  } else if (spec.state) {
    spec.state->value = clamp_unit_interval(spec.state->value, "SliderState", "value");
    spec.value = spec.state->value;
  }
  apply_default_range_semantics(spec.accessibility, AccessibilityRole::Slider, enabled, spec.value);
  return spec;
}

ButtonSpec normalizeButtonSpec(ButtonSpec const& specInput) {
  ButtonSpec spec = specInput;
  sanitize_size_spec(spec.size, "ButtonSpec.size");
  spec.textInsetX = clamp_non_negative(spec.textInsetX, "ButtonSpec", "textInsetX");
  spec.baseOpacity = clamp_unit_interval(spec.baseOpacity, "ButtonSpec", "baseOpacity");
  spec.hoverOpacity = clamp_unit_interval(spec.hoverOpacity, "ButtonSpec", "hoverOpacity");
  spec.pressedOpacity = clamp_unit_interval(spec.pressedOpacity, "ButtonSpec", "pressedOpacity");
  spec.tabIndex = clamp_tab_index(spec.tabIndex, "ButtonSpec", "tabIndex");
  apply_default_accessibility_semantics(spec.accessibility, AccessibilityRole::Button, spec.enabled);
  return spec;
}

DividerSpec normalizeDividerSpec(DividerSpec const& specInput) {
  DividerSpec spec = specInput;
  sanitize_size_spec(spec.size, "DividerSpec.size");
  return spec;
}

SpacerSpec normalizeSpacerSpec(SpacerSpec const& specInput) {
  SpacerSpec spec = specInput;
  sanitize_size_spec(spec.size, "SpacerSpec.size");
  return spec;
}

TextLineSpec normalizeTextLineSpec(TextLineSpec const& specInput) {
  TextLineSpec spec = specInput;
  apply_default_accessibility_semantics(spec.accessibility, AccessibilityRole::StaticText, true);
  return spec;
}

TextFieldSpec normalizeTextFieldSpec(TextFieldSpec const& specInput) {
  TextFieldSpec spec = specInput;
  sanitize_size_spec(spec.size, "TextFieldSpec.size");
  spec.paddingX = clamp_non_negative(spec.paddingX, "TextFieldSpec", "paddingX");
  spec.cursorWidth = clamp_non_negative(spec.cursorWidth, "TextFieldSpec", "cursorWidth");
  if (spec.cursorBlinkInterval.count() < 0) {
    report_validation_int("TextFieldSpec",
                          "cursorBlinkIntervalMs",
                          static_cast<int>(spec.cursorBlinkInterval.count()),
                          0);
    spec.cursorBlinkInterval = std::chrono::milliseconds(0);
  }
  spec.tabIndex = clamp_tab_index(spec.tabIndex, "TextFieldSpec", "tabIndex");
  apply_default_accessibility_semantics(spec.accessibility, AccessibilityRole::TextField, spec.enabled);
  return spec;
}

LabelSpec normalizeLabelSpec(LabelSpec const& specInput) {
  LabelSpec spec = specInput;
  sanitize_size_spec(spec.size, "LabelSpec.size");
  spec.maxWidth = clamp_non_negative(spec.maxWidth, "LabelSpec", "maxWidth");
  apply_default_accessibility_semantics(spec.accessibility, AccessibilityRole::StaticText, true);
  return spec;
}

ParagraphSpec normalizeParagraphSpec(ParagraphSpec const& specInput) {
  ParagraphSpec spec = specInput;
  sanitize_size_spec(spec.size, "ParagraphSpec.size");
  spec.maxWidth = clamp_non_negative(spec.maxWidth, "ParagraphSpec", "maxWidth");
  apply_default_accessibility_semantics(spec.accessibility, AccessibilityRole::StaticText, true);
  return spec;
}

SelectableTextSpec normalizeSelectableTextSpec(SelectableTextSpec const& specInput) {
  SelectableTextSpec spec = specInput;
  sanitize_size_spec(spec.size, "SelectableTextSpec.size");
  spec.paddingX = clamp_non_negative(spec.paddingX, "SelectableTextSpec", "paddingX");
  spec.maxWidth = clamp_non_negative(spec.maxWidth, "SelectableTextSpec", "maxWidth");
  apply_default_accessibility_semantics(spec.accessibility, AccessibilityRole::StaticText, spec.enabled);
  return spec;
}

PanelSpec normalizePanelSpec(PanelSpec const& specInput) {
  PanelSpec spec = specInput;
  sanitize_size_spec(spec.size, "PanelSpec.size");
  spec.padding = sanitize_insets(spec.padding, "PanelSpec");
  spec.gap = clamp_non_negative(spec.gap, "PanelSpec", "gap");
  return spec;
}

TextSelectionOverlaySpec normalizeTextSelectionOverlaySpec(TextSelectionOverlaySpec const& specInput) {
  return specInput;
}

WindowSpec normalizeWindowSpec(WindowSpec const& specInput) {
  WindowSpec spec = specInput;
  spec.width = clamp_non_negative(spec.width, "WindowSpec", "width");
  spec.height = clamp_non_negative(spec.height, "WindowSpec", "height");
  spec.minWidth = clamp_non_negative(spec.minWidth, "WindowSpec", "minWidth");
  spec.minHeight = clamp_non_negative(spec.minHeight, "WindowSpec", "minHeight");
  spec.titleBarHeight = clamp_non_negative(spec.titleBarHeight, "WindowSpec", "titleBarHeight");
  spec.contentPadding = clamp_non_negative(spec.contentPadding, "WindowSpec", "contentPadding");
  spec.resizeHandleSize =
      clamp_non_negative(spec.resizeHandleSize, "WindowSpec", "resizeHandleSize");
  spec.tabIndex = clamp_tab_index(spec.tabIndex, "WindowSpec", "tabIndex");
  apply_default_accessibility_semantics(spec.accessibility, AccessibilityRole::Group, spec.focusable);

  if (spec.width < spec.minWidth) {
    report_validation_float("WindowSpec", "width", spec.width, spec.minWidth);
    spec.width = spec.minWidth;
  }
  if (spec.height < spec.minHeight) {
    report_validation_float("WindowSpec", "height", spec.height, spec.minHeight);
    spec.height = spec.minHeight;
  }
  return spec;
}

ScrollViewSpec normalizeScrollViewSpec(ScrollViewSpec const& specInput) {
  ScrollViewSpec spec = specInput;
  sanitize_size_spec(spec.size, "ScrollViewSpec.size");
  spec.vertical.thickness = clamp_non_negative(spec.vertical.thickness, "ScrollViewSpec.vertical", "thickness");
  spec.vertical.inset = clamp_non_negative(spec.vertical.inset, "ScrollViewSpec.vertical", "inset");
  spec.vertical.startPadding =
      clamp_non_negative(spec.vertical.startPadding, "ScrollViewSpec.vertical", "startPadding");
  spec.vertical.endPadding =
      clamp_non_negative(spec.vertical.endPadding, "ScrollViewSpec.vertical", "endPadding");
  spec.vertical.thumbLength =
      clamp_non_negative(spec.vertical.thumbLength, "ScrollViewSpec.vertical", "thumbLength");
  spec.vertical.thumbOffset =
      clamp_non_negative(spec.vertical.thumbOffset, "ScrollViewSpec.vertical", "thumbOffset");
  spec.horizontal.thickness =
      clamp_non_negative(spec.horizontal.thickness, "ScrollViewSpec.horizontal", "thickness");
  spec.horizontal.inset =
      clamp_non_negative(spec.horizontal.inset, "ScrollViewSpec.horizontal", "inset");
  spec.horizontal.startPadding =
      clamp_non_negative(spec.horizontal.startPadding, "ScrollViewSpec.horizontal", "startPadding");
  spec.horizontal.endPadding =
      clamp_non_negative(spec.horizontal.endPadding, "ScrollViewSpec.horizontal", "endPadding");
  spec.horizontal.thumbLength =
      clamp_non_negative(spec.horizontal.thumbLength, "ScrollViewSpec.horizontal", "thumbLength");
  spec.horizontal.thumbOffset =
      clamp_non_negative(spec.horizontal.thumbOffset, "ScrollViewSpec.horizontal", "thumbOffset");
  return spec;
}

InternalRect resolveRect(SizeSpec const& size) {
  Rect resolved = resolve_rect(size);
  return InternalRect{resolved.x, resolved.y, resolved.width, resolved.height};
}

float defaultScrollViewWidth() {
  return DefaultScrollViewWidth;
}

float defaultScrollViewHeight() {
  return DefaultScrollViewHeight;
}

float defaultCollectionWidth() {
  return DefaultCollectionWidth;
}

float defaultCollectionHeight() {
  return DefaultCollectionHeight;
}

float defaultSelectableTextWrapWidth() {
  return DefaultSelectableTextWrapWidth;
}

bool textFieldStateIsPristine(TextFieldState const& state) {
  return text_field_state_is_pristine(state);
}

void seedTextFieldStateFromSpec(TextFieldState& state, TextFieldSpec const& spec) {
  seed_text_field_state_from_spec(state, spec);
}

uint32_t clampTextIndex(uint32_t value,
                        uint32_t maxValue,
                        char const* context,
                        char const* field) {
  return clamp_text_index(value, maxValue, context, field);
}

std::vector<float> buildCaretPositionsForText(PrimeFrame::Frame& frame,
                                              PrimeFrame::TextStyleToken token,
                                              std::string_view text) {
  return buildCaretPositions(frame, token, text);
}

float estimateTextWidth(PrimeFrame::Frame& frame,
                        PrimeFrame::TextStyleToken token,
                        std::string_view text) {
  return estimate_text_width(frame, token, text);
}

float sliderValueFromEvent(PrimeFrame::Event const& event, bool vertical, float thumbSize) {
  return slider_value_from_event(event, vertical, thumbSize);
}

float resolveLineHeight(PrimeFrame::Frame& frame, PrimeFrame::TextStyleToken token) {
  return resolve_line_height(frame, token);
}

InternalFocusStyle resolveFocusStyle(PrimeFrame::Frame& frame,
                                     PrimeFrame::RectStyleToken focusStyle,
                                     PrimeFrame::RectStyleOverride const& focusStyleOverride,
                                     PrimeFrame::RectStyleToken fallbackA,
                                     PrimeFrame::RectStyleToken fallbackB,
                                     PrimeFrame::RectStyleToken fallbackC,
                                     PrimeFrame::RectStyleToken fallbackD,
                                     PrimeFrame::RectStyleToken fallbackE,
                                     std::optional<PrimeFrame::RectStyleOverride> fallbackOverride) {
  ResolvedFocusStyle resolved = resolve_focus_style(frame,
                                                    focusStyle,
                                                    focusStyleOverride,
                                                    {fallbackA, fallbackB, fallbackC, fallbackD, fallbackE},
                                                    fallbackOverride);
  return InternalFocusStyle{resolved.token, resolved.overrideStyle};
}

void attachFocusOverlay(PrimeFrame::Frame& frame,
                        PrimeFrame::NodeId nodeId,
                        InternalRect const& rect,
                        InternalFocusStyle const& focusStyle,
                        bool visible) {
  auto overlay = add_focus_overlay_node(frame,
                                        nodeId,
                                        Rect{rect.x, rect.y, rect.width, rect.height},
                                        focusStyle.token,
                                        focusStyle.overrideStyle,
                                        visible);
  if (overlay.has_value()) {
    attach_focus_callbacks(frame, nodeId, *overlay);
  }
}

void attachFocusOverlay(WidgetRuntimeContext const& runtime,
                        PrimeFrame::NodeId nodeId,
                        InternalRect const& rect,
                        InternalFocusStyle const& focusStyle) {
  attachFocusOverlay(runtimeFrame(runtime), nodeId, rect, focusStyle, runtime.visible);
}

void addDisabledScrimOverlay(PrimeFrame::Frame& frame,
                             PrimeFrame::NodeId nodeId,
                             InternalRect const& rect,
                             bool visible) {
  add_state_scrim_overlay(frame,
                          nodeId,
                          Rect{rect.x, rect.y, rect.width, rect.height},
                          DisabledScrimOpacity,
                          visible);
}

void addDisabledScrimOverlay(WidgetRuntimeContext const& runtime,
                             PrimeFrame::NodeId nodeId,
                             InternalRect const& rect) {
  addDisabledScrimOverlay(runtimeFrame(runtime), nodeId, rect, runtime.visible);
}

void addReadOnlyScrimOverlay(PrimeFrame::Frame& frame,
                             PrimeFrame::NodeId nodeId,
                             InternalRect const& rect,
                             bool visible) {
  add_state_scrim_overlay(frame,
                          nodeId,
                          Rect{rect.x, rect.y, rect.width, rect.height},
                          ReadOnlyScrimOpacity,
                          visible);
}

PrimeFrame::NodeId createNode(PrimeFrame::Frame& frame,
                              PrimeFrame::NodeId parent,
                              InternalRect const& rect,
                              SizeSpec const* size,
                              PrimeFrame::LayoutType layout,
                              PrimeFrame::Insets const& padding,
                              float gap,
                              bool clipChildren,
                              bool visible,
                              char const* context) {
  return create_node(frame,
                     parent,
                     Rect{rect.x, rect.y, rect.width, rect.height},
                     size,
                     layout,
                     padding,
                     gap,
                     clipChildren,
                     visible,
                     context);
}

PrimeFrame::NodeId createRectNode(PrimeFrame::Frame& frame,
                                  PrimeFrame::NodeId parent,
                                  InternalRect const& rect,
                                  PrimeFrame::RectStyleToken token,
                                  PrimeFrame::RectStyleOverride const& overrideStyle,
                                  bool clipChildren,
                                  bool visible) {
  return create_rect_node(frame,
                          parent,
                          Rect{rect.x, rect.y, rect.width, rect.height},
                          token,
                          overrideStyle,
                          clipChildren,
                          visible);
}

PrimeFrame::NodeId createTextNode(PrimeFrame::Frame& frame,
                                  PrimeFrame::NodeId parent,
                                  InternalRect const& rect,
                                  std::string_view text,
                                  PrimeFrame::TextStyleToken textStyle,
                                  PrimeFrame::TextStyleOverride const& overrideStyle,
                                  PrimeFrame::TextAlign align,
                                  PrimeFrame::WrapMode wrap,
                                  float maxWidth,
                                  bool visible) {
  return create_text_node(frame,
                          parent,
                          Rect{rect.x, rect.y, rect.width, rect.height},
                          text,
                          textStyle,
                          overrideStyle,
                          align,
                          wrap,
                          maxWidth,
                          visible);
}

} // namespace Internal

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

UiNode::UiNode(PrimeFrame::Frame& frame, PrimeFrame::NodeId id, bool allowAbsolute)
    : frame_(frame), id_(id), allowAbsolute_(allowAbsolute) {
  ensure_readable_theme_defaults(frame_);
}

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

Version getVersion() {
  Version version;
  version.major = static_cast<uint32_t>(PRIMESTAGE_VERSION_MAJOR);
  version.minor = static_cast<uint32_t>(PRIMESTAGE_VERSION_MINOR);
  version.patch = static_cast<uint32_t>(PRIMESTAGE_VERSION_PATCH);
  return version;
}

std::string_view getVersionString() {
  return PRIMESTAGE_VERSION_STRING;
}

} // namespace PrimeStage
