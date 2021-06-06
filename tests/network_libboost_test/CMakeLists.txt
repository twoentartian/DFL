add_executable(TEST_network_libboost_tcp_loopback test_network_libboost_tcp_loopback.cpp)
target_link_libraries(TEST_network_libboost_tcp_loopback uv_cpp "${Boost_LIBRARIES}" "libuv.a" -pthread)