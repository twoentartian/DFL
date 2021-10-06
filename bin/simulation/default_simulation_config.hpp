#pragma once

#include <tuple>
#include <configure_file.hpp>
#include "global_types.hpp"

configuration_file::json get_default_simulation_configuration()
{
	configuration_file::json output;
	
	output["report_time_remaining_per_tick_elapsed"] = 100;
	
	output["ml_solver_proto"] = "../../../dataset/MNIST/lenet_solver_memory.prototxt";
	output["ml_train_dataset"] = "../../../dataset/MNIST/train-images.idx3-ubyte";
	output["ml_train_dataset_label"] = "../../../dataset/MNIST/train-labels.idx1-ubyte";
	output["ml_test_dataset"] = "../../../dataset/MNIST/t10k-images.idx3-ubyte";
	output["ml_test_dataset_label"] = "../../../dataset/MNIST/t10k-labels.idx1-ubyte";
	output["ml_delayed_test_accuracy"] = false;
	
	output["ml_max_tick"] = 1200;
	output["ml_train_batch_size"] = 64;
	output["ml_test_interval_tick"] = 10;
	output["ml_test_batch_size"] = 100;
	output["ml_non_iid_normal_weight"] = configuration_file::json::array({10.0, 15.0});
	output["ml_dataset_all_possible_labels"] = configuration_file::json::array({0,1,2,3,4,5,6,7,8,9});
	output["ml_reputation_dll_path"] = "../reputation_sdk/sample/libreputation_api_sample.so";
	
	configuration_file::json node;
	configuration_file::json node_non_iid = configuration_file::json::object();
	node_non_iid["1"] = configuration_file::json::array({1.0,2.0});
	node_non_iid["3"] = configuration_file::json::array({2.0,3.0});
	node["name"] = "1";
	node["dataset_mode"] = "default"; //default - randomly choose from dataset, iid - randomly choose from iid labels, non-iid - choose higher frequency labels for specific label
	node["training_interval_tick"] = configuration_file::json::array({8,9,10,11,12});
	node["buffer_size"] = 2;
	node["model_generation_type"] = "compressed"; //normal, compressed
	node["filter_limit"] = 0.5;
	node["node_type"] = "normal";
	node["non_iid_distribution"] = node_non_iid;
	
	configuration_file::json nodes = configuration_file::json::array();
	nodes.push_back(node);
	node["name"] = "2";
	node["dataset_mode"] = "iid";
	node["node_type"] = "malicious_random_strategy";
	nodes.push_back(node);
	
	output["nodes"] = nodes;
	
	output["node_topology"] = configuration_file::json::array({"fully_connect", "average_degree-2", "1->2", "1--2"});
	
	return output;
}
