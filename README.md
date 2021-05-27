# DFL

DFL stands for Decentralized Federated Learning.

- Decentralized: no center server.
- Federated: only clients hold the data.
- Learning: train a neural network.

Development environment configuration notes:
- Boost: >=1.75, due to Boost.json
- TensorFlow C: currently tested 2.4.0, manually compiled required.
  
  [Compile](https://github.com/tensorflow/tensorflow/blob/master/tensorflow/tools/lib_package/README.md) TensorFlow:
  
  bazel test --config opt //tensorflow/tools/lib_package:libtensorflow_test
  
  bazel build --config opt //tensorflow/tools/lib_package:libtensorflow
  

Dependency:
- bazel: 3.1.0
- python
- numpy
