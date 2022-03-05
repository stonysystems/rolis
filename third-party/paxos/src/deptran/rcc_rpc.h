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
        FORWARD = 0x1f300b37,
        PREPARE = 0x3ed339bc,
        ACCEPT = 0x2c1aa5e7,
        DECIDE = 0x1e2df369,
        HEARTBEAT = 0x249987b3,
        BULKPREPARE = 0x3094a906,
        BULKACCEPT = 0x58ffece4,
        BULKPREPARE2 = 0x245ecf49,
        SYNCLOG = 0x692edba6,
        SYNCCOMMIT = 0x54c77d9a,
        SYNCNOOPS = 0x23b1c916,
        BULKDECIDE = 0x3ede1dac,
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
    // for 'raw' handlers, remember to reply req, delete req, and sconn->release(); use sconn->run_async for heavy job
    virtual void Forward(const MarshallDeputy& cmd, rrr::DeferredReply* defer) = 0;
    virtual void Prepare(const uint64_t& slot, const ballot_t& ballot, ballot_t* max_ballot, rrr::DeferredReply* defer) = 0;
    virtual void Accept(const uint64_t& slot, const ballot_t& ballot, const MarshallDeputy& cmd, ballot_t* max_ballot, rrr::DeferredReply* defer) = 0;
    virtual void Decide(const uint64_t& slot, const ballot_t& ballot, const MarshallDeputy& cmd, rrr::DeferredReply* defer) = 0;
    virtual void Heartbeat(const MarshallDeputy& cmd, rrr::i32* ballot, rrr::i32* val, rrr::DeferredReply* defer) = 0;
    virtual void BulkPrepare(const MarshallDeputy& cmd, rrr::i32* ballot, rrr::i32* val, rrr::DeferredReply* defer) = 0;
    virtual void BulkAccept(const MarshallDeputy& cmd, rrr::i32* ballot, rrr::i32* val, rrr::DeferredReply* defer) = 0;
    virtual void BulkPrepare2(const MarshallDeputy& cmd, rrr::i32* ballot, rrr::i32* val, MarshallDeputy* ret, rrr::DeferredReply* defer) = 0;
    virtual void SyncLog(const MarshallDeputy& cmd, rrr::i32* ballot, rrr::i32* val, MarshallDeputy* ret, rrr::DeferredReply* defer) = 0;
    virtual void SyncCommit(const MarshallDeputy& cmd, rrr::i32* ballot, rrr::i32* val, rrr::DeferredReply* defer) = 0;
    virtual void SyncNoOps(const MarshallDeputy& cmd, rrr::i32* ballot, rrr::i32* val, rrr::DeferredReply* defer) = 0;
    virtual void BulkDecide(const MarshallDeputy& cmd, rrr::i32* ballot, rrr::i32* val, rrr::DeferredReply* defer) = 0;
private:
    void __Forward__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        MarshallDeputy* in_0 = new MarshallDeputy;
        req->m >> *in_0;
        auto __marshal_reply__ = [=] {
        };
        auto __cleanup__ = [=] {
            delete in_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->Forward(*in_0, __defer__);
    }
    void __Prepare__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        uint64_t* in_0 = new uint64_t;
        req->m >> *in_0;
        ballot_t* in_1 = new ballot_t;
        req->m >> *in_1;
        ballot_t* out_0 = new ballot_t;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete in_1;
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->Prepare(*in_0, *in_1, out_0, __defer__);
    }
    void __Accept__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        uint64_t* in_0 = new uint64_t;
        req->m >> *in_0;
        ballot_t* in_1 = new ballot_t;
        req->m >> *in_1;
        MarshallDeputy* in_2 = new MarshallDeputy;
        req->m >> *in_2;
        ballot_t* out_0 = new ballot_t;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete in_1;
            delete in_2;
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->Accept(*in_0, *in_1, *in_2, out_0, __defer__);
    }
    void __Decide__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        uint64_t* in_0 = new uint64_t;
        req->m >> *in_0;
        ballot_t* in_1 = new ballot_t;
        req->m >> *in_1;
        MarshallDeputy* in_2 = new MarshallDeputy;
        req->m >> *in_2;
        auto __marshal_reply__ = [=] {
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete in_1;
            delete in_2;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->Decide(*in_0, *in_1, *in_2, __defer__);
    }
    void __Heartbeat__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        MarshallDeputy* in_0 = new MarshallDeputy;
        req->m >> *in_0;
        rrr::i32* out_0 = new rrr::i32;
        rrr::i32* out_1 = new rrr::i32;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
            *sconn << *out_1;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
            delete out_1;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->Heartbeat(*in_0, out_0, out_1, __defer__);
    }
    void __BulkPrepare__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        MarshallDeputy* in_0 = new MarshallDeputy;
        req->m >> *in_0;
        rrr::i32* out_0 = new rrr::i32;
        rrr::i32* out_1 = new rrr::i32;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
            *sconn << *out_1;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
            delete out_1;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->BulkPrepare(*in_0, out_0, out_1, __defer__);
    }
    void __BulkAccept__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        MarshallDeputy* in_0 = new MarshallDeputy;
        req->m >> *in_0;
        rrr::i32* out_0 = new rrr::i32;
        rrr::i32* out_1 = new rrr::i32;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
            *sconn << *out_1;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
            delete out_1;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->BulkAccept(*in_0, out_0, out_1, __defer__);
    }
    void __BulkPrepare2__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        MarshallDeputy* in_0 = new MarshallDeputy;
        req->m >> *in_0;
        rrr::i32* out_0 = new rrr::i32;
        rrr::i32* out_1 = new rrr::i32;
        MarshallDeputy* out_2 = new MarshallDeputy;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
            *sconn << *out_1;
            *sconn << *out_2;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
            delete out_1;
            delete out_2;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->BulkPrepare2(*in_0, out_0, out_1, out_2, __defer__);
    }
    void __SyncLog__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        MarshallDeputy* in_0 = new MarshallDeputy;
        req->m >> *in_0;
        rrr::i32* out_0 = new rrr::i32;
        rrr::i32* out_1 = new rrr::i32;
        MarshallDeputy* out_2 = new MarshallDeputy;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
            *sconn << *out_1;
            *sconn << *out_2;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
            delete out_1;
            delete out_2;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->SyncLog(*in_0, out_0, out_1, out_2, __defer__);
    }
    void __SyncCommit__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        MarshallDeputy* in_0 = new MarshallDeputy;
        req->m >> *in_0;
        rrr::i32* out_0 = new rrr::i32;
        rrr::i32* out_1 = new rrr::i32;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
            *sconn << *out_1;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
            delete out_1;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->SyncCommit(*in_0, out_0, out_1, __defer__);
    }
    void __SyncNoOps__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        MarshallDeputy* in_0 = new MarshallDeputy;
        req->m >> *in_0;
        rrr::i32* out_0 = new rrr::i32;
        rrr::i32* out_1 = new rrr::i32;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
            *sconn << *out_1;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
            delete out_1;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->SyncNoOps(*in_0, out_0, out_1, __defer__);
    }
    void __BulkDecide__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        MarshallDeputy* in_0 = new MarshallDeputy;
        req->m >> *in_0;
        rrr::i32* out_0 = new rrr::i32;
        rrr::i32* out_1 = new rrr::i32;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
            *sconn << *out_1;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
            delete out_1;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->BulkDecide(*in_0, out_0, out_1, __defer__);
    }
};

