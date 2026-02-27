#ifndef RUST_FFI_WRAPPER_H_
#define RUST_FFI_WRAPPER_H_

#include "../constants.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Type Definitions
// =============================================================================

// Opaque pointer types for C++ objects
typedef void* Frame_C;
typedef void* RaftCommo_C;
typedef void* Timer_C;
typedef void* Marshallable_C;
typedef void* RaftData_C;
typedef void* IntEvent_C;
typedef void* VoteQuorumEvent_C;
// DiskEvent_C removed - not used by Rust implementation
typedef void* RaftLogs_C;
typedef void* TxScheduler_C;
typedef void* AppNextFn_C;
typedef void* RpcProxies_C;

// Callback function types
typedef void (*raft_callback_t)(void* context);
typedef void (*raft_coroutine_callback_t)(void* context);

// =============================================================================
// A. RaftCommo (RPC Communication)
// =============================================================================

IntEvent_C raft_commo_send_append_entries(
    RaftCommo_C commo,
    siteid_t site_id,
    parid_t par_id,
    slotid_t slot_id,
    ballot_t ballot,
    bool_t is_leader,
    uint64_t current_term,
    uint64_t prev_log_index,
    uint64_t prev_log_term,
    uint64_t commit_index,
    Marshallable_C cmd,
    uint64_t cmd_log_term,
    uint64_t* ret_status,
    uint64_t* ret_term,
    uint64_t* ret_last_log_index
);

VoteQuorumEvent_C raft_commo_broadcast_vote(
    RaftCommo_C commo,
    parid_t par_id,
    slotid_t lst_log_idx,
    ballot_t lst_log_term,
    siteid_t self_id,
    ballot_t cur_term
);

RpcProxies_C raft_commo_get_rpc_par_proxies(
    RaftCommo_C commo,
    parid_t par_id
);

// =============================================================================
// B. Config (Configuration Management)
// =============================================================================

void* raft_config_get_config();

int raft_config_get_partition_size(
    void* config,
    parid_t par_id
);

// =============================================================================
// C. Coroutine (Asynchronous Operations)
// =============================================================================

void raft_coroutine_create_run(
    raft_coroutine_callback_t callback,
    void* context
);

void raft_coroutine_sleep(uint64_t microseconds);

// =============================================================================
// D. Timer (Time Measurement)
// =============================================================================

Timer_C raft_timer_new();

void raft_timer_delete(Timer_C timer);

void raft_timer_start(Timer_C timer);

double raft_timer_elapsed(Timer_C timer);

uint64_t raft_time_now();

// =============================================================================
// E. RandomGenerator (Random Numbers)
// =============================================================================

int raft_random_rand(int min, int max);

double raft_random_rand_double(double min, double max);

// =============================================================================
// F. Logging (Debug/Info/Error Logging)
// =============================================================================
// Logging functions removed - Rust uses native log crate instead

// =============================================================================
// G. Frame (Server Framework)
// =============================================================================

uint32_t raft_frame_get_locale_id(Frame_C frame);

siteid_t raft_frame_get_site_id(Frame_C frame);

parid_t raft_frame_get_partition_id(Frame_C frame);

// =============================================================================
// H. Event (Asynchronous Event Handling)
// =============================================================================

void raft_int_event_wait(IntEvent_C event, uint64_t timeout);

int raft_int_event_get_status(IntEvent_C event);

void raft_vote_quorum_event_wait(VoteQuorumEvent_C event);

int raft_vote_quorum_event_yes(VoteQuorumEvent_C event);

int raft_vote_quorum_event_no(VoteQuorumEvent_C event);

ballot_t raft_vote_quorum_event_get_term(VoteQuorumEvent_C event);

// =============================================================================
// I. IO (Asynchronous I/O Operations) - REMOVED
// =============================================================================
// DiskEvent and IO functions removed as they are not used by Rust implementation

// =============================================================================
// J. RaftData (Log Entry Data)
// =============================================================================

RaftData_C raft_data_new();

ballot_t raft_data_get_term(RaftData_C data);

void raft_data_set_term(RaftData_C data, ballot_t term);

Marshallable_C raft_data_get_log(RaftData_C data);

void raft_data_set_log(RaftData_C data, Marshallable_C log);

ballot_t raft_data_get_prev_term(RaftData_C data);

void raft_data_set_prev_term(RaftData_C data, ballot_t prev_term);

slotid_t raft_data_get_slot_id(RaftData_C data);

void raft_data_set_slot_id(RaftData_C data, slotid_t slot_id);

ballot_t raft_data_get_ballot(RaftData_C data);

void raft_data_set_ballot(RaftData_C data, ballot_t ballot);

// =============================================================================
// K. Marshallable (Command Objects)
// =============================================================================

Marshallable_C raft_marshallable_retain(Marshallable_C ptr);

void raft_marshallable_release(Marshallable_C ptr);

int raft_marshallable_get_kind(Marshallable_C ptr);

