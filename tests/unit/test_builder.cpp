#include "PrimeStage/StudioUi.h"
#include "PrimeFrame/Events.h"

#include "third_party/doctest.h"

#include <algorithm>

namespace Studio = PrimeStage::Studio;

static PrimeStage::UiNode createRoot(PrimeFrame::Frame& frame, float width, float height) {
  PrimeStage::SizeSpec size;
  size.preferredWidth = width;
  size.preferredHeight = height;
  return Studio::createStudioRoot(frame, size);
}

TEST_CASE("PrimeStage UiNode builds panels and labels") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 100.0f, 50.0f);
  CHECK(frame.roots().size() == 1);
  PrimeFrame::Node const* rootNode = frame.getNode(root.nodeId());
  REQUIRE(rootNode != nullptr);
  CHECK(rootNode->sizeHint.width.preferred.has_value());
  CHECK(rootNode->sizeHint.height.preferred.has_value());
  CHECK(*rootNode->sizeHint.width.preferred == doctest::Approx(100.0f));
  CHECK(*rootNode->sizeHint.height.preferred == doctest::Approx(50.0f));

  PrimeStage::PanelSpec panelSpec;
  panelSpec.size.preferredWidth = 40.0f;
  panelSpec.size.preferredHeight = 20.0f;
  panelSpec.rectStyle = Studio::rectToken(Studio::RectRole::Panel);
  PrimeStage::UiNode panel = root.createPanel(panelSpec);
  PrimeFrame::Node const* panelNode = frame.getNode(panel.nodeId());
  REQUIRE(panelNode != nullptr);
  CHECK(panelNode->parent == root.nodeId());
  CHECK(panelNode->primitives.size() == 1);
  CHECK(panelNode->localX == doctest::Approx(0.0f));
  CHECK(panelNode->localY == doctest::Approx(0.0f));

  PrimeStage::LabelSpec labelSpec;
  labelSpec.size.preferredWidth = 10.0f;
  labelSpec.size.preferredHeight = 8.0f;
  labelSpec.text = "Label";
  labelSpec.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  PrimeStage::UiNode label = panel.createLabel(labelSpec);
  PrimeFrame::Node const* labelNode = frame.getNode(label.nodeId());
  REQUIRE(labelNode != nullptr);
  CHECK(labelNode->parent == panel.nodeId());
  CHECK(labelNode->primitives.size() == 1);
  CHECK(labelNode->localX == doctest::Approx(0.0f));
  CHECK(labelNode->localY == doctest::Approx(0.0f));
}

TEST_CASE("PrimeStage role helpers create panels and labels") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = createRoot(frame, 50.0f, 20.0f);
  PrimeStage::SizeSpec panelSize;
  panelSize.preferredWidth = 20.0f;
  panelSize.preferredHeight = 10.0f;
  PrimeStage::UiNode panel = Studio::createPanel(root,
                                                 Studio::RectRole::PanelStrong,
                                                 panelSize);
  PrimeStage::SizeSpec labelSize;
  labelSize.preferredWidth = 10.0f;
  labelSize.preferredHeight = 8.0f;
  PrimeStage::UiNode label = Studio::createLabel(panel,
                                                 "Hello",
                                                 Studio::TextRole::SmallMuted,
                                                 labelSize);
  CHECK(frame.getNode(panel.nodeId()) != nullptr);
  CHECK(frame.getNode(label.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage paragraph creates a node") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 80.0f);

  PrimeStage::SizeSpec paragraphSize;
  paragraphSize.preferredWidth = 160.0f;
  PrimeStage::UiNode paragraph = Studio::createParagraph(root,
                                                         "Line one\nLine two",
                                                         Studio::TextRole::SmallMuted,
                                                         paragraphSize);
  CHECK(frame.getNode(paragraph.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage text line creates a node") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  PrimeStage::SizeSpec lineSize;
  lineSize.preferredWidth = 160.0f;
  lineSize.preferredHeight = 20.0f;
  PrimeStage::UiNode line = Studio::createTextLine(root,
                                                   "Title",
                                                   Studio::TextRole::BodyBright,
                                                   lineSize,
                                                   PrimeFrame::TextAlign::Center);
  CHECK(frame.getNode(line.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage table creates a node") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 120.0f);

  Studio::TableSpec tableSpec;
  tableSpec.base.size.preferredWidth = 180.0f;
  tableSpec.columns = {
      Studio::TableColumn{"Item", 100.0f,
                              Studio::TextRole::SmallBright,
                              Studio::TextRole::SmallBright},
      Studio::TableColumn{"Status", 80.0f,
                              Studio::TextRole::SmallBright,
                              Studio::TextRole::SmallMuted}
  };
  tableSpec.base.rows = {
      {"Row", "Ready"},
      {"Row", "Ready"}
  };
  PrimeStage::UiNode table = Studio::createTable(root, tableSpec);
  CHECK(frame.getNode(table.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage table header includes column dividers") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 140.0f);

  PrimeStage::TableSpec spec;
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 80.0f;
  spec.headerHeight = 20.0f;
  spec.headerStyle = 9801;
  spec.dividerStyle = 9802;
  spec.showColumnDividers = true;
  spec.columns = {
      PrimeStage::TableColumn{"A", 60.0f,
                              Studio::textToken(Studio::TextRole::SmallBright),
                              Studio::textToken(Studio::TextRole::SmallMuted)},
      PrimeStage::TableColumn{"B", 60.0f,
                              Studio::textToken(Studio::TextRole::SmallBright),
                              Studio::textToken(Studio::TextRole::SmallMuted)},
      PrimeStage::TableColumn{"C", 60.0f,
                              Studio::textToken(Studio::TextRole::SmallBright),
                              Studio::textToken(Studio::TextRole::SmallMuted)}
  };

  PrimeStage::UiNode table = root.createTable(spec);

  PrimeFrame::Node const* tableNode = frame.getNode(table.nodeId());
  REQUIRE(tableNode != nullptr);

  PrimeFrame::Node const* headerRow = nullptr;
  for (PrimeFrame::NodeId childId : tableNode->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (!child || child->primitives.empty()) {
      continue;
    }
    PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
    if (prim && prim->rect.token == spec.headerStyle) {
      headerRow = child;
      break;
    }
  }
  REQUIRE(headerRow != nullptr);
  CHECK(headerRow->children.size() == 5);
}

TEST_CASE("PrimeStage table infers width from fixed columns") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 140.0f);

  PrimeStage::TableSpec spec;
  spec.columns = {
      PrimeStage::TableColumn{"A", 40.0f,
                              Studio::textToken(Studio::TextRole::SmallBright),
                              Studio::textToken(Studio::TextRole::SmallMuted)},
      PrimeStage::TableColumn{"B", 50.0f,
                              Studio::textToken(Studio::TextRole::SmallBright),
                              Studio::textToken(Studio::TextRole::SmallMuted)}
  };

  PrimeStage::UiNode table = root.createTable(spec);
  PrimeFrame::Node const* tableNode = frame.getNode(table.nodeId());
  REQUIRE(tableNode != nullptr);
  REQUIRE(tableNode->sizeHint.width.preferred.has_value());
  CHECK(tableNode->sizeHint.width.preferred.value() == doctest::Approx(90.0f));
}

TEST_CASE("PrimeStage table inferred width grows with header text") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 140.0f);

  PrimeStage::TableSpec shortSpec;
  shortSpec.columns = {
      PrimeStage::TableColumn{"A", 0.0f,
                              Studio::textToken(Studio::TextRole::SmallBright),
                              Studio::textToken(Studio::TextRole::SmallMuted)}
  };

  PrimeStage::TableSpec longSpec = shortSpec;
  longSpec.columns = {
      PrimeStage::TableColumn{"LongHeaderLabel", 0.0f,
                              Studio::textToken(Studio::TextRole::SmallBright),
                              Studio::textToken(Studio::TextRole::SmallMuted)}
  };

  PrimeStage::UiNode shortTable = root.createTable(shortSpec);
  PrimeStage::UiNode longTable = root.createTable(longSpec);

  PrimeFrame::Node const* shortNode = frame.getNode(shortTable.nodeId());
  PrimeFrame::Node const* longNode = frame.getNode(longTable.nodeId());
  REQUIRE(shortNode != nullptr);
  REQUIRE(longNode != nullptr);
  REQUIRE(shortNode->sizeHint.width.preferred.has_value());
  REQUIRE(longNode->sizeHint.width.preferred.has_value());
  CHECK(longNode->sizeHint.width.preferred.value() >
        shortNode->sizeHint.width.preferred.value());
}

TEST_CASE("PrimeStage table header dividers add extra rows") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 140.0f);

  PrimeStage::TableSpec spec;
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 100.0f;
  spec.headerHeight = 20.0f;
  spec.headerStyle = 9951;
  spec.dividerStyle = 9952;
  spec.showHeaderDividers = true;
  spec.columns = {
      PrimeStage::TableColumn{"A", 60.0f,
                              Studio::textToken(Studio::TextRole::SmallBright),
                              Studio::textToken(Studio::TextRole::SmallMuted)},
      PrimeStage::TableColumn{"B", 60.0f,
                              Studio::textToken(Studio::TextRole::SmallBright),
                              Studio::textToken(Studio::TextRole::SmallMuted)}
  };

  PrimeStage::UiNode table = root.createTable(spec);

  PrimeFrame::Node const* tableNode = frame.getNode(table.nodeId());
  REQUIRE(tableNode != nullptr);

  int dividerCount = 0;
  for (PrimeFrame::NodeId childId : tableNode->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (!child || child->primitives.empty()) {
      continue;
    }
    PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
    if (prim && prim->rect.token == spec.dividerStyle) {
      ++dividerCount;
    }
  }

  CHECK(dividerCount == 2);
}

TEST_CASE("PrimeStage table auto columns share remaining width") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 140.0f);

  PrimeStage::TableSpec spec;
  spec.size.preferredWidth = 210.0f;
  spec.size.preferredHeight = 80.0f;
  spec.headerHeight = 20.0f;
  spec.headerStyle = 9961;
  spec.dividerStyle = 9962;
  spec.showColumnDividers = true;
  spec.columns = {
      PrimeStage::TableColumn{"Fixed", 50.0f,
                              Studio::textToken(Studio::TextRole::SmallBright),
                              Studio::textToken(Studio::TextRole::SmallMuted)},
      PrimeStage::TableColumn{"Auto1", 0.0f,
                              Studio::textToken(Studio::TextRole::SmallBright),
                              Studio::textToken(Studio::TextRole::SmallMuted)},
      PrimeStage::TableColumn{"Auto2", 0.0f,
                              Studio::textToken(Studio::TextRole::SmallBright),
                              Studio::textToken(Studio::TextRole::SmallMuted)}
  };

  PrimeStage::UiNode table = root.createTable(spec);

  PrimeFrame::Node const* tableNode = frame.getNode(table.nodeId());
  REQUIRE(tableNode != nullptr);

  PrimeFrame::Node const* headerRow = nullptr;
  for (PrimeFrame::NodeId childId : tableNode->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (!child || child->primitives.empty()) {
      continue;
    }
    PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
    if (prim && prim->rect.token == spec.headerStyle) {
      headerRow = child;
      break;
    }
  }
  REQUIRE(headerRow != nullptr);

  std::vector<float> widths;
  for (PrimeFrame::NodeId childId : headerRow->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (!child) {
      continue;
    }
    if (!child->primitives.empty()) {
      PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
      if (prim && prim->rect.token == spec.dividerStyle) {
        continue;
      }
    }
    REQUIRE(child->sizeHint.width.preferred.has_value());
    widths.push_back(child->sizeHint.width.preferred.value());
  }

  REQUIRE(widths.size() == 3);
  CHECK(widths[0] == doctest::Approx(50.0f));
  CHECK(widths[1] == doctest::Approx(79.0f));
  CHECK(widths[2] == doctest::Approx(79.0f));
}

TEST_CASE("PrimeStage table clamps last column when fixed widths overflow") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 140.0f);

  PrimeStage::TableSpec spec;
  spec.size.preferredWidth = 120.0f;
  spec.size.preferredHeight = 80.0f;
  spec.headerHeight = 20.0f;
  spec.headerStyle = 9971;
  spec.dividerStyle = 9972;
  spec.showColumnDividers = true;
  spec.columns = {
      PrimeStage::TableColumn{"A", 70.0f,
                              Studio::textToken(Studio::TextRole::SmallBright),
                              Studio::textToken(Studio::TextRole::SmallMuted)},
      PrimeStage::TableColumn{"B", 70.0f,
                              Studio::textToken(Studio::TextRole::SmallBright),
                              Studio::textToken(Studio::TextRole::SmallMuted)}
  };

  PrimeStage::UiNode table = root.createTable(spec);

  PrimeFrame::Node const* tableNode = frame.getNode(table.nodeId());
  REQUIRE(tableNode != nullptr);

  PrimeFrame::Node const* headerRow = nullptr;
  for (PrimeFrame::NodeId childId : tableNode->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (!child || child->primitives.empty()) {
      continue;
    }
    PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
    if (prim && prim->rect.token == spec.headerStyle) {
      headerRow = child;
      break;
    }
  }
  REQUIRE(headerRow != nullptr);

  std::vector<float> widths;
  for (PrimeFrame::NodeId childId : headerRow->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (!child) {
      continue;
    }
    if (!child->primitives.empty()) {
      PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
      if (prim && prim->rect.token == spec.dividerStyle) {
        continue;
      }
    }
    REQUIRE(child->sizeHint.width.preferred.has_value());
    widths.push_back(child->sizeHint.width.preferred.value());
  }

  REQUIRE(widths.size() == 2);
  CHECK(widths[0] == doctest::Approx(70.0f));
  CHECK(widths[1] == doctest::Approx(49.0f));
}

TEST_CASE("PrimeStage table auto columns use content widths when space is tight") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 140.0f);

  PrimeStage::TableSpec spec;
  spec.size.preferredWidth = 60.0f;
  spec.size.preferredHeight = 80.0f;
  spec.headerHeight = 20.0f;
  spec.headerStyle = 9981;
  spec.showColumnDividers = false;
  spec.showHeaderDividers = false;
  spec.columns = {
      PrimeStage::TableColumn{"Fixed", 60.0f,
                              Studio::textToken(Studio::TextRole::SmallBright),
                              Studio::textToken(Studio::TextRole::SmallMuted)},
      PrimeStage::TableColumn{"A", 0.0f,
                              Studio::textToken(Studio::TextRole::SmallBright),
                              Studio::textToken(Studio::TextRole::SmallMuted)},
      PrimeStage::TableColumn{"LongHeaderLabel", 0.0f,
                              Studio::textToken(Studio::TextRole::SmallBright),
                              Studio::textToken(Studio::TextRole::SmallMuted)}
  };

  PrimeStage::UiNode table = root.createTable(spec);

  PrimeFrame::Node const* tableNode = frame.getNode(table.nodeId());
  REQUIRE(tableNode != nullptr);

  PrimeFrame::Node const* headerRow = nullptr;
  for (PrimeFrame::NodeId childId : tableNode->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (!child || child->primitives.empty()) {
      continue;
    }
    PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
    if (prim && prim->rect.token == spec.headerStyle) {
      headerRow = child;
      break;
    }
  }
  REQUIRE(headerRow != nullptr);

  std::vector<float> widths;
  for (PrimeFrame::NodeId childId : headerRow->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (!child) {
      continue;
    }
    REQUIRE(child->sizeHint.width.preferred.has_value());
    widths.push_back(child->sizeHint.width.preferred.value());
  }

  REQUIRE(widths.size() == 3);
  CHECK(widths[0] == doctest::Approx(60.0f));
  CHECK(widths[2] > widths[1]);
}

TEST_CASE("PrimeStage table auto columns share width when labels match") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 140.0f);

  PrimeStage::TableSpec spec;
  spec.headerHeight = 20.0f;
  spec.headerStyle = 9991;
  spec.showColumnDividers = false;
  spec.showHeaderDividers = false;
  spec.columns = {
      PrimeStage::TableColumn{"Fixed", 50.0f,
                              Studio::textToken(Studio::TextRole::SmallBright),
                              Studio::textToken(Studio::TextRole::SmallMuted)},
      PrimeStage::TableColumn{"Auto", 0.0f,
                              Studio::textToken(Studio::TextRole::SmallBright),
                              Studio::textToken(Studio::TextRole::SmallMuted)},
      PrimeStage::TableColumn{"Auto", 0.0f,
                              Studio::textToken(Studio::TextRole::SmallBright),
                              Studio::textToken(Studio::TextRole::SmallMuted)}
  };

  PrimeStage::UiNode table = root.createTable(spec);
  PrimeFrame::Node const* tableNode = frame.getNode(table.nodeId());
  REQUIRE(tableNode != nullptr);
  REQUIRE(tableNode->sizeHint.width.preferred.has_value());

  PrimeFrame::Node const* headerRow = nullptr;
  for (PrimeFrame::NodeId childId : tableNode->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (!child || child->primitives.empty()) {
      continue;
    }
    PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
    if (prim && prim->rect.token == spec.headerStyle) {
      headerRow = child;
      break;
    }
  }
  REQUIRE(headerRow != nullptr);
  REQUIRE(headerRow->children.size() == 3);

  std::vector<float> widths;
  for (PrimeFrame::NodeId childId : headerRow->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    REQUIRE(child != nullptr);
    REQUIRE(child->sizeHint.width.preferred.has_value());
    widths.push_back(child->sizeHint.width.preferred.value());
  }

  CHECK(widths[1] == doctest::Approx(widths[2]));
  CHECK(tableNode->sizeHint.width.preferred.value() ==
        doctest::Approx(widths[0] + widths[1] + widths[2]));
}

TEST_CASE("PrimeStage table auto columns split remaining width with dividers") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 140.0f);

  PrimeStage::TableSpec spec;
  spec.size.preferredWidth = 210.0f;
  spec.size.preferredHeight = 80.0f;
  spec.headerHeight = 20.0f;
  spec.headerStyle = 9993;
  spec.dividerStyle = 9994;
  spec.showColumnDividers = true;
  spec.showHeaderDividers = false;
  spec.columns = {
      PrimeStage::TableColumn{"Fixed", 50.0f,
                              Studio::textToken(Studio::TextRole::SmallBright),
                              Studio::textToken(Studio::TextRole::SmallMuted)},
      PrimeStage::TableColumn{"Auto", 0.0f,
                              Studio::textToken(Studio::TextRole::SmallBright),
                              Studio::textToken(Studio::TextRole::SmallMuted)},
      PrimeStage::TableColumn{"Auto", 0.0f,
                              Studio::textToken(Studio::TextRole::SmallBright),
                              Studio::textToken(Studio::TextRole::SmallMuted)}
  };

  PrimeStage::UiNode table = root.createTable(spec);
  PrimeFrame::Node const* tableNode = frame.getNode(table.nodeId());
  REQUIRE(tableNode != nullptr);

  PrimeFrame::Node const* headerRow = nullptr;
  for (PrimeFrame::NodeId childId : tableNode->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (!child || child->primitives.empty()) {
      continue;
    }
    PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
    if (prim && prim->rect.token == spec.headerStyle) {
      headerRow = child;
      break;
    }
  }
  REQUIRE(headerRow != nullptr);

  std::vector<float> widths;
  for (PrimeFrame::NodeId childId : headerRow->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (!child) {
      continue;
    }
    if (!child->primitives.empty()) {
      PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
      if (prim && prim->rect.token == spec.dividerStyle) {
        continue;
      }
    }
    REQUIRE(child->sizeHint.width.preferred.has_value());
    widths.push_back(child->sizeHint.width.preferred.value());
  }

  REQUIRE(widths.size() == 3);
  CHECK(widths[0] == doctest::Approx(50.0f));
  CHECK(widths[1] == doctest::Approx(79.0f));
  CHECK(widths[2] == doctest::Approx(79.0f));
}

TEST_CASE("PrimeStage table inferred width grows with row content") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 140.0f);

  PrimeStage::TableSpec shortSpec;
  shortSpec.columns = {
      PrimeStage::TableColumn{"Col", 0.0f,
                              Studio::textToken(Studio::TextRole::SmallBright),
                              Studio::textToken(Studio::TextRole::SmallMuted)}
  };
  shortSpec.rows = {{"X"}};

  PrimeStage::TableSpec longSpec = shortSpec;
  longSpec.rows = {{"MuchLongerValue"}};

  PrimeStage::UiNode shortTable = root.createTable(shortSpec);
  PrimeStage::UiNode longTable = root.createTable(longSpec);

  PrimeFrame::Node const* shortNode = frame.getNode(shortTable.nodeId());
  PrimeFrame::Node const* longNode = frame.getNode(longTable.nodeId());
  REQUIRE(shortNode != nullptr);
  REQUIRE(longNode != nullptr);
  REQUIRE(shortNode->sizeHint.width.preferred.has_value());
  REQUIRE(longNode->sizeHint.width.preferred.has_value());
  CHECK(longNode->sizeHint.width.preferred.value() >
        shortNode->sizeHint.width.preferred.value());
}

TEST_CASE("PrimeStage table rows include column dividers") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 140.0f);

  PrimeStage::TableSpec spec;
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 100.0f;
  spec.headerHeight = 20.0f;
  spec.rowHeight = 16.0f;
  spec.rowGap = 0.0f;
  spec.headerStyle = 9901;
  spec.rowStyle = 9902;
  spec.rowAltStyle = 9903;
  spec.dividerStyle = 9904;
  spec.showColumnDividers = true;
  spec.columns = {
      PrimeStage::TableColumn{"A", 60.0f,
                              Studio::textToken(Studio::TextRole::SmallBright),
                              Studio::textToken(Studio::TextRole::SmallMuted)},
      PrimeStage::TableColumn{"B", 60.0f,
                              Studio::textToken(Studio::TextRole::SmallBright),
                              Studio::textToken(Studio::TextRole::SmallMuted)},
      PrimeStage::TableColumn{"C", 60.0f,
                              Studio::textToken(Studio::TextRole::SmallBright),
                              Studio::textToken(Studio::TextRole::SmallMuted)}
  };
  spec.rows = {{"1", "2", "3"}};

  PrimeStage::UiNode table = root.createTable(spec);

  PrimeFrame::Node const* tableNode = frame.getNode(table.nodeId());
  REQUIRE(tableNode != nullptr);

  PrimeFrame::Node const* rowsContainer = nullptr;
  for (PrimeFrame::NodeId childId : tableNode->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (child && child->layout == PrimeFrame::LayoutType::VerticalStack) {
      rowsContainer = child;
    }
  }
  REQUIRE(rowsContainer != nullptr);
  REQUIRE(rowsContainer->children.size() == 1);

  PrimeFrame::Node const* rowNode = frame.getNode(rowsContainer->children[0]);
  REQUIRE(rowNode != nullptr);
  CHECK(rowNode->children.size() == 5);
}

