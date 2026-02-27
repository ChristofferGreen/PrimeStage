#include "PrimeStage/Ui.h"
#include "src/PrimeStageCollectionInternals.h"

#include "PrimeFrame/Events.h"
#include "PrimeFrame/Focus.h"
#include "PrimeFrame/Frame.h"
#include "PrimeFrame/Layout.h"

#include "third_party/doctest.h"

#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

PrimeStage::UiNode createRoot(PrimeFrame::Frame& frame) {
  PrimeFrame::NodeId rootId = frame.createNode();
  frame.addRoot(rootId);
  PrimeFrame::Node* node = frame.getNode(rootId);
  if (node) {
    node->layout = PrimeFrame::LayoutType::Overlay;
    node->sizeHint.width.preferred = 640.0f;
    node->sizeHint.height.preferred = 360.0f;
  }
  return PrimeStage::UiNode(frame, rootId, true);
}

PrimeFrame::LayoutOutput layoutFrame(PrimeFrame::Frame& frame) {
  PrimeFrame::LayoutOutput output;
  PrimeFrame::LayoutEngine engine;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 640.0f;
  options.rootHeight = 360.0f;
  engine.layout(frame, output, options);
  return output;
}

PrimeFrame::Event makePointerEvent(PrimeFrame::EventType type, int pointerId, float x, float y) {
  PrimeFrame::Event event;
  event.type = type;
  event.pointerId = pointerId;
  event.x = x;
  event.y = y;
  return event;
}

PrimeFrame::RectStyleToken firstRectToken(PrimeFrame::Frame const& frame, PrimeFrame::NodeId nodeId) {
  PrimeFrame::Node const* node = frame.getNode(nodeId);
  if (!node) {
    return 0;
  }
  for (PrimeFrame::PrimitiveId primId : node->primitives) {
    PrimeFrame::Primitive const* prim = frame.getPrimitive(primId);
    if (prim && prim->type == PrimeFrame::PrimitiveType::Rect) {
      return prim->rect.token;
    }
  }
  return 0;
}

PrimeFrame::Primitive const* firstTextPrimitive(PrimeFrame::Frame const& frame,
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

std::string firstChildText(PrimeFrame::Frame const& frame, PrimeFrame::NodeId parent) {
  PrimeFrame::Node const* parentNode = frame.getNode(parent);
  if (!parentNode) {
    return {};
  }
  for (PrimeFrame::NodeId childId : parentNode->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (!child) {
      continue;
    }
    for (PrimeFrame::PrimitiveId primId : child->primitives) {
      PrimeFrame::Primitive const* prim = frame.getPrimitive(primId);
      if (prim && prim->type == PrimeFrame::PrimitiveType::Text) {
        return prim->textBlock.text;
      }
    }
  }
  return {};
}

PrimeFrame::Primitive const* findRectPrimitiveByToken(PrimeFrame::Frame const& frame,
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
    PrimeFrame::Primitive const* child = findRectPrimitiveByToken(frame, childId, token);
    if (child) {
      return child;
    }
  }
  return nullptr;
}

size_t countRectToken(PrimeFrame::Frame const& frame,
                      PrimeFrame::NodeId nodeId,
                      PrimeFrame::RectStyleToken token) {
  PrimeFrame::Node const* node = frame.getNode(nodeId);
  if (!node) {
    return 0u;
  }
  size_t count = 0u;
  for (PrimeFrame::PrimitiveId primId : node->primitives) {
    PrimeFrame::Primitive const* prim = frame.getPrimitive(primId);
    if (prim && prim->type == PrimeFrame::PrimitiveType::Rect && prim->rect.token == token) {
      ++count;
    }
  }
  for (PrimeFrame::NodeId childId : node->children) {
    count += countRectToken(frame, childId, token);
  }
  return count;
}

size_t countTextValue(PrimeFrame::Frame const& frame,
                      PrimeFrame::NodeId nodeId,
                      std::string_view text) {
  PrimeFrame::Node const* node = frame.getNode(nodeId);
  if (!node) {
    return 0u;
  }
  size_t count = 0u;
  for (PrimeFrame::PrimitiveId primId : node->primitives) {
    PrimeFrame::Primitive const* prim = frame.getPrimitive(primId);
    if (prim && prim->type == PrimeFrame::PrimitiveType::Text &&
        prim->textBlock.text == text) {
      ++count;
    }
  }
  for (PrimeFrame::NodeId childId : node->children) {
    count += countTextValue(frame, childId, text);
  }
  return count;
}

