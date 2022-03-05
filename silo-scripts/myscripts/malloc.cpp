#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include <string.h>
#include <chrono>

using namespace std;

#define MAX_ARRAY_SIZE_IN_BYTES  size_t(4098)*size_t(1024)*size_t(4098)*sizeof(char)

int main(int argc, char **argv) {
    std::cout << MAX_ARRAY_SIZE_IN_BYTES << std::endl;
    unsigned char * LOG = (unsigned char *) malloc (MAX_ARRAY_SIZE_IN_BYTES);
    LOG[0] = '0';
    LOG[MAX_ARRAY_SIZE_IN_BYTES - 1] = '0';
    std::cout << LOG[MAX_ARRAY_SIZE_IN_BYTES - 1] << std::endl;

    unsigned char * LOG0 = (unsigned char *) malloc (MAX_ARRAY_SIZE_IN_BYTES);
    LOG0[0] = '0';
    LOG0[MAX_ARRAY_SIZE_IN_BYTES - 1] = '0';
    std::cout << LOG0[MAX_ARRAY_SIZE_IN_BYTES - 1] << std::endl;

    unsigned char * LOG1 = (unsigned char *) malloc (MAX_ARRAY_SIZE_IN_BYTES);
    LOG1[0] = '0';
    LOG1[MAX_ARRAY_SIZE_IN_BYTES - 1] = '0';
    std::cout << LOG1[MAX_ARRAY_SIZE_IN_BYTES - 1] << std::endl;

    unsigned char * LOG2 = (unsigned char *) malloc (MAX_ARRAY_SIZE_IN_BYTES);
    LOG2[0] = '0';
    LOG2[MAX_ARRAY_SIZE_IN_BYTES - 1] = '0';
    std::cout << LOG2[MAX_ARRAY_SIZE_IN_BYTES - 1] << std::endl;

    unsigned char * LOG3 = (unsigned char *) malloc (MAX_ARRAY_SIZE_IN_BYTES);
    LOG3[0] = '0';
    LOG3[MAX_ARRAY_SIZE_IN_BYTES - 1] = '0';
    std::cout << LOG3[MAX_ARRAY_SIZE_IN_BYTES - 1] << std::endl;

    unsigned char * LOG4 = (unsigned char *) malloc (MAX_ARRAY_SIZE_IN_BYTES);
    LOG4[0] = '0';
    LOG4[MAX_ARRAY_SIZE_IN_BYTES - 1] = '0';
    std::cout << LOG4[MAX_ARRAY_SIZE_IN_BYTES - 1] << std::endl;

    unsigned char * LOG5 = (unsigned char *) malloc (MAX_ARRAY_SIZE_IN_BYTES);
    LOG5[0] = '0';
    LOG5[MAX_ARRAY_SIZE_IN_BYTES - 1] = '0';
    std::cout << LOG5[MAX_ARRAY_SIZE_IN_BYTES - 1] << std::endl;

    unsigned char * LOG6 = (unsigned char *) malloc (MAX_ARRAY_SIZE_IN_BYTES);
    LOG6[0] = '0';
    LOG6[MAX_ARRAY_SIZE_IN_BYTES - 1] = '0';
    std::cout << LOG6[MAX_ARRAY_SIZE_IN_BYTES - 1] << std::endl;

    std::this_thread::sleep_for (std::chrono::seconds (1000));
    return 0 ;
}