// server_rust.cc - Rust implementation wrapper
// Forward all RaftServer methods to Rust via FFI

#include "server.h"
#include "rust_ffi_wrapper.h"

namespace janus {

RaftServer::RaftServer(Frame *frame) {
    frame_ = frame;

    // Determine test mode based on RAFT_TEST_CORO flag (matches C++ server.cc lines 15-19)
    bool is_test_mode = false;
#ifdef RAFT_TEST_CORO
    is_test_mode = true;
#endif

    // Create Rust server immediately with nullptr for commo (will be updated in Setup)
    rust_server_ptr_ = rust_raft_server_new(
        frame_,
        nullptr,  // commo not available yet
        &raft_logs_,
        tx_sched_,
        &app_next_,
        is_test_mode
    );
}

RaftServer::~RaftServer() {
   // Log_info("[RUST] Destroying Rust RaftServer for site %d", site_id_);

    if (rust_server_ptr_) {
        rust_raft_server_delete(rust_server_ptr_);
        rust_server_ptr_ = nullptr;
    }

    // Log statistics
   // Log_info("RaftServer stats: n_vote=%d, n_prepare=%d, n_accept=%d, n_commit=%d",
     //        n_vote_, n_prepare_, n_accept_, n_commit_);
}

void RaftServer::Setup() {
    // Update commo pointer now that it's initialized
    rust_raft_server_set_commo(rust_server_ptr_, commo_);

    // Start election timer and heartbeat loop
    rust_raft_server_setup(rust_server_ptr_);

    // Signal that setup is complete (matches C++ server.cc:38)
    setup_done_ = true;
    Log_debug("RaftServer::Setup() completed for site %d (Rust implementation)", site_id_);
}

bool RaftServer::IsLeader() {
    return rust_raft_server_is_leader(rust_server_ptr_);
}

void RaftServer::setIsLeader(bool isLeader) {
    rust_raft_server_set_is_leader(rust_server_ptr_, isLeader);
}

void RaftServer::GetState(bool *is_leader, uint64_t *term) {
    rust_raft_server_get_state(rust_server_ptr_, is_leader, term);
}

void RaftServer::Disconnect(const bool disconnect) {
    rust_raft_server_disconnect(rust_server_ptr_, disconnect);
}

void RaftServer::Reconnect() {
    rust_raft_server_reconnect(rust_server_ptr_);
}

bool RaftServer::IsDisconnected() {
    return rust_raft_server_is_disconnected(rust_server_ptr_);
}

bool RaftServer::Start(shared_ptr<Marshallable> &cmd,
                       uint64_t *index,
                       uint64_t *term,
                       slotid_t slot_id,
                       ballot_t ballot) {
    // Convert shared_ptr to raw pointer for FFI
    auto* cmd_ptr = new shared_ptr<Marshallable>(cmd);
    bool result = rust_raft_server_start(
        rust_server_ptr_,
        cmd_ptr,
        index,
        term,
        slot_id,
        ballot
    );
    return result;
}

bool RaftServer::RequestVote() {
    return rust_raft_server_request_vote(rust_server_ptr_);
}

void RaftServer::OnRequestVote(const slotid_t& lst_log_idx,
                                const ballot_t& lst_log_term,
                                const siteid_t& can_id,
                                const ballot_t& can_term,
                                ballot_t *reply_term,
                                bool_t *vote_granted,
                                const function<void()> &cb) {
    // Create callback context
    auto* callback_ptr = new function<void()>(cb);
    auto callback_wrapper = [](void* ctx) {
        auto* cb_ptr = static_cast<function<void()>*>(ctx);
        (*cb_ptr)();
        delete cb_ptr;
    };

    rust_raft_server_on_request_vote(
        rust_server_ptr_,
        lst_log_idx,
        lst_log_term,
        can_id,
        can_term,
        reply_term,
        vote_granted,
        callback_wrapper,
        callback_ptr
    );
}

void RaftServer::OnAppendEntries(const slotid_t slot_id,
                                  const ballot_t ballot,
                                  const uint64_t leaderCurrentTerm,
                                  const uint64_t leaderPrevLogIndex,
                                  const uint64_t leaderPrevLogTerm,
                                  const uint64_t leaderCommitIndex,
                                  shared_ptr<Marshallable> &cmd,
                                  const uint64_t leaderNextLogTerm,
                                  uint64_t *followerAppendOK,
                                  uint64_t *followerCurrentTerm,
                                  uint64_t *followerLastLogIndex,
                                  const function<void()> &cb) {
    // Convert shared_ptr to raw pointer for FFI
    auto* cmd_ptr = cmd ? new shared_ptr<Marshallable>(cmd) : nullptr;

    // Create callback context
    auto* callback_ptr = new function<void()>(cb);
    auto callback_wrapper = [](void* ctx) {
        auto* cb_ptr = static_cast<function<void()>*>(ctx);
        (*cb_ptr)();
        delete cb_ptr;
    };

    rust_raft_server_on_append_entries(
        rust_server_ptr_,
        slot_id,
        ballot,
        leaderCurrentTerm,
        leaderPrevLogIndex,
        leaderPrevLogTerm,
        leaderCommitIndex,
        cmd_ptr,
        leaderNextLogTerm,
        followerAppendOK,
        followerCurrentTerm,
        followerLastLogIndex,
        callback_wrapper,
        callback_ptr
    );
}

void RaftServer::applyLogs() {
    rust_raft_server_apply_logs(rust_server_ptr_);
}

void RaftServer::SetLocalAppend(shared_ptr<Marshallable>& cmd,
                                 uint64_t* term,
                                 uint64_t* index,
                                 slotid_t slot_id,
                                 ballot_t ballot) {
    // Convert shared_ptr to raw pointer for FFI
    auto* cmd_ptr = new shared_ptr<Marshallable>(cmd);
    rust_raft_server_set_local_append(
        rust_server_ptr_,
        cmd_ptr,
        term,
        index,
        slot_id,
        ballot
    );
}

void RaftServer::removeCmd(slotid_t slot) {
    rust_raft_server_remove_cmd(rust_server_ptr_, slot);
}

shared_ptr<RaftData> RaftServer::GetRaftInstance(slotid_t id) {
    void* data_ptr = rust_raft_server_get_raft_instance(rust_server_ptr_, id);
    // Note: This returns a shared_ptr managed by Rust
    // Assumes Rust properly maintains lifetime
    return *(static_cast<shared_ptr<RaftData>*>(data_ptr));
}

} // namespace janus
