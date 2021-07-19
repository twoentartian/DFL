#pragma once

#include <iostream>
#include <chrono>
#include <string>

#include <hash_interface.hpp>
#include <json_serialization.hpp>

class node_info : i_hashable, i_json_serialization
{
public:
	std::string node_address;
	std::string node_pubkey;
	
	void to_byte_buffer(byte_buffer& target) const
	{
		target.add(node_address);
		target.add(node_pubkey);
	}
	
	i_json_serialization::json to_json() const
	{
		i_json_serialization::json output;
		output["node_address"] = node_address;
		output["node_pubkey"] = node_pubkey;
		return output;
	}
	
	void from_json(const json& json_target)
	{
		node_address = json_target["node_address"];
		node_pubkey = json_target["node_pubkey"];
	}
	
	bool operator==(const node_info& target) const
	{
		return (node_address == target.node_address) && (node_pubkey == target.node_pubkey);
	}
	
	bool operator!=(const node_info& target) const
	{
		return !operator==(target);
	}

private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & node_address;
		ar & node_pubkey;
	}
	
};