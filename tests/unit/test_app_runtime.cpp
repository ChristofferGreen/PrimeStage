#include "PrimeStage/App.h"
#include "PrimeStage/AppRuntime.h"

#include "third_party/doctest.h"

#include <array>
#include <string>
#include <vector>

TEST_CASE("AppShortcut equality compares key modifiers and repeat policy") {
  PrimeStage::AppShortcut baseline;
  baseline.key = PrimeStage::HostKey::Enter;
  baseline.modifiers =
      static_cast<PrimeHost::KeyModifierMask>(PrimeHost::KeyModifier::Control);
  baseline.allowRepeat = false;

  PrimeStage::AppShortcut same = baseline;
  CHECK(baseline == same);

  PrimeStage::AppShortcut differentKey = baseline;
  differentKey.key = PrimeStage::HostKey::Space;
  CHECK_FALSE(baseline == differentKey);

  PrimeStage::AppShortcut differentModifiers = baseline;
  differentModifiers.modifiers =
      static_cast<PrimeHost::KeyModifierMask>(PrimeHost::KeyModifier::Shift);
  CHECK_FALSE(baseline == differentModifiers);

  PrimeStage::AppShortcut differentRepeat = baseline;
  differentRepeat.allowRepeat = true;
  CHECK_FALSE(baseline == differentRepeat);
}

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

TEST_CASE("App render and platform service accessors round-trip state") {
  PrimeStage::App app;

  PrimeStage::RenderOptions renderOptions;
  renderOptions.clear = false;
  renderOptions.clearColor = PrimeStage::Rgba8{1u, 2u, 3u, 4u};
  renderOptions.roundedCorners = false;
  renderOptions.cornerStyle.fallbackRadius = 7.0f;
  app.setRenderOptions(renderOptions);

  CHECK_FALSE(app.renderOptions().clear);
  CHECK(app.renderOptions().clearColor.r == 1u);
  CHECK(app.renderOptions().clearColor.g == 2u);
  CHECK(app.renderOptions().clearColor.b == 3u);
  CHECK(app.renderOptions().clearColor.a == 4u);
  CHECK_FALSE(app.renderOptions().roundedCorners);
  CHECK(app.renderOptions().cornerStyle.fallbackRadius == doctest::Approx(7.0f));

  app.renderOptions().roundedCorners = true;
  app.renderOptions().cornerStyle.fallbackRadius = 9.5f;

  PrimeStage::App const& constApp = app;
  CHECK(constApp.renderOptions().roundedCorners);
  CHECK(constApp.renderOptions().cornerStyle.fallbackRadius == doctest::Approx(9.5f));

  bool cursorChanged = false;
  PrimeStage::AppPlatformServices services;
  services.onCursorHintChanged = [&](PrimeStage::CursorHint hint) {
    cursorChanged = (hint == PrimeStage::CursorHint::Arrow);
  };
  app.setPlatformServices(services);

  REQUIRE(static_cast<bool>(app.platformServices().onCursorHintChanged));
  app.platformServices().onCursorHintChanged(PrimeStage::CursorHint::Arrow);
  CHECK(cursorChanged);
  REQUIRE(static_cast<bool>(constApp.platformServices().onCursorHintChanged));
}

TEST_CASE("App metric setters only request layout when values change") {
  PrimeStage::App app;

  CHECK(app.runRebuildIfNeeded([](PrimeStage::UiNode) {}));
  CHECK(app.runLayoutIfNeeded());
  app.markFramePresented();
  CHECK_FALSE(app.lifecycle().rebuildPending());
  CHECK_FALSE(app.lifecycle().layoutPending());
  CHECK_FALSE(app.lifecycle().framePending());

  app.setSurfaceMetrics(1280u, 720u, 1.0f);
  CHECK_FALSE(app.lifecycle().layoutPending());
  CHECK_FALSE(app.lifecycle().framePending());

  app.setRenderMetrics(0u, 0u, 1.0f);
  CHECK_FALSE(app.lifecycle().layoutPending());
  CHECK_FALSE(app.lifecycle().framePending());

  app.setSurfaceMetrics(640u, 480u, 0.0f);
  CHECK(app.lifecycle().layoutPending());
  CHECK(app.lifecycle().framePending());
  CHECK_FALSE(app.lifecycle().rebuildPending());

  CHECK(app.runLayoutIfNeeded());
  CHECK_FALSE(app.lifecycle().layoutPending());
  CHECK(app.lifecycle().framePending());
  app.markFramePresented();
  CHECK_FALSE(app.lifecycle().framePending());

  app.setRenderMetrics(320u, 200u, 0.0f);
  CHECK(app.lifecycle().layoutPending());
  CHECK(app.lifecycle().framePending());
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

  app.markFramePresented();
  CHECK_FALSE(app.lifecycle().framePending());
  (void)app.dispatchFrameEvent(event);
  CHECK(app.focus().focusedNode() == buttonId);
  CHECK(app.lifecycle().framePending());
}

