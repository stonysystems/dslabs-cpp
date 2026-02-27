#pragma once

#include "rrr.hpp"

#include <errno.h>


namespace janus {

class RaftService: public rrr::Service {
public:
    enum {
        VOTE = 0x24db0506,
        APPENDENTRIES = 0x48d43982,
        EMPTYAPPENDENTRIES = 0x26632676,
    };
    int __reg_to__(rrr::Server* svr) {
        int ret = 0;
        if ((ret = svr->reg(VOTE, this, &RaftService::__Vote__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(APPENDENTRIES, this, &RaftService::__AppendEntries__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(EMPTYAPPENDENTRIES, this, &RaftService::__EmptyAppendEntries__wrapper__)) != 0) {
            goto err;
        }
        return 0;
    err:
        svr->unreg(VOTE);
        svr->unreg(APPENDENTRIES);
        svr->unreg(EMPTYAPPENDENTRIES);
        return ret;
    }
    // these RPC handler functions need to be implemented by user
    // for 'raw' handlers, req is rusty::Box (auto-cleaned); weak_ptr requires lock() before use
    virtual void Vote(const uint64_t& lst_log_idx, const ballot_t& lst_log_term, const siteid_t& site_id, const ballot_t& cur_term, ballot_t* max_ballot, bool_t* vote_granted, rrr::DeferredReply* defer) = 0;
    virtual void AppendEntries(const uint64_t& slot, const ballot_t& ballot, const uint64_t& leaderCurrentTerm, const uint64_t& leaderPrevLogIndex, const uint64_t& leaderPrevLogTerm, const uint64_t& leaderCommitIndex, const MarshallDeputy& cmd, const uint64_t& leaderNextLogTerm, uint64_t* followerAppendOK, uint64_t* followerCurrentTerm, uint64_t* followerLastLogIndex, rrr::DeferredReply* defer) = 0;
    virtual void EmptyAppendEntries(const uint64_t& slot, const ballot_t& ballot, const uint64_t& leaderCurrentTerm, const uint64_t& leaderPrevLogIndex, const uint64_t& leaderPrevLogTerm, const uint64_t& leaderCommitIndex, uint64_t* followerAppendOK, uint64_t* followerCurrentTerm, uint64_t* followerLastLogIndex, rrr::DeferredReply* defer) = 0;
private:
    void __Vote__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        uint64_t* in_0 = new uint64_t;
        req->m >> *in_0;
        ballot_t* in_1 = new ballot_t;
        req->m >> *in_1;
        siteid_t* in_2 = new siteid_t;
        req->m >> *in_2;
        ballot_t* in_3 = new ballot_t;
        req->m >> *in_3;
        ballot_t* out_0 = new ballot_t;
        bool_t* out_1 = new bool_t;
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
        this->Vote(*in_0, *in_1, *in_2, *in_3, out_0, out_1, __defer__);
    }
    void __AppendEntries__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        uint64_t* in_0 = new uint64_t;
        req->m >> *in_0;
        ballot_t* in_1 = new ballot_t;
        req->m >> *in_1;
        uint64_t* in_2 = new uint64_t;
        req->m >> *in_2;
        uint64_t* in_3 = new uint64_t;
        req->m >> *in_3;
        uint64_t* in_4 = new uint64_t;
        req->m >> *in_4;
        uint64_t* in_5 = new uint64_t;
        req->m >> *in_5;
        MarshallDeputy* in_6 = new MarshallDeputy;
        req->m >> *in_6;
        uint64_t* in_7 = new uint64_t;
        req->m >> *in_7;
        uint64_t* out_0 = new uint64_t;
        uint64_t* out_1 = new uint64_t;
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
            delete in_3;
            delete in_4;
            delete in_5;
            delete in_6;
            delete in_7;
            delete out_0;
            delete out_1;
            delete out_2;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->AppendEntries(*in_0, *in_1, *in_2, *in_3, *in_4, *in_5, *in_6, *in_7, out_0, out_1, out_2, __defer__);
    }
    void __EmptyAppendEntries__wrapper__(rusty::Box<rrr::Request> req, std::weak_ptr<rrr::ServerConnection> weak_sconn) {
        uint64_t* in_0 = new uint64_t;
        req->m >> *in_0;
        ballot_t* in_1 = new ballot_t;
        req->m >> *in_1;
        uint64_t* in_2 = new uint64_t;
        req->m >> *in_2;
        uint64_t* in_3 = new uint64_t;
        req->m >> *in_3;
        uint64_t* in_4 = new uint64_t;
        req->m >> *in_4;
        uint64_t* in_5 = new uint64_t;
        req->m >> *in_5;
        uint64_t* out_0 = new uint64_t;
        uint64_t* out_1 = new uint64_t;
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
            delete in_3;
            delete in_4;
            delete in_5;
            delete out_0;
            delete out_1;
            delete out_2;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(std::move(req), weak_sconn, __marshal_reply__, __cleanup__);
        this->EmptyAppendEntries(*in_0, *in_1, *in_2, *in_3, *in_4, *in_5, out_0, out_1, out_2, __defer__);
    }
};

class RaftProxy {
protected:
    rrr::Client* __cl__;
public:
    RaftProxy(rrr::Client* cl): __cl__(cl) { }
    rrr::Future* async_Vote(const uint64_t& lst_log_idx, const ballot_t& lst_log_term, const siteid_t& site_id, const ballot_t& cur_term, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(RaftService::VOTE, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << lst_log_idx;
            *__cl__ << lst_log_term;
            *__cl__ << site_id;
            *__cl__ << cur_term;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 Vote(const uint64_t& lst_log_idx, const ballot_t& lst_log_term, const siteid_t& site_id, const ballot_t& cur_term, ballot_t* max_ballot, bool_t* vote_granted) {
        rrr::Future* __fu__ = this->async_Vote(lst_log_idx, lst_log_term, site_id, cur_term);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *max_ballot;
            __fu__->get_reply() >> *vote_granted;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_AppendEntries(const uint64_t& slot, const ballot_t& ballot, const uint64_t& leaderCurrentTerm, const uint64_t& leaderPrevLogIndex, const uint64_t& leaderPrevLogTerm, const uint64_t& leaderCommitIndex, const MarshallDeputy& cmd, const uint64_t& leaderNextLogTerm, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(RaftService::APPENDENTRIES, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << slot;
            *__cl__ << ballot;
            *__cl__ << leaderCurrentTerm;
            *__cl__ << leaderPrevLogIndex;
            *__cl__ << leaderPrevLogTerm;
            *__cl__ << leaderCommitIndex;
            *__cl__ << cmd;
            *__cl__ << leaderNextLogTerm;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 AppendEntries(const uint64_t& slot, const ballot_t& ballot, const uint64_t& leaderCurrentTerm, const uint64_t& leaderPrevLogIndex, const uint64_t& leaderPrevLogTerm, const uint64_t& leaderCommitIndex, const MarshallDeputy& cmd, const uint64_t& leaderNextLogTerm, uint64_t* followerAppendOK, uint64_t* followerCurrentTerm, uint64_t* followerLastLogIndex) {
        rrr::Future* __fu__ = this->async_AppendEntries(slot, ballot, leaderCurrentTerm, leaderPrevLogIndex, leaderPrevLogTerm, leaderCommitIndex, cmd, leaderNextLogTerm);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *followerAppendOK;
            __fu__->get_reply() >> *followerCurrentTerm;
            __fu__->get_reply() >> *followerLastLogIndex;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_EmptyAppendEntries(const uint64_t& slot, const ballot_t& ballot, const uint64_t& leaderCurrentTerm, const uint64_t& leaderPrevLogIndex, const uint64_t& leaderPrevLogTerm, const uint64_t& leaderCommitIndex, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(RaftService::EMPTYAPPENDENTRIES, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << slot;
            *__cl__ << ballot;
            *__cl__ << leaderCurrentTerm;
            *__cl__ << leaderPrevLogIndex;
            *__cl__ << leaderPrevLogTerm;
            *__cl__ << leaderCommitIndex;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 EmptyAppendEntries(const uint64_t& slot, const ballot_t& ballot, const uint64_t& leaderCurrentTerm, const uint64_t& leaderPrevLogIndex, const uint64_t& leaderPrevLogTerm, const uint64_t& leaderCommitIndex, uint64_t* followerAppendOK, uint64_t* followerCurrentTerm, uint64_t* followerLastLogIndex) {
        rrr::Future* __fu__ = this->async_EmptyAppendEntries(slot, ballot, leaderCurrentTerm, leaderPrevLogIndex, leaderPrevLogTerm, leaderCommitIndex);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *followerAppendOK;
            __fu__->get_reply() >> *followerCurrentTerm;
            __fu__->get_reply() >> *followerLastLogIndex;
        }
        __fu__->release();
        return __ret__;
    }
};

} // namespace janus



