sudo apt-get update
sudo apt update
sudo apt --assume-yes install make automake cmake
sudo apt --assume-yes install gcc
sudo apt --assume-yes install g++
sudo apt --assume-yes install boost
sudo apt-get --assume-yes install libboost-all-dev
sudo apt --assume-yes install libyaml-cpp-dev
sudo apt-get --assume-yes install -y libjemalloc-dev
sudo apt-get --assume-yes install libgoogle-perftools-dev
sudo apt --assume-yes install -y libaio-dev
sudo apt-get --assume-yes install build-essential libssl-dev libffi-dev python-dev
sudo apt --assume-yes install silversearcher-ag
sudo apt --assume-yes install numactl
sudo apt --assume-yes install autoconf
sudo apt-get --assume-yes install -y cgroup-tools
sudo apt --assume-yes install net-tools
sudo apt-get --assume-yes install -y pkg-config
sudo apt-get --assume-yes install -y strace
#sudo apt-get install google-perftools
#sudo apt-get install libaio1
#sudo apt install zlib1g-dev

#cd ~
#wget https://boostorg.jfrog.io/artifactory/main/release/1.70.0/source/boost_1_70_0.tar.bz2
#tar -xvf boost_1_70_0.tar.bz2
#cd boost_1_70_0/
#./bootstrap.sh  --prefix=/usr/local --libdir=/usr/lib/x86_64-linux-gnu
#sudo ./b2 install -j8

cd ~
git clone https://github.com/rpclib/rpclib.git
cd rpclib
cmake .
make -j4
sudo make install
cd ~
rm -rf rpclib

cd ~
wget https://raw.githubusercontent.com/thorsteinssonh/bash_test_tools/master/bash_test_tools
sudo cp bash_test_tools ~/.bash_test_tools
