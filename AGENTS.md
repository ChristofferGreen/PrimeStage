# AGENTS.md for PrimeStage

## Scope
Defines naming and coding rules plus build/test entrypoints for contributors working in this repo.

## Naming rules
- **Namespaces:** PascalCase for public namespaces (e.g., `PrimeStage`).
- **Types/structs/classes/enums/aliases:** PascalCase.
- **Functions (free/member/static):** lowerCamelCase.
- **Struct/class fields:** lowerCamelCase.
- **Local/file-only helpers:** lower_snake_case is acceptable when marked `static` in a `.cpp`.
- **Constants (`constexpr`/`static`):** PascalCase; avoid `k`-prefixes.
- **Macros:** avoid new ones; if unavoidable, use `PS_` prefix and ALL_CAPS.

## Build/test workflow
- Preferred build: `scripts/compile.sh [--debug|--release|--relwithdebinfo|--minsizerel] [--test] [--warnings-as-errors] [--asan] [--ubsan]` (uses clang, builds examples by default, tests built by default; sanitizer mode disables examples and `--test` runs tests).
- Debug: `cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug` then `cmake --build build-debug`.
- Release: `cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release` then `cmake --build build-release`.
- Options: `-DPRIMESTAGE_BUILD_TESTS=ON/OFF`, `-DPRIMESTAGE_BUILD_EXAMPLES=ON/OFF`, `-DPRIMESTAGE_WARNINGS_AS_ERRORS=ON/OFF`, `-DPRIMESTAGE_ENABLE_ASAN=ON/OFF`, `-DPRIMESTAGE_ENABLE_UBSAN=ON/OFF`.
- Tests: from a build dir, run `ctest --output-on-failure` or execute `./PrimeStage_tests` directly.

## Tests
- Unit tests live in `tests/unit` and use doctest.

## Code guidelines
- Target C++23; prefer value semantics, RAII, `std::span`, and `std::optional`/`std::expected`.
- Use `std::chrono` types for durations/timeouts.
- Keep hot paths allocation-free; reuse buffers and caches where possible.
- Avoid raw `new`; use `std::unique_ptr` or `std::shared_ptr` with clear ownership intent.
