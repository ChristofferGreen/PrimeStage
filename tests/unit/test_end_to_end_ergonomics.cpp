#include "PrimeStage/App.h"

#include "third_party/doctest.h"

#include <concepts>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace {

PrimeStage::InputBridgeResult dispatchPointer(PrimeStage::App& app,
                                              PrimeHost::PointerPhase phase,
                                              int32_t x,
                                              int32_t y,
                                              uint32_t pointerId = 1u) {
  PrimeHost::PointerEvent pointer;
  pointer.pointerId = pointerId;
  pointer.x = x;
  pointer.y = y;
  pointer.phase = phase;
  PrimeHost::InputEvent input = pointer;
  PrimeHost::EventBatch batch{};
  return app.bridgeHostInputEvent(input, batch);
}

PrimeStage::InputBridgeResult dispatchKey(PrimeStage::App& app,
                                          PrimeStage::HostKey key,
                                          bool pressed = true) {
  PrimeHost::KeyEvent keyEvent;
  keyEvent.pressed = pressed;
  keyEvent.keyCode = PrimeStage::hostKeyCode(key);
  PrimeHost::InputEvent input = keyEvent;
  PrimeHost::EventBatch batch{};
  return app.bridgeHostInputEvent(input, batch);
}

PrimeStage::InputBridgeResult dispatchText(PrimeStage::App& app, std::string_view text) {
  std::vector<char> bytes(text.begin(), text.end());
  PrimeHost::TextEvent textEvent;
  textEvent.text.offset = 0u;
  textEvent.text.length = static_cast<uint32_t>(bytes.size());
  PrimeHost::InputEvent input = textEvent;
  PrimeHost::EventBatch batch{
      std::span<const PrimeHost::Event>{},
      std::span<const char>(bytes.data(), bytes.size()),
  };
  return app.bridgeHostInputEvent(input, batch);
}

template <typename Node>
concept SupportsDeclarativeConvenienceErgonomics = requires(Node node,
                                                            PrimeStage::State<bool>& boolState,
                                                            PrimeStage::State<float>& floatState,
                                                            PrimeStage::State<int>& intState) {
  { node.column([](PrimeStage::UiNode& child) { child.label("Title"); }) } -> std::same_as<Node>;
  { node.row([](PrimeStage::UiNode& child) { child.button("Run"); }) } -> std::same_as<Node>;
  { node.overlay([](PrimeStage::UiNode& child) { child.panel(); }) } -> std::same_as<Node>;
  { node.form([](PrimeStage::UiNode& form) {
      form.formField("Name", [](PrimeStage::UiNode& field) { field.textLine("Value"); });
    }) } -> std::same_as<Node>;
  { node.toggle(PrimeStage::bind(boolState)) } -> std::same_as<Node>;
  { node.checkbox("Enabled", PrimeStage::bind(boolState)) } -> std::same_as<Node>;
  { node.slider(PrimeStage::bind(floatState)) } -> std::same_as<Node>;
  { node.tabs({"One", "Two"}, PrimeStage::bind(intState)) } -> std::same_as<Node>;
  { node.dropdown({"Preview", "Edit"}, PrimeStage::bind(intState)) } -> std::same_as<Node>;
  { node.progressBar(PrimeStage::bind(floatState)) } -> std::same_as<Node>;
};

}  // namespace

static_assert(SupportsDeclarativeConvenienceErgonomics<PrimeStage::UiNode>);
static_assert(!std::is_invocable_v<decltype(&PrimeStage::UiNode::toggle),
                                   PrimeStage::UiNode&,
                                   PrimeStage::Binding<int>>);

TEST_CASE("PrimeStage end-to-end ergonomics high-level app flow handles mouse keyboard and text input") {
  PrimeStage::App app;

  std::shared_ptr<PrimeStage::TextFieldState> textState =
      std::make_shared<PrimeStage::TextFieldState>();
  PrimeStage::WidgetFocusHandle textHandle;
  int mouseClicks = 0;
  std::string lastText;

  CHECK(app.runRebuildIfNeeded([&](PrimeStage::UiNode root) {
    PrimeStage::StackSpec columnSpec;
    columnSpec.gap = 8.0f;
    columnSpec.size.stretchX = 1.0f;
    columnSpec.size.stretchY = 1.0f;
    root.column(columnSpec, [&](PrimeStage::UiNode& column) {
      PrimeStage::ButtonSpec mouseButton;
      mouseButton.label = "Mouse";
      mouseButton.size.preferredWidth = 120.0f;
      mouseButton.size.preferredHeight = 28.0f;
      mouseButton.callbacks.onActivate = [&]() { mouseClicks += 1; };
      column.createButton(mouseButton);

      PrimeStage::TextFieldSpec field;
      field.ownedState = textState;
      field.size.preferredWidth = 200.0f;
      field.size.preferredHeight = 28.0f;
      field.callbacks.onChange = [&](std::string_view text) { lastText = std::string(text); };
      textHandle = column.createTextField(field).focusHandle();
    });
  }));
  CHECK(app.runLayoutIfNeeded());

  app.markFramePresented();
  CHECK_FALSE(app.lifecycle().framePending());
  PrimeStage::InputBridgeResult mouseDown =
      dispatchPointer(app, PrimeHost::PointerPhase::Down, 24, 14, 1u);
  PrimeStage::InputBridgeResult mouseUp =
      dispatchPointer(app, PrimeHost::PointerPhase::Up, 24, 14, 1u);
  CHECK(mouseDown.requestFrame);
  CHECK(mouseDown.bypassFrameCap);
  CHECK(mouseUp.requestFrame);
  CHECK(mouseUp.bypassFrameCap);
  CHECK(mouseClicks == 1);

  PrimeStage::InputBridgeResult unfocusedBackspace =
      dispatchKey(app, PrimeStage::HostKey::Backspace);
  CHECK_FALSE(unfocusedBackspace.requestFrame);
  CHECK(textState->text.empty());
  CHECK(lastText.empty());

  CHECK(app.focusWidget(textHandle));
  PrimeStage::InputBridgeResult textInput = dispatchText(app, "Prime");
  CHECK(textInput.requestFrame);
  CHECK(textState->text == "Prime");
  CHECK(lastText == "Prime");

  PrimeStage::InputBridgeResult backspace = dispatchKey(app, PrimeStage::HostKey::Backspace);
  CHECK(backspace.requestFrame);
  CHECK(textState->text == "Prim");
  CHECK(lastText == "Prim");
}
