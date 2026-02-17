#pragma once

#include "PrimeFrame/Frame.h"

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace PrimeStage {

enum class RectRole : PrimeFrame::RectStyleToken {
  Background = 0,
  Topbar,
  Statusbar,
  Sidebar,
  Content,
  Inspector,
  Panel,
  PanelAlt,
  PanelStrong,
  Accent,
  Selection,
  ScrollTrack,
  ScrollThumb,
  Divider
};

enum class TextRole : PrimeFrame::TextStyleToken {
  TitleBright = 0,
  BodyBright,
  BodyMuted,
  SmallBright,
  SmallMuted
};

PrimeFrame::RectStyleToken rectToken(RectRole role);
PrimeFrame::TextStyleToken textToken(TextRole role);
void applyStudioTheme(PrimeFrame::Frame& frame);

struct Bounds {
  float x = 0.0f;
  float y = 0.0f;
  float width = 0.0f;
  float height = 0.0f;
};

struct PanelSpec {
  Bounds bounds;
  PrimeFrame::RectStyleToken rectStyle = 0;
  PrimeFrame::RectStyleOverride rectStyleOverride{};
  PrimeFrame::LayoutType layout = PrimeFrame::LayoutType::None;
  PrimeFrame::Insets padding{};
  float gap = 0.0f;
  bool clipChildren = true;
  bool visible = true;
};

struct LabelSpec {
  Bounds bounds;
  std::string text;
  PrimeFrame::TextStyleToken textStyle = 0;
  PrimeFrame::TextStyleOverride textStyleOverride{};
  PrimeFrame::TextAlign align = PrimeFrame::TextAlign::Start;
  PrimeFrame::WrapMode wrap = PrimeFrame::WrapMode::Word;
  float maxWidth = 0.0f;
  bool visible = true;
};

struct ParagraphSpec {
  Bounds bounds;
  std::string text;
  TextRole textRole = TextRole::BodyMuted;
  PrimeFrame::TextAlign align = PrimeFrame::TextAlign::Start;
  PrimeFrame::WrapMode wrap = PrimeFrame::WrapMode::Word;
  float maxWidth = 0.0f;
  float textOffsetY = 0.0f;
  bool autoHeight = true;
  bool visible = true;
};

struct TextLineSpec {
  Bounds bounds;
  std::string text;
  TextRole textRole = TextRole::BodyBright;
  PrimeFrame::TextAlign align = PrimeFrame::TextAlign::Start;
  float textOffsetY = 0.0f;
  bool visible = true;
};

struct DividerSpec {
  Bounds bounds;
  PrimeFrame::RectStyleToken rectStyle = 0;
  PrimeFrame::RectStyleOverride rectStyleOverride{};
  bool visible = true;
};

struct SpacerSpec {
  Bounds bounds;
  bool visible = true;
};

struct TableColumn {
  std::string label;
  float width = 0.0f;
  TextRole headerRole = TextRole::SmallBright;
  TextRole cellRole = TextRole::SmallMuted;
};

struct TableSpec {
  Bounds bounds;
  float headerInset = 6.0f;
  float headerHeight = 20.0f;
  float rowHeight = 28.0f;
  float rowGap = 0.0f;
  float headerPaddingX = 16.0f;
  float cellPaddingX = 16.0f;
  RectRole headerRole = RectRole::PanelStrong;
  RectRole rowRole = RectRole::Panel;
  RectRole rowAltRole = RectRole::PanelAlt;
  RectRole dividerRole = RectRole::Divider;
  bool showHeaderDividers = true;
  bool showColumnDividers = true;
  bool clipChildren = true;
  std::vector<TableColumn> columns;
  std::vector<std::vector<std::string>> rows;
};

struct SectionHeaderSpec {
  Bounds bounds;
  std::string title;
  float accentWidth = 3.0f;
  float textInsetX = 16.0f;
  float textOffsetY = 0.0f;
  RectRole backgroundRole = RectRole::PanelStrong;
  RectRole accentRole = RectRole::Accent;
  RectRole dividerRole = RectRole::Divider;
  TextRole textRole = TextRole::SmallBright;
  bool addDivider = false;
  float dividerOffsetY = 0.0f;
};

struct SectionPanelSpec {
  Bounds bounds;
  std::string title;
  RectRole panelRole = RectRole::PanelAlt;
  RectRole headerRole = RectRole::PanelStrong;
  RectRole accentRole = RectRole::Accent;
  TextRole textRole = TextRole::SmallBright;
  float headerHeight = 22.0f;
  float headerInsetX = 8.0f;
  float headerInsetY = 12.0f;
  float headerInsetRight = 8.0f;
  float headerPaddingX = 8.0f;
  float accentWidth = 3.0f;
  bool showAccent = true;
  float contentInsetX = 16.0f;
  float contentInsetY = 12.0f;
  float contentInsetRight = 16.0f;
  float contentInsetBottom = 12.0f;
};

struct PropertyRow {
  std::string label;
  std::string value;
};

struct PropertyListSpec {
  Bounds bounds;
  float rowHeight = 12.0f;
  float rowGap = 12.0f;
  float labelInsetX = 0.0f;
  float valueInsetX = 0.0f;
  float valuePaddingRight = 0.0f;
  bool valueAlignRight = true;
  TextRole labelRole = TextRole::SmallMuted;
  TextRole valueRole = TextRole::SmallBright;
  std::vector<PropertyRow> rows;
};

struct ProgressBarSpec {
  Bounds bounds;
  float value = 0.0f;
  float minFillWidth = 0.0f;
  RectRole trackRole = RectRole::PanelStrong;
  RectRole fillRole = RectRole::Accent;
};

struct StatusBarSpec {
  Bounds bounds;
  std::string leftText;
  std::string rightText;
  float paddingX = 16.0f;
  RectRole backgroundRole = RectRole::Statusbar;
  TextRole leftRole = TextRole::SmallMuted;
  TextRole rightRole = TextRole::SmallMuted;
};

struct ShellSpec {
  Bounds bounds;
  float topbarHeight = 56.0f;
  float statusHeight = 24.0f;
  float sidebarWidth = 240.0f;
  float inspectorWidth = 260.0f;
  RectRole backgroundRole = RectRole::Background;
  RectRole topbarRole = RectRole::Topbar;
  RectRole sidebarRole = RectRole::Sidebar;
  RectRole contentRole = RectRole::Content;
  RectRole inspectorRole = RectRole::Inspector;
  RectRole dividerRole = RectRole::Divider;
  bool drawDividers = true;
};

struct CardSpec {
  std::string title;
  std::string subtitle;
};

struct CardGridSpec {
  Bounds bounds;
  float cardWidth = 180.0f;
  float cardHeight = 120.0f;
  float gapX = 16.0f;
  float gapY = 16.0f;
  float paddingX = 16.0f;
  float titleOffsetY = 16.0f;
  float subtitleOffsetY = 44.0f;
  RectRole cardRole = RectRole::PanelAlt;
  TextRole titleRole = TextRole::SmallBright;
  TextRole subtitleRole = TextRole::SmallMuted;
  std::vector<CardSpec> cards;
};

struct ButtonSpec {
  Bounds bounds;
  std::string label;
  RectRole backgroundRole = RectRole::Panel;
  TextRole textRole = TextRole::BodyBright;
  float textInsetX = 16.0f;
  float textOffsetY = 0.0f;
  bool centerText = true;
};

struct TextFieldSpec {
  Bounds bounds;
  std::string text;
  std::string placeholder;
  float paddingX = 16.0f;
  float textOffsetY = 0.0f;
  RectRole backgroundRole = RectRole::Panel;
  TextRole textRole = TextRole::BodyBright;
  TextRole placeholderRole = TextRole::BodyMuted;
  bool showPlaceholderWhenEmpty = true;
};

struct ScrollAxisSpec {
  bool enabled = true;
  float thickness = 6.0f;
  float inset = 12.0f;
  float startPadding = 12.0f;
  float endPadding = 12.0f;
  float thumbLength = 120.0f;
  float thumbOffset = 0.0f;
  RectRole trackRole = RectRole::ScrollTrack;
  RectRole thumbRole = RectRole::ScrollThumb;
};

struct ScrollBarSpec {
  bool enabled = true;
  float inset = 10.0f;
  float padding = 8.0f;
  float width = 6.0f;
  float minThumbHeight = 16.0f;
  float thumbFraction = 0.18f;
  float thumbProgress = 0.1f;
  RectRole trackRole = RectRole::ScrollTrack;
  RectRole thumbRole = RectRole::ScrollThumb;
};

struct ScrollViewSpec {
  Bounds bounds;
  bool clipChildren = true;
  bool showVertical = true;
  bool showHorizontal = true;
  ScrollAxisSpec vertical{};
  ScrollAxisSpec horizontal{};
};

struct TreeNode {
  std::string label;
  std::vector<TreeNode> children;
  bool expanded = true;
  bool selected = false;
};

struct TreeViewSpec {
  Bounds bounds;
  float rowStartX = 8.0f;
  float rowStartY = 36.0f;
  float rowWidthInset = 20.0f;
  float rowHeight = 22.0f;
  float rowGap = 0.0f;
  float indent = 12.0f;
  float caretBaseX = 14.0f;
  float caretSize = 10.0f;
  float caretInset = 2.0f;
  float caretThickness = 2.0f;
  float caretMaskPad = 2.0f;
  float connectorThickness = 1.0f;
  float linkEndInset = 4.0f;
  float selectionAccentWidth = 3.0f;
  bool showHeaderDivider = false;
  float headerDividerY = 0.0f;
  bool showConnectors = true;
  bool showCaretMasks = true;
  bool showScrollBar = true;
  bool clipChildren = true;
  RectRole rowRole = RectRole::Panel;
  RectRole rowAltRole = RectRole::PanelAlt;
  RectRole selectionRole = RectRole::Selection;
  RectRole selectionAccentRole = RectRole::Accent;
  RectRole caretBackgroundRole = RectRole::PanelStrong;
  RectRole caretLineRole = RectRole::PanelAlt;
  RectRole connectorRole = RectRole::ScrollTrack;
  TextRole textRole = TextRole::SmallMuted;
  TextRole selectedTextRole = TextRole::SmallBright;
  ScrollBarSpec scrollBar{};
  std::vector<TreeNode> nodes;
};

struct SectionPanel;

class UiNode {
public:
  UiNode(PrimeFrame::Frame& frame, PrimeFrame::NodeId id);

  PrimeFrame::NodeId nodeId() const { return id_; }
  PrimeFrame::Frame& frame() const { return frame_.get(); }

  UiNode createPanel(PanelSpec const& spec);
  UiNode createPanel(RectRole role, Bounds const& bounds);
  UiNode createLabel(LabelSpec const& spec);
  UiNode createLabel(std::string_view text, TextRole role, Bounds const& bounds);
  UiNode createParagraph(ParagraphSpec const& spec);
  UiNode createParagraph(Bounds const& bounds,
                         std::string_view text,
                         TextRole role = TextRole::BodyMuted);
  UiNode createTextLine(TextLineSpec const& spec);
  UiNode createTextLine(Bounds const& bounds,
                        std::string_view text,
                        TextRole role = TextRole::BodyBright,
                        PrimeFrame::TextAlign align = PrimeFrame::TextAlign::Start);
  UiNode createDivider(DividerSpec const& spec);
  UiNode createSpacer(SpacerSpec const& spec);
  UiNode createTable(TableSpec const& spec);
  UiNode createTable(Bounds const& bounds,
                     std::vector<TableColumn> columns,
                     std::vector<std::vector<std::string>> rows);
  UiNode createSectionHeader(SectionHeaderSpec const& spec);
  UiNode createSectionHeader(Bounds const& bounds,
                             std::string_view title,
                             TextRole role = TextRole::SmallBright);
  UiNode createSectionHeader(Bounds const& bounds,
                             std::string_view title,
                             TextRole role,
                             bool addDivider,
                             float dividerOffsetY = 0.0f);
  SectionPanel createSectionPanel(SectionPanelSpec const& spec);
  UiNode createPropertyList(PropertyListSpec const& spec);
  UiNode createProgressBar(ProgressBarSpec const& spec);
  UiNode createStatusBar(StatusBarSpec const& spec);
  UiNode createCardGrid(CardGridSpec const& spec);
  UiNode createCardGrid(Bounds const& bounds, std::vector<CardSpec> cards);
  UiNode createButton(ButtonSpec const& spec);
  UiNode createTextField(TextFieldSpec const& spec);
  UiNode createScrollView(ScrollViewSpec const& spec);
  UiNode createTreeView(TreeViewSpec const& spec);

private:
  std::reference_wrapper<PrimeFrame::Frame> frame_;
  PrimeFrame::NodeId id_{};
};

struct SectionPanel {
  UiNode panel;
  Bounds headerBounds;
  Bounds contentBounds;
};

struct ShellLayout {
  UiNode root;
  UiNode background;
  UiNode topbar;
  UiNode sidebar;
  UiNode content;
  UiNode inspector;
  Bounds bounds;
  Bounds topbarBounds;
  Bounds statusBounds;
  Bounds sidebarBounds;
  Bounds contentBounds;
  Bounds inspectorBounds;
};

UiNode createRoot(PrimeFrame::Frame& frame, Bounds const& bounds);
ShellLayout createShell(PrimeFrame::Frame& frame, ShellSpec const& spec);

} // namespace PrimeStage
