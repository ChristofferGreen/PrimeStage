#include "PrimeStage/Ui.h"

#include "PrimeFrame/Events.h"
#include "PrimeFrame/Focus.h"
#include "PrimeFrame/Frame.h"
#include "PrimeFrame/Layout.h"

#include "third_party/doctest.h"

#include <array>
#include <functional>
#include <random>
#include <string>
#include <vector>

namespace {

constexpr float RootWidth = 360.0f;
constexpr float RootHeight = 240.0f;
constexpr int RowCount = 2;

PrimeStage::UiNode createRoot(PrimeFrame::Frame& frame) {
  PrimeFrame::NodeId rootId = frame.createNode();
  frame.addRoot(rootId);
  PrimeFrame::Node* root = frame.getNode(rootId);
  REQUIRE(root != nullptr);
  root->layout = PrimeFrame::LayoutType::Overlay;
  root->sizeHint.width.preferred = RootWidth;
  root->sizeHint.height.preferred = RootHeight;
  return PrimeStage::UiNode(frame, rootId, true);
}

PrimeFrame::LayoutOutput layoutFrame(PrimeFrame::Frame& frame) {
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutEngine engine;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = RootWidth;
  options.rootHeight = RootHeight;
  engine.layout(frame, layout, options);
  return layout;
}

PrimeFrame::Event makePointerEvent(PrimeFrame::EventType type, int pointerId, float x, float y) {
  PrimeFrame::Event event;
  event.type = type;
  event.pointerId = pointerId;
  event.x = x;
  event.y = y;
  return event;
}

PrimeFrame::Event makeKeyDown(int key) {
  PrimeFrame::Event event;
  event.type = PrimeFrame::EventType::KeyDown;
  event.key = key;
  return event;
}

PrimeFrame::Event makeTextInput(std::string text) {
  PrimeFrame::Event event;
  event.type = PrimeFrame::EventType::TextInput;
  event.text = std::move(text);
  return event;
}

struct StateMachineHarness {
  PrimeFrame::Frame frame;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;

  PrimeStage::TextFieldState textState;
  PrimeStage::ToggleState toggleState;

  PrimeFrame::NodeId buttonNode{};
  PrimeFrame::NodeId textFieldNode{};
  PrimeFrame::NodeId toggleNode{};
  PrimeFrame::NodeId tableNode{};

