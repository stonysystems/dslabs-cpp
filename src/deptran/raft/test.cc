#include "test.h"
#include "../../kv/server.h"
#include "../../kv/client.h"
#include "../../shardkv/client.h"
#include "../../shardkv/server.h"
#include "../../shardmaster/service.h"
#include "../../shardmaster/client.h"

namespace janus {

#ifdef RAFT_TEST_CORO

// #define TEST_EXPAND(x) x || x || x || x || x 
#define TEST_EXPAND(x) x 

int RaftLabTest::GenericKvTest(int n_cli, bool unreliable, uint64_t timeout, int leader, int maxraftstate) {
  vector<thread*> threads;
  threads.resize(n_cli, nullptr);
  atomic<int> done{0};
  if (unreliable) {
    config_->SetUnreliable(true);
  }
  for (int i = 0; i < n_cli; i++) {
    threads[i] = new thread([this, i, &done, unreliable, leader](){
      verify(config_->GetKvServer(0) != nullptr); 
      auto cli = config_->GetKvServer(0)->CreateClient();
      cli->leader_idx_ = leader;
      int r = RandomGenerator::rand(0, 10000);
      string k = to_string(i);
      string v1 = to_string(r);
      do {
        auto ret = cli->Put(k, v1);
        if (ret == KV_TIMEOUT) {
          verify(unreliable);
          continue;
        } 
        string v2 = to_string(RandomGenerator::rand(0, 10000));
        ret = cli->Append(k, v2);
        if (ret == KV_TIMEOUT) {
          verify(unreliable);
          continue;
        } 
        string v3; 
        ret = cli->Get(k, &v3);
        if (ret == KV_TIMEOUT) {
          verify(unreliable);
          continue;
        }
        verify(v1+v2 == v3);
        break;
      } while (true);
      done.fetch_add(1);  
      // int ret = cli->Append("test", "hello world");
      // verify(ret == KV_SUCCESS); 
      // string v;
      // ret = cli->Get("test", &v);
      // verify(ret == KV_SUCCESS); 
      // verify(v == "hello world"); 
    });
  }
  auto t1 = Time::now();
  while (true) {
    std::this_thread::sleep_for(100ms);
    if (done.load() == n_cli) {
      break;
    }  
    if (Time::now() - t1 > timeout) {
      Log_fatal("run out of time to finish");
    }
  }
  if (unreliable) {
    config_->SetUnreliable(false);
  }

  if (maxraftstate > 0) {
    int logsize = config_->GetLogSize();
    if (logsize > 8*maxraftstate) {
        verify(0);
    }
  }

  verify(done.load() == n_cli);
  return 0;
}

int RaftLabTest::RunShard(void) {
  Coroutine::Sleep(5000000);
  int leader = config_->OneLeader();   
  if (false
      || TEST_EXPAND(testShardBasic()) 
      || TEST_EXPAND(testShardConcurrent()) 
      || TEST_EXPAND(testShardMinimalTransferJoin()) 
      || TEST_EXPAND(testShardMinimalTransferLeave()) 
      || TEST_EXPAND(testShardStaticShardsPut())
      || TEST_EXPAND(testShardJoinLeaveAppend())
  ) {
    Print("TESTS FAILED");
    return 1;
  }
  Print("ALL TESTS PASSED");
  return 0;
}

int RaftLabTest::RunKv(void) {
  Coroutine::Sleep(5000000);
  int leader = config_->OneLeader();
  if (false
      || TEST_EXPAND(testKvBasic())
      || TEST_EXPAND(testKvConcurrent())
      || TEST_EXPAND(testKvMajority())
      || TEST_EXPAND(testKvMinority())
      || TEST_EXPAND(testKvReElection())
      || TEST_EXPAND(testKvMajorityConcurrent())
      || TEST_EXPAND(testKvMajorityConcurrentSameKey())
      || TEST_EXPAND(testKvMajorityConcurrentLinear())
    //   || TEST_EXPAND(testKvUnreliable())
      || TEST_EXPAND(testKvSnapInstallRPC())
      || TEST_EXPAND(testKvSnapSize())
    ) {
    Print("TESTS FAILED");
    return 1;
  }
  Print("ALL TESTS PASSED");
  return 0;
}

int RaftLabTest::RunRaft(void) {
  config_->SetLearnerAction();
  uint64_t start_rpc = config_->RpcTotal();
  if (testInitialElection()
      || TEST_EXPAND(testReElection())
      || TEST_EXPAND(testBasicAgree())
      // || TEST_EXPAND(testFailAgree())
      // || TEST_EXPAND(testFailNoAgree())
      // || TEST_EXPAND(testRejoin())
      // || TEST_EXPAND(testConcurrentStarts())
      // || TEST_EXPAND(testBackup())
      // || TEST_EXPAND(testCount())
      // || TEST_EXPAND(testUnreliableAgree())
      // || TEST_EXPAND(testFigure8())
      || TEST_EXPAND(testBasicPersistence())
      || TEST_EXPAND(testMorePersistence1())
      || TEST_EXPAND(testMorePersistence2())
    ) {
    Print("TESTS FAILED");
    return 1;
  }
  Print("ALL TESTS PASSED");
  Print("Total RPC count: %ld", config_->RpcTotal() - start_rpc);
  return 0;

}

int RaftLabTest::Run(void) {
  auto config_node = Config::GetConfig()->yaml_config_["lab"];
  if (config_node) {
    if (config_node["shard"].as<bool>()) {
      return RunShard();
    } 
    if (config_node["kv"].as<bool>()) {
      return RunKv();
    }
    if (config_node["raft"].as<bool>()) {
      return RunRaft();
    }
  } else {
    verify(0);
  }
  verify(0);
  return 0;
}

void RaftLabTest::Cleanup(void) {
  config_->Shutdown();
}

#define Init2(test_id, description) \
  Init(test_id, description); \
  verify(config_->NDisconnected() == 0 && !config_->IsUnreliable())
#define Passed2() Passed(); return 0

#define Assert(expr) if (!(expr)) { \
  return 1; \
}
#define Assert2(expr, msg, ...) if (!(expr)) { \
  Failed(msg, ##__VA_ARGS__); \
  return 1; \
}

#define AssertOneLeader(ldr) Assert(ldr >= 0)
#define AssertReElection(ldr, old) \
        Assert2(ldr != old, "no reelection despite leader being disconnected")
#define AssertNoneCommitted(index) { \
        auto nc = config_->NCommitted(index); \
        Assert2(nc == 0, \
                "%d servers unexpectedly committed index %ld", \
                nc, index) \
      }
#define AssertNCommitted(index, expected) { \
        auto nc = config_->NCommitted(index); \
        Assert2(nc == expected, \
                "%d servers committed index %ld (%d expected)", \
                nc, index, expected) \
      }
#define AssertStartOk(ok) Assert2(ok, "unexpected leader change during Start()")
#define AssertWaitNoError(ret, index) \
        Assert2(ret != -3, "committed values differ for index %ld", index)