TEST_CASE("PrimeStage tree view creates a node") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 120.0f);

  Studio::TreeViewSpec spec;
  spec.base.size.preferredWidth = 200.0f;
  spec.base.size.preferredHeight = 120.0f;
  spec.base.nodes = {Studio::TreeNode{"Root", {Studio::TreeNode{"Child"}}, true, false}};
  PrimeStage::UiNode tree = Studio::createTreeView(root, spec);
  CHECK(frame.getNode(tree.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage tree view header divider uses connector style") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 160.0f);

  PrimeStage::TreeViewSpec spec;
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 120.0f;
  spec.showHeaderDivider = true;
  spec.headerDividerY = 12.0f;
  spec.connectorThickness = 2.0f;
  spec.showConnectors = false;
  spec.showScrollBar = false;
  spec.connectorStyle = 9001;
  spec.rowStyle = 9002;
  spec.rowAltStyle = 9003;
  spec.selectionStyle = 9004;
  spec.caretBackgroundStyle = 9005;
  spec.caretLineStyle = 9006;
  spec.textStyle = Studio::textToken(Studio::TextRole::SmallMuted);
  spec.selectedTextStyle = Studio::textToken(Studio::TextRole::SmallBright);
  spec.nodes = {PrimeStage::TreeNode{"Root", {}, true, false}};
  PrimeStage::UiNode tree = root.createTreeView(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 240.0f;
  options.rootHeight = 160.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::Node const* treeNode = frame.getNode(tree.nodeId());
  REQUIRE(treeNode != nullptr);

  PrimeFrame::NodeId dividerId{};
  for (PrimeFrame::NodeId childId : treeNode->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (!child || child->primitives.empty()) {
      continue;
    }
    PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
    if (prim && prim->rect.token == spec.connectorStyle) {
      dividerId = childId;
      break;
    }
  }
  REQUIRE(dividerId.isValid());
  PrimeFrame::LayoutOut const* dividerOut = layout.get(dividerId);
  REQUIRE(dividerOut != nullptr);
  CHECK(dividerOut->absW == doctest::Approx(200.0f));
  CHECK(dividerOut->absH == doctest::Approx(spec.connectorThickness));
  CHECK(dividerOut->absY == doctest::Approx(spec.headerDividerY + spec.rowStartY));
}

TEST_CASE("PrimeStage tree view scroll bar auto thumb clamps height") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 180.0f);

  PrimeStage::TreeViewSpec spec;
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 100.0f;
  spec.rowHeight = 20.0f;
  spec.rowGap = 0.0f;
  spec.rowStartY = 0.0f;
  spec.showConnectors = false;
  spec.showHeaderDivider = false;
  spec.scrollBar.inset = 4.0f;
  spec.scrollBar.padding = 0.0f;
  spec.scrollBar.width = 6.0f;
  spec.scrollBar.minThumbHeight = 10.0f;
  spec.scrollBar.trackStyle = 9101;
  spec.scrollBar.thumbStyle = 9102;
  spec.rowStyle = 9103;
  spec.rowAltStyle = 9104;
  spec.selectionStyle = 9105;
  spec.caretBackgroundStyle = 9106;
  spec.caretLineStyle = 9107;
  spec.textStyle = Studio::textToken(Studio::TextRole::SmallMuted);
  spec.selectedTextStyle = Studio::textToken(Studio::TextRole::SmallBright);
  spec.nodes = {
      PrimeStage::TreeNode{"Root", {PrimeStage::TreeNode{"A"},
                                   PrimeStage::TreeNode{"B"},
                                   PrimeStage::TreeNode{"C"},
                                   PrimeStage::TreeNode{"D"},
                                   PrimeStage::TreeNode{"E"}}, true, false}
  };
  PrimeStage::UiNode tree = root.createTreeView(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 240.0f;
  options.rootHeight = 180.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::Node const* treeNode = frame.getNode(tree.nodeId());
  REQUIRE(treeNode != nullptr);

  auto find_child_by_token = [&](PrimeFrame::RectStyleToken token) {
    for (PrimeFrame::NodeId childId : treeNode->children) {
      PrimeFrame::Node const* child = frame.getNode(childId);
      if (!child || child->primitives.empty()) {
        continue;
      }
      PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
      if (prim && prim->rect.token == token) {
        return childId;
      }
    }
    return PrimeFrame::NodeId{};
  };

  PrimeFrame::NodeId trackId = find_child_by_token(spec.scrollBar.trackStyle);
  PrimeFrame::NodeId thumbId = find_child_by_token(spec.scrollBar.thumbStyle);
  REQUIRE(trackId.isValid());
  REQUIRE(thumbId.isValid());

  PrimeFrame::LayoutOut const* trackOut = layout.get(trackId);
  PrimeFrame::LayoutOut const* thumbOut = layout.get(thumbId);
  REQUIRE(trackOut != nullptr);
  REQUIRE(thumbOut != nullptr);

  float expectedTrackH = 100.0f;
  float contentHeight = spec.rowStartY + 6.0f * spec.rowHeight;
  float expectedFraction = std::clamp(100.0f / contentHeight, 0.0f, 1.0f);
  float expectedThumbH = expectedTrackH * expectedFraction;
  expectedThumbH = std::max(expectedThumbH, spec.scrollBar.minThumbHeight);

  CHECK(trackOut->absW == doctest::Approx(spec.scrollBar.width));
  CHECK(trackOut->absH == doctest::Approx(expectedTrackH));
  CHECK(trackOut->absX == doctest::Approx(200.0f - spec.scrollBar.inset));
  CHECK(trackOut->absY == doctest::Approx(spec.scrollBar.padding));

  CHECK(thumbOut->absW == doctest::Approx(spec.scrollBar.width));
  CHECK(thumbOut->absH == doctest::Approx(expectedThumbH));
  CHECK(thumbOut->absX == doctest::Approx(trackOut->absX));
  CHECK(thumbOut->absY == doctest::Approx(trackOut->absY));
}

TEST_CASE("PrimeStage tree view scroll bar auto thumb fills when content smaller") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 160.0f);

  PrimeStage::TreeViewSpec spec;
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 120.0f;
  spec.rowHeight = 20.0f;
  spec.rowGap = 0.0f;
  spec.rowStartY = 0.0f;
  spec.showConnectors = false;
  spec.showHeaderDivider = false;
  spec.scrollBar.padding = 0.0f;
  spec.scrollBar.inset = 4.0f;
  spec.scrollBar.width = 6.0f;
  spec.scrollBar.trackStyle = 9201;
  spec.scrollBar.thumbStyle = 9202;
  spec.rowStyle = 9203;
  spec.rowAltStyle = 9204;
  spec.selectionStyle = 9205;
  spec.caretBackgroundStyle = 9206;
  spec.caretLineStyle = 9207;
  spec.textStyle = Studio::textToken(Studio::TextRole::SmallMuted);
  spec.selectedTextStyle = Studio::textToken(Studio::TextRole::SmallBright);
  spec.nodes = {PrimeStage::TreeNode{"Root", {}, true, false}};
  PrimeStage::UiNode tree = root.createTreeView(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 240.0f;
  options.rootHeight = 160.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::Node const* treeNode = frame.getNode(tree.nodeId());
  REQUIRE(treeNode != nullptr);

  auto find_child_by_token = [&](PrimeFrame::RectStyleToken token) {
    for (PrimeFrame::NodeId childId : treeNode->children) {
      PrimeFrame::Node const* child = frame.getNode(childId);
      if (!child || child->primitives.empty()) {
        continue;
      }
      PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
      if (prim && prim->rect.token == token) {
        return childId;
      }
    }
    return PrimeFrame::NodeId{};
  };

  PrimeFrame::NodeId trackId = find_child_by_token(spec.scrollBar.trackStyle);
  PrimeFrame::NodeId thumbId = find_child_by_token(spec.scrollBar.thumbStyle);
  REQUIRE(trackId.isValid());
  REQUIRE(thumbId.isValid());

  PrimeFrame::LayoutOut const* trackOut = layout.get(trackId);
  PrimeFrame::LayoutOut const* thumbOut = layout.get(thumbId);
  REQUIRE(trackOut != nullptr);
  REQUIRE(thumbOut != nullptr);

  CHECK(trackOut->absH == doctest::Approx(120.0f));
  CHECK(thumbOut->absH == doctest::Approx(trackOut->absH));
  CHECK(thumbOut->absX == doctest::Approx(trackOut->absX));
  CHECK(thumbOut->absY == doctest::Approx(trackOut->absY));
}

TEST_CASE("PrimeStage tree view caret line count differs for expanded vs collapsed") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 260.0f, 200.0f);

  PrimeStage::TreeViewSpec spec;
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 160.0f;
  spec.rowHeight = 24.0f;
  spec.rowGap = 0.0f;
  spec.rowStartY = 0.0f;
  spec.showConnectors = false;
  spec.showCaretMasks = false;
  spec.showScrollBar = false;
  spec.caretLineStyle = 9401;
  spec.caretBackgroundStyle = 9402;
  spec.rowStyle = 9403;
  spec.rowAltStyle = 9404;
  spec.selectionStyle = 9405;
  spec.textStyle = Studio::textToken(Studio::TextRole::SmallMuted);
  spec.selectedTextStyle = Studio::textToken(Studio::TextRole::SmallBright);
  spec.nodes = {
      PrimeStage::TreeNode{"Expanded", {PrimeStage::TreeNode{"Child"}}, true, false},
      PrimeStage::TreeNode{"Collapsed", {PrimeStage::TreeNode{"Child2"}}, false, false}
  };
  PrimeStage::UiNode tree = root.createTreeView(spec);

  PrimeFrame::Node const* treeNode = frame.getNode(tree.nodeId());
  REQUIRE(treeNode != nullptr);

  PrimeFrame::Node const* rowsNode = nullptr;
  for (PrimeFrame::NodeId childId : treeNode->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (child && child->layout == PrimeFrame::LayoutType::VerticalStack) {
      rowsNode = child;
      break;
    }
  }
  REQUIRE(rowsNode != nullptr);

  auto find_row_by_label = [&](std::string_view label) -> PrimeFrame::Node const* {
    for (PrimeFrame::NodeId rowId : rowsNode->children) {
      PrimeFrame::Node const* rowNode = frame.getNode(rowId);
      if (!rowNode) {
        continue;
      }
      for (PrimeFrame::NodeId childId : rowNode->children) {
        PrimeFrame::Node const* child = frame.getNode(childId);
        if (!child || child->primitives.empty()) {
          continue;
        }
        PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
        if (prim && prim->type == PrimeFrame::PrimitiveType::Text &&
            prim->textBlock.text == label) {
          return rowNode;
        }
      }
    }
    return nullptr;
  };

  PrimeFrame::Node const* expandedRow = find_row_by_label("Expanded");
  PrimeFrame::Node const* collapsedRow = find_row_by_label("Collapsed");
  REQUIRE(expandedRow != nullptr);
  REQUIRE(collapsedRow != nullptr);

  auto count_caret_lines = [&](PrimeFrame::Node const* rowNode) {
    int count = 0;
    for (PrimeFrame::NodeId childId : rowNode->children) {
      PrimeFrame::Node const* child = frame.getNode(childId);
      if (!child || child->primitives.empty()) {
        continue;
      }
      PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
      if (prim && prim->rect.token == spec.caretLineStyle) {
        ++count;
      }
    }
    return count;
  };

  CHECK(count_caret_lines(expandedRow) == 1);
  CHECK(count_caret_lines(collapsedRow) == 2);
}

TEST_CASE("PrimeStage tree view selection accent uses configured width") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 160.0f);

  PrimeStage::TreeViewSpec spec;
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 120.0f;
  spec.rowHeight = 22.0f;
  spec.rowGap = 0.0f;
  spec.rowStartY = 0.0f;
  spec.selectionAccentWidth = 5.0f;
  spec.selectionAccentStyle = 9501;
  spec.rowStyle = 9502;
  spec.rowAltStyle = 9503;
  spec.selectionStyle = 9504;
  spec.caretBackgroundStyle = 9505;
  spec.caretLineStyle = 9506;
  spec.textStyle = Studio::textToken(Studio::TextRole::SmallMuted);
  spec.selectedTextStyle = Studio::textToken(Studio::TextRole::SmallBright);
  spec.showConnectors = false;
  spec.showScrollBar = false;
  spec.nodes = {PrimeStage::TreeNode{"Selected", {}, true, true}};
  PrimeStage::UiNode tree = root.createTreeView(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 240.0f;
  options.rootHeight = 160.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::Node const* treeNode = frame.getNode(tree.nodeId());
  REQUIRE(treeNode != nullptr);

  PrimeFrame::Node const* rowsNode = nullptr;
  for (PrimeFrame::NodeId childId : treeNode->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (child && child->layout == PrimeFrame::LayoutType::VerticalStack) {
      rowsNode = child;
      break;
    }
  }
  REQUIRE(rowsNode != nullptr);
  REQUIRE(rowsNode->children.size() == 1);

  PrimeFrame::Node const* rowNode = frame.getNode(rowsNode->children[0]);
  REQUIRE(rowNode != nullptr);

  PrimeFrame::NodeId accentId{};
  for (PrimeFrame::NodeId childId : rowNode->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (!child || child->primitives.empty()) {
      continue;
    }
    PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
    if (prim && prim->rect.token == spec.selectionAccentStyle) {
      accentId = childId;
      break;
    }
  }

  REQUIRE(accentId.isValid());
  PrimeFrame::LayoutOut const* accentOut = layout.get(accentId);
  REQUIRE(accentOut != nullptr);
  CHECK(accentOut->absW == doctest::Approx(spec.selectionAccentWidth));
  CHECK(accentOut->absH == doctest::Approx(spec.rowHeight));
  CHECK(accentOut->absX == doctest::Approx(0.0f));
}

TEST_CASE("PrimeStage tree view draws connectors for expanded nodes") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 260.0f, 200.0f);

  PrimeStage::TreeViewSpec spec;
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 160.0f;
  spec.rowHeight = 24.0f;
  spec.rowGap = 0.0f;
  spec.rowStartY = 0.0f;
  spec.indent = 12.0f;
  spec.caretBaseX = 14.0f;
  spec.caretSize = 10.0f;
  spec.connectorThickness = 2.0f;
  spec.connectorStyle = 9701;
  spec.showConnectors = true;
  spec.showScrollBar = false;
  spec.showCaretMasks = false;
  spec.rowStyle = 9702;
  spec.rowAltStyle = 9703;
  spec.selectionStyle = 9704;
  spec.caretBackgroundStyle = 9705;
  spec.caretLineStyle = 9706;
  spec.textStyle = Studio::textToken(Studio::TextRole::SmallMuted);
  spec.selectedTextStyle = Studio::textToken(Studio::TextRole::SmallBright);
  spec.nodes = {PrimeStage::TreeNode{"Root", {PrimeStage::TreeNode{"Child"}}, true, false}};
  PrimeStage::UiNode tree = root.createTreeView(spec);

  PrimeFrame::Node const* treeNode = frame.getNode(tree.nodeId());
  REQUIRE(treeNode != nullptr);

  PrimeFrame::Node const* rowsNode = nullptr;
  for (PrimeFrame::NodeId childId : treeNode->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (child && child->layout == PrimeFrame::LayoutType::VerticalStack) {
      rowsNode = child;
      break;
    }
  }
  REQUIRE(rowsNode != nullptr);
  REQUIRE(rowsNode->children.size() >= 2);

  PrimeFrame::Node const* childRow = nullptr;
  for (PrimeFrame::NodeId rowId : rowsNode->children) {
    PrimeFrame::Node const* rowNode = frame.getNode(rowId);
    if (!rowNode) {
      continue;
    }
    for (PrimeFrame::NodeId childId : rowNode->children) {
      PrimeFrame::Node const* child = frame.getNode(childId);
      if (!child || child->primitives.empty()) {
        continue;
      }
      PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
      if (prim && prim->type == PrimeFrame::PrimitiveType::Text &&
          prim->textBlock.text == "Child") {
        childRow = rowNode;
        break;
      }
    }
    if (childRow) {
      break;
    }
  }

  REQUIRE(childRow != nullptr);

  bool foundConnector = false;
  for (PrimeFrame::NodeId childId : childRow->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (!child || child->primitives.empty()) {
      continue;
    }
    PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
    if (prim && prim->rect.token == spec.connectorStyle) {
      foundConnector = true;
      break;
    }
  }

  CHECK(foundConnector);
}

TEST_CASE("PrimeStage tree view connector count grows with depth") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 260.0f, 220.0f);

  PrimeStage::TreeViewSpec spec;
  spec.size.preferredWidth = 220.0f;
  spec.size.preferredHeight = 180.0f;
  spec.rowHeight = 22.0f;
  spec.rowGap = 0.0f;
  spec.rowStartY = 0.0f;
  spec.indent = 12.0f;
  spec.caretBaseX = 14.0f;
  spec.caretSize = 10.0f;
  spec.connectorThickness = 2.0f;
  spec.connectorStyle = 9711;
  spec.showConnectors = true;
  spec.showScrollBar = false;
  spec.showCaretMasks = false;
  spec.rowStyle = 9712;
  spec.rowAltStyle = 9713;
  spec.selectionStyle = 9714;
  spec.caretBackgroundStyle = 9715;
  spec.caretLineStyle = 9716;
  spec.textStyle = Studio::textToken(Studio::TextRole::SmallMuted);
  spec.selectedTextStyle = Studio::textToken(Studio::TextRole::SmallBright);
  spec.nodes = {
      PrimeStage::TreeNode{"Root",
                           {PrimeStage::TreeNode{"Mid",
                                                 {PrimeStage::TreeNode{"Leaf"}}, true, false}},
                           true, false}
  };
  PrimeStage::UiNode tree = root.createTreeView(spec);

  PrimeFrame::Node const* treeNode = frame.getNode(tree.nodeId());
  REQUIRE(treeNode != nullptr);

  PrimeFrame::Node const* rowsNode = nullptr;
  for (PrimeFrame::NodeId childId : treeNode->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (child && child->layout == PrimeFrame::LayoutType::VerticalStack) {
      rowsNode = child;
      break;
    }
  }
  REQUIRE(rowsNode != nullptr);
  REQUIRE(rowsNode->children.size() >= 3);

  auto find_row = [&](std::string_view label) -> PrimeFrame::Node const* {
    for (PrimeFrame::NodeId rowId : rowsNode->children) {
      PrimeFrame::Node const* rowNode = frame.getNode(rowId);
      if (!rowNode) {
        continue;
      }
      for (PrimeFrame::NodeId childId : rowNode->children) {
        PrimeFrame::Node const* child = frame.getNode(childId);
        if (!child || child->primitives.empty()) {
          continue;
        }
        PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
        if (prim && prim->type == PrimeFrame::PrimitiveType::Text &&
            prim->textBlock.text == label) {
          return rowNode;
        }
      }
    }
    return nullptr;
  };

  PrimeFrame::Node const* midRow = find_row("Mid");
  PrimeFrame::Node const* leafRow = find_row("Leaf");
  REQUIRE(midRow != nullptr);
  REQUIRE(leafRow != nullptr);

  auto count_connectors = [&](PrimeFrame::Node const* rowNode) {
    int count = 0;
    for (PrimeFrame::NodeId childId : rowNode->children) {
      PrimeFrame::Node const* child = frame.getNode(childId);
      if (!child || child->primitives.empty()) {
        continue;
      }
      PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
      if (prim && prim->rect.token == spec.connectorStyle) {
        ++count;
      }
    }
    return count;
  };

  CHECK(count_connectors(midRow) >= count_connectors(leafRow));
}
TEST_CASE("PrimeStage tree view infers size from rows") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 160.0f);

  PrimeStage::TreeViewSpec spec;
  spec.rowHeight = 20.0f;
  spec.rowGap = 4.0f;
  spec.rowStartY = 8.0f;
  spec.rowWidthInset = 12.0f;
  spec.showConnectors = false;
  spec.showScrollBar = false;
  spec.textStyle = Studio::textToken(Studio::TextRole::SmallMuted);
  spec.selectedTextStyle = Studio::textToken(Studio::TextRole::SmallBright);
  spec.rowStyle = 9601;
  spec.rowAltStyle = 9602;
  spec.selectionStyle = 9603;
  spec.caretBackgroundStyle = 9604;
  spec.caretLineStyle = 9605;
  spec.nodes = {PrimeStage::TreeNode{"", {}, true, false}};

  PrimeStage::UiNode tree = root.createTreeView(spec);
  PrimeFrame::Node const* treeNode = frame.getNode(tree.nodeId());
  REQUIRE(treeNode != nullptr);
  REQUIRE(treeNode->sizeHint.width.preferred.has_value());
  REQUIRE(treeNode->sizeHint.height.preferred.has_value());

  float expectedWidth = spec.rowWidthInset + 20.0f;
  float expectedHeight = spec.rowStartY + spec.rowHeight;
  CHECK(treeNode->sizeHint.width.preferred.value() == doctest::Approx(expectedWidth));
  CHECK(treeNode->sizeHint.height.preferred.value() == doctest::Approx(expectedHeight));
}

TEST_CASE("PrimeStage setScrollBarThumbPixels clamps fraction and progress") {
  PrimeStage::ScrollBarSpec spec;
  spec.autoThumb = true;

  PrimeStage::setScrollBarThumbPixels(spec, 0.0f, 200.0f, 50.0f);
  CHECK(!spec.autoThumb);
  CHECK(spec.thumbFraction == doctest::Approx(1.0f));
  CHECK(spec.thumbProgress == doctest::Approx(1.0f));

  PrimeStage::setScrollBarThumbPixels(spec, 100.0f, 20.0f, 200.0f);
  CHECK(spec.thumbFraction == doctest::Approx(0.2f));
  CHECK(spec.thumbProgress == doctest::Approx(1.0f));
}

TEST_CASE("PrimeStage section header creates a node") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 120.0f, 40.0f);

  PrimeStage::SizeSpec size;
  size.preferredWidth = 100.0f;
  size.preferredHeight = 20.0f;
  PrimeStage::UiNode header = Studio::createSectionHeader(root,
                                                          size,
                                                          "Header",
                                                          Studio::TextRole::SmallBright);
  CHECK(frame.getNode(header.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage section header accent and inset add children") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 80.0f);

  Studio::SectionHeaderSpec spec;
  spec.size.preferredWidth = 180.0f;
  spec.size.preferredHeight = 24.0f;
  spec.title = "Header";
  spec.accentWidth = 4.0f;
  spec.textInsetX = 12.0f;
  PrimeStage::UiNode header = Studio::createSectionHeader(root, spec);

  PrimeFrame::Node const* headerNode = frame.getNode(header.nodeId());
  REQUIRE(headerNode != nullptr);
  REQUIRE(headerNode->children.size() == 4);

  PrimeFrame::Node const* accentNode = frame.getNode(headerNode->children[0]);
  PrimeFrame::Node const* spacerLeft = frame.getNode(headerNode->children[1]);
  PrimeFrame::Node const* textNode = frame.getNode(headerNode->children[2]);
  PrimeFrame::Node const* spacerRight = frame.getNode(headerNode->children[3]);
  REQUIRE(accentNode != nullptr);
  REQUIRE(spacerLeft != nullptr);
  REQUIRE(textNode != nullptr);
  REQUIRE(spacerRight != nullptr);

  REQUIRE(accentNode->sizeHint.width.preferred.has_value());
  CHECK(accentNode->sizeHint.width.preferred.value() == doctest::Approx(spec.accentWidth));
  REQUIRE(spacerLeft->sizeHint.width.preferred.has_value());
  CHECK(spacerLeft->sizeHint.width.preferred.value() == doctest::Approx(spec.textInsetX));
  REQUIRE(spacerRight->sizeHint.width.preferred.has_value());
  CHECK(spacerRight->sizeHint.width.preferred.value() == doctest::Approx(spec.textInsetX));
  CHECK(!textNode->primitives.empty());
}

TEST_CASE("PrimeStage section header infers width from title and inset") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 80.0f);

  Studio::SectionHeaderSpec specBase;
  specBase.title = "Header";
  specBase.textInsetX = 0.0f;
  PrimeStage::UiNode headerBase = Studio::createSectionHeader(root, specBase);

  Studio::SectionHeaderSpec specInset = specBase;
  specInset.textInsetX = 12.0f;
  PrimeStage::UiNode headerInset = Studio::createSectionHeader(root, specInset);

  PrimeFrame::Node const* baseNode = frame.getNode(headerBase.nodeId());
  PrimeFrame::Node const* insetNode = frame.getNode(headerInset.nodeId());
  REQUIRE(baseNode != nullptr);
  REQUIRE(insetNode != nullptr);
  REQUIRE(baseNode->sizeHint.width.preferred.has_value());
  REQUIRE(insetNode->sizeHint.width.preferred.has_value());
  CHECK(insetNode->sizeHint.width.preferred.value() -
        baseNode->sizeHint.width.preferred.value() ==
        doctest::Approx(12.0f));
}

TEST_CASE("PrimeStage section header divider adds spacer and divider") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 220.0f, 120.0f);

  PrimeStage::StackSpec stackSpec;
  stackSpec.size.preferredWidth = 200.0f;
  stackSpec.size.preferredHeight = 100.0f;
  PrimeStage::UiNode container = root.createVerticalStack(stackSpec);

  Studio::SectionHeaderSpec spec;
  spec.size.preferredWidth = 180.0f;
  spec.size.preferredHeight = 24.0f;
  spec.title = "Header";
  spec.addDivider = true;
  spec.dividerOffsetY = 6.0f;
  spec.dividerRole = Studio::RectRole::Divider;
  PrimeStage::UiNode header = Studio::createSectionHeader(container, spec);
  CHECK(frame.getNode(header.nodeId()) != nullptr);

  PrimeFrame::Node const* containerNode = frame.getNode(container.nodeId());
  REQUIRE(containerNode != nullptr);
  REQUIRE(containerNode->children.size() >= 3);

  PrimeFrame::RectStyleToken dividerToken = Studio::rectToken(spec.dividerRole);
  PrimeFrame::NodeId dividerId{};
  bool foundSpacer = false;
  for (PrimeFrame::NodeId childId : containerNode->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (!child) {
      continue;
    }
    if (child->primitives.empty()) {
      if (child->sizeHint.height.preferred.has_value() &&
          *child->sizeHint.height.preferred == doctest::Approx(spec.dividerOffsetY)) {
        foundSpacer = true;
      }
      continue;
    }
    PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
    if (prim && prim->rect.token == dividerToken) {
      dividerId = childId;
    }
  }

  CHECK(foundSpacer);
  REQUIRE(dividerId.isValid());

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 220.0f;
  options.rootHeight = 120.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* dividerOut = layout.get(dividerId);
  REQUIRE(dividerOut != nullptr);
  CHECK(dividerOut->absW == doctest::Approx(200.0f));
  CHECK(dividerOut->absH == doctest::Approx(1.0f));
}

TEST_CASE("PrimeStage section panel creates a node") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 120.0f);

  Studio::SectionPanelSpec spec;
  spec.size.preferredWidth = 180.0f;
  spec.size.preferredHeight = 80.0f;
  spec.title = "Section";
  Studio::SectionPanel panel = Studio::createSectionPanel(root, spec);
  CHECK(frame.getNode(panel.panel.nodeId()) != nullptr);
  CHECK(frame.getNode(panel.content.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage section panel infers height from insets") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 220.0f, 140.0f);

  Studio::SectionPanelSpec spec;
  spec.title = "Section";
  spec.headerInsetY = 6.0f;
  spec.headerHeight = 20.0f;
  spec.contentInsetY = 8.0f;
  spec.contentInsetBottom = 10.0f;
  Studio::SectionPanel panel = Studio::createSectionPanel(root, spec);

  PrimeFrame::Node const* panelNode = frame.getNode(panel.panel.nodeId());
  REQUIRE(panelNode != nullptr);
  REQUIRE(panelNode->sizeHint.height.preferred.has_value());
  float expected = spec.headerInsetY + spec.headerHeight + spec.contentInsetY +
                   spec.contentInsetBottom;
  CHECK(panelNode->sizeHint.height.preferred.value() == doctest::Approx(expected));
}

TEST_CASE("PrimeStage property list infers height") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 120.0f);

  Studio::PropertyListSpec spec;
  spec.rowHeight = 12.0f;
  spec.rowGap = 4.0f;
  spec.rows = {Studio::PropertyRow{"One", "A"},
               Studio::PropertyRow{"Two", "B"},
               Studio::PropertyRow{"Three", "C"}};
  PrimeStage::UiNode list = Studio::createPropertyList(root, spec);

  PrimeFrame::Node const* listNode = frame.getNode(list.nodeId());
  REQUIRE(listNode != nullptr);
  REQUIRE(listNode->sizeHint.height.preferred.has_value());
  float expected = 3.0f * spec.rowHeight + 2.0f * spec.rowGap;
  CHECK(listNode->sizeHint.height.preferred.value() == doctest::Approx(expected));
}

