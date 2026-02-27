#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
dry_run=0
include_all=0

usage() {
  echo "Usage: $0 [--dry-run] [--all]" >&2
}

for arg in "$@"; do
  case "$arg" in
    --dry-run) dry_run=1 ;;
    --all) include_all=1 ;;
    *)
      usage
      exit 1
      ;;
  esac
done

remove_path() {
  local path="$1"
  if [[ "$dry_run" -eq 1 ]]; then
    echo "would remove: $path"
  else
    rm -rf "$path"
    echo "removed: $path"
  fi
}

patterns=(
  "$root_dir/build"
  "$root_dir/build-*"
  "$root_dir/build_*"
)

if [[ "$include_all" -eq 1 ]]; then
  patterns+=(
    "$root_dir/.cache"
    "$root_dir/compile_commands.json"
  )
fi

removed=0
for pattern in "${patterns[@]}"; do
  while IFS= read -r path; do
    [[ -z "$path" ]] && continue
    remove_path "$path"
    removed=$((removed + 1))
  done < <(compgen -G "$pattern" || true)
done

if [[ "$removed" -eq 0 ]]; then
  echo "nothing to clean"
fi
