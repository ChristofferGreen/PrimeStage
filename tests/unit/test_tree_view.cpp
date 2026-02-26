#include "PrimeStage/Ui.h"
#include "PrimeFrame/Frame.h"
#include "PrimeFrame/Layout.h"

#include "third_party/doctest.h"

#include <algorithm>

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

static PrimeFrame::NodeId findVerticalStack(PrimeFrame::Frame const& frame,
                                            PrimeFrame::NodeId parent) {
  PrimeFrame::Node const* parentNode = frame.getNode(parent);
  if (!parentNode) {
    return PrimeFrame::NodeId{};
  }
  for (PrimeFrame::NodeId child : parentNode->children) {
    PrimeFrame::Node const* childNode = frame.getNode(child);
    if (childNode && childNode->layout == PrimeFrame::LayoutType::VerticalStack) {
      return child;
    }
  }
  return PrimeFrame::NodeId{};
}

static PrimeFrame::Primitive const* findRectToken(PrimeFrame::Frame const& frame,
                                                  PrimeFrame::Node const& row,
                                                  PrimeFrame::RectStyleToken token) {
  for (PrimeFrame::NodeId child : row.children) {
    PrimeFrame::Node const* childNode = frame.getNode(child);
    if (!childNode || childNode->primitives.empty()) {
      continue;
    }
    PrimeFrame::Primitive const* prim = frame.getPrimitive(childNode->primitives.front());
    if (prim && prim->type == PrimeFrame::PrimitiveType::Rect &&
        prim->rect.token == token) {
      return prim;
    }
  }
  return nullptr;
}

TEST_CASE("PrimeStage tree view flattens expanded nodes and selection accent") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 220.0f, 140.0f);

  PrimeStage::TreeViewSpec spec;
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 120.0f;
  spec.rowHeight = 12.0f;
  spec.rowGap = 2.0f;
  spec.rowStartY = 0.0f;
  spec.rowStartX = 0.0f;
  spec.rowWidthInset = 0.0f;
  spec.indent = 10.0f;
  spec.caretBaseX = 0.0f;
  spec.caretSize = 8.0f;
  spec.caretThickness = 2.0f;
  spec.selectionAccentWidth = 6.0f;
  spec.showScrollBar = false;
  spec.rowStyle = 11u;
  spec.rowAltStyle = 12u;
  spec.selectionStyle = 13u;
  spec.selectionAccentStyle = 14u;
  spec.caretBackgroundStyle = 15u;
  spec.caretLineStyle = 16u;
  spec.connectorStyle = 17u;
  spec.textStyle = 1u;
  spec.selectedTextStyle = 2u;

  PrimeStage::TreeNode rootNode;
  rootNode.label = "Root";
  rootNode.expanded = true;
  rootNode.children = {
      PrimeStage::TreeNode{"ChildA", {PrimeStage::TreeNode{"GrandA"}}, false, false},
      PrimeStage::TreeNode{"ChildB", {PrimeStage::TreeNode{"GrandB"}}, true, true}
  };

  PrimeStage::TreeNode second;
  second.label = "Second";
  second.expanded = false;
  second.children = {PrimeStage::TreeNode{"Hidden"}};

  spec.nodes = {rootNode, second};

  PrimeStage::UiNode tree = root.createTreeView(spec);
  PrimeFrame::Node const* treeNode = frame.getNode(tree.nodeId());
  REQUIRE(treeNode != nullptr);

  PrimeFrame::NodeId rowsNodeId = findVerticalStack(frame, tree.nodeId());
  REQUIRE(rowsNodeId.isValid());
  PrimeFrame::Node const* rowsNode = frame.getNode(rowsNodeId);
  REQUIRE(rowsNode != nullptr);

  const size_t expectedRows = 5;
  CHECK(rowsNode->children.size() == expectedRows);

  PrimeFrame::Node const* selectedRow = frame.getNode(rowsNode->children[2]);
  REQUIRE(selectedRow != nullptr);
  CHECK(findRectToken(frame, *selectedRow, spec.selectionAccentStyle) != nullptr);

  PrimeFrame::Node const* unselectedRow = frame.getNode(rowsNode->children[0]);
  REQUIRE(unselectedRow != nullptr);
  PrimeFrame::Primitive const* accent = findRectToken(frame, *unselectedRow, spec.selectionAccentStyle);
  REQUIRE(accent != nullptr);
  REQUIRE(accent->rect.overrideStyle.opacity.has_value());
  CHECK(accent->rect.overrideStyle.opacity.value() == doctest::Approx(0.0f));
}

