add_executable(TEST_network_libuv_tcp_echo test_network_libuv_tcp_echo.cpp)
target_link_libraries(TEST_network_libuv_tcp_echo uv_cpp "${Boost_LIBRARIES}" "libuv.a" -pthread) # uv_cpp
# target_link_libraries(TEST_network_tcp_echo "${Boost_LIBRARIES}" -pthread) #uvw

add_executable(TEST_network_libuv_tcp_server test_network_libuv_tcp_server.cpp)
target_link_libraries(TEST_network_libuv_tcp_server uv_cpp "${Boost_LIBRARIES}" "libuv.a" -pthread)

add_executable(TEST_network_libuv_tcp_client test_network_libuv_tcp_client.cpp)
target_link_libraries(TEST_network_libuv_tcp_client uv_cpp "${Boost_LIBRARIES}" "libuv.a" -pthread)

add_executable(TEST_network_libuv_cpp_loop_exit test_network_libuv_cpp_loop_exit.cpp)
target_link_libraries(TEST_network_libuv_cpp_loop_exit uv_cpp "${Boost_LIBRARIES}" "libuv.a" -pthread)

add_executable(TEST_network_libuv_tcp_server_block test_network_libuv_tcp_server_block.cpp)
target_link_libraries(TEST_network_libuv_tcp_server_block uv_cpp "${Boost_LIBRARIES}" "libuv.a" -pthread)

add_executable(TEST_network_libuv_p2p test_network_libuv_p2p.cpp)
target_link_libraries(TEST_network_libuv_p2p uv_cpp "${Boost_LIBRARIES}" "libuv.a" -pthread)