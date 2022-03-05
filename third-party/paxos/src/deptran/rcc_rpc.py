import os
from simplerpc.marshal import Marshal
from simplerpc.future import Future

ValueTimesPair = Marshal.reg_type('ValueTimesPair', [('value', 'rrr::i64'), ('times', 'rrr::i64')])

TxnInfoRes = Marshal.reg_type('TxnInfoRes', [('start_txn', 'rrr::i32'), ('total_txn', 'rrr::i32'), ('total_try', 'rrr::i32'), ('commit_txn', 'rrr::i32'), ('num_exhausted', 'rrr::i32'), ('this_latency', 'std::vector<double>'), ('last_latency', 'std::vector<double>'), ('attempt_latency', 'std::vector<double>'), ('interval_latency', 'std::vector<double>'), ('all_interval_latency', 'std::vector<double>'), ('num_try', 'std::vector<rrr::i32>')])

ServerResponse = Marshal.reg_type('ServerResponse', [('statistics', 'std::map<std::string, ValueTimesPair>'), ('cpu_util', 'double'), ('r_cnt_sum', 'rrr::i64'), ('r_cnt_num', 'rrr::i64'), ('r_sz_sum', 'rrr::i64'), ('r_sz_num', 'rrr::i64')])

ClientResponse = Marshal.reg_type('ClientResponse', [('txn_info', 'std::map<rrr::i32, TxnInfoRes>'), ('run_sec', 'rrr::i64'), ('run_nsec', 'rrr::i64'), ('period_sec', 'rrr::i64'), ('period_nsec', 'rrr::i64'), ('is_finish', 'rrr::i32'), ('n_asking', 'rrr::i64')])

TxDispatchRequest = Marshal.reg_type('TxDispatchRequest', [('id', 'rrr::i32'), ('tx_type', 'rrr::i32'), ('input', 'std::vector<Value>')])

TxnDispatchResponse = Marshal.reg_type('TxnDispatchResponse', [])

class MultiPaxosService(object):
    FORWARD = 0x1f300b37
    PREPARE = 0x3ed339bc
    ACCEPT = 0x2c1aa5e7
    DECIDE = 0x1e2df369
    HEARTBEAT = 0x249987b3
    BULKPREPARE = 0x3094a906
    BULKACCEPT = 0x58ffece4
    BULKPREPARE2 = 0x245ecf49
    SYNCLOG = 0x692edba6
    SYNCCOMMIT = 0x54c77d9a
    SYNCNOOPS = 0x23b1c916
    BULKDECIDE = 0x3ede1dac

    __input_type_info__ = {
        'Forward': ['MarshallDeputy'],
        'Prepare': ['uint64_t','ballot_t'],
        'Accept': ['uint64_t','ballot_t','MarshallDeputy'],
        'Decide': ['uint64_t','ballot_t','MarshallDeputy'],
        'Heartbeat': ['MarshallDeputy'],
        'BulkPrepare': ['MarshallDeputy'],
        'BulkAccept': ['MarshallDeputy'],
        'BulkPrepare2': ['MarshallDeputy'],
        'SyncLog': ['MarshallDeputy'],
        'SyncCommit': ['MarshallDeputy'],
        'SyncNoOps': ['MarshallDeputy'],
        'BulkDecide': ['MarshallDeputy'],
    }

    __output_type_info__ = {
        'Forward': [],
        'Prepare': ['ballot_t'],
        'Accept': ['ballot_t'],
        'Decide': [],
        'Heartbeat': ['rrr::i32','rrr::i32'],
        'BulkPrepare': ['rrr::i32','rrr::i32'],
        'BulkAccept': ['rrr::i32','rrr::i32'],
        'BulkPrepare2': ['rrr::i32','rrr::i32','MarshallDeputy'],
        'SyncLog': ['rrr::i32','rrr::i32','MarshallDeputy'],
        'SyncCommit': ['rrr::i32','rrr::i32'],
        'SyncNoOps': ['rrr::i32','rrr::i32'],
        'BulkDecide': ['rrr::i32','rrr::i32'],
    }

    def __bind_helper__(self, func):
        def f(*args):
            return getattr(self, func.__name__)(*args)
        return f

    def __reg_to__(self, server):
        server.__reg_func__(MultiPaxosService.FORWARD, self.__bind_helper__(self.Forward), ['MarshallDeputy'], [])
        server.__reg_func__(MultiPaxosService.PREPARE, self.__bind_helper__(self.Prepare), ['uint64_t','ballot_t'], ['ballot_t'])
        server.__reg_func__(MultiPaxosService.ACCEPT, self.__bind_helper__(self.Accept), ['uint64_t','ballot_t','MarshallDeputy'], ['ballot_t'])
        server.__reg_func__(MultiPaxosService.DECIDE, self.__bind_helper__(self.Decide), ['uint64_t','ballot_t','MarshallDeputy'], [])
        server.__reg_func__(MultiPaxosService.HEARTBEAT, self.__bind_helper__(self.Heartbeat), ['MarshallDeputy'], ['rrr::i32','rrr::i32'])
        server.__reg_func__(MultiPaxosService.BULKPREPARE, self.__bind_helper__(self.BulkPrepare), ['MarshallDeputy'], ['rrr::i32','rrr::i32'])
        server.__reg_func__(MultiPaxosService.BULKACCEPT, self.__bind_helper__(self.BulkAccept), ['MarshallDeputy'], ['rrr::i32','rrr::i32'])
        server.__reg_func__(MultiPaxosService.BULKPREPARE2, self.__bind_helper__(self.BulkPrepare2), ['MarshallDeputy'], ['rrr::i32','rrr::i32','MarshallDeputy'])
        server.__reg_func__(MultiPaxosService.SYNCLOG, self.__bind_helper__(self.SyncLog), ['MarshallDeputy'], ['rrr::i32','rrr::i32','MarshallDeputy'])
        server.__reg_func__(MultiPaxosService.SYNCCOMMIT, self.__bind_helper__(self.SyncCommit), ['MarshallDeputy'], ['rrr::i32','rrr::i32'])
        server.__reg_func__(MultiPaxosService.SYNCNOOPS, self.__bind_helper__(self.SyncNoOps), ['MarshallDeputy'], ['rrr::i32','rrr::i32'])
        server.__reg_func__(MultiPaxosService.BULKDECIDE, self.__bind_helper__(self.BulkDecide), ['MarshallDeputy'], ['rrr::i32','rrr::i32'])

    def Forward(__self__, cmd):
        raise NotImplementedError('subclass MultiPaxosService and implement your own Forward function')

    def Prepare(__self__, slot, ballot):
        raise NotImplementedError('subclass MultiPaxosService and implement your own Prepare function')

    def Accept(__self__, slot, ballot, cmd):
        raise NotImplementedError('subclass MultiPaxosService and implement your own Accept function')

    def Decide(__self__, slot, ballot, cmd):
        raise NotImplementedError('subclass MultiPaxosService and implement your own Decide function')

    def Heartbeat(__self__, cmd):
        raise NotImplementedError('subclass MultiPaxosService and implement your own Heartbeat function')

    def BulkPrepare(__self__, cmd):
        raise NotImplementedError('subclass MultiPaxosService and implement your own BulkPrepare function')

    def BulkAccept(__self__, cmd):
        raise NotImplementedError('subclass MultiPaxosService and implement your own BulkAccept function')

    def BulkPrepare2(__self__, cmd):
        raise NotImplementedError('subclass MultiPaxosService and implement your own BulkPrepare2 function')

    def SyncLog(__self__, cmd):
        raise NotImplementedError('subclass MultiPaxosService and implement your own SyncLog function')

    def SyncCommit(__self__, cmd):
        raise NotImplementedError('subclass MultiPaxosService and implement your own SyncCommit function')

    def SyncNoOps(__self__, cmd):
        raise NotImplementedError('subclass MultiPaxosService and implement your own SyncNoOps function')

    def BulkDecide(__self__, cmd):
        raise NotImplementedError('subclass MultiPaxosService and implement your own BulkDecide function')

