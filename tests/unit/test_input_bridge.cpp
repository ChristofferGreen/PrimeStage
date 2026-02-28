#include "PrimeStage/InputBridge.h"

#include "third_party/doctest.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

enum class ReplayStepKind {
  Pointer,
  Key,
  Text,
  Scroll,
};

struct ReplayStep {
  ReplayStepKind kind = ReplayStepKind::Pointer;
  PrimeHost::PointerEvent pointer{};
  PrimeHost::KeyEvent key{};
  PrimeHost::TextEvent text{};
  PrimeHost::ScrollEvent scroll{};
  std::string textBytes;
};

struct ReplayTrace {
  std::vector<ReplayStep> steps;
};

struct ReplayTraceLoadResult {
  ReplayTrace trace;
  std::string error;
  size_t errorLine = 0u;

  [[nodiscard]] bool ok() const {
    return error.empty();
  }
};

struct ReplaySummary {
  std::vector<PrimeFrame::Event> dispatchedEvents;
  std::vector<PrimeStage::InputBridgeResult> results;
  PrimeStage::InputBridgeState finalState{};
};

std::string toLower(std::string_view value) {
  std::string out(value);
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return out;
}

std::string_view trimView(std::string_view value) {
  while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
    value.remove_prefix(1u);
  }
  while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
    value.remove_suffix(1u);
  }
  return value;
}

ReplayTraceLoadResult makeReplayParseError(size_t line, std::string message) {
  ReplayTraceLoadResult result;
  result.errorLine = line;
  result.error = std::move(message);
  return result;
}

bool parseUnsigned(std::string const& token, uint32_t& out) {
  try {
    size_t consumed = 0u;
    unsigned long value = std::stoul(token, &consumed);
    if (consumed != token.size()) {
      return false;
    }
    out = static_cast<uint32_t>(value);
    return true;
  } catch (...) {
    return false;
  }
}

bool parseInt(std::string const& token, int32_t& out) {
  try {
    size_t consumed = 0u;
    long value = std::stol(token, &consumed);
    if (consumed != token.size()) {
      return false;
    }
    out = static_cast<int32_t>(value);
    return true;
  } catch (...) {
    return false;
  }
}

bool parseFloat(std::string const& token, float& out) {
  try {
    size_t consumed = 0u;
    float value = std::stof(token, &consumed);
    if (consumed != token.size()) {
      return false;
    }
    out = value;
    return true;
  } catch (...) {
    return false;
  }
}

std::optional<PrimeHost::PointerPhase> parsePointerPhase(std::string_view token) {
  std::string normalized = toLower(token);
  if (normalized == "down") {
    return PrimeHost::PointerPhase::Down;
  }
  if (normalized == "move") {
    return PrimeHost::PointerPhase::Move;
  }
  if (normalized == "up") {
    return PrimeHost::PointerPhase::Up;
  }
  if (normalized == "cancel") {
    return PrimeHost::PointerPhase::Cancel;
  }
  return std::nullopt;
}

std::optional<PrimeStage::HostKey> parseHostKey(std::string_view token) {
  std::string normalized = toLower(token);
  if (normalized == "enter") {
    return PrimeStage::HostKey::Enter;
  }
  if (normalized == "escape") {
    return PrimeStage::HostKey::Escape;
  }
  if (normalized == "space") {
    return PrimeStage::HostKey::Space;
  }
  if (normalized == "backspace") {
    return PrimeStage::HostKey::Backspace;
  }
  if (normalized == "left") {
    return PrimeStage::HostKey::Left;
  }
  if (normalized == "right") {
    return PrimeStage::HostKey::Right;
  }
  if (normalized == "up") {
    return PrimeStage::HostKey::Up;
  }
  if (normalized == "down") {
    return PrimeStage::HostKey::Down;
  }
  return std::nullopt;
}

std::filesystem::path replayTracePath(std::string_view fileName) {
  std::filesystem::path sourcePath = std::filesystem::path(__FILE__);
  return sourcePath.parent_path() / "input_replay" / std::string(fileName);
}

