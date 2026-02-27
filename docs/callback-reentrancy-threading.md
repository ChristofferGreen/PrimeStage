# PrimeStage Callback Reentrancy And Threading Guarantees

## Execution Context

PrimeStage widget callbacks execute synchronously on the same thread that dispatches `PrimeFrame::Event` input.

Implications:
- PrimeStage does not schedule callbacks on worker threads.
- PrimeStage does not add internal callback locks for app-owned state.
- App code is responsible for cross-thread synchronization before sharing mutable state with callback code.

## Rebuild/Layout Requests From Callbacks

Callbacks may request rebuild/layout/frame work through app runtime state (for example `FrameLifecycle::requestRebuild()` and related flags).

Recommended pattern:
- callback updates app state and marks runtime work as pending
- frame loop applies rebuild/layout after callback returns

Avoid performing full rebuild/layout recursively inside the callback itself.

## Reentrancy Guardrails

For callback composition helpers:
- `appendNodeOnEvent(...)`
- `appendNodeOnFocus(...)`
- `appendNodeOnBlur(...)`

PrimeStage suppresses direct reentrant invocation of the same composed callback chain.

This prevents recursive callback loops such as:
- `onEvent` handler re-triggering its own callback chain immediately
- `onFocus`/`onBlur` handlers recursively invoking focus callbacks

Suppressed invocations return `false` for `onEvent` or no-op for focus/blur handlers.
Debug builds emit a diagnostic message to stderr.

## App Guidance

1. Keep callback bodies short and deterministic.
2. Restrict callback side effects to state updates and lifecycle requests.
3. Treat callbacks as single-thread-affine to the event loop thread.
4. If app code must mutate shared state from background threads, synchronize before event dispatch.
