#include "PrimeStage/PrimeStage.h"

#include "PrimeStageCollectionInternals.h"
#include "PrimeFrame/Events.h"

#include <algorithm>
#include <memory>

namespace PrimeStage {
namespace {

constexpr int KeyLeft = keyCodeInt(KeyCode::Left);
constexpr int KeyRight = keyCodeInt(KeyCode::Right);
constexpr int KeyDown = keyCodeInt(KeyCode::Down);
constexpr int KeyUp = keyCodeInt(KeyCode::Up);
constexpr int KeyHome = keyCodeInt(KeyCode::Home);
constexpr int KeyEnd = keyCodeInt(KeyCode::End);

} // namespace

UiNode UiNode::createProgressBar(ProgressBarSpec const& specInput) {
  ProgressBarSpec spec = Internal::normalizeProgressBarSpec(specInput);
  bool enabled = spec.enabled;

  Internal::InternalRect bounds = Internal::resolveRect(spec.size);
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
  if (PrimeFrame::Node* node = frame().getNode(bar.nodeId())) {
    node->focusable = enabled;
    node->hitTestVisible = enabled;
    node->tabIndex = enabled ? spec.tabIndex : -1;
  }

  auto computeFillWidth = [boundsW = bounds.width, minFillW = spec.minFillWidth](float value) {
    float clamped = std::clamp(value, 0.0f, 1.0f);
    float fillW = boundsW * clamped;
    if (minFillW > 0.0f) {
      fillW = std::max(fillW, minFillW);
    }
    return std::min(fillW, boundsW);
  };
  float value = std::clamp(spec.value, 0.0f, 1.0f);
  float fillW = computeFillWidth(value);
  bool needsPatchState = enabled ||
                         spec.binding.state != nullptr ||
                         spec.state != nullptr ||
                         static_cast<bool>(spec.callbacks.onChange) ||
                         static_cast<bool>(spec.callbacks.onValueChanged);

  PrimeFrame::NodeId fillNodeId{};
  if (fillW > 0.0f || needsPatchState) {
    fillNodeId = Internal::createRectNode(frame(),
                                          bar.nodeId(),
                                          Internal::InternalRect{0.0f, 0.0f, fillW, bounds.height},
                                          spec.fillStyle,
                                          spec.fillStyleOverride,
                                          false,
                                          spec.visible);
  }
  auto applyProgressVisual = [framePtr = &frame(),
                              fillNodeId,
                              boundsH = bounds.height,
                              fillBaseOverride = spec.fillStyleOverride,
                              computeFillWidth](float nextValue) {
    if (!fillNodeId.isValid()) {
      return;
    }
    float fillWInner = computeFillWidth(nextValue);
    if (PrimeFrame::Node* fillNode = framePtr->getNode(fillNodeId)) {
      fillNode->localX = 0.0f;
      fillNode->localY = 0.0f;
      fillNode->sizeHint.width.preferred = fillWInner;
      fillNode->sizeHint.height.preferred = boundsH;
      fillNode->visible = fillWInner > 0.0f && boundsH > 0.0f;
    }
    if (PrimeFrame::Node* fillNode = framePtr->getNode(fillNodeId);
        fillNode && !fillNode->primitives.empty()) {
      if (PrimeFrame::Primitive* fillPrim = framePtr->getPrimitive(fillNode->primitives.front())) {
        fillPrim->rect.overrideStyle = fillBaseOverride;
        fillPrim->width = fillWInner;
        fillPrim->height = boundsH;
        if (fillWInner <= 0.0f || boundsH <= 0.0f) {
          fillPrim->rect.overrideStyle.opacity = 0.0f;
        }
      }
    }
  };
  applyProgressVisual(value);

  if (enabled && needsPatchState) {
    struct ProgressBarInteractionState {
      bool pressed = false;
      float value = 0.0f;
    };
    auto state = std::make_shared<ProgressBarInteractionState>();
    state->value = value;
    auto setValue = [state,
                     bindingState = spec.binding.state,
                     progressState = spec.state,
                     onChange = spec.callbacks.onChange,
                     onChanged = spec.callbacks.onValueChanged,
                     applyProgressVisual](float nextValue) {
      float clamped = std::clamp(nextValue, 0.0f, 1.0f);
      state->value = clamped;
      if (bindingState) {
        bindingState->value = clamped;
      }
      if (progressState) {
        progressState->value = clamped;
      }
      applyProgressVisual(clamped);
      if (onChange) {
        onChange(clamped);
      } else if (onChanged) {
        onChanged(clamped);
      }
    };
    PrimeFrame::Callback callback;
    callback.onEvent = [state, setValue](PrimeFrame::Event const& event) mutable -> bool {
      switch (event.type) {
        case PrimeFrame::EventType::PointerDown:
          state->pressed = true;
          setValue(Internal::sliderValueFromEvent(event, false, 0.0f));
          return true;
        case PrimeFrame::EventType::PointerDrag:
        case PrimeFrame::EventType::PointerMove:
          if (state->pressed) {
            setValue(Internal::sliderValueFromEvent(event, false, 0.0f));
            return true;
          }
          break;
        case PrimeFrame::EventType::PointerUp:
          if (state->pressed) {
            setValue(Internal::sliderValueFromEvent(event, false, 0.0f));
          }
          state->pressed = false;
          return true;
        case PrimeFrame::EventType::PointerCancel:
        case PrimeFrame::EventType::PointerLeave:
          state->pressed = false;
          return true;
        case PrimeFrame::EventType::KeyDown:
          if (event.key == KeyLeft || event.key == KeyDown) {
            setValue(state->value - 0.05f);
            return true;
          }
          if (event.key == KeyRight || event.key == KeyUp) {
            setValue(state->value + 0.05f);
            return true;
          }
          if (event.key == KeyHome) {
            setValue(0.0f);
            return true;
          }
          if (event.key == KeyEnd) {
            setValue(1.0f);
            return true;
          }
          break;
        default:
          break;
      }
      return false;
    };
    if (PrimeFrame::Node* node = frame().getNode(bar.nodeId())) {
      node->callbacks = frame().addCallback(std::move(callback));
    }
  }

  if (spec.visible && enabled) {
    Internal::InternalFocusStyle focusStyle = Internal::resolveFocusStyle(frame(),
                                                                          spec.focusStyle,
                                                                          spec.focusStyleOverride,
                                                                          spec.trackStyle,
                                                                          spec.fillStyle,
                                                                          0,
                                                                          0,
                                                                          0,
                                                                          spec.trackStyleOverride);
    Internal::attachFocusOverlay(frame(),
                                 bar.nodeId(),
                                 Internal::InternalRect{0.0f, 0.0f, bounds.width, bounds.height},
                                 focusStyle,
                                 spec.visible);
  }

  if (!enabled) {
    Internal::addDisabledScrimOverlay(frame(),
                                      bar.nodeId(),
                                      Internal::InternalRect{0.0f, 0.0f, bounds.width, bounds.height},
                                      spec.visible);
  }

  return UiNode(frame(), bar.nodeId(), allowAbsolute_);
}

UiNode UiNode::createProgressBar(Binding<float> binding) {
  ProgressBarSpec spec;
  spec.binding = binding;
  return createProgressBar(spec);
}

} // namespace PrimeStage
