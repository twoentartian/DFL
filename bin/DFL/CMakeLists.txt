add_executable(DFL DFL.cpp)
target_link_libraries(DFL "${OPENSSL_CRYPTO_LIBRARY}" caffe caffeproto "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}" "${ROCKSDB_LIBRARIES}" "${LZ4_LIBRARIES}")
