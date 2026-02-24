#include "PrimeStage/StudioUi.h"
#include "PrimeFrame/Frame.h"

#include "third_party/doctest.h"

namespace Studio = PrimeStage::Studio;

static PrimeStage::UiNode createRoot(PrimeFrame::Frame& frame, float width, float height) {
  PrimeStage::SizeSpec size;
  size.preferredWidth = width;
  size.preferredHeight = height;
  return Studio::createRoot(frame, size);
}

static PrimeFrame::Primitive const* firstTextPrimitive(PrimeFrame::Frame const& frame,
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

static PrimeFrame::Primitive const* findTextChild(PrimeFrame::Frame const& frame,
                                                  PrimeFrame::NodeId parent,
                                                  PrimeFrame::TextStyleToken token) {
  PrimeFrame::Node const* parentNode = frame.getNode(parent);
  if (!parentNode) {
    return nullptr;
  }
  for (PrimeFrame::NodeId child : parentNode->children) {
    PrimeFrame::Primitive const* prim = firstTextPrimitive(frame, child);
    if (prim && prim->textStyle.token == token) {
      return prim;
    }
  }
  return nullptr;
}

TEST_CASE("PrimeStage tabs create active and inactive tab styles") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 80.0f);

  PrimeStage::TabsSpec spec;
  spec.labels = {"One", "Two", "Three"};
  spec.selectedIndex = 1;
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 28.0f;
  spec.tabStyle = 61u;
  spec.activeTabStyle = 62u;
  spec.textStyle = 71u;
  spec.activeTextStyle = 72u;

  PrimeStage::UiNode tabs = root.createTabs(spec);
  PrimeFrame::Node const* row = frame.getNode(tabs.nodeId());
  REQUIRE(row != nullptr);
  CHECK(row->layout == PrimeFrame::LayoutType::HorizontalStack);
  CHECK(row->children.size() == spec.labels.size());

  for (size_t i = 0; i < row->children.size(); ++i) {
    PrimeFrame::Node const* tabNode = frame.getNode(row->children[i]);
    REQUIRE(tabNode != nullptr);
    REQUIRE(!tabNode->primitives.empty());
    PrimeFrame::Primitive const* rectPrim = frame.getPrimitive(tabNode->primitives.front());
    REQUIRE(rectPrim != nullptr);
    PrimeFrame::RectStyleToken expectedToken =
        static_cast<int>(i) == spec.selectedIndex ? spec.activeTabStyle : spec.tabStyle;
    CHECK(rectPrim->rect.token == expectedToken);
    REQUIRE(tabNode->sizeHint.height.preferred.has_value());
    CHECK(tabNode->sizeHint.height.preferred.value() == doctest::Approx(spec.size.preferredHeight.value()));

    PrimeFrame::TextStyleToken expectedText =
        static_cast<int>(i) == spec.selectedIndex ? spec.activeTextStyle : spec.textStyle;
    PrimeFrame::Primitive const* textPrim = findTextChild(frame, tabNode->id, expectedText);
    REQUIRE(textPrim != nullptr);
    CHECK(textPrim->textStyle.token == expectedText);
  }
}

TEST_CASE("PrimeStage dropdown with label creates label and indicator text") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 80.0f);

  PrimeStage::DropdownSpec spec;
  spec.label = "Options";
  spec.indicator = "v";
  spec.size.preferredWidth = 160.0f;
  spec.size.preferredHeight = 24.0f;
  spec.paddingX = 8.0f;
  spec.indicatorGap = 6.0f;
  spec.backgroundStyle = 81u;
  spec.textStyle = 91u;
  spec.indicatorStyle = 92u;

  PrimeStage::UiNode dropdown = root.createDropdown(spec);
  PrimeFrame::Node const* node = frame.getNode(dropdown.nodeId());
  REQUIRE(node != nullptr);
  CHECK(node->layout == PrimeFrame::LayoutType::HorizontalStack);
  CHECK(node->padding.left == doctest::Approx(spec.paddingX));
  CHECK(node->padding.right == doctest::Approx(spec.paddingX));
  CHECK(node->gap == doctest::Approx(spec.indicatorGap));
  CHECK(node->children.size() == 2);

  PrimeFrame::Primitive const* labelPrim = findTextChild(frame, node->id, spec.textStyle);
  PrimeFrame::Primitive const* indicatorPrim = findTextChild(frame, node->id, spec.indicatorStyle);
  REQUIRE(labelPrim != nullptr);
  REQUIRE(indicatorPrim != nullptr);
  CHECK(labelPrim->textBlock.align == PrimeFrame::TextAlign::Start);
  CHECK(indicatorPrim->textBlock.align == PrimeFrame::TextAlign::Center);
}

TEST_CASE("PrimeStage dropdown with empty label inserts spacer") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 60.0f);

  PrimeStage::DropdownSpec spec;
  spec.label = "";
  spec.indicator = "v";
  spec.size.preferredWidth = 140.0f;
  spec.size.preferredHeight = 20.0f;
  spec.paddingX = 6.0f;
  spec.indicatorGap = 4.0f;
  spec.backgroundStyle = 101u;
  spec.textStyle = 111u;
  spec.indicatorStyle = 112u;

  PrimeStage::UiNode dropdown = root.createDropdown(spec);
  PrimeFrame::Node const* node = frame.getNode(dropdown.nodeId());
  REQUIRE(node != nullptr);
  CHECK(node->children.size() == 2);

  int spacerCount = 0;
  int indicatorCount = 0;
  for (PrimeFrame::NodeId child : node->children) {
    PrimeFrame::Node const* childNode = frame.getNode(child);
    REQUIRE(childNode != nullptr);
    PrimeFrame::Primitive const* prim = firstTextPrimitive(frame, child);
    if (!prim) {
      spacerCount += 1;
      continue;
    }
    if (prim->textStyle.token == spec.indicatorStyle) {
      indicatorCount += 1;
    }
  }

  CHECK(spacerCount == 1);
  CHECK(indicatorCount == 1);
}
