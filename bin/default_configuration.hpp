#pragma once

#include <tuple>
#include <configure_file.hpp>
#include "global_types.hpp"

configuration_file::json get_default_configuration()
{
	configuration_file::json output;
	output["blockchain_public_key"] = "06b0caa32089e1430f2472581f6ac7504141cb885430ca1679af5e89e638dd6fb32601696b34d7f827d7cca0a1cd38fd6f818b5cae25450b1ed3a6ac48059acd82";
	output["blockchain_private_key"] = "c231a5b395f271b6b03cb0b4490219995290f949071fceaec4ce127c410447b1";
	output["blockchain_address"] = "62fafaaa3db9d81c2123fdee77227571ada00d5b620e9cb591d06ee3b50ee79b";
	
	output["ml_solver_proto_path"] = "../../../dataset/MNIST/lenet_solver_memory.prototxt";
	output["ml_test_batch_size"] = 100;
	
	output["data_storage_service_port"] = 8040;
	output["data_storage_service_concurrency"] = 2;
	output["data_storage_db_path"] = "./dataset_db";
	output["data_storage_trigger_training_size"] = 64; // must be equal to the batch size of the model
	
	output["network"]["port"] = 8000;
	peer_enpoint item0 = {"127.0.0.1", 8000};
	peer_enpoint item1 = {"127.0.0.1", 8001};
	std::vector<configuration_file::json> peers = {item0.to_json(), item1.to_json()};
	output["network"]["peers"] = peers;
	
	output["transaction_count_per_model_update"] = 10;
	output["transaction_db_path"] = "./transaction_db";
	output["reputation_dll_path"] = "../reputation_sdk/sample/libreputation_api_sample.so";
	output["reputation_dll_datatype"] = "float";
	
	return output;
}
