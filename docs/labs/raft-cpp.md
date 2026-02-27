# Lab 1 (C++): Raft Guide

## Scope

This guide is for the current lab tree in this repository (`lab-solution-26sp` style layout).

Recent repo changes that matter for this lab:

- Raft code path is `src/protocol/raft` (not `src/deptran/raft`).
- Build/test flow is CMake + `labtest`.
- Many non-lab protocols/benchmarks were removed for faster lab builds.

If you are referencing the historical skeleton branch `lab-25-private-1013`, note the path mapping:

- old: `src/deptran/raft/*`
- current: `src/protocol/raft/*`

## Build And Run (C++ Raft)

### Option A: Makefile wrapper

```bash
make labtest
./build/labtest -f config/raft_lab_test.yml
```

### Option B: Explicit CMake

```bash
cmake -S . -B build-cpp -DBUILD_RAFT_LAB_TESTS=ON
cmake --build build-cpp --target labtest -j$(nproc)
./build-cpp/labtest -f config/raft_lab_test.yml
```

The test source is in:

- `test/labtest.cc`
- `test/labtestconf.cc`

## Where To Implement

Primary files for C++ Raft lab work:

- `src/protocol/raft/server.h`
- `src/protocol/raft/server.cc`
- `src/protocol/raft/commo.h`
- `src/protocol/raft/commo.cc`
- `src/protocol/raft/service.h`
- `src/protocol/raft/service.cc`
- `src/protocol/raft/raft_rpc.rpc` (RPC signatures, if needed)

## Required Behaviors

You must implement standard Raft leader election + log replication behavior expected by lab tests:

- `Start(...)`
  - Return `false` if not leader.
  - If leader, append locally and start replication.
  - Fill `index` and `term` outputs.
- `GetState(...)`
  - Return current term and leader status.
- Commit/apply path
  - Invoke `app_next_` exactly once per committed command, in index order.
- Leader election
  - randomized election timeout
  - vote/term rules from Raft Figure 2
- AppendEntries
  - heartbeat behavior
  - log consistency checks
  - follower log overwrite/truncation on conflict
  - commit index advancement

## Suggested Implementation Order

1. Election + heartbeats (tests 1-2)
2. Basic append/replication (tests 3-4)
3. Failure/rejoin paths (tests 5-8)
4. RPC efficiency + unstable network + figure8 (tests 9-11)

## Useful Test Commands

Run Raft only:

```bash
./build/labtest -f config/raft_lab_test.yml
```

Run all labs in C++ mode:

```bash
./build/labtest -f config/raft_lab_test.yml
./build/labtest -f config/kv_lab_test.yml
./build/labtest -f config/shard_lab_test.yml
```

## Debugging Notes

- Use targeted `Log_info`/`Log_debug` in:
  - election state transitions
  - vote responses
  - append replies (`nextIndex` / `matchIndex` updates)
  - commit/apply progression
- Keep locking discipline consistent (`mtx_` usage in server state transitions).
- Avoid busy-wait loops; use existing event/coroutine waits.

