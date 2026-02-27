// lib.rs - Main Rust library entry point
// Exports FFI functions for C++ to call
//
// This is the adapter layer between raw C FFI and safe Rust wrappers.
// It converts raw pointers into safe wrapper types before calling server methods.

mod ffi;
pub mod wrappers;
mod server;

use server::RaftServer;
use ffi::*;
use wrappers::{Command, VoteReply, AppendEntriesReply, ServerHandle};
use std::collections::HashMap;
use std::sync::Mutex;
use std::ffi::c_void;
use lazy_static::lazy_static;
use std::sync::atomic::{AtomicUsize, Ordering};
use log::info;

// Wrapper to make raw pointer Send-able across coroutines
struct SendRaftServerPtr(*mut RaftServer);
unsafe impl Send for SendRaftServerPtr {}

impl SendRaftServerPtr {
    fn new(ptr: *mut RaftServer) -> Self {
        SendRaftServerPtr(ptr)
    }

    fn get(&self) -> *mut RaftServer {
        self.0
    }
}

// =============================================================================
// Global Registry
// =============================================================================

lazy_static! {
    /// Global registry mapping server IDs to raw RaftServer pointers
    static ref SERVER_REGISTRY: Mutex<HashMap<usize, SendRaftServerPtr>> = {
        Mutex::new(HashMap::new())
    };

    /// Initialize logger once
    static ref LOGGER_INIT: () = {
        let _ = env_logger::builder()
            .filter_level(log::LevelFilter::Info)
            .try_init();
    };
}

/// Global counter for assigning unique server IDs
static NEXT_SERVER_ID: AtomicUsize = AtomicUsize::new(1);

/// Get next unique server ID
fn next_server_id() -> usize {
    NEXT_SERVER_ID.fetch_add(1, Ordering::SeqCst)
}

/// Helper macro to get raw server pointer from registry
macro_rules! get_server_ptr {
    ($id:expr) => {{
        let registry = SERVER_REGISTRY.lock().unwrap();
        registry.get(&$id).map(|p| p.get())
    }};
}

// =============================================================================
// FFI Exports - C++ can call these functions
// =============================================================================

/// Create a new RaftServer instance
#[no_mangle]
pub extern "C" fn rust_raft_server_new(
    frame: Frame_C,
    commo: RaftCommo_C,
    raft_logs: RaftLogs_C,
    tx_sched: TxScheduler_C,
    app_next: AppNextFn_C,
    is_test_mode: bool,
) -> *mut c_void {
    let _ = *LOGGER_INIT;

    let server = RaftServer::new(frame, commo, raft_logs, tx_sched, app_next, is_test_mode);
    let raw_server = Box::into_raw(Box::new(server));

    let id = next_server_id();
    SERVER_REGISTRY.lock().unwrap().insert(id, SendRaftServerPtr::new(raw_server));

    id as *mut c_void
}

/// Delete a RaftServer instance
#[no_mangle]
pub extern "C" fn rust_raft_server_delete(server: *mut c_void) {
    let id = server as usize;

    let server_ptr = {
        let registry = SERVER_REGISTRY.lock().unwrap();
        registry.get(&id).map(|p| p.get())
    };

    if let Some(ptr) = server_ptr {
        unsafe {
            (*ptr).stop = true;
            (*ptr).looping = false;
        }

        std::thread::sleep(std::time::Duration::from_millis(200));

        SERVER_REGISTRY.lock().unwrap().remove(&id);
        unsafe {
            let _ = Box::from_raw(ptr);
        }
    }
}

/// Setup the RaftServer (start election timer, heartbeat loop)
#[no_mangle]
pub extern "C" fn rust_raft_server_setup(server: *mut c_void) {
    let id = server as usize;
    let registry = SERVER_REGISTRY.lock().unwrap();

    if let Some(send_ptr) = registry.get(&id) {
        let server_ptr = send_ptr.get();
        unsafe {
            info!("[lib.rs] rust_raft_server_setup called for server ID {} (site_id: {})", id, (*server_ptr).site_id);
            let handle = ServerHandle::new(server_ptr);
            (*server_ptr).setup(handle);
        }
    }
}

