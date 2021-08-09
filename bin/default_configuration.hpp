#pragma once

#include <tuple>
#include <configure_file.hpp>
#include "global_types.hpp"

configuration_file::json get_default_configuration()
{
	configuration_file::json output;
	output["blockchain_public_key"] = "070a95eb6bd64eb273a2649a0ea61b932d50417275b8e250595c7a47468534a86fbd01f2f06efbd226ef86ab56abdd5cc296800794c9250c12c105fa3df86d3ad7";
	output["blockchain_private_key"] = "e321c369d8b1ff39aeabadc7846735e4fe5178af41b87eb421a6773c8c4810c3";
	output["blockchain_address"] = "a49487e4550b9961af17161900c8496a4ddc0638979d3c4f9b305d2b4dc0c665";
	
	output["blockchain_block_db_path"] = "./blocks";
	output["blockchain_estimated_block_size"] = 10;
	
	output["ml_solver_proto_path"] = "../../../dataset/MNIST/lenet_solver_memory.prototxt";
	output["ml_test_batch_size"] = 100;
	output["ml_model_stream_type"] = "compressed";  //compressed or normal
	output["ml_model_stream_compressed_filter_limit"] = 0.5;
	
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
	
	output["enable_profiler"] = true;
	
	return output;
}