ReplayTraceLoadResult loadReplayTrace(std::filesystem::path const& path) {
  std::ifstream input(path);
  if (!input.good()) {
    return makeReplayParseError(0u, "unable to open replay trace: " + path.string());
  }

  ReplayTraceLoadResult result;
  std::string rawLine;
  size_t lineNumber = 0u;
  while (std::getline(input, rawLine)) {
    ++lineNumber;
    std::string_view lineView = trimView(rawLine);
    if (lineView.empty() || lineView.front() == '#') {
      continue;
    }

    std::vector<std::string> tokens;
    {
      std::istringstream tokenStream{std::string(lineView)};
      std::string token;
      while (tokenStream >> token) {
        tokens.push_back(token);
      }
    }
    if (tokens.empty()) {
      continue;
    }

    std::string command = toLower(tokens[0]);
    if (command == "pointer") {
      if (tokens.size() != 5u) {
        return makeReplayParseError(lineNumber,
                                    "pointer step expects: pointer <phase> <pointerId> <x> <y>");
      }
      std::optional<PrimeHost::PointerPhase> phase = parsePointerPhase(tokens[1]);
      if (!phase) {
        return makeReplayParseError(lineNumber,
                                    "invalid pointer phase '" + tokens[1] + "'");
      }
      uint32_t pointerId = 0u;
      int32_t x = 0;
      int32_t y = 0;
      if (!parseUnsigned(tokens[2], pointerId) || !parseInt(tokens[3], x) ||
          !parseInt(tokens[4], y)) {
        return makeReplayParseError(lineNumber, "invalid pointer numeric argument");
      }

      ReplayStep step;
      step.kind = ReplayStepKind::Pointer;
      step.pointer.pointerId = pointerId;
      step.pointer.phase = *phase;
      step.pointer.x = x;
      step.pointer.y = y;
      result.trace.steps.push_back(std::move(step));
      continue;
    }

    if (command == "key") {
      if (tokens.size() != 3u && tokens.size() != 4u) {
        return makeReplayParseError(lineNumber,
                                    "key step expects: key <down|up> <key> [modifiers]");
      }
      std::string action = toLower(tokens[1]);
      if (action != "down" && action != "up") {
        return makeReplayParseError(lineNumber,
                                    "key action must be 'down' or 'up'");
      }
      std::optional<PrimeStage::HostKey> key = parseHostKey(tokens[2]);
      if (!key) {
        return makeReplayParseError(lineNumber,
                                    "unsupported host key '" + tokens[2] + "'");
      }

      uint32_t modifiers = 0u;
      if (tokens.size() == 4u && !parseUnsigned(tokens[3], modifiers)) {
        return makeReplayParseError(lineNumber, "invalid key modifiers value");
      }

      ReplayStep step;
      step.kind = ReplayStepKind::Key;
      step.key.pressed = action == "down";
      step.key.keyCode = PrimeStage::hostKeyCode(*key);
      step.key.modifiers = static_cast<PrimeHost::KeyModifierMask>(modifiers);
      result.trace.steps.push_back(std::move(step));
      continue;
    }

    if (command == "text") {
      size_t separator = lineView.find_first_of(" \t");
      if (separator == std::string_view::npos) {
        return makeReplayParseError(lineNumber, "text step requires payload bytes");
      }
      std::string_view payload = trimView(lineView.substr(separator + 1u));
      if (payload.size() >= 2u && payload.front() == '"' && payload.back() == '"') {
        payload.remove_prefix(1u);
        payload.remove_suffix(1u);
      }

      ReplayStep step;
      step.kind = ReplayStepKind::Text;
      step.textBytes.assign(payload.begin(), payload.end());
      step.text.text.offset = 0u;
      step.text.text.length = static_cast<uint32_t>(step.textBytes.size());
      result.trace.steps.push_back(std::move(step));
      continue;
    }

    if (command == "scroll") {
      if (tokens.size() != 4u) {
        return makeReplayParseError(lineNumber,
                                    "scroll step expects: scroll <deltaX> <deltaY> <lines|pixels>");
      }
      float deltaX = 0.0f;
      float deltaY = 0.0f;
      if (!parseFloat(tokens[1], deltaX) || !parseFloat(tokens[2], deltaY)) {
        return makeReplayParseError(lineNumber, "invalid scroll numeric argument");
      }
      std::string units = toLower(tokens[3]);
      if (units != "lines" && units != "pixels") {
        return makeReplayParseError(lineNumber,
                                    "scroll units must be 'lines' or 'pixels'");
      }

      ReplayStep step;
      step.kind = ReplayStepKind::Scroll;
      step.scroll.deltaX = deltaX;
      step.scroll.deltaY = deltaY;
      step.scroll.isLines = units == "lines";
      result.trace.steps.push_back(std::move(step));
      continue;
    }

    return makeReplayParseError(lineNumber, "unsupported replay command '" + tokens[0] + "'");
  }

  return result;
}

ReplaySummary replayTrace(ReplayTrace const& trace) {
  ReplaySummary summary;
  PrimeStage::InputBridgeState state;

  for (ReplayStep const& step : trace.steps) {
    PrimeHost::InputEvent input;
    PrimeHost::EventBatch batch{};
    std::vector<char> textBytes;

    switch (step.kind) {
      case ReplayStepKind::Pointer:
        input = step.pointer;
        break;
      case ReplayStepKind::Key:
        input = step.key;
        break;
      case ReplayStepKind::Text: {
        PrimeHost::TextEvent text = step.text;
        textBytes.assign(step.textBytes.begin(), step.textBytes.end());
        text.text.length = static_cast<uint32_t>(textBytes.size());
        input = text;
        batch = PrimeHost::EventBatch{
            std::span<const PrimeHost::Event>{},
            std::span<const char>(textBytes.data(), textBytes.size()),
        };
        break;
      }
      case ReplayStepKind::Scroll:
        input = step.scroll;
        break;
    }

    PrimeStage::InputBridgeResult result = PrimeStage::bridgeHostInputEvent(
        input,
        batch,
        state,
        [&](PrimeFrame::Event const& event) {
          summary.dispatchedEvents.push_back(event);
          return true;
        });
    summary.results.push_back(result);
  }

  summary.finalState = state;
  return summary;
}

