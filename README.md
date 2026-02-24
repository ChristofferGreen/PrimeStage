# PrimeStage

PrimeStage is the UI authoring layer that owns ground-truth widget state and emits rect hierarchies for PrimeFrame. The Studio UI kit now lives in the PrimeStudio repo.

## Building

```sh
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug
```

```sh
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
```

Optional configuration:
- `-DPRIMESTAGE_BUILD_TESTS=ON/OFF` toggles tests.
- `-DPRIMESTAGE_BUILD_EXAMPLES=ON/OFF` toggles example binaries.

## Tests

From a build dir:

```sh
ctest --output-on-failure
```

Or run the test binary directly:

```sh
./PrimeStage_tests
```

## Docs
- `docs/prime-stage-design.md`
