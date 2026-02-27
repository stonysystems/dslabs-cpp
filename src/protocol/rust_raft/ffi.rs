// ffi.rs - Rust FFI declarations for calling C++ wrapper functions
// This file declares all the C functions from rust_ffi_wrapper.h

use std::os::raw::{c_void, c_char, c_int, c_double};

// =============================================================================
// Type Definitions
// =============================================================================

// Primitive type aliases matching C++ constants.h
pub type slotid_t = u64;
pub type ballot_t = i64;      // SIGNED!
pub type siteid_t = u16;      // 16-bit!
pub type parid_t = u32;       // 32-bit!
pub type bool_t = i8;

// Opaque pointer types for C++ objects
pub type Frame_C = *mut c_void;
pub type RaftCommo_C = *mut c_void;
pub type Timer_C = *mut c_void;
pub type Marshallable_C = *mut c_void;
pub type RaftData_C = *mut c_void;
pub type IntEvent_C = *mut c_void;
pub type VoteQuorumEvent_C = *mut c_void;
// DiskEvent_C removed - not used by Rust implementation
pub type RaftLogs_C = *mut c_void;
pub type TxScheduler_C = *mut c_void;
pub type AppNextFn_C = *mut c_void;
pub type RpcProxies_C = *mut c_void;
pub type Config_C = *mut c_void;

// Callback function types
pub type RaftCallback = extern "C" fn(*mut c_void);
pub type CoroutineCallback = extern "C" fn(*mut c_void);

// =============================================================================
// FFI Function Declarations
// =============================================================================