size_t countRequestFrameResults(ReplaySummary const& summary) {
  return static_cast<size_t>(
      std::count_if(summary.results.begin(), summary.results.end(), [](auto const& result) {
        return result.requestFrame;
      }));
}

size_t countBypassFrameCapResults(ReplaySummary const& summary) {
  return static_cast<size_t>(
      std::count_if(summary.results.begin(), summary.results.end(), [](auto const& result) {
        return result.bypassFrameCap;
      }));
}

size_t countExitResults(ReplaySummary const& summary) {
  return static_cast<size_t>(
      std::count_if(summary.results.begin(), summary.results.end(), [](auto const& result) {
        return result.requestExit;
      }));
}

} // namespace

TEST_CASE("Input bridge maps pointer events and updates pointer state") {
  PrimeStage::InputBridgeState state;
  PrimeHost::PointerEvent pointer;
  pointer.pointerId = 7u;
  pointer.x = 25;
  pointer.y = 40;
  pointer.phase = PrimeHost::PointerPhase::Down;
  PrimeHost::InputEvent input = pointer;

  PrimeHost::EventBatch batch{};
  PrimeFrame::Event captured;
  bool dispatched = false;

  PrimeStage::InputBridgeResult result = PrimeStage::bridgeHostInputEvent(
      input,
      batch,
      state,
      [&](PrimeFrame::Event const& event) {
        captured = event;
        dispatched = true;
        return true;
      });

  CHECK(dispatched);
  CHECK(captured.type == PrimeFrame::EventType::PointerDown);
  CHECK(captured.pointerId == 7);
  CHECK(captured.x == doctest::Approx(25.0f));
  CHECK(captured.y == doctest::Approx(40.0f));
  CHECK(state.pointerX == doctest::Approx(25.0f));
  CHECK(state.pointerY == doctest::Approx(40.0f));
  CHECK(result.requestFrame);
  CHECK(result.bypassFrameCap);
  CHECK_FALSE(result.requestExit);
}

TEST_CASE("Input bridge textFromHostSpan enforces bounds and supports empty spans") {
  std::array<char, 4> textBytes{'A', 'B', 'C', 'D'};
  PrimeHost::EventBatch batch{
      std::span<const PrimeHost::Event>{},
      std::span<const char>(textBytes.data(), textBytes.size()),
  };

  PrimeHost::TextSpan exact;
  exact.offset = 2u;
  exact.length = 2u;
  std::optional<std::string_view> exactView = PrimeStage::textFromHostSpan(batch, exact);
  REQUIRE(exactView.has_value());
  CHECK(*exactView == "CD");

  PrimeHost::TextSpan outOfBounds;
  outOfBounds.offset = 4u;
  outOfBounds.length = 1u;
  CHECK_FALSE(PrimeStage::textFromHostSpan(batch, outOfBounds).has_value());

  PrimeHost::TextSpan empty;
  empty.offset = 99u;
  empty.length = 0u;
  std::optional<std::string_view> emptyView = PrimeStage::textFromHostSpan(batch, empty);
  REQUIRE(emptyView.has_value());
  CHECK(emptyView->empty());
}

TEST_CASE("Input bridge isHostKeyPressed requires pressed state and matching key code") {
  PrimeHost::KeyEvent event;
  event.pressed = false;
  event.keyCode = PrimeStage::hostKeyCode(PrimeStage::HostKey::Escape);
  CHECK_FALSE(PrimeStage::isHostKeyPressed(event, PrimeStage::HostKey::Escape));

  event.pressed = true;
  event.keyCode = PrimeStage::hostKeyCode(PrimeStage::HostKey::Enter);
  CHECK_FALSE(PrimeStage::isHostKeyPressed(event, PrimeStage::HostKey::Escape));

  event.keyCode = PrimeStage::hostKeyCode(PrimeStage::HostKey::Escape);
  CHECK(PrimeStage::isHostKeyPressed(event, PrimeStage::HostKey::Escape));
}

TEST_CASE("Input bridge maps pointer cancel and keeps bypass flag even when unhandled") {
  PrimeStage::InputBridgeState state;
  PrimeHost::PointerEvent pointer;
  pointer.pointerId = 4u;
  pointer.x = 11;
  pointer.y = 13;
  pointer.phase = PrimeHost::PointerPhase::Cancel;
  PrimeHost::InputEvent input = pointer;
  PrimeHost::EventBatch batch{};

  PrimeFrame::Event captured;
  bool dispatched = false;
  PrimeStage::InputBridgeResult result = PrimeStage::bridgeHostInputEvent(
      input,
      batch,
      state,
      [&](PrimeFrame::Event const& event) {
        captured = event;
        dispatched = true;
        return false;
      });

  CHECK(dispatched);
  CHECK(captured.type == PrimeFrame::EventType::PointerCancel);
  CHECK(captured.pointerId == 4);
  CHECK(captured.x == doctest::Approx(11.0f));
  CHECK(captured.y == doctest::Approx(13.0f));
  CHECK(state.pointerX == doctest::Approx(11.0f));
  CHECK(state.pointerY == doctest::Approx(13.0f));
  CHECK_FALSE(result.requestFrame);
  CHECK(result.bypassFrameCap);
  CHECK_FALSE(result.requestExit);
}

