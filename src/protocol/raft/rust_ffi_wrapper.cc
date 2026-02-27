#include "rust_ffi_wrapper.h"
#include "commo.h"
#include "server.h"
#include "../config.h"
#include "../frame.h"
#include "../scheduler.h"
#include "../communicator.h"
#include "../../rrr/reactor/fiber.h"
#include "../../rrr/base/basetypes.hpp"
#include "../../rrr/misc/rand.hpp"
#include "../../rrr/base/logging.hpp"
#include "../../rrr/misc/io.hpp"
#include <memory>
#include <functional>

using namespace janus;
using namespace rrr;

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
) {
    //Log_info("reached rust_ffi_wrapper send append entries, commo ptr: %p", commo);
    try {
        auto* raft_commo = static_cast<RaftCommo*>(commo);
        auto cmd_ptr = static_cast<std::shared_ptr<Marshallable>*>(cmd);
        //Log_info("raft_commo ptr after cast: %p", raft_commo);

        // Handle null cmd (for heartbeats)
        std::shared_ptr<Marshallable> cmd_shared;
        if (cmd_ptr != nullptr) {
            cmd_shared = *cmd_ptr;
        }

        auto event = raft_commo->SendAppendEntries2(
            site_id,
            par_id,
            slot_id,
            ballot,
            is_leader != 0,
            current_term,
            prev_log_index,
            prev_log_term,
            commit_index,
            cmd_shared,
            cmd_log_term,
            ret_status,
            ret_term,
            ret_last_log_index
        );

        // Return raw pointer from shared_ptr
        return new std::shared_ptr<IntEvent>(event);
    } catch (const std::exception& e) {
        Log_error("raft_commo_send_append_entries failed: %s", e.what());
        return nullptr;
    }
}

VoteQuorumEvent_C raft_commo_broadcast_vote(
    RaftCommo_C commo,
    parid_t par_id,
    slotid_t lst_log_idx,
    ballot_t lst_log_term,
    siteid_t self_id,
    ballot_t cur_term
) {
   
    try {
        auto* raft_commo = static_cast<RaftCommo*>(commo);
        

        auto event = raft_commo->BroadcastVote(
            par_id,
            lst_log_idx,
            lst_log_term,
            self_id,
            cur_term
        );
        //Log_info("after auto event reached rust_ffi_wrapper.cc");

        return new std::shared_ptr<RaftVoteQuorumEvent>(event);
    } catch (const std::exception& e) {
       // Log_info("raft_commo_broadcast_vote failed: %s", e.what());
        return nullptr;
    }
}

RpcProxies_C raft_commo_get_rpc_par_proxies(
    RaftCommo_C commo,
    parid_t par_id
) {
    try {
        auto* raft_commo = static_cast<RaftCommo*>(commo);
        auto& proxies = raft_commo->rpc_par_proxies_[par_id];
        return &proxies;
    } catch (const std::exception& e) {
        Log_error("raft_commo_get_rpc_par_proxies failed: %s", e.what());
        return nullptr;
    }
}

// =============================================================================
// B. Config (Configuration Management)
// =============================================================================

void* raft_config_get_config() {
    try {
        return Config::GetConfig();
    } catch (const std::exception& e) {
        Log_error("raft_config_get_config failed: %s", e.what());
        return nullptr;
    }
}

int raft_config_get_partition_size(
    void* config,
    parid_t par_id
) {
    try {
        auto* cfg = static_cast<Config*>(config);
        return cfg->GetPartitionSize(par_id);
    } catch (const std::exception& e) {
        Log_error("raft_config_get_partition_size failed: %s", e.what());
        return 0;
    }
}

// =============================================================================
// C. Coroutine (Asynchronous Operations)
// =============================================================================

void raft_coroutine_create_run(
    raft_coroutine_callback_t callback,
    void* context
) {
    try {
        Coroutine::CreateRun([callback, context]() {
            callback(context);
        });
    } catch (const std::exception& e) {
        Log_error("raft_coroutine_create_run failed: %s", e.what());
    }
}

void raft_coroutine_sleep(uint64_t microseconds) {
    try {
        Coroutine::Sleep(microseconds);
    } catch (const std::exception& e) {
        Log_error("raft_coroutine_sleep failed: %s", e.what());
    }
}

