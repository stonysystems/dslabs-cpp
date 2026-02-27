// wrappers.rs - Safe wrapper types for the Raft lab FFI
//
// DO NOT MODIFY - This is infrastructure code.
//
// These wrappers provide safe Rust APIs over the raw C FFI functions.
// Students interact with these types instead of raw *mut c_void pointers.

use crate::ffi;
use std::ffi::c_void;

// Re-export primitive type aliases so students can use them
pub use ffi::{slotid_t, ballot_t, siteid_t, parid_t, bool_t};

// Re-export the spawn_coroutine helper (defined in server.rs infrastructure section)
// Students use: spawn_coroutine(move || { ... })

// =============================================================================
// Command - wraps Marshallable_C
// =============================================================================

/// An opaque command object from the C++ side.
/// May be null/empty (e.g., heartbeat messages carry no command).
#[derive(Clone, Copy)]
pub struct Command {
    raw: ffi::Marshallable_C,
}

impl Command {
    pub(crate) fn from_raw(ptr: ffi::Marshallable_C) -> Self {
        Command { raw: ptr }
    }

    /// Check if this is an empty/null command (e.g., heartbeat).
    pub fn is_empty(&self) -> bool {
        self.raw.is_null()
    }

    /// Get the command kind (type tag).
    pub fn kind(&self) -> i32 {
        if self.raw.is_null() {
            return 0;
        }
        unsafe { ffi::raft_marshallable_get_kind(self.raw) }
    }

    pub(crate) fn as_raw(&self) -> ffi::Marshallable_C {
        self.raw
    }

    /// Create a null/empty command.
    pub fn null() -> Self {
        Command {
            raw: std::ptr::null_mut(),
        }
    }
}

// =============================================================================
// LogEntry - wraps RaftData_C
// =============================================================================

/// A log entry in the Raft log.
/// Provides safe get/set access to term, command, slot_id, ballot, etc.
///
/// Uses interior mutability: setters take &self because the underlying
/// C++ object is mutated through the opaque pointer. This is safe in the
/// single-threaded coroutine model.
pub struct LogEntry {
    raw: ffi::RaftData_C,
}

impl LogEntry {
    pub(crate) fn from_raw(ptr: ffi::RaftData_C) -> Self {
        LogEntry { raw: ptr }
    }

    /// Get the term of this log entry.
    pub fn term(&self) -> ballot_t {
        unsafe { ffi::raft_data_get_term(self.raw) }
    }

    /// Set the term of this log entry.
    pub fn set_term(&self, term: ballot_t) {
        unsafe { ffi::raft_data_set_term(self.raw, term) }
    }

    /// Get the command stored in this log entry.
    pub fn command(&self) -> Command {
        Command::from_raw(unsafe { ffi::raft_data_get_log(self.raw) })
    }

    /// Set the command for this log entry.
    pub fn set_command(&self, cmd: Command) {
        unsafe { ffi::raft_data_set_log(self.raw, cmd.as_raw()) }
    }

    /// Get the previous term.
    pub fn prev_term(&self) -> ballot_t {
        unsafe { ffi::raft_data_get_prev_term(self.raw) }
    }

    /// Set the previous term.
    pub fn set_prev_term(&self, term: ballot_t) {
        unsafe { ffi::raft_data_set_prev_term(self.raw, term) }
    }

    /// Get the slot ID.
    pub fn slot_id(&self) -> slotid_t {
        unsafe { ffi::raft_data_get_slot_id(self.raw) }
    }

    /// Set the slot ID.
    pub fn set_slot_id(&self, slot_id: slotid_t) {
        unsafe { ffi::raft_data_set_slot_id(self.raw, slot_id) }
    }

    /// Get the ballot.
    pub fn ballot(&self) -> ballot_t {
        unsafe { ffi::raft_data_get_ballot(self.raw) }
    }

    /// Set the ballot.
    pub fn set_ballot(&self, ballot: ballot_t) {
        unsafe { ffi::raft_data_set_ballot(self.raw, ballot) }
    }