TEST_CASE("App dispatchFrameEvent leaves lifecycle idle when event is ignored") {
  PrimeStage::App app;

  CHECK(app.runRebuildIfNeeded([](PrimeStage::UiNode) {}));
  CHECK(app.runLayoutIfNeeded());
  app.markFramePresented();
  CHECK_FALSE(app.lifecycle().framePending());

  PrimeFrame::Event event;
  event.type = PrimeFrame::EventType::PointerMove;
  event.pointerId = 7;
  event.x = 4000.0f;
  event.y = 3000.0f;

  CHECK_FALSE(app.dispatchFrameEvent(event));
  CHECK_FALSE(app.lifecycle().framePending());
}

TEST_CASE("App focusWidget no-op does not request an extra frame") {
  PrimeStage::App app;

  PrimeStage::WidgetFocusHandle focusHandle;
  CHECK(app.runRebuildIfNeeded([&](PrimeStage::UiNode root) {
    PrimeStage::ButtonSpec button;
    button.label = "FocusOnce";
    button.size.preferredWidth = 120.0f;
    button.size.preferredHeight = 28.0f;
    focusHandle = root.createButton(button).focusHandle();
  }));
  CHECK(app.runLayoutIfNeeded());
  app.markFramePresented();
  CHECK_FALSE(app.lifecycle().framePending());

  CHECK(app.focusWidget(focusHandle));
  CHECK(app.lifecycle().framePending());
  app.markFramePresented();
  CHECK_FALSE(app.lifecycle().framePending());

  CHECK_FALSE(app.focusWidget(focusHandle));
  CHECK(app.isWidgetFocused(focusHandle));
  CHECK_FALSE(app.lifecycle().framePending());
}

TEST_CASE("App isWidgetFocused returns false for invalid handle") {
  PrimeStage::App app;

  app.markFramePresented();
  CHECK_FALSE(app.lifecycle().framePending());
  CHECK_FALSE(app.isWidgetFocused(PrimeStage::WidgetFocusHandle{}));
  CHECK_FALSE(app.lifecycle().framePending());
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
  CHECK_FALSE(app.setWidgetHitTestVisible(PrimeStage::WidgetVisibilityHandle{}, true));
  CHECK_FALSE(app.setWidgetSize(PrimeStage::WidgetActionHandle{}, PrimeStage::SizeSpec{}));
  CHECK_FALSE(app.dispatchWidgetEvent(PrimeStage::WidgetActionHandle{}, event));
}

TEST_CASE("App widget visibility setters do not request lifecycle work on no-op updates") {
  PrimeStage::App app;

  PrimeFrame::NodeId buttonId{};
  PrimeStage::WidgetVisibilityHandle visibilityHandle;
  CHECK(app.runRebuildIfNeeded([&](PrimeStage::UiNode root) {
    PrimeStage::ButtonSpec button;
    button.label = "NoOp";
    button.size.preferredWidth = 100.0f;
    button.size.preferredHeight = 28.0f;
    PrimeStage::UiNode built = root.createButton(button);
    buttonId = built.lowLevelNodeId();
    visibilityHandle = built.visibilityHandle();
  }));
  CHECK(app.runLayoutIfNeeded());
  app.markFramePresented();
  CHECK_FALSE(app.lifecycle().rebuildPending());
  CHECK_FALSE(app.lifecycle().layoutPending());
  CHECK_FALSE(app.lifecycle().framePending());

  PrimeFrame::Node const* node = app.frame().getNode(buttonId);
  REQUIRE(node != nullptr);
  CHECK(node->visible);
  CHECK(node->hitTestVisible);

  CHECK(app.setWidgetVisible(visibilityHandle, true));
  CHECK_FALSE(app.lifecycle().layoutPending());
  CHECK_FALSE(app.lifecycle().framePending());

  CHECK(app.setWidgetHitTestVisible(visibilityHandle, true));
  CHECK_FALSE(app.lifecycle().layoutPending());
  CHECK_FALSE(app.lifecycle().framePending());

  CHECK(app.setWidgetVisible(visibilityHandle, false));
  CHECK(app.lifecycle().layoutPending());
  CHECK(app.lifecycle().framePending());
}

