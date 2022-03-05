cd ~
wget https://boostorg.jfrog.io/artifactory/main/release/1.70.0/source/boost_1_70_0.tar.bz2
tar -xvf boost_1_70_0.tar.bz2
cd boost_1_70_0/
./bootstrap.sh  --prefix=/usr/local --libdir=/usr/lib/x86_64-linux-gnu
sudo ./b2 install -j8
