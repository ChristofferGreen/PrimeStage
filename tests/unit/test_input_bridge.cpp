#include "PrimeStage/InputBridge.h"

#include "third_party/doctest.h"

#include <array>
#include <span>
#include <string>

TEST_CASE("Input bridge maps pointer events and updates pointer state") {
  PrimeStage::InputBridgeState state;
  PrimeHost::PointerEvent pointer;
  pointer.pointerId = 7u;
  pointer.x = 25;
  pointer.y = 40;
  pointer.phase = PrimeHost::PointerPhase::Down;
  PrimeHost::InputEvent input = pointer;

  PrimeHost::EventBatch batch{};
  PrimeFrame::Event captured;
  bool dispatched = false;

  PrimeStage::InputBridgeResult result = PrimeStage::bridgeHostInputEvent(
      input,
      batch,
      state,
      [&](PrimeFrame::Event const& event) {
        captured = event;
        dispatched = true;
        return true;
      });

  CHECK(dispatched);
  CHECK(captured.type == PrimeFrame::EventType::PointerDown);
  CHECK(captured.pointerId == 7);
  CHECK(captured.x == doctest::Approx(25.0f));
  CHECK(captured.y == doctest::Approx(40.0f));
  CHECK(state.pointerX == doctest::Approx(25.0f));
  CHECK(state.pointerY == doctest::Approx(40.0f));
  CHECK(result.requestFrame);
  CHECK(result.bypassFrameCap);
  CHECK_FALSE(result.requestExit);
}

TEST_CASE("Input bridge maps key events and uses symbolic escape key") {
  PrimeStage::InputBridgeState state;
  PrimeHost::EventBatch batch{};

  PrimeHost::KeyEvent escape;
  escape.pressed = true;
  escape.keyCode = PrimeStage::hostKeyCode(PrimeStage::HostKey::Escape);
  PrimeHost::InputEvent input = escape;

  bool dispatched = false;
  PrimeStage::InputBridgeResult result = PrimeStage::bridgeHostInputEvent(
      input,
      batch,
      state,
      [&](PrimeFrame::Event const&) {
        dispatched = true;
        return true;
      });
  CHECK_FALSE(dispatched);
  CHECK(result.requestExit);
  CHECK_FALSE(result.requestFrame);

  PrimeHost::KeyEvent keyDown;
  keyDown.pressed = true;
  keyDown.keyCode = PrimeStage::hostKeyCode(PrimeStage::HostKey::Enter);
  keyDown.modifiers = static_cast<PrimeHost::KeyModifierMask>(PrimeHost::KeyModifier::Shift);
  input = keyDown;

  PrimeFrame::Event captured;
  result = PrimeStage::bridgeHostInputEvent(
      input,
      batch,
      state,
      [&](PrimeFrame::Event const& event) {
        captured = event;
        return true;
      });

  CHECK(captured.type == PrimeFrame::EventType::KeyDown);
  CHECK(captured.key == static_cast<int>(PrimeStage::hostKeyCode(PrimeStage::HostKey::Enter)));
  CHECK(captured.modifiers == keyDown.modifiers);
  CHECK_FALSE(result.requestExit);
  CHECK(result.requestFrame);
}

TEST_CASE("Input bridge maps text spans and ignores invalid spans") {
  PrimeStage::InputBridgeState state;
  std::array<char, 8> textBytes{'P', 'r', 'i', 'm', 'e', '!', '\0', '\0'};
  PrimeHost::EventBatch batch{
      std::span<const PrimeHost::Event>{},
      std::span<const char>(textBytes.data(), 6u),
  };

  PrimeHost::TextEvent text;
  text.text.offset = 1u;
  text.text.length = 4u;
  PrimeHost::InputEvent input = text;

  PrimeFrame::Event captured;
  bool dispatched = false;
  PrimeStage::InputBridgeResult result = PrimeStage::bridgeHostInputEvent(
      input,
      batch,
      state,
      [&](PrimeFrame::Event const& event) {
        captured = event;
        dispatched = true;
        return true;
      });

  CHECK(dispatched);
  CHECK(captured.type == PrimeFrame::EventType::TextInput);
  CHECK(captured.text == "rime");
  CHECK(result.requestFrame);

  text.text.offset = 5u;
  text.text.length = 4u;
  input = text;
  dispatched = false;
  result = PrimeStage::bridgeHostInputEvent(
      input,
      batch,
      state,
      [&](PrimeFrame::Event const&) {
        dispatched = true;
        return true;
      });
  CHECK_FALSE(dispatched);
  CHECK_FALSE(result.requestFrame);
}

