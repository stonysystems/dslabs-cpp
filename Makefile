

# Variables
BUILD_DIR = build

.PHONY: all configure configure-labtest build dbtest labtest clean rebuild run test test-verbose test-parallel run-raft-tests run-kv-tests run-shard-tests run-all-lab-tests

# Default target: build labtest with slim build (excludes erpc, rust, mako)
all: labtest

# Standard configure (full build with mako, erpc, rust)
configure:
	cmake -S . -B $(BUILD_DIR)

# Configure for lab tests (slim build - excludes erpc, rust, mako)
configure-labtest:
	cmake -S . -B $(BUILD_DIR) -DBUILD_RAFT_LAB_TESTS=ON

# Full build (all targets)
build: configure
	@echo "Building with $(if $(filter -j%,$(MAKEFLAGS)),$(subst -j,,$(filter -j%,$(MAKEFLAGS))),4) parallel jobs..."
	cmake --build $(BUILD_DIR) --parallel $(if $(filter -j%,$(MAKEFLAGS)),$(subst -j,,$(filter -j%,$(MAKEFLAGS))),4)

# Build dbtest (requires full build with mako)
dbtest: configure
	@echo "Building dbtest target..."
	cmake --build $(BUILD_DIR) --target dbtest --parallel $(if $(filter -j%,$(MAKEFLAGS)),$(subst -j,,$(filter -j%,$(MAKEFLAGS))),4)

# Build labtest (slim build - default target)
labtest: configure-labtest
	@echo "Building labtest target (slim build)..."
	cmake --build $(BUILD_DIR) --target labtest --parallel $(if $(filter -j%,$(MAKEFLAGS)),$(subst -j,,$(filter -j%,$(MAKEFLAGS))),4)  

clean:
	rm -rf $(BUILD_DIR)
	# Clean out-perf.masstree
	rm -rf ./out-perf.masstree/*
	# Clean mako out-perf.masstree
	rm -rf ./src/mako/out-perf.masstree/*
	# Clean Masstree configuration
	@echo "Cleaning Masstree configuration..."
	@cd src/mako/masstree && make distclean 2>/dev/null || true
	@rm -f src/mako/masstree/config.h src/mako/masstree/config.h.in
	@rm -f src/mako/masstree/configure src/mako/masstree/config.status
	@rm -f src/mako/masstree/config.log src/mako/masstree/GNUmakefile
	@rm -f src/mako/masstree/autom4te.cache -rf
	# Clean LZ4 library
	@echo "Cleaning LZ4 library..."
	@cd third-party/lz4 && make clean 2>/dev/null || true
	@rm -f third-party/lz4/liblz4.so third-party/lz4/*.o
	# Clean Rust library
	@echo "Cleaning Rust library..."
	@cd rust-lib && cargo clean 2>/dev/null || true
	# Clean rusty-cpp
	@rm -rf third-party/rusty-cpp/target || true




rebuild: clean all

run: build
	./$(BUILD_DIR)/dbtest
	./$(BUILD_DIR)/simpleTransction
	./$(BUILD_DIR)/simpleTransctionRep
	./$(BUILD_DIR)/simplePaxos

# Run Raft lab tests
run-raft-tests: labtest
	@echo "Running Raft lab tests..."
	./$(BUILD_DIR)/labtest -f config/raft_lab_test.yml

# Run KV lab tests
run-kv-tests: labtest
	@echo "Running KV lab tests..."
	./$(BUILD_DIR)/labtest -f config/kv_lab_test.yml

# Run Shard lab tests
run-shard-tests: labtest
	@echo "Running Shard lab tests..."
	./$(BUILD_DIR)/labtest -f config/shard_lab_test.yml

# Run all lab tests
run-all-lab-tests: labtest
	@echo "Running all lab tests..."
	./test/run_all_lab_tests.sh

# Run tests using ctest
test: build
	@echo "Running tests..."
	@cd $(BUILD_DIR) && ctest --output-on-failure

# Run tests with verbose output
test-verbose: build
	@echo "Running tests with verbose output..."
	@cd $(BUILD_DIR) && ctest --verbose --output-on-failure

# Run tests in parallel
test-parallel: build
	@echo "Running tests in parallel..."
	@cd $(BUILD_DIR) && ctest -j$(if $(filter -j%,$(MAKEFLAGS)),$(subst -j,,$(filter -j%,$(MAKEFLAGS))),4) --output-on-failure