TEST_CASE("App setWidgetHitTestVisible requests frame only when visibility changes") {
  PrimeStage::App app;

  PrimeStage::WidgetVisibilityHandle visibilityHandle;
  PrimeFrame::NodeId buttonId{};
  CHECK(app.runRebuildIfNeeded([&](PrimeStage::UiNode root) {
    PrimeStage::ButtonSpec button;
    button.label = "HitTest";
    button.size.preferredWidth = 100.0f;
    button.size.preferredHeight = 28.0f;
    PrimeStage::UiNode built = root.createButton(button);
    visibilityHandle = built.visibilityHandle();
    buttonId = built.lowLevelNodeId();
  }));
  CHECK(app.runLayoutIfNeeded());
  app.markFramePresented();
  CHECK_FALSE(app.lifecycle().layoutPending());
  CHECK_FALSE(app.lifecycle().framePending());

  CHECK(app.setWidgetHitTestVisible(visibilityHandle, false));
  PrimeFrame::Node const* node = app.frame().getNode(buttonId);
  REQUIRE(node != nullptr);
  CHECK_FALSE(node->hitTestVisible);
  CHECK_FALSE(app.lifecycle().layoutPending());
  CHECK(app.lifecycle().framePending());

  app.markFramePresented();
  CHECK_FALSE(app.lifecycle().framePending());
  CHECK(app.setWidgetHitTestVisible(visibilityHandle, false));
  CHECK_FALSE(app.lifecycle().layoutPending());
  CHECK_FALSE(app.lifecycle().framePending());
}

TEST_CASE("App dispatchWidgetEvent does not request frame when callbacks ignore the event") {
  PrimeStage::App app;

  PrimeStage::WidgetActionHandle actionHandle;
  int activateCount = 0;
  CHECK(app.runRebuildIfNeeded([&](PrimeStage::UiNode root) {
    PrimeStage::ButtonSpec button;
    button.label = "Ignore";
    button.size.preferredWidth = 100.0f;
    button.size.preferredHeight = 28.0f;
    button.callbacks.onActivate = [&]() { activateCount += 1; };
    actionHandle = root.createButton(button).actionHandle();
  }));
  CHECK(app.runLayoutIfNeeded());

  app.markFramePresented();
  CHECK_FALSE(app.lifecycle().framePending());

  PrimeFrame::Event keyDown;
  keyDown.type = PrimeFrame::EventType::KeyDown;
  keyDown.key = PrimeStage::keyCodeInt(PrimeStage::KeyCode::Escape);
  CHECK_FALSE(app.dispatchWidgetEvent(actionHandle, keyDown));
  CHECK(activateCount == 0);
  CHECK_FALSE(app.lifecycle().framePending());
}

