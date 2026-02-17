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
  constexpr float topbarH = 56.0f;
  constexpr float statusH = 24.0f;
  constexpr float sidebarW = 240.0f;
  constexpr float inspectorW = 260.0f;
  constexpr float opacityBarH = 22.0f;
  constexpr float opacityBarInset = 24.0f;
  constexpr float contentW = kWidth - sidebarW - inspectorW;
  constexpr float contentH = kHeight - topbarH - statusH;

  Frame frame;

  ShellSpec shellSpec;
  shellSpec.bounds = Bounds{0.0f, 0.0f, kWidth, kHeight};
  ShellLayout shell = createShell(frame, shellSpec);
  UiNode root = shell.root;
  UiNode topbarNode = shell.topbar;
  UiNode sidebarNode = shell.sidebar;
  UiNode contentNode = shell.content;
  UiNode inspectorNode = shell.inspector;

  auto add_panel = [&](UiNode& parent, float x, float y, float w, float h, RectRole role) -> UiNode {
    return parent.createPanel(role, Bounds{x, y, w, h});
  };

  auto add_divider = [&](UiNode& parent, float x, float y, float w, float h) -> UiNode {
    DividerSpec spec;
    spec.bounds = Bounds{x, y, w, h};
    spec.rectStyle = rectToken(RectRole::Divider);
    return parent.createDivider(spec);
  };

  auto create_topbar = [&]() {
    add_divider(topbarNode, 232.0f, 12.0f, 1.0f, 32.0f);
    topbarNode.createTextField(Bounds{248.0f, 12.0f, 360.0f, 32.0f}, "Search...");
    UiNode runNode = topbarNode.createButton(Bounds{kWidth - 220.0f, 12.0f, 88.0f, 32.0f},
                                             "Run",
                                             ButtonVariant::Primary);

    UiNode shareNode = topbarNode.createButton(Bounds{kWidth - 120.0f, 12.0f, 88.0f, 32.0f},
                                               "Share");

    topbarNode.createTextLine(Bounds{0.0f, 14.0f, 232.0f, 28.0f},
                              "PrimeFrame Studio",
                              TextRole::TitleBright,
                              PrimeFrame::TextAlign::Center);
  };

  auto create_sidebar = [&]() {
    UiNode treePanel = add_panel(sidebarNode, 12.0f, 44.0f, sidebarW - 24.0f, contentH - 64.0f, RectRole::Panel);
    add_panel(sidebarNode, 12.0f, 12.0f, sidebarW - 24.0f, 20.0f, RectRole::PanelStrong);
    add_panel(sidebarNode, 12.0f, 12.0f, 3.0f, 20.0f, RectRole::Accent);

    sidebarNode.createTextLine(Bounds{40.0f, 12.0f, 120.0f, 20.0f},
                               "Scene",
                               TextRole::BodyBright);
    sidebarNode.createTextLine(Bounds{40.0f, 52.0f, 140.0f, 20.0f},
                               "Hierarchy",
                               TextRole::SmallMuted);

    TreeViewSpec treeSpec;
    treeSpec.bounds = Bounds{0.0f, 0.0f, sidebarW - 24.0f, contentH - 64.0f};
    treeSpec.showHeaderDivider = true;
    treeSpec.headerDividerY = 30.0f;
    treeSpec.scrollBar.thumbFraction = 90.0f / (contentH - 80.0f);
    float treeTrackSpan = std::max(1.0f, (contentH - 80.0f) - 90.0f);
    treeSpec.scrollBar.thumbProgress = (60.0f - 8.0f) / treeTrackSpan;

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
    UiNode boardPanel = add_panel(contentNode, 16.0f, 60.0f, contentW - 32.0f, 110.0f, RectRole::Panel);
    float boardButtonW = 120.0f;
    float boardButtonH = 32.0f;
    float boardButtonX = contentW - 148.0f;
    float boardButtonY = 60.0f + 110.0f - boardButtonH - 12.0f;
    UiNode primaryButton = contentNode.createButton(Bounds{boardButtonX, boardButtonY, boardButtonW, boardButtonH},
                                                    "Primary Action",
                                                    ButtonVariant::Primary);
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

    ScrollViewSpec scrollSpec;
    scrollSpec.bounds = Bounds{0.0f, 0.0f, contentW, contentH};
    scrollSpec.vertical.thumbLength = 120.0f;
    scrollSpec.vertical.thumbOffset = 108.0f;
    scrollSpec.horizontal.startPadding = 16.0f;
    scrollSpec.horizontal.endPadding = 48.0f;
    scrollSpec.horizontal.thumbLength = 120.0f;
    scrollSpec.horizontal.thumbOffset = 124.0f;
    contentNode.createScrollView(scrollSpec);
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

    UiNode publishButton = inspectorNode.createButton(Bounds{16.0f, contentH - 56.0f, inspectorW - 32.0f, 32.0f},
                                                      "Publish",
                                                      ButtonVariant::Primary);

    propsPanel.panel.createPropertyList(propsPanel.contentBounds,
                                        {{"Name", "SceneRoot"}, {"Tag", "Environment"}});

    transformPanel.panel.createPropertyList(transformPanel.contentBounds,
                                            {{"Position", "0, 0, 0"}, {"Scale", "1, 1, 1"}});

    PropertyListSpec opacityList;
    opacityList.bounds = transformPanel.contentBounds;
    opacityList.bounds.y = opacityRowY;
    opacityList.rowHeight = opacityBarH;
    opacityList.rowGap = 0.0f;
    opacityList.labelRole = TextRole::SmallBright;
    opacityList.rows = {{"Opacity", "85%"}};
    transformPanel.panel.createPropertyList(opacityList);
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
