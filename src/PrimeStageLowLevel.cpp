#include "PrimeStage/PrimeStage.h"
#include "PrimeFrame/Focus.h"

#include <cstdio>
#include <memory>
#include <utility>

namespace PrimeStage {

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


} // namespace PrimeStage