TEST_CASE("Input bridge maps scroll events using pointer position and line scale") {
  PrimeStage::InputBridgeState state;
  state.pointerX = 12.0f;
  state.pointerY = 34.0f;
  state.scrollLinePixels = 16.0f;

  PrimeHost::ScrollEvent scroll;
  scroll.deltaX = 1.5f;
  scroll.deltaY = -2.0f;
  scroll.isLines = true;
  PrimeHost::InputEvent input = scroll;
  PrimeHost::EventBatch batch{};

  PrimeFrame::Event captured;
  PrimeStage::InputBridgeResult result = PrimeStage::bridgeHostInputEvent(
      input,
      batch,
      state,
      [&](PrimeFrame::Event const& event) {
        captured = event;
        return true;
      });

  CHECK(captured.type == PrimeFrame::EventType::PointerScroll);
  CHECK(captured.x == doctest::Approx(12.0f));
  CHECK(captured.y == doctest::Approx(34.0f));
  CHECK(captured.scrollX == doctest::Approx(24.0f));
  CHECK(captured.scrollY == doctest::Approx(-32.0f));
  CHECK(result.requestFrame);
  CHECK(result.bypassFrameCap);
}

TEST_CASE("Input bridge preserves pixel scroll units and normalizes direction sign") {
  PrimeStage::InputBridgeState state;
  state.pointerX = 48.0f;
  state.pointerY = 96.0f;
  state.scrollLinePixels = 100.0f; // ignored for pixel-mode events
  state.scrollDirectionSign = -1.0f;

  PrimeHost::ScrollEvent scroll;
  scroll.deltaX = 6.0f;
  scroll.deltaY = -3.0f;
  scroll.isLines = false;
  PrimeHost::InputEvent input = scroll;
  PrimeHost::EventBatch batch{};

  PrimeFrame::Event captured;
  PrimeStage::InputBridgeResult result = PrimeStage::bridgeHostInputEvent(
      input,
      batch,
      state,
      [&](PrimeFrame::Event const& event) {
        captured = event;
        return true;
      });

  CHECK(captured.type == PrimeFrame::EventType::PointerScroll);
  CHECK(captured.x == doctest::Approx(48.0f));
  CHECK(captured.y == doctest::Approx(96.0f));
  CHECK(captured.scrollX == doctest::Approx(-6.0f));
  CHECK(captured.scrollY == doctest::Approx(3.0f));
  CHECK(result.requestFrame);
  CHECK(result.bypassFrameCap);
}

TEST_CASE("Input bridge treats non-negative direction sign as default orientation") {
  PrimeStage::InputBridgeState state;
  state.scrollDirectionSign = 0.0f;

  PrimeHost::ScrollEvent scroll;
  scroll.deltaX = 1.0f;
  scroll.deltaY = 2.0f;
  scroll.isLines = true;
  PrimeHost::InputEvent input = scroll;
  PrimeHost::EventBatch batch{};

  PrimeFrame::Event captured;
  PrimeStage::InputBridgeResult result = PrimeStage::bridgeHostInputEvent(
      input,
      batch,
      state,
      [&](PrimeFrame::Event const& event) {
        captured = event;
        return true;
      });

  CHECK(captured.scrollX == doctest::Approx(32.0f));
  CHECK(captured.scrollY == doctest::Approx(64.0f));
  CHECK(result.requestFrame);
}
