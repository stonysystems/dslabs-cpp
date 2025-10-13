#!/bin/bash

echo "Running RRR Function Tests"
echo "=========================="

# Compile and run the simple marshal test
echo "Building and running marshal tests..."
g++ -std=c++17 -I../src -I../src/rrr -o test_marshal_simple test_marshal_simple.cc \
    -L../build -lpthread 2>/dev/null

if [ -f test_marshal_simple ]; then
    ./test_marshal_simple
    rm test_marshal_simple
else
    echo "Failed to build marshal tests"
fi

echo ""
echo "Test suite completed!"