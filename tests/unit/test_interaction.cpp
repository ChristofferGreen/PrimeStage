#include "PrimeStage/Ui.h"
#include "PrimeFrame/Events.h"
#include "PrimeFrame/Frame.h"
#include "PrimeFrame/Layout.h"
#include "src/PrimeStageCollectionInternals.h"

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
  for (PrimeFrame::NodeId childId : node->children) {
    PrimeFrame::NodeId candidate = findFirstNodeWithOnEventInSubtree(frame, childId);
    if (candidate.isValid()) {
      return candidate;
    }
  }
  if (node->callbacks != PrimeFrame::InvalidCallbackId) {
    PrimeFrame::Callback const* callback = frame.getCallback(node->callbacks);
    if (callback && callback->onEvent) {
      return nodeId;
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
  buttonSpec.callbacks.onActivate = [&]() { buttonClicks += 1; };

  PrimeStage::ToggleSpec toggleSpec;
  toggleSpec.trackStyle = 21u;
  toggleSpec.knobStyle = 22u;
  toggleSpec.size.preferredWidth = 60.0f;
  toggleSpec.size.preferredHeight = 24.0f;
  toggleSpec.enabled = false;
  toggleSpec.callbacks.onChange = [&](bool) { toggleChanges += 1; };

  PrimeStage::CheckboxSpec checkboxSpec;
  checkboxSpec.label = "Flag";
  checkboxSpec.boxStyle = 31u;
  checkboxSpec.checkStyle = 32u;
  checkboxSpec.size.preferredHeight = 24.0f;
  checkboxSpec.enabled = false;
  checkboxSpec.callbacks.onChange = [&](bool) { checkboxChanges += 1; };

  PrimeStage::TabsSpec tabsSpec;
  tabsSpec.labels = {"A", "B", "C"};
  tabsSpec.tabStyle = 41u;
  tabsSpec.activeTabStyle = 42u;
  tabsSpec.size.preferredHeight = 24.0f;
  tabsSpec.enabled = false;
  tabsSpec.callbacks.onSelect = [&](int) { tabChanges += 1; };

  PrimeStage::DropdownSpec dropdownSpec;
  dropdownSpec.options = {"One", "Two"};
  dropdownSpec.backgroundStyle = 51u;
  dropdownSpec.size.preferredWidth = 120.0f;
  dropdownSpec.size.preferredHeight = 24.0f;
  dropdownSpec.enabled = false;
  dropdownSpec.callbacks.onSelect = [&](int) { dropdownChanges += 1; };
  dropdownSpec.callbacks.onOpen = [&]() { dropdownChanges += 1; };

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
  spec.callbacks.onActivate = [&]() { clicks += 1; };

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

TEST_CASE("PrimeStage button legacy onClick callback remains supported") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 100.0f);

  PrimeStage::ButtonSpec spec;
  spec.size.preferredWidth = 80.0f;
  spec.size.preferredHeight = 30.0f;
  spec.label = "Legacy";
  int clicks = 0;
  spec.callbacks.onClick = [&]() { clicks += 1; };

  PrimeStage::UiNode button = root.createButton(spec);
  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 200.0f, 100.0f);
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
  CHECK(clicks == 1);

  focus.setFocus(frame, layout, button.nodeId());
  router.dispatch(makeKeyDownEvent(PrimeStage::KeyCode::Enter), frame, layout, &focus);
  CHECK(clicks == 2);
}

TEST_CASE("PrimeStage text field legacy onTextChanged callback remains supported") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 220.0f, 120.0f);

  PrimeStage::TextFieldState state;
  PrimeStage::TextFieldSpec spec;
  spec.state = &state;
  spec.size.preferredWidth = 160.0f;
  spec.size.preferredHeight = 28.0f;
  int legacyChanges = 0;
  std::string lastText;
  spec.callbacks.onTextChanged = [&](std::string_view text) {
    legacyChanges += 1;
    lastText = std::string(text);
  };

  PrimeStage::UiNode field = root.createTextField(spec);
  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 220.0f, 120.0f);
  PrimeFrame::LayoutOut const* out = layout.get(field.nodeId());
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

  PrimeFrame::Event textInput;
  textInput.type = PrimeFrame::EventType::TextInput;
  textInput.text = "A";
  router.dispatch(textInput, frame, layout, &focus);

  CHECK(state.text == "A");
  CHECK(legacyChanges == 1);
  CHECK(lastText == "A");
}

TEST_CASE("PrimeStage toggle checkbox slider and progress legacy aliases remain supported") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 320.0f, 220.0f);

  PrimeStage::StackSpec stackSpec;
  stackSpec.gap = 10.0f;
  stackSpec.size.stretchX = 1.0f;
  stackSpec.size.stretchY = 1.0f;
  PrimeStage::UiNode stack = root.createVerticalStack(stackSpec);

  PrimeStage::ToggleSpec toggleSpec;
  toggleSpec.size.preferredWidth = 60.0f;
  toggleSpec.size.preferredHeight = 24.0f;
  toggleSpec.trackStyle = 701u;
  toggleSpec.knobStyle = 702u;

  PrimeStage::CheckboxSpec checkboxSpec;
  checkboxSpec.label = "Enabled";
  checkboxSpec.boxStyle = 711u;
  checkboxSpec.checkStyle = 712u;

  PrimeStage::SliderSpec sliderSpec;
  sliderSpec.size.preferredWidth = 180.0f;
  sliderSpec.size.preferredHeight = 12.0f;
  sliderSpec.trackStyle = 721u;
  sliderSpec.fillStyle = 722u;
  sliderSpec.thumbStyle = 723u;

  PrimeStage::ProgressBarSpec progressSpec;
  progressSpec.size.preferredWidth = 180.0f;
  progressSpec.size.preferredHeight = 12.0f;
  progressSpec.trackStyle = 731u;
  progressSpec.fillStyle = 732u;

  std::vector<bool> toggleValues;
  std::vector<bool> checkboxValues;
  std::vector<float> sliderValues;
  std::vector<float> progressValues;
  toggleSpec.callbacks.onChanged = [&](bool value) { toggleValues.push_back(value); };
  checkboxSpec.callbacks.onChanged = [&](bool value) { checkboxValues.push_back(value); };
  sliderSpec.callbacks.onValueChanged = [&](float value) { sliderValues.push_back(value); };
  progressSpec.callbacks.onValueChanged = [&](float value) { progressValues.push_back(value); };

  PrimeStage::UiNode toggle = stack.createToggle(toggleSpec);
  PrimeStage::UiNode checkbox = stack.createCheckbox(checkboxSpec);
  PrimeStage::UiNode slider = stack.createSlider(sliderSpec);
  PrimeStage::UiNode progress = stack.createProgressBar(progressSpec);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 320.0f, 220.0f);
  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;

  auto clickCenter = [&](PrimeFrame::NodeId nodeId, int pointerId) {
    PrimeFrame::LayoutOut const* out = layout.get(nodeId);
    REQUIRE(out != nullptr);
    float x = out->absX + out->absW * 0.8f;
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

  clickCenter(toggle.nodeId(), 1);
  clickCenter(checkbox.nodeId(), 2);
  clickCenter(slider.nodeId(), 3);
  clickCenter(progress.nodeId(), 4);

  CHECK(toggleValues.size() == 1);
  CHECK(checkboxValues.size() == 1);
  CHECK(sliderValues.size() >= 1);
  CHECK(progressValues.size() >= 1);
}