TEST_CASE("PrimeStage property list infers width with empty strings") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 120.0f);

  Studio::PropertyListSpec spec;
  spec.labelInsetX = 10.0f;
  spec.valueInsetX = 7.0f;
  spec.valuePaddingRight = 5.0f;
  spec.valueAlignRight = true;
  spec.rows = {Studio::PropertyRow{"", ""}};
  PrimeStage::UiNode list = Studio::createPropertyList(root, spec);

  PrimeFrame::Node const* listNode = frame.getNode(list.nodeId());
  REQUIRE(listNode != nullptr);
  REQUIRE(listNode->sizeHint.width.preferred.has_value());
  CHECK(listNode->sizeHint.width.preferred.value() == doctest::Approx(15.0f));

  Studio::PropertyListSpec specLeft = spec;
  specLeft.valueAlignRight = false;
  PrimeStage::UiNode listLeft = Studio::createPropertyList(root, specLeft);

  PrimeFrame::Node const* listNodeLeft = frame.getNode(listLeft.nodeId());
  REQUIRE(listNodeLeft != nullptr);
  REQUIRE(listNodeLeft->sizeHint.width.preferred.has_value());
  CHECK(listNodeLeft->sizeHint.width.preferred.value() == doctest::Approx(10.0f));
}

TEST_CASE("PrimeStage property list honors explicit width for right aligned values") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 120.0f);

  Studio::PropertyListSpec spec;
  spec.size.preferredWidth = 200.0f;
  spec.labelInsetX = 10.0f;
  spec.valueInsetX = 7.0f;
  spec.valuePaddingRight = 5.0f;
  spec.valueAlignRight = true;
  spec.rows = {Studio::PropertyRow{"", ""}};
  PrimeStage::UiNode list = Studio::createPropertyList(root, spec);

  PrimeFrame::Node const* listNode = frame.getNode(list.nodeId());
  REQUIRE(listNode != nullptr);
  REQUIRE(listNode->children.size() == 1);
  PrimeFrame::Node const* rowNode = frame.getNode(listNode->children[0]);
  REQUIRE(rowNode != nullptr);
  REQUIRE(rowNode->children.size() == 2);

  PrimeFrame::Node const* labelCell = frame.getNode(rowNode->children[0]);
  PrimeFrame::Node const* valueCell = frame.getNode(rowNode->children[1]);
  REQUIRE(labelCell != nullptr);
  REQUIRE(valueCell != nullptr);
  REQUIRE(labelCell->sizeHint.width.preferred.has_value());
  REQUIRE(valueCell->sizeHint.width.preferred.has_value());

  CHECK(labelCell->sizeHint.width.preferred.value() == doctest::Approx(10.0f));
  CHECK(valueCell->sizeHint.width.preferred.value() == doctest::Approx(190.0f));

  Studio::PropertyListSpec specLeft = spec;
  specLeft.valueAlignRight = false;
  PrimeStage::UiNode listLeft = Studio::createPropertyList(root, specLeft);

  PrimeFrame::Node const* listNodeLeft = frame.getNode(listLeft.nodeId());
  REQUIRE(listNodeLeft != nullptr);
  REQUIRE(listNodeLeft->children.size() == 1);
  PrimeFrame::Node const* rowNodeLeft = frame.getNode(listNodeLeft->children[0]);
  REQUIRE(rowNodeLeft != nullptr);
  REQUIRE(rowNodeLeft->children.size() == 2);
  PrimeFrame::Node const* valueCellLeft = frame.getNode(rowNodeLeft->children[1]);
  REQUIRE(valueCellLeft != nullptr);
  REQUIRE(valueCellLeft->sizeHint.width.preferred.has_value());
  CHECK(valueCellLeft->sizeHint.width.preferred.value() == doctest::Approx(12.0f));
}

TEST_CASE("PrimeStage property list layout respects explicit width") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 120.0f);

  Studio::PropertyListSpec spec;
  spec.size.preferredWidth = 200.0f;
  spec.rowHeight = 14.0f;
  spec.rowGap = 4.0f;
  spec.labelInsetX = 10.0f;
  spec.valueInsetX = 7.0f;
  spec.valuePaddingRight = 5.0f;
  spec.valueAlignRight = true;
  spec.rows = {Studio::PropertyRow{"", ""}};
  PrimeStage::UiNode list = Studio::createPropertyList(root, spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 240.0f;
  options.rootHeight = 120.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::Node const* listNode = frame.getNode(list.nodeId());
  REQUIRE(listNode != nullptr);
  REQUIRE(listNode->children.size() == 1);
  PrimeFrame::Node const* rowNode = frame.getNode(listNode->children[0]);
  REQUIRE(rowNode != nullptr);
  REQUIRE(rowNode->children.size() == 2);

  PrimeFrame::LayoutOut const* labelOut = layout.get(rowNode->children[0]);
  PrimeFrame::LayoutOut const* valueOut = layout.get(rowNode->children[1]);
  REQUIRE(labelOut != nullptr);
  REQUIRE(valueOut != nullptr);
  CHECK(labelOut->absW == doctest::Approx(10.0f));
  CHECK(valueOut->absW == doctest::Approx(190.0f));
  CHECK(labelOut->absX == doctest::Approx(0.0f));
  CHECK(valueOut->absX == doctest::Approx(10.0f));
}

TEST_CASE("PrimeStage property list layout left aligned uses minimal value width") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 120.0f);

  Studio::PropertyListSpec spec;
  spec.size.preferredWidth = 200.0f;
  spec.rowHeight = 14.0f;
  spec.labelInsetX = 10.0f;
  spec.valueInsetX = 7.0f;
  spec.valuePaddingRight = 5.0f;
  spec.valueAlignRight = false;
  spec.rows = {Studio::PropertyRow{"", ""}};
  PrimeStage::UiNode list = Studio::createPropertyList(root, spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 240.0f;
  options.rootHeight = 120.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::Node const* listNode = frame.getNode(list.nodeId());
  REQUIRE(listNode != nullptr);
  REQUIRE(listNode->children.size() == 1);
  PrimeFrame::Node const* rowNode = frame.getNode(listNode->children[0]);
  REQUIRE(rowNode != nullptr);
  REQUIRE(rowNode->children.size() == 2);

  PrimeFrame::LayoutOut const* labelOut = layout.get(rowNode->children[0]);
  PrimeFrame::LayoutOut const* valueOut = layout.get(rowNode->children[1]);
  REQUIRE(labelOut != nullptr);
  REQUIRE(valueOut != nullptr);
  CHECK(labelOut->absW == doctest::Approx(10.0f));
  CHECK(valueOut->absW == doctest::Approx(12.0f));
  CHECK(labelOut->absX == doctest::Approx(0.0f));
  CHECK(valueOut->absX == doctest::Approx(10.0f));
}

TEST_CASE("PrimeStage card grid lays out columns and rows") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 200.0f);

  Studio::CardGridSpec spec;
  spec.cardWidth = 80.0f;
  spec.cardHeight = 40.0f;
  spec.gapX = 10.0f;
  spec.gapY = 8.0f;
  spec.size.preferredWidth = 170.0f;
  spec.size.preferredHeight = 100.0f;
  spec.cards = {Studio::CardSpec{"One", "A"},
                Studio::CardSpec{"Two", "B"},
                Studio::CardSpec{"Three", "C"}};
  PrimeStage::UiNode grid = Studio::createCardGrid(root, spec);

  PrimeFrame::Node const* gridNode = frame.getNode(grid.nodeId());
  REQUIRE(gridNode != nullptr);
  REQUIRE(gridNode->children.size() == 2);

  PrimeFrame::Node const* row0 = frame.getNode(gridNode->children[0]);
  PrimeFrame::Node const* row1 = frame.getNode(gridNode->children[1]);
  REQUIRE(row0 != nullptr);
  REQUIRE(row1 != nullptr);
  CHECK(row0->children.size() == 2);
  CHECK(row1->children.size() == 1);
}

TEST_CASE("PrimeStage card grid uses single column when width too small") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 200.0f);

  Studio::CardGridSpec spec;
  spec.cardWidth = 120.0f;
  spec.cardHeight = 40.0f;
  spec.gapX = 10.0f;
  spec.gapY = 6.0f;
  spec.size.preferredWidth = 80.0f;
  spec.size.preferredHeight = 200.0f;
  spec.cards = {Studio::CardSpec{"One", "A"},
                Studio::CardSpec{"Two", "B"},
                Studio::CardSpec{"Three", "C"}};
  PrimeStage::UiNode grid = Studio::createCardGrid(root, spec);

  PrimeFrame::Node const* gridNode = frame.getNode(grid.nodeId());
  REQUIRE(gridNode != nullptr);
  REQUIRE(gridNode->children.size() == 3);
  for (PrimeFrame::NodeId rowId : gridNode->children) {
    PrimeFrame::Node const* rowNode = frame.getNode(rowId);
    REQUIRE(rowNode != nullptr);
    CHECK(rowNode->children.size() == 1);
  }
}

TEST_CASE("PrimeStage card grid columns respect gap spacing") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 200.0f);

  Studio::CardGridSpec spec;
  spec.cardWidth = 80.0f;
  spec.cardHeight = 40.0f;
  spec.gapX = 12.0f;
  spec.gapY = 6.0f;
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 120.0f;
  spec.cards = {Studio::CardSpec{"One", "A"},
                Studio::CardSpec{"Two", "B"},
                Studio::CardSpec{"Three", "C"}};
  PrimeStage::UiNode grid = Studio::createCardGrid(root, spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 240.0f;
  options.rootHeight = 200.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::Node const* gridNode = frame.getNode(grid.nodeId());
  REQUIRE(gridNode != nullptr);
  REQUIRE(gridNode->children.size() == 2);

  PrimeFrame::Node const* row0 = frame.getNode(gridNode->children[0]);
  PrimeFrame::Node const* row1 = frame.getNode(gridNode->children[1]);
  REQUIRE(row0 != nullptr);
  REQUIRE(row1 != nullptr);
  REQUIRE(row0->children.size() == 2);
  REQUIRE(row1->children.size() == 1);

  PrimeFrame::LayoutOut const* card0 = layout.get(row0->children[0]);
  PrimeFrame::LayoutOut const* card1 = layout.get(row0->children[1]);
  PrimeFrame::LayoutOut const* card2 = layout.get(row1->children[0]);
  REQUIRE(card0 != nullptr);
  REQUIRE(card1 != nullptr);
  REQUIRE(card2 != nullptr);

  CHECK(card0->absX == doctest::Approx(0.0f));
  CHECK(card1->absX == doctest::Approx(spec.cardWidth + spec.gapX));
  CHECK(card2->absX == doctest::Approx(0.0f));
}
TEST_CASE("PrimeStage card grid text width clamps when padding exceeds width") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 120.0f);

  Studio::CardGridSpec spec;
  spec.cardWidth = 30.0f;
  spec.cardHeight = 40.0f;
  spec.paddingX = 20.0f;
  spec.size.preferredWidth = 30.0f;
  spec.size.preferredHeight = 40.0f;
  spec.cards = {Studio::CardSpec{"Title", "Sub"}};
  PrimeStage::UiNode grid = Studio::createCardGrid(root, spec);

  PrimeFrame::Node const* gridNode = frame.getNode(grid.nodeId());
  REQUIRE(gridNode != nullptr);
  REQUIRE(gridNode->children.size() == 1);
  PrimeFrame::Node const* rowNode = frame.getNode(gridNode->children[0]);
  REQUIRE(rowNode != nullptr);
  REQUIRE(rowNode->children.size() == 1);
  PrimeFrame::Node const* cardNode = frame.getNode(rowNode->children[0]);
  REQUIRE(cardNode != nullptr);

  for (PrimeFrame::NodeId childId : cardNode->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (!child || child->primitives.empty()) {
      continue;
    }
    PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
    if (!prim || prim->type != PrimeFrame::PrimitiveType::Text) {
      continue;
    }
    CHECK(!child->sizeHint.width.preferred.has_value());
  }
}
TEST_CASE("PrimeStage card grid positions title and subtitle") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 140.0f);

  Studio::CardGridSpec spec;
  spec.cardWidth = 120.0f;
  spec.cardHeight = 60.0f;
  spec.paddingX = 10.0f;
  spec.titleOffsetY = 6.0f;
  spec.subtitleOffsetY = 28.0f;
  spec.size.preferredWidth = 120.0f;
  spec.size.preferredHeight = 60.0f;
  spec.cards = {Studio::CardSpec{"Title", "Sub"}};
  PrimeStage::UiNode grid = Studio::createCardGrid(root, spec);

  PrimeFrame::Node const* gridNode = frame.getNode(grid.nodeId());
  REQUIRE(gridNode != nullptr);
  REQUIRE(gridNode->children.size() == 1);
  PrimeFrame::Node const* rowNode = frame.getNode(gridNode->children[0]);
  REQUIRE(rowNode != nullptr);
  REQUIRE(rowNode->children.size() == 1);
  PrimeFrame::Node const* cardNode = frame.getNode(rowNode->children[0]);
  REQUIRE(cardNode != nullptr);

  PrimeFrame::Node const* titleNode = nullptr;
  PrimeFrame::Node const* subtitleNode = nullptr;
  for (PrimeFrame::NodeId childId : cardNode->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (!child || child->primitives.empty()) {
      continue;
    }
    PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
    if (!prim || prim->type != PrimeFrame::PrimitiveType::Text) {
      continue;
    }
    if (prim->textBlock.text == "Title") {
      titleNode = child;
    } else if (prim->textBlock.text == "Sub") {
      subtitleNode = child;
    }
  }

  REQUIRE(titleNode != nullptr);
  REQUIRE(subtitleNode != nullptr);
  REQUIRE(titleNode->sizeHint.width.preferred.has_value());
  REQUIRE(subtitleNode->sizeHint.width.preferred.has_value());
  CHECK(titleNode->localX == doctest::Approx(spec.paddingX));
  CHECK(subtitleNode->localX == doctest::Approx(spec.paddingX));
  CHECK(titleNode->localY == doctest::Approx(spec.titleOffsetY));
  CHECK(subtitleNode->localY == doctest::Approx(spec.subtitleOffsetY));
  CHECK(titleNode->sizeHint.width.preferred.value() ==
        doctest::Approx(spec.cardWidth - spec.paddingX * 2.0f));
  CHECK(subtitleNode->sizeHint.width.preferred.value() ==
        doctest::Approx(spec.cardWidth - spec.paddingX * 2.0f));
}

TEST_CASE("PrimeStage card grid infers size from cards") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 220.0f, 200.0f);

  Studio::CardGridSpec spec;
  spec.cardWidth = 90.0f;
  spec.cardHeight = 40.0f;
  spec.gapY = 6.0f;
  spec.cards = {Studio::CardSpec{"One", "A"},
                Studio::CardSpec{"Two", "B"},
                Studio::CardSpec{"Three", "C"}};
  PrimeStage::UiNode grid = Studio::createCardGrid(root, spec);

  PrimeFrame::Node const* gridNode = frame.getNode(grid.nodeId());
  REQUIRE(gridNode != nullptr);
  REQUIRE(gridNode->sizeHint.width.preferred.has_value());
  REQUIRE(gridNode->sizeHint.height.preferred.has_value());

  float expectedW = spec.cardWidth;
  float expectedH = 3.0f * spec.cardHeight + 2.0f * spec.gapY;
  CHECK(gridNode->sizeHint.width.preferred.value() == doctest::Approx(expectedW));
  CHECK(gridNode->sizeHint.height.preferred.value() == doctest::Approx(expectedH));
}

TEST_CASE("PrimeStage shell creates a layout") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  Studio::ShellSpec spec;
  spec.size.preferredWidth = 320.0f;
  spec.size.preferredHeight = 180.0f;
  Studio::ShellLayout layout = Studio::createShell(frame, spec);
  CHECK(frame.getNode(layout.root.nodeId()) != nullptr);
  CHECK(frame.getNode(layout.topbar.nodeId()) != nullptr);
  CHECK(frame.getNode(layout.status.nodeId()) != nullptr);
  CHECK(frame.getNode(layout.sidebar.nodeId()) != nullptr);
  CHECK(frame.getNode(layout.content.nodeId()) != nullptr);
  CHECK(frame.getNode(layout.inspector.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage shell layout sizes panels") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  Studio::ShellSpec spec;
  spec.size.preferredWidth = 320.0f;
  spec.size.preferredHeight = 180.0f;
  spec.topbarHeight = 20.0f;
  spec.statusHeight = 18.0f;
  spec.sidebarWidth = 64.0f;
  spec.inspectorWidth = 52.0f;
  spec.drawDividers = false;

  Studio::ShellLayout shell = Studio::createShell(frame, spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 320.0f;
  options.rootHeight = 180.0f;
  layoutEngine.layout(frame, layout, options);

  float contentW = 320.0f - spec.sidebarWidth - spec.inspectorWidth;
  float contentH = 180.0f - spec.topbarHeight - spec.statusHeight;

  PrimeFrame::LayoutOut const* topbar = layout.get(shell.topbar.nodeId());
  PrimeFrame::LayoutOut const* status = layout.get(shell.status.nodeId());
  PrimeFrame::LayoutOut const* sidebar = layout.get(shell.sidebar.nodeId());
  PrimeFrame::LayoutOut const* content = layout.get(shell.content.nodeId());
  PrimeFrame::LayoutOut const* inspector = layout.get(shell.inspector.nodeId());
  REQUIRE(topbar != nullptr);
  REQUIRE(status != nullptr);
  REQUIRE(sidebar != nullptr);
  REQUIRE(content != nullptr);
  REQUIRE(inspector != nullptr);

  CHECK(topbar->absW == doctest::Approx(320.0f));
  CHECK(topbar->absH == doctest::Approx(spec.topbarHeight));
  CHECK(topbar->absX == doctest::Approx(0.0f));
  CHECK(topbar->absY == doctest::Approx(0.0f));

  CHECK(status->absW == doctest::Approx(320.0f));
  CHECK(status->absH == doctest::Approx(spec.statusHeight));
  CHECK(status->absX == doctest::Approx(0.0f));
  CHECK(status->absY == doctest::Approx(180.0f - spec.statusHeight));

  CHECK(sidebar->absW == doctest::Approx(spec.sidebarWidth));
  CHECK(sidebar->absH == doctest::Approx(contentH));
  CHECK(sidebar->absX == doctest::Approx(0.0f));
  CHECK(sidebar->absY == doctest::Approx(spec.topbarHeight));

  CHECK(inspector->absW == doctest::Approx(spec.inspectorWidth));
  CHECK(inspector->absH == doctest::Approx(contentH));
  CHECK(inspector->absX == doctest::Approx(320.0f - spec.inspectorWidth));
  CHECK(inspector->absY == doctest::Approx(spec.topbarHeight));

  CHECK(content->absW == doctest::Approx(contentW));
  CHECK(content->absH == doctest::Approx(contentH));
  CHECK(content->absX == doctest::Approx(spec.sidebarWidth));
  CHECK(content->absY == doctest::Approx(spec.topbarHeight));
}

TEST_CASE("PrimeStage shell draws dividers at expected positions") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  Studio::ShellSpec spec;
  spec.size.preferredWidth = 320.0f;
  spec.size.preferredHeight = 180.0f;
  spec.topbarHeight = 20.0f;
  spec.statusHeight = 18.0f;
  spec.sidebarWidth = 64.0f;
  spec.inspectorWidth = 52.0f;
  spec.drawDividers = true;

  Studio::ShellLayout shell = Studio::createShell(frame, spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 320.0f;
  options.rootHeight = 180.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::RectStyleToken dividerToken = Studio::rectToken(spec.dividerRole);
  PrimeFrame::Node const* rootNode = frame.getNode(shell.root.nodeId());
  REQUIRE(rootNode != nullptr);

  std::vector<PrimeFrame::LayoutOut const*> dividers;
  for (PrimeFrame::NodeId childId : rootNode->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (!child || child->primitives.empty()) {
      continue;
    }
    PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
    if (prim && prim->rect.token == dividerToken) {
      PrimeFrame::LayoutOut const* out = layout.get(childId);
      REQUIRE(out != nullptr);
      dividers.push_back(out);
    }
  }

  REQUIRE(dividers.size() == 4);

  float contentH = 180.0f - spec.topbarHeight - spec.statusHeight;
  float leftDividerX = spec.sidebarWidth - 1.0f;
  float rightDividerX = 320.0f - spec.inspectorWidth;
  bool foundTop = false;
  bool foundBottom = false;
  bool foundLeft = false;
  bool foundRight = false;
  for (PrimeFrame::LayoutOut const* out : dividers) {
    if (out->absH == doctest::Approx(1.0f) &&
        out->absW == doctest::Approx(320.0f) &&
        out->absY == doctest::Approx(spec.topbarHeight)) {
      foundTop = true;
    } else if (out->absH == doctest::Approx(1.0f) &&
               out->absW == doctest::Approx(320.0f) &&
               out->absY == doctest::Approx(180.0f - spec.statusHeight)) {
      foundBottom = true;
    } else if (out->absW == doctest::Approx(1.0f) &&
               out->absH == doctest::Approx(contentH) &&
               out->absX == doctest::Approx(leftDividerX) &&
               out->absY == doctest::Approx(spec.topbarHeight)) {
      foundLeft = true;
    } else if (out->absW == doctest::Approx(1.0f) &&
               out->absH == doctest::Approx(contentH) &&
               out->absX == doctest::Approx(rightDividerX) &&
               out->absY == doctest::Approx(spec.topbarHeight)) {
      foundRight = true;
    }
  }
  CHECK(foundTop);
  CHECK(foundBottom);
  CHECK(foundLeft);
  CHECK(foundRight);
}

TEST_CASE("PrimeStage progress bar min fill applies only when value positive") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 80.0f);

  Studio::ProgressBarSpec spec;
  spec.size.preferredWidth = 100.0f;
  spec.size.preferredHeight = 10.0f;
  spec.value = 0.0f;
  spec.minFillWidth = 20.0f;
  spec.trackRole = Studio::RectRole::PanelStrong;
  spec.fillRole = Studio::RectRole::Accent;
  PrimeStage::UiNode bar = Studio::createProgressBar(root, spec);

  PrimeFrame::Node const* barNode = frame.getNode(bar.nodeId());
  REQUIRE(barNode != nullptr);
  CHECK(barNode->children.empty());

  Studio::ProgressBarSpec spec2 = spec;
  spec2.value = 0.1f;
  PrimeStage::UiNode bar2 = Studio::createProgressBar(root, spec2);

  PrimeFrame::Node const* barNode2 = frame.getNode(bar2.nodeId());
  REQUIRE(barNode2 != nullptr);
  REQUIRE(barNode2->children.size() == 1);
  PrimeFrame::Node const* fillNode = frame.getNode(barNode2->children[0]);
  REQUIRE(fillNode != nullptr);
  REQUIRE(fillNode->sizeHint.width.preferred.has_value());
  CHECK(fillNode->sizeHint.width.preferred.value() == doctest::Approx(20.0f));
}

