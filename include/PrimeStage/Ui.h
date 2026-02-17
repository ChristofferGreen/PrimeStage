#pragma once

#include "PrimeFrame/Frame.h"

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace PrimeStage {

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
  PrimeFrame::TextStyleToken textStyle = 0;
  PrimeFrame::TextStyleOverride textStyleOverride{};
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
  PrimeFrame::TextStyleToken textStyle = 0;
  PrimeFrame::TextStyleOverride textStyleOverride{};
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

struct ButtonSpec {
  Bounds bounds;
  std::string label;
  PrimeFrame::RectStyleToken backgroundStyle = 0;
  PrimeFrame::RectStyleOverride backgroundStyleOverride{};
  PrimeFrame::TextStyleToken textStyle = 0;
  PrimeFrame::TextStyleOverride textStyleOverride{};
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
  PrimeFrame::RectStyleToken backgroundStyle = 0;
  PrimeFrame::RectStyleOverride backgroundStyleOverride{};
  PrimeFrame::TextStyleToken textStyle = 0;
  PrimeFrame::TextStyleOverride textStyleOverride{};
  PrimeFrame::TextStyleToken placeholderStyle = 0;
  PrimeFrame::TextStyleOverride placeholderStyleOverride{};
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
  Bounds bounds;
  bool clipChildren = true;
  bool showVertical = true;
  bool showHorizontal = true;
  ScrollAxisSpec vertical{};
  ScrollAxisSpec horizontal{};
};

Bounds insetBounds(Bounds const& bounds, float inset);
Bounds insetBounds(Bounds const& bounds, float insetX, float insetY);
Bounds insetBounds(Bounds const& bounds, float left, float top, float right, float bottom);
Bounds alignBottomRight(Bounds const& bounds, float width, float height, float insetX, float insetY);
Bounds alignCenterY(Bounds const& bounds, float height);
void setScrollBarThumbPixels(ScrollBarSpec& spec,
                             float trackHeight,
                             float thumbHeight,
                             float thumbOffset);

#ifdef PRIMESTAGE_STUDIO_UI
#include "PrimeStage/StudioUiTypes.h"
struct SectionPanel;
struct ShellLayout;
#endif

class UiNode {
public:
  UiNode(PrimeFrame::Frame& frame, PrimeFrame::NodeId id);

  PrimeFrame::NodeId nodeId() const { return id_; }
  PrimeFrame::Frame& frame() const { return frame_.get(); }

  UiNode createPanel(PanelSpec const& spec);
  UiNode createPanel(PrimeFrame::RectStyleToken rectStyle, Bounds const& bounds);
  UiNode createLabel(LabelSpec const& spec);
  UiNode createLabel(std::string_view text,
                     PrimeFrame::TextStyleToken textStyle,
                     Bounds const& bounds);
  UiNode createParagraph(ParagraphSpec const& spec);
  UiNode createParagraph(Bounds const& bounds,
                         std::string_view text,
                         PrimeFrame::TextStyleToken textStyle);
  UiNode createTextLine(TextLineSpec const& spec);
  UiNode createTextLine(Bounds const& bounds,
                        std::string_view text,
                        PrimeFrame::TextStyleToken textStyle,
                        PrimeFrame::TextAlign align = PrimeFrame::TextAlign::Start);
  UiNode createDivider(DividerSpec const& spec);
  UiNode createSpacer(SpacerSpec const& spec);
  UiNode createButton(ButtonSpec const& spec);
  UiNode createTextField(TextFieldSpec const& spec);
  UiNode createScrollView(ScrollViewSpec const& spec);

#ifdef PRIMESTAGE_STUDIO_UI
#include "PrimeStage/StudioUiMethods.inl"
#endif

private:
  std::reference_wrapper<PrimeFrame::Frame> frame_;
  PrimeFrame::NodeId id_{};
};

#ifdef PRIMESTAGE_STUDIO_UI
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
ShellSpec makeShellSpec(Bounds const& bounds);
#endif

} // namespace PrimeStage
