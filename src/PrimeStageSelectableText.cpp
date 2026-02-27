#include "PrimeStage/PrimeStage.h"

#include "PrimeStage/TextSelection.h"
#include "PrimeStageCollectionInternals.h"
#include "PrimeFrame/Events.h"

#include <algorithm>
#include <cctype>
#include <memory>

namespace PrimeStage {
namespace {

static bool is_word_char(std::string_view text, uint32_t index) {
  if (index >= text.size()) {
    return false;
  }
  unsigned char ch = static_cast<unsigned char>(text[index]);
  if (ch >= 0x80u) {
    return true;
  }
  return std::isalnum(ch) || ch == '_';
}

static bool is_space_char(std::string_view text, uint32_t index) {
  if (index >= text.size()) {
    return false;
  }
  unsigned char ch = static_cast<unsigned char>(text[index]);
  return std::isspace(ch) != 0;
}

static uint32_t prev_word_boundary(std::string_view text, uint32_t cursor) {
  if (cursor == 0u) {
    return 0u;
  }
  uint32_t i = utf8Prev(text, cursor);
  while (i > 0u && is_space_char(text, i)) {
    i = utf8Prev(text, i);
  }
  if (is_word_char(text, i)) {
    while (i > 0u) {
      uint32_t prev = utf8Prev(text, i);
      if (!is_word_char(text, prev)) {
        break;
      }
      i = prev;
    }
    return i;
  }
  while (i > 0u && !is_word_char(text, i)) {
    i = utf8Prev(text, i);
  }
  if (!is_word_char(text, i)) {
    return 0u;
  }
  while (i > 0u) {
    uint32_t prev = utf8Prev(text, i);
    if (!is_word_char(text, prev)) {
      break;
    }
    i = prev;
  }
  return i;
}

static uint32_t next_word_boundary(std::string_view text, uint32_t cursor) {
  uint32_t size = static_cast<uint32_t>(text.size());
  if (cursor >= size) {
    return size;
  }
  uint32_t i = cursor;
  if (is_word_char(text, i)) {
    while (i < size && is_word_char(text, i)) {
      i = utf8Next(text, i);
    }
    return i;
  }
  while (i < size && !is_word_char(text, i)) {
    i = utf8Next(text, i);
  }
  return i;
}

} // namespace

UiNode UiNode::createSelectableText(SelectableTextSpec const& specInput) {
  SelectableTextSpec spec = Internal::normalizeSelectableTextSpec(specInput);
  bool enabled = spec.enabled;

  Internal::InternalRect bounds = Internal::resolveRect(spec.size);
  std::shared_ptr<SelectableTextState> stateOwner = spec.ownedState;
  SelectableTextState* state = spec.state;
  if (!state) {
    if (!stateOwner) {
      stateOwner = std::make_shared<SelectableTextState>();
    }
    state = stateOwner.get();
  }
  std::string_view text = spec.text;
  float maxWidth = spec.maxWidth;
  if (maxWidth <= 0.0f && spec.size.maxWidth.has_value()) {
    maxWidth = std::max(0.0f, *spec.size.maxWidth - spec.paddingX * 2.0f);
  }
  if (maxWidth <= 0.0f && bounds.width > 0.0f) {
    float available = bounds.width - spec.paddingX * 2.0f;
    maxWidth = std::max(0.0f, available);
  }
  if (maxWidth <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f &&
      !text.empty()) {
    maxWidth = Internal::defaultSelectableTextWrapWidth();
  }

  TextSelectionLayout layout =
      buildTextSelectionLayout(frame(), spec.textStyle, text, maxWidth, spec.wrap);
  if (layout.lineHeight <= 0.0f) {
    layout.lineHeight = Internal::resolveLineHeight(frame(), spec.textStyle);
  }
  size_t lineCount = std::max<size_t>(1u, layout.lines.size());
  float textHeight = layout.lineHeight * static_cast<float>(lineCount);
  float textWidth = 0.0f;
  for (auto const& line : layout.lines) {
    textWidth = std::max(textWidth, line.width);
  }
  float desiredWidth = (maxWidth > 0.0f ? maxWidth : textWidth) + spec.paddingX * 2.0f;

  if (bounds.width <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f) {
    bounds.width = desiredWidth;
  }
  if (bounds.width <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX > 0.0f &&
      maxWidth > 0.0f) {
    bounds.width = desiredWidth;
  }
  if (bounds.height <= 0.0f &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchY <= 0.0f) {
    bounds.height = textHeight;
  }
  if (bounds.width <= 0.0f &&
      bounds.height <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchX <= 0.0f &&
      spec.size.stretchY <= 0.0f) {
    return UiNode(frame(), id_, allowAbsolute_);
  }

  StackSpec overlaySpec;
  overlaySpec.size = spec.size;
  if (!overlaySpec.size.preferredWidth.has_value() && bounds.width > 0.0f) {
    overlaySpec.size.preferredWidth = bounds.width;
  }
  if (!overlaySpec.size.preferredHeight.has_value() && bounds.height > 0.0f) {
    overlaySpec.size.preferredHeight = bounds.height;
  }
  if (spec.paddingX > 0.0f) {
    overlaySpec.padding.left = spec.paddingX;
    overlaySpec.padding.right = spec.paddingX;
  }
  overlaySpec.clipChildren = true;
  overlaySpec.visible = spec.visible;
  UiNode overlay = createOverlay(overlaySpec);
  overlay.setHitTestVisible(enabled);

  if (!spec.visible) {
    return UiNode(frame(), overlay.nodeId(), allowAbsolute_);
  }

  uint32_t textSize = static_cast<uint32_t>(text.size());
  uint32_t selectionStart =
      Internal::clampTextIndex(spec.selectionStart, textSize, "SelectableTextSpec", "selectionStart");
  uint32_t selectionEnd =
      Internal::clampTextIndex(spec.selectionEnd, textSize, "SelectableTextSpec", "selectionEnd");
  if (state && enabled) {
    state->text = text;
    state->selectionAnchor = Internal::clampTextIndex(state->selectionAnchor,
                                                      textSize,
                                                      "SelectableTextState",
                                                      "selectionAnchor");
    state->selectionStart = Internal::clampTextIndex(state->selectionStart,
                                                     textSize,
                                                     "SelectableTextState",
                                                     "selectionStart");
    state->selectionEnd = Internal::clampTextIndex(state->selectionEnd,
                                                   textSize,
                                                   "SelectableTextState",
                                                   "selectionEnd");
    selectionStart = state->selectionStart;
    selectionEnd = state->selectionEnd;
  }

  TextSelectionOverlaySpec selectionSpec;
  selectionSpec.text = text;
  selectionSpec.textStyle = spec.textStyle;
  selectionSpec.wrap = spec.wrap;
  selectionSpec.maxWidth = maxWidth;
  selectionSpec.layout = &layout;
  selectionSpec.selectionStart = selectionStart;
  selectionSpec.selectionEnd = selectionEnd;
  selectionSpec.paddingX = 0.0f;
  selectionSpec.selectionStyle = spec.selectionStyle;
  selectionSpec.selectionStyleOverride = spec.selectionStyleOverride;
  float textAreaWidth = maxWidth > 0.0f ? maxWidth : std::max(0.0f, bounds.width - spec.paddingX * 2.0f);
  selectionSpec.size.preferredWidth = textAreaWidth;
  selectionSpec.size.preferredHeight = bounds.height;
  selectionSpec.visible = spec.visible;
  overlay.createTextSelectionOverlay(selectionSpec);

  ParagraphSpec paragraphSpec;
  paragraphSpec.text = text;
  paragraphSpec.textStyle = spec.textStyle;
  paragraphSpec.textStyleOverride = spec.textStyleOverride;
  paragraphSpec.wrap = spec.wrap;
  paragraphSpec.maxWidth = maxWidth;
  paragraphSpec.size.preferredWidth = textAreaWidth;
  paragraphSpec.size.preferredHeight = bounds.height;
  paragraphSpec.visible = spec.visible;
  overlay.createParagraph(paragraphSpec);

  if (state) {
    auto layoutPtr = std::make_shared<TextSelectionLayout>(layout);
    PrimeFrame::Callback callback;
    callback.onEvent = [state,
                        callbacks = spec.callbacks,
                        clipboard = spec.clipboard,
                        layoutPtr,
                        framePtr = &frame(),
                        textStyle = spec.textStyle,
                        paddingX = spec.paddingX,
                        handleClipboardShortcuts = spec.handleClipboardShortcuts,
                        stateOwner](
                           PrimeFrame::Event const& event) -> bool {
      (void)stateOwner;
      if (!state) {
        return false;
      }
      auto update_cursor_hint = [&](bool hovered) {
        CursorHint next = hovered ? CursorHint::IBeam : CursorHint::Arrow;
        if (state->cursorHint != next) {
          state->cursorHint = next;
          if (callbacks.onCursorHintChanged) {
            callbacks.onCursorHintChanged(next);
          }
        }
      };
      auto notify_state = [&]() {
        if (callbacks.onStateChanged) {
          callbacks.onStateChanged();
        }
      };
      auto notify_selection = [&]() {
        uint32_t start = std::min(state->selectionStart, state->selectionEnd);
        uint32_t end = std::max(state->selectionStart, state->selectionEnd);
        if (callbacks.onSelectionChanged) {
          callbacks.onSelectionChanged(start, end);
        }
      };
      auto clamp_indices = [&]() {
        uint32_t size = static_cast<uint32_t>(state->text.size());
        state->selectionAnchor = std::min(state->selectionAnchor, size);
        state->selectionStart = std::min(state->selectionStart, size);
        state->selectionEnd = std::min(state->selectionEnd, size);
      };

      switch (event.type) {
        case PrimeFrame::EventType::PointerEnter: {
          if (!state->hovered) {
            state->hovered = true;
            if (callbacks.onHoverChanged) {
              callbacks.onHoverChanged(true);
            }
            update_cursor_hint(true);
            notify_state();
          }
          return true;
        }
        case PrimeFrame::EventType::PointerLeave: {
          if (state->hovered) {
            state->hovered = false;
            if (callbacks.onHoverChanged) {
              callbacks.onHoverChanged(false);
            }
            update_cursor_hint(false);
            notify_state();
          }
          return true;
        }
        case PrimeFrame::EventType::PointerDown: {
          clamp_indices();
          uint32_t cursorIndex = caretIndexForClickInLayout(*framePtr,
                                                            textStyle,
                                                            state->text,
                                                            *layoutPtr,
                                                            paddingX,
                                                            event.localX,
                                                            event.localY);
          state->selectionAnchor = cursorIndex;
          state->selectionStart = cursorIndex;
          state->selectionEnd = cursorIndex;
          state->selecting = true;
          state->pointerId = event.pointerId;
          notify_selection();
          notify_state();
          return true;
        }
        case PrimeFrame::EventType::PointerDrag:
        case PrimeFrame::EventType::PointerMove: {
          if (!state->selecting || state->pointerId != event.pointerId) {
            return false;
          }
          clamp_indices();
          uint32_t cursorIndex = caretIndexForClickInLayout(*framePtr,
                                                            textStyle,
                                                            state->text,
                                                            *layoutPtr,
                                                            paddingX,
                                                            event.localX,
                                                            event.localY);
          if (state->selectionEnd != cursorIndex) {
            state->selectionStart = state->selectionAnchor;
            state->selectionEnd = cursorIndex;
            notify_selection();
            notify_state();
          }
          return true;
        }
        case PrimeFrame::EventType::PointerUp:
        case PrimeFrame::EventType::PointerCancel: {
          if (state->pointerId == event.pointerId) {
            if (state->selecting) {
              state->selecting = false;
              state->pointerId = -1;
              notify_state();
            }
            if (state->hovered && event.targetW > 0.0f && event.targetH > 0.0f) {
              bool inside = event.localX >= 0.0f && event.localX < event.targetW &&
                            event.localY >= 0.0f && event.localY < event.targetH;
              if (!inside) {
                state->hovered = false;
                if (callbacks.onHoverChanged) {
                  callbacks.onHoverChanged(false);
                }
                update_cursor_hint(false);
                notify_state();
              }
            }
            return true;
          }
          return false;
        }
        case PrimeFrame::EventType::KeyDown: {
          if (!state->focused) {
            return false;
          }
          constexpr int KeyA = keyCodeInt(KeyCode::A);
          constexpr int KeyC = keyCodeInt(KeyCode::C);
          constexpr int KeyLeft = keyCodeInt(KeyCode::Left);
          constexpr int KeyRight = keyCodeInt(KeyCode::Right);
          constexpr int KeyHome = keyCodeInt(KeyCode::Home);
          constexpr int KeyEnd = keyCodeInt(KeyCode::End);
          constexpr int KeyUp = keyCodeInt(KeyCode::Up);
          constexpr int KeyDown = keyCodeInt(KeyCode::Down);
          constexpr int KeyPageUp = keyCodeInt(KeyCode::PageUp);
          constexpr int KeyPageDown = keyCodeInt(KeyCode::PageDown);
          constexpr uint32_t ShiftMask = 1u << 0u;
          constexpr uint32_t ControlMask = 1u << 1u;
          constexpr uint32_t AltMask = 1u << 2u;
          constexpr uint32_t SuperMask = 1u << 3u;
          bool shiftPressed = (event.modifiers & ShiftMask) != 0u;
          bool altPressed = (event.modifiers & AltMask) != 0u;
          bool isShortcut =
              handleClipboardShortcuts &&
              ((event.modifiers & ControlMask) != 0u || (event.modifiers & SuperMask) != 0u);
          if (!isShortcut) {
            clamp_indices();
            uint32_t selectionStart = std::min(state->selectionStart, state->selectionEnd);
            uint32_t selectionEnd = std::max(state->selectionStart, state->selectionEnd);
            bool hasSelection = selectionStart != selectionEnd;
            uint32_t cursor = hasSelection ? state->selectionEnd : state->selectionStart;
            uint32_t size = static_cast<uint32_t>(state->text.size());
            bool changed = false;
            auto move_cursor = [&](uint32_t nextCursor, uint32_t anchorCursor) {
              if (shiftPressed) {
                if (!hasSelection) {
                  state->selectionAnchor = anchorCursor;
                }
                state->selectionStart = state->selectionAnchor;
                state->selectionEnd = nextCursor;
              } else {
                clearSelectableTextSelection(*state, nextCursor);
              }
            };
            auto line_height = [&]() -> float {
              float height = layoutPtr->lineHeight;
              if (height <= 0.0f) {
                height = Internal::resolveLineHeight(*framePtr, textStyle);
              }
              return height;
            };
            auto find_line_index = [&](uint32_t index) -> size_t {
              if (layoutPtr->lines.empty()) {
                return 0u;
              }
              for (size_t i = 0; i < layoutPtr->lines.size(); ++i) {
                const auto& line = layoutPtr->lines[i];
                if (index >= line.start && index <= line.end) {
                  return i;
                }
              }
              return layoutPtr->lines.size() - 1u;
            };
            auto cursor_x_for_line = [&](size_t lineIndex, uint32_t index) -> float {
              if (layoutPtr->lines.empty()) {
                return 0.0f;
              }
              const auto& line = layoutPtr->lines[lineIndex];
              if (line.end < line.start) {
                return 0.0f;
              }
              uint32_t localIndex = 0u;
              if (index >= line.start) {
                uint32_t clampedIndex = std::min(index, line.end);
                localIndex = clampedIndex - line.start;
              }
              std::string_view lineText(state->text.data() + line.start, line.end - line.start);
              size_t prefixSize = std::min<size_t>(localIndex, lineText.size());
              return Internal::estimateTextWidth(*framePtr, textStyle, lineText.substr(0u, prefixSize));
            };
            auto move_vertical = [&](int deltaLines) -> bool {
              if (layoutPtr->lines.empty()) {
                return false;
              }
              size_t lineIndex = find_line_index(cursor);
              int target = static_cast<int>(lineIndex) + deltaLines;
              if (target < 0) {
                target = 0;
              }
              int maxIndex = static_cast<int>(layoutPtr->lines.size()) - 1;
              if (target > maxIndex) {
                target = maxIndex;
              }
              float height = line_height();
              if (height <= 0.0f) {
                return false;
              }
              float cursorX = cursor_x_for_line(lineIndex, cursor);
              float localX = paddingX + cursorX;
              float localY = (static_cast<float>(target) + 0.5f) * height;
              uint32_t nextCursor = caretIndexForClickInLayout(*framePtr,
                                                               textStyle,
                                                               state->text,
                                                               *layoutPtr,
                                                               paddingX,
                                                               localX,
                                                               localY);
              move_cursor(nextCursor, cursor);
              return true;
            };
            if (event.key == KeyLeft) {
              if (altPressed) {
                if (!shiftPressed && hasSelection) {
                  move_cursor(selectionStart, cursor);
                } else {
                  uint32_t anchorCursor = cursor;
                  cursor = prev_word_boundary(state->text, cursor);
                  move_cursor(cursor, anchorCursor);
                }
              } else if (shiftPressed) {
                if (!hasSelection) {
                  state->selectionAnchor = cursor;
                }
                cursor = utf8Prev(state->text, cursor);
                state->selectionStart = state->selectionAnchor;
                state->selectionEnd = cursor;
              } else {
                cursor = hasSelection ? selectionStart : utf8Prev(state->text, cursor);
                move_cursor(cursor, cursor);
              }
              changed = true;
            } else if (event.key == KeyRight) {
              if (altPressed) {
                if (!shiftPressed && hasSelection) {
                  move_cursor(selectionEnd, cursor);
                } else {
                  uint32_t anchorCursor = cursor;
                  cursor = next_word_boundary(state->text, cursor);
                  move_cursor(cursor, anchorCursor);
                }
              } else if (shiftPressed) {
                if (!hasSelection) {
                  state->selectionAnchor = cursor;
                }
                cursor = utf8Next(state->text, cursor);
                state->selectionStart = state->selectionAnchor;
                state->selectionEnd = cursor;
              } else {
                cursor = hasSelection ? selectionEnd : utf8Next(state->text, cursor);
                move_cursor(cursor, cursor);
              }
              changed = true;
            } else if (event.key == KeyHome) {
              if (shiftPressed) {
                if (!hasSelection) {
                  state->selectionAnchor = cursor;
                }
                cursor = 0u;
                state->selectionStart = state->selectionAnchor;
                state->selectionEnd = cursor;
              } else {
                cursor = 0u;
                move_cursor(cursor, cursor);
              }
              changed = true;
            } else if (event.key == KeyEnd) {
              if (shiftPressed) {
                if (!hasSelection) {
                  state->selectionAnchor = cursor;
                }
                cursor = size;
                state->selectionStart = state->selectionAnchor;
                state->selectionEnd = cursor;
              } else {
                cursor = size;
                move_cursor(cursor, cursor);
              }
              changed = true;
            } else if (event.key == KeyUp) {
              changed = move_vertical(-1);
            } else if (event.key == KeyDown) {
              changed = move_vertical(1);
            } else if (event.key == KeyPageUp || event.key == KeyPageDown) {
              float height = line_height();
              int pageStep = 1;
              if (height > 0.0f && event.targetH > 0.0f) {
                pageStep = std::max(1, static_cast<int>(event.targetH / height) - 1);
              }
              int delta = (event.key == KeyPageDown) ? pageStep : -pageStep;
              changed = move_vertical(delta);
            }
            if (changed) {
              notify_selection();
              notify_state();
              return true;
            }
            return false;
          }
          clamp_indices();
          if (event.key == KeyA) {
            uint32_t size = static_cast<uint32_t>(state->text.size());
            state->selectionAnchor = 0u;
            state->selectionStart = 0u;
            state->selectionEnd = size;
            notify_selection();
            notify_state();
            return true;
          }
          if (event.key == KeyC) {
            uint32_t start = 0u;
            uint32_t end = 0u;
            if (selectableTextHasSelection(*state, start, end) && clipboard.setText) {
              clipboard.setText(std::string_view(state->text.data() + start, end - start));
            }
            return true;
          }
          return false;
        }
        default:
          break;
      }
      return false;
    };

    callback.onFocus = [state, callbacks = spec.callbacks, stateOwner]() {
      (void)stateOwner;
      if (!state) {
        return;
      }
      bool changed = !state->focused;
      if (!changed) {
        return;
      }
      state->focused = true;
      if (callbacks.onFocusChanged) {
        callbacks.onFocusChanged(true);
      }
      if (callbacks.onStateChanged) {
        callbacks.onStateChanged();
      }
    };

    callback.onBlur = [state, callbacks = spec.callbacks, stateOwner]() {
      (void)stateOwner;
      if (!state) {
        return;
      }
      bool changed = state->focused;
      if (!changed) {
        return;
      }
      state->focused = false;
      state->selecting = false;
      state->pointerId = -1;
      uint32_t start = std::min(state->selectionStart, state->selectionEnd);
      uint32_t end = std::max(state->selectionStart, state->selectionEnd);
      if (start != end) {
        clearSelectableTextSelection(*state, start);
        if (callbacks.onSelectionChanged) {
          callbacks.onSelectionChanged(start, start);
        }
      }
      if (callbacks.onFocusChanged) {
        callbacks.onFocusChanged(false);
      }
      if (callbacks.onStateChanged) {
        callbacks.onStateChanged();
      }
    };

    if (PrimeFrame::Node* node = frame().getNode(overlay.nodeId())) {
      node->callbacks = frame().addCallback(std::move(callback));
    }
  }

  if (spec.visible && enabled) {
    Internal::InternalFocusStyle focusStyle = Internal::resolveFocusStyle(frame(),
                                                                          spec.focusStyle,
                                                                          spec.focusStyleOverride,
                                                                          0,
                                                                          0,
                                                                          0,
                                                                          0,
                                                                          0);
    Internal::attachFocusOverlay(frame(),
                                 overlay.nodeId(),
                                 Internal::InternalRect{0.0f, 0.0f, bounds.width, bounds.height},
                                 focusStyle,
                                 spec.visible);
    if (PrimeFrame::Node* node = frame().getNode(overlay.nodeId())) {
      node->focusable = false;
    }
  }

  if (!enabled) {
    Internal::addDisabledScrimOverlay(frame(),
                                      overlay.nodeId(),
                                      Internal::InternalRect{0.0f, 0.0f, bounds.width, bounds.height},
                                      spec.visible);
  }

  return UiNode(frame(), overlay.nodeId(), allowAbsolute_);
}


} // namespace PrimeStage
