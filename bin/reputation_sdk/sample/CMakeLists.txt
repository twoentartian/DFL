add_library (reputation_api_sample SHARED "sample_reputation.cpp")
target_link_libraries (reputation_api_sample caffe caffeproto "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}")

add_library (reputation_FedAvg SHARED "FedAvg_reputation.cpp")
target_link_libraries (reputation_FedAvg caffe caffeproto "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}")

add_library (reputation_ML_hcluster SHARED "hcluster_reputation.cpp")
target_link_libraries (reputation_ML_hcluster caffe caffeproto "${GLOG_LIBRARY}" "${Protobuf_LIBRARIES}" "${snappy_LIBRARIES}" "${LevelDB_LIBRARIES}" "${LMDB_LIBRARIES}" "${OpenCV_LIBS}" "${Boost_LIBRARIES}")
