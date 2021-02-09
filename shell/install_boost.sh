#!/bin/bash
sudo apt install -y wget gcc g++

BOOST_MAJOR_VERSION="1"
BOOST_MINOR_VERSION="75"
BOOST_ZERO_VERSION="0"
BOOST_VERSION=${BOOST_MAJOR_VERSION}_${BOOST_MINOR_VERSION}_${BOOST_ZERO_VERSION}
BOOST_VERSION_DOT=${BOOST_MAJOR_VERSION}.${BOOST_MINOR_VERSION}.${BOOST_ZERO_VERSION}
cd ..
mkdir 3rd
cd 3rd || exit
if [ -d "./boost_${BOOST_VERSION}" ]; then
  echo "boost_${BOOST_VERSION} exists"
else
  echo "downloading boost_BOOST_VERSION"
  wget https://dl.bintray.com/boostorg/release/${BOOST_VERSION_DOT}/source/boost_${BOOST_VERSION}.tar.gz || exit
  tar -zxf boost_${BOOST_VERSION}.tar.gz || exit
  rm boost_${BOOST_VERSION}.tar.gz || exit
fi
cd boost_${BOOST_VERSION} || exit
./bootstrap.sh
./b2
