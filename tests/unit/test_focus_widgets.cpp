#include "PrimeFrame/Events.h"
#include "PrimeFrame/Focus.h"
#include "PrimeFrame/Frame.h"
#include "PrimeFrame/Layout.h"
#include "PrimeStage/Ui.h"

#include "third_party/doctest.h"

#include <functional>
#include <string_view>
#include <vector>

namespace {

constexpr float RootWidth = 360.0f;
constexpr float RootHeight = 240.0f;

PrimeStage::UiNode createRoot(PrimeFrame::Frame& frame) {
  PrimeFrame::NodeId rootId = frame.createNode();
  frame.addRoot(rootId);
  if (PrimeFrame::Node* node = frame.getNode(rootId)) {
    node->layout = PrimeFrame::LayoutType::Overlay;
    node->sizeHint.width.preferred = RootWidth;
    node->sizeHint.height.preferred = RootHeight;
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

PrimeFrame::Event makePointerDown(float x, float y) {
  PrimeFrame::Event event;
  event.type = PrimeFrame::EventType::PointerDown;
  event.pointerId = 1;
  event.x = x;
  event.y = y;
  return event;
}

void clickCenter(PrimeFrame::Frame& frame,
                 PrimeFrame::LayoutOutput const& layout,
                 PrimeFrame::EventRouter& router,
                 PrimeFrame::FocusManager& focus,
                 PrimeStage::UiNode node) {
  PrimeFrame::LayoutOut const* out = layout.get(node.nodeId());
  REQUIRE(out != nullptr);
  float centerX = out->absX + out->absW * 0.5f;
  float centerY = out->absY + out->absH * 0.5f;
  router.dispatch(makePointerDown(centerX, centerY), frame, layout, &focus);
}

struct FocusCase {
  std::string_view name;
  bool expectFocusable = true;
  bool expectClickFocus = true;
  bool expectTabFocus = true;
  bool expectVisibleFocus = false;
  std::function<PrimeStage::UiNode(PrimeStage::UiNode&)> createWidget;
};

struct PrimitiveVisualState {
  PrimeFrame::PrimitiveType type = PrimeFrame::PrimitiveType::Rect;
  PrimeFrame::RectStyleToken rectToken = 0;
  std::optional<float> rectOpacity;
};

void collectVisualState(PrimeFrame::Frame const& frame,
                        PrimeFrame::NodeId nodeId,
                        std::vector<PrimitiveVisualState>& out) {
  PrimeFrame::Node const* node = frame.getNode(nodeId);
  if (!node) {
    return;
  }
  for (PrimeFrame::PrimitiveId primitiveId : node->primitives) {
    PrimeFrame::Primitive const* primitive = frame.getPrimitive(primitiveId);
    if (!primitive) {
      continue;
    }
    PrimitiveVisualState state;
    state.type = primitive->type;
    if (primitive->type == PrimeFrame::PrimitiveType::Rect) {
      state.rectToken = primitive->rect.token;
      state.rectOpacity = primitive->rect.overrideStyle.opacity;
    }
    out.push_back(state);
  }
  for (PrimeFrame::NodeId child : node->children) {
    collectVisualState(frame, child, out);
  }
}

std::vector<PrimitiveVisualState> captureWidgetVisualState(PrimeFrame::Frame const& frame,
                                                           PrimeFrame::NodeId widgetNodeId) {
  std::vector<PrimitiveVisualState> state;
  collectVisualState(frame, widgetNodeId, state);
  return state;
}

bool hasVisualDifference(std::vector<PrimitiveVisualState> const& before,
                         std::vector<PrimitiveVisualState> const& after) {
  if (before.size() != after.size()) {
    return true;
  }
  for (size_t i = 0; i < before.size(); ++i) {
    PrimitiveVisualState const& lhs = before[i];
    PrimitiveVisualState const& rhs = after[i];
    if (lhs.type != rhs.type) {
      return true;
    }
    if (lhs.rectToken != rhs.rectToken) {
      return true;
    }
    if (lhs.rectOpacity != rhs.rectOpacity) {
      return true;
    }
  }
  return false;
}

void runFocusCase(FocusCase const& focusCase) {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);
  PrimeStage::UiNode widget = focusCase.createWidget(root);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame);
  PrimeFrame::Node const* node = frame.getNode(widget.nodeId());
  REQUIRE(node != nullptr);

  CHECK_MESSAGE(node->focusable == focusCase.expectFocusable,
                focusCase.name << ": unexpected node->focusable value");

  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;

  clickCenter(frame, layout, router, focus, widget);
  if (focusCase.expectClickFocus) {
    CHECK_MESSAGE(focus.focusedNode() == widget.nodeId(),
                  focusCase.name << ": click did not focus widget");
  } else {
    CHECK_MESSAGE(!focus.focusedNode().isValid(),
                  focusCase.name << ": click unexpectedly focused a node");
  }

  focus.clearFocus(frame);
  std::vector<PrimitiveVisualState> beforeFocus = captureWidgetVisualState(frame, widget.nodeId());
  bool tabHandled = focus.handleTab(frame, layout, true);
  if (focusCase.expectTabFocus) {
    CHECK_MESSAGE(tabHandled, focusCase.name << ": tab did not find focus target");
    CHECK_MESSAGE(focus.focusedNode() == widget.nodeId(),
                  focusCase.name << ": tab focused unexpected widget");
    if (focusCase.expectVisibleFocus) {
      std::vector<PrimitiveVisualState> afterFocus = captureWidgetVisualState(frame, widget.nodeId());
      CHECK_MESSAGE(hasVisualDifference(beforeFocus, afterFocus),
                    focusCase.name << ": focus did not produce a visible state change");
    }
  } else {
    CHECK_MESSAGE(!tabHandled, focusCase.name << ": tab unexpectedly found focus target");
    CHECK_MESSAGE(!focus.focusedNode().isValid(),
                  focusCase.name << ": tab unexpectedly focused a node");
  }
}

} // namespace