TEST_CASE("PrimeStage semantic callbacks take precedence over legacy aliases for core widgets") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 320.0f, 260.0f);

  PrimeStage::StackSpec stackSpec;
  stackSpec.gap = 10.0f;
  stackSpec.size.stretchX = 1.0f;
  stackSpec.size.stretchY = 1.0f;
  PrimeStage::UiNode stack = root.createVerticalStack(stackSpec);

  PrimeStage::ButtonSpec buttonSpec;
  buttonSpec.label = "Apply";
  buttonSpec.size.preferredWidth = 120.0f;
  buttonSpec.size.preferredHeight = 28.0f;
  int buttonActivate = 0;
  int buttonClick = 0;
  buttonSpec.callbacks.onActivate = [&]() { buttonActivate += 1; };
  buttonSpec.callbacks.onClick = [&]() { buttonClick += 1; };

  PrimeStage::TextFieldState textState;
  PrimeStage::TextFieldSpec fieldSpec;
  fieldSpec.state = &textState;
  fieldSpec.size.preferredWidth = 180.0f;
  fieldSpec.size.preferredHeight = 28.0f;
  int fieldChange = 0;
  int fieldTextChanged = 0;
  fieldSpec.callbacks.onChange = [&](std::string_view) { fieldChange += 1; };
  fieldSpec.callbacks.onTextChanged = [&](std::string_view) { fieldTextChanged += 1; };

  PrimeStage::ToggleSpec toggleSpec;
  toggleSpec.size.preferredWidth = 60.0f;
  toggleSpec.size.preferredHeight = 24.0f;
  toggleSpec.trackStyle = 801u;
  toggleSpec.knobStyle = 802u;
  int toggleChange = 0;
  int toggleChanged = 0;
  toggleSpec.callbacks.onChange = [&](bool) { toggleChange += 1; };
  toggleSpec.callbacks.onChanged = [&](bool) { toggleChanged += 1; };

  PrimeStage::CheckboxSpec checkboxSpec;
  checkboxSpec.label = "Check";
  checkboxSpec.boxStyle = 811u;
  checkboxSpec.checkStyle = 812u;
  int checkboxChange = 0;
  int checkboxChanged = 0;
  checkboxSpec.callbacks.onChange = [&](bool) { checkboxChange += 1; };
  checkboxSpec.callbacks.onChanged = [&](bool) { checkboxChanged += 1; };

  PrimeStage::SliderSpec sliderSpec;
  sliderSpec.size.preferredWidth = 180.0f;
  sliderSpec.size.preferredHeight = 12.0f;
  sliderSpec.trackStyle = 821u;
  sliderSpec.fillStyle = 822u;
  sliderSpec.thumbStyle = 823u;
  int sliderChange = 0;
  int sliderValueChanged = 0;
  sliderSpec.callbacks.onChange = [&](float) { sliderChange += 1; };
  sliderSpec.callbacks.onValueChanged = [&](float) { sliderValueChanged += 1; };

  PrimeStage::ProgressBarSpec progressSpec;
  progressSpec.size.preferredWidth = 180.0f;
  progressSpec.size.preferredHeight = 12.0f;
  progressSpec.trackStyle = 831u;
  progressSpec.fillStyle = 832u;
  int progressChange = 0;
  int progressValueChanged = 0;
  progressSpec.callbacks.onChange = [&](float) { progressChange += 1; };
  progressSpec.callbacks.onValueChanged = [&](float) { progressValueChanged += 1; };

  PrimeStage::UiNode button = stack.createButton(buttonSpec);
  PrimeStage::UiNode field = stack.createTextField(fieldSpec);
  PrimeStage::UiNode toggle = stack.createToggle(toggleSpec);
  PrimeStage::UiNode checkbox = stack.createCheckbox(checkboxSpec);
  PrimeStage::UiNode slider = stack.createSlider(sliderSpec);
  PrimeStage::UiNode progress = stack.createProgressBar(progressSpec);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 320.0f, 260.0f);
  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;

  auto clickCenter = [&](PrimeFrame::NodeId nodeId, int pointerId, float xFactor = 0.5f) {
    PrimeFrame::LayoutOut const* out = layout.get(nodeId);
    REQUIRE(out != nullptr);
    float x = out->absX + out->absW * xFactor;
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
  clickCenter(field.nodeId(), 2);
  PrimeFrame::Event textInput;
  textInput.type = PrimeFrame::EventType::TextInput;
  textInput.text = "A";
  router.dispatch(textInput, frame, layout, &focus);
  clickCenter(toggle.nodeId(), 3);
  clickCenter(checkbox.nodeId(), 4);
  clickCenter(slider.nodeId(), 5, 0.8f);
  clickCenter(progress.nodeId(), 6, 0.8f);

  CHECK(buttonActivate == 1);
  CHECK(buttonClick == 0);
  CHECK(fieldChange == 1);
  CHECK(fieldTextChanged == 0);
  CHECK(toggleChange == 1);
  CHECK(toggleChanged == 0);
  CHECK(checkboxChange == 1);
  CHECK(checkboxChanged == 0);
  CHECK(sliderChange >= 1);
  CHECK(sliderValueChanged == 0);
  CHECK(progressChange >= 1);
  CHECK(progressValueChanged == 0);
}

TEST_CASE("PrimeStage semantic callbacks take precedence over legacy aliases for selection widgets") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 440.0f, 300.0f);

  PrimeStage::StackSpec stackSpec;
  stackSpec.gap = 10.0f;
  stackSpec.size.stretchX = 1.0f;
  stackSpec.size.stretchY = 1.0f;
  PrimeStage::UiNode stack = root.createVerticalStack(stackSpec);

  PrimeStage::TableSpec tableSpec;
  tableSpec.columns = {{"Name", 180.0f, 0u, 0u}};
  tableSpec.rows = {{"Alpha"}, {"Beta"}};
  tableSpec.headerInset = 0.0f;
  tableSpec.headerHeight = 0.0f;
  tableSpec.rowHeight = 24.0f;
  tableSpec.rowGap = 0.0f;
  tableSpec.size.preferredWidth = 220.0f;
  tableSpec.size.preferredHeight = 80.0f;
  int tableSelect = 0;
  int tableRowClicked = 0;
  tableSpec.callbacks.onSelect = [&](PrimeStage::TableRowInfo const&) { tableSelect += 1; };
  tableSpec.callbacks.onRowClicked = [&](PrimeStage::TableRowInfo const&) { tableRowClicked += 1; };

  PrimeStage::ListSpec listSpec;
  listSpec.items = {"One", "Two"};
  listSpec.rowHeight = 24.0f;
  listSpec.rowGap = 0.0f;
  listSpec.size.preferredWidth = 220.0f;
  listSpec.size.preferredHeight = 72.0f;
  int listSelect = 0;
  int listSelected = 0;
  listSpec.callbacks.onSelect = [&](PrimeStage::ListRowInfo const&) { listSelect += 1; };
  listSpec.callbacks.onSelected = [&](PrimeStage::ListRowInfo const&) { listSelected += 1; };

  PrimeStage::TreeViewSpec treeSpec;
  treeSpec.size.preferredWidth = 220.0f;
  treeSpec.size.preferredHeight = 72.0f;
  treeSpec.rowStartY = 0.0f;
  treeSpec.rowHeight = 20.0f;
  treeSpec.rowGap = 0.0f;
  treeSpec.rowStartX = 8.0f;
  treeSpec.rowWidthInset = 0.0f;
  treeSpec.rowStyle = 901u;
  treeSpec.rowAltStyle = 902u;
  treeSpec.selectionStyle = 903u;
  treeSpec.selectionAccentStyle = 904u;
  treeSpec.textStyle = 905u;
  treeSpec.selectedTextStyle = 906u;
  treeSpec.nodes = {PrimeStage::TreeNode{"Leaf"}};
  int treeSelect = 0;
  int treeSelectionChanged = 0;
  int treeActivate = 0;
  int treeActivated = 0;
  treeSpec.callbacks.onSelect = [&](PrimeStage::TreeViewRowInfo const&) { treeSelect += 1; };
  treeSpec.callbacks.onSelectionChanged = [&](PrimeStage::TreeViewRowInfo const&) { treeSelectionChanged += 1; };
  treeSpec.callbacks.onActivate = [&](PrimeStage::TreeViewRowInfo const&) { treeActivate += 1; };
  treeSpec.callbacks.onActivated = [&](PrimeStage::TreeViewRowInfo const&) { treeActivated += 1; };

  PrimeStage::UiNode table = stack.createTable(tableSpec);
  PrimeStage::UiNode list = stack.createList(listSpec);
  PrimeStage::UiNode tree = stack.createTreeView(treeSpec);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 440.0f, 300.0f);
  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;

  PrimeFrame::NodeId tableCallbackNode = findFirstNodeWithOnEventInSubtree(frame, table.nodeId());
  PrimeFrame::NodeId listCallbackNode = findFirstNodeWithOnEventInSubtree(frame, list.nodeId());
  REQUIRE(tableCallbackNode.isValid());
  REQUIRE(listCallbackNode.isValid());

  PrimeFrame::LayoutOut const* tableOut = layout.get(tableCallbackNode);
  PrimeFrame::LayoutOut const* listOut = layout.get(listCallbackNode);
  PrimeFrame::LayoutOut const* treeOut = layout.get(tree.nodeId());
  REQUIRE(tableOut != nullptr);
  REQUIRE(listOut != nullptr);
  REQUIRE(treeOut != nullptr);

  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown,
                                   1,
                                   tableOut->absX + tableOut->absW * 0.5f,
                                   tableOut->absY + tableSpec.rowHeight * 0.5f),
                  frame,
                  layout,
                  &focus);

  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown,
                                   2,
                                   listOut->absX + listOut->absW * 0.5f,
                                   listOut->absY + listSpec.rowHeight * 0.5f),
                  frame,
                  layout,
                  &focus);

  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown,
                                   3,
                                   treeOut->absX + treeSpec.rowStartX + 12.0f,
                                   treeOut->absY + treeSpec.rowHeight * 0.5f),
                  frame,
                  layout,
                  &focus);
  router.dispatch(makeKeyDownEvent(PrimeStage::KeyCode::Enter), frame, layout, &focus);

  CHECK(tableSelect == 1);
  CHECK(tableRowClicked == 0);
  CHECK(listSelect == 1);
  CHECK(listSelected == 0);
  CHECK(treeSelect == 1);
  CHECK(treeSelectionChanged == 0);
  CHECK(treeActivate == 1);
  CHECK(treeActivated == 0);
}

