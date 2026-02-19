#include "PrimeStage/StudioUi.h"
#include "PrimeStage/Render.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

using namespace PrimeFrame;
using namespace PrimeStage;
using namespace PrimeStage::Studio;

int main(int argc, char** argv) {
  std::string outPath = argc > 1 ? argv[1] : "screenshots/primeframe_ui.png";
  std::filesystem::path outFile(outPath);
  if (outFile.has_parent_path()) {
    std::filesystem::create_directories(outFile.parent_path());
  }

  constexpr float kWidth = StudioDefaults::CanvasWidth;
  constexpr float kHeight = StudioDefaults::CanvasHeight;
  constexpr float opacityBarH = StudioDefaults::OpacityBarHeight;

  Frame frame;

  SizeSpec shellSize;
  shellSize.preferredWidth = kWidth;
  shellSize.preferredHeight = kHeight;
  ShellSpec shellSpec = makeShellSpec(shellSize);
  ShellLayout shell = createShell(frame, shellSpec);
  float shellWidth = shellSpec.size.preferredWidth.value_or(kWidth);
  float shellHeight = shellSpec.size.preferredHeight.value_or(kHeight);
  float sidebarW = shellSpec.sidebarWidth;
  float inspectorW = shellSpec.inspectorWidth;
  float contentW = std::max(0.0f, shellWidth - sidebarW - inspectorW);
  float contentH = std::max(0.0f, shellHeight - shellSpec.topbarHeight - shellSpec.statusHeight);
  UiNode edgeBar = shell.topbar;
  UiNode statusBar = shell.status;
  UiNode leftRail = shell.sidebar;
  UiNode centerPane = shell.content;
  UiNode rightRail = shell.inspector;

  auto create_topbar = [&]() {
    StackSpec rowSpec;
    rowSpec.size.preferredWidth = shellWidth;
    rowSpec.size.preferredHeight = StudioDefaults::EdgeBarHeight;
    rowSpec.padding = Insets{0.0f,
                             StudioDefaults::PanelInset,
                             0.0f,
                             StudioDefaults::PanelInset};
    rowSpec.gap = StudioDefaults::PanelInset;

    UiNode row = edgeBar.createHorizontalStack(rowSpec);

    auto fixed = [](float width, float height) {
      SizeSpec size;
      size.preferredWidth = width;
      size.preferredHeight = height;
      return size;
    };

    createTextLine(row,
                   "PrimeFrame Studio",
                   TextRole::TitleBright,
                   fixed(StudioDefaults::TitleBlockWidth, StudioDefaults::ControlHeight),
                   PrimeFrame::TextAlign::Center);
    row.createDivider(rectToken(RectRole::Divider),
                      fixed(StudioDefaults::DividerThickness, StudioDefaults::ControlHeight));
    row.createSpacer(fixed(StudioDefaults::PanelInset, StudioDefaults::ControlHeight));
    createTextField(row, "Search...", {});

    SizeSpec spacer;
    spacer.stretchX = 1.0f;
    row.createSpacer(spacer);

    createButton(row,
                 "Share",
                 ButtonVariant::Default,
                 {});
    createButton(row,
                 "Run",
                 ButtonVariant::Primary,
                 {});
  };

  auto create_sidebar = [&]() {
    StackSpec columnSpec;
    columnSpec.size.preferredWidth = sidebarW;
    columnSpec.size.preferredHeight = contentH;
    columnSpec.padding = Insets{StudioDefaults::PanelInset,
                                StudioDefaults::PanelInset,
                                StudioDefaults::PanelInset,
                                StudioDefaults::PanelInset};
    columnSpec.gap = StudioDefaults::PanelInset;
    UiNode column = leftRail.createVerticalStack(columnSpec);

    PanelSpec headerSpec;
    headerSpec.rectStyle = rectToken(RectRole::PanelStrong);
    headerSpec.layout = PrimeFrame::LayoutType::HorizontalStack;
    headerSpec.size.preferredHeight = StudioDefaults::HeaderHeight;
    UiNode header = column.createPanel(headerSpec);

    SizeSpec accentSize;
    accentSize.preferredWidth = StudioDefaults::AccentThickness;
    accentSize.preferredHeight = StudioDefaults::HeaderHeight;
    header.createPanel(rectToken(RectRole::Accent), accentSize);

    SizeSpec indentSize;
    indentSize.preferredWidth = StudioDefaults::LabelIndent;
    indentSize.preferredHeight = StudioDefaults::HeaderHeight;
    header.createSpacer(indentSize);

    SizeSpec headerTextSize;
    headerTextSize.stretchX = 1.0f;
    headerTextSize.preferredHeight = StudioDefaults::HeaderHeight;
    createTextLine(header, "Scene", TextRole::BodyBright, headerTextSize);

    SizeSpec hierarchyTextSize;
    hierarchyTextSize.preferredHeight = StudioDefaults::HeaderHeight;
    createTextLine(column, "Hierarchy", TextRole::SmallMuted, hierarchyTextSize);

    PanelSpec treePanelSpec;
    treePanelSpec.rectStyle = rectToken(RectRole::Panel);
    treePanelSpec.layout = PrimeFrame::LayoutType::VerticalStack;
    treePanelSpec.size.stretchX = 1.0f;
    treePanelSpec.size.stretchY = 1.0f;
    UiNode treePanel = column.createPanel(treePanelSpec);

    Studio::TreeViewSpec treeSpec;
    treeSpec.base.size.stretchX = 1.0f;
    treeSpec.base.size.stretchY = 1.0f;
    treeSpec.base.showHeaderDivider = false;
    treeSpec.base.rowStartX = 12.0f;
    treeSpec.base.rowStartY = 4.0f;
    treeSpec.base.headerDividerY = 0.0f;
    treeSpec.base.rowWidthInset = 28.0f;
    treeSpec.base.rowHeight = 24.0f;
    treeSpec.base.rowGap = 2.0f;
    treeSpec.base.indent = 14.0f;
    treeSpec.base.caretBaseX = 12.0f;
    treeSpec.base.caretSize = 12.0f;
    treeSpec.base.caretInset = 2.0f;
    treeSpec.base.caretThickness = 2.5f;
    treeSpec.base.caretMaskPad = 1.0f;
    treeSpec.base.connectorThickness = 2.0f;
    treeSpec.base.linkEndInset = 0.0f;
    treeSpec.base.selectionAccentWidth = 2.0f;
    treeSpec.rowRole = RectRole::PanelAlt;
    treeSpec.rowAltRole = RectRole::Panel;
    treeSpec.caretBackgroundRole = RectRole::PanelStrong;
    treeSpec.caretLineRole = RectRole::Accent;
    treeSpec.connectorRole = RectRole::Accent;
    treeSpec.textRole = TextRole::SmallBright;
    treeSpec.selectedTextRole = TextRole::SmallBright;
    float treePanelHeight = contentH - StudioDefaults::HeaderHeight * 2.0f -
                            StudioDefaults::PanelInset * 4.0f;
    treeSpec.base.size.preferredWidth = sidebarW - StudioDefaults::PanelInset * 2.0f;
    treeSpec.base.size.preferredHeight = std::max(0.0f, treePanelHeight);

    Studio::TreeNode treeRoot{
        "Root",
        {
            Studio::TreeNode{"World",
                     {Studio::TreeNode{"Camera"}, Studio::TreeNode{"Lights"}, Studio::TreeNode{"Environment"}},
                     true,
                     false},
            Studio::TreeNode{"UI",
                     {
                         Studio::TreeNode{"Sidebar"},
                         Studio::TreeNode{"Toolbar", {Studio::TreeNode{"Buttons"}}, false, false},
                         Studio::TreeNode{"Panels",
                                  {Studio::TreeNode{"TreeView", {}, true, true}, Studio::TreeNode{"Rows"}},
                                  true,
                                  false}
                     },
                     true,
                     false}
        },
        true,
        false};

    treeSpec.base.nodes = {treeRoot};
    createTreeView(treePanel, treeSpec);
  };

  auto create_content = [&]() {
    StackSpec columnSpec;
    float scrollGutterX = StudioDefaults::TableRightInset;
    float scrollGutterY = StudioDefaults::PanelInset;
    float columnWidth = std::max(0.0f, contentW - scrollGutterX);
    float columnHeight = std::max(0.0f, contentH - scrollGutterY);
    columnSpec.size.preferredWidth = columnWidth;
    columnSpec.size.preferredHeight = columnHeight;
    columnSpec.padding = Insets{StudioDefaults::SurfaceInset,
                                StudioDefaults::SectionHeaderOffsetY,
                                StudioDefaults::SurfaceInset,
                                StudioDefaults::SurfaceInset};
    columnSpec.gap = StudioDefaults::SectionGap;
    UiNode column = centerPane.createVerticalStack(columnSpec);

    float sectionWidth = columnWidth - StudioDefaults::SurfaceInset * 2.0f;
    SizeSpec overviewSize;
    overviewSize.preferredWidth = sectionWidth;
    overviewSize.preferredHeight = StudioDefaults::SectionHeaderHeight;
    createSectionHeader(column, overviewSize, "Overview", TextRole::TitleBright);

    PanelSpec boardSpec;
    boardSpec.rectStyle = rectToken(RectRole::Panel);
    boardSpec.layout = PrimeFrame::LayoutType::VerticalStack;
    boardSpec.padding = Insets{StudioDefaults::SurfaceInset,
                               StudioDefaults::PanelInset,
                               StudioDefaults::SurfaceInset,
                               StudioDefaults::PanelInset};
    boardSpec.gap = StudioDefaults::PanelInset;
    boardSpec.size.preferredWidth = sectionWidth;
    boardSpec.size.preferredHeight = StudioDefaults::PanelHeightL +
                                     StudioDefaults::ControlHeight +
                                     StudioDefaults::PanelInset;
    UiNode boardPanel = column.createPanel(boardSpec);

    float boardTextWidth = std::max(0.0f, sectionWidth - StudioDefaults::SurfaceInset * 2.0f);
    SizeSpec titleSize;
    titleSize.preferredWidth = boardTextWidth;
    titleSize.preferredHeight = StudioDefaults::TitleHeight;
    createTextLine(boardPanel, "Active Board", TextRole::SmallMuted, titleSize);

    SizeSpec paragraphSize;
    paragraphSize.preferredWidth = boardTextWidth;
    createParagraph(boardPanel,
                    "Lorem ipsum dolor sit amet, consectetur adipiscing elit.\n"
                    "Sed do eiusmod tempor incididunt ut labore et dolore.\n"
                    "Ut enim ad minim veniam, quis nostrud exercitation.",
                    TextRole::SmallMuted,
                    paragraphSize);

    StackSpec buttonRowSpec;
    buttonRowSpec.size.preferredWidth = boardTextWidth;
    buttonRowSpec.size.preferredHeight = StudioDefaults::ControlHeight;
    UiNode boardButtons = boardPanel.createHorizontalStack(buttonRowSpec);
    SizeSpec buttonSpacer;
    buttonSpacer.stretchX = 1.0f;
    boardButtons.createSpacer(buttonSpacer);
    createButton(boardButtons, "Primary Action", ButtonVariant::Primary, {});

    SizeSpec highlightSize;
    highlightSize.preferredWidth = sectionWidth;
    highlightSize.preferredHeight = StudioDefaults::HeaderHeight;
    createSectionHeader(column,
                        highlightSize,
                        "Highlights",
                        TextRole::SmallBright,
                        true,
                        StudioDefaults::HeaderDividerOffset);

    CardGridSpec cardSpec;
    cardSpec.size.preferredWidth = sectionWidth;
    cardSpec.size.preferredHeight = StudioDefaults::CardHeight;
    cardSpec.gapX = StudioDefaults::PanelInset;
    cardSpec.cardWidth = (sectionWidth - cardSpec.gapX * 2.0f) / 3.0f;
    cardSpec.cards = {
        {"Card", "Detail"},
        {"Card", "Detail"},
        {"Card", "Detail"}
    };
    createCardGrid(column, cardSpec);

    float tableWidth = sectionWidth;
    float firstColWidth = tableWidth - StudioDefaults::TableStatusOffset;
    float secondColWidth = tableWidth - firstColWidth;

    Studio::TableSpec tableSpec;
    tableSpec.base.size.preferredWidth = tableWidth;
    tableSpec.base.size.stretchY = 1.0f;
    tableSpec.base.showHeaderDividers = false;
    tableSpec.columns = {
        Studio::TableColumn{"Item", firstColWidth, TextRole::SmallBright, TextRole::SmallBright},
        Studio::TableColumn{"Status", secondColWidth, TextRole::SmallBright, TextRole::SmallMuted}
    };
    tableSpec.base.rows = {
        {"Item Row", "Ready"},
        {"Item Row", "Ready"},
        {"Item Row", "Ready"},
        {"Item Row", "Ready"},
        {"Item Row", "Ready"},
        {"Item Row", "Ready"}
    };
    createTable(column, tableSpec);

    SizeSpec tableSpacer;
    tableSpacer.preferredHeight = StudioDefaults::PanelInset;
    column.createSpacer(tableSpacer);

    StackSpec overlaySpec;
    overlaySpec.size.preferredWidth = columnWidth + scrollGutterX;
    overlaySpec.size.preferredHeight = columnHeight + scrollGutterY;
    overlaySpec.clipChildren = false;
    UiNode scrollOverlay = centerPane.createOverlay(overlaySpec);

    StackSpec vertSpec;
    vertSpec.size.preferredWidth = columnWidth + scrollGutterX;
    vertSpec.size.preferredHeight = columnHeight;
    vertSpec.gap = 0.0f;
    UiNode vertRow = scrollOverlay.createHorizontalStack(vertSpec);

    SizeSpec vertSpacer;
    vertSpacer.preferredWidth = columnWidth;
    vertSpacer.preferredHeight = columnHeight;
    vertRow.createSpacer(vertSpacer);

    ScrollHintsSpec scrollVertical;
    scrollVertical.size.preferredWidth = scrollGutterX;
    scrollVertical.size.preferredHeight = columnHeight;
    scrollVertical.showHorizontal = false;
    createScrollHints(vertRow, scrollVertical);

    StackSpec horizSpec;
    horizSpec.size.preferredWidth = columnWidth;
    horizSpec.size.preferredHeight = columnHeight + scrollGutterY;
    horizSpec.gap = 0.0f;
    UiNode horizCol = scrollOverlay.createVerticalStack(horizSpec);

    SizeSpec horizSpacer;
    horizSpacer.preferredWidth = columnWidth;
    horizSpacer.preferredHeight = columnHeight;
    horizCol.createSpacer(horizSpacer);

    ScrollHintsSpec scrollHorizontal;
    scrollHorizontal.size.preferredWidth = columnWidth;
    scrollHorizontal.size.preferredHeight = scrollGutterY;
    scrollHorizontal.showVertical = false;
    createScrollHints(horizCol, scrollHorizontal);
  };

  auto create_inspector = [&]() {
    StackSpec columnSpec;
    columnSpec.size.preferredWidth = inspectorW;
    columnSpec.size.preferredHeight = contentH;
    columnSpec.padding = Insets{StudioDefaults::SurfaceInset,
                                StudioDefaults::SurfaceInset,
                                StudioDefaults::SurfaceInset,
                                StudioDefaults::SurfaceInset};
    columnSpec.gap = StudioDefaults::PanelGap;
    UiNode column = rightRail.createVerticalStack(columnSpec);

    SizeSpec headerSpacer;
    headerSpacer.preferredHeight = StudioDefaults::SectionHeaderOffsetY;
    column.createSpacer(headerSpacer);

    float sectionWidth = inspectorW - StudioDefaults::SurfaceInset * 2.0f;
    SizeSpec inspectorHeaderSize;
    inspectorHeaderSize.preferredWidth = sectionWidth;
    inspectorHeaderSize.preferredHeight = StudioDefaults::SectionHeaderHeight;
    createSectionHeader(column,
                        inspectorHeaderSize,
                        "Inspector",
                        TextRole::BodyBright);

    SizeSpec propsSize;
    propsSize.preferredWidth = sectionWidth;
    propsSize.preferredHeight = StudioDefaults::PanelHeightS;
    SectionPanelSpec propsSpec;
    propsSpec.size = propsSize;
    propsSpec.title = "Properties";
    SectionPanel propsPanel = createSectionPanel(column, propsSpec);
    float propsContentW = std::max(0.0f,
                                   propsSize.preferredWidth.value_or(0.0f) -
                                       propsSpec.contentInsetX -
                                       propsSpec.contentInsetRight);

    SizeSpec transformSize;
    transformSize.preferredWidth = sectionWidth;
    transformSize.preferredHeight = StudioDefaults::PanelHeightM + opacityBarH;
    SectionPanelSpec transformSpec;
    transformSpec.size = transformSize;
    transformSpec.title = "Transform";
    SectionPanel transformPanel = createSectionPanel(column, transformSpec);
    float transformContentW = std::max(0.0f,
                                       transformSize.preferredWidth.value_or(0.0f) -
                                           transformSpec.contentInsetX -
                                           transformSpec.contentInsetRight);
    float transformContentH = std::max(0.0f,
                                       transformSize.preferredHeight.value_or(0.0f) -
                                           (transformSpec.headerInsetY +
                                            transformSpec.headerHeight +
                                            transformSpec.contentInsetY +
                                            transformSpec.contentInsetBottom));

    SizeSpec propsListSize;
    propsListSize.preferredWidth = propsContentW;
    createPropertyList(propsPanel.content,
                       propsListSize,
                       {{"Name", "SceneRoot"}, {"Tag", "Environment"}});

    StackSpec transformStackSpec;
    transformStackSpec.size.preferredWidth = transformContentW;
    transformStackSpec.size.preferredHeight = transformContentH;
    transformStackSpec.gap = StudioDefaults::PanelInset;
    UiNode transformStack = transformPanel.content.createVerticalStack(transformStackSpec);

    SizeSpec transformListSize;
    transformListSize.preferredWidth = transformContentW;
    createPropertyList(transformStack,
                       transformListSize,
                       {{"Position", "0, 0, 0"}, {"Scale", "1, 1, 1"}});

    StackSpec opacityOverlaySpec;
    opacityOverlaySpec.size.preferredWidth = transformContentW;
    opacityOverlaySpec.size.preferredHeight = opacityBarH;
    UiNode opacityOverlay = transformStack.createOverlay(opacityOverlaySpec);

    SizeSpec opacityBarSize;
    opacityBarSize.preferredWidth = transformContentW;
    opacityBarSize.preferredHeight = opacityBarH;
    createProgressBar(opacityOverlay, opacityBarSize, 0.85f);

    PropertyListSpec opacitySpec;
    opacitySpec.size.preferredWidth = transformContentW;
    opacitySpec.size.preferredHeight = opacityBarH;
    opacitySpec.rowHeight = opacityBarH;
    opacitySpec.rowGap = 0.0f;
    opacitySpec.labelRole = TextRole::SmallBright;
    opacitySpec.valueRole = TextRole::SmallBright;
    opacitySpec.rows = {{"Opacity", "85%"}};
    createPropertyList(opacityOverlay, opacitySpec);

    SizeSpec footerSpacer;
    footerSpacer.stretchY = 1.0f;
    column.createSpacer(footerSpacer);

    SizeSpec publishSize;
    publishSize.preferredWidth = sectionWidth;
    publishSize.stretchX = 1.0f;
    createButton(column,
                 "Publish",
                 ButtonVariant::Primary,
                 publishSize);

  };

  auto create_status = [&]() {
    StackSpec rowSpec;
    rowSpec.size.preferredWidth = shellWidth;
    rowSpec.size.preferredHeight = StudioDefaults::StatusHeight;
    rowSpec.padding = Insets{StudioDefaults::SurfaceInset, 0.0f, StudioDefaults::SurfaceInset, 0.0f};
    rowSpec.gap = StudioDefaults::PanelInset;
    UiNode bar = statusBar.createHorizontalStack(rowSpec);

    SizeSpec lineSize;
    lineSize.preferredHeight = StudioDefaults::StatusHeight;
    createTextLine(bar, "Ready", TextRole::SmallMuted, lineSize);

    SizeSpec barSpacer;
    barSpacer.stretchX = 1.0f;
    bar.createSpacer(barSpacer);

    createTextLine(bar, "PrimeFrame Demo", TextRole::SmallMuted, lineSize);
  };

  create_topbar();
  create_sidebar();
  create_content();
  create_inspector();
  create_status();

  return renderFrameToPng(frame, outPath) ? 0 : 1;
}
