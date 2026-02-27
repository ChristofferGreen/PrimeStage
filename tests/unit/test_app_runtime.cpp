#include "PrimeStage/App.h"
#include "PrimeStage/AppRuntime.h"

#include "third_party/doctest.h"

#include <array>
#include <string>
#include <vector>

TEST_CASE("FrameLifecycle defaults to pending rebuild layout and frame") {
  PrimeStage::FrameLifecycle runtime;
  CHECK(runtime.rebuildPending());
  CHECK(runtime.layoutPending());
  CHECK(runtime.framePending());
}

TEST_CASE("FrameLifecycle consumes rebuild and layout work deterministically") {
  PrimeStage::FrameLifecycle runtime;

  int rebuildCalls = 0;
  CHECK(runtime.runRebuildIfNeeded([&]() { rebuildCalls += 1; }));
  CHECK(rebuildCalls == 1);
  CHECK_FALSE(runtime.rebuildPending());
  CHECK(runtime.layoutPending());
  CHECK(runtime.framePending());

  CHECK_FALSE(runtime.runRebuildIfNeeded([&]() { rebuildCalls += 1; }));
  CHECK(rebuildCalls == 1);

  int layoutCalls = 0;
  CHECK(runtime.runLayoutIfNeeded([&]() { layoutCalls += 1; }));
  CHECK(layoutCalls == 1);
  CHECK_FALSE(runtime.layoutPending());
  CHECK(runtime.framePending());

  CHECK_FALSE(runtime.runLayoutIfNeeded([&]() { layoutCalls += 1; }));
  CHECK(layoutCalls == 1);
}

TEST_CASE("FrameLifecycle request and presentation transitions update pending flags") {
  PrimeStage::FrameLifecycle runtime;

  runtime.runRebuildIfNeeded([]() {});
  runtime.runLayoutIfNeeded([]() {});
  CHECK(runtime.framePending());

  runtime.markFramePresented();
  CHECK_FALSE(runtime.framePending());
  CHECK_FALSE(runtime.rebuildPending());
  CHECK_FALSE(runtime.layoutPending());

  runtime.requestFrame();
  CHECK(runtime.framePending());
  CHECK_FALSE(runtime.rebuildPending());
  CHECK_FALSE(runtime.layoutPending());

  runtime.markFramePresented();
  runtime.requestLayout();
  CHECK(runtime.layoutPending());
  CHECK(runtime.framePending());
  CHECK_FALSE(runtime.rebuildPending());

  runtime.runLayoutIfNeeded([]() {});
  CHECK_FALSE(runtime.layoutPending());
  CHECK_FALSE(runtime.rebuildPending());
  CHECK(runtime.framePending());

  runtime.markFramePresented();
  runtime.requestRebuild();
  CHECK(runtime.rebuildPending());
  CHECK(runtime.layoutPending());
  CHECK(runtime.framePending());
}

TEST_CASE("App rebuild and layout are driven by the high-level lifecycle") {
  PrimeStage::App app;

  int rebuildCalls = 0;
  PrimeFrame::NodeId buttonId{};
  CHECK(app.runRebuildIfNeeded([&](PrimeStage::UiNode root) {
    rebuildCalls += 1;
    PrimeStage::ButtonSpec button;
    button.label = "Run";
    button.size.preferredWidth = 120.0f;
    button.size.preferredHeight = 28.0f;
    buttonId = root.createButton(button).nodeId();
  }));
  CHECK(rebuildCalls == 1);
  CHECK_FALSE(app.lifecycle().rebuildPending());
  CHECK(app.lifecycle().layoutPending());
  CHECK(app.lifecycle().framePending());

  CHECK_FALSE(app.runRebuildIfNeeded([&](PrimeStage::UiNode) { rebuildCalls += 1; }));
  CHECK(rebuildCalls == 1);

  CHECK(app.runLayoutIfNeeded());
  CHECK_FALSE(app.lifecycle().layoutPending());
  CHECK(app.layout().get(buttonId) != nullptr);
}

TEST_CASE("App dispatchFrameEvent routes through owned router and focus manager") {
  PrimeStage::App app;

  PrimeFrame::NodeId buttonId{};
  CHECK(app.runRebuildIfNeeded([&](PrimeStage::UiNode root) {
    PrimeStage::ButtonSpec button;
    button.label = "Focus";
    button.size.preferredWidth = 100.0f;
    button.size.preferredHeight = 28.0f;
    buttonId = root.createButton(button).nodeId();
  }));
  CHECK(app.runLayoutIfNeeded());

  PrimeFrame::LayoutOut const* layoutOut = app.layout().get(buttonId);
  REQUIRE(layoutOut != nullptr);
  PrimeFrame::Event event;
  event.type = PrimeFrame::EventType::PointerDown;
  event.pointerId = 1;
  event.x = layoutOut->absX + layoutOut->absW * 0.5f;
  event.y = layoutOut->absY + layoutOut->absH * 0.5f;

  (void)app.dispatchFrameEvent(event);
  CHECK(app.focus().focusedNode() == buttonId);
}

