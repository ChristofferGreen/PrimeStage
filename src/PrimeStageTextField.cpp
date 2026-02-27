#include "PrimeStage/PrimeStage.h"

#include "PrimeStage/TextSelection.h"
#include "PrimeStageCollectionInternals.h"
#include "PrimeFrame/Events.h"

#include <algorithm>
#include <chrono>
#include <memory>

namespace PrimeStage {

UiNode UiNode::createTextField(TextFieldSpec const& specInput) {
  TextFieldSpec spec = Internal::normalizeTextFieldSpec(specInput);
  bool enabled = spec.enabled;
  bool readOnly = spec.readOnly;
  PrimeFrame::Frame& runtimeFrame = frame();
  Internal::WidgetRuntimeContext runtime = Internal::makeWidgetRuntimeContext(runtimeFrame,
                                                                              nodeId(),
                                                                              allowAbsolute(),
                                                                              enabled,
                                                                              spec.visible,
                                                                              spec.tabIndex);

  Internal::InternalRect bounds = Internal::resolveRect(spec.size);
  std::shared_ptr<TextFieldState> stateOwner = spec.ownedState;
  TextFieldState* state = spec.state;
  if (!state) {
    if (!stateOwner) {
      stateOwner = std::make_shared<TextFieldState>();
    }
    if (Internal::textFieldStateIsPristine(*stateOwner)) {
      Internal::seedTextFieldStateFromSpec(*stateOwner, spec);
    }
    state = stateOwner.get();
  }
  std::string_view previewText = std::string_view(state->text);
  PrimeFrame::TextStyleToken previewStyle = spec.textStyle;
  if (previewText.empty() && spec.showPlaceholderWhenEmpty) {
    previewText = spec.placeholder;
    previewStyle = spec.placeholderStyle;
  }
  float lineHeight = Internal::resolveLineHeight(runtimeFrame, spec.textStyle);
  if (lineHeight <= 0.0f && previewStyle != spec.textStyle) {
    lineHeight = Internal::resolveLineHeight(runtimeFrame, previewStyle);
  }
  if (bounds.height <= 0.0f &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchY <= 0.0f) {
    if (lineHeight > 0.0f) {
      bounds.height = lineHeight;
    }
  }
  if (bounds.width <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      spec.size.stretchX <= 0.0f &&
      !previewText.empty()) {
    float previewWidth = Internal::estimateTextWidth(runtimeFrame, previewStyle, previewText);
    bounds.width = std::max(bounds.width, previewWidth + spec.paddingX * 2.0f);
  }
  if (bounds.width <= 0.0f &&
      bounds.height <= 0.0f &&
      !spec.size.preferredWidth.has_value() &&
      !spec.size.preferredHeight.has_value() &&
      spec.size.stretchX <= 0.0f &&
      spec.size.stretchY <= 0.0f) {
    return UiNode(runtimeFrame, runtime.parentId, runtime.allowAbsolute);
  }
  PanelSpec panel;
  panel.size = spec.size;
  if (!panel.size.preferredWidth.has_value() && bounds.width > 0.0f) {
    panel.size.preferredWidth = bounds.width;
  }
  if (!panel.size.preferredHeight.has_value() && bounds.height > 0.0f) {
    panel.size.preferredHeight = bounds.height;
  }
  panel.rectStyle = spec.backgroundStyle;
  panel.rectStyleOverride = spec.backgroundStyleOverride;
  panel.visible = spec.visible;
  UiNode field = createPanel(panel);

  if (!spec.visible) {
    return UiNode(runtimeFrame, field.nodeId(), runtime.allowAbsolute);
  }

  std::string_view activeText = std::string_view(state->text);
  uint32_t textSize = static_cast<uint32_t>(activeText.size());
  uint32_t cursorIndex = state ? state->cursor : spec.cursorIndex;
  uint32_t selectionAnchor = state ? state->selectionAnchor : cursorIndex;
  uint32_t selectionStart = state ? state->selectionStart : spec.selectionStart;
  uint32_t selectionEnd = state ? state->selectionEnd : spec.selectionEnd;
  cursorIndex = Internal::clampTextIndex(cursorIndex, textSize, "TextFieldSpec", "cursor");
  selectionAnchor =
      Internal::clampTextIndex(selectionAnchor, textSize, "TextFieldSpec", "selectionAnchor");
  selectionStart =
      Internal::clampTextIndex(selectionStart, textSize, "TextFieldSpec", "selectionStart");
  selectionEnd = Internal::clampTextIndex(selectionEnd, textSize, "TextFieldSpec", "selectionEnd");
  if (state && enabled) {
    state->cursor = cursorIndex;
    state->selectionAnchor = selectionAnchor;
    state->selectionStart = selectionStart;
    state->selectionEnd = selectionEnd;
  }

  std::string_view content = std::string_view(state->text);
  PrimeFrame::TextStyleToken style = spec.textStyle;
  PrimeFrame::TextStyleOverride overrideStyle = spec.textStyleOverride;
  if (content.empty() && spec.showPlaceholderWhenEmpty) {
    content = spec.placeholder;
    style = spec.placeholderStyle;
    overrideStyle = spec.placeholderStyleOverride;
  }

  lineHeight = Internal::resolveLineHeight(runtimeFrame, style);
  if (lineHeight <= 0.0f && style != spec.textStyle) {
    lineHeight = Internal::resolveLineHeight(runtimeFrame, spec.textStyle);
  }
  float textY = (bounds.height - lineHeight) * 0.5f + spec.textOffsetY;
  float textWidth = std::max(0.0f, bounds.width - spec.paddingX * 2.0f);
  bool showCursor = state ? (state->focused && state->cursorVisible) : spec.showCursor;

  std::vector<float> initialCaretPositions;
  if (!activeText.empty() && (showCursor || selectionStart != selectionEnd)) {
    initialCaretPositions = Internal::buildCaretPositionsForText(runtimeFrame, spec.textStyle, activeText);
  }
  auto initialCaretAdvanceFor = [&](uint32_t index) -> float {
    if (initialCaretPositions.empty()) {
      return 0.0f;
    }
    uint32_t clamped = std::min(index, textSize);
    return initialCaretPositions[clamped];
  };

  Internal::InternalRect initialSelectionRect{spec.paddingX, textY, 0.0f, std::max(0.0f, lineHeight)};
  bool initialSelectionVisible = false;
  uint32_t selStart = std::min(selectionStart, selectionEnd);
  uint32_t selEnd = std::max(selectionStart, selectionEnd);
  if (selStart < selEnd && !activeText.empty() && spec.selectionStyle != 0) {
    float startAdvance = initialCaretAdvanceFor(selStart);
    float endAdvance = initialCaretAdvanceFor(selEnd);
    float startX = spec.paddingX + startAdvance;
    float endX = spec.paddingX + endAdvance;
    float maxX = bounds.width - spec.paddingX;
    if (maxX < spec.paddingX) {
      maxX = spec.paddingX;
    }
    startX = std::clamp(startX, spec.paddingX, maxX);
    endX = std::clamp(endX, spec.paddingX, maxX);
    if (endX > startX) {
      initialSelectionRect.x = startX;
      initialSelectionRect.width = endX - startX;
      initialSelectionVisible = true;
    }
  }

  Internal::InternalRect initialCursorRect{spec.paddingX, textY, 0.0f, std::max(0.0f, lineHeight)};
  bool initialCursorVisible = false;
  if (showCursor && spec.cursorStyle != 0) {
    float cursorAdvance = initialCaretAdvanceFor(cursorIndex);
    float cursorX = spec.paddingX + cursorAdvance;
    float maxX = bounds.width - spec.paddingX - spec.cursorWidth;
    if (maxX < spec.paddingX) {
      maxX = spec.paddingX;
    }
    if (cursorX > maxX) {
      cursorX = maxX;
    }
    initialCursorRect.x = cursorX;
    initialCursorRect.width = spec.cursorWidth;
    initialCursorVisible = initialCursorRect.width > 0.0f && initialCursorRect.height > 0.0f;
  }

  PrimeFrame::NodeId selectionNodeId{};
  PrimeFrame::PrimitiveId selectionPrim{};
  if (spec.selectionStyle != 0) {
    selectionNodeId = Internal::createRectNode(runtimeFrame,
                                       field.nodeId(),
                                       initialSelectionRect,
                                       spec.selectionStyle,
                                       spec.selectionStyleOverride,
                                       false,
                                       spec.visible);
    if (PrimeFrame::Node* selectionNode = runtimeFrame.getNode(selectionNodeId);
        selectionNode && !selectionNode->primitives.empty()) {
      selectionPrim = selectionNode->primitives.front();
      selectionNode->visible = initialSelectionVisible;
    }
  }

  Internal::InternalRect textRect{spec.paddingX, textY, textWidth, std::max(0.0f, lineHeight)};
  PrimeFrame::NodeId textNodeId = Internal::createTextNode(runtimeFrame,
                                                   field.nodeId(),
                                                   textRect,
                                                   content,
                                                   style,
                                                   overrideStyle,
                                                   PrimeFrame::TextAlign::Start,
                                                   PrimeFrame::WrapMode::None,
                                                   textWidth,
                                                   spec.visible);
  PrimeFrame::PrimitiveId textPrim{};
  if (PrimeFrame::Node* textNode = runtimeFrame.getNode(textNodeId);
      textNode && !textNode->primitives.empty()) {
    textPrim = textNode->primitives.front();
  }

  PrimeFrame::NodeId cursorNodeId{};
  PrimeFrame::PrimitiveId cursorPrim{};
  if (spec.cursorStyle != 0) {
    cursorNodeId = Internal::createRectNode(runtimeFrame,
                                    field.nodeId(),
                                    initialCursorRect,
                                    spec.cursorStyle,
                                    spec.cursorStyleOverride,
                                    false,
                                    spec.visible);
    if (PrimeFrame::Node* cursorNode = runtimeFrame.getNode(cursorNodeId);
        cursorNode && !cursorNode->primitives.empty()) {
      cursorPrim = cursorNode->primitives.front();
      cursorNode->visible = initialCursorVisible;
    }
  }

  struct TextFieldPatchState {
    PrimeFrame::Frame* frame = nullptr;
    TextFieldState* state = nullptr;
    PrimeFrame::NodeId textNode{};
    PrimeFrame::PrimitiveId textPrim{};
    PrimeFrame::NodeId selectionNode{};
    PrimeFrame::PrimitiveId selectionPrim{};
    PrimeFrame::NodeId cursorNode{};
    PrimeFrame::PrimitiveId cursorPrim{};
    std::string placeholderText;
    float width = 0.0f;
    float height = 0.0f;
    float paddingX = 0.0f;
    float textOffsetY = 0.0f;
    float cursorWidth = 0.0f;
    bool showPlaceholderWhenEmpty = false;
    PrimeFrame::TextStyleToken textStyle = 0;
    PrimeFrame::TextStyleOverride textStyleOverride{};
    PrimeFrame::TextStyleToken placeholderStyle = 0;
    PrimeFrame::TextStyleOverride placeholderStyleOverride{};
  };

  auto patchState = std::make_shared<TextFieldPatchState>();
  patchState->frame = &runtimeFrame;
  patchState->state = state;
  patchState->textNode = textNodeId;
  patchState->textPrim = textPrim;
  patchState->selectionNode = selectionNodeId;
  patchState->selectionPrim = selectionPrim;
  patchState->cursorNode = cursorNodeId;
  patchState->cursorPrim = cursorPrim;
  patchState->placeholderText = std::string(spec.placeholder);
  patchState->width = bounds.width;
  patchState->height = bounds.height;
  patchState->paddingX = spec.paddingX;
  patchState->textOffsetY = spec.textOffsetY;
  patchState->cursorWidth = spec.cursorWidth;
  patchState->showPlaceholderWhenEmpty = spec.showPlaceholderWhenEmpty;
  patchState->textStyle = spec.textStyle;
  patchState->textStyleOverride = spec.textStyleOverride;
  patchState->placeholderStyle = spec.placeholderStyle;
  patchState->placeholderStyleOverride = spec.placeholderStyleOverride;

  auto patchTextFieldVisuals = [patchState, stateOwner]() {
    (void)stateOwner;
    if (!patchState || !patchState->frame || !patchState->state) {
      return;
    }

    PrimeFrame::Frame& frameRef = *patchState->frame;
    TextFieldState* stateRef = patchState->state;
    uint32_t textSize = static_cast<uint32_t>(stateRef->text.size());
    stateRef->cursor = std::min(stateRef->cursor, textSize);
    stateRef->selectionAnchor = std::min(stateRef->selectionAnchor, textSize);
    stateRef->selectionStart = std::min(stateRef->selectionStart, textSize);
    stateRef->selectionEnd = std::min(stateRef->selectionEnd, textSize);

    std::string_view activeText = stateRef->text;
    std::string_view renderedText = activeText;
    PrimeFrame::TextStyleToken renderedStyle = patchState->textStyle;
    PrimeFrame::TextStyleOverride renderedOverride = patchState->textStyleOverride;
    if (renderedText.empty() && patchState->showPlaceholderWhenEmpty) {
      renderedText = patchState->placeholderText;
      renderedStyle = patchState->placeholderStyle;
      renderedOverride = patchState->placeholderStyleOverride;
    }

    float lineHeight = Internal::resolveLineHeight(frameRef, renderedStyle);
    if (lineHeight <= 0.0f && renderedStyle != patchState->textStyle) {
      lineHeight = Internal::resolveLineHeight(frameRef, patchState->textStyle);
    }
    lineHeight = std::max(0.0f, lineHeight);
    float textY = (patchState->height - lineHeight) * 0.5f + patchState->textOffsetY;
    float textWidth = std::max(0.0f, patchState->width - patchState->paddingX * 2.0f);

    if (PrimeFrame::Node* textNode = frameRef.getNode(patchState->textNode)) {
      textNode->localX = patchState->paddingX;
      textNode->localY = textY;
      textNode->visible = true;
      textNode->sizeHint.width.preferred = textWidth;
      textNode->sizeHint.height.preferred = lineHeight;
    }
    if (PrimeFrame::Primitive* textPrim = frameRef.getPrimitive(patchState->textPrim)) {
      textPrim->width = textWidth;
      textPrim->height = lineHeight;
      textPrim->textBlock.text = std::string(renderedText);
      textPrim->textBlock.maxWidth = textWidth;
      textPrim->textStyle.token = renderedStyle;
      textPrim->textStyle.overrideStyle = renderedOverride;
    }

    uint32_t selStart = 0u;
    uint32_t selEnd = 0u;
    bool hasSelection = textFieldHasSelection(*stateRef, selStart, selEnd);
    bool showCursor = stateRef->focused && stateRef->cursorVisible;

    std::vector<float> caretPositions;
    if (!activeText.empty() && (hasSelection || showCursor)) {
      caretPositions = Internal::buildCaretPositionsForText(frameRef, patchState->textStyle, activeText);
    }
    auto caretAdvanceFor = [&](uint32_t index) -> float {
      if (caretPositions.empty()) {
        return 0.0f;
      }
      uint32_t clamped = std::min(index, textSize);
      return caretPositions[clamped];
    };

    if (patchState->selectionNode.isValid()) {
      Internal::InternalRect selectionRect{patchState->paddingX, textY, 0.0f, lineHeight};
      bool showSelection = false;
      if (hasSelection && !activeText.empty()) {
        float startAdvance = caretAdvanceFor(selStart);
        float endAdvance = caretAdvanceFor(selEnd);
        float startX = patchState->paddingX + startAdvance;
        float endX = patchState->paddingX + endAdvance;
        float maxX = patchState->width - patchState->paddingX;
        if (maxX < patchState->paddingX) {
          maxX = patchState->paddingX;
        }
        startX = std::clamp(startX, patchState->paddingX, maxX);
        endX = std::clamp(endX, patchState->paddingX, maxX);
        if (endX > startX) {
          selectionRect.x = startX;
          selectionRect.width = endX - startX;
          showSelection = true;
        }
      }
      if (PrimeFrame::Node* selectionNode = frameRef.getNode(patchState->selectionNode)) {
        selectionNode->localX = selectionRect.x;
        selectionNode->localY = selectionRect.y;
        selectionNode->sizeHint.width.preferred = selectionRect.width;
        selectionNode->sizeHint.height.preferred = selectionRect.height;
        selectionNode->visible = showSelection;
      }
      if (PrimeFrame::Primitive* selectionPrim = frameRef.getPrimitive(patchState->selectionPrim)) {
        selectionPrim->width = selectionRect.width;
        selectionPrim->height = selectionRect.height;
      }
    }

    if (patchState->cursorNode.isValid()) {
      Internal::InternalRect cursorRect{patchState->paddingX, textY, 0.0f, lineHeight};
      bool showCursorVisual = false;
      if (showCursor) {
        float cursorAdvance = caretAdvanceFor(stateRef->cursor);
        float cursorX = patchState->paddingX + cursorAdvance;
        float maxX = patchState->width - patchState->paddingX - patchState->cursorWidth;
        if (maxX < patchState->paddingX) {
          maxX = patchState->paddingX;
        }
        if (cursorX > maxX) {
          cursorX = maxX;
        }
        cursorRect.x = cursorX;
        cursorRect.width = patchState->cursorWidth;
        showCursorVisual = cursorRect.width > 0.0f && cursorRect.height > 0.0f;
      }
      if (PrimeFrame::Node* cursorNode = frameRef.getNode(patchState->cursorNode)) {
        cursorNode->localX = cursorRect.x;
        cursorNode->localY = cursorRect.y;
        cursorNode->sizeHint.width.preferred = cursorRect.width;
        cursorNode->sizeHint.height.preferred = cursorRect.height;
        cursorNode->visible = showCursorVisual;
      }
      if (PrimeFrame::Primitive* cursorPrim = frameRef.getPrimitive(patchState->cursorPrim)) {
        cursorPrim->width = cursorRect.width;
        cursorPrim->height = cursorRect.height;
      }
    }
  };

  patchTextFieldVisuals();

  if (state) {
    PrimeFrame::Node* node = runtimeFrame.getNode(field.nodeId());
    if (node) {
      PrimeFrame::Callback callback;
      callback.onEvent = [state,
                          callbacks = spec.callbacks,
                          clipboard = spec.clipboard,
                          framePtr = &runtimeFrame,
                          textStyle = spec.textStyle,
                          paddingX = spec.paddingX,
                          allowNewlines = spec.allowNewlines,
                          readOnly,
                          handleClipboardShortcuts = spec.handleClipboardShortcuts,
                          cursorBlinkInterval = spec.cursorBlinkInterval,
                          patchTextFieldVisuals](PrimeFrame::Event const& event) -> bool {
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
        auto clamp_indices = [&]() {
          uint32_t size = static_cast<uint32_t>(state->text.size());
          state->cursor = std::min(state->cursor, size);
          state->selectionAnchor = std::min(state->selectionAnchor, size);
          state->selectionStart = std::min(state->selectionStart, size);
          state->selectionEnd = std::min(state->selectionEnd, size);
        };
        auto reset_blink = [&](std::chrono::steady_clock::time_point now) {
          state->cursorVisible = true;
          state->nextBlink = now + cursorBlinkInterval;
        };
        auto notify_state = [&]() {
          patchTextFieldVisuals();
          if (callbacks.onStateChanged) {
            callbacks.onStateChanged();
          }
        };
        auto notify_text = [&]() {
          if (callbacks.onChange) {
            callbacks.onChange(std::string_view(state->text));
          } else if (callbacks.onTextChanged) {
            callbacks.onTextChanged(std::string_view(state->text));
          }
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
            uint32_t cursorIndex =
                caretIndexForClick(*framePtr, textStyle, state->text, paddingX, event.localX);
            state->cursor = cursorIndex;
            state->selectionAnchor = cursorIndex;
            state->selectionStart = cursorIndex;
            state->selectionEnd = cursorIndex;
            state->selecting = true;
            state->pointerId = event.pointerId;
            reset_blink(std::chrono::steady_clock::now());
            notify_state();
            return true;
          }
          case PrimeFrame::EventType::PointerDrag:
          case PrimeFrame::EventType::PointerMove: {
            if (!state->selecting || state->pointerId != event.pointerId) {
              return false;
            }
            clamp_indices();
            uint32_t cursorIndex =
                caretIndexForClick(*framePtr, textStyle, state->text, paddingX, event.localX);
            if (cursorIndex != state->cursor || state->selectionEnd != cursorIndex) {
              state->cursor = cursorIndex;
              state->selectionStart = state->selectionAnchor;
              state->selectionEnd = cursorIndex;
              reset_blink(std::chrono::steady_clock::now());
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
              return true;
            }
            return false;
          }
          case PrimeFrame::EventType::KeyDown: {
            if (!state->focused) {
              return false;
            }
            constexpr int KeyReturn = keyCodeInt(KeyCode::Enter);
            constexpr int KeyEscape = keyCodeInt(KeyCode::Escape);
            constexpr int KeyBackspace = keyCodeInt(KeyCode::Backspace);
            constexpr int KeyLeft = keyCodeInt(KeyCode::Left);
            constexpr int KeyRight = keyCodeInt(KeyCode::Right);
            constexpr int KeyHome = keyCodeInt(KeyCode::Home);
            constexpr int KeyEnd = keyCodeInt(KeyCode::End);
            constexpr int KeyDelete = keyCodeInt(KeyCode::Delete);
            constexpr int KeyA = keyCodeInt(KeyCode::A);
            constexpr int KeyC = keyCodeInt(KeyCode::C);
            constexpr int KeyV = keyCodeInt(KeyCode::V);
            constexpr int KeyX = keyCodeInt(KeyCode::X);
            constexpr uint32_t ShiftMask = 1u << 0u;
            constexpr uint32_t ControlMask = 1u << 1u;
            constexpr uint32_t SuperMask = 1u << 3u;
            bool shiftPressed = (event.modifiers & ShiftMask) != 0u;
            bool isShortcut =
                handleClipboardShortcuts &&
                ((event.modifiers & ControlMask) != 0u || (event.modifiers & SuperMask) != 0u);

            clamp_indices();
            uint32_t selectionStart = 0u;
            uint32_t selectionEnd = 0u;
            bool hasSelection = textFieldHasSelection(*state, selectionStart, selectionEnd);
            auto delete_selection = [&]() -> bool {
              if (!hasSelection) {
                return false;
              }
              state->text.erase(selectionStart, selectionEnd - selectionStart);
              state->cursor = selectionStart;
              clearTextFieldSelection(*state, state->cursor);
              return true;
            };

            if (isShortcut) {
              if (event.key == KeyA) {
                uint32_t size = static_cast<uint32_t>(state->text.size());
                state->selectionAnchor = 0u;
                state->selectionStart = 0u;
                state->selectionEnd = size;
                state->cursor = size;
                reset_blink(std::chrono::steady_clock::now());
                notify_state();
                return true;
              }
              if (event.key == KeyC) {
                if (hasSelection && clipboard.setText) {
                  clipboard.setText(
                      std::string_view(state->text.data() + selectionStart,
                                       selectionEnd - selectionStart));
                }
                return true;
              }
              if (event.key == KeyX) {
                if (readOnly) {
                  return true;
                }
                if (hasSelection) {
                  if (clipboard.setText) {
                    clipboard.setText(
                        std::string_view(state->text.data() + selectionStart,
                                         selectionEnd - selectionStart));
                  }
                  delete_selection();
                  notify_text();
                  reset_blink(std::chrono::steady_clock::now());
                  notify_state();
                }
                return true;
              }
              if (event.key == KeyV) {
                if (readOnly) {
                  return true;
                }
                if (clipboard.getText) {
                  std::string paste = clipboard.getText();
                  if (!allowNewlines) {
                    paste.erase(std::remove(paste.begin(), paste.end(), '\n'), paste.end());
                    paste.erase(std::remove(paste.begin(), paste.end(), '\r'), paste.end());
                  }
                  if (!paste.empty()) {
                    delete_selection();
                    uint32_t cursor = state->cursor;
                    cursor = std::min(cursor, static_cast<uint32_t>(state->text.size()));
                    state->text.insert(cursor, paste);
                    state->cursor = cursor + static_cast<uint32_t>(paste.size());
                    clearTextFieldSelection(*state, state->cursor);
                    notify_text();
                    reset_blink(std::chrono::steady_clock::now());
                    notify_state();
                  }
                }
                return true;
              }
            }

            bool changed = false;
            bool keepSelection = false;
            uint32_t cursor = state->cursor;
            switch (event.key) {
              case KeyEscape:
                if (callbacks.onRequestBlur) {
                  callbacks.onRequestBlur();
                }
                return true;
              case KeyLeft:
                if (shiftPressed) {
                  if (!hasSelection) {
                    state->selectionAnchor = cursor;
                  }
                  cursor = utf8Prev(state->text, cursor);
                  state->selectionStart = state->selectionAnchor;
                  state->selectionEnd = cursor;
                  keepSelection = true;
                } else {
                  if (hasSelection) {
                    cursor = selectionStart;
                  } else {
                    cursor = utf8Prev(state->text, cursor);
                  }
                  clearTextFieldSelection(*state, cursor);
                }
                changed = true;
                break;
              case KeyRight:
                if (shiftPressed) {
                  if (!hasSelection) {
                    state->selectionAnchor = cursor;
                  }
                  cursor = utf8Next(state->text, cursor);
                  state->selectionStart = state->selectionAnchor;
                  state->selectionEnd = cursor;
                  keepSelection = true;
                } else {
                  if (hasSelection) {
                    cursor = selectionEnd;
                  } else {
                    cursor = utf8Next(state->text, cursor);
                  }
                  clearTextFieldSelection(*state, cursor);
                }
                changed = true;
                break;
              case KeyHome:
                if (shiftPressed) {
                  if (!hasSelection) {
                    state->selectionAnchor = cursor;
                  }
                  cursor = 0u;
                  state->selectionStart = state->selectionAnchor;
                  state->selectionEnd = cursor;
                  keepSelection = true;
                } else {
                  cursor = 0u;
                  clearTextFieldSelection(*state, cursor);
                }
                changed = true;
                break;
              case KeyEnd:
                if (shiftPressed) {
                  if (!hasSelection) {
                    state->selectionAnchor = cursor;
                  }
                  cursor = static_cast<uint32_t>(state->text.size());
                  state->selectionStart = state->selectionAnchor;
                  state->selectionEnd = cursor;
                  keepSelection = true;
                } else {
                  cursor = static_cast<uint32_t>(state->text.size());
                  clearTextFieldSelection(*state, cursor);
                }
                changed = true;
                break;
              case KeyBackspace:
                if (readOnly) {
                  return true;
                }
                if (delete_selection()) {
                  changed = true;
                  cursor = state->cursor;
                  notify_text();
                } else if (cursor > 0u) {
                  uint32_t start = utf8Prev(state->text, cursor);
                  state->text.erase(start, cursor - start);
                  cursor = start;
                  changed = true;
                  notify_text();
                }
                break;
              case KeyDelete:
                if (readOnly) {
                  return true;
                }
                if (delete_selection()) {
                  changed = true;
                  cursor = state->cursor;
                  notify_text();
                } else if (cursor < static_cast<uint32_t>(state->text.size())) {
                  uint32_t end = utf8Next(state->text, cursor);
                  state->text.erase(cursor, end - cursor);
                  changed = true;
                  notify_text();
                }
                break;
              case KeyReturn:
                if (!allowNewlines) {
                  if (!readOnly && callbacks.onSubmit) {
                    callbacks.onSubmit();
                  }
                  return true;
                }
                return true;
              default:
                break;
            }
            if (changed) {
              state->cursor = std::min(cursor, static_cast<uint32_t>(state->text.size()));
              if (!keepSelection) {
                clearTextFieldSelection(*state, state->cursor);
              }
              reset_blink(std::chrono::steady_clock::now());
              notify_state();
              return true;
            }
            return false;
          }
          case PrimeFrame::EventType::TextInput: {
            if (!state->focused) {
              return false;
            }
            if (readOnly) {
              return true;
            }
            if (event.text.empty()) {
              return true;
            }
            std::string filtered;
            filtered.reserve(event.text.size());
            for (char ch : event.text) {
              if (!allowNewlines && (ch == '\n' || ch == '\r')) {
                continue;
              }
              filtered.push_back(ch);
            }
            if (filtered.empty()) {
              return true;
            }
            clamp_indices();
            uint32_t selectionStart = 0u;
            uint32_t selectionEnd = 0u;
            bool hasSelection = textFieldHasSelection(*state, selectionStart, selectionEnd);
            if (hasSelection) {
              state->text.erase(selectionStart, selectionEnd - selectionStart);
              state->cursor = selectionStart;
              clearTextFieldSelection(*state, state->cursor);
            }
            uint32_t cursor = std::min(state->cursor, static_cast<uint32_t>(state->text.size()));
            state->text.insert(cursor, filtered);
            state->cursor = cursor + static_cast<uint32_t>(filtered.size());
            clearTextFieldSelection(*state, state->cursor);
            notify_text();
            reset_blink(std::chrono::steady_clock::now());
            notify_state();
            return true;
          }
          default:
            break;
        }
        return false;
      };

      callback.onFocus = [state,
                          callbacks = spec.callbacks,
                          cursorBlinkInterval = spec.cursorBlinkInterval,
                          setCursorToEndOnFocus = spec.setCursorToEndOnFocus,
                          patchTextFieldVisuals]() {
        if (!state) {
          return;
        }
        bool focusChanged = !state->focused;
        if (!focusChanged) {
          return;
        }
        state->focused = true;
        uint32_t size = static_cast<uint32_t>(state->text.size());
        state->cursor = std::min(state->cursor, size);
        if (focusChanged && setCursorToEndOnFocus) {
          state->cursor = size;
        }
        clearTextFieldSelection(*state, state->cursor);
        state->cursorVisible = true;
        state->nextBlink = std::chrono::steady_clock::now() + cursorBlinkInterval;
        patchTextFieldVisuals();
        if (focusChanged && callbacks.onFocusChanged) {
          callbacks.onFocusChanged(true);
        }
        if (callbacks.onStateChanged) {
          callbacks.onStateChanged();
        }
      };

      callback.onBlur = [state, callbacks = spec.callbacks, patchTextFieldVisuals]() {
        if (!state) {
          return;
        }
        bool focusChanged = state->focused;
        if (!focusChanged) {
          return;
        }
        state->focused = false;
        state->cursorVisible = false;
        state->nextBlink = {};
        state->selecting = false;
        state->pointerId = -1;
        uint32_t size = static_cast<uint32_t>(state->text.size());
        state->cursor = std::min(state->cursor, size);
        clearTextFieldSelection(*state, state->cursor);
        patchTextFieldVisuals();
        if (focusChanged && callbacks.onFocusChanged) {
          callbacks.onFocusChanged(false);
        }
        if (callbacks.onStateChanged) {
          callbacks.onStateChanged();
        }
      };

      node->callbacks = runtimeFrame.addCallback(std::move(callback));
    }
  }

