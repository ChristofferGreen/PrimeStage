#include "PrimeStage/App.h"

#include <algorithm>

namespace PrimeStage {

namespace {

uint32_t clamp_dimension(uint32_t value) {
  return std::max(1u, value);
}

float clamp_scale(float value) {
  return value > 0.0f ? value : 1.0f;
}

} // namespace

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
  return lifecycle_.runLayoutIfNeeded([this]() {
    PrimeFrame::LayoutOptions options;
    float scale = resolvedLayoutScale();
    options.rootWidth = static_cast<float>(resolvedLayoutWidth()) / scale;
    options.rootHeight = static_cast<float>(resolvedLayoutHeight()) / scale;
    layoutEngine_.layout(frame_, layout_, options);
    focus_.updateAfterRebuild(frame_, layout_);
  });
}

bool App::dispatchFrameEvent(PrimeFrame::Event const& event) {
  (void)runLayoutIfNeeded();
  return router_.dispatch(event, frame_, layout_, &focus_);
}

InputBridgeResult App::bridgeHostInputEvent(PrimeHost::InputEvent const& input,
                                            PrimeHost::EventBatch const& batch,
                                            HostKey exitKey) {
  return PrimeStage::bridgeHostInputEvent(
      input,
      batch,
      inputBridge_,
      [this](PrimeFrame::Event const& event) { return dispatchFrameEvent(event); },
      exitKey);
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

} // namespace PrimeStage
