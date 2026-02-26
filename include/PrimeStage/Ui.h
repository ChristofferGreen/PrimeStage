#pragma once

#include "PrimeFrame/Frame.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace PrimeFrame {
class FocusManager;
struct LayoutOutput;
} // namespace PrimeFrame

namespace PrimeStage {

struct TextSelectionLayout;

enum class CursorHint : uint8_t {
  Arrow,
  IBeam,
};

struct SizeSpec {
  std::optional<float> minWidth;
  std::optional<float> maxWidth;
  std::optional<float> preferredWidth;
  float stretchX = 0.0f;
  std::optional<float> minHeight;
  std::optional<float> maxHeight;
  std::optional<float> preferredHeight;
  float stretchY = 0.0f;
};

struct ContainerSpec {
  SizeSpec size;
  PrimeFrame::Insets padding{};
  float gap = 0.0f;
  bool clipChildren = true;
  bool visible = true;
};

struct StackSpec : ContainerSpec {
};

struct PanelSpec : ContainerSpec {
  PrimeFrame::RectStyleToken rectStyle = 0;
  PrimeFrame::RectStyleOverride rectStyleOverride{};
  PrimeFrame::LayoutType layout = PrimeFrame::LayoutType::None;
};

struct LabelSpec {
  std::string_view text;
  PrimeFrame::TextStyleToken textStyle = 0;
  PrimeFrame::TextStyleOverride textStyleOverride{};
  PrimeFrame::TextAlign align = PrimeFrame::TextAlign::Start;
  PrimeFrame::WrapMode wrap = PrimeFrame::WrapMode::Word;
  float maxWidth = 0.0f;
  bool visible = true;
  SizeSpec size;
};

struct ParagraphSpec {
  std::string_view text;
  PrimeFrame::TextStyleToken textStyle = 0;
  PrimeFrame::TextStyleOverride textStyleOverride{};
  PrimeFrame::TextAlign align = PrimeFrame::TextAlign::Start;
  PrimeFrame::WrapMode wrap = PrimeFrame::WrapMode::Word;
  float maxWidth = 0.0f;
  float textOffsetY = 0.0f;
  bool autoHeight = true;
  bool visible = true;
  SizeSpec size;
};

struct TextLineSpec {
  std::string_view text;
  PrimeFrame::TextStyleToken textStyle = 0;
  PrimeFrame::TextStyleOverride textStyleOverride{};
  PrimeFrame::TextAlign align = PrimeFrame::TextAlign::Start;
  float textOffsetY = 0.0f;
  bool visible = true;
  SizeSpec size;
};

struct DividerSpec {
  PrimeFrame::RectStyleToken rectStyle = 0;
  PrimeFrame::RectStyleOverride rectStyleOverride{};
  bool visible = true;
  SizeSpec size;
};

struct SpacerSpec {
  bool visible = true;
  SizeSpec size;
};

struct ButtonCallbacks {
  std::function<void()> onClick;
  std::function<void(bool)> onHoverChanged;
  std::function<void(bool)> onPressedChanged;
};

struct ButtonSpec {
  std::string_view label;
  PrimeFrame::RectStyleToken backgroundStyle = 0;
  PrimeFrame::RectStyleOverride backgroundStyleOverride{};
  PrimeFrame::RectStyleToken hoverStyle = 0;
  PrimeFrame::RectStyleOverride hoverStyleOverride{};
  PrimeFrame::RectStyleToken pressedStyle = 0;
  PrimeFrame::RectStyleOverride pressedStyleOverride{};
  PrimeFrame::RectStyleToken focusStyle = 0;
  PrimeFrame::RectStyleOverride focusStyleOverride{};
  PrimeFrame::TextStyleToken textStyle = 0;
  PrimeFrame::TextStyleOverride textStyleOverride{};
  float textInsetX = 16.0f;
  float textOffsetY = 0.0f;
  bool centerText = true;
  float baseOpacity = 1.0f;
  float hoverOpacity = 1.0f;
  float pressedOpacity = 1.0f;
  ButtonCallbacks callbacks{};
  bool visible = true;
  SizeSpec size;
};

struct TextFieldState {
  std::string text;
  uint32_t cursor = 0u;
  uint32_t selectionAnchor = 0u;
  uint32_t selectionStart = 0u;
  uint32_t selectionEnd = 0u;
  bool focused = false;
  bool hovered = false;
  bool selecting = false;
  int pointerId = -1;
  bool cursorVisible = false;
  std::chrono::steady_clock::time_point nextBlink{};
  CursorHint cursorHint = CursorHint::Arrow;
};

struct TextFieldClipboard {
  std::function<void(std::string_view)> setText;
  std::function<std::string()> getText;
};

struct TextFieldCallbacks {
  std::function<void()> onStateChanged;
  std::function<void(std::string_view)> onTextChanged;
  std::function<void(bool)> onFocusChanged;
  std::function<void(bool)> onHoverChanged;
  std::function<void(CursorHint)> onCursorHintChanged;
  std::function<void()> onRequestBlur;
  std::function<void()> onSubmit;
};

struct TextFieldSpec {
  TextFieldState* state = nullptr;
  TextFieldCallbacks callbacks{};
  TextFieldClipboard clipboard{};
  std::string_view text;
  std::string_view placeholder;
  float paddingX = 16.0f;
  float textOffsetY = 0.0f;
  PrimeFrame::RectStyleToken backgroundStyle = 0;
  PrimeFrame::RectStyleOverride backgroundStyleOverride{};
  PrimeFrame::RectStyleToken focusStyle = 0;
  PrimeFrame::RectStyleOverride focusStyleOverride{};
  PrimeFrame::TextStyleToken textStyle = 0;
  PrimeFrame::TextStyleOverride textStyleOverride{};
  PrimeFrame::TextStyleToken placeholderStyle = 0;
  PrimeFrame::TextStyleOverride placeholderStyleOverride{};
  bool showPlaceholderWhenEmpty = true;
  bool showCursor = false;
  uint32_t cursorIndex = 0u;
  float cursorWidth = 2.0f;
  PrimeFrame::RectStyleToken cursorStyle = 0;
  PrimeFrame::RectStyleOverride cursorStyleOverride{};
  uint32_t selectionStart = 0u;
  uint32_t selectionEnd = 0u;
  PrimeFrame::RectStyleToken selectionStyle = 0;
  PrimeFrame::RectStyleOverride selectionStyleOverride{};
  std::chrono::milliseconds cursorBlinkInterval{std::chrono::milliseconds(500)};
  bool allowNewlines = false;
  bool handleClipboardShortcuts = true;
  bool setCursorToEndOnFocus = true;
  bool visible = true;
  SizeSpec size;
};

struct SelectableTextState {
  std::string_view text;
  uint32_t selectionAnchor = 0u;
  uint32_t selectionStart = 0u;
  uint32_t selectionEnd = 0u;
  bool focused = false;
  bool hovered = false;
  bool selecting = false;
  int pointerId = -1;
  CursorHint cursorHint = CursorHint::Arrow;
};

struct SelectableTextClipboard {
  std::function<void(std::string_view)> setText;
};

struct SelectableTextCallbacks {
  std::function<void()> onStateChanged;
  std::function<void(uint32_t, uint32_t)> onSelectionChanged;
  std::function<void(bool)> onFocusChanged;
  std::function<void(bool)> onHoverChanged;
  std::function<void(CursorHint)> onCursorHintChanged;
};

struct SelectableTextSpec {
  SelectableTextState* state = nullptr;
  SelectableTextCallbacks callbacks{};
  SelectableTextClipboard clipboard{};
  std::string_view text;
  PrimeFrame::TextStyleToken textStyle = 0;
  PrimeFrame::TextStyleOverride textStyleOverride{};
  PrimeFrame::WrapMode wrap = PrimeFrame::WrapMode::Word;
  float maxWidth = 0.0f;
  float paddingX = 0.0f;
  uint32_t selectionStart = 0u;
  uint32_t selectionEnd = 0u;
  PrimeFrame::RectStyleToken selectionStyle = 0;
  PrimeFrame::RectStyleOverride selectionStyleOverride{};
  PrimeFrame::RectStyleToken focusStyle = 0;
  PrimeFrame::RectStyleOverride focusStyleOverride{};
  bool handleClipboardShortcuts = true;
  bool visible = true;
  SizeSpec size;
};

struct TextSelectionOverlaySpec {
  std::string_view text;
  PrimeFrame::TextStyleToken textStyle = 0;
  PrimeFrame::WrapMode wrap = PrimeFrame::WrapMode::Word;
  float maxWidth = 0.0f;
  TextSelectionLayout const* layout = nullptr;
  uint32_t selectionStart = 0u;
  uint32_t selectionEnd = 0u;
  float paddingX = 0.0f;
  PrimeFrame::RectStyleToken selectionStyle = 0;
  PrimeFrame::RectStyleOverride selectionStyleOverride{};
  bool clipChildren = true;
  bool visible = true;
  SizeSpec size;
};

struct ToggleCallbacks {
  std::function<void(bool)> onChanged;
};

struct ToggleSpec {
  bool on = false;
  ToggleCallbacks callbacks{};
  float knobInset = 2.0f;
  PrimeFrame::RectStyleToken trackStyle = 0;
  PrimeFrame::RectStyleOverride trackStyleOverride{};
  PrimeFrame::RectStyleToken knobStyle = 0;
  PrimeFrame::RectStyleOverride knobStyleOverride{};
  PrimeFrame::RectStyleToken focusStyle = 0;
  PrimeFrame::RectStyleOverride focusStyleOverride{};
  bool visible = true;
  SizeSpec size;
};

struct CheckboxCallbacks {
  std::function<void(bool)> onChanged;
};

struct CheckboxSpec {
  std::string_view label;
  bool checked = false;
  CheckboxCallbacks callbacks{};
  float boxSize = 16.0f;
  float checkInset = 3.0f;
  float gap = 8.0f;
  PrimeFrame::RectStyleToken boxStyle = 0;
  PrimeFrame::RectStyleOverride boxStyleOverride{};
  PrimeFrame::RectStyleToken checkStyle = 0;
  PrimeFrame::RectStyleOverride checkStyleOverride{};
  PrimeFrame::RectStyleToken focusStyle = 0;
  PrimeFrame::RectStyleOverride focusStyleOverride{};
  PrimeFrame::TextStyleToken textStyle = 0;
  PrimeFrame::TextStyleOverride textStyleOverride{};
  bool visible = true;
  SizeSpec size;
};

struct SliderCallbacks {
  std::function<void(float)> onValueChanged;
  std::function<void()> onDragStart;
  std::function<void()> onDragEnd;
};

struct SliderSpec {
  float value = 0.0f;
  bool vertical = false;
  float trackThickness = 6.0f;
  float thumbSize = 14.0f;
  PrimeFrame::RectStyleToken trackStyle = 0;
  PrimeFrame::RectStyleOverride trackStyleOverride{};
  PrimeFrame::RectStyleToken fillStyle = 0;
  PrimeFrame::RectStyleOverride fillStyleOverride{};
  std::optional<float> fillHoverOpacity;
  std::optional<float> fillPressedOpacity;
  PrimeFrame::RectStyleToken thumbStyle = 0;
  PrimeFrame::RectStyleOverride thumbStyleOverride{};
  PrimeFrame::RectStyleToken focusStyle = 0;
  PrimeFrame::RectStyleOverride focusStyleOverride{};
  std::optional<float> trackHoverOpacity;
  std::optional<float> trackPressedOpacity;
  std::optional<float> thumbHoverOpacity;
  std::optional<float> thumbPressedOpacity;
  SliderCallbacks callbacks{};
  bool visible = true;
  SizeSpec size;
};

struct TabsCallbacks {
  std::function<void(int)> onTabChanged;
};

struct TabsSpec {
  std::vector<std::string_view> labels;
  int selectedIndex = 0;
  TabsCallbacks callbacks{};
  float tabPaddingX = 12.0f;
  float tabPaddingY = 6.0f;
  float gap = 4.0f;
  PrimeFrame::RectStyleToken tabStyle = 0;
  PrimeFrame::RectStyleOverride tabStyleOverride{};
  PrimeFrame::RectStyleToken activeTabStyle = 0;
  PrimeFrame::RectStyleOverride activeTabStyleOverride{};
  PrimeFrame::TextStyleToken textStyle = 0;
  PrimeFrame::TextStyleOverride textStyleOverride{};
  PrimeFrame::TextStyleToken activeTextStyle = 0;
  PrimeFrame::TextStyleOverride activeTextStyleOverride{};
  bool visible = true;
  SizeSpec size;
};

struct DropdownCallbacks {
  std::function<void()> onOpened;
  std::function<void(int)> onSelected;
};

struct DropdownSpec {
  std::vector<std::string_view> options;
  int selectedIndex = 0;
  DropdownCallbacks callbacks{};
  std::string_view label;
  std::string_view indicator = "v";
  float paddingX = 12.0f;
  float indicatorGap = 8.0f;
  PrimeFrame::RectStyleToken backgroundStyle = 0;
  PrimeFrame::RectStyleOverride backgroundStyleOverride{};
  PrimeFrame::TextStyleToken textStyle = 0;
  PrimeFrame::TextStyleOverride textStyleOverride{};
  PrimeFrame::TextStyleToken indicatorStyle = 0;
  PrimeFrame::TextStyleOverride indicatorStyleOverride{};
  PrimeFrame::RectStyleToken focusStyle = 0;
  PrimeFrame::RectStyleOverride focusStyleOverride{};
  bool visible = true;
  SizeSpec size;
};

struct ProgressBarSpec {
  float value = 0.0f;
  float minFillWidth = 0.0f;
  PrimeFrame::RectStyleToken trackStyle = 0;
  PrimeFrame::RectStyleOverride trackStyleOverride{};
  PrimeFrame::RectStyleToken fillStyle = 0;
  PrimeFrame::RectStyleOverride fillStyleOverride{};
  PrimeFrame::RectStyleToken focusStyle = 0;
  PrimeFrame::RectStyleOverride focusStyleOverride{};
  bool visible = true;
  SizeSpec size;
};

struct ScrollAxisSpec {
  bool enabled = true;
  float thickness = 6.0f;
  float inset = 12.0f;
  float startPadding = 12.0f;
  float endPadding = 12.0f;
  float thumbLength = 120.0f;
  float thumbOffset = 0.0f;
  PrimeFrame::RectStyleToken trackStyle = 0;
  PrimeFrame::RectStyleToken thumbStyle = 0;
};

struct ScrollBarSpec {
  bool enabled = true;
  bool autoThumb = true;
  float inset = 10.0f;
  float padding = 8.0f;
  float width = 6.0f;
  float minThumbHeight = 16.0f;
  float thumbFraction = 0.18f;
  float thumbProgress = 0.0f;
  std::optional<float> trackHoverOpacity;
  std::optional<float> trackPressedOpacity;
  std::optional<float> thumbHoverOpacity;
  std::optional<float> thumbPressedOpacity;
  PrimeFrame::RectStyleToken trackStyle = 0;
  PrimeFrame::RectStyleOverride trackStyleOverride{};
  PrimeFrame::RectStyleToken thumbStyle = 0;
  PrimeFrame::RectStyleOverride thumbStyleOverride{};
};

struct TableColumn {
  std::string_view label;
  float width = 0.0f;
  PrimeFrame::TextStyleToken headerStyle = 0;
  PrimeFrame::TextStyleToken cellStyle = 0;
};

struct TableRowInfo {
  int rowIndex = -1;
  std::span<const std::string_view> row{};
};

struct TableCallbacks {
  std::function<void(TableRowInfo const&)> onRowClicked;
};

struct TableSpec {
  float headerInset = 6.0f;
  float headerHeight = 20.0f;
  float rowHeight = 28.0f;
  float rowGap = 0.0f;
  float headerPaddingX = 16.0f;
  float cellPaddingX = 16.0f;
  PrimeFrame::RectStyleToken headerStyle = 0;
  PrimeFrame::RectStyleToken rowStyle = 0;
  PrimeFrame::RectStyleToken rowAltStyle = 0;
  PrimeFrame::RectStyleToken selectionStyle = 0;
  PrimeFrame::RectStyleToken dividerStyle = 0;
  PrimeFrame::RectStyleToken focusStyle = 0;
  PrimeFrame::RectStyleOverride focusStyleOverride{};
  int selectedRow = -1;
  bool showHeaderDividers = true;
  bool showColumnDividers = true;
  bool clipChildren = true;
  bool visible = true;
  TableCallbacks callbacks{};
  std::vector<TableColumn> columns;
  std::vector<std::vector<std::string_view>> rows;
  SizeSpec size;
};

struct TreeNode {
  std::string_view label;
  std::vector<TreeNode> children;
  bool expanded = true;
  bool selected = false;
};

struct TreeViewRowInfo {
  int rowIndex = -1;
  std::span<const uint32_t> path{};
  bool hasChildren = false;
  bool expanded = false;
};

struct TreeViewScrollInfo {
  float offset = 0.0f;
  float maxOffset = 0.0f;
  float progress = 0.0f;
  float viewportHeight = 0.0f;
  float contentHeight = 0.0f;
};

struct TreeViewCallbacks {
  std::function<void(TreeViewRowInfo const&)> onSelectionChanged;
  std::function<void(TreeViewRowInfo const&, bool)> onExpandedChanged;
  std::function<void(TreeViewRowInfo const&)> onActivated;
  std::function<void(int)> onHoverChanged;
  std::function<void(TreeViewScrollInfo const&)> onScrollChanged;
};

struct TreeViewSpec {
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
  float doubleClickMs = 350.0f;
  bool keyboardNavigation = true;
  bool showHeaderDivider = false;
  float headerDividerY = 0.0f;
  bool showConnectors = true;
  bool showCaretMasks = true;
  bool showScrollBar = true;
  bool clipChildren = true;
  bool visible = true;
  PrimeFrame::RectStyleToken rowStyle = 0;
  PrimeFrame::RectStyleToken rowAltStyle = 0;
  PrimeFrame::RectStyleToken hoverStyle = 0;
  PrimeFrame::RectStyleToken selectionStyle = 0;
  PrimeFrame::RectStyleToken selectionAccentStyle = 0;
  PrimeFrame::RectStyleToken caretBackgroundStyle = 0;
  PrimeFrame::RectStyleToken caretLineStyle = 0;
  PrimeFrame::RectStyleToken connectorStyle = 0;
  PrimeFrame::RectStyleToken focusStyle = 0;
  PrimeFrame::RectStyleOverride focusStyleOverride{};
  PrimeFrame::TextStyleToken textStyle = 0;
  PrimeFrame::TextStyleToken selectedTextStyle = 0;
  ScrollBarSpec scrollBar{};
  std::vector<TreeNode> nodes;
  TreeViewCallbacks callbacks{};
  SizeSpec size;
};

struct ScrollViewSpec {
  bool clipChildren = true;
  bool showVertical = true;
  bool showHorizontal = true;
  ScrollAxisSpec vertical{};
  ScrollAxisSpec horizontal{};
  bool visible = true;
  SizeSpec size;
};

void setScrollBarThumbPixels(ScrollBarSpec& spec,
                             float trackHeight,
                             float thumbHeight,
                             float thumbOffset);

float measureTextWidth(PrimeFrame::Frame& frame,
                       PrimeFrame::TextStyleToken token,
                       std::string_view text);

bool textFieldHasSelection(TextFieldState const& state,
                           uint32_t& start,
                           uint32_t& end);
void clearTextFieldSelection(TextFieldState& state, uint32_t cursor);
bool updateTextFieldBlink(TextFieldState& state,
                          std::chrono::steady_clock::time_point now,
                          std::chrono::milliseconds interval);
bool selectableTextHasSelection(SelectableTextState const& state,
                                uint32_t& start,
                                uint32_t& end);
void clearSelectableTextSelection(SelectableTextState& state, uint32_t anchor);

bool appendNodeOnEvent(PrimeFrame::Frame& frame,
                       PrimeFrame::NodeId nodeId,
                       std::function<bool(PrimeFrame::Event const&)> onEvent);
bool appendNodeOnFocus(PrimeFrame::Frame& frame,
                       PrimeFrame::NodeId nodeId,
                       std::function<void()> onFocus);
bool appendNodeOnBlur(PrimeFrame::Frame& frame,
                      PrimeFrame::NodeId nodeId,
                      std::function<void()> onBlur);

class WidgetIdentityReconciler {
public:
  void beginRebuild(PrimeFrame::NodeId focusedNode);
  void registerNode(std::string_view identity, PrimeFrame::NodeId nodeId);
  PrimeFrame::NodeId findNode(std::string_view identity) const;
  bool restoreFocus(PrimeFrame::FocusManager& focus,
                    PrimeFrame::Frame const& frame,
                    PrimeFrame::LayoutOutput const& layout);

private:
  struct Entry {
    std::string identity;
    PrimeFrame::NodeId nodeId{};
  };

