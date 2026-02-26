#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_type="Debug"
run_tests=0

for arg in "$@"; do
  case "$arg" in
    --debug) build_type="Debug" ;;
    --release) build_type="Release" ;;
    --relwithdebinfo) build_type="RelWithDebInfo" ;;
    --minsizerel) build_type="MinSizeRel" ;;
    Debug|Release|RelWithDebInfo|MinSizeRel) build_type="$arg" ;;
    --test) run_tests=1 ;;
    *)
      echo "Usage: $0 [--debug|--release|--relwithdebinfo|--minsizerel|Debug|Release|RelWithDebInfo|MinSizeRel] [--test]" >&2
      exit 1
      ;;
  esac
done

build_dir="${root_dir}/build-$(echo "$build_type" | tr '[:upper:]' '[:lower:]')"

cmake -S "$root_dir" -B "$build_dir" \
  -DCMAKE_BUILD_TYPE="$build_type" \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_OBJCXX_COMPILER=clang++ \
  -DPRIMESTAGE_BUILD_EXAMPLES=ON \
  -DPRIMESTAGE_ENABLE_PRIMEMANIFEST=ON \
  -DPRIMESTAGE_FETCH_PRIMEHOST=ON \
  -DPRIMESTAGE_BUILD_TESTS=ON

cmake --build "$build_dir"

if [[ "$run_tests" -eq 1 ]]; then
  ctest --test-dir "$build_dir" --output-on-failure
fi
