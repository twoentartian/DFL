#pragma once

#include <string>
#include <utility>

#include <configure_file.hpp>
#include <hash_interface.hpp>
#include <boost_serialization_wrapper.hpp>

class peer_endpoint : i_hashable
{
public:
	enum peer_type
	{
		peer_type_unknown = 0,
		peer_type_normal_node,
		peer_type_introducer
	};
	
	std::string name; //hash(public_key)
	std::string address; //network address
	std::string public_key;
	uint16_t port;
	peer_type type;
	
	peer_endpoint() : port(0), type(peer_type_unknown)
	{

	}
	
	peer_endpoint(std::string name, std::string public_key, std::string address, uint16_t port, peer_type type) : name(std::move(name)), address(std::move(address)), public_key(std::move(public_key)), port(port), type(type)
	{
	
	}
	
	[[nodiscard]] std::string to_string() const
	{
		return name + "-" + address + ":" + std::to_string(port);
	}
	
	configuration_file::json to_json()
	{
		configuration_file::json output;
		output["name"] = name;
		output["address"] = address;
		output["port"] = port;
		output["type"] = type;
		return output;
	}
	
	void to_byte_buffer(byte_buffer& target) const override
	{
		target.add(name);
		target.add(address);
		target.add(port);
		target.add(type);
	}

private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & name;
		ar & address;
		ar & port;
		ar & type;
	}
	
};

class request_peer_info_data : i_hashable
{
public:
	std::string requester_address;
	std::vector<peer_endpoint> peers_info;
	std::string generator_address;
	std::string node_pubkey;

	std::string hash;
	std::string signature;
	
	void to_byte_buffer(byte_buffer& target) const override
	{
		target.add(requester_address);
		for (auto& single_peer : peers_info)
		{
			single_peer.to_byte_buffer(target);
		}
		target.add(node_pubkey);
		target.add(generator_address);
	}
	
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & requester_address;
		ar & peers_info;
		ar & generator_address;
		ar & node_pubkey;
		
		ar & hash;
		ar & signature;
	}
};

class register_as_peer_data : i_hashable
{
public:
	std::string address;
	std::string node_pubkey;
	uint16_t port;
	
	std::string hash;
	std::string signature;
	
	void to_byte_buffer(byte_buffer& target) const override
	{
		target.add(address);
		target.add(node_pubkey);
		target.add(port);
	}
	
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & address;
		ar & node_pubkey;
		ar & port;
		
		ar & hash;
		ar & signature;
	}
	
};