extern "C" {
    // =========================================================================
    // A. RaftCommo (RPC Communication)
    // =========================================================================

    pub fn raft_commo_send_append_entries(
        commo: RaftCommo_C,
        site_id: siteid_t,
        par_id: parid_t,
        slot_id: slotid_t,
        ballot: ballot_t,
        is_leader: bool_t,
        current_term: u64,
        prev_log_index: u64,
        prev_log_term: u64,
        commit_index: u64,
        cmd: Marshallable_C,
        cmd_log_term: u64,
        ret_status: *mut u64,
        ret_term: *mut u64,
        ret_last_log_index: *mut u64,
    ) -> IntEvent_C;

    pub fn raft_commo_broadcast_vote(
        commo: RaftCommo_C,
        par_id: parid_t,
        lst_log_idx: slotid_t,
        lst_log_term: ballot_t,
        self_id: siteid_t,
        cur_term: ballot_t,
    ) -> VoteQuorumEvent_C;

    pub fn raft_commo_get_rpc_par_proxies(
        commo: RaftCommo_C,
        par_id: parid_t,
    ) -> RpcProxies_C;

    // =========================================================================
    // B. Config (Configuration Management)
    // =========================================================================

    pub fn raft_config_get_config() -> Config_C;

    pub fn raft_config_get_partition_size(
        config: Config_C,
        par_id: parid_t,
    ) -> c_int;

    // =========================================================================
    // C. Coroutine (Asynchronous Operations)
    // =========================================================================

    pub fn raft_coroutine_create_run(
        callback: CoroutineCallback,
        context: *mut c_void,
    );

    pub fn raft_coroutine_sleep(microseconds: u64);

    // =========================================================================
    // D. Timer (Time Measurement)
    // =========================================================================

    pub fn raft_timer_new() -> Timer_C;

    pub fn raft_timer_delete(timer: Timer_C);

    pub fn raft_timer_start(timer: Timer_C);

    pub fn raft_timer_elapsed(timer: Timer_C) -> c_double;

    pub fn raft_time_now() -> u64;

    // =========================================================================
    // E. RandomGenerator (Random Numbers)
    // =========================================================================

    pub fn raft_random_rand(min: c_int, max: c_int) -> c_int;

    pub fn raft_random_rand_double(min: c_double, max: c_double) -> c_double;

    // =========================================================================
    // F. Logging (Debug/Info/Error Logging)
    // =========================================================================
    // Logging functions removed - using Rust log crate instead

    // =========================================================================
    // G. Frame (Server Framework)
    // =========================================================================

    pub fn raft_frame_get_locale_id(frame: Frame_C) -> u32;

    pub fn raft_frame_get_site_id(frame: Frame_C) -> siteid_t;

    pub fn raft_frame_get_partition_id(frame: Frame_C) -> parid_t;

    // =========================================================================
    // H. Event (Asynchronous Event Handling)
    // =========================================================================

    pub fn raft_int_event_wait(event: IntEvent_C, timeout: u64);

    pub fn raft_int_event_get_status(event: IntEvent_C) -> c_int;

    pub fn raft_vote_quorum_event_wait(event: VoteQuorumEvent_C);

    pub fn raft_vote_quorum_event_yes(event: VoteQuorumEvent_C) -> c_int;

    pub fn raft_vote_quorum_event_no(event: VoteQuorumEvent_C) -> c_int;

    pub fn raft_vote_quorum_event_get_term(event: VoteQuorumEvent_C) -> ballot_t;

    // =========================================================================
    // I. IO (Asynchronous I/O Operations) - REMOVED
    // =========================================================================
    // DiskEvent and IO functions removed as they are not used by Rust implementation

    // =========================================================================
    // J. RaftData (Log Entry Data)
    // =========================================================================

    pub fn raft_data_new() -> RaftData_C;

    pub fn raft_data_get_term(data: RaftData_C) -> ballot_t;

    pub fn raft_data_set_term(data: RaftData_C, term: ballot_t);

    pub fn raft_data_get_log(data: RaftData_C) -> Marshallable_C;

    pub fn raft_data_set_log(data: RaftData_C, log: Marshallable_C);

    pub fn raft_data_get_prev_term(data: RaftData_C) -> ballot_t;

    pub fn raft_data_set_prev_term(data: RaftData_C, prev_term: ballot_t);

    pub fn raft_data_get_slot_id(data: RaftData_C) -> slotid_t;

    pub fn raft_data_set_slot_id(data: RaftData_C, slot_id: slotid_t);

    pub fn raft_data_get_ballot(data: RaftData_C) -> ballot_t;

    pub fn raft_data_set_ballot(data: RaftData_C, ballot: ballot_t);

    // =========================================================================
    // K. Marshallable (Command Objects)
    // =========================================================================

    pub fn raft_marshallable_retain(ptr: Marshallable_C) -> Marshallable_C;

    pub fn raft_marshallable_release(ptr: Marshallable_C);

    pub fn raft_marshallable_get_kind(ptr: Marshallable_C) -> c_int;

    // =========================================================================
    // L. Logs Map (raft_logs_)
    // =========================================================================

    pub fn raft_logs_get_instance(logs: RaftLogs_C, slot_id: slotid_t) -> RaftData_C;

    pub fn raft_logs_set_instance(logs: RaftLogs_C, slot_id: slotid_t, data: RaftData_C);

    pub fn raft_logs_erase(logs: RaftLogs_C, slot_id: slotid_t);

    // =========================================================================
    // M. Transaction Scheduler
    // =========================================================================

    pub fn raft_tx_sched_destroy_tx(tx_sched: TxScheduler_C, txn_id: u64);

    // =========================================================================
    // N. App Callback
    // =========================================================================

    pub fn raft_app_next_call(app_next: AppNextFn_C, index: c_int, cmd: Marshallable_C) -> c_int;

    // =========================================================================
    // O. RpcProxies Map Iterator (for setIsLeader initialization)
    // =========================================================================

    pub fn raft_rpc_proxies_size(proxies: RpcProxies_C) -> usize;

    pub fn raft_rpc_proxies_get_site_ids(
        proxies: RpcProxies_C,
        out_site_ids: *mut siteid_t,
        size: usize,
    );

    // =========================================================================
    // P. RPC Proxy Manipulation (for disconnect/reconnect)
    // =========================================================================

    pub fn raft_commo_clear_rpc_proxies(
        commo: RaftCommo_C,
        par_id: parid_t,
    );

    pub fn raft_commo_get_rpc_proxy_count(
        commo: RaftCommo_C,
        par_id: parid_t,
    ) -> usize;

    pub fn raft_commo_save_rpc_proxies(
        commo: RaftCommo_C,
        par_id: parid_t,
        loc_id: u32,
    );

    pub fn raft_commo_restore_rpc_proxies(
        commo: RaftCommo_C,
        par_id: parid_t,
        loc_id: u32,
    );

    pub fn raft_commo_has_backup_proxies(
        par_id: parid_t,
        loc_id: u32,
    ) -> bool;

    pub fn raft_commo_get_backup_proxy_count(
        par_id: parid_t,
        loc_id: u32,
    ) -> usize;
}

// =============================================================================
// Logging - Now using native Rust log crate directly
// =============================================================================
// Logging macros removed - use log::{debug, info, warn, error} directly in code
