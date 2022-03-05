#pragma once

#include "__dep__.h"
#include "constants.h"
#include "../rcc/graph.h"
#include "../rcc/graph_marshaler.h"
#include "../command.h"
#include "deptran/procedure.h"
#include "../command_marshaler.h"
#include "../rcc_rpc.h"

class SimpleCommand;
namespace janus {

class TxLogServer;
class PaxosServer;
class MultiPaxosServiceImpl : public MultiPaxosService {
 public:
  PaxosServer* sched_;
  MultiPaxosServiceImpl(TxLogServer* sched);
  void Forward(const MarshallDeputy& cmd,
               rrr::DeferredReply* defer) override;

  void Prepare(const uint64_t& slot,
               const ballot_t& ballot,
               ballot_t* max_ballot,
               rrr::DeferredReply* defer) override;

  void Accept(const uint64_t& slot,
              const ballot_t& ballot,
              const MarshallDeputy& cmd,
              ballot_t* max_ballot,
              rrr::DeferredReply* defer) override;

  void Decide(const uint64_t& slot,
              const ballot_t& ballot,
              const MarshallDeputy& cmd,
              rrr::DeferredReply* defer) override;

  void BulkDecide(const MarshallDeputy& cmd,
                  i32* ballot,
                  i32* valid,
                  rrr::DeferredReply* defer) override;

  void BulkAccept(const MarshallDeputy& cmd,
                  i32* ballot,
                  i32* valid,
                  rrr::DeferredReply* defer) override;

  void BulkPrepare(const MarshallDeputy& cmd,
                  i32* ballot,
                  i32* valid,
                  rrr::DeferredReply* defer) override;

  void Heartbeat(const MarshallDeputy& cmd,
                  i32* ballot,
                  i32* valid,
                  rrr::DeferredReply* defer) override;

  void BulkPrepare2(const MarshallDeputy& md_cmd,
                     i32* ballot,
                     i32* valid,
                     MarshallDeputy* ret,
                     rrr::DeferredReply* defer) override;

  void SyncLog(const MarshallDeputy& md_cmd,
                     i32* ballot,
                     i32* valid,
                     MarshallDeputy* ret,
                     rrr::DeferredReply* defer) override;

  void SyncCommit(const MarshallDeputy& md_cmd,
                     i32* ballot,
                     i32* valid,
                     rrr::DeferredReply* defer) override;

  void SyncNoOps(const MarshallDeputy& md_cmd,
                 i32* ballot,
                 i32* valid,
                 rrr::DeferredReply* defer) override;

};

} // namespace janus