// =============================================================================
// D. Timer (Time Measurement)
// =============================================================================

Timer_C raft_timer_new() {
    try {
        return new Timer();
    } catch (const std::exception& e) {
        Log_error("raft_timer_new failed: %s", e.what());
        return nullptr;
    }
}

void raft_timer_delete(Timer_C timer) {
    try {
        delete static_cast<Timer*>(timer);
    } catch (const std::exception& e) {
        Log_error("raft_timer_delete failed: %s", e.what());
    }
}

void raft_timer_start(Timer_C timer) {
    try {
        static_cast<Timer*>(timer)->start();
    } catch (const std::exception& e) {
        Log_error("raft_timer_start failed: %s", e.what());
    }
}

double raft_timer_elapsed(Timer_C timer) {
    try {
        return static_cast<Timer*>(timer)->elapsed();
    } catch (const std::exception& e) {
        Log_error("raft_timer_elapsed failed: %s", e.what());
        return 0.0;
    }
}

uint64_t raft_time_now() {
    try {
        return Time::now();
    } catch (const std::exception& e) {
        Log_error("raft_time_now failed: %s", e.what());
        return 0;
    }
}

// =============================================================================
// E. RandomGenerator (Random Numbers)
// =============================================================================

int raft_random_rand(int min, int max) {
    try {
        return RandomGenerator::rand(min, max);
    } catch (const std::exception& e) {
        Log_error("raft_random_rand failed: %s", e.what());
        return min;
    }
}

double raft_random_rand_double(double min, double max) {
    try {
        return RandomGenerator::rand_double(min, max);
    } catch (const std::exception& e) {
        Log_error("raft_random_rand_double failed: %s", e.what());
        return min;
    }
}

// =============================================================================
// F. Logging (Debug/Info/Error Logging)
// =============================================================================
// Logging functions removed - Rust uses native log crate instead

// =============================================================================
// G. Frame (Server Framework)
// =============================================================================

uint32_t raft_frame_get_locale_id(Frame_C frame) {
    try {
        auto* f = static_cast<Frame*>(frame);
        return f->site_info_->locale_id;
    } catch (const std::exception& e) {
        Log_error("raft_frame_get_locale_id failed: %s", e.what());
        return 0;
    }
}

siteid_t raft_frame_get_site_id(Frame_C frame) {
    try {
        auto* f = static_cast<Frame*>(frame);
        return f->site_info_->id;
    } catch (const std::exception& e) {
        Log_error("raft_frame_get_site_id failed: %s", e.what());
        return 0;
    }
}

parid_t raft_frame_get_partition_id(Frame_C frame) {
    try {
        auto* f = static_cast<Frame*>(frame);
        return f->site_info_->partition_id_;
    } catch (const std::exception& e) {
        Log_error("raft_frame_get_partition_id failed: %s", e.what());
        return 0;
    }
}

// =============================================================================
// H. Event (Asynchronous Event Handling)
// =============================================================================

void raft_int_event_wait(IntEvent_C event, uint64_t timeout) {
    try {
        auto* ev = static_cast<std::shared_ptr<IntEvent>*>(event);
        (*ev)->Wait(timeout);
    } catch (const std::exception& e) {
        Log_error("raft_int_event_wait failed: %s", e.what());
    }
}

int raft_int_event_get_status(IntEvent_C event) {
    try {
        auto* ev = static_cast<std::shared_ptr<IntEvent>*>(event);
        return static_cast<int>((*ev)->status_);
    } catch (const std::exception& e) {
        Log_error("raft_int_event_get_status failed: %s", e.what());
        return 0;
    }
}

void raft_vote_quorum_event_wait(VoteQuorumEvent_C event) {
    try {
        auto* ev = static_cast<std::shared_ptr<RaftVoteQuorumEvent>*>(event);
        (*ev)->Wait(1000000);  // 1 second timeout, matching C++ server.cc:328
    } catch (const std::exception& e) {
        Log_error("raft_vote_quorum_event_wait failed: %s", e.what());
    }
}

int raft_vote_quorum_event_yes(VoteQuorumEvent_C event) {
    try {
        auto* ev = static_cast<std::shared_ptr<RaftVoteQuorumEvent>*>(event);
        return (*ev)->Yes();
    } catch (const std::exception& e) {
        Log_error("raft_vote_quorum_event_yes failed: %s", e.what());
        return 0;
    }
}