std::string boolString(bool value) {
  return value ? "true" : "false";
}

std::string optionalBoolString(std::optional<bool> const& value) {
  if (!value.has_value()) {
    return "-";
  }
  return value.value() ? "true" : "false";
}

std::string optionalIntString(std::optional<int> const& value) {
  if (!value.has_value()) {
    return "-";
  }
  return std::to_string(value.value());
}

std::string optionalFloatString(std::optional<float> const& value) {
  if (!value.has_value()) {
    return "-";
  }
  std::ostringstream out;
  out << std::fixed << std::setprecision(2) << value.value();
  return out.str();
}

std::string roleString(PrimeStage::AccessibilityRole role) {
  switch (role) {
    case PrimeStage::AccessibilityRole::Unspecified:
      return "unspecified";
    case PrimeStage::AccessibilityRole::Group:
      return "group";
    case PrimeStage::AccessibilityRole::StaticText:
      return "static_text";
    case PrimeStage::AccessibilityRole::Button:
      return "button";
    case PrimeStage::AccessibilityRole::TextField:
      return "text_field";
    case PrimeStage::AccessibilityRole::Toggle:
      return "toggle";
    case PrimeStage::AccessibilityRole::Checkbox:
      return "checkbox";
    case PrimeStage::AccessibilityRole::Slider:
      return "slider";
    case PrimeStage::AccessibilityRole::TabList:
      return "tab_list";
    case PrimeStage::AccessibilityRole::Tab:
      return "tab";
    case PrimeStage::AccessibilityRole::ComboBox:
      return "combo_box";
    case PrimeStage::AccessibilityRole::ProgressBar:
      return "progress_bar";
    case PrimeStage::AccessibilityRole::Table:
      return "table";
    case PrimeStage::AccessibilityRole::Tree:
      return "tree";
    case PrimeStage::AccessibilityRole::TreeItem:
      return "tree_item";
  }
  return "unknown";
}

std::string exportSemanticsRow(std::string_view name,
                               PrimeStage::AccessibilitySemantics const& semantics) {
  std::ostringstream out;
  out << name;
  out << " role=" << roleString(semantics.role);
  out << " disabled=" << boolString(semantics.state.disabled);
  out << " checked=" << optionalBoolString(semantics.state.checked);
  out << " selected=" << optionalBoolString(semantics.state.selected);
  out << " expanded=" << optionalBoolString(semantics.state.expanded);
  out << " valueNow=" << optionalFloatString(semantics.state.valueNow);
  out << " valueMin=" << optionalFloatString(semantics.state.valueMin);
  out << " valueMax=" << optionalFloatString(semantics.state.valueMax);
  out << " position=" << optionalIntString(semantics.state.positionInSet);
  out << " setSize=" << optionalIntString(semantics.state.setSize);
  return out.str();
}

std::string exportFocusRow(std::string_view name,
                           PrimeStage::AccessibilitySemantics const& semantics,
                           bool focused) {
  std::ostringstream out;
  out << name;
  out << " role=" << roleString(semantics.role);
  out << " focused=" << boolString(focused);
  out << " disabled=" << boolString(semantics.state.disabled);
  return out.str();
}

} // namespace