TEST_CASE("PrimeStage internal extension primitive seam supports typed callbacks and runtime gating") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 340.0f, 220.0f);

  int enabledEventCount = 0;
  int enabledFocusCount = 0;
  int enabledBlurCount = 0;

  PrimeStage::Internal::WidgetRuntimeContext enabledRuntime =
      PrimeStage::Internal::makeWidgetRuntimeContext(
          frame,
          root.nodeId(),
          true,
          true,
          true,
          4);

  PrimeStage::Internal::ExtensionPrimitiveSpec enabledSpec;
  enabledSpec.rect = {0.0f, 0.0f, 120.0f, 28.0f};
  enabledSpec.size.preferredWidth = 120.0f;
  enabledSpec.size.preferredHeight = 28.0f;
  enabledSpec.focusable = true;
  enabledSpec.hitTestVisible = true;
  enabledSpec.rectStyle = 941u;
  enabledSpec.callbacks.onEvent = [&](PrimeFrame::Event const& event) {
    if (event.type == PrimeFrame::EventType::PointerDown ||
        event.type == PrimeFrame::EventType::KeyDown) {
      enabledEventCount += 1;
      return true;
    }
    return false;
  };
  enabledSpec.callbacks.onFocus = [&]() { enabledFocusCount += 1; };
  enabledSpec.callbacks.onBlur = [&]() { enabledBlurCount += 1; };

  PrimeStage::UiNode enabledExtension =
      PrimeStage::Internal::createExtensionPrimitive(enabledRuntime, enabledSpec);

  int disabledEventCount = 0;
  PrimeStage::Internal::WidgetRuntimeContext disabledRuntime =
      PrimeStage::Internal::makeWidgetRuntimeContext(
          frame,
          root.nodeId(),
          true,
          false,
          true,
          9);
  PrimeStage::Internal::ExtensionPrimitiveSpec disabledSpec;
  disabledSpec.rect = {0.0f, 0.0f, 120.0f, 28.0f};
  disabledSpec.size.preferredWidth = 120.0f;
  disabledSpec.size.preferredHeight = 28.0f;
  disabledSpec.focusable = true;
  disabledSpec.hitTestVisible = true;
  disabledSpec.rectStyle = 942u;
  disabledSpec.callbacks.onEvent = [&](PrimeFrame::Event const& event) {
    if (event.type == PrimeFrame::EventType::PointerDown) {
      disabledEventCount += 1;
      return true;
    }
    return false;
  };
  PrimeStage::UiNode disabledExtension =
      PrimeStage::Internal::createExtensionPrimitive(disabledRuntime, disabledSpec);

  int hiddenEventCount = 0;
  PrimeStage::Internal::WidgetRuntimeContext hiddenRuntime =
      PrimeStage::Internal::makeWidgetRuntimeContext(
          frame,
          root.nodeId(),
          true,
          true,
          false,
          12);
  PrimeStage::Internal::ExtensionPrimitiveSpec hiddenSpec;
  hiddenSpec.rect = {0.0f, 0.0f, 120.0f, 28.0f};
  hiddenSpec.size.preferredWidth = 120.0f;
  hiddenSpec.size.preferredHeight = 28.0f;
  hiddenSpec.focusable = true;
  hiddenSpec.hitTestVisible = true;
  hiddenSpec.rectStyle = 943u;
  hiddenSpec.callbacks.onEvent = [&](PrimeFrame::Event const& event) {
    if (event.type == PrimeFrame::EventType::PointerDown) {
      hiddenEventCount += 1;
      return true;
    }
    return false;
  };
  PrimeStage::UiNode hiddenExtension =
      PrimeStage::Internal::createExtensionPrimitive(hiddenRuntime, hiddenSpec);

  PrimeFrame::Node const* enabledNode = frame.getNode(enabledExtension.nodeId());
  PrimeFrame::Node const* disabledNode = frame.getNode(disabledExtension.nodeId());
  PrimeFrame::Node const* hiddenNode = frame.getNode(hiddenExtension.nodeId());
  REQUIRE(enabledNode != nullptr);
  REQUIRE(disabledNode != nullptr);
  REQUIRE(hiddenNode != nullptr);
  CHECK(enabledNode->focusable);
  CHECK(enabledNode->hitTestVisible);
  CHECK(enabledNode->tabIndex == 4);
  CHECK_FALSE(disabledNode->focusable);
  CHECK_FALSE(disabledNode->hitTestVisible);
  CHECK(disabledNode->tabIndex == -1);
  CHECK_FALSE(hiddenNode->focusable);
  CHECK_FALSE(hiddenNode->hitTestVisible);
  CHECK(hiddenNode->tabIndex == -1);

  PrimeFrame::Primitive const* enabledRect =
      findRectPrimitiveByTokenInSubtree(frame, enabledExtension.nodeId(), 941u);
  PrimeFrame::Primitive const* disabledRect =
      findRectPrimitiveByTokenInSubtree(frame, disabledExtension.nodeId(), 942u);
  PrimeFrame::Primitive const* hiddenRect =
      findRectPrimitiveByTokenInSubtree(frame, hiddenExtension.nodeId(), 943u);
  REQUIRE(enabledRect != nullptr);
  REQUIRE(disabledRect != nullptr);
  REQUIRE(hiddenRect != nullptr);

  REQUIRE(enabledNode->callbacks != PrimeFrame::InvalidCallbackId);
  PrimeFrame::Callback const* enabledCallback = frame.getCallback(enabledNode->callbacks);
  REQUIRE(enabledCallback != nullptr);
  REQUIRE(enabledCallback->onEvent);
  REQUIRE(enabledCallback->onFocus);
  REQUIRE(enabledCallback->onBlur);

  PrimeFrame::Event pointerDown;
  pointerDown.type = PrimeFrame::EventType::PointerDown;
  pointerDown.pointerId = 1;
  CHECK(enabledCallback->onEvent(pointerDown));
  CHECK(enabledEventCount == 1);

  PrimeFrame::Event keyDown = makeKeyDownEvent(PrimeStage::KeyCode::Enter);
  CHECK(enabledCallback->onEvent(keyDown));
  CHECK(enabledEventCount == 2);

  enabledCallback->onFocus();
  enabledCallback->onBlur();
  CHECK(enabledFocusCount == 1);
  CHECK(enabledBlurCount == 1);

  if (disabledNode->callbacks != PrimeFrame::InvalidCallbackId) {
    PrimeFrame::Callback const* disabledCallback = frame.getCallback(disabledNode->callbacks);
    REQUIRE(disabledCallback != nullptr);
    CHECK_FALSE(disabledCallback->onEvent);
    CHECK_FALSE(disabledCallback->onFocus);
    CHECK_FALSE(disabledCallback->onBlur);
  }
  if (hiddenNode->callbacks != PrimeFrame::InvalidCallbackId) {
    PrimeFrame::Callback const* hiddenCallback = frame.getCallback(hiddenNode->callbacks);
    REQUIRE(hiddenCallback != nullptr);
    CHECK_FALSE(hiddenCallback->onEvent);
    CHECK_FALSE(hiddenCallback->onFocus);
    CHECK_FALSE(hiddenCallback->onBlur);
  }
  CHECK(disabledEventCount == 0);
  CHECK(hiddenEventCount == 0);
}

