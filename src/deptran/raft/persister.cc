#include "persister.h"

namespace janus {

Persister::Persister() {}

Persister::~Persister() {}

void Persister::SaveRaftState(shared_ptr<Marshallable>& curr_raftstate) {
  std::lock_guard<std::mutex> lock(mtx_);
  raftstate_kind_ = curr_raftstate->kind_;
  if (raftstate_size_ != -1) {
    auto initializer = MarshallDeputy::GetInitializer(raftstate_kind_)();
    shared_ptr<Marshallable> old_raftstate(initializer);
    old_raftstate->FromMarshal(serialized_raftstate_);
  }
  curr_raftstate->ToMarshal(serialized_raftstate_);
  raftstate_size_ = serialized_raftstate_.content_size();
}

shared_ptr<Marshallable> Persister::ReadRaftState() {
  std::lock_guard<std::mutex> lock(mtx_);
  auto initializer = MarshallDeputy::GetInitializer(raftstate_kind_)();
  shared_ptr<Marshallable> raftstate(initializer);
  raftstate->FromMarshal(serialized_raftstate_);
  raftstate->ToMarshal(serialized_raftstate_);
  return raftstate;
}

size_t Persister::RaftStateSize() {
  std::lock_guard<std::mutex> lock(mtx_);
  return raftstate_size_;
}

void Persister::SaveSnapshot(shared_ptr<Marshallable>& curr_snapshot) {
  std::lock_guard<std::mutex> lock(mtx_);
  snapshot_kind_ = curr_snapshot->kind_;
  if (snapshot_size_ != -1) {
    auto initializer = MarshallDeputy::GetInitializer(snapshot_kind_)();
    shared_ptr<Marshallable> old_snapshot(initializer);
    old_snapshot->FromMarshal(serialized_snapshot_);
  }
  curr_snapshot->ToMarshal(serialized_snapshot_);
  snapshot_size_ = serialized_snapshot_.content_size();
}

shared_ptr<Marshallable> Persister::ReadSnapshot() {
  std::lock_guard<std::mutex> lock(mtx_);
  auto initializer = MarshallDeputy::GetInitializer(snapshot_kind_)();
  shared_ptr<Marshallable> snapshot(initializer);
  snapshot->FromMarshal(serialized_snapshot_);
  snapshot->ToMarshal(serialized_snapshot_);
  return snapshot;
}

size_t Persister::SnapshotSize() {
  std::lock_guard<std::mutex> lock(mtx_);
  return snapshot_size_;
}

} // namespace janus