TEST_CASE("Input bridge maps key events and uses symbolic escape key") {
  PrimeStage::InputBridgeState state;
  PrimeHost::EventBatch batch{};

  PrimeHost::KeyEvent escape;
  escape.pressed = true;
  escape.keyCode = PrimeStage::hostKeyCode(PrimeStage::HostKey::Escape);
  PrimeHost::InputEvent input = escape;

  bool dispatched = false;
  PrimeStage::InputBridgeResult result = PrimeStage::bridgeHostInputEvent(
      input,
      batch,
      state,
      [&](PrimeFrame::Event const&) {
        dispatched = true;
        return true;
      });
  CHECK_FALSE(dispatched);
  CHECK(result.requestExit);
  CHECK_FALSE(result.requestFrame);

  PrimeHost::KeyEvent keyDown;
  keyDown.pressed = true;
  keyDown.keyCode = PrimeStage::hostKeyCode(PrimeStage::HostKey::Enter);
  keyDown.modifiers = static_cast<PrimeHost::KeyModifierMask>(PrimeHost::KeyModifier::Shift);
  input = keyDown;

  PrimeFrame::Event captured;
  result = PrimeStage::bridgeHostInputEvent(
      input,
      batch,
      state,
      [&](PrimeFrame::Event const& event) {
        captured = event;
        return true;
      });

  CHECK(captured.type == PrimeFrame::EventType::KeyDown);
  CHECK(captured.key == static_cast<int>(PrimeStage::hostKeyCode(PrimeStage::HostKey::Enter)));
  CHECK(captured.modifiers == keyDown.modifiers);
  CHECK_FALSE(result.requestExit);
  CHECK(result.requestFrame);
}

TEST_CASE("Input bridge uses configured exit key for key events") {
  PrimeStage::InputBridgeState state;
  PrimeHost::EventBatch batch{};

  PrimeHost::KeyEvent enter;
  enter.pressed = true;
  enter.keyCode = PrimeStage::hostKeyCode(PrimeStage::HostKey::Enter);
  PrimeHost::InputEvent input = enter;

  bool dispatched = false;
  PrimeStage::InputBridgeResult result = PrimeStage::bridgeHostInputEvent(
      input,
      batch,
      state,
      [&](PrimeFrame::Event const&) {
        dispatched = true;
        return true;
      },
      PrimeStage::HostKey::Enter);

  CHECK_FALSE(dispatched);
  CHECK(result.requestExit);
  CHECK_FALSE(result.requestFrame);
  CHECK_FALSE(result.bypassFrameCap);
}

TEST_CASE("Input bridge dispatches when key does not match configured exit key") {
  PrimeStage::InputBridgeState state;
  PrimeHost::EventBatch batch{};

  PrimeHost::KeyEvent escape;
  escape.pressed = true;
  escape.keyCode = PrimeStage::hostKeyCode(PrimeStage::HostKey::Escape);
  escape.modifiers = static_cast<PrimeHost::KeyModifierMask>(PrimeHost::KeyModifier::Control);
  PrimeHost::InputEvent input = escape;

  PrimeFrame::Event captured;
  bool dispatched = false;
  PrimeStage::InputBridgeResult result = PrimeStage::bridgeHostInputEvent(
      input,
      batch,
      state,
      [&](PrimeFrame::Event const& event) {
        captured = event;
        dispatched = true;
        return true;
      },
      PrimeStage::HostKey::Enter);

  CHECK(dispatched);
  CHECK(captured.type == PrimeFrame::EventType::KeyDown);
  CHECK(captured.key == static_cast<int>(PrimeStage::hostKeyCode(PrimeStage::HostKey::Escape)));
  CHECK(captured.modifiers == escape.modifiers);
  CHECK_FALSE(result.requestExit);
  CHECK(result.requestFrame);
  CHECK_FALSE(result.bypassFrameCap);
}