TEST_CASE("PrimeStage scroll view creates a node") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 120.0f);

  PrimeStage::ScrollViewSpec spec;
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 120.0f;
  spec.vertical.thumbLength = 24.0f;
  spec.horizontal.thumbLength = 24.0f;
  PrimeStage::ScrollView scroll = root.createScrollView(spec);
  CHECK(frame.getNode(scroll.root.nodeId()) != nullptr);
  CHECK(frame.getNode(scroll.content.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage button creates a node") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 120.0f, 40.0f);

  PrimeStage::ButtonSpec spec;
  spec.size.preferredWidth = 80.0f;
  spec.size.preferredHeight = 24.0f;
  spec.label = "Click";
  spec.backgroundStyle = Studio::rectToken(Studio::RectRole::Accent);
  spec.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  PrimeStage::UiNode button = root.createButton(spec);
  CHECK(frame.getNode(button.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage text field creates a node") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  PrimeStage::TextFieldSpec spec;
  spec.size.preferredWidth = 160.0f;
  spec.size.preferredHeight = 24.0f;
  spec.placeholder = "Search...";
  PrimeStage::UiNode field = root.createTextField(spec);
  CHECK(frame.getNode(field.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage text field uses placeholder when empty") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  PrimeStage::TextFieldSpec spec;
  spec.size.preferredWidth = 160.0f;
  spec.size.preferredHeight = 24.0f;
  spec.placeholder = "Search...";
  PrimeStage::UiNode field = root.createTextField(spec);

  PrimeFrame::Node const* fieldNode = frame.getNode(field.nodeId());
  REQUIRE(fieldNode != nullptr);
  bool foundPlaceholder = false;
  for (PrimeFrame::NodeId childId : fieldNode->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (!child || child->primitives.empty()) {
      continue;
    }
    PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
    if (prim && prim->type == PrimeFrame::PrimitiveType::Text &&
        prim->textBlock.text == spec.placeholder) {
      foundPlaceholder = true;
      break;
    }
  }
  CHECK(foundPlaceholder);
}

TEST_CASE("PrimeStage text field prefers text over placeholder") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  PrimeStage::TextFieldSpec spec;
  spec.size.preferredWidth = 160.0f;
  spec.size.preferredHeight = 24.0f;
  spec.text = "Hello";
  spec.placeholder = "Search...";
  PrimeStage::UiNode field = root.createTextField(spec);

  PrimeFrame::Node const* fieldNode = frame.getNode(field.nodeId());
  REQUIRE(fieldNode != nullptr);
  bool foundText = false;
  bool foundPlaceholder = false;
  for (PrimeFrame::NodeId childId : fieldNode->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (!child || child->primitives.empty()) {
      continue;
    }
    PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
    if (prim && prim->type == PrimeFrame::PrimitiveType::Text) {
      if (prim->textBlock.text == spec.text) {
        foundText = true;
      } else if (prim->textBlock.text == spec.placeholder) {
        foundPlaceholder = true;
      }
    }
  }
  CHECK(foundText);
  CHECK(!foundPlaceholder);
}

TEST_CASE("PrimeStage text field inferred width grows with placeholder") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  PrimeStage::TextFieldSpec shortSpec;
  shortSpec.placeholder = "A";
  shortSpec.backgroundStyle = Studio::rectToken(Studio::RectRole::Panel);
  shortSpec.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  shortSpec.placeholderStyle = Studio::textToken(Studio::TextRole::BodyMuted);
  PrimeStage::UiNode shortField = root.createTextField(shortSpec);

  PrimeStage::TextFieldSpec longSpec = shortSpec;
  longSpec.placeholder = "LongerPlaceholder";
  PrimeStage::UiNode longField = root.createTextField(longSpec);

  PrimeFrame::Node const* shortNode = frame.getNode(shortField.nodeId());
  PrimeFrame::Node const* longNode = frame.getNode(longField.nodeId());
  REQUIRE(shortNode != nullptr);
  REQUIRE(longNode != nullptr);
  REQUIRE(shortNode->sizeHint.width.preferred.has_value());
  REQUIRE(longNode->sizeHint.width.preferred.has_value());
  CHECK(longNode->sizeHint.width.preferred.value() >
        shortNode->sizeHint.width.preferred.value());
}

TEST_CASE("PrimeStage text field inferred width grows with text") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  PrimeStage::TextFieldSpec shortSpec;
  shortSpec.text = "Hi";
  shortSpec.backgroundStyle = Studio::rectToken(Studio::RectRole::Panel);
  shortSpec.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  shortSpec.placeholderStyle = Studio::textToken(Studio::TextRole::BodyMuted);
  PrimeStage::UiNode shortField = root.createTextField(shortSpec);

  PrimeStage::TextFieldSpec longSpec = shortSpec;
  longSpec.text = "Hello there";
  PrimeStage::UiNode longField = root.createTextField(longSpec);

  PrimeFrame::Node const* shortNode = frame.getNode(shortField.nodeId());
  PrimeFrame::Node const* longNode = frame.getNode(longField.nodeId());
  REQUIRE(shortNode != nullptr);
  REQUIRE(longNode != nullptr);
  REQUIRE(shortNode->sizeHint.width.preferred.has_value());
  REQUIRE(longNode->sizeHint.width.preferred.has_value());
  CHECK(longNode->sizeHint.width.preferred.value() >
        shortNode->sizeHint.width.preferred.value());
}

TEST_CASE("PrimeStage text field inferred height grows with text style") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  PrimeStage::TextFieldSpec smallSpec;
  smallSpec.text = "Hello";
  smallSpec.backgroundStyle = Studio::rectToken(Studio::RectRole::Panel);
  smallSpec.textStyle = Studio::textToken(Studio::TextRole::SmallMuted);
  smallSpec.placeholderStyle = Studio::textToken(Studio::TextRole::BodyMuted);
  PrimeStage::UiNode smallField = root.createTextField(smallSpec);

  PrimeStage::TextFieldSpec largeSpec = smallSpec;
  largeSpec.textStyle = Studio::textToken(Studio::TextRole::TitleBright);
  PrimeStage::UiNode largeField = root.createTextField(largeSpec);

  PrimeFrame::Node const* smallNode = frame.getNode(smallField.nodeId());
  PrimeFrame::Node const* largeNode = frame.getNode(largeField.nodeId());
  REQUIRE(smallNode != nullptr);
  REQUIRE(largeNode != nullptr);
  REQUIRE(smallNode->sizeHint.height.preferred.has_value());
  REQUIRE(largeNode->sizeHint.height.preferred.has_value());
  CHECK(largeNode->sizeHint.height.preferred.value() >
        smallNode->sizeHint.height.preferred.value());
}

TEST_CASE("PrimeStage text field inferred height grows with placeholder style") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  PrimeStage::TextFieldSpec smallSpec;
  smallSpec.placeholder = "Search...";
  smallSpec.backgroundStyle = Studio::rectToken(Studio::RectRole::Panel);
  smallSpec.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  smallSpec.placeholderStyle = Studio::textToken(Studio::TextRole::SmallMuted);
  PrimeStage::UiNode smallField = root.createTextField(smallSpec);

  PrimeStage::TextFieldSpec largeSpec = smallSpec;
  largeSpec.placeholderStyle = Studio::textToken(Studio::TextRole::TitleBright);
  PrimeStage::UiNode largeField = root.createTextField(largeSpec);

  PrimeFrame::Node const* smallNode = frame.getNode(smallField.nodeId());
  PrimeFrame::Node const* largeNode = frame.getNode(largeField.nodeId());
  REQUIRE(smallNode != nullptr);
  REQUIRE(largeNode != nullptr);
  REQUIRE(smallNode->sizeHint.height.preferred.has_value());
  REQUIRE(largeNode->sizeHint.height.preferred.has_value());
  CHECK(largeNode->sizeHint.height.preferred.value() ==
        doctest::Approx(smallNode->sizeHint.height.preferred.value()));
}
TEST_CASE("PrimeStage toggle creates a node") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 80.0f, 40.0f);

  PrimeStage::ToggleSpec spec;
  spec.size.preferredWidth = 48.0f;
  spec.size.preferredHeight = 24.0f;
  spec.trackStyle = Studio::rectToken(Studio::RectRole::Panel);
  spec.knobStyle = Studio::rectToken(Studio::RectRole::Accent);
  spec.on = true;
  PrimeStage::UiNode toggle = root.createToggle(spec);
  CHECK(frame.getNode(toggle.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage toggle hidden has no knob") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 80.0f, 40.0f);

  PrimeStage::ToggleSpec spec;
  spec.size.preferredWidth = 48.0f;
  spec.size.preferredHeight = 24.0f;
  spec.trackStyle = Studio::rectToken(Studio::RectRole::Panel);
  spec.knobStyle = Studio::rectToken(Studio::RectRole::Accent);
  spec.visible = false;
  PrimeStage::UiNode toggle = root.createToggle(spec);

  PrimeFrame::Node const* toggleNode = frame.getNode(toggle.nodeId());
  REQUIRE(toggleNode != nullptr);
  CHECK(toggleNode->children.empty());
}

TEST_CASE("PrimeStage toggle defaults size when not provided") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 80.0f, 40.0f);

  PrimeStage::ToggleSpec spec;
  spec.trackStyle = Studio::rectToken(Studio::RectRole::Panel);
  spec.knobStyle = Studio::rectToken(Studio::RectRole::Accent);
  PrimeStage::UiNode toggle = root.createToggle(spec);

  PrimeFrame::Node const* toggleNode = frame.getNode(toggle.nodeId());
  REQUIRE(toggleNode != nullptr);
  REQUIRE(toggleNode->sizeHint.width.preferred.has_value());
  REQUIRE(toggleNode->sizeHint.height.preferred.has_value());
  CHECK(toggleNode->sizeHint.width.preferred.value() == doctest::Approx(40.0f));
  CHECK(toggleNode->sizeHint.height.preferred.value() == doctest::Approx(20.0f));
}

TEST_CASE("PrimeStage toggle knob moves with on state") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 80.0f, 40.0f);

  PrimeStage::ToggleSpec offSpec;
  offSpec.size.preferredWidth = 60.0f;
  offSpec.size.preferredHeight = 20.0f;
  offSpec.knobInset = 2.0f;
  offSpec.trackStyle = Studio::rectToken(Studio::RectRole::Panel);
  offSpec.knobStyle = Studio::rectToken(Studio::RectRole::Accent);
  offSpec.on = false;
  PrimeStage::UiNode offToggle = root.createToggle(offSpec);

  PrimeStage::ToggleSpec onSpec = offSpec;
  onSpec.on = true;
  PrimeStage::UiNode onToggle = root.createToggle(onSpec);

  auto find_knob_x = [&](PrimeStage::UiNode const& node) {
    PrimeFrame::Node const* toggleNode = frame.getNode(node.nodeId());
    REQUIRE(toggleNode != nullptr);
    REQUIRE(toggleNode->children.size() == 1);
    PrimeFrame::Node const* knobNode = frame.getNode(toggleNode->children[0]);
    REQUIRE(knobNode != nullptr);
    return knobNode->localX;
  };

  float offX = find_knob_x(offToggle);
  float onX = find_knob_x(onToggle);
  CHECK(onX > offX);
}

TEST_CASE("PrimeStage toggle clamps knob inset") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 80.0f, 40.0f);

  PrimeStage::ToggleSpec spec;
  spec.size.preferredWidth = 40.0f;
  spec.size.preferredHeight = 10.0f;
  spec.knobInset = 10.0f;
  spec.trackStyle = Studio::rectToken(Studio::RectRole::Panel);
  spec.knobStyle = Studio::rectToken(Studio::RectRole::Accent);
  PrimeStage::UiNode toggle = root.createToggle(spec);

  PrimeFrame::Node const* toggleNode = frame.getNode(toggle.nodeId());
  REQUIRE(toggleNode != nullptr);
  REQUIRE(toggleNode->children.size() == 1);

  PrimeFrame::Node const* knobNode = frame.getNode(toggleNode->children[0]);
  REQUIRE(knobNode != nullptr);

  CHECK(knobNode->localX == doctest::Approx(10.0f));
  CHECK(!knobNode->sizeHint.width.preferred.has_value());
  CHECK(!knobNode->sizeHint.height.preferred.has_value());
}

TEST_CASE("PrimeStage toggle on state clamps knob inset") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 80.0f, 40.0f);

  PrimeStage::ToggleSpec spec;
  spec.size.preferredWidth = 40.0f;
  spec.size.preferredHeight = 10.0f;
  spec.knobInset = 10.0f;
  spec.trackStyle = Studio::rectToken(Studio::RectRole::Panel);
  spec.knobStyle = Studio::rectToken(Studio::RectRole::Accent);
  spec.on = true;
  PrimeStage::UiNode toggle = root.createToggle(spec);

  PrimeFrame::Node const* toggleNode = frame.getNode(toggle.nodeId());
  REQUIRE(toggleNode != nullptr);
  REQUIRE(toggleNode->children.size() == 1);

  PrimeFrame::Node const* knobNode = frame.getNode(toggleNode->children[0]);
  REQUIRE(knobNode != nullptr);

  CHECK(knobNode->localX == doctest::Approx(30.0f));
  CHECK(!knobNode->sizeHint.width.preferred.has_value());
  CHECK(!knobNode->sizeHint.height.preferred.has_value());
}

TEST_CASE("PrimeStage checkbox creates a node") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 160.0f, 40.0f);

  PrimeStage::CheckboxSpec spec;
  spec.label = "Enabled";
  spec.checked = true;
  spec.boxStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  spec.checkStyle = Studio::rectToken(Studio::RectRole::Accent);
  spec.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  PrimeStage::UiNode checkbox = root.createCheckbox(spec);
  CHECK(frame.getNode(checkbox.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage checkbox hidden skips checkmark") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 160.0f, 40.0f);

  PrimeStage::CheckboxSpec spec;
  spec.label = "Enabled";
  spec.checked = true;
  spec.visible = false;
  spec.boxStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  spec.checkStyle = Studio::rectToken(Studio::RectRole::Accent);
  spec.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  PrimeStage::UiNode checkbox = root.createCheckbox(spec);

  PrimeFrame::Node const* rowNode = frame.getNode(checkbox.nodeId());
  REQUIRE(rowNode != nullptr);
  REQUIRE(!rowNode->children.empty());
  PrimeFrame::Node const* boxNode = frame.getNode(rowNode->children[0]);
  REQUIRE(boxNode != nullptr);
  CHECK(boxNode->children.empty());
}

TEST_CASE("PrimeStage checkbox unchecked skips checkmark") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 160.0f, 40.0f);

  PrimeStage::CheckboxSpec spec;
  spec.label = "Enabled";
  spec.checked = false;
  spec.boxStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  spec.checkStyle = Studio::rectToken(Studio::RectRole::Accent);
  spec.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  PrimeStage::UiNode checkbox = root.createCheckbox(spec);

  PrimeFrame::Node const* rowNode = frame.getNode(checkbox.nodeId());
  REQUIRE(rowNode != nullptr);
  REQUIRE(!rowNode->children.empty());
  PrimeFrame::Node const* boxNode = frame.getNode(rowNode->children[0]);
  REQUIRE(boxNode != nullptr);
  CHECK(boxNode->children.empty());
}

TEST_CASE("PrimeStage checkbox height follows box size when larger") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 60.0f);

  PrimeStage::CheckboxSpec spec;
  spec.label = "Enabled";
  spec.boxSize = 32.0f;
  spec.boxStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  spec.checkStyle = Studio::rectToken(Studio::RectRole::Accent);
  spec.textStyle = Studio::textToken(Studio::TextRole::SmallMuted);
  PrimeStage::UiNode checkbox = root.createCheckbox(spec);

  PrimeFrame::Node const* rowNode = frame.getNode(checkbox.nodeId());
  REQUIRE(rowNode != nullptr);
  REQUIRE(rowNode->sizeHint.height.preferred.has_value());
  CHECK(rowNode->sizeHint.height.preferred.value() == doctest::Approx(32.0f));
}

TEST_CASE("PrimeStage checkbox height follows text when larger") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 60.0f);

  PrimeStage::CheckboxSpec spec;
  spec.label = "Enabled";
  spec.boxSize = 12.0f;
  spec.boxStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  spec.checkStyle = Studio::rectToken(Studio::RectRole::Accent);
  spec.textStyle = Studio::textToken(Studio::TextRole::TitleBright);
  PrimeStage::UiNode checkbox = root.createCheckbox(spec);

  PrimeFrame::Node const* rowNode = frame.getNode(checkbox.nodeId());
  REQUIRE(rowNode != nullptr);
  REQUIRE(rowNode->sizeHint.height.preferred.has_value());
  CHECK(rowNode->sizeHint.height.preferred.value() > doctest::Approx(12.0f));
}
TEST_CASE("PrimeStage checkbox inferred width grows with label") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  PrimeStage::CheckboxSpec shortSpec;
  shortSpec.label = "On";
  shortSpec.boxStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  shortSpec.checkStyle = Studio::rectToken(Studio::RectRole::Accent);
  shortSpec.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  PrimeStage::UiNode shortBox = root.createCheckbox(shortSpec);

  PrimeStage::CheckboxSpec longSpec = shortSpec;
  longSpec.label = "Enable Feature";
  PrimeStage::UiNode longBox = root.createCheckbox(longSpec);

  PrimeFrame::Node const* shortNode = frame.getNode(shortBox.nodeId());
  PrimeFrame::Node const* longNode = frame.getNode(longBox.nodeId());
  REQUIRE(shortNode != nullptr);
  REQUIRE(longNode != nullptr);
  REQUIRE(shortNode->sizeHint.width.preferred.has_value());
  REQUIRE(longNode->sizeHint.width.preferred.has_value());
  CHECK(longNode->sizeHint.width.preferred.value() >
        shortNode->sizeHint.width.preferred.value());
}

TEST_CASE("PrimeStage checkbox inferred width grows with gap") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  PrimeStage::CheckboxSpec smallGap;
  smallGap.label = "On";
  smallGap.gap = 4.0f;
  smallGap.boxStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  smallGap.checkStyle = Studio::rectToken(Studio::RectRole::Accent);
  smallGap.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  PrimeStage::UiNode smallBox = root.createCheckbox(smallGap);

  PrimeStage::CheckboxSpec largeGap = smallGap;
  largeGap.gap = 12.0f;
  PrimeStage::UiNode largeBox = root.createCheckbox(largeGap);

  PrimeFrame::Node const* smallNode = frame.getNode(smallBox.nodeId());
  PrimeFrame::Node const* largeNode = frame.getNode(largeBox.nodeId());
  REQUIRE(smallNode != nullptr);
  REQUIRE(largeNode != nullptr);
  REQUIRE(smallNode->sizeHint.width.preferred.has_value());
  REQUIRE(largeNode->sizeHint.width.preferred.has_value());
  CHECK(largeNode->sizeHint.width.preferred.value() -
        smallNode->sizeHint.width.preferred.value() ==
        doctest::Approx(8.0f));
}

TEST_CASE("PrimeStage checkbox inferred height grows with text style") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 60.0f);

  PrimeStage::CheckboxSpec smallSpec;
  smallSpec.label = "On";
  smallSpec.boxStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  smallSpec.checkStyle = Studio::rectToken(Studio::RectRole::Accent);
  smallSpec.textStyle = Studio::textToken(Studio::TextRole::SmallMuted);
  PrimeStage::UiNode smallBox = root.createCheckbox(smallSpec);

  PrimeStage::CheckboxSpec largeSpec = smallSpec;
  largeSpec.textStyle = Studio::textToken(Studio::TextRole::TitleBright);
  PrimeStage::UiNode largeBox = root.createCheckbox(largeSpec);

  PrimeFrame::Node const* smallNode = frame.getNode(smallBox.nodeId());
  PrimeFrame::Node const* largeNode = frame.getNode(largeBox.nodeId());
  REQUIRE(smallNode != nullptr);
  REQUIRE(largeNode != nullptr);
  REQUIRE(smallNode->sizeHint.height.preferred.has_value());
  REQUIRE(largeNode->sizeHint.height.preferred.has_value());
  CHECK(largeNode->sizeHint.height.preferred.value() >
        smallNode->sizeHint.height.preferred.value());
}

TEST_CASE("PrimeStage slider creates a node") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  PrimeStage::SliderSpec spec;
  spec.size.preferredWidth = 160.0f;
  spec.size.preferredHeight = 24.0f;
  spec.value = 0.6f;
  spec.trackStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  spec.fillStyle = Studio::rectToken(Studio::RectRole::Accent);
  spec.thumbStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  PrimeStage::UiNode slider = root.createSlider(spec);
  CHECK(frame.getNode(slider.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage slider callback updates on drag") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  float value = 0.0f;
  PrimeStage::SliderSpec spec;
  spec.size.preferredWidth = 160.0f;
  spec.size.preferredHeight = 24.0f;
  spec.trackStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  spec.fillStyle = Studio::rectToken(Studio::RectRole::Accent);
  spec.thumbStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  spec.callbacks.onValueChanged = [&](float next) { value = next; };
  PrimeStage::UiNode slider = root.createSlider(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 200.0f;
  options.rootHeight = 40.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* out = layout.get(slider.nodeId());
  REQUIRE(out != nullptr);
  float y = out->absY + out->absH * 0.5f;
  float x0 = out->absX + out->absW * 0.1f;
  float x1 = out->absX + out->absW * 0.9f;

  PrimeFrame::EventRouter router;
  PrimeFrame::Event down;
  down.type = PrimeFrame::EventType::PointerDown;
  down.pointerId = 1;
  down.x = x0;
  down.y = y;
  router.dispatch(down, frame, layout, nullptr);

  PrimeFrame::Event move;
  move.type = PrimeFrame::EventType::PointerMove;
  move.pointerId = 1;
  move.x = x1;
  move.y = y;
  router.dispatch(move, frame, layout, nullptr);

  CHECK(value > 0.5f);
}

TEST_CASE("PrimeStage slider clamps when dragged outside") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 220.0f, 40.0f);

  float value = 0.0f;
  PrimeStage::SliderSpec spec;
  spec.size.preferredWidth = 180.0f;
  spec.size.preferredHeight = 24.0f;
  spec.value = 0.5f;
  spec.trackStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  spec.fillStyle = Studio::rectToken(Studio::RectRole::Accent);
  spec.thumbStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  spec.callbacks.onValueChanged = [&](float next) { value = next; };
  PrimeStage::UiNode slider = root.createSlider(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 220.0f;
  options.rootHeight = 40.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* out = layout.get(slider.nodeId());
  REQUIRE(out != nullptr);
  float y = out->absY + out->absH * 0.5f;
  float xInside = out->absX + out->absW * 0.5f;
  float xOutside = out->absX - 80.0f;

  PrimeFrame::EventRouter router;
  PrimeFrame::Event down;
  down.type = PrimeFrame::EventType::PointerDown;
  down.pointerId = 1;
  down.x = xInside;
  down.y = y;
  router.dispatch(down, frame, layout, nullptr);

  PrimeFrame::Event move;
  move.type = PrimeFrame::EventType::PointerMove;
  move.pointerId = 1;
  move.x = xOutside;
  move.y = y;
  router.dispatch(move, frame, layout, nullptr);

  CHECK(value == doctest::Approx(0.0f));
  PrimeFrame::Node const* sliderNode = frame.getNode(slider.nodeId());
  REQUIRE(sliderNode != nullptr);
  REQUIRE(sliderNode->primitives.size() >= 2);
  PrimeFrame::Primitive const* fillPrim = frame.getPrimitive(sliderNode->primitives[1]);
  REQUIRE(fillPrim != nullptr);
  CHECK(fillPrim->width == doctest::Approx(0.0f));
  REQUIRE(fillPrim->rect.overrideStyle.opacity.has_value());
  CHECK(fillPrim->rect.overrideStyle.opacity.value() == doctest::Approx(0.0f));
}

TEST_CASE("PrimeStage slider hover updates fill opacity") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  PrimeStage::SliderSpec spec;
  spec.size.preferredWidth = 160.0f;
  spec.size.preferredHeight = 24.0f;
  spec.value = 0.5f;
  spec.trackStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  spec.fillStyle = Studio::rectToken(Studio::RectRole::Accent);
  spec.thumbStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  spec.fillStyleOverride.opacity = 0.6f;
  spec.fillHoverOpacity = 1.0f;
  spec.callbacks.onValueChanged = [](float) {};
  PrimeStage::UiNode slider = root.createSlider(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 200.0f;
  options.rootHeight = 40.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* out = layout.get(slider.nodeId());
  REQUIRE(out != nullptr);
  float x = out->absX + out->absW * 0.5f;
  float y = out->absY + out->absH * 0.5f;

  PrimeFrame::EventRouter router;
  PrimeFrame::Event move;
  move.type = PrimeFrame::EventType::PointerMove;
  move.pointerId = 1;
  move.x = x;
  move.y = y;
  router.dispatch(move, frame, layout, nullptr);

  PrimeFrame::Node const* sliderNode = frame.getNode(slider.nodeId());
  REQUIRE(sliderNode != nullptr);
  REQUIRE(sliderNode->primitives.size() >= 2);
  PrimeFrame::Primitive const* fillPrim = frame.getPrimitive(sliderNode->primitives[1]);
  REQUIRE(fillPrim != nullptr);
  REQUIRE(fillPrim->rect.overrideStyle.opacity.has_value());
  CHECK(fillPrim->rect.overrideStyle.opacity.value() == doctest::Approx(1.0f));
}

TEST_CASE("PrimeStage button hover and press update style") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 80.0f);

  bool clicked = false;
  bool hovered = false;
  bool pressed = false;
  PrimeStage::ButtonSpec spec;
  spec.label = "Action";
  spec.size.preferredWidth = 120.0f;
  spec.size.preferredHeight = 32.0f;
  spec.backgroundStyle = Studio::rectToken(Studio::RectRole::ButtonBase);
  spec.hoverStyle = Studio::rectToken(Studio::RectRole::ButtonHover);
  spec.pressedStyle = Studio::rectToken(Studio::RectRole::ButtonPressed);
  spec.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  spec.callbacks.onClick = [&]() { clicked = true; };
  spec.callbacks.onHoverChanged = [&](bool value) { hovered = value; };
  spec.callbacks.onPressedChanged = [&](bool value) { pressed = value; };
  PrimeStage::UiNode button = root.createButton(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 240.0f;
  options.rootHeight = 80.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* out = layout.get(button.nodeId());
  REQUIRE(out != nullptr);
  float x = out->absX + out->absW * 0.5f;
  float y = out->absY + out->absH * 0.5f;

  PrimeFrame::EventRouter router;
  PrimeFrame::Event move;
  move.type = PrimeFrame::EventType::PointerMove;
  move.pointerId = 1;
  move.x = x;
  move.y = y;
  router.dispatch(move, frame, layout, nullptr);

  CHECK(hovered);
  PrimeFrame::Node const* buttonNode = frame.getNode(button.nodeId());
  REQUIRE(buttonNode != nullptr);
  REQUIRE(!buttonNode->primitives.empty());
  PrimeFrame::Primitive const* bg = frame.getPrimitive(buttonNode->primitives.front());
  REQUIRE(bg != nullptr);
  CHECK(bg->rect.token == Studio::rectToken(Studio::RectRole::ButtonHover));

  PrimeFrame::Event down;
  down.type = PrimeFrame::EventType::PointerDown;
  down.pointerId = 1;
  down.x = x;
  down.y = y;
  router.dispatch(down, frame, layout, nullptr);

  CHECK(pressed);
  bg = frame.getPrimitive(buttonNode->primitives.front());
  REQUIRE(bg != nullptr);
  CHECK(bg->rect.token == Studio::rectToken(Studio::RectRole::ButtonPressed));

  PrimeFrame::Event up;
  up.type = PrimeFrame::EventType::PointerUp;
  up.pointerId = 1;
  up.x = x;
  up.y = y;
  router.dispatch(up, frame, layout, nullptr);

  CHECK(clicked);
  CHECK(!pressed);
  CHECK(hovered);
  bg = frame.getPrimitive(buttonNode->primitives.front());
  REQUIRE(bg != nullptr);
  CHECK(bg->rect.token == Studio::rectToken(Studio::RectRole::ButtonHover));
}

