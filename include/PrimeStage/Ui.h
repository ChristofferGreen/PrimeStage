#pragma once

#include "PrimeFrame/Frame.h"

#include <optional>
#include <string_view>
#include <vector>

namespace PrimeStage {

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

struct ButtonSpec {
  std::string_view label;
  PrimeFrame::RectStyleToken backgroundStyle = 0;
  PrimeFrame::RectStyleOverride backgroundStyleOverride{};
  PrimeFrame::TextStyleToken textStyle = 0;
  PrimeFrame::TextStyleOverride textStyleOverride{};
  float textInsetX = 16.0f;
  float textOffsetY = 0.0f;
  bool centerText = true;
  bool visible = true;
  SizeSpec size;
};

struct TextFieldSpec {
  std::string_view text;
  std::string_view placeholder;
  float paddingX = 16.0f;
  float textOffsetY = 0.0f;
  PrimeFrame::RectStyleToken backgroundStyle = 0;
  PrimeFrame::RectStyleOverride backgroundStyleOverride{};
  PrimeFrame::TextStyleToken textStyle = 0;
  PrimeFrame::TextStyleOverride textStyleOverride{};
  PrimeFrame::TextStyleToken placeholderStyle = 0;
  PrimeFrame::TextStyleOverride placeholderStyleOverride{};
  bool showPlaceholderWhenEmpty = true;
  bool visible = true;
  SizeSpec size;
};

struct ToggleSpec {
  bool on = false;
  float knobInset = 2.0f;
  PrimeFrame::RectStyleToken trackStyle = 0;
  PrimeFrame::RectStyleOverride trackStyleOverride{};
  PrimeFrame::RectStyleToken knobStyle = 0;
  PrimeFrame::RectStyleOverride knobStyleOverride{};
  bool visible = true;
  SizeSpec size;
};

struct CheckboxSpec {
  std::string_view label;
  bool checked = false;
  float boxSize = 16.0f;
  float checkInset = 3.0f;
  float gap = 8.0f;
  PrimeFrame::RectStyleToken boxStyle = 0;
  PrimeFrame::RectStyleOverride boxStyleOverride{};
  PrimeFrame::RectStyleToken checkStyle = 0;
  PrimeFrame::RectStyleOverride checkStyleOverride{};
  PrimeFrame::TextStyleToken textStyle = 0;
  PrimeFrame::TextStyleOverride textStyleOverride{};
  bool visible = true;
  SizeSpec size;
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
  PrimeFrame::RectStyleToken thumbStyle = 0;
  PrimeFrame::RectStyleOverride thumbStyleOverride{};
  bool visible = true;
  SizeSpec size;
};

struct TabsSpec {
  std::vector<std::string_view> labels;
  int selectedIndex = 0;
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

struct DropdownSpec {
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
  float inset = 10.0f;
  float padding = 8.0f;
  float width = 6.0f;
  float minThumbHeight = 16.0f;
  float thumbFraction = 0.18f;
  float thumbProgress = 0.1f;
  PrimeFrame::RectStyleToken trackStyle = 0;
  PrimeFrame::RectStyleToken thumbStyle = 0;
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

struct ScrollView;

class UiNode {
public:
  UiNode(PrimeFrame::Frame& frame, PrimeFrame::NodeId id, bool allowAbsolute = false);

  PrimeFrame::NodeId nodeId() const { return id_; }
  PrimeFrame::Frame& frame() const { return frame_.get(); }
  bool allowAbsolute() const { return allowAbsolute_; }

  UiNode& setVisible(bool visible);
  UiNode& setSize(SizeSpec const& size);

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
  UiNode createToggle(ToggleSpec const& spec);
  UiNode createCheckbox(CheckboxSpec const& spec);
  UiNode createSlider(SliderSpec const& spec);
  UiNode createTabs(TabsSpec const& spec);
  UiNode createDropdown(DropdownSpec const& spec);
  UiNode createProgressBar(ProgressBarSpec const& spec);
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
