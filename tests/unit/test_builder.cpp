#include "PrimeStage/StudioUi.h"

#include "third_party/doctest.h"

TEST_CASE("PrimeStage UiNode builds panels and labels") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = PrimeStage::createRoot(frame, PrimeStage::Bounds{0.0f, 0.0f, 100.0f, 50.0f});
  CHECK(frame.roots().size() == 1);
  PrimeFrame::Node const* rootNode = frame.getNode(root.nodeId());
  REQUIRE(rootNode != nullptr);
  CHECK(rootNode->sizeHint.width.preferred.has_value());
  CHECK(rootNode->sizeHint.height.preferred.has_value());
  CHECK(*rootNode->sizeHint.width.preferred == doctest::Approx(100.0f));
  CHECK(*rootNode->sizeHint.height.preferred == doctest::Approx(50.0f));

  PrimeStage::PanelSpec panelSpec;
  panelSpec.bounds = PrimeStage::Bounds{10.0f, 5.0f, 40.0f, 20.0f};
  panelSpec.rectStyle = PrimeStage::rectToken(PrimeStage::RectRole::Panel);
  PrimeStage::UiNode panel = root.createPanel(panelSpec);
  PrimeFrame::Node const* panelNode = frame.getNode(panel.nodeId());
  REQUIRE(panelNode != nullptr);
  CHECK(panelNode->parent == root.nodeId());
  CHECK(panelNode->primitives.size() == 1);

  PrimeStage::LabelSpec labelSpec;
  labelSpec.bounds = PrimeStage::Bounds{4.0f, 3.0f, 10.0f, 8.0f};
  labelSpec.text = "Label";
  labelSpec.textStyle = PrimeStage::textToken(PrimeStage::TextRole::BodyBright);
  PrimeStage::UiNode label = panel.createLabel(labelSpec);
  PrimeFrame::Node const* labelNode = frame.getNode(label.nodeId());
  REQUIRE(labelNode != nullptr);
  CHECK(labelNode->parent == panel.nodeId());
  CHECK(labelNode->primitives.size() == 1);
}