/// Check if server is leader
#[no_mangle]
pub extern "C" fn rust_raft_server_is_leader(server: *mut c_void) -> bool {
    let id = server as usize;
    if let Some(server_ptr) = get_server_ptr!(id) {
        unsafe {
            return (*server_ptr).is_leader();
        }
    }
    false
}

/// Set leader status
#[no_mangle]
pub extern "C" fn rust_raft_server_set_is_leader(server: *mut c_void, is_leader: bool) {
    let id = server as usize;
    if let Some(server_ptr) = get_server_ptr!(id) {
        unsafe {
            (*server_ptr).set_is_leader(is_leader);
        }
    }
}

/// Get server state (is_leader, term)
#[no_mangle]
pub extern "C" fn rust_raft_server_get_state(
    server: *mut c_void,
    is_leader: *mut bool,
    term: *mut u64,
) {
    let id = server as usize;

    if let Some(server_ptr) = get_server_ptr!(id) {
        unsafe {
            let (leader, t) = (*server_ptr).get_state();

            if !is_leader.is_null() {
                *is_leader = leader;
            }
            if !term.is_null() {
                *term = t;
            }
        }
    }
}

/// Disconnect from network (for testing)
#[no_mangle]
pub extern "C" fn rust_raft_server_disconnect(server: *mut c_void, disconnect: bool) {
    let id = server as usize;

    if let Some(server_ptr) = get_server_ptr!(id) {
        unsafe {
            (*server_ptr).disconnect(disconnect);
        }
    }
}

/// Reconnect to network
#[no_mangle]
pub extern "C" fn rust_raft_server_reconnect(server: *mut c_void) {
    let id = server as usize;

    if let Some(server_ptr) = get_server_ptr!(id) {
        unsafe {
            (*server_ptr).reconnect();
        }
    }
}

/// Check if disconnected
#[no_mangle]
pub extern "C" fn rust_raft_server_is_disconnected(server: *mut c_void) -> bool {
    let id = server as usize;

    if let Some(server_ptr) = get_server_ptr!(id) {
        unsafe {
            return (*server_ptr).is_disconnected();
        }
    }
    false
}

/// Start accepting a new command (leader only)
/// Converts raw Marshallable_C to safe Command before calling server
#[no_mangle]
pub extern "C" fn rust_raft_server_start(
    server: *mut c_void,
    cmd: Marshallable_C,
    index: *mut u64,
    term: *mut u64,
    slot_id: slotid_t,
    ballot: ballot_t,
) -> bool {
    let id = server as usize;

    if let Some(server_ptr) = get_server_ptr!(id) {
        info!("appended to local log");

        unsafe {
            let mut idx = 0u64;
            let mut t = 0u64;
            let result = (*server_ptr).start(Command::from_raw(cmd), &mut idx, &mut t, slot_id, ballot);

            if !index.is_null() {
                *index = idx;
            }
            if !term.is_null() {
                *term = t;
            }

            return result;
        }
    }
    false
}

/// Request vote from peers (start election)
#[no_mangle]
pub extern "C" fn rust_raft_server_request_vote(server: *mut c_void) -> bool {
    let id = server as usize;

    if let Some(server_ptr) = get_server_ptr!(id) {
        unsafe {
            return (*server_ptr).request_vote();
        }
    }
    false
}