TEST_CASE("App dispatchWidgetEvent returns false when widget has no callbacks") {
  PrimeStage::App app;

  PrimeStage::WidgetActionHandle actionHandle;
  PrimeFrame::NodeId nodeId{};
  CHECK(app.runRebuildIfNeeded([&](PrimeStage::UiNode root) {
    PrimeStage::ButtonSpec button;
    button.label = "NoCallback";
    button.size.preferredWidth = 100.0f;
    button.size.preferredHeight = 28.0f;
    PrimeStage::UiNode built = root.createButton(button);
    actionHandle = built.actionHandle();
    nodeId = built.lowLevelNodeId();
  }));

  PrimeFrame::Node const* node = app.frame().getNode(nodeId);
  REQUIRE(node != nullptr);
  REQUIRE(node->callbacks != PrimeFrame::InvalidCallbackId);
  PrimeFrame::Callback const* callback = app.frame().getCallback(node->callbacks);
  REQUIRE(callback != nullptr);
  CHECK_FALSE(static_cast<bool>(callback->onEvent));

  app.markFramePresented();
  CHECK_FALSE(app.lifecycle().framePending());

  PrimeFrame::Event keyDown;
  keyDown.type = PrimeFrame::EventType::KeyDown;
  keyDown.key = PrimeStage::keyCodeInt(PrimeStage::KeyCode::Enter);
  CHECK_FALSE(app.dispatchWidgetEvent(actionHandle, keyDown));
  CHECK_FALSE(app.lifecycle().framePending());
}

TEST_CASE("App render wrappers return target and path diagnostics") {
  PrimeStage::App app;

  CHECK(app.runRebuildIfNeeded([](PrimeStage::UiNode root) {
    PrimeStage::ButtonSpec button;
    button.label = "Render";
    button.size.preferredWidth = 80.0f;
    button.size.preferredHeight = 24.0f;
    root.createButton(button);
  }));
  app.setRenderMetrics(320u, 200u, 1.0f);

  std::array<uint8_t, 4> pixels{};
  PrimeStage::RenderTarget target;
  target.pixels = std::span<uint8_t>(pixels);
  target.width = 0u;
  target.height = 16u;
  target.stride = 0u;
  target.scale = 1.0f;

  PrimeStage::RenderStatus targetStatus = app.renderToTarget(target);
  CHECK(targetStatus.code == PrimeStage::RenderStatusCode::InvalidTargetDimensions);
  CHECK(targetStatus.targetWidth == 0u);
  CHECK(targetStatus.targetHeight == 16u);

  PrimeStage::RenderStatus pngStatus = app.renderToPng("");
  CHECK(pngStatus.code == PrimeStage::RenderStatusCode::PngPathEmpty);
}

TEST_CASE("App action routing unifies widget and shortcut entrypoints") {
  PrimeStage::App app;

  int invocationCount = 0;
  PrimeStage::AppActionInvocation lastInvocation{};
  CHECK(app.registerAction("demo.next", [&](PrimeStage::AppActionInvocation const& invocation) {
    invocationCount += 1;
    lastInvocation = invocation;
    app.lifecycle().requestRebuild();
  }));

  PrimeStage::AppShortcut shortcut;
  shortcut.key = PrimeStage::HostKey::Enter;
  shortcut.modifiers =
      static_cast<PrimeHost::KeyModifierMask>(PrimeHost::KeyModifier::Control);
  CHECK(app.bindShortcut(shortcut, "demo.next"));

  PrimeStage::WidgetActionHandle actionHandle;
  CHECK(app.runRebuildIfNeeded([&](PrimeStage::UiNode root) {
    PrimeStage::ButtonSpec button;
    button.label = "Next";
    button.size.preferredWidth = 100.0f;
    button.size.preferredHeight = 28.0f;
    button.callbacks.onActivate = app.makeActionCallback("demo.next");
    actionHandle = root.createButton(button).actionHandle();
  }));
  CHECK(app.runLayoutIfNeeded());

  PrimeFrame::Event widgetKey;
  widgetKey.type = PrimeFrame::EventType::KeyDown;
  widgetKey.key = PrimeStage::keyCodeInt(PrimeStage::KeyCode::Enter);
  CHECK(app.dispatchWidgetEvent(actionHandle, widgetKey));
  CHECK(invocationCount == 1);
  CHECK(lastInvocation.actionId == "demo.next");
  CHECK(lastInvocation.source == PrimeStage::AppActionSource::Widget);
  CHECK_FALSE(lastInvocation.shortcut.has_value());

  PrimeHost::KeyEvent shortcutKey;
  shortcutKey.pressed = true;
  shortcutKey.keyCode = PrimeStage::hostKeyCode(PrimeStage::HostKey::Enter);
  shortcutKey.modifiers = shortcut.modifiers;
  PrimeHost::InputEvent input = shortcutKey;
  PrimeHost::EventBatch batch{};

  app.markFramePresented();
  CHECK_FALSE(app.lifecycle().framePending());
  PrimeStage::InputBridgeResult result = app.bridgeHostInputEvent(input, batch);
  CHECK(result.requestFrame);
  CHECK_FALSE(result.requestExit);
  CHECK(invocationCount == 2);
  CHECK(lastInvocation.actionId == "demo.next");
  CHECK(lastInvocation.source == PrimeStage::AppActionSource::Shortcut);
  REQUIRE(lastInvocation.shortcut.has_value());
  CHECK(lastInvocation.shortcut->key == PrimeStage::HostKey::Enter);
  CHECK(lastInvocation.shortcut->modifiers == shortcut.modifiers);
  CHECK(app.lifecycle().framePending());
}