TEST_CASE("PrimeStage size validation clamps invalid ranges and negative values") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::PanelSpec panel;
  panel.rectStyle = 100u;
  panel.size.minWidth = 80.0f;
  panel.size.maxWidth = 40.0f;
  panel.size.preferredWidth = 12.0f;
  panel.size.stretchX = -1.0f;
  panel.size.minHeight = -10.0f;
  panel.size.maxHeight = 20.0f;
  panel.size.preferredHeight = 50.0f;
  panel.size.stretchY = -2.0f;
  panel.padding.left = -4.0f;
  panel.padding.top = -3.0f;
  panel.gap = -5.0f;

  PrimeStage::UiNode node = root.createPanel(panel);
  PrimeFrame::Node const* panelNode = frame.getNode(node.nodeId());
  REQUIRE(panelNode != nullptr);
  REQUIRE(panelNode->sizeHint.width.min.has_value());
  REQUIRE(panelNode->sizeHint.width.max.has_value());
  REQUIRE(panelNode->sizeHint.width.preferred.has_value());
  REQUIRE(panelNode->sizeHint.height.min.has_value());
  REQUIRE(panelNode->sizeHint.height.max.has_value());
  REQUIRE(panelNode->sizeHint.height.preferred.has_value());

  CHECK(panelNode->sizeHint.width.min.value() == doctest::Approx(80.0f));
  CHECK(panelNode->sizeHint.width.max.value() == doctest::Approx(80.0f));
  CHECK(panelNode->sizeHint.width.preferred.value() == doctest::Approx(80.0f));
  CHECK(panelNode->sizeHint.width.stretch == doctest::Approx(0.0f));

  CHECK(panelNode->sizeHint.height.min.value() == doctest::Approx(0.0f));
  CHECK(panelNode->sizeHint.height.max.value() == doctest::Approx(20.0f));
  CHECK(panelNode->sizeHint.height.preferred.value() == doctest::Approx(20.0f));
  CHECK(panelNode->sizeHint.height.stretch == doctest::Approx(0.0f));

  CHECK(panelNode->padding.left == doctest::Approx(0.0f));
  CHECK(panelNode->padding.top == doctest::Approx(0.0f));
  CHECK(panelNode->gap == doctest::Approx(0.0f));
}

TEST_CASE("PrimeStage helper widgets clamp invalid helper spec inputs") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::LabelSpec label;
  label.text = "Helper label";
  label.maxWidth = -120.0f;
  label.size.preferredWidth = -60.0f;
  label.size.preferredHeight = -24.0f;
  label.size.stretchX = -1.0f;
  label.size.stretchY = -2.0f;

  PrimeStage::UiNode labelNode = root.createLabel(label);
  PrimeFrame::Node const* labelFrameNode = frame.getNode(labelNode.nodeId());
  REQUIRE(labelFrameNode != nullptr);
  REQUIRE(labelFrameNode->sizeHint.width.preferred.has_value());
  REQUIRE(labelFrameNode->sizeHint.height.preferred.has_value());
  CHECK(labelFrameNode->sizeHint.width.preferred.value() == doctest::Approx(0.0f));
  CHECK(labelFrameNode->sizeHint.height.preferred.value() == doctest::Approx(0.0f));
  CHECK(labelFrameNode->sizeHint.width.stretch == doctest::Approx(0.0f));
  CHECK(labelFrameNode->sizeHint.height.stretch == doctest::Approx(0.0f));

  PrimeFrame::Primitive const* labelText = firstTextPrimitive(frame, labelNode.nodeId());
  REQUIRE(labelText != nullptr);
  CHECK(labelText->textBlock.maxWidth == doctest::Approx(0.0f));

  PrimeStage::DividerSpec divider;
  divider.size.preferredWidth = -20.0f;
  divider.size.preferredHeight = -4.0f;
  PrimeStage::UiNode dividerNode = root.createDivider(divider);
  PrimeFrame::Node const* dividerFrameNode = frame.getNode(dividerNode.nodeId());
  REQUIRE(dividerFrameNode != nullptr);
  REQUIRE(dividerFrameNode->sizeHint.width.preferred.has_value());
  REQUIRE(dividerFrameNode->sizeHint.height.preferred.has_value());
  CHECK(dividerFrameNode->sizeHint.width.preferred.value() == doctest::Approx(0.0f));
  CHECK(dividerFrameNode->sizeHint.height.preferred.value() == doctest::Approx(0.0f));
  CHECK_FALSE(dividerFrameNode->hitTestVisible);

  PrimeStage::SpacerSpec spacer;
  spacer.size.preferredWidth = -18.0f;
  spacer.size.preferredHeight = -8.0f;
  PrimeStage::UiNode spacerNode = root.createSpacer(spacer);
  PrimeFrame::Node const* spacerFrameNode = frame.getNode(spacerNode.nodeId());
  REQUIRE(spacerFrameNode != nullptr);
  REQUIRE(spacerFrameNode->sizeHint.width.preferred.has_value());
  REQUIRE(spacerFrameNode->sizeHint.height.preferred.has_value());
  CHECK(spacerFrameNode->sizeHint.width.preferred.value() == doctest::Approx(0.0f));
  CHECK(spacerFrameNode->sizeHint.height.preferred.value() == doctest::Approx(0.0f));
  CHECK_FALSE(spacerFrameNode->hitTestVisible);
}

