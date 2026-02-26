#include "PrimeStage/Ui.h"
#include "PrimeFrame/Events.h"
#include "PrimeFrame/Frame.h"
#include "PrimeFrame/Layout.h"

#include "third_party/doctest.h"

#include <string>
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
