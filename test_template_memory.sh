#!/bin/bash

# Script to test template instantiation memory consumption
# This will build the project and monitor memory usage

set -e

echo "=================================="
echo "Template Memory Consumption Test"
echo "=================================="
echo ""

# Check if we have the necessary tools
if ! command -v free &> /dev/null; then
    echo "WARNING: 'free' command not found. Memory monitoring will be limited."
fi

# Clean build
echo "Step 1: Cleaning previous build..."
make clean
rm -rf build/CMakeFiles

echo ""
echo "Step 2: Starting build with memory monitoring..."
echo "This may take several minutes. Watch for high memory usage."
echo ""

# Create a simple memory monitoring function
monitor_memory() {
    local max_memory=0
    while true; do
        # Find memory usage of compilation processes
        memory=$(ps aux | grep -E "(cc1plus|g\+\+|clang)" | grep -v grep | awk '{sum+=$6} END {print sum/1024}')
        if [ ! -z "$memory" ]; then
            # Convert to integer for comparison
            memory_int=$(echo $memory | cut -d. -f1)
            if [ "$memory_int" -gt "$max_memory" ]; then
                max_memory=$memory_int
                echo "Peak compilation memory so far: ${max_memory} MB ($(date +%H:%M:%S))"
            fi
        fi
        sleep 2
        # Check if compilation is still running
        if ! pgrep -x "cc1plus" > /dev/null && ! pgrep -x "g++" > /dev/null && ! pgrep -x "clang" > /dev/null; then
            break
        fi
    done
    echo "Final peak compilation memory: ${max_memory} MB"
}

# Start memory monitoring in background
monitor_memory &
MONITOR_PID=$!

# Build with timing
echo "Building with 4 parallel jobs (to better observe per-process memory)..."
START_TIME=$(date +%s)

if time make -j4; then
    END_TIME=$(date +%s)
    BUILD_TIME=$((END_TIME - START_TIME))

    echo ""
    echo "=================================="
    echo "Build SUCCEEDED"
    echo "=================================="
    echo "Total build time: ${BUILD_TIME} seconds ($((BUILD_TIME / 60)) minutes)"

    # Wait for monitor to finish
    wait $MONITOR_PID 2>/dev/null || true

    # Check object file sizes
    echo ""
    echo "Object file sizes:"
    if [ -f "build/CMakeFiles/txlog.dir/src/rrr/rpc/server.cpp.o" ]; then
        SERVER_SIZE=$(ls -lh build/CMakeFiles/txlog.dir/src/rrr/rpc/server.cpp.o | awk '{print $5}')
        echo "  server.cpp.o: ${SERVER_SIZE}"
    fi

    if [ -f "build/CMakeFiles/test_future.dir/test/test_future.cc.o" ]; then
        TEST_SIZE=$(ls -lh build/CMakeFiles/test_future.dir/test/test_future.cc.o | awk '{print $5}')
        echo "  test_future.cc.o: ${TEST_SIZE}"
    fi

    echo ""
    echo "Running tests to verify functionality..."
    if [ -f "build/test_future" ]; then
        if ./build/test_future; then
            echo "Tests PASSED"
        else
            echo "Tests FAILED"
            exit 1
        fi
    else
        echo "test_future not found, skipping tests"
    fi

    echo ""
    echo "=================================="
    echo "TEST RESULTS SUMMARY"
    echo "=================================="
    echo ""
    echo "EXPECTED with template approach:"
    echo "  - Peak memory: 4000-8000 MB per process"
    echo "  - Build time: 300-600 seconds (5-10 minutes)"
    echo "  - server.cpp.o: 50-100 MB"
    echo ""
    echo "EXPECTED with raw pointer approach:"
    echo "  - Peak memory: 500-1000 MB per process"
    echo "  - Build time: 60-120 seconds (1-2 minutes)"
    echo "  - server.cpp.o: 5-10 MB"
    echo ""
    echo "Compare your results above with these expectations."
    echo ""

else
    END_TIME=$(date +%s)
    BUILD_TIME=$((END_TIME - START_TIME))

    echo ""
    echo "=================================="
    echo "Build FAILED"
    echo "=================================="
    echo "Build time before failure: ${BUILD_TIME} seconds"

    # Kill monitor
    kill $MONITOR_PID 2>/dev/null || true

    echo ""
    echo "Possible reasons:"
    echo "  1. Out of memory (OOM) - check dmesg | tail"
    echo "  2. Compilation timeout"
    echo "  3. Template instantiation depth exceeded"
    echo ""
    exit 1
fi

echo "Test complete!"
echo ""
echo "To revert to the working version:"
echo "  git checkout src/rrr/reactor/coroutine.h"
echo "  git checkout src/rrr/rpc/server.cpp"
