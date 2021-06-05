cmake_minimum_required(VERSION 3.17)
project(DFL)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

add_executable(DFL main.cpp)

# Caffe manual
#set(CAFFE_HOME 3rd/caffe)
#include_directories(${CAFFE_HOME}/include)
#link_directories(${CAFFE_HOME}/build/lib)
#set(Caffe_LIBRARIES libcaffe.so)

# Caffe cmake
set(CAFFE_HOME 3rd/caffe)
add_subdirectory(${CAFFE_HOME})
include_directories(${CAFFE_HOME}/include)

# Nlohmann Json
set(JSON_HOME 3rd/json)
add_subdirectory(${JSON_HOME})
include_directories(${JSON_HOME}/include)

# uv-cpp
add_subdirectory(3rd/uv-cpp)
include_directories(3rd/uv-cpp)
LINK_DIRECTORIES(3rd/uv-cpp/libuv1.30.0/lib/gcc8.3.0)

# uvw
# add_subdirectory(3rd/uvw)
# include_directories(3rd/uvw/src)

# OS
if (WIN32)
    message(STATUS "Operation system is WIN32")
    set(BOOST_ROOT "C:\\lib\\boost_1_75_0")
elseif(APPLE)
    message(STATUS "Operation system is APPLE")
    set(BOOST_ROOT "./3rd/boost_1_75_0")
elseif(UNIX)
    message(STATUS "Operation system is UNIX")
    set(BOOST_ROOT "./3rd/boost_1_75_0")
endif ()


# Boost
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost 1.71.0 REQUIRED COMPONENTS system filesystem serialization unit_test_framework )
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(DFL ${Boost_LIBRARIES})
else()
    message(FATAL_ERROR "Boost not found")
endif()

# find glog
find_package(GLOG REQUIRED)

# find protobuf
find_package(Protobuf REQUIRED)

# find levelDB
find_package(LevelDB REQUIRED)

# find snappy
find_package(Snappy REQUIRED)

# find LMDB
find_package(LMDB REQUIRED)

# find OpenCV
find_package(OpenCV REQUIRED)

# OpenSSL
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

list(APPEND Caffe_libs "${CAFFE_LIB}" "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}")

# TensorFlow
#set(TENSORFLOW_HOME 3rd/tensorflow)
#include_directories(${TENSORFLOW_HOME}/build/include)
#link_directories(${TENSORFLOW_HOME}/build/link)
#set(TENSORFLOW_LIB libtensorflow_cc.so libtensorflow_framework.so libprotobuf.so)

include_directories(lib)

add_subdirectory(tests)
add_subdirectory(bin)