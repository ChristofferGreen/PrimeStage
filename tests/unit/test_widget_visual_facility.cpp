#include "PrimeStage/Render.h"
#include "PrimeStage/Ui.h"

#include "PrimeFrame/Events.h"
#include "PrimeFrame/Focus.h"
#include "PrimeFrame/Frame.h"
#include "PrimeFrame/Layout.h"

#include "third_party/doctest.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

constexpr float CanvasSize = 256.0f;
constexpr uint32_t CanvasPixels = 256u;

constexpr PrimeFrame::ColorToken ColorSurface = 1u;
constexpr PrimeFrame::ColorToken ColorSurfaceAlt = 2u;
constexpr PrimeFrame::ColorToken ColorHover = 3u;
constexpr PrimeFrame::ColorToken ColorPressed = 4u;
constexpr PrimeFrame::ColorToken ColorFocus = 5u;
constexpr PrimeFrame::ColorToken ColorSelection = 6u;
constexpr PrimeFrame::ColorToken ColorTrack = 7u;
constexpr PrimeFrame::ColorToken ColorFill = 8u;
constexpr PrimeFrame::ColorToken ColorKnob = 9u;
constexpr PrimeFrame::ColorToken ColorTextPrimary = 10u;
constexpr PrimeFrame::ColorToken ColorTextAccent = 11u;

constexpr PrimeFrame::RectStyleToken StyleSurface = 1u;
constexpr PrimeFrame::RectStyleToken StyleSurfaceAlt = 2u;
constexpr PrimeFrame::RectStyleToken StyleHover = 3u;
constexpr PrimeFrame::RectStyleToken StylePressed = 4u;
constexpr PrimeFrame::RectStyleToken StyleFocus = 5u;
constexpr PrimeFrame::RectStyleToken StyleSelection = 6u;
constexpr PrimeFrame::RectStyleToken StyleTrack = 7u;
constexpr PrimeFrame::RectStyleToken StyleFill = 8u;
constexpr PrimeFrame::RectStyleToken StyleKnob = 9u;
constexpr PrimeFrame::RectStyleToken StyleDivider = 10u;
constexpr PrimeFrame::RectStyleToken StyleWindowFrame = 11u;
constexpr PrimeFrame::RectStyleToken StyleWindowTitle = 12u;
constexpr PrimeFrame::RectStyleToken StyleWindowContent = 13u;
constexpr PrimeFrame::RectStyleToken StyleWindowResize = 14u;

PrimeFrame::Color makeColor(float r, float g, float b) {
  PrimeFrame::Color color;
  color.r = r;
  color.g = g;
  color.b = b;
  color.a = 1.0f;
  return color;
}

void configureVisualTheme(PrimeFrame::Frame& frame) {
  PrimeFrame::Theme* theme = frame.getTheme(PrimeFrame::DefaultThemeId);
  REQUIRE(theme != nullptr);

  theme->palette.assign(16u, PrimeFrame::Color{});
  theme->palette[ColorSurface] = makeColor(0.15f, 0.17f, 0.20f);
  theme->palette[ColorSurfaceAlt] = makeColor(0.22f, 0.25f, 0.29f);
  theme->palette[ColorHover] = makeColor(0.19f, 0.45f, 0.78f);
  theme->palette[ColorPressed] = makeColor(0.13f, 0.31f, 0.57f);
  theme->palette[ColorFocus] = makeColor(0.89f, 0.32f, 0.16f);
  theme->palette[ColorSelection] = makeColor(0.10f, 0.64f, 0.24f);
  theme->palette[ColorTrack] = makeColor(0.31f, 0.34f, 0.38f);
  theme->palette[ColorFill] = makeColor(0.20f, 0.58f, 0.90f);
  theme->palette[ColorKnob] = makeColor(0.95f, 0.96f, 0.98f);
  theme->palette[ColorTextPrimary] = makeColor(0.95f, 0.97f, 0.99f);
  theme->palette[ColorTextAccent] = makeColor(0.99f, 0.88f, 0.70f);

  theme->rectStyles.assign(16u, PrimeFrame::RectStyle{});
  theme->rectStyles[StyleSurface].fill = ColorSurface;
  theme->rectStyles[StyleSurfaceAlt].fill = ColorSurfaceAlt;
  theme->rectStyles[StyleHover].fill = ColorHover;
  theme->rectStyles[StylePressed].fill = ColorPressed;
  theme->rectStyles[StyleFocus].fill = ColorFocus;
  theme->rectStyles[StyleSelection].fill = ColorSelection;
  theme->rectStyles[StyleTrack].fill = ColorTrack;
  theme->rectStyles[StyleFill].fill = ColorFill;
  theme->rectStyles[StyleKnob].fill = ColorKnob;
  theme->rectStyles[StyleDivider].fill = ColorSurfaceAlt;
  theme->rectStyles[StyleWindowFrame].fill = ColorSurface;
  theme->rectStyles[StyleWindowTitle].fill = ColorSurfaceAlt;
  theme->rectStyles[StyleWindowContent].fill = ColorTrack;
  theme->rectStyles[StyleWindowResize].fill = ColorHover;

  theme->textStyles.assign(4u, PrimeFrame::TextStyle{});
  theme->textStyles[0].color = ColorTextPrimary;
  theme->textStyles[1].color = ColorTextAccent;
  theme->textStyles[2].color = ColorTextPrimary;
  theme->textStyles[3].color = ColorTextAccent;
}

PrimeStage::UiNode createCanvasRoot(PrimeFrame::Frame& frame) {
  PrimeFrame::NodeId rootId = frame.createNode();
  frame.addRoot(rootId);
  PrimeFrame::Node* rootNode = frame.getNode(rootId);
  REQUIRE(rootNode != nullptr);
  rootNode->layout = PrimeFrame::LayoutType::Overlay;
  rootNode->sizeHint.width.preferred = CanvasSize;
  rootNode->sizeHint.height.preferred = CanvasSize;
  return PrimeStage::UiNode(frame, rootId, true);
}

PrimeFrame::LayoutOutput layoutCanvas(PrimeFrame::Frame& frame) {
  PrimeFrame::LayoutOutput layout;
  PrimeFrame::LayoutEngine engine;
  PrimeFrame::LayoutOptions options;
  options.rootWidth = CanvasSize;
  options.rootHeight = CanvasSize;
  engine.layout(frame, layout, options);
  return layout;
}

void centerWidgetInCanvas(PrimeFrame::Frame& frame,
                          PrimeFrame::NodeId rootId,
                          PrimeFrame::NodeId widgetId) {
  PrimeFrame::LayoutOutput layout = layoutCanvas(frame);
  PrimeFrame::LayoutOut const* rootOut = layout.get(rootId);
  PrimeFrame::LayoutOut const* widgetOut = layout.get(widgetId);
  REQUIRE(rootOut != nullptr);
  REQUIRE(widgetOut != nullptr);

  PrimeFrame::Node* widgetNode = frame.getNode(widgetId);
  REQUIRE(widgetNode != nullptr);
  widgetNode->localX = (rootOut->absW - widgetOut->absW) * 0.5f;
  widgetNode->localY = (rootOut->absH - widgetOut->absH) * 0.5f;
}

std::string slugify(std::string_view text) {
  std::string out;
  out.reserve(text.size());
  bool lastUnderscore = false;
  for (char ch : text) {
    unsigned char uch = static_cast<unsigned char>(ch);
    if (std::isalnum(uch)) {
      out.push_back(static_cast<char>(std::tolower(uch)));
      lastUnderscore = false;
      continue;
    }
    if (!lastUnderscore) {
      out.push_back('_');
      lastUnderscore = true;
    }
  }
  while (!out.empty() && out.back() == '_') {
    out.pop_back();
  }
  if (out.empty()) {
    out = "scenario";
  }
  return out;
}

enum class InputStepType : uint8_t {
  PointerMove,
  PointerDown,
  PointerUp,
  PointerDrag,
  PointerScroll,
  KeyDown,
  TextInput,
  FocusWidget,
  TabForward,
  TabBackward,
};