TEST_CASE("Input bridge key down keeps requestFrame false when dispatch is unhandled") {
  PrimeStage::InputBridgeState state;
  PrimeHost::EventBatch batch{};

  PrimeHost::KeyEvent keyDown;
  keyDown.pressed = true;
  keyDown.keyCode = PrimeStage::hostKeyCode(PrimeStage::HostKey::Space);
  keyDown.modifiers = static_cast<PrimeHost::KeyModifierMask>(PrimeHost::KeyModifier::Alt);
  PrimeHost::InputEvent input = keyDown;

  PrimeFrame::Event captured;
  bool dispatched = false;
  PrimeStage::InputBridgeResult result = PrimeStage::bridgeHostInputEvent(
      input,
      batch,
      state,
      [&](PrimeFrame::Event const& event) {
        captured = event;
        dispatched = true;
        return false;
      });

  CHECK(dispatched);
  CHECK(captured.type == PrimeFrame::EventType::KeyDown);
  CHECK(captured.key == static_cast<int>(PrimeStage::hostKeyCode(PrimeStage::HostKey::Space)));
  CHECK(captured.modifiers == keyDown.modifiers);
  CHECK_FALSE(result.requestExit);
  CHECK_FALSE(result.requestFrame);
  CHECK_FALSE(result.bypassFrameCap);
}

TEST_CASE("Input bridge maps escape release to key up without requesting exit") {
  PrimeStage::InputBridgeState state;
  PrimeHost::EventBatch batch{};

  PrimeHost::KeyEvent escapeUp;
  escapeUp.pressed = false;
  escapeUp.keyCode = PrimeStage::hostKeyCode(PrimeStage::HostKey::Escape);
  escapeUp.modifiers = static_cast<PrimeHost::KeyModifierMask>(PrimeHost::KeyModifier::Control);
  PrimeHost::InputEvent input = escapeUp;

  PrimeFrame::Event captured;
  bool dispatched = false;
  PrimeStage::InputBridgeResult result = PrimeStage::bridgeHostInputEvent(
      input,
      batch,
      state,
      [&](PrimeFrame::Event const& event) {
        captured = event;
        dispatched = true;
        return false;
      });

  CHECK(dispatched);
  CHECK(captured.type == PrimeFrame::EventType::KeyUp);
  CHECK(captured.key == static_cast<int>(PrimeStage::hostKeyCode(PrimeStage::HostKey::Escape)));
  CHECK(captured.modifiers == escapeUp.modifiers);
  CHECK_FALSE(result.requestExit);
  CHECK_FALSE(result.requestFrame);
  CHECK_FALSE(result.bypassFrameCap);
}

TEST_CASE("Input bridge key release requests frame when dispatch handles event") {
  PrimeStage::InputBridgeState state;
  PrimeHost::EventBatch batch{};

  PrimeHost::KeyEvent keyUp;
  keyUp.pressed = false;
  keyUp.keyCode = PrimeStage::hostKeyCode(PrimeStage::HostKey::Left);
  keyUp.modifiers = static_cast<PrimeHost::KeyModifierMask>(PrimeHost::KeyModifier::Shift);
  PrimeHost::InputEvent input = keyUp;

  PrimeFrame::Event captured;
  bool dispatched = false;
  PrimeStage::InputBridgeResult result = PrimeStage::bridgeHostInputEvent(
      input,
      batch,
      state,
      [&](PrimeFrame::Event const& event) {
        captured = event;
        dispatched = true;
        return true;
      });

  CHECK(dispatched);
  CHECK(captured.type == PrimeFrame::EventType::KeyUp);
  CHECK(captured.key == static_cast<int>(PrimeStage::hostKeyCode(PrimeStage::HostKey::Left)));
  CHECK(captured.modifiers == keyUp.modifiers);
  CHECK_FALSE(result.requestExit);
  CHECK(result.requestFrame);
  CHECK_FALSE(result.bypassFrameCap);
}

TEST_CASE("Input bridge maps text spans and ignores invalid spans") {
  PrimeStage::InputBridgeState state;
  std::array<char, 8> textBytes{'P', 'r', 'i', 'm', 'e', '!', '\0', '\0'};
  PrimeHost::EventBatch batch{
      std::span<const PrimeHost::Event>{},
      std::span<const char>(textBytes.data(), 6u),
  };

  PrimeHost::TextEvent text;
  text.text.offset = 1u;
  text.text.length = 4u;
  PrimeHost::InputEvent input = text;

  PrimeFrame::Event captured;
  bool dispatched = false;
  PrimeStage::InputBridgeResult result = PrimeStage::bridgeHostInputEvent(
      input,
      batch,
      state,
      [&](PrimeFrame::Event const& event) {
        captured = event;
        dispatched = true;
        return true;
      });

  CHECK(dispatched);
  CHECK(captured.type == PrimeFrame::EventType::TextInput);
  CHECK(captured.text == "rime");
  CHECK(result.requestFrame);

  text.text.offset = 5u;
  text.text.length = 4u;
  input = text;
  dispatched = false;
  result = PrimeStage::bridgeHostInputEvent(
      input,
      batch,
      state,
      [&](PrimeFrame::Event const&) {
        dispatched = true;
        return true;
      });
  CHECK_FALSE(dispatched);
  CHECK_FALSE(result.requestFrame);
}

