#include <thread>

#include <glog/logging.h>

#include <configure_file.hpp>

#include <crypto.hpp>
#include <util.hpp>
#include <time_util.hpp>
#include <ml_layer.hpp>
#include <thread_pool.hpp>
#include <dll_importer.hpp>
#include <duplicate_checker.hpp>
#include <performance_profiler.hpp>

#include "../transaction.hpp"
#include "../global_var.hpp"
#include "../init_system.hpp"
#include "../dataset_storage.hpp"
#include "../transaction_tran_rece.hpp"
#include "../transaction_storage.hpp"
#include "../std_output.hpp"
#include "../transaction_verifier.hpp"
#include "../reputation_manager.hpp"
#include "../reputation_sdk.hpp"
#include "../transaction_storage_for_block.hpp"
#include "../block_manager.hpp"
#include "../reputation_dll_test.hpp"
#include "default_configuration.hpp"

constexpr char LOG_PATH[] = "./log/";
using model_datatype = float;

std::shared_ptr<Ml::MlCaffeModel<model_datatype, caffe::SGDSolver>> model_train;
std::shared_ptr<Ml::MlCaffeModel<model_datatype, caffe::SGDSolver>> model_test;
std::mutex training_lock;

std::shared_ptr<dataset_storage<model_datatype>> main_dataset_storage;  //store machine learning dataset
std::shared_ptr<transaction_generator> main_transaction_generator;  //generate transactions, receipts
std::shared_ptr<transaction_tran_rece> main_transaction_tran_rece;  //send/receive transactions/blocks
std::shared_ptr<transaction_storage> main_transaction_storage;      //store transactions for machine learning
std::shared_ptr<reputation_manager> main_reputation_manager;        //manage reputation
std::shared_ptr<transaction_storage_for_block> main_transaction_storage_for_block;  //store verified transactions and block cache
std::shared_ptr<block_manager> main_block_manager;                  //generate blocks, store blocks.

dll_loader<reputation_interface<model_datatype>> reputation_dll;
std::condition_variable exit_cv;
std::mutex exit_cv_lock;

void generate_block()
{
	std::shared_ptr<profiler_auto> profiler_p;
	if (global_var::enable_profiler)
		profiler_p.reset(new profiler_auto("generate_block"));
	
	LOG(INFO) << "generating block";
	std_cout::println("[DFL] generating block");
	auto cached_transactions_with_receipt = main_transaction_storage_for_block->dump_block_cache(std::ceil(float(main_transaction_tran_rece->get_peers().size()) / 2));//we need to collect at least over 50% confirmations to generate a block
	if (cached_transactions_with_receipt.empty())
	{
		std::stringstream ss;
		ss << "no transactions are confirmed, cannot generate block";
		LOG(INFO) << ss.str();
		std_cout::println(ss.str());
		return;
	}
	else
	{
		std::stringstream ss;
		ss << "block transaction size: " << cached_transactions_with_receipt.size();
		LOG(INFO) << ss.str();
		std_cout::println(ss.str());
	}
	
	auto generated_block = *main_block_manager->generate_block(cached_transactions_with_receipt);
	
	std::vector<block_confirmation> confirmations;
	{
		if (global_var::enable_profiler)
		{
			profiler_auto profiler_confirm("generate_block/gather_confirmation");
			confirmations = main_transaction_tran_rece->broadcast_block_and_receive_confirmation(generated_block);
		}
		else
		{
			confirmations = main_transaction_tran_rece->broadcast_block_and_receive_confirmation(generated_block);
		}
	}
	for (auto& confirmation: confirmations)
	{
		main_block_manager->append_block_confirmation(confirmation);
	}
	auto final_block = main_block_manager->store_finalized_block();
	
	std::stringstream ss;
	ss << "generating block - done, transaction count: " << final_block.content.transaction_container.size() << " confirmation count: " << confirmations.size() << " height: " << final_block.height;
	LOG(INFO) << ss.str();
	std_cout::println(ss.str());
}

