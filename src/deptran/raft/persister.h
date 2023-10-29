#pragma once

#include "../__dep__.h"
#include "../constants.h"
#include "../deptran/marshallable.h"

namespace janus {

class Persister {
 private:
  int raftstate_kind_;
  int snapshot_kind_;
  size_t raftstate_size_ = -1;
  size_t snapshot_size_ = -1;
  rrr::Marshal serialized_raftstate_;
  rrr::Marshal serialized_snapshot_;
  std::mutex mtx_{};
  
 public:
  Persister();
  ~Persister();

  void SaveRaftState(shared_ptr<Marshallable>& curr_raftstate);
  shared_ptr<Marshallable> ReadRaftState();
  size_t RaftStateSize();
  void SaveSnapshot(shared_ptr<Marshallable>& curr_snapshot);
  shared_ptr<Marshallable> ReadSnapshot();
  size_t SnapshotSize();

};

} // namespace janus
