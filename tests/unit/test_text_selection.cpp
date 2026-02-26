#include "PrimeStage/TextSelection.h"
#include "PrimeStage/Ui.h"

#include "PrimeFrame/Events.h"

#include "third_party/doctest.h"

#include <string>
#include <vector>

namespace {

std::vector<uint32_t> utf8_boundaries(std::string_view text) {
  std::vector<uint32_t> indices;
  indices.push_back(0u);
  uint32_t cursor = PrimeStage::utf8Next(text, 0u);
  while (cursor <= text.size()) {
    indices.push_back(cursor);
    if (cursor == text.size()) {
      break;
    }
    cursor = PrimeStage::utf8Next(text, cursor);
  }
  return indices;
}

std::vector<float> prefix_widths(PrimeFrame::Frame& frame,
                                 PrimeFrame::TextStyleToken token,
                                 std::string_view text,
                                 std::vector<uint32_t> const& indices) {
  std::vector<float> widths;
  widths.reserve(indices.size());
  for (uint32_t index : indices) {
    widths.push_back(PrimeStage::measureTextWidth(frame, token, text.substr(0, index)));
  }
  return widths;
}

std::string sample_utf8_text() {
  return std::string("a") +
         "\xC3" "\xA9"
         "b"
         "\xE2" "\x82" "\xAC"
         "c";
}

} // namespace

TEST_CASE("Text selection utf8 navigation returns codepoint boundaries") {
  std::string text = sample_utf8_text();
  CHECK(text.size() == 8u);
  CHECK(PrimeStage::utf8Next(text, 0u) == 1u);
  CHECK(PrimeStage::utf8Next(text, 1u) == 3u);
  CHECK(PrimeStage::utf8Next(text, 3u) == 4u);
  CHECK(PrimeStage::utf8Next(text, 4u) == 7u);
  CHECK(PrimeStage::utf8Next(text, 7u) == 8u);
  CHECK(PrimeStage::utf8Prev(text, 8u) == 7u);
  CHECK(PrimeStage::utf8Prev(text, 7u) == 4u);
  CHECK(PrimeStage::utf8Prev(text, 4u) == 3u);
  CHECK(PrimeStage::utf8Prev(text, 3u) == 1u);
  CHECK(PrimeStage::utf8Prev(text, 1u) == 0u);
}

TEST_CASE("Caret index clamps to bounds with padding") {
  PrimeFrame::Frame frame;
  std::string text = "Hello";
  float padding = 12.0f;
  float total = PrimeStage::measureTextWidth(frame, 0, text);

  CHECK(PrimeStage::caretIndexForClick(frame, 0, text, padding, padding - 4.0f) == 0u);
  CHECK(PrimeStage::caretIndexForClick(frame, 0, text, padding, padding + total + 2.0f) ==
        static_cast<uint32_t>(text.size()));
}

TEST_CASE("Caret index follows nearest boundary for ASCII text") {
  PrimeFrame::Frame frame;
  std::string text = "HelloWorld";
  float padding = 6.0f;
  auto indices = utf8_boundaries(text);
  auto widths = prefix_widths(frame, 0, text, indices);

  for (size_t i = 0; i + 1 < indices.size(); ++i) {
    float start = widths[i];
    float end = widths[i + 1];
    float diff = end - start;
    CHECK(diff > 0.0f);

    float nearStart = padding + start + diff * 0.1f;
    float nearEnd = padding + start + diff * 0.9f;
    CHECK(PrimeStage::caretIndexForClick(frame, 0, text, padding, nearStart) == indices[i]);
    CHECK(PrimeStage::caretIndexForClick(frame, 0, text, padding, nearEnd) == indices[i + 1]);
  }
}

TEST_CASE("Caret index respects UTF-8 boundaries") {
  PrimeFrame::Frame frame;
  std::string text = sample_utf8_text();
  float padding = 4.0f;
  auto indices = utf8_boundaries(text);
  auto widths = prefix_widths(frame, 0, text, indices);

  for (size_t i = 0; i + 1 < indices.size(); ++i) {
    float start = widths[i];
    float end = widths[i + 1];
    float diff = end - start;
    CHECK(diff > 0.0f);

    float nearStart = padding + start + diff * 0.1f;
    float nearEnd = padding + start + diff * 0.9f;
    CHECK(PrimeStage::caretIndexForClick(frame, 0, text, padding, nearStart) == indices[i]);
    CHECK(PrimeStage::caretIndexForClick(frame, 0, text, padding, nearEnd) == indices[i + 1]);
  }
}

