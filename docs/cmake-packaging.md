# PrimeStage CMake Packaging

PrimeStage exports an installable CMake package for downstream consumers.

## Install To A Prefix

Configure and build PrimeStage, then install it:

```sh
./scripts/compile.sh --release
cmake --install build-release --prefix /tmp/primestage-install
```

Installed package files:
- `lib/cmake/PrimeStage/PrimeStageConfig.cmake`
- `lib/cmake/PrimeStage/PrimeStageConfigVersion.cmake`
- `lib/cmake/PrimeStage/PrimeStageTargets.cmake`
- public headers under `include/PrimeStage`

## Consumer Integration (`find_package`)

Consumer `CMakeLists.txt`:

```cmake
find_package(PrimeStage CONFIG REQUIRED)
add_executable(app main.cpp)
target_link_libraries(app PRIVATE PrimeStage::PrimeStage)
```

Configure consumer with the install prefix in `CMAKE_PREFIX_PATH`:

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/tmp/primestage-install
cmake --build build
```

## Presubmit/CTest Verification

PrimeStage includes a package smoke test (`PrimeStage_find_package_smoke`) that:
1. installs the current build to a staging prefix
2. configures a tiny consumer with `find_package(PrimeStage)`
3. builds the consumer target

This guards install/export package regressions in the standard `./scripts/compile.sh --test` flow.
