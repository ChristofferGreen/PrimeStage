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

namespace {
struct StudioMetrics {
  static constexpr float ShellWidth = 1100.0f;
  static constexpr float ShellHeight = 700.0f;
  static constexpr float TopbarHeight = 56.0f;
  static constexpr float StatusHeight = 24.0f;
  static constexpr float SidebarWidth = 240.0f;
  static constexpr float InspectorWidth = 260.0f;

  static constexpr float TopbarDividerX = 232.0f;
  static constexpr float TopbarDividerW = 1.0f;
  static constexpr float TopbarPaddingY = 12.0f;
  static constexpr float TopbarButtonW = 88.0f;
  static constexpr float TopbarButtonH = 32.0f;
  static constexpr float TopbarRunRight = 220.0f;
  static constexpr float TopbarShareRight = 120.0f;
  static constexpr float TopbarSearchW = 360.0f;
  static constexpr float TopbarSearchH = 32.0f;
  static constexpr float TopbarTitleY = 14.0f;
  static constexpr float TopbarTitleH = 28.0f;

  static constexpr float SidebarInset = 12.0f;
  static constexpr float SidebarHeaderH = 20.0f;
  static constexpr float SidebarTreeTop = 44.0f;
  static constexpr float SidebarTreeBottomInset = 20.0f;
  static constexpr float SidebarAccentW = 3.0f;
  static constexpr float SidebarSceneX = 40.0f;
  static constexpr float SidebarSceneY = 12.0f;
  static constexpr float SidebarSceneW = 120.0f;
  static constexpr float SidebarHierarchyY = 52.0f;
  static constexpr float SidebarHierarchyW = 140.0f;
  static constexpr float TreeHeaderDividerY = 30.0f;
  static constexpr float TreeThumbHeight = 90.0f;
  static constexpr float TreeThumbOffset = 60.0f;

  static constexpr float ContentInset = 16.0f;
  static constexpr float SectionHeaderY = 14.0f;
  static constexpr float SectionHeaderH = 32.0f;
  static constexpr float BoardTitleY = 12.0f;
  static constexpr float BoardTitleH = 30.0f;
  static constexpr float BoardTitleW = 200.0f;
  static constexpr float BoardBodyY = 36.0f;
  static constexpr float BoardBodyW = 460.0f;
  static constexpr float TableHeaderPadY = 6.0f;
  static constexpr float TableStatusOffset = 200.0f;
  static constexpr float BoardPanelY = 60.0f;
  static constexpr float BoardPanelH = 110.0f;
  static constexpr float BoardButtonW = 120.0f;
  static constexpr float BoardButtonH = 32.0f;
  static constexpr float BoardButtonInset = 12.0f;
  static constexpr float HighlightHeaderY = 170.0f;
  static constexpr float HighlightHeaderH = 20.0f;
  static constexpr float HighlightDividerOffset = 2.0f;
  static constexpr float CardGridY = 196.0f;
  static constexpr float CardGridH = 120.0f;
  static constexpr float TableX = 16.0f;
  static constexpr float TableRightInset = 48.0f;
  static constexpr float ListY = 340.0f;
  static constexpr float ListHeaderOffset = 20.0f;

  static constexpr float InspectorHeaderY = 14.0f;
  static constexpr float InspectorHeaderH = 32.0f;
  static constexpr float InspectorPanelX = 16.0f;
  static constexpr float InspectorPanelInset = 32.0f;
  static constexpr float PropertiesPanelY = 56.0f;
  static constexpr float PropertiesPanelH = 90.0f;
  static constexpr float TransformPanelY = 164.0f;
  static constexpr float TransformPanelH = 140.0f;
  static constexpr float OpacityRowOffset = 44.0f;
  static constexpr float OpacityBarH = 22.0f;
  static constexpr float OpacityValue = 0.85f;
  static constexpr float PublishInsetX = 16.0f;
  static constexpr float PublishInsetY = 24.0f;
  static constexpr float PublishButtonH = 32.0f;
};
} // namespace

