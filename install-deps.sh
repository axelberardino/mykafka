#!/bin/bash

sudo aptitude install \
     build-essential autoconf libtool libgflags-dev \
     libgtest-dev clang libc++-dev automake wbritish \
     libboost-test-dev libboost-mutex-dev \
     libboost-system-dev libboost-thread-dev \
     libboost-filesystem-dev libboost-program-options-dev &&\
git clone -b $(curl -L http://grpc.io/release) https://github.com/grpc/grpc && \
cd grpc && \
git submodule update --init && \
make && \
sudo make install && \
cd ./grpc/third_party/pr/grpc/third_party/protobuf/ && \
sudo make install
