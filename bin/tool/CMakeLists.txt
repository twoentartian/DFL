add_executable(generate_node_address generate_node_address.cpp)
target_link_libraries(generate_node_address "${OPENSSL_CRYPTO_LIBRARY}")

add_executable(print_rocksdb print_rocksdb.cpp)
target_link_libraries(print_rocksdb "${ROCKSDB_LIBRARIES}" "${GLOG_LIBRARY}")