TEST_CASE("App action routing validates bindings and repeat policy") {
  PrimeStage::App app;
  CHECK_FALSE(app.registerAction("", [](PrimeStage::AppActionInvocation const&) {}));
  CHECK_FALSE(app.registerAction("demo.empty_callback", PrimeStage::AppActionCallback{}));

  PrimeStage::AppShortcut shortcut;
  shortcut.key = PrimeStage::HostKey::Space;
  shortcut.modifiers =
      static_cast<PrimeHost::KeyModifierMask>(PrimeHost::KeyModifier::Control);
  CHECK_FALSE(app.bindShortcut(shortcut, ""));
  CHECK_FALSE(app.bindShortcut(shortcut, "missing"));

  int invocationCount = 0;
  CHECK(app.registerAction("demo.repeat", [&](PrimeStage::AppActionInvocation const&) {
    invocationCount += 1;
  }));
  CHECK(app.bindShortcut(shortcut, "demo.repeat"));

  PrimeHost::KeyEvent repeatKey;
  repeatKey.pressed = true;
  repeatKey.repeat = true;
  repeatKey.keyCode = PrimeStage::hostKeyCode(PrimeStage::HostKey::Space);
  repeatKey.modifiers = shortcut.modifiers;
  PrimeHost::InputEvent input = repeatKey;
  PrimeHost::EventBatch batch{};

  PrimeStage::InputBridgeResult result = app.bridgeHostInputEvent(input, batch);
  CHECK_FALSE(result.requestFrame);
  CHECK(invocationCount == 0);

  shortcut.allowRepeat = true;
  CHECK(app.bindShortcut(shortcut, "demo.repeat"));
  result = app.bridgeHostInputEvent(input, batch);
  CHECK(result.requestFrame);
  CHECK(invocationCount == 1);

  CHECK(app.unbindShortcut(shortcut));
  CHECK_FALSE(app.unbindShortcut(shortcut));
  CHECK(app.unregisterAction("demo.repeat"));
  CHECK_FALSE(app.unregisterAction("demo.repeat"));
  CHECK_FALSE(app.invokeAction("demo.repeat"));
}

TEST_CASE("App bindShortcut updates existing binding action id") {
  PrimeStage::App app;

  int firstCount = 0;
  int secondCount = 0;
  CHECK(app.registerAction("demo.first", [&](PrimeStage::AppActionInvocation const&) {
    firstCount += 1;
  }));
  CHECK(app.registerAction("demo.second", [&](PrimeStage::AppActionInvocation const&) {
    secondCount += 1;
  }));

  PrimeStage::AppShortcut shortcut;
  shortcut.key = PrimeStage::HostKey::Space;
  shortcut.modifiers =
      static_cast<PrimeHost::KeyModifierMask>(PrimeHost::KeyModifier::Control);
  CHECK(app.bindShortcut(shortcut, "demo.first"));
  CHECK(app.bindShortcut(shortcut, "demo.second"));

  PrimeHost::KeyEvent key;
  key.pressed = true;
  key.keyCode = PrimeStage::hostKeyCode(PrimeStage::HostKey::Space);
  key.modifiers = shortcut.modifiers;
  PrimeHost::InputEvent input = key;
  PrimeHost::EventBatch batch{};

  app.markFramePresented();
  CHECK_FALSE(app.lifecycle().framePending());
  PrimeStage::InputBridgeResult result = app.bridgeHostInputEvent(input, batch);
  CHECK(result.requestFrame);
  CHECK_FALSE(result.requestExit);
  CHECK(firstCount == 0);
  CHECK(secondCount == 1);
  CHECK(app.lifecycle().framePending());
}