#define AssertWaitNoTimeout(ret, index, n) \
        Assert2(ret != -1, "waited too long for %d server(s) to commit index %ld", n, index); \
        Assert2(ret != -2, "term moved on before index %ld committed by %d server(s)", index, n)
#define DoAgreeAndAssertIndex(cmd, n, index) { \
        auto r = config_->DoAgreement(cmd, n, false); \
        auto ind = index; \
        Assert2(r > 0, "failed to reach agreement for command %d among %d servers, expected commit index>0, got %" PRId64, cmd, n, r); \
        Assert2(r == ind, "agreement index incorrect. got %ld, expected %ld", r, ind); \
      }
#define DoAgreeAndAssertWaitSuccess(cmd, n) { \
        auto r = config_->DoAgreement(cmd, n, true); \
        Assert2(r > 0, "failed to reach agreement for command %d among %d servers", cmd, n); \
        index_ = r + 1; \
      }

void RaftLabTest::checkShardBasic(const map<uint32_t, vector<uint32_t>>& group_servers) {
  auto cli = config_->GetShardMasterServer(0)->CreateClient();
  ShardConfig config;
  auto ret = cli->Query(-1, &config);
  verify(ret == KV_SUCCESS);
  if (group_servers.size() > 0) {
    verify(config.number > 0);
    if (config.group_servers_map_ != group_servers) {
      Log_fatal("some groups missing!");
    }
  }
  // look for any un-allocated shards
  for (auto& pair : config.shard_group_map_) {
    auto shard = pair.first;
    auto group = pair.second; 
    if (group == 0) continue;
    if (config.group_servers_map_.find(group) == config.group_servers_map_.end()) {
      Log_fatal("Shard %d is not assigned to a valid group", shard);  
    }
  }
  // check if sharding is balanced
  uint32_t n_shard = config.shard_group_map_.size();
  uint32_t n_group = config.group_servers_map_.size();
  uint32_t max_shard_per_group = (n_shard + n_group - 1) / n_group; 
  map<uint32_t, uint32_t> count{};
  for (auto& pair : config.shard_group_map_) {
    auto group = pair.second;
    count[group]++;
  }
  uint32_t max = 0;
  uint32_t min = UINT32_MAX;
  for (auto& pair : count) {
    auto c = pair.second;
    if (c < min) {
      min = c;
    }
    if (c > max) {
      max = c;
    }
  } 
  verify(max <= min+1);
}

int RaftLabTest::testShardConcurrent() {
  Init2(2, "Concurrent shard operations");
  vector<thread*> threads;
  int n_cli = 5;
  threads.resize(n_cli, nullptr);
  atomic<int> done{0};
  // if (unreliable) {
  //   config_->SetUnreliable(true);
  // }
  bool unreliable = false;
  int leader = 0;
  for (int i = 0; i < n_cli; i++) {
    threads[i] = new thread([this, i, &done, unreliable, leader](){
      verify(config_->GetShardMasterServer(0) != nullptr); // TODO initialize config_->GetShardKvServer(0)
      auto cli = config_->GetShardMasterServer(0)->CreateClient();
      cli->leader_idx_ = leader;
      int r = RandomGenerator::rand(0, 10000);
      string k = to_string(i);
      string v1 = to_string(r);
      do {
        map<uint32_t, vector<uint32_t>> group_servers = {{1,{5,6,7,8,9}}};
        auto ret = cli->Join(group_servers);
        verify(ret == KV_SUCCESS);
        checkShardBasic();
        map<uint32_t, vector<uint32_t>> group_servers_2 = {{2,{10,11,12,13,14}}};
        auto ret2 = cli->Join(group_servers_2);
        verify(ret2 == KV_SUCCESS);

        map<uint32_t, vector<uint32_t>> group_servers_3 = {{1,{5,6,7,8,9}},{2,{10,11,12,13,14}}};
        checkShardBasic();
        checkShardBasic(); // check it twice
        auto ret3 = cli->Leave({1});
        checkShardBasic();

        break; 
      } while (true);
      done.fetch_add(1);  
    });
  }
  auto t1 = Time::now();
  while (true) {
    std::this_thread::sleep_for(100ms);
    if (done.load() == n_cli) {
      break;
    }  
    if (Time::now() - t1 > 100000000) {
      Log_fatal("run out of time to finish");
    }
  }
  verify(done.load() == n_cli);
  Passed2();
}

int RaftLabTest::testShardMinimalTransferJoin() {
  Init2(3, "Minimal transfers after joins");
  map<uint32_t, vector<uint32_t>> group_servers_1 = {{1,{5,6,7,8,9}}};
  map<uint32_t, vector<uint32_t>> group_servers_3 = {{3,{15,16,17,18,19}}};
  map<uint32_t, vector<uint32_t>> group_servers_4 = {{4,{20,21,22,23,24}}};
  map<uint32_t, vector<uint32_t>> group_servers_5 = {{5,{25,26,27,28,29}}};
  auto cli = config_->GetShardMasterServer(0)->CreateClient();
  ShardConfig c1, c2;
  cli->Join(group_servers_1);
  cli->Query(-1, &c1);
  cli->Join(group_servers_3);
  cli->Join(group_servers_4);
  cli->Join(group_servers_5);
  cli->Query(-1, &c2);
  for (auto& pair : c2.shard_group_map_) {
    auto& shard = pair.first;
    auto& group = pair.second;
    if (c1.group_servers_map_.count(group) > 0) {
      verify(c1.shard_group_map_[shard] == group);
    } 
  }
  Passed2();
}

int RaftLabTest::testShardMinimalTransferLeave() {
  Init2(4, "Minimal transfers after leaves");
  map<uint32_t, vector<uint32_t>> group_servers_1 = {{1,{5,6,7,8,9}}};
  map<uint32_t, vector<uint32_t>> group_servers_3 = {{3,{15,16,17,18,19}}};
  map<uint32_t, vector<uint32_t>> group_servers_4 = {{4,{20,21,22,23,24}}};
  auto cli = config_->GetShardMasterServer(0)->CreateClient();
  ShardConfig c1, c2;
  cli->Leave({2});
  cli->Query(-1, &c1);
  cli->Leave({3});
  cli->Leave({4});
  cli->Leave({5});
  cli->Query(-1, &c2);
  for (auto& pair : c1.shard_group_map_) {
    auto& shard = pair.first;
    auto& group = pair.second;
    if (c2.group_servers_map_.count(group) > 0) {
      verify(c2.shard_group_map_[shard] == group);
    } 
  }
  Passed2();
}