  std::vector<Entry> currentEntries_;
  std::optional<std::string> pendingFocusedIdentity_;
};

struct ScrollView;

class UiNode {
public:
  UiNode(PrimeFrame::Frame& frame, PrimeFrame::NodeId id, bool allowAbsolute = false);

  PrimeFrame::NodeId nodeId() const { return id_; }
  PrimeFrame::Frame& frame() const { return frame_.get(); }
  bool allowAbsolute() const { return allowAbsolute_; }

  UiNode& setVisible(bool visible);
  UiNode& setSize(SizeSpec const& size);
  UiNode& setHitTestVisible(bool visible);

  UiNode createVerticalStack(StackSpec const& spec);
  UiNode createHorizontalStack(StackSpec const& spec);
  UiNode createOverlay(StackSpec const& spec);
  template <typename Fn>
  UiNode createVerticalStack(StackSpec const& spec, Fn&& fn) {
    UiNode child = createVerticalStack(spec);
    fn(child);
    return child;
  }
  template <typename Fn>
  UiNode createHorizontalStack(StackSpec const& spec, Fn&& fn) {
    UiNode child = createHorizontalStack(spec);
    fn(child);
    return child;
  }
  template <typename Fn>
  UiNode createOverlay(StackSpec const& spec, Fn&& fn) {
    UiNode child = createOverlay(spec);
    fn(child);
    return child;
  }