// =============================================================================
// L. Logs Map (raft_logs_)
// =============================================================================

RaftData_C raft_logs_get_instance(RaftLogs_C logs, slotid_t slot_id);

void raft_logs_set_instance(RaftLogs_C logs, slotid_t slot_id, RaftData_C data);

void raft_logs_erase(RaftLogs_C logs, slotid_t slot_id);

// =============================================================================
// M. Transaction Scheduler
// =============================================================================

void raft_tx_sched_destroy_tx(TxScheduler_C tx_sched, uint64_t txn_id);

// =============================================================================
// N. App Callback
// =============================================================================

int raft_app_next_call(AppNextFn_C app_next, int index, Marshallable_C cmd);

// =============================================================================
// O. RpcProxies Map Iterator (for setIsLeader initialization)
// =============================================================================

size_t raft_rpc_proxies_size(RpcProxies_C proxies);

void raft_rpc_proxies_get_site_ids(RpcProxies_C proxies, siteid_t* out_site_ids, size_t size);

// =============================================================================
// P. RPC Proxy Manipulation (for disconnect/reconnect)
// =============================================================================

// Clear all RPC proxies from commo for a given partition
void raft_commo_clear_rpc_proxies(RaftCommo_C commo, parid_t par_id);

// Get the number of RPC proxies in commo for a given partition
size_t raft_commo_get_rpc_proxy_count(RaftCommo_C commo, parid_t par_id);

// Save RPC proxies to a backup map (partition_id, locale_id) -> proxies
void raft_commo_save_rpc_proxies(RaftCommo_C commo, parid_t par_id, uint32_t loc_id);

// Restore RPC proxies from the backup map
void raft_commo_restore_rpc_proxies(RaftCommo_C commo, parid_t par_id, uint32_t loc_id);

// Check if backup exists for (partition_id, locale_id)
bool raft_commo_has_backup_proxies(parid_t par_id, uint32_t loc_id);

// Get size of backup proxies for (partition_id, locale_id)
size_t raft_commo_get_backup_proxy_count(parid_t par_id, uint32_t loc_id);

// =============================================================================
// Rust RaftServer FFI Functions (Rust → C++)
// =============================================================================

void* rust_raft_server_new(Frame_C frame, RaftCommo_C commo, RaftLogs_C raft_logs,
                            TxScheduler_C tx_sched, AppNextFn_C app_next, bool is_test_mode);

void rust_raft_server_delete(void* rust_server_ptr);

void rust_raft_server_setup(void* rust_server_ptr);

void rust_raft_server_set_commo(void* rust_server_ptr, RaftCommo_C commo);

bool rust_raft_server_is_leader(void* rust_server_ptr);

void rust_raft_server_set_is_leader(void* rust_server_ptr, bool is_leader);

void rust_raft_server_get_state(void* rust_server_ptr, bool* is_leader, uint64_t* term);

void rust_raft_server_disconnect(void* rust_server_ptr, bool disconnect);

void rust_raft_server_reconnect(void* rust_server_ptr);

bool rust_raft_server_is_disconnected(void* rust_server_ptr);

bool rust_raft_server_start(void* rust_server_ptr, Marshallable_C cmd,
                             uint64_t* index, uint64_t* term,
                             slotid_t slot_id, ballot_t ballot);

bool rust_raft_server_request_vote(void* rust_server_ptr);

void rust_raft_server_on_request_vote(void* rust_server_ptr,
                                       slotid_t lst_log_idx,
                                       ballot_t lst_log_term,
                                       siteid_t can_id,
                                       ballot_t can_term,
                                       ballot_t* reply_term,
                                       bool_t* vote_granted,
                                       raft_callback_t callback,
                                       void* callback_context);

void rust_raft_server_on_append_entries(void* rust_server_ptr,
                                         slotid_t slot_id,
                                         ballot_t ballot,
                                         uint64_t leaderCurrentTerm,
                                         uint64_t leaderPrevLogIndex,
                                         uint64_t leaderPrevLogTerm,
                                         uint64_t leaderCommitIndex,
                                         Marshallable_C cmd,
                                         uint64_t leaderNextLogTerm,
                                         uint64_t* followerAppendOK,
                                         uint64_t* followerCurrentTerm,
                                         uint64_t* followerLastLogIndex,
                                         raft_callback_t callback,
                                         void* callback_context);

void rust_raft_server_apply_logs(void* rust_server_ptr);

void rust_raft_server_set_local_append(void* rust_server_ptr,
                                        Marshallable_C cmd,
                                        uint64_t* term,
                                        uint64_t* index,
                                        slotid_t slot_id,
                                        ballot_t ballot);

void rust_raft_server_remove_cmd(void* rust_server_ptr, slotid_t slot);

RaftData_C rust_raft_server_get_raft_instance(void* rust_server_ptr, slotid_t id);

#ifdef __cplusplus
}
#endif

#endif // RUST_FFI_WRAPPER_H_
