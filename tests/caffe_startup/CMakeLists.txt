add_executable(TEST_Caffe_Startup main.cpp)
#target_link_libraries(TEST_TensorFlow_Startup ${TENSORFLOW_LIB})
target_link_libraries(TEST_Caffe_Startup caffe caffeproto "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}")

add_executable(TEST_Caffe_lmdb_append test_lmdb_append.cpp)
target_link_libraries(TEST_Caffe_lmdb_append caffe caffeproto "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}")

add_executable(TEST_Caffe_mutual_train_demo mutual_train_demo.cpp)
target_link_libraries(TEST_Caffe_mutual_train_demo caffe caffeproto "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}")

add_executable(TEST_Caffe_mutual_train_random_test mutual_train_demo_random_data.cpp)
target_link_libraries(TEST_Caffe_mutual_train_random_test caffe caffeproto "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}")

add_executable(TEST_Caffe_mutual_train_non_iid mutual_train_demo_non_iid.cpp)
target_link_libraries(TEST_Caffe_mutual_train_non_iid caffe caffeproto "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}")