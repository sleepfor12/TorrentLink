#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

if ! command -v clang-format >/dev/null 2>&1; then
  echo "clang-format not found" >&2
  exit 1
fi

files="$(git ls-files '*.h' '*.hpp' '*.hh' '*.c' '*.cc' '*.cpp' '*.cxx')"
if [[ -z "${files}" ]]; then
  exit 0
fi

bad=0
while IFS= read -r f; do
  if [[ ! -f "$f" ]]; then
    continue
  fi
  if ! clang-format --dry-run --Werror "$f"; then
    bad=1
  fi
done <<< "${files}"

exit "${bad}"

