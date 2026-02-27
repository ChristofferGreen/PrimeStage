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

constexpr int KeyEnter = keyCodeInt(KeyCode::Enter);
constexpr int KeySpace = keyCodeInt(KeyCode::Space);
constexpr int KeyLeft = keyCodeInt(KeyCode::Left);
constexpr int KeyRight = keyCodeInt(KeyCode::Right);
constexpr int KeyDown = keyCodeInt(KeyCode::Down);
constexpr int KeyUp = keyCodeInt(KeyCode::Up);
constexpr int KeyHome = keyCodeInt(KeyCode::Home);
constexpr int KeyEnd = keyCodeInt(KeyCode::End);
constexpr float DisabledScrimOpacity = 0.38f;
constexpr float ReadOnlyScrimOpacity = 0.16f;
constexpr float DefaultParagraphWrapWidth = 360.0f;
constexpr float DefaultSelectableTextWrapWidth = 360.0f;
constexpr float DefaultScrollViewWidth = 320.0f;
constexpr float DefaultScrollViewHeight = 180.0f;
constexpr float DefaultCollectionWidth = 280.0f;
constexpr float DefaultCollectionHeight = 120.0f;
constexpr float MinDefaultTextContrastRatio = 4.5f;
constexpr float MinDefaultFocusContrastRatio = 3.0f;

bool is_activation_key(int key) {
  return key == KeyEnter || key == KeySpace;
}

bool is_pointer_inside(PrimeFrame::Event const& event) {
  return event.localX >= 0.0f && event.localX <= event.targetW &&
         event.localY >= 0.0f && event.localY <= event.targetH;
}

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

bool is_utf8_continuation(uint8_t value) {
  return (value & 0xC0u) == 0x80u;
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

#if defined(PRIMESTAGE_HAS_PRIMEMANIFEST)
PrimeManifest::Typography make_typography(PrimeFrame::Frame& frame, PrimeFrame::TextStyleToken token) {
  PrimeManifest::Typography typography;
  PrimeFrame::Theme const* theme = frame.getTheme(PrimeFrame::DefaultThemeId);
  if (!theme) {
    return typography;
  }
  PrimeFrame::ResolvedTextStyle resolved = PrimeFrame::resolveTextStyle(*theme, token, {});
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
  return typography;
}

void ensure_text_fonts_loaded() {
  static bool fontsLoaded = false;
  if (fontsLoaded) {
    return;
  }
  auto& registry = PrimeManifest::GetFontRegistry();
#if defined(PRIMESTAGE_HAS_BUNDLED_FONT) && PRIMESTAGE_HAS_BUNDLED_FONT
  registry.addBundleDir(PRIMESTAGE_BUNDLED_FONT_DIR);
#endif
  registry.loadBundledFonts();
  registry.loadOsFallbackFonts();
  fontsLoaded = true;
}
#endif

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

} // namespace

namespace Internal {

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
  ensure_text_fonts_loaded();
  auto& registry = PrimeManifest::GetFontRegistry();
  PrimeManifest::Typography typography = make_typography(frame, token);
  typography.lineHeight = resolved.lineHeight > 0.0f ? resolved.lineHeight : typography.lineHeight;
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

float textLineHeight(PrimeFrame::Frame& frame, PrimeFrame::TextStyleToken token) {
  return resolve_line_height(frame, token);
}

uint32_t utf8Prev(std::string_view text, uint32_t index) {
  if (index == 0u) {
    return 0u;
  }
  uint32_t size = static_cast<uint32_t>(text.size());
  uint32_t i = std::min(index, size);
  if (i > 0u) {
    --i;
  }
  while (i > 0u && is_utf8_continuation(static_cast<uint8_t>(text[i]))) {
    --i;
  }
  return i;
}

uint32_t utf8Next(std::string_view text, uint32_t index) {
  uint32_t size = static_cast<uint32_t>(text.size());
  if (index >= size) {
    return size;
  }
  uint32_t i = index + 1u;
  while (i < size && is_utf8_continuation(static_cast<uint8_t>(text[i]))) {
    ++i;
  }
  return i;
}

static bool is_word_char(std::string_view text, uint32_t index) {
  if (index >= text.size()) {
    return false;
  }
  unsigned char ch = static_cast<unsigned char>(text[index]);
  if (ch >= 0x80u) {
    return true;
  }
  return std::isalnum(ch) || ch == '_';
}

static bool is_space_char(std::string_view text, uint32_t index) {
  if (index >= text.size()) {
    return false;
  }
  unsigned char ch = static_cast<unsigned char>(text[index]);
  return std::isspace(ch) != 0;
}

static uint32_t prev_word_boundary(std::string_view text, uint32_t cursor) {
  if (cursor == 0u) {
    return 0u;
  }
  uint32_t i = utf8Prev(text, cursor);
  while (i > 0u && is_space_char(text, i)) {
    i = utf8Prev(text, i);
  }
  if (is_word_char(text, i)) {
    while (i > 0u) {
      uint32_t prev = utf8Prev(text, i);
      if (!is_word_char(text, prev)) {
        break;
      }
      i = prev;
    }
    return i;
  }
  while (i > 0u && !is_word_char(text, i)) {
    i = utf8Prev(text, i);
  }
  if (!is_word_char(text, i)) {
    return 0u;
  }
  while (i > 0u) {
    uint32_t prev = utf8Prev(text, i);
    if (!is_word_char(text, prev)) {
      break;
    }
    i = prev;
  }
  return i;
}

static uint32_t next_word_boundary(std::string_view text, uint32_t cursor) {
  uint32_t size = static_cast<uint32_t>(text.size());
  if (cursor >= size) {
    return size;
  }
  uint32_t i = cursor;
  if (is_word_char(text, i)) {
    while (i < size && is_word_char(text, i)) {
      i = utf8Next(text, i);
    }
    return i;
  }
  while (i < size && !is_word_char(text, i)) {
    i = utf8Next(text, i);
  }
  return i;
}

bool textFieldHasSelection(TextFieldState const& state,
                           uint32_t& start,
                           uint32_t& end) {
  start = std::min(state.selectionStart, state.selectionEnd);
  end = std::max(state.selectionStart, state.selectionEnd);
  return start != end;
}

void clearTextFieldSelection(TextFieldState& state, uint32_t cursor) {
  state.selectionAnchor = cursor;
  state.selectionStart = cursor;
  state.selectionEnd = cursor;
  state.selecting = false;
  state.pointerId = -1;
}

bool updateTextFieldBlink(TextFieldState& state,
                          std::chrono::steady_clock::time_point now,
                          std::chrono::milliseconds interval) {
  bool changed = false;
  if (state.focused) {
    if (state.nextBlink.time_since_epoch().count() == 0) {
      state.cursorVisible = true;
      state.nextBlink = now + interval;
      changed = true;
    } else if (now >= state.nextBlink) {
      state.cursorVisible = !state.cursorVisible;
      state.nextBlink = now + interval;
      changed = true;
    }
  } else if (state.cursorVisible || state.nextBlink.time_since_epoch().count() != 0) {
    state.cursorVisible = false;
    state.nextBlink = {};
    changed = true;
  }
  return changed;
}

bool selectableTextHasSelection(SelectableTextState const& state,
                                uint32_t& start,
                                uint32_t& end) {
  start = std::min(state.selectionStart, state.selectionEnd);
  end = std::max(state.selectionStart, state.selectionEnd);
  return start != end;
}

void clearSelectableTextSelection(SelectableTextState& state, uint32_t anchor) {
  state.selectionAnchor = anchor;
  state.selectionStart = anchor;
  state.selectionEnd = anchor;
  state.selecting = false;
  state.pointerId = -1;
}

std::vector<float> buildCaretPositions(PrimeFrame::Frame& frame,
                                       PrimeFrame::TextStyleToken token,
                                       std::string_view text) {
  std::vector<float> positions;
  if (text.empty()) {
    positions.resize(1, 0.0f);
    return positions;
  }

  positions.assign(text.size() + 1u, std::numeric_limits<float>::quiet_NaN());
  positions[0] = 0.0f;

#if defined(PRIMESTAGE_HAS_PRIMEMANIFEST)
  bool usedLayout = false;
  ensure_text_fonts_loaded();
  PrimeManifest::Typography typography = make_typography(frame, token);
  auto run = PrimeManifest::LayoutText(text, typography, 1.0f, false);
  if (run) {
    float penX = 0.0f;
    for (auto const& glyph : run->glyphs) {
      uint32_t cluster = std::min<uint32_t>(glyph.cluster, static_cast<uint32_t>(text.size()));
      if (!std::isfinite(positions[cluster])) {
        positions[cluster] = penX;
      }
      penX += glyph.advance;
    }
    positions[text.size()] = penX;
    usedLayout = true;
  }
#endif

  uint32_t index = utf8Next(text, 0u);
  while (index <= text.size()) {
#if defined(PRIMESTAGE_HAS_PRIMEMANIFEST)
    if (!usedLayout || !std::isfinite(positions[index])) {
      positions[index] = measureTextWidth(frame, token, text.substr(0, index));
    }
#else
    positions[index] = measureTextWidth(frame, token, text.substr(0, index));
#endif
    if (index == text.size()) {
      break;
    }
    index = utf8Next(text, index);
  }

  float last = positions[0];
  for (uint32_t i = 1u; i <= text.size(); ++i) {
    if (!std::isfinite(positions[i])) {
      positions[i] = last;
    } else {
      last = positions[i];
    }
  }

  return positions;
}

uint32_t caretIndexForClick(PrimeFrame::Frame& frame,
                            PrimeFrame::TextStyleToken token,
                            std::string_view text,
                            float paddingX,
                            float localX) {
  if (text.empty()) {
    return 0u;
  }
  float targetX = localX - paddingX;
  if (targetX <= 0.0f) {
    return 0u;
  }
  auto positions = buildCaretPositions(frame, token, text);
  float totalWidth = positions.back();
  if (targetX >= totalWidth) {
    return static_cast<uint32_t>(text.size());
  }
  uint32_t prevIndex = 0u;
  float prevWidth = positions[0];
  uint32_t index = utf8Next(text, 0u);
  while (index <= text.size()) {
    float width = positions[index];
    if (width >= targetX) {
      float prevDist = targetX - prevWidth;
      float nextDist = width - targetX;
      return (prevDist <= nextDist) ? prevIndex : index;
    }
    prevIndex = index;
    prevWidth = width;
    index = utf8Next(text, index);
  }
  return static_cast<uint32_t>(text.size());
}

std::vector<TextSelectionLine> wrapTextLineRanges(PrimeFrame::Frame& frame,
                                                  PrimeFrame::TextStyleToken token,
                                                  std::string_view text,
                                                  float maxWidth,
                                                  PrimeFrame::WrapMode wrap) {
  std::vector<TextSelectionLine> lines;
  if (text.empty()) {
    lines.push_back({0u, 0u, 0.0f});
    return lines;
  }
  if (maxWidth <= 0.0f || wrap == PrimeFrame::WrapMode::None) {
    uint32_t lineStart = 0u;
    for (uint32_t i = 0u; i < text.size(); ++i) {
      if (text[i] == '\n') {
        float width = measureTextWidth(frame, token, text.substr(lineStart, i - lineStart));
        lines.push_back({lineStart, i, width});
        lineStart = i + 1u;
      }
    }
    float width = measureTextWidth(frame, token, text.substr(lineStart, text.size() - lineStart));
    lines.push_back({lineStart, static_cast<uint32_t>(text.size()), width});
    return lines;
  }

  float spaceWidth = measureTextWidth(frame, token, " ");
  bool wrapByChar = wrap == PrimeFrame::WrapMode::Character;
  uint32_t i = 0u;
  uint32_t lineStart = 0u;
  uint32_t lineEnd = 0u;
  float lineWidth = 0.0f;
  bool lineHasWord = false;

  auto push_line = [&](uint32_t endIndex, float width) {
    lines.push_back({lineStart, endIndex, width});
    lineStart = endIndex;
    lineEnd = endIndex;
    lineWidth = 0.0f;
    lineHasWord = false;
  };

  while (i < text.size()) {
    char ch = text[i];
    if (ch == '\n') {
      push_line(lineHasWord ? lineEnd : i, lineWidth);
      ++i;
      lineStart = i;
      continue;
    }
    if (std::isspace(static_cast<unsigned char>(ch))) {
      ++i;
      continue;
    }
    uint32_t wordStart = i;
    if (wrapByChar) {
      i = utf8Next(text, i);
    } else {
      while (i < text.size()) {
        char wordCh = text[i];
        if (wordCh == '\n' || std::isspace(static_cast<unsigned char>(wordCh))) {
          break;
        }
        ++i;
      }
    }
    uint32_t wordEnd = i;
    if (wordEnd <= wordStart) {
      ++i;
      continue;
    }
    float wordWidth = measureTextWidth(frame, token, text.substr(wordStart, wordEnd - wordStart));
    if (lineHasWord && lineWidth + spaceWidth + wordWidth > maxWidth) {
      push_line(lineEnd, lineWidth);
    }
    if (!lineHasWord) {
      lineStart = wordStart;
      lineEnd = wordEnd;
      lineWidth = wordWidth;
      lineHasWord = true;
    } else {
      lineEnd = wordEnd;
      lineWidth += spaceWidth + wordWidth;
    }
  }
  if (lineHasWord) {
    push_line(lineEnd, lineWidth);
  } else if (lineStart < text.size()) {
    lines.push_back({lineStart, static_cast<uint32_t>(text.size()), 0.0f});
  }
  if (lines.empty()) {
    lines.push_back({0u, static_cast<uint32_t>(text.size()), 0.0f});
  }
  return lines;
}

TextSelectionLayout buildTextSelectionLayout(PrimeFrame::Frame& frame,
                                             PrimeFrame::TextStyleToken token,
                                             std::string_view text,
                                             float maxWidth,
                                             PrimeFrame::WrapMode wrap) {
  TextSelectionLayout layout;
  layout.lines = wrapTextLineRanges(frame, token, text, maxWidth, wrap);
  layout.lineHeight = textLineHeight(frame, token);
  if (layout.lineHeight <= 0.0f) {
    layout.lineHeight = 1.0f;
  }
  return layout;
}

std::vector<TextSelectionRect> buildSelectionRects(PrimeFrame::Frame& frame,
                                                   PrimeFrame::TextStyleToken token,
                                                   std::string_view text,
                                                   TextSelectionLayout const& layout,
                                                   uint32_t selectionStart,
                                                   uint32_t selectionEnd,
                                                   float paddingX) {
  std::vector<TextSelectionRect> rects;
  if (text.empty() || layout.lines.empty() || selectionStart == selectionEnd) {
    return rects;
  }
  uint32_t textSize = static_cast<uint32_t>(text.size());
  uint32_t selStart = std::min(selectionStart, selectionEnd);
  uint32_t selEnd = std::max(selectionStart, selectionEnd);
  selStart = std::min(selStart, textSize);
  selEnd = std::min(selEnd, textSize);
  if (selStart >= selEnd) {
    return rects;
  }
  for (size_t lineIndex = 0; lineIndex < layout.lines.size(); ++lineIndex) {
    const auto& line = layout.lines[lineIndex];
    if (selEnd <= line.start || selStart >= line.end) {
      continue;
    }
    uint32_t localStart = std::max(selStart, line.start) - line.start;
    uint32_t localEnd = std::min(selEnd, line.end) - line.start;
    std::string_view lineText(text.data() + line.start, line.end - line.start);
    auto caretPositions = buildCaretPositions(frame, token, lineText);
    uint32_t maxIndex = static_cast<uint32_t>(lineText.size());
    localStart = std::min(localStart, maxIndex);
    localEnd = std::min(localEnd, maxIndex);
    float leftWidth = caretPositions[localStart];
    float rightWidth = caretPositions[localEnd];
    float width = rightWidth - leftWidth;
    if (width <= 0.0f) {
      continue;
    }
    TextSelectionRect rect;
    rect.x = paddingX + leftWidth;
    rect.y = static_cast<float>(lineIndex) * layout.lineHeight;
    rect.width = width;
    rect.height = layout.lineHeight;
    rects.push_back(rect);
  }
  return rects;
}

uint32_t caretIndexForClickInLayout(PrimeFrame::Frame& frame,
                                    PrimeFrame::TextStyleToken token,
                                    std::string_view text,
                                    TextSelectionLayout const& layout,
                                    float paddingX,
                                    float localX,
                                    float localY) {
  if (layout.lines.empty() || layout.lineHeight <= 0.0f) {
    return caretIndexForClick(frame, token, text, paddingX, localX);
  }
  float lineHeight = layout.lineHeight;
  int lineIndex = static_cast<int>(localY / lineHeight);
  lineIndex = std::clamp(lineIndex, 0, static_cast<int>(layout.lines.size() - 1));
  const auto& line = layout.lines[static_cast<size_t>(lineIndex)];
  std::string_view lineText(text.data() + line.start, line.end - line.start);
  uint32_t localIndex = caretIndexForClick(frame, token, lineText, paddingX, localX);
  return line.start + localIndex;
}

struct CallbackReentryScope {
  explicit CallbackReentryScope(std::shared_ptr<bool> state)
      : state_(std::move(state)) {
    if (!state_ || *state_) {
      return;
    }
    *state_ = true;
    entered_ = true;
  }

