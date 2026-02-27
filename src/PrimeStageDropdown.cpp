#include "PrimeStage/PrimeStage.h"

#include "PrimeStageCollectionInternals.h"
#include "PrimeFrame/Events.h"

#include <algorithm>
#include <memory>

namespace PrimeStage {
namespace {

constexpr int KeyEnter = keyCodeInt(KeyCode::Enter);
constexpr int KeySpace = keyCodeInt(KeyCode::Space);
constexpr int KeyDown = keyCodeInt(KeyCode::Down);
constexpr int KeyUp = keyCodeInt(KeyCode::Up);

bool isActivationKey(int key) {
  return key == KeyEnter || key == KeySpace;
}

bool isPointerInside(PrimeFrame::Event const& event) {
  return event.localX >= 0.0f && event.localX <= event.targetW &&
         event.localY >= 0.0f && event.localY <= event.targetH;
}

} // namespace

UiNode UiNode::createDropdown(DropdownSpec const& specInput) {
  DropdownSpec spec = Internal::normalizeDropdownSpec(specInput);
  bool enabled = spec.enabled;

  int optionCount = static_cast<int>(spec.options.size());
  int selectedIndex = spec.selectedIndex;
  std::string_view selectedLabel = spec.label;
  if (optionCount > 0) {
    selectedLabel = spec.options[static_cast<size_t>(selectedIndex)];
  }

  Internal::InternalRect bounds = Internal::resolveRect(spec.size);
  float lineHeight = Internal::resolveLineHeight(frame(), spec.textStyle);
  if (bounds.height <= 0.0f &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchY <= 0.0f) {
    bounds.height = lineHeight + spec.paddingX;
  }
  if (bounds.width <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f) {
    float labelWidth = 0.0f;
    if (optionCount > 0) {
      for (std::string_view option : spec.options) {
        labelWidth = std::max(labelWidth, Internal::estimateTextWidth(frame(), spec.textStyle, option));
      }
    } else if (!selectedLabel.empty()) {
      labelWidth = Internal::estimateTextWidth(frame(), spec.textStyle, selectedLabel);
    }
    float indicatorWidth = Internal::estimateTextWidth(frame(), spec.indicatorStyle, spec.indicator);
    float gap = selectedLabel.empty() ? 0.0f : spec.indicatorGap;
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

  if (!selectedLabel.empty()) {
    TextLineSpec labelText;
    labelText.text = selectedLabel;
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

  if (!spec.visible) {
    return UiNode(frame(), dropdown.nodeId(), allowAbsolute_);
  }

  PrimeFrame::Node* dropdownNode = frame().getNode(dropdown.nodeId());
  if (dropdownNode) {
    dropdownNode->focusable = enabled;
    dropdownNode->hitTestVisible = enabled;
    dropdownNode->tabIndex = enabled ? spec.tabIndex : -1;
  }
  if (dropdownNode && enabled) {
    struct DropdownInteractionState {
      bool pressed = false;
      int currentIndex = 0;
    };
    auto state = std::make_shared<DropdownInteractionState>();
    state->currentIndex = selectedIndex;
    PrimeFrame::Callback callback;
    callback.onEvent = [callbacks = spec.callbacks,
                        bindingState = spec.binding.state,
                        dropdownState = spec.state,
                        optionCount,
                        state](PrimeFrame::Event const& event) mutable -> bool {
      auto selectWithStep = [&](int step) {
        if (callbacks.onOpen) {
          callbacks.onOpen();
        } else if (callbacks.onOpened) {
          callbacks.onOpened();
        }
        if (optionCount <= 0) {
          return;
        }
        int span = optionCount;
        int index = state->currentIndex + step;
        index %= span;
        if (index < 0) {
          index += span;
        }
        state->currentIndex = index;
        if (bindingState) {
          bindingState->value = state->currentIndex;
        }
        if (dropdownState) {
          dropdownState->selectedIndex = state->currentIndex;
        }
        if (callbacks.onSelect) {
          callbacks.onSelect(state->currentIndex);
        } else if (callbacks.onSelected) {
          callbacks.onSelected(state->currentIndex);
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
            selectWithStep(1);
          }
          return true;
        }
        case PrimeFrame::EventType::PointerCancel:
        case PrimeFrame::EventType::PointerLeave:
          state->pressed = false;
          return true;
        case PrimeFrame::EventType::KeyDown:
          if (isActivationKey(event.key) || event.key == KeyDown) {
            selectWithStep(1);
            return true;
          }
          if (event.key == KeyUp) {
            selectWithStep(-1);
            return true;
          }
          break;
        default:
          break;
      }
      return false;
    };
    dropdownNode->callbacks = frame().addCallback(std::move(callback));
  }

  if (spec.visible && enabled) {
    Internal::InternalFocusStyle focusStyle = Internal::resolveFocusStyle(frame(),
                                                                          spec.focusStyle,
                                                                          spec.focusStyleOverride,
                                                                          spec.backgroundStyle,
                                                                          0,
                                                                          0,
                                                                          0,
                                                                          0,
                                                                          spec.backgroundStyleOverride);
    Internal::attachFocusOverlay(frame(),
                                 dropdown.nodeId(),
                                 Internal::InternalRect{0.0f, 0.0f, bounds.width, bounds.height},
                                 focusStyle,
                                 spec.visible);
  }

  if (!enabled) {
    Internal::addDisabledScrimOverlay(frame(),
                                      dropdown.nodeId(),
                                      Internal::InternalRect{0.0f, 0.0f, bounds.width, bounds.height},
                                      spec.visible);
  }

  return UiNode(frame(), dropdown.nodeId(), allowAbsolute_);
}

UiNode UiNode::createDropdown(std::vector<std::string_view> options, Binding<int> binding) {
  DropdownSpec spec;
  spec.options = std::move(options);
  spec.binding = binding;
  return createDropdown(spec);
}

} // namespace PrimeStage
