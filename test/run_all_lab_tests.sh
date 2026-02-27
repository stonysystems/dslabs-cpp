#!/bin/bash
# Script to run all three lab tests (Raft, KV, Shard)

set -e  # Exit on error (optional, remove if you want to continue even if one test fails)

echo "======================================"
echo "Running All Lab Tests"
echo "======================================"
echo ""

# Get the directory of this script
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Change to project root
cd "$PROJECT_ROOT"

# Check if labtest executable exists
if [ ! -f "./build/labtest" ]; then
    echo "ERROR: labtest executable not found at ./build/labtest"
    echo "Please run 'make' to build the project first"
    exit 1
fi

echo "Lab 1: Raft Tests"
echo "--------------------------------------"
./build/labtest -f config/raft_lab_test.yml
RAFT_RESULT=$?

echo ""
echo "======================================"
echo "Lab 2: KV Tests"
echo "--------------------------------------"
./build/labtest -f config/kv_lab_test.yml
KV_RESULT=$?

echo ""
echo "======================================"
echo "Lab 3: Shard Tests"
echo "--------------------------------------"
./build/labtest -f config/shard_lab_test.yml
SHARD_RESULT=$?

echo ""
echo "======================================"
echo "Test Results Summary"
echo "======================================"
echo "  Lab 1 (Raft):  $([ $RAFT_RESULT -eq 0 ] && echo '✓ PASSED' || echo '✗ FAILED')"
echo "  Lab 2 (KV):    $([ $KV_RESULT -eq 0 ] && echo '✓ PASSED' || echo '✗ FAILED')"
echo "  Lab 3 (Shard): $([ $SHARD_RESULT -eq 0 ] && echo '✓ PASSED' || echo '✗ FAILED')"
echo "======================================"

# Exit with combined status
TOTAL_FAILURES=$((RAFT_RESULT + KV_RESULT + SHARD_RESULT))
if [ $TOTAL_FAILURES -eq 0 ]; then
    echo "All tests passed!"
    exit 0
else
    echo "$TOTAL_FAILURES test suite(s) failed"
    exit 1
fi
