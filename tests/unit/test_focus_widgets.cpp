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

PrimeFrame::Event makePointerUp(float x, float y) {
  PrimeFrame::Event event;
  event.type = PrimeFrame::EventType::PointerUp;
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

void clickCenterPressRelease(PrimeFrame::Frame& frame,
                             PrimeFrame::LayoutOutput const& layout,
                             PrimeFrame::EventRouter& router,
                             PrimeFrame::FocusManager& focus,
                             PrimeStage::UiNode node) {
  PrimeFrame::LayoutOut const* out = layout.get(node.nodeId());
  REQUIRE(out != nullptr);
  float centerX = out->absX + out->absW * 0.5f;
  float centerY = out->absY + out->absH * 0.5f;
  router.dispatch(makePointerDown(centerX, centerY), frame, layout, &focus);
  router.dispatch(makePointerUp(centerX, centerY), frame, layout, &focus);
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

std::vector<PrimeFrame::NodeId> childNodes(PrimeFrame::Frame const& frame, PrimeFrame::NodeId nodeId) {
  PrimeFrame::Node const* node = frame.getNode(nodeId);
  if (!node) {
    return {};
  }
  return node->children;
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
      .expectVisibleFocus = true,
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
      .expectVisibleFocus = true,
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
      .expectVisibleFocus = true,
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
      .expectVisibleFocus = true,
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

TEST_CASE("PrimeStage click-to-focus contract covers all focusable widget abstractions") {
  struct ClickFocusCase {
    std::string_view name;
    std::function<PrimeStage::UiNode(PrimeStage::UiNode&)> createWidget;
    std::function<PrimeFrame::NodeId(PrimeFrame::Frame const&, PrimeStage::UiNode)> resolveFocusedNode;
  };

  auto focusSelf = [](PrimeFrame::Frame const&, PrimeStage::UiNode widget) {
    return widget.nodeId();
  };

  PrimeStage::TextFieldState textState;
  textState.text = "focus-check";

  std::vector<ClickFocusCase> cases;
  cases.push_back(ClickFocusCase{
      .name = "button",
      .createWidget =
          [](PrimeStage::UiNode& root) {
            PrimeStage::ButtonSpec spec;
            spec.label = "Button";
            spec.size.preferredWidth = 120.0f;
            spec.size.preferredHeight = 28.0f;
            spec.backgroundStyle = 11u;
            return root.createButton(spec);
          },
      .resolveFocusedNode = focusSelf,
  });

  cases.push_back(ClickFocusCase{
      .name = "text_field",
      .createWidget =
          [&textState](PrimeStage::UiNode& root) {
            PrimeStage::TextFieldSpec spec;
            spec.state = &textState;
            spec.size.preferredWidth = 180.0f;
            spec.size.preferredHeight = 28.0f;
            spec.backgroundStyle = 21u;
            spec.textStyle = 22u;
            return root.createTextField(spec);
          },
      .resolveFocusedNode = focusSelf,
  });

  cases.push_back(ClickFocusCase{
      .name = "toggle",
      .createWidget =
          [](PrimeStage::UiNode& root) {
            PrimeStage::ToggleSpec spec;
            spec.size.preferredWidth = 56.0f;
            spec.size.preferredHeight = 24.0f;
            spec.trackStyle = 31u;
            spec.knobStyle = 32u;
            return root.createToggle(spec);
          },
      .resolveFocusedNode = focusSelf,
  });

  cases.push_back(ClickFocusCase{
      .name = "checkbox",
      .createWidget =
          [](PrimeStage::UiNode& root) {
            PrimeStage::CheckboxSpec spec;
            spec.label = "Check";
            spec.boxStyle = 41u;
            spec.checkStyle = 42u;
            spec.textStyle = 43u;
            spec.size.preferredWidth = 140.0f;
            spec.size.preferredHeight = 26.0f;
            return root.createCheckbox(spec);
          },
      .resolveFocusedNode = focusSelf,
  });

  cases.push_back(ClickFocusCase{
      .name = "slider",
      .createWidget =
          [](PrimeStage::UiNode& root) {
            PrimeStage::SliderSpec spec;
            spec.size.preferredWidth = 180.0f;
            spec.size.preferredHeight = 20.0f;
            spec.trackStyle = 51u;
            spec.fillStyle = 52u;
            spec.thumbStyle = 53u;
            return root.createSlider(spec);
          },
      .resolveFocusedNode = focusSelf,
  });

  cases.push_back(ClickFocusCase{
      .name = "tabs",
      .createWidget =
          [](PrimeStage::UiNode& root) {
            PrimeStage::TabsSpec spec;
            spec.labels = {"Only"};
            spec.size.preferredHeight = 28.0f;
            spec.tabStyle = 61u;
            spec.activeTabStyle = 62u;
            spec.textStyle = 63u;
            spec.activeTextStyle = 64u;
            return root.createTabs(spec);
          },
      .resolveFocusedNode =
          [](PrimeFrame::Frame const& frame, PrimeStage::UiNode tabs) {
            std::vector<PrimeFrame::NodeId> tabsChildren = childNodes(frame, tabs.nodeId());
            REQUIRE(tabsChildren.size() == 1u);
            return tabsChildren.front();
          },
  });

  cases.push_back(ClickFocusCase{
      .name = "dropdown",
      .createWidget =
          [](PrimeStage::UiNode& root) {
            PrimeStage::DropdownSpec spec;
            spec.options = {"One", "Two"};
            spec.size.preferredWidth = 140.0f;
            spec.size.preferredHeight = 24.0f;
            spec.backgroundStyle = 71u;
            spec.textStyle = 72u;
            spec.indicatorStyle = 73u;
            return root.createDropdown(spec);
          },
      .resolveFocusedNode = focusSelf,
  });

  cases.push_back(ClickFocusCase{
      .name = "progress_bar",
      .createWidget =
          [](PrimeStage::UiNode& root) {
            PrimeStage::ProgressBarSpec spec;
            spec.size.preferredWidth = 180.0f;
            spec.size.preferredHeight = 14.0f;
            spec.trackStyle = 81u;
            spec.fillStyle = 82u;
            return root.createProgressBar(spec);
          },
      .resolveFocusedNode = focusSelf,
  });

  cases.push_back(ClickFocusCase{
      .name = "table",
      .createWidget =
          [](PrimeStage::UiNode& root) {
            PrimeStage::TableSpec spec;
            spec.size.preferredWidth = 240.0f;
            spec.size.preferredHeight = 120.0f;
            spec.headerHeight = 20.0f;
            spec.headerStyle = 91u;
            spec.rowStyle = 92u;
            spec.rowAltStyle = 93u;
            spec.columns = {{"A", 100.0f, 0u, 0u}, {"B", 100.0f, 0u, 0u}};
            spec.rows = {{"1", "2"}, {"3", "4"}};
            return root.createTable(spec);
          },
      .resolveFocusedNode = focusSelf,
  });

  cases.push_back(ClickFocusCase{
      .name = "list",
      .createWidget =
          [](PrimeStage::UiNode& root) {
            PrimeStage::ListSpec spec;
            spec.items = {"Alpha", "Beta", "Gamma"};
            spec.size.preferredWidth = 180.0f;
            spec.size.preferredHeight = 120.0f;
            spec.textStyle = 101u;
            spec.rowStyle = 102u;
            spec.rowAltStyle = 103u;
            spec.selectionStyle = 104u;
            spec.dividerStyle = 105u;
            return root.createList(spec);
          },
      .resolveFocusedNode = focusSelf,
  });

  cases.push_back(ClickFocusCase{
      .name = "tree_view",
      .createWidget =
          [](PrimeStage::UiNode& root) {
            PrimeStage::TreeViewSpec spec;
            spec.size.preferredWidth = 240.0f;
            spec.size.preferredHeight = 120.0f;
            spec.rowStyle = 111u;
            spec.rowAltStyle = 112u;
            spec.selectionStyle = 113u;
            spec.selectionAccentStyle = 114u;
            spec.textStyle = 115u;
            spec.selectedTextStyle = 116u;
            spec.nodes = {PrimeStage::TreeNode{"Root"}};
            return root.createTreeView(spec);
          },
      .resolveFocusedNode = focusSelf,
  });

  cases.push_back(ClickFocusCase{
      .name = "window",
      .createWidget =
          [](PrimeStage::UiNode& root) {
            PrimeStage::WindowSpec spec;
            spec.title = "Window";
            spec.width = 220.0f;
            spec.height = 140.0f;
            spec.frameStyle = 121u;
            spec.titleBarStyle = 122u;
            spec.titleTextStyle = 123u;
            spec.contentStyle = 124u;
            PrimeStage::Window window = root.createWindow(spec);
            return window.root;
          },
      .resolveFocusedNode = focusSelf,
  });

  for (ClickFocusCase const& focusCase : cases) {
    INFO(focusCase.name);
    PrimeFrame::Frame frame;
    PrimeStage::UiNode root = createRoot(frame);
    PrimeStage::UiNode widget = focusCase.createWidget(root);
    PrimeFrame::LayoutOutput layout = layoutFrame(frame);
    PrimeFrame::FocusManager focus;
    PrimeFrame::EventRouter router;

    CHECK_FALSE(focus.focusedNode().isValid());
    clickCenterPressRelease(frame, layout, router, focus, widget);

    PrimeFrame::NodeId expectedFocus = focusCase.resolveFocusedNode(frame, widget);
    REQUIRE(expectedFocus.isValid());
    CHECK(focus.focusedNode() == expectedFocus);
  }
}

TEST_CASE("PrimeStage focus visuals have semantic defaults without style opt-in") {
  PrimeStage::TextFieldState textFieldState;
  textFieldState.text = "plain";

  std::vector<FocusCase> focusCases;

  focusCases.push_back(FocusCase{
      .name = "button_default_style",
      .expectVisibleFocus = true,
      .createWidget =
          [](PrimeStage::UiNode& root) {
            PrimeStage::ButtonSpec spec;
            spec.size.preferredWidth = 120.0f;
            spec.size.preferredHeight = 28.0f;
            return root.createButton(spec);
          }});

  focusCases.push_back(FocusCase{
      .name = "text_field_default_style",
      .expectVisibleFocus = true,
      .createWidget =
          [&textFieldState](PrimeStage::UiNode& root) {
            PrimeStage::TextFieldSpec spec;
            spec.state = &textFieldState;
            spec.size.preferredWidth = 180.0f;
            spec.size.preferredHeight = 28.0f;
            return root.createTextField(spec);
          }});

  focusCases.push_back(FocusCase{
      .name = "table_default_style",
      .expectVisibleFocus = true,
      .createWidget =
          [](PrimeStage::UiNode& root) {
            PrimeStage::TableSpec spec;
            spec.size.preferredWidth = 240.0f;
            spec.size.preferredHeight = 120.0f;
            spec.headerHeight = 20.0f;
            spec.columns = {{"A", 100.0f, 0u, 0u}, {"B", 100.0f, 0u, 0u}};
            spec.rows = {{"1", "2"}, {"3", "4"}};
            return root.createTable(spec);
          }});

  focusCases.push_back(FocusCase{
      .name = "tree_view_default_style",
      .expectVisibleFocus = true,
      .createWidget =
          [](PrimeStage::UiNode& root) {
            PrimeStage::TreeViewSpec spec;
            spec.size.preferredWidth = 240.0f;
            spec.size.preferredHeight = 120.0f;
            spec.nodes = {PrimeStage::TreeNode{"Node"}};
            return root.createTreeView(spec);
          }});

  for (FocusCase const& focusCase : focusCases) {
    INFO(focusCase.name);
    runFocusCase(focusCase);
  }
}

TEST_CASE("PrimeStage tabIndex controls deterministic mixed-widget tab order") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::StackSpec stackSpec;
  stackSpec.gap = 8.0f;
  stackSpec.size.stretchX = 1.0f;
  stackSpec.size.stretchY = 1.0f;
  PrimeStage::UiNode stack = root.createVerticalStack(stackSpec);

  PrimeStage::TextFieldState textState;
  textState.text = "focus-order";

  PrimeStage::ButtonSpec buttonSpec;
  buttonSpec.label = "Button";
  buttonSpec.tabIndex = 40;
  buttonSpec.size.preferredWidth = 120.0f;
  buttonSpec.size.preferredHeight = 28.0f;
  PrimeStage::UiNode button = stack.createButton(buttonSpec);

  PrimeStage::SliderSpec sliderSpec;
  sliderSpec.tabIndex = 20;
  sliderSpec.size.preferredWidth = 180.0f;
  sliderSpec.size.preferredHeight = 20.0f;
  sliderSpec.trackStyle = 101u;
  sliderSpec.fillStyle = 102u;
  sliderSpec.thumbStyle = 103u;
  PrimeStage::UiNode slider = stack.createSlider(sliderSpec);

  PrimeStage::ToggleSpec toggleSpec;
  toggleSpec.tabIndex = 30;
  toggleSpec.size.preferredWidth = 56.0f;
  toggleSpec.size.preferredHeight = 24.0f;
  toggleSpec.trackStyle = 201u;
  toggleSpec.knobStyle = 202u;
  PrimeStage::UiNode toggle = stack.createToggle(toggleSpec);

  PrimeStage::TextFieldSpec fieldSpec;
  fieldSpec.state = &textState;
  fieldSpec.tabIndex = 10;
  fieldSpec.size.preferredWidth = 180.0f;
  fieldSpec.size.preferredHeight = 28.0f;
  PrimeStage::UiNode field = stack.createTextField(fieldSpec);

  PrimeStage::DropdownSpec dropdownSpec;
  dropdownSpec.tabIndex = 50;
  dropdownSpec.options = {"One", "Two", "Three"};
  dropdownSpec.size.preferredWidth = 120.0f;
  dropdownSpec.size.preferredHeight = 28.0f;
  PrimeStage::UiNode dropdown = stack.createDropdown(dropdownSpec);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame);
  PrimeFrame::FocusManager focus;

  std::vector<PrimeFrame::NodeId> expected{
      field.nodeId(), slider.nodeId(), toggle.nodeId(), button.nodeId(), dropdown.nodeId()};
  for (PrimeFrame::NodeId nodeId : expected) {
    CHECK(focus.handleTab(frame, layout, true));
    CHECK(focus.focusedNode() == nodeId);
  }

  CHECK(focus.handleTab(frame, layout, true));
  CHECK(focus.focusedNode() == expected.front());
}

TEST_CASE("PrimeStage tabs tabIndex seeds sequential tab focus order") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::StackSpec stackSpec;
  stackSpec.gap = 8.0f;
  stackSpec.size.stretchX = 1.0f;
  stackSpec.size.stretchY = 1.0f;
  PrimeStage::UiNode stack = root.createVerticalStack(stackSpec);

  PrimeStage::TabsSpec tabsSpec;
  tabsSpec.labels = {"Overview", "Assets", "Settings"};
  tabsSpec.tabIndex = 5;
  tabsSpec.size.preferredHeight = 28.0f;
  tabsSpec.tabStyle = 301u;
  tabsSpec.activeTabStyle = 302u;
  PrimeStage::UiNode tabs = stack.createTabs(tabsSpec);

  PrimeStage::ButtonSpec buttonSpec;
  buttonSpec.label = "Apply";
  buttonSpec.tabIndex = 20;
  buttonSpec.size.preferredWidth = 120.0f;
  buttonSpec.size.preferredHeight = 28.0f;
  PrimeStage::UiNode button = stack.createButton(buttonSpec);

  std::vector<PrimeFrame::NodeId> tabsChildren = childNodes(frame, tabs.nodeId());
  REQUIRE(tabsChildren.size() == 3u);
  PrimeFrame::Node const* tab0 = frame.getNode(tabsChildren[0]);
  PrimeFrame::Node const* tab1 = frame.getNode(tabsChildren[1]);
  PrimeFrame::Node const* tab2 = frame.getNode(tabsChildren[2]);
  REQUIRE(tab0 != nullptr);
  REQUIRE(tab1 != nullptr);
  REQUIRE(tab2 != nullptr);
  CHECK(tab0->tabIndex == 5);
  CHECK(tab1->tabIndex == 6);
  CHECK(tab2->tabIndex == 7);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame);
  PrimeFrame::FocusManager focus;
  CHECK(focus.handleTab(frame, layout, true));
  CHECK(focus.focusedNode() == tabsChildren[0]);
  CHECK(focus.handleTab(frame, layout, true));
  CHECK(focus.focusedNode() == tabsChildren[1]);
  CHECK(focus.handleTab(frame, layout, true));
  CHECK(focus.focusedNode() == tabsChildren[2]);
  CHECK(focus.handleTab(frame, layout, true));
  CHECK(focus.focusedNode() == button.nodeId());
}

TEST_CASE("PrimeStage tabIndex values below -1 clamp to auto mode") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::TextFieldState textState;
  textState.text = "clamp";

  PrimeStage::ButtonSpec buttonSpec;
  buttonSpec.label = "Clamp";
  buttonSpec.tabIndex = -9;
  buttonSpec.size.preferredWidth = 100.0f;
  buttonSpec.size.preferredHeight = 24.0f;
  PrimeStage::UiNode button = root.createButton(buttonSpec);

  PrimeStage::TextFieldSpec fieldSpec;
  fieldSpec.state = &textState;
  fieldSpec.tabIndex = -4;
  fieldSpec.size.preferredWidth = 160.0f;
  fieldSpec.size.preferredHeight = 24.0f;
  PrimeStage::UiNode field = root.createTextField(fieldSpec);

  PrimeFrame::Node const* buttonNode = frame.getNode(button.nodeId());
  PrimeFrame::Node const* fieldNode = frame.getNode(field.nodeId());
  REQUIRE(buttonNode != nullptr);
  REQUIRE(fieldNode != nullptr);
  CHECK(buttonNode->tabIndex == -1);
  CHECK(fieldNode->tabIndex == -1);
}