int raft_vote_quorum_event_no(VoteQuorumEvent_C event) {
    try {
        auto* ev = static_cast<std::shared_ptr<RaftVoteQuorumEvent>*>(event);
        return (*ev)->No();
    } catch (const std::exception& e) {
        Log_error("raft_vote_quorum_event_no failed: %s", e.what());
        return 0;
    }
}

ballot_t raft_vote_quorum_event_get_term(VoteQuorumEvent_C event) {
    try {
        auto* ev = static_cast<std::shared_ptr<RaftVoteQuorumEvent>*>(event);
        return (*ev)->Term();
    } catch (const std::exception& e) {
        Log_error("raft_vote_quorum_event_get_term failed: %s", e.what());
        return 0;
    }
}

// =============================================================================
// I. IO (Asynchronous I/O Operations) - REMOVED
// =============================================================================
// DiskEvent and IO functions removed as they are not used by Rust implementation

// =============================================================================
// J. RaftData (Log Entry Data)
// =============================================================================

RaftData_C raft_data_new() {
    try {
        return new std::shared_ptr<RaftData>(std::make_shared<RaftData>());
    } catch (const std::exception& e) {
        Log_error("raft_data_new failed: %s", e.what());
        return nullptr;
    }
}

ballot_t raft_data_get_term(RaftData_C data) {
    try {
        auto* rd = static_cast<std::shared_ptr<RaftData>*>(data);
        return (*rd)->term;
    } catch (const std::exception& e) {
        Log_error("raft_data_get_term failed: %s", e.what());
        return 0;
    }
}

void raft_data_set_term(RaftData_C data, ballot_t term) {
    try {
        auto* rd = static_cast<std::shared_ptr<RaftData>*>(data);
        (*rd)->term = term;
    } catch (const std::exception& e) {
        Log_error("raft_data_set_term failed: %s", e.what());
    }
}

Marshallable_C raft_data_get_log(RaftData_C data) {
    try {
        auto* rd = static_cast<std::shared_ptr<RaftData>*>(data);
        return new std::shared_ptr<Marshallable>((*rd)->log_);
    } catch (const std::exception& e) {
        Log_error("raft_data_get_log failed: %s", e.what());
        return nullptr;
    }
}

void raft_data_set_log(RaftData_C data, Marshallable_C log) {
    try {
        auto* rd = static_cast<std::shared_ptr<RaftData>*>(data);
        auto* log_ptr = static_cast<std::shared_ptr<Marshallable>*>(log);
        (*rd)->log_ = *log_ptr;
    } catch (const std::exception& e) {
        Log_error("raft_data_set_log failed: %s", e.what());
    }
}

ballot_t raft_data_get_prev_term(RaftData_C data) {
    try {
        auto* rd = static_cast<std::shared_ptr<RaftData>*>(data);
        return (*rd)->prevTerm;
    } catch (const std::exception& e) {
        Log_error("raft_data_get_prev_term failed: %s", e.what());
        return 0;
    }
}

void raft_data_set_prev_term(RaftData_C data, ballot_t prev_term) {
    try {
        auto* rd = static_cast<std::shared_ptr<RaftData>*>(data);
        (*rd)->prevTerm = prev_term;
    } catch (const std::exception& e) {
        Log_error("raft_data_set_prev_term failed: %s", e.what());
    }
}

slotid_t raft_data_get_slot_id(RaftData_C data) {
    try {
        auto* rd = static_cast<std::shared_ptr<RaftData>*>(data);
        return (*rd)->slot_id;
    } catch (const std::exception& e) {
        Log_error("raft_data_get_slot_id failed: %s", e.what());
        return 0;
    }
}

void raft_data_set_slot_id(RaftData_C data, slotid_t slot_id) {
    try {
        auto* rd = static_cast<std::shared_ptr<RaftData>*>(data);
        (*rd)->slot_id = slot_id;
    } catch (const std::exception& e) {
        Log_error("raft_data_set_slot_id failed: %s", e.what());
    }
}

ballot_t raft_data_get_ballot(RaftData_C data) {
    try {
        auto* rd = static_cast<std::shared_ptr<RaftData>*>(data);
        return (*rd)->ballot;
    } catch (const std::exception& e) {
        Log_error("raft_data_get_ballot failed: %s", e.what());
        return 0;
    }
}

