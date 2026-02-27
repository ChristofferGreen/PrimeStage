#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_type="Debug"
run_tests=0
warnings_as_errors=0
enable_asan=0
enable_ubsan=0
enable_coverage=0
run_perf=0
check_perf_budget=0
build_examples="ON"

for arg in "$@"; do
  case "$arg" in
    --debug) build_type="Debug" ;;
    --release) build_type="Release" ;;
    --relwithdebinfo) build_type="RelWithDebInfo" ;;
    --minsizerel) build_type="MinSizeRel" ;;
    Debug|Release|RelWithDebInfo|MinSizeRel) build_type="$arg" ;;
    --test) run_tests=1 ;;
    --coverage) enable_coverage=1; run_tests=1 ;;
    --warnings-as-errors) warnings_as_errors=1 ;;
    --asan) enable_asan=1 ;;
    --ubsan) enable_ubsan=1 ;;
    --perf) run_perf=1 ;;
    --perf-budget) run_perf=1; check_perf_budget=1 ;;
    *)
      echo "Usage: $0 [--debug|--release|--relwithdebinfo|--minsizerel|Debug|Release|RelWithDebInfo|MinSizeRel] [--test] [--coverage] [--warnings-as-errors] [--asan] [--ubsan] [--perf] [--perf-budget]" >&2
      exit 1
      ;;
  esac
done

if [[ "$enable_coverage" -eq 1 && ( "$enable_asan" -eq 1 || "$enable_ubsan" -eq 1 ) ]]; then
  echo "[compile.sh] ERROR: --coverage cannot be combined with --asan/--ubsan." >&2
  exit 2
fi

if [[ "$enable_asan" -eq 1 || "$enable_ubsan" -eq 1 ]]; then
  build_examples="OFF"
fi

coverage_cmake_args=()
if [[ "$enable_coverage" -eq 1 ]]; then
  if ! command -v clang++ >/dev/null 2>&1; then
    echo "[compile.sh] ERROR: --coverage requires clang++ in PATH." >&2
    exit 2
  fi
  coverage_cxx_flags="${CXXFLAGS:-} -fprofile-instr-generate -fcoverage-mapping"
  coverage_link_flags="${LDFLAGS:-} -fprofile-instr-generate"
  coverage_cmake_args+=("-DCMAKE_CXX_FLAGS=${coverage_cxx_flags}")
  coverage_cmake_args+=("-DCMAKE_EXE_LINKER_FLAGS=${coverage_link_flags}")
fi

build_dir="${root_dir}/build-$(echo "$build_type" | tr '[:upper:]' '[:lower:]')"

cmake -S "$root_dir" -B "$build_dir" \
  -DCMAKE_BUILD_TYPE="$build_type" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_OBJCXX_COMPILER=clang++ \
  -DPRIMESTAGE_BUILD_EXAMPLES="$build_examples" \
  -DPRIMESTAGE_ENABLE_PRIMEMANIFEST=ON \
  -DPRIMESTAGE_FETCH_PRIMEHOST=ON \
  -DPRIMESTAGE_BUILD_TESTS=ON \
  -DPRIMESTAGE_BUILD_BENCHMARKS=ON \
  -DPRIMESTAGE_WARNINGS_AS_ERRORS=$([[ "$warnings_as_errors" -eq 1 ]] && echo ON || echo OFF) \
  -DPRIMESTAGE_ENABLE_ASAN=$([[ "$enable_asan" -eq 1 ]] && echo ON || echo OFF) \
  -DPRIMESTAGE_ENABLE_UBSAN=$([[ "$enable_ubsan" -eq 1 ]] && echo ON || echo OFF) \
  "${coverage_cmake_args[@]}"

compile_commands_path="$build_dir/compile_commands.json"
if [[ -f "$compile_commands_path" ]]; then
  ln -sfn "$compile_commands_path" "$root_dir/compile_commands.json"
fi

cmake --build "$build_dir"

