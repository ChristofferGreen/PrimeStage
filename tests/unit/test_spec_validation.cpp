#include "PrimeStage/Ui.h"

#include "PrimeFrame/Frame.h"

#include "third_party/doctest.h"

#include <string>
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