TEST_CASE("PrimeStage internal extension primitive seam routes pointer and focus callbacks through event router") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 360.0f, 220.0f);

  int firstPointerCount = 0;
  int firstKeyCount = 0;
  int firstFocusCount = 0;
  int firstBlurCount = 0;

  PrimeStage::Internal::WidgetRuntimeContext firstRuntime =
      PrimeStage::Internal::makeWidgetRuntimeContext(
          frame,
          root.nodeId(),
          true,
          true,
          true,
          5);
  PrimeStage::Internal::ExtensionPrimitiveSpec firstSpec;
  firstSpec.rect = {16.0f, 16.0f, 120.0f, 28.0f};
  firstSpec.size.preferredWidth = 120.0f;
  firstSpec.size.preferredHeight = 28.0f;
  firstSpec.focusable = true;
  firstSpec.hitTestVisible = true;
  firstSpec.rectStyle = 951u;
  firstSpec.callbacks.onEvent = [&](PrimeFrame::Event const& event) {
    if (event.type == PrimeFrame::EventType::PointerDown) {
      firstPointerCount += 1;
      return true;
    }
    if (event.type == PrimeFrame::EventType::KeyDown &&
        event.key == PrimeStage::keyCodeInt(PrimeStage::KeyCode::Enter)) {
      firstKeyCount += 1;
      return true;
    }
    return false;
  };
  firstSpec.callbacks.onFocus = [&]() { firstFocusCount += 1; };
  firstSpec.callbacks.onBlur = [&]() { firstBlurCount += 1; };
  PrimeStage::UiNode first = PrimeStage::Internal::createExtensionPrimitive(firstRuntime, firstSpec);

  int secondPointerCount = 0;
  int secondKeyCount = 0;
  int secondFocusCount = 0;
  int secondBlurCount = 0;

  PrimeStage::Internal::WidgetRuntimeContext secondRuntime =
      PrimeStage::Internal::makeWidgetRuntimeContext(
          frame,
          root.nodeId(),
          true,
          true,
          true,
          6);
  PrimeStage::Internal::ExtensionPrimitiveSpec secondSpec;
  secondSpec.rect = {16.0f, 64.0f, 120.0f, 28.0f};
  secondSpec.size.preferredWidth = 120.0f;
  secondSpec.size.preferredHeight = 28.0f;
  secondSpec.focusable = true;
  secondSpec.hitTestVisible = true;
  secondSpec.rectStyle = 952u;
  secondSpec.callbacks.onEvent = [&](PrimeFrame::Event const& event) {
    if (event.type == PrimeFrame::EventType::PointerDown) {
      secondPointerCount += 1;
      return true;
    }
    if (event.type == PrimeFrame::EventType::KeyDown &&
        event.key == PrimeStage::keyCodeInt(PrimeStage::KeyCode::Enter)) {
      secondKeyCount += 1;
      return true;
    }
    return false;
  };
  secondSpec.callbacks.onFocus = [&]() { secondFocusCount += 1; };
  secondSpec.callbacks.onBlur = [&]() { secondBlurCount += 1; };
  PrimeStage::UiNode second =
      PrimeStage::Internal::createExtensionPrimitive(secondRuntime, secondSpec);

  int disabledPointerCount = 0;
  PrimeStage::Internal::WidgetRuntimeContext disabledRuntime =
      PrimeStage::Internal::makeWidgetRuntimeContext(
          frame,
          root.nodeId(),
          true,
          false,
          true,
          7);
  PrimeStage::Internal::ExtensionPrimitiveSpec disabledSpec;
  disabledSpec.rect = {16.0f, 112.0f, 120.0f, 28.0f};
  disabledSpec.size.preferredWidth = 120.0f;
  disabledSpec.size.preferredHeight = 28.0f;
  disabledSpec.focusable = true;
  disabledSpec.hitTestVisible = true;
  disabledSpec.rectStyle = 953u;
  disabledSpec.callbacks.onEvent = [&](PrimeFrame::Event const& event) {
    if (event.type == PrimeFrame::EventType::PointerDown) {
      disabledPointerCount += 1;
      return true;
    }
    return false;
  };
  PrimeStage::UiNode disabled =
      PrimeStage::Internal::createExtensionPrimitive(disabledRuntime, disabledSpec);

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

  clickCenter(first.nodeId(), 1);
  CHECK(focus.focusedNode() == first.nodeId());
  CHECK(firstPointerCount == 1);
  CHECK(firstFocusCount == 1);
  CHECK(firstBlurCount == 0);

  router.dispatch(makeKeyDownEvent(PrimeStage::KeyCode::Enter), frame, layout, &focus);
  CHECK(firstKeyCount == 1);

  clickCenter(second.nodeId(), 2);
  CHECK(focus.focusedNode() == second.nodeId());
  CHECK(secondPointerCount == 1);
  CHECK(secondFocusCount == 1);
  CHECK(firstBlurCount == 1);

  router.dispatch(makeKeyDownEvent(PrimeStage::KeyCode::Enter), frame, layout, &focus);
  CHECK(secondKeyCount == 1);

  clickCenter(disabled.nodeId(), 3);
  CHECK(disabledPointerCount == 0);
  CHECK(focus.focusedNode() != disabled.nodeId());

  clickCenter(first.nodeId(), 4);
  CHECK(focus.focusedNode() == first.nodeId());
  CHECK(firstPointerCount == 2);
  CHECK(firstFocusCount == 2);
  CHECK(secondBlurCount == 1);

  PrimeFrame::Primitive const* firstRect =
      findRectPrimitiveByTokenInSubtree(frame, first.nodeId(), 951u);
  PrimeFrame::Primitive const* secondRect =
      findRectPrimitiveByTokenInSubtree(frame, second.nodeId(), 952u);
  PrimeFrame::Primitive const* disabledRect =
      findRectPrimitiveByTokenInSubtree(frame, disabled.nodeId(), 953u);
  REQUIRE(firstRect != nullptr);
  REQUIRE(secondRect != nullptr);
  REQUIRE(disabledRect != nullptr);
}

