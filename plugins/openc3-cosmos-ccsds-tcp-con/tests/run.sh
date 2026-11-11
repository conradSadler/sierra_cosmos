#!/usr/bin/env bash
#
# Purpose: This script is intended to automate doctests for tcp_con.c
#
#################################################################################

set -euo pipefail
cd "$(dirname "$0")/.."

coverage=false
if [[ "${1:-}" == "--coverage" ]]; then
  coverage=true
  shift
fi
build_dir=$(mktemp -d "${TMPDIR:-/tmp}/tcp-con-tests.XXXXXX")
trap 'rm -rf "$build_dir"' EXIT

flags=(-g -O0 -Wall -Wextra)
if $coverage; then
  flags+=(-fprofile-instr-generate -fcoverage-mapping)
fi

# Only the production C object receives the test substitutions.
"${CC:-clang}" -std=c11 "${flags[@]}" -DTCP_CON_TEST_BUILD \
  -include tests/test_io.h -c tcp_con.c -o "$build_dir/tcp_con.o"
"${CXX:-clang++}" -std=c++11 "${flags[@]}" tests/tcp_con_test.cpp \
  "$build_dir/tcp_con.o" -o "$build_dir/tcp_con_test"
LLVM_PROFILE_FILE="$build_dir/tests.profraw" "$build_dir/tcp_con_test" "$@"

if $coverage; then
  if command -v xcrun >/dev/null 2>&1; then
    profdata=$(xcrun --find llvm-profdata)
    cov=$(xcrun --find llvm-cov)
  else
    profdata=${LLVM_PROFDATA:-llvm-profdata}
    cov=${LLVM_COV:-llvm-cov}
  fi
  "$profdata" merge -sparse "$build_dir/tests.profraw" -o "$build_dir/tests.profdata"
  "$cov" report "$build_dir/tcp_con_test" \
    -instr-profile="$build_dir/tests.profdata" "$PWD/tcp_con.c"
  mkdir -p tests/coverage
  "$cov" show "$build_dir/tcp_con_test" \
    -instr-profile="$build_dir/tests.profdata" "$PWD/tcp_con.c" \
    -show-branches=count -format=html > tests/coverage/index.html
  echo "Coverage details: $PWD/tests/coverage/index.html"
fi