TEST_CASE("PrimeStage interactive helper overloads build expected widgets") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::SizeSpec buttonSize;
  buttonSize.preferredWidth = 120.0f;
  buttonSize.preferredHeight = 28.0f;
  PrimeStage::UiNode button = root.createButton("Apply", 601u, 602u, buttonSize);
  PrimeFrame::Node const* buttonNode = frame.getNode(button.nodeId());
  REQUIRE(buttonNode != nullptr);
  CHECK(firstRectToken(frame, button.nodeId()) == 601u);
  CHECK(firstChildText(frame, button.nodeId()) == "Apply");
  CHECK(buttonNode->focusable);

  PrimeStage::TextFieldState fieldState;
  fieldState.text = "Prime";
  fieldState.cursor = static_cast<uint32_t>(fieldState.text.size());
  PrimeStage::SizeSpec fieldSize;
  fieldSize.preferredWidth = 180.0f;
  fieldSize.preferredHeight = 24.0f;
  PrimeStage::UiNode field = root.createTextField(fieldState, "Name", 611u, 612u, fieldSize);
  PrimeFrame::Node const* fieldNode = frame.getNode(field.nodeId());
  REQUIRE(fieldNode != nullptr);
  CHECK(firstRectToken(frame, field.nodeId()) == 611u);
  CHECK(firstChildText(frame, field.nodeId()) == "Prime");
  CHECK(fieldNode->focusable);

  PrimeStage::SizeSpec toggleSize;
  toggleSize.preferredWidth = 48.0f;
  toggleSize.preferredHeight = 24.0f;
  PrimeStage::UiNode toggle = root.createToggle(true, 621u, 622u, toggleSize);
  CHECK(firstRectToken(frame, toggle.nodeId()) == 621u);
  CHECK(findRectPrimitiveByToken(frame, toggle.nodeId(), 622u) != nullptr);

  PrimeStage::SizeSpec checkboxSize;
  checkboxSize.preferredWidth = 180.0f;
  checkboxSize.preferredHeight = 24.0f;
  PrimeStage::UiNode checkbox =
      root.createCheckbox("Enable", true, 631u, 632u, 633u, checkboxSize);
  CHECK(findRectPrimitiveByToken(frame, checkbox.nodeId(), 631u) != nullptr);
  CHECK(findRectPrimitiveByToken(frame, checkbox.nodeId(), 632u) != nullptr);
  CHECK(firstChildText(frame, checkbox.nodeId()) == "Enable");

  PrimeStage::SizeSpec sliderSize;
  sliderSize.preferredWidth = 120.0f;
  sliderSize.preferredHeight = 20.0f;
  PrimeStage::UiNode slider =
      root.createSlider(2.0f, false, 641u, 642u, 643u, sliderSize);
  CHECK(firstRectToken(frame, slider.nodeId()) == 641u);
  PrimeFrame::Primitive const* fill = findRectPrimitiveByToken(frame, slider.nodeId(), 642u);
  PrimeFrame::Primitive const* thumb = findRectPrimitiveByToken(frame, slider.nodeId(), 643u);
  REQUIRE(fill != nullptr);
  REQUIRE(thumb != nullptr);
  CHECK(fill->width == doctest::Approx(120.0f));
}