    pub(crate) fn as_raw(&self) -> ffi::RaftData_C {
        self.raw
    }
}

// =============================================================================
// LogStore - wraps RaftLogs_C
// =============================================================================

/// The Raft log store. Provides indexed access to log entries.
pub struct LogStore {
    raw: ffi::RaftLogs_C,
}

// Safety: LogStore is used across coroutine boundaries (single-threaded cooperative)
unsafe impl Send for LogStore {}

impl LogStore {
    pub(crate) fn from_raw(ptr: ffi::RaftLogs_C) -> Self {
        LogStore { raw: ptr }
    }

    /// Get (or create) the log entry at the given index.
    /// The C++ side auto-creates entries on first access.
    pub fn get(&self, slot_id: slotid_t) -> LogEntry {
        LogEntry::from_raw(unsafe { ffi::raft_logs_get_instance(self.raw, slot_id) })
    }

    /// Remove the log entry at the given index.
    pub fn erase(&self, slot_id: slotid_t) {
        unsafe { ffi::raft_logs_erase(self.raw, slot_id) }
    }
}

// =============================================================================
// EventOutcome - result of waiting on an async event
// =============================================================================

/// Result of waiting on an asynchronous RPC event.
pub enum EventOutcome {
    /// RPC completed (check reply values for success/failure).
    Ok,
    /// RPC timed out.
    Timeout,
}

// =============================================================================
// AppendEntriesFuture - pending AppendEntries RPC
// =============================================================================

/// Handle to a pending AppendEntries RPC.
/// Call `wait()` to block until the RPC completes or times out,
/// then read the reply values.
pub struct AppendEntriesFuture {
    event: ffi::IntEvent_C,
    // Boxed so the address is stable (C++ writes into these during wait)
    result: Box<AppendEntriesResultInner>,
}

struct AppendEntriesResultInner {
    status: u64,
    term: u64,
    last_log_index: u64,
}

impl AppendEntriesFuture {
    pub(crate) fn new(event: ffi::IntEvent_C, result: Box<AppendEntriesResultInner>) -> Self {
        AppendEntriesFuture { event, result }
    }

    /// Wait for the RPC to complete, with the given timeout in microseconds.
    pub fn wait(&self, timeout_micros: u64) -> EventOutcome {
        unsafe {
            ffi::raft_int_event_wait(self.event, timeout_micros);
            let status = ffi::raft_int_event_get_status(self.event);
            if status == 4 {
                EventOutcome::Timeout
            } else {
                EventOutcome::Ok
            }
        }
    }

    /// The follower's append status (1 = accepted, 0 = rejected).
    pub fn reply_status(&self) -> u64 {
        self.result.status
    }

    /// The follower's current term.
    pub fn reply_term(&self) -> u64 {
        self.result.term
    }

    /// The follower's last log index.
    pub fn reply_last_log_index(&self) -> u64 {
        self.result.last_log_index
    }
}

// =============================================================================
// VoteResult - result of a vote broadcast
// =============================================================================

/// Result of broadcasting a vote request to all peers.
pub struct VoteResult {
    event: ffi::VoteQuorumEvent_C,
}

impl VoteResult {
    pub(crate) fn new(event: ffi::VoteQuorumEvent_C) -> Self {
        VoteResult { event }
    }

    /// Block until the vote quorum is reached.
    pub fn wait(&self) {
        unsafe { ffi::raft_vote_quorum_event_wait(self.event) }
    }

    /// Number of "yes" votes received.
    pub fn yes_count(&self) -> i32 {
        unsafe { ffi::raft_vote_quorum_event_yes(self.event) }
    }

    /// Number of "no" votes received.
    pub fn no_count(&self) -> i32 {
        unsafe { ffi::raft_vote_quorum_event_no(self.event) }
    }