int RaftLabTest::testShardStaticShardsPut() {
  Init2(5, "Static shards, put");
  auto sm_cli = config_->GetShardMasterServer(0)->CreateClient();
  map<uint32_t, vector<uint32_t>> group_servers_2 = {{2,{10,11,12,13,14}}};
  auto ret = sm_cli->Join(group_servers_2);
  verify(ret == KV_SUCCESS);
  auto kv_cli = ShardKvServer::CreateClient(config_->GetShardMasterServer(0)->sp_log_svr_->commo_);
  string k1 = "1";
  int r1 = RandomGenerator::rand(0,10000);
  string v1 = to_string(r1);
  auto ret1 = kv_cli->Put(k1, v1);
  verify(ret1 == KV_SUCCESS);
  string k2 = "2";
  int r2 = RandomGenerator::rand(0,10000);
  string v2 = to_string(r2);
  auto ret2 = kv_cli->Put(k2, v2);
  verify(ret2 == KV_SUCCESS);
  string val1;
  kv_cli->Get(k1, &val1);
  verify(v1 == val1);
  string val2;
  kv_cli->Get(k2, &val2);
  verify(v2 == val2);
  Passed2();
}

int RaftLabTest::testShardJoinLeaveAppend() {
  Init2(6, "Shard joining and leaving with append");
  auto sm_cli = config_->GetShardMasterServer(0)->CreateClient();
  auto kv_cli = ShardKvServer::CreateClient(config_->GetShardMasterServer(0)->sp_log_svr_->commo_);
  string k1 = "3";
  int r1 = RandomGenerator::rand(0,10000);
  string v1 = to_string(r1);
  auto ret1 = kv_cli->Put(k1, v1);
  verify(ret1 == KV_SUCCESS);
  int r2 = RandomGenerator::rand(0,10000);
  string v2 = to_string(r2);
  auto ret2 = kv_cli->Append(k1, v2);
  verify(ret2 == KV_SUCCESS);
  sm_cli->Leave({3});
  int r3 = RandomGenerator::rand(0,10000);
  string v3 = to_string(r3);
  auto ret3 = kv_cli->Append(k1, v3);
  string val;
  kv_cli->Get(k1, &val);
  verify(val == v1+v2+v3);
  Passed2();
}

int RaftLabTest::testShardBasic() {
  Init2(1, "Basic shard operations");
  vector<thread*> threads;
  int n_cli = 1;
  threads.resize(n_cli, nullptr);
  atomic<int> done{0};
  // if (unreliable) {
  //   config_->SetUnreliable(true);
  // }
  bool unreliable = false;
  int leader = 0;
  for (int i = 0; i < n_cli; i++) {
    threads[i] = new thread([this, i, &done, unreliable, leader](){
      verify(config_->GetShardMasterServer(0) != nullptr); // TODO initialize config_->GetShardKvServer(0)
      auto cli = config_->GetShardMasterServer(0)->CreateClient();
      cli->leader_idx_ = leader;
      int r = RandomGenerator::rand(0, 10000);
      string k = to_string(i);
      string v1 = to_string(r);
      do {
        map<uint32_t, vector<uint32_t>> group_servers = {{1,{5,6,7,8,9}}};
        auto ret = cli->Join(group_servers);
        verify(ret == KV_SUCCESS);
        checkShardBasic(group_servers);
        map<uint32_t, vector<uint32_t>> group_servers_2 = {{2,{10,11,12,13,14}}};
        auto ret2 = cli->Join(group_servers_2);
        verify(ret2 == KV_SUCCESS);

        map<uint32_t, vector<uint32_t>> group_servers_3 = {{1,{5,6,7,8,9}},{2,{10,11,12,13,14}}};
        checkShardBasic(group_servers_3);
        checkShardBasic(group_servers_3); // check it twice
        auto ret3 = cli->Leave({1});
        checkShardBasic(group_servers_2);

        break; 
        // if (ret == KV_TIMEOUT) {
        //   verify(unreliable);
        //   continue;
        // } 
        // string v2 = to_string(RandomGenerator::rand(0, 10000));
        // ret = cli->Append(k, v2);
        // if (ret == KV_TIMEOUT) {
        //   verify(unreliable);
        //   continue;
        // } 
        // string v3; 
        // ret = cli->Get(k, &v3);
        // if (ret == KV_TIMEOUT) {
        //   verify(unreliable);
        //   continue;
        // }
        // verify(v1+v2 == v3);
        // break;
      } while (true);
      done.fetch_add(1);  
      // int ret = cli->Append("test", "hello world");
      // verify(ret == KV_SUCCESS); 
      // string v;
      // ret = cli->Get("test", &v);
      // verify(ret == KV_SUCCESS); 
      // verify(v == "hello world"); 
    });
  }
  auto t1 = Time::now();
  while (true) {
    std::this_thread::sleep_for(100ms);
    if (done.load() == n_cli) {
      break;
    }  
    if (Time::now() - t1 > 100000000) {
      Log_fatal("run out of time to finish");
    }
  }
  verify(done.load() == n_cli);
  Passed2();
}


int RaftLabTest::testKvSnapInstallRPC() {
  Init2(9, "Basic kv snap : InstallSnapshot RPC");
    auto cli = config_->GetKvServer(0)->CreateClient();
    auto leader = config_->OneLeader();
    cli->leader_idx_ = leader;
    int maxraftstate = 1000;
    do {
        auto ret = cli->Put("a", "A");
        if (ret == KV_TIMEOUT) {
            verify(false);
            continue;
        } 
        string v1; 
        ret = cli->Get("a", &v1);
        if (ret == KV_TIMEOUT) {
            verify(false);
            continue;
        }
        verify(v1 == "A");
        break;
    } while (true);
    
    config_->Disconnect((leader+1)%5);
    config_->Disconnect((leader+2)%5);
    for (int i = 0; i < 200; i++) {
        string key = to_string(i);
        string val = to_string(RandomGenerator::rand(0, 10000));
        auto ret = cli->Put(key, val);
        if (ret == KV_TIMEOUT) {
            verify(false);
            continue;
        }
    }
    Coroutine::Sleep(ELECTIONTIMEOUT);
    auto ret = cli->Put("b", "B");
    if (ret == KV_TIMEOUT) {
        verify(false);
    }

    int currentLogSize = config_->GetLogSize();
    if (currentLogSize > (maxraftstate * 8)) {
        verify(0);
    }

    config_->Disconnect((leader+3)%5);
    config_->Disconnect((leader+4)%5);
    config_->Reconnect((leader+1)%5);
    config_->Reconnect((leader+2)%5);

    ret = cli->Put("c", "C");
    if (ret == KV_TIMEOUT) {
        verify(false);
    }
    ret = cli->Put("d", "D");
    if (ret == KV_TIMEOUT) {
        verify(false);
    }

    string v4; 
    ret = cli->Get("a", &v4);
    if (ret == KV_TIMEOUT) {
        verify(false);
    }
    verify("A" == v4);
    string v5;
    ret = cli->Get("b", &v5);
    if (ret == KV_TIMEOUT) {
        verify(false);
    }
    verify(v5 == "B");

    config_->Reconnect((leader+3)%5);
    config_->Reconnect((leader+4)%5);
    ret = cli->Put("e", "E");
    if (ret == KV_TIMEOUT) {
        verify(false);
    }

    string v8; 
    ret = cli->Get("a", &v8);
    if (ret == KV_TIMEOUT) {
        verify(false);
    }
    verify("A" == v8);
    string v9;
    ret = cli->Get("e", &v9);
    if (ret == KV_TIMEOUT) {
        verify(false);
    }
    verify(v9 == "E");
    string v10;
    ret = cli->Get("1", &v10);
    if (ret == KV_TIMEOUT) {
        verify(false);
    } 
    Passed2();
}

