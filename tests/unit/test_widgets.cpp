#include "PrimeStage/Ui.h"
#include "PrimeFrame/Frame.h"

#include "third_party/doctest.h"

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

static PrimeFrame::Node const* findChildWithRectToken(PrimeFrame::Frame const& frame,
                                                      PrimeFrame::NodeId parent,
                                                      PrimeFrame::RectStyleToken token) {
  PrimeFrame::Node const* parentNode = frame.getNode(parent);
  if (!parentNode) {
    return nullptr;
  }
  for (PrimeFrame::NodeId child : parentNode->children) {
    PrimeFrame::Node const* childNode = frame.getNode(child);
    if (!childNode || childNode->primitives.empty()) {
      continue;
    }
    PrimeFrame::Primitive const* prim = frame.getPrimitive(childNode->primitives.front());
    if (!prim || prim->type != PrimeFrame::PrimitiveType::Rect) {
      continue;
    }
    if (prim->rect.token == token) {
      return childNode;
    }
  }
  return nullptr;
}

TEST_CASE("PrimeStage progress bar min fill width clamps to bounds") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 80.0f);

  PrimeStage::ProgressBarSpec spec;
  spec.size.preferredWidth = 100.0f;
  spec.size.preferredHeight = 12.0f;
  spec.value = 0.1f;
  spec.minFillWidth = 40.0f;
  spec.trackStyle = 401u;
  spec.fillStyle = 402u;

  PrimeStage::UiNode bar = root.createProgressBar(spec);
  PrimeFrame::Node const* barNode = frame.getNode(bar.nodeId());
  REQUIRE(barNode != nullptr);

  PrimeFrame::Node const* fillNode = findChildWithRectToken(frame, bar.nodeId(), spec.fillStyle);
  REQUIRE(fillNode != nullptr);
  REQUIRE(fillNode->sizeHint.width.preferred.has_value());
  CHECK(fillNode->sizeHint.width.preferred.value() == doctest::Approx(40.0f));

  spec.minFillWidth = 140.0f;
  PrimeStage::UiNode barClamp = root.createProgressBar(spec);
  PrimeFrame::Node const* fillClamp = findChildWithRectToken(frame, barClamp.nodeId(), spec.fillStyle);
  REQUIRE(fillClamp != nullptr);
  REQUIRE(fillClamp->sizeHint.width.preferred.has_value());
  CHECK(fillClamp->sizeHint.width.preferred.value() == doctest::Approx(100.0f));
}

TEST_CASE("PrimeStage progress bar with zero value and no min fill creates no fill node") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 160.0f, 60.0f);

  PrimeStage::ProgressBarSpec spec;
  spec.size.preferredWidth = 120.0f;
  spec.size.preferredHeight = 10.0f;
  spec.value = 0.0f;
  spec.minFillWidth = 0.0f;
  spec.trackStyle = 501u;
  spec.fillStyle = 502u;

  PrimeStage::UiNode bar = root.createProgressBar(spec);
  PrimeFrame::Node const* fillNode = findChildWithRectToken(frame, bar.nodeId(), spec.fillStyle);
  CHECK(fillNode == nullptr);
}

TEST_CASE("PrimeStage intrinsic defaults keep unsized widgets visible") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 800.0f, 600.0f);

  PrimeStage::ScrollView scrollView = root.createScrollView(PrimeStage::ScrollViewSpec{});
  PrimeFrame::Node const* scrollNode = frame.getNode(scrollView.root.nodeId());
  REQUIRE(scrollNode != nullptr);
  REQUIRE(scrollNode->sizeHint.width.preferred.has_value());
  REQUIRE(scrollNode->sizeHint.height.preferred.has_value());
  CHECK(scrollNode->sizeHint.width.preferred.value() == doctest::Approx(320.0f));
  CHECK(scrollNode->sizeHint.height.preferred.value() == doctest::Approx(180.0f));

  PrimeStage::TableSpec tableSpec;
  PrimeStage::UiNode table = root.createTable(tableSpec);
  PrimeFrame::Node const* tableNode = frame.getNode(table.nodeId());
  REQUIRE(tableNode != nullptr);
  REQUIRE(tableNode->sizeHint.width.preferred.has_value());
  REQUIRE(tableNode->sizeHint.height.preferred.has_value());
  CHECK(tableNode->sizeHint.width.preferred.value() == doctest::Approx(280.0f));
  CHECK(tableNode->sizeHint.height.preferred.value() > 0.0f);

  PrimeStage::TreeViewSpec treeSpec;
  PrimeStage::UiNode tree = root.createTreeView(treeSpec);
  PrimeFrame::Node const* treeNode = frame.getNode(tree.nodeId());
  REQUIRE(treeNode != nullptr);
  REQUIRE(treeNode->sizeHint.width.preferred.has_value());
  REQUIRE(treeNode->sizeHint.height.preferred.has_value());
  CHECK(treeNode->sizeHint.width.preferred.value() == doctest::Approx(280.0f));
  CHECK(treeNode->sizeHint.height.preferred.value() > 0.0f);
}

TEST_CASE("PrimeStage text widgets use size.maxWidth as responsive wrap policy") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 800.0f, 600.0f);

  PrimeStage::ParagraphSpec paragraph;
  paragraph.text = "Paragraph text should wrap without explicit widget maxWidth.";
  paragraph.size.maxWidth = 180.0f;
  PrimeStage::UiNode paragraphNode = root.createParagraph(paragraph);
  PrimeFrame::Node const* paragraphRaw = frame.getNode(paragraphNode.nodeId());
  REQUIRE(paragraphRaw != nullptr);
  REQUIRE(paragraphRaw->sizeHint.width.preferred.has_value());
  CHECK(paragraphRaw->sizeHint.width.preferred.value() <= doctest::Approx(180.0f));

  PrimeStage::SelectableTextSpec selectable;
  selectable.text = "Selectable text follows size.maxWidth for default wrapping.";
  selectable.size.maxWidth = 200.0f;
  PrimeStage::UiNode selectableNode = root.createSelectableText(selectable);
  PrimeFrame::Node const* selectableRaw = frame.getNode(selectableNode.nodeId());
  REQUIRE(selectableRaw != nullptr);
  REQUIRE(selectableRaw->sizeHint.width.preferred.has_value());
  CHECK(selectableRaw->sizeHint.width.preferred.value() <= doctest::Approx(200.0f));
}
