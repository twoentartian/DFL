add_executable(generate_node_address generate_node_address.cpp)
target_link_libraries(generate_node_address "${OPENSSL_CRYPTO_LIBRARY}")

add_executable(print_rocksdb print_rocksdb.cpp)
target_link_libraries(print_rocksdb "${ROCKSDB_LIBRARIES}" "${GLOG_LIBRARY}")

add_executable(block_db_to_json block_db_to_json.cpp)
target_link_libraries(block_db_to_json "${OPENSSL_CRYPTO_LIBRARY}" caffe caffeproto "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}" "${ROCKSDB_LIBRARIES}" "${LZ4_LIBRARIES}")

add_executable(dirichlet_distribution_config_generator dirichlet_distribution_config_generator.cpp)
target_link_libraries(dirichlet_distribution_config_generator "${GLOG_LIBRARY}" "${Boost_LIBRARIES}")