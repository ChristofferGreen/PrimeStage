#include "PrimeStage/Ui.h"

#include "PrimeFrame/Events.h"
#include "PrimeFrame/Focus.h"
#include "PrimeFrame/Frame.h"
#include "PrimeFrame/Layout.h"

#include "third_party/doctest.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace {

constexpr float RootWidth = 320.0f;
constexpr float RootHeight = 180.0f;

PrimeStage::UiNode createRoot(PrimeFrame::Frame& frame) {
  PrimeFrame::NodeId rootId = frame.createNode();
  frame.addRoot(rootId);
  if (PrimeFrame::Node* node = frame.getNode(rootId)) {
    node->layout = PrimeFrame::LayoutType::Overlay;
    node->sizeHint.width.preferred = RootWidth;
    node->sizeHint.height.preferred = RootHeight;
  }
  return PrimeStage::UiNode(frame, rootId, true);
}

PrimeFrame::LayoutOutput layoutFrame(PrimeFrame::Frame& frame) {
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutEngine engine;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = RootWidth;
  options.rootHeight = RootHeight;
  engine.layout(frame, layout, options);
  return layout;
}

PrimeFrame::Event makePointerEvent(PrimeFrame::EventType type, float x, float y) {
  PrimeFrame::Event event;
  event.type = type;
  event.pointerId = 1;
  event.x = x;
  event.y = y;
  return event;
}

} // namespace

TEST_CASE("PrimeStage button interactions wire through spec callbacks") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  int clickCount = 0;
  PrimeStage::ButtonSpec buttonSpec;
  buttonSpec.label = "Apply";
  buttonSpec.size.preferredWidth = 120.0f;
  buttonSpec.size.preferredHeight = 28.0f;
  buttonSpec.backgroundStyle = 101u;
  buttonSpec.hoverStyle = 102u;
  buttonSpec.pressedStyle = 103u;
  buttonSpec.focusStyle = 104u;
  buttonSpec.callbacks.onClick = [&]() { clickCount += 1; };

  PrimeStage::UiNode button = root.createButton(buttonSpec);
  PrimeFrame::Node const* buttonNode = frame.getNode(button.nodeId());
  REQUIRE(buttonNode != nullptr);
  CHECK(buttonNode->focusable);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame);
  PrimeFrame::LayoutOut const* out = layout.get(button.nodeId());
  REQUIRE(out != nullptr);
  float centerX = out->absX + out->absW * 0.5f;
  float centerY = out->absY + out->absH * 0.5f;

  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, centerX, centerY),
                  frame,
                  layout,
                  &focus);
  CHECK(focus.focusedNode() == button.nodeId());

  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, centerX, centerY),
                  frame,
                  layout,
                  &focus);
  CHECK(clickCount == 1);
}

TEST_CASE("PrimeStage text field editing is owned by app state") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::TextFieldState state;
  state.text = "Prime";
  state.cursor = static_cast<uint32_t>(state.text.size());
  state.focused = true;

  int stateChangedCount = 0;
  std::string lastText;

  PrimeStage::TextFieldSpec fieldSpec;
  fieldSpec.state = &state;
  fieldSpec.size.preferredWidth = 220.0f;
  fieldSpec.size.preferredHeight = 28.0f;
  fieldSpec.callbacks.onStateChanged = [&]() { stateChangedCount += 1; };
  fieldSpec.callbacks.onTextChanged = [&](std::string_view text) { lastText = std::string(text); };

  PrimeStage::UiNode field = root.createTextField(fieldSpec);
  PrimeFrame::Node const* node = frame.getNode(field.nodeId());
  REQUIRE(node != nullptr);
  REQUIRE(node->callbacks != PrimeFrame::InvalidCallbackId);

  PrimeFrame::Callback const* callback = frame.getCallback(node->callbacks);
  REQUIRE(callback != nullptr);
  REQUIRE(callback->onEvent);

  PrimeFrame::Event textInput;
  textInput.type = PrimeFrame::EventType::TextInput;
  textInput.text = " Stage";
  CHECK(callback->onEvent(textInput));
  CHECK(state.text == "Prime Stage");
  CHECK(state.cursor == 11u);
  CHECK(lastText == "Prime Stage");

  PrimeFrame::Event newlineFilteredInput;
  newlineFilteredInput.type = PrimeFrame::EventType::TextInput;
  newlineFilteredInput.text = "\n!";
  CHECK(callback->onEvent(newlineFilteredInput));
  CHECK(state.text == "Prime Stage!");
  CHECK(state.cursor == 12u);
  CHECK(lastText == "Prime Stage!");
  CHECK(stateChangedCount >= 2);
}

TEST_CASE("PrimeStage text field without state is non-editable by default") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::TextFieldSpec fieldSpec;
  fieldSpec.text = "Preview";
  fieldSpec.size.preferredWidth = 180.0f;
  fieldSpec.size.preferredHeight = 24.0f;

  PrimeStage::UiNode field = root.createTextField(fieldSpec);
  PrimeFrame::Node const* node = frame.getNode(field.nodeId());
  REQUIRE(node != nullptr);
  CHECK_FALSE(node->focusable);
  CHECK(node->callbacks == PrimeFrame::InvalidCallbackId);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame);
  PrimeFrame::LayoutOut const* out = layout.get(field.nodeId());
  REQUIRE(out != nullptr);

  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;
  float centerX = out->absX + out->absW * 0.5f;
  float centerY = out->absY + out->absH * 0.5f;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, centerX, centerY),
                  frame,
                  layout,
                  &focus);
  CHECK_FALSE(focus.focusedNode().isValid());
}