TEST_CASE("App unregisterAction removes bound shortcuts for that action") {
  PrimeStage::App app;

  int invocationCount = 0;
  CHECK(app.registerAction("demo.cleanup", [&](PrimeStage::AppActionInvocation const&) {
    invocationCount += 1;
  }));

  PrimeStage::AppShortcut shortcut;
  shortcut.key = PrimeStage::HostKey::Space;
  shortcut.modifiers =
      static_cast<PrimeHost::KeyModifierMask>(PrimeHost::KeyModifier::Control);
  CHECK(app.bindShortcut(shortcut, "demo.cleanup"));
  CHECK(app.unregisterAction("demo.cleanup"));

  PrimeHost::KeyEvent key;
  key.pressed = true;
  key.keyCode = PrimeStage::hostKeyCode(PrimeStage::HostKey::Space);
  key.modifiers = shortcut.modifiers;
  PrimeHost::InputEvent input = key;
  PrimeHost::EventBatch batch{};

  app.markFramePresented();
  CHECK_FALSE(app.lifecycle().framePending());
  PrimeStage::InputBridgeResult result = app.bridgeHostInputEvent(input, batch);
  CHECK_FALSE(result.requestExit);
  CHECK_FALSE(result.requestFrame);
  CHECK(invocationCount == 0);
  CHECK_FALSE(app.lifecycle().framePending());
}

TEST_CASE("App registerAction replaces callback for existing action id") {
  PrimeStage::App app;

  int firstCallbackCount = 0;
  int secondCallbackCount = 0;
  CHECK(app.registerAction("demo.replace", [&](PrimeStage::AppActionInvocation const&) {
    firstCallbackCount += 1;
  }));
  CHECK(app.registerAction("demo.replace", [&](PrimeStage::AppActionInvocation const&) {
    secondCallbackCount += 1;
  }));

  app.markFramePresented();
  CHECK_FALSE(app.lifecycle().framePending());
  CHECK(app.invokeAction("demo.replace"));
  CHECK(firstCallbackCount == 0);
  CHECK(secondCallbackCount == 1);
  CHECK(app.lifecycle().framePending());
}

TEST_CASE("App unregisterAction rejects empty action id") {
  PrimeStage::App app;

  app.markFramePresented();
  CHECK_FALSE(app.lifecycle().framePending());
  CHECK_FALSE(app.unregisterAction(""));
  CHECK_FALSE(app.lifecycle().framePending());
}

TEST_CASE("App invokeAction rejects missing action id without requesting frame") {
  PrimeStage::App app;

  app.markFramePresented();
  CHECK_FALSE(app.lifecycle().framePending());
  CHECK_FALSE(app.invokeAction("demo.missing"));
  CHECK_FALSE(app.lifecycle().framePending());
}

TEST_CASE("App makeActionCallback returns empty callback for empty action id") {
  PrimeStage::App app;

  app.markFramePresented();
  CHECK_FALSE(app.lifecycle().framePending());
  std::function<void()> callback = app.makeActionCallback("");
  CHECK_FALSE(static_cast<bool>(callback));
  CHECK_FALSE(app.lifecycle().framePending());
}

TEST_CASE("App action invocation retains action id after callback lifetime ends") {
  PrimeStage::App app;

  PrimeStage::AppActionInvocation saved{};
  CHECK(app.registerAction("demo.persist", [&](PrimeStage::AppActionInvocation const& invocation) {
    saved = invocation;
    CHECK(app.unregisterAction("demo.persist"));
  }));

  CHECK(app.invokeAction("demo.persist"));
  CHECK(saved.actionId == "demo.persist");
  CHECK(saved.source == PrimeStage::AppActionSource::Programmatic);
  CHECK_FALSE(saved.shortcut.has_value());
}

