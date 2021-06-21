add_executable(TEST_Blockchain_transaction transaction_test.cpp)
target_link_libraries(TEST_Blockchain_transaction caffe caffeproto "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}" "${OPENSSL_CRYPTO_LIBRARY}")

add_executable(TEST_Blockchain_block block_test.cpp)
target_link_libraries(TEST_Blockchain_block caffe caffeproto "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}" "${OPENSSL_CRYPTO_LIBRARY}")

add_executable(TEST_Blockchain_dataset_storage dataset_storage_test.cpp)
target_link_libraries(TEST_Blockchain_dataset_storage caffe caffeproto "${ROCKSDB_LIBRARIES}" "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}" "${OPENSSL_CRYPTO_LIBRARY}")

