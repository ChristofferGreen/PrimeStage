#pragma once

#include "PrimeStage/AppRuntime.h"
#include "PrimeStage/InputBridge.h"
#include "PrimeStage/Render.h"
#include "PrimeStage/Ui.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace PrimeStage {

struct AppPlatformServices {
  TextFieldClipboard textFieldClipboard{};
  SelectableTextClipboard selectableTextClipboard{};
  std::function<void(CursorHint)> onCursorHintChanged;
  std::function<void(int32_t, int32_t, int32_t, int32_t)> onImeCompositionRectChanged;
};

enum class AppActionSource : uint8_t {
  Programmatic,
  Shortcut,
  Widget,
};

struct AppShortcut {
  HostKey key = HostKey::Enter;
  PrimeHost::KeyModifierMask modifiers = 0u;
  bool allowRepeat = false;
};

constexpr bool operator==(AppShortcut const& lhs, AppShortcut const& rhs) {
  return lhs.key == rhs.key &&
         lhs.modifiers == rhs.modifiers &&
         lhs.allowRepeat == rhs.allowRepeat;
}

struct AppActionInvocation {
  std::string_view actionId;
  AppActionSource source = AppActionSource::Programmatic;
  std::optional<AppShortcut> shortcut;
};

using AppActionCallback = std::function<void(AppActionInvocation const&)>;

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
  [[nodiscard]] AppPlatformServices& platformServices() { return platformServices_; }
  [[nodiscard]] AppPlatformServices const& platformServices() const { return platformServices_; }
  void setPlatformServices(AppPlatformServices const& services);
  void applyPlatformServices(TextFieldSpec& spec) const;
  void applyPlatformServices(SelectableTextSpec& spec) const;
  void connectHostServices(PrimeHost::Host& host, PrimeHost::SurfaceId surfaceId);
  void clearHostServices();
  bool registerAction(std::string_view actionId, AppActionCallback callback);
  bool unregisterAction(std::string_view actionId);
  bool bindShortcut(AppShortcut const& shortcut, std::string_view actionId);
  bool unbindShortcut(AppShortcut const& shortcut);
  bool invokeAction(std::string_view actionId,
                    AppActionSource source = AppActionSource::Programmatic,
                    std::optional<AppShortcut> shortcut = std::nullopt);
  std::function<void()> makeActionCallback(std::string actionId);

  void setSurfaceMetrics(uint32_t width, uint32_t height, float scale = 1.0f);
  void setRenderMetrics(uint32_t width, uint32_t height, float scale = 1.0f);

  [[nodiscard]] bool runRebuildIfNeeded(std::function<void(UiNode)> const& rebuildUi);
  [[nodiscard]] bool runLayoutIfNeeded();
  [[nodiscard]] bool dispatchFrameEvent(PrimeFrame::Event const& event);
  [[nodiscard]] InputBridgeResult bridgeHostInputEvent(PrimeHost::InputEvent const& input,
                                                       PrimeHost::EventBatch const& batch,
                                                       HostKey exitKey = HostKey::Escape);
  [[nodiscard]] bool focusWidget(WidgetFocusHandle handle);
  [[nodiscard]] bool isWidgetFocused(WidgetFocusHandle handle) const;
  [[nodiscard]] bool setWidgetVisible(WidgetVisibilityHandle handle, bool visible);
  [[nodiscard]] bool setWidgetHitTestVisible(WidgetVisibilityHandle handle, bool visible);
  [[nodiscard]] bool setWidgetSize(WidgetActionHandle handle, SizeSpec const& size);
  [[nodiscard]] bool dispatchWidgetEvent(WidgetActionHandle handle,
                                         PrimeFrame::Event const& event);

  [[nodiscard]] RenderStatus renderToTarget(RenderTarget const& target);
  [[nodiscard]] RenderStatus renderToPng(std::string_view path);

  void markFramePresented() { lifecycle_.markFramePresented(); }

  // Low-level escape hatches for advanced runtime integrations.
  [[nodiscard]] PrimeFrame::Frame& frame() { return frame_; }
  [[nodiscard]] PrimeFrame::Frame const& frame() const { return frame_; }
  [[nodiscard]] PrimeFrame::LayoutOutput& layout() { return layout_; }
  [[nodiscard]] PrimeFrame::LayoutOutput const& layout() const { return layout_; }
  [[nodiscard]] PrimeFrame::FocusManager& focus() { return focus_; }
  [[nodiscard]] PrimeFrame::FocusManager const& focus() const { return focus_; }
  [[nodiscard]] PrimeFrame::EventRouter& router() { return router_; }
  [[nodiscard]] PrimeFrame::EventRouter const& router() const { return router_; }

private:
  struct ActionEntry {
    std::string id;
    AppActionCallback callback{};
  };
  struct ShortcutEntry {
    AppShortcut shortcut{};
    std::string actionId;
  };

  [[nodiscard]] ActionEntry* findAction(std::string_view actionId);
  [[nodiscard]] ActionEntry const* findAction(std::string_view actionId) const;
  [[nodiscard]] bool dispatchShortcut(PrimeHost::KeyEvent const& event);
  [[nodiscard]] float resolvedLayoutScale() const;
  [[nodiscard]] uint32_t resolvedLayoutWidth() const;
  [[nodiscard]] uint32_t resolvedLayoutHeight() const;
  void syncImeCompositionRect();

  PrimeFrame::Frame frame_{};
  PrimeFrame::LayoutEngine layoutEngine_{};
  PrimeFrame::LayoutOutput layout_{};
  PrimeFrame::EventRouter router_{};
  PrimeFrame::FocusManager focus_{};
  FrameLifecycle lifecycle_{};
  InputBridgeState inputBridge_{};
  RenderOptions renderOptions_{};
  AppPlatformServices platformServices_{};
  std::vector<ActionEntry> actions_{};
  std::vector<ShortcutEntry> shortcutBindings_{};
  uint32_t surfaceWidth_ = 1280u;
  uint32_t surfaceHeight_ = 720u;
  float surfaceScale_ = 1.0f;
  uint32_t renderWidth_ = 0u;
  uint32_t renderHeight_ = 0u;
  float renderScale_ = 1.0f;
  PrimeFrame::NodeId imeFocusedNode_{};
  int32_t imeX_ = 0;
  int32_t imeY_ = 0;
  int32_t imeW_ = 0;
  int32_t imeH_ = 0;
};

} // namespace PrimeStage