TEST_CASE("Input bridge text events propagate unhandled dispatch as no frame request") {
  PrimeStage::InputBridgeState state;
  std::array<char, 8> textBytes{'t', 'e', 'x', 't', '\0', '\0', '\0', '\0'};
  PrimeHost::EventBatch batch{
      std::span<const PrimeHost::Event>{},
      std::span<const char>(textBytes.data(), 4u),
  };

  PrimeHost::TextEvent text;
  text.text.offset = 0u;
  text.text.length = 4u;
  PrimeHost::InputEvent input = text;

  PrimeFrame::Event captured;
  bool dispatched = false;
  PrimeStage::InputBridgeResult result = PrimeStage::bridgeHostInputEvent(
      input,
      batch,
      state,
      [&](PrimeFrame::Event const& event) {
        captured = event;
        dispatched = true;
        return false;
      });

  CHECK(dispatched);
  CHECK(captured.type == PrimeFrame::EventType::TextInput);
  CHECK(captured.text == "text");
  CHECK_FALSE(result.requestFrame);
  CHECK_FALSE(result.requestExit);
  CHECK_FALSE(result.bypassFrameCap);
}

TEST_CASE("Input bridge dispatches empty text spans as empty text input") {
  PrimeStage::InputBridgeState state;
  PrimeHost::TextEvent text;
  text.text.offset = 99u;
  text.text.length = 0u;
  PrimeHost::InputEvent input = text;
  PrimeHost::EventBatch batch{};

  PrimeFrame::Event captured;
  bool dispatched = false;
  PrimeStage::InputBridgeResult result = PrimeStage::bridgeHostInputEvent(
      input,
      batch,
      state,
      [&](PrimeFrame::Event const& event) {
        captured = event;
        dispatched = true;
        return true;
      });

  CHECK(dispatched);
  CHECK(captured.type == PrimeFrame::EventType::TextInput);
  CHECK(captured.text.empty());
  CHECK(result.requestFrame);
  CHECK_FALSE(result.requestExit);
  CHECK_FALSE(result.bypassFrameCap);
}

TEST_CASE("Input bridge ignores unsupported input variants") {
  PrimeStage::InputBridgeState state;
  PrimeHost::GamepadButtonEvent gamepad;
  gamepad.deviceId = 5u;
  gamepad.controlId = static_cast<uint32_t>(PrimeHost::GamepadButtonId::South);
  gamepad.pressed = true;
  PrimeHost::InputEvent input = gamepad;
  PrimeHost::EventBatch batch{};

  bool dispatched = false;
  PrimeStage::InputBridgeResult result = PrimeStage::bridgeHostInputEvent(
      input,
      batch,
      state,
      [&](PrimeFrame::Event const&) {
        dispatched = true;
        return true;
      });

  CHECK_FALSE(dispatched);
  CHECK_FALSE(result.requestFrame);
  CHECK_FALSE(result.requestExit);
  CHECK_FALSE(result.bypassFrameCap);
}

TEST_CASE("Input bridge unsupported variants preserve pointer state") {
  PrimeStage::InputBridgeState state;
  state.pointerX = 123.0f;
  state.pointerY = -45.0f;

  PrimeHost::DeviceEvent device;
  device.deviceId = 9u;
  device.deviceType = PrimeHost::DeviceType::Gamepad;
  device.connected = false;
  PrimeHost::InputEvent input = device;
  PrimeHost::EventBatch batch{};

  bool dispatched = false;
  PrimeStage::InputBridgeResult result = PrimeStage::bridgeHostInputEvent(
      input,
      batch,
      state,
      [&](PrimeFrame::Event const&) {
        dispatched = true;
        return true;
      });

  CHECK_FALSE(dispatched);
  CHECK_FALSE(result.requestFrame);
  CHECK_FALSE(result.requestExit);
  CHECK_FALSE(result.bypassFrameCap);
  CHECK(state.pointerX == doctest::Approx(123.0f));
  CHECK(state.pointerY == doctest::Approx(-45.0f));
}

TEST_CASE("Input bridge maps scroll events using pointer position and line scale") {
  PrimeStage::InputBridgeState state;
  state.pointerX = 12.0f;
  state.pointerY = 34.0f;
  state.scrollLinePixels = 16.0f;

  PrimeHost::ScrollEvent scroll;
  scroll.deltaX = 1.5f;
  scroll.deltaY = -2.0f;
  scroll.isLines = true;
  PrimeHost::InputEvent input = scroll;
  PrimeHost::EventBatch batch{};

  PrimeFrame::Event captured;
  PrimeStage::InputBridgeResult result = PrimeStage::bridgeHostInputEvent(
      input,
      batch,
      state,
      [&](PrimeFrame::Event const& event) {
        captured = event;
        return true;
      });

  CHECK(captured.type == PrimeFrame::EventType::PointerScroll);
  CHECK(captured.x == doctest::Approx(12.0f));
  CHECK(captured.y == doctest::Approx(34.0f));
  CHECK(captured.scrollX == doctest::Approx(24.0f));
  CHECK(captured.scrollY == doctest::Approx(-32.0f));
  CHECK(result.requestFrame);
  CHECK(result.bypassFrameCap);
}

