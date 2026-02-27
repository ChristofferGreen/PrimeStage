#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

failures=0

check_file_limit() {
  local file="$1"
  local max_lines="$2"
  local abs_file="$root_dir/$file"
  if [[ ! -f "$abs_file" ]]; then
    echo "missing architecture hotspot file: $file" >&2
    failures=$((failures + 1))
    return
  fi
  local lines
  lines="$(wc -l <"$abs_file" | tr -d ' ')"
  if (( lines > max_lines )); then
    echo "file size guardrail failed: $file has $lines lines (max $max_lines)" >&2
    failures=$((failures + 1))
  fi
}

measure_function_lines() {
  local file="$1"
  local signature="$2"
  awk -v sig="$signature" '
BEGIN {
  found = 0
  started = 0
  depth = 0
  start = 0
}
{
  if (!found && index($0, sig) > 0) {
    found = 1
    start = NR
  }
  if (found) {
    line = $0
    openCount = gsub(/\{/, "{", line)
    closeCount = gsub(/\}/, "}", line)
    if (!started && openCount > 0) {
      started = 1
    }
    if (started) {
      depth += openCount - closeCount
      if (depth == 0) {
        print NR - start + 1
        exit 0
      }
    }
  }
}
END {
  if (!found) {
    print "MISSING"
    exit 2
  }
  if (found && !started) {
    print "NO_BRACE"
    exit 3
  }
  if (started && depth != 0) {
    print "UNTERMINATED"
    exit 4
  }
}
' "$file"
}

check_function_limit() {
  local file="$1"
  local signature="$2"
  local max_lines="$3"
  local abs_file="$root_dir/$file"
  if [[ ! -f "$abs_file" ]]; then
    echo "missing architecture hotspot file: $file" >&2
    failures=$((failures + 1))
    return
  fi
  local measured
  if ! measured="$(measure_function_lines "$abs_file" "$signature")"; then
    echo "function size guardrail failed: cannot measure '$signature' in $file" >&2
    failures=$((failures + 1))
    return
  fi
  if [[ ! "$measured" =~ ^[0-9]+$ ]]; then
    echo "function size guardrail failed: unexpected measurement '$measured' for '$signature' in $file" >&2
    failures=$((failures + 1))
    return
  fi
  if (( measured > max_lines )); then
    echo "function size guardrail failed: '$signature' in $file has $measured lines (max $max_lines)" >&2
    failures=$((failures + 1))
  fi
}

# File-size guardrails for architecture hotspots.
check_file_limit "src/PrimeStageTreeView.cpp" 1400
check_file_limit "src/PrimeStageTextField.cpp" 1000
check_file_limit "src/PrimeStageSelectableText.cpp" 760
check_file_limit "src/PrimeStageTable.cpp" 560
check_file_limit "src/PrimeStage.cpp" 1800

# Function-size guardrails for largest runtime entrypoints.
check_function_limit "src/PrimeStageTreeView.cpp" "UiNode UiNode::createTreeView(TreeViewSpec const& spec)" 1100
check_function_limit "src/PrimeStageTextField.cpp" "UiNode UiNode::createTextField(TextFieldSpec const& specInput)" 900
check_function_limit "src/PrimeStageSelectableText.cpp" "UiNode UiNode::createSelectableText(SelectableTextSpec const& specInput)" 650
check_function_limit "src/PrimeStageTable.cpp" "UiNode UiNode::createTable(TableSpec const& specInput)" 520

if (( failures > 0 )); then
  echo "architecture size guardrails failed with $failures violation(s)" >&2
  exit 1
fi

echo "architecture size guardrails passed"