int RaftLabTest::testKvSnapSize() {
  Init2(10, "Basic kv snap : Check Snapshot size is reasonable");
    auto cli = config_->GetKvServer(0)->CreateClient();
    auto leader = config_->OneLeader();
    cli->leader_idx_ = leader;
    int maxraftstate = 1000;
    
    for (int i = 0; i < 400; i++) {
        string key = to_string(i);
        string val = to_string(RandomGenerator::rand(0, 10000));
        auto ret = cli->Put(key, val);
        if (ret == KV_TIMEOUT) {
            verify(false);
            continue;
        }
        string val2;
        ret = cli->Get(key, &val2);
        if (ret == KV_TIMEOUT) {
            verify(false);
            continue;
        }
        verify(val2 == val);
    }  
    int currentLogSize = config_->GetLogSize();
    if (currentLogSize > (maxraftstate * 8)) {
        verify(0);
    }
    int currentSnapSize = config_->GetSnapshotSize();
    if (currentSnapSize > (maxraftstate * 8)) {
        verify(0);
    }

    Passed2();
}

int RaftLabTest::testKvBasic() {
  Init2(1, "Basic kv operations");
  GenericKvTest(1, false, 3000000, 0,1000);
  Passed2();
}

int RaftLabTest::testKvConcurrent() {
  Init2(2, "Concurrent kv operations");
  GenericKvTest(5, false, 3000000, 0,1000);
  Passed2();
}

int RaftLabTest::testKvMajority() {
  Init2(3, "Progress with a majority");
  auto leader = config_->OneLeader();
  config_->Disconnect((leader+1)%5);
  config_->Disconnect((leader+2)%5);
  GenericKvTest(1, false, 3000000, 0,1000);
  config_->Reconnect((leader+1)%5);
  config_->Reconnect((leader+2)%5);
  Passed2();
}

int RaftLabTest::testKvMajorityConcurrent() {
  Init2(6, "Progress with a majority and concurrent requests");
  auto leader = config_->OneLeader();
  config_->Disconnect((leader+1)%5);
  config_->Disconnect((leader+2)%5);
  GenericKvTest(5, false, 3000000, leader, 1000);
  config_->Reconnect((leader+1)%5);
  config_->Reconnect((leader+2)%5);
  Passed2();
}

int RaftLabTest::testKvMajorityConcurrentSameKey() {
  Init2(7, "Progress with a majority writing the same key");
  auto leader = config_->OneLeader();
  config_->Disconnect((leader+1)%5);
  config_->Disconnect((leader+2)%5);
  vector<thread*> threads;
  int n_cli = 5;
  threads.resize(n_cli, nullptr);
  atomic<int> done{0};
  string k = "test_key_7";
  for (int i = 0; i < n_cli; i++) {
    threads[i] = new thread([this, i, &done, leader, k](){
      verify(config_->GetKvServer(0) != nullptr);
      auto cli = config_->GetKvServer(0)->CreateClient();
      cli->leader_idx_ = leader;
      verify(cli->Append(k, "1") == KV_SUCCESS);
      done.fetch_add(1);  
    });
  }
  auto t1 = Time::now();
  while (true) {
    std::this_thread::sleep_for(100ms);
    if (done.load() == n_cli) {
      break;
    }  
    if (Time::now() - t1 > 10000000) {
      Log_fatal("run out of time to finish");
    }
  }
  verify(done.load() == n_cli);
  auto cli = config_->GetKvServer(0)->CreateClient();
  cli->leader_idx_ = leader;
  string s3;
  verify(cli->Get(k, &s3) == KV_SUCCESS);
  verify(s3=="11111");

  config_->Reconnect((leader+1)%5);
  config_->Reconnect((leader+2)%5);
  Passed2();
}

int RaftLabTest::testKvMajorityConcurrentLinear() {
  Init2(8, "Progress with a majority testing linearizability");
  auto leader = config_->OneLeader();
  config_->Disconnect((leader+1)%5);
  config_->Disconnect((leader+2)%5);
  vector<thread*> threads;
  int n_cli = 5;
  threads.resize(n_cli, nullptr);
  atomic<int> done{0};
  string k = "test_key_8";
  for (int i = 0; i < n_cli; i++) {
    verify(config_->GetKvServer(0) != nullptr);
    auto cli = config_->GetKvServer(0)->CreateClient();
    cli->leader_idx_ = leader;
    verify(cli->Append(k, to_string(i)) == KV_SUCCESS);
    done.fetch_add(1);  
  }
  auto t1 = Time::now();
  while (true) {
    std::this_thread::sleep_for(100ms);
    if (done.load() == n_cli) {
      break;
    }  
    if (Time::now() - t1 > 10000000) {
      Log_fatal("run out of time to finish");
    }
  }
  verify(done.load() == n_cli);
  auto cli = config_->GetKvServer(0)->CreateClient();
  cli->leader_idx_ = leader;
  string s3;
  verify(cli->Get(k, &s3) == KV_SUCCESS);
  verify(s3=="01234");

  config_->Reconnect((leader+1)%5);
  config_->Reconnect((leader+2)%5);
  Passed2();
}

int RaftLabTest::testKvMinority() {
  Init2(4, "No progress with a minority");
  auto leader = config_->OneLeader();
  config_->Disconnect((leader+1)%5);
  config_->Disconnect((leader+2)%5);
  config_->Disconnect((leader+3)%5);
  auto cli = config_->GetKvServer(0)->CreateClient();
  int r = RandomGenerator::rand(0, 10000);
  string k = to_string(1);
  string v1 = to_string(r);
  auto ret = cli->Put(k, v1);
  verify (ret == KV_TIMEOUT);
  string v3;
  ret = cli->Get(k, &v3);
  verify (ret == KV_TIMEOUT);
  config_->Reconnect((leader+1)%5);
  config_->Reconnect((leader+2)%5);
  config_->Reconnect((leader+3)%5);
  Passed2();
}

