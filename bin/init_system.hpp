#pragma once

#include <glog/logging.h>

#include "global_var.hpp"
#include "configure_file.hpp"

int init_node_key_address(const configuration_file& config)
{
	LOG(INFO) << "checking keys and address";
	
	auto pubkey_str = config.get<std::string>("blockchain_public_key");
	if (!pubkey_str)
	{
		LOG(FATAL) << "invalid public key";
		return -1;
	}
	global_var::public_key.assign(*pubkey_str);
	
	auto prikey_str = config.get<std::string>("blockchain_private_key");
	if (!prikey_str)
	{
		LOG(FATAL) << "invalid private key";
		return -1;
	}
	global_var::private_key.assign(*prikey_str);
	
	auto address_str = config.get<std::string>("blockchain_address");
	if (!address_str)
	{
		LOG(FATAL) << "invalid address";
		return -1;
	}
	global_var::address.assign(*address_str);
	return 0;
}