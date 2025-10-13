# Lab Instructions for Lab-25

## Building and Running Tests

### Building

To build the lab tests, you need to enable the `BUILD_RAFT_LAB_TESTS` CMake option:

```bash
make labtest
```

This will:
- Configure CMake with `-DBUILD_RAFT_LAB_TESTS=ON`
- Build the `labtest` executable with `RAFT_TEST_CORO` defined
- Place the executable in `./build/labtest`

### Running Tests

#### Raft Tests (Lab 1)
```bash
./build/labtest -f config/raft_lab_test.yml
```

#### KV Tests (Lab 2)
```bash
./build/labtest -f config/kv_lab_test.yml
```

#### Shard Tests (Lab 3)
```bash
./build/labtest -f config/shard_lab_test.yml
```

### Expected Behavior

With the skeleton code:
- Tests should run and report failures
- You will see log messages indicating which test is running
- Failed tests will print error messages like:
  ```
  TEST 1: Initial election
  TEST 1 Failed: waited too long for leader election
  TESTS FAILED
  ```
- The test will exit with a non-zero status code
- The process should exit cleanly after about 5-10 seconds

### Skeleton Code Structure

All three labs have stub implementations:

#### Lab 1: Raft (src/deptran/raft/server.cc)
- `RequestVote()` - Returns false
- `OnRequestVote()` - Sets default values and calls callback
- `OnAppendEntries()` - Sets default values and calls callback

#### Lab 2: KV (src/kv/server.cc)
- `Put()` - Returns KV_SUCCESS
- `Get()` - Returns KV_SUCCESS
- `Append()` - Returns KV_SUCCESS
- `OnNextCommand()` - Empty implementation

#### Lab 3: ShardKV & ShardMaster
- src/shardkv/server.cc: Put/Get/Append/OnNextCommand stubs
- src/shardmaster/service.cc: Join/Leave/Move/Query/OnNextCommand stubs

All stub methods have comments indicating where students should add their code.
