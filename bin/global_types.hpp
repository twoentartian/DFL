#pragma once

#include <string>
#include <utility>

class peer_enpoint
{
public:
	std::string address;
	uint16_t port;
	
	peer_enpoint() = default;
	peer_enpoint(std::string address, uint16_t port) : address(std::move(address)), port(port)
	{
	
	}
	
	[[nodiscard]] std::string to_string() const
	{
		return address + ":" + std::to_string(port);
	}
	
	configuration_file::json to_json()
	{
		configuration_file::json output;
		output["address"] = address;
		output["port"] = port;
		return output;
	}
};