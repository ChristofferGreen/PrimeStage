#include "PrimeStage/PrimeStage.h"

#include "PrimeStageCollectionInternals.h"
#include "PrimeFrame/Events.h"

#include <algorithm>
#include <memory>

namespace PrimeStage {
namespace {

PrimeFrame::PrimitiveId addRectPrimitive(PrimeFrame::Frame& frame,
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
  return pid;
}

} // namespace

Window UiNode::createWindow(WindowSpec const& specInput) {
  WindowSpec spec = Internal::normalizeWindowSpec(specInput);

  Internal::InternalRect windowRect{spec.positionX, spec.positionY, spec.width, spec.height};
  PrimeFrame::NodeId windowId = Internal::createNode(frame(),
                                                     id_,
                                                     windowRect,
                                                     nullptr,
                                                     PrimeFrame::LayoutType::Overlay,
                                                     PrimeFrame::Insets{},
                                                     0.0f,
                                                     true,
                                                     spec.visible,
                                                     "WindowSpec");
  addRectPrimitive(frame(), windowId, spec.frameStyle, spec.frameStyleOverride);

  PrimeFrame::Node* windowNode = frame().getNode(windowId);
  if (windowNode) {
    windowNode->focusable = spec.focusable;
    windowNode->tabIndex = spec.focusable ? spec.tabIndex : -1;
    windowNode->hitTestVisible = true;
  }

  float titleBarHeight = std::min(spec.titleBarHeight, spec.height);
  Internal::InternalRect titleBarRect{0.0f, 0.0f, spec.width, titleBarHeight};
  PrimeFrame::NodeId titleBarId = Internal::createNode(frame(),
                                                       windowId,
                                                       titleBarRect,
                                                       nullptr,
                                                       PrimeFrame::LayoutType::Overlay,
                                                       PrimeFrame::Insets{},
                                                       0.0f,
                                                       false,
                                                       spec.visible,
                                                       "WindowSpec.titleBar");
  addRectPrimitive(frame(), titleBarId, spec.titleBarStyle, spec.titleBarStyleOverride);
  if (PrimeFrame::Node* titleNode = frame().getNode(titleBarId)) {
    titleNode->hitTestVisible = true;
  }

  if (!spec.title.empty() && titleBarHeight > 0.0f) {
    float titleLineHeight = Internal::resolveLineHeight(frame(), spec.titleTextStyle);
    if (titleLineHeight <= 0.0f) {
      titleLineHeight = titleBarHeight;
    }
    float titleY = (titleBarHeight - titleLineHeight) * 0.5f;
    float titleX = std::max(0.0f, spec.contentPadding);
    float titleW = std::max(0.0f, spec.width - titleX * 2.0f);
    Internal::createTextNode(frame(),
                             titleBarId,
                             Internal::InternalRect{titleX, titleY, titleW, titleLineHeight},
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
  Internal::InternalRect contentRect{0.0f, contentY, spec.width, contentHeight};
  PrimeFrame::NodeId contentId = Internal::createNode(frame(),
                                                      windowId,
                                                      contentRect,
                                                      nullptr,
                                                      PrimeFrame::LayoutType::VerticalStack,
                                                      contentInsets,
                                                      0.0f,
                                                      true,
                                                      spec.visible,
                                                      "WindowSpec.content");
  addRectPrimitive(frame(), contentId, spec.contentStyle, spec.contentStyleOverride);
  if (PrimeFrame::Node* contentNode = frame().getNode(contentId)) {
    contentNode->hitTestVisible = true;
  }

  PrimeFrame::NodeId resizeHandleId{};
  if (spec.resizable && spec.resizeHandleSize > 0.0f) {
    float handleSize = std::min(spec.resizeHandleSize, std::min(spec.width, spec.height));
    float handleX = std::max(0.0f, spec.width - handleSize);
    float handleY = std::max(0.0f, spec.height - handleSize);
    resizeHandleId = Internal::createNode(frame(),
                                          windowId,
                                          Internal::InternalRect{handleX, handleY, handleSize, handleSize},
                                          nullptr,
                                          PrimeFrame::LayoutType::None,
                                          PrimeFrame::Insets{},
                                          0.0f,
                                          false,
                                          spec.visible,
                                          "WindowSpec.resizeHandle");
    addRectPrimitive(frame(), resizeHandleId, spec.resizeHandleStyle, spec.resizeHandleStyleOverride);
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
                                [callbacks = spec.callbacks](PrimeFrame::Event const& event) -> bool {
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
    Internal::InternalFocusStyle focusStyle =
        Internal::resolveFocusStyle(frame(), 0, {}, 0, 0, 0, 0, 0);
    Internal::attachFocusOverlay(frame(),
                                 windowId,
                                 Internal::InternalRect{0.0f, 0.0f, spec.width, spec.height},
                                 focusStyle,
                                 spec.visible);
  }

  return Window{
      UiNode(frame(), windowId, allowAbsolute_),
      UiNode(frame(), titleBarId, allowAbsolute_),
      UiNode(frame(), contentId, allowAbsolute_),
      resizeHandleId,
  };
}

} // namespace PrimeStage