TEST_CASE("PrimeStage focus contract for interactive widgets") {
  PrimeStage::TextFieldState textFieldState;
  textFieldState.text = "hello";
  PrimeStage::SelectableTextState selectableState;

  std::vector<FocusCase> focusCases;

  focusCases.push_back(FocusCase{
      .name = "button",
      .createWidget =
          [](PrimeStage::UiNode& root) {
            PrimeStage::ButtonSpec spec;
            spec.size.preferredWidth = 120.0f;
            spec.size.preferredHeight = 28.0f;
            spec.backgroundStyle = 101u;
            return root.createButton(spec);
          }});

  focusCases.push_back(FocusCase{
      .name = "text_field",
      .createWidget =
          [&textFieldState](PrimeStage::UiNode& root) {
            PrimeStage::TextFieldSpec spec;
            spec.state = &textFieldState;
            spec.size.preferredWidth = 180.0f;
            spec.size.preferredHeight = 28.0f;
            spec.backgroundStyle = 201u;
            spec.textStyle = 301u;
            return root.createTextField(spec);
          }});

  focusCases.push_back(FocusCase{
      .name = "toggle",
      .expectVisibleFocus = true,
      .createWidget =
          [](PrimeStage::UiNode& root) {
            PrimeStage::ToggleSpec spec;
            spec.size.preferredWidth = 56.0f;
            spec.size.preferredHeight = 28.0f;
            spec.trackStyle = 401u;
            spec.knobStyle = 402u;
            return root.createToggle(spec);
          }});

  focusCases.push_back(FocusCase{
      .name = "checkbox",
      .expectVisibleFocus = true,
      .createWidget =
          [](PrimeStage::UiNode& root) {
            PrimeStage::CheckboxSpec spec;
            spec.label = "check";
            spec.boxStyle = 451u;
            spec.checkStyle = 452u;
            spec.textStyle = 453u;
            return root.createCheckbox(spec);
          }});

  focusCases.push_back(FocusCase{
      .name = "slider",
      .expectVisibleFocus = true,
      .createWidget =
          [](PrimeStage::UiNode& root) {
            PrimeStage::SliderSpec spec;
            spec.size.preferredWidth = 180.0f;
            spec.size.preferredHeight = 20.0f;
            spec.trackStyle = 501u;
            spec.fillStyle = 502u;
            spec.thumbStyle = 503u;
            return root.createSlider(spec);
          }});

  focusCases.push_back(FocusCase{
      .name = "progress_bar",
      .expectVisibleFocus = true,
      .createWidget =
          [](PrimeStage::UiNode& root) {
            PrimeStage::ProgressBarSpec spec;
            spec.size.preferredWidth = 180.0f;
            spec.size.preferredHeight = 14.0f;
            spec.value = 0.5f;
            spec.trackStyle = 601u;
            spec.fillStyle = 602u;
            return root.createProgressBar(spec);
          }});

  focusCases.push_back(FocusCase{
      .name = "table",
      .createWidget =
          [](PrimeStage::UiNode& root) {
            PrimeStage::TableSpec spec;
            spec.size.preferredWidth = 240.0f;
            spec.size.preferredHeight = 120.0f;
            spec.headerHeight = 20.0f;
            spec.headerStyle = 701u;
            spec.rowStyle = 702u;
            spec.rowAltStyle = 703u;
            spec.columns = {{"A", 100.0f, 711u, 712u}, {"B", 100.0f, 711u, 712u}};
            spec.rows = {{"1", "2"}, {"3", "4"}};
            return root.createTable(spec);
          }});

  focusCases.push_back(FocusCase{
      .name = "tree_view",
      .createWidget =
          [](PrimeStage::UiNode& root) {
            PrimeStage::TreeViewSpec spec;
            spec.size.preferredWidth = 240.0f;
            spec.size.preferredHeight = 120.0f;
            spec.rowStyle = 801u;
            spec.rowAltStyle = 802u;
            spec.selectionStyle = 803u;
            spec.selectionAccentStyle = 804u;
            spec.textStyle = 805u;
            spec.selectedTextStyle = 806u;
            spec.nodes = {PrimeStage::TreeNode{"Node"}};
            return root.createTreeView(spec);
          }});

  focusCases.push_back(FocusCase{
      .name = "selectable_text",
      .expectFocusable = false,
      .expectClickFocus = false,
      .expectTabFocus = false,
      .createWidget =
          [&selectableState](PrimeStage::UiNode& root) {
            PrimeStage::SelectableTextSpec spec;
            spec.state = &selectableState;
            spec.text = "selectable";
            spec.textStyle = 901u;
            spec.selectionStyle = 902u;
            spec.size.preferredWidth = 180.0f;
            spec.size.preferredHeight = 30.0f;
            return root.createSelectableText(spec);
          }});

  for (FocusCase const& focusCase : focusCases) {
    INFO(focusCase.name);
    runFocusCase(focusCase);
  }
}