TEST_CASE("PrimeStage button drag outside cancels press and click") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 80.0f);

  bool clicked = false;
  bool hovered = false;
  bool pressed = false;
  PrimeStage::ButtonSpec spec;
  spec.label = "Action";
  spec.size.preferredWidth = 120.0f;
  spec.size.preferredHeight = 32.0f;
  spec.backgroundStyle = Studio::rectToken(Studio::RectRole::ButtonBase);
  spec.hoverStyle = Studio::rectToken(Studio::RectRole::ButtonHover);
  spec.pressedStyle = Studio::rectToken(Studio::RectRole::ButtonPressed);
  spec.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  spec.callbacks.onClick = [&]() { clicked = true; };
  spec.callbacks.onHoverChanged = [&](bool value) { hovered = value; };
  spec.callbacks.onPressedChanged = [&](bool value) { pressed = value; };
  PrimeStage::UiNode button = root.createButton(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 240.0f;
  options.rootHeight = 80.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* out = layout.get(button.nodeId());
  REQUIRE(out != nullptr);
  float xInside = out->absX + out->absW * 0.5f;
  float yInside = out->absY + out->absH * 0.5f;
  float xOutside = out->absX - 40.0f;

  PrimeFrame::EventRouter router;
  PrimeFrame::Event down;
  down.type = PrimeFrame::EventType::PointerDown;
  down.pointerId = 1;
  down.x = xInside;
  down.y = yInside;
  router.dispatch(down, frame, layout, nullptr);

  CHECK(pressed);

  PrimeFrame::Event move;
  move.type = PrimeFrame::EventType::PointerMove;
  move.pointerId = 1;
  move.x = xOutside;
  move.y = yInside;
  router.dispatch(move, frame, layout, nullptr);

  CHECK(!pressed);
  CHECK(!hovered);

  PrimeFrame::Event up;
  up.type = PrimeFrame::EventType::PointerUp;
  up.pointerId = 1;
  up.x = xOutside;
  up.y = yInside;
  router.dispatch(up, frame, layout, nullptr);

  CHECK(!clicked);
  PrimeFrame::Node const* buttonNode = frame.getNode(button.nodeId());
  REQUIRE(buttonNode != nullptr);
  REQUIRE(!buttonNode->primitives.empty());
  PrimeFrame::Primitive const* bg = frame.getPrimitive(buttonNode->primitives.front());
  REQUIRE(bg != nullptr);
  CHECK(bg->rect.token == Studio::rectToken(Studio::RectRole::ButtonBase));
}

TEST_CASE("PrimeStage button hover leave restores base style") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 80.0f);

  bool hovered = false;
  PrimeStage::ButtonSpec spec;
  spec.label = "Action";
  spec.size.preferredWidth = 120.0f;
  spec.size.preferredHeight = 32.0f;
  spec.backgroundStyle = Studio::rectToken(Studio::RectRole::ButtonBase);
  spec.hoverStyle = Studio::rectToken(Studio::RectRole::ButtonHover);
  spec.pressedStyle = Studio::rectToken(Studio::RectRole::ButtonPressed);
  spec.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  spec.callbacks.onHoverChanged = [&](bool value) { hovered = value; };
  PrimeStage::UiNode button = root.createButton(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 200.0f;
  options.rootHeight = 80.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* out = layout.get(button.nodeId());
  REQUIRE(out != nullptr);
  float xInside = out->absX + out->absW * 0.5f;
  float yInside = out->absY + out->absH * 0.5f;
  float xOutside = out->absX - 60.0f;

  PrimeFrame::EventRouter router;
  PrimeFrame::Event moveInside;
  moveInside.type = PrimeFrame::EventType::PointerMove;
  moveInside.pointerId = 1;
  moveInside.x = xInside;
  moveInside.y = yInside;
  router.dispatch(moveInside, frame, layout, nullptr);

  CHECK(hovered);
  PrimeFrame::Node const* buttonNode = frame.getNode(button.nodeId());
  REQUIRE(buttonNode != nullptr);
  REQUIRE(!buttonNode->primitives.empty());
  PrimeFrame::Primitive const* bg = frame.getPrimitive(buttonNode->primitives.front());
  REQUIRE(bg != nullptr);
  CHECK(bg->rect.token == Studio::rectToken(Studio::RectRole::ButtonHover));

  PrimeFrame::Event moveOutside;
  moveOutside.type = PrimeFrame::EventType::PointerMove;
  moveOutside.pointerId = 1;
  moveOutside.x = xOutside;
  moveOutside.y = yInside;
  router.dispatch(moveOutside, frame, layout, nullptr);

  CHECK(!hovered);
  bg = frame.getPrimitive(buttonNode->primitives.front());
  REQUIRE(bg != nullptr);
  CHECK(bg->rect.token == Studio::rectToken(Studio::RectRole::ButtonBase));
}

TEST_CASE("PrimeStage slider move without press does not change value") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  float value = 0.25f;
  int updates = 0;
  PrimeStage::SliderSpec spec;
  spec.size.preferredWidth = 160.0f;
  spec.size.preferredHeight = 24.0f;
  spec.value = value;
  spec.trackStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  spec.fillStyle = Studio::rectToken(Studio::RectRole::Accent);
  spec.thumbStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  spec.callbacks.onValueChanged = [&](float next) {
    value = next;
    updates++;
  };
  PrimeStage::UiNode slider = root.createSlider(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 200.0f;
  options.rootHeight = 40.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* out = layout.get(slider.nodeId());
  REQUIRE(out != nullptr);
  float x = out->absX + out->absW * 0.8f;
  float y = out->absY + out->absH * 0.5f;

  PrimeFrame::EventRouter router;
  PrimeFrame::Event move;
  move.type = PrimeFrame::EventType::PointerMove;
  move.pointerId = 1;
  move.x = x;
  move.y = y;
  router.dispatch(move, frame, layout, nullptr);

  CHECK(updates == 0);
  CHECK(value == doctest::Approx(0.25f));
}

TEST_CASE("PrimeStage slider pressed opacity overrides hover") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  PrimeStage::SliderSpec spec;
  spec.size.preferredWidth = 160.0f;
  spec.size.preferredHeight = 24.0f;
  spec.value = 0.5f;
  spec.trackStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  spec.fillStyle = Studio::rectToken(Studio::RectRole::Accent);
  spec.thumbStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  spec.fillStyleOverride.opacity = 0.6f;
  spec.fillHoverOpacity = 0.9f;
  spec.fillPressedOpacity = 0.4f;
  spec.callbacks.onValueChanged = [](float) {};
  PrimeStage::UiNode slider = root.createSlider(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 200.0f;
  options.rootHeight = 40.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* out = layout.get(slider.nodeId());
  REQUIRE(out != nullptr);
  float x = out->absX + out->absW * 0.5f;
  float y = out->absY + out->absH * 0.5f;

  PrimeFrame::EventRouter router;
  PrimeFrame::Event move;
  move.type = PrimeFrame::EventType::PointerMove;
  move.pointerId = 1;
  move.x = x;
  move.y = y;
  router.dispatch(move, frame, layout, nullptr);

  PrimeFrame::Node const* sliderNode = frame.getNode(slider.nodeId());
  REQUIRE(sliderNode != nullptr);
  REQUIRE(sliderNode->primitives.size() >= 2);
  PrimeFrame::Primitive const* fillPrim = frame.getPrimitive(sliderNode->primitives[1]);
  REQUIRE(fillPrim != nullptr);
  REQUIRE(fillPrim->rect.overrideStyle.opacity.has_value());
  CHECK(fillPrim->rect.overrideStyle.opacity.value() == doctest::Approx(0.9f));

  PrimeFrame::Event down;
  down.type = PrimeFrame::EventType::PointerDown;
  down.pointerId = 1;
  down.x = x;
  down.y = y;
  router.dispatch(down, frame, layout, nullptr);

  fillPrim = frame.getPrimitive(sliderNode->primitives[1]);
  REQUIRE(fillPrim != nullptr);
  REQUIRE(fillPrim->rect.overrideStyle.opacity.has_value());
  CHECK(fillPrim->rect.overrideStyle.opacity.value() == doctest::Approx(0.4f));
}

TEST_CASE("PrimeStage slider hover leave restores fill opacity") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  PrimeStage::SliderSpec spec;
  spec.size.preferredWidth = 160.0f;
  spec.size.preferredHeight = 24.0f;
  spec.value = 0.5f;
  spec.trackStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  spec.fillStyle = Studio::rectToken(Studio::RectRole::Accent);
  spec.thumbStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  spec.fillStyleOverride.opacity = 0.5f;
  spec.fillHoverOpacity = 0.9f;
  spec.callbacks.onValueChanged = [](float) {};
  PrimeStage::UiNode slider = root.createSlider(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 200.0f;
  options.rootHeight = 40.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* out = layout.get(slider.nodeId());
  REQUIRE(out != nullptr);
  float xInside = out->absX + out->absW * 0.5f;
  float yInside = out->absY + out->absH * 0.5f;
  float xOutside = out->absX - 60.0f;

  PrimeFrame::EventRouter router;
  PrimeFrame::Event moveInside;
  moveInside.type = PrimeFrame::EventType::PointerMove;
  moveInside.pointerId = 1;
  moveInside.x = xInside;
  moveInside.y = yInside;
  router.dispatch(moveInside, frame, layout, nullptr);

  PrimeFrame::Node const* sliderNode = frame.getNode(slider.nodeId());
  REQUIRE(sliderNode != nullptr);
  REQUIRE(sliderNode->primitives.size() >= 2);
  PrimeFrame::Primitive const* fillPrim = frame.getPrimitive(sliderNode->primitives[1]);
  REQUIRE(fillPrim != nullptr);
  REQUIRE(fillPrim->rect.overrideStyle.opacity.has_value());
  CHECK(fillPrim->rect.overrideStyle.opacity.value() == doctest::Approx(0.9f));

  PrimeFrame::Event moveOutside;
  moveOutside.type = PrimeFrame::EventType::PointerMove;
  moveOutside.pointerId = 1;
  moveOutside.x = xOutside;
  moveOutside.y = yInside;
  router.dispatch(moveOutside, frame, layout, nullptr);

  fillPrim = frame.getPrimitive(sliderNode->primitives[1]);
  REQUIRE(fillPrim != nullptr);
  REQUIRE(fillPrim->rect.overrideStyle.opacity.has_value());
  CHECK(fillPrim->rect.overrideStyle.opacity.value() == doctest::Approx(0.5f));
}

TEST_CASE("PrimeStage slider clamps to max when dragged outside right") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 220.0f, 40.0f);

  float value = 0.0f;
  PrimeStage::SliderSpec spec;
  spec.size.preferredWidth = 180.0f;
  spec.size.preferredHeight = 24.0f;
  spec.trackStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  spec.fillStyle = Studio::rectToken(Studio::RectRole::Accent);
  spec.thumbStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  spec.callbacks.onValueChanged = [&](float next) { value = next; };
  PrimeStage::UiNode slider = root.createSlider(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 220.0f;
  options.rootHeight = 40.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* out = layout.get(slider.nodeId());
  REQUIRE(out != nullptr);
  float y = out->absY + out->absH * 0.5f;
  float xInside = out->absX + out->absW * 0.5f;
  float xOutside = out->absX + out->absW + 80.0f;

  PrimeFrame::EventRouter router;
  PrimeFrame::Event down;
  down.type = PrimeFrame::EventType::PointerDown;
  down.pointerId = 1;
  down.x = xInside;
  down.y = y;
  router.dispatch(down, frame, layout, nullptr);

  PrimeFrame::Event move;
  move.type = PrimeFrame::EventType::PointerMove;
  move.pointerId = 1;
  move.x = xOutside;
  move.y = y;
  router.dispatch(move, frame, layout, nullptr);

  CHECK(value == doctest::Approx(1.0f));
  PrimeFrame::Node const* sliderNode = frame.getNode(slider.nodeId());
  REQUIRE(sliderNode != nullptr);
  REQUIRE(sliderNode->primitives.size() >= 2);
  PrimeFrame::Primitive const* fillPrim = frame.getPrimitive(sliderNode->primitives[1]);
  REQUIRE(fillPrim != nullptr);
  CHECK(fillPrim->width > 0.0f);
}

TEST_CASE("PrimeStage button drag out and back in clicks on release") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 80.0f);

  bool clicked = false;
  PrimeStage::ButtonSpec spec;
  spec.label = "Action";
  spec.size.preferredWidth = 120.0f;
  spec.size.preferredHeight = 32.0f;
  spec.backgroundStyle = Studio::rectToken(Studio::RectRole::ButtonBase);
  spec.hoverStyle = Studio::rectToken(Studio::RectRole::ButtonHover);
  spec.pressedStyle = Studio::rectToken(Studio::RectRole::ButtonPressed);
  spec.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  spec.callbacks.onClick = [&]() { clicked = true; };
  PrimeStage::UiNode button = root.createButton(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 240.0f;
  options.rootHeight = 80.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* out = layout.get(button.nodeId());
  REQUIRE(out != nullptr);
  float xInside = out->absX + out->absW * 0.5f;
  float yInside = out->absY + out->absH * 0.5f;
  float xOutside = out->absX - 40.0f;

  PrimeFrame::EventRouter router;
  PrimeFrame::Event down;
  down.type = PrimeFrame::EventType::PointerDown;
  down.pointerId = 1;
  down.x = xInside;
  down.y = yInside;
  router.dispatch(down, frame, layout, nullptr);

  PrimeFrame::Event moveOut;
  moveOut.type = PrimeFrame::EventType::PointerMove;
  moveOut.pointerId = 1;
  moveOut.x = xOutside;
  moveOut.y = yInside;
  router.dispatch(moveOut, frame, layout, nullptr);

  PrimeFrame::Event moveIn;
  moveIn.type = PrimeFrame::EventType::PointerMove;
  moveIn.pointerId = 1;
  moveIn.x = xInside;
  moveIn.y = yInside;
  router.dispatch(moveIn, frame, layout, nullptr);

  PrimeFrame::Event up;
  up.type = PrimeFrame::EventType::PointerUp;
  up.pointerId = 1;
  up.x = xInside;
  up.y = yInside;
  router.dispatch(up, frame, layout, nullptr);

  CHECK(clicked);
  PrimeFrame::Node const* buttonNode = frame.getNode(button.nodeId());
  REQUIRE(buttonNode != nullptr);
  REQUIRE(!buttonNode->primitives.empty());
  PrimeFrame::Primitive const* bg = frame.getPrimitive(buttonNode->primitives.front());
  REQUIRE(bg != nullptr);
  CHECK(bg->rect.token == Studio::rectToken(Studio::RectRole::ButtonHover));
}

TEST_CASE("PrimeStage slider zero fill hides primitive") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  PrimeStage::SliderSpec spec;
  spec.size.preferredWidth = 160.0f;
  spec.size.preferredHeight = 24.0f;
  spec.value = 0.0f;
  spec.trackStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  spec.fillStyle = Studio::rectToken(Studio::RectRole::Accent);
  spec.thumbStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  spec.fillStyleOverride.opacity = 0.8f;
  spec.callbacks.onValueChanged = [](float) {};
  PrimeStage::UiNode slider = root.createSlider(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 200.0f;
  options.rootHeight = 40.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::Node const* sliderNode = frame.getNode(slider.nodeId());
  REQUIRE(sliderNode != nullptr);
  REQUIRE(sliderNode->primitives.size() >= 2);
  PrimeFrame::Primitive const* fillPrim = frame.getPrimitive(sliderNode->primitives[1]);
  REQUIRE(fillPrim != nullptr);
  CHECK(fillPrim->width == doctest::Approx(0.0f));
  REQUIRE(fillPrim->rect.overrideStyle.opacity.has_value());
  CHECK(fillPrim->rect.overrideStyle.opacity.value() == doctest::Approx(0.0f));
}

TEST_CASE("PrimeStage slider track hover updates opacity") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  PrimeStage::SliderSpec spec;
  spec.size.preferredWidth = 160.0f;
  spec.size.preferredHeight = 24.0f;
  spec.value = 0.5f;
  spec.trackStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  spec.fillStyle = Studio::rectToken(Studio::RectRole::Accent);
  spec.thumbStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  spec.trackStyleOverride.opacity = 0.5f;
  spec.trackHoverOpacity = 0.9f;
  spec.callbacks.onValueChanged = [](float) {};
  PrimeStage::UiNode slider = root.createSlider(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 200.0f;
  options.rootHeight = 40.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* out = layout.get(slider.nodeId());
  REQUIRE(out != nullptr);
  float x = out->absX + out->absW * 0.5f;
  float y = out->absY + out->absH * 0.5f;

  PrimeFrame::EventRouter router;
  PrimeFrame::Event move;
  move.type = PrimeFrame::EventType::PointerMove;
  move.pointerId = 1;
  move.x = x;
  move.y = y;
  router.dispatch(move, frame, layout, nullptr);

  PrimeFrame::Node const* sliderNode = frame.getNode(slider.nodeId());
  REQUIRE(sliderNode != nullptr);
  REQUIRE(!sliderNode->primitives.empty());
  PrimeFrame::Primitive const* trackPrim = frame.getPrimitive(sliderNode->primitives.front());
  REQUIRE(trackPrim != nullptr);
  REQUIRE(trackPrim->rect.overrideStyle.opacity.has_value());
  CHECK(trackPrim->rect.overrideStyle.opacity.value() == doctest::Approx(0.9f));
}

TEST_CASE("PrimeStage button cancel clears pressed state") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 80.0f);

  bool pressed = false;
  PrimeStage::ButtonSpec spec;
  spec.label = "Action";
  spec.size.preferredWidth = 120.0f;
  spec.size.preferredHeight = 32.0f;
  spec.backgroundStyle = Studio::rectToken(Studio::RectRole::ButtonBase);
  spec.hoverStyle = Studio::rectToken(Studio::RectRole::ButtonHover);
  spec.pressedStyle = Studio::rectToken(Studio::RectRole::ButtonPressed);
  spec.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  spec.callbacks.onPressedChanged = [&](bool value) { pressed = value; };
  PrimeStage::UiNode button = root.createButton(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 200.0f;
  options.rootHeight = 80.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* out = layout.get(button.nodeId());
  REQUIRE(out != nullptr);
  float x = out->absX + out->absW * 0.5f;
  float y = out->absY + out->absH * 0.5f;

  PrimeFrame::EventRouter router;
  PrimeFrame::Event down;
  down.type = PrimeFrame::EventType::PointerDown;
  down.pointerId = 1;
  down.x = x;
  down.y = y;
  router.dispatch(down, frame, layout, nullptr);

  CHECK(pressed);

  PrimeFrame::Event cancel;
  cancel.type = PrimeFrame::EventType::PointerCancel;
  cancel.pointerId = 1;
  cancel.x = x;
  cancel.y = y;
  router.dispatch(cancel, frame, layout, nullptr);

  CHECK(!pressed);
  PrimeFrame::Node const* buttonNode = frame.getNode(button.nodeId());
  REQUIRE(buttonNode != nullptr);
  REQUIRE(!buttonNode->primitives.empty());
  PrimeFrame::Primitive const* bg = frame.getPrimitive(buttonNode->primitives.front());
  REQUIRE(bg != nullptr);
  CHECK(bg->rect.token == Studio::rectToken(Studio::RectRole::ButtonBase));
}

TEST_CASE("PrimeStage button hover without callbacks still updates style") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 80.0f);

  PrimeStage::ButtonSpec spec;
  spec.label = "Action";
  spec.size.preferredWidth = 120.0f;
  spec.size.preferredHeight = 32.0f;
  spec.backgroundStyle = Studio::rectToken(Studio::RectRole::ButtonBase);
  spec.hoverStyle = Studio::rectToken(Studio::RectRole::ButtonHover);
  spec.pressedStyle = Studio::rectToken(Studio::RectRole::ButtonPressed);
  spec.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  PrimeStage::UiNode button = root.createButton(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 200.0f;
  options.rootHeight = 80.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* out = layout.get(button.nodeId());
  REQUIRE(out != nullptr);
  float x = out->absX + out->absW * 0.5f;
  float y = out->absY + out->absH * 0.5f;

  PrimeFrame::EventRouter router;
  PrimeFrame::Event move;
  move.type = PrimeFrame::EventType::PointerMove;
  move.pointerId = 1;
  move.x = x;
  move.y = y;
  router.dispatch(move, frame, layout, nullptr);

  PrimeFrame::Node const* buttonNode = frame.getNode(button.nodeId());
  REQUIRE(buttonNode != nullptr);
  REQUIRE(!buttonNode->primitives.empty());
  PrimeFrame::Primitive const* bg = frame.getPrimitive(buttonNode->primitives.front());
  REQUIRE(bg != nullptr);
  CHECK(bg->rect.token == Studio::rectToken(Studio::RectRole::ButtonHover));
}

TEST_CASE("PrimeStage slider cancel leaves value unchanged") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 220.0f, 40.0f);

  float value = 0.3f;
  PrimeStage::SliderSpec spec;
  spec.size.preferredWidth = 180.0f;
  spec.size.preferredHeight = 24.0f;
  spec.value = value;
  spec.trackStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  spec.fillStyle = Studio::rectToken(Studio::RectRole::Accent);
  spec.thumbStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  spec.callbacks.onValueChanged = [&](float next) { value = next; };
  PrimeStage::UiNode slider = root.createSlider(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 220.0f;
  options.rootHeight = 40.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* out = layout.get(slider.nodeId());
  REQUIRE(out != nullptr);
  float y = out->absY + out->absH * 0.5f;
  float xInside = out->absX + out->absW * 0.5f;
  float xOutside = out->absX + out->absW + 60.0f;

  PrimeFrame::EventRouter router;
  PrimeFrame::Event down;
  down.type = PrimeFrame::EventType::PointerDown;
  down.pointerId = 1;
  down.x = xInside;
  down.y = y;
  router.dispatch(down, frame, layout, nullptr);

  PrimeFrame::Event move;
  move.type = PrimeFrame::EventType::PointerMove;
  move.pointerId = 1;
  move.x = xOutside;
  move.y = y;
  router.dispatch(move, frame, layout, nullptr);

  PrimeFrame::Event cancel;
  cancel.type = PrimeFrame::EventType::PointerCancel;
  cancel.pointerId = 1;
  cancel.x = xOutside;
  cancel.y = y;
  router.dispatch(cancel, frame, layout, nullptr);

  CHECK(value == doctest::Approx(1.0f));
}

TEST_CASE("PrimeStage slider track hover leave restores opacity") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  PrimeStage::SliderSpec spec;
  spec.size.preferredWidth = 160.0f;
  spec.size.preferredHeight = 24.0f;
  spec.value = 0.5f;
  spec.trackStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  spec.fillStyle = Studio::rectToken(Studio::RectRole::Accent);
  spec.thumbStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  spec.trackStyleOverride.opacity = 0.4f;
  spec.trackHoverOpacity = 0.8f;
  spec.callbacks.onValueChanged = [](float) {};
  PrimeStage::UiNode slider = root.createSlider(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 200.0f;
  options.rootHeight = 40.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* out = layout.get(slider.nodeId());
  REQUIRE(out != nullptr);
  float xInside = out->absX + out->absW * 0.5f;
  float yInside = out->absY + out->absH * 0.5f;
  float xOutside = out->absX - 60.0f;

  PrimeFrame::EventRouter router;
  PrimeFrame::Event moveInside;
  moveInside.type = PrimeFrame::EventType::PointerMove;
  moveInside.pointerId = 1;
  moveInside.x = xInside;
  moveInside.y = yInside;
  router.dispatch(moveInside, frame, layout, nullptr);

  PrimeFrame::Node const* sliderNode = frame.getNode(slider.nodeId());
  REQUIRE(sliderNode != nullptr);
  REQUIRE(!sliderNode->primitives.empty());
  PrimeFrame::Primitive const* trackPrim = frame.getPrimitive(sliderNode->primitives.front());
  REQUIRE(trackPrim != nullptr);
  REQUIRE(trackPrim->rect.overrideStyle.opacity.has_value());
  CHECK(trackPrim->rect.overrideStyle.opacity.value() == doctest::Approx(0.8f));

  PrimeFrame::Event moveOutside;
  moveOutside.type = PrimeFrame::EventType::PointerMove;
  moveOutside.pointerId = 1;
  moveOutside.x = xOutside;
  moveOutside.y = yInside;
  router.dispatch(moveOutside, frame, layout, nullptr);

  trackPrim = frame.getPrimitive(sliderNode->primitives.front());
  REQUIRE(trackPrim != nullptr);
  REQUIRE(trackPrim->rect.overrideStyle.opacity.has_value());
  CHECK(trackPrim->rect.overrideStyle.opacity.value() == doctest::Approx(0.4f));
}

TEST_CASE("PrimeStage slider track pressed overrides hover") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  PrimeStage::SliderSpec spec;
  spec.size.preferredWidth = 160.0f;
  spec.size.preferredHeight = 24.0f;
  spec.value = 0.5f;
  spec.trackStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  spec.fillStyle = Studio::rectToken(Studio::RectRole::Accent);
  spec.thumbStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  spec.trackStyleOverride.opacity = 0.5f;
  spec.trackHoverOpacity = 0.9f;
  spec.trackPressedOpacity = 0.3f;
  spec.callbacks.onValueChanged = [](float) {};
  PrimeStage::UiNode slider = root.createSlider(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 200.0f;
  options.rootHeight = 40.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* out = layout.get(slider.nodeId());
  REQUIRE(out != nullptr);
  float x = out->absX + out->absW * 0.5f;
  float y = out->absY + out->absH * 0.5f;

  PrimeFrame::EventRouter router;
  PrimeFrame::Event move;
  move.type = PrimeFrame::EventType::PointerMove;
  move.pointerId = 1;
  move.x = x;
  move.y = y;
  router.dispatch(move, frame, layout, nullptr);

  PrimeFrame::Node const* sliderNode = frame.getNode(slider.nodeId());
  REQUIRE(sliderNode != nullptr);
  REQUIRE(!sliderNode->primitives.empty());
  PrimeFrame::Primitive const* trackPrim = frame.getPrimitive(sliderNode->primitives.front());
  REQUIRE(trackPrim != nullptr);
  REQUIRE(trackPrim->rect.overrideStyle.opacity.has_value());
  CHECK(trackPrim->rect.overrideStyle.opacity.value() == doctest::Approx(0.9f));

  PrimeFrame::Event down;
  down.type = PrimeFrame::EventType::PointerDown;
  down.pointerId = 1;
  down.x = x;
  down.y = y;
  router.dispatch(down, frame, layout, nullptr);

  trackPrim = frame.getPrimitive(sliderNode->primitives.front());
  REQUIRE(trackPrim != nullptr);
  REQUIRE(trackPrim->rect.overrideStyle.opacity.has_value());
  CHECK(trackPrim->rect.overrideStyle.opacity.value() == doctest::Approx(0.3f));
}

