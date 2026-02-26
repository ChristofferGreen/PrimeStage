#pragma once

#include "PrimeFrame/Events.h"
#include "PrimeHost/Host.h"

#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace PrimeStage {

enum class HostKey : uint32_t {
  Enter = 0x28u,
  Escape = 0x29u,
  Space = 0x2Cu,
  Left = 0x50u,
  Right = 0x4Fu,
  Down = 0x51u,
  Up = 0x52u,
  Home = 0x4Au,
  End = 0x4Du,
  PageUp = 0x4Bu,
  PageDown = 0x4Eu,
};

constexpr uint32_t hostKeyCode(HostKey key) {
  return static_cast<uint32_t>(key);
}

inline bool isHostKeyPressed(PrimeHost::KeyEvent const& event, HostKey key) {
  return event.pressed && event.keyCode == hostKeyCode(key);
}

struct InputBridgeState {
  float pointerX = 0.0f;
  float pointerY = 0.0f;
  float scrollLinePixels = 32.0f;
};

struct InputBridgeResult {
  bool requestFrame = false;
  bool bypassFrameCap = false;
  bool requestExit = false;
};

inline std::optional<std::string_view> textFromHostSpan(PrimeHost::EventBatch const& batch,
                                                         PrimeHost::TextSpan span) {
  if (span.length == 0u) {
    return std::string_view{};
  }
  if (span.offset + span.length > batch.textBytes.size()) {
    return std::nullopt;
  }
  return std::string_view(batch.textBytes.data() + span.offset, span.length);
}

template <typename DispatchFn>
InputBridgeResult bridgeHostInputEvent(PrimeHost::InputEvent const& input,
                                       PrimeHost::EventBatch const& batch,
                                       InputBridgeState& state,
                                       DispatchFn&& dispatchFrameEvent,
                                       HostKey exitKey = HostKey::Escape) {
  InputBridgeResult result;

  if (auto const* pointer = std::get_if<PrimeHost::PointerEvent>(&input)) {
    PrimeFrame::Event frameEvent;
    frameEvent.pointerId = static_cast<int>(pointer->pointerId);
    frameEvent.x = static_cast<float>(pointer->x);
    frameEvent.y = static_cast<float>(pointer->y);
    state.pointerX = frameEvent.x;
    state.pointerY = frameEvent.y;
    switch (pointer->phase) {
      case PrimeHost::PointerPhase::Down:
        frameEvent.type = PrimeFrame::EventType::PointerDown;
        break;
      case PrimeHost::PointerPhase::Move:
        frameEvent.type = PrimeFrame::EventType::PointerMove;
        break;
      case PrimeHost::PointerPhase::Up:
        frameEvent.type = PrimeFrame::EventType::PointerUp;
        break;
      case PrimeHost::PointerPhase::Cancel:
        frameEvent.type = PrimeFrame::EventType::PointerCancel;
        break;
    }
    result.requestFrame = dispatchFrameEvent(frameEvent);
    result.bypassFrameCap = true;
    return result;
  }

  if (auto const* key = std::get_if<PrimeHost::KeyEvent>(&input)) {
    if (isHostKeyPressed(*key, exitKey)) {
      result.requestExit = true;
      return result;
    }
    PrimeFrame::Event frameEvent;
    frameEvent.type = key->pressed ? PrimeFrame::EventType::KeyDown
                                   : PrimeFrame::EventType::KeyUp;
    frameEvent.key = static_cast<int>(key->keyCode);
    frameEvent.modifiers = static_cast<uint32_t>(key->modifiers);
    result.requestFrame = dispatchFrameEvent(frameEvent);
    return result;
  }

  if (auto const* text = std::get_if<PrimeHost::TextEvent>(&input)) {
    std::optional<std::string_view> view = textFromHostSpan(batch, text->text);
    if (!view) {
      return result;
    }
    PrimeFrame::Event frameEvent;
    frameEvent.type = PrimeFrame::EventType::TextInput;
    frameEvent.text = std::string(*view);
    result.requestFrame = dispatchFrameEvent(frameEvent);
    return result;
  }

  if (auto const* scroll = std::get_if<PrimeHost::ScrollEvent>(&input)) {
    PrimeFrame::Event frameEvent;
    frameEvent.type = PrimeFrame::EventType::PointerScroll;
    frameEvent.x = state.pointerX;
    frameEvent.y = state.pointerY;
    float deltaScale = scroll->isLines ? state.scrollLinePixels : 1.0f;
    frameEvent.scrollX = scroll->deltaX * deltaScale;
    frameEvent.scrollY = scroll->deltaY * deltaScale;
    result.requestFrame = dispatchFrameEvent(frameEvent);
    result.bypassFrameCap = true;
    return result;
  }

  return result;
}

} // namespace PrimeStage