if [[ "$run_tests" -eq 1 ]]; then
  if [[ "$enable_coverage" -eq 1 ]]; then
    llvm_profdata_bin="${LLVM_PROFDATA:-llvm-profdata}"
    llvm_cov_bin="${LLVM_COV:-llvm-cov}"
    if ! command -v "$llvm_profdata_bin" >/dev/null 2>&1; then
      if command -v xcrun >/dev/null 2>&1; then
        llvm_profdata_bin="$(xcrun --find llvm-profdata 2>/dev/null || true)"
      fi
    fi
    if ! command -v "$llvm_cov_bin" >/dev/null 2>&1; then
      if command -v xcrun >/dev/null 2>&1; then
        llvm_cov_bin="$(xcrun --find llvm-cov 2>/dev/null || true)"
      fi
    fi

    if [[ -z "$llvm_profdata_bin" || ! -x "$llvm_profdata_bin" ]]; then
      echo "[compile.sh] ERROR: llvm-profdata not found; install LLVM tools or set LLVM_PROFDATA." >&2
      exit 2
    fi
    if [[ -z "$llvm_cov_bin" || ! -x "$llvm_cov_bin" ]]; then
      echo "[compile.sh] ERROR: llvm-cov not found; install LLVM tools or set LLVM_COV." >&2
      exit 2
    fi

    coverage_dir="$build_dir/coverage"
    profile_dir="$coverage_dir/profraw"
    rm -rf "$coverage_dir"
    mkdir -p "$profile_dir"

    (cd "$build_dir" && LLVM_PROFILE_FILE="$profile_dir/%p-%m.profraw" ctest --output-on-failure)

    if ! compgen -G "$profile_dir/*.profraw" >/dev/null; then
      echo "[compile.sh] WARN: no .profraw files produced; coverage report skipped." >&2
      exit 0
    fi

    profdata_file="$coverage_dir/PrimeStage.profdata"
    "$llvm_profdata_bin" merge -sparse "$profile_dir"/*.profraw -o "$profdata_file"

    coverage_bins=()
    for candidate in "$build_dir/PrimeStage_tests" "$build_dir/PrimeFrame_tests"; do
      if [[ -x "$candidate" ]]; then
        coverage_bins+=("$candidate")
      fi
    done
    if [[ "${#coverage_bins[@]}" -eq 0 ]]; then
      echo "[compile.sh] ERROR: no coverage binaries found in $build_dir." >&2
      exit 2
    fi

    coverage_ignore_regex="${PRIMESTAGE_COVERAGE_IGNORE_REGEX:-.*/(third_party|tests|examples|build-.*|out|tools)/?}"
    coverage_args=()
    if [[ -n "$coverage_ignore_regex" ]]; then
      echo "[compile.sh] Coverage: ignoring files matching regex: $coverage_ignore_regex"
      coverage_args=(--ignore-filename-regex "$coverage_ignore_regex")
    fi

    text_report="$coverage_dir/coverage.txt"
    echo "[compile.sh] Generating coverage report -> $text_report"
    "$llvm_cov_bin" report "${coverage_args[@]}" "${coverage_bins[@]}" -instr-profile "$profdata_file" > "$text_report"

    echo "[compile.sh] Generating HTML coverage -> $coverage_dir/html"
    "$llvm_cov_bin" show "${coverage_args[@]}" "${coverage_bins[@]}" -instr-profile "$profdata_file" \
      -format=html -output-dir "$coverage_dir/html" -show-instantiations -show-line-counts-or-regions
  else
    ctest --test-dir "$build_dir" --output-on-failure
  fi
fi

if [[ "$run_perf" -eq 1 ]]; then
  benchmark_bin="$build_dir/PrimeStage_benchmarks"
  if [[ ! -x "$benchmark_bin" ]]; then
    echo "Expected benchmark executable not found: $benchmark_bin" >&2
    exit 1
  fi

  benchmark_args=(
    --output "$build_dir/perf-results.json"
  )
  if [[ "$check_perf_budget" -eq 1 ]]; then
    benchmark_args+=(
      --budget-file "$root_dir/tests/perf/perf_budgets.txt"
      --check-budgets
    )
  fi

  "$benchmark_bin" "${benchmark_args[@]}"
fi
