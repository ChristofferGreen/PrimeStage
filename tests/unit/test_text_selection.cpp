#include "PrimeStage/TextSelection.h"

#include "PrimeStage/StudioUi.h"
#include "third_party/doctest.h"

namespace {

PrimeFrame::Frame makeFrame() {
  PrimeFrame::Frame frame;
  PrimeStage::Studio::applyStudioTheme(frame);
  return frame;
}

PrimeFrame::TextStyleToken bodyToken() {
  return PrimeStage::Studio::textToken(PrimeStage::Studio::TextRole::BodyBright);
}

float textWidth(PrimeFrame::Frame& frame, std::string_view text) {
  return PrimeStage::measureTextWidth(frame, bodyToken(), text);
}

} // namespace

TEST_CASE("text selection :: wrap none respects newlines") {
  auto frame = makeFrame();
  std::string_view text = "a\nb\n";
  auto lines = PrimeStage::wrapTextLineRanges(frame,
                                              bodyToken(),
                                              text,
                                              0.0f,
                                              PrimeFrame::WrapMode::None);
  REQUIRE(lines.size() == 3u);
  CHECK(lines[0].start == 0u);
  CHECK(lines[0].end == 1u);
  CHECK(lines[1].start == 2u);
  CHECK(lines[1].end == 3u);
  CHECK(lines[2].start == 4u);
  CHECK(lines[2].end == 4u);
}

TEST_CASE("text selection :: wrap word splits on width") {
  auto frame = makeFrame();
  std::string_view text = "one two three";
  float maxWidth = textWidth(frame, "one two") + 0.1f;
  auto lines = PrimeStage::wrapTextLineRanges(frame,
                                              bodyToken(),
                                              text,
                                              maxWidth,
                                              PrimeFrame::WrapMode::Word);
  REQUIRE(lines.size() == 2u);
  CHECK(text.substr(lines[0].start, lines[0].end - lines[0].start) == "one two");
  CHECK(text.substr(lines[1].start, lines[1].end - lines[1].start) == "three");
}

TEST_CASE("text selection :: wrap word keeps long word intact") {
  auto frame = makeFrame();
  std::string_view text = "supercalifragilistic";
  float maxWidth = textWidth(frame, "super") * 0.5f;
  auto lines = PrimeStage::wrapTextLineRanges(frame,
                                              bodyToken(),
                                              text,
                                              maxWidth,
                                              PrimeFrame::WrapMode::Word);
  REQUIRE(lines.size() == 1u);
  CHECK(lines[0].start == 0u);
  CHECK(lines[0].end == text.size());
}

TEST_CASE("text selection :: wrap ignores leading spaces") {
  auto frame = makeFrame();
  std::string_view text = "   one two";
  float maxWidth = textWidth(frame, "one two");
  auto lines = PrimeStage::wrapTextLineRanges(frame,
                                              bodyToken(),
                                              text,
                                              maxWidth,
                                              PrimeFrame::WrapMode::Word);
  REQUIRE(!lines.empty());
  CHECK(lines[0].start == 3u);
}

TEST_CASE("text selection :: wrap produces ordered ranges") {
  auto frame = makeFrame();
  std::string_view text = "one two three four five";
  float maxWidth = textWidth(frame, "one two") + 0.1f;
  auto lines = PrimeStage::wrapTextLineRanges(frame,
                                              bodyToken(),
                                              text,
                                              maxWidth,
                                              PrimeFrame::WrapMode::Word);
  REQUIRE(!lines.empty());
  uint32_t lastEnd = 0u;
  for (const auto& line : lines) {
    CHECK(line.start <= line.end);
    CHECK(line.end <= text.size());
    CHECK(line.start >= lastEnd);
    lastEnd = line.end;
  }
  CHECK(lastEnd == text.size());
}

TEST_CASE("text selection :: caret hit test clamps to bounds") {
  auto frame = makeFrame();
  std::string_view text = "abcd";
  CHECK(PrimeStage::caretIndexForClick(frame, bodyToken(), text, 0.0f, -10.0f) == 0u);
  float width = textWidth(frame, text);
  CHECK(PrimeStage::caretIndexForClick(frame, bodyToken(), text, 0.0f, width + 10.0f) == text.size());
}

TEST_CASE("text selection :: caret hit test chooses closest boundary") {
  auto frame = makeFrame();
  std::string_view text = "abcd";
  float w1 = textWidth(frame, "a");
  float w2 = textWidth(frame, "ab");
  float nearFirst = w1 + (w2 - w1) * 0.3f;
  float nearSecond = w1 + (w2 - w1) * 0.7f;
  CHECK(PrimeStage::caretIndexForClick(frame, bodyToken(), text, 0.0f, nearFirst) == 1u);
  CHECK(PrimeStage::caretIndexForClick(frame, bodyToken(), text, 0.0f, nearSecond) == 2u);
}