TEST_CASE("PrimeStage collection helpers and list adapter build expected widgets") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::SizeSpec scrollSize;
  scrollSize.preferredWidth = 220.0f;
  scrollSize.preferredHeight = 120.0f;
  PrimeStage::ScrollView scrollView = root.createScrollView(scrollSize, true, false);
  CHECK(frame.getNode(scrollView.root.nodeId()) != nullptr);
  CHECK(frame.getNode(scrollView.content.nodeId()) != nullptr);

  std::vector<PrimeStage::TableColumn> columns;
  columns.push_back({"Name", 0.0f, 701u, 702u});
  std::vector<std::vector<std::string_view>> rows;
  rows.push_back({"Alpha"});
  rows.push_back({"Beta"});
  PrimeStage::SizeSpec tableSize;
  tableSize.preferredWidth = 240.0f;
  tableSize.preferredHeight = 120.0f;
  PrimeStage::UiNode table = root.createTable(std::move(columns), std::move(rows), 0, tableSize);
  PrimeFrame::Node const* tableNode = frame.getNode(table.nodeId());
  REQUIRE(tableNode != nullptr);
  CHECK(countTextValue(frame, table.nodeId(), "Alpha") == 1u);
  CHECK(countTextValue(frame, table.nodeId(), "Beta") == 1u);

  std::vector<PrimeStage::TreeNode> nodes;
  nodes.push_back(PrimeStage::TreeNode{"Root", {}, true, false});
  PrimeStage::SizeSpec treeSize;
  treeSize.preferredWidth = 220.0f;
  treeSize.preferredHeight = 140.0f;
  PrimeStage::UiNode tree = root.createTreeView(std::move(nodes), treeSize);
  CHECK(frame.getNode(tree.nodeId()) != nullptr);
  CHECK(countTextValue(frame, tree.nodeId(), "Root") == 1u);

  PrimeStage::ListSpec invalidList;
  invalidList.items = {"One", "Two"};
  invalidList.selectedIndex = 99;
  invalidList.selectionStyle = 711u;
  invalidList.focusStyle = 714u;
  invalidList.rowStyle = 712u;
  invalidList.rowAltStyle = 713u;
  invalidList.size.preferredWidth = 220.0f;
  invalidList.size.preferredHeight = 100.0f;
  PrimeStage::UiNode invalidListNode = root.createList(invalidList);
  CHECK(countRectToken(frame, invalidListNode.nodeId(), invalidList.selectionStyle) == 0u);

  int clickedRow = -1;
  std::string clickedItem;
  PrimeStage::ListSpec listSpec;
  listSpec.items = {"One", "Two"};
  listSpec.selectedIndex = 1;
  listSpec.selectionStyle = 721u;
  listSpec.focusStyle = 724u;
  listSpec.rowStyle = 722u;
  listSpec.rowAltStyle = 723u;
  listSpec.size.preferredWidth = 220.0f;
  listSpec.size.preferredHeight = 100.0f;
  listSpec.callbacks.onSelected = [&](PrimeStage::ListRowInfo const& info) {
    clickedRow = info.rowIndex;
    clickedItem = std::string(info.item);
  };
  PrimeStage::UiNode list = root.createList(listSpec);
  CHECK(countRectToken(frame, list.nodeId(), listSpec.selectionStyle) == 1u);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame);
  PrimeFrame::LayoutOut const* listOut = layout.get(list.nodeId());
  REQUIRE(listOut != nullptr);
  float clickX = listOut->absX + listOut->absW * 0.5f;
  float clickY = listOut->absY + listSpec.rowHeight + listSpec.rowGap + listSpec.rowHeight * 0.5f;
  PrimeFrame::EventRouter router;
  PrimeFrame::FocusManager focus;
  router.dispatch(makePointerEvent(PrimeFrame::EventType::PointerDown, 1, clickX, clickY),
                  frame,
                  layout,
                  &focus);
  CHECK(clickedRow == 1);
  CHECK(clickedItem == "Two");
}

