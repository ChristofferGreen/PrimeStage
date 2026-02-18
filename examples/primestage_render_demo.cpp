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

  constexpr float kWidth = UiDefaults::CanvasWidth;
  constexpr float kHeight = UiDefaults::CanvasHeight;
  constexpr float opacityBarH = UiDefaults::OpacityBarHeight;

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
    rowSpec.size.preferredHeight = UiDefaults::EdgeBarHeight;
    rowSpec.padding = Insets{0.0f,
                             UiDefaults::PanelInset,
                             0.0f,
                             UiDefaults::PanelInset};
    rowSpec.gap = UiDefaults::PanelInset;

    UiNode row = edgeBar.createHorizontalStack(rowSpec);

    auto fixed = [](float width, float height) {
      SizeSpec size;
      size.preferredWidth = width;
      size.preferredHeight = height;
      return size;
    };

    row.createTextLine("PrimeFrame Studio",
                       TextRole::TitleBright,
                       fixed(UiDefaults::TitleBlockWidth, UiDefaults::ControlHeight),
                       PrimeFrame::TextAlign::Center);
    row.createDivider(rectToken(RectRole::Divider),
                      fixed(UiDefaults::DividerThickness, UiDefaults::ControlHeight));
    row.createSpacer(fixed(UiDefaults::PanelInset, UiDefaults::ControlHeight));
    row.createTextField("Search...", {});

    SizeSpec spacer;
    spacer.stretchX = 1.0f;
    row.createSpacer(spacer);

    row.createButton("Share",
                     ButtonVariant::Default,
                     {});
    row.createButton("Run",
                     ButtonVariant::Primary,
                     {});
  };

  auto create_sidebar = [&]() {
    StackSpec columnSpec;
    columnSpec.size.preferredWidth = sidebarW;
    columnSpec.size.preferredHeight = sidebarBounds.height;
    columnSpec.padding = Insets{UiDefaults::PanelInset,
                                UiDefaults::PanelInset,
                                UiDefaults::PanelInset,
                                UiDefaults::PanelInset};
    columnSpec.gap = UiDefaults::PanelInset;
    UiNode column = leftRail.createVerticalStack(columnSpec);

    PanelSpec headerSpec;
    headerSpec.rectStyle = rectToken(RectRole::PanelStrong);
    headerSpec.layout = PrimeFrame::LayoutType::HorizontalStack;
    headerSpec.size.preferredHeight = UiDefaults::HeaderHeight;
    UiNode header = column.createPanel(headerSpec);

    SizeSpec accentSize;
    accentSize.preferredWidth = UiDefaults::AccentThickness;
    accentSize.preferredHeight = UiDefaults::HeaderHeight;
    header.createPanel(rectToken(RectRole::Accent), accentSize);

    SizeSpec indentSize;
    indentSize.preferredWidth = UiDefaults::LabelIndent;
    indentSize.preferredHeight = UiDefaults::HeaderHeight;
    header.createSpacer(indentSize);

    SizeSpec headerTextSize;
    headerTextSize.stretchX = 1.0f;
    headerTextSize.preferredHeight = UiDefaults::HeaderHeight;
    header.createTextLine("Scene", TextRole::BodyBright, headerTextSize);

    SizeSpec hierarchyTextSize;
    hierarchyTextSize.preferredHeight = UiDefaults::HeaderHeight;
    column.createTextLine("Hierarchy", TextRole::SmallMuted, hierarchyTextSize);

    PanelSpec treePanelSpec;
    treePanelSpec.rectStyle = rectToken(RectRole::Panel);
    treePanelSpec.size.stretchX = 1.0f;
    treePanelSpec.size.stretchY = 1.0f;
    UiNode treePanel = column.createPanel(treePanelSpec);

    TreeViewSpec treeSpec;
    float treeWidth = std::max(0.0f, sidebarW - UiDefaults::PanelInset * 2.0f);
    float treeHeight = std::max(0.0f, sidebarBounds.height - UiDefaults::PanelInset * 2.0f -
                                        UiDefaults::HeaderHeight * 2.0f - UiDefaults::PanelInset);
    treeSpec.size.preferredWidth = treeWidth;
    treeSpec.size.preferredHeight = treeHeight;
    treeSpec.showHeaderDivider = true;
    treeSpec.headerDividerY = UiDefaults::TreeHeaderDividerY;
    float treeTrackH = treeHeight - treeSpec.scrollBar.padding * 2.0f;
    setScrollBarThumbPixels(treeSpec.scrollBar,
                            treeTrackH,
                            UiDefaults::ScrollThumbHeight,
                            UiDefaults::ScrollThumbOffset - treeSpec.scrollBar.padding);

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
    treePanel.createTreeView(treeSpec);
  };

  auto create_content = [&]() {
    StackSpec columnSpec;
    columnSpec.size.preferredWidth = contentW;
    columnSpec.size.preferredHeight = contentH;
    columnSpec.padding = Insets{UiDefaults::SurfaceInset,
                                UiDefaults::SectionHeaderOffsetY,
                                UiDefaults::SurfaceInset,
                                UiDefaults::SurfaceInset};
    columnSpec.gap = UiDefaults::SectionGap;
    UiNode column = centerPane.createVerticalStack(columnSpec);

    float sectionWidth = contentW - UiDefaults::SurfaceInset * 2.0f;
    SizeSpec overviewSize;
    overviewSize.preferredWidth = sectionWidth;
    overviewSize.preferredHeight = UiDefaults::SectionHeaderHeight;
    column.createSectionHeader(overviewSize, "Overview", TextRole::TitleBright);

    PanelSpec boardSpec;
    boardSpec.rectStyle = rectToken(RectRole::Panel);
    boardSpec.layout = PrimeFrame::LayoutType::VerticalStack;
    boardSpec.padding = Insets{UiDefaults::SurfaceInset,
                               UiDefaults::PanelInset,
                               UiDefaults::SurfaceInset,
                               UiDefaults::PanelInset};
    boardSpec.gap = UiDefaults::PanelInset;
    boardSpec.size.preferredWidth = sectionWidth;
    boardSpec.size.preferredHeight = UiDefaults::PanelHeightL +
                                     UiDefaults::ControlHeight +
                                     UiDefaults::PanelInset;
    UiNode boardPanel = column.createPanel(boardSpec);

    float boardTextWidth = std::max(0.0f, sectionWidth - UiDefaults::SurfaceInset * 2.0f);
    SizeSpec titleSize;
    titleSize.preferredWidth = boardTextWidth;
    titleSize.preferredHeight = UiDefaults::TitleHeight;
    boardPanel.createTextLine("Active Board", TextRole::SmallMuted, titleSize);

    SizeSpec paragraphSize;
    paragraphSize.preferredWidth = boardTextWidth;
    boardPanel.createParagraph("Lorem ipsum dolor sit amet, consectetur adipiscing elit.\n"
                               "Sed do eiusmod tempor incididunt ut labore et dolore.\n"
                               "Ut enim ad minim veniam, quis nostrud exercitation.",
                               TextRole::SmallMuted,
                               paragraphSize);

    StackSpec buttonRowSpec;
    buttonRowSpec.size.preferredWidth = boardTextWidth;
    buttonRowSpec.size.preferredHeight = UiDefaults::ControlHeight;
    UiNode boardButtons = boardPanel.createHorizontalStack(buttonRowSpec);
    SizeSpec buttonSpacer;
    buttonSpacer.stretchX = 1.0f;
    boardButtons.createSpacer(buttonSpacer);
    boardButtons.createButton("Primary Action", ButtonVariant::Primary, {});

    SizeSpec highlightSize;
    highlightSize.preferredWidth = sectionWidth;
    highlightSize.preferredHeight = UiDefaults::HeaderHeight;
    column.createSectionHeader(highlightSize,
                               "Highlights",
                               TextRole::SmallBright,
                               true,
                               UiDefaults::HeaderDividerOffset);

    CardGridSpec cardSpec;
    cardSpec.size.preferredWidth = sectionWidth;
    cardSpec.size.preferredHeight = UiDefaults::CardHeight;
    cardSpec.gapX = UiDefaults::PanelInset;
    cardSpec.cardWidth = (sectionWidth - cardSpec.gapX * 2.0f) / 3.0f;
    cardSpec.cards = {
        {"Card", "Detail"},
        {"Card", "Detail"},
        {"Card", "Detail"}
    };
    column.createCardGrid(cardSpec);

    float tableWidth = contentW - UiDefaults::SurfaceInset - UiDefaults::TableRightInset;
    float firstColWidth = contentW - UiDefaults::TableStatusOffset;
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
    column.createTable(tableSpec);

    ScrollHintsSpec scrollSpec;
    scrollSpec.size.preferredWidth = contentW;
    scrollSpec.size.preferredHeight = contentH;
    centerPane.createScrollHints(scrollSpec);
  };

  auto create_inspector = [&]() {
    StackSpec columnSpec;
    columnSpec.size.preferredWidth = inspectorW;
    columnSpec.size.preferredHeight = inspectorBounds.height;
    columnSpec.padding = Insets{UiDefaults::SurfaceInset,
                                UiDefaults::SurfaceInset,
                                UiDefaults::SurfaceInset,
                                UiDefaults::SurfaceInset};
    columnSpec.gap = UiDefaults::PanelGap;
    UiNode column = rightRail.createVerticalStack(columnSpec);

    SizeSpec headerSpacer;
    headerSpacer.preferredHeight = UiDefaults::SectionHeaderOffsetY;
    column.createSpacer(headerSpacer);

    float sectionWidth = inspectorW - UiDefaults::SurfaceInset * 2.0f;
    SizeSpec inspectorHeaderSize;
    inspectorHeaderSize.preferredWidth = sectionWidth;
    inspectorHeaderSize.preferredHeight = UiDefaults::SectionHeaderHeight;
    column.createSectionHeader(inspectorHeaderSize,
                               "Inspector",
                               TextRole::BodyBright);

    SizeSpec propsSize;
    propsSize.preferredWidth = sectionWidth;
    propsSize.preferredHeight = UiDefaults::PanelHeightS;
    SectionPanel propsPanel = column.createSectionPanel(propsSize, "Properties");

    SizeSpec transformSize;
    transformSize.preferredWidth = sectionWidth;
    transformSize.preferredHeight = UiDefaults::PanelHeightM;
    SectionPanel transformPanel = column.createSectionPanel(transformSize, "Transform");

    SizeSpec propsListSize;
    propsListSize.preferredWidth = propsPanel.contentBounds.width;
    propsPanel.content.createPropertyList(propsListSize,
                                          {{"Name", "SceneRoot"}, {"Tag", "Environment"}});

    StackSpec transformStackSpec;
    transformStackSpec.size.preferredWidth = transformPanel.contentBounds.width;
    transformStackSpec.size.preferredHeight = transformPanel.contentBounds.height;
    transformStackSpec.gap = UiDefaults::PanelInset;
    UiNode transformStack = transformPanel.content.createVerticalStack(transformStackSpec);

    SizeSpec transformListSize;
    transformListSize.preferredWidth = transformPanel.contentBounds.width;
    transformStack.createPropertyList(transformListSize,
                                      {{"Position", "0, 0, 0"}, {"Scale", "1, 1, 1"}});

    SizeSpec opacityRowSize;
    opacityRowSize.preferredWidth = transformPanel.contentBounds.width;
    opacityRowSize.preferredHeight = opacityBarH;
    transformStack.createPropertyRow(opacityRowSize, "Opacity", "85%");

    SizeSpec opacityBarSize;
    opacityBarSize.preferredWidth = transformPanel.contentBounds.width;
    opacityBarSize.preferredHeight = opacityBarH;
    transformStack.createProgressBar(opacityBarSize, 0.85f);

    SizeSpec footerSpacer;
    footerSpacer.stretchY = 1.0f;
    column.createSpacer(footerSpacer);

    SizeSpec publishSize;
    publishSize.stretchX = 1.0f;
    column.createButton("Publish",
                        ButtonVariant::Primary,
                        publishSize);

  };

  auto create_status = [&]() {
    StackSpec rowSpec;
    rowSpec.size.preferredWidth = shellWidth;
    rowSpec.size.preferredHeight = UiDefaults::StatusHeight;
    rowSpec.padding = Insets{UiDefaults::SurfaceInset, 0.0f, UiDefaults::SurfaceInset, 0.0f};
    rowSpec.gap = UiDefaults::PanelInset;
    UiNode bar = statusBar.createHorizontalStack(rowSpec);

    SizeSpec lineSize;
    lineSize.preferredHeight = UiDefaults::StatusHeight;
    bar.createTextLine("Ready", TextRole::SmallMuted, lineSize);

    SizeSpec barSpacer;
    barSpacer.stretchX = 1.0f;
    bar.createSpacer(barSpacer);

    bar.createTextLine("PrimeFrame Demo", TextRole::SmallMuted, lineSize);
  };

  create_topbar();
  create_sidebar();
  create_content();
  create_inspector();
  create_status();

  return renderFrameToPng(frame, outPath) ? 0 : 1;
}
