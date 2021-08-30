# DFL

## What is DFL?
DFL is a blockchain framework integrating specially optimized for, and works for federated machine learning. In DFL, all contributions are reflected on the improvements of model accuracy and blockchain database works as a proof of contribution rather than a distributed ledger.

## Dependency

- Caffe, DFL uses Caffe as the machine learning backend, CUDA support is still under testing.
- Boost 1.76
- [nlohmann/json](https://github.com/nlohmann/json)
- RocksDB
- Lz4
- OpenSSL

## Getting started

1. You can install above dependencies by executing the shell scripts in shell folder. 
2. Install CMake and GCC with C++17 support.
3. Compile DFL, which is a node in the DFL network. Optional: compiler DFL_Simulator if you want to simulate the network behavior and machine learning performance rather than deploying them.
4. Write you own "reputation dll", which will define the way of updating ML models and updating the other nodes' reputation. This implementation is critical for different dataset distribution, malicious ratio situations. We provide a sample "reputation dll" [here](https://github.com/twoentartian/DFL/blob/main/bin/reputation_sdk/sample/sample_reputation.cpp). We will write a tutorial to explain the reputation implementation in the future(hopefully).
5. Run DFL, it should provide a sample configuration file for you.
6. Modify the configuration as you wish, for example, peers, node address, private key, public key, etc. Notice that the batch_size and test_batch_size must be identical to the Caffe solver configuration.
7. DFL receive ML dataset by network, there is an executable file called "data_injector" for MNIST dataset, use it to inject dataset to DFL.
8. DFL will train the model once it receives enough dataset for training, and send it as a transaction to other nodes. The node will generate a block when generating enough transactions and perform FedAvg when receiving enough models from other nodes.

For simulator, 