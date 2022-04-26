# DFL

## What is DFL?
DFL is a blockchain framework integrating specially optimized for, and works for federated machine learning. In DFL, all contributions are reflected on the improvements of model accuracy and blockchain database works as a proof of contribution rather than a distributed ledger.

## Toolchain
Here are two tested toolchain configuration cases.
### General x86-64
- Ubuntu 20
- GCC 9.3.0
- CMake 3.16
- Boost 1.76
- CUDA 10.2 (optional)

### Arm Aarch64: Nvidia Jetson Nano 2Gb
- Ubuntu 18 (Official Jetson image)
- GCC 9.4.0
- CMake 3.16
- Boost 1.76
- CUDA 10.2 (under testing)
- CuDNN 8 (under testing)

## Dependency

- Caffe, DFL uses Caffe as the machine learning backend, CUDA support is still under testing.
- Boost 1.76
- [nlohmann/json](https://github.com/nlohmann/json)
- RocksDB
- Lz4
- OpenSSL

## Getting started

### For deployment

1. Install CMake and GCC with C++17 support.
2. You can install above dependencies by executing the shell scripts in [shell folder](https://github.com/twoentartian/DFL/tree/main/shell). For most cases, you should execute these scripts with this order:
    - [install_boost.sh](https://github.com/twoentartian/DFL/blob/main/shell/install_boost.sh)
    - [install_json.sh](https://github.com/twoentartian/DFL/blob/main/shell/install_json.sh)
    - [install_miscellaneous.sh](https://github.com/twoentartian/DFL/blob/main/shell/install_miscellaneous.sh)
    - [install_caffe.sh](https://github.com/twoentartian/DFL/blob/main/shell/install_caffe.sh)
    - [patch_caffe.sh]()
   
	If you are going to depoly DFL to Jetson Nano, you must execute two additional scripts:
	-   [rename_hdf5lib_for_aarch.sh](https://github.com/twoentartian/DFL/blob/main/shell/rename_hdf5lib_for_aarch.sh)
	-   [caffe_support_cudnn8.sh](https://github.com/twoentartian/DFL/blob/main/shell/caffe_support_cudnn8.sh)
3. Compile DFL executable(the source code is in [DFL.cpp](https://github.com/twoentartian/DFL/blob/main/bin/DFL/DFL.cpp), you can find everything you need in CMake), which will start a node in the DFL network. There are several tools that we recommend to build, they are listed below:

	- [Keys generator](https://github.com/twoentartian/DFL/blob/main/bin/tool/generate_node_address.cpp): to generate private keys and public keys. These keys will be used in the configuration file.

4. Compile your own "reputation algorithm", which will define the way of updating ML models and updating the other nodes' reputation. This implementation is critical for different dataset distribution, malicious ratio situations. We provide four sample "reputation algorithm" [here](https://github.com/twoentartian/DFL/tree/main/bin/reputation_sdk/sample). 

5. Run DFL executable, it should provide a sample configuration file for you.

6. Modify the configuration file as you wish, for example, peers, node address, private key, public key, etc. Notice that the batch_size and test_batch_size must be identical to the Caffe solver's configuration. Here is an [explaination file](https://github.com/twoentartian/DFL/blob/main/readme/config.json.txt) for the configuration.

7. DFL receive ML dataset by network, there is an executable file called [data_injector](https://github.com/twoentartian/DFL/blob/main/bin/data_injector/mnist_data_injector.cpp) for MNIST dataset, use it to inject dataset to DFL. Current version of data_injector only supports I.I.D. dataset injection.

8. DFL will train the model once it receives enough dataset for training, and send it as a transaction to other nodes. The node will generate a block when generating enough transactions and perform FedAvg when receiving enough models from other nodes.

### For simulation

1. Perform step 1, step 2 and step 4 in deployment.

2. Compile DFL_Simulator_mt (source file: [simulator_mt.cpp](https://github.com/twoentartian/DFL/blob/main/bin/simulation/simulator_mt.cpp)). This version have multi-threading optimization.

   Some tools:
   - [Dirichlet_distribution_generator_for_Non_IID dataset](https://github.com/twoentartian/DFL/blob/main/bin/tool/dirichlet_distribution_config_generator.cpp), used to generate Dirichlet distribution. You can execute without any arguments it to get its usage. 

   - [large_scale_simulation_generator](https://github.com/twoentartian/DFL/blob/main/bin/tool/large_scale_simulation_generator.cpp), it can automatically generate a configuration file for many many nodes (the configuration file is over 3000+ lines, so you'd better use this tool if you want to simulate for over 20 nodes).

3. Run the simulator, it should generate a sample configuration file and execute simulation immediately. You can use Ctrl+C to exit.

4. Modify the configuration file with this [explanation file](https://github.com/twoentartian/DFL/blob/main/readme/simulator_config.json.txt).

5. The simulator will automatically crate an output folder, whose name is the current time, in the executable path. The configuration file and reputation dll will also be copied to the output folder for easily reproduce the output.

We provide a sample simulation output folder [here](https://github.com/twoentartian/DFL/tree/main/readme/sample_simulation_result), you can reuse the [reputation dll](https://github.com/twoentartian/DFL/blob/main/readme/sample_simulation_result/libreputation_HalfFedAvg.so)  and the [configuration](https://github.com/twoentartian/DFL/blob/main/readme/sample_simulation_result/simulator_config.json). This configuration contains 5 nodes(1 observer) and all of them use IID dataset. Please note that this configuration uses HalfFedAvg(the output model = 50% previous model + 50% FedAvg output) because there is no malicious node.



### Reputation algorithm SDK API:

Please refer to this [link](https://github.com/twoentartian/DFL/tree/main/bin/reputation_sdk/sample) for sample reputation algorithm. The SDK API is not written yet.



### Future work:

- Large scale DFL deployment (50+ nodes) tool is on its way (50%). Introducer is under testing.


### For more details
https://arxiv.org/pdf/2110.15457.pdf