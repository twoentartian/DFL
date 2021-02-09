#!/bin/bash
#add bazel repo
sudo apt install curl gnupg
curl -fsSL https://bazel.build/bazel-release.pub.gpg | gpg --dearmor > bazel.gpg
sudo mv bazel.gpg /etc/apt/trusted.gpg.d/
echo "deb [arch=amd64] https://storage.googleapis.com/bazel-apt stable jdk1.8" | sudo tee /etc/apt/sources.list.d/bazel.list
sudo apt update

sudo apt install -y cmake git gcc g++
sudo apt install -y autoconf automake libtool curl make g++ unzip  # Protobuf Dependencies
sudo apt install -y swig bazel bazel-3.1.0 python3-pip python3  # TensorFlow Dependencies

#numpy
pip3 install numpy

cd ..
mkdir "3rd"
cd "3rd" || exit
if [ -d "./tensorflow" ]; then
  git clone https://github.com/tensorflow/tensorflow              # TensorFlow
fi
cd "tensorflow" || exit
git checkout v2.4.0

if [ `grep -c "libtensorflow_all.so" tensorflow/BUILD` -eq '0' ]; then
    if [ "$(uname)" == "Darwin" ]; then
    printf "# Added build rule
cc_binary(
    name = \"libtensorflow_all.so\",
    linkshared = 1,
    deps = [
        \"//tensorflow/core:framework_internal\",
        \"//tensorflow/core:tensorflow\",
        \"//tensorflow/cc:cc_ops\",
        \"//tensorflow/cc:client_session\",
        \"//tensorflow/cc:scope\",
        \"//tensorflow/c:c_api\",
    ],
) " >> tensorflow/BUILD
else
    printf "# Added build rule
cc_binary(
    name = \"libtensorflow_all.so\",
    linkshared = 1,
    linkopts = [\"-Wl,--version-script=tensorflow/tf_version_script.lds\"], # Remove this line if you are using MacOS
    deps = [
        \"//tensorflow/core:framework_internal\",
        \"//tensorflow/core:tensorflow\",
        \"//tensorflow/cc:cc_ops\",
        \"//tensorflow/cc:client_session\",
        \"//tensorflow/cc:scope\",
        \"//tensorflow/c:c_api\",
    ],
) " >> tensorflow/BUILD
fi
fi
./configure      # Note that this requires user input
bazel --version
bazel build tensorflow:libtensorflow_all.so || exit
mkdir "out"
mkdir "out/obj"
mkdir "out/include"
sudo cp bazel-bin/tensorflow/libtensorflow_all.so ./out/obj
sudo cp -r tensorflow ./out/include
sudo find ./out/include/tensorflow -type f  ! -name "*.h" -delete
sudo cp bazel-tensorflow/tensorflow/core/framework/*.h  ./out/include/tensorflow/core/framework
sudo cp bazel-tensorflow/tensorflow/core/kernels/*.h  ./out/include/tensorflow/core/kernels
sudo cp bazel-tensorflow/tensorflow/core/lib/core/*.h  ./out/include/tensorflow/core/lib/core
#sudo cp bazel-tensorflow/tensorflow/core/protobuf/*.h  ./out/include/tensorflow/core/protobuf #files not found.
sudo cp bazel-tensorflow/tensorflow/core/util/*.h  ./out/include/tensorflow/core/util
sudo cp bazel-tensorflow/tensorflow/cc/ops/*.h  ./out/include/tensorflow/cc/ops