void raft_data_set_ballot(RaftData_C data, ballot_t ballot) {
    try {
        auto* rd = static_cast<std::shared_ptr<RaftData>*>(data);
        (*rd)->ballot = ballot;
    } catch (const std::exception& e) {
        Log_error("raft_data_set_ballot failed: %s", e.what());
    }
}

// =============================================================================
// K. Marshallable (Command Objects)
// =============================================================================

Marshallable_C raft_marshallable_retain(Marshallable_C ptr) {
    try {
        auto* sp = static_cast<std::shared_ptr<Marshallable>*>(ptr);
        return new std::shared_ptr<Marshallable>(*sp);
    } catch (const std::exception& e) {
        Log_error("raft_marshallable_retain failed: %s", e.what());
        return nullptr;
    }
}

void raft_marshallable_release(Marshallable_C ptr) {
    try {
        delete static_cast<std::shared_ptr<Marshallable>*>(ptr);
    } catch (const std::exception& e) {
        Log_error("raft_marshallable_release failed: %s", e.what());
    }
}

int raft_marshallable_get_kind(Marshallable_C ptr) {
    try {
        auto* sp = static_cast<std::shared_ptr<Marshallable>*>(ptr);
        return (*sp)->kind_;
    } catch (const std::exception& e) {
        Log_error("raft_marshallable_get_kind failed: %s", e.what());
        return 0;
    }
}

// =============================================================================
// L. Logs Map (raft_logs_)
// =============================================================================

RaftData_C raft_logs_get_instance(RaftLogs_C logs, slotid_t slot_id) {
    try {
        auto* logs_map = static_cast<std::map<slotid_t, std::shared_ptr<RaftData>>*>(logs);
        auto& sp_instance = (*logs_map)[slot_id];
        if (!sp_instance) {
            sp_instance = std::make_shared<RaftData>();
        }
        return new std::shared_ptr<RaftData>(sp_instance);
    } catch (const std::exception& e) {
        Log_error("raft_logs_get_instance failed: %s", e.what());
        return nullptr;
    }
}

void raft_logs_set_instance(RaftLogs_C logs, slotid_t slot_id, RaftData_C data) {
    try {
        auto* logs_map = static_cast<std::map<slotid_t, std::shared_ptr<RaftData>>*>(logs);
        auto* rd = static_cast<std::shared_ptr<RaftData>*>(data);
        (*logs_map)[slot_id] = *rd;
    } catch (const std::exception& e) {
        Log_error("raft_logs_set_instance failed: %s", e.what());
    }
}

void raft_logs_erase(RaftLogs_C logs, slotid_t slot_id) {
    try {
        auto* logs_map = static_cast<std::map<slotid_t, std::shared_ptr<RaftData>>*>(logs);
        logs_map->erase(slot_id);
    } catch (const std::exception& e) {
        Log_error("raft_logs_erase failed: %s", e.what());
    }
}

// =============================================================================
// M. Transaction Scheduler
// =============================================================================

void raft_tx_sched_destroy_tx(TxScheduler_C tx_sched, uint64_t txn_id) {
    try {
        auto* sched = static_cast<TxLogServer*>(tx_sched);
        sched->DestroyTx(txn_id);
    } catch (const std::exception& e) {
        Log_error("raft_tx_sched_destroy_tx failed: %s", e.what());
    }
}

// =============================================================================
// N. App Callback
// =============================================================================

int raft_app_next_call(AppNextFn_C app_next, int index, Marshallable_C cmd) {
    try {
        auto* fn = static_cast<std::function<int(int, std::shared_ptr<Marshallable>)>*>(app_next);
        auto* cmd_ptr = static_cast<std::shared_ptr<Marshallable>*>(cmd);
        return (*fn)(index, *cmd_ptr);
    } catch (const std::exception& e) {
        Log_error("raft_app_next_call failed: %s", e.what());
        return -1;
    }
}

// =============================================================================
// O. RpcProxies Map Iterator (for setIsLeader initialization)
// =============================================================================

size_t raft_rpc_proxies_size(RpcProxies_C proxies) {
    try {
        auto* vec = static_cast<std::vector<SiteProxyPair>*>(proxies);
        return vec->size();
    } catch (const std::exception& e) {
        Log_error("raft_rpc_proxies_size failed: %s", e.what());
        return 0;
    }
}

