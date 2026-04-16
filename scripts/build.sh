#!/usr/bin/env bash
# 一键配置、编译、测试（开发前验证与日常构建）
set -euo pipefail

cd "$(dirname "$0")/.."

BUILD_TYPE="${CMAKE_BUILD_TYPE:-Debug}"
BUILD_DIR="${BUILD_DIR:-build}"
TESTING="${BUILD_TESTING:-ON}"
RUN_TESTS="${RUN_TESTS:-1}"

usage() {
  echo "Usage: $0 [OPTIONS]"
  echo "  -r          Release 构建（默认 Debug）"
  echo "  -b DIR      构建目录（默认 build）"
  echo "  -n          不启用测试（BUILD_TESTING=OFF）"
  echo "  -T          构建但不运行 ctest"
  echo "  -h          显示此帮助"
  exit 0
}

while getopts "rb:nTh" opt; do
  case "$opt" in
    r) BUILD_TYPE=Release ;;
    b) BUILD_DIR="$OPTARG" ;;
    n) TESTING=OFF ;;
    T) RUN_TESTS=0 ;;
    h) usage ;;
    \?) usage ;;
  esac
done

echo "=== 配置 (${BUILD_TYPE}, BUILD_TESTING=${TESTING}) ==="
cmake -S . -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  -DBUILD_TESTING="${TESTING}"

echo "=== 编译 ==="
cmake --build "${BUILD_DIR}" -j

if [[ "${TESTING}" == "ON" ]] && [[ "$RUN_TESTS" -eq 1 ]]; then
  echo "=== 测试 ==="
  ctest --test-dir "${BUILD_DIR}" --output-on-failure
fi

echo "=== 完成 ==="
