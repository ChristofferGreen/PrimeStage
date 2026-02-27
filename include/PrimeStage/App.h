#pragma once

#include "PrimeStage/AppRuntime.h"
#include "PrimeStage/InputBridge.h"
#include "PrimeStage/Render.h"
#include "PrimeStage/Ui.h"

#include <functional>
#include <string_view>

namespace PrimeStage {

class App {
public:
  App() = default;

  [[nodiscard]] FrameLifecycle& lifecycle() { return lifecycle_; }
  [[nodiscard]] FrameLifecycle const& lifecycle() const { return lifecycle_; }

  [[nodiscard]] InputBridgeState& inputBridge() { return inputBridge_; }
  [[nodiscard]] InputBridgeState const& inputBridge() const { return inputBridge_; }

  [[nodiscard]] RenderOptions& renderOptions() { return renderOptions_; }
  [[nodiscard]] RenderOptions const& renderOptions() const { return renderOptions_; }
  void setRenderOptions(RenderOptions const& options) { renderOptions_ = options; }

  void setSurfaceMetrics(uint32_t width, uint32_t height, float scale = 1.0f);
  void setRenderMetrics(uint32_t width, uint32_t height, float scale = 1.0f);

  [[nodiscard]] bool runRebuildIfNeeded(std::function<void(UiNode)> const& rebuildUi);
  [[nodiscard]] bool runLayoutIfNeeded();
  [[nodiscard]] bool dispatchFrameEvent(PrimeFrame::Event const& event);
  [[nodiscard]] InputBridgeResult bridgeHostInputEvent(PrimeHost::InputEvent const& input,
                                                       PrimeHost::EventBatch const& batch,
                                                       HostKey exitKey = HostKey::Escape);

  [[nodiscard]] RenderStatus renderToTarget(RenderTarget const& target);
  [[nodiscard]] RenderStatus renderToPng(std::string_view path);

  void markFramePresented() { lifecycle_.markFramePresented(); }

  [[nodiscard]] PrimeFrame::Frame& frame() { return frame_; }
  [[nodiscard]] PrimeFrame::Frame const& frame() const { return frame_; }
  [[nodiscard]] PrimeFrame::LayoutOutput& layout() { return layout_; }
  [[nodiscard]] PrimeFrame::LayoutOutput const& layout() const { return layout_; }
  [[nodiscard]] PrimeFrame::FocusManager& focus() { return focus_; }
  [[nodiscard]] PrimeFrame::FocusManager const& focus() const { return focus_; }
  [[nodiscard]] PrimeFrame::EventRouter& router() { return router_; }
  [[nodiscard]] PrimeFrame::EventRouter const& router() const { return router_; }

private:
  [[nodiscard]] float resolvedLayoutScale() const;
  [[nodiscard]] uint32_t resolvedLayoutWidth() const;
  [[nodiscard]] uint32_t resolvedLayoutHeight() const;

  PrimeFrame::Frame frame_{};
  PrimeFrame::LayoutEngine layoutEngine_{};
  PrimeFrame::LayoutOutput layout_{};
  PrimeFrame::EventRouter router_{};
  PrimeFrame::FocusManager focus_{};
  FrameLifecycle lifecycle_{};
  InputBridgeState inputBridge_{};
  RenderOptions renderOptions_{};
  uint32_t surfaceWidth_ = 1280u;
  uint32_t surfaceHeight_ = 720u;
  float surfaceScale_ = 1.0f;
  uint32_t renderWidth_ = 0u;
  uint32_t renderHeight_ = 0u;
  float renderScale_ = 1.0f;
};

} // namespace PrimeStage
