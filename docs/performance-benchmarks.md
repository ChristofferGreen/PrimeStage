# PrimeStage Performance Benchmarks And Budgets

PrimeStage ships a benchmark harness for regression tracking of frame lifecycle cost and interaction-heavy flows.

## Scope

The benchmark executable is `PrimeStage_benchmarks` and covers:
- representative scene rebuild/layout/render cost for a mixed dashboard widget tree
- representative scene rebuild/layout/render cost for a tree-heavy navigation scene
- interaction-heavy flows: text typing, slider drag, and wheel scrolling

## Run Locally

Build with the preferred workflow and run benchmark mode:

```sh
./scripts/compile.sh --release --perf
```

Run with budget enforcement:

```sh
./scripts/compile.sh --release --perf-budget
```

The benchmark writes JSON output to `build-<type>/perf-results.json`.

## Budget File

Budgets are defined in `tests/perf/perf_budgets.txt` as p95 microsecond thresholds.

Format:

```text
<metric_name> <max_p95_microseconds>
```

If `--perf-budget` exceeds any threshold, the command fails and CI blocks the change.

## CI Gate

Presubmit runs the benchmark budget gate on the `release` matrix lane via:

```sh
./build-release/PrimeStage_benchmarks --budget-file tests/perf/perf_budgets.txt --check-budgets --output build-release/perf-results.json
```

This keeps the gate deterministic and tied to versioned budget expectations.