  ~CallbackReentryScope() {
    if (entered_ && state_) {
      *state_ = false;
    }
  }

  bool entered() const { return entered_; }

private:
  std::shared_ptr<bool> state_;
  bool entered_ = false;
};

void report_callback_reentry(char const* callbackName) {
#if !defined(NDEBUG)
  std::fprintf(stderr,
               "PrimeStage callback guard: reentrant %s invocation suppressed\n",
               callbackName);
#else
  (void)callbackName;
#endif
}

LowLevel::NodeCallbackHandle::NodeCallbackHandle(PrimeFrame::Frame& frame,
                                                 PrimeFrame::NodeId nodeId,
                                                 LowLevel::NodeCallbackTable callbackTable) {
  bind(frame, nodeId, std::move(callbackTable));
}

LowLevel::NodeCallbackHandle::NodeCallbackHandle(NodeCallbackHandle&& other) noexcept
    : frame_(other.frame_),
      nodeId_(other.nodeId_),
      previousCallbackId_(other.previousCallbackId_),
      active_(other.active_) {
  other.frame_ = nullptr;
  other.nodeId_ = PrimeFrame::NodeId{};
  other.previousCallbackId_ = PrimeFrame::InvalidCallbackId;
  other.active_ = false;
}

LowLevel::NodeCallbackHandle& LowLevel::NodeCallbackHandle::operator=(
    NodeCallbackHandle&& other) noexcept {
  if (this == &other) {
    return *this;
  }
  reset();
  frame_ = other.frame_;
  nodeId_ = other.nodeId_;
  previousCallbackId_ = other.previousCallbackId_;
  active_ = other.active_;
  other.frame_ = nullptr;
  other.nodeId_ = PrimeFrame::NodeId{};
  other.previousCallbackId_ = PrimeFrame::InvalidCallbackId;
  other.active_ = false;
  return *this;
}

LowLevel::NodeCallbackHandle::~NodeCallbackHandle() {
  reset();
}

bool LowLevel::NodeCallbackHandle::bind(PrimeFrame::Frame& frame,
                                        PrimeFrame::NodeId nodeId,
                                        LowLevel::NodeCallbackTable callbackTable) {
  reset();
  PrimeFrame::Node* node = frame.getNode(nodeId);
  if (!node) {
    return false;
  }
  previousCallbackId_ = node->callbacks;
  PrimeFrame::Callback callback;
  callback.onEvent = std::move(callbackTable.onEvent);
  callback.onFocus = std::move(callbackTable.onFocus);
  callback.onBlur = std::move(callbackTable.onBlur);
  node->callbacks = frame.addCallback(std::move(callback));
  frame_ = &frame;
  nodeId_ = nodeId;
  active_ = true;
  return true;
}

void LowLevel::NodeCallbackHandle::reset() {
  if (!active_ || !frame_) {
    frame_ = nullptr;
    nodeId_ = PrimeFrame::NodeId{};
    previousCallbackId_ = PrimeFrame::InvalidCallbackId;
    active_ = false;
    return;
  }
  if (PrimeFrame::Node* node = frame_->getNode(nodeId_)) {
    node->callbacks = previousCallbackId_;
  }
  frame_ = nullptr;
  nodeId_ = PrimeFrame::NodeId{};
  previousCallbackId_ = PrimeFrame::InvalidCallbackId;
  active_ = false;
}

static PrimeFrame::Callback* ensureNodeCallback(PrimeFrame::Frame& frame, PrimeFrame::NodeId nodeId) {
  PrimeFrame::Node* node = frame.getNode(nodeId);
  if (!node) {
    return nullptr;
  }
  if (node->callbacks == PrimeFrame::InvalidCallbackId) {
    PrimeFrame::Callback callback;
    node->callbacks = frame.addCallback(std::move(callback));
  }
  PrimeFrame::Callback* result = frame.getCallback(node->callbacks);
  if (result) {
    return result;
  }
  PrimeFrame::Callback callback;
  node->callbacks = frame.addCallback(std::move(callback));
  return frame.getCallback(node->callbacks);
}

bool LowLevel::appendNodeOnEvent(PrimeFrame::Frame& frame,
                                 PrimeFrame::NodeId nodeId,
                                 std::function<bool(PrimeFrame::Event const&)> onEvent) {
  if (!onEvent) {
    return false;
  }
  PrimeFrame::Callback* callback = ensureNodeCallback(frame, nodeId);
  if (!callback) {
    return false;
  }
  auto previous = callback->onEvent;
  std::shared_ptr<bool> reentryState = std::make_shared<bool>(false);
  callback->onEvent = [handler = std::move(onEvent),
                       previous = std::move(previous),
                       reentryState = std::move(reentryState)](
                          PrimeFrame::Event const& event) -> bool {
    CallbackReentryScope reentryGuard(reentryState);
    if (!reentryGuard.entered()) {
      report_callback_reentry("onEvent");
      return false;
    }
    if (handler && handler(event)) {
      return true;
    }
    if (previous) {
      return previous(event);
    }
    return false;
  };
  return true;
}

bool LowLevel::appendNodeOnFocus(PrimeFrame::Frame& frame,
                                 PrimeFrame::NodeId nodeId,
                                 std::function<void()> onFocus) {
  if (!onFocus) {
    return false;
  }
  PrimeFrame::Callback* callback = ensureNodeCallback(frame, nodeId);
  if (!callback) {
    return false;
  }
  auto previous = callback->onFocus;
  std::shared_ptr<bool> reentryState = std::make_shared<bool>(false);
  callback->onFocus = [handler = std::move(onFocus),
                       previous = std::move(previous),
                       reentryState = std::move(reentryState)]() {
    CallbackReentryScope reentryGuard(reentryState);
    if (!reentryGuard.entered()) {
      report_callback_reentry("onFocus");
      return;
    }
    if (previous) {
      previous();
    }
    if (handler) {
      handler();
    }
  };
  return true;
}

bool LowLevel::appendNodeOnBlur(PrimeFrame::Frame& frame,
                                PrimeFrame::NodeId nodeId,
                                std::function<void()> onBlur) {
  if (!onBlur) {
    return false;
  }
  PrimeFrame::Callback* callback = ensureNodeCallback(frame, nodeId);
  if (!callback) {
    return false;
  }
  auto previous = callback->onBlur;
  std::shared_ptr<bool> reentryState = std::make_shared<bool>(false);
  callback->onBlur = [handler = std::move(onBlur),
                      previous = std::move(previous),
                      reentryState = std::move(reentryState)]() {
    CallbackReentryScope reentryGuard(reentryState);
    if (!reentryGuard.entered()) {
      report_callback_reentry("onBlur");
      return;
    }
    if (previous) {
      previous();
    }
    if (handler) {
      handler();
    }
  };
  return true;
}

void WidgetIdentityReconciler::beginRebuild(PrimeFrame::NodeId focusedNode) {
  pendingFocusedIdentityId_.reset();
  if (focusedNode.isValid()) {
    for (Entry const& entry : currentEntries_) {
      if (entry.nodeId == focusedNode) {
        pendingFocusedIdentityId_ = entry.identityId;
        break;
      }
    }
  }
  currentEntries_.clear();
}

void WidgetIdentityReconciler::registerNode(WidgetIdentityId identity, PrimeFrame::NodeId nodeId) {
  if (!nodeId.isValid() || identity == InvalidWidgetIdentityId) {
    return;
  }
  for (Entry& entry : currentEntries_) {
    if (entry.identityId == identity) {
      entry.nodeId = nodeId;
      return;
    }
  }
  Entry entry;
  entry.identityId = identity;
  entry.nodeId = nodeId;
  currentEntries_.push_back(std::move(entry));
}

void WidgetIdentityReconciler::registerNode(std::string_view identity, PrimeFrame::NodeId nodeId) {
  WidgetIdentityId identityValue = widgetIdentityId(identity);
  registerNode(identityValue, nodeId);
  if (!nodeId.isValid() || identity.empty() || identityValue == InvalidWidgetIdentityId) {
    return;
  }
  for (Entry& entry : currentEntries_) {
    if (entry.identityId == identityValue) {
      entry.identity = std::string(identity);
      entry.nodeId = nodeId;
      return;
    }
  }
  Entry entry;
  entry.identityId = identityValue;
  entry.identity = std::string(identity);
  entry.nodeId = nodeId;
  currentEntries_.push_back(std::move(entry));
}

PrimeFrame::NodeId WidgetIdentityReconciler::findNode(WidgetIdentityId identity) const {
  if (identity == InvalidWidgetIdentityId) {
    return PrimeFrame::NodeId{};
  }
  for (Entry const& entry : currentEntries_) {
    if (entry.identityId == identity) {
      return entry.nodeId;
    }
  }
  return PrimeFrame::NodeId{};
}

PrimeFrame::NodeId WidgetIdentityReconciler::findNode(std::string_view identity) const {
  WidgetIdentityId identityValue = widgetIdentityId(identity);
  if (identityValue == InvalidWidgetIdentityId) {
    return PrimeFrame::NodeId{};
  }
  for (Entry const& entry : currentEntries_) {
    if (entry.identityId != identityValue) {
      continue;
    }
    if (entry.identity.empty() || entry.identity == identity) {
      return entry.nodeId;
    }
  }
  return PrimeFrame::NodeId{};
}

bool WidgetIdentityReconciler::restoreFocus(PrimeFrame::FocusManager& focus,
                                            PrimeFrame::Frame const& frame,
                                            PrimeFrame::LayoutOutput const& layout) {
  if (!pendingFocusedIdentityId_.has_value()) {
    return false;
  }
  PrimeFrame::NodeId nodeId = findNode(*pendingFocusedIdentityId_);
  pendingFocusedIdentityId_.reset();
  if (!nodeId.isValid()) {
    return false;
  }
  return focus.setFocus(frame, layout, nodeId);
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

UiNode UiNode::createPanel(PanelSpec const& specInput) {
  PanelSpec spec = specInput;
  sanitize_size_spec(spec.size, "PanelSpec.size");
  spec.padding = sanitize_insets(spec.padding, "PanelSpec");
  spec.gap = clamp_non_negative(spec.gap, "PanelSpec", "gap");

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


UiNode UiNode::createLabel(LabelSpec const& specInput) {
  LabelSpec spec = specInput;
  sanitize_size_spec(spec.size, "LabelSpec.size");
  spec.maxWidth = clamp_non_negative(spec.maxWidth, "LabelSpec", "maxWidth");
  apply_default_accessibility_semantics(spec.accessibility, AccessibilityRole::StaticText, true);

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


UiNode UiNode::createParagraph(ParagraphSpec const& specInput) {
  ParagraphSpec spec = specInput;
  sanitize_size_spec(spec.size, "ParagraphSpec.size");
  spec.maxWidth = clamp_non_negative(spec.maxWidth, "ParagraphSpec", "maxWidth");
  apply_default_accessibility_semantics(spec.accessibility, AccessibilityRole::StaticText, true);

  Rect bounds = resolve_rect(spec.size);
  PrimeFrame::TextStyleToken token = spec.textStyle;
  float maxWidth = spec.maxWidth > 0.0f ? spec.maxWidth : bounds.width;
  if (maxWidth <= 0.0f && spec.size.maxWidth.has_value()) {
    maxWidth = std::max(0.0f, *spec.size.maxWidth);
  }
  if (maxWidth <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f &&
      !spec.text.empty()) {
    maxWidth = DefaultParagraphWrapWidth;
  }
  if (bounds.width <= 0.0f &&
      maxWidth > 0.0f &&
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
    if (maxWidth > 0.0f) {
      inferredWidth = std::min(inferredWidth, maxWidth);
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

UiNode UiNode::createTextSelectionOverlay(TextSelectionOverlaySpec const& spec) {
  Rect bounds = resolve_rect(spec.size);
  float maxWidth = spec.maxWidth;
  if (maxWidth <= 0.0f && bounds.width > 0.0f) {
    maxWidth = bounds.width;
  }

  TextSelectionLayout computedLayout;
  TextSelectionLayout const* layout = spec.layout;
  if (!layout) {
    computedLayout = buildTextSelectionLayout(frame(), spec.textStyle, spec.text, maxWidth, spec.wrap);
    layout = &computedLayout;
  }

  float lineHeight = layout->lineHeight > 0.0f ? layout->lineHeight : textLineHeight(frame(), spec.textStyle);
  if (lineHeight <= 0.0f) {
    lineHeight = 1.0f;
  }
  size_t lineCount = std::max<size_t>(1u, layout->lines.size());

  float inferredWidth = bounds.width;
  if (inferredWidth <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f) {
    for (auto const& line : layout->lines) {
      inferredWidth = std::max(inferredWidth, line.width);
    }
  }
  float inferredHeight = bounds.height;
  if (inferredHeight <= 0.0f &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchY <= 0.0f) {
    inferredHeight = lineHeight * static_cast<float>(lineCount);
  }

  StackSpec columnSpec;
  columnSpec.size = spec.size;
  if (!columnSpec.size.preferredWidth.has_value() && inferredWidth > 0.0f) {
    columnSpec.size.preferredWidth = inferredWidth;
  }
  if (!columnSpec.size.preferredHeight.has_value() && inferredHeight > 0.0f) {
    columnSpec.size.preferredHeight = inferredHeight;
  }
  columnSpec.gap = 0.0f;
  columnSpec.clipChildren = spec.clipChildren;
  columnSpec.visible = spec.visible;
  UiNode column = createVerticalStack(columnSpec);
  column.setHitTestVisible(false);

  if (spec.selectionStyle == 0 || spec.selectionStart == spec.selectionEnd || spec.text.empty()) {
    return column;
  }

  auto selectionRects = buildSelectionRects(frame(),
                                            spec.textStyle,
                                            spec.text,
                                            *layout,
                                            spec.selectionStart,
                                            spec.selectionEnd,
                                            spec.paddingX);
  if (selectionRects.empty()) {
    return column;
  }

  size_t rectIndex = 0u;
  float rowWidth = columnSpec.size.preferredWidth.value_or(inferredWidth);

  for (size_t lineIndex = 0; lineIndex < lineCount; ++lineIndex) {
    StackSpec lineSpec;
    if (rowWidth > 0.0f) {
      lineSpec.size.preferredWidth = rowWidth;
    } else {
      lineSpec.size.stretchX = 1.0f;
    }
    lineSpec.size.preferredHeight = lineHeight;
    lineSpec.gap = 0.0f;
    UiNode lineRow = column.createHorizontalStack(lineSpec);
    lineRow.setHitTestVisible(false);

    float leftWidth = 0.0f;
    float selectWidth = 0.0f;
    if (rectIndex < selectionRects.size()) {
      const auto& rect = selectionRects[rectIndex];
      float lineY = static_cast<float>(lineIndex) * lineHeight;
      if (std::abs(rect.y - lineY) <= 0.5f) {
        leftWidth = rect.x;
        selectWidth = rect.width;
        rectIndex++;
      }
    }

    if (leftWidth > 0.0f) {
      SizeSpec leftSize;
      leftSize.preferredWidth = leftWidth;
      leftSize.preferredHeight = lineHeight;
      lineRow.createSpacer(leftSize);
    }
    if (selectWidth > 0.0f) {
      SizeSpec selectSize;
      selectSize.preferredWidth = selectWidth;
      selectSize.preferredHeight = lineHeight;
      PanelSpec selectSpec;
      selectSpec.rectStyle = spec.selectionStyle;
      selectSpec.rectStyleOverride = spec.selectionStyleOverride;
      selectSpec.size = selectSize;
      UiNode selectPanel = lineRow.createPanel(selectSpec);
      selectPanel.setHitTestVisible(false);
    }
    SizeSpec fillSize;
    fillSize.stretchX = 1.0f;
    fillSize.preferredHeight = lineHeight;
    lineRow.createSpacer(fillSize);
  }

  return column;
}


UiNode UiNode::createTextLine(TextLineSpec const& specInput) {
  TextLineSpec spec = specInput;
  apply_default_accessibility_semantics(spec.accessibility, AccessibilityRole::StaticText, true);
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



UiNode UiNode::createDivider(DividerSpec const& specInput) {
  DividerSpec spec = specInput;
  sanitize_size_spec(spec.size, "DividerSpec.size");

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

UiNode UiNode::createSpacer(SpacerSpec const& specInput) {
  SpacerSpec spec = specInput;
  sanitize_size_spec(spec.size, "SpacerSpec.size");

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

UiNode UiNode::createButton(ButtonSpec const& specInput) {
  ButtonSpec spec = specInput;
  sanitize_size_spec(spec.size, "ButtonSpec.size");
  spec.textInsetX = clamp_non_negative(spec.textInsetX, "ButtonSpec", "textInsetX");
  spec.baseOpacity = clamp_unit_interval(spec.baseOpacity, "ButtonSpec", "baseOpacity");
  spec.hoverOpacity = clamp_unit_interval(spec.hoverOpacity, "ButtonSpec", "hoverOpacity");
  spec.pressedOpacity = clamp_unit_interval(spec.pressedOpacity, "ButtonSpec", "pressedOpacity");
  spec.tabIndex = clamp_tab_index(spec.tabIndex, "ButtonSpec", "tabIndex");
  bool enabled = spec.enabled;
  apply_default_accessibility_semantics(spec.accessibility, AccessibilityRole::Button, enabled);

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
  bool needsInteraction = enabled &&
                          (spec.callbacks.onActivate ||
                          spec.callbacks.onClick ||
                          spec.callbacks.onHoverChanged ||
                          spec.callbacks.onPressedChanged ||
                          hoverToken != baseToken ||
                          pressedToken != baseToken ||
                          std::abs(spec.hoverOpacity - spec.baseOpacity) > 0.001f ||
                          std::abs(spec.pressedOpacity - spec.baseOpacity) > 0.001f);

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
  if (!spec.visible) {
    return UiNode(frame(), button.nodeId(), allowAbsolute_);
  }

  if (!spec.label.empty()) {
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
  }

  std::optional<FocusOverlay> focusOverlay;
  if (spec.visible && enabled) {
    ResolvedFocusStyle focusStyle = resolve_focus_style(
        frame(),
        spec.focusStyle,
        spec.focusStyleOverride,
        {pressedToken, hoverToken, baseToken},
        spec.backgroundStyleOverride);
    Rect focusRect{0.0f, 0.0f, bounds.width, bounds.height};
    focusOverlay = add_focus_overlay_node(frame(),
                                          button.nodeId(),
                                          focusRect,
                                          focusStyle.token,
                                          focusStyle.overrideStyle,
                                          spec.visible);
    if (PrimeFrame::Node* node = frame().getNode(button.nodeId())) {
      node->focusable = true;
    }
  }

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
            bool inside = is_pointer_inside(event);
            update(inside, inside);
            return true;
          }
          case PrimeFrame::EventType::PointerUp: {
            bool inside = is_pointer_inside(event);
            bool fire = state->pressed && inside;
            update(false, inside);
            if (fire) {
              if (callbacks.onActivate) {
                callbacks.onActivate();
              } else if (callbacks.onClick) {
                callbacks.onClick();
              }
            }
            return true;
          }
          case PrimeFrame::EventType::PointerCancel:
            update(false, false);
            return true;
          case PrimeFrame::EventType::PointerMove: {
            bool inside = is_pointer_inside(event);
            update(state->pressed && inside, inside);
            return true;
          }
          case PrimeFrame::EventType::KeyDown:
            if (is_activation_key(event.key)) {
              if (callbacks.onPressedChanged) {
                callbacks.onPressedChanged(true);
                callbacks.onPressedChanged(false);
              }
              if (callbacks.onActivate) {
                callbacks.onActivate();
              } else if (callbacks.onClick) {
                callbacks.onClick();
              }
              return true;
            }
            break;
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

  if (PrimeFrame::Node* node = frame().getNode(button.nodeId())) {
    node->focusable = enabled;
    node->hitTestVisible = enabled;
    node->tabIndex = enabled ? spec.tabIndex : -1;
  }

  if (focusOverlay.has_value()) {
    attach_focus_callbacks(frame(), button.nodeId(), *focusOverlay);
  }

  if (!enabled) {
    add_state_scrim_overlay(frame(),
                            button.nodeId(),
                            Rect{0.0f, 0.0f, bounds.width, bounds.height},
                            DisabledScrimOpacity,
                            spec.visible);
  }

  return UiNode(frame(), button.nodeId(), allowAbsolute_);
}

UiNode UiNode::createButton(std::string_view label,
                            PrimeFrame::RectStyleToken backgroundStyle,
                            PrimeFrame::TextStyleToken textStyle,
                            SizeSpec const& size) {
  ButtonSpec spec;
  spec.label = label;
  spec.backgroundStyle = backgroundStyle;
  spec.textStyle = textStyle;
  spec.size = size;
  return createButton(spec);
}

UiNode UiNode::createTextField(TextFieldSpec const& specInput) {
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
  bool enabled = spec.enabled;
  bool readOnly = spec.readOnly;
  apply_default_accessibility_semantics(spec.accessibility, AccessibilityRole::TextField, enabled);

  Rect bounds = resolve_rect(spec.size);
  std::shared_ptr<TextFieldState> stateOwner = spec.ownedState;
  TextFieldState* state = spec.state;
  if (!state) {
    if (!stateOwner) {
      stateOwner = std::make_shared<TextFieldState>();
    }
    if (text_field_state_is_pristine(*stateOwner)) {
      seed_text_field_state_from_spec(*stateOwner, spec);
    }
    state = stateOwner.get();
  }
  std::string_view previewText = std::string_view(state->text);
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

  if (!spec.visible) {
    return UiNode(frame(), field.nodeId(), allowAbsolute_);
  }

  std::string_view activeText = std::string_view(state->text);
  uint32_t textSize = static_cast<uint32_t>(activeText.size());
  uint32_t cursorIndex = state ? state->cursor : spec.cursorIndex;
  uint32_t selectionAnchor = state ? state->selectionAnchor : cursorIndex;
  uint32_t selectionStart = state ? state->selectionStart : spec.selectionStart;
  uint32_t selectionEnd = state ? state->selectionEnd : spec.selectionEnd;
  cursorIndex = clamp_text_index(cursorIndex, textSize, "TextFieldSpec", "cursor");
  selectionAnchor =
      clamp_text_index(selectionAnchor, textSize, "TextFieldSpec", "selectionAnchor");
  selectionStart =
      clamp_text_index(selectionStart, textSize, "TextFieldSpec", "selectionStart");
  selectionEnd = clamp_text_index(selectionEnd, textSize, "TextFieldSpec", "selectionEnd");
  if (state && enabled) {
    state->cursor = cursorIndex;
    state->selectionAnchor = selectionAnchor;
    state->selectionStart = selectionStart;
    state->selectionEnd = selectionEnd;
  }

  std::string_view content = std::string_view(state->text);
  PrimeFrame::TextStyleToken style = spec.textStyle;
  PrimeFrame::TextStyleOverride overrideStyle = spec.textStyleOverride;
  if (content.empty() && spec.showPlaceholderWhenEmpty) {
    content = spec.placeholder;
    style = spec.placeholderStyle;
    overrideStyle = spec.placeholderStyleOverride;
  }

  lineHeight = resolve_line_height(frame(), style);
  if (lineHeight <= 0.0f && style != spec.textStyle) {
    lineHeight = resolve_line_height(frame(), spec.textStyle);
  }
  float textY = (bounds.height - lineHeight) * 0.5f + spec.textOffsetY;
  float textWidth = std::max(0.0f, bounds.width - spec.paddingX * 2.0f);
  bool showCursor = state ? (state->focused && state->cursorVisible) : spec.showCursor;

  std::vector<float> initialCaretPositions;
  if (!activeText.empty() && (showCursor || selectionStart != selectionEnd)) {
    initialCaretPositions = buildCaretPositions(frame(), spec.textStyle, activeText);
  }
  auto initialCaretAdvanceFor = [&](uint32_t index) -> float {
    if (initialCaretPositions.empty()) {
      return 0.0f;
    }
    uint32_t clamped = std::min(index, textSize);
    return initialCaretPositions[clamped];
  };

  Rect initialSelectionRect{spec.paddingX, textY, 0.0f, std::max(0.0f, lineHeight)};
  bool initialSelectionVisible = false;
  uint32_t selStart = std::min(selectionStart, selectionEnd);
  uint32_t selEnd = std::max(selectionStart, selectionEnd);
  if (selStart < selEnd && !activeText.empty() && spec.selectionStyle != 0) {
    float startAdvance = initialCaretAdvanceFor(selStart);
    float endAdvance = initialCaretAdvanceFor(selEnd);
    float startX = spec.paddingX + startAdvance;
    float endX = spec.paddingX + endAdvance;
    float maxX = bounds.width - spec.paddingX;
    if (maxX < spec.paddingX) {
      maxX = spec.paddingX;
    }
    startX = std::clamp(startX, spec.paddingX, maxX);
    endX = std::clamp(endX, spec.paddingX, maxX);
    if (endX > startX) {
      initialSelectionRect.x = startX;
      initialSelectionRect.width = endX - startX;
      initialSelectionVisible = true;
    }
  }

  Rect initialCursorRect{spec.paddingX, textY, 0.0f, std::max(0.0f, lineHeight)};
  bool initialCursorVisible = false;
  if (showCursor && spec.cursorStyle != 0) {
    float cursorAdvance = initialCaretAdvanceFor(cursorIndex);
    float cursorX = spec.paddingX + cursorAdvance;
    float maxX = bounds.width - spec.paddingX - spec.cursorWidth;
    if (maxX < spec.paddingX) {
      maxX = spec.paddingX;
    }
    if (cursorX > maxX) {
      cursorX = maxX;
    }
    initialCursorRect.x = cursorX;
    initialCursorRect.width = spec.cursorWidth;
    initialCursorVisible = initialCursorRect.width > 0.0f && initialCursorRect.height > 0.0f;
  }

  PrimeFrame::NodeId selectionNodeId{};
  PrimeFrame::PrimitiveId selectionPrim{};
  if (spec.selectionStyle != 0) {
    selectionNodeId = create_rect_node(frame(),
                                       field.nodeId(),
                                       initialSelectionRect,
                                       spec.selectionStyle,
                                       spec.selectionStyleOverride,
                                       false,
                                       spec.visible);
    if (PrimeFrame::Node* selectionNode = frame().getNode(selectionNodeId);
        selectionNode && !selectionNode->primitives.empty()) {
      selectionPrim = selectionNode->primitives.front();
      selectionNode->visible = initialSelectionVisible;
    }
  }

  Rect textRect{spec.paddingX, textY, textWidth, std::max(0.0f, lineHeight)};
  PrimeFrame::NodeId textNodeId = create_text_node(frame(),
                                                   field.nodeId(),
                                                   textRect,
                                                   content,
                                                   style,
                                                   overrideStyle,
                                                   PrimeFrame::TextAlign::Start,
                                                   PrimeFrame::WrapMode::None,
                                                   textWidth,
                                                   spec.visible);
  PrimeFrame::PrimitiveId textPrim{};
  if (PrimeFrame::Node* textNode = frame().getNode(textNodeId);
      textNode && !textNode->primitives.empty()) {
    textPrim = textNode->primitives.front();
  }

  PrimeFrame::NodeId cursorNodeId{};
  PrimeFrame::PrimitiveId cursorPrim{};
  if (spec.cursorStyle != 0) {
    cursorNodeId = create_rect_node(frame(),
                                    field.nodeId(),
                                    initialCursorRect,
                                    spec.cursorStyle,
                                    spec.cursorStyleOverride,
                                    false,
                                    spec.visible);
    if (PrimeFrame::Node* cursorNode = frame().getNode(cursorNodeId);
        cursorNode && !cursorNode->primitives.empty()) {
      cursorPrim = cursorNode->primitives.front();
      cursorNode->visible = initialCursorVisible;
    }
  }

  struct TextFieldPatchState {
    PrimeFrame::Frame* frame = nullptr;
    TextFieldState* state = nullptr;
    PrimeFrame::NodeId textNode{};
    PrimeFrame::PrimitiveId textPrim{};
    PrimeFrame::NodeId selectionNode{};
    PrimeFrame::PrimitiveId selectionPrim{};
    PrimeFrame::NodeId cursorNode{};
    PrimeFrame::PrimitiveId cursorPrim{};
    std::string placeholderText;
    float width = 0.0f;
    float height = 0.0f;
    float paddingX = 0.0f;
    float textOffsetY = 0.0f;
    float cursorWidth = 0.0f;
    bool showPlaceholderWhenEmpty = false;
    PrimeFrame::TextStyleToken textStyle = 0;
    PrimeFrame::TextStyleOverride textStyleOverride{};
    PrimeFrame::TextStyleToken placeholderStyle = 0;
    PrimeFrame::TextStyleOverride placeholderStyleOverride{};
  };

  auto patchState = std::make_shared<TextFieldPatchState>();
  patchState->frame = &frame();
  patchState->state = state;
  patchState->textNode = textNodeId;
  patchState->textPrim = textPrim;
  patchState->selectionNode = selectionNodeId;
  patchState->selectionPrim = selectionPrim;
  patchState->cursorNode = cursorNodeId;
  patchState->cursorPrim = cursorPrim;
  patchState->placeholderText = std::string(spec.placeholder);
  patchState->width = bounds.width;
  patchState->height = bounds.height;
  patchState->paddingX = spec.paddingX;
  patchState->textOffsetY = spec.textOffsetY;
  patchState->cursorWidth = spec.cursorWidth;
  patchState->showPlaceholderWhenEmpty = spec.showPlaceholderWhenEmpty;
  patchState->textStyle = spec.textStyle;
  patchState->textStyleOverride = spec.textStyleOverride;
  patchState->placeholderStyle = spec.placeholderStyle;
  patchState->placeholderStyleOverride = spec.placeholderStyleOverride;

  auto patchTextFieldVisuals = [patchState, stateOwner]() {
    (void)stateOwner;
    if (!patchState || !patchState->frame || !patchState->state) {
      return;
    }

    PrimeFrame::Frame& frameRef = *patchState->frame;
    TextFieldState* stateRef = patchState->state;
    uint32_t textSize = static_cast<uint32_t>(stateRef->text.size());
    stateRef->cursor = std::min(stateRef->cursor, textSize);
    stateRef->selectionAnchor = std::min(stateRef->selectionAnchor, textSize);
    stateRef->selectionStart = std::min(stateRef->selectionStart, textSize);
    stateRef->selectionEnd = std::min(stateRef->selectionEnd, textSize);

    std::string_view activeText = stateRef->text;
    std::string_view renderedText = activeText;
    PrimeFrame::TextStyleToken renderedStyle = patchState->textStyle;
    PrimeFrame::TextStyleOverride renderedOverride = patchState->textStyleOverride;
    if (renderedText.empty() && patchState->showPlaceholderWhenEmpty) {
      renderedText = patchState->placeholderText;
      renderedStyle = patchState->placeholderStyle;
      renderedOverride = patchState->placeholderStyleOverride;
    }

    float lineHeight = resolve_line_height(frameRef, renderedStyle);
    if (lineHeight <= 0.0f && renderedStyle != patchState->textStyle) {
      lineHeight = resolve_line_height(frameRef, patchState->textStyle);
    }
    lineHeight = std::max(0.0f, lineHeight);
    float textY = (patchState->height - lineHeight) * 0.5f + patchState->textOffsetY;
    float textWidth = std::max(0.0f, patchState->width - patchState->paddingX * 2.0f);

    if (PrimeFrame::Node* textNode = frameRef.getNode(patchState->textNode)) {
      textNode->localX = patchState->paddingX;
      textNode->localY = textY;
      textNode->visible = true;
      textNode->sizeHint.width.preferred = textWidth;
      textNode->sizeHint.height.preferred = lineHeight;
    }
    if (PrimeFrame::Primitive* textPrim = frameRef.getPrimitive(patchState->textPrim)) {
      textPrim->width = textWidth;
      textPrim->height = lineHeight;
      textPrim->textBlock.text = std::string(renderedText);
      textPrim->textBlock.maxWidth = textWidth;
      textPrim->textStyle.token = renderedStyle;
      textPrim->textStyle.overrideStyle = renderedOverride;
    }

    uint32_t selStart = 0u;
    uint32_t selEnd = 0u;
    bool hasSelection = textFieldHasSelection(*stateRef, selStart, selEnd);
    bool showCursor = stateRef->focused && stateRef->cursorVisible;

    std::vector<float> caretPositions;
    if (!activeText.empty() && (hasSelection || showCursor)) {
      caretPositions = buildCaretPositions(frameRef, patchState->textStyle, activeText);
    }
    auto caretAdvanceFor = [&](uint32_t index) -> float {
      if (caretPositions.empty()) {
        return 0.0f;
      }
      uint32_t clamped = std::min(index, textSize);
      return caretPositions[clamped];
    };

    if (patchState->selectionNode.isValid()) {
      Rect selectionRect{patchState->paddingX, textY, 0.0f, lineHeight};
      bool showSelection = false;
      if (hasSelection && !activeText.empty()) {
        float startAdvance = caretAdvanceFor(selStart);
        float endAdvance = caretAdvanceFor(selEnd);
        float startX = patchState->paddingX + startAdvance;
        float endX = patchState->paddingX + endAdvance;
        float maxX = patchState->width - patchState->paddingX;
        if (maxX < patchState->paddingX) {
          maxX = patchState->paddingX;
        }
        startX = std::clamp(startX, patchState->paddingX, maxX);
        endX = std::clamp(endX, patchState->paddingX, maxX);
        if (endX > startX) {
          selectionRect.x = startX;
          selectionRect.width = endX - startX;
          showSelection = true;
        }
      }
      if (PrimeFrame::Node* selectionNode = frameRef.getNode(patchState->selectionNode)) {
        selectionNode->localX = selectionRect.x;
        selectionNode->localY = selectionRect.y;
        selectionNode->sizeHint.width.preferred = selectionRect.width;
        selectionNode->sizeHint.height.preferred = selectionRect.height;
        selectionNode->visible = showSelection;
      }
      if (PrimeFrame::Primitive* selectionPrim = frameRef.getPrimitive(patchState->selectionPrim)) {
        selectionPrim->width = selectionRect.width;
        selectionPrim->height = selectionRect.height;
      }
    }

    if (patchState->cursorNode.isValid()) {
      Rect cursorRect{patchState->paddingX, textY, 0.0f, lineHeight};
      bool showCursorVisual = false;
      if (showCursor) {
        float cursorAdvance = caretAdvanceFor(stateRef->cursor);
        float cursorX = patchState->paddingX + cursorAdvance;
        float maxX = patchState->width - patchState->paddingX - patchState->cursorWidth;
        if (maxX < patchState->paddingX) {
          maxX = patchState->paddingX;
        }
        if (cursorX > maxX) {
          cursorX = maxX;
        }
        cursorRect.x = cursorX;
        cursorRect.width = patchState->cursorWidth;
        showCursorVisual = cursorRect.width > 0.0f && cursorRect.height > 0.0f;
      }
      if (PrimeFrame::Node* cursorNode = frameRef.getNode(patchState->cursorNode)) {
        cursorNode->localX = cursorRect.x;
        cursorNode->localY = cursorRect.y;
        cursorNode->sizeHint.width.preferred = cursorRect.width;
        cursorNode->sizeHint.height.preferred = cursorRect.height;
        cursorNode->visible = showCursorVisual;
      }
      if (PrimeFrame::Primitive* cursorPrim = frameRef.getPrimitive(patchState->cursorPrim)) {
        cursorPrim->width = cursorRect.width;
        cursorPrim->height = cursorRect.height;
      }
    }
  };

  patchTextFieldVisuals();

  if (state) {
    PrimeFrame::Node* node = frame().getNode(field.nodeId());
    if (node) {
      PrimeFrame::Callback callback;
      callback.onEvent = [state,
                          callbacks = spec.callbacks,
                          clipboard = spec.clipboard,
                          framePtr = &frame(),
                          textStyle = spec.textStyle,
                          paddingX = spec.paddingX,
                          allowNewlines = spec.allowNewlines,
                          readOnly,
                          handleClipboardShortcuts = spec.handleClipboardShortcuts,
                          cursorBlinkInterval = spec.cursorBlinkInterval,
                          patchTextFieldVisuals](PrimeFrame::Event const& event) -> bool {
        if (!state) {
          return false;
        }

        auto update_cursor_hint = [&](bool hovered) {
          CursorHint next = hovered ? CursorHint::IBeam : CursorHint::Arrow;
          if (state->cursorHint != next) {
            state->cursorHint = next;
            if (callbacks.onCursorHintChanged) {
              callbacks.onCursorHintChanged(next);
            }
          }
        };
        auto clamp_indices = [&]() {
          uint32_t size = static_cast<uint32_t>(state->text.size());
          state->cursor = std::min(state->cursor, size);
          state->selectionAnchor = std::min(state->selectionAnchor, size);
          state->selectionStart = std::min(state->selectionStart, size);
          state->selectionEnd = std::min(state->selectionEnd, size);
        };
        auto reset_blink = [&](std::chrono::steady_clock::time_point now) {
          state->cursorVisible = true;
          state->nextBlink = now + cursorBlinkInterval;
        };
        auto notify_state = [&]() {
          patchTextFieldVisuals();
          if (callbacks.onStateChanged) {
            callbacks.onStateChanged();
          }
        };
        auto notify_text = [&]() {
          if (callbacks.onChange) {
            callbacks.onChange(std::string_view(state->text));
          } else if (callbacks.onTextChanged) {
            callbacks.onTextChanged(std::string_view(state->text));
          }
        };

        switch (event.type) {
          case PrimeFrame::EventType::PointerEnter: {
            if (!state->hovered) {
              state->hovered = true;
              if (callbacks.onHoverChanged) {
                callbacks.onHoverChanged(true);
              }
              update_cursor_hint(true);
              notify_state();
            }
            return true;
          }
          case PrimeFrame::EventType::PointerLeave: {
            if (state->hovered) {
              state->hovered = false;
              if (callbacks.onHoverChanged) {
                callbacks.onHoverChanged(false);
              }
              update_cursor_hint(false);
              notify_state();
            }
            return true;
          }
          case PrimeFrame::EventType::PointerDown: {
            clamp_indices();
            uint32_t cursorIndex =
                caretIndexForClick(*framePtr, textStyle, state->text, paddingX, event.localX);
            state->cursor = cursorIndex;
            state->selectionAnchor = cursorIndex;
            state->selectionStart = cursorIndex;
            state->selectionEnd = cursorIndex;
            state->selecting = true;
            state->pointerId = event.pointerId;
            reset_blink(std::chrono::steady_clock::now());
            notify_state();
            return true;
          }
          case PrimeFrame::EventType::PointerDrag:
          case PrimeFrame::EventType::PointerMove: {
            if (!state->selecting || state->pointerId != event.pointerId) {
              return false;
            }
            clamp_indices();
            uint32_t cursorIndex =
                caretIndexForClick(*framePtr, textStyle, state->text, paddingX, event.localX);
            if (cursorIndex != state->cursor || state->selectionEnd != cursorIndex) {
              state->cursor = cursorIndex;
              state->selectionStart = state->selectionAnchor;
              state->selectionEnd = cursorIndex;
              reset_blink(std::chrono::steady_clock::now());
              notify_state();
            }
            return true;
          }
          case PrimeFrame::EventType::PointerUp:
          case PrimeFrame::EventType::PointerCancel: {
            if (state->pointerId == event.pointerId) {
              if (state->selecting) {
                state->selecting = false;
                state->pointerId = -1;
                notify_state();
              }
              return true;
            }
            return false;
          }
          case PrimeFrame::EventType::KeyDown: {
            if (!state->focused) {
              return false;
            }
            constexpr int KeyReturn = keyCodeInt(KeyCode::Enter);
            constexpr int KeyEscape = keyCodeInt(KeyCode::Escape);
            constexpr int KeyBackspace = keyCodeInt(KeyCode::Backspace);
            constexpr int KeyLeft = keyCodeInt(KeyCode::Left);
            constexpr int KeyRight = keyCodeInt(KeyCode::Right);
            constexpr int KeyHome = keyCodeInt(KeyCode::Home);
            constexpr int KeyEnd = keyCodeInt(KeyCode::End);
            constexpr int KeyDelete = keyCodeInt(KeyCode::Delete);
            constexpr int KeyA = keyCodeInt(KeyCode::A);
            constexpr int KeyC = keyCodeInt(KeyCode::C);
            constexpr int KeyV = keyCodeInt(KeyCode::V);
            constexpr int KeyX = keyCodeInt(KeyCode::X);
            constexpr uint32_t ShiftMask = 1u << 0u;
            constexpr uint32_t ControlMask = 1u << 1u;
            constexpr uint32_t SuperMask = 1u << 3u;
            bool shiftPressed = (event.modifiers & ShiftMask) != 0u;
            bool isShortcut =
                handleClipboardShortcuts &&
                ((event.modifiers & ControlMask) != 0u || (event.modifiers & SuperMask) != 0u);

            clamp_indices();
            uint32_t selectionStart = 0u;
            uint32_t selectionEnd = 0u;
            bool hasSelection = textFieldHasSelection(*state, selectionStart, selectionEnd);
            auto delete_selection = [&]() -> bool {
              if (!hasSelection) {
                return false;
              }
              state->text.erase(selectionStart, selectionEnd - selectionStart);
              state->cursor = selectionStart;
              clearTextFieldSelection(*state, state->cursor);
              return true;
            };

            if (isShortcut) {
              if (event.key == KeyA) {
                uint32_t size = static_cast<uint32_t>(state->text.size());
                state->selectionAnchor = 0u;
                state->selectionStart = 0u;
                state->selectionEnd = size;
                state->cursor = size;
                reset_blink(std::chrono::steady_clock::now());
                notify_state();
                return true;
              }
              if (event.key == KeyC) {
                if (hasSelection && clipboard.setText) {
                  clipboard.setText(
                      std::string_view(state->text.data() + selectionStart,
                                       selectionEnd - selectionStart));
                }
                return true;
              }
              if (event.key == KeyX) {
                if (readOnly) {
                  return true;
                }
                if (hasSelection) {
                  if (clipboard.setText) {
                    clipboard.setText(
                        std::string_view(state->text.data() + selectionStart,
                                         selectionEnd - selectionStart));
                  }
                  delete_selection();
                  notify_text();
                  reset_blink(std::chrono::steady_clock::now());
                  notify_state();
                }
                return true;
              }
              if (event.key == KeyV) {
                if (readOnly) {
                  return true;
                }
                if (clipboard.getText) {
                  std::string paste = clipboard.getText();
                  if (!allowNewlines) {
                    paste.erase(std::remove(paste.begin(), paste.end(), '\n'), paste.end());
                    paste.erase(std::remove(paste.begin(), paste.end(), '\r'), paste.end());
                  }
                  if (!paste.empty()) {
                    delete_selection();
                    uint32_t cursor = state->cursor;
                    cursor = std::min(cursor, static_cast<uint32_t>(state->text.size()));
                    state->text.insert(cursor, paste);
                    state->cursor = cursor + static_cast<uint32_t>(paste.size());
                    clearTextFieldSelection(*state, state->cursor);
                    notify_text();
                    reset_blink(std::chrono::steady_clock::now());
                    notify_state();
                  }
                }
                return true;
              }
            }

            bool changed = false;
            bool keepSelection = false;
            uint32_t cursor = state->cursor;
            switch (event.key) {
              case KeyEscape:
                if (callbacks.onRequestBlur) {
                  callbacks.onRequestBlur();
                }
                return true;
              case KeyLeft:
                if (shiftPressed) {
                  if (!hasSelection) {
                    state->selectionAnchor = cursor;
                  }
                  cursor = utf8Prev(state->text, cursor);
                  state->selectionStart = state->selectionAnchor;
                  state->selectionEnd = cursor;
                  keepSelection = true;
                } else {
                  if (hasSelection) {
                    cursor = selectionStart;
                  } else {
                    cursor = utf8Prev(state->text, cursor);
                  }
                  clearTextFieldSelection(*state, cursor);
                }
                changed = true;
                break;
              case KeyRight:
                if (shiftPressed) {
                  if (!hasSelection) {
                    state->selectionAnchor = cursor;
                  }
                  cursor = utf8Next(state->text, cursor);
                  state->selectionStart = state->selectionAnchor;
                  state->selectionEnd = cursor;
                  keepSelection = true;
                } else {
                  if (hasSelection) {
                    cursor = selectionEnd;
                  } else {
                    cursor = utf8Next(state->text, cursor);
                  }
                  clearTextFieldSelection(*state, cursor);
                }
                changed = true;
                break;
              case KeyHome:
                if (shiftPressed) {
                  if (!hasSelection) {
                    state->selectionAnchor = cursor;
                  }
                  cursor = 0u;
                  state->selectionStart = state->selectionAnchor;
                  state->selectionEnd = cursor;
                  keepSelection = true;
                } else {
                  cursor = 0u;
                  clearTextFieldSelection(*state, cursor);
                }
                changed = true;
                break;
              case KeyEnd:
                if (shiftPressed) {
                  if (!hasSelection) {
                    state->selectionAnchor = cursor;
                  }
                  cursor = static_cast<uint32_t>(state->text.size());
                  state->selectionStart = state->selectionAnchor;
                  state->selectionEnd = cursor;
                  keepSelection = true;
                } else {
                  cursor = static_cast<uint32_t>(state->text.size());
                  clearTextFieldSelection(*state, cursor);
                }
                changed = true;
                break;
              case KeyBackspace:
                if (readOnly) {
                  return true;
                }
                if (delete_selection()) {
                  changed = true;
                  cursor = state->cursor;
                  notify_text();
                } else if (cursor > 0u) {
                  uint32_t start = utf8Prev(state->text, cursor);
                  state->text.erase(start, cursor - start);
                  cursor = start;
                  changed = true;
                  notify_text();
                }
                break;
              case KeyDelete:
                if (readOnly) {
                  return true;
                }
                if (delete_selection()) {
                  changed = true;
                  cursor = state->cursor;
                  notify_text();
                } else if (cursor < static_cast<uint32_t>(state->text.size())) {
                  uint32_t end = utf8Next(state->text, cursor);
                  state->text.erase(cursor, end - cursor);
                  changed = true;
                  notify_text();
                }
                break;
              case KeyReturn:
                if (!allowNewlines) {
                  if (!readOnly && callbacks.onSubmit) {
                    callbacks.onSubmit();
                  }
                  return true;
                }
                return true;
              default:
                break;
            }
            if (changed) {
              state->cursor = std::min(cursor, static_cast<uint32_t>(state->text.size()));
              if (!keepSelection) {
                clearTextFieldSelection(*state, state->cursor);
              }
              reset_blink(std::chrono::steady_clock::now());
              notify_state();
              return true;
            }
            return false;
          }
          case PrimeFrame::EventType::TextInput: {
            if (!state->focused) {
              return false;
            }
            if (readOnly) {
              return true;
            }
            if (event.text.empty()) {
              return true;
            }
            std::string filtered;
            filtered.reserve(event.text.size());
            for (char ch : event.text) {
              if (!allowNewlines && (ch == '\n' || ch == '\r')) {
                continue;
              }
              filtered.push_back(ch);
            }
            if (filtered.empty()) {
              return true;
            }
            clamp_indices();
            uint32_t selectionStart = 0u;
            uint32_t selectionEnd = 0u;
            bool hasSelection = textFieldHasSelection(*state, selectionStart, selectionEnd);
            if (hasSelection) {
              state->text.erase(selectionStart, selectionEnd - selectionStart);
              state->cursor = selectionStart;
              clearTextFieldSelection(*state, state->cursor);
            }
            uint32_t cursor = std::min(state->cursor, static_cast<uint32_t>(state->text.size()));
            state->text.insert(cursor, filtered);
            state->cursor = cursor + static_cast<uint32_t>(filtered.size());
            clearTextFieldSelection(*state, state->cursor);
            notify_text();
            reset_blink(std::chrono::steady_clock::now());
            notify_state();
            return true;
          }
          default:
            break;
        }
        return false;
      };

      callback.onFocus = [state,
                          callbacks = spec.callbacks,
                          cursorBlinkInterval = spec.cursorBlinkInterval,
                          setCursorToEndOnFocus = spec.setCursorToEndOnFocus,
                          patchTextFieldVisuals]() {
        if (!state) {
          return;
        }
        bool focusChanged = !state->focused;
        if (!focusChanged) {
          return;
        }
        state->focused = true;
        uint32_t size = static_cast<uint32_t>(state->text.size());
        state->cursor = std::min(state->cursor, size);
        if (focusChanged && setCursorToEndOnFocus) {
          state->cursor = size;
        }
        clearTextFieldSelection(*state, state->cursor);
        state->cursorVisible = true;
        state->nextBlink = std::chrono::steady_clock::now() + cursorBlinkInterval;
        patchTextFieldVisuals();
        if (focusChanged && callbacks.onFocusChanged) {
          callbacks.onFocusChanged(true);
        }
        if (callbacks.onStateChanged) {
          callbacks.onStateChanged();
        }
      };

      callback.onBlur = [state, callbacks = spec.callbacks, patchTextFieldVisuals]() {
        if (!state) {
          return;
        }
        bool focusChanged = state->focused;
        if (!focusChanged) {
          return;
        }
        state->focused = false;
        state->cursorVisible = false;
        state->nextBlink = {};
        state->selecting = false;
        state->pointerId = -1;
        uint32_t size = static_cast<uint32_t>(state->text.size());
        state->cursor = std::min(state->cursor, size);
        clearTextFieldSelection(*state, state->cursor);
        patchTextFieldVisuals();
        if (focusChanged && callbacks.onFocusChanged) {
          callbacks.onFocusChanged(false);
        }
        if (callbacks.onStateChanged) {
          callbacks.onStateChanged();
        }
      };

      node->callbacks = frame().addCallback(std::move(callback));
    }
  }

  std::optional<FocusOverlay> focusOverlay;
  bool canFocus = enabled && state != nullptr;
  if (spec.visible && canFocus) {
    ResolvedFocusStyle focusStyle = resolve_focus_style(
        frame(),
        spec.focusStyle,
        spec.focusStyleOverride,
        {spec.cursorStyle, spec.selectionStyle, spec.backgroundStyle},
        spec.backgroundStyleOverride);
    Rect focusRect{0.0f, 0.0f, bounds.width, bounds.height};
    focusOverlay = add_focus_overlay_node(frame(),
                                          field.nodeId(),
                                          focusRect,
                                          focusStyle.token,
                                          focusStyle.overrideStyle,
                                          spec.visible);
  }

  if (PrimeFrame::Node* node = frame().getNode(field.nodeId())) {
    node->focusable = canFocus;
    node->hitTestVisible = enabled;
    node->tabIndex = canFocus ? spec.tabIndex : -1;
  }

  if (focusOverlay.has_value()) {
    attach_focus_callbacks(frame(), field.nodeId(), *focusOverlay);
  }

  if (!enabled) {
    add_state_scrim_overlay(frame(),
                            field.nodeId(),
                            Rect{0.0f, 0.0f, bounds.width, bounds.height},
                            DisabledScrimOpacity,
                            spec.visible);
  } else if (readOnly) {
    add_state_scrim_overlay(frame(),
                            field.nodeId(),
                            Rect{0.0f, 0.0f, bounds.width, bounds.height},
                            ReadOnlyScrimOpacity,
                            spec.visible);
  }

  return UiNode(frame(), field.nodeId(), allowAbsolute_);
}

UiNode UiNode::createTextField(TextFieldState& state,
                               std::string_view placeholder,
                               PrimeFrame::RectStyleToken backgroundStyle,
                               PrimeFrame::TextStyleToken textStyle,
                               SizeSpec const& size) {
  TextFieldSpec spec;
  spec.state = &state;
  spec.placeholder = placeholder;
  spec.backgroundStyle = backgroundStyle;
  spec.textStyle = textStyle;
  spec.size = size;
  return createTextField(spec);
}

UiNode UiNode::createSelectableText(SelectableTextSpec const& specInput) {
  SelectableTextSpec spec = specInput;
  sanitize_size_spec(spec.size, "SelectableTextSpec.size");
  spec.paddingX = clamp_non_negative(spec.paddingX, "SelectableTextSpec", "paddingX");
  spec.maxWidth = clamp_non_negative(spec.maxWidth, "SelectableTextSpec", "maxWidth");
  bool enabled = spec.enabled;
  apply_default_accessibility_semantics(spec.accessibility, AccessibilityRole::StaticText, enabled);

  Rect bounds = resolve_rect(spec.size);
  std::shared_ptr<SelectableTextState> stateOwner = spec.ownedState;
  SelectableTextState* state = spec.state;
  if (!state) {
    if (!stateOwner) {
      stateOwner = std::make_shared<SelectableTextState>();
    }
    state = stateOwner.get();
  }
  std::string_view text = spec.text;
  float maxWidth = spec.maxWidth;
  if (maxWidth <= 0.0f && spec.size.maxWidth.has_value()) {
    maxWidth = std::max(0.0f, *spec.size.maxWidth - spec.paddingX * 2.0f);
  }
  if (maxWidth <= 0.0f && bounds.width > 0.0f) {
    float available = bounds.width - spec.paddingX * 2.0f;
    maxWidth = std::max(0.0f, available);
  }
  if (maxWidth <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f &&
      !text.empty()) {
    maxWidth = DefaultSelectableTextWrapWidth;
  }

  TextSelectionLayout layout =
      buildTextSelectionLayout(frame(), spec.textStyle, text, maxWidth, spec.wrap);
  if (layout.lineHeight <= 0.0f) {
    layout.lineHeight = resolve_line_height(frame(), spec.textStyle);
  }
  size_t lineCount = std::max<size_t>(1u, layout.lines.size());
  float textHeight = layout.lineHeight * static_cast<float>(lineCount);
  float textWidth = 0.0f;
  for (auto const& line : layout.lines) {
    textWidth = std::max(textWidth, line.width);
  }
  float desiredWidth = (maxWidth > 0.0f ? maxWidth : textWidth) + spec.paddingX * 2.0f;

  if (bounds.width <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f) {
    bounds.width = desiredWidth;
  }
  if (bounds.width <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX > 0.0f &&
      maxWidth > 0.0f) {
    bounds.width = desiredWidth;
  }
  if (bounds.height <= 0.0f &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchY <= 0.0f) {
    bounds.height = textHeight;
  }
  if (bounds.width <= 0.0f &&
      bounds.height <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchX <= 0.0f &&
      spec.size.stretchY <= 0.0f) {
    return UiNode(frame(), id_, allowAbsolute_);
  }

  StackSpec overlaySpec;
  overlaySpec.size = spec.size;
  if (!overlaySpec.size.preferredWidth.has_value() && bounds.width > 0.0f) {
    overlaySpec.size.preferredWidth = bounds.width;
  }
  if (!overlaySpec.size.preferredHeight.has_value() && bounds.height > 0.0f) {
    overlaySpec.size.preferredHeight = bounds.height;
  }
  if (spec.paddingX > 0.0f) {
    overlaySpec.padding.left = spec.paddingX;
    overlaySpec.padding.right = spec.paddingX;
  }
  overlaySpec.clipChildren = true;
  overlaySpec.visible = spec.visible;
  UiNode overlay = createOverlay(overlaySpec);
  overlay.setHitTestVisible(enabled);

  if (!spec.visible) {
    return UiNode(frame(), overlay.nodeId(), allowAbsolute_);
  }

  uint32_t textSize = static_cast<uint32_t>(text.size());
  uint32_t selectionStart =
      clamp_text_index(spec.selectionStart, textSize, "SelectableTextSpec", "selectionStart");
  uint32_t selectionEnd =
      clamp_text_index(spec.selectionEnd, textSize, "SelectableTextSpec", "selectionEnd");
  if (state && enabled) {
    state->text = text;
    state->selectionAnchor = clamp_text_index(state->selectionAnchor,
                                              textSize,
                                              "SelectableTextState",
                                              "selectionAnchor");
    state->selectionStart = clamp_text_index(state->selectionStart,
                                             textSize,
                                             "SelectableTextState",
                                             "selectionStart");
    state->selectionEnd = clamp_text_index(state->selectionEnd,
                                           textSize,
                                           "SelectableTextState",
                                           "selectionEnd");
    selectionStart = state->selectionStart;
    selectionEnd = state->selectionEnd;
  }

  TextSelectionOverlaySpec selectionSpec;
  selectionSpec.text = text;
  selectionSpec.textStyle = spec.textStyle;
  selectionSpec.wrap = spec.wrap;
  selectionSpec.maxWidth = maxWidth;
  selectionSpec.layout = &layout;
  selectionSpec.selectionStart = selectionStart;
  selectionSpec.selectionEnd = selectionEnd;
  selectionSpec.paddingX = 0.0f;
  selectionSpec.selectionStyle = spec.selectionStyle;
  selectionSpec.selectionStyleOverride = spec.selectionStyleOverride;
  float textAreaWidth = maxWidth > 0.0f ? maxWidth : std::max(0.0f, bounds.width - spec.paddingX * 2.0f);
  selectionSpec.size.preferredWidth = textAreaWidth;
  selectionSpec.size.preferredHeight = bounds.height;
  selectionSpec.visible = spec.visible;
  overlay.createTextSelectionOverlay(selectionSpec);

  ParagraphSpec paragraphSpec;
  paragraphSpec.text = text;
  paragraphSpec.textStyle = spec.textStyle;
  paragraphSpec.textStyleOverride = spec.textStyleOverride;
  paragraphSpec.wrap = spec.wrap;
  paragraphSpec.maxWidth = maxWidth;
  paragraphSpec.size.preferredWidth = textAreaWidth;
  paragraphSpec.size.preferredHeight = bounds.height;
  paragraphSpec.visible = spec.visible;
  overlay.createParagraph(paragraphSpec);

  if (state) {
    auto layoutPtr = std::make_shared<TextSelectionLayout>(layout);
    PrimeFrame::Callback callback;
    callback.onEvent = [state,
                        callbacks = spec.callbacks,
                        clipboard = spec.clipboard,
                        layoutPtr,
                        framePtr = &frame(),
                        textStyle = spec.textStyle,
                        paddingX = spec.paddingX,
                        handleClipboardShortcuts = spec.handleClipboardShortcuts,
                        stateOwner](
                           PrimeFrame::Event const& event) -> bool {
      (void)stateOwner;
      if (!state) {
        return false;
      }
      auto update_cursor_hint = [&](bool hovered) {
        CursorHint next = hovered ? CursorHint::IBeam : CursorHint::Arrow;
        if (state->cursorHint != next) {
          state->cursorHint = next;
          if (callbacks.onCursorHintChanged) {
            callbacks.onCursorHintChanged(next);
          }
        }
      };
      auto notify_state = [&]() {
        if (callbacks.onStateChanged) {
          callbacks.onStateChanged();
        }
      };
      auto notify_selection = [&]() {
        uint32_t start = std::min(state->selectionStart, state->selectionEnd);
        uint32_t end = std::max(state->selectionStart, state->selectionEnd);
        if (callbacks.onSelectionChanged) {
          callbacks.onSelectionChanged(start, end);
        }
      };
      auto clamp_indices = [&]() {
        uint32_t size = static_cast<uint32_t>(state->text.size());
        state->selectionAnchor = std::min(state->selectionAnchor, size);
        state->selectionStart = std::min(state->selectionStart, size);
        state->selectionEnd = std::min(state->selectionEnd, size);
      };

      switch (event.type) {
        case PrimeFrame::EventType::PointerEnter: {
          if (!state->hovered) {
            state->hovered = true;
            if (callbacks.onHoverChanged) {
              callbacks.onHoverChanged(true);
            }
            update_cursor_hint(true);
            notify_state();
          }
          return true;
        }
        case PrimeFrame::EventType::PointerLeave: {
          if (state->hovered) {
            state->hovered = false;
            if (callbacks.onHoverChanged) {
              callbacks.onHoverChanged(false);
            }
            update_cursor_hint(false);
            notify_state();
          }
          return true;
        }
        case PrimeFrame::EventType::PointerDown: {
          clamp_indices();
          uint32_t cursorIndex = caretIndexForClickInLayout(*framePtr,
                                                            textStyle,
                                                            state->text,
                                                            *layoutPtr,
                                                            paddingX,
                                                            event.localX,
                                                            event.localY);
          state->selectionAnchor = cursorIndex;
          state->selectionStart = cursorIndex;
          state->selectionEnd = cursorIndex;
          state->selecting = true;
          state->pointerId = event.pointerId;
          notify_selection();
          notify_state();
          return true;
        }
        case PrimeFrame::EventType::PointerDrag:
        case PrimeFrame::EventType::PointerMove: {
          if (!state->selecting || state->pointerId != event.pointerId) {
            return false;
          }
          clamp_indices();
          uint32_t cursorIndex = caretIndexForClickInLayout(*framePtr,
                                                            textStyle,
                                                            state->text,
                                                            *layoutPtr,
                                                            paddingX,
                                                            event.localX,
                                                            event.localY);
          if (state->selectionEnd != cursorIndex) {
            state->selectionStart = state->selectionAnchor;
            state->selectionEnd = cursorIndex;
            notify_selection();
            notify_state();
          }
          return true;
        }
        case PrimeFrame::EventType::PointerUp:
        case PrimeFrame::EventType::PointerCancel: {
          if (state->pointerId == event.pointerId) {
            if (state->selecting) {
              state->selecting = false;
              state->pointerId = -1;
              notify_state();
            }
            if (state->hovered && event.targetW > 0.0f && event.targetH > 0.0f) {
              bool inside = event.localX >= 0.0f && event.localX < event.targetW &&
                            event.localY >= 0.0f && event.localY < event.targetH;
              if (!inside) {
                state->hovered = false;
                if (callbacks.onHoverChanged) {
                  callbacks.onHoverChanged(false);
                }
                update_cursor_hint(false);
                notify_state();
              }
            }
            return true;
          }
          return false;
        }
        case PrimeFrame::EventType::KeyDown: {
          if (!state->focused) {
            return false;
          }
          constexpr int KeyA = keyCodeInt(KeyCode::A);
          constexpr int KeyC = keyCodeInt(KeyCode::C);
          constexpr int KeyLeft = keyCodeInt(KeyCode::Left);
          constexpr int KeyRight = keyCodeInt(KeyCode::Right);
          constexpr int KeyHome = keyCodeInt(KeyCode::Home);
          constexpr int KeyEnd = keyCodeInt(KeyCode::End);
          constexpr int KeyUp = keyCodeInt(KeyCode::Up);
          constexpr int KeyDown = keyCodeInt(KeyCode::Down);
          constexpr int KeyPageUp = keyCodeInt(KeyCode::PageUp);
          constexpr int KeyPageDown = keyCodeInt(KeyCode::PageDown);
          constexpr uint32_t ShiftMask = 1u << 0u;
          constexpr uint32_t ControlMask = 1u << 1u;
          constexpr uint32_t AltMask = 1u << 2u;
          constexpr uint32_t SuperMask = 1u << 3u;
          bool shiftPressed = (event.modifiers & ShiftMask) != 0u;
          bool altPressed = (event.modifiers & AltMask) != 0u;
          bool isShortcut =
              handleClipboardShortcuts &&
              ((event.modifiers & ControlMask) != 0u || (event.modifiers & SuperMask) != 0u);
          if (!isShortcut) {
            clamp_indices();
            uint32_t selectionStart = std::min(state->selectionStart, state->selectionEnd);
            uint32_t selectionEnd = std::max(state->selectionStart, state->selectionEnd);
            bool hasSelection = selectionStart != selectionEnd;
            uint32_t cursor = hasSelection ? state->selectionEnd : state->selectionStart;
            uint32_t size = static_cast<uint32_t>(state->text.size());
            bool changed = false;
            auto move_cursor = [&](uint32_t nextCursor, uint32_t anchorCursor) {
              if (shiftPressed) {
                if (!hasSelection) {
                  state->selectionAnchor = anchorCursor;
                }
                state->selectionStart = state->selectionAnchor;
                state->selectionEnd = nextCursor;
              } else {
                clearSelectableTextSelection(*state, nextCursor);
              }
            };
            auto line_height = [&]() -> float {
              float height = layoutPtr->lineHeight;
              if (height <= 0.0f) {
                height = resolve_line_height(*framePtr, textStyle);
              }
              return height;
            };
            auto find_line_index = [&](uint32_t index) -> size_t {
              if (layoutPtr->lines.empty()) {
                return 0u;
              }
              for (size_t i = 0; i < layoutPtr->lines.size(); ++i) {
                const auto& line = layoutPtr->lines[i];
                if (index >= line.start && index <= line.end) {
                  return i;
                }
              }
              return layoutPtr->lines.size() - 1u;
            };
            auto cursor_x_for_line = [&](size_t lineIndex, uint32_t index) -> float {
              if (layoutPtr->lines.empty()) {
                return 0.0f;
              }
              const auto& line = layoutPtr->lines[lineIndex];
              if (line.end < line.start) {
                return 0.0f;
              }
              uint32_t localIndex = 0u;
              if (index >= line.start) {
                uint32_t clampedIndex = std::min(index, line.end);
                localIndex = clampedIndex - line.start;
              }
              std::string_view lineText(state->text.data() + line.start, line.end - line.start);
              auto positions = buildCaretPositions(*framePtr, textStyle, lineText);
              if (positions.empty()) {
                return 0.0f;
              }
              localIndex = std::min<uint32_t>(localIndex, static_cast<uint32_t>(positions.size() - 1u));
              return positions[localIndex];
            };
            auto move_vertical = [&](int deltaLines) -> bool {
              if (layoutPtr->lines.empty()) {
                return false;
              }
              size_t lineIndex = find_line_index(cursor);
              int target = static_cast<int>(lineIndex) + deltaLines;
              if (target < 0) {
                target = 0;
              }
              int maxIndex = static_cast<int>(layoutPtr->lines.size()) - 1;
              if (target > maxIndex) {
                target = maxIndex;
              }
              float height = line_height();
              if (height <= 0.0f) {
                return false;
              }
              float cursorX = cursor_x_for_line(lineIndex, cursor);
              float localX = paddingX + cursorX;
              float localY = (static_cast<float>(target) + 0.5f) * height;
              uint32_t nextCursor = caretIndexForClickInLayout(*framePtr,
                                                               textStyle,
                                                               state->text,
                                                               *layoutPtr,
                                                               paddingX,
                                                               localX,
                                                               localY);
              move_cursor(nextCursor, cursor);
              return true;
            };
            if (event.key == KeyLeft) {
              if (altPressed) {
                if (!shiftPressed && hasSelection) {
                  move_cursor(selectionStart, cursor);
                } else {
                  uint32_t anchorCursor = cursor;
                  cursor = prev_word_boundary(state->text, cursor);
                  move_cursor(cursor, anchorCursor);
                }
              } else if (shiftPressed) {
                if (!hasSelection) {
                  state->selectionAnchor = cursor;
                }
                cursor = utf8Prev(state->text, cursor);
                state->selectionStart = state->selectionAnchor;
                state->selectionEnd = cursor;
              } else {
                cursor = hasSelection ? selectionStart : utf8Prev(state->text, cursor);
                move_cursor(cursor, cursor);
              }
              changed = true;
            } else if (event.key == KeyRight) {
              if (altPressed) {
                if (!shiftPressed && hasSelection) {
                  move_cursor(selectionEnd, cursor);
                } else {
                  uint32_t anchorCursor = cursor;
                  cursor = next_word_boundary(state->text, cursor);
                  move_cursor(cursor, anchorCursor);
                }
              } else if (shiftPressed) {
                if (!hasSelection) {
                  state->selectionAnchor = cursor;
                }
                cursor = utf8Next(state->text, cursor);
                state->selectionStart = state->selectionAnchor;
                state->selectionEnd = cursor;
              } else {
                cursor = hasSelection ? selectionEnd : utf8Next(state->text, cursor);
                move_cursor(cursor, cursor);
              }
              changed = true;
            } else if (event.key == KeyHome) {
              if (shiftPressed) {
                if (!hasSelection) {
                  state->selectionAnchor = cursor;
                }
                cursor = 0u;
                state->selectionStart = state->selectionAnchor;
                state->selectionEnd = cursor;
              } else {
                cursor = 0u;
                move_cursor(cursor, cursor);
              }
              changed = true;
            } else if (event.key == KeyEnd) {
              if (shiftPressed) {
                if (!hasSelection) {
                  state->selectionAnchor = cursor;
                }
                cursor = size;
                state->selectionStart = state->selectionAnchor;
                state->selectionEnd = cursor;
              } else {
                cursor = size;
                move_cursor(cursor, cursor);
              }
              changed = true;
            } else if (event.key == KeyUp) {
              changed = move_vertical(-1);
            } else if (event.key == KeyDown) {
              changed = move_vertical(1);
            } else if (event.key == KeyPageUp || event.key == KeyPageDown) {
              float height = line_height();
              int pageStep = 1;
              if (height > 0.0f && event.targetH > 0.0f) {
                pageStep = std::max(1, static_cast<int>(event.targetH / height) - 1);
              }
              int delta = (event.key == KeyPageDown) ? pageStep : -pageStep;
              changed = move_vertical(delta);
            }
            if (changed) {
              notify_selection();
              notify_state();
              return true;
            }
            return false;
          }
          clamp_indices();
          if (event.key == KeyA) {
            uint32_t size = static_cast<uint32_t>(state->text.size());
            state->selectionAnchor = 0u;
            state->selectionStart = 0u;
            state->selectionEnd = size;
            notify_selection();
            notify_state();
            return true;
          }
          if (event.key == KeyC) {
            uint32_t start = 0u;
            uint32_t end = 0u;
            if (selectableTextHasSelection(*state, start, end) && clipboard.setText) {
              clipboard.setText(std::string_view(state->text.data() + start, end - start));
            }
            return true;
          }
          return false;
        }
        default:
          break;
      }
      return false;
    };

    callback.onFocus = [state, callbacks = spec.callbacks, stateOwner]() {
      (void)stateOwner;
      if (!state) {
        return;
      }
      bool changed = !state->focused;
      if (!changed) {
        return;
      }
      state->focused = true;
      if (callbacks.onFocusChanged) {
        callbacks.onFocusChanged(true);
      }
      if (callbacks.onStateChanged) {
        callbacks.onStateChanged();
      }
    };

    callback.onBlur = [state, callbacks = spec.callbacks, stateOwner]() {
      (void)stateOwner;
      if (!state) {
        return;
      }
      bool changed = state->focused;
      if (!changed) {
        return;
      }
      state->focused = false;
      state->selecting = false;
      state->pointerId = -1;
      uint32_t start = std::min(state->selectionStart, state->selectionEnd);
      uint32_t end = std::max(state->selectionStart, state->selectionEnd);
      if (start != end) {
        clearSelectableTextSelection(*state, start);
        if (callbacks.onSelectionChanged) {
          callbacks.onSelectionChanged(start, start);
        }
      }
      if (callbacks.onFocusChanged) {
        callbacks.onFocusChanged(false);
      }
      if (callbacks.onStateChanged) {
        callbacks.onStateChanged();
      }
    };

    if (PrimeFrame::Node* node = frame().getNode(overlay.nodeId())) {
      node->callbacks = frame().addCallback(std::move(callback));
    }
  }

  std::optional<FocusOverlay> focusOverlay;
  if (spec.visible && enabled) {
    Rect focusRect{0.0f, 0.0f, bounds.width, bounds.height};
    focusOverlay = add_focus_overlay_node(frame(),
                                          overlay.nodeId(),
                                          focusRect,
                                          spec.focusStyle,
                                          spec.focusStyleOverride,
                                          spec.visible);
    if (PrimeFrame::Node* node = frame().getNode(overlay.nodeId())) {
      node->focusable = false;
    }
  }

  if (focusOverlay.has_value()) {
    attach_focus_callbacks(frame(), overlay.nodeId(), *focusOverlay);
  }

  if (!enabled) {
    add_state_scrim_overlay(frame(),
                            overlay.nodeId(),
                            Rect{0.0f, 0.0f, bounds.width, bounds.height},
                            DisabledScrimOpacity,
                            spec.visible);
  }

  return UiNode(frame(), overlay.nodeId(), allowAbsolute_);
}

UiNode UiNode::createToggle(ToggleSpec const& specInput) {
  ToggleSpec spec = specInput;
  sanitize_size_spec(spec.size, "ToggleSpec.size");
  spec.knobInset = clamp_non_negative(spec.knobInset, "ToggleSpec", "knobInset");
  spec.tabIndex = clamp_tab_index(spec.tabIndex, "ToggleSpec", "tabIndex");
  bool enabled = spec.enabled;
  bool on = spec.binding.state ? spec.binding.state->value : (spec.state ? spec.state->on : spec.on);
  apply_default_checked_semantics(spec.accessibility, AccessibilityRole::Toggle, enabled, on);

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
  float knobX = on ? maxX - inset : inset;
  knobX = std::clamp(knobX, 0.0f, maxX);
  Rect knobRect{knobX, inset, knobSize, knobSize};
  PrimeFrame::NodeId knobNodeId = create_rect_node(frame(),
                                                   toggle.nodeId(),
                                                   knobRect,
                                                   spec.knobStyle,
                                                   spec.knobStyleOverride,
                                                   false,
                                                   spec.visible);
  auto applyToggleVisual = [framePtr = &frame(),
                            knobNodeId,
                            width = bounds.width,
                            height = bounds.height,
                            inset](bool value) {
    float knobSizeInner = std::max(0.0f, height - inset * 2.0f);
    float maxXInner = std::max(0.0f, width - knobSizeInner);
    float knobXInner = value ? (maxXInner - inset) : inset;
    knobXInner = std::clamp(knobXInner, 0.0f, maxXInner);
    if (PrimeFrame::Node* knobNode = framePtr->getNode(knobNodeId)) {
      knobNode->localX = knobXInner;
      knobNode->localY = inset;
      knobNode->sizeHint.width.preferred = knobSizeInner;
      knobNode->sizeHint.height.preferred = knobSizeInner;
      knobNode->visible = knobSizeInner > 0.0f;
    }
    if (PrimeFrame::Node* knobNode = framePtr->getNode(knobNodeId);
        knobNode && !knobNode->primitives.empty()) {
      if (PrimeFrame::Primitive* knobPrim = framePtr->getPrimitive(knobNode->primitives.front())) {
        knobPrim->width = knobSizeInner;
        knobPrim->height = knobSizeInner;
      }
    }
  };
  applyToggleVisual(on);

  ResolvedFocusStyle focusStyle = resolve_focus_style(
      frame(),
      spec.focusStyle,
      spec.focusStyleOverride,
      {spec.knobStyle, spec.trackStyle},
      spec.knobStyleOverride);
  std::optional<FocusOverlay> focusOverlay;
  if (spec.visible && enabled) {
    Rect focusRect{0.0f, 0.0f, bounds.width, bounds.height};
    focusOverlay = add_focus_overlay_node(frame(),
                                          toggle.nodeId(),
                                          focusRect,
                                          focusStyle.token,
                                          focusStyle.overrideStyle,
                                          spec.visible);
    if (PrimeFrame::Node* node = frame().getNode(toggle.nodeId())) {
      node->focusable = true;
      struct ToggleInteractionState {
        bool pressed = false;
        bool value = false;
      };
      auto state = std::make_shared<ToggleInteractionState>();
      state->value = on;
      PrimeFrame::Callback callback;
      callback.onEvent = [callbacks = spec.callbacks,
                          bindingState = spec.binding.state,
                          toggleState = spec.state,
                          state,
                          applyToggleVisual](PrimeFrame::Event const& event) mutable -> bool {
        auto activate = [&]() {
          state->value = !state->value;
          if (bindingState) {
            bindingState->value = state->value;
          }
          if (toggleState) {
            toggleState->on = state->value;
          }
          applyToggleVisual(state->value);
          if (callbacks.onChange) {
            callbacks.onChange(state->value);
          } else if (callbacks.onChanged) {
            callbacks.onChanged(state->value);
          }
        };
        switch (event.type) {
          case PrimeFrame::EventType::PointerDown:
            state->pressed = true;
            return true;
          case PrimeFrame::EventType::PointerDrag:
          case PrimeFrame::EventType::PointerMove:
            if (state->pressed) {
              state->pressed = is_pointer_inside(event);
              return true;
            }
            break;
          case PrimeFrame::EventType::PointerUp: {
            bool fire = state->pressed && is_pointer_inside(event);
            state->pressed = false;
            if (fire) {
              activate();
            }
            return true;
          }
          case PrimeFrame::EventType::PointerCancel:
          case PrimeFrame::EventType::PointerLeave:
            state->pressed = false;
            return true;
          case PrimeFrame::EventType::KeyDown:
            if (is_activation_key(event.key)) {
              activate();
              return true;
            }
            break;
          default:
            break;
        }
        return false;
      };
      node->callbacks = frame().addCallback(std::move(callback));
    }
  }
  if (PrimeFrame::Node* node = frame().getNode(toggle.nodeId())) {
    node->focusable = enabled;
    node->hitTestVisible = enabled;
    node->tabIndex = enabled ? spec.tabIndex : -1;
  }
  if (focusOverlay.has_value()) {
    attach_focus_callbacks(frame(), toggle.nodeId(), *focusOverlay);
  }

  if (!enabled) {
    add_state_scrim_overlay(frame(),
                            toggle.nodeId(),
                            Rect{0.0f, 0.0f, bounds.width, bounds.height},
                            DisabledScrimOpacity,
                            spec.visible);
  }

  return UiNode(frame(), toggle.nodeId(), allowAbsolute_);
}

UiNode UiNode::createToggle(bool on,
                            PrimeFrame::RectStyleToken trackStyle,
                            PrimeFrame::RectStyleToken knobStyle,
                            SizeSpec const& size) {
  ToggleSpec spec;
  spec.on = on;
  spec.trackStyle = trackStyle;
  spec.knobStyle = knobStyle;
  spec.size = size;
  return createToggle(spec);
}

UiNode UiNode::createToggle(Binding<bool> binding) {
  ToggleSpec spec;
  spec.binding = binding;
  return createToggle(spec);
}

UiNode UiNode::createCheckbox(CheckboxSpec const& specInput) {
  CheckboxSpec spec = specInput;
  sanitize_size_spec(spec.size, "CheckboxSpec.size");
  spec.boxSize = clamp_non_negative(spec.boxSize, "CheckboxSpec", "boxSize");
  spec.checkInset = clamp_non_negative(spec.checkInset, "CheckboxSpec", "checkInset");
  spec.gap = clamp_non_negative(spec.gap, "CheckboxSpec", "gap");
  spec.tabIndex = clamp_tab_index(spec.tabIndex, "CheckboxSpec", "tabIndex");
  bool enabled = spec.enabled;
  bool checked = spec.binding.state ? spec.binding.state->value
                                    : (spec.state ? spec.state->checked : spec.checked);
  apply_default_checked_semantics(spec.accessibility, AccessibilityRole::Checkbox, enabled, checked);

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
  float inset = std::max(0.0f, spec.checkInset);
  float checkSize = std::max(0.0f, spec.boxSize - inset * 2.0f);
  Rect checkRect{inset, inset, checkSize, checkSize};
  PrimeFrame::NodeId checkNodeId = create_rect_node(frame(),
                                                    boxNode.nodeId(),
                                                    checkRect,
                                                    spec.checkStyle,
                                                    spec.checkStyleOverride,
                                                    false,
                                                    spec.visible);
  auto applyCheckboxVisual = [framePtr = &frame(),
                              checkNodeId,
                              inset,
                              boxSize = spec.boxSize](bool value) {
    float checkSizeInner = std::max(0.0f, boxSize - inset * 2.0f);
    if (PrimeFrame::Node* checkNode = framePtr->getNode(checkNodeId)) {
      checkNode->localX = inset;
      checkNode->localY = inset;
      checkNode->sizeHint.width.preferred = checkSizeInner;
      checkNode->sizeHint.height.preferred = checkSizeInner;
      checkNode->visible = value && checkSizeInner > 0.0f;
    }
    if (PrimeFrame::Node* checkNode = framePtr->getNode(checkNodeId);
        checkNode && !checkNode->primitives.empty()) {
      if (PrimeFrame::Primitive* checkPrim = framePtr->getPrimitive(checkNode->primitives.front())) {
        checkPrim->width = checkSizeInner;
        checkPrim->height = checkSizeInner;
      }
    }
  };
  applyCheckboxVisual(checked);

  if (!spec.visible) {
    if (PrimeFrame::Node* checkNode = frame().getNode(checkNodeId)) {
      checkNode->visible = false;
    }
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

  ResolvedFocusStyle focusStyle = resolve_focus_style(
      frame(),
      spec.focusStyle,
      spec.focusStyleOverride,
      {spec.checkStyle, spec.boxStyle},
      spec.checkStyleOverride);
  std::optional<FocusOverlay> focusOverlay;
  if (spec.visible && enabled) {
    Rect focusRect{0.0f, 0.0f, bounds.width, bounds.height};
    focusOverlay = add_focus_overlay_node(frame(),
                                          row.nodeId(),
                                          focusRect,
                                          focusStyle.token,
                                          focusStyle.overrideStyle,
                                          spec.visible);
    if (PrimeFrame::Node* node = frame().getNode(row.nodeId())) {
      node->focusable = true;
      node->hitTestVisible = true;
      struct CheckboxInteractionState {
        bool pressed = false;
        bool checked = false;
      };
      auto state = std::make_shared<CheckboxInteractionState>();
      state->checked = checked;
      PrimeFrame::Callback callback;
      callback.onEvent = [callbacks = spec.callbacks,
                          bindingState = spec.binding.state,
                          checkboxState = spec.state,
                          state,
                          applyCheckboxVisual](PrimeFrame::Event const& event) mutable -> bool {
        auto activate = [&]() {
          state->checked = !state->checked;
          if (bindingState) {
            bindingState->value = state->checked;
          }
          if (checkboxState) {
            checkboxState->checked = state->checked;
          }
          applyCheckboxVisual(state->checked);
          if (callbacks.onChange) {
            callbacks.onChange(state->checked);
          } else if (callbacks.onChanged) {
            callbacks.onChanged(state->checked);
          }
        };
        switch (event.type) {
          case PrimeFrame::EventType::PointerDown:
            state->pressed = true;
            return true;
          case PrimeFrame::EventType::PointerDrag:
          case PrimeFrame::EventType::PointerMove:
            if (state->pressed) {
              state->pressed = is_pointer_inside(event);
              return true;
            }
            break;
          case PrimeFrame::EventType::PointerUp: {
            bool fire = state->pressed && is_pointer_inside(event);
            state->pressed = false;
            if (fire) {
              activate();
            }
            return true;
          }
          case PrimeFrame::EventType::PointerCancel:
          case PrimeFrame::EventType::PointerLeave:
            state->pressed = false;
            return true;
          case PrimeFrame::EventType::KeyDown:
            if (is_activation_key(event.key)) {
              activate();
              return true;
            }
            break;
          default:
            break;
        }
        return false;
      };
      node->callbacks = frame().addCallback(std::move(callback));
    }
  }
  if (PrimeFrame::Node* node = frame().getNode(row.nodeId())) {
    node->focusable = enabled;
    node->hitTestVisible = enabled;
    node->tabIndex = enabled ? spec.tabIndex : -1;
  }
  if (focusOverlay.has_value()) {
    attach_focus_callbacks(frame(), row.nodeId(), *focusOverlay);
  }

  if (!enabled) {
    add_state_scrim_overlay(frame(),
                            row.nodeId(),
                            Rect{0.0f, 0.0f, bounds.width, bounds.height},
                            DisabledScrimOpacity,
                            spec.visible);
  }

  return UiNode(frame(), row.nodeId(), allowAbsolute_);
}

UiNode UiNode::createCheckbox(std::string_view label,
                              bool checked,
                              PrimeFrame::RectStyleToken boxStyle,
                              PrimeFrame::RectStyleToken checkStyle,
                              PrimeFrame::TextStyleToken textStyle,
                              SizeSpec const& size) {
  CheckboxSpec spec;
  spec.label = label;
  spec.checked = checked;
  spec.boxStyle = boxStyle;
  spec.checkStyle = checkStyle;
  spec.textStyle = textStyle;
  spec.size = size;
  return createCheckbox(spec);
}

UiNode UiNode::createCheckbox(std::string_view label, Binding<bool> binding) {
  CheckboxSpec spec;
  spec.label = label;
  spec.binding = binding;
  return createCheckbox(spec);
}

UiNode UiNode::createSlider(SliderSpec const& specInput) {
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

  bool wantsInteraction = enabled &&
                          (spec.binding.state != nullptr ||
                           spec.state != nullptr ||
                           spec.callbacks.onChange ||
                           spec.callbacks.onValueChanged ||
                           spec.callbacks.onDragStart ||
                           spec.callbacks.onDragEnd);
  ResolvedFocusStyle focusStyle = resolve_focus_style(
      frame(),
      spec.focusStyle,
      spec.focusStyleOverride,
      {spec.thumbStyle, spec.fillStyle, spec.trackStyle},
      spec.thumbStyleOverride);
  std::optional<FocusOverlay> focusOverlay;
  Rect focusRect{0.0f, 0.0f, bounds.width, bounds.height};
  if (enabled) {
    focusOverlay = add_focus_overlay_node(frame(),
                                          slider.nodeId(),
                                          focusRect,
                                          focusStyle.token,
                                          focusStyle.overrideStyle,
                                          spec.visible);
  }
  if (PrimeFrame::Node* node = frame().getNode(slider.nodeId())) {
    node->focusable = enabled;
    node->hitTestVisible = enabled;
    node->tabIndex = enabled ? spec.tabIndex : -1;
  }

  if (wantsInteraction) {
    struct SliderInteractionState {
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
    auto state = std::make_shared<SliderInteractionState>();
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
    auto notify_value_changed = [state,
                                 bindingState = spec.binding.state,
                                 sliderState = spec.state,
                                 onChange = spec.callbacks.onChange,
                                 onValueChanged = spec.callbacks.onValueChanged]() {
      if (bindingState) {
        bindingState->value = state->value;
      }
      if (sliderState) {
        sliderState->value = state->value;
      }
      if (onChange) {
        onChange(state->value);
      } else if (onValueChanged) {
        onValueChanged(state->value);
      }
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
                        framePtr = &frame(),
                        state,
                        apply_geometry,
                        update_from_event,
                        notify_value_changed,
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
          notify_value_changed();
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
          notify_value_changed();
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
          notify_value_changed();
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

  if (focusOverlay.has_value()) {
    attach_focus_callbacks(frame(), slider.nodeId(), *focusOverlay);
  }

  if (!enabled) {
    add_state_scrim_overlay(frame(),
                            slider.nodeId(),
                            Rect{0.0f, 0.0f, bounds.width, bounds.height},
                            DisabledScrimOpacity,
                            spec.visible);
  }

  return UiNode(frame(), slider.nodeId(), allowAbsolute_);
}

UiNode UiNode::createSlider(float value,
                            bool vertical,
                            PrimeFrame::RectStyleToken trackStyle,
                            PrimeFrame::RectStyleToken fillStyle,
                            PrimeFrame::RectStyleToken thumbStyle,
                            SizeSpec const& size) {
  SliderSpec spec;
  spec.value = value;
  spec.vertical = vertical;
  spec.trackStyle = trackStyle;
  spec.fillStyle = fillStyle;
  spec.thumbStyle = thumbStyle;
  spec.size = size;
  return createSlider(spec);
}

UiNode UiNode::createSlider(Binding<float> binding, bool vertical) {
  SliderSpec spec;
  spec.binding = binding;
  spec.vertical = vertical;
  return createSlider(spec);
}

UiNode UiNode::createTabs(TabsSpec const& specInput) {
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
  apply_default_accessibility_semantics(spec.accessibility, AccessibilityRole::TabList, enabled);
  if (tabCount > 0) {
    spec.accessibility.state.positionInSet = selectedIndex + 1;
    spec.accessibility.state.setSize = tabCount;
  } else {
    spec.accessibility.state.positionInSet.reset();
    spec.accessibility.state.setSize.reset();
  }

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
          (static_cast<int>(i) == selectedIndex) ? spec.activeTextStyle : spec.textStyle;
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
  if (PrimeFrame::Node* rowNode = frame().getNode(row.nodeId())) {
    rowNode->hitTestVisible = enabled;
  }
  auto sharedSelected = std::make_shared<int>(selectedIndex);

  for (size_t i = 0; i < spec.labels.size(); ++i) {
    int tabIndex = static_cast<int>(i);
    bool active = tabIndex == selectedIndex;
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

    PrimeFrame::Node* tabNode = frame().getNode(tab.nodeId());
    if (!tabNode) {
      continue;
    }
    tabNode->focusable = spec.visible && enabled;
    tabNode->hitTestVisible = enabled;
    tabNode->tabIndex = (enabled && spec.tabIndex >= 0) ? (spec.tabIndex + tabIndex) : -1;
    if (!spec.visible || !enabled) {
      continue;
    }
    struct TabState {
      bool pressed = false;
    };
    auto state = std::make_shared<TabState>();
    PrimeFrame::Callback callback;
    callback.onEvent = [callbacks = spec.callbacks,
                        bindingState = spec.binding.state,
                        tabsState = spec.state,
                        tabIndex,
                        tabCount,
                        state,
                        sharedSelected](PrimeFrame::Event const& event) mutable -> bool {
      auto commitSelection = [&](int next) {
        if (next < 0 || next >= tabCount) {
          return;
        }
        if (*sharedSelected == next) {
          return;
        }
        *sharedSelected = next;
        if (bindingState) {
          bindingState->value = next;
        }
        if (tabsState) {
          tabsState->selectedIndex = next;
        }
        if (callbacks.onSelect) {
          callbacks.onSelect(next);
        } else if (callbacks.onTabChanged) {
          callbacks.onTabChanged(next);
        }
      };
      switch (event.type) {
        case PrimeFrame::EventType::PointerDown:
          state->pressed = true;
          return true;
        case PrimeFrame::EventType::PointerDrag:
        case PrimeFrame::EventType::PointerMove:
          if (state->pressed) {
            state->pressed = is_pointer_inside(event);
            return true;
          }
          break;
        case PrimeFrame::EventType::PointerUp: {
          bool fire = state->pressed && is_pointer_inside(event);
          state->pressed = false;
          if (fire) {
            commitSelection(tabIndex);
          }
          return true;
        }
        case PrimeFrame::EventType::PointerCancel:
        case PrimeFrame::EventType::PointerLeave:
          state->pressed = false;
          return true;
        case PrimeFrame::EventType::KeyDown: {
          if (is_activation_key(event.key)) {
            commitSelection(tabIndex);
            return true;
          }
          if (tabCount <= 0) {
            return false;
          }
          int next = *sharedSelected;
          if (event.key == KeyLeft || event.key == KeyUp) {
            next = std::max(0, next - 1);
          } else if (event.key == KeyRight || event.key == KeyDown) {
            next = std::min(tabCount - 1, next + 1);
          } else if (event.key == KeyHome) {
            next = 0;
          } else if (event.key == KeyEnd) {
            next = tabCount - 1;
          } else {
            return false;
          }
          commitSelection(next);
          return true;
        }
        default:
          break;
      }
      return false;
    };
    tabNode->callbacks = frame().addCallback(std::move(callback));

    ResolvedFocusStyle focusStyle = resolve_focus_style(frame(), 0, {}, {});
    std::optional<FocusOverlay> focusOverlay;
    Rect focusRect{0.0f, 0.0f, textWidth + spec.tabPaddingX * 2.0f, bounds.height};
    focusOverlay = add_focus_overlay_node(frame(),
                                          tab.nodeId(),
                                          focusRect,
                                          focusStyle.token,
                                          focusStyle.overrideStyle,
                                          spec.visible);
    if (focusOverlay.has_value()) {
      attach_focus_callbacks(frame(), tab.nodeId(), *focusOverlay);
    }
  }

  if (!enabled) {
    add_state_scrim_overlay(frame(),
                            row.nodeId(),
                            Rect{0.0f, 0.0f, bounds.width, bounds.height},
                            DisabledScrimOpacity,
                            spec.visible);
  }

  return UiNode(frame(), row.nodeId(), allowAbsolute_);
}

UiNode UiNode::createTabs(std::vector<std::string_view> labels, Binding<int> binding) {
  TabsSpec spec;
  spec.labels = std::move(labels);
  spec.binding = binding;
  return createTabs(spec);
}

Window UiNode::createWindow(WindowSpec const& specInput) {
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

  Rect windowRect{spec.positionX, spec.positionY, spec.width, spec.height};
  PrimeFrame::NodeId windowId = create_node(frame(),
                                            id_,
                                            windowRect,
                                            nullptr,
                                            PrimeFrame::LayoutType::Overlay,
                                            PrimeFrame::Insets{},
                                            0.0f,
                                            true,
                                            spec.visible,
                                            "WindowSpec");
  add_rect_primitive(frame(), windowId, spec.frameStyle, spec.frameStyleOverride);

  PrimeFrame::Node* windowNode = frame().getNode(windowId);
  if (windowNode) {
    windowNode->focusable = spec.focusable;
    windowNode->tabIndex = spec.focusable ? spec.tabIndex : -1;
    windowNode->hitTestVisible = true;
  }

  float titleBarHeight = std::min(spec.titleBarHeight, spec.height);
  Rect titleBarRect{0.0f, 0.0f, spec.width, titleBarHeight};
  PrimeFrame::NodeId titleBarId = create_node(frame(),
                                              windowId,
                                              titleBarRect,
                                              nullptr,
                                              PrimeFrame::LayoutType::Overlay,
                                              PrimeFrame::Insets{},
                                              0.0f,
                                              false,
                                              spec.visible,
                                              "WindowSpec.titleBar");
  add_rect_primitive(frame(), titleBarId, spec.titleBarStyle, spec.titleBarStyleOverride);
  if (PrimeFrame::Node* titleNode = frame().getNode(titleBarId)) {
    titleNode->hitTestVisible = true;
  }

  if (!spec.title.empty() && titleBarHeight > 0.0f) {
    float titleLineHeight = resolve_line_height(frame(), spec.titleTextStyle);
    if (titleLineHeight <= 0.0f) {
      titleLineHeight = titleBarHeight;
    }
    float titleY = (titleBarHeight - titleLineHeight) * 0.5f;
    float titleX = std::max(0.0f, spec.contentPadding);
    float titleW = std::max(0.0f, spec.width - titleX * 2.0f);
    create_text_node(frame(),
                     titleBarId,
                     Rect{titleX, titleY, titleW, titleLineHeight},
                     spec.title,
                     spec.titleTextStyle,
                     spec.titleTextStyleOverride,
                     PrimeFrame::TextAlign::Start,
                     PrimeFrame::WrapMode::None,
                     titleW,
                     spec.visible);
  }

  PrimeFrame::Insets contentInsets{};
  contentInsets.left = spec.contentPadding;
  contentInsets.top = spec.contentPadding;
  contentInsets.right = spec.contentPadding;
  contentInsets.bottom = spec.contentPadding;

  float contentY = titleBarHeight;
  float contentHeight = std::max(0.0f, spec.height - titleBarHeight);
  Rect contentRect{0.0f, contentY, spec.width, contentHeight};
  PrimeFrame::NodeId contentId = create_node(frame(),
                                             windowId,
                                             contentRect,
                                             nullptr,
                                             PrimeFrame::LayoutType::VerticalStack,
                                             contentInsets,
                                             0.0f,
                                             true,
                                             spec.visible,
                                             "WindowSpec.content");
  add_rect_primitive(frame(), contentId, spec.contentStyle, spec.contentStyleOverride);
  if (PrimeFrame::Node* contentNode = frame().getNode(contentId)) {
    contentNode->hitTestVisible = true;
  }

  PrimeFrame::NodeId resizeHandleId{};
  if (spec.resizable && spec.resizeHandleSize > 0.0f) {
    float handleSize = std::min(spec.resizeHandleSize, std::min(spec.width, spec.height));
    float handleX = std::max(0.0f, spec.width - handleSize);
    float handleY = std::max(0.0f, spec.height - handleSize);
    resizeHandleId = create_node(frame(),
                                 windowId,
                                 Rect{handleX, handleY, handleSize, handleSize},
                                 nullptr,
                                 PrimeFrame::LayoutType::None,
                                 PrimeFrame::Insets{},
                                 0.0f,
                                 false,
                                 spec.visible,
                                 "WindowSpec.resizeHandle");
    add_rect_primitive(frame(),
                       resizeHandleId,
                       spec.resizeHandleStyle,
                       spec.resizeHandleStyleOverride);
    if (PrimeFrame::Node* resizeNode = frame().getNode(resizeHandleId)) {
      resizeNode->hitTestVisible = true;
    }
  }

  if (spec.callbacks.onFocusChanged) {
    LowLevel::appendNodeOnFocus(frame(),
                                windowId,
                                [callbacks = spec.callbacks]() {
                        callbacks.onFocusChanged(true);
                      });
    LowLevel::appendNodeOnBlur(frame(),
                               windowId,
                               [callbacks = spec.callbacks]() {
                       callbacks.onFocusChanged(false);
                     });
  }

  if (spec.callbacks.onFocusRequested) {
    LowLevel::appendNodeOnEvent(frame(),
                                windowId,
                                [callbacks = spec.callbacks](PrimeFrame::Event const& event)
                                    -> bool {
                        if (event.type == PrimeFrame::EventType::PointerDown) {
                          callbacks.onFocusRequested();
                        }
                        return false;
                      });
  }

  struct PointerDeltaState {
    bool active = false;
    int pointerId = -1;
    float lastX = 0.0f;
    float lastY = 0.0f;
  };

  if (spec.movable &&
      (spec.callbacks.onMoveStarted || spec.callbacks.onMoved || spec.callbacks.onMoveEnded ||
       spec.callbacks.onFocusRequested)) {
    auto moveState = std::make_shared<PointerDeltaState>();
    LowLevel::appendNodeOnEvent(frame(),
                                titleBarId,
                                [callbacks = spec.callbacks,
                                 moveState](PrimeFrame::Event const& event) -> bool {
                        switch (event.type) {
                          case PrimeFrame::EventType::PointerDown:
                            moveState->active = true;
                            moveState->pointerId = event.pointerId;
                            moveState->lastX = event.x;
                            moveState->lastY = event.y;
                            if (callbacks.onFocusRequested) {
                              callbacks.onFocusRequested();
                            }
                            if (callbacks.onMoveStarted) {
                              callbacks.onMoveStarted();
                            }
                            return true;
                          case PrimeFrame::EventType::PointerDrag:
                          case PrimeFrame::EventType::PointerMove:
                            if (!moveState->active || moveState->pointerId != event.pointerId) {
                              return false;
                            }
                            if (callbacks.onMoved) {
                              float deltaX = event.x - moveState->lastX;
                              float deltaY = event.y - moveState->lastY;
                              callbacks.onMoved(deltaX, deltaY);
                            }
                            moveState->lastX = event.x;
                            moveState->lastY = event.y;
                            return true;
                          case PrimeFrame::EventType::PointerUp:
                          case PrimeFrame::EventType::PointerCancel:
                            if (!moveState->active || moveState->pointerId != event.pointerId) {
                              return false;
                            }
                            moveState->active = false;
                            moveState->pointerId = -1;
                            if (callbacks.onMoveEnded) {
                              callbacks.onMoveEnded();
                            }
                            return true;
                          default:
                            break;
                        }
                        return false;
                      });
  }

  if (resizeHandleId.isValid() &&
      (spec.callbacks.onResizeStarted || spec.callbacks.onResized || spec.callbacks.onResizeEnded ||
       spec.callbacks.onFocusRequested)) {
    auto resizeState = std::make_shared<PointerDeltaState>();
    LowLevel::appendNodeOnEvent(frame(),
                                resizeHandleId,
                                [callbacks = spec.callbacks,
                                 resizeState](PrimeFrame::Event const& event) -> bool {
                        switch (event.type) {
                          case PrimeFrame::EventType::PointerDown:
                            resizeState->active = true;
                            resizeState->pointerId = event.pointerId;
                            resizeState->lastX = event.x;
                            resizeState->lastY = event.y;
                            if (callbacks.onFocusRequested) {
                              callbacks.onFocusRequested();
                            }
                            if (callbacks.onResizeStarted) {
                              callbacks.onResizeStarted();
                            }
                            return true;
                          case PrimeFrame::EventType::PointerDrag:
                          case PrimeFrame::EventType::PointerMove:
                            if (!resizeState->active || resizeState->pointerId != event.pointerId) {
                              return false;
                            }
                            if (callbacks.onResized) {
                              float deltaWidth = event.x - resizeState->lastX;
                              float deltaHeight = event.y - resizeState->lastY;
                              callbacks.onResized(deltaWidth, deltaHeight);
                            }
                            resizeState->lastX = event.x;
                            resizeState->lastY = event.y;
                            return true;
                          case PrimeFrame::EventType::PointerUp:
                          case PrimeFrame::EventType::PointerCancel:
                            if (!resizeState->active || resizeState->pointerId != event.pointerId) {
                              return false;
                            }
                            resizeState->active = false;
                            resizeState->pointerId = -1;
                            if (callbacks.onResizeEnded) {
                              callbacks.onResizeEnded();
                            }
                            return true;
                          default:
                            break;
                        }
                        return false;
                      });
  }

  if (spec.visible && spec.focusable) {
    ResolvedFocusStyle focusStyle = resolve_focus_style(frame(), 0, {}, {});
    Rect focusRect{0.0f, 0.0f, spec.width, spec.height};
    auto focusOverlay = add_focus_overlay_node(frame(),
                                               windowId,
                                               focusRect,
                                               focusStyle.token,
                                               focusStyle.overrideStyle,
                                               spec.visible);
    if (focusOverlay.has_value()) {
      attach_focus_callbacks(frame(), windowId, *focusOverlay);
    }
  }

  return Window{
      UiNode(frame(), windowId, allowAbsolute_),
      UiNode(frame(), titleBarId, allowAbsolute_),
      UiNode(frame(), contentId, allowAbsolute_),
      resizeHandleId,
  };
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
