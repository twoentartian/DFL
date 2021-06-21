add_executable(TEST_rocksdb_performance_test test_rocksdb_performance.cpp)
target_link_libraries(TEST_rocksdb_performance_test "${Boost_LIBRARIES}" "${ROCKSDB_LIBRARIES}" -pthread)