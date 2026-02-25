#include "PrimeStage/Ui.h"

#include "PrimeFrame/Events.h"

#include "third_party/doctest.h"

TEST_CASE("Text field arrow keys move cursor by one") {
  PrimeFrame::Frame frame;
  PrimeFrame::NodeId rootId = frame.createNode();
  frame.addRoot(rootId);
  PrimeStage::UiNode root(frame, rootId, true);

  PrimeStage::TextFieldState state;
  state.text = "Hello";
  state.cursor = 5u;
  state.focused = true;

  PrimeStage::TextFieldSpec spec;
  spec.state = &state;
  PrimeStage::UiNode field = root.createTextField(spec);

  PrimeFrame::Node const* node = frame.getNode(field.nodeId());
  REQUIRE(node != nullptr);
  PrimeFrame::Callback const* callback = frame.getCallback(node->callbacks);
  REQUIRE(callback != nullptr);
  REQUIRE(callback->onEvent);

  constexpr int KeyLeft = 0x50;
  constexpr int KeyRight = 0x4F;

  PrimeFrame::Event event;
  event.type = PrimeFrame::EventType::KeyDown;
  event.key = KeyLeft;
  callback->onEvent(event);
  CHECK(state.cursor == 4u);

  event.key = KeyRight;
  callback->onEvent(event);
  CHECK(state.cursor == 5u);
}

TEST_CASE("Text field arrows collapse selection without shift") {
  PrimeFrame::Frame frame;
  PrimeFrame::NodeId rootId = frame.createNode();
  frame.addRoot(rootId);
  PrimeStage::UiNode root(frame, rootId, true);

  PrimeStage::TextFieldState state;
  state.text = "Hello";
  state.cursor = 2u;
  state.selectionStart = 0u;
  state.selectionEnd = 5u;
  state.focused = true;

  PrimeStage::TextFieldSpec spec;
  spec.state = &state;
  PrimeStage::UiNode field = root.createTextField(spec);

  PrimeFrame::Node const* node = frame.getNode(field.nodeId());
  REQUIRE(node != nullptr);
  PrimeFrame::Callback const* callback = frame.getCallback(node->callbacks);
  REQUIRE(callback != nullptr);
  REQUIRE(callback->onEvent);

  constexpr int KeyLeft = 0x50;
  constexpr int KeyRight = 0x4F;

  PrimeFrame::Event event;
  event.type = PrimeFrame::EventType::KeyDown;
  event.key = KeyLeft;
  callback->onEvent(event);
  CHECK(state.cursor == 0u);
  CHECK(state.selectionStart == state.selectionEnd);

  state.selectionStart = 0u;
  state.selectionEnd = 5u;
  state.cursor = 2u;
  event.key = KeyRight;
  callback->onEvent(event);
  CHECK(state.cursor == 5u);
  CHECK(state.selectionStart == state.selectionEnd);
}