TEST_CASE("PrimeStage button press without hover callbacks updates style and clicks") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 80.0f);

  bool clicked = false;
  PrimeStage::ButtonSpec spec;
  spec.label = "Action";
  spec.size.preferredWidth = 120.0f;
  spec.size.preferredHeight = 32.0f;
  spec.backgroundStyle = Studio::rectToken(Studio::RectRole::ButtonBase);
  spec.hoverStyle = Studio::rectToken(Studio::RectRole::ButtonHover);
  spec.pressedStyle = Studio::rectToken(Studio::RectRole::ButtonPressed);
  spec.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  spec.callbacks.onClick = [&]() { clicked = true; };
  PrimeStage::UiNode button = root.createButton(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 240.0f;
  options.rootHeight = 80.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* out = layout.get(button.nodeId());
  REQUIRE(out != nullptr);
  float x = out->absX + out->absW * 0.5f;
  float y = out->absY + out->absH * 0.5f;

  PrimeFrame::EventRouter router;
  PrimeFrame::Event down;
  down.type = PrimeFrame::EventType::PointerDown;
  down.pointerId = 1;
  down.x = x;
  down.y = y;
  router.dispatch(down, frame, layout, nullptr);

  PrimeFrame::Node const* buttonNode = frame.getNode(button.nodeId());
  REQUIRE(buttonNode != nullptr);
  REQUIRE(!buttonNode->primitives.empty());
  PrimeFrame::Primitive const* bg = frame.getPrimitive(buttonNode->primitives.front());
  REQUIRE(bg != nullptr);
  CHECK(bg->rect.token == Studio::rectToken(Studio::RectRole::ButtonPressed));

  PrimeFrame::Event up;
  up.type = PrimeFrame::EventType::PointerUp;
  up.pointerId = 1;
  up.x = x;
  up.y = y;
  router.dispatch(up, frame, layout, nullptr);

  CHECK(clicked);
}

TEST_CASE("PrimeStage button release outside does not click") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 80.0f);

  bool clicked = false;
  PrimeStage::ButtonSpec spec;
  spec.label = "Action";
  spec.size.preferredWidth = 120.0f;
  spec.size.preferredHeight = 32.0f;
  spec.backgroundStyle = Studio::rectToken(Studio::RectRole::ButtonBase);
  spec.hoverStyle = Studio::rectToken(Studio::RectRole::ButtonHover);
  spec.pressedStyle = Studio::rectToken(Studio::RectRole::ButtonPressed);
  spec.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  spec.callbacks.onClick = [&]() { clicked = true; };
  PrimeStage::UiNode button = root.createButton(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 240.0f;
  options.rootHeight = 80.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* out = layout.get(button.nodeId());
  REQUIRE(out != nullptr);
  float xInside = out->absX + out->absW * 0.5f;
  float yInside = out->absY + out->absH * 0.5f;
  float xOutside = out->absX - 40.0f;

  PrimeFrame::EventRouter router;
  PrimeFrame::Event down;
  down.type = PrimeFrame::EventType::PointerDown;
  down.pointerId = 1;
  down.x = xInside;
  down.y = yInside;
  router.dispatch(down, frame, layout, nullptr);

  PrimeFrame::Event move;
  move.type = PrimeFrame::EventType::PointerMove;
  move.pointerId = 1;
  move.x = xOutside;
  move.y = yInside;
  router.dispatch(move, frame, layout, nullptr);

  PrimeFrame::Event up;
  up.type = PrimeFrame::EventType::PointerUp;
  up.pointerId = 1;
  up.x = xOutside;
  up.y = yInside;
  router.dispatch(up, frame, layout, nullptr);

  CHECK(!clicked);
}

TEST_CASE("PrimeStage slider hover does not update value without press") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  float value = 0.25f;
  PrimeStage::SliderSpec spec;
  spec.size.preferredWidth = 160.0f;
  spec.size.preferredHeight = 24.0f;
  spec.value = value;
  spec.trackStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  spec.fillStyle = Studio::rectToken(Studio::RectRole::Accent);
  spec.thumbStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  spec.fillStyleOverride.opacity = 0.5f;
  spec.fillHoverOpacity = 0.9f;
  spec.callbacks.onValueChanged = [&](float next) { value = next; };
  PrimeStage::UiNode slider = root.createSlider(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 200.0f;
  options.rootHeight = 40.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* out = layout.get(slider.nodeId());
  REQUIRE(out != nullptr);
  float x = out->absX + out->absW * 0.75f;
  float y = out->absY + out->absH * 0.5f;

  PrimeFrame::EventRouter router;
  PrimeFrame::Event move;
  move.type = PrimeFrame::EventType::PointerMove;
  move.pointerId = 1;
  move.x = x;
  move.y = y;
  router.dispatch(move, frame, layout, nullptr);

  CHECK(value == doctest::Approx(0.25f));
}

TEST_CASE("PrimeStage slider fill hover ignored when unset") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  PrimeStage::SliderSpec spec;
  spec.size.preferredWidth = 160.0f;
  spec.size.preferredHeight = 24.0f;
  spec.value = 0.5f;
  spec.trackStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  spec.fillStyle = Studio::rectToken(Studio::RectRole::Accent);
  spec.thumbStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  spec.fillStyleOverride.opacity = 0.55f;
  spec.callbacks.onValueChanged = [](float) {};
  PrimeStage::UiNode slider = root.createSlider(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 200.0f;
  options.rootHeight = 40.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* out = layout.get(slider.nodeId());
  REQUIRE(out != nullptr);
  float x = out->absX + out->absW * 0.5f;
  float y = out->absY + out->absH * 0.5f;

  PrimeFrame::EventRouter router;
  PrimeFrame::Event move;
  move.type = PrimeFrame::EventType::PointerMove;
  move.pointerId = 1;
  move.x = x;
  move.y = y;
  router.dispatch(move, frame, layout, nullptr);

  PrimeFrame::Node const* sliderNode = frame.getNode(slider.nodeId());
  REQUIRE(sliderNode != nullptr);
  REQUIRE(sliderNode->primitives.size() >= 2);
  PrimeFrame::Primitive const* fillPrim = frame.getPrimitive(sliderNode->primitives[1]);
  REQUIRE(fillPrim != nullptr);
  REQUIRE(fillPrim->rect.overrideStyle.opacity.has_value());
  CHECK(fillPrim->rect.overrideStyle.opacity.value() == doctest::Approx(0.55f));
}

TEST_CASE("PrimeStage button without interactions does not attach callbacks") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 80.0f);

  PrimeStage::ButtonSpec spec;
  spec.label = "Static";
  spec.size.preferredWidth = 120.0f;
  spec.size.preferredHeight = 32.0f;
  spec.backgroundStyle = Studio::rectToken(Studio::RectRole::ButtonBase);
  spec.hoverStyle = spec.backgroundStyle;
  spec.pressedStyle = spec.backgroundStyle;
  spec.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  spec.baseOpacity = 1.0f;
  spec.hoverOpacity = 1.0f;
  spec.pressedOpacity = 1.0f;
  PrimeStage::UiNode button = root.createButton(spec);

  PrimeFrame::Node const* buttonNode = frame.getNode(button.nodeId());
  REQUIRE(buttonNode != nullptr);
  CHECK(buttonNode->callbacks == PrimeFrame::InvalidCallbackId);
}

TEST_CASE("PrimeStage button opacity overrides change on hover/press") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 220.0f, 80.0f);

  PrimeStage::ButtonSpec spec;
  spec.label = "Fade";
  spec.size.preferredWidth = 120.0f;
  spec.size.preferredHeight = 32.0f;
  spec.backgroundStyle = Studio::rectToken(Studio::RectRole::ButtonBase);
  spec.hoverStyle = spec.backgroundStyle;
  spec.pressedStyle = spec.backgroundStyle;
  spec.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  spec.baseOpacity = 0.4f;
  spec.hoverOpacity = 0.8f;
  spec.pressedOpacity = 0.2f;
  PrimeStage::UiNode button = root.createButton(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 220.0f;
  options.rootHeight = 80.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::Node const* buttonNode = frame.getNode(button.nodeId());
  REQUIRE(buttonNode != nullptr);
  REQUIRE(!buttonNode->primitives.empty());
  PrimeFrame::Primitive const* bg = frame.getPrimitive(buttonNode->primitives.front());
  REQUIRE(bg != nullptr);
  REQUIRE(bg->rect.overrideStyle.opacity.has_value());
  CHECK(bg->rect.overrideStyle.opacity.value() == doctest::Approx(0.4f));

  PrimeFrame::LayoutOut const* out = layout.get(button.nodeId());
  REQUIRE(out != nullptr);
  float x = out->absX + out->absW * 0.5f;
  float y = out->absY + out->absH * 0.5f;

  PrimeFrame::EventRouter router;
  PrimeFrame::Event move;
  move.type = PrimeFrame::EventType::PointerMove;
  move.pointerId = 1;
  move.x = x;
  move.y = y;
  router.dispatch(move, frame, layout, nullptr);

  bg = frame.getPrimitive(buttonNode->primitives.front());
  REQUIRE(bg != nullptr);
  REQUIRE(bg->rect.overrideStyle.opacity.has_value());
  CHECK(bg->rect.overrideStyle.opacity.value() == doctest::Approx(0.8f));

  PrimeFrame::Event down;
  down.type = PrimeFrame::EventType::PointerDown;
  down.pointerId = 1;
  down.x = x;
  down.y = y;
  router.dispatch(down, frame, layout, nullptr);

  bg = frame.getPrimitive(buttonNode->primitives.front());
  REQUIRE(bg != nullptr);
  REQUIRE(bg->rect.overrideStyle.opacity.has_value());
  CHECK(bg->rect.overrideStyle.opacity.value() == doctest::Approx(0.2f));
}

TEST_CASE("PrimeStage slider zero thumb hides primitive") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  PrimeStage::SliderSpec spec;
  spec.size.preferredWidth = 160.0f;
  spec.size.preferredHeight = 24.0f;
  spec.value = 0.4f;
  spec.thumbSize = 0.0f;
  spec.trackStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  spec.fillStyle = Studio::rectToken(Studio::RectRole::Accent);
  spec.thumbStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  spec.callbacks.onValueChanged = [](float) {};
  PrimeStage::UiNode slider = root.createSlider(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 200.0f;
  options.rootHeight = 40.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::Node const* sliderNode = frame.getNode(slider.nodeId());
  REQUIRE(sliderNode != nullptr);
  REQUIRE(sliderNode->primitives.size() >= 3);
  PrimeFrame::Primitive const* thumbPrim = frame.getPrimitive(sliderNode->primitives[2]);
  REQUIRE(thumbPrim != nullptr);
  CHECK(thumbPrim->width == doctest::Approx(0.0f));
  REQUIRE(thumbPrim->rect.overrideStyle.opacity.has_value());
  CHECK(thumbPrim->rect.overrideStyle.opacity.value() == doctest::Approx(0.0f));
}

TEST_CASE("PrimeStage slider vertical clamps top and bottom") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 80.0f, 220.0f);

  float value = 0.0f;
  PrimeStage::SliderSpec spec;
  spec.vertical = true;
  spec.size.preferredWidth = 24.0f;
  spec.size.preferredHeight = 180.0f;
  spec.trackStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  spec.fillStyle = Studio::rectToken(Studio::RectRole::Accent);
  spec.thumbStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  spec.callbacks.onValueChanged = [&](float next) { value = next; };
  PrimeStage::UiNode slider = root.createSlider(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 80.0f;
  options.rootHeight = 220.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* out = layout.get(slider.nodeId());
  REQUIRE(out != nullptr);
  float x = out->absX + out->absW * 0.5f;
  float yTop = out->absY + 2.0f;
  float yBottom = out->absY + out->absH - 2.0f;

  PrimeFrame::EventRouter router;
  PrimeFrame::Event down;
  down.type = PrimeFrame::EventType::PointerDown;
  down.pointerId = 1;
  down.x = x;
  down.y = yTop;
  router.dispatch(down, frame, layout, nullptr);
  CHECK(value == doctest::Approx(1.0f));

  PrimeFrame::Event move;
  move.type = PrimeFrame::EventType::PointerMove;
  move.pointerId = 1;
  move.x = x;
  move.y = yBottom;
  router.dispatch(move, frame, layout, nullptr);
  CHECK(value == doctest::Approx(0.0f));
}

TEST_CASE("PrimeStage slider large thumb clamps value range") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  float value = 0.0f;
  PrimeStage::SliderSpec spec;
  spec.size.preferredWidth = 160.0f;
  spec.size.preferredHeight = 24.0f;
  spec.thumbSize = 120.0f;
  spec.trackStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  spec.fillStyle = Studio::rectToken(Studio::RectRole::Accent);
  spec.thumbStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  spec.callbacks.onValueChanged = [&](float next) { value = next; };
  PrimeStage::UiNode slider = root.createSlider(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 200.0f;
  options.rootHeight = 40.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* out = layout.get(slider.nodeId());
  REQUIRE(out != nullptr);
  float y = out->absY + out->absH * 0.5f;
  float xLeft = out->absX + 2.0f;
  float xRight = out->absX + out->absW - 2.0f;

  PrimeFrame::EventRouter router;
  PrimeFrame::Event down;
  down.type = PrimeFrame::EventType::PointerDown;
  down.pointerId = 1;
  down.x = xLeft;
  down.y = y;
  router.dispatch(down, frame, layout, nullptr);
  CHECK(value == doctest::Approx(0.0f));

  PrimeFrame::Event move;
  move.type = PrimeFrame::EventType::PointerMove;
  move.pointerId = 1;
  move.x = xRight;
  move.y = y;
  router.dispatch(move, frame, layout, nullptr);
  CHECK(value == doctest::Approx(1.0f));
}

TEST_CASE("PrimeStage button hover/press styles can match base") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 80.0f);

  bool clicked = false;
  PrimeStage::ButtonSpec spec;
  spec.label = "Same";
  spec.size.preferredWidth = 120.0f;
  spec.size.preferredHeight = 32.0f;
  spec.backgroundStyle = Studio::rectToken(Studio::RectRole::ButtonBase);
  spec.hoverStyle = spec.backgroundStyle;
  spec.pressedStyle = spec.backgroundStyle;
  spec.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  spec.callbacks.onClick = [&]() { clicked = true; };
  PrimeStage::UiNode button = root.createButton(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 200.0f;
  options.rootHeight = 80.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* out = layout.get(button.nodeId());
  REQUIRE(out != nullptr);
  float x = out->absX + out->absW * 0.5f;
  float y = out->absY + out->absH * 0.5f;

  PrimeFrame::EventRouter router;
  PrimeFrame::Event down;
  down.type = PrimeFrame::EventType::PointerDown;
  down.pointerId = 1;
  down.x = x;
  down.y = y;
  router.dispatch(down, frame, layout, nullptr);

  PrimeFrame::Event up;
  up.type = PrimeFrame::EventType::PointerUp;
  up.pointerId = 1;
  up.x = x;
  up.y = y;
  router.dispatch(up, frame, layout, nullptr);

  CHECK(clicked);
}

TEST_CASE("PrimeStage button opacity pressed overrides hover when styles match") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 220.0f, 80.0f);

  PrimeStage::ButtonSpec spec;
  spec.label = "Opacity";
  spec.size.preferredWidth = 120.0f;
  spec.size.preferredHeight = 32.0f;
  spec.backgroundStyle = Studio::rectToken(Studio::RectRole::ButtonBase);
  spec.hoverStyle = spec.backgroundStyle;
  spec.pressedStyle = spec.backgroundStyle;
  spec.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  spec.baseOpacity = 0.5f;
  spec.hoverOpacity = 0.9f;
  spec.pressedOpacity = 0.2f;
  PrimeStage::UiNode button = root.createButton(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 220.0f;
  options.rootHeight = 80.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* out = layout.get(button.nodeId());
  REQUIRE(out != nullptr);
  float x = out->absX + out->absW * 0.5f;
  float y = out->absY + out->absH * 0.5f;

  PrimeFrame::EventRouter router;
  PrimeFrame::Event move;
  move.type = PrimeFrame::EventType::PointerMove;
  move.pointerId = 1;
  move.x = x;
  move.y = y;
  router.dispatch(move, frame, layout, nullptr);

  PrimeFrame::Node const* buttonNode = frame.getNode(button.nodeId());
  REQUIRE(buttonNode != nullptr);
  REQUIRE(!buttonNode->primitives.empty());
  PrimeFrame::Primitive const* bg = frame.getPrimitive(buttonNode->primitives.front());
  REQUIRE(bg != nullptr);
  REQUIRE(bg->rect.overrideStyle.opacity.has_value());
  CHECK(bg->rect.overrideStyle.opacity.value() == doctest::Approx(0.9f));

  PrimeFrame::Event down;
  down.type = PrimeFrame::EventType::PointerDown;
  down.pointerId = 1;
  down.x = x;
  down.y = y;
  router.dispatch(down, frame, layout, nullptr);

  bg = frame.getPrimitive(buttonNode->primitives.front());
  REQUIRE(bg != nullptr);
  REQUIRE(bg->rect.overrideStyle.opacity.has_value());
  CHECK(bg->rect.overrideStyle.opacity.value() == doctest::Approx(0.2f));
}

TEST_CASE("PrimeStage slider vertical hover and pressed update fill opacity") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 80.0f, 220.0f);

  PrimeStage::SliderSpec spec;
  spec.vertical = true;
  spec.size.preferredWidth = 24.0f;
  spec.size.preferredHeight = 180.0f;
  spec.value = 0.5f;
  spec.trackStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  spec.fillStyle = Studio::rectToken(Studio::RectRole::Accent);
  spec.thumbStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  spec.fillStyleOverride.opacity = 0.4f;
  spec.fillHoverOpacity = 0.8f;
  spec.fillPressedOpacity = 0.2f;
  spec.callbacks.onValueChanged = [](float) {};
  PrimeStage::UiNode slider = root.createSlider(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 80.0f;
  options.rootHeight = 220.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* out = layout.get(slider.nodeId());
  REQUIRE(out != nullptr);
  float x = out->absX + out->absW * 0.5f;
  float y = out->absY + out->absH * 0.5f;

  PrimeFrame::EventRouter router;
  PrimeFrame::Event move;
  move.type = PrimeFrame::EventType::PointerMove;
  move.pointerId = 1;
  move.x = x;
  move.y = y;
  router.dispatch(move, frame, layout, nullptr);

  PrimeFrame::Node const* sliderNode = frame.getNode(slider.nodeId());
  REQUIRE(sliderNode != nullptr);
  REQUIRE(sliderNode->primitives.size() >= 2);
  PrimeFrame::Primitive const* fillPrim = frame.getPrimitive(sliderNode->primitives[1]);
  REQUIRE(fillPrim != nullptr);
  REQUIRE(fillPrim->rect.overrideStyle.opacity.has_value());
  CHECK(fillPrim->rect.overrideStyle.opacity.value() == doctest::Approx(0.8f));

  PrimeFrame::Event down;
  down.type = PrimeFrame::EventType::PointerDown;
  down.pointerId = 1;
  down.x = x;
  down.y = y;
  router.dispatch(down, frame, layout, nullptr);

  fillPrim = frame.getPrimitive(sliderNode->primitives[1]);
  REQUIRE(fillPrim != nullptr);
  REQUIRE(fillPrim->rect.overrideStyle.opacity.has_value());
  CHECK(fillPrim->rect.overrideStyle.opacity.value() == doctest::Approx(0.2f));
}

TEST_CASE("PrimeStage slider track thickness clamps to height") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  PrimeStage::SliderSpec spec;
  spec.size.preferredWidth = 160.0f;
  spec.size.preferredHeight = 24.0f;
  spec.value = 0.5f;
  spec.trackThickness = 100.0f;
  spec.trackStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  spec.fillStyle = Studio::rectToken(Studio::RectRole::Accent);
  spec.thumbStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  spec.callbacks.onValueChanged = [](float) {};
  PrimeStage::UiNode slider = root.createSlider(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 200.0f;
  options.rootHeight = 40.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* out = layout.get(slider.nodeId());
  REQUIRE(out != nullptr);
  PrimeFrame::Node const* sliderNode = frame.getNode(slider.nodeId());
  REQUIRE(sliderNode != nullptr);
  REQUIRE(sliderNode->primitives.size() >= 2);
  PrimeFrame::Primitive const* fillPrim = frame.getPrimitive(sliderNode->primitives[1]);
  REQUIRE(fillPrim != nullptr);
  CHECK(fillPrim->height == doctest::Approx(out->absH));
}

TEST_CASE("PrimeStage slider vertical with large thumb clamps range") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 80.0f, 220.0f);

  float value = 0.0f;
  PrimeStage::SliderSpec spec;
  spec.vertical = true;
  spec.size.preferredWidth = 24.0f;
  spec.size.preferredHeight = 180.0f;
  spec.thumbSize = 140.0f;
  spec.trackStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  spec.fillStyle = Studio::rectToken(Studio::RectRole::Accent);
  spec.thumbStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  spec.callbacks.onValueChanged = [&](float next) { value = next; };
  PrimeStage::UiNode slider = root.createSlider(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 80.0f;
  options.rootHeight = 220.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* out = layout.get(slider.nodeId());
  REQUIRE(out != nullptr);
  float x = out->absX + out->absW * 0.5f;
  float yTop = out->absY + 2.0f;
  float yBottom = out->absY + out->absH - 2.0f;

  PrimeFrame::EventRouter router;
  PrimeFrame::Event down;
  down.type = PrimeFrame::EventType::PointerDown;
  down.pointerId = 1;
  down.x = x;
  down.y = yTop;
  router.dispatch(down, frame, layout, nullptr);
  CHECK(value == doctest::Approx(1.0f));

  PrimeFrame::Event move;
  move.type = PrimeFrame::EventType::PointerMove;
  move.pointerId = 1;
  move.x = x;
  move.y = yBottom;
  router.dispatch(move, frame, layout, nullptr);
  CHECK(value == doctest::Approx(0.0f));
}

TEST_CASE("PrimeStage button cancel while hovered returns to base style") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 220.0f, 80.0f);

  PrimeStage::ButtonSpec spec;
  spec.label = "Hover";
  spec.size.preferredWidth = 120.0f;
  spec.size.preferredHeight = 32.0f;
  spec.backgroundStyle = Studio::rectToken(Studio::RectRole::ButtonBase);
  spec.hoverStyle = Studio::rectToken(Studio::RectRole::ButtonHover);
  spec.pressedStyle = Studio::rectToken(Studio::RectRole::ButtonPressed);
  spec.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  PrimeStage::UiNode button = root.createButton(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 220.0f;
  options.rootHeight = 80.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* out = layout.get(button.nodeId());
  REQUIRE(out != nullptr);
  float x = out->absX + out->absW * 0.5f;
  float y = out->absY + out->absH * 0.5f;

  PrimeFrame::EventRouter router;
  PrimeFrame::Event move;
  move.type = PrimeFrame::EventType::PointerMove;
  move.pointerId = 1;
  move.x = x;
  move.y = y;
  router.dispatch(move, frame, layout, nullptr);

  PrimeFrame::Event down;
  down.type = PrimeFrame::EventType::PointerDown;
  down.pointerId = 1;
  down.x = x;
  down.y = y;
  router.dispatch(down, frame, layout, nullptr);

  PrimeFrame::Event cancel;
  cancel.type = PrimeFrame::EventType::PointerCancel;
  cancel.pointerId = 1;
  cancel.x = x;
  cancel.y = y;
  router.dispatch(cancel, frame, layout, nullptr);

  PrimeFrame::Node const* buttonNode = frame.getNode(button.nodeId());
  REQUIRE(buttonNode != nullptr);
  REQUIRE(!buttonNode->primitives.empty());
  PrimeFrame::Primitive const* bg = frame.getPrimitive(buttonNode->primitives.front());
  REQUIRE(bg != nullptr);
  CHECK(bg->rect.token == Studio::rectToken(Studio::RectRole::ButtonBase));
}

