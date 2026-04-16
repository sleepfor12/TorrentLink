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

echo "Formatting ${#files[@]} files..."
for f in "${files[@]}"; do
  [[ -f "$f" ]] && clang-format -i "$f"
done

echo "clang-format completed."