TEST_CASE("App typed widget handles drive focus visibility and imperative actions") {
  PrimeStage::App app;

  PrimeFrame::NodeId buttonId{};
  PrimeStage::WidgetFocusHandle focusHandle;
  PrimeStage::WidgetVisibilityHandle visibilityHandle;
  PrimeStage::WidgetActionHandle actionHandle;
  int activateCount = 0;

  CHECK(app.runRebuildIfNeeded([&](PrimeStage::UiNode root) {
    PrimeStage::ButtonSpec button;
    button.label = "Handle";
    button.size.preferredWidth = 100.0f;
    button.size.preferredHeight = 28.0f;
    button.callbacks.onActivate = [&]() { activateCount += 1; };
    PrimeStage::UiNode built = root.createButton(button);
    buttonId = built.lowLevelNodeId();
    focusHandle = built.focusHandle();
    visibilityHandle = built.visibilityHandle();
    actionHandle = built.actionHandle();
  }));
  CHECK(app.runLayoutIfNeeded());

  CHECK(app.focusWidget(focusHandle));
  CHECK(app.isWidgetFocused(focusHandle));

  PrimeFrame::Node const* node = app.frame().getNode(buttonId);
  REQUIRE(node != nullptr);
  CHECK(node->visible);

  CHECK(app.setWidgetVisible(visibilityHandle, false));
  node = app.frame().getNode(buttonId);
  REQUIRE(node != nullptr);
  CHECK_FALSE(node->visible);

  CHECK(app.setWidgetVisible(visibilityHandle, true));
  node = app.frame().getNode(buttonId);
  REQUIRE(node != nullptr);
  CHECK(node->visible);

  CHECK(app.setWidgetHitTestVisible(visibilityHandle, false));
  node = app.frame().getNode(buttonId);
  REQUIRE(node != nullptr);
  CHECK_FALSE(node->hitTestVisible);

  PrimeStage::SizeSpec size;
  size.minWidth = 90.0f;
  size.maxWidth = 180.0f;
  CHECK(app.setWidgetSize(actionHandle, size));
  node = app.frame().getNode(buttonId);
  REQUIRE(node != nullptr);
  REQUIRE(node->sizeHint.width.min.has_value());
  REQUIRE(node->sizeHint.width.max.has_value());
  CHECK(*node->sizeHint.width.min == doctest::Approx(90.0f));
  CHECK(*node->sizeHint.width.max == doctest::Approx(180.0f));

  PrimeFrame::Event event;
  event.type = PrimeFrame::EventType::KeyDown;
  event.key = PrimeStage::keyCodeInt(PrimeStage::KeyCode::Enter);
  CHECK(app.dispatchWidgetEvent(actionHandle, event));
  CHECK(activateCount == 1);

  CHECK_FALSE(app.focusWidget(PrimeStage::WidgetFocusHandle{}));
  CHECK_FALSE(app.setWidgetVisible(PrimeStage::WidgetVisibilityHandle{}, true));
  CHECK_FALSE(app.setWidgetSize(PrimeStage::WidgetActionHandle{}, PrimeStage::SizeSpec{}));
  CHECK_FALSE(app.dispatchWidgetEvent(PrimeStage::WidgetActionHandle{}, event));
}

TEST_CASE("App bridges host input events through the owned input bridge state") {
  PrimeStage::App app;

  PrimeFrame::NodeId buttonId{};
  CHECK(app.runRebuildIfNeeded([&](PrimeStage::UiNode root) {
    PrimeStage::ButtonSpec button;
    button.label = "Bridge";
    button.size.preferredWidth = 100.0f;
    button.size.preferredHeight = 28.0f;
    buttonId = root.createButton(button).nodeId();
  }));
  CHECK(app.runLayoutIfNeeded());

  PrimeFrame::LayoutOut const* layoutOut = app.layout().get(buttonId);
  REQUIRE(layoutOut != nullptr);

  PrimeHost::PointerEvent pointer;
  pointer.pointerId = 3u;
  pointer.x = static_cast<int32_t>(layoutOut->absX + layoutOut->absW * 0.5f);
  pointer.y = static_cast<int32_t>(layoutOut->absY + layoutOut->absH * 0.5f);
  pointer.phase = PrimeHost::PointerPhase::Down;
  PrimeHost::InputEvent input = pointer;
  PrimeHost::EventBatch batch{};

  PrimeStage::InputBridgeResult result = app.bridgeHostInputEvent(input, batch);
  CHECK(result.bypassFrameCap);
  CHECK_FALSE(result.requestExit);
  CHECK(app.focus().focusedNode() == buttonId);
}

