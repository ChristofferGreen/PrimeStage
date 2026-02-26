#include "PrimeStage/Ui.h"
#include "PrimeFrame/Events.h"
#include "PrimeFrame/Focus.h"
#include "PrimeFrame/Frame.h"
#include "PrimeFrame/Layout.h"

#include "third_party/doctest.h"

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

static PrimeFrame::Primitive const* firstTextPrimitive(PrimeFrame::Frame const& frame,
                                                       PrimeFrame::NodeId nodeId) {
  PrimeFrame::Node const* node = frame.getNode(nodeId);
  if (!node) {
    return nullptr;
  }
  for (PrimeFrame::PrimitiveId primId : node->primitives) {
    PrimeFrame::Primitive const* prim = frame.getPrimitive(primId);
    if (prim && prim->type == PrimeFrame::PrimitiveType::Text) {
      return prim;
    }
  }
  return nullptr;
}

static PrimeFrame::Primitive const* findTextChild(PrimeFrame::Frame const& frame,
                                                  PrimeFrame::NodeId parent,
                                                  PrimeFrame::TextStyleToken token) {
  PrimeFrame::Node const* parentNode = frame.getNode(parent);
  if (!parentNode) {
    return nullptr;
  }
  for (PrimeFrame::NodeId child : parentNode->children) {
    PrimeFrame::Primitive const* prim = firstTextPrimitive(frame, child);
    if (prim && prim->textStyle.token == token) {
      return prim;
    }
  }
  return nullptr;
}

TEST_CASE("PrimeStage tabs create active and inactive tab styles") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 80.0f);

  PrimeStage::TabsSpec spec;
  spec.labels = {"One", "Two", "Three"};
  spec.selectedIndex = 1;
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 28.0f;
  spec.tabStyle = 61u;
  spec.activeTabStyle = 62u;
  spec.textStyle = 71u;
  spec.activeTextStyle = 72u;

  PrimeStage::UiNode tabs = root.createTabs(spec);
  PrimeFrame::Node const* row = frame.getNode(tabs.nodeId());
  REQUIRE(row != nullptr);
  CHECK(row->layout == PrimeFrame::LayoutType::HorizontalStack);
  CHECK(row->children.size() == spec.labels.size());

  for (size_t i = 0; i < row->children.size(); ++i) {
    PrimeFrame::Node const* tabNode = frame.getNode(row->children[i]);
    REQUIRE(tabNode != nullptr);
    REQUIRE(!tabNode->primitives.empty());
    PrimeFrame::Primitive const* rectPrim = frame.getPrimitive(tabNode->primitives.front());
    REQUIRE(rectPrim != nullptr);
    PrimeFrame::RectStyleToken expectedToken =
        static_cast<int>(i) == spec.selectedIndex ? spec.activeTabStyle : spec.tabStyle;
    CHECK(rectPrim->rect.token == expectedToken);
    REQUIRE(tabNode->sizeHint.height.preferred.has_value());
    CHECK(tabNode->sizeHint.height.preferred.value() == doctest::Approx(spec.size.preferredHeight.value()));

    PrimeFrame::TextStyleToken expectedText =
        static_cast<int>(i) == spec.selectedIndex ? spec.activeTextStyle : spec.textStyle;
    PrimeFrame::Primitive const* textPrim = findTextChild(frame, tabNode->id, expectedText);
    REQUIRE(textPrim != nullptr);
    CHECK(textPrim->textStyle.token == expectedText);
  }
}