  bool canFocus = enabled && state != nullptr;
  if (spec.visible && canFocus) {
    Internal::InternalFocusStyle focusStyle =
        Internal::resolveFocusStyle(runtimeFrame,
                                    spec.focusStyle,
                                    spec.focusStyleOverride,
                                    spec.cursorStyle,
                                    spec.selectionStyle,
                                    spec.backgroundStyle,
                                    0,
                                    0,
                                    spec.backgroundStyleOverride);
    Internal::attachFocusOverlay(runtime,
                                 field.nodeId(),
                                 Internal::InternalRect{0.0f, 0.0f, bounds.width, bounds.height},
                                 focusStyle);
  }

  if (PrimeFrame::Node* node = runtimeFrame.getNode(field.nodeId())) {
    node->focusable = canFocus;
    node->hitTestVisible = enabled;
    node->tabIndex = canFocus ? spec.tabIndex : -1;
  }

  if (!enabled) {
    Internal::addDisabledScrimOverlay(runtime,
                                      field.nodeId(),
                                      Internal::InternalRect{0.0f, 0.0f, bounds.width, bounds.height});
  } else if (readOnly) {
    Internal::addReadOnlyScrimOverlay(runtimeFrame,
                                      field.nodeId(),
                                      Internal::InternalRect{0.0f, 0.0f, bounds.width, bounds.height},
                                      spec.visible);
  }

  return UiNode(runtimeFrame, field.nodeId(), runtime.allowAbsolute);
}

UiNode UiNode::createTextField(TextFieldState& state,
                               std::string_view placeholder,
                               PrimeFrame::RectStyleToken backgroundStyle,
                               PrimeFrame::TextStyleToken textStyle,
                               SizeSpec const& size) {
  TextFieldSpec spec;
  spec.state = &state;
  spec.placeholder = placeholder;
  spec.backgroundStyle = backgroundStyle;
  spec.textStyle = textStyle;
  spec.size = size;
  return createTextField(spec);
}


} // namespace PrimeStage
