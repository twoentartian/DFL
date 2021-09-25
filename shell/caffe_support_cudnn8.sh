#!/bin/bash

sed -i '201s/cudnn.h/cudnn_version.h/' ../3rd/caffe/cmake/Cuda.cmake