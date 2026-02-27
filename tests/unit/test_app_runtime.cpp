#include "PrimeStage/App.h"
#include "PrimeStage/AppRuntime.h"

#include "third_party/doctest.h"

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
