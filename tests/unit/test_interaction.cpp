#include "PrimeStage/Ui.h"
#include "PrimeFrame/Events.h"
#include "PrimeFrame/Frame.h"
#include "PrimeFrame/Layout.h"

#include "third_party/doctest.h"

#include <string>
#include <utility>
#include <vector>

static PrimeStage::UiNode createRoot(PrimeFrame::Frame& frame, float width, float height) {
  PrimeFrame::NodeId rootId = frame.createNode();
  frame.addRoot(rootId);
  if (auto* node = frame.getNode(rootId)) {
    node->layout = PrimeFrame::LayoutType::Overlay;
    node->sizeHint.width.preferred = width;
    node->sizeHint.height.preferred = height;
  }
  return PrimeStage::UiNode(frame, rootId, true);
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

static PrimeFrame::Event makeKeyDownEvent(PrimeStage::KeyCode key) {
  PrimeFrame::Event event;
  event.type = PrimeFrame::EventType::KeyDown;
  event.key = PrimeStage::keyCodeInt(key);
  return event;
}

static PrimeFrame::Primitive const* findRectPrimitiveByTokenInSubtree(PrimeFrame::Frame const& frame,
                                                                       PrimeFrame::NodeId nodeId,
                                                                       PrimeFrame::RectStyleToken token) {
  PrimeFrame::Node const* node = frame.getNode(nodeId);
  if (!node) {
    return nullptr;
  }
  for (PrimeFrame::PrimitiveId primId : node->primitives) {
    PrimeFrame::Primitive const* prim = frame.getPrimitive(primId);
    if (prim && prim->type == PrimeFrame::PrimitiveType::Rect && prim->rect.token == token) {
      return prim;
    }
  }
  for (PrimeFrame::NodeId childId : node->children) {
    PrimeFrame::Primitive const* child = findRectPrimitiveByTokenInSubtree(frame, childId, token);
    if (child) {
      return child;
    }
  }
  return nullptr;
}

static PrimeFrame::NodeId findFirstNodeWithRectTokenInSubtree(PrimeFrame::Frame const& frame,
                                                               PrimeFrame::NodeId nodeId,
                                                               PrimeFrame::RectStyleToken token) {
  PrimeFrame::Node const* node = frame.getNode(nodeId);
  if (!node) {
    return PrimeFrame::NodeId{};
  }
  for (PrimeFrame::PrimitiveId primId : node->primitives) {
    PrimeFrame::Primitive const* prim = frame.getPrimitive(primId);
    if (prim && prim->type == PrimeFrame::PrimitiveType::Rect && prim->rect.token == token) {
      return nodeId;
    }
  }
  for (PrimeFrame::NodeId childId : node->children) {
    PrimeFrame::NodeId child = findFirstNodeWithRectTokenInSubtree(frame, childId, token);
    if (child.isValid()) {
      return child;
    }
  }
  return PrimeFrame::NodeId{};
}

static PrimeFrame::NodeId findFirstNodeWithOnEventInSubtree(PrimeFrame::Frame const& frame,
                                                             PrimeFrame::NodeId nodeId) {
  PrimeFrame::Node const* node = frame.getNode(nodeId);
  if (!node) {
    return PrimeFrame::NodeId{};
  }
  if (node->callbacks != PrimeFrame::InvalidCallbackId) {
    PrimeFrame::Callback const* callback = frame.getCallback(node->callbacks);
    if (callback && callback->onEvent) {
      return nodeId;
    }
  }
  for (PrimeFrame::NodeId childId : node->children) {
    PrimeFrame::NodeId candidate = findFirstNodeWithOnEventInSubtree(frame, childId);
    if (candidate.isValid()) {
      return candidate;
    }
  }
  return PrimeFrame::NodeId{};
}

TEST_CASE("PrimeStage disabled controls are not focusable or interactive") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 360.0f, 220.0f);

  PrimeStage::StackSpec stackSpec;
  stackSpec.gap = 8.0f;
  stackSpec.size.stretchX = 1.0f;
  stackSpec.size.stretchY = 1.0f;
  PrimeStage::UiNode stack = root.createVerticalStack(stackSpec);

  int buttonClicks = 0;
  int toggleChanges = 0;
  int checkboxChanges = 0;
  int tabChanges = 0;
  int dropdownChanges = 0;

  PrimeStage::ButtonSpec buttonSpec;
  buttonSpec.label = "Disabled";
  buttonSpec.backgroundStyle = 11u;
  buttonSpec.size.preferredWidth = 120.0f;
  buttonSpec.size.preferredHeight = 28.0f;
  buttonSpec.enabled = false;
  buttonSpec.callbacks.onClick = [&]() { buttonClicks += 1; };

  PrimeStage::ToggleSpec toggleSpec;
  toggleSpec.trackStyle = 21u;
  toggleSpec.knobStyle = 22u;
  toggleSpec.size.preferredWidth = 60.0f;
  toggleSpec.size.preferredHeight = 24.0f;
  toggleSpec.enabled = false;
  toggleSpec.callbacks.onChanged = [&](bool) { toggleChanges += 1; };

  PrimeStage::CheckboxSpec checkboxSpec;
  checkboxSpec.label = "Flag";
  checkboxSpec.boxStyle = 31u;
  checkboxSpec.checkStyle = 32u;
  checkboxSpec.size.preferredHeight = 24.0f;
  checkboxSpec.enabled = false;
  checkboxSpec.callbacks.onChanged = [&](bool) { checkboxChanges += 1; };

  PrimeStage::TabsSpec tabsSpec;
  tabsSpec.labels = {"A", "B", "C"};
  tabsSpec.tabStyle = 41u;
  tabsSpec.activeTabStyle = 42u;
  tabsSpec.size.preferredHeight = 24.0f;
  tabsSpec.enabled = false;
  tabsSpec.callbacks.onTabChanged = [&](int) { tabChanges += 1; };

  PrimeStage::DropdownSpec dropdownSpec;
  dropdownSpec.options = {"One", "Two"};
  dropdownSpec.backgroundStyle = 51u;
  dropdownSpec.size.preferredWidth = 120.0f;
  dropdownSpec.size.preferredHeight = 24.0f;
  dropdownSpec.enabled = false;
  dropdownSpec.callbacks.onSelected = [&](int) { dropdownChanges += 1; };
  dropdownSpec.callbacks.onOpened = [&]() { dropdownChanges += 1; };

  PrimeStage::UiNode button = stack.createButton(buttonSpec);
  PrimeStage::UiNode toggle = stack.createToggle(toggleSpec);
  PrimeStage::UiNode checkbox = stack.createCheckbox(checkboxSpec);
  PrimeStage::UiNode tabs = stack.createTabs(tabsSpec);
  PrimeStage::UiNode dropdown = stack.createDropdown(dropdownSpec);

  PrimeFrame::Node const* buttonNode = frame.getNode(button.nodeId());
  PrimeFrame::Node const* toggleNode = frame.getNode(toggle.nodeId());
  PrimeFrame::Node const* checkboxNode = frame.getNode(checkbox.nodeId());
  PrimeFrame::Node const* tabsNode = frame.getNode(tabs.nodeId());
  PrimeFrame::Node const* dropdownNode = frame.getNode(dropdown.nodeId());
  REQUIRE(buttonNode != nullptr);
  REQUIRE(toggleNode != nullptr);
  REQUIRE(checkboxNode != nullptr);
  REQUIRE(tabsNode != nullptr);
  REQUIRE(dropdownNode != nullptr);

  CHECK_FALSE(buttonNode->focusable);
  CHECK_FALSE(toggleNode->focusable);
  CHECK_FALSE(checkboxNode->focusable);
  CHECK_FALSE(dropdownNode->focusable);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 360.0f, 220.0f);
  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;

  auto clickCenter = [&](PrimeFrame::NodeId nodeId, int pointerId) {
    PrimeFrame::LayoutOut const* out = layout.get(nodeId);
    REQUIRE(out != nullptr);
    float x = out->absX + out->absW * 0.5f;
    float y = out->absY + out->absH * 0.5f;
    router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, pointerId, x, y),
                    frame,
                    layout,
                    &focus);
    router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, pointerId, x, y),
                    frame,
                    layout,
                    &focus);
  };

  clickCenter(button.nodeId(), 1);
  clickCenter(toggle.nodeId(), 2);
  clickCenter(checkbox.nodeId(), 3);
  clickCenter(tabs.nodeId(), 4);
  clickCenter(dropdown.nodeId(), 5);

  CHECK(buttonClicks == 0);
  CHECK(toggleChanges == 0);
  CHECK(checkboxChanges == 0);
  CHECK(tabChanges == 0);
  CHECK(dropdownChanges == 0);
  CHECK_FALSE(focus.focusedNode().isValid());

  PrimeFrame::Primitive const* disabledScrim =
      findRectPrimitiveByTokenInSubtree(frame, button.nodeId(), 1u);
  REQUIRE(disabledScrim != nullptr);
  REQUIRE(disabledScrim->rect.overrideStyle.opacity.has_value());
  CHECK(disabledScrim->rect.overrideStyle.opacity.value() < 1.0f);
}