    /// The term from the vote response (relevant when rejected).
    pub fn term(&self) -> ballot_t {
        unsafe { ffi::raft_vote_quorum_event_get_term(self.event) }
    }
}

// =============================================================================
// Commo - wraps RaftCommo_C
// =============================================================================

/// RPC communication handle. Sends AppendEntries and vote requests to peers.
pub struct Commo {
    raw: ffi::RaftCommo_C,
}

// Safety: Commo is used across coroutine boundaries (single-threaded cooperative)
unsafe impl Send for Commo {}

impl Commo {
    pub(crate) fn from_raw(ptr: ffi::RaftCommo_C) -> Self {
        Commo { raw: ptr }
    }

    pub(crate) fn update_raw(&mut self, ptr: ffi::RaftCommo_C) {
        self.raw = ptr;
    }

    pub(crate) fn as_raw(&self) -> ffi::RaftCommo_C {
        self.raw
    }

    /// Check if the commo handle is null (not yet initialized).
    pub fn is_null(&self) -> bool {
        self.raw.is_null()
    }

    /// Send an AppendEntries RPC to a specific follower.
    /// Returns a future that can be waited on.
    pub fn send_append_entries(
        &self,
        site_id: siteid_t,
        par_id: parid_t,
        slot_id: slotid_t,
        ballot: ballot_t,
        is_leader: bool,
        current_term: u64,
        prev_log_index: u64,
        prev_log_term: u64,
        commit_index: u64,
        cmd: Command,
        cmd_log_term: u64,
    ) -> AppendEntriesFuture {
        let mut result = Box::new(AppendEntriesResultInner {
            status: 0,
            term: 0,
            last_log_index: 0,
        });
        let event = unsafe {
            ffi::raft_commo_send_append_entries(
                self.raw,
                site_id,
                par_id,
                slot_id,
                ballot,
                if is_leader { 1 } else { 0 },
                current_term,
                prev_log_index,
                prev_log_term,
                commit_index,
                cmd.as_raw(),
                cmd_log_term,
                &mut result.status,
                &mut result.term,
                &mut result.last_log_index,
            )
        };
        AppendEntriesFuture::new(event, result)
    }

    /// Broadcast a vote request to all peers in the partition.
    pub fn broadcast_vote(
        &self,
        par_id: parid_t,
        last_log_idx: slotid_t,
        last_log_term: ballot_t,
        self_id: siteid_t,
        current_term: ballot_t,
    ) -> VoteResult {
        let event = unsafe {
            ffi::raft_commo_broadcast_vote(
                self.raw,
                par_id,
                last_log_idx,
                last_log_term,
                self_id,
                current_term,
            )
        };
        VoteResult::new(event)
    }

    /// Get the list of peer site IDs in the given partition.
    pub fn get_peer_site_ids(&self, par_id: parid_t) -> Vec<siteid_t> {
        unsafe {
            let proxies = ffi::raft_commo_get_rpc_par_proxies(self.raw, par_id);
            if proxies.is_null() {
                return Vec::new();
            }
            let size = ffi::raft_rpc_proxies_size(proxies);
            let mut site_ids = vec![0u16; size];
            ffi::raft_rpc_proxies_get_site_ids(proxies, site_ids.as_mut_ptr(), size);
            site_ids
        }
    }

    /// Get the number of RPC proxies for the given partition.
    pub(crate) fn proxy_count(&self, par_id: parid_t) -> usize {
        unsafe { ffi::raft_commo_get_rpc_proxy_count(self.raw, par_id) }
    }

    /// Save RPC proxies to backup (for disconnect simulation).
    pub(crate) fn save_proxies(&self, par_id: parid_t, loc_id: u32) {
        unsafe { ffi::raft_commo_save_rpc_proxies(self.raw, par_id, loc_id) }
    }

    /// Restore RPC proxies from backup (for reconnect simulation).
    pub(crate) fn restore_proxies(&self, par_id: parid_t, loc_id: u32) {
        unsafe { ffi::raft_commo_restore_rpc_proxies(self.raw, par_id, loc_id) }
    }
}