TEST_CASE("Caret index maps to correct line in layout") {
  PrimeFrame::Frame frame;
  std::string text = "Hello\nWorld";
  PrimeStage::TextSelectionLayout layout =
      PrimeStage::buildTextSelectionLayout(frame, 0, text, 0.0f, PrimeFrame::WrapMode::None);
  REQUIRE(layout.lines.size() == 2u);

  float padding = 3.0f;
  for (size_t lineIndex = 0; lineIndex < layout.lines.size(); ++lineIndex) {
    auto line = layout.lines[lineIndex];
    std::string_view lineText(text.data() + line.start, line.end - line.start);
    auto indices = utf8_boundaries(lineText);
    auto widths = prefix_widths(frame, 0, lineText, indices);
    float localY = (static_cast<float>(lineIndex) + 0.5f) * layout.lineHeight;

    for (size_t i = 0; i + 1 < indices.size(); ++i) {
      float start = widths[i];
      float end = widths[i + 1];
      float diff = end - start;
      CHECK(diff > 0.0f);
      float nearStart = padding + start + diff * 0.1f;
      float nearEnd = padding + start + diff * 0.9f;

      uint32_t expectedStart = line.start + indices[i];
      uint32_t expectedEnd = line.start + indices[i + 1];
      CHECK(PrimeStage::caretIndexForClickInLayout(frame,
                                                   0,
                                                   text,
                                                   layout,
                                                   padding,
                                                   nearStart,
                                                   localY) == expectedStart);
      CHECK(PrimeStage::caretIndexForClickInLayout(frame,
                                                   0,
                                                   text,
                                                   layout,
                                                   padding,
                                                   nearEnd,
                                                   localY) == expectedEnd);
    }
  }
}

TEST_CASE("Selection rects follow line ranges and padding") {
  PrimeFrame::Frame frame;
  std::string text = "Hello\nWorld";
  PrimeStage::TextSelectionLayout layout =
      PrimeStage::buildTextSelectionLayout(frame, 0, text, 0.0f, PrimeFrame::WrapMode::None);
  REQUIRE(layout.lines.size() == 2u);

  float padding = 5.0f;
  uint32_t selectionStart = 2u;
  uint32_t selectionEnd = 9u;
  auto rects = PrimeStage::buildSelectionRects(frame,
                                               0,
                                               text,
                                               layout,
                                               selectionStart,
                                               selectionEnd,
                                               padding);
  REQUIRE(rects.size() == 2u);

  auto line0 = layout.lines[0];
  std::string_view line0Text(text.data() + line0.start, line0.end - line0.start);
  float line0Start = PrimeStage::measureTextWidth(frame, 0, line0Text.substr(0, selectionStart));
  float line0End = PrimeStage::measureTextWidth(frame, 0, line0Text);
  CHECK(rects[0].x == doctest::Approx(padding + line0Start).epsilon(0.05));
  CHECK(rects[0].width == doctest::Approx(line0End - line0Start).epsilon(0.05));

  auto line1 = layout.lines[1];
  std::string_view line1Text(text.data() + line1.start, line1.end - line1.start);
  uint32_t localEnd = selectionEnd - line1.start;
  float line1End = PrimeStage::measureTextWidth(frame, 0, line1Text.substr(0, localEnd));
  CHECK(rects[1].x == doctest::Approx(padding).epsilon(0.05));
  CHECK(rects[1].width == doctest::Approx(line1End).epsilon(0.05));
}

TEST_CASE("Selectable text helpers track and clear selection") {
  PrimeStage::SelectableTextState state;
  state.text = "Hello";
  state.selectionStart = 1u;
  state.selectionEnd = 4u;
  uint32_t start = 0u;
  uint32_t end = 0u;
  CHECK(PrimeStage::selectableTextHasSelection(state, start, end));
  CHECK(start == 1u);
  CHECK(end == 4u);

  PrimeStage::clearSelectableTextSelection(state, 2u);
  CHECK(state.selectionAnchor == 2u);
  CHECK(state.selectionStart == 2u);
  CHECK(state.selectionEnd == 2u);
  CHECK_FALSE(PrimeStage::selectableTextHasSelection(state, start, end));
}