int RaftLabTest::testKvReElection() {
  Init2(5, "Kv operations through re-election");
  // find current leader
  int leader = config_->OneLeader();
  AssertOneLeader(leader);

  auto cli = config_->GetKvServer(0)->CreateClient();
  int r = RandomGenerator::rand(0, 10000);
  string k = "reelection";
  string v1 = to_string(r);
  cli->leader_idx_ = leader;
  verify(cli->Put(k, v1) == KV_SUCCESS);
  // disconnect leader - make sure a new one is elected
  Log_debug("disconnecting old leader");
  config_->Disconnect(leader);
  int oldLeader = leader;
  Coroutine::Sleep(ELECTIONTIMEOUT);
  leader = config_->OneLeader();
  AssertOneLeader(leader);
  AssertReElection(leader, oldLeader);
  // try write to the new leader
  uint64_t leaderLocId = config_->getLocId(leader);
  Log_debug("leader loc id is %d, replica index is %d",leaderLocId,leader);
  cli->leader_idx_ = leaderLocId;
  string v2 = to_string(RandomGenerator::rand(0, 10000));
  auto ret = cli->Append(k, v2);
  // Log_info("ret %d", ret);
  verify(ret == KV_SUCCESS);
  string v3; 
  verify(cli->Get(k, &v3) == KV_SUCCESS);
  verify(v1+v2 == v3);
  // reconnect old leader - should not disturb new leader
  config_->Reconnect(oldLeader);
  Log_debug("reconnecting old leader");
  Coroutine::Sleep(ELECTIONTIMEOUT);
  AssertOneLeader(config_->OneLeader(leader));
  // try write another value
  string v4 = to_string(RandomGenerator::rand(0, 10000));
  verify(cli->Append(k, v4)==KV_SUCCESS);
  string v5; 
  verify(cli->Get(k, &v5)==KV_SUCCESS);
  verify(v1+v2+v4 == v5);
  Passed2();
}

int RaftLabTest::testKvUnreliable(void) {
  Init2(9, "Progress in unreliable net");
  GenericKvTest(1, true, 300*1000000ull, 0,1000);
  Passed2();
}

int RaftLabTest::testInitialElection(void) {
  Init2(1, "Initial election");
  // Initial election: is there one leader?
  // Initial election does not need extra time 
  // Coroutine::Sleep(ELECTIONTIMEOUT);
  int leader = config_->OneLeader();
  AssertOneLeader(leader);
  // calculate RPC count for initial election for later use
  init_rpcs_ = 0;
  for (int i = 0; i < NSERVERS; i++) {
    init_rpcs_ += config_->RpcCount(i);
  }
  // Does everyone agree on the term number?
  uint64_t term = config_->OneTerm();
  Assert2(term != -1, "servers disagree on term number");
  // Sleep for a while
  Coroutine::Sleep(ELECTIONTIMEOUT);
  // Does the term stay the same after a while if there's no failures?
  Assert2(config_->OneTerm() == term, "unexpected term change");
  // Is the same server still the only leader?
  AssertOneLeader(config_->OneLeader(leader));
  Passed2();
}

int RaftLabTest::testReElection(void) {
  Init2(2, "Re-election after network failure");
  // find current leader
  int leader = config_->OneLeader();
  AssertOneLeader(leader);
  // disconnect leader - make sure a new one is elected
  Log_debug("disconnecting old leader");
  config_->Disconnect(leader);
  int oldLeader = leader;
  Coroutine::Sleep(ELECTIONTIMEOUT);
  leader = config_->OneLeader();
  AssertOneLeader(leader);
  AssertReElection(leader, oldLeader);
  // reconnect old leader - should not disturb new leader
  config_->Reconnect(oldLeader);
  Log_debug("reconnecting old leader");
  Coroutine::Sleep(ELECTIONTIMEOUT);
  AssertOneLeader(config_->OneLeader(leader));
  // no quorum -> no leader
  Log_debug("disconnecting more servers");
  config_->Disconnect((leader + 1) % NSERVERS);
  config_->Disconnect((leader + 2) % NSERVERS);
  config_->Disconnect(leader);
  Assert(config_->NoLeader());
  // quorum restored
  Log_debug("reconnecting a server to enable majority");
  config_->Reconnect((leader + 2) % NSERVERS);
  Coroutine::Sleep(ELECTIONTIMEOUT);
  AssertOneLeader(config_->OneLeader());
  // rejoin all servers
  Log_debug("rejoining all servers");
  config_->Reconnect((leader + 1) % NSERVERS);
  config_->Reconnect(leader);
  Coroutine::Sleep(ELECTIONTIMEOUT);
  AssertOneLeader(config_->OneLeader());
  Passed2();
}

int RaftLabTest::testBasicAgree(void) {
  Init2(3, "Basic agreement");
  for (int i = 1; i <= 3; i++) {
    // make sure no commits exist before any agreements are started
    AssertNoneCommitted(index_);
    // complete 1 agreement and make sure its index is as expected
    DoAgreeAndAssertIndex((int)(index_ + 300), NSERVERS, index_++);
  }
  Passed2();
}

int RaftLabTest::testFailAgree(void) {
  Init2(4, "Agreement despite follower disconnection");
  // disconnect 2 followers
  auto leader = config_->OneLeader();
  AssertOneLeader(leader);
  Log_debug("disconnecting two followers leader");
  config_->Disconnect((leader + 1) % NSERVERS);
  config_->Disconnect((leader + 2) % NSERVERS);
  // Agreement despite 2 disconnected servers
  Log_debug("try commit a few commands after disconnect");
  DoAgreeAndAssertIndex(401, NSERVERS - 2, index_++);
  DoAgreeAndAssertIndex(402, NSERVERS - 2, index_++);
  Coroutine::Sleep(ELECTIONTIMEOUT);
  DoAgreeAndAssertIndex(403, NSERVERS - 2, index_++);
  DoAgreeAndAssertIndex(404, NSERVERS - 2, index_++);
  // reconnect followers
  Log_debug("reconnect servers");
  config_->Reconnect((leader + 1) % NSERVERS);
  config_->Reconnect((leader + 2) % NSERVERS);
  Coroutine::Sleep(ELECTIONTIMEOUT);
  Log_debug("try commit a few commands after reconnect");
  DoAgreeAndAssertWaitSuccess(405, NSERVERS);
  DoAgreeAndAssertWaitSuccess(406, NSERVERS);
  Passed2();
}

int RaftLabTest::testFailNoAgree(void) {
  Init2(5, "No agreement if too many followers disconnect");
  // disconnect 3 followers
  auto leader = config_->OneLeader();
  AssertOneLeader(leader);
  config_->Disconnect((leader + 1) % NSERVERS);
  config_->Disconnect((leader + 2) % NSERVERS);
  config_->Disconnect((leader + 3) % NSERVERS);
  // attempt to do an agreement
  uint64_t index, term;
  AssertStartOk(config_->Start(leader, 501, &index, &term));
  Assert2(index == index_++ && term > 0,
          "Start() returned unexpected index (%ld, expected %ld) and/or term (%ld, expected >0)",
          index, index_-1, term);
  Coroutine::Sleep(ELECTIONTIMEOUT);
  AssertNoneCommitted(index);
  // reconnect followers
  config_->Reconnect((leader + 1) % NSERVERS);
  config_->Reconnect((leader + 2) % NSERVERS);
  config_->Reconnect((leader + 3) % NSERVERS);
  // do agreement in restored quorum
  Coroutine::Sleep(ELECTIONTIMEOUT);
  DoAgreeAndAssertWaitSuccess(502, NSERVERS);
  Passed2();
}

