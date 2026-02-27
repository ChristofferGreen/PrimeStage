#include "PrimeStage/PrimeStage.h"

#include "PrimeStageCollectionInternals.h"
#include "PrimeFrame/Events.h"

#include <algorithm>
#include <cmath>
#include <memory>

namespace PrimeStage {
namespace {

constexpr int KeyEnter = keyCodeInt(KeyCode::Enter);
constexpr int KeySpace = keyCodeInt(KeyCode::Space);

bool isActivationKey(int key) {
  return key == KeyEnter || key == KeySpace;
}

bool isPointerInside(PrimeFrame::Event const& event) {
  return event.localX >= 0.0f && event.localX <= event.targetW &&
         event.localY >= 0.0f && event.localY <= event.targetH;
}

} // namespace

UiNode UiNode::createButton(ButtonSpec const& specInput) {
  ButtonSpec spec = Internal::normalizeButtonSpec(specInput);
  bool enabled = spec.enabled;
  Internal::WidgetRuntimeContext runtime = Internal::makeWidgetRuntimeContext(frame(),
                                                                              nodeId(),
                                                                              allowAbsolute(),
                                                                              enabled,
                                                                              spec.visible,
                                                                              spec.tabIndex);
  PrimeFrame::Frame& runtimeFrame = Internal::runtimeFrame(runtime);

  Internal::InternalRect bounds = Internal::resolveRect(spec.size);
  float lineHeight = Internal::resolveLineHeight(runtimeFrame, spec.textStyle);
  if (bounds.height <= 0.0f &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchY <= 0.0f &&
      lineHeight > 0.0f) {
    bounds.height = lineHeight;
  }
  if (bounds.width <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f &&
      !spec.label.empty()) {
    float textWidth = Internal::estimateTextWidth(runtimeFrame, spec.textStyle, spec.label);
    bounds.width = std::max(bounds.width, textWidth + spec.textInsetX * 2.0f);
  }
  if (bounds.width <= 0.0f &&
      bounds.height <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchX <= 0.0f &&
      spec.size.stretchY <= 0.0f) {
    return UiNode(runtimeFrame, runtime.parentId, runtime.allowAbsolute);
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
    return UiNode(runtimeFrame, button.nodeId(), runtime.allowAbsolute);
  }

  if (!spec.label.empty()) {
    float textWidth = Internal::estimateTextWidth(runtimeFrame, spec.textStyle, spec.label);
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

    Internal::createTextNode(runtimeFrame,
                             button.nodeId(),
                             Internal::InternalRect{textX, textY, labelWidth, lineHeight},
                             spec.label,
                             spec.textStyle,
                             spec.textStyleOverride,
                             align,
                             PrimeFrame::WrapMode::None,
                             labelWidth,
                             spec.visible);
  }

  if (needsInteraction) {
    PrimeFrame::Node* node = runtimeFrame.getNode(button.nodeId());
    if (node && !node->primitives.empty()) {
      PrimeFrame::PrimitiveId background = node->primitives.front();
      PrimeFrame::Frame* framePtr = &runtimeFrame;
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
            bool inside = isPointerInside(event);
            update(inside, inside);
            return true;
          }
          case PrimeFrame::EventType::PointerUp: {
            bool inside = isPointerInside(event);
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
            bool inside = isPointerInside(event);
            update(state->pressed && inside, inside);
            return true;
          }
          case PrimeFrame::EventType::KeyDown:
            if (isActivationKey(event.key)) {
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
      node->callbacks = runtimeFrame.addCallback(std::move(callback));
      applyStyle(false, false);
    }
  }

  Internal::configureInteractiveRoot(runtime, button.nodeId());

  if (spec.visible && enabled) {
    Internal::InternalFocusStyle focusStyle = Internal::resolveFocusStyle(runtimeFrame,
                                                                          spec.focusStyle,
                                                                          spec.focusStyleOverride,
                                                                          pressedToken,
                                                                          hoverToken,
                                                                          baseToken,
                                                                          0,
                                                                          0,
                                                                          spec.backgroundStyleOverride);
    Internal::attachFocusOverlay(runtime,
                                 button.nodeId(),
                                 Internal::InternalRect{0.0f, 0.0f, bounds.width, bounds.height},
                                 focusStyle);
  }

  if (!enabled) {
    Internal::addDisabledScrimOverlay(runtime,
                                      button.nodeId(),
                                      Internal::InternalRect{0.0f, 0.0f, bounds.width, bounds.height});
  }

  return UiNode(runtimeFrame, button.nodeId(), runtime.allowAbsolute);
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

} // namespace PrimeStage
