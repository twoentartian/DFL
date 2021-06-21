#include <chrono>
#include <glog/logging.h>

#include <network.hpp>
#include <configure_file.hpp>

#include "data_generator.hpp"
#include "boost_serialization_wrapper.hpp"

configuration_file::json get_default_configuration()
{
	configuration_file::json output;
	output["dataset_path"] = "../../../dataset/MNIST/t10k-images.idx3-ubyte";
	output["dataset_label_path"] = "../../../dataset/MNIST/t10k-labels.idx1-ubyte";
	output["inject_interval_ms"] = "1000"; //ms
	output["inject_amount"] = "8";
	output["ip_address"] = "127.0.0.1";
	output["ip_port"] = "8040";
	return output;
}

int main(int argc, char **argv)
{
	std::string ip;
	uint16_t port;
	std::string dataset_path, label_dataset_path;
	std::chrono::milliseconds inject_interval;
	size_t inject_amount;
	
	configuration_file config;
	config.SetDefaultConfiguration(get_default_configuration());
	config.LoadConfiguration("./data_injector_config.json");
	
	//set dataset
	{
		auto dataset_path_opt = config.get<std::string>("dataset_path");
		if (dataset_path_opt) dataset_path = *dataset_path_opt;
		else LOG(ERROR) << "no dataset path provided";
		auto label_dataset_path_opt = config.get<std::string>("dataset_label_path");
		if (label_dataset_path_opt) label_dataset_path = *label_dataset_path_opt;
		else LOG(ERROR) << "no dataset label path provided";
	}
	
	//set ip
	{
		auto ip_opt = config.get<std::string>("ip_address");
		if (ip_opt) ip = *ip_opt;
		else LOG(ERROR) << "no ip address in config file";
		auto port_opt = config.get<std::string>("ip_port");
		if (port_opt)
		{
			try
			{
				port = std::stoi(*port_opt);
			}
			catch (...)
			{
				LOG(ERROR) << "invalid port";
			}
		}
		else LOG(ERROR) << "no port in config file";
	}
	
	//set injection
	{
		auto inject_interval_opt = config.get<std::string>("inject_interval_ms");
		try
		{
			if (inject_interval_opt) inject_interval = std::chrono::milliseconds(std::stoi(*inject_interval_opt));
			else LOG(ERROR) << "no inject interval in config file";
		}
		catch (...)
		{
			LOG(ERROR) << "invalid inject interval";
		}
		auto inject_amount_opt = config.get<std::string>("inject_amount");
		try
		{
			if (inject_amount_opt) inject_amount = std::stoi(*inject_amount_opt);
			else LOG(ERROR) << "no inject amount in config file";
		}
		catch (...)
		{
			LOG(ERROR) << "invalid inject amount";
		}
	}

	//load dataset
	LOG(INFO) << "loading dataset";
	Ml::data_converter<float> dataset;
	dataset.load_dataset_mnist(dataset_path, label_dataset_path);
	LOG(INFO) << "loading dataset - done";
	
	network::p2p client;
	bool _running = true;
	std::thread worker_thread([&](){
		auto loop_start_time = std::chrono::system_clock::now();
		int count = 0;
		while (_running)
		{
			count++;
			auto inject_data = dataset.get_random_data(inject_amount);
			std::stringstream inject_data_stream = serialize_wrap<boost::archive::binary_oarchive>(inject_data);
			std::string inject_data_str = inject_data_stream.str();
			
			client.send(ip,port,network::i_p2p_node::ipv4,inject_data_str.data(),inject_data_str.size(), [](network::p2p::send_packet_status status, const char* data, int length){
				std::string reply(data, length);
				if(status == network::p2p::send_packet_success)
				{
					LOG(INFO) << "Inject success";
				}
				else
				{
					LOG(INFO) << "Inject fail: " << network::i_p2p_node::send_packet_status_message[status];
				}
			});
			std::this_thread::sleep_until(loop_start_time + inject_interval * count);
		}
	});
	printf("press any key to stop\n");
	std::cin.get();
	printf("stopping ...\n");
	_running = false;
	worker_thread.join();
	return 0;
}