TEST_CASE("PrimeStage read-only text field blocks editing but keeps focus behavior") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 260.0f, 120.0f);

  PrimeStage::TextFieldState state;
  state.text = "Prime";
  state.cursor = static_cast<uint32_t>(state.text.size());

  int textChanged = 0;
  int submitCount = 0;

  PrimeStage::TextFieldSpec spec;
  spec.state = &state;
  spec.readOnly = true;
  spec.backgroundStyle = 61u;
  spec.cursorStyle = 62u;
  spec.size.preferredWidth = 180.0f;
  spec.size.preferredHeight = 28.0f;
  spec.callbacks.onTextChanged = [&](std::string_view) { textChanged += 1; };
  spec.callbacks.onSubmit = [&]() { submitCount += 1; };

  PrimeStage::UiNode field = root.createTextField(spec);
  PrimeFrame::Node const* node = frame.getNode(field.nodeId());
  REQUIRE(node != nullptr);
  CHECK(node->focusable);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 260.0f, 120.0f);
  PrimeFrame::LayoutOut const* out = layout.get(field.nodeId());
  REQUIRE(out != nullptr);
  float centerX = out->absX + out->absW * 0.5f;
  float centerY = out->absY + out->absH * 0.5f;

  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 1, centerX, centerY),
                  frame,
                  layout,
                  &focus);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 1, centerX, centerY),
                  frame,
                  layout,
                  &focus);
  CHECK(focus.focusedNode() == field.nodeId());

  PrimeFrame::Event textInput;
  textInput.type = PrimeFrame::EventType::TextInput;
  textInput.text = "X";
  router.dispatch(textInput, frame, layout, &focus);

  PrimeFrame::Event backspace;
  backspace.type = PrimeFrame::EventType::KeyDown;
  backspace.key = 0x2A; // Backspace
  router.dispatch(backspace, frame, layout, &focus);

  PrimeFrame::Event keyEnter;
  keyEnter.type = PrimeFrame::EventType::KeyDown;
  keyEnter.key = 0x28; // Enter
  router.dispatch(keyEnter, frame, layout, &focus);

  CHECK(state.text == "Prime");
  CHECK(textChanged == 0);
  CHECK(submitCount == 0);
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

TEST_CASE("PrimeStage slider state-backed interactions do not require callbacks") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 80.0f);

  PrimeStage::SliderState sliderState;
  sliderState.value = 0.20f;

  PrimeStage::SliderSpec spec;
  spec.state = &sliderState;
  spec.value = 0.90f; // state-backed mode reads SliderState as source of truth
  spec.trackStyle = 261u;
  spec.fillStyle = 262u;
  spec.thumbStyle = 263u;
  spec.focusStyle = 264u;
  spec.trackThickness = 8.0f;
  spec.thumbSize = 0.0f;
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 16.0f;

  PrimeStage::UiNode slider = root.createSlider(spec);
  PrimeFrame::Node const* sliderNode = frame.getNode(slider.nodeId());
  REQUIRE(sliderNode != nullptr);
  CHECK(sliderNode->callbacks != PrimeFrame::InvalidCallbackId);

  PrimeFrame::Primitive const* fillPrimBefore =
      findRectPrimitiveByTokenInSubtree(frame, slider.nodeId(), spec.fillStyle);
  REQUIRE(fillPrimBefore != nullptr);
  float widthBefore = fillPrimBefore->width;
  CHECK(widthBefore == doctest::Approx(40.0f));

  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 240.0f, 80.0f);
  PrimeFrame::LayoutOut const* out = layout.get(slider.nodeId());
  REQUIRE(out != nullptr);

  PrimeFrame::EventRouter router;
  router.setDragThreshold(0.0f);
  float x80 = out->absX + out->absW * 0.80f;
  float y = out->absY + out->absH * 0.5f;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 1, x80, y), frame, layout);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 1, x80, y), frame, layout);

  CHECK(sliderState.value >= 0.79f);
  PrimeFrame::Primitive const* fillPrimAfter =
      findRectPrimitiveByTokenInSubtree(frame, slider.nodeId(), spec.fillStyle);
  REQUIRE(fillPrimAfter != nullptr);
  CHECK(fillPrimAfter->width > widthBefore);
}

TEST_CASE("PrimeStage disabled state-backed slider ignores pointer input") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 80.0f);

  PrimeStage::SliderState sliderState;
  sliderState.value = 0.45f;

  PrimeStage::SliderSpec spec;
  spec.state = &sliderState;
  spec.enabled = false;
  spec.trackStyle = 271u;
  spec.fillStyle = 272u;
  spec.thumbStyle = 273u;
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 16.0f;

  PrimeStage::UiNode slider = root.createSlider(spec);
  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 240.0f, 80.0f);
  PrimeFrame::LayoutOut const* out = layout.get(slider.nodeId());
  REQUIRE(out != nullptr);

  PrimeFrame::EventRouter router;
  float x90 = out->absX + out->absW * 0.90f;
  float y = out->absY + out->absH * 0.5f;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 1, x90, y), frame, layout);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 1, x90, y), frame, layout);

  CHECK(sliderState.value == doctest::Approx(0.45f));
}

TEST_CASE("PrimeStage slider and progress binding mode clamps and syncs with legacy state") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 280.0f, 140.0f);

  PrimeStage::StackSpec stackSpec;
  stackSpec.gap = 12.0f;
  stackSpec.size.stretchX = 1.0f;
  stackSpec.size.stretchY = 1.0f;
  PrimeStage::UiNode stack = root.createVerticalStack(stackSpec);

  PrimeStage::State<float> sliderBinding;
  sliderBinding.value = -0.25f;
  PrimeStage::SliderState sliderLegacy;
  sliderLegacy.value = 0.80f;

  PrimeStage::SliderSpec sliderSpec;
  sliderSpec.binding = PrimeStage::bind(sliderBinding);
  sliderSpec.state = &sliderLegacy;
  sliderSpec.value = 0.40f;
  sliderSpec.trackStyle = 281u;
  sliderSpec.fillStyle = 282u;
  sliderSpec.thumbStyle = 283u;
  sliderSpec.size.preferredWidth = 220.0f;
  sliderSpec.size.preferredHeight = 16.0f;

  PrimeStage::State<float> progressBinding;
  progressBinding.value = 1.35f;
  PrimeStage::ProgressBarState progressLegacy;
  progressLegacy.value = 0.20f;

  PrimeStage::ProgressBarSpec progressSpec;
  progressSpec.binding = PrimeStage::bind(progressBinding);
  progressSpec.state = &progressLegacy;
  progressSpec.value = 0.30f;
  progressSpec.trackStyle = 291u;
  progressSpec.fillStyle = 292u;
  progressSpec.focusStyle = 293u;
  progressSpec.size.preferredWidth = 220.0f;
  progressSpec.size.preferredHeight = 14.0f;

  PrimeStage::UiNode slider = stack.createSlider(sliderSpec);
  PrimeStage::UiNode progress = stack.createProgressBar(progressSpec);

  // Binding state is the source of truth and is clamped during build.
  CHECK(sliderBinding.value == doctest::Approx(0.0f));
  CHECK(progressBinding.value == doctest::Approx(1.0f));
  CHECK(sliderLegacy.value == doctest::Approx(0.80f));
  CHECK(progressLegacy.value == doctest::Approx(0.20f));

  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 280.0f, 140.0f);
  PrimeFrame::LayoutOut const* sliderOut = layout.get(slider.nodeId());
  PrimeFrame::LayoutOut const* progressOut = layout.get(progress.nodeId());
  REQUIRE(sliderOut != nullptr);
  REQUIRE(progressOut != nullptr);

  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;

  float sliderX = sliderOut->absX + sliderOut->absW * 0.75f;
  float sliderY = sliderOut->absY + sliderOut->absH * 0.5f;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 1, sliderX, sliderY),
                  frame,
                  layout,
                  &focus);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 1, sliderX, sliderY),
                  frame,
                  layout,
                  &focus);
  CHECK(sliderBinding.value >= 0.70f);
  CHECK(sliderLegacy.value == doctest::Approx(sliderBinding.value));

  focus.setFocus(frame, layout, progress.nodeId());
  router.dispatch(makeKeyDownEvent(PrimeStage::KeyCode::Home), frame, layout, &focus);
  CHECK(progressBinding.value == doctest::Approx(0.0f));
  CHECK(progressLegacy.value == doctest::Approx(0.0f));

  float progressX = progressOut->absX + progressOut->absW * 0.65f;
  float progressY = progressOut->absY + progressOut->absH * 0.5f;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 2, progressX, progressY),
                  frame,
                  layout,
                  &focus);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 2, progressX, progressY),
                  frame,
                  layout,
                  &focus);
  CHECK(progressBinding.value >= 0.60f);
  CHECK(progressLegacy.value == doctest::Approx(progressBinding.value));
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

