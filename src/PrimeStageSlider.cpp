#include "PrimeStage/PrimeStage.h"

#include "PrimeStageCollectionInternals.h"
#include "PrimeFrame/Events.h"

#include <algorithm>
#include <memory>

namespace PrimeStage {
namespace {

struct Rect {
  float x = 0.0f;
  float y = 0.0f;
  float width = 0.0f;
  float height = 0.0f;
};

PrimeFrame::PrimitiveId addRectPrimitiveWithRect(PrimeFrame::Frame& frame,
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

} // namespace

UiNode UiNode::createSlider(SliderSpec const& specInput) {
  SliderSpec spec = Internal::normalizeSliderSpec(specInput);
  bool enabled = spec.enabled;

  Internal::InternalRect bounds = Internal::resolveRect(spec.size);
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
  auto applyGeometry = [vertical = spec.vertical,
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
    auto applyRect = [&](PrimeFrame::PrimitiveId primId,
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
      applyRect(fillPrim, fillRect, fillOverride);
      float clampedThumb = std::min(thumb, std::min(width, height));
      Rect thumbRect{0.0f, 0.0f, 0.0f, 0.0f};
      if (clampedThumb > 0.0f) {
        float thumbY = (1.0f - clamped) * (height - clampedThumb);
        thumbRect = Rect{(width - clampedThumb) * 0.5f, thumbY, clampedThumb, clampedThumb};
      }
      applyRect(thumbPrim, thumbRect, thumbOverride);
    } else {
      float trackH = std::min(height, track);
      float trackY = (height - trackH) * 0.5f;
      float fillW = width * clamped;
      Rect fillRect{0.0f, trackY, fillW, trackH};
      applyRect(fillPrim, fillRect, fillOverride);
      float clampedThumb = std::min(thumb, std::min(width, height));
      Rect thumbRect{0.0f, 0.0f, 0.0f, 0.0f};
      if (clampedThumb > 0.0f) {
        float thumbX = clamped * (width - clampedThumb);
        thumbRect = Rect{thumbX, (height - clampedThumb) * 0.5f, clampedThumb, clampedThumb};
      }
      applyRect(thumbPrim, thumbRect, thumbOverride);
    }
  };

  PrimeFrame::PrimitiveId fillPrim = addRectPrimitiveWithRect(frame(),
                                                              slider.nodeId(),
                                                              Rect{0.0f, 0.0f, 0.0f, 0.0f},
                                                              spec.fillStyle,
                                                              spec.fillStyleOverride);
  PrimeFrame::PrimitiveId thumbPrim = addRectPrimitiveWithRect(frame(),
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
  applyGeometry(frame(),
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

    auto updateFromEvent = [state,
                            vertical = spec.vertical,
                            thumbSize = spec.thumbSize](PrimeFrame::Event const& event) {
      if (event.targetW > 0.0f) {
        state->targetW = event.targetW;
      }
      if (event.targetH > 0.0f) {
        state->targetH = event.targetH;
      }
      float next = Internal::sliderValueFromEvent(event, vertical, thumbSize);
      state->value = std::clamp(next, 0.0f, 1.0f);
    };

    auto notifyValueChanged = [state,
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

    auto buildThumbOverride = [state,
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

    auto buildFillOverride = [state,
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

    auto buildTrackOverride = [state,
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

    auto applyTrackOverride = [framePtr = &frame(), state, buildTrackOverride]() {
      if (!state->trackPrimValid) {
        return;
      }
      if (PrimeFrame::Primitive* prim = framePtr->getPrimitive(state->trackPrim)) {
        prim->rect.overrideStyle = buildTrackOverride();
      }
    };

    PrimeFrame::Callback callback;
    callback.onEvent = [callbacks = spec.callbacks,
                        framePtr = &frame(),
                        state,
                        applyGeometry,
                        updateFromEvent,
                        notifyValueChanged,
                        buildFillOverride,
                        buildThumbOverride,
                        applyTrackOverride](PrimeFrame::Event const& event) -> bool {
      switch (event.type) {
        case PrimeFrame::EventType::PointerEnter:
          state->hovered = true;
          applyTrackOverride();
          applyGeometry(*framePtr,
                        state->fillPrim,
                        state->thumbPrim,
                        state->value,
                        state->targetW,
                        state->targetH,
                        buildFillOverride(),
                        buildThumbOverride());
          return true;
        case PrimeFrame::EventType::PointerLeave:
          state->hovered = false;
          applyTrackOverride();
          applyGeometry(*framePtr,
                        state->fillPrim,
                        state->thumbPrim,
                        state->value,
                        state->targetW,
                        state->targetH,
                        buildFillOverride(),
                        buildThumbOverride());
          return true;
        case PrimeFrame::EventType::PointerDown:
          state->active = true;
          applyTrackOverride();
          updateFromEvent(event);
          applyGeometry(*framePtr,
                        state->fillPrim,
                        state->thumbPrim,
                        state->value,
                        state->targetW,
                        state->targetH,
                        buildFillOverride(),
                        buildThumbOverride());
          if (callbacks.onDragStart) {
            callbacks.onDragStart();
          }
          notifyValueChanged();
          return true;
        case PrimeFrame::EventType::PointerDrag:
        case PrimeFrame::EventType::PointerMove:
          if (!state->active) {
            return false;
          }
          updateFromEvent(event);
          applyGeometry(*framePtr,
                        state->fillPrim,
                        state->thumbPrim,
                        state->value,
                        state->targetW,
                        state->targetH,
                        buildFillOverride(),
                        buildThumbOverride());
          notifyValueChanged();
          return true;
        case PrimeFrame::EventType::PointerUp:
        case PrimeFrame::EventType::PointerCancel:
          if (!state->active) {
            return false;
          }
          updateFromEvent(event);
          applyGeometry(*framePtr,
                        state->fillPrim,
                        state->thumbPrim,
                        state->value,
                        state->targetW,
                        state->targetH,
                        buildFillOverride(),
                        buildThumbOverride());
          notifyValueChanged();
          if (callbacks.onDragEnd) {
            callbacks.onDragEnd();
          }
          state->active = false;
          applyTrackOverride();
          return true;
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

  if (enabled) {
    Internal::InternalFocusStyle focusStyle = Internal::resolveFocusStyle(frame(),
                                                                          spec.focusStyle,
                                                                          spec.focusStyleOverride,
                                                                          spec.thumbStyle,
                                                                          spec.fillStyle,
                                                                          spec.trackStyle,
                                                                          0,
                                                                          0,
                                                                          spec.thumbStyleOverride);
    Internal::attachFocusOverlay(frame(),
                                 slider.nodeId(),
                                 Internal::InternalRect{0.0f, 0.0f, bounds.width, bounds.height},
                                 focusStyle,
                                 spec.visible);
  }

  if (!enabled) {
    Internal::addDisabledScrimOverlay(frame(),
                                      slider.nodeId(),
                                      Internal::InternalRect{0.0f, 0.0f, bounds.width, bounds.height},
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

} // namespace PrimeStage