class MultiPaxosProxy {
protected:
    rrr::Client* __cl__;
public:
    MultiPaxosProxy(rrr::Client* cl): __cl__(cl) { }
    rrr::Future* async_Forward(const MarshallDeputy& cmd, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(MultiPaxosService::FORWARD, __fu_attr__);
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
    rrr::Future* async_Prepare(const uint64_t& slot, const ballot_t& ballot, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(MultiPaxosService::PREPARE, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << slot;
            *__cl__ << ballot;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 Prepare(const uint64_t& slot, const ballot_t& ballot, ballot_t* max_ballot) {
        rrr::Future* __fu__ = this->async_Prepare(slot, ballot);
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
    rrr::Future* async_Accept(const uint64_t& slot, const ballot_t& ballot, const MarshallDeputy& cmd, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(MultiPaxosService::ACCEPT, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << slot;
            *__cl__ << ballot;
            *__cl__ << cmd;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 Accept(const uint64_t& slot, const ballot_t& ballot, const MarshallDeputy& cmd, ballot_t* max_ballot) {
        rrr::Future* __fu__ = this->async_Accept(slot, ballot, cmd);
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

class ClassicService: public rrr::Service {
public:
    enum {
        MSGSTRING = 0x1d0eebab,
        MSGMARSHALL = 0x56332c42,
        DISPATCH = 0x4eeae560,
        PREPARE = 0x12c808be,
        COMMIT = 0x3f5f50ce,
        ABORT = 0x35c4289d,
        UPGRADEEPOCH = 0x606ad021,
        TRUNCATEEPOCH = 0x5aa4b85a,
        RPC_NULL = 0x62b4c084,
        TAPIRACCEPT = 0x386f2e01,
        TAPIRFASTACCEPT = 0x2800f924,
        TAPIRDECIDE = 0x3b2877f4,
        RCCDISPATCH = 0x137064c0,
        RCCFINISH = 0x14436ea6,
        RCCINQUIRE = 0x4177fa53,
        RCCDISPATCHRO = 0x5bc4ebfa,
        RCCINQUIREVALIDATION = 0x4611385a,
        RCCNOTIFYGLOBALVALIDATION = 0x1cae978d,
        JANUSDISPATCH = 0x2dcc423d,
        JANUSCOMMIT = 0x6f9254e7,
        JANUSCOMMITWOGRAPH = 0x1b3241f8,
        JANUSINQUIRE = 0x5ce268e9,
        JANUSPREACCEPT = 0x6a9a671f,
        JANUSPREACCEPTWOGRAPH = 0x10fe58f2,
        JANUSACCEPT = 0x158460fa,
        PREACCEPTFEBRUUS = 0x65ea71d0,
        ACCEPTFEBRUUS = 0x36085b2c,
        COMMITFEBRUUS = 0x551bbfaf,
    };
    int __reg_to__(rrr::Server* svr) {
        int ret = 0;
        if ((ret = svr->reg(MSGSTRING, this, &ClassicService::__MsgString__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(MSGMARSHALL, this, &ClassicService::__MsgMarshall__wrapper__)) != 0) {
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
        if ((ret = svr->reg(UPGRADEEPOCH, this, &ClassicService::__UpgradeEpoch__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(TRUNCATEEPOCH, this, &ClassicService::__TruncateEpoch__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(RPC_NULL, this, &ClassicService::__rpc_null__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(TAPIRACCEPT, this, &ClassicService::__TapirAccept__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(TAPIRFASTACCEPT, this, &ClassicService::__TapirFastAccept__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(TAPIRDECIDE, this, &ClassicService::__TapirDecide__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(RCCDISPATCH, this, &ClassicService::__RccDispatch__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(RCCFINISH, this, &ClassicService::__RccFinish__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(RCCINQUIRE, this, &ClassicService::__RccInquire__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(RCCDISPATCHRO, this, &ClassicService::__RccDispatchRo__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(RCCINQUIREVALIDATION, this, &ClassicService::__RccInquireValidation__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(RCCNOTIFYGLOBALVALIDATION, this, &ClassicService::__RccNotifyGlobalValidation__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(JANUSDISPATCH, this, &ClassicService::__JanusDispatch__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(JANUSCOMMIT, this, &ClassicService::__JanusCommit__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(JANUSCOMMITWOGRAPH, this, &ClassicService::__JanusCommitWoGraph__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(JANUSINQUIRE, this, &ClassicService::__JanusInquire__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(JANUSPREACCEPT, this, &ClassicService::__JanusPreAccept__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(JANUSPREACCEPTWOGRAPH, this, &ClassicService::__JanusPreAcceptWoGraph__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(JANUSACCEPT, this, &ClassicService::__JanusAccept__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(PREACCEPTFEBRUUS, this, &ClassicService::__PreAcceptFebruus__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(ACCEPTFEBRUUS, this, &ClassicService::__AcceptFebruus__wrapper__)) != 0) {
            goto err;
        }
        if ((ret = svr->reg(COMMITFEBRUUS, this, &ClassicService::__CommitFebruus__wrapper__)) != 0) {
            goto err;
        }
        return 0;
    err:
        svr->unreg(MSGSTRING);
        svr->unreg(MSGMARSHALL);
        svr->unreg(DISPATCH);
        svr->unreg(PREPARE);
        svr->unreg(COMMIT);
        svr->unreg(ABORT);
        svr->unreg(UPGRADEEPOCH);
        svr->unreg(TRUNCATEEPOCH);
        svr->unreg(RPC_NULL);
        svr->unreg(TAPIRACCEPT);
        svr->unreg(TAPIRFASTACCEPT);
        svr->unreg(TAPIRDECIDE);
        svr->unreg(RCCDISPATCH);
        svr->unreg(RCCFINISH);
        svr->unreg(RCCINQUIRE);
        svr->unreg(RCCDISPATCHRO);
        svr->unreg(RCCINQUIREVALIDATION);
        svr->unreg(RCCNOTIFYGLOBALVALIDATION);
        svr->unreg(JANUSDISPATCH);
        svr->unreg(JANUSCOMMIT);
        svr->unreg(JANUSCOMMITWOGRAPH);
        svr->unreg(JANUSINQUIRE);
        svr->unreg(JANUSPREACCEPT);
        svr->unreg(JANUSPREACCEPTWOGRAPH);
        svr->unreg(JANUSACCEPT);
        svr->unreg(PREACCEPTFEBRUUS);
        svr->unreg(ACCEPTFEBRUUS);
        svr->unreg(COMMITFEBRUUS);
        return ret;
    }
    // these RPC handler functions need to be implemented by user
    // for 'raw' handlers, remember to reply req, delete req, and sconn->release(); use sconn->run_async for heavy job
    virtual void MsgString(const std::string& arg, std::string* ret, rrr::DeferredReply* defer) = 0;
    virtual void MsgMarshall(const MarshallDeputy& arg, MarshallDeputy* ret, rrr::DeferredReply* defer) = 0;
    virtual void Dispatch(const rrr::i64& tid, const MarshallDeputy& cmd, rrr::i32* res, TxnOutput* output, rrr::DeferredReply* defer) = 0;
    virtual void Prepare(const rrr::i64& tid, const std::vector<rrr::i32>& sids, rrr::i32* res, rrr::DeferredReply* defer) = 0;
    virtual void Commit(const rrr::i64& tid, rrr::i32* res, rrr::DeferredReply* defer) = 0;
    virtual void Abort(const rrr::i64& tid, rrr::i32* res, rrr::DeferredReply* defer) = 0;
    virtual void UpgradeEpoch(const uint32_t& curr_epoch, int32_t* res, rrr::DeferredReply* defer) = 0;
    virtual void TruncateEpoch(const uint32_t& old_epoch, rrr::DeferredReply* defer) = 0;
    virtual void rpc_null(rrr::DeferredReply* defer) = 0;
    virtual void TapirAccept(const uint64_t& cmd_id, const int64_t& ballot, const int32_t& decision, rrr::DeferredReply* defer) = 0;
    virtual void TapirFastAccept(const uint64_t& cmd_id, const std::vector<SimpleCommand>& txn_cmds, rrr::i32* res, rrr::DeferredReply* defer) = 0;
    virtual void TapirDecide(const uint64_t& cmd_id, const rrr::i32& commit, rrr::DeferredReply* defer) = 0;
    virtual void RccDispatch(const std::vector<SimpleCommand>& cmd, rrr::i32* res, TxnOutput* output, MarshallDeputy* md_graph, rrr::DeferredReply* defer) = 0;
    virtual void RccFinish(const cmdid_t& id, const MarshallDeputy& md_graph, std::map<uint32_t, std::map<int32_t, Value>>* outputs, rrr::DeferredReply* defer) = 0;
    virtual void RccInquire(const epoch_t& epoch, const txnid_t& txn_id, MarshallDeputy* md_res_graph, rrr::DeferredReply* defer) = 0;
    virtual void RccDispatchRo(const SimpleCommand& cmd, std::map<rrr::i32, Value>* output, rrr::DeferredReply* defer) = 0;
    virtual void RccInquireValidation(const txid_t& tx_id, int32_t* res, rrr::DeferredReply* defer) = 0;
    virtual void RccNotifyGlobalValidation(const txid_t& tx_id, const int32_t& res, rrr::DeferredReply* defer) = 0;
    virtual void JanusDispatch(const std::vector<SimpleCommand>& cmd, rrr::i32* res, TxnOutput* output, MarshallDeputy* ret_graph, rrr::DeferredReply* defer) = 0;
    virtual void JanusCommit(const cmdid_t& id, const rank_t& rank, const int32_t& need_validation, const MarshallDeputy& graph, int32_t* res, TxnOutput* output, rrr::DeferredReply* defer) = 0;
    virtual void JanusCommitWoGraph(const cmdid_t& id, const rank_t& rank, const int32_t& need_validation, int32_t* res, TxnOutput* output, rrr::DeferredReply* defer) = 0;
    virtual void JanusInquire(const epoch_t& epoch, const txnid_t& txn_id, MarshallDeputy* ret_graph, rrr::DeferredReply* defer) = 0;
    virtual void JanusPreAccept(const cmdid_t& txn_id, const rank_t& rank, const std::vector<SimpleCommand>& cmd, const MarshallDeputy& graph, rrr::i32* res, MarshallDeputy* ret_graph, rrr::DeferredReply* defer) = 0;
    virtual void JanusPreAcceptWoGraph(const cmdid_t& txn_id, const rank_t& rank, const std::vector<SimpleCommand>& cmd, rrr::i32* res, MarshallDeputy* ret_graph, rrr::DeferredReply* defer) = 0;
    virtual void JanusAccept(const cmdid_t& txn_id, const ballot_t& ballot, const MarshallDeputy& graph, rrr::i32* res, rrr::DeferredReply* defer) = 0;
    virtual void PreAcceptFebruus(const txid_t& tx_id, rrr::i32* ret, uint64_t* timestamp, rrr::DeferredReply* defer) = 0;
    virtual void AcceptFebruus(const txid_t& tx_id, const ballot_t& ballot, const uint64_t& timestamp, rrr::i32* ret, rrr::DeferredReply* defer) = 0;
    virtual void CommitFebruus(const txid_t& tx_id, const uint64_t& timestamp, rrr::i32* ret, rrr::DeferredReply* defer) = 0;
private:
    void __MsgString__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        std::string* in_0 = new std::string;
        req->m >> *in_0;
        std::string* out_0 = new std::string;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->MsgString(*in_0, out_0, __defer__);
    }
    void __MsgMarshall__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        MarshallDeputy* in_0 = new MarshallDeputy;
        req->m >> *in_0;
        MarshallDeputy* out_0 = new MarshallDeputy;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->MsgMarshall(*in_0, out_0, __defer__);
    }
    void __Dispatch__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        rrr::i64* in_0 = new rrr::i64;
        req->m >> *in_0;
        MarshallDeputy* in_1 = new MarshallDeputy;
        req->m >> *in_1;
        rrr::i32* out_0 = new rrr::i32;
        TxnOutput* out_1 = new TxnOutput;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
            *sconn << *out_1;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete in_1;
            delete out_0;
            delete out_1;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->Dispatch(*in_0, *in_1, out_0, out_1, __defer__);
    }
    void __Prepare__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        rrr::i64* in_0 = new rrr::i64;
        req->m >> *in_0;
        std::vector<rrr::i32>* in_1 = new std::vector<rrr::i32>;
        req->m >> *in_1;
        rrr::i32* out_0 = new rrr::i32;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete in_1;
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->Prepare(*in_0, *in_1, out_0, __defer__);
    }
    void __Commit__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        rrr::i64* in_0 = new rrr::i64;
        req->m >> *in_0;
        rrr::i32* out_0 = new rrr::i32;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->Commit(*in_0, out_0, __defer__);
    }
    void __Abort__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        rrr::i64* in_0 = new rrr::i64;
        req->m >> *in_0;
        rrr::i32* out_0 = new rrr::i32;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->Abort(*in_0, out_0, __defer__);
    }
    void __UpgradeEpoch__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        uint32_t* in_0 = new uint32_t;
        req->m >> *in_0;
        int32_t* out_0 = new int32_t;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->UpgradeEpoch(*in_0, out_0, __defer__);
    }
    void __TruncateEpoch__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        uint32_t* in_0 = new uint32_t;
        req->m >> *in_0;
        auto __marshal_reply__ = [=] {
        };
        auto __cleanup__ = [=] {
            delete in_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->TruncateEpoch(*in_0, __defer__);
    }
    void __rpc_null__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        auto __marshal_reply__ = [=] {
        };
        auto __cleanup__ = [=] {
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->rpc_null(__defer__);
    }
    void __TapirAccept__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        uint64_t* in_0 = new uint64_t;
        req->m >> *in_0;
        int64_t* in_1 = new int64_t;
        req->m >> *in_1;
        int32_t* in_2 = new int32_t;
        req->m >> *in_2;
        auto __marshal_reply__ = [=] {
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete in_1;
            delete in_2;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->TapirAccept(*in_0, *in_1, *in_2, __defer__);
    }
    void __TapirFastAccept__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        uint64_t* in_0 = new uint64_t;
        req->m >> *in_0;
        std::vector<SimpleCommand>* in_1 = new std::vector<SimpleCommand>;
        req->m >> *in_1;
        rrr::i32* out_0 = new rrr::i32;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete in_1;
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->TapirFastAccept(*in_0, *in_1, out_0, __defer__);
    }
    void __TapirDecide__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        uint64_t* in_0 = new uint64_t;
        req->m >> *in_0;
        rrr::i32* in_1 = new rrr::i32;
        req->m >> *in_1;
        auto __marshal_reply__ = [=] {
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete in_1;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->TapirDecide(*in_0, *in_1, __defer__);
    }
    void __RccDispatch__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        std::vector<SimpleCommand>* in_0 = new std::vector<SimpleCommand>;
        req->m >> *in_0;
        rrr::i32* out_0 = new rrr::i32;
        TxnOutput* out_1 = new TxnOutput;
        MarshallDeputy* out_2 = new MarshallDeputy;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
            *sconn << *out_1;
            *sconn << *out_2;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
            delete out_1;
            delete out_2;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->RccDispatch(*in_0, out_0, out_1, out_2, __defer__);
    }
    void __RccFinish__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        cmdid_t* in_0 = new cmdid_t;
        req->m >> *in_0;
        MarshallDeputy* in_1 = new MarshallDeputy;
        req->m >> *in_1;
        std::map<uint32_t, std::map<int32_t, Value>>* out_0 = new std::map<uint32_t, std::map<int32_t, Value>>;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete in_1;
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->RccFinish(*in_0, *in_1, out_0, __defer__);
    }
    void __RccInquire__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        epoch_t* in_0 = new epoch_t;
        req->m >> *in_0;
        txnid_t* in_1 = new txnid_t;
        req->m >> *in_1;
        MarshallDeputy* out_0 = new MarshallDeputy;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete in_1;
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->RccInquire(*in_0, *in_1, out_0, __defer__);
    }
    void __RccDispatchRo__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        SimpleCommand* in_0 = new SimpleCommand;
        req->m >> *in_0;
        std::map<rrr::i32, Value>* out_0 = new std::map<rrr::i32, Value>;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->RccDispatchRo(*in_0, out_0, __defer__);
    }
    void __RccInquireValidation__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        txid_t* in_0 = new txid_t;
        req->m >> *in_0;
        int32_t* out_0 = new int32_t;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->RccInquireValidation(*in_0, out_0, __defer__);
    }
    void __RccNotifyGlobalValidation__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        txid_t* in_0 = new txid_t;
        req->m >> *in_0;
        int32_t* in_1 = new int32_t;
        req->m >> *in_1;
        auto __marshal_reply__ = [=] {
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete in_1;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->RccNotifyGlobalValidation(*in_0, *in_1, __defer__);
    }
    void __JanusDispatch__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        std::vector<SimpleCommand>* in_0 = new std::vector<SimpleCommand>;
        req->m >> *in_0;
        rrr::i32* out_0 = new rrr::i32;
        TxnOutput* out_1 = new TxnOutput;
        MarshallDeputy* out_2 = new MarshallDeputy;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
            *sconn << *out_1;
            *sconn << *out_2;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
            delete out_1;
            delete out_2;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->JanusDispatch(*in_0, out_0, out_1, out_2, __defer__);
    }
    void __JanusCommit__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        cmdid_t* in_0 = new cmdid_t;
        req->m >> *in_0;
        rank_t* in_1 = new rank_t;
        req->m >> *in_1;
        int32_t* in_2 = new int32_t;
        req->m >> *in_2;
        MarshallDeputy* in_3 = new MarshallDeputy;
        req->m >> *in_3;
        int32_t* out_0 = new int32_t;
        TxnOutput* out_1 = new TxnOutput;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
            *sconn << *out_1;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete in_1;
            delete in_2;
            delete in_3;
            delete out_0;
            delete out_1;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->JanusCommit(*in_0, *in_1, *in_2, *in_3, out_0, out_1, __defer__);
    }
    void __JanusCommitWoGraph__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        cmdid_t* in_0 = new cmdid_t;
        req->m >> *in_0;
        rank_t* in_1 = new rank_t;
        req->m >> *in_1;
        int32_t* in_2 = new int32_t;
        req->m >> *in_2;
        int32_t* out_0 = new int32_t;
        TxnOutput* out_1 = new TxnOutput;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
            *sconn << *out_1;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete in_1;
            delete in_2;
            delete out_0;
            delete out_1;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->JanusCommitWoGraph(*in_0, *in_1, *in_2, out_0, out_1, __defer__);
    }
    void __JanusInquire__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        epoch_t* in_0 = new epoch_t;
        req->m >> *in_0;
        txnid_t* in_1 = new txnid_t;
        req->m >> *in_1;
        MarshallDeputy* out_0 = new MarshallDeputy;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete in_1;
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->JanusInquire(*in_0, *in_1, out_0, __defer__);
    }
    void __JanusPreAccept__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        cmdid_t* in_0 = new cmdid_t;
        req->m >> *in_0;
        rank_t* in_1 = new rank_t;
        req->m >> *in_1;
        std::vector<SimpleCommand>* in_2 = new std::vector<SimpleCommand>;
        req->m >> *in_2;
        MarshallDeputy* in_3 = new MarshallDeputy;
        req->m >> *in_3;
        rrr::i32* out_0 = new rrr::i32;
        MarshallDeputy* out_1 = new MarshallDeputy;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
            *sconn << *out_1;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete in_1;
            delete in_2;
            delete in_3;
            delete out_0;
            delete out_1;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->JanusPreAccept(*in_0, *in_1, *in_2, *in_3, out_0, out_1, __defer__);
    }
    void __JanusPreAcceptWoGraph__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        cmdid_t* in_0 = new cmdid_t;
        req->m >> *in_0;
        rank_t* in_1 = new rank_t;
        req->m >> *in_1;
        std::vector<SimpleCommand>* in_2 = new std::vector<SimpleCommand>;
        req->m >> *in_2;
        rrr::i32* out_0 = new rrr::i32;
        MarshallDeputy* out_1 = new MarshallDeputy;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
            *sconn << *out_1;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete in_1;
            delete in_2;
            delete out_0;
            delete out_1;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->JanusPreAcceptWoGraph(*in_0, *in_1, *in_2, out_0, out_1, __defer__);
    }
    void __JanusAccept__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        cmdid_t* in_0 = new cmdid_t;
        req->m >> *in_0;
        ballot_t* in_1 = new ballot_t;
        req->m >> *in_1;
        MarshallDeputy* in_2 = new MarshallDeputy;
        req->m >> *in_2;
        rrr::i32* out_0 = new rrr::i32;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete in_1;
            delete in_2;
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->JanusAccept(*in_0, *in_1, *in_2, out_0, __defer__);
    }
    void __PreAcceptFebruus__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        txid_t* in_0 = new txid_t;
        req->m >> *in_0;
        rrr::i32* out_0 = new rrr::i32;
        uint64_t* out_1 = new uint64_t;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
            *sconn << *out_1;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
            delete out_1;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->PreAcceptFebruus(*in_0, out_0, out_1, __defer__);
    }
    void __AcceptFebruus__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        txid_t* in_0 = new txid_t;
        req->m >> *in_0;
        ballot_t* in_1 = new ballot_t;
        req->m >> *in_1;
        uint64_t* in_2 = new uint64_t;
        req->m >> *in_2;
        rrr::i32* out_0 = new rrr::i32;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete in_1;
            delete in_2;
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->AcceptFebruus(*in_0, *in_1, *in_2, out_0, __defer__);
    }
    void __CommitFebruus__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        txid_t* in_0 = new txid_t;
        req->m >> *in_0;
        uint64_t* in_1 = new uint64_t;
        req->m >> *in_1;
        rrr::i32* out_0 = new rrr::i32;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete in_1;
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->CommitFebruus(*in_0, *in_1, out_0, __defer__);
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
    rrr::Future* async_Dispatch(const rrr::i64& tid, const MarshallDeputy& cmd, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::DISPATCH, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << tid;
            *__cl__ << cmd;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 Dispatch(const rrr::i64& tid, const MarshallDeputy& cmd, rrr::i32* res, TxnOutput* output) {
        rrr::Future* __fu__ = this->async_Dispatch(tid, cmd);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *res;
            __fu__->get_reply() >> *output;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_Prepare(const rrr::i64& tid, const std::vector<rrr::i32>& sids, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::PREPARE, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << tid;
            *__cl__ << sids;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 Prepare(const rrr::i64& tid, const std::vector<rrr::i32>& sids, rrr::i32* res) {
        rrr::Future* __fu__ = this->async_Prepare(tid, sids);
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
    rrr::Future* async_Commit(const rrr::i64& tid, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::COMMIT, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << tid;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 Commit(const rrr::i64& tid, rrr::i32* res) {
        rrr::Future* __fu__ = this->async_Commit(tid);
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
    rrr::Future* async_Abort(const rrr::i64& tid, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::ABORT, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << tid;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 Abort(const rrr::i64& tid, rrr::i32* res) {
        rrr::Future* __fu__ = this->async_Abort(tid);
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
    rrr::Future* async_TapirAccept(const uint64_t& cmd_id, const int64_t& ballot, const int32_t& decision, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::TAPIRACCEPT, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << cmd_id;
            *__cl__ << ballot;
            *__cl__ << decision;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 TapirAccept(const uint64_t& cmd_id, const int64_t& ballot, const int32_t& decision) {
        rrr::Future* __fu__ = this->async_TapirAccept(cmd_id, ballot, decision);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_TapirFastAccept(const uint64_t& cmd_id, const std::vector<SimpleCommand>& txn_cmds, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::TAPIRFASTACCEPT, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << cmd_id;
            *__cl__ << txn_cmds;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 TapirFastAccept(const uint64_t& cmd_id, const std::vector<SimpleCommand>& txn_cmds, rrr::i32* res) {
        rrr::Future* __fu__ = this->async_TapirFastAccept(cmd_id, txn_cmds);
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
    rrr::Future* async_TapirDecide(const uint64_t& cmd_id, const rrr::i32& commit, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::TAPIRDECIDE, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << cmd_id;
            *__cl__ << commit;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 TapirDecide(const uint64_t& cmd_id, const rrr::i32& commit) {
        rrr::Future* __fu__ = this->async_TapirDecide(cmd_id, commit);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_RccDispatch(const std::vector<SimpleCommand>& cmd, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::RCCDISPATCH, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << cmd;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 RccDispatch(const std::vector<SimpleCommand>& cmd, rrr::i32* res, TxnOutput* output, MarshallDeputy* md_graph) {
        rrr::Future* __fu__ = this->async_RccDispatch(cmd);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *res;
            __fu__->get_reply() >> *output;
            __fu__->get_reply() >> *md_graph;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_RccFinish(const cmdid_t& id, const MarshallDeputy& md_graph, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::RCCFINISH, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << id;
            *__cl__ << md_graph;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 RccFinish(const cmdid_t& id, const MarshallDeputy& md_graph, std::map<uint32_t, std::map<int32_t, Value>>* outputs) {
        rrr::Future* __fu__ = this->async_RccFinish(id, md_graph);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *outputs;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_RccInquire(const epoch_t& epoch, const txnid_t& txn_id, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::RCCINQUIRE, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << epoch;
            *__cl__ << txn_id;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 RccInquire(const epoch_t& epoch, const txnid_t& txn_id, MarshallDeputy* md_res_graph) {
        rrr::Future* __fu__ = this->async_RccInquire(epoch, txn_id);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *md_res_graph;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_RccDispatchRo(const SimpleCommand& cmd, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::RCCDISPATCHRO, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << cmd;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 RccDispatchRo(const SimpleCommand& cmd, std::map<rrr::i32, Value>* output) {
        rrr::Future* __fu__ = this->async_RccDispatchRo(cmd);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *output;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_RccInquireValidation(const txid_t& tx_id, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::RCCINQUIREVALIDATION, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << tx_id;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 RccInquireValidation(const txid_t& tx_id, int32_t* res) {
        rrr::Future* __fu__ = this->async_RccInquireValidation(tx_id);
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
    rrr::Future* async_RccNotifyGlobalValidation(const txid_t& tx_id, const int32_t& res, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::RCCNOTIFYGLOBALVALIDATION, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << tx_id;
            *__cl__ << res;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 RccNotifyGlobalValidation(const txid_t& tx_id, const int32_t& res) {
        rrr::Future* __fu__ = this->async_RccNotifyGlobalValidation(tx_id, res);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_JanusDispatch(const std::vector<SimpleCommand>& cmd, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::JANUSDISPATCH, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << cmd;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 JanusDispatch(const std::vector<SimpleCommand>& cmd, rrr::i32* res, TxnOutput* output, MarshallDeputy* ret_graph) {
        rrr::Future* __fu__ = this->async_JanusDispatch(cmd);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *res;
            __fu__->get_reply() >> *output;
            __fu__->get_reply() >> *ret_graph;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_JanusCommit(const cmdid_t& id, const rank_t& rank, const int32_t& need_validation, const MarshallDeputy& graph, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::JANUSCOMMIT, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << id;
            *__cl__ << rank;
            *__cl__ << need_validation;
            *__cl__ << graph;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 JanusCommit(const cmdid_t& id, const rank_t& rank, const int32_t& need_validation, const MarshallDeputy& graph, int32_t* res, TxnOutput* output) {
        rrr::Future* __fu__ = this->async_JanusCommit(id, rank, need_validation, graph);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *res;
            __fu__->get_reply() >> *output;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_JanusCommitWoGraph(const cmdid_t& id, const rank_t& rank, const int32_t& need_validation, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::JANUSCOMMITWOGRAPH, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << id;
            *__cl__ << rank;
            *__cl__ << need_validation;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 JanusCommitWoGraph(const cmdid_t& id, const rank_t& rank, const int32_t& need_validation, int32_t* res, TxnOutput* output) {
        rrr::Future* __fu__ = this->async_JanusCommitWoGraph(id, rank, need_validation);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *res;
            __fu__->get_reply() >> *output;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_JanusInquire(const epoch_t& epoch, const txnid_t& txn_id, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::JANUSINQUIRE, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << epoch;
            *__cl__ << txn_id;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 JanusInquire(const epoch_t& epoch, const txnid_t& txn_id, MarshallDeputy* ret_graph) {
        rrr::Future* __fu__ = this->async_JanusInquire(epoch, txn_id);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *ret_graph;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_JanusPreAccept(const cmdid_t& txn_id, const rank_t& rank, const std::vector<SimpleCommand>& cmd, const MarshallDeputy& graph, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::JANUSPREACCEPT, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << txn_id;
            *__cl__ << rank;
            *__cl__ << cmd;
            *__cl__ << graph;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 JanusPreAccept(const cmdid_t& txn_id, const rank_t& rank, const std::vector<SimpleCommand>& cmd, const MarshallDeputy& graph, rrr::i32* res, MarshallDeputy* ret_graph) {
        rrr::Future* __fu__ = this->async_JanusPreAccept(txn_id, rank, cmd, graph);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *res;
            __fu__->get_reply() >> *ret_graph;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_JanusPreAcceptWoGraph(const cmdid_t& txn_id, const rank_t& rank, const std::vector<SimpleCommand>& cmd, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::JANUSPREACCEPTWOGRAPH, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << txn_id;
            *__cl__ << rank;
            *__cl__ << cmd;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 JanusPreAcceptWoGraph(const cmdid_t& txn_id, const rank_t& rank, const std::vector<SimpleCommand>& cmd, rrr::i32* res, MarshallDeputy* ret_graph) {
        rrr::Future* __fu__ = this->async_JanusPreAcceptWoGraph(txn_id, rank, cmd);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *res;
            __fu__->get_reply() >> *ret_graph;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_JanusAccept(const cmdid_t& txn_id, const ballot_t& ballot, const MarshallDeputy& graph, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::JANUSACCEPT, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << txn_id;
            *__cl__ << ballot;
            *__cl__ << graph;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 JanusAccept(const cmdid_t& txn_id, const ballot_t& ballot, const MarshallDeputy& graph, rrr::i32* res) {
        rrr::Future* __fu__ = this->async_JanusAccept(txn_id, ballot, graph);
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
    rrr::Future* async_PreAcceptFebruus(const txid_t& tx_id, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::PREACCEPTFEBRUUS, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << tx_id;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 PreAcceptFebruus(const txid_t& tx_id, rrr::i32* ret, uint64_t* timestamp) {
        rrr::Future* __fu__ = this->async_PreAcceptFebruus(tx_id);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rrr::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *ret;
            __fu__->get_reply() >> *timestamp;
        }
        __fu__->release();
        return __ret__;
    }
    rrr::Future* async_AcceptFebruus(const txid_t& tx_id, const ballot_t& ballot, const uint64_t& timestamp, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::ACCEPTFEBRUUS, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << tx_id;
            *__cl__ << ballot;
            *__cl__ << timestamp;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 AcceptFebruus(const txid_t& tx_id, const ballot_t& ballot, const uint64_t& timestamp, rrr::i32* ret) {
        rrr::Future* __fu__ = this->async_AcceptFebruus(tx_id, ballot, timestamp);
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
    rrr::Future* async_CommitFebruus(const txid_t& tx_id, const uint64_t& timestamp, const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClassicService::COMMITFEBRUUS, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << tx_id;
            *__cl__ << timestamp;
        }
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 CommitFebruus(const txid_t& tx_id, const uint64_t& timestamp, rrr::i32* ret) {
        rrr::Future* __fu__ = this->async_CommitFebruus(tx_id, timestamp);
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
};

class ServerControlService: public rrr::Service {
public:
    enum {
        SERVER_SHUTDOWN = 0x190eb91f,
        SERVER_READY = 0x41ae209d,
        SERVER_HEART_BEAT_WITH_DATA = 0x393504f4,
        SERVER_HEART_BEAT = 0x1920fc51,
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
    // for 'raw' handlers, remember to reply req, delete req, and sconn->release(); use sconn->run_async for heavy job
    virtual void server_shutdown(rrr::DeferredReply* defer) = 0;
    virtual void server_ready(rrr::i32* res, rrr::DeferredReply* defer) = 0;
    virtual void server_heart_beat_with_data(ServerResponse* res, rrr::DeferredReply* defer) = 0;
    virtual void server_heart_beat(rrr::DeferredReply* defer) = 0;
private:
    void __server_shutdown__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        auto __marshal_reply__ = [=] {
        };
        auto __cleanup__ = [=] {
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->server_shutdown(__defer__);
    }
    void __server_ready__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        rrr::i32* out_0 = new rrr::i32;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
        };
        auto __cleanup__ = [=] {
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->server_ready(out_0, __defer__);
    }
    void __server_heart_beat_with_data__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        ServerResponse* out_0 = new ServerResponse;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
        };
        auto __cleanup__ = [=] {
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->server_heart_beat_with_data(out_0, __defer__);
    }
    void __server_heart_beat__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        auto __marshal_reply__ = [=] {
        };
        auto __cleanup__ = [=] {
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
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
        CLIENT_GET_TXN_NAMES = 0x20e359d6,
        CLIENT_SHUTDOWN = 0x2d619bad,
        CLIENT_FORCE_STOP = 0x596b5b38,
        CLIENT_RESPONSE = 0x3e9e633e,
        CLIENT_READY = 0x41451c54,
        CLIENT_READY_BLOCK = 0x2a473f8a,
        CLIENT_START = 0x2a2a7e21,
        DISPATCHTXN = 0x2e11b470,
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
    // for 'raw' handlers, remember to reply req, delete req, and sconn->release(); use sconn->run_async for heavy job
    virtual void client_get_txn_names(std::map<rrr::i32, std::string>* txn_names, rrr::DeferredReply* defer) = 0;
    virtual void client_shutdown(rrr::DeferredReply* defer) = 0;
    virtual void client_force_stop(rrr::DeferredReply* defer) = 0;
    virtual void client_response(ClientResponse* res, rrr::DeferredReply* defer) = 0;
    virtual void client_ready(rrr::i32* res, rrr::DeferredReply* defer) = 0;
    virtual void client_ready_block(rrr::i32* res, rrr::DeferredReply* defer) = 0;
    virtual void client_start(rrr::DeferredReply* defer) = 0;
    virtual void DispatchTxn(const TxDispatchRequest& req, TxReply* result, rrr::DeferredReply* defer) = 0;
private:
    void __client_get_txn_names__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        std::map<rrr::i32, std::string>* out_0 = new std::map<rrr::i32, std::string>;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
        };
        auto __cleanup__ = [=] {
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->client_get_txn_names(out_0, __defer__);
    }
    void __client_shutdown__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        auto __marshal_reply__ = [=] {
        };
        auto __cleanup__ = [=] {
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->client_shutdown(__defer__);
    }
    void __client_force_stop__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        auto __marshal_reply__ = [=] {
        };
        auto __cleanup__ = [=] {
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->client_force_stop(__defer__);
    }
    void __client_response__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        ClientResponse* out_0 = new ClientResponse;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
        };
        auto __cleanup__ = [=] {
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->client_response(out_0, __defer__);
    }
    void __client_ready__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        rrr::i32* out_0 = new rrr::i32;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
        };
        auto __cleanup__ = [=] {
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->client_ready(out_0, __defer__);
    }
    void __client_ready_block__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        rrr::i32* out_0 = new rrr::i32;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
        };
        auto __cleanup__ = [=] {
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->client_ready_block(out_0, __defer__);
    }
    void __client_start__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        auto __marshal_reply__ = [=] {
        };
        auto __cleanup__ = [=] {
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
        this->client_start(__defer__);
    }
    void __DispatchTxn__wrapper__(rrr::Request* req, rrr::ServerConnection* sconn) {
        TxDispatchRequest* in_0 = new TxDispatchRequest;
        req->m >> *in_0;
        TxReply* out_0 = new TxReply;
        auto __marshal_reply__ = [=] {
            *sconn << *out_0;
        };
        auto __cleanup__ = [=] {
            delete in_0;
            delete out_0;
        };
        rrr::DeferredReply* __defer__ = new rrr::DeferredReply(req, sconn, __marshal_reply__, __cleanup__);
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
    rrr::Future* async_client_response(const rrr::FutureAttr& __fu_attr__ = rrr::FutureAttr()) {
        rrr::Future* __fu__ = __cl__->begin_request(ClientControlService::CLIENT_RESPONSE, __fu_attr__);
        __cl__->end_request();
        return __fu__;
    }
    rrr::i32 client_response(ClientResponse* res) {
        rrr::Future* __fu__ = this->async_client_response();
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



