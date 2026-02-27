#include "PrimeStage/App.h"

#include <algorithm>
#include <cmath>
#include <span>

namespace PrimeStage {

namespace {

uint32_t clamp_dimension(uint32_t value) {
  return std::max(1u, value);
}

float clamp_scale(float value) {
  return value > 0.0f ? value : 1.0f;
}

PrimeHost::CursorShape cursorShapeForHint(CursorHint hint) {
  switch (hint) {
    case CursorHint::IBeam:
      return PrimeHost::CursorShape::IBeam;
    case CursorHint::Arrow:
    default:
      return PrimeHost::CursorShape::Arrow;
  }
}

int32_t round_to_i32(float value) {
  return static_cast<int32_t>(std::lround(value));
}

} // namespace

void App::setPlatformServices(AppPlatformServices const& services) {
  platformServices_ = services;
}

void App::applyPlatformServices(TextFieldSpec& spec) const {
  if (!spec.clipboard.setText && platformServices_.textFieldClipboard.setText) {
    spec.clipboard.setText = platformServices_.textFieldClipboard.setText;
  }
  if (!spec.clipboard.getText && platformServices_.textFieldClipboard.getText) {
    spec.clipboard.getText = platformServices_.textFieldClipboard.getText;
  }
  if (platformServices_.onCursorHintChanged) {
    auto previous = spec.callbacks.onCursorHintChanged;
    auto onCursorHint = platformServices_.onCursorHintChanged;
    spec.callbacks.onCursorHintChanged = [previous = std::move(previous),
                                          onCursorHint = std::move(onCursorHint)](CursorHint hint) {
      if (previous) {
        previous(hint);
      }
      onCursorHint(hint);
    };
  }
}

void App::applyPlatformServices(SelectableTextSpec& spec) const {
  if (!spec.clipboard.setText && platformServices_.selectableTextClipboard.setText) {
    spec.clipboard.setText = platformServices_.selectableTextClipboard.setText;
  }
  if (platformServices_.onCursorHintChanged) {
    auto previous = spec.callbacks.onCursorHintChanged;
    auto onCursorHint = platformServices_.onCursorHintChanged;
    spec.callbacks.onCursorHintChanged = [previous = std::move(previous),
                                          onCursorHint = std::move(onCursorHint)](CursorHint hint) {
      if (previous) {
        previous(hint);
      }
      onCursorHint(hint);
    };
  }
}

void App::connectHostServices(PrimeHost::Host& host, PrimeHost::SurfaceId surfaceId) {
  AppPlatformServices services = platformServices_;

  services.textFieldClipboard.setText = [&host](std::string_view text) {
    (void)host.setClipboardText(text);
  };
  services.textFieldClipboard.getText = [&host]() -> std::string {
    auto size = host.clipboardTextSize();
    if (!size || size.value() == 0u) {
      return {};
    }
    std::string buffer(size.value(), '\0');
    auto text = host.clipboardText(std::span<char>(buffer.data(), buffer.size()));
    if (!text) {
      return {};
    }
    return std::string(text->data(), text->size());
  };
  services.selectableTextClipboard.setText = services.textFieldClipboard.setText;
  services.onCursorHintChanged = [&host, surfaceId](CursorHint hint) {
    if (!surfaceId.isValid()) {
      return;
    }
    (void)host.setCursorShape(surfaceId, cursorShapeForHint(hint));
  };
  services.onImeCompositionRectChanged = [&host, surfaceId](int32_t x,
                                                             int32_t y,
                                                             int32_t width,
                                                             int32_t height) {
    if (!surfaceId.isValid()) {
      return;
    }
    (void)host.setImeCompositionRect(surfaceId, x, y, width, height);
  };
  setPlatformServices(services);
}

void App::clearHostServices() {
  platformServices_.textFieldClipboard = {};
  platformServices_.selectableTextClipboard = {};
  platformServices_.onCursorHintChanged = {};
  platformServices_.onImeCompositionRectChanged = {};
}

void App::setSurfaceMetrics(uint32_t width, uint32_t height, float scale) {
  uint32_t nextWidth = clamp_dimension(width);
  uint32_t nextHeight = clamp_dimension(height);
  float nextScale = clamp_scale(scale);
  if (surfaceWidth_ == nextWidth && surfaceHeight_ == nextHeight && surfaceScale_ == nextScale) {
    return;
  }
  surfaceWidth_ = nextWidth;
  surfaceHeight_ = nextHeight;
  surfaceScale_ = nextScale;
  lifecycle_.requestLayout();
}

void App::setRenderMetrics(uint32_t width, uint32_t height, float scale) {
  float nextScale = clamp_scale(scale);
  if (renderWidth_ == width && renderHeight_ == height && renderScale_ == nextScale) {
    return;
  }
  renderWidth_ = width;
  renderHeight_ = height;
  renderScale_ = nextScale;
  lifecycle_.requestLayout();
}

bool App::runRebuildIfNeeded(std::function<void(UiNode)> const& rebuildUi) {
  if (!lifecycle_.rebuildPending()) {
    return false;
  }
  frame_ = PrimeFrame::Frame();
  router_.clearAllCaptures();

  PrimeFrame::NodeId rootId = frame_.createNode();
  frame_.addRoot(rootId);
  if (PrimeFrame::Node* rootNode = frame_.getNode(rootId)) {
    rootNode->layout = PrimeFrame::LayoutType::Overlay;
    rootNode->visible = true;
    rootNode->clipChildren = true;
    rootNode->hitTestVisible = false;
  }

  rebuildUi(UiNode(frame_, rootId, true));
  lifecycle_.markRebuildComplete();
  return true;
}

bool App::runLayoutIfNeeded() {
  bool didLayout = lifecycle_.runLayoutIfNeeded([this]() {
    PrimeFrame::LayoutOptions options;
    float scale = resolvedLayoutScale();
    options.rootWidth = static_cast<float>(resolvedLayoutWidth()) / scale;
    options.rootHeight = static_cast<float>(resolvedLayoutHeight()) / scale;
    layoutEngine_.layout(frame_, layout_, options);
    focus_.updateAfterRebuild(frame_, layout_);
  });
  if (didLayout) {
    syncImeCompositionRect();
  }
  return didLayout;
}

bool App::dispatchFrameEvent(PrimeFrame::Event const& event) {
  (void)runLayoutIfNeeded();
  PrimeFrame::NodeId focusedBefore = focus_.focusedNode();
  bool handled = router_.dispatch(event, frame_, layout_, &focus_);
  bool focusChanged = focus_.focusedNode() != focusedBefore;
  if (handled || focusChanged) {
    lifecycle_.requestFrame();
  }
  syncImeCompositionRect();
  return handled || focusChanged;
}

InputBridgeResult App::bridgeHostInputEvent(PrimeHost::InputEvent const& input,
                                            PrimeHost::EventBatch const& batch,
                                            HostKey exitKey) {
  InputBridgeResult result = PrimeStage::bridgeHostInputEvent(
      input,
      batch,
      inputBridge_,
      [this](PrimeFrame::Event const& event) { return dispatchFrameEvent(event); },
      exitKey);
  if (result.requestFrame) {
    lifecycle_.requestFrame();
  }
  return result;
}

bool App::focusWidget(WidgetFocusHandle handle) {
  PrimeFrame::NodeId nodeId = handle.lowLevelNodeId();
  if (!nodeId.isValid()) {
    return false;
  }
  (void)runLayoutIfNeeded();
  bool changed = focus_.setFocus(frame_, layout_, nodeId);
  if (changed) {
    lifecycle_.requestFrame();
  }
  syncImeCompositionRect();
  return changed;
}

bool App::isWidgetFocused(WidgetFocusHandle handle) const {
  PrimeFrame::NodeId nodeId = handle.lowLevelNodeId();
  if (!nodeId.isValid()) {
    return false;
  }
  return focus_.focusedNode() == nodeId;
}

bool App::setWidgetVisible(WidgetVisibilityHandle handle, bool visible) {
  PrimeFrame::NodeId nodeId = handle.lowLevelNodeId();
  PrimeFrame::Node* node = nodeId.isValid() ? frame_.getNode(nodeId) : nullptr;
  if (!node) {
    return false;
  }
  if (node->visible != visible) {
    node->visible = visible;
    lifecycle_.requestLayout();
    syncImeCompositionRect();
  }
  return true;
}

bool App::setWidgetHitTestVisible(WidgetVisibilityHandle handle, bool visible) {
  PrimeFrame::NodeId nodeId = handle.lowLevelNodeId();
  PrimeFrame::Node* node = nodeId.isValid() ? frame_.getNode(nodeId) : nullptr;
  if (!node) {
    return false;
  }
  if (node->hitTestVisible != visible) {
    node->hitTestVisible = visible;
    lifecycle_.requestFrame();
  }
  return true;
}

bool App::setWidgetSize(WidgetActionHandle handle, SizeSpec const& size) {
  PrimeFrame::NodeId nodeId = handle.lowLevelNodeId();
  PrimeFrame::Node* node = nodeId.isValid() ? frame_.getNode(nodeId) : nullptr;
  if (!node) {
    return false;
  }
  UiNode(frame_, nodeId, true).setSize(size);
  lifecycle_.requestLayout();
  syncImeCompositionRect();
  return true;
}

bool App::dispatchWidgetEvent(WidgetActionHandle handle, PrimeFrame::Event const& event) {
  PrimeFrame::NodeId nodeId = handle.lowLevelNodeId();
  PrimeFrame::Node const* node = nodeId.isValid() ? frame_.getNode(nodeId) : nullptr;
  if (!node || node->callbacks == PrimeFrame::InvalidCallbackId) {
    return false;
  }
  PrimeFrame::Callback const* callback = frame_.getCallback(node->callbacks);
  if (!callback || !callback->onEvent) {
    return false;
  }
  bool handled = callback->onEvent(event);
  if (handled) {
    lifecycle_.requestFrame();
  }
  syncImeCompositionRect();
  return handled;
}

RenderStatus App::renderToTarget(RenderTarget const& target) {
  (void)runLayoutIfNeeded();
  return renderFrameToTarget(frame_, layout_, target, renderOptions_);
}

RenderStatus App::renderToPng(std::string_view path) {
  (void)runLayoutIfNeeded();
  return renderFrameToPng(frame_, layout_, path, renderOptions_);
}

float App::resolvedLayoutScale() const {
  if (renderScale_ > 0.0f) {
    return renderScale_;
  }
  return clamp_scale(surfaceScale_);
}

uint32_t App::resolvedLayoutWidth() const {
  if (renderWidth_ > 0u) {
    return renderWidth_;
  }
  return clamp_dimension(surfaceWidth_);
}

uint32_t App::resolvedLayoutHeight() const {
  if (renderHeight_ > 0u) {
    return renderHeight_;
  }
  return clamp_dimension(surfaceHeight_);
}

void App::syncImeCompositionRect() {
  if (!platformServices_.onImeCompositionRectChanged) {
    return;
  }
  PrimeFrame::NodeId focusedNode = focus_.focusedNode();
  PrimeFrame::LayoutOut const* out = focusedNode.isValid() ? layout_.get(focusedNode) : nullptr;
  if (!out) {
    bool changed = imeFocusedNode_.isValid() || imeW_ != 0 || imeH_ != 0 || imeX_ != 0 || imeY_ != 0;
    if (changed) {
      imeFocusedNode_ = PrimeFrame::NodeId{};
      imeX_ = 0;
      imeY_ = 0;
      imeW_ = 0;
      imeH_ = 0;
      platformServices_.onImeCompositionRectChanged(0, 0, 0, 0);
    }
    return;
  }
  int32_t x = round_to_i32(out->absX);
  int32_t y = round_to_i32(out->absY);
  int32_t w = std::max(1, round_to_i32(out->absW));
  int32_t h = std::max(1, round_to_i32(out->absH));
  bool changed = focusedNode != imeFocusedNode_ || x != imeX_ || y != imeY_ || w != imeW_ || h != imeH_;
  if (!changed) {
    return;
  }
  imeFocusedNode_ = focusedNode;
  imeX_ = x;
  imeY_ = y;
  imeW_ = w;
  imeH_ = h;
  platformServices_.onImeCompositionRectChanged(x, y, w, h);
}

} // namespace PrimeStage