  std::vector<PrimeFrame::NodeId> focusableNodes;
  int textChangeCount = 0;
  int toggleChangeCount = 0;
  int lastClickedRow = -1;
};

StateMachineHarness buildHarness() {
  StateMachineHarness harness;
  harness.textState.text = "seed";
  harness.textState.cursor = static_cast<uint32_t>(harness.textState.text.size());
  harness.textState.selectionAnchor = harness.textState.cursor;
  harness.textState.selectionStart = harness.textState.cursor;
  harness.textState.selectionEnd = harness.textState.cursor;
  harness.toggleState.on = false;

  PrimeStage::UiNode root = createRoot(harness.frame);
  PrimeStage::StackSpec stackSpec;
  stackSpec.size.stretchX = 1.0f;
  stackSpec.size.stretchY = 1.0f;
  stackSpec.padding.left = 12.0f;
  stackSpec.padding.top = 12.0f;
  stackSpec.padding.right = 12.0f;
  stackSpec.padding.bottom = 12.0f;
  stackSpec.gap = 8.0f;
  PrimeStage::UiNode stack = root.createVerticalStack(stackSpec);

  PrimeStage::ButtonSpec buttonSpec;
  buttonSpec.label = "Action";
  buttonSpec.size.preferredWidth = 120.0f;
  buttonSpec.size.preferredHeight = 28.0f;
  PrimeStage::UiNode button = stack.createButton(buttonSpec);

  PrimeStage::TextFieldSpec fieldSpec;
  fieldSpec.state = &harness.textState;
  fieldSpec.size.preferredWidth = 180.0f;
  fieldSpec.size.preferredHeight = 28.0f;
  fieldSpec.callbacks.onTextChanged = [&](std::string_view) { harness.textChangeCount += 1; };
  PrimeStage::UiNode field = stack.createTextField(fieldSpec);

  PrimeStage::ToggleSpec toggleSpec;
  toggleSpec.state = &harness.toggleState;
  toggleSpec.size.preferredWidth = 56.0f;
  toggleSpec.size.preferredHeight = 24.0f;
  toggleSpec.trackStyle = 1u;
  toggleSpec.knobStyle = 2u;
  toggleSpec.callbacks.onChanged = [&](bool) { harness.toggleChangeCount += 1; };
  PrimeStage::UiNode toggle = stack.createToggle(toggleSpec);

  PrimeStage::TableSpec tableSpec;
  tableSpec.size.preferredWidth = 260.0f;
  tableSpec.size.preferredHeight = 90.0f;
  tableSpec.headerHeight = 20.0f;
  tableSpec.rowHeight = 24.0f;
  tableSpec.rowGap = 0.0f;
  tableSpec.columns = {{"Name", 120.0f, 0u, 0u}, {"Value", 120.0f, 0u, 0u}};
  tableSpec.rows = {{"A", "1"}, {"B", "2"}};
  tableSpec.selectionStyle = 3u;
  tableSpec.callbacks.onRowClicked = [&](PrimeStage::TableRowInfo const& info) {
    harness.lastClickedRow = info.rowIndex;
  };
  PrimeStage::UiNode table = stack.createTable(tableSpec);

  harness.buttonNode = button.nodeId();
  harness.textFieldNode = field.nodeId();
  harness.toggleNode = toggle.nodeId();
  harness.tableNode = table.nodeId();
  harness.focusableNodes = {
      harness.buttonNode,
      harness.textFieldNode,
      harness.toggleNode,
      harness.tableNode,
  };
  harness.layout = layoutFrame(harness.frame);
  return harness;
}

void assertInvariants(StateMachineHarness const& harness) {
  PrimeFrame::NodeId focused = harness.focus.focusedNode();
  if (focused.isValid()) {
    PrimeFrame::Node const* node = harness.frame.getNode(focused);
    REQUIRE(node != nullptr);
    CHECK(node->focusable);
  }

  size_t textSize = harness.textState.text.size();
  CHECK(harness.textState.cursor <= textSize);
  CHECK(harness.textState.selectionAnchor <= textSize);
  CHECK(harness.textState.selectionStart <= textSize);
  CHECK(harness.textState.selectionEnd <= textSize);
  CHECK(harness.textState.selectionStart <= harness.textState.selectionEnd);
  CHECK(harness.lastClickedRow >= -1);
  CHECK(harness.lastClickedRow < RowCount);
}

void clickNodeCenter(StateMachineHarness& harness, PrimeFrame::NodeId nodeId, int pointerId) {
  PrimeFrame::LayoutOut const* out = harness.layout.get(nodeId);
  REQUIRE(out != nullptr);
  float centerX = out->absX + out->absW * 0.5f;
  float centerY = out->absY + out->absH * 0.5f;
  harness.router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, pointerId, centerX, centerY),
                          harness.frame,
                          harness.layout,
                          &harness.focus);
  harness.router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, pointerId, centerX, centerY),
                          harness.frame,
                          harness.layout,
                          &harness.focus);
}

} // namespace