TEST_CASE("Input bridge scroll keeps bypass frame cap when dispatch is unhandled") {
  PrimeStage::InputBridgeState state;
  state.pointerX = 21.0f;
  state.pointerY = 55.0f;

  PrimeHost::ScrollEvent scroll;
  scroll.deltaX = -2.0f;
  scroll.deltaY = 3.0f;
  scroll.isLines = false;
  PrimeHost::InputEvent input = scroll;
  PrimeHost::EventBatch batch{};

  PrimeFrame::Event captured;
  bool dispatched = false;
  PrimeStage::InputBridgeResult result = PrimeStage::bridgeHostInputEvent(
      input,
      batch,
      state,
      [&](PrimeFrame::Event const& event) {
        captured = event;
        dispatched = true;
        return false;
      });

  CHECK(dispatched);
  CHECK(captured.type == PrimeFrame::EventType::PointerScroll);
  CHECK(captured.x == doctest::Approx(21.0f));
  CHECK(captured.y == doctest::Approx(55.0f));
  CHECK(captured.scrollX == doctest::Approx(-2.0f));
  CHECK(captured.scrollY == doctest::Approx(3.0f));
  CHECK_FALSE(result.requestFrame);
  CHECK(result.bypassFrameCap);
  CHECK_FALSE(result.requestExit);
}

TEST_CASE("Input bridge preserves pixel scroll units and normalizes direction sign") {
  PrimeStage::InputBridgeState state;
  state.pointerX = 48.0f;
  state.pointerY = 96.0f;
  state.scrollLinePixels = 100.0f; // ignored for pixel-mode events
  state.scrollDirectionSign = -1.0f;

  PrimeHost::ScrollEvent scroll;
  scroll.deltaX = 6.0f;
  scroll.deltaY = -3.0f;
  scroll.isLines = false;
  PrimeHost::InputEvent input = scroll;
  PrimeHost::EventBatch batch{};

  PrimeFrame::Event captured;
  PrimeStage::InputBridgeResult result = PrimeStage::bridgeHostInputEvent(
      input,
      batch,
      state,
      [&](PrimeFrame::Event const& event) {
        captured = event;
        return true;
      });

  CHECK(captured.type == PrimeFrame::EventType::PointerScroll);
  CHECK(captured.x == doctest::Approx(48.0f));
  CHECK(captured.y == doctest::Approx(96.0f));
  CHECK(captured.scrollX == doctest::Approx(-6.0f));
  CHECK(captured.scrollY == doctest::Approx(3.0f));
  CHECK(result.requestFrame);
  CHECK(result.bypassFrameCap);
}

TEST_CASE("Input bridge applies direction sign to line-based scroll deltas") {
  PrimeStage::InputBridgeState state;
  state.pointerX = 5.0f;
  state.pointerY = 9.0f;
  state.scrollLinePixels = 20.0f;
  state.scrollDirectionSign = -1.0f;

  PrimeHost::ScrollEvent scroll;
  scroll.deltaX = 1.0f;
  scroll.deltaY = -0.5f;
  scroll.isLines = true;
  PrimeHost::InputEvent input = scroll;
  PrimeHost::EventBatch batch{};

  PrimeFrame::Event captured;
  PrimeStage::InputBridgeResult result = PrimeStage::bridgeHostInputEvent(
      input,
      batch,
      state,
      [&](PrimeFrame::Event const& event) {
        captured = event;
        return true;
      });

  CHECK(captured.type == PrimeFrame::EventType::PointerScroll);
  CHECK(captured.x == doctest::Approx(5.0f));
  CHECK(captured.y == doctest::Approx(9.0f));
  CHECK(captured.scrollX == doctest::Approx(-20.0f));
  CHECK(captured.scrollY == doctest::Approx(10.0f));
  CHECK(result.requestFrame);
  CHECK(result.bypassFrameCap);
  CHECK_FALSE(result.requestExit);
}

TEST_CASE("Input bridge treats non-negative direction sign as default orientation") {
  PrimeStage::InputBridgeState state;
  state.scrollDirectionSign = 0.0f;

  PrimeHost::ScrollEvent scroll;
  scroll.deltaX = 1.0f;
  scroll.deltaY = 2.0f;
  scroll.isLines = true;
  PrimeHost::InputEvent input = scroll;
  PrimeHost::EventBatch batch{};

  PrimeFrame::Event captured;
  PrimeStage::InputBridgeResult result = PrimeStage::bridgeHostInputEvent(
      input,
      batch,
      state,
      [&](PrimeFrame::Event const& event) {
        captured = event;
        return true;
      });

  CHECK(captured.scrollX == doctest::Approx(32.0f));
  CHECK(captured.scrollY == doctest::Approx(64.0f));
  CHECK(result.requestFrame);
}