TEST_CASE("PrimeStage dropdown with label creates label and indicator text") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 80.0f);

  PrimeStage::DropdownSpec spec;
  spec.label = "Options";
  spec.indicator = "v";
  spec.size.preferredWidth = 160.0f;
  spec.size.preferredHeight = 24.0f;
  spec.paddingX = 8.0f;
  spec.indicatorGap = 6.0f;
  spec.backgroundStyle = 81u;
  spec.textStyle = 91u;
  spec.indicatorStyle = 92u;

  PrimeStage::UiNode dropdown = root.createDropdown(spec);
  PrimeFrame::Node const* node = frame.getNode(dropdown.nodeId());
  REQUIRE(node != nullptr);
  CHECK(node->layout == PrimeFrame::LayoutType::HorizontalStack);
  CHECK(node->padding.left == doctest::Approx(spec.paddingX));
  CHECK(node->padding.right == doctest::Approx(spec.paddingX));
  CHECK(node->gap == doctest::Approx(spec.indicatorGap));
  CHECK(node->children.size() >= 2);

  PrimeFrame::Primitive const* labelPrim = findTextChild(frame, node->id, spec.textStyle);
  PrimeFrame::Primitive const* indicatorPrim = findTextChild(frame, node->id, spec.indicatorStyle);
  REQUIRE(labelPrim != nullptr);
  REQUIRE(indicatorPrim != nullptr);
  CHECK(labelPrim->textBlock.align == PrimeFrame::TextAlign::Start);
  CHECK(indicatorPrim->textBlock.align == PrimeFrame::TextAlign::Center);
}

TEST_CASE("PrimeStage dropdown with empty label inserts spacer") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 60.0f);

  PrimeStage::DropdownSpec spec;
  spec.label = "";
  spec.indicator = "v";
  spec.size.preferredWidth = 140.0f;
  spec.size.preferredHeight = 20.0f;
  spec.paddingX = 6.0f;
  spec.indicatorGap = 4.0f;
  spec.backgroundStyle = 101u;
  spec.textStyle = 111u;
  spec.indicatorStyle = 112u;

  PrimeStage::UiNode dropdown = root.createDropdown(spec);
  PrimeFrame::Node const* node = frame.getNode(dropdown.nodeId());
  REQUIRE(node != nullptr);
  CHECK(node->children.size() >= 2);

  int spacerCount = 0;
  int indicatorCount = 0;
  for (PrimeFrame::NodeId child : node->children) {
    PrimeFrame::Node const* childNode = frame.getNode(child);
    REQUIRE(childNode != nullptr);
    PrimeFrame::Primitive const* prim = firstTextPrimitive(frame, child);
    if (!prim) {
      if (childNode->primitives.empty()) {
        spacerCount += 1;
      }
      continue;
    }
    if (prim->textStyle.token == spec.indicatorStyle) {
      indicatorCount += 1;
    }
  }

  CHECK(spacerCount == 1);
  CHECK(indicatorCount == 1);
}

TEST_CASE("PrimeStage tabs onTabChanged supports pointer and keyboard activation") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 280.0f, 100.0f);

  PrimeStage::TabsSpec spec;
  spec.labels = {"One", "Two", "Three"};
  spec.selectedIndex = 1;
  spec.size.preferredWidth = 240.0f;
  spec.size.preferredHeight = 28.0f;
  spec.tabStyle = 61u;
  spec.activeTabStyle = 62u;
  spec.textStyle = 71u;
  spec.activeTextStyle = 72u;

  std::vector<int> selections;
  spec.callbacks.onTabChanged = [&](int index) { selections.push_back(index); };

  PrimeStage::UiNode tabs = root.createTabs(spec);
  PrimeFrame::Node const* row = frame.getNode(tabs.nodeId());
  REQUIRE(row != nullptr);
  REQUIRE(row->children.size() == 3);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 280.0f, 100.0f);
  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;

  PrimeFrame::LayoutOut const* third = layout.get(row->children[2]);
  REQUIRE(third != nullptr);
  float thirdX = third->absX + third->absW * 0.5f;
  float thirdY = third->absY + third->absH * 0.5f;

  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 1, thirdX, thirdY),
                  frame,
                  layout,
                  &focus);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 1, thirdX, thirdY),
                  frame,
                  layout,
                  &focus);
  REQUIRE(!selections.empty());
  CHECK(selections.back() == 2);

  PrimeFrame::Event keyLeft;
  keyLeft.type = PrimeFrame::EventType::KeyDown;
  keyLeft.key = PrimeStage::keyCodeInt(PrimeStage::KeyCode::Left);
  router.dispatch(keyLeft, frame, layout, &focus);
  REQUIRE(selections.size() >= 2);
  CHECK(selections.back() == 1);

  PrimeFrame::Event keyEnter;
  keyEnter.type = PrimeFrame::EventType::KeyDown;
  keyEnter.key = PrimeStage::keyCodeInt(PrimeStage::KeyCode::Enter);
  router.dispatch(keyEnter, frame, layout, &focus);
  REQUIRE(selections.size() >= 3);
  CHECK(selections.back() == 2);
}

