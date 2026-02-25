#include "PrimeStage/TextSelection.h"
#include "PrimeStage/Ui.h"

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

} // namespace

TEST_CASE("Text selection utf8 navigation returns codepoint boundaries") {
  std::string text = "a\xC3\xA9b\xE2\x82\xACc";
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
  std::string text = "a\xC3\xA9b\xE2\x82\xACc";
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
  CHECK(rects[0].x == doctest::Approx(padding + line0Start));
  CHECK(rects[0].width == doctest::Approx(line0End - line0Start));

  auto line1 = layout.lines[1];
  std::string_view line1Text(text.data() + line1.start, line1.end - line1.start);
  uint32_t localEnd = selectionEnd - line1.start;
  float line1End = PrimeStage::measureTextWidth(frame, 0, line1Text.substr(0, localEnd));
  CHECK(rects[1].x == doctest::Approx(padding));
  CHECK(rects[1].width == doctest::Approx(line1End));
}
