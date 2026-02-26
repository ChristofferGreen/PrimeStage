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