/// Check if backup proxies exist (static, no commo instance needed).
pub(crate) fn has_backup_proxies(par_id: parid_t, loc_id: u32) -> bool {
    unsafe { ffi::raft_commo_has_backup_proxies(par_id, loc_id) }
}

/// Get the backup proxy count (static, no commo instance needed).
pub(crate) fn get_backup_proxy_count(par_id: parid_t, loc_id: u32) -> usize {
    unsafe { ffi::raft_commo_get_backup_proxy_count(par_id, loc_id) }
}

// =============================================================================
// AppCallback - wraps AppNextFn_C
// =============================================================================

/// Application state machine callback.
/// Used to apply committed commands to the state machine.
pub struct AppCallback {
    raw: ffi::AppNextFn_C,
}

// Safety: AppCallback is used across coroutine boundaries (single-threaded cooperative)
unsafe impl Send for AppCallback {}

impl AppCallback {
    pub(crate) fn from_raw(ptr: ffi::AppNextFn_C) -> Self {
        AppCallback { raw: ptr }
    }

    /// Apply a committed command to the state machine.
    /// Returns the result from the application layer.
    pub fn apply(&self, index: i32, cmd: Command) -> i32 {
        unsafe { ffi::raft_app_next_call(self.raw, index, cmd.as_raw()) }
    }
}

// =============================================================================
// Config - wraps Config_C
// =============================================================================

/// Cluster configuration.
pub struct Config {
    raw: ffi::Config_C,
}

impl Config {
    /// Get the global config singleton.
    pub fn get() -> Self {
        Config {
            raw: unsafe { ffi::raft_config_get_config() },
        }
    }

    /// Number of servers in the given partition.
    pub fn partition_size(&self, par_id: parid_t) -> usize {
        unsafe { ffi::raft_config_get_partition_size(self.raw, par_id) as usize }
    }
}

// =============================================================================
// VoteReply - RAII reply for RequestVote RPC
// =============================================================================

/// RAII handle for replying to a RequestVote RPC.
///
/// Set the reply values using `set_term()` and `set_vote_granted()`.
/// The reply is automatically sent when this object is dropped (at function exit).
/// You can also call `send()` to send it explicitly.
pub struct VoteReply {
    reply_term: *mut ballot_t,
    vote_granted: *mut bool_t,
    callback: unsafe extern "C" fn(*mut c_void),
    callback_context: *mut c_void,
    sent: bool,
}

// Safety: VoteReply is only used within the RPC handler (single-threaded)
unsafe impl Send for VoteReply {}

impl VoteReply {
    pub(crate) fn new(
        reply_term: *mut ballot_t,
        vote_granted: *mut bool_t,
        callback: unsafe extern "C" fn(*mut c_void),
        callback_context: *mut c_void,
    ) -> Self {
        VoteReply {
            reply_term,
            vote_granted,
            callback,
            callback_context,
            sent: false,
        }
    }

    /// Set the reply term.
    pub fn set_term(&self, term: ballot_t) {
        unsafe {
            *self.reply_term = term;
        }
    }

    /// Grant or deny the vote.
    pub fn set_vote_granted(&self, granted: bool) {
        unsafe {
            *self.vote_granted = if granted { 1 } else { 0 };
        }
    }

    /// Explicitly send the reply now. If not called, it is sent on drop.
    pub fn send(mut self) {
        self.do_send();
        self.sent = true;
    }

    fn do_send(&self) {
        unsafe {
            (self.callback)(self.callback_context);
        }
    }
}

impl Drop for VoteReply {
    fn drop(&mut self) {
        if !self.sent {
            self.do_send();
        }
    }
}

// =============================================================================
// AppendEntriesReply - RAII reply for AppendEntries RPC
// =============================================================================