TEST_CASE("PrimeStage button key activation triggers click") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 220.0f, 120.0f);

  PrimeStage::ButtonSpec spec;
  spec.size.preferredWidth = 100.0f;
  spec.size.preferredHeight = 32.0f;
  spec.label = "Key";
  spec.backgroundStyle = 121u;
  spec.hoverStyle = 122u;
  spec.pressedStyle = 123u;
  spec.focusStyle = 124u;

  int clicks = 0;
  spec.callbacks.onClick = [&]() { clicks += 1; };

  PrimeStage::UiNode button = root.createButton(spec);
  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 220.0f, 120.0f);
  PrimeFrame::LayoutOut const* out = layout.get(button.nodeId());
  REQUIRE(out != nullptr);

  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;

  float centerX = out->absX + out->absW * 0.5f;
  float centerY = out->absY + out->absH * 0.5f;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 1, centerX, centerY),
                  frame,
                  layout,
                  &focus);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 1, centerX, centerY),
                  frame,
                  layout,
                  &focus);
  REQUIRE(clicks == 1);

  PrimeFrame::Event keyEnter;
  keyEnter.type = PrimeFrame::EventType::KeyDown;
  keyEnter.key = 0x28; // Enter
  router.dispatch(keyEnter, frame, layout, &focus);

  PrimeFrame::Event keySpace;
  keySpace.type = PrimeFrame::EventType::KeyDown;
  keySpace.key = 0x2C; // Space
  router.dispatch(keySpace, frame, layout, &focus);

  CHECK(clicks == 3);
}

TEST_CASE("PrimeStage toggle and checkbox emit onChanged for pointer and keyboard") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 260.0f, 140.0f);

  PrimeStage::StackSpec stackSpec;
  stackSpec.gap = 12.0f;
  stackSpec.size.stretchX = 1.0f;
  stackSpec.size.stretchY = 1.0f;
  PrimeStage::UiNode stack = root.createVerticalStack(stackSpec);

  PrimeStage::ToggleSpec toggleSpec;
  toggleSpec.on = false;
  toggleSpec.trackStyle = 201u;
  toggleSpec.knobStyle = 202u;
  toggleSpec.focusStyle = 203u;
  toggleSpec.size.preferredWidth = 60.0f;
  toggleSpec.size.preferredHeight = 28.0f;

  PrimeStage::CheckboxSpec checkboxSpec;
  checkboxSpec.label = "Enabled";
  checkboxSpec.checked = true;
  checkboxSpec.boxStyle = 211u;
  checkboxSpec.checkStyle = 212u;
  checkboxSpec.focusStyle = 213u;
  checkboxSpec.textStyle = 214u;

  std::vector<bool> toggleValues;
  std::vector<bool> checkboxValues;
  toggleSpec.callbacks.onChanged = [&](bool on) { toggleValues.push_back(on); };
  checkboxSpec.callbacks.onChanged = [&](bool checked) { checkboxValues.push_back(checked); };

  PrimeStage::UiNode toggle = stack.createToggle(toggleSpec);
  PrimeStage::UiNode checkbox = stack.createCheckbox(checkboxSpec);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 260.0f, 140.0f);
  PrimeFrame::LayoutOut const* toggleOut = layout.get(toggle.nodeId());
  PrimeFrame::LayoutOut const* checkboxOut = layout.get(checkbox.nodeId());
  REQUIRE(toggleOut != nullptr);
  REQUIRE(checkboxOut != nullptr);

  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;

  float toggleX = toggleOut->absX + toggleOut->absW * 0.5f;
  float toggleY = toggleOut->absY + toggleOut->absH * 0.5f;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 1, toggleX, toggleY),
                  frame,
                  layout,
                  &focus);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 1, toggleX, toggleY),
                  frame,
                  layout,
                  &focus);
  REQUIRE(!toggleValues.empty());
  CHECK(toggleValues.back() == true);

  PrimeFrame::Event keySpace;
  keySpace.type = PrimeFrame::EventType::KeyDown;
  keySpace.key = 0x2C; // Space
  router.dispatch(keySpace, frame, layout, &focus);
  REQUIRE(toggleValues.size() >= 2);
  CHECK(toggleValues.back() == false);

  float checkboxX = checkboxOut->absX + checkboxOut->absW * 0.5f;
  float checkboxY = checkboxOut->absY + checkboxOut->absH * 0.5f;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 2, checkboxX, checkboxY),
                  frame,
                  layout,
                  &focus);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 2, checkboxX, checkboxY),
                  frame,
                  layout,
                  &focus);
  REQUIRE(!checkboxValues.empty());
  CHECK(checkboxValues.back() == false);

  PrimeFrame::Event keyEnter;
  keyEnter.type = PrimeFrame::EventType::KeyDown;
  keyEnter.key = 0x28; // Enter
  router.dispatch(keyEnter, frame, layout, &focus);
  REQUIRE(checkboxValues.size() >= 2);
  CHECK(checkboxValues.back() == true);
}

TEST_CASE("PrimeStage toggle and checkbox support state-backed uncontrolled mode") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 260.0f, 140.0f);

  PrimeStage::StackSpec stackSpec;
  stackSpec.gap = 12.0f;
  stackSpec.size.stretchX = 1.0f;
  stackSpec.size.stretchY = 1.0f;
  PrimeStage::UiNode stack = root.createVerticalStack(stackSpec);

  PrimeStage::ToggleState toggleState;
  toggleState.on = true;
  PrimeStage::CheckboxState checkboxState;
  checkboxState.checked = false;

  PrimeStage::ToggleSpec toggleSpec;
  toggleSpec.state = &toggleState;
  toggleSpec.on = false; // state-backed mode uses ToggleState as source of truth
  toggleSpec.trackStyle = 221u;
  toggleSpec.knobStyle = 222u;
  toggleSpec.focusStyle = 223u;
  toggleSpec.size.preferredWidth = 60.0f;
  toggleSpec.size.preferredHeight = 28.0f;

  PrimeStage::CheckboxSpec checkboxSpec;
  checkboxSpec.state = &checkboxState;
  checkboxSpec.checked = true; // state-backed mode uses CheckboxState as source of truth
  checkboxSpec.label = "Enabled";
  checkboxSpec.boxStyle = 231u;
  checkboxSpec.checkStyle = 232u;
  checkboxSpec.focusStyle = 233u;
  checkboxSpec.textStyle = 234u;

  std::vector<bool> toggleValues;
  std::vector<bool> checkboxValues;
  toggleSpec.callbacks.onChanged = [&](bool on) { toggleValues.push_back(on); };
  checkboxSpec.callbacks.onChanged = [&](bool checked) { checkboxValues.push_back(checked); };

  PrimeStage::UiNode toggle = stack.createToggle(toggleSpec);
  PrimeStage::UiNode checkbox = stack.createCheckbox(checkboxSpec);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 260.0f, 140.0f);
  PrimeFrame::LayoutOut const* toggleOut = layout.get(toggle.nodeId());
  PrimeFrame::LayoutOut const* checkboxOut = layout.get(checkbox.nodeId());
  REQUIRE(toggleOut != nullptr);
  REQUIRE(checkboxOut != nullptr);

  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;

  float toggleX = toggleOut->absX + toggleOut->absW * 0.5f;
  float toggleY = toggleOut->absY + toggleOut->absH * 0.5f;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 1, toggleX, toggleY),
                  frame,
                  layout,
                  &focus);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 1, toggleX, toggleY),
                  frame,
                  layout,
                  &focus);
  CHECK(toggleState.on == false);
  REQUIRE(!toggleValues.empty());
  CHECK(toggleValues.back() == false);

  PrimeFrame::Event keySpace;
  keySpace.type = PrimeFrame::EventType::KeyDown;
  keySpace.key = 0x2C; // Space
  router.dispatch(keySpace, frame, layout, &focus);
  CHECK(toggleState.on == true);
  REQUIRE(toggleValues.size() >= 2);
  CHECK(toggleValues.back() == true);

  float checkboxX = checkboxOut->absX + checkboxOut->absW * 0.5f;
  float checkboxY = checkboxOut->absY + checkboxOut->absH * 0.5f;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 2, checkboxX, checkboxY),
                  frame,
                  layout,
                  &focus);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 2, checkboxX, checkboxY),
                  frame,
                  layout,
                  &focus);
  CHECK(checkboxState.checked == true);
  REQUIRE(!checkboxValues.empty());
  CHECK(checkboxValues.back() == true);

  PrimeFrame::Event keyEnter;
  keyEnter.type = PrimeFrame::EventType::KeyDown;
  keyEnter.key = 0x28; // Enter
  router.dispatch(keyEnter, frame, layout, &focus);
  CHECK(checkboxState.checked == false);
  REQUIRE(checkboxValues.size() >= 2);
  CHECK(checkboxValues.back() == false);
}

