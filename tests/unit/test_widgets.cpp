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
