add_executable(TEST_Caffe_single_train_demo single_train_demo.cpp)
target_link_libraries(TEST_Caffe_single_train_demo caffe caffeproto "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}")

add_executable(TEST_Caffe_mutual_train_random_test mutual_train_demo_random_data.cpp)
target_link_libraries(TEST_Caffe_mutual_train_random_test caffe caffeproto "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}")

add_executable(TEST_Caffe_mutual_train_non_iid mutual_train_demo_non_iid.cpp)
target_link_libraries(TEST_Caffe_mutual_train_non_iid caffe caffeproto "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}")

add_executable(TEST_Caffe_mutual_train_non_iid_fedavg mutual_train_demo_non_iid_fedavg.cpp)
target_link_libraries(TEST_Caffe_mutual_train_non_iid_fedavg caffe caffeproto "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}")

add_executable(TEST_Caffe_mutual_train_malicious_node mutual_train_demo_malicious_node.cpp)
target_link_libraries(TEST_Caffe_mutual_train_malicious_node caffe caffeproto "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}")

add_executable(TEST_Caffe_tyd_casual single_train_tyd_casual_test.cpp)
target_link_libraries(TEST_Caffe_tyd_casual caffe caffeproto "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}")

add_executable(TEST_Caffe_mutual_train_model_diff mutual_train_demo_model_diff.cpp)
target_link_libraries(TEST_Caffe_mutual_train_model_diff caffe caffeproto "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}")

add_executable(TEST_Caffe_mutual_train_model_diff_non_iid mutual_train_demo_model_diff_none_iid.cpp)
target_link_libraries(TEST_Caffe_mutual_train_model_diff_non_iid caffe caffeproto "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}")

add_executable(TEST_Caffe_boost_unit_test boost_test.cpp)
target_link_libraries(TEST_Caffe_boost_unit_test caffe caffeproto "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}" "${LZ4_LIBRARIES}")
