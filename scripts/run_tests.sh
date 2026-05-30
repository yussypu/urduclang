#!/bin/bash
# run_tests.sh - Test runner for Urdu-Clang
# اردو-کلینگ ٹیسٹ رنر
#
# Transforms, compiles, and runs every test/*.c. When a matching
# test/expected/<name>.txt exists, the program's output is diffed against it.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"
TEST_DIR="${PROJECT_DIR}/test"
EXPECTED_DIR="${TEST_DIR}/expected"
URDUC="${BUILD_DIR}/urduc-tool"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo ""
echo "اردو-کلینگ ٹیسٹ سویٹ / Urdu-Clang test suite"
echo ""

if [[ ! -x "$URDUC" ]]; then
    echo -e "${RED}urduc-tool not found at ${URDUC}${NC}"
    echo "Build first:  cmake -S . -B build && cmake --build build -j"
    exit 1
fi

PASSED=0
FAILED=0
TOTAL=0

run_test() {
    local test_file="$1"
    local test_name
    test_name="$(basename "$test_file" .c)"
    local output_file="${BUILD_DIR}/${test_name}"
    local expected="${EXPECTED_DIR}/${test_name}.txt"
    local actual="${BUILD_DIR}/${test_name}_output.txt"

    TOTAL=$((TOTAL + 1))
    echo -n "Testing ${test_name}... "

    if ! "$URDUC" -E "$test_file" > "${BUILD_DIR}/${test_name}_transformed.c" 2>/dev/null; then
        echo -e "${RED}[تبدیلی ✗]${NC} transform failed"
        FAILED=$((FAILED + 1))
        return
    fi
    echo -n "[تبدیلی ✓] "

    if ! "$URDUC" "$test_file" -o "$output_file" >/dev/null 2>&1; then
        echo -e "${RED}[تالیف ✗]${NC} compile failed"
        FAILED=$((FAILED + 1))
        return
    fi
    echo -n "[تالیف ✓] "

    if ! "$output_file" > "$actual" 2>&1; then
        echo -e "${RED}[چلانا ✗]${NC} run failed"
        FAILED=$((FAILED + 1))
        return
    fi
    echo -n "[چلانا ✓] "

    if [[ -f "$expected" ]]; then
        if diff -q "$expected" "$actual" >/dev/null; then
            echo -e "${GREEN}[توقع ✓]${NC}"
            PASSED=$((PASSED + 1))
        else
            echo -e "${RED}[توقع ✗]${NC} output differs from expected"
            diff "$expected" "$actual" | sed 's/^/    /'
            FAILED=$((FAILED + 1))
        fi
    else
        echo -e "${GREEN}✓${NC} ${YELLOW}(no expected file)${NC}"
        PASSED=$((PASSED + 1))
    fi
}

echo "Running tests from: $TEST_DIR"
echo ""

shopt -s nullglob
for test_file in "${TEST_DIR}"/*.c; do
    run_test "$test_file"
done
shopt -u nullglob

echo ""
echo "نتائج / Results:"
echo "  کل / Total:      $TOTAL"
echo -e "  ${GREEN}کامیاب / Passed: $PASSED${NC}"
if [[ $FAILED -gt 0 ]]; then
    echo -e "  ${RED}ناکام / Failed:  $FAILED${NC}"
fi
echo ""

if [[ $FAILED -eq 0 && $TOTAL -gt 0 ]]; then
    echo -e "${GREEN}✓ تمام ٹیسٹ کامیاب! / All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}✗ کچھ ٹیسٹ ناکام ہوئے / Some tests failed${NC}"
    exit 1
fi
