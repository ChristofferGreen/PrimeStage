#include "PrimeStage/Ui.h"

#include "PrimeFrame/Events.h"
#include "PrimeFrame/Frame.h"
#include "PrimeFrame/Focus.h"
#include "PrimeFrame/Layout.h"

#include "third_party/doctest.h"

namespace {

PrimeStage::UiNode createRoot(PrimeFrame::Frame& frame) {
  PrimeFrame::NodeId rootId = frame.createNode();
  frame.addRoot(rootId);
  if (PrimeFrame::Node* node = frame.getNode(rootId)) {
    node->layout = PrimeFrame::LayoutType::Overlay;
    node->sizeHint.width.preferred = 640.0f;
    node->sizeHint.height.preferred = 360.0f;
  }
  return PrimeStage::UiNode(frame, rootId, true);
}

bool hasChild(PrimeFrame::Frame const& frame,
              PrimeFrame::NodeId parentId,
              PrimeFrame::NodeId childId) {
  PrimeFrame::Node const* parent = frame.getNode(parentId);
  if (!parent) {
    return false;
  }
  for (PrimeFrame::NodeId const candidate : parent->children) {
    if (candidate == childId) {
      return true;
    }
  }
  return false;
}

PrimeFrame::LayoutOutput layoutFrame(PrimeFrame::Frame& frame) {
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutEngine engine;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 640.0f;
  options.rootHeight = 360.0f;
  engine.layout(frame, layout, options);
  return layout;
}

} // namespace

TEST_CASE("PrimeStage builder API supports nested fluent composition") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::StackSpec stackSpec;
  stackSpec.size.preferredWidth = 260.0f;
  stackSpec.size.preferredHeight = 140.0f;
  stackSpec.gap = 4.0f;

  PrimeStage::PanelSpec panelSpec;
  panelSpec.layout = PrimeFrame::LayoutType::Overlay;
  panelSpec.size.preferredWidth = 200.0f;
  panelSpec.size.preferredHeight = 60.0f;

  PrimeStage::ButtonSpec buttonSpec;
  buttonSpec.label = "Build";
  buttonSpec.size.preferredWidth = 120.0f;
  buttonSpec.size.preferredHeight = 28.0f;

  int stackCalls = 0;
  int panelCalls = 0;
  int buttonCalls = 0;
  int withCalls = 0;
  PrimeFrame::NodeId stackId{};
  PrimeFrame::NodeId panelId{};
  PrimeFrame::NodeId buttonId{};
  PrimeFrame::NodeId buttonReturnId{};
  PrimeFrame::NodeId withReturnId{};

  root.createVerticalStack(stackSpec, [&](PrimeStage::UiNode& stack) {
    ++stackCalls;
    stackId = stack.nodeId();
    stack.createPanel(panelSpec, [&](PrimeStage::UiNode& panel) {
      ++panelCalls;
      panelId = panel.nodeId();
      PrimeStage::UiNode button = panel.createButton(buttonSpec, [&](PrimeStage::UiNode& built) {
        ++buttonCalls;
        buttonId = built.nodeId();
        built.setVisible(false);
      });
      buttonReturnId = button.nodeId();
      PrimeStage::UiNode chained = button.with([&](PrimeStage::UiNode& node) {
        ++withCalls;
        node.setHitTestVisible(false);
      });
      withReturnId = chained.nodeId();
    });
  });

  CHECK(stackCalls == 1);
  CHECK(panelCalls == 1);
  CHECK(buttonCalls == 1);
  CHECK(withCalls == 1);
  CHECK(buttonId == buttonReturnId);
  CHECK(buttonId == withReturnId);
  CHECK(hasChild(frame, root.nodeId(), stackId));
  CHECK(hasChild(frame, stackId, panelId));
  CHECK(hasChild(frame, panelId, buttonId));

  PrimeFrame::Node const* buttonNode = frame.getNode(buttonId);
  REQUIRE(buttonNode != nullptr);
  CHECK_FALSE(buttonNode->visible);
  CHECK_FALSE(buttonNode->hitTestVisible);
}

TEST_CASE("PrimeStage builder API materializes default widget fallbacks") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::ButtonSpec buttonSpec;
  PrimeStage::UiNode button = root.createButton(buttonSpec);
  CHECK(button.nodeId() != root.nodeId());
  CHECK(frame.getNode(button.nodeId()) != nullptr);

  PrimeStage::TextFieldSpec fieldSpec;
  PrimeStage::UiNode field = root.createTextField(fieldSpec);
  CHECK(field.nodeId() != root.nodeId());
  CHECK(frame.getNode(field.nodeId()) != nullptr);

  PrimeStage::ScrollViewSpec scrollSpec;
  PrimeStage::ScrollView scrollView = root.createScrollView(scrollSpec);
  CHECK(scrollView.root.nodeId() != root.nodeId());
  CHECK(scrollView.content.nodeId().isValid());
  CHECK(frame.getNode(scrollView.root.nodeId()) != nullptr);
  CHECK(frame.getNode(scrollView.content.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage declarative helpers support nested composition ergonomics") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  int clickCount = 0;
  PrimeFrame::NodeId columnId{};
  PrimeFrame::NodeId rowId{};
  PrimeFrame::NodeId buttonId{};
  PrimeFrame::NodeId spacerId{};
  PrimeFrame::NodeId windowContentId{};

  root.column([&](PrimeStage::UiNode& column) {
    columnId = column.nodeId();
    column.label("Declarative");
    rowId = column.row([&](PrimeStage::UiNode& row) {
      buttonId = row.button("Apply", [&]() { clickCount += 1; }).nodeId();
      spacerId = row.spacer(-8.0f).nodeId();
    }).nodeId();

    PrimeStage::WindowSpec windowSpec;
    windowSpec.title = "Panel";
    windowSpec.width = 220.0f;
    windowSpec.height = 140.0f;
    column.window(windowSpec, [&](PrimeStage::Window& window) {
      windowContentId = window.content.label("Window content").nodeId();
    });
  });

  CHECK(hasChild(frame, root.nodeId(), columnId));
  CHECK(hasChild(frame, columnId, rowId));
  CHECK(hasChild(frame, rowId, buttonId));
  CHECK(frame.getNode(windowContentId) != nullptr);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame);
  PrimeFrame::LayoutOut const* buttonOut = layout.get(buttonId);
  REQUIRE(buttonOut != nullptr);
  PrimeFrame::Event down;
  down.type = PrimeFrame::EventType::PointerDown;
  down.pointerId = 1;
  down.x = buttonOut->absX + buttonOut->absW * 0.5f;
  down.y = buttonOut->absY + buttonOut->absH * 0.5f;
  PrimeFrame::Event up = down;
  up.type = PrimeFrame::EventType::PointerUp;

  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;
  router.dispatch(down, frame, layout, &focus);
  router.dispatch(up, frame, layout, &focus);
  CHECK(clickCount == 1);

  // Diagnostic path: declarative helper invalid spacer height should clamp safely.
  PrimeFrame::LayoutOut const* spacerOut = layout.get(spacerId);
  REQUIRE(spacerOut != nullptr);
  CHECK(spacerOut->absH >= 0.0f);
}
