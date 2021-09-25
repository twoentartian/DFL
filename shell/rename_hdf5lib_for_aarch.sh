#!/bin/bash

sudo ln -s /usr/lib/aarch64-linux-gnu/libhdf5_serial.so.8.0.2 /usr/lib/aarch64-linux-gnu/libhdf5.so
sudo ln -s /usr/lib/aarch64-linux-gnu/libhdf5_serial_hl.so.8.0.2 /usr/lib/aarch64-linux-gnu/libhdf5_hl.so

sed -i '418i//LIBRARY_DIRS += /usr/lib/aarch64-linux-gnu/hdf5/serial/'  ../3rd/caffe/Makefile