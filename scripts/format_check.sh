#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

if ! command -v clang-format >/dev/null 2>&1; then
  echo "clang-format not found" >&2
  exit 1
fi

if ! command -v git >/dev/null 2>&1; then
  echo "git not found" >&2
  exit 1
fi

mapfile -d '' files < <(git ls-files -z '*.h' '*.hpp' '*.hh' '*.c' '*.cc' '*.cpp' '*.cxx')
if [[ "${#files[@]}" -eq 0 ]]; then
  echo "No C/C++ source files found."
  exit 0
fi

bad=0
for f in "${files[@]}"; do
  if [[ ! -f "$f" ]]; then
    continue
  fi
  if ! clang-format --dry-run --Werror "$f"; then
    echo "Formatting check failed: $f" >&2
    bad=1
  fi
done

if [[ "${bad}" -ne 0 ]]; then
  echo "Run: bash scripts/format.sh" >&2
fi

exit "${bad}"