TEST_CASE("PrimeStage internal extension primitive seam composes appended callbacks predictably") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 360.0f, 220.0f);

  int extensionPointerCount = 0;
  int extensionKeyCount = 0;
  int appendedPointerCount = 0;
  int appendedKeyCount = 0;
  std::vector<std::string> focusTrace;

  PrimeStage::Internal::WidgetRuntimeContext runtime =
      PrimeStage::Internal::makeWidgetRuntimeContext(
          frame,
          root.nodeId(),
          true,
          true,
          true,
          3);
  PrimeStage::Internal::ExtensionPrimitiveSpec spec;
  spec.rect = {20.0f, 20.0f, 140.0f, 30.0f};
  spec.size.preferredWidth = 140.0f;
  spec.size.preferredHeight = 30.0f;
  spec.focusable = true;
  spec.hitTestVisible = true;
  spec.rectStyle = 961u;
  spec.callbacks.onEvent = [&](PrimeFrame::Event const& event) {
    if (event.type == PrimeFrame::EventType::PointerDown) {
      extensionPointerCount += 1;
      return false;
    }
    if (event.type == PrimeFrame::EventType::KeyDown &&
        event.key == PrimeStage::keyCodeInt(PrimeStage::KeyCode::Enter)) {
      extensionKeyCount += 1;
      return false;
    }
    return false;
  };
  spec.callbacks.onFocus = [&]() { focusTrace.push_back("extensionFocus"); };
  spec.callbacks.onBlur = [&]() { focusTrace.push_back("extensionBlur"); };
  PrimeStage::UiNode extension = PrimeStage::Internal::createExtensionPrimitive(runtime, spec);

  CHECK(PrimeStage::LowLevel::appendNodeOnEvent(
      frame,
      extension.nodeId(),
      [&](PrimeFrame::Event const& event) {
        if (event.type == PrimeFrame::EventType::PointerDown) {
          appendedPointerCount += 1;
          return false;
        }
        if (event.type == PrimeFrame::EventType::KeyDown &&
            event.key == PrimeStage::keyCodeInt(PrimeStage::KeyCode::Enter)) {
          appendedKeyCount += 1;
          return true;
        }
        return false;
      }));
  CHECK(PrimeStage::LowLevel::appendNodeOnFocus(
      frame, extension.nodeId(), [&]() { focusTrace.push_back("appendedFocus"); }));
  CHECK(PrimeStage::LowLevel::appendNodeOnBlur(
      frame, extension.nodeId(), [&]() { focusTrace.push_back("appendedBlur"); }));

  PrimeStage::Internal::WidgetRuntimeContext blurRuntime =
      PrimeStage::Internal::makeWidgetRuntimeContext(
          frame,
          root.nodeId(),
          true,
          true,
          true,
          4);
  PrimeStage::Internal::ExtensionPrimitiveSpec blurSpec;
  blurSpec.rect = {20.0f, 70.0f, 140.0f, 30.0f};
  blurSpec.size.preferredWidth = 140.0f;
  blurSpec.size.preferredHeight = 30.0f;
  blurSpec.focusable = true;
  blurSpec.hitTestVisible = true;
  blurSpec.rectStyle = 962u;
  PrimeStage::UiNode blurTarget =
      PrimeStage::Internal::createExtensionPrimitive(blurRuntime, blurSpec);

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

  clickCenter(extension.nodeId(), 1);
  CHECK(focus.focusedNode() == extension.nodeId());
  CHECK(appendedPointerCount == 1);
  CHECK(extensionPointerCount == 1);
  REQUIRE(focusTrace.size() >= 2u);
  CHECK(focusTrace[0] == "extensionFocus");
  CHECK(focusTrace[1] == "appendedFocus");

  router.dispatch(makeKeyDownEvent(PrimeStage::KeyCode::Enter), frame, layout, &focus);
  CHECK(appendedKeyCount == 1);
  CHECK(extensionKeyCount == 0);

  clickCenter(blurTarget.nodeId(), 2);
  CHECK(focus.focusedNode() == blurTarget.nodeId());
  REQUIRE(focusTrace.size() >= 4u);
  CHECK(focusTrace[2] == "extensionBlur");
  CHECK(focusTrace[3] == "appendedBlur");

  PrimeFrame::Primitive const* extensionRect =
      findRectPrimitiveByTokenInSubtree(frame, extension.nodeId(), 961u);
  PrimeFrame::Primitive const* blurRect =
      findRectPrimitiveByTokenInSubtree(frame, blurTarget.nodeId(), 962u);
  REQUIRE(extensionRect != nullptr);
  REQUIRE(blurRect != nullptr);
}

TEST_CASE("PrimeStage internal extension primitive seam suppresses routed reentrant callback recursion") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 360.0f, 220.0f);

  PrimeFrame::NodeId guardedId{};
  bool nestedEventHandled = true;
  int guardedEventCalls = 0;
  int guardedFocusCalls = 0;
  int guardedBlurCalls = 0;

  PrimeStage::Internal::WidgetRuntimeContext guardedRuntime =
      PrimeStage::Internal::makeWidgetRuntimeContext(
          frame,
          root.nodeId(),
          true,
          true,
          true,
          2);
  PrimeStage::Internal::ExtensionPrimitiveSpec guardedSpec;
  guardedSpec.rect = {20.0f, 20.0f, 140.0f, 30.0f};
  guardedSpec.size.preferredWidth = 140.0f;
  guardedSpec.size.preferredHeight = 30.0f;
  guardedSpec.focusable = true;
  guardedSpec.hitTestVisible = true;
  guardedSpec.rectStyle = 971u;
  guardedSpec.callbacks.onEvent = [&](PrimeFrame::Event const& event) {
    if (event.type != PrimeFrame::EventType::PointerDown) {
      return false;
    }
    guardedEventCalls += 1;
    if (guardedEventCalls == 1) {
      PrimeFrame::Node const* node = frame.getNode(guardedId);
      REQUIRE(node != nullptr);
      PrimeFrame::Callback const* callback = frame.getCallback(node->callbacks);
      REQUIRE(callback != nullptr);
      REQUIRE(callback->onEvent);
      nestedEventHandled = callback->onEvent(event);
    }
    return true;
  };
  guardedSpec.callbacks.onFocus = [&]() {
    guardedFocusCalls += 1;
    if (guardedFocusCalls == 1) {
      PrimeFrame::Node const* node = frame.getNode(guardedId);
      REQUIRE(node != nullptr);
      PrimeFrame::Callback const* callback = frame.getCallback(node->callbacks);
      REQUIRE(callback != nullptr);
      REQUIRE(callback->onFocus);
      callback->onFocus();
    }
  };
  guardedSpec.callbacks.onBlur = [&]() {
    guardedBlurCalls += 1;
    if (guardedBlurCalls == 1) {
      PrimeFrame::Node const* node = frame.getNode(guardedId);
      REQUIRE(node != nullptr);
      PrimeFrame::Callback const* callback = frame.getCallback(node->callbacks);
      REQUIRE(callback != nullptr);
      REQUIRE(callback->onBlur);
      callback->onBlur();
    }
  };
  PrimeStage::UiNode guarded =
      PrimeStage::Internal::createExtensionPrimitive(guardedRuntime, guardedSpec);
  guardedId = guarded.nodeId();

  PrimeStage::Internal::WidgetRuntimeContext blurRuntime =
      PrimeStage::Internal::makeWidgetRuntimeContext(
          frame,
          root.nodeId(),
          true,
          true,
          true,
          3);
  PrimeStage::Internal::ExtensionPrimitiveSpec blurSpec;
  blurSpec.rect = {20.0f, 70.0f, 140.0f, 30.0f};
  blurSpec.size.preferredWidth = 140.0f;
  blurSpec.size.preferredHeight = 30.0f;
  blurSpec.focusable = true;
  blurSpec.hitTestVisible = true;
  blurSpec.rectStyle = 972u;
  PrimeStage::UiNode blurTarget =
      PrimeStage::Internal::createExtensionPrimitive(blurRuntime, blurSpec);

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

  clickCenter(guarded.nodeId(), 1);
  CHECK(focus.focusedNode() == guarded.nodeId());
  CHECK(guardedEventCalls == 1);
  CHECK_FALSE(nestedEventHandled);
  CHECK(guardedFocusCalls == 1);

  clickCenter(blurTarget.nodeId(), 2);
  CHECK(focus.focusedNode() == blurTarget.nodeId());
  CHECK(guardedBlurCalls == 1);

  PrimeFrame::Primitive const* guardedRect =
      findRectPrimitiveByTokenInSubtree(frame, guarded.nodeId(), 971u);
  PrimeFrame::Primitive const* blurRect =
      findRectPrimitiveByTokenInSubtree(frame, blurTarget.nodeId(), 972u);
  REQUIRE(guardedRect != nullptr);
  REQUIRE(blurRect != nullptr);
}