TEST_CASE("App platform services apply clipboard and cursor plumbing to text specs") {
  PrimeStage::App app;
  std::string clipboardValue;
  std::vector<PrimeStage::CursorHint> cursorHints;

  PrimeStage::AppPlatformServices services;
  services.textFieldClipboard.setText = [&](std::string_view text) { clipboardValue = std::string(text); };
  services.textFieldClipboard.getText = [&]() { return clipboardValue; };
  services.selectableTextClipboard.setText = [&](std::string_view text) {
    clipboardValue = std::string(text);
  };
  services.onCursorHintChanged = [&](PrimeStage::CursorHint hint) { cursorHints.push_back(hint); };
  app.setPlatformServices(services);

  PrimeStage::TextFieldSpec field;
  app.applyPlatformServices(field);
  REQUIRE(field.clipboard.setText);
  REQUIRE(field.clipboard.getText);
  REQUIRE(field.callbacks.onCursorHintChanged);

  field.clipboard.setText("Prime");
  CHECK(clipboardValue == "Prime");
  CHECK(field.clipboard.getText() == "Prime");
  field.callbacks.onCursorHintChanged(PrimeStage::CursorHint::IBeam);
  CHECK(cursorHints.size() == 1);
  CHECK(cursorHints.back() == PrimeStage::CursorHint::IBeam);

  PrimeStage::SelectableTextSpec selectable;
  app.applyPlatformServices(selectable);
  REQUIRE(selectable.clipboard.setText);
  REQUIRE(selectable.callbacks.onCursorHintChanged);
  selectable.clipboard.setText("Stage");
  CHECK(clipboardValue == "Stage");
  selectable.callbacks.onCursorHintChanged(PrimeStage::CursorHint::Arrow);
  CHECK(cursorHints.size() == 2);
  CHECK(cursorHints.back() == PrimeStage::CursorHint::Arrow);
}

TEST_CASE("App updates IME composition rect from focused layout node") {
  PrimeStage::App app;

  std::array<int32_t, 4> imeRect{0, 0, 0, 0};
  int imeUpdates = 0;
  PrimeStage::AppPlatformServices services;
  services.onImeCompositionRectChanged = [&](int32_t x, int32_t y, int32_t width, int32_t height) {
    imeRect = {x, y, width, height};
    imeUpdates += 1;
  };
  app.setPlatformServices(services);

  PrimeFrame::NodeId fieldId{};
  PrimeFrame::NodeId buttonId{};
  PrimeStage::TextFieldState textState;
  textState.text = "IME";
  textState.cursor = static_cast<uint32_t>(textState.text.size());

  CHECK(app.runRebuildIfNeeded([&](PrimeStage::UiNode root) {
    PrimeStage::StackSpec stack;
    stack.gap = 8.0f;
    stack.size.stretchX = 1.0f;
    stack.size.stretchY = 1.0f;
    PrimeStage::UiNode col = root.createVerticalStack(stack);

    PrimeStage::TextFieldSpec field;
    field.state = &textState;
    field.size.preferredWidth = 180.0f;
    field.size.preferredHeight = 28.0f;
    fieldId = col.createTextField(field).nodeId();

    PrimeStage::ButtonSpec button;
    button.label = "Blur";
    button.size.preferredWidth = 120.0f;
    button.size.preferredHeight = 28.0f;
    buttonId = col.createButton(button).nodeId();
  }));
  CHECK(app.runLayoutIfNeeded());

  PrimeFrame::LayoutOut const* fieldOut = app.layout().get(fieldId);
  PrimeFrame::LayoutOut const* buttonOut = app.layout().get(buttonId);
  REQUIRE(fieldOut != nullptr);
  REQUIRE(buttonOut != nullptr);

  PrimeFrame::Event fieldDown;
  fieldDown.type = PrimeFrame::EventType::PointerDown;
  fieldDown.pointerId = 1;
  fieldDown.x = fieldOut->absX + fieldOut->absW * 0.5f;
  fieldDown.y = fieldOut->absY + fieldOut->absH * 0.5f;
  (void)app.dispatchFrameEvent(fieldDown);
  CHECK(app.focus().focusedNode() == fieldId);
  CHECK(imeUpdates >= 1);
  CHECK(imeRect[2] > 0);
  CHECK(imeRect[3] > 0);

  PrimeFrame::Event buttonDown;
  buttonDown.type = PrimeFrame::EventType::PointerDown;
  buttonDown.pointerId = 2;
  buttonDown.x = buttonOut->absX + buttonOut->absW * 0.5f;
  buttonDown.y = buttonOut->absY + buttonOut->absH * 0.5f;
  (void)app.dispatchFrameEvent(buttonDown);
  CHECK(app.focus().focusedNode() == buttonId);
  CHECK(imeUpdates >= 2);
}
