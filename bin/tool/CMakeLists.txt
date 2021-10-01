add_executable(generate_node_address generate_node_address.cpp)
target_link_libraries(generate_node_address "${OPENSSL_CRYPTO_LIBRARY}")

add_executable(print_rocksdb print_rocksdb.cpp)
target_link_libraries(print_rocksdb "${ROCKSDB_LIBRARIES}" "${GLOG_LIBRARY}")

add_executable(block_db_to_json block_db_to_json.cpp)
target_link_libraries(block_db_to_json "${OPENSSL_CRYPTO_LIBRARY}" caffe caffeproto "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}" "${ROCKSDB_LIBRARIES}" "${LZ4_LIBRARIES}")

add_executable(dirichlet_distribution_config_generator dirichlet_distribution_config_generator.cpp)
target_link_libraries(dirichlet_distribution_config_generator "${GLOG_LIBRARY}" "${Boost_LIBRARIES}")

add_executable(network_receiver network_test_receiver.cpp)
target_link_libraries(network_receiver "${GLOG_LIBRARY}" "${Boost_LIBRARIES}" pthread)

add_executable(network_sender network_test_sender.cpp)
target_link_libraries(network_sender "${GLOG_LIBRARY}" "${Boost_LIBRARIES}" pthread)

add_executable(large_scale_simulation_generator large_scale_simulation_generator.cpp)
target_link_libraries(large_scale_simulation_generator "${GLOG_LIBRARY}" "${Boost_LIBRARIES}" pthread)

add_executable(remove_caffe_log remove_caffe_log.cpp)
target_link_libraries(remove_caffe_log "${GLOG_LIBRARY}" "${Boost_LIBRARIES}" pthread)