TEST_CASE("PrimeStage role helpers create panels and labels") {
  PrimeFrame::Frame frame;
  PrimeStage::UiNode root = PrimeStage::createRoot(frame, PrimeStage::Bounds{0.0f, 0.0f, 50.0f, 20.0f});
  PrimeStage::UiNode panel = root.createPanel(PrimeStage::RectRole::PanelStrong,
                                              PrimeStage::Bounds{2.0f, 2.0f, 20.0f, 10.0f});
  PrimeStage::UiNode label = panel.createLabel("Hello", PrimeStage::TextRole::SmallMuted,
                                               PrimeStage::Bounds{2.0f, 2.0f, 10.0f, 8.0f});
  CHECK(frame.getNode(panel.nodeId()) != nullptr);
  CHECK(frame.getNode(label.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage paragraph creates a node") {
  PrimeFrame::Frame frame;
  PrimeStage::applyStudioTheme(frame);
  PrimeStage::UiNode root = PrimeStage::createRoot(frame, PrimeStage::Bounds{0.0f, 0.0f, 200.0f, 80.0f});

  PrimeStage::UiNode paragraph = root.createParagraph(PrimeStage::Bounds{8.0f, 8.0f, 160.0f, 0.0f},
                                                      "Line one\nLine two",
                                                      PrimeStage::TextRole::SmallMuted);
  CHECK(frame.getNode(paragraph.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage text line creates a node") {
  PrimeFrame::Frame frame;
  PrimeStage::applyStudioTheme(frame);
  PrimeStage::UiNode root = PrimeStage::createRoot(frame, PrimeStage::Bounds{0.0f, 0.0f, 200.0f, 40.0f});

  PrimeStage::UiNode line = root.createTextLine(PrimeStage::Bounds{8.0f, 8.0f, 160.0f, 20.0f},
                                                "Title",
                                                PrimeStage::TextRole::BodyBright,
                                                PrimeFrame::TextAlign::Center);
  CHECK(frame.getNode(line.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage table creates a node") {
  PrimeFrame::Frame frame;
  PrimeStage::applyStudioTheme(frame);
  PrimeStage::UiNode root = PrimeStage::createRoot(frame, PrimeStage::Bounds{0.0f, 0.0f, 200.0f, 120.0f});

  PrimeStage::UiNode table = root.createTable(PrimeStage::Bounds{10.0f, 10.0f, 180.0f, 0.0f},
                                              {
                                                  PrimeStage::TableColumn{"Item", 100.0f,
                                                                          PrimeStage::TextRole::SmallBright,
                                                                          PrimeStage::TextRole::SmallBright},
                                                  PrimeStage::TableColumn{"Status", 80.0f,
                                                                          PrimeStage::TextRole::SmallBright,
                                                                          PrimeStage::TextRole::SmallMuted}
                                              },
                                              {
                                                  {"Row", "Ready"},
                                                  {"Row", "Ready"}
                                              });
  CHECK(frame.getNode(table.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage tree view creates a node") {
  PrimeFrame::Frame frame;
  PrimeStage::applyStudioTheme(frame);
  PrimeStage::UiNode root = PrimeStage::createRoot(frame, PrimeStage::Bounds{0.0f, 0.0f, 200.0f, 120.0f});

  PrimeStage::TreeViewSpec spec;
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 120.0f;
  spec.nodes = {PrimeStage::TreeNode{"Root", {PrimeStage::TreeNode{"Child"}}, true, false}};
  PrimeStage::UiNode tree = root.createTreeView(spec);
  CHECK(frame.getNode(tree.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage section header creates a node") {
  PrimeFrame::Frame frame;
  PrimeStage::applyStudioTheme(frame);
  PrimeStage::UiNode root = PrimeStage::createRoot(frame, PrimeStage::Bounds{0.0f, 0.0f, 120.0f, 40.0f});

  PrimeStage::SizeSpec size;
  size.preferredWidth = 100.0f;
  size.preferredHeight = 20.0f;
  PrimeStage::UiNode header = root.createSectionHeader(size,
                                                       "Header",
                                                       PrimeStage::TextRole::SmallBright);
  CHECK(frame.getNode(header.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage section panel creates a node") {
  PrimeFrame::Frame frame;
  PrimeStage::applyStudioTheme(frame);
  PrimeStage::UiNode root = PrimeStage::createRoot(frame, PrimeStage::Bounds{0.0f, 0.0f, 200.0f, 120.0f});

  PrimeStage::SectionPanelSpec spec;
  spec.size.preferredWidth = 180.0f;
  spec.size.preferredHeight = 80.0f;
  spec.title = "Section";
  PrimeStage::SectionPanel panel = root.createSectionPanel(spec);
  CHECK(frame.getNode(panel.panel.nodeId()) != nullptr);
  CHECK(frame.getNode(panel.content.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage shell creates a layout") {
  PrimeFrame::Frame frame;
  PrimeStage::applyStudioTheme(frame);
  PrimeStage::ShellSpec spec;
  spec.bounds = PrimeStage::Bounds{0.0f, 0.0f, 320.0f, 180.0f};
  PrimeStage::ShellLayout layout = PrimeStage::createShell(frame, spec);
  CHECK(frame.getNode(layout.root.nodeId()) != nullptr);
  CHECK(frame.getNode(layout.topbar.nodeId()) != nullptr);
  CHECK(frame.getNode(layout.sidebar.nodeId()) != nullptr);
  CHECK(frame.getNode(layout.content.nodeId()) != nullptr);
  CHECK(frame.getNode(layout.inspector.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage scroll view creates a node") {
  PrimeFrame::Frame frame;
  PrimeStage::applyStudioTheme(frame);
  PrimeStage::UiNode root = PrimeStage::createRoot(frame, PrimeStage::Bounds{0.0f, 0.0f, 200.0f, 120.0f});

  PrimeStage::ScrollViewSpec spec;
  spec.bounds = PrimeStage::Bounds{0.0f, 0.0f, 200.0f, 120.0f};
  spec.vertical.thumbLength = 24.0f;
  spec.horizontal.thumbLength = 24.0f;
  PrimeStage::UiNode scroll = root.createScrollView(spec);
  CHECK(frame.getNode(scroll.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage button creates a node") {
  PrimeFrame::Frame frame;
  PrimeStage::applyStudioTheme(frame);
  PrimeStage::UiNode root = PrimeStage::createRoot(frame, PrimeStage::Bounds{0.0f, 0.0f, 120.0f, 40.0f});

  PrimeStage::ButtonSpec spec;
  spec.bounds = PrimeStage::Bounds{8.0f, 8.0f, 80.0f, 24.0f};
  spec.label = "Click";
  spec.backgroundStyle = PrimeStage::rectToken(PrimeStage::RectRole::Accent);
  spec.textStyle = PrimeStage::textToken(PrimeStage::TextRole::BodyBright);
  PrimeStage::UiNode button = root.createButton(spec);
  CHECK(frame.getNode(button.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage text field creates a node") {
  PrimeFrame::Frame frame;
  PrimeStage::applyStudioTheme(frame);
  PrimeStage::UiNode root = PrimeStage::createRoot(frame, PrimeStage::Bounds{0.0f, 0.0f, 200.0f, 40.0f});

  PrimeStage::TextFieldSpec spec;
  spec.bounds = PrimeStage::Bounds{8.0f, 8.0f, 160.0f, 24.0f};
  spec.placeholder = "Search...";
  PrimeStage::UiNode field = root.createTextField(spec);
  CHECK(frame.getNode(field.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage status bar creates a node") {
  PrimeFrame::Frame frame;
  PrimeStage::applyStudioTheme(frame);
  PrimeStage::UiNode root = PrimeStage::createRoot(frame, PrimeStage::Bounds{0.0f, 0.0f, 200.0f, 30.0f});

  PrimeStage::StatusBarSpec spec;
  spec.bounds = PrimeStage::Bounds{0.0f, 6.0f, 200.0f, 24.0f};
  spec.leftText = "Ready";
  spec.rightText = "Demo";
  PrimeStage::UiNode bar = root.createStatusBar(spec);
  CHECK(frame.getNode(bar.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage property list creates a node") {
  PrimeFrame::Frame frame;
  PrimeStage::applyStudioTheme(frame);
  PrimeStage::UiNode root = PrimeStage::createRoot(frame, PrimeStage::Bounds{0.0f, 0.0f, 200.0f, 80.0f});

  PrimeStage::PropertyListSpec spec;
  spec.size.preferredWidth = 180.0f;
  spec.rows = {
      {"Key", "Value"},
      {"Key2", "Value2"}
  };
  PrimeStage::UiNode list = root.createPropertyList(spec);
  CHECK(frame.getNode(list.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage card grid creates a node") {
  PrimeFrame::Frame frame;
  PrimeStage::applyStudioTheme(frame);
  PrimeStage::UiNode root = PrimeStage::createRoot(frame, PrimeStage::Bounds{0.0f, 0.0f, 300.0f, 200.0f});

  PrimeStage::CardGridSpec spec;
  spec.size.preferredWidth = 280.0f;
  spec.size.preferredHeight = 120.0f;
  spec.cards = {{"Card", "Detail"}, {"Card", "Detail"}};
  PrimeStage::UiNode grid = root.createCardGrid(spec);
  CHECK(frame.getNode(grid.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage progress bar creates a node") {
  PrimeFrame::Frame frame;
  PrimeStage::applyStudioTheme(frame);
  PrimeStage::UiNode root = PrimeStage::createRoot(frame, PrimeStage::Bounds{0.0f, 0.0f, 120.0f, 24.0f});

  PrimeStage::ProgressBarSpec spec;
  spec.size.preferredWidth = 120.0f;
  spec.size.preferredHeight = 10.0f;
  spec.value = 0.5f;
  PrimeStage::UiNode bar = root.createProgressBar(spec);
  CHECK(frame.getNode(bar.nodeId()) != nullptr);
}