TEST_CASE("PrimeStage internal extension primitive seam restores callbacks after NodeCallbackHandle override") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 320.0f, 200.0f);

  int extensionEventCount = 0;
  int extensionFocusCount = 0;
  int extensionBlurCount = 0;

  PrimeStage::Internal::WidgetRuntimeContext runtime =
      PrimeStage::Internal::makeWidgetRuntimeContext(
          frame,
          root.nodeId(),
          true,
          true,
          true,
          2);
  PrimeStage::Internal::ExtensionPrimitiveSpec spec;
  spec.rect = {16.0f, 16.0f, 120.0f, 30.0f};
  spec.size.preferredWidth = 120.0f;
  spec.size.preferredHeight = 30.0f;
  spec.focusable = true;
  spec.hitTestVisible = true;
  spec.rectStyle = 981u;
  spec.callbacks.onEvent = [&](PrimeFrame::Event const& event) {
    if (event.type == PrimeFrame::EventType::PointerDown) {
      extensionEventCount += 1;
      return true;
    }
    return false;
  };
  spec.callbacks.onFocus = [&]() { extensionFocusCount += 1; };
  spec.callbacks.onBlur = [&]() { extensionBlurCount += 1; };
  PrimeStage::UiNode extension =
      PrimeStage::Internal::createExtensionPrimitive(runtime, spec);

  PrimeFrame::Node* extensionNode = frame.getNode(extension.nodeId());
  REQUIRE(extensionNode != nullptr);
  PrimeFrame::CallbackId originalCallbackId = extensionNode->callbacks;
  REQUIRE(originalCallbackId != PrimeFrame::InvalidCallbackId);

  int overrideEventCount = 0;
  int overrideFocusCount = 0;
  int overrideBlurCount = 0;
  PrimeStage::LowLevel::NodeCallbackTable overrideTable;
  overrideTable.onEvent = [&](PrimeFrame::Event const& event) {
    if (event.type == PrimeFrame::EventType::PointerDown) {
      overrideEventCount += 1;
      return true;
    }
    return false;
  };
  overrideTable.onFocus = [&]() { overrideFocusCount += 1; };
  overrideTable.onBlur = [&]() { overrideBlurCount += 1; };

  PrimeStage::LowLevel::NodeCallbackHandle handle(frame, extension.nodeId(), std::move(overrideTable));
  CHECK(handle.active());
  REQUIRE(extensionNode->callbacks != PrimeFrame::InvalidCallbackId);
  CHECK(extensionNode->callbacks != originalCallbackId);

  PrimeFrame::Callback const* overrideCallbacks = frame.getCallback(extensionNode->callbacks);
  REQUIRE(overrideCallbacks != nullptr);
  REQUIRE(overrideCallbacks->onEvent);
  REQUIRE(overrideCallbacks->onFocus);
  REQUIRE(overrideCallbacks->onBlur);

  PrimeFrame::Event pointerDown;
  pointerDown.type = PrimeFrame::EventType::PointerDown;
  pointerDown.pointerId = 1;
  CHECK(overrideCallbacks->onEvent(pointerDown));
  overrideCallbacks->onFocus();
  overrideCallbacks->onBlur();

  CHECK(overrideEventCount == 1);
  CHECK(overrideFocusCount == 1);
  CHECK(overrideBlurCount == 1);
  CHECK(extensionEventCount == 0);
  CHECK(extensionFocusCount == 0);
  CHECK(extensionBlurCount == 0);

  handle.reset();
  CHECK_FALSE(handle.active());
  CHECK(extensionNode->callbacks == originalCallbackId);

  PrimeFrame::Callback const* restoredCallbacks = frame.getCallback(extensionNode->callbacks);
  REQUIRE(restoredCallbacks != nullptr);
  REQUIRE(restoredCallbacks->onEvent);
  REQUIRE(restoredCallbacks->onFocus);
  REQUIRE(restoredCallbacks->onBlur);

  CHECK(restoredCallbacks->onEvent(pointerDown));
  restoredCallbacks->onFocus();
  restoredCallbacks->onBlur();

  CHECK(extensionEventCount == 1);
  CHECK(extensionFocusCount == 1);
  CHECK(extensionBlurCount == 1);
  CHECK(overrideEventCount == 1);
  CHECK(overrideFocusCount == 1);
  CHECK(overrideBlurCount == 1);

  PrimeFrame::Primitive const* extensionRect =
      findRectPrimitiveByTokenInSubtree(frame, extension.nodeId(), 981u);
  REQUIRE(extensionRect != nullptr);
}

TEST_CASE("PrimeStage internal extension primitive seam tolerates NodeCallbackHandle move and destroyed node reset") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 320.0f, 200.0f);

  int extensionEventCount = 0;
  PrimeStage::Internal::WidgetRuntimeContext runtime =
      PrimeStage::Internal::makeWidgetRuntimeContext(
          frame,
          root.nodeId(),
          true,
          true,
          true,
          2);
  PrimeStage::Internal::ExtensionPrimitiveSpec spec;
  spec.rect = {16.0f, 16.0f, 120.0f, 30.0f};
  spec.size.preferredWidth = 120.0f;
  spec.size.preferredHeight = 30.0f;
  spec.focusable = true;
  spec.hitTestVisible = true;
  spec.rectStyle = 982u;
  spec.callbacks.onEvent = [&](PrimeFrame::Event const& event) {
    if (event.type == PrimeFrame::EventType::PointerDown) {
      extensionEventCount += 1;
      return true;
    }
    return false;
  };
  PrimeStage::UiNode extension =
      PrimeStage::Internal::createExtensionPrimitive(runtime, spec);

  int overrideEventCount = 0;
  PrimeStage::LowLevel::NodeCallbackTable overrideTable;
  overrideTable.onEvent = [&](PrimeFrame::Event const& event) {
    if (event.type == PrimeFrame::EventType::PointerDown) {
      overrideEventCount += 1;
      return true;
    }
    return false;
  };

  PrimeStage::LowLevel::NodeCallbackHandle first(frame, extension.nodeId(), std::move(overrideTable));
  CHECK(first.active());
  PrimeStage::LowLevel::NodeCallbackHandle second(std::move(first));
  CHECK_FALSE(first.active());
  CHECK(second.active());

  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 320.0f, 200.0f);
  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;
  PrimeFrame::LayoutOut const* out = layout.get(extension.nodeId());
  REQUIRE(out != nullptr);
  float x = out->absX + out->absW * 0.5f;
  float y = out->absY + out->absH * 0.5f;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 1, x, y),
                  frame,
                  layout,
                  &focus);

  CHECK(overrideEventCount == 1);
  CHECK(extensionEventCount == 0);

  PrimeFrame::Primitive const* extensionRect =
      findRectPrimitiveByTokenInSubtree(frame, extension.nodeId(), 982u);
  REQUIRE(extensionRect != nullptr);

  REQUIRE(frame.destroyNode(extension.nodeId()));
  second.reset();
  CHECK_FALSE(second.active());
}