  UiNode createPanel(PanelSpec const& spec);
  UiNode createPanel(PrimeFrame::RectStyleToken rectStyle, SizeSpec const& size);
  UiNode createLabel(LabelSpec const& spec);
  UiNode createLabel(std::string_view text,
                     PrimeFrame::TextStyleToken textStyle,
                     SizeSpec const& size);
  UiNode createParagraph(ParagraphSpec const& spec);
  UiNode createParagraph(std::string_view text,
                         PrimeFrame::TextStyleToken textStyle,
                         SizeSpec const& size);
  UiNode createTextSelectionOverlay(TextSelectionOverlaySpec const& spec);
  UiNode createTextLine(TextLineSpec const& spec);
  UiNode createTextLine(std::string_view text,
                        PrimeFrame::TextStyleToken textStyle,
                        SizeSpec const& size,
                        PrimeFrame::TextAlign align = PrimeFrame::TextAlign::Start);
  UiNode createDivider(DividerSpec const& spec);
  UiNode createDivider(PrimeFrame::RectStyleToken rectStyle, SizeSpec const& size);
  UiNode createSpacer(SpacerSpec const& spec);
  UiNode createSpacer(SizeSpec const& size);
  UiNode createButton(ButtonSpec const& spec);
  UiNode createTextField(TextFieldSpec const& spec);
  UiNode createSelectableText(SelectableTextSpec const& spec);
  UiNode createToggle(ToggleSpec const& spec);
  UiNode createCheckbox(CheckboxSpec const& spec);
  UiNode createSlider(SliderSpec const& spec);
  UiNode createTabs(TabsSpec const& spec);
  UiNode createDropdown(DropdownSpec const& spec);
  UiNode createProgressBar(ProgressBarSpec const& spec);
  UiNode createTable(TableSpec const& spec);
  UiNode createTreeView(TreeViewSpec const& spec);
  ScrollView createScrollView(ScrollViewSpec const& spec);

private:
  std::reference_wrapper<PrimeFrame::Frame> frame_;
  PrimeFrame::NodeId id_{};
  bool allowAbsolute_ = false;
};

struct ScrollView {
  UiNode root;
  UiNode content;
};

} // namespace PrimeStage
