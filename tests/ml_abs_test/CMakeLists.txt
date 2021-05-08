add_executable(TEST_ml_abs_temp_test temp_test.cpp)
#target_link_libraries(TEST_TensorFlow_Startup ${TENSORFLOW_LIB})
target_link_libraries(TEST_ml_abs_temp_test caffe caffeproto "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}")

add_executable(TEST_ml_abs_caffe test_caffe.cpp)
target_link_libraries(TEST_ml_abs_caffe caffe caffeproto "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}")

add_executable(TEST_ml_abs_memory_data_layer test_memory_data_layer.cpp)
target_link_libraries(TEST_ml_abs_memory_data_layer caffe caffeproto "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}")