TEST_CASE("PrimeStage internal extension primitive seam respects visibility toggles for routed callbacks") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 320.0f, 200.0f);

  int pointerCount = 0;
  PrimeStage::Internal::WidgetRuntimeContext runtime =
      PrimeStage::Internal::makeWidgetRuntimeContext(
          frame,
          root.nodeId(),
          true,
          true,
          true,
          2);
  PrimeStage::Internal::ExtensionPrimitiveSpec spec;
  spec.rect = {16.0f, 16.0f, 120.0f, 30.0f};
  spec.size.preferredWidth = 120.0f;
  spec.size.preferredHeight = 30.0f;
  spec.focusable = true;
  spec.hitTestVisible = true;
  spec.rectStyle = 983u;
  spec.callbacks.onEvent = [&](PrimeFrame::Event const& event) {
    if (event.type == PrimeFrame::EventType::PointerDown) {
      pointerCount += 1;
      return true;
    }
    return false;
  };
  PrimeStage::UiNode extension =
      PrimeStage::Internal::createExtensionPrimitive(runtime, spec);

  PrimeFrame::Node* extensionNode = frame.getNode(extension.nodeId());
  REQUIRE(extensionNode != nullptr);
  PrimeFrame::CallbackId callbackId = extensionNode->callbacks;
  REQUIRE(callbackId != PrimeFrame::InvalidCallbackId);

  PrimeFrame::LayoutOutput visibleLayout = layoutFrame(frame, 320.0f, 200.0f);
  PrimeFrame::LayoutOut const* visibleOut = visibleLayout.get(extension.nodeId());
  REQUIRE(visibleOut != nullptr);
  float hiddenProbeX = visibleOut->absX + visibleOut->absW * 0.5f;
  float hiddenProbeY = visibleOut->absY + visibleOut->absH * 0.5f;

  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 1, hiddenProbeX, hiddenProbeY),
                  frame,
                  visibleLayout,
                  &focus);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 1, hiddenProbeX, hiddenProbeY),
                  frame,
                  visibleLayout,
                  &focus);
  CHECK(pointerCount == 1);

  extension.setVisible(false);
  CHECK_FALSE(extensionNode->visible);
  CHECK(extensionNode->callbacks == callbackId);
  PrimeFrame::LayoutOutput hiddenLayout = layoutFrame(frame, 320.0f, 200.0f);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 2, hiddenProbeX, hiddenProbeY),
                  frame,
                  hiddenLayout,
                  &focus);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 2, hiddenProbeX, hiddenProbeY),
                  frame,
                  hiddenLayout,
                  &focus);
  CHECK(pointerCount == 1);

  extension.setVisible(true);
  CHECK(extensionNode->visible);
  CHECK(extensionNode->callbacks == callbackId);
  PrimeFrame::LayoutOutput reshownLayout = layoutFrame(frame, 320.0f, 200.0f);
  PrimeFrame::LayoutOut const* reshownOut = reshownLayout.get(extension.nodeId());
  REQUIRE(reshownOut != nullptr);
  float reshownX = reshownOut->absX + reshownOut->absW * 0.5f;
  float reshownY = reshownOut->absY + reshownOut->absH * 0.5f;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 3, reshownX, reshownY),
                  frame,
                  reshownLayout,
                  &focus);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 3, reshownX, reshownY),
                  frame,
                  reshownLayout,
                  &focus);
  CHECK(pointerCount == 2);

  PrimeFrame::Primitive const* extensionRect =
      findRectPrimitiveByTokenInSubtree(frame, extension.nodeId(), 983u);
  REQUIRE(extensionRect != nullptr);
}

TEST_CASE("PrimeStage internal extension primitive seam respects hit-test toggles for routed callbacks") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 320.0f, 200.0f);

  int pointerCount = 0;
  PrimeStage::Internal::WidgetRuntimeContext runtime =
      PrimeStage::Internal::makeWidgetRuntimeContext(
          frame,
          root.nodeId(),
          true,
          true,
          true,
          2);
  PrimeStage::Internal::ExtensionPrimitiveSpec spec;
  spec.rect = {16.0f, 16.0f, 120.0f, 30.0f};
  spec.size.preferredWidth = 120.0f;
  spec.size.preferredHeight = 30.0f;
  spec.focusable = true;
  spec.hitTestVisible = true;
  spec.rectStyle = 984u;
  spec.callbacks.onEvent = [&](PrimeFrame::Event const& event) {
    if (event.type == PrimeFrame::EventType::PointerDown) {
      pointerCount += 1;
      return true;
    }
    return false;
  };
  PrimeStage::UiNode extension =
      PrimeStage::Internal::createExtensionPrimitive(runtime, spec);

  PrimeFrame::Node* extensionNode = frame.getNode(extension.nodeId());
  REQUIRE(extensionNode != nullptr);
  PrimeFrame::CallbackId callbackId = extensionNode->callbacks;
  REQUIRE(callbackId != PrimeFrame::InvalidCallbackId);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 320.0f, 200.0f);
  PrimeFrame::LayoutOut const* out = layout.get(extension.nodeId());
  REQUIRE(out != nullptr);
  float x = out->absX + out->absW * 0.5f;
  float y = out->absY + out->absH * 0.5f;
  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;

  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 1, x, y),
                  frame,
                  layout,
                  &focus);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 1, x, y),
                  frame,
                  layout,
                  &focus);
  CHECK(pointerCount == 1);

  extension.setHitTestVisible(false);
  CHECK_FALSE(extensionNode->hitTestVisible);
  CHECK(extensionNode->callbacks == callbackId);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 2, x, y),
                  frame,
                  layout,
                  &focus);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 2, x, y),
                  frame,
                  layout,
                  &focus);
  CHECK(pointerCount == 1);

  extension.setHitTestVisible(true);
  CHECK(extensionNode->hitTestVisible);
  CHECK(extensionNode->callbacks == callbackId);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 3, x, y),
                  frame,
                  layout,
                  &focus);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 3, x, y),
                  frame,
                  layout,
                  &focus);
  CHECK(pointerCount == 2);

  PrimeFrame::Primitive const* extensionRect =
      findRectPrimitiveByTokenInSubtree(frame, extension.nodeId(), 984u);
  REQUIRE(extensionRect != nullptr);
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
  spec.callbacks.onChange = [&](float value) { values.push_back(value); };

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
  spec.callbacks.onActivate = [&]() { clicks += 1; };
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
  spec.callbacks.onActivate = [&]() { clicks += 1; };

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

