#!/bin/bash

cd ..
mkdir "3rd"
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
make
make check
sudo make install
sudo ldconfig
cd ..

# OpenCV
#wget https://github.com/opencv/opencv/archive/2.4.13.5.zip -O opencv-2.4.13.5.zip
#unzip opencv-2.4.13.5.zip
#cd opencv-2.4.13.5 ||exit
#mkdir release
#cd release ||exit
#cmake -G "Unix Makefiles" -DCMAKE_CXX_COMPILER=/usr/bin/g++ CMAKE_C_COMPILER=/usr/bin/gcc -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_INSTALL_PREFIX=/usr/local -DWITH_TBB=ON -DBUILD_NEW_PYTHON_SUPPORT=ON -DWITH_V4L=ON -DINSTALL_C_EXAMPLES=ON -DINSTALL_PYTHON_EXAMPLES=ON -DBUILD_EXAMPLES=ON -DWITH_QT=ON -DWITH_OPENGL=ON -DBUILD_FAT_JAVA_LIB=ON -DINSTALL_TO_MANGLED_PATHS=ON -DINSTALL_CREATE_DISTRIB=ON -DINSTALL_TESTS=ON -DENABLE_FAST_MATH=ON -DWITH_IMAGEIO=ON -DBUILD_SHARED_LIBS=OFF -DWITH_GSTREAMER=ON ..
#make all -j"$(nproc)" # Uses all machine cores
#sudo make install
#cd ../../
#rm -rf ./opencv-2.4.13.5
#echo -e "OpenCV version:"
#pkg-config --modversion opencv

# other dependencies
sudo apt install -y libgoogle-glog-dev libgflags-dev libgflags-dev libhdf5-dev

# caffe
git clone git@github.com:BVLC/caffe.git
cd caffe ||exit
cp ../../shell/caffe_config/Makefile.config .
make all -j"$(nproc)"
