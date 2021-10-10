#!/bin/bash

# create caffe snap folder
mkdir "../dataset/MNIST/result"

cd ..
mkdir "3rd"
sudo chmod 777 ./3rd
cd "3rd" || exit

# openBLS
#sudo apt-get install -y libopenblas-dev

# atlas
sudo apt-get install -y libatlas-base-dev liblapack-dev libblas-dev

# protobuf
#! /bin/bash
if type protoc >/dev/null 2>&1; then
  echo 'protobuf already installed'
else
  echo 'installing protobuf'
  sudo apt-get install autoconf automake libtool curl make g++ unzip -y
  git clone https://github.com/google/protobuf.git
  cd protobuf || exit
  git checkout 3.3.x
  git submodule update --init --recursive
  ./autogen.sh
  ./configure
  make -j12
  make check
  sudo make install
  sudo ldconfig
  cd ..
fi


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
sudo apt install -y libgoogle-glog-dev libgflags-dev libgflags-dev libhdf5-dev libturbojpeg0-dev libopenblas-dev

# caffe
if type nvcc >/dev/null 2>&1; then
  echo 'cuda detected, use nvidia caffe'
  if [ ! -d "./caffe" ]; then
    git clone https://github.com/twoentartian/caffe.git
  fi
  cd caffe ||exit
  cp ../../shell/caffe_config/Makefile.config .
  cp ../../shell/caffe_config/CMakeLists.txt .
  make all -j"$(nproc)"

  protoc src/caffe/proto/caffe.proto --cpp_out=.
  mkdir include/caffe/proto
  mv src/caffe/proto/caffe.pb.h include/caffe/proto
else
  echo 'cuda not detected, use BLVC caffe'
  if [ ! -d "./caffe" ]; then
    git clone https://github.com/BVLC/caffe.git
  fi
  cd caffe ||exit
  cp ../../shell/caffe_config/Makefile.config .
  cp ../../shell/caffe_config/CMakeLists.txt .
  make all -j"$(nproc)"

  protoc src/caffe/proto/caffe.proto --cpp_out=.
  mkdir include/caffe/proto
  mv src/caffe/proto/caffe.pb.h include/caffe/proto
fi

# patch caffe
cd ../../shell || exit
sh ./patch_caffe.sh