TEST_CASE("PrimeStage tree view scroll bar auto thumb sizes") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 220.0f, 140.0f);

  PrimeStage::TreeViewSpec spec;
  spec.size.preferredWidth = 180.0f;
  spec.size.preferredHeight = 60.0f;
  spec.rowHeight = 10.0f;
  spec.rowGap = 0.0f;
  spec.rowStartY = 0.0f;
  spec.rowStartX = 0.0f;
  spec.rowWidthInset = 0.0f;
  spec.indent = 10.0f;
  spec.caretBaseX = 0.0f;
  spec.caretSize = 8.0f;
  spec.caretThickness = 2.0f;
  spec.rowStyle = 21u;
  spec.rowAltStyle = 22u;
  spec.selectionStyle = 23u;
  spec.selectionAccentStyle = 24u;
  spec.caretBackgroundStyle = 25u;
  spec.caretLineStyle = 26u;
  spec.connectorStyle = 27u;
  spec.textStyle = 1u;
  spec.selectedTextStyle = 2u;

  spec.showScrollBar = true;
  spec.scrollBar.enabled = true;
  spec.scrollBar.autoThumb = true;
  spec.scrollBar.inset = 4.0f;
  spec.scrollBar.padding = 5.0f;
  spec.scrollBar.width = 7.0f;
  spec.scrollBar.minThumbHeight = 8.0f;
  spec.scrollBar.thumbProgress = 0.25f;
  spec.scrollBar.trackStyle = 31u;
  spec.scrollBar.thumbStyle = 32u;

  for (int i = 0; i < 10; ++i) {
    PrimeStage::TreeNode node;
    node.label = "Row";
    spec.nodes.push_back(node);
  }

  PrimeStage::UiNode tree = root.createTreeView(spec);
  PrimeFrame::Node const* treeNode = frame.getNode(tree.nodeId());
  REQUIRE(treeNode != nullptr);

  PrimeFrame::Node const* trackNode = nullptr;
  PrimeFrame::Node const* thumbNode = nullptr;
  for (PrimeFrame::NodeId child : treeNode->children) {
    PrimeFrame::Node const* childNode = frame.getNode(child);
    if (!childNode || childNode->primitives.empty()) {
      continue;
    }
    PrimeFrame::Primitive const* prim = frame.getPrimitive(childNode->primitives.front());
    if (!prim || prim->type != PrimeFrame::PrimitiveType::Rect) {
      continue;
    }
    if (prim->rect.token == spec.scrollBar.trackStyle) {
      trackNode = childNode;
    } else if (prim->rect.token == spec.scrollBar.thumbStyle) {
      thumbNode = childNode;
    }
  }

  REQUIRE(trackNode != nullptr);
  REQUIRE(thumbNode != nullptr);
  REQUIRE(trackNode->sizeHint.height.preferred.has_value());
  REQUIRE(thumbNode->sizeHint.height.preferred.has_value());

  float contentHeight = spec.rowStartY + 10.0f * spec.rowHeight;
  float trackH = std::max(0.0f, spec.size.preferredHeight.value() - spec.scrollBar.padding * 2.0f);
  float thumbFraction = std::clamp(spec.size.preferredHeight.value() / contentHeight, 0.0f, 1.0f);
  float thumbH = trackH * thumbFraction;
  thumbH = std::max(thumbH, spec.scrollBar.minThumbHeight);
  if (thumbH > trackH) {
    thumbH = trackH;
  }

  CHECK(trackNode->sizeHint.height.preferred.value() == doctest::Approx(trackH));
  CHECK(thumbNode->sizeHint.height.preferred.value() == doctest::Approx(thumbH));
}

TEST_CASE("PrimeStage scroll bar thumb pixels clamp to track") {
  PrimeStage::ScrollBarSpec spec;
  setScrollBarThumbPixels(spec, 100.0f, 20.0f, 30.0f);
  CHECK(spec.autoThumb == false);
  CHECK(spec.thumbFraction == doctest::Approx(0.2f));
  CHECK(spec.thumbProgress == doctest::Approx(0.375f));

  setScrollBarThumbPixels(spec, 0.0f, 5.0f, 5.0f);
  CHECK(spec.thumbFraction == doctest::Approx(1.0f));
  CHECK(spec.thumbProgress == doctest::Approx(1.0f));
}