/// Handle vote request RPC from candidate
/// Constructs a safe VoteReply (RAII) before calling server
#[no_mangle]
pub extern "C" fn rust_raft_server_on_request_vote(
    server: *mut c_void,
    lst_log_idx: slotid_t,
    lst_log_term: ballot_t,
    can_id: siteid_t,
    can_term: ballot_t,
    reply_term: *mut ballot_t,
    vote_granted: *mut bool_t,
    callback: unsafe extern "C" fn(*mut c_void),
    callback_context: *mut c_void,
) {
    let id = server as usize;

    if let Some(server_ptr) = get_server_ptr!(id) {
        unsafe {
            let reply = VoteReply::new(reply_term, vote_granted, callback, callback_context);
            (*server_ptr).on_request_vote(
                lst_log_idx,
                lst_log_term,
                can_id,
                can_term,
                reply,
            );
        }
    }
}

/// Handle append entries RPC from leader
/// Constructs safe Command and AppendEntriesReply (RAII) before calling server
#[no_mangle]
pub extern "C" fn rust_raft_server_on_append_entries(
    server: *mut c_void,
    slot_id: slotid_t,
    ballot: ballot_t,
    leader_current_term: u64,
    leader_prev_log_index: u64,
    leader_prev_log_term: u64,
    leader_commit_index: u64,
    cmd: Marshallable_C,
    leader_next_log_term: u64,
    follower_append_ok: *mut u64,
    follower_current_term: *mut u64,
    follower_last_log_index: *mut u64,
    callback: unsafe extern "C" fn(*mut c_void),
    callback_context: *mut c_void,
) {
    let id = server as usize;

    if let Some(server_ptr) = get_server_ptr!(id) {
        unsafe {
            let reply = AppendEntriesReply::new(
                follower_append_ok,
                follower_current_term,
                follower_last_log_index,
                callback,
                callback_context,
            );
            (*server_ptr).on_append_entries(
                slot_id,
                ballot,
                leader_current_term,
                leader_prev_log_index,
                leader_prev_log_term,
                leader_commit_index,
                Command::from_raw(cmd),
                leader_next_log_term,
                reply,
            );
        }
    }
}

/// Apply committed logs to state machine
#[no_mangle]
pub extern "C" fn rust_raft_server_apply_logs(server: *mut c_void) {
    let id = server as usize;

    if let Some(server_ptr) = get_server_ptr!(id) {
        unsafe {
            (*server_ptr).apply_logs();
        }
    }
}

/// Set local append (internal use)
#[no_mangle]
pub extern "C" fn rust_raft_server_set_local_append(
    server: *mut c_void,
    cmd: Marshallable_C,
    term: *mut u64,
    index: *mut u64,
    slot_id: slotid_t,
    ballot: ballot_t,
) {
    let id = server as usize;

    if let Some(server_ptr) = get_server_ptr!(id) {
        unsafe {
            (*server_ptr).set_local_append(Command::from_raw(cmd), &mut *term, &mut *index, slot_id, ballot);
        }
    }
}

/// Remove old command (garbage collection)
#[no_mangle]
pub extern "C" fn rust_raft_server_remove_cmd(server: *mut c_void, slot: slotid_t) {
    let id = server as usize;

    if let Some(server_ptr) = get_server_ptr!(id) {
        unsafe {
            (*server_ptr).remove_cmd(slot);
        }
    }
}

/// Get raft instance at index
#[no_mangle]
pub extern "C" fn rust_raft_server_get_raft_instance(
    server: *mut c_void,
    id: slotid_t,
) -> RaftData_C {
    let server_id = server as usize;

    if let Some(server_ptr) = get_server_ptr!(server_id) {
        unsafe {
            return (*server_ptr).get_raft_instance(id);
        }
    }
    std::ptr::null_mut()
}

/// Update commo pointer (called in Setup when commo is available)
#[no_mangle]
pub extern "C" fn rust_raft_server_set_commo(server: *mut c_void, commo: RaftCommo_C) {
    let id = server as usize;

    if let Some(server_ptr) = get_server_ptr!(id) {
        unsafe {
            (*server_ptr).commo.update_raw(commo);
            info!("[lib.rs] Updated commo pointer for site {}", (*server_ptr).site_id);
        }
    }
}
