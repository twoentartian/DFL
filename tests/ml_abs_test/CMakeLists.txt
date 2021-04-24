add_executable(TEST_Ml_Abs main.cpp)
#target_link_libraries(TEST_TensorFlow_Startup ${TENSORFLOW_LIB})
target_link_libraries(TEST_Ml_Abs caffe caffeproto "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}")