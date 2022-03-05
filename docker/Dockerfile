FROM ubuntu:18.04

RUN apt-get update

RUN echo 'root:root' |chpasswd

RUN apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# install basic libraries
RUN apt-get update
RUN apt update
RUN apt-get install -y git vim htop sudo
RUN apt-get install -y make cmake
RUN apt-get install -y gcc
RUN apt-get install -y gdb
RUN apt-get install -y g++ strace
RUN apt-get install -y silversearcher-ag
RUN apt-get install -y net-tools

# copy .gitconfig
COPY gitconfig /root/.gitconfig

# deploy Rolis environment
WORKDIR "/root"
# using branch "master" for the Rolis
RUN git clone https://github.com/shenweihai1/rolis-eurosys2022.git

RUN apt-get install -y wget
WORKDIR "/root/rolis-eurosys2022"
# install dependencies
RUN bash install.sh