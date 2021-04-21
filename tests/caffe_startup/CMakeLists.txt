add_executable(TEST_Caffe_Startup main.cpp)
#target_link_libraries(TEST_TensorFlow_Startup ${TENSORFLOW_LIB})
target_link_libraries(TEST_Caffe_Startup caffe caffeproto "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}")