void generate_transaction(const std::vector<Ml::tensor_blob_like<model_datatype>> &data, const std::vector<Ml::tensor_blob_like<model_datatype>> &label)
{
	std::shared_ptr<profiler_auto> profiler_p;
	if (global_var::enable_profiler)
		profiler_p.reset(new profiler_auto("generate_transaction"));
	
	Ml::caffe_parameter_net<model_datatype> net_after,net_before;
	float accuracy;
	{
		std::lock_guard guard(training_lock);
		net_before = model_train->get_parameter();
		model_train->train(data, label, false);
		auto[test_dataset_data, test_dataset_label] = main_dataset_storage->get_random_data(global_var::ml_test_batch_size);
		{
			std::shared_ptr<profiler_auto> profiler_p;
			if (global_var::enable_profiler)
				profiler_p.reset(new profiler_auto("generate_transaction/measure_accuracy"));
			accuracy = model_train->evaluation(test_dataset_data, test_dataset_label);
		}
		LOG(INFO) << "training complete, accuracy: " << accuracy;
		std_cout::println("[DFL] training complete, accuracy: " + std::to_string(accuracy));
		net_after = model_train->get_parameter();
	}
	
	std::string parameter_str;
	if (global_var::ml_model_stream_type == "compressed")
	{
		parameter_str = Ml::model_interpreter<float>::generate_model_stream_by_compress_diff(net_before, net_after, global_var::ml_model_stream_compressed_filter_limit);
	}
	else if (global_var::ml_model_stream_type == "normal")
	{
		parameter_str = Ml::model_interpreter<float>::generate_model_stream_normal(net_after);
	}
	else
	{
		LOG(WARNING) << "unknown ml_model_stream_type";
	}
	
	transaction trans = main_transaction_generator->generate(parameter_str, std::to_string(accuracy));
	
	main_transaction_storage_for_block->add_to_block_cache(trans);
	if (main_transaction_storage_for_block->block_cache_size() >= global_var::estimated_transaction_per_block)
	{
		std::shared_ptr<profiler_auto> profiler_triggered_generating_block;
		if (global_var::enable_profiler)
			profiler_triggered_generating_block.reset(new profiler_auto("generate_transaction/triggered_generating_block"));
		
		//trigger dump
		generate_block();
	}
	
	std::shared_ptr<profiler_auto> profiler_broadcast;
	if (global_var::enable_profiler)
		profiler_broadcast.reset(new profiler_auto("generate_transaction/broadcast_transaction"));
	main_transaction_tran_rece->broadcast_transaction(trans);
	
	//record transaction
	//transaction_helper::save_to_file(trans, "./trans.json");
}