TEST_CASE("PrimeStage toggle and checkbox emit onChange for pointer and keyboard") {
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
  toggleSpec.callbacks.onChange = [&](bool on) { toggleValues.push_back(on); };
  checkboxSpec.callbacks.onChange = [&](bool checked) { checkboxValues.push_back(checked); };

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
  toggleSpec.callbacks.onChange = [&](bool on) { toggleValues.push_back(on); };
  checkboxSpec.callbacks.onChange = [&](bool checked) { checkboxValues.push_back(checked); };

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
  toggleSpec.callbacks.onChange = [&](bool on) { toggleValues.push_back(on); };
  checkboxSpec.callbacks.onChange = [&](bool checked) { checkboxValues.push_back(checked); };

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
  buttonSpec.callbacks.onActivate = [&]() { buttonActivations += 1; };

  PrimeStage::ToggleSpec toggleSpec;
  toggleSpec.tabIndex = 20;
  toggleSpec.size.preferredWidth = 56.0f;
  toggleSpec.size.preferredHeight = 24.0f;
  toggleSpec.trackStyle = 501u;
  toggleSpec.knobStyle = 502u;
  toggleSpec.callbacks.onChange = [&](bool) { toggleActivations += 1; };

  PrimeStage::CheckboxSpec checkboxSpec;
  checkboxSpec.label = "Enable";
  checkboxSpec.tabIndex = 30;
  checkboxSpec.boxStyle = 511u;
  checkboxSpec.checkStyle = 512u;
  checkboxSpec.callbacks.onChange = [&](bool) { checkboxActivations += 1; };

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
  spec.callbacks.onChange = [&](float value) { values.push_back(value); };

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
  spec.callbacks.onChange = [&](float value) { values.push_back(value); };

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
  spec.callbacks.onChange = [&](float) { changed += 1; };

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

TEST_CASE("PrimeStage default progress bar supports pointer and keyboard adjustments") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 100.0f);

  PrimeStage::ProgressBarSpec spec;
  spec.trackStyle = 341u;
  spec.fillStyle = 342u;
  spec.focusStyle = 343u;
  spec.size.preferredWidth = 180.0f;
  spec.size.preferredHeight = 12.0f;

  PrimeStage::UiNode progress = root.createProgressBar(spec);
  PrimeFrame::NodeId fillNodeId =
      findFirstNodeWithRectTokenInSubtree(frame, progress.nodeId(), spec.fillStyle);
  REQUIRE(fillNodeId.isValid());

  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 240.0f, 100.0f);
  PrimeFrame::LayoutOut const* out = layout.get(progress.nodeId());
  REQUIRE(out != nullptr);

  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;
  float x = out->absX + out->absW * 0.75f;
  float y = out->absY + out->absH * 0.5f;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 1, x, y), frame, layout, &focus);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 1, x, y), frame, layout, &focus);

  PrimeFrame::Node const* fillAfterPointer = frame.getNode(fillNodeId);
  REQUIRE(fillAfterPointer != nullptr);
  CHECK(fillAfterPointer->visible);
  REQUIRE(fillAfterPointer->sizeHint.width.preferred.has_value());
  CHECK(fillAfterPointer->sizeHint.width.preferred.value() > 0.0f);

  focus.setFocus(frame, layout, progress.nodeId());
  router.dispatch(makeKeyDownEvent(PrimeStage::KeyCode::Home), frame, layout, &focus);
  PrimeFrame::Node const* fillAfterHome = frame.getNode(fillNodeId);
  REQUIRE(fillAfterHome != nullptr);
  CHECK_FALSE(fillAfterHome->visible);

  router.dispatch(makeKeyDownEvent(PrimeStage::KeyCode::End), frame, layout, &focus);
  PrimeFrame::Node const* fillAfterEnd = frame.getNode(fillNodeId);
  REQUIRE(fillAfterEnd != nullptr);
  CHECK(fillAfterEnd->visible);
  REQUIRE(fillAfterEnd->sizeHint.width.preferred.has_value());
  CHECK(fillAfterEnd->sizeHint.width.preferred.value() == doctest::Approx(180.0f));
}

TEST_CASE("PrimeStage table and list keyboard selection matches pointer selection defaults") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 420.0f, 260.0f);

  PrimeStage::StackSpec stackSpec;
  stackSpec.gap = 10.0f;
  stackSpec.size.stretchX = 1.0f;
  stackSpec.size.stretchY = 1.0f;
  PrimeStage::UiNode stack = root.createVerticalStack(stackSpec);

  int tableSelected = -1;
  int tableSelectionEvents = 0;
  PrimeStage::TableSpec tableSpec;
  tableSpec.columns = {{"Name", 160.0f, 0u, 0u}};
  tableSpec.rows = {{"Alpha"}, {"Beta"}, {"Gamma"}, {"Delta"}};
  tableSpec.size.preferredWidth = 220.0f;
  tableSpec.size.preferredHeight = 120.0f;
  tableSpec.headerInset = 0.0f;
  tableSpec.headerHeight = 0.0f;
  tableSpec.rowHeight = 24.0f;
  tableSpec.rowGap = 0.0f;
  tableSpec.rowStyle = 351u;
  tableSpec.rowAltStyle = 352u;
  tableSpec.selectionStyle = 353u;
  tableSpec.callbacks.onSelect = [&](PrimeStage::TableRowInfo const& info) {
    tableSelected = info.rowIndex;
    tableSelectionEvents += 1;
  };
  PrimeStage::UiNode table = stack.createTable(tableSpec);

  int listSelected = -1;
  int listSelectionEvents = 0;
  PrimeStage::ListSpec listSpec;
  listSpec.items = {"One", "Two", "Three"};
  listSpec.size.preferredWidth = 220.0f;
  listSpec.size.preferredHeight = 96.0f;
  listSpec.rowHeight = 24.0f;
  listSpec.rowGap = 0.0f;
  listSpec.rowStyle = 361u;
  listSpec.rowAltStyle = 362u;
  listSpec.selectionStyle = 363u;
  listSpec.callbacks.onSelect = [&](PrimeStage::ListRowInfo const& info) {
    listSelected = info.rowIndex;
    listSelectionEvents += 1;
  };
  PrimeStage::UiNode list = stack.createList(listSpec);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 420.0f, 260.0f);
  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;

  PrimeFrame::NodeId tableCallbackNode =
      findFirstNodeWithOnEventInSubtree(frame, table.nodeId());
  REQUIRE(tableCallbackNode.isValid());
  PrimeFrame::LayoutOut const* tableOut = layout.get(tableCallbackNode);
  REQUIRE(tableOut != nullptr);
  float tableClickX = tableOut->absX + tableOut->absW * 0.5f;
  float tableClickY = tableOut->absY + tableSpec.rowHeight * 1.5f;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 1, tableClickX, tableClickY),
                  frame,
                  layout,
                  &focus);
  CHECK(tableSelected == 1);

  focus.setFocus(frame, layout, table.nodeId());
  router.dispatch(makeKeyDownEvent(PrimeStage::KeyCode::End), frame, layout, &focus);
  CHECK(tableSelected == 3);
  router.dispatch(makeKeyDownEvent(PrimeStage::KeyCode::Home), frame, layout, &focus);
  CHECK(tableSelected == 0);
  router.dispatch(makeKeyDownEvent(PrimeStage::KeyCode::Down), frame, layout, &focus);
  CHECK(tableSelected == 1);
  int tableEventsBeforeEnter = tableSelectionEvents;
  router.dispatch(makeKeyDownEvent(PrimeStage::KeyCode::Enter), frame, layout, &focus);
  CHECK(tableSelected == 1);
  CHECK(tableSelectionEvents == tableEventsBeforeEnter + 1);

  PrimeFrame::NodeId listCallbackNode = findFirstNodeWithOnEventInSubtree(frame, list.nodeId());
  REQUIRE(listCallbackNode.isValid());
  PrimeFrame::LayoutOut const* listOut = layout.get(listCallbackNode);
  REQUIRE(listOut != nullptr);
  float listClickX = listOut->absX + listOut->absW * 0.5f;
  float listClickY = listOut->absY + listSpec.rowHeight * 0.5f;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 2, listClickX, listClickY),
                  frame,
                  layout,
                  &focus);
  CHECK(listSelected == 0);

  focus.setFocus(frame, layout, list.nodeId());
  router.dispatch(makeKeyDownEvent(PrimeStage::KeyCode::End), frame, layout, &focus);
  CHECK(listSelected == 2);
  router.dispatch(makeKeyDownEvent(PrimeStage::KeyCode::Up), frame, layout, &focus);
  CHECK(listSelected == 1);
  CHECK(listSelectionEvents >= 3);
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
