#pragma once

#include "PrimeStage/Ui.h"
#include "PrimeFrame/Events.h"
#include "PrimeHost/Host.h"

#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace PrimeStage {

using HostKey = KeyCode;

constexpr uint32_t hostKeyCode(HostKey key) {
  return keyCodeValue(key);
}

inline bool isHostKeyPressed(PrimeHost::KeyEvent const& event, HostKey key) {
  return event.pressed && event.keyCode == hostKeyCode(key);
}

struct InputBridgeState {
  float pointerX = 0.0f;
  float pointerY = 0.0f;
  // Converts line-based wheel deltas to pixels when PrimeHost marks an event as line units.
  float scrollLinePixels = 32.0f;
  // Normalized PrimeFrame semantics: positive scrollY moves content toward larger Y offsets.
  // Set to -1 when a backend reports opposite-sign vertical deltas.
  float scrollDirectionSign = 1.0f;
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
    float directionSign = state.scrollDirectionSign < 0.0f ? -1.0f : 1.0f;
    frameEvent.scrollX = scroll->deltaX * deltaScale * directionSign;
    frameEvent.scrollY = scroll->deltaY * deltaScale * directionSign;
    result.requestFrame = dispatchFrameEvent(frameEvent);
    result.bypassFrameCap = true;
    return result;
  }

  return result;
}

} // namespace PrimeStage
