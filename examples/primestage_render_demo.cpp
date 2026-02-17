#include "PrimeStage/PrimeStage.h"
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

  ShellSpec shellSpec = makeShellSpec(Bounds{0.0f, 0.0f, kWidth, kHeight});
  ShellLayout shell = createShell(frame, shellSpec);
  Bounds sidebarBounds = shell.sidebarBounds;
  Bounds contentBounds = shell.contentBounds;
  Bounds inspectorBounds = shell.inspectorBounds;
  Bounds sidebarLocal{0.0f, 0.0f, sidebarBounds.width, sidebarBounds.height};
  Bounds inspectorLocal{0.0f, 0.0f, inspectorBounds.width, inspectorBounds.height};
  float contentW = contentBounds.width;
  float contentH = contentBounds.height;
  float sidebarW = sidebarBounds.width;
  float inspectorW = inspectorBounds.width;
  float shellWidth = shell.bounds.width;
  UiNode root = shell.root;
  UiNode edgeBar = shell.topbar;
  UiNode leftRail = shell.sidebar;
  UiNode centerPane = shell.content;
  UiNode rightRail = shell.inspector;

  auto add_panel = [&](UiNode& parent, Bounds const& bounds, RectRole role) -> UiNode {
    return parent.createPanel(role, bounds);
  };

  auto add_divider = [&](UiNode& parent, Bounds const& bounds) -> UiNode {
    DividerSpec spec;
    spec.bounds = bounds;
    spec.rectStyle = rectToken(RectRole::Divider);
    return parent.createDivider(spec);
  };

  auto create_topbar = [&]() {
    RowLayout row(Bounds{0.0f, UiDefaults::PanelInset, shellWidth, UiDefaults::ControlHeight});
    Bounds titleBlock = row.takeLeft(UiDefaults::TitleBlockWidth);
    Bounds dividerBounds = row.takeLeft(UiDefaults::DividerThickness);
    Bounds searchBounds = row.takeLeft(UiDefaults::FieldWidthL);
    Bounds shareBounds = row.takeRight(UiDefaults::ControlWidthM);
    Bounds runBounds = row.takeRight(UiDefaults::ControlWidthM);

    add_divider(edgeBar, alignCenterY(dividerBounds, UiDefaults::ControlHeight));
    edgeBar.createTextField(searchBounds, "Search...");
    UiNode runNode = edgeBar.createButton(runBounds, "Run", ButtonVariant::Primary);
    UiNode shareNode = edgeBar.createButton(shareBounds, "Share");

    edgeBar.createTextLine(alignCenterY(titleBlock, UiDefaults::TitleHeight),
                              "PrimeFrame Studio",
                              TextRole::TitleBright,
                              PrimeFrame::TextAlign::Center);
  };

  auto create_sidebar = [&]() {
    float headerY = UiDefaults::PanelInset;
    float treeTop = headerY + UiDefaults::HeaderHeight + UiDefaults::PanelInset;
    float treeBottomInset = UiDefaults::HeaderHeight;
    Bounds treePanelBounds = insetBounds(sidebarLocal,
                                         UiDefaults::PanelInset,
                                         treeTop,
                                         UiDefaults::PanelInset,
                                         treeBottomInset);
    UiNode treePanel = add_panel(leftRail, treePanelBounds, RectRole::Panel);
    add_panel(leftRail,
              Bounds{UiDefaults::PanelInset,
                     headerY,
                     sidebarW - UiDefaults::PanelInset * 2.0f,
                     UiDefaults::HeaderHeight},
              RectRole::PanelStrong);
    add_panel(leftRail,
              Bounds{UiDefaults::PanelInset,
                     headerY,
                     UiDefaults::AccentThickness,
                     UiDefaults::HeaderHeight},
              RectRole::Accent);

    float labelX = UiDefaults::PanelInset + UiDefaults::LabelIndent;
    float labelW = std::max(0.0f, sidebarW - labelX - UiDefaults::PanelInset);
    leftRail.createTextLine(Bounds{labelX,
                                   headerY,
                                   labelW,
                                   UiDefaults::HeaderHeight},
                               "Scene",
                               TextRole::BodyBright);
    leftRail.createTextLine(Bounds{labelX,
                                   treeTop + UiDefaults::LabelGap,
                                   labelW,
                                   UiDefaults::HeaderHeight},
                               "Hierarchy",
                               TextRole::SmallMuted);

    TreeViewSpec treeSpec;
    treeSpec.bounds = Bounds{0.0f, 0.0f, treePanelBounds.width, treePanelBounds.height};
    treeSpec.showHeaderDivider = true;
    treeSpec.headerDividerY = UiDefaults::TreeHeaderDividerY;
    float treeTrackH = treePanelBounds.height - treeSpec.scrollBar.padding * 2.0f;
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
    float cursorY = UiDefaults::SectionHeaderOffsetY;
    Bounds overviewHeader{UiDefaults::SurfaceInset,
                          cursorY,
                          contentW - UiDefaults::SurfaceInset * 2.0f,
                          UiDefaults::SectionHeaderHeight};
    centerPane.createSectionHeader(overviewHeader,
                                    "Overview",
                                    TextRole::TitleBright);
    cursorY += UiDefaults::SectionHeaderHeight + UiDefaults::SectionGap;

    Bounds boardPanelBounds{UiDefaults::SurfaceInset,
                            cursorY,
                            contentW - UiDefaults::SurfaceInset * 2.0f,
                            UiDefaults::PanelHeightL};
    UiNode boardPanel = add_panel(centerPane, boardPanelBounds, RectRole::Panel);
    Bounds primaryBounds = alignBottomRight(boardPanelBounds,
                                            UiDefaults::ControlWidthL,
                                            UiDefaults::ControlHeight,
                                            UiDefaults::PanelInset,
                                            UiDefaults::PanelInset);
    UiNode primaryButton = centerPane.createButton(primaryBounds, "Primary Action", ButtonVariant::Primary);
    cursorY += UiDefaults::PanelHeightL;

    float highlightHeaderY = cursorY;
    float highlightHeaderH = UiDefaults::HeaderHeight;

    centerPane.createSectionHeader(Bounds{UiDefaults::SurfaceInset,
                                          highlightHeaderY,
                                          contentW - UiDefaults::SurfaceInset * 2.0f,
                                          highlightHeaderH},
                                    "Highlights",
                                    TextRole::SmallBright,
                                    true,
                                    UiDefaults::HeaderDividerOffset);
    cursorY += UiDefaults::HeaderHeight + UiDefaults::SectionGapSmall;


    Bounds cardBounds{UiDefaults::SurfaceInset,
                      cursorY,
                      contentW - UiDefaults::SurfaceInset,
                      UiDefaults::CardHeight};
    centerPane.createCardGrid(cardBounds,
                               {
                                   {"Card", "Detail"},
                                   {"Card", "Detail"},
                                   {"Card", "Detail"}
                               });
    cursorY += UiDefaults::CardHeight + UiDefaults::SectionGapLarge;

    float boardTextWidth = std::max(0.0f, primaryBounds.x - boardPanelBounds.x -
                                            UiDefaults::SurfaceInset - UiDefaults::PanelInset);
    boardPanel.createTextLine(Bounds{UiDefaults::SurfaceInset,
                                     UiDefaults::PanelInset,
                                     boardTextWidth,
                                     UiDefaults::TitleHeight},
                              "Active Board",
                              TextRole::SmallMuted);
    boardPanel.createParagraph(Bounds{UiDefaults::SurfaceInset,
                                      UiDefaults::BodyOffsetY,
                                      boardTextWidth,
                                      0.0f},
                               "Lorem ipsum dolor sit amet, consectetur adipiscing elit.\n"
                               "Sed do eiusmod tempor incididunt ut labore et dolore.\n"
                               "Ut enim ad minim veniam, quis nostrud exercitation.",
                               TextRole::SmallMuted);

    float listY = cursorY;
    float listHeaderY = listY - UiDefaults::TableHeaderOffset;
    float tableWidth = contentW - UiDefaults::SurfaceInset - UiDefaults::TableRightInset;
    float firstColWidth = contentW - UiDefaults::TableStatusOffset;
    float secondColWidth = tableWidth - firstColWidth;

    centerPane.createTable(Bounds{UiDefaults::SurfaceInset,
                                  listHeaderY - UiDefaults::TableHeaderPadY,
                                  tableWidth,
                                  0.0f},
                            {
                                TableColumn{"Item", firstColWidth, TextRole::SmallBright, TextRole::SmallBright},
                                TableColumn{"Status", secondColWidth, TextRole::SmallBright, TextRole::SmallMuted}
                            },
                            {
                                {"Item Row", "Ready"},
                                {"Item Row", "Ready"},
                                {"Item Row", "Ready"},
                                {"Item Row", "Ready"},
                                {"Item Row", "Ready"},
                                {"Item Row", "Ready"}
                            });

    centerPane.createScrollHints(Bounds{0.0f, 0.0f, contentW, contentH});
  };

  auto create_inspector = [&]() {
    float headerY = UiDefaults::SectionHeaderOffsetY;
    rightRail.createSectionHeader(Bounds{UiDefaults::SurfaceInset,
                                         headerY,
                                         inspectorW - UiDefaults::SurfaceInset * 2.0f,
                                         UiDefaults::SectionHeaderHeight},
                                      "Inspector",
                                      TextRole::BodyBright);

    float propsPanelY = headerY + UiDefaults::SectionHeaderHeight + UiDefaults::PanelGap;
    SectionPanel propsPanel =
        rightRail.createSectionPanel(Bounds{UiDefaults::SurfaceInset,
                                            propsPanelY,
                                            inspectorW - UiDefaults::SurfaceInset * 2.0f,
                                            UiDefaults::PanelHeightS},
                                                               "Properties");

    float transformPanelY = propsPanelY + UiDefaults::PanelHeightS + UiDefaults::PanelGapLarge;
    SectionPanel transformPanel =
        rightRail.createSectionPanel(Bounds{UiDefaults::SurfaceInset,
                                            transformPanelY,
                                            inspectorW - UiDefaults::SurfaceInset * 2.0f,
                                            UiDefaults::PanelHeightM},
                                                                   "Transform");

    float opacityRowY = transformPanel.contentBounds.y + UiDefaults::InlineRowOffset;
    float opacityBarX = transformPanel.contentBounds.x;
    transformPanel.panel.createProgressBar(
        Bounds{opacityBarX, opacityRowY, transformPanel.contentBounds.width, opacityBarH},
        0.85f);

    Bounds publishBounds = alignBottomRight(inspectorLocal,
                                            inspectorW - UiDefaults::SurfaceInset * 2.0f,
                                            UiDefaults::ControlHeight,
                                            UiDefaults::SurfaceInset,
                                            UiDefaults::FooterInset);
    UiNode publishButton = rightRail.createButton(publishBounds,
                                                      "Publish",
                                                      ButtonVariant::Primary);

    propsPanel.panel.createPropertyList(propsPanel.contentBounds,
                                        {{"Name", "SceneRoot"}, {"Tag", "Environment"}});

    transformPanel.panel.createPropertyList(transformPanel.contentBounds,
                                            {{"Position", "0, 0, 0"}, {"Scale", "1, 1, 1"}});

    Bounds opacityLabelBounds = transformPanel.contentBounds;
    opacityLabelBounds.y = opacityRowY;
    opacityLabelBounds.height = opacityBarH;
    transformPanel.panel.createPropertyRow(opacityLabelBounds, "Opacity", "85%");
  };

  auto create_status = [&]() {
    root.createStatusBar(shell.statusBounds, "Ready", "PrimeFrame Demo");
  };

  create_topbar();
  create_sidebar();
  create_content();
  create_inspector();
  create_status();

  return renderFrameToPng(frame, outPath) ? 0 : 1;
}
