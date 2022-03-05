#include <iostream>
#include <string>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <chrono>
#include <stdlib.h> // https://stackoverflow.com/questions/1646031/strtoull-and-long-long-arithmetic

using namespace std;

int OFFSET_CID = 0; // sizeof = 8
int OFFSET_K = 8 ; // sizeof = 8
int OFFSET_V = 16 ; // sizeof = 8
int OFFSET_TABLE_ID = 24; // sizeof = 2
int OFFSET_DELETE_TRUE = 26 ; // sizeof = 1

class one_container{
    typedef unsigned long long int ULL_I;
    size_t ULL_LEN = sizeof (ULL_I);
    size_t UC_LEN = sizeof (unsigned char);
    size_t USI_LEN = sizeof (unsigned short int);

public:
//  static size_t dread;
    ULL_I k =0;
    ULL_I v = 0;
    unsigned short int table_id=0;
    ULL_I insert_true=0;
    unsigned char delete_true=0;
    ULL_I cid=0;
    std::string str_value="";
    std::string str_key = "";
    bool exists=false;

    std::string &getValue(){
        return str_value;
    }

    one_container(char *buffer,size_t& rw){
        k =0;
        v = 0;
        table_id=0;
        insert_true=0;
        delete_true=0;
        cid=0;
        memcpy ((char *) &cid, buffer + rw, ULL_LEN);
        rw += ULL_LEN;

        memcpy ((char *) &k, buffer + rw, ULL_LEN);
        rw += ULL_LEN;

        memcpy ((char *) &v, buffer + rw, ULL_LEN);
        rw += ULL_LEN;

        memcpy ((char *) &table_id, buffer + rw, USI_LEN);
        rw += USI_LEN;

        memcpy ((char *) &delete_true, buffer + rw, UC_LEN);
        rw += UC_LEN;
    }
};

class one_container_pointer{
    typedef unsigned long long int ULL_I;
    size_t ULL_LEN = sizeof (ULL_I);
    size_t UC_LEN = sizeof (unsigned char);
    size_t USI_LEN = sizeof (unsigned short int);

public:
    char *cid ;
    char *k ;
    char *v ;
    char *table_id ;
    char *delete_true ;
    std::string str_value="";
    std::string str_key = "";

    std::string &getValue(){
        return str_value;
    }

    one_container_pointer(char *buffer, size_t& rw){
        cid = buffer + rw ;  // ULL_LEN
        //cout << static_cast<void*>(cid) << endl;
        rw += ULL_LEN;

        k = buffer + rw ;  // ULL_LEN
        rw += ULL_LEN;

        v = buffer + rw ; // ULL_LEN
        rw += ULL_LEN;

        table_id = buffer + rw ; // USI_LEN
        rw += USI_LEN;

        delete_true = buffer + rw ; // UC_LEN
        rw += UC_LEN;
    }
};


long get_file_size(std::string filename)
{
    struct stat *stat_buf=new struct stat();
    int rc = stat(filename.c_str(), stat_buf);
    auto ans = rc == 0 ? stat_buf->st_size : -1;
    delete stat_buf;
    return ans;
}

void printEntry(one_container *c) {
    cout << "cid:" << c->cid << ", key:" << c->k << ", value:" << c->v << ", table_id: " << c->table_id << ", delete_true:" << c->delete_true<< endl;
}

void * memcpyV(void * dst, void const * src, size_t len)
{
    // https://stackoverflow.com/questions/9627962/is-it-possible-to-convert-char-to-char-in-c/9628309
    char * pDst = (char *) dst;
    char const * pSrc = (char const *) src;

    while (len--)
    {
        *pDst++ = *pSrc++;
    }

    return (dst);
}