TEST_CASE("PrimeStage slider hover updates both track and fill overrides") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  PrimeStage::SliderSpec spec;
  spec.size.preferredWidth = 160.0f;
  spec.size.preferredHeight = 24.0f;
  spec.value = 0.5f;
  spec.trackStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  spec.fillStyle = Studio::rectToken(Studio::RectRole::Accent);
  spec.thumbStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  spec.trackStyleOverride.opacity = 0.4f;
  spec.trackHoverOpacity = 0.7f;
  spec.fillStyleOverride.opacity = 0.5f;
  spec.fillHoverOpacity = 0.9f;
  spec.callbacks.onValueChanged = [](float) {};
  PrimeStage::UiNode slider = root.createSlider(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 200.0f;
  options.rootHeight = 40.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* out = layout.get(slider.nodeId());
  REQUIRE(out != nullptr);
  float x = out->absX + out->absW * 0.5f;
  float y = out->absY + out->absH * 0.5f;

  PrimeFrame::EventRouter router;
  PrimeFrame::Event move;
  move.type = PrimeFrame::EventType::PointerMove;
  move.pointerId = 1;
  move.x = x;
  move.y = y;
  router.dispatch(move, frame, layout, nullptr);

  PrimeFrame::Node const* sliderNode = frame.getNode(slider.nodeId());
  REQUIRE(sliderNode != nullptr);
  REQUIRE(sliderNode->primitives.size() >= 2);
  PrimeFrame::Primitive const* trackPrim = frame.getPrimitive(sliderNode->primitives.front());
  PrimeFrame::Primitive const* fillPrim = frame.getPrimitive(sliderNode->primitives[1]);
  REQUIRE(trackPrim != nullptr);
  REQUIRE(fillPrim != nullptr);
  REQUIRE(trackPrim->rect.overrideStyle.opacity.has_value());
  REQUIRE(fillPrim->rect.overrideStyle.opacity.has_value());
  CHECK(trackPrim->rect.overrideStyle.opacity.value() == doctest::Approx(0.7f));
  CHECK(fillPrim->rect.overrideStyle.opacity.value() == doctest::Approx(0.9f));
}

TEST_CASE("PrimeStage slider vertical track thickness clamps to width") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 80.0f, 220.0f);

  PrimeStage::SliderSpec spec;
  spec.vertical = true;
  spec.size.preferredWidth = 24.0f;
  spec.size.preferredHeight = 180.0f;
  spec.value = 0.5f;
  spec.trackThickness = 100.0f;
  spec.trackStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  spec.fillStyle = Studio::rectToken(Studio::RectRole::Accent);
  spec.thumbStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  spec.callbacks.onValueChanged = [](float) {};
  PrimeStage::UiNode slider = root.createSlider(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 80.0f;
  options.rootHeight = 220.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* out = layout.get(slider.nodeId());
  REQUIRE(out != nullptr);
  PrimeFrame::Node const* sliderNode = frame.getNode(slider.nodeId());
  REQUIRE(sliderNode != nullptr);
  REQUIRE(sliderNode->primitives.size() >= 2);
  PrimeFrame::Primitive const* fillPrim = frame.getPrimitive(sliderNode->primitives[1]);
  REQUIRE(fillPrim != nullptr);
  CHECK(fillPrim->width == doctest::Approx(out->absW));
}

TEST_CASE("PrimeStage slider pressed without hover uses pressed opacity") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  PrimeStage::SliderSpec spec;
  spec.size.preferredWidth = 160.0f;
  spec.size.preferredHeight = 24.0f;
  spec.value = 0.5f;
  spec.trackStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  spec.fillStyle = Studio::rectToken(Studio::RectRole::Accent);
  spec.thumbStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  spec.fillStyleOverride.opacity = 0.6f;
  spec.fillPressedOpacity = 0.3f;
  spec.callbacks.onValueChanged = [](float) {};
  PrimeStage::UiNode slider = root.createSlider(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 200.0f;
  options.rootHeight = 40.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* out = layout.get(slider.nodeId());
  REQUIRE(out != nullptr);
  float x = out->absX + out->absW * 0.5f;
  float y = out->absY + out->absH * 0.5f;

  PrimeFrame::EventRouter router;
  PrimeFrame::Event down;
  down.type = PrimeFrame::EventType::PointerDown;
  down.pointerId = 1;
  down.x = x;
  down.y = y;
  router.dispatch(down, frame, layout, nullptr);

  PrimeFrame::Node const* sliderNode = frame.getNode(slider.nodeId());
  REQUIRE(sliderNode != nullptr);
  REQUIRE(sliderNode->primitives.size() >= 2);
  PrimeFrame::Primitive const* fillPrim = frame.getPrimitive(sliderNode->primitives[1]);
  REQUIRE(fillPrim != nullptr);
  REQUIRE(fillPrim->rect.overrideStyle.opacity.has_value());
  CHECK(fillPrim->rect.overrideStyle.opacity.value() == doctest::Approx(0.3f));
}

TEST_CASE("PrimeStage button press outside does not start interaction") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 80.0f);

  bool clicked = false;
  PrimeStage::ButtonSpec spec;
  spec.label = "Action";
  spec.size.preferredWidth = 120.0f;
  spec.size.preferredHeight = 32.0f;
  spec.backgroundStyle = Studio::rectToken(Studio::RectRole::ButtonBase);
  spec.hoverStyle = Studio::rectToken(Studio::RectRole::ButtonHover);
  spec.pressedStyle = Studio::rectToken(Studio::RectRole::ButtonPressed);
  spec.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  spec.callbacks.onClick = [&]() { clicked = true; };
  PrimeStage::UiNode button = root.createButton(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 240.0f;
  options.rootHeight = 80.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::LayoutOut const* out = layout.get(button.nodeId());
  REQUIRE(out != nullptr);
  float xOutside = out->absX - 50.0f;
  float y = out->absY + out->absH * 0.5f;

  PrimeFrame::EventRouter router;
  PrimeFrame::Event down;
  down.type = PrimeFrame::EventType::PointerDown;
  down.pointerId = 1;
  down.x = xOutside;
  down.y = y;
  router.dispatch(down, frame, layout, nullptr);

  PrimeFrame::Event up;
  up.type = PrimeFrame::EventType::PointerUp;
  up.pointerId = 1;
  up.x = xOutside;
  up.y = y;
  router.dispatch(up, frame, layout, nullptr);

  CHECK(!clicked);
  PrimeFrame::Node const* buttonNode = frame.getNode(button.nodeId());
  REQUIRE(buttonNode != nullptr);
  REQUIRE(!buttonNode->primitives.empty());
  PrimeFrame::Primitive const* bg = frame.getPrimitive(buttonNode->primitives.front());
  REQUIRE(bg != nullptr);
  CHECK(bg->rect.token == Studio::rectToken(Studio::RectRole::ButtonBase));
}

TEST_CASE("PrimeStage tabs creates a node") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 40.0f);

  PrimeStage::TabsSpec spec;
  spec.labels = {"Alpha", "Beta", "Gamma"};
  spec.selectedIndex = 1;
  spec.tabStyle = Studio::rectToken(Studio::RectRole::Panel);
  spec.activeTabStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  spec.textStyle = Studio::textToken(Studio::TextRole::SmallMuted);
  spec.activeTextStyle = Studio::textToken(Studio::TextRole::SmallBright);
  PrimeStage::UiNode tabs = root.createTabs(spec);
  CHECK(frame.getNode(tabs.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage tabs inferred width grows with label length") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 40.0f);

  PrimeStage::TabsSpec shortSpec;
  shortSpec.labels = {"A", "B"};
  shortSpec.tabStyle = Studio::rectToken(Studio::RectRole::Panel);
  shortSpec.activeTabStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  shortSpec.textStyle = Studio::textToken(Studio::TextRole::SmallMuted);
  shortSpec.activeTextStyle = Studio::textToken(Studio::TextRole::SmallBright);
  PrimeStage::UiNode shortTabs = root.createTabs(shortSpec);

  PrimeStage::TabsSpec longSpec = shortSpec;
  longSpec.labels = {"LongLabel", "LongerLabel"};
  PrimeStage::UiNode longTabs = root.createTabs(longSpec);

  PrimeFrame::Node const* shortNode = frame.getNode(shortTabs.nodeId());
  PrimeFrame::Node const* longNode = frame.getNode(longTabs.nodeId());
  REQUIRE(shortNode != nullptr);
  REQUIRE(longNode != nullptr);
  REQUIRE(shortNode->sizeHint.width.preferred.has_value());
  REQUIRE(longNode->sizeHint.width.preferred.has_value());
  CHECK(longNode->sizeHint.width.preferred.value() >
        shortNode->sizeHint.width.preferred.value());
}

TEST_CASE("PrimeStage tabs inferred width grows with gap") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 40.0f);

  PrimeStage::TabsSpec smallGap;
  smallGap.labels = {"A", "B", "C"};
  smallGap.gap = 2.0f;
  smallGap.tabStyle = Studio::rectToken(Studio::RectRole::Panel);
  smallGap.activeTabStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  smallGap.textStyle = Studio::textToken(Studio::TextRole::SmallMuted);
  smallGap.activeTextStyle = Studio::textToken(Studio::TextRole::SmallBright);
  PrimeStage::UiNode smallTabs = root.createTabs(smallGap);

  PrimeStage::TabsSpec largeGap = smallGap;
  largeGap.gap = 10.0f;
  PrimeStage::UiNode largeTabs = root.createTabs(largeGap);

  PrimeFrame::Node const* smallNode = frame.getNode(smallTabs.nodeId());
  PrimeFrame::Node const* largeNode = frame.getNode(largeTabs.nodeId());
  REQUIRE(smallNode != nullptr);
  REQUIRE(largeNode != nullptr);
  REQUIRE(smallNode->sizeHint.width.preferred.has_value());
  REQUIRE(largeNode->sizeHint.width.preferred.has_value());
  CHECK(largeNode->sizeHint.width.preferred.value() -
        smallNode->sizeHint.width.preferred.value() ==
        doctest::Approx(16.0f));
}

TEST_CASE("PrimeStage tabs inferred width grows with padding") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 40.0f);

  PrimeStage::TabsSpec smallPad;
  smallPad.labels = {"A", "B"};
  smallPad.tabPaddingX = 6.0f;
  smallPad.tabStyle = Studio::rectToken(Studio::RectRole::Panel);
  smallPad.activeTabStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  smallPad.textStyle = Studio::textToken(Studio::TextRole::SmallMuted);
  smallPad.activeTextStyle = Studio::textToken(Studio::TextRole::SmallBright);
  PrimeStage::UiNode smallTabs = root.createTabs(smallPad);

  PrimeStage::TabsSpec largePad = smallPad;
  largePad.tabPaddingX = 12.0f;
  PrimeStage::UiNode largeTabs = root.createTabs(largePad);

  PrimeFrame::Node const* smallNode = frame.getNode(smallTabs.nodeId());
  PrimeFrame::Node const* largeNode = frame.getNode(largeTabs.nodeId());
  REQUIRE(smallNode != nullptr);
  REQUIRE(largeNode != nullptr);
  REQUIRE(smallNode->sizeHint.width.preferred.has_value());
  REQUIRE(largeNode->sizeHint.width.preferred.has_value());
  CHECK(largeNode->sizeHint.width.preferred.value() -
        smallNode->sizeHint.width.preferred.value() ==
        doctest::Approx(24.0f));
}

TEST_CASE("PrimeStage tabs inferred height grows with padding") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 40.0f);

  PrimeStage::TabsSpec smallPad;
  smallPad.labels = {"A", "B"};
  smallPad.tabPaddingY = 4.0f;
  smallPad.tabStyle = Studio::rectToken(Studio::RectRole::Panel);
  smallPad.activeTabStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  smallPad.textStyle = Studio::textToken(Studio::TextRole::SmallMuted);
  smallPad.activeTextStyle = Studio::textToken(Studio::TextRole::SmallBright);
  PrimeStage::UiNode smallTabs = root.createTabs(smallPad);

  PrimeStage::TabsSpec largePad = smallPad;
  largePad.tabPaddingY = 10.0f;
  PrimeStage::UiNode largeTabs = root.createTabs(largePad);

  PrimeFrame::Node const* smallNode = frame.getNode(smallTabs.nodeId());
  PrimeFrame::Node const* largeNode = frame.getNode(largeTabs.nodeId());
  REQUIRE(smallNode != nullptr);
  REQUIRE(largeNode != nullptr);
  REQUIRE(smallNode->sizeHint.height.preferred.has_value());
  REQUIRE(largeNode->sizeHint.height.preferred.has_value());
  CHECK(largeNode->sizeHint.height.preferred.value() -
        smallNode->sizeHint.height.preferred.value() ==
        doctest::Approx(12.0f));
}

TEST_CASE("PrimeStage dropdown creates a node") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  PrimeStage::DropdownSpec spec;
  spec.label = "Select";
  spec.backgroundStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  spec.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  spec.indicatorStyle = Studio::textToken(Studio::TextRole::BodyMuted);
  PrimeStage::UiNode dropdown = root.createDropdown(spec);
  CHECK(frame.getNode(dropdown.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage dropdown inferred width grows with label length") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  PrimeStage::DropdownSpec base;
  base.backgroundStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  base.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  base.indicatorStyle = Studio::textToken(Studio::TextRole::BodyMuted);
  base.indicator = "v";

  PrimeStage::DropdownSpec shortSpec = base;
  shortSpec.label = "A";
  PrimeStage::UiNode shortDrop = root.createDropdown(shortSpec);

  PrimeStage::DropdownSpec longSpec = base;
  longSpec.label = "LongerLabel";
  PrimeStage::UiNode longDrop = root.createDropdown(longSpec);

  PrimeFrame::Node const* shortNode = frame.getNode(shortDrop.nodeId());
  PrimeFrame::Node const* longNode = frame.getNode(longDrop.nodeId());
  REQUIRE(shortNode != nullptr);
  REQUIRE(longNode != nullptr);
  REQUIRE(shortNode->sizeHint.width.preferred.has_value());
  REQUIRE(longNode->sizeHint.width.preferred.has_value());
  CHECK(longNode->sizeHint.width.preferred.value() >
        shortNode->sizeHint.width.preferred.value());
}

TEST_CASE("PrimeStage dropdown inferred width grows with indicator length") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  PrimeStage::DropdownSpec base;
  base.backgroundStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  base.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  base.indicatorStyle = Studio::textToken(Studio::TextRole::BodyMuted);
  base.label = "Label";

  PrimeStage::DropdownSpec shortSpec = base;
  shortSpec.indicator = "v";
  PrimeStage::UiNode shortDrop = root.createDropdown(shortSpec);

  PrimeStage::DropdownSpec longSpec = base;
  longSpec.indicator = "vvvv";
  PrimeStage::UiNode longDrop = root.createDropdown(longSpec);

  PrimeFrame::Node const* shortNode = frame.getNode(shortDrop.nodeId());
  PrimeFrame::Node const* longNode = frame.getNode(longDrop.nodeId());
  REQUIRE(shortNode != nullptr);
  REQUIRE(longNode != nullptr);
  REQUIRE(shortNode->sizeHint.width.preferred.has_value());
  REQUIRE(longNode->sizeHint.width.preferred.has_value());
  CHECK(longNode->sizeHint.width.preferred.value() >
        shortNode->sizeHint.width.preferred.value());
}

TEST_CASE("PrimeStage dropdown inferred width grows with indicator gap") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  PrimeStage::DropdownSpec smallGap;
  smallGap.label = "Label";
  smallGap.backgroundStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  smallGap.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  smallGap.indicatorStyle = Studio::textToken(Studio::TextRole::BodyMuted);
  smallGap.indicator = "v";
  smallGap.indicatorGap = 4.0f;
  PrimeStage::UiNode smallDrop = root.createDropdown(smallGap);

  PrimeStage::DropdownSpec largeGap = smallGap;
  largeGap.indicatorGap = 12.0f;
  PrimeStage::UiNode largeDrop = root.createDropdown(largeGap);

  PrimeFrame::Node const* smallNode = frame.getNode(smallDrop.nodeId());
  PrimeFrame::Node const* largeNode = frame.getNode(largeDrop.nodeId());
  REQUIRE(smallNode != nullptr);
  REQUIRE(largeNode != nullptr);
  REQUIRE(smallNode->sizeHint.width.preferred.has_value());
  REQUIRE(largeNode->sizeHint.width.preferred.has_value());
  CHECK(largeNode->sizeHint.width.preferred.value() -
        smallNode->sizeHint.width.preferred.value() ==
        doctest::Approx(8.0f));
}

TEST_CASE("PrimeStage dropdown inferred width grows with indicator when label empty") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  PrimeStage::DropdownSpec base;
  base.backgroundStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  base.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  base.indicatorStyle = Studio::textToken(Studio::TextRole::BodyMuted);
  base.label = "";

  PrimeStage::DropdownSpec shortSpec = base;
  shortSpec.indicator = "v";
  PrimeStage::UiNode shortDrop = root.createDropdown(shortSpec);

  PrimeStage::DropdownSpec longSpec = base;
  longSpec.indicator = "vvvv";
  PrimeStage::UiNode longDrop = root.createDropdown(longSpec);

  PrimeFrame::Node const* shortNode = frame.getNode(shortDrop.nodeId());
  PrimeFrame::Node const* longNode = frame.getNode(longDrop.nodeId());
  REQUIRE(shortNode != nullptr);
  REQUIRE(longNode != nullptr);
  REQUIRE(shortNode->sizeHint.width.preferred.has_value());
  REQUIRE(longNode->sizeHint.width.preferred.has_value());
  CHECK(longNode->sizeHint.width.preferred.value() >
        shortNode->sizeHint.width.preferred.value());
}

TEST_CASE("PrimeStage dropdown inferred height grows with padding") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 60.0f);

  PrimeStage::DropdownSpec base;
  base.label = "Label";
  base.backgroundStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  base.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  base.indicatorStyle = Studio::textToken(Studio::TextRole::BodyMuted);
  base.indicator = "v";

  PrimeStage::DropdownSpec smallPad = base;
  smallPad.paddingX = 4.0f;
  PrimeStage::UiNode smallDrop = root.createDropdown(smallPad);

  PrimeStage::DropdownSpec largePad = base;
  largePad.paddingX = 10.0f;
  PrimeStage::UiNode largeDrop = root.createDropdown(largePad);

  PrimeFrame::Node const* smallNode = frame.getNode(smallDrop.nodeId());
  PrimeFrame::Node const* largeNode = frame.getNode(largeDrop.nodeId());
  REQUIRE(smallNode != nullptr);
  REQUIRE(largeNode != nullptr);
  REQUIRE(smallNode->sizeHint.height.preferred.has_value());
  REQUIRE(largeNode->sizeHint.height.preferred.has_value());
  CHECK(largeNode->sizeHint.height.preferred.value() -
        smallNode->sizeHint.height.preferred.value() ==
        doctest::Approx(6.0f));
}

TEST_CASE("PrimeStage dropdown inferred height grows with text style") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 60.0f);

  PrimeStage::DropdownSpec smallSpec;
  smallSpec.label = "Label";
  smallSpec.backgroundStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  smallSpec.textStyle = Studio::textToken(Studio::TextRole::SmallMuted);
  smallSpec.indicatorStyle = Studio::textToken(Studio::TextRole::SmallMuted);
  smallSpec.indicator = "v";
  PrimeStage::UiNode smallDrop = root.createDropdown(smallSpec);

  PrimeStage::DropdownSpec largeSpec = smallSpec;
  largeSpec.textStyle = Studio::textToken(Studio::TextRole::TitleBright);
  PrimeStage::UiNode largeDrop = root.createDropdown(largeSpec);

  PrimeFrame::Node const* smallNode = frame.getNode(smallDrop.nodeId());
  PrimeFrame::Node const* largeNode = frame.getNode(largeDrop.nodeId());
  REQUIRE(smallNode != nullptr);
  REQUIRE(largeNode != nullptr);
  REQUIRE(smallNode->sizeHint.height.preferred.has_value());
  REQUIRE(largeNode->sizeHint.height.preferred.has_value());
  CHECK(largeNode->sizeHint.height.preferred.value() >
        smallNode->sizeHint.height.preferred.value());
}

TEST_CASE("PrimeStage dropdown inferred height with indicator only") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 60.0f);

  PrimeStage::DropdownSpec base;
  base.label = "";
  base.backgroundStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  base.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  base.indicatorStyle = Studio::textToken(Studio::TextRole::BodyBright);
  base.indicator = "v";

  PrimeStage::DropdownSpec shortSpec = base;
  shortSpec.paddingX = 4.0f;
  PrimeStage::UiNode shortDrop = root.createDropdown(shortSpec);

  PrimeStage::DropdownSpec tallSpec = base;
  tallSpec.paddingX = 10.0f;
  PrimeStage::UiNode tallDrop = root.createDropdown(tallSpec);

  PrimeFrame::Node const* shortNode = frame.getNode(shortDrop.nodeId());
  PrimeFrame::Node const* tallNode = frame.getNode(tallDrop.nodeId());
  REQUIRE(shortNode != nullptr);
  REQUIRE(tallNode != nullptr);
  REQUIRE(shortNode->sizeHint.height.preferred.has_value());
  REQUIRE(tallNode->sizeHint.height.preferred.has_value());
  CHECK(tallNode->sizeHint.height.preferred.value() >
        shortNode->sizeHint.height.preferred.value());
}

TEST_CASE("PrimeStage dropdown inferred height with indicator text style change") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 60.0f);

  PrimeStage::DropdownSpec shortSpec;
  shortSpec.label = "";
  shortSpec.backgroundStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  shortSpec.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  shortSpec.indicatorStyle = Studio::textToken(Studio::TextRole::SmallMuted);
  shortSpec.indicator = "v";
  PrimeStage::UiNode shortDrop = root.createDropdown(shortSpec);

  PrimeStage::DropdownSpec tallSpec = shortSpec;
  tallSpec.indicatorStyle = Studio::textToken(Studio::TextRole::TitleBright);
  PrimeStage::UiNode tallDrop = root.createDropdown(tallSpec);

  PrimeFrame::Node const* shortNode = frame.getNode(shortDrop.nodeId());
  PrimeFrame::Node const* tallNode = frame.getNode(tallDrop.nodeId());
  REQUIRE(shortNode != nullptr);
  REQUIRE(tallNode != nullptr);
  REQUIRE(shortNode->sizeHint.height.preferred.has_value());
  REQUIRE(tallNode->sizeHint.height.preferred.has_value());
  CHECK(tallNode->sizeHint.height.preferred.value() >=
        shortNode->sizeHint.height.preferred.value());
}

TEST_CASE("PrimeStage dropdown inferred height grows with label text style") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 60.0f);

  PrimeStage::DropdownSpec smallSpec;
  smallSpec.label = "Label";
  smallSpec.backgroundStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  smallSpec.textStyle = Studio::textToken(Studio::TextRole::SmallMuted);
  smallSpec.indicatorStyle = Studio::textToken(Studio::TextRole::SmallMuted);
  smallSpec.indicator = "v";
  PrimeStage::UiNode smallDrop = root.createDropdown(smallSpec);

  PrimeStage::DropdownSpec largeSpec = smallSpec;
  largeSpec.textStyle = Studio::textToken(Studio::TextRole::TitleBright);
  PrimeStage::UiNode largeDrop = root.createDropdown(largeSpec);

  PrimeFrame::Node const* smallNode = frame.getNode(smallDrop.nodeId());
  PrimeFrame::Node const* largeNode = frame.getNode(largeDrop.nodeId());
  REQUIRE(smallNode != nullptr);
  REQUIRE(largeNode != nullptr);
  REQUIRE(smallNode->sizeHint.height.preferred.has_value());
  REQUIRE(largeNode->sizeHint.height.preferred.has_value());
  CHECK(largeNode->sizeHint.height.preferred.value() >
        smallNode->sizeHint.height.preferred.value());
}

TEST_CASE("PrimeStage progress bar creates a node") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  PrimeStage::ProgressBarSpec spec;
  spec.size.preferredWidth = 160.0f;
  spec.size.preferredHeight = 12.0f;
  spec.value = 0.45f;
  spec.trackStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  spec.fillStyle = Studio::rectToken(Studio::RectRole::Accent);
  PrimeStage::UiNode bar = root.createProgressBar(spec);
  CHECK(frame.getNode(bar.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage studio progress bar defaults size") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  Studio::ProgressBarSpec spec;
  spec.value = 0.5f;
  PrimeStage::UiNode bar = Studio::createProgressBar(root, spec);

  PrimeFrame::Node const* barNode = frame.getNode(bar.nodeId());
  REQUIRE(barNode != nullptr);
  REQUIRE(barNode->sizeHint.width.preferred.has_value());
  REQUIRE(barNode->sizeHint.height.preferred.has_value());
  CHECK(barNode->sizeHint.width.preferred.value() ==
        doctest::Approx(Studio::StudioDefaults::ControlWidthL));
  CHECK(barNode->sizeHint.height.preferred.value() ==
        doctest::Approx(Studio::StudioDefaults::OpacityBarHeight));
}

TEST_CASE("PrimeStage progress bar clamps fill width") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  PrimeStage::ProgressBarSpec spec;
  spec.size.preferredWidth = 100.0f;
  spec.size.preferredHeight = 10.0f;
  spec.value = 0.0f;
  spec.minFillWidth = 20.0f;
  spec.trackStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  spec.fillStyle = Studio::rectToken(Studio::RectRole::Accent);
  PrimeStage::UiNode bar = root.createProgressBar(spec);

  PrimeStage::ProgressBarSpec spec2 = spec;
  spec2.size.preferredWidth = 80.0f;
  spec2.value = 2.0f;
  spec2.minFillWidth = 0.0f;
  PrimeStage::UiNode bar2 = root.createProgressBar(spec2);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 200.0f;
  options.rootHeight = 40.0f;
  layoutEngine.layout(frame, layout, options);

  auto find_fill_width = [&](PrimeStage::UiNode const& barNode,
                             PrimeFrame::RectStyleToken fillToken) {
    PrimeFrame::Node const* node = frame.getNode(barNode.nodeId());
    REQUIRE(node != nullptr);
    float width = -1.0f;
    for (PrimeFrame::NodeId childId : node->children) {
      PrimeFrame::Node const* child = frame.getNode(childId);
      if (!child || child->primitives.empty()) {
        continue;
      }
      PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
      if (prim && prim->rect.token == fillToken) {
        PrimeFrame::LayoutOut const* out = layout.get(childId);
        REQUIRE(out != nullptr);
        width = out->absW;
        break;
      }
    }
    return width;
  };

  float fillWidth = find_fill_width(bar, spec.fillStyle);
  CHECK(fillWidth >= 0.0f);
  CHECK(fillWidth == doctest::Approx(20.0f));

  float fillWidth2 = find_fill_width(bar2, spec2.fillStyle);
  CHECK(fillWidth2 >= 0.0f);
  CHECK(fillWidth2 == doctest::Approx(80.0f));
}

