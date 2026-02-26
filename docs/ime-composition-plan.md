# IME/Composition Plan

## Goal
Define a stable text-composition API for `TextField` so host backends can deliver IME preedit workflows consistently.

## Scope
This plan defines API shape and behavior contracts for composition start/update/commit/cancel.
It does not yet require PrimeFrame event-type expansion or full runtime wiring.

## Public API Shape
`include/PrimeStage/Ui.h` defines:
- `TextCompositionState`
- `TextCompositionCallbacks`
- `TextFieldSpec::compositionState`
- `TextFieldSpec::compositionCallbacks`

### Composition Lifecycle
- Start:
  - host indicates composition session begins.
  - `compositionState->active` becomes `true`.
  - `onCompositionStart()` fires once per session.
- Update:
  - host publishes preedit text and replacement range in UTF-8 byte indices.
  - `compositionState->text`, `replacementStart`, `replacementEnd` are updated.
  - `onCompositionUpdate(preedit, start, end)` fires.
- Commit:
  - host commits final text.
  - committed text is inserted/replaced in `TextFieldState::text`.
  - `compositionState->active` becomes `false`; transient composition text/range clear.
  - `onCompositionCommit(committed)` fires.
- Cancel:
  - host cancels composition session.
  - `compositionState->active` becomes `false`; transient composition text/range clear.
  - no text mutation is applied for cancel.
  - `onCompositionCancel()` fires.

## Invariants
- Composition indices are UTF-8 byte offsets and must align to codepoint boundaries.
- `replacementStart <= replacementEnd <= state->text.size()` after clamping/validation.
- While composition is active, selection/caret remain valid byte boundaries.
- Non-composition text input (`EventType::TextInput`) keeps current behavior.

## Planned Runtime Integration
1. Add host-level composition events in PrimeHost and bridge translation in PrimeStage input bridge.
2. Extend PrimeFrame event surface (or equivalent dispatch channel) for composition lifecycle payloads.
3. Wire `createTextField` handlers to update `TextCompositionState` and invoke callbacks.
4. Render preedit text/underline feedback for active composition range.
5. Add focused integration tests for Japanese/Korean/Chinese IME sessions and cancellation edge cases.

## Test Strategy (Current)
Before full composition-event plumbing, keep regression tests for:
- non-ASCII insertion/deletion correctness
- composition-like high-frequency replacement flows using repeated `TextInput` with selection replacement

These tests protect UTF-8 correctness and cursor/selection invariants during eventual IME rollout.
