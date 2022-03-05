# we want to find in /usr/local/include
# https://stackoverflow.com/questions/344317/where-does-gcc-look-for-c-and-c-header-files
# echo '#include "rpc/server.h"' > t.c; g++ -v t.c; rm t.c
g++ echo_follower.cc -g -lrpc -lpthread -Wall -O0 -o echo_follower
g++ echo_old_leader.cc -g -lrpc -lpthread -Wall -O0 -o echo_old_leader