TEST_CASE("PrimeStage scroll view clamps vertical thumb length and offset") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 200.0f);

  PrimeStage::ScrollViewSpec spec;
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 120.0f;
  spec.showVertical = true;
  spec.showHorizontal = false;
  spec.vertical.thickness = 8.0f;
  spec.vertical.inset = 4.0f;
  spec.vertical.startPadding = 10.0f;
  spec.vertical.endPadding = 6.0f;
  spec.vertical.thumbLength = 200.0f;
  spec.vertical.thumbOffset = 50.0f;
  spec.vertical.trackStyle = 1111;
  spec.vertical.thumbStyle = 2222;
  PrimeStage::ScrollView view = root.createScrollView(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 240.0f;
  options.rootHeight = 200.0f;
  layoutEngine.layout(frame, layout, options);

  auto find_node_by_token = [&](PrimeFrame::RectStyleToken token) {
    PrimeFrame::Node const* rootNode = frame.getNode(view.root.nodeId());
    REQUIRE(rootNode != nullptr);
    for (PrimeFrame::NodeId childId : rootNode->children) {
      PrimeFrame::Node const* child = frame.getNode(childId);
      if (!child || child->primitives.empty()) {
        continue;
      }
      PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
      if (prim && prim->rect.token == token) {
        return childId;
      }
    }
    return PrimeFrame::NodeId{};
  };

  PrimeFrame::NodeId trackId = find_node_by_token(spec.vertical.trackStyle);
  PrimeFrame::NodeId thumbId = find_node_by_token(spec.vertical.thumbStyle);
  REQUIRE(trackId.isValid());
  REQUIRE(thumbId.isValid());

  PrimeFrame::LayoutOut const* trackOut = layout.get(trackId);
  PrimeFrame::LayoutOut const* thumbOut = layout.get(thumbId);
  REQUIRE(trackOut != nullptr);
  REQUIRE(thumbOut != nullptr);

  float expectedTrackH = 120.0f - spec.vertical.startPadding - spec.vertical.endPadding;
  float expectedTrackW = spec.vertical.thickness;
  float expectedTrackX = 200.0f - spec.vertical.inset;
  float expectedTrackY = spec.vertical.startPadding;

  CHECK(trackOut->absW == doctest::Approx(expectedTrackW));
  CHECK(trackOut->absH == doctest::Approx(expectedTrackH));
  CHECK(trackOut->absX == doctest::Approx(expectedTrackX));
  CHECK(trackOut->absY == doctest::Approx(expectedTrackY));

  CHECK(thumbOut->absW == doctest::Approx(expectedTrackW));
  CHECK(thumbOut->absH == doctest::Approx(expectedTrackH));
  CHECK(thumbOut->absX == doctest::Approx(expectedTrackX));
  CHECK(thumbOut->absY == doctest::Approx(expectedTrackY));
}

TEST_CASE("PrimeStage scroll view clamps horizontal thumb offset") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 220.0f, 140.0f);

  PrimeStage::ScrollViewSpec spec;
  spec.size.preferredWidth = 180.0f;
  spec.size.preferredHeight = 100.0f;
  spec.showHorizontal = true;
  spec.showVertical = false;
  spec.horizontal.thickness = 6.0f;
  spec.horizontal.inset = 6.0f;
  spec.horizontal.startPadding = 12.0f;
  spec.horizontal.endPadding = 8.0f;
  spec.horizontal.thumbLength = 40.0f;
  spec.horizontal.thumbOffset = 200.0f;
  spec.horizontal.trackStyle = 3333;
  spec.horizontal.thumbStyle = 4444;
  PrimeStage::ScrollView view = root.createScrollView(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 220.0f;
  options.rootHeight = 140.0f;
  layoutEngine.layout(frame, layout, options);

  auto find_node_by_token = [&](PrimeFrame::RectStyleToken token) {
    PrimeFrame::Node const* rootNode = frame.getNode(view.root.nodeId());
    REQUIRE(rootNode != nullptr);
    for (PrimeFrame::NodeId childId : rootNode->children) {
      PrimeFrame::Node const* child = frame.getNode(childId);
      if (!child || child->primitives.empty()) {
        continue;
      }
      PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
      if (prim && prim->rect.token == token) {
        return childId;
      }
    }
    return PrimeFrame::NodeId{};
  };

  PrimeFrame::NodeId trackId = find_node_by_token(spec.horizontal.trackStyle);
  PrimeFrame::NodeId thumbId = find_node_by_token(spec.horizontal.thumbStyle);
  REQUIRE(trackId.isValid());
  REQUIRE(thumbId.isValid());

  PrimeFrame::LayoutOut const* trackOut = layout.get(trackId);
  PrimeFrame::LayoutOut const* thumbOut = layout.get(thumbId);
  REQUIRE(trackOut != nullptr);
  REQUIRE(thumbOut != nullptr);

  float expectedTrackW = 180.0f - spec.horizontal.startPadding - spec.horizontal.endPadding;
  float expectedTrackH = spec.horizontal.thickness;
  float expectedTrackX = spec.horizontal.startPadding;
  float expectedTrackY = 100.0f - spec.horizontal.inset;
  float expectedThumbW = std::min(expectedTrackW, spec.horizontal.thumbLength);
  float expectedMaxOffset = std::max(0.0f, expectedTrackW - expectedThumbW);
  float expectedThumbX = expectedTrackX + expectedMaxOffset;

  CHECK(trackOut->absW == doctest::Approx(expectedTrackW));
  CHECK(trackOut->absH == doctest::Approx(expectedTrackH));
  CHECK(trackOut->absX == doctest::Approx(expectedTrackX));
  CHECK(trackOut->absY == doctest::Approx(expectedTrackY));

  CHECK(thumbOut->absW == doctest::Approx(expectedThumbW));
  CHECK(thumbOut->absH == doctest::Approx(expectedTrackH));
  CHECK(thumbOut->absX == doctest::Approx(expectedThumbX));
  CHECK(thumbOut->absY == doctest::Approx(expectedTrackY));
}

TEST_CASE("PrimeStage scroll view returns invalid content when size missing") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 120.0f, 80.0f);

  PrimeStage::ScrollViewSpec spec;
  PrimeStage::ScrollView view = root.createScrollView(spec);

  CHECK(view.root.nodeId() == root.nodeId());
  CHECK(!view.content.nodeId().isValid());
}

TEST_CASE("PrimeStage table shrinks last column on fixed overflow") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 300.0f, 120.0f);

  PrimeStage::TableSpec spec;
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 60.0f;
  spec.columns = {
      PrimeStage::TableColumn{"First", 120.0f,
                              Studio::textToken(Studio::TextRole::SmallBright),
                              Studio::textToken(Studio::TextRole::SmallMuted)},
      PrimeStage::TableColumn{"Second", 100.0f,
                              Studio::textToken(Studio::TextRole::SmallBright),
                              Studio::textToken(Studio::TextRole::SmallMuted)}
  };
  spec.headerHeight = 24.0f;
  spec.headerStyle = 7001;
  spec.rowStyle = 7002;
  spec.rowAltStyle = 7003;
  spec.dividerStyle = 7004;
  spec.showColumnDividers = true;

  PrimeStage::UiNode table = root.createTable(spec);

  PrimeFrame::LayoutEngine layoutEngine;
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = 300.0f;
  options.rootHeight = 120.0f;
  layoutEngine.layout(frame, layout, options);

  PrimeFrame::Node const* tableNode = frame.getNode(table.nodeId());
  REQUIRE(tableNode != nullptr);

  PrimeFrame::NodeId headerRowId{};
  for (PrimeFrame::NodeId childId : tableNode->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (!child || child->primitives.empty()) {
      continue;
    }
    PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
    if (prim && prim->rect.token == spec.headerStyle) {
      headerRowId = childId;
      break;
    }
  }
  REQUIRE(headerRowId.isValid());

  PrimeFrame::Node const* headerRow = frame.getNode(headerRowId);
  REQUIRE(headerRow != nullptr);
  REQUIRE(headerRow->children.size() >= 2);

  std::vector<float> cellWidths;
  for (PrimeFrame::NodeId childId : headerRow->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (!child) {
      continue;
    }
    if (!child->primitives.empty()) {
      PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
      if (prim && prim->rect.token == spec.dividerStyle) {
        continue;
      }
    }
    PrimeFrame::LayoutOut const* out = layout.get(childId);
    REQUIRE(out != nullptr);
    cellWidths.push_back(out->absW);
  }

  REQUIRE(cellWidths.size() == 2);
  CHECK(cellWidths[0] == doctest::Approx(120.0f));
  CHECK(cellWidths[1] == doctest::Approx(79.0f));
}

TEST_CASE("PrimeStage tabs apply active style to selected index") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 240.0f, 40.0f);

  PrimeStage::TabsSpec spec;
  spec.labels = {"Alpha", "Beta", "Gamma"};
  spec.selectedIndex = 1;
  spec.tabStyle = Studio::rectToken(Studio::RectRole::Panel);
  spec.activeTabStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  spec.textStyle = Studio::textToken(Studio::TextRole::SmallMuted);
  spec.activeTextStyle = Studio::textToken(Studio::TextRole::SmallBright);
  PrimeStage::UiNode tabs = root.createTabs(spec);

  PrimeFrame::Node const* row = frame.getNode(tabs.nodeId());
  REQUIRE(row != nullptr);
  REQUIRE(row->children.size() == 3);
  for (size_t i = 0; i < row->children.size(); ++i) {
    PrimeFrame::Node const* tabNode = frame.getNode(row->children[i]);
    REQUIRE(tabNode != nullptr);
    REQUIRE(!tabNode->primitives.empty());
    PrimeFrame::Primitive const* bg = frame.getPrimitive(tabNode->primitives.front());
    REQUIRE(bg != nullptr);
    PrimeFrame::RectStyleToken expected = (static_cast<int>(i) == spec.selectedIndex)
                                             ? spec.activeTabStyle
                                             : spec.tabStyle;
    CHECK(bg->rect.token == expected);
  }
}

TEST_CASE("PrimeStage dropdown without label uses spacer") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 40.0f);

  PrimeStage::DropdownSpec spec;
  spec.label = "";
  spec.backgroundStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  spec.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  spec.indicatorStyle = Studio::textToken(Studio::TextRole::BodyMuted);
  PrimeStage::UiNode dropdown = root.createDropdown(spec);

  PrimeFrame::Node const* dropdownNode = frame.getNode(dropdown.nodeId());
  REQUIRE(dropdownNode != nullptr);
  REQUIRE(dropdownNode->children.size() == 2);

  PrimeFrame::Node const* spacer = frame.getNode(dropdownNode->children[0]);
  REQUIRE(spacer != nullptr);
  CHECK(spacer->primitives.empty());

  PrimeFrame::Node const* indicator = frame.getNode(dropdownNode->children[1]);
  REQUIRE(indicator != nullptr);
  REQUIRE(!indicator->primitives.empty());
  PrimeFrame::Primitive const* prim = frame.getPrimitive(indicator->primitives.front());
  REQUIRE(prim != nullptr);
  CHECK(prim->type == PrimeFrame::PrimitiveType::Text);
}

TEST_CASE("PrimeStage status bar creates a node") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 30.0f);

  Studio::StatusBarSpec spec;
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 24.0f;
  spec.leftText = "Ready";
  spec.rightText = "Demo";
  PrimeStage::UiNode bar = Studio::createStatusBar(root, spec);
  CHECK(frame.getNode(bar.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage status bar defaults height") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 30.0f);

  Studio::StatusBarSpec spec;
  spec.leftText = "Left";
  PrimeStage::UiNode bar = Studio::createStatusBar(root, spec);

  PrimeFrame::Node const* barNode = frame.getNode(bar.nodeId());
  REQUIRE(barNode != nullptr);
  REQUIRE(barNode->sizeHint.height.preferred.has_value());
  CHECK(barNode->sizeHint.height.preferred.value() ==
        doctest::Approx(Studio::StudioDefaults::StatusHeight));
}

TEST_CASE("PrimeStage status bar defaults height when no text") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 30.0f);

  Studio::StatusBarSpec spec;
  spec.leftText = "";
  spec.rightText = "";
  PrimeStage::UiNode bar = Studio::createStatusBar(root, spec);

  PrimeFrame::Node const* barNode = frame.getNode(bar.nodeId());
  REQUIRE(barNode != nullptr);
  REQUIRE(barNode->sizeHint.height.preferred.has_value());
  CHECK(barNode->sizeHint.height.preferred.value() ==
        doctest::Approx(Studio::StudioDefaults::StatusHeight));
}

TEST_CASE("PrimeStage status bar infers width from text and padding") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 300.0f, 30.0f);

  Studio::StatusBarSpec spec;
  spec.paddingX = 6.0f;
  spec.leftText = "Left";
  spec.rightText = "Right";
  PrimeStage::UiNode bar = Studio::createStatusBar(root, spec);

  PrimeFrame::Node const* barNode = frame.getNode(bar.nodeId());
  REQUIRE(barNode != nullptr);
  REQUIRE(barNode->sizeHint.width.preferred.has_value());

  PrimeFrame::Node const* leftNode = nullptr;
  PrimeFrame::Node const* rightNode = nullptr;
  for (PrimeFrame::NodeId childId : barNode->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (!child || child->primitives.empty()) {
      continue;
    }
    PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
    if (!prim || prim->type != PrimeFrame::PrimitiveType::Text) {
      continue;
    }
    if (prim->textBlock.text == spec.leftText) {
      leftNode = child;
    } else if (prim->textBlock.text == spec.rightText) {
      rightNode = child;
    }
  }

  REQUIRE(leftNode != nullptr);
  REQUIRE(rightNode != nullptr);
  REQUIRE(leftNode->sizeHint.width.preferred.has_value());
  REQUIRE(rightNode->sizeHint.width.preferred.has_value());

  float expected = spec.paddingX * 2.0f +
                   leftNode->sizeHint.width.preferred.value() +
                   rightNode->sizeHint.width.preferred.value();
  CHECK(barNode->sizeHint.width.preferred.value() == doctest::Approx(expected));
}

TEST_CASE("PrimeStage status bar positions right text from padding") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 300.0f, 30.0f);

  Studio::StatusBarSpec spec;
  spec.size.preferredWidth = 180.0f;
  spec.size.preferredHeight = 24.0f;
  spec.paddingX = 8.0f;
  spec.leftText = "Left";
  spec.rightText = "Right";
  PrimeStage::UiNode bar = Studio::createStatusBar(root, spec);

  PrimeFrame::Node const* barNode = frame.getNode(bar.nodeId());
  REQUIRE(barNode != nullptr);

  PrimeFrame::Node const* rightNode = nullptr;
  for (PrimeFrame::NodeId childId : barNode->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (!child || child->primitives.empty()) {
      continue;
    }
    PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
    if (prim && prim->type == PrimeFrame::PrimitiveType::Text &&
        prim->textBlock.text == spec.rightText) {
      rightNode = child;
      break;
    }
  }

  REQUIRE(rightNode != nullptr);
  REQUIRE(rightNode->sizeHint.width.preferred.has_value());

  float rightWidth = rightNode->sizeHint.width.preferred.value();
  float expectedX = spec.size.preferredWidth.value() - spec.paddingX - rightWidth;
  CHECK(rightNode->localX == doctest::Approx(expectedX));
}

TEST_CASE("PrimeStage status bar clamps right text when width is small") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 30.0f);

  Studio::StatusBarSpec spec;
  spec.size.preferredWidth = 20.0f;
  spec.size.preferredHeight = 24.0f;
  spec.paddingX = 8.0f;
  spec.leftText = "";
  spec.rightText = "Right";
  PrimeStage::UiNode bar = Studio::createStatusBar(root, spec);

  PrimeFrame::Node const* barNode = frame.getNode(bar.nodeId());
  REQUIRE(barNode != nullptr);

  PrimeFrame::Node const* rightNode = nullptr;
  for (PrimeFrame::NodeId childId : barNode->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (!child || child->primitives.empty()) {
      continue;
    }
    PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
    if (prim && prim->type == PrimeFrame::PrimitiveType::Text &&
        prim->textBlock.text == spec.rightText) {
      rightNode = child;
      break;
    }
  }

  REQUIRE(rightNode != nullptr);
  REQUIRE(rightNode->sizeHint.width.preferred.has_value());
  CHECK(rightNode->sizeHint.width.preferred.value() ==
        doctest::Approx(spec.size.preferredWidth.value() - spec.paddingX));
  CHECK(rightNode->localX == doctest::Approx(0.0f));
}

TEST_CASE("PrimeStage status bar inferred width uses only right text") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 30.0f);

  Studio::StatusBarSpec spec;
  spec.paddingX = 6.0f;
  spec.leftText = "";
  spec.rightText = "Right";
  PrimeStage::UiNode bar = Studio::createStatusBar(root, spec);

  PrimeFrame::Node const* barNode = frame.getNode(bar.nodeId());
  REQUIRE(barNode != nullptr);
  REQUIRE(barNode->sizeHint.width.preferred.has_value());

  PrimeFrame::Node const* rightNode = nullptr;
  for (PrimeFrame::NodeId childId : barNode->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (!child || child->primitives.empty()) {
      continue;
    }
    PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
    if (prim && prim->type == PrimeFrame::PrimitiveType::Text &&
        prim->textBlock.text == spec.rightText) {
      rightNode = child;
      break;
    }
  }

  REQUIRE(rightNode != nullptr);
  REQUIRE(rightNode->sizeHint.width.preferred.has_value());
  CHECK(barNode->sizeHint.width.preferred.value() >
        rightNode->sizeHint.width.preferred.value());
}

TEST_CASE("PrimeStage status bar with no text uses padding width") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 30.0f);

  Studio::StatusBarSpec spec;
  spec.paddingX = 6.0f;
  spec.leftText = "";
  spec.rightText = "";
  PrimeStage::UiNode bar = Studio::createStatusBar(root, spec);

  PrimeFrame::Node const* barNode = frame.getNode(bar.nodeId());
  REQUIRE(barNode != nullptr);
  REQUIRE(barNode->sizeHint.width.preferred.has_value());
  CHECK(barNode->sizeHint.width.preferred.value() ==
        doctest::Approx(spec.paddingX * 2.0f));
  CHECK(barNode->children.empty());
}

TEST_CASE("PrimeStage status bar width grows with padding when empty") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 30.0f);

  Studio::StatusBarSpec smallSpec;
  smallSpec.paddingX = 6.0f;
  PrimeStage::UiNode smallBar = Studio::createStatusBar(root, smallSpec);

  Studio::StatusBarSpec largeSpec;
  largeSpec.paddingX = 10.0f;
  PrimeStage::UiNode largeBar = Studio::createStatusBar(root, largeSpec);

  PrimeFrame::Node const* smallNode = frame.getNode(smallBar.nodeId());
  PrimeFrame::Node const* largeNode = frame.getNode(largeBar.nodeId());
  REQUIRE(smallNode != nullptr);
  REQUIRE(largeNode != nullptr);
  REQUIRE(smallNode->sizeHint.width.preferred.has_value());
  REQUIRE(largeNode->sizeHint.width.preferred.has_value());
  CHECK(largeNode->sizeHint.width.preferred.value() -
        smallNode->sizeHint.width.preferred.value() ==
        doctest::Approx(8.0f));
}

TEST_CASE("PrimeStage status bar inferred width with left text only") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 30.0f);

  Studio::StatusBarSpec spec;
  spec.paddingX = 6.0f;
  spec.leftText = "Left";
  spec.rightText = "";
  PrimeStage::UiNode bar = Studio::createStatusBar(root, spec);

  PrimeFrame::Node const* barNode = frame.getNode(bar.nodeId());
  REQUIRE(barNode != nullptr);
  REQUIRE(barNode->sizeHint.width.preferred.has_value());

  PrimeFrame::Node const* leftNode = nullptr;
  for (PrimeFrame::NodeId childId : barNode->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (!child || child->primitives.empty()) {
      continue;
    }
    PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
    if (prim && prim->type == PrimeFrame::PrimitiveType::Text &&
        prim->textBlock.text == spec.leftText) {
      leftNode = child;
      break;
    }
  }

  REQUIRE(leftNode != nullptr);
  REQUIRE(leftNode->sizeHint.width.preferred.has_value());
  float expected = spec.paddingX * 2.0f + leftNode->sizeHint.width.preferred.value();
  CHECK(barNode->sizeHint.width.preferred.value() == doctest::Approx(expected));
}

TEST_CASE("PrimeStage status bar clamps left text when width is small") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 30.0f);

  Studio::StatusBarSpec spec;
  spec.size.preferredWidth = 20.0f;
  spec.size.preferredHeight = 24.0f;
  spec.paddingX = 8.0f;
  spec.leftText = "Left";
  spec.rightText = "Right";
  PrimeStage::UiNode bar = Studio::createStatusBar(root, spec);

  PrimeFrame::Node const* barNode = frame.getNode(bar.nodeId());
  REQUIRE(barNode != nullptr);

  PrimeFrame::Node const* leftNode = nullptr;
  PrimeFrame::Node const* rightNode = nullptr;
  for (PrimeFrame::NodeId childId : barNode->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (!child || child->primitives.empty()) {
      continue;
    }
    PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
    if (prim && prim->type == PrimeFrame::PrimitiveType::Text) {
      if (prim->textBlock.text == spec.leftText) {
        leftNode = child;
      } else if (prim->textBlock.text == spec.rightText) {
        rightNode = child;
      }
    }
  }

  REQUIRE(leftNode != nullptr);
  REQUIRE(rightNode != nullptr);
  CHECK(!leftNode->sizeHint.width.preferred.has_value());
  CHECK(leftNode->localX == doctest::Approx(spec.paddingX));
}

TEST_CASE("PrimeStage status bar overlap uses clamped widths") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 30.0f);

  Studio::StatusBarSpec spec;
  spec.size.preferredWidth = 24.0f;
  spec.size.preferredHeight = 24.0f;
  spec.paddingX = 8.0f;
  spec.leftText = "Left";
  spec.rightText = "Right";
  PrimeStage::UiNode bar = Studio::createStatusBar(root, spec);

  PrimeFrame::Node const* barNode = frame.getNode(bar.nodeId());
  REQUIRE(barNode != nullptr);

  PrimeFrame::Node const* leftNode = nullptr;
  PrimeFrame::Node const* rightNode = nullptr;
  for (PrimeFrame::NodeId childId : barNode->children) {
    PrimeFrame::Node const* child = frame.getNode(childId);
    if (!child || child->primitives.empty()) {
      continue;
    }
    PrimeFrame::Primitive const* prim = frame.getPrimitive(child->primitives.front());
    if (prim && prim->type == PrimeFrame::PrimitiveType::Text) {
      if (prim->textBlock.text == spec.leftText) {
        leftNode = child;
      } else if (prim->textBlock.text == spec.rightText) {
        rightNode = child;
      }
    }
  }

  REQUIRE(leftNode != nullptr);
  REQUIRE(rightNode != nullptr);
  CHECK(leftNode->localX == doctest::Approx(spec.paddingX));
  CHECK(rightNode->localX == doctest::Approx(0.0f));
}

TEST_CASE("PrimeStage text field defaults size hints") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 30.0f);

  PrimeStage::SizeSpec size;
  PrimeStage::UiNode field = Studio::createTextField(root, "Placeholder", size);

  PrimeFrame::Node const* fieldNode = frame.getNode(field.nodeId());
  REQUIRE(fieldNode != nullptr);
  REQUIRE(fieldNode->sizeHint.height.preferred.has_value());
  CHECK(fieldNode->sizeHint.height.preferred.value() ==
        doctest::Approx(Studio::StudioDefaults::ControlHeight));
  REQUIRE(fieldNode->sizeHint.width.min.has_value());
  CHECK(fieldNode->sizeHint.width.min.value() ==
        doctest::Approx(Studio::StudioDefaults::FieldWidthL));
}

TEST_CASE("PrimeStage text field does not force min width when stretching") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 30.0f);

  PrimeStage::SizeSpec size;
  size.stretchX = 1.0f;
  PrimeStage::UiNode field = Studio::createTextField(root, "Placeholder", size);

  PrimeFrame::Node const* fieldNode = frame.getNode(field.nodeId());
  REQUIRE(fieldNode != nullptr);
  CHECK(!fieldNode->sizeHint.width.min.has_value());
}

TEST_CASE("PrimeStage property list creates a node") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 80.0f);

  Studio::PropertyListSpec spec;
  spec.size.preferredWidth = 180.0f;
  spec.rows = {
      {"Key", "Value"},
      {"Key2", "Value2"}
  };
  PrimeStage::UiNode list = Studio::createPropertyList(root, spec);
  CHECK(frame.getNode(list.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage card grid creates a node") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 300.0f, 200.0f);

  Studio::CardGridSpec spec;
  spec.size.preferredWidth = 280.0f;
  spec.size.preferredHeight = 120.0f;
  spec.cards = {{"Card", "Detail"}, {"Card", "Detail"}};
  PrimeStage::UiNode grid = Studio::createCardGrid(root, spec);
  CHECK(frame.getNode(grid.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage progress bar creates a node") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 120.0f, 24.0f);

  Studio::ProgressBarSpec spec;
  spec.size.preferredWidth = 120.0f;
  spec.size.preferredHeight = 10.0f;
  spec.value = 0.5f;
  PrimeStage::UiNode bar = Studio::createProgressBar(root, spec);
  CHECK(frame.getNode(bar.nodeId()) != nullptr);
}