TEST_CASE("PrimeStage toggle and checkbox binding mode takes precedence and syncs legacy state") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 260.0f, 140.0f);

  PrimeStage::StackSpec stackSpec;
  stackSpec.gap = 12.0f;
  stackSpec.size.stretchX = 1.0f;
  stackSpec.size.stretchY = 1.0f;
  PrimeStage::UiNode stack = root.createVerticalStack(stackSpec);

  PrimeStage::State<bool> toggleBinding;
  toggleBinding.value = false;
  PrimeStage::ToggleState toggleLegacy;
  toggleLegacy.on = true;

  PrimeStage::ToggleSpec toggleSpec;
  toggleSpec.binding = PrimeStage::bind(toggleBinding);
  toggleSpec.state = &toggleLegacy;
  toggleSpec.on = true;
  toggleSpec.trackStyle = 224u;
  toggleSpec.knobStyle = 225u;
  toggleSpec.focusStyle = 226u;
  toggleSpec.size.preferredWidth = 60.0f;
  toggleSpec.size.preferredHeight = 28.0f;

  PrimeStage::State<bool> checkboxBinding;
  checkboxBinding.value = true;
  PrimeStage::CheckboxState checkboxLegacy;
  checkboxLegacy.checked = false;

  PrimeStage::CheckboxSpec checkboxSpec;
  checkboxSpec.binding = PrimeStage::bind(checkboxBinding);
  checkboxSpec.state = &checkboxLegacy;
  checkboxSpec.checked = false;
  checkboxSpec.label = "Enabled";
  checkboxSpec.boxStyle = 234u;
  checkboxSpec.checkStyle = 235u;
  checkboxSpec.focusStyle = 236u;
  checkboxSpec.textStyle = 237u;

  std::vector<bool> toggleValues;
  std::vector<bool> checkboxValues;
  toggleSpec.callbacks.onChanged = [&](bool on) { toggleValues.push_back(on); };
  checkboxSpec.callbacks.onChanged = [&](bool checked) { checkboxValues.push_back(checked); };

  PrimeStage::UiNode toggle = stack.createToggle(toggleSpec);
  PrimeStage::UiNode checkbox = stack.createCheckbox(checkboxSpec);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 260.0f, 140.0f);
  PrimeFrame::LayoutOut const* toggleOut = layout.get(toggle.nodeId());
  PrimeFrame::LayoutOut const* checkboxOut = layout.get(checkbox.nodeId());
  REQUIRE(toggleOut != nullptr);
  REQUIRE(checkboxOut != nullptr);

  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;

  float toggleX = toggleOut->absX + toggleOut->absW * 0.5f;
  float toggleY = toggleOut->absY + toggleOut->absH * 0.5f;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 1, toggleX, toggleY),
                  frame,
                  layout,
                  &focus);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 1, toggleX, toggleY),
                  frame,
                  layout,
                  &focus);
  // Binding value (false -> true) wins over legacy initial value (true).
  CHECK(toggleBinding.value == true);
  CHECK(toggleLegacy.on == true);
  REQUIRE(!toggleValues.empty());
  CHECK(toggleValues.back() == true);

  PrimeFrame::Event keySpace;
  keySpace.type = PrimeFrame::EventType::KeyDown;
  keySpace.key = 0x2C; // Space
  router.dispatch(keySpace, frame, layout, &focus);
  CHECK(toggleBinding.value == false);
  CHECK(toggleLegacy.on == false);
  REQUIRE(toggleValues.size() >= 2);
  CHECK(toggleValues.back() == false);

  float checkboxX = checkboxOut->absX + checkboxOut->absW * 0.5f;
  float checkboxY = checkboxOut->absY + checkboxOut->absH * 0.5f;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 2, checkboxX, checkboxY),
                  frame,
                  layout,
                  &focus);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 2, checkboxX, checkboxY),
                  frame,
                  layout,
                  &focus);
  // Binding value (true -> false) wins over legacy initial value (false).
  CHECK(checkboxBinding.value == false);
  CHECK(checkboxLegacy.checked == false);
  REQUIRE(!checkboxValues.empty());
  CHECK(checkboxValues.back() == false);

  PrimeFrame::Event keyEnter;
  keyEnter.type = PrimeFrame::EventType::KeyDown;
  keyEnter.key = 0x28; // Enter
  router.dispatch(keyEnter, frame, layout, &focus);
  CHECK(checkboxBinding.value == true);
  CHECK(checkboxLegacy.checked == true);
  REQUIRE(checkboxValues.size() >= 2);
  CHECK(checkboxValues.back() == true);
}

