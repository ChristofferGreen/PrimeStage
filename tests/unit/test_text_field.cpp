#include "PrimeStage/Ui.h"

#include "PrimeFrame/Events.h"

#include "third_party/doctest.h"

#include <vector>

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

  constexpr int KeyLeft = PrimeStage::keyCodeInt(PrimeStage::KeyCode::Left);
  constexpr int KeyRight = PrimeStage::keyCodeInt(PrimeStage::KeyCode::Right);

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

  constexpr int KeyLeft = PrimeStage::keyCodeInt(PrimeStage::KeyCode::Left);
  constexpr int KeyRight = PrimeStage::keyCodeInt(PrimeStage::KeyCode::Right);

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

TEST_CASE("Text field non-ASCII text input and backspace keep UTF-8 boundaries") {
  PrimeFrame::Frame frame;
  PrimeFrame::NodeId rootId = frame.createNode();
  frame.addRoot(rootId);
  PrimeStage::UiNode root(frame, rootId, true);

  PrimeStage::TextFieldState state;
  state.text = "";
  state.cursor = 0u;
  state.focused = true;

  std::vector<std::string> changes;
  PrimeStage::TextFieldSpec spec;
  spec.state = &state;
  spec.callbacks.onTextChanged = [&](std::string_view text) { changes.emplace_back(text); };
  PrimeStage::UiNode field = root.createTextField(spec);

  PrimeFrame::Node const* node = frame.getNode(field.nodeId());
  REQUIRE(node != nullptr);
  PrimeFrame::Callback const* callback = frame.getCallback(node->callbacks);
  REQUIRE(callback != nullptr);
  REQUIRE(callback->onEvent);

  PrimeFrame::Event input;
  input.type = PrimeFrame::EventType::TextInput;
  input.text = "にほんご";
  CHECK(callback->onEvent(input));
  CHECK(state.text == "にほんご");
  CHECK(state.cursor == static_cast<uint32_t>(state.text.size()));

  PrimeFrame::Event backspace;
  backspace.type = PrimeFrame::EventType::KeyDown;
  backspace.key = PrimeStage::keyCodeInt(PrimeStage::KeyCode::Backspace);
  CHECK(callback->onEvent(backspace));
  CHECK(state.text == "にほん");
  CHECK(state.cursor == static_cast<uint32_t>(state.text.size()));

  CHECK_FALSE(changes.empty());
  CHECK(changes.front() == "にほんご");
  CHECK(changes.back() == "にほん");
}

TEST_CASE("Text field supports composition-like replacement workflows with UTF-8 text") {
  PrimeFrame::Frame frame;
  PrimeFrame::NodeId rootId = frame.createNode();
  frame.addRoot(rootId);
  PrimeStage::UiNode root(frame, rootId, true);

  PrimeStage::TextFieldState state;
  state.text = "";
  state.cursor = 0u;
  state.focused = true;

  PrimeStage::TextFieldSpec spec;
  spec.state = &state;
  PrimeStage::UiNode field = root.createTextField(spec);

  PrimeFrame::Node const* node = frame.getNode(field.nodeId());
  REQUIRE(node != nullptr);
  PrimeFrame::Callback const* callback = frame.getCallback(node->callbacks);
  REQUIRE(callback != nullptr);
  REQUIRE(callback->onEvent);

  PrimeFrame::Event input;
  input.type = PrimeFrame::EventType::TextInput;

  // Provisional ASCII preedit-style commit.
  input.text = "n";
  CHECK(callback->onEvent(input));
  CHECK(state.text == "n");

  // Replace provisional text with first committed kana candidate.
  state.selectionAnchor = 0u;
  state.selectionStart = 0u;
  state.selectionEnd = static_cast<uint32_t>(state.text.size());
  state.cursor = state.selectionEnd;
  input.text = "に";
  CHECK(callback->onEvent(input));
  CHECK(state.text == "に");

  // Replace again with longer candidate.
  state.selectionAnchor = 0u;
  state.selectionStart = 0u;
  state.selectionEnd = static_cast<uint32_t>(state.text.size());
  state.cursor = state.selectionEnd;
  input.text = "日本語";
  CHECK(callback->onEvent(input));
  CHECK(state.text == "日本語");
  CHECK(state.cursor == static_cast<uint32_t>(state.text.size()));
}
