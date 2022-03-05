#include "__dep__.h"
#include "deptran/s_main.h"
#include <iostream>
#include <vector>
#include <sys/time.h>
#include <thread>
#include <string>
#include <cstring>
#include <unistd.h>
#include "concurrentqueue.h"
#include "network_client/network_impl.h"
#include "nc_util.h"
#include "deptran/communicator.h"
#include <pthread.h>

using namespace janus;
using namespace network_client;

std::vector<shared_ptr<network_client::NetworkClientProxy>> nc_clients = {} ;
fast_random r(0);
int nthreads=1;
int batch_size=1000;
char *server_ip="127.0.0.1";

int nkeys=1000000;
int batch_size_ycsb=10000;

int NumWarehouses() { return nthreads; }

std::vector<int> nc_generate_rmw(int par_id) {
  std::vector<int> ret;
  for (int i=0; i<batch_size_ycsb * 4; i++) {
    int k = r.next() % nkeys;
    ret.push_back(k);
  }
  return ret;
}

std::vector<int> nc_generate_read(int par_id) {
  std::vector<int> ret;
  for (int i=0; i<batch_size_ycsb * 4; i++) {
    int k = r.next() % nkeys;
    ret.push_back(k);
  }
  return ret;
}


std::vector<int> nc_generate_new_order(int par_id) {
  /*
    warehouse_id
    districtID
    customerID
    numItems
    for 1 .. numItems:
      itemID
      supplierWarehouseID
      orderQuantity
  */
  std::vector<int> ret;
  for (int i=0; i<batch_size; i++) {
    uint warehouse_id=par_id+1; // 1 warehouse per thread
    ret.push_back(warehouse_id);

    uint districtID=RandomNumber(r, 1, NumDistrictsPerWarehouse()); // [1,10]
    ret.push_back(districtID);

    uint customerID=GetCustomerId(r); // [1,3000]
    ret.push_back(customerID);

    uint numItems=RandomNumber(r, 5, 15);  // [5,15]
    ret.push_back(numItems);
    
    // uint itemIDs[numItems], supplierWarehouseIDs[numItems], orderQuantities[numItems]
    for (uint i=0; i<numItems; i++) {
        ret.push_back(GetItemId(r));  // g_uniform_item_dist == 0

        // g_disable_xpartition_txn == 0, g_new_order_remote_item_pct == 1
        if (likely(NumWarehouses() == 1 || RandomNumber(r, 1, 100) > 1)) {
            ret.push_back(warehouse_id) ;
        } else {
            int remote_id=warehouse_id;
            do {
                remote_id = RandomNumber(r, 1, NumWarehouses());
            } while (remote_id == warehouse_id);
            ret.push_back(remote_id);
        }
        ret.push_back(RandomNumber(r, 1, 10)) ;
    }
  }
  return ret;
}

std::vector<int> nc_generate_payment(int par_id) {
    /*
        warehouse_id
        districtID
        customerDistrictID
        customerWarehouseID
        paymentAmount
    */
    std::vector<int> ret;
    for (int i=0; i<batch_size; i++) {
      uint warehouse_id=par_id+1; // 1 warehouse per thread
      ret.push_back(warehouse_id);

      uint districtID=RandomNumber(r, 1, NumDistrictsPerWarehouse());
      ret.push_back(districtID);

      if (likely(NumWarehouses() == 1 || RandomNumber(r, 1, 100) <= 85)) {
          ret.push_back(districtID);
          ret.push_back(warehouse_id);
      } else {
          ret.push_back(RandomNumber(r, 1, NumDistrictsPerWarehouse()));
          int remote_id=warehouse_id;
          do {
            remote_id = RandomNumber(r, 1, NumWarehouses());
          } while (remote_id == warehouse_id);
          ret.push_back(remote_id);
      }
      
      ret.push_back(RandomNumber(r, 100, 500000)); // paymentAmount * 100
    }
    
    return ret;
}

std::vector<int> nc_generate_delivery(int par_id) {
    /*
      warehouse_id
      carrier_id
    */
    std::vector<int> ret;
    for (int i=0; i<batch_size; i++) {
      uint warehouse_id=par_id+1; // 1 warehouse per thread
      ret.push_back(warehouse_id);

      ret.push_back(RandomNumber(r, 1, NumDistrictsPerWarehouse()));
    }
    return ret;
}

std::vector<int> nc_generate_order_status(int par_id) {
    /*
      warehouse_id
      districtID
      threshold
      customerID
    */
    std::vector<int> ret;
    for (int i=0; i<batch_size; i++) {
      uint warehouse_id=par_id+1; // 1 warehouse per thread
      ret.push_back(warehouse_id);

      ret.push_back(RandomNumber(r, 1, NumDistrictsPerWarehouse())) ;

      int threshold = RandomNumber(r, 1, 100) ;
      ret.push_back(threshold) ;
      if (threshold <= 60) {
          ret.push_back(0);
      } else {
          ret.push_back(GetCustomerId(r)) ;
      }
    }
    
    return ret;
}

std::vector<int> nc_generate_stock_level(int par_id) {
    /*
      warehouse_id
      threshold
      districtID
    */
    std::vector<int> ret;
    for (int i=0; i<batch_size; i++) {
      uint warehouse_id=par_id+1; // 1 warehouse per thread
      ret.push_back(warehouse_id);

      ret.push_back(RandomNumber(r, 10, 20));

      ret.push_back(RandomNumber(r, 1, NumDistrictsPerWarehouse())) ;
    }
    return ret;
}


struct args {
    int par_id;
};

