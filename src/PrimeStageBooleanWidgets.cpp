#include "PrimeStage/PrimeStage.h"

#include "PrimeStageCollectionInternals.h"
#include "PrimeFrame/Events.h"

#include <algorithm>
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

UiNode UiNode::createToggle(ToggleSpec const& specInput) {
  ToggleSpec spec = Internal::normalizeToggleSpec(specInput);
  bool enabled = spec.enabled;
  bool on = spec.on;
  Internal::WidgetRuntimeContext runtime = Internal::makeWidgetRuntimeContext(frame(),
                                                                              nodeId(),
                                                                              allowAbsolute(),
                                                                              enabled,
                                                                              spec.visible,
                                                                              spec.tabIndex);
  PrimeFrame::Frame& runtimeFrame = Internal::runtimeFrame(runtime);

  Internal::InternalRect bounds = Internal::resolveRect(spec.size);
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
    return UiNode(runtimeFrame, toggle.nodeId(), runtime.allowAbsolute);
  }

  float inset = std::max(0.0f, spec.knobInset);
  float knobSize = std::max(0.0f, bounds.height - inset * 2.0f);
  float maxX = std::max(0.0f, bounds.width - knobSize);
  float knobX = on ? maxX - inset : inset;
  knobX = std::clamp(knobX, 0.0f, maxX);
  PrimeFrame::NodeId knobNodeId =
      Internal::createRectNode(runtimeFrame,
                               toggle.nodeId(),
                               Internal::InternalRect{knobX, inset, knobSize, knobSize},
                               spec.knobStyle,
                               spec.knobStyleOverride,
                               false,
                               spec.visible);

  auto applyToggleVisual = [framePtr = &runtimeFrame,
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

  Internal::configureInteractiveRoot(runtime, toggle.nodeId());

  Internal::InternalFocusStyle focusStyle = Internal::resolveFocusStyle(runtimeFrame,
                                                                        spec.focusStyle,
                                                                        spec.focusStyleOverride,
                                                                        spec.knobStyle,
                                                                        spec.trackStyle,
                                                                        0,
                                                                        0,
                                                                        0,
                                                                        spec.knobStyleOverride);
  if (spec.visible && enabled) {
    if (PrimeFrame::Node* node = runtimeFrame.getNode(toggle.nodeId())) {
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
              state->pressed = isPointerInside(event);
              return true;
            }
            break;
          case PrimeFrame::EventType::PointerUp: {
            bool fire = state->pressed && isPointerInside(event);
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
            if (isActivationKey(event.key)) {
              activate();
              return true;
            }
            break;
          default:
            break;
        }
        return false;
      };
      node->callbacks = runtimeFrame.addCallback(std::move(callback));
    }
    Internal::attachFocusOverlay(runtime,
                                 toggle.nodeId(),
                                 Internal::InternalRect{0.0f, 0.0f, bounds.width, bounds.height},
                                 focusStyle);
  }

  if (!enabled) {
    Internal::addDisabledScrimOverlay(runtime,
                                      toggle.nodeId(),
                                      Internal::InternalRect{0.0f, 0.0f, bounds.width, bounds.height});
  }

  return UiNode(runtimeFrame, toggle.nodeId(), runtime.allowAbsolute);
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
  CheckboxSpec spec = Internal::normalizeCheckboxSpec(specInput);
  bool enabled = spec.enabled;
  bool checked = spec.checked;
  Internal::WidgetRuntimeContext runtime = Internal::makeWidgetRuntimeContext(frame(),
                                                                              nodeId(),
                                                                              allowAbsolute(),
                                                                              enabled,
                                                                              spec.visible,
                                                                              spec.tabIndex);
  PrimeFrame::Frame& runtimeFrame = Internal::runtimeFrame(runtime);

  Internal::InternalRect bounds = Internal::resolveRect(spec.size);
  float lineHeight = Internal::resolveLineHeight(runtimeFrame, spec.textStyle);
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
                           : Internal::estimateTextWidth(runtimeFrame, spec.textStyle, spec.label);
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
  PrimeFrame::NodeId checkNodeId =
      Internal::createRectNode(runtimeFrame,
                               boxNode.nodeId(),
                               Internal::InternalRect{inset, inset, checkSize, checkSize},
                               spec.checkStyle,
                               spec.checkStyleOverride,
                               false,
                               spec.visible);

  auto applyCheckboxVisual = [framePtr = &runtimeFrame,
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
    if (PrimeFrame::Node* checkNode = runtimeFrame.getNode(checkNodeId)) {
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

  Internal::configureInteractiveRoot(runtime, row.nodeId());

  Internal::InternalFocusStyle focusStyle = Internal::resolveFocusStyle(runtimeFrame,
                                                                        spec.focusStyle,
                                                                        spec.focusStyleOverride,
                                                                        spec.checkStyle,
                                                                        spec.boxStyle,
                                                                        0,
                                                                        0,
                                                                        0,
                                                                        spec.checkStyleOverride);
  if (spec.visible && enabled) {
    if (PrimeFrame::Node* node = runtimeFrame.getNode(row.nodeId())) {
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
              state->pressed = isPointerInside(event);
              return true;
            }
            break;
          case PrimeFrame::EventType::PointerUp: {
            bool fire = state->pressed && isPointerInside(event);
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
            if (isActivationKey(event.key)) {
              activate();
              return true;
            }
            break;
          default:
            break;
        }
        return false;
      };
      node->callbacks = runtimeFrame.addCallback(std::move(callback));
    }
    Internal::attachFocusOverlay(runtime,
                                 row.nodeId(),
                                 Internal::InternalRect{0.0f, 0.0f, bounds.width, bounds.height},
                                 focusStyle);
  }

  if (!enabled) {
    Internal::addDisabledScrimOverlay(runtime,
                                      row.nodeId(),
                                      Internal::InternalRect{0.0f, 0.0f, bounds.width, bounds.height});
  }

  return UiNode(runtimeFrame, row.nodeId(), runtime.allowAbsolute);
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

} // namespace PrimeStage
