sudo apt install -y cmake git gcc g++
sudo apt install -y autoconf automake libtool curl make g++ unzip  # Protobuf Dependencies
sudo apt install -y python-numpy swig python-dev bazel-3.1.0 bazel    # TensorFlow Dependencies

mkdir "3rd"
cd "3rd" || exit
git clone https://github.com/tensorflow/tensorflow              # TensorFlow
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
bazel build tensorflow:libtensorflow_all.so || exit
mkdir "out"
mkdir "out/obj"
mkdir "out/include"
sudo cp bazel-bin/tensorflow/libtensorflow_all.so ./out/obj
sudo cp -r tensorflow ./out/include
sudo find ./out/include/tensorflow -type f  ! -name "*.h" -delete
sudo cp bazel-genfiles/tensorflow/core/framework/*.h  ./out/include/tensorflow/core/framework
sudo cp bazel-genfiles/tensorflow/core/kernels/*.h  ./out/include/tensorflow/core/kernels
sudo cp bazel-genfiles/tensorflow/core/lib/core/*.h  ./out/include/tensorflow/core/lib/core
sudo cp bazel-genfiles/tensorflow/core/protobuf/*.h  ./out/include/tensorflow/core/protobuf
sudo cp bazel-genfiles/tensorflow/core/util/*.h  ./out/include/tensorflow/core/util
sudo cp bazel-genfiles/tensorflow/cc/ops/*.h  ./out/include/tensorflow/cc/ops