void receive_transaction(const transaction& trans)
{
	std::shared_ptr<profiler_auto> profiler_p;
	if (global_var::enable_profiler)
		profiler_p.reset(new profiler_auto("receive_transaction"));
	
	static duplicate_checker<std::string> transaction_duplicate_checker(3600);
	
	//verify transaction
	auto [status, message] = verify_transaction(trans);
	if (!status)
	{
		LOG(WARNING) << "receive an invalid transaction, message: " << message;
		return;
	}

	//check is this transaction generated by myself?
	if (trans.content.creator.node_address == global_var::address.getTextStr_lowercase())
	{
		//yes, this is my transaction
		main_transaction_storage_for_block->add_to_block_cache(trans);
		return;
	}
	
	//check duplication
	bool exist = transaction_duplicate_checker.find(trans.hash_sha256);
	if (exist)
	{
		//duplicate transaction
		LOG(INFO) << "duplicate transaction with hash " << trans.hash_sha256;
		std_cout::println("duplicate transaction with hash " + trans.hash_sha256);
		return;
	}
	
	//new transaction
	transaction_duplicate_checker.add(trans.hash_sha256);
	LOG(INFO) << "receive transaction with hash " << trans.hash_sha256;
	std_cout::println("receive transaction with hash " + trans.hash_sha256);
	
	//calculate accuracy
	float accuracy;
	{
		std::shared_ptr<profiler_auto> profiler_calculate_accuracy;
		if (global_var::enable_profiler)
			profiler_calculate_accuracy.reset(new profiler_auto("receive_transaction/calculate_accuracy"));
		
		auto [model_parameter, model_type] = Ml::model_interpreter<model_datatype>::parse_model_stream(trans.content.model_data);
		auto[test_dataset_data, test_dataset_label] = main_dataset_storage->get_random_data(global_var::ml_test_batch_size);
		if (test_dataset_data.empty())
		{
			LOG(ERROR) << "empty dataset db, cannot perform accuracy test";
			return;
		}
		
		if (model_type == Ml::model_compress_type::compressed_by_diff)
		{
			Ml::caffe_parameter_net<model_datatype> parameter = model_train->get_parameter();
			auto self_parameter_copy = parameter;
			self_parameter_copy.patch_weight(model_parameter);
			model_test->set_parameter(self_parameter_copy);
			accuracy = model_test->evaluation(test_dataset_data, test_dataset_label);
		}
		else if (model_type == Ml::model_compress_type::normal)
		{
			model_test->set_parameter(model_parameter);
			accuracy = model_test->evaluation(test_dataset_data, test_dataset_label);
		}
		else
		{
			LOG(WARNING) << "unknown ml_model_stream_type";
		}
	}
	
	//append receipt
	transaction new_transaction = trans;
	auto [receipt_status,receipt] = main_transaction_generator->append_receipt(new_transaction, std::to_string(accuracy));
	if (receipt_status == transaction_generator::append_receipt_status::success)
	{
		//add to the verified transaction
		main_transaction_storage_for_block->add_verified_transaction(trans, *receipt);
		
		//add the new transaction and broadcast
		main_transaction_storage->add(new_transaction);
		
		std::shared_ptr<profiler_auto> profiler_broadcast_new_transaction;
		if (global_var::enable_profiler)
			profiler_broadcast_new_transaction.reset(new profiler_auto("receive_transaction/broadcast_generated_transaction"));
		main_transaction_tran_rece->broadcast_transaction(new_transaction);
	}
}

// args: transactions should contain the receipt generated by self
void update_model(std::shared_ptr<std::vector<transaction>> transactions)
{
	std::shared_ptr<profiler_auto> profiler_p;
	if (global_var::enable_profiler)
		profiler_p.reset(new profiler_auto("update_model"));
	
	static std::mutex update_model_reputation_lock;
	std::lock_guard guard(update_model_reputation_lock);
	
	std_cout::println("[DFL] begin updating model");
	
	LOG(INFO) << "start updating model - retrieving reputation";
	std::unordered_map<std::string, double> reputation_map;
	for (int i = 0; i < transactions->size(); ++i)
	{
		reputation_map[(*transactions)[i].content.creator.node_address] = 0;
	}
	
	main_reputation_manager->get_reputation_map(reputation_map);
	LOG(INFO) << "start updating model - retrieving reputation - done";
	
	Ml::caffe_parameter_net<model_datatype> parameter = model_train->get_parameter();
	
	double self_accuracy;
	{
		std::shared_ptr<profiler_auto> profiler_calculate_self_accuracy;
		if (global_var::enable_profiler)
			profiler_calculate_self_accuracy.reset(new profiler_auto("update_model/calculate_self_accuracy"));
		auto[test_dataset_data, test_dataset_label] = main_dataset_storage->get_random_data(global_var::ml_test_batch_size);
		self_accuracy = model_train->evaluation(test_dataset_data, test_dataset_label);
	}
	
	std::vector<updated_model<model_datatype>> received_models;
	received_models.resize(transactions->size());
	for (int i = 0; i < received_models.size(); ++i)
	{
		Ml::model_compress_type model_type;
		std::tie(received_models[i].model_parameter, model_type) = Ml::model_interpreter<model_datatype>::parse_model_stream((*transactions)[i].content.model_data);
		
		received_models[i].generator_address = (*transactions)[i].content.creator.node_address;
		received_models[i].type = model_type;
		
		//set accuracy
		bool found_flag = false;
		for (auto& [receipt_hash, receipt]:(*transactions)[i].receipts)
		{
			if (receipt.content.creator.node_address == global_var::address.getTextStr_lowercase())
			{
				//find the generated accuracy
				found_flag = true;
				received_models[i].accuracy = std::stof(receipt.content.accuracy);
			}
		}
		LOG_IF(WARNING, !found_flag) << "cannot find the receipt to retrieve the accuracy data";
	}
	
	reputation_dll.get()->update_model(parameter, self_accuracy, received_models, reputation_map);
	model_train->set_parameter(parameter);
	main_reputation_manager->update_reputation(reputation_map);
	
	//display reputation and accuracy.
	std_cout::println("[[DEBUG]] REPUTATION:");
	for (auto& reputation_item: reputation_map)
	{
		std_cout::println("[[DEBUG]] reputation: " + reputation_item.first + ":" +  std::to_string(reputation_item.second));
	}
	
	std_cout::println("[[DEBUG]] DEBUG ACCURACY:");
	for (auto& model_item: received_models)
	{
		std_cout::println("[[DEBUG]] model accuracy: " + model_item.generator_address + ":" +  std::to_string(model_item.accuracy));
	}
	
}

