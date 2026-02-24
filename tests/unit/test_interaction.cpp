#include "PrimeStage/StudioUi.h"
#include "PrimeFrame/Events.h"
#include "PrimeFrame/Layout.h"

#include "third_party/doctest.h"

#include <vector>

namespace Studio = PrimeStage::Studio;

static PrimeStage::UiNode createRoot(PrimeFrame::Frame& frame, float width, float height) {
  PrimeStage::SizeSpec size;
  size.preferredWidth = width;
  size.preferredHeight = height;
  return Studio::createRoot(frame, size);
}

static PrimeFrame::LayoutOutput layoutFrame(PrimeFrame::Frame& frame, float width, float height) {
  PrimeFrame::LayoutOutput output;
  PrimeFrame::LayoutEngine engine;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = width;
  options.rootHeight = height;
  engine.layout(frame, output, options);
  return output;
}

static PrimeFrame::Event makePointerEvent(PrimeFrame::EventType type, int pointerId, float x, float y) {
  PrimeFrame::Event event;
  event.type = type;
  event.pointerId = pointerId;
  event.x = x;
  event.y = y;
  return event;
}

TEST_CASE("PrimeStage button hover/press transitions update styles and callbacks") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 100.0f);

  PrimeStage::ButtonSpec spec;
  spec.size.preferredWidth = 80.0f;
  spec.size.preferredHeight = 30.0f;
  spec.label = "Test";
  spec.backgroundStyle = 101u;
  spec.hoverStyle = 102u;
  spec.pressedStyle = 103u;
  spec.baseOpacity = 0.4f;
  spec.hoverOpacity = 0.6f;
  spec.pressedOpacity = 0.9f;

  int hoverChanges = 0;
  int pressChanges = 0;
  int clicks = 0;
  bool lastHover = false;
  bool lastPressed = false;
  spec.callbacks.onHoverChanged = [&](bool hovered) {
    hoverChanges += 1;
    lastHover = hovered;
  };
  spec.callbacks.onPressedChanged = [&](bool pressed) {
    pressChanges += 1;
    lastPressed = pressed;
  };
  spec.callbacks.onClick = [&]() { clicks += 1; };

  PrimeStage::UiNode button = root.createButton(spec);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 200.0f, 100.0f);
  PrimeFrame::LayoutOut const* out = layout.get(button.nodeId());
  REQUIRE(out != nullptr);
  float centerX = out->absX + out->absW * 0.5f;
  float centerY = out->absY + out->absH * 0.5f;

  PrimeFrame::Node const* node = frame.getNode(button.nodeId());
  REQUIRE(node != nullptr);
  REQUIRE(!node->primitives.empty());
  PrimeFrame::Primitive* prim = frame.getPrimitive(node->primitives.front());
  REQUIRE(prim != nullptr);

  CHECK(prim->rect.token == spec.backgroundStyle);
  CHECK(prim->rect.overrideStyle.opacity == doctest::Approx(spec.baseOpacity));

  PrimeFrame::EventRouter router;

  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerMove, 1, centerX, centerY), frame, layout);
  CHECK(lastHover);
  CHECK(prim->rect.token == spec.hoverStyle);
  CHECK(prim->rect.overrideStyle.opacity == doctest::Approx(spec.hoverOpacity));

  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 1, centerX, centerY), frame, layout);
  CHECK(lastPressed);
  CHECK(prim->rect.token == spec.pressedStyle);
  CHECK(prim->rect.overrideStyle.opacity == doctest::Approx(spec.pressedOpacity));

  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 1, centerX, centerY), frame, layout);
  CHECK_FALSE(lastPressed);
  CHECK(clicks == 1);
  CHECK(prim->rect.token == spec.hoverStyle);
  CHECK(prim->rect.overrideStyle.opacity == doctest::Approx(spec.hoverOpacity));

  float outX = out->absX - 5.0f;
  float outY = out->absY - 5.0f;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerMove, 1, outX, outY), frame, layout);
  CHECK_FALSE(lastHover);
  CHECK(prim->rect.token == spec.backgroundStyle);
  CHECK(prim->rect.overrideStyle.opacity == doctest::Approx(spec.baseOpacity));

  CHECK(hoverChanges >= 2);
  CHECK(pressChanges >= 2);
}