class MultiPaxosProxy(object):
    def __init__(self, clnt):
        self.__clnt__ = clnt

    def async_Forward(__self__, cmd):
        return __self__.__clnt__.async_call(MultiPaxosService.FORWARD, [cmd], MultiPaxosService.__input_type_info__['Forward'], MultiPaxosService.__output_type_info__['Forward'])

    def async_Prepare(__self__, slot, ballot):
        return __self__.__clnt__.async_call(MultiPaxosService.PREPARE, [slot, ballot], MultiPaxosService.__input_type_info__['Prepare'], MultiPaxosService.__output_type_info__['Prepare'])

    def async_Accept(__self__, slot, ballot, cmd):
        return __self__.__clnt__.async_call(MultiPaxosService.ACCEPT, [slot, ballot, cmd], MultiPaxosService.__input_type_info__['Accept'], MultiPaxosService.__output_type_info__['Accept'])

    def async_Decide(__self__, slot, ballot, cmd):
        return __self__.__clnt__.async_call(MultiPaxosService.DECIDE, [slot, ballot, cmd], MultiPaxosService.__input_type_info__['Decide'], MultiPaxosService.__output_type_info__['Decide'])

    def async_Heartbeat(__self__, cmd):
        return __self__.__clnt__.async_call(MultiPaxosService.HEARTBEAT, [cmd], MultiPaxosService.__input_type_info__['Heartbeat'], MultiPaxosService.__output_type_info__['Heartbeat'])

    def async_BulkPrepare(__self__, cmd):
        return __self__.__clnt__.async_call(MultiPaxosService.BULKPREPARE, [cmd], MultiPaxosService.__input_type_info__['BulkPrepare'], MultiPaxosService.__output_type_info__['BulkPrepare'])

    def async_BulkAccept(__self__, cmd):
        return __self__.__clnt__.async_call(MultiPaxosService.BULKACCEPT, [cmd], MultiPaxosService.__input_type_info__['BulkAccept'], MultiPaxosService.__output_type_info__['BulkAccept'])

    def async_BulkPrepare2(__self__, cmd):
        return __self__.__clnt__.async_call(MultiPaxosService.BULKPREPARE2, [cmd], MultiPaxosService.__input_type_info__['BulkPrepare2'], MultiPaxosService.__output_type_info__['BulkPrepare2'])

    def async_SyncLog(__self__, cmd):
        return __self__.__clnt__.async_call(MultiPaxosService.SYNCLOG, [cmd], MultiPaxosService.__input_type_info__['SyncLog'], MultiPaxosService.__output_type_info__['SyncLog'])

    def async_SyncCommit(__self__, cmd):
        return __self__.__clnt__.async_call(MultiPaxosService.SYNCCOMMIT, [cmd], MultiPaxosService.__input_type_info__['SyncCommit'], MultiPaxosService.__output_type_info__['SyncCommit'])

    def async_SyncNoOps(__self__, cmd):
        return __self__.__clnt__.async_call(MultiPaxosService.SYNCNOOPS, [cmd], MultiPaxosService.__input_type_info__['SyncNoOps'], MultiPaxosService.__output_type_info__['SyncNoOps'])

    def async_BulkDecide(__self__, cmd):
        return __self__.__clnt__.async_call(MultiPaxosService.BULKDECIDE, [cmd], MultiPaxosService.__input_type_info__['BulkDecide'], MultiPaxosService.__output_type_info__['BulkDecide'])

    def sync_Forward(__self__, cmd):
        __result__ = __self__.__clnt__.sync_call(MultiPaxosService.FORWARD, [cmd], MultiPaxosService.__input_type_info__['Forward'], MultiPaxosService.__output_type_info__['Forward'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_Prepare(__self__, slot, ballot):
        __result__ = __self__.__clnt__.sync_call(MultiPaxosService.PREPARE, [slot, ballot], MultiPaxosService.__input_type_info__['Prepare'], MultiPaxosService.__output_type_info__['Prepare'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_Accept(__self__, slot, ballot, cmd):
        __result__ = __self__.__clnt__.sync_call(MultiPaxosService.ACCEPT, [slot, ballot, cmd], MultiPaxosService.__input_type_info__['Accept'], MultiPaxosService.__output_type_info__['Accept'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_Decide(__self__, slot, ballot, cmd):
        __result__ = __self__.__clnt__.sync_call(MultiPaxosService.DECIDE, [slot, ballot, cmd], MultiPaxosService.__input_type_info__['Decide'], MultiPaxosService.__output_type_info__['Decide'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_Heartbeat(__self__, cmd):
        __result__ = __self__.__clnt__.sync_call(MultiPaxosService.HEARTBEAT, [cmd], MultiPaxosService.__input_type_info__['Heartbeat'], MultiPaxosService.__output_type_info__['Heartbeat'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_BulkPrepare(__self__, cmd):
        __result__ = __self__.__clnt__.sync_call(MultiPaxosService.BULKPREPARE, [cmd], MultiPaxosService.__input_type_info__['BulkPrepare'], MultiPaxosService.__output_type_info__['BulkPrepare'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_BulkAccept(__self__, cmd):
        __result__ = __self__.__clnt__.sync_call(MultiPaxosService.BULKACCEPT, [cmd], MultiPaxosService.__input_type_info__['BulkAccept'], MultiPaxosService.__output_type_info__['BulkAccept'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_BulkPrepare2(__self__, cmd):
        __result__ = __self__.__clnt__.sync_call(MultiPaxosService.BULKPREPARE2, [cmd], MultiPaxosService.__input_type_info__['BulkPrepare2'], MultiPaxosService.__output_type_info__['BulkPrepare2'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_SyncLog(__self__, cmd):
        __result__ = __self__.__clnt__.sync_call(MultiPaxosService.SYNCLOG, [cmd], MultiPaxosService.__input_type_info__['SyncLog'], MultiPaxosService.__output_type_info__['SyncLog'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_SyncCommit(__self__, cmd):
        __result__ = __self__.__clnt__.sync_call(MultiPaxosService.SYNCCOMMIT, [cmd], MultiPaxosService.__input_type_info__['SyncCommit'], MultiPaxosService.__output_type_info__['SyncCommit'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_SyncNoOps(__self__, cmd):
        __result__ = __self__.__clnt__.sync_call(MultiPaxosService.SYNCNOOPS, [cmd], MultiPaxosService.__input_type_info__['SyncNoOps'], MultiPaxosService.__output_type_info__['SyncNoOps'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_BulkDecide(__self__, cmd):
        __result__ = __self__.__clnt__.sync_call(MultiPaxosService.BULKDECIDE, [cmd], MultiPaxosService.__input_type_info__['BulkDecide'], MultiPaxosService.__output_type_info__['BulkDecide'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

class ClassicService(object):
    MSGSTRING = 0x1d0eebab
    MSGMARSHALL = 0x56332c42
    DISPATCH = 0x4eeae560
    PREPARE = 0x12c808be
    COMMIT = 0x3f5f50ce
    ABORT = 0x35c4289d
    UPGRADEEPOCH = 0x606ad021
    TRUNCATEEPOCH = 0x5aa4b85a
    RPC_NULL = 0x62b4c084
    TAPIRACCEPT = 0x386f2e01
    TAPIRFASTACCEPT = 0x2800f924
    TAPIRDECIDE = 0x3b2877f4
    RCCDISPATCH = 0x137064c0
    RCCFINISH = 0x14436ea6
    RCCINQUIRE = 0x4177fa53
    RCCDISPATCHRO = 0x5bc4ebfa
    RCCINQUIREVALIDATION = 0x4611385a
    RCCNOTIFYGLOBALVALIDATION = 0x1cae978d
    JANUSDISPATCH = 0x2dcc423d
    JANUSCOMMIT = 0x6f9254e7
    JANUSCOMMITWOGRAPH = 0x1b3241f8
    JANUSINQUIRE = 0x5ce268e9
    JANUSPREACCEPT = 0x6a9a671f
    JANUSPREACCEPTWOGRAPH = 0x10fe58f2
    JANUSACCEPT = 0x158460fa
    PREACCEPTFEBRUUS = 0x65ea71d0
    ACCEPTFEBRUUS = 0x36085b2c
    COMMITFEBRUUS = 0x551bbfaf

    __input_type_info__ = {
        'MsgString': ['std::string'],
        'MsgMarshall': ['MarshallDeputy'],
        'Dispatch': ['rrr::i64','MarshallDeputy'],
        'Prepare': ['rrr::i64','std::vector<rrr::i32>'],
        'Commit': ['rrr::i64'],
        'Abort': ['rrr::i64'],
        'UpgradeEpoch': ['uint32_t'],
        'TruncateEpoch': ['uint32_t'],
        'rpc_null': [],
        'TapirAccept': ['uint64_t','int64_t','int32_t'],
        'TapirFastAccept': ['uint64_t','std::vector<SimpleCommand>'],
        'TapirDecide': ['uint64_t','rrr::i32'],
        'RccDispatch': ['std::vector<SimpleCommand>'],
        'RccFinish': ['cmdid_t','MarshallDeputy'],
        'RccInquire': ['epoch_t','txnid_t'],
        'RccDispatchRo': ['SimpleCommand'],
        'RccInquireValidation': ['txid_t'],
        'RccNotifyGlobalValidation': ['txid_t','int32_t'],
        'JanusDispatch': ['std::vector<SimpleCommand>'],
        'JanusCommit': ['cmdid_t','rank_t','int32_t','MarshallDeputy'],
        'JanusCommitWoGraph': ['cmdid_t','rank_t','int32_t'],
        'JanusInquire': ['epoch_t','txnid_t'],
        'JanusPreAccept': ['cmdid_t','rank_t','std::vector<SimpleCommand>','MarshallDeputy'],
        'JanusPreAcceptWoGraph': ['cmdid_t','rank_t','std::vector<SimpleCommand>'],
        'JanusAccept': ['cmdid_t','ballot_t','MarshallDeputy'],
        'PreAcceptFebruus': ['txid_t'],
        'AcceptFebruus': ['txid_t','ballot_t','uint64_t'],
        'CommitFebruus': ['txid_t','uint64_t'],
    }

    __output_type_info__ = {
        'MsgString': ['std::string'],
        'MsgMarshall': ['MarshallDeputy'],
        'Dispatch': ['rrr::i32','TxnOutput'],
        'Prepare': ['rrr::i32'],
        'Commit': ['rrr::i32'],
        'Abort': ['rrr::i32'],
        'UpgradeEpoch': ['int32_t'],
        'TruncateEpoch': [],
        'rpc_null': [],
        'TapirAccept': [],
        'TapirFastAccept': ['rrr::i32'],
        'TapirDecide': [],
        'RccDispatch': ['rrr::i32','TxnOutput','MarshallDeputy'],
        'RccFinish': ['std::map<uint32_t, std::map<int32_t, Value>>'],
        'RccInquire': ['MarshallDeputy'],
        'RccDispatchRo': ['std::map<rrr::i32, Value>'],
        'RccInquireValidation': ['int32_t'],
        'RccNotifyGlobalValidation': [],
        'JanusDispatch': ['rrr::i32','TxnOutput','MarshallDeputy'],
        'JanusCommit': ['int32_t','TxnOutput'],
        'JanusCommitWoGraph': ['int32_t','TxnOutput'],
        'JanusInquire': ['MarshallDeputy'],
        'JanusPreAccept': ['rrr::i32','MarshallDeputy'],
        'JanusPreAcceptWoGraph': ['rrr::i32','MarshallDeputy'],
        'JanusAccept': ['rrr::i32'],
        'PreAcceptFebruus': ['rrr::i32','uint64_t'],
        'AcceptFebruus': ['rrr::i32'],
        'CommitFebruus': ['rrr::i32'],
    }

    def __bind_helper__(self, func):
        def f(*args):
            return getattr(self, func.__name__)(*args)
        return f

    def __reg_to__(self, server):
        server.__reg_func__(ClassicService.MSGSTRING, self.__bind_helper__(self.MsgString), ['std::string'], ['std::string'])
        server.__reg_func__(ClassicService.MSGMARSHALL, self.__bind_helper__(self.MsgMarshall), ['MarshallDeputy'], ['MarshallDeputy'])
        server.__reg_func__(ClassicService.DISPATCH, self.__bind_helper__(self.Dispatch), ['rrr::i64','MarshallDeputy'], ['rrr::i32','TxnOutput'])
        server.__reg_func__(ClassicService.PREPARE, self.__bind_helper__(self.Prepare), ['rrr::i64','std::vector<rrr::i32>'], ['rrr::i32'])
        server.__reg_func__(ClassicService.COMMIT, self.__bind_helper__(self.Commit), ['rrr::i64'], ['rrr::i32'])
        server.__reg_func__(ClassicService.ABORT, self.__bind_helper__(self.Abort), ['rrr::i64'], ['rrr::i32'])
        server.__reg_func__(ClassicService.UPGRADEEPOCH, self.__bind_helper__(self.UpgradeEpoch), ['uint32_t'], ['int32_t'])
        server.__reg_func__(ClassicService.TRUNCATEEPOCH, self.__bind_helper__(self.TruncateEpoch), ['uint32_t'], [])
        server.__reg_func__(ClassicService.RPC_NULL, self.__bind_helper__(self.rpc_null), [], [])
        server.__reg_func__(ClassicService.TAPIRACCEPT, self.__bind_helper__(self.TapirAccept), ['uint64_t','int64_t','int32_t'], [])
        server.__reg_func__(ClassicService.TAPIRFASTACCEPT, self.__bind_helper__(self.TapirFastAccept), ['uint64_t','std::vector<SimpleCommand>'], ['rrr::i32'])
        server.__reg_func__(ClassicService.TAPIRDECIDE, self.__bind_helper__(self.TapirDecide), ['uint64_t','rrr::i32'], [])
        server.__reg_func__(ClassicService.RCCDISPATCH, self.__bind_helper__(self.RccDispatch), ['std::vector<SimpleCommand>'], ['rrr::i32','TxnOutput','MarshallDeputy'])
        server.__reg_func__(ClassicService.RCCFINISH, self.__bind_helper__(self.RccFinish), ['cmdid_t','MarshallDeputy'], ['std::map<uint32_t, std::map<int32_t, Value>>'])
        server.__reg_func__(ClassicService.RCCINQUIRE, self.__bind_helper__(self.RccInquire), ['epoch_t','txnid_t'], ['MarshallDeputy'])
        server.__reg_func__(ClassicService.RCCDISPATCHRO, self.__bind_helper__(self.RccDispatchRo), ['SimpleCommand'], ['std::map<rrr::i32, Value>'])
        server.__reg_func__(ClassicService.RCCINQUIREVALIDATION, self.__bind_helper__(self.RccInquireValidation), ['txid_t'], ['int32_t'])
        server.__reg_func__(ClassicService.RCCNOTIFYGLOBALVALIDATION, self.__bind_helper__(self.RccNotifyGlobalValidation), ['txid_t','int32_t'], [])
        server.__reg_func__(ClassicService.JANUSDISPATCH, self.__bind_helper__(self.JanusDispatch), ['std::vector<SimpleCommand>'], ['rrr::i32','TxnOutput','MarshallDeputy'])
        server.__reg_func__(ClassicService.JANUSCOMMIT, self.__bind_helper__(self.JanusCommit), ['cmdid_t','rank_t','int32_t','MarshallDeputy'], ['int32_t','TxnOutput'])
        server.__reg_func__(ClassicService.JANUSCOMMITWOGRAPH, self.__bind_helper__(self.JanusCommitWoGraph), ['cmdid_t','rank_t','int32_t'], ['int32_t','TxnOutput'])
        server.__reg_func__(ClassicService.JANUSINQUIRE, self.__bind_helper__(self.JanusInquire), ['epoch_t','txnid_t'], ['MarshallDeputy'])
        server.__reg_func__(ClassicService.JANUSPREACCEPT, self.__bind_helper__(self.JanusPreAccept), ['cmdid_t','rank_t','std::vector<SimpleCommand>','MarshallDeputy'], ['rrr::i32','MarshallDeputy'])
        server.__reg_func__(ClassicService.JANUSPREACCEPTWOGRAPH, self.__bind_helper__(self.JanusPreAcceptWoGraph), ['cmdid_t','rank_t','std::vector<SimpleCommand>'], ['rrr::i32','MarshallDeputy'])
        server.__reg_func__(ClassicService.JANUSACCEPT, self.__bind_helper__(self.JanusAccept), ['cmdid_t','ballot_t','MarshallDeputy'], ['rrr::i32'])
        server.__reg_func__(ClassicService.PREACCEPTFEBRUUS, self.__bind_helper__(self.PreAcceptFebruus), ['txid_t'], ['rrr::i32','uint64_t'])
        server.__reg_func__(ClassicService.ACCEPTFEBRUUS, self.__bind_helper__(self.AcceptFebruus), ['txid_t','ballot_t','uint64_t'], ['rrr::i32'])
        server.__reg_func__(ClassicService.COMMITFEBRUUS, self.__bind_helper__(self.CommitFebruus), ['txid_t','uint64_t'], ['rrr::i32'])

    def MsgString(__self__, arg):
        raise NotImplementedError('subclass ClassicService and implement your own MsgString function')

    def MsgMarshall(__self__, arg):
        raise NotImplementedError('subclass ClassicService and implement your own MsgMarshall function')

    def Dispatch(__self__, tid, cmd):
        raise NotImplementedError('subclass ClassicService and implement your own Dispatch function')

    def Prepare(__self__, tid, sids):
        raise NotImplementedError('subclass ClassicService and implement your own Prepare function')

    def Commit(__self__, tid):
        raise NotImplementedError('subclass ClassicService and implement your own Commit function')

    def Abort(__self__, tid):
        raise NotImplementedError('subclass ClassicService and implement your own Abort function')

    def UpgradeEpoch(__self__, curr_epoch):
        raise NotImplementedError('subclass ClassicService and implement your own UpgradeEpoch function')

    def TruncateEpoch(__self__, old_epoch):
        raise NotImplementedError('subclass ClassicService and implement your own TruncateEpoch function')

    def rpc_null(__self__):
        raise NotImplementedError('subclass ClassicService and implement your own rpc_null function')

    def TapirAccept(__self__, cmd_id, ballot, decision):
        raise NotImplementedError('subclass ClassicService and implement your own TapirAccept function')

    def TapirFastAccept(__self__, cmd_id, txn_cmds):
        raise NotImplementedError('subclass ClassicService and implement your own TapirFastAccept function')

    def TapirDecide(__self__, cmd_id, commit):
        raise NotImplementedError('subclass ClassicService and implement your own TapirDecide function')

    def RccDispatch(__self__, cmd):
        raise NotImplementedError('subclass ClassicService and implement your own RccDispatch function')

    def RccFinish(__self__, id, md_graph):
        raise NotImplementedError('subclass ClassicService and implement your own RccFinish function')

    def RccInquire(__self__, epoch, txn_id):
        raise NotImplementedError('subclass ClassicService and implement your own RccInquire function')

    def RccDispatchRo(__self__, cmd):
        raise NotImplementedError('subclass ClassicService and implement your own RccDispatchRo function')

    def RccInquireValidation(__self__, tx_id):
        raise NotImplementedError('subclass ClassicService and implement your own RccInquireValidation function')

    def RccNotifyGlobalValidation(__self__, tx_id, res):
        raise NotImplementedError('subclass ClassicService and implement your own RccNotifyGlobalValidation function')

    def JanusDispatch(__self__, cmd):
        raise NotImplementedError('subclass ClassicService and implement your own JanusDispatch function')

    def JanusCommit(__self__, id, rank, need_validation, graph):
        raise NotImplementedError('subclass ClassicService and implement your own JanusCommit function')

    def JanusCommitWoGraph(__self__, id, rank, need_validation):
        raise NotImplementedError('subclass ClassicService and implement your own JanusCommitWoGraph function')

    def JanusInquire(__self__, epoch, txn_id):
        raise NotImplementedError('subclass ClassicService and implement your own JanusInquire function')

    def JanusPreAccept(__self__, txn_id, rank, cmd, graph):
        raise NotImplementedError('subclass ClassicService and implement your own JanusPreAccept function')

    def JanusPreAcceptWoGraph(__self__, txn_id, rank, cmd):
        raise NotImplementedError('subclass ClassicService and implement your own JanusPreAcceptWoGraph function')

    def JanusAccept(__self__, txn_id, ballot, graph):
        raise NotImplementedError('subclass ClassicService and implement your own JanusAccept function')

    def PreAcceptFebruus(__self__, tx_id):
        raise NotImplementedError('subclass ClassicService and implement your own PreAcceptFebruus function')

    def AcceptFebruus(__self__, tx_id, ballot, timestamp):
        raise NotImplementedError('subclass ClassicService and implement your own AcceptFebruus function')

    def CommitFebruus(__self__, tx_id, timestamp):
        raise NotImplementedError('subclass ClassicService and implement your own CommitFebruus function')

class ClassicProxy(object):
    def __init__(self, clnt):
        self.__clnt__ = clnt

    def async_MsgString(__self__, arg):
        return __self__.__clnt__.async_call(ClassicService.MSGSTRING, [arg], ClassicService.__input_type_info__['MsgString'], ClassicService.__output_type_info__['MsgString'])

    def async_MsgMarshall(__self__, arg):
        return __self__.__clnt__.async_call(ClassicService.MSGMARSHALL, [arg], ClassicService.__input_type_info__['MsgMarshall'], ClassicService.__output_type_info__['MsgMarshall'])

    def async_Dispatch(__self__, tid, cmd):
        return __self__.__clnt__.async_call(ClassicService.DISPATCH, [tid, cmd], ClassicService.__input_type_info__['Dispatch'], ClassicService.__output_type_info__['Dispatch'])

    def async_Prepare(__self__, tid, sids):
        return __self__.__clnt__.async_call(ClassicService.PREPARE, [tid, sids], ClassicService.__input_type_info__['Prepare'], ClassicService.__output_type_info__['Prepare'])

    def async_Commit(__self__, tid):
        return __self__.__clnt__.async_call(ClassicService.COMMIT, [tid], ClassicService.__input_type_info__['Commit'], ClassicService.__output_type_info__['Commit'])

    def async_Abort(__self__, tid):
        return __self__.__clnt__.async_call(ClassicService.ABORT, [tid], ClassicService.__input_type_info__['Abort'], ClassicService.__output_type_info__['Abort'])

    def async_UpgradeEpoch(__self__, curr_epoch):
        return __self__.__clnt__.async_call(ClassicService.UPGRADEEPOCH, [curr_epoch], ClassicService.__input_type_info__['UpgradeEpoch'], ClassicService.__output_type_info__['UpgradeEpoch'])

    def async_TruncateEpoch(__self__, old_epoch):
        return __self__.__clnt__.async_call(ClassicService.TRUNCATEEPOCH, [old_epoch], ClassicService.__input_type_info__['TruncateEpoch'], ClassicService.__output_type_info__['TruncateEpoch'])

    def async_rpc_null(__self__):
        return __self__.__clnt__.async_call(ClassicService.RPC_NULL, [], ClassicService.__input_type_info__['rpc_null'], ClassicService.__output_type_info__['rpc_null'])

    def async_TapirAccept(__self__, cmd_id, ballot, decision):
        return __self__.__clnt__.async_call(ClassicService.TAPIRACCEPT, [cmd_id, ballot, decision], ClassicService.__input_type_info__['TapirAccept'], ClassicService.__output_type_info__['TapirAccept'])

    def async_TapirFastAccept(__self__, cmd_id, txn_cmds):
        return __self__.__clnt__.async_call(ClassicService.TAPIRFASTACCEPT, [cmd_id, txn_cmds], ClassicService.__input_type_info__['TapirFastAccept'], ClassicService.__output_type_info__['TapirFastAccept'])

    def async_TapirDecide(__self__, cmd_id, commit):
        return __self__.__clnt__.async_call(ClassicService.TAPIRDECIDE, [cmd_id, commit], ClassicService.__input_type_info__['TapirDecide'], ClassicService.__output_type_info__['TapirDecide'])

    def async_RccDispatch(__self__, cmd):
        return __self__.__clnt__.async_call(ClassicService.RCCDISPATCH, [cmd], ClassicService.__input_type_info__['RccDispatch'], ClassicService.__output_type_info__['RccDispatch'])

    def async_RccFinish(__self__, id, md_graph):
        return __self__.__clnt__.async_call(ClassicService.RCCFINISH, [id, md_graph], ClassicService.__input_type_info__['RccFinish'], ClassicService.__output_type_info__['RccFinish'])

    def async_RccInquire(__self__, epoch, txn_id):
        return __self__.__clnt__.async_call(ClassicService.RCCINQUIRE, [epoch, txn_id], ClassicService.__input_type_info__['RccInquire'], ClassicService.__output_type_info__['RccInquire'])

    def async_RccDispatchRo(__self__, cmd):
        return __self__.__clnt__.async_call(ClassicService.RCCDISPATCHRO, [cmd], ClassicService.__input_type_info__['RccDispatchRo'], ClassicService.__output_type_info__['RccDispatchRo'])

    def async_RccInquireValidation(__self__, tx_id):
        return __self__.__clnt__.async_call(ClassicService.RCCINQUIREVALIDATION, [tx_id], ClassicService.__input_type_info__['RccInquireValidation'], ClassicService.__output_type_info__['RccInquireValidation'])

    def async_RccNotifyGlobalValidation(__self__, tx_id, res):
        return __self__.__clnt__.async_call(ClassicService.RCCNOTIFYGLOBALVALIDATION, [tx_id, res], ClassicService.__input_type_info__['RccNotifyGlobalValidation'], ClassicService.__output_type_info__['RccNotifyGlobalValidation'])

    def async_JanusDispatch(__self__, cmd):
        return __self__.__clnt__.async_call(ClassicService.JANUSDISPATCH, [cmd], ClassicService.__input_type_info__['JanusDispatch'], ClassicService.__output_type_info__['JanusDispatch'])

    def async_JanusCommit(__self__, id, rank, need_validation, graph):
        return __self__.__clnt__.async_call(ClassicService.JANUSCOMMIT, [id, rank, need_validation, graph], ClassicService.__input_type_info__['JanusCommit'], ClassicService.__output_type_info__['JanusCommit'])

    def async_JanusCommitWoGraph(__self__, id, rank, need_validation):
        return __self__.__clnt__.async_call(ClassicService.JANUSCOMMITWOGRAPH, [id, rank, need_validation], ClassicService.__input_type_info__['JanusCommitWoGraph'], ClassicService.__output_type_info__['JanusCommitWoGraph'])

    def async_JanusInquire(__self__, epoch, txn_id):
        return __self__.__clnt__.async_call(ClassicService.JANUSINQUIRE, [epoch, txn_id], ClassicService.__input_type_info__['JanusInquire'], ClassicService.__output_type_info__['JanusInquire'])

    def async_JanusPreAccept(__self__, txn_id, rank, cmd, graph):
        return __self__.__clnt__.async_call(ClassicService.JANUSPREACCEPT, [txn_id, rank, cmd, graph], ClassicService.__input_type_info__['JanusPreAccept'], ClassicService.__output_type_info__['JanusPreAccept'])

    def async_JanusPreAcceptWoGraph(__self__, txn_id, rank, cmd):
        return __self__.__clnt__.async_call(ClassicService.JANUSPREACCEPTWOGRAPH, [txn_id, rank, cmd], ClassicService.__input_type_info__['JanusPreAcceptWoGraph'], ClassicService.__output_type_info__['JanusPreAcceptWoGraph'])

    def async_JanusAccept(__self__, txn_id, ballot, graph):
        return __self__.__clnt__.async_call(ClassicService.JANUSACCEPT, [txn_id, ballot, graph], ClassicService.__input_type_info__['JanusAccept'], ClassicService.__output_type_info__['JanusAccept'])

    def async_PreAcceptFebruus(__self__, tx_id):
        return __self__.__clnt__.async_call(ClassicService.PREACCEPTFEBRUUS, [tx_id], ClassicService.__input_type_info__['PreAcceptFebruus'], ClassicService.__output_type_info__['PreAcceptFebruus'])

    def async_AcceptFebruus(__self__, tx_id, ballot, timestamp):
        return __self__.__clnt__.async_call(ClassicService.ACCEPTFEBRUUS, [tx_id, ballot, timestamp], ClassicService.__input_type_info__['AcceptFebruus'], ClassicService.__output_type_info__['AcceptFebruus'])

    def async_CommitFebruus(__self__, tx_id, timestamp):
        return __self__.__clnt__.async_call(ClassicService.COMMITFEBRUUS, [tx_id, timestamp], ClassicService.__input_type_info__['CommitFebruus'], ClassicService.__output_type_info__['CommitFebruus'])

    def sync_MsgString(__self__, arg):
        __result__ = __self__.__clnt__.sync_call(ClassicService.MSGSTRING, [arg], ClassicService.__input_type_info__['MsgString'], ClassicService.__output_type_info__['MsgString'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_MsgMarshall(__self__, arg):
        __result__ = __self__.__clnt__.sync_call(ClassicService.MSGMARSHALL, [arg], ClassicService.__input_type_info__['MsgMarshall'], ClassicService.__output_type_info__['MsgMarshall'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_Dispatch(__self__, tid, cmd):
        __result__ = __self__.__clnt__.sync_call(ClassicService.DISPATCH, [tid, cmd], ClassicService.__input_type_info__['Dispatch'], ClassicService.__output_type_info__['Dispatch'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_Prepare(__self__, tid, sids):
        __result__ = __self__.__clnt__.sync_call(ClassicService.PREPARE, [tid, sids], ClassicService.__input_type_info__['Prepare'], ClassicService.__output_type_info__['Prepare'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_Commit(__self__, tid):
        __result__ = __self__.__clnt__.sync_call(ClassicService.COMMIT, [tid], ClassicService.__input_type_info__['Commit'], ClassicService.__output_type_info__['Commit'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_Abort(__self__, tid):
        __result__ = __self__.__clnt__.sync_call(ClassicService.ABORT, [tid], ClassicService.__input_type_info__['Abort'], ClassicService.__output_type_info__['Abort'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_UpgradeEpoch(__self__, curr_epoch):
        __result__ = __self__.__clnt__.sync_call(ClassicService.UPGRADEEPOCH, [curr_epoch], ClassicService.__input_type_info__['UpgradeEpoch'], ClassicService.__output_type_info__['UpgradeEpoch'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_TruncateEpoch(__self__, old_epoch):
        __result__ = __self__.__clnt__.sync_call(ClassicService.TRUNCATEEPOCH, [old_epoch], ClassicService.__input_type_info__['TruncateEpoch'], ClassicService.__output_type_info__['TruncateEpoch'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_rpc_null(__self__):
        __result__ = __self__.__clnt__.sync_call(ClassicService.RPC_NULL, [], ClassicService.__input_type_info__['rpc_null'], ClassicService.__output_type_info__['rpc_null'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_TapirAccept(__self__, cmd_id, ballot, decision):
        __result__ = __self__.__clnt__.sync_call(ClassicService.TAPIRACCEPT, [cmd_id, ballot, decision], ClassicService.__input_type_info__['TapirAccept'], ClassicService.__output_type_info__['TapirAccept'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_TapirFastAccept(__self__, cmd_id, txn_cmds):
        __result__ = __self__.__clnt__.sync_call(ClassicService.TAPIRFASTACCEPT, [cmd_id, txn_cmds], ClassicService.__input_type_info__['TapirFastAccept'], ClassicService.__output_type_info__['TapirFastAccept'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_TapirDecide(__self__, cmd_id, commit):
        __result__ = __self__.__clnt__.sync_call(ClassicService.TAPIRDECIDE, [cmd_id, commit], ClassicService.__input_type_info__['TapirDecide'], ClassicService.__output_type_info__['TapirDecide'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_RccDispatch(__self__, cmd):
        __result__ = __self__.__clnt__.sync_call(ClassicService.RCCDISPATCH, [cmd], ClassicService.__input_type_info__['RccDispatch'], ClassicService.__output_type_info__['RccDispatch'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_RccFinish(__self__, id, md_graph):
        __result__ = __self__.__clnt__.sync_call(ClassicService.RCCFINISH, [id, md_graph], ClassicService.__input_type_info__['RccFinish'], ClassicService.__output_type_info__['RccFinish'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_RccInquire(__self__, epoch, txn_id):
        __result__ = __self__.__clnt__.sync_call(ClassicService.RCCINQUIRE, [epoch, txn_id], ClassicService.__input_type_info__['RccInquire'], ClassicService.__output_type_info__['RccInquire'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_RccDispatchRo(__self__, cmd):
        __result__ = __self__.__clnt__.sync_call(ClassicService.RCCDISPATCHRO, [cmd], ClassicService.__input_type_info__['RccDispatchRo'], ClassicService.__output_type_info__['RccDispatchRo'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_RccInquireValidation(__self__, tx_id):
        __result__ = __self__.__clnt__.sync_call(ClassicService.RCCINQUIREVALIDATION, [tx_id], ClassicService.__input_type_info__['RccInquireValidation'], ClassicService.__output_type_info__['RccInquireValidation'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_RccNotifyGlobalValidation(__self__, tx_id, res):
        __result__ = __self__.__clnt__.sync_call(ClassicService.RCCNOTIFYGLOBALVALIDATION, [tx_id, res], ClassicService.__input_type_info__['RccNotifyGlobalValidation'], ClassicService.__output_type_info__['RccNotifyGlobalValidation'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_JanusDispatch(__self__, cmd):
        __result__ = __self__.__clnt__.sync_call(ClassicService.JANUSDISPATCH, [cmd], ClassicService.__input_type_info__['JanusDispatch'], ClassicService.__output_type_info__['JanusDispatch'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_JanusCommit(__self__, id, rank, need_validation, graph):
        __result__ = __self__.__clnt__.sync_call(ClassicService.JANUSCOMMIT, [id, rank, need_validation, graph], ClassicService.__input_type_info__['JanusCommit'], ClassicService.__output_type_info__['JanusCommit'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_JanusCommitWoGraph(__self__, id, rank, need_validation):
        __result__ = __self__.__clnt__.sync_call(ClassicService.JANUSCOMMITWOGRAPH, [id, rank, need_validation], ClassicService.__input_type_info__['JanusCommitWoGraph'], ClassicService.__output_type_info__['JanusCommitWoGraph'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_JanusInquire(__self__, epoch, txn_id):
        __result__ = __self__.__clnt__.sync_call(ClassicService.JANUSINQUIRE, [epoch, txn_id], ClassicService.__input_type_info__['JanusInquire'], ClassicService.__output_type_info__['JanusInquire'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_JanusPreAccept(__self__, txn_id, rank, cmd, graph):
        __result__ = __self__.__clnt__.sync_call(ClassicService.JANUSPREACCEPT, [txn_id, rank, cmd, graph], ClassicService.__input_type_info__['JanusPreAccept'], ClassicService.__output_type_info__['JanusPreAccept'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_JanusPreAcceptWoGraph(__self__, txn_id, rank, cmd):
        __result__ = __self__.__clnt__.sync_call(ClassicService.JANUSPREACCEPTWOGRAPH, [txn_id, rank, cmd], ClassicService.__input_type_info__['JanusPreAcceptWoGraph'], ClassicService.__output_type_info__['JanusPreAcceptWoGraph'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_JanusAccept(__self__, txn_id, ballot, graph):
        __result__ = __self__.__clnt__.sync_call(ClassicService.JANUSACCEPT, [txn_id, ballot, graph], ClassicService.__input_type_info__['JanusAccept'], ClassicService.__output_type_info__['JanusAccept'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_PreAcceptFebruus(__self__, tx_id):
        __result__ = __self__.__clnt__.sync_call(ClassicService.PREACCEPTFEBRUUS, [tx_id], ClassicService.__input_type_info__['PreAcceptFebruus'], ClassicService.__output_type_info__['PreAcceptFebruus'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_AcceptFebruus(__self__, tx_id, ballot, timestamp):
        __result__ = __self__.__clnt__.sync_call(ClassicService.ACCEPTFEBRUUS, [tx_id, ballot, timestamp], ClassicService.__input_type_info__['AcceptFebruus'], ClassicService.__output_type_info__['AcceptFebruus'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_CommitFebruus(__self__, tx_id, timestamp):
        __result__ = __self__.__clnt__.sync_call(ClassicService.COMMITFEBRUUS, [tx_id, timestamp], ClassicService.__input_type_info__['CommitFebruus'], ClassicService.__output_type_info__['CommitFebruus'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

class ServerControlService(object):
    SERVER_SHUTDOWN = 0x190eb91f
    SERVER_READY = 0x41ae209d
    SERVER_HEART_BEAT_WITH_DATA = 0x393504f4
    SERVER_HEART_BEAT = 0x1920fc51

    __input_type_info__ = {
        'server_shutdown': [],
        'server_ready': [],
        'server_heart_beat_with_data': [],
        'server_heart_beat': [],
    }

    __output_type_info__ = {
        'server_shutdown': [],
        'server_ready': ['rrr::i32'],
        'server_heart_beat_with_data': ['ServerResponse'],
        'server_heart_beat': [],
    }

    def __bind_helper__(self, func):
        def f(*args):
            return getattr(self, func.__name__)(*args)
        return f

    def __reg_to__(self, server):
        server.__reg_func__(ServerControlService.SERVER_SHUTDOWN, self.__bind_helper__(self.server_shutdown), [], [])
        server.__reg_func__(ServerControlService.SERVER_READY, self.__bind_helper__(self.server_ready), [], ['rrr::i32'])
        server.__reg_func__(ServerControlService.SERVER_HEART_BEAT_WITH_DATA, self.__bind_helper__(self.server_heart_beat_with_data), [], ['ServerResponse'])
        server.__reg_func__(ServerControlService.SERVER_HEART_BEAT, self.__bind_helper__(self.server_heart_beat), [], [])

    def server_shutdown(__self__):
        raise NotImplementedError('subclass ServerControlService and implement your own server_shutdown function')

    def server_ready(__self__):
        raise NotImplementedError('subclass ServerControlService and implement your own server_ready function')

    def server_heart_beat_with_data(__self__):
        raise NotImplementedError('subclass ServerControlService and implement your own server_heart_beat_with_data function')

    def server_heart_beat(__self__):
        raise NotImplementedError('subclass ServerControlService and implement your own server_heart_beat function')

class ServerControlProxy(object):
    def __init__(self, clnt):
        self.__clnt__ = clnt

    def async_server_shutdown(__self__):
        return __self__.__clnt__.async_call(ServerControlService.SERVER_SHUTDOWN, [], ServerControlService.__input_type_info__['server_shutdown'], ServerControlService.__output_type_info__['server_shutdown'])

    def async_server_ready(__self__):
        return __self__.__clnt__.async_call(ServerControlService.SERVER_READY, [], ServerControlService.__input_type_info__['server_ready'], ServerControlService.__output_type_info__['server_ready'])

    def async_server_heart_beat_with_data(__self__):
        return __self__.__clnt__.async_call(ServerControlService.SERVER_HEART_BEAT_WITH_DATA, [], ServerControlService.__input_type_info__['server_heart_beat_with_data'], ServerControlService.__output_type_info__['server_heart_beat_with_data'])

    def async_server_heart_beat(__self__):
        return __self__.__clnt__.async_call(ServerControlService.SERVER_HEART_BEAT, [], ServerControlService.__input_type_info__['server_heart_beat'], ServerControlService.__output_type_info__['server_heart_beat'])

    def sync_server_shutdown(__self__):
        __result__ = __self__.__clnt__.sync_call(ServerControlService.SERVER_SHUTDOWN, [], ServerControlService.__input_type_info__['server_shutdown'], ServerControlService.__output_type_info__['server_shutdown'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_server_ready(__self__):
        __result__ = __self__.__clnt__.sync_call(ServerControlService.SERVER_READY, [], ServerControlService.__input_type_info__['server_ready'], ServerControlService.__output_type_info__['server_ready'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_server_heart_beat_with_data(__self__):
        __result__ = __self__.__clnt__.sync_call(ServerControlService.SERVER_HEART_BEAT_WITH_DATA, [], ServerControlService.__input_type_info__['server_heart_beat_with_data'], ServerControlService.__output_type_info__['server_heart_beat_with_data'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_server_heart_beat(__self__):
        __result__ = __self__.__clnt__.sync_call(ServerControlService.SERVER_HEART_BEAT, [], ServerControlService.__input_type_info__['server_heart_beat'], ServerControlService.__output_type_info__['server_heart_beat'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

class ClientControlService(object):
    CLIENT_GET_TXN_NAMES = 0x20e359d6
    CLIENT_SHUTDOWN = 0x2d619bad
    CLIENT_FORCE_STOP = 0x596b5b38
    CLIENT_RESPONSE = 0x3e9e633e
    CLIENT_READY = 0x41451c54
    CLIENT_READY_BLOCK = 0x2a473f8a
    CLIENT_START = 0x2a2a7e21
    DISPATCHTXN = 0x2e11b470

    __input_type_info__ = {
        'client_get_txn_names': [],
        'client_shutdown': [],
        'client_force_stop': [],
        'client_response': [],
        'client_ready': [],
        'client_ready_block': [],
        'client_start': [],
        'DispatchTxn': ['TxDispatchRequest'],
    }

    __output_type_info__ = {
        'client_get_txn_names': ['std::map<rrr::i32, std::string>'],
        'client_shutdown': [],
        'client_force_stop': [],
        'client_response': ['ClientResponse'],
        'client_ready': ['rrr::i32'],
        'client_ready_block': ['rrr::i32'],
        'client_start': [],
        'DispatchTxn': ['TxReply'],
    }

    def __bind_helper__(self, func):
        def f(*args):
            return getattr(self, func.__name__)(*args)
        return f

    def __reg_to__(self, server):
        server.__reg_func__(ClientControlService.CLIENT_GET_TXN_NAMES, self.__bind_helper__(self.client_get_txn_names), [], ['std::map<rrr::i32, std::string>'])
        server.__reg_func__(ClientControlService.CLIENT_SHUTDOWN, self.__bind_helper__(self.client_shutdown), [], [])
        server.__reg_func__(ClientControlService.CLIENT_FORCE_STOP, self.__bind_helper__(self.client_force_stop), [], [])
        server.__reg_func__(ClientControlService.CLIENT_RESPONSE, self.__bind_helper__(self.client_response), [], ['ClientResponse'])
        server.__reg_func__(ClientControlService.CLIENT_READY, self.__bind_helper__(self.client_ready), [], ['rrr::i32'])
        server.__reg_func__(ClientControlService.CLIENT_READY_BLOCK, self.__bind_helper__(self.client_ready_block), [], ['rrr::i32'])
        server.__reg_func__(ClientControlService.CLIENT_START, self.__bind_helper__(self.client_start), [], [])
        server.__reg_func__(ClientControlService.DISPATCHTXN, self.__bind_helper__(self.DispatchTxn), ['TxDispatchRequest'], ['TxReply'])

    def client_get_txn_names(__self__):
        raise NotImplementedError('subclass ClientControlService and implement your own client_get_txn_names function')

    def client_shutdown(__self__):
        raise NotImplementedError('subclass ClientControlService and implement your own client_shutdown function')

    def client_force_stop(__self__):
        raise NotImplementedError('subclass ClientControlService and implement your own client_force_stop function')

    def client_response(__self__):
        raise NotImplementedError('subclass ClientControlService and implement your own client_response function')

    def client_ready(__self__):
        raise NotImplementedError('subclass ClientControlService and implement your own client_ready function')

    def client_ready_block(__self__):
        raise NotImplementedError('subclass ClientControlService and implement your own client_ready_block function')

    def client_start(__self__):
        raise NotImplementedError('subclass ClientControlService and implement your own client_start function')

    def DispatchTxn(__self__, req):
        raise NotImplementedError('subclass ClientControlService and implement your own DispatchTxn function')

class ClientControlProxy(object):
    def __init__(self, clnt):
        self.__clnt__ = clnt

    def async_client_get_txn_names(__self__):
        return __self__.__clnt__.async_call(ClientControlService.CLIENT_GET_TXN_NAMES, [], ClientControlService.__input_type_info__['client_get_txn_names'], ClientControlService.__output_type_info__['client_get_txn_names'])

    def async_client_shutdown(__self__):
        return __self__.__clnt__.async_call(ClientControlService.CLIENT_SHUTDOWN, [], ClientControlService.__input_type_info__['client_shutdown'], ClientControlService.__output_type_info__['client_shutdown'])

    def async_client_force_stop(__self__):
        return __self__.__clnt__.async_call(ClientControlService.CLIENT_FORCE_STOP, [], ClientControlService.__input_type_info__['client_force_stop'], ClientControlService.__output_type_info__['client_force_stop'])

    def async_client_response(__self__):
        return __self__.__clnt__.async_call(ClientControlService.CLIENT_RESPONSE, [], ClientControlService.__input_type_info__['client_response'], ClientControlService.__output_type_info__['client_response'])

    def async_client_ready(__self__):
        return __self__.__clnt__.async_call(ClientControlService.CLIENT_READY, [], ClientControlService.__input_type_info__['client_ready'], ClientControlService.__output_type_info__['client_ready'])

    def async_client_ready_block(__self__):
        return __self__.__clnt__.async_call(ClientControlService.CLIENT_READY_BLOCK, [], ClientControlService.__input_type_info__['client_ready_block'], ClientControlService.__output_type_info__['client_ready_block'])

    def async_client_start(__self__):
        return __self__.__clnt__.async_call(ClientControlService.CLIENT_START, [], ClientControlService.__input_type_info__['client_start'], ClientControlService.__output_type_info__['client_start'])

    def async_DispatchTxn(__self__, req):
        return __self__.__clnt__.async_call(ClientControlService.DISPATCHTXN, [req], ClientControlService.__input_type_info__['DispatchTxn'], ClientControlService.__output_type_info__['DispatchTxn'])

    def sync_client_get_txn_names(__self__):
        __result__ = __self__.__clnt__.sync_call(ClientControlService.CLIENT_GET_TXN_NAMES, [], ClientControlService.__input_type_info__['client_get_txn_names'], ClientControlService.__output_type_info__['client_get_txn_names'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_client_shutdown(__self__):
        __result__ = __self__.__clnt__.sync_call(ClientControlService.CLIENT_SHUTDOWN, [], ClientControlService.__input_type_info__['client_shutdown'], ClientControlService.__output_type_info__['client_shutdown'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_client_force_stop(__self__):
        __result__ = __self__.__clnt__.sync_call(ClientControlService.CLIENT_FORCE_STOP, [], ClientControlService.__input_type_info__['client_force_stop'], ClientControlService.__output_type_info__['client_force_stop'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_client_response(__self__):
        __result__ = __self__.__clnt__.sync_call(ClientControlService.CLIENT_RESPONSE, [], ClientControlService.__input_type_info__['client_response'], ClientControlService.__output_type_info__['client_response'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_client_ready(__self__):
        __result__ = __self__.__clnt__.sync_call(ClientControlService.CLIENT_READY, [], ClientControlService.__input_type_info__['client_ready'], ClientControlService.__output_type_info__['client_ready'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_client_ready_block(__self__):
        __result__ = __self__.__clnt__.sync_call(ClientControlService.CLIENT_READY_BLOCK, [], ClientControlService.__input_type_info__['client_ready_block'], ClientControlService.__output_type_info__['client_ready_block'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_client_start(__self__):
        __result__ = __self__.__clnt__.sync_call(ClientControlService.CLIENT_START, [], ClientControlService.__input_type_info__['client_start'], ClientControlService.__output_type_info__['client_start'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

    def sync_DispatchTxn(__self__, req):
        __result__ = __self__.__clnt__.sync_call(ClientControlService.DISPATCHTXN, [req], ClientControlService.__input_type_info__['DispatchTxn'], ClientControlService.__output_type_info__['DispatchTxn'])
        if __result__[0] != 0:
            raise Exception("RPC returned non-zero error code %d: %s" % (__result__[0], os.strerror(__result__[0])))
        if len(__result__[1]) == 1:
            return __result__[1][0]
        elif len(__result__[1]) > 1:
            return __result__[1]

