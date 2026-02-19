#include "PrimeStage/Ui.h"
#include "PrimeStage/Render.h"
#include "PrimeStage/StudioUi.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

using namespace PrimeFrame;
using namespace PrimeStage;

int main(int argc, char** argv) {
  std::string outPath = argc > 1 ? argv[1] : "screenshots/primestage_widgets.png";
  std::filesystem::path outFile(outPath);
  if (outFile.has_parent_path()) {
    std::filesystem::create_directories(outFile.parent_path());
  }

  Frame frame;
  Studio::applyStudioTheme(frame);

  SizeSpec rootSize;
  rootSize.preferredWidth = 900.0f;
  rootSize.preferredHeight = 600.0f;

  UiNode root = Studio::createRoot(frame, rootSize);

  PanelSpec background;
  background.size = rootSize;
  background.rectStyle = Studio::rectToken(Studio::RectRole::Background);
  background.layout = LayoutType::Overlay;
  UiNode canvas = root.createPanel(background);

  StackSpec columnSpec;
  columnSpec.size.stretchX = 1.0f;
  columnSpec.size.stretchY = 1.0f;
  columnSpec.padding = Insets{24.0f, 24.0f, 24.0f, 24.0f};
  columnSpec.gap = 18.0f;
  UiNode column = canvas.createVerticalStack(columnSpec);

  LabelSpec title;
  title.text = "PrimeStage Base Widgets";
  title.textStyle = Studio::textToken(Studio::TextRole::TitleBright);
  title.size.stretchX = 1.0f;
  title.size.preferredHeight = 32.0f;
  column.createLabel(title);

  StackSpec rowSpec;
  rowSpec.size.preferredHeight = 36.0f;
  rowSpec.gap = 12.0f;
  UiNode row = column.createHorizontalStack(rowSpec);

  ButtonSpec primary;
  primary.label = "Primary";
  primary.backgroundStyle = Studio::rectToken(Studio::RectRole::Accent);
  primary.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  primary.size.preferredWidth = 120.0f;
  primary.size.preferredHeight = 32.0f;
  row.createButton(primary);

  ButtonSpec secondary;
  secondary.label = "Secondary";
  secondary.backgroundStyle = Studio::rectToken(Studio::RectRole::Panel);
  secondary.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  secondary.size.preferredWidth = 120.0f;
  secondary.size.preferredHeight = 32.0f;
  row.createButton(secondary);

  TextFieldSpec field;
  field.placeholder = "Search...";
  field.backgroundStyle = Studio::rectToken(Studio::RectRole::Panel);
  field.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  field.placeholderStyle = Studio::textToken(Studio::TextRole::BodyMuted);
  field.size.preferredWidth = 240.0f;
  field.size.preferredHeight = 32.0f;
  row.createTextField(field);

  StackSpec toggleRow;
  toggleRow.size.preferredHeight = 32.0f;
  toggleRow.gap = 20.0f;
  UiNode toggles = column.createHorizontalStack(toggleRow);

  ToggleSpec toggle;
  toggle.on = true;
  toggle.trackStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  toggle.knobStyle = Studio::rectToken(Studio::RectRole::Accent);
  toggle.size.preferredWidth = 44.0f;
  toggle.size.preferredHeight = 22.0f;
  toggles.createToggle(toggle);

  CheckboxSpec checkbox;
  checkbox.label = "Enable sync";
  checkbox.checked = true;
  checkbox.boxStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  checkbox.checkStyle = Studio::rectToken(Studio::RectRole::Accent);
  checkbox.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  toggles.createCheckbox(checkbox);

  SliderSpec slider;
  slider.value = 0.65f;
  slider.trackStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  slider.fillStyle = Studio::rectToken(Studio::RectRole::Accent);
  slider.thumbStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  slider.size.preferredWidth = 220.0f;
  slider.size.preferredHeight = 24.0f;
  toggles.createSlider(slider);

  TabsSpec tabs;
  tabs.labels = {"Overview", "Details", "Stats"};
  tabs.selectedIndex = 1;
  tabs.tabStyle = Studio::rectToken(Studio::RectRole::Panel);
  tabs.activeTabStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  tabs.textStyle = Studio::textToken(Studio::TextRole::SmallMuted);
  tabs.activeTextStyle = Studio::textToken(Studio::TextRole::SmallBright);
  UiNode tabsNode = column.createTabs(tabs);
  (void)tabsNode;

  StackSpec dropdownRow;
  dropdownRow.size.preferredHeight = 32.0f;
  dropdownRow.gap = 16.0f;
  UiNode dropdowns = column.createHorizontalStack(dropdownRow);

  DropdownSpec dropdown;
  dropdown.label = "Quality";
  dropdown.backgroundStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  dropdown.textStyle = Studio::textToken(Studio::TextRole::BodyBright);
  dropdown.indicatorStyle = Studio::textToken(Studio::TextRole::BodyMuted);
  dropdown.size.preferredWidth = 200.0f;
  dropdown.size.preferredHeight = 32.0f;
  dropdowns.createDropdown(dropdown);

  ProgressBarSpec progress;
  progress.value = 0.42f;
  progress.trackStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  progress.fillStyle = Studio::rectToken(Studio::RectRole::Accent);
  progress.size.preferredWidth = 240.0f;
  progress.size.preferredHeight = 12.0f;
  dropdowns.createProgressBar(progress);

  ScrollViewSpec scrollSpec;
  scrollSpec.size.preferredWidth = 300.0f;
  scrollSpec.size.preferredHeight = 140.0f;
  scrollSpec.vertical.trackStyle = Studio::rectToken(Studio::RectRole::ScrollTrack);
  scrollSpec.vertical.thumbStyle = Studio::rectToken(Studio::RectRole::ScrollThumb);
  scrollSpec.horizontal.trackStyle = Studio::rectToken(Studio::RectRole::ScrollTrack);
  scrollSpec.horizontal.thumbStyle = Studio::rectToken(Studio::RectRole::ScrollThumb);
  scrollSpec.vertical.thumbLength = 40.0f;
  scrollSpec.horizontal.thumbLength = 60.0f;
  ScrollView scroll = column.createScrollView(scrollSpec);

  StackSpec scrollContentSpec;
  scrollContentSpec.size.preferredWidth = 420.0f;
  scrollContentSpec.size.preferredHeight = 220.0f;
  scrollContentSpec.padding = Insets{12.0f, 12.0f, 12.0f, 12.0f};
  scrollContentSpec.gap = 8.0f;
  UiNode scrollContent = scroll.content.createVerticalStack(scrollContentSpec);

  for (int i = 0; i < 6; ++i) {
    LabelSpec item;
    item.text = "Scrollable item";
    item.textStyle = Studio::textToken(Studio::TextRole::SmallMuted);
    item.size.preferredHeight = 18.0f;
    scrollContent.createLabel(item);
  }

  StackSpec dataRow;
  dataRow.size.stretchX = 1.0f;
  dataRow.size.preferredHeight = 180.0f;
  dataRow.gap = 16.0f;
  UiNode dataRowNode = column.createHorizontalStack(dataRow);

  TreeViewSpec treeSpec;
  treeSpec.size.preferredWidth = 260.0f;
  treeSpec.size.preferredHeight = 180.0f;
  treeSpec.rowStartX = 10.0f;
  treeSpec.rowStartY = 6.0f;
  treeSpec.rowWidthInset = 18.0f;
  treeSpec.rowHeight = 22.0f;
  treeSpec.rowGap = 2.0f;
  treeSpec.indent = 14.0f;
  treeSpec.caretBaseX = 10.0f;
  treeSpec.caretSize = 10.0f;
  treeSpec.caretInset = 2.0f;
  treeSpec.caretThickness = 2.0f;
  treeSpec.caretMaskPad = 1.0f;
  treeSpec.connectorThickness = 2.0f;
  treeSpec.selectionAccentWidth = 3.0f;
  treeSpec.rowStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  treeSpec.rowAltStyle = Studio::rectToken(Studio::RectRole::Panel);
  treeSpec.selectionStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  treeSpec.selectionAccentStyle = Studio::rectToken(Studio::RectRole::Accent);
  treeSpec.caretBackgroundStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  treeSpec.caretLineStyle = Studio::rectToken(Studio::RectRole::Accent);
  treeSpec.connectorStyle = Studio::rectToken(Studio::RectRole::Accent);
  treeSpec.textStyle = Studio::textToken(Studio::TextRole::SmallBright);
  treeSpec.selectedTextStyle = Studio::textToken(Studio::TextRole::SmallBright);
  treeSpec.scrollBar.trackStyle = Studio::rectToken(Studio::RectRole::ScrollTrack);
  treeSpec.scrollBar.thumbStyle = Studio::rectToken(Studio::RectRole::ScrollThumb);

  TreeNode widgetsRoot{
      "Widgets",
      {
          TreeNode{"Controls",
                   {TreeNode{"Button"}, TreeNode{"Checkbox"}, TreeNode{"Slider"}},
                   true,
                   false},
          TreeNode{"Data",
                   {TreeNode{"Table"}, TreeNode{"TreeView"}},
                   true,
                   true},
          TreeNode{"Containers",
                   {TreeNode{"Panel"}, TreeNode{"ScrollView"}},
                   true,
                   false}
      },
      true,
      false};

  treeSpec.nodes = {widgetsRoot};
  dataRowNode.createTreeView(treeSpec);

  TableSpec tableSpec;
  tableSpec.size.stretchX = 1.0f;
  tableSpec.size.preferredHeight = 180.0f;
  tableSpec.headerHeight = 22.0f;
  tableSpec.rowHeight = 24.0f;
  tableSpec.headerPaddingX = 12.0f;
  tableSpec.cellPaddingX = 12.0f;
  tableSpec.headerStyle = Studio::rectToken(Studio::RectRole::PanelStrong);
  tableSpec.rowStyle = Studio::rectToken(Studio::RectRole::Panel);
  tableSpec.rowAltStyle = Studio::rectToken(Studio::RectRole::PanelAlt);
  tableSpec.dividerStyle = Studio::rectToken(Studio::RectRole::Divider);
  tableSpec.columns = {
      TableColumn{"Name", 0.0f, Studio::textToken(Studio::TextRole::SmallBright),
                  Studio::textToken(Studio::TextRole::SmallBright)},
      TableColumn{"State", 120.0f, Studio::textToken(Studio::TextRole::SmallMuted),
                  Studio::textToken(Studio::TextRole::SmallMuted)}
  };
  tableSpec.rows = {
      {"Button", "Active"},
      {"Text Field", "Idle"},
      {"Slider", "Active"},
      {"Checkbox", "Disabled"},
      {"Tree View", "Active"}
  };
  dataRowNode.createTable(tableSpec);

  RenderOptions options;
  options.clearColor = Rgba8{10, 12, 18, 255};
  renderFrameToPng(frame, outPath, options);

  return 0;
}