TEST_CASE("PrimeStage slider drag clamps and updates hover/press styles") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 60.0f);

  PrimeStage::SliderSpec spec;
  spec.size.preferredWidth = 120.0f;
  spec.size.preferredHeight = 12.0f;
  spec.value = 0.25f;
  spec.trackStyle = 201u;
  spec.fillStyle = 202u;
  spec.thumbStyle = 203u;
  spec.trackStyleOverride.opacity = 0.4f;
  spec.fillStyleOverride.opacity = 0.5f;
  spec.thumbStyleOverride.opacity = 0.6f;
  spec.trackThickness = 8.0f;
  spec.trackHoverOpacity = 0.7f;
  spec.fillHoverOpacity = 0.8f;
  spec.trackPressedOpacity = 0.2f;
  spec.fillPressedOpacity = 0.9f;
  spec.thumbSize = 0.0f;

  int dragStart = 0;
  int dragEnd = 0;
  std::vector<float> values;
  spec.callbacks.onDragStart = [&]() { dragStart += 1; };
  spec.callbacks.onDragEnd = [&]() { dragEnd += 1; };
  spec.callbacks.onValueChanged = [&](float value) { values.push_back(value); };

  PrimeStage::UiNode slider = root.createSlider(spec);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 200.0f, 60.0f);
  PrimeFrame::LayoutOut const* out = layout.get(slider.nodeId());
  REQUIRE(out != nullptr);

  PrimeFrame::Node const* node = frame.getNode(slider.nodeId());
  REQUIRE(node != nullptr);
  REQUIRE(node->primitives.size() >= 3);
  PrimeFrame::Primitive* trackPrim = frame.getPrimitive(node->primitives[0]);
  PrimeFrame::Primitive* fillPrim = frame.getPrimitive(node->primitives[1]);
  PrimeFrame::Primitive* thumbPrim = frame.getPrimitive(node->primitives[2]);
  REQUIRE(trackPrim != nullptr);
  REQUIRE(fillPrim != nullptr);
  REQUIRE(thumbPrim != nullptr);
  auto baseTrackOpacity = trackPrim->rect.overrideStyle.opacity;
  auto baseFillOpacity = fillPrim->rect.overrideStyle.opacity;

  PrimeFrame::EventRouter router;
  router.setDragThreshold(0.0f);

  float centerX = out->absX + out->absW * 0.5f;
  float centerY = out->absY + out->absH * 0.5f;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerMove, 1, centerX, centerY), frame, layout);

  REQUIRE(trackPrim->rect.overrideStyle.opacity.has_value());
  REQUIRE(fillPrim->rect.overrideStyle.opacity.has_value());
  CHECK(trackPrim->rect.overrideStyle.opacity.value() == doctest::Approx(0.7f));
  CHECK(fillPrim->rect.overrideStyle.opacity.value() == doctest::Approx(0.8f));

  float x75 = out->absX + out->absW * 0.75f;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 1, x75, centerY), frame, layout);
  CHECK(dragStart == 1);
  REQUIRE(!values.empty());
  CHECK(values.back() == doctest::Approx(0.75f));
  REQUIRE(trackPrim->rect.overrideStyle.opacity.has_value());
  REQUIRE(fillPrim->rect.overrideStyle.opacity.has_value());
  CHECK(trackPrim->rect.overrideStyle.opacity.value() == doctest::Approx(0.2f));
  CHECK(fillPrim->rect.overrideStyle.opacity.value() == doctest::Approx(0.9f));

  float outsideX = out->absX - 10.0f;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerMove, 1, outsideX, centerY), frame, layout);
  REQUIRE(values.size() >= 2);
  CHECK(values.back() == doctest::Approx(0.0f));

  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 1, outsideX, centerY), frame, layout);
  CHECK(dragEnd == 1);
  REQUIRE(values.size() >= 3);
  CHECK(values.back() == doctest::Approx(0.0f));

  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerMove, 1, outsideX - 5.0f, centerY), frame, layout);
  CHECK(trackPrim->rect.overrideStyle.opacity == baseTrackOpacity);
  if (fillPrim->width <= 0.0f || fillPrim->height <= 0.0f) {
    REQUIRE(fillPrim->rect.overrideStyle.opacity.has_value());
    CHECK(fillPrim->rect.overrideStyle.opacity.value() == doctest::Approx(0.0f));
  } else {
    CHECK(fillPrim->rect.overrideStyle.opacity == baseFillOpacity);
  }
}