TEST_CASE("PrimeStage toggle and checkbox patch visuals in place without rebuild") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 280.0f, 180.0f);

  PrimeStage::StackSpec stackSpec;
  stackSpec.gap = 12.0f;
  stackSpec.size.stretchX = 1.0f;
  stackSpec.size.stretchY = 1.0f;
  PrimeStage::UiNode stack = root.createVerticalStack(stackSpec);

  PrimeStage::ToggleState toggleState;
  toggleState.on = false;
  PrimeStage::ToggleSpec toggleSpec;
  toggleSpec.state = &toggleState;
  toggleSpec.trackStyle = 241u;
  toggleSpec.knobStyle = 242u;
  toggleSpec.focusStyle = 243u;
  toggleSpec.size.preferredWidth = 64.0f;
  toggleSpec.size.preferredHeight = 28.0f;

  PrimeStage::CheckboxState checkboxState;
  checkboxState.checked = false;
  PrimeStage::CheckboxSpec checkboxSpec;
  checkboxSpec.state = &checkboxState;
  checkboxSpec.label = "Patch";
  checkboxSpec.boxStyle = 251u;
  checkboxSpec.checkStyle = 252u;
  checkboxSpec.focusStyle = 253u;
  checkboxSpec.textStyle = 254u;

  PrimeStage::UiNode toggle = stack.createToggle(toggleSpec);
  PrimeStage::UiNode checkbox = stack.createCheckbox(checkboxSpec);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 280.0f, 180.0f);
  PrimeFrame::LayoutOut const* toggleOut = layout.get(toggle.nodeId());
  PrimeFrame::LayoutOut const* checkboxOut = layout.get(checkbox.nodeId());
  REQUIRE(toggleOut != nullptr);
  REQUIRE(checkboxOut != nullptr);

  PrimeFrame::NodeId knobNodeId =
      findFirstNodeWithRectTokenInSubtree(frame, toggle.nodeId(), toggleSpec.knobStyle);
  PrimeFrame::NodeId checkNodeId =
      findFirstNodeWithRectTokenInSubtree(frame, checkbox.nodeId(), checkboxSpec.checkStyle);
  REQUIRE(knobNodeId.isValid());
  REQUIRE(checkNodeId.isValid());
  PrimeFrame::Node const* knobBefore = frame.getNode(knobNodeId);
  PrimeFrame::Node const* checkBefore = frame.getNode(checkNodeId);
  REQUIRE(knobBefore != nullptr);
  REQUIRE(checkBefore != nullptr);
  float knobBeforeX = knobBefore->localX;
  CHECK_FALSE(checkBefore->visible);

  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;

  float toggleX = toggleOut->absX + toggleOut->absW * 0.5f;
  float toggleY = toggleOut->absY + toggleOut->absH * 0.5f;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 1, toggleX, toggleY),
                  frame,
                  layout,
                  &focus);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 1, toggleX, toggleY),
                  frame,
                  layout,
                  &focus);
  CHECK(toggleState.on);
  PrimeFrame::Node const* knobAfterPointer = frame.getNode(knobNodeId);
  REQUIRE(knobAfterPointer != nullptr);
  float knobAfterPointerX = knobAfterPointer->localX;
  CHECK(knobAfterPointerX > knobBeforeX);

  focus.setFocus(frame, layout, toggle.nodeId());
  router.dispatch(makeKeyDownEvent(PrimeStage::KeyCode::Space), frame, layout, &focus);
  CHECK_FALSE(toggleState.on);
  PrimeFrame::Node const* knobAfterKey = frame.getNode(knobNodeId);
  REQUIRE(knobAfterKey != nullptr);
  CHECK(knobAfterKey->localX < knobAfterPointerX);

  float checkboxX = checkboxOut->absX + checkboxOut->absW * 0.5f;
  float checkboxY = checkboxOut->absY + checkboxOut->absH * 0.5f;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 2, checkboxX, checkboxY),
                  frame,
                  layout,
                  &focus);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 2, checkboxX, checkboxY),
                  frame,
                  layout,
                  &focus);
  CHECK(checkboxState.checked);
  PrimeFrame::Node const* checkAfterPointer = frame.getNode(checkNodeId);
  REQUIRE(checkAfterPointer != nullptr);
  CHECK(checkAfterPointer->visible);

  focus.setFocus(frame, layout, checkbox.nodeId());
  router.dispatch(makeKeyDownEvent(PrimeStage::KeyCode::Enter), frame, layout, &focus);
  CHECK_FALSE(checkboxState.checked);
  PrimeFrame::Node const* checkAfterKey = frame.getNode(checkNodeId);
  REQUIRE(checkAfterKey != nullptr);
  CHECK_FALSE(checkAfterKey->visible);
}

TEST_CASE("PrimeStage accessibility keyboard focus and activation contract is consistent") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 320.0f, 180.0f);

  PrimeStage::StackSpec stackSpec;
  stackSpec.gap = 10.0f;
  stackSpec.size.stretchX = 1.0f;
  stackSpec.size.stretchY = 1.0f;
  PrimeStage::UiNode stack = root.createVerticalStack(stackSpec);

  int buttonActivations = 0;
  int toggleActivations = 0;
  int checkboxActivations = 0;

  PrimeStage::ButtonSpec buttonSpec;
  buttonSpec.label = "Apply";
  buttonSpec.tabIndex = 10;
  buttonSpec.size.preferredWidth = 120.0f;
  buttonSpec.size.preferredHeight = 28.0f;
  buttonSpec.callbacks.onClick = [&]() { buttonActivations += 1; };

  PrimeStage::ToggleSpec toggleSpec;
  toggleSpec.tabIndex = 20;
  toggleSpec.size.preferredWidth = 56.0f;
  toggleSpec.size.preferredHeight = 24.0f;
  toggleSpec.trackStyle = 501u;
  toggleSpec.knobStyle = 502u;
  toggleSpec.callbacks.onChanged = [&](bool) { toggleActivations += 1; };

  PrimeStage::CheckboxSpec checkboxSpec;
  checkboxSpec.label = "Enable";
  checkboxSpec.tabIndex = 30;
  checkboxSpec.boxStyle = 511u;
  checkboxSpec.checkStyle = 512u;
  checkboxSpec.callbacks.onChanged = [&](bool) { checkboxActivations += 1; };

  PrimeStage::UiNode button = stack.createButton(buttonSpec);
  PrimeStage::UiNode toggle = stack.createToggle(toggleSpec);
  PrimeStage::UiNode checkbox = stack.createCheckbox(checkboxSpec);
  (void)toggle;
  (void)checkbox;

  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 320.0f, 180.0f);
  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;

  CHECK(focus.handleTab(frame, layout, true));
  CHECK(focus.focusedNode() == button.nodeId());
  router.dispatch(makeKeyDownEvent(PrimeStage::KeyCode::Enter), frame, layout, &focus);
  router.dispatch(makeKeyDownEvent(PrimeStage::KeyCode::Space), frame, layout, &focus);
  CHECK(buttonActivations == 2);

  CHECK(focus.handleTab(frame, layout, true));
  router.dispatch(makeKeyDownEvent(PrimeStage::KeyCode::Enter), frame, layout, &focus);
  router.dispatch(makeKeyDownEvent(PrimeStage::KeyCode::Space), frame, layout, &focus);
  CHECK(toggleActivations == 2);

  CHECK(focus.handleTab(frame, layout, true));
  router.dispatch(makeKeyDownEvent(PrimeStage::KeyCode::Enter), frame, layout, &focus);
  router.dispatch(makeKeyDownEvent(PrimeStage::KeyCode::Space), frame, layout, &focus);
  CHECK(checkboxActivations == 2);
}

TEST_CASE("PrimeStage tree view hover/selection callbacks and double click toggle") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 160.0f);

  PrimeStage::TreeViewSpec spec;
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 120.0f;
  spec.rowStartY = 0.0f;
  spec.rowHeight = 20.0f;
  spec.rowGap = 0.0f;
  spec.rowStartX = 8.0f;
  spec.rowWidthInset = 0.0f;
  spec.rowStyle = 301u;
  spec.rowAltStyle = 302u;
  spec.hoverStyle = 303u;
  spec.selectionStyle = 304u;
  spec.selectionAccentStyle = 305u;
  spec.textStyle = 401u;
  spec.selectedTextStyle = 402u;
  spec.doubleClickMs = 1000.0f;
  spec.nodes = {
      PrimeStage::TreeNode{"Root", {PrimeStage::TreeNode{"Child"}}, true, false},
      PrimeStage::TreeNode{"Second", {}, true, false}
  };

  int hoverRow = -2;
  int selectedRow = -2;
  int expandedRow = -2;
  bool expandedValue = false;
  spec.callbacks.onHoverChanged = [&](int row) { hoverRow = row; };
  spec.callbacks.onSelectionChanged = [&](PrimeStage::TreeViewRowInfo const& info) {
    selectedRow = info.rowIndex;
  };
  spec.callbacks.onExpandedChanged = [&](PrimeStage::TreeViewRowInfo const& info, bool expanded) {
    expandedRow = info.rowIndex;
    expandedValue = expanded;
  };

  PrimeStage::UiNode tree = root.createTreeView(spec);
  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 240.0f, 160.0f);
  PrimeFrame::LayoutOut const* out = layout.get(tree.nodeId());
  REQUIRE(out != nullptr);

  PrimeFrame::EventRouter router;
  PrimeFrame::Event move = makePointerEvent(PrimeFrame::EventType::PointerMove, 1,
                                            out->absX + spec.rowStartX + 32.0f,
                                            out->absY + spec.rowStartY + spec.rowHeight * 0.5f);
  router.dispatch(move, frame, layout);
  CHECK(hoverRow == 0);

  PrimeFrame::Event down = makePointerEvent(PrimeFrame::EventType::PointerDown, 1, move.x, move.y);
  router.dispatch(down, frame, layout);
  CHECK(selectedRow == 0);

  PrimeFrame::Event down2 = makePointerEvent(PrimeFrame::EventType::PointerDown, 1, move.x, move.y);
  router.dispatch(down2, frame, layout);
  CHECK(expandedRow == 0);
  CHECK(expandedValue == false);

  PrimeFrame::Event up = makePointerEvent(PrimeFrame::EventType::PointerUp, 1, move.x, move.y);
  router.dispatch(up, frame, layout);

  PrimeFrame::Event outMove = makePointerEvent(PrimeFrame::EventType::PointerMove, 1,
                                               out->absX - 10.0f, out->absY - 10.0f);
  router.dispatch(outMove, frame, layout);
  CHECK(hoverRow == -1);
}

