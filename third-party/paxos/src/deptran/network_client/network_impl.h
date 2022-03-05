//#pragma once
// similar to service.h

#include "__dep__.h"
#include "constants.h"
#include "../rcc/graph.h"
#include "../rcc/graph_marshaler.h"
#include "../command.h"
#include "deptran/procedure.h"
#include "../command_marshaler.h"
#include "../network.h"

namespace network_client {
    class NetworkClientServiceImpl : public NetworkClientService {

    public: 
        NetworkClientServiceImpl() ;
        
        // YCSB++ benchmark
        void txn_rmw(const std::vector<int32_t>& _req, rrr::DeferredReply* defer) override;
        void txn_read(const std::vector<int32_t>& _req, rrr::DeferredReply* defer) override;
        
        // TPC-C bencmark
        void txn_new_order(const std::vector<int32_t>& _req, rrr::DeferredReply* defer) override;
        void txn_payment(const std::vector<int32_t>& _req, rrr::DeferredReply* defer) override;
        void txn_delivery(const std::vector<int32_t>& _req, rrr::DeferredReply* defer) override;
        void txn_order_status(const std::vector<int32_t>& _req, rrr::DeferredReply* defer) override;
        void txn_stock_level(const std::vector<int32_t>& _req, rrr::DeferredReply* defer) override;

    public:
        int counter = 0;

        int counter_new_order=0;
        int counter_payement=0;
        int counter_delivery=0;
        int counter_order_status=0;
        int counter_stock_level=0;

        std::vector<std::vector<int>> new_order_requests;
        std::vector<std::vector<int>> payment_requests;
        std::vector<std::vector<int>> delivery_requests;
        std::vector<std::vector<int>> order_status_requests;
        std::vector<std::vector<int>> stock_level_requests;

        int counter_rmw=0;
        int counter_read=0;
        std::vector<std::vector<int>> rmw_requests;
        std::vector<std::vector<int>> read_requests;
    } ;
} // namespace network_client