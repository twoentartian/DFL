#include <glog/logging.h>

#include <configure_file.hpp>
#include <default_configuration.hpp>
#include <crypto.hpp>
#include <util.hpp>
#include <time_util.hpp>
#include <ml_layer.hpp>
#include <thread>

#include "../transaction.hpp"
#include "../global_var.hpp"
#include "../init_system.hpp"
#include "../dataset_storage.hpp"

constexpr char LOG_PATH[] = "./log/";

std::shared_ptr<Ml::MlCaffeModel<float, caffe::SGDSolver>> model;
std::mutex training_lock;

std::shared_ptr<dataset_storage<float>> main_dataset_storage;
std::shared_ptr<transaction_generator> main_transaction_generator;

std::condition_variable exit_cv;
std::mutex exit_cv_lock;

void generate_transaction(const std::vector<Ml::tensor_blob_like<float>>&data, const std::vector<Ml::tensor_blob_like<float>>&label)
{
	Ml::caffe_parameter_net<float> parameter;
	float accuracy;
	{
		std::lock_guard guard(training_lock);
		model->train(data, label, false);
		auto [test_dataset_data, test_dataset_label] = main_dataset_storage->get_random_data(100);
		accuracy = model->evaluation(test_dataset_data, test_dataset_label);
		LOG(INFO) << "training complete, accuracy: " << accuracy;
		std::cout << "[DFL] training complete, accuracy: " << accuracy << std::endl;
		parameter = model->get_parameter();
	}
	transaction trans = main_transaction_generator->generate(parameter, std::to_string(accuracy));
	std::string trans_binary_str = serialize_wrap<boost::archive::binary_oarchive>(trans).str();
	std::cout << "binary transaction size: " << trans_binary_str.size() << std::endl;
	
	
	exit_cv.notify_one();
	

	
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
	
	//set public key and private key
	int ret = init_node_key_address(config);
	if (ret != 0) return ret;
	
	//transaction generator
	main_transaction_generator.reset(new transaction_generator());
	main_transaction_generator->set_key(global_var::private_key, global_var::public_key, global_var::address);
	CHECK(main_transaction_generator->verify()) << "verify private key/ public key/ address failed";
	
	//start ml model
	model.reset(new Ml::MlCaffeModel<float, caffe::SGDSolver>());
	model->load_caffe_model(*config.get<std::string>("ml_solver_proto_path"));
	
	//start data storage
	main_dataset_storage.reset(new dataset_storage<float>(*config.get<std::string>("data_storage_db_path"), *config.get<int>("data_storage_trigger_training_size")));
	main_dataset_storage->set_full_callback([](const std::vector<Ml::tensor_blob_like<float>>&data, const std::vector<Ml::tensor_blob_like<float>>&label){
		LOG(INFO) << "generating a transaction";
		static bool executed = false;
		if (!executed)
		{
			executed = true;
			std::thread generate_transaction_thread([&data,&label](){
				generate_transaction(data, label);
			});
			generate_transaction_thread.detach();
		}
	});
	uint16_t port = *config.get<uint16_t>("data_storage_service_port");
	size_t concurrency = *config.get<size_t>("data_storage_service_concurrency");
	LOG(INFO) << "starting network service for data storage, port: " << port << " concurrency: " << concurrency;
	main_dataset_storage->start_network_service(port, concurrency);
	
	std::cout << "wait for signal to exit" << std::endl;
	std::unique_lock lock(exit_cv_lock);
	exit_cv.wait(lock);
	return 0;
}