TEST_CASE("App shortcut dispatch ignores key-release events") {
  PrimeStage::App app;

  PrimeStage::AppShortcut shortcut;
  shortcut.key = PrimeStage::HostKey::Space;
  shortcut.modifiers =
      static_cast<PrimeHost::KeyModifierMask>(PrimeHost::KeyModifier::Control);

  int invocationCount = 0;
  CHECK(app.registerAction("demo.key_release", [&](PrimeStage::AppActionInvocation const&) {
    invocationCount += 1;
  }));
  CHECK(app.bindShortcut(shortcut, "demo.key_release"));

  PrimeHost::KeyEvent keyUp;
  keyUp.pressed = false;
  keyUp.keyCode = PrimeStage::hostKeyCode(PrimeStage::HostKey::Space);
  keyUp.modifiers = shortcut.modifiers;
  PrimeHost::InputEvent input = keyUp;
  PrimeHost::EventBatch batch{};

  app.markFramePresented();
  CHECK_FALSE(app.lifecycle().framePending());
  PrimeStage::InputBridgeResult result = app.bridgeHostInputEvent(input, batch);
  CHECK_FALSE(result.requestExit);
  CHECK_FALSE(result.requestFrame);
  CHECK(invocationCount == 0);
  CHECK_FALSE(app.lifecycle().framePending());
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

  app.markFramePresented();
  CHECK_FALSE(app.lifecycle().framePending());
  PrimeStage::InputBridgeResult result = app.bridgeHostInputEvent(input, batch);
  CHECK(result.bypassFrameCap);
  CHECK_FALSE(result.requestExit);
  CHECK(result.requestFrame);
  CHECK(app.focus().focusedNode() == buttonId);
  CHECK(app.lifecycle().framePending());
}

TEST_CASE("App bridgeHostInputEvent returns early for exit-key presses") {
  PrimeStage::App app;

  PrimeHost::KeyEvent key;
  key.pressed = true;
  key.keyCode = PrimeStage::hostKeyCode(PrimeStage::HostKey::Escape);
  key.modifiers = 0u;
  PrimeHost::InputEvent input = key;
  PrimeHost::EventBatch batch{};

  app.markFramePresented();
  CHECK_FALSE(app.lifecycle().framePending());
  PrimeStage::InputBridgeResult result =
      app.bridgeHostInputEvent(input, batch, PrimeStage::HostKey::Escape);
  CHECK(result.requestExit);
  CHECK_FALSE(result.requestFrame);
  CHECK_FALSE(app.lifecycle().framePending());
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

TEST_CASE("App clearHostServices removes clipboard and cursor platform hooks") {
  PrimeStage::App app;

  PrimeStage::AppPlatformServices services;
  services.textFieldClipboard.setText = [](std::string_view) {};
  services.textFieldClipboard.getText = []() { return std::string("x"); };
  services.selectableTextClipboard.setText = [](std::string_view) {};
  services.onCursorHintChanged = [](PrimeStage::CursorHint) {};
  services.onImeCompositionRectChanged = [](int32_t, int32_t, int32_t, int32_t) {};
  app.setPlatformServices(services);

  app.clearHostServices();
  CHECK_FALSE(static_cast<bool>(app.platformServices().textFieldClipboard.setText));
  CHECK_FALSE(static_cast<bool>(app.platformServices().textFieldClipboard.getText));
  CHECK_FALSE(static_cast<bool>(app.platformServices().selectableTextClipboard.setText));
  CHECK_FALSE(static_cast<bool>(app.platformServices().onCursorHintChanged));
  CHECK_FALSE(static_cast<bool>(app.platformServices().onImeCompositionRectChanged));

  std::string clipboardValue;
  bool fieldCursorCalled = false;
  PrimeStage::TextFieldSpec field;
  field.clipboard.setText = [&](std::string_view text) { clipboardValue = std::string(text); };
  field.callbacks.onCursorHintChanged = [&](PrimeStage::CursorHint) { fieldCursorCalled = true; };
  app.applyPlatformServices(field);

  REQUIRE(field.clipboard.setText);
  REQUIRE(field.callbacks.onCursorHintChanged);
  field.clipboard.setText("kept");
  field.callbacks.onCursorHintChanged(PrimeStage::CursorHint::Arrow);
  CHECK(clipboardValue == "kept");
  CHECK(fieldCursorCalled);
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
