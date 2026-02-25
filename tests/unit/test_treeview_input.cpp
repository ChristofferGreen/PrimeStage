#include "PrimeStage/PrimeStage.h"

#include "PrimeFrame/Events.h"
#include "PrimeFrame/Focus.h"
#include "PrimeFrame/Layout.h"
#include "third_party/doctest.h"

using namespace PrimeStage;

namespace {
PrimeFrame::NodeId makeRoot(PrimeFrame::Frame& frame, float width, float height) {
  PrimeFrame::NodeId root = frame.createNode();
  frame.addRoot(root);
  if (auto* node = frame.getNode(root)) {
    node->layout = PrimeFrame::LayoutType::Overlay;
    node->sizeHint.width.preferred = width;
    node->sizeHint.height.preferred = height;
  }
  return root;
}
}

TEST_CASE("TreeView keyboard navigation selects rows") {
  PrimeFrame::Frame frame;
  PrimeFrame::NodeId rootId = makeRoot(frame, 240.0f, 200.0f);
  UiNode root(frame, rootId);

  TreeViewSpec spec;
  spec.size.preferredWidth = 240.0f;
  spec.size.preferredHeight = 200.0f;
  spec.rowStartY = 0.0f;
  spec.rowHeight = 20.0f;
  spec.rowGap = 0.0f;
  spec.keyboardNavigation = true;
  spec.rowStyle = 0;
  spec.rowAltStyle = 0;
  spec.selectionStyle = 0;
  spec.textStyle = 0;
  spec.selectedTextStyle = 0;
  spec.nodes = {TreeNode{"Root", {TreeNode{"Child"}}, true, false}};

  int selected = -1;
  spec.callbacks.onSelectionChanged = [&](TreeViewRowInfo const& info) { selected = info.rowIndex; };

  UiNode tree = root.createTreeView(spec);

  PrimeFrame::LayoutEngine engine;
  PrimeFrame::LayoutOutput layout;
  engine.layout(frame, layout);

  PrimeFrame::FocusManager focus;
  focus.setActiveRoot(frame, layout, rootId);

  PrimeFrame::EventRouter router;
  PrimeFrame::Event down;
  down.type = PrimeFrame::EventType::PointerDown;
  down.pointerId = 1;
  down.x = 8.0f;
  down.y = 10.0f;
  router.dispatch(down, frame, layout, &focus);

  CHECK(selected == 0);

  PrimeFrame::Event keyDown;
  keyDown.type = PrimeFrame::EventType::KeyDown;
  keyDown.key = 0x51; // KeyDown
  router.dispatch(keyDown, frame, layout, &focus);
  CHECK(selected == 1);

  keyDown.key = 0x51; // move down again
  router.dispatch(keyDown, frame, layout, &focus);
  CHECK(selected == 1);
}

TEST_CASE("Slider drag updates value") {
  PrimeFrame::Frame frame;
  PrimeFrame::NodeId rootId = makeRoot(frame, 300.0f, 120.0f);
  UiNode root(frame, rootId);

  SliderSpec spec;
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 20.0f;
  spec.value = 0.0f;
  float lastValue = -1.0f;
  spec.callbacks.onValueChanged = [&](float value) { lastValue = value; };

  UiNode slider = root.createSlider(spec);

  PrimeFrame::LayoutEngine engine;
  PrimeFrame::LayoutOutput layout;
  engine.layout(frame, layout);

  PrimeFrame::EventRouter router;
  PrimeFrame::Event down;
  down.type = PrimeFrame::EventType::PointerDown;
  down.pointerId = 2;
  down.x = 10.0f;
  down.y = 10.0f;
  router.dispatch(down, frame, layout, nullptr);

  PrimeFrame::Event move;
  move.type = PrimeFrame::EventType::PointerMove;
  move.pointerId = 2;
  move.x = 150.0f;
  move.y = 10.0f;
  router.dispatch(move, frame, layout, nullptr);

  CHECK(lastValue > 0.0f);
}

TEST_CASE("TreeView scroll follows keyboard selection") {
  PrimeFrame::Frame frame;
  PrimeFrame::NodeId rootId = makeRoot(frame, 240.0f, 80.0f);
  UiNode root(frame, rootId);

  TreeViewSpec spec;
  spec.size.preferredWidth = 240.0f;
  spec.size.preferredHeight = 80.0f;
  spec.rowStartY = 0.0f;
  spec.rowHeight = 20.0f;
  spec.rowGap = 0.0f;
  spec.keyboardNavigation = true;
  spec.rowStyle = 0;
  spec.rowAltStyle = 0;
  spec.selectionStyle = 0;
  spec.textStyle = 0;
  spec.selectedTextStyle = 0;

  std::vector<TreeNode> children;
  for (int i = 0; i < 12; ++i) {
    children.push_back(TreeNode{"Item", {}});
  }
  spec.nodes = {TreeNode{"Root", children, true, false}};

  float lastScrollOffset = 0.0f;
  spec.callbacks.onScrollChanged = [&](TreeViewScrollInfo const& info) {
    lastScrollOffset = info.offset;
  };

  UiNode tree = root.createTreeView(spec);

  PrimeFrame::LayoutEngine engine;
  PrimeFrame::LayoutOutput layout;
  engine.layout(frame, layout);

  PrimeFrame::FocusManager focus;
  focus.setActiveRoot(frame, layout, rootId);

  PrimeFrame::EventRouter router;
  PrimeFrame::Event down;
  down.type = PrimeFrame::EventType::PointerDown;
  down.pointerId = 1;
  down.x = 8.0f;
  down.y = 10.0f;
  router.dispatch(down, frame, layout, &focus);

  PrimeFrame::Event keyDown;
  keyDown.type = PrimeFrame::EventType::KeyDown;
  keyDown.key = 0x51; // KeyDown
  for (int i = 0; i < 6; ++i) {
    router.dispatch(keyDown, frame, layout, &focus);
  }

  CHECK(lastScrollOffset > 0.0f);
}