void raft_rpc_proxies_get_site_ids(RpcProxies_C proxies, siteid_t* out_site_ids, size_t size) {
    try {
        auto* vec = static_cast<std::vector<SiteProxyPair>*>(proxies);
        size_t i = 0;
        for (auto& pair : *vec) {
            if (i >= size) break;
            out_site_ids[i++] = pair.first;
        }
    } catch (const std::exception& e) {
        Log_error("raft_rpc_proxies_get_site_ids failed: %s", e.what());
    }
}

// =============================================================================
// P. RPC Proxy Manipulation (for disconnect/reconnect)
// =============================================================================

// Global static map to store backed-up RPC proxies (matching C++ implementation)
// Structure: partition_id -> locale_id -> vector of SiteProxyPair
static std::map<parid_t, std::map<uint32_t, std::vector<SiteProxyPair>>> _proxies_backup;

void raft_commo_clear_rpc_proxies(RaftCommo_C commo, parid_t par_id) {
    try {
        auto* raft_commo = static_cast<RaftCommo*>(commo);
        raft_commo->rpc_par_proxies_[par_id].clear();
    } catch (const std::exception& e) {
        Log_error("raft_commo_clear_rpc_proxies failed: %s", e.what());
    }
}

size_t raft_commo_get_rpc_proxy_count(RaftCommo_C commo, parid_t par_id) {
    try {
        auto* raft_commo = static_cast<RaftCommo*>(commo);
        return raft_commo->rpc_par_proxies_[par_id].size();
    } catch (const std::exception& e) {
        Log_error("raft_commo_get_rpc_proxy_count failed: %s", e.what());
        return 0;
    }
}

void raft_commo_save_rpc_proxies(RaftCommo_C commo, parid_t par_id, uint32_t loc_id) {
    try {
        auto* raft_commo = static_cast<RaftCommo*>(commo);

        // Initialize partition map if needed
        if (_proxies_backup.find(par_id) == _proxies_backup.end()) {
            _proxies_backup[par_id] = {};
        }

        // Save the proxies
        auto& proxies = raft_commo->rpc_par_proxies_[par_id];
        _proxies_backup[par_id][loc_id].clear();
        _proxies_backup[par_id][loc_id].insert(_proxies_backup[par_id][loc_id].end(), proxies.begin(), proxies.end());

        // Clear the actual proxies
        proxies.clear();
    } catch (const std::exception& e) {
        Log_error("raft_commo_save_rpc_proxies failed: %s", e.what());
    }
}

void raft_commo_restore_rpc_proxies(RaftCommo_C commo, parid_t par_id, uint32_t loc_id) {
    try {
        auto* raft_commo = static_cast<RaftCommo*>(commo);

        // Restore the proxies
        auto& backup = _proxies_backup[par_id][loc_id];
        raft_commo->rpc_par_proxies_[par_id].clear();
        raft_commo->rpc_par_proxies_[par_id].insert(raft_commo->rpc_par_proxies_[par_id].end(), backup.begin(), backup.end());

        // Clear the backup
        backup.clear();
    } catch (const std::exception& e) {
        Log_error("raft_commo_restore_rpc_proxies failed: %s", e.what());
    }
}

bool raft_commo_has_backup_proxies(parid_t par_id, uint32_t loc_id) {
    try {
        return _proxies_backup.find(par_id) != _proxies_backup.end() &&
               _proxies_backup[par_id].find(loc_id) != _proxies_backup[par_id].end() &&
               _proxies_backup[par_id][loc_id].size() > 0;
    } catch (const std::exception& e) {
        Log_error("raft_commo_has_backup_proxies failed: %s", e.what());
        return false;
    }
}

size_t raft_commo_get_backup_proxy_count(parid_t par_id, uint32_t loc_id) {
    try {
        if (_proxies_backup.find(par_id) == _proxies_backup.end() ||
            _proxies_backup[par_id].find(loc_id) == _proxies_backup[par_id].end()) {
            return 0;
        }
        return _proxies_backup[par_id][loc_id].size();
    } catch (const std::exception& e) {
        Log_error("raft_commo_get_backup_proxy_count failed: %s", e.what());
        return 0;
    }
}