int main(int argc, char **argv)
{
	//log file path
	google::InitGoogleLogging(argv[0]);
	std::filesystem::path log_path(LOG_PATH);
	if (!std::filesystem::exists(log_path)) std::filesystem::create_directories(log_path);
	google::SetLogDestination(google::INFO, log_path.c_str());
	google::SetStderrLogging(google::WARNING);
	
	//load configuration
	configuration_file config;
	config.SetDefaultConfiguration(get_default_configuration());
	auto ret_code = config.LoadConfiguration("./config.json");
	if(ret_code < configuration_file::NoError)
	{
		if (ret_code == configuration_file::FileFormatError)
			LOG(FATAL) << "configuration file format error";
		else
			LOG(FATAL) << "configuration file error code: " << ret_code;
	}
	
	//load reputation dll
	if constexpr(std::is_same_v<model_datatype, float>)
	{
		auto [status,msg] = reputation_dll.load(*config.get<std::string>("reputation_dll_path"), export_class_name_reputation_float);
		LOG_IF(FATAL, !status) << "cannot load reputation dll: " << msg;
	}
	else if constexpr(std::is_same_v<model_datatype, double>)
	{
		auto [status,msg] = reputation_dll.load(*config.get<std::string>("reputation_dll_path"), export_class_name_reputation_double);
		LOG_IF(FATAL, !status) << "cannot load reputation dll: " << msg;
	}
	else
	{
		LOG(FATAL) << "unknown model datatype";
	}
	
	//set public key and private key
	int ret = init_node_key_address(config);
	if (ret != 0) return ret;
	
	//start transaction generator
	main_transaction_generator.reset(new transaction_generator());
	main_transaction_generator->set_key(global_var::private_key, global_var::public_key, global_var::address);
	CHECK(main_transaction_generator->verify_key()) << "verify_key private key/ public key/ address failed";
	
	//start ml model
	model_train.reset(new Ml::MlCaffeModel<float, caffe::SGDSolver>());
	model_train->load_caffe_model(*config.get<std::string>("ml_solver_proto_path"));
	model_test.reset(new Ml::MlCaffeModel<float, caffe::SGDSolver>());
	model_test->load_caffe_model(*config.get<std::string>("ml_solver_proto_path"));
	
	//model stream type
	global_var::ml_model_stream_type = *config.get<std::string>("ml_model_stream_type");
	global_var::ml_model_stream_compressed_filter_limit = *config.get<float>("ml_model_stream_compressed_filter_limit");
	
	//enable_profiler
	global_var::enable_profiler = *config.get<bool>("enable_profiler");
	std::shared_ptr<profiler_auto> profiler_p;
	if (global_var::enable_profiler)
		profiler_p.reset(new profiler_auto("application_running"));
	
	//global blockchain config
	global_var::estimated_transaction_per_block = *config.get<int>("blockchain_estimated_block_size");
	
	//start transaction_storage
	main_transaction_storage.reset(new transaction_storage());
	main_transaction_storage->add_event_callback(update_model);
	main_transaction_storage->set_event_trigger(*config.get<int>("transaction_count_per_model_update"));
	
	//start reputation manager
	main_reputation_manager.reset(new reputation_manager(*config.get<std::string>("transaction_db_path")));
	
	//block database
	std::string blockchain_block_db_path = *config.get<std::string>("blockchain_block_db_path");
	main_block_manager.reset(new block_manager(blockchain_block_db_path));
	main_block_manager->set_genesis_content(model_train->get_network_structure_info());
	
	//start block cache and transaction storage
	main_transaction_storage_for_block.reset(new transaction_storage_for_block(blockchain_block_db_path));
	
	//start data storage
	main_dataset_storage.reset(new dataset_storage<model_datatype>(*config.get<std::string>("data_storage_db_path"), *config.get<int>("data_storage_trigger_training_size")));
	main_dataset_storage->set_full_callback([](const std::vector<Ml::tensor_blob_like<model_datatype>> &data, const std::vector<Ml::tensor_blob_like<model_datatype>> &label)
	                                        {
		                                        LOG(INFO) << "plenty data, start training";
		                                        std_cout::println("[DFL] plenty data, start training");
		                                        std::thread generate_transaction_thread([&data, &label]()
		                                                                                {
			                                                                                generate_transaction(data, label);
		                                                                                });
		                                        generate_transaction_thread.detach();
	                                        });
	uint16_t port = *config.get<uint16_t>("data_storage_service_port");
	size_t concurrency = *config.get<size_t>("data_storage_service_concurrency");
	LOG(INFO) << "starting network service for data storage, port: " << port << ", concurrency: " << concurrency;
	main_dataset_storage->start_network_service(port, concurrency);
	
	//start transaction_tran_rece
	{
		main_transaction_tran_rece.reset(new transaction_tran_rece(global_var::public_key, global_var::private_key, global_var::address,
															 main_transaction_storage_for_block, config.get_json()["network"]["use_preferred_peers_only"]));
		main_transaction_tran_rece->start_listen(config.get_json()["network"]["port"]);
		auto preferred_peers = config.get_json()["network"]["peers"];
		main_transaction_tran_rece->get_maximum_peer() = config.get_json()["network"]["maximum_peer"];
		main_transaction_tran_rece->get_inactive_time() = config.get_json()["network"]["inactive_peer_second"];
		for (auto &single_preferred_peer : preferred_peers)
		{
			main_transaction_tran_rece->add_preferred_peer(single_preferred_peer.get<std::string>());
		}
		main_transaction_tran_rece->set_receive_transaction_callback(receive_transaction);
		
		configuration_file::json introducers = config.get_json()["network"]["introducers"];
		for (configuration_file::json& single_introducer_json : introducers)
		{
			main_transaction_tran_rece->add_introducer(single_introducer_json["address"].get<std::string>(),
			                                           single_introducer_json["ip"].get<std::string>(),
			                                           single_introducer_json["public_key"].get<std::string>(),
			                                           single_introducer_json["port"].get<uint16_t>());
		}
		
	}
	
	//perform reputation test
	Ml::caffe_parameter_net<model_datatype> model = model_train->get_parameter();
	{
		auto [pass, info] = reputation_dll_same_reputation_test(reputation_dll, model);
		std::stringstream ss;
		ss << "[reputation dll] [same reputation test] " << (pass?"pass":"fail") << ": " << info;
		std_cout::println(ss.str());
	}
	{
		auto [pass, info] = reputation_dll_same_model_test(reputation_dll, model);
		std::stringstream ss;
		ss << "[reputation dll] [same model test] " << (pass?"pass":"fail") << ": " << info;
		std_cout::println(ss.str());
	}
	
	std::cout << "press any key to exit" << std::endl;
	std::cin.get();
	//	std::unique_lock lock(exit_cv_lock);
	//	exit_cv.wait(lock);
	return 0;
}
