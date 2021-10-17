add_executable(DFL_simulator simulator.cpp)
target_link_libraries(DFL_simulator "${OPENSSL_CRYPTO_LIBRARY}" caffe caffeproto "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}" "${ROCKSDB_LIBRARIES}" "${LZ4_LIBRARIES}")

add_executable(DFL_simulator_mt simulator_mt.cpp)
target_link_libraries(DFL_simulator_mt "${OPENSSL_CRYPTO_LIBRARY}" caffe caffeproto "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}" "${ROCKSDB_LIBRARIES}" "${LZ4_LIBRARIES}")

add_executable(DFL_simulator_ART simulator_ART.cpp)
target_link_libraries(DFL_simulator_ART "${GLOG_LIBRARY}" "${Boost_LIBRARIES}")
