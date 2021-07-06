#include <thread>

#include <glog/logging.h>

#include <configure_file.hpp>
#include <default_configuration.hpp>
#include <crypto.hpp>
#include <util.hpp>
#include <time_util.hpp>
#include <ml_layer.hpp>
#include <thread_pool.hpp>
#include <dll_importer.hpp>

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

constexpr char LOG_PATH[] = "./log/";
using model_datatype = float;

std::shared_ptr<Ml::MlCaffeModel<model_datatype, caffe::SGDSolver>> model_train;
std::shared_ptr<Ml::MlCaffeModel<model_datatype, caffe::SGDSolver>> model_test;
std::mutex training_lock;

std::shared_ptr<dataset_storage<model_datatype>> main_dataset_storage;
std::shared_ptr<transaction_generator> main_transaction_generator;
std::shared_ptr<transaction_tran_rece> main_transaction_tran_rece;
std::shared_ptr<transaction_storage> main_transaction_storage;
std::shared_ptr<reputation_manager> main_reputation_manager;

dll_loader<reputation_interface<model_datatype>> reputation_dll;
std::condition_variable exit_cv;
std::mutex exit_cv_lock;

void generate_transaction(const std::vector<Ml::tensor_blob_like<model_datatype>> &data, const std::vector<Ml::tensor_blob_like<model_datatype>> &label)
{
	Ml::caffe_parameter_net<model_datatype> parameter;
	float accuracy;
	{
		std::lock_guard guard(training_lock);
		model_train->train(data, label, false);
		auto[test_dataset_data, test_dataset_label] = main_dataset_storage->get_random_data(global_var::ml_test_batch_size);
		accuracy = model_train->evaluation(test_dataset_data, test_dataset_label);
		LOG(INFO) << "training complete, accuracy: " << accuracy;
		std_cout::println("[DFL] training complete, accuracy: " + std::to_string(accuracy));
		parameter = model_train->get_parameter();
	}
	transaction trans = main_transaction_generator->generate(parameter, std::to_string(accuracy));
	main_transaction_tran_rece->broadcast_transaction(trans);
	
	//record transaction
	//transaction_helper::save_to_file(trans, "./trans.json");
	
}

void receive_transaction(const transaction& trans)
{
	//record transaction
	//transaction_helper::save_to_file(trans, "./trans.json");
	
	auto [status, message] = verify_transaction(trans);
	if (!status)
	{
		LOG(WARNING) << "receive an invalid transaction, message: " << message;
		return;
	}
	else
	{
		LOG(INFO) << "receive transaction with hash " << trans.hash_sha256;
		std_cout::println("receive transaction with hash " + trans.hash_sha256);
	}
	
	main_transaction_storage->add(trans);
}

void update_model(std::shared_ptr<std::vector<transaction>> transactions)
{
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
	
	auto[test_dataset_data, test_dataset_label] = main_dataset_storage->get_random_data(global_var::ml_test_batch_size);
	double self_accuracy = model_train->evaluation(test_dataset_data, test_dataset_label);
	
	std::vector<updated_model<model_datatype>> received_models;
	received_models.resize(transactions->size());
	//TODO: diff_model logic not implemented
	for (int i = 0; i < received_models.size(); ++i)
	{
		received_models[i].type = model_type::normal;
		std::stringstream ss;
		ss << (*transactions)[i].content.model_data;
		received_models[i].model_parameter = deserialize_wrap<boost::archive::binary_iarchive, Ml::caffe_parameter_net<model_datatype>>(ss);
		received_models[i].generator_address = (*transactions)[i].content.creator.node_address;
		auto[test_dataset_data, test_dataset_label] = main_dataset_storage->get_random_data(global_var::ml_test_batch_size);
		if (test_dataset_data.empty())
		{
			LOG(ERROR) << "empty dataset db, cannot perform accuracy test";
			return;
		}
		model_test->set_parameter(received_models[i].model_parameter);
		received_models[i].accuracy = model_test->evaluation(test_dataset_data, test_dataset_label);
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
	
	//load configuration
	configuration_file config;
	config.SetDefaultConfiguration(get_default_configuration());
	config.LoadConfiguration("./config.json");
	
	//load reputation dll
	if (std::is_same_v<model_datatype, float>)
	{
		reputation_dll.load(*config.get<std::string>("reputation_dll_path"), export_class_name_reputation_float);
	}
	else if (std::is_same_v<model_datatype, double>)
	{
		reputation_dll.load(*config.get<std::string>("reputation_dll_path"), export_class_name_reputation_double);
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
	
	//start transaction_storage
	main_transaction_storage.reset(new transaction_storage());
	main_transaction_storage->add_event_callback(update_model);
	main_transaction_storage->set_event_trigger(*config.get<int>("transaction_count_per_model_update"));
	
	//start reputation manager
	main_reputation_manager.reset(new reputation_manager(*config.get<std::string>("transaction_db_path")));
	
	//start transaction_tran_rece
	main_transaction_tran_rece.reset(new transaction_tran_rece());
	main_transaction_tran_rece->start_listen(config.get_json()["network"]["port"]);
	auto peers = config.get_json()["network"]["peers"];
	for (auto &peer : peers)
	{
		main_transaction_tran_rece->add_peer(peer["address"], peer["port"]);
	}
	main_transaction_tran_rece->set_receive_transaction_callback(receive_transaction);
	
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
	
	std::cout << "press any key to exit" << std::endl;
	std::cin.get();
	//	std::unique_lock lock(exit_cv_lock);
	//	exit_cv.wait(lock);
	return 0;
}