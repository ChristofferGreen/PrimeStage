# PrimeStage Deterministic Visual-Test Harness

PrimeStage visual snapshots are generated from deterministic render-batch commands instead of platform font raster output. This keeps visual regressions stable across macOS/Linux/CI while still catching interaction and layering changes.

## Deterministic Inputs

The harness is implemented in `tests/unit/visual_test_harness.h` and pins:
- theme palette and style tokens (`interaction_palette_v1`)
- root viewport (`480x280`)
- layout scale (`1.00` in the golden baseline)
- font policy marker (`command_batch_no_raster`) to indicate snapshots are command-level, not glyph-raster-level

Golden snapshot files include a `[harness]` metadata block so drift in these assumptions is explicit.

## Golden Update Workflow

1. Run tests to verify current failures:

```sh
./scripts/compile.sh --test
```

2. If visual changes are intentional, regenerate goldens:

```sh
PRIMESTAGE_UPDATE_SNAPSHOTS=1 ./scripts/compile.sh --test
```

3. Re-run without update mode and confirm clean pass:

```sh
./scripts/compile.sh --test
```

4. Review snapshot diffs in `tests/snapshots/*.snap` before commit.

## Failure Triage Guidance

When `test_visual_regression` fails:

1. Confirm harness metadata did not change unexpectedly (`version`, `theme`, `layout_scale`, `root_size`).
2. Compare command diff by section (`[button_*]`, `[text_field_selection_focus]`, `[table_selection_focus]`).
3. Classify change source:
   - input/interaction state transition
   - focus/selection compositing order
   - layout or sizing contract change
   - theme/style token mapping change
4. If behavior is unintended, fix code and keep existing golden.
5. If behavior is intended, regenerate golden and mention why in the commit message.
