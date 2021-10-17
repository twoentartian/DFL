#!/bin/bash

sed -i '79s/LOG(WARNING)/\/\/LOG(WARNING)/' ../3rd/caffe/include/caffe/solver.hpp
sed -i '90s/LOG(WARNING)/\/\/LOG(WARNING)/' ../3rd/caffe/src/caffe/layers/memory_data_layer.cpp