// server.rs - Rust Raft lab skeleton
//
// This file intentionally contains a minimal/stubbed implementation for lab use.
// Students should implement the Raft logic in the TODO sections.

use crate::wrappers::*;
use log::info;
use std::collections::HashMap;
use std::ffi::c_void;
use std::time::Instant;
use rand::Rng;

// Wrapper to make raw pointers Send (we manage thread safety via cooperative coroutines)
pub struct SendPtr(pub *mut c_void);
unsafe impl Send for SendPtr {}

impl SendPtr {
    pub fn new(ptr: *mut c_void) -> Self {
        SendPtr(ptr)
    }

    pub fn get(&self) -> *mut c_void {
        self.0
    }

    pub fn is_null(&self) -> bool {
        self.0.is_null()
    }
}

const INVALID_SITEID: siteid_t = u16::MAX;
const HEARTBEAT_INTERVAL: u64 = 100000; // microseconds

extern "C" fn coroutine_trampoline(context: *mut c_void) {
    unsafe {
        let closure = Box::from_raw(context as *mut Box<dyn FnOnce() + Send>);
        closure();
    }
}

pub fn spawn_coroutine<F>(closure: F)
where
    F: FnOnce() + Send + 'static,
{
    let boxed_closure: Box<Box<dyn FnOnce() + Send>> = Box::new(Box::new(closure));
    let context = Box::into_raw(boxed_closure) as *mut c_void;
    unsafe {
        crate::ffi::raft_coroutine_create_run(coroutine_trampoline, context);
    }
}

fn random_range(min: i32, max: i32) -> i32 {
    rand::thread_rng().gen_range(min..max)
}

fn get_time_micros() -> u64 {
    static START_TIME: std::sync::OnceLock<Instant> = std::sync::OnceLock::new();
    let start = START_TIME.get_or_init(|| Instant::now());
    start.elapsed().as_micros() as u64
}

pub struct RaftServer {
    // Infrastructure (used by FFI/runtime)
    pub commo: Commo,
    pub logs: LogStore,
    pub app_callback: AppCallback,
    pub loc_id: u32,
    pub site_id: siteid_t,
    pub partition_id: parid_t,
    pub stop: bool,
    pub disconnected: bool,
    pub(crate) frame_raw: SendPtr,
    pub(crate) tx_sched_raw: SendPtr,

    // Raft state
    pub current_term: u64,
    pub last_log_index: u64,
    pub commit_index: u64,
    pub execute_index: u64,

    // Leader/election state
    pub is_leader: bool,
    pub match_index: HashMap<siteid_t, u64>,
    pub next_index: HashMap<siteid_t, u64>,
    pub vote_for: siteid_t,
    pub last_heartbeat_time: u64,

    // Runtime flags
    pub req_voting: bool,
    pub in_applying_logs: bool,
    pub failover: bool,
    pub looping: bool,
    pub heartbeat: bool,
    pub init: bool,

    // Misc state kept for compatibility with wrappers/FFI
    pub counter: i64,
    pub snap_idx: slotid_t,
    pub snap_term: ballot_t,
    pub leader_start_time: u64,
    pub n_vote: i32,
    pub n_prepare: i32,
    pub n_accept: i32,
    pub n_commit: i32,
    pub min_active_slot: slotid_t,
    pub max_executed_slot: slotid_t,
    pub max_committed_slot: slotid_t,
}

impl RaftServer {
    /// Create a new RaftServer instance.
    pub fn new(
        frame: crate::ffi::Frame_C,
        commo: crate::ffi::RaftCommo_C,
        raft_logs: crate::ffi::RaftLogs_C,
        tx_sched: crate::ffi::TxScheduler_C,
        app_next: crate::ffi::AppNextFn_C,
        is_test_mode: bool,
    ) -> Self {
        unsafe {
            let locale_id = crate::ffi::raft_frame_get_locale_id(frame);
            let site_id = crate::ffi::raft_frame_get_site_id(frame);
            let partition_id = crate::ffi::raft_frame_get_partition_id(frame);

            let initial_is_leader = if is_test_mode { false } else { locale_id == 0 };

            RaftServer {
                commo: Commo::from_raw(commo),
                logs: LogStore::from_raw(raft_logs),
                app_callback: AppCallback::from_raw(app_next),
                loc_id: locale_id,
                site_id,
                partition_id,
                stop: false,
                disconnected: false,
                frame_raw: SendPtr::new(frame),
                tx_sched_raw: SendPtr::new(tx_sched),

                current_term: 0,
                last_log_index: 0,
                commit_index: 0,
                execute_index: 0,

                is_leader: initial_is_leader,
                match_index: HashMap::new(),
                next_index: HashMap::new(),
                vote_for: INVALID_SITEID,
                last_heartbeat_time: get_time_micros(),

                req_voting: false,
                in_applying_logs: false,
                failover: true,
                looping: false,
                heartbeat: true,
                init: false,

                counter: 0,
                snap_idx: 0,
                snap_term: 0,
                leader_start_time: 0,
                n_vote: 0,
                n_prepare: 0,
                n_accept: 0,
                n_commit: 0,
                min_active_slot: 1,
                max_executed_slot: 0,
                max_committed_slot: 0,
            }
        }
    }

