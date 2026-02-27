#pragma once

#include "rrr.hpp"

#include <errno.h>


namespace janus {

struct ValueTimesPair {
    rrr::i64 value;
    rrr::i64 times;
};

inline rrr::Marshal& operator <<(rrr::Marshal& m, const ValueTimesPair& o) {
    m << o.value;
    m << o.times;
    return m;
}

inline rrr::Marshal& operator >>(rrr::Marshal& m, ValueTimesPair& o) {
    m >> o.value;
    m >> o.times;
    return m;
}

struct DepId {
    std::string str;
    rrr::i64 id;
};

inline rrr::Marshal& operator <<(rrr::Marshal& m, const DepId& o) {
    m << o.str;
    m << o.id;
    return m;
}

inline rrr::Marshal& operator >>(rrr::Marshal& m, DepId& o) {
    m >> o.str;
    m >> o.id;
    return m;
}

struct TxnInfoRes {
    rrr::i32 start_txn;
    rrr::i32 total_txn;
    rrr::i32 total_try;
    rrr::i32 commit_txn;
    rrr::i32 num_exhausted;
    std::vector<double> this_latency;
    std::vector<double> last_latency;
    std::vector<double> attempt_latency;
    std::vector<double> interval_latency;
    std::vector<double> all_interval_latency;
    std::vector<rrr::i32> num_try;
};

inline rrr::Marshal& operator <<(rrr::Marshal& m, const TxnInfoRes& o) {
    m << o.start_txn;
    m << o.total_txn;
    m << o.total_try;
    m << o.commit_txn;
    m << o.num_exhausted;
    m << o.this_latency;
    m << o.last_latency;
    m << o.attempt_latency;
    m << o.interval_latency;
    m << o.all_interval_latency;
    m << o.num_try;
    return m;
}

inline rrr::Marshal& operator >>(rrr::Marshal& m, TxnInfoRes& o) {
    m >> o.start_txn;
    m >> o.total_txn;
    m >> o.total_try;
    m >> o.commit_txn;
    m >> o.num_exhausted;
    m >> o.this_latency;
    m >> o.last_latency;
    m >> o.attempt_latency;
    m >> o.interval_latency;
    m >> o.all_interval_latency;
    m >> o.num_try;
    return m;
}

struct ServerResponse {
    std::map<std::string, ValueTimesPair> statistics;
    double cpu_util;
    rrr::i64 r_cnt_sum;
    rrr::i64 r_cnt_num;
    rrr::i64 r_sz_sum;
    rrr::i64 r_sz_num;
};

inline rrr::Marshal& operator <<(rrr::Marshal& m, const ServerResponse& o) {
    m << o.statistics;
    m << o.cpu_util;
    m << o.r_cnt_sum;
    m << o.r_cnt_num;
    m << o.r_sz_sum;
    m << o.r_sz_num;
    return m;
}

inline rrr::Marshal& operator >>(rrr::Marshal& m, ServerResponse& o) {
    m >> o.statistics;
    m >> o.cpu_util;
    m >> o.r_cnt_sum;
    m >> o.r_cnt_num;
    m >> o.r_sz_sum;
    m >> o.r_sz_num;
    return m;
}

struct ClientResponse {
    std::map<rrr::i32, TxnInfoRes> txn_info;
    rrr::i64 run_sec;
    rrr::i64 run_nsec;
    rrr::i64 period_sec;
    rrr::i64 period_nsec;
    rrr::i32 is_finish;
    rrr::i64 n_asking;
};

inline rrr::Marshal& operator <<(rrr::Marshal& m, const ClientResponse& o) {
    m << o.txn_info;
    m << o.run_sec;
    m << o.run_nsec;
    m << o.period_sec;
    m << o.period_nsec;
    m << o.is_finish;
    m << o.n_asking;
    return m;
}

inline rrr::Marshal& operator >>(rrr::Marshal& m, ClientResponse& o) {
    m >> o.txn_info;
    m >> o.run_sec;
    m >> o.run_nsec;
    m >> o.period_sec;
    m >> o.period_nsec;
    m >> o.is_finish;
    m >> o.n_asking;
    return m;
}

struct Profiling {
    double cpu_util;
    double tx_util;
    double rx_util;
    double mem_util;
};

inline rrr::Marshal& operator <<(rrr::Marshal& m, const Profiling& o) {
    m << o.cpu_util;
    m << o.tx_util;
    m << o.rx_util;
    m << o.mem_util;
    return m;
}

inline rrr::Marshal& operator >>(rrr::Marshal& m, Profiling& o) {
    m >> o.cpu_util;
    m >> o.tx_util;
    m >> o.rx_util;
    m >> o.mem_util;
    return m;
}

struct TxDispatchRequest {
    rrr::i32 id;
    rrr::i32 tx_type;
    std::vector<Value> input;
};

inline rrr::Marshal& operator <<(rrr::Marshal& m, const TxDispatchRequest& o) {
    m << o.id;
    m << o.tx_type;
    m << o.input;
    return m;
}

inline rrr::Marshal& operator >>(rrr::Marshal& m, TxDispatchRequest& o) {
    m >> o.id;
    m >> o.tx_type;
    m >> o.input;
    return m;
}

struct TxnDispatchResponse {
};

inline rrr::Marshal& operator <<(rrr::Marshal& m, const TxnDispatchResponse& o) {
    return m;
}

inline rrr::Marshal& operator >>(rrr::Marshal& m, TxnDispatchResponse& o) {
    return m;
}

class MultiPaxosService: public rrr::Service {
public:
    enum {
        FORWARD = 0x6d7b1379,
        PREPARE = 0x6ad11c9e,
        ACCEPT = 0x1b8207f9,
        DECIDE = 0x12fc294a,
        HEARTBEAT = 0x2dcceee8,
        FORWARDTOLEARNERSERVER = 0x6ff86ca5,
        BULKPREPARE = 0x5ef1ffea,
        BULKACCEPT = 0x66756424,
        BULKPREPARE2 = 0x5d57a0f6,
        SYNCLOG = 0x1ae1d4e7,
        SYNCCOMMIT = 0x69db0060,
        SYNCNOOPS = 0x13fdf676,
        BULKDECIDE = 0x51c81f0c,
    };
    int __reg_to__(rrr::Server* svr) {
        int ret = 0;
        if ((ret = svr->reg(FORWARD, this, &MultiPaxosService::__Forward__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(PREPARE, this, &MultiPaxosService::__Prepare__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(ACCEPT, this, &MultiPaxosService::__Accept__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(DECIDE, this, &MultiPaxosService::__Decide__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(HEARTBEAT, this, &MultiPaxosService::__Heartbeat__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(FORWARDTOLEARNERSERVER, this, &MultiPaxosService::__ForwardToLearnerServer__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(BULKPREPARE, this, &MultiPaxosService::__BulkPrepare__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(BULKACCEPT, this, &MultiPaxosService::__BulkAccept__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(BULKPREPARE2, this, &MultiPaxosService::__BulkPrepare2__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(SYNCLOG, this, &MultiPaxosService::__SyncLog__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(SYNCCOMMIT, this, &MultiPaxosService::__SyncCommit__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(SYNCNOOPS, this, &MultiPaxosService::__SyncNoOps__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(BULKDECIDE, this, &MultiPaxosService::__BulkDecide__wrapper__)) != 0) {
            goto err;
        }
        return 0;
    err:
        svr->unreg(FORWARD);
        svr->unreg(PREPARE);
        svr->unreg(ACCEPT);
        svr->unreg(DECIDE);
        svr->unreg(HEARTBEAT);
        svr->unreg(FORWARDTOLEARNERSERVER);
        svr->unreg(BULKPREPARE);
        svr->unreg(BULKACCEPT);
        svr->unreg(BULKPREPARE2);
        svr->unreg(SYNCLOG);
        svr->unreg(SYNCCOMMIT);
        svr->unreg(SYNCNOOPS);
        svr->unreg(BULKDECIDE);
        return ret;
    }
    // these RPC handler functions need to be implemented by user
    // for 'raw' handlers, req is rusty::Box (auto-cleaned); weak_ptr requires lock() before use
    virtual void Forward(const MarshallDeputy& cmd, const uint64_t& dep_id, uint64_t* coro_id, rrr::DeferredReply* defer) = 0;
    virtual void Prepare(const uint64_t& slot, const ballot_t& ballot, ballot_t* max_ballot, uint64_t* coro_id, rrr::DeferredReply* defer) = 0;
    virtual void Accept(const uint64_t& slot, const uint64_t& time, const ballot_t& ballot, const MarshallDeputy& cmd, ballot_t* max_ballot, uint64_t* coro_id, rrr::DeferredReply* defer) = 0;
    virtual void Decide(const uint64_t& slot, const ballot_t& ballot, const MarshallDeputy& cmd, rrr::DeferredReply* defer) = 0;
    virtual void Heartbeat(const MarshallDeputy& cmd, rrr::i32* ballot, rrr::i32* val, rrr::DeferredReply* defer) = 0;
    virtual void ForwardToLearnerServer(const rrr::i32& par_id, const uint64_t& slot, const ballot_t& ballot, const MarshallDeputy& cmd, uint64_t* ret_slot, ballot_t* ret_ballot, rrr::DeferredReply* defer) = 0;
    virtual void BulkPrepare(const MarshallDeputy& cmd, rrr::i32* ballot, rrr::i32* val, rrr::DeferredReply* defer) = 0;
    virtual void BulkAccept(const MarshallDeputy& cmd, rrr::i32* ballot, rrr::i32* val, rrr::DeferredReply* defer) = 0;
    virtual void BulkPrepare2(const MarshallDeputy& cmd, rrr::i32* ballot, rrr::i32* val, MarshallDeputy* ret, rrr::DeferredReply* defer) = 0;
    virtual void SyncLog(const MarshallDeputy& cmd, rrr::i32* ballot, rrr::i32* val, MarshallDeputy* ret, rrr::DeferredReply* defer) = 0;
    virtual void SyncCommit(const MarshallDeputy& cmd, rrr::i32* ballot, rrr::i32* val, rrr::DeferredReply* defer) = 0;
    virtual void SyncNoOps(const MarshallDeputy& cmd, rrr::i32* ballot, rrr::i32* val, rrr::DeferredReply* defer) = 0;
    virtual void BulkDecide(const MarshallDeputy& cmd, rrr::i32* ballot, rrr::i32* val, rrr::DeferredReply* defer) = 0;
private:
    void __Forward__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        MarshallDeputy* in_0 = new MarshallDeputy;
        req->m >> *in_0;
        uint64_t* in_1 = new uint64_t;
        req->m >> *in_1;
        uint64_t* out_0 = new uint64_t;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
            }
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete in_1;
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->Forward(*in_0, *in_1, out_0, __defer__);
    }
    void __Prepare__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        uint64_t* in_0 = new uint64_t;
        req->m >> *in_0;
        ballot_t* in_1 = new ballot_t;
        req->m >> *in_1;
        ballot_t* out_0 = new ballot_t;
        uint64_t* out_1 = new uint64_t;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
                *sconn << *out_1;
            }
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete in_1;
            delete out_0;
            delete out_1;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->Prepare(*in_0, *in_1, out_0, out_1, __defer__);
    }
    void __Accept__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        uint64_t* in_0 = new uint64_t;
        req->m >> *in_0;
        uint64_t* in_1 = new uint64_t;
        req->m >> *in_1;
        ballot_t* in_2 = new ballot_t;
        req->m >> *in_2;
        MarshallDeputy* in_3 = new MarshallDeputy;
        req->m >> *in_3;
        ballot_t* out_0 = new ballot_t;
        uint64_t* out_1 = new uint64_t;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
                *sconn << *out_1;
            }
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete in_1;
            delete in_2;
            delete in_3;
            delete out_0;
            delete out_1;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->Accept(*in_0, *in_1, *in_2, *in_3, out_0, out_1, __defer__);
    }
    void __Decide__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        uint64_t* in_0 = new uint64_t;
        req->m >> *in_0;
        ballot_t* in_1 = new ballot_t;
        req->m >> *in_1;
        MarshallDeputy* in_2 = new MarshallDeputy;
        req->m >> *in_2;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
            }
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete in_1;
            delete in_2;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->Decide(*in_0, *in_1, *in_2, __defer__);
    }
    void __Heartbeat__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        MarshallDeputy* in_0 = new MarshallDeputy;
        req->m >> *in_0;
        rrr::i32* out_0 = new rrr::i32;
        rrr::i32* out_1 = new rrr::i32;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
                *sconn << *out_1;
            }
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
            delete out_1;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->Heartbeat(*in_0, out_0, out_1, __defer__);
    }
    void __ForwardToLearnerServer__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        rrr::i32* in_0 = new rrr::i32;
        req->m >> *in_0;
        uint64_t* in_1 = new uint64_t;
        req->m >> *in_1;
        ballot_t* in_2 = new ballot_t;
        req->m >> *in_2;
        MarshallDeputy* in_3 = new MarshallDeputy;
        req->m >> *in_3;
        uint64_t* out_0 = new uint64_t;
        ballot_t* out_1 = new ballot_t;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
                *sconn << *out_1;
            }
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete in_1;
            delete in_2;
            delete in_3;
            delete out_0;
            delete out_1;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->ForwardToLearnerServer(*in_0, *in_1, *in_2, *in_3, out_0, out_1, __defer__);
    }
    void __BulkPrepare__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        MarshallDeputy* in_0 = new MarshallDeputy;
        req->m >> *in_0;
        rrr::i32* out_0 = new rrr::i32;
        rrr::i32* out_1 = new rrr::i32;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
                *sconn << *out_1;
            }
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
            delete out_1;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->BulkPrepare(*in_0, out_0, out_1, __defer__);
    }
    void __BulkAccept__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        MarshallDeputy* in_0 = new MarshallDeputy;
        req->m >> *in_0;
        rrr::i32* out_0 = new rrr::i32;
        rrr::i32* out_1 = new rrr::i32;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
                *sconn << *out_1;
            }
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
            delete out_1;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->BulkAccept(*in_0, out_0, out_1, __defer__);
    }
    void __BulkPrepare2__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        MarshallDeputy* in_0 = new MarshallDeputy;
        req->m >> *in_0;
        rrr::i32* out_0 = new rrr::i32;
        rrr::i32* out_1 = new rrr::i32;
        MarshallDeputy* out_2 = new MarshallDeputy;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
                *sconn << *out_1;
                *sconn << *out_2;
            }
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
            delete out_1;
            delete out_2;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->BulkPrepare2(*in_0, out_0, out_1, out_2, __defer__);
    }
    void __SyncLog__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        MarshallDeputy* in_0 = new MarshallDeputy;
        req->m >> *in_0;
        rrr::i32* out_0 = new rrr::i32;
        rrr::i32* out_1 = new rrr::i32;
        MarshallDeputy* out_2 = new MarshallDeputy;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
                *sconn << *out_1;
                *sconn << *out_2;
            }
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
            delete out_1;
            delete out_2;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->SyncLog(*in_0, out_0, out_1, out_2, __defer__);
    }
    void __SyncCommit__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        MarshallDeputy* in_0 = new MarshallDeputy;
        req->m >> *in_0;
        rrr::i32* out_0 = new rrr::i32;
        rrr::i32* out_1 = new rrr::i32;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
                *sconn << *out_1;
            }
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
            delete out_1;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->SyncCommit(*in_0, out_0, out_1, __defer__);
    }
    void __SyncNoOps__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        MarshallDeputy* in_0 = new MarshallDeputy;
        req->m >> *in_0;
        rrr::i32* out_0 = new rrr::i32;
        rrr::i32* out_1 = new rrr::i32;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
                *sconn << *out_1;
            }
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
            delete out_1;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->SyncNoOps(*in_0, out_0, out_1, __defer__);
    }
    void __BulkDecide__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        MarshallDeputy* in_0 = new MarshallDeputy;
        req->m >> *in_0;
        rrr::i32* out_0 = new rrr::i32;
        rrr::i32* out_1 = new rrr::i32;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
                *sconn << *out_1;
            }
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
            delete out_1;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->BulkDecide(*in_0, out_0, out_1, __defer__);
    }
};

