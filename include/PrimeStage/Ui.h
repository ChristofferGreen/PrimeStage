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

enum class KeyCode : uint32_t {
  A = 0x04u,
  C = 0x06u,
  V = 0x19u,
  X = 0x1Bu,
  Enter = 0x28u,
  Escape = 0x29u,
  Backspace = 0x2Au,
  Space = 0x2Cu,
  Delete = 0x4Cu,
  Right = 0x4Fu,
  Left = 0x50u,
  Down = 0x51u,
  Up = 0x52u,
  Home = 0x4Au,
  End = 0x4Du,
  PageUp = 0x4Bu,
  PageDown = 0x4Eu,
};

constexpr uint32_t keyCodeValue(KeyCode key) {
  return static_cast<uint32_t>(key);
}

constexpr int keyCodeInt(KeyCode key) {
  return static_cast<int>(keyCodeValue(key));
}

enum class AccessibilityRole : uint8_t {
  Unspecified,
  Group,
  StaticText,
  Button,
  TextField,
  Toggle,
  Checkbox,
  Slider,
  TabList,
  Tab,
  ComboBox,
  ProgressBar,
  Table,
  Tree,
  TreeItem,
};

struct AccessibilityState {
  bool disabled = false;
  std::optional<bool> checked;
  std::optional<bool> selected;
  std::optional<bool> expanded;
  std::optional<float> valueNow;
  std::optional<float> valueMin;
  std::optional<float> valueMax;
  std::optional<int> level;
  std::optional<int> positionInSet;
  std::optional<int> setSize;
};

struct AccessibilitySemantics {
  AccessibilityRole role = AccessibilityRole::Unspecified;
  std::string_view label;
  std::string_view description;
  std::string_view valueText;
  AccessibilityState state{};
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
  AccessibilitySemantics accessibility{};
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
  AccessibilitySemantics accessibility{};
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
  AccessibilitySemantics accessibility{};
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
  AccessibilitySemantics accessibility{};
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
  bool enabled = true;
  int tabIndex = -1;
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

struct TextCompositionState {
  bool active = false;
  std::string text;
  uint32_t replacementStart = 0u;
  uint32_t replacementEnd = 0u;
};

struct TextCompositionCallbacks {
  std::function<void()> onCompositionStart;
  std::function<void(std::string_view, uint32_t, uint32_t)> onCompositionUpdate;
  std::function<void(std::string_view)> onCompositionCommit;
  std::function<void()> onCompositionCancel;
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
  TextCompositionState* compositionState = nullptr;
  TextFieldCallbacks callbacks{};
  TextCompositionCallbacks compositionCallbacks{};
  TextFieldClipboard clipboard{};
  AccessibilitySemantics accessibility{};
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
  bool enabled = true;
  int tabIndex = -1;
  bool readOnly = false;
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
  AccessibilitySemantics accessibility{};
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
  bool enabled = true;
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

struct ToggleState {
  bool on = false;
};

struct ToggleSpec {
  ToggleState* state = nullptr;
  AccessibilitySemantics accessibility{};
  bool on = false;
  bool enabled = true;
  int tabIndex = -1;
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

struct CheckboxState {
  bool checked = false;
};

struct CheckboxSpec {
  CheckboxState* state = nullptr;
  AccessibilitySemantics accessibility{};
  std::string_view label;
  bool checked = false;
  bool enabled = true;
  int tabIndex = -1;
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
  AccessibilitySemantics accessibility{};
  float value = 0.0f;
  bool enabled = true;
  int tabIndex = -1;
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

struct TabsState {
  int selectedIndex = 0;
};

struct TabsSpec {
  TabsState* state = nullptr;
  AccessibilitySemantics accessibility{};
  std::vector<std::string_view> labels;
  int selectedIndex = 0;
  bool enabled = true;
  int tabIndex = -1;
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

struct DropdownState {
  int selectedIndex = 0;
};

struct DropdownSpec {
  DropdownState* state = nullptr;
  AccessibilitySemantics accessibility{};
  std::vector<std::string_view> options;
  int selectedIndex = 0;
  bool enabled = true;
  int tabIndex = -1;
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

struct ProgressBarCallbacks {
  std::function<void(float)> onValueChanged;
};

struct ProgressBarState {
  float value = 0.0f;
};

struct ProgressBarSpec {
  ProgressBarState* state = nullptr;
  ProgressBarCallbacks callbacks{};
  AccessibilitySemantics accessibility{};
  float value = 0.0f;
  float minFillWidth = 0.0f;
  bool enabled = true;
  int tabIndex = -1;
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
  AccessibilitySemantics accessibility{};
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
  bool enabled = true;
  int tabIndex = -1;
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
  AccessibilitySemantics accessibility{};
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
  bool enabled = true;
  int tabIndex = -1;
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

struct WindowCallbacks {
  std::function<void()> onFocusRequested;
  std::function<void(bool)> onFocusChanged;
  std::function<void()> onMoveStarted;
  std::function<void(float, float)> onMoved;
  std::function<void()> onMoveEnded;
  std::function<void()> onResizeStarted;
  std::function<void(float, float)> onResized;
  std::function<void()> onResizeEnded;
};

struct WindowSpec {
  AccessibilitySemantics accessibility{};
  std::string_view title;
  float positionX = 0.0f;
  float positionY = 0.0f;
  float width = 360.0f;
  float height = 240.0f;
  float minWidth = 160.0f;
  float minHeight = 120.0f;
  float titleBarHeight = 30.0f;
  float contentPadding = 10.0f;
  float resizeHandleSize = 14.0f;
  bool movable = true;
  bool resizable = true;
  bool focusable = true;
  int tabIndex = -1;
  bool visible = true;
  WindowCallbacks callbacks{};
  PrimeFrame::RectStyleToken frameStyle = 0;
  PrimeFrame::RectStyleOverride frameStyleOverride{};
  PrimeFrame::RectStyleToken titleBarStyle = 0;
  PrimeFrame::RectStyleOverride titleBarStyleOverride{};
  PrimeFrame::TextStyleToken titleTextStyle = 0;
  PrimeFrame::TextStyleOverride titleTextStyleOverride{};
  PrimeFrame::RectStyleToken contentStyle = 0;
  PrimeFrame::RectStyleOverride contentStyleOverride{};
  PrimeFrame::RectStyleToken resizeHandleStyle = 0;
  PrimeFrame::RectStyleOverride resizeHandleStyleOverride{};
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

// Callback composition helpers execute on the caller/dispatch thread and do not
// provide cross-thread synchronization. Direct reentrant invocation of the same
// composed callback chain is suppressed at runtime to avoid recursive callback
// loops.
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
struct Window;

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
  Window createWindow(WindowSpec const& spec);

private:
  std::reference_wrapper<PrimeFrame::Frame> frame_;
  PrimeFrame::NodeId id_{};
  bool allowAbsolute_ = false;
};

struct ScrollView {
  UiNode root;
  UiNode content;
};

struct Window {
  UiNode root;
  UiNode titleBar;
  UiNode content;
  PrimeFrame::NodeId resizeHandleId{};
};

} // namespace PrimeStage