TEST_CASE("PrimeStage tabs clamp invalid selected index") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::TabsSpec spec;
  spec.labels = {"One", "Two", "Three"};
  spec.selectedIndex = 999;
  spec.tabStyle = 201u;
  spec.activeTabStyle = 202u;
  spec.gap = -2.0f;
  spec.tabPaddingX = -8.0f;
  spec.tabPaddingY = -4.0f;
  spec.size.preferredHeight = 24.0f;

  PrimeStage::UiNode tabs = root.createTabs(spec);
  PrimeFrame::Node const* row = frame.getNode(tabs.nodeId());
  REQUIRE(row != nullptr);
  REQUIRE(row->children.size() == 3u);

  CHECK(firstRectToken(frame, row->children[0]) == spec.tabStyle);
  CHECK(firstRectToken(frame, row->children[1]) == spec.tabStyle);
  CHECK(firstRectToken(frame, row->children[2]) == spec.activeTabStyle);
}

TEST_CASE("PrimeStage dropdown clamps invalid selected index") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::DropdownSpec spec;
  spec.options = {"Alpha", "Beta"};
  spec.selectedIndex = 42;
  spec.paddingX = -9.0f;
  spec.indicatorGap = -3.0f;
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 24.0f;

  PrimeStage::UiNode dropdown = root.createDropdown(spec);
  std::string label = firstChildText(frame, dropdown.nodeId());
  CHECK(label == "Beta");
}

TEST_CASE("PrimeStage text field clamps out-of-range indices and negative cursor width") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::TextFieldState state;
  state.text = "Prime";
  state.cursor = 200u;
  state.selectionAnchor = 100u;
  state.selectionStart = 150u;
  state.selectionEnd = 250u;
  state.focused = true;
  state.cursorVisible = true;

  PrimeStage::TextFieldSpec spec;
  spec.state = &state;
  spec.paddingX = -12.0f;
  spec.cursorWidth = -4.0f;
  spec.cursorStyle = 301u;
  spec.selectionStyle = 302u;
  spec.size.preferredWidth = 180.0f;
  spec.size.preferredHeight = 24.0f;

  PrimeStage::UiNode field = root.createTextField(spec);
  REQUIRE(state.cursor == 5u);
  REQUIRE(state.selectionAnchor == 5u);
  REQUIRE(state.selectionStart == 5u);
  REQUIRE(state.selectionEnd == 5u);

  PrimeFrame::Primitive const* cursor = findRectPrimitiveByToken(frame, field.nodeId(), spec.cursorStyle);
  REQUIRE(cursor != nullptr);
  CHECK(cursor->width == doctest::Approx(0.0f));
}

TEST_CASE("PrimeStage table clamps invalid selected row to none") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::TableSpec spec;
  spec.columns = {{"Name", 0.0f, 0u, 0u}};
  spec.rows = {{"Row A"}, {"Row B"}};
  spec.selectedRow = 99;
  spec.selectionStyle = 501u;
  spec.focusStyle = 502u;
  spec.rowHeight = -20.0f;
  spec.headerHeight = -8.0f;
  spec.headerInset = -4.0f;
  spec.rowGap = -2.0f;
  spec.headerPaddingX = -6.0f;
  spec.cellPaddingX = -7.0f;
  spec.size.preferredWidth = 260.0f;
  spec.size.preferredHeight = 120.0f;

  PrimeStage::UiNode table = root.createTable(spec);
  CHECK(countRectToken(frame, table.nodeId(), spec.selectionStyle) == 0u);
}

