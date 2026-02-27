# Janus Raft Lab Branch

This branch contains the Raft lab code and test harness, with support for:

- C++ Raft implementation (`src/protocol/raft`)
- Rust Raft implementation (`src/protocol/rust_raft`) via FFI bridge

It is configured for fast lab iteration with a slim build path.

## Repository Layout

- `src/protocol/raft/` - C++ Raft implementation
- `src/protocol/rust_raft/` - Rust Raft server logic and wrappers
- `src/protocol/raft/rust_ffi_wrapper.cc` - C++ <-> Rust bridge
- `test/labtest.cc`, `test/labtestconf.cc` - lab test harness
- `docs/labs/raft-cpp.md` - C++ lab guide
- `docs/labs/raft-rust.md` - Rust lab guide

## Build And Run

### C++ Raft Lab Test

```bash
cmake -S . -B build-cpp -DBUILD_RAFT_LAB_TESTS=ON
cmake --build build-cpp --target labtest -j$(nproc)
./build-cpp/labtest -f config/raft_lab_test.yml
```

### Rust Raft Lab Test

```bash
cmake -S . -B build-rust -DBUILD_RAFT_LAB_TESTS=ON -DUSE_RUST_RAFT=ON
cmake --build build-rust --target labtest -j$(nproc)
./build-rust/labtest -f config/raft_lab_test.yml
```

## Optional Makefile Shortcuts

```bash
make labtest
./build/labtest -f config/raft_lab_test.yml
```

Note: `make labtest` builds the default (C++) labtest binary under `build/`.

## Other Lab Tests

C++ mode also supports:

```bash
./build-cpp/labtest -f config/kv_lab_test.yml
./build-cpp/labtest -f config/shard_lab_test.yml
```

## Environment Notes

- `env.txt` is required by CMake.
- Rust mode requires a working Rust toolchain (`cargo`, `rustc`).
- This branch intentionally excludes many non-lab protocols/bench workloads.