int main()
{
    std::string file_name = "/home/weihshen/Desktop/GenLogThd20.Time.15/Log-ThreadID:0.txt" ;
    size_t sz = get_file_size(file_name);
    int fd = open (file_name.c_str (), O_RDONLY);
    size_t ret = 0;
    void *ptr = NULL;
    char *buffer;

    ptr = (void *) malloc (sz);
    buffer = (char *) ptr;
    ret = read (fd, buffer, sz);
    if (ret == -1 || ret == 0) {
        std::cout << "[mymain] file is empty " << ret << std::endl;
    }

    auto start = std::chrono::steady_clock::now();
    for (size_t offset=0; offset < 10; ++offset) {
        size_t rw = 0;
        auto *entry = new one_container((char *) (buffer + offset * 27), rw);
        printEntry(entry) ;
    }
    auto end = std::chrono::steady_clock::now();
    auto diff = end - start;
    std::cout << "Time Taken : " << std::chrono::duration_cast<std::chrono::milliseconds>(diff).count()<< " millis\n";

    unsigned long long int *cid = 0;
    unsigned long long int *k = 0;
    unsigned long long int *v = 0;
    unsigned short int *table_id = 0;
    unsigned char *delete_true = 0;
    for (size_t offset=0; offset < 10; ++offset) {
        char *base = buffer + offset * 27 ;

        cid = reinterpret_cast<unsigned long long int*>(base + OFFSET_CID);
        k = reinterpret_cast<unsigned long long int*>(base + OFFSET_K);
        v = reinterpret_cast<unsigned long long int*>(base + OFFSET_V);
        table_id = reinterpret_cast<unsigned short int*>(base + OFFSET_TABLE_ID);
        delete_true = reinterpret_cast<unsigned char*>(base + OFFSET_DELETE_TRUE);

        cout << "cid:" << *cid << ", key:" << *k << ", value:" << *v << ", table_id: " << *table_id << ", delete_true:" << *delete_true << endl;

//        unsigned long long int cid = 0;
//        unsigned short int table_id = 0;
//        memcpyV(&cid, base + OFFSET_CID, sizeof(unsigned long long int)) ;
//        memcpyV(&table_id, base + OFFSET_TABLE_ID, sizeof(unsigned short int)) ;
//
//        unsigned long long int *ccid = 0;
//        ccid = reinterpret_cast<unsigned long long int*>(base + OFFSET_CID);

//        size_t rw = 0;
//       // ccid = (unsigned long long int) (base + OFFSET_CID) ;
//        // it is not the same thing
//        //unsigned long long int cccid = *reinterpret_cast<unsigned long long int*>( base + OFFSET_CID );
//        unsigned long long int cccid = reinterpret_cast<unsigned long long int>( base + OFFSET_CID );
//        unsigned long long int ccccid = (unsigned long long int)strtoull(base + OFFSET_CID, (char**)NULL, 10) ;
//        cout << "cid:" << cid << ", ccid: " << *ccid << ", cccid: " << cccid << ", ccccid:" << ccccid << endl;
//        cout << "tid:" << table_id << endl;

//        auto *entry = new one_container_pointer((char *) (buffer + offset * 27), rw);
//
//        unsigned long long int cid = 0;
//        unsigned short int table_id = 0;
//
//        memcpyV ((char *) &cid, entry->cid, 8);
//        memcpyV ((char *) &table_id, entry->table_id, 2);
//        cout << "cid: " << cid  << ", table_id: " << table_id << endl;

        // https://stackoverflow.com/questions/33320876/is-there-any-alternative-to-strtoull-function-in-c
        // http://www.cplusplus.com/reference/cstdlib/strtoull/
        // https://stackoverflow.com/questions/27260304/equivalent-of-atoi-for-unsigned-integers
        // http://www.cplusplus.com/reference/climits/
//        unsigned long long l = 0;
//        for (int i = 0; i < 8; ++i) {
//            l = l | ((unsigned long long)entry->cid[i] << (8 * i));
//        }
//        cout << "l: " << l << endl;
//        auto ccid = (unsigned long long int) entry->cid ;
//        unsigned long long int cccid = reinterpret_cast<unsigned long long int>( entry->cid );
//        unsigned long long int ccccid = (unsigned long long int)strtoull(entry->cid, (char**)NULL, 10) ;
//        cout << "ccid: " << ccid << ":" << cccid << ":" << ccccid << endl;

//        auto aid = (unsigned short int)entry->table_id;
//        cout "table_id: " << aid;

    }

}