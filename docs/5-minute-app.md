# PrimeStage 5-Minute App

## Goal

Build a first PrimeStage app using only the recommended canonical API path.
This maps directly to `examples/canonical/primestage_modern_api.cpp`.

## Use This Path By Default

- Keep app code in `PrimeStage::App` + `PrimeStage::UiNode`.
- Use canonical example structure before touching advanced host/frame APIs.
- Start from `examples/canonical/primestage_modern_api.cpp`.

## Minimal Flow

1. Define app-owned state.
2. Build UI in a `buildUi(...)` function.
3. Use `PrimeStage::App` to set metrics, rebuild, and render.

```cpp
#include "PrimeStage/App.h"

struct DemoState {
  PrimeStage::State<bool> notifications{true};
  PrimeStage::State<int> tabIndex{0};
  PrimeStage::State<float> progress{0.64f};
};

void buildUi(PrimeStage::UiNode root, DemoState& state) {
  root.column([&](PrimeStage::UiNode& screen) {
    screen.label("PrimeStage Modern API");
    screen.button("Build");
    screen.createToggle(PrimeStage::bind(state.notifications));
    screen.createTabs({"Overview", "Assets", "Settings"}, PrimeStage::bind(state.tabIndex));

    PrimeStage::ProgressBarSpec progress;
    progress.binding = PrimeStage::bind(state.progress);
    screen.createProgressBar(progress);
  });
}

int main() {
  PrimeStage::App app;
  DemoState state;
  app.setSurfaceMetrics(1024u, 640u, 1.0f);
  app.setRenderMetrics(1024u, 640u, 1.0f);
  (void)app.runRebuildIfNeeded([&](PrimeStage::UiNode root) { buildUi(root, state); });
  return app.renderToPng("primestage_modern_api.png").ok() ? 0 : 1;
}
```

## Command

```sh
./scripts/compile.sh
./build-debug/primestage_modern_api
```

## Canonical Mapping

- Full canonical sample: `examples/canonical/primestage_modern_api.cpp`
- Reference API surface: `docs/minimal-api-reference.md`
- Canonical example quality rules: `docs/example-app-consumer-checklist.md`
- Ergonomics budgets: `docs/api-ergonomics-scorecard.md`

## When This Is Not Enough

If you need explicit host event-loop ownership, frame/layout internals, or low-level callback
composition, see `docs/advanced-escape-hatches.md`.