int RaftLabTest::testRejoin(void) {
  Init2(6, "Rejoin of disconnected leader");
  DoAgreeAndAssertIndex(601, NSERVERS, index_++);
  // disconnect leader
  auto leader1 = config_->OneLeader();
  AssertOneLeader(leader1);
  config_->Disconnect(leader1);
  Coroutine::Sleep(ELECTIONTIMEOUT);
  // Make old leader try to agree on some entries (these should not commit)
  uint64_t index, term;
  AssertStartOk(config_->Start(leader1, 602, &index, &term));
  AssertStartOk(config_->Start(leader1, 603, &index, &term));
  AssertStartOk(config_->Start(leader1, 604, &index, &term));
  // New leader commits, successfully
  DoAgreeAndAssertWaitSuccess(605, NSERVERS - 1);
  DoAgreeAndAssertWaitSuccess(606, NSERVERS - 1);
  // Disconnect new leader
  auto leader2 = config_->OneLeader();
  AssertOneLeader(leader2);
  AssertReElection(leader2, leader1);
  config_->Disconnect(leader2);
  // reconnect old leader
  config_->Reconnect(leader1);
  // wait for new election
  Coroutine::Sleep(ELECTIONTIMEOUT);
  auto leader3 = config_->OneLeader();
  AssertOneLeader(leader3);
  AssertReElection(leader3, leader2);
  // More commits
  DoAgreeAndAssertWaitSuccess(607, NSERVERS - 1);
  DoAgreeAndAssertWaitSuccess(608, NSERVERS - 1);
  // Reconnect all
  config_->Reconnect(leader2);
  DoAgreeAndAssertWaitSuccess(609, NSERVERS);
  Passed2();
}

class CSArgs {
 public:
  std::vector<uint64_t> *indices;
  std::mutex *mtx;
  int i;
  int leader;
  uint64_t term;
  RaftTestConfig *config;
};

static void *doConcurrentStarts(void *args) {
  CSArgs *csargs = (CSArgs *)args;
  uint64_t idx, tm;
  auto ok = csargs->config->Start(csargs->leader, 701 + csargs->i, &idx, &tm);
  if (!ok || tm != csargs->term) {
    return nullptr;
  }
  {
    std::lock_guard<std::mutex> lock(*(csargs->mtx));
    csargs->indices->push_back(idx);
  }
  return nullptr;
}

int RaftLabTest::testConcurrentStarts(void) {
  Init2(7, "Concurrently started agreements");
  int nconcurrent = 5;
  bool success = false;
  for (int again = 0; again < 5; again++) {
    if (again > 0) {
      wait(3000000);
    }
    auto leader = config_->OneLeader();
    AssertOneLeader(leader);
    uint64_t index, term;
    auto ok = config_->Start(leader, 701, &index, &term);
    if (!ok) {
      continue; // retry (up to 5 times)
    }
    // create 5 threads that each Start a command to leader
    std::vector<uint64_t> indices{};
    std::vector<int> cmds{};
    std::mutex mtx{};
    pthread_t threads[nconcurrent];
    for (int i = 0; i < nconcurrent; i++) {
      CSArgs *args = new CSArgs{};
      args->indices = &indices;
      args->mtx = &mtx;
      args->i = i;
      args->leader = leader;
      args->term = term;
      args->config = config_;
      verify(pthread_create(&threads[i], nullptr, doConcurrentStarts, (void*)args) == 0);
    }
    // join all threads
    for (int i = 0; i < nconcurrent; i++) {
      verify(pthread_join(threads[i], nullptr) == 0);
    }
    if (config_->TermMovedOn(term)) {
      goto skip; // if leader's term is expiring, start over
    }
    // wait for all indices to commit
    for (auto index : indices) {
      int cmd = config_->Wait(index, NSERVERS, term);
      if (cmd < 0) {
        AssertWaitNoError(cmd, index);
        goto skip; // on timeout and term changes, try again
      }
      cmds.push_back(cmd);
    }
    // make sure all the commits are there with the correct values
    for (int i = 0; i < nconcurrent; i++) {
      auto val = 701 + i;
      int j;
      for (j = 0; j < cmds.size(); j++) {
        if (cmds[j] == val) {
          break;
        }
      }
      Assert2(j < cmds.size(), "cmd %d missing", val);
    }
    success = true;
    break;
    skip: ;
  }
  Assert2(success, "too many term changes and/or delayed responses");
  index_ += nconcurrent + 1;
  Passed2();
}

int RaftLabTest::testBackup(void) {
  Init2(8, "Leader backs up quickly over incorrect follower logs");
  // disconnect 3 servers that are not the leader
  int leader1 = config_->OneLeader();
  AssertOneLeader(leader1);
  Log_debug("disconnect 3 followers");
  config_->Disconnect((leader1 + 2) % NSERVERS);
  config_->Disconnect((leader1 + 3) % NSERVERS);
  config_->Disconnect((leader1 + 4) % NSERVERS);
  // Start() a bunch of commands that won't be committed
  uint64_t index, term;
  for (int i = 0; i < 50; i++) {
    AssertStartOk(config_->Start(leader1, 800 + i, &index, &term));
  }
  Coroutine::Sleep(ELECTIONTIMEOUT);
  // disconnect the leader and its 1 follower, then reconnect the 3 servers
  Log_debug("disconnect the leader and its 1 follower, reconnect the 3 followers");
  config_->Disconnect((leader1 + 1) % NSERVERS);
  config_->Disconnect(leader1);
  config_->Reconnect((leader1 + 2) % NSERVERS);
  config_->Reconnect((leader1 + 3) % NSERVERS);
  config_->Reconnect((leader1 + 4) % NSERVERS);
  // do a bunch of agreements among the new quorum
  Coroutine::Sleep(ELECTIONTIMEOUT);
  Log_debug("try to commit a lot of commands");
  for (int i = 1; i <= 50; i++) {
    DoAgreeAndAssertIndex(800 + i, NSERVERS - 2, index_++);
  }
  // reconnect the old leader and its follower
  Log_debug("reconnect the old leader and the follower");
  config_->Reconnect((leader1 + 1) % NSERVERS);
  config_->Reconnect(leader1);
  Coroutine::Sleep(ELECTIONTIMEOUT);
  // do an agreement all together to check the old leader's incorrect
  // entries are replaced in a timely manner
  int leader2 = config_->OneLeader();
  AssertOneLeader(leader2);
  AssertStartOk(config_->Start(leader2, 851, &index, &term));
  index_++;
  // 10 seconds should be enough to back up 50 incorrect logs
  Coroutine::Sleep(2*ELECTIONTIMEOUT);
  Log_debug("check if the old leader has enough committed");
  AssertNCommitted(index, NSERVERS);
  Passed2();
}

