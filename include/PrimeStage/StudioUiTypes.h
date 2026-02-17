#pragma once

// Intended for inclusion from PrimeStage/Ui.h when PRIMESTAGE_STUDIO_UI is defined.

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

struct UiDefaults {
  static constexpr float CanvasWidth = 1100.0f;
  static constexpr float CanvasHeight = 700.0f;
  static constexpr float EdgeBarHeight = 56.0f;
  static constexpr float StatusHeight = 24.0f;
  static constexpr float PrimaryRailWidth = 240.0f;
  static constexpr float SecondaryRailWidth = 260.0f;
  static constexpr float SurfaceInset = 16.0f;
  static constexpr float PanelInset = 12.0f;
  static constexpr float PanelGap = 10.0f;
  static constexpr float PanelGapLarge = 18.0f;
  static constexpr float DividerThickness = 1.0f;
  static constexpr float AccentThickness = 3.0f;
  static constexpr float HeaderHeight = 20.0f;
  static constexpr float TitleHeight = 28.0f;
  static constexpr float SectionHeaderHeight = 32.0f;
  static constexpr float SectionHeaderOffsetY = 14.0f;
  static constexpr float SectionGap = 14.0f;
  static constexpr float SectionGapSmall = 6.0f;
  static constexpr float SectionGapLarge = 24.0f;
  static constexpr float HeaderDividerOffset = 2.0f;
  static constexpr float ControlHeight = 32.0f;
  static constexpr float ControlWidthM = 88.0f;
  static constexpr float ControlWidthL = 120.0f;
  static constexpr float FieldWidthL = 360.0f;
  static constexpr float TitleBlockWidth = 232.0f;
  static constexpr float PanelHeightL = 110.0f;
  static constexpr float PanelHeightM = 140.0f;
  static constexpr float PanelHeightS = 90.0f;
  static constexpr float CardWidth = 180.0f;
  static constexpr float CardHeight = 120.0f;
  static constexpr float TableHeaderOffset = 20.0f;
  static constexpr float TableHeaderPadY = 6.0f;
  static constexpr float TableRightInset = 48.0f;
  static constexpr float TableStatusOffset = 200.0f;
  static constexpr float LabelIndent = 20.0f;
  static constexpr float LabelGap = 8.0f;
  static constexpr float TreeHeaderDividerY = 30.0f;
  static constexpr float ScrollThumbHeight = 90.0f;
  static constexpr float ScrollThumbOffset = 60.0f;
  static constexpr float OpacityBarHeight = 22.0f;
  static constexpr float InlineRowOffset = 44.0f;
  static constexpr float FooterInset = 24.0f;
  static constexpr float TitleBaselineOffsetY = 14.0f;
  static constexpr float BodyOffsetY = 36.0f;
};

PrimeFrame::RectStyleToken rectToken(RectRole role);
PrimeFrame::TextStyleToken textToken(TextRole role);
void applyStudioTheme(PrimeFrame::Frame& frame);

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
  SizeSpec size;
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
  float headerInsetRight = 24.0f;
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
  SizeSpec size;
};

struct ProgressBarSpec {
  Bounds bounds;
  float value = 0.0f;
  float minFillWidth = 0.0f;
  RectRole trackRole = RectRole::PanelStrong;
  RectRole fillRole = RectRole::Accent;
  SizeSpec size;
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
  float topbarHeight = UiDefaults::EdgeBarHeight;
  float statusHeight = UiDefaults::StatusHeight;
  float sidebarWidth = UiDefaults::PrimaryRailWidth;
  float inspectorWidth = UiDefaults::SecondaryRailWidth;
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
  SizeSpec size;
};

enum class ButtonVariant {
  Default,
  Primary
};

struct ScrollHintsSpec {
  Bounds bounds;
  bool showVertical = true;
  bool showHorizontal = true;
  float verticalThumbLength = 120.0f;
  float verticalThumbOffset = 108.0f;
  float horizontalThumbLength = 120.0f;
  float horizontalThumbOffset = 124.0f;
  float horizontalStartPadding = 16.0f;
  float horizontalEndPadding = 48.0f;
  SizeSpec size;
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
  ScrollBarSpec scrollBar = [] {
    ScrollBarSpec spec;
    spec.trackStyle = rectToken(RectRole::ScrollTrack);
    spec.thumbStyle = rectToken(RectRole::ScrollThumb);
    return spec;
  }();
  std::vector<TreeNode> nodes;
  SizeSpec size;
};

struct RowLayout {
  Bounds bounds;
  float gap = UiDefaults::PanelInset;
  float cursorLeft = 0.0f;
  float cursorRight = 0.0f;

  RowLayout(Bounds const& bounds, float gap = UiDefaults::PanelInset);
  Bounds takeLeft(float width, float height = 0.0f);
  Bounds takeRight(float width, float height = 0.0f);
};
