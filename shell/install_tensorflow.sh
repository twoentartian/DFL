#!/bin/bash
#add bazel repo
sudo apt install curl gnupg
curl -fsSL https://bazel.build/bazel-release.pub.gpg | gpg --dearmor > bazel.gpg
sudo mv bazel.gpg /etc/apt/trusted.gpg.d/
echo "deb [arch=amd64] https://storage.googleapis.com/bazel-apt stable jdk1.8" | sudo tee /etc/apt/sources.list.d/bazel.list
sudo apt update

sudo apt install -y cmake git
sudo apt install -y autoconf automake libtool curl make gcc g++ unzip  # Protobuf Dependencies
sudo apt install -y swig bazel-3.1.0 bazel python3-pip python3  # TensorFlow Dependencies

#numpy
pip3 install numpy six keras_preprocessing

cd ..
mkdir "3rd"
cd "3rd" || exit

# protobuf-3.9.2
PROTOBUF_VERSION=3.9.2
if [ -d "./protobuf-${PROTOBUF_VERSION}" ]; then
  echo "protobuf-${PROTOBUF_VERSION} exists"
else
  echo "downloading protobuf-${PROTOBUF_VERSION}"
  wget https://github.com/protocolbuffers/protobuf/releases/download/v${PROTOBUF_VERSION}/protobuf-all-${PROTOBUF_VERSION}.tar.gz || exit
  tar xvf protobuf-all-${PROTOBUF_VERSION}.tar.gz || exit
  rm protobuf-all-${PROTOBUF_VERSION}.tar.gz || exit
  cd protobuf-${PROTOBUF_VERSION} || exit
  ./autogen.sh
  ./configure
  make
  sudo make install
fi

# tensorflow-2.4.0
if [ ! -d "./tensorflow" ]; then
  git clone --recursive https://github.com/tensorflow/tensorflow              # TensorFlow
fi
cd "tensorflow" || exit
git checkout v2.4.0
./configure      # Note that this requires user input
bazel --version
bazel build --incompatible_do_not_split_linking_cmdline --config=opt //tensorflow:libtensorflow_cc.so //tensorflow/tools/pip_package:build_pip_package|| exit
./bazel-bin/tensorflow/tools/pip_package/build_pip_package /tmp/tensorflow_pkg
pip3 install /tmp/tensorflow_pkg/tensorflow-2.4.0-cp38-cp38-linux_x86_64.whl

mkdir "build"
mkdir "build/include"
mkdir "build/link"
sudo cp -rf "${HOME}/.local/lib/python3.8/site-packages/tensorflow/include" "./build"
#sudo cp  "./bazel-bin/tensorflow/libtensorflow_cc.so" "./build/link/libtensorflow_cc.so"
#sudo cp  "./bazel-bin/tensorflow/libtensorflow_cc.so.2" "./build/link/libtensorflow_cc.so.2"
#sudo cp  "./bazel-bin/tensorflow/libtensorflow_cc.so.2.4.0" "./build/link/libtensorflow_cc.so.2.4.0"
#sudo cp  "./bazel-bin/tensorflow/libtensorflow_framework.so" "./build/link/libtensorflow_framework.so"
#sudo cp  "./bazel-bin/tensorflow/libtensorflow_framework.so.2" "./build/link/libtensorflow_framework.so.2"
#sudo cp  "./bazel-bin/tensorflow/libtensorflow_framework.so.2.4.0" "./build/link/libtensorflow_framework.so.2.4.0"

sudo cp  "./bazel-bin/tensorflow/libtensorflow_cc.so.2.4.0" "./build/link/libtensorflow_cc.so"
sudo cp  "./bazel-bin/tensorflow/libtensorflow_framework.so.2.4.0" "./build/link/libtensorflow_framework.so"