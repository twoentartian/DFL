add_library (reputation_api_sample SHARED "sample_reputation.cpp")
target_link_libraries (reputation_api_sample caffe caffeproto "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}")

add_library (reputation_implementation_tyd SHARED "tyd_test_reputation.cpp")
target_link_libraries (reputation_implementation_tyd caffe caffeproto "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}")