int RaftLabTest::testCount(void) {
  Init2(9, "RPC counts aren't too high");

  // reset RPC counts before starting
  for (int i = 0; i < NSERVERS; i++) {
    config_->RpcCount(i, true);
  }

  auto rpcs = [this]() {
    uint64_t total = 0;
    for (int i = 0; i < NSERVERS; i++) {
      total += config_->RpcCount(i);
    }
    return total;
  };

  // initial election RPC count
  Assert2(init_rpcs_ > 1 && init_rpcs_ <= 30,
          "too many or too few RPCs (%ld) to elect initial leader",
          init_rpcs_);

  // agreement RPC count
  int iters = 10;
  uint64_t total = -1;
  bool success = false;
  for (int again = 0; again < 5; again++) {
    if (again > 0) {
      wait(3000000);
    }
    auto leader = config_->OneLeader();
    AssertOneLeader(leader);
    rpcs();
    uint64_t index, term, startindex, startterm;
    auto ok = config_->Start(leader, 900, &startindex, &startterm);
    if (!ok) {
      // leader moved on quickly: start over
      continue;
    }
    for (int i = 1; i <= iters; i++) {
      ok = config_->Start(leader, 900 + i, &index, &term);
      if (!ok || term != startterm) {
        // no longer the leader and/or term changed: start over
        goto loop;
      }
      Assert2(index == (startindex + i), "Start() failed");
    }
    for (int i = 1; i <= iters; i++) {
      auto r = config_->Wait(startindex + i, NSERVERS, startterm);
      AssertWaitNoError(r, startindex + i);
      if (r < 0) {
        // timeout or term change: start over
        goto loop;
      }
      Assert2(r == (900 + i), "wrong value %d committed for index %ld: expected %d", r, startindex + i, 900 + i);
    }
    if (config_->TermMovedOn(startterm)) {
      // term changed -- can't expect low RPC counts: start over
      continue;
    }
    total = rpcs();
    Assert2(total <= COMMITRPCS(iters),
            "too many RPCs (%ld) for %d entries",
            total, iters);
    success = true;
    break;
    loop: ;
  }
  Assert2(success, "term changed too often");

  // idle RPC count
  wait(1000000);
  total = rpcs();
  Assert2(total <= 60,
          "too many RPCs (%ld) for 1 second of idleness",
          total);
  Passed2();
}

class CAArgs {
 public:
  int iter;
  int i;
  std::mutex *mtx;
  std::vector<uint64_t> *retvals;
  RaftTestConfig *config;
};

static void *doConcurrentAgreement(void *args) {
  CAArgs *caargs = (CAArgs *)args;
  uint64_t retval = caargs->config->DoAgreement(1000 + caargs->iter, 1, true);
  if (retval == 0) {
    std::lock_guard<std::mutex> lock(*(caargs->mtx));
    caargs->retvals->push_back(retval);
  }
  return nullptr;
}

int RaftLabTest::testUnreliableAgree(void) {
  Init2(10, "Unreliable agreement (takes a few minutes)");
  config_->SetUnreliable(true);
  std::vector<pthread_t> threads{};
  std::vector<uint64_t> retvals{};
  std::mutex mtx{};
  for (int iter = 1; iter < 50; iter++) {
    for (int i = 0; i < 4; i++) {
      CAArgs *args = new CAArgs{};
      args->iter = iter;
      args->i = i;
      args->mtx = &mtx;
      args->retvals = &retvals;
      args->config = config_;
      pthread_t thread;
      verify(pthread_create(&thread,
                            nullptr,
                            doConcurrentAgreement,
                            (void*)args) == 0);
      threads.push_back(thread);
    }
    if (retvals.size() > 0)
      break;
    if (config_->DoAgreement(1000 + iter, 1, true) == 0) {
      std::lock_guard<std::mutex> lock(mtx);
      retvals.push_back(0);
      break;
    }
  }
  config_->SetUnreliable(false);
  // join all threads
  for (auto thread : threads) {
    verify(pthread_join(thread, nullptr) == 0);
  }
  Assert2(retvals.size() == 0, "Failed to reach agreement");
  index_ += 50 * 5;
  DoAgreeAndAssertWaitSuccess(1060, NSERVERS);
  Passed2();
}

int RaftLabTest::testFigure8(void) {
  Init2(11, "Figure 8");
  bool success = false;
  // Leader should not determine commitment using log entries from previous terms
  for (int again = 0; again < 10; again++) {
    // find out initial leader (S1) and term
    auto leader1 = config_->OneLeader();
    AssertOneLeader(leader1);
    uint64_t index1, term1, index2, term2;
    auto ok = config_->Start(leader1, 1100, &index1, &term1);
    if (!ok) {
      continue; // term moved on too quickly: start over
    }
    auto r = config_->Wait(index1, NSERVERS, term1);
    AssertWaitNoError(r, index1);
    AssertWaitNoTimeout(r, index1, NSERVERS);
    index_ = index1;
    // Start() a command (C1) and only let it get replicated to 1 follower (S2)
    config_->Disconnect((leader1 + 1) % NSERVERS);
    config_->Disconnect((leader1 + 2) % NSERVERS);
    config_->Disconnect((leader1 + 3) % NSERVERS);
    ok = config_->Start(leader1, 1101, &index1, &term1);
    if (!ok) {
      config_->Reconnect((leader1 + 1) % NSERVERS);
      config_->Reconnect((leader1 + 2) % NSERVERS);
      config_->Reconnect((leader1 + 3) % NSERVERS);
      continue;
    }
    Coroutine::Sleep(ELECTIONTIMEOUT);
    // C1 is at index i1 for S1 and S2
    AssertNoneCommitted(index1);
    // Elect new leader (S3) among other 3 servers
    config_->Disconnect((leader1 + 4) % NSERVERS);
    config_->Disconnect(leader1);
    config_->Reconnect((leader1 + 1) % NSERVERS);
    config_->Reconnect((leader1 + 2) % NSERVERS);
    config_->Reconnect((leader1 + 3) % NSERVERS);
    auto leader2 = config_->OneLeader();
    AssertOneLeader(leader2);
    // let old leader (S1) and follower (S2) become a follower in the new term
    config_->Reconnect((leader1 + 4) % NSERVERS);
    config_->Reconnect(leader1);
    Coroutine::Sleep(ELECTIONTIMEOUT);
    AssertOneLeader(config_->OneLeader(leader2));
    Log_debug("disconnect all followers and Start() a cmd (C2) to isolated new leader");
    for (int i = 0; i < NSERVERS; i++) {
      if (i != leader2) {
        config_->Disconnect(i);
      }
    }
    ok = config_->Start(leader2, 1102, &index2, &term2);
    if (!ok) {
      for (int i = 1; i < 5; i++) {
        config_->Reconnect((leader2 + i) % NSERVERS);
      }
      continue;
    }
    // C2 is at index i1 for S3, C1 still at index i1 for S1 & S2
    Assert2(index2 == index1, "Start() returned index %ld (%ld expected)", index2, index1);
    Assert2(term2 > term1, "Start() returned term %ld (%ld expected)", term2, term1);
    Coroutine::Sleep(ELECTIONTIMEOUT);
    AssertNoneCommitted(index1);
    // Let first leader (S1) or its initial follower (S2) become next leader
    config_->Disconnect(leader2);
    config_->Reconnect(leader1);
    verify((leader1 + 4) % NSERVERS != leader2);
    config_->Reconnect((leader1 + 4) % NSERVERS);
    if (leader2 == leader1 + 1)
      config_->Reconnect((leader1 + 2) % NSERVERS);
    else
      config_->Reconnect((leader1 + 1) % NSERVERS);
    auto leader3 = config_->OneLeader();
    AssertOneLeader(leader3);
    if (leader3 != leader1 && leader3 != ((leader1 + 4) % NSERVERS)) {
      continue; // failed this step with a 1/3 chance. just start over until success.
    }
    // give leader3 more than enough time to replicate index1 to a third server
    Coroutine::Sleep(ELECTIONTIMEOUT);
    // Make sure initial Start() value isn't getting committed at this point
    AssertNoneCommitted(index1);
    // Commit a new index in the current term
    Assert2(config_->DoAgreement(1103, NSERVERS - 2, false) > index1,
            "failed to reach agreement");
    // Make sure that C1 is committed for index i1 now
    AssertNCommitted(index1, NSERVERS - 2);
    Assert2(config_->ServerCommitted(leader3, index1, 1101),
            "value 1101 is not committed at index %ld when it should be", index1);
    success = true;
    // Reconnect all servers
    config_->Reconnect((leader1 + 3) % NSERVERS);
    if (leader2 == leader1 + 1)
      config_->Reconnect((leader1 + 1) % NSERVERS);
    else
      config_->Reconnect((leader1 + 2) % NSERVERS);
    break;
  }
  Assert2(success, "Failed to test figure 8");
  Passed2();
}