class MultiPaxosProxy {
protected:
    rrr::Client* __cl__;
public:
    MultiPaxosProxy(rrr::Client* cl): __cl__(cl) { }
    rrr::Future* async_Forward(const MarshallDeputy& cmd, const uint64_t& dep_id, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(MultiPaxosService::FORWARD, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << cmd;
            *__cl__ << dep_id;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 Forward(const MarshallDeputy& cmd, const uint64_t& dep_id, uint64_t* coro_id) {
        rrr::Future* __fu__ = this->async_Forward(cmd, dep_id);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *coro_id;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_Prepare(const uint64_t& slot, const ballot_t& ballot, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(MultiPaxosService::PREPARE, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << slot;
            *__cl__ << ballot;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 Prepare(const uint64_t& slot, const ballot_t& ballot, ballot_t* max_ballot, uint64_t* coro_id) {
        rrr::Future* __fu__ = this->async_Prepare(slot, ballot);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *max_ballot;
            __fu__->get_reply() >> *coro_id;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_Accept(const uint64_t& slot, const uint64_t& time, const ballot_t& ballot, const MarshallDeputy& cmd, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(MultiPaxosService::ACCEPT, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << slot;
            *__cl__ << time;
            *__cl__ << ballot;
            *__cl__ << cmd;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 Accept(const uint64_t& slot, const uint64_t& time, const ballot_t& ballot, const MarshallDeputy& cmd, ballot_t* max_ballot, uint64_t* coro_id) {
        rrr::Future* __fu__ = this->async_Accept(slot, time, ballot, cmd);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *max_ballot;
            __fu__->get_reply() >> *coro_id;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_Decide(const uint64_t& slot, const ballot_t& ballot, const MarshallDeputy& cmd, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(MultiPaxosService::DECIDE, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << slot;
            *__cl__ << ballot;
            *__cl__ << cmd;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 Decide(const uint64_t& slot, const ballot_t& ballot, const MarshallDeputy& cmd) {
        rrr::Future* __fu__ = this->async_Decide(slot, ballot, cmd);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_Heartbeat(const MarshallDeputy& cmd, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(MultiPaxosService::HEARTBEAT, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << cmd;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 Heartbeat(const MarshallDeputy& cmd, rrr::i32* ballot, rrr::i32* val) {
        rrr::Future* __fu__ = this->async_Heartbeat(cmd);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *ballot;
            __fu__->get_reply() >> *val;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_ForwardToLearnerServer(const rrr::i32& par_id, const uint64_t& slot, const ballot_t& ballot, const MarshallDeputy& cmd, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(MultiPaxosService::FORWARDTOLEARNERSERVER, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << par_id;
            *__cl__ << slot;
            *__cl__ << ballot;
            *__cl__ << cmd;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 ForwardToLearnerServer(const rrr::i32& par_id, const uint64_t& slot, const ballot_t& ballot, const MarshallDeputy& cmd, uint64_t* ret_slot, ballot_t* ret_ballot) {
        rrr::Future* __fu__ = this->async_ForwardToLearnerServer(par_id, slot, ballot, cmd);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *ret_slot;
            __fu__->get_reply() >> *ret_ballot;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_BulkPrepare(const MarshallDeputy& cmd, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(MultiPaxosService::BULKPREPARE, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << cmd;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 BulkPrepare(const MarshallDeputy& cmd, rrr::i32* ballot, rrr::i32* val) {
        rrr::Future* __fu__ = this->async_BulkPrepare(cmd);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *ballot;
            __fu__->get_reply() >> *val;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_BulkAccept(const MarshallDeputy& cmd, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(MultiPaxosService::BULKACCEPT, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << cmd;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 BulkAccept(const MarshallDeputy& cmd, rrr::i32* ballot, rrr::i32* val) {
        rrr::Future* __fu__ = this->async_BulkAccept(cmd);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *ballot;
            __fu__->get_reply() >> *val;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_BulkPrepare2(const MarshallDeputy& cmd, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(MultiPaxosService::BULKPREPARE2, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << cmd;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 BulkPrepare2(const MarshallDeputy& cmd, rrr::i32* ballot, rrr::i32* val, MarshallDeputy* ret) {
        rrr::Future* __fu__ = this->async_BulkPrepare2(cmd);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *ballot;
            __fu__->get_reply() >> *val;
            __fu__->get_reply() >> *ret;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_SyncLog(const MarshallDeputy& cmd, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(MultiPaxosService::SYNCLOG, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << cmd;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 SyncLog(const MarshallDeputy& cmd, rrr::i32* ballot, rrr::i32* val, MarshallDeputy* ret) {
        rrr::Future* __fu__ = this->async_SyncLog(cmd);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *ballot;
            __fu__->get_reply() >> *val;
            __fu__->get_reply() >> *ret;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_SyncCommit(const MarshallDeputy& cmd, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(MultiPaxosService::SYNCCOMMIT, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << cmd;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 SyncCommit(const MarshallDeputy& cmd, rrr::i32* ballot, rrr::i32* val) {
        rrr::Future* __fu__ = this->async_SyncCommit(cmd);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *ballot;
            __fu__->get_reply() >> *val;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_SyncNoOps(const MarshallDeputy& cmd, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(MultiPaxosService::SYNCNOOPS, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << cmd;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 SyncNoOps(const MarshallDeputy& cmd, rrr::i32* ballot, rrr::i32* val) {
        rrr::Future* __fu__ = this->async_SyncNoOps(cmd);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *ballot;
            __fu__->get_reply() >> *val;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_BulkDecide(const MarshallDeputy& cmd, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(MultiPaxosService::BULKDECIDE, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << cmd;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 BulkDecide(const MarshallDeputy& cmd, rrr::i32* ballot, rrr::i32* val) {
        rrr::Future* __fu__ = this->async_BulkDecide(cmd);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *ballot;
            __fu__->get_reply() >> *val;
        }
        __fu__->release();
        return __ret__;
    }
};

class CopilotService: public rrr::Service {
public:
    enum {
        FORWARD = 0x67b298d9,
        PREPARE = 0x3be61dc0,
        FASTACCEPT = 0x150ad43e,
        ACCEPT = 0x5c6ace29,
        COMMIT = 0x66a9bcb9,
    };
    int __reg_to__(rrr::Server* svr) {
        int ret = 0;
        if ((ret = svr->reg(FORWARD, this, &CopilotService::__Forward__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(PREPARE, this, &CopilotService::__Prepare__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(FASTACCEPT, this, &CopilotService::__FastAccept__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(ACCEPT, this, &CopilotService::__Accept__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(COMMIT, this, &CopilotService::__Commit__wrapper__)) != 0) {
            goto err;
        }
        return 0;
    err:
        svr->unreg(FORWARD);
        svr->unreg(PREPARE);
        svr->unreg(FASTACCEPT);
        svr->unreg(ACCEPT);
        svr->unreg(COMMIT);
        return ret;
    }
    // these RPC handler functions need to be implemented by user
    // for 'raw' handlers, req is rusty::Box (auto-cleaned); weak_ptr requires lock() before use
    virtual void Forward(const MarshallDeputy& cmd, rrr::DeferredReply* defer) = 0;
    virtual void Prepare(const uint8_t& is_pilot, const uint64_t& slot, const ballot_t& ballot, const DepId& dep_id, MarshallDeputy* ret_cmd, ballot_t* max_ballot, uint64_t* dep, status_t* status, rrr::DeferredReply* defer) = 0;
    virtual void FastAccept(const uint8_t& is_pilot, const uint64_t& slot, const ballot_t& ballot, const uint64_t& dep, const MarshallDeputy& cmd, const DepId& dep_id, ballot_t* max_ballot, uint64_t* ret_dep, rrr::DeferredReply* defer) = 0;
    virtual void Accept(const uint8_t& is_pilot, const uint64_t& slot, const ballot_t& ballot, const uint64_t& dep, const MarshallDeputy& cmd, const DepId& dep_id, ballot_t* max_ballot, rrr::DeferredReply* defer) = 0;
    virtual void Commit(const uint8_t& is_pilot, const uint64_t& slot, const uint64_t& dep, const MarshallDeputy& cmd, rrr::DeferredReply* defer) = 0;
private:
    void __Forward__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        MarshallDeputy* in_0 = new MarshallDeputy;
        req->m >> *in_0;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
            }
        };
        auto __cleanup__ = [=] {
            delete in_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->Forward(*in_0, __defer__);
    }
    void __Prepare__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        uint8_t* in_0 = new uint8_t;
        req->m >> *in_0;
        uint64_t* in_1 = new uint64_t;
        req->m >> *in_1;
        ballot_t* in_2 = new ballot_t;
        req->m >> *in_2;
        DepId* in_3 = new DepId;
        req->m >> *in_3;
        MarshallDeputy* out_0 = new MarshallDeputy;
        ballot_t* out_1 = new ballot_t;
        uint64_t* out_2 = new uint64_t;
        status_t* out_3 = new status_t;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
                *sconn << *out_1;
                *sconn << *out_2;
                *sconn << *out_3;
            }
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete in_1;
            delete in_2;
            delete in_3;
            delete out_0;
            delete out_1;
            delete out_2;
            delete out_3;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->Prepare(*in_0, *in_1, *in_2, *in_3, out_0, out_1, out_2, out_3, __defer__);
    }
    void __FastAccept__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        uint8_t* in_0 = new uint8_t;
        req->m >> *in_0;
        uint64_t* in_1 = new uint64_t;
        req->m >> *in_1;
        ballot_t* in_2 = new ballot_t;
        req->m >> *in_2;
        uint64_t* in_3 = new uint64_t;
        req->m >> *in_3;
        MarshallDeputy* in_4 = new MarshallDeputy;
        req->m >> *in_4;
        DepId* in_5 = new DepId;
        req->m >> *in_5;
        ballot_t* out_0 = new ballot_t;
        uint64_t* out_1 = new uint64_t;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
                *sconn << *out_1;
            }
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete in_1;
            delete in_2;
            delete in_3;
            delete in_4;
            delete in_5;
            delete out_0;
            delete out_1;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->FastAccept(*in_0, *in_1, *in_2, *in_3, *in_4, *in_5, out_0, out_1, __defer__);
    }
    void __Accept__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        uint8_t* in_0 = new uint8_t;
        req->m >> *in_0;
        uint64_t* in_1 = new uint64_t;
        req->m >> *in_1;
        ballot_t* in_2 = new ballot_t;
        req->m >> *in_2;
        uint64_t* in_3 = new uint64_t;
        req->m >> *in_3;
        MarshallDeputy* in_4 = new MarshallDeputy;
        req->m >> *in_4;
        DepId* in_5 = new DepId;
        req->m >> *in_5;
        ballot_t* out_0 = new ballot_t;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
            }
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete in_1;
            delete in_2;
            delete in_3;
            delete in_4;
            delete in_5;
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->Accept(*in_0, *in_1, *in_2, *in_3, *in_4, *in_5, out_0, __defer__);
    }
    void __Commit__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        uint8_t* in_0 = new uint8_t;
        req->m >> *in_0;
        uint64_t* in_1 = new uint64_t;
        req->m >> *in_1;
        uint64_t* in_2 = new uint64_t;
        req->m >> *in_2;
        MarshallDeputy* in_3 = new MarshallDeputy;
        req->m >> *in_3;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
            }
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete in_1;
            delete in_2;
            delete in_3;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->Commit(*in_0, *in_1, *in_2, *in_3, __defer__);
    }
};

class CopilotProxy {
protected:
    rrr::Client* __cl__;
public:
    CopilotProxy(rrr::Client* cl): __cl__(cl) { }
    rrr::Future* async_Forward(const MarshallDeputy& cmd, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(CopilotService::FORWARD, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << cmd;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 Forward(const MarshallDeputy& cmd) {
        rrr::Future* __fu__ = this->async_Forward(cmd);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_Prepare(const uint8_t& is_pilot, const uint64_t& slot, const ballot_t& ballot, const DepId& dep_id, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(CopilotService::PREPARE, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << is_pilot;
            *__cl__ << slot;
            *__cl__ << ballot;
            *__cl__ << dep_id;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 Prepare(const uint8_t& is_pilot, const uint64_t& slot, const ballot_t& ballot, const DepId& dep_id, MarshallDeputy* ret_cmd, ballot_t* max_ballot, uint64_t* dep, status_t* status) {
        rrr::Future* __fu__ = this->async_Prepare(is_pilot, slot, ballot, dep_id);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *ret_cmd;
            __fu__->get_reply() >> *max_ballot;
            __fu__->get_reply() >> *dep;
            __fu__->get_reply() >> *status;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_FastAccept(const uint8_t& is_pilot, const uint64_t& slot, const ballot_t& ballot, const uint64_t& dep, const MarshallDeputy& cmd, const DepId& dep_id, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(CopilotService::FASTACCEPT, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << is_pilot;
            *__cl__ << slot;
            *__cl__ << ballot;
            *__cl__ << dep;
            *__cl__ << cmd;
            *__cl__ << dep_id;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 FastAccept(const uint8_t& is_pilot, const uint64_t& slot, const ballot_t& ballot, const uint64_t& dep, const MarshallDeputy& cmd, const DepId& dep_id, ballot_t* max_ballot, uint64_t* ret_dep) {
        rrr::Future* __fu__ = this->async_FastAccept(is_pilot, slot, ballot, dep, cmd, dep_id);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *max_ballot;
            __fu__->get_reply() >> *ret_dep;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_Accept(const uint8_t& is_pilot, const uint64_t& slot, const ballot_t& ballot, const uint64_t& dep, const MarshallDeputy& cmd, const DepId& dep_id, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(CopilotService::ACCEPT, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << is_pilot;
            *__cl__ << slot;
            *__cl__ << ballot;
            *__cl__ << dep;
            *__cl__ << cmd;
            *__cl__ << dep_id;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 Accept(const uint8_t& is_pilot, const uint64_t& slot, const ballot_t& ballot, const uint64_t& dep, const MarshallDeputy& cmd, const DepId& dep_id, ballot_t* max_ballot) {
        rrr::Future* __fu__ = this->async_Accept(is_pilot, slot, ballot, dep, cmd, dep_id);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *max_ballot;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_Commit(const uint8_t& is_pilot, const uint64_t& slot, const uint64_t& dep, const MarshallDeputy& cmd, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(CopilotService::COMMIT, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << is_pilot;
            *__cl__ << slot;
            *__cl__ << dep;
            *__cl__ << cmd;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 Commit(const uint8_t& is_pilot, const uint64_t& slot, const uint64_t& dep, const MarshallDeputy& cmd) {
        rrr::Future* __fu__ = this->async_Commit(is_pilot, slot, dep, cmd);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        __fu__->release();
        return __ret__;
    }
};

class ClassicService: public rrr::Service {
public:
    enum {
        MSGSTRING = 0x4c9a50b4,
        MSGMARSHALL = 0x66d3a271,
        REELECT = 0x54f864ab,
        DISPATCH = 0x16879047,
        PREPARE = 0x41ec30d9,
        COMMIT = 0x2dddcd34,
        ABORT = 0x42bd6bdc,
        EARLYABORT = 0x53b7ea58,
        UPGRADEEPOCH = 0x3044d7bc,
        TRUNCATEEPOCH = 0x5eba785c,
        ISLEADER = 0x5e86b097,
        ISFPGALEADER = 0x4cc67585,
        SIMPLECMD = 0x41f2a97e,
        FAILOVERTRIG = 0x4cd8d42f,
        RPC_NULL = 0x671be5f9,
    };
    int __reg_to__(rrr::Server* svr) {
        int ret = 0;
        if ((ret = svr->reg(MSGSTRING, this, &ClassicService::__MsgString__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(MSGMARSHALL, this, &ClassicService::__MsgMarshall__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(REELECT, this, &ClassicService::__ReElect__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(DISPATCH, this, &ClassicService::__Dispatch__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(PREPARE, this, &ClassicService::__Prepare__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(COMMIT, this, &ClassicService::__Commit__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(ABORT, this, &ClassicService::__Abort__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(EARLYABORT, this, &ClassicService::__EarlyAbort__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(UPGRADEEPOCH, this, &ClassicService::__UpgradeEpoch__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(TRUNCATEEPOCH, this, &ClassicService::__TruncateEpoch__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(ISLEADER, this, &ClassicService::__IsLeader__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(ISFPGALEADER, this, &ClassicService::__IsFPGALeader__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(SIMPLECMD, this, &ClassicService::__SimpleCmd__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(FAILOVERTRIG, this, &ClassicService::__FailOverTrig__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(RPC_NULL, this, &ClassicService::__rpc_null__wrapper__)) != 0) {
            goto err;
        }
        return 0;
    err:
        svr->unreg(MSGSTRING);
        svr->unreg(MSGMARSHALL);
        svr->unreg(REELECT);
        svr->unreg(DISPATCH);
        svr->unreg(PREPARE);
        svr->unreg(COMMIT);
        svr->unreg(ABORT);
        svr->unreg(EARLYABORT);
        svr->unreg(UPGRADEEPOCH);
        svr->unreg(TRUNCATEEPOCH);
        svr->unreg(ISLEADER);
        svr->unreg(ISFPGALEADER);
        svr->unreg(SIMPLECMD);
        svr->unreg(FAILOVERTRIG);
        svr->unreg(RPC_NULL);
        return ret;
    }
    // these RPC handler functions need to be implemented by user
    // for 'raw' handlers, req is rusty::Box (auto-cleaned); weak_ptr requires lock() before use
    virtual void MsgString(const std::string& arg, std::string* ret, rrr::DeferredReply* defer) = 0;
    virtual void MsgMarshall(const MarshallDeputy& arg, MarshallDeputy* ret, rrr::DeferredReply* defer) = 0;
    virtual void ReElect(bool_t* success, rrr::DeferredReply* defer) = 0;
    virtual void Dispatch(const rrr::i64& tid, const DepId& dep_id, const MarshallDeputy& cmd, rrr::i32* res, TxnOutput* output, uint64_t* coro_id, rrr::DeferredReply* defer) = 0;
    virtual void Prepare(const rrr::i64& tid, const std::vector<rrr::i32>& sids, const DepId& dep_id, rrr::i32* res, bool_t* slow, uint64_t* coro_id, rrr::DeferredReply* defer) = 0;
    virtual void Commit(const rrr::i64& tid, const DepId& dep_id, rrr::i32* res, bool_t* slow, uint64_t* coro_id, Profiling* profile, rrr::DeferredReply* defer) = 0;
    virtual void Abort(const rrr::i64& tid, const DepId& dep_id, rrr::i32* res, bool_t* slow, uint64_t* coro_id, Profiling* profile, rrr::DeferredReply* defer) = 0;
    virtual void EarlyAbort(const rrr::i64& tid, rrr::i32* res, rrr::DeferredReply* defer) = 0;
    virtual void UpgradeEpoch(const uint32_t& curr_epoch, int32_t* res, rrr::DeferredReply* defer) = 0;
    virtual void TruncateEpoch(const uint32_t& old_epoch, rrr::DeferredReply* defer) = 0;
    virtual void IsLeader(const locid_t& cur_pause, bool_t* is_leader, rrr::DeferredReply* defer) = 0;
    virtual void IsFPGALeader(const locid_t& cur_pause, bool_t* is_leader, rrr::DeferredReply* defer) = 0;
    virtual void SimpleCmd(const SimpleCommand& cmd, rrr::i32* res, rrr::DeferredReply* defer) = 0;
    virtual void FailOverTrig(const bool_t& pause, rrr::i32* res, rrr::DeferredReply* defer) = 0;
    virtual void rpc_null(rrr::DeferredReply* defer) = 0;
private:
    void __MsgString__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        std::string* in_0 = new std::string;
        req->m >> *in_0;
        std::string* out_0 = new std::string;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
            }
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->MsgString(*in_0, out_0, __defer__);
    }
    void __MsgMarshall__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        MarshallDeputy* in_0 = new MarshallDeputy;
        req->m >> *in_0;
        MarshallDeputy* out_0 = new MarshallDeputy;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
            }
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->MsgMarshall(*in_0, out_0, __defer__);
    }
    void __ReElect__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        bool_t* out_0 = new bool_t;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
            }
        };
        auto __cleanup__ = [=] {
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->ReElect(out_0, __defer__);
    }
    void __Dispatch__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        rrr::i64* in_0 = new rrr::i64;
        req->m >> *in_0;
        DepId* in_1 = new DepId;
        req->m >> *in_1;
        MarshallDeputy* in_2 = new MarshallDeputy;
        req->m >> *in_2;
        rrr::i32* out_0 = new rrr::i32;
        TxnOutput* out_1 = new TxnOutput;
        uint64_t* out_2 = new uint64_t;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
                *sconn << *out_1;
                *sconn << *out_2;
            }
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete in_1;
            delete in_2;
            delete out_0;
            delete out_1;
            delete out_2;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->Dispatch(*in_0, *in_1, *in_2, out_0, out_1, out_2, __defer__);
    }
    void __Prepare__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        rrr::i64* in_0 = new rrr::i64;
        req->m >> *in_0;
        std::vector<rrr::i32>* in_1 = new std::vector<rrr::i32>;
        req->m >> *in_1;
        DepId* in_2 = new DepId;
        req->m >> *in_2;
        rrr::i32* out_0 = new rrr::i32;
        bool_t* out_1 = new bool_t;
        uint64_t* out_2 = new uint64_t;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
                *sconn << *out_1;
                *sconn << *out_2;
            }
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete in_1;
            delete in_2;
            delete out_0;
            delete out_1;
            delete out_2;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->Prepare(*in_0, *in_1, *in_2, out_0, out_1, out_2, __defer__);
    }
    void __Commit__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        rrr::i64* in_0 = new rrr::i64;
        req->m >> *in_0;
        DepId* in_1 = new DepId;
        req->m >> *in_1;
        rrr::i32* out_0 = new rrr::i32;
        bool_t* out_1 = new bool_t;
        uint64_t* out_2 = new uint64_t;
        Profiling* out_3 = new Profiling;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
                *sconn << *out_1;
                *sconn << *out_2;
                *sconn << *out_3;
            }
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete in_1;
            delete out_0;
            delete out_1;
            delete out_2;
            delete out_3;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->Commit(*in_0, *in_1, out_0, out_1, out_2, out_3, __defer__);
    }
    void __Abort__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        rrr::i64* in_0 = new rrr::i64;
        req->m >> *in_0;
        DepId* in_1 = new DepId;
        req->m >> *in_1;
        rrr::i32* out_0 = new rrr::i32;
        bool_t* out_1 = new bool_t;
        uint64_t* out_2 = new uint64_t;
        Profiling* out_3 = new Profiling;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
                *sconn << *out_1;
                *sconn << *out_2;
                *sconn << *out_3;
            }
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete in_1;
            delete out_0;
            delete out_1;
            delete out_2;
            delete out_3;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->Abort(*in_0, *in_1, out_0, out_1, out_2, out_3, __defer__);
    }
    void __EarlyAbort__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        rrr::i64* in_0 = new rrr::i64;
        req->m >> *in_0;
        rrr::i32* out_0 = new rrr::i32;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
            }
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->EarlyAbort(*in_0, out_0, __defer__);
    }
    void __UpgradeEpoch__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        uint32_t* in_0 = new uint32_t;
        req->m >> *in_0;
        int32_t* out_0 = new int32_t;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
            }
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->UpgradeEpoch(*in_0, out_0, __defer__);
    }
    void __TruncateEpoch__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        uint32_t* in_0 = new uint32_t;
        req->m >> *in_0;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
            }
        };
        auto __cleanup__ = [=] {
            delete in_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->TruncateEpoch(*in_0, __defer__);
    }
    void __IsLeader__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        locid_t* in_0 = new locid_t;
        req->m >> *in_0;
        bool_t* out_0 = new bool_t;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
            }
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->IsLeader(*in_0, out_0, __defer__);
    }
    void __IsFPGALeader__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        locid_t* in_0 = new locid_t;
        req->m >> *in_0;
        bool_t* out_0 = new bool_t;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
            }
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->IsFPGALeader(*in_0, out_0, __defer__);
    }
    void __SimpleCmd__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        SimpleCommand* in_0 = new SimpleCommand;
        req->m >> *in_0;
        rrr::i32* out_0 = new rrr::i32;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
            }
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->SimpleCmd(*in_0, out_0, __defer__);
    }
    void __FailOverTrig__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        bool_t* in_0 = new bool_t;
        req->m >> *in_0;
        rrr::i32* out_0 = new rrr::i32;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
            }
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->FailOverTrig(*in_0, out_0, __defer__);
    }
    void __rpc_null__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
            }
        };
        auto __cleanup__ = [=] {
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->rpc_null(__defer__);
    }
};

class ClassicProxy {
protected:
    rrr::Client* __cl__;
public:
    ClassicProxy(rrr::Client* cl): __cl__(cl) { }
    rrr::Future* async_MsgString(const std::string& arg, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::MSGSTRING, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << arg;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 MsgString(const std::string& arg, std::string* ret) {
        rrr::Future* __fu__ = this->async_MsgString(arg);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *ret;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_MsgMarshall(const MarshallDeputy& arg, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::MSGMARSHALL, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << arg;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 MsgMarshall(const MarshallDeputy& arg, MarshallDeputy* ret) {
        rrr::Future* __fu__ = this->async_MsgMarshall(arg);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *ret;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_ReElect(const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::REELECT, __fu_attr__);
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 ReElect(bool_t* success) {
        rrr::Future* __fu__ = this->async_ReElect();
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *success;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_Dispatch(const rrr::i64& tid, const DepId& dep_id, const MarshallDeputy& cmd, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::DISPATCH, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << tid;
            *__cl__ << dep_id;
            *__cl__ << cmd;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 Dispatch(const rrr::i64& tid, const DepId& dep_id, const MarshallDeputy& cmd, rrr::i32* res, TxnOutput* output, uint64_t* coro_id) {
        rrr::Future* __fu__ = this->async_Dispatch(tid, dep_id, cmd);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *res;
            __fu__->get_reply() >> *output;
            __fu__->get_reply() >> *coro_id;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_Prepare(const rrr::i64& tid, const std::vector<rrr::i32>& sids, const DepId& dep_id, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::PREPARE, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << tid;
            *__cl__ << sids;
            *__cl__ << dep_id;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 Prepare(const rrr::i64& tid, const std::vector<rrr::i32>& sids, const DepId& dep_id, rrr::i32* res, bool_t* slow, uint64_t* coro_id) {
        rrr::Future* __fu__ = this->async_Prepare(tid, sids, dep_id);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *res;
            __fu__->get_reply() >> *slow;
            __fu__->get_reply() >> *coro_id;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_Commit(const rrr::i64& tid, const DepId& dep_id, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::COMMIT, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << tid;
            *__cl__ << dep_id;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 Commit(const rrr::i64& tid, const DepId& dep_id, rrr::i32* res, bool_t* slow, uint64_t* coro_id, Profiling* profile) {
        rrr::Future* __fu__ = this->async_Commit(tid, dep_id);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *res;
            __fu__->get_reply() >> *slow;
            __fu__->get_reply() >> *coro_id;
            __fu__->get_reply() >> *profile;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_Abort(const rrr::i64& tid, const DepId& dep_id, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::ABORT, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << tid;
            *__cl__ << dep_id;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 Abort(const rrr::i64& tid, const DepId& dep_id, rrr::i32* res, bool_t* slow, uint64_t* coro_id, Profiling* profile) {
        rrr::Future* __fu__ = this->async_Abort(tid, dep_id);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *res;
            __fu__->get_reply() >> *slow;
            __fu__->get_reply() >> *coro_id;
            __fu__->get_reply() >> *profile;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_EarlyAbort(const rrr::i64& tid, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::EARLYABORT, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << tid;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 EarlyAbort(const rrr::i64& tid, rrr::i32* res) {
        rrr::Future* __fu__ = this->async_EarlyAbort(tid);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *res;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_UpgradeEpoch(const uint32_t& curr_epoch, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::UPGRADEEPOCH, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << curr_epoch;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 UpgradeEpoch(const uint32_t& curr_epoch, int32_t* res) {
        rrr::Future* __fu__ = this->async_UpgradeEpoch(curr_epoch);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *res;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_TruncateEpoch(const uint32_t& old_epoch, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::TRUNCATEEPOCH, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << old_epoch;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 TruncateEpoch(const uint32_t& old_epoch) {
        rrr::Future* __fu__ = this->async_TruncateEpoch(old_epoch);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_IsLeader(const locid_t& cur_pause, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::ISLEADER, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << cur_pause;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 IsLeader(const locid_t& cur_pause, bool_t* is_leader) {
        rrr::Future* __fu__ = this->async_IsLeader(cur_pause);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *is_leader;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_IsFPGALeader(const locid_t& cur_pause, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::ISFPGALEADER, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << cur_pause;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 IsFPGALeader(const locid_t& cur_pause, bool_t* is_leader) {
        rrr::Future* __fu__ = this->async_IsFPGALeader(cur_pause);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *is_leader;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_SimpleCmd(const SimpleCommand& cmd, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::SIMPLECMD, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << cmd;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 SimpleCmd(const SimpleCommand& cmd, rrr::i32* res) {
        rrr::Future* __fu__ = this->async_SimpleCmd(cmd);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *res;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_FailOverTrig(const bool_t& pause, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::FAILOVERTRIG, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << pause;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 FailOverTrig(const bool_t& pause, rrr::i32* res) {
        rrr::Future* __fu__ = this->async_FailOverTrig(pause);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *res;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_rpc_null(const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::RPC_NULL, __fu_attr__);
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 rpc_null() {
        rrr::Future* __fu__ = this->async_rpc_null();
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        __fu__->release();
        return __ret__;
    }
};

class ServerControlService: public rrr::Service {
public:
    enum {
        SERVER_SHUTDOWN = 0x2a497d19,
        SERVER_READY = 0x6cddaf1b,
        SERVER_HEART_BEAT_WITH_DATA = 0x49ceac2e,
        SERVER_HEART_BEAT = 0x6b3e3d7c,
    };
    int __reg_to__(rrr::Server* svr) {
        int ret = 0;
        if ((ret = svr->reg(SERVER_SHUTDOWN, this, &ServerControlService::__server_shutdown__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(SERVER_READY, this, &ServerControlService::__server_ready__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(SERVER_HEART_BEAT_WITH_DATA, this, &ServerControlService::__server_heart_beat_with_data__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(SERVER_HEART_BEAT, this, &ServerControlService::__server_heart_beat__wrapper__)) != 0) {
            goto err;
        }
        return 0;
    err:
        svr->unreg(SERVER_SHUTDOWN);
        svr->unreg(SERVER_READY);
        svr->unreg(SERVER_HEART_BEAT_WITH_DATA);
        svr->unreg(SERVER_HEART_BEAT);
        return ret;
    }
    // these RPC handler functions need to be implemented by user
    // for 'raw' handlers, req is rusty::Box (auto-cleaned); weak_ptr requires lock() before use
    virtual void server_shutdown(rrr::DeferredReply* defer) = 0;
    virtual void server_ready(rrr::i32* res, rrr::DeferredReply* defer) = 0;
    virtual void server_heart_beat_with_data(ServerResponse* res, rrr::DeferredReply* defer) = 0;
    virtual void server_heart_beat(rrr::DeferredReply* defer) = 0;
private:
    void __server_shutdown__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
            }
        };
        auto __cleanup__ = [=] {
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->server_shutdown(__defer__);
    }
    void __server_ready__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        rrr::i32* out_0 = new rrr::i32;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
            }
        };
        auto __cleanup__ = [=] {
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->server_ready(out_0, __defer__);
    }
    void __server_heart_beat_with_data__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        ServerResponse* out_0 = new ServerResponse;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
            }
        };
        auto __cleanup__ = [=] {
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->server_heart_beat_with_data(out_0, __defer__);
    }
    void __server_heart_beat__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
            }
        };
        auto __cleanup__ = [=] {
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->server_heart_beat(__defer__);
    }
};

class ServerControlProxy {
protected:
    rrr::Client* __cl__;
public:
    ServerControlProxy(rrr::Client* cl): __cl__(cl) { }
    rrr::Future* async_server_shutdown(const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ServerControlService::SERVER_SHUTDOWN, __fu_attr__);
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 server_shutdown() {
        rrr::Future* __fu__ = this->async_server_shutdown();
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_server_ready(const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ServerControlService::SERVER_READY, __fu_attr__);
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 server_ready(rrr::i32* res) {
        rrr::Future* __fu__ = this->async_server_ready();
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *res;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_server_heart_beat_with_data(const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ServerControlService::SERVER_HEART_BEAT_WITH_DATA, __fu_attr__);
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 server_heart_beat_with_data(ServerResponse* res) {
        rrr::Future* __fu__ = this->async_server_heart_beat_with_data();
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *res;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_server_heart_beat(const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ServerControlService::SERVER_HEART_BEAT, __fu_attr__);
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 server_heart_beat() {
        rrr::Future* __fu__ = this->async_server_heart_beat();
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        __fu__->release();
        return __ret__;
    }
};

class ClientControlService: public rrr::Service {
public:
    enum {
        CLIENT_GET_TXN_NAMES = 0x2d0585eb,
        CLIENT_SHUTDOWN = 0x5fcdaf3c,
        CLIENT_FORCE_STOP = 0x4ea28bbe,
        CLIENT_RESPONSE = 0x4c5bd87c,
        CLIENT_READY = 0x6206c014,
        CLIENT_READY_BLOCK = 0x2ba4e66d,
        CLIENT_START = 0x5d5cddab,
        DISPATCHTXN = 0x6aa887d7,
    };
    int __reg_to__(rrr::Server* svr) {
        int ret = 0;
        if ((ret = svr->reg(CLIENT_GET_TXN_NAMES, this, &ClientControlService::__client_get_txn_names__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(CLIENT_SHUTDOWN, this, &ClientControlService::__client_shutdown__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(CLIENT_FORCE_STOP, this, &ClientControlService::__client_force_stop__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(CLIENT_RESPONSE, this, &ClientControlService::__client_response__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(CLIENT_READY, this, &ClientControlService::__client_ready__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(CLIENT_READY_BLOCK, this, &ClientControlService::__client_ready_block__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(CLIENT_START, this, &ClientControlService::__client_start__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(DISPATCHTXN, this, &ClientControlService::__DispatchTxn__wrapper__)) != 0) {
            goto err;
        }
        return 0;
    err:
        svr->unreg(CLIENT_GET_TXN_NAMES);
        svr->unreg(CLIENT_SHUTDOWN);
        svr->unreg(CLIENT_FORCE_STOP);
        svr->unreg(CLIENT_RESPONSE);
        svr->unreg(CLIENT_READY);
        svr->unreg(CLIENT_READY_BLOCK);
        svr->unreg(CLIENT_START);
        svr->unreg(DISPATCHTXN);
        return ret;
    }
    // these RPC handler functions need to be implemented by user
    // for 'raw' handlers, req is rusty::Box (auto-cleaned); weak_ptr requires lock() before use
    virtual void client_get_txn_names(std::map<rrr::i32, std::string>* txn_names, rrr::DeferredReply* defer) = 0;
    virtual void client_shutdown(rrr::DeferredReply* defer) = 0;
    virtual void client_force_stop(rrr::DeferredReply* defer) = 0;
    virtual void client_response(const DepId& dep_id, ClientResponse* res, rrr::DeferredReply* defer) = 0;
    virtual void client_ready(rrr::i32* res, rrr::DeferredReply* defer) = 0;
    virtual void client_ready_block(rrr::i32* res, rrr::DeferredReply* defer) = 0;
    virtual void client_start(rrr::DeferredReply* defer) = 0;
    virtual void DispatchTxn(const TxDispatchRequest& req, TxReply* result, rrr::DeferredReply* defer) = 0;
private:
    void __client_get_txn_names__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        std::map<rrr::i32, std::string>* out_0 = new std::map<rrr::i32, std::string>;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
            }
        };
        auto __cleanup__ = [=] {
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->client_get_txn_names(out_0, __defer__);
    }
    void __client_shutdown__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
            }
        };
        auto __cleanup__ = [=] {
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->client_shutdown(__defer__);
    }
    void __client_force_stop__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
            }
        };
        auto __cleanup__ = [=] {
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->client_force_stop(__defer__);
    }
    void __client_response__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        DepId* in_0 = new DepId;
        req->m >> *in_0;
        ClientResponse* out_0 = new ClientResponse;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
            }
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->client_response(*in_0, out_0, __defer__);
    }
    void __client_ready__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        rrr::i32* out_0 = new rrr::i32;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
            }
        };
        auto __cleanup__ = [=] {
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->client_ready(out_0, __defer__);
    }
    void __client_ready_block__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        rrr::i32* out_0 = new rrr::i32;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
            }
        };
        auto __cleanup__ = [=] {
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->client_ready_block(out_0, __defer__);
    }
    void __client_start__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
            }
        };
        auto __cleanup__ = [=] {
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->client_start(__defer__);
    }
    void __DispatchTxn__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        TxDispatchRequest* in_0 = new TxDispatchRequest;
        req->m >> *in_0;
        TxReply* out_0 = new TxReply;
        auto __marshal_reply__ = [=] {
            auto sconn = weak_sconn.lock();
            if (sconn) {
                *sconn << *out_0;
            }
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->DispatchTxn(*in_0, out_0, __defer__);
    }
};

class ClientControlProxy {
protected:
    rrr::Client* __cl__;
public:
    ClientControlProxy(rrr::Client* cl): __cl__(cl) { }
    rrr::Future* async_client_get_txn_names(const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClientControlService::CLIENT_GET_TXN_NAMES, __fu_attr__);
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 client_get_txn_names(std::map<rrr::i32, std::string>* txn_names) {
        rrr::Future* __fu__ = this->async_client_get_txn_names();
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *txn_names;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_client_shutdown(const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClientControlService::CLIENT_SHUTDOWN, __fu_attr__);
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 client_shutdown() {
        rrr::Future* __fu__ = this->async_client_shutdown();
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_client_force_stop(const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClientControlService::CLIENT_FORCE_STOP, __fu_attr__);
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 client_force_stop() {
        rrr::Future* __fu__ = this->async_client_force_stop();
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_client_response(const DepId& dep_id, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClientControlService::CLIENT_RESPONSE, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << dep_id;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 client_response(const DepId& dep_id, ClientResponse* res) {
        rrr::Future* __fu__ = this->async_client_response(dep_id);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *res;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_client_ready(const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClientControlService::CLIENT_READY, __fu_attr__);
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 client_ready(rrr::i32* res) {
        rrr::Future* __fu__ = this->async_client_ready();
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *res;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_client_ready_block(const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClientControlService::CLIENT_READY_BLOCK, __fu_attr__);
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 client_ready_block(rrr::i32* res) {
        rrr::Future* __fu__ = this->async_client_ready_block();
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *res;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_client_start(const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClientControlService::CLIENT_START, __fu_attr__);
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 client_start() {
        rrr::Future* __fu__ = this->async_client_start();
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_DispatchTxn(const TxDispatchRequest& req, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClientControlService::DISPATCHTXN, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << req;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 DispatchTxn(const TxDispatchRequest& req, TxReply* result) {
        rrr::Future* __fu__ = this->async_DispatchTxn(req);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *result;
        }
        __fu__->release();
        return __ret__;
    }
};

} // namespace janus