TEST_CASE("PrimeStage dropdown onOpened and onSelected support pointer and keyboard") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 260.0f, 100.0f);

  PrimeStage::DropdownSpec spec;
  spec.options = {"Preview", "Edit", "Export"};
  spec.selectedIndex = 0;
  spec.indicator = "v";
  spec.backgroundStyle = 81u;
  spec.textStyle = 91u;
  spec.indicatorStyle = 92u;
  spec.focusStyle = 93u;
  spec.size.preferredWidth = 180.0f;
  spec.size.preferredHeight = 24.0f;

  int openedCount = 0;
  std::vector<int> selections;
  spec.callbacks.onOpened = [&]() { openedCount += 1; };
  spec.callbacks.onSelected = [&](int index) { selections.push_back(index); };

  PrimeStage::UiNode dropdown = root.createDropdown(spec);
  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 260.0f, 100.0f);
  PrimeFrame::LayoutOut const* out = layout.get(dropdown.nodeId());
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
  CHECK(openedCount == 1);
  REQUIRE(!selections.empty());
  CHECK(selections.back() == 1);

  PrimeFrame::Event keySpace;
  keySpace.type = PrimeFrame::EventType::KeyDown;
  keySpace.key = PrimeStage::keyCodeInt(PrimeStage::KeyCode::Space);
  router.dispatch(keySpace, frame, layout, &focus);
  CHECK(openedCount == 2);
  REQUIRE(selections.size() >= 2);
  CHECK(selections.back() == 2);
}

TEST_CASE("PrimeStage dropdown with no options emits onOpened but not onSelected") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 220.0f, 80.0f);

  PrimeStage::DropdownSpec spec;
  spec.label = "Static";
  spec.indicator = "v";
  spec.backgroundStyle = 101u;
  spec.textStyle = 111u;
  spec.indicatorStyle = 112u;
  spec.focusStyle = 113u;
  spec.size.preferredWidth = 140.0f;
  spec.size.preferredHeight = 22.0f;

  int openedCount = 0;
  int selectedCount = 0;
  spec.callbacks.onOpened = [&]() { openedCount += 1; };
  spec.callbacks.onSelected = [&](int) { selectedCount += 1; };

  PrimeStage::UiNode dropdown = root.createDropdown(spec);
  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 220.0f, 80.0f);
  PrimeFrame::LayoutOut const* out = layout.get(dropdown.nodeId());
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

  PrimeFrame::Event keyEnter;
  keyEnter.type = PrimeFrame::EventType::KeyDown;
  keyEnter.key = PrimeStage::keyCodeInt(PrimeStage::KeyCode::Enter);
  router.dispatch(keyEnter, frame, layout, &focus);

  CHECK(openedCount == 2);
  CHECK(selectedCount == 0);
}