TEST_CASE("Input bridge replays mixed host-input trace fixture deterministically") {
  ReplayTraceLoadResult loaded = loadReplayTrace(replayTracePath("mixed_input.trace"));
  INFO("error line: " << loaded.errorLine);
  INFO("error: " << loaded.error);
  REQUIRE(loaded.ok());
  REQUIRE(loaded.trace.steps.size() == 6u);

  ReplaySummary summary = replayTrace(loaded.trace);
  REQUIRE(summary.results.size() == 6u);
  CHECK(countRequestFrameResults(summary) == 5u);
  CHECK(countBypassFrameCapResults(summary) == 3u);
  CHECK(countExitResults(summary) == 1u);

  REQUIRE(summary.dispatchedEvents.size() == 5u);
  PrimeFrame::Event const& pointerDown = summary.dispatchedEvents[0];
  CHECK(pointerDown.type == PrimeFrame::EventType::PointerDown);
  CHECK(pointerDown.pointerId == 1);
  CHECK(pointerDown.x == doctest::Approx(25.0f));
  CHECK(pointerDown.y == doctest::Approx(40.0f));

  PrimeFrame::Event const& pointerUp = summary.dispatchedEvents[1];
  CHECK(pointerUp.type == PrimeFrame::EventType::PointerUp);
  CHECK(pointerUp.pointerId == 1);

  PrimeFrame::Event const& keyDown = summary.dispatchedEvents[2];
  CHECK(keyDown.type == PrimeFrame::EventType::KeyDown);
  CHECK(keyDown.key == static_cast<int>(PrimeStage::hostKeyCode(PrimeStage::HostKey::Enter)));
  CHECK(keyDown.modifiers == 1u);

  PrimeFrame::Event const& textInput = summary.dispatchedEvents[3];
  CHECK(textInput.type == PrimeFrame::EventType::TextInput);
  CHECK(textInput.text == "Prime");

  PrimeFrame::Event const& scroll = summary.dispatchedEvents[4];
  CHECK(scroll.type == PrimeFrame::EventType::PointerScroll);
  CHECK(scroll.x == doctest::Approx(25.0f));
  CHECK(scroll.y == doctest::Approx(40.0f));
  CHECK(scroll.scrollX == doctest::Approx(48.0f));
  CHECK(scroll.scrollY == doctest::Approx(-64.0f));

  CHECK(summary.finalState.pointerX == doctest::Approx(25.0f));
  CHECK(summary.finalState.pointerY == doctest::Approx(40.0f));
}

TEST_CASE("Input bridge replays scroll-orientation trace fixture deterministically") {
  ReplayTraceLoadResult loaded = loadReplayTrace(replayTracePath("scroll_direction.trace"));
  INFO("error line: " << loaded.errorLine);
  INFO("error: " << loaded.error);
  REQUIRE(loaded.ok());
  REQUIRE(loaded.trace.steps.size() == 3u);

  ReplaySummary summary = replayTrace(loaded.trace);
  REQUIRE(summary.results.size() == 3u);
  CHECK(countRequestFrameResults(summary) == 3u);
  CHECK(countBypassFrameCapResults(summary) == 3u);
  CHECK(countExitResults(summary) == 0u);

  REQUIRE(summary.dispatchedEvents.size() == 3u);
  CHECK(summary.dispatchedEvents[0].type == PrimeFrame::EventType::PointerMove);
  CHECK(summary.dispatchedEvents[0].x == doctest::Approx(48.0f));
  CHECK(summary.dispatchedEvents[0].y == doctest::Approx(96.0f));

  CHECK(summary.dispatchedEvents[1].type == PrimeFrame::EventType::PointerScroll);
  CHECK(summary.dispatchedEvents[1].x == doctest::Approx(48.0f));
  CHECK(summary.dispatchedEvents[1].y == doctest::Approx(96.0f));
  CHECK(summary.dispatchedEvents[1].scrollX == doctest::Approx(6.0f));
  CHECK(summary.dispatchedEvents[1].scrollY == doctest::Approx(-3.0f));

  CHECK(summary.dispatchedEvents[2].type == PrimeFrame::EventType::PointerScroll);
  CHECK(summary.dispatchedEvents[2].x == doctest::Approx(48.0f));
  CHECK(summary.dispatchedEvents[2].y == doctest::Approx(96.0f));
  CHECK(summary.dispatchedEvents[2].scrollX == doctest::Approx(32.0f));
  CHECK(summary.dispatchedEvents[2].scrollY == doctest::Approx(64.0f));

  CHECK(summary.finalState.pointerX == doctest::Approx(48.0f));
  CHECK(summary.finalState.pointerY == doctest::Approx(96.0f));
}

TEST_CASE("Input bridge replay parser reports deterministic diagnostics for invalid traces") {
  ReplayTraceLoadResult loaded = loadReplayTrace(replayTracePath("invalid_trace.trace"));
  CHECK_FALSE(loaded.ok());
  CHECK(loaded.errorLine == 1u);
  CHECK(loaded.error.find("invalid pointer phase") != std::string::npos);
}
