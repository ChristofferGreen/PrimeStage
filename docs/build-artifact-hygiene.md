# Build Artifact Hygiene

PrimeStage keeps generated build artifacts out of source-control workflows.

## Expected Ignored Paths

These paths are local outputs and must stay ignored:
- `build/`
- `build-*/`
- `build_*/`
- `.cache/`
- `compile_commands.json`
- `*.profraw`
- `*.profdata`

Tracked snapshot assets are intentionally versioned and should not be deleted by cleanup workflows:
- `tests/snapshots/*.snap`
- `screenshots/*.png`

## Local Cleanup Script

Use the cleanup script for repeatable local cleanup:

```sh
./scripts/clean.sh --dry-run
```

Remove build directories:

```sh
./scripts/clean.sh
```

Remove build directories plus cache/index artifacts:

```sh
./scripts/clean.sh --all
```