TEST_CASE("Selectable text keyboard selection moves anchor and end") {
  PrimeFrame::Frame frame;
  PrimeFrame::NodeId rootId = frame.createNode();
  frame.addRoot(rootId);
  PrimeStage::UiNode root(frame, rootId, true);

  PrimeStage::SelectableTextState state;
  PrimeStage::SelectableTextSpec spec;
  spec.state = &state;
  spec.text = "Hello";
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 40.0f;
  PrimeStage::UiNode text = root.createSelectableText(spec);

  PrimeFrame::Node const* node = frame.getNode(text.nodeId());
  REQUIRE(node != nullptr);
  PrimeFrame::Callback const* callback = frame.getCallback(node->callbacks);
  REQUIRE(callback != nullptr);
  REQUIRE(callback->onEvent);

  state.focused = true;

  constexpr int KeyRight = 0x4F;
  constexpr int KeyHome = 0x4A;
  constexpr uint32_t ShiftMask = 1u << 0u;

  PrimeFrame::Event event;
  event.type = PrimeFrame::EventType::KeyDown;
  event.key = KeyRight;
  event.modifiers = ShiftMask;
  callback->onEvent(event);

  uint32_t start = 0u;
  uint32_t end = 0u;
  CHECK(PrimeStage::selectableTextHasSelection(state, start, end));
  CHECK(start == 0u);
  CHECK(end == 1u);

  event.modifiers = 0u;
  callback->onEvent(event);
  CHECK_FALSE(PrimeStage::selectableTextHasSelection(state, start, end));
  CHECK(state.selectionStart == 1u);
  CHECK(state.selectionEnd == 1u);

  event.key = KeyHome;
  event.modifiers = ShiftMask;
  callback->onEvent(event);
  CHECK(PrimeStage::selectableTextHasSelection(state, start, end));
  CHECK(start == 0u);
  CHECK(end == 1u);
}

TEST_CASE("Selectable text moves vertically across lines") {
  PrimeFrame::Frame frame;
  PrimeFrame::NodeId rootId = frame.createNode();
  frame.addRoot(rootId);
  PrimeStage::UiNode root(frame, rootId, true);

  PrimeStage::SelectableTextState state;
  PrimeStage::SelectableTextSpec spec;
  spec.state = &state;
  spec.text = "Hello\nWorld";
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 80.0f;
  PrimeStage::UiNode text = root.createSelectableText(spec);

  PrimeFrame::Node const* node = frame.getNode(text.nodeId());
  REQUIRE(node != nullptr);
  PrimeFrame::Callback const* callback = frame.getCallback(node->callbacks);
  REQUIRE(callback != nullptr);
  REQUIRE(callback->onEvent);

  state.focused = true;

  constexpr int KeyDown = 0x51;
  constexpr int KeyUp = 0x52;

  PrimeFrame::Event event;
  event.type = PrimeFrame::EventType::KeyDown;
  event.key = KeyDown;
  callback->onEvent(event);

  uint32_t start = 0u;
  uint32_t end = 0u;
  CHECK_FALSE(PrimeStage::selectableTextHasSelection(state, start, end));
  CHECK(state.selectionStart >= 5u);

  event.key = KeyUp;
  callback->onEvent(event);
  CHECK(state.selectionStart <= 5u);
}

TEST_CASE("Selectable text shift vertical selection extends") {
  PrimeFrame::Frame frame;
  PrimeFrame::NodeId rootId = frame.createNode();
  frame.addRoot(rootId);
  PrimeStage::UiNode root(frame, rootId, true);

  PrimeStage::SelectableTextState state;
  PrimeStage::SelectableTextSpec spec;
  spec.state = &state;
  spec.text = "Hello\nWorld";
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 80.0f;
  PrimeStage::UiNode text = root.createSelectableText(spec);

  PrimeFrame::Node const* node = frame.getNode(text.nodeId());
  REQUIRE(node != nullptr);
  PrimeFrame::Callback const* callback = frame.getCallback(node->callbacks);
  REQUIRE(callback != nullptr);
  REQUIRE(callback->onEvent);

  state.focused = true;

  constexpr int KeyDown = 0x51;
  constexpr int KeyUp = 0x52;
  constexpr uint32_t ShiftMask = 1u << 0u;

  PrimeFrame::Event event;
  event.type = PrimeFrame::EventType::KeyDown;
  event.key = KeyDown;
  event.modifiers = ShiftMask;
  callback->onEvent(event);

  uint32_t start = 0u;
  uint32_t end = 0u;
  CHECK(PrimeStage::selectableTextHasSelection(state, start, end));
  CHECK(start == 0u);
  CHECK(end > 0u);

  event.key = KeyUp;
  callback->onEvent(event);
  CHECK(state.selectionStart == state.selectionEnd);
}

