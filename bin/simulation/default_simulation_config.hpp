#pragma once

#include <tuple>
#include <configure_file.hpp>
#include "global_types.hpp"

configuration_file::json get_default_simulation_configuration()
{
	configuration_file::json output;
	
	output["ml_solver_proto"] = "../../../dataset/MNIST/lenet_solver_memory.prototxt";
	output["ml_train_dataset"] = "../../../dataset/MNIST/train-images.idx3-ubyte";
	output["ml_train_dataset_label"] = "../../../dataset/MNIST/train-labels.idx1-ubyte";
	output["ml_test_dataset"] = "../../../dataset/MNIST/t10k-images.idx3-ubyte";
	output["ml_test_dataset_label"] = "../../../dataset/MNIST/t10k-labels.idx1-ubyte";
	
	output["ml_max_tick"] = 1200;
	output["ml_train_batch_size"] = 64;
	output["ml_test_interval_tick"] = 10;
	output["ml_test_batch_size"] = 100;
	output["ml_non_iid_higher_frequency_lower"] = 10.0;
	output["ml_non_iid_higher_frequency_upper"] = 15.0;
	output["ml_non_iid_lower_frequency_lower"] = 3.0;
	output["ml_non_iid_lower_frequency_upper"] = 4.0;
	output["ml_dataset_all_possible_labels"] = configuration_file::json::array({0,1,2,3,4,5,6,7,8,9});
	output["ml_reputation_dll_path"] = "./libreputation_api_sample.so";
	
	configuration_file::json node;
	node["name"] = "1";
	node["dataset_mode"] = "default"; //default - randomly choose from dataset, iid - randomly choose from iid labels, non-iid - choose higher frequency labels for specific label
	node["non_iid_high_labels"] = configuration_file::json::array({0,1});
	node["training_interval_tick"] = configuration_file::json::array({8,9,10});
	node["buffer_size"] = 2;
	node["model_generation_type"] = "filtered"; //normal, filtered
	node["filter_limit"] = 0.5;
	
	configuration_file::json nodes = configuration_file::json::array();
	nodes.push_back(node);
	node["name"] = "2";
	node["dataset_mode"] = "iid";
	nodes.push_back(node);
	
	output["nodes"] = nodes;
	
	return output;
}