TEST_CASE("PrimeStage tree view keyboard navigation") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 160.0f);

  PrimeStage::TreeViewSpec spec;
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 120.0f;
  spec.rowStartY = 0.0f;
  spec.rowHeight = 20.0f;
  spec.rowGap = 0.0f;
  spec.rowStartX = 8.0f;
  spec.rowWidthInset = 0.0f;
  spec.rowStyle = 311u;
  spec.rowAltStyle = 312u;
  spec.selectionStyle = 313u;
  spec.selectionAccentStyle = 314u;
  spec.textStyle = 411u;
  spec.selectedTextStyle = 412u;
  spec.nodes = {
      PrimeStage::TreeNode{"First"},
      PrimeStage::TreeNode{"Second"}
  };

  int selectedRow = -1;
  spec.callbacks.onSelectionChanged = [&](PrimeStage::TreeViewRowInfo const& info) {
    selectedRow = info.rowIndex;
  };

  PrimeStage::UiNode tree = root.createTreeView(spec);
  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 240.0f, 160.0f);
  PrimeFrame::LayoutOut const* out = layout.get(tree.nodeId());
  REQUIRE(out != nullptr);

  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;

  PrimeFrame::Event down = makePointerEvent(PrimeFrame::EventType::PointerDown, 1,
                                            out->absX + spec.rowStartX + 16.0f,
                                            out->absY + spec.rowStartY + spec.rowHeight * 0.5f);
  router.dispatch(down, frame, layout, &focus);
  CHECK(selectedRow == 0);

  PrimeFrame::Event keyDown;
  keyDown.type = PrimeFrame::EventType::KeyDown;
  keyDown.key = 0x51;
  router.dispatch(keyDown, frame, layout, &focus);
  CHECK(selectedRow == 1);
}

TEST_CASE("PrimeStage tree view left moves to parent when leaf selected") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 160.0f);

  PrimeStage::TreeViewSpec spec;
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 120.0f;
  spec.rowStartY = 0.0f;
  spec.rowHeight = 20.0f;
  spec.rowGap = 0.0f;
  spec.rowStartX = 8.0f;
  spec.rowWidthInset = 0.0f;
  spec.rowStyle = 331u;
  spec.rowAltStyle = 332u;
  spec.selectionStyle = 333u;
  spec.selectionAccentStyle = 334u;
  spec.textStyle = 431u;
  spec.selectedTextStyle = 432u;
  spec.nodes = {
      PrimeStage::TreeNode{"Parent", {PrimeStage::TreeNode{"Child"}}, true, false}
  };

  int selectedRow = -1;
  spec.callbacks.onSelectionChanged = [&](PrimeStage::TreeViewRowInfo const& info) {
    selectedRow = info.rowIndex;
  };

  PrimeStage::UiNode tree = root.createTreeView(spec);
  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 240.0f, 160.0f);
  PrimeFrame::LayoutOut const* out = layout.get(tree.nodeId());
  REQUIRE(out != nullptr);

  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;

  PrimeFrame::Event down = makePointerEvent(PrimeFrame::EventType::PointerDown, 1,
                                            out->absX + spec.rowStartX + 16.0f,
                                            out->absY + spec.rowStartY + spec.rowHeight * 1.5f);
  router.dispatch(down, frame, layout, &focus);
  CHECK(selectedRow == 1);

  PrimeFrame::Event keyLeft;
  keyLeft.type = PrimeFrame::EventType::KeyDown;
  keyLeft.key = 0x50;
  router.dispatch(keyLeft, frame, layout, &focus);
  CHECK(selectedRow == 0);
}

TEST_CASE("PrimeStage tree view right moves to last child") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 160.0f);

  PrimeStage::TreeViewSpec spec;
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 120.0f;
  spec.rowStartY = 0.0f;
  spec.rowHeight = 20.0f;
  spec.rowGap = 0.0f;
  spec.rowStartX = 8.0f;
  spec.rowWidthInset = 0.0f;
  spec.rowStyle = 341u;
  spec.rowAltStyle = 342u;
  spec.selectionStyle = 343u;
  spec.selectionAccentStyle = 344u;
  spec.textStyle = 441u;
  spec.selectedTextStyle = 442u;
  spec.nodes = {
      PrimeStage::TreeNode{"Parent",
                           {PrimeStage::TreeNode{"Child A"},
                            PrimeStage::TreeNode{"Child B"},
                            PrimeStage::TreeNode{"Child C"}},
                           true,
                           false}
  };

  int selectedRow = -1;
  spec.callbacks.onSelectionChanged = [&](PrimeStage::TreeViewRowInfo const& info) {
    selectedRow = info.rowIndex;
  };

  PrimeStage::UiNode tree = root.createTreeView(spec);
  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 240.0f, 160.0f);
  PrimeFrame::LayoutOut const* out = layout.get(tree.nodeId());
  REQUIRE(out != nullptr);

  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;

  PrimeFrame::Event down = makePointerEvent(PrimeFrame::EventType::PointerDown, 1,
                                            out->absX + spec.rowStartX + 16.0f,
                                            out->absY + spec.rowStartY + spec.rowHeight * 0.5f);
  router.dispatch(down, frame, layout, &focus);
  CHECK(selectedRow == 0);

  PrimeFrame::Event keyRight;
  keyRight.type = PrimeFrame::EventType::KeyDown;
  keyRight.key = 0x4F;
  router.dispatch(keyRight, frame, layout, &focus);
  CHECK(selectedRow == 3);
}

TEST_CASE("PrimeStage tree view page and edge keys") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 160.0f);

  PrimeStage::TreeViewSpec spec;
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 30.0f;
  spec.rowStartY = 0.0f;
  spec.rowHeight = 10.0f;
  spec.rowGap = 0.0f;
  spec.rowStartX = 8.0f;
  spec.rowWidthInset = 0.0f;
  spec.rowStyle = 351u;
  spec.rowAltStyle = 352u;
  spec.selectionStyle = 353u;
  spec.selectionAccentStyle = 354u;
  spec.textStyle = 451u;
  spec.selectedTextStyle = 452u;
  spec.nodes = {
      PrimeStage::TreeNode{"Row 1"},
      PrimeStage::TreeNode{"Row 2"},
      PrimeStage::TreeNode{"Row 3"},
      PrimeStage::TreeNode{"Row 4"},
      PrimeStage::TreeNode{"Row 5"},
      PrimeStage::TreeNode{"Row 6"},
      PrimeStage::TreeNode{"Row 7"},
      PrimeStage::TreeNode{"Row 8"}
  };

  int selectedRow = -1;
  spec.callbacks.onSelectionChanged = [&](PrimeStage::TreeViewRowInfo const& info) {
    selectedRow = info.rowIndex;
  };

  PrimeStage::UiNode tree = root.createTreeView(spec);
  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 240.0f, 160.0f);
  PrimeFrame::LayoutOut const* out = layout.get(tree.nodeId());
  REQUIRE(out != nullptr);

  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;

  PrimeFrame::Event down = makePointerEvent(PrimeFrame::EventType::PointerDown, 1,
                                            out->absX + spec.rowStartX + 16.0f,
                                            out->absY + spec.rowStartY + spec.rowHeight * 1.5f);
  router.dispatch(down, frame, layout, &focus);
  CHECK(selectedRow == 1);

  PrimeFrame::Event keyPageDown;
  keyPageDown.type = PrimeFrame::EventType::KeyDown;
  keyPageDown.key = 0x4E;
  router.dispatch(keyPageDown, frame, layout, &focus);
  CHECK(selectedRow == 4);

  PrimeFrame::Event keyPageUp;
  keyPageUp.type = PrimeFrame::EventType::KeyDown;
  keyPageUp.key = 0x4B;
  router.dispatch(keyPageUp, frame, layout, &focus);
  CHECK(selectedRow == 1);

  PrimeFrame::Event keyHome;
  keyHome.type = PrimeFrame::EventType::KeyDown;
  keyHome.key = 0x4A;
  router.dispatch(keyHome, frame, layout, &focus);
  CHECK(selectedRow == 0);

  PrimeFrame::Event keyEnd;
  keyEnd.type = PrimeFrame::EventType::KeyDown;
  keyEnd.key = 0x4D;
  router.dispatch(keyEnd, frame, layout, &focus);
  CHECK(selectedRow == 7);
}

