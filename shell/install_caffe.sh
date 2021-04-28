#!/bin/bash

cd ..
mkdir "3rd"
sudo chmod 777 ./3rd
cd "3rd" || exit

# boost
sudo apt-get install -y libboost-all-dev

# openBLS
#sudo apt-get install -y libopenblas-dev

# atlas
sudo apt-get install -y libatlas-base-dev liblapack-dev libblas-dev

# protobuf
sudo apt-get install autoconf automake libtool curl make g++ unzip -y
git clone https://github.com/google/protobuf.git
cd protobuf || exit
git submodule update --init --recursive
./autogen.sh
./configure
make
make check
sudo make install
sudo ldconfig
cd ..

# OpenCV
if [ ! -d "./opencv" ]; then
  git clone https://github.com/opencv/opencv
fi
cd opencv ||exit
git checkout 3.4
mkdir build
cd build ||exit
cmake ..
sudo make install -j"$(nproc)"
cd ../..

# levelDB
sudo apt install -y libleveldb-dev libsnappy-dev

# lmdb
sudo apt install -y liblmdb-dev

# other dependencies
sudo apt install -y libgoogle-glog-dev libgflags-dev libgflags-dev libhdf5-dev

# caffe
if [ ! -d "./caffe" ]; then
  git clone git@github.com:BVLC/caffe.git
fi
cd caffe ||exit
cp ../../shell/caffe_config/Makefile.config .
make all -j"$(nproc)"
make install
protoc src/caffe/proto/caffe.proto --cpp_out=.
mkdir include/caffe/proto
mv src/caffe/proto/caffe.pb.h include/caffe/proto
