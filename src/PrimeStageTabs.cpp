#include "PrimeStage/PrimeStage.h"

#include "PrimeStageCollectionInternals.h"
#include "PrimeFrame/Events.h"

#include <algorithm>
#include <memory>

namespace PrimeStage {
namespace {

constexpr int KeyEnter = keyCodeInt(KeyCode::Enter);
constexpr int KeySpace = keyCodeInt(KeyCode::Space);
constexpr int KeyLeft = keyCodeInt(KeyCode::Left);
constexpr int KeyRight = keyCodeInt(KeyCode::Right);
constexpr int KeyDown = keyCodeInt(KeyCode::Down);
constexpr int KeyUp = keyCodeInt(KeyCode::Up);
constexpr int KeyHome = keyCodeInt(KeyCode::Home);
constexpr int KeyEnd = keyCodeInt(KeyCode::End);

bool isActivationKey(int key) {
  return key == KeyEnter || key == KeySpace;
}

bool isPointerInside(PrimeFrame::Event const& event) {
  return event.localX >= 0.0f && event.localX <= event.targetW &&
         event.localY >= 0.0f && event.localY <= event.targetH;
}

} // namespace

UiNode UiNode::createTabs(TabsSpec const& specInput) {
  TabsSpec spec = Internal::normalizeTabsSpec(specInput);
  bool enabled = spec.enabled;
  Internal::WidgetRuntimeContext runtime = Internal::makeWidgetRuntimeContext(frame(),
                                                                              nodeId(),
                                                                              allowAbsolute(),
                                                                              enabled,
                                                                              spec.visible,
                                                                              spec.tabIndex);
  PrimeFrame::Frame& runtimeFrame = Internal::runtimeFrame(runtime);

  int tabCount = static_cast<int>(spec.labels.size());
  int selectedIndex = spec.selectedIndex;

  Internal::InternalRect bounds = Internal::resolveRect(spec.size);
  float lineHeight = Internal::resolveLineHeight(runtimeFrame, spec.textStyle);
  float activeLineHeight = Internal::resolveLineHeight(runtimeFrame, spec.activeTextStyle);
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
      float textWidth = Internal::estimateTextWidth(runtimeFrame, token, spec.labels[i]);
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
  if (PrimeFrame::Node* rowNode = runtimeFrame.getNode(row.nodeId())) {
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

    float textWidth = Internal::estimateTextWidth(runtimeFrame, textToken, spec.labels[i]);
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

    PrimeFrame::Node* tabNode = runtimeFrame.getNode(tab.nodeId());
    if (!tabNode) {
      continue;
    }
    if (!spec.visible) {
      tabNode->focusable = false;
      tabNode->hitTestVisible = enabled;
      tabNode->tabIndex = (enabled && spec.tabIndex >= 0) ? (spec.tabIndex + tabIndex) : -1;
      continue;
    }

    Internal::WidgetRuntimeContext tabRuntime = runtime;
    tabRuntime.tabIndex = (enabled && spec.tabIndex >= 0) ? (spec.tabIndex + tabIndex) : -1;
    Internal::configureInteractiveRoot(tabRuntime, tab.nodeId());
    if (!enabled) {
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
            state->pressed = isPointerInside(event);
            return true;
          }
          break;
        case PrimeFrame::EventType::PointerUp: {
          bool fire = state->pressed && isPointerInside(event);
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
          if (isActivationKey(event.key)) {
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
    tabNode->callbacks = runtimeFrame.addCallback(std::move(callback));

    Internal::InternalFocusStyle focusStyle =
        Internal::resolveFocusStyle(runtimeFrame, 0, {}, 0, 0, 0, 0, 0);
    Internal::attachFocusOverlay(
        tabRuntime,
        tab.nodeId(),
        Internal::InternalRect{0.0f, 0.0f, textWidth + spec.tabPaddingX * 2.0f, bounds.height},
        focusStyle);
  }

  if (!enabled) {
    Internal::addDisabledScrimOverlay(runtime,
                                      row.nodeId(),
                                      Internal::InternalRect{0.0f, 0.0f, bounds.width, bounds.height});
  }

  return UiNode(runtimeFrame, row.nodeId(), runtime.allowAbsolute);
}

UiNode UiNode::createTabs(std::vector<std::string_view> labels, Binding<int> binding) {
  TabsSpec spec;
  spec.labels = std::move(labels);
  spec.binding = binding;
  return createTabs(spec);
}

} // namespace PrimeStage