TEST_CASE("PrimeStage tree view scroll updates callback") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 140.0f);

  PrimeStage::TreeViewSpec spec;
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 80.0f;
  spec.rowStartY = 0.0f;
  spec.rowHeight = 20.0f;
  spec.rowGap = 0.0f;
  spec.rowStartX = 8.0f;
  spec.rowWidthInset = 0.0f;
  spec.rowStyle = 321u;
  spec.rowAltStyle = 322u;
  spec.selectionStyle = 323u;
  spec.selectionAccentStyle = 324u;
  spec.textStyle = 421u;
  spec.selectedTextStyle = 422u;
  spec.scrollBar.autoThumb = true;
  spec.nodes = {
      PrimeStage::TreeNode{"One"},
      PrimeStage::TreeNode{"Two"},
      PrimeStage::TreeNode{"Three"},
      PrimeStage::TreeNode{"Four"},
      PrimeStage::TreeNode{"Five"},
      PrimeStage::TreeNode{"Six"}
  };

  bool scrolled = false;
  PrimeStage::TreeViewScrollInfo lastScroll;
  spec.callbacks.onScrollChanged = [&](PrimeStage::TreeViewScrollInfo const& info) {
    scrolled = true;
    lastScroll = info;
  };

  PrimeStage::UiNode tree = root.createTreeView(spec);
  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 240.0f, 140.0f);
  PrimeFrame::LayoutOut const* out = layout.get(tree.nodeId());
  REQUIRE(out != nullptr);

  PrimeFrame::EventRouter router;
  PrimeFrame::Event scroll;
  scroll.type = PrimeFrame::EventType::PointerScroll;
  scroll.x = out->absX + 12.0f;
  scroll.y = out->absY + 12.0f;
  scroll.scrollY = 30.0f;
  router.dispatch(scroll, frame, layout);

  CHECK(scrolled);
  CHECK(lastScroll.progress >= 0.0f);
  CHECK(lastScroll.progress <= 1.0f);
}

TEST_CASE("PrimeStage tree view scrolls with mouse wheel when scroll bar is disabled") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 140.0f);

  PrimeStage::TreeViewSpec spec;
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 80.0f;
  spec.rowStartY = 0.0f;
  spec.rowHeight = 20.0f;
  spec.rowGap = 0.0f;
  spec.rowStartX = 8.0f;
  spec.rowWidthInset = 0.0f;
  spec.rowStyle = 321u;
  spec.rowAltStyle = 322u;
  spec.selectionStyle = 323u;
  spec.selectionAccentStyle = 324u;
  spec.textStyle = 421u;
  spec.selectedTextStyle = 422u;
  spec.keyboardNavigation = false;
  spec.showScrollBar = false;
  spec.scrollBar.enabled = false;
  spec.nodes = {
      PrimeStage::TreeNode{"One"},
      PrimeStage::TreeNode{"Two"},
      PrimeStage::TreeNode{"Three"},
      PrimeStage::TreeNode{"Four"},
      PrimeStage::TreeNode{"Five"},
      PrimeStage::TreeNode{"Six"},
  };

  bool scrolled = false;
  PrimeStage::TreeViewScrollInfo lastScroll;
  spec.callbacks.onScrollChanged = [&](PrimeStage::TreeViewScrollInfo const& info) {
    scrolled = true;
    lastScroll = info;
  };

  PrimeStage::UiNode tree = root.createTreeView(spec);
  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 240.0f, 140.0f);
  PrimeFrame::LayoutOut const* out = layout.get(tree.nodeId());
  REQUIRE(out != nullptr);

  PrimeFrame::EventRouter router;
  PrimeFrame::Event scroll;
  scroll.type = PrimeFrame::EventType::PointerScroll;
  scroll.x = out->absX + 12.0f;
  scroll.y = out->absY + 12.0f;
  scroll.scrollY = 30.0f;
  router.dispatch(scroll, frame, layout);

  CHECK(scrolled);
  CHECK(lastScroll.offset > 0.0f);
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

TEST_CASE("PrimeStage progress bar state-backed interactions patch fill in place") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 260.0f, 120.0f);

  PrimeStage::ProgressBarState progressState;
  progressState.value = 0.20f;
  std::vector<float> values;

  PrimeStage::ProgressBarSpec spec;
  spec.state = &progressState;
  spec.value = 0.85f;
  spec.trackStyle = 321u;
  spec.fillStyle = 322u;
  spec.focusStyle = 323u;
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 14.0f;
  spec.callbacks.onValueChanged = [&](float value) { values.push_back(value); };

  PrimeStage::UiNode progress = root.createProgressBar(spec);

  PrimeFrame::NodeId fillNodeId =
      findFirstNodeWithRectTokenInSubtree(frame, progress.nodeId(), spec.fillStyle);
  REQUIRE(fillNodeId.isValid());
  PrimeFrame::Node const* fillBefore = frame.getNode(fillNodeId);
  REQUIRE(fillBefore != nullptr);
  REQUIRE(fillBefore->sizeHint.width.preferred.has_value());
  float widthBefore = fillBefore->sizeHint.width.preferred.value();
  CHECK(widthBefore == doctest::Approx(40.0f));

  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 260.0f, 120.0f);
  PrimeFrame::LayoutOut const* progressOut = layout.get(progress.nodeId());
  REQUIRE(progressOut != nullptr);

  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;
  float clickX = progressOut->absX + progressOut->absW * 0.80f;
  float clickY = progressOut->absY + progressOut->absH * 0.5f;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 1, clickX, clickY),
                  frame,
                  layout,
                  &focus);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 1, clickX, clickY),
                  frame,
                  layout,
                  &focus);
  CHECK(progressState.value > 0.70f);
  REQUIRE(!values.empty());

  PrimeFrame::Node const* fillAfterPointer = frame.getNode(fillNodeId);
  REQUIRE(fillAfterPointer != nullptr);
  REQUIRE(fillAfterPointer->sizeHint.width.preferred.has_value());
  CHECK(fillAfterPointer->sizeHint.width.preferred.value() > widthBefore);

  focus.setFocus(frame, layout, progress.nodeId());
  router.dispatch(makeKeyDownEvent(PrimeStage::KeyCode::Home), frame, layout, &focus);
  CHECK(progressState.value == doctest::Approx(0.0f));
  PrimeFrame::Node const* fillAfterHome = frame.getNode(fillNodeId);
  REQUIRE(fillAfterHome != nullptr);
  CHECK_FALSE(fillAfterHome->visible);

  router.dispatch(makeKeyDownEvent(PrimeStage::KeyCode::End), frame, layout, &focus);
  CHECK(progressState.value == doctest::Approx(1.0f));
  PrimeFrame::Node const* fillAfterEnd = frame.getNode(fillNodeId);
  REQUIRE(fillAfterEnd != nullptr);
  CHECK(fillAfterEnd->visible);
  REQUIRE(fillAfterEnd->sizeHint.width.preferred.has_value());
  CHECK(fillAfterEnd->sizeHint.width.preferred.value() == doctest::Approx(200.0f));
}

TEST_CASE("PrimeStage disabled progress bar ignores interaction callbacks") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 100.0f);

  PrimeStage::ProgressBarState state;
  state.value = 0.45f;
  int changed = 0;

  PrimeStage::ProgressBarSpec spec;
  spec.state = &state;
  spec.enabled = false;
  spec.trackStyle = 331u;
  spec.fillStyle = 332u;
  spec.size.preferredWidth = 180.0f;
  spec.size.preferredHeight = 12.0f;
  spec.callbacks.onValueChanged = [&](float) { changed += 1; };

  PrimeStage::UiNode progress = root.createProgressBar(spec);
  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 240.0f, 100.0f);
  PrimeFrame::LayoutOut const* out = layout.get(progress.nodeId());
  REQUIRE(out != nullptr);

  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;
  float x = out->absX + out->absW * 0.9f;
  float y = out->absY + out->absH * 0.5f;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 1, x, y), frame, layout, &focus);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 1, x, y), frame, layout, &focus);
  router.dispatch(makeKeyDownEvent(PrimeStage::KeyCode::End), frame, layout, &focus);

  CHECK(changed == 0);
  CHECK(state.value == doctest::Approx(0.45f));
}