TEST_CASE("PrimeStage input focus state machine keeps invariants under deterministic fuzz") {
  StateMachineHarness harness = buildHarness();

  constexpr uint64_t FuzzSeed = 0xD1CEB00Cu;
  std::mt19937_64 rng(FuzzSeed);
  std::uniform_int_distribution<int> actionDist(0, 9);
  std::uniform_int_distribution<int> pointerIdDist(1, 5);
  std::uniform_real_distribution<float> xDist(-48.0f, RootWidth + 48.0f);
  std::uniform_real_distribution<float> yDist(-48.0f, RootHeight + 48.0f);
  std::uniform_real_distribution<float> scrollDist(-120.0f, 120.0f);

  std::array<int, 9> keys{{
      PrimeStage::keyCodeInt(PrimeStage::KeyCode::Enter),
      PrimeStage::keyCodeInt(PrimeStage::KeyCode::Space),
      PrimeStage::keyCodeInt(PrimeStage::KeyCode::Backspace),
      PrimeStage::keyCodeInt(PrimeStage::KeyCode::Left),
      PrimeStage::keyCodeInt(PrimeStage::KeyCode::Right),
      PrimeStage::keyCodeInt(PrimeStage::KeyCode::Up),
      PrimeStage::keyCodeInt(PrimeStage::KeyCode::Down),
      PrimeStage::keyCodeInt(PrimeStage::KeyCode::Home),
      PrimeStage::keyCodeInt(PrimeStage::KeyCode::End),
  }};
  std::array<std::string, 6> textInputs{{"a", "B", " ", "z", "\n", "01"}};

  for (int step = 0; step < 1200; ++step) {
    int action = actionDist(rng);
    switch (action) {
      case 0:
      case 1:
      case 2: {
        PrimeFrame::EventType type = PrimeFrame::EventType::PointerMove;
        if (action == 1) {
          type = PrimeFrame::EventType::PointerDown;
        } else if (action == 2) {
          type = PrimeFrame::EventType::PointerUp;
        }
        harness.router.dispatch(makePointerEvent(type, pointerIdDist(rng), xDist(rng), yDist(rng)),
                                harness.frame,
                                harness.layout,
                                &harness.focus);
        break;
      }
      case 3: {
        PrimeFrame::Event event;
        event.type = PrimeFrame::EventType::PointerScroll;
        event.pointerId = pointerIdDist(rng);
        event.x = xDist(rng);
        event.y = yDist(rng);
        event.scrollY = scrollDist(rng);
        harness.router.dispatch(event, harness.frame, harness.layout, &harness.focus);
        break;
      }
      case 4: {
        int key = keys[static_cast<size_t>(rng() % keys.size())];
        harness.router.dispatch(makeKeyDown(key), harness.frame, harness.layout, &harness.focus);
        break;
      }
      case 5: {
        std::string token = textInputs[static_cast<size_t>(rng() % textInputs.size())];
        harness.router.dispatch(makeTextInput(token), harness.frame, harness.layout, &harness.focus);
        break;
      }
      case 6: {
        bool forward = (rng() & 1u) != 0u;
        (void)harness.focus.handleTab(harness.frame, harness.layout, forward);
        break;
      }
      case 7: {
        PrimeFrame::NodeId nodeId =
            harness.focusableNodes[static_cast<size_t>(rng() % harness.focusableNodes.size())];
        (void)harness.focus.setFocus(harness.frame, harness.layout, nodeId);
        break;
      }
      case 8: {
        harness.focus.clearFocus(harness.frame);
        break;
      }
      case 9: {
        PrimeFrame::NodeId nodeId =
            harness.focusableNodes[static_cast<size_t>(rng() % harness.focusableNodes.size())];
        clickNodeCenter(harness, nodeId, pointerIdDist(rng));
        break;
      }
      default:
        break;
    }
    assertInvariants(harness);
  }
}

TEST_CASE("PrimeStage input focus regression corpus preserves invariants") {
  StateMachineHarness harness = buildHarness();

  std::vector<std::function<void()>> corpus;
  corpus.push_back([&]() {
    clickNodeCenter(harness, harness.textFieldNode, 1);
    harness.router.dispatch(makeTextInput("prime"), harness.frame, harness.layout, &harness.focus);
    harness.router.dispatch(makeKeyDown(PrimeStage::keyCodeInt(PrimeStage::KeyCode::Backspace)),
                            harness.frame,
                            harness.layout,
                            &harness.focus);
    harness.router.dispatch(makeKeyDown(PrimeStage::keyCodeInt(PrimeStage::KeyCode::Enter)),
                            harness.frame,
                            harness.layout,
                            &harness.focus);
  });
  corpus.push_back([&]() {
    clickNodeCenter(harness, harness.tableNode, 2);
    harness.router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 2, -30.0f, -30.0f),
                            harness.frame,
                            harness.layout,
                            &harness.focus);
    harness.router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerUp, 2, -30.0f, -30.0f),
                            harness.frame,
                            harness.layout,
                            &harness.focus);
    (void)harness.focus.handleTab(harness.frame, harness.layout, true);
    (void)harness.focus.handleTab(harness.frame, harness.layout, true);
  });
  corpus.push_back([&]() {
    clickNodeCenter(harness, harness.toggleNode, 3);
    harness.router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerMove, 3, RootWidth + 60.0f, 10.0f),
                            harness.frame,
                            harness.layout,
                            &harness.focus);
    PrimeFrame::Event scroll;
    scroll.type = PrimeFrame::EventType::PointerScroll;
    scroll.pointerId = 3;
    scroll.x = RootWidth * 0.5f;
    scroll.y = RootHeight * 0.5f;
    scroll.scrollY = 240.0f;
    harness.router.dispatch(scroll, harness.frame, harness.layout, &harness.focus);
  });

  for (auto const& run : corpus) {
    run();
    assertInvariants(harness);
  }

  CHECK(harness.textChangeCount >= 1);
  CHECK(harness.toggleChangeCount >= 1);
  CHECK(harness.lastClickedRow >= -1);
  CHECK(harness.lastClickedRow < RowCount);
}