TEST_CASE("PrimeStage tabs state-backed mode uses and updates TabsState") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 280.0f, 100.0f);

  PrimeStage::TabsState tabsState;
  tabsState.selectedIndex = 2;

  PrimeStage::TabsSpec spec;
  spec.state = &tabsState;
  spec.labels = {"One", "Two", "Three"};
  spec.selectedIndex = 0; // state-backed mode uses TabsState as source of truth
  spec.size.preferredWidth = 240.0f;
  spec.size.preferredHeight = 28.0f;
  spec.tabStyle = 161u;
  spec.activeTabStyle = 162u;
  spec.textStyle = 171u;
  spec.activeTextStyle = 172u;

  std::vector<int> selections;
  spec.callbacks.onTabChanged = [&](int index) { selections.push_back(index); };

  PrimeStage::UiNode tabs = root.createTabs(spec);
  PrimeFrame::Node const* row = frame.getNode(tabs.nodeId());
  REQUIRE(row != nullptr);
  REQUIRE(row->children.size() == 3);

  PrimeFrame::Node const* initiallyActive = frame.getNode(row->children[2]);
  REQUIRE(initiallyActive != nullptr);
  REQUIRE_FALSE(initiallyActive->primitives.empty());
  PrimeFrame::Primitive const* activeRect = frame.getPrimitive(initiallyActive->primitives.front());
  REQUIRE(activeRect != nullptr);
  CHECK(activeRect->rect.token == spec.activeTabStyle);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 280.0f, 100.0f);
  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;

  PrimeFrame::LayoutOut const* first = layout.get(row->children[0]);
  REQUIRE(first != nullptr);
  float firstX = first->absX + first->absW * 0.5f;
  float firstY = first->absY + first->absH * 0.5f;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 1, firstX, firstY),
                  frame,
                  layout,
                  &focus);
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 1, firstX, firstY),
                  frame,
                  layout,
                  &focus);
  CHECK(tabsState.selectedIndex == 0);
  REQUIRE(!selections.empty());
  CHECK(selections.back() == 0);

  PrimeFrame::Event keyRight;
  keyRight.type = PrimeFrame::EventType::KeyDown;
  keyRight.key = PrimeStage::keyCodeInt(PrimeStage::KeyCode::Right);
  router.dispatch(keyRight, frame, layout, &focus);
  CHECK(tabsState.selectedIndex == 1);
  REQUIRE(selections.size() >= 2);
  CHECK(selections.back() == 1);
}

TEST_CASE("PrimeStage dropdown state-backed mode uses and updates DropdownState") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 260.0f, 100.0f);

  PrimeStage::DropdownState dropdownState;
  dropdownState.selectedIndex = 2;

  PrimeStage::DropdownSpec spec;
  spec.state = &dropdownState;
  spec.options = {"Preview", "Edit", "Export"};
  spec.selectedIndex = 0; // state-backed mode uses DropdownState as source of truth
  spec.indicator = "v";
  spec.backgroundStyle = 181u;
  spec.textStyle = 191u;
  spec.indicatorStyle = 192u;
  spec.focusStyle = 193u;
  spec.size.preferredWidth = 180.0f;
  spec.size.preferredHeight = 24.0f;

  int openedCount = 0;
  std::vector<int> selections;
  spec.callbacks.onOpened = [&]() { openedCount += 1; };
  spec.callbacks.onSelected = [&](int index) { selections.push_back(index); };

  PrimeStage::UiNode dropdown = root.createDropdown(spec);
  PrimeFrame::Node const* dropdownNode = frame.getNode(dropdown.nodeId());
  REQUIRE(dropdownNode != nullptr);
  PrimeFrame::Primitive const* labelPrim = findTextChild(frame, dropdownNode->id, spec.textStyle);
  REQUIRE(labelPrim != nullptr);
  CHECK(labelPrim->textBlock.text == "Export");

  PrimeFrame::LayoutOutput layout = layoutFrame(frame, 260.0f, 100.0f);
  PrimeFrame::LayoutOut const* out = layout.get(dropdown.nodeId());
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
  CHECK(openedCount == 1);
  CHECK(dropdownState.selectedIndex == 0);
  REQUIRE(!selections.empty());
  CHECK(selections.back() == 0);

  PrimeFrame::Event keyUp;
  keyUp.type = PrimeFrame::EventType::KeyDown;
  keyUp.key = PrimeStage::keyCodeInt(PrimeStage::KeyCode::Up);
  router.dispatch(keyUp, frame, layout, &focus);
  CHECK(openedCount == 2);
  CHECK(dropdownState.selectedIndex == 2);
  REQUIRE(selections.size() >= 2);
  CHECK(selections.back() == 2);
}