struct InputStep {
  InputStepType type = InputStepType::PointerMove;
  float x = 0.5f;
  float y = 0.5f;
  float scrollX = 0.0f;
  float scrollY = 0.0f;
  PrimeStage::KeyCode key = PrimeStage::KeyCode::Enter;
  std::string text;
};

std::string_view keyCodeName(PrimeStage::KeyCode key) {
  switch (key) {
    case PrimeStage::KeyCode::A:
      return "A";
    case PrimeStage::KeyCode::C:
      return "C";
    case PrimeStage::KeyCode::V:
      return "V";
    case PrimeStage::KeyCode::X:
      return "X";
    case PrimeStage::KeyCode::Enter:
      return "Enter";
    case PrimeStage::KeyCode::Escape:
      return "Escape";
    case PrimeStage::KeyCode::Backspace:
      return "Backspace";
    case PrimeStage::KeyCode::Space:
      return "Space";
    case PrimeStage::KeyCode::Delete:
      return "Delete";
    case PrimeStage::KeyCode::Right:
      return "Right";
    case PrimeStage::KeyCode::Left:
      return "Left";
    case PrimeStage::KeyCode::Down:
      return "Down";
    case PrimeStage::KeyCode::Up:
      return "Up";
    case PrimeStage::KeyCode::Home:
      return "Home";
    case PrimeStage::KeyCode::End:
      return "End";
    case PrimeStage::KeyCode::PageUp:
      return "PageUp";
    case PrimeStage::KeyCode::PageDown:
      return "PageDown";
  }
  return "Unknown";
}

std::string inputStepSummary(InputStep const& step) {
  std::ostringstream out;
  switch (step.type) {
    case InputStepType::PointerMove:
      out << "pointer_move(" << step.x << ", " << step.y << ")";
      break;
    case InputStepType::PointerDown:
      out << "pointer_down(" << step.x << ", " << step.y << ")";
      break;
    case InputStepType::PointerUp:
      out << "pointer_up(" << step.x << ", " << step.y << ")";
      break;
    case InputStepType::PointerDrag:
      out << "pointer_drag(" << step.x << ", " << step.y << ")";
      break;
    case InputStepType::PointerScroll:
      out << "pointer_scroll(" << step.x << ", " << step.y << ", " << step.scrollX << ", "
          << step.scrollY << ")";
      break;
    case InputStepType::KeyDown:
      out << "key_down(" << keyCodeName(step.key) << ")";
      break;
    case InputStepType::TextInput:
      out << "text_input(\"" << step.text << "\")";
      break;
    case InputStepType::FocusWidget:
      out << "focus_widget";
      break;
    case InputStepType::TabForward:
      out << "tab_forward";
      break;
    case InputStepType::TabBackward:
      out << "tab_backward";
      break;
  }
  return out.str();
}

PrimeFrame::Event makePointerEvent(InputStepType type,
                                   int pointerId,
                                   float x,
                                   float y,
                                   float scrollX,
                                   float scrollY) {
  PrimeFrame::Event event;
  switch (type) {
    case InputStepType::PointerMove:
      event.type = PrimeFrame::EventType::PointerMove;
      break;
    case InputStepType::PointerDown:
      event.type = PrimeFrame::EventType::PointerDown;
      break;
    case InputStepType::PointerUp:
      event.type = PrimeFrame::EventType::PointerUp;
      break;
    case InputStepType::PointerDrag:
      event.type = PrimeFrame::EventType::PointerDrag;
      break;
    case InputStepType::PointerScroll:
      event.type = PrimeFrame::EventType::PointerScroll;
      break;
    default:
      event.type = PrimeFrame::EventType::PointerMove;
      break;
  }
  event.pointerId = pointerId;
  event.x = x;
  event.y = y;
  event.scrollX = scrollX;
  event.scrollY = scrollY;
  return event;
}

void replayInputScript(std::vector<InputStep> const& script,
                       PrimeFrame::Frame& frame,
                       PrimeFrame::LayoutOutput const& layout,
                       PrimeFrame::NodeId rootId,
                       PrimeFrame::NodeId widgetId,
                       PrimeFrame::EventRouter& router,
                       PrimeFrame::FocusManager& focus) {
  PrimeFrame::LayoutOut const* widgetOut = layout.get(widgetId);
  REQUIRE(widgetOut != nullptr);
  focus.setActiveRoot(frame, layout, rootId);

  auto resolvePointer = [&](InputStep const& step) {
    float px = widgetOut->absX + widgetOut->absW * std::clamp(step.x, 0.0f, 1.0f);
    float py = widgetOut->absY + widgetOut->absH * std::clamp(step.y, 0.0f, 1.0f);
    return std::pair<float, float>{px, py};
  };

  constexpr int PointerId = 1;
  for (InputStep const& step : script) {
    if (step.type == InputStepType::FocusWidget) {
      (void)focus.setFocus(frame, layout, widgetId);
      continue;
    }
    if (step.type == InputStepType::TabForward) {
      (void)focus.handleTab(frame, layout, true);
      continue;
    }
    if (step.type == InputStepType::TabBackward) {
      (void)focus.handleTab(frame, layout, false);
      continue;
    }

    if (step.type == InputStepType::KeyDown) {
      PrimeFrame::Event event;
      event.type = PrimeFrame::EventType::KeyDown;
      event.key = PrimeStage::keyCodeInt(step.key);
      router.dispatch(event, frame, layout, &focus);
      continue;
    }

    if (step.type == InputStepType::TextInput) {
      PrimeFrame::Event event;
      event.type = PrimeFrame::EventType::TextInput;
      event.text = step.text;
      router.dispatch(event, frame, layout, &focus);
      continue;
    }

    auto [px, py] = resolvePointer(step);
    PrimeFrame::Event event =
        makePointerEvent(step.type, PointerId, px, py, step.scrollX, step.scrollY);
    router.dispatch(event, frame, layout, &focus);
  }
}

struct WidgetVisualScenario {
  std::string widget;
  std::string name;
  std::string summary;
  std::vector<InputStep> inputScript;
  std::function<PrimeFrame::NodeId(PrimeFrame::Frame&, PrimeStage::UiNode&)> buildWidget;
  std::function<void()> resetState{};
  bool rebuildAfterInput = false;
};