TEST_CASE("PrimeStage accessibility semantics export snapshot covers default disabled and selected contracts") {
  PrimeStage::ButtonSpec buttonDefault;
  PrimeStage::ButtonSpec buttonDisabled;
  buttonDisabled.enabled = false;
  PrimeStage::ButtonSpec buttonSelected;
  buttonSelected.accessibility.state.selected = true;

  PrimeStage::ToggleSpec toggleChecked;
  toggleChecked.on = true;
  PrimeStage::ToggleSpec toggleDisabled;
  toggleDisabled.on = true;
  toggleDisabled.enabled = false;

  PrimeStage::DropdownSpec dropdownSelected;
  dropdownSelected.options = {"Red", "Green", "Blue"};
  dropdownSelected.selectedIndex = 1;

  PrimeStage::TabsSpec tabsSelected;
  tabsSelected.labels = {"A", "B", "C"};
  tabsSelected.selectedIndex = 2;

  PrimeStage::SliderSpec sliderDefault;
  sliderDefault.value = 0.75f;

  PrimeStage::ProgressBarSpec progressDefault;
  progressDefault.value = 0.25f;

  PrimeStage::TableSpec tableDefault;
  tableDefault.columns = {{"Name", 0.0f, 0u, 0u}};
  tableDefault.rows = {{"Row"}};

  PrimeStage::TreeViewSpec treeDefault;
  treeDefault.nodes = {PrimeStage::TreeNode{"Root", {}, true, false}};

  PrimeStage::TextFieldSpec fieldDefault;
  PrimeStage::LabelSpec labelDefault;
  PrimeStage::ListSpec listDefault;
  listDefault.items = {"One", "Two"};
  listDefault.selectedIndex = 1;

  std::string snapshot;
  snapshot += exportSemanticsRow("button.default",
                                 PrimeStage::Internal::normalizeButtonSpec(buttonDefault).accessibility) +
              "\n";
  snapshot += exportSemanticsRow("button.disabled",
                                 PrimeStage::Internal::normalizeButtonSpec(buttonDisabled).accessibility) +
              "\n";
  snapshot += exportSemanticsRow("button.selected_override",
                                 PrimeStage::Internal::normalizeButtonSpec(buttonSelected).accessibility) +
              "\n";
  snapshot += exportSemanticsRow("toggle.checked",
                                 PrimeStage::Internal::normalizeToggleSpec(toggleChecked).accessibility) +
              "\n";
  snapshot += exportSemanticsRow("toggle.disabled_checked",
                                 PrimeStage::Internal::normalizeToggleSpec(toggleDisabled).accessibility) +
              "\n";
  snapshot += exportSemanticsRow("dropdown.selected_index",
                                 PrimeStage::Internal::normalizeDropdownSpec(dropdownSelected).accessibility) +
              "\n";
  snapshot += exportSemanticsRow("tabs.selected_index",
                                 PrimeStage::Internal::normalizeTabsSpec(tabsSelected).accessibility) +
              "\n";
  snapshot += exportSemanticsRow("slider.default",
                                 PrimeStage::Internal::normalizeSliderSpec(sliderDefault).accessibility) +
              "\n";
  snapshot += exportSemanticsRow("progress.default",
                                 PrimeStage::Internal::normalizeProgressBarSpec(progressDefault).accessibility) +
              "\n";
  snapshot += exportSemanticsRow("table.default",
                                 PrimeStage::Internal::normalizeTableSpec(tableDefault).accessibility) +
              "\n";
  snapshot += exportSemanticsRow("tree.default",
                                 PrimeStage::Internal::normalizeTreeViewSpec(treeDefault).accessibility) +
              "\n";
  snapshot += exportSemanticsRow("text_field.default",
                                 PrimeStage::Internal::normalizeTextFieldSpec(fieldDefault).accessibility) +
              "\n";
  snapshot += exportSemanticsRow("label.default",
                                 PrimeStage::Internal::normalizeLabelSpec(labelDefault).accessibility) +
              "\n";
  snapshot += exportSemanticsRow("list.default",
                                 PrimeStage::Internal::normalizeListSpec(listDefault).accessibility);

  CHECK(snapshot == R"(button.default role=button disabled=false checked=- selected=- expanded=- valueNow=- valueMin=- valueMax=- position=- setSize=-
button.disabled role=button disabled=true checked=- selected=- expanded=- valueNow=- valueMin=- valueMax=- position=- setSize=-
button.selected_override role=button disabled=false checked=- selected=true expanded=- valueNow=- valueMin=- valueMax=- position=- setSize=-
toggle.checked role=toggle disabled=false checked=true selected=- expanded=- valueNow=- valueMin=- valueMax=- position=- setSize=-
toggle.disabled_checked role=toggle disabled=true checked=true selected=- expanded=- valueNow=- valueMin=- valueMax=- position=- setSize=-
dropdown.selected_index role=combo_box disabled=false checked=- selected=- expanded=- valueNow=- valueMin=- valueMax=- position=2 setSize=3
tabs.selected_index role=tab_list disabled=false checked=- selected=- expanded=- valueNow=- valueMin=- valueMax=- position=3 setSize=3
slider.default role=slider disabled=false checked=- selected=- expanded=- valueNow=0.75 valueMin=0.00 valueMax=1.00 position=- setSize=-
progress.default role=progress_bar disabled=false checked=- selected=- expanded=- valueNow=0.25 valueMin=0.00 valueMax=1.00 position=- setSize=-
table.default role=table disabled=false checked=- selected=- expanded=- valueNow=- valueMin=- valueMax=- position=- setSize=-
tree.default role=tree disabled=false checked=- selected=- expanded=- valueNow=- valueMin=- valueMax=- position=- setSize=-
text_field.default role=text_field disabled=false checked=- selected=- expanded=- valueNow=- valueMin=- valueMax=- position=- setSize=-
label.default role=static_text disabled=false checked=- selected=- expanded=- valueNow=- valueMin=- valueMax=- position=- setSize=-
list.default role=table disabled=false checked=- selected=- expanded=- valueNow=- valueMin=- valueMax=- position=- setSize=-)");
}

