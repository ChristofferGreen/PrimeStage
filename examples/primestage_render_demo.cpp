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

  constexpr float kWidth = 1100.0f;
  constexpr float kHeight = 700.0f;
  constexpr float opacityBarH = 22.0f;

  Frame frame;

  ShellSpec shellSpec;
  shellSpec.bounds = Bounds{0.0f, 0.0f, kWidth, kHeight};
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
    add_divider(topbarNode, Bounds{232.0f, 12.0f, 1.0f, 32.0f});
    topbarNode.createTextField(Bounds{248.0f, 12.0f, 360.0f, 32.0f}, "Search...");
    UiNode runNode = topbarNode.createButton(Bounds{shellWidth - 220.0f, 12.0f, 88.0f, 32.0f},
                                             "Run",
                                             ButtonVariant::Primary);

    UiNode shareNode = topbarNode.createButton(Bounds{shellWidth - 120.0f, 12.0f, 88.0f, 32.0f},
                                               "Share");

    topbarNode.createTextLine(Bounds{0.0f, 14.0f, 232.0f, 28.0f},
                              "PrimeFrame Studio",
                              TextRole::TitleBright,
                              PrimeFrame::TextAlign::Center);
  };

  auto create_sidebar = [&]() {
    Bounds treePanelBounds = insetBounds(sidebarLocal, 12.0f, 44.0f, 12.0f, 20.0f);
    UiNode treePanel = add_panel(sidebarNode, treePanelBounds, RectRole::Panel);
    add_panel(sidebarNode, Bounds{12.0f, 12.0f, sidebarW - 24.0f, 20.0f}, RectRole::PanelStrong);
    add_panel(sidebarNode, Bounds{12.0f, 12.0f, 3.0f, 20.0f}, RectRole::Accent);

    sidebarNode.createTextLine(Bounds{40.0f, 12.0f, 120.0f, 20.0f},
                               "Scene",
                               TextRole::BodyBright);
    sidebarNode.createTextLine(Bounds{40.0f, 52.0f, 140.0f, 20.0f},
                               "Hierarchy",
                               TextRole::SmallMuted);

    TreeViewSpec treeSpec;
    treeSpec.bounds = Bounds{0.0f, 0.0f, treePanelBounds.width, treePanelBounds.height};
    treeSpec.showHeaderDivider = true;
    treeSpec.headerDividerY = 30.0f;
    float treeTrackH = treePanelBounds.height - treeSpec.scrollBar.padding * 2.0f;
    setScrollBarThumbPixels(treeSpec.scrollBar,
                            treeTrackH,
                            90.0f,
                            60.0f - treeSpec.scrollBar.padding);

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
    Bounds boardPanelBounds{16.0f, 60.0f, contentW - 32.0f, 110.0f};
    UiNode boardPanel = add_panel(contentNode, boardPanelBounds, RectRole::Panel);
    Bounds primaryBounds = alignBottomRight(boardPanelBounds, 120.0f, 32.0f, 12.0f, 12.0f);
    UiNode primaryButton = contentNode.createButton(primaryBounds, "Primary Action", ButtonVariant::Primary);
    float highlightHeaderY = 170.0f;
    float highlightHeaderH = 20.0f;

    contentNode.createSectionHeader(Bounds{16.0f, 14.0f, contentW - 32.0f, 32.0f},
                                    "Overview",
                                    TextRole::TitleBright);

    contentNode.createSectionHeader(Bounds{16.0f, highlightHeaderY, contentW - 32.0f, highlightHeaderH},
                                    "Highlights",
                                    TextRole::SmallBright,
                                    true,
                                    2.0f);


    contentNode.createCardGrid(Bounds{16.0f, 196.0f, contentW - 16.0f, 120.0f},
                               {
                                   {"Card", "Detail"},
                                   {"Card", "Detail"},
                                   {"Card", "Detail"}
                               });

    boardPanel.createTextLine(Bounds{16.0f, 12.0f, 200.0f, 30.0f},
                              "Active Board",
                              TextRole::SmallMuted);
    boardPanel.createParagraph(Bounds{16.0f, 36.0f, 460.0f, 0.0f},
                               "Lorem ipsum dolor sit amet, consectetur adipiscing elit.\n"
                               "Sed do eiusmod tempor incididunt ut labore et dolore.\n"
                               "Ut enim ad minim veniam, quis nostrud exercitation.",
                               TextRole::SmallMuted);

    float listY = 340.0f;
    float listHeaderY = listY - 20.0f;
    float tableWidth = contentW - 64.0f;
    float firstColWidth = contentW - 200.0f;
    float secondColWidth = tableWidth - firstColWidth;

    contentNode.createTable(Bounds{16.0f, listHeaderY - 6.0f, tableWidth, 0.0f},
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
    inspectorNode.createSectionHeader(Bounds{16.0f, 14.0f, inspectorW - 32.0f, 32.0f},
                                      "Inspector",
                                      TextRole::BodyBright);

    SectionPanel propsPanel = inspectorNode.createSectionPanel(Bounds{16.0f, 56.0f, inspectorW - 32.0f, 90.0f},
                                                               "Properties");

    SectionPanel transformPanel = inspectorNode.createSectionPanel(Bounds{16.0f, 164.0f, inspectorW - 32.0f, 140.0f},
                                                                   "Transform");

    float opacityRowY = transformPanel.contentBounds.y + 44.0f;
    float opacityBarX = transformPanel.contentBounds.x;
    transformPanel.panel.createProgressBar(
        Bounds{opacityBarX, opacityRowY, transformPanel.contentBounds.width, opacityBarH},
        0.85f);

    Bounds publishBounds = alignBottomRight(inspectorLocal, inspectorW - 32.0f, 32.0f, 16.0f, 24.0f);
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
