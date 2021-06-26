#pragma once

#include <iostream>
#include <chrono>
#include <string>

#include <util.hpp>
#include <crypto.hpp>
#include <hash_interface.hpp>
#include <time_util.hpp>
#include <json_serialization.hpp>
#include <boost_serialization_wrapper.hpp>
#include <base64.hpp>

#include "ml_layer.hpp"
#include "node_info.hpp"

class transaction_without_hash_sig : i_hashable, i_json_serialization
{
public:
	using TIME_STAMP_TYPE = uint64_t;
	
	TIME_STAMP_TYPE creation_time;
	TIME_STAMP_TYPE expire_time;
	node_info creator;
	int TTL;
	
	std::string model_data;
	std::string accuracy;
	
	void to_byte_buffer(byte_buffer& target)
	{
		target.add(creation_time);
		target.add(expire_time);
		creator.to_byte_buffer(target);
		target.add(model_data);
		target.add(accuracy);
		target.add(TTL);
	}
	
	i_json_serialization::json to_json()
	{
		i_json_serialization::json output;
		output["creation_time"] = creation_time;
		output["expire_time"] = expire_time;
		i_json_serialization::json creator_json = creator.to_json();
		output["creator"] = creator_json;
		output["TTL"] = TTL;
		std::string model_data_base64 = Base64::Encode(model_data);
		output["model_data"] = model_data_base64;
		output["accuracy"] = accuracy;
		
		return output;
	}
	
	void from_json(const i_json_serialization::json& json_target)
	{
		creation_time = json_target["creation_time"];
		expire_time = json_target["expire_time"];
		creator.from_json(json_target["creator"]);
		TTL = json_target["TTL"];
		Base64::Decode(json_target["model_data"], model_data);
		accuracy = json_target["accuracy"];
	}

private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & creation_time;
		ar & expire_time;
		ar & creator;
		ar & model_data;
		ar & accuracy;
		ar & TTL;
	}
};

class transaction
{
public:
	transaction_without_hash_sig content;
	std::string hash_sha256;
	std::string signature;
	
	i_json_serialization::json to_json()
	{
		i_json_serialization::json output;
		i_json_serialization::json content_json = content.to_json();
		output["content"] = content_json;
		output["hash_sha256"] = hash_sha256;
		output["signature"] = signature;
		
		return output;
	}
	
	void from_json(const i_json_serialization::json& json_target)
	{
		content.from_json(json_target["content"]);
		hash_sha256 = json_target["hash_sha256"];
		signature = json_target["signature"];
	}
	
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & content;
		ar & hash_sha256;
		ar & signature;
	}
};

class transaction_generator
{
public:
	transaction_generator() = default;
	
	void set_key(const crypto::hex_data& private_key, const crypto::hex_data& public_key, const crypto::hex_data& address)
	{
		_private_key = private_key;
		_public_key = public_key;
		_address = address;
	}
	
	bool verify()
	{
		//check private key and public key
		std::string data = util::get_random_str();
		auto hash = crypto::md5::digest_s(data);
		auto signature = crypto::ecdsa_openssl::sign(hash, _private_key);
		bool pass = crypto::ecdsa_openssl::verify(signature, hash, _public_key);
		if (!pass) return false;
		
		//check node_name & pubkey
		auto pubkey_hex = _public_key.getHexMemory();
		auto sha256 = crypto::sha256::digest_s(pubkey_hex.data(), pubkey_hex.size());
		if (sha256 != _address) return false;
		return true;
	}
	
	transaction generate(const Ml::caffe_parameter_net<float>& parameter, std::string accuracy)
	{
		transaction output;
		output.content.creator.node_pubkey = _public_key.getTextStr_lowercase();
		output.content.creator.node_address = _address.getTextStr_lowercase();
		output.content.accuracy = accuracy;
		output.content.creation_time = time_util::get_current_utc_time();
		output.content.expire_time = output.content.creation_time + 60;
		output.content.TTL = 10;
		output.content.model_data = serialize_wrap<boost::archive::binary_oarchive>(parameter).str();
		
		//hash
		byte_buffer output_buffer;
		output.content.to_byte_buffer(output_buffer);
		auto hash_hex = crypto::sha256::digest_s(output_buffer.data(), output_buffer.size());
		output.hash_sha256 = hash_hex.getTextStr_lowercase();
		
		//signature
		auto signature_hex = crypto::ecdsa_openssl::sign(hash_hex, _private_key);
		output.signature = signature_hex.getTextStr_lowercase();
		
		return output;
	}


private:
	crypto::hex_data _private_key;
	crypto::hex_data _public_key;
	crypto::hex_data _address;
};