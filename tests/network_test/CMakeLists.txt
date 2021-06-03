add_executable(TEST_network_tcp_echo network_tcp_echo.cpp)
target_link_libraries(TEST_network_tcp_echo uv_cpp "${Boost_LIBRARIES}" "libuv.a" -pthread)
