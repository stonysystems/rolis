#!/bin/bash

source ~/.bash_test_tools
cd ..
SCRIPT=`realpath $0`
SCRIPTPATH=`dirname $SCRIPT`

base="$SCRIPTPATH"

function setup
{
  cd "$base"
}

function teardown
{
  echo ""
}

# test 1 leader + 1 follower with reply enabled
function test_2replicas_normal
{
  sudo pkill -f dbtest
  ./multi_local.sh 2 0 0  # {thread_number} {skip compile} {if use tail}
  sleep 20
  run "grep process. $base/xxxx12/follower-p1-2.log"
  assert_success

  run "grep agg_throughput: $base/xxxx12/leader-2.log"
  assert_success
}

# test silo-only
function test_silo_only
{
  ./multi-silo-only.sh 3 3 1
  sleep 20
  run "grep agg_throughput: $base/silo-only-logs/leader-3-1000.log"
  assert_success
}

# test failure
function test_failure
{
  sudo pkill -f dbtest
  ./multi-failover.sh
  ./multi_recover_b0.sh 2
  sleep 1
  ./multi_recover_b1.sh 2
  sleep 1
  ./multi_recover_b2.sh 2
  sleep 10
  kill $(ps aux | grep 'paxos-leader-config' | awk '{print $2}')
  sleep 50

  sudo pkill -f dbtest
  run "grep commits: $base/xxxx_recover/leader-2.log"
  assert_success

  cat "$base/xxxx_recover/follower-p1-2.log" "$base/xxxx_recover/follower-p2-2.log" > "$base/xxxx_recover/follower-2.log"
  run "grep agg_throughput: $base/xxxx_recover/follower-2.log"
  assert_success

  run "grep process. $base/xxxx_recover/follower-2.log"
  assert_success
}

testrunner
