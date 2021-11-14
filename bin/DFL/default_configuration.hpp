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
	output["ml_model_stream_type"] = "normal";  //compressed or normal
	output["ml_model_stream_compressed_filter_limit"] = 0.5;
	
	output["data_storage_service_port"] = 8040;
	output["data_storage_service_concurrency"] = 2;
	output["data_storage_db_path"] = "./dataset_db";
	output["data_storage_trigger_training_size"] = 64; // must be equal to the batch size of the model
	
	output["network"]["port"] = 8000;
	std::string item0 = {"address"};
	std::string item1 = {"address"};
	std::vector<configuration_file::json> peers = {item0, item1};
	output["network"]["preferred_peers"] = peers;
	output["network"]["maximum_peer"] = 10;
	output["network"]["use_preferred_peers_only"] = false;
	output["network"]["inactive_peer_second"] = 60;
	
	configuration_file::json introducer;
	introducer["ip"] = "127.0.0.1";
	introducer["port"] = 5666;
	introducer["address"] = "94c4ccdec72c2955f46fc1e1de9d5db0a6a4664f5085cd90149d392cc3fef803";
	introducer["public_key"] = "0712335841163e55f1540189fd9ec800343b34877ff72d0646f5c3e5fd2f990846df32dc4617a4efceffe46329e3f48b0078a6e1ddc5a69c51bf2dbe4242bcba25";
	output["network"]["introducers"] = {introducer};
	
	output["transaction_count_per_model_update"] = 10;
	output["transaction_db_path"] = "./transaction_db";
	output["reputation_dll_path"] = "../reputation_sdk/sample/libreputation_api_sample.so";
	output["reputation_dll_datatype"] = "float";
	
	output["enable_profiler"] = true;
	
	return output;
}
