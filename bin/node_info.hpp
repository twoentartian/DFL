#pragma once

#include <iostream>
#include <chrono>
#include <string>

#include "hash_interface.hpp"

class node_info : i_hashable
{
public:
	std::string node_address;
	std::string node_pubkey;
	
	void to_byte_buffer(byte_buffer& target)
	{
		target.add(node_address);
		target.add(node_pubkey);
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