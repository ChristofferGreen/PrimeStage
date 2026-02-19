#include "PrimeStage/StudioUi.h"

#include "third_party/doctest.h"

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
  tableSpec.size.preferredWidth = 180.0f;
  tableSpec.columns = {
      Studio::TableColumn{"Item", 100.0f,
                              Studio::TextRole::SmallBright,
                              Studio::TextRole::SmallBright},
      Studio::TableColumn{"Status", 80.0f,
                              Studio::TextRole::SmallBright,
                              Studio::TextRole::SmallMuted}
  };
  tableSpec.rows = {
      {"Row", "Ready"},
      {"Row", "Ready"}
  };
  PrimeStage::UiNode table = Studio::createTable(root, tableSpec);
  CHECK(frame.getNode(table.nodeId()) != nullptr);
}

TEST_CASE("PrimeStage tree view creates a node") {
  PrimeFrame::Frame frame;
  Studio::applyStudioTheme(frame);
  PrimeStage::UiNode root = createRoot(frame, 200.0f, 120.0f);

  Studio::TreeViewSpec spec;
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 120.0f;
  spec.nodes = {Studio::TreeNode{"Root", {Studio::TreeNode{"Child"}}, true, false}};
  PrimeStage::UiNode tree = Studio::createTreeView(root, spec);
  CHECK(frame.getNode(tree.nodeId()) != nullptr);
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
