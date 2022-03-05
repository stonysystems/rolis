
#include "../__dep__.h"
#include "../constants.h"
#include "coordinator.h"
#include "commo.h"
#include "paxos_worker.h"

namespace janus {

std::shared_ptr<ElectionState> es_cc = ElectionState::instance();

CoordinatorMultiPaxos::CoordinatorMultiPaxos(uint32_t coo_id,
                                             int32_t benchmark,
                                             ClientControlServiceImpl* ccsi,
                                            uint32_t thread_id)
    : Coordinator(coo_id, benchmark, ccsi, thread_id) {
}

BulkCoordinatorMultiPaxos::BulkCoordinatorMultiPaxos(uint32_t coo_id,
                                             int32_t benchmark,
                                             ClientControlServiceImpl* ccsi,
                                             uint32_t thread_id)
  : CoordinatorMultiPaxos(coo_id, benchmark, ccsi, thread_id) {
}

void CoordinatorMultiPaxos::Submit(shared_ptr<Marshallable>& cmd,
                                   const function<void()>& func,
                                   const function<void()>& exe_callback) {
  if (!IsLeader()) {
    Log_fatal("i am not the leader; site %d; locale %d",
              frame_->site_info_->id, loc_id_);
  }

  std::lock_guard<std::recursive_mutex> lock(mtx_);
  verify(!in_submission_);
  verify(cmd_ == nullptr);
//  verify(cmd.self_cmd_ != nullptr);
  in_submission_ = true;
  cmd_ = cmd;
  verify(cmd_->kind_ != MarshallDeputy::UNKNOWN);
  commit_callback_ = func;
  GotoNextPhase();
}

void BulkCoordinatorMultiPaxos::BulkSubmit(shared_ptr<Marshallable>& cmd,
                                       const function<void()>& func,
                                       const function<void()>& exe_callback) {
    /*if (!IsLeader()) {
        Log_fatal("i am not the leader; site %d; locale %d",
                  frame_->site_info_->id, loc_id_);
    }*/
    //std::lock_guard<std::recursive_mutex> lock(mtx_);
    verify(!in_submission_);
    in_submission_ = true;
    cmd_ = cmd;
    commit_callback_ = func;
    GotoNextPhase();
}

ballot_t CoordinatorMultiPaxos::PickBallot() {
  return curr_ballot_ + 1;
}

void CoordinatorMultiPaxos::Prepare() {
  //std::lock_guard<std::recursive_mutex> lock(mtx_);
  verify(0); // for debug;
  verify(!in_prepare_);
  in_prepare_ = true;
  curr_ballot_ = PickBallot();
  verify(slot_id_ > 0);
  Log_debug("multi-paxos coordinator broadcasts prepare, "
                "par_id_: %lx, slot_id: %llx",
            par_id_,
            slot_id_);
  verify(n_prepare_ack_ == 0);
  int n_replica = Config::GetConfig()->GetPartitionSize(par_id_);
  auto sp_quorum = commo()->BroadcastPrepare(par_id_, slot_id_, curr_ballot_);
  sp_quorum->Wait();
  if (sp_quorum->Yes()) {
    verify(!sp_quorum->HasAcceptedValue());
    // TODO use the previously accepted value.

  } else if (sp_quorum->No()) {
    // TODO restart prepare?
    verify(0);
  } else {
    // TODO timeout
    verify(0);
  }
//  commo()->BroadcastPrepare(par_id_,
//                            slot_id_,
//                            curr_ballot_,
//                            std::bind(&CoordinatorMultiPaxos::PrepareAck,
//                                      this,
//                                      phase_,
//                                      std::placeholders::_1));
//}
//
//void CoordinatorMultiPaxos::PrepareAck(phase_t phase, Future* fu) {
//  std::lock_guard<std::recursive_mutex> lock(mtx_);
//  if (phase_ != phase) return;
//  ballot_t max_ballot;
//  fu->get_reply() >> max_ballot;
//  if (max_ballot == curr_ballot_) {
//    n_prepare_ack_++;
//    verify(n_prepare_ack_ <= n_replica_);
//    if (n_prepare_ack_ >= GetQuorum()) {
//      GotoNextPhase();
//    }
//  } else {
//    if (max_ballot > curr_ballot_) {
//      curr_ballot_ = max_ballot + 1;
//      Log_debug("%s: saw greater ballot increment to %d",
//                __FUNCTION__, curr_ballot_);
//      phase_ = Phase::INIT_END;
//      GotoNextPhase();
//    } else {
////       max_ballot < curr_ballot ignore
//    }
//  }
}

void CoordinatorMultiPaxos::Accept() {
  //std::lock_guard<std::recursive_mutex> lock(mtx_);
  verify(!in_accept);
  in_accept = true;
  Log_debug("multi-paxos coordinator broadcasts accept, "
                "par_id_: %lx, slot_id: %llx",
            par_id_, slot_id_);
  auto sp_quorum = commo()->BroadcastAccept(par_id_, slot_id_, curr_ballot_, cmd_);
  sp_quorum->Wait();
  if (sp_quorum->Yes()) {
    committed_ = true;
  } else if (sp_quorum->No()) {
    // TODO process the case: failed to get a majority.
    verify(0);
  } else {
    // TODO process timeout.
    verify(0);
  }
//  commo()->BroadcastAccept(par_id_,
//                           slot_id_,
//                           curr_ballot_,
//                           cmd_,
//                           std::bind(&CoordinatorMultiPaxos::AcceptAck,
//                                     this,
//                                     phase_,
//                                     std::placeholders::_1));
//}
//
//void CoordinatorMultiPaxos::AcceptAck(phase_t phase, Future* fu) {
//  std::lock_guard<std::recursive_mutex> lock(mtx_);
//  if (phase_ > phase) return;
//  ballot_t max_ballot;
//  fu->get_reply() >> max_ballot;
//  if (max_ballot == curr_ballot_) {
//    n_finish_ack_++;
//    if (n_finish_ack_ >= GetQuorum()) {
//      committed_ = true;
//      GotoNextPhase();
//    }
//  } else {
//    if (max_ballot > curr_ballot_) {
//      curr_ballot_ = max_ballot + 1;
//      Log_debug("%s: saw greater ballot increment to %d",
//                __FUNCTION__, curr_ballot_);
//      phase_ = Phase::INIT_END;
//      GotoNextPhase();
//    } else {
//      // max_ballot < curr_ballot ignore
//    }
//  }
}

void CoordinatorMultiPaxos::Commit() {
  //std::lock_guard<std::recursive_mutex> lock(mtx_);
  commit_callback_();
  Log_debug("multi-paxos broadcast commit for partition: %d, slot %d",
            (int) par_id_, (int) slot_id_);
  commo()->BroadcastDecide(par_id_, slot_id_, curr_ballot_, cmd_);
  verify(phase_ == Phase::COMMIT);
  GotoNextPhase();
}

void CoordinatorMultiPaxos::GotoNextPhase() {
  int n_phase = 4;
  int current_phase = phase_ % n_phase;
  phase_++;
  switch (current_phase) {
    case Phase::INIT_END:
      if (IsLeader()) {
        phase_++; // skip prepare phase for "leader"
        verify(phase_ % n_phase == Phase::ACCEPT);
        Accept();
        phase_++;
        verify(phase_ % n_phase == Phase::COMMIT);
      } else {
        // TODO
        verify(0);
      }
    case Phase::ACCEPT:
      verify(phase_ % n_phase == Phase::COMMIT);
      if (committed_) {
        Commit();
      } else {
        verify(0);
      }
      break;
    case Phase::PREPARE:
      verify(phase_ % n_phase == Phase::ACCEPT);
      Accept();
      break;
    case Phase::COMMIT:
      // do nothing.
      break;
    default:
      verify(0);
  }
}

void BulkCoordinatorMultiPaxos::GotoNextPhase() {
  while(true){
    int n_phase = 4;
    int current_phase = phase_ % n_phase;
    phase_++;
    if(current_phase == Phase::INIT_END){
      if(phase_ > 3){
        break;
      }
      Prepare();
      if(!in_submission_){
        break;
      }
      phase_++;// need to do this because Phase::Dispatch = 1
    } else if(current_phase == Phase::ACCEPT){
      //Log_info("In accept mode");
      Accept();
      if(!in_submission_){
        break;
      }
    } else if(current_phase == Phase::COMMIT){
      Commit();
      if(!in_submission_){
        break;
      }
    }
  }
}

void BulkCoordinatorMultiPaxos::Prepare() {
  //std::lock_guard<std::recursive_mutex> lock(mtx_);
   in_prepare_ = true;
  // curr_ballot_ = PickBallot();
  // verify(slot_id_ > 0);
  // Log_debug("multi-paxos coordinator broadcasts prepare, "
  //               "par_id_: %lx, slot_id: %llx",
  //           par_id_,
  //           slot_id_);
  // verify(n_prepare_ack_ == 0);
  // int n_replica = Config::GetConfig()->GetPartitionSize(par_id_);
  //return;
  auto cmd_temp1 = dynamic_pointer_cast<BulkPaxosCmd>(cmd_);
  auto prep_cmd = make_shared<PaxosPrepCmd>();
  prep_cmd->slots = cmd_temp1->slots;
  prep_cmd->ballots = cmd_temp1->ballots;
  prep_cmd->leader_id = cmd_temp1->leader_id;

  auto prep_cmd_marshallable = dynamic_pointer_cast<Marshallable>(prep_cmd);

  //std::vector<pair<ballot_t, shared_ptr<Marshallable>>> vec_md;
  auto ess_cc = es_cc;
  if(es_cc->machine_id == 0)
	Log_debug("Sending paxos prepare request for slot %d and parition %d", cmd_temp1->slots[0], frame_->site_info_->partition_id_);
  auto sp_quorum = commo()->BroadcastPrepare2(par_id_, prep_cmd_marshallable, [this, ess_cc](MarshallDeputy md, ballot_t bt, int valid){
    if(!this->in_prepare_)
	     return;
    if(!valid){
      //Log_info("Invalid value received for prepare and leader steps down");
      //verify(0);
      ess_cc->step_down(bt);
      this->in_submission_ = false;
    } else{
      //Log_info("Valid value received for prepare %d", bt);
      if(valid == 1)
        this->vec_md.push_back(make_pair(bt, md.sp_data_));
      // Weihai: comment, this line will cause an seg fault
      //else
      //  this->vec_md.push_back(make_pair(bt, cmd_));
    }
  });
  sp_quorum->Wait();
  if (sp_quorum->Yes()) {
    //Log_info("The prepare is successfull");
    ballot_t candidate_b = 0;
    shared_ptr<Marshallable> candidate_val = nullptr;
    for(int i = 0; i < vec_md.size(); i++){
      if(vec_md[i].first > candidate_b){
        candidate_b = vec_md[i].first;
        candidate_val = vec_md[i].second;
      }
    }
    //auto cmd_temp1 = dynamic_pointer_cast<BulkPaxosCmd>(cmd_);
    if(candidate_val){
      auto cmd_temp = dynamic_pointer_cast<BulkPaxosCmd>(candidate_val);
      auto cmd_temp1 = dynamic_pointer_cast<BulkPaxosCmd>(cmd_);
      //cmd_temp1->cmds.clear();
      //cmd_temp1->cmds.push_back(cmd_temp->cmds[0]);
    }
    //Log_info("in submission ? %d", in_submission_);
    // Log_info("Should be in accept now for slot %d", cmd_temp1->slots[0]);
  } else if (sp_quorum->No()) {
    // TODO restart prepare?
    // verify(0);
    //.. not a leader anymore, exit.
  } else {
    // TODO timeout
    verify(0);
  }
  in_prepare_ = false;
}

void BulkCoordinatorMultiPaxos::Accept() {
    //std::lock_guard<std::recursive_mutex> lock(mtx_);
    //committed_ = true;
    //return;
    in_accept = true;
    auto cmd_temp1 = dynamic_pointer_cast<BulkPaxosCmd>(cmd_);
    // Log_info("Sending paxos accept request for slot %d", cmd_temp1->slots[0]);
    //Log_info("Accept: some slot is committed");
    if(!in_submission_){
      return;
    }
    auto ess_cc = es_cc;
    auto sp_quorum = commo()->BroadcastBulkAccept(par_id_, cmd_, [this, ess_cc](ballot_t ballot, int valid){
      if(!this->in_accept)
	       return;
      if(!valid){
	         verify(0);
        ess_cc->step_down(ballot);
        this->in_submission_ = false;
      }
    });
    sp_quorum->Wait();
    if (sp_quorum->Yes()) {
	      if(ess_cc->machine_id == 0)
			Log_debug("Accept: slot %d  is committed, parition id %d", cmd_temp1->slots[0], frame_->site_info_->partition_id_);
        committed_ = true;
    } else if (sp_quorum->No()) {
        in_submission_ = false;
        return;
    } else {
        verify(0);
    }
    in_accept = false;
}

void BulkCoordinatorMultiPaxos::Commit() {
    //std::lock_guard<std::recursive_mutex> lock(mtx_);
    //commit_callback_();
    //return;
    if(!in_submission_){
      return;
    }

    in_commit = true;

    auto cmd_temp1 = dynamic_pointer_cast<BulkPaxosCmd>(cmd_);
    auto commit_cmd = make_shared<PaxosPrepCmd>();
    commit_cmd->slots = cmd_temp1->slots;
    commit_cmd->ballots = cmd_temp1->ballots;
    commit_cmd->leader_id = cmd_temp1->leader_id;

    auto commit_cmd_marshallable = dynamic_pointer_cast<Marshallable>(commit_cmd);


    auto ess_cc = es_cc;
    auto sp_quorum = commo()->BroadcastBulkDecide(par_id_, commit_cmd_marshallable, [this, ess_cc](ballot_t ballot, int valid){
      if(!this->in_commit){
        return;
      }
      if(!valid){
        ess_cc->step_down(ballot);
        this->in_submission_ = false;
      }
    });
    sp_quorum->Wait();
    if (sp_quorum->Yes()) {
	//Log_info("Commit: some stuff is committed");
    } else if (sp_quorum->No()) {
      in_submission_ = false;
      return;
    } else {
      verify(0);
    }
    in_commit = false;
    //verify(phase_ == Phase::COMMIT);
    commit_callback_();
    //GotoNextPhase();
}

} // namespace janus