std::vector<WidgetVisualScenario> buildScenarioCatalog() {
  std::vector<WidgetVisualScenario> scenarios;
  static PrimeStage::SelectableTextState selectableState;
  static PrimeStage::DropdownState dropdownState;
  static int listSelectedIndex = 1;
  static int treeSelectedRow = 0;
  static PrimeStage::TextFieldState focusFieldState;
  static PrimeStage::TabsState focusTabsState;
  static PrimeStage::DropdownState focusDropdownState;
  static PrimeStage::ProgressBarState focusProgressState;

  scenarios.push_back(WidgetVisualScenario{
      "stack",
      "overlay_baseline",
      "Container baseline without direct input.",
      {},
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::StackSpec spec;
        spec.size.preferredWidth = 200.0f;
        spec.size.preferredHeight = 120.0f;
        spec.padding = PrimeFrame::Insets{8.0f, 8.0f, 8.0f, 8.0f};
        spec.gap = 6.0f;
        PrimeStage::UiNode stack = root.createVerticalStack(spec);

        PrimeStage::PanelSpec panel;
        panel.rectStyle = StyleSurfaceAlt;
        panel.size.preferredWidth = 160.0f;
        panel.size.preferredHeight = 36.0f;
        stack.createPanel(panel);

        PrimeStage::DividerSpec divider;
        divider.rectStyle = StyleDivider;
        divider.size.preferredWidth = 160.0f;
        divider.size.preferredHeight = 2.0f;
        stack.createDivider(divider);
        return stack.nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "panel",
      "panel_baseline",
      "Simple panel baseline.",
      {},
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::PanelSpec spec;
        spec.rectStyle = StyleSurface;
        spec.size.preferredWidth = 180.0f;
        spec.size.preferredHeight = 120.0f;
        return root.createPanel(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "label",
      "label_baseline",
      "Single-line text rendering.",
      {},
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::LabelSpec spec;
        spec.text = "Label widget";
        spec.textStyle = 0u;
        spec.size.preferredWidth = 200.0f;
        spec.size.preferredHeight = 26.0f;
        return root.createLabel(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "paragraph",
      "paragraph_wrap",
      "Wrapped paragraph baseline.",
      {},
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::ParagraphSpec spec;
        spec.text = "Paragraph widget with wrapping to verify visual composition.";
        spec.textStyle = 0u;
        spec.maxWidth = 170.0f;
        spec.size.preferredWidth = 170.0f;
        spec.size.preferredHeight = 72.0f;
        return root.createParagraph(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "text_line",
      "line_baseline",
      "Single line text node.",
      {},
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::TextLineSpec spec;
        spec.text = "TextLine";
        spec.textStyle = 1u;
        spec.size.preferredWidth = 160.0f;
        spec.size.preferredHeight = 24.0f;
        return root.createTextLine(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "divider",
      "divider_baseline",
      "Divider rendering.",
      {},
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::DividerSpec spec;
        spec.rectStyle = StyleDivider;
        spec.size.preferredWidth = 180.0f;
        spec.size.preferredHeight = 3.0f;
        return root.createDivider(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "spacer",
      "spacer_baseline",
      "Spacer dimensions.",
      {},
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::SpacerSpec spec;
        spec.size.preferredWidth = 100.0f;
        spec.size.preferredHeight = 50.0f;
        return root.createSpacer(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "focus_navigation",
      "tab_forward_cycle",
      "Tab moves focus across multiple focusable widgets.",
      {
          {InputStepType::TabForward},
          {InputStepType::TabForward},
          {InputStepType::TabForward},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::StackSpec rowSpec;
        rowSpec.gap = 10.0f;
        rowSpec.size.preferredWidth = 220.0f;
        rowSpec.size.preferredHeight = 38.0f;
        PrimeStage::UiNode row = root.createHorizontalStack(rowSpec);

        PrimeStage::ButtonSpec buttonSpec;
        buttonSpec.label = "One";
        buttonSpec.backgroundStyle = StyleSurface;
        buttonSpec.hoverStyle = StyleHover;
        buttonSpec.pressedStyle = StylePressed;
        buttonSpec.focusStyle = StyleFocus;
        buttonSpec.textStyle = 0u;
        buttonSpec.size.preferredWidth = 62.0f;
        buttonSpec.size.preferredHeight = 34.0f;
        row.createButton(buttonSpec);

        PrimeStage::ButtonSpec buttonSpec2 = buttonSpec;
        buttonSpec2.label = "Two";
        row.createButton(buttonSpec2);

        PrimeStage::ButtonSpec buttonSpec3 = buttonSpec;
        buttonSpec3.label = "Three";
        row.createButton(buttonSpec3);

        return row.nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "focus_navigation",
      "tab_backward_cycle",
      "Shift+Tab style reverse movement across multiple focusable widgets.",
      {
          {InputStepType::TabForward},
          {InputStepType::TabForward},
          {InputStepType::TabForward},
          {InputStepType::TabBackward},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::StackSpec rowSpec;
        rowSpec.gap = 10.0f;
        rowSpec.size.preferredWidth = 220.0f;
        rowSpec.size.preferredHeight = 38.0f;
        PrimeStage::UiNode row = root.createHorizontalStack(rowSpec);

        PrimeStage::ButtonSpec buttonSpec;
        buttonSpec.label = "One";
        buttonSpec.backgroundStyle = StyleSurface;
        buttonSpec.hoverStyle = StyleHover;
        buttonSpec.pressedStyle = StylePressed;
        buttonSpec.focusStyle = StyleFocus;
        buttonSpec.textStyle = 0u;
        buttonSpec.size.preferredWidth = 62.0f;
        buttonSpec.size.preferredHeight = 34.0f;
        row.createButton(buttonSpec);

        PrimeStage::ButtonSpec buttonSpec2 = buttonSpec;
        buttonSpec2.label = "Two";
        row.createButton(buttonSpec2);

        PrimeStage::ButtonSpec buttonSpec3 = buttonSpec;
        buttonSpec3.label = "Three";
        row.createButton(buttonSpec3);

        return row.nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "button",
      "focus_click",
      "Click focuses button.",
      {
          {InputStepType::PointerDown, 0.5f, 0.5f},
          {InputStepType::PointerUp, 0.5f, 0.5f},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::ButtonSpec spec;
        spec.label = "Focus";
        spec.backgroundStyle = StyleSurface;
        spec.hoverStyle = StyleHover;
        spec.pressedStyle = StylePressed;
        spec.focusStyle = StyleFocus;
        spec.textStyle = 0u;
        spec.size.preferredWidth = 150.0f;
        spec.size.preferredHeight = 34.0f;
        return root.createButton(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "text_field",
      "focus_click",
      "Click focuses text field.",
      {
          {InputStepType::PointerDown, 0.35f, 0.5f},
          {InputStepType::PointerUp, 0.35f, 0.5f},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        focusFieldState = PrimeStage::TextFieldState{};
        focusFieldState.text = "Focus";
        focusFieldState.cursor = static_cast<uint32_t>(focusFieldState.text.size());
        PrimeStage::TextFieldSpec spec;
        spec.state = &focusFieldState;
        spec.backgroundStyle = StyleSurface;
        spec.focusStyle = StyleFocus;
        spec.selectionStyle = StyleSelection;
        spec.cursorStyle = StyleKnob;
        spec.textStyle = 0u;
        spec.showCursor = true;
        spec.size.preferredWidth = 180.0f;
        spec.size.preferredHeight = 34.0f;
        return root.createTextField(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "toggle",
      "focus_click",
      "Click focuses toggle.",
      {
          {InputStepType::PointerDown, 0.5f, 0.5f},
          {InputStepType::PointerUp, 0.5f, 0.5f},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::ToggleSpec spec;
        spec.trackStyle = StyleTrack;
        spec.knobStyle = StyleKnob;
        spec.focusStyle = StyleFocus;
        spec.size.preferredWidth = 72.0f;
        spec.size.preferredHeight = 30.0f;
        return root.createToggle(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "checkbox",
      "focus_click",
      "Click focuses checkbox.",
      {
          {InputStepType::PointerDown, 0.15f, 0.5f},
          {InputStepType::PointerUp, 0.15f, 0.5f},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::CheckboxSpec spec;
        spec.label = "Check";
        spec.boxStyle = StyleTrack;
        spec.checkStyle = StyleSelection;
        spec.focusStyle = StyleFocus;
        spec.textStyle = 0u;
        spec.size.preferredWidth = 160.0f;
        spec.size.preferredHeight = 28.0f;
        return root.createCheckbox(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "slider",
      "focus_click",
      "Click focuses slider.",
      {
          {InputStepType::PointerDown, 0.5f, 0.5f},
          {InputStepType::PointerUp, 0.5f, 0.5f},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::SliderSpec spec;
        spec.value = 0.4f;
        spec.trackStyle = StyleTrack;
        spec.fillStyle = StyleFill;
        spec.thumbStyle = StyleKnob;
        spec.focusStyle = StyleFocus;
        spec.size.preferredWidth = 200.0f;
        spec.size.preferredHeight = 30.0f;
        return root.createSlider(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "tabs",
      "focus_click",
      "Click focuses tab widget.",
      {
          {InputStepType::PointerDown, 0.18f, 0.5f},
          {InputStepType::PointerUp, 0.18f, 0.5f},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        focusTabsState = PrimeStage::TabsState{};
        focusTabsState.selectedIndex = 0;
        PrimeStage::TabsSpec spec;
        spec.state = &focusTabsState;
        spec.labels = {"One", "Two", "Three"};
        spec.tabStyle = StyleSurface;
        spec.activeTabStyle = StyleHover;
        spec.textStyle = 0u;
        spec.activeTextStyle = 1u;
        spec.size.preferredWidth = 210.0f;
        spec.size.preferredHeight = 32.0f;
        return root.createTabs(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "dropdown",
      "focus_click",
      "Click focuses dropdown.",
      {
          {InputStepType::PointerDown, 0.5f, 0.5f},
          {InputStepType::PointerUp, 0.5f, 0.5f},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        focusDropdownState = PrimeStage::DropdownState{};
        focusDropdownState.selectedIndex = 0;
        PrimeStage::DropdownSpec spec;
        spec.state = &focusDropdownState;
        spec.options = {"Preview", "Edit", "Export"};
        spec.backgroundStyle = StyleSurface;
        spec.textStyle = 0u;
        spec.indicatorStyle = 1u;
        spec.focusStyle = StyleFocus;
        spec.size.preferredWidth = 180.0f;
        spec.size.preferredHeight = 32.0f;
        return root.createDropdown(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "progress_bar",
      "focus_click",
      "Click focuses progress bar.",
      {
          {InputStepType::PointerDown, 0.5f, 0.5f},
          {InputStepType::PointerUp, 0.5f, 0.5f},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        focusProgressState = PrimeStage::ProgressBarState{};
        focusProgressState.value = 0.4f;
        PrimeStage::ProgressBarSpec spec;
        spec.state = &focusProgressState;
        spec.trackStyle = StyleTrack;
        spec.fillStyle = StyleFill;
        spec.focusStyle = StyleFocus;
        spec.size.preferredWidth = 200.0f;
        spec.size.preferredHeight = 16.0f;
        return root.createProgressBar(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "table",
      "focus_click",
      "Click focuses table.",
      {
          {InputStepType::PointerDown, 0.5f, 0.42f},
          {InputStepType::PointerUp, 0.5f, 0.42f},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::TableSpec spec;
        spec.columns = {{"A", 80.0f, 0u, 0u}, {"B", 80.0f, 0u, 0u}};
        spec.rows = {{"1", "2"}, {"3", "4"}, {"5", "6"}};
        spec.selectedRow = 0;
        spec.headerStyle = StyleSurfaceAlt;
        spec.rowStyle = StyleSurface;
        spec.rowAltStyle = StyleSurfaceAlt;
        spec.selectionStyle = StyleSelection;
        spec.dividerStyle = StyleDivider;
        spec.focusStyle = StyleFocus;
        spec.size.preferredWidth = 210.0f;
        spec.size.preferredHeight = 130.0f;
        return root.createTable(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "list",
      "focus_click",
      "Click focuses list.",
      {
          {InputStepType::PointerDown, 0.5f, 0.35f},
          {InputStepType::PointerUp, 0.5f, 0.35f},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::ListSpec spec;
        spec.items = {"Alpha", "Beta", "Gamma"};
        spec.selectedIndex = 0;
        spec.textStyle = 0u;
        spec.rowStyle = StyleSurface;
        spec.rowAltStyle = StyleSurfaceAlt;
        spec.selectionStyle = StyleSelection;
        spec.dividerStyle = StyleDivider;
        spec.focusStyle = StyleFocus;
        spec.size.preferredWidth = 180.0f;
        spec.size.preferredHeight = 120.0f;
        return root.createList(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "tree_view",
      "focus_click",
      "Click focuses tree view.",
      {
          {InputStepType::PointerDown, 0.42f, 0.2f},
          {InputStepType::PointerUp, 0.42f, 0.2f},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::TreeViewSpec spec;
        spec.nodes = {
            PrimeStage::TreeNode{"Root", {PrimeStage::TreeNode{"Child A"}, PrimeStage::TreeNode{"Child B"}}, true, false},
            PrimeStage::TreeNode{"Second", {}, false, false},
        };
        spec.rowStyle = StyleSurface;
        spec.rowAltStyle = StyleSurfaceAlt;
        spec.hoverStyle = StyleHover;
        spec.selectionStyle = StyleSelection;
        spec.selectionAccentStyle = StyleFill;
        spec.caretBackgroundStyle = StyleTrack;
        spec.caretLineStyle = StyleKnob;
        spec.connectorStyle = StyleDivider;
        spec.focusStyle = StyleFocus;
        spec.textStyle = 0u;
        spec.selectedTextStyle = 1u;
        spec.size.preferredWidth = 220.0f;
        spec.size.preferredHeight = 140.0f;
        return root.createTreeView(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "window",
      "focus_click",
      "Click focuses window.",
      {
          {InputStepType::PointerDown, 0.5f, 0.5f},
          {InputStepType::PointerUp, 0.5f, 0.5f},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::WindowSpec spec;
        spec.title = "Widget";
        spec.width = 180.0f;
        spec.height = 130.0f;
        spec.minWidth = 140.0f;
        spec.minHeight = 100.0f;
        spec.titleBarHeight = 24.0f;
        spec.resizeHandleSize = 16.0f;
        spec.frameStyle = StyleWindowFrame;
        spec.titleBarStyle = StyleWindowTitle;
        spec.titleTextStyle = 0u;
        spec.contentStyle = StyleWindowContent;
        spec.resizeHandleStyle = StyleWindowResize;
        PrimeStage::Window window = root.createWindow(spec);

        PrimeStage::LabelSpec label;
        label.text = "Window content";
        label.textStyle = 0u;
        label.size.preferredWidth = 120.0f;
        label.size.preferredHeight = 20.0f;
        window.content.createLabel(label);
        return window.root.nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "button",
      "mouse_down_hold",
      "Pointer down hold appearance for button.",
      {
          {InputStepType::PointerDown, 0.5f, 0.5f},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::ButtonSpec spec;
        spec.label = "Down";
        spec.backgroundStyle = StyleSurface;
        spec.hoverStyle = StyleHover;
        spec.pressedStyle = StylePressed;
        spec.focusStyle = StyleFocus;
        spec.textStyle = 0u;
        spec.size.preferredWidth = 150.0f;
        spec.size.preferredHeight = 34.0f;
        return root.createButton(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "text_field",
      "mouse_down_hold",
      "Pointer down hold appearance for text field.",
      {
          {InputStepType::PointerDown, 0.35f, 0.5f},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        focusFieldState = PrimeStage::TextFieldState{};
        focusFieldState.text = "Down";
        focusFieldState.cursor = static_cast<uint32_t>(focusFieldState.text.size());
        PrimeStage::TextFieldSpec spec;
        spec.state = &focusFieldState;
        spec.backgroundStyle = StyleSurface;
        spec.focusStyle = StyleFocus;
        spec.selectionStyle = StyleSelection;
        spec.cursorStyle = StyleKnob;
        spec.textStyle = 0u;
        spec.showCursor = true;
        spec.size.preferredWidth = 180.0f;
        spec.size.preferredHeight = 34.0f;
        return root.createTextField(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "toggle",
      "mouse_down_hold",
      "Pointer down hold appearance for toggle.",
      {
          {InputStepType::PointerDown, 0.5f, 0.5f},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::ToggleSpec spec;
        spec.trackStyle = StyleTrack;
        spec.knobStyle = StyleKnob;
        spec.focusStyle = StyleFocus;
        spec.size.preferredWidth = 72.0f;
        spec.size.preferredHeight = 30.0f;
        return root.createToggle(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "checkbox",
      "mouse_down_hold",
      "Pointer down hold appearance for checkbox.",
      {
          {InputStepType::PointerDown, 0.15f, 0.5f},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::CheckboxSpec spec;
        spec.label = "Check";
        spec.boxStyle = StyleTrack;
        spec.checkStyle = StyleSelection;
        spec.focusStyle = StyleFocus;
        spec.textStyle = 0u;
        spec.size.preferredWidth = 160.0f;
        spec.size.preferredHeight = 28.0f;
        return root.createCheckbox(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "slider",
      "mouse_down_hold",
      "Pointer down hold appearance for slider.",
      {
          {InputStepType::PointerDown, 0.7f, 0.5f},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::SliderSpec spec;
        spec.value = 0.3f;
        spec.trackStyle = StyleTrack;
        spec.fillStyle = StyleFill;
        spec.thumbStyle = StyleKnob;
        spec.focusStyle = StyleFocus;
        spec.size.preferredWidth = 200.0f;
        spec.size.preferredHeight = 30.0f;
        return root.createSlider(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "tabs",
      "mouse_down_hold",
      "Pointer down hold appearance for tabs.",
      {
          {InputStepType::PointerDown, 0.18f, 0.5f},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        focusTabsState = PrimeStage::TabsState{};
        focusTabsState.selectedIndex = 1;
        PrimeStage::TabsSpec spec;
        spec.state = &focusTabsState;
        spec.labels = {"One", "Two", "Three"};
        spec.tabStyle = StyleSurface;
        spec.activeTabStyle = StyleHover;
        spec.textStyle = 0u;
        spec.activeTextStyle = 1u;
        spec.size.preferredWidth = 210.0f;
        spec.size.preferredHeight = 32.0f;
        return root.createTabs(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "dropdown",
      "mouse_down_hold",
      "Pointer down hold appearance for dropdown.",
      {
          {InputStepType::PointerDown, 0.5f, 0.5f},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        focusDropdownState = PrimeStage::DropdownState{};
        focusDropdownState.selectedIndex = 0;
        PrimeStage::DropdownSpec spec;
        spec.state = &focusDropdownState;
        spec.options = {"Preview", "Edit", "Export"};
        spec.backgroundStyle = StyleSurface;
        spec.textStyle = 0u;
        spec.indicatorStyle = 1u;
        spec.focusStyle = StyleFocus;
        spec.size.preferredWidth = 180.0f;
        spec.size.preferredHeight = 32.0f;
        return root.createDropdown(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "progress_bar",
      "mouse_down_hold",
      "Pointer down hold appearance for progress bar.",
      {
          {InputStepType::PointerDown, 0.7f, 0.5f},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        focusProgressState = PrimeStage::ProgressBarState{};
        focusProgressState.value = 0.2f;
        PrimeStage::ProgressBarSpec spec;
        spec.state = &focusProgressState;
        spec.trackStyle = StyleTrack;
        spec.fillStyle = StyleFill;
        spec.focusStyle = StyleFocus;
        spec.size.preferredWidth = 200.0f;
        spec.size.preferredHeight = 16.0f;
        return root.createProgressBar(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "progress_bar",
      "click_drag",
      "Pointer click-drag updates progress fill.",
      {
          {InputStepType::PointerDown, 0.20f, 0.5f},
          {InputStepType::PointerDrag, 0.82f, 0.5f},
          {InputStepType::PointerUp, 0.82f, 0.5f},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        focusProgressState = PrimeStage::ProgressBarState{};
        focusProgressState.value = 0.20f;
        PrimeStage::ProgressBarSpec spec;
        spec.state = &focusProgressState;
        spec.trackStyle = StyleTrack;
        spec.fillStyle = StyleFill;
        spec.focusStyle = StyleFocus;
        spec.size.preferredWidth = 200.0f;
        spec.size.preferredHeight = 16.0f;
        return root.createProgressBar(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "table",
      "mouse_down_hold",
      "Pointer down hold appearance for table.",
      {
          {InputStepType::PointerDown, 0.5f, 0.45f},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::TableSpec spec;
        spec.columns = {{"A", 80.0f, 0u, 0u}, {"B", 80.0f, 0u, 0u}};
        spec.rows = {{"1", "2"}, {"3", "4"}, {"5", "6"}};
        spec.selectedRow = -1;
        spec.headerStyle = StyleSurfaceAlt;
        spec.rowStyle = StyleSurface;
        spec.rowAltStyle = StyleSurfaceAlt;
        spec.selectionStyle = StyleSelection;
        spec.dividerStyle = StyleDivider;
        spec.focusStyle = StyleFocus;
        spec.size.preferredWidth = 210.0f;
        spec.size.preferredHeight = 130.0f;
        return root.createTable(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "list",
      "mouse_down_hold",
      "Pointer down hold appearance for list.",
      {
          {InputStepType::PointerDown, 0.5f, 0.35f},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::ListSpec spec;
        spec.items = {"Alpha", "Beta", "Gamma"};
        spec.selectedIndex = 0;
        spec.textStyle = 0u;
        spec.rowStyle = StyleSurface;
        spec.rowAltStyle = StyleSurfaceAlt;
        spec.selectionStyle = StyleSelection;
        spec.dividerStyle = StyleDivider;
        spec.focusStyle = StyleFocus;
        spec.size.preferredWidth = 180.0f;
        spec.size.preferredHeight = 120.0f;
        return root.createList(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "tree_view",
      "mouse_down_hold",
      "Pointer down hold appearance for tree view.",
      {
          {InputStepType::PointerDown, 0.42f, 0.5f},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::TreeViewSpec spec;
        spec.nodes = {
            PrimeStage::TreeNode{"Root", {PrimeStage::TreeNode{"Child A"}, PrimeStage::TreeNode{"Child B"}}, true, false},
            PrimeStage::TreeNode{"Second", {}, false, false},
        };
        spec.rowStyle = StyleSurface;
        spec.rowAltStyle = StyleSurfaceAlt;
        spec.hoverStyle = StyleHover;
        spec.selectionStyle = StyleSelection;
        spec.selectionAccentStyle = StyleFill;
        spec.caretBackgroundStyle = StyleTrack;
        spec.caretLineStyle = StyleKnob;
        spec.connectorStyle = StyleDivider;
        spec.focusStyle = StyleFocus;
        spec.textStyle = 0u;
        spec.selectedTextStyle = 1u;
        spec.size.preferredWidth = 220.0f;
        spec.size.preferredHeight = 140.0f;
        return root.createTreeView(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "window",
      "mouse_down_hold",
      "Pointer down hold appearance for window.",
      {
          {InputStepType::PointerDown, 0.5f, 0.5f},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::WindowSpec spec;
        spec.title = "Widget";
        spec.width = 180.0f;
        spec.height = 130.0f;
        spec.minWidth = 140.0f;
        spec.minHeight = 100.0f;
        spec.titleBarHeight = 24.0f;
        spec.resizeHandleSize = 16.0f;
        spec.frameStyle = StyleWindowFrame;
        spec.titleBarStyle = StyleWindowTitle;
        spec.titleTextStyle = 0u;
        spec.contentStyle = StyleWindowContent;
        spec.resizeHandleStyle = StyleWindowResize;
        PrimeStage::Window window = root.createWindow(spec);

        PrimeStage::LabelSpec label;
        label.text = "Window content";
        label.textStyle = 0u;
        label.size.preferredWidth = 120.0f;
        label.size.preferredHeight = 20.0f;
        window.content.createLabel(label);
        return window.root.nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "button",
      "mouse_pressed",
      "Mouse hover and press state.",
      {
          {InputStepType::PointerMove, 0.5f, 0.5f},
          {InputStepType::PointerDown, 0.5f, 0.5f},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::ButtonSpec spec;
        spec.label = "Button";
        spec.backgroundStyle = StyleSurface;
        spec.hoverStyle = StyleHover;
        spec.pressedStyle = StylePressed;
        spec.focusStyle = StyleFocus;
        spec.textStyle = 0u;
        spec.size.preferredWidth = 150.0f;
        spec.size.preferredHeight = 34.0f;
        return root.createButton(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "button",
      "keyboard_focused",
      "Keyboard focus and activation readiness.",
      {
          {InputStepType::FocusWidget},
          {InputStepType::KeyDown, 0.5f, 0.5f, 0.0f, 0.0f, PrimeStage::KeyCode::Enter},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::ButtonSpec spec;
        spec.label = "Button";
        spec.backgroundStyle = StyleSurface;
        spec.hoverStyle = StyleHover;
        spec.pressedStyle = StylePressed;
        spec.focusStyle = StyleFocus;
        spec.textStyle = 0u;
        spec.size.preferredWidth = 150.0f;
        spec.size.preferredHeight = 34.0f;
        return root.createButton(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "text_field",
      "type_text",
      "Pointer focus, typing, and cursor movement.",
      {
          {InputStepType::PointerDown, 0.3f, 0.5f},
          {InputStepType::PointerUp, 0.3f, 0.5f},
          {InputStepType::TextInput, 0.5f, 0.5f, 0.0f, 0.0f, PrimeStage::KeyCode::Enter, "Prime"},
          {InputStepType::KeyDown, 0.5f, 0.5f, 0.0f, 0.0f, PrimeStage::KeyCode::Left},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        static PrimeStage::TextFieldState state;
        state = PrimeStage::TextFieldState{};
        PrimeStage::TextFieldSpec spec;
        spec.state = &state;
        spec.placeholder = "Type";
        spec.backgroundStyle = StyleSurface;
        spec.focusStyle = StyleFocus;
        spec.selectionStyle = StyleSelection;
        spec.cursorStyle = StyleKnob;
        spec.textStyle = 0u;
        spec.placeholderStyle = 1u;
        spec.showCursor = true;
        spec.size.preferredWidth = 180.0f;
        spec.size.preferredHeight = 34.0f;
        return root.createTextField(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "selectable_text",
      "drag_selection",
      "Mouse drag selection persisted through state rebuild.",
      {
          {InputStepType::FocusWidget},
          {InputStepType::PointerDown, 0.2f, 0.5f},
          {InputStepType::PointerDrag, 0.8f, 0.5f},
          {InputStepType::PointerUp, 0.8f, 0.5f},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::SelectableTextSpec spec;
        spec.state = &selectableState;
        spec.text = "Selectable text example";
        spec.textStyle = 0u;
        spec.selectionStyle = StyleSelection;
        spec.focusStyle = StyleFocus;
        spec.size.preferredWidth = 190.0f;
        spec.size.preferredHeight = 36.0f;
        return root.createSelectableText(spec).nodeId();
      },
      []() {
        selectableState = PrimeStage::SelectableTextState{};
      },
      true});

  scenarios.push_back(WidgetVisualScenario{
      "toggle",
      "mouse_then_keyboard",
      "Toggle with mouse and keyboard space.",
      {
          {InputStepType::PointerDown, 0.5f, 0.5f},
          {InputStepType::PointerUp, 0.5f, 0.5f},
          {InputStepType::FocusWidget},
          {InputStepType::KeyDown, 0.5f, 0.5f, 0.0f, 0.0f, PrimeStage::KeyCode::Space},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        static PrimeStage::ToggleState state;
        state = PrimeStage::ToggleState{};
        state.on = false;
        PrimeStage::ToggleSpec spec;
        spec.state = &state;
        spec.trackStyle = StyleTrack;
        spec.knobStyle = StyleKnob;
        spec.focusStyle = StyleFocus;
        spec.size.preferredWidth = 72.0f;
        spec.size.preferredHeight = 30.0f;
        return root.createToggle(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "checkbox",
      "mouse_then_keyboard",
      "Checkbox with pointer and keyboard activation.",
      {
          {InputStepType::PointerDown, 0.15f, 0.5f},
          {InputStepType::PointerUp, 0.15f, 0.5f},
          {InputStepType::FocusWidget},
          {InputStepType::KeyDown, 0.5f, 0.5f, 0.0f, 0.0f, PrimeStage::KeyCode::Space},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        static PrimeStage::CheckboxState state;
        state = PrimeStage::CheckboxState{};
        state.checked = false;
        PrimeStage::CheckboxSpec spec;
        spec.state = &state;
        spec.label = "Check";
        spec.boxStyle = StyleTrack;
        spec.checkStyle = StyleSelection;
        spec.focusStyle = StyleFocus;
        spec.textStyle = 0u;
        spec.size.preferredWidth = 160.0f;
        spec.size.preferredHeight = 28.0f;
        return root.createCheckbox(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "slider",
      "drag_and_key",
      "Slider drag and keyboard adjustment.",
      {
          {InputStepType::PointerDown, 0.25f, 0.5f},
          {InputStepType::PointerDrag, 0.8f, 0.5f},
          {InputStepType::PointerUp, 0.8f, 0.5f},
          {InputStepType::FocusWidget},
          {InputStepType::KeyDown, 0.5f, 0.5f, 0.0f, 0.0f, PrimeStage::KeyCode::Left},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::SliderSpec spec;
        spec.value = 0.2f;
        spec.trackStyle = StyleTrack;
        spec.fillStyle = StyleFill;
        spec.thumbStyle = StyleKnob;
        spec.focusStyle = StyleFocus;
        spec.size.preferredWidth = 200.0f;
        spec.size.preferredHeight = 30.0f;
        return root.createSlider(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "slider",
      "click_drag",
      "Pointer click-drag updates slider thumb and fill.",
      {
          {InputStepType::PointerDown, 0.20f, 0.5f},
          {InputStepType::PointerDrag, 0.82f, 0.5f},
          {InputStepType::PointerUp, 0.82f, 0.5f},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::SliderSpec spec;
        spec.value = 0.20f;
        spec.trackStyle = StyleTrack;
        spec.fillStyle = StyleFill;
        spec.thumbStyle = StyleKnob;
        spec.focusStyle = StyleFocus;
        spec.callbacks.onValueChanged = [](float) {};
        spec.size.preferredWidth = 200.0f;
        spec.size.preferredHeight = 30.0f;
        return root.createSlider(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "tabs",
      "pointer_and_keys",
      "Pointer selection then keyboard navigation.",
      {
          {InputStepType::PointerDown, 0.7f, 0.5f},
          {InputStepType::PointerUp, 0.7f, 0.5f},
          {InputStepType::KeyDown, 0.5f, 0.5f, 0.0f, 0.0f, PrimeStage::KeyCode::Left},
          {InputStepType::KeyDown, 0.5f, 0.5f, 0.0f, 0.0f, PrimeStage::KeyCode::Enter},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::TabsSpec spec;
        spec.labels = {"One", "Two", "Three"};
        spec.selectedIndex = 0;
        spec.tabStyle = StyleSurface;
        spec.activeTabStyle = StyleHover;
        spec.textStyle = 0u;
        spec.activeTextStyle = 1u;
        spec.size.preferredWidth = 210.0f;
        spec.size.preferredHeight = 32.0f;
        return root.createTabs(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "dropdown",
      "keyboard_space_rebuild",
      "Keyboard activation updates selected option via state rebuild.",
      {
          {InputStepType::FocusWidget},
          {InputStepType::KeyDown, 0.5f, 0.5f, 0.0f, 0.0f, PrimeStage::KeyCode::Space},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::DropdownSpec spec;
        spec.state = &dropdownState;
        spec.options = {"Preview", "Edit", "Export"};
        spec.backgroundStyle = StyleSurface;
        spec.textStyle = 0u;
        spec.indicatorStyle = 1u;
        spec.focusStyle = StyleFocus;
        spec.size.preferredWidth = 180.0f;
        spec.size.preferredHeight = 32.0f;
        return root.createDropdown(spec).nodeId();
      },
      []() {
        dropdownState = PrimeStage::DropdownState{};
        dropdownState.selectedIndex = 0;
      },
      true});

  scenarios.push_back(WidgetVisualScenario{
      "progress_bar",
      "pointer_and_keys",
      "Progress interaction with pointer and Home/End keys.",
      {
          {InputStepType::PointerDown, 0.75f, 0.5f},
          {InputStepType::PointerUp, 0.75f, 0.5f},
          {InputStepType::FocusWidget},
          {InputStepType::KeyDown, 0.5f, 0.5f, 0.0f, 0.0f, PrimeStage::KeyCode::Home},
          {InputStepType::KeyDown, 0.5f, 0.5f, 0.0f, 0.0f, PrimeStage::KeyCode::End},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        static PrimeStage::ProgressBarState state;
        state = PrimeStage::ProgressBarState{};
        state.value = 0.3f;
        PrimeStage::ProgressBarSpec spec;
        spec.state = &state;
        spec.trackStyle = StyleTrack;
        spec.fillStyle = StyleFill;
        spec.focusStyle = StyleFocus;
        spec.size.preferredWidth = 200.0f;
        spec.size.preferredHeight = 16.0f;
        return root.createProgressBar(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "table",
      "pointer_and_down",
      "Table row click and keyboard navigation.",
      {
          {InputStepType::PointerDown, 0.5f, 0.45f},
          {InputStepType::PointerUp, 0.5f, 0.45f},
          {InputStepType::KeyDown, 0.5f, 0.5f, 0.0f, 0.0f, PrimeStage::KeyCode::Down},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::TableSpec spec;
        spec.columns = {{"A", 80.0f, 0u, 0u}, {"B", 80.0f, 0u, 0u}};
        spec.rows = {{"1", "2"}, {"3", "4"}, {"5", "6"}};
        spec.selectedRow = 0;
        spec.headerStyle = StyleSurfaceAlt;
        spec.rowStyle = StyleSurface;
        spec.rowAltStyle = StyleSurfaceAlt;
        spec.selectionStyle = StyleSelection;
        spec.dividerStyle = StyleDivider;
        spec.focusStyle = StyleFocus;
        spec.size.preferredWidth = 210.0f;
        spec.size.preferredHeight = 130.0f;
        return root.createTable(spec).nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "list",
      "pointer_select_rebuild",
      "Pointer selection persisted through state rebuild.",
      {
          {InputStepType::PointerDown, 0.5f, 0.55f},
          {InputStepType::PointerUp, 0.5f, 0.55f},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::ListSpec spec;
        spec.items = {"Alpha", "Beta", "Gamma"};
        spec.selectedIndex = listSelectedIndex;
        spec.textStyle = 0u;
        spec.rowStyle = StyleSurface;
        spec.rowAltStyle = StyleSurfaceAlt;
        spec.selectionStyle = StyleSelection;
        spec.dividerStyle = StyleDivider;
        spec.focusStyle = StyleFocus;
        spec.size.preferredWidth = 180.0f;
        spec.size.preferredHeight = 120.0f;
        spec.callbacks.onSelected = [](PrimeStage::ListRowInfo const& info) {
          listSelectedIndex = info.rowIndex;
        };
        return root.createList(spec).nodeId();
      },
      []() { listSelectedIndex = 1; },
      true});

  scenarios.push_back(WidgetVisualScenario{
      "tree_view",
      "keyboard_down_rebuild",
      "Tree keyboard navigation persisted through state rebuild.",
      {
          {InputStepType::FocusWidget},
          {InputStepType::KeyDown, 0.5f, 0.5f, 0.0f, 0.0f, PrimeStage::KeyCode::Down},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::TreeViewSpec spec;
        PrimeStage::TreeNode childA{"Child A", {}, false, treeSelectedRow == 1};
        PrimeStage::TreeNode childB{"Child B", {}, false, treeSelectedRow == 2};
        PrimeStage::TreeNode rootNode{"Root", {childA, childB}, true, treeSelectedRow == 0};
        PrimeStage::TreeNode secondNode{"Second", {}, false, treeSelectedRow == 3};
        spec.nodes = {
            rootNode,
            secondNode,
        };
        spec.rowStyle = StyleSurface;
        spec.rowAltStyle = StyleSurfaceAlt;
        spec.hoverStyle = StyleHover;
        spec.selectionStyle = StyleSelection;
        spec.selectionAccentStyle = StyleFill;
        spec.caretBackgroundStyle = StyleTrack;
        spec.caretLineStyle = StyleKnob;
        spec.connectorStyle = StyleDivider;
        spec.focusStyle = StyleFocus;
        spec.textStyle = 0u;
        spec.selectedTextStyle = 1u;
        spec.size.preferredWidth = 220.0f;
        spec.size.preferredHeight = 140.0f;
        spec.callbacks.onSelectionChanged = [](PrimeStage::TreeViewRowInfo const& info) {
          treeSelectedRow = info.rowIndex;
        };
        return root.createTreeView(spec).nodeId();
      },
      []() { treeSelectedRow = 0; },
      true});

  scenarios.push_back(WidgetVisualScenario{
      "scroll_view",
      "scroll_input",
      "Scroll view baseline plus wheel input.",
      {
          {InputStepType::PointerScroll, 0.8f, 0.5f, 0.0f, -12.0f},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::ScrollViewSpec spec;
        spec.size.preferredWidth = 190.0f;
        spec.size.preferredHeight = 130.0f;
        spec.showVertical = true;
        spec.showHorizontal = true;
        spec.vertical.trackStyle = StyleSurfaceAlt;
        spec.vertical.thumbStyle = StyleHover;
        spec.vertical.thumbLength = 48.0f;
        spec.horizontal.trackStyle = StyleSurfaceAlt;
        spec.horizontal.thumbStyle = StyleHover;
        spec.horizontal.thumbLength = 56.0f;
        PrimeStage::ScrollView view = root.createScrollView(spec);

        PrimeStage::PanelSpec content;
        content.rectStyle = StyleSurface;
        content.size.preferredWidth = 160.0f;
        content.size.preferredHeight = 220.0f;
        view.content.createPanel(content);
        return view.root.nodeId();
      }});

  scenarios.push_back(WidgetVisualScenario{
      "window",
      "move_and_resize",
      "Window drag and resize interactions.",
      {
          {InputStepType::PointerDown, 0.5f, 0.08f},
          {InputStepType::PointerDrag, 0.54f, 0.12f},
          {InputStepType::PointerUp, 0.54f, 0.12f},
          {InputStepType::PointerDown, 0.95f, 0.95f},
          {InputStepType::PointerDrag, 0.97f, 0.97f},
          {InputStepType::PointerUp, 0.97f, 0.97f},
      },
      [](PrimeFrame::Frame&, PrimeStage::UiNode& root) {
        PrimeStage::WindowSpec spec;
        spec.title = "Widget";
        spec.width = 180.0f;
        spec.height = 130.0f;
        spec.minWidth = 140.0f;
        spec.minHeight = 100.0f;
        spec.titleBarHeight = 24.0f;
        spec.resizeHandleSize = 16.0f;
        spec.frameStyle = StyleWindowFrame;
        spec.titleBarStyle = StyleWindowTitle;
        spec.titleTextStyle = 0u;
        spec.contentStyle = StyleWindowContent;
        spec.resizeHandleStyle = StyleWindowResize;
        PrimeStage::Window window = root.createWindow(spec);

        PrimeStage::LabelSpec label;
        label.text = "Window content";
        label.textStyle = 0u;
        label.size.preferredWidth = 120.0f;
        label.size.preferredHeight = 20.0f;
        window.content.createLabel(label);
        return window.root.nodeId();
      }});

  return scenarios;
}

struct GeneratedImage {
  std::string widget;
  std::string scenario;
  std::string summary;
  std::vector<InputStep> inputScript;
  std::filesystem::path beforePngPath;
  std::filesystem::path afterPngPath;
  bool rebuiltAfterInput = false;
};

std::filesystem::path defaultOutputDir() {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  std::filesystem::path repoRoot = sourcePath.parent_path().parent_path().parent_path();
  return repoRoot / "tests" / "snapshots" / "widget_visuals";
}

std::optional<std::filesystem::path> outputDirFromEnvironment() {
  char const* env = std::getenv("PRIMESTAGE_WIDGET_VISUAL_OUTPUT_DIR");
  if (!env || std::string_view(env).empty()) {
    return std::nullopt;
  }
  return std::filesystem::path(env);
}

bool generationEnabled() {
  return std::getenv("PRIMESTAGE_GENERATE_WIDGET_VISUALS") != nullptr;
}

void renderSnapshotPng(PrimeFrame::Frame& frame,
                       PrimeFrame::LayoutOutput const& layout,
                       std::filesystem::path const& pngPath,
                       std::string_view widget,
                       std::string_view scenario,
                       std::string_view phase) {
  std::string pngPathText = pngPath.string();
  PrimeStage::RenderOptions options;
  options.clear = true;
  options.clearColor = PrimeStage::Rgba8{13, 18, 24, 255};
  PrimeStage::RenderStatus status =
      PrimeStage::renderFrameToPng(frame, layout, pngPathText, options);
  REQUIRE_MESSAGE(status.ok(),
                  "render failed for " << widget << "/" << scenario << " (" << phase << "): "
                                       << PrimeStage::renderStatusMessage(status.code)
                                       << " detail=" << status.detail);
  CHECK(status.targetWidth == CanvasPixels);
  CHECK(status.targetHeight == CanvasPixels);
}

std::vector<GeneratedImage> generateWidgetVisuals(std::filesystem::path const& outputDir,
                                                  std::vector<WidgetVisualScenario> const& scenarios) {
  std::vector<GeneratedImage> generated;
  generated.reserve(scenarios.size());

  for (WidgetVisualScenario const& scenario : scenarios) {
    if (scenario.resetState) {
      scenario.resetState();
    }

    auto buildScenarioFrame = [&](PrimeFrame::Frame& frame,
                                  PrimeFrame::NodeId& rootId,
                                  PrimeFrame::NodeId& widgetId) {
      configureVisualTheme(frame);
      PrimeStage::UiNode root = createCanvasRoot(frame);
      rootId = root.nodeId();
      widgetId = scenario.buildWidget(frame, root);
      REQUIRE(widgetId.isValid());
      centerWidgetInCanvas(frame, rootId, widgetId);
    };

    std::filesystem::path widgetDir = outputDir / slugify(scenario.widget);
    std::filesystem::create_directories(widgetDir);
    std::string scenarioSlug = slugify(scenario.name);
    std::filesystem::path beforePngPath = widgetDir / (scenarioSlug + "_before.png");
    std::filesystem::path afterPngPath = widgetDir / (scenarioSlug + "_after.png");

    PrimeFrame::Frame beforeFrame;
    PrimeFrame::NodeId rootId{};
    PrimeFrame::NodeId widgetId{};
    buildScenarioFrame(beforeFrame, rootId, widgetId);
    PrimeFrame::LayoutOutput beforeLayout = layoutCanvas(beforeFrame);
    renderSnapshotPng(beforeFrame,
                      beforeLayout,
                      beforePngPath,
                      scenario.widget,
                      scenario.name,
                      "before");

    PrimeFrame::LayoutOutput replayLayout = layoutCanvas(beforeFrame);
    PrimeFrame::EventRouter router;
    router.setDragThreshold(0.0f);
    PrimeFrame::FocusManager focus;
    replayInputScript(scenario.inputScript,
                      beforeFrame,
                      replayLayout,
                      rootId,
                      widgetId,
                      router,
                      focus);

    if (scenario.rebuildAfterInput) {
      PrimeFrame::Frame afterFrame;
      PrimeFrame::NodeId afterRootId{};
      PrimeFrame::NodeId afterWidgetId{};
      buildScenarioFrame(afterFrame, afterRootId, afterWidgetId);
      PrimeFrame::LayoutOutput afterLayout = layoutCanvas(afterFrame);
      renderSnapshotPng(afterFrame,
                        afterLayout,
                        afterPngPath,
                        scenario.widget,
                        scenario.name,
                        "after_rebuild");
    } else {
      PrimeFrame::LayoutOutput afterLayout = layoutCanvas(beforeFrame);
      renderSnapshotPng(beforeFrame,
                        afterLayout,
                        afterPngPath,
                        scenario.widget,
                        scenario.name,
                        "after_live");
    }

    generated.push_back(GeneratedImage{
        scenario.widget,
        scenario.name,
        scenario.summary,
        scenario.inputScript,
        beforePngPath,
        afterPngPath,
        scenario.rebuildAfterInput,
    });
  }

  return generated;
}

void writeManifest(std::filesystem::path const& outputDir,
                   std::vector<GeneratedImage> const& generated) {
  std::filesystem::create_directories(outputDir);
  std::filesystem::path manifestPath = outputDir / "manifest.txt";
  std::ofstream out(manifestPath, std::ios::binary | std::ios::trunc);
  REQUIRE(out.good());

  out << "[widget-visual-facility]\n";
  out << "canvas=" << CanvasPixels << "x" << CanvasPixels << "\n";
  out << "scenarios=" << generated.size() << "\n";
  out << "images=" << (generated.size() * 2u) << "\n\n";

  for (GeneratedImage const& image : generated) {
    std::filesystem::path relativeBefore = std::filesystem::relative(image.beforePngPath, outputDir);
    std::filesystem::path relativeAfter = std::filesystem::relative(image.afterPngPath, outputDir);
    out << "[" << image.widget << "/" << image.scenario << "]\n";
    out << "summary=" << image.summary << "\n";
    out << "before_png=" << relativeBefore.string() << "\n";
    out << "after_png=" << relativeAfter.string() << "\n";
    out << "after_mode=" << (image.rebuiltAfterInput ? "rebuild_after_input" : "live_after_input")
        << "\n";
    out << "inputs=";
    if (image.inputScript.empty()) {
      out << "none";
    } else {
      for (size_t i = 0; i < image.inputScript.size(); ++i) {
        if (i > 0) {
          out << " -> ";
        }
        out << inputStepSummary(image.inputScript[i]);
      }
    }
    out << "\n\n";
  }

  REQUIRE(out.good());
}

TEST_CASE("PrimeStage widget visual facility catalog covers every widget class") {
  std::vector<WidgetVisualScenario> scenarios = buildScenarioCatalog();
  REQUIRE_FALSE(scenarios.empty());

  std::set<std::string> covered;
  for (WidgetVisualScenario const& scenario : scenarios) {
    CHECK_FALSE(scenario.widget.empty());
    CHECK_FALSE(scenario.name.empty());
    CHECK_FALSE(scenario.summary.empty());
    CHECK(static_cast<bool>(scenario.buildWidget));
    covered.insert(scenario.widget);
  }

  std::vector<std::string> expected = {
      "stack",
      "panel",
      "label",
      "paragraph",
      "text_line",
      "divider",
      "spacer",
      "button",
      "text_field",
      "selectable_text",
      "toggle",
      "checkbox",
      "slider",
      "tabs",
      "dropdown",
      "progress_bar",
      "table",
      "list",
      "tree_view",
      "scroll_view",
      "window",
  };

  for (std::string const& widget : expected) {
    CHECK_MESSAGE(covered.count(widget) > 0, "missing scenario coverage for widget: " << widget);
  }
}

TEST_CASE("PrimeStage widget visual facility generates manual-review PNGs on demand") {
  if (!generationEnabled()) {
    INFO("Set PRIMESTAGE_GENERATE_WIDGET_VISUALS=1 to export 256x256 widget PNGs.");
    CHECK(true);
    return;
  }

  std::filesystem::path outputDir = outputDirFromEnvironment().value_or(defaultOutputDir());
  std::vector<WidgetVisualScenario> scenarios = buildScenarioCatalog();
  std::vector<GeneratedImage> generated = generateWidgetVisuals(outputDir, scenarios);

  REQUIRE(generated.size() == scenarios.size());
  for (GeneratedImage const& image : generated) {
    CHECK(std::filesystem::exists(image.beforePngPath));
    CHECK(std::filesystem::exists(image.afterPngPath));
    std::error_code error;
    uintmax_t beforeSize = std::filesystem::file_size(image.beforePngPath, error);
    CHECK_FALSE(error);
    CHECK(beforeSize > 0u);
    error.clear();
    uintmax_t afterSize = std::filesystem::file_size(image.afterPngPath, error);
    CHECK_FALSE(error);
    CHECK(afterSize > 0u);
  }

  writeManifest(outputDir, generated);
  CHECK(std::filesystem::exists(outputDir / "manifest.txt"));
}

} // namespace