int main(int argc, char** argv) {
  std::string outPath = argc > 1 ? argv[1] : "screenshots/primeframe_ui.png";
  std::filesystem::path outFile(outPath);
  if (outFile.has_parent_path()) {
    std::filesystem::create_directories(outFile.parent_path());
  }

  constexpr float kWidth = StudioMetrics::ShellWidth;
  constexpr float kHeight = StudioMetrics::ShellHeight;
  constexpr float opacityBarH = StudioMetrics::OpacityBarH;

  Frame frame;

  ShellSpec shellSpec;
  shellSpec.bounds = Bounds{0.0f, 0.0f, kWidth, kHeight};
  shellSpec.topbarHeight = StudioMetrics::TopbarHeight;
  shellSpec.statusHeight = StudioMetrics::StatusHeight;
  shellSpec.sidebarWidth = StudioMetrics::SidebarWidth;
  shellSpec.inspectorWidth = StudioMetrics::InspectorWidth;
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
  UiNode topbarNode = shell.topbar;
  UiNode sidebarNode = shell.sidebar;
  UiNode contentNode = shell.content;
  UiNode inspectorNode = shell.inspector;

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
    add_divider(topbarNode, Bounds{StudioMetrics::TopbarDividerX,
                                   StudioMetrics::TopbarPaddingY,
                                   StudioMetrics::TopbarDividerW,
                                   StudioMetrics::TopbarButtonH});
    topbarNode.createTextField(Bounds{StudioMetrics::TopbarDividerX + StudioMetrics::ContentInset,
                                      StudioMetrics::TopbarPaddingY,
                                      StudioMetrics::TopbarSearchW,
                                      StudioMetrics::TopbarSearchH},
                               "Search...");
    UiNode runNode = topbarNode.createButton(Bounds{shellWidth - StudioMetrics::TopbarRunRight,
                                                    StudioMetrics::TopbarPaddingY,
                                                    StudioMetrics::TopbarButtonW,
                                                    StudioMetrics::TopbarButtonH},
                                             "Run",
                                             ButtonVariant::Primary);

    UiNode shareNode = topbarNode.createButton(Bounds{shellWidth - StudioMetrics::TopbarShareRight,
                                                      StudioMetrics::TopbarPaddingY,
                                                      StudioMetrics::TopbarButtonW,
                                                      StudioMetrics::TopbarButtonH},
                                               "Share");

    topbarNode.createTextLine(Bounds{0.0f,
                                     StudioMetrics::TopbarTitleY,
                                     StudioMetrics::TopbarDividerX,
                                     StudioMetrics::TopbarTitleH},
                              "PrimeFrame Studio",
                              TextRole::TitleBright,
                              PrimeFrame::TextAlign::Center);
  };

  auto create_sidebar = [&]() {
    Bounds treePanelBounds = insetBounds(sidebarLocal,
                                         StudioMetrics::SidebarInset,
                                         StudioMetrics::SidebarTreeTop,
                                         StudioMetrics::SidebarInset,
                                         StudioMetrics::SidebarTreeBottomInset);
    UiNode treePanel = add_panel(sidebarNode, treePanelBounds, RectRole::Panel);
    add_panel(sidebarNode,
              Bounds{StudioMetrics::SidebarInset,
                     StudioMetrics::SidebarInset,
                     sidebarW - StudioMetrics::SidebarInset * 2.0f,
                     StudioMetrics::SidebarHeaderH},
              RectRole::PanelStrong);
    add_panel(sidebarNode,
              Bounds{StudioMetrics::SidebarInset,
                     StudioMetrics::SidebarInset,
                     StudioMetrics::SidebarAccentW,
                     StudioMetrics::SidebarHeaderH},
              RectRole::Accent);

    sidebarNode.createTextLine(Bounds{StudioMetrics::SidebarSceneX,
                                      StudioMetrics::SidebarSceneY,
                                      StudioMetrics::SidebarSceneW,
                                      StudioMetrics::SidebarHeaderH},
                               "Scene",
                               TextRole::BodyBright);
    sidebarNode.createTextLine(Bounds{StudioMetrics::SidebarSceneX,
                                      StudioMetrics::SidebarHierarchyY,
                                      StudioMetrics::SidebarHierarchyW,
                                      StudioMetrics::SidebarHeaderH},
                               "Hierarchy",
                               TextRole::SmallMuted);

    TreeViewSpec treeSpec;
    treeSpec.bounds = Bounds{0.0f, 0.0f, treePanelBounds.width, treePanelBounds.height};
    treeSpec.showHeaderDivider = true;
    treeSpec.headerDividerY = StudioMetrics::TreeHeaderDividerY;
    float treeTrackH = treePanelBounds.height - treeSpec.scrollBar.padding * 2.0f;
    setScrollBarThumbPixels(treeSpec.scrollBar,
                            treeTrackH,
                            StudioMetrics::TreeThumbHeight,
                            StudioMetrics::TreeThumbOffset - treeSpec.scrollBar.padding);

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
    Bounds boardPanelBounds{StudioMetrics::ContentInset,
                            StudioMetrics::BoardPanelY,
                            contentW - StudioMetrics::ContentInset * 2.0f,
                            StudioMetrics::BoardPanelH};
    UiNode boardPanel = add_panel(contentNode, boardPanelBounds, RectRole::Panel);
    Bounds primaryBounds = alignBottomRight(boardPanelBounds,
                                            StudioMetrics::BoardButtonW,
                                            StudioMetrics::BoardButtonH,
                                            StudioMetrics::BoardButtonInset,
                                            StudioMetrics::BoardButtonInset);
    UiNode primaryButton = contentNode.createButton(primaryBounds, "Primary Action", ButtonVariant::Primary);
    float highlightHeaderY = StudioMetrics::HighlightHeaderY;
    float highlightHeaderH = StudioMetrics::HighlightHeaderH;

    contentNode.createSectionHeader(Bounds{StudioMetrics::ContentInset,
                                           StudioMetrics::SectionHeaderY,
                                           contentW - StudioMetrics::ContentInset * 2.0f,
                                           StudioMetrics::SectionHeaderH},
                                    "Overview",
                                    TextRole::TitleBright);

    contentNode.createSectionHeader(Bounds{StudioMetrics::ContentInset,
                                           highlightHeaderY,
                                           contentW - StudioMetrics::ContentInset * 2.0f,
                                           highlightHeaderH},
                                    "Highlights",
                                    TextRole::SmallBright,
                                    true,
                                    StudioMetrics::HighlightDividerOffset);


    contentNode.createCardGrid(Bounds{StudioMetrics::ContentInset,
                                      StudioMetrics::CardGridY,
                                      contentW - StudioMetrics::ContentInset,
                                      StudioMetrics::CardGridH},
                               {
                                   {"Card", "Detail"},
                                   {"Card", "Detail"},
                                   {"Card", "Detail"}
                               });

    boardPanel.createTextLine(Bounds{StudioMetrics::ContentInset,
                                     StudioMetrics::BoardTitleY,
                                     StudioMetrics::BoardTitleW,
                                     StudioMetrics::BoardTitleH},
                              "Active Board",
                              TextRole::SmallMuted);
    boardPanel.createParagraph(Bounds{StudioMetrics::ContentInset,
                                      StudioMetrics::BoardBodyY,
                                      StudioMetrics::BoardBodyW,
                                      0.0f},
                               "Lorem ipsum dolor sit amet, consectetur adipiscing elit.\n"
                               "Sed do eiusmod tempor incididunt ut labore et dolore.\n"
                               "Ut enim ad minim veniam, quis nostrud exercitation.",
                               TextRole::SmallMuted);

    float listY = StudioMetrics::ListY;
    float listHeaderY = listY - StudioMetrics::ListHeaderOffset;
    float tableWidth = contentW - StudioMetrics::TableX - StudioMetrics::TableRightInset;
    float firstColWidth = contentW - StudioMetrics::TableStatusOffset;
    float secondColWidth = tableWidth - firstColWidth;

    contentNode.createTable(Bounds{StudioMetrics::TableX,
                                   listHeaderY - StudioMetrics::TableHeaderPadY,
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

    contentNode.createScrollHints(Bounds{0.0f, 0.0f, contentW, contentH});
  };

  auto create_inspector = [&]() {
    inspectorNode.createSectionHeader(Bounds{StudioMetrics::InspectorPanelX,
                                              StudioMetrics::InspectorHeaderY,
                                              inspectorW - StudioMetrics::InspectorPanelInset,
                                              StudioMetrics::InspectorHeaderH},
                                      "Inspector",
                                      TextRole::BodyBright);

    SectionPanel propsPanel =
        inspectorNode.createSectionPanel(Bounds{StudioMetrics::InspectorPanelX,
                                                 StudioMetrics::PropertiesPanelY,
                                                 inspectorW - StudioMetrics::InspectorPanelInset,
                                                 StudioMetrics::PropertiesPanelH},
                                                               "Properties");

    SectionPanel transformPanel =
        inspectorNode.createSectionPanel(Bounds{StudioMetrics::InspectorPanelX,
                                                 StudioMetrics::TransformPanelY,
                                                 inspectorW - StudioMetrics::InspectorPanelInset,
                                                 StudioMetrics::TransformPanelH},
                                                                   "Transform");

    float opacityRowY = transformPanel.contentBounds.y + StudioMetrics::OpacityRowOffset;
    float opacityBarX = transformPanel.contentBounds.x;
    transformPanel.panel.createProgressBar(
        Bounds{opacityBarX, opacityRowY, transformPanel.contentBounds.width, opacityBarH},
        StudioMetrics::OpacityValue);

    Bounds publishBounds = alignBottomRight(inspectorLocal,
                                            inspectorW - StudioMetrics::InspectorPanelInset,
                                            StudioMetrics::PublishButtonH,
                                            StudioMetrics::PublishInsetX,
                                            StudioMetrics::PublishInsetY);
    UiNode publishButton = inspectorNode.createButton(publishBounds,
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
