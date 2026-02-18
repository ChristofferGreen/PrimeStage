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
  Bounds sidebarBounds = shell.sidebarBounds;
  Bounds contentBounds = shell.contentBounds;
  Bounds inspectorBounds = shell.inspectorBounds;
  float contentW = contentBounds.width;
  float contentH = contentBounds.height;
  float sidebarW = sidebarBounds.width;
  float inspectorW = inspectorBounds.width;
  float shellWidth = shell.bounds.width;
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
    columnSpec.size.preferredHeight = sidebarBounds.height;
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
    treePanelSpec.size.stretchX = 1.0f;
    treePanelSpec.size.stretchY = 1.0f;
    UiNode treePanel = column.createPanel(treePanelSpec);

    TreeViewSpec treeSpec;
    float treeWidth = std::max(0.0f, sidebarW - StudioDefaults::PanelInset * 2.0f);
    float treeHeight = std::max(0.0f, sidebarBounds.height - StudioDefaults::PanelInset * 2.0f -
                                        StudioDefaults::HeaderHeight * 2.0f - StudioDefaults::PanelInset);
    treeSpec.size.preferredWidth = treeWidth;
    treeSpec.size.preferredHeight = treeHeight;
    treeSpec.showHeaderDivider = true;
    treeSpec.headerDividerY = StudioDefaults::TreeHeaderDividerY;
    float treeTrackH = treeHeight - treeSpec.scrollBar.padding * 2.0f;
    setScrollBarThumbPixels(treeSpec.scrollBar,
                            treeTrackH,
                            StudioDefaults::ScrollThumbHeight,
                            StudioDefaults::ScrollThumbOffset - treeSpec.scrollBar.padding);

    TreeNode treeRoot{
        "Root",
        {
            TreeNode{"World",
                     {TreeNode{"Camera"}, TreeNode{"Lights"}, TreeNode{"Environment"}},
                     true,
                     false},
            TreeNode{"UI",
                     {
                         TreeNode{"Sidebar"},
                         TreeNode{"Toolbar", {TreeNode{"Buttons"}}, false, false},
                         TreeNode{"Panels",
                                  {TreeNode{"TreeView", {}, true, true}, TreeNode{"Rows"}},
                                  true,
                                  false}
                     },
                     true,
                     false}
        },
        true,
        false};

    treeSpec.nodes = {treeRoot};
    createTreeView(treePanel, treeSpec);
  };

  auto create_content = [&]() {
    StackSpec columnSpec;
    columnSpec.size.preferredWidth = contentW;
    columnSpec.size.preferredHeight = contentH;
    columnSpec.padding = Insets{StudioDefaults::SurfaceInset,
                                StudioDefaults::SectionHeaderOffsetY,
                                StudioDefaults::SurfaceInset,
                                StudioDefaults::SurfaceInset};
    columnSpec.gap = StudioDefaults::SectionGap;
    UiNode column = centerPane.createVerticalStack(columnSpec);

    float sectionWidth = contentW - StudioDefaults::SurfaceInset * 2.0f;
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

    float tableWidth = contentW - StudioDefaults::SurfaceInset - StudioDefaults::TableRightInset;
    float firstColWidth = contentW - StudioDefaults::TableStatusOffset;
    float secondColWidth = tableWidth - firstColWidth;

    TableSpec tableSpec;
    tableSpec.size.preferredWidth = tableWidth;
    tableSpec.size.stretchY = 1.0f;
    tableSpec.showHeaderDividers = false;
    tableSpec.columns = {
        TableColumn{"Item", firstColWidth, TextRole::SmallBright, TextRole::SmallBright},
        TableColumn{"Status", secondColWidth, TextRole::SmallBright, TextRole::SmallMuted}
    };
    tableSpec.rows = {
        {"Item Row", "Ready"},
        {"Item Row", "Ready"},
        {"Item Row", "Ready"},
        {"Item Row", "Ready"},
        {"Item Row", "Ready"},
        {"Item Row", "Ready"}
    };
    createTable(column, tableSpec);

    ScrollHintsSpec scrollSpec;
    scrollSpec.size.preferredWidth = contentW;
    scrollSpec.size.preferredHeight = contentH;
    createScrollHints(centerPane, scrollSpec);
  };

  auto create_inspector = [&]() {
    StackSpec columnSpec;
    columnSpec.size.preferredWidth = inspectorW;
    columnSpec.size.preferredHeight = inspectorBounds.height;
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
    SectionPanel propsPanel = createSectionPanel(column, propsSize, "Properties");

    SizeSpec transformSize;
    transformSize.preferredWidth = sectionWidth;
    transformSize.preferredHeight = StudioDefaults::PanelHeightM + opacityBarH;
    SectionPanel transformPanel = createSectionPanel(column, transformSize, "Transform");

    SizeSpec propsListSize;
    propsListSize.preferredWidth = propsPanel.contentBounds.width;
    createPropertyList(propsPanel.content,
                       propsListSize,
                       {{"Name", "SceneRoot"}, {"Tag", "Environment"}});

    StackSpec transformStackSpec;
    transformStackSpec.size.preferredWidth = transformPanel.contentBounds.width;
    transformStackSpec.size.preferredHeight = transformPanel.contentBounds.height;
    transformStackSpec.gap = StudioDefaults::PanelInset;
    UiNode transformStack = transformPanel.content.createVerticalStack(transformStackSpec);

    SizeSpec transformListSize;
    transformListSize.preferredWidth = transformPanel.contentBounds.width;
    createPropertyList(transformStack,
                       transformListSize,
                       {{"Position", "0, 0, 0"}, {"Scale", "1, 1, 1"}});

    StackSpec opacityOverlaySpec;
    opacityOverlaySpec.size.preferredWidth = transformPanel.contentBounds.width;
    opacityOverlaySpec.size.preferredHeight = opacityBarH;
    UiNode opacityOverlay = transformStack.createOverlay(opacityOverlaySpec);

    SizeSpec opacityBarSize;
    opacityBarSize.preferredWidth = transformPanel.contentBounds.width;
    opacityBarSize.preferredHeight = opacityBarH;
    createProgressBar(opacityOverlay, opacityBarSize, 0.85f);

    PropertyListSpec opacitySpec;
    opacitySpec.size.preferredWidth = transformPanel.contentBounds.width;
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