/// RAII handle for replying to an AppendEntries RPC.
///
/// Set the reply values using the setter methods, or use the convenience
/// methods `accept()` / `reject()`. The reply is automatically sent when
/// this object is dropped.
pub struct AppendEntriesReply {
    follower_append_ok: *mut u64,
    follower_current_term: *mut u64,
    follower_last_log_index: *mut u64,
    callback: unsafe extern "C" fn(*mut c_void),
    callback_context: *mut c_void,
    sent: bool,
}

// Safety: AppendEntriesReply is only used within the RPC handler (single-threaded)
unsafe impl Send for AppendEntriesReply {}

impl AppendEntriesReply {
    pub(crate) fn new(
        follower_append_ok: *mut u64,
        follower_current_term: *mut u64,
        follower_last_log_index: *mut u64,
        callback: unsafe extern "C" fn(*mut c_void),
        callback_context: *mut c_void,
    ) -> Self {
        AppendEntriesReply {
            follower_append_ok,
            follower_current_term,
            follower_last_log_index,
            callback,
            callback_context,
            sent: false,
        }
    }

    /// Set whether the append was accepted (true) or rejected (false).
    pub fn set_ok(&self, ok: bool) {
        unsafe {
            *self.follower_append_ok = if ok { 1 } else { 0 };
        }
    }

    /// Set the follower's current term.
    pub fn set_term(&self, term: u64) {
        unsafe {
            *self.follower_current_term = term;
        }
    }

    /// Set the follower's last log index.
    pub fn set_last_log_index(&self, idx: u64) {
        unsafe {
            *self.follower_last_log_index = idx;
        }
    }

    /// Convenience: reject with the given current state.
    pub fn reject(&self, current_term: u64, last_log_index: u64) {
        self.set_ok(false);
        self.set_term(current_term);
        self.set_last_log_index(last_log_index);
    }

    /// Convenience: accept with the given current state.
    pub fn accept(&self, current_term: u64, last_log_index: u64) {
        self.set_ok(true);
        self.set_term(current_term);
        self.set_last_log_index(last_log_index);
    }

    /// Explicitly send the reply now. If not called, it is sent on drop.
    pub fn send(mut self) {
        self.do_send();
        self.sent = true;
    }

    fn do_send(&self) {
        unsafe {
            (self.callback)(self.callback_context);
        }
    }
}

impl Drop for AppendEntriesReply {
    fn drop(&mut self) {
        if !self.sent {
            self.do_send();
        }
    }
}

// =============================================================================
// ServerHandle - safe handle for coroutine access to RaftServer
// =============================================================================

use crate::server::RaftServer;

/// An opaque handle to the RaftServer, for passing into spawned coroutines.
///
/// Use `with()` to safely access the server from within a coroutine:
/// ```ignore
/// spawn_coroutine(move || {
///     handle.with(|server| {
///         // access server fields here
///     });
/// });
/// ```
///
/// Safety guarantee: The server is heap-allocated and outlives all coroutines.
/// This is enforced by the framework (server deletion waits for coroutines to stop).
#[derive(Clone, Copy)]
pub struct ServerHandle {
    ptr: *mut RaftServer,
}

unsafe impl Send for ServerHandle {}

impl ServerHandle {
    pub(crate) fn new(ptr: *mut RaftServer) -> Self {
        ServerHandle { ptr }
    }

    /// Access the server within a coroutine.
    ///
    /// The closure receives a mutable reference to the server.
    /// This is safe because coroutines are cooperatively scheduled
    /// (single-threaded, no preemption).
    pub fn with<F, R>(&self, f: F) -> R
    where
        F: FnOnce(&mut RaftServer) -> R,
    {
        unsafe { f(&mut *self.ptr) }
    }
}

// =============================================================================
// Free functions
// =============================================================================

/// Sleep the current coroutine for the given number of microseconds.
/// This yields to the C++ coroutine scheduler.
pub fn coroutine_sleep(microseconds: u64) {
    unsafe {
        ffi::raft_coroutine_sleep(microseconds);
    }
}