int RaftLabTest::testBasicPersistence(void) {
  Init2(12, "Basic persistence");
  int leader1 = config_->OneLeader();
  AssertOneLeader(leader1);
  DoAgreeAndAssertIndex(1201, NSERVERS, index_++);
  
  Log_debug("restart all servers");
  for (int i=0; i<NSERVERS; i++) {
    config_->Restart(i);
  }
  Coroutine::Sleep(ELECTIONTIMEOUT);
  DoAgreeAndAssertIndex(1202, NSERVERS, index_++);

  Log_debug("restart leader");
  int leader2 = config_->OneLeader();
  AssertOneLeader(leader2);
  config_->Restart(leader2);
  Coroutine::Sleep(ELECTIONTIMEOUT);
  DoAgreeAndAssertIndex(1203, NSERVERS, index_++);

  Log_debug("disconnect and restart leader");
  int leader3 = config_->OneLeader();
  AssertOneLeader(leader3);
  config_->Disconnect(leader3);
  Coroutine::Sleep(ELECTIONTIMEOUT);
  DoAgreeAndAssertIndex(1204, NSERVERS-1, index_++);
  config_->Reconnect(leader3);
  config_->Restart(leader3);

  Log_debug("disconnect and restart follower");
  int leader4 = config_->OneLeader();
  AssertOneLeader(leader4);
  config_->Disconnect((leader4 + 1) % NSERVERS);
  DoAgreeAndAssertIndex(1205, NSERVERS-1, index_++);
  config_->Reconnect((leader4 + 1) % NSERVERS);
  config_->Restart((leader4 + 1) % NSERVERS);

  DoAgreeAndAssertIndex(1206, NSERVERS, index_++);
  Passed2();
}

int RaftLabTest::testMorePersistence1(void) {
  Init2(13, "More persistence - part 1");
  for (int iter = 0; iter < 5; iter++){
    int leader1 = config_->OneLeader();
    AssertOneLeader(leader1);
    DoAgreeAndAssertIndex(1301 + (10*iter), NSERVERS, index_++);

    config_->Disconnect((leader1 + 1) % NSERVERS);
    config_->Disconnect((leader1 + 2) % NSERVERS);
    DoAgreeAndAssertIndex(1302 + (10*iter), NSERVERS-2, index_++);

    config_->Disconnect(leader1 % NSERVERS);
    config_->Disconnect((leader1 + 3) % NSERVERS);
    config_->Disconnect((leader1 + 4) % NSERVERS);

    config_->Reconnect((leader1 + 1) % NSERVERS);
    config_->Reconnect((leader1 + 2) % NSERVERS);
    config_->Restart((leader1 + 1) % NSERVERS);
    config_->Restart((leader1 + 2) % NSERVERS);

    config_->Reconnect((leader1 + 3) % NSERVERS);
    config_->Restart((leader1 + 3) % NSERVERS);
    Coroutine::Sleep(ELECTIONTIMEOUT);
    DoAgreeAndAssertIndex(1303 + (10*iter), NSERVERS-2, index_++);

    config_->Reconnect(leader1 % NSERVERS);
    config_->Restart(leader1 % NSERVERS);
    config_->Reconnect((leader1 + 4) % NSERVERS);
    config_->Restart((leader1 + 4) % NSERVERS);
  }
  DoAgreeAndAssertIndex(1360, NSERVERS, index_++);
  Passed2();
}

int RaftLabTest::testMorePersistence2(void) {
  Init2(14, "More persistence - part 2");
  for (int iter = 0; iter < 5; iter++){
    int leader1 = config_->OneLeader();
    AssertOneLeader(leader1);
    DoAgreeAndAssertIndex(1401 + (10 * iter), NSERVERS, index_++);

    config_->Disconnect((leader1 + 1) % NSERVERS);
    config_->Disconnect((leader1 + 2) % NSERVERS);
    DoAgreeAndAssertIndex(1402 + (10 * iter), NSERVERS-2, index_++);

    config_->Disconnect(leader1 % NSERVERS);
    config_->Disconnect((leader1 + 3) % NSERVERS);
    config_->Disconnect((leader1 + 4) % NSERVERS);

    config_->Reconnect((leader1 + 1) % NSERVERS);
    config_->Restart((leader1 + 1) % NSERVERS);
    config_->Reconnect((leader1 + 2) % NSERVERS);
    config_->Restart((leader1 + 2) % NSERVERS);

    config_->Reconnect(leader1 % NSERVERS);
    config_->Restart(leader1 % NSERVERS);
    Coroutine::Sleep(ELECTIONTIMEOUT);
    DoAgreeAndAssertIndex(1403 + (10 * iter), NSERVERS-2, index_++);

    config_->Reconnect((leader1 + 3) % NSERVERS);
    config_->Restart((leader1 + 3) % NSERVERS);
    config_->Reconnect((leader1 + 4) % NSERVERS);
    config_->Restart((leader1 + 4) % NSERVERS);
  }
  DoAgreeAndAssertIndex(1460, NSERVERS, index_++);
  Passed2();
}

void RaftLabTest::wait(uint64_t microseconds) {
  Reactor::CreateSpEvent<TimeoutEvent>(microseconds)->Wait();
}

#endif

}
