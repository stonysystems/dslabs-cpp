# Lab 1 (Rust): Raft Guide

## Scope

This guide is for the Rust Raft implementation wired into the same lab harness.

- Rust server logic: `src/protocol/rust_raft/server.rs`
- FFI bridge: `src/protocol/rust_raft/lib.rs`, `src/protocol/raft/rust_ffi_wrapper.cc`
- RPC/type wrappers: `src/protocol/rust_raft/wrappers.rs`

`wrappers.rs` is infrastructure and should generally stay unchanged for lab assignments.

## Build And Run (Rust Raft Mode)

```bash
cmake -S . -B build-rust -DBUILD_RAFT_LAB_TESTS=ON -DUSE_RUST_RAFT=ON
cmake --build build-rust --target labtest -j$(nproc)
./build-rust/labtest -f config/raft_lab_test.yml
```

The test harness is still the same C++ lab runner:

- `test/labtest.cc`
- `test/labtestconf.cc`

## What To Implement In `server.rs`

Core methods to focus on:

- `setup(...)`
- election flow:
  - `start_election_timer(...)`
  - `request_vote(...)`
  - `on_request_vote(...)`
- replication flow:
  - `start_heartbeat_loop(...)`
  - `heartbeat_iteration(...)`
  - `on_append_entries(...)`
- client entry point:
  - `start(...)`
  - `set_local_append(...)`
- apply/commit flow:
  - `apply_logs(...)`

Required outcomes are the same as C++ lab expectations:

- correct leader election
- correct term/vote semantics
- log consistency + overwrite on conflicts
- ordered single-apply semantics through app callback
- robustness under disconnect/reconnect and unreliable links

## RPC And Reply Model (Rust Side)

- Outgoing RPCs are sent via `Commo` wrapper helpers (e.g. `send_append_entries`, `broadcast_vote`).
- Incoming RPC handlers receive RAII reply wrappers:
  - `VoteReply`
  - `AppendEntriesReply`
- Reply callbacks are triggered when wrapper objects are dropped, so handler paths must set reply fields before return.

## Practical Workflow

1. Implement election tests first (1-2).
2. Implement append/commit correctness (3-8).
3. Tune behavior for RPC-count and unreliable/figure8 cases (9-11).
4. Re-run full raft test repeatedly:

```bash
./build-rust/labtest -f config/raft_lab_test.yml
```

