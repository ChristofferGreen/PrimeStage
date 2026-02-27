#include "PrimeStage/Ui.h"

#include "PrimeFrame/Frame.h"

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

TEST_CASE("PrimeStage builder API handles non-materialized defaults") {
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
  CHECK(scrollView.root.nodeId() == root.nodeId());
  CHECK_FALSE(scrollView.content.nodeId().isValid());
}
