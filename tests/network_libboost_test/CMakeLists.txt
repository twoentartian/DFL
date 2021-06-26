add_subdirectory(v2)

add_executable(TEST_network_libboost_tcp_loopback test_network_libboost_tcp_loopback.cpp)
target_link_libraries(TEST_network_libboost_tcp_loopback "${Boost_LIBRARIES}" -pthread)

add_executable(TEST_network_libboost_udp test_network_libboost_udp.cpp)
target_link_libraries(TEST_network_libboost_udp "${Boost_LIBRARIES}" -pthread)

add_executable(TEST_boost_udp_sample_client boost_udp_sample_client.cpp)
target_link_libraries(TEST_boost_udp_sample_client "${Boost_LIBRARIES}" -pthread)

add_executable(TEST_boost_udp_sample_server boost_udp_sample_server.cpp)
target_link_libraries(TEST_boost_udp_sample_server "${Boost_LIBRARIES}" -pthread)

add_executable(TEST_network_libboost_p2p test_network_libboost_p2p.cpp)
target_link_libraries(TEST_network_libboost_p2p "${Boost_LIBRARIES}" "${GLOG_LIBRARY}" -pthread)


add_executable(TEST_network_libboost_crc test_tcp_crc.cpp)
target_link_libraries(TEST_network_libboost_crc caffe caffeproto "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}" "${OPENSSL_CRYPTO_LIBRARY}"  -pthread )