TEST_CASE("Selectable text word navigation uses alt") {
  PrimeFrame::Frame frame;
  PrimeFrame::NodeId rootId = frame.createNode();
  frame.addRoot(rootId);
  PrimeStage::UiNode root(frame, rootId, true);

  PrimeStage::SelectableTextState state;
  PrimeStage::SelectableTextSpec spec;
  spec.state = &state;
  spec.text = "Hello world";
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 40.0f;
  PrimeStage::UiNode text = root.createSelectableText(spec);

  PrimeFrame::Node const* node = frame.getNode(text.nodeId());
  REQUIRE(node != nullptr);
  PrimeFrame::Callback const* callback = frame.getCallback(node->callbacks);
  REQUIRE(callback != nullptr);
  REQUIRE(callback->onEvent);

  state.focused = true;

  constexpr int KeyRight = 0x4F;
  constexpr int KeyLeft = 0x50;
  constexpr uint32_t AltMask = 1u << 2u;

  PrimeFrame::Event event;
  event.type = PrimeFrame::EventType::KeyDown;
  event.key = KeyRight;
  event.modifiers = AltMask;
  callback->onEvent(event);

  CHECK(state.selectionStart == 5u);
  CHECK(state.selectionEnd == 5u);

  callback->onEvent(event);
  CHECK(state.selectionStart == 6u);
  CHECK(state.selectionEnd == 6u);

  event.key = KeyLeft;
  callback->onEvent(event);
  CHECK(state.selectionStart == 0u);
  CHECK(state.selectionEnd == 0u);

  callback->onEvent(event);
  CHECK(state.selectionStart == 0u);
  CHECK(state.selectionEnd == 0u);
}

TEST_CASE("Selectable text alt shift extends word selection") {
  PrimeFrame::Frame frame;
  PrimeFrame::NodeId rootId = frame.createNode();
  frame.addRoot(rootId);
  PrimeStage::UiNode root(frame, rootId, true);

  PrimeStage::SelectableTextState state;
  PrimeStage::SelectableTextSpec spec;
  spec.state = &state;
  spec.text = "Hello world";
  spec.size.preferredWidth = 200.0f;
  spec.size.preferredHeight = 40.0f;
  PrimeStage::UiNode text = root.createSelectableText(spec);

  PrimeFrame::Node const* node = frame.getNode(text.nodeId());
  REQUIRE(node != nullptr);
  PrimeFrame::Callback const* callback = frame.getCallback(node->callbacks);
  REQUIRE(callback != nullptr);
  REQUIRE(callback->onEvent);

  state.focused = true;

  constexpr int KeyRight = 0x4F;
  constexpr int KeyLeft = 0x50;
  constexpr uint32_t ShiftMask = 1u << 0u;
  constexpr uint32_t AltMask = 1u << 2u;

  PrimeFrame::Event event;
  event.type = PrimeFrame::EventType::KeyDown;
  event.key = KeyRight;
  event.modifiers = ShiftMask | AltMask;
  callback->onEvent(event);

  uint32_t start = 0u;
  uint32_t end = 0u;
  CHECK(PrimeStage::selectableTextHasSelection(state, start, end));
  CHECK(start == 0u);
  CHECK(end == 5u);

  callback->onEvent(event);
  CHECK(PrimeStage::selectableTextHasSelection(state, start, end));
  CHECK(start == 0u);
  CHECK(end == 6u);

  event.key = KeyLeft;
  callback->onEvent(event);
  CHECK(state.selectionStart == state.selectionEnd);
  CHECK(state.selectionStart == 0u);
}

TEST_CASE("Selectable text clears selection on blur") {
  PrimeFrame::Frame frame;
  PrimeFrame::NodeId rootId = frame.createNode();
  frame.addRoot(rootId);
  PrimeStage::UiNode root(frame, rootId, true);

  PrimeStage::SelectableTextState state;
  state.focused = true;
  state.selectionStart = 0u;
  state.selectionEnd = 5u;
  PrimeStage::SelectableTextSpec spec;
  spec.state = &state;
  spec.text = "Hello world";
  PrimeStage::UiNode text = root.createSelectableText(spec);

  PrimeFrame::Node const* node = frame.getNode(text.nodeId());
  REQUIRE(node != nullptr);
  PrimeFrame::Callback const* callback = frame.getCallback(node->callbacks);
  REQUIRE(callback != nullptr);
  REQUIRE(callback->onBlur);

  callback->onBlur();
  CHECK(state.selectionStart == state.selectionEnd);
}
