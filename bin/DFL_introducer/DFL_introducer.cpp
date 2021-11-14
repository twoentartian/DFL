//
// Created by tyd on 07-11-21.
//

#include <filesystem>

#include <glog/logging.h>

#include <configure_file.hpp>
#include "DFL_introducer_default_configuration.hpp"
#include "DFL_introducer_p2p.hpp"

constexpr char INTRODUCER_LOG_PATH[] = "./introducer_log/";

std::shared_ptr<introducer_p2p> introducer_server;

int main(int argc, char **argv)
{
	//log file path
	google::InitGoogleLogging(argv[0]);
	std::filesystem::path log_path(INTRODUCER_LOG_PATH);
	if (!std::filesystem::exists(log_path)) std::filesystem::create_directories(log_path);
	google::SetLogDestination(google::INFO, log_path.c_str());
	google::SetStderrLogging(google::WARNING);
	
	//load configuration
	configuration_file config;
	config.SetDefaultConfiguration(get_default_introducer_configuration());
	auto ret_code = config.LoadConfiguration("./introducer_config.json");
	if(ret_code < configuration_file::NoError)
	{
		if (ret_code == configuration_file::FileFormatError)
			LOG(FATAL) << "configuration file format error";
		else
			LOG(FATAL) << "configuration file error code: " << ret_code;
	}
	
	uint16_t port = *config.get<uint16_t>("port");
	std::string blockchain_public_key = *config.get<std::string>("blockchain_public_key");
	std::string blockchain_private_key = *config.get<std::string>("blockchain_private_key");
	std::string blockchain_address = *config.get<std::string>("blockchain_address");
	
	//verify
	if (!dfl_util::verify_private_public_key(blockchain_private_key, blockchain_public_key))
	{
		LOG(FATAL) << "invalid private/public key pair";
	}
	if (!dfl_util::verify_address_public_key(blockchain_address, blockchain_public_key))
	{
		LOG(FATAL) << "invalid address/public key";
	}
	
	introducer_server.reset(new introducer_p2p(blockchain_public_key, blockchain_private_key, blockchain_address));
	introducer_server->start_listen(port);
	introducer_server->add_new_peer_callback([](const peer_endpoint& peer){
		std::stringstream info_ss;
		info_ss << "add peer " << peer.name << " with endpoint " << peer.address <<":"<< peer.port;
		dfl_util::print_info_to_log_stdcout(info_ss);
		std::cout << ">> ";
		std::cout.flush();
	});
	
	while (true)
	{
		std::string user_input;
		std::cout << ">> ";
		std::getline(std::cin, user_input);
		if (user_input == "q")
		{
			std::cout << "quitting" << std::endl;
			break;
		}
		else
		{
			std::cout << "unknown command" << std::endl;
			continue;
		}
	}

	return 0;
}