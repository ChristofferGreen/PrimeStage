#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

mapfile -t canonical_examples < <(find "$repo_root/examples/canonical" -type f -name '*.cpp' | sort)

if [[ ${#canonical_examples[@]} -eq 0 ]]; then
  echo "error: no canonical example sources found under examples/canonical" >&2
  exit 1
fi

canonical_docs=(
  "$repo_root/README.md"
  "$repo_root/docs/5-minute-app.md"
)

for doc in "${canonical_docs[@]}"; do
  if [[ ! -f "$doc" ]]; then
    echo "error: expected canonical docs file is missing: $doc" >&2
    exit 1
  fi
done

forbidden_patterns=(
  '#include[[:space:]]*"PrimeFrame/'
  'PrimeFrame::'
  '(^|[^A-Za-z0-9_])NodeId([^A-Za-z0-9_]|$)'
  'lowLevelNodeId[[:space:]]*\('
  'std::get_if<PrimeHost::'
  'PrimeHost::PointerEvent'
  'PrimeHost::KeyEvent'
)

forbidden_reasons=(
  'PrimeFrame header include is forbidden in canonical path'
  'PrimeFrame type usage is forbidden in canonical path'
  'raw NodeId usage is forbidden in canonical path'
  'low-level node-id escape hatch is forbidden in canonical path'
  'manual host-event translation is forbidden in canonical path'
  'manual pointer-event translation is forbidden in canonical path'
  'manual key-event translation is forbidden in canonical path'
)

failures=0

scan_target() {
  local label="$1"
  local file="$2"

  for i in "${!forbidden_patterns[@]}"; do
    local pattern="${forbidden_patterns[$i]}"
    local reason="${forbidden_reasons[$i]}"
    local matches
    matches="$(grep -En "$pattern" "$file" || true)"
    if [[ -n "$matches" ]]; then
      echo "error: $reason ($label)" >&2
      echo "$matches" >&2
      failures=$((failures + 1))
    fi
  done
}

extract_cpp_snippets() {
  local file="$1"
  awk '
    BEGIN {
      in_block = 0;
      capture = 0;
    }
    {
      if ($0 ~ /^```/) {
        if (in_block == 0) {
          in_block = 1;
          lang = tolower(substr($0, 4));
          gsub(/^[[:space:]]+/, "", lang);
          gsub(/[[:space:]]+$/, "", lang);
          capture = (lang == "" || lang == "c" || lang == "cc" || lang == "cpp" ||
                     lang == "cxx" || lang == "c++");
        } else {
          in_block = 0;
          capture = 0;
        }
        next;
      }

      if (in_block == 1 && capture == 1) {
        printf "%d:%s\n", NR, $0;
      }
    }
  ' "$file"
}

for source_file in "${canonical_examples[@]}"; do
  scan_target "$source_file" "$source_file"
done

for doc in "${canonical_docs[@]}"; do
  tmp_file="$(mktemp)"
  extract_cpp_snippets "$doc" > "$tmp_file"
  scan_target "$doc (cpp snippets)" "$tmp_file"
  rm -f "$tmp_file"
done

if [[ $failures -ne 0 ]]; then
  echo "canonical API surface lint failed with $failures violation(s)" >&2
  exit 1
fi

echo "canonical API surface lint passed"
