#include "PrimeStage/Ui.h"

#include "PrimeFrame/Focus.h"
#include "PrimeFrame/Frame.h"
#include "PrimeFrame/Layout.h"

#include "third_party/doctest.h"

namespace {

constexpr float RootWidth = 360.0f;
constexpr float RootHeight = 220.0f;
constexpr std::string_view IdentityPrimary = "primary";
constexpr std::string_view IdentityField = "field";

PrimeStage::UiNode createRoot(PrimeFrame::Frame& frame) {
  PrimeFrame::NodeId rootId = frame.createNode();
  frame.addRoot(rootId);
  if (PrimeFrame::Node* root = frame.getNode(rootId)) {
    root->layout = PrimeFrame::LayoutType::VerticalStack;
    root->sizeHint.width.preferred = RootWidth;
    root->sizeHint.height.preferred = RootHeight;
    root->gap = 8.0f;
  }
  return PrimeStage::UiNode(frame, rootId, true);
}

PrimeFrame::LayoutOutput layoutFrame(PrimeFrame::Frame& frame) {
  PrimeFrame::LayoutOutput output;
  PrimeFrame::LayoutEngine engine;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = RootWidth;
  options.rootHeight = RootHeight;
  engine.layout(frame, output, options);
  return output;
}

PrimeStage::ButtonSpec makeButtonSpec() {
  PrimeStage::ButtonSpec spec;
  spec.label = "Action";
  spec.size.preferredWidth = 120.0f;
  spec.size.preferredHeight = 28.0f;
  spec.backgroundStyle = 101u;
  return spec;
}

PrimeStage::TextFieldSpec makeFieldSpec(PrimeStage::TextFieldState& state) {
  PrimeStage::TextFieldSpec spec;
  spec.state = &state;
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 28.0f;
  spec.backgroundStyle = 201u;
  spec.textStyle = 202u;
  return spec;
}

} // namespace

TEST_CASE("WidgetIdentityReconciler restores keyed focus after rebuild") {
  PrimeStage::WidgetIdentityReconciler reconciler;
  PrimeFrame::FocusManager focus;

  PrimeFrame::Frame initialFrame;
  PrimeStage::UiNode initialRoot = createRoot(initialFrame);
  PrimeStage::UiNode initialPrimary = initialRoot.createButton(makeButtonSpec());
  PrimeStage::TextFieldState initialFieldState;
  initialFieldState.text = "hello";
  PrimeStage::UiNode initialField = initialRoot.createTextField(makeFieldSpec(initialFieldState));

  reconciler.registerNode(IdentityPrimary, initialPrimary.nodeId());
  reconciler.registerNode(IdentityField, initialField.nodeId());

  PrimeFrame::LayoutOutput initialLayout = layoutFrame(initialFrame);
  CHECK(focus.setFocus(initialFrame, initialLayout, initialField.nodeId()));
  CHECK(focus.focusedNode() == initialField.nodeId());

  reconciler.beginRebuild(focus.focusedNode());

  PrimeFrame::Frame rebuiltFrame;
  PrimeStage::UiNode rebuiltRoot = createRoot(rebuiltFrame);
  rebuiltRoot.createButton(makeButtonSpec()); // shifts subsequent node ids
  PrimeStage::UiNode rebuiltPrimary = rebuiltRoot.createButton(makeButtonSpec());
  PrimeStage::TextFieldState rebuiltFieldState;
  rebuiltFieldState.text = "hello";
  PrimeStage::UiNode rebuiltField = rebuiltRoot.createTextField(makeFieldSpec(rebuiltFieldState));

  reconciler.registerNode(IdentityPrimary, rebuiltPrimary.nodeId());
  reconciler.registerNode(IdentityField, rebuiltField.nodeId());

  PrimeFrame::LayoutOutput rebuiltLayout = layoutFrame(rebuiltFrame);
  focus.updateAfterRebuild(rebuiltFrame, rebuiltLayout);
  CHECK(focus.focusedNode() != rebuiltField.nodeId());

  CHECK(reconciler.restoreFocus(focus, rebuiltFrame, rebuiltLayout));
  CHECK(focus.focusedNode() == rebuiltField.nodeId());
}

TEST_CASE("WidgetIdentityReconciler restoreFocus returns false when identity is missing") {
  PrimeStage::WidgetIdentityReconciler reconciler;
  PrimeFrame::FocusManager focus;

  PrimeFrame::Frame initialFrame;
  PrimeStage::UiNode initialRoot = createRoot(initialFrame);
  PrimeStage::TextFieldState initialFieldState;
  initialFieldState.text = "persist";
  PrimeStage::UiNode initialField = initialRoot.createTextField(makeFieldSpec(initialFieldState));

  reconciler.registerNode(IdentityField, initialField.nodeId());

  PrimeFrame::LayoutOutput initialLayout = layoutFrame(initialFrame);
  CHECK(focus.setFocus(initialFrame, initialLayout, initialField.nodeId()));

  reconciler.beginRebuild(focus.focusedNode());

  PrimeFrame::Frame rebuiltFrame;
  PrimeStage::UiNode rebuiltRoot = createRoot(rebuiltFrame);
  PrimeStage::UiNode fallbackNode = rebuiltRoot.createButton(makeButtonSpec());
  reconciler.registerNode(IdentityPrimary, fallbackNode.nodeId());

  PrimeFrame::LayoutOutput rebuiltLayout = layoutFrame(rebuiltFrame);
  focus.updateAfterRebuild(rebuiltFrame, rebuiltLayout);

  CHECK_FALSE(reconciler.restoreFocus(focus, rebuiltFrame, rebuiltLayout));
  CHECK_FALSE(reconciler.restoreFocus(focus, rebuiltFrame, rebuiltLayout));
}

TEST_CASE("WidgetIdentityReconciler findNode resolves current registrations") {
  PrimeStage::WidgetIdentityReconciler reconciler;

  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);
  PrimeStage::UiNode node = root.createButton(makeButtonSpec());
  reconciler.registerNode(IdentityPrimary, node.nodeId());

  CHECK(reconciler.findNode(IdentityPrimary) == node.nodeId());
  CHECK_FALSE(reconciler.findNode(IdentityField).isValid());
}