TEST_CASE("PrimeStage widgets example uses widget callbacks without PrimeFrame callback plumbing") {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  std::filesystem::path examplePath = repoRoot / "examples" / "primestage_widgets.cpp";
  REQUIRE(std::filesystem::exists(examplePath));

  std::ifstream input(examplePath);
  REQUIRE(input.good());
  std::string source((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
  REQUIRE(!source.empty());

  CHECK(source.find("tabsSpec.callbacks.onTabChanged") != std::string::npos);
  CHECK(source.find("dropdownSpec.callbacks.onOpened") != std::string::npos);
  CHECK(source.find("dropdownSpec.callbacks.onSelected") != std::string::npos);
  CHECK(source.find("widgetIdentity.registerNode") != std::string::npos);
  CHECK(source.find("widgetIdentity.restoreFocus") != std::string::npos);
  CHECK(source.find("bridgeHostInputEvent") != std::string::npos);
  CHECK(source.find("HostKey::Escape") != std::string::npos);

  // App-level widget usage should not rely on raw PrimeFrame callback mutation.
  CHECK(source.find("appendNodeEventCallback") == std::string::npos);
  CHECK(source.find("node->callbacks =") == std::string::npos);
  CHECK(source.find("PrimeFrame::Callback callback") == std::string::npos);
  CHECK(source.find("RestoreFocusTarget") == std::string::npos);
  CHECK(source.find("std::get_if<PrimeHost::PointerEvent>") == std::string::npos);
  CHECK(source.find("std::get_if<PrimeHost::KeyEvent>") == std::string::npos);
  CHECK(source.find("KeyEscape") == std::string::npos);
}

TEST_CASE("PrimeStage appendNodeOnEvent composes without clobbering existing callback") {
  PrimeFrame::Frame frame;
  PrimeFrame::NodeId nodeId = frame.createNode();
  frame.addRoot(nodeId);
  PrimeFrame::Node* node = frame.getNode(nodeId);
  REQUIRE(node != nullptr);

  int previousCalls = 0;
  PrimeFrame::Callback base;
  base.onEvent = [&](PrimeFrame::Event const& event) -> bool {
    previousCalls += 1;
    return event.key == 42;
  };
  node->callbacks = frame.addCallback(std::move(base));

  int appendedCalls = 0;
  bool appended = PrimeStage::appendNodeOnEvent(
      frame,
      nodeId,
      [&](PrimeFrame::Event const& event) -> bool {
        appendedCalls += 1;
        return event.key == 7;
      });
  CHECK(appended);

  PrimeFrame::Callback const* callback = frame.getCallback(node->callbacks);
  REQUIRE(callback != nullptr);
  REQUIRE(callback->onEvent);

  PrimeFrame::Event handledByNew;
  handledByNew.type = PrimeFrame::EventType::KeyDown;
  handledByNew.key = 7;
  CHECK(callback->onEvent(handledByNew));
  CHECK(appendedCalls == 1);
  CHECK(previousCalls == 0);

  PrimeFrame::Event handledByPrevious;
  handledByPrevious.type = PrimeFrame::EventType::KeyDown;
  handledByPrevious.key = 42;
  CHECK(callback->onEvent(handledByPrevious));
  CHECK(appendedCalls == 2);
  CHECK(previousCalls == 1);
}

TEST_CASE("PrimeStage appendNodeOnFocus and appendNodeOnBlur compose callbacks") {
  PrimeFrame::Frame frame;
  PrimeFrame::NodeId nodeId = frame.createNode();
  frame.addRoot(nodeId);
  PrimeFrame::Node* node = frame.getNode(nodeId);
  REQUIRE(node != nullptr);

  int previousFocus = 0;
  int previousBlur = 0;
  PrimeFrame::Callback base;
  base.onFocus = [&]() { previousFocus += 1; };
  base.onBlur = [&]() { previousBlur += 1; };
  node->callbacks = frame.addCallback(std::move(base));

  int appendedFocus = 0;
  int appendedBlur = 0;
  CHECK(PrimeStage::appendNodeOnFocus(frame, nodeId, [&]() { appendedFocus += 1; }));
  CHECK(PrimeStage::appendNodeOnBlur(frame, nodeId, [&]() { appendedBlur += 1; }));

  PrimeFrame::Callback const* callback = frame.getCallback(node->callbacks);
  REQUIRE(callback != nullptr);
  REQUIRE(callback->onFocus);
  REQUIRE(callback->onBlur);

  callback->onFocus();
  callback->onBlur();

  CHECK(previousFocus == 1);
  CHECK(previousBlur == 1);
  CHECK(appendedFocus == 1);
  CHECK(appendedBlur == 1);
}