TEST_CASE("PrimeStage accessibility semantics focus snapshot covers focused and disabled navigation states") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame);

  PrimeStage::ButtonSpec buttonSpec;
  buttonSpec.label = "Run";
  buttonSpec.size.preferredWidth = 140.0f;
  buttonSpec.size.preferredHeight = 28.0f;

  PrimeStage::ButtonSpec disabledButtonSpec;
  disabledButtonSpec.label = "Disabled";
  disabledButtonSpec.enabled = false;
  disabledButtonSpec.size.preferredWidth = 140.0f;
  disabledButtonSpec.size.preferredHeight = 28.0f;

  PrimeStage::TextFieldSpec fieldSpec;
  fieldSpec.text = "Prime";
  fieldSpec.size.preferredWidth = 180.0f;
  fieldSpec.size.preferredHeight = 24.0f;

  PrimeStage::UiNode button = root.createButton(buttonSpec);
  PrimeStage::UiNode disabledButton = root.createButton(disabledButtonSpec);
  PrimeStage::UiNode field = root.createTextField(fieldSpec);

  PrimeFrame::LayoutOutput layout = layoutFrame(frame);
  PrimeFrame::FocusManager focus;
  REQUIRE(focus.setActiveRoot(frame, layout, root.nodeId()));

  PrimeStage::AccessibilitySemantics buttonSemantics =
      PrimeStage::Internal::normalizeButtonSpec(buttonSpec).accessibility;
  PrimeStage::AccessibilitySemantics disabledButtonSemantics =
      PrimeStage::Internal::normalizeButtonSpec(disabledButtonSpec).accessibility;
  PrimeStage::AccessibilitySemantics fieldSemantics =
      PrimeStage::Internal::normalizeTextFieldSpec(fieldSpec).accessibility;

  std::string snapshot;
  snapshot += exportFocusRow("focus.first.button",
                             buttonSemantics,
                             focus.focusedNode() == button.nodeId()) +
              "\n";
  snapshot += exportFocusRow("focus.first.disabled_button",
                             disabledButtonSemantics,
                             focus.focusedNode() == disabledButton.nodeId()) +
              "\n";
  snapshot += exportFocusRow("focus.first.text_field",
                             fieldSemantics,
                             focus.focusedNode() == field.nodeId()) +
              "\n";

  REQUIRE(focus.handleTab(frame, layout, true));

  snapshot += exportFocusRow("focus.after_tab.button",
                             buttonSemantics,
                             focus.focusedNode() == button.nodeId()) +
              "\n";
  snapshot += exportFocusRow("focus.after_tab.disabled_button",
                             disabledButtonSemantics,
                             focus.focusedNode() == disabledButton.nodeId()) +
              "\n";
  snapshot += exportFocusRow("focus.after_tab.text_field",
                             fieldSemantics,
                             focus.focusedNode() == field.nodeId());

  CHECK(snapshot == R"(focus.first.button role=button focused=true disabled=false
focus.first.disabled_button role=button focused=false disabled=true
focus.first.text_field role=text_field focused=false disabled=false
focus.after_tab.button role=button focused=false disabled=false
focus.after_tab.disabled_button role=button focused=false disabled=true
focus.after_tab.text_field role=text_field focused=true disabled=false)");
}