TEST_CASE("PrimeStage table callbacks keep row text alive for short-lived source buffers") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 320.0f, 180.0f);

  PrimeStage::TableSpec spec;
  spec.columns = {{"Name", 120.0f, 0u, 0u}, {"Value", 120.0f, 0u, 0u}};
  spec.size.preferredWidth = 260.0f;
  spec.size.preferredHeight = 120.0f;
  spec.rowHeight = 24.0f;
  spec.rowGap = 0.0f;
  spec.headerHeight = 20.0f;

  std::vector<std::string> sourceCells = {"Alpha", "One", "Beta", "Two"};
  spec.rows = {
      {sourceCells[0], sourceCells[1]},
      {sourceCells[2], sourceCells[3]},
  };

  int clickedRow = -1;
  std::vector<std::string> clickedCells;
  spec.callbacks.onRowClicked = [&](PrimeStage::TableRowInfo const& info) {
    clickedRow = info.rowIndex;
    clickedCells.clear();
    clickedCells.reserve(info.row.size());
    for (std::string_view cell : info.row) {
      clickedCells.emplace_back(cell);
    }
  };

  PrimeStage::UiNode table = root.createTable(spec);

  sourceCells[0] = "omega";
  sourceCells[1] = "uno";
  sourceCells[2] = "zeta";
  sourceCells[3] = "dos";

  PrimeFrame::NodeId callbackNodeId = findFirstNodeWithOnEventInSubtree(frame, table.nodeId());
  REQUIRE(callbackNodeId.isValid());

  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 320.0f, 180.0f);
  PrimeFrame::LayoutOut const* callbackOut = layout.get(callbackNodeId);
  REQUIRE(callbackOut != nullptr);

  float clickX = callbackOut->absX + callbackOut->absW * 0.5f;
  float clickY = callbackOut->absY + spec.rowHeight * 0.5f;

  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 1, clickX, clickY),
                  frame,
                  layout,
                  &focus);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 1, clickX, clickY),
                  frame,
                  layout,
                  &focus);

  CHECK(clickedRow == 0);
  REQUIRE(clickedCells.size() == 2u);
  CHECK(clickedCells[0] == "Alpha");
  CHECK(clickedCells[1] == "One");
}

TEST_CASE("PrimeStage window builder clamps geometry and emits slots") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 640.0f, 480.0f);

  PrimeStage::WindowSpec spec;
  spec.title = "Inspector";
  spec.positionX = 32.0f;
  spec.positionY = 24.0f;
  spec.width = 120.0f;
  spec.height = 80.0f;
  spec.minWidth = 220.0f;
  spec.minHeight = 140.0f;
  spec.titleBarHeight = 24.0f;
  spec.contentPadding = 8.0f;
  spec.resizeHandleSize = -4.0f;
  spec.tabIndex = -9;
  spec.frameStyle = 701u;
  spec.titleBarStyle = 702u;
  spec.contentStyle = 703u;

  PrimeStage::Window window = root.createWindow(spec);

  PrimeFrame::Node const* windowNode = frame.getNode(window.root.nodeId());
  PrimeFrame::Node const* titleNode = frame.getNode(window.titleBar.nodeId());
  PrimeFrame::Node const* contentNode = frame.getNode(window.content.nodeId());
  REQUIRE(windowNode != nullptr);
  REQUIRE(titleNode != nullptr);
  REQUIRE(contentNode != nullptr);

  REQUIRE(windowNode->sizeHint.width.preferred.has_value());
  REQUIRE(windowNode->sizeHint.height.preferred.has_value());
  CHECK(windowNode->localX == doctest::Approx(32.0f));
  CHECK(windowNode->localY == doctest::Approx(24.0f));
  CHECK(windowNode->sizeHint.width.preferred.value() == doctest::Approx(220.0f));
  CHECK(windowNode->sizeHint.height.preferred.value() == doctest::Approx(140.0f));
  CHECK(windowNode->tabIndex == -1);

  REQUIRE(titleNode->sizeHint.height.preferred.has_value());
  CHECK(titleNode->sizeHint.height.preferred.value() == doctest::Approx(24.0f));
  CHECK(contentNode->localY == doctest::Approx(24.0f));
  REQUIRE(contentNode->sizeHint.height.preferred.has_value());
  CHECK(contentNode->sizeHint.height.preferred.value() == doctest::Approx(116.0f));
  CHECK_FALSE(window.resizeHandleId.isValid());
}

TEST_CASE("PrimeStage window builder wires focus move and resize callbacks") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 640.0f, 480.0f);

  int focusRequests = 0;
  int focusChanges = 0;
  int moveStart = 0;
  int moveEnd = 0;
  int resizeStart = 0;
  int resizeEnd = 0;
  std::vector<std::pair<float, float>> moveDeltas;
  std::vector<std::pair<float, float>> resizeDeltas;

  PrimeStage::WindowSpec spec;
  spec.title = "Main";
  spec.positionX = 50.0f;
  spec.positionY = 40.0f;
  spec.width = 260.0f;
  spec.height = 180.0f;
  spec.titleBarHeight = 28.0f;
  spec.resizeHandleSize = 16.0f;
  spec.frameStyle = 711u;
  spec.titleBarStyle = 712u;
  spec.contentStyle = 713u;
  spec.resizeHandleStyle = 714u;
  spec.callbacks.onFocusRequested = [&]() { focusRequests += 1; };
  spec.callbacks.onFocusChanged = [&](bool focused) { focusChanges += focused ? 1 : -1; };
  spec.callbacks.onMoveStarted = [&]() { moveStart += 1; };
  spec.callbacks.onMoved = [&](float deltaX, float deltaY) {
    moveDeltas.push_back({deltaX, deltaY});
  };
  spec.callbacks.onMoveEnded = [&]() { moveEnd += 1; };
  spec.callbacks.onResizeStarted = [&]() { resizeStart += 1; };
  spec.callbacks.onResized = [&](float deltaWidth, float deltaHeight) {
    resizeDeltas.push_back({deltaWidth, deltaHeight});
  };
  spec.callbacks.onResizeEnded = [&]() { resizeEnd += 1; };

  PrimeStage::Window window = root.createWindow(spec);
  REQUIRE(window.resizeHandleId.isValid());

  PrimeFrame::Node const* windowNode = frame.getNode(window.root.nodeId());
  REQUIRE(windowNode != nullptr);
  REQUIRE(windowNode->callbacks != PrimeFrame::InvalidCallbackId);
  PrimeFrame::Callback const* windowCallbacks = frame.getCallback(windowNode->callbacks);
  REQUIRE(windowCallbacks != nullptr);
  REQUIRE(windowCallbacks->onFocus);
  REQUIRE(windowCallbacks->onBlur);
  windowCallbacks->onFocus();
  windowCallbacks->onBlur();
  CHECK(focusChanges == 0);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 640.0f, 480.0f);
  PrimeFrame::LayoutOut const* titleOut = layout.get(window.titleBar.nodeId());
  PrimeFrame::LayoutOut const* resizeOut = layout.get(window.resizeHandleId);
  REQUIRE(titleOut != nullptr);
  REQUIRE(resizeOut != nullptr);

  PrimeFrame::EventRouter router;
  router.setDragThreshold(0.0f);
  PrimeFrame::FocusManager focus;

  float titleX = titleOut->absX + titleOut->absW * 0.5f;
  float titleY = titleOut->absY + titleOut->absH * 0.5f;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 1, titleX, titleY),
                  frame,
                  layout,
                  &focus);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDrag, 1, titleX + 18.0f, titleY + 11.0f),
                  frame,
                  layout,
                  &focus);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 1, titleX + 18.0f, titleY + 11.0f),
                  frame,
                  layout,
                  &focus);

  float resizeX = resizeOut->absX + resizeOut->absW * 0.5f;
  float resizeY = resizeOut->absY + resizeOut->absH * 0.5f;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 2, resizeX, resizeY),
                  frame,
                  layout,
                  &focus);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDrag, 2, resizeX + 14.0f, resizeY + 9.0f),
                  frame,
                  layout,
                  &focus);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 2, resizeX + 14.0f, resizeY + 9.0f),
                  frame,
                  layout,
                  &focus);

  CHECK(focusRequests >= 2);
  CHECK(moveStart == 1);
  CHECK(moveEnd == 1);
  REQUIRE(moveDeltas.size() == 1u);
  CHECK(moveDeltas[0].first == doctest::Approx(18.0f));
  CHECK(moveDeltas[0].second == doctest::Approx(11.0f));

  CHECK(resizeStart == 1);
  CHECK(resizeEnd == 1);
  REQUIRE(resizeDeltas.size() == 1u);
  CHECK(resizeDeltas[0].first == doctest::Approx(14.0f));
  CHECK(resizeDeltas[0].second == doctest::Approx(9.0f));
}