    pub fn is_leader(&self) -> bool {
        self.is_leader
    }

    pub fn is_disconnected(&self) -> bool {
        self.disconnected
    }

    pub fn get_state(&self) -> (bool, u64) {
        (self.is_leader, self.current_term)
    }

    pub fn set_is_leader(&mut self, is_leader: bool) {
        self.is_leader = is_leader;
        if is_leader {
            self.leader_start_time = get_time_micros();
        }
    }

    pub fn disconnect(&mut self, disconnect: bool) {
        // Lab skeleton: tests call disconnect/reconnect to simulate failures.
        // Keep behavior simple; students can refine if needed.
        self.disconnected = disconnect;
        info!("site {} disconnected={}", self.site_id, disconnect);
    }

    pub fn reconnect(&mut self) {
        self.disconnect(false);
        self.reset_timer();
    }

    fn reset_timer(&mut self) {
        self.last_heartbeat_time = get_time_micros();
    }

    fn reset_timer_batch(&mut self) {
        if !self.failover {
            return;
        }

        self.counter += 1;
        if self.counter > 100 {
            let elapsed_micros = get_time_micros() - self.last_heartbeat_time;
            if elapsed_micros > 1_000_000 {
                self.reset_timer();
            }
            self.counter = 0;
        }
    }

    fn rand_duration(&self) -> i32 {
        random_range((4 * HEARTBEAT_INTERVAL) as i32, (7 * HEARTBEAT_INTERVAL) as i32)
    }

    // -------------------------------------------------------------------------
    // Lab skeleton methods (students implement these)
    // -------------------------------------------------------------------------

    pub fn setup(&mut self, _handle: ServerHandle) {
        // TODO: start heartbeat loop + election timer coroutines.
        self.init = true;
    }

    fn start_election_timer(_handle: ServerHandle) {
        // TODO: randomized election timeout loop.
    }

    fn start_heartbeat_loop(_handle: ServerHandle) {
        // TODO: periodic heartbeat / AppendEntries loop.
    }

    pub fn heartbeat_iteration(&mut self) {
        // TODO: leader replication logic.
    }

    pub fn request_vote(&mut self) -> bool {
        // TODO: increment term, request votes, become leader on quorum.
        false
    }

    pub fn on_request_vote(
        &mut self,
        _lst_log_idx: slotid_t,
        _lst_log_term: ballot_t,
        _can_id: siteid_t,
        _can_term: ballot_t,
        reply: VoteReply,
    ) {
        // TODO: term/log checks + vote granting logic.
        reply.set_term(self.current_term as ballot_t);
        reply.set_vote_granted(false);
    }

    pub fn on_append_entries(
        &mut self,
        _slot_id: slotid_t,
        _ballot: ballot_t,
        _leader_current_term: u64,
        _leader_prev_log_index: u64,
        _leader_prev_log_term: u64,
        _leader_commit_index: u64,
        _cmd: Command,
        _leader_next_log_term: u64,
        reply: AppendEntriesReply,
    ) {
        // TODO: heartbeat handling + log consistency + commit advancement.
        reply.reject(self.current_term, self.last_log_index);
    }

    pub fn start(
        &mut self,
        _cmd: Command,
        index: &mut u64,
        term: &mut u64,
        _slot_id: slotid_t,
        _ballot: ballot_t,
    ) -> bool {
        // TODO: if leader, append a new log entry and start replication.
        *index = 0;
        *term = 0;
        false
    }

    pub fn set_local_append(
        &mut self,
        _cmd: Command,
        term: &mut u64,
        index: &mut u64,
        _slot_id: slotid_t,
        _ballot: ballot_t,
    ) {
        // TODO: local append helper.
        *index = self.last_log_index;
        *term = self.current_term;
    }

    pub fn apply_logs(&mut self) {
        // TODO: apply committed logs in order via app_callback.apply(...).
    }

    pub fn remove_cmd(&mut self, slot: slotid_t) {
        self.logs.erase(slot);
    }

    pub fn get_raft_instance(&self, id: slotid_t) -> crate::ffi::RaftData_C {
        self.logs.get(id).as_raw()
    }
}

impl Drop for RaftServer {
    fn drop(&mut self) {
        self.stop = true;
    }
}