void *nc_start_client(void *input) { // benchmark implementation in the client
  int par_id = ((struct args*)input)->par_id;
  std::atomic<int64_t> done(0);
  int64_t t_counter=0;
  // mix of the workload: [45, 43, 4, 4, 4]
  while (1) {
    t_counter ++;
    while (t_counter - done > 1) { usleep(1000 * 10); }
    //usleep(10 * 1000);
    int r = rand() % 100 + 1; // [1, 100]
    int ret=0;
    if (r<=45) {  // communicator.cc
        FutureAttr fuattr;  // fuattr
        fuattr.callback = [&done] (Future* fu) {
          done.fetch_add(1);
        };
        vector<int> _req = nc_generate_new_order(par_id);
        Future::safe_release(nc_clients[par_id]->async_txn_new_order(_req, fuattr));
    } else if (r <= 88) {
        FutureAttr fuattr;  // fuattr
        fuattr.callback = [&done] (Future* fu) {
          done.fetch_add(1);
        };
        vector<int> _req = nc_generate_payment(par_id);
        Future::safe_release(nc_clients[par_id]->async_txn_payment(_req, fuattr));
    } else if (r <= 92) {
        FutureAttr fuattr;  // fuattr
        fuattr.callback = [&done] (Future* fu) {
          done.fetch_add(1);
        };
        vector<int> _req = nc_generate_delivery(par_id);
        Future::safe_release(nc_clients[par_id]->async_txn_delivery(_req, fuattr));
    } else if (r <= 96) {
        FutureAttr fuattr;  // fuattr
        fuattr.callback = [&done] (Future* fu) {
          done.fetch_add(1);
        };
        vector<int> _req = nc_generate_order_status(par_id);
        Future::safe_release(nc_clients[par_id]->async_txn_order_status(_req, fuattr));
    } else {
        FutureAttr fuattr;  // fuattr
        fuattr.callback = [&done] (Future* fu) {
          done.fetch_add(1);
        };
        vector<int> _req = nc_generate_stock_level(par_id);
        Future::safe_release(nc_clients[par_id]->async_txn_stock_level(_req, fuattr));
    }

    if (t_counter % 100==0) std::cout << "issue # of transactions[par-id:" << par_id << "]: " << t_counter << std::endl;
  }
}

void *nc_start_client_ycsb(void *input) { // benchmark implementation in the client
  int par_id = ((struct args*)input)->par_id;
  std::atomic<int64_t> done(0);
  int64_t t_counter=0;
  // mix of the workload: [50, 50]
  while (1) {
    t_counter ++;
    while (t_counter - done > 1) { usleep(1000 * 10); }
    int r = rand() % 100 + 1; // [1, 100]
    int ret=0;
    if (r<=50) {  // communicator.cc
        FutureAttr fuattr;  // fuattr
        fuattr.callback = [&done] (Future* fu) {
          done.fetch_add(1);
        };
        vector<int> _req = nc_generate_read(par_id);
        Future::safe_release(nc_clients[par_id]->async_txn_read(_req, fuattr));
    } else {
        FutureAttr fuattr;  // fuattr
        fuattr.callback = [&done] (Future* fu) {
          done.fetch_add(1);
        };
        vector<int> _req = nc_generate_rmw(par_id);
        Future::safe_release(nc_clients[par_id]->async_txn_rmw(_req, fuattr));
    }
    if (t_counter % 100==0) std::cout << "issue # of transactions[par-id:" << par_id << "]: " << t_counter << std::endl;
  }
}

void nc_setup_bench(int nkeys, int nthreads, int run) {  // nkeys for YCSB++
  for (int i=0; i<nthreads; i++) {
    rrr::PollMgr *pm = new rrr::PollMgr();
    rrr::Client *client = new rrr::Client(pm);
    auto port_s=std::to_string(10010+i);
    while (client->connect((std::string(server_ip)+":"+port_s).c_str())!=0) {
      usleep(100 * 1000); // retry to connect
    }
    NetworkClientProxy *nc_client_proxy = new NetworkClientProxy(client);
    nc_clients.push_back(std::shared_ptr<NetworkClientProxy>(nc_client_proxy));
  }

  // using different threads to issue transactions independently
  for (int par_id=0; par_id<nthreads; par_id++) {
    pthread_t ph_c;
    struct args *ps = (struct args *)malloc(sizeof(struct args));
    ps->par_id=par_id;
    pthread_create(&ph_c, NULL, nc_start_client_ycsb, (void *)ps);
    pthread_detach(ph_c);
    usleep(10 * 1000);
  }

  sleep(run);  // sleep 60 seconds
}

int main(int argc, char* argv[]){
    if (argc < 3) return -1;

    unsigned int is_server = atoi(argv[1]) ;
    nthreads = atoi(argv[2]);
    if (argc > 3)
      server_ip = argv[3];

    std::cout << "Using nthreads: " << nthreads << ", on server_ip: " << server_ip << std::endl;
    int runningTime=60;
    if (is_server) {  // XXX, this branch should attach to Rolis, we put it here just for testing
        nc_setup_server(nthreads, "127.0.0.1");
        sleep(runningTime);
        // while (1) { 
        //     sleep(1); 
        //     for (int i=0; i<nthreads; i++) {
        //         std::vector<std::vector<int>> *requests = nc_get_new_order_requests(i);
        //         std::cout << "# of new order requests[par_id-" << i << "]: "<< requests->size() << std::endl;
        //     }
        // }
    } else {
        sleep(1) ; // wait for server start
        std::cout << "start the benchmark\n";
        nc_setup_bench(100*10000, nthreads, runningTime);
    }
    return 0;
}