TEST_CASE("PrimeStage button drag outside cancels click and resets style") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 100.0f);

  PrimeStage::ButtonSpec spec;
  spec.size.preferredWidth = 70.0f;
  spec.size.preferredHeight = 24.0f;
  spec.label = "Drag";
  spec.backgroundStyle = 111u;
  spec.hoverStyle = 112u;
  spec.pressedStyle = 113u;
  spec.baseOpacity = 0.35f;
  spec.hoverOpacity = 0.55f;
  spec.pressedOpacity = 0.85f;

  int clicks = 0;
  bool hovered = false;
  bool pressed = false;
  spec.callbacks.onClick = [&]() { clicks += 1; };
  spec.callbacks.onHoverChanged = [&](bool value) { hovered = value; };
  spec.callbacks.onPressedChanged = [&](bool value) { pressed = value; };

  PrimeStage::UiNode button = root.createButton(spec);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 200.0f, 100.0f);
  PrimeFrame::LayoutOut const* out = layout.get(button.nodeId());
  REQUIRE(out != nullptr);
  float centerX = out->absX + out->absW * 0.5f;
  float centerY = out->absY + out->absH * 0.5f;
  float outsideX = out->absX - 8.0f;
  float outsideY = out->absY - 8.0f;

  PrimeFrame::Node const* node = frame.getNode(button.nodeId());
  REQUIRE(node != nullptr);
  REQUIRE(!node->primitives.empty());
  PrimeFrame::Primitive* prim = frame.getPrimitive(node->primitives.front());
  REQUIRE(prim != nullptr);

  PrimeFrame::EventRouter router;
  router.setDragThreshold(0.0f);

  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerMove, 1, centerX, centerY), frame, layout);
  CHECK(hovered);
  CHECK(prim->rect.token == spec.hoverStyle);

  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 1, centerX, centerY), frame, layout);
  CHECK(pressed);
  CHECK(prim->rect.token == spec.pressedStyle);

  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerMove, 1, outsideX, outsideY), frame, layout);
  CHECK_FALSE(pressed);
  CHECK_FALSE(hovered);
  CHECK(prim->rect.token == spec.backgroundStyle);

  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 1, outsideX, outsideY), frame, layout);
  CHECK(clicks == 0);
  CHECK_FALSE(pressed);
  CHECK_FALSE(hovered);
  CHECK(prim->rect.token == spec.backgroundStyle);
}

TEST_CASE("PrimeStage vertical slider maps top to 1 and bottom to 0") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 120.0f, 160.0f);

  PrimeStage::SliderSpec spec;
  spec.vertical = true;
  spec.size.preferredWidth = 14.0f;
  spec.size.preferredHeight = 120.0f;
  spec.trackStyle = 301u;
  spec.fillStyle = 302u;
  spec.thumbStyle = 303u;
  spec.trackThickness = 10.0f;
  spec.thumbSize = 0.0f;

  std::vector<float> values;
  spec.callbacks.onValueChanged = [&](float value) { values.push_back(value); };

  PrimeStage::UiNode slider = root.createSlider(spec);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 120.0f, 160.0f);
  PrimeFrame::LayoutOut const* out = layout.get(slider.nodeId());
  REQUIRE(out != nullptr);

  PrimeFrame::EventRouter router;
  router.setDragThreshold(0.0f);

  float centerX = out->absX + out->absW * 0.5f;
  float topY = out->absY + 1.0f;
  float bottomY = out->absY + out->absH - 1.0f;

  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 1, centerX, topY), frame, layout);
  REQUIRE(!values.empty());
  CHECK(values.back() >= 0.98f);

  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 1, centerX, topY), frame, layout);

  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 2, centerX, bottomY), frame, layout);
  REQUIRE(values.size() >= 2);
  CHECK(values.back() <